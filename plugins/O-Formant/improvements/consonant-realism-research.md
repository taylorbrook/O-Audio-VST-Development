# O-Formant Consonant Realism — Research Report

**Date:** 2026-04-16
**Target version:** v1.23.0 (MINOR bump, additive, non-breaking)
**Research scope:** Level 3 parallel investigation (phonetic acoustics literature + Klatt synthesis architecture + open-source reference implementations)

---

## Executive Summary

O-Formant v1.22.0's consonant engine uses a single white-noise source through 2 resonant bandpass filters, routed into the shared 5-formant bank. This topology is **fundamentally inadequate** for realistic consonants for five reasons supported by the literature:

1. **No frication-specific formant bank** — Klatt (1980) and all modern formant synths (KlattGrid, eSpeak NG) use an independent parallel bank with a **bypass path** for weak fricatives. A shared vowel formant bank cannot produce /f/, /θ/, /h/ correctly.
2. **No burst spectral templates** — Stevens & Blumstein (1978, 1979) showed three distinct burst shapes (diffuse-falling, diffuse-rising, compact) are the primary perceptual cue for /p/ vs /t/ vs /k/. O-Formant produces the same burst shape regardless of place.
3. **No VOT (Voice Onset Time) modeling** — Lisker & Abramson (1964) measured English /p t k/ VOTs of 58/70/80 ms (aspiration gap between burst and voice). O-Formant's 25 ms onset suppression is half of even the shortest VOT.
4. **No formant transitions into adjacent vowel** — Delattre-Liberman-Cooper (1955) locus theory is the single most important cue for place. O-Formant has no F2/F3 transitions at all.
5. **No voiced-fricative noise modulation** — Klatt's MOD (50% square gate at F0) is why /z/ sounds different from /s/+vowel. Missing in O-Formant.

**Top 4 recommended additions for v1.23.0** (ranked by realism-per-LOC):

| # | Change | Realism Impact | Complexity | New params |
|---|--------|----------------|------------|------------|
| **1** | Frication formant bank + bypass path (Klatt AF/AB topology) | ★★★★★ | Medium (~200 LOC, 1 new file) | 1 optional |
| **2** | Plosive burst spectral templates (Stevens-Blumstein) | ★★★★☆ | Low (~50 LOC in ConsonantEngine) | 0 |
| **3** | VOT / aspiration phase for voiceless stops | ★★★★☆ | Medium (~100 LOC) | 1 optional |
| **4** | Voiced-fricative noise gating (Klatt MOD) | ★★★☆☆ | Trivial (~10 LOC) | 0 |

**Stretch goal (item 5):** locus-based F2/F3 transitions — ★★★★★ impact but Medium-High complexity; defer to v1.24.0 if scope tight.

---

# Part 1: Measured Consonant Acoustics (Literature)

## 1.1 Sibilants /s z ʃ ʒ/ — Jongman, Wayland & Wong (2000)

| Consonant | Spectral Peak | Spectral Mean (M1) | Duration | Amplitude (re vowel) |
|-----------|--------------|--------------------|----------|---------------------|
| /s/ | 6500–7500 Hz | 6133 Hz | ~170 ms | 0 to −5 dB |
| /z/ | 6000–7000 Hz | 5500 Hz | ~120 ms | −3 to −7 dB |
| /ʃ/ | 2500–4000 Hz | 4229 Hz | ~170 ms | 0 to −3 dB |
| /ʒ/ | 2500–4000 Hz | 4000 Hz | ~120 ms | −3 to −5 dB |

**Synthesis rules:**
- /s z/: bandpass noise, center 6–7 kHz, BW 2–3 kHz (Q ≈ 2–3). **Front cavity is only ~1 cm** → narrow, high peak.
- /ʃ ʒ/: bandpass noise, center 3 kHz, BW 2 kHz (Q ≈ 1.5). Longer front cavity (~2.5 cm).
- **Voicing:** voiced sibilants have shorter duration (~30%), amplitude −3 to −5 dB lower, and the noise is **AM-gated by the glottal pulse** (see Klatt MOD, §2.2).

## 1.2 Non-sibilant fricatives /f v θ ð/ — Stevens (1998), Jongman (2000)

| Consonant | Spectral Peak | Duration | Amplitude (re vowel) |
|-----------|--------------|----------|---------------------|
| /f/ | ~7500 Hz (flat) | ~140 ms | **−15 to −20 dB** |
| /v/ | ~7500 Hz (flat) | ~100 ms | −18 to −22 dB |
| /θ/ | ~7500 Hz (flat) | ~150 ms | −15 to −20 dB |
| /ð/ | ~7500 Hz (flat) | ~90 ms | −18 to −22 dB |

**Why they're weak and broad:**
- **Obstacle source** (flow hitting teeth/lip) is ~15–20 dB weaker than sibilant **free-jet source**.
- **No front cavity resonance** — constriction is at the lips/teeth with no forward resonator.
- Spectrum is essentially **flat** across 1–10 kHz.

**Synthesis rule:** broadband noise (flat or +3 dB/oct), −15 dB vs sibilants, **no resonant peak**. This is why Klatt routes them through a **bypass path** (AB parameter) that skips the formant bank entirely.

## 1.3 Plosive bursts — Stevens & Blumstein (1978, 1979)

| Stop | Template | Burst Peak | Burst Duration | VOT | Amplitude |
|------|----------|-----------|---------------|-----|-----------|
| /p/ | Diffuse-falling | 500–1500 Hz | 5–15 ms | 58 ms | −10 to −15 dB |
| /b/ | Diffuse-falling | 500–1000 Hz | 5–10 ms | 0–20 ms | −12 to −18 dB |
| /t/ | Diffuse-rising | 3500–5000 Hz | 10–20 ms | 70 ms | −5 to −10 dB |
| /d/ | Diffuse-rising | 3000–4500 Hz | 8–15 ms | 0–20 ms | −10 to −15 dB |
| /k/ | Compact | 1500–4000 Hz (vowel-dep.) | 20–40 ms | 80 ms | −3 to −8 dB |
| /g/ | Compact | 1500–3500 Hz | 15–30 ms | 0–25 ms | −5 to −12 dB |

**Three template shapes (Stevens-Blumstein):**
- **Diffuse-falling** (/p b/): 6 dB/oct lowpass at ~800 Hz, energy spread but tilted down
- **Diffuse-rising** (/t d/): 6 dB/oct highpass at ~3.5 kHz, energy tilted up
- **Compact** (/k g/): narrow bandpass, Q ≈ 3, center tuned to following vowel F2 (front V: 3–4 kHz; back V: 1–1.5 kHz)

**Classification accuracy:** templates alone achieve ~85% correct place identification in perception tests (Blumstein & Stevens 1979). This is the dominant place cue for stops.

## 1.4 VOT and closure — Lisker & Abramson (1964), Klatt (1975)

**Voice Onset Time (word-initial, English):**
- /p/ = 58 ms, /t/ = 70 ms, /k/ = 80 ms (aspirated voiceless)
- /b/ /d/ /g/ = 0–25 ms (short-lag voiced) or −80 ms (prevoiced, ~31% of speakers)
- After /s/ (/sp st sk/): VOT reduces to 10–30 ms

**Aspiration phase:** between burst and voice onset, 40–60 ms of **glottal frication noise** filtered by the opening vocal tract (F1 rising from ~180 Hz to vowel target). This is Klatt's AH parameter.

**Stop closure duration (intervocalic):**
- Voiceless: /p/ 118 ms, /t/ 102 ms, /k/ 113 ms
- Voiced: /b/ 86 ms, /d/ 69 ms, /g/ 80 ms (~1.4× shorter)
- Pre-voicing is **not necessary** in American English (69% of speakers don't prevoice) — closure duration + F1 onset frequency are the primary voicing cues.

## 1.5 Formant transitions — Delattre-Liberman-Cooper (1955), Kewley-Port (1982)

**F2 Locus frequencies:**

| Place | F2 Locus | F3 Locus | Slope (F2_onset vs F2_vowel) |
|-------|---------|---------|------------------------------|
| Labial | **720 Hz** | ~2000 Hz | 0.90 (steep — strong coarticulation) |
| Alveolar | **1800 Hz** | ~2700 Hz | 0.40 (shallow — rigid) |
| Palatal | **2200 Hz** | ~3000 Hz | 0.70 |
| Velar (front V) | ~3000 Hz | ~2500 Hz | 0.95 |
| Velar (back V) | ~1200 Hz | ~2200 Hz | 1.0 (velar pinch: F2≈F3) |
| Retroflex /ɹ/ | ~1200 Hz | **~1600 Hz** (low F3 = key cue) | — |

**Transition parameters:**
- **Duration:** 40–50 ms typical (30–60 ms perceptual range)
- **Trajectory:** exponential decay to vowel target, τ ≈ 15 ms (first-order lowpass on a step)
- **CV vs VC asymmetry:** VC ~20% longer and shallower; perceptually critical portion = last 20 ms of VC, first 20 ms of CV

**Why this matters:** locus-based transitions are the #1 place cue for stops when bursts are masked or ambiguous. Missing in O-Formant entirely.

## 1.6 Affricates /tʃ dʒ/ — Howell & Rosen (1983), Kluender & Walsh (1992)

The **rise time** of the frication amplitude distinguishes affricate from stop+fricative cluster:

| Parameter | Affricate /tʃ/ | Cluster /t+ʃ/ |
|-----------|---------------|--------------|
| Frication rise time | **~30 ms** (abrupt) | ~75 ms (gradual) |
| Total frication duration | 60–90 ms | 100–140 ms |

**Synthesis rule:** affricate = closure (50–80 ms) → **30 ms raised-cosine rise** → 60–80 ms frication (same spectrum as /ʃ/) → transition. **No audible burst** between closure and frication.

---

# Part 2: Klatt Synthesis Architecture

## 2.1 Signal Flow (Klatt 1980)

Klatt has **two independent noise sources** routed to different paths:

```
  F0 ─▶ IMPULSE ─▶ RGP ─▶ RGZ ─┬─▶ ×AV ────────────────┐ voicing
                                └─▶ ×AVS ─▶ (voicebar) ─┤
                                                        ▼
  NOISE ─▶ MOD ─▶ ×AH ─────────────────────────────────▶ CASCADE (F1..F5)
          (50%      aspiration (same noise filters)                   │
          F0 gate)  routed through vowel formants                     ▼
                                                                   Σ ─▶ OUT
          ─▶ ×AF ─▶ LPF(−6dB/oct) ─┬─▶ ×A2..A6 ─▶ R2..R6 PARALLEL ───▶│
                                   └─▶ ×AB (BYPASS, flat) ────────────▶│
```

**Key insight:** frication (AF) and aspiration (AH) are the **same noise process** but routed through **different formant banks**:
- **AH** → cascade vowel-tract formants (for /h/, voiceless stop aspiration)
- **AF** → parallel frication formants F2F/F3F/F4F (for /s/, /ʃ/, /f/, bursts)
- **AB** (bypass) → flat noise, no formant filtering (for weak fricatives /f θ/)

## 2.2 Consonant Source/Routing Table (Klatt 1980 Table III)

| Consonant | Voicing | Noise Source | Routing | Notes |
|-----------|---------|-------------|---------|-------|
| /h/ | off | AH | cascade (vowel formants) | Noise-excited vowel |
| /s/ | off | AF | Parallel: **A6=52** (F6=4900 Hz) | Narrow high peak |
| /z/ | AVS | AF + MOD | Same as /s/ + voice bar | AM-gated noise |
| /ʃ/ | off | AF | Parallel: A3=57, A4=48, A5=48, A6=46 | Broad mid peak |
| /ʒ/ | AVS | AF + MOD | Same as /ʃ/ | |
| **/f/, /v/** | (AVS) | AF | **AB=57, all An=0** | **Bypass only — flat** |
| /θ/, /ð/ | (AVS) | AF | **AB=48, A6=28** | Bypass + tiny high formant |
| /p/, /b/ | off | AF (step, >50 dB jump) | **AB=63** | Flat burst, labial |
| /t/, /d/ | off | AF (step) | A3=30, A4=45, A5=57, A6=63 | Alveolar high energy |
| /k/, /g/ | off | AF (step) | A3=53, A4=43, A5=45, A6=45 | Compact mid peak |
| /tʃ/, /dʒ/ | (AVS) | AF | A3=44, A4=60, A5=53, A6=53 | /ʃ/ with burst onset |

## 2.3 Frication Formant Bank (Klatt defaults)

Klatt uses **fixed parallel-branch formants** — not swept per consonant. You only change the *amplitudes* A2..A6:

| Formant | Freq (Hz) | BW (Hz) | Used for |
|---------|----------|---------|----------|
| F2F | 1450 | 80–200 | Velar bursts, /f v θ ð/ |
| F3F | 2450 | 170–300 | /ʃ ʒ tʃ dʒ/ main peak |
| F4F | 3300 | 250 | /t d/ burst peak |
| F5F | 3750 | 200 | High-frequency shape |
| **F6F** | **4900** | **1000** | **Sibilant /s z/ — dedicated** |

**F6 is a fixed "sibilant formant"** parked at 4900 Hz with wide 1 kHz bandwidth. You don't move it — you only change A6 gain. This is the trick for /s/.

## 2.4 Burst Generation (not an impulse!)

Klatt's plosive burst is **NOT** an impulse — it's gated frication noise:
1. AF steps from 0 → ~60 dB in one frame (the >50 dB/frame rule bypasses interpolation → sharp attack)
2. Duration: 5–15 ms burst
3. Shape: determined by which A2..A6 amplitudes are open at release moment (per Table III above)
4. For /p/: AB=63, all An=0 → flat diffuse burst through bypass path
5. For /k/: A3=53, A4=43 → compact mid-frequency burst

## 2.5 VOT / Aspiration / Burst Timing (Klatt 1980 /pa/ example)

```
time:    0────5ms burst────5────45ms aspiration────45ms voicing onset
AV:      0──────0──────0──────0────────0──────────────60 (step at glottal pulse)
AF:      0──jump60──0──────────0────────0──────────────0   (5 ms burst)
AH:      0──0──────linear→60──60────────60─→0─────────0   (40 ms aspiration)
F0:      0──────0──────0──────0────────0──────────────130
```

Three sequential phases:
1. **Burst (5–15 ms):** AF steps up, parallel formant amplitudes set per Table III
2. **Aspiration (30–60 ms):** AH sustained, routed through cascade (excites vowel formants as they track toward target)
3. **Voice onset:** AV steps at next glottal pulse

**Voiced stops /b d g/:** AH ≈ 0–20 dB (short voice lead), AV kicks in within 5–20 ms of release. Voice bar during closure via AVS.

## 2.6 MOD — voiced fricative noise gating

50% square wave at F0 gates the noise source **before** AF amplitude:

```
noise_sample *= (glottal_phase < 0.5) ? 1.0 : 0.5;  // when voicing is active
```

This is what makes /z/ sound different from /s/ + vowel bleedthrough.

## 2.7 KlattGrid improvements (Weenink 2009)

KlattGrid (Praat) exposes **independent frication formant tiers** (F2F–F6F) with their own frequency, bandwidth, and amplitude — separate from the oral vowel-tract formants. This is architecturally cleaner than Klatt80's shared formants and is what I recommend O-Formant follow.

---

# Part 3: Open-Source Reference Code

## 3.1 Pink Trombone (Neil Thapen) — articulatory waveguide

**Most borrowable idea: `thinness × openness` turbulence gate**

One-line formula on constriction diameter that smoothly interpolates vowel → fricative → plosive behavior:

```js
thinness  = clamp(8  * (0.7 - diameter), 0, 1);
openness  = clamp(30 * (diameter - 0.3), 0, 1);
noise    *= thinness * openness / 2;
```

Bell-shaped: zero below d=0.3 (closure, no airflow) and above d=0.7 (vowel), peaks at d=0.5 (fricative). Replace any "is this a fricative?" branching with this curve.

**Plosive trigger:** when `diameter[i] ≤ 0`, mark obstruction; on release, spawn a `Transient` (exponentially-decaying alternating spike, `strength · (-2)^(t·200)`).

## 3.2 eSpeak NG — Klatt cascade/parallel with alternating-sign trick

**Most borrowable idea: alternating-sign parallel summation**

```c
for (ix = R2p; ix <= R6p; ix++)
    out = resonator(&rsn[ix], sourc) - out;   // ALTERNATING SIGN
outbypas = amp_bypas * sourc;
out = outbypas - out;
```

This creates **spectral notches between formants without explicit anti-resonators**, giving fricatives their characteristic valley-and-peak envelope. Cheaper and more stable than pole-zero pairs.

**Fun fact:** eSpeak actually uses **pre-recorded WAV bursts** for most stops (`WAV(ustop/p)`, `WAV(ustop/t)`) because pure Klatt burst synthesis is hard. They blend the WAV with generated formant transitions. This is evidence that **burst templates are hard to get right** — sample-based is the expedient fallback.

## 3.3 Praat KlattGrid — independent frication chain

Cleanest architectural separation. Frication has its own:
- Noise generator (white + one-pole LP, same as Klatt)
- Independent formant bank (F2F–F6F, not shared with vowel)
- Amplitude envelope tier
- **Bypass tier** (separate gain for unfiltered noise path)

## 3.4 Gnuspeech TRM — bandpass pre-filter + fractional frication taps

**Most borrowable idea: pre-filter noise source with bandpass, distribute across fractional tract position**

```cpp
signal = vocalTract((pulse + ah1 * signal) * VT_SCALE,
                    bandpassFilter_->filter(signal));
```

The noise is bandpass-filtered (CF and BW as continuous parameters) **before** injection into the tube, then amplitude-distributed across two adjacent tract sections via linear interpolation:

```cpp
int integerPart = (int) fricPos;
double complement = fricPos - integerPart;
fricationTap_[i]   = (1.0 - complement) * fricationAmplitude;
fricationTap_[i+1] = complement * fricationAmplitude;
```

**This is the cleanest mapping from continuous "place" to spectral output** across all surveyed codebases. It maps directly to the existing O-Formant place axis.

---

# Part 4: Implementation Plan for v1.23.0

## Priority Ranking (realism-per-LOC)

### ★★★★★ Recommended for v1.23.0

#### **Item 1: Frication Formant Bank + Bypass Path (Klatt AF/AB topology)**

**Why it's the biggest win:**
- Current architecture fundamentally cannot produce weak fricatives /f θ/ (flat noise −15 dB, needs bypass)
- Current architecture cannot produce /s/ correctly (needs dedicated 5–7 kHz sibilant peak beyond vowel F5)
- Currently the consonant noise rides on vowel formant bank, so timbre depends on vowel position — wrong physics

**DSP design:**

New file `Source/dsp/FricationFormantBank.h`:
```cpp
class FricationFormantBank {
  // 3 parallel BPFs at Klatt-style fixed frequencies
  FormantBiquad F3F;   // 2500 Hz, Q=2.0, BW=300 Hz — for /ʃ ʒ tʃ dʒ/
  FormantBiquad F4F;   // 3500 Hz, Q=1.8, BW=400 Hz — for /t d/
  FormantBiquad F6F;   // 6000 Hz, Q=2.5, BW=1000 Hz — for /s z/

  // Per-formant amplitudes (derived from place continuously)
  SmoothedValue<float> a3f, a4f, a6f, bypass;

  float process(float noise) {
    // Alternating-sign summation (eSpeak trick for inter-formant notches)
    float out = F6F.process(noise) * a6f.getNextValue();
    out = F4F.process(noise) * a4f.getNextValue() - out;
    out = F3F.process(noise) * a3f.getNextValue() - out;
    out = bypass.getNextValue() * noise - out;
    return out;
  }
};
```

**Continuous place → amplitude mapping** (replaces current single-BPF logic):

| Place value | Phonetic target | a3f | a4f | a6f | bypass |
|-------------|----------------|-----|-----|-----|--------|
| 0.0 (Labial) | /f θ p/ | 0 | 0 | 0 | **1.0** |
| 0.25 (Dental) | /θ/ | 0 | 0.3 | 0.2 | 0.7 |
| 0.5 (Alveolar) | /s t/ | 0 | 0.5 | **1.0** | 0 |
| 0.75 (Post-alv.) | /ʃ tʃ/ | **1.0** | 0.6 | 0.3 | 0 |
| 1.0 (Velar) | /k/ | 0.5 | 0.4 | 0 | 0 |

Piecewise-linear interpolation between these anchors gives a smooth continuous place axis.

**Integration point:** `FormantVoice.cpp:570-575` — in Cascade/Hybrid branch, replace `consonantFiltered = filterBank.process(consonantNoise)` with `fricationBank.process(consonantNoise)`. Voice path stays on vowel formants.

**Complexity:** ~200 LOC, 1 new file. Low risk — pure additive DSP.
**Parameter additions:** 0 required (internal). Optionally add `fricationBalance` (0–1) to blend old vs new path for preset compatibility safety.

---

#### **Item 2: Plosive Burst Spectral Templates (Stevens-Blumstein)**

**Why:** Perception research shows ~85% of place identification comes from burst shape. Current identical-burst-everywhere is why /p/, /t/, /k/ sound like the same click.

**DSP design:**

Modify `ConsonantEngine::getNextSample()` so when `cachedManner < 0.3` (plosive), the burst portion uses a place-dependent template filter:

```cpp
// In ConsonantEngine:
enum class BurstTemplate { DiffuseFalling, DiffuseRising, Compact };

float applyBurstTemplate(float noise, float place) {
    // Three one-pole filters crossfaded by place
    float lp  = lpFilter.process(noise);   // 6dB/oct @ 800 Hz — labial
    float hp  = noise - lpFilter2.process(noise);  // 6dB/oct HP @ 3.5 kHz — alveolar
    float bp  = bpFilter.process(noise);   // Q=3 @ 2 kHz — velar

    // Place crossfade: 0→labial, 0.5→alveolar, 1→velar
    if (place < 0.5f) {
        float t = place * 2.0f;
        return (1.0f - t) * lp + t * hp;
    } else {
        float t = (place - 0.5f) * 2.0f;
        return (1.0f - t) * hp + t * bp;
    }
}
```

**Integration point:** `ConsonantEngine::getNextSample()` line 152 (burst branch) — wrap `shaped` with `applyBurstTemplate()` when manner<0.3.

**Complexity:** ~50 LOC, no new files.
**Parameter additions:** 0.

---

#### **Item 3: VOT / Aspiration Phase for Voiceless Stops**

**Why:** Voiceless stops without VOT sound "buzzy" — voice kicks in within 25 ms instead of realistic 58–80 ms. This is possibly the single biggest issue user is hearing.

**DSP design:**

Extend ConsonantEngine state machine with an aspiration phase:

```cpp
enum class EnvPhase { Off, Burst, Aspiration, Decay };
// when voicing < 0.5 AND manner < 0.3 (voiceless plosive):
//   Burst (8–15 ms) -> Aspiration (30–60 ms, AH noise through vowel formants) -> Decay
// During Aspiration: glottal source fully suppressed, AH noise fed through cascade bank
//                    F1 bandwidth widened to ~300 Hz (breathy)
```

**Key routing change:** aspiration noise must go through the **cascade bank** (not the frication bank) because it needs to be shaped by the opening vocal tract toward the following vowel — this is how listeners hear the transition into the vowel.

**VOT duration** derived from voicing parameter:
```cpp
float vot_ms = (1.0f - voicing) * 80.0f + 5.0f;  // voiceless=85ms, voiced=5ms
```

**Integration point:** `FormantVoice.cpp:538-544` — during aspiration phase, inject aspiration noise into `tiltedVoice` before `cascadeBank.process()`. Existing `aspirationNoise` module can be reused for this (it already has the breathiness-dependent tilt filter).

**Complexity:** ~100 LOC, extends existing state machine.
**Parameter additions:** 1 optional — `consonantVOT` (0–1, scales aspiration duration 0–80 ms). Default 0.5 = faithful to voicing parameter.

---

#### **Item 4: Voiced-Fricative Noise Gating (Klatt MOD)**

**Why:** /z/, /ʒ/, /v/, /ð/ currently sound like /s/, /ʃ/, /f/, /θ/ + a voice bar tacked on. Real voiced fricatives have **amplitude-modulated noise** synced to F0 — the noise pulses with the glottal cycle.

**DSP design:**

One-line modification in `ConsonantEngine::getNextSample()`:

```cpp
// Gate noise with 50% square wave at F0 when voiced fricative
if (cachedVoicing > 0.5f && cachedManner > 0.3f) {
    float glottalPhase = glottalPhaseRef.load();  // injected from FormantVoice
    float modGate = (glottalPhase < 0.5f) ? 1.0f : 0.5f;
    noise *= modGate;
}
```

**Integration point:** Pass `glottalSource.getPhase()` into `ConsonantEngine::process()` (same mechanism already used by `AspirationNoise::setGlottalPhase()`).

**Complexity:** ~10 LOC.
**Parameter additions:** 0.

---

### ★★★★★ Stretch Goal (defer to v1.24.0 if scope tight)

#### **Item 5: Locus-Based F2/F3 Transitions (Delattre-Liberman)**

**Why:** Biggest remaining realism win. Listeners rely heavily on F2/F3 trajectories for place — more than on bursts in many studies. Completely missing from O-Formant.

**DSP design:**

Add a ConsonantTransition state to FormantVoice. For 40–50 ms after note onset (when in "consonant mode"), bias the VowelMorpher output toward place-specific loci:

```cpp
// Block-rate, after vowelMorpher.compute():
if (consonantTransitionActive) {
    float loci[2] = { computeF2Locus(place), computeF3Locus(place) };
    float locusWeight = std::exp(-transitionSamples / tauSamples);  // τ=15ms
    formantFreqs[1] = locusWeight * loci[0] + (1-locusWeight) * formantFreqs[1];
    formantFreqs[2] = locusWeight * loci[1] + (1-locusWeight) * formantFreqs[2];
    transitionSamples += 32;
}
```

Locus table:
```cpp
float computeF2Locus(float place) {
    // Labial=720, Alveolar=1800, Palatal=2200, Velar=1200 or 3000 (context)
    if (place < 0.33f) return 720 + place/0.33f * (1800-720);
    if (place < 0.67f) return 1800 + (place-0.33f)/0.34f * (2200-1800);
    return 2200 + (place-0.67f)/0.33f * (1200-2200);  // velar back-V default
}
```

**Complexity:** ~150 LOC, new transition state machine.
**Parameter additions:** 1 optional — `transitionAmount` (0–1, scales locus pull; 0 = legacy).

**Why deferred:** requires careful integration with existing `transitionTime` formant smoothing, VowelMorpher output pipeline, and interaction with Singer's Formant + other modulations. Worth its own version.

---

### ★★☆☆☆ Not recommended

#### **Item 6: Aspiration noise injection into cascade bank**
Already proposed as part of Item 3 (VOT). Not a standalone change.

#### **Item 7: Affricate rise time control**
Low impact (only affects /tʃ dʒ/); current 30 ms attack is already reasonable. Skip.

---

## Final Recommendation for v1.23.0

**Implement items 1–4 together** in a single v1.23.0 release:

1. **Frication formant bank + bypass** (new file, ~200 LOC)
2. **Plosive burst templates** (~50 LOC in ConsonantEngine)
3. **VOT / aspiration phase** (~100 LOC, extends state machine)
4. **Voiced-fricative noise gating** (~10 LOC)

**Total:** ~360 LOC, 1 new file, 0–2 new parameters (depending on whether VOT exposed as knob).

**All additive, non-breaking.** Existing presets load unchanged; new behavior kicks in automatically when place/manner/voicing parameters hit the appropriate ranges.

**Defer Item 5 (locus transitions) to v1.24.0** — worth its own focused release because of the VowelMorpher integration complexity.

---

## Sources

### Phonetic Acoustics
- Jongman, Wayland & Wong (2000). Acoustic characteristics of English fricatives. JASA 108:1252.
- Stevens & Blumstein (1978). Invariant cues for place of articulation in stop consonants. JASA 64:1358.
- Blumstein & Stevens (1979). Acoustic invariance in speech production. JASA 66:1001.
- Kewley-Port (1982). Measurement of formant transitions in naturally produced stop CV syllables. JASA 72:379.
- Lisker & Abramson (1964). A cross-language study of voicing in initial stops. Word 20:384.
- Klatt (1975). Voice onset time, frication, and aspiration in word-initial consonant clusters. JSLHR 18:686.
- Delattre, Liberman & Cooper (1955). Acoustic loci and transitional cues for consonants. JASA 27:769.
- Suen & Beddoes (1974). The silent interval of stop consonants. Language and Speech 17:126.
- Howell & Rosen (1983). Production and perception of rise time in the voiceless affricate/fricative distinction. JASA 73:976.
- Stevens (1998). Acoustic Phonetics. MIT Press.

### Synthesis Architecture
- Klatt (1980). Software for a cascade/parallel formant synthesizer. JASA 67:971.
- Weenink (2009). The KlattGrid speech synthesizer. Interspeech 2009.
- Praat KlattGrid manual: https://www.fon.hum.uva.nl/praat/manual/KlattGrid.html

### Open-Source Implementations
- Pink Trombone (Neil Thapen): https://github.com/zakaton/Pink-Trombone
- eSpeak NG Klatt: https://github.com/espeak-ng/espeak-ng/blob/master/src/libespeak-ng/klatt.c
- Praat KlattGrid source: https://github.com/praat/praat/blob/master/dwtools/KlattGrid.cpp
- Gnuspeech TRM: https://github.com/mym-br/gnuspeech_sa
- Klatt TypeScript port (reference): https://github.com/chdh/klatt-syn
