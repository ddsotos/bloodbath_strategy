# Base-funded Robots with reviewed Kale Farms

This extends the [Glitch/Robot slice](M1_GLITCH_ROBOTS.md). Independent visual
review of printed p7/p9 and p5 Kale Farm in the pinned reproduction confirms:
all players pay Robot population loss before any remaining instructions. Robot
Waiter loses four people then up to two food. Mining Robot loses six people and
grants money only for the named Mine types; Kale Farm is not one of them. Its
FARM/PLAY instructions do not trigger on Robots. Source SHA-256 and edition
limits remain those in RULE_PROVENANCE.md and M1_ROBOT_AUDIT.md.

## Scope

This document describes the schema-8 base-funded slice. The later
[Robot choices](M1_ROBOT_CHOICES.md) add nonterminal Kale Farm losses and schema
9, superseding the Building-loss stop described below.

Waiter and Mining Robot now support reviewed Kale Farms when BOTH bases cover
the loss and BOTH total populations strictly exceed it. Base may become zero
while retained Building population keeps the colony alive. No Building is lost
in this slice; boxes, hands, draw piles and money remain unchanged. Waiter food
loss is `min(2, food)` after both population losses; Mining's Kale-only bonus is
zero. Automatic processing finishes the active card and adds no decision.

If either player needs a Building loss or terminal handling, the Robot stays
active in `blocked_robot_loss`, with both colonies unchanged and no death or
official outcome. This is unresolved processing, not survival or immunity.
Butler with any Building still stops as `blocked_tableau_effect`, because its
box choice is not implemented. Unknown effects and other Buildings keep their
existing explicit boundaries. Supplied private Robot source/setup assumptions
are unchanged. Existing no-Building handlers retain their outcomes.

## Records

Session Replay/JSON schema 8 adds a nullable `robot` audit checkpoint field with
effect tag, per-player population payment components, food loss and money gain.
Successful records have no lost Buildings/shortfall/death; tail values are logged
even when zero. Each known Robot clears the previous record before processing;
blocked Robot frames have no completed record. Completed records may persist
across unrelated card types. No new phase/command or policy information is added.
Old Session schemas reject; the manifest hash changes.

## Validation

Synthetic physical-ID fixtures cover base depletion with printed survival,
partial/absent food, mining bonus absence, unchanged Buildings/boxes, both-player
preflight, terminal/Building-choice stops, Butler boundary, repeated records,
automatic clocks, deterministic replay and tampering. Python parses a supplied
trace independently. No full catalog, terminal semantics, shadow continuation,
throughput gain or strategy result is claimed.

Local Windows x64 MSVC Debug build succeeded and all 20 CTest cases passed in
4.57 seconds. Independent source, implementation and record review has no
remaining P0/P1/P2 findings. Commands:

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
```
