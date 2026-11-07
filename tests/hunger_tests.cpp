#include "engine/session.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace base = bloodbath::base_rules;
namespace tab = bloodbath::tableau;
namespace ses = bloodbath::session;
using bloodbath::Player;
using ses::Phase;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class Exception, class F> void rejects(F action) {
    try { action(); } catch (const Exception&) { return; }
    throw std::runtime_error("expected exception");
}

// All physical IDs, positions, resources and copy counts are synthetic inputs;
// this is not an official setup sampler or a claim about Farm multiplicities.
ses::Input fixture(std::array<std::size_t, 2> farms = {2, 1}) {
    ses::Input input;
    std::vector<base::Card> remainder;
    for (base::Card card = 8; card < 90; ++card) remainder.push_back(card);
    input.colonies = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
                                          std::vector<base::Card>{4, 5, 6, 7}}, remainder);
    input.config.scope = base::EffectScope::caller_verified_unmodified;
    input.config.tableau_enabled = true;
    input.config.effects = {{100, ses::Effect::hunger}, {101, ses::Effect::hunger},
                            {102, ses::Effect::instruction_manual}};
    input.progress.deck = {102, 101, 100};
    for (Player p = 0; p < 2; ++p) {
        auto& colony = input.colonies.colonies[p];
        colony.resources.people = 10;
        for (std::size_t i = 0; i < farms[p]; ++i) {
            const auto card = static_cast<base::Card>(p * 4 + i);
            colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), card));
            colony.buildings.push_back(card);
            colony.boxes.emplace(card, i + 2);
            input.config.building_catalog.emplace(card, tab::Kind::kale_farm);
        }
    }
    return input;
}
void record_equals(const ses::Snapshot& snapshot, std::array<std::uint64_t, 2> required,
                   std::array<std::uint64_t, 2> spent, std::array<std::uint64_t, 2> loss,
                   std::array<std::uint64_t, 2> gain) {
    require(snapshot.hunger && snapshot.hunger->required_food == required &&
                snapshot.hunger->spent_food == spent && snapshot.hunger->population_loss == loss &&
                snapshot.hunger->population_gain == gain, "Hunger audit components incorrect");
    require(!snapshot.deaths[0] && !snapshot.deaths[1] && !snapshot.official &&
                !snapshot.progress.active && snapshot.phase == Phase::ready,
            "nonterminal Hunger acquired death/outcome or retained active card");
}
ses::Input json_fixture() {
    auto input = fixture();
    input.colonies.colonies[0].resources.food = 1;
    input.colonies.colonies[1].resources.food = 4;
    return input;
}
void sufficient_food_and_zero_base() {
    auto input = fixture();
    input.colonies.colonies[0].resources.food = 2;
    input.colonies.colonies[1].resources.food = 7;
    input.colonies.colonies[0].resources.people = 0;
    ses::Session game(input);
    require(game.advance() == Phase::ready, "sufficient food did not resolve automatically");
    auto expected = input.colonies;
    expected.colonies[0].resources.food = 0;
    expected.colonies[1].resources.food = 6;
    expected.colonies[0].resources.people = 2;
    expected.colonies[1].resources.people = 12;
    const auto state = game.snapshot();
    require(state.colonies == expected, "food payment or growth changed unrelated state");
    record_equals(state, {2, 1}, {2, 1}, {0, 0}, {2, 2});
    require(state.progress.clock == bloodbath::EventClock{1, 0, 1, 0},
            "automatic Hunger invented a decision");
    require(game.observe(0).loss_options.empty() && game.observe(1).loss_options.empty(),
            "automatic Hunger offered Building choices");

    auto bare = fixture({0, 0});
    bare.config.tableau_enabled = false;
    bare.colonies.colonies[0].resources.food = 0;
    ses::Session no_buildings(bare);
    require(no_buildings.advance() == Phase::ready, "legacy bare Hunger failed");
    record_equals(no_buildings.snapshot(), {0, 0}, {0, 0}, {0, 0}, {2, 2});
    require(no_buildings.snapshot().colonies.colonies[0].resources.food == 0 &&
                no_buildings.snapshot().colonies.colonies[0].resources.people == 12,
            "no-food-needed growth changed bare Hunger semantics");
}
void shortage_payment_and_repetition() {
    const auto input = json_fixture();
    ses::Session game(input);
    require(game.advance() == Phase::ready, "base-funded shortage did not resolve");
    auto expected = input.colonies;
    expected.colonies[0].resources.food = 0;
    expected.colonies[0].resources.people = 9;
    expected.colonies[1].resources.food = 3;
    expected.colonies[1].resources.people = 12;
    require(game.snapshot().colonies == expected, "shortage applied wrong payment or growth");
    record_equals(game.snapshot(), {2, 1}, {1, 1}, {1, 0}, {0, 2});
    require(game.advance() == Phase::ready, "repeated Hunger did not resolve");
    expected.colonies[0].resources.people = 7;
    expected.colonies[1].resources.food = 2;
    expected.colonies[1].resources.people = 14;
    require(game.snapshot().colonies == expected, "repeated Hunger retained stale charge");
    record_equals(game.snapshot(), {2, 1}, {0, 1}, {2, 0}, {0, 2});
    require(game.snapshot().progress.clock == bloodbath::EventClock{2, 0, 2, 0},
            "repeated Hunger decision/event clock wrong");
}
void base_zero_with_printed_population() {
    auto input = fixture({1, 1});
    input.colonies.colonies[0].resources.people = 1;
    input.colonies.colonies[0].resources.food = 0;
    ses::Session game(input);
    require(game.advance() == Phase::ready, "exact base payment confused with colony death");
    require(game.snapshot().colonies.colonies[0].resources.people == 0 &&
                tab::total_people(game.snapshot().colonies.colonies[0], input.config.building_catalog) == 5,
            "Building population or full shortage payment wrong");
    record_equals(game.snapshot(), {1, 1}, {0, 1}, {1, 0}, {0, 2});
}
void blockers_and_overflow_are_atomic() {
    for (const std::uint64_t base_people : {std::uint64_t{0}, std::uint64_t{1}}) {
        auto input = fixture();
        input.colonies.colonies[1].resources.food = 0;
        // A two-Farm colony needs two food, but still has ten printed people:
        // failing this base payment requests a future Building-choice frame.
        input.colonies.colonies[0].resources.food = 0;
        input.colonies.colonies[0].resources.people = base_people;
        ses::Session game(input);
        require(game.advance() == Phase::hunger_choices && game.snapshot().pending_hunger &&
                    game.snapshot().colonies == input.colonies && !game.snapshot().hunger &&
                    !game.snapshot().deaths[0] && !game.snapshot().deaths[1] &&
                    !game.snapshot().official && game.snapshot().progress.clock.decision == 1,
                "pending Building loss partly paid Hunger or invented mortality");
        const auto blocked = game.snapshot();
        require(game.advance() == Phase::hunger_choices && game.snapshot() == blocked,
                "pending Hunger skipped its active card");
    }
    auto input = fixture();
    input.colonies.colonies[0].resources.food = 1; // valid payment must remain uncommitted
    input.colonies.colonies[1].resources.people =
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) - 5;
    ses::Session overflow(input);
    const auto initial = overflow.snapshot();
    rejects<std::overflow_error>([&] { (void)overflow.advance(); });
    require(overflow.snapshot() == initial, "second player's growth overflow partly committed first payment");
}
void replay_contracts() {
    for (std::uint64_t seed = 0; seed < 16; ++seed) {
        auto input = json_fixture();
        input.config.seed = seed;
        const auto trace = ses::record(input, {{}, {}});
        require(trace.schema == 8 && trace == ses::record(input, {{}, {}}) &&
                    ses::verify(trace) == trace.checkpoints.back(), "Hunger replay not reproducible");
        auto changed = trace;
        changed.checkpoints[0].hunger->spent_food[0]++;
        rejects<std::invalid_argument>([&] { (void)ses::verify(changed); });
        changed = trace;
        changed.checkpoints.back().hunger->population_gain[0] = 2;
        rejects<std::invalid_argument>([&] { (void)ses::verify(changed); });
        changed = trace;
        changed.schema = 5;
        rejects<std::invalid_argument>([&] { (void)ses::verify(changed); });
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--json") {
            std::cout << ses::serialize_record(ses::record(json_fixture(), {{}, {}}));
            return 0;
        }
        sufficient_food_and_zero_base();
        shortage_payment_and_repetition();
        base_zero_with_printed_population();
        blockers_and_overflow_are_atomic();
        replay_contracts();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional Hunger contracts passed with synthetic scenarios\n";
}
