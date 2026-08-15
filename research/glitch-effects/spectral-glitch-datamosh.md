---
title: "Spectral Glitch & Datamosh Audio Techniques"
created: 2026-08-14
last_verified: 2026-08-15
juce_version: "8.0.14"
summary: "STFT glitch primitives (freeze, smear, scramble, gate, spectral delay), spectral freeze state of the art, what datamosh audio products actually do, granular-vs-spectral trade-offs, and JUCE 8 FFT/OLA patterns."
domain: dsp
type: algorithm
keywords:
  - spectral
  - stft
  - fft
  - overlap-add
  - cola
  - freeze
  - phase-vocoder
  - datamosh
  - paulstretch
  - spectral-delay
  - birdies
  - juce
stages: [0, 2]
agents: [dsp, research]
---

# Spectral Glitch & Datamosh Audio Techniques

Level 3 research deliverable (2026-08-14). Primary target: an "O-Lossy" spectral codec-artifact plugin; also spectral modules for O-Glitch/O-Bitrot.

## 0. The substrate: realtime STFT overlap-add

Canonical JUCE walkthrough: audiodev.blog/fft-processing. Everything below sits on: input FIFO → every `hopSize` samples: window → FFT → per-bin work → IFFT → window again → OLA. Typical: `fftSize=1024, hopSize=256` (75% overlap), Hann.

Traps (each individually breaks a null test):
- **Double-windowing/COLA**: any phase-destroying processing (i.e. all of it) needs a synthesis window after IFFT. Hann² @75% overlap sums to 1.5 → multiply by `2/3` after IFFT.
- **Periodic window**: instantiate `juce::dsp::WindowingFunction` with `fftSize + 1` points, use the first `fftSize`. The symmetric version breaks exact COLA.
- **`normalise = false`** on WindowingFunction or its internal rescale breaks the 2/3 math.
- **Latency**: OLA delays `fftSize` samples (design-dependent) — `setLatencySamples()` in `prepareToPlay` (JUCE 8 `getLatencySamples()` is non-virtual). Keep OLA running while bypassed.
- **JUCE FFT layout**: `performRealOnlyForwardTransform` on `float[2*fftSize]`; only bins `0..fftSize/2` valid (interleaved re/im).
- **Null test**: identity spectrum → output minus delayed input must be silence. Validates window, periodicity, scaling at once.
- Sample-accurate FIFO — never assume host blockSize relates to hopSize; verify 512-vs-4096 block bit-identity in the harness.
- RNG: audio-thread-owned, one stream per stochastic purpose (block-size invariance).
- Stereo: decide shared vs independent phase-random streams per channel — audible mono/wide difference; make it a Width switch.

Library-grade alternative: Signalsmith DSP's perfect-reconstruction STFT (modified Kaiser).

## 1. Spectral glitch primitives

### Freeze
```text
Coherent (phase-vocoder): store mag[k] and dphi[k] = princarg(phase_now − phase_prev)
  each frame: phi[k] += dphi[k];  resynthesize mag·e^{iφ}
Stochastic (paulstretch): phi[k] = uniform(0, 2π) each frame  → breathy "whisperization"
```
Coherent alone goes metallic (every bin loops at fixed period). **State of the art: small randomized phase-increment jitter** `phi[k] += dphi[k] + ε·uniform(−π,π)`, ε scaled up with frequency (highs tolerate/need jitter; lows need coherence). Signalsmith Stretch does the same at product quality ("randomise slightly for longer stretches"). Further refinements worth stealing: sum complex values (not raw phases) so strong tones dominate; predict phase forwards in time AND upwards in frequency; lock phase locally 1:1 around spectral peaks so harmonics stay glued. Never freeze a transient frame: gate capture on low spectral flux, or cycle 2–4 captured frames.

### Smear / accumulation
```cpp
smoothedMag[k] = std::max(mag[k], decay * smoothedMag[k]);  // peak-hold smear (reverb-like wash)
```
Mix frozen/smeared layers as **complex spectra in rectangular form** — never add mags and phases independently (KVR t=489310's resolved bug).

### Scramble (SpectralSuite BinScrambler)
Permute bins within a range; re-roll the permutation every 4–30 frames (every frame = noise). Move the whole complex pair (clickless) vs magnitude-only (smoother). Keep the low end out of range.

### Phase randomize / phase lock
Randomize phases, keep mags = whisperization (input's envelope on noise); partial `phi += r·uniform(−π,π)` is a dry-to-whisper macro. Phase lock (hold phases or dphi from a captured frame) = amplitude rhythm survives, pitch content locked (SpectralSuite PhaseLock).

### Gate / threshold — the codec sound
```cpp
if (mag[k] < T) mag[k] = 0;          // hard gate → birdies (bins toggling frame-to-frame)
// or keep top-N bins by energy — reproduces "loud parts steal all bits"
```
Pushed hard = the MP3 underwater/birdie sound. Hysteresis or 2–3-frame gain smoothing tames birdies (or don't — it's the glitch). Threshold against a psychoacoustic weighting curve for codec authenticity (Goodhertz Lossy exposes exactly this switch).

### Bin quantize / decimate
Magnitude staircase (quantize in dB sounds more natural; fizzy gated floor) vs grouping adjacent bins to their max (vocoder-ish "low-bitrate" pitch smear). Dither/hysteresis the boundaries against frame flutter.

### Spectral delay (DAFx-04 "Spectral Delays", dafx.de/paper-archive/2004/P_042.PDF)
Per-bin delay in FRAMES into a ring of past spectra; chirpy dispersive echoes.
```cpp
out[k] = frames[(writeIdx - delayFrames[k]) % F][k];
frames[writeIdx][k] = in[k] + fb[k] * out[k];   // per-bin feedback — clamp or single bins explode
```
Delay resolution quantized to hop (1024 @44.1k → 23.2 ms steps). Memory: F × numBins × 8 B (4 s @1024/256 ≈ 2.9 MB/ch) — allocate in prepareToPlay. Per-bin feedback is a sticky-NaN risk: NaN-probe it.

### Stretch (paulstretch)
Frame history + fractional read pointer at 1/stretch frames per hop; interpolate mags between stored frames; randomized phase. Realtime "stretch" in a fixed-latency plugin is necessarily a freeze with a moving capture point.

## 2. Datamosh audio — what products actually do

**No audio product literally corrupts codec bitstreams in realtime.** The metaphor is implemented with: (a) buffer capture/repeat (P-frame repetition analog), (b) spectral gating/smearing (macroblock loss analog), (c) granular relooping + degradation.

- **Puremagnetik Driftmaker** (explicitly datamosh-inspired): rolling buffer → wrong-segment recall (Parse/Chop) → granular stretch → analog-style deterioration. Time-domain, not FFT.
- **Goodhertz Lossy** — the design template (manuals.goodhertz.com/3.13/lossy/). Standard = psychoacoustically-weighted spectral gate + smear; **Inverse = output what was removed** (`input − gated` residual — trivial and brilliant); Phase Jitter = per-bin clocking perturbation; **Packet Loss** = zero N frames; **Packet Repeat** = re-emit last good frame (freeze) while input runs on — the I/P-frame analog; **Loss Speed** = frame rate of the loss path ("slower = more smear, faster = more garbled"); bitrate presets = threshold + high-cut pairings; Auto Gain compensates removed content.
- **Glitchmachines Hysteresis**: stutter engine **inside the delay feedback loop** — each pass re-glitches its own history ("disintegration over generations"). Memory-pattern tie-in: effects inside a feedback loop change decay-per-generation; tune/limit inside the loop.
- **Fracture**: buffer stutter + multimode filter + 3 LFOs + delay (free, simple).

**Unclaimed design space**: emit a stale frame's **magnitudes** with the **current** frame's phase-deltas applied — the motion (phase evolution) of live audio driving the content (mags) of a stale frame. The closest true analog of datamosh motion vectors; per this research no shipping product markets it. Candidate marquee feature for O-Lossy.

**Codec-corruption recipe**: treat each STFT frame as a packet; maintain `lastGoodFrame[]`; per sequencer/probability, a frame is "lost" → silence | repeat-with-phase-accumulation | stale-mags-live-phase-deltas hybrid.

## 3. Granular vs spectral for glitch

| Concern | Time-domain granular/buffer | Spectral (STFT) |
|---|---|---|
| Transients | preserved bit-exact in a grain | smeared; PV "fails for percussive" |
| Latency | zero–one grain | ≥ fftSize (~23 ms @1024/44.1k) |
| CPU | trivial | 2 FFTs/hop/ch + per-bin |
| Freeze wash / whisper / bin games | can't | only way |
| Polyphonic pitch/stretch | poor | good |

Rules: beat-synced stutter/retrigger/tapestop/reverse/chop → always time-domain (why Fracture, Hysteresis, dblue Glitch, Driftmaker all are). Freeze pads, whisper, gate-underwater, scramble, spectral delay → spectral only. **Hybrid**: time-domain stutter engine around the spectral path, sharing one capture ring, dry/granular path delay-compensated by the STFT latency; report one combined latency.

## 4. Reference implementations

- **maxsolomonhenry/FftBuffer** (JUCE) — cleanest small reference: `OlaBuffer` base class owns all overlap-add, subclass implements `processFrameBuffers()`; spectral freeze with Stutter Rate (freeze refresh in Hz/beats) and Envelope Depth (frozen layer follows wet envelope — very effective musicality trick). Architecture worth copying wholesale.
- **andrewreeman/SpectralSuite** (JUCE, CMake) — seven per-bin plugins on one shared spectral framework with generated boilerplate — the proven pattern for shipping a *family* of spectral effects on one OLA core.
- **audiodev.blog FFTProcessor** — single-file STFT skeleton with the gotchas handled.
- **Signalsmith DSP** (signalsmith-audio.co.uk/code/dsp) — production STFT; stretch-design write-up (signalsmith-audio.co.uk/writing/2023/stretch-design/) documents the phase-prediction refinements.

## 5. Harness probes (Stage 2 gate)

Null test (identity spectrum) · freeze spectral-centroid stability over 10 s · COLA flatness (DC in → constant out) · block-size invariance 512 vs 4096 · NaN-probe pathological input into per-bin feedback.

## Key sources

audiodev.blog/fft-processing · kvraudio.com/forum/viewtopic.php?t=489310 (freeze recipe; rectangular-form mixing) · signalsmith-audio.co.uk/writing/2023/stretch-design · dafx.de/paper-archive/2004/P_042.PDF (spectral delays) · manuals.goodhertz.com/3.13/lossy · puremagnetik.com Driftmaker · glitchmachines.com Fracture/Hysteresis · github.com/maxsolomonhenry/FftBuffer · github.com/andrewreeman/SpectralSuite · PhaVoRIT (hci.rwth-aachen.de/publications/karrer2006a.pdf) · "Phase Vocoder Done Right" (arxiv.org/pdf/2202.07382) · polarity.me paulstretch explainer
