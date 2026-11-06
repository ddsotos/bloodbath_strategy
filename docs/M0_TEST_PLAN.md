# M0 black-box acceptance plan

Status: independent test specification for scaffolding and the initial observation
boundary. Test cases below are planned requirements, not a claim that every case
is implemented or that Moon Colony Bloodbath rules are implemented.

Sources: [ARCHITECTURE.md](ARCHITECTURE.md),
[RESEARCH_DESIGN.md](RESEARCH_DESIGN.md), [ROADMAP.md](ROADMAP.md),
[RULES_CHECKLIST.md](RULES_CHECKLIST.md), and [MULTI_AGENT.md](MULTI_AGENT.md).

## Scope and assumptions

All population thresholds, scores, actions, progression schedules and horizons in
these fixtures are **toy conventions**, unrelated to verified game rules. Use
explicit fixture/rules/adapter identifiers in records. No gameplay fidelity,
opening recommendation or search strength follows from passing these tests.

M0 should exercise value contracts, deterministic replay and toy transitions;
the initial M1 slice should exercise observations and simultaneous commitments.
The continuation tests describe a toy harness that validates future M2 contracts.
Full card rules, production search and statistical claims remain outside scope.

## Test cases

| ID | Public input / operation | Required observable result |
| --- | --- | --- |
| BUILD-1 | Configure, build and run CTest from a clean build directory | C++20 library, tests and replay tool build; tests run without downloading a test framework. |
| RNG-1 | Repeat identical seed, configuration, version and actions | Identical random choices, deterministic record and raw official/shadow outcomes; elapsed runtime is excluded from deterministic equality. |
| RNG-2 | Consume extra setup or action random values before a progression draw | Progression and building sources retain their original draws. Repeat for every declared source, including shadow adapter. |
| RNG-3 | Query stable random event identities in different orders, with extra unrelated queries | Shared identities yield identical values in paired branches. Duplicate identity semantics are documented. This validates the toy coupling only; deck mutation needs its own mathematically justified coupling later. |
| REPLAY-1 | Replay a stored toy record and compare to original | Reconstructed clocks, actions, choices, first deaths and both outcomes agree. A snapshot plus suffix replay agrees with uninterrupted execution if snapshot support is included. |
| REPLAY-2 | Change one logged choice, action, version, rule hash or event clock; truncate or append invalid data | Replay fails with a nonzero result and identifies the mismatch; it must not silently regenerate a different valid record. |
| OBS-1 | Compare states differing only in opponent hand or future deck order | Observations and legal-action lists for the unchanged player are equal. Own private hand remains available to its owner. No pointer/reference to full state is exposed through the policy API. |
| OBS-2 | Change public history or the player's own hand | Observation changes as specified, avoiding a vacuous test that always returns an empty observation. |
| COMMIT-1 | Commit player A's action, inspect B's observation, then commit B | B cannot read A's pending choice before both are locked. Resolution returns both choices only after the second commitment. Test both commitment orders. |
| COMMIT-2 | Submit duplicate, invalid-player or illegal commitments; request early reveal | Reject without revealing or replacing a previously locked action. Hidden truth must not affect whether an otherwise identical observable action is legal. |
| CLOCK-1 | Advance one decision, one progression event and one cycle boundary separately | Each counter advances according to the documented indexing convention. Decisions alone do not masquerade as progression reveals. First and final event indexing are checked explicitly. |
| DEATH-1 | Record first death, then later death with a different cause and signed shortfall | First record and permanent mortality flag remain unchanged; exact cycle, within-cycle, absolute event and decision coordinates survive copying and replay. |
| DEATH-2 | Compare never-dead, death at first event and death at final horizon event | Three distinct states; never-dead uses an absent record, never an overloaded final timestamp. |
| OUTCOME-1 | Freeze terminal result, continue shadow state and attempt a second freeze | Official winner/draw, scores, cause and timestamp remain those of the first terminal trigger. Mutating a shadow result or copied state cannot mutate the frozen official value. |
| SHADOW-1 | First player dies early; opponent dies later | Both continue to the configured common horizon. Each first death keeps its own time. Both use the same fixed-policy specification after their respective deaths. |
| POLICY-1 | Dead player later recovers positive population and gains an attractive toy action | Normal-agent call counter stays unchanged after death. The configured deterministic mapping remains active; repeated equal allowed observations give equal actions, independent of legal-action enumeration order if the policy specifies a priority. |
| POLICY-2 | Dead player has no legal actions | Documented no-action advance is used; no normal search call and no infinite retry. |
| HORIZON-1 | One death, two deaths, no deaths, and death exactly at the horizon | Both final scores are measured at the explicit common horizon. No-death records remain absent. The final-event death is retained. |
| HORIZON-2 | Adapter cannot advance to horizon, or requested horizon precedes terminal state | Reject invalid configuration or mark result censored with actual stopping time; never export ordinary comparable common-horizon scores. |
| RECORD-1 | Serialize a complete toy experiment | Include schema/rules/adapter/policy versions, rule hash, seed, explicit config and horizon, branch/coupling identity, both official scores, both shadow scores, mortality and synthetic adjustments separately. Parse valid JSON independently. |

## Explicit mortality fixture

Use a named toy adapter, for example `toy-mortality-v1`, with a caller-supplied
bounded horizon and event schedule. The fixture may use signed toy population,
declare `population <= 0` its toy death predicate, and receive explicit score
vectors at each event. These values are fixture inputs, never game defaults.

At event A player 0 dies and the official result freezes. At later event B,
player 0 recovers positive toy population and receives an exceptionally large
shadow score. At event C player 1 first dies. At horizon H both players have
final shadow scores. Assert `A < B < C <= H`, immutable official scores at A,
first-death points A/C, permanent fixed policy after each death, and final scores
at H for both players. Run variants with no recovery, repeated death/recovery,
both players dying on one event, and a surviving opponent.

Preserve raw scores without adding an arbitrary mortality bonus. When a
configurable death-first objective is introduced, add an adversarial assertion
that this early-death/high-shadow-score branch cannot win solely through wealth.
Until that objective exists, test the permanent mortality and raw components
and leave scalar ranking explicitly unimplemented.

## Completion evidence

Map implemented case IDs to test names and record exact configure/build/CTest
and replay commands in the implementation notes. Distinguish passing cases,
deferred cases and unverified official rules. A planned test is not a passing
test, and a toy transition is not an official game transition.
