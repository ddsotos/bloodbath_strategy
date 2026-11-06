# Moon Colony Bloodbath — strategy research

Research project to build a **strong two-player AI** for *Moon Colony Bloodbath* and determine **whether, and under which opening conditions, Research/draw on the first Work is optimal**.

**Status:** design-first bootstrap. No full rules implementation, trained AI, or validated result exists yet. Rule details and terminology must be checked against an authoritative rulebook before implementing them.

## Agreed research constraints

- Exactly **two players** in the initial research.
- Prioritize decision quality over runtime per decision; CPU/GPU budget can be substantial.
- Build a faithful, deterministic, testable **C++20 game engine**, with Python/PyTorch as a possible learning and analysis layer.
- Compare first-Work Research/draw to the **best non-draw alternative under the same observation**, not merely a fixed other action.
- Track **own final score**, **opponent final score**, **official outcome**, and **first death timing** as separate data. Do not reduce everything to win/loss or score difference.
- When either colony dies, record its first death but **continue a separate research-only shadow rollout for both players to a common horizon**. After a colony's first death its action policy is **fixed**, not strategically optimized or switched back after recovery. Never present shadow scores as official scores.
- Evaluate shared-deck event/robot/development additions with paired counterfactual rollouts (add vs. do not add), including the opponent's response.
- Start with a baseline heuristic + reproducible rollouts and benchmark PIMC / information-set MCTS; add policy/value learning only when it yields measured improvement. No guarantee that any named algorithm is best.
- The experiment must distinguish observed rules from research-specific conventions and assumptions.

## Repository map

- [AGENTS.md](AGENTS.md): Codex instructions, quality gates and initial assignment.
- [docs/RESEARCH_DESIGN.md](docs/RESEARCH_DESIGN.md): experiment objectives, evaluation and first-death continuation.
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): engine/search boundaries, data contracts, testing and benchmarks.
- [docs/ROADMAP.md](docs/ROADMAP.md): ordered implementation milestones with acceptance criteria.
- [docs/RULES_CHECKLIST.md](docs/RULES_CHECKLIST.md): rule ambiguities that must be resolved before claiming game fidelity.

## Recommended first Codex task

Read `AGENTS.md` and `docs/ROADMAP.md`, then implement **Milestone 0 / 1**: obtain or document the authoritative rules, create a CMake C++20 engine/test skeleton, add seeded RNG and replayable transition interfaces, and write tests for the distinction between *official end* and *shadow continuation*. Do not invent card effects.

After those foundations pass, implement complete rules and cards incrementally with fixtures. Analysis of first-Work draw must wait until rule completeness and benchmark strength are validated.

## Research integrity

A shadow continuation does **not** change the official loss. A colony that first reached zero (or otherwise died under official rules) remains marked dead for evaluation even if a synthetic continuation later accumulates population. Any scoring formula using death timing, severity, and shadow score is experimental; log raw components so alternative objectives can be tested without rerunning games.

## Codex delegation
See [Multi-agent collaboration policy](docs/MULTI_AGENT.md). Preferred setup is a Sol lead with up to three Luna subagents for independently verifiable rule research, tests, and CI. Availability of those exact models/subagents depends on the Codex environment; serial work is an acceptable fallback. The lead owns integration and scientific decisions.
