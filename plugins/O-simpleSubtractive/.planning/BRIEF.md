# O-simpleSubtractive - Creative Brief

## Overview

**Type:** Synth (Pedagogical Subtractive / Oscillator→Filter→Amp)
**Core Concept:** A deliberately simple subtractive synthesizer — start from a harmonically rich oscillator, carve it with a resonant filter and its envelope, and shape its level with an amp envelope — built to make the canonical synth signal path tangible for classroom teaching and self-directed learning.
**Status:** 💡 Ideated
**Created:** 2026-06-25

## Vision

O-simpleSubtractive is the subtractive sibling to **O-simpleFM** and **O-simpleAdditive**: a teaching instrument first and a synth second. Where O-simpleFM strips FM to one carrier, one modulator, a ratio, and an index, and O-simpleAdditive strips additive to a bank of harmonic drawbars, O-simpleSubtractive strips subtractive synthesis down to its irreducible spine — **an oscillator, a filter, and an amplifier, with one envelope on the filter and one on the amp** — and makes the central insight tangible: *start with a tone rich in harmonics and remove from it; the filter and its envelope are where the sound is actually carved.*

It is built to run alongside the MUSC319 **wk06-wed subtractive** session. That session teaches the canonical synth signal path — oscillator → filter → VCA — as the reference point that additive, FM, and granular are later contrasted against. The four moves of the method are: pick a harmonically rich oscillator, set a filter cutoff and resonance to remove and emphasize harmonics, route a filter envelope to make the brightness move, and shape the level with the amp envelope. O-simpleSubtractive lets a student perform all four in a real instrument and *see each one happen*.

The pedagogical payload is the tight loop between **gesture and visible consequence** (the O-simpleFM north star). Lower the cutoff and watch the upper harmonics in the live spectrum fall away under the filter curve — the literal subtraction the method is named for. Raise the resonance and watch a peak grow at the cutoff; push it further and hear the filter self-oscillate into a pure whistle. Switch the slope from 6 to 12 to 24 dB/oct and watch the curve steepen so the same cutoff reads darker. Route a fast filter envelope with a short decay and hear a bright pluck that snaps closed; route a slow attack and hear a pad open. Sweep the cutoff live and hear the squelching acid move. Add the sub-oscillator for bass weight, or open the filter on white noise for wind and percussion.

Every control is annotated with a short, plain-language tooltip (what cutoff does, why resonance whistles, what "poles" mean, the difference between the filter envelope and the amp envelope). A live signal-path diagram keeps oscillator → filter → amp visible at all times, with the two envelopes shown routing up to the cutoff and the VCA — exactly the block diagram from the class. The design north star, like its siblings: a curious student should reach a genuine "oh, *that's* how subtractive works" moment within five minutes, with no manual — and leave able to **build and save a bass and a lead** to their A2 patch palette, naming what the oscillator, filter, and amplifier each contributed (the in-class activity).

## Architecture

**The canonical subtractive voice: oscillator → filter → VCA, with two independent ADSR envelopes.** Sixteen-voice polyphony with a mono/legato mode, so the same voice serves both the classic monosynth (Minimoog, TB-303) and the polysynth (Juno, Prophet) lessons — *a polysynth is several subtractive voices in parallel, not a different method.*

```
   ┌──────────────────────── one voice (×16 poly / 1 mono) ────────────────────────┐
   │                                                                               │
   │   OSC (saw/square/tri/sine) ─┐                                                │
   │   SUB OSC (octave-down)    ──┼─► MIX ─► FILTER ─────────► VCA ─────► voice out │
   │   NOISE (white)            ──┘         (LP/HP/BP/Notch)    │                   │
   │                                        6/12/24 dB/oct      │                   │
   │                                        cutoff · resonance  │                   │
   │                                            ▲               ▲                   │
   │                                       FILTER ADSR      AMP ADSR                │
   │                                       × env amount                             │
   └───────────────────────────────────────────────────────────────────────────────┘
                                   voices ─► output level ─► out
```

- **Oscillator:** one waveform-selectable oscillator (sawtooth / square / triangle / sine) — the harmonically rich source. Saw gives the filter the most to remove; square is hollow (odd harmonics); triangle is already dark; sine gives the filter almost nothing (kept for contrast).
- **Sub-oscillator:** an octave-down oscillator mixed under the main osc for bass weight.
- **Noise:** a white-noise source mixed in for the filtered-noise lesson (wind, breath, percussion).
- **Filter:** a state-variable filter offering **LP / HP / BP / Notch** modes at **6 / 12 / 24 dB/oct** (1/2/4-pole) slopes, with **cutoff** and **resonance** controls. Resonance can be pushed to **self-oscillation** (a pure tone at the cutoff with no input) — the whistle the class demonstrates. The low-pass is the default and the workhorse.
- **Filter envelope (ADSR) + env amount:** a dedicated ADSR routed to the cutoff through a bipolar **env amount**, so the brightness opens on the attack and closes on the release. The expressive heart of the patch.
- **Amp envelope (ADSR):** a second, independent ADSR driving the VCA — the loudness contour that decides whether the sound reads as a percussive pluck or a sustained pad. *Two envelopes, two jobs: a sound can be bright but quiet, or loud but dark.*
- **Voice mode:** Poly (16 voices) / Mono / Legato, with glide (portamento) for the mono slide (the TB-303 move).

## Parameters

*Core set defined here; Stage 0 research should confirm ranges, tapers, filter topology, and the exact self-oscillation / env-amount formulations. Ranges below are starting proposals.*

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Osc Waveform | sawtooth / square / triangle / sine | sawtooth | The harmonically rich source. Saw = all harmonics (brightest); square = odd only (hollow); triangle = odd, fast rolloff (dark); sine = single partial (contrast). |
| Sub Osc Level | 0–100% | 0% | Octave-down oscillator mixed under the main osc for bass weight. |
| Noise Level | 0–100% | 0% | White-noise source mixed in — the filtered-noise lesson (wind/breath/percussion). |
| Filter Type | LP / HP / BP / Notch | LP | Filter response shape. LP removes highs (the workhorse); HP removes lows; BP keeps a band; Notch removes a band. |
| Filter Slope | 6 / 12 / 24 dB/oct (1/2/4-pole) | 24 dB/oct | Steepness of rejection past the cutoff. 24 dB/oct = the classic Moog-ladder 4-pole. Steeper reads darker at the same cutoff. |
| Cutoff | 20 Hz–20 kHz (log) | ~2 kHz | Frequency where the filter begins to act. Lowering it darkens the tone by removing upper harmonics. THE defining subtractive control. |
| Resonance (Q) | 0–100% | ~10% | Emphasizes a band at the cutoff; at high settings builds the sweep whistle and approaches self-oscillation. |
| Filter Env Amount | -100 to +100% | +50% | Bipolar depth of the filter envelope routed to cutoff. Positive opens then closes; negative inverts. |
| Filter Attack | 0–5 s | 0.005 s | Filter-envelope attack — how fast the brightness opens. |
| Filter Decay | 0–5 s | 0.3 s | Filter-envelope decay toward the sustain brightness. |
| Filter Sustain | 0–100% | 40% | Filter-envelope held brightness while the key is down. |
| Filter Release | 0–5 s | 0.2 s | Filter-envelope release — how the brightness closes after key-up. |
| Amp Attack | 0–5 s | 0.005 s | Amplitude attack — sharp = pluck, slow = swell. |
| Amp Decay | 0–5 s | 0.3 s | Amplitude decay toward the sustain level. |
| Amp Sustain | 0–100% | 80% | Amplitude held while the key is down. |
| Amp Release | 0–5 s | 0.1 s | Amplitude release after key-up. |
| Voice Mode | Poly / Mono / Legato | Poly | 16-voice polyphony, or monophonic (last/legato) for classic lead/bass. |
| Glide (Portamento) | 0–1 s | 0 s | Pitch-slide time between notes — the mono slide (TB-303). |
| Output Level | -inf–0 dB | 0 dB | Master output gain. |

**Likely additions / confirmations from research (Stage 0):** filter topology choice (Moog ladder vs SVF/TPT — which gives the best self-oscillation + all four modes), key-tracking of cutoff (cutoff follows pitch; important so a self-oscillating filter plays in tune), velocity routing (to amp level and/or filter env amount), pulse-width / PWM on the square wave, anti-aliasing strategy for saw/square at high notes (PolyBLEP or band-limited tables — a teaching tool must not buzz), self-oscillation gain compensation, exact resonance taper, master tune/octave.

## UI Concept

*Captured from the sibling template and class figures; full UI design happens in the mockup phase, not here.*

**Layout:** A single clear page (no deep menus), classroom/projector-readable, laid out left-to-right as the signal path itself — Oscillator (waveform + sub + noise) | Filter (type, slope, cutoff, resonance, env amount) | Filter Envelope | Amp Envelope | Voice/Output. The live visuals occupy a prominent panel, mirroring the hardware panels (MS-20, Minimoog, Juno) the class shows laid out as the same chain.

**Visual Style:** Clean, instructional, uncluttered — consistent with O-simpleFM and O-simpleAdditive. Readable at a glance.

**Key Elements (pedagogical layer — first-class functional features, not decoration):**
- **Live filter-response-over-spectrum display** — THE headline visual: the oscillator's harmonic spectrum drawn with the filter's true magnitude-response curve overlaid on top, so a student literally sees harmonics being attenuated, the resonance peak grow, and the curve slide as the filter envelope sweeps. This is the class's "before/after filter" figure made live.
- **Live signal-path diagram** — oscillator → filter → VCA block diagram with the filter envelope routing up to cutoff and the amp envelope up to the VCA, the active stage highlighted (the class's signal-chain diagram, animated).
- **Animated dual-ADSR display** — the filter envelope (to cutoff) and the amp envelope (to level) drawn over a single note on independent scales, so it is obvious they are two envelopes doing two different jobs (the class's filter-envelope figure).
- **Oscilloscope** — time-domain view of the output waveform morphing as cutoff/resonance/envelopes move.
- **On-hover pedagogical tooltips** — short plain-language explanation per control (what cutoff does, why resonance whistles, what "poles/slope" means, filter-env vs amp-env).
- **Concept-preset tour** — named patches each isolating one idea: *Saw → LP Sweep*, *Acid Bass (303)*, *Brass Lead*, *Pluck*, *Sweep Pad*, *Self-Oscillation Sine*, *Hollow Square Bass*, *Filtered Noise (wind)* — so students reverse-engineer each move from the minimal control set, then build and save their own bass and lead (the A2 activity).

## Use Cases

- **Classroom demonstration** — instructor projects the plugin, picks a saw, lowers the cutoff and raises resonance to the whistle, routes a filter envelope, and shapes the amp — each stage's contribution visible and audible in real time.
- **Self-directed student learning** — a student works the preset tour, reads tooltips, and builds + saves a bass and a lead to their A2 patch palette (the in-class activity), articulating what oscillator, filter, and amplifier each add.
- **Mono ↔ poly bridge** — demonstrates concretely that a polysynth is several copies of the same subtractive voice by toggling Poly/Mono on one instrument.
- **Lightweight creative subtractive synth** — playable and musical enough to double as a simple instrument for basses, leads, plucks, and pads.

## Inspirations

- **O-simpleFM / O-simpleAdditive** — the direct siblings and pedagogical template (irreducible control set, gesture→visible-consequence, live visuals, tooltips, concept-isolating presets).
- **Minimoog Model D** — the 4-pole ladder filter that became the reference subtractive sound; one voice through oscillator → filter → amp.
- **Roland TB-303** — cutoff + resonance under the player's hands; the squelching acid sweep where the filter itself becomes the performance.
- **Korg MS-20** — the semi-modular panel that labels the oscillator/filter/amp/envelope stages directly (highpass + lowpass filter).
- **Roland Juno-60 / Sequential Prophet-5** — polysynths that stack copies of the same subtractive voice so chords can be played.
- **MUSC319 wk06-wed subtractive demo** — students build a patch from an initialized synth, set a filter and its envelope, shape the amp, and save a bass and a lead; this plugin lets them reproduce that move in a real instrument.

## Technical Notes

- **DSP:** Classic subtractive voice — band-limited oscillator (saw/square/triangle/sine) + octave-down sub + white noise → mixer → resonant multi-mode filter → VCA. Two per-voice ADSRs (filter, amp). 16-voice polyphony with a mono/legato mode and glide.
- **Filter:** Multi-mode (LP/HP/BP/Notch) at selectable 6/12/24 dB/oct slopes, capable of self-oscillation at high resonance. Confirm topology in research — a zero-delay-feedback (TPT) state-variable filter cleanly gives all four modes and the 12 dB slope, with cascading for 24 dB; a Moog-ladder model gives the most authentic 24 dB self-oscillation. Pick the approach that delivers stable self-oscillation, all four modes, *and* visibly correct magnitude curves for the headline visual. Key-track the cutoff so a self-oscillating filter can be played in tune.
- **Envelopes:** Two independent ADSRs. The filter envelope routes to cutoff through a bipolar env-amount; the amp envelope drives the VCA. The independence of the two envelopes must be audibly and visibly obvious (the headline lesson).
- **Anti-aliasing:** Saw/square are harmonically rich; at high notes they must not alias. Use PolyBLEP or band-limited wavetables so high keys stay clean — a teaching tool must not buzz.
- **Visualizations:** The filter magnitude response is computed from the live cutoff/resonance/slope/type and drawn over the oscillator's harmonic spectrum; the scope draws from the audio thread via a lock-free FIFO. Keep audio-thread work allocation-free; UI draws from a lock-free handoff. The spectrum/curve must reflect what is actually heard.
- **Self-oscillation:** At max resonance the filter should produce a clean sine at the cutoff with no input (the class's whistle). Apply gain compensation so resonance sweeps don't blow up level.
- **Platform:** WebView UI (JUCE 8) for rich live visualizations + tooltips, consistent with the Ouaricon suite and the siblings. Must set Windows WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) per project standards. If a second binary-data target is added (embedded presets + WebView resources), give it a distinct `NAMESPACE` to avoid the BinaryData collision seen in O-simpleGrain.

## Out of Scope (v1.0)

- A second full oscillator with detune / supersaw (the "fat" lesson) — the sub-osc + noise cover weight and texture without a second osc; possible future expansion.
- A modulation matrix / multiple LFOs / dedicated vibrato LFO (a single optional pitch/filter LFO may be revisited in research; v1.0 keeps the two envelopes as the modulation story).
- Effects (reverb/delay/chorus) — keep the signal path transparent for teaching (the Juno chorus is noted historically but not built).
- Full filter-shapes deep dive (the EQ/filters session owns that); here LP is the focus with HP/BP/Notch available for contrast.
- Unison/voice-stacking, arpeggiator, sequencer (the 303's sequencer is referenced but not built).

## Next Steps

- [ ] Create UI mockup (`/start O-simpleSubtractive` → option 3)
- [ ] Start planning / DSP research (`/plan O-simpleSubtractive`)
