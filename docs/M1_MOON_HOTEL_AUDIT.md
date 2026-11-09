# Moon Hotel source audit and implementation boundary

Audit date: 2026-11-09. This records a candidate Building extension from p10 of
the pinned publisher-authored English reproduction cited in
[M1_RULE_AUDIT.md](M1_RULE_AUDIT.md), SHA-256
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.
The same bytes were independently re-fetched from the Tesera-hosted rulebook
mirror and matched this SHA-256. The publisher URL remains behind a Cloudflare
challenge, so equality to the publisher's currently served bytes and the
intended physical printing remain unverified; see
[RULE_PROVENANCE.md](RULE_PROVENANCE.md).

## Card face

The embedded Moon Hotel image on p10 is legible when extracted at native image
resolution. It shows cost 6 Money, printed population 10, the ability “Each time
you get +1 Food or more, +1 People,” and `PLAY: Add “Tourists”`. This verifies
the card metadata and the exact printed trigger threshold against the pinned
reproduction. The Tourists card's effect and the transition for adding it to the
Progress deck still need their own implementation-ready audit.

## Reviewed example

The p10 Building Card Notes say that Moon Hotel counts each time its owner gets
food. Its example says the base FARM gain of +4 Food and the separate Kale Farm
bonus of +1 Food cause two Moon Hotel triggers, yielding +2 People. The card
face's “+1 Food or more” wording and this example together support one trigger
per distinct positive Food-gain effect, regardless of its amount; they do not
support one trigger per Food token. No exhaustive list of every Food source or
the ordering of nested/simultaneous triggers has been audited.

## Required follow-up before implementation

- Audit the Tourists perk text, physical identity, and Progress-deck addition
  semantics before implementing `PLAY: Add “Tourists”`.
- Resolve ordering for simultaneous/nested Food gains and verify each supported
  Food-producing effect separately before connecting Moon Hotel to it.
- Add explicit fixtures for the p10 `+4` Farm plus `+1` Kale Farm example and
  each adopted trigger boundary before enabling the card in the Tableau catalog.
- Keep any unresolved case as a stop; do not infer an official transition or
  terminal outcome from this example.

This is a source-gap record, not an executable card definition or a strategic
claim.
