#include "engine/session.hpp"

#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace bloodbath::session {
namespace {
void validate(const Input& input) {
    // Reuse the subset's living/no-buildings/unique-90-card boundary.
    (void)base_rules::WorkBatch(input.colonies, input.config.seed, input.config.scope);
    const auto& progress = input.progress;
    (void)progression::Deck(progress, input.config.seed);
    if (progress.active) throw std::invalid_argument("session input must be at a resolved card boundary");
    if (progress.clock.absolute_event || progress.clock.cycle || progress.clock.position ||
        progress.clock.decision || progress.shuffle_index || progress.addition_index)
        throw std::invalid_argument("session starts a new local trace, not an unverified midgame restore");
    for (const auto& colony : input.colonies.colonies)
        if (colony.resources.people > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
            throw std::overflow_error("population exceeds official score type");
    std::set<progression::Card> used(progress.deck.begin(), progress.deck.end());
    used.insert(progress.discard.begin(), progress.discard.end());
    for (const auto card : input.config.ordered_events) {
        if (!used.insert(card).second) throw std::invalid_argument("Event queue duplicates a progression instance");
        const auto effect = input.config.effects.find(card);
        if (effect != input.config.effects.end() &&
            (effect->second == Effect::work || effect->second == Effect::trouble))
            throw std::invalid_argument("a known starting card is not an Event queue entry");
    }
    for (const auto& [card, effect] : input.config.effects) {
        (void)card;
        if (effect != Effect::work && effect != Effect::trouble && effect != Effect::hunger &&
            effect != Effect::accident && effect != Effect::instruction_manual && effect != Effect::unsupported)
            throw std::invalid_argument("invalid effect tag");
    }
}
Effect effect_of(const Config& config, progression::Card card) {
    const auto found = config.effects.find(card);
    return found == config.effects.end() ? Effect::unsupported : found->second;
}
void execute(Session& session, const Command& command) {
    switch (command.kind) {
    case CommandKind::advance: (void)session.advance(); break;
    case CommandKind::commit: session.commit(command.player, command.action); break;
    case CommandKind::resolve_work: (void)session.resolve_work(command.schedule); break;
    default: throw std::invalid_argument("invalid replay command");
    }
}
} // namespace
Session::Session(Input input) : config_(input.config) {
    validate(input);
    state_.colonies = std::move(input.colonies);
    state_.progress = std::move(input.progress);
}
void Session::terminal(Snapshot& next, const EventClock& clock, const std::string& cause) {
    const Scores scores{static_cast<std::int64_t>(next.colonies.colonies[0].resources.people),
                        static_cast<std::int64_t>(next.colonies.colonies[1].resources.people)};
    next.terminal_evidence = TerminalEvidence{clock, scores, cause};
    if (scores[0] == scores[1]) {
        next.phase = Phase::unresolved_tie;
    } else {
        next.official = OfficialOutcome{clock, scores,
            scores[0] > scores[1] ? Winner::player0 : Winner::player1, cause};
        next.phase = Phase::official_terminal;
    }
}
Phase Session::advance() {
    if (state_.phase != Phase::ready) return state_.phase;
    auto next = state_;
    progression::Deck deck(next.progress, config_.seed);
    const auto reveal = deck.reveal();
    if (!reveal) {
        next.phase = Phase::no_progress_cards;
        state_ = std::move(next);
        return state_.phase;
    }
    switch (effect_of(config_, reveal->card)) {
    case Effect::work:
        deck.record_decision(); // One simultaneous Work decision point, not two reveals.
        next.phase = Phase::work_decision;
        break;
    case Effect::trouble:
        if (next.next_event == config_.ordered_events.size()) {
            next.phase = Phase::blocked_event_queue;
        } else {
            deck.add_group({config_.ordered_events[next.next_event]});
            ++next.next_event;
            deck.finish_active();
        }
        break;
    case Effect::hunger:
        // p6: zero buildings => zero food payment, zero deaths, +2 base people.
        for (auto& colony : next.colonies.colonies) {
            const auto limit = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
            if (colony.resources.people > limit - 2) throw std::overflow_error("Hunger population overflow");
            colony.resources.people += 2;
        }
        deck.finish_active();
        break;
    case Effect::accident: {
        // p2 card image: -3 people. Do not settle unpayable simultaneous scoring.
        bool unpayable = false;
        for (const auto& colony : next.colonies.colonies)
            unpayable = unpayable || colony.resources.people < 3;
        if (unpayable) { next.phase = Phase::blocked_population_payment; break; }
        for (Player p = 0; p < 2; ++p) {
            population::Loss loss({next.colonies.colonies[p].resources.people, {}, {}}, p, 3,
                                  reveal->clock, "Accident");
            next.colonies.colonies[p].resources.people = loss.snapshot().base_people;
            if (loss.result().death && !next.deaths[p]) next.deaths[p] = loss.result().death;
        }
        if (next.deaths[0] || next.deaths[1]) terminal(next, reveal->clock, "Accident");
        deck.finish_active();
        break;
    }
    case Effect::instruction_manual:
        terminal(next, reveal->clock, "Instruction Manual");
        deck.finish_active();
        break;
    case Effect::unsupported:
        next.phase = Phase::blocked_unknown_card;
        break;
    }
    next.progress = deck.snapshot();
    state_ = std::move(next);
    return state_.phase;
}
void Session::commit(Player player, base_rules::WorkAction action) {
    if (player > 1 || state_.phase != Phase::work_decision || state_.commitments[player])
        throw std::invalid_argument("not an open Work commitment");
    if (action != base_rules::WorkAction::mine && action != base_rules::WorkAction::farm &&
        action != base_rules::WorkAction::research)
        throw std::invalid_argument("unsupported Work action");
    state_.commitments[player] = action;
}
base_rules::WorkRecord Session::resolve_work(std::array<Player, 2> schedule) {
    if (state_.phase != Phase::work_decision || !state_.commitments[0] || !state_.commitments[1])
        throw std::invalid_argument("Work requires both commitments");
    auto next = state_;
    base_rules::WorkBatch batch(next.colonies, config_.seed, config_.scope);
    for (Player p = 0; p < 2; ++p) batch.commit(p, *next.commitments[p]);
    auto result = batch.resolve(schedule);
    progression::Deck deck(next.progress, config_.seed);
    deck.finish_active();
    next.colonies = batch.snapshot();
    next.progress = deck.snapshot();
    next.commitments = {};
    next.phase = Phase::ready;
    state_ = std::move(next);
    return result;
}
Observation Session::observe(Player p) const {
    // WorkBatch rejects dead colonies, so observation must redact directly after terminal.
    if (p > 1) throw std::invalid_argument("invalid player");
    std::optional<base_rules::Card> top;
    if (!state_.colonies.building_discard.empty()) top = state_.colonies.building_discard.back();
    base_rules::Observation colony{p,
        {state_.colonies.colonies[0].resources, state_.colonies.colonies[1].resources},
        state_.colonies.colonies[p].hand, top};
    return {colony, progression::Deck(state_.progress, config_.seed).observe(), state_.phase, state_.official};
}
Replay record(Input input, const std::vector<Command>& commands) {
    Session session(input);
    Replay replay{1, manifest_hash, std::move(input), commands, {}};
    for (const auto& command : commands) {
        execute(session, command);
        replay.checkpoints.push_back(session.snapshot());
    }
    return replay;
}
Snapshot verify(const Replay& replay) {
    if (replay.schema != 1 || replay.subset_hash != manifest_hash ||
        replay.commands.size() != replay.checkpoints.size())
        throw std::invalid_argument("unsupported or incomplete session replay");
    Session session(replay.initial);
    for (std::size_t i = 0; i < replay.commands.size(); ++i) {
        execute(session, replay.commands[i]);
        if (session.snapshot() != replay.checkpoints[i]) throw std::invalid_argument("session replay mismatch");
    }
    return session.snapshot();
}
} // namespace bloodbath::session
