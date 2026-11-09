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
template<class F> void rejects(F function) {
    try { function(); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("expected invalid_argument");
}
ses::Input fixture() {
    std::vector<base::Card> deck;
    for (base::Card card = 8; card < 90; ++card) deck.push_back(card);
    ses::Input input;
    input.colonies = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
                                           std::vector<base::Card>{4, 5, 6, 7}}, deck);
    input.progress.deck = {100};
    input.config.scope = scope;
    input.config.tableau_enabled = true;
    input.config.building_catalog = {{0, tab::Kind::fuel_cells}, {1, tab::Kind::kale_farm}};
    input.config.effects = {{100, ses::Effect::work}};
    return input;
}
void install_fuel_cells(ses::Input& input, bloodbath::Player p, base::Card card, std::uint64_t boxes) {
    auto& colony = input.colonies.colonies[p];
    const auto found = std::find(colony.hand.begin(), colony.hand.end(), card);
    require(found != colony.hand.end(), "Fuel Cells fixture card is not in hand");
    colony.hand.erase(found);
    colony.buildings.push_back(card);
    colony.boxes.emplace(card, boxes);
}
std::vector<ses::Command> script(base::Card discard = 89) {
    using K = ses::CommandKind;
    ses::Command activation{K::commit_choice, 0, WorkAction::restock, {0, 1}, {0, 0}};
    activation.fuel_cells_spend = true;
    return {{K::advance}, activation, {K::commit, 1, WorkAction::mine},
        {K::resolve_work, 0, WorkAction::mine, {0, 1}},
        {K::commit_discards, 0, WorkAction::mine, {0, 1}, {discard}}};
}
void metadata_options_and_same_restock_activation() {
    auto input = fixture();
    install_fuel_cells(input, 0, 0, 0);
    require(tab::build_cost(tab::Kind::fuel_cells) == 3 && tab::printed_people(tab::Kind::fuel_cells) == 6,
            "Fuel Cells cost or printed people metadata is wrong");
    auto options = tab::options(input.colonies, input.config.building_catalog, 0);
    const tab::Choice restock{WorkAction::restock, std::nullopt, {0, 0}};
    auto activate = restock;
    activate.spend_fuel_cells = true;
    require(std::find(options.begin(), options.end(), restock) != options.end() &&
            std::find(options.begin(), options.end(), activate) != options.end(),
            "RESTOCK options omitted spending a box placed on Fuel Cells by this action");

    ses::Session game(input);
    require(game.advance() == ses::Phase::work_decision, "Work did not open");
    const auto opponent_before = game.observe(1);
    game.commit_choice(0, activate);
    require(game.observe(1) == opponent_before, "Fuel Cells use choice leaked before both Work commits");
    game.commit(1, WorkAction::mine);
    (void)game.resolve_work({0, 1});
    require(game.snapshot().phase == ses::Phase::fuel_cells_discard &&
            game.snapshot().pending_fuel_cells_discard == bloodbath::Player{0},
            "Fuel Cells did not pause Work at the private discard choice");
    require(game.observe(0).required_discards == 1 && game.observe(1).required_discards == 0,
            "Fuel Cells discard decision was not restricted to its owner");
    require(game.observe(0).colony.own_hand.size() == 4 && game.observe(1).colony.own_hand.size() == 4,
            "Fuel Cells draw did not add one card or leaked an opponent hand change");
    require(game.observe(1).colony.own_hand == input.colonies.colonies[1].hand,
            "opponent observation exposed the Fuel Cells draw");
    const auto drawn = game.snapshot().tableau_work->base.drawn[0];
    require(drawn.size() == 1 && game.snapshot().progress.active == bloodbath::progression::Card{100},
            "Fuel Cells draw or active Work checkpoint was not retained");
    require(game.advance() == ses::Phase::fuel_cells_discard,
            "Progress advanced before the Fuel Cells discard choice");
    rejects([&] { game.commit_discards(1, {drawn[0]}); });
    rejects([&] { game.commit_discards(0, {}); });
    rejects([&] { game.commit_discards(0, {drawn[0], base::Card{1}}); });
    game.commit_discards(0, {drawn[0]}); // The newly drawn card is eligible.
    const auto& final = game.snapshot();
    require(final.phase == ses::Phase::ready && !final.pending_fuel_cells_discard &&
            final.colonies.colonies[0].boxes.at(0) == 1 &&
            final.colonies.colonies[0].resources.money == 8 &&
            final.colonies.building_discard.back() == drawn[0] &&
            std::find(final.colonies.colonies[0].hand.begin(), final.colonies.colonies[0].hand.end(), drawn[0]) ==
                final.colonies.colonies[0].hand.end() &&
            final.tableau_work->fuel_cells_discarded[0] == drawn[0],
            "Fuel Cells box, money, draw/discard or public result is wrong");
}
void existing_box_and_invalid_choices() {
    auto input = fixture();
    install_fuel_cells(input, 0, 0, 1);
    input.colonies.colonies[0].hand.erase(std::find(input.colonies.colonies[0].hand.begin(),
                                                    input.colonies.colonies[0].hand.end(), base::Card{1}));
    input.colonies.colonies[0].buildings.push_back(1);
    input.colonies.colonies[0].boxes.emplace(1, 0);
    const tab::Choice spend_existing{WorkAction::restock, std::nullopt, {1, 1}, 0, true};
    const auto options = tab::options(input.colonies, input.config.building_catalog, 0);
    require(std::find(options.begin(), options.end(), spend_existing) != options.end(),
            "RESTOCK options omitted spending a pre-existing Fuel Cells box");
    const std::array<tab::Choice, 2> choices{spend_existing, {WorkAction::mine}};
    auto record = tab::resolve(input.colonies, input.config.building_catalog, choices, {0, 1}, 7, scope);
    require(input.colonies.colonies[0].boxes.at(0) == 0 && input.colonies.colonies[0].boxes.at(1) == 2 &&
            input.colonies.colonies[0].resources.money == 8 && record.base.drawn[0].size() == 1,
            "Fuel Cells did not spend an existing box exactly once");

    auto empty = fixture();
    install_fuel_cells(empty, 0, 0, 0);
    empty.colonies.colonies[0].hand.erase(std::find(empty.colonies.colonies[0].hand.begin(),
                                                    empty.colonies.colonies[0].hand.end(), base::Card{1}));
    empty.colonies.colonies[0].buildings.push_back(1);
    empty.colonies.colonies[0].boxes.emplace(1, 0);
    auto invalid = tab::Choice{WorkAction::restock, std::nullopt, {1, 1}, 0, true};
    rejects([&] { tab::validate_choice(empty.colonies, empty.config.building_catalog, 0, invalid); });
    invalid.action = WorkAction::farm;
    invalid.box_targets.clear();
    rejects([&] { tab::validate_choice(empty.colonies, empty.config.building_catalog, 0, invalid); });
    auto no_active_fuel_cells = fixture();
    rejects([&] { tab::validate_choice(no_active_fuel_cells.colonies, no_active_fuel_cells.config.building_catalog, 0,
        tab::Choice{WorkAction::build, 0, {}, 0, true}); });

    auto duplicate = fixture();
    install_fuel_cells(duplicate, 0, 0, 0);
    duplicate.colonies.colonies[0].hand.erase(std::find(duplicate.colonies.colonies[0].hand.begin(),
        duplicate.colonies.colonies[0].hand.end(), base::Card{1}));
    duplicate.colonies.colonies[0].buildings.push_back(1);
    duplicate.colonies.colonies[0].boxes.emplace(1, 0);
    duplicate.config.building_catalog[1] = tab::Kind::fuel_cells;
    rejects([&] { tab::validate(duplicate.colonies, duplicate.config.building_catalog, scope); });

    auto recycled = fixture();
    install_fuel_cells(recycled, 0, 0, 1);
    recycled.colonies.building_discard = recycled.colonies.building_deck;
    recycled.colonies.building_deck.clear();
    const std::array<tab::Choice, 2> recycle_choices{
        tab::Choice{WorkAction::restock, std::nullopt, {0, 0}, 0, true}, {WorkAction::mine}};
    const auto recycle_record = tab::resolve(recycled.colonies, recycled.config.building_catalog,
        recycle_choices, {0, 1}, 9, scope);
    require(recycle_record.base.drawn[0].size() == 1 && recycled.colonies.building_shuffle_index == 1 &&
            recycled.colonies.building_deck.size() + recycled.colonies.building_discard.size() == 81,
            "Fuel Cells draw did not recycle the Building discard using existing draw rules");
}
void unsupported_draw_interleavings_are_atomic() {
    auto input = fixture();
    install_fuel_cells(input, 0, 0, 1);
    const std::array<tab::Choice, 2> research{tab::Choice{WorkAction::restock, std::nullopt, {0, 0}, 0, true},
                                              tab::Choice{WorkAction::research}};
    const auto before = input.colonies;
    rejects([&] { (void)tab::resolve(input.colonies, input.config.building_catalog, research, {0, 1}, 3, scope); });
    require(input.colonies == before, "unsupported Fuel Cells/Research combination partially mutated the state");

    auto both_input = fixture();
    install_fuel_cells(both_input, 0, 0, 1);
    install_fuel_cells(both_input, 1, 4, 1);
    both_input.config.building_catalog.emplace(4, tab::Kind::fuel_cells);
    const std::array<tab::Choice, 2> both{
        tab::Choice{WorkAction::restock, std::nullopt, {0, 0}, 0, true},
        tab::Choice{WorkAction::restock, std::nullopt, {4, 4}, 0, true}};
    const auto both_before = both_input.colonies;
    rejects([&] { (void)tab::resolve(both_input.colonies, both_input.config.building_catalog, both, {0, 1}, 3, scope); });
    require(both_input.colonies == both_before, "two-player Fuel Cells draw combination partially mutated state");
}
void replay_and_json_are_complete() {
    auto input = fixture();
    install_fuel_cells(input, 0, 0, 0);
    const auto commands = script();
    const auto replay = ses::record(input, commands);
    require(replay.schema == 15 && ses::verify(replay) == replay.checkpoints.back(),
            "Fuel Cells replay did not round trip with schema 15");
    auto tampered = replay;
    tampered.commands[1].fuel_cells_spend = false;
    rejects([&] { (void)ses::verify(tampered); });
    tampered = replay;
    tampered.checkpoints[3].tableau_work->base.drawn[0][0]++;
    rejects([&] { (void)ses::verify(tampered); });
    tampered = replay;
    tampered.commands.back().cards[0] = 1;
    rejects([&] { (void)ses::verify(tampered); });
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--json") {
            auto input = fixture();
            install_fuel_cells(input, 0, 0, 0);
            std::cout << ses::serialize_record(ses::record(input, script())) << '\n';
            return 0;
        }
        if (argc != 1) throw std::invalid_argument("unknown Fuel Cells test mode");
        metadata_options_and_same_restock_activation();
        existing_box_and_invalid_choices();
        unsupported_draw_interleavings_are_atomic();
        replay_and_json_are_complete();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "reviewed Fuel Cells contracts passed with synthetic scenarios\n";
}
