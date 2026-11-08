# Fuel Cells source audit and implementation boundary

Audit date: 2026-11-08. I inspected the Fuel Cells entry and example on p10 of
the pinned publisher-authored English reproduction cited in
[M1_RULE_AUDIT.md](M1_RULE_AUDIT.md), SHA-256
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.
The publisher's current PDF endpoint returned a Cloudflare challenge page to a
direct download attempt, so equality with current publisher bytes and the
intended physical printing remain unverified.

## Reviewed behavior

The p10 note explicitly describes this sequence: choose RESTOCK, place one of
the two new boxes on Fuel Cells, spend that box for +4 money, then draw a
Building card and discard a card. This establishes that a box placed by the
same RESTOCK can be spent immediately in the example. It does not by itself
resolve every legal ordering or how a newly drawn card participates in the
discard choice.

The local page render is not legible enough to independently record the full
Fuel Cells card face, including its cost, printed population, and exact optional
wording. Do not add it to the supported catalog from the note alone.

## Required implementation work

- Obtain a legible source for the full card face and record all printed metadata
  and exact ability text.
- Add a privately recorded choice to spend a box on Fuel Cells during RESTOCK,
  including boxes just placed by that action.
- Model the +4 money and Building draw, then capture the discard choice after
  the drawn card is known; keep that card private from the opponent.
- Preserve replay/JSON data for the extra choice and validate an empty-deck
  recycle path against the adopted Building-deck rule.
- Add fixtures for both spending an existing box and spending a box placed by
  that RESTOCK, plus invalid/unavailable choices and hidden-draw privacy.

No gameplay handler is approved by this partial audit. Unknown or unresolved
cases must stop explicitly; no official terminal result is inferred.
