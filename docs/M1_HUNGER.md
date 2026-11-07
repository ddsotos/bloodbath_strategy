# Hunger payment with reviewed Kale Farms

Visual review of printed p6 Hunger and p5 Kale Farm in the pinned reproduction
confirms: lose food equal to the number of Buildings at the event's start;
each unpaid food causes one population loss. Apply that shortage as one loss.
If no population is lost, gain two base people, including when no food was needed.
Losing a Building does not cancel its original food requirement. Kale Farm's
FARM/PLAY effects do not trigger on Hunger. Source SHA-256 and edition limits
remain those in RULE_PROVENANCE.md.

## Implemented scope

`Session::advance` computes required food, spent food, population loss and gain
for both players before applying payment. This slice supports no Buildings or
reviewed Kale Farms with no external modifiers, and only when each player's base
can pay their shortage and total population remains positive. A zero base with
positive printed population can survive when food covers the requirement.

If either player needs a Building choice or a terminal loss, the phase becomes
`blocked_population_payment` with both colonies unchanged, no death and no
official result. The active Progress card remains unresolved. Population gain
overflow rejects the whole advance, including the reveal, without partial payment.
Building-loss choices and terminal interruption need a later frame; they are not
inferred from the Accident frame. Other Building-aware Robot effects remain open.

Successful payment changes food and base population only, leaves Buildings and
boxes intact, finishes the active card and adds no decision. Requirements are
recomputed on each Hunger. The optional Hunger record is cleared on each new
Hunger and populated on success; blocked frames have no completed Hunger record.
As with prior effect records, the latest completed record may persist across
other card types until the next occurrence of its own type.

## Replay and validation

Replay/JSON schema 6 adds `hunger` to audit checkpoints: nullable or per-player
`required_food`, `spent_food`, `population_loss`, `population_gain` arrays.
Command and phase IDs are unchanged. The manifest hash changes and older Session
schemas reject. Audit output remains private; policy observations gain no hidden
state. Official outcome, first death and shadow output remain separate.

Synthetic physical-ID fixtures cover food payments, partial shortages, no-loss
bonuses, total-vs-base survival, blocked choices/terminal cases, atomic overflow,
repeated events, replay tampering and independent JSON parsing. No claim of full
game fidelity, throughput improvement or strategy strength accompanies this slice.

Local Windows x64 MSVC Debug build passed; all 16 CTest cases passed in 3.24
seconds, including Hunger contracts and independent JSON parsing. Independent
source and implementation review has no remaining P0/P1/P2 findings.

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
```
