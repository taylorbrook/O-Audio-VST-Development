# Stage 2 / Phase 2.1 — Core Additive Voice — SUMMARY

**Date:** 2026-06-22
**Goal:** A polyphonic, MIDI-playable additive voice — 16 drawbars (Frame A) summed into a per-note band-limited single-cycle table, read by phase, shaped by the amp envelope. The audible additive core (FUNC-01/05, DSP-01/02).
**Gate (incremental-DSP mode):** clean build + `auval` — **MET**.

## What was implemented

### New: `Source/AdditiveVoice.h`
- `OSimpleAdditive::fastSine` — shared 1024-pt `dsp::LookupTableTransform` sine primitive (ported from O-simpleFM `Operator.h`), with the mandatory floor-modulo phase wrap (LUT clamps, not wraps; harmonics evaluate sin at k·θ up to ~32π).
- `AdditiveSound` — trivial `SynthesiserSound` (applies to all notes/channels).
- `AdditiveVoice : juce::SynthesiserVoice`:
  - **Non-virtual** `prepareToPlay(sr, maxBlock)` (JUCE 8 — dispatched via `dynamic_cast`); `setSampleRate` before first `setParameters`.
  - `setParams(frameA[16], ampParams)` — block param-push; marks the table dirty only when Frame A actually moved (`juce::exactlyEqual` exact dirty-check), so a static patch refills once per note, not once per block.
  - `startNote` — `f0 = MidiMessage::getMidiNoteInHertz`; phase→0; per-note `Kmax = floor(0.5·fs/f0)`; immediate table refill; `ampEnv.noteOn()`.
  - `refillTable()` — band-limited additive sum into a 2048-pt single-cycle table: `table[i] = (Σ_{k≤Kmax} band_k·sin(2π·k·i/N)) / max(1, Σ band_k)`.
    - **Band-limit:** `nyquistGain(k, Kmax)` — hard drop for k>Kmax, raised-cosine taper on the top 2 surviving harmonics (Kmax-1 ≈ 0.5, Kmax ≈ 0), fundamental always full → no boundary click, no aliasing.
    - **Headroom:** divide by `max(1, Σ band_k)` ⇒ `|table| ≤ 1` ⇒ 16 maxed drawbars cannot clip; single partial stays full-scale.
  - `renderNextBlock` — control-rate refill if dirty, then per-sample `readTableLinear(phase)` (linear interp, power-of-two mask wrap) × `ampEnv` × velocity; `phase += f0/fs` normalized wrap. Voice lifetime gated on **amp** envelope only.
  - v1.0: no pitch bend (`pitchWheelMoved`/`controllerMoved` no-ops — pitch fixed per note).

### Modified
- `Source/PluginProcessor.h` — `#include "AdditiveVoice.h"`; added `juce::Synthesiser synth` + `pushParamsToVoices()`; removed silent-shell framing.
- `Source/PluginProcessor.cpp`:
  - ctor: pre-allocate 16 `AdditiveVoice` + shared `AdditiveSound`; `setNoteStealingEnabled(true)`.
  - `prepareToPlay`: `synth.setCurrentPlaybackSampleRate`; per-voice custom `prepareToPlay` via `dynamic_cast`. Still `setLatencySamples(0)` (no oversampling).
  - `processBlock`: `ScopedNoDenormals` → clear → `pushParamsToVoices()` → `synth.renderNextBlock` → smoothed `outputLevel` trim → `isfinite` scrub. (No viz tap yet — 2.3.)
  - `pushParamsToVoices()`: reads 16 `partialN` + amp ADSR from APVTS once/block → `setParams`. Voices never touch APVTS.
  - `releaseResources`: `synth.allNotesOff`.
- `CMakeLists.txt` — registered `Source/AdditiveVoice.h` in `target_sources`.

## Verification
- **Build:** VST3 + AU compile clean (only the pre-existing `processorRef` unused-field warning from the Stage-1 GenericAudioProcessorEditor placeholder — resolved when the WebView editor lands in Stage 3). DSP code warning-free.
- **auval:** `aumu OSiA OuDv` → **AU VALIDATION SUCCEEDED**. Render tests pass at 11025/22050/44100/48000/96000/192000 Hz, mono + stereo, 64–4096 frames; bad-max-frames handled; **MIDI test PASS** (voice triggers/renders/releases without NaN, denormal, or crash).

## Audible criteria — for DAW/Standalone confirmation (not auval-checkable)
- [ ] H1=100 / rest 0 → pure sine; drawbars at 1/k ≈ sawtooth (FUNC-01).
- [ ] Each drawbar audibly changes its harmonic.
- [ ] Correct pitch per MIDI note; high keys (C7+) all-drawbars-up → no aliasing/buzz (DSP-02).
- [ ] 16 maxed drawbars do not clip.
- [ ] Amp ADSR shapes notes; no stuck/silent voices; no note-on/off clicks.

## Not in 2.1 (deferred to later Stage-2 phases)
- 2.2: Frame A→B spectral morph, scan LFO, mod-env→scan. (`frameBSource`, `scanPosition`, `scanLfoRate/Depth`, `scanEnvAmount`, mod ADSR params present in APVTS but not yet consumed.)
- 2.3: spectral-decay tilt, bit-depth quantizer, lock-free viz tap + active-spectrum snapshot. (`spectralDecay`, `bitDepth`, `velToDecay` present but not yet consumed.)

The `refillTable()` active-spectrum pipeline has marked extension points where 2.2 (morph) and 2.3 (decay) compose, before the band-limit + sum.
