# O-simpleFM - Creative Brief

## Overview

**Type:** Synth (FM Synthesizer)
**Core Concept:** A deliberately simple 2-operator FM synthesizer built for pedagogy — teaching how FM synthesis works and letting students learn the essential common parameters by hearing AND seeing them.
**Status:** 💡 Ideated
**Created:** 2026-06-20

## Vision

O-simpleFM is a teaching instrument first and a synth second. Most FM synths (DX7, Operator, FM8) overwhelm beginners with 4–6 operators, dozens of algorithms, and deep menus — the very complexity that makes FM feel impenetrable. O-simpleFM strips FM down to its irreducible core: **one carrier, one modulator, a ratio, and a modulation index.** From those few controls a student can directly experience the central insight of FM — that a simple frequency ratio plus a modulation depth generates a whole spectrum of sidebands.

The pedagogical payload is the tight loop between **gesture and visible consequence.** Turn up the modulation index and watch sidebands bloom in the live spectrum analyzer. Change the C:M ratio from 1:1 to 2:1 to 1.41:1 and watch harmonic content snap from harmonic to inharmonic while the oscilloscope waveform morphs in real time. Give the modulator its own envelope and hear/see the timbre evolve from bright attack to mellow sustain — the single most important expressive trick in FM. Add feedback and watch the spectrum smear toward sawtooth and noise.

Every control is annotated: hovering any parameter surfaces a short, plain-language explanation of what it does and why ("Ratio 2:1 → octave-related harmonics; integer ratios = harmonic, irrational ratios = bell-like"). A live operator-routing diagram keeps the signal flow visible at all times so students build an accurate mental model rather than memorizing knob positions. An educational preset tour offers a handful of named patches that each isolate one concept, inviting students to reverse-engineer classic FM sounds (bell, brass, e-piano) from the minimal control set.

The design north star: a curious student should reach a genuine "oh, THAT's how FM works" moment within five minutes, with no manual.

## Architecture

**2-operator FM with feedback** (the minimal topology that still teaches the full core concept):

```
        ┌──fb──┐
        ▼      │
      [MOD] ───┘ ──FM──► [CAR] ──► amp env ──► out
        │                  │
     mod env            amp env
```

- **Carrier (CAR):** the operator you hear; its amplitude is shaped by the amp envelope.
- **Modulator (MOD):** frequency-modulates the carrier; its frequency = carrier × ratio; its output level is scaled by the modulation index and shaped by the mod envelope.
- **Feedback:** the modulator feeds back into itself, progressively enriching/roughening the spectrum (approaches sawtooth, then noise, at high settings).

## Parameters

*Core set defined here; Stage 0 research should surface and confirm additional standard FM parameters (see Technical Notes). Ranges below are starting proposals to be validated in planning.*

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Ratio (C:M) | 0.5–16 (fine + integer-snap option) | 1.0 | Modulator frequency as a multiple of carrier. Integer = harmonic, non-integer = inharmonic. THE defining FM control. |
| Modulation Index | 0–~20 | 0 | Depth of frequency modulation; controls sideband count / brightness. The other defining FM control. |
| Feedback | 0–100% | 0 | Modulator self-feedback; enriches/roughens spectrum toward saw/noise. |
| Mod Env Attack | 0–5 s | short | Modulator-envelope attack — shapes how timbre brightens in. |
| Mod Env Decay | 0–5 s | medium | Modulator-envelope decay. |
| Mod Env Sustain | 0–100% | 80% | Modulator-envelope sustain level (timbre held during note). |
| Mod Env Release | 0–5 s | short | Modulator-envelope release. |
| Amp Attack | 0–5 s | short | Carrier amplitude attack. |
| Amp Decay | 0–5 s | medium | Carrier amplitude decay. |
| Amp Sustain | 0–100% | 80% | Carrier amplitude sustain. |
| Amp Release | 0–5 s | short | Carrier amplitude release. |
| Carrier Waveform | sine / (tri / saw / square?) | sine | Carrier shape; sine default keeps FM math clean for teaching. |
| Modulator Waveform | sine / (tri / saw / square?) | sine | Modulator shape; sine default. |
| Output Level | -inf–0 dB | 0 dB | Master output gain. |

**Likely additions from research (to confirm in Stage 0):** mod-envelope-to-index amount, velocity → modulation index, fixed-frequency vs ratio mode for the modulator, fine detune, key tracking of index, master tuning, polyphony/voice count, optional LFO/vibrato.

## UI Concept

*Captured from user-volunteered direction; full UI design happens in the mockup phase, not here.*

**Layout:** A single clear page (no deep menus). Controls grouped as: Operators/Ratio + Index + Feedback | Modulator Envelope | Amp Envelope | Waveforms/Output. Visualizations occupy a prominent live panel.

**Visual Style:** Clean, instructional, uncluttered — readable at a glance, suited to a classroom/projector.

**Key Elements (pedagogical layer — these are first-class functional features, not decoration):**
- **Live spectrum analyzer** — real-time FFT showing sidebands appear/multiply as index rises. The headline teaching visual.
- **Live waveform / oscilloscope** — shows the output waveform morphing as ratio/index/feedback change.
- **Live operator routing diagram** — block diagram of CAR/MOD + feedback reflecting current signal flow.
- **On-hover pedagogical tooltips** — touching/hovering any parameter shows a short plain-language explanation of what it does and a concrete example.

## Use Cases

- **Classroom demonstration** — instructor projects the plugin and shows FM concepts live (sidebands, ratios, index envelope, feedback) with immediate audio + visual feedback.
- **Self-directed student learning** — a student explores the preset tour, reverse-engineers each patch, and reads tooltips to build intuition with no manual.
- **Quick concept reference** — anyone who half-remembers FM can reload it and instantly see "ratio vs index vs envelope" in action.
- **Lightweight creative FM** — because it's playable and musical, it doubles as a simple FM instrument for bells, e-pianos, and brass.

## Inspirations

- **Yamaha DX7 / DX-style FM** — the canonical FM reference, deliberately simplified down from 6 operators to 2.
- **Ableton Operator** — clean modern FM UI; O-simpleFM goes further toward minimalism + explicit teaching.
- **Native Instruments FM8** — feature-rich FM; O-simpleFM is the antithesis: the smallest set that still explains FM.
- **Syntorial / interactive synth-teaching tools** — the "learn by doing with instant feedback" pedagogy, applied specifically to FM.
- John Chowning's original FM synthesis work — the carrier/modulator/index sideband model O-simpleFM makes tangible.

## Technical Notes

- **DSP:** Classic 2-operator phase-modulation (the "FM" of commercial synths is technically phase modulation) — sine-based operators, modulator phase scaled by index drives carrier phase. Feedback path on the modulator with the standard one-sample-delay averaging to stay stable.
- **Modulation index semantics:** Decide in research whether index is exposed as a raw index, as modulator output level, or as a Hz deviation — pick the formulation that is most pedagogically transparent and label it clearly.
- **Ratio control:** Offer continuous ratio with an optional integer-snap so students can cleanly compare harmonic (integer) vs inharmonic (non-integer) timbres.
- **Visualizations:** Real-time FFT (spectrum) and time-domain (scope) draw from the audio thread via a lock-free FIFO to the UI; keep audio-thread work allocation-free. Spectrum needs enough resolution/smoothing to clearly show discrete sidebands.
- **Envelopes:** Two independent ADSRs (modulator, amplitude). The modulator envelope routing to index is the key expressive feature and must be audibly/visually obvious.
- **Polyphony:** Confirm voice count in research (a modest polyphony, e.g. 8–16 voices, is fine for a teaching tool).
- **Platform:** WebView UI (JUCE 8) for rich live visualizations + tooltips, consistent with the Ouaricon suite. Ensure Windows WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) per project standards.

## Out of Scope (v1.0)

- 4-op / 6-op operators and selectable algorithms (would dilute the pedagogical focus; possible future "O-FM" sibling).
- Effects (reverb/delay/chorus) — keep the signal path transparent.
- Deep modulation matrix / LFO networks.
- A/B compare snapshot (considered, deferred — revisit if it aids teaching).

## Next Steps

- [ ] Create UI mockup (`/start O-simpleFM` → option 3)
- [ ] Start planning / DSP research (`/plan O-simpleFM`)
