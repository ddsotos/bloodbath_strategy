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
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
ses::Input fixture(std::vector<base::Card> sequence = {100}) {
    std::vector<base::Card> deck;
    for (base::Card card = 8; card < 90; ++card) deck.push_back(card);
    ses::Input input;
    input.colonies = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
                                           std::vector<base::Card>{4, 5, 6, 7}}, deck);
    input.progress.deck = std::move(sequence);
    input.config.scope = scope;
    input.config.tableau_enabled = true;
    input.config.building_catalog = {{0, tab::Kind::flag}, {1, tab::Kind::kale_farm},
                                     {2, tab::Kind::potato_farm}};
    input.config.effects = {{100, ses::Effect::work}, {101, ses::Effect::budget_cuts}};
    return input;
}
void install_potato_farm(ses::Input& input, bloodbath::Player player = 0) {
    auto& colony = input.colonies.colonies[player];
    colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), base::Card{2}));
    colony.buildings.push_back(2);
    colony.boxes.emplace(2, 0);
}
std::vector<ses::Command> mixed_build_script() {
    using K = ses::CommandKind;
    return {{K::advance}, {K::commit_choice, 0, WorkAction::build, {0, 1}, {1}, 1},
        {K::commit, 1, WorkAction::mine}, {K::resolve_work, 0, WorkAction::mine, {0, 1}}};
}
void costs_bonuses_and_mixed_payment() {
    auto input = fixture();
    install_potato_farm(input);
    auto& colony = input.colonies.colonies[0];
    colony.resources.money = 1;
    colony.resources.food = 1;
    require(tab::build_cost(tab::Kind::potato_farm) == 5 &&
            tab::printed_people(tab::Kind::potato_farm) == 6,
            "Potato Farm printed cost/population metadata is wrong");
    const auto options = tab::options(input.colonies, input.config.building_catalog, 0);
    require(std::find(options.begin(), options.end(), tab::Choice{WorkAction::build, 1, {}, 1}) != options.end() &&
            std::find(options.begin(), options.end(), tab::Choice{WorkAction::build, 1, {}, 0}) == options.end(),
            "Build options did not enumerate the affordable mixed payment");

    ses::Session game(input);
    (void)game.advance();
    const auto before = game.observe(1);
    game.commit_choice(0, {WorkAction::build, 1, {}, 1});
    require(game.observe(1) == before, "opponent observation leaked food payment choice");
    game.commit(1, WorkAction::mine);
    (void)game.resolve_work({0, 1});
    const auto& state = game.snapshot();
    require(state.colonies.colonies[0].resources.money == 0 &&
            state.colonies.colonies[0].resources.food == 7 &&
            tab::total_people(state.colonies.colonies[0], input.config.building_catalog) == 41,
            "Potato-enabled mixed payment or Kale Farm-triggered food bonus is wrong");
    const auto& effects = state.tableau_work->effects;
    require(effects.size() == 7 && effects[0] == tab::EffectRecord{0, tab::EffectKind::pay_money, 1, 1} &&
            effects[1] == tab::EffectRecord{0, tab::EffectKind::pay_food, 1, 1} &&
            effects[2] == tab::EffectRecord{0, tab::EffectKind::enter_play, 1, 5} &&
            effects[3] == tab::EffectRecord{0, tab::EffectKind::farm_base, std::nullopt, 4} &&
            effects[4] == tab::EffectRecord{0, tab::EffectKind::farm_bonus, 2, 2} &&
            effects[5] == tab::EffectRecord{0, tab::EffectKind::farm_bonus, 1, 1},
            "Potato payment or Farm bonus components were not recorded separately");
    const auto replay = ses::record(input, mixed_build_script());
    require(ses::verify(replay) == replay.checkpoints.back(), "mixed payment replay did not verify");
    auto tampered = replay;
    tampered.commands[1].food_payment = 0;
    bool rejected = false;
    try { (void)ses::verify(tampered); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "replay accepted a changed hidden food payment");
}
void valid_splits_and_rejections() {
    auto input = fixture();
    install_potato_farm(input);
    input.colonies.colonies[0].resources.money = 3;
    input.colonies.colonies[0].resources.food = 3;
    const auto options = tab::options(input.colonies, input.config.building_catalog, 0);
    for (std::uint64_t food = 0; food <= 2; ++food)
        require(std::find(options.begin(), options.end(), tab::Choice{WorkAction::build, 1, {}, food}) != options.end(),
                "valid money/food payment split missing");

    auto no_potato = fixture();
    no_potato.colonies.colonies[0].resources.money = 1;
    no_potato.colonies.colonies[0].resources.food = 10;
    ses::Session no_passive(no_potato);
    (void)no_passive.advance();
    const auto before = no_passive.snapshot();
    bool rejected = false;
    try { no_passive.commit_choice(0, {WorkAction::build, 1, {}, 1}); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected && no_passive.snapshot() == before,
            "food incorrectly paid a Build without an in-play Potato Farm");

    auto cannot_self_fund = fixture();
    cannot_self_fund.colonies.colonies[0].resources.money = 4;
    cannot_self_fund.colonies.colonies[0].resources.food = 10;
    const auto self_options = tab::options(cannot_self_fund.colonies,
        cannot_self_fund.config.building_catalog, 0);
    require(std::none_of(self_options.begin(), self_options.end(), [](const tab::Choice& choice) {
        return choice.action == WorkAction::build && choice.build == base::Card{2};
    }), "Potato Farm in hand funded its own initial Build");

    rejected = false;
    try { tab::validate_choice(input.colonies, input.config.building_catalog, 0,
            {WorkAction::farm, std::nullopt, {}, 1}); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "food payment was accepted on non-Build Work");
}
void budget_cuts_still_lose_money() {
    auto input = fixture({101});
    install_potato_farm(input);
    input.colonies.colonies[0].resources.money = 3;
    input.colonies.colonies[0].resources.food = 8;
    ses::Session game(input);
    require(game.advance() == ses::Phase::ready && game.snapshot().budget_cuts &&
            game.snapshot().budget_cuts->money_loss[0] == 2 &&
            game.snapshot().colonies.colonies[0].resources.money == 1 &&
            game.snapshot().colonies.colonies[0].resources.food == 8,
            "Potato Farm turned mandatory money loss into food payment");
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--json") {
            auto input = fixture();
            install_potato_farm(input);
            input.colonies.colonies[0].resources.money = 1;
            input.colonies.colonies[0].resources.food = 1;
            std::cout << ses::serialize_record(ses::record(input, mixed_build_script())) << '\n';
            return 0;
        }
        if (argc != 1) throw std::invalid_argument("unknown Potato Farm test mode");
        costs_bonuses_and_mixed_payment();
        valid_splits_and_rejections();
        budget_cuts_still_lose_money();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "reviewed Potato Farm contracts passed with synthetic scenarios\n";
}
