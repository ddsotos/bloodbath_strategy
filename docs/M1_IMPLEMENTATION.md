# M1 conditional setup and base Work

This is a narrow M1 slice, not a full official engine. It adds a separately named
`bloodbath::base_rules` module; the M0 toy engine and replay format are unchanged.
No card effects or complete legal-action enumeration are claimed.

## Evidence and assumptions

See [M1_RULE_AUDIT.md](M1_RULE_AUDIT.md) for the independent source review and
unresolved interpretation/edition issues. The publisher-authored English
rulebook was inspected as rendered images from the pinned mirror. Its SHA-256
is `63bff1b40316f317c880f3f947c487b65d01358aaa3dc58edfe705766b0b96c6`.
The publisher download was blocked locally and its browser rendering supplied
no readable text/images. Mirror equality to the publisher's current bytes and
the intended physical printing remain **unverified**. No PDF/page image is
committed; local audit artifacts live under ignored `build/sources/`.

Implemented rule facts and targeted tests:

| Source | Conditional behavior | Test group |
| --- | --- | --- |
| p2, p3 | Validate supplied physical Building IDs and initial resource/deal fragment | setup validation |
| p3, p8 | Own hand visible; only top discard exposed, never lower pile or future order | observation privacy |
| p4 | Bare Mine/Farm/Research resource/draw primitives | base effects and repeated Work |
| p8 | On-demand Building recycle; resolve available portion when cards run out | draw/recycle/conservation |

All table references are to the pinned reproduction, not a certified complete
official ruleset. `engine/base_rules_manifest.txt` records sources, behavior
and conventions; CMake hashes it after CRLF normalization. This is a **subset
manifest hash**, not proof of complete official fidelity.

## API and guardrails

`setup_fragment` takes two supplied four-card hands and 82 supplied remaining
physical card IDs. It validates sizes and uniqueness, and assigns the reviewed
base resources. IDs are opaque instances. Synthetic IDs in tests deliberately
do not supply Building text or establish the real catalog. This does not sample
setup: no arbitrary dealing order, missing Twist, Perk or Robot deck is invented.

`WorkBatch` requires explicit `caller_verified_unmodified` scope. It rejects
unverified contexts, buildings, and already failed colonies. That flag is a
precondition the caller must establish from future effect processing; it does
not automatically certify a real position. No default real-game scenario is
silently stripped of Twist or Building effects.

Supported operations are labelled `supported_actions`, not `legal_actions`.
BUILD and RESTOCK enum values exist for the five listed Work names but are
explicitly unsupported. The source's four-versus-five editorial inconsistency
is recorded in the audit. Project commit/reveal timing is a research API
boundary; the source explicitly discusses simultaneous concealment for Build
targets and does not prove all action chits are secret.

Both players lock once; no state change/draw occurs until both have committed.
Resolution requires a caller-supplied player permutation, retained in the
record. Shared draws are allocated in that order. This schedule is an explicit
adapter convention; official simultaneous allocation/priority remains unresolved,
especially near exhaustion. It must not be used for unbiased opening comparisons
until verified or sensitivity-tested. Commit order is not draw priority.

Each batch resolves once. A next Work is a new batch from its snapshot; repetition
does not create an artificial action cap. Engine snapshots include private truth
and must never be handed to an agent. `observe` is the policy input boundary.
Internal Work records include private draws and are likewise engine/audit data.

State transitions apply to a copy and commit atomically after successful
resolution. Overflow is rejected; a failed resolution leaves the initial state
and locked commitments intact. The module is official-only conditional logic:
it does not create shadow populations, choose death criteria, score a game or
select a horizon. The existing toy shadow adapter remains separate.

## Randomness and replay

Building piles store their top at vector back. On a needed draw with an empty
deck, a nonempty discard is recycled and Fisher-Yates shuffled using the building
RNG source, incremented shuffle index and remaining-size draw identity. The index
persists in snapshots. These are versioned simulation conventions; no physical
shuffle order or card-insertion counterfactual coupling is claimed.

`WorkRecord` stores both commitments, explicit schedule, drawn IDs, seed, shuffle
indices and subset hash. `replay_work` takes a retained initial engine snapshot,
re-executes the operation and rejects differing fields/hash. This in-memory
conditional replay is separate from the M0 toy text/JSON formats. It is not a
full persistent official-game experiment format or snapshot interchange schema.

## Validation and remaining work

Run the README CMake build/CTest commands. `base_work_contracts` tests the new
module independently with synthetic catalog fixtures; existing toy/replay/JSON
tests remain required. No throughput benchmark is asserted: the module does not
yet simulate games or make strategic decisions.

Local validation: Windows x64, MSVC 19.51, CMake 4.4.4; Debug build succeeded
and CTest passed **4/4** (0.65 seconds). Exact commands on this machine:

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
```

The new contracts include 32-seed shuffle/replay checks. Independent semantic
review found no P0/P1/P2 findings. The rule audit and black-box tests were
delegated with separate file ownership; the lead reviewed and integrated both.
Hosted Windows/Linux CI is configured; PR checks provide current results.

Full M1 still needs the verified physical catalog, all setup piles, effects and
targets, progression scheduling, triggered decisions, depletion allocation,
population loss and terminal resolution. M2 production continuation and M3
research rollouts remain gated on those semantics. The actual initial state
always includes Twists; a caller-verified unmodified base Work is a conditional
kernel, not a substitute initial game or a basis for a Research recommendation.
