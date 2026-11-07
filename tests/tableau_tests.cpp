#include "engine/session.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace base = bloodbath::base_rules;
namespace tab = bloodbath::tableau;
namespace session = bloodbath::session;
using bloodbath::Player;
using base::WorkAction;
using session::Effect;
using session::Phase;

namespace {
constexpr auto scope = base::EffectScope::caller_verified_unmodified;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class Exception = std::invalid_argument, class F> void rejects(F function) {
    try { function(); } catch (const Exception&) { return; }
    throw std::runtime_error("expected rejection");
}

// Every physical ID and supplied position below is synthetic. Mapping IDs to
// Kale Farm tests reviewed effects; it does not establish official multiplicities.
std::vector<base::Card> cards(base::Card first, base::Card end) {
    std::vector<base::Card> result;
    for (auto card = first; card < end; ++card) result.push_back(card);
    return result;
}
session::Input fixture() {
    session::Input input;
    input.colonies = base::setup_fragment({cards(0, 4), cards(4, 8)}, cards(8, 90));
    input.config.scope = scope;
    input.config.tableau_enabled = true;
    input.config.building_catalog = {{0, tab::Kind::kale_farm}, {1, tab::Kind::kale_farm},
                                     {4, tab::Kind::kale_farm}};
    input.config.effects = {{100, Effect::work}, {101, Effect::instruction_manual}};
    input.progress.deck = {101, 100};
    return input;
}
void place(base::State& state, Player p, base::Card card, std::uint64_t boxes = 0) {
    auto& colony = state.colonies[p];
    const auto found = std::find(colony.hand.begin(), colony.hand.end(), card);
    require(found != colony.hand.end(), "fixture Building was not in hand");
    colony.hand.erase(found);
    colony.buildings.push_back(card);
    colony.boxes.emplace(card, boxes);
}
void conserved(const base::State& state) {
    auto physical = state.building_deck;
    physical.insert(physical.end(), state.building_discard.begin(), state.building_discard.end());
    for (const auto& colony : state.colonies) {
        physical.insert(physical.end(), colony.hand.begin(), colony.hand.end());
        physical.insert(physical.end(), colony.buildings.begin(), colony.buildings.end());
        for (const auto& [card, count] : colony.boxes) {
            (void)count;
            require(std::find(colony.buildings.begin(), colony.buildings.end(), card) != colony.buildings.end(),
                    "boxes exist outside owned in-play Buildings");
        }
    }
    std::sort(physical.begin(), physical.end());
    require(physical == cards(0, 90), "physical Building identity changed");
}
tab::Choice simple(WorkAction action) { return {action}; }
tab::Choice build(base::Card card) { return {WorkAction::build, card}; }
tab::Choice restock(std::vector<base::Card> targets = {}) {
    return {WorkAction::restock, std::nullopt, std::move(targets)};
}
bool contains(const std::vector<tab::Choice>& choices, const tab::Choice& choice) {
    return std::find(choices.begin(), choices.end(), choice) != choices.end();
}

void build_play_and_other_work() {
    const auto input = fixture();
    auto state = input.colonies;
    const auto first = tab::resolve(state, input.config.building_catalog,
        {build(0), simple(WorkAction::mine)}, {0, 1}, 5, scope);
    require(state.colonies[0].resources == base::Resources{30, 2, 9},
            "first Kale Farm cost/base/PLAY Farm result wrong");
    require(state.colonies[0].buildings == std::vector<base::Card>{0} &&
                state.colonies[0].boxes.at(0) == 0 && state.colonies[0].hand == cards(1, 4),
            "BUILD did not move the selected card into play");
    require(tab::total_people(state.colonies[0], input.config.building_catalog) == 35,
            "printed population missing or double-counted");
    const std::vector<tab::EffectRecord> expected{
        {0, tab::EffectKind::pay_money, 0, 2}, {0, tab::EffectKind::enter_play, 0, 5},
        {0, tab::EffectKind::farm_base, std::nullopt, 4}, {0, tab::EffectKind::farm_bonus, 0, 1},
        {1, tab::EffectKind::mine_base, std::nullopt, 4}};
    require(first.effects == expected && first.choices[0] == build(0) &&
                first.subset_hash == BLOODBATH_TABLEAU_HASH,
            "effect record omitted new-card trigger or canonical chain");
    require(state.colonies[1].resources.money == 8, "Mine did not add four money");
    conserved(state);

    const auto second = tab::resolve(state, input.config.building_catalog,
        {build(1), simple(WorkAction::mine)}, {1, 0}, 5, scope);
    require(state.colonies[0].resources == base::Resources{30, 0, 15} &&
                state.colonies[0].buildings == std::vector<base::Card>{0, 1},
            "second Build did not include both own Farm triggers");
    require(second.effects.size() == 6 && tab::total_people(state.colonies[0], input.config.building_catalog) == 40,
            "second effect chain/printed population wrong");
    const auto research = tab::resolve(state, input.config.building_catalog,
        {simple(WorkAction::farm), simple(WorkAction::research)}, {0, 1}, 5, scope);
    require(state.colonies[0].resources.food == 21 &&
                research.base.drawn[1] == std::vector<base::Card>{89, 88},
            "Farm scaling or Research with Buildings wrong");
    require(research.effects.size() == 5, "Farm/Research effect rows missing");
    (void)tab::resolve(state, input.config.building_catalog,
        {restock({0, 0}), restock()}, {1, 0}, 5, scope);
    require(state.colonies[0].boxes.at(0) == 2 && state.colonies[1].boxes.empty(),
            "same-Building Restock or empty-tableau Restock wrong");
    (void)tab::resolve(state, input.config.building_catalog,
        {restock({0, 1}), simple(WorkAction::mine)}, {0, 1}, 5, scope);
    require(state.colonies[0].boxes.at(0) == 3 && state.colonies[0].boxes.at(1) == 1,
            "split Restock imposed a cap or misplaced boxes");
    conserved(state);
}

void enumeration_and_validation() {
    auto input = fixture();
    const auto initial = input.colonies;
    const auto options = tab::options(initial, input.config.building_catalog, 0);
    require(options.size() == 6 && contains(options, build(0)) && contains(options, build(1)) &&
                contains(options, restock()), "supported five-action enumeration incomplete");
    auto hidden = initial;
    std::swap(hidden.colonies[1].hand[0], hidden.building_deck[0]);
    std::reverse(hidden.building_deck.begin(), hidden.building_deck.end());
    require(tab::options(hidden, input.config.building_catalog, 0) == options,
            "options depend on opponent hand or future deck order");
    auto poor = initial;
    poor.colonies[0].resources.money = 1;
    const auto poor_options = tab::options(poor, input.config.building_catalog, 0);
    require(poor_options.size() == 4 && !contains(poor_options, build(0)), "unaffordable Build enumerated");
    auto built = initial;
    place(built, 0, 0);
    place(built, 0, 1);
    const auto allocations = tab::options(built, input.config.building_catalog, 0);
    require(contains(allocations, restock({0, 0})) && contains(allocations, restock({0, 1})) &&
                contains(allocations, restock({1, 1})), "canonical Restock allocations missing");

    for (const auto& bad : {build(4), build(2), tab::Choice{WorkAction::build},
            tab::Choice{WorkAction::build, 0, {0}}, restock({0}),
            tab::Choice{WorkAction::mine, 0}, tab::Choice{static_cast<WorkAction>(255)}}) {
        auto state = initial;
        rejects([&] { (void)tab::resolve(state, input.config.building_catalog,
            {bad, simple(WorkAction::mine)}, {1, 0}, 1, scope); });
        require(state == initial, "invalid choice partially mutated a Work batch");
    }
    rejects([&] { tab::validate_choice(poor, input.config.building_catalog, 0, build(0)); });
    rejects([&] { tab::validate_choice(built, input.config.building_catalog, 0, restock({0, 4})); });
    rejects([&] { tab::validate_choice(built, input.config.building_catalog, 0, restock()); });
    rejects([&] { (void)tab::options(initial, input.config.building_catalog, 2); });
    rejects([&] { tab::validate(initial, input.config.building_catalog, base::EffectScope::unverified); });
    auto unknown = initial;
    place(unknown, 0, 2);
    rejects([&] { tab::validate(unknown, input.config.building_catalog, scope); });
    auto bad_boxes = initial;
    bad_boxes.colonies[0].boxes[0] = 2;
    rejects([&] { tab::validate(bad_boxes, input.config.building_catalog, scope); });
    auto bad_catalog = input.config.building_catalog;
    bad_catalog[0] = static_cast<tab::Kind>(99);
    rejects([&] { tab::validate(initial, bad_catalog, scope); });
    bad_catalog = input.config.building_catalog;
    bad_catalog[999] = tab::Kind::kale_farm;
    rejects([&] { tab::validate(initial, bad_catalog, scope); });
    auto wrong_schedule = initial;
    rejects([&] { (void)tab::resolve(wrong_schedule, input.config.building_catalog,
        {build(0), simple(WorkAction::mine)}, {1, 1}, 1, scope); });
    require(wrong_schedule == initial, "invalid schedule changed state");
}

void secret_build_choices_and_public_tableau() {
    const auto input = fixture();
    session::Session left(input), right(input);
    require(left.work_options(0).empty(), "Work options available outside Work");
    (void)left.advance(); (void)right.advance();
    const auto before = left.observe(1);
    left.commit_choice(0, build(0));
    right.commit_choice(0, build(1));
    require(left.observe(1) == before && right.observe(1) == before,
            "opponent's face-down BUILD target leaked");
    require(left.work_options(0).empty() && left.snapshot().colonies == input.colonies,
            "BUILD committed resources before barrier or retained own open options");
    const auto locked = left.snapshot();
    rejects([&] { left.commit_choice(0, build(1)); });
    rejects([&] { (void)left.resolve_work({0, 1}); });
    require(left.snapshot() == locked, "early resolution/recommit changed secret target");
    left.commit(1, WorkAction::farm);
    (void)left.resolve_work({1, 0});
    const auto after = left.snapshot();
    require(after.tableau_work && !after.work_choices[0] && !after.work_choices[1] &&
                after.tableau_work->choices[0] == build(0), "resolved rich choices missing from audit record");
    require(left.observe(1).colony.public_buildings[0] == std::vector<base::Card>{0} &&
                left.observe(1).colony.public_boxes[0].at(0) == 0,
            "public Building/box state omitted after reveal");
    session::Session reverse(input);
    (void)reverse.advance();
    reverse.commit(1, WorkAction::farm);
    reverse.commit_choice(0, build(0));
    (void)reverse.resolve_work({1, 0});
    require(reverse.snapshot() == after, "submission order changed same locked Build result");

    auto legacy = input;
    legacy.config.tableau_enabled = false;
    legacy.config.building_catalog.clear();
    session::Session bare(legacy);
    (void)bare.advance();
    rejects([&] { bare.commit_choice(0, build(0)); });
    rejects([&] { bare.commit(0, WorkAction::build); });
    rejects([&] { bare.commit(0, WorkAction::restock); });
    place(legacy.colonies, 0, 0);
    rejects([&] { (void)session::Session(legacy); });
}

void overflow_atomicity() {
    auto input = fixture();
    auto state = input.colonies;
    state.colonies[0].resources.food = std::numeric_limits<std::uint64_t>::max() - 4;
    auto before = state;
    rejects<std::overflow_error>([&] { (void)tab::resolve(state, input.config.building_catalog,
        {build(0), simple(WorkAction::mine)}, {1, 0}, 0, scope); });
    require(state == before, "PLAY bonus overflow committed cost/Building/other player's action");
    state = input.colonies;
    place(state, 0, 0, std::numeric_limits<std::uint64_t>::max() - 1);
    before = state;
    rejects<std::overflow_error>([&] { (void)tab::resolve(state, input.config.building_catalog,
        {restock({0, 0}), simple(WorkAction::mine)}, {1, 0}, 0, scope); });
    require(state == before, "second box overflow committed first placement or other action");
    state = input.colonies;
    state.colonies[0].resources.people = std::numeric_limits<std::uint64_t>::max() - 4;
    before = state;
    rejects<std::overflow_error>([&] { (void)tab::resolve(state, input.config.building_catalog,
        {build(0), simple(WorkAction::mine)}, {1, 0}, 0, scope); });
    require(state == before, "standalone Build total overflow committed a new Building");

    input.colonies.colonies[0].resources.people =
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) - 4;
    session::Session capped(input);
    (void)capped.advance();
    capped.commit_choice(0, build(0));
    capped.commit(1, WorkAction::mine);
    const auto pending = capped.snapshot();
    rejects<std::overflow_error>([&] { (void)capped.resolve_work({1, 0}); });
    require(capped.snapshot() == pending, "score-range overflow partially committed a Build");
    state = fixture().colonies;
    place(state, 0, 0);
    state.colonies[0].resources.people = std::numeric_limits<std::uint64_t>::max();
    rejects<std::overflow_error>([&] { (void)tab::total_people(state.colonies[0], input.config.building_catalog); });
}

void building_population_guards_and_paperwork() {
    auto input = fixture();
    place(input.colonies, 0, 0, 2);
    input.colonies.colonies[0].resources.people = 0; // Five printed people still live.
    session::Session living(input);
    (void)living.advance();
    living.commit(0, WorkAction::research);
    living.commit(1, WorkAction::mine);
    const auto work = living.resolve_work({0, 1});
    require(work.drawn[0] == std::vector<base::Card>{89, 88} &&
                living.snapshot().colonies.colonies[0].resources.people == 0,
            "zero base was confused with colony death or blocked Research");
    require(living.advance() == Phase::official_terminal &&
                living.snapshot().official->scores == bloodbath::Scores{5, 30} &&
                !living.snapshot().deaths[0], "Instruction Manual did not include printed population");

    auto paperwork_input = fixture();
    place(paperwork_input.colonies, 0, 0, 2);
    paperwork_input.progress.deck = {200};
    paperwork_input.config.effects[200] = Effect::paperwork;
    session::Session paper(paperwork_input);
    require(paper.advance() == Phase::paperwork_draw_order &&
                paper.prepare_paperwork({0, 1}) == Phase::paperwork_discard,
            "Kale-only Paperwork draw frame rejected supported Buildings");
    paper.commit_discards(0, {1, 89});
    paper.commit_discards(1, {4, 88});
    (void)paper.resolve_paperwork({1, 0});
    require(paper.snapshot().colonies.colonies[0].buildings == std::vector<base::Card>{0} &&
                paper.snapshot().colonies.colonies[0].boxes.at(0) == 2 &&
                paper.snapshot().colonies.colonies[0].resources == paperwork_input.colonies.colonies[0].resources,
            "hand discard affected Building population, boxes or triggers");
    conserved(paper.snapshot().colonies);
}

void replay_rich_targets_and_effects() {
    auto input = fixture();
    input.progress.deck = {101, 102, 100};
    input.config.effects[102] = Effect::work;
    const std::vector<session::Command> commands{
        {session::CommandKind::advance},
        {session::CommandKind::commit_choice, 0, WorkAction::build, {0, 1}, {0}},
        {session::CommandKind::commit, 1, WorkAction::mine},
        {session::CommandKind::resolve_work, 0, WorkAction::mine, {1, 0}},
        {session::CommandKind::advance},
        {session::CommandKind::commit_choice, 0, WorkAction::restock, {0, 1}, {0, 0}},
        {session::CommandKind::commit, 1, WorkAction::research},
        {session::CommandKind::resolve_work}, {session::CommandKind::advance}};
    for (std::uint64_t seed = 0; seed < 32; ++seed) {
        input.config.seed = seed;
        const auto replay = session::record(input, commands);
        require(replay.schema == 10 && replay == session::record(input, commands), "rich Work trace not reproducible");
        const auto final = session::verify(replay);
        require(final == replay.checkpoints.back() && final.official &&
                    final.official->scores == bloodbath::Scores{35, 30} &&
                    final.colonies.colonies[0].boxes.at(0) == 2,
                "replayed Build/Restock/terminal result diverged");
        conserved(final.colonies);
        auto changed = replay;
        changed.schema = 3;
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.checkpoints[1].work_choices[0]->build = 1;
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.checkpoints[3].tableau_work->effects[0].amount++;
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.commands[5].cards = {0, 1};
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.checkpoints.back().colonies.colonies[0].boxes[0]++;
        rejects([&] { (void)session::verify(changed); });
    }
}
} // namespace

int main() {
    try {
        build_play_and_other_work();
        enumeration_and_validation();
        secret_build_choices_and_public_tableau();
        overflow_atomicity();
        building_population_guards_and_paperwork();
        replay_rich_targets_and_effects();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "reviewed Tableau contracts passed with synthetic physical-card fixtures\n";
}
