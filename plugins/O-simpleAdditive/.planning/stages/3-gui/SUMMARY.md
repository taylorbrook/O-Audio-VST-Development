# Stage 3 (GUI) — Implementation Summary

**Date:** 2026-06-22
**Result:** ✓ Complete — VST3 + AU build clean, `auval` SUCCEEDED, installed, UI verified rendering.

## What was built

A single-page Ouaricon "Additive Field Guide" WebView UI (sibling of O-simpleFM),
with the 16-drawbar surface dominant and doubling as the live spectrum display.

### Phase 3.1 — Layout + drawbars + controls + cross-platform wiring
- `Source/ui/public/index.html` — header, drawbar bay, oscilloscope, 5 control groups,
  lesson-preset tour, on-screen keyboard, tooltip layer.
- `Source/ui/public/css/styles.css` — Field Guide aesthetic adapted from O-simpleFM
  (aged-paper palette, Garamond, seed-knobs, green accents) + new **brass drawbar** and
  **combo (select)** styling, odd/even/fundamental harmonic tinting.
- `Source/ui/public/js/app.js` — controller: 16 drawbars (absolute fader drag + wheel +
  arrow keys), 15 seed-knobs (relative drag), 2 combo boxes; two-way bound via
  `Juce.getSliderState` / `Juce.getComboBoxState`.
- Copied `js/juce/{index.js, check_native_interop.js}` + `img/insects.png` from O-simpleFM.
- `PluginEditor.h/.cpp` rewritten — 31 `WebSliderRelay` + 2 `WebComboBoxRelay`
  (relays → webView → attachments order); resource provider (bare-path equality);
  native fns `uiMidi`, `getSampleRate`, `applyFactoryPreset`; Windows `withUserDataFolder`.
- `PluginProcessor.h/.cpp` — added `juce::MidiMessageCollector` + `handleUiMidi` +
  `removeNextBlockOfMessages` drain (on-screen keyboard), `getCurrentSampleRate()`,
  `isSounding()` flag.
- `CMakeLists.txt` — `juce_add_binary_data(O-simpleAdditive_UIResources …)` + link;
  added `AdditiveVizAnalyzer.h` to sources. (WebView2 flags already set at Foundation.)

### Phase 3.2 — Live drawbar-spectrum + oscilloscope
- 30 Hz editor `Timer` → `AdditiveVizAnalyzer.process(getVizRing(), sampleRate)` (scope).
- Emits `drawbarSpectrumUpdate {sounding, levels[16]}` (the morphed+decayed active-spectrum
  snapshot — exact, QUAL-02) + `scopeUpdate` (128-pt summed waveform).
- JS lights each drawbar's translucent **live glow** over the brass set-level; when idle
  (`!sounding`) the glow hides so only the set levels show. DPR-aware scope canvas.

### Phase 3.3 — Tooltips + lesson preset tour
- Plain-language hover tooltips on **every** control + per-partial harmonic tips
  (fundamental / odd / even, with the overtone explanation).
- 6 lesson presets via the C++ `applyFactoryPreset` native fn (full APVTS snapshots;
  relays propagate to the UI): **Pure Sine, Sawtooth, Square, Organ, Morph Pad, Lo-Fi Bells**.
  Captions explain each concept. Persistent save/load preset BAR deferred to Stage 4.

## Files changed
- New: `Source/ui/public/{index.html, css/styles.css, js/app.js, js/juce/index.js,
  js/juce/check_native_interop.js, img/insects.png}`
- Rewritten: `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- Edited: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `CMakeLists.txt`

## Deferred to Stage 4
- Persistent user-preset bar (OuariconPresetManager: save/load/browse/delete) — the
  "preset-manager is a Stage 4 concern" note in CMake. Lesson tour ships now as C++ snapshots.
- A bespoke (non-`insects.png`) botanical overlay if a more additive-apt asset is desired.
