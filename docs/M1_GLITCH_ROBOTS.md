# M1 Glitch and conditional Robots

This connects Glitch and three reviewed Robot effects to the no-Building,
no-modifier Session. It does not complete the catalog, legal setup or M1/M2.
[M1_ROBOT_AUDIT.md](M1_ROBOT_AUDIT.md) records the independent visual review of
the pinned publisher-authored reproduction (p3,p6,p7,p9). Current publisher-byte,
physical-edition and errata reconciliation remain unverified.

## Sources and conditional behavior

| Source | Effect | Implemented no-Building path |
| --- | --- | --- |
| p3,p6 | Glitch | Consume one next Robot for the table from the supplied already-shuffled source, place on Progress top, then finish Glitch. |
| p7,p9 | Robot Butler | Lose four people each; no boxes exist to lose. |
| p7,p9 | Robot Waiter | Lose four people each, then `min(2, available_food)`. |
| p7,p9 | Mining Robot | Lose six people each; no listed Mines exist, so no money bonus. |

Every Robot loses printed population before its tail. Current support requires
**both** players strictly above that loss. If either is equal or below,
`blocked_robot_loss` retains the active Robot and changes neither colony's
population, tail resources, deaths nor outcome. This conservative unsupported
boundary avoids settling terminal/tail interruption. It is not an official
immunity, cancellation or proof of survival. Unlike plain Accident, these
Robots have additional instructions after the loss. No shadow continuation is
implemented to pass that boundary.

Unknown Robots stop before any guessed loss. Missing Robots return
`blocked_robot_deck`, not an automatic refill or game end. The reviewed recycle
rule applies to Progress/Building decks, not an invented Robot source recycle.

## Physical sources and observations

`Config::ordered_robots` is private caller-supplied **top-first, already-shuffled**
order. It does not verify all twenty Robot definitions, multiplicities or the
probability of that order. Glitch does not resample or shuffle on each reveal.
The caller must supply the certified effect context and source provenance.

Glitch consumes `ordered_robots[next_robot]` once and increments the index. The
added instance is the next separate Progress reveal, before older cards or a
lazy recycle. Later it recycles inside Progress, without being consumed again
from the Robot source. The immutable source vector is input history; count
physical conservation with **only its unconsumed suffix**.

Validation rejects duplicate physical IDs across Progress, queued Events and
remaining Robots. Known non-Robot tags cannot enter the Robot source; known
Robot tags cannot be queued as Events. Opaque unknown Robots may be inserted,
but their effects remain unresolved on reveal.

`observe(player)` omits source order and `next_robot`. After Glitch, the newly
inserted face-down top identity is not leaked via the current/discard card.
JSON/snapshots include private truth for audit only and must not reach agents.

## Replay schema 3

Session Replay/JSON now use schema **3**, rejecting older session schemas.
M0 formats are unchanged. Config adds `ordered_robots`; checkpoints add
`next_robot`. Manifest hashing includes the new semantics and dependency
hashes. Commands and existing numeric tags retain their values.

New effect tags: Glitch=7, Robot Butler=8, Robot Waiter=9, Mining Robot=10.
New phases: blocked_robot_deck=11, blocked_robot_loss=12. No new command is
required. Blocked states are not completed comparable rollouts; absent death
or outcome does not establish survival or an official draw.
The later Tableau extension advances current Session schema to 4, leaving
these Robot IDs unchanged and blocking Robot paths with any Buildings in play.

## Run and validation

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
& ./build/Debug/session_demo.exe --robots
```

The synthetic supplied scenario executes Work(Research/Farm), Glitch, Robot
Waiter and Instruction Manual. Initial populations [30,29] are fixture inputs,
not sampled setup. Final conditional scores are [26,25], food [2,6], local
Progress event 4 and decision 1. JSON explicitly retains `full_game=false`.
Python independently validates results, next-card insertion and physical
Building/Progress/remaining-Robot conservation in all three demo modes.

Independent `robot_contracts` covers one-per-table insertion, private order,
exhaustion/unknown barriers, safe tails, strict all-player loss guards, zone
validation, repeated Glitch/recycle and deterministic replay/tampering. Source
audit, test file and engine changes have disjoint ownership and lead review.

Final local Debug build (MSVC 19.51, CMake 4.4.4): **10/10 CTest cases passed**
in 1.20 seconds, including independent Robot tests and all JSON demo modes.
Source/semantic/output review has no remaining P0/P1/P2 findings. Hosted
Windows/Linux CI remains the PR validation gate.

## Reproducible microbenchmark

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Release --target session_demo
& ./build/Release/session_demo.exe --benchmark-robots
```

Windows x64, Ryzen 7 3700X, MSVC 19.51, CMake 4.4.4 Release: 1,000 seven-command
synthetic trace recordings plus full verifications in **0.0617762 seconds**,
about **16,188 traces/sec**, checksum **4000**. Seeds 0..999 are recorded but
this scenario does not shuffle; seeded recycle coverage is tested separately.
JSON writing is excluded. This measures trace overhead, not full-game speed,
improvement or AI strength. Earlier demos measure different effect paths.

## Remaining work

Other Robot tails, terminal Robot interruption, Building/box effects, full
catalog/setup, BUILD/RESTOCK, remaining card types, ties and production shadow
continuation are unimplemented. No first-Work strategy conclusion follows.
