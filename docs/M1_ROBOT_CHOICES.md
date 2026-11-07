# Robot Building-loss choices

Waiter and Mining Robot now connect to the reviewed Kale Farm loss adapter when
base people cannot pay. Independent visual audit of printed p7/p9 and the p5
Kale Farm card confirms: all players lose the printed population amount before
any remaining Robot instructions. Base shortage forces a chosen Building's
printed people to transfer to base; discard the card and all boxes, repeat as
needed, then pay the full original amount. Kale Farm has no LOST/Robot trigger
and losing it does not execute PLAY/FARM. Source SHA-256 and physical-edition
limits remain those in RULE_PROVENANCE.md/M1_ROBOT_AUDIT.md.

## Frame contract

Both total populations must strictly exceed Waiter's four or Mining Robot's six.
Otherwise the active Robot stops at `blocked_robot_loss` before either colony
changes. If both bases already cover the amount, automatic handling remains.
If either base is insufficient, `robot_choices` records one decision and holds
the active Progress card. `pending_robot` identifies the fixed effect/amount;
there is no population or tail-resource mutation yet.

Both players call shared `commit_losses` with a complete ordered list of owned
Kale Farms. A funded base must commit an empty list. For Mining Robot, base zero
with two Farms requires both transfers before paying six; an incomplete or
excess list rejects atomically. Choices are validated on copies. Observations
expose only the observing player's owned public Building IDs while their own
insufficient-base commitment is open. Opposing choices stay private.

`resolve_robot` requires both commitments and an explicit player permutation.
It applies all population payments on a copy, records transfers/card discards/
discarded boxes, and only then applies the tail. Waiter loses up to two food per
player; Mining has zero Mine money in the Kale-only context. The recorded order
defines shared discard order as a caller convention; the source does not define
cross-player discard priority or private commitment protocol.

On success, pending effect/commitments clear, the Robot record is retained, and
active Progress finishes. Both populations remain positive; there are no death,
official-terminal or shadow changes. Butler with any Building, unknown LOST,
other Buildings, terminal interruption and full setup/catalog remain outside
scope. A blocked trace is unresolved, not a completed survival result.
The later [Butler extension](M1_BUTLER.md) supports population loss and box tails
with zero or one remaining target; ambiguous multi-target box selection still stops.

## Records and validation

Session Replay/JSON schema 9 adds phase 16, command 10, nullable `pending_robot`
and `resolution_order` in Robot records. `commit_losses` remains command 7.
Existing automatic Robot records use internal order `[0,1]` and no card discard.
Old schemas reject; the manifest hash changes. No private audit fields enter
policy observations.

Synthetic fixtures cover Waiter/Mining choices, multiple Farms, mixed funded
bases, box/card conservation, tail barrier, private commitments, atomic rejection,
terminal guards, clocks, repeated effects, deterministic replay/tampering and
independent Python JSON parsing. This remains a partial engine, without a
simulation throughput claim or strategic recommendation.

Local Windows x64 MSVC Debug build succeeded and all 22 CTest cases passed in
5.05 seconds. Independent source, implementation and record review has no
remaining P0/P1/P2 findings. Commands:

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
```
