#include "engine/session.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace bloodbath::session {
namespace {
std::uint64_t robot_amount(Effect effect) { return effect == Effect::mining_robot ? 6 : 4; }
std::string robot_cause(Effect effect) {
    return effect == Effect::mining_robot ? "Mining Robot" :
        (effect == Effect::robot_butler ? "Robot Butler" : "Robot Waiter");
}
void robot_tail(base_rules::State& state, RobotRecord& result) {
    // Only reviewed Kale Farms can be in play; they are not Mines.
    if (result.effect == Effect::robot_waiter)
        for (Player p = 0; p < 2; ++p) {
            auto& colony = state.colonies[p];
            result.food_loss[p] = std::min(std::uint64_t{2}, colony.resources.food);
            colony.resources.food -= result.food_loss[p];
        }
}
bool robot_effect(Effect effect) {
    return effect == Effect::robot_butler || effect == Effect::robot_waiter || effect == Effect::mining_robot;
}
std::vector<base_rules::Card> box_targets(const base_rules::Colony& colony) {
    std::vector<base_rules::Card> result;
    for (const auto card : colony.buildings) {
        const auto found = colony.boxes.find(card);
        if (found != colony.boxes.end() && found->second > 0) result.push_back(card);
    }
    return result;
}
void complete_robot(Snapshot& next, progression::Deck& deck, RobotRecord result) {
    robot_tail(next.colonies, result); // All players' population payments must precede this call.
    next.loss_commitments = {};
    if (result.effect == Effect::robot_butler) {
        const auto first = box_targets(next.colonies.colonies[0]);
        const auto second = box_targets(next.colonies.colonies[1]);
        if (first.size() > 1 || second.size() > 1) {
            result.completed = false;
            next.robot = std::move(result);
            next.pending_robot = Effect::robot_butler;
            next.phase = Phase::blocked_tableau_effect; // Box target selector is not established by the source.
            return;
        }
        const std::array<std::vector<base_rules::Card>, 2> targets{first, second};
        for (Player p = 0; p < 2; ++p)
            if (!targets[p].empty()) {
                const auto card = targets[p][0];
                --next.colonies.colonies[p].boxes.at(card);
                result.box_loss[p] = card;
            }
    }
    next.robot = std::move(result);
    next.pending_robot.reset();
    next.phase = Phase::ready;
    deck.finish_active();
}
void validate(const Input& input) {
    // Bare context stays strict; only explicit reviewed Tableau rules allow Buildings.
    if (input.config.tableau_enabled)
        tableau::validate(input.colonies, input.config.building_catalog, input.config.scope);
    else {
        if (!input.config.building_catalog.empty()) throw std::invalid_argument("Building catalog needs explicit opt-in");
        (void)base_rules::WorkBatch(input.colonies, input.config.seed, input.config.scope);
    }
    const auto& progress = input.progress;
    (void)progression::Deck(progress, input.config.seed);
    if (progress.active) throw std::invalid_argument("session input must be at a resolved card boundary");
    if (progress.clock.absolute_event || progress.clock.cycle || progress.clock.position ||
        progress.clock.decision || progress.shuffle_index || progress.addition_index)
        throw std::invalid_argument("session starts a new local trace, not an unverified midgame restore");
    for (const auto& colony : input.colonies.colonies)
        if (tableau::total_people(colony, input.config.building_catalog) >
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
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
    case CommandKind::commit_losses: session.commit_losses(command.player, command.cards); break;
    case CommandKind::resolve_accident: (void)session.resolve_accident(command.schedule); break;
    case CommandKind::resolve_hunger: (void)session.resolve_hunger(command.schedule); break;
    case CommandKind::resolve_robot: (void)session.resolve_robot(command.schedule); break;
    case CommandKind::commit_choice: {
        tableau::Choice choice{command.action};
        if (command.action == base_rules::WorkAction::build) {
            if (command.cards.size() != 1) throw std::invalid_argument("BUILD command needs one card");
            choice.build = command.cards[0];
        } else if (command.action == base_rules::WorkAction::restock) choice.box_targets = command.cards;
        else if (!command.cards.empty()) throw std::invalid_argument("unexpected Work command cards");
        session.commit_choice(command.player, choice);
        break;
    }
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
    const Scores scores{static_cast<std::int64_t>(tableau::total_people(next.colonies.colonies[0], config_.building_catalog)),
                        static_cast<std::int64_t>(tableau::total_people(next.colonies.colonies[1], config_.building_catalog))};
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
    const auto revealed_effect = effect_of(config_, reveal->card);
    const bool has_buildings = !next.colonies.colonies[0].buildings.empty() || !next.colonies.colonies[1].buildings.empty();
    switch (revealed_effect) {
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
    case Effect::hunger: {
        next.hunger.reset();
        next.pending_hunger.reset();
        next.hunger_losses.reset();
        HungerRecord result;
        bool needs_choice = false;
        bool terminal_possible = false;
        for (Player p = 0; p < 2; ++p) {
            const auto& colony = next.colonies.colonies[p];
            result.required_food[p] = colony.buildings.size();
            result.spent_food[p] = std::min(colony.resources.food, result.required_food[p]);
            result.population_loss[p] = result.required_food[p] - result.spent_food[p];
            const auto total = tableau::total_people(colony, config_.building_catalog);
            needs_choice = needs_choice || result.population_loss[p] > colony.resources.people;
            terminal_possible = terminal_possible || result.population_loss[p] >= total;
            result.population_gain[p] = result.population_loss[p] == 0 ? 2 : 0;
        }
        if (terminal_possible) { next.phase = Phase::blocked_population_payment; break; }
        for (Player p = 0; p < 2; ++p) {
            const auto limit = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
            if (tableau::total_people(next.colonies.colonies[p], config_.building_catalog) > limit - result.population_gain[p])
                throw std::overflow_error("Hunger population overflow");
        }
        if (needs_choice) {
            next.pending_hunger = result; // Keep original Building count and payment despite later losses.
            deck.record_decision();
            next.phase = Phase::hunger_choices;
            break;
        }
        for (Player p = 0; p < 2; ++p) {
            auto& colony = next.colonies.colonies[p];
            colony.resources.food -= result.spent_food[p];
            colony.resources.people -= result.population_loss[p];
            colony.resources.people += result.population_gain[p];
        }
        next.hunger = result;
        deck.finish_active();
        break;
    }
    case Effect::accident: {
        next.accident.reset();
        if (has_buildings) {
            bool terminal_possible = false;
            for (const auto& colony : next.colonies.colonies)
                terminal_possible = terminal_possible || tableau::total_people(colony, config_.building_catalog) <= 3;
            if (terminal_possible) next.phase = Phase::blocked_population_payment;
            else if (next.colonies.colonies[0].resources.people >= 3 &&
                     next.colonies.colonies[1].resources.people >= 3) {
                AccidentRecord result{};
                for (Player p = 0; p < 2; ++p)
                    result.losses[p] = tableau::lose_people(next.colonies, config_.building_catalog,
                        p, 3, {}, reveal->clock, "Accident", config_.scope);
                next.accident = std::move(result);
                deck.finish_active(); // Deterministic payment has no genuine decision.
            }
            else {
                deck.record_decision();
                next.phase = Phase::accident_choices;
            }
            break;
        }
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
        next.robot.reset();
        next.pending_robot.reset();
        const auto effect = effect_of(config_, reveal->card);
        const auto amount = robot_amount(effect);
        bool possible_failure = false;
        bool needs_choice = false;
        for (const auto& colony : next.colonies.colonies) {
            possible_failure = possible_failure || tableau::total_people(colony, config_.building_catalog) <= amount;
            needs_choice = needs_choice || colony.resources.people < amount;
        }
        if (possible_failure) {
            // Do not settle whether post-loss Robot instructions run after first death.
            next.phase = Phase::blocked_robot_loss;
            break;
        }
        if (needs_choice) {
            next.pending_robot = effect;
            deck.record_decision();
            next.phase = Phase::robot_choices;
            break;
        }
        const auto cause = robot_cause(effect);
        RobotRecord result{};
        result.effect = effect;
        for (Player p = 0; p < 2; ++p)
            result.losses[p] = tableau::lose_people(next.colonies, config_.building_catalog, p,
                amount, {}, reveal->clock, cause, config_.scope);
        complete_robot(next, deck, std::move(result));
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
    state_.work_choices[player] = tableau::Choice{action};
}
void Session::commit_choice(Player player, const tableau::Choice& choice) {
    if (!config_.tableau_enabled || player > 1 || state_.phase != Phase::work_decision || state_.commitments[player])
        throw std::invalid_argument("not an open reviewed-tableau commitment");
    tableau::validate_choice(state_.colonies, config_.building_catalog, player, choice);
    auto next = state_;
    next.commitments[player] = choice.action;
    next.work_choices[player] = choice;
    state_ = std::move(next);
}
std::vector<tableau::Choice> Session::work_options(Player player) const {
    if (player > 1) throw std::invalid_argument("invalid player");
    if (state_.phase != Phase::work_decision || state_.commitments[player]) return {};
    if (config_.tableau_enabled) return tableau::options(state_.colonies, config_.building_catalog, player);
    return {{base_rules::WorkAction::mine}, {base_rules::WorkAction::farm}, {base_rules::WorkAction::research}};
}
base_rules::WorkRecord Session::resolve_work(std::array<Player, 2> schedule) {
    if (state_.phase != Phase::work_decision || !state_.commitments[0] || !state_.commitments[1])
        throw std::invalid_argument("Work requires both commitments");
    auto next = state_;
    base_rules::WorkRecord result{};
    if (config_.tableau_enabled) {
        const std::array<tableau::Choice, 2> choices{*next.work_choices[0], *next.work_choices[1]};
        auto record = tableau::resolve(next.colonies, config_.building_catalog, choices, schedule,
                                       config_.seed, config_.scope);
        for (const auto& colony : next.colonies.colonies)
            if (tableau::total_people(colony, config_.building_catalog) >
                static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
                throw std::overflow_error("Building population exceeds official score range");
        result = record.base;
        next.tableau_work = std::move(record);
    } else {
        base_rules::WorkBatch batch(next.colonies, config_.seed, config_.scope);
        for (Player p = 0; p < 2; ++p) batch.commit(p, *next.commitments[p]);
        result = batch.resolve(schedule);
        next.colonies = batch.snapshot();
    }
    progression::Deck deck(next.progress, config_.seed);
    deck.finish_active();
    next.progress = deck.snapshot();
    next.commitments = {};
    next.work_choices = {};
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
    colony.public_buildings = {state_.colonies.colonies[0].buildings, state_.colonies.colonies[1].buildings};
    colony.public_boxes = {state_.colonies.colonies[0].boxes, state_.colonies.colonies[1].boxes};
    const auto required = state_.phase == Phase::paperwork_discard ?
        std::min(std::size_t{2}, state_.colonies.colonies[p].hand.size()) : 0;
    std::vector<base_rules::Card> loss_options;
    const auto loss_amount = state_.phase == Phase::hunger_choices ? state_.pending_hunger->population_loss[p] :
        (state_.phase == Phase::robot_choices ? robot_amount(*state_.pending_robot) : 3);
    if ((state_.phase == Phase::accident_choices || state_.phase == Phase::hunger_choices ||
         state_.phase == Phase::robot_choices) &&
        !state_.loss_commitments[p] && state_.colonies.colonies[p].resources.people < loss_amount)
        loss_options = state_.colonies.colonies[p].buildings;
    return {colony, progression::Deck(state_.progress, config_.seed).observe(),
            state_.phase, state_.official, required, work_options(p), loss_options};
}
void Session::commit_losses(Player p, std::vector<base_rules::Card> cards) {
    if (p > 1 || (state_.phase != Phase::accident_choices && state_.phase != Phase::hunger_choices &&
                 state_.phase != Phase::robot_choices) || state_.loss_commitments[p])
        throw std::invalid_argument("not an open loss commitment");
    auto trial = state_.colonies;
    const bool hunger = state_.phase == Phase::hunger_choices;
    const bool robot = state_.phase == Phase::robot_choices;
    const auto amount = hunger ? state_.pending_hunger->population_loss[p] :
        (robot ? robot_amount(*state_.pending_robot) : 3);
    (void)tableau::lose_people(trial, config_.building_catalog, p, amount, cards,
        state_.progress.clock, hunger ? "Hunger" : (robot ? robot_cause(*state_.pending_robot) : "Accident"), config_.scope);
    state_.loss_commitments[p] = std::move(cards);
}
AccidentRecord Session::resolve_accident(std::array<Player, 2> schedule) {
    if (state_.phase != Phase::accident_choices || !state_.loss_commitments[0] || !state_.loss_commitments[1])
        throw std::invalid_argument("Accident requires both loss commitments");
    if (!((schedule[0] == 0 && schedule[1] == 1) || (schedule[0] == 1 && schedule[1] == 0)))
        throw std::invalid_argument("loss schedule must be a player permutation");
    auto next = state_;
    AccidentRecord result{};
    result.resolution_order = schedule;
    for (const Player p : schedule)
        result.losses[p] = tableau::lose_people(next.colonies, config_.building_catalog, p, 3,
            *next.loss_commitments[p], next.progress.clock, "Accident", config_.scope);
    progression::Deck deck(next.progress, config_.seed);
    deck.finish_active();
    next.progress = deck.snapshot();
    next.accident = result;
    next.loss_commitments = {};
    next.phase = Phase::ready;
    state_ = std::move(next);
    return result;
}
HungerRecord Session::resolve_hunger(std::array<Player, 2> schedule) {
    if (state_.phase != Phase::hunger_choices || !state_.loss_commitments[0] || !state_.loss_commitments[1])
        throw std::invalid_argument("Hunger requires both loss commitments");
    if (!((schedule[0] == 0 && schedule[1] == 1) || (schedule[0] == 1 && schedule[1] == 0)))
        throw std::invalid_argument("loss schedule must be a player permutation");
    auto next = state_;
    const auto result = *next.pending_hunger;
    AccidentRecord losses{};
    losses.resolution_order = schedule;
    for (const Player p : schedule) {
        losses.losses[p] = tableau::lose_people(next.colonies, config_.building_catalog, p,
            result.population_loss[p], *next.loss_commitments[p], next.progress.clock, "Hunger", config_.scope);
        auto& colony = next.colonies.colonies[p];
        colony.resources.food -= result.spent_food[p];
        colony.resources.people += result.population_gain[p]; // Range checked before opening this frame.
    }
    progression::Deck deck(next.progress, config_.seed);
    deck.finish_active();
    next.progress = deck.snapshot();
    next.hunger = result;
    next.hunger_losses = std::move(losses);
    next.pending_hunger.reset();
    next.loss_commitments = {};
    next.phase = Phase::ready;
    state_ = std::move(next);
    return result;
}
RobotRecord Session::resolve_robot(std::array<Player, 2> schedule) {
    if (state_.phase != Phase::robot_choices || !state_.loss_commitments[0] || !state_.loss_commitments[1])
        throw std::invalid_argument("Robot requires both loss commitments");
    if (!((schedule[0] == 0 && schedule[1] == 1) || (schedule[0] == 1 && schedule[1] == 0)))
        throw std::invalid_argument("loss schedule must be a player permutation");
    auto next = state_;
    RobotRecord result{};
    result.effect = *next.pending_robot;
    result.resolution_order = schedule;
    for (const Player p : schedule)
        result.losses[p] = tableau::lose_people(next.colonies, config_.building_catalog, p,
            robot_amount(result.effect), *next.loss_commitments[p], next.progress.clock,
            robot_cause(result.effect), config_.scope);
    progression::Deck deck(next.progress, config_.seed);
    complete_robot(next, deck, result);
    next.progress = deck.snapshot();
    result = *next.robot;
    state_ = std::move(next);
    return result;
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
        base_rules::BuildingDrawRecord draws{};
        if (config_.tableau_enabled) {
            tableau::validate(next.colonies, config_.building_catalog, config_.scope);
            draws = base_rules::draw_buildings(next.colonies, {1, 1}, schedule, config_.seed,
                                              base_rules::DrawScope::caller_verified_unmodified_draw);
        } else draws = base_rules::draw_buildings(next.colonies, {1, 1}, schedule, config_.seed, config_.scope);
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
    Replay replay{11, manifest_hash, std::move(input), commands, {}};
    for (const auto& command : commands) {
        execute(session, command);
        replay.checkpoints.push_back(session.snapshot());
    }
    return replay;
}
Snapshot verify(const Replay& replay) {
    if (replay.schema != 11 || replay.subset_hash != manifest_hash ||
        replay.commands.size() != replay.checkpoints.size())
        throw std::invalid_argument("unsupported or incomplete session replay");
    Session session(replay.initial);
    for (std::size_t i = 0; i < replay.commands.size(); ++i) {
        execute(session, replay.commands[i]);
        if (session.snapshot() != replay.checkpoints[i]) throw std::invalid_argument("session replay mismatch");
    }
    return session.snapshot();
}
TraceAssessment assess(const Snapshot& snapshot) {
    TraceAssessment result;
    result.has_conditional_outcome = snapshot.official.has_value();
    switch (snapshot.phase) {
    case Phase::ready: return result;
    case Phase::work_decision:
    case Phase::paperwork_draw_order:
    case Phase::paperwork_discard:
    case Phase::accident_choices:
    case Phase::hunger_choices:
    case Phase::robot_choices:
        result.status = TraceStatus::awaiting_input;
        return result;
    case Phase::official_terminal:
        result.status = TraceStatus::conditional_terminal;
        result.terminal_resolved = result.has_conditional_outcome;
        return result;
    case Phase::blocked_unknown_card: result.reason = "unsupported_card_effect"; break;
    case Phase::blocked_event_queue: result.reason = "supplied_event_source_exhausted"; break;
    case Phase::blocked_population_payment: result.reason = "population_terminal_interruption_unverified"; break;
    case Phase::unresolved_tie: result.reason = "terminal_tie_rule_unverified"; break;
    case Phase::no_progress_cards: result.reason = "progress_exhaustion_not_official_end"; break;
    case Phase::blocked_shared_draw: result.reason = "shared_draw_timing_unverified"; break;
    case Phase::blocked_robot_deck: result.reason = "supplied_robot_source_exhausted"; break;
    case Phase::blocked_robot_loss: result.reason = "robot_terminal_interruption_unverified"; break;
    case Phase::blocked_tableau_effect:
        result.reason = snapshot.robot && snapshot.robot->effect == Effect::robot_butler &&
            !snapshot.robot->completed ? "butler_box_selector_unverified" : "unsupported_tableau_effect";
        break;
    default: throw std::invalid_argument("unknown trace phase");
    }
    result.status = TraceStatus::blocked_rule;
    return result;
}
} // namespace bloodbath::session
