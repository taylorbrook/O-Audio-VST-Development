# O-Detune Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.1.0
- **Type:** Audio Effect (Detuning / Pitch Thickening)

## Lifecycle Timeline

- **2026-02-01 (Ideation):** Plugin concept created - dual-engine detuning (wobble + unison) with mono-safe mode
- **2026-02-01 (Stage 0):** Research & Planning complete - ARCHITECTURE.md and ROADMAP.md documented
- **2026-02-01 (v1.0.0):** Initial release - basic implementation with parameter infrastructure
- **2026-02-02 (v1.1.0):** Complete Implementation Release - all 21 parameters connected to DSP, Ouaricon Naturalist UI aesthetic applied

## Overview

A colorful lo-fi detuning plugin that combines analog tape wobble with unison thickness in one mono-safe package. Fills the gap between MicroShift's clinical widening and RC-20's tape character.

**Tagline:** "Tape wobble meets unison thickness - colorful detuning, mono-safe."

## v1.1.0 Changes

### DSP Implementation (Complete)
- **True Random LFO**: Sample-and-hold with 10ms smoothing, uses system random for non-repeating patterns
- **Era Character**: 60s (slower/warmer), 70s (default), 80s (faster/brighter)
- **Dynamic Voices**: 2/3/4/5/7 voice selection now functional
- **Distribution Modes**: Linear, Exponential, Random voice spreading
- **Character Section**: Drive (tanh saturation), Color (LP/HS blend), Age (noise injection)
- **Advanced Section**: Pre-delay, Feedback, Per-voice randomization
- **Mono-Safe**: Side content limiting for phase correlation

### UI Overhaul
- **Ouaricon Naturalist Aesthetic** applied
- Paper background texture (vintage cream)
- Botanical slug overlay (sea slug illustration)
- Seed cross-section knob design
- Garamond serif typography
- Green botanical accent colors

## Parameters (21 Total)

**Mode Selection:**
- blend (0-1) - Wobble ↔ Unison crossfade

**Wobble Engine:**
- wobble_era (Choice: 60s/70s/80s) - Era character presets
- wobble_rate (0.1-10 Hz) - Modulation speed
- wobble_depth (0-100 cents) - Pitch deviation amount
- wobble_shape (Choice: Sine/Triangle/Random) - LFO waveform
- wobble_sync (Bool) - Tempo sync enable

**Unison Engine:**
- unison_voices (Choice: 2/3/4/5/7) - Voice count
- unison_detune (0-50 cents) - Total spread
- unison_dist (Choice: Linear/Exp/Random) - Voice distribution
- unison_spread (0-100%) - Stereo panning width

**Character Section:**
- drive (0-100%) - Saturation intensity
- color (-100 to +100) - Dark (LP) to Bright (shelf)
- age (0-100%) - Degradation (noise + drift)

**Output Section:**
- width (0-200%) - Stereo spread
- mix (0-100%) - Wet/dry blend
- focus_low (20-500 Hz) - High-pass cutoff
- focus_high (1k-20k Hz) - Low-pass cutoff
- mono_safe (Bool) - Phase-coherent toggle

**Advanced:**
- delay (0-50 ms) - Pre-delay
- feedback (0-80%) - Recirculation amount
- random_amt (0-100%) - Per-voice variation

## Technical Details

**DSP Architecture:**
- Dual-engine design (Wobble + Unison with crossfade blend)
- Delay-based pitch shifting (juce::dsp::DelayLine with Lagrange3rd interpolation)
- Custom LFO generator with sine, triangle, and sample-and-hold random shapes
- Era-specific filters (LP @ 2kHz for 60s, HS @ 4kHz for 80s)
- Color filters (LP for dark, HS for bright)
- Mid-side stereo processing for width
- Side limiting for mono-safe mode

**Latency:** 50ms (2400 samples @ 48kHz)

**JUCE Modules:**
- juce::dsp::DelayLine (Lagrange3rd interpolation)
- juce::dsp::IIR::Filter (focus/era/color filters)
- juce::dsp::DryWetMixer (latency-compensated mixing)

## Factory Presets

1. **Default** - Balanced starting point
2. **Thick Vocals** - 3-voice unison for vocal thickening
3. **Supersaw Synth** - Wide 5-voice detuning for synths
4. **70s Tape Wobble** - Authentic Teac-style pitch variation
5. **Cassette Lo-Fi** - Degraded 80s tape character
6. **Hybrid Wobble Unison** - Combined wobbling unison voices

## Known Issues

None

## Installation

- **VST3:** ~/Library/Audio/Plug-Ins/VST3/O-Detune.vst3
- **AU:** ~/Library/Audio/Plug-Ins/Components/O-Detune.component

## Validation

- pluginval: PASSED (strictness level 5)
- auval: PASSED

---

*Last updated: 2026-02-02*
