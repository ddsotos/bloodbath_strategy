# M1 independent rule audit

Audit date: 2026-11-06. This is a source review, not a claim that these
rules are implemented or that a complete card catalog is available.

## Evidence and provenance limit

The [publisher rulebook URL](https://www.riograndegames.com/wp-content/uploads/2024/07/MCB.pdf)
was independently opened through the browser: it identifies a 12-page PDF,
but returned no extractable page text. Local publisher retrieval was blocked.
The lead supplied rendered images from a
[mirror reproduction](https://tesera.ru/images/items/2462157/Moon_Colony_Bloodbath_Rulebook.pdf)
for visual inspection. Its SHA-256 is
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.

I visually reviewed printed pages 2-8 and 12. Page 12 identifies Rio Grande
Games and copyright 2024; no printing/revision identifier was identified.
Publisher/mirror byte equality has **not** been established. The facts below
are verified against this pinned reproduction of the publisher-authored rules;
authenticity/edition reconciliation remains a distinct provenance gap.

## Extracted rule facts

| Location | Reviewed fact |
| --- | --- |
| p3 setup | Each player: four Building cards, thirty base people, four money, four food, five personal Perks, four action chits. Initial Progress: two studied random Twists, two Trouble, four Work; exclude solitaire Loneliness. |
| p3, p8 visibility | Hands private. Discards face up, but players cannot inspect through either discard pile. |
| p4 Work | Listed actions: Restock two boxes onto owned buildings; Build from hand, pay cost and resolve PLAY; Mine four money; Farm four food; Research two Building draws. Repeating an action is unrestricted. |
| p4 timing | Same-time builders conceal targets until simultaneous reveal. Triggered building abilities can precede/follow the action and be ordered freely. |
| p4, p8 decks | Reshuffle only when another card is needed and its deck is empty. Added Progress cards go on top; simultaneous additions are shuffled together. Building draws recycle Building discards. |
| p5 buildings | Printed people contribute to population; losing a building moves those people to the base. |
| p7 mortality | Insufficient total people or zero population ends play. Scores count base plus printed building people. Complete LOST effects before continuing a loss. |
| p8 effects | Resolve feasible portions of instructions; optional payments require actual payment. |

## Decisions and remaining risks for implementation

The p4 introduction describes four actions but explicitly enumerates five.
The five named operations are the useful implementation contract; preserve this
editorial inconsistency in the ledger rather than silently deleting an action.

The Work text depicts action chits placed on mats before actions are performed;
the explicit secrecy requirement concerns simultaneous Build targets. It does
not independently establish a universal hidden-action protocol. A software
commit/reveal barrier can enforce the project's simultaneous-commitment invariant,
but should not be cited as an additional printed rule about all action chits.

Suggested conformance fixtures for the lead's implementation:

- Exact setup resources, hand sizes, initial Progress multiset and absence of
  solitaire cards; preserve opaque identities until the catalog is verified.
- Equal observations when only another player's hand or deck order changes;
  reject queries that expose discard contents beneath the top card.
- Base Mine/Farm/Research outcomes, Research exhaustion/recycle, and card
  conservation. Inputs with unsupported trigger cards must stop explicitly.
- A Work action cannot be represented as a complete real-game resolution merely
  by applying its base resource delta: triggered effects introduce choices and
  ordering. BUILD and Restock need target choices, not just action enum values.
- Progress exhaustion must not trigger an early reshuffle that would swallow a
  newly inserted top card. Cycle/index conventions remain an engine contract.

Still unresolved: full card catalog and multiplicities by identity; matching the
intended physical edition/errata; simultaneous allocation when a shared Building
deck cannot satisfy all draws; terminal interruption ordering during simultaneous
losses; tie handling; complete target legality and all triggered effect chains.
Do not turn those gaps into invented defaults. Production shadow continuation,
post-death resource handling and a common horizon remain separate research
conventions rather than facts supplied by this source.

## Work performed

Read README and required design/checklist documents; opened the publisher PDF;
visually inspected the lead-supplied page images. Changed only this audit report.
No engine code changed and no tests/builds were run by this audit agent.
