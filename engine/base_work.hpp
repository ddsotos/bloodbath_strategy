#pragma once

#include "engine/core.hpp"

namespace bloodbath::base_rules {
// Physical-card IDs are caller data, not an implemented official card catalog.
using Card = std::uint32_t;
struct Resources {
    std::uint64_t people;
    std::uint64_t money;
    std::uint64_t food;
    bool operator==(const Resources&) const = default;
};
constexpr Resources starting_resources() { return {30, 4, 4}; }
struct Colony {
    Resources resources = starting_resources();
    std::vector<Card> hand;
    std::vector<Card> buildings;
    bool operator==(const Colony&) const = default;
};
struct State {
    std::array<Colony, 2> colonies;
    // Back is top. Discard order is engine truth; agents see only its top.
    std::vector<Card> building_deck;
    std::vector<Card> building_discard;
    std::uint64_t building_shuffle_index = 0;
    bool operator==(const State&) const = default;
};
// Validates a supplied deal. Does not invent a card catalog or setup distribution.
State setup_fragment(std::array<std::vector<Card>, 2> hands, std::vector<Card> remainder);

enum class WorkAction : std::uint8_t { restock, build, mine, farm, research };
enum class EffectScope { unverified, caller_verified_unmodified };
struct Observation {
    Player player;
    std::array<Resources, 2> resources;
    std::vector<Card> own_hand;
    std::optional<Card> top_building_discard;
    // These are supported subset operations, NOT a full legal-action list.
    std::array<WorkAction, 3> supported_actions{WorkAction::mine, WorkAction::farm, WorkAction::research};
    bool operator==(const Observation&) const = default;
};
struct WorkRecord {
    std::array<WorkAction, 2> commitments;
    std::array<Player, 2> resolution_order;
    std::array<std::vector<Card>, 2> drawn;
    std::uint64_t seed;
    std::uint64_t shuffles_before;
    std::uint64_t shuffles_after;
    std::string subset_hash = BLOODBATH_BASE_RULES_HASH;
    bool operator==(const WorkRecord&) const = default;
};
struct BuildingDrawRecord {
    std::array<std::uint32_t, 2> requested;
    std::array<std::vector<Card>, 2> drawn;
    std::array<Player, 2> resolution_order;
    std::uint64_t seed;
    std::uint64_t shuffles_before;
    std::uint64_t shuffles_after;
    bool operator==(const BuildingDrawRecord&) const = default;
};
// Raw conditional draw instruction, not a Work action. Caller owns effect semantics.
BuildingDrawRecord draw_buildings(State&, std::array<std::uint32_t, 2> counts,
    std::array<Player, 2> explicit_schedule, std::uint64_t seed, EffectScope);
class WorkBatch {
  public:
    WorkBatch(State state, std::uint64_t seed, EffectScope scope);
    Observation observe(Player) const;
    void commit(Player, WorkAction);
    // External schedule is explicit and logged, not claimed as an official priority.
    WorkRecord resolve(std::array<Player, 2> resolution_order);
    State snapshot() const { return state_; }
  private:
    State state_;
    std::uint64_t seed_;
    std::array<std::optional<WorkAction>, 2> pending_;
    bool resolved_ = false;
};
// Re-run a conditional Work from a caller-retained engine snapshot; validates all fields.
State replay_work(State initial, EffectScope scope, const WorkRecord& record);
} // namespace bloodbath::base_rules
