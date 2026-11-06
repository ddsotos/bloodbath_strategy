#include "engine/base_work.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace bloodbath::base_rules {
namespace {
void check_player(Player player) {
    if (player > 1) throw std::invalid_argument("invalid player");
}
void validate_cards(const State& state) {
    std::set<Card> cards;
    const auto add = [&](const std::vector<Card>& zone) {
        for (const auto card : zone)
            if (!cards.insert(card).second) throw std::invalid_argument("duplicate physical card ID");
    };
    for (const auto& colony : state.colonies) {
        add(colony.hand); add(colony.buildings);
        for (const auto& [card, count] : colony.boxes) {
            (void)count;
            if (std::find(colony.buildings.begin(), colony.buildings.end(), card) == colony.buildings.end())
                throw std::invalid_argument("boxes must belong to an owned Building");
        }
    }
    add(state.building_deck);
    add(state.building_discard);
    if (cards.size() != 90) throw std::invalid_argument("base catalog must contain 90 physical IDs");
}
void add_four(std::uint64_t& value) {
    if (value > std::numeric_limits<std::uint64_t>::max() - 4)
        throw std::overflow_error("resource overflow");
    value += 4;
}
std::optional<Card> draw(State& state, const RandomStreams& rng) {
    if (state.building_deck.empty() && !state.building_discard.empty()) {
        if (state.building_shuffle_index == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("shuffle index overflow");
        state.building_deck = std::move(state.building_discard);
        state.building_discard.clear();
        ++state.building_shuffle_index;
        for (std::size_t size = state.building_deck.size(); size > 1; --size) {
            const auto j = rng.bounded(RandomSource::building, state.building_shuffle_index,
                                       static_cast<std::uint64_t>(size), size);
            std::swap(state.building_deck[size - 1], state.building_deck[static_cast<std::size_t>(j)]);
        }
    }
    if (state.building_deck.empty()) return std::nullopt;
    const auto card = state.building_deck.back();
    state.building_deck.pop_back();
    return card;
}
} // namespace
State setup_fragment(std::array<std::vector<Card>, 2> hands, std::vector<Card> remainder) {
    if (hands[0].size() != 4 || hands[1].size() != 4 || remainder.size() != 82)
        throw std::invalid_argument("setup fragment requires two hands of four and 82 remaining cards");
    State state;
    for (Player p = 0; p < 2; ++p) state.colonies[p].hand = std::move(hands[p]);
    state.building_deck = std::move(remainder);
    validate_cards(state);
    return state;
}
BuildingDrawRecord draw_buildings(State& state, std::array<std::uint32_t, 2> counts,
    std::array<Player, 2> schedule, std::uint64_t seed, EffectScope scope) {
    (void)WorkBatch(state, seed, scope); // Reuse the certified context/card boundary.
    return draw_buildings(state, counts, schedule, seed, DrawScope::caller_verified_unmodified_draw);
}
BuildingDrawRecord draw_buildings(State& state, std::array<std::uint32_t, 2> counts,
    std::array<Player, 2> schedule, std::uint64_t seed, DrawScope scope) {
    if (scope != DrawScope::caller_verified_unmodified_draw)
        throw std::invalid_argument("unverified draw modifiers");
    validate_cards(state);
    for (const auto& colony : state.colonies)
        if (!colony.resources.people && colony.buildings.empty())
            throw std::invalid_argument("draw requires a caller-certified living colony");
    if (!((schedule[0] == 0 && schedule[1] == 1) || (schedule[0] == 1 && schedule[1] == 0)))
        throw std::invalid_argument("draw schedule must be a player permutation");
    auto next = state;
    BuildingDrawRecord record{counts, {}, schedule, seed, state.building_shuffle_index,
                              state.building_shuffle_index};
    const RandomStreams rng(seed);
    for (const Player p : schedule)
        for (std::uint32_t i = 0; i < counts[p]; ++i) {
            const auto card = draw(next, rng);
            if (!card) break;
            next.colonies[p].hand.push_back(*card);
            record.drawn[p].push_back(*card);
        }
    record.shuffles_after = next.building_shuffle_index;
    state = std::move(next);
    return record;
}
WorkBatch::WorkBatch(State state, std::uint64_t seed, EffectScope scope)
    : state_(std::move(state)), seed_(seed) {
    if (scope != EffectScope::caller_verified_unmodified)
        throw std::invalid_argument("unverified action modifiers: subset cannot resolve this Work");
    for (const auto& colony : state_.colonies)
        if (!colony.buildings.empty() || !colony.resources.people)
            throw std::invalid_argument("subset requires living colonies without buildings");
    validate_cards(state_);
}
Observation WorkBatch::observe(Player p) const {
    check_player(p);
    std::optional<Card> top;
    if (!state_.building_discard.empty()) top = state_.building_discard.back();
    Observation observation{p, {state_.colonies[0].resources, state_.colonies[1].resources},
                            state_.colonies[p].hand, top};
    observation.public_buildings = {state_.colonies[0].buildings, state_.colonies[1].buildings};
    observation.public_boxes = {state_.colonies[0].boxes, state_.colonies[1].boxes};
    return observation;
}
void WorkBatch::commit(Player p, WorkAction action) {
    check_player(p);
    if (resolved_ || pending_[p]) throw std::invalid_argument("closed or duplicate commitment");
    if (action != WorkAction::mine && action != WorkAction::farm && action != WorkAction::research)
        throw std::invalid_argument("unsupported subset action, not an official illegality");
    pending_[p] = action;
}
WorkRecord WorkBatch::resolve(std::array<Player, 2> order) {
    if (resolved_ || !pending_[0] || !pending_[1])
        throw std::invalid_argument("both commitments must be locked before resolution");
    if (!((order[0] == 0 && order[1] == 1) || (order[0] == 1 && order[1] == 0)))
        throw std::invalid_argument("resolution order must be a player permutation");
    auto next = state_; // Commit atomically only after all validations/allocations succeed.
    WorkRecord record{{*pending_[0], *pending_[1]}, order, {}, seed_,
                      state_.building_shuffle_index, state_.building_shuffle_index};
    const RandomStreams rng(seed_);
    for (const Player p : order) {
        switch (*pending_[p]) {
        case WorkAction::mine: add_four(next.colonies[p].resources.money); break;
        case WorkAction::farm: add_four(next.colonies[p].resources.food); break;
        case WorkAction::research:
            for (int i = 0; i < 2; ++i) {
                const auto card = draw(next, rng);
                if (!card) break; // p8: perform as much as possible.
                next.colonies[p].hand.push_back(*card);
                record.drawn[p].push_back(*card);
            }
            break;
        default: throw std::logic_error("unsupported locked action");
        }
    }
    record.shuffles_after = next.building_shuffle_index;
    state_ = std::move(next);
    resolved_ = true;
    return record;
}
State replay_work(State initial, EffectScope scope, const WorkRecord& record) {
    if (record.subset_hash != BLOODBATH_BASE_RULES_HASH)
        throw std::invalid_argument("base subset manifest mismatch");
    WorkBatch batch(std::move(initial), record.seed, scope);
    for (Player p = 0; p < 2; ++p) batch.commit(p, record.commitments[p]);
    if (batch.resolve(record.resolution_order) != record)
        throw std::invalid_argument("base Work replay mismatch");
    return batch.snapshot();
}
} // namespace bloodbath::base_rules
