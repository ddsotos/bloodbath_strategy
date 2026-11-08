#include "engine/session.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace base = bloodbath::base_rules;
namespace ses = bloodbath::session;
namespace tab = bloodbath::tableau;
using base::WorkAction;

namespace {
constexpr auto scope = base::EffectScope::caller_verified_unmodified;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
ses::Input fixture(const std::vector<base::Card>& sequence) {
    std::vector<base::Card> deck;
    for (base::Card card = 8; card < 90; ++card) deck.push_back(card);
    ses::Input input;
    input.colonies = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
                                           std::vector<base::Card>{4, 5, 6, 7}}, deck);
    input.progress.deck = sequence;
    input.config.scope = scope;
    input.config.tableau_enabled = true;
    input.config.building_catalog = {{0, tab::Kind::flag}, {1, tab::Kind::kale_farm}};
    input.config.effects = {{100, ses::Effect::work}, {101, ses::Effect::hunger}};
    return input;
}
std::vector<ses::Command> build_flag() {
    using K = ses::CommandKind;
    return {{K::advance}, {K::commit_choice, 0, WorkAction::build, {0, 1}, {0}},
        {K::commit, 1, WorkAction::mine}, {K::resolve_work, 0, WorkAction::mine, {0, 1}}};
}
void flag_metadata_and_work() {
    auto input = fixture({100});
    const auto options = tab::options(input.colonies, input.config.building_catalog, 0);
    require(std::find(options.begin(), options.end(), tab::Choice{WorkAction::build, 0}) != options.end(),
            "zero-cost Flag was missing from affordable BUILD options");
    require(tab::printed_people(tab::Kind::flag) == 0 && tab::build_cost(tab::Kind::flag) == 0,
            "Flag printed metadata is wrong");
    ses::Session game(input);
    require(game.advance() == ses::Phase::work_decision, "Work did not open");
    game.commit_choice(0, {WorkAction::build, 0});
    game.commit(1, WorkAction::mine);
    (void)game.resolve_work({0, 1});
    const auto state = game.snapshot();
    require(state.colonies.colonies[0].resources.money == 10 &&
            state.colonies.colonies[0].resources.food == 4 &&
            state.colonies.colonies[0].buildings == std::vector<base::Card>{0} &&
            tab::total_people(state.colonies.colonies[0], input.config.building_catalog) == 30,
            "Flag build, PLAY cash, food or printed-population total is wrong");
    require(state.tableau_work->effects == std::vector<tab::EffectRecord>{{0, tab::EffectKind::pay_money, 0, 0},
            {0, tab::EffectKind::enter_play, 0, 0}, {0, tab::EffectKind::money_gain, 0, 6},
            {1, tab::EffectKind::mine_base, std::nullopt, 4}}, "Flag effect record is incomplete");
}
void hunger_ignores_flag() {
    auto input = fixture({101, 100}); // Work first, then Hunger (top is back).
    ses::Session game(input);
    for (const auto& command : build_flag()) {
        if (command.kind == ses::CommandKind::advance) (void)game.advance();
        else if (command.kind == ses::CommandKind::commit_choice)
            game.commit_choice(command.player, {command.action, command.cards[0]});
        else if (command.kind == ses::CommandKind::commit) game.commit(command.player, command.action);
        else (void)game.resolve_work(command.schedule);
    }
    require(game.advance() == ses::Phase::ready, "Hunger did not resolve with Flag only");
    const auto result = game.snapshot().hunger;
    require(result && result->required_food == std::array<std::uint64_t, 2>{0, 0} &&
            result->spent_food == std::array<std::uint64_t, 2>{0, 0} &&
            result->population_gain == std::array<std::uint64_t, 2>{2, 2} &&
            tab::hunger_food_cost(game.snapshot().colonies.colonies[0], input.config.building_catalog) == 0,
            "Flag incorrectly required Hunger food");
}
void zero_printed_loss_and_terminal_guard() {
    auto input = fixture({100});
    auto& colony = input.colonies.colonies[0];
    colony.resources.people = 1;
    colony.buildings = {0, 1};
    colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), base::Card{0}));
    colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), base::Card{1}));
    colony.boxes = {{0, 2}, {1, 3}};
    auto state = input.colonies;
    const auto loss = tab::lose_people(state, input.config.building_catalog, 0, 2, {0, 1},
        {1, 0, 1, 0}, "synthetic Flag loss", scope);
    require(loss.status == bloodbath::population::Status::paid &&
            loss.lost == std::vector<bloodbath::population::LostBuilding>{{0, 0, 2}, {1, 5, 3}} &&
            state.colonies[0].resources.people == 4 && state.colonies[0].buildings.empty() &&
            state.building_discard == std::vector<base::Card>{0, 1},
            "Flag did not transfer zero printed people and discard its boxes/card");
    auto terminal = input.colonies;
    terminal.colonies[0].resources.people = 0;
    terminal.colonies[0].buildings = {0};
    terminal.colonies[0].boxes = {{0, 1}};
    const auto before = terminal;
    bool rejected = false;
    try { (void)tab::lose_people(terminal, input.config.building_catalog, 0, 1, {0},
        {1, 0, 1, 0}, "synthetic terminal Flag loss", scope); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected && terminal == before, "terminal Flag loss was silently adjudicated or mutated");
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--json") {
            auto input = fixture({100});
            const auto replay = ses::record(input, build_flag());
            std::cout << ses::serialize_record(replay) << '\n';
            return 0;
        }
        if (argc != 1) throw std::invalid_argument("unknown Flag test mode");
        flag_metadata_and_work();
        hunger_ignores_flag();
        zero_printed_loss_and_terminal_guard();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "reviewed Flag contracts passed with synthetic scenarios\n";
}
