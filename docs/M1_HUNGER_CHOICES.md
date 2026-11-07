# Hunger Building-loss choices

This extends [Hunger payment](M1_HUNGER.md) using the reviewed Kale Farm loss
adapter. Independent visual audit of printed p6/p7 in the pinned reproduction
confirms that food requirements use the original Building count; losing a
Building does not cancel its food requirement. Shortage is one population loss.
Lost Buildings transfer printed people and lose all boxes; Kale Farm has no
LOST trigger (p5). Source/edition limits remain those in RULE_PROVENANCE.md.

## Frame contract

Both colonies must have total population strictly greater than their respective
shortage. Growth overflow is checked before opening a frame. A terminal case
still stops before all resource mutation. Reviewed Kale Farm-only scope and
certification of absent external modifiers remain required.

If base population covers every shortage, the existing automatic path remains.
Otherwise `hunger_choices` holds the active card and adds one decision. No food,
population, Buildings or boxes change when the frame opens. `pending_hunger`
freezes initial required food, food payment, population loss and gain arrays.

`commit_losses` now accepts both Accident and Hunger frames, trial-validating
the complete ordered list against the appropriate fixed loss. A base-funded or
zero-loss player must commit an empty list. A player needing more than one Farm
must choose enough Farms to assemble the entire loss; excess choices reject.
Observation `loss_options` contains only the observer's public owned Buildings
while their own insufficient-base commitment is open. Opposing pending choices
and future deck order remain private.

`resolve_hunger` requires both commitments and a player permutation. It resolves
on a copy, pays the original shortage once, transfers lost Farms to the shared
discard, removes their boxes and pays the frozen food amount. Only shortage zero
receives +2. Requirement and loss do not shrink after discarding a Farm. The
recorded schedule defines shared discard order as an explicit caller convention;
the rulebook does not establish a cross-player priority or secret commitment
protocol. Unknown LOST effects, other Buildings and terminal semantics remain open.

The completed frame clears pending amounts/commitments, stores `hunger` and
`hunger_losses`, finishes active Progress and returns ready. Later Hunger events
recompute requirements from their own initial tableau, clearing prior results.
No shadow continuation or new official end behavior is introduced.

## Records and validation

Session Replay/JSON schema 7 adds phase 15, command 9 and nullable `pending_hunger`
and `hunger_losses` checkpoint fields. The latter uses the same loss/order record
shape as Accident. Automatic Hunger keeps its amount record and has null
`hunger_losses`. Old schemas reject; the versioned manifest changes.

Synthetic fixtures cover frozen requirements, multi-Farm choices, transfer/box
loss, mixed zero-loss bonuses, commitment secrecy, rejected-operation atomicity,
active-card barriers, clocks, repeated events, overflow preflight and replay
tampering. Independent Python parsing checks the complete audit scenario.

Local Windows x64 MSVC Debug build succeeded; all 18 CTest cases passed in
3.69 seconds. Independent source, implementation and record review has no
remaining P0/P1/P2 findings. Commands:

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
```

This slice measures correctness only; it makes no simulation throughput claim.
