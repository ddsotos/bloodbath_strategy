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
} // namespace
int main(int argc, char** argv) {
    try {
        std::cout.imbue(std::locale::classic());
        if (argc == 1) {
            std::cout << session::serialize_record(session::record(fixture(42), script())) << '\n';
            return 0;
        }
        if (argc != 2 || std::string(argv[1]) != "--benchmark")
            throw std::invalid_argument("usage: session_demo [--benchmark]");
        constexpr std::uint64_t count = 1000;
        const auto start = std::chrono::steady_clock::now();
        std::uint64_t checksum = 0;
        for (std::uint64_t seed = 0; seed < count; ++seed) {
            const auto replay = session::record(fixture(seed), script());
            checksum += session::verify(replay).progress.clock.absolute_event;
        }
        const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "{\"fixture\":\"conditional-script-v1\",\"full_game\":false,\"traces\":"
            << count << ",\"record_and_verify_seconds\":" << seconds << ",\"traces_per_second\":"
            << count / seconds << ",\"checksum\":" << checksum << "}\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
