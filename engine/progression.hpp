#pragma once

#include "engine/core.hpp"

namespace bloodbath::progression {
inline constexpr const char* subset_manifest_hash = BLOODBATH_PROGRESSION_POPULATION_HASH;
using Card = std::uint32_t;
struct Snapshot {
    // Back is top. Opaque IDs are physical instances; no card effects are implied.
    std::vector<Card> deck;
    std::vector<Card> discard;
    std::optional<Card> active;
    std::uint64_t shuffle_index = 0;
    std::uint64_t addition_index = 0;
    EventClock clock;
    bool operator==(const Snapshot&) const = default;
};
struct Observation {
    std::optional<Card> current_or_top_discard;
    EventClock clock;
    bool operator==(const Observation&) const = default;
};
struct Reveal {
    Card card;
    EventClock clock;
    bool reshuffled;
    bool operator==(const Reveal&) const = default;
};
class Deck {
  public:
    Deck(Snapshot, std::uint64_t seed);
    Observation observe() const;
    std::optional<Reveal> reveal();
    // Call only after every triggered choice/effect of the active card is resolved.
    void finish_active();
    // Caller supplies one simultaneous addition group; cannot contain existing IDs.
    void add_group(std::vector<Card>);
    void record_decision();
    Snapshot snapshot() const { return state_; }
  private:
    Snapshot state_;
    std::uint64_t seed_;
};
} // namespace bloodbath::progression
