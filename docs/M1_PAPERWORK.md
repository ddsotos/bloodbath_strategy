# M1 Paperwork draw and discard frame

This extends the conditional no-Building/no-modifier Session with Paperwork.
It does not complete M1 or implement Hacker Lair, Twist/Building modifiers,
the physical card catalog or a legal full-game setup. See the independent
[source audit](M1_PAPERWORK_AUDIT.md) for p6/p8 facts and provenance limits.
The same pinned publisher-authored reproduction is used; current publisher-byte,
physical edition and errata reconciliation remain unverified.

## Rule facts and explicit conventions

Paperwork draws one Building card per player, then discards two. The new card
belongs to the post-draw hand and can be discarded. With fewer than two cards,
discard the available hand. Hacker Lair immunity skips both parts and is not
optional; the current no-Building scope excludes this situation rather than
partially implementing immunity. Hand discard does not lose an in-play Building,
transfer printed population or run LOST solely from that operation. The caller
still certifies all relevant modifiers absent, including any discard triggers.

The source does not define a total shared-draw or discard-pile priority. The
current **versioned conventions** are therefore explicit and recorded:

1. Caller supplies a player permutation for draws.
2. Both players draw before either commits a discard choice.
3. Each privately locks distinct owned IDs, exactly `min(2, post_draw_hand_size)`.
4. Both commit before either hand/discard pile changes.
5. Caller supplies a player permutation for discard application. Within each
   player's selected vector, cards are discarded in supplied vector order.

Both player order and within-player vector order determine the public top card.
These are trace conventions, not newly verified official priorities. Reversing
commit submission order does not alter results under the same schedules/vectors.

When the **old draw deck plus old discard pile contains fewer than two cards**,
`prepare_paperwork` sets `blocked_shared_draw` before any draw, discard or
decision count change. The revealed Paperwork stays active. This avoids deciding
whether one player's new discards can supply another player's draw. It is an
unsupported timing boundary, not cancellation of the card, death, official
terminal event or completed comparable rollout. Existing old discards do count
as available supply and recycle only on a draw that needs them.

## State machine and API

`advance` reveals Paperwork and returns `paperwork_draw_order`. No cards are
drawn yet. `prepare_paperwork(draw_order)` calls the shared conditional Building
draw primitive, records one draw each and opens `paperwork_discard`. One
simultaneous discard decision increments the clock once here; administrative
schedule selection does not count as a player decision.

`observe(player)` exposes only that player's updated hand, required quota,
public resources and top discard. It never exposes pending opponent choices,
the opponent's drawn card or buried discard contents. `commit_discards` rejects
wrong quota, repeated/unowned IDs, duplicate commitment and wrong phase without
mutation. A newly drawn card is eligible. A one-card post-draw hand commits one.

`resolve_paperwork(discard_order)` requires both choices, applies them atomically,
records the discarded vectors/order, finishes the active Progress card and
returns ready. No next reveal can skip either phase. Population, money, food,
in-play state and deaths are unchanged. Repeated Paperwork frames clear previous
commitments; old records remain in the replay checkpoints.

The common `base_rules::draw_buildings` instruction accepts counts, a seed and
explicit schedule, and shares the existing on-demand recycling code with Research.
It retains the certified living/no-Building/90-physical-ID validation and commits
state atomically. It is not a new Work action or an automatic effect resolver.

## Replay and JSON schema 2

Paperwork introduced Session Replay/JSON schema **2**. Schema 1 session records reject instead
of being silently interpreted with new choice/frame fields; M0 toy replay/JSON
formats are unchanged. Manifest/dependency hashes also change. Effects retain
their previous numeric IDs and append Paperwork=6. Phases append:
paperwork_draw_order=8, paperwork_discard=9, blocked_shared_draw=10.
Commands append prepare_paperwork=3, commit_discards=4, resolve_paperwork=5.

Commands now contain selected card vectors. Checkpoints add private discard
commitments and a Paperwork record (draw requests, drawn IDs, seed, draw order,
shuffle indices, discarded IDs, discard order and completion flag). The record
persists after completion until the next Paperwork frame. JSON export verifies
all checkpoints first and explicitly retains `full_game=false`. It is private
engine/audit data, not an observation or a complete game sample. No JSON importer
or generic checkpoint restoration is introduced.
The subsequent Robot extension advances the current Session schema to 3;
see M1_GLITCH_ROBOTS.md. The Paperwork fields and commands retain their IDs.
The reviewed Tableau extension advances current Session schema to 4 and
permits this frame with known Kale Farms (no draw/discard modifiers). See
M1_TABLEAU.md; other Building immunity/effects remain unsupported.

## Run and validation

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
& ./build/Debug/session_demo.exe --paperwork
```

The synthetic supplied scenario executes Work, Paperwork and Instruction Manual.
Research gives player 0 two cards; the explicit Paperwork draw order [1,0] gives
players [86] and [87]. They discard [0,86] and [4,87] in player order [1,0], so
the final Building discard is [4,87,0,86]. The final conditional scores are
[30,29] at local Progress event 3, decision 2. These IDs/populations are fixtures,
not an official catalog or sampled opening. Python independently parses both
demo modes, checks conservation and validates frame/command data.

`paperwork_contracts` adds independent tests for phase barriers, new-card choices,
partial quotas, explicit schedules, invalid commitments, hidden choices,
old-supply blocking, recycling, repeated frames, record metadata and replay.
The source review, independent test file and engine changes have separate
owners; the lead integrates and reviews their deliverables.

Final local Windows x64 Debug build (MSVC 19.51, CMake 4.4.4): **9/9 CTest
cases passed** in 1.63 seconds. `git diff --check` passed. Independent source,
semantic and output review has no remaining P0/P1/P2 findings. Hosted
Windows/Linux CI is the PR validation gate.

## Reproducible microbenchmark

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Release --target session_demo
& ./build/Release/session_demo.exe --benchmark-paperwork
```

Windows x64, AMD Ryzen 7 3700X, MSVC 19.51, CMake 4.4.4 Release baseline:
1,000 ten-command synthetic trace recordings and full verifications in
**0.0821642 seconds**, approximately **12,171 traces/sec**, checksum **3000**.
Seeds are 0..999; this fixed scenario does not shuffle. JSON writing is excluded.
This measures trace overhead, not full-game throughput or strategic strength,
and is not a measured improvement over the differently scoped earlier demo.

## Remaining rule work

Full card catalog/setup, Hacker Lair and other modifiers, BUILD/RESTOCK,
remaining Events/Twists/Robots/Perks/Developments, shared-priority conventions,
unpayable loss interruption, ties and production shadow continuation remain
open. Conservative blocked states and absent outcome/death fields must never
be interpreted as proven survival or a completed win/draw.
