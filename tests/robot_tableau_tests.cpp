#include "engine/session.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace base = bloodbath::base_rules;
namespace tab = bloodbath::tableau;
namespace ses = bloodbath::session;
using bloodbath::Player;
using ses::Effect;
using ses::Phase;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try { action(); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("expected rejection");
}

// Physical IDs, supplied deck positions, resources and Farm multiplicities below
// are synthetic scenarios, not legal setup samples or a complete Robot catalog.
ses::Input fixture() {
    ses::Input input;
    std::vector<base::Card> remainder;
    for (base::Card id = 8; id < 90; ++id) remainder.push_back(id);
    input.colonies = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
                                          std::vector<base::Card>{4, 5, 6, 7}}, remainder);
    input.config.scope = base::EffectScope::caller_verified_unmodified;
    input.config.tableau_enabled = true;
    input.config.effects = {{100, Effect::robot_waiter}, {101, Effect::mining_robot},
                            {102, Effect::robot_butler}, {103, Effect::unsupported}};
    input.progress.deck = {101, 100};
    for (Player p = 0; p < 2; ++p) {
        auto& colony = input.colonies.colonies[p];
        colony.resources.people = 10 + p;
        colony.resources.food = p == 0 ? 1 : 4;
        const auto id = static_cast<base::Card>(p * 4);
        colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), id));
        colony.buildings.push_back(id);
        colony.boxes.emplace(id, p + 2);
        input.config.building_catalog.emplace(id, tab::Kind::kale_farm);
    }
    return input;
}
void audit(const ses::Snapshot& state, Effect effect, std::uint64_t amount,
           std::array<std::uint64_t, 2> food_loss) {
    require(state.robot && state.robot->effect == effect && state.robot->food_loss == food_loss &&
                state.robot->money_gain == std::array<std::uint64_t, 2>{0, 0},
            "Robot tail audit is incorrect");
    for (const auto& loss : state.robot->losses)
        require(loss.status == bloodbath::population::Status::paid && loss.requested_loss == amount &&
                    loss.paid == amount && loss.unpaid_shortfall == 0 && loss.lost.empty() && !loss.death,
                "Robot population loss audit is incorrect");
    require(state.phase == Phase::ready && !state.progress.active && !state.official &&
                !state.deaths[0] && !state.deaths[1] && state.progress.clock.decision == 0,
            "nonterminal Robot invented decision, death or official outcome");
}
void waiter_food_and_unrelated_state() {
    for (const std::uint64_t food : {std::uint64_t{0}, std::uint64_t{1}, std::uint64_t{2}, std::uint64_t{7}}) {
        auto input = fixture();
        input.colonies.colonies[0].resources.food = food;
        ses::Session game(input);
        require(game.advance() == Phase::ready, "Kale-only Waiter was blocked");
        auto expected = input.colonies;
        expected.colonies[0].resources.people -= 4;
        expected.colonies[1].resources.people -= 4;
        expected.colonies[0].resources.food -= std::min(food, std::uint64_t{2});
        expected.colonies[1].resources.food -= 2;
        require(game.snapshot().colonies == expected,
                "Waiter changed cards, boxes, money or applied wrong food loss");
        audit(game.snapshot(), Effect::robot_waiter, 4, {std::min(food, std::uint64_t{2}), 2});
        require(game.snapshot().progress.clock == bloodbath::EventClock{1, 0, 1, 0} &&
                    game.observe(0).loss_options.empty() && game.observe(1).loss_options.empty(),
                "automatic Robot exposed a Building choice or miscounted clock");
    }
}
void base_exact_with_printed_people() {
    for (const auto effect : {Effect::robot_waiter, Effect::mining_robot}) {
        auto input = fixture();
        const std::uint64_t amount = effect == Effect::robot_waiter ? 4 : 6;
        input.progress.deck = {100};
        input.config.effects[100] = effect;
        for (auto& colony : input.colonies.colonies) colony.resources.people = amount;
        ses::Session game(input);
        require(game.advance() == Phase::ready, "exact base loss confused with total death");
        require(game.snapshot().colonies.colonies[0].resources.people == 0 &&
                    game.snapshot().colonies.colonies[1].resources.people == 0 &&
                    tab::total_people(game.snapshot().colonies.colonies[0], input.config.building_catalog) == 5,
                "printed Farm population did not preserve Robot survival");
        audit(game.snapshot(), effect, amount,
              effect == Effect::robot_waiter ? std::array<std::uint64_t, 2>{1, 2} :
                                                std::array<std::uint64_t, 2>{0, 0});
        if (effect == Effect::mining_robot) {
            auto expected = input.colonies;
            for (auto& colony : expected.colonies) colony.resources.people = 0;
            require(game.snapshot().colonies == expected,
                    "Kale Farm received an invented Mine/Farm/box bonus");
        }
    }
}
void preflight_and_unsupported_boundaries() {
    for (const auto effect : {Effect::robot_waiter, Effect::mining_robot}) {
        const std::uint64_t amount = effect == Effect::robot_waiter ? 4 : 6;
        for (Player weak = 0; weak < 2; ++weak) {
            auto input = fixture();
            input.progress.deck = {100};
            input.config.effects[100] = effect;
            input.colonies.colonies[weak].resources.people = amount - 1;
            ses::Session game(input);
            require(game.advance() == Phase::robot_choices && game.snapshot().pending_robot == effect &&
                        game.snapshot().colonies == input.colonies && !game.snapshot().robot &&
                        !game.snapshot().deaths[0] && !game.snapshot().deaths[1] && !game.snapshot().official,
                    "pending Building choice partly applied Robot loss/tail");
            const auto blocked = game.snapshot();
            require(game.advance() == Phase::robot_choices && game.snapshot() == blocked,
                    "pending Robot advanced past unresolved effect");
        }
        auto mixed = fixture();
        mixed.progress.deck = {100};
        mixed.config.effects[100] = effect;
        auto& bare = mixed.colonies.colonies[1];
        bare.hand.push_back(4);
        bare.buildings.clear();
        bare.boxes.clear();
        bare.resources.people = amount;
        ses::Session game(mixed);
        require(game.advance() == Phase::blocked_robot_loss && game.snapshot().colonies == mixed.colonies,
                "bare exact-total loss partly paid a mixed Tableau Robot");
    }
    for (const auto effect : {Effect::unsupported}) {
        auto input = fixture();
        input.progress.deck = {100};
        input.config.effects[100] = effect;
        ses::Session game(input);
        const auto expected = effect == Effect::robot_butler ? Phase::blocked_tableau_effect : Phase::blocked_unknown_card;
        require(game.advance() == expected && game.snapshot().colonies == input.colonies &&
                    !game.snapshot().robot, "unsupported Robot path gained inferred effects");
    }
}
void repeated_robot_and_replay() {
    auto input = fixture();
    ses::Session game(input);
    require(game.advance() == Phase::ready && game.advance() == Phase::ready,
            "Waiter/Mining sequence did not resolve");
    auto expected = input.colonies;
    expected.colonies[0].resources.people = 0;
    expected.colonies[1].resources.people = 1;
    expected.colonies[0].resources.food = 0;
    expected.colonies[1].resources.food = 2;
    require(game.snapshot().colonies == expected && game.snapshot().progress.clock == bloodbath::EventClock{2, 0, 2, 0},
            "repeated Robot record retained stale effects or changed unrelated state");
    audit(game.snapshot(), Effect::mining_robot, 6, {0, 0});
    auto bare_input = fixture();
    bare_input.config.tableau_enabled = false;
    bare_input.config.building_catalog.clear();
    for (Player p = 0; p < 2; ++p) {
        auto& colony = bare_input.colonies.colonies[p];
        colony.hand.insert(colony.hand.end(), colony.buildings.begin(), colony.buildings.end());
        colony.buildings.clear();
        colony.boxes.clear();
        colony.resources.people = 20 + p;
    }
    ses::Session bare(bare_input);
    require(bare.advance() == Phase::ready && bare.advance() == Phase::ready &&
                bare.snapshot().colonies.colonies[0].resources.people == 10 &&
                bare.snapshot().colonies.colonies[1].resources.people == 11,
            "legacy safe bare Robot sequence regressed");
    audit(bare.snapshot(), Effect::mining_robot, 6, {0, 0});
    for (std::uint64_t seed = 0; seed < 16; ++seed) {
        input.config.seed = seed;
        const auto replay = ses::record(input, {{}, {}});
        require(replay.schema == 15 && replay == ses::record(input, {{}, {}}) &&
                    ses::verify(replay) == replay.checkpoints.back(), "Robot Tableau replay diverged");
        auto changed = replay;
        changed.checkpoints[0].robot->food_loss[0]++;
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.checkpoints.back().robot->losses[0].paid--;
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.checkpoints.back().robot->money_gain[0] = 1;
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.schema = 7;
        rejects([&] { (void)ses::verify(changed); });
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--json") {
            std::cout << ses::serialize_record(ses::record(fixture(), {{}, {}}));
            return 0;
        }
        waiter_food_and_unrelated_state();
        base_exact_with_printed_people();
        preflight_and_unsupported_boundaries();
        repeated_robot_and_replay();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional Robot Tableau contracts passed with synthetic scenarios\n";
}
