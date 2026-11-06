# M0 implementation and limits

This slice implements the M0 research contracts and initial M1 observation and
commitment boundary. All executable transitions are **toy-only**. No official
Moon Colony Bloodbath rule/card transition is implemented. Source discovery and
remaining edition/card gaps are in [RULE_PROVENANCE.md](RULE_PROVENANCE.md).

## Files and semantics

- `engine/core.hpp`, `engine/core.cpp`: schema-v1 value types, event-addressed RNG,
  private-state observations, simultaneous commitments, first-death tracker,
  frozen official values, explicit `ToyShadowRulesAdapter`, replay and JSON export.
- `engine/toy_manifest.txt`: exact toy conventions; CMake computes SHA-256 after
  normalizing CRLF to LF so Windows/Linux checkouts identify the same fixture.
  This hash identifies a toy manifest, never the official game's rules.
- `tests/core_tests.cpp`: independent boundary and mortality assertions plus 256
  seeded full-result/replay comparisons. Tests remain active in Release builds.
- `tests/check_record.py`: independent standard-library JSON parsing test when
  Python is available. `tools/replay_verify.cpp`: standalone replay verifier/exporter.
- `.github/workflows/ci.yml`: Windows/Linux build and test jobs; `.clang-format`
  and compiler warnings configure basic code hygiene. CI results are pending.

The toy clock starts at zero. Event e uses absolute index e, zero-based cycle
`(e-1)/4`, one-based cycle position `(e-1)%4+1`, and decision index e. Each toy
event happens to contain one simultaneous decision; independent real-game
decision/progression scheduling is deferred. Never-dead is `nullopt`/JSON `null`.

At the first toy terminal event, the official score vector, winner, cause and
clock are copied into `OfficialOutcome` once. All later writes affect only the
toy continuation state and final `ShadowOutcome`; the official value is immutable
through the tracker API. Each death is recorded once. Dead players permanently
use `toy-wait-v1`, even after recovery or when gain remains legal. Both players
reach the caller-specified horizon. Empty action sets are impossible for this
fixture; the policy rejects them rather than guessing a real-game fallback.

The toy normal population and continuation state use the same value structure;
there is no production official engine yet. All post-terminal state mutation is
routed through the named toy adapter. Signed continuation and periodic recovery
are defined toy transitions, not synthetic adjustments to reported scores;
the separate synthetic score adjustment vector is zero.

RNG calls use `(seed, source, event, draw)` and are independent of query order.
Repeated identities deliberately return the same value. Identical identities
couple only the same toy hazards. This does not justify deck-addition coupling.
`event-splitmix64-v1` uses specified uint64 arithmetic and bounded rejection;
no implementation-dependent standard distributions or mutable RNG cursor.

Replay schema v1 is ASCII whitespace-separated: magic `bloodbath-replay`, schema,
toy rule version, seed, horizon, manifest SHA-256, RNG version; followed by exactly
horizon lines of two action IDs and two signed deltas. Replay checks the version,
hash, RNG, action policy and regenerated deltas. Truncation/trailing data fail.
Clocks/deaths/outcomes are reconstructed; arbitrary injected actions, snapshots,
partial replay, official rules and full experiment-record import are deferred.

JSON records retain schema, manifest hash, RNG/agent/policy/adapter versions,
seed, initial toy configuration, explicit horizon, branch/coupling IDs, official
and shadow raw scores, each first-death record, adjustment vector and replay.
Winner IDs: 0=player0, 1=player1, 2=draw. Strings are ASCII fixture metadata;
non-ASCII bytes are escaped and are not a Unicode metadata interface. Runtime
is excluded from deterministic records; performance/strength benchmarks are
deferred because these are contract fixtures, not a gameplay implementation.

## Local validation

Windows x64, MSVC 19.51.36256, Windows SDK 10.0.19041, CMake 4.4.4.
CMake was installed locally using `python -m pip install --target build-tools cmake`.
On this machine it is outside PATH; exact commands are:

```powershell
& ./build-tools/cmake/data/bin/cmake.exe -S . -B build -G 'Visual Studio 18 2026' -A x64
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
& ./build/Debug/replay_verify.exe
& ./build/Debug/replay_verify.exe --json
```

Independent review found a locale-dependent serialization issue; streams now use
the classic locale and a grouped-number locale regression test protects it.
Observation tests cover both commitment orders and altered hidden state.
Final local Debug build and CTest: **3/3 passed**, including 256-seed contracts,
standalone replay roundtrip and independent JSON parsing (0.49 seconds in the
recorded final run before documentation-only updates).

Covered portions of [M0_TEST_PLAN.md](M0_TEST_PLAN.md): BUILD-1, RNG-1/2/3 toy
source isolation, REPLAY-1/2 supported fields, OBS-1/2, COMMIT-1/2, toy CLOCK-1,
DEATH-1, OUTCOME-1, SHADOW-1, POLICY-1, common-horizon and JSON RECORD-1.
Deferred: snapshot replay, independent clock scheduling, empty-action fallback,
censored adapter outcomes, explicit externally scheduled edge fixtures, scalar
mortality objectives and official game semantics. No death reward or strategic
ranking is implemented. The M2 checklist remains open for production semantics.

Rule-source discovery and black-box test planning were independently delegated
before the server restart; their persisted reports were reviewed. A subsequent
independent code review covered the bounded toy contracts. No model-specific
performance or game-strength claim follows from delegation or passing tests.
