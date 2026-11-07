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
void colonies(std::ostream& out, const base_rules::State& value, const tableau::Catalog& catalog) {
    out << "{\"players\":[";
    for (Player p = 0; p < 2; ++p) {
        if (p) out << ',';
        const auto& colony = value.colonies[p];
        out << "{\"people\":" << colony.resources.people << ",\"base_people\":" << colony.resources.people
            << ",\"total_people\":" << tableau::total_people(colony, catalog) << ",\"money\":" << colony.resources.money
            << ",\"food\":" << colony.resources.food << ",\"hand\":";
        cards(out, colony.hand); out << ",\"buildings\":"; cards(out, colony.buildings);
        out << ",\"boxes\":[";
        bool first = true;
        for (const auto& [id, count] : colony.boxes) {
            if (!first) out << ',';
            first = false;
            out << '[' << id << ',' << count << ']';
        }
        out << "]}";
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
void paperwork(std::ostream& out, const std::optional<PaperworkRecord>& value) {
    if (!value) { out << "null"; return; }
    const auto& draws = value->draws;
    out << "{\"requested\":[" << draws.requested[0] << ',' << draws.requested[1]
        << "],\"drawn\":["; cards(out, draws.drawn[0]); out << ','; cards(out, draws.drawn[1]);
    out << "],\"draw_order\":[" << static_cast<unsigned>(draws.resolution_order[0]) << ','
        << static_cast<unsigned>(draws.resolution_order[1]) << "],\"seed\":" << draws.seed
        << ",\"shuffles_before\":" << draws.shuffles_before << ",\"shuffles_after\":" << draws.shuffles_after
        << ",\"discarded\":["; cards(out, value->discarded[0]); out << ','; cards(out, value->discarded[1]);
    out << "],\"discard_order\":[" << static_cast<unsigned>(value->discard_order[0]) << ','
        << static_cast<unsigned>(value->discard_order[1]) << "],\"completed\":"
        << (value->completed ? "true" : "false") << '}';
}
void choice(std::ostream& out, const tableau::Choice& value) {
    out << "{\"action\":" << static_cast<unsigned>(value.action) << ",\"build\":";
    if (value.build) out << *value.build; else out << "null";
    out << ",\"box_targets\":"; cards(out, value.box_targets); out << '}';
}
void tableau_record(std::ostream& out, const std::optional<tableau::WorkRecord>& value) {
    if (!value) { out << "null"; return; }
    out << "{\"subset_hash\":\"" << value->subset_hash << "\",\"choices\":[";
    choice(out, value->choices[0]); out << ','; choice(out, value->choices[1]);
    out << "],\"schedule\":[" << static_cast<unsigned>(value->base.resolution_order[0]) << ','
        << static_cast<unsigned>(value->base.resolution_order[1]) << "],\"seed\":" << value->base.seed
        << ",\"shuffles_before\":" << value->base.shuffles_before << ",\"shuffles_after\":" << value->base.shuffles_after
        << ",\"drawn\":["; cards(out, value->base.drawn[0]); out << ','; cards(out, value->base.drawn[1]);
    out << "],\"effects\":[";
    bool first = true;
    for (const auto& effect : value->effects) {
        if (!first) out << ',';
        first = false;
        out << "{\"player\":" << static_cast<unsigned>(effect.player) << ",\"kind\":"
            << static_cast<unsigned>(effect.kind) << ",\"card\":";
        if (effect.card) out << *effect.card; else out << "null";
        out << ",\"amount\":" << effect.amount << '}';
    }
    out << "]}";
}
void accident(std::ostream& out, const std::optional<AccidentRecord>& value) {
    if (!value) { out << "null"; return; }
    out << "{\"resolution_order\":[" << static_cast<unsigned>(value->resolution_order[0]) << ','
        << static_cast<unsigned>(value->resolution_order[1]) << "],\"losses\":[";
    for (Player p = 0; p < 2; ++p) {
        if (p) out << ',';
        const auto& loss = value->losses[p];
        out << "{\"requested_loss\":" << loss.requested_loss << ",\"paid\":" << loss.paid
            << ",\"unpaid_shortfall\":" << loss.unpaid_shortfall << ",\"lost\":[";
        bool first = true;
        for (const auto& building : loss.lost) {
            if (!first) out << ',';
            first = false;
            out << "{\"card\":" << building.card << ",\"transferred_people\":" << building.transferred_people
                << ",\"discarded_boxes\":" << building.discarded_boxes << '}';
        }
        out << "]}";
    }
    out << "]}";
}
void snapshot(std::ostream& out, const Snapshot& value, const Config& config) {
    out << "{\"colonies\":"; colonies(out, value.colonies, config.building_catalog);
    out << ",\"progress\":"; progress(out, value.progress);
    out << ",\"phase\":" << static_cast<unsigned>(value.phase)
        << ",\"next_event\":" << value.next_event << ",\"next_robot\":" << value.next_robot
        << ",\"commitments\":[";
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
    out << "],\"paperwork\":"; paperwork(out, value.paperwork);
    out << ",\"discard_commitments\":[";
    for (Player p = 0; p < 2; ++p) {
        if (p) out << ',';
        if (value.discard_commitments[p]) cards(out, *value.discard_commitments[p]); else out << "null";
    }
    out << "],\"work_choices\":[";
    for (Player p = 0; p < 2; ++p) {
        if (p) out << ',';
        if (value.work_choices[p]) choice(out, *value.work_choices[p]); else out << "null";
    }
    out << "],\"tableau_work\":"; tableau_record(out, value.tableau_work);
    out << ",\"accident\":"; accident(out, value.accident);
    out << ",\"loss_commitments\":[";
    for (Player p = 0; p < 2; ++p) {
        if (p) out << ',';
        if (value.loss_commitments[p]) cards(out, *value.loss_commitments[p]); else out << "null";
    }
    out << "]}";
}
} // namespace
std::string serialize_record(const Replay& replay) {
    const auto final = verify(replay); // Only regenerated known strings reach the JSON writer.
    const auto& config = replay.initial.config;
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "{\"schema_version\":5,\"scope\":\"" << (config.tableau_enabled ?
           "conditional-reviewed-tableau-no-external-modifiers" : "conditional-no-buildings-no-modifiers")
        << "\",\"full_game\":false,\"policy_version\":\"caller-commands-v5\","
           "\"branch\":\"supplied-conditional-trace\",\"rng_version\":\"event-splitmix64-v1\","
           "\"session_hash\":\"" << replay.subset_hash << "\",\"base_rules_hash\":\""
        << BLOODBATH_BASE_RULES_HASH << "\",\"progression_population_hash\":\""
        << BLOODBATH_PROGRESSION_POPULATION_HASH << "\",\"tableau_hash\":\"" << BLOODBATH_TABLEAU_HASH
        << "\",\"initial\":{\"colonies\":";
    colonies(out, replay.initial.colonies, config.building_catalog);
    out << ",\"progress\":"; progress(out, replay.initial.progress);
    out << ",\"config\":{\"seed\":" << config.seed << ",\"scope\":" << static_cast<unsigned>(config.scope)
        << ",\"ordered_events\":"; cards(out, config.ordered_events);
    out << ",\"ordered_robots\":"; cards(out, config.ordered_robots);
    out << ",\"tableau_enabled\":" << (config.tableau_enabled ? "true" : "false") << ",\"building_catalog\":[";
    bool first_building = true;
    for (const auto& [id, kind] : config.building_catalog) {
        if (!first_building) out << ',';
        first_building = false;
        out << '[' << id << ',' << static_cast<unsigned>(kind) << ']';
    }
    out << ']';
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
            << static_cast<unsigned>(command.schedule[1]) << "],\"cards\":";
        cards(out, command.cards); out << '}';
    }
    out << "],\"checkpoints\":[";
    first = true;
    for (const auto& point : replay.checkpoints) {
        if (!first) out << ',';
        first = false;
        snapshot(out, point, config);
    }
    out << "],\"final\":"; snapshot(out, final, config);
    out << ",\"shadow\":null,\"synthetic_score_adjustments\":[0,0]}";
    return out.str();
}
} // namespace bloodbath::session
