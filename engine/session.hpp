#pragma once

#include "engine/base_work.hpp"
#include "engine/population.hpp"
#include "engine/progression.hpp"
#include <map>

namespace bloodbath::session {
inline constexpr const char* manifest_hash = BLOODBATH_SESSION_HASH;
enum class Effect { work, trouble, hunger, accident, instruction_manual, unsupported };
enum class Phase {
    ready, work_decision, blocked_unknown_card, blocked_event_queue,
    blocked_population_payment, unresolved_tie, no_progress_cards, official_terminal
};
struct Config {
    std::uint64_t seed = 0;
    // Caller-reviewed instance mapping and Event order, not an invented catalog/setup.
    std::map<progression::Card, Effect> effects;
    std::vector<progression::Card> ordered_events;
    // Certifies absence of ALL relevant modifiers (including Event/terminal effects).
    base_rules::EffectScope scope = base_rules::EffectScope::unverified;
    bool operator==(const Config&) const = default;
};
struct Input {
    base_rules::State colonies;
    progression::Snapshot progress;
    Config config;
    bool operator==(const Input&) const = default;
};
struct TerminalEvidence {
    EventClock clock;
    Scores populations;
    std::string cause;
    bool operator==(const TerminalEvidence&) const = default;
};
struct Snapshot {
    base_rules::State colonies;
    progression::Snapshot progress;
    Phase phase = Phase::ready;
    std::array<std::optional<base_rules::WorkAction>, 2> commitments;
    std::size_t next_event = 0;
    std::optional<OfficialOutcome> official;
    std::array<std::optional<DeathRecord>, 2> deaths;
    std::optional<TerminalEvidence> terminal_evidence;
    bool operator==(const Snapshot&) const = default;
};
struct Observation {
    base_rules::Observation colony;
    progression::Observation progress;
    Phase phase;
    std::optional<OfficialOutcome> official;
    bool operator==(const Observation&) const = default;
};
class Session {
  public:
    explicit Session(Input);
    // Advances at most one Progress card. Waiting/blocked/terminal phases cannot be skipped.
    Phase advance();
    void commit(Player, base_rules::WorkAction);
    base_rules::WorkRecord resolve_work(std::array<Player, 2> explicit_schedule);
    Observation observe(Player) const;
    Snapshot snapshot() const { return state_; }
  private:
    void terminal(Snapshot&, const EventClock&, const std::string& cause);
    Config config_;
    Snapshot state_;
};
enum class CommandKind { advance, commit, resolve_work };
struct Command {
    CommandKind kind = CommandKind::advance;
    Player player = 0;
    base_rules::WorkAction action = base_rules::WorkAction::mine;
    std::array<Player, 2> schedule{0, 1};
    bool operator==(const Command&) const = default;
};
struct Replay {
    std::uint32_t schema = 1;
    std::string subset_hash = manifest_hash;
    Input initial;
    std::vector<Command> commands;
    std::vector<Snapshot> checkpoints;
    bool operator==(const Replay&) const = default;
};
Replay record(Input, const std::vector<Command>&);
Snapshot verify(const Replay&);
// Engine/audit output containing private truth. Never provide it to a policy.
std::string serialize_record(const Replay&);
} // namespace bloodbath::session
