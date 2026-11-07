#pragma once

#include "engine/base_work.hpp"
#include "engine/population.hpp"

namespace bloodbath::tableau {
inline constexpr const char* manifest_hash = BLOODBATH_TABLEAU_HASH;
using Card = base_rules::Card;
enum class Kind { kale_farm };
using Catalog = std::map<Card, Kind>;
struct Choice {
    base_rules::WorkAction action = base_rules::WorkAction::mine;
    std::optional<Card> build = std::nullopt;
    std::vector<Card> box_targets{};
    bool operator==(const Choice&) const = default;
};
enum class EffectKind { pay_money, enter_play, farm_base, farm_bonus, place_box, mine_base, draw_card };
struct EffectRecord {
    Player player;
    EffectKind kind;
    std::optional<Card> card;
    std::uint64_t amount;
    bool operator==(const EffectRecord&) const = default;
};
struct WorkRecord {
    base_rules::WorkRecord base;
    std::array<Choice, 2> choices;
    std::vector<EffectRecord> effects;
    std::string subset_hash = manifest_hash;
    bool operator==(const WorkRecord&) const = default;
};
void validate(const base_rules::State&, const Catalog&, base_rules::EffectScope);
std::uint64_t total_people(const base_rules::Colony&, const Catalog&);
// Pure choice queries require an already-validated reviewed context.
std::vector<Choice> options(const base_rules::State&, const Catalog&, Player);
void validate_choice(const base_rules::State&, const Catalog&, Player, const Choice&);
// Atomic single-colony nonterminal loss. The caller supplies the complete ordered
// Building choices; incomplete/excess choices and total <= loss reject unchanged.
population::Result lose_people(base_rules::State&, const Catalog&, Player, std::uint64_t amount,
                               const std::vector<Card>& chosen_buildings, EventClock,
                               std::string cause, base_rules::EffectScope);
// Both choices must already be locked by the caller. Explicit order remains a convention.
WorkRecord resolve(base_rules::State&, const Catalog&, const std::array<Choice, 2>&,
                   std::array<Player, 2> schedule, std::uint64_t seed, base_rules::EffectScope);
} // namespace bloodbath::tableau
