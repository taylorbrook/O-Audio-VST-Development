# Stage 2 (DSP) — Research

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 2 of 4 — DSP Implementation
**Phase:** research
**Date:** 2026-06-26
**Inputs:** `research/ARCHITECTURE.md` (immutable DSP contract), `stages/2-dsp/CONTEXT.md` (locked scope: 3 must-phases 2.1/2.2/2.3; Waveguide 2.4 → v1.1), `ROADMAP.md`, `REQUIREMENTS.md`, Stage-1 build (`Source/`).

This phase does **not** re-litigate DSP decisions — the architecture is locked. It resolves the **5 open research items** from CONTEXT.md by extracting **port-ready code** (verbatim, with `file:line`) from the in-house references and flagging where the references **diverge** from what the architecture mandates, so the plan + execute phases are turnkey.

---

## 0. Three findings that reshape the CONTEXT assumptions (read first)

Verbatim extraction across O-Lyrica, O-Bells, O-Bowed, O-Bassoon, O-simpleFM/Grain surfaced three corrections to the assumptions baked into CONTEXT.md's "open items." None break the architecture — they redirect *which* reference to port from.

| # | CONTEXT assumed | Reality (verified) | Consequence |
|---|-----------------|--------------------|-------------|
| **F1** | Port O-Lyrica's **Thiran** phase-delay compensation for KS tuning (open item #1). | **O-Lyrica uses Lagrange3rd, NOT Thiran**, and has **no Thiran phase-delay code and no DC blocker**. The mandated `juce::dsp::DelayLine<float, Thiran>` is used in production by **O-Bowed** (`WaveguideString.h:67-72`). | **O-Bowed is the Thiran reference.** Port O-Lyrica's `OnePoleLPF` + group-delay *formula* + position comb + shadow-crossfade (those ARE clean); take the Thiran delay-line usage (incl. "delay ≥ 2 samples" clamp) from O-Bowed. Author our own DC blocker. |
| **F2** | Port STK `BowTable` friction from O-Bowed `Source/DSP` (open item #2). | Memoryless friction lives in a **shared module** `modules/synthesis/bow-friction/` (v1.0.0), already extracted, registered in `modules/registry.yaml`, consumed by O-Bowed + O-Contrabass via `ouaricon_add_module`. It's a **hyperbolic Stribeck** curve, not the literal STK `pow()` table (equivalent role). | **Use `ouaricon_add_module(O-simplePhysicalModelSynth bow-friction)`** — don't copy-paste. Pure value-class C++, zero deps, `cpp_standard 20`. |
| **F3** | Reuse O-Bells modal bank + `DECAY_MULTIPLIERS` + T60 mapping (open item #3). | **O-Bells is triggered decaying SINUSOIDS (additive), not filters** — incompatible with exciter-driven cross-driving (FUNC-04). O-Bassoon's `ModeBank` is the closest (pole-only resonator) but had to add **`strike()` state-injection** because impulse-driven high-Q modes sit **~50 dB below target ("inaudible sustain")**. | **Build the modal bank new** as exciter-driven RBJ bandpass biquads. Reuse from O-Bells only the **data + formulas** (`DECAY_MULTIPLIERS`, amplitude tilt). **Derive T60→Q ourselves** (O-Bells stores an amplitude envelope, not a Q). Carry O-Bassoon's `strike()` injection as the validated **fallback** for the "too quiet" risk. |

---

## 1. Phase 2.1 — Core KS String + Tuning + Pluck

**Goal:** first audio — a tunable, decaying plucked string. Verifies FUNC-01, DSP-01, DSP-02, DSP-03 (partial), FUNC-02 (Pluck).

### 1.1 `OnePoleLPF` — loop damping filter (PORT VERBATIM from O-Lyrica)

`O-Lyrica/Source/DSP/WaveguideString.h:191-216` — bilinear-transform one-pole, trivially-copyable POD (the POD-ness is load-bearing for the shadow crossfade in §1.6):

```cpp
struct OnePoleLPF
{
    float b0 = 1.0f, b1 = 0.0f, a1 = 0.0f, state = 0.0f;

    inline float processSample(float x) noexcept
    {
        const float y = b0 * x + state;
        state = b1 * x - a1 * y;
        return y;
    }
    void reset() noexcept { state = 0.0f; }
    void setCutoff(float cutoffHz, double sampleRate) noexcept
    {
        const double n = std::tan(juce::MathConstants<double>::pi * (cutoffHz / sampleRate));
        const double a0 = n + 1.0, invA0 = 1.0 / a0;
        b0 = (float)(n * invA0);
        b1 = (float)(n * invA0);
        a1 = (float)((n - 1.0) * invA0);
    }
};
```

**Damping mapping (ARCHITECTURE §3):** `damping` 0–100% → loop-LPF cutoff, **log-mapped, MORE damping = darker**: `fc = 12000 * pow(1500.0/12000.0, damping/100)` → 12 kHz (bright, damping=0) … 1.5 kHz (dark, damping=100).

### 1.2 Thiran fractional-delay tuning (TOPOLOGY from O-Bowed; F1)

The KS loop is a **single** delay line (not O-Lyrica's dual rail). Use the architecture-mandated Thiran interpolation, validated in O-Bowed:

`O-Bowed/Source/DSP/WaveguideString.h:67-72`:
```cpp
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> bridgeDelay { 4410 };
```
`O-Bowed/Source/DSP/WaveguideString.cpp:20-32, 74-79` (sizing + the **Thiran ≥ 2-sample** clamp):
```cpp
int maxDelay = (int)(sampleRate / 20.0) + 100;        // lowest note ~20 Hz headroom
bridgeDelay.setMaximumDelayInSamples(maxDelay);
...
bridgeSamples = std::max(2.0f, bridgeSamples);         // Thiran needs >= 2 samples
bridgeDelay.setDelay(bridgeSamples);
```

**KS loop (architecture §Algorithm Details), per sample, per voice:**
```
out = delay.popSample(0);
s   = loopLPF.processSample(out);
delay.pushSample(0, g * s + eSustain);   // eSustain: pluck/strike burst→0; bow=continuous friction
```
`g` = feedback (Decay), clamped `[0.80, 0.999]` (< 1). Excitation written once at trigger (Pluck/Strike) by seeding the loop with the burst, OR injected continuously (Bow, §2.2).

### 1.3 Group-delay compensation — final delay length N (FORMULA from O-Lyrica; F1)

The loop-LPF adds delay that flattens pitch unless compensated. O-Lyrica's formulas port directly (the *math*, not the dual-rail wrapper):

- **First-order LPF group delay at DC** (`O-Lyrica/.../WaveguideString.cpp:589-594`): `τ_lpf = fs / (2π · fc)` samples (`fc` = current loop-LPF cutoff; clamp `fc ≥ 1`).
- **Final tuned delay:** `N = fs / f0 − τ_lpf`, then `delay.setDelay(max(2.0f, N))`. The **Thiran interpolator absorbs the fractional remainder of N** — do **not** also subtract a separate Thiran phase-delay term (JUCE's `setDelay(d)` makes the *total* delay = `d`, fraction included).
- `f0 = 440 · 2^((midiNote − 69)/12) · 2^((coarse + fine/100)/12)`.
- **Tuning↔Damping coupling (ARCHITECTURE Integration):** `τ_lpf` depends on `fc`, so **recompute N whenever `damping` (cutoff) changes**, not just at note-on.

> Group-delay note for any all-pass we add later (stiffness/Waveguide v1.1): first-order all-pass group delay at DC = `(1−a)/(1+a)` (`O-Lyrica/.../WaveguideString.cpp:615`). Not needed for v1.0 KS.

**Gate:** the autocorrelation harness (§5) asserts ±5 cents at C1/C3/C5/C7. **Fallback** (architecture): if Thiran drifts at the top octave, add a small per-note offline correction table measured by the harness.

### 1.4 Feedback from Decay (architecture lerp; O-Lyrica decay-time alternative)

**Architecture-mandated (use this):** `g = lerp(0.80, 0.999, decay/100)`, applied per-sample in the loop, hard-clamped `< 1`. Perceptually most of the musical range lives in `g ∈ [0.99, 0.999]` (per-sample `g` over an `N≈100` round-trip: `0.995^100≈0.61` = steel/long; `0.93^100≈7e-4` = nylon/short) — expect to tune the curve by ear in execute.

**Alternative if a decay-*time* feel is wanted** (`O-Lyrica/.../WaveguideString.h:325-342`): `g = 10^(−3 / (T·f0))` clamped `[0.9, 0.99999]` (per-cycle −60 dB derivation). Our Decay param is 0–100%, so the lerp is the contract; keep this in reserve only if the lerp reads unevenly.

### 1.5 Pluck exciter — filtered noise burst + brightness LPF + **position comb** (PORT from O-Lyrica)

Position comb `y[n] = x[n] − x[n−D]` with linear-interpolated fractional delay — `O-Lyrica/Source/DSP/PluckExciter.cpp:129-144`:
```cpp
combBuffer[combWriteIndex] = noiseSample;
float delayedSample = 0.0f;
if (combDelaySamples > 0.0f) {
    int   delayInt  = (int)combDelaySamples;
    float delayFrac = combDelaySamples - delayInt;
    int   r0 = (combWriteIndex - delayInt + MAX_COMB_DELAY) % MAX_COMB_DELAY;
    int   r1 = (r0 - 1 + MAX_COMB_DELAY) % MAX_COMB_DELAY;
    delayedSample = combBuffer[r0]*(1.0f-delayFrac) + combBuffer[r1]*delayFrac;
}
combWriteIndex = (combWriteIndex + 1) % MAX_COMB_DELAY;
float positionFiltered = noiseSample - delayedSample;
```
D computation — `PluckExciter.cpp:242-247`: `D = jlimit(0.05f, 0.95f, position) · (fs/f0)`, clamped to `[1, MAX_COMB_DELAY-2]`. `MAX_COMB_DELAY = 8192`.

Pluck recipe (architecture §7): `juce::Random` white noise × short ramp/`ADSR` (~5–15 ms burst) → brightness `OnePoleLPF` (cutoff from `excitationColor`, raised by velocity via `velToBrightness`) → position comb. Write the burst into the KS loop; `eSustain = 0` after it.

### 1.6 Shadow-filter crossfade — zipper-free coefficient changes (PORT VERBATIM from O-Lyrica)

When `damping`/material moves the loop-LPF cutoff, crossfade old→new over 64 samples. `O-Lyrica/Source/DSP/WaveguideString.h:229-233` + `.cpp:515-553` (snapshot) + the blend `.cpp:140-161`:
```cpp
static constexpr int FILTER_CROSSFADE_LENGTH = 64;
// on cutoff change (audio thread): snapshot live→shadow (POD copy = coeffs+state), install new coeffs, restart fade
loopDampingShadow = loopDamping;                 // POD copy
loopDamping.setCutoff(newCutoff, sampleRate);
filterCrossfadeRemaining = FILTER_CROSSFADE_LENGTH;
// per sample while fading:
const float t = 1.0f - (float)filterCrossfadeRemaining / FILTER_CROSSFADE_LENGTH;
float yNew = loopDamping.processSample(x);
float yOld = loopDampingShadow.processSample(x);
float y    = yOld + (yNew - yOld) * t;
--filterCrossfadeRemaining;
```
(O-Lyrica drives this from a message-thread atomic `filterUpdatePending`; for us the cutoff change originates on the audio thread from a param read, so we can snapshot+restart inline when the target cutoff differs from the live one.)

### 1.7 DC blocker — **BUILD NEW** (F1: none exists to port)

Neither O-Lyrica nor the architecture's references ship a DC blocker; the asymmetric exciter injection can leave DC. Standard one-pole (architecture §1 specifies the coefficient):
```cpp
// y[n] = x[n] − x[n−1] + R·y[n−1],  R = 0.995
dcOut = x - dcX1 + 0.995f * dcY1;  dcX1 = x;  dcY1 = dcOut;
```
Place on the **String output** (the comb already blocks DC on the exciter side, but the loop can accumulate it).

---

## 2. Phase 2.2 — Strike + Bow + Material + Velocity + Position

**Goal:** all three exciters on the String; material/dynamics complete. Verifies FUNC-02 (all), FUNC-06, DSP-07, DSP-08, QUAL-01.

### 2.1 Strike exciter — band-limited raised-cosine mallet (**BUILD NEW**; F3)

O-Bells' "strike" is a **2-sample rectangular impulse → 4× SVF bandpass** (attack-transient only); the raised-cosine window and "hardness LPF" the architecture describes **do not exist there** (the LPF fields are vestigial dead code — `BellVoice.cpp:887-914`). A bare impulse carries DC + alias energy (the click DSP-08 forbids). **Build a band-limited raised-cosine-windowed impulse:**
```cpp
// width W samples (a few; harder mallet = narrower W → brighter). Raised-cosine (Hann) window:
for (int n = 0; n < W; ++n)
    burst[n] = 0.5f * (1.0f - std::cos(2.0f*pi*(n+1)/(W+1)));   // smooth on/off → no DC step, band-limited
```
(The raised-cosine math is the same shape O-Bells uses for bloom — `BellVoice.cpp:1124`.) Then a **hardness `OnePoleLPF`** (cutoff from `excitationColor`) shapes mallet softness. Feed the result into the KS loop (Strike-on-String) or the modal bank (Strike-on-Modal). `excitationColor` → mallet hardness (width + LPF cutoff); velocity raises both via `velToBrightness`.

### 2.2 Bow exciter (String) — **`ouaricon_add_module(bow-friction)`** (F2)

Add the module rather than copy: `modules/synthesis/bow-friction/` v1.0.0 (`module.yaml`), classes `HyperbolicFriction` + `BowModel`, pure value-class C++ (no JUCE modules, `cpp_standard 20`), defaults `mu_s=0.8, mu_d=0.3, v_0=0.05, R_s=0.5`.

**Memoryless friction curve** — `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h:33-44`:
```cpp
float computeReflectionCoefficient(float v_delta, float F_bow) const noexcept {
    float absV = std::abs(v_delta);
    float mu   = mu_d + (mu_s - mu_d) * v_0 / (v_0 + absV);   // hyperbolic Stribeck
    float r    = 0.25f * mu * F_bow / R_s;
    float rho  = r / (1.0f + r);                              // bounded [0, ~0.5], no epsilon needed
    return rho;
}
```
**Injection into a single KS loop** (adapt O-Bowed `writeJunction`, `WaveguideString.cpp:204-222`): read the loop sample as the junction velocity, drive friction, inject a friction-limited velocity:
```cpp
float vString = loopLPF.processSample(delay.popSample(0));   // string velocity at the junction (KS variant)
float vDelta  = vBow - vString;                              // vBow scaled by velocity
float rho     = friction.computeReflectionCoefficient(vDelta, F_bow);   // F_bow from bowForce
rho           = std::min(rho, 0.85f);                        // O-Bowed voice-level clamp (:170)
float clampedRho = std::min(rho, 0.99f);                     // guards the 1-clampedRho division (:207)
float fVel    = 2.0f * clampedRho / (1.0f - clampedRho);
float inject  = std::copysign(std::min(fVel, std::abs(vDelta)), vDelta);  // stick below limit, slip above
float toLoop  = g * vString + inject;
constexpr float sat = 4.0f;
toLoop = sat * std::tanh(toLoop / sat);                      // odd-symmetric soft sat — won't inject DC (:217)
delay.pushSample(0, toLoop);
```
`bowForce` → `F_bow` (pressure; higher = larger `rho`); rosin sharpness via `v_0 = 0.1·exp(−4.6·rosin)` if exposed (v1.0 keeps single `bowForce`, so default rosin). **Memoryless-first per CONTEXT D2:** validate a basic sustained tone BEFORE refining; the elasto-plastic/thermal tiers are OUT OF SCOPE (in O-Bowed they're dormant scaffolding — `bristleFriction` is configured but **never called** in the render path, so memoryless is already the shipping behavior).

**Fallback (CONTEXT D2):** if the table destabilizes the KS loop or reads poorly → sustained band-limited **friction-noise drive** into the loop (BowNoiseGenerator, §2.3) — still "Bow sustains," less idiomatic timbre.

**Stability guards (PORT ALL — QUAL-01):** `rho` clamps (0.85 then 0.99), `tanh` soft-sat (sat=4), denormal flush `if(|y|<1e-15) y=0`, final per-voice `jlimit(-1,1)`. The harness asserts finite/bounded at **max Bow Force + max Decay**.

### 2.3 Bow → Modal driver / friction-noise fallback — `BowNoiseGenerator` (COPY HEADER)

Not in the module — copy `O-Bowed/Source/DSP/BowNoiseGenerator.h:21-44` verbatim:
```cpp
*bandpassFilter.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(sampleRate, 3464.0f, 0.87f);
...
float noise    = noiseRandom.nextFloat()*2.0f - 1.0f;          // white
float filtered = bandpassFilter.processSample(noise);          // band-limited (3464 Hz, Q 0.87)
float amplitude= bowPressure * bowSpeed * noiseAmount * 0.03f; // tracks bowing intensity
return filtered * amplitude;
```
Per-voice seed `voiceIndex * 31337` (deterministic, no shared mutable state). This is the **sustained `e[n]` that bows the Modal bank** (continuous energy → the bandpass modes sustain instead of decaying) and the **String fallback**.

### 2.4 Material macro — steel↔nylon, co-moves Damping + Decay (constants resolved; open item #5)

Macro writes BOTH `damping` and `decay` param values (so both knobs visibly move — DSP-07). `material ∈ [0,1]` (0 = steel, 100 = nylon, default 30 = steel-ish). **Concrete starting constants** (architecture §3; tune by ear in execute):
- **Cutoff (log-frequency lerp):** `fc = exp( lerp(ln(10000), ln(2000), material/100) )` → 10 kHz steel … 2 kHz nylon.
- **Feedback (linear lerp):** `g = lerp(0.995, 0.93, material/100)` → long steel … short nylon.
- **Write-back to the visible 0–100% knobs** (keep them truthful): `damping% = 100·ln(12000/fc)/ln(12000/1500)`, `decay% = 100·(g−0.80)/(0.999−0.80)`.

Runs on the message thread (APVTS writes), atomic — must run **before** the loop reads `damping`/`decay` (architecture Integration). NB the macro→param write is the one place the Material gesture sets the other two knobs; in execute decide whether the macro write is UI-driven (Stage 3) or also applied at block start in DSP.

### 2.5 Velocity dynamics (FUNC-06)

Inline in the exciters (architecture §11): velocity (0–127→0–1) scales exciter **amplitude** AND raises exciter-LPF **cutoff**; depth = `velToBrightness`. Shared across all three exciters.

---

## 3. Phase 2.3 — Modal resonator + cross-driving (**BUILD NEW**; F3)

**Goal:** the second resonator engine, driven by the SAME `e[n]` (FUNC-04). Verifies FUNC-03, FUNC-04, DSP-04, DSP-05.

### 3.1 Topology — exciter-driven RBJ constant-skirt bandpass biquad bank (custom direct-form)

8 parallel 2nd-order bandpass biquads fed `e[n]` each sample; output = `Σ gain_k · bp_k(e[n])`. Use a **custom direct-form array** (O-Bassoon precedent — NOT `juce::dsp::IIR::Filter` at 128-instance scale). O-Bassoon's per-mode struct is the structural template (`O-Bassoon/Source/ModeBank.h:61-89`) — note its **isfinite guard** (which O-Bells lacks):
```cpp
struct ModeBiquad {
    float b0=0,b1=0,b2=0,a1=0,a2=0, x1=0,x2=0,y1=0,y2=0;
    inline float processSample(float x) noexcept {
        float y0 = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
        x2=x1; x1=x; y2=y1; y1=y0;
        if (!std::isfinite(y1) || !std::isfinite(y2)) { x1=x2=y1=y2=0; return 0.0f; }   // PORT THIS GUARD
        return y0;
    }
};
```
**RBJ constant-skirt bandpass coefficients** (peak gain ∝ Q — gives high-Q modes natural loudness, the antidote to O-Bassoon's quiet pole-only modes):
```cpp
float w0 = 2.0f*pi*f_k/fs, cw = std::cos(w0), sw = std::sin(w0);
float alpha = sw / (2.0f * Q_k);
float a0 =  1.0f + alpha;
b0 =  (sw*0.5f)/a0;  b1 = 0.0f;  b2 = -(sw*0.5f)/a0;       // constant-skirt: peak gain = Q
a1 = (-2.0f*cw)/a0;  a2 = (1.0f - alpha)/a0;
```
Recompute coefficients at **block rate** (not per-sample). Output gain per mode = the amplitude tilt (§3.4), with a global normalization (`~1/Σ amp_k`) so the bank can't clip.

### 3.2 Inharmonicity — Fletcher stretch (architecture formula, NOT O-Bells tables; DSP-05)

Architecture mandates the **continuous** Fletcher stretch (O-Bells' harmonic/bell/gamelan **morph tables are a different model — do not use**):
```
f_k = f0 · k · sqrt(1 + B·k²),   k = 1..8
B   = (inharmonicity/100) · B_max,   B_max ≈ 0.012   // 0% ≈ bar/harmonic, 100% ≈ bell
```
Recompute `f_k` at note-on and on `inharmonicity` change (block rate). The viz stem display reads these exact `f_k` (truthful by construction — UI-05).

### 3.3 T60 → Q — **DERIVE (F3: O-Bells stores an amplitude env, not a Q)**

O-Bells' `decayRate = exp(−1/(T·fs))` is an oscillator amplitude multiplier, **not** a biquad bandwidth. Standard modal conversion:
```
τ_k  = T60_k / 6.908            // 6.908 = ln(1000); amplitude e-folding time
Q_k  = π · f_k · τ_k = π·f_k·T60_k / 6.908   ( = 0.4548 · f_k · T60_k )
```
`T60_k = baseT60 · DECAY_MULTIPLIERS[k]`, with `baseT60 = lerp(0.3s, 6.0s, decay/100)` (tune by ear). Clamp `Q_k` to a sane upper bound (e.g. ≤ 500) — O-Bells/O-Bassoon have **no Q clamp**; add one.

**`DECAY_MULTIPLIERS` (REUSE DATA — `O-Bells/Source/BellVoice.h:74-75`):**
```cpp
static constexpr float DECAY_MULTIPLIERS[8] = {1.2f,1.0f,0.85f,0.7f,0.6f,0.5f,0.4f,0.3f};  // higher modes faster
```
Optionally seed `baseT60` from O-Bells' physically-grounded freq-dependent damping `R_k = b1 + b3·f²` (`BellVoice.cpp:944-968`), where `b1=0.5`, `b3 = (100−brilliance)/100·2e-8`, giving `T60_k ∝ 1/R_k`.

### 3.4 Mode amplitude tilt (REUSE FORMULAS — `O-Bells/Source/BellVoice.cpp:744-758`)

```cpp
float baseAmp = 1.0f / (k + 1.0f);                                  // 1/k falloff
float ratio   = k / 8.0f;
float bright  = 1.0f + ratio * (1.9f * modeBrightness - 0.9f);      // 0.5 ≈ flat, <0.5 dark, >0.5 metallic
amp_k = baseAmp * bright;
```
`modeBrightness` 0–100% → tilt; these become the per-mode **output gains** (§3.1).

### 3.5 The "inaudible sustain" risk + mitigation (O-Bassoon lesson — HIGHEST modal risk)

O-Bassoon found a pole-only resonator driven by a short impulse leaves high-Q low-freq modes **~50 dB below target** (`ModeBank.h:50-56`) and had to add `strike()` to inject state. Our mitigations, in order:
1. **Constant-skirt RBJ bandpass** (§3.1): peak gain = Q, so high-Q modes are intrinsically loud (opposite of pole-only). Primary fix.
2. **Drive with energetic `e[n]`:** the Pluck noise *burst* and the sustained Bow *noise* carry far more energy than a single impulse; the Strike raised-cosine burst (§2.1) is several samples wide.
3. **Fallback (validated):** if modes still ring too quietly, port O-Bassoon's `strike()` — set each mode's `y1,y2` at note-on to launch `amp·sin((n+1)θ)·R^n` (`ModeBank.h` `strike()`), cosθ/sinθ cached from `f_k`. Keep in reserve.

### 3.6 Cross-driving + resonator switch (FUNC-04)

`e[n]` is computed once (Pluck/Strike/Bow) and fed to whichever resonator `resonatorType` selects — String (KS loop) or Modal (bandpass bank). The Modal path is **feed-forward** (no feedback loop → no stability risk). Bow→Modal uses the continuous BowNoiseGenerator noise as `e[n]` (sustains the bandpass ring); Pluck/Strike→Modal ring-and-decay per T60. `inharmonicity`/`modeBrightness` are no-ops in String; `stringModel`/Position-as-pickup are no-ops in Modal (UI greys per resonator at Stage 3).

### 3.7 Modal-stem viz snapshot

Lead-voice publishes the 8 `(f_k, amp_k)` pairs (the exact values driving the audio) into a fixed-size atomic array (§6). Read by the Stage-3 stem display (UI-05).

---

## 4. (Deferred) Phase 2.4 — Waveguide string

OUT for v1.0 (CONTEXT D1). `stringModel` ships exposing **KS only**. When built (v1.1): dual `DelayLine<float,Thiran>` rails + bridge loss LPF + nut sign-inversion — O-Bowed `readJunction`/`writeJunction` (`WaveguideString.cpp:186-235`) and bridge-loss coeffs (`g·(1−p), 0, 1, −p`, `p=exp(−2π·brightnessHz/fs)`, `.cpp:86-95`) are the reference. No contract break: the `stringModel` choice param is already wired (Stage 1).

---

## 5. Render-harness gate — autocorrelation pitch probe (PORT VERBATIM + tighten)

The Stage-2 correctness gate. The current harness CMake is **already correct** (`tests/render-harness/CMakeLists.txt`: `JUCE_WEB_BROWSER=0`, drops `PluginEditor.cpp`, no `_UIResources`) — **do not regress it** toward the O-simpleFM/Grain retrofit. The current `main.cpp` stub tests `output-finite`, `shell-silent` (peak<1e-6), `state-roundtrip`.

**Stage-2 changes to `main.cpp`:**
1. **Invert `shell-silent` → `makes-sound`** (`r > 0.02` RMS) — the shell-silent assertion at `:106` becomes false the moment audio renders.
2. Add the vector `render()` + `rms`/`peakAbs`/`allFinite` helpers (O-simpleFM `main.cpp:53-71, 76-104`). For a decaying pluck, hold the note and put the analysis window **early/short** (after the attack, before decay into noise floor).
3. **Add `autocorrPitchHz` verbatim** — `O-simpleGrain/tests/render-harness/main.cpp:112-141`:
```cpp
static double autocorrPitchHz(const std::vector<float>& x, int off, int len,
                              double fs, double fLo, double fHi, double minCorr = 0.3) {
    const int lagMin = juce::jmax(1,(int)(fs/fHi)), lagMax = (int)(fs/fLo);
    double e0 = 0.0; for (int n=0;n<len;++n){double s=x[off+n]; e0+=s*s;}
    if (e0 < 1.0e-9) return 0.0;
    double bestCorr=0.0; int bestLag=-1;
    for (int lag=lagMin; lag<=lagMax; ++lag){
        double acc=0.0,eL=0.0;
        for (int n=0;n+lag<len;++n){double a=x[off+n],b=x[off+n+lag]; acc+=a*b; eL+=b*b;}
        double c = acc/(std::sqrt(e0*eL)+1.0e-12);
        if (c>bestCorr){bestCorr=c; bestLag=lag;}
    }
    return (bestLag>0 && bestCorr>=minCorr) ? fs/(double)bestLag : 0.0;
}
```
4. **Cents-tight gate (NOT O-Grain's loose ±7%):** bracket the search per note `[f0·0.8, f0·1.25]` (kills octave ghosts), convert to cents and assert ≤ **±5 cents** at C1/C3/C5/C7:
```cpp
double meas = autocorrPitchHz(y, off, len, fs, f0*0.8, f0*1.25);
double cents = 1200.0 * std::log2(meas / f0);
check("tuning-C5", meas>0 && std::abs(cents) <= 5.0, juce::String(cents,2)+" cents");
```
5. Add: decay-shortens-with-Decay, no-DC (mean ≈ 0 post-DC-blocker), Bow-sustains (RMS steady while held), Bow+max-Decay finite/bounded, Strike-no-top-octave-alias (architecture audit — add oversampling only if this trips).

**PASS/FAIL + exit:** keep the existing `check` lambda + `return failures==0?0:1` (O-simpleFM `main.cpp:137-142, 260-264`). Build with `-DOUARICON_BUILD_TESTS=ON`; **harness-gate each phase before the next** (CONTEXT D3).

---

## 6. Viz wiring (through Stage 2, consumed Stage 3)

**`VizRing` — PORT VERBATIM** (`O-simpleFM/Source/FmVizAnalyzer.h:30-58`): 8192-sample power-of-two lock-free ring, copy-only `write` (audio thread, relaxed stores + release writePos), `readLatest` (message thread, acquire). Fully allocated at construction — nothing to `prepare()`.

**Loop-energy scalar + modal-stem array — AUTHOR NEW** (F: not in FmVizAnalyzer; only a `frameCount` atomic exists at `:117/:122/:130`). Idiom = `std::atomic<float>` written `store(...,relaxed)` on the audio thread, read `load(relaxed)` on the message thread (exactly like `frameCount`). For the lead voice publish: (a) loop-energy scalar (KS circulating energy, for the dampening loop diagram UI-02), (b) fixed-size 8-entry `(f_k, amp_k)` stem array. **Audio-thread copy-only, no alloc/lock/FFT (PERF-01).** Lead voice = most-recently-triggered (architecture §7). Confirm taps running by end of Stage 2; FFT/scope/stems computed on the Stage-3 editor 30 Hz Timer.

---

## 7. Decisions carried into the plan phase

| # | Decision | Rationale |
|---|----------|-----------|
| R1 | **Thiran delay-line from O-Bowed**, not O-Lyrica (F1). `setDelay(max(2, fs/f0 − τ_lpf))`. | O-Lyrica is Lagrange3rd; O-Bowed is the production Thiran reference. |
| R2 | **`ouaricon_add_module(bow-friction)`** for the String Bow (F2) — `/module-add O-simplePhysicalModelSynth bow-friction`. | Registered, dep-free, already consumed by 2 plugins. Cleaner than copy-paste; tracks upgrades. |
| R3 | **Build modal bank new** (RBJ constant-skirt bandpass, exciter-driven); reuse O-Bells `DECAY_MULTIPLIERS` + tilt **data**; **derive Q** (F3). | O-Bells is triggered sinusoids (breaks FUNC-04); O-Bells stores no Q. |
| R4 | **Author DC blocker + biquad isfinite-guard + Q clamp** — none exist in the references. | QUAL-01 / DSP-08 stability; references lack these. |
| R5 | **Build the Strike raised-cosine exciter new** (O-Bells' is a bare 2-sample impulse). | DSP-08 (no click/DC/alias). |
| R6 | **Material macro constants** resolved (§2.4): log-cutoff lerp 10k↔2k, linear g lerp 0.995↔0.93. | open item #5; tune by ear in execute. |
| R7 | **Modal "too quiet" mitigation ladder** (§3.5): constant-skirt → energetic drive → O-Bassoon `strike()` fallback. | Highest modal risk (O-Bassoon's −50 dB lesson). |
| R8 | **Harness: invert `shell-silent`→`makes-sound`, add `autocorrPitchHz`, ±5-cent gate**; CMake untouched. | Stage-2 gate; current CMake already seamed correctly. |

---

## 8. Pitfalls (knowledge base) — status for this stage

| Pitfall (memory) | Status |
|------------------|--------|
| KS loop comb fools spectral pitch probes | **Pre-empted** — autocorrelation probe (§5), NOT DFT. O-simpleGrain lesson. |
| Render-harness breaks when editor becomes WebView | **Pre-empted** — CMake already `JUCE_WEB_BROWSER=0` + drops `PluginEditor.cpp`; re-run at start of Stage 4. |
| `SamplerVoice`/`SamplerSound` shadow `juce::` types | **Avoided** — `PhysicalModelVoice`/`Sound`. New DSP classes: `StringResonator`, `ModalResonator`, `PluckExciter`, `StrikeExciter`, `BowExciter` — none shadow `juce::`. |
| Bare `end`/`begin` param-ID shadows `juce::` free fn | **N/A** — none in the 17 IDs. |
| Friction blow-up (NaN/screech/runaway) | **Guarded** — rho clamps + tanh sat + denormal flush + final jlimit (§2.2); harness asserts bounded at max Bow Force + max Decay. |
| High-Q biquad denormal/instability | **Guarded** — isfinite guard + Q clamp (authored, §3.1/3.3). |
| Modal modes inaudible (pole-only −50 dB) | **Mitigated** — constant-skirt + energetic drive + `strike()` fallback (§3.5). |
| Zipper clicks on cutoff change | **Handled** — 64-sample shadow-filter crossfade (§1.6). |
| Strike top-octave aliasing | **Audited** by harness; oversampling added ONLY if it trips (no oversampling v1.0). |
| Dual `juce_add_binary_data` namespace collision | **N/A** (no binary data until Stage 3; one UI target → no collision). |

---

## 9. File manifest for the plan phase

New under `Source/` (all header-only DSP unless noted — keeps the render-harness clean):
- `StringResonator.h` — KS loop: `juce::dsp::DelayLine<float,Thiran>` + `OnePoleLPF` (§1.1) + feedback + group-delay comp (§1.3) + shadow crossfade (§1.6) + DC blocker (§1.7).
- `PluckExciter.h` — noise burst + brightness LPF + position comb (§1.5).
- `StrikeExciter.h` — band-limited raised-cosine impulse + hardness LPF (§2.1, new).
- `BowExciter.h` — wraps module `HyperbolicFriction` (String, §2.2) + `BowNoiseGenerator.h` (Modal/fallback, §2.3, copied header).
- `ModalResonator.h` — 8× RBJ constant-skirt bandpass biquads + Fletcher inharmonicity + T60→Q + tilt + isfinite/Q guards (§3).
- `VizTap.h` (or extend) — `VizRing` (copied) + loop-energy/stem atomics (§6, new).
- **Edit** `PhysicalModelVoice.h` — wire exciter→resonator chain, amp ADSR, velocity, per-voice state, lead-voice viz publish; replace the silent `renderNextBlock` no-op.
- **Edit** `PluginProcessor.cpp` — `getRawParameterValue()->load()` reads, Material macro (§2.4), lead-voice tracking, VizRing write, `getTailLengthSeconds` already 5 s.
- **Edit** `CMakeLists.txt` — `ouaricon_add_module(O-simplePhysicalModelSynth bow-friction)` (R2); `juce_dsp` already linked.
- **Edit** `tests/render-harness/main.cpp` — §5 (CMake untouched).

---

## 10. Verify-phase targets (Stage 2 exit — carried from CONTEXT)

- [ ] Note-on → plucked tone rings + decays; f0 within ±5 cents at C1/C3/C5/C7 (autocorrelation harness).
- [ ] Pluck=plucked, Strike=mallet, Bow=sustained (no decay while held); swapping exciter changes only attack/drive.
- [ ] Material sweeps steel→nylon in one gesture; Damping+Decay co-move; harder velocity = brighter/stronger.
- [ ] Modal = inharmonic struck-bar/bell; each exciter drives Modal (cross-driving); Inharmonicity 0%≈bar, high%≈bell.
- [ ] Feedback clamped < 1 — bounded/finite at max Decay AND max Bow Force (QUAL-01); no click/DC/buzz/alias.
- [ ] Viz taps running (loop-energy + modal-stem), audio-thread copy-only.
- [ ] Render-harness re-buildable + ALL PASS; pluginval clean.

---

## References

**In-house (verbatim sources, file:line in body):**
- `O-Bowed/Source/DSP/WaveguideString.{h,cpp}` — Thiran delay (R1), junction inject/read, tanh sat, stability guards. `BowNoiseGenerator.h` — Modal bow / fallback.
- `modules/synthesis/bow-friction/` (v1.0.0) — `HyperbolicFriction` + `BowModel` (R2).
- `O-Lyrica/Source/DSP/WaveguideString.{h,cpp}` — `OnePoleLPF`, group-delay formula, shadow crossfade. `PluckExciter.cpp` — position comb. (NOT Thiran; NO DC blocker — F1.)
- `O-Bells/Source/BellVoice.{h,cpp}` — `DECAY_MULTIPLIERS`, amplitude/strike tilt, `R_k=b1+b3·f²` (data only; triggered-sinusoid engine is replaced — F3).
- `O-Bassoon/Source/ModeBank.h` — custom direct-form biquad struct, isfinite guard, `strike()` injection + the "inaudible sustain" lesson (R7).
- `O-simpleFM/Source/FmVizAnalyzer.h` — `VizRing`. `tests/render-harness/main.cpp` — render skeleton, PASS/FAIL pattern, DFT contrast.
- `O-simpleGrain/tests/render-harness/main.cpp` — `autocorrPitchHz` (R8).
- Current build: `Source/PluginProcessor.{h,cpp}`, `Source/PhysicalModelVoice.h`, `tests/render-harness/{CMakeLists.txt,main.cpp}`.

**Upstream:** `research/ARCHITECTURE.md` (immutable), `stages/2-dsp/CONTEXT.md` (D1–D3), `ROADMAP.md`, `REQUIREMENTS.md`. Theory: Karplus-Strong (1983); J.O. Smith *PASP* (waveguide/Thiran); Fletcher-Rossing (inharmonicity `f_k=f0·k·√(1+B·k²)`); STK Cook/Scavone (friction).
