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

I extracted the individual Fuel Cells image embedded in the pinned PDF's p10
page and visually inspected it at its native 174x235 pixels. The card face
shows cost 3 money, printed population 6, and: “RESTOCK: You may spend a box
from here. If you do: +4 money, +1 card, -1 card.” The p10 note clarifies that
the new box may be placed on Fuel Cells by that same RESTOCK before it is spent.
This card text is now reviewed for the pinned reproduction, although the
publisher-byte and physical-edition caveats above remain.

## Required implementation work

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
