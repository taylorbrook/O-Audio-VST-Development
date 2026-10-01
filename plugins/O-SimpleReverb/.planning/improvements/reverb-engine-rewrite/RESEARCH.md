# Research: Reverb engine rewrite

**Plugin:** O-SimpleReverb
**Milestone:** reverb-engine-rewrite
**Created:** 2026-09-30
**Phase:** Research

Numbers in this document marked *measured* come from a standalone C++ prototype of the three
engines (`prototype/`, no JUCE, double precision; full output in `prototype/results.txt`).
Nothing in `Source/` was changed and v1.14.0 was not rendered — the v1.14.0 baseline is still an
execute task (CONTEXT requirement 10). Literature constants were checked against the sources
listed under [References](#references); items that could not be verified are marked.

---

## Context Summary

From CONTEXT.md:
- **Core requirement:** replace the single `juce::dsp::Reverb` with an FDN (Booth/Room/Hall/Ambient),
  a Dattorro plate and a dispersive spring, behind the same 8 parameters and UI. DECAY becomes a
  calibrated multiplier on a per-type base RT60; SIZE moves delay lengths only and glides.
- **Scope:** engines, output-mixed stereo early reflections, RT60 calibration, 48 presets re-voiced,
  render-check re-anchored and extended. No new parameters, no UI/layout change, no legacy mode.
- **Success criteria:** measured RT60 matches the table and holds across SIZE; spring chirps; plate
  shimmer builds in the tail and cannot run away; every control that is click-free stays so;
  `params.tsv` and v1.14.0 state load unchanged.

---

## Implementation Approach

### Recommended Approach

**Strategy:** three header-only engines behind a two-slot ring-out host, calibrated by formula and
gated by measurement.

The engines are small per-sample classes in `Source/dsp/`, each taking stereo in and giving stereo
out, with `prepare / reset / setSize / setT60 / process`. `PluginProcessor` keeps everything it has
after the reverb (type EQ, CHARACTER, LOW CUT, wet/dry, meter) and replaces everything before and
including `reverb.process()`. Each engine converts a decay time in seconds to its own loop gains,
so DECAY x base RT60 is one number handed down, and SIZE is one scale factor that the engine slews.

The findings that shape the plan, in the order of CONTEXT's open questions:

#### 1. FDN (Booth, Room, Hall, Ambient)

| Decision | Choice | Evidence |
|---|---|---|
| Matrix | Hadamard (fast Walsh-Hadamard, N log N adds) | Householder for N > 4 is diagonal-dominant and "approaches a bank of decoupled parallel comb filters" (JOS). Measured without input diffusion: mixing time 288 ms (Hadamard) vs 390 ms (Householder) at N=8; L/R correlation +0.01 vs +0.15. |
| Line count | **16, not the brief's 8** — see Open Question 1 | Schroeder's mode-density rule is total delay >= 0.15 x T60. N=8 gives Hall 0.38 s against 0.45 s needed at base decay and Ambient 0.54 s against 1.05 s. N=16 gives 0.74 s and 1.06 s. RT60 accuracy is the same for both. |
| Delay set | Geometric ladder between a per-type min and max, with a per-type +/-3.5 % irregularity, scaled by SIZE | Tail spectra of any two types correlate at -0.08..+0.07 (measured, 200-2000 Hz). The same delays with different damping — what v1.x does — correlate at +0.93. |
| Per-line gain | `g_i = 10^(-3 L_i / (fs T60))`, recomputed from the line's *current* length | JOS/Jot formula, verified. This is what holds RT60 while SIZE moves. |
| Two-band decay | Per line: `y = g_hi x + (g_lo - g_hi) LP1(x)`, one-pole LP at a per-type crossover. `|H| <= g_lo < 1`, so the loop is stable for any setting | Simpler than Jot's pole/tone-corrector pair and needs no output corrector. |
| Mid-band calibration | Solve `g_lo` so that `|H(1 kHz)|`, not `H(DC)`, meets the target (closed-form quadratic) | Without it Hall reads -5.8 %; with it every one of 26 rows is within -2.0..+3.0 % (measured, 44.1/48/96 kHz). |
| Modulation | Odd-numbered lines only, sine, per-line rate spread 0.6x-1.6x, deterministic start phases, depth in ms (Room 0.06, Hall 0.15, Ambient 0.25; Booth none) | Deterministic phases keep render-diff reproducible. Rates stay well under Schlecht's 4 Hz "natural" limit. |
| Interpolation | 3rd-order Lagrange on fractional reads; unmodulated lines settle on **integer** lengths | Measured 8 kHz RT60 on Hall: integer 1.75 s, modulated Lagrange 1.68 s (-4 %), modulated linear 1.48 s (-15 %), all-fractional linear 1.35 s (-23 %). Linear interpolation in the loop is a second, uncontrolled damping filter. |
| Input diffusion | 4 Schroeder allpasses per channel (0.75, 0.75, 0.625, 0.625), L and R lengths different, **scaled per type** | An allpass rings for `3 D / -log10(g)` seconds: 12.7 ms at 0.625 is 187 ms. With Hall-sized diffusers Booth at 0.5x read 0.232 s against a 0.20 s target (+15.8 %); with the diffusers at x0.25 it reads 0.202 s. |
| True stereo | L and R injected through two different Hadamard rows, read through two others | Late-tail L/R correlation -0.12..+0.02 on all rows, with no fixed offset. |

The **HF decay time is a voicing, not a calibrated quantity.** The one-pole shelf does not reach
`g_hi` until well above the crossover, so the 8 kHz-octave RT60 reads 5-40 % longer than
`hfRatio x T60`. The plan should gate "HF RT60 < mid RT60" and a per-type measured value, not the
nominal ratio.

#### 2. Early reflections

A multi-tap delay per channel fed from the pre-delayed input (before diffusion), 6-8 taps per side
with different times and alternating polarity, gains falling as 1/t, tap times scaled by SIZE and
mixed to the **output** bus of the slot at a per-type level. FDN types only: Plate's acceptance
criterion is "no discrete early taps" and a spring tank has none. The taps sit ahead of the type
EQ, CHARACTER and LOW CUT with the rest of the wet signal. No prototype was needed; the unit can be
gated on its own impulse response exactly as `FlutterDelay` is today.

#### 3. Holding RT60 while SIZE moves, and the glide

RT60 holds because gain is a function of current length (above): measured mid RT60 moves by about
4 % at most between SIZE x0.5 and x2.0 (Hall, the worst type). For the glide, a one-pole slew on the size scale:

| Slew time constant | HF burst above 3 kHz, re wet peak (measured, Hall, 300 Hz feed, x0.5 <-> x2.0) |
|---|---|
| none (jump) | -11.5 dB |
| 50 ms | -40.7 dB |
| 150 ms | -53.0 dB |
| **250 ms** | **-63.4 dB** |
| 400 ms | -65.4 dB |

250 ms is in the region of the existing smoothed-WET reference (about -60 dB, though that figure is
re the input level, not the wet peak). A second-difference
metric is the wrong gate here: it rose 4.3x at 250 ms purely because the pitch bend raises the
frequency. Use the HF-burst metric of render-check section 5.

Recommended SIZE maps (geometric, base value at SIZE 50):

| Engine | Scale at SIZE 0 / 50 / 100 | Why |
|---|---|---|
| FDN | x0.5 / x1.0 / x2.0 | RT60 verified across the range |
| Plate | x0.40 / x0.57 / x0.80 of Dattorro's lengths | density, see 4 |
| Spring | x0.75 / x1.0 / x1.33 of the echo time (25-55 ms) | keeps the tank inside real-spring territory |

#### 4. Plate (Dattorro)

All of Dattorro's constants were confirmed against the paper's Fig. 1 and Tables 1-2: 29761 Hz
design rate, input diffusers 142/107 (0.75) and 379/277 (0.625), tank 672 -> 4453 -> damp -> 1800 ->
3720 and 908 -> 4217 -> damp -> 2656 -> 3163, decay diffusion 0.70 (sign inverted) and 0.50, the 14
output taps at +/-0.6, excursion "about 8 samples" peak at "on the order of 1 Hz". One rule to add:
`decay diffusion 2 = decay + 0.15, floor 0.25, ceiling 0.50`. All lengths scale by `fs / 29761`.

**Decay in seconds.** The paper gives no relation ("everything must be set by ear"). Derived here:
one trip round the figure-8 passes four `decay` multiplications and a mean delay of the four delay
lines plus the four allpass lengths (0.725 s at Dattorro's size), so

```
decay = 10^( -3 * loopSeconds * 1.055 / (4 * T60) )
```

The 1.055 is empirical: the bare formula reads +3.5..+7.4 % long at 2.5 s and above (more at
1.25 s), and by the same amount at 44.1 and 96 kHz. With it, measured mid RT60 is within
-1.1..+2.0 % over size x0.4-0.8 and T60 1.25-5 s (48 kHz).

**Density is this engine's weak point.** Dattorro's tank deliberately undershoots Schroeder's
criterion. Normalised echo density (1.0 = Gaussian) at 100 ms after the impulse, measured:
FDN Hall 0.99; plate at Dattorro's own size 0.60; at x0.55, 0.84; at x0.40, 0.89. Raising the
input-diffusion coefficients made it worse. Hence the small SIZE range above. At the top of the
range and the shortest decay the tail decays in audible steps (loop 0.58 s against a 1.25 s RT60).

**Shimmer in the loop.** `OctaveUpShifter` goes on the two cross-feeds as a convex mix:

```
feedback = decay * ( (1 - s) * x + s * shift(lowpass(x)) )
```

- *Stability.* The shifter's two windows sum to 1 and each input sample is read with total weight 1,
  so its L2 gain is <= 1; a convex mix of it with the identity is <= 1; the rest of the loop is
  allpasses and delays. Loop gain is therefore bounded by `decay`, whatever `s` is. Measured with
  `decay` forced to 0.9999 and damping off: energy keeps falling across three checkpoints over
  40 s at s = 0.08, 0.25 and 1.0.
  Clamp `decay <= 0.97` and requirement 3's "loop gain below 1 at every setting" holds by
  construction.
- *Decay time.* The shifter drains the mid band (energy leaves upward): uncompensated, s = 0.08
  shortens a 5 s tail by 10-19 %. Dividing `decay` by `sqrt(1 - 0.9 s)` (two shifter crossings per
  four decay stages) brings all 27 measured rows within -4.6..+2.0 %.
- *Bloom.* 300 Hz burst, size x0.55, s = 0.08: the octave's level against the fundamental is
  -32 dB while the burst sounds, -14 dB in the second after it stops, -8 dB at three seconds. It
  grows over the tail, as required, and the next octave stays 22 dB further down.
- *Level.* The bloom rate depends on crossings per second, so it is faster at small SIZE. `s`
  should be derived from loop time (constant transfer per second), and its value — the prototype
  suggests 0.03-0.08 at SIZE 50 — is a listening decision. v1.14.0's one-shot shimmer is about
  -10 dB from the first sample; anything in this range is far subtler at the onset.
- *Aliasing.* A one-pole low-pass ahead of the shifter keeps its input below about fs/4.

`OctaveUpShifter` needs no change. `FlutterDelay` loses its last caller.

#### 5. Spring

Per spring: a feedback loop holding a delay line, a cascade of M stretched first-order allpasses
`(a + z^-K) / (1 + a z^-K)` with `K = round(fs / (2 fC))`, a 6th-order low-pass at the transition
frequency fC, a 100 Hz high-pass, and an inverting feedback gain (the low-chirp structure of
Välimäki, Parker & Abel 2010; the published values are M = 100, a = 0.62, fC 2-5 kHz, feedback
about -0.8). The separate high-frequency chirp path is omitted: it is "at least 30 dB below" the
low chirps and Parker says it may be dropped.

- **Chirp:** group delay rises towards fC. Analytic first-echo time at M = 80, a = 0.62, fC 4.5 kHz:
  39.8 ms at 300 Hz, 43.5 ms at 3 kHz, 54.4 ms at 4 kHz — a 14.5 ms upward chirp that lengthens
  with every echo. M = 120 gives 21.8 ms. The measured first echo arrives later with frequency as
  predicted up to 3 kHz; at 3.8 kHz the low-pass trims the latest arrivals (45.9 ms measured,
  50.6 ms predicted).
- **Decay:** `g = 10^(-3 * echoSeconds(1 kHz) / T60)` where the echo time includes the cascade's
  group delay at 1 kHz. Measured mid RT60 within +/-0.7 % at 44.1/48/96 kHz and T60 1.25-5 s.
  The 2-4 kHz band rings 1-4 % longer (longer echo time there), as on a real spring.
- **Amp-tank voicing:** three springs with echo times 33 / 37 / 41 ms at SIZE 50 (the Accutronics
  figures), fC about 4.3-4.8 kHz, slightly different M per spring. Real tanks: 1.75-3.0 s "medium",
  2.75-4.0 s "long" — the 2.5 s base is in range. Usable band roughly 100 Hz - 4/5 kHz.
- **Stereo:** springs are mono devices. Feed L, R and (L+R)/sqrt2 to the three springs and mix them
  to L/R with different weights. True stereo is a requirement of the FDN only.
- **Cost at 96 kHz:** the stage count does not change with sample rate, only K (memory). Three
  springs at M = 80: 2.2 % of one core at 96 kHz in the prototype.
- The delay-line modulation in the literature is filtered noise; a deterministic slow wobble is
  recommended instead so renders stay reproducible (project pattern).

#### 6. RT60 measurement and tolerance

Impulse in, left channel out, band-passed to the 500 Hz + 1 kHz octaves (354-1414 Hz, 4th order),
Schroeder backward integration, least-squares line over -5..-35 dB (ISO 3382 T30), extrapolated to
60 dB. Render length `1.35 x T60 + 0.4 s`. The whole grid is about 30 impulse responses and
150 s of audio; the prototype renders it in a few seconds.

**Tolerance: +/-10 %**, as CONTEXT suggested. Worst measured prototype errors: FDN +3.0 %, plate
-4.6 %, spring +0.7 %, so the gate has at least twice the margin. ISO 3382-1 puts the audible
difference at 5 %; later studies report 6-39 %.

#### 7. Engine switch on TYPE

Recommended: **two slots, ring-out.** A slot owns one engine of each kind (only the active one is
processed), its early reflections, its input trim and its type EQ. On a TYPE change the playing
slot's *input* ducks to zero over the existing 10 ms and it keeps running, so the old tail rings
out under its own type and EQ; the other slot is reset and its input rises. A slot is retired when
its output has stayed below about -100 dB. A third change while both slots sound steals the older
one with a fast output fade, which is the present duck-and-swap as the fallback.

This maps onto what exists: `typeEq[2]` and its crossfade become the two slots' EQs, and the
input-side duck and wet trim stay where they are. It also preserves today's behaviour — in v1.14.0
the tail keeps ringing through a type change because every type shares one tank. Clearing at the
duck's silent point would cut the tail on every type and preset change, which is a regression.

#### 8. Mono

Engines always run stereo. On a mono bus, feed L = R = x and write `0.7071 (L + R)`: the tails are
decorrelated, so this keeps power. The wet-level gate should be run in mono as well.

#### 9. Robustness

- **Capacities from the running rate.** FDN lines need `maxMs x 2.0 (SIZE) x 1.035 + modulation`;
  at 192 kHz that is about 48 k samples per line, 3 MB per FDN.
- **Oversized host block.** `processBlock` already calls `setSize(..., avoidReallocating)` on its
  scratch buffers, which allocates if the host block exceeds the prepared size. Process in chunks
  of at most the prepared block size.
- **NaN.** Check the slot output once per chunk; on a non-finite value reset that engine and output
  silence for the chunk, so it recovers on the next one.
- **`getTailLengthSeconds()`**: 15.0 (Ambient 14 s at 2.0x plus pre-delay).
- **Per-line gain recompute.** `pow` per line per sample is only needed while SIZE or DECAY moves;
  recompute on change and per 16-32 samples during a glide.

**Pros:**
- Every open question in CONTEXT now has a measured answer; RT60 is hit by formula, not by trimming.
- Stability of each loop is provable from its structure (gains < 1, convex shimmer mix).
- Header-only engines can be measured directly by render-check, as `ModulationFx.h` is today.

**Cons:**
- 16 lines and two slots roughly quadruple the memory and, during a ring-out, double the CPU.
- The plate's echo density cannot match the FDN's; SIZE on Plate is a narrower control.
- The shimmer level and both HF voicings remain listening decisions.

### Alternative Approaches Considered

**Alternative 1: 8-line Householder FDN (the brief as written)**
- Why not chosen: misses the mode-density rule on Hall and Ambient at base decay, and Householder
  mixes poorly above N = 4. Kept as Open Question 1 because it departs from the approved brief.

**Alternative 2: clear the engine at the duck's silent point (one slot)**
- Why not chosen: cuts the tail on every type or preset change; today's tail survives.

**Alternative 3: keep inactive engines running in parallel**
- Why not chosen: three engines always on for no audible gain over ring-out.

**Alternative 4: Jot's per-line pole + output tone corrector for two-band decay**
- Why not chosen: needs a corrector filter per output and exact pole solve; the shelf form is
  unconditionally stable, and mid-band accuracy is already within 3 %.

**Alternative 5: lift O-Strata's `ReverbProcessor` for the plate**
- Why not chosen: same topology, but linear-interpolated modulation, uncalibrated decay and no
  shimmer path. Use it as a reference for the tap wiring only.

**Alternative 6: additive shimmer (`x + s * shift(x)`)**
- Why not chosen: loop gain bound becomes `decay (1 + s)`, which is only conditionally stable.

---

## Affected Components

### Files to Modify

| File | Changes | Complexity |
|------|---------|------------|
| `Source/PluginProcessor.h` | Remove `juce::dsp::Reverb`, pre-delay/ER/allpass/flutter members and constants; add two engine slots; reshape `TypePreset` (base RT60, size range, delay set, HF ratio, crossover, modulation, ER table, wet trim); `getTailLengthSeconds()` 15.0 | High |
| `Source/PluginProcessor.cpp` | `prepareToPlay` sizing, chunked `processBlock`, slot ring-out replacing `switchChainTo`, DECAY/SIZE mapping, mono fold, NaN recovery, re-voiced 48-preset table, new wet trims | High |
| `Source/ModulationFx.h` | Remove `FlutterDelay` (no caller); keep `OctaveUpShifter` | Low |
| `tests/render-check/main.cpp` | Re-anchor and add gates (table below) | High |
| `CMakeLists.txt` | Version 2.0.0; a CPU/baseline console target if kept separate | Low |
| `CHANGELOG.md`, `NOTES.md`, `PLUGINS.md` row | v2.0.0 entry (`## [2.0.0]` heading form), architecture section rewritten | Low |
| `Source/ui/public/js/i18n.js` | Only if Open Question 2 is answered yes: three tooltip bodies x three languages | Medium |

Not touched: `PluginEditor.*`, `index.html`, parameter layout, state code, preset-manager module.

### New Files

| File | Purpose |
|------|---------|
| `Source/dsp/ReverbPrimitives.h` | Power-of-two ring delay with integer and 3rd-order Lagrange reads, Schroeder allpass, one-pole, RT60-to-gain helpers |
| `Source/dsp/FdnEngine.h` | 16-line Hadamard FDN, per-type delay set, two-band absorption, modulation, input diffusion |
| `Source/dsp/EarlyReflections.h` | Stereo output tap line |
| `Source/dsp/PlateEngine.h` | Dattorro tank with in-loop shimmer |
| `Source/dsp/SpringEngine.h` | Three dispersive springs |
| `.planning/improvements/reverb-engine-rewrite/BASELINE.md` | v1.14.0 measurements (requirement 10) |
| `tests/fixtures/state-v1.14.0.bin` | A v1.14.0 state blob, captured before any change |

Header-only on purpose: `ouaricon_add_processor_console` builds the processor TUs, so headers reach
render-check with no CMake source-list change.

### Dependencies

**JUCE Modules Used:**
- `juce_dsp` — `StateVariableTPTFilter`, `IIR::Filter` / `ArrayCoefficients`, `ProcessorDuplicator`
  (all existing). `dsp::Reverb` and `dsp::DelayLine` are no longer used by this plugin.
- `juce_audio_processors` — unchanged.

**External Dependencies:** none.

---

## Pattern Analysis

### Existing Patterns in Codebase

**Pattern 1: header-only DSP unit measured directly by the gate**
- Location: `Source/ModulationFx.h`, `tests/render-check/main.cpp:199-225, 392-397`
- Relevance: the model for the engine headers and their unit gates.

**Pattern 2: input-side duck, trim at the reverb input**
- Location: `PluginProcessor.cpp:569-582, 637-645`
- Relevance: becomes the slot's input gain; the reasoning in the comment (an output trim re-scales
  a ringing tail) still applies.

**Pattern 3: two EQ instances crossfaded**
- Location: `PluginProcessor.h:185-191`, `PluginProcessor.cpp:662-698`
- Relevance: becomes one EQ per slot; the crossfade is replaced by the slots' own fades.

**Pattern 4: HF-burst click metric**
- Location: `tests/render-check/main.cpp:101-133, 354-373`
- Relevance: the gate for SIZE and DECAY moves; add the cases beside the existing ones.

**Pattern 5: factory bank in engineering units, stale-file sweep, sentinel**
- Location: `PluginProcessor.cpp:839-941`
- Relevance: re-voicing edits the `bank[]` table only. Names stay, so the stale-file sweep is a no-op.

**Pattern 6: out-of-tree negative control**
- Location: project memory (seeded out-of-tree pair via `git archive`)
- Relevance: build the new gates against `git archive 02bac73f` (v1.14.0) for success criterion 10.
  A checkout in the working tree would wipe uncommitted engine work.

### Project patterns confirmed or sharpened by the prototype

- *Diffusion slows decay.* Confirmed twice: the plate needs the allpass lengths in its loop time
  plus a 5.5 % correction, and input diffusers put a floor under short tails.
- *Interpolation in a loop is a filter.* Linear costs 15-23 % of HF decay time.
- *Deterministic modulation phases* — designed in.
- *Rate dependence* — FDN, plate and spring RT60 agree within 0.5 % across 44.1/48/96 kHz.

### Similar Implementations

**In other plugins:**
- O-Strata / O-Prism `Source/dsp/ReverbProcessor.{h,cpp}`: a Dattorro plate (figure-8, modulated,
  multi-tap output). O-Bells, O-Lyrica, O-Wind, O-Formant carry variants. None calibrates decay in
  seconds. If the new engines prove out, they are the natural content of the follow-up shared
  module that CONTEXT lists as out of scope.
- O-Bells `tests/render-harness/main.cpp`: a `malloc_logger` allocation gate, the model for
  success criterion 9.

---

## Gate map for `tests/render-check`

| # today | Fate | Note |
|---|---|---|
| 1 preset TYPE recall | keep | |
| 2 CHARACTER second-difference | keep | ratio-based, no anchor |
| 3 Ambient @ 192 kHz | extend | all six types at 44.1/96/192 kHz, plus a host block larger than prepared |
| 4 VU peak hold | keep | |
| 5 HF bursts (LOW CUT, CHARACTER, TYPE) | keep, add | SIZE 0 <-> 100 and DECAY 0.5 <-> 2.0 against the WET reference |
| 6 type loudness spread <= 1 dB | keep | re-trim `wetTrimDb`; add a mono run |
| 7 flutter = ideal swept delay | remove | `FlutterDelay` is gone |
| 7 shifter is an octave | keep | unit test, unchanged |
| 7 Plate 600/300 Hz > Room + 20 dB | replace | bloom: octave ratio in the tail exceeds the ratio at onset by >= 10 dB, and Room shows none |
| 8 Ambient tail grows with DECAY | replace | superseded by the RT60 table |
| 9 bank shape and +5 dB ceiling | keep | re-voiced bank |

| New gate | v1.14.0 expected to fail? |
|---|---|
| RT60 table: 6 types x DECAY 0.5/1.0/2.0, +/-10 % | yes |
| RT60 holds across SIZE 0 / 100, +/-10 % | yes (SIZE is feedback there) |
| SIZE moves the structure: first-reflection time or mixing time changes with SIZE | yes |
| FDN types do not share resonances: tail-spectrum correlation < 0.3 for each pair | yes (expected about 0.9) |
| Early taps present in the output IR, different in L and R | yes |
| Late-tail L/R correlation small (`|r| < 0.3`), mono input | to be seen at baseline |
| Spring: echo arrives later at 3-4 kHz than at 1 kHz, repeating at the echo time | yes |
| Plate: no runaway — maximum DECAY, both SIZE extremes, 10 s of noise, then monotone decay | n/a (new structure) |
| No allocation in `processBlock`, including an oversized block | to be seen |
| NaN injected into a slot recovers within one block | n/a |
| `params.tsv` byte-identical; v1.14.0 state blob loads with equal values | passes both |

---

## Complexity Assessment

### Overall Complexity: High

**Justification:**
- Files affected: 4 modified, 5 new source headers, plus changelog/notes/registry
- DSP changes: Significant (the whole wet path up to the type EQ is new)
- UI changes: None (tooltip text only, if Open Question 2 is yes)
- Parameter changes: None
- Breaking changes: sound only — IDs, ranges, state format unchanged

Expect four execute sessions, one per stage in the brief, each ending on a green render-check.

### Risk Areas

1. **Plate echo density**
   - Risk: the acceptance criterion says "fast echo-density build-up"; Dattorro's tank reaches 0.84
     normalised density at 100 ms where the FDN reaches 0.99.
   - Mitigation: SIZE range x0.40-0.80; judge at the listening pass. If it is not dense enough,
     the fallback is more input diffusion stages, measured the same way.

2. **Level against DECAY and SIZE**
   - Risk: reverberant energy grows with RT60, so the same WET is about 3 dB louder at 2.0x. With
     tails up to 14 s, insert presets may breach the +5 dB ceiling.
   - Mitigation: measure during re-voicing; if needed scale the slot input by a partial
     `1/sqrt(T60)` law. Section 9 of render-check already catches a breach.

3. **Shimmer level is not settled by measurement**
   - Risk: too low is inaudible, too high reads as a shimmer effect.
   - Mitigation: one constant, derived per second; set at the listening pass; the bloom gate bounds
     it from both sides.

4. **Ring-out slot logic**
   - Risk: more state than today's duck (two slots, retire threshold, steal).
   - Mitigation: keep the existing TYPE HF-burst gate, add rapid A-B-A-B switching and a preset
     sweep through all 48 as cases.

5. **CPU**
   - Not a pass/fail item. Prototype cost per engine at 48 kHz, one core, double precision:
     FDN-16 0.8 %, three springs 1.0 %, plate 0.2 %; about double at 96 kHz and during a ring-out.
     The brief's "same class as Freeverb" is plausible but v1.14.0 has not been measured yet.

6. **Tooltips describe the old DSP** — see Open Question 2.

7. **Stale `.planning/STATUS.md`**
   - Risk: it still describes Stage 1 from January; only `activeMilestone` is current.
   - Mitigation: leave it; the milestone state lives in `STATUS.yaml`.

---

## Domain Detection

Keyword scoring per the skill, over CONTEXT.md + RESEARCH.md:

| Domain | Score | Keywords Found |
|--------|-------|----------------|
| DSP | 8 | processblock, filter, gain, dsp, buffer, sample, frequency, latency |
| GUI | 5 | ui, html, editor, layout, component |
| Polish | 4 | preset, cpu, pluginval, factory preset |

**Detected Domain:** mixed by the scoring rule (DSP and GUI both non-zero), DSP by content.
**Recommended Execute Agent:** `general-purpose`, briefed with the DSP real-time rules. The rule
and the practical need agree: `dsp-agent` has no shell, and every stage of this milestone ends on a
build and a render-check run.

---

## Version Impact

### Recommended Version Bump: MAJOR (1.14.0 -> 2.0.0)

**Justification:**
- [ ] Bug fix only (PATCH)
- [ ] New feature, backward compatible (MINOR)
- [x] Breaking changes (MAJOR) — every session sounds different

**Breaking Change Analysis:**
- Parameter IDs: Unchanged
- Parameter ranges: Unchanged
- Preset format: Unchanged
- Public API: Unchanged
- Audio output: changed for every type and preset

---

## Open Questions

1. **16 FDN lines instead of 8?** Recommended: yes. Eight lines miss the mode-density rule on Hall
   and Ambient; sixteen meet it at base decay and cost about 0.4 % of a core more. The brief and
   CONTEXT both say 8, so this needs a yes.
2. **Update the three tooltips?** `tip.type`, `tip.decay` and `tip.size` describe the Freeverb
   behaviour ("growing the room and easing its damping together", "from half its size at 0 % to
   its full size at 100 %", "not a time in seconds") in en, fr and zh-Hans. After the rewrite they
   are wrong. Recommended: rewrite the three bodies in all three languages — text only, no layout
   change — and accept that this touches the i18n lint and the zh-Hans review. CONTEXT lists UI
   changes as out of scope, so this also needs a yes.
3. **Ring-out on TYPE change** (two slots) rather than cutting the tail. Recommended and assumed
   unless you object; it is the larger of the two host designs.

Decisions left to the listening pass, not blocking the plan: shimmer level, per-type HF ratio and
crossover, spring M and fC, FDN delay ranges per type, early-reflection level.

---

## References

- Dattorro, "Effect Design Part 1: Reverberator and Other Filters", JAES 1997 —
  https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf (Fig. 1, Tables 1-2 confirmed)
- J. O. Smith, *Physical Audio Signal Processing*: delay-line damping filter design, first-order
  delay filter design, Householder and Hadamard feedback matrices, mode density requirement —
  https://ccrma.stanford.edu/~jos/pasp/
- Välimäki, Parker, Savioja, Smith, Abel, "Fifty Years of Artificial Reverberation" (spring fC
  range, modulation, zita-rev1's Hadamard 8)
- Parker, "Efficient Dispersion Generation Structures for Spring Reverb Emulation", EURASIP JASP
  2011 (M = 100 / 200, dropping the HF path)
- Gamper, Parker, Välimäki, DAFx-11 paper 39 (a = 0.62, Td 56 ms, feedback -0.8, fC 4.3 kHz)
- Schlecht, PhD thesis (FIR interpolation in the loop makes RT control inaccurate; modulation rates)
- Accutronics specifications via Amplified Parts (33 / 37 / 41 ms; decay classes)
- ISO 3382-1 conventions and JND via Hak et al., Acta Acustica 2012

**Not verified:** the Välimäki 2010 JAES paper and Abel 2006 AES paper were not reachable in full;
`K = fs / (2 fC)` comes from a student port. Which two of the three Accutronics delays a Type 4
tank uses, and a manufacturer bandwidth figure, were not found. The Jot pole formula was read
through a summariser.

---

## Next Phase

This research document feeds into the **Plan** phase, which will:
- Create atomic task breakdown (baseline first, then the brief's four stages)
- Define dependencies between tasks
- Set verification criteria per task from the gate map above
- Determine execution order

---

*Generated by improve-milestone research phase*
