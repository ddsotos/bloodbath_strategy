# Codex multi-agent collaboration policy

This document defines **responsibilities**, not a claim that any particular Codex deployment exposes a specific model or subagent feature. If GPT-6.1 Sol / GPT-6 Luna or model pinning is unsupported in the current Codex environment, use available agents with equivalent roles, or complete work serially. Verify capabilities in the actual Codex session.

## Roles and delegation

**Lead agent (preferred: Sol):** owns architecture, rule-engine APIs, official vs shadow semantics, information sets, evaluation objectives, search algorithms, integration, and final tests/PR. The lead must retain all substantive design decisions.

**Research agent (preferred: Luna A):** collect authoritative rulebook and card catalog sources; produce traceable rule manifests and an uncertainty ledger. Separate exact rules from hypotheses and experimental conventions. Do not implement guessed effects.

**Test agent (preferred: Luna B):** independently derive invariants and black-box tests from approved specifications. Focus on simultaneous action secrecy, replay determinism, first-death timestamp permanence, fixed post-death action policy, shadow continuation of *both* players to a common horizon, and frozen official outcomes. During M0, author test *plans* / toy fixtures; after API stabilizes, implement tests.

**Build/benchmark agent (preferred: Luna C):** own CMake/CTest/CI, build matrices, sanitizer runs, replay tooling and microbenchmarks. Do not assert performance gains without measurements.

Later: delegate independent card fixtures, static rule review, exact-solver tests for tiny games, statistical analysis scripts, and experimental-result audits. Keep core ISMCTS design, game rules reconciliation and objective choices under the lead agent.

## Task assignment protocol

- Lead first writes a brief work breakdown: output files, dependencies, acceptance criteria, and expected handoff format for each agent.
- Prefer **one owner per file**. For shared files, use review comments/patch suggestions instead of concurrent edits. Isolate work in separate branches/worktrees where supported; never have agents simultaneously change `AGENTS.md`, primary state types, or the same engine files.
- Set a finite number of independently useful subtasks (initially 2–3), not a swarm. Avoid delegating tasks that need constant design negotiation.
- Each subagent returns: files changed or report link, sources, assumptions, commands/tests run, findings, and open questions. The lead reviews and merges rather than auto-accepting.
- If subagents can't be invoked in the user's tool configuration, proceed with the lead agent and record that delegation was unavailable. Do not block M0.
- Do not grant agents permission to silently fabricate rulebook content, reweight objectives, alter death semantics, or commit unreviewed generated game rules.

## Mandatory review gates

1. **Rule provenance**: no rule/card effect enters the production engine without source and a targeted test.
2. **Observability**: opponent hand and future deck order absent from policy input and information-set search.
3. **Experiment separation**: official outcome frozen; shadow extension clearly labeled; both players reach the same horizon; first-death immutable.
4. **Mortality integrity**: no reward for strategic death/recovery exploits; explicitly test early death with exceptional shadow score. Fixed post-death policy stays fixed *even after positive recovery*.
5. **Statistical integrity**: held-out seeds, paired runs with valid coupling, confidence intervals, and reported runtime / rollout count.
6. **Scientific humility**: no claim of optimality from one model, one opponent, a self-play win rate of 50%, or stability at one search budget.

## Cost and parallelism

Parallelize rules inventory, test planning and CI independently while the lead designs the engine. Avoid running expensive simulations in each subagent by default. Keep benchmark seeds, compute budgets and hardware specifications explicit.

## Sample lead prompt

Read `AGENTS.md`, `docs/ROADMAP.md`, `docs/RESEARCH_DESIGN.md`, and `docs/MULTI_AGENT.md`. Implement Issue #1 (M0) as a focused PR. Own architecture and official/shadow contracts yourself. If supported, delegate rule-source verification, independent test design, and CI/build setup to up to three Luna subagents with separate file ownership. Require source-backed reports and tested deliverables. Merge only after reviewing changes, running CTest and summarizing remaining rule gaps. If model-specific delegation is unavailable, carry out the same tasks with available agents or serially; do not pretend subagents ran.
