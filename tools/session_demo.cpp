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
} // namespace
int main(int argc, char** argv) {
    try {
        std::cout.imbue(std::locale::classic());
        if (argc == 1) {
            std::cout << session::serialize_record(session::record(fixture(42), script())) << '\n';
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--paperwork") {
            std::cout << session::serialize_record(session::record(paperwork_fixture(42), paperwork_script())) << '\n';
            return 0;
        }
        if (argc != 2 || (std::string(argv[1]) != "--benchmark" && std::string(argv[1]) != "--benchmark-paperwork"))
            throw std::invalid_argument("usage: session_demo [--paperwork | --benchmark | --benchmark-paperwork]");
        const bool with_paperwork = std::string(argv[1]) == "--benchmark-paperwork";
        constexpr std::uint64_t count = 1000;
        const auto start = std::chrono::steady_clock::now();
        std::uint64_t checksum = 0;
        for (std::uint64_t seed = 0; seed < count; ++seed) {
            const auto replay = session::record(with_paperwork ? paperwork_fixture(seed) : fixture(seed),
                                                with_paperwork ? paperwork_script() : script());
            checksum += session::verify(replay).progress.clock.absolute_event;
        }
        const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "{\"fixture\":\"" << (with_paperwork ? "conditional-paperwork-v1" : "conditional-script-v1")
            << "\",\"full_game\":false,\"traces\":"
            << count << ",\"record_and_verify_seconds\":" << seconds << ",\"traces_per_second\":"
            << count / seconds << ",\"checksum\":" << checksum << "}\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
