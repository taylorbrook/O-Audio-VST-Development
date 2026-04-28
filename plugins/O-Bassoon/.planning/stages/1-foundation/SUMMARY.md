# O-Bassoon Stage 1: Foundation — Execution Summary

**Completed:** 2026-04-27
**Templates:** O-Wind (CMake), O-Lyrica (NE wiring + voice/sound shape)
**Agent:** foundation-shell-agent + orchestrator (build/install/validate)

## What Was Built

### Project Structure
- `plugins/O-Bassoon/CMakeLists.txt` — `juce_add_plugin` with `PLUGIN_CODE OBsn`, `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, scala-tuning-engine direct sources, `ouaricon_add_module(... note-expression)`, licensing gate, no binary-data block (Stage 3)
- `Source/BassoonSound.h` — header-only `juce::SynthesiserSound` catch-all
- `Source/BassoonVoice.{h,cpp}` — silent-stub `juce::SynthesiserVoice`; three setter wires (APVTS, `TuningEngine*`, `PendingTuningTable*`); `renderNextBlock` empty
- `Source/PluginProcessor.{h,cpp}` — `OBassoonAudioProcessor` with APVTS (10 params), `juce::Synthesiser`, headless `TuningEngine`, `Ouaricon::NoteExpression::VST3Extensions`; 16 voices pre-allocated in ctor; `processBlock` runs `vst3Extensions.drainAndUpdate()` BEFORE `synthesiser.renderNextBlock`; `getVST3ClientExtensions()` returns `&vst3Extensions`; APVTS XML state round-trip
- `Source/PluginEditor.{h,cpp}` — `juce::GenericAudioProcessorEditor` placeholder, 500x480, no WebView (Stage 3)

### Parameters (10 total — frozen spec)

| ID | Class | Range | Default | Suffix |
|---|---|---|---|---|
| `vibrato_rate` | Float | 0.0–10.0 | 5.0 | " Hz" |
| `vibrato_depth` | Float | 0.0–100.0 | 15.0 | " cents" |
| `vibrato_onset` | Float | 0.0–2000.0 | 400.0 | " ms" |
| `breath` | Float | 0.0–1.0 | 0.7 | — |
| `tone` | Float | 0.0–1.0 | 0.5 | — |
| `attack_character` | Float | 0.0–1.0 | 0.0 | — |
| `attack_time` | Float | 0.0–2000.0 | 300.0 | " ms" |
| `release_time` | Float | 0.0–3000.0 | 800.0 | " ms" |
| `voice_count` | Int | 1–16 | 8 | — |
| `output_gain` | Float | -24.0–6.0 | 0.0 | " dB" |

### Discrepancies absorbed (PLAN §"Discrepancies")
- **D1:** `ouaricon_add_module(O-Bassoon note-expression)` only — no `target_link_libraries(... PRIVATE Ouaricon::note_expression)` line
- **D2:** `TuningEngine` declared in **global namespace** (no `Ouaricon::` prefix) in voice + processor
- **D3:** `NEEDS_WEBVIEW2 TRUE` present in `juce_add_plugin` block alongside `NEEDS_WEB_BROWSER TRUE`; `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` in `target_compile_definitions`

### Build & Validation Results

- CMake configure: success — `[note-expression] JUCE-NE-PATCH markers verified`, `Added note-expression SharedCode sources to O-Bassoon`, `Added note-expression/cpp/vst3 sources to O-Bassoon_VST3`
- `cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone`: success (0 errors; only pre-existing note-expression module warnings carried from `funknown.h` and `Controller` dtor)
- AU registration via direct probe: `auval -v aumu OBsn OuDv` → `AU VALIDATION SUCCEEDED` (component code: `aumu OBsn OuDv`, name: "Ouaricon Audio Development: O-Bassoon-dev")
- VST3 + AU installed to system plugin folders after AU cache clear
- pluginval strictness 5 on installed VST3: `SUCCESS` (exit 0)

### Forbidden tokens (verified absent in `Source/`)
- ✅ No `Ouaricon::TuningEngine` anywhere in source
- ✅ No `target_link_libraries(... PRIVATE Ouaricon::note_expression)` line in CMake
- ✅ No `O-Reed` / `OReed` references (DSP-07)
- ✅ No `setLatencySamples(...)` calls
- ✅ No `juce::WebBrowserComponent` / WebSliderRelay / resource provider (Stage 3)
- ✅ No `juce_add_binary_data(...)` (Stage 1 has no resources)
- ✅ No DSP code (no biquads, no exciter, no ADSR, no LFO, no parameter reads in voice or processBlock beyond NE drain)

## Deviations from Plan

**One minor:** `createPluginFilter()` factory function added to `PluginProcessor.cpp` (JUCE plugin entry point — required for the binary to link; functionally equivalent to O-Wind/O-Lyrica's pattern).

**No-op (auto-handled):** Root CMakeLists.txt auto-discovers plugins via glob — manual registration unnecessary. CMake reconfigure picked up the new plugin directory automatically.

**Quality gate:** 0-ideation → 1-foundation gate was bypassed via `--force` because the build check has no buildable target at the start of the foundation stage (no code exists at ideation). Logged to `gate-bypasses.log`. This is the documented entry-point pattern, not a regression.

## Verifies Requirements

- **COMPAT-01** (pluginval strictness ≥5 pass): ✅ pluginval strictness 5 returned SUCCESS
- **DSP-07** (no O-Reed dependency): ✅ `grep` of `Source/` returns no matches

## Next

Verify phase: `/plugin-verify O-Bassoon 1-foundation`
