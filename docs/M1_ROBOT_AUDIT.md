# M1 Glitch and conditional Robot source audit

Audit date: 2026-11-06. This reviews a limited Glitch/Robot extension, not the
complete Robot catalog, legal setup distribution or a strategy result.

## Evidence and provenance

I visually inspected the local renders of printed rulebook pages 3, 6, 7 and 9.
They reproduce the publisher-authored English rulebook pinned in
[M1_RULE_AUDIT.md](M1_RULE_AUDIT.md), PDF SHA-256
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.

The [publisher URL](https://www.riograndegames.com/wp-content/uploads/2024/07/MCB.pdf)
and [downloaded reproduction](https://tesera.ru/images/items/2462157/Moon_Colony_Bloodbath_Rulebook.pdf)
remain distinct provenance references. Equality to the publisher's current
bytes and intended physical printing is **unverified**. No complete errata set,
printing identifier or full catalog is established by this report. Existing
local page renders are audit evidence, not new committed source assets.

## Reviewed mechanics

| Source | Reviewed fact | Conditional implementation consequence |
| --- | --- | --- |
| p3 setup | The Robot deck is shuffled and kept face down. | A supplied Robot order is hidden engine state. A legal setup sampler remains separate. |
| p6 Glitch | Add the next Robot from its deck once for the table, on top of Progress; it is next to happen. | Consume one existing Robot-deck instance, not one per player and not a fresh random selection from the full catalog. |
| p7 Robots | First every player loses the printed population amount; only afterward follow the remaining Robot instructions. | Preserve loss-before-tail order. Unsupported terminal interruption must not silently run the tail after a failure. |
| p9 Robot Butler | Its card prints four population lost and one box lost; with no boxes, losing a box simply fails. | In the no-Building scope there are no boxes to remove, so the supported nonterminal effect is only the population loss. |
| p9 Robot Waiter | Its card prints four population lost and two food lost. The notes explicitly handle one or no food. | Lose `min(2, available food)` without negative food, after the supported population loss. |
| p9 Mining Robot | Its card prints six population lost; its money benefit depends on the listed Mine types. | With no Buildings, none of those Mines exist, so this slice gains no money. Do not infer a general duplicate-Mine counting algorithm from this slice. |

The p9 Mining Robot notes list Dust Mine, Silica Mine, Iron Mine, Helium-3 Mine,
Titanium Mine and Aluminum Mine. This review establishes why the no-Building
benefit is zero; it does not implement those Buildings, their effects or their
catalog multiplicities.

Robot population loss values here come from each illustrated card, with p7
supplying the loss-before-tail procedure. Do not infer an unreviewed Robot's
printed loss merely from its type or from neighboring cards.

## Supported context and explicit stops

The session remains restricted to living no-Building inputs with all relevant
modifiers certified absent. This excludes Building population transfers, LOST
chains, Robot interactions with Buildings and persistent effect modifiers.

For these three supported Robots, the proposed kernel can require that **both
players have strictly more population than the printed loss** before changing
either player's population or tail resources. An exact-zero or insufficient
case must stop before all loss/tail mutation because the remaining instructions
and simultaneous terminal interruption have not been fully verified.

That strict guard is a conservative implementation boundary, not an official
Robot immunity rule, cancellation, completed draw or terminal adjudication.
The revealed Robot remains active. An absent death record in the blocked state
means unresolved processing, not proven survival. Do not export that branch as
a completed comparable rollout or continue to a later Progress card.

An unknown Robot should stop before any guessed population loss or tail effect.
Glitch may add its opaque physical ID without authorizing unknown effect
resolution. If no Robot is available, preserve a distinct unsupported/exhaustion
boundary; do not invent a new Robot, a recycled Robot pile or a terminal rule.
The reviewed recycle rules concern Building and Progress decks, not an automatic
refill of the separate Robot deck.

## Order, catalog and visibility

The externally supplied Robot deck is a conditional precondition. Consuming its
next card implements Glitch's next-card operation; it does not establish a
complete 20-Robot setup catalog or the probability of the supplied order.
Do not shuffle or resample that order afresh on each Glitch. The added Robot
then belongs to Progress and will later recycle there under Progress rules.

Each physical Robot instance must move once between zones. Reject duplicate
instances across remaining Robots, Progress and any other tracked physical
Progress-card source. A complete trace should conserve added and not-yet-added
Robot instances together, including unsupported ones.

Policies may see the currently revealed Robot as permitted public information;
they must not receive remaining Robot identities/order, seed-derived future
draws, the opponent's hand or buried discard contents. Full snapshots and JSON
audit records can retain private order for reproducibility only when kept
outside policy observations.

## Targeted acceptance cases

- Glitch consumes exactly the next supplied Robot once, puts it on top, and
  finishes its own processing before the Robot is revealed as a separate event.
- An insertion into an empty Progress deck happens before old Progress discards
  recycle. Robot-deck consumption neither resamples nor increments an invented
  Robot shuffle counter.
- Butler safely loses four population for both players with no box/state
  fabrication. Waiter then loses two, one or zero food as available. Mining
  Robot safely loses six population and adds no money with no Buildings.
- If either player has population equal to or below the printed loss, neither
  player's population, food, money or death record changes and the Robot remains
  active in a distinct blocked phase.
- An unknown Robot blocks before inferred loss; an empty Robot source preserves
  explicit exhaustion without fabrication or automatic refill.
- Changing hidden remaining Robot order leaves current observations unchanged.
  Replay/JSON retain the full supplied order and consumed position for audits.
- Count physical cards across Progress plus remaining Robot sources; verify
  one-card movement and deterministic replay under identical input/commands.

## Work performed

Only this report was written by the audit task. No engine or test files changed,
no builds/tests were run, and no full Robot or Building conformance is claimed.
