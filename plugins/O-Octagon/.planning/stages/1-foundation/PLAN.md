# Stage 1 — Foundation: PLAN

**Plugin:** O-Octagon
**Stage:** 1 of 4 — Foundation + Shell
**Phase:** plan ✓
**Date:** 2026-08-11
**Branch:** `feat/o-octagon`
**Inputs:** `CONTEXT.md` (D1–D4), `RESEARCH.md` (F1–F8, §9), `research/ARCHITECTURE.md` §4.1/§4.2/§6.1/§12, `ROADMAP.md` Stage 1

---

## Goal

Produce a **loadable, validating 8-channel shell**: a `OuariconOctagon` CMake target, the bus
declaration and negotiation predicate from ARCHITECTURE §4.2, all 17 APVTS parameters from §6.1,
and the session-state round-trip from §4.1 — written once, here, and not revisited.

**No DBAP.** No `ChannelMap`, no `VENUE` tree, no smoothers, no control grid, no WebView. The shell
exists to prove the 8-channel transport before any geometry depends on it.

Success is measured at the host boundary, not in the source: Logic/`auval`/pluginval must all
negotiate an 8-channel container and expose 17 correctly-labelled parameters.

---

## Decisions taken at plan phase

RESEARCH §9 handed four open items to this phase. All four are resolved here; none reopens a
locked contract.

### P1 — Parameter grouping: **five `AudioProcessorParameterGroup`s**

Taking RESEARCH §3.4's recommendation.

| Group ID | Group name | Members |
|---|---|---|
| `position` | Position | `srcX` `srcY` `srcZ` `width` |
| `solve` | Solve | `rolloff` `blur` |
| `weights` | Weights | `w1`..`w8` |
| `space` | Space | `hullAtten` `airAmount` |
| `output` | Output | `outputGain` |

Separator `"|"`, matching `O-Orbit/Source/PluginProcessor.cpp:62`. The headline gesture of this
plugin is automating eight weights; a flat 17-entry menu buries them. Group membership does not
participate in parameter identity, so this stays reversible if Stage 4 host testing disagrees.

### P2 — Member declaration order: **reserve the venue slot above `apvts`**

`PluginProcessor.h` declares, in this order:

```
1. (reserved) venue store — lands at Phase 2.1, MUST precede apvts
2. juce::AudioProcessorValueTreeState apvts;
3. cached raw parameter pointers
```

At Stage 1 slot 1 is a **comment marker only** — there is no member to declare yet. The point is
that the position is claimed now, because member declaration order is fixed at Stage 1 and is
annoying to change once Phase 2.1 depends on it (RESEARCH §3.3). Costs one comment.

### P3 — Placeholder marker token: `// PHASE-2.2-REPLACE:`

D1's dry-to-all-outputs loop is a deliberate raw-buffer-index write that Phase 2.1's gate
(*"zero hardcoded output channel indices outside `ChannelMap`"*) is **expected to fail against**.
Marking it with a greppable token is what makes that gate retire the loop rather than grandfather
it. The token appears on the loop and nowhere else in the plugin.

### P4 — `parameter-spec.md` is promoted, and is what the executor reads

Task 1. The draft carries OQ3/4/5 and the 17-vs-18 count as *open* although all four were resolved
at Stage 0; it is stale in exactly the places Stage 1 reads from.

---

## Tasks

### Task 1 — Promote `parameter-spec.md` from the draft

- **Files:** `plugins/O-Octagon/.planning/parameter-spec.md` (new)
- **Depends on:** none
- **Source of truth:** ARCHITECTURE §6.1 (ranges, defaults, skews) + RESEARCH §3.2 (labels, display precision)

Write the final spec: 17 parameters, all `AudioParameterFloat`, all skews linear, with the
`withLabel` and display-precision columns that §6.1 gives only in prose. Carry P1's grouping.
State explicitly that OQ3/4/5 and the count are **resolved** (17), and that the venue is a separate
`ValueTree` (§4.1) — the three places the draft is stale.

`parameter-spec-draft.md` stays on disk as a historical artifact. Add a one-line banner to the top
of the draft pointing at the promoted file so nothing reads it by accident.

**The executor reads `parameter-spec.md`, never the draft.**

---

### Task 2 — `plugins/O-Octagon/CMakeLists.txt`

- **Files:** `plugins/O-Octagon/CMakeLists.txt` (new)
- **Depends on:** none

Per ARCHITECTURE §12, with RESEARCH §5's three corrections applied:

```cmake
juce_add_plugin(OuariconOctagon
    COMPANY_NAME             "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE              OuOc
    FORMATS                  VST3 AU Standalone
    PRODUCT_NAME             "O-Octagon${OUARICON_DEV_SUFFIX}"
    VERSION                  1.0.0
    NEEDS_WEB_BROWSER        TRUE
    NEEDS_WEBVIEW2           TRUE
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    IS_MIDI_EFFECT           FALSE
    VST3_CATEGORIES          "Fx" "Spatial"
    AU_MAIN_TYPE             "kAudioUnitType_Effect"
    # DO NOT set PLUGIN_CHANNEL_CONFIGURATIONS — counts only, not types.
    # JUCE derives the AU config set from isBusesLayoutSupported() (RESEARCH F2).
)
```

Then: `target_sources` (2 files), `target_include_directories(PRIVATE Source)`,
`target_link_libraries` (the O-Orbit module list **minus `saf`, minus the `_WebUI` binary-data
target**, plus `juce::juce_dsp`), `juce_generate_juce_header(OuariconOctagon)`,
`target_compile_definitions`.

**Hard constraints, each with a named failure mode:**

| Constraint | Why |
|---|---|
| `VERSION`, never `PLUGIN_VERSION` | `PLUGIN_VERSION` is not a JUCE keyword — silently ignored, ships 1.0.0 regardless (`critical_plugin_version_keyword_ignored_by_juce`) |
| No `PLUGIN_CHANNEL_CONFIGURATIONS` | Counts only, not types; breaks surround detection. Also *redundant* — RESEARCH F2 |
| No SAF, no `add_subdirectory(libs/SAF/...)` | Unlike sibling O-Orbit. Copying its CMakeLists wholesale imports SAF |
| No `juce_add_binary_data` target | Phase 3.1. Neither hyphen-stripping nor namespace collision applies yet |
| `VST3_CATEGORIES "Fx" "Spatial"` in that order | JUCE hoists `Fx` to index 0 regardless (F6); writing it first makes the source match what ships |
| `juce_generate_juce_header()` **after** `target_link_libraries`, **before** `target_compile_definitions` | juce8-critical-patterns §1 |
| `JUCE_WEB_BROWSER=1`, `JUCE_USE_CURL=0`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` | Set now though unused — having `JUCE_WEB_BROWSER` defined from the start is what lets the Phase 2.2 harness guard `createEditor` *before* the Stage 3 swap (`pattern_render_harness_breaks_on_webview_editor`) |

**No root `CMakeLists.txt` edit** — `:48-58` globs `plugins/*` (RESEARCH §5, verified).

---

### Task 3 — `PluginProcessor.h` — class declaration and member order

- **Files:** `plugins/O-Octagon/Source/PluginProcessor.h` (new)
- **Depends on:** Task 1

Class `OOctagonProcessor : public juce::AudioProcessor`. **No `AsyncUpdater` base** — there is
nothing to defer yet, and adding one now means Phase 2.1 inherits a `cancelPendingUpdate()`
obligation it did not ask for (`pattern_asyncupdater_guard_flag_needs_cancel`).

Standard override set. `static juce::AudioProcessorValueTreeState::ParameterLayout
createParameterLayout();`

Private members, in this order (P2):

```cpp
// ─────────────────────────────────────────────────────────────────────
// VENUE STORE SLOT — Phase 2.1 declares the venue member HERE, above
// apvts. Declaration order is fixed at Stage 1; a venue-aware parameter
// lambda would need the member to already exist. See RESEARCH §3.3.
// ─────────────────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState apvts;

std::atomic<float>* srcXParam { nullptr };   // …17 cached pointers
```

Cached raw parameter pointers for all 17 (`getRawParameterValue`), matching the O-Orbit idiom.
Stage 1 does not read them in `processBlock`, but they are the Phase 2.2 snapshot source and
caching them here is what makes the constructor complete.

`JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OOctagonProcessor)`.

**Explicitly absent:** `ChannelMap`, `VenueSnapshot`, `SmoothedValue`, `FirstOrderTPTFilter`,
`absoluteSampleCounter`, any `Source/DSP/` or `Source/Data/` include.

---

### Task 4 — Constructor, `BusesProperties`, and `createParameterLayout()`

- **Files:** `plugins/O-Octagon/Source/PluginProcessor.cpp`
- **Depends on:** Tasks 1, 3

**Constructor member-initialiser list** — `BusesProperties` here, never in `prepareToPlay()`
(juce8-critical-patterns §4):

```cpp
OOctagonProcessor::OOctagonProcessor()
  : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::mono(),          true)
        .withOutput ("Output", juce::AudioChannelSet::create7point1(), true)),
    apvts (*this, nullptr, juce::Identifier ("OOctagon"), createParameterLayout())
{ /* cache the 17 raw parameter pointers */ }
```

**State root is `OOctagon`** — the architecture's identifier, *not* the sibling's `OOrbitParams`
idiom. Phase 2.1 attaches the `VENUE` child to this exact node; changing it later orphans every
saved session and every `.venue` file written in between. This identifier must never change.

`createParameterLayout()`: 17 `AudioParameterFloat` in five groups (P1), ranges/defaults per
`parameter-spec.md`.

Two construction rules:

1. **`juce::ParameterID { "srcX", 1 }`** — the version hint is mandatory in JUCE 8.
2. **Two-argument `NormalisableRange<float>(min, max)`** — gives interval 0, skew 1. Do **not**
   write the 4-argument form with an explicit `1.0f` skew; it reads as though a skew were intended
   and invites a future "fix".

Labels via `AudioParameterFloatAttributes().withLabel(...)` per `parameter-spec.md` §units.
`srcX`/`srcY`/`blur`/`airAmount`/`w1..w8` take **no** label — deliberate; inventing "norm" or "%"
would misrepresent the weights.

**`srcX`/`srcY` host display is normalised 0.00–1.00.** Metres are a Stage 3.1 UI-side conversion.
A host value→text lambda is captured at construction by definition and therefore cannot read a
live venue; §6.1's metres mandate is about the *readout*, which is a UI concern
(`pattern_webview_knob_readout_scaled_value`). Do not attempt a `this`-capturing lambda here.

---

### Task 5 — `isBusesLayoutSupported()`

- **Files:** `plugins/O-Octagon/Source/PluginProcessor.cpp`
- **Depends on:** Task 3

Verbatim from ARCHITECTURE §4.2: reject disabled buses; inputs mono/stereo only; accept
`create7point1()`, `create7point1SDDS()`, `create5point1point2()` (real mode) and
`mono()`/`stereo()` (SAFE mode); reject everything else — including `octagonal()`, which JUCE
offers as an 8-channel candidate and which Logic ignores.

**This function is load-bearing beyond its four lines.** JUCE derives the entire AU channel-config
set from it (RESEARCH F2): `AUChannelInfo = {(1,1),(1,2),(1,8),(2,1),(2,2),(2,8)}`. Widening or
narrowing the predicate silently changes what `auval` tests and what Logic offers.

`const` and `override`. Do not name unused parameters.

---

### Task 6 — `prepareToPlay`, `releaseResources`, `processBlock` placeholder

- **Files:** `plugins/O-Octagon/Source/PluginProcessor.cpp`
- **Depends on:** Tasks 3, 5

`prepareToPlay(double, int)` — nothing to prepare at Stage 1. Omit the parameter *names* (not
`(void)` casts, not `ignoreUnused` if the names can simply be dropped) to satisfy
`-Wunused-parameter`. **Never call `setLatencySamples()`** — latency is zero and
`getLatencySamples()` is non-virtual in JUCE 8.

`releaseResources()` — empty.

`processBlock` — D1's placeholder, and the one place in Stage 1 where the exact shape matters:

```cpp
void OOctagonProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // ── PHASE-2.2-REPLACE: ────────────────────────────────────────────────
    // Stage 1 placeholder. Writes the mono-summed dry input to EVERY output
    // channel at unity, by RAW BUFFER INDEX, so that a successful 8-channel
    // negotiation is visible on Logic's surround meters (FUNC-01/COMPAT-01).
    // This is the ONLY hardcoded output index in the plugin and it is deleted
    // wholesale by the GainStage inner loop at Phase 2.2. It must acquire no
    // dependants. Phase 2.1's "zero hardcoded output indices outside
    // ChannelMap" gate is EXPECTED to fail against this block — retire it,
    // do not grandfather it.
    // ──────────────────────────────────────────────────────────────────────

    const int numSamples = buffer.getNumSamples();
    const int numOut     = buffer.getNumChannels();                 // NOT 8, NOT getTotalNumOutputChannels()
    const int numIn      = juce::jmin (getTotalNumInputChannels(), numOut);

    for (int n = 0; n < numSamples; ++n)
    {
        // Read BEFORE writing: out[0] and in[0] alias the same pointer.
        const float s = numIn >= 2 ? 0.5f * (buffer.getSample (0, n) + buffer.getSample (1, n))
                      : numIn == 1 ? buffer.getSample (0, n)
                                   : 0.0f;

        for (int ch = 0; ch < numOut; ++ch)
            buffer.setSample (ch, n, s);
    }
}
```

Four properties, each of which is a defect if broken:

1. **Bound by `buffer.getNumChannels()`.** RESEARCH F3: on a 3–7 output device
   `canonicalChannelSet(n)` is rejected, Debug asserts, and **Release keeps the 7.1 layout while
   the buffer has only *n* channels**. `getTotalNumOutputChannels()` returns 8 in exactly that
   state — it is the accessor that lies. A loop bounded by 8 writes past the end of the buffer.
2. **Sample-interleaved read-then-write, no scratch buffer.** `out[0]` and `in[0]` are the same
   memory. Writing all 8 outputs channel-major would destroy `in[1]` before it is summed. A member
   scratch buffer would also work but is an unnecessary member for a block that gets deleted.
3. **Correct at 1 and 2 output channels, not only 8.** `auval` exercises all six AU configs from
   F2, including 1-in/1-out.
4. **`numIn == 0` yields silence, not a read of channel 0.** Defensive; costs one branch.

**No `MidiBuffer` use** — the parameter is unnamed.

---

### Task 7 — Editor hooks, boilerplate overrides, plugin entry point

- **Files:** `plugins/O-Octagon/Source/PluginProcessor.cpp`
- **Depends on:** Task 3

`hasEditor()` → `true`. `createEditor()` → `return new juce::GenericAudioProcessorEditor (*this);`

**No `PluginEditor.{h,cpp}` at Stage 1.** The generic editor renders all 17 parameters with their
names, ranges, defaults and units — which *is* the Stage 1 exit criterion — and gives the
Standalone build a non-empty window for the COMPAT-04 eyeball check. It is five lines and it is
deleted at Phase 3.1. Nothing may come to depend on it.

Remaining overrides: `getName()` → `"O-Octagon"`; `acceptsMidi/producesMidi/isMidiEffect` → false;
`getTailLengthSeconds()` → `0.0`; the four program stubs (`getNumPrograms()` → 1).

`createPluginFilter()` at the bottom of the file.

AGPL/SPDX header on both new source files — run `python3 scripts/add-agpl-headers.py --apply`
(idempotent) after the files exist, or write them by hand from any `plugins/*/Source/PluginProcessor.h`.

---

### Task 8 — `getStateInformation` / `setStateInformation`

- **Files:** `plugins/O-Octagon/Source/PluginProcessor.cpp`
- **Depends on:** Tasks 3, 4

ARCHITECTURE §4.1 verbatim: `apvts.copyState()` → `createXml()` → `copyXmlToBinary`; restore via
`getXmlFromBinary`, tag-name check against `apvts.state.getType()`, `apvts.replaceState()`.

**This code is written once and does not change when Phase 2.1 adds the `VENUE` child** — the
child rides along inside `copyState()` automatically. That is the whole reason it is written now.

Two forward-compatibility notes to carry as comments, so Phase 2.1 does not have to rediscover them:

- Stage 1 sessions carry **no `VENUE` child**. Every session saved between now and Phase 2.1
  restores a tree with 17 parameters and nothing else. Phase 2.1's `readVenueFromState()` must
  therefore treat a missing *or partial* `VENUE` node as "use defaults" — not as an error — and
  must not assume 8 `SPEAKER` children exist.
- Phase 2.1 calls `readVenueFromState()` **after** `replaceState()`, and `rebuildChannelMap()`
  after both.

No `AsyncUpdater`, no deferral, no guard flag at this stage.

---

### Task 9 — Build clean: VST3 + AU + Standalone, zero warnings

- **Files:** none (verification)
- **Depends on:** Tasks 2, 4, 5, 6, 7, 8

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # re-configure picks up the new plugin dir
cmake --build build --target OuariconOctagon_VST3 OuariconOctagon_AU OuariconOctagon_Standalone 2>&1 | tee /tmp/octagon-build.log
grep -iE "warning|error" /tmp/octagon-build.log            # must be empty for our sources
```

`juce_recommended_warning_flags` is on. Four shapes are pre-banned (RESEARCH §7) and each has a
concrete site in this stage:

| Flag | Site in Stage 1 |
|---|---|
| `-Wunused-parameter` | `prepareToPlay(double, int)`, `processBlock`'s `MidiBuffer&`, `isBusesLayoutSupported` if it early-returns |
| `-Wshadow-all` | a parameter named `width` shadows nothing yet — but watch it if a helper is added |
| `-Wconversion` / `-Wsign-conversion` | channel/sample loop counters are `int`, matching `getNumChannels()`/`getNumSamples()`. Do not introduce `size_t` |
| `-Wswitch-enum` | not triggered at Stage 1 — but **never `switch` on `AudioChannelSet::ChannelType`** (~60 enumerators, warns even with `default:`). Phase 2.1's label map must be a table or `if`-chain |

Zero warnings **from O-Octagon's own translation units** is the bar; pre-existing JUCE warnings are
not in scope.

---

### Task 10 — Install and confirm the AU registers

- **Files:** none (verification)
- **Depends on:** Task 9

```bash
./scripts/build-and-install.sh O-Octagon        # folder name; resolves target OuariconOctagon
auval -a | grep -i octagon
```

The script's Phase 4 sweeps both `-dev` and unsuffixed bundles
(`critical_dev_release_variant_shadowing`) and clears the AU cache. Expect
`aufx OuOc OuDv` under dev branding.

If `auval -a` does not list it: `killall -9 AudioComponentRegistrar`,
`rm -rf ~/Library/Caches/AudioUnitCache/`, retry before investigating anything else.

Then a **full `auval -v aufx OuOc OuDv`** — this is what exercises all six AU channel configs from
F2, including 1-in/1-out, and is the real test of Task 6's `buffer.getNumChannels()` bound.

---

### Task 11 — pluginval strictness 10, VST3 **and** AU, 2–3 runs each

- **Files:** none (verification)
- **Depends on:** Task 10

```bash
PV=/Applications/pluginval.app/Contents/MacOS/pluginval
for i in 1 2 3; do
  "$PV" --strictness-level 10 --skip-gui-tests --timeout-ms 60000 \
        --validate ~/Library/Audio/Plug-Ins/VST3/O-Octagon-dev.vst3
  "$PV" --strictness-level 10 --skip-gui-tests --timeout-ms 60000 \
        --validate ~/Library/Audio/Plug-Ins/Components/O-Octagon-dev.component
done
```

**Repeat runs are not optional** (`pattern_ci_pluginval10_catches_latent_nan`) — strictness 10
randomises buffer sizes and parameter values across runs, and a single green run proves less than
it appears. `scripts/verify-suite-battery.sh` is the in-repo precedent but defaults to strictness 8
and **VST3 only**; the AU run is a separate invocation against the installed `.component`.

pluginval's state-restoration test covers the Task 8 round-trip at any strictness.

---

### Task 12 — Standalone: SAFE mode (COMPAT-04) and the 17-parameter check

- **Files:** none (verification, manual)
- **Depends on:** Task 10

1. Launch `O-Octagon-dev.app`. On a 2-channel interface it must open **without error** —
   `canonicalChannelSet(2) == stereo()` is in the SAFE clause (RESEARCH §2.3). This is the
   COMPAT-04 gate.
2. The window shows the `GenericAudioProcessorEditor`. **Cross-check all 17 parameters against
   `parameter-spec.md`** — name, range, default and unit, one by one. This is the exit criterion,
   and the generic editor is the reason it is checkable at Stage 1.
3. Audio: input reaches both outputs at unity.

**Expected, not a defect:** on a 3–7 output device a Debug build fires JUCE's own `jassert` in
`setPlayConfigDetails`. That is F3. A Release build must not crash or produce garbage — which is
what Task 6's `buffer.getNumChannels()` bound buys. If an 8-output interface is available, the
Standalone negotiates **real 7.1 with no host** (F4) and all 8 lanes should carry the dry signal —
a free preview of the Stage 2 listening rig.

---

### Task 13 — Logic: 8-channel negotiation and session round-trip

- **Files:** none (verification, manual)
- **Depends on:** Task 10

1. Insert O-Octagon on a **surround** audio track in Logic (7.1 / 7.1-SDDS / 5.1.2). It must appear
   in the plugin menu and instantiate.
2. Play audio: **all 8 lanes of Logic's surround meter move.** This is the strongest available
   Stage 1 evidence for FUNC-01 / COMPAT-01 — it proves both that an 8-channel container was
   negotiated and that every channel index is writable.
3. Move `outputGain` off its default, save the project, close it, reopen — the value persists.
   That is FUNC-05's Stage 1 slice.
4. Confirm the automation menu lists **17** parameters under the five P1 groups.

**Record which container Logic actually negotiated.** R2 predicts 7.1-SDDS (the Emagic default
ordering); the plugin accepts all three either way, so this is *observation*, not a gate. It feeds
COMPAT-02 at Stage 4.

---

### Task 14 — Registry and status

- **Files:** `PLUGINS.md`, `plugins/O-Octagon/.planning/STATUS.md`
- **Depends on:** Tasks 11, 12, 13

`PLUGINS.md:68` — flip O-Octagon from `🚧 Stage 0` to `🚧 Stage 1`, version `1.0.0-dev`, date.

`STATUS.md` — phase table (`plan ✓`, `execute ✓`), progress bar, next action, and **the Task 13
observation of which 8-channel container Logic negotiated**, which is a Stage 4 input.

Commit on `feat/o-octagon`. End the session committed
(`pattern_uncommitted_improve_versions_lost`).

---

## Dependency graph

```
T1 parameter-spec.md ─┬─> T3 header ─┬─> T4 ctor + params ──┬──> T9 build ──> T10 install/auval
T2 CMakeLists ────────┘              ├─> T5 busesSupported ─┤        │
                                     ├─> T6 processBlock ───┤        ├──> T11 pluginval ─┐
                                     ├─> T7 editor/boiler ──┤        ├──> T12 standalone ┼─> T14 status
                                     └─> T8 state I/O ──────┘        └──> T13 Logic ─────┘
```

T2 is independent of T1 and can be written first. T5 depends on T3 only for the declaration —
T4–T8 are all edits to the same `.cpp` and in practice land together.

---

## Files

**Created**

| File | Task |
|---|---|
| `plugins/O-Octagon/.planning/parameter-spec.md` | 1 |
| `plugins/O-Octagon/CMakeLists.txt` | 2 |
| `plugins/O-Octagon/Source/PluginProcessor.h` | 3 |
| `plugins/O-Octagon/Source/PluginProcessor.cpp` | 4–8 |

**Modified**

| File | Task |
|---|---|
| `plugins/O-Octagon/.planning/parameter-spec-draft.md` (superseded banner) | 1 |
| `PLUGINS.md` | 14 |
| `plugins/O-Octagon/.planning/STATUS.md` | 14 |

**Not touched:** root `CMakeLists.txt` (the `plugins/*` glob picks the new directory up on
re-configure — RESEARCH §5, verified at `:48-58`).

---

## Success criteria — Stage 1 exit

- [ ] Builds clean on macOS — VST3, AU, Standalone — **zero warnings** from
      `juce_recommended_warning_flags` in O-Octagon's own translation units
- [ ] `auval -a | grep -i octagon` lists the AU; `auval -v aufx OuOc OuDv` passes all six
      channel configs
- [ ] **pluginval strictness 10 passes — VST3 *and* AU — 2–3 runs each**
- [ ] Standalone opens on a 2-channel interface without error (**COMPAT-04**)
- [ ] All **17** parameters appear with correct names, ranges, defaults and units, in five groups
- [ ] A parameter change round-trips through save/reload of session state
- [ ] All 8 lanes of Logic's surround meter move (**FUNC-01, COMPAT-01**)
- [ ] `parameter-spec.md` exists and matches the shipped parameter set exactly

## Non-goals — do not build these at Stage 1

`ChannelMap` / `rebuildChannelMap()` (Phase 2.1 — **one construction site** is a load-bearing
property; a Stage 1 "temporary" map creates a second) · the `VENUE` tree and its 42 values (2.1) ·
`VenueModel`, `ConvexHull2D`, `DbapSolver`, `SourceShaper`, `HullProcessor`, `GainStage`,
`VerifyPing` · the 64-sample control grid and the 17 `SmoothedValue`s (2.2) · the render harness
(2.2) · any WebView asset or `juce_add_binary_data` target (3.1) · `PluginEditor.{h,cpp}` (3.1) ·
any module dependency (RESEARCH §6 — Stage 1 adds none).

## Carried forward — recorded here so Stage 2 does not re-derive them

- **[2.1]** F1: all three accepted containers have initializer order == enum-bit order, so a
  hardcoded 0..7 map is byte-identical to a correct one under *every* accepted layout. The
  channel-map test must drive **non-identity `map1..map8` label maps**; a container-only test is
  vacuous.
- **[2.1]** Never `switch` on `AudioChannelSet::ChannelType` — `-Wswitch-enum` warns even with a
  `default:`. Table or `if`-chain.
- **[2.1]** `readVenueFromState()` must treat a missing/partial `VENUE` node as "use defaults".
- **[2.2]** Output loop bound is `buffer.getNumChannels()` — never `8`, never
  `getTotalNumOutputChannels()`.
- **[2.2]** `-Wfloat-equal` bans `==`/`!=` on floats — it bites the control-grid dirty check the
  moment it is written.
- **[3.1]** Module candidates: `vu-meter` (ballistics need attack 0.5 / decay 0.12 — check
  configurability), `webview-relay-manager` (17 relays → destruction order), `resource-provider`,
  `preset-manager` (3.2, must never reach the `VENUE` node).
