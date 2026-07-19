# Stage 4: Polish — Research

**Date:** 2026-07-14
**Plugin:** O-Contrabass
**Entry state:** Stage 3 (GUI) VERIFIED 2026-07-11. Discuss complete (`CONTEXT.md`).
**Research depth:** DEEP (complexity tier 6).
**Method:** 4 parallel research agents (preset format, Windows CI, Dorico `.doricolib`, PERF/audition) + direct source/CMake/CI inspection.

Stage 4 is the final v1.0 stage: author preset banks, run subjective/perf/Dorico validation,
close the Windows cross-platform gate, ship the Dorico distribution bundle, and reconcile
version/docs. **DSP is FROZEN** — 19/19 render goldens must stay byte-identical; all work
writes parameter STATE + docs + distribution artifacts, never signal-path arithmetic.

Requirements under research (REQUIREMENTS.md:255): **FUNC-03** (dual orchestral/drone, subjective),
**FUNC-04** (10 presets), **PERF-02** (<5% CPU/voice), **COMPAT-02** (Dorico NE microtonal), plus
the Stage-4-owned remainder **COMPAT-01** (Windows VST3 pluginval-10) and the promoted subjective
carry-forward **DSP-10** (slow-attack character confirmation).

---

## ⚠️ Corrections to CONTEXT.md (verified against code — apply in plan)

1. **The "6 skewed params" list in CONTEXT is wrong.** Reading `PluginProcessor.cpp:36-132`, only
   **4** params are actually skewed, and the set differs from CONTEXT's:
   - **Actually skewed:** `BOW_SPEED`(skew 0.5), `BOW_PRESSURE`(0.5), `BRIGHTNESS`(0.25), **`VIBRATO_ONSET`(0.5)** ← CONTEXT missed this one.
   - **Actually LINEAR (CONTEXT wrongly called skewed):** `VIBRATO_RATE`, `SLOW_LFO_RATE`, `REFERENCE_PITCH`.
   - **Impact: none on approach** — the mitigation (route *every* value through `convertTo0to1`) is correct for skewed and linear alike. But the plan's per-param notes must be corrected so the doc isn't self-contradictory.
2. **Compiled version is already `1.0.0`** (root `project(JUCEPlugins VERSION 1.0.0)` → `JucePlugin_VersionString`).
   The CHANGELOG's `[1.1.0-dev]` header is **doc-only**, not a compiled macro. The v1.0.0 "collapse"
   is therefore a CHANGELOG/doc reconciliation, not a code-version change. **This interacts with a
   preset re-seed gotcha — see Q1.5.**
3. **parameter-spec.md is stale on voice count** — says "monophonic/1 voice"; actual `kNumVoices = 4`
   (`PluginProcessor.h:138`, for MPE per-note + double-stop drones). Relevant because drone presets
   can sound multiple strings. IDs/ranges/defaults in the spec are otherwise accurate (but it omits
   skew factors — read skew only from the .cpp).

---

## Q1 — Factory presets (FUNC-04)

### Q1.1 Mechanism: seeded to disk, NOT embedded

O-Contrabass uses the **canonical** `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`
via CMake include (no vendored copy → no drift). Factory presets are **written to disk on first run**,
not embedded in BinaryData:

- Storage root: `~/Library/O-Contrabass/Presets/` → `Factory/*.json` (read-only) + `User/*.json`.
- `getPresetList()` globs `Factory/` then `User/` into one **flat, alphabetically-sorted** list.
- Declaration API (`OuariconPresetManager.h:167-180`):
  ```cpp
  struct FactoryPresetDef {
      juce::String name;
      std::map<juce::String, float> parameters;  // Parameter ID -> NORMALIZED 0..1 value
      juce::var customState;                       // optional
  };
  void initializeFactoryPresets(const std::vector<FactoryPresetDef>& presets);
  ```
- **`parameters` values are stored verbatim as normalized 0..1** and fed back through
  `param->setValueNotifyingHost(value)` (i.e. `convertFrom0to1`) on load.

### Q1.2 The ONLY missing code

The full JS↔C++ preset bridge (all 10 native fns: save/load/list/next/prev/delete/isFactory/dialogs)
is **already wired** in `PluginEditor.cpp:134-243`. **`initializeFactoryPresets(...)` is called
NOWHERE in O-Contrabass.** The entire Stage-4 preset task reduces to:
1. Author the 10 `FactoryPresetDef`s (engineering units).
2. Convert each value via `convertTo0to1` (skew-safe).
3. Add one `presetManager.initializeFactoryPresets(...)` call in the processor constructor.

### Q1.3 Skew-safe authoring (pattern_factory_preset_normalized_ignores_skew)

Author values in **engineering units**, then convert once, right before seeding — this is correct for
skewed, linear, int, choice, and bool params uniformly:
```cpp
for (auto& preset : factoryPresets)
    for (auto& [id, value] : preset.parameters)
        if (auto* p = parameters.getParameter(id))
            value = p->convertTo0to1(value);     // THE mechanism — handles skew + int/choice/bool
presetManager.initializeFactoryPresets(factoryPresets);
```
JUCE skew math (`juce_NormalisableRange.h`): `convertTo0to1(v) = pow((v-min)/(max-min), skew)`.
Hand-writing raw normalized fractions ignores skew → recalls 4×–30× wrong on the 4 skewed params.

### Q1.4 Reset-to-defaults (pattern_preset_apply_needs_reset_to_defaults) — already handled

`applyPresetJson` (`OuariconPresetManager.h:296-308`) resets **every** APVTS param to its default
*before* applying preset keys. So factory defs may safely omit keys they want at default — omitted keys
revert to APVTS default, not stale live state. No extra work needed; the canonical module is correct.

### Q1.5 ⚠️ Version-sentinel re-seed gotcha (execute-loop hazard)

`initializeFactoryPresets` writes a `.factory-version` sentinel and **only (re)seeds when
`JucePlugin_VersionString` changes** (`OuariconPresetManager.h:587-590`). Since the compiled version
is pinned at `1.0.0` and stays there all stage, the **audition→tweak→rebuild loop will NOT re-seed
edited preset defs** once the Factory dir exists — the sentinel already reads `1.0.0`.

**Mitigation for the plan:** during the execute audition loop, either (a) `rm -rf ~/Library/O-Contrabass/Presets/Factory/`
before each reinstall so presets re-seed, or (b) add a dev-only force-reseed path. Document this in the
execute steps so tweaks actually take. (First-ever install seeds cleanly — the hazard is only on
*re-tweaks* within the same version.)

### Q1.6 Parameter inventory (31 params — authoritative, from PluginProcessor.cpp:36-132)

Skewed params in **bold**. `NormalisableRange(min, max, interval[, skew])`; skew omitted ⇒ 1.0.

| Param ID | Type | Min | Max | Skew | Default |
|----------|------|-----|-----|------|---------|
| **`BOW_SPEED`** | float | 0.02 | 1.5 | **0.5** | 0.15 |
| **`BOW_PRESSURE`** | float | 0.05 | 8.0 | **0.5** | 1.0 |
| `BOW_POSITION` | float | 0.02 | 0.25 | 1.0 | 0.10 |
| **`BRIGHTNESS`** | float | 80.0 | 12000.0 | **0.25** | 4500.0 |
| `OUTPUT_GAIN` | float | -60.0 | 12.0 | 1.0 | 0.0 |
| `ROSIN` | float | 0.0 | 1.0 | 1.0 | 0.65 |
| `BOW_NOISE` | float | 0.0 | 1.0 | 1.0 | 0.35 |
| `BODY_SIZE` | float | 0.0 | 1.0 | 1.0 | 0.75 |
| `BODY_DAMPING` | float | 0.0 | 1.0 | 1.0 | 0.40 |
| `BODY_MIX` | float | 0.0 | 1.0 | 1.0 | 0.80 |
| `STRING_TENSION` | float | 0.0 | 1.0 | 1.0 | 0.50 (INERT — v1.1, do not author away from 0.5) |
| `STRING_STIFFNESS` | float | 0.0 | 1.0 | 1.0 | 0.30 |
| `ACTIVE_STRINGS` | **int** | 1 | 4 | — | 4 |
| `DETUNE_E` | float | -1200 | 1200 | 1.0 | 0.0 |
| `DETUNE_A` | float | -1200 | 1200 | 1.0 | 0.0 |
| `DETUNE_D` | float | -1200 | 1200 | 1.0 | 0.0 |
| `DETUNE_G` | float | -1200 | 1200 | 1.0 | 0.0 |
| `VIBRATO_RATE` | float | 0.1 | 12.0 | 1.0 | 5.0 |
| `VIBRATO_DEPTH` | float | 0.0 | 50.0 | 1.0 | 0.0 |
| **`VIBRATO_ONSET`** | float | 0.0 | 3000.0 | **0.5** | 600.0 |
| `SLOW_LFO_RATE` | float | 0.05 | 2.0 | 1.0 | 0.3 |
| `SLOW_LFO_DEPTH` | float | 0.0 | 1.0 | 1.0 | 0.0 |
| `EXPRESSION_MACRO` | float | 0.0 | 1.0 | 1.0 | 0.0 |
| `INFINITE_SUSTAIN` | float | 0.0 | 1.0 | 1.0 | 0.0 |
| `SUB_HARMONICS` | float | 0.0 | 1.0 | 1.0 | 0.0 |
| `WIDTH` | float | 0.0 | 2.0 | 1.0 | 1.0 |
| `MASTER_SAT_AMOUNT` | float | 0.0 | 1.0 | 1.0 | 0.50 |
| `LIMITER_CEILING_DB` | float | -6.0 | 0.0 | 1.0 | -0.3 |
| `REFERENCE_PITCH` | float | 220.0 | 880.0 | 1.0 | 440.0 |
| `TUNING_SYSTEM` | **choice** | 0 | 2 | — | 2 (12-TET); 0=Scala 1=MTS-ESP 2=12-TET |
| `NOTE_EXPRESSION` | **bool** | — | — | — | true |

Author int/choice/bool in natural units and let `convertTo0to1` normalize
(`{"ACTIVE_STRINGS",4.0f}`, `{"TUNING_SYSTEM",2.0f}`, `{"NOTE_EXPRESSION",1.0f}`).

DETUNE range ±1200¢ comfortably holds the Just-Intoned Drone targets `DETUNE_A=+204`,
`DETUNE_D=-14`, `DETUNE_G=+182` (all on the 0.1¢ grid).

### Q1.7 Preset→param relevance map (values authored at execute, not now)

**Orchestral:** Cinematic Bass Sustain (default) = high BODY_SIZE/MIX, slow BOW_SPEED, long
VIBRATO_ONSET, low BODY_DAMPING · Section Bass = damped + WIDTH↑ · Solo Arco = BOW_NOISE/ROSIN↑,
expressive vibrato · Pianissimo = low BOW_PRESSURE/SPEED, sul-tasto BOW_POSITION, low OUTPUT_GAIN ·
Forte = high BOW_PRESSURE, faster BOW_SPEED, BRIGHTNESS↑.
**Drone:** Infinite Drone = INFINITE_SUSTAIN max + SLOW_LFO + SUB_HARMONICS, ACTIVE_STRINGS>1 ·
Just-Intoned Drone = DETUNE_A/D/G above + ACTIVE_STRINGS=4 · Scordatura = alternate DETUNE_* ·
Sub Drone = SUB_HARMONICS heavy, low BRIGHTNESS · Dark Pad = BODY_DAMPING↑, BRIGHTNESS↓, slow swell.

### Q1.8 ⚠️ Bank grouping + tuning-reset decisions (flag for plan)

- **No `category` field** in the canonical module → "Orchestral"/"Drone" banks are **cosmetic** (one
  flat sorted list). Real folders would require vendoring O-Prism's categorized manager (bigger change).
  **Recommend flat + naming convention** (e.g. prefix or clear names) for a 10-preset set.
- **Preset load resets TUNING_SYSTEM→12-TET, REFERENCE_PITCH→440, NOTE_EXPRESSION→true** (WR-01 reset
  covers tuning params). Per-string pitch offsets for Just-Intoned/Scordatura must use `DETUNE_*`
  (independent APVTS params — they do NOT touch the Scala engine). Decide whether drone presets should
  carry an explicit `TUNING_SYSTEM`/`NOTE_EXPRESSION` to avoid the reset clobbering a user's setup.

### Q1.9 Template to copy

**O-SpectralShaper** — inline `std::vector<FactoryPresetDef>` in engineering units + single
`convertTo0to1` loop (`plugins/O-SpectralShaper/Source/PluginProcessor.cpp:254-291`). Simplest, no
`category` dependency. (Alternative: O-Prism/O-Bassoon `FactoryPresets.{h,cpp}` helper — scales better
but pulls in the vendored-`category` variant; unnecessary here.)

---

## Q2 — Dorico `.doricolib` bundle (COMPAT-02)

### Q2.1 Three artifacts, three folders

Only ONE full Dorico bundle exists in the repo: `plugins/O-MicrotonalSampler/Resources/dorico/` — it is
the sole authoritative template (O-Lyrica/O-Bells/O-Wind/etc. have the NE *module* wired but **no**
Dorico artifacts checked in). O-Contrabass needs a single-family reduction of it:

| Artifact | Installs to (macOS `~/Library/Application Support/Steinberg/Dorico 6/`) | Role |
|----------|------------------------------------------------------------------------|------|
| `playbacktemplatedeps.doricolib` | `DefaultLibraryAdditions/O-Contrabass.doricolib` | The `<ExpressionMapDefinition>` (load-bearing microtonal fields + one natural technique) |
| `endpointconfig.xml` | `EndpointConfigs/O-Contrabass/endpointconfig.xml` | Binds VST3 GUID+name → slot/channel + exp-map + routed instrument IDs |
| `playbacktemplatespec.xml` | `PlaybackTemplateSpecs/O-Contrabass/playbacktemplatespec.xml` | User-facing template in *Play → Playback Template…* |

**ID reference chain (must match byte-for-byte):**
```
spec  <configID>endpointconfig.user.o_contrabass</configID>
  → endpoint <configID>endpointconfig.user.o_contrabass</configID>
             <expressionMapID>xmap.ouaricon.o_contrabass</expressionMapID>
  → doricolib <entityID>xmap.ouaricon.o_contrabass</entityID>
  + endpoint <pluginID> = VST3 ClassID GUID (see Q2.4)
```
**Repo location (new — no `Resources/` dir exists under O-Contrabass yet):**
`plugins/O-Contrabass/Resources/dorico/{EndpointConfigs,PlaybackTemplateSpecs}/... + INSTALL-DORICO.md + SMOKE-TEST.md`.

### Q2.2 LOAD-BEARING microtonal fields (P0 — recurring silent regression)

In the `<ExpressionMapDefinition>`, between `<applyStageTemplateSettings>` and `<initSwitchData>`:
```xml
<pitchBendRange>2</pitchBendRange>
<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>
```
- Value is **`2`, not 48** (NE carries absolute tuning via `kTuningTypeID`; this field is nominal but
  required present). Do NOT use `kAuto` (falls back to 12-TET/pitch-bend).
- **Per-combination copies are additional, NOT a substitute** — removing the top-level pair silently
  drops to nearest-12-TET even with per-combination copies present. pluginval/auval/template-load all
  still pass; only an end-to-end microtonal pitch test catches it → **make TC-4 (24-EDO quarter-sharp)
  the Stage-4 P0 acceptance test.**
- After edits: bump the definition `<version>` (cache key = entityID+version) + full Cmd-Q relaunch.

### Q2.3 Expression map reduces to one natural technique

No keyswitches → keep the load-bearing fields + a single `pt.natural` combo with an **empty
`<switchOnActions array="true"/>`**; delete the 7 non-natural technique combos from the reference
Strings map. Full skeletons for all three files are in the agent findings (copy at plan/execute).

### Q2.4 Routing + identifiers

- **VST3 ClassID `<pluginID>` (dev build) = `ABCDEF019182FAEB4F7544764F436273`** (from built
  `moduleinfo.json`; tail decodes to `OuDv`+`OCbs`). This is the single most error-prone field — a
  mismatch = silent empty slot. **Release build (manufacturer `OuAu`, no `-dev`) has a DIFFERENT GUID.**
  Plan must decide: ship a dev-branded bundle now + document the release-GUID swap (O-MicrotonalSampler
  precedent), or generate against the release build.
- `<pluginName>` = `O-Contrabass-dev` (dev) / `O-Contrabass` (release).
- **Routing is by endpoint `<instruments>` entityIDs, NOT `<instrumentFamilies>`** (the latter is
  vestigial per O-MicrotonalSampler INSTALL-DORICO). Enumerate: `instrument.strings.contrabass`,
  `instrument.strings.contrabass.d`, `instrument.strings.uprightbass`.
- Single-timbre solo bass → **one endpoint config, one channel, `kSoloPlayer`**. No family split.

### Q2.5 Dynamics: use `kNoteVelocity` (differs from O-MicrotonalSampler)

O-Contrabass has **no CC11/CC1 dynamics listener** (verified in PluginProcessor/Voice) — dynamics come
from APVTS params + note velocity + MPE Y/Z. So set `volumeType kNoteVelocity`, **not** the CC11 default
O-MicrotonalSampler uses (that plugin added a dedicated CC11 crossfade path in v1.21.0).
**Caveat:** velocity is fixed at note-on → sustained crescendos/hairpins within one arco note won't
render. If continuous sustained dynamics matter, a small plugin-side CC11→`EXPRESSION_MACRO`/bow-pressure
listener + `volumeType kCC param1=11` would be needed — **but that touches DSP/param handling and risks
the frozen-goldens invariant; recommend deferring to v1.1** and shipping `kNoteVelocity` for v1.0.

### Q2.6 Distribution + gotchas

- A bare `.doricoexpmap` in `User/` is **silently skipped** — must ship the 3-folder bundle with the
  `.doricolib` in `DefaultLibraryAdditions/`.
- Activation: quit Dorico → copy 3 files → relaunch (DefaultLibraryAdditions loads at app startup) →
  *Play → Playback Template… → tick O-Contrabass → Apply and Close*. **"Loaded ≠ Applied"** — a fully
  ingested template still leaves staves on NotePerformer until explicitly applied.
- **Parser strictness:** any XML comment must live INSIDE `<kScoreLibrary>`, never before the root
  element (leading comment → "invalid file format" crash on launch).
- Delegate authoring to **dorico-agent**; author `INSTALL-DORICO.md` + `SMOKE-TEST.md` (P0 = TC-4).
- Add a CMake `install(DIRECTORY Resources/dorico/…)` rule (none exists) so the bundle ships.

---

## Q3 — Windows CI + pluginval-10 (COMPAT-01 remainder) — ⚠️ BLOCKER

### Q3.1 Two blockers in the current workflow

Repo has exactly one workflow: `.github/workflows/build-and-release.yml`.

1. **CI runs pluginval NOWHERE** (repo-wide grep: every `pluginval` hit is docs). The
   Stage-4 gate "Windows VST3 pluginval-10 via CI" is **NOT satisfiable as-is** — a new CI step must be
   authored (download pluginval Windows build + run `--strictness-level 10` against the built VST3).
2. **The only trigger is `push` of a `*-v*` tag, which also PUBLISHES a public GitHub Release**
   (`create-release`, `draft:false prerelease:false`). This directly conflicts with the Stage-4 user
   decision to **hold public release**. A non-publishing path must be added.

### Q3.2 What's already correct (no work needed)

- O-Contrabass is auto-discovered by root CMake (`file(GLOB plugins/*)`) and **not** in `SKIP_PLUGINS`
  (only `O-Orbit` is skipped). It is CI-buildable today — it has simply never been tagged/built on Windows.
- Windows job builds VST3: `cmake --build build --target O-Contrabass_VST3 --config Release`. Target
  name `O-Contrabass` == folder name → **no target/folder mismatch** (the fleet-wide hazard doesn't bite here).
- **WebView2 SDK provisioning is correct**: CI installs `Microsoft.Web.WebView2` v1.0.1901.177 via NuGet
  to exactly the path JUCE's `FindWebView2.cmake` searches by default; static linking wired via
  `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`. **The blank-UI risk is mitigated
  at config level — but never exercised on a real Windows runner.**
- JUCE pinned 8.0.9, downloaded fresh; vendored `JUCE-NE-PATCH` overrides copied via `cp -R
  vendored/JUCE-overrides/modules/.` before configure on both OSes; note-expression's configure-time
  marker check is CMake-native (Windows-safe, reads `JUCE_DIR`). **Vendoring is CI-correct** — no
  missing-patch failure risk for the Windows build.

### Q3.3 What the plan must decide/author

- **Add a `workflow_dispatch` trigger** (with a `plugin_name` input, and/or a `validate-only` flag) so a
  Windows build + pluginval can run **without** publishing a release; gate/skip the `create-release`
  (and macOS sign/notarize) jobs on that path.
- **Add a pluginval-10 step** to `build-windows`: pin a pluginval version, download the Windows build,
  run `pluginval.exe --strictness-level 10 --validate <...>/O-Contrabass*.vst3` (VST3 only — no AU on
  Windows), upload the log as an artifact.
- **Consider raising `--timeout-ms`/a warm-up run** — macOS saw a cold-WKWebView first-run Editor
  Automation timeout that passed warm; a fresh Windows runner (cold WebView2, no cache) may hit the same.

### Q3.4 Risks

- Windows configure builds **all** non-skipped plugins (`cmake -B build` configures every `plugins/*`
  except O-Orbit) → a Windows-hostile configure error in **any** sibling would fail O-Contrabass's
  configure step. Whether all ~30+ siblings configure clean on `windows-latest` is unverified (this path
  only ever ran per-tagged-plugin). Possible mitigation: extend `SKIP_PLUGINS` for the validation run.
- WebView UI has never been visually validated on real Windows — first CI run is the actual test.

---

## Q4 — PERF-02 benchmark method (<5% CPU/voice, Apple Silicon, 44.1/48kHz, 256-block)

### Q4.1 Harness already has the timing plumbing

`tests/render-harness/main.cpp` already wraps every `processBlock` in a high-res timer and pushes
per-block micros into `blockMicros`, reducing to median/max + a **spike-sentinel** ratio (`pass_blockTime
= ratio <= 5×`), NOT a CPU budget. Non-golden-locked modes already emit real `blockMicros_median`
/`_max`; golden-locked modes zero timing for sha256 determinism.

- **Golden gate is WAV-sha256-only** (`reproduce-goldens.sh:90-91`) → adding an RTF/%CPU field or a
  dedicated `--perf` mode **cannot break the 19/19 byte-identical invariant.** Safe under DSP-frozen.
- Harness hardcodes `sampleRate=44100 / blockSize=512` (`main.cpp:653-654`). PERF-02 needs 256-block ×
  {44100, 48000} → add `--sample-rate` + `--block-size` CLI overrides (or a `--perf` mode).
- RTF math (all inputs exist): `cpuPct = 100 × medianMicros / (blockSize/sampleRate × 1e6)`.
  Budget per 256-block: 5805µs @44.1k, 5333µs @48k.

### Q4.2 "Per voice" isolation is free

`kNumVoices=4` but MPESynthesiser allocates per note-on; idle voices no-op in `renderNextBlock`. A single
sustained MIDI note = exactly 1 active voice → the existing single-note render **is** a per-voice
measurement. Cost drivers per voice: 2× oversampled waveguide + friction, 8-mode body resonator, 3-band
bow-noise BPF, in-loop sub-harmonic bias + tanh; post-mix (per-block): master saturator + limiter + width.

### Q4.3 Method choice

**Primary = harness RTF** (deterministic, CI-reproducible, zero-DAW, doesn't touch WAV goldens, isolates
one voice, ~90% already built). **Corroborate with a Logic CPU-meter spot-read** (O-Bassoon precedent —
the only prior house "<5% CPU" check) as the real-DAW sanity anchor. **Reject pluginval timing** as the
number source. No house plugin has ever computed an RTF/CPU% number — **this is the first**; the method
becomes reusable. "Typical settings" = factory defaults (= the Cinematic Bass Sustain default), Release build.

---

## Q5 — A/B audition rig (FUNC-03 + DSP-10 subjective sign-off)

### Q5.1 House method = documented probe-table (the R38 precedent)

Stage 2 captured subjective sign-off via a **documented Logic AU audition with a probe-sequence table**
(RESEARCH §21.11, `stages/2-dsp/RESEARCH.md:7139-7197`): a `| Probe | Setup | Pre-expectation |
Post-expectation | PASS criterion |` table + a FAIL-handling table, with the user CONFIRM recorded as
STATUS.md `carry_forward` prose + REQUIREMENTS verify comments. R38 Probe 7 was already an orchestral A/B
vs a reference library — the direct template for FUNC-03. The `.planning/evidence/*.wav` files are
objective matrix-stability renders, not subjective evidence.

### Q5.2 Recommended rig

A **documented audition checklist in `stages/4-polish/`** reusing the R38 probe-table format — **no
git-worktree side-by-side needed** (DSP frozen → there's no "before"). The A/B is **(a) preset-switch
within one installed Release build** + **(b) compare against user-owned references** (Spitfire
Albion/CSS/VSL orchestral; Stephen O'Malley/Tony Conrad drone). Probe rows:

| Probe | Preset | Material | Reference | PASS |
|-------|--------|----------|-----------|------|
| Orchestral arco (FUNC-03a) | Cinematic Bass Sustain | Sustained E1+A1 arco 8s | Spitfire/CSS/VSL | "same sonic family" |
| Slow attack (DSP-10) | Cinematic | Held legato; ~1168ms onset ramp | — | no note-on click; natural swell |
| Drone (FUNC-03b) | Infinite Drone | Near-infinite evolving | O'Malley/Conrad | "evolving drone in the spirit of" |
| A/B switch (FUNC-03c) | Cinematic ↔ Infinite Drone | switch preset, no host retune | — | both credible, one instance |
| Preset QA ×8 | remaining 8 presets | per-preset audition | as applicable | accept or request tweak |

The rig **doubles as FUNC-04 preset QA** (user auditions all 10, requests tweaks in execute). Record
CONFIRM as STATUS.md carry-forward + REQUIREMENTS verify comments on FUNC-03/DSP-10/FUNC-04. Manual host
locked to **Logic Pro** (+ Dorico for COMPAT-02). Optionally archive harness `--out` renders as a
repeatable objective anchor (O-Bassoon reference-archive precedent).

---

## Open decisions for the plan phase

1. **Preset banks:** flat sorted list (recommended) vs vendoring O-Prism's categorized manager for real
   Orchestral/Drone folders. → Recommend flat + naming.
2. **Preset tuning-reset:** do drone presets carry explicit `TUNING_SYSTEM`/`NOTE_EXPRESSION` to avoid
   the WR-01 reset clobbering a user's Scala setup? → Likely yes for drone presets; decide per-preset.
3. **Dorico GUID:** ship dev-branded bundle now + document release-GUID swap, vs generate against release
   build. → Recommend dev bundle for validation + documented swap (matches O-MicrotonalSampler).
4. **Dorico dynamics:** `kNoteVelocity` for v1.0 (recommended) vs add a CC11 listener (defer to v1.1 —
   touches param handling, risks frozen goldens).
5. **Windows CI shape:** add `workflow_dispatch` + validate-only gate + pluginval step. Decide whether to
   extend `SKIP_PLUGINS` for the validation run to de-risk sibling configure failures. Decide pluginval
   version to pin + timeout handling.
6. **PERF harness:** `--perf` mode vs `--sample-rate`/`--block-size` knobs; corroborating Logic read yes/no.
7. **Version/docs:** collapse CHANGELOG `[1.1.0-dev]` → `[1.0.0]` (code version already 1.0.0);
   update parameter-spec.md stale "monophonic" note; NOTES.md v1.1 deferral list; registry.yaml R5.

## Do-NOT-touch (frozen / v1.1 deferrals — from CONTEXT)

STRING_TENSION inert (keep default 0.5, do not wire); `.tun` parser absent (picker `.scl`-only);
DSP-07 sub-harmonic depth; DSP-08 slow-LFO; DSP-09 vibrato depth; FUNC-07 MTS-ESP stub. 19/19 goldens
byte-identical is the invariant — any golden shift = defect, not polish.

## Key files (planning refs)

- Preset: `modules/persistence/preset-manager/cpp/OuariconPresetManager.h:167-180,296-308,587-630`;
  template `plugins/O-SpectralShaper/Source/PluginProcessor.cpp:254-291`; bridge already at
  `Source/PluginEditor.cpp:134-243`; params `Source/PluginProcessor.cpp:36-132`.
- Dorico: template `plugins/O-MicrotonalSampler/Resources/dorico/` (Strings def in
  `EndpointConfigs/.../playbacktemplatedeps.doricolib:33-320`); target GUID in built
  `moduleinfo.json`.
- Windows CI: `.github/workflows/build-and-release.yml` (trigger :9-12, windows job :397-517,
  create-release :522-586); `vendored/JUCE-overrides/`.
- PERF: `tests/render-harness/main.cpp:653-654,4085-4087`; `reproduce-goldens.sh:90-91`.
- Audition: `stages/2-dsp/RESEARCH.md:7139-7197` (R38 probe table); `REQUIREMENTS.md:139-142` (FUNC-03).
