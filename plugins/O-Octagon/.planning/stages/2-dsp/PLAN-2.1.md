# Stage 2 — DSP · Phase 2.1 (Geometry Core) — Plan

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.1 of 3 — Geometry Core
**GSD phase:** plan
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `f135b3da`
**Inputs:** `CONTEXT-2.1.md`, `RESEARCH-2.1.md`, `research/ARCHITECTURE.md` §2/§3.1/§3.2/§3.6.6/§4.1/§OQ3/§OQ4, `ROADMAP.md` Phase 2.1, `REQUIREMENTS.md`

---

## Entry Check — contract checksums

Re-run at this phase per C6. All four byte-exact against `STATUS.md` frontmatter:

| Contract | SHA-256 | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…b9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `bff8a83b…06cfe` | ✅ |
| `ROADMAP.md` | `aec7d0ce…7ee29` | ✅ |

No drift. This plan is written against these exact documents.

---

## Goal

Build everything that is **geometry and routing**, with the channel-map test suite standing before a
single gain is computed. Four source components (`VenueModel`, `ConvexHull2D`, `ChannelMap`,
`VenueSnapshot`), the `VENUE` child tree on `apvts.state`, and **two console test targets** plus the
Layer-2 source-parsing generator.

At the end of this phase the plugin still writes the same mono sum to every output — but it writes it
**through the channel map**, the `PHASE-2.2-REPLACE` block is gone, and R1 (the highest-risk
component) has a three-layer test suite standing over it with no gain math in the picture to confuse
a diagnosis.

---

## Requirement staging — read this before writing the verify report

CONTEXT-2.1 lists four requirements for this phase. **Two of them cannot fully close here**, and
saying so now is the direct lesson of Stage 1, where FUNC-01 was discovered mis-staged *at verify*
after the work was done.

| Req | Criterion | Closes at |
|---|---|---|
| **COMPAT-03** | 1. Grep confirms zero hardcoded channel indices in the output path | **2.1** |
| | 2. Unit test asserts the map built by `getChannelIndexForType()` against known JUCE enum-bit order for 7.1 | **2.1** |
| | 3. Test fails loudly if JUCE's enum-bit order changes (parsed source, not a mirrored constant) | **2.1** |
| | → **COMPAT-03 closes complete at 2.1** | |
| **DSP-03** | 1. Hull yields vertices 1,2,4,5,6,7; speakers 3 and 8 `ON_EDGE` | **2.1** |
| | 2. Rear corner classifies outside | **2.1** |
| | 3. Projection matches brute-force nearest-point-on-segment | **2.1** |
| | 4. Degenerate venues finite, non-crashing | **2.1** |
| | → **DSP-03 closes complete at 2.1** | |
| **DSP-04** | 1. `srcZ = 0` height varies linearly `rakeFront`@`bbMinY` → `rakeRear`@`bbMaxY` | **2.1** |
| | 2. `srcZ = 0` at the rear of a steep rake is never below rear-row ear height | **2.1** |
| | 3. *Changing `rakeRear` alone changes the **gain vector** for a rear source* | **2.2 — needs `DbapSolver`** |
| | → **DSP-04 is PARTIAL at 2.1.** Criteria 1–2 are recorded complete; criterion 3 re-maps to stage-2 phase 2.2 | |
| **FUNC-03** | 1. Each of the 8 rows can be assigned any of the 8 channel labels | **2.1** (via the `VENUE` tree; the UI is 3.2) |
| | 2. Duplicate or missing assignment detected and surfaced, never silently routed | **2.1** |
| | 3. *Changing a row moves audio to the corresponding physical output* | **2.2 — Layer 3, needs per-speaker gains** |
| | → **FUNC-03 is PARTIAL at 2.1.** Criterion 3 is unobservable while all 8 lanes carry identical signal | |

**Action for the executor:** do not attempt to close DSP-04/3 or FUNC-03/3 at 2.1, and do not report
them as closed. `REQUIREMENTS.md` is updated at *verify*, marking COMPAT-03 and DSP-03 `complete`,
DSP-04 and FUNC-03 `partial` with the residual criterion named. This is planned, not a shortfall.

---

## Plan Decisions

Continuing Stage 1's P-series.

### P5 — Test target naming

CMake targets `O-Octagon-geometry-test` and `O-Octagon-render-test`, each with an identical
`PRODUCT_NAME`. Both reference the **plugin CMake target `OuariconOctagon`** — never the folder name
`O-Octagon` — for `add_dependencies`, `$<TARGET_PROPERTY:…,INCLUDE_DIRECTORIES>` and the
`JUCE_VERSION` read (`build_script_target_name_vs_folder`; 11 of 37 plugins differ, this is one).

### P6 — The SAFE/REAL branch is a *named helper*, not an inline condition

RESEARCH open item 2. G1 must be stated exactly once in the codebase:

```cpp
// PluginProcessor.h, private
/** True iff the channel map may be used to index this block's buffer.
    BOTH conditions are load-bearing and neither implies the other (RESEARCH-2.1 G1):
      - a VALID map is not evidence of an 8-channel BUFFER — under the F3 hazard the
        LAYOUT reports 7.1 while the buffer holds n < 8, so mapInvalid stays false while
        speakerToBuffer still holds indices up to 7;
      - an 8-channel buffer is not evidence of a valid map.
    Phase 2.2's GainStage inner loop calls THIS, it does not re-derive it. */
bool mappedOutputAvailable (int numOutputChannels) const noexcept;
```

Implementation is one line: `numOutputChannels == 8 && ! mapInvalid.load (std::memory_order_acquire)`.
The argument is the count the caller already read from `buffer.getNumChannels()` — the helper never
calls `getTotalNumOutputChannels()`, which is the accessor that lies (C3, F3).

### P7 — `ChannelMap` is a **pure function**; the processor is the single construction site

D3/Q5 require the unit target to link the geometry TUs **without** `PluginProcessor.cpp`. That is
incompatible with putting the build logic on the processor. Resolution — the logic and the site are
separated without weakening "one construction site":

- `ChannelMap.cpp` holds free functions in namespace `ochan`, with **no processor reference**:
  `buildSpeakerToBuffer (outSet, labelTypes, outMap) -> bool`, `isPermutationOf0to7 (map) -> bool`,
  `verifyEnumBitOrder (set, String* whyNot) -> bool`.
- `OOctagonProcessor::rebuildChannelMap()` remains the **only caller** of `buildSpeakerToBuffer` in
  the plugin, the only writer of `mapInvalid`, and the only publisher of `speakerToBuffer` into the
  snapshot. It is called from `prepareToPlay()` and on a label-map edit — nowhere else.

The safety property in ARCHITECTURE §3.2.3 note 3 is preserved intact: the map is keyed on
`ChannelType`, so `getChannelIndexForType()` remains a genuine lookup. **Do not "simplify" it to a
slot index** (§14 note 1).

### P8 — Layer 1 is one function with two callers

RESEARCH open item 3, answered **yes** — but implemented so it cannot drift. `verifyEnumBitOrder()`
lives in `ChannelMap.cpp` and is called by:

1. `rebuildChannelMap()` under `#if JUCE_DEBUG` → `jassert (ochan::verifyEnumBitOrder (outSet, &why));`
   — a developer who never builds the test target still trips it on the first `prepareToPlay()`.
2. `tests/unit/main.cpp` → a `check()` probe over all three accepted 8-channel sets.

Two fixes fold in from **G2**, and both are mandatory:

- Scan bound `kMaxChannelTypeScan = 256`, not 64. Named `ChannelType` values run to 99 and
  `discreteChannel0 = 128` (`juce_AudioChannelSet.h:402-543`); `channels` is a `BigInteger`.
- **Assert `expected.size() == set.size()`.** This is the part that actually closes it: a truncated
  reconstruction is *still strictly increasing*, so the bound alone fails silently in exactly the
  direction the test exists to catch.

`EXPECT_EQ` in ARCHITECTURE §3.2.5's snippet is GoogleTest pseudocode (**G6**) — transcribe to
`check()`. Do not resolve the symbol by adding a framework.

### P9 — The 200-point hull fixture is **generated in-test from a pinned seed**

RESEARCH open item 4. No committed fixture file: the brute-force oracle lives in the same
translation unit as the assertion, so there is nothing to drift against. `juce::Random rng (0x0C7A9042)`,
200 points sampled uniformly over the speaker bbox expanded ×2 in each axis (so roughly half fall
outside the hull), compared to a naive all-edges nearest-point-on-segment loop at 1e-6.

### P10 — The Layer-2 checksum gate is a `static_assert`, and the SHA covers *parsed data*, not file text

ROADMAP:131 says Layer 2 must **fail the build**. A runtime exit code is a test failure, not a build
failure, so:

```cpp
static_assert (std::string_view (kGeneratedChannelOrderSha256) == kCommittedChannelOrderSha256,
               "JUCE's ChannelType enum values or 8-channel set membership have CHANGED. ...");
```

plus a runtime `check()` printing both hashes, so the diagnostic is readable when it fires.

The generator must emit **deterministically**: the SHA is taken over a canonical serialisation of the
*parsed data* (the referenced `name → value` pairs and the three ordered lists), never over the
emitted file text — otherwise a comment reflow churns the hash and trains everyone to re-bless it
without reading. No timestamps, no absolute paths, no build-directory strings in the output.

### P11 — Hull classification is a first-class `ConvexHull2D` return value

Q4, confirmed. `ConvexHull2D::classify (int speakerIndex) const -> Classification` (`VERTEX` /
`ON_EDGE` / `INTERIOR`). No processor accessor, no test-only hook. The 3.2 Venue screen reads the
same call, so drift is structurally impossible.

### P12 — Two supporting headers

- `Source/DSP/Vec.h` — POD `Vec2 { float x, y; }` and `Vec3 { float x, y, z; }` with
  `static_assert (std::is_trivially_copyable_v<…>)`. A shared root for `VenueModel` and
  `ConvexHull2D` so neither includes the other, and deliberately **not** `juce::Point` — no
  `juce_graphics` in the RT path (§2 ConvexHull2D).
- `Source/Data/VenueSnapshot.h` — the POD struct of §3.6.6 plus the double-buffer publisher.

### P13 — The CI gap is logged as a **repo-level todo**, not an O-Octagon item

RESEARCH open item 5 / Q3. The fix is a secrets-free `push`/`pull_request` workflow configuring
`-DOUARICON_BUILD_TESTS=ON`, which benefits all twelve existing harnesses. It goes to
`.planning/todos/pending/` (the repo's existing convention, `area: tooling`) — **not** into 2.1's
scope, and not quietly dropped. `build-and-release.yml` is not touched by this phase.

---

## Tasks

### Task 1 — Run the Logic 8-channel negotiation check (Stage 1 Task 13) ⚠️ MANUAL, BLOCKING

**This is a human step and the execute phase must not begin without it** (D2). It is the one open
Stage 1 exit criterion.

- Instantiate O-Octagon on a Logic surround track; confirm it appears and instantiates
- Confirm **all 8 surround-meter lanes move**
- Confirm `outputGain` survives save / close / reopen (FUNC-05 slice)
- Confirm the automation menu lists 17 parameters under 5 groups
- **Record which container Logic actually negotiated** — R2 predicts 7.1-SDDS. Observation, not a
  gate: all three are accepted. Feeds COMPAT-02 at Stage 4.

**Do not over-read the meter check.** With the D1 placeholder in place all 8 lanes carry *identical*
signal, so this proves negotiation and writability, not independence. Independence is FUNC-01 and
lands at 2.2.

**Files:** none. **Depends on:** nothing. **Blocks:** every task below.

---

### Task 2 — `Source/DSP/Vec.h`

POD `Vec2`/`Vec3` per P12, with trivially-copyable static asserts and the free helpers the hull needs
(`cross`, `dot`, `sub`, `len2`). Header-only.

Add nothing to `target_sources` (header-only).

**Files:** `Source/DSP/Vec.h` (new)
**Depends on:** Task 1

---

### Task 3 — `Source/Data/VenueModel.{h,cpp}`

The 42-value store and every geometric quantity that does not change per block.

**Schema** (§4.1) — child `VENUE` of `apvts.state` (root `OOctagon`, never renamed):

```
VENUE  @name @savedAt @schemaVersion=1 @rakeFront @rakeRear
└── SPEAKER × 8  { @index, @x, @y, @z, @trimDb, @label }
```

- **Defaults are §OQ4 verbatim.** The graded heights 4.50 → 5.40 m are **load-bearing** — a uniform
  default `z` makes every `(z_i − z_s)` difference identical and hides a dropped `z` term in 2.2's
  DSP-01 test. **Do not flatten them.** `rakeFront = 1.10`, `rakeRear = 3.20`.
- **`readVenueFromState()` treats a missing OR partial node as "use defaults", per attribute** — not
  per node, and never as zeros. Stage 1 sessions carry no `VENUE` child at all; that path must
  produce the §OQ4 default venue silently and without error.
- Labels are stored as the **abbreviated channel-type name string**
  (`getAbbreviatedChannelTypeName`, `.h:550`) and resolved with
  **`AudioChannelSet::getChannelTypeFromAbbreviation`** (`.h:553`) — **G4**, do not hand-roll the
  table §3.2.4 suggests. Unrecognised → `unknown`, which is in no accepted set, so
  `getChannelIndexForType()` returns −1 and the permutation check rejects. Shipped default is the
  identity map of §3.2.4 (speaker 4 → `LFE` is intentional and safe).
- Derived, recomputed on any venue edit: `bbMinX/bbMaxX/bbMinY/bbMaxY`, 3D `centroid`,
  `rigScale = sqrt((1/8) Σ ‖p_i − c‖²)`, and
  `earHeight(y) = rakeFront + (rakeRear − rakeFront)·(y − bbMinY)/(bbMaxY − bbMinY)`.
- **Two zero-span guards, both required:** `bbMaxY − bbMinY < 1e-6` → `earHeight(y) ≡ rakeFront`;
  and the same on the `normToMetres (nx, ny)` bbox denormalisation used to resolve normalised
  `srcX`/`srcY`. Neither may divide by zero or return NaN.
- `absoluteHeight (yMetres, srcZ) = earHeight (yMetres) + srcZ` — the DSP-04 sink at 2.1.
- `trimDb` is **stored** here (part of the 42) and converted to `trimLin` for the snapshot.
  **Application is FUNC-07 at Phase 2.3** — do not apply it anywhere in this phase.
- No `switch` on `ChannelType` anywhere (**G9**, `-Wswitch-enum` at `JUCEHelperTargets.cmake:73`).

Add `Source/Data/VenueModel.cpp` to `target_sources`.

**Files:** `Source/Data/VenueModel.h`, `Source/Data/VenueModel.cpp` (new), `CMakeLists.txt` (modify)
**Depends on:** Task 2

---

### Task 4 — `Source/DSP/ConvexHull2D.{h,cpp}`

Andrew's monotone chain over the (x, y) floor projection, per §3.1 — implement it **as written**.

- **STEP 0 dedup:** two points within `EPS_DEDUP = 1e-4 m` are one point; keep the lowest speaker
  index as representative and record the collapsed set.
- **STEP 2 comparison is `<= EPS_CROSS` — collinear points are POPPED.** This yields strict vertices
  only and is the entire reason speakers 3 and 8 come out `ON_EDGE`.
- `EPS_CROSS = 1e-6 · spanX · spanY` — an **area** tolerance, so it scales with the room.
- CCW output: assert signed area ≥ 0, negate the winding once if the chain came out clockwise.
- `classify (speakerIndex)` per P11 / §3.1.3, with `EPS_ONEDGE = 1e-3 m`.
- Inside test §3.1.4 (audio-thread-shaped: 8 cross products, no allocation, no branching on data).
- Projection §3.1.5 with `EPS_LEN2 = 1e-12`, structurally inside the `if (!inside)` branch (PERF-01
  criterion 3 is satisfied by placement, not by a comment).
- **The full §3.1.6 degeneracy matrix**, with `m < 3` detected explicitly by chain length and routed
  to the segment (`m == 2`) or point (`m == 1`) path. No branch divides by zero; no branch yields NaN.
- Storage shape for the snapshot: `std::array<Vec2, 8> hullPts` + `int hullCount` (§3.1.7).

**The dimensional split is intentional, not a bug** (§3.1.1): the hull is 2D on the floor projection
while DBAP distances are 3D, so a source at `srcZ = +8 m` above room centre is **inside** and gets no
hull attenuation and no air filtering. Do not "fix" this.

Add `Source/DSP/ConvexHull2D.cpp` to `target_sources`.

**Files:** `Source/DSP/ConvexHull2D.h`, `Source/DSP/ConvexHull2D.cpp` (new), `CMakeLists.txt` (modify)
**Depends on:** Task 2

---

### Task 5 — `Source/DSP/ChannelMap.{h,cpp}`

Namespace `ochan`, free functions only, **no processor reference** (P7):

| Function | Contract |
|---|---|
| `buildSpeakerToBuffer (const AudioChannelSet& outSet, const std::array<ChannelType,8>& labels, std::array<int,8>& out)` | §3.2.3 verbatim: requires `outSet.size() == 8`; each label resolved by `getChannelIndexForType`; any −1 fails; then `isPermutationOf0to7`. Returns false on any failure and **leaves `out` untouched** |
| `isPermutationOf0to7 (const std::array<int,8>&)` | The FUNC-03 duplicate/missing detector — a duplicate label repeats a target index, a missing one yields −1, both fail |
| `verifyEnumBitOrder (const AudioChannelSet&, juce::String* whyNot)` | Layer 1 per P8 — scan bound 256, **plus the `expected.size() == set.size()` assertion** |

`speakerToBuffer` is the only thing in the plugin that ever indexes an output channel.

Add `Source/DSP/ChannelMap.cpp` to `target_sources`.

**Files:** `Source/DSP/ChannelMap.h`, `Source/DSP/ChannelMap.cpp` (new), `CMakeLists.txt` (modify)
**Depends on:** Task 2

---

### Task 6 — `Source/Data/VenueSnapshot.h`

§3.6.6 verbatim. POD struct (~250 bytes) carrying `spk[8]`, `trimLin[8]`, `speakerToBuffer[8]`,
`hullPts[8]`, `hullCount`, `centroid`, `rigScale`, the bbox, `rakeFront`, `rakeRear`.

Publisher: **two slots**, `std::atomic<int> activeSlot`, `std::atomic<uint32_t> generation`. Message
thread fills the inactive slot then `store(newIdx, std::memory_order_release)`; audio thread
`load(std::memory_order_acquire)` **once per control block** and holds that index for the whole block.

**Explicitly not `std::atomic<shared_ptr>`** — a refcount decrement (and therefore a `free`) must
never land on the audio thread (`pattern_retired_map_reaper_rt_free`). Do not substitute one.

`static_assert (std::is_trivially_copyable_v<VenueSnapshot>)`.

**Files:** `Source/Data/VenueSnapshot.h` (new)
**Depends on:** Tasks 2, 4

---

### Task 7 — Processor integration

The venue store, the single map construction site, and the state ordering.

- Declare the venue member in the **slot claimed at Stage 1** (`PluginProcessor.h:79-86`, above
  `apvts`) — replace the comment marker with the member. Declaration order is fixed and this is what
  the slot was reserved for (P2).
- `std::atomic<bool> mapInvalid { false }`, the last-valid `std::array<int,8> speakerToBuffer`, the
  `VenueSnapshot` double-buffer, the `ConvexHull2D`.
- `rebuildChannelMap()` — the **single construction site**. Calls `ochan::buildSpeakerToBuffer`; on
  success publishes and clears `mapInvalid`; on failure **retains the last valid map** and sets
  `mapInvalid`. Never silently routes. Under `#if JUCE_DEBUG`, the Layer-1 `jassert` of P8.
- `readVenueFromState()` — reads the `VENUE` child (defaults per Task 3), recomputes derived
  quantities, rebuilds the hull, publishes a new snapshot.
- `prepareToPlay()` → `readVenueFromState()` then `rebuildChannelMap()`.
- `setStateInformation()` **ordering hazard** (§4.1): `replaceState()` → `readVenueFromState()` →
  `rebuildChannelMap()`, in that order. If `prepareToPlay()` has not yet run, **defer the map
  rebuild** rather than adding a second construction site. If anything here is deferred via
  `AsyncUpdater`, `cancelPendingUpdate()` **must** be called in the restore path
  (`pattern_asyncupdater_guard_flag_needs_cancel`) — the Stage 1 header comment notes the processor
  is deliberately not an `AsyncUpdater` today, so prefer a direct call and a `bool preparedYet` flag.
- `getStateInformation()` is **unchanged** — the `VENUE` child rides along inside `copyState()`. That
  is why it was written at Stage 1.
- The `mappedOutputAvailable (int)` helper of **P6**.

**Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp` (modify)
**Depends on:** Tasks 3, 4, 5, 6

---

### Task 8 — `processBlock` rewrite + retire `PHASE-2.2-REPLACE` + guard `createEditor`

**Retire, do not grandfather** (C2). Delete the block at `PluginProcessor.cpp:188-223` and replace
with the Q1 shape:

```
const int numOut = buffer.getNumChannels();          // never 8, never getTotalNumOutputChannels()
if (mappedOutputAvailable (numOut))   → REAL: write via speakerToBuffer[i]
else                                  → SAFE: for (ch = 0; ch < numOut; ++ch)
```

- The sample-interleaved **read-before-write** sum at the old `:209-223` is **preserved as written** —
  `out[0]` and `in[0]` alias the same memory, and the channel map does not change that reasoning.
- All 8 lanes still carry identical signal. That is correct at 2.1; independence is FUNC-01 at 2.2.
- Neither path contains a literal channel index, so the COMPAT-03 grep gate passes.
- Also in this task, two lines (**G8**): guard `createEditor()` with `#if JUCE_WEB_BROWSER`, returning
  `GenericAudioProcessorEditor` in both arms today. It is provably inert now and becomes a build break
  in a target nobody is looking at once 3.1 swaps in the WebView
  (`pattern_render_harness_breaks_on_webview_editor`).

**Files:** `Source/PluginProcessor.cpp` (modify)
**Depends on:** Task 7

---

### Task 9 — `tests/tools/gen_juce_channel_order.py`

Layer 2's generator. Reads JUCE source, emits `JuceChannelOrderGolden.h` + a SHA-256.

1. Parse the `enum ChannelType` block in `juce_AudioChannelSet.h` into `name → int`. **G3, both
   traps:** extract `name = <integer>` pairs — **never assign by line position** (`topSideLeft = 28`
   and `topSideRight = 29` are declared *before* `ambisonicACN0..3 = 24..27`, and those are precisely
   the two types `create5point1point2()` uses) — and handle identifier-valued entries such as
   `surround = centreSurround` by resolving the RHS or skipping, never by mis-parsing.
2. Parse the `create7point1()` / `create7point1SDDS()` / `create5point1point2()` initializer lists in
   `juce_AudioChannelSet.cpp` (`:567`, `:568`, `:574` — one line each, no continuations).
3. **Assert every name referenced by an initializer list resolved.** A silently-missing name drops a
   channel from the golden, and the golden's whole job is to be independent of the implementation.
4. Sort each list by parsed enum value; emit the three derived orders.
5. SHA-256 over the **canonical serialisation of the parsed data**, not the file text (P10).
6. **Exit non-zero on any parse failure and never emit a header with zero entries.** A vacuous golden
   is worse than no golden — it reports green.

Invoked from CMake with `--juce-modules ${JUCE_MODULES_DIR}` (**Q2**; `JUCE/CMakeLists.txt:41`,
`CACHE INTERNAL`, resolved before the plugin glob loop, and portable across local/CI because the root
resolution is). Never a path literal.

**Files:** `tests/tools/gen_juce_channel_order.py` (new)
**Depends on:** Task 1

---

### Task 10 — `tests/unit/` → `O-Octagon-geometry-test`

`juce_add_console_app` per **Q5** — no Catch2, no GoogleTest, no doctest, no `juce::UnitTest`. The
`int failures` + `check(name, ok, detail)` + `return failures == 0 ? 0 : 1` idiom of
`O-ReverseDelay/tests/render-harness/main.cpp:808-824`. **The Catch2 references in
`docs/codebase/TESTING.md` describe an intent never implemented — do not follow them.**

Links **only** `VenueModel.cpp`, `ConvexHull2D.cpp`, `ChannelMap.cpp`. Not `PluginProcessor.cpp`, so
it needs no `JucePlugin_*` block, compiles fast, and cannot be broken by the Stage-3 WebView swap.

CMake also carries the Layer-2 codegen:

```cmake
find_package (Python3 COMPONENTS Interpreter REQUIRED)
if (NOT JUCE_MODULES_DIR)
    message (FATAL_ERROR "JUCE_MODULES_DIR is empty — Layer 2's golden would be generated from "
                         "nothing and the COMPAT-03 gate would pass vacuously.")
endif()
add_custom_command (OUTPUT .../JuceChannelOrderGolden.h
                    COMMAND ${Python3_EXECUTABLE} .../gen_juce_channel_order.py ...
                    DEPENDS the script AND the two JUCE source files)
```

**Probes:**

| # | Probe | Source |
|---|---|---|
| A | Layer 1 `verifyEnumBitOrder` over all three accepted 8-channel sets | §3.2.5, G2 |
| B | Layer 2 — generated golden vs runtime order for all three sets | ROADMAP:131 |
| C | Layer 2 — `static_assert` SHA match (build-time) + runtime hash print | P10 |
| D | **Non-identity map, same container** — a rotated label set under `create7point1()` yields a known non-identity `speakerToBuffer`. **A hardcoded 0..7 map fails this immediately** | **C1 / G5** |
| E | **Cross-container** — 7.1 labels against `create7point1SDDS()`: 4 of 8 types absent → build fails, `mapInvalid` set. The ROADMAP:131 missing-label test, from real JUCE sets | **G5** |
| F | Duplicate label → map rejected, last valid map retained | FUNC-03 |
| G | Corrupt numeric label `"7"` → `getChannelTypeFromAbbreviation` returns a *plausible* `discreteChannel` (`.cpp:283-285`, no range check) and the map is rejected **by the permutation check**, not by a parse error | **G4** |
| H | Hull of the §OQ4 default venue = vertices **1, 2, 4, 5, 6, 7**; speakers **3 and 8 `ON_EDGE`** | DSP-03/1 |
| I | Near-collinear (not exactly collinear) point set — the epsilon path. *Probe H's walls are dead straight, so its cross products are exactly zero and it does not exercise `EPS_CROSS` at all* | DSP-03/1 |
| J | Rear room corner (13.0, 22.0) classifies **outside** | DSP-03/2 |
| K | 200-point projection vs brute-force nearest-point-on-segment to 1e-6, pinned seed | DSP-03/3, P9 |
| L | Degeneracy matrix: all 8 collinear (m=2), all 8 coincident (m=1), zero rake span, degenerate bbox → all finite, no NaN, no crash | DSP-03/4, QUAL-02 |
| M | `earHeight` linear `rakeFront`@`bbMinY` → `rakeRear`@`bbMaxY`; changing `rakeRear` alone changes a rear source's absolute height; zero-span guard returns `rakeFront` | DSP-04/1 |
| N | `srcZ = 0` at the rear of a steep rake is never below rear-row ear height | DSP-04/2 |
| O | `rigScale` ≈ 7.95 m for the §OQ4 defaults **and** the scaling invariant: doubling every coordinate doubles `rigScale`. *The invariant is the real assertion — a bare constant is a mirrored fixture* | §OQ4, `pattern_test_fixture_mirrors_drift_silently` |
| P | Missing `VENUE` node and partially-populated `VENUE` node both yield the §OQ4 defaults, per attribute | Stage 1 carry |

**Files:** `tests/unit/CMakeLists.txt`, `tests/unit/main.cpp` (new)
**Depends on:** Tasks 3, 4, 5, 9

---

### Task 11 — `tests/render-harness/` → `O-Octagon-render-test`

Pulled forward from 2.2 by **D3**. Template: `O-ReverseDelay/tests/render-harness/CMakeLists.txt`.

- Links `PluginProcessor.cpp`; `JUCE_WEB_BROWSER=0`, `JUCE_STANDALONE_APPLICATION=1`,
  `JucePlugin_Build_Standalone=1`, plus the minimal `JucePlugin_*` block.
- **Version DERIVED, never mirrored (G7):** `get_target_property (_VER OuariconOctagon JUCE_VERSION)`
  with a `FATAL_ERROR` if empty, and the `(major<<16)+(minor<<8)+patch` code computed from it. The
  sibling documents this literal drifting **twice** across five releases — a "keep in sync" comment
  is not a mechanism. O-Octagon has no factory presets yet, which is exactly why it costs nothing to
  do correctly now.

**Probes:**

| # | Probe | Closes |
|---|---|---|
| Q | **Unity gain** through all 8 outputs at `create7point1()` — measured, not inspected | **C4 / Stage 1 issue 3** |
| R | Bus layouts **1, 2 and 8 output channels** constructed programmatically; each renders finite, non-crashing, correct SAFE/REAL selection. *No hardware needed* | **C5 / Stage 1 issue 4** |
| S | **The F3 hazard, directly:** set the layout to `create7point1()`, then hand `processBlock` an `AudioBuffer` of 3..7 channels. `getTotalNumOutputChannels()` returns 8 while the buffer does not — assert no out-of-bounds write and no crash. This is the state G1 describes, now a *tested* path rather than a reasoned one | **G1 / F3** |
| T | Session round-trip: state with a non-default `VENUE` (edited speaker coords + a permuted label map) survives `getStateInformation` → `setStateInformation` bit-exactly | FUNC-02 slice |
| U | A Stage-1-shaped session (no `VENUE` child at all) restores to the §OQ4 defaults without error | Stage 1 carry |

**Files:** `tests/render-harness/CMakeLists.txt`, `tests/render-harness/main.cpp` (new)
**Depends on:** Tasks 7, 8

---

### Task 12 — Wire both test targets into the plugin `CMakeLists.txt`

Repo convention, twelve times over:

```cmake
option (OUARICON_BUILD_TESTS "Build O-Octagon test targets" OFF)
if (OUARICON_BUILD_TESTS)
    add_subdirectory (tests/unit)
    add_subdirectory (tests/render-harness)
endif()
```

One option gates both. **Do not touch `.github/workflows/build-and-release.yml`** (Q3) — it is
tag-triggered, secrets-bearing, and carries a standing rule against widening its trigger surface.

**Files:** `plugins/O-Octagon/CMakeLists.txt` (modify)
**Depends on:** Tasks 10, 11

---

### Task 13 — Gates

Run every one; record actual output, do not read results out of a prior document.

1. **Clean 3-format build** (VST3 + AU + Standalone), forced TU recompile — **zero warnings, zero
   errors** in the whole log
2. **`grep -rn` for hardcoded output channel indices outside `ChannelMap`** — expected **zero**. A
   loop variable is not a hardcoded index; a literal `8` or a constant slot number is
3. **`grep -c "PHASE-2.2-REPLACE"` → 0** (Stage 1's uniqueness gate inverts here: retired, not present)
4. `auval -a | grep -i octagon`, then `auval -v aufx OuOc OuDv` → **AU VALIDATION SUCCEEDED**
5. **pluginval strictness 10, VST3 and AU, ×3 each** (`pattern_ci_pluginval10_catches_latent_nan`)
6. Configure with `-DOUARICON_BUILD_TESTS=ON`; build and run **both** test targets → **exit 0**
7. Confirm the 17 parameters are unchanged against `parameter-spec.md` — 2.1 adds none
8. Confirm `setLatencySamples` still appears nowhere

**Files:** none (verification)
**Depends on:** Task 12

---

### Task 14 — Log the CI gap; write `SUMMARY-2.1.md`

- `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md`, `area: tooling` — a
  push/PR workflow configuring `-DOUARICON_BUILD_TESTS=ON`, benefiting all twelve existing harnesses.
  State the residual gap plainly: **a JUCE bump performed without running the test target ships
  silently.** Not closed at 2.1 (P13).
- `stages/2-dsp/SUMMARY-2.1.md` and `STATUS.md`.

**Files:** `.planning/todos/pending/…md`, `stages/2-dsp/SUMMARY-2.1.md`, `.planning/STATUS.md` (new/modify)
**Depends on:** Task 13

---

## Execution Constraints

- **DO NOT execute this phase in an isolated worktree.** `stages/2-dsp/` is untracked; the scope
  would vanish and every gate would pass vacuously
  (`pattern_worktree_isolation_wrong_for_untracked_scope`).
- Plugin CMake target is **`OuariconOctagon`**; the folder is `O-Octagon`. Both test CMakeLists use
  the target name.
- Read `parameter-spec.md`, **never** `parameter-spec-draft.md` (banner-marked superseded).
- No `switch` on `AudioChannelSet::ChannelType` anywhere (`-Wswitch-enum`).
- No `setLatencySamples()`.
- Clear the AU cache and sweep **both** `-dev` and unsuffixed bundles before any install
  (`./scripts/build-and-install.sh O-Octagon` does this in Phase 4).

## Non-goals for Phase 2.1 — must not appear

`DbapSolver`, `GainStage`, the 64-sample control grid, `SmoothedValue` targets, `SourceShaper`,
`HullProcessor`, `VerifyPing`, any WebView editor, any `juce_add_binary_data` target, any change to
`build-and-release.yml`, any FUNC-07 trim *application*, any new APVTS parameter.

---

## Success Criteria

**ROADMAP Phase 2.1 test criteria — all ten:**

- [ ] Channel-map **Layer 1** — reconstructed order strictly increasing and matching
      `getTypeOfChannel(i)` for all i, **with the size assertion** (G2)
- [ ] Channel-map **Layer 2** — golden generated from parsed JUCE source at build time; committed
      SHA compared; **build fails** on an enum-value or membership change
- [ ] Duplicate label assignment → map rejected, `mapInvalid` set, last valid map retained
- [ ] Missing label assignment → same
- [ ] Hull of the traced layout yields exactly vertices **1, 2, 4, 5, 6, 7**, with 3 and 8 `ON_EDGE`
- [ ] A point at a physical rear corner classifies as **outside**
- [ ] Hull projection matches brute-force nearest-point-on-segment to 1e-6 over 200 points
- [ ] Degenerate venues (8 collinear, 8 coincident, zero rake span) → finite, no crash
- [ ] `srcZ = 0` height varies linearly `rakeFront`@`bbMinY` → `rakeRear`@`bbMaxY`; changing
      `rakeRear` alone changes a rear source's absolute height
- [ ] `grep -rn` confirms **zero hardcoded output channel indices** outside `ChannelMap`

**Added by this plan:**

- [ ] **Every channel-map probe drives a non-identity label map** (C1/G5) — the default identity map
      alone is vacuous under all three accepted containers
- [ ] The `PHASE-2.2-REPLACE` block is **gone**, not commented out
- [ ] The F3 hazard is **measured** (probe S), not reasoned (C5)
- [ ] Unity gain through all 8 outputs is **measured** (probe Q), not inspected (C4)
- [ ] Both test targets exit 0; the plugin builds clean in 3 formats with zero warnings
- [ ] pluginval s10 ×3 on VST3 and AU; `auval` SUCCEEDED
- [ ] Contract checksums re-verified at 2.1 verify (C6)
- [ ] CI gap logged as a repo-level todo (P13)

**Requirement outcomes expected at verify:** COMPAT-03 ✅ complete · DSP-03 ✅ complete ·
DSP-04 ⚠️ partial (criterion 3 → 2.2) · FUNC-03 ⚠️ partial (criterion 3 → 2.2).

---

## Risks Active in This Phase

| Risk | Severity | Mitigation in this plan |
|---|---|---|
| **R1 — speaker→buffer channel map** | CRITICAL | Front-loaded by design. Layers 1 and 2 land here (probes A–G); Layer 3 at 2.2. C1 enforced: probes D and E drive non-identity maps, so a hardcoded 0..7 fails immediately |
| **G1 — valid map ≠ 8-channel buffer** | HIGH | P6's named helper states it once; probe S measures it. Carries verbatim into 2.2's `GainStage` |
| **G2 — Layer 1 truncation is silent** | HIGH | Scan bound 256 **and** `expected.size() == set.size()` |
| **G3 — enum parse by line position** | HIGH | Parse `name = <int>` pairs; assert every referenced name resolved; exit non-zero otherwise |
| R4 — convex hull degeneracy | MEDIUM | The §3.1.6 matrix is probe L; probe I covers the epsilon path probe H does not |
| Vacuous Layer 2 in CI | MEDIUM | `FATAL_ERROR` on empty `JUCE_MODULES_DIR`; generator exits non-zero rather than emitting an empty golden; residual gap logged (P13) |

---

## Next Phase

**Ready for:** execute phase — Phase 2.1 (Geometry Core), **after Task 1 (Logic) is done**.

Writes `stages/2-dsp/SUMMARY-2.1.md`. The stage-level `VERIFICATION.md` is written only at the close
of 2.3.
