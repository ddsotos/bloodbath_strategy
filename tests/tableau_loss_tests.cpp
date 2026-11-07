#include "engine/tableau.hpp"
#include "engine/population.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace base = bloodbath::base_rules;
namespace tab = bloodbath::tableau;
namespace pop = bloodbath::population;
namespace {
constexpr auto scope = base::EffectScope::caller_verified_unmodified;
constexpr bloodbath::EventClock clock_at_loss{17, 2, 3, 11};
const tab::Catalog catalog{{0, tab::Kind::kale_farm}, {1, tab::Kind::kale_farm},
                           {2, tab::Kind::kale_farm}};
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
// IDs, deal, resources and loss amounts are synthetic scenarios, not official
// setup samples or a claim about the number of physical Kale Farm copies.
base::State fixture(std::uint64_t people, std::size_t farms) {
    std::vector<base::Card> deck;
    for (base::Card card = 8; card < 90; ++card) deck.push_back(card);
    auto state = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
                                       std::vector<base::Card>{4, 5, 6, 7}}, deck);
    auto& colony = state.colonies[0];
    colony.resources.people = people;
    for (std::size_t i = 0; i < farms; ++i) {
        const auto card = static_cast<base::Card>(i);
        colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), card));
        colony.buildings.push_back(card);
        colony.boxes.emplace(card, 3 + i);
    }
    // Existing shared discard must remain below newly lost cards.
    state.building_discard.push_back(state.building_deck.back());
    state.building_deck.pop_back();
    state.building_shuffle_index = 9;
    return state;
}
pop::Result lose(base::State& state, std::uint64_t amount,
                 const std::vector<base::Card>& choices = {}) {
    return tab::lose_people(state, catalog, 0, amount, choices, clock_at_loss,
                            "synthetic-loss", scope);
}
void paid(const pop::Result& result, std::uint64_t amount) {
    require(result.status == pop::Status::paid && result.requested_loss == amount &&
                result.paid == amount && result.unpaid_shortfall == 0 && !result.death,
            "nonterminal payment record is incorrect");
}
template<class F> void rejects_unchanged(base::State& state, F action) {
    const auto before = state;
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "expected invalid_argument rejection");
    require(state == before, "rejected adapter call mutated shared game state");
}
void direct_payment() {
    auto state = fixture(10, 2);
    auto expected = state;
    expected.colonies[0].resources.people = 7;
    const auto result = lose(state, 3);
    paid(result, 3);
    require(result.lost.empty() && state == expected,
            "base payment changed Buildings, boxes, resources or other player");
    const auto before = state;
    paid(lose(state, 0), 0);
    require(state == before, "zero loss changed state");
    rejects_unchanged(state, [&] { (void)lose(state, 0, {0}); });
    auto bare = fixture(8, 0);
    auto bare_expected = bare;
    bare_expected.colonies[0].resources.people = 6;
    paid(lose(bare, 2), 2);
    require(bare == bare_expected, "no-Building payment changed unrelated state");
}
void building_transfer() {
    auto state = fixture(1, 3);
    auto expected = state;
    expected.colonies[0].resources.people = 3; // 1 + 5 + 5 - 8
    expected.colonies[0].buildings = {1};
    expected.colonies[0].boxes = {{1, 4}};
    expected.building_discard.push_back(2);
    expected.building_discard.push_back(0);
    const auto result = lose(state, 8, {2, 0});
    paid(result, 8);
    require(result.lost == std::vector<pop::LostBuilding>{{2, 5, 5}, {0, 5, 3}},
            "ordered printed transfers and discarded boxes were not recorded");
    require(state == expected, "Building losses did not update exact shared zones");
    require(tab::total_people(state.colonies[0], catalog) == 8,
            "total population did not decrease exactly once");

    auto zero_base = fixture(0, 2);
    const auto zero_result = lose(zero_base, 3, {1});
    paid(zero_result, 3);
    require(zero_base.colonies[0].resources.people == 2 &&
                zero_base.colonies[0].buildings == std::vector<base::Card>{0} &&
                zero_base.colonies[0].boxes == std::map<base::Card, std::uint64_t>{{0, 3}},
            "living zero-base colony was mishandled");
}
void choices_are_required_and_atomic() {
    auto state = fixture(1, 3);
    rejects_unchanged(state, [&] { (void)lose(state, 8); });
    rejects_unchanged(state, [&] { (void)lose(state, 8, {0}); });
    rejects_unchanged(state, [&] { (void)lose(state, 8, {0, 1, 2}); });
    rejects_unchanged(state, [&] { (void)lose(state, 8, {0, 0}); });
    rejects_unchanged(state, [&] { (void)lose(state, 8, {0, 4}); });
    rejects_unchanged(state, [&] { (void)lose(state, 8, {99, 0}); });
    rejects_unchanged(state, [&] { (void)lose(state, 1, {0}); });
    rejects_unchanged(state, [&] { (void)lose(state, 16, {0, 1, 2}); });
    rejects_unchanged(state, [&] { (void)lose(state, 17, {0, 1, 2}); });
    auto bare = fixture(3, 0);
    rejects_unchanged(bare, [&] { (void)lose(bare, 3); });
    rejects_unchanged(bare, [&] { (void)lose(bare, 4); });
}
void context_validation() {
    auto state = fixture(1, 2);
    auto unknown = catalog;
    unknown.erase(1);
    rejects_unchanged(state, [&] {
        (void)tab::lose_people(state, unknown, 0, 3, {0}, clock_at_loss, "loss", scope);
    });
    rejects_unchanged(state, [&] {
        (void)tab::lose_people(state, catalog, 2, 3, {0}, clock_at_loss, "loss", scope);
    });
    rejects_unchanged(state, [&] {
        (void)tab::lose_people(state, catalog, 0, 3, {0}, clock_at_loss, "loss",
                                base::EffectScope::unverified);
    });
    rejects_unchanged(state, [&] {
        (void)tab::lose_people(state, catalog, 0, 3, {0}, clock_at_loss, "", scope);
    });
}
} // namespace
int main() {
    try {
        direct_payment();
        building_transfer();
        choices_are_required_and_atomic();
        context_validation();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "reviewed Tableau nonterminal loss contracts passed with synthetic scenarios\n";
}
