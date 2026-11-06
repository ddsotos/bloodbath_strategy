# Roadmap for Codex

Each milestone must produce code + tests + a brief reproducible benchmark when applicable. Stop at rule ambiguities; document them in RULES_CHECKLIST rather than making up rules.

## M0 — Source-of-truth and scaffolding (first PR)
- [ ] Document authoritative rulebook source/version and card catalog completeness.
- [ ] Create C++20 CMake project with CTest and a minimal smoke test.
- [ ] Define versioned `EventClock`, `DeathRecord`, `OfficialOutcome`, `ShadowOutcome` types.
- [ ] Define seeded RNG and deterministic replay log contract.
- [ ] Set up formatting, compiler warnings and CI where available.
**Accept:** clean build/test instructions, no hidden rules assumptions, sample toy event replay reproducible.

## M1 — Minimal verified two-player engine
- [ ] Model full/private state vs player observations and simultaneous commitments.
- [ ] Apply verified setup, basic Work choices, progression/death/terminal transitions.
- [ ] Add verified card effects one at a time with fixtures and provenance.
- [ ] Test card conservation, negative resources if applicable, deck mutation/shuffling and legal choices.
**Accept:** tests cover implemented rule subset; never claim full coverage before full card catalog passes.

## M2 — Official outcome vs shadow continuation
- [ ] Freeze official results at the first official terminal event.
- [ ] Define common horizon and shadow rule adapter explicitly (versioned).
- [ ] Implement first-death recording by event/cycle and first-negative/zero semantics as verified.
- [ ] Implement deterministic fixed post-death policy (configurable, permanently assigned).
- [ ] Continue the non-dead opponent after official terminal; handle second death independently.
- [ ] Export paired official + shadow result schema.
**Accept:** all mortality/continuation invariants in ARCHITECTURE tests pass, including wealth-then-death recovery exploit fixture.

## M3 — Reproducible rollouts and baselines
- [ ] Seeded batch runner with structured output.
- [ ] Legal random agent and clearly versioned simple heuristic.
- [ ] Separate own and opponent shadow scores, official outcome and mortality metrics.
- [ ] Paired action evaluation with confidence intervals.
**Accept:** replayable rollouts and analysis of paired trials.

## M4 — Strong search benchmark
- [ ] Implement PIMC baseline; explicitly check information leakage/strategy fusion.
- [ ] Implement information-set search for hidden information and simultaneous Work.
- [ ] Compare search strength vs *compute budget* using independent held-out seeds.
- [ ] Verify tiny-game results against exact enumeration.
**Accept:** documented search convergence curves and unbiased benchmark setup.

## M5 — Learned search assistance
- [ ] Add Python/PyTorch training interface (if M4 results justify it).
- [ ] Train optional value heads for own/opponent shadow scores, official outcome, death timing.
- [ ] Add prior/value inference to search; benchmark same-compute improvements.
- [ ] Self-play with historical checkpoints; keep non-learning baselines.
**Accept:** gains reproducible across held-out cases; no leakage in observations.

## M6 — Research experiments
- [ ] First-Work draw vs best non-draw conditional policy with equal budget.
- [ ] Counterfactual add-vs-omit shared deck cards with opponent rerouting.
- [ ] Compare mortality-first vs smooth mortality penalties; report both shadow scores and official wins.
- [ ] Publish confidence intervals, seed/hardware/provenance/limitations.
**Accept:** stable, independently validated opening recommendations, not merely one self-play model's preferences.

## First executable Codex prompt

> Read AGENTS.md and docs/*.md. Implement M0 only, including a compiling C++20/CMake skeleton with CTest; EventClock/DeathRecord/OfficialOutcome/ShadowOutcome and seeded replay interfaces; toy tests for first death freezing and paired official/shadow outcomes. Do not invent Moon Colony Bloodbath card rules. Report build/test commands and any rulebook gaps. Open a focused PR.

## Execution policy
See [MULTI_AGENT.md](MULTI_AGENT.md): lead owns design/integration; delegate independent source verification, black-box test planning and build/CI to up to three subagents when supported, with disjoint file ownership and review gates. Do not block M0 on subagent availability.
