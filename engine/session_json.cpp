#include "engine/session.hpp"

#include <locale>
#include <sstream>

namespace bloodbath::session {
namespace {
template<class T> void cards(std::ostream& out, const std::vector<T>& values) {
    out << '[';
    bool first = true;
    for (const auto value : values) {
        if (!first) out << ',';
        first = false;
        out << value;
    }
    out << ']';
}
void clock(std::ostream& out, const EventClock& value) {
    out << "{\"absolute_event\":" << value.absolute_event << ",\"cycle\":" << value.cycle
        << ",\"position\":" << value.position << ",\"decision\":" << value.decision << '}';
}
void progress(std::ostream& out, const progression::Snapshot& value) {
    out << "{\"deck\":"; cards(out, value.deck);
    out << ",\"discard\":"; cards(out, value.discard);
    out << ",\"active\":";
    if (value.active) out << *value.active; else out << "null";
    out << ",\"shuffle_index\":" << value.shuffle_index << ",\"addition_index\":" << value.addition_index
        << ",\"clock\":"; clock(out, value.clock); out << '}';
}
void colonies(std::ostream& out, const base_rules::State& value) {
    out << "{\"players\":[";
    for (Player p = 0; p < 2; ++p) {
        if (p) out << ',';
        const auto& colony = value.colonies[p];
        out << "{\"people\":" << colony.resources.people << ",\"money\":" << colony.resources.money
            << ",\"food\":" << colony.resources.food << ",\"hand\":";
        cards(out, colony.hand); out << ",\"buildings\":"; cards(out, colony.buildings); out << '}';
    }
    out << "],\"building_deck\":"; cards(out, value.building_deck);
    out << ",\"building_discard\":"; cards(out, value.building_discard);
    out << ",\"building_shuffle_index\":" << value.building_shuffle_index << '}';
}
void outcome(std::ostream& out, const std::optional<OfficialOutcome>& value) {
    if (!value) { out << "null"; return; }
    out << "{\"terminal\":"; clock(out, value->terminal);
    out << ",\"scores\":[" << value->scores[0] << ',' << value->scores[1]
        << "],\"winner\":" << static_cast<unsigned>(value->winner)
        << ",\"cause\":\"" << value->cause << "\"}";
}
void snapshot(std::ostream& out, const Snapshot& value) {
    out << "{\"colonies\":"; colonies(out, value.colonies);
    out << ",\"progress\":"; progress(out, value.progress);
    out << ",\"phase\":" << static_cast<unsigned>(value.phase)
        << ",\"next_event\":" << value.next_event << ",\"commitments\":[";
    for (Player p = 0; p < 2; ++p) {
        if (p) out << ',';
        if (value.commitments[p]) out << static_cast<unsigned>(*value.commitments[p]); else out << "null";
    }
    out << "],\"conditional_official_outcome\":"; outcome(out, value.official);
    out << ",\"terminal_evidence\":";
    if (!value.terminal_evidence) out << "null";
    else {
        out << "{\"clock\":"; clock(out, value.terminal_evidence->clock);
        out << ",\"populations\":[" << value.terminal_evidence->populations[0] << ','
            << value.terminal_evidence->populations[1] << "],\"cause\":\""
            << value.terminal_evidence->cause << "\"}";
    }
    out << ",\"first_deaths\":[";
    for (Player p = 0; p < 2; ++p) {
        if (p) out << ',';
        if (!value.deaths[p]) { out << "null"; continue; }
        const auto& death = *value.deaths[p];
        out << "{\"player\":" << static_cast<unsigned>(death.player) << ",\"clock\":";
        clock(out, death.clock);
        out << ",\"cause\":\"" << death.cause << "\",\"deficit\":" << death.deficit << '}';
    }
    out << "]}";
}
} // namespace
std::string serialize_record(const Replay& replay) {
    const auto final = verify(replay); // Only regenerated known strings reach the JSON writer.
    const auto& config = replay.initial.config;
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "{\"schema_version\":1,\"scope\":\"conditional-no-buildings-no-modifiers\","
           "\"full_game\":false,\"policy_version\":\"caller-commands-v1\","
           "\"branch\":\"supplied-conditional-trace\",\"rng_version\":\"event-splitmix64-v1\","
           "\"session_hash\":\"" << replay.subset_hash << "\",\"base_rules_hash\":\""
        << BLOODBATH_BASE_RULES_HASH << "\",\"progression_population_hash\":\""
        << BLOODBATH_PROGRESSION_POPULATION_HASH << "\",\"initial\":{\"colonies\":";
    colonies(out, replay.initial.colonies);
    out << ",\"progress\":"; progress(out, replay.initial.progress);
    out << ",\"config\":{\"seed\":" << config.seed << ",\"scope\":" << static_cast<unsigned>(config.scope)
        << ",\"ordered_events\":"; cards(out, config.ordered_events);
    out << ",\"effects\":[";
    bool first = true;
    for (const auto& [id, effect] : config.effects) {
        if (!first) out << ',';
        first = false;
        out << '[' << id << ',' << static_cast<unsigned>(effect) << ']';
    }
    out << "]}},\"commands\":[";
    first = true;
    for (const auto& command : replay.commands) {
        if (!first) out << ',';
        first = false;
        out << "{\"kind\":" << static_cast<unsigned>(command.kind) << ",\"player\":"
            << static_cast<unsigned>(command.player) << ",\"action\":" << static_cast<unsigned>(command.action)
            << ",\"schedule\":[" << static_cast<unsigned>(command.schedule[0]) << ','
            << static_cast<unsigned>(command.schedule[1]) << "]}";
    }
    out << "],\"checkpoints\":[";
    first = true;
    for (const auto& point : replay.checkpoints) {
        if (!first) out << ',';
        first = false;
        snapshot(out, point);
    }
    out << "],\"final\":"; snapshot(out, final);
    out << ",\"shadow\":null,\"synthetic_score_adjustments\":[0,0]}";
    return out.str();
}
} // namespace bloodbath::session
