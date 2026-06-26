# Stage 4 (Validation / Polish) — PLAN

**Requirements:** FUNC-05 (concept presets), FUNC-08 (playability), QUAL-02
(audible-vs-visible), COMPAT-01 (pluginval VST3+AU re-gate) · **Ships:** v1.0.0

**Goal:** Take the verified Stage-3 teaching instrument from "all mechanisms work"
to "ships as v1.0.0" with **no new DSP or UI mechanism**. Fill the six
concept-isolating factory presets (FUNC-05) by adding a lightweight C++ factory
table (`BeatPresets.h`) + one host-notifying `applyConceptPreset(int)` + one
`applyPreset` native fn wired into the already-stubbed `initPresetTour()`; confirm
playability (FUNC-08) via preset-velocity discipline plus a single mandatory
audible clipping check (change defaults ONLY if clipping is heard); run the full
cross-format validation sweep (auval + pluginval strictness-10 VST3+AU +
render-harness 6/6 + UI screenshot with a preset loaded); do the hands-on QUAL-02
audible-vs-visible audit; and author CHANGELOG v1.0.0 covering the whole staged
build. Stability of the verified Stage-3 build is paramount — presets and any
playability tweak must not regress the grid, playhead, timing lane, MIDI readout,
or any of the 42 param bindings. All implementation code is already specified in
RESEARCH.md (cited per task) — transcribe it, do not reinvent it.

## Tasks

### T1 — `Source/BeatPresets.h` (RESEARCH §1a + §2)
- Create `Source/BeatPresets.h`: `#pragma once`, include `<array>`, `<cstdint>`,
  `"BeatmakerIDs.h"`. Define `struct BeatPreset` (name, patternLengthChoice,
  swing01, humanize01, quantize01, tempoBpm, `grid[kNumVoices][16]`) exactly as
  RESEARCH §1a.
- Transcribe the `inline constexpr std::array<BeatPreset, 6> kBeatPresets` table
  body **verbatim** from RESEARCH §2 "Transcription helper". Row order is the
  Voice enum order Kick/Snare/Clap/ClosedHat/OpenHat/Tom (`BeatmakerIDs.h:25`).
- `BeatPreset::name` strings must match the `data-preset` button labels in
  `index.html` (apply path keys on **index**, not name — names are caption-only).
- No per-voice param columns (RESEARCH §1a decision A1) — five timing-feel params
  + grid only.

### T2 — `applyConceptPreset(int)` on the processor (RESEARCH §1b)
- `PluginProcessor.cpp`: `#include "BeatPresets.h"`.
- `PluginProcessor.h`: declare `void applyConceptPreset (int index);` public, near
  the step-grid API.
- `.cpp`: implement exactly per RESEARCH §1b — bounds-check index; for each
  timing-feel param (`swing`, `humanize`, `quantizeStrength`, `tempo`) call
  `prm->setValueNotifyingHost(prm->convertTo0to1(real))`; for `patternLength` use
  `convertTo0to1((float) choiceIndex)`; then `clearGrid()` followed by `setStep`
  for every non-zero cell of the 16 columns. Message-thread only, no audio-thread
  work, no new lock (CONTEXT decision 2; PERF-01 unchanged).
- **Normalisation is load-bearing** — `setValueNotifyingHost` takes 0–1; passing a
  real value jams the param to max (pitfall P2/P3). Always go through
  `convertTo0to1`.

### T3 — `applyPreset` native fn (RESEARCH §1c)
- `PluginEditor.cpp`: add `.withNativeFunction("applyPreset", …)` inside the
  existing chain (alongside `setStep`/`getGrid`/`clearGrid`/`getSampleRate`).
  Body: if `args.size() >= 1` call `processorRef.applyConceptPreset((int) args[0])`;
  `complete(juce::var())`. Same `(int) args[0]` marshalling as `setStep`.

### T4 — Wire `initPresetTour()` in `app.js` (RESEARCH §1d)
- `Source/ui/public/js/app.js`: in `boot()`, declare `applyPresetFn` next to the
  other native-fn lookups: `try { applyPresetFn = Juce.getNativeFunction("applyPreset"); } catch …`
  with a null fallback + `console.error`. **Declare before use** (pitfall P6 — a
  `ReferenceError` here silently kills the whole WebView UI).
- Replace the stub `initPresetTour()` body per RESEARCH §1d: for each `.tour-btn`
  (DOM order == preset index), on click set `.armed`, `await applyPresetFn(index)`
  (null-guarded), then `await refreshGridFromBackend()` to repaint grid velocities,
  then update the caption. Knobs + length combo update for free via attachments —
  only the grid needs an explicit refresh (pitfall P7).

### T5 — Native-fn parity grep-diff gate (RESEARCH §1e) — BEFORE build
- Run the §1e grep-diff. Both sides MUST now list exactly:
  `applyPreset clearGrid getGrid getSampleRate setStep` (currently 4 each; T3+T4
  add the 5th). A mismatch = silently dead buttons that still pass build/auval/
  harness (pitfall P1, `pattern_webview_native_fn_bridge_gap`). Do not proceed to
  build until the two sets are identical.

### T6 — Build + cache-clear + dual-variant install (RESEARCH §4a/§4b)
- Reconfigure with tests on, then build all three formats:
  `cmake .. -G Ninja -DOUARICON_BUILD_TESTS=ON` then
  `ninja O-simpleBeatmaker_VST3 O-simpleBeatmaker_AU O-simpleBeatmaker_Standalone`.
- Install via `./scripts/build-and-install.sh O-simpleBeatmaker` (its Phase-4 does
  the dual-variant `-dev`↔unsuffixed sweep + AU cache clear automatically). Watch
  for the `⚠ Sweeping ALTERNATE-variant` warning.

### T7 — FUNC-08 playability audible check (RESEARCH §3)
- After install, load **Preset 1 (Backbeat + Accents)** and audition the downbeat
  (kick 127 + accented hat) at master 0 dB; the snare/clap noise component is the
  hot ×2 path. Listen/meter for clipping.
- **Default recommendation: change nothing** (RESEARCH §3 — Stage-2 defaults
  passed QUAL-01). Apply a fallback **only if clipping is heard**: `outputLevel`
  default `0.0f → -3.0f` (or `snareLevel`/`clapLevel` `0.0f → -2.0f`) in
  `createParameterLayout` — a **value-only** change, never a new param (contract
  frozen, pitfall P4). If applied: re-run the save/reload round-trip + the T8
  render-harness, and flag the moved default in SUMMARY for verify.

### T8 — Validation sweep (RESEARCH §4c/§4d/§4e)
- **auval:** `auval -a | grep -i beatmaker` then `auval -v aumu OSiB OuDv` (use
  `OuAu` for a release build). Expect `AU VALIDATION SUCCEEDED`.
- **pluginval strictness-10** on BOTH formats (COMPAT-01 final gate):
  `pluginval --strictness-level 10 --validate-in-process` against the installed
  `…/VST3/O-simpleBeatmaker-dev.vst3` and `…/Components/O-simpleBeatmaker-dev.component`.
  Expect `ALL TESTS PASSED`.
- **render-harness (6 probes):** build + run the DSP regression gate. NOTE: the
  target is **`O-simpleBeatmaker-render-test`** (RESEARCH §4e/A4 guessed
  `_RenderTest` — wrong; confirmed in `tests/render-harness/CMakeLists.txt:13`):
  `ninja O-simpleBeatmaker-render-test` then run
  `build/plugins/O-simpleBeatmaker/tests/render-harness/O-simpleBeatmaker-render-test`
  — exit 0 = all 6 probes green. The harness sets params directly (not via presets),
  so a green run proves the timing DSP is untouched by Stage 4.

### T9 — Hands-on QUAL-02 audit + screenshot (RESEARCH §4f)
- In a DAW (transport playing) AND in Standalone free-run, walk RESEARCH §4f
  steps 1–5: Straight (dots on the line, playhead matches what's heard, MIDI
  readout prints 36/38/42 with right velocities) → Backbeat (accents visibly
  taller/brighter AND louder) → Triplet Swing (off-beat hat dots sit right/late,
  quantize 100% does NOT pull them back) → Humanized (raise Quantize live → scatter
  collapses) → Quantize Demo (sweep Quantize: scatter pulls in while swing remains
  — DSP-04 interactive). The lane draws the **applied Δt**, never a recompute.
- If a bug appears only in the DAW but is clean in harness + Standalone, quit/reopen
  the host first — it's a stale cached instance, not a defect (pitfall P9).
- Capture the verify artifact: a UI screenshot **with a preset loaded** (grid
  populated, knobs at preset values, lane animating) — not a blank grid.

### T10 — `CHANGELOG.md` v1.0.0 (RESEARCH §4g)
- Create `plugins/O-simpleBeatmaker/CHANGELOG.md` as the first release entry
  covering the whole staged build (Foundation → DSP → GUI → Polish): synthesized
  808/909 six-voice kit, host-synced sample-accurate sequencer, swing/humanize/
  quantize feel engine with the DSP-04 invariant, WebView teaching UI (grid +
  playhead + timing lane + MIDI readout + tooltips), six concept presets,
  cross-platform WebView2 config. Match `plugins/O-simpleGrain/CHANGELOG.md` style
  (Keep a Changelog, `## [1.0.0] — 2026-06-25`). Note any FUNC-08 default change
  from T7 if applied.

### T11 — SUMMARY + status flip
- Write `.planning/stages/4-polish/SUMMARY.md` (what shipped, the 6 presets, sweep
  results, any T7 default change, screenshot path).
- Update `.planning/STATUS.md` → stage 4 complete / status working.
- Update root `PLUGINS.md` line 65: `🚧 Stage 3` → `✅ Working`, v1.0.0.

## Files

- **Create:** `Source/BeatPresets.h`, `CHANGELOG.md`,
  `.planning/stages/4-polish/SUMMARY.md`.
- **Modify:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`,
  `Source/PluginEditor.cpp`, `Source/ui/public/js/app.js`, `.planning/STATUS.md`,
  `PLUGINS.md` (root).
- **Conditional (T7 fallback only):** `Source/PluginProcessor.cpp`
  `createParameterLayout` default value (`outputLevel` / `snareLevel` / `clapLevel`)
  — value change only, no new param.
- **No new** `juce_add_binary_data` target (preset data is C++ source — pitfall P5);
  `CMakeLists.txt` unchanged except the existing `-DOUARICON_BUILD_TESTS=ON` path.

## Dependencies

`T1 → T2 → T3 → T4 → T5` (linear: table → processor method → native fn → JS wiring
→ parity gate). `T6` after `T5` (build the parity-clean tree). `T7` after `T6`
(audible check needs the installed build). `T8` after `T6` (sweep needs the build).
`T9` after `T8` (audit the validated build). `T10` any time (independent doc).
`T11` last (records the finished result of all above).

## Success criteria (goal-backward — verify gates)

1. **All 6 tour buttons load a distinct, recognizable pattern + its concept-isolating
   params** — grid velocities, knobs, length combo, and lane all visibly update on
   click (T4 + T9 step 1–2).
2. **The presets demonstrably isolate their concept** — esp. Quantize Demo: sweeping
   quantize pulls the humanize scatter back toward the grid WHILE the swing lateness
   remains (DSP-04 visible/audible) (T9 step 5).
3. **Kit sounds good out of the box (FUNC-08)** — preset velocities + defaults are
   musical, no harshness/clicks; the Preset-1 downbeat does not clip at 0 dB (T7).
4. **Build clean VST3+AU+Standalone; auval SUCCEEDED; pluginval strictness-10 PASSED
   (VST3 + AU); render-harness 6/6 green; UI screenshot with a preset loaded** (not
   blank) (T6 + T8 + T9 step 6).
5. **No regression** — every Stage 1–3 verified behavior still holds (grid, playhead,
   timing lane, MIDI readout, all 42 bindings, save/reload round-trip) (T8 harness +
   T9 audit).
6. **CHANGELOG v1.0.0 written; STATUS + root PLUGINS.md → ✅ Working** (T10 + T11).

## Risks / no-regression callouts (RESEARCH §5)

- **Native-fn parity (P1):** `applyPreset` registered on one side only → silently
  dead buttons that pass build/auval/harness. T5 grep-diff is a hard gate; expect 5
  names on both sides.
- **`setValueNotifyingHost` normalisation (P2/P3):** the arg is 0–1. Passing a real
  value (e.g. `120.0f` tempo, `0.8f` swing, choice index) jams the param to max.
  Always `convertTo0to1(real)`; for `patternLength` use `convertTo0to1((float)idx)`.
- **Frozen 42-param contract (P4):** presets set existing params only; never add a
  "preset id" param or rename an ID (breaks saved sessions + the relay/attachment
  lists). FUNC-08 fallback is a default **value** change only, round-trip-tested.
- **No 2nd binary-data target (P5):** preset data is `constexpr` C++ in
  `BeatPresets.h`, not a blob — avoids the O-simpleGrain BinaryData namespace
  collision. Keep the single `O-simpleBeatmaker_UIResources` target.
- **JS helper-ref death (P6):** declare `applyPresetFn` in `boot()` before use and
  null-guard the call; a `ReferenceError` in `app.js` kills the entire UI silently —
  test the live UI, not just the build.
- **clearGrid before stamp (P8):** `applyConceptPreset` must `clearGrid()` first, then
  write the 16 columns, or a prior 32-step pattern's tail (cols 16–31) survives behind
  a 16-step preset.
- **Grid repaint (P7):** knobs/combo update via attachments; the grid has no
  attachment — the button handler must call `refreshGridFromBackend()` (raf poll is
  the backstop).
- **Stale host instance (P9):** a DAW-only bug clean in harness + Standalone =
  stale cached plugin; quit/reopen the host before chasing it.
