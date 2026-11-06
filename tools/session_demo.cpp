#include "engine/session.hpp"

#include <chrono>
#include <iostream>
#include <locale>
#include <stdexcept>

namespace session = bloodbath::session;
namespace rules = bloodbath::base_rules;
namespace {
session::Input fixture(std::uint64_t seed) {
    std::vector<rules::Card> remainder;
    for (rules::Card card = 8; card < 90; ++card) remainder.push_back(card);
    session::Input input;
    input.colonies = rules::setup_fragment({std::vector<rules::Card>{0, 1, 2, 3},
                                          std::vector<rules::Card>{4, 5, 6, 7}}, remainder);
    // Explicit synthetic conditional position: not official setup or a game distribution.
    input.colonies.colonies[1].resources.people = 29;
    input.progress.deck = {104, 103, 102, 101, 100};
    input.config.seed = seed;
    input.config.scope = rules::EffectScope::caller_verified_unmodified;
    input.config.effects = {{100, session::Effect::work}, {101, session::Effect::trouble},
        {102, session::Effect::hunger}, {103, session::Effect::accident},
        {104, session::Effect::instruction_manual}, {105, session::Effect::hunger}};
    input.config.ordered_events = {105};
    return input;
}
std::vector<session::Command> script() {
    return {{session::CommandKind::advance},
        {session::CommandKind::commit, 0, rules::WorkAction::research},
        {session::CommandKind::commit, 1, rules::WorkAction::farm},
        {session::CommandKind::resolve_work, 0, rules::WorkAction::mine, {1, 0}},
        {session::CommandKind::advance}, {session::CommandKind::advance},
        {session::CommandKind::advance}, {session::CommandKind::advance},
        {session::CommandKind::advance}};
}
session::Input paperwork_fixture(std::uint64_t seed) {
    auto input = fixture(seed);
    input.progress.deck = {104, 106, 100};
    input.config.effects[106] = session::Effect::paperwork;
    input.config.ordered_events.clear();
    return input;
}
std::vector<session::Command> paperwork_script() {
    auto commands = script();
    commands.resize(4); // Existing Work, then a Paperwork draw/discard frame.
    commands.push_back({session::CommandKind::advance});
    commands.push_back({session::CommandKind::prepare_paperwork, 0, rules::WorkAction::mine, {1, 0}});
    commands.push_back({session::CommandKind::commit_discards, 0, rules::WorkAction::mine, {0, 1}, {0, 86}});
    commands.push_back({session::CommandKind::commit_discards, 1, rules::WorkAction::mine, {0, 1}, {4, 87}});
    commands.push_back({session::CommandKind::resolve_paperwork, 0, rules::WorkAction::mine, {1, 0}});
    commands.push_back({session::CommandKind::advance});
    return commands;
}
session::Input robot_fixture(std::uint64_t seed) {
    auto input = fixture(seed);
    input.progress.deck = {104, 107, 100};
    input.config.effects[107] = session::Effect::glitch;
    input.config.effects[108] = session::Effect::robot_waiter;
    input.config.ordered_events.clear();
    input.config.ordered_robots = {108};
    return input;
}
std::vector<session::Command> robot_script() {
    auto commands = script();
    commands.resize(4);
    commands.push_back({session::CommandKind::advance});
    commands.push_back({session::CommandKind::advance});
    commands.push_back({session::CommandKind::advance});
    return commands;
}
session::Input tableau_fixture(std::uint64_t seed) {
    auto input = fixture(seed);
    input.progress.deck = {104, 113, 112, 111, 100};
    for (const auto id : {111U, 112U, 113U}) input.config.effects[id] = session::Effect::work;
    input.config.ordered_events.clear();
    input.config.tableau_enabled = true;
    input.config.building_catalog = {{0, bloodbath::tableau::Kind::kale_farm}, {1, bloodbath::tableau::Kind::kale_farm}};
    return input;
}
std::vector<session::Command> tableau_script() {
    using K = session::CommandKind;
    using A = rules::WorkAction;
    return {{K::advance}, {K::commit_choice, 0, A::build, {0, 1}, {0}}, {K::commit, 1, A::farm}, {K::resolve_work},
        {K::advance}, {K::commit_choice, 0, A::build, {0, 1}, {1}}, {K::commit, 1, A::research}, {K::resolve_work},
        {K::advance}, {K::commit_choice, 0, A::restock, {0, 1}, {0, 1}}, {K::commit, 1, A::farm}, {K::resolve_work},
        {K::advance}, {K::commit, 0, A::farm}, {K::commit, 1, A::mine}, {K::resolve_work}, {K::advance}};
}
enum class Mode { basic, paperwork, robots, tableau };
session::Input input_for(Mode mode, std::uint64_t seed) {
    switch (mode) {
    case Mode::paperwork: return paperwork_fixture(seed);
    case Mode::robots: return robot_fixture(seed);
    case Mode::tableau: return tableau_fixture(seed);
    default: return fixture(seed);
    }
}
std::vector<session::Command> script_for(Mode mode) {
    switch (mode) {
    case Mode::paperwork: return paperwork_script();
    case Mode::robots: return robot_script();
    case Mode::tableau: return tableau_script();
    default: return script();
    }
}
const char* label_for(Mode mode) {
    switch (mode) {
    case Mode::paperwork: return "conditional-paperwork-v1";
    case Mode::robots: return "conditional-robots-v1";
    case Mode::tableau: return "conditional-tableau-v1";
    default: return "conditional-script-v1";
    }
}
} // namespace
int main(int argc, char** argv) {
    try {
        std::cout.imbue(std::locale::classic());
        if (argc > 2) throw std::invalid_argument("session_demo takes at most one mode");
        const std::string argument = argc == 2 ? argv[1] : "";
        Mode mode = Mode::basic;
        bool benchmark = false;
        if (argument == "--paperwork") mode = Mode::paperwork;
        else if (argument == "--robots") mode = Mode::robots;
        else if (argument == "--tableau") mode = Mode::tableau;
        else if (argument == "--benchmark") benchmark = true;
        else if (argument == "--benchmark-paperwork") { mode = Mode::paperwork; benchmark = true; }
        else if (argument == "--benchmark-robots") { mode = Mode::robots; benchmark = true; }
        else if (argument == "--benchmark-tableau") { mode = Mode::tableau; benchmark = true; }
        else if (!argument.empty()) throw std::invalid_argument("unknown session_demo mode");
        if (!benchmark) {
            std::cout << session::serialize_record(session::record(input_for(mode, 42), script_for(mode))) << '\n';
            return 0;
        }
        constexpr std::uint64_t count = 1000;
        const auto start = std::chrono::steady_clock::now();
        std::uint64_t checksum = 0;
        for (std::uint64_t seed = 0; seed < count; ++seed) {
            const auto replay = session::record(input_for(mode, seed), script_for(mode));
            checksum += session::verify(replay).progress.clock.absolute_event;
        }
        const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "{\"fixture\":\"" << label_for(mode)
            << "\",\"full_game\":false,\"traces\":"
            << count << ",\"record_and_verify_seconds\":" << seconds << ",\"traces_per_second\":"
            << count / seconds << ",\"checksum\":" << checksum << "}\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
