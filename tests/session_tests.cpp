#include "engine/session.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace session = bloodbath::session;
namespace base = bloodbath::base_rules;
using bloodbath::EventClock;
using bloodbath::Player;
using session::Effect;
using session::Phase;

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class Exception = std::invalid_argument, class F> void rejects(F function) {
    try { function(); } catch (const Exception&) { return; }
    throw std::runtime_error("expected rejection");
}

// Synthetic physical IDs, supplied populations and Progress sequence are fixtures,
// not an official catalog, legal initial Progress deck or sampled opening.
std::vector<base::Card> cards(base::Card first, base::Card end) {
    std::vector<base::Card> result;
    for (auto card = first; card < end; ++card) result.push_back(card);
    return result;
}
session::Input fixture() {
    session::Input input;
    input.colonies = base::setup_fragment({cards(0, 4), cards(4, 8)}, cards(8, 90));
    input.colonies.colonies[1].resources.people = 29;
    input.progress.deck = {104, 103, 102, 101}; // Back is top.
    input.config.effects = {{101, Effect::work}, {102, Effect::trouble},
                            {103, Effect::accident}, {104, Effect::instruction_manual},
                            {105, Effect::hunger}};
    input.config.ordered_events = {105};
    input.config.scope = base::EffectScope::caller_verified_unmodified;
    input.config.seed = 42;
    return input;
}
session::Input single(Effect effect, std::uint64_t p0 = 30, std::uint64_t p1 = 29) {
    auto input = fixture();
    input.progress.deck = {101};
    input.config.effects = {{101, effect}};
    input.config.ordered_events.clear();
    input.colonies.colonies[0].resources.people = p0;
    input.colonies.colonies[1].resources.people = p1;
    return input;
}
void stopped_without_mutation(session::Session& run, Phase expected) {
    const auto snapshot = run.snapshot();
    for (int i = 0; i < 3; ++i)
        require(run.advance() == expected && run.snapshot() == snapshot,
                "pending, blocked or terminal phase advanced");
}
void end_to_end_and_work_barrier() {
    const auto initial = fixture();
    session::Session run(initial);
    rejects([&] { run.commit(0, base::WorkAction::mine); });
    require(run.advance() == Phase::work_decision, "Work did not create a decision boundary");
    const auto pending = run.snapshot();
    require(pending.progress.active == bloodbath::progression::Card{101} &&
            pending.progress.clock == EventClock{1, 0, 1, 1}, "Work clock or active card wrong");
    require(pending.colonies == initial.colonies, "Work reveal applied an action");
    stopped_without_mutation(run, Phase::work_decision);
    const auto before = run.observe(1);
    run.commit(0, base::WorkAction::research);
    require(run.observe(1) == before, "first commitment leaked to opponent");
    require(run.snapshot().colonies == initial.colonies, "first commitment drew private cards");
    rejects([&] { (void)run.resolve_work({0, 1}); });
    rejects([&] { run.commit(0, base::WorkAction::mine); });
    rejects([&] { run.commit(2, base::WorkAction::farm); });
    rejects([&] { run.commit(1, base::WorkAction::build); });
    rejects([&] { run.commit(1, base::WorkAction::restock); });
    rejects([&] { run.commit(1, static_cast<base::WorkAction>(255)); });
    run.commit(1, base::WorkAction::farm);
    const auto locked = run.snapshot();
    rejects([&] { (void)run.resolve_work({1, 1}); });
    require(run.snapshot() == locked, "invalid resolution partially changed the session");
    const auto work = run.resolve_work({1, 0});
    const auto after_work = run.snapshot();
    require(work.drawn[0] == std::vector<base::Card>{89, 88} && work.drawn[1].empty(),
            "Research was not integrated with Building draws");
    require(after_work.colonies.colonies[1].resources.food == 8 &&
            after_work.colonies.colonies[0].hand.size() == 6,
            "Work resource/hand effects wrong");
    require(after_work.phase == Phase::ready && !after_work.progress.active &&
            !after_work.commitments[0] && !after_work.commitments[1], "Work did not close cleanly");
    require(after_work.progress.clock == EventClock{1, 0, 1, 1}, "Work resolution advanced reveal clock");
    rejects([&] { (void)run.resolve_work({0, 1}); });

    session::Session reverse(initial);
    (void)reverse.advance();
    reverse.commit(1, base::WorkAction::farm);
    reverse.commit(0, base::WorkAction::research);
    require(reverse.resolve_work({1, 0}) == work && reverse.snapshot() == after_work,
            "commit submission order changed Work result");

    require(run.advance() == Phase::ready, "Trouble did not resolve");
    const auto troubled = run.snapshot();
    require(troubled.next_event == 1 && troubled.progress.deck.back() == 105 &&
            troubled.progress.shuffle_index == 0, "Trouble did not add exactly one next Event on top");
    require(troubled.colonies == after_work.colonies, "Trouble altered colony resources");
    require(run.advance() == Phase::ready, "no-Building Hunger did not resolve");
    const auto fed = run.snapshot();
    require(fed.colonies.colonies[0].resources.people == 32 &&
            fed.colonies.colonies[1].resources.people == 31,
            "Hunger did not add two base people to both");
    require(fed.colonies.colonies[0].resources.food == 4 &&
            fed.colonies.colonies[1].resources.food == 8, "no-Building Hunger charged food");
    require(fed.progress.discard.back() == 105 && fed.next_event == 1,
            "added Event did not occur next or was consumed twice");
    require(run.advance() == Phase::ready, "safe Accident did not resolve");
    require(run.snapshot().colonies.colonies[0].resources.people == 29 &&
            run.snapshot().colonies.colonies[1].resources.people == 28,
            "Accident did not remove three from both");
    require(run.advance() == Phase::official_terminal, "Instruction Manual did not terminate");
    const auto terminal = run.snapshot();
    require(terminal.official && terminal.official->winner == bloodbath::Winner::player0 &&
            terminal.official->scores == bloodbath::Scores{29, 28} &&
            terminal.official->cause == "Instruction Manual" &&
            terminal.official->terminal == EventClock{5, 0, 5, 1}, "conditional terminal record wrong");
    require(!terminal.deaths[0] && !terminal.deaths[1] && terminal.terminal_evidence,
            "natural end fabricated deaths or omitted evidence");
    stopped_without_mutation(run, Phase::official_terminal);
    rejects([&] { run.commit(0, base::WorkAction::mine); });
    require(run.snapshot() == terminal, "terminal mutation overwrote frozen record");
}
void mortality_ties_and_unpayable() {
    session::Session one(single(Effect::accident, 3, 7));
    require(one.advance() == Phase::official_terminal, "exact-zero Accident did not terminate");
    const auto dead = one.snapshot();
    require(dead.official && dead.official->winner == bloodbath::Winner::player1 &&
            dead.official->scores == bloodbath::Scores{0, 4}, "exact-zero outcome wrong");
    require(dead.deaths[0] && !dead.deaths[1] && dead.deaths[0]->player == 0 &&
            dead.deaths[0]->clock == EventClock{1, 0, 1, 0} &&
            dead.deaths[0]->cause == "Accident" && dead.deaths[0]->deficit == 0,
            "exact-zero first-death context wrong");
    require(one.observe(0).colony.resources[0].people == 0, "dead-colony observation rejected");
    stopped_without_mutation(one, Phase::official_terminal);

    session::Session both(single(Effect::accident, 3, 3));
    require(both.advance() == Phase::unresolved_tie, "simultaneous equal failure became an invented draw");
    const auto tied = both.snapshot();
    require(!tied.official && tied.terminal_evidence &&
            tied.terminal_evidence->populations == bloodbath::Scores{0, 0} &&
            tied.deaths[0] && tied.deaths[1] &&
            tied.deaths[0]->clock == tied.deaths[1]->clock, "resolved simultaneous loss evidence wrong");
    stopped_without_mutation(both, Phase::unresolved_tie);

    session::Session natural_tie(single(Effect::instruction_manual, 7, 7));
    require(natural_tie.advance() == Phase::unresolved_tie && !natural_tie.snapshot().official,
            "Instruction Manual tie became an official draw");
    require(!natural_tie.snapshot().deaths[0] && !natural_tie.snapshot().deaths[1],
            "Instruction Manual tie fabricated deaths");

    for (const auto populations : {std::array<std::uint64_t, 2>{2, 8}, {8, 2}, {2, 2}}) {
        const auto input = single(Effect::accident, populations[0], populations[1]);
        session::Session blocked(input);
        require(blocked.advance() == Phase::blocked_population_payment, "unpayable Accident was resolved");
        const auto state = blocked.snapshot();
        require(state.colonies == input.colonies && state.progress.active == bloodbath::progression::Card{101} &&
                !state.official && !state.terminal_evidence && !state.deaths[0] && !state.deaths[1],
                "blocked payment changed a colony or fabricated terminal/death evidence");
        stopped_without_mutation(blocked, Phase::blocked_population_payment);
    }
}
void unknowns_queues_and_privacy() {
    auto missing = single(Effect::work);
    missing.config.effects.clear();
    session::Session absent(missing);
    require(absent.advance() == Phase::blocked_unknown_card &&
            absent.snapshot().progress.active == bloodbath::progression::Card{101},
            "missing effect was finished as no-op");
    stopped_without_mutation(absent, Phase::blocked_unknown_card);
    session::Session unsupported(single(Effect::unsupported));
    require(unsupported.advance() == Phase::blocked_unknown_card, "explicit unsupported effect passed");
    session::Session exhausted(single(Effect::trouble));
    require(exhausted.advance() == Phase::blocked_event_queue && exhausted.snapshot().next_event == 0 &&
            exhausted.snapshot().progress.active == bloodbath::progression::Card{101},
            "empty Event queue invented an Event or terminal result");
    stopped_without_mutation(exhausted, Phase::blocked_event_queue);

    auto unsupported_event = single(Effect::trouble);
    unsupported_event.config.ordered_events = {102};
    session::Session added(unsupported_event);
    require(added.advance() == Phase::ready && added.snapshot().next_event == 1,
            "Trouble did not add unknown Event as a physical card");
    require(added.advance() == Phase::blocked_unknown_card &&
            added.snapshot().progress.active == bloodbath::progression::Card{102},
            "added unknown Event was silently skipped");
    auto empty = single(Effect::work);
    empty.progress.deck.clear();
    session::Session no_cards(empty);
    require(no_cards.advance() == Phase::no_progress_cards && !no_cards.snapshot().official,
            "no Progress cards became official game end");
    stopped_without_mutation(no_cards, Phase::no_progress_cards);

    auto a = fixture();
    a.colonies.building_deck = cards(8, 87);
    a.colonies.building_discard = {87, 88, 89};
    a.progress.discard = {700, 701, 702};
    auto b = a;
    std::swap(b.colonies.colonies[1].hand[0], b.colonies.building_deck[0]);
    std::reverse(b.colonies.building_deck.begin(), b.colonies.building_deck.end());
    std::swap(b.colonies.building_discard[0], b.colonies.building_discard[1]);
    std::swap(b.progress.deck[0], b.progress.deck[1]);
    std::swap(b.progress.discard[0], b.progress.discard[1]);
    session::Session left(a), right(b);
    require(left.observe(0) == right.observe(0), "hidden state leaked in ready observation");
    require(left.observe(1) != right.observe(1), "changed own hand was invisible");
    (void)left.advance();
    (void)right.advance();
    require(left.observe(0) == right.observe(0), "hidden state leaked at Work");
    const auto before = left.observe(0);
    left.commit(1, base::WorkAction::research);
    right.commit(1, base::WorkAction::mine);
    require(left.observe(0) == before && left.observe(0) == right.observe(0),
            "other player's pending commitment leaked");
    rejects([&] { (void)left.observe(2); });
}
void validation_and_overflow() {
    auto input = fixture();
    input.config.scope = base::EffectScope::unverified;
    rejects([&] { (void)session::Session(input); });
    input = fixture();
    input.colonies.colonies[0].buildings.push_back(input.colonies.colonies[0].hand.back());
    input.colonies.colonies[0].hand.pop_back();
    rejects([&] { (void)session::Session(input); });
    input = fixture();
    input.progress.active = 900;
    rejects([&] { (void)session::Session(input); });
    input = fixture();
    input.progress.clock.absolute_event = 1;
    rejects([&] { (void)session::Session(input); });
    input = fixture();
    input.config.ordered_events = {105, 105};
    rejects([&] { (void)session::Session(input); });
    input = fixture();
    input.config.ordered_events = {101};
    rejects([&] { (void)session::Session(input); });
    input = fixture();
    input.config.effects[105] = Effect::work;
    rejects([&] { (void)session::Session(input); });
    input.config.effects[105] = Effect::trouble;
    rejects([&] { (void)session::Session(input); });
    input = fixture();
    input.config.effects[101] = static_cast<Effect>(255);
    rejects([&] { (void)session::Session(input); });
    input = fixture();
    input.colonies.colonies[0].resources.people = 0;
    rejects([&] { (void)session::Session(input); });
    input = fixture();
    input.colonies.colonies[0].resources.people = std::numeric_limits<std::uint64_t>::max();
    rejects<std::overflow_error>([&] { (void)session::Session(input); });

    input = single(Effect::hunger, 30,
                   static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));
    session::Session hunger(input);
    const auto before = hunger.snapshot();
    rejects<std::overflow_error>([&] { (void)hunger.advance(); });
    require(hunger.snapshot() == before, "second-player Hunger overflow partly committed first player/reveal");
    input = single(Effect::work);
    input.colonies.colonies[1].resources.food = std::numeric_limits<std::uint64_t>::max();
    session::Session work(input);
    (void)work.advance();
    work.commit(0, base::WorkAction::research);
    work.commit(1, base::WorkAction::farm);
    const auto locked = work.snapshot();
    rejects<std::overflow_error>([&] { (void)work.resolve_work({0, 1}); });
    require(work.snapshot() == locked, "failed Work committed draws or finished active card");
}
session::Command advance() { return {}; }
session::Command commit(Player p, base::WorkAction action) {
    return {session::CommandKind::commit, p, action, {0, 1}};
}
session::Command resolve() { return {session::CommandKind::resolve_work, 0, base::WorkAction::mine, {1, 0}}; }
void trace_replay() {
    auto input = single(Effect::work);
    input.colonies.colonies[0].hand = cards(0, 4);
    input.colonies.colonies[1].hand = cards(4, 85);
    input.colonies.building_deck = {85, 86};
    input.colonies.building_discard = {87, 88, 89};
    std::vector<session::Command> commands;
    for (int i = 0; i < 4; ++i) {
        commands.push_back(advance());
        commands.push_back(commit(1, base::WorkAction::mine));
        commands.push_back(commit(0, base::WorkAction::research));
        commands.push_back(resolve());
    }
    for (std::uint64_t seed = 0; seed < 32; ++seed) {
        input.config.seed = seed;
        const auto recorded = session::record(input, commands);
        require(recorded == session::record(input, commands), "session recording not deterministic");
        const auto final = session::verify(recorded);
        require(final == recorded.checkpoints.back() && final.phase == Phase::ready,
                "session replay did not reproduce final state");
        require(final.progress.clock == EventClock{4, 3, 1, 4} &&
                final.colonies.building_shuffle_index == 1 &&
                final.colonies.colonies[0].hand.size() == 9,
                "replay omitted recycle, partial draw or independent clock state");
        auto changed = recorded;
        changed.checkpoints[0].progress.clock.decision++;
        rejects([&] { (void)session::verify(changed); });
        changed = recorded;
        changed.commands[2].action = base::WorkAction::farm;
        rejects([&] { (void)session::verify(changed); });
        changed = recorded;
        changed.schema = 99;
        rejects([&] { (void)session::verify(changed); });
        changed = recorded;
        changed.subset_hash = "wrong-subset";
        rejects([&] { (void)session::verify(changed); });
        changed = recorded;
        changed.checkpoints.pop_back();
        rejects([&] { (void)session::verify(changed); });
    }
    const auto no_commands = session::record(fixture(), {});
    require(session::verify(no_commands).phase == Phase::ready, "empty trace did not preserve initial state");
    auto invalid = advance();
    invalid.kind = static_cast<session::CommandKind>(255);
    rejects([&] { (void)session::record(fixture(), {invalid}); });
}
} // namespace

int main() {
    try {
        end_to_end_and_work_barrier();
        mortality_ties_and_unpayable();
        unknowns_queues_and_privacy();
        validation_and_overflow();
        trace_replay();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional session contracts passed with synthetic supplied-scenario fixtures\n";
}
