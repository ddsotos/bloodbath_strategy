#include "engine/session.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace ses = bloodbath::session;
namespace base = bloodbath::base_rules;
namespace tab = bloodbath::tableau;
using bloodbath::Player;
using ses::Effect;
using ses::Phase;
using ses::TraceStatus;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

// Supplied IDs, order, resources and Farm multiplicities are synthetic fixtures,
// not an official setup sampler or a statement about physical card copies.
ses::Input fixture(Effect effect, bool farms = false) {
    ses::Input input;
    std::vector<base::Card> deck;
    for (base::Card card = 8; card < 90; ++card) deck.push_back(card);
    input.colonies = base::setup_fragment({std::vector<base::Card>{0, 1, 2, 3},
        std::vector<base::Card>{4, 5, 6, 7}}, deck);
    input.config.scope = base::EffectScope::caller_verified_unmodified;
    input.config.effects = {{100, effect}};
    input.progress.deck = {100};
    input.config.tableau_enabled = farms;
    if (farms)
        for (Player p = 0; p < 2; ++p) {
            auto& colony = input.colonies.colonies[p];
            colony.resources.people = 0;
            colony.resources.food = 0;
            for (base::Card i = 0; i < 2; ++i) {
                const auto id = static_cast<base::Card>(p * 4 + i);
                colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), id));
                colony.buildings.push_back(id);
                colony.boxes.emplace(id, 2);
                input.config.building_catalog.emplace(id, tab::Kind::kale_farm);
            }
        }
    return input;
}

void checked_assessment(ses::Session& game, TraceStatus expected, bool terminal = false) {
    const auto before = game.snapshot();
    const auto first_view = game.observe(0);
    const auto second_view = game.observe(1);
    const auto assessment = ses::assess(before);
    require(assessment.status == expected && assessment.terminal_resolved == terminal &&
                assessment.has_conditional_outcome == terminal,
            "trace assessment mislabeled completion or conditional outcome");
    require(assessment.has_conditional_outcome == before.official.has_value(),
            "assessment fabricated or suppressed an existing conditional outcome");
    if (expected == TraceStatus::blocked_rule)
        require(assessment.reason && !assessment.reason->empty(), "blocked trace has no reason");
    else require(!assessment.reason, "resolved/runnable trace acquired a rule blocker");
    require(ses::assess(before) == assessment && game.snapshot() == before &&
                game.observe(0) == first_view && game.observe(1) == second_view,
            "assessment changed state, mortality or policy observations");
}

void runnable_and_waiting_inputs() {
    auto input = fixture(Effect::work);
    ses::Session work(input);
    checked_assessment(work, TraceStatus::in_progress);
    require(work.advance() == Phase::work_decision, "Work did not open its input frame");
    checked_assessment(work, TraceStatus::awaiting_input);
    work.commit(0, base::WorkAction::mine);
    checked_assessment(work, TraceStatus::awaiting_input);
    work.commit(1, base::WorkAction::farm);
    (void)work.resolve_work({0, 1});
    checked_assessment(work, TraceStatus::in_progress);

    ses::Session paper(fixture(Effect::paperwork));
    require(paper.advance() == Phase::paperwork_draw_order, "Paperwork draw frame missing");
    checked_assessment(paper, TraceStatus::awaiting_input);
    require(paper.prepare_paperwork({0, 1}) == Phase::paperwork_discard,
            "Paperwork discard frame missing");
    checked_assessment(paper, TraceStatus::awaiting_input);

    for (const auto effect : {Effect::accident, Effect::hunger, Effect::robot_waiter}) {
        ses::Session loss(fixture(effect, true));
        (void)loss.advance();
        checked_assessment(loss, TraceStatus::awaiting_input);
    }
}

void missing_rules_are_not_survival_results() {
    auto input = fixture(Effect::work);
    input.progress.deck.clear();
    ses::Session empty(input);
    require(empty.advance() == Phase::no_progress_cards, "empty supplied Progress did not stop");
    checked_assessment(empty, TraceStatus::blocked_rule);
    require(!empty.snapshot().deaths[0] && !empty.snapshot().deaths[1],
            "source exhaustion invented mortality");

    ses::Session unknown(fixture(Effect::unsupported));
    require(unknown.advance() == Phase::blocked_unknown_card, "unknown card did not stop");
    checked_assessment(unknown, TraceStatus::blocked_rule);

    input = fixture(Effect::robot_waiter);
    input.colonies.colonies[1].resources.people = 4;
    ses::Session terminal_gap(input);
    require(terminal_gap.advance() == Phase::blocked_robot_loss, "unresolved Robot terminal did not stop");
    checked_assessment(terminal_gap, TraceStatus::blocked_rule);
    require(!terminal_gap.snapshot().official && !terminal_gap.snapshot().deaths[1],
            "assessment turned unexecuted Robot loss into terminal outcome");

    ses::Session tie(fixture(Effect::instruction_manual));
    require(tie.advance() == Phase::unresolved_tie, "equal terminal scores did not retain tie gap");
    checked_assessment(tie, TraceStatus::blocked_rule);
    require(tie.snapshot().terminal_evidence && !tie.snapshot().official,
            "tie evidence was confused with adjudicated outcome");
}

ses::Input partial_butler() {
    auto input = fixture(Effect::robot_butler, true);
    for (auto& colony : input.colonies.colonies) colony.resources.people = 10;
    return input;
}

void partial_effect_and_conditional_terminal() {
    const auto input = partial_butler();
    ses::Session butler(input);
    require(butler.advance() == Phase::blocked_tableau_effect,
            "ambiguous Butler selector did not retain active effect");
    checked_assessment(butler, TraceStatus::blocked_rule);
    const auto stopped = butler.snapshot();
    const auto assessment = ses::assess(stopped);
    require(assessment.reason == std::optional<std::string>{"butler_box_selector_unverified"} &&
                stopped.robot && !stopped.robot->completed &&
                stopped.colonies.colonies[0].resources.people == 6 &&
                stopped.colonies.colonies[0].boxes == input.colonies.colonies[0].boxes &&
                stopped.progress.active == base::Card{100},
            "partially resolved Butler was mislabeled complete or lost its precise gap");
    require(butler.advance() == Phase::blocked_tableau_effect && butler.snapshot() == stopped,
            "classification allowed unresolved effect to be bypassed");

    auto final_input = fixture(Effect::instruction_manual);
    final_input.colonies.colonies[1].resources.people = 29;
    ses::Session terminal(final_input);
    require(terminal.advance() == Phase::official_terminal, "unique Instruction Manual did not resolve");
    checked_assessment(terminal, TraceStatus::conditional_terminal, true);
    const auto frozen = terminal.snapshot();
    require(terminal.advance() == Phase::official_terminal && terminal.snapshot() == frozen,
            "assessment or later advance changed frozen conditional result");
}

void invalid_phase_rejects() {
    ses::Snapshot invalid;
    invalid.phase = static_cast<Phase>(999);
    bool rejected = false;
    try { (void)ses::assess(invalid); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "invalid phase silently acquired an assessment");
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--json") {
            const auto trace = ses::record(partial_butler(), {{}});
            require(trace.schema == 14, "assessment trace schema incorrect");
            std::cout << ses::serialize_record(trace);
            return 0;
        }
        runnable_and_waiting_inputs();
        missing_rules_are_not_survival_results();
        partial_effect_and_conditional_terminal();
        invalid_phase_rejects();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "conditional trace assessment contracts passed with synthetic scenarios\n";
}
