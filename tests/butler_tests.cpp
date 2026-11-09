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
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F action) {
    try { action(); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("expected rejection");
}
// All IDs, copy counts, resources and Progress order are synthetic scenarios,
// not legal setup samples or official physical Kale Farm multiplicities.
ses::Input fixture() {
    ses::Input input;
    std::vector<base::Card> deck;
    for (base::Card id = 8; id < 90; ++id) deck.push_back(id);
    input.colonies = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
        std::vector<base::Card>{4, 5, 6, 7}}, deck);
    input.config.scope = base::EffectScope::caller_verified_unmodified;
    input.config.tableau_enabled = true;
    input.config.effects = {{100, Effect::robot_butler}, {101, Effect::robot_butler}};
    input.progress.deck = {101, 100};
    for (Player p = 0; p < 2; ++p) {
        auto& colony = input.colonies.colonies[p];
        colony.resources.people = 0;
        for (base::Card i = 0; i < 3; ++i) {
            const auto id = static_cast<base::Card>(p * 4 + i);
            colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), id));
            colony.buildings.push_back(id);
            colony.boxes.emplace(id, i == 0 ? p + 2 : (i == 1 ? p + 7 : 0));
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
void population_then_unique_tail() {
    const auto input = fixture();
    ses::Session game(input);
    require(game.advance() == Phase::robot_choices && game.snapshot().colonies == input.colonies,
            "Butler applied population or tail before choices");
    const auto opponent = game.observe(1);
    game.commit_losses(0, {1});
    require(game.observe(1) == opponent && game.snapshot().colonies == input.colonies,
            "Butler commitment leaked or executed early");
    game.commit_losses(1, {5});
    const auto result = game.resolve_robot({1, 0});
    auto expected = input.colonies;
    expected.colonies[0].resources.people = 1;
    expected.colonies[1].resources.people = 1;
    expected.colonies[0].buildings = {0, 2};
    expected.colonies[1].buildings = {4, 6};
    expected.colonies[0].boxes = {{0, 1}, {2, 0}};
    expected.colonies[1].boxes = {{4, 2}, {6, 0}};
    expected.building_discard = {5, 1};
    const auto done = game.snapshot();
    require(done.colonies == expected && done.phase == Phase::ready && done.robot == result &&
                result.completed && !done.pending_robot && !done.progress.active &&
                !done.loss_commitments[0] && !done.loss_commitments[1] &&
                done.progress.clock == bloodbath::EventClock{1, 0, 1, 1} &&
                !done.official && !done.deaths[0] && !done.deaths[1], "Butler unique-tail state incorrect");
    require(result.box_loss == std::array<std::optional<base::Card>, 2>{base::Card{0}, base::Card{4}} &&
                result.losses[0].lost == std::vector<bloodbath::population::LostBuilding>{{1, 5, 7}} &&
                result.losses[1].lost == std::vector<bloodbath::population::LostBuilding>{{5, 5, 8}} &&
                result.food_loss == std::array<std::uint64_t, 2>{0, 0} &&
                result.money_gain == std::array<std::uint64_t, 2>{0, 0},
            "Butler lost-Farm boxes double counted or forced target audit incorrect");
    rejects([&] { (void)game.resolve_robot({0, 1}); });
    require(game.snapshot() == done, "completed Butler allowed repeated payment");
}
void automatic_zero_one_targets() {
    auto input = fixture();
    for (auto& colony : input.colonies.colonies) {
        colony.resources.people = 10;
        for (auto& [id, boxes] : colony.boxes) { (void)id; boxes = 0; }
    }
    input.colonies.colonies[0].boxes[0] = 2;
    ses::Session game(input);
    auto expected = input.colonies;
    expected.colonies[0].resources.people = 6;
    expected.colonies[1].resources.people = 6;
    expected.colonies[0].boxes[0] = 1;
    require(game.advance() == Phase::ready && game.snapshot().colonies == expected &&
                game.snapshot().robot->completed && game.snapshot().robot->box_loss[0] == base::Card{0} &&
                !game.snapshot().robot->box_loss[1] && game.snapshot().progress.clock.decision == 0,
            "forced zero/single target invented a decision or removed wrong boxes");
    require(game.advance() == Phase::ready && game.snapshot().colonies.colonies[0].boxes.at(0) == 0 &&
                game.snapshot().robot->completed && game.snapshot().progress.clock.decision == 0,
            "repeated Butler retained stale box audit or missed last-box loss");
}
void ambiguous_tail_after_population() {
    auto input = fixture();
    ses::Session game(input);
    require(game.advance() == Phase::robot_choices, "Butler did not open population choices");
    game.commit_losses(0, {2});
    game.commit_losses(1, {6});
    const auto result = game.resolve_robot({1, 0});
    auto expected = input.colonies;
    expected.colonies[0].resources.people = 1;
    expected.colonies[1].resources.people = 1;
    expected.colonies[0].buildings = {0, 1};
    expected.colonies[1].buildings = {4, 5};
    expected.colonies[0].boxes = {{0, 2}, {1, 7}};
    expected.colonies[1].boxes = {{4, 3}, {5, 8}};
    expected.building_discard = {6, 2};
    const auto stopped = game.snapshot();
    require(stopped.phase == Phase::blocked_tableau_effect && stopped.colonies == expected &&
                stopped.robot == result && !result.completed && stopped.pending_robot == Effect::robot_butler &&
                result.box_loss == std::array<std::optional<base::Card>, 2>{} &&
                stopped.progress.active == base::Card{100} &&
                stopped.progress.clock == bloodbath::EventClock{1, 0, 1, 1} &&
                !stopped.loss_commitments[0] && !stopped.loss_commitments[1],
            "ambiguous tail chose target, skipped population or added a decision");
    require(game.advance() == Phase::blocked_tableau_effect && game.snapshot() == stopped,
            "advance skipped ambiguous Butler tail");
    rejects([&] { (void)game.resolve_robot({0, 1}); });
    require(game.snapshot() == stopped, "blocked tail allowed population payment twice");
    input = fixture();
    for (auto& colony : input.colonies.colonies) colony.resources.people = 10;
    input.colonies.colonies[1].boxes[5] = 0;
    ses::Session funded(input);
    expected = input.colonies;
    for (auto& colony : expected.colonies) colony.resources.people = 6;
    require(funded.advance() == Phase::blocked_tableau_effect && funded.snapshot().colonies == expected &&
                !funded.snapshot().robot->completed && funded.snapshot().progress.clock.decision == 0,
            "ambiguous tail removed opponent's forced box or invented decision");
}
void terminal_and_replay() {
    auto input = fixture();
    auto& bare = input.colonies.colonies[1];
    bare.hand.insert(bare.hand.end(), bare.buildings.begin(), bare.buildings.end());
    bare.buildings.clear();
    bare.boxes.clear();
    bare.resources.people = 4;
    ses::Session blocked(input);
    require(blocked.advance() == Phase::blocked_robot_loss && blocked.snapshot().colonies == input.colonies &&
                !blocked.snapshot().robot && !blocked.snapshot().pending_robot,
            "Butler terminal preflight paid population or boxes");
    for (std::uint64_t seed = 0; seed < 16; ++seed) {
        input = fixture();
        input.config.seed = seed;
        const auto replay = ses::record(input, script());
        require(replay.schema == 15 && replay == ses::record(input, script()) &&
                    ses::verify(replay) == replay.checkpoints.back(), "Butler replay diverged");
        auto changed = replay;
        changed.checkpoints.back().robot->completed = false;
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.commands[1].cards = {2};
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.checkpoints.back().robot->box_loss[0] = 2;
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.schema = 9;
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
        population_then_unique_tail();
        automatic_zero_one_targets();
        ambiguous_tail_after_population();
        terminal_and_replay();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional Butler forced-box contracts passed with synthetic scenarios\n";
}
