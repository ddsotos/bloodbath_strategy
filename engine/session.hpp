#pragma once

#include "engine/base_work.hpp"
#include "engine/population.hpp"
#include "engine/progression.hpp"
#include <map>

namespace bloodbath::session {
inline constexpr const char* manifest_hash = BLOODBATH_SESSION_HASH;
enum class Effect { work, trouble, hunger, accident, instruction_manual, unsupported, paperwork,
                    glitch, robot_butler, robot_waiter, mining_robot };
enum class Phase {
    ready, work_decision, blocked_unknown_card, blocked_event_queue,
    blocked_population_payment, unresolved_tie, no_progress_cards, official_terminal,
    paperwork_draw_order, paperwork_discard, blocked_shared_draw,
    blocked_robot_deck, blocked_robot_loss
};
struct Config {
    std::uint64_t seed = 0;
    // Caller-reviewed instance mapping and Event order, not an invented catalog/setup.
    std::map<progression::Card, Effect> effects;
    std::vector<progression::Card> ordered_events;
    // Supplied already-shuffled Robot deck in top-first order. Private engine truth.
    std::vector<progression::Card> ordered_robots;
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
struct PaperworkRecord {
    base_rules::BuildingDrawRecord draws;
    std::array<std::vector<base_rules::Card>, 2> discarded;
    std::array<Player, 2> discard_order{0, 1};
    bool completed = false;
    bool operator==(const PaperworkRecord&) const = default;
};
struct Snapshot {
    base_rules::State colonies;
    progression::Snapshot progress;
    Phase phase = Phase::ready;
    std::array<std::optional<base_rules::WorkAction>, 2> commitments;
    std::size_t next_event = 0;
    std::size_t next_robot = 0;
    std::optional<OfficialOutcome> official;
    std::array<std::optional<DeathRecord>, 2> deaths;
    std::optional<TerminalEvidence> terminal_evidence;
    std::optional<PaperworkRecord> paperwork;
    std::array<std::optional<std::vector<base_rules::Card>>, 2> discard_commitments;
    bool operator==(const Snapshot&) const = default;
};
struct Observation {
    base_rules::Observation colony;
    progression::Observation progress;
    Phase phase;
    std::optional<OfficialOutcome> official;
    std::size_t required_discards = 0;
    bool operator==(const Observation&) const = default;
};
class Session {
  public:
    explicit Session(Input);
    // Advances at most one Progress card. Waiting/blocked/terminal phases cannot be skipped.
    Phase advance();
    void commit(Player, base_rules::WorkAction);
    base_rules::WorkRecord resolve_work(std::array<Player, 2> explicit_schedule);
    Phase prepare_paperwork(std::array<Player, 2> explicit_draw_schedule);
    void commit_discards(Player, std::vector<base_rules::Card>);
    PaperworkRecord resolve_paperwork(std::array<Player, 2> explicit_discard_schedule);
    Observation observe(Player) const;
    Snapshot snapshot() const { return state_; }
  private:
    void terminal(Snapshot&, const EventClock&, const std::string& cause);
    Config config_;
    Snapshot state_;
};
enum class CommandKind { advance, commit, resolve_work, prepare_paperwork, commit_discards, resolve_paperwork };
struct Command {
    CommandKind kind = CommandKind::advance;
    Player player = 0;
    base_rules::WorkAction action = base_rules::WorkAction::mine;
    std::array<Player, 2> schedule{0, 1};
    std::vector<base_rules::Card> cards{};
    bool operator==(const Command&) const = default;
};
struct Replay {
    std::uint32_t schema = 3;
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
