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
template<class F> void unchanged(ses::Session& game, F action) {
    const auto before = game.snapshot();
    rejects(action);
    require(game.snapshot() == before, "rejected Robot choice changed Session");
}

// All IDs, copy counts, resources and Progress positions are synthetic fixture
// inputs, not a legal setup sampler or official Kale Farm multiplicities.
ses::Input fixture(Effect effect = Effect::robot_waiter) {
    ses::Input input;
    std::vector<base::Card> deck;
    for (base::Card id = 8; id < 90; ++id) deck.push_back(id);
    input.colonies = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
                                          std::vector<base::Card>{4, 5, 6, 7}}, deck);
    input.config.scope = base::EffectScope::caller_verified_unmodified;
    input.config.tableau_enabled = true;
    input.config.effects = {{100, effect}, {101, effect}, {102, effect}};
    input.progress.deck = {102, 101, 100};
    for (Player p = 0; p < 2; ++p) {
        auto& colony = input.colonies.colonies[p];
        colony.resources.people = 0;
        colony.resources.food = p == 0 ? 1 : 4;
        for (base::Card i = 0; i < 2; ++i) {
            const auto id = static_cast<base::Card>(p * 4 + i);
            colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), id));
            colony.buildings.push_back(id);
            colony.boxes.emplace(id, i == 0 ? p + 2 : p + 7);
            input.config.building_catalog.emplace(id, tab::Kind::kale_farm);
        }
    }
    return input;
}
std::vector<ses::Command> script() {
    ses::Command first;
    first.kind = ses::CommandKind::commit_losses;
    first.cards = {1};
    auto second = first;
    second.player = 1;
    second.cards = {5};
    ses::Command resolve;
    resolve.kind = ses::CommandKind::resolve_robot;
    resolve.schedule = {1, 0};
    return {{}, first, second, resolve};
}
void waiter_choices_privacy_and_atomicity() {
    const auto input = fixture();
    ses::Session game(input);
    require(game.advance() == Phase::robot_choices, "Waiter did not open choices");
    const auto opened = game.snapshot();
    require(opened.colonies == input.colonies && !opened.robot &&
                opened.pending_robot == Effect::robot_waiter &&
                opened.progress.clock == bloodbath::EventClock{1, 0, 1, 1} &&
                opened.progress.active == base::Card{100},
            "opening Robot frame applied loss/tail or miscounted clock");
    require(game.advance() == Phase::robot_choices && game.snapshot() == opened,
            "advance skipped unresolved Robot");
    require(game.observe(0).loss_options == std::vector<base::Card>{0, 1} &&
                game.observe(1).loss_options == std::vector<base::Card>{4, 5},
            "Robot did not expose own owned Building options");
    unchanged(game, [&] { (void)game.resolve_robot({0, 1}); });
    unchanged(game, [&] { (void)game.resolve_hunger({0, 1}); });
    unchanged(game, [&] { (void)game.resolve_accident({0, 1}); });
    unchanged(game, [&] { game.commit_losses(0, {}); });
    unchanged(game, [&] { game.commit_losses(0, {1, 0}); });
    unchanged(game, [&] { game.commit_losses(0, {1, 1}); });
    unchanged(game, [&] { game.commit_losses(0, {4}); });
    unchanged(game, [&] { game.commit_losses(2, {1}); });
    const auto opponent = game.observe(1);
    game.commit_losses(0, {1});
    require(game.observe(1) == opponent && game.observe(0).loss_options.empty() &&
                game.snapshot().colonies == input.colonies,
            "Robot commitment leaked or executed resources early");
    unchanged(game, [&] { game.commit_losses(0, {0}); });
    unchanged(game, [&] { (void)game.resolve_robot({1, 0}); });
    game.commit_losses(1, {5});
    unchanged(game, [&] { (void)game.resolve_robot({1, 1}); });
    auto expected = input.colonies;
    expected.colonies[0].resources.people = 1;
    expected.colonies[1].resources.people = 1;
    expected.colonies[0].resources.food = 0;
    expected.colonies[1].resources.food = 2;
    expected.colonies[0].buildings = {0};
    expected.colonies[1].buildings = {4};
    expected.colonies[0].boxes = {{0, 2}};
    expected.colonies[1].boxes = {{4, 3}};
    expected.building_discard = {5, 1};
    const auto result = game.resolve_robot({1, 0});
    const auto done = game.snapshot();
    require(done.colonies == expected && done.robot == result && done.phase == Phase::ready &&
                !done.pending_robot && !done.loss_commitments[0] && !done.loss_commitments[1] &&
                !done.progress.active && done.progress.clock == opened.progress.clock &&
                !done.official && !done.deaths[0] && !done.deaths[1],
            "Robot resolution failed exact state/frame cleanup");
    require(result.effect == Effect::robot_waiter && result.resolution_order == std::array<Player, 2>{1, 0} &&
                result.food_loss == std::array<std::uint64_t, 2>{1, 2} &&
                result.money_gain == std::array<std::uint64_t, 2>{0, 0} &&
                result.losses[0].lost == std::vector<bloodbath::population::LostBuilding>{{1, 5, 7}} &&
                result.losses[1].lost == std::vector<bloodbath::population::LostBuilding>{{5, 5, 8}},
            "Robot transfer/boxes/tail audit incorrect");
    for (const auto& loss : result.losses)
        require(loss.requested_loss == 4 && loss.paid == 4 && !loss.death && loss.unpaid_shortfall == 0,
                "Waiter population payment was not one full nonterminal loss");
    unchanged(game, [&] { (void)game.resolve_robot({0, 1}); });
    require(game.advance() == Phase::robot_choices && !game.snapshot().robot &&
                game.snapshot().progress.clock.decision == 2,
            "repeated Robot retained completed record or wrong decision clock");
    game.commit_losses(0, {0});
    game.commit_losses(1, {4});
    (void)game.resolve_robot({0, 1});
    require(game.snapshot().colonies.colonies[0].resources.people == 2 &&
                game.snapshot().colonies.colonies[1].resources.people == 2,
            "repeated Robot loss incorrect");
    const auto before_terminal = game.snapshot().colonies;
    require(game.advance() == Phase::blocked_robot_loss && game.snapshot().colonies == before_terminal &&
                !game.snapshot().robot && !game.snapshot().pending_robot,
            "terminal Robot partly paid or retained pending frame");
}
void mining_multiple_buildings_and_mixed_funding() {
    auto input = fixture(Effect::mining_robot);
    ses::Session game(input);
    require(game.advance() == Phase::robot_choices, "Mining Robot did not open choices");
    unchanged(game, [&] { game.commit_losses(0, {1}); });
    game.commit_losses(0, {1, 0});
    game.commit_losses(1, {4, 5});
    const auto result = game.resolve_robot({1, 0});
    auto expected = input.colonies;
    for (auto& colony : expected.colonies) {
        colony.resources.people = 4;
        colony.buildings.clear();
        colony.boxes.clear();
    }
    expected.building_discard = {4, 5, 1, 0};
    require(game.snapshot().colonies == expected && result.losses[0].paid == 6 &&
                result.losses[1].paid == 6 && result.losses[0].lost.size() == 2 &&
                result.food_loss == std::array<std::uint64_t, 2>{0, 0} &&
                result.money_gain == std::array<std::uint64_t, 2>{0, 0},
            "Mining multiple-Farm loss or zero tail incorrect");
    auto mixed = fixture();
    mixed.colonies.colonies[1].resources.people = 10;
    ses::Session funded(mixed);
    require(funded.advance() == Phase::robot_choices && funded.observe(1).loss_options.empty(),
            "funded player offered voluntary Building loss");
    unchanged(funded, [&] { funded.commit_losses(1, {4}); });
    funded.commit_losses(1, {});
    funded.commit_losses(0, {0});
    const auto funded_result = funded.resolve_robot({0, 1});
    require(funded_result.losses[1].lost.empty() &&
                funded.snapshot().colonies.colonies[1].resources.people == 6 &&
                funded.snapshot().colonies.colonies[1].buildings == mixed.colonies.colonies[1].buildings,
            "empty funded commitment destroyed Buildings");
}
void terminal_and_butler_guards() {
    for (const auto effect : {Effect::robot_waiter, Effect::mining_robot}) {
        const std::uint64_t amount = effect == Effect::robot_waiter ? 4 : 6;
        for (const auto total : {amount - 1, amount}) {
            auto input = fixture(effect);
            auto& bare = input.colonies.colonies[1];
            bare.hand.insert(bare.hand.end(), bare.buildings.begin(), bare.buildings.end());
            bare.buildings.clear();
            bare.boxes.clear();
            bare.resources.people = total;
            ses::Session game(input);
            require(game.advance() == Phase::blocked_robot_loss && game.snapshot().colonies == input.colonies &&
                        !game.snapshot().pending_robot && !game.snapshot().robot &&
                        !game.snapshot().deaths[0] && !game.snapshot().deaths[1],
                    "terminal preflight mutated another player's choice/tail");
        }
    }
}
void replay_contracts() {
    for (std::uint64_t seed = 0; seed < 16; ++seed) {
        auto input = fixture();
        input.config.seed = seed;
        const auto replay = ses::record(input, script());
        require(replay.schema == 10 && replay == ses::record(input, script()) &&
                    ses::verify(replay) == replay.checkpoints.back(), "Robot choice replay diverged");
        auto changed = replay;
        changed.checkpoints[0].pending_robot = Effect::mining_robot;
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.commands[1].cards = {0};
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.checkpoints.back().robot->losses[0].lost[0].discarded_boxes++;
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.checkpoints.back().robot->resolution_order = {0, 1};
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.schema = 8;
        rejects([&] { (void)ses::verify(changed); });
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--json") {
            std::cout << ses::serialize_record(ses::record(fixture(), script()));
            return 0;
        }
        waiter_choices_privacy_and_atomicity();
        mining_multiple_buildings_and_mixed_funding();
        terminal_and_butler_guards();
        replay_contracts();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional Robot Building choice contracts passed with synthetic scenarios\n";
}
