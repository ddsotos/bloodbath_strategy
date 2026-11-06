#pragma once

#include "engine/core.hpp"

namespace bloodbath::population {
inline constexpr const char* subset_manifest_hash = BLOODBATH_PROGRESSION_POPULATION_HASH;
using Card = std::uint32_t;
enum class LostEffects { unverified, caller_verified_none };
struct Building {
    Card card;
    std::uint64_t printed_people;
    std::uint64_t boxes;
    LostEffects lost_effects = LostEffects::unverified;
    bool operator==(const Building&) const = default;
};
struct Colony {
    std::uint64_t base_people;
    std::vector<Building> buildings;
    std::vector<Card> discarded_buildings;
    bool operator==(const Colony&) const = default;
};
enum class Status { needs_building_choice, needs_terminal_rule, paid, failed };
struct LostBuilding {
    Card card;
    std::uint64_t transferred_people;
    std::uint64_t discarded_boxes;
    bool operator==(const LostBuilding&) const = default;
};
struct Result {
    Status status;
    std::uint64_t requested_loss;
    std::uint64_t paid;
    std::uint64_t unpaid_shortfall;
    std::optional<DeathRecord> death;
    std::vector<LostBuilding> lost;
    bool operator==(const Result&) const = default;
};
std::uint64_t total_people(const Colony&);
// Requires living input. Conditional single-colony loss, not whole-card end ordering.
class Loss {
  public:
    Loss(Colony, Player, std::uint64_t amount, EventClock, std::string cause);
    Status status() const { return result_.status; }
    std::vector<Card> choices() const;
    void choose_building(Card);
    Colony snapshot() const { return colony_; }
    const Result& result() const { return result_; }
  private:
    void advance();
    Colony colony_;
    Player player_;
    EventClock clock_;
    std::string cause_;
    Result result_;
};
} // namespace bloodbath::population
