# Architecture and contracts

## Modules (target, not yet implemented)

```text
engine/             C++20 rule engine and deterministic state transitions
engine/cards/       data-driven cards and verified rule effects
search/             rollout, PIMC, ISMCTS and simultaneous-action search
agents/             baseline, normal learned policy and FIXED post-death policy
experiments/        paired branch and first-Work evaluation harness
python/             training/analysis bindings and scripts (later)
tests/              fixtures, rule conformance and differential tests
docs/               authoritative decisions and rule provenance
```

Prefer CMake + CTest and a small dependency surface. Inference batching and GPU integration only after profiling.

## Explicit states and boundaries

`GameState`: full engine truth, including private hands and future ordered decks, phase, resources, card zones, progress counter, RNG metadata. Never pass directly to an agent.

`Observation(player)`: solely legally visible information and allowed legal actions. No opponent hand or future random order. For simultaneous Work, both choices must be committed before either is revealed.

`DecisionPoint`: an optional simultaneous or single-actor choice. Automatically resolve deterministic / stochastic transitions between decisions without losing triggered choices.

`EventClock`: `absolute_progress_event_index`, `round_or_cycle_index`, `position_in_cycle`, `decision_index`; define off-by-one semantics and card-type effects in tests.

`DeathRecord`: first death point, player, cause, signed shortfall if applicable. Exactly-once first death marker with `never_died` distinct from `died_at_end`.

`OfficialOutcome`: terminal event, official score vector, official winner/draw.

`ShadowOutcome`: common-horizon score vector, fixed-policy version, recorded first-death point per player, adapter version, synthetic adjustments.

`ExperimentResult`: both outcomes, seed, config, source rules hash, branch identity, model/search versions, action/observation and simulation wall time.

## State machine

```text
initialize (seed) -> decisions / progress
    -> official terminal:
         freeze OfficialOutcome ONCE
         shadow continuation starts using same state copy
         for each dead player, fixed post-death policy
         for each newly dead player, freeze first-death record
    -> shadow common horizon -> ShadowOutcome
```

No official terminal: game continues officially until its usual end; a shadow score at the experimental common horizon must still use a clearly documented time convention. If the official horizon is also the common horizon, results may coincide.

After death, never use the normal search policy again for that player, irrespective of later population. Never erase first death. When both die, continue under fixed policies until the same horizon (unless the adapter defines a truly unavoidable termination, which is logged as a censored result, not a comparable score).

## Determinism and randomness

Define distinct seed streams for setup, progression deck, building draws, action-level randomness and shadow adapter. RNG event identities should be stable enough for paired counterfactuals; identical integer seeds by themselves are insufficient if a branch consumes different numbers of random values.

Support state snapshots, replay logs, deterministic serialization, versioned adapters, and a replay verification command.

## Interfaces (illustrative, not implemented)

```cpp
struct Observation;
struct GameState;
struct Action;
struct EventClock;
struct DeathRecord;
struct OfficialOutcome;
struct ShadowOutcome;

class Agent {
public:
    virtual Action choose(const Observation&) = 0;
    virtual ~Agent() = default;
};

class Engine {
public:
    Observation observe(const GameState&, int player) const;
    // Must support simultaneous commitments without leakage.
    // Advance only to the next genuine decision point.
};
```

An agent's legal-action queries must be derived from its legally available information. Before coding, finalize explicit representations for multi-step card effect resolution, simultaneous commitments, and publicly known vs unknown deck information.

## Required tests

- Same seed/actions/version -> identical event log, official outcome and shadow outcome.
- Observations redact opponents' hands and future deck order.
- Simultaneous commitments prevent reading the other player's pending move.
- First-death timestamp is exact and immutable, including on negative deficit and recovery.
- Post-death policy remains fixed after any positive shadow recovery, and even if a normally appealing BUILD action becomes available.
- A death at an early event and late event have different death-time records/penalties.
- The other player continues after the first official death to the common horizon.
- Shadow continuation cannot mutate frozen official outcomes.
- Both players use the same defined horizon and preserve their own first-death times.
- Adversarial case: early death plus very high shadow score must not accidentally win under a configured death-first objective.
- Paired counterfactual results retain both branches with audited random coupling.
- Tiny toy game exact enumeration vs search validates expected outcomes and no hidden-information leakage.

## Profiling

Measure game completions/sec, decision rollouts/sec, memory and state-copy cost. Do not optimize for decision speed at the expense of search correctness. Record hardware, compiler and runtime config.
