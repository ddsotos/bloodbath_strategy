# M1 progression and population-loss primitives

This slice adds `engine/progression.*` and `engine/population.*`. They are
conditional engine primitives, not a complete game, a verified card catalog,
or production shadow continuation. The earlier toy engine and base Work module
remain separate. See [M1_PROGRESS_AUDIT.md](M1_PROGRESS_AUDIT.md) for the
independent rule review and source limits.

## Sources and conventions

Rule facts were reviewed on printed pages 4, 7 and 8 of the publisher-authored
English rulebook reproduction pinned in [M1_RULE_AUDIT.md](M1_RULE_AUDIT.md).
Publisher/current-file/physical-edition equality is still unverified.
`engine/progression_population_manifest.txt` lists the facts, assumptions and
representation choices. CMake exports a SHA-256 of this manifest after CRLF
normalization; both modules expose `subset_manifest_hash`. This identifies the
conditional subset, never a complete official ruleset.

All test card IDs, printed populations, boxes and requested loss amounts are
explicit synthetic inputs. They are not new official card definitions.

## Progression deck

`Deck` takes a caller-supplied snapshot and seed. It handles physical card IDs
only, without dispatching their effects. It cannot infer the initial Progress
deck, card catalog, Trouble/event ordering or when a particular addition is legal.
The future whole-game scheduler must supply those verified decisions.

- Deck back is top. A revealed card occupies `active` until the effect resolver
  calls `finish_active`; it is logically already the public face-up current
  discard. This internal separate zone preserves card conservation.
- Another reveal is forbidden while active. `finish_active` is the caller's
  assertion that every effect/choice is complete; the deck does not implement
  card effects or silently skip unknown instructions.
- Recycle occurs only when another reveal is requested and the deck is empty.
  A card inserted on an empty deck is therefore next, without early recycling.
- One `add_group` is one externally determined simultaneous addition group;
  it is shuffled together and placed above the existing deck. No existing
  discard or future card moves into that group. Duplicate physical IDs reject.
- Observation exposes only active/top discard and the event clock. Future order
  and lower discards remain engine-only. Snapshots contain private truth and
  must not be passed to policies.

Clock convention: initially all zero; a Progress reveal increments absolute and
within-cycle position. A recycle increments cycle and starts its first reveal
at position 1. Insertions do not start a new cycle. Explicit `record_decision`
requires an active card and increments only the decision index. These indexing
choices are engineering conventions, not claims about printed round numbering.

Recycle and grouped-addition randomness use the progression source with separate
indices and even/odd draw identities, as detailed in the manifest. Preserve seed,
manifest hash and snapshot together when recording a checkpoint. Snapshot+seed
reexecution is tested; no serialized checkpoint interchange format or audited
deck-insertion counterfactual coupling is claimed. Counters reject overflow,
and rejected operations leave the deck state unchanged.

## Forced population loss

`Loss` is a single-colony operation requiring a **living** input, player, event
clock, amount and cause. Caller-supplied Building metadata includes printed
population, boxes and whether LOST effects are verified absent. This is not a
card definition loader or a full state adapter to the base Work module.
The later [Kale Farm adapter](M1_TABLEAU_LOSS.md) connects these primitives to
shared Building state for strictly nonterminal losses only.

If base population covers the amount, pay the full amount once. Zero base is
not death while printed Building population remains. If base is insufficient
but total population covers the loss, expose owned Building choices. On a
supported choice, transfer all printed people, discard that Building and its
boxes, then reassess whether another choice is needed. Do not pay piecemeal.
The caller may choose any owned Building, rather than an optimized/default one.

Selecting a Building with unverified LOST effects rejects before mutation.
No effects are skipped and no next loss is processed through that boundary.
Future triggered-effect processing must run all LOST effects before resuming.

Statuses are deliberately distinct:

| Status | Meaning |
| --- | --- |
| `needs_building_choice` | Caller must choose a Building; unsupported LOST may block that choice. |
| `needs_terminal_rule` | Positive total population is insufficient and Buildings remain. Interruption vs LOST order is unresolved; stop without payment, destruction or death record. |
| `paid` | Full requested loss paid and total population remains positive. |
| `failed` | This operation reduced total to zero, or cannot pay with no Buildings left. |

For an impossible no-Building payment, keep the pre-payment population and record
shortfall separately. This is a diagnostic state at the unsupported payment,
**not a final official score** and not negative population or shadow debt.
`needs_terminal_rule` is a blocked rule boundary; its absent death field means
**unresolved**, not proven survival, and must not be exported as a completed
comparable rollout. No official winner, draw or terminal-score vector is made.

Failure records use the supplied event clock and cause. Already-zero inputs
reject rather than fabricate an earlier first-death time; the surrounding
engine must own that history. A finished `Loss` rejects further choices, and
its record cannot be overwritten through its API. A future whole-game scheduler
must merge these events into permanent first-death history before continuing
research-only play. This module does not implement that shadow adapter.

Resource amounts are uint64; losses are limited to signed64 so DeathRecord
deficits remain representable. Total-population overflow, invalid IDs, duplicate
physical cards and unsupported choices reject. Building choices preserve card
identity and retain a separate transfer/box-loss record.

## Validation and remaining scope

`progression_contracts`: deferred recycle, top/group insertion, active barrier,
privacy, independent clocks, physical conservation, rejected-operation atomicity
and 32-seed snapshot restoration. `population_contracts`: full payment, exact-zero
death, base-vs-total distinction, explicit Building choices, transfers/boxes,
unknown LOST, unresolved terminal order, shortages, metadata and overflow.

The implementation and independent tests were assigned to disjoint files;
the lead reviewed the source audit and test code. A semantic review found an
insufficient-total interruption assumption; `needs_terminal_rule` now prevents
that path from destructively resolving an unverified order. Review also checked
the living-input precondition to protect first-death timestamps.

Local commands (Windows x64, MSVC 19.51, CMake 4.4.4):

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
```

Final local result: Debug build succeeded and **6/6 CTest cases passed**
(0.90 seconds). `git diff --check` found no whitespace errors. Independent
review has no remaining P0/P1/P2 findings after the terminal-boundary and
living-input fixes. Hosted Windows/Linux CI remains the PR validation gate.

Whole-game dispatch, legal setup, effect catalog, shared draw/death interruption
order, ties and official scoring remain open. No full M1/M2 completion, opening
recommendation, performance improvement or playable game is claimed. Throughput
benchmarks await an executable game/rollout path rather than these primitives.
