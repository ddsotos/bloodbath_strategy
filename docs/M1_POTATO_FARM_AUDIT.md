# Potato Farm source audit

Audit date: 2026-11-08. I visually inspected p10 of the pinned
publisher-authored English rulebook reproduction recorded in
[M1_RULE_AUDIT.md](M1_RULE_AUDIT.md), SHA-256
`63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.
The publisher's current bytes and intended physical printing remain unverified.

The card face shows cost 5 money, printed population 6 and FARM +2 food. The
card's passive text and p10 note allow food to substitute for money when paying,
including any mix of the two resources. This applies to the reviewed BUILD cost.
An in-play Potato Farm is required before the cost is paid; a Potato Farm being
built cannot fund itself. The p10 note explicitly says this does not convert
Budget Cuts or another money loss into a food payment. It also does not make
money usable to pay food costs.

Schema 14 records the privately committed food portion of a BUILD payment and
the actual money/food amounts separately. Contracts cover mixed and all-valid
splits, unavailable substitutions, secrecy, replay/JSON and the Budget Cuts
interaction. No complete Building catalog or strategy claim follows.
