# Stage 3 (GUI) — Context, Research & Plan

**Date:** 2026-06-22
**Mode:** manual · "build directly, show running plugin"
**Reference:** O-simpleFM (shipped sibling) — Field Guide WebView UI, reused near-verbatim.

## Decisions (discuss)

1. **Aesthetic → Match O-simpleFM "Field Guide" house style** (sibling pair). Reuse
   `styles.css` design tokens (aged-paper palette, Garamond, seed-knobs, green botanical
   accents, fleurons, `insects.png` overlay, tooltip + lesson-tour + on-screen-keyboard
   idioms). Layout adapted: **16-drawbar surface dominant** in place of the FM routing diagram.
2. **Spectrum viz → the 16 drawbars ARE the live spectrum.** Each drawbar shows two layers:
   the **set** level (the control, draggable) and a **live** glow = the morphed+decayed
   sounding level (from the lock-free active-spectrum snapshot, exact — QUAL-02). One
   **oscilloscope** below shows the summed waveform. **No separate FFT spectrum canvas.**
3. **Process → build directly across the 3 planned phases**, build/auval each, install, show
   the running plugin.

## Research (what exists vs. what Stage 3 adds)

Confirmed against source:

- **Exists (Stage 2):** `getVizRing()`, `readActiveSpectrum(float* dest16)`, `AdditiveVizAnalyzer.h`
  (VizRing + message-thread FFT/scope), full 33-param APVTS, plain-XML state persistence.
- **Missing (Stage 3 must add):**
  - On-screen-keyboard MIDI plumbing — `juce::MidiMessageCollector` + `handleUiMidi(note,on,vel)`
    + `midiCollector.removeNextBlockOfMessages` drain in `processBlock` (port from O-simpleFM).
  - `getCurrentSampleRate()` getter (analyzer needs it; also the keyboard collector reset).
  - `isSounding()` flag (so the drawbar live-glow snaps to the set level when idle, instead of
    showing a stale snapshot).
  - WebView editor (relays → webview → attachments), resource provider (bare-path), CMake
    `juce_add_binary_data` + link.
  - Lesson presets — `applyFactoryPreset(name)` native fn applying full APVTS snapshots
    (relays propagate to the JS controls). **Persistent save/load preset BAR deferred to Stage 4**
    (per the existing CMake note "preset-manager is a Stage 4 concern"; the plan allows
    "APVTS snapshots" for the tour).

### Param inventory (drives relays + JS)

- **31 `WebSliderRelay`:** `partial1..16` (drawbars, 0–1 → ×100 %), `scanPosition` (%),
  `scanLfoRate` (Hz), `scanLfoDepth` (%), `scanEnvAmount` (±%), `spectralDecay` (%),
  `velToDecay` (%), `ampAttack/Decay/Sustain/Release`, `modAttack/Decay/Sustain/Release`,
  `outputLevel` (dB).
- **2 `WebComboBoxRelay`:** `frameBSource` {Sine,Saw,Square,Odd}, `bitDepth` {Off,12,10,8,6,4,2}.

Ranges (from `createParameterLayout`): drawbars/scan/depth/decay/vel/sustain = `unitRange` 0–1;
ADSR times = {0.001,5,_,0.35 skew}; `scanLfoRate` {0.01,20,_,0.3}; `scanEnvAmount` {−1,1};
`outputLevel` {−60,0}. JS formatters scale accordingly.

## Plan (3 phases, ROADMAP §Stage 3)

- **3.1 Layout + drawbars + controls + cross-platform wiring** — index.html / styles.css / app.js
  (drawbars + knobs + combos two-way bound), js/juce + insects.png copied, CMake binary data,
  editor (relays/webview/attachments + native fns: `uiMidi`, `getSampleRate`, `applyFactoryPreset`),
  processor keyboard MIDI + getters. WebView2 flags already set at Foundation.
- **3.2 Live drawbar-spectrum + oscilloscope** — 30 Hz Timer → `AdditiveVizAnalyzer` scope +
  emit `drawbarSpectrumUpdate {sounding, levels[16]}` + `scopeUpdate`. JS lights the live glow and
  draws the scope (DPR-aware canvas; copy scope window before FFT — already handled in analyzer).
- **3.3 Tooltips + lesson preset tour** — TIPS map on every `[data-tip]`; 6 lesson presets via
  `applyFactoryPreset`; odd/even/fundamental drawbar tinting + H-number labels.

## Critical patterns honored

#11/#12 member order relays→webview→attachments; 3-arg attach + `nullptr` undoManager.
#19 `getSliderState`/`getComboBoxState` per type. #21 `import * as Juce`. Resource provider
bare-path equality. Canvas DPR-aware backing store (project memory). WebView2 static-linking flag
(set at Foundation). Windows `withUserDataFolder(tempDir)`.
