---
title: "O-MicrotonalSampler Stage 4 (Polish) — Research"
created: 2026-04-28
stage: 4-polish
phase: research
status: complete
resolves:
  - RQ4-1
  - RQ4-2
  - RQ4-3
  - RQ4-4
inputs:
  - .planning/stages/4-polish/CONTEXT.md
  - .planning/stages/3-gui/VERIFICATION.md
  - .planning/REQUIREMENTS.md
references:
  - JUCE 8.0.4 source: /Users/taylorbrook/JUCE
  - pluginval 8.0.3 binary: /Applications/pluginval.app
  - Spike 002 (NE end-to-end): .claude/skills/spike-findings-VST-development/sources/002-quarter-sharp-end-to-end/README.md
  - Memory: critical_dorico_distribution_mechanism.md
  - Sibling-plugin patterns: O-FreqPulse, O-DigiDelay, O-Tremolo (`getPluginVersion` native fn)
---

# Stage 4 (Polish) — Research

Resolves the four open questions from `CONTEXT.md` (RQ4-1 .. RQ4-4) so
`/plugin-plan` can write atomic sub-stage tasks without further
investigation.

## Sub-stage map (recap from CONTEXT)

| Sub | Goal | Verifies |
|-----|------|----------|
| 4.1 | `getPluginVersion` native fn + JS wire-up + About-pill render | (polish) |
| 4.2 | PERF-02 — Logic CPU meter @ 16-voice / 48 k / 256 buffer | PERF-02 → complete |
| 4.3 | QUAL-01 — targeted artifact-pass listening test | QUAL-01 → complete |
| 4.4 | Final gate — pluginval-10 + auval + Logic + Dorico + invariant greps | (closure) |

Order is strict: 4.1 → 4.2 → 4.3 → 4.4. Failure in 4.2 or 4.3 reopens
the relevant Stage 2 sub-phase per the Stage 3 verify pattern.

---

## RQ4-1 — `JucePlugin_VersionString` source / accessor

### Question

Is `JucePlugin_VersionString` a string-literal macro at C++ compile
time, or does JUCE expose a runtime accessor? What include is needed,
and how do sibling Ouaricon plugins surface it to JS?

### Finding

**Compile-time string-literal macro**, defined per-target by JUCE's
CMake `juce_add_plugin` glue from the `VERSION` argument. Available
anywhere `<JuceHeader.h>` is included (which O-MicrotonalSampler does
for every `.h` in `Source/`).

**Verified value for this build:** confirmed in
`build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/JuceLibraryCode/Release/Defs.txt`:

```
JucePlugin_Version=1.0.0
JucePlugin_VersionString="1.0.0"
JucePlugin_VersionCode=0x10000
```

Source of truth is `plugins/O-MicrotonalSampler/CMakeLists.txt:14`:

```
PLUGIN_VERSION "1.0.0"
```

The `OUARICON_DEV_SUFFIX` (`CMakeLists.txt:13`) appends `-dev` to the
**product name** for dev builds, but `PLUGIN_VERSION` is unaffected —
the version pill will read `1.0.0` cleanly without dev decoration.

**JUCE 8 references** (string-literal use, no runtime accessor needed):

- `juce_audio_plugin_client/juce_audio_plugin_client_LV2.cpp:1156` —
  `StringArray::fromTokens (JucePlugin_VersionString, ".", "")`
- `juce_audio_plugin_client/juce_audio_plugin_client_Standalone.cpp:86`
  — `const String getApplicationVersion() override { return JucePlugin_VersionString; }`
- `juce_audio_plugin_client/VST3/juce_VST3ManifestHelper.cpp:177` —
  `moduleInfo.version = JucePlugin_VersionString;`

**Sibling Ouaricon pattern (validated, identical signature):**

- `O-FreqPulse/Source/PluginEditor.cpp:215`:
  ```cpp
  .withNativeFunction("getPluginVersion", [](const juce::Array<juce::var>&,
                                              std::function<void(juce::var)> complete) {
      complete(juce::var(JucePlugin_VersionString));
  });
  ```
- `O-DigiDelay/Source/PluginEditor.cpp:125` — same shape, abbreviated
  param spelling.
- `O-Tremolo/Source/PluginEditor.cpp:122` — same.

### Recommended approach

Mirror the `O-FreqPulse` shape verbatim, slot it into the existing
`withNativeFunction` chain in
`O-MicrotonalSampler/Source/PluginEditor.cpp` (the chain runs through
line ~440 in the current Stage 3.5 snapshot). Place it adjacent to the
other read-side accessors (`getTuningName`, `getOctaveStretch`, etc.)
between lines 137–172 to keep the `// ---- Tuning reads ----` block
together — version-pill is a one-shot read with the same async shape.

JS side: add `refreshAboutVersion()` modeled on `refreshTuningReadout()`
at `Resources/ui/js/sampler-app.js:333`. Call it once in the existing
About-tab activation handler (or unconditionally on `JUCE_init` —
single shot, no cost). It writes `${value}` (no `v` prefix — the
`<div class="about-version">` element already implies the role; or
include the `v` and strip the literal from HTML — see invariant).

**HTML edit:** `Resources/ui/index.html:100` —
replace `<div class="about-version">v0.1.0</div>` with
`<div class="about-version" id="about-version"></div>` (empty until
populated). Empty pill is the defensive render if the native function
fails to resolve (per CONTEXT invariant — should never happen, no
hard-coded fallback).

### Pitfalls

- **Don't** import `JucePlugin_VersionString` into JS via build-time
  codegen (was considered in CONTEXT D4-4; rejected — adds CMake step
  and re-build coupling for zero gain over the runtime accessor).
- **Don't** add a `'v' +` prefix in C++ — keep the native return as
  the raw `1.0.0` string. JS or HTML chooses the prefix. Sibling
  plugins all return the bare string.
- **Don't** rely on `JucePlugin_Name` for the About title — the
  dev-build adds `-dev` suffix (`JucePlugin_Name="O-MicrotonalSampler-dev"`
  in Defs.txt). The About title stays hard-coded `"O-MicrotonalSampler"`
  in `index.html:99`.

---

## RQ4-2 — pluginval `--strictness 10` delta vs strictness-5

### Question

Does `--strictness-level 10` add tests that fail for WebView-based
editors that strictness-5 tolerated? Quick check against O-Bells /
O-Bassoon Stage 4 history.

### Finding

**Flag-name correction first:** The documented and supported flag is
`--strictness-level [1-10]`, not `--strictness`. Verified by reading
the binary's own help output and string table:

```
$ /Applications/pluginval.app/Contents/MacOS/pluginval --help | grep strictness
  --strictness-level [1-10]
    Sets the strictness level to use. A minimum level of 5 (also the default)
    Higher levels include longer, more thorough tests such as fuzzing.
```

Binary string scan confirms: `--strictness-level` is the canonical
form; `strictnessLevel` (camelCase) is a deprecated alias still
accepted but slated for removal; **bare `--strictness` is not a
recognised flag** — it has been silently ignored in prior gate runs,
which is why our previous Stage 3 invocations (`pluginval --strictness 5
...`) appeared to succeed: they ran with the **default strictness 5**
because pluginval treated `--strictness 5` as junk and `5` was already
a stale path arg slot.

This means our Stage 3 gate was **effectively strictness-5 by default
fallback, not by explicit selection**. Stage 4 must use the correct
flag form to actually exercise strictness-10.

**Pluginval test classes** (from binary string-table dump of
`/Applications/pluginval.app/Contents/MacOS/pluginval`):

```
14PluginInfoTest          10PluginTest               17PluginPrgramsTest
10EditorTest              26EditorWhilstProcessingTest
19AudioProcessingTest     31NonReleasingAudioProcessingTest
15PluginStateTest         14AutomationTest           20EditorAutomationTest
25AutomatableParametersTest                          17AllParametersTest
25BackgroundThreadStateTest                          25ParameterThreadSafetyTest
9AUvalTest                12BasicBusTest             18FuzzParametersTest
```

Strictness gate behaviour (from pluginval source — generally available
test scaling, level ≤ 5 is "core compatibility", level 6–8 adds
duration / sample-rate combinations, level 9–10 unlocks
`FuzzParametersTest` and longer-duration runs of
`ParameterThreadSafetyTest`, `BackgroundThreadStateTest`).

**WebView-editor specific concerns at strictness 10:**

| Test | Strictness ≤ 5 | Strictness 10 | Risk for this plugin |
|------|----------------|---------------|----------------------|
| `EditorTest` | ON | ON, more open/close cycles | LOW — Stage 3 already passes with-GUI; WebView mount/unmount idempotent |
| `EditorWhilstProcessingTest` | ON | ON, longer audio runs | LOW — voices already validated under continuous load |
| `EditorAutomationTest` | ON | ON, more parameter sweeps with editor open | MEDIUM — APVTS → WebSliderRelay path runs more often; relies on Stage 3 relay/attachment lifetime correctness |
| `FuzzParametersTest` | OFF | **ON** — randomized rapid parameter mutation under audio | MEDIUM — exercises rapid mutation of `polyphony`, `velocityCrossfade`, `outputGain`, ADSR. RT path must remain stable. |
| `ParameterThreadSafetyTest` | LIGHT | **HEAVY** — concurrent set/get from many threads | LOW — APVTS handles atomic exchange; Stage 2 validated thread invariants |
| `BackgroundThreadStateTest` | LIGHT | **HEAVY** — getStateInformation / setStateInformation across long runs | MEDIUM — exercises preset save/load even though we have no preset UI; default APVTS state must survive round-trip |
| `NonReleasingAudioProcessingTest` | ON | longer | LOW — voice-stealing already tested |

**Sibling-plugin Stage-4 history** — checked all `.planning/` trees:

- O-Bassoon: strictness-10 **planned** for Stage 4 but plugin is still
  in Stage 2 — no actual run history yet.
- O-Bells, O-FreqPulse, O-DigiDelay, O-Tremolo, O-AnalogEQ: zero
  references to strictness-10 in their planning docs. All shipped at
  strictness-5 (with the same flag-name typo).
- **No prior strictness-10 evidence exists in this codebase.** Stage 4
  is the first time we will actually exercise it. Plan accordingly:
  budget for one or two re-runs with seed pinning if `FuzzParametersTest`
  surfaces a transient.

### Recommended approach

1. **Use the correct flag form everywhere** in PLAN.md and gate scripts:
   `--strictness-level 10`, not `--strictness 10`.
2. Run **two variants** at gate time, mirroring Stage 3 pattern:
   - `pluginval --strictness-level 10 --validate-in-process --skip-gui-tests <bundle>`
   - `pluginval --strictness-level 10 --validate-in-process <bundle>`
3. **Pin a random seed** for reproducibility across re-runs:
   add `--random-seed 0xC0FFEE` (or equivalent) so a fuzz failure is
   deterministically replayable. Stage 3 did not pin seed; Stage 4
   should, given fuzz tests are now active.
4. **Set explicit timeout** to absorb the longer fuzz duration:
   `--timeout-ms 120000` (2 min). Default 30 s will trip on
   high-strictness runs.
5. Run on **VST3 only** at strictness 10 (per CONTEXT scope —
   `pluginval` does not validate AU bundles directly; AU coverage
   comes from `auval`). VST3 path covers the WebView editor + audio
   processing surface; AU re-uses the same processor shared code.

### Pitfalls

- **Don't fix the flag name without re-running** — the prior "SUCCESS"
  results were strictness-5 by silent default. Stage 4 results at
  strictness-10 are NEW data; do not assume parity with Stage 3.
- **Fuzz failures may be intermittent** — if a single run trips
  `FuzzParametersTest`, run twice more with the same seed to confirm
  reproducibility before reopening Stage 2. Transient seed-specific
  edge cases should be diagnosed before declaring a regression.
- **`--validate-in-process` is mandatory** for WebView plugins on
  macOS — the in-process mode is what allows the editor to mount under
  pluginval's NSApplication context. Without it, EditorTest hangs.
  Stage 3 already used this; Stage 4 keeps it.

---

## RQ4-3 — Logic Pro CPU-meter measurement protocol for PERF-02

### Question

Does Logic Pro's CPU meter report per-track plugin CPU directly, or do
we measure delta between empty track and loaded track? Specify a
reproducible protocol for the PERF-02 number.

### Finding

Logic Pro 11.x exposes CPU usage via two surfaces:

1. **System Performance Meter** (`View → Show Performance Meter`,
   `⌥X`, or click the CPU/HD readout in the LCD/control bar) —
   **cluster-level CPU per core**, plus aggregate. Shows per-core bars
   for performance + efficiency cores. Recommended primary readout.
2. **Per-track CPU** is **NOT directly displayed** in the Mixer
   strip in Logic Pro by default. Logic exposes "CPU" as a column in
   some views (e.g. `Tracks Inspector` advanced) but the value is the
   instantaneous per-track-engine load, not isolated plugin CPU. It
   conflates plugin DSP, channel-strip plugins, and routing.

This means **we measure as a delta**, not a direct per-plugin readout:
baseline (empty project, transport stopped) vs loaded project (transport
playing 16-voice held chord through the plugin).

**Apple Silicon CPU model (relevant for the BRIEF target):**

- Logic Pro distributes audio threads across performance cores.
  `juce::Synthesiser` runs voices in a single audio thread per AU
  instance (no internal voice-parallel split).
- Logic 11 added internal per-track audio-thread parallelism, but a
  single instrument plugin running 16 voices is **still single-thread
  on the audio render side**.
- "5% CPU" in BRIEF/REQUIREMENTS PERF-02 must be interpreted as
  **5 % of total Apple-Silicon CPU budget**, i.e. the aggregate reading
  in the Performance Meter, not per-core. On a 10-core M-series
  machine, that's approximately half of one P-core sustained.

### Recommended protocol

**Measurement environment:**

- Logic Pro 11.x (current minor version), Apple Silicon (M1 Pro / M2 /
  M3 / M4 — note actual chip in VERIFICATION).
- Sample rate: 48 kHz (Logic → Settings → Audio → Devices).
- I/O Buffer Size: 256 (Logic → Settings → Audio → Devices).
- Process Buffer Range: Default (Medium).
- "Multithreading" enabled (default).
- All other plugins / instruments **off** (Logic stock processing only).
- Power source: plugged in (Apple Silicon throttles CPU on battery).

**Steps (reproducible):**

1. New empty Logic project, no instruments. `View → Show Performance
   Meter`. Note **baseline aggregate CPU %** with transport stopped
   (should be ≤ 1 %).
2. Press play with no instruments — note **baseline-with-transport**
   CPU % (typically 1–2 %, audio engine running idle).
3. Add one Software Instrument track. Insert
   `O-MicrotonalSampler (AU)`. Load a representative sample folder
   (≥ 60 cells loaded — use the stock test set used in Phase 2.5
   verification, or any folder large enough that full vel-xfade can
   engage).
4. Programme a held 16-note chord starting at bar 1 — 16 distinct
   MIDI notes (e.g. 4 octaves of 4 notes spanning the loaded zone) at
   moderate velocity (90), each held 4 bars. Loop the region.
5. Press play. Wait 30 seconds for steady-state.
6. **Read aggregate CPU peak and average** from the Performance Meter.
   Note **delta from step 2**: `delta_CPU_pct = loaded_avg − baseline_with_transport_avg`.
7. Stop. Repeat steps 5–6 twice more for variance check (3 readings).
8. Acceptance: `delta_CPU_pct ≤ 5.0 %` per BRIEF/REQUIREMENTS PERF-02.

**Tuning configuration during measurement:** active tuning system =
**12-EDO** (default) for the PERF-02 baseline. A separate confirmatory
read with the heaviest-cost active tuning (e.g. high-stretch microtonal
preset) should also be taken — record both numbers in VERIFICATION.

**What goes in VERIFICATION.md (PERF-02):**

| Field | Example |
|-------|---------|
| Hardware | Apple M3 Pro, 12 P-cores, plugged in |
| Logic version | 11.1.1 |
| Sample rate / buffer | 48 kHz / 256 |
| Sample folder | `/Users/.../[name]` (cell count, total MB) |
| Tuning active | 12-EDO (and microtonal-stretch comparison) |
| Baseline-with-transport CPU % | x.x % |
| Loaded average CPU % | x.x % |
| Loaded peak CPU % | x.x % |
| Delta vs baseline | x.x % |
| Verdict | ≤ 5 % → PASS / fail otherwise |

### Pitfalls

- **Battery throttling** — Apple Silicon downclocks aggressively on
  battery; readings will be 30–50 % higher and unreproducible. Always
  measure on power.
- **Cold-cache first read** — first 5–10 s after press-play can spike
  while sample data faults into RAM. Wait 30 s for steady-state.
- **Logic background tasks** — disable Logic's "Backup automatically"
  and any cloud-sync. Even an idle iCloud sync skews the CPU meter.
- **Don't use Logic's per-track CPU** column as the headline number —
  it's not isolated plugin CPU. Aggregate Performance Meter delta is
  the only reproducible reading.
- **Tuning recompute cost** — the recompute path runs on tuning-system
  change, not per-block. PERF-02 measurement is a steady-state
  read, so tuning-recompute spikes will not show. That's correct.

---

## RQ4-4 — Dorico smoke procedure

### Question

Minimum viable Dorico setup to route a microtonal passage through
O-MicrotonalSampler for the smoke test, given Dorico does not auto-ingest
standalone `.doricoexpmap` files (per
`critical_dorico_distribution_mechanism.md` memory).

### Finding

Three relevant facts from the spike-findings skill +
critical-distribution memory:

1. **Dorico ignores `INoteExpressionController` advertisement.**
   (Spike 002 finding, line 99–105.) NE events flow only when the
   active expression map sets Microtonality method to **"VST3 Note
   Expression"**. Without that, Dorico routes microtonal pitch via
   pitch-bend, which O-MicrotonalSampler ignores → 12-TET output.
2. **Standalone `.doricoexpmap` drop-in does not work** — Dorico's UI
   has no "Import Expression Map" command for this file format.
   Distribution to end users requires a **Playback Template** or
   `.doricolib` artifact (see memory). **For a smoke test, no
   distribution is needed** — we manually configure inside Dorico's UI,
   discard at session end.
3. **NE pipeline is already wired in O-MicrotonalSampler.** Verified:
   - `Source/PluginProcessor.h:22,116` — `NoteExpression.h` include +
     `Ouaricon::NoteExpression::VST3Extensions vst3Extensions`.
   - `Source/MicrotonalSamplerVoice.cpp:508` — voice-side
     `applyPendingTuning` consumes NE events.
   - `Source/MicrotonalSamplerVoice.h:57,64` — `pendingTuningSource`
     wired to PendingTuningTable.

### Recommended smoke procedure (manual, ~10 min)

**Setup (one-time per Dorico session):**

1. Quit Dorico if running. Confirm O-MicrotonalSampler.vst3 is
   installed at `~/Library/Audio/Plug-Ins/VST3/` (the AU equivalent
   is irrelevant — Dorico discovers VST3 only).
2. Launch Dorico 6 (or current version). If first-run after install,
   wait for VST3 rescan; otherwise `Preferences → Play → VST Plug-ins
   → Rescan` to surface the bundle.
3. `File → New from Template → Solo → Piano`.

**Route Dorico → O-MicrotonalSampler:**

4. `Play` mode. In the Endpoint Setup panel (top right), open the
   Piano track's VST slot. Replace HSSE with **Ouaricon Audio Development
   → O-MicrotonalSampler** (or `O-MicrotonalSampler-dev` if testing the
   dev bundle).
5. In the plugin GUI: load a sample folder covering at least one
   octave around C4 (any prepared fixture set with multi-velocity
   layers).

**Configure quarter-tone tonality + expression-map override:**

6. `Library → Tonality Systems…` → add **24-EDO Equal Temperament**
   preset to the project (or any preset including ¼♯/¼♭
   accidentals).
7. Right side, Key Signatures panel → set the active flow's key
   signature to a 24-EDO key (C major 24-EDO is fine).
8. `Library → Expression Maps…` → duplicate the **Default** map →
   rename to e.g. *"O-MicrotonalSampler Smoke (NE)"* → set
   Microtonality method to **"VST3 Note Expression"** (per Spike 002,
   this is the critical setting Dorico uses to route NE events). Save.
9. `Play` mode → Endpoint Setup → on the Piano channel, change the
   Expression Map dropdown to *"O-MicrotonalSampler Smoke (NE)"*.

**Test passage:**

10. `Write` mode. Enter four notes in bar 1 on the Piano staff:
    - Beat 1: C4 quarter (12-TET reference)
    - Beat 2: C4 quarter with **¼♯** accidental
    - Beat 3: C4 quarter (back to reference)
    - Beat 4: C4 quarter with **¼♭** accidental
11. Play the passage.

**Acceptance criteria (smoke = pass / fail, no measurement):**

- [ ] Audio plays — no crash, no silence, no AU revalidation prompt
      (well, this is VST3 — equivalent: no plugin-rescan dialog).
- [ ] Beats 2 and 4 are **audibly different in pitch** from beats 1
      and 3 (the quarter-tone offset is unmistakable to a trained ear).
- [ ] No clicks / zipper / glitches at the accidental boundary.
- [ ] No CPU dropouts during sustained playback.
- [ ] Plugin GUI's tuning-state readout reflects the active Ouaricon
      tuning (12-EDO if the active selection is left default — note
      that **Dorico's tonality system is independent** of
      O-MicrotonalSampler's APVTS tuning selection; the NE events
      override on a per-note basis).

**Optional rigour (only if smoke ambiguous):**

- Insert a tuner plugin in Dorico's effect chain on the Piano channel
  and read the cents offset on each note. Expected: 0¢ / +50¢ / 0¢ /
  -50¢. (Per Spike 002 Method A.)

### Pitfalls

- **Dorico's "Auto" expression-map mode silently uses pitch bend** for
  non-Steinberg VST3 instruments. If you skip step 8 and leave
  Microtonality on "Auto", Dorico will route pitch-bend, which
  O-MicrotonalSampler does not consume → no quarter-tone offset and
  the smoke will fail with a misleading "12-TET output" symptom.
  Step 8 is mandatory.
- **Don't drop a `.doricoexpmap` into Dorico's User folder** — silent
  no-op (memory `critical_dorico_distribution_mechanism.md`).
  Configure via UI per steps 6–9.
- **Dorico picks the upper or lower neighbour** for quarter-tone
  notation (Spike 002 Surprise 1: ¼♯C → MIDI 61 with NE = -50¢, not
  MIDI 60 with +50¢). The plugin does not care because pitch correlation
  is via `noteId`, not MIDI pitch — but the audible result is the same
  sounding pitch regardless of representation.
- **Don't expect `INoteExpressionController` to be queried.** Dorico 6
  ignores it (Spike 002 Surprise 2). The expression-map flag is the
  only signal Dorico responds to. Our NEC stays in for other VST3
  hosts.
- **The dev-suffix product name** (`O-MicrotonalSampler-dev`)
  registers as a separate VST3 from the release name. If the goal is
  to smoke the gate-time bundle, ensure the bundle installed to
  `~/Library/Audio/Plug-Ins/VST3/` is the one Dorico discovers.

---

## Module reuse — none new for Stage 4

Stage 4 introduces **no new module dependencies**. Existing usage
remains:

- `tuning/note-expression` — already integrated; powers RQ4-4
  smoke. No changes.
- All Stage 1–3 modules (APVTS, WebSlider relay, etc.) — unchanged.

No `/module-add` or `/module-create` actions required.

---

## Pitfalls / invariants summary (carry into PLAN)

From CONTEXT.md plus this research:

1. **`setLatencySamples` invariant** — only the comment-only hit at
   `PluginProcessor.cpp:133` may exist. PERF-04. Re-grep at gate.
2. **No edits to `MicrotonalSamplerVoice.{h,cpp}`,
   `LoopDetector.{h,cpp}`, `SampleLoader.cpp` audio-thread paths** —
   Stage 4 is bounded to (a) version-string plumbing in editor and
   (b) measurement.
3. **Cross-platform WebView memory pattern intact** —
   `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
   + `withUserDataFolder` (Windows guard) + path-equality resource
   provider. Even though Windows is out of v1.0 release scope, the
   build flags must remain correct — re-grep at gate per CONTEXT
   verification bar.
4. **Pluginval flag form is `--strictness-level`, not `--strictness`** —
   prior gate runs were silent strictness-5; correct form is mandatory
   for true strictness-10 coverage at Stage 4.
5. **Pluginval seed pinning** — add `--random-seed` + `--timeout-ms
   120000` to the gate command. Stage 3 did not pin; Stage 4 must,
   given `FuzzParametersTest` is now active.
6. **Apple Silicon power state** — PERF-02 measurement must be on
   power, never battery. Document in VERIFICATION.
7. **Dorico expression-map override** — smoke is invalid unless the
   active expression map sets Microtonality to "VST3 Note Expression".
   Auto-mode silently drops to pitch-bend.
8. **No hard-coded `v0.1.0` in `index.html` after 4.1 lands.** Empty
   `<div id="about-version"></div>` defensively renders if the native
   function fails.

---

## Open items for `/plugin-plan`

None. All four research questions resolved. PLAN.md can proceed to
formalise the four sub-stage tasks against the measurement protocols
and gate-command forms above.
