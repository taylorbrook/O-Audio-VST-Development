# O-simplePhysicalModelSynth - Requirements

---
version: 1.0.0
plugin: O-simplePhysicalModelSynth
created: 2026-06-26
lastUpdated: 2026-06-27
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 24
**Coverage:** must: 16 | should: 6 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Excitation → resonator → material signal chain is the playable structure (energy injected, carried, then damped) | must | complete | stage-2 |
| FUNC-02 | Swappable excitation: Pluck (noise burst), Strike (impulse/mallet), Bow (sustained friction) | must | complete | stage-2 |
| FUNC-03 | Two-way resonator switch: String (Karplus-Strong/waveguide) and Modal (decaying sinusoids) | must | complete | stage-2 |
| FUNC-04 | Same exciters drive either resonator (hold exciter, swap resonator — and vice versa) | must | complete | stage-2 |
| FUNC-05 | 16-voice polyphony; plays chords and sustains | should | complete | stage-2 |
| FUNC-06 | Velocity scales excitation strength/brightness (model responds to playing dynamics) | should | complete | stage-2 |
| FUNC-07 | Concept-isolating preset tour (bright steel, muted nylon, koto/harp, struck bar, bell, bowed string) | should | complete | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Karplus-Strong string: N-sample delay + one-pole low-pass + feedback loop; pitch = SR ÷ N | must | complete | stage-2 |
| DSP-02 | Fractional-delay tuning (all-pass interpolation) so notes stay in tune across the keyboard | must | complete | stage-2 |
| DSP-03 | Material/damping = loop low-pass cutoff (Damping) + feedback gain (Decay), feedback clamped < 1 for stability | must | complete | stage-2 |
| DSP-04 | Modal resonator: bank of decaying sinusoids with per-mode frequency/amplitude/decay; higher modes quieter and faster-decaying | must | complete | stage-2 |
| DSP-05 | Inharmonicity stretches modal mode spacing from harmonic (bar) toward inharmonic (bell) | must | complete | stage-2 |
| DSP-06 | Waveguide string option (dual traveling-wave delays) enabling meaningful Excitation Position | nice | deferred | v1.1 |
| DSP-07 | Material macro maps a single steel↔nylon axis onto co-moving (cutoff, feedback) | should | complete | stage-2 |
| DSP-08 | DC-safe, band-limited excitation (no click/buzz/alias); Bow drive stable and sustaining | must | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Single clear page, classroom/projector-readable, laid out as the signal flows (excitation → resonator → material → output) | should | complete | stage-3 |
| UI-02 | Animated loop/flow diagram reflecting ACTUAL circulating energy, re-skinning per resonator (KS loop / waveguide / modal stems) and visibly dampening each pass | must | complete | stage-3 |
| UI-03 | Live waveform + decay scope | should | complete | stage-3 |
| UI-04 | Live spectrum/spectrogram showing harmonics fade top-down (and inharmonic spacing in Modal) | must | complete | stage-3 |
| UI-05 | Modal stem display (mode frequencies + decay rates) in Modal mode | should | complete | stage-3 |
| UI-06 | On-hover pedagogical tooltips on every control | nice | complete | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations in processBlock); visualization data via lock-free FIFO | must | complete | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | complete | stage-1 |
| COMPAT-02 | Windows WebView2 flags set (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) | must | complete | stage-1 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (no clicks, buzz, aliasing, or runaway feedback) across parameter ranges and the keyboard | must | complete | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Excitation → resonator → material chain

**Description:** The plugin makes the class's unifying mental model directly playable.

**Acceptance Criteria:**
- [ ] Injecting energy (any exciter) produces a tone that rings and decays through the resonator
- [ ] Material/damping controls audibly change how the energy is colored and how long it sustains
- [ ] Signal flow is legible: a user can point to where energy enters, is carried, and is lost

### FUNC-02: Swappable excitation (Pluck / Strike / Bow)

**Acceptance Criteria:**
- [ ] Pluck produces a plucked-string attack from a noise burst
- [ ] Strike produces a struck/mallet attack from an impulse
- [ ] Bow produces a sustained tone (does not decay while the note is held)
- [ ] Swapping the exciter with the resonator unchanged audibly changes only the attack/drive

### FUNC-03 / FUNC-04: Resonator switch and cross-driving

**Acceptance Criteria:**
- [ ] String mode produces harmonic, plucked/struck/bowed string timbres
- [ ] Modal mode produces inharmonic struck-bar/bell timbres
- [ ] Each exciter can drive each resonator (the same Strike drives a struck string and a struck bell)

### DSP-01 / DSP-02: Karplus-Strong + tuning

**Acceptance Criteria:**
- [ ] Fundamental tracks the played MIDI note (delay length sets pitch)
- [ ] Notes are in tune across the keyboard (measured fundamental within a few cents — fractional-delay tuning verified by an offline render-harness pitch probe)
- [ ] Lowering Damping and Decay turns a bright steel string into a muted nylon one

### DSP-03 / DSP-08 / QUAL-01: Material and stability

**Acceptance Criteria:**
- [ ] Feedback gain stays below 1 — no runaway/clipping at long Decay settings
- [ ] No clicks at note-on, no DC offset, no aliasing buzz at high notes
- [ ] Damping (low-pass) audibly darkens the decay tail

### DSP-04 / DSP-05: Modal resonator

**Acceptance Criteria:**
- [ ] Struck modal body starts bright and settles onto its lowest mode
- [ ] Inharmonicity at 0% reads bar-like (near-harmonic); at high settings reads bell-like (inharmonic)
- [ ] Modal stem display matches the modes actually sounding

### UI-02 / UI-04: Headline visualizations

**Acceptance Criteria:**
- [ ] Loop/flow diagram animates from the real loop state and visibly loses energy each pass
- [ ] Diagram re-skins between KS loop, waveguide, and modal stems with the resonator switch
- [ ] Spectrum shows upper harmonics dying before lower ones during a plucked decay

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01, COMPAT-02 |
| stage-2 | FUNC-01..06, DSP-*, PERF-01, QUAL-01 |
| stage-3 | UI-* |
| stage-4 | FUNC-07, COMPAT-*, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Blow / tube resonator (winds, brass) | Bow chosen over blow for v1; tube waveguide is the natural next step (O-Wind/O-Reed cover production winds) | v1.1+ |
| Sculpture-level component modeling (movable pickups, multi-string) | Would break the legible single-chain teaching design | future |
| Sympathetic/coupled strings, body convolution | Beyond the irreducible teaching core | future |
| Built-in effects (reverb/delay/chorus) | Keep the signal path transparent | n/a |
| Deep modulation matrix / LFOs | Only velocity→brightness needed for the dynamics lesson | future |

---
*Generated from BRIEF.md on 2026-06-26*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
