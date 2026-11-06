# Instructions for Codex agents

You are implementing a reproducible research platform for a **two-player Moon Colony Bloodbath AI**, not a production game UI.

Read `README.md`, `docs/RESEARCH_DESIGN.md`, `docs/ARCHITECTURE.md`, `docs/ROADMAP.md`, and `docs/RULES_CHECKLIST.md` before coding.

## Core invariants
1. **No invented game rules.** If the authoritative rules/cards are missing or ambiguous, leave a documented TODO and use clearly labelled toy fixtures in tests; don't silently claim fidelity.
2. Separate `OfficialGame` from `ShadowExperiment`. The official result freezes at the **first official terminal event**; a shadow rollout continues both players to a **common, explicitly defined horizon**.
3. Record for each player the **first death event**, including round/cycle, within-cycle event index, absolute event index, and cause. Once dead, `ever_died=true` is permanent even if shadow population rises. A player who never dies has a distinct sentinel/optional field.
4. Post-death action selection is **deterministic fixed policy** (configured and versioned), not a survival optimizer, not continued search, and not an invitation to exploit recovery. Apply the same fixed-policy specification across compared experiments.
5. Retain separate measurements: official win/draw/loss, official terminal scores, shadow final scores of both players, death times, and any synthetic adjustments. Never call shadow scores official.
6. Search agents must observe only permitted information. Never expose deck order or opponent hand in their observation. Search must not leak future draws at decision time.
7. Randomness must be seeded, reproducible and separated by source; use paired experiments with sound coupling, not only identical RNG seeds and assumptions about identical sequences.
8. Optimize for research accuracy and testability first, then simulation speed. No premature neural network or complex search until baselines are measured.
9. Action enumeration must support simultaneous commitments without revealing the other action before both are locked.
10. Never make up initial resources, card counts, deck composition, horizon or scoring rules; verify and cite their source in docs.

## Coding
- Prefer C++20 + CMake and standard library for engine/search; thin pybind11 and PyTorch integration later, only if justified.
- Use small explicit value types for `GameState`, `Observation`, `Action`, `Transition`, `DeathRecord`, `ExperimentResult`.
- Write tests alongside engine changes; add property/invariant tests for replay, information hiding, continuation and mortality.
- Keep synthetic shadow-only state separate from real game state. If official transitions become undefined after death, implement an explicitly documented `ShadowRulesAdapter`; never pretend these are official rules.
- Produce machine-readable structured experiment records with schema version, rule-set hash, policy/model version, seed, game config, and raw results.
- Do not implement arbitrary death bonuses or arbitrary score weights as settled facts. Make objectives configurable, retain components, and validate against official wins.

## Workflow for each task
1. State the documented rule assumptions and files to modify.
2. Implement a narrow vertical slice, tests and benchmark when relevant.
3. Run tests locally; report exact commands/results and known rule gaps.
4. Update docs/checklist before marking functionality complete.
5. Prefer focused PRs; avoid bulk fabricated card implementations.

## Immediate first task
Implement Roadmap Milestone 0 and the initial part of Milestone 1: CMake project with tests, RNG/replay abstractions, observation boundary, event-clock / death-record data types, and separate official vs shadow outcome types. Use toy transition fixtures *only if labelled*, until authoritative rule details are available. Do not claim a playable or strategically strong AI yet.
