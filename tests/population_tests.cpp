#include "engine/population.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace pop = bloodbath::population;
using bloodbath::EventClock;

namespace {
constexpr auto no_lost = pop::LostEffects::caller_verified_none;
constexpr EventClock clock_at_loss{17, 2, 3, 11};

void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class Exception = std::invalid_argument, class F> void rejects(F function) {
    try { function(); } catch (const Exception&) { return; }
    throw std::runtime_error("expected rejection");
}

// Every ID, printed population, box count and requested loss below is a synthetic
// fixture input, not a definition of an official Building or card effect.
std::vector<pop::Card> physical_cards(const pop::Colony& colony) {
    auto result = colony.discarded_buildings;
    for (const auto& building : colony.buildings) result.push_back(building.card);
    std::sort(result.begin(), result.end());
    return result;
}
void conserved(const pop::Colony& before, const pop::Colony& after) {
    require(physical_cards(before) == physical_cards(after), "physical building cards changed");
}

void payment_and_exact_zero() {
    const pop::Colony initial{8, {{1, 3, 2, no_lost}}, {9}};
    pop::Loss loss(initial, 0, 5, clock_at_loss, "fixture-loss");
    require(loss.status() == pop::Status::paid, "sufficient base population was not paid");
    require(loss.snapshot().base_people == 3, "base payment amount wrong");
    require(loss.snapshot().buildings == initial.buildings, "payment voluntarily lost building");
    require(loss.result().requested_loss == 5 && loss.result().paid == 5 &&
                loss.result().unpaid_shortfall == 0, "payment components wrong");
    require(!loss.result().death && loss.result().lost.empty() && loss.choices().empty(),
            "living payment acquired mortality or choices");
    conserved(initial, loss.snapshot());
    const auto saved = loss.snapshot();
    rejects([&] { loss.choose_building(1); });
    require(loss.snapshot() == saved, "completed loss allowed voluntary building loss");

    pop::Loss zero({5, {}, {}}, 1, 5, clock_at_loss, "exact-zero");
    require(zero.status() == pop::Status::failed, "exact-zero population did not fail");
    require(zero.snapshot().base_people == 0 && zero.result().paid == 5,
            "exact-zero payment was not retained");
    require(zero.result().death.has_value(), "exact-zero death record absent");
    const auto death = zero.result().death;
    require(death->player == 1 && death->clock == clock_at_loss &&
                death->cause == "exact-zero" && death->deficit == 0,
            "exact-zero death context wrong");
    rejects([&] { zero.choose_building(77); });
    require(zero.result().death == death, "rejected operation changed death record");

    pop::Loss base_empty({2, {{1, 3, 2, no_lost}}, {}}, 0, 2, clock_at_loss, "base-empty");
    require(base_empty.status() == pop::Status::paid && !base_empty.result().death,
            "zero base was confused with zero total population");
    require(base_empty.snapshot().base_people == 0 && pop::total_people(base_empty.snapshot()) == 3,
            "printed building people missing from total");

    pop::Loss noop(initial, 0, 0, clock_at_loss, "zero-amount");
    require(noop.status() == pop::Status::paid && noop.snapshot() == initial &&
                noop.result().paid == 0 && !noop.result().death,
            "zero amount changed a living colony");
}

void explicit_building_choices_and_boxes() {
    const pop::Colony initial{1, {{1, 2, 3, no_lost}, {2, 4, 5, no_lost},
                                  {3, 8, 1, no_lost}}, {9}};
    pop::Loss loss(initial, 0, 6, clock_at_loss, "multi-building");
    require(loss.status() == pop::Status::needs_building_choice && loss.snapshot() == initial,
            "insufficient base was partially paid before choosing");
    require(loss.choices() == std::vector<pop::Card>{1, 2, 3}, "owned choices missing");
    const auto pending_result = loss.result();
    rejects([&] { loss.choose_building(99); });
    require(loss.snapshot() == initial && loss.result() == pending_result,
            "invalid building choice mutated pending loss");

    loss.choose_building(1);
    const auto after_first = loss.snapshot();
    require(loss.status() == pop::Status::needs_building_choice && after_first.base_people == 3,
            "full printed population was not transferred before payment");
    require(loss.result().paid == 0 && !loss.result().death,
            "requested loss was paid piecemeal");
    require(pop::total_people(after_first) == pop::total_people(initial),
            "transfer itself changed total population");
    require(after_first.discarded_buildings == std::vector<pop::Card>{9, 1},
            "lost physical card not appended to discard");
    require(loss.result().lost == std::vector<pop::LostBuilding>{{1, 2, 3}},
            "printed transfer or discarded boxes not recorded");
    require(loss.choices() == std::vector<pop::Card>{2, 3}, "lost building remained selectable");
    rejects([&] { loss.choose_building(1); });
    require(loss.snapshot() == after_first, "repeat choice mutated state");
    conserved(initial, after_first);

    loss.choose_building(2);
    const auto after = loss.snapshot();
    require(loss.status() == pop::Status::paid && after.base_people == 1,
            "multi-building assembled payment incorrect");
    require(pop::total_people(after) == pop::total_people(initial) - 6,
            "final population loss not applied exactly once");
    require(loss.result().lost == std::vector<pop::LostBuilding>{{1, 2, 3}, {2, 4, 5}},
            "building transfer history wrong");
    require(after.buildings == std::vector<pop::Building>{{3, 8, 1, no_lost}},
            "unchosen building or its boxes changed");
    conserved(initial, after);

    pop::Loss arbitrary(initial, 0, 6, clock_at_loss, "arbitrary-choice");
    arbitrary.choose_building(3);
    require(arbitrary.status() == pop::Status::paid && arbitrary.snapshot().base_people == 3,
            "large owned building could not be chosen first");
    require(arbitrary.snapshot().buildings.size() == 2 && arbitrary.result().lost[0].card == 3,
            "resolver selected a strategy instead of honoring caller choice");
    conserved(initial, arbitrary.snapshot());

    auto detached = arbitrary.snapshot();
    detached.base_people = 999;
    detached.buildings.clear();
    detached.discarded_buildings.clear();
    require(arbitrary.snapshot().base_people == 3 && arbitrary.snapshot().buildings.size() == 2,
            "snapshot aliases internal population state");
}

void insufficient_and_preexisting_zero() {
    const pop::Colony initial{2, {}, {1}};
    pop::Loss loss(initial, 1, 5, clock_at_loss, "cannot-pay");
    require(loss.status() == pop::Status::failed && loss.snapshot() == initial,
            "unpayable loss fabricated payment or negative population");
    require(loss.result().paid == 0 && loss.result().unpaid_shortfall == 3,
            "unpayable amount not preserved separately");
    require(loss.result().death && loss.result().death->deficit == 3 &&
                loss.result().death->clock == clock_at_loss &&
                loss.result().death->player == 1 && loss.result().death->cause == "cannot-pay",
            "unpayable death context wrong");

    const pop::Colony building_shortfall{1, {{4, 2, 7, no_lost}}, {}};
    pop::Loss assembled(building_shortfall, 0, 5, clock_at_loss, "still-short");
    require(assembled.status() == pop::Status::needs_terminal_rule &&
                assembled.snapshot() == building_shortfall && assembled.choices().empty(),
            "ambiguous total shortfall destroyed a building or chose interruption order");
    require(assembled.result().paid == 0 && assembled.result().unpaid_shortfall == 2 &&
                !assembled.result().death && assembled.result().lost.empty(),
            "unresolved terminal interruption fabricated payment or death");
    const auto unresolved_result = assembled.result();
    rejects([&] { assembled.choose_building(4); });
    require(assembled.snapshot() == building_shortfall && assembled.result() == unresolved_result,
            "terminal-rule blocker allowed a forced building choice");

    auto unknown_shortfall = building_shortfall;
    unknown_shortfall.buildings[0].lost_effects = pop::LostEffects::unverified;
    pop::Loss unknown(unknown_shortfall, 1, 5, clock_at_loss, "unknown-shortfall");
    require(unknown.status() == pop::Status::needs_terminal_rule &&
                unknown.snapshot() == unknown_shortfall && unknown.choices().empty() &&
                !unknown.result().death && unknown.result().unpaid_shortfall == 2,
            "unknown LOST shortfall invented effect resolution or failure timing");
    rejects([&] { unknown.choose_building(4); });
    require(unknown.snapshot() == unknown_shortfall && unknown.result().paid == 0 &&
                unknown.result().lost.empty(), "unknown LOST blocker changed official state");

    const pop::Colony zero{0, {{7, 0, 99, no_lost}}, {}};
    rejects([&] { pop::Loss failed(zero, 0, 1, clock_at_loss, "already-zero"); });
    rejects([&] { pop::Loss noop(zero, 0, 0, clock_at_loss, "already-zero-noop"); });
    rejects([&] { pop::Loss empty({0, {}, {}}, 0, 1, clock_at_loss, "unknown-prior-death"); });
}

void unsupported_effects_and_validation() {
    const pop::Colony unknown{1, {{5, 8, 4}}, {}};
    pop::Loss blocked(unknown, 0, 2, clock_at_loss, "unsupported-lost");
    const auto result = blocked.result();
    rejects([&] { blocked.choose_building(5); });
    require(blocked.snapshot() == unknown && blocked.result() == result,
            "unknown LOST effect was silently skipped or partially applied");
    require(blocked.status() == pop::Status::needs_building_choice &&
                blocked.choices() == std::vector<pop::Card>{5},
            "unsupported effect removed the real pending choice");

    rejects([] { pop::Loss x({1, {}, {}}, 2, 1, clock_at_loss, "bad-player"); });
    rejects([] { pop::Loss x({1, {}, {}}, 0, 1, clock_at_loss, ""); });
    rejects([] { pop::Loss x({1, {{1, 2, 0, no_lost}, {1, 3, 0, no_lost}}, {}},
                            0, 1, clock_at_loss, "duplicate"); });
    rejects([] { pop::Loss x({1, {{1, 2, 0, no_lost}}, {1}},
                            0, 1, clock_at_loss, "duplicate-zone"); });
    rejects([] { pop::Loss x({1, {}, {1, 1}}, 0, 1, clock_at_loss, "duplicate-discard"); });
    rejects<std::overflow_error>([] {
        pop::Loss x({std::numeric_limits<std::uint64_t>::max(), {{1, 1, 0, no_lost}}, {}},
                    0, 1, clock_at_loss, "total-overflow");
    });
    rejects<std::overflow_error>([] {
        (void)pop::total_people({0, {{1, std::numeric_limits<std::uint64_t>::max(), 0, no_lost},
                                     {2, 1, 0, no_lost}}, {}});
    });
    rejects<std::overflow_error>([] {
        pop::Loss x({1, {}, {}}, 0,
                    static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1,
                    clock_at_loss, "death-range-overflow");
    });
}
} // namespace

int main() {
    try {
        payment_and_exact_zero();
        explicit_building_choices_and_boxes();
        insufficient_and_preexisting_zero();
        unsupported_effects_and_validation();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional population-loss contracts passed with synthetic fixtures\n";
}
