# Conditional Flag Building

## Source and implemented behavior

The p10 Building Card Notes in the pinned reproduction state that Flag costs no
money, has no printed people, gives +6 money on PLAY and does not require Hunger
food. The notes also say a Flag is lost when the player has no population and it
is their only Building. The publisher-byte/physical-edition limitations are in
[RULE_PROVENANCE.md](RULE_PROVENANCE.md); this is not a complete Building catalog.

Schema 13 models Flag alongside Kale Farm using per-kind cost and printed-
population metadata. Its PLAY adds six money; it does not trigger Farm bonuses.
Hunger counts all reviewed Buildings except Flag. Losing Flag during a supported
nonterminal Building-loss sequence transfers zero printed people, removes its
boxes and moves the card to the shared discard. The loss record retains the
zero transfer.

## Boundaries and tests

If a loss reaches zero total population or requires unresolved terminal
interruption, the current adapter rejects the transition without mutation. The
p10 single-Flag/no-population note is recorded, but this engine does not yet
resolve resulting failure or score. No official terminal outcome is inferred.

Synthetic contract tests cover free Build enumeration/payment, +6 PLAY money,
zero printed population, no Farm trigger, Hunger exemption, zero-transfer loss,
box/card disposal, terminal guard atomicity, deterministic replay and JSON audit.
These fixtures are not official setup samples; no strategic or full-game claim
follows.

Schema 14 extends this Tableau subset with Potato Farm; see
[M1_POTATO_FARM_AUDIT.md](M1_POTATO_FARM_AUDIT.md).
