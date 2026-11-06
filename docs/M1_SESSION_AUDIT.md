# M1 conditional session source audit

Audit date: 2026-11-06. This report supports a conditional scheduler slice,
not a complete game, setup distribution, card catalog or strategy result.

## Evidence and provenance

I read the required project instructions/design documents and visually inspected
the local renders of printed rulebook pages 2, 4, 6, 7 and 8. These renders came
from the publisher-authored English rulebook reproduction recorded in
[M1_RULE_AUDIT.md](M1_RULE_AUDIT.md). Its PDF SHA-256 is
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.

The [publisher URL](https://www.riograndegames.com/wp-content/uploads/2024/07/MCB.pdf)
and the [downloaded reproduction](https://tesera.ru/images/items/2462157/Moon_Colony_Bloodbath_Rulebook.pdf)
remain distinct provenance references. Equality to the publisher's current
bytes and the intended physical printing is **unverified**. No revision or
errata set is established. Local PDF/images are audit artifacts, not committed
source assets. No new download or edition claim was made by this audit.

## Reviewed effects and scheduler boundaries

| Source | Reviewed fact | Conditional implementation boundary |
| --- | --- | --- |
| p4 | Finish all players' processing of the current Progress card before revealing another. | Work choices and unresolved effects must keep the current card active. |
| p4 | Mine adds four money, Farm four food, Research draws two Building cards; Building triggers may resolve before or after the basic action. | Bare base effects require an explicitly verified unmodified context with no relevant Buildings/Twists. |
| p6 Trouble | Add the next numbered Event once for the table, on top of Progress; it is next to occur. | Do not add once per player or wait for a later reshuffle. The external Event queue must carry its own verified ordering. |
| p6 Hunger | Food cost equals Building count; missing food causes population loss; no population loss gives two base people. The text explicitly covers needing no food. | With no Buildings and no modifiers, food is unchanged and base people increase by two. |
| p2 Accident card, p6 loss-Event text | The illustrated Accident card prints a loss of three people; loss Events apply their indicated amount to each player. | Do not infer other Event loss values, multiplicities, Event numbers or extra effects from this illustration. |
| p6, p7 Instruction Manual | This Event ends play; highest total surviving population wins, counting base and printed Building people. | In a no-Building scope, base population is the score. Equal counts remain unresolved until a tie rule is verified. |
| p7 | Population inability or zero total triggers failure; Building and LOST processing can create intermediate choices. | Unpayable losses and unresolved interruption ordering must stop instead of fabricating final scores or partial payments. |
| p8 | Recycle only when a needed next card is unavailable; added Progress cards go on top; players cannot inspect through discard piles. | Preserve lazy shuffle, top insertion and observation redaction while effects are integrated. |

The p4 Work prose calls the set four actions but lists five named actions.
That editorial inconsistency remains recorded in the earlier audit. A conditional
Mine/Farm/Research handler must label its supported subset rather than imply
that BUILD or RESTOCK is officially illegal. The project's universal commit
barrier is an engine/research contract; p4 explicitly discusses concealment of
simultaneous Build targets and does not prove all action chits are secret.

## Event queue and unknown effects

Trouble's ordering is prescribed by printed Event numbers. This audit confirms
the rule, not a complete numbered catalog. A caller-supplied queue is acceptable
as a conditional precondition only when its provenance/order is explicit; it
does not become the official setup or Event sequence merely because the engine
can execute it. Queue exhaustion is an unresolved/error boundary, not a new
terminal rule. A future legal setup always includes Twists; an unmodified
conditional fixture must not be called the default initial game.

An unsupported card must remain visibly unresolved. Do not finish it as a no-op,
continue to a later card, or silently suppress a persistent modifier. Likewise,
adding an unsupported Event through Trouble does not authorize resolving it.

## Terminal and mortality limits

For a bare Accident where both players can pay the printed loss without reaching
failure, the no-Building transition is straightforward. Exact-zero, insufficient
payment, simultaneous failure and nested effects need deliberate boundaries.
Only the genuinely resolved first death may receive the current event clock;
an unexecuted failure candidate must not create a retrospective death record.

This audit does not independently settle simultaneous terminal interruption,
what later portions of an effect continue, or a tie rule. Where the scheduler
uses the current conditional scope to complete a simple simultaneous loss, it
must state that scope and preserve both loss results before choosing an outcome.
Any unresolved branch must retain a distinct stop status, rather than treating
absent outcome/death fields as a completed draw or proven survival.

No shadow continuation, post-death action policy or common experimental horizon
is verified by these source pages. A conditional terminal record must remain
labelled by its supplied scenario and subset; it cannot establish strategic
strength or a legal whole-game result.

## Targeted acceptance cases

- Work reveal creates one decision boundary; no next reveal, resource mutation
  or private draw occurs before both commitments are locked. Reverse submission
  order should preserve results under the same explicit draw schedule.
- Trouble adds exactly one next Event, which appears before existing future
  cards and before a lazy reshuffle. Event queue exhaustion stops visibly.
- No-Building Hunger changes both base populations by two, with food unchanged;
  arithmetic failure must not partly update one player.
- Accident's safe no-Building loss is three for both players. Separate safe,
  exact-zero and unpayable cases; do not conflate pending terminal adjudication
  with survival or a completed official outcome.
- Instruction Manual selects a unique maximum from the allowed score scope;
  ties stop explicitly, and no further event changes a frozen terminal record.
- Unknown cards/modifiers block with the revealed card retained; observations
  exclude future order, the opponent's hand and lower discard content.
- The same input, seed and command trace should reproduce state, clocks,
  pending choices, known terminal evidence and stop reason.

## Work performed

Only this report was written by the audit task. No engine files were changed,
no builds/tests were run, and no complete official-card conformance is asserted.
