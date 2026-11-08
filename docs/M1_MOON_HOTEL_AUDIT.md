# Moon Hotel source audit and implementation boundary

Audit date: 2026-11-08. This records a candidate Building extension from p10 of
the pinned publisher-authored English reproduction cited in
[M1_RULE_AUDIT.md](M1_RULE_AUDIT.md), SHA-256
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.
The source bytes' equality to the current publisher PDF and the intended
physical printing remain unverified; see [RULE_PROVENANCE.md](RULE_PROVENANCE.md).

## Reviewed example

The p10 Building Card Notes say that Moon Hotel counts each time its owner gets
`+1 food`. The printed example says a Farm produces `+4 food` and an owned Kale
Farm produces another `+1 food`, so Moon Hotel gives `+2 people` in that case.
This supports counting those two cited food gains as two triggers in that
example. It does not establish that each individual food token is a separate
trigger, or resolve every other source of food gain.

The card image's cost, printed population, full card text, and PLAY effect were
not legible enough in the available local render to audit independently. In
particular, do not assume that BUILD/PLAY is a Farm or that the card's own food
gain can trigger itself. No implementation or rule manifest entry is approved
from this partial observation.

## Required follow-up before implementation

- Obtain a legible source for the full Moon Hotel card face and independently
  record cost, printed population, all triggers, and PLAY behavior.
- Resolve the trigger unit and ordering for simultaneous or nested food gains,
  including whether Moon Hotel itself can cause a further trigger.
- Add explicit fixtures for the p10 `+4` Farm plus `+1` Kale Farm example and
  each adopted trigger boundary before enabling the card in the Tableau catalog.
- Keep any unresolved case as a stop; do not infer an official transition or
  terminal outcome from this example.

This is a source-gap record, not an executable card definition or a strategic
claim.
