# Roadmap for Codex

Each milestone must produce code + tests + a brief reproducible benchmark when applicable. Stop at rule ambiguities; document them in RULES_CHECKLIST rather than making up rules.

## M0 — Source-of-truth and scaffolding (first PR)
- [x] Document authoritative rulebook source/version and card catalog completeness (source found; edition and catalog explicitly unverified in RULE_PROVENANCE.md).
- [x] Create C++20 CMake project with CTest and a minimal smoke test.
- [x] Define versioned `EventClock`, `DeathRecord`, `OfficialOutcome`, `ShadowOutcome` types.
- [x] Define seeded RNG and deterministic replay log contract.
- [x] Set up formatting, compiler warnings and CI where available (CI configured; local MSVC tested).
**Accept:** clean build/test instructions, no hidden rules assumptions, sample toy event replay reproducible.

## M1 — Minimal verified two-player engine
- [x] Model full/private state vs player observations and simultaneous commitments (toy contract only; official visibility still unverified).
- [ ] Apply verified setup, basic Work choices, progression/death/terminal transitions.
  Partial: `base_rules::setup_fragment` validates supplied 90-ID Building deals
  and initial resources. Conditional Mine/Farm/Research, deferred Building
  reshuffle and replay are implemented; complete setup and other transitions
  remain open. See M1_IMPLEMENTATION.md.
  Additional partial coverage: Progress deck recycle/addition/clock primitives
  and conditional forced population loss with explicit Building choices and
  failure diagnostics. Unknown LOST and terminal interruption remain blocked.
  See M1_PROGRESSION_POPULATION.md; whole-game transitions are still incomplete.
  Connected conditional slice: Session dispatches reviewed Work/Trouble/Hunger/
  Accident/Instruction Manual paths, first exact-zero death and unique terminal
  result, with explicit stops and command replay. No full setup/catalog or
  production shadow continuation. See M1_SESSION.md.
- [ ] Add verified card effects one at a time with fixtures and provenance.
  Partial: the five conditional session paths above have source audit and
  targeted fixtures; full card coverage remains unimplemented.
  Paperwork now has a conditional draw/discard frame with hidden commitments,
  explicit schedules, partial quotas and a scarce-supply stop. Session schema 2
  includes frame checkpoints and selected cards. See M1_PAPERWORK.md.
  Glitch now consumes a supplied private Robot source; Butler/Waiter/Mining
  Robot have reviewed nonterminal no-Building paths, conservative loss guards,
  and schema-3 replay/JSON. See M1_GLITCH_ROBOTS.md; other Robots remain open.
  First typed Building: Kale Farm cost/printed population/PLAY Farm, cumulative
  own Farm triggers, supported BUILD/RESTOCK choices/boxes, printed population
  scoring and schema-4 rich traces. Unknown Buildings reject and affected
  Events block. See M1_TABLEAU.md; full catalog remains open.
  Nonterminal Kale Farm population loss now has an atomic adapter to shared
  Building state with explicit choices and transfer/box records. Session effect
  frames and terminal cases remain pending. See M1_TABLEAU_LOSS.md.
  Accident now connects nonterminal Kale Farm loss choices with private
  commitments, explicit resolution order and schema-5 replay/JSON records.
  Other affected Events/Robots and terminal cases remain open (M1_ACCIDENT_CHOICES.md).
  Hunger now pays Building-count food and base-funded nonterminal shortages,
  applying +2 only without population loss, with schema-6 audit records.
  Building-loss and terminal frames remain pending (M1_HUNGER.md).
  Hunger Building-loss choices now preserve original food requirements, private
  commitments and ordered transfer/box records in schema 7. Terminal cases and
  other Building definitions remain open (M1_HUNGER_CHOICES.md).
  Waiter/Mining Robot now support base-funded nonterminal Kale Farm contexts
  with schema-8 payment/tail records. Butler box choices, Robot Building losses
  and terminal cases remain open (M1_ROBOT_TABLEAU.md).
  Nonterminal Waiter/Mining Building losses now have private full choices and
  all-player population-before-tail resolution with schema-9 records.
  Butler box choices/terminal cases remain open (M1_ROBOT_CHOICES.md).
  Butler population loss and zero/single-target box tails are supported with
  schema-10 partial/completed records. Multiple boxed Buildings stop pending
  authoritative selector clarification; terminal cases remain open (M1_BUTLER.md).
  Schema-11 trace assessments now report waiting/blocked/in-progress/conditional
  terminal states with explicit reason tags. Accident terminal interruption with
  Building choices remains unverified (M1_TRACE_ASSESSMENT.md).
  Schema-12 adds the p11 Budget Cuts money-loss handler, capped at available
  money and recorded per player (M1_BUDGET_CUTS_AUDIT.md). Development deck
  setup and other card triggers remain unimplemented.
  Schema-13 adds the p10 Flag Building: zero cost/printed people, +6 money on
  PLAY, no Hunger food requirement, and nonterminal printed-population loss
  support. Exact-zero terminal handling remains unresolved (M1_FLAG.md).
  Schema-14 adds Potato Farm cost/population/FARM bonus and a privately committed
  food-for-money split on BUILD when Potato Farm is already in play. Money-loss
  effects remain money-only (M1_POTATO_FARM_AUDIT.md).
  Moon Hotel p10 notes confirm the cited +4 FARM and +1 Kale Farm food gains
  yield +2 people, but the local card-face render does not establish its full
  card text or trigger boundaries. It remains unimplemented pending source
  clarification (M1_MOON_HOTEL_AUDIT.md).
  Fuel Cells p10 confirms spending a box placed by the same RESTOCK for +4
  money, then drawing and discarding a Building card. Its full card face and
  post-draw private discard frame remain pending (M1_FUEL_CELLS_AUDIT.md).
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
