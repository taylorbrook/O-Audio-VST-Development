# O-simpleSubtractive - Requirements

---
version: 1.0.0
plugin: O-simpleSubtractive
created: 2026-06-25
lastUpdated: 2026-06-25
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 24
**Coverage:** must: 14 | should: 8 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Subtractive voice: a harmonically rich oscillator is shaped by a resonant multi-mode filter and a VCA, with one ADSR on the filter and one on the amp | must | pending | stage-2 |
| FUNC-02 | Oscillator waveform selectable (sawtooth / square / triangle / sine), each giving the filter a different amount of harmonic material | must | pending | stage-2 |
| FUNC-03 | Sub-oscillator (octave-down) and white-noise source mixable under the main oscillator | should | pending | stage-2 |
| FUNC-04 | Filter envelope routed to cutoff through a bipolar env-amount; amp envelope drives the VCA — two independent ADSRs doing two different jobs | must | pending | stage-2 |
| FUNC-05 | 16-voice polyphony with a Mono/Legato mode and glide (portamento) for classic lead/bass | should | pending | stage-2 |
| FUNC-06 | Concept-preset tour (Saw→LP Sweep, Acid Bass 303, Brass Lead, Pluck, Sweep Pad, Self-Oscillation Sine, Hollow Square Bass, Filtered Noise) loadable as factory presets | should | ✅ done | stage-4 |
| FUNC-07 | Playable/musical enough to double as a simple subtractive instrument (bass, lead, pluck, pad) | nice | ✅ done | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Band-limited oscillator (saw/square/triangle/sine) + octave-down sub + white noise → mixer, per voice | must | pending | stage-2 |
| DSP-02 | Multi-mode filter (LP/HP/BP/Notch) at selectable 6/12/24 dB/oct slopes, with cutoff and resonance controls | must | pending | stage-2 |
| DSP-03 | Filter self-oscillates at high resonance (clean sine at the cutoff with no input), with gain compensation so resonance sweeps don't blow up level | must | pending | stage-2 |
| DSP-04 | Two per-voice ADSR envelopes: filter (to cutoff via bipolar env amount) and amplitude (to VCA), zipper-free under modulation | must | pending | stage-2 |
| DSP-05 | Cutoff key-tracking so a self-oscillating filter can be played in tune | should | pending | stage-2 |
| DSP-06 | Anti-aliasing on saw/square (PolyBLEP or band-limited tables) so high notes stay clean | must | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Live filter-response-over-spectrum display: the oscillator's harmonic spectrum with the filter's true magnitude curve overlaid, updating as cutoff/resonance/slope/type and the filter envelope move (the headline "before/after filter" visual) | must | pending | stage-3 |
| UI-02 | Animated dual-ADSR display showing the filter envelope (to cutoff) and amp envelope (to level) over a note on independent scales | should | pending | stage-3 |
| UI-03 | Live signal-path diagram (oscillator → filter → VCA, envelopes routing to cutoff and VCA) with the active stage highlighted | should | pending | stage-3 |
| UI-04 | Oscilloscope of the output waveform morphing as cutoff/resonance/envelopes change | should | pending | stage-3 |
| UI-05 | On-hover pedagogical tooltips on every control (cutoff, resonance, poles/slope, filter-env vs amp-env, sub/noise) | should | pending | stage-3 |
| UI-06 | Single-page, projector-readable layout laid out left-to-right as the signal path, consistent with O-simpleFM / O-simpleAdditive | must | pending | stage-3 |
| UI-07 | Preset tour selectable from the UI | should | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations/locks in processBlock); visualization data crosses to UI via lock-free handoff | must | pending | stage-2 |
| PERF-02 | 16-voice polyphony renders without dropouts at typical buffer sizes | should | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | pending | stage-1 |
| COMPAT-02 | Windows WebView2 configured correctly (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`); UI renders on Windows | should | pending | stage-3 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (aliasing, zipper, clicks, filter blow-up) at normal parameter ranges and across the keyboard | must | pending | stage-2 |
| QUAL-02 | The filter-response curve and spectrum accurately reflect what is heard (visual matches audio) | should | pending | stage-3 |

## Acceptance Criteria Details

### FUNC-01: Subtractive voice (osc → filter → amp)

**Description:** A harmonically rich oscillator is carved by a resonant filter and shaped by a VCA, with independent filter and amp envelopes.

**Acceptance Criteria:**
- [ ] A note plays oscillator → filter → VCA; lowering cutoff audibly darkens the tone
- [ ] The filter envelope and amp envelope can be set independently (a patch can be bright-but-quiet or loud-but-dark)
- [ ] Tone is correctly pitched to the incoming MIDI note number

### FUNC-02: Oscillator waveforms

**Acceptance Criteria:**
- [ ] Saw / square / triangle / sine are each selectable and audibly distinct
- [ ] Saw is brightest (all harmonics), square is hollow (odd only), triangle is dark, sine is a single partial
- [ ] The selected waveform's harmonic content shows in the spectrum display

### FUNC-04: Two independent envelopes

**Acceptance Criteria:**
- [ ] Filter envelope opens/closes the cutoff by the bipolar env-amount; negative amount inverts the sweep
- [ ] Amp envelope shapes loudness independently (percussive pluck vs slow swell)
- [ ] Both envelopes are visible in the dual-ADSR display and audibly independent

### DSP-02: Multi-mode, multi-slope filter

**Acceptance Criteria:**
- [ ] LP/HP/BP/Notch each produce the correct response shape
- [ ] 6/12/24 dB/oct slopes are audibly and visibly steeper in turn; the same cutoff reads darker at 24 dB than at 6 dB
- [ ] Cutoff and resonance respond smoothly across their full range

### DSP-03: Self-oscillation

**Acceptance Criteria:**
- [ ] At maximum resonance the filter produces a clean sine at the cutoff with no input
- [ ] Resonance sweeps do not produce runaway level (gain compensation works)

### DSP-06: Anti-aliasing

**Acceptance Criteria:**
- [ ] Saw and square produce no audible aliasing at high notes (e.g. C7+)
- [ ] Band-limiting does not dull the tone at normal playing range

### UI-01: Filter-response-over-spectrum (headline visual)

**Acceptance Criteria:**
- [ ] The oscillator's harmonic spectrum is drawn with the filter's magnitude curve overlaid
- [ ] Lowering cutoff visibly attenuates harmonics above it; raising resonance grows a peak at the cutoff
- [ ] The curve slides in real time as the filter envelope sweeps the cutoff

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [ ] No allocations, locks, or file I/O in processBlock
- [ ] Visualization data reaches the UI via a lock-free FIFO/atomic handoff

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01/02/03/04/05, DSP-*, PERF-*, QUAL-01 |
| stage-3 | UI-*, COMPAT-02, QUAL-02 |
| stage-4 | FUNC-06/07, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Second full oscillator + detune / supersaw | Sub-osc + noise cover weight/texture without a 2nd osc | v1.1+ |
| Modulation matrix / multiple LFOs / dedicated vibrato | Two envelopes are the v1.0 modulation story | v1.1+ |
| Effects (reverb/delay/chorus) | Keep signal path transparent for teaching | out |
| Deep filter-shapes dive (all EQ shapes) | The EQ/filters session owns that; LP is the focus here | out |
| Unison / arpeggiator / step sequencer | Referenced (303) but out of pedagogical core | out |

---
*Generated from BRIEF.md on 2026-06-25*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
