# M1 progression and forced-loss audit

Audit date: 2026-11-06. This records a conditional rule subset and implementation
boundaries, not a complete official-game specification.

## Source and verification status

The source is the publisher-authored English rulebook reproduced at the
[pinned public mirror](https://tesera.ru/images/items/2462157/Moon_Colony_Bloodbath_Rulebook.pdf).
Downloaded PDF SHA-256:
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.
Printed pages 4, 6, 7 and 8 were visually inspected using the local page renders.
See [M1_RULE_AUDIT.md](M1_RULE_AUDIT.md) for earlier provenance and edition review.
The [publisher URL](https://www.riograndegames.com/wp-content/uploads/2024/07/MCB.pdf)
is known, but equality between its current bytes, the mirror and the intended
physical printing remains **unverified**. No printing/revision identifier is
established. Local source images are audit artifacts, not committed assets.

## Reviewed mechanics

| Page | Verified against the pinned reproduction |
| --- | --- |
| 4 | Reveal the next Progress card onto its face-up discard pile, resolve it, and finish all players' processing before the next reveal. |
| 4, 8 | Recycle the Progress discards only when a next reveal is needed and the draw deck is empty. Emptying the deck alone does not shuffle it. |
| 8 | Add Progress cards on top. A simultaneous group of additions is shuffled together first. A single added card can become the next reveal even when the old draw deck is empty. |
| 8 | Players cannot inspect through either Building or Progress discard piles. A face-up top card does not authorize a complete pile query. |
| 6 | Trouble adds the next numbered Event to the top once for the table; that Event is the next reveal. This is not one addition per player. |
| 7 | Population loss is paid from base people. If base people are insufficient, the player chooses a Building to lose, transferring its printed people to the base. Repeat if necessary, then pay the requested loss. |
| 7 | A lost Building goes to the Building discard pile; its boxes are lost. Its LOST effects finish before another Building is lost or the requested population is paid. |
| 7 | A forced Building choice is not restricted by its printed population. Buildings cannot be discarded voluntarily; a direct instruction to lose one also transfers printed people. |
| 7 | Inability to cover the loss or zero total people causes colony failure; ordinary end scoring counts base and printed Building people. Instruction Manual also ends the game. |

The transfer/payment sequence matters: do not first subtract the available base
population as a partial payment and then let LOST effects run against that
different state. The text describes assembling sufficient base people before
deducting the requested loss.

## Conditional implementation boundary

Generic deck scheduling can use caller-supplied opaque physical-card identities.
That does not verify the initial Progress multiset, complete Event sequence, card
effects, optional choices, or an action policy. Revealing an unknown card must
not silently resolve it as a no-op. Card-resolution completion is an explicit
boundary; a second reveal must not skip pending choices or effect processing.

A conditional loss resolver may handle caller-supplied printed populations only
where the relevant effect context is verified. Physical card IDs and population
values in fixtures are synthetic test inputs, not a fabricated official catalog.
Unknown or unsupported LOST effects need a visible stop before their effects
are omitted. A no-LOST-effects subset can make that precondition explicit rather
than claiming all Buildings are supported. It must not choose the Building for
the player, prefer a population-minimizing target, or introduce a voluntary-loss
operation while ordinary base payment is possible.

## Unresolved terminal and effect semantics

- No tie rule was established by these pages. Equal survivor counts cannot be
  silently converted into an official draw.
- The text does not establish a full interruption order for simultaneous colony
  failures, nested LOST chains, or an effect with unresolved later instructions.
- When a requested loss exceeds all available people, do not infer a negative
  official population or invent partial-payment semantics for final scoring.
  Preserve the unmet amount/failure evidence separately and leave official
  scoring or interruption decisions unresolved where required.
- A LOST effect can add people, cause further losses, add Progress cards or
  introduce choices. These interactions need verified card text and explicit
  resolution frames; they cannot be replaced by a generic resource transfer.
- Progress/Event pile exhaustion does not establish an arbitrary round limit or
  shadow horizon. A missing next Event or unresolvable deck state needs an
  explicit unresolved/error result until its semantics are established.
- Shuffle/cycle indices and random event identities are engine conventions.
  A reshuffle is a useful cycle boundary only if it is consistently specified;
  merely adding a card or resolving an event is not evidence of a new cycle.

These gaps remain separate from research-only shadow continuation, fixed
post-death policy and first-death recording. No shadow convention verifies an
official terminal rule.

## Targeted acceptance cases

1. Draw the last Progress card without reshuffling; add one card to the empty
   draw deck; verify that it is next and old discards remain unrecycled.
2. With no addition, request another reveal and verify exactly one on-demand
   shuffle and preservation of every physical ID. Repeat by seed and test
   simultaneous additions as one shuffled batch.
3. Inspect an observation with equal top discard but altered buried order and
   future order; observations stay equal. Pending card processing blocks reveal.
4. With sufficient base population, pay once without losing a Building; exact
   zero and positive remainder must remain distinct results.
5. With insufficient base population, require an explicit owned-Building choice;
   reject invalid/repeated targets and voluntary choices when payment is ready.
6. Transfer all printed people from the selected Building, discard its card and
   boxes, and check total population conservation before the actual loss. Permit
   any owned target, including one whose population does not yet cover the loss.
7. For an unsupported LOST ability, stop visibly; do not continue to payment or
   another Building. For a verified no-effect fixture, proceed until payment or
   unmet loss is established. Reject arithmetic overflow without partial state
   mutation.
8. Preserve requested loss, unmet amount, supplied event clock and cause. Do not
   label an unresolved terminal score or equal survivor count an official win or
   draw. The first-death record, if made by a higher layer, remains immutable.

## Work performed

Read the earlier audit and source ledger, inspected the four named page images,
and wrote this report only. No engine changes or builds/tests were performed by
this audit task. The lead owns integration and reconciliation of code with this
conditional specification.
