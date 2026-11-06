# M1 conditional card-resolution session

This connects the base Work, progression and population primitives into an
executable conditional trace. `engine/session.*` supports a small reviewed card
subset and stops on unsupported semantics. It is not a full game, an initial
state sampler, a legal action enumerator, or a strategic AI.

The [independent card audit](M1_SESSION_AUDIT.md) cites the pinned publisher-authored
rulebook reproduction. Its match to the publisher's current bytes/physical edition
is still unverified. `engine/session_manifest.txt` records the implemented facts
and conventions; its SHA-256 includes the base-rules and progression/population
manifest hashes, so changing a dependency changes this session's identity.

## Scope and input

`Input` contains a supplied base-state fragment, Progress state, seed, physical
ID-to-effect mapping, and ordered remaining Event IDs. This mapping is caller
data, not a fabricated official catalog. The caller must review Event order;
no default Event sequence or initial Progress deck is invented. Known Work or
Trouble instances are rejected as Event queue entries. Unknown Event text is
allowed as opaque input, but blocks as soon as revealed.

The reused `EffectScope::caller_verified_unmodified` flag certifies **all**
relevant modifiers are absent, including Event/terminal effects, not merely
Work bonuses. Every colony must be living, have no Buildings, and fit the score
type. The API cannot automatically certify this flag: a future full engine must
establish it from verified effects. The real initial game includes Twists;
this conditional input is not a replacement setup.

A session starts a new local trace at a resolved boundary, with zero Progress
clock/counters. It rejects active-card or nonzero-clock inputs, rather than
inventing prior first-death history. It does not restore arbitrary midgame state;
replay reconstructs a trace from its original supplied input. Clock coordinates
are relative to this trace, not a reconstructed earlier game.

## Implemented card paths

| Effect | Source | Conditional handling |
| --- | --- | --- |
| Work | p4 | Open one simultaneous decision; both commits required; resolve supported Mine/Farm/Research using an explicit logged caller schedule. |
| Trouble | p6 | Add exactly one next queued Event on top; finish Trouble; that Event is next. |
| Hunger | p6 | No Buildings means no food payment/loss and +2 base people for both. |
| Accident | p2 card image, p6 | Printed loss is 3 for each player. Resolve only when both have at least 3 people. |
| Instruction Manual | p6,p7 | Stop and award the conditional official result to a unique greatest population; leave ties unresolved. |

BUILD, RESTOCK, Paperwork, Glitch, Robot/Twist/Development/Perk effects and all
Building triggers remain unsupported. Unknown effects retain the active card;
they are never discarded as no-ops. The session does not implement a rescue or
fallback action to pass an unresolved boundary.

`advance` processes at most one Progress reveal. During Work it returns the same
decision boundary without revealing another card. One simultaneous Work point
increments the decision counter once on entry; commitments/resolution do not
increment Progress or decision clocks again. Pending choices do not enter the
other player's observation. `resolve_work` clears them only after successful
atomic resolution.

## Terminal and blocked states

For plain simultaneous Accident with both populations at least 3, both complete
losses can be paid without Buildings or further effects. Resolve both, record
any exact-zero first death at the shared event clock, and freeze scores/winner
once. This limited complete plain-loss case does not resolve the earlier
ambiguous unpayable-payment/LOST interruption path.

If either player has fewer than 3, stop **before either colony's population
changes**. Keep the revealed Accident active, and emit no death/outcome or final
score claim. `blocked_population_payment` is an unresolved rule boundary, not
a surviving/finished game. Exact equal terminal scores use `unresolved_tie`,
retaining raw terminal evidence and any already-established deaths but no
invented official draw. No-Progress-card and missing-Event states are also
explicit stops, not guessed natural ends or horizons.

`official_terminal` is frozen through the Session API; subsequent `advance`
calls do nothing and Work actions reject. There is no shadow continuation here.
All current phases, in numeric JSON order: ready=0, work_decision=1,
blocked_unknown_card=2, blocked_event_queue=3, blocked_population_payment=4,
unresolved_tie=5, no_progress_cards=6, official_terminal=7.

## Observation and records

Only `observe(player)` is policy input: own hand, public resources, top Building
discard, active/top Progress discard, current clock/phase and frozen outcome.
The action list is labelled supported operations, not full legal actions.
Snapshot/Replay/JSON contain private engine truth and must not enter policy input.

`record` saves the complete supplied input/config and commands plus a checkpoint
after every command. `verify` checks schema, dependency-aware hash, command count
and each reconstructed checkpoint. This is deterministic in-memory replay, not
a cryptographic signature or an arbitrary external-snapshot validator. Equal
behavior under an altered unused input field is not an integrity/authentication
claim. JSON export runs verification before writing all input, commands,
checkpoints, final state, deaths, outcomes, seed and manifest IDs.

JSON explicitly labels the conditional scope and `full_game=false`; the result
is named `conditional_official_outcome`. Shadow is null and score adjustments
are zero. No incomplete trace is promoted to a comparable full-game sample.
The existing M0 JSON/text formats are unchanged. No C++ JSON importer or
versioned persistent-checkpoint restoration is implemented in this slice.

## Run and validate

Use the README CMake commands. On the current Windows machine:

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
& ./build/Debug/session_demo.exe
```

The demo is `conditional-script-v1`: synthetic Building IDs, supplied Progress
sequence and initial populations [30,29], not a legal initial game distribution.
It executes Work, Trouble, the added Hunger, another Hunger, Accident and
Instruction Manual. The final conditional scores are [31,30], at Progress
event 6 and simultaneous decision 1. Python independently parses JSON, checks
both card families' conservation across checkpoints and validates these results.

Independent C++ tests cover the end-to-end flow, both commitment orders,
privacy, exact-zero deaths, unresolved ties, all-player payment stop, missing
effects/Events, no-card stop, overflow atomicity and 32-seed replay/tampering.
Tests were authored with disjoint ownership and reviewed by the lead. An
independent source/semantic review covers the dispatcher and exporter.

Final local Debug build and CTest: **8/8 passed** (1.33 seconds), including the
new C++ session and independent Python JSON tests. `git diff --check` passed.
Independent review found no remaining P0/P1/P2 issues.

## Reproducible microbenchmark

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Release --target session_demo
& ./build/Release/session_demo.exe --benchmark
```

This measures **1,000 synthetic trace recordings plus complete re-verification**
with seeds 0..999 and nine commands per trace. It excludes JSON output and is
not game-completions/sec, search strength or an improvement claim. Record the
compiler, machine and configuration alongside each measurement; the checksum
must be 6000. The fixture contains no shuffle, so these seeds document the run
rather than prove stochastic game coverage. Seeded reshuffle coverage belongs
to the contract tests.

Recorded baseline: Windows x64, AMD Ryzen 7 3700X 8-Core Processor, MSVC 19.51,
CMake 4.4.4, Release: 1,000 record+verify traces in **0.0631695 seconds**,
approximately **15,830 traces/sec**, checksum 6000. This is one local measurement
of this small supplied scenario, not a claim about full-game throughput.

## Remaining M1 work

Verified full catalog/setup, Building targets and triggered effect frames,
remaining cards, Event order, shared-draw priority, unpayable loss interruption,
ties, full game replay and production shadow conventions remain open. This slice
adds actual reviewed card transitions but does not mark M1/M2 complete or justify
an opening Research recommendation.
