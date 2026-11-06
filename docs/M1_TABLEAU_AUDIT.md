# M1 Kale Farm and Restock source audit

Audit date: 2026-11-06. This reviews a deliberately limited Tableau extension,
not a complete Building catalog, legal setup sampler or strategic conclusion.

## Evidence and provenance

I visually inspected the local renders of printed pages 2, 3, 4, 5 and 7,
including the lead-prepared enlargement `build/sources/kale-farm.png` from p5.
The underlying publisher-authored English reproduction is pinned in
[M1_RULE_AUDIT.md](M1_RULE_AUDIT.md), PDF SHA-256
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.

The [publisher URL](https://www.riograndegames.com/wp-content/uploads/2024/07/MCB.pdf)
and [downloaded reproduction](https://tesera.ru/images/items/2462157/Moon_Colony_Bloodbath_Rulebook.pdf)
remain distinct provenance references. Equality to current publisher bytes and
the intended physical printing is **unverified**. No complete errata set or
physical-card multiplicities is established. Local source images remain audit
artifacts; this report does not commit the PDF or card images.

## Reviewed facts and conditional effects

| Source | Reviewed fact | Kale-only implementation consequence |
| --- | --- | --- |
| p5 Kale Farm card | Printed cost two money, printed population five; Farm gains one extra food; PLAY performs Farm. | A supported Build pays two, puts Kale Farm in play, then resolves its Farm action and current own Farm triggers. |
| p4 Build | Pay the card's cost, put it into play, then perform PLAY rules. | The new card is already active when its PLAY action runs. Do not treat PLAY as occurring while the card is still in hand. |
| p5 example | A newly built Kale Farm immediately performs Farm and also receives its own Farm bonus. | The first own Kale Farm's PLAY gains five food, not four. |
| p5 duplicates | More than one copy of a Building is allowed and abilities are generally cumulative. | In a scope containing only Kale Farms, a Farm gains four plus the number of own active Kale Farms; the new Farm is included after Build. |
| p3, p5 | Building people are represented by the card's printed population, not population tokens placed on it. | Build does not move/add those five people to base tokens. Total population is base plus printed in-play population. |
| p4 Restock | Put two boxes on owned Buildings; both may go to one Building or one to each of two Buildings. | Require explicit owned targets/allocations, preserving both same-target and split-target choices. |
| p3 boxes | Boxes belong on Buildings, never on the base mat. | Empty-Tableau Restock cannot fabricate a base box store. |
| p2 components, p4 Restock | Chits are not component limited, and Restock permits two boxes on one Building. | Do not invent a global one-box capacity or limit boxes by the supplied cardboard-token count. |
| p7 end scoring | Count base people plus printed population of in-play Buildings. | Instruction Manual must include each supported Kale Farm's printed five while keeping base population separate. |

Kale Farm's illustrated text contains no LOST effect, no box expenditure and
no Restock trigger. That is a property of this reviewed card, not a default for
unknown Buildings. In the Kale-only scope its Farm gains are nonnegative,
additive and commute, so the before/after action ordering permission does not
create different food totals. Other cards can introduce choices/order effects
and must not inherit that simplification.

## Opt-in scope and catalog limits

The previous bare-Work path rejected Buildings. Supporting this card should be
an explicit reviewed-Tableau mode rather than silently weakening that original
guard. Each in-play physical instance must have an adopted supported definition;
unknown in-play cards must reject or stop before their abilities are omitted.
All other relevant modifiers must remain absent or explicitly implemented.

Caller-supplied opaque IDs can identify fixture instances as Kale Farms without
claiming they establish the official deck's full identity list or number of
copies. The p5 duplicate rule verifies cumulative behavior when multiple copies
exist, not the actual catalog multiplicity. Preserve the original physical-ID
conservation rules across hand, deck, discard and in-play zones.

Build choices must specify the owned hand card before the reveal barrier.
Face-down simultaneous Build targets must not leak through another player's
observation. An unsupported or unaffordable target is not a valid supported
choice, but the partial engine must not call every unimplemented official
Building illegal. Physical card text and costs must not be inferred for an
unmapped ID.

Restock allocations are explicit choices, not an automatic best-target policy.
The printed Kale Farm has no allocation-triggered ability, so this subset can
apply its two box placements directly after commitments resolve. A generic box
placement system will still need to handle other cards' effects and any
card-specific limits. Numeric overflow is an implementation error boundary,
not a printed resource cap.

## Population-changing events and scoring

Adding printed Building population changes the meaning of later Hunger,
population-loss and Robot instructions. The old no-Building event handlers must
not keep running unchanged once a Farm is present. Until the Building-loss and
effect adapters are connected, retain an explicit stop before all mutation of
such affected events, even if a particular fixture might survive a bare loss.

This guard is a conditional implementation boundary, not official immunity or
an additional game-ending rule. An absent death record at that boundary means
unresolved processing, not proven survival. Instruction Manual can use the
reviewed total-population calculation; its unresolved tie rule remains a
distinct stop. Printed Building people must not be counted both in the base
and again on the card.

## Targeted acceptance cases

- First Kale Build removes the selected physical card from hand, pays exactly
  two money, adds it to play, leaves base population unchanged and gains five
  food through PLAY Farm plus the new card's own trigger.
- Building a second own Kale Farm gains six food; a later Farm with two in play
  also gains six. Only the acting player's own Farms supply these triggers.
- Simultaneous Build targets and Restock allocations remain concealed until
  both commitments lock. Reverse submission order preserves results under the
  same recorded resolution schedule.
- Reject wrong ownership, unknown supported definitions and unaffordable Builds
  without partial payment, placement or another player's mutation.
- Restock places two boxes on one owned Farm or splits them across two; reject
  unowned targets and invented base storage. Overflow must not partly place
  another box or resolve another player's action.
- Preserve all physical IDs across Building zones and keep boxes attached to
  the selected in-play instances. No unknown Building ability is skipped.
- With a Farm in play, affected Hunger/loss/Robot events stop before the old
  no-Building logic changes state. Instruction Manual scores base plus five per
  own Farm without moving printed people to the base.
- Replay and engine JSON retain instance definitions, target/box choices,
  conditional scope and card-zone outcomes without claiming a complete catalog.

## Work performed

Only this report was written by the audit task. No engine or test files changed,
no builds/tests were run, and no complete Tableau conformance is asserted.
