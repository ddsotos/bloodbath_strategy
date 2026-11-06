#include "engine/base_work.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

namespace rules = bloodbath::base_rules;
using bloodbath::Player;
using rules::EffectScope;
using rules::WorkAction;

namespace {
constexpr auto scope = EffectScope::caller_verified_unmodified;

void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class Exception = std::invalid_argument, class F> void rejects(F function) {
    try { function(); } catch (const Exception&) { return; }
    throw std::runtime_error("expected rejection");
}

// Synthetic physical IDs are fixture tokens, not official card definitions.
std::vector<rules::Card> cards(rules::Card first, rules::Card end) {
    std::vector<rules::Card> result;
    for (auto card = first; card < end; ++card) result.push_back(card);
    return result;
}
rules::State initial_fragment() {
    return rules::setup_fragment({cards(0, 4), cards(4, 8)}, cards(8, 90));
}
rules::State scarce_fixture(rules::Card deck_count, rules::Card discard_count) {
    rules::State state;
    state.colonies[0].hand = cards(0, 4);
    const auto hand_end = 90 - deck_count - discard_count;
    state.colonies[1].hand = cards(4, hand_end);
    state.building_deck = cards(hand_end, hand_end + deck_count);
    state.building_discard = cards(hand_end + deck_count, 90);
    return state;
}
std::vector<rules::Card> all_cards(const rules::State& state) {
    std::vector<rules::Card> result;
    for (const auto& colony : state.colonies) {
        result.insert(result.end(), colony.hand.begin(), colony.hand.end());
        result.insert(result.end(), colony.buildings.begin(), colony.buildings.end());
    }
    result.insert(result.end(), state.building_deck.begin(), state.building_deck.end());
    result.insert(result.end(), state.building_discard.begin(), state.building_discard.end());
    std::sort(result.begin(), result.end());
    return result;
}
void conserved(const rules::State& before, const rules::State& after) {
    const auto originals = all_cards(before);
    require(originals.size() == 90, "fixture does not contain ninety cards");
    require(std::set<rules::Card>(originals.begin(), originals.end()).size() == 90,
            "fixture contains duplicate physical IDs");
    require(originals == all_cards(after), "physical cards lost, duplicated, or changed");
}

void setup_and_scope() {
    const auto state = initial_fragment();
    require(rules::starting_resources() == rules::Resources{30, 4, 4}, "incorrect base resources");
    for (const auto& colony : state.colonies) {
        require(colony.resources == rules::Resources{30, 4, 4}, "setup resources differ");
        require(colony.hand.size() == 4 && colony.buildings.empty(), "incorrect setup fragment");
    }
    require(state.building_deck.size() == 82 && state.building_discard.empty(), "wrong remainder");
    rejects([] { (void)rules::setup_fragment({cards(0, 3), cards(4, 8)}, cards(8, 90)); });
    rejects([] { (void)rules::setup_fragment({cards(0, 4), cards(4, 8)}, cards(8, 89)); });
    rejects([] { (void)rules::setup_fragment({cards(0, 4), cards(3, 7)}, cards(8, 90)); });
    rejects([&] { (void)rules::WorkBatch(state, 1, EffectScope::unverified); });
    auto built = state;
    built.colonies[0].buildings.push_back(built.colonies[0].hand.back());
    built.colonies[0].hand.pop_back();
    rejects([&] { (void)rules::WorkBatch(built, 1, scope); });
    auto dead = state;
    dead.colonies[1].resources.people = 0;
    rejects([&] { (void)rules::WorkBatch(dead, 1, scope); });
    auto duplicate = state;
    duplicate.building_deck[0] = duplicate.colonies[0].hand[0];
    rejects([&] { (void)rules::WorkBatch(duplicate, 1, scope); });
}

void observation_boundary() {
    const auto a = scarce_fixture(5, 3);
    auto b = a;
    std::swap(b.colonies[1].hand[0], b.building_deck[0]);
    std::reverse(b.building_deck.begin(), b.building_deck.end());
    std::swap(b.building_discard[0], b.building_discard[1]);
    rules::WorkBatch left(a, 7, scope), right(b, 7, scope);
    const auto observation = left.observe(0);
    require(observation == right.observe(0), "opponent hand, deck, or buried discard leaked");
    require(observation.own_hand == a.colonies[0].hand, "own private cards omitted");
    require(observation.top_building_discard == a.building_discard.back(), "top discard missing");
    require(left.observe(1) != right.observe(1), "owner cannot observe changed hand");
    auto public_changed = a;
    ++public_changed.colonies[1].resources.money;
    rules::WorkBatch public_batch(public_changed, 7, scope);
    require(observation != public_batch.observe(0), "public resources absent");
    require(observation.supported_actions ==
                std::array{WorkAction::mine, WorkAction::farm, WorkAction::research},
            "subset action boundary changed");
    rejects([&] { (void)left.observe(2); });
}

void commitments_and_gains() {
    const auto initial = initial_fragment();
    rules::WorkBatch batch(initial, 7, scope);
    const auto before = batch.observe(1);
    rejects([&] { (void)batch.resolve({0, 1}); });
    rejects([&] { batch.commit(0, WorkAction::build); });
    rejects([&] { batch.commit(0, WorkAction::restock); });
    rejects([&] { batch.commit(0, static_cast<WorkAction>(255)); });
    rejects([&] { batch.commit(2, WorkAction::mine); });
    batch.commit(0, WorkAction::mine);
    require(batch.observe(1) == before, "pending choice leaked through observation");
    require(batch.snapshot() == initial, "commit applied an action before both locked");
    rejects([&] { batch.commit(0, WorkAction::farm); });
    rejects([&] { (void)batch.resolve({0, 1}); });
    batch.commit(1, WorkAction::farm);
    require(batch.snapshot() == initial, "second commitment mutated state before resolve");
    rejects([&] { (void)batch.resolve({0, 0}); });
    rejects([&] { (void)batch.resolve({0, 2}); });
    const auto record = batch.resolve({1, 0});
    const auto after = batch.snapshot();
    require(after.colonies[0].resources == rules::Resources{30, 8, 4}, "Mine base effect wrong");
    require(after.colonies[1].resources == rules::Resources{30, 4, 8}, "Farm base effect wrong");
    require(record.commitments == std::array{WorkAction::mine, WorkAction::farm}, "actions not logged");
    require(record.resolution_order == std::array<Player, 2>{1, 0}, "schedule not logged");
    require(record.seed == 7 && record.subset_hash == BLOODBATH_BASE_RULES_HASH, "provenance missing");
    require(record.drawn[0].empty() && record.drawn[1].empty(), "resource action drew cards");
    require(record.shuffles_before == 0 && record.shuffles_after == 0, "unneeded shuffle");
    conserved(initial, after);
    rejects([&] { (void)batch.resolve({0, 1}); });
    rejects([&] { batch.commit(0, WorkAction::mine); });

    rules::WorkBatch reversed(initial, 7, scope);
    reversed.commit(1, WorkAction::farm);
    reversed.commit(0, WorkAction::mine);
    require(reversed.resolve({1, 0}) == record && reversed.snapshot() == after,
            "commit submission order changed result");
    rules::WorkBatch repeat(after, 7, scope);
    repeat.commit(0, WorkAction::mine);
    repeat.commit(1, WorkAction::farm);
    (void)repeat.resolve({0, 1});
    require(repeat.snapshot().colonies[0].resources.money == 12 &&
            repeat.snapshot().colonies[1].resources.food == 12, "repeated Work action rejected");
}

void research_and_shared_schedule() {
    const auto initial = initial_fragment();
    rules::WorkBatch batch(initial, 8, scope);
    batch.commit(0, WorkAction::research);
    batch.commit(1, WorkAction::mine);
    const auto record = batch.resolve({1, 0});
    require(record.drawn[0] == std::vector<rules::Card>{89, 88}, "Research did not draw top two");
    require(record.drawn[1].empty(), "non-Research player drew");
    const auto result = batch.snapshot();
    require(result.colonies[0].hand == std::vector<rules::Card>{0, 1, 2, 3, 89, 88},
            "drawn cards not added to hand");
    require(result.building_deck.size() == 80, "incorrect Research draw count");
    conserved(initial, result);
    require(rules::replay_work(initial, scope, record) == result, "Work replay diverged");
    auto tampered = record;
    tampered.subset_hash = "unverified-subset";
    rejects([&] { (void)rules::replay_work(initial, scope, tampered); });
    tampered = record;
    tampered.drawn[0][0] = 0;
    rejects([&] { (void)rules::replay_work(initial, scope, tampered); });
    tampered = record;
    ++tampered.shuffles_after;
    rejects([&] { (void)rules::replay_work(initial, scope, tampered); });
    tampered = record;
    tampered.resolution_order = {1, 1};
    rejects([&] { (void)rules::replay_work(initial, scope, tampered); });
    rejects([&] { (void)rules::replay_work(initial, EffectScope::unverified, record); });

    const auto scarce = scarce_fixture(3, 0);
    rules::WorkBatch forward(scarce, 8, scope), reverse(scarce, 8, scope);
    for (Player p = 0; p < 2; ++p) {
        forward.commit(p, WorkAction::research);
        reverse.commit(p, WorkAction::research);
    }
    const auto first = forward.resolve({0, 1});
    const auto second = reverse.resolve({1, 0});
    require(first.drawn[0] == std::vector<rules::Card>{89, 88} &&
            first.drawn[1] == std::vector<rules::Card>{87}, "forward explicit allocation wrong");
    require(second.drawn[1] == std::vector<rules::Card>{89, 88} &&
            second.drawn[0] == std::vector<rules::Card>{87}, "reverse explicit allocation wrong");
    require(first.resolution_order != second.resolution_order, "allocation convention unrecorded");
    conserved(scarce, forward.snapshot());
    conserved(scarce, reverse.snapshot());

    const auto exhausted = scarce_fixture(0, 0);
    rules::WorkBatch empty(exhausted, 8, scope);
    empty.commit(0, WorkAction::research);
    empty.commit(1, WorkAction::research);
    const auto unavailable = empty.resolve({0, 1});
    require(unavailable.drawn[0].empty() && unavailable.drawn[1].empty(), "fabricated unavailable cards");
    require(empty.snapshot() == exhausted, "empty Research changed card state");
}

void deferred_recycle_and_determinism() {
    const auto initial = scarce_fixture(2, 3);
    rules::WorkBatch first(initial, 100, scope);
    first.commit(0, WorkAction::research);
    first.commit(1, WorkAction::farm);
    const auto record = first.resolve({0, 1});
    const auto emptied = first.snapshot();
    require(emptied.building_deck.empty() && emptied.building_discard.size() == 3,
            "discard recycled immediately on deck exhaustion");
    require(record.shuffles_after == 0, "early reshuffle recorded");
    for (std::uint64_t seed = 0; seed < 32; ++seed) {
        rules::WorkBatch a(emptied, seed, scope), b(emptied, seed, scope);
        for (auto* batch : {&a, &b}) {
            batch->commit(1, WorkAction::mine);
            batch->commit(0, WorkAction::research);
        }
        const auto ra = a.resolve({1, 0});
        const auto rb = b.resolve({1, 0});
        const auto after = a.snapshot();
        require(ra == rb && after == b.snapshot(), "recycle not seed reproducible");
        require(rules::replay_work(emptied, scope, ra) == after, "recycled Work replay diverged");
        require(ra.shuffles_before == 0 && ra.shuffles_after == 1, "needed shuffle not counted once");
        require(after.building_discard.empty() && after.building_deck.size() == 1,
                "recycle draw counts wrong");
        require(ra.drawn[0].size() == 2, "recycle did not supply two cards");
        conserved(emptied, after);
    }
    const auto one_and_discard = scarce_fixture(1, 3);
    rules::WorkBatch crossing(one_and_discard, 23, scope);
    crossing.commit(0, WorkAction::research);
    crossing.commit(1, WorkAction::mine);
    const auto cross = crossing.resolve({0, 1});
    require(cross.drawn[0].size() == 2 && cross.drawn[0][0] == one_and_discard.building_deck.back(),
            "mid-Research recycle changed first top card");
    require(cross.shuffles_after == 1 && crossing.snapshot().building_deck.size() == 2,
            "mid-Research shuffle/draw wrong");
    conserved(one_and_discard, crossing.snapshot());
}

void atomic_overflow() {
    auto state = initial_fragment();
    state.colonies[0].resources.food = std::numeric_limits<std::uint64_t>::max();
    rules::WorkBatch batch(state, 4, scope);
    batch.commit(0, WorkAction::farm);
    batch.commit(1, WorkAction::mine);
    rejects<std::overflow_error>([&] { (void)batch.resolve({1, 0}); });
    require(batch.snapshot() == state, "overflow partially applied earlier action");
    rejects<std::overflow_error>([&] { (void)batch.resolve({0, 1}); });
    require(batch.snapshot() == state, "retry after overflow mutated state");
    auto shuffle_overflow = scarce_fixture(0, 2);
    shuffle_overflow.building_shuffle_index = std::numeric_limits<std::uint64_t>::max();
    rules::WorkBatch shuffle(shuffle_overflow, 4, scope);
    shuffle.commit(0, WorkAction::research);
    shuffle.commit(1, WorkAction::mine);
    rejects<std::overflow_error>([&] { (void)shuffle.resolve({1, 0}); });
    require(shuffle.snapshot() == shuffle_overflow, "shuffle overflow partly mutated state");
}
} // namespace

int main() {
    try {
        setup_and_scope();
        observation_boundary();
        commitments_and_gains();
        research_and_shared_schedule();
        deferred_recycle_and_determinism();
        atomic_overflow();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "base Work subset contracts passed with synthetic card-ID fixtures\n";
}
