---
spike: 001
name: patch-build-load
validates: "Given a patched JUCE + O-Lyrica NEC wiring + voice-side tuning hook, when built on macOS, then VST3 builds cleanly, pluginval passes at strictness 5, and plugin loads in Dorico without crash"
verdict: VALIDATED
related: [002-quarter-sharp-end-to-end]
tags: [juce-patch, vst3, note-expression, build]
---

# Spike 001: patch-build-load

## What This Validates

**Given** a patched `juce_VST3ClientExtensions.h` (adds `Vst3RawEvent` + `onVst3RawEvent`) and a patched `juce_audio_plugin_client_VST3.cpp` (forwards VST3 events to the extensions before MIDI conversion), plus O-Lyrica plugin-side wiring (`TuningNoteExpressionController`, `LyricaVST3Extensions`, pending-tuning array, voice-side `setPendingTuningSource`),
**when** built on macOS via ninja,
**then** the VST3 builds cleanly, pluginval passes at strictness 5, and the plugin loads in Dorico without crash.

## Files Touched

### JUCE patch (Approach 2 — side-channel via VST3ClientExtensions)

- `/Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h`
  - Added `struct Vst3RawEvent` (NoteOn / NoteOff / NoteExpressionValue with noteId, sampleOffset, pitch, channel, typeId, value).
  - Added `virtual void onVst3RawEvent (const Vst3RawEvent&)` default no-op.
- `/Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp`
  - In the wrapper's process path (right before `MidiEventList::toMidiBuffer`), iterate `data.inputEvents` and forward matching events (`kNoteOnEvent` / `kNoteOffEvent` / `kNoteExpressionValueEvent`) to `extensions->onVst3RawEvent`. Upstream MIDI conversion continues to drop NE silently — we capture it before it's lost.

Both edits are commented `// JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)` for easy reapply after future JUCE updates.

### O-Lyrica plugin changes

- `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` (new, header-only)
  - `OLyrica::TuningNoteExpressionController` — implements `Steinberg::Vst::INoteExpressionController`, advertises `kTuningTypeID` (bipolar, absolute, range `[0,1]` → `[-120, +120]` semitones).
  - `OLyrica::LyricaVST3Extensions` — inherits `juce::VST3ClientExtensions`, overrides `queryIEditController` (returns NEC on `INoteExpressionController::iid`) and `onVst3RawEvent` (buffers raw events for the processor to drain).
- `plugins/O-Lyrica/Source/PluginProcessor.h`
  - Member `OLyrica::LyricaVST3Extensions vst3Extensions`.
  - Override `getVST3ClientExtensions()` returning `&vst3Extensions`.
  - Member `std::array<std::atomic<double>, 128> pendingTuningSemis` — per-MIDI-pitch pending tuning delta.
  - Member `std::vector<Vst3RawEvent> rawEventScratch` — scratch for draining per-block events.
- `plugins/O-Lyrica/Source/PluginProcessor.cpp`
  - At top of `processBlock`: drain `vst3Extensions.drainBlockEvents(rawEventScratch)`, build `noteId → MIDI pitch` map from noteOn events, apply each `kTuningTypeID` NE event to `pendingTuningSemis[pitch]` (semitones = `240.0 * (value - 0.5)`).
  - Voice constructor loop: `voice->setPendingTuningSource(&pendingTuningSemis)`.
- `plugins/O-Lyrica/Source/HarpSynthVoice.h` / `.cpp`
  - Added `setPendingTuningSource(std::array<std::atomic<double>, 128>*)`.
  - In `startNote`, after humanization: `semis = (*src)[midiNote].exchange(0.0); currentFrequency *= pow(2, semis/12.0)`. Exchange ensures a re-triggered note on the same pitch later doesn't inherit a stale offset.

## How to Run

```bash
cd /Users/taylorbrook/Dev/VST-development/build
ninja OLyrica_VST3

# Install
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3
cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/O-Lyrica-dev.vst3 \
      ~/Library/Audio/Plug-Ins/VST3/

# Validate
/Applications/pluginval.app/Contents/MacOS/pluginval \
    --strictness-level 5 --verbose --skip-gui-tests --validate-in-process \
    ~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3
```

Then:

1. Quit Dorico entirely.
2. Launch Dorico 6. Let it rescan VST3s (first launch after install may auto-rescan; if not, Preferences → Play → VST Plug-ins → Rescan).
3. Open a new piano score. In Play mode, select the default piano instrument track and change its VST to "O-Lyrica-dev" (Ouaricon).
4. Enter any single note. Playback should produce O-Lyrica's harp sound (untuned, 12-TET — this is expected at this stage, Spike 002 verifies tuning).
5. If Dorico loads the plugin without crash and produces sound, Spike 001 is VALIDATED.

## What to Expect

- `ninja` completes, reports "Linking CXX CFBundle ... O-Lyrica-dev" and re-signing.
- Pluginval strictness 5 ends with `SUCCESS`.
- Dorico's VST3 scan completes without error.
- Plugin loads, produces audible harp-like tone on noteOn, no crash.

## Results

### Self-verifiable (completed by Claude)

- ✅ **Build:** `ninja OLyrica_VST3` succeeded, linking CFBundle at `build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/O-Lyrica-dev.vst3`. Two shadow-field warnings from the Steinberg SDK header (`funknown.h:549`) — pre-existing, not from our code.
- ✅ **Pluginval:** `--strictness-level 5 --verbose --skip-gui-tests --validate-in-process` returned `SUCCESS` in 902ms. All bus layout tests pass.
- ✅ **Install:** Bundle ad-hoc signed by Xcode toolchain, installed at `~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3`.

### User-verified

- ✅ **Dorico load (2026-04-23):** User confirmed O-Lyrica-dev appears in Dorico's VST3 instrument list, loads without crash, and plays audio as expected.

### Verdict

**VALIDATED.** Every build-time risk from the research has been retired: the JUCE patch compiles and doesn't break the wrapper, the NEC is correctly exposed via `queryIEditController` (implied by Dorico not rejecting the plugin), and the plugin operates normally in a real host. Ready to test actual NE event flow in Spike 002.
