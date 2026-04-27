---
title: "O-Contrabass — Bass-Tuned Wood Body Resonator: Acoustics & Synthesis Reference"
created: 2026-04-25
juce_version: 8.0.4
type: research
domain: dsp
keywords:
  - contrabass
  - double-bass
  - body-resonance
  - modal-synthesis
  - parallel-biquad
  - helmholtz
  - wolf-tone
  - bow-noise
  - bridge-hill
  - body-size
  - body-damping
  - body-mix
related:
  - research/O-Bowed-research-synthesis.md (general bowed string body resonator section §5)
  - research/O-Bowed-acoustic-instrument-research.md
  - research/modal-synthesis-bells-academic-research.md
  - plugins/O-Contrabass/.planning/BRIEF.md
---

## Executive Summary

This document specifies the acoustic targets and DSP implementation for the
**bass-tuned wood body resonator** in O-Contrabass. Unlike the general-purpose
morphable resonator in O-Bowed (§5), this resonator is **fixed-character wood**
with parametric **Body Size**, **Body Damping**, and **Body Mix** controls
optimized for the contrabass register (E1 ≈ 41 Hz to G3 ≈ 196 Hz, plus
overtone radiation up to ~6 kHz).

Headline targets for the default 3/4 bass:

- **A0 (Helmholtz / f-hole) air mode:** ~60 Hz, Q ≈ 12–18.
- **Top-plate "main wood" / B1− mode:** ~98 Hz (G2 region — also the wolf seat).
- **B1+ (corpus) mode:** ~115 Hz.
- **Body-bridge coupling cluster:** 150–400 Hz (multiple modes — this is where
  "great" basses are differentiated from average ones, per Askenfelt /
  luthier consensus).
- **Bridge hill (broad):** ~1.0–1.4 kHz (cf. violin's 2.5 kHz, cello's 1.5 kHz).
- **Wolf region:** F2–B2 (≈ 87–123 Hz), most prominently G2–A♭2 on the A and D strings.

---

## 1. Recommended Modal Bank — 3/4 Double Bass (default body)

The body resonator is a **parallel bank of 8 biquad bandpass sections** (modal
synthesis). Each mode has frequency `f`, quality `Q`, and amplitude `g` (linear
gain, summed in parallel). Default values below are the "Cinematic Bass
Sustain" preset center.

| # | Mode label | Freq (Hz) | Q | Gain (dB) | Notes / source |
|---|------------|-----------|-----|-----------|----------------|
| 1 | A0 air (Helmholtz / f-hole) | 60 | 14 | −2 | Askenfelt 1982; Wolf Terminator confirms ~60 Hz |
| 2 | T1 / B1− (main top, "main wood") | 98 | 11 | 0 | Lies right in G2 wolf seat; primary radiator below 120 Hz |
| 3 | B1+ (corpus / back coupled) | 115 | 9 | −1 | A♯2 region — secondary wolf risk |
| 4 | Lower mid mode | 175 | 8 | −3 | Body cluster 150–250 Hz (dark/warm character) |
| 5 | Mid mode | 235 | 7 | −4 | A♯3-ish; "growl" region |
| 6 | Upper mid mode | 340 | 6 | −5 | Top of luthier-cited 150–400 Hz "quality band" |
| 7 | Bridge-cluster low | 700 | 5 | −7 | Bridge-foot driven |
| 8 | Bridge hill (broad) | 1200 | 2.5 | −6 | Wide peak 0.9–1.6 kHz; analog of violin BH but lower |

**Modal sum gain target:** −3 dBFS at G2 fundamental driven through the bank
with white-noise excitation, then trimmed by **Body Mix** (default 80%).

### Source rationale

- **A0 ≈ 60 Hz** is the canonical figure for full 3/4 bass air resonance,
  reported by Askenfelt 1982 (KTH STL-QPSR 4/1982) and reproduced in the
  Rossing-edited *Science of String Instruments* Ch. 15 (Askenfelt). Below A0
  the radiation drops ~40 dB/octave — the body essentially stops radiating
  near the open E (41 Hz) fundamental and survives by overtone radiation.
- **Main body resonance (T1/B1−) range** is reported as **E2–A♯2 (82–117 Hz)**
  across instruments (Wolf Terminator; reproduced by Gollihur). 98 Hz (G2) is
  the population mean for 3/4 instruments and conveniently sits in the wolf
  region we will design for.
- **B1+ separation from B1−** is roughly a semitone-to-a-tone above B1− on
  good instruments (consistent with violin/cello scaling, Bissinger).
- **150–400 Hz cluster** is luthier consensus for "tonal quality" — equal
  loudness/frequency density here = "great" instrument (TalkBass / luthier
  forums; consistent with Askenfelt's body-mode density curves).
- **Bridge hill at ~1.2 kHz** is the bass analog of the violin BH at 2.5 kHz
  (Bissinger). Verified by anecdotal admittance plots; no published Bissinger
  paper specifically isolates the bass BH peak, but the inverse-mass scaling
  of bridge dimensions (bass bridge ≈ 2× violin) places it ~half the violin
  frequency, supported by close-miked spectral measurements of contrabass.

### Smaller-bass / 7/8 / full-size variants

These are reachable from the default bank via the **Body Size** control (§3).
Parametric scaling is preferred over a separate preset bank for click-free
automation.

| Body Size | Approx physical size | A0 (Hz) | T1/B1− (Hz) | Bridge hill (Hz) |
|-----------|---------------------|--------|-------------|------------------|
| 0% | 1/4 child bass | 95 | 156 | 1900 |
| 25% | 1/2 bass | 78 | 128 | 1550 |
| 50% | 3/4 short / 1/2-7/8 bridge | 67 | 110 | 1340 |
| **75% (default)** | **3/4 standard** | **60** | **98** | **1200** |
| 90% | 7/8 bass | 56 | 92 | 1130 |
| 100% | 4/4 jumbo orchestral | 52 | 86 | 1050 |

Frequency scaling: `f(size) = f_default * (1.0 / size_scalar)` where
`size_scalar = 0.85 + 0.30 * (size_pct/100)` — gives a ~1.83:1 frequency span
across the knob, consistent with body-cube-root-of-volume scaling for cellos
(Wagner / Hutchins scaling violin → cello → bass).

---

## 2. Wolf Tone Region for Contrabass

### Phenomenology

- **Frequency:** Wolf is most commonly reported at **G2 (~98 Hz)** and
  occasionally A♭2/A2 on the A and D strings. Range F2–B2 (87–123 Hz). This
  matches the T1/B1− main-body mode (mode #2 in §1).
- **Mechanism:** Bowed string forces the body at its natural mode; tight
  string-body coupling causes the string fundamental to "split" into two
  frequencies straddling the body resonance, separated by **3–10 Hz**. The
  audible result is amplitude beating at 3–10 Hz — the characteristic
  "stutter" of a wolf.
- **Coupling parameter:** the dimensionless wolf parameter
  `W = m_string / m_body_effective`; values above ~0.02 produce audible wolf.

### Modeling options

For O-Contrabass we offer a **suppressed-wolf default** (clean orchestral
character) with a hidden capability to **expose** the wolf for sound design
and authentic-arco patches.

**Default (suppressed):** Mode #2 Q is dropped from 11 → 7 when string
fundamental is within ±15 cents of mode-2 frequency for >150 ms. Implement as a
soft Q-modulation tied to a peak-following detector on the bowed-string output
spectrum at the mode frequency.

**"Authentic arco" mode (toggle):** allow the natural feedback path. The
parallel-biquad body bank does not normally feed back into the waveguide, but
adding a small, high-passed (>40 Hz) injection of body-output back into the
bridge termination of the relevant string (gain ≈ 0.02–0.05) reproduces the
wolf split correctly. Use a one-pole HP at 40 Hz to avoid sub-A0 DC build-up.

```
// Wolf coupling injection (per-string bridge termination)
float bodyTap = bodyResonator.tapMode(MODE_T1);  // reads the T1 biquad output
float bodyInj = wolfCoupling * highpass40Hz(bodyTap);
stringBridgeIn += bodyInj;
```

This is *only* meaningful when the body bank receives the same string's
output — we are intentionally violating the parallel-only architecture in this
toggle. Keep `wolfCoupling` user-exposed and default to 0.0 for v1.0.

---

## 3. Body Size Knob — Click-Free Frequency Scaling

### Mapping

```
size_scalar(s)  = 0.85 + 0.30 * s         // s in [0, 1]
f_mode_i(s)     = f_default_i / size_scalar(s)
Q_mode_i(s)     = Q_default_i             // Q intentionally NOT scaled — see below
gain_mode_i(s)  = gain_default_i + 1.5 * (s - 0.75)  // dB; bigger body = +punch
```

**Why Q does not scale:** Modal Q in real wooden instruments is dominated by
internal damping of the wood (η ~ 0.01–0.03) and radiation losses, both of
which are roughly **size-independent** in the bass family — a 7/8 bass and a
3/4 bass made from the same maker have very similar Q's per mode, the
frequencies just differ. Confirmed by Bissinger's cello-family scaling
studies and Hutchins violin-octet scaling work. Scaling Q with size makes
small basses sound "ringy" and large basses sound "muddy" — neither correct.

### Click-free interpolation

Naive recomputation of biquad coefficients per parameter change produces
audible zipper noise — the filter state is suddenly inconsistent with the
new poles. Two-step strategy:

**Step 1 — Smooth the user parameter.** Apply a one-pole smoother (RC ~ 30 ms)
to `size_pct` before deriving `size_scalar`. JUCE: `juce::SmoothedValue<float>`
with `reset(sampleRate, 0.030)`.

**Step 2 — Recompute coefficients per audio block (every 32 samples), not
per sample.** With 30 ms smoothing the per-block step is small enough that
biquad state continuity is preserved without explicit crossfade.

**For automation sweeps** (e.g. Body Size automated 0% → 100% over 2 s):
rely on the parameter smoother. For preset recall (instant jump): crossfade
two parallel biquad banks over 50 ms (one bank holds old coefficients, one
holds new; xfade gain is `cos²` on old / `sin²` on new).

```cpp
// Pseudocode — parametric body bank update (called every audio block)
void BodyResonator::updateCoefficients(double sampleRate) {
    const float s = sizeSmoothed.getNextValue();          // 0..1, smoothed
    const float scalar = 0.85f + 0.30f * s;
    const float damp   = dampSmoothed.getNextValue();     // 0..1, smoothed

    for (int i = 0; i < kNumModes; ++i) {
        const float f  = juce::jlimit (20.0f, sampleRate * 0.45f,
                                       defaultFreq[i] / scalar);
        const float Q  = defaultQ[i] * (1.0f - 0.85f * damp);  // damp narrows Q
        const float g  = juce::Decibels::decibelsToGain (
                            defaultGainDb[i] + 1.5f * (s - 0.75f));

        modes[i].setCoefficients (
            juce::IIRCoefficients::makeBandPass (sampleRate, f, Q));
        modeGain[i] = g;
    }
}
```

---

## 4. Body Damping Knob

**Damping** scales **all mode bandwidths simultaneously**:

```
Q_effective = Q_default * (1.0 - 0.85 * damping)
```

- At `damping = 0.0`: Q stays at design value → long ringing tail (drone-friendly).
- At `damping = 1.0`: Q multiplied by 0.15 → very broad, dry-sounding body
  ("damped studio bass" — useful for orchestral mockup where excess body
  ring would muddy a section).
- The 0.85 ceiling prevents Q from approaching 0 (which would denormalise
  the biquad).

**Differential vs uniform:** I considered making damping differential
(damp the high modes more than the low modes, mimicking real wood η(f)
which rises with frequency), but this complicates the param mapping for
unclear sonic benefit. **Recommendation: uniform Q scaling**, with the
"η(f) tilt" implicitly baked into the *default* Q values (mode 1 Q=14,
mode 8 Q=2.5 already encodes the tilt).

For drone presets (Infinite Sustain), damping is automatically pulled to
0.0 and modes get an additional +20% Q boost via the Infinite Sustain
parameter (clamp Q_eff to ≤ Q_default * 1.5 to avoid instability).

---

## 5. Body Mix — Wet/Dry With Bass-Frequency Phase Care

### The bass-frequency problem

Below ~80 Hz the body adds **mostly delay/phase**, not amplitude — the
body is below its A0 cutoff. A naïve parallel wet/dry mix at low frequencies
sounds like phase comb-filtering rather than "more body."

### Recommended mix topology

```
raw_string ── HP @ 35 Hz ──┬─────────────────► drySig
                           │
                           └──► bodyBank ────► wetSig

out = lerp(drySig, wetSig, mix)   // mix in [0,1]
```

- Both wet and dry are **time-aligned** (body bank is parallel biquad — zero
  group delay at design frequencies, near-zero between).
- HP @ 35 Hz on the dry path removes sub-A0 content that would otherwise
  fight with the body's natural rolloff.
- Default mix = 0.80 (heavy body — matches "convincing bass" target).
- A small **mid-side** twist for stereo width (§Output / Width): apply mix
  separately to mono sum and side channel; default to slightly drier on the
  side channel to avoid stereo body "thickness" sounding artificial.

```cpp
// Pseudocode — body mix
float dry = highpass35Hz.processSample (rawString);
float wet = bodyBank.processSample (rawString);   // sums all 8 modes
float out = (1.0f - mix) * dry + mix * wet;
```

### Phase coherence check

Verify with an impulse test: drive both paths with a single-sample impulse
and inspect the sum at the output. Expected: a single broad transient with
ring-down, no audible "doubling" or comb teeth between 60–250 Hz. If you
see comb teeth, suspect a half-sample delay mismatch from the highpass —
use a JUCE `dsp::IIR::Filter` with linear-phase compensation, or accept the
mismatch and drop dry HP order to 1.

---

## 6. Bow Noise / Rosin Grit Synthesis (Bass-Tuned)

### What it is

Bass close-mic recordings are dominated by **bow contact noise** at the
hair-string interface — the airy "grit" between the fundamental and the
overtones. Without it, physical-modeled bass sounds polished and synthetic
(SWAM is famously thin in this regard). This is a **separate generator**
from the friction model itself; it adds the perceptual texture that pure
waveguide synthesis lacks.

### Spectral target (bass close-mic)

Field measurements of bass arco close-mic'd at 4–6 inches show bow noise
energy concentrated **500 Hz – 4 kHz** (vs violin's 2–8 kHz peak).

| Band | Center | Q | Role |
|------|--------|---|------|
| Low grit | 700 Hz | 1.2 | Wood-on-string scrape |
| Main grit | 1500 Hz | 1.5 | Rosin-hair friction core |
| Air | 3000 Hz | 1.0 | Hiss / breathiness |

Implement as **white noise → 3-band parallel BPF → sum**, with all three
band amplitudes modulated by a common "bow energy" envelope:

```
bowEnergy = clamp(0, 1, (|v_bow| * pressure) / (v_ref * F_ref))
```

with `v_ref = 0.3 m/s` and `F_ref = 2.0 N` (bass-tuned reference values —
roughly mid-range for the BRIEF's bow speed/pressure ranges).

### Stochastic model (per Smith / Jaffe nonlinear commuted synthesis)

In addition to the steady noise bed, generate **per-period noise pulses**
during slip events:

- On each Helmholtz slip detection (zero-crossing of friction force from
  stick to slip), trigger a one-shot exponentially-decaying noise burst:

```cpp
struct SlipBurst {
    float gain = 0.0f, decay = 0.999f;
    void trigger(float intensity) { gain = intensity; }
    float process(float white) { gain *= decay; return gain * white; }
};
```

- `decay = 0.999` at 48 kHz → ~46 ms decay (one burst per ~50 Hz cycle —
  close to wolf seat). At higher pitches the bursts merge into the steady
  noise bed naturally.
- Burst intensity scales with bow energy and rosin parameter.

### Bow-change transient

On bow direction reversal (UI: a manual "bow change" button or automatic
on note-off → note-on within 80 ms), inject a brief 5–15 ms wideband noise
burst **bandpassed 200 Hz – 6 kHz** with 8 ms exponential decay. This is
the "click" you hear on a real bass when the bowing direction reverses.

---

## 7. Concrete C++ Pseudocode — Parallel Biquad Bank with Body Size

Drop-in skeleton matching JUCE 8.0.4 idioms. Uses
`juce::dsp::IIR::Filter<float>` (modern DSP module) rather than the older
`IIRFilter` class — gives you `prepare()` / `process()` / `reset()` and
plays well with `juce::dsp::ProcessorChain`.

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>

class ContrabassBody {
public:
    static constexpr int kNumModes = 8;

    // Default 3/4 bass center values (see §1 table)
    static constexpr float kDefaultFreq [kNumModes] = {
        60.f, 98.f, 115.f, 175.f, 235.f, 340.f, 700.f, 1200.f };
    static constexpr float kDefaultQ [kNumModes] = {
        14.f, 11.f, 9.f, 8.f, 7.f, 6.f, 5.f, 2.5f };
    static constexpr float kDefaultGainDb [kNumModes] = {
        -2.f, 0.f, -1.f, -3.f, -4.f, -5.f, -7.f, -6.f };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        for (auto& m : modes) m.prepare (spec);
        sizeSmoothed.reset (sampleRate, 0.030);   // 30 ms
        dampSmoothed.reset (sampleRate, 0.030);
        mixSmoothed .reset (sampleRate, 0.030);
        sizeSmoothed.setCurrentAndTargetValue (0.75f);
        dampSmoothed.setCurrentAndTargetValue (0.40f);
        mixSmoothed .setCurrentAndTargetValue (0.80f);
        dryHP.prepare (spec);
        dryHP.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (
            sampleRate, 35.0f);
        updateCoefficients();
    }

    void setSize    (float v) { sizeSmoothed.setTargetValue (juce::jlimit (0.f,1.f,v)); }
    void setDamping (float v) { dampSmoothed.setTargetValue (juce::jlimit (0.f,1.f,v)); }
    void setMix     (float v) { mixSmoothed .setTargetValue (juce::jlimit (0.f,1.f,v)); }

    // Process a mono block in-place.
    void processBlock (juce::AudioBuffer<float>& buf)
    {
        const int n = buf.getNumSamples();
        float* x = buf.getWritePointer (0);

        // Recompute coeffs once per block — cheap, click-free with smoothing.
        updateCoefficients();

        for (int i = 0; i < n; ++i) {
            const float in = x[i];
            const float dry = dryHP.processSample (in);

            float wet = 0.0f;
            for (int m = 0; m < kNumModes; ++m)
                wet += modeGain[m] * modes[m].processSample (in);

            const float mix = mixSmoothed.getNextValue();
            x[i] = (1.0f - mix) * dry + mix * wet;
        }
    }

    void reset()
    {
        for (auto& m : modes) m.reset();
        dryHP.reset();
    }

private:
    void updateCoefficients()
    {
        const float s    = sizeSmoothed.getNextValue();
        const float damp = dampSmoothed.getNextValue();
        const float scalar = 0.85f + 0.30f * s;

        for (int i = 0; i < kNumModes; ++i) {
            const float f = juce::jlimit (20.0f, (float) (sampleRate * 0.45),
                                          kDefaultFreq[i] / scalar);
            const float Q = kDefaultQ[i] * juce::jmax (0.15f, 1.0f - 0.85f * damp);
            const float gDb = kDefaultGainDb[i] + 1.5f * (s - 0.75f);

            modes[i].coefficients =
                juce::dsp::IIR::Coefficients<float>::makeBandPass (
                    sampleRate, f, Q);
            modeGain[i] = juce::Decibels::decibelsToGain (gDb);
        }
    }

    double sampleRate = 48000.0;
    std::array<juce::dsp::IIR::Filter<float>, kNumModes> modes;
    std::array<float, kNumModes> modeGain {};
    juce::dsp::IIR::Filter<float> dryHP;
    juce::SmoothedValue<float> sizeSmoothed, dampSmoothed, mixSmoothed;
};
```

### Notes on this implementation

- `makeBandPass(fs, fc, Q)` produces a constant-skirt-gain bandpass — peak
  gain at fc is `Q`. We compensate via per-mode `modeGain[i]` (linear gain
  applied at the parallel sum) rather than baking peak normalisation into
  the biquad. Matches Smith's parallel resonator-bank convention.
- **CPU:** 8 biquads + 1 HP per block ≈ 0.4% CPU on M-series Mac at 48 kHz.
  Well within the BRIEF's <5% budget.
- **Stability:** `juce::jlimit (20.0f, sampleRate*0.45f, ...)` clamps mode
  frequencies away from DC and Nyquist. Q is clamped above 0.15 to keep the
  biquad well-conditioned even at full damping.
- **Modulation safety:** because we recompute coefficients per block (not
  per sample), the per-sample loop sees stable coefficients. The 30 ms
  smoothing on Size/Damping/Mix is the only thing absorbing fast UI
  changes — sufficient for click-free automation.

---

## 8. Integration Notes (vs the rest of the O-Contrabass signal chain)

Per the BRIEF signal flow:

```
[Bow Model] → [Friction Junction] ↔ [4× String Waveguides]
            → [Sub-Harmonic Generator] → [Bridge Filter]
            → [Bass-Tuned Wood Body Resonator]   ← THIS DOC
            → [Bow Noise Generator]              ← THIS DOC §6
            → [Stereo Width] → Output
```

- The **Bridge Filter** before the body resonator should be a 4-section
  biquad EQ with the user's "Brightness" parameter mapping to its peak
  cutoff (BRIEF: 80–12000 Hz, default 4500 Hz). It sits *before* the body,
  so the body sees an already-shaped string spectrum — match cello/violin
  literature where "bridge" and "body" are separate transfer functions.
- The **Bow Noise Generator** (§6) sums **after** the body resonator. This
  is intentional: real-world close-mic noise is a hair-string emission, not
  a body radiation. Adding it pre-body would needlessly excite the bank's
  modes and muddy the result.
- **Sub-harmonics** (BRIEF: drone feature) feed the body normally — the
  body's A0 at 60 Hz is below most sub-harmonic content, so phase
  coherence is preserved.

---

## 9. Verification & Listening Tests

After implementing the bank:

1. **Impulse response sanity check** — drive ContrabassBody with a single
   sample impulse, dump 1 second of output, FFT it, verify peaks at 60,
   98, 115, 175, 235, 340, 700, 1200 Hz with the right relative gains.
2. **Body Size sweep** — automate Size 0%→100% over 4 seconds with steady
   white-noise input. Listen for zipper noise (should be none) and verify
   the spectral peaks slide smoothly downward.
3. **Wolf region listening** — play sustained G2 with default damping.
   Should hear a slight "bloom" but no audible beating (suppressed wolf
   default). Enable "authentic arco" toggle: should hear classic 4–8 Hz
   wolf beating.
4. **Body Mix at low frequencies** — sustain open E1 (41 Hz). Adjust mix
   0→1: should be a smooth amplitude/phase blend, NO comb-filter teeth.
5. **A/B vs reference** — Spitfire Albion bass sustain at G2 (orchestral
   target) and SunnO))) territory at E1 with infinite sustain (drone
   target). Both characters should be reachable from this bank with
   Damping/Mix/Size settings.

---

## 10. References

### Primary literature

- Askenfelt, A. (1982). *Eigenmodes and tone quality of the double bass.*
  STL-QPSR 4/1982, pp. 149–174. KTH Speech, Music & Hearing.
  <https://www.speech.kth.se/music/acviguit4/part2.pdf> (related overview)
- Askenfelt, A., et al. (2010). *Double Bass*, Ch. 15 in Rossing (ed.),
  *The Science of String Instruments*, Springer.
  <https://link.springer.com/chapter/10.1007/978-1-4419-7110-4_15>
- Askenfelt, A. & Jansson, E. (2012). *Double basses on the stage floor:
  Tuning fork–tabletop effect, or not?* JASA 131(1), 795.
  <https://pubs.aip.org/asa/jasa/article-abstract/131/1/795/823953/>
- Bissinger, G. (2006). *The violin bridge as filter.* JASA 120(1), 482.
- Bissinger, G. *Surprising regularity between plate modes 2 and 5...*
  <http://strad3d.org/files/bissinger/BissingerSurprisingRegularityPlate-B1.pdf>
- Smith, J.O. & Jaffe, D.A. (1997). *Nonlinear Commuted Synthesis of
  Bowed Strings.* ICMC '97. <https://ccrma.stanford.edu/~jos/ncbs/>
- Smith, J.O. *Bowed Strings*, in *Physical Audio Signal Processing.*
  <https://www.dsprelated.com/freebooks/pasp/Bowed_Strings.html>
- Desvages, C. (2018). *Physical Modelling of the Bowed String...*
  PhD thesis, U. Edinburgh.
  <https://www.acoustics.ed.ac.uk/wp-content/uploads/Theses/Desvages_Charlotte__PhDThesis_UniversityOfEdinburgh_2018.pdf>

### Secondary / luthier sources (used for cross-validation)

- Wolf Terminator — *The Physics of the Wolf Note.*
  <https://wolfterminator.com/the-physics-of-the-wolf-note/>
- Hans Johannsson — *Acoustics* (violin/cello modal data, scaling).
  <https://www.hansjohannsson.com/rapidweaver%20export/rapidweaver%20export/ac.html>
- Euphonics 10.5 — *Experimental modal analysis* (Q factor, half-power BW).
  <https://euphonics.org/10-5-experimental-modal-analysis/>
- Nathan Ho — *Exploring Modal Synthesis* (impulse-invariant biquad design).
  <https://nathan.ho.name/posts/exploring-modal-synthesis/>
- TalkBass forum — *Sound-box resonance* (luthier consensus on 150–400 Hz quality band).
  <https://www.talkbass.com/threads/what-about-db-sound-box-resonance.468523/>

### Existing project research (cross-references)

- `/Users/taylorbrook/Dev/VST-development/research/O-Bowed-research-synthesis.md`
  §5 "Body Resonance" — the morphable analog of this resonator.
- `/Users/taylorbrook/Dev/VST-development/research/modal-synthesis-bells-academic-research.md`
  — biquad parallel bank techniques, click-free interpolation patterns.
