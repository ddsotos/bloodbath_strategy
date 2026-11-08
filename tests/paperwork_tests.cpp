#include "engine/session.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace base = bloodbath::base_rules;
namespace session = bloodbath::session;
using bloodbath::EventClock;
using bloodbath::Player;
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

// Physical IDs, input positions and Progress sequences are synthetic fixtures,
// not an official card catalog, legal setup distribution or card-effect definition.
std::vector<base::Card> cards(base::Card first, base::Card end) {
    std::vector<base::Card> result;
    for (auto card = first; card < end; ++card) result.push_back(card);
    return result;
}
session::Input fixture() {
    session::Input input;
    input.colonies = base::setup_fragment({cards(0, 4), cards(4, 8)}, cards(8, 90));
    input.config.seed = 42;
    input.config.scope = scope;
    input.config.effects = {{200, session::Effect::paperwork}, {201, session::Effect::paperwork}};
    input.progress.deck = {200};
    return input;
}
std::vector<base::Card> all_cards(const base::State& state) {
    auto result = state.building_deck;
    result.insert(result.end(), state.building_discard.begin(), state.building_discard.end());
    for (const auto& colony : state.colonies) {
        result.insert(result.end(), colony.hand.begin(), colony.hand.end());
        result.insert(result.end(), colony.buildings.begin(), colony.buildings.end());
    }
    std::sort(result.begin(), result.end());
    return result;
}
void conserved(const base::State& state) {
    require(all_cards(state) == cards(0, 90), "Paperwork lost, duplicated or fabricated a physical card");
}
void unchanged_resources(const base::State& before, const base::State& after) {
    for (Player p = 0; p < 2; ++p)
        require(before.colonies[p].resources == after.colonies[p].resources &&
                    before.colonies[p].buildings == after.colonies[p].buildings,
                "hand discard changed population, resources or in-play Buildings");
}
void no_advance(session::Session& run, Phase phase) {
    const auto before = run.snapshot();
    for (int i = 0; i < 3; ++i)
        require(run.advance() == phase && run.snapshot() == before, "pending Paperwork was skipped");
}

void draw_then_private_commit_then_discard() {
    const auto initial = fixture();
    session::Session run(initial);
    rejects([&] { (void)run.prepare_paperwork({0, 1}); });
    rejects([&] { run.commit_discards(0, {0, 1}); });
    rejects([&] { (void)run.resolve_paperwork({0, 1}); });
    require(run.advance() == Phase::paperwork_draw_order, "Paperwork did not wait for draw schedule");
    const auto revealed = run.snapshot();
    require(revealed.colonies == initial.colonies && !revealed.paperwork &&
                revealed.progress.clock == EventClock{1, 0, 1, 0},
            "Paperwork reveal drew cards or counted schedule as a policy decision");
    no_advance(run, Phase::paperwork_draw_order);
    rejects([&] { run.commit_discards(0, {0, 1}); });
    rejects([&] { run.commit(0, base::WorkAction::mine); });
    rejects([&] { (void)run.resolve_paperwork({0, 1}); });
    rejects([&] { (void)run.prepare_paperwork({0, 0}); });
    rejects([&] { (void)run.prepare_paperwork({2, 0}); });
    require(run.snapshot() == revealed, "invalid draw schedule changed state");
    require(run.prepare_paperwork({1, 0}) == Phase::paperwork_discard, "Paperwork did not open discard choices");
    const auto drawn = run.snapshot();
    require(drawn.paperwork && !drawn.paperwork->completed &&
                drawn.paperwork->draws.requested == std::array<std::uint32_t, 2>{1, 1} &&
                drawn.paperwork->draws.resolution_order == std::array<Player, 2>{1, 0} &&
                drawn.paperwork->draws.drawn[0] == std::vector<base::Card>{88} &&
                drawn.paperwork->draws.drawn[1] == std::vector<base::Card>{89} &&
                drawn.paperwork->draws.seed == 42,
            "one-card private draws or explicit schedule not retained");
    require(drawn.progress.clock == EventClock{1, 0, 1, 1} &&
                drawn.colonies.building_discard.empty(), "discard decision clock or barrier wrong");
    require(run.observe(0).required_discards == 2 && run.observe(1).required_discards == 2,
            "discard quotas do not describe post-draw hands");
    no_advance(run, Phase::paperwork_discard);
    rejects([&] { (void)run.prepare_paperwork({0, 1}); });
    rejects([&] { (void)run.resolve_paperwork({0, 1}); });
    rejects([&] { run.commit_discards(0, {0}); });
    rejects([&] { run.commit_discards(0, {0, 1, 2}); });
    rejects([&] { run.commit_discards(0, {0, 0}); });
    rejects([&] { run.commit_discards(0, {0, 4}); });
    rejects([&] { run.commit_discards(2, {0, 1}); });
    require(run.snapshot() == drawn, "invalid discard selection changed state");
    const auto other_observation = run.observe(1);
    run.commit_discards(0, {0, 88}); // Newly drawn cards are eligible.
    require(run.observe(1) == other_observation && run.snapshot().colonies == drawn.colonies,
            "private discard commit leaked or applied before both locked");
    const auto first_locked = run.snapshot();
    rejects([&] { run.commit_discards(0, {1, 2}); });
    rejects([&] { (void)run.resolve_paperwork({0, 1}); });
    require(run.snapshot() == first_locked, "duplicate/early operation replaced first commitment");
    run.commit_discards(1, {4, 89});
    const auto locked = run.snapshot();
    require(locked.colonies == drawn.colonies, "second commitment applied before resolve");
    rejects([&] { (void)run.resolve_paperwork({1, 1}); });
    rejects([&] { (void)run.resolve_paperwork({0, 2}); });
    require(run.snapshot() == locked, "invalid discard schedule partly applied a batch");
    const auto record = run.resolve_paperwork({1, 0});
    const auto finished = run.snapshot();
    require(record.completed && record.discard_order == std::array<Player, 2>{1, 0} &&
                record.discarded[0] == std::vector<base::Card>{0, 88} &&
                record.discarded[1] == std::vector<base::Card>{4, 89},
            "discard record does not preserve both selected batches/order");
    require(finished.colonies.building_discard == std::vector<base::Card>{4, 89, 0, 88} &&
                run.observe(0).colony.top_building_discard == base::Card{88},
            "explicit discard order did not determine public top card");
    require(finished.phase == Phase::ready && !finished.progress.active &&
                !finished.discard_commitments[0] && !finished.discard_commitments[1] &&
                finished.paperwork == record && finished.progress.clock == EventClock{1, 0, 1, 1},
            "Paperwork did not close choices/card without extra clock ticks");
    require(finished.colonies.colonies[0].hand == std::vector<base::Card>{1, 2, 3} &&
                finished.colonies.colonies[1].hand == std::vector<base::Card>{5, 6, 7},
            "discard did not remove exactly the selected hand cards");
    unchanged_resources(initial.colonies, finished.colonies);
    conserved(finished.colonies);
    rejects([&] { run.commit_discards(0, {1, 2}); });
    rejects([&] { (void)run.resolve_paperwork({0, 1}); });
    require(run.snapshot() == finished, "completed Paperwork accepted another choice");

    session::Session reversed(initial);
    (void)reversed.advance();
    (void)reversed.prepare_paperwork({1, 0});
    reversed.commit_discards(1, {4, 89});
    reversed.commit_discards(0, {0, 88});
    require(reversed.resolve_paperwork({1, 0}) == record && reversed.snapshot() == finished,
            "commit submission order affected explicitly scheduled discard result");
}

void partial_quota_and_repeated_paperwork() {
    auto input = fixture();
    input.colonies.colonies[0].hand.clear();
    input.colonies.colonies[1].hand = cards(0, 88);
    input.colonies.building_deck = {88, 89};
    session::Session partial(input);
    (void)partial.advance();
    (void)partial.prepare_paperwork({0, 1});
    require(partial.observe(0).required_discards == 1 && partial.observe(1).required_discards == 2,
            "partial quota did not use actual post-draw hand size");
    rejects([&] { partial.commit_discards(0, {}); });
    rejects([&] { partial.commit_discards(0, {89, 0}); });
    partial.commit_discards(0, {89});
    partial.commit_discards(1, {0, 88});
    (void)partial.resolve_paperwork({0, 1});
    require(partial.snapshot().colonies.colonies[0].hand.empty(), "one-card hand was not fully discarded");
    conserved(partial.snapshot().colonies);

    input = fixture();
    input.progress.deck = {201, 200};
    session::Session repeat(input);
    (void)repeat.advance();
    (void)repeat.prepare_paperwork({0, 1});
    repeat.commit_discards(0, {0, 89});
    repeat.commit_discards(1, {4, 88});
    (void)repeat.resolve_paperwork({0, 1});
    require(repeat.snapshot().paperwork->completed, "first repeated effect incomplete");
    require(repeat.advance() == Phase::paperwork_draw_order && !repeat.snapshot().paperwork &&
                !repeat.snapshot().discard_commitments[0] && !repeat.snapshot().discard_commitments[1],
            "next Paperwork retained old choice/result as pending");
    (void)repeat.prepare_paperwork({0, 1});
    require(repeat.snapshot().progress.clock == EventClock{2, 0, 2, 2},
            "repeated Paperwork failed to count one new simultaneous decision");
    repeat.commit_discards(0, {1, 87});
    repeat.commit_discards(1, {5, 86});
    const auto second = repeat.resolve_paperwork({1, 0});
    require(second.discarded[0] == std::vector<base::Card>{1, 87} &&
                second.discarded[1] == std::vector<base::Card>{5, 86}, "old Paperwork selections reused");
    conserved(repeat.snapshot().colonies);
}

void scarce_supply_stops_before_all_draws() {
    for (int supply = 0; supply < 2; ++supply) {
        for (const bool use_discard : {false, true}) {
            auto input = fixture();
            input.colonies.colonies[0].hand = cards(0, 4);
            input.colonies.colonies[1].hand = cards(4, static_cast<base::Card>(90 - supply));
            input.colonies.building_deck.clear();
            input.colonies.building_discard.clear();
            if (supply == 1) {
                auto& zone = use_discard ? input.colonies.building_discard : input.colonies.building_deck;
                zone = {89};
            }
            session::Session run(input);
            (void)run.advance();
            const auto before = run.snapshot();
            require(run.prepare_paperwork({1, 0}) == Phase::blocked_shared_draw,
                    "scarce supply silently chose cross-player discard/draw timing");
            const auto blocked = run.snapshot();
            require(blocked.colonies == before.colonies && blocked.progress == before.progress &&
                        blocked.progress.active == bloodbath::progression::Card{200} &&
                        !blocked.paperwork && !blocked.official && !blocked.terminal_evidence &&
                        !blocked.deaths[0] && !blocked.deaths[1],
                    "scarce supply partly drew, changed clock or invented terminal evidence");
            no_advance(run, Phase::blocked_shared_draw);
            rejects([&] { run.commit_discards(0, {0, 1}); });
            rejects([&] { (void)run.prepare_paperwork({0, 1}); });
            rejects([&] { (void)run.resolve_paperwork({0, 1}); });
            conserved(blocked.colonies);
        }
    }
}

session::Command command(session::CommandKind kind, Player player = 0,
                         std::array<Player, 2> schedule = {0, 1},
                         std::vector<base::Card> selected = {}) {
    return {kind, player, base::WorkAction::mine, schedule, std::move(selected)};
}
void recycle_and_replay() {
    auto input = fixture();
    input.colonies.colonies[0].hand = cards(0, 4);
    input.colonies.colonies[1].hand = cards(4, 85);
    input.colonies.building_deck = {85};
    input.colonies.building_discard = {86, 87, 88, 89};
    const std::vector<session::Command> commands{
        command(session::CommandKind::advance), command(session::CommandKind::prepare_paperwork),
        command(session::CommandKind::commit_discards, 0, {0, 1}, {0, 85}),
        command(session::CommandKind::commit_discards, 1, {0, 1}, {4, 5}),
        command(session::CommandKind::resolve_paperwork, 0, {1, 0})};
    std::set<base::Card> second_draws;
    for (std::uint64_t seed = 0; seed < 32; ++seed) {
        input.config.seed = seed;
        const auto replay = session::record(input, commands);
        require(replay.schema == 13 && replay == session::record(input, commands),
                "Paperwork schema or repeated recording wrong");
        const auto final = session::verify(replay);
        require(final == replay.checkpoints.back() && final.paperwork->completed &&
                    final.paperwork->draws.drawn[0] == std::vector<base::Card>{85} &&
                    final.paperwork->draws.drawn[1].size() == 1 &&
                    final.paperwork->draws.shuffles_before == 0 &&
                    final.paperwork->draws.shuffles_after == 1 &&
                    final.colonies.building_shuffle_index == 1,
                "mid-draw recycle or complete replay result wrong");
        second_draws.insert(final.paperwork->draws.drawn[1][0]);
        conserved(final.colonies);
        unchanged_resources(input.colonies, final.colonies);
        auto changed = replay;
        changed.schema = 1;
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.checkpoints[2].discard_commitments[0] = std::vector<base::Card>{1, 85};
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.checkpoints[1].paperwork->draws.drawn[0][0] = 86;
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.checkpoints.back().paperwork->discard_order = {0, 1};
        rejects([&] { (void)session::verify(changed); });
        changed = replay;
        changed.commands[2].cards = {1, 85};
        rejects([&] { (void)session::verify(changed); });
    }
    require(second_draws.size() > 1, "recycled draws ignored seed");

    auto deferred = fixture();
    deferred.colonies.colonies[1].hand = cards(4, 85);
    deferred.colonies.building_deck = {85, 86};
    deferred.colonies.building_discard = {87, 88, 89};
    session::Session no_early_shuffle(deferred);
    (void)no_early_shuffle.advance();
    (void)no_early_shuffle.prepare_paperwork({0, 1});
    require(no_early_shuffle.snapshot().colonies.building_deck.empty() &&
                no_early_shuffle.snapshot().colonies.building_discard == std::vector<base::Card>{87, 88, 89} &&
                no_early_shuffle.snapshot().paperwork->draws.shuffles_after == 0,
            "last second draw prematurely recycled old discards");
}

void raw_draw_batch_guards() {
    auto state = fixture().colonies;
    const auto initial = state;
    rejects([&] { (void)base::draw_buildings(state, {1, 1}, {0, 0}, 1, scope); });
    require(state == initial, "invalid raw draw schedule mutated input");
    rejects([&] { (void)base::draw_buildings(state, {1, 1}, {0, 1}, 1, base::EffectScope::unverified); });
    require(state == initial, "unverified raw draw scope mutated input");
    const auto record = base::draw_buildings(state, {2, 1}, {0, 1}, 7, scope);
    require(record.drawn[0] == std::vector<base::Card>{89, 88} &&
                record.drawn[1] == std::vector<base::Card>{87} &&
                record.requested == std::array<std::uint32_t, 2>{2, 1} &&
                record.seed == 7 && record.shuffles_before == 0 && record.shuffles_after == 0,
            "generic requested draw batch/result wrong");
    conserved(state);
    unchanged_resources(initial, state);

    auto overflow = fixture().colonies;
    overflow.colonies[1].hand = cards(4, 88);
    overflow.building_deck = {88};
    overflow.building_discard = {89};
    overflow.building_shuffle_index = std::numeric_limits<std::uint64_t>::max();
    const auto saved = overflow;
    rejects<std::overflow_error>([&] { (void)base::draw_buildings(overflow, {1, 1}, {0, 1}, 0, scope); });
    require(overflow == saved, "second-player shuffle overflow committed first-player draw");

    auto input = fixture();
    input.colonies = saved;
    session::Session run(input);
    (void)run.advance();
    const auto before = run.snapshot();
    rejects<std::overflow_error>([&] { (void)run.prepare_paperwork({0, 1}); });
    require(run.snapshot() == before, "Paperwork preparation overflow changed phase, cards or decision clock");

    auto with_building = fixture();
    with_building.colonies.colonies[0].buildings.push_back(with_building.colonies.colonies[0].hand.back());
    with_building.colonies.colonies[0].hand.pop_back();
    rejects([&] { (void)session::Session(with_building); });
}
} // namespace

int main() {
    try {
        draw_then_private_commit_then_discard();
        partial_quota_and_repeated_paperwork();
        scarce_supply_stops_before_all_draws();
        recycle_and_replay();
        raw_draw_batch_guards();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional Paperwork contracts passed with synthetic supplied-state fixtures\n";
}
