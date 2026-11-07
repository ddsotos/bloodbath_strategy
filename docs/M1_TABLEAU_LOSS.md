# Reviewed nonterminal Kale Farm population loss

`tableau::lose_people` connects the existing single-colony `population::Loss`
primitive to `base_rules::State`. This is a narrow state adapter, not a complete
Event dispatcher or a shadow rule. Session continues to block population-changing
Events/Robots when Buildings are in play until their full effect frames exist.

## Rule provenance and scope

Independent visual audit of printed p7 and the p5 Kale Farm card in the pinned
publisher-authored reproduction confirms: pay population from the base; when
insufficient, choose an owned Building, transfer its printed people to the base,
discard the Building and all its boxes, then reassess. A Building cannot be lost
voluntarily when base population covers the payment. Kale Farm has five printed
people and no LOST effect; losing it does not execute its PLAY Farm action.
See [source ledger](RULE_PROVENANCE.md) for the pinned SHA-256 and edition limits.

Only reviewed Kale Farms and caller-certified absent external modifiers are
accepted. Total population must strictly exceed the requested loss. Exact zero,
shortfall and terminal interruption order remain unsupported. Amounts above
signed64 are rejected by the underlying population primitive.

## Contract

The caller passes player, amount, event clock, cause and a complete ordered list
of Building choices. The adapter validates the full context and resolves on a
copy. Incomplete, excess, repeated or unowned choices reject without changing
the caller state. A zero amount permits no voluntary Building loss.

The result retains the requested/paid amounts and each lost card's transferred
population and discarded boxes. Successful losses have no death record. Lost
cards append to the shared Building discard in selected order; the caller must
define and record any ordering across players. Other resources, the other colony,
hands, deck order and shuffle counters remain unchanged. No official result or
shadow score is produced.

The Tableau manifest now includes this adapter and its hash depends on both the
base and population manifests. Existing Session schema 4 stays structurally the
same; the changed dependency hash deliberately distinguishes the implementation.
This API does not add a new Session command or replay interchange format.

## Validation

Independent synthetic physical-ID fixtures exercise base payment, zero base
with positive printed population, multiple choices, transfers and box removal,
shared discard conservation, retained resources/Buildings and rejected-operation
atomicity. Local Windows x64 MSVC Debug build succeeded and all 12 CTest cases
passed (2.82 seconds), including `tableau_loss_contracts` and existing JSON
replay checks. Commands:

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
```

Independent source/implementation review found no remaining P0/P1/P2 findings.
No throughput claim is made for this state adapter; game/rollout benchmarking
awaits the connected effect frames.

The later [Accident frame](M1_ACCIDENT_CHOICES.md) connects this adapter to
Session with replay and observation coverage. Other affected Events and
separately verified terminal semantics remain open.
