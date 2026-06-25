# Stage 2 (DSP) — PLAN

**Goal:** Implement the full subtractive voice (osc bank → multimode ZDF SVF → VCA, dual ADSR,
voice modes + glide, real-time-safe viz tap) per `ARCHITECTURE.md`, gated by an offline render-harness.

**Strategy:** three de-risking phases (2.1 → 2.2 → 2.3), linear filter + curve match BEFORE self-osc.

## Files

| File | Action | Phase |
|------|--------|-------|
| `Source/OscillatorBank.h` | create — PolyBLEP saw/square/tri + LUT sine + sub-square + noise + mixer | 2.1 |
| `Source/SvfZDF.h` | create — Cytomic ZDF SVF (linear first; tanh self-osc added 2.2); 1-pole/12/24 dB | 2.1→2.2 |
| `Source/SubVoice.h` | create — `SubSound` + `SubVoice : SynthesiserVoice`; per-sample osc→filter→VCA, dual ADSR, fcEff | 2.1 |
| `Source/SubVizAnalyzer.h` | create — `VizRing` (verbatim) + analyzer + `computeMagnitudeDb` closed-form curve | 2.1/2.2 |
| `Source/PluginProcessor.h` | edit — add Synthesiser, MonoController, VizRing, atomics, push/handleUiMidi decls | 2.1→2.3 |
| `Source/PluginProcessor.cpp` | edit — voices, prepare (no oversampling), pushParamsToVoices, processBlock, viz tap | 2.1→2.3 |
| `CMakeLists.txt` | edit — add new headers to target_sources; add tests subdir (OUARICON_BUILD_TESTS) | 2.1 |
| `tests/render-harness/{CMakeLists.txt,main.cpp}` | create — Stage-2 correctness gate | 2.1→2.3 |

## Phase 2.1 — Source + Linear Filter + Dual ADSR + VCA

1. `OscillatorBank.h`: per-voice osc. Saw (`2φ−1` − polyBLEP), Square (pulse + 2 polyBLEPs / saw-difference), Triangle (polyBLAMP or leaky-integrated square), Sine (1024 LUT, wrapped). Sub = polyBLEP square at `f0/2`. Noise = xorshift white. Mix `src = main + subLevel·sub + noiseLevel·noise`. `dt = f0/fs`.
2. `SvfZDF.h`: linear Cytomic core (simultaneous lp/bp/hp; notch = src−k·bp). `setSlope` (1/2/4 pole); 24 dB = 2 cascaded stages (res on stage 1). `reset()` clears states. (tanh path stubbed off until 2.2.)
3. `SubVoice.h`: `SubSound`; `SubVoice` holds OscillatorBank, two `SvfZDF` stages, two `juce::ADSR` (filter, amp). Non-virtual `prepareToPlay(double,int)`; `setParams(...)` block-push. Per sample: `eF=filterEnv.getNextSample()` → `fcEff` (octaves + keyTrack) → SVF → mode select → `out·ampEnv·vel`. Lifetime keyed on `ampEnv.isActive()`. ±2 semitone pitch bend.
4. `PluginProcessor`: add `juce::Synthesiser synth` (16 × SubVoice + SubSound), `midiCollector`, `outputGain`. `prepareToPlay` (host rate, no oversampling, `setLatencySamples(0)`). `pushParamsToVoices()`. `processBlock`: clear → merge UI MIDI → push → renderNextBlock → output gain → isfinite scrub.
5. CMake: add headers; add `tests/` via `OUARICON_BUILD_TESTS` option. Render-harness scenarios 1–7 (sound, pitch, modes, slopes, env independence, keyTrack, aliasing).

**Exit 2.1:** builds; render-harness 1–7 pass; plays polyphonically; each mode/slope audibly distinct; no aliasing at high keys.

## Phase 2.2 — Self-Oscillation + Gain Compensation + Magnitude Curve

6. `SvfZDF.h`: add tanh nonlinearity on the resonant path → clean bounded sine at cutoff as `k→0`. Resonance taper so knob top reaches self-osc. `isfinite` scrub on states.
7. Resonance→k map + make-up trim (`1/(1+c·resonance)`); verify no level jump across the self-osc threshold.
8. `SubVizAnalyzer::computeMagnitudeDb(f, fcDisplay, k, type, slope)` closed-form (same g/k as audio). `displayCutoffHz`/`displayK` atomics updated from the lead (most-recent active) voice each block.
9. Render-harness scenarios 8–10 (self-osc clean, self-osc in tune at keyTrack=100%, curve-vs-measured swept-sine).

**Exit 2.2:** max res → clean sustained sine at cutoff (no blow-up/DC); keyTrack=100% in tune; closed-form curve matches measured sweep within tolerance for all modes/slopes.

## Phase 2.3 — Voice Modes + Glide + Visualization Tap

10. `MonoController` (processor-side): held-note stack, last-note priority. Mono retriggers both envelopes; Legato suppresses retrigger while held; both glide pitch over `glide` seconds (multiplicative). Route Mono/Legato to a single voice; Poly via standard Synthesiser allocation.
11. Glide: per-voice multiplicative frequency ramp; `glide=0` instant. `voiceMode` switching robust to mid-stream changes (no stuck notes).
12. `VizRing` tap: post-gain mono-sum → ring (copy-only, no alloc). Env-value atomics (`filterEnvValue`/`ampEnvValue`) from the lead voice for the Stage-3 dual-ADSR display.
13. Render-harness scenario 11 (poly/mono/legato + glide); confirm alloc-free processBlock (no `new`/lock in the hot path) and latency 0.

**Exit 2.3 (Stage 2 exit):** all render-harness scenarios pass; pluginval + auval pass; the 8 Stage-2 success criteria met.

## Success Criteria → Test mapping

| Criterion | Gate |
|-----------|------|
| Polyphonic instrument, MIDI, no crash | auval / pluginval / harness #1, #11 |
| 4 modes × 3 slopes audible | harness #3, #4 |
| Dual-ADSR independence; bipolar env; keyTrack | harness #5, #6 |
| Clean self-osc, in tune | harness #8, #9 |
| Closed-form curve == measured (QUAL-02) | harness #10 |
| No high-key aliasing (QUAL-01/DSP-06) | harness #7 |
| Poly/Mono/Legato + glide, no stuck notes | harness #11 |
| Alloc-free, latency 0 | code inspection + `setLatencySamples(0)` |
