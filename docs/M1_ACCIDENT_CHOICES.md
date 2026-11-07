# Accident with reviewed Kale Farms

Accident's printed loss of three people (rulebook p2 card image) now connects to
the [nonterminal Kale Farm adapter](M1_TABLEAU_LOSS.md), using the p7 population
payment rules and p5 absence of Kale Farm LOST effects. The pinned source and
edition limits remain those in RULE_PROVENANCE.md. Synthetic scenario IDs and
supplied deck order do not define physical copy counts or official setup.

## Scope and state machine

When Buildings exist, `advance` checks that BOTH players' total populations are
strictly greater than three. Otherwise it stops at `blocked_population_payment`
with no population/Building/box changes, no death record and no official result.
Hunger and Robot handlers with Buildings remain blocked. The previous bare
Accident handler, including its exact-zero cases, is preserved.

If both bases cover three, automatic payment records an Accident result and
finishes the active Progress card without adding a decision. Its internal order
is 0 then 1; neither player discards a Building in this deterministic branch.

Otherwise `accident_choices` holds the active card and records one decision.
Each player calls `commit_losses` with the complete ordered list of Buildings
needed to cover the loss. A sufficient base requires an empty list. Kale Farm
has five printed people, so a base below three requires exactly one Farm in this
subset. Invalid/excess/missing/unowned choices reject without changing state.
Commit validation runs the adapter on a copy; no payment occurs at commitment.

`resolve_accident` requires both commitments and an explicit permutation of the
players. It resolves both losses on a copy, stores transfer/box records, clears
commitments and finishes the active card. Shared discard ordering follows the
recorded schedule: this is an explicit caller convention, not a verified official
priority. General LOST effects and terminal interruption are outside this slice.

## Observation and replay

Observations expose `loss_options` only for the observing player while their
base is insufficient and their commitment is still open. It contains their
public in-play Building IDs. Neither pending opposing choices nor hidden hands,
future Progress/Building order or audit records enter this field.

Replay/JSON schema 5 appends phase 14 and commands 7/8. Audit checkpoints retain
nullable commitments and an Accident record with resolution order, requested and
paid amounts, shortfall and ordered card/transfer/box entries. Old schemas reject.
The manifest hash changes. Death and official/shadow outcomes remain separate.

## Validation

Independent contract tests cover base and Building payments, observation privacy,
blocked terminal boundaries, rejection atomicity, active-card barriers, clocks,
repeated events, replay determinism and tampering. The JSON audit is also checked
by a separate Python parser. Local Windows x64 MSVC Debug validation passed all
14 CTest cases in 3.01 seconds after fixing signed literal comparisons in the
new test. Independent implementation review has no remaining P0/P1/P2 findings.

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
```

No simulation throughput claim accompanies this correctness-focused extension.
