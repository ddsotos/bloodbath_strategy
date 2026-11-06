#include "engine/core.hpp"
#include <iostream>
#include <locale>
#include <stdexcept>
#include <string>

using namespace bloodbath;
namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class F> void rejects(F f) {
    try { f(); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("expected invalid_argument");
}
void boundaries() {
    GameState a;
    a.hands = {std::vector<int>{1}, std::vector<int>{2}};
    a.future_deck = {3, 4};
    auto b = a;
    b.hands[1] = {90};
    b.future_deck = {4, 3};
    require(observe(a, 0) == observe(b, 0), "hidden information leaked");
    require(observe(a, 1) != observe(b, 1), "own hand missing");
    Commitments c;
    const auto before_commit = observe(a, 1);
    require(!c.reveal(), "empty commitment revealed");
    c.commit(observe(a, 0), Action::gain);
    require(!c.reveal(), "single commitment revealed");
    require(observe(a, 1) == before_commit, "observation changed after commitment");
    rejects([&] { c.commit(observe(a, 0), Action::wait); });
    auto obs = observe(a, 1);
    obs.legal_actions = {Action::wait};
    rejects([&] { c.commit(obs, Action::gain); });
    c.commit(obs, Action::wait);
    require(c.reveal().value() == std::array{Action::gain, Action::wait}, "reveal mismatch");
    Commitments reversed;
    reversed.commit(obs, Action::wait);
    require(!reversed.reveal(), "reverse early reveal");
    reversed.commit(observe(a, 0), Action::gain);
    require(reversed.reveal() == c.reveal(), "commit order changed result");
    rejects([&] { (void)observe(a, 2); });
}
void mortality() {
    OutcomeTracker t;
    const EventClock first{1, 0, 1, 1};
    const EventClock late{7, 1, 3, 7};
    require(!t.deaths()[0], "survivor sentinel missing");
    t.record_death(0, first, "test", 2);
    const auto death = t.deaths()[0];
    t.record_death(0, late, "different", 9);
    require(t.deaths()[0] == death, "death overwritten");
    t.record_death(1, late, "second", 0);
    require(t.deaths()[1]->clock == late, "second death wrong");
    const OfficialOutcome official{first, {0, 2}, Winner::player1, "test"};
    t.freeze_official(official);
    t.freeze_official({late, {100, 200}, Winner::player0, "changed"});
    require(t.official() == official, "official result overwritten");
    rejects([&] { t.record_death(2, first, "invalid", 0); });
    rejects([&] { t.record_death(0, first, "invalid", -1); });
}
void randomness() {
    RandomStreams rng(42), same(42), other(43);
    const auto original = rng.sample(RandomSource::building, 3, 2);
    for (std::uint64_t i = 0; i < 100; ++i) (void)rng.sample(RandomSource::action, i, 0);
    require(original == rng.sample(RandomSource::building, 3, 2), "source interference");
    require(original == same.sample(RandomSource::building, 3, 2), "seed mismatch");
    require(original != other.sample(RandomSource::building, 3, 2), "seed ignored");
    require(original != rng.sample(RandomSource::progression, 3, 2), "source ignored");
    for (std::uint64_t i = 0; i < 100; ++i)
        require(rng.bounded(RandomSource::setup, i, 0, 7) < 7, "out of range RNG");
    rejects([&] { (void)rng.bounded(RandomSource::setup, 0, 0, 0); });
}
void rollouts() {
    bool recovery = false, later_death = false, survivor = false;
    for (std::uint64_t seed = 0; seed < 256; ++seed) {
        auto r = run_toy(seed, 12);
        require(r == run_toy(seed, 12), "nondeterministic simulation");
        require(r == replay_toy(deserialize(serialize(r.replay))), "replay result mismatch");
        require(r.shadow.horizon == EventClock{12, 2, 4, 12}, "common horizon wrong");
        auto prefix = run_toy(seed, r.official.terminal.absolute_event);
        require(prefix.official == r.official, "shadow mutated official outcome");
        for (Player p = 0; p < 2; ++p) {
            const auto& death = r.shadow.deaths[p];
            if (!death) { survivor = true; continue; }
            if (death->clock.absolute_event > r.official.terminal.absolute_event) later_death = true;
            for (auto i = death->clock.absolute_event; i < r.replay.events.size(); ++i)
                require(r.replay.events[static_cast<std::size_t>(i)].commitments[p] == Action::wait,
                        "post-death policy resumed optimization");
            if (r.shadow.scores[p] > 0) recovery = true;
        }
    }
    require(recovery, "missing positive recovery fixture");
    require(later_death, "missing second death during shadow fixture");
    require(survivor, "missing survival sentinel fixture");
    auto log = run_toy(42, 12).replay;
    log.events[0].population_delta[0]++;
    rejects([&] { (void)replay_toy(log); });
    log = run_toy(42, 12).replay;
    log.rules_hash = "wrong-rules";
    rejects([&] { (void)replay_toy(log); });
    rejects([] { (void)deserialize("invalid"); });
    rejects([] { (void)deserialize(serialize(run_toy(1, 1).replay) + "extra"); });
    rejects([] { (void)run_toy(1, 0); });
}
struct GroupedNumbers : std::numpunct<char> {
    char do_thousands_sep() const override { return ','; }
    std::string do_grouping() const override { return "\3"; }
};
void locale_independence() {
    const auto result = run_toy(123456, 12);
    const auto replay = serialize(result.replay);
    const auto json = serialize_result(result);
    const auto previous = std::locale();
    std::locale::global(std::locale(previous, new GroupedNumbers));
    const auto replay_grouped = serialize(result.replay);
    const auto json_grouped = serialize_result(result);
    const auto parsed = deserialize(replay);
    std::locale::global(previous);
    require(replay == replay_grouped && json == json_grouped, "locale changed record");
    require(parsed == result.replay, "locale changed parse");
}
} // namespace
int main() {
    try { boundaries(); mortality(); randomness(); rollouts(); locale_independence(); }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    std::cout << "toy contracts passed across 256 seeds\n";
}
