# Rules and experimental conventions checklist

**Do not assume any item is verified merely because it was mentioned during strategy discussion.** Before claiming gameplay fidelity, cite the specific authoritative rulebook page/section and card text in a versioned rule manifest.

M0 source discovery is recorded in [RULE_PROVENANCE.md](RULE_PROVENANCE.md).
No official gameplay item below is verified by the toy engine. The separate
[toy manifest](../engine/toy_manifest.txt) defines only test conventions and is
SHA-256 identified in replay/experiment records. Its thresholds, cycles,
actions, scores and bounded horizon do not resolve the official checklist.

## Verify official rules before full implementation
- [ ] Full official two-player setup: starting hand, resources, initial population and deck composition.
- [ ] Deck and cycle semantics: which draws define a round/cycle, shuffle points, insertions, and final horizon.
- [x] Exact Work action names and Research draw count reviewed against pinned publisher-authored reproduction, p4; publisher/physical-edition match remains open. See M1_RULE_AUDIT.md.
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

## M1 partial coverage

Verified against the pinned reproduction: initial base resources and Building
hand count (p3), base Mine/Farm/Research effects (p4), draw-time Building recycle
and discard inspection limit (p8). `engine/base_work.*` implements these only
in a caller-verified unmodified context with no buildings. Complete setup,
BUILD/RESTOCK targeting, Twist effects and card catalog are still unimplemented.
An explicit caller schedule for simultaneous draws is recorded as a convention,
not an official priority rule. No full first-Work comparison is possible yet.

Progress deck recycling/insertion (p4,p8) and conditional population loss (p7)
are now implemented as separate primitives; see M1_PROGRESSION_POPULATION.md.
They do not verify the Progress catalog, actual cycle horizon or official end
interruption ordering. Insufficient total with Buildings still present is an
explicit unresolved boundary; ties and final scores remain unimplemented.

M1_SESSION.md adds the connected no-Building/no-modifier subset: Work, Trouble,
Hunger, Accident (-3 reviewed on p2) and Instruction Manual. Unique terminal
results are supported only for these complete conditional paths. Unpayable
Accident, equal terminal populations, missing effects and unknown Event order
are not filled in. Terminal outcomes do not make the full scoring/end checklist
verified; caller-supplied mappings and physical-edition reconciliation remain open.

Paperwork's base draw-one/discard-two rule and feasible quota were reviewed on
p6/p8 (M1_PAPERWORK_AUDIT.md). Its conditional frame is implemented with explicit
ordering conventions and a stop for scarce shared supply. Hacker Lair immunity
and other modifiers remain outside the no-Building scope; cross-player timing
and discard-pile ordering are not claimed as newly verified official rules.

Glitch and three scoped Robot effects were reviewed on p3/p6/p7/p9; see
M1_ROBOT_AUDIT.md. The supplied Robot order is not a full setup distribution.
Exact-zero/unpayable Robot interruption blocks before effects; unknown Robots
and source exhaustion also remain explicit boundaries. Full catalogs and
terminal semantics are not made verified by these nonterminal handlers.

Kale Farm cost/population/PLAY/Farm and basic box allocation were reviewed on
p4/p5; see M1_TABLEAU_AUDIT.md. The opt-in subset supports BUILD/RESTOCK and
total-population scoring but does not establish physical copy counts or a
complete catalog. Population-changing Events/Robots stop with Buildings in play;
their adapters and general ordering/LOST semantics remain unimplemented.

The single-colony nonterminal Kale Farm adapter is now implemented separately
(M1_TABLEAU_LOSS.md), using reviewed p7 transfers/discard/box loss and p5 absence
of LOST. Event dispatch, cross-player priority and terminal cases remain open;
the full population checklist item is therefore still incomplete.

Nonterminal Accident now has a Kale Farm choice frame (M1_ACCIDENT_CHOICES.md)
using the p2 loss and p7 payment rules. Cross-player schedule is an explicit
convention; terminal interruption, Hunger/Robots with Buildings and general
LOST effects remain unresolved. No full checklist item is marked complete.
Hunger implements the p6 food formula and loss-free +2 in no-Building/reviewed
Building contexts when shortage can be paid from base without death (M1_HUNGER.md).
Its Building-loss choices and terminal cases remain blocked; full coverage is open.
The subsequent M1_HUNGER_CHOICES.md slice adds nonterminal Kale Farm losses
while preserving original p6 food requirements. Terminal/general LOST handling
and full card coverage remain open; caller schedule is a convention.
M1_ROBOT_TABLEAU.md extends p7/p9 Waiter/Mining to base-funded nonterminal Kale
contexts. Box choices, Robot Building-loss frames, other Buildings and terminal
interruption remain pending; this does not complete the Robot catalog.
M1_ROBOT_CHOICES.md now connects nonterminal Kale Farm losses for Waiter/Mining
under the p7 all-population-before-tail barrier. Other Buildings, Butler boxes
and terminal interruption remain pending; ordering protocol is a caller convention.
M1_BUTLER.md adds nonterminal Butler population payment and forced zero/single
box targets. P7/p8/p9 do not identify the multi-target box selector: TODO obtain
authoritative clarification. That tail stops after population with boxes unchanged.
Reinspection p2/p6/p7 does not establish completing an opponent's Accident
Building choices after first zero. Keep that terminal boundary blocked pending
authoritative clarification. Schema-14 assessments mark unresolved traces explicitly;
absence of death in them is not evidence of survival (M1_TRACE_ASSESSMENT.md).
Budget Cuts p11 is a conditional per-player money loss capped at available
money (M1_BUDGET_CUTS_AUDIT.md). Development setup, catalog, and general trigger
timing remain unverified.
Flag p10's cost, printed population, PLAY gain, Hunger exemption and reviewed
nonterminal loss metadata are implemented in the scoped Tableau (M1_FLAG.md).
Terminal Flag loss and full Building catalog/setup remain unverified.
Potato Farm p10 payment substitution is limited to actual payments and recorded
in hidden Work commitments; money-loss effects remain unaffected.
Moon Hotel p10's card face verifies cost 6, printed population 10, one People
per distinct positive Food-gain effect, and `PLAY: Add “Tourists”`. Its example
confirms separate +4 FARM and +1 Kale Farm gains trigger twice. Tourists' effect
and nested/simultaneous trigger ordering remain open; see
[M1_MOON_HOTEL_AUDIT.md](M1_MOON_HOTEL_AUDIT.md).
Fuel Cells p10 cost 3/population 6 and RESTOCK box effect are implemented in
schema 15 with a private post-draw discard choice. Multiple active copies,
simultaneous activations, and concurrent Research remain outside this subset;
see [M1_FUEL_CELLS_AUDIT.md](M1_FUEL_CELLS_AUDIT.md).
