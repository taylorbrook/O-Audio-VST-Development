# Stage 3 (GUI) — PLAN

**Goal:** Single-page WebView teaching UI replacing the generic shell — 6×16 grid
+ playhead, all 42 params bound, the applied-Δt timing lane, the live MIDI readout,
tooltips, and a preset-tour hook. Cross-platform correct. Build clean + pluginval.

## Tasks

### T1 — Processor read-only getters for the lane (minimal, additive)
- Add `double getCurrentSampleRate() const noexcept`.
- Add `std::atomic<double> lastKnownBpm {120.0}`; store the resolved `bpm` once/block
  in `processBlock` (relaxed). Add `double getLastKnownBpm() const noexcept`.
- **No change to DSP behaviour** — read-only taps for the UI. Files: `PluginProcessor.h/.cpp`.

### T2 — WebView UI assets (Phase 3.1/3.2/3.3 combined)
Create `Source/ui/public/`:
- `index.html` — single page: header → grid+playhead → timing lane → MIDI readout →
  global timing knobs (swing/humanize/quantize/length/tempo) → 6 per-voice strips
  (tune/decay/tone/level/mute/solo) → master + preset tour → floating tooltip. Loads
  `check_native_interop.js` then `app.js` as `type="module"`.
- `css/styles.css` — field-guide aesthetic; the grid is the dominant element; per-voice
  hue coding; projector-legible type; DPR-safe canvases.
- `js/app.js` — bind 29 sliders + 1 combo + 12 toggles; grid build + click cycle
  (off/normal/accent/ghost) via `setStep`; `getGrid()` paint-on-load; playhead sweep;
  timing-lane canvas (tempo-normalised Δt); MIDI readout list; tooltips; preset hook.
- `js/juce/index.js` + `js/juce/check_native_interop.js` — copied from O-simpleSubtractive.

### T3 — PluginEditor rewrite (relays → WebView → attachments + native fns + Timer)
- `PluginEditor.h`: `Timer` base; relay vectors (slider/combo/toggle); `webView`;
  attachment vectors; `getResource`. Correct member order.
- `PluginEditor.cpp`: build the 3 relay families from the param-ID lists; resource
  provider (bare-path); native fns `setStep/getGrid/clearGrid/getSampleRate/getBpm`;
  `#if JUCE_WINDOWS` user-data folder; 3-arg attachments; `startTimerHz(60)`;
  `timerCallback` drains `VizAnalyzer` → emits `playhead` + `triggers`.

### T4 — CMake
- Add `juce_add_binary_data(O-simpleBeatmaker_UIResources SOURCES …)` (5 files); link it
  PRIVATE. Default `BinaryData` namespace (only target). Keep all existing flags.

### T5 — Build, install, verify
- `ninja O-simpleBeatmaker_VST3 O-simpleBeatmaker_AU O-simpleBeatmaker_Standalone`.
- Clear AU cache, dual-variant sweep, install (build-and-install.sh).
- `pluginval` strictness 10 on the VST3; `auval` for the AU.
- Visual check via Standalone (grid clickable, playhead sweeps, lane reacts, MIDI prints).

## Files
- **Modify:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`,
  `Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `CMakeLists.txt`.
- **Create:** `Source/ui/public/{index.html, css/styles.css, js/app.js,
  js/juce/index.js, js/juce/check_native_interop.js}`.

## Success criteria
Every ROADMAP Stage-3 test-criteria box (CONTEXT → Acceptance) green; build clean for
VST3+AU+Standalone; pluginval strictness-10 SUCCESS; native-fn JS↔C++ sets match
(no silent dead control); UI is not blank on load.

## Risks / mitigations
- **Silent dead control** (native-fn mismatch / ID drift) → `jassert(param)` on each
  attachment; grep-diff `getNativeFunction` vs `withNativeFunction`; assert grid native fns.
- **Blank UI** (resource provider / module error) → bare-path matching; self-contained
  `app.js`; guarded `getElementById`.
- **Lane ≠ audio** → render the Δt baked in the VizEvent, never recompute the feel formula.
