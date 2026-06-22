# O-simpleAdditive - Creative Brief

## Overview

**Type:** Synth (Pedagogical Additive + Light Wavetable)
**Core Concept:** A deliberately simple additive synthesizer — build a timbre by summing harmonic partials, then scan/morph that spectrum to teach wavetable thinking — designed for classroom teaching and self-directed learning.
**Status:** 💡 Ideated
**Created:** 2026-06-22

## Vision

O-simpleAdditive is the additive sibling to **O-simpleFM**: a teaching instrument first and a synth second. Where O-simpleFM strips FM down to one carrier, one modulator, a ratio, and an index, O-simpleAdditive strips additive synthesis down to its irreducible core — **a bank of harmonic partials, each with its own amplitude** — and makes the central insight tangible: *any periodic tone is the sum of its sine partials, and the mix of partial amplitudes is the timbre.*

It is built to run alongside the MUSC319 wk07 additive/wavetable session. The class teaches two engines that answer one question — how a synth specifies a spectrum: additive sums partials up; wavetable stores and scans complete single cycles. O-simpleAdditive unifies them so a student feels they are **two directions of one idea**. You sculpt a single-cycle waveform by pushing 16 harmonic drawbars; the waveshape those partials sum to draws itself live beside the spectrum; then you scan/morph that spectrum toward a second one and hear the timbre evolve across a held note. The wavetable frame *is* a summed-partials waveshape — the bridge the class wants students to internalize.

The pedagogical payload is the tight loop between **gesture and visible consequence** (the O-simpleFM north star). Raise the 3rd harmonic and watch the waveform grow a wrinkle and the tone brighten. Pull every partial down except the fundamental and hear a pure sine; raise the odds-only and hear it go hollow toward a square. Load the saw preset and read how 1/n amplitudes fill in the ramp. Turn the spectral-decay macro and hear the upper partials fade faster than the lower ones — the expressive heart of additive, in one knob. Sweep the scan position with an LFO and watch a held note morph. Drop the bit depth and hear the gritty quantization character of early digital wavetables.

Every control is annotated with a short, plain-language tooltip (overtone series, why odd-only sounds hollow, what bit depth does). The design north star, like its sibling: a curious student should reach a genuine "oh, *that's* how additive works" moment within five minutes, with no manual — and leave able to save both an additive patch and an evolving wavetable patch to their A2 palette.

## Architecture

**Unified additive engine with a scan/morph (wavetable) dimension.** The instrument is always additive; the wavetable is an *added dimension*, not a separate mode.

```
  16 harmonic drawbars ─► FRAME A (spectrum A)
                                      │
                                  scan/morph ◄── manual knob / LFO / mod-envelope
                                      │
  target spectrum      ─► FRAME B (spectrum B)
                                      │
                          summed single-cycle waveform
                                      │
                           bit-depth quantize
                                      │
            per-voice amp ADSR  ×  spectral-decay macro (high partials decay faster)
                                      │
                                  output level ─► out   (16-voice poly)
```

- **Frame A:** the spectrum you build with the 16 harmonic drawbars (the main control surface; the bars double as the live spectrum display).
- **Frame B:** a second target spectrum (drawbar-editable and/or a preset shape) that scan morphs toward.
- **Scan position:** interpolates/morphs Frame A → Frame B; drivable manually, by an LFO, and by a mod-envelope so the timbre evolves across a single held note (the wavetable "spectral evolution" lesson).
- **Spectral-decay macro:** one control that makes higher partials decay faster than lower ones over the note — captures real-instrument behaviour without 16 separate envelopes.
- **Bit depth:** quantizes the summed single-cycle waveform to N amplitude levels for the early-digital grit the class demonstrates.
- **Amp ADSR + 16-voice polyphony:** standard per-voice amplitude envelope, mirroring O-simpleFM conventions.

## Parameters

*Core set defined here; Stage 0 research should confirm ranges, tapers, and the exact spectral-decay / morph formulations. Ranges below are starting proposals.*

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Partial 1–16 Level (Frame A) | 0–100% each | H1=100%, rest 0% | The 16 harmonic drawbars — per-partial amplitude of Frame A. THE defining additive control; bars double as the live spectrum. |
| Frame B Source | drawbar-edit / preset (sine/saw/square/odd) | saw | Target spectrum the scan morphs toward. Confirm in research whether B is fully drawbar-editable or preset-selectable (or both). |
| Scan Position | 0–100% (A↔B) | 0% (Frame A) | Morph pointer between Frame A and Frame B. The wavetable "position/scan" control. |
| Scan LFO Rate | 0.01–20 Hz | 0.5 Hz | LFO speed driving the scan position. |
| Scan LFO Depth | 0–100% | 0% | How far the LFO sweeps the scan. |
| Scan Env Amount | -100–100% | 0% | Mod-envelope amount routed to scan (evolution once per note). |
| Spectral Decay | 0–100% | 0% | Macro: how much faster higher partials decay than lower ones over the note. The "expressive heart" in one knob. |
| Bit Depth | 2–16 bits (or "off") | off/16 | Amplitude-resolution quantization of the summed waveform — early-digital grit. |
| Amp Attack | 0–5 s | 0.005 s | Carrier amplitude attack. |
| Amp Decay | 0–5 s | 0.3 s | Carrier amplitude decay. |
| Amp Sustain | 0–100% | 80% | Carrier amplitude sustain. |
| Amp Release | 0–5 s | 0.1 s | Carrier amplitude release. |
| Mod Env Attack/Decay/Sustain/Release | 0–5 s / % | short / med / 80% / short | Envelope feeding Scan Env Amount (and available for spectral evolution routing). Confirm scope in research. |
| Output Level | -inf–0 dB | 0 dB | Master output gain. |

**Likely additions / confirmations from research (Stage 0):** anti-aliasing strategy for high partials (band-limit / Carson-style cap so the 16th harmonic doesn't alias at high notes), morph interpolation method (linear spectral vs waveform crossfade), whether Frame B gets its own drawbar set in the UI, LFO waveform choice, polyphony confirmation (proposing 16), velocity routing (to amp and/or spectral decay).

## UI Concept

*Captured from user-volunteered direction; full UI design happens in the mockup phase, not here.*

**Layout:** A single clear page (no deep menus), classroom/projector-readable. Grouped as: 16 harmonic drawbars (Frame A) — the dominant element | Frame B / Scan + LFO + Env (the wavetable dimension) | Spectral Decay + Bit Depth | Amp Envelope | Output. The live summed-waveform scope sits next to the drawbar spectrum.

**Visual Style:** Clean, instructional, uncluttered — consistent with O-simpleFM. Readable at a glance.

**Key Elements (pedagogical layer — first-class functional features, not decoration):**
- **Drawbar spectrum display** — the 16 harmonic bars ARE both the control surface and the live spectrum readout (the spectrum is the interface).
- **Live summed-waveform scope** — time-domain view that redraws as drawbars / scan move, shown beside the spectrum (the demo's headline "partials → waveshape" visual).
- **Classic-waveform preset tour** — sine / sawtooth / square / odd-harmonics-only presets matching the class demo, so students reverse-engineer how harmonics build each shape.
- **On-hover pedagogical tooltips** — short plain-language explanation per control (overtone series, why odd-only is hollow, what scan/morph does, what bit depth does).

## Use Cases

- **Classroom demonstration** — instructor projects the plugin, builds a tone from partials, loads saw/square/odd-only presets, then sweeps the scan to show spectral evolution — immediate audio + visual feedback.
- **Self-directed student learning** — a student works the preset tour, reads tooltips, builds an additive patch and an evolving wavetable patch, and saves both to their A2 patch palette (the in-class activity).
- **Additive ↔ wavetable bridge** — demonstrates concretely that a wavetable frame is a summed-partials waveshape and that scanning morphs the spectrum.
- **Lightweight creative additive synth** — playable and musical enough to double as a simple instrument for organ-ish, hollow, and evolving pad timbres.

## Inspirations

- **O-simpleFM** — the direct sibling and pedagogical template (irreducible control set, gesture→visible-consequence, live visuals, tooltips, concept-isolating presets).
- **Organ drawbars (Hammond)** — the canonical coarse additive interface; one bar per harmonic.
- **PPG Wave / Waldorf Microwave** — the wavetable lineage the class cites (scanning single-cycle frames, bit-depth grit).
- **Logic Alchemy / Image-Line Harmor / NI Razor** — modern additive + spectral engines; O-simpleAdditive is the deliberately minimal teaching counterpart.
- **MUSC319 wk07 additive/wavetable demo** — builds a tone from the first 8 harmonics and morphs a wavetable position; this plugin lets students reproduce that move in a real instrument.

## Technical Notes

- **DSP:** Additive resynthesis of one single-cycle period from 16 harmonic partials (sum of sines at integer multiples of the fundamental, per-partial amplitude). Two spectra (Frame A / Frame B); scan morphs between them. Confirm in research whether to render per-sample by summing partials, or precompute a band-limited wavetable per voice/note and interpolate — pick the approach that is both real-time safe and pedagogically faithful (the live spectrum must reflect what's heard).
- **Anti-aliasing:** With 16 harmonics, high notes push upper partials past Nyquist. Band-limit the partial set per note (drop/attenuate partials above Nyquist) so high keys stay clean — a teaching tool must not buzz.
- **Spectral-decay macro:** Applies a per-partial decay multiplier that scales with harmonic number over the note (higher partials decay faster). Define the exact curve in research; it must be audibly and visibly obvious (the spectrum should visibly tilt over the note).
- **Scan / morph:** Interpolation between Frame A and Frame B spectra (or between their rendered cycles). Drivable by manual knob, LFO, and mod-envelope. Morphing should be smooth (no zipper) for held-note evolution.
- **Bit depth:** Quantize the summed waveform to N levels (2–16 bits) for the quantization-grit lesson; "off"/16-bit = clean.
- **Visualizations:** Drawbar bars = spectrum; live time-domain scope from the rendered single cycle (or audio-thread FIFO) showing the summed waveshape morph. Keep audio-thread work allocation-free; UI draws from a lock-free handoff.
- **Envelopes:** Per-voice amp ADSR (mirrors O-simpleFM). A mod-envelope routes to scan (and possibly spectral decay) for once-per-note evolution.
- **Polyphony:** Proposing 16 voices (matches O-simpleFM); confirm in research.
- **Platform:** WebView UI (JUCE 8) for rich live visualizations + tooltips, consistent with the Ouaricon suite and O-simpleFM. Must set Windows WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) per project standards.

## Out of Scope (v1.0)

- Full per-partial envelopes (32+ controls) — the spectral-decay macro stands in for the "higher partials decay faster" lesson without the slider wall.
- More than two morph frames / a multi-frame wavetable bank (possible future expansion; v1.0 is the A→B morph).
- FFT analysis / additive *resynthesis from recorded audio* (the class discusses it; the plugin demonstrates the synthesis direction only).
- Effects (reverb/delay/chorus) — keep the signal path transparent for teaching.
- Non-sine partials, deep modulation matrices, formant filters.

## Next Steps

- [ ] Create UI mockup (`/start O-simpleAdditive` → option 3)
- [ ] Start planning / DSP research (`/plan O-simpleAdditive`)
