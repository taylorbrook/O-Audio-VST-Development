# Stage 2 / Phase 2.3 — Spectral-Decay + Bit-Depth + Viz Tap — SUMMARY

**Date:** 2026-06-22
**Goal:** The three remaining DSP pieces — the spectral-decay macro (DSP-04), the bit-depth quantizer (DSP-05), and the real-time-safe visualization tap (PERF-01) with an active-spectrum snapshot for the Stage-3 drawbar display (QUAL-02). Completes Stage 2.
**Gate (incremental-DSP mode):** clean build + `auval` — **MET** (VST3 + AU build clean, `aumu OSiA OuDv` → AU VALIDATION SUCCEEDED incl. MIDI test).

## What was implemented

### `source/AdditiveVoice.h`
- **Spectral-decay macro** — internal per-voice ramp `tau` (0→1 from note-on over `kTauRampSeconds = 2.0 s`), advanced at **control rate** in `renderNextBlock` (`tau = jmin(1, tau + numSamples/(kTauRampSeconds·fs))`), reset to 0 in `startNote`. Per-partial multiplier composed into `refillTable()` **at the marked 2.3 extension point**, after the morph and **before** band-limit + sum: `D_k = exp(-rate·k·tau)` with **0-based** `k` (k=0 fundamental → D=1, never decays; k=15 decays fastest). `activeK = morphed_k · D_k`. Effective `rate = (spectralDecay + velLevel·velToDecay)·kDecayRateMax`, clamped ≥ 0, with `kDecayRateMax = 0.35f` (at tau=1: `D_15 = exp(-0.35·15) ≈ 0.005 ≈ −46 dB` — a strong, clearly audible/visible darkening).
- **Refill cadence** — `renderNextBlock` re-dirties the table every block while `rate > 0 && tau < 1`, so the changing decayed spectrum is tracked. When `spectralDecay==0 && velToDecay==0` → `rate==0` → this branch never runs → the Phase-2.2 once-per-note / scan-driven cadence is preserved exactly.
- **Bit-depth quantizer** — applied at **read time** per sample in `renderNextBlock`, **after** `readTableLinear(phase)` and **before** the amp env: `bitDepthBits==0` (Off) → passthrough; else mid-tread `s = round(s·L)/L`, `L = 2^(bits-1)`, no dither. `L`/`1/L` precomputed once per block (not per sample).
- **`setParams` extended** — signature now `(frameA[16], frameB[16], scanBase, scanEnvAmount, spectralDecay, velToDecay, bitDepthBits, ampParams, modParams)`. Decay sources stored raw (voice combines with its own velocity); `bitDepthBits` is the resolved bit count (0 = Off), resolved from the choice index in the processor.
- **Active-spectrum snapshot** — `float activeSpectrum[16]` stored each `refillTable()` as the post-morph, post-decay, **pre-band-limit, pre-norm** amplitudes (the conceptual "what the engine is summing" bars). Accessors: `getActiveSpectrum()`, `isAmpActive()` (= `ampEnv.isActive()`; deliberately NOT named `isVoiceActive` — that is a `juce::SynthesiserVoice` virtual used for voice-stealing, shadowing it would hijack allocation), `getNoteAge()`.
- **Primary-voice selection** — `std::uint64_t noteAge` per voice, stamped in `startNote` from a shared `static inline std::atomic<std::uint64_t> sNoteCounter` (`noteAge = ++sNoteCounter`) — avoids touching the fixed `SynthesiserVoice::startNote` override signature. Strictly increasing per note-on → the newest sounding note wins.

### `source/AdditiveVizAnalyzer.h` (NEW)
- Lifted **verbatim** from `O-simpleFM/source/FmVizAnalyzer.h`: `VizRing` (lock-free power-of-two atomic overwrite ring, 8192) kept exactly as-is; analyzer class renamed `FmVizAnalyzer` → `AdditiveVizAnalyzer` (4096 / Blackman-Harris FFT + 1024→128 max-abs scope downsampler). File header retargeted to O-simpleAdditive. Header-only; in Stage 2 the processor instantiates **only `VizRing`** (the analyzer is defined-but-not-constructed until the Stage-3 editor wires it).

### `source/PluginProcessor.{h,cpp}`
- **Viz tap** — `VizRing viz;` member + pre-allocated `std::vector<float> monoScratch` (sized to `samplesPerBlock` in `prepareToPlay`). In `processBlock`, **after** the output-gain ramp and the NaN scrub: mono-sum the output block into `monoScratch` (channel 0 if mono; equal-weight average otherwise) and `viz.write(...)`. Copy-only — no alloc/FFT/locks on the audio thread; the copy length is clamped to the prepared scratch size (never reallocates if a host overruns the prepared block).
- **Active-spectrum snapshot** — `std::array<std::atomic<float>,16> activeSpectrumSnapshot`. In `processBlock`, **after** `synth.renderNextBlock`, scan all voices, pick the **active** voice with the largest `noteAge`, and `store` its `getActiveSpectrum()` (relaxed). If no voice is active the snapshot is left unchanged (Stage 3 handles idle). This read of voice state is on the audio thread (same thread as render) → no voice→processor atomics; only the snapshot crosses to the editor.
- **Public accessors (for Stage 3)** — `getVizRing()` (const ref to the ring for the editor's analyzer) and `readActiveSpectrum(float* dest16)` (copies the 16 atomics, relaxed).
- **`pushParamsToVoices`** — now also reads `spectralDecay`, `velToDecay`, and resolves the `bitDepth` choice index → bit count via `kBitTable = {0,12,10,8,6,4,2}`; passes them through the extended `setParams`.

## No-regression argument (spectralDecay=0 & bitDepth=Off ⇒ bit-identical to 2.2)
- **Spectral decay:** `spectralDecay=0 && velToDecay=0` ⇒ `effectiveDecayRate()=0` ⇒ `rate=0` ⇒ the decay branch in `renderNextBlock` never runs (tau stays 0, no extra dirtying) ⇒ `refillTable` uses `D_k = 1.0` for all k (the `rate > 0.0f ? exp(...) : 1.0f` short-circuit, so `std::exp` is not even called). `activeK = morphed_k · 1.0 = morphed_k`. The morph → band-limit → norm → sum math is unchanged and unre-ordered; only `D_k` is multiplied in between morph and band-limit. ⇒ table output is **bit-identical** to Phase 2.2.
- **Bit depth:** default choice index 0 (Off) ⇒ `bitBits=0` ⇒ `quantOn=false` ⇒ the per-sample read is `s = readTableLinear(phase)` with no quantization (the `if (quantOn)` body is skipped) ⇒ **bit-identical** to Phase 2.2.
- **Refill cadence:** with rate=0 the table is dirtied only by Frame A/B changes (setParams) and scan motion (`kScanRefillEps`) — exactly the Phase-2.2 triggers. Static patch ⇒ once per note.
- **Viz tap / snapshot:** read-only — never written back into the audio path; the snapshot publish is post-render and does not touch the buffer. Output audio is unaffected.

## Verification
- **Build:** VST3 + AU compile clean. Only the pre-existing Stage-1 `processorRef` unused-private-field warning (from the `GenericAudioProcessorEditor` placeholder — resolves when the WebView editor lands in Stage 3). DSP code warning-free. (Fixed during this phase: an accidental `isVoiceActive` override of the JUCE `SynthesiserVoice` virtual → renamed `isAmpActive`.)
- **auval:** `aumu OSiA OuDv` → **AU VALIDATION SUCCEEDED**. Render tests pass at 11025/22050/44100/48000/96000/192000 Hz, mono + stereo, 64–4096 frames; bad-max-frames handled; parameter set/ramp PASS; **MIDI test PASS** (the new spectral-decay + bit-depth + viz-tap path exercised without NaN, denormal, or crash).

## Audible / visual criteria — for DAW/Standalone confirmation (not auval-checkable)
- [ ] **DSP-04 spectral tilt:** raise `spectralDecay`; a held note audibly darkens over ~1–2 s (upper partials fade); at 100% the top harmonic drops ≈ −46 dB by tau=1. At `spectralDecay=0` the spectrum is steady (balance holds).
- [ ] **DSP-04 velocity→decay:** with `velToDecay>0`, harder notes darken faster than soft notes.
- [ ] **DSP-05 bit grit:** step `bitDepth` down (12→2); the tone gains progressively coarser early-digital quantization grit; `Off` is clean (the reference). Quieter tails show more relative grit (fixed-step quantizer — faithful).
- [ ] **No-regression:** default patch (`spectralDecay=0`, `bitDepth=Off`) sounds identical to Phase 2.2.
- [ ] **Viz tap feeds ring + snapshot (Stage 3 confirmation):** the VizRing receives the post-gain mono sum and the active-spectrum snapshot tracks the newest sounding voice's morphed + decayed bars.

## Consumed by Stage 3
The **active-spectrum snapshot** (`readActiveSpectrum`) drives the exact drawbar spectrum display (the morphed + decayed bars, not the raw drawbar params — QUAL-02). The **`VizRing`** (`getVizRing`) feeds an `AdditiveVizAnalyzer` on the editor's 30 Hz Timer for the oscilloscope + optional FFT spectrum overlay. Both accessors are defined now; the Stage-3 WebView editor instantiates the analyzer and emits the frames.

## Files changed / created
- **Modified:** `plugins/O-simpleAdditive/source/AdditiveVoice.h`
- **Modified:** `plugins/O-simpleAdditive/source/PluginProcessor.h`
- **Modified:** `plugins/O-simpleAdditive/source/PluginProcessor.cpp`
- **Created:** `plugins/O-simpleAdditive/source/AdditiveVizAnalyzer.h`
- **Created:** `plugins/O-simpleAdditive/.planning/stages/2-dsp/SUMMARY-2.3.md` (this file)
