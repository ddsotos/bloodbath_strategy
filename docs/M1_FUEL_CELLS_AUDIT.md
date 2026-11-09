# Fuel Cells source audit and implementation boundary

Audit date: 2026-11-09. I inspected the Fuel Cells entry and example on p10 of
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

## Conditional implementation

Session schema 15 implements this Building for a deliberately narrow supported
context. The hidden Work commitment records whether the player will use Fuel
Cells. RESTOCK first places both boxes; the selected Fuel Cells box is then
spent, +4 money is recorded, and one Building is drawn using the existing
seeded recycle path. The Session enters a private discard phase after the draw.
Only the activating player may choose one card from their resulting hand,
including the newly drawn card. The discard becomes public and the active Work
finishes only after that choice commits. Replay and JSON retain the activation,
draw, pending owner, and discard.

The subset accepts at most one in-play Fuel Cells per colony. It also accepts
at most one activation in a simultaneous Work and rejects an activation paired
with either player's Research. Those stops avoid inventing draw/discard
interleaving rules not covered by the reviewed example. They are implementation
boundaries, not claimed official restrictions. Source edition and physical
printing remain unverified, and no full catalog or strategy claim follows.

Synthetic contract and JSON tests cover existing-box and same-RESTOCK-box
spending, Building discard recycling, hidden commitments/draw, owner-only
discard choice, replay tampering and the unsupported timing boundary.

No official terminal result is inferred by this card handler.
