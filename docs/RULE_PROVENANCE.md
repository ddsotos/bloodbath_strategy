# Rule source ledger

Checked: 2026-11-06. Scope: M0 source discovery for the English base game,
restricted to two-player research. Finding a source does not verify every rule
or establish that the engine implements it.

M1 update: pages of a pinned publisher-authored reproduction have now been
visually audited. See [M1_RULE_AUDIT.md](M1_RULE_AUDIT.md) for the reviewed
facts, the mirror's SHA-256, and its still-unverified byte equality to the
publisher-hosted file. The M0 discovery statements below describe the earlier
stage. Conditional setup/base-Work primitives are implemented as specified in
[M1_IMPLEMENTATION.md](M1_IMPLEMENTATION.md); catalog completeness, edition and
real-game resolution remain open.

The follow-up [progression/loss audit](M1_PROGRESS_AUDIT.md) reviews p4/p7/p8
primitives against the same pinned reproduction. See
[implementation boundaries](M1_PROGRESSION_POPULATION.md), especially the
unresolved terminal interruption and unknown LOST effect guards.

[M1_SESSION_AUDIT.md](M1_SESSION_AUDIT.md) reviews the first connected card paths,
including Accident's printed three-person loss on p2 and Hunger's empty-tableau
case on p6. See [M1_SESSION.md](M1_SESSION.md) for exact support and stops.

[M1_PAPERWORK_AUDIT.md](M1_PAPERWORK_AUDIT.md) reviews Paperwork's draw/discard
rule, immunity and feasible quotas, while separating explicit scheduling
conventions from the printed rules. Implementation limits are in M1_PAPERWORK.md.

[M1_ROBOT_AUDIT.md](M1_ROBOT_AUDIT.md) reviews Glitch and three illustrated
Robot effects against the same pinned reproduction. No complete Robot catalog
or physical-edition reconciliation is established; M1_GLITCH_ROBOTS.md records
the safe conditional paths and explicit terminal/tail guards.

[M1_TABLEAU_AUDIT.md](M1_TABLEAU_AUDIT.md) reviews Kale Farm's illustrated
definition and Build/Restock semantics. M1_TABLEAU.md records opt-in scope,
physical-ID/copy-count limits and affected Event guards.

## Sources

| ID | Source | Verification status | Limits |
| --- | --- | --- | --- |
| RGG-PRODUCT | [Rio Grande Games publisher page](https://www.riograndegames.com/games/moon-colony-bloodbath/) | Publisher search result identifies Moon Colony Bloodbath, Donald X. Vaccarino, and stock number RIO647. | Direct page retrieval returned HTTP 403. Search-index text is sufficient for source discovery, not a rule-conformance manifest. |
| RGG-RULEBOOK | [Publisher-hosted English rulebook, MCB.pdf](https://www.riograndegames.com/wp-content/uploads/2024/07/MCB.pdf) | Direct retrieval succeeded as a 12-page PDF; document metadata title is `MCB_Rules.indd`. | The browser returned no readable text lines. Individual page/card semantics have not been audited for implementation. The URL path is not an edition identifier. |

The publisher product listing supports the broad identification of the game and
its multiplayer scope. Its summary does not resolve terminal-event ordering,
ties, or interactions. Do not use that summary as an executable rule specification.

## Edition and catalog completeness

- **Unverified:** explicit revision/printing identifier, any applicable errata,
  and whether the linked PDF matches the physical edition used for research.
- **Unverified:** complete base-game card catalog, multiplicities, exact text,
  and required interactions. A rulebook URL alone is not a complete card dataset.
- **Unverified:** all two-player setup values, action effects, legal visibility,
  simultaneous resolution, death checks, score rules, and deck exhaustion timing.
- Expansion material is outside this source ledger's scope. No expansion rules
  have been adopted.

No official rule or card effect is marked implementation-ready by this document.
See [RULES_CHECKLIST.md](RULES_CHECKLIST.md) for the unresolved gameplay items.

## Required rule-manifest work before official transitions

1. Audit the publisher PDF page images and confirm the edition against the
   intended game; locate publisher/designer errata where applicable.
2. Obtain and inventory the complete card texts legally available to the project.
   Record which entries are present, missing, or ambiguous without redistributing
   a full copyrighted rulebook or catalog unnecessarily.
3. For each adopted rule, record source ID, page/section or card ID, verified
   interpretation, and a targeted test. Record unresolved interactions explicitly.
4. Pin downloaded source bytes with a cryptographic digest and retain a versioned
   semantic manifest. A mutable URL is not a reproducible ruleset hash. M0 toy
   fixture identifiers must never be presented as an official ruleset digest.

## Research conventions

The [Accident choice frame](M1_ACCIDENT_CHOICES.md) connects the reviewed p2
three-person loss to p7 payment and p5 Kale Farm metadata. Cross-player discard
schedule remains a recorded caller convention; terminal cases with Buildings
remain blocked before mutation. Session schema 5 records these choices/results.

The later [nonterminal Kale Farm loss audit](M1_TABLEAU_LOSS.md) visually checks
printed p7 population payment/Building transfer/discard/box loss and the p5
absence of Kale Farm LOST effects in the pinned reproduction. Its adapter uses
explicit caller-selected order and rejects terminal cases. This extends only
the reviewed subset; it does not resolve complete card coverage or edition match.

Event-clock indexing, synthetic toy populations and transitions, a bounded toy
horizon, post-death fixed policy, and shadow continuation are research fixture
conventions. They do not establish official rules. Each executable fixture must
label and version its own conventions; official population/death semantics and a
real-game shadow adapter remain pending rule verification.

## Discovery method and limits

Searched for the publisher's game page and English rulebook, then directly opened
the two URLs above. PDF screenshot requests were also made for pages 1, 2, and 12;
no detailed visual rule audit is asserted here. No source file was downloaded
into the repository, hashed, or redistributed. No engine changes or gameplay
tests are part of this documentation-only source discovery.
