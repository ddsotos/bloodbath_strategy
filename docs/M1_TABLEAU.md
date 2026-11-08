# M1 reviewed Kale Farm Tableau

This adds the first typed Building and its effects alongside the strict bare
Work mode. It is not a complete catalog, card distribution or legal setup.
[M1_TABLEAU_AUDIT.md](M1_TABLEAU_AUDIT.md) independently reviews the pinned
publisher-authored reproduction, including enlarged p5 Kale Farm text. Printing,
publisher-byte equality, errata and exact copy counts remain unverified.

## Source-backed behavior

Kale Farm costs **2 money**, has **5 printed people**, gains **1 food on FARM**,
and its PLAY performs FARM (p5). Build pays cost, moves the hand card into play,
then resolves PLAY (p4). The new Farm is already active: first PLAY gains
**4+1 food**, a second own Farm's PLAY gains **4+2**, and later Farm also gains
six with two own Farms. Opponent Farms do not contribute. This specific set of
additive triggers commutes; canonical base-then-bonuses logging is not a generic
ordering engine for other cards.

Printed people stay on the card, not base tokens. Total population is base plus
five per Farm. Zero base is not death with printed population remaining (p5,p7).
Restock places two boxes on owned Buildings, together or split (p4); with no
Buildings, no base box store is invented. No arbitrary box capacity/component
limit is imposed (p2); integer overflow rejects as an implementation boundary.

`Config::tableau_enabled` explicitly opts into the reviewed mode. Each in-play
instance needs an adopted `Kind::kale_farm` definition. The existing scope flag
certifies absence of **unmodeled external effects**, while adopted Farm effects
run. Unknown in-play Buildings reject before any ability can be skipped. Unknown
hand cards remain opaque, unsupported Build targets. Caller-supplied physical IDs
do not establish official copy counts; all 90 physical IDs remain conserved.
Legacy bare Work/Session entry points retain their no-Building guards.

## Choices and transitions

`tableau::Choice` carries an action and Build target or box-target vector.
`Session::commit_choice` locks the full choice without resource/card changes;
both players must commit before resolution. `commit` retains the basic
Mine/Farm/Research interface. Other-player observations cannot see a face-down
target. The explicit caller resolution schedule remains logged.

`work_options` enumerates **implemented choices**, not all legal full-game
actions: basic three, known affordable own Build targets, and canonical same/
split box allocations. It depends only on own hand/public state; one's own
commitment closes one's options. Rich targets remain private until resolution.

The effect log records payment, entry, Farm base, each own Farm bonus, each box,
Mine gain and drawn cards. `enter_play.amount=5` means printed population, not
base-token gain. Completed records retain choices, seed/schedule/draw metadata,
effect rows and the Tableau manifest hash. All mutations commit only after
validation. Food, boxes, total-population and Session signed-score overflow
reject atomically, including standalone resolver calls.

## Draw and Event boundaries

The new explicit `DrawScope` overload is for certified effect resolvers. It has
no printed-card metadata: the caller must prove a living total and no draw
modifiers. Tableau validation establishes those preconditions for known Farms,
including base-zero/positive-printed-population cases. The older draw entry
still enforces the bare context. Research and Paperwork can use this route;
Kale Farm has no draw/discard trigger or immunity. Paperwork never changes the
in-play Farm, printed people or boxes.

In the original Tableau slice, Hunger, Accident and population-loss Robot
paths stop as `blocked_tableau_effect` before **all** population/resource
mutation. The old no-Building formulas are not reused. Trouble/Glitch can add
cards because Kale Farm has no such trigger. Instruction Manual counts reviewed
printed people; ties remain unresolved. A blocked trace is not completed play,
survival, immunity or official end. Building loss and related Event adapters
remain unimplemented.
Later extensions add [nonterminal Accident choices](M1_ACCIDENT_CHOICES.md)
and [base-funded Hunger payments](M1_HUNGER.md). Robot paths with Buildings
remain blocked; refer to those extension documents for current boundaries.

## Schema 4 and information boundary

Session Replay/JSON schema **4** rejects older session schemas; M0 formats are
unchanged. New command commit_choice=6 and phase blocked_tableau_effect=13 append
without changing previous IDs. `cards` encodes one Build target, Restock targets,
or no targets for basic actions.

Config records opt-in/instance definitions. Checkpoints add rich `work_choices`,
`tableau_work` and per-instance boxes. `tableau_hash` and its dependencies enter
the session hash. JSON's old `people` field remains a **base-population alias**;
explicit `base_people` and `total_people` prevent double counting. The opt-in
scope is labelled `conditional-reviewed-tableau-no-external-modifiers` and
`full_game=false`.

Observations expose both public Building ID lists/boxes, own hand and supported
own choices. Opponent hands, pending targets, future decks and engine effect
logs stay hidden. Replay/JSON are private audit data. No JSON importer or
arbitrary midgame restoration is added.

## Run and verification

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Debug
& ./build-tools/cmake/data/bin/ctest.exe --test-dir build -C Debug --output-on-failure
& ./build/Debug/session_demo.exe --tableau
```

The synthetic scenario builds two own Farms, splits two Restock boxes, Farms,
then reaches Instruction Manual. Only IDs 0 and 1 are mapped as Farms; their
true multiplicity is not claimed. Expected totals **[40,29]**, bases **[30,29]**,
food **[21,12]**, money **[0,8]**, boxes **0:1/1:1**, Progress event **5**, decision
**4**. Python checks each physical-card bag, population components, first PLAY
chain, private pending target and final data across all four demo modes.

Independent `tableau_contracts` covers five actions, enumeration/privacy, target
secrecy, cumulative/new-card triggers, allocation, total population, default
guards, Paperwork, Event stops, overflow and 32-seed schema-4 replay/tampering.
Source, test file and implementation have separate ownership and lead review.
Review found a standalone output-total overflow path; pre-commit validation
and a regression test now protect it. Final local Debug build: **11/11 CTest
cases passed** in 1.61 seconds. Independent review has no remaining P0/P1/P2
findings after that fix; hosted Windows/Linux CI is the PR gate.

## Reproducible microbenchmark

```powershell
& ./build-tools/cmake/data/bin/cmake.exe --build build --config Release --target session_demo
& ./build/Release/session_demo.exe --benchmark-tableau
```

Windows x64, Ryzen 7 3700X, MSVC 19.51, CMake 4.4.4 Release: 1,000 seventeen-
command trace recordings plus verification in **0.31074 seconds**, about
**3,218 traces/sec**, checksum **5000**. Seeds 0..999 are recorded; this scenario
does not shuffle. JSON writing is excluded. This is trace overhead, not full-
game speed, improvement or strategic strength.

Other Buildings, legal catalogs/setup, general effects, Building-aware losses,
ties, unresolved priorities and production shadow continuation remain open.
No opening Research recommendation follows from this conditional subset.

Schema 13 adds the source-reviewed Flag as a second typed Building; see
[M1_FLAG.md](M1_FLAG.md) for its metadata, Hunger exclusion and supported
nonterminal loss behavior. The card catalog remains incomplete.
