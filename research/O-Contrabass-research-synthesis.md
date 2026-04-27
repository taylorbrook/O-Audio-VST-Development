---
title: "O-Contrabass: Research Synthesis"
created: 2026-04-25
juce_version: "8.0.4"
type: research-synthesis
domain: dsp
plugin: O-Contrabass
keywords:
  - physical-modeling
  - bowed-string
  - contrabass
  - double-bass
  - waveguide
  - friction-model
  - drone
  - subharmonics
  - infinite-sustain
  - inharmonicity
  - body-resonator
  - microtonal
  - mts-esp
  - vst3-note-expression
stages: [0]
agents: [research]
source_documents:
  - research/bow-string-friction-models.md
  - research/O-Bowed-research-synthesis.md
  - research/O-Contrabass-bass-waveguide-stability.md
  - research/O-Contrabass-body-acoustics.md
  - research/O-Contrabass-drone-and-subharmonics.md
---

# O-Contrabass: Research Synthesis

Consolidated DSP research for O-Contrabass — a bass-only physical-modeling bowed-string plugin (E1–G3) with first-class drone features, generated from three parallel Level 3 research agents on top of the existing O-Bowed research foundation.

---

## 1. Strategic Direction

O-Contrabass is **bass-only by design**. Every DSP decision is tuned for the E1–G3 fundamental range and sustained / drone-first articulation. This is the inverse of O-Bowed (general-purpose, configurable, presets). The architecture intentionally specializes:

| Decision | O-Bowed | O-Contrabass |
|----------|---------|--------------|
| Pitch range | violin → bass → erhu | E1–G3 only |
| Body | morphable (membrane/wood/metal/glass) | fixed wood, parametric size + damping |
| Strings | configurable 1–4 + sympathetic | fixed 4-string EADG, no sympathetic |
| Friction tuning | generic across instruments | thick rosined hair on bass strings |
| Drone features | listed under "impossible physics" (optional) | first-class design pillar |
| Articulation | many | sustained-first (legato held + release tail) |

**The dual identity** the brief calls out — "Cinematic orchestral arco" + "Stephen O'Malley / Tony Conrad drone" — is satisfied by parameter ranges and presets on a single engine, not by mode switching.

---

## 2. Architecture (consolidated)

```
MIDI / MPE / VST3 Note Expression
    │
    ▼
[Bow Model] ── Speed, Pressure, Position (CC11/CC2/CC74)
[Vibrato Section] ── 5 Hz / 12¢ / 600 ms onset (Mick 2025 bass averages)
[Slow Bow LFO] ── 0.05–2 Hz Schelleng-aware diagonal modulation
    │
    ▼
[Friction Junction]  ←──→  [4× String Waveguides E-A-D-G]
  ─ Hyperbolic curve (core)             │
  ─ Optional elasto-plastic (attack)    │
  ─ Period-doubling biased for          │
    sub-harmonics                       │
    │                                   │
    │   ┌─ Stiffness allpass dispersion (4→1 sections per string)
    │   ├─ Lagrange3rd fractional delay (NOT Thiran)
    │   ├─ Per-string detune ±1200¢ via SmoothedValue (20 ms ramp)
    │   └─ Bridge LP filter (loss + body coupling)
    ▼
[In-Loop Soft Saturator] ── algebraic, prevents drone runaway
    │
    ▼
[8-Mode Body Resonator] ── parallel biquads, 3/4 bass tuned
  A0 60 Hz ─ T1 98 Hz ─ B1+ 115 Hz ─ Cluster 175/235/340 ─ Bridge 700 ─ Hill 1.2k
  Body Size: f / (0.85 + 0.30·s) frequency scaling, Q invariant
  Wolf re-injection: optional toggle for authentic arco
    │
    ▼
[Bow Noise Bed] ── 3-band BPF 700/1500/3000 Hz, modulated by |v|·F
[Sub-Harmonic Component] ── friction-junction operating point shift
    │
    ▼
[Master Polynomial Saturator] + [Feedforward Peak Limiter -1 dBFS] (zero-latency)
    │
    ▼
[Width / Output] → Stereo Out
```

---

## 3. Friction Model

**Selected:** Enhanced hyperbolic bow table (core, always-on) with optional elasto-plastic enhancement for attack quality. Detailed coverage already in `bow-string-friction-models.md`. Bass-specific tuning:

| Parameter | Bass-tuned default | Rationale |
|-----------|--------------------|-----------|
| `mu_s` | 0.85 | Slightly higher than violin — heavier rosin grip on thick strings |
| `mu_d` | 0.25 | Lower than violin — wider stick-slip excursion at low fundamentals |
| `v_0` | 0.05 m/s | Larger than violin defaults — bow speeds are slower for bass |
| Default `bow_speed` | 0.15 m/s | Matches BRIEF (slower than violin's 0.3) |
| Default `F_bow` | 1.0 N | Matches BRIEF (heavier than violin's ~0.5) |
| Default `beta` | 0.10 | Slightly closer to bridge for fundamental richness |
| Bow table `slope` | 5.0 − 4.0 · normPressure | Standard STK mapping, retuned at extremes |

**Sub-harmonic generation uses the same friction junction** — biased into period-doubling regime by raising `F_bow` toward `F_max` and tightening `v_0`. No separate generator. The "Sub-Harmonics 0-100%" knob is an operating-point shift, not a parallel module.

---

## 4. String Waveguide

### 4.1 Stiffness Dispersion (lightweight)

Inharmonicity is mostly absorbed by the friction nonlinearity (stick-slip phase-locks the string into harmonic ratios), so dispersion is a *character* effect rather than a tuning correction.

**Implementation:** Cascaded first-order allpass sections per string (Rauhala/Välimäki 2006 closed-form coefficient).

| String | Inharmonicity B | Allpass sections |
|--------|------------------|-------------------|
| E1 (41 Hz) | ~1e-4 | 4 |
| A1 (55 Hz) | ~7e-5 | 3 |
| D2 (73 Hz) | ~5e-5 | 2 |
| G2 (98 Hz) | ~3e-5 | 1 |

### 4.2 Fractional Delay

**Use Lagrange 3rd-order FIR interpolation** (stateless) — NOT Thiran allpass. Thiran is stateful and clicks on detune automation; Lagrange is glitch-free for the BRIEF's ±1200¢ per-string detune requirement.

### 4.3 Smoothing & Stability

- `juce::SmoothedValue<float, ValueSmoothingTypes::Linear>` per parameter, 20 ms ramp
- `juce::ScopedNoDenormals` at top of `processBlock`
- 1e-20 constant leak in bridge filter (disabled in pure drone mode)
- `float` delay-line buffers, `double` for friction-junction state and phase accumulators

### 4.4 Oversampling

**2x at the friction junction** is sufficient (polyphase IIR halfband). Bass-register slow attacks reduce aliasing pressure vs violin. CPU budget projection: ~3.2% on M1, well under PERF-02's 5% target.

---

## 5. Body Resonator (8-Mode Wood)

**Anchored to published double-bass acoustics** (Askenfelt KTH 1982, Rossing 2010, Bissinger). Default 3/4 size:

| Mode | Frequency | Q | Role |
|------|-----------|---|------|
| A0 (Helmholtz / F-hole) | 60 Hz | ~14 | Air resonance, fundamental fullness |
| T1 / B1− (main wood) | 98 Hz | ~11 | **Wolf seat** — coincident with G2 fundamental |
| B1+ | 115 Hz | ~10 | Body cluster bottom |
| Cluster mode 1 | 175 Hz | ~8 | Quality band start (luthier-cited 150–400 Hz) |
| Cluster mode 2 | 235 Hz | ~7 | |
| Cluster mode 3 | 340 Hz | ~6 | Quality band top |
| Bridge cluster | 700 Hz | ~5 | |
| Bridge hill | 1200 Hz | ~3 | Bass analog of violin's 2.5 kHz BH |

### 5.1 Body Size scaling

```
f_scaled = f_default / (0.85 + 0.30 · size)
```

This produces a **1.83:1 frequency span** from "1/4 child bass" to "4/4 jumbo." **Q does NOT scale** — size-independent in real wood. Click-free coefficient updates use per-block recomputation + 30 ms parameter smoothing.

### 5.2 Body Damping

Uniformly narrows mode bandwidths: `Q_eff = Q · (1 − 0.85 · damping)` (×0.15 at full damp).

### 5.3 Body Mix

Parallel wet/dry of body filter vs raw string. Use a **35 Hz HP on the dry path** to prevent sub-A0 phase combing. The body chain itself dominates below 80 Hz where the raw string contributes mostly delay/phase rather than amplitude.

### 5.4 Wolf Tone Region (F2–B2 / 82–117 Hz)

Two-mode design:
- **Default:** Q-modulation suppression at the T1 mode when fundamental locks within ±15¢
- **"Authentic Arco" toggle:** Re-inject mode T1 output back into the bridge termination (energy coupling that produces the real wolf beating effect)

---

## 6. Bow Noise Generator

**3-band BPF bed** (700 Hz, 1.5 kHz, 3 kHz) modulated by `|v_bow| · F_bow`, plus per-period slip bursts and bow-change transients. Bass close-mic recordings show the texture sits in this 500 Hz–4 kHz band — significantly lower than violin's 1–8 kHz noise band.

User-facing as **"Bow Noise 0-100%"** (BRIEF default 35% — heavier than O-Bowed's default to support the "intimate close-mic realism" sonic target).

---

## 7. Drone & Sub-Harmonic Features

### 7.1 Sub-Harmonics (DSP-07)

**Selected approach: friction-junction operating-point shift** — physically authentic period-doubling bifurcation (Hanson, Guettler, Kawano ALF physics).

The "Sub-Harmonics 0-100%" knob biases:
- `F_bow` toward `F_max` (Schelleng upper bound)
- `v_0` tighter (sharper friction transition)
- `mu_s/mu_d` ratio wider

Why not the alternatives:
- Octave-down ring modulation: not physical, produces beating with the fundamental
- Allpass-comb sub-octave filter: not bowing-correlated, sounds like an effect
- Parallel sub-waveguide: doubles CPU, no integration with body / vibrato

### 7.2 Infinite Sustain (DSP-06)

**Quadratic loop-gain curve from 0.997 to 0.99995** (NEVER 1.0), backed by:
- Hard ceiling at 0.9999999
- In-loop DC blocker
- FTZ/DAZ via `juce::ScopedNoDenormals`
- Opposite-sign denormal guard
- Soft state-clamp at the friction junction (limit string state magnitude when bow disengages)

This satisfies QUAL-01 (no NaN/runaway) and QUAL-02 (musical self-oscillation) at max settings.

### 7.3 Slow Bow LFO (DSP-08)

**Schelleng-aware diagonal modulation** of `v_b` and `F_bow` simultaneously, with ~23° pressure phase lag (mimics natural arm motion). LFO depth dynamically scaled to **80% of remaining wedge headroom** — the LFO trajectory always stays inside the playable region. 20 ms smoothing across LFO zero-crossings prevents friction-junction artifacts.

### 7.4 Vibrato (DSP-09)

**Bass vibrato defaults: 5 Hz rate, 12 cents depth, 600 ms onset** — validated by Mick (2025) "An Analysis of Double Bass Vibrato": professional bass mean 5.17 Hz, 19 cents. The BRIEF's 12 cents default is on the gentler side, appropriate for orchestral / drone aesthetic. 300 ms S-curve fade-in. Modulates delay-line length via Lagrange interpolation (same path as detune).

### 7.5 Output Protection (QUAL-02)

- **In-loop:** algebraic soft saturator on string state
- **Post-bridge:** polynomial soft saturator
- **Master:** zero-latency feedforward peak limiter at -1 dBFS

Zero algorithmic latency goal (PERF-03) preserved — feedforward limiter has no look-ahead.

---

## 8. Microtonal & Expression (Ouaricon Convention)

| Channel | Implementation |
|---------|----------------|
| **VST3 Note Expression** | spike-validated pattern from `spike-findings-VST-development` skill (FUNC-06, COMPAT-02) |
| **MTS-ESP** | runtime retuning (FUNC-07) |
| **Scala/TUN** | static tuning import (FUNC-07) |
| **MPE** | pitch / pressure (Z) / slide (Y) — bow expression in real time (FUNC-05) |
| **CC11** | bow speed (intrinsic) |
| **CC2 / aftertouch** | bow pressure |
| **CC74** | bow position (β) |
| **Vibrato section** | rate / depth / onset — independent of MIDI input |
| **Expression Macro** | single knob: bow speed + pressure + vibrato + body brightness |

The microtonal channels coexist (priority: Note Expression > MTS-ESP > Scala > MPE pitch-bend > 12-TET) and integrate with per-string detune for scordatura / just intonation drone presets.

---

## 9. Performance Projection

| Metric | Target | Projected | Notes |
|--------|--------|-----------|-------|
| CPU (typical) | <5% | ~3.2% (M1) | Per CPU budget calculation in waveguide-stability doc |
| Oversampling | 2x at junction | 2x polyphase IIR | Sufficient for bass register |
| Latency | 0 algorithmic | 0 | Causal waveguide + zero-latency limiter |
| Polyphony | 1 | 1 | Mono by design |
| Memory | <2 MB / voice | ~1.5 MB | Delay lines + LUTs + filter states |

---

## 10. Module Extraction Opportunity

The existing O-Bowed plugin contains a friction junction module that could be **extracted as a shared Ouaricon module** (per BRIEF technical notes). Candidate boundaries:

```
ouaricon_bow_friction/
  ├── HyperbolicBowTable
  ├── ElastoPlasticFriction (optional)
  ├── ThermalFriction (optional, "quality" tier)
  ├── BowState envelope
  └── SchellengGuard (F_min / F_max computation)
```

Decision deferred to planning phase. If extracted, both O-Bowed and O-Contrabass consume it; O-Contrabass adds bass-tuned defaults + period-doubling sub-harmonic biasing on top.

---

## 11. Implementation Roadmap (Stage 0 → Stage 4)

### Stage 1: Foundation
- JUCE plugin skeleton, parameters (APVTS), CMake project
- VST3 + AU build targets
- pluginval strictness 10 baseline

### Stage 2: DSP
- Single-string waveguide (E string first), bridge LP, hyperbolic friction
- Stiffness dispersion (4→1 allpass sections per string)
- Lagrange3rd fractional delay
- Per-string detune ±1200¢ with smoothing
- 4-string E-A-D-G + bow-string switching logic
- Bass-tuned friction defaults
- 8-mode wood body resonator + Body Size scaling
- Bow noise generator (3-band)
- Slow attack / sustained envelope
- Vibrato section + Slow Bow LFO
- Sub-harmonic friction biasing
- Infinite Sustain damping curve
- In-loop saturator + master limiter
- VST3 Note Expression + MTS-ESP + MPE
- 2x oversampling at friction junction

### Stage 3: GUI (TBD in mockup phase)
- WebView UI per BRIEF section groupings
- Dark wood / stage-lit aesthetic (suggested)

### Stage 4: Verification & Polish
- Orchestral preset bank (5 presets)
- Drone / experimental preset bank (5 presets)
- Dorico microtonal playback verification
- Cross-DAW testing
- 60+ second sustain stability
- Schelleng diagram visualization (optional)

---

## 12. Open Decisions for Planning Phase

1. **Friction tier to ship in v1.0**: Hyperbolic only? Or hyperbolic + elasto-plastic toggle?
2. **Module extraction**: Extract `ouaricon_bow_friction` from O-Bowed before or during Stage 2?
3. **Wolf-tone "Authentic Arco" toggle**: Include in v1.0 or defer to v1.1?
4. **Sub-harmonic depth at max**: How far into the period-doubling regime is musical (1 octave down? 2?)
5. **Body Size knob mapping**: 0–100% UI range maps to which physical bass-size span (1/4 → 4/4 vs 1/2 → full)?
6. **Wood material variants**: Single fixed wood, or 2–3 wood character presets (spruce-bright vs maple-warm)?

---

## 13. References (consolidated)

### Existing Repo Documents
- `research/bow-string-friction-models.md` — friction theory & DSP
- `research/O-Bowed-research-synthesis.md` — general bowed-string framework
- `research/O-Contrabass-bass-waveguide-stability.md` — low-fundamental stability + stiffness
- `research/O-Contrabass-body-acoustics.md` — 8-mode wood body resonator
- `research/O-Contrabass-drone-and-subharmonics.md` — drone features & subharmonic generation
- `troubleshooting/dsp-issues/physical-modelling-synthesis-complete-guide.md`
- `spike-findings-VST-development` skill (Note Expression pattern)

### Foundational Acoustics
- Askenfelt, A. "Eigenmodes of the Double Bass" (KTH STL-QPSR 1982).
- Rossing, T. (Ed.) (2010). *The Science of String Instruments*, Ch. 15.
- Bissinger, G. — bridge-hill measurements on bowed-string family.
- Smith, J.O. *Physical Audio Signal Processing* — Bowed Strings chapter.

### Subharmonic Bowing
- Hanson, Schumacher, Macomber. "The Violinist's Sound Palette: Spectral Centroid, Pitch Flattening, and Anomalous Low Frequencies." (Hanson ALF papers).
- Kawano et al. (2025). "Experimental Validation of String Oscillation in Subharmonic Generation." arXiv:2502.11902.
- Guettler. "The Violin Bow in Action: A Sound-Sculpturing Wand."

### Bass Vibrato & Performance
- Mick, J. (2025). "An Analysis of Double Bass Vibrato." String Research Journal.

### DSP Implementation
- Rauhala & Välimäki (2006). "Tunable dispersion filter design for piano synthesis."
- Bensa, J., Bilbao, S., Kronland-Martinet, R., Smith, J.O. (2003). "The simulation of piano string vibration."
- Karjalainen, M., Välimäki, V., Tolonen, T. (1998). "Plucked-string models: From the Karplus-Strong algorithm to digital waveguides."
- Smith — DC Blocker (CCRMA).
- Willemsen, S. & Bilbao, S. (2019). "Real-time elasto-plastic friction model." DAFx.

### Schelleng & Playability
- Schelleng, J.C. (1973). "The bowed string and the player." JASA.
- "Mapping Playability: The Schelleng Diagram." SMAC 2023.
- Euphonics 9.3 — Schelleng's Diagram.

### Commercial Reference
- AAS String Studio VS-3 manual.
- SWAM Double Bass.
- Madrona Labs Aalto.
