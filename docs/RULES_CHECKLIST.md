# Rules and experimental conventions checklist

**Do not assume any item is verified merely because it was mentioned during strategy discussion.** Before claiming gameplay fidelity, cite the specific authoritative rulebook page/section and card text in a versioned rule manifest.

## Verify official rules before full implementation
- [ ] Full official two-player setup: starting hand, resources, initial population and deck composition.
- [ ] Deck and cycle semantics: which draws define a round/cycle, shuffle points, insertions, and final horizon.
- [ ] Exact first Work action names and number of drawn cards for Research.
- [ ] Simultaneous Work selection/reveal and ordering of resolution.
- [ ] Complete building/event/robot/development catalog and triggered effects; which effects are optional.
- [ ] How population on buildings is counted, lost, paid, or made negative.
- [ ] Exact official elimination/end triggers: at population zero, below zero, inability to pay loss, or combination; timing within event resolution.
- [ ] How official scores and ties are determined when a player dies.
- [ ] Whether any card effects continue resolving when an official terminal condition triggers.
- [ ] Information visibility: private hands, face-up/discard zones, known ordering of cards.
- [ ] Whether any resources other than population can be negative and when checks occur.
- [ ] Whether building loss, debt or action opportunities exist after colony death (officially likely undefined).

## Define research-only conventions separately
- [ ] Versioned common shadow horizon and what happens after official deck exhaustion.
- [ ] Shadow signed population and deficit representation where official state transitions end.
- [ ] How mandatory population loss, missing buildings and impossible actions resolve in shadow.
- [ ] Fixed post-death policy action priority and no-action fallback; never switch back.
- [ ] First-death timestamp indexing convention, tie/death on same event, and survival sentinel.
- [ ] Score treatment when population is negative at shadow horizon.
- [ ] Evaluations and mortality penalties (configurable, not hardcoded).
- [ ] Coupling strategy when a shared-deck card is added vs omitted.

## Change management

Label every rule entry **verified**, **unverified**, or **shadow convention**. Include rulebook edition/URL, original excerpt location, examples, and test fixture reference. If text is unclear, write a failing/pending test and ask for source clarification rather than filling gaps from intuition.
