#include "engine/session.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace base = bloodbath::base_rules;
namespace tab = bloodbath::tableau;
namespace ses = bloodbath::session;
using ses::Phase;
using bloodbath::Player;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try { action(); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("expected invalid_argument");
}
template<class F> void unchanged(ses::Session& game, F action) {
    const auto before = game.snapshot();
    rejects(action);
    require(game.snapshot() == before, "rejected command mutated session");
}
// IDs, initial positions/populations and supplied Progress order are synthetic
// fixtures, not legal setup samples or official Kale Farm multiplicities.
ses::Input fixture() {
    ses::Input input;
    std::vector<base::Card> remainder;
    for (base::Card card = 8; card < 90; ++card) remainder.push_back(card);
    input.colonies = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
                                          std::vector<base::Card>{4, 5, 6, 7}}, remainder);
    input.config.scope = base::EffectScope::caller_verified_unmodified;
    input.config.tableau_enabled = true;
    input.config.building_catalog = {{0, tab::Kind::kale_farm}, {1, tab::Kind::kale_farm},
                                     {4, tab::Kind::kale_farm}};
    input.config.effects = {{100, ses::Effect::accident}, {101, ses::Effect::accident},
                            {102, ses::Effect::instruction_manual}};
    input.progress.deck = {102, 101, 100};
    for (Player p = 0; p < 2; ++p) {
        auto& colony = input.colonies.colonies[p];
        colony.resources.people = 1;
        const auto card = static_cast<base::Card>(p * 4);
        colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), card));
        colony.buildings.push_back(card);
        colony.boxes.emplace(card, p + 2);
    }
    auto& first = input.colonies.colonies[0];
    first.hand.erase(std::find(first.hand.begin(), first.hand.end(), base::Card{1}));
    first.buildings.push_back(1);
    first.boxes.emplace(1, 7);
    return input;
}
void choice_frame_and_privacy() {
    const auto input = fixture();
    ses::Session game(input);
    const auto initial = game.snapshot();
    require(game.observe(0).loss_options.empty(), "loss choices leaked before Accident");
    require(game.advance() == Phase::accident_choices, "Accident did not open Building choices");
    const auto opened = game.snapshot();
    require(opened.colonies == initial.colonies && !opened.accident &&
                opened.progress.active == base::Card{100} &&
                opened.progress.clock == bloodbath::EventClock{1, 0, 1, 1},
            "opening frame applied loss or miscounted decision clock");
    require(game.observe(0).loss_options == std::vector<base::Card>{0, 1} &&
                game.observe(1).loss_options == std::vector<base::Card>{4},
            "loss choices exposed other player's Buildings");
    require(game.advance() == Phase::accident_choices && game.snapshot() == opened,
            "advance skipped pending loss frame");
    unchanged(game, [&] { (void)game.resolve_accident({0, 1}); });
    unchanged(game, [&] { game.commit_losses(0, {}); });
    unchanged(game, [&] { game.commit_losses(0, {0, 1}); });
    unchanged(game, [&] { game.commit_losses(0, {0, 0}); });
    unchanged(game, [&] { game.commit_losses(0, {4}); });
    unchanged(game, [&] { game.commit_losses(2, {0}); });
    const auto opponent_view = game.observe(1);
    game.commit_losses(0, {1});
    require(game.observe(1) == opponent_view, "private loss commitment leaked to opponent");
    require(game.observe(0).loss_options.empty() &&
                game.snapshot().colonies == opened.colonies,
            "commitment executed loss before both locked");
    unchanged(game, [&] { game.commit_losses(0, {0}); });
    unchanged(game, [&] { (void)game.resolve_accident({0, 1}); });
    game.commit_losses(1, {4});
    unchanged(game, [&] { (void)game.resolve_accident({1, 1}); });
    auto expected = input.colonies;
    expected.colonies[0].resources.people = 3;
    expected.colonies[0].buildings = {0};
    expected.colonies[0].boxes = {{0, 2}};
    expected.colonies[1].resources.people = 3;
    expected.colonies[1].buildings.clear();
    expected.colonies[1].boxes.clear();
    expected.building_discard = {4, 1};
    const auto result = game.resolve_accident({1, 0});
    require(result.resolution_order == std::array<Player, 2>{1, 0} &&
                result.losses[0].lost == std::vector<bloodbath::population::LostBuilding>{{1, 5, 7}} &&
                result.losses[1].lost == std::vector<bloodbath::population::LostBuilding>{{4, 5, 3}},
            "loss audit record omitted chosen transfers or box removal");
    for (const auto& loss : result.losses)
        require(loss.paid == 3 && loss.requested_loss == 3 && !loss.death,
                "nonterminal Accident record has wrong payment/death");
    const auto done = game.snapshot();
    require(done.phase == Phase::ready && done.colonies == expected && done.accident == result &&
                !done.progress.active && done.progress.clock == opened.progress.clock &&
                !done.loss_commitments[0] && !done.loss_commitments[1] &&
                !done.official && !done.deaths[0] && !done.deaths[1],
            "resolved Accident did not preserve exact state and clear frame");
    unchanged(game, [&] { game.commit_losses(0, {}); });
    unchanged(game, [&] { (void)game.resolve_accident({0, 1}); });
    // Second Accident cannot safely resolve player 1's exact-zero loss. Neither
    // colony is mutated before this shared preflight boundary.
    require(game.advance() == Phase::blocked_population_payment &&
                game.snapshot().colonies == done.colonies && !game.snapshot().deaths[1],
            "terminal preflight partly applied another player's loss");
}
void base_payment_and_repeated_frame() {
    auto mixed = fixture();
    mixed.colonies.colonies[1].resources.people = 10;
    ses::Session mixed_game(mixed);
    require(mixed_game.advance() == Phase::accident_choices && mixed_game.observe(1).loss_options.empty(),
            "mixed base funding exposed voluntary Building choices");
    unchanged(mixed_game, [&] { mixed_game.commit_losses(1, {4}); });
    mixed_game.commit_losses(1, {});
    mixed_game.commit_losses(0, {0});
    const auto mixed_result = mixed_game.resolve_accident({0, 1});
    require(mixed_result.losses[1].lost.empty() &&
                mixed_game.snapshot().colonies.colonies[1].resources.people == 7,
            "empty commitment did not preserve funded player's Buildings");
    auto input = fixture();
    input.colonies.colonies[0].resources.people = 10;
    input.colonies.colonies[1].resources.people = 10;
    ses::Session game(input);
    require(game.advance() == Phase::ready, "base-funded tableau Accident did not auto-resolve");
    require(game.observe(0).loss_options.empty() && game.observe(1).loss_options.empty(),
            "sufficient base allowed voluntary Building loss");
    unchanged(game, [&] { game.commit_losses(0, {0}); });
    const auto first = *game.snapshot().accident;
    require(first.losses[0].lost.empty() && first.losses[1].lost.empty(),
            "base-funded Accident discarded Buildings");
    require(game.advance() == Phase::ready && game.snapshot().accident &&
                game.snapshot().progress.clock.decision == 0,
            "base-funded repeated Accident invented a decision");
    require(game.snapshot().colonies.colonies[0].resources.people == 4 &&
                game.snapshot().colonies.colonies[1].resources.people == 4,
            "two Accidents were not paid exactly once each");

    input = fixture();
    for (auto& colony : input.colonies.colonies) colony.resources.people = 0;
    auto& second = input.colonies.colonies[1];
    second.hand.erase(std::find(second.hand.begin(), second.hand.end(), base::Card{5}));
    second.buildings.push_back(5);
    second.boxes.emplace(5, 9);
    input.config.building_catalog.emplace(5, tab::Kind::kale_farm);
    ses::Session repeated(input);
    require(repeated.advance() == Phase::accident_choices, "first low-base Accident did not request choice");
    repeated.commit_losses(0, {0});
    repeated.commit_losses(1, {4});
    (void)repeated.resolve_accident({0, 1});
    require(repeated.advance() == Phase::accident_choices && !repeated.snapshot().accident &&
                repeated.snapshot().progress.clock.decision == 2,
            "new Accident choice frame retained stale record or clock");
    repeated.commit_losses(0, {1});
    repeated.commit_losses(1, {5});
    (void)repeated.resolve_accident({1, 0});
    require(repeated.snapshot().colonies.colonies[0].resources.people == 4 &&
                repeated.snapshot().colonies.colonies[1].resources.people == 4,
            "repeated choices lost or paid population incorrectly");
}
void terminal_guards_and_legacy() {
    for (const std::uint64_t people : {std::uint64_t{2}, std::uint64_t{3}}) {
        auto input = fixture();
        input.colonies.colonies[1].resources.people = people;
        input.colonies.colonies[1].hand.push_back(4);
        input.colonies.colonies[1].buildings.clear();
        input.colonies.colonies[1].boxes.clear();
        ses::Session game(input);
        require(game.advance() == Phase::blocked_population_payment &&
                    game.snapshot().colonies == input.colonies && !game.snapshot().official &&
                    !game.snapshot().deaths[0] && !game.snapshot().deaths[1] &&
                    game.snapshot().progress.clock.decision == 0,
                "tableau terminal preflight fabricated payment or death");
        const auto blocked = game.snapshot();
        require(game.advance() == Phase::blocked_population_payment && game.snapshot() == blocked,
                "blocked terminal frame advanced");
    }
    auto input = fixture();
    for (auto& colony : input.colonies.colonies) {
        colony.hand.insert(colony.hand.end(), colony.buildings.begin(), colony.buildings.end());
        colony.buildings.clear();
        colony.boxes.clear();
    }
    input.config.tableau_enabled = false;
    input.config.building_catalog.clear();
    input.colonies.colonies[0].resources.people = 3;
    input.colonies.colonies[1].resources.people = 4;
    ses::Session game(input);
    require(game.advance() == Phase::official_terminal && game.snapshot().deaths[0] &&
                !game.snapshot().deaths[1] && game.snapshot().official->scores == bloodbath::Scores{0, 1} &&
                game.snapshot().progress.clock.decision == 0,
            "bare exact-zero Accident behavior changed");
}
void replay_contracts() {
    ses::Command commit0;
    commit0.kind = ses::CommandKind::commit_losses;
    commit0.cards = {1};
    auto commit1 = commit0;
    commit1.player = 1;
    commit1.cards = {4};
    ses::Command resolve;
    resolve.kind = ses::CommandKind::resolve_accident;
    resolve.schedule = {1, 0};
    const std::vector<ses::Command> commands{{}, commit0, commit1, resolve};
    for (std::uint64_t seed = 0; seed < 16; ++seed) {
        auto input = fixture();
        input.config.seed = seed;
        const auto trace = ses::record(input, commands);
        require(trace.schema == 6 && trace == ses::record(input, commands) &&
                    ses::verify(trace) == trace.checkpoints.back(), "Accident replay not deterministic");
        auto changed = trace;
        changed.commands[1].cards = {0};
        rejects([&] { (void)ses::verify(changed); });
        changed = trace;
        changed.checkpoints.back().accident->losses[0].lost[0].discarded_boxes++;
        rejects([&] { (void)ses::verify(changed); });
        changed = trace;
        changed.checkpoints[1].loss_commitments[0] = std::vector<base::Card>{0};
        rejects([&] { (void)ses::verify(changed); });
        changed = trace;
        changed.schema = 4;
        rejects([&] { (void)ses::verify(changed); });
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--json") {
            ses::Command commit0;
            commit0.kind = ses::CommandKind::commit_losses;
            commit0.cards = {1};
            auto commit1 = commit0;
            commit1.player = 1;
            commit1.cards = {4};
            ses::Command resolve;
            resolve.kind = ses::CommandKind::resolve_accident;
            resolve.schedule = {1, 0};
            std::cout << ses::serialize_record(ses::record(fixture(), {{}, commit0, commit1, resolve}));
            return 0;
        }
        choice_frame_and_privacy();
        base_payment_and_repeated_frame();
        terminal_guards_and_legacy();
        replay_contracts();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional Accident choice contracts passed with synthetic scenarios\n";
}
