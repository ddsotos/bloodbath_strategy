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
template<class Exception = std::invalid_argument, class F> void rejects(F action) {
    try { action(); } catch (const Exception&) { return; }
    throw std::runtime_error("expected rejection");
}
template<class F> void unchanged(ses::Session& game, F action) {
    const auto before = game.snapshot();
    rejects(action);
    require(game.snapshot() == before, "rejected choice command changed Session");
}

// Every physical ID, Farm multiplicity, initial resource and supplied Progress
// order is synthetic. These scenarios are not samples of official setup.
ses::Input fixture() {
    ses::Input input;
    std::vector<base::Card> remainder;
    for (base::Card id = 8; id < 90; ++id) remainder.push_back(id);
    input.colonies = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
                                          std::vector<base::Card>{4, 5, 6, 7}}, remainder);
    input.config.scope = base::EffectScope::caller_verified_unmodified;
    input.config.tableau_enabled = true;
    input.config.effects = {{100, ses::Effect::hunger}, {101, ses::Effect::hunger},
                            {102, ses::Effect::hunger}, {103, ses::Effect::instruction_manual}};
    input.progress.deck = {103, 102, 101, 100};
    for (Player p = 0; p < 2; ++p) {
        auto& colony = input.colonies.colonies[p];
        colony.resources.people = 0;
        colony.resources.food = 0;
        const auto id = static_cast<base::Card>(p * 4);
        colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), id));
        colony.buildings.push_back(id);
        colony.boxes.emplace(id, p + 2);
        input.config.building_catalog.emplace(id, tab::Kind::kale_farm);
    }
    auto& first = input.colonies.colonies[0];
    first.hand.erase(std::find(first.hand.begin(), first.hand.end(), base::Card{1}));
    first.buildings.push_back(1);
    first.boxes.emplace(1, 7);
    input.config.building_catalog.emplace(1, tab::Kind::kale_farm);
    return input;
}
std::vector<ses::Command> script() {
    ses::Command first;
    first.kind = ses::CommandKind::commit_losses;
    first.cards = {1};
    auto second = first;
    second.player = 1;
    second.cards = {4};
    ses::Command resolve;
    resolve.kind = ses::CommandKind::resolve_hunger;
    resolve.schedule = {1, 0};
    return {{}, first, second, resolve};
}
void complete_choice_frame() {
    const auto input = fixture();
    ses::Session game(input);
    require(game.observe(0).loss_options.empty(), "loss choices exposed before Hunger");
    require(game.advance() == Phase::hunger_choices, "Hunger did not open choices");
    const auto opened = game.snapshot();
    require(opened.colonies == input.colonies && opened.pending_hunger && !opened.hunger &&
                !opened.hunger_losses && opened.progress.active == base::Card{100} &&
                opened.progress.clock == bloodbath::EventClock{1, 0, 1, 1},
            "opening Hunger paid resources or miscounted clocks");
    require(opened.pending_hunger->required_food == std::array<std::uint64_t, 2>{2, 1} &&
                opened.pending_hunger->population_loss == std::array<std::uint64_t, 2>{2, 1},
            "pending Hunger requirements not frozen at initial Building count");
    require(game.observe(0).loss_options == std::vector<base::Card>{0, 1} &&
                game.observe(1).loss_options == std::vector<base::Card>{4},
            "own Building loss options incorrect");
    require(game.advance() == Phase::hunger_choices && game.snapshot() == opened,
            "advance skipped active Hunger choice frame");
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
            "loss commitment leaked or executed before resolution");
    unchanged(game, [&] { game.commit_losses(0, {0}); });
    unchanged(game, [&] { (void)game.resolve_hunger({0, 1}); });
    game.commit_losses(1, {4});
    unchanged(game, [&] { (void)game.resolve_hunger({0, 0}); });
    auto expected = input.colonies;
    expected.colonies[0].resources.people = 3; // five transferred minus frozen shortage TWO
    expected.colonies[0].buildings = {0};
    expected.colonies[0].boxes = {{0, 2}};
    expected.colonies[1].resources.people = 4;
    expected.colonies[1].buildings.clear();
    expected.colonies[1].boxes.clear();
    expected.building_discard = {4, 1};
    const auto result = game.resolve_hunger({1, 0});
    const auto done = game.snapshot();
    require(done.colonies == expected && done.phase == Phase::ready && done.hunger == result &&
                !done.pending_hunger && !done.loss_commitments[0] && !done.loss_commitments[1] &&
                !done.progress.active && done.progress.clock == opened.progress.clock &&
                !done.official && !done.deaths[0] && !done.deaths[1],
            "Hunger resolution changed wrong resources, retained frame or invented death");
    require(result.required_food == std::array<std::uint64_t, 2>{2, 1} &&
                result.spent_food == std::array<std::uint64_t, 2>{0, 0} &&
                result.population_loss == std::array<std::uint64_t, 2>{2, 1} &&
                result.population_gain == std::array<std::uint64_t, 2>{0, 0},
            "loss of a Building reduced initial Hunger requirement or awarded growth");
    require(done.hunger_losses && done.hunger_losses->resolution_order == std::array<Player, 2>{1, 0} &&
                done.hunger_losses->losses[0].lost == std::vector<bloodbath::population::LostBuilding>{{1, 5, 7}} &&
                done.hunger_losses->losses[1].lost == std::vector<bloodbath::population::LostBuilding>{{4, 5, 3}},
            "Hunger transfer and discarded box audit incorrect");
    unchanged(game, [&] { (void)game.resolve_hunger({1, 0}); });
    unchanged(game, [&] { game.commit_losses(0, {}); });
    // Next event has one Farm versus none: requirements must be recomputed.
    require(game.advance() == Phase::ready && game.snapshot().hunger->required_food ==
                std::array<std::uint64_t, 2>{1, 0} &&
                game.snapshot().colonies.colonies[0].resources.people == 2 &&
                game.snapshot().colonies.colonies[1].resources.people == 6 &&
                game.snapshot().progress.clock.decision == 1,
            "later Hunger retained old Building count or invented a decision");
}
void zero_shortage_empty_commit_and_growth() {
    auto input = fixture();
    input.colonies.colonies[0].resources.food = 1;
    input.colonies.colonies[1].resources.food = 2;
    ses::Session game(input);
    require(game.advance() == Phase::hunger_choices && game.observe(1).loss_options.empty(),
            "zero-shortage player offered voluntary Building loss");
    unchanged(game, [&] { game.commit_losses(1, {4}); });
    game.commit_losses(1, {});
    game.commit_losses(0, {0});
    const auto result = game.resolve_hunger({0, 1});
    require(result.spent_food == std::array<std::uint64_t, 2>{1, 1} &&
                result.population_loss == std::array<std::uint64_t, 2>{1, 0} &&
                result.population_gain == std::array<std::uint64_t, 2>{0, 2} &&
                game.snapshot().colonies.colonies[0].resources.people == 4 &&
                game.snapshot().colonies.colonies[1].resources.people == 2 &&
                game.snapshot().colonies.colonies[1].resources.food == 1 &&
                game.snapshot().colonies.colonies[1].buildings == std::vector<base::Card>{4},
            "mixed Hunger did not combine frozen food payment, loss and growth");
}
void multiple_building_choices() {
    auto input = fixture();
    auto& state = input.colonies;
    for (const base::Card id : {base::Card{2}, base::Card{3}, base::Card{8}, base::Card{9}}) {
        auto& source = id < 8 ? state.colonies[0].hand : state.building_deck;
        source.erase(std::find(source.begin(), source.end(), id));
        state.colonies[0].buildings.push_back(id);
        state.colonies[0].boxes.emplace(id, id + 1);
        input.config.building_catalog.emplace(id, tab::Kind::kale_farm);
    }
    auto& other = state.colonies[1];
    other.hand.push_back(4);
    other.buildings.clear();
    other.boxes.clear();
    other.resources.people = 30;
    ses::Session game(input);
    require(game.advance() == Phase::hunger_choices, "six-Farm shortage did not open choices");
    unchanged(game, [&] { game.commit_losses(0, {9}); });
    game.commit_losses(0, {9, 0});
    game.commit_losses(1, {});
    (void)game.resolve_hunger({0, 1});
    const auto first = game.snapshot();
    require(first.hunger->population_loss[0] == 6 && first.colonies.colonies[0].resources.people == 4 &&
                first.colonies.colonies[0].buildings == std::vector<base::Card>{1, 2, 3, 8} &&
                first.colonies.building_discard == std::vector<base::Card>{9, 0} &&
                first.hunger_losses->losses[0].lost == std::vector<bloodbath::population::LostBuilding>{{9, 5, 10}, {0, 5, 2}} &&
                first.colonies.colonies[1].resources.people == 32,
            "multi-Building shortage was not paid as one frozen loss");
    require(game.advance() == Phase::ready && game.snapshot().colonies.colonies[0].resources.people == 0,
            "base-funded repeated Hunger did not preserve living printed population");
    require(game.advance() == Phase::hunger_choices && !game.snapshot().hunger &&
                !game.snapshot().hunger_losses && game.snapshot().pending_hunger->required_food[0] == 4 &&
                game.snapshot().progress.clock.decision == 2,
            "repeated choice frame retained stale results or clock");
    game.commit_losses(0, {2});
    game.commit_losses(1, {});
    (void)game.resolve_hunger({1, 0});
    require(game.snapshot().colonies.colonies[0].resources.people == 1,
            "repeated choice frame reduced frozen requirement after discard");
}
void overflow_before_choice_is_atomic() {
    auto input = fixture();
    input.colonies.colonies[1].resources.food = 1;
    input.colonies.colonies[1].resources.people =
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) - 5;
    ses::Session game(input);
    const auto before = game.snapshot();
    rejects<std::overflow_error>([&] { (void)game.advance(); });
    require(game.snapshot() == before, "growth overflow opened an unresolvable choice frame");
}
void replay_contracts() {
    for (std::uint64_t seed = 0; seed < 16; ++seed) {
        auto input = fixture();
        input.config.seed = seed;
        const auto replay = ses::record(input, script());
        require(replay.schema == 11 && replay == ses::record(input, script()) &&
                    ses::verify(replay) == replay.checkpoints.back(), "Hunger choice replay diverged");
        auto changed = replay;
        changed.checkpoints[0].pending_hunger->required_food[0]--;
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.commands[1].cards = {0};
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.checkpoints.back().hunger_losses->losses[0].lost[0].discarded_boxes++;
        rejects([&] { (void)ses::verify(changed); });
        changed = replay;
        changed.schema = 6;
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
        complete_choice_frame();
        zero_shortage_empty_commit_and_growth();
        multiple_building_choices();
        overflow_before_choice_is_atomic();
        replay_contracts();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional Hunger Building choice contracts passed with synthetic scenarios\n";
}
