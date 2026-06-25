# Stage 2 (DSP) — RESEARCH

**Mode:** Non-interactive (express). The deep DSP research lives in `ARCHITECTURE.md`
(Stage 0, depth DEEP). This file captures the **concrete implementation decisions** derived
from re-reading the immediate sibling source (O-simpleFM) and resolves the porting specifics.

## Sibling port map (O-simpleFM → O-simpleSubtractive)

| O-simpleFM file | O-simpleSubtractive analog | Change |
|-----------------|---------------------------|--------|
| `Operator.h` (fastSine LUT) | `OscillatorBank.h` | Replace single sine op with PolyBLEP saw/square/tri + LUT sine + sub-square + noise mixer. |
| `FMVoice.h` / `FMSound` | `SubVoice.h` / `SubSound` | osc→filter→VCA per sample; filter ADSR → cutoff (octaves) + amp ADSR → VCA; SVF state per voice. |
| (none) | `SvfZDF.h` | NEW: Cytomic ZDF SVF core (LP/BP/HP simultaneous; Notch=src−k·BP; 1-pole 6 dB; cascade ×2 24 dB; tanh self-osc). |
| (none) | `MonoController` (in processor) | NEW: held-note stack, last-note priority, Mono/Legato + glide. |
| `FmVizAnalyzer.h` (`VizRing`+FFT) | `SubVizAnalyzer.h` | `VizRing` verbatim; analyzer adds closed-form filter-curve calc (Stage 3 draws it). |
| `PluginProcessor.{h,cpp}` | same | Synthesiser wiring; **drop the 2× oversampler entirely** (PolyBLEP → zero latency). |
| `tests/render-harness/` | same | Port harness; swap FM probes for SVF probes (per-mode magnitude, self-osc, aliasing). |

## Key simplification: NO oversampling

O-simpleFM oversamples 2× because hard phase-modulation generates sidebands above Nyquist that
PolyBLEP cannot fix. O-simpleSubtractive runs at a **steady phase increment** → PolyBLEP/polyBLAMP
band-limit at source. Therefore:

- `processBlock` is **much simpler**: `buffer.clear()` → `midiCollector` merge → `pushParamsToVoices()`
  → `synth.renderNextBlock(buffer, midi, 0, numSamples)` → output gain → isfinite scrub → VizRing tap.
  No `processSamplesUp/Down`, no chunking, no `scaledMidi`, no `kOsFactorLog2`.
- `setLatencySamples(0)`. Voices prepare at the **host** rate (not osRate).
- This removes O-simpleFM's most complex code path — a net win for a teaching tool.

## SVF core (verified against ARCHITECTURE.md §Algorithm Details)

Per sample, one 2-pole stage (states `ic1eq`, `ic2eq`):
```
g  = tan(π·fcEff/fs);  k = 1/Q
a1 = 1/(1 + g(g+k));  a2 = g·a1;  a3 = g·a2
v3 = x − ic2eq;  v1 = a1·ic1eq + a2·v3;  v2 = ic2eq + a2·ic1eq + a3·v3
ic1eq = 2·v1 − ic1eq;  ic2eq = 2·v2 − ic2eq
lp = v2;  bp = v1;  hp = x − k·v1 − v2;  notch = x − k·v1
```
- **Slopes:** 6 dB = one 1-pole TPT (`FirstOrderTPTFilter`-equivalent inline; BP/Notch degrade to gentle shelves — "a 1-pole can't resonate"); 12 dB = one SVF; 24 dB = two cascaded SVFs at the same `fcEff`, **resonance on stage 1 only** (O-Prism precedent — prevents runaway peak).
- **Coefficient cost:** recompute `g,a1,a2,a3` per-sample for zipper-free env modulation. 16 voices × `tan`/sample is acceptable (ARCHITECTURE Risk #6); sub-block fallback documented but not needed for v1.0.
- **fcEff (per sample):** `clamp(cutoff · 2^(keyTrack·(note−60)/12) · 2^(filterEnvAmount·eF·ENV_OCT), 20, min(20k, 0.45·fs))`, `ENV_OCT ≈ 7`.

## Self-oscillation (Phase 2.2)

- **Nonlinear variant:** apply `tanh` to the resonant feedback term — replace `k·v1` influence by driving the resonant integrator through a soft saturator. Concretely (Cytomic clean-sine form): bound `v1`/`ic1eq` with `tanh` so the limit cycle settles to a clean bounded sine at `fcEff` as `k→0`. tanh IS the gain compensation (caps amplitude — sweeping resonance to max cannot blow up).
- **Resonance map:** `resonance 0..1 → k`. `k = 2·(1 − resonance^p)` with taper `p` (~1.0–1.5) so the top of the knob reaches `k≈0` (self-osc). At `resonance=0`, `k≈2` (heavily damped, no peak); Butterworth ≈ `k=1.41`. Clamp `k ≥ 0`. (Exact taper tuned against the render-harness self-osc + curve checks.)
- **Make-up trim (secondary):** gentle `outGain *= 1/(1 + c·resonance)` so resonance sweeps don't jump in loudness.

## Closed-form magnitude curve (Phase 2.2 — QUAL-02 foundation)

`computeMagnitudeDb(f, fcDisplay, k, type, slope)`: `Ω = tan(π·f/fs)/g`, `g = tan(π·fcDisplay/fs)`;
`|D| = sqrt((1−Ω²)² + (kΩ)²)`; LP=1/|D|, HP=Ω²/|D|, BP=kΩ/|D|, Notch=|1−Ω²|/|D|; 24 dB → square |H|;
6 dB → one-pole `D1 = 1+jΩ`. **Same `g/k` math the audio thread uses** → exact match by construction.
Lives in `SubVizAnalyzer` (header-only, callable from the render-harness for the curve-vs-measured check).

## Render-harness probe design (the Stage-2 gate)

Port O-simpleFM's harness; replace FM-specific probes (Bessel null, feedback) with SVF probes:

1. **makes-sound** — default params, note-on → non-trivial RMS, finite.
2. **pitch=fundamental** — Saw → dominant partial at played freq; Sine passes filter ~unchanged.
3. **filter modes** — LP attenuates HF vs source; HP attenuates LF; BP peaks at cutoff; Notch dips at cutoff.
4. **filter slopes** — 24 dB steeper rolloff than 12 dB than 6 dB (compare stopband attenuation an octave above cutoff).
5. **filter env independence** — filterEnvAmount sweeps cutoff with amp sustain flat (amp ADSR unaffected); bipolar sign flips sweep direction.
6. **keyTrack** — cutoff rises with note when keyTrack>0.
7. **aa-highpitch** — Saw at C7, inter-harmonic alias energy ≪ harmonic energy; finite/bounded (QUAL-01, DSP-06).
8. **self-osc clean** (2.2) — max res + no input → strong bin at cutoff, low THD, bounded peak, no DC.
9. **self-osc in tune** (2.2) — keyTrack=100%, max res, two notes an octave apart → self-osc freq ratio ≈ 2.
10. **curve-vs-measured** (2.2) — swept-sine measured magnitude matches `computeMagnitudeDb` within tolerance (per mode).
11. **voice modes** (2.3) — Poly two notes → both present; Mono two notes → only last; glide ramps pitch over time; no NaN.

`peakAbs < ~4`, `allFinite`, and bounded RMS guard every scenario (catches self-osc blow-up).

## Pitfalls (project memory + sibling lessons)

- **LUT wrap:** `LookupTableTransform` CLAMPS out-of-range inputs — floor-mod the sine phase into [0, 2π) before lookup (O-simpleFM `Operator.h` critical note). Applies to the sine oscillator path.
- **ADSR order:** `setSampleRate` before first `setParameters` (JUCE 8 ADSR).
- **`getLatencySamples()` non-virtual** in JUCE 8 — call `setLatencySamples(0)`, don't override (project memory).
- **No virtual `SynthesiserVoice::prepareToPlay`** — declare a non-virtual `prepareToPlay(double,int)` and dispatch via `dynamic_cast` (O-Bassoon/O-simpleFM pattern).
- **In-place FFT** clobbers its buffer — copy scope before FFT (deferred to Stage 3, but `VizRing` API already enforces it).
- **BinaryData NAMESPACE** collision — N/A this stage (no binary-data target until Stage 3 UI).
- **MonoController stuck-notes** — held-note stack with last-note priority; pop on note-off; release the single voice only when the stack empties (ARCHITECTURE Risk #4).
