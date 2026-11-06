#include "engine/core.hpp"
#include <algorithm>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace bloodbath {
namespace {
std::uint64_t mix(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
void check_player(Player p) {
    if (p > 1) throw std::invalid_argument("invalid player");
}
void validate(const ReplayLog& log) {
    if (log.schema != schema_version || log.rules_version != "toy-v1" ||
        log.rules_hash != BLOODBATH_TOY_RULES_HASH ||
        log.horizon < 1 || log.horizon > 100000 || log.events.size() != log.horizon)
        throw std::invalid_argument("unsupported or incomplete toy replay");
}
Transition toy_transition(const GameState& state, const OutcomeTracker& tracker,
                          const RandomStreams& rng, std::uint64_t event) {
    Transition t{};
    FixedPolicy policy;
    for (Player p = 0; p < 2; ++p) {
        t.commitments[p] = tracker.ever_died(p) ? policy.choose(observe(state, p)) : Action::gain;
        t.population_delta[p] = (t.commitments[p] == Action::gain ? 1 : 0) -
            static_cast<std::int64_t>(rng.bounded(RandomSource::progression, event, p, 3)) - 1 +
            (event % 3 == 0 ? 7 : 0);
    }
    return t;
}
ExperimentResult execute(std::uint64_t seed, std::uint64_t horizon, const ReplayLog* input) {
    if (horizon < 1 || horizon > 100000) throw std::invalid_argument("invalid toy horizon");
    GameState state;
    OutcomeTracker tracker;
    RandomStreams rng(seed);
    ReplayLog log{schema_version, "toy-v1", seed, horizon, {}};
    for (std::uint64_t event = 1; event <= horizon; ++event) {
        const auto t = toy_transition(state, tracker, rng, event);
        if (input && t != input->events[static_cast<std::size_t>(event - 1)])
            throw std::invalid_argument("replay transition mismatch");
        Commitments commits;
        for (Player p = 0; p < 2; ++p) commits.commit(observe(state, p), t.commitments[p]);
        if (!commits.reveal()) throw std::logic_error("missing simultaneous actions");
        if (tracker.official()) {
            ToyShadowRulesAdapter{}.apply(state, t.population_delta);
        } else {
            for (Player p = 0; p < 2; ++p) state.population[p] += t.population_delta[p];
        }
        state.clock = {event, (event - 1) / 4, (event - 1) % 4 + 1, event};
        for (Player p = 0; p < 2; ++p)
            if (state.population[p] <= 0)
                tracker.record_death(p, state.clock, "toy-nonpositive", -state.population[p]);
        if (tracker.ever_died(0) || tracker.ever_died(1) || event == horizon) {
            Winner winner = Winner::draw;
            if (tracker.ever_died(0) != tracker.ever_died(1))
                winner = tracker.ever_died(0) ? Winner::player1 : Winner::player0;
            tracker.freeze_official({state.clock, state.population, winner,
                tracker.ever_died(0) || tracker.ever_died(1) ? "toy-death" : "toy-horizon"});
        }
        log.events.push_back(t);
    }
    return {*tracker.official(),
        {state.clock, state.population, tracker.deaths(), "toy-wait-v1", ToyShadowRulesAdapter::version, {}}, log};
}
} // namespace
std::uint64_t RandomStreams::sample(RandomSource source, std::uint64_t event,
                                    std::uint64_t draw) const {
    return mix(mix(mix(mix(seed_) ^ static_cast<std::uint64_t>(source)) ^ event) ^ draw);
}
std::uint64_t RandomStreams::bounded(RandomSource source, std::uint64_t event,
                                     std::uint64_t draw, std::uint64_t bound) const {
    if (!bound) throw std::invalid_argument("zero RNG bound");
    const auto threshold = (std::uint64_t{0} - bound) % bound;
    auto value = sample(source, event, draw);
    std::uint64_t retry = 0;
    while (value < threshold) value = mix(value ^ mix(++retry));
    return value % bound;
}
Observation observe(const GameState& state, Player p) {
    check_player(p);
    return {p, state.population, state.hands[p], {Action::wait, Action::gain}, state.clock};
}
void Commitments::commit(const Observation& obs, Action action) {
    check_player(obs.player);
    if (pending_[obs.player] || (action != Action::wait && action != Action::gain) ||
        std::find(obs.legal_actions.begin(), obs.legal_actions.end(), action) == obs.legal_actions.end())
        throw std::invalid_argument("duplicate or illegal commitment");
    pending_[obs.player] = action;
}
std::optional<std::array<Action, 2>> Commitments::reveal() const {
    if (!pending_[0] || !pending_[1]) return std::nullopt;
    return std::array<Action, 2>{*pending_[0], *pending_[1]};
}
void OutcomeTracker::record_death(Player p, const EventClock& clock, std::string cause,
                                   std::int64_t deficit) {
    check_player(p);
    if (deficit < 0) throw std::invalid_argument("negative deficit");
    if (!deaths_[p]) deaths_[p] = DeathRecord{p, clock, std::move(cause), deficit};
}
bool OutcomeTracker::ever_died(Player p) const { check_player(p); return deaths_[p].has_value(); }
void OutcomeTracker::freeze_official(OfficialOutcome outcome) {
    if (!official_) official_ = std::move(outcome);
}
Action FixedPolicy::choose(const Observation& obs) const {
    if (std::find(obs.legal_actions.begin(), obs.legal_actions.end(), Action::wait) ==
        obs.legal_actions.end()) throw std::invalid_argument("toy policy requires wait");
    return Action::wait;
}
void ToyShadowRulesAdapter::apply(GameState& state, Scores delta) const {
    for (Player p = 0; p < 2; ++p) state.population[p] += delta[p];
}
ExperimentResult run_toy(std::uint64_t seed, std::uint64_t horizon) {
    return execute(seed, horizon, nullptr);
}
ExperimentResult replay_toy(const ReplayLog& log) {
    validate(log);
    return execute(log.seed, log.horizon, &log);
}
std::string serialize(const ReplayLog& log) {
    validate(log);
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "bloodbath-replay " << log.schema << ' ' << log.rules_version << ' ' << log.seed
        << ' ' << log.horizon << ' ' << log.rules_hash << " event-splitmix64-v1\n";
    for (const auto& e : log.events)
        out << static_cast<unsigned>(e.commitments[0]) << ' '
            << static_cast<unsigned>(e.commitments[1]) << ' '
            << e.population_delta[0] << ' ' << e.population_delta[1] << '\n';
    return out.str();
}
ReplayLog deserialize(const std::string& text) {
    std::istringstream in(text);
    in.imbue(std::locale::classic());
    std::string magic;
    std::string rng_version;
    ReplayLog log;
    if (!(in >> magic >> log.schema >> log.rules_version >> log.seed >> log.horizon
             >> log.rules_hash >> rng_version) || rng_version != "event-splitmix64-v1" ||
        magic != "bloodbath-replay" || log.horizon < 1 || log.horizon > 100000)
        throw std::invalid_argument("invalid replay header");
    for (std::uint64_t i = 0; i < log.horizon; ++i) {
        unsigned a, b;
        Scores delta{};
        if (!(in >> a >> b >> delta[0] >> delta[1]) || a > 1 || b > 1)
            throw std::invalid_argument("invalid replay event");
        log.events.push_back({{static_cast<Action>(a), static_cast<Action>(b)}, delta});
    }
    in >> std::ws;
    if (!in.eof()) throw std::invalid_argument("trailing replay content");
    validate(log);
    return log;
}
namespace {
std::string quote(const std::string& text) {
    std::ostringstream out;
    out << '"';
    constexpr char hex[] = "0123456789abcdef";
    for (const unsigned char c : text) {
        if (c == '"' || c == '\\') out << '\\' << static_cast<char>(c);
        else if (c < 32 || c >= 127) out << "\\u00" << hex[c >> 4] << hex[c & 15];
        else out << static_cast<char>(c);
    }
    out << '"';
    return out.str();
}
void write_clock(std::ostream& out, const EventClock& clock) {
    out << "{\"absolute_event\":" << clock.absolute_event << ",\"cycle\":" << clock.cycle
        << ",\"position\":" << clock.position << ",\"decision\":" << clock.decision << '}';
}
void write_scores(std::ostream& out, Scores scores) {
    out << '[' << scores[0] << ',' << scores[1] << ']';
}
} // namespace
std::string serialize_result(const ExperimentResult& r) {
    // Version 1 is an ASCII toy record; full official rule manifests are pending.
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "{\"schema_version\":" << schema_version << ",\"fixture\":\"toy-only\",\"rules_hash\":"
        << quote(r.rules_hash) << ",\"rng_version\":\"event-splitmix64-v1\",\"agent_version\":"
        << quote(r.agent_version) << ",\"seed\":" << r.replay.seed
        << ",\"config\":{\"initial_population\":[2,2],\"horizon\":" << r.replay.horizon
        << "},\"branch\":\"baseline\",\"coupling\":\"event-source-draw-v1\",\"official\":{\"terminal\":";
    write_clock(out, r.official.terminal);
    out << ",\"scores\":"; write_scores(out, r.official.scores);
    out << ",\"winner\":" << static_cast<unsigned>(r.official.winner)
        << ",\"cause\":" << quote(r.official.cause) << "},\"shadow\":{\"horizon\":";
    write_clock(out, r.shadow.horizon);
    out << ",\"scores\":"; write_scores(out, r.shadow.scores);
    out << ",\"policy_version\":" << quote(r.shadow.policy_version)
        << ",\"adapter_version\":" << quote(r.shadow.adapter_version)
        << ",\"synthetic_adjustments\":"; write_scores(out, r.shadow.synthetic_adjustments);
    out << ",\"first_deaths\":[";
    for (Player p = 0; p < 2; ++p) {
        if (p) out << ',';
        const auto& death = r.shadow.deaths[p];
        if (!death) { out << "null"; continue; }
        out << "{\"player\":" << static_cast<unsigned>(death->player) << ",\"clock\":";
        write_clock(out, death->clock);
        out << ",\"cause\":" << quote(death->cause) << ",\"deficit\":" << death->deficit << '}';
    }
    out << "]},\"replay\":" << quote(serialize(r.replay)) << '}';
    return out.str();
}
} // namespace bloodbath
