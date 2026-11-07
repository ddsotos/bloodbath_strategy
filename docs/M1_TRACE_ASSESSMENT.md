# Conditional trace assessment and unresolved results

`session::assess` reports the disposition of an engine snapshot without changing
it. Session Replay/JSON schema 11 includes this assessment in every checkpoint,
the final snapshot, and at the top level for consumers of audit results.

| Status | Meaning |
| --- | --- |
| `in_progress` | Resolved card boundary; the caller's trace ended before a terminal result. |
| `awaiting_input` | An active effect needs choices or an explicit ordering schedule. |
| `blocked_rule` | Unsupported rule, source exhaustion or unresolved terminal behavior prevents completion. |
| `conditional_terminal` | The implemented conditional handler reached its terminal phase. |

`reason` is null except for blocked traces, where a fixed tag identifies the
boundary. Examples include `butler_box_selector_unverified`,
`robot_terminal_interruption_unverified`, `terminal_tie_rule_unverified` and
`progress_exhaustion_not_official_end`. Exhausting a supplied source is never
promoted to an official end or draw. `terminal_resolved` requires the terminal
phase and an existing conditional outcome; `has_conditional_outcome` indicates
whether that outcome is stored.

These fields describe the existing partial engine. They do not certify the
complete game, a legal setup distribution, first-Work conclusions or a common
shadow horizon. Even `conditional_terminal` remains conditional on the documented
handler scope and assumptions. `full_game=false` and separate official/death/
shadow fields are retained. No scores or death records are synthesized.

## Handling incomplete traces

Consumers must retain the assessment alongside raw resources and death fields.
An absent death in a blocked/unfinished trace does not establish survival. A
blocked trace must not be treated as a completed comparable rollout or win/draw.
Record the stopping boundary and coverage; do not silently replace missing
outcomes with zero scores. This API does not define statistical censoring or
imputation conventions, or automatically exclude records from future analyses.

Butler is a concrete partial-transition example: population loss may be paid,
but the box tail can stop because its selector is unverified. The state records
that population progress, active card, pending Butler and `robot.completed=false`.
Its assessment reports `blocked_rule`; the box/resource snapshot is not labelled
an official final score. Waiting for a legal choice is instead `awaiting_input`.

## Mortality source boundary

Reinspection of printed p2/p6/p7 in the pinned publisher-authored reproduction
confirms Accident's loss of three, the each-player loss instruction, population
zero failure and base-plus-Building scoring. It does **not** explicitly establish
whether to complete another player's Accident payment/Building choices after
the first colony reaches zero. P6's explicit simultaneous wording is under
Hunger. Therefore mixed exact-zero Accident handling has not been extended.
Existing bare Accident terminal output retains its previously documented
conditional interpretation; it is not evidence settling the new Building case.
TODO: authoritative interruption clarification before relaxing those guards.
See RULE_PROVENANCE.md and M1_SESSION_AUDIT.md for source/edition limitations.

## Validation

Independent tests create real ready, waiting, blocked and conditional-terminal
Sessions, checking assessment purity and preserving outcome/death state. JSON
parsing independently checks a partially resolved Butler trace. The manifest
changes and old Session schemas reject; command/phase IDs and game transitions
are unchanged. No simulation speed or strategic result is claimed.

Local Windows x64 MSVC Debug build succeeded and all 26 CTest cases passed in
6.21 seconds. Independent source, implementation and record review has no
remaining P0/P1/P2 findings. Commands:

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
```
