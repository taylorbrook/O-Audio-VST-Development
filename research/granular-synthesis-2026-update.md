# Granular Synthesis — 2026 Update & House Practice

**Companion to:** `granular-synthesis-state-of-the-art.md` (2026-03-06). Nothing here repeats that file. It covers what has appeared since then, the topics the March report skims, and how our own engines measure up.
**Date:** 2026-10-26 · **Method:** `/research` Level 3, three parallel investigations (new work since March 2026 · topic deep-dive · code audit of our granular plugins), then a synthesis pass.
**Confidence:** HIGH on the audit findings marked *(spot-checked)*. MEDIUM on the field survey: dates are marked [V] verified or [S] from search summaries only. CPU figures are estimates and need measuring.

---

## 0. TL;DR

1. **Our biggest problems are basic engine correctness, not missing features.** No engine has working anti-aliasing for grains pitched up. Four different loudness-vs-density laws are in use (two of them are "none"). Two engines quantise grain onsets to the block. Every engine is time-seeded, so no render is reproducible. Every voice steal is a hard cut.
2. **The fix is a shared `grain-engine` module** seeded from O-ReverseDelay's hardened parts (window LUT plus norms, xorshift streams, sample-accurate scheduler), plus a band-limited reader and fade-on-steal. Five plugins re-implement the same five pieces today, and each carries a different subset of the fixes.
3. **What's new in the field (2026):** latent/codec-space granular (Sony CSL, ISMIR LBD on JUCE + RTNeural, DAFx26 neural morphing); audio-rate "grain-as-oscillator" engines (Grainferno, Phonon); grain-skip probability and grain-shape tilt (Tempera 2.3); a buffer library kept separate from presets (GRN 3).
4. **Cheapest high-value wins:** fractional-onset scheduler plus counter-based RNG (§3.1); closed-form density compensation (§3.2); grain skip probability and shape tilt (§5).

---

## 1. Field update, March → October 2026

### 1.1 Academic
| Work | Venue | Idea | Relevance |
|---|---|---|---|
| Pichard & Lattner, *Descriptor-Aware Latent Granular Mosaicing* | AIMC 2026 [V] — https://zenodo.org/records/22277800 | Grains are picked in an autoencoder latent space by latent distance plus 4 steerable descriptors (loudness, transientness, brightness, flatness) | The descriptor steering works **without** the neural part. See §3.5 |
| Karystinaios, *Neural Morphing* | DAFx26 demo [V] — https://dafx26.mit.edu/assets/papers/DAFx26_demo_63.pdf | Grains made of codec tokens (DAC, G=7 frames, H=2). Codebooks split coarse/mid/fine. Beam search keeps the grain sequence continuous. Shipped as VST3/AU | Picking each grain as part of a sequence beats picking it greedily |
| Eriksen, Segovia, Blackett | ISMIR 2026 LBD [V] — https://github.com/Grashopr-888/ISMIR_LBD_2026_submission | 103k-param MLP distilled from the music2latent encoder. Grains mixed every 64 samples. Corpus search on a worker thread every 93 ms. **No decoder**: stored grains + 10 ms crossfades. 8-bit latent table = 2.6–3.1× faster search | **Built on JUCE 8 + RTNeural.** The most plugin-ready neural design so far |
| Nielsen, *Keyframe Time Stretching via Extrema Sampling* | DAFx26 [V] — https://dafx26.mit.edu/assets/papers/DAFx26_paper_21.pdf | How tightly the local peaks and troughs are packed sets the OLA crossfade length, with no FFT | Grain length that adapts to the material, at O(1) per sample. Open firmware: Capicola |
| Abate & Hansen, *Concatenation-Driven Convolution* | DAFx26 [V] | Interpolate FFT kernels inside **one** partitioned engine instead of crossfading two | Only matters for a granular IR/reverb stage |
| Tokui & Baker, *Latent Granular Resynthesis using Neural Audio Codecs* | arXiv 2507.19202 (Jul 2025) [V] | The base paper for the latent line above | Missed in the March report |

DAFx26 had no paper that was mainly about granular synthesis. ICMC and SMC 2026 proceedings could not be found. No 2025–26 AES work on anti-aliasing pitched grains turned up.

### 1.2 Commercial
| Product | What's actually new |
|---|---|
| **Baby Audio Grainferno** (Mar 2026 [S], v1.2 Jul [V]) | Audio-rate grain triggering, so the grains become an oscillator. Keyboard pitch plus Auto-Pitch to C. **Per-grain morph between 2 sources** (7 modes). Per-grain FX. An exponential window was added in v1.1 |
| **PolyFreq Phonon** (May 2026 [S]) | A synchronous "graintable" engine. **Grain rate and density as separate controls.** Window morphs into a rectangle. 256 grains per voice |
| **Bitwig 6.1 Sampler "Fragments"** (Aug 2026 [V]) | 256 grains per voice, each with its own rate, direction, position and size. A Spectral play mode with formant processing |
| **FRCTL GRN 3.x** (Jun–Jul 2026 [V]) | Effect becomes a polyphonic instrument. **A buffer library kept separate from presets.** Modulation sources add together. 16-step grain sequencer |
| **Beetlecrab Tempera 2.3** (Jun 2026 [V]) | **Grain skip probability** (0–100%). **Grain-shape tilt** from plucked to reverse-swell. Modulators can modulate modulators. A visual "sparkle" per grain |
| **Thephonoloop Outlines** (Jul 2026 [V]) | "Sprinkle": after a time, a normal grain turns into a short, separately shaped fragment |
| **Tasty Chips GR-2** (May 2026 [S]) | 4 layers × 16 voices × 128 grains, a spectral engine, USB 8-in/10-out |
| **Oddment ODD-1V** (Sep 2026 [V]) | 4 playheads, each with its own speed, pitch and time path. A 1:1 port of the hardware |

Not found: any granular update from NI, Output, Spitfire, Sonic Charge, Tracktion, Make Noise, 1010music, or Torso. Pigments 7 (Dec 2025) did not change its granular engine. The March report also missed Ableton Granulator III (MPE plus real-time capture), released with Live 12 in 2024.

### 1.3 Open source worth reading
- **Capicola** (AGPL-3.0, STM32/libDaisy) — https://github.com/heavylight-industries/capicola — reference code for the extrema stretcher. It also has Hermite and B-spline delay reads with derivatives, and a TKEO transient detector.
- **Oi-Grandad V2** (HISE, GPL-3.0, 411★) — https://github.com/publicsamples/Oi-Grandad — 4 playheads per voice plus a waveguide resonator per voice. The most active granular plugin repo found.
- **Hairball** (JUCE, AGPL-3.0) — https://github.com/SpeakingColors/hairball — the test setup is the interesting part: pure C++ engine, a JS port, a parity tool and 280 checks.
- **Mutable Instruments Clouds** — `clouds/dsp/granular_sample_player.h` — still the clearest shipped reference for density normalisation and tiered grain quality (§3.2, §3.4).

---

## 2. Our engines: survey and gaps

O-Strata has no granular code (it is wave-terrain). O-Texture is a VAE decoder doing Hann OLA at 50%. **O-ReverseDelay** is not a granular plugin by name, but it is our best-hardened grain engine (`WindowLut.h`, `GrainScheduler.h`) and serves as the house reference.

### 2.1 Comparison
| | O-simpleGrain | O-TextureForge | O-GrainScatter | O-Freeze | O-Polystutter |
|---|---|---|---|---|---|
| Buffer | Static sample, mono, 10 s | Corpus (KD-tree, UMAP/PCA) | Live 12 s ring + 2 s freeze snapshot | Live 2 s ring | 5 s ring per lane + slice snapshot |
| Scheduling | Per-sample countdown; **spawn position taken at block start** | **Block-quantised** | Sample-accurate | Per-sample counter | **Block-quantised** (WR-04/07 open) |
| RNG | Time-seeded `juce::Random` | Time-seeded | 2 time-seeded streams | Time-seeded | Time-seeded |
| Steal | **Hard restart** | **Hard cut**, also on note-off | **Hard cut** | Round-robin restart (IN-03) | n/a |
| Envelope | 2048-point LUT + Tukey remap ✅ | `std::cos` per sample | `switch` formula per sample; ExpDecay starts at 1 ❌ | Hann table, **rebuilt under live grains** ❌ | 5 ms linear fades |
| Overlap norm | 1/√(size·density): ignores pool cap and window μ₂ | **None**, hard clip ±1 | **None**, `tanh` | ÷Σw (coherent law) | n/a |
| Interpolation | Lagrange-4, double position | Linear, **float** position | Lagrange-4, **float** position | Linear, double | Linear |
| Pitch-up AA | **One-pole after the read: ineffective** | None | None (up to +36 st, Doppler 2×) | n/a (±50 ct) | None (±24 st) |
| RT handoff | Atomic pointer + retired list, gen fence | Atomic pointer, **one retired slot, no fence** | Preallocated | Preallocated | Preallocated |

### 2.2 Verified gaps, by audibility
1. **GrainScatter freeze replays the same material every grain** *(spot-checked)*. The frozen branch reads `freezeMgr.readSample(ch, v.readPosition)`, and `readPosition` starts at 0 or len−1 (`O-GrainScatter/Source/dsp/GrainPool.h:182-185, 224-227`). `positionOffset` is used only in the live branch. Result: a periodic buzz at the spawn rate, which is failure mode #1 in `pattern_freeze_needs_loopback_write`. The loop wrap is also an un-crossfaded `fmod` (`FreezeManager.h:77`), and grains still sounding at release switch to the live buffer mid-grain.
2. **No engine band-limits pitched-up grains.** simpleGrain filters *after* the decimating read *(spot-checked: `O-simpleGrain/Source/GrainVoice.h:299`)*, and only at 6 dB/oct, so the aliasing has already happened. The others don't filter at all.
3. **Loudness changes with density** in four different ways. TextureForge clips *(spot-checked: `PluginProcessor.cpp:210`)*. GrainScatter applies `tanh`, so density acts as a distortion control. O-Freeze's ÷Σw turns Grain Count into a level control, and with one live grain it removes the window entirely. simpleGrain under-reads once overlap goes past the 24-grain cap (500 ms × 200 g/s → −20 dB applied while ≤24 grains are live).
4. **Steal and note-off are hard cuts** in simpleGrain (`GrainVoice.h:376`), TextureForge (`GrainVoice.h:138,151`), GrainScatter (`GrainPool.h:167`) and O-Freeze (IN-03).
5. **Onsets are quantised to the block** *(TextureForge spot-checked: `GrainScheduler.cpp:149-155`, so stacked drone grains all fire at offset 0)*. Polystutter's sync and MIDI triggers fire up to one block late. simpleGrain's scan position depends on block size. GrainScatter drops sync repeats that fall past the block end (`GrainScheduler.h:130-140`).
6. **Float read positions on long rings.** TextureForge (`GrainVoice.h:40`) and GrainScatter (`DelayBuffer.h:53`): on a 12 s ring a float step is 0.06–0.125 samples. simpleGrain v1.1.2 and O-Freeze v2.3.0 have already fixed this.
7. **Smaller items.** O-Freeze rebuilds its window under live grains (`PluginProcessor.cpp:296-312`). Polystutter clicks on pitched-down repeats (inferred). O-Texture runs a 48 kHz model at the host rate with no resampling (44.1 k → −1.5 st). simpleGrain's reaper has no entry counter (`pattern_generation_reaper_freezes_when_host_idles_audio_thread`). TextureForge's single retired slot can race on two fast loads (narrow window).
8. **Test coverage.** No harness varies the block size, and none seeds the RNG. simpleGrain's own CHANGELOG v1.4.2 says its render metrics "are not byte-comparable run to run".

### 2.3 Bug classes that keep coming back (look for these first in any new granular work)
Hard-cut steals · freeze/capture region overwritten or looping on itself · allocation on the audio thread (`atomic_load(shared_ptr)`, `reset()` realloc, vector growth, `makeLowPass`) · float read positions · reads past the write head or past the buffer edge · events quantised to the block · clicks at envelope or crossfade edges.

---

## 3. Techniques the March report covers only briefly

### 3.1 Sample-accurate scheduling + reproducible renders (do this first)
```
for n in block:
    nextOnset -= 1
    while nextOnset <= 0:
        δ = -nextOnset                    // fractional onset ∈ [0,1)
        g = allocVoice(); g.t = δ; g.pos = start + rate·δ   // offset BOTH window and read
        nextOnset += interOnset(hash(seed, grainIdx, SLOT_IOI))
        grainIdx += 1
```
- **Counter-based RNG:** `u = hash(seed, grainIdx, slot)` (splitmix64 or Philox). Each random grain attribute reads its own slot, so the order of draws can't drift with block size. This makes the RNG-interleave and common-random-number traps impossible by construction.
- While the transport plays, derive `grainIdx` and the scheduler phase from the absolute sample position, so loops and bounces reproduce exactly.
- Persist the seed in plugin state, and offer a "Fixed / Random seed" option (Clouds has `use_deterministic_seed`).
- **Why fractional onsets matter:** with integer onsets the timing error e has RMS 0.29 samples. At 5 kHz that gives ≈0.19 rad of phase error, so spurs sit only about −14 dB down. Fractional onsets are mandatory for audio-rate, pulsar and FM grain modes.
- **Gate:** render the same input at block sizes 32, 512 and a variable size, on fresh instances, and null them. The residual must be ≤ −120 dBFS.

### 3.2 Loudness vs. density: closed form
Mean overlap N = D·T. Window moments over normalised time: μ₁ = ∫w, μ₂ = ∫w².
- Coherent grains (synchronous, aligned): **g = 1/(N·μ₁)**. Incoherent grains (sprayed, Poisson onsets): **g = 1/√(N·μ₂)**. Partly correlated: P ∝ N·μ₂ + ρN(N−1)μ₁².

| Window | μ₁ | μ₂ |
|---|---|---|
| Rect | 1 | 1 |
| Hann | 1/2 | 3/8 |
| Triangle | 1/2 | 1/3 |
| Welch | 2/3 | 8/15 |
| Trapezoid (ramp a/side) | 1−a | 1−4a/3 |
| Tukey (taper α) | 1−α/2 | 1−5α/8 |

*Derived values: check them numerically before relying on them.* (Sanity check: Tukey at α=1 gives Hann and at α=0 gives Rect, as it should.)
- **Recipe:**
  - Default to the incoherent law, and crossfade toward the coherent law as spray → 0 or sync turns on (a stand-in for the correlation ρ).
  - Use the **live grain count N, capped by the pool size**, not the nominal overlap. That is simpleGrain's bug.
  - Never boost when N < 1.
  - Smooth with a one-pole of 50–200 ms.
  - On any feedback tap, use the amplitude law (O-ReverseDelay's `shapeNorm` vs `getLoopNorm` split).
- Clouds ships essentially this: `rsqrt(n_active − 1)` × a window-shape term, one-pole smoothed.

### 3.3 Anti-aliasing for pitched-up grains
- Taps per output sample grow as 2·N_z·r. At r=4 that is about 1.6 GFLOP/s for 256 voices, which is not viable.
- **Use a mipmapped source instead:**
  - Build octave levels with half-band filtering and ÷2 decimation. That is about 2× the memory: build them off-thread for samples, or run a half-band cascade on the write path for rings.
  - At grain onset, latch level ℓ = ⌊log₂ r⌋. The residual ratio r′ ∈ [1,2) then needs only a small polyphase sinc kernel set indexed by cutoff (4–8 sets).
- **Compensate the half-band group delay D_ℓ** (p_ℓ = p₀/2^ℓ − D_ℓ), or grains jump in time when the level changes.
- Normalise every polyphase phase to unity DC gain (JUCE WindowedSinc is 0.990: `critical_juce_windowedsinc_dc_gain_is_0990`).
- Tier the quality: 16 taps for "HQ" voices and 8 taps beyond a budget. That is about 5% of a core at 64 voices *[estimate]*.

### 3.4 Freeze, time domain and spectral
- **Granular freeze:** position jitter must be **≥ about one grain length**. Smaller jitter rereads nearly the same samples at a fixed hop, which makes a comb at 1/hop; GrainScatter's jitter is zero. Add ±3–10 ct detune per grain and Poisson onsets. At N ≥ 4 the texture turns smooth.
- **Spectral freeze without the metallic sound** — combine all four:
  1. Average magnitudes over 4–16 frames around the freeze point.
  2. Drift each bin's magnitude ±1–3 dB with independent random walks at 0.2–3 Hz.
  3. Advance phase as φ ← φ + ω̂H + β·U(−π,π), with β exposed as "Diffusion": 0 is a phase-vocoder freeze, 1 is a PaulStretch wash.
  4. Use windows ≥4096 (8–32k for pads) at 4× overlap, normalised by 1/1.5 for Hann×Hann.
- Keep the DC and Nyquist bins real. Use phase locking (Laroche & Dolson 1999) when β is near 0.
- The FFT cost is trivial, about 10 MFLOP/s. The latency is the window length W, so use large windows only in freeze and stretch modes.

### 3.5 Corpus and descriptor navigation (most of this is already in TextureForge)
- **Descriptors:** loudness (weight it low, or use it only as a filter), confidence-gated pitch, log₂ spectral centroid, flatness in dB, MFCC 1–4 without c0, and spectral flux.
- **Normalisation and weights:** normalise robustly, (x−median)/IQR. Apply weights by pre-scaling each dimension by √w, so the tree stays plain Euclidean.
- **Search:** keep the KD-tree at ≤8 dimensions (reduce first). Cache k=8–16 candidates **per block, not per grain**, then pick by softmax(−D²/τ) with a short ring that penalises recently used grains (k=1 sounds like a machine gun).
- **Snapshot:** publish {audio, units, tree, generation} as **one** snapshot, and reap with generation plus an entry counter.
- **The 2026 latent mosaicing papers boil down to steerable descriptor weights**, and we can ship that without a model.

### 3.6 Pulsar / grain-rate modulation
- A pulsaret of duty d inside period P = 1/f_p gives fundamental f_p and formant ≈ f_p/d. Both can be controlled independently, and the rate can be FM'd at audio rate with a phase accumulator and fractional onsets.
- Clamp d ≥ f_p/(0.45·fs) to keep the formant below Nyquist. Use Blackman-Harris for pulsarets under about 1 ms.
- Variants (Roads, *Microsound*): glisson (pitch glide inside the grain), grainlet (length = c/f, constant-Q clouds), trainlet (BLIT content). Masking pulses on and off gives subharmonics.
- Analytic grains cost about 20 flops per sample per grain with no source reads, so 256 grains is easily affordable.
- This is the same idea behind 2026's Grainferno and Phonon audio-rate engines.

### 3.7 Neural / latent granular: keep it a spike
- Running the grain logic in latent space at control rate and feeding **one** streaming decoder per instance through ANIRA is feasible. The latent frame rate (23–86 Hz) puts a floor on grain length, latency is ≥ 10–45 ms, and the licences of pretrained weights need checking against AGPL.
- **The ISMIR LBD design avoids the decoder entirely**: it searches latents but plays stored grains. That is the most practical variant and maps onto TextureForge's architecture almost directly.

---

## 4. Recommended shared module: `modules/synthesis/grain-engine`

```cpp
struct GrainSpec { double startPos; float rate; int lengthSamples; int shape; float taper;
                   float gain, panL, panR; bool reverse; };
class GrainSource { virtual float read (int ch, double pos) const noexcept = 0;   // double positions throughout
                    virtual double validSpan() const noexcept = 0; };            // bound for the read guard
class WindowLut   { float read (int shape, float phase, float taperEnd) const noexcept;
                    float mu1 (int shape, float a) const noexcept;               // amplitude law (feedback)
                    float mu2 (int shape, float a) const noexcept; };            // power law (output)
class GrainScheduler { void prepare (double fs, uint64_t seed);                  // counter-based RNG
                       int schedule (int n, const SchedParams&, SpawnEvent* out, int cap) noexcept; };  // fractional onsets
template <int N> class GrainPool {
    void spawn (const GrainSpec&) noexcept;               // steal = ~2 ms fade, never a hard cut
    void render (const GrainSource&, float* L, float* R, int n) noexcept;  // applies live-N norm
    void releaseAll (int fadeSamples) noexcept; };
// Optional parts: BandlimitedReader (mipmap + polyphase), FreezeRing (loop-back write, latched length,
// crossfaded wrap), RetiredReaper (generation + entry counter), test kit (block-size null, seeded determinism).
```
Each plugin keeps its own policy: MIDI, sync and corpus scheduling, and spatial placement. GrainScatter's HOA path takes the mono output of each grain.
**Risk:** loudness and timbre will change. Migrate one plugin per `/improve` release, behind a null or A/B test against the old binary.

---

## 5. Ranked roadmap (effort → payoff)

| # | Item | Effort | Payoff | Where |
|---|---|---|---|---|
| 1 | Fix the GrainScatter freeze (position, jitter ≥ grain length, crossfaded wrap, release fade) | S | Very high (audible bug) | O-GrainScatter |
| 2 | Fade-on-steal and fade-on-note-off | S | High (clicks) | simpleGrain, TextureForge, GrainScatter, Freeze |
| 3 | Fractional-onset scheduler + counter RNG + block-size null gate | S–M | Very high (needed by every other item) | module seed → TextureForge, Polystutter first |
| 4 | Closed-form live-N density compensation | S | High | all; simpleGrain cap bug |
| 5 | Mipmap + polyphase band-limited reader | M | High for pitch-up | module; GrainScatter (+36 st) first |
| 6 | Grain skip probability + shape tilt (Tempera) | XS | Medium–high, cheap | any engine on the module |
| 7 | Spectral "Diffusion" freeze (β control) | M | High (sound design) | O-Freeze v3 candidate |
| 8 | Audio-rate / pulsar grain mode | S–M after #3 | Medium–high, distinctive | O-simpleGrain v2 candidate |
| 9 | Steerable descriptor weights + softmax candidate picking | M | High for TextureForge | O-TextureForge |
| 10 | Buffer library separate from presets (GRN) | S | UX | via the preset-manager module |
| 11 | Signal-adaptive grain length (Nielsen extrema) | M | Medium | stretch and freeze modes |
| 12 | Latent granular, ISMIR-style without a decoder | L | Novelty | spike only |

---

## 6. Open questions / to verify
- Check the μ₁/μ₂ values numerically, and measure every CPU estimate in this file.
- Defaults of RAVE v2 (compression ratio, model sizes); Nasca's exact PaulStretch window and hop; chapter numbers in *Microsound*.
- How Portal, Quanta and Granulator III normalise loudness (unknown; only Clouds is verified).
- Audit items that were inferred, not measured: Polystutter's pitched-down click; the TextureForge retired-slot race.

## Sources
The URLs are inline above. Core references: Bencina, *Implementing Real-Time Granular Synthesis* · Roads, *Microsound* (2001) and *Sound Composition with Pulsars*, JAES 49(3) · Laroche & Dolson, IEEE TSAP 1999 · J.O. Smith, *Digital Audio Resampling* (CCRMA) · Schwarz et al., CataRT, DAFx 2006 · FluCoMa KDTree docs · Salmon et al., Random123 (SC'11) · Mutable Instruments Clouds source.
