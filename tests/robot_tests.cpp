#include "engine/session.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
#include <utility>

namespace base = bloodbath::base_rules;
namespace session = bloodbath::session;
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

// All physical IDs, deck orders and starting positions below are synthetic
// fixtures, not a complete official Robot catalog or legal setup distribution.
std::vector<base::Card> cards(base::Card first, base::Card end) {
    std::vector<base::Card> result;
    for (auto card = first; card < end; ++card) result.push_back(card);
    return result;
}
session::Input fixture() {
    session::Input input;
    input.colonies = base::setup_fragment({cards(0, 4), cards(4, 8)}, cards(8, 90));
    input.colonies.colonies[1].resources.people = 29;
    input.progress.deck = {102, 100};
    input.config.seed = 42;
    input.config.scope = base::EffectScope::caller_verified_unmodified;
    input.config.effects = {{100, Effect::glitch}, {101, Effect::glitch},
        {102, Effect::instruction_manual}, {103, Effect::hunger},
        {200, Effect::robot_butler}, {201, Effect::robot_waiter}, {202, Effect::mining_robot}};
    input.config.ordered_robots = {200, 201, 202};
    return input;
}
session::Input single(Effect effect, std::uint64_t p0 = 30, std::uint64_t p1 = 29) {
    auto input = fixture();
    input.progress.deck = {200};
    input.config.effects[200] = effect;
    input.config.ordered_robots.clear();
    input.colonies.colonies[0].resources.people = p0;
    input.colonies.colonies[1].resources.people = p1;
    return input;
}
std::vector<bloodbath::progression::Card> physical_progress(const session::Input& input,
                                                          const session::Snapshot& state) {
    auto result = state.progress.deck;
    result.insert(result.end(), state.progress.discard.begin(), state.progress.discard.end());
    if (state.progress.active) result.push_back(*state.progress.active);
    result.insert(result.end(), input.config.ordered_events.begin() + state.next_event,
                  input.config.ordered_events.end());
    result.insert(result.end(), input.config.ordered_robots.begin() + state.next_robot,
                  input.config.ordered_robots.end());
    std::sort(result.begin(), result.end());
    require(std::set<bloodbath::progression::Card>(result.begin(), result.end()).size() == result.size(),
            "duplicate physical Progress/Robot instance");
    return result;
}
void conserved(const session::Input& input, const session::Snapshot& state) {
    session::Session initial(input);
    require(physical_progress(input, state) == physical_progress(input, initial.snapshot()),
            "Progress/Robot card identity was lost or fabricated");
    auto building_cards = state.colonies.building_deck;
    building_cards.insert(building_cards.end(), state.colonies.building_discard.begin(),
                          state.colonies.building_discard.end());
    for (const auto& colony : state.colonies.colonies) {
        building_cards.insert(building_cards.end(), colony.hand.begin(), colony.hand.end());
        building_cards.insert(building_cards.end(), colony.buildings.begin(), colony.buildings.end());
    }
    std::sort(building_cards.begin(), building_cards.end());
    require(building_cards == cards(0, 90), "Robot changed the physical Building catalog");
}
void stopped(session::Session& run, Phase phase) {
    const auto saved = run.snapshot();
    for (int i = 0; i < 3; ++i)
        require(run.advance() == phase && run.snapshot() == saved, "blocked Robot processing was skipped");
    rejects([&] { run.commit(0, base::WorkAction::mine); });
    rejects([&] { (void)run.resolve_work({0, 1}); });
}

void glitch_adds_next_once_and_precedes_recycle() {
    const auto input = fixture();
    session::Session run(input);
    require(run.advance() == Phase::ready, "Glitch did not resolve");
    const auto added = run.snapshot();
    require(added.next_robot == 1 && added.progress.deck == std::vector<base::Card>{102, 200},
            "Glitch did not add exactly the next Robot above old future cards");
    require(added.progress.discard == std::vector<base::Card>{100} && !added.progress.active &&
                added.progress.clock == EventClock{1, 0, 1, 0} && added.progress.shuffle_index == 0,
            "Glitch resolved Robot immediately or invented a shuffle/decision");
    require(added.colonies == input.colonies && !added.deaths[0] && !added.deaths[1],
            "Glitch addition applied a Robot loss before its reveal");
    conserved(input, added);
    require(run.advance() == Phase::ready && run.snapshot().progress.discard.back() == 200 &&
                run.snapshot().next_robot == 1, "Robot was not next or consumed another source card");
    require(run.advance() == Phase::official_terminal && run.snapshot().next_robot == 1,
            "existing future card did not follow added Robot");

    auto empty_progress = fixture();
    empty_progress.progress.deck = {100};
    empty_progress.progress.discard = {102};
    session::Session inserted(empty_progress);
    (void)inserted.advance();
    const auto before_robot = inserted.snapshot();
    require(before_robot.progress.deck == std::vector<base::Card>{200} &&
                before_robot.progress.shuffle_index == 0 && before_robot.progress.clock.cycle == 0,
            "inserting into emptied Progress deck recycled old discards");
    require(inserted.advance() == Phase::ready && inserted.snapshot().progress.discard.back() == 200 &&
                inserted.snapshot().progress.shuffle_index == 0,
            "new Robot was swallowed into a premature reshuffle");
    conserved(empty_progress, inserted.snapshot());

    auto repeated = fixture();
    repeated.progress.deck = {102, 101, 100};
    session::Session twice(repeated);
    (void)twice.advance();
    (void)twice.advance();
    require(twice.snapshot().next_robot == 1, "Robot tail consumed source twice");
    (void)twice.advance();
    require(twice.snapshot().next_robot == 2 && twice.snapshot().progress.deck.back() == 201,
            "second Glitch resampled or reused first source Robot");
    (void)twice.advance();
    require(twice.snapshot().next_robot == 2 && twice.snapshot().progress.discard.back() == 201,
            "second supplied Robot did not reveal next");
    conserved(repeated, twice.snapshot());
}

void supported_nonterminal_effects() {
    for (const auto effect : {Effect::robot_butler, Effect::robot_waiter, Effect::mining_robot}) {
        auto input = single(effect);
        input.colonies.colonies[0].resources.money = 7;
        input.colonies.colonies[1].resources.money = 9;
        session::Session run(input);
        require(run.advance() == Phase::ready, "safe Robot failed to finish");
        const auto after = run.snapshot();
        const std::uint64_t amount = effect == Effect::mining_robot ? 6 : 4;
        for (Player p = 0; p < 2; ++p) {
            const auto& before = input.colonies.colonies[p];
            const auto& colony = after.colonies.colonies[p];
            require(colony.resources.people == before.resources.people - amount,
                    "Robot printed population amount wrong");
            require(colony.resources.money == before.resources.money && colony.hand == before.hand &&
                        colony.buildings == before.buildings, "no-Building Robot fabricated bonus/boxes/cards");
            const auto food_loss = effect == Effect::robot_waiter ? std::uint64_t{2} : std::uint64_t{0};
            require(colony.resources.food == before.resources.food - food_loss, "Robot tail food loss wrong");
        }
        require(!after.official && !after.terminal_evidence && !after.deaths[0] && !after.deaths[1] &&
                    !after.progress.active && after.next_robot == 0,
                "safe Robot invented mortality/end or consumed its original source");
        conserved(input, after);
    }
    for (std::uint64_t food = 0; food < 5; ++food) {
        auto input = single(Effect::robot_waiter);
        input.colonies.colonies[0].resources.food = food;
        input.colonies.colonies[1].resources.food = 4 - food;
        session::Session waiter(input);
        (void)waiter.advance();
        for (Player p = 0; p < 2; ++p) {
            const auto available = input.colonies.colonies[p].resources.food;
            require(waiter.snapshot().colonies.colonies[p].resources.food ==
                        available - std::min(std::uint64_t{2}, available),
                    "Waiter partial food loss underflowed or skipped available food");
        }
    }
}

void terminal_and_unknown_boundaries() {
    for (const auto effect : {Effect::robot_butler, Effect::robot_waiter, Effect::mining_robot}) {
        const std::uint64_t amount = effect == Effect::mining_robot ? 6 : 4;
        for (const auto populations : {std::array<std::uint64_t, 2>{amount, 20},
                {20, amount}, {amount - 1, 20}, {20, amount - 1}, {amount, amount}}) {
            auto input = single(effect, populations[0], populations[1]);
            input.colonies.colonies[0].resources.food = 9;
            input.colonies.colonies[1].resources.food = 1;
            session::Session run(input);
            require(run.advance() == Phase::blocked_robot_loss, "possible Robot death was adjudicated");
            const auto blocked = run.snapshot();
            require(blocked.colonies == input.colonies && !blocked.official && !blocked.terminal_evidence &&
                        !blocked.deaths[0] && !blocked.deaths[1] &&
                        blocked.progress.active == bloodbath::progression::Card{200},
                    "Robot interruption gate partly changed population/tail or invented death");
            stopped(run, Phase::blocked_robot_loss);
            conserved(input, blocked);
        }
    }
    auto missing = fixture();
    missing.config.ordered_robots.clear();
    session::Session exhausted(missing);
    require(exhausted.advance() == Phase::blocked_robot_deck, "empty Robot deck invented a card/refill/end");
    require(exhausted.snapshot().next_robot == 0 && exhausted.snapshot().colonies == missing.colonies &&
                exhausted.snapshot().progress.active == bloodbath::progression::Card{100} &&
                !exhausted.snapshot().official, "Robot exhaustion mutated resources/source or finished Glitch");
    stopped(exhausted, Phase::blocked_robot_deck);
    for (const bool explicit_unknown : {false, true}) {
        auto unknown = fixture();
        if (explicit_unknown) unknown.config.effects[200] = Effect::unsupported;
        else unknown.config.effects.erase(200);
        session::Session run(unknown);
        require(run.advance() == Phase::ready && run.snapshot().next_robot == 1,
                "opaque Robot could not move as a physical instance");
        require(run.advance() == Phase::blocked_unknown_card && run.snapshot().colonies == unknown.colonies &&
                    run.snapshot().progress.active == bloodbath::progression::Card{200} &&
                    !run.snapshot().deaths[0] && !run.snapshot().deaths[1],
                "unknown Robot inferred a damage amount or skipped its effects");
        stopped(run, Phase::blocked_unknown_card);
        conserved(unknown, run.snapshot());
    }
}

void privacy_and_validation() {
    const auto a = fixture();
    auto b = a;
    std::reverse(b.config.ordered_robots.begin(), b.config.ordered_robots.end());
    b.config.effects[201] = Effect::robot_butler;
    session::Session left(a), right(b);
    require(left.observe(0) == right.observe(0), "private Robot order/mapping leaked before Glitch");
    (void)left.advance();
    (void)right.advance();
    require(left.observe(0) == right.observe(0) && left.observe(1) == right.observe(1),
            "newly inserted face-down Robot identity leaked before its reveal");
    require(left.snapshot().progress.deck.back() != right.snapshot().progress.deck.back(),
            "privacy fixture failed to change future Robot");

    auto invalid = fixture();
    invalid.config.ordered_robots = {200, 200};
    rejects([&] { (void)session::Session(invalid); });
    invalid = fixture();
    invalid.progress.deck.push_back(200);
    rejects([&] { (void)session::Session(invalid); });
    invalid = fixture();
    invalid.progress.discard = {200};
    rejects([&] { (void)session::Session(invalid); });
    invalid = fixture();
    invalid.config.ordered_events = {900};
    invalid.config.ordered_robots = {900};
    rejects([&] { (void)session::Session(invalid); });
    for (const auto non_robot : {Effect::work, Effect::trouble, Effect::hunger,
            Effect::paperwork, Effect::glitch, Effect::instruction_manual}) {
        invalid = fixture();
        invalid.config.effects[200] = non_robot;
        rejects([&] { (void)session::Session(invalid); });
    }
    invalid = fixture();
    invalid.config.ordered_robots.clear();
    invalid.config.ordered_events = {200};
    rejects([&] { (void)session::Session(invalid); });
    invalid = fixture();
    invalid.config.scope = base::EffectScope::unverified;
    rejects([&] { (void)session::Session(invalid); });
    invalid = fixture();
    invalid.colonies.colonies[0].buildings.push_back(invalid.colonies.colonies[0].hand.back());
    invalid.colonies.colonies[0].hand.pop_back();
    rejects([&] { (void)session::Session(invalid); });
}

void recycling_and_replay() {
    // A Robot already in Progress must recycle there without drawing its source again.
    auto cycling = fixture();
    cycling.progress.deck = {103, 201};
    cycling.config.ordered_robots = {200, 202};
    cycling.colonies.colonies[0].resources.people = 100;
    cycling.colonies.colonies[1].resources.people = 99;
    const std::vector<session::Command> cycles(8);
    for (std::uint64_t seed = 0; seed < 32; ++seed) {
        cycling.config.seed = seed;
        const auto replay = session::record(cycling, cycles);
        require(replay.schema == 11 && replay == session::record(cycling, cycles),
                "Robot schema or recording determinism wrong");
        const auto final = session::verify(replay);
        require(final == replay.checkpoints.back() && final.phase == Phase::ready && final.next_robot == 0 &&
                    final.progress.shuffle_index == 3 && final.progress.clock == EventClock{8, 3, 2, 0},
                "Progress recycle consumed external Robot deck or lost clock state");
        require(final.colonies.colonies[0].resources.people == 92 &&
                    final.colonies.colonies[1].resources.people == 91,
                "recycled Robot/Hunger effects did not each resolve once per cycle");
        conserved(cycling, final);
        auto changed = replay;
        changed.schema = 2;
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.subset_hash = "wrong-Robot-subset";
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.checkpoints[1].colonies.colonies[0].resources.food++;
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.checkpoints.back().next_robot = 1;
        rejects([&] { (void)session::verify(changed); });
    }
    const std::vector<session::Command> commands(3);
    const auto input = fixture();
    const auto recorded = session::record(input, commands);
    require(session::verify(recorded).phase == Phase::official_terminal, "Glitch replay omitted final result");
    auto changed = recorded;
    std::swap(changed.initial.config.ordered_robots[0], changed.initial.config.ordered_robots[1]);
    rejects([&] { (void)session::verify(changed); });
    changed = recorded;
    changed.checkpoints[0].next_robot = 2;
    rejects([&] { (void)session::verify(changed); });
}
} // namespace

int main() {
    try {
        glitch_adds_next_once_and_precedes_recycle();
        supported_nonterminal_effects();
        terminal_and_unknown_boundaries();
        privacy_and_validation();
        recycling_and_replay();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional Glitch/Robot contracts passed with synthetic supplied-state fixtures\n";
}
