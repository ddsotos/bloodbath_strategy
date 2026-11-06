# M1 Paperwork source audit

Audit date: 2026-11-06. This report reviews a conditional Paperwork slice,
not a complete card catalog, whole-game ruleset or strategic conclusion.

## Source and provenance

I visually inspected printed rulebook pages 5, 6, 7 and 8 using the local page
renders. The publisher-authored English reproduction and its SHA-256 remain
those recorded in [M1_RULE_AUDIT.md](M1_RULE_AUDIT.md):
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.

The [publisher URL](https://www.riograndegames.com/wp-content/uploads/2024/07/MCB.pdf)
and [pinned reproduction](https://tesera.ru/images/items/2462157/Moon_Colony_Bloodbath_Rulebook.pdf)
are distinct references. Equality to current publisher bytes and the intended
physical edition remains **unverified**, as do a complete errata set and card
catalog. No PDF or rendered source page is committed by this audit.

## Reviewed facts

| Source | Reviewed rule fact | Conditional implementation consequence |
| --- | --- | --- |
| p6 Paperwork | Each player draws one Building card and then discards two cards. | Draw must precede that player's discard choice; the newly drawn card is eligible as part of the resulting hand. |
| p6 Paperwork | Hacker Lair immunity skips both the draw and discard, and cannot be declined. | A generic implementation must not merely omit the discard. The current no-Building/all-modifier-absent scope excludes this immunity rather than pretending to implement it. |
| p8 | Execute the feasible portion of an instruction. | With fewer than two hand cards after drawing, discard the available hand cards; do not require nonexistent IDs or fabricate cards. |
| p8 | A needed Building draw with an empty deck recycles the Building discards. | Recycling is on demand, not immediately when the last old card is drawn. |
| p3 as previously reviewed, p8 | Hands are private; discard piles are face up but cannot be inspected through. | Observe own post-draw hand and top discard only. Do not expose opponent hand, pending private choices, buried discards or future order. |
| p4, p7 | Playing a Building puts it into play; losing a Building transfers its printed people and executes LOST effects. | Discarding an unplayed idea from hand is a different operation from losing an in-play Building. Do not transfer its people, remove boxes or execute in-play LOST solely because a hand card is discarded. |

The final row uses the source's distinction between hand cards and in-play
Buildings. It does not prove the absence of every possible card-specific
discard trigger; the session must continue to require that all relevant
modifiers/effects are absent or implemented. The current no-Building scope
should leave in-play state and population untouched during hand discards.

## Scheduling conventions and guarded timing

The printed Paperwork description gives each player's draw-then-discard order.
It does **not** establish a total player priority for shared draws, a universal
hidden-discard commitment protocol, or an all-players-draw barrier.

The proposed session can use these explicit, recorded conventions:

- Require a caller-supplied player permutation for the two Building draws.
- Complete both draws before accepting either player's private discard choice.
- Lock both private choices before revealing/applying either discard batch.
- Use a separate caller-supplied player permutation for discard application,
  which determines the final public top discard and is retained in the trace.

Those choices must remain labelled engineering/research conventions; they are
not newly discovered official priorities. Reversing commitment submission order
should not alter a result when the recorded draw and discard schedules are equal.

If fewer than two cards exist in the **old Building deck plus old Building
discard pile**, this session should stop before either player draws. Otherwise,
allowing a first player's current-event discards to feed a later player's draw
would settle an unverified cross-player timing question. This guard is a
conservative unsupported boundary, not an official failure/terminal rule and
not a claim that the printed effect cancels. No death, winner or comparable
completed rollout should be inferred from the stop.

The guard should count existing discard cards that may legally be recycled,
not just remaining draw cards. A last-card draw may leave the deck empty without
recycling until the second draw actually needs a card. Neither current-event
discard commitment should reach the shared discard pile before the draw phase
has completed under this convention.

## Targeted acceptance cases

- Each player receives exactly one card before choosing discards in the supported
  supply range. The newly drawn card can be selected for discard.
- A two-card hand discards two; a one-card post-draw hand discards one. The quota
  is based on the actual post-draw hand, not an invented minimum hand size.
- Reject unowned/repeated IDs and wrong quotas without changing either player's
  hand, the discard pile or the other player's locked private choice.
- Both draw and discard schedules are validated permutations and are recorded;
  reverse commitment submission order has no effect under equal schedules.
- Hidden opponent cards and pending discard choices do not enter observations.
  After the recorded discard order completes, expose only its final top card.
- A draw can cross the old deck/discard boundary with one on-demand shuffle.
  Do not prematurely recycle merely because the old deck reached zero.
- When old deck plus discard has zero or one card, retain the active Paperwork,
  preserve all Building zones and return a distinct unsupported timing status.
- Hand discards preserve population/resources, do not execute in-play LOST,
  conserve every physical card ID and leave the Building tableau unchanged.
- Replay and JSON checkpoints retain the new phases, choices, schedules and
  card-zone outcomes without presenting this conditional fixture as a full game.

## Work performed

Only this report was written by the audit task. No engine/test files changed,
no builds were run and no complete Paperwork immunity implementation is claimed.
