#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace bloodbath {
inline constexpr std::uint32_t schema_version = 1;
using Player = std::uint8_t;
using Scores = std::array<std::int64_t, 2>;

struct EventClock {
    std::uint64_t absolute_event = 0;
    std::uint64_t cycle = 0;
    std::uint64_t position = 0;
    std::uint64_t decision = 0;
    bool operator==(const EventClock&) const = default;
};
struct DeathRecord {
    Player player;
    EventClock clock;
    std::string cause;
    std::int64_t deficit;
    bool operator==(const DeathRecord&) const = default;
};
enum class Winner : std::uint8_t { player0, player1, draw };
struct OfficialOutcome {
    EventClock terminal;
    Scores scores;
    Winner winner;
    std::string cause;
    bool operator==(const OfficialOutcome&) const = default;
};
struct ShadowOutcome {
    EventClock horizon;
    Scores scores;
    std::array<std::optional<DeathRecord>, 2> deaths;
    std::string policy_version;
    std::string adapter_version;
    Scores synthetic_adjustments{};
    bool operator==(const ShadowOutcome&) const = default;
};

// Event-addressed SplitMix64: no mutable RNG cursor and no std distribution dependency.
enum class RandomSource : std::uint64_t { setup, progression, building, action, shadow };
class RandomStreams {
  public:
    explicit RandomStreams(std::uint64_t seed) : seed_(seed) {}
    std::uint64_t sample(RandomSource source, std::uint64_t event, std::uint64_t draw) const;
    std::uint64_t bounded(RandomSource source, std::uint64_t event, std::uint64_t draw,
                          std::uint64_t bound) const;
  private:
    std::uint64_t seed_;
};

// All game-facing values below are deliberately toy-only; no official card effects.
enum class Action : std::uint8_t { wait, gain };
struct Observation {
    Player player;
    Scores public_population;
    std::vector<int> own_hand;
    std::vector<Action> legal_actions;
    EventClock clock;
    bool operator==(const Observation&) const = default;
};
struct GameState {
    Scores population{2, 2};
    std::array<std::vector<int>, 2> hands;
    std::vector<int> future_deck;
    EventClock clock;
};
Observation observe(const GameState&, Player);
class Commitments {
  public:
    void commit(const Observation&, Action);
    std::optional<std::array<Action, 2>> reveal() const;
  private:
    std::array<std::optional<Action>, 2> pending_;
};
class OutcomeTracker {
  public:
    void record_death(Player, const EventClock&, std::string cause, std::int64_t deficit);
    void freeze_official(OfficialOutcome);
    bool ever_died(Player) const;
    const std::array<std::optional<DeathRecord>, 2>& deaths() const { return deaths_; }
    const std::optional<OfficialOutcome>& official() const { return official_; }
  private:
    std::array<std::optional<DeathRecord>, 2> deaths_;
    std::optional<OfficialOutcome> official_;
};
struct FixedPolicy {
    std::string version = "toy-wait-v1";
    Action choose(const Observation&) const;
};
struct ToyShadowRulesAdapter {
    static constexpr const char* version = "toy-signed-v1";
    void apply(GameState&, Scores delta) const;
};
struct Transition {
    std::array<Action, 2> commitments;
    Scores population_delta;
    bool operator==(const Transition&) const = default;
};
struct ReplayLog {
    std::uint32_t schema = schema_version;
    std::string rules_version = "toy-v1";
    std::uint64_t seed = 0;
    std::uint64_t horizon = 0;
    std::vector<Transition> events;
    std::string rules_hash = BLOODBATH_TOY_RULES_HASH;
    bool operator==(const ReplayLog&) const = default;
};
struct ExperimentResult {
    OfficialOutcome official;
    ShadowOutcome shadow;
    ReplayLog replay;
    std::string rules_hash = BLOODBATH_TOY_RULES_HASH;
    std::string agent_version = "toy-normal-gain-v1";
    bool operator==(const ExperimentResult&) const = default;
};
ExperimentResult run_toy(std::uint64_t seed, std::uint64_t horizon);
ExperimentResult replay_toy(const ReplayLog&);
std::string serialize(const ReplayLog&);
ReplayLog deserialize(const std::string&);
std::string serialize_result(const ExperimentResult&);
} // namespace bloodbath
