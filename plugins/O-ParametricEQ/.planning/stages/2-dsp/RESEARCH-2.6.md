# O-ParametricEQ Stage 2 (DSP), Phase 2.6: Research (analyzer taps, display model, listen)

**Researched:** 2026-11-15
**Scope:** ROADMAP Phase 2.6. It covers `eq::SpectrumTap` rings, the message-thread `SpectrumAnalyzer`, `eq::DisplayModel` with its section packets, and `eq::Listen`. The gates are G9 (a), the analyzer reading, G6 with the analyzer on and the rings saturated, and G3 plus bit identity for listen. Carried in from VERIFICATION-2.5:
- Issue 2: five gate gaps held only by the probe (PX17, PX11, PX4, AG3, OS18).
- VERIFICATION-2.4 Issue 1: the stop-band burst, still waiting on the listen.

Out of scope: the WebView editor, the JS evaluator and G9 (b) (Stage 3).
**Method:** a scratch bench over the real `Source/dsp` headers, `stages/2-dsp/research-2.6/tap_bench.cpp` (output: `research-2.6/bench-output.txt`). It runs the double engine (24 `eq::SlotBand`, started settled, processed in index order). It simulates the plugin's float output by rounding each output sample. It measures impulse responses, the analyzer pipeline on sines, and listen crossfades scored with G3's step measures.
- **[BENCH]:** a number from that bench on this machine (Apple M4 Max, clang -O3, scalar, double).
- **[MODEL]:** a number computed from pole radii.
- **[ARGUED]:** a statement from reading `Source/PluginProcessor.cpp` at `90551e85`, JUCE 8.0.15 headers and ARCHITECTURE.

Nothing in `Source/`, the harnesses, the model or the goldens was changed.
**Contracts read:** ARCHITECTURE § 8, 9 and 10, "Processing Chain", "True-response display", "Analyzer", "Thread Boundaries", G3, G6 and G9; REQUIREMENTS UI-02, UI-03 and FUNC-08; Stage 2 CONTEXT D11; VERIFICATION-2.4 Issue 1; VERIFICATION-2.5.

## Summary

1. **The closed form is exact; measuring it on the unit circle is not.** The display's 2x2 composition of the ResponseEval forms equals the double engine within **1.1e-6 dB** [BENCH].
   - **The problem:** G9 (a) as written ("measured from a rendered impulse") cannot be met on the unit circle. Near slow resonances the truncated impulse response is wrong by up to **4 dB**, and 2 – 6 % of random full-range draws do not decay 140 dB within the harness's 20 s cap [MODEL]. Even draws that reach the cap's tail rule miss by up to 0.58 dB near a narrow low resonance [BENCH].
   - **What works:** measure on a circle just outside the unit circle (`z = e^{jω}/a`, a 2^17-sample impulse response weighted by aⁿ). The closed form evaluated at the same z agrees to **≤ 1.2e-3 dB** through the plugin's float output, provided points are scored within 80 dB of each response's own maximum [BENCH].
   - This changes G9 (a)'s measurement and its "above −80 dB" floor (call C1).
2. **The analyzer as specified misreads a −12 dBFS 1 kHz sine at 96 and 192 kHz: −12.63 dB**, outside ±0.5. The cause is Hann scalloping: 1 kHz falls a third of a bin off a bin centre at 11.7 Hz spacing. Zero-padding the windowed frame to **4x the FFT size** fixes it:
   - 1 kHz reads −12.08 / −12.04 / −12.04 at 44.1 / 96 / 192 kHz.
   - A 20 Hz – 20 kHz sweep at five rates stays within **−12.11 … −11.99** [BENCH].
   - Hann resolution is kept (the main lobe is unchanged); the cost is a 4x larger FFT on the message thread.
3. **A listen fade that starts its filter from rest bursts.** A listen band-pass started from rest on a tone in its stop band rings far louder than either settled output.
   - **Readings:** listen-on with a 20 ms crossfade reads up to **29 672x** the louder held render on G3 (a). It reads 250x for a 1 kHz Q 10 band-pass on a 50 Hz tone [BENCH]. This is the same physics as VERIFICATION-2.4 Issue 1, but worse, because a band-pass removes most of the tone.
   - **A fixed pre-roll does not cure it:** 200 ms still leaves 10 of 3040 band changes over 2.0x [BENCH].
   - **What works: fade the listen filter's input in over 30 ms, then crossfade over 20 ms.** Every listen-on and every one of the 3040 band-change pairs then passes: (a) ≤ **1.35x**, 1 kHz κ ≤ **2.41x** [BENCH].
   - **Cost to the user:** listen takes effect over 50 ms instead of 20 ms (call C2).
4. **Where listen sits relative to auto gain is a real choice** (call C3). ARCHITECTURE puts the listen crossfade before both gains, so a solo plays at the auto-gain level of an EQ it no longer contains. The recommendation is to move it to after auto gain and before output gain.
5. **Display packets survive JUCE's JSON.** `serialiseDouble` keeps about 16 significant digits; rounding every section parameter that way moves the drawn responses by ≤ **1.5e-8 dB** [BENCH]. No binary channel is needed.
6. **Display design cost is negligible:** a full push of 24 designs is 4 – 41 µs on the message thread [BENCH]. The display model polls the parameter atomics on the editor timer and sends the bands whose snapshot changed. A preset load therefore needs no special path.
7. **The taps are read-only on the buffer**, so G4 and every existing `memcmp` row are untouched by construction.
   - **Ring storage:** fixed in the constructor, not in `prepareToPlay`, so a host that prepares while the editor's timer reads cannot free memory under the reader.
   - **Overflow:** keeps the newest samples and counts the rest [ARGUED].
8. **Five rows are owed from VERIFICATION-2.5** (PX17, PX11, PX4, AG3, OS18), and the 2.4 stop-band burst now has a tested remedy. The input fade-in that cures listen would also cure it, at the cost of 40 ms structural changes. It stays a listening item, outside this phase.

Three calls are Taylor's (C1 – C3). The rest are the plan's.

---

## Q1. G9 (a): can the displayed response be checked against a measured one? (UI-02)

### The closed form

ARCHITECTURE "True-response display" composes bands as 2x2 matrices in L / R:

| Placement | Matrix |
|---|---|
| Stereo | F·I |
| Left | diag (F, 1) |
| Right | diag (1, F) |
| Mid | I + (F − 1)·½[[1,1],[1,1]] |
| Side | I + (F − 1)·½[[1,−1],[−1,1]] |

The product runs in band index order (`M = M_n ⋯ M_1`). The four drawn responses are:

- **LL** = M₀₀ and **RR** = M₁₁;
- **MM** = ½ ΣMᵢⱼ;
- **SS** = ½ (M₀₀ − M₀₁ − M₁₀ + M₁₁).

The SVF and the one-pole are the bilinear images of `(mL + mB σ + mH σ²) / (1 + k σ + σ²)` with σ = s / g, and of `(cL + cH σ′) / (1 + σ′)` with σ′ = s / G. On the unit circle s = j tan (ω/2) gives ResponseEval's forms. Elsewhere, s = (z − 1) / (z + 1). [ARGUED; bench `composeAtS`]

### Measuring on the unit circle fails near slow poles

`tap_bench decay` [MODEL] gives the slowest pole of random full-range configurations (the G5 draw) and the time it needs to fall 140 dB:

| Bands | p50 | p90 | p99 | max | share over the 20 s cap of `renderImpulse` |
|---|---|---|---|---|---|
| 1 | 0.03 – 0.05 s | 1.2 – 1.9 s | 9 – 14 s | 32 – 61 s | 0.2 – 0.7 % |
| 1 – 8 | 0.7 – 0.8 s | 6 – 7 s | 26 – 40 s | 58 – 83 s | 1.6 – 3.0 % |
| 1 – 24 | 2.5 – 3.0 s | 13 – 14 s | 40 – 53 s | 59 – 84 s | 4.3 – 6.3 % |

The worst is a +30 dB bell at 10 Hz, Q 40. Its poles have Q 225 and decay at 1.2 dB/s.

Reaching the tail is not enough either. The truncation error at ω is about the tail's amplitude divided by `|1 − r e^{j(θ−ω)}|`. Near a pole of radius r close to 1 that divisor is about 1 − r, so a tail at −140 dB re peak becomes a −20 dB error next to the resonance. `tap_bench g9` measures the closed form against the DFT of the float-rounded impulse response on 96 log points (10 Hz – min (30 kHz, 0.49 fs)), LL / RR / MM / SS above −80 dB [BENCH]:

| Draws | Worst over draws that met the 140 dB tail rule | Over capped draws |
|---|---|---|
| 50 per rate, 1 – 8 bands | **0.58 dB** (44.1), 0.21 (192), 0.11 (88.2), 0.03 (48), 0.01 (96) | up to 4.3 dB |
| 20 per rate, 1 – 24 bands | **0.29 dB** (48), 0.16 (96), 0.14 (192), 0.09 (44.1), 0.03 (88.2) | ≤ 0.04 dB |

The 0.05 dB threshold is unreachable this way. The errors come from the measurement, not from the display: `g9diag` gives 4.1 – 5.0 dB in double as well as in float for a fixed 8 s response [BENCH].

### Measuring on a slightly larger circle works

`tap_bench damped / g9diag` [BENCH]:

- **Measurement:** the impulse response is rendered for N = 2^17 samples at every rate and weighted by aⁿ with a^N = 1e-9. Its DFT is H at z = e^{jω}/a.
- **Comparison:** the closed form (`composeAtS`) is evaluated at the same z.
- **Truncation:** after the weight the tail is 180 dB down whatever the poles. The poles are inside the unit circle, so the circle of radius 1/a ≈ 1 + 1.6e-4 encloses all of them.

| Path | Points within 40 dB of the response max | 40 – 60 dB below | 60 – 80 dB below |
|---|---|---|---|
| Double impulse response | 2.1e-8 dB | 7.1e-7 | 1.1e-6 |
| Float-rounded impulse response (the plugin's output) | 1.7e-5 dB | 1.0e-4 | **1.2e-3** |
| Float, floor at −80 dB **absolute** (as G9 is written) | 2.3e-4 | 3.4e-4 | **2.2e-2** (and 5.3e-2 on 24-band draws, `damped`) |

The float error is quantisation noise. It scales with the response's energy (Parseval), not with 1.0, so a high-gain chain raises it in absolute terms. Scored relative to each drawn response's own maximum, it is ≤ 1.2e-3 dB. Scored against −80 dB absolute, a +60 dB stack fails 0.05 dB at its deep points. The floor must therefore be relative.

**What the damped measurement checks.** It checks that the realised filter's transfer function equals the closed-form rational function. A wrong `g`, `k`, mix, one-pole, placement matrix or composition order changes that function everywhere off the poles, so it shows at these points too. It does not sample the unit circle's sharpest peaks themselves; the closed form does that, and Q1's exactness result covers it.

The bench's planted control (one band's `g` × (1 + 1e-4) in the closed form) is too weak: it reads 0 dB where the perturbed section has no SVF. The plan needs controls that must fail:
- k × 1.01;
- the one-pole dropped;
- two non-commuting bands (an L band and an M band) composed in the wrong order;
- a Mid matrix used for Side.

**A unit-circle row is still possible on draws that decay.** `tap_bench g9unit`: keep the draws whose slowest pole falls 200 dB within 6 s (25 of 29 – 35 drawn), and render 1.25x that time. Through float, scored within 80 dB of the maximum, the worst is ≤ 2.6e-3 dB at four rates and **3.8e-2 dB at 48 kHz** [BENCH]. That passes, but with little margin, so it is best kept as a confirming row.

### P1 (for the plan, under C1)

- **G9 (a) binding row (render harness):** 50 random full-range configurations at each of 44.1, 48, 88.2, 96 and 192 kHz, 1 – 24 bands, stereo, plus 10 mono.
  - **Source of the closed form:** the closed form comes from the `DisplayModel` packets of the processor under test (Q4), not from the harness's own design calls, so the display path itself is what is tested.
  - **Stimuli:** four impulses (L, R, mid (1, 1), side (1, −1)), N = 2^17, damped DFT, 192 log points 10 Hz – min (30 kHz, 0.49 fs).
  - **Threshold:** LL / RR / MM / SS ≤ **0.05 dB** where the closed form is within 80 dB of that response's maximum over the grid. Expected ≤ 1.2e-3 dB.
  - **Controls:** the four controls above must each exceed 0.05 dB somewhere.
- **G9 (a) unit-circle row (render harness):** the same, on draws whose slowest pole falls 200 dB within 6 s (pole radii from the packet), rendered for 1.25x that time. Threshold ≤ 0.05 dB (expected ≤ 0.04).
- **Engine row (designer harness, double):** the damped comparison with no float rounding ≤ **1e-4 dB** (expected 1.1e-6).
- **Packet identity row (render harness):** after the first block, every band's packet sections equal the sections the band realises, **bit for bit** (`memcmp` of the `SectionParams`). This needs a test accessor on the processor for a band's current design, like `getBandDynOffsetDb`. The row is stronger than any dB tolerance and costs nothing. It holds because both threads call the same `Designer::design` on the same `BandInput` at the same rate (Phase 2.3: a settled band is bit-identical to a static design).
- **`ResponseEval.h`** gains the complex-s evaluation (`sectionResponseAtS`), header-only and std-only; the unit-circle forms stay as they are. Alternatively it stays harness-local.

---

## Q2. Analyzer: taps, rings, FFT, calibration (UI-03 engine half)

### Calibration

`tap_bench analyzer` [BENCH]. A −12 dBFS sine (both channels, so the mono sum 0.5 (L + R) is the sine) goes through ARCHITECTURE's pipeline:

- the newest window of 4096 / 8192 / 16384 samples (fs ≤ 48 / ≤ 96 / ≤ 192 kHz), windowed;
- scaled by 2 / Σw, so a full-scale sine reads 0 dB;
- reduced to 256 log points over the mockup's axis, 10 Hz – 30 kHz (empty above fs / 2), each the maximum bin in its span;
- a span that holds no bin reads the dB interpolation of the two bins around its centre.

The reading is the largest display value, worst over 8 phases.

| Window | Zero-pad | 1 kHz at 44.1 / 96 / 192 kHz | Sweep 20 Hz – 20 kHz, 300 tones × 5 rates |
|---|---|---|---|
| Hann (as specified) | 1x | −12.08 / **−12.63 / −12.63** | **−13.42** … −12.00 |
| Hann | 2x | −12.08 / −12.16 / −12.16 | −12.38 … −12.00 |
| **Hann** | **4x** | **−12.08 / −12.04 / −12.04** | **−12.11 … −11.99** |
| Blackman-Harris 4 | 4x | −12.05 / −12.02 / −12.02 | −12.05 … −12.00 |
| Flat-top | 1x | −12.00 (all) | −12.25 … **−10.77** (its wide lobe meets the negative-frequency image at low frequencies) |

At 96 and 192 kHz the bin spacing is 11.72 Hz, and 1 kHz sits at bin 85.33, a third of a bin off-centre. Hann loses 0.63 dB there, which fails the roadmap's ±0.5. At 44.1 kHz it sits at bin 92.88 (0.12 bin off) and passes by luck. Zero-padding to 4x interpolates the spectrum so the maximum bin is never more than 1/8 bin from the peak. The maximum error is 0.11 dB anywhere, and the window's resolution (main lobe about 23 Hz at −6 dB) is unchanged.

**P2. Hann, frame as specified, zero-padded to 4x.** The FFT order is 14 / 15 / 16 for the three window sizes. `juce::dsp::FFT::performFrequencyOnlyForwardTransform` takes a buffer of 2 × size; the Apple engine (vDSP) and the fallback both handle order 16. Cost: two transforms per 33 ms tick (pre and post), message thread. Blackman-Harris would be more accurate still (≤ 0.05 dB) but widens the lobe by about 36 %; it is not needed.

Low-frequency display points are narrower than a bin even at 4x (the point at 10 Hz spans 0.32 Hz; bins are 2.9 Hz apart). The interpolation rule above therefore matters below about 90 Hz, and the plan should state it.

### The audio side (`eq::SpectrumTap`)

[ARGUED, from the processor at `90551e85` and JUCE 8.0.15 `juce_AbstractFifo.h`]

- **Where the taps are.**
  - **Pre:** before `processAudio` in `processBlock` and `processBlockBypassed`.
  - **Post:** after it, beside `updateOutputPeak`. That matches ARCHITECTURE's order: pre before anything, post after the bypass mix.
  - Both are read-only on the buffer, so every `memcmp` row (G4, block identity, the 122 render rows) is untouched by construction.
  - The taps run on every path that has a prepared rate (processed, untouched, fully bypassed) and are skipped when not prepared.
- **What is written:** the float mono sum `0.5f * (L + R)` (or the single channel), through a 256-sample stack piece, into a single-producer / single-consumer ring per tap (`juce::AbstractFifo` over 32 768 floats).
- **Gating:** the audio thread writes only when a consumer has registered (`std::atomic<bool>`, set by the analyzer's owner: the editor in Stage 3, the harness here) and `analyzerMode` has that tap's bit (1 Pre, 2 Post, 3 both: the mode value is the bit mask). Mode Off writes nothing.
- **Storage is fixed at construction.** ARCHITECTURE says "allocated in `prepareToPlay`". A host may call `prepareToPlay` on a thread other than the one running the editor's timer, and re-allocating a ring the reader is draining is a use-after-free. With 2 × 32 768 floats (256 KB) owned by the processor from construction, prepare only clears the rings (resets both indices) and bumps a `std::atomic<uint32_t>` generation. The reader discards what it drained if the generation moved during the drain. The rate the reader needs is a `std::atomic<double>` written in prepare.
- **Overflow:** when a block does not fit, write the **newest** samples that fit and add the rest to a `std::atomic<uint32_t>` drop counter. The ring never blocks and never waits. A host block longer than 32 768 samples (an offline bounce with the editor open) then still delivers its last 32 768, instead of being dropped every time. When the reader sees the drop counter move, it discards its history before the gap, so the window never straddles a discontinuity.
- **Capacity check:** at 192 kHz the ring holds 170 ms, five editor ticks, so a stalled message thread drops data after about 170 ms and recovers on the next drain.

### The message side (`SpectrumAnalyzer`)

A JUCE class (it needs `juce::dsp::FFT`), so the **render harness** runs it, not the std-only designer harness. Per call to `process (double dtSeconds)`:

1. Drain both rings into per-tap history buffers.
2. If at least one window has accumulated since the last reset, transform the newest window: Hann, zero-pad 4x, |X|·2/Σw, then dB.
3. Apply per-bin ballistics: instant attack, release 20 dB/s × dt.
4. Reduce to 256 points with the max-and-interpolate rule; emit two `float[256]`.

`dt` is passed in rather than read from a clock, so the harness is deterministic (Stage 3 passes the timer's measured interval). With no new data the release still runs, so the display falls to the floor when the host stops calling `processBlock`.

### P3 (rows)

| Row | What | Expected |
|---|---|---|
| Analyzer reading | −12 dBFS sine, both channels, 1 kHz, at 44.1, 96 and 192 kHz (and 48, 88.2): largest display value, pre and post (no band) | −12 ± 0.5 dB; bench −12.08 / −12.04 / −12.04 |
| Analyzer sweep (INFO or binding at ±0.25) | 50 log tones 20 Hz – 20 kHz | −12.11 … −11.99 [BENCH] |
| Analyzer control | the same with zero-pad 1x at 96 kHz | must read below −12.5 (bench −12.63) |
| Pre / post | +12 dB bell at 1 kHz: post − pre at 1 kHz | +12 ± 0.5 dB |
| Mode gating | mode Off, or no consumer: ring write index does not move; mode 1 / 2 move only their ring | exact |
| Overflow | no reader for 2 s with blocks of 32 / 4096 / 65 536: `processBlock` returns, drop counter > 0, ring holds the newest samples (compare to the input tail), then a drain and a transform give the reading row's value | bit-exact tail; reading within ±0.5 dB |
| Generation | `prepareToPlay` at a new rate between two drains: the reader discards the drained data; no sample of the old rate reaches a transform | exact |
| G4 / identity | the existing `memcmp` rows run with the analyzer on (mode 3, consumer registered) | unchanged |

---

## Q3. Listen (FUNC-08 engine half, D11)

### What it plays

D11 maps the listened band to a listen filter:

| Listened band | Listen filter |
|---|---|
| Bell, Notch, Band Pass | matched Band Pass at f, Q |
| Low Shelf, Low Cut | 12 dB/oct low-pass at f (the High Cut design, slope index 1, Q 1.0) |
| High Shelf, High Cut | 12 dB/oct high-pass (the Low Cut design) |
| Tilt | Band Pass, Q 0.7 |

Every one is an existing type through the existing `Designer`, so listen adds no design code. [ARGUED]

"Placement respected" needs a solo form of the router, because the band routers are additive (`L += F(m) − m`). The plan's reading of D11:

| Placement | Listen output |
|---|---|
| Stereo | L = F(L), R = F(R) |
| Left | L = F(L), R = 0 |
| Right | L = 0, R = F(R) |
| Mid | L = R = F(m) |
| Side | L = F(s), R = −F(s) |
| Mono layout | F(x) |

That is the band's region of the input and nothing else. Where the band does not act, the output is silent rather than dry. If Taylor wants the untouched channel to pass dry on Left / Right bands, it is a one-line change of the solo router; it is not raised as a call.

The listen filter reads the **dry input** (the converted lanes before any band runs), as D11 says. The processor already takes this copy per segment for the bypass mix; it is taken whenever listen is not idle as well.

### The crossfade, and why it cannot start from rest

`tap_bench listen` [BENCH]: 48 kHz, tone at −6 dBFS, 4 phases. Listen on at 0.5 s and off at 1.0 s, both off the control grid. The output is (1 − w)·EQ + w·listen with the `Fade.h` 20 ms raised cosine, and the listen filter starts from rest at the start of the fade (the SlotBand pattern). G3 step measures: (a) largest second difference over −50 … +70 ms around the event / the larger of the two held renders; (b) 1 kHz local κ / the larger held κ.

| Listened band | 50 Hz on | 50 Hz off | 1 kHz on / off | 1 kHz κ on / off |
|---|---|---|---|---|
| Bell 1 kHz +12 Q 2 | 1.29 | 1.12 | 1.00 / 1.00 | 1.27 / 1.25 |
| Bell 5 kHz +6 Q 40 | **5.49** | 1.12 | 1.00 / 1.00 | 2.21 / 2.06 |
| Band Pass 1 kHz Q 10 | **250** | 1.00 | 1.00 / 1.00 | 1.02 / 1.00 |
| High Shelf 8 kHz −12 | 1.22 | 1.12 | 1.00 / 1.00 | 2.36 / 2.22 |
| 7 other bands | ≤ 1.32 | ≤ 1.12 | 1.00 | ≤ 2.25 |

Switching back to the EQ path is always clean, because that path is warm. Switching to listen is not. A band-pass started from rest on a tone in its stop band rings at its own frequency with an amplitude set by the tone's level. Its settled output is the tone attenuated by 40 – 90 dB. Against that held render the ring is a burst: 250x for 1 kHz Q 10 on 50 Hz, and **29 672x** for 8 kHz Q 40 (`listen2`, P = 0). This is VERIFICATION-2.4 Issue 1's physics, made worse because a band-pass's job is to remove the tone.

**Two remedies tried.** The set is 20 listened bands and all 20 × 19 ordered band changes (each a crossfade between two listen filters, the incoming one started at the request), both tones, 4 phases, 48 kHz [BENCH]:

| Remedy | Listen on: worst (a), (b) | Band change: worst (a), (b), pairs over 2.0x |
|---|---|---|
| None (20 ms fade from rest) | 29 672x, 4.69x | 29 671x, 38.8x, 631 / 3040 |
| Pre-roll P: filter from rest at the request, weight 0 for P, then the 20 ms fade. P = 30 ms | 67.9x, 2.36x | 111x, 2.35x, 158 / 3040 |
| Pre-roll 200 ms | 2.10x, 2.36x | 2.11x, 2.35x, 10 / 3040 |
| **Input fade-in R, then the 20 ms fade after P = R.** R = P = 20 ms | 1.82x, 2.36x | 1.91x, 2.41x, 0 / 3040 |
| **R = P = 30 ms** | **1.21x, 2.36x** | **1.35x, 2.41x, 0 / 3040** |
| R = P = 50 / 100 ms | 1.14x, 2.36x | 1.34x, 2.41x, 0 / 3040 |
| R = 50, P = 30 (fade starts before the ramp ends) | 2.87x | 30.7x, 42 / 3040 |

**Why a pre-roll is not enough, and an input ramp is.** A pre-roll waits for the start-up ring to decay. A narrow low-mid band-pass decays slowly (300 Hz Q 40: τ = Q / πf ≈ 42 ms), so no short pre-roll covers it. The ring is the filter's response to the step the input makes when a filter at rest suddenly sees a tone. Ramping the input in over R removes the step: a raised-cosine onset of a 50 Hz tone has almost no energy a few hundred hertz away, so the band-pass barely rings. The weight must start only after the ramp ends. The last row shows that overlapping the two does not work.

**P4. Listen engages in 50 ms:** the listen filter's input rises over 30 ms (the `Fade.h` raised cosine at 30 ms), then the output crossfades over the standard 20 ms. Leaving listen (back to the warm EQ path) is the plain 20 ms crossfade. A band change while listening is the same two-step move between two listen voices: the incoming voice's input ramps for 30 ms, then the 20 ms crossfade. The 1 kHz κ (2.36 – 2.41x) is the generic level-change κ every 20 ms fade shows (G3 step rows: limit 2.5x, 2.20x measured in 2.4). This is call C2: it makes listen 30 ms slower to engage than ARCHITECTURE's 20 ms.

The bench ran at 48 kHz only. The plan's rows run it at 96 and 192 kHz as well, where the 30 ms is longer in samples.

### Structure

**P5. `eq::Listen`, a std-only header the designer harness can drive.**

- **Voices.** Two listen voices, each an `eq::Band` slot plus a solo placement. Each voice has its own input-ramp position and is designed by the processor's `Designer`.
- **Fade.** One fade between the EQ path and the listen path, and one between voices, both on the processor's `FadeTable` for the 20 ms part. The 30 ms input ramp is a second table built in prepare.
- **Requests.** The rule is the SlotBand latch: a request during a fade waits for its end.
  - **Listen index change** (−1 ↔ band, or band A → band B): always the two-step move above, never a ramp. A ramp would sweep a narrow band-pass across octaves in 20 ms, which is a chirp. The bench's continuous BP → BP ramps read 23 – 76x (`listen`, band change rows).
  - **The listened band's frequency or Q edited:** a continuous target on the active voice, ramped like a band (G3 sweep rows).
  - **Its type or placement changed:** the two-step move.
  - **Enable and bypass do not stop listen.** The listened region is defined by the band's parameters. Whether the UI ends listen when a band is disabled is a Stage 3 call.
- **Reading listenBand.** `listenBand` (`std::atomic<int>`, −1 off, not saved, UI state like `analyzerMode`) is read at the block start and handed over at the next control tick. Placement takes the layout's effective placement. Listen therefore renders identically at any block size, like every other request.
- **Seeding.** At the first block and both un-bypass restarts, listen starts settled on the wanted state: no fade, the voice warm from rest, as the bands do. The bypass fade covers it.
- **Idle.** Listen is idle when the target is −1, no fade is in flight and nothing is pending. Idle, it does nothing and takes no copy, so the processor's step 5 (no running band and the stage idle: return untouched) and step 6 (the converted lanes) extend with one more term each. Not idle: every lane is converted, the dry copy is taken, and a global bypass toggle fades.
- **Cost:** one or two SVF sections per lane, two voices only during a fade. Not measurable against X-cpu.

### Where it sits (C3)

| Order | Effect |
|---|---|
| ARCHITECTURE: bands → **listen** → auto gain → output gain | The solo is scaled by the auto-gain estimate of the EQ curve it no longer contains. With a +12 dB shelf in, auto gain is about −6 dB and the solo plays 6 dB low; with deep cuts in, it can play up to +12 dB loud. |
| **Recommended:** bands → auto gain → **listen** → output gain | The solo is the input's region at unity, trimmed by output gain only. |

Cost of the change:

- With listen idle, `OutputStage` keeps its single fused multiply `ag × og`, so every existing row is unchanged bit for bit.
- With listen not idle, the stage runs as two multiplies (EQ path × ag, the mix, then × og). That is a rounding-level difference only while listen is in use.
- Amends ARCHITECTURE "Processing Chain" and "Processing Order Requirements" item 3.

### P6 (rows)

| Row | What | Expected |
|---|---|---|
| Listen G3 on / off | the bench set (20 bands incl. narrow band-passes 150 Hz – 8 kHz), 50 Hz and 1 kHz, blocks 32 / 512, 48 and 96 kHz | (a) ≤ 2.0x (bench 1.21x), (b) ≤ 2.5x (2.36x); hard control (no ramp, no fade) ≥ 10x, or the row is void |
| Listen band change | ordered pairs from that set (sampled: at least 60) | (a) ≤ 2.0x (1.35x), (b) ≤ 2.5x (2.41x) |
| Listen sweep | listened bell's frequency 10 Hz → 30 kHz in 50 ms, Q 0.025 → 40 in 20 ms | the G3 sweep form, ≤ 4.0x a same-motion reference |
| Listen off identity | sessions rendered with listen never on, against the same sessions through a processor built without the listen path | `memcmp` equal |
| Listen back off | listen on → off, no band, output 0 dB: from the end of the fade back, `memcmp` equal to the input | from end + 0 samples, the stage idle |
| Solo routing | each placement, stereo and mono: listen output per the table above; mono maps every placement to F(x) | within 1e-12 of a harness render |
| Block identity | listen events off the tick grid, blocks 1 – 2048 and offline at 44.1 / 96 / 192 kHz | bit-identical |
| Seeding | listen on at session open and changed while bypassed, by both routes: listen output from the first processed sample, equal to a listen started settled | exact |
| C3 placement | auto gain on with a +12 dB shelf: the listen output level is independent of the auto-gain estimate | within 1e-9 dB |

For "listen off identity" the cleanest form is a harness-only second build: the processor compiled with the listen hook removed by a macro that only the harness sets, rendering the same sessions to `memcmp`. The plan can instead argue it structurally (listen idle executes no instruction on the sample path) and rely on the 122 existing rows, which are all listen-off renders. The macro is the literal reading of the roadmap criterion.

---

## Q4. Display model (UI-02 C++ half)

[ARGUED, plus bench `json` / `cost`]

- **Same inputs as the audio thread.** Per band, the display model builds the `BandInput` exactly as `makeInput` does:
  - the eight raw atomics and the raw `gain_scale`;
  - `eq::makeBandInput (raw, gainScale, 0.0)`;
  - `effectivePlacement` with the processor's layout.

  It then calls `designer.design` on its own `eq::Designer`, prepared at the processor's rate. The plan should move `makeInput`'s two lines to a shared free function in `BandParams.h`, so there is one definition.
- **The rate and the layout.**
  - The polish decision depends on the rate (`Designer::prepare`), so the display needs the exact rate the processor prepared, not a guess. The processor publishes `std::atomic<double> preparedRate` and `std::atomic<bool> preparedStereo` in `prepareToPlay`.
  - The display re-prepares its designer and sends everything when either changes.
  - Before the first prepare it uses 48 kHz stereo and marks the packet `rateKnown: false` (the page can draw it, labelled).
- **Change detection by polling, not listeners.** On each editor tick the model reads the 196 atomics (cost: noise) and compares each band's 9-float snapshot by bit pattern, as the processor does. Changed bands are designed and sent.
  - A parameter listener would fire on whatever thread set the value (the audio thread under automation) and design there.
  - Polling covers a preset load, host automation and a state restore with one rule, so the "push after preset load" of ARCHITECTURE needs no hook (suite lesson: one-shot pushes go stale on preset load).
  - A page `init` request answers with all 24.
- **Packet:** `{ band, type, placement (effective), active, sections: [[g, k, mL, mB, mH] …], onePoles: [[G, cL, cH] …] }`, plus a header `{ sampleRate, stereo, rateKnown }`.
  - `active` must mirror the engine's: false when the band is inactive, and false when it sleeps (a gain type at exactly 0 dB, `gainIsZero`). The engine does not run a sleeping band, and the display must not draw a design the engine is not running. A dedicated G9 row draws such bands.
  - Sections without an SVF or one-pole half send that half's neutral values, or omit it; the packet says which.
- **Precision.** JUCE serialises a `var` double through `serialiseDouble`:
  - normal range: 13 – 20 decimals by magnitude, about 16 significant digits;
  - scientific form outside [1e-5, 1e6]: 15 decimals.

  Rounding every parameter to 16 significant digits moves the drawn responses by ≤ 1.5e-8 dB over 400 random 1 – 24-band draws per rate [BENCH]. JSON is fine for G9 (b)'s 0.01 dB. Designs are finite by DSP-06, so no NaN reaches JSON.
- **Cost:** a full push of 24 designs is 5.8 µs (bells), 3.9 µs (cuts) and 41 µs (shelves at 96 dB/oct, polished) at 44.1 and 96 kHz [BENCH]. Building the `var` dominates and stays on the message thread.
- **Where it runs in Stage 2:** a plain class with `poll() → list of changed packets` and `packetFor (band)`. The render harness constructs it on the test thread against a live processor. Stage 3 adds the timer and `emitEventIfBrowserIsVisible` (and the suite lesson that emits are dropped while the browser is hidden: the page pulls on `init`).

The display shows the target; the audio reaches it within one 20 ms ramp, and within 50 ms for a crossfade with a listen pre-ramp. The curve is correct with the transport stopped, because nothing in it comes from `processBlock`.

---

## Q5. G6, CPU, and the rows carried from 2.5

**G6 (PERF-01).** The A-rows extended: analyzer consumer registered, mode 3, **no reader** (the rings fill in about 0.7 s and then every block overflows), listen on and switching between bands every 100 ms, 24 bands moving, auto gain refreshing. Expected: 0 allocations, 0 frees. The ring write, the drop counter, the listen voices and the input ramp are all fixed storage. [ARGUED; the plan's `--neg` control: a `std::vector` push in the tap must fire]

**CPU (INFO).** Two read-only passes over the block for the taps, and listen at one or two sections per lane: X-cpu4 plus less than 1 %. [ARGUED]

**Owed rows, VERIFICATION-2.5 Issue 2.** Each is caught today only by the 2.5 probe:

| Mutant | Row | Template |
|---|---|---|
| PX17 | latched un-bypass with no band and output gain ≠ 0 dB: the gain is in effect from the first processed sample (no fade-in) | probe S3 / S3b |
| PX11 | output-gain edits off the tick boundary at blocks 1 – 2048: bit-identical | probe T1 / T3 |
| PX4 | auto-gain toggle in an otherwise idle block (empty list, stage at unity): lands on the refresh tick | probe T2 |
| AG3 | a G12 auto-gain row at 22.05 or 32 kHz, where the 0.45 fs grid top is active | probe E1 |
| OS18 | designer-harness row: `GainSmoother` against a restated four-stage cascade, bit-exact through the snap | probe T3 |

**VERIFICATION-2.4 Issue 1 (stop-band burst), status.** Not in this phase's scope, and still a listening item. Q3 adds evidence and a remedy. The burst is the incoming slot's start-up ring, and the input fade-in that cures listen would cure it too: the incoming slot's input ramps for R, then the 20 ms crossfade. The cost is structural changes taking 20 + R ms (40 ms at R = 20, which read ≤ 1.91x across the listen set). If the listen confirms Issue 1 is audible, `SlotBand` can take the same ramp as an option; the infrastructure (a second fade table) arrives with listen.

---

## Calls

**C1 (Taylor's): G9 (a) is measured on the damped circle, scored relative to each response's maximum.**
- **Why:** as written (unit circle, "above −80 dB" absolute) it cannot pass. Impulse-response truncation near slow resonances misreads by up to 4 dB, 2 – 6 % of draws do not decay within the harness cap, and float quantisation of high-gain chains reaches 0.05 dB at −80 dB absolute.
- **Recommended:** the damped measurement (N = 2^17, a^N = 1e-9) with the closed form at the same z, ≤ 0.05 dB within 80 dB of each drawn response's maximum (bench ≤ 1.2e-3 dB). Add a unit-circle confirming row on decaying draws and a bit-identity row (packet = realised design).
- **Alternative:** unit circle only, restricted to decaying draws (bench ≤ 0.04 dB, thin margin at 48 kHz).

**C2 (Taylor's): listen engages over 50 ms (a 30 ms input ramp, then the 20 ms crossfade) instead of 20 ms.**
- **Why:** a filter started from rest bursts on stop-band material, up to 29 672x the G3 step limit. Pre-rolls up to 200 ms do not fix every case; the input ramp fixes all of them with margin (1.35x).
- **Alternative:** a 20 ms input ramp (40 ms total, 1.91x worst, thin margin).

**C3 (Taylor's): listen sits after auto gain, before output gain.**
- **Why:** so the solo is not scaled by the auto-gain estimate of an EQ curve it does not contain.
- **Alternative:** keep ARCHITECTURE's order (listen before both gains); the code is the same size either way.

For the plan (no call needed):

- P1 G9 (a) rows from `DisplayModel` packets of the processor under test; designer-harness exactness row; packet-equals-realised-design row; the four controls; complex-s response evaluation.
- P2 analyzer: Hann, ARCHITECTURE frame sizes, zero-pad 4x, scale 2 / Σw, max-in-span with interpolation for empty spans, per-bin release 20 dB/s on a passed dt.
- `SpectrumTap`:
  - fixed storage from construction;
  - generation and drop counters;
  - newest-samples-fit overflow;
  - consumer flag × mode bits;
  - taps around `processAudio`, read-only.
- P4 / P5 `eq::Listen`:
  - two voices, solo router, input ramp then crossfade;
  - an index, type or placement change is the two-step move; a frequency or Q edit ramps;
  - read at the block start, applied on the tick;
  - seeded;
  - idle predicate extends steps 5 and 6;
  - dry copy shared with the bypass mix.
- `DisplayModel`:
  - shared `makeInput` free function;
  - `preparedRate` / `preparedStereo` atomics;
  - polling snapshot compare;
  - packet `active` mirrors sleep;
  - `rateKnown`.
- The rows of P3, P6 and Q5, including the five owed from 2.5.
- Contract amendments:
  - ARCHITECTURE "Analyzer" (zero-pad, storage at construction, overflow rule);
  - § 10 and "Processing Chain" (listen timing; C3 order);
  - G9 (C1);
  - G6 text (listen on);
  - ROADMAP 2.6 test criteria to match;
  - refresh the checksums in STATUS.
