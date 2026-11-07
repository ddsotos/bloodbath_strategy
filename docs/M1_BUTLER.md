# Butler population loss and unambiguous box loss

Independent visual review of printed p7/p9 and the p5 Kale Farm card confirms:
every player loses four people before Butler's box instruction. A Building lost
to population payment discards all its boxes immediately. Butler then loses one
box; with no boxes the instruction cannot be performed. Kale Farm has no LOST
or box-loss trigger. The pinned source digest and edition limits remain those
in RULE_PROVENANCE.md and M1_ROBOT_AUDIT.md.

## Verified scope and source gap

Butler now uses the existing nonterminal Robot population frame, including
Kale Farm transfers, ordered discards and discarded boxes. Both total populations
must exceed four, otherwise the event stops before any colony mutation. Funded
bases pay automatically; insufficient bases use the existing private full
Building commitments and explicit caller schedule. All population payments
finish before examining remaining boxes.

The tail counts surviving owned Buildings with positive box counts. If each
player has zero or one such Building, lose one box from that sole target when
present, or none if unavailable. Multiple boxes on the same Building still have
only one target and lose exactly one box. Empty box-map entries are harmless.
Lost Buildings cannot be selected or lose boxes a second time. Forced box loss
adds no decision; population choice frames retain their existing clock behavior.

**Unresolved rule:** p9 specifies losing a box but does not identify who selects
the Building when several hold boxes. P7's explicit choice for losing a Building
does not establish box selection. P8's perform-as-much-as-possible guidance also
does not identify a selector. TODO: verify an authoritative clarification before
implementing multiple-target choices. No owner/random/opponent policy is assumed.

If either player has multiple boxed Buildings after population payment, the
active Robot stops at `blocked_tableau_effect`. The already-completed population
payments remain in state and audit records, but neither player's tail box loss
runs. `pending_robot` remains Butler and `robot.completed=false`. This state is
unresolved and must not be counted as completed survival or continued play.
`advance` cannot bypass it. No artificial decision is counted for missing rules.

## Records and validation

Session schema 10 adds `box_loss` (nullable target IDs per player) and `completed`
to Robot records. Successful tails mark completion and clear pending state;
Waiter/Mining records remain complete with null box targets. Partial Butler
records distinguish population progress from unresolved tail instructions. No
command/phase ID is added. Old schemas reject; the manifest changes. Private
audit records remain outside policy observations.

Synthetic fixtures cover zero/single/multiple targets, box counts, post-loss
target recomputation, full Building-box disposal, tail barriers, terminal guards,
population choice privacy, clocks, replay tampering and independent JSON parsing.
Other Buildings/LOST effects, box-selector clarification and terminal interruption
remain open. No full-game fidelity, shadow extension or throughput claim is made.

Local Windows x64 MSVC Debug build succeeded and all 24 CTest cases passed in
5.50 seconds. Independent source, implementation and record review has no
remaining P0/P1/P2 findings after correcting a historical manifest description.

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
```
