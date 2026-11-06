#include "engine/session.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace bloodbath::session {
namespace {
bool robot_effect(Effect effect) {
    return effect == Effect::robot_butler || effect == Effect::robot_waiter || effect == Effect::mining_robot;
}
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
            (effect->second == Effect::work || effect->second == Effect::trouble || robot_effect(effect->second)))
            throw std::invalid_argument("a known starting card is not an Event queue entry");
    }
    for (const auto card : input.config.ordered_robots) {
        if (!used.insert(card).second) throw std::invalid_argument("Robot deck duplicates a progression instance");
        const auto effect = input.config.effects.find(card);
        if (effect != input.config.effects.end() && effect->second != Effect::unsupported &&
            !robot_effect(effect->second))
            throw std::invalid_argument("a known non-Robot effect is not a Robot deck entry");
    }
    for (const auto& [card, effect] : input.config.effects) {
        (void)card;
        if (effect != Effect::work && effect != Effect::trouble && effect != Effect::hunger &&
            effect != Effect::accident && effect != Effect::instruction_manual &&
            effect != Effect::unsupported && effect != Effect::paperwork && effect != Effect::glitch &&
            !robot_effect(effect))
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
    case CommandKind::prepare_paperwork: (void)session.prepare_paperwork(command.schedule); break;
    case CommandKind::commit_discards: session.commit_discards(command.player, command.cards); break;
    case CommandKind::resolve_paperwork: (void)session.resolve_paperwork(command.schedule); break;
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
    case Effect::paperwork:
        next.paperwork.reset();
        next.phase = Phase::paperwork_draw_order;
        break;
    case Effect::glitch:
        if (next.next_robot == config_.ordered_robots.size()) {
            next.phase = Phase::blocked_robot_deck;
        } else {
            deck.add_group({config_.ordered_robots[next.next_robot]});
            ++next.next_robot;
            deck.finish_active();
        }
        break;
    case Effect::robot_butler:
    case Effect::robot_waiter:
    case Effect::mining_robot: {
        const auto effect = effect_of(config_, reveal->card);
        const std::uint64_t amount = effect == Effect::mining_robot ? 6 : 4;
        bool possible_failure = false;
        for (const auto& colony : next.colonies.colonies)
            possible_failure = possible_failure || colony.resources.people <= amount;
        if (possible_failure) {
            // Do not settle whether post-loss Robot instructions run after first death.
            next.phase = Phase::blocked_robot_loss;
            break;
        }
        const std::string cause = effect == Effect::mining_robot ? "Mining Robot" :
            (effect == Effect::robot_butler ? "Robot Butler" : "Robot Waiter");
        for (Player p = 0; p < 2; ++p) {
            population::Loss loss({next.colonies.colonies[p].resources.people, {}, {}}, p,
                                  amount, reveal->clock, cause);
            next.colonies.colonies[p].resources.people = loss.snapshot().base_people;
        }
        // No Buildings => no boxes for Butler, no Mine bonuses for Mining Robot.
        if (effect == Effect::robot_waiter)
            for (auto& colony : next.colonies.colonies)
                colony.resources.food -= std::min(std::uint64_t{2}, colony.resources.food);
        deck.finish_active();
        break;
    }
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
    const auto required = state_.phase == Phase::paperwork_discard ?
        std::min(std::size_t{2}, state_.colonies.colonies[p].hand.size()) : 0;
    return {colony, progression::Deck(state_.progress, config_.seed).observe(),
            state_.phase, state_.official, required};
}
Phase Session::prepare_paperwork(std::array<Player, 2> schedule) {
    if (state_.phase != Phase::paperwork_draw_order)
        throw std::invalid_argument("not awaiting Paperwork draw schedule");
    if (!((schedule[0] == 0 && schedule[1] == 1) || (schedule[0] == 1 && schedule[1] == 0)))
        throw std::invalid_argument("draw schedule must be a player permutation");
    auto next = state_;
    if (next.colonies.building_deck.size() + next.colonies.building_discard.size() < 2) {
        // Do not choose whether current-event discards can supply another player's draw.
        next.phase = Phase::blocked_shared_draw;
    } else {
        const auto draws = base_rules::draw_buildings(next.colonies, {1, 1}, schedule,
                                                      config_.seed, config_.scope);
        next.paperwork = PaperworkRecord{draws, {}, {0, 1}, false};
        progression::Deck deck(next.progress, config_.seed);
        deck.record_decision(); // One simultaneous discard decision, not schedule selection.
        next.progress = deck.snapshot();
        next.phase = Phase::paperwork_discard;
    }
    state_ = std::move(next);
    return state_.phase;
}
void Session::commit_discards(Player p, std::vector<base_rules::Card> cards) {
    if (p > 1 || state_.phase != Phase::paperwork_discard || state_.discard_commitments[p])
        throw std::invalid_argument("not an open Paperwork discard commitment");
    const auto& hand = state_.colonies.colonies[p].hand;
    if (cards.size() != std::min(std::size_t{2}, hand.size()))
        throw std::invalid_argument("Paperwork must discard as many as possible up to two");
    std::set<base_rules::Card> selected;
    for (const auto card : cards)
        if (!selected.insert(card).second || std::find(hand.begin(), hand.end(), card) == hand.end())
            throw std::invalid_argument("discard must select distinct owned hand cards");
    state_.discard_commitments[p] = std::move(cards);
}
PaperworkRecord Session::resolve_paperwork(std::array<Player, 2> schedule) {
    if (state_.phase != Phase::paperwork_discard ||
        !state_.discard_commitments[0] || !state_.discard_commitments[1])
        throw std::invalid_argument("Paperwork requires both discard commitments");
    if (!((schedule[0] == 0 && schedule[1] == 1) || (schedule[0] == 1 && schedule[1] == 0)))
        throw std::invalid_argument("discard schedule must be a player permutation");
    auto next = state_;
    auto record = *next.paperwork;
    record.discard_order = schedule;
    for (const Player p : schedule)
        for (const auto card : *next.discard_commitments[p]) {
            auto& hand = next.colonies.colonies[p].hand;
            hand.erase(std::find(hand.begin(), hand.end(), card));
            next.colonies.building_discard.push_back(card);
            record.discarded[p].push_back(card);
        }
    record.completed = true;
    progression::Deck deck(next.progress, config_.seed);
    deck.finish_active();
    next.progress = deck.snapshot();
    next.paperwork = record;
    next.discard_commitments = {};
    next.phase = Phase::ready;
    state_ = std::move(next);
    return record;
}
Replay record(Input input, const std::vector<Command>& commands) {
    Session session(input);
    Replay replay{3, manifest_hash, std::move(input), commands, {}};
    for (const auto& command : commands) {
        execute(session, command);
        replay.checkpoints.push_back(session.snapshot());
    }
    return replay;
}
Snapshot verify(const Replay& replay) {
    if (replay.schema != 3 || replay.subset_hash != manifest_hash ||
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
