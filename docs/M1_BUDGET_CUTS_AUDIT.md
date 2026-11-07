# Budget Cuts source audit

Audit date: 2026-11-07. I visually inspected p11 of the local pinned
publisher-authored English rulebook reproduction recorded in
[M1_RULE_AUDIT.md](M1_RULE_AUDIT.md), SHA-256
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.
Its equality to the publisher's current PDF and intended physical printing
remain unverified.

The p11 Development Card Notes state that Budget Cuts removes two money; if a
player has only one money, remove one; if none, it has no effect. The card face
also shows `-2 money`. Session schema 12 implements this as an immediate,
per-player loss of `min(2, current money)` and records the amount actually lost.

This establishes only this card's scoped transition. It does not establish the
Development deck's composition or setup distribution, nor general timing or
trigger rules for other Development cards. No strategic conclusion is claimed.

Tests cover each player losing two, one, or zero money and verify replay and JSON
recording. These use supplied synthetic scenarios, not a sampled legal game.
