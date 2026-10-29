# Research: Spectral Diffusion Freeze

**Plugin:** O-Freeze
**Milestone:** spectral-diffusion-freeze
**Created:** 2026-10-28
**Phase:** Research

---

## Context Summary

From CONTEXT.md:
- **Core requirement:** A second freeze engine (`ENGINE` = Granular / Spectral). Spectral is an STFT freeze (8192 / 4×, Hann×Hann) that averages magnitudes over `AVG_FRAMES` frames, drifts each bin with a random walk (Drift), advances phase by ω̂H + β·U(−π,π) with β = `DIFFUSION`, and uses phase locking near β = 0. Detune spreads pitch per bin; the LFO modulates Diffusion.
- **Scope:** three appended params, an RT-safe STFT engine, gates, i18n. No user FFT size, no PDC, no engine blending, no stretch.
- **Success criteria:** Granular stays bit-exact vs v2.8.0. Spectral renders are block-size-independent and reproducible from the seed. β sweep is monotone. N = 8 is less metallic than N = 1. 0 allocations. No clicks. i18n gates, pluginval 10 and auval pass.

---

## Implementation Approach

### Recommended Approach

**Strategy:** Self-contained `SpectralFreeze` class. Analysis runs on the write-head clock, resynthesis on the synth clock, and every random draw is keyed on a hop counter.

**Description.**
Add `Source/SpectralFreeze.h/.cpp`, a class that owns all the STFT state. It preallocates everything in `prepare()` and is driven per sample from the existing loop in `processBlock`. It does no work on samples where it has nothing scheduled. It has three parts:

1. **Analysis (rolling, only while `ENGINE` = Spectral and the write head is moving).** Every H = 2048 samples *written* to `freezeBuffer`, take the last W = 8192 samples, apply the Hann window, run a forward real FFT per channel, and push |X|² into a 16-frame power ring (16 × 4097 × 2 ch floats ≈ 0.5 MB). Keep the phase of the two most recent frames. The hop counter counts write-head advances, so it pauses with the write head during a freeze and release tail, and it doesn't depend on the host block size. **Fallback:** if the ring holds fewer than N valid frames (engine switched just before engaging, or a Granular→Spectral switch mid-freeze), the same `analyseFrame(endPos)` function runs N times on demand from `freezeBuffer`, which still holds the pre-freeze audio because the write head is held. One function, two callers.

2. **Capture (on engage, or on a mid-freeze switch to Spectral).**
   - `|X̄_k| = sqrt(mean_N |X_k|²)`, the RMS average of the last N frames. It preserves energy, so frozen noise keeps the source level, and it divides the variance of a noise bin by about N.
   - Instantaneous frequency comes from the two latest frames: `ω̂_k = ω_k + princarg(φ_k[m] − φ_k[m−1] − ω_k·H) / H`. At H = W/4 this is unambiguous for deviations within ±2 bins, which covers the Hann main lobe.
   - The start phase is the latest frame's phase.
   - Peak detection for phase locking: a local maximum of |X̄| over ±2 bins and above −60 dB re the frame max. Each bin takes the nearest peak (boundary at the magnitude minimum between two peaks). Store `peakOf[k]` and `Δ_k = φ_k − φ_peak(k)`.
   - Per-bin draws, once per engage from `engageCounter`: detune factor r_k ∈ [−1, 1), and random-walk rate ρ_k log-uniform in 0.2–3 Hz.
   - **Prime the OLA.** Synthesise 4 frames back to back at offsets −3H … 0, so the output is at steady-state level on the very first sample. An empty OLA would ramp in over 3 hops (128 ms at 48 k), which is longer than the 50 ms freeze fade.

3. **Resynthesis (one frame every H samples on the synth clock).** For each bin k:
   - `β = clamp(DIFFUSION + lfo·depth, 0, 1)`, sampled at the hop sample.
   - Free phase: `φᶠ_k += ω̂_k·H·2^(r_k·cents/1200) + β·π·(2u−1)`, with u = `CounterRng(seed, hop·4097 + k, kSlotSpecPhase)`.
   - **Soft identity lock:** `φ_k = φᶠ_k + λ(β)·princarg(φ_peak(k) + Δ_k − φᶠ_k)`, then `φᶠ_k := φ_k`. Use `λ(β) = max(0, 1 − β/0.3)²`. λ is 1 at β = 0 (pure Laroche–Dolson identity locking) and falls smoothly to 0 by β = 0.3, so there is no hard switch and it is continuous under LFO modulation. Peak bins advance by their own ω̂_p with the same β term.
   - Magnitude: `|X̄_k| · 10^(σ(drift)·d_k/20)`. d_k is a unit-variance OU walk updated per hop: `d ← a_k·d + sqrt(1−a_k²)·√3·(2u−1)`, with `a_k = exp(−2π ρ_k H / fs)` and u from slot `kSlotSpecDrift`. Map σ = Drift% × 1.5 dB so that ±2σ ≈ ±3 dB at 100 %. The walk state is normalised, so a live Drift move rescales it with no jump; the 4× OLA crossfades the per-hop step.
   - DC and Nyquist: magnitude only, phase forced to 0 or π (kept real).
   - Inverse real FFT, × Hann synthesis window × (1/1.5) × makeup(β), overlap-add into a 2W output ring. The per-sample output reads one sample from the ring.
   - **Stereo:** compute the phase noise, drift walk and detune factors **once per bin and share them across L/R**. Inter-channel phase and level relations are preserved, so the image doesn't wander. Each channel keeps its own captured magnitude and phase.

**Engine wiring in `processBlock`.**
- The per-sample `frozenSample` becomes `gGran·granular + gSpec·spectral`. `gSpec` is a 50 ms linear `LinearSmoothedValue` toward `ENGINE`, and the pair is always summed as `(1−e)·gran + e·spec`.
- At e = 0 (Engine = Granular, steady) the spectral path is skipped entirely and the expression must reduce to exactly the v2.8.0 `granularSum * densityGain`. Branch on `e == 0`; don't multiply by 1.0f. That keeps the bit-exact guarantee independent of FP contraction.
- When e reaches 0 or 1 after a switch, the idle engine stops (grains finish naturally / the spectral synth stops scheduling hops).
- Switching to Granular mid-freeze reuses the existing staggered steady-state start. Factor it out of `setFrozen` into `startGranularVoices(...)`, unchanged in its draws.
- `setFrozen` release fade: Granular keeps `remaining + 50 ms`. Spectral uses 100 ms (from CONTEXT). With both live mid-switch, take the max.
- Re-engage during a spectral release tail keeps the synthesis running with no recapture, mirroring the granular `tailLive` path.

**Pros:**
- Granular code is untouched apart from one factored-out function and a branch. Spectral state lives in its own class with its own RNG slots, so the v2.8.0 null is structurally protected.
- Every schedule (analysis hop, synth hop, RNG counters) runs on a sample counter from `prepareToPlay`, so block-size independence holds by construction, as in v2.8.0.
- Engage cost is low (an average over N frames plus 4 primed IFFTs) because the analysis already ran. There is no N-FFT burst on the engage sample.

**Cons:**
- Two analysis callers (rolling and on-demand fallback). The fallback still bursts N stereo FFTs, but only on the rare mid-freeze switch or an engage within N·H of switching engines.
- Each synth hop does 4097 × (sincos + exp + 2 hashes) + 2 IFFTs on one sample. Estimate ≈ 150–300 µs per hop on Apple silicon: under 1 % average CPU, but a burst inside one host block (see Risk 3).

### Alternative Approaches Considered

**Alternative 1: On-demand capture only (N FFTs per channel on the engage sample)**
- Description: no rolling analysis. Engage analyses N frames ending exactly at the freeze point.
- Why not chosen: up to 32 × 8192 FFTs (~1 ms) on one sample. At a 32–64-sample host buffer that overruns the block once per engage. It is kept as the fallback path only.

**Alternative 2: Amortised resynthesis (bins of frame m+1 processed in 16 chunks across hop m, IFFT at the end)**
- Description: spreads the per-hop burst evenly; stays deterministic because chunks are tied to the sample index.
- Why not chosen (yet): more state and harder to read. Recommend it only if the worst-case-block-time gate (Risk 3) fails. The plan should keep the per-hop update in one `synthesiseFrame()` so this stays a contained change.

**Alternative 3: Separate L/R random draws**
- Description: independent phase noise, drift and detune per channel, for maximum decorrelation.
- Why not chosen: ±3 dB independent per-bin drift and independent β phases make the stereo image wander and collapse mono compatibility. The granular engine shares jitter and detune across channels too.

---

## Affected Components

### Files to Modify

| File | Changes | Complexity |
|------|---------|------------|
| PluginProcessor.h | `SpectralFreeze` member; engine smoother; 3 new RNG slots appended after `kSlotHold` (`kSlotSpecPhase`, `kSlotSpecDrift`, `kSlotSpecDetune`/rate); `startGranularVoices()` decl | Low |
| PluginProcessor.cpp | Append ENGINE / DIFFUSION / AVG_FRAMES to layout (order preserved); prepare/release; per-sample engine mix; analysis tick on write-head advance; setFrozen release time per engine; factor `startGranularVoices` | Medium |
| SpectralFreeze.h/.cpp (new) | Analysis, capture, IF, peak lock, OU drift, resynthesis, OLA | High |
| PluginEditor.h/.cpp | 2 slider relays + 1 combo relay + attachments (relays declared before attachments, appended in order) | Low |
| Source/ui/public/index.html | Engine toggle, Diffusion + Avg knobs, engine-dependent swap/dim, LFO group label | Medium |
| Source/ui/public/js/i18n.js | EN / FR / zh-Hans labels + tips | Low |
| CMakeLists.txt | Add SpectralFreeze.cpp; version 2.9.0 | Low |
| tests/blocksize-null/gate.py | Spectral configs (manual + threshold, β 0/0.6/1, drift, detune, LFO) | Low |
| tests/spectral/ (new) | β sweep, metallic metric, hop-AM, clicks, Granular-vs-v2.8.0 null, alloc / worst-block-time | Medium |

### New Files

| File | Purpose |
|------|---------|
| Source/SpectralFreeze.h / .cpp | The STFT freeze engine (no APVTS access; plain setters, RT-safe) |
| tests/spectral/gate.py | Spectral behaviour gates (metrics below) |
| tests/granular-null/ (or a mode in blocksize-null) | v2.8.0 vs v2.9.0 Engine = Granular null, using a seeded out-of-tree pair |

### Dependencies

**JUCE Modules Used:**
- `juce_dsp`: `juce::dsp::FFT` (order 13). On macOS it is backed by vDSP (`AppleFFT`, juce_FFT.cpp:454). `performRealOnlyForwardTransform` / `performRealOnlyInverseTransform` need a **2·N float buffer**; the inverse reads only the first N/2+1 complex values and returns the reals in the first half (juce_FFT.h:97–104). The inverse is scaled by 1/N in JUCE; confirm once with an identity round-trip test before relying on the 1/1.5 normalisation.
- Construct the FFT object in the constructor (it allocates). The order is fixed, so no reconstruction is ever needed.

**External Dependencies:** none.

---

## Pattern Analysis

### Existing Patterns in Codebase

**Counter-RNG streams.** `PluginProcessor.h` `RngSlot` + `rand01(counter, slot)`; `CounterRng.h`.
Spectral draws use counter = `hop·4097 + bin` (phase, drift) or `engage·4097 + bin` (detune, rate), each on its own new slot. Never touch `grainCounter` / `engageCounter` increments on the Granular path. Spectral keeps its own `specEngageCounter` and `specHopCounter`, reset in prepare and on seed change (alongside the existing reset, PluginProcessor.cpp:471–477).

**Fixed control tick on the sample clock.** `controlTick`, PluginProcessor.cpp:412–457.
The LFO value used for β is the `lfoCurrentValue` this already produces. Sample it at the synth-hop sample. The LFO only advances while `bufferFrozen`, which is unchanged.

**Write-head hold during tail (WR-03).** PluginProcessor.cpp:725, 819.
The analysis hop counter must advance only where `writePosition` advances, so analysis and the ring stay in lockstep.

**Staggered steady-state granular start.** PluginProcessor.cpp:358–383.
Extract it as `startGranularVoices(reverse, drift, detune)` so a mid-freeze switch to Granular gets the same no-ramp start. The extraction must be draw-for-draw identical (memory: *module extraction: verify helper refs*).

**STFT OLA reference.** O-SpectralShaper `STFTProcessor` (512 / 50 %, per-sample FIFO).
Same FIFO-per-sample structure, but its window, hop and normalisation don't carry over. There is no existing phase-vocoder freeze or phase-lock code in the repo.

### JUCE Documentation References

- `juce::dsp::FFT` real-only API and buffer contract: checked in `~/JUCE/modules/juce_dsp/frequency/juce_FFT.h:85–115` (JUCE 8.0.15 pin).

### Similar Implementations

- **Laroche & Dolson 1999**, *Improved phase vocoder time-scale modification of audio*: identity phase locking (peak picking, region of influence, φ_k = φ_p + (∠X_k − ∠X_p)). Here it is applied with a fixed analysis frame, so Δ_k is computed once per capture.
- **PaulStretch:** random phases with a large window. This is the β = 1 limit.

---

## Key Technical Findings

### F1. Output level depends on inter-frame coherence: about −6 dB at β = 1 for tonal content

With fixed magnitudes, coherent frames (a β = 0 tone) stay localised under the window, and the Hann×Hann OLA sum of 1.5 restores amplitude A exactly.

Incoherent frames (β = 1) spread each frame's energy over all of W. The output power ratio, incoherent to coherent, is `mean(w_a²)·O·mean(w_s²) / (O·mean(w_a·w_s))² = 1/O = 0.25`, i.e. **−6.0 dB** at 4× overlap, for any window.

Noisy sources are already incoherent at β = 0 (their ω̂ estimates are random), so their level barely moves with β. A β-only makeup therefore helps tones and over-boosts noise.

**Recommendation:** split the difference. Use `makeup(β) = 1 / sqrt(1 − 0.5·(1 − sinc²(β)))`: +3 dB at β = 1 when inter-hop correlation for a tone falls as sinc(β). Acceptance: steady-state RMS of the chord and of pink noise each stays within ±3 dB of its β = 0 level across β ∈ {0, .25, .5, .75, 1}. Execute measures this and refits the constant if needed.

### F2. Detune via phase increment is a pitch shift only for small deviations

Scaling ω̂_k·H doesn't move a component's bin. Beyond about ±1–2 bins of shift (5.86 Hz/bin at 48 k) the frames decohere and the result is roughness/AM rather than a clean pitch change. 50 ct at 1 kHz ≈ 5 bins.

With locking (β ≈ 0), a whole partial's lobe follows its peak's factor, so low partials detune cleanly and high partials shimmer. That is acceptable for a "per-bin pitch spread", but the tooltip should say *spread / shimmer*, not *pitch*.

### F3. Frequency resolution at 192 k

8192 at 192 k: 43 ms window, 23.4 Hz/bin, hop 10.7 ms (94 hops/s). The PV freeze still works because the window only needs to exceed about 2 periods of the lowest partial (fine above about 50 Hz).

The audible differences: low partials (< 60 Hz) smear, and the averaging span of N frames covers ¼ of the time it does at 48 k (N = 8: 160 ms vs 640 ms).

**Acceptable for this milestone** (CONTEXT scope). The drift random walk's rate is defined in Hz (`a_k` uses fs), so its timing doesn't change with sample rate.

### F4. The capture point lags the freeze sample by 0–H samples

Rolling analysis is frame-aligned. The newest frame ends up to H−1 written samples before the engage (≤ 43 ms at 48 k) and is centred W/2 earlier still. With a 170 ms window this is inaudible as "late", and it is deterministic.

The fallback path is aligned exactly to `writePosition`. Rolling and fallback captures of the same moment differ slightly, which is fine: each is deterministic for a given input and seed.

### F5. Bit-exact Granular

The structural requirements are:
- (a) spectral work is skipped when e == 0, with a branch rather than a ×1.0;
- (b) no shared RNG counters;
- (c) `freezeGain` behaviour at Engine = Granular is byte-identical in `setFrozen`;
- (d) rolling analysis is gated on Engine = Spectral, so Granular doesn't even pay its CPU. It reads `freezeBuffer` only, so it couldn't affect output anyway.

The null needs a v2.8.0 binary. Build it out of tree from `git archive v2.8.0` (memory: *null test w/ seeded out-of-tree pair*). VST3 params match by name, and the new params default to Granular / 0 % / 8 when a v2.8.0 template state is loaded.

### F6. Clicks

- **Engage/release:** covered by the existing `freezeGain` fade, plus OLA priming (no level ramp).
- **Engine switch while frozen:** 50 ms equal-gain linear crossfade. The engines are uncorrelated, so equal-power is arguably better, but linear matches the existing freeze fade; the gate decides.
- **DIFFUSION / DRIFT / DETUNE live moves:** each changes per-hop quantities only, and 4× OLA crossfades successive frames over 2048 samples. No per-sample smoothing is needed.
- **AVG_FRAMES:** takes effect at the next capture only (documented in the tooltip).

---

## Objective Metrics (for the gates)

| Criterion | Metric | Pass |
|---|---|---|
| Metallic (N = 8 vs 1) | Pink-noise source, β = 1, Drift 0. Welch PSD of 4 s frozen output (output long-term spectrum ≈ frozen magnitudes); std dev across bins 200 Hz–10 kHz of (PSD_dB − ⅓-oct-smoothed PSD_dB) | N = 8 ≤ 0.6 × N = 1. Theory: single-frame Rayleigh ≈ 5.6 dB, 8-frame RMS ≈ 2.2 dB with frame overlap |
| β monotone / coherence | Chord source (gate's 220/277/330/466 Hz). (a) peak-to-floor of a 65 k-point Welch around each partial; (b) circular variance of hop-to-hop phase advance in the partial's bin of an output STFT (same H) | (a) strictly decreasing and (b) strictly increasing over β ∈ {0, .25, .5, .75, 1}; β = 0 circular variance < 0.05 |
| No hop-rate AM | β = 0 chord: envelope spectrum at fs/H and 2·fs/H | AM depth < 0.5 dB |
| Level vs β | Steady RMS, chord and pink noise | within ±3 dB of β = 0 (F1) |
| Clicks | Engage / release / engine switch: max \|2nd difference\| in a 10 ms window vs the same motion with Mix 0 (same-motion control, memory: *step click gate*) | ≤ 1.5× control |
| Determinism | Block-size null 32 / 441 / 7 vs 512 | ≤ −120 dBFS; seed-distinct control must NOT null |
| Granular bit-exact | v2.8.0 vs v2.9.0, Engine = Granular, all three existing configs | identical (≤ −140 dBFS / exact) |
| Allocations | Existing alloc-hook harness around processBlock with Spectral engaged, including the fallback path and an engine switch | 0 |
| Worst block time | Max processBlock time at block 32 / 48 k with Spectral frozen over 20 s | < 50 % of the block period (333 µs); if it fails → Alternative 2 |

---

## Complexity Assessment

### Overall Complexity: High

**Justification:**
- Files affected: 8 modified + 3 new
- DSP changes: Significant (new engine, crossfaded with the existing one)
- UI changes: Minor–Moderate (3 controls in a fixed 550×530 layout)
- Parameter changes: Add 3 (appended)
- Breaking changes: No

### Risk Areas

1. **Granular regression.**
   - Risk: the `startGranularVoices` extraction or the engine mix changes a single float op, and the v2.8.0 null fails.
   - Mitigation: branch on e == 0; draw-for-draw identical extraction; run the v2.8.0 null as the first gate after wiring, before any spectral DSP lands.

2. **Level jump between engines / across β.**
   - Risk: Spectral comes out several dB away from Granular, so the engine switch is a level step (F1).
   - Mitigation: measure the granular density-law output RMS vs spectral β = 0 on the same source; set a fixed spectral trim so the means match within 1 dB.

3. **Per-hop CPU burst.**
   - Risk: about 0.3 ms on one sample every 2048 (plus 4 IFFTs per channel when priming on engage). It is fine at 128+ buffers; marginal at 32.
   - Mitigation: worst-block-time gate; Alternative 2 if it fails. Precompute the Hann tables; use `std::sin/cos` once per bin per hop (or a sincos LUT if measured hot).

4. **Phase-lock region bookkeeping on dense spectra.**
   - Risk: on noise, peaks are everywhere, regions are 1–3 bins, and locking behaves like no locking (harmless). On a chord, a wrong region boundary makes a partial's side bins jump.
   - Mitigation: regions bounded by the magnitude minimum between adjacent peaks; peak threshold −60 dB re max; β-lock law tested on the chord metric (b).

5. **Preset/automation compatibility.**
   - Risk: param order or IDs disturbed.
   - Mitigation: append ENGINE, DIFFUSION, AVG_FRAMES after DETUNE with version hint 1 (new IDs, so no migration). An old session loads with Engine = Granular. No `rngSeed` change.

---

## Domain Detection

| Domain | Score | Keywords Found |
|--------|-------|----------------|
| DSP | 10 | processblock, filter, gain, dsp, algorithm, buffer, sample, frequency, latency, fft |
| GUI | 6 | webview, ui, relay, attachment, html, layout |
| Polish | 3 | preset, cpu, pluginval |

**Detected Domain:** mixed (DSP-dominant, DSP > 0 and GUI > 0)
**Recommended Execute Agent:** general-purpose. Suggest splitting execution: the DSP tasks first (engine + gates), then the UI/i18n tasks, with the v2.8.0 null gate between them.

---

## Version Impact

### Recommended Version Bump: MINOR (2.8.0 → 2.9.0)

- [ ] Bug fix only (PATCH)
- [x] New feature, backward compatible (MINOR)
- [ ] Breaking changes (MAJOR)

**Breaking Change Analysis:**
- Parameter IDs: Unchanged (3 appended)
- Parameter ranges: Unchanged
- Preset format: Unchanged (new params default; old sessions → Granular)
- Public API: n/a

---

## UI Findings

- The fixed 550×530 window's knob row already holds 6 knobs (Threshold, Drift, Size, Grains, Detune, Mix), so 8 won't fit at the current size. **Recommended: swap slots by engine.** In Spectral, the Size slot becomes **Diffusion** and the Grains slot becomes **Avg** (frames). Reverse dims. This is the same footprint and needs no relayout, and an inactive control is never just a dimmed knob taking space. It deviates from CONTEXT's "dim", so confirm at plan.
- **Engine toggle:** a second segmented toggle styled like `#mode-toggle`. Captions = the AudioParameterChoice strings verbatim (Granular / Spectral), so they are exempt under D-01 arm 1 like Manual / Threshold. They stay untranslated in FR and zh, matching the Mode precedent.
- **The LFO group label is "Drift LFO"** (`label.driftLfo`), which is wrong in Spectral, where the LFO modulates Diffusion. Switch it with the engine ("Diffusion LFO"), or rename it to the neutral "LFO". Recommend the engine-dependent label (new key, EN/FR/zh), and check the French width budget (memory: *pin content-sized boxes at widest language*).
- **Tooltips per engine:** Drift (grain position vs per-bin level walk) and Detune (per-grain vs per-bin spread/shimmer, F2) need a Spectral variant.

---

## Open Questions

1. **UI:** swap Size→Diffusion / Grains→Avg in Spectral (recommended) or keep all 8 visible with dimming (needs a wider window or smaller knobs)?
2. **β makeup:** accept the half-compensation (+3 dB at β = 1, F1), refit in execute against the ±3 dB acceptance? (recommended: yes)
3. **AVG_FRAMES type:** int 1–16 knob (recommended), or a 1/2/4/8/16 choice? The int knob is consistent with Grains.

---

## Next Phase

This research feeds the **Plan** phase, which will:
- Order tasks as follows:
  - params + engine wiring + `startGranularVoices` extraction
  - v2.8.0 null
  - SpectralFreeze analysis / capture / resynthesis
  - spectral gates
  - UI + i18n
  - pluginval / auval
- Define per-task verification from the metrics table
- Pin version 2.9.0, execute agent general-purpose (DSP then GUI)

---

*Generated by improve-milestone research phase*
