---
plugin: O-MicrotonalSampler
stage: 4-polish
phase: execute (Phase 4.1 [b47434d] + Phase 4.2 [this commit] shipped; ready for Phase 4.3 listening pass)
status: phase_4_2_complete_with_methodology_deviation; PERF-02 flipped complete (conditional on 4.4 pluginval-10); ready_for_phase_4_3
last_updated: 2026-04-28
---

# Resume Point

## Current State: Phase 4.2 PERF-02 closed (methodology deviation)

`/plugin-execute O-MicrotonalSampler 4-polish` Phase 4.2 produced
`.planning/stages/4-polish/{VERIFICATION,PHASE-4.2-SUMMARY,gate-report}.{md,json}`
and flipped `REQUIREMENTS.md` row `PERF-02` from `partial → complete`
(verified at `stage-4`, with the methodology-deviation caveat carried
in the row's `verified at` field for surface-level visibility).

### Phase 4.2 deviation summary

The spec metric (Logic Pro Performance Meter `delta_CPU_pct ≤ 5 %`)
was unmeasurable: Logic 11.x's Performance Meter is not surfaceable
in this user's environment (window restructured / removed; LCD
mini-meter not visible). Path B taken (per pre-execute discuss):

- **Activity Monitor used as supporting headline only.** One run on
  M4 Max laptop on power, 16 voices / 48 kHz / 256: ~16 % of one
  core ≈ ~1 % of total system CPU on the 16-core part. Well below
  the 5 % spec budget at the system level.
- **Objective per-block timing budget = `pluginval --strictness-level
  10` in Phase 4.4** — gate-of-record. Strictness-10 stress includes
  timing constraints + fuzzed parameter sequences, an objective and
  reproducible substitute for the Logic-side metric.
- **Conditional flip.** PERF-02 → `complete` on the basis of (1)
  Activity Monitor headline (2) PERF-01 RT-safety precondition
  (already verified stage-2) and (3) deferred objective gate to 4.4.
  If 4.4 strictness-10 surfaces a timing regression, the flip rolls
  back and Stage 2 sub-phase 2.4 / 2.5 reopens per `PLAN.md
  §Failure Routing`.
- **v1.1 follow-up logged.** Capture Logic-side metric on a future
  Logic release (or alternative DAW with stable per-track meter, e.g.
  Reaper) once one is available.

### Phase 4.2 commit also backfills Stage 4 planning prerequisites

`CONTEXT.md`, `RESEARCH.md`, `PLAN.md` were on-disk-but-untracked
from the discuss/research/plan phases (never landed in their own
commits). VERIFICATION.md and PHASE-4.2-SUMMARY.md reference these
documents (RESEARCH §RQ4-3 in particular), so they're brought into
tree alongside the 4.2 deliverables to keep cross-references live.

## Stage 4 Sub-stage Status

| Phase | Goal | Gate | Commit | Status |
|---|---|---|---|---|
| 4.1 Version pill | runtime version-pill via `getPluginVersion` | triple build + pluginval-5 + auval | b47434d | ✅ PASS |
| 4.2 PERF-02 | 16-voice CPU budget within spec | methodology-deviation; objective gate-of-record = 4.4 | this commit | ✅ PASS (conditional) |
| 4.3 QUAL-01 | listening pass (no clicks / zipper / aliasing) | 7-item subjective checklist | pending | ⏳ next |
| 4.4 Final gate | pluginval-10 + auval + Logic + Dorico smoke + invariants | strictness-10 SUCCESS, all greens | pending | ⏳ |

## Previous State: Phase 2.5 reopen RESOLVED — full chromatic playback verified

Case A (the "only D#3/E3 audible" symptom) was a **two-bug interaction**
surfaced by audit, not the load-pipeline corruption hypothesized in the
charter. Both bugs fixed in this commit; engineering bar green; user
perceptual verification PASS in Standalone against both test folders.

### Bug 1 — `cubicInterp` always-wrap defect (primary cause of silence)

`Source/MicrotonalSamplerVoice.cpp` — the cubic-Hermite interpolator
unconditionally folded read indices into the loop region whenever a slot
had `loopMode = Auto`. Effect: at note-on with `pos = 0`, the four taps
came from `buf[loopEnd-1, loopStart, loopStart+1, loopStart+2]` instead
of `buf[0..3]`. The entire attack `[0, loopStart)` was never played —
the sampler started **inside** the loop region from sample 0.

Combined with `LoopDetector::detectLoop`'s search for the **quietest**
1024-sample window (which is exactly what a loop detector should look
for), every slot whose variance gate passed rendered near-silent through
the ADSR fade-in. Slots whose variance gate rejected (→ `OneShot`) played
correctly because the no-wrap path was already correct.

User-confirmed: every silent cell in the test had `loopMode: Auto`,
every audible cell had `loopMode: OneShot`.

**Fix:** `cubicInterp` is now pure clamp (loopStart/loopEnd parameters
removed). Looping is the protocol of `readSlotWithLoop` (boundary
crossfade) plus `wrapLoopPosition` (cursor reset). Since
`wrapLoopPosition` only fires when `pos >= loopEnd`, the first pass
naturally plays `[0, loopEnd)` (attack + body) and subsequent passes
oscillate in `[loopStart, loopEnd)`.

The crossfade `inSample` math was also corrected — it previously read
the END of the loop region (degenerate, identical to `outSample`); now
reads the loop HEAD at `pos - lpLen + 8 ∈ [lpStart, lpStart + 8)` as
intended.

### Bug 2 — `FilenameParser` pre-note dynamics false-match

`Source/FilenameParser.cpp` — the velocity scan walked all tokens
left-to-right and accepted the first match. Filenames like
`vln_long_mp-D#3-V127-T6N6.aif` matched `mp` (a dynamics token) at
token index 2 — BEFORE the note token at index 3 — and assigned
`velLayer=1` to every slot in the library. Combined with
`numVelocityLayers = jlimit(1, 4, maxLayer + 1) = 2` and
`layerWidth = 64`, this silenced the entire library at MIDI velocities
< 65 (the layer-1 threshold for a 2-layer map).

**Fix:** velocity scan is now two-tier:
- **Tier 1 (post-note):** any velocity form, including dynamics. Handles
  the conventional `[note]_[dyn]` and `[note]_[v_N]` patterns.
- **Tier 2 (pre-note):** explicit forms only (`v[1-4]` / `vel[1-4]` /
  `layer[N]` / `L[N]` / `lyr[N]`). Dynamics letters (`p`/`mp`/`mf`/`f`)
  are skipped here because they collide with instrument-name fragments.

`vln_long_mp-D#3-…` now resolves to `velLayer=0`. Existing pre-note
explicit conventions (`Lyr3_C4`, `L4_C4`, `vel2_C4`) still resolve
correctly. New regression test cases added under `OMTS_UNIT_TESTS`.

### Engineering bar (post-fix, against installed `~/Library/Audio/Plug-Ins/`)

- triple build (VST3 + AU + Standalone): GREEN
- cache-clear + reinstall per CLAUDE.md
- `pluginval --strictness-level 10 --validate-in-process --skip-gui-tests --timeout-ms 120000`: SUCCESS
- `pluginval --strictness-level 10 --validate-in-process --timeout-ms 120000` (with-GUI): SUCCESS
- `auval -v aumu OMtS OuDv`: AU VALIDATION SUCCEEDED

### User perceptual verification (Standalone, fixed-velocity on-screen keyboard)

- `~/Documents/samples/vlnsolo_flaut/` (42 .aif, "Auto Sampled Instrument-…"
  naming, parser-clean velLayer=0): full chromatic G2..C6 audible. ✓
- `~/Documents/samples/vln_long_mp/` (42 .aif, "vln_long_mp-…" naming,
  previously snagged by Bug 2): full chromatic at any velocity now audible. ✓

## Stage-4 resume condition

Both reopens closed. Resume from **Stage 4 Phase 4.2** (PERF-02 Logic Pro
CPU meter measurement) per `.planning/stages/4-polish/PLAN.md`. The
`CASE-A-AUDIT-CHARTER.md` can be archived — no fresh-context deep audit
needed.

## Previous State: Phase 2.1 reopened + rectified mid-Stage-4

While prepping for Phase 4.2 (Logic Pro CPU meter measurement), the
user loaded a real sparse sample folder (`vln_long_mp-A#2-V127-T6N6.aif`
× 43 files) into the fixture-OFF binary and surfaced two Stage 2
defects that the in-memory test fixture had been masking:

- **DEF-2.1-R1 (FUNC-04):** `SampleMap::findSlot` was exact-MIDI-match
  only — the spec's "or nearest if N is unsampled" clause
  (`REQUIREMENTS.md:80`) was never implemented. Sparse-folder keys
  silenced.
- **DEF-2.1-R2 (FUNC-03):** the `polyphony` APVTS parameter was wired
  through APVTS + WebSlider but never read by the audio engine. The
  cap had no effect.

Per the PLAN failure-routing table, Phase 2.1 was reopened. Both
fixes shipped in one atomic commit. Engineering bar green:

- pluginval `--strictness-level 10 --skip-gui-tests`: SUCCESS (21 tests, 0 fail)
- pluginval `--strictness-level 10` with-GUI: SUCCESS (25 tests, 0 fail)
- `auval -v aumu OMtS OuDv`: AU VALIDATION SUCCEEDED

See `.planning/stages/2-dsp/PHASE-2.1-REOPEN-SUMMARY.md` for the full
defect + fix narrative. REQUIREMENTS rows FUNC-03 and FUNC-04 annotated
with `(rectified stage-4 phase-2.1 reopen ...)`.

**Next up — user-side perceptual verification** (closes the reopen
and unblocks Stage 4 Phase 4.2):

- [ ] Single-note coverage across the full range (sparse folder)
- [ ] 16-note chord rings 16 voices; cap of 4 rings 4 with smooth steals
- [ ] Voice-steal ramp inaudible at moderate velocity (D2-3 regression)

If all three pass: resume at Stage 4 Phase 4.2 (PERF-02 Logic Pro
CPU meter run). If any fail: file the defect against the responsible
sub-phase per PLAN failure-routing table.

## Previous State: Stage 4 PLAN complete — ready for execute

`/plugin-plan O-MicrotonalSampler 4-polish` produced
`.planning/stages/4-polish/PLAN.md` with **20 numbered tasks** organised
across the four sub-stages locked in CONTEXT (4.1 version-pill →
4.2 PERF-02 → 4.3 QUAL-01 → 4.4 final gate). Each sub-stage carries an
atomic commit + `gate-report.json` + `PHASE-4.N-SUMMARY.md`, matching
Stage 2/3 cadence.

**Sub-stage breakdown:**

- **4.1 (Tasks 1–4):** insert `getPluginVersion` native function in
  `PluginEditor.cpp` between `getOctaveStretch` (line 127) and
  `getEmbeddedTuningList` (line 137); HTML strips hard-coded `v0.1.0`
  to empty `<div id="about-version">`; JS adds `refreshAboutVersion`
  modeled on `refreshTuningReadout`, called once at JUCE-init alongside
  the existing tuning-readout call. Phase 4.1 gate = triple build +
  cache-clear+install + visual confirmation `v1.0.0` in About pill +
  no-literal grep + pluginval-5 + auval.
- **4.2 (Tasks 5–8):** Logic Pro CPU-meter measurement per RQ4-3
  protocol — Apple Silicon on power, 48 kHz / 256 buffer, 16-voice
  held chord, 3 runs, delta-from-baseline-with-transport, plus
  microtonal-stretch confirmatory read. Acceptance:
  `delta_CPU_pct ≤ 5 %` flips PERF-02 to `complete`.
- **4.3 (Tasks 9–11):** QUAL-01 listening checklist (sustained sine,
  cello vibrato, transient, ±50 c retune sweep, voice-steal stress,
  mixed-SR fixture, short-region loop edge case). All-pass flips
  QUAL-01 to `complete`; any fail reopens Stage 2 sub-phase.
- **4.4 (Tasks 12–20):** final stage gate — clean triple build,
  `pluginval --strictness-level 10 --validate-in-process
  --skip-gui-tests --random-seed 0xC0FFEE --timeout-ms 120000` then
  same with-GUI; auval; Logic AU smoke; Dorico microtonal smoke
  (C4 / ¼♯C4 / C4 / ¼♭C4 with Microtonality="VST3 Note Expression");
  invariant greps (latency-zero, WebView2 flags, no `v0.1.0`); final
  VERIFICATION.md + STATUS.md update; atomic commit.

**Strict order: 4.1 → 4.2 → 4.3 → 4.4.** A failure in 4.2 / 4.3
**reopens** the relevant Stage 2 sub-phase rather than absorbing into
Stage 4. Only 4.4 closes the stage.

**Dependency graph + failure routing** documented in PLAN.md (per-task
defect routes back to Stage 2/3 sub-phase ownership).

**Files modified at execute time:**
`Source/PluginEditor.cpp`, `Resources/ui/index.html`,
`Resources/ui/js/sampler-app.js`, `.planning/REQUIREMENTS.md`,
`.planning/STATUS.md`, `.planning/stages/4-polish/{VERIFICATION,
PHASE-4.{1,2,3,4}-SUMMARY,gate-report}.{md,json}`. **Untouched:**
all Stage 2 audio-thread paths, CMakeLists.txt, modules.json.

## Previous State: Stage 4 RESEARCH complete — ready for plan

`/plugin-research O-MicrotonalSampler 4-polish` produced
`.planning/stages/4-polish/RESEARCH.md`, resolving all four open
questions from CONTEXT (RQ4-1 .. RQ4-4):

- **RQ4-1** — `JucePlugin_VersionString` is a compile-time string-literal
  macro, available via `<JuceHeader.h>`. Sibling Ouaricon plugins
  (O-FreqPulse:215, O-DigiDelay:125, O-Tremolo:122) all use the same
  `withNativeFunction("getPluginVersion", ...)` shape. Pattern to
  mirror verbatim. Build's actual value confirmed `"1.0.0"` from
  `Defs.txt` / `CMakeLists.txt:14`.
- **RQ4-2** — Correct flag is `--strictness-level`, NOT `--strictness`.
  Prior Stage 3 runs were silent strictness-5 fallback (flag
  malformed). Strictness-10 unlocks `FuzzParametersTest` + heavier
  `ParameterThreadSafetyTest` / `BackgroundThreadStateTest`. No prior
  strictness-10 evidence in this codebase — Stage 4 is first run.
  Plan to pin `--random-seed` + `--timeout-ms 120000` for
  reproducibility.
- **RQ4-3** — Logic Pro per-track CPU is not isolated; PERF-02 measures
  as **delta from baseline-with-transport** in the aggregate
  Performance Meter. Apple Silicon must be on power. Specified
  reproducible 8-step protocol with VERIFICATION fields.
- **RQ4-4** — Dorico smoke procedure: 11-step manual UI configuration
  (no `.doricoexpmap` distribution needed for smoke). Critical step:
  duplicate Default expression map and set Microtonality to **"VST3
  Note Expression"** — Dorico ignores `INoteExpressionController` and
  Auto-mode silently routes to pitch-bend. Test passage:
  C4 / ¼♯C4 / C4 / ¼♭C4 quarter-tone alternation.

**No new module dependencies.** Eight invariants/pitfalls carried
forward into PLAN.

## Previous State: Stage 4 DISCUSS complete — ready for research

`/plugin-discuss O-MicrotonalSampler 4-polish` produced
`.planning/stages/4-polish/CONTEXT.md` with **7 locked decisions
(D4-1..D4-7)** and a **provisional 4-sub-stage plan** (4.1 version
plumbing → 4.2 PERF-02 benchmark → 4.3 QUAL-01 listening → 4.4 final
gate).

**Stage 4 scope (intentionally narrow):**
- Close PERF-02 (16-voice ≤ 5 % CPU) and QUAL-01 (no artifacts) — both
  carry `partial` from Stage 2.
- Plumb dynamic version pill via `getPluginVersion()` native function
  (replaces hard-coded `v0.1.0` in About tab).
- Final gate: pluginval `--strictness 10` + auval + Logic + Dorico
  smoke. macOS-only (VST3 + AU + Standalone). Internal use; no signing,
  no installer, no public release.

**Out of v1.0:** preset system, installers, Windows build, per-slot
xfade, octave grouping, render-harness target.

**4 open questions (RQ4-1..RQ4-4)** pending research:
- JucePlugin_VersionString macro source / runtime accessor
- pluginval `--strictness 10` delta vs strictness-5 for WebView editors
- Logic CPU-meter measurement protocol (per-track vs delta)
- Dorico smoke procedure (Playback Template / endpoint mapping)

## Previous State: Stage 3 VERIFIED

`/plugin-verify O-MicrotonalSampler 3-gui` produced
`.planning/stages/3-gui/VERIFICATION.md`. All five Stage 3 requirements
(FUNC-05, FUNC-06, DSP-06, UI-01, UI-02) marked **complete** in
`REQUIREMENTS.md`. All five sub-stage gates green; all 11 Phase 3.5 gate
criteria green; Stage 2 audio invariant intact end-to-end.

Stage 3 (GUI) is closed.

## Previous State: Phase 3.5 GATE PASS — Stage 3 EXECUTE COMPLETE

`/plugin-execute O-MicrotonalSampler 3-gui` Phase 3.5 produced
`.planning/stages/3-gui/PHASE-3.5-SUMMARY.md` and overwrote
`gate-report.json` (phase 3.5). Tasks 29–34 implemented. **All five Stage 3
sub-stage gates green.**

**Phase 3.5 deliverables:**
- Bottom control strip rebuilt as 7 SVG arc-knobs (44 px, 270 deg sweep,
  antique-gold vine, rosewood track) — lifted from O-Bells `#effects-tab .knob`
  ruleset. Each knob wraps a hidden `<input type="range">` so the existing
  WebSliderRelay binding (Phase 3.1) is preserved verbatim — relay attaches by
  element id, not DOM hierarchy. Order left→right: Attack · Decay · Sustain ·
  Release · Polyphony · Vel-XF · Out Gain.
- `KNOB_FORMATS` table maps each relay to per-parameter display range + unit
  suffix + formatter (Attack/Decay/Release in seconds with adaptive precision,
  Sustain/Vel-XF unitless 0..1, Polyphony integer-rounded, Out Gain in dB
  with sign).
- Pointer drag = relative-vertical (200 px = full sweep, sliderDragStarted/Ended
  bracketing); wheel = 2 % per tick; dblclick = snap to mid (Stage 4 will plumb
  parameter defaults explicitly).
- Tuning-state readout in chrome (`<span id="tuning-readout">`) already present
  from Phase 3.1; verified poll cadence honours RP3-3 (editor open +
  Tuning-tab activation only, no background interval).
- About tab populated (RP3-4): `.about-card` with plugin name (Garamond
  serif heading), version pill `v0.1.0` (hard-coded; Stage 4 will plumb
  dynamically from CMakeLists.txt PLUGIN_VERSION), tagline "Microtonal
  sample engine for Dorico microtonal playback", short blurb, Ouaricon
  license link (`https://ouaricon.com`).
- Aesthetic polish: 8/16/24 px spacing scale enforced via `--gap-sm/md/lg`
  CSS vars; hover states on cells, buttons, knobs, tabs, links; warm-card
  shadows + `--border-warm` border treatments matching O-Bells convention;
  Garamond serif for headings + system sans for numeric readouts.
- Narrow-window guard: `checkNarrowWindowGuard` auto-closes the loop editor
  + toasts "Resize wider to use the loop editor." when window width crosses
  below 900 px with the panel open. ResizeObserver-driven; one-shot per
  bucket transition (no spam).

**Gate (Tasks 33 + 34):**
- Triple build (VST3 + AU + Standalone) GREEN.
- Cache-clear + reinstall per CLAUDE.md.
- `pluginval --strictness 5 --validate-in-process --skip-gui-tests`: **SUCCESS**.
- `pluginval --strictness 5 --validate-in-process` (with GUI tests): **SUCCESS**.
- `auval -v aumu OMtS OuDv`: **AU VALIDATION SUCCEEDED**.
- Latency invariant: `grep -rn setLatencySamples plugins/O-MicrotonalSampler/Source/`
  returns one comment-only hit (PluginProcessor.cpp:133) — no actual calls.
  Stage 2 latency-zero contract preserved end-to-end across Stage 3.

## Previous State: Phase 3.4 GATE PASS

`/plugin-execute O-MicrotonalSampler 3-gui` Phase 3.4 produced
`.planning/stages/3-gui/PHASE-3.4-SUMMARY.md` and overwrote
`gate-report.json` (phase 3.4). Tasks 23–28 implemented.

**Phase 3.4 deliverables:**
- `OMicrotonalSamplerAudioProcessor::overrideLoopPoints` full impl
  (atomic deep-copy via `std::make_shared<SampleMap>(*current)` + slot
  mutation + version bump + callback). Manual override sets
  `LoopMode::Manual`; `resetToAutoDetect=true` re-runs
  `LoopDetector::detectLoop` and writes `Auto` (valid) or `OneShot`
  (invalid).
- New `OMicrotonalSamplerAudioProcessor::resetLoopToAutoDetect(midi, vel)`
  convenience wrapper.
- `OMicrotonalSamplerAudioProcessor::snapshotWaveformPeaks` full impl —
  per-bin min/max scan over slot audio, `framesPerBin = numFrames/bins`,
  sum-of-channels mixdown; emits `juce::DynamicObject` with peaks +
  meta per RESEARCH §RQ3-5 schema (midiNote, velocityLayer,
  lengthSamples, sourceSampleRate, loopStart, loopEnd, loopMode,
  filename, peaks). Single-pass O(N), ≈1 ms / 5 s sample at 48 kHz.
- Three native function skeletons replaced with full impls in
  `PluginEditor.cpp`: `getWaveformPeaks(midi, vel, bins=512)`,
  `overrideLoopPoints(midi, vel, start, end, xfade=8)`,
  `resetLoopToAutoDetect(midi, vel)`.
- `Resources/ui/index.html` — `#loop-editor-panel` populated (header
  with filename · MIDI · L<vel> + close X, canvas wrap, meta row,
  Reset/Cancel/Apply actions).
- `Resources/ui/css/sampler-shell.css` — full panel slide-in (350 ms
  ease, `body.le-open` grid reflow with `padding-right: calc(...
  + 360px)`); canvas sized via `width: calc(100% - 0px); height: 200px`
  (memory pitfall #6 — never `position: absolute` with `left+right`).
- `Resources/ui/js/sampler-app.js` — loop-editor module:
  `openLoopEditor(midi, vel)` async fetch + render; `redrawLoopEditor`
  with DPR-aware backing store + warm-brown stroke + antique-gold fill
  envelope + draggable markers (gold start, rust-red end, 8-px
  hit-tolerance, 16-sample min gap); pointer-event drag with
  `setPointerCapture`; Apply emits toast `"New loop points apply to
  next note-on."` (EC3-6); Reset disabled + tooltip when one-shot
  (EC3-7); Esc/X/Cancel close; `ResizeObserver` re-renders on canvas
  size change.
- `handleCellSingleClick` for cell-loaded now calls `openLoopEditor`
  (replaces Phase 3.2 placeholder); context menu open-loop-editor
  routes to same.
- `handleSampleMapSnapshot` syncs editor state when open + not
  mid-drag so loop-mode label stays consistent after Apply.

**Gate:** triple build green, cache-clear+install per CLAUDE.md, pluginval --strictness 5 SUCCESS, auval AU VALIDATION SUCCEEDED.

## Previous State: Phase 3.3 GATE PASS

`/plugin-execute O-MicrotonalSampler 3-gui` Phase 3.3 produced
`.planning/stages/3-gui/PHASE-3.3-SUMMARY.md` and overwrote
`gate-report.json` (phase 3.3). Tasks 19–22 implemented.

**Phase 3.3 deliverables:**
- `OMicrotonalSamplerAudioProcessorEditor::filesDropped` full hit-test + routing
  (cell + audio file → `loadSingleSample`; folder-zone + folder → `loadSampleFolder`;
  invalid combinations → toast hints; out-of-bounds → silent reject; EC3-3 folder-on-cell disallowed)
- `fileDragEnter/Move/Exit` emit `hostFileDragMove({x,y})` / `hostFileDragExit({})`
  events for JS hover visuals
- `loadSampleFolderDialog` native function full impl (FileChooser canSelectDirectories
  → `processorRef.loadSampleFolder`)
- JS `bindHostDragEvents` toggles `.drag-over` class on `#folder-drop-zone` based on (x,y) ∈ rect
- Folder-button enabled (was disabled placeholder in 3.1) — click → `loadSampleFolderDialog`
- 3-second single-element toast queue (`showToast` + backend `toast` event subscription)
- Skipped-files disclosure: `<ul id="issues-list">` rendered from `snapshot.skippedFiles`
  on every `sampleMapUpdated`; transition-tracked toast `"N files skipped"` on set change
- CSS `.drag-over` glow strengthened with inset box-shadow

**Gate:** triple build green, cache-clear+install per CLAUDE.md, pluginval --strictness 5 SUCCESS, auval AU VALIDATION SUCCEEDED.

## Previous State: Phase 3.2 GATE PASS

`/plugin-execute O-MicrotonalSampler 3-gui` Phase 3.2 produced
`.planning/stages/3-gui/PHASE-3.2-SUMMARY.md` and overwrote
`gate-report.json` (phase 3.2). Tasks 12–18 implemented.

**Phase 3.2 deliverables:**
- `SampleLoader::loadSingleSlot` worker (SR-convert + loop-detect + async completion)
- `OMicrotonalSamplerAudioProcessor::loadSingleSample` full impl (atomic deep-copy + version bump + callback)
- `loadSingleSampleDialog` native function (FileChooser launch)
- `renderGrid` JS — 88×4 CSS grid, cell-loaded/empty/loading classes
- Cell interactions (RP3-1): single-click empty → FileChooser; single-click loaded → loop-editor placeholder; double-click loaded → replace; right-click → context menu
- 250 ms double-click discrimination
- `publishCellLayout` (ResizeObserver + rAF-throttled) → `reportCellLayout` native function

**Gate:** triple build green, cache-clear+install per CLAUDE.md, pluginval --strictness 5 SUCCESS, auval AU VALIDATION SUCCEEDED.

## Previous State: Phase 3.1 GATE PASS

`/plugin-execute O-MicrotonalSampler 3-gui` Phase 3.1 produced
`.planning/stages/3-gui/PHASE-3.1-SUMMARY.md` and `gate-report.json`.

**Phase 3.1 Foundation delivers:**

- WebView shell replaces the Phase 2.2 placeholder editor wholesale.
- 7 APVTS sliders (`attack`, `decay`, `sustain`, `release`, `polyphony`,
  `velocity_crossfade`, `output_gain`) bound via `WebSliderRelay` +
  `WebSliderParameterAttachment` in correct destruction order.
- Tabbed UI (Sample Map / Tuning / About) with read-only TuningPanel
  (verbatim O-Bells carry + readonly CSS overlay + interval-input → span
  swap shim per RESEARCH §RQ3-1).
- 8 fully-implemented native functions (`getSampleMap`, `getTuningName`,
  `getTuningIntervals`, `getTonicNote`, `getOctaveStretch`,
  `getEmbeddedTuningList`, `getEmbeddedTuningCategories`, `reportCellLayout`,
  `getSkippedFiles`) + 6 skeletons returning sane defaults
  (`loadSampleFolderDialog`, `loadSingleSampleDialog`, `overrideLoopPoints`,
  `resetLoopToAutoDetect`, `getWaveformPeaks` for 3.2/3.3/3.4).
- `sampleMapUpdated` event scaffold: processor's
  `setSampleMapChangedCallback` lambda emits the JSON snapshot whenever the
  sample map atomic-stores; editor wires up the lambda on construction.
- Cross-platform WebView2 compliance: `NEEDS_WEBVIEW2 TRUE` +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder` +
  resource provider URL=path equality.
- Stage 2 invariant addition (per RESEARCH §RQ3-3): `SampleSlot::audio` →
  `std::shared_ptr<juce::AudioBuffer<float>>`; `SampleSlot::filename`;
  `LoopMode` enum + `SampleSlot::loopMode`; `SampleMap::version` monotonic
  counter.

**Stage 2 regression gate (Task 4):** `pluginval --strictness 5
--validate-in-process --skip-gui-tests` SUCCESS + `auval -v aumu OMtS OuDv`
AU VALIDATION SUCCEEDED on the post-shared_ptr-swap build. No render-harness
existed; coverage substituted by pluginval+auval per gate-report advisory.

**Phase 3.1 gate (Task 11):** Triple build green. Cache-clear + install per
CLAUDE.md. pluginval SUCCESS. auval SUCCEEDED. Atomic commit recipe
documented in PHASE-3.1-SUMMARY.md.

## Stage 3 Sub-stage Status

| Phase | Goal | Gate | Commit | Status |
|---|---|---|---|---|
| 3.1 Foundation | WebView shell + Stage 2 invariant + relays + JSON broadcast | infra | d1a0d7a | ✅ PASS |
| 3.2 Grid | FUNC-06, UI-01 | grid in <100 ms; per-cell replace | 4083582 | ✅ PASS |
| 3.3 Folder Drop | FUNC-05 | drop = button parity; skipped files surface | aa99790 | ✅ PASS |
| 3.4 Loop Editor | DSP-06, UI-02 | edit → audible diff on next note-on | d7cfd29 | ✅ PASS |
| 3.5 Polish | (visual) | aesthetic + final pluginval gate | pending atomic commit | ✅ Code + automated gate green |

## Previous State: Stage 3 (GUI) PLAN complete

`/plugin-plan O-MicrotonalSampler 3-gui` produced
`.planning/stages/3-gui/PLAN.md` with **34 numbered tasks** organized
across 5 sub-stages (3.1 Foundation → 3.2 Grid → 3.3 Folder Drop →
3.4 Loop Editor → 3.5 Polish), each with its own atomic-commit gate.

**Plan-phase resolutions (open questions RP3-1..RP3-5):**

- **RP3-1** Cell interactions: single-click loaded cell → loop editor;
  double-click → replace via FileChooser; right-click → context menu;
  single-click empty cell → FileChooser.
- **RP3-2** Crossfade-length stays global (Phase 2.5 constant) for
  v1.0; per-slot xfade is a v1.1 candidate.
- **RP3-3** Tuning-state readout polls on Tuning-tab activation +
  editor open only (no background interval).
- **RP3-4** About tab: empty in 3.1; minimal version + license link
  in 3.5.
- **RP3-5** Narrow-window grid: horizontal scroll when min cell width
  (8 px) is hit; no octave grouping in v1.0.

**Critical sequencing note:** Phase 3.1 includes a Stage 2 invariant
addition (`SampleSlot::audio` → `std::shared_ptr<juce::AudioBuffer<float>>`).
Task 4 blocks on a full Stage 2 verification gate (pluginval, auval,
render-harness identity test) before proceeding to editor work — any
regression reopens Stage 2 rather than being absorbed into 3.1.

## Previous State: Stage 3 (GUI) RESEARCH complete

`/plugin-research O-MicrotonalSampler 3-gui` produced
`.planning/stages/3-gui/RESEARCH.md` resolving all 8 research questions
(RQ3-1..RQ3-8). Key resolutions:

- **TuningPanel readonly mode** (RQ3-1): carry verbatim suite copy + CSS
  overlay + register only read-side native functions.
- **SampleMap JSON schema** (RQ3-2): version-stamped snapshot with per-slot
  filename/length/SR/loopStart/loopEnd/loopMode + skippedFiles array.
- **Per-cell loader** (RQ3-3): new `loadSingleSample(midi, vel, file)` —
  requires Stage 2 invariant addition `SampleSlot::audio` →
  `std::shared_ptr<juce::AudioBuffer<float>>` to keep map deep-copy cheap
  on per-cell replace. Land in 3.1.
- **Loop-override** (RQ3-4): `overrideLoopPoints(midi, vel, start, end,
  xfade)` on message thread, atomic shared_ptr replace, snapshot
  rebroadcast. Voices keep their own snapshot for active notes.
- **Waveform render** (RQ3-5): pre-render 512-bin peak summary on message
  thread, broadcast via `emitEventIfBrowserIsVisible("waveformPeaks", ...)`,
  JS draws on DPR-aware canvas.
- **Cell DnD** (RQ3-6): `juce::FileDragAndDropTarget` on host editor +
  C++-side cell-layout shadow published by JS via `reportCellLayout`
  native function. No reliance on HTML5 `dataTransfer.files` paths.
- **Aesthetic** (RQ3-7): pull palette/typography from O-Bells inline
  styles. Garamond serif, cream parchment + warm-brown + antique-gold +
  rust-red active. Botanical motif deferred to 3.5 polish.
- **Resource bundling** (RQ3-8): `juce_add_binary_data` baked, served via
  resource provider — matches O-Bells.

Stage 3 verifies 5 requirements: FUNC-05, FUNC-06, DSP-06, UI-01, UI-02.

Native function inventory: ~13 (`getSampleMap`, `loadSingleSampleDialog`,
`overrideLoopPoints`, `getWaveformPeaks`, `reportCellLayout`,
`getTuning*` reads, etc.).

Open RP3-1..RP3-5 for plan phase to resolve (single-click cell behavior,
crossfade-len global vs per-slot, tuning-readout polling cadence,
About-tab content, narrow-window cell clamp).

## Previous State: Stage 3 (GUI) DISCUSS complete

`/plugin-discuss O-MicrotonalSampler 3-gui` produced
`.planning/stages/3-gui/CONTEXT.md` with 15 locked decisions (D3-1..D3-15)
and 5 sub-stages (3.1 shell+tabs+TuningPanel → 3.2 sample-map grid →
3.3 folder-drop + skipped-files → 3.4 loop-point editor → 3.5 control
strip + aesthetic polish). 8 research questions resolved in RESEARCH.md.

**Key decisions:** WebView UI (D3-1), Ouaricon house aesthetic (D3-2),
no separate `/ui-mockup` pass (D3-3 — design specified in prose),
tabbed layout with TuningPanel as its own tab (D3-4 + D3-7 — copy-paste
the suite tuning-panel.{js,css} per O-Bells pattern), horizontal piano
strip × 4 vel-layer rows (D3-5), loop editor as side panel inside the
Sample Map tab (D3-6), 7 APVTS relays + custom `sampleMap` JSON relay
(D3-11). Cross-platform WebView2 flags from memory are mandatory.

Stage 3 verifies 5 requirements: FUNC-05, FUNC-06, DSP-06, UI-01, UI-02.

## Previous State: Stage 2 (DSP) VERIFIED

`/plugin-verify O-MicrotonalSampler 2-dsp` ran goal-backward analysis against
CONTEXT.md / PLAN.md / 5×PHASE-N-SUMMARY.md, walked all 15 in-scope requirements,
and re-ran the automated bar (triple build green; cache-clear + fresh install;
`pluginval --strictness 5 --validate-in-process --skip-gui-tests` SUCCESS;
`auval -v aumu OMtS OuDv` AU VALIDATION SUCCEEDED).

**Verdict:** ✅ VERIFIED — 13 requirements complete (FUNC-01..04, FUNC-07,
DSP-01..05, DSP-07, DSP-08, PERF-01, PERF-03, PERF-04, COMPAT-02), 2 marked
partial pending the user's subjective DAW pass (PERF-02 CPU benchmark; QUAL-01
listening test). All engineering mitigations for the partials are in place;
they remain open only because they require a human listener / metering step.

See `.planning/stages/2-dsp/VERIFICATION.md` for the full evidence table and
the deferred Human Verification checklist.

**Phase 2.5 commit still pending.** The Phase 2.5 source changes
(`LoopDetector.{h,cpp}`, modified `MicrotonalSamplerVoice.{h,cpp}`,
`SampleLoader.cpp`, `CMakeLists.txt`) plus the new verify artefacts (this
file, `REQUIREMENTS.md` updates, `VERIFICATION.md`) ride in a single atomic
commit per the recipe in `VERIFICATION.md` Outstanding Actions §1.

## Stage 2 Sub-stage Status

| Phase | Gate | Commit | Status |
|---|---|---|---|
| 2.1 Voice DSP | 1 | `bb0e7f7` | ✅ PASS |
| 2.2 Loader | 2 | `cacffda` | ✅ PASS |
| 2.3 Vel xfade | 3 | `11bd39c` | ✅ PASS |
| 2.4 Voice-steal | 4 | `1aceb4c` | ✅ PASS |
| 2.5 Loop detect | 5 | pending atomic commit | ✅ Code + automated gate green |

## Completed So Far

**Ideation:** ✓ Complete
**Stage 1 (Foundation):** ✓ Verified — silent shell builds + AU/VST3/Standalone validate
**Stage 2 Discuss:** ✓ Complete (CONTEXT.md, 2026-04-27)
**Stage 2 Research:** ✓ Complete (RESEARCH.md, 2026-04-27)
**Stage 2 Plan:** ✓ Complete (PLAN.md, 2026-04-27)
**Stage 2 Execute:** ✓ All 5 sub-stages code-complete; 4 committed, 5th pending atomic commit
**Stage 2 Verify:** ✓ VERIFIED (VERIFICATION.md, 2026-04-27)

## Stage 2 Locked Decisions (D2-1..D2-12)

- **D2-1 Interpolator:** Cubic-Hermite (4-pt). Conditional 1st-order tilt LPF NOT added (Phase 2.1 sine-sweep null test landed below threshold).
- **D2-2 Voice-steal:** JUCE default `findVoiceToSteal` already implements oldest-released → oldest-keyup → oldest-non-protected (R1; no override).
- **D2-3 Steal ramp:** 5 ms linear (`ceil(0.005·SR)+16` samples).
- **D2-4 Loop auto-detect:** RMS scan + zc snap + 8-sample equal-power xfade; one-shot fallback on variance / length / headroom failures.
- **D2-5 ADSR:** `juce::ADSR` (linear segments).
- **D2-6 Sub-stage order:** 2.1 → 2.2 → 2.3 → 2.4 → 2.5 (all complete).
- **D2-7 Filename parser:** Tolerant; case-insensitive; multi-convention.
- **D2-8 Out-of-range notes:** Silence.
- **D2-9 SR conversion:** `juce::LagrangeInterpolator` per channel at load time.
- **D2-10 Mono → stereo:** Duplicate L/R at unity gain.
- **D2-11 Smoothing:** `output_gain` smoothed via `juce::SmoothedValue` + `applyGainRamp`. `velocity_crossfade` consumed once per startNote (no SmoothedValue needed).
- **D2-12 NE granularity:** Once at `startNote()`.

## Files Created/Modified (Stage 2)

`Source/MicrotonalSamplerVoice.{h,cpp}`,
`Source/SampleMap.h` (`findSlot` linear scan),
`Source/SampleLoader.{h,cpp}` (full implementation),
`Source/FilenameParser.{h,cpp}` (new, Phase 2.2),
`Source/LoopDetector.{h,cpp}` (new, Phase 2.5),
`Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.{h,cpp}`,
`Source/tests/aliasing_check.cpp` (RQ-1 driver, EXCLUDE_FROM_ALL),
`plugins/O-MicrotonalSampler/CMakeLists.txt`,
`.planning/stages/2-dsp/CONTEXT.md`, `RESEARCH.md`, `PLAN.md`,
`PHASE-2.{1,2,3,4,5}-SUMMARY.md`, `VERIFICATION.md`,
`.planning/STATUS.md`, `.planning/REQUIREMENTS.md`.

## Outstanding Actions (post-verify)

1. **User commits Phase 2.5 + verify artefacts** — atomic commit per recipe in
   `VERIFICATION.md` Outstanding Actions §1.
2. **Subjective DAW pass** (Human Verification checklist in
   `VERIFICATION.md`) — sustained sine, vibrato cello, transient fallback,
   short-region edge case, regression suite re-run, +50 c retune listening
   test, mixed-SR fixture.
3. **CPU benchmark (PERF-02)** — 16 sustained voices, 48 kHz / 256 buffer,
   Apple Silicon, looping samples. Logic CPU meter or `pluginval
   --benchmark`. Confirm ≤ 5 %.

If any subjective check fails, file a defect and reopen the relevant
sub-phase rather than advancing to Stage 3.

## Next Steps

1. **Atomic commit** of Phase 2.5 + Stage 2 verify (recipe in
   `VERIFICATION.md`) — still outstanding.
2. **Stage 3 plan** — `/plugin-plan O-MicrotonalSampler 3-gui` to break
   3.1–3.5 into ordered tasks with gate-reports.
