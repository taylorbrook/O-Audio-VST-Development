# O-Detune Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.1.1
- **Type:** Audio Effect (Detuning / Pitch Thickening)
- **Complexity:** 5.0 (Maximum)

## Lifecycle Timeline

- **2026-02-01 (Ideation):** Plugin concept created - dual-engine detuning (wobble + unison) with mono-safe mode
- **2026-02-01 (Stage 0):** Research & Planning complete - ARCHITECTURE.md and ROADMAP.md documented (Complexity 5.0)
- **2026-02-02 (v1.0.0):** Initial release - dual-engine architecture, WebView UI
- **2026-02-02 (v1.0.1):** Fixed UI knobs and dropdowns (JUCE 8 ComboBox API correction)
- **2026-02-02 (v1.1.0):** Complete UI redesign to Ouaricon Naturalist aesthetic (paper background, SVG vine-arc knobs, nudibranch botanical overlay)
- **2026-02-02 (v1.1.1):** Fixed UI knob smoothness - implemented requestAnimationFrame interpolation loop, added mouse wheel support

## Overview

A colorful lo-fi detuning plugin that combines analog tape wobble with unison thickness in one mono-safe package. Fills the gap between MicroShift's clinical widening and RC-20's tape character.

**Tagline:** "Tape wobble meets unison thickness - colorful detuning, mono-safe."

## Market Position

**The gap we fill:**
- Combines tape-style wobble (wow/flutter) + unison detuning in one plugin
- Mono-safe mode (Polyverse Wider-style) built-in
- Character processing (saturation + color + age)
- Target price: $49-69 (competitive with RC-20, lower than MicroShift/Wow Control)

**Competitors:**
- Soundtoys MicroShift ($99) - Unison only, no tape wobble
- Goodhertz Wow Control ($129) - Tape wobble only, no unison
- XLN RC-20 ($59) - All-in-one lo-fi, less focused
- Polyverse Wider (FREE) - Mono-safe widening only, no pitch processing

## Technical Summary

**DSP Architecture:**
- Dual-engine design (Wobble + Unison with crossfade blend)
- Delay-based pitch shifting (juce::dsp::DelayLine with Lagrange3rd interpolation)
- Multi-LFO modulation (primary + secondary + noise for non-repeating patterns)
- Mono-safe mode (all-pass/comb filter array - Polyverse Wider-style)
- 21 parameters across 11 DSP components

**Complexity Breakdown:**
- Parameters: 2.0 (21 params, capped)
- Algorithms: 11.0 (dual engines + modulation + character + stereo)
- Features: +2 (feedback loops + modulation systems)
- Total: 5.0 (maximum complexity score)

**JUCE Modules:**
- juce::dsp::DelayLine (Lagrange3rd interpolation for pitch modulation)
- juce::dsp::Oscillator (LFO system)
- juce::dsp::IIR::Filter (color/focus/era filters)
- juce::dsp::DryWetMixer (latency-compensated mixing)

## Implementation Strategy

**Phased implementation (3 DSP phases + 2-3 GUI phases):**

**Phase 4.1: Core Processing**
- Wobble + unison engines (simple sine LFO, 3 voices)
- Blend control
- Focus filter
- Dry/wet mixing

**Phase 4.2: Modulation & Character**
- Multi-LFO (dual-LFO + noise modulation)
- Triangle/random shapes, tempo sync
- Saturation, color, age
- Feedback loop

**Phase 4.3: Advanced Features**
- Unison expansion (2/4/5/7 voices)
- Exponential/random distribution
- Stereo width processing
- Mono-safe mode
- Era presets (60s/70s/80s)

**Phase 5.1: GUI Layout**
- WebView mockup integration
- Basic controls

**Phase 5.2: GUI Binding**
- Two-way parameter communication

**Phase 5.3: GUI Visualizations (OPTIONAL)**
- Wobble visualization
- Unison indicator
- Mono-safe indicator

## Parameters (21 Total)

**Mode Selection:**
- blend (0-1) - Wobble ↔ Unison crossfade

**Wobble Engine:**
- wobble_era (Choice: 60s/70s/80s) - Ampex/Teac/Cassette character
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
- width (0-200%) - Stereo spread (0=Mono, 100=Stereo, 200=Extra-wide)
- mix (0-100%) - Wet/dry blend
- focus_low (20-500 Hz) - High-pass cutoff
- focus_high (1k-20k Hz) - Low-pass cutoff
- mono_safe (Bool) - Phase-coherent toggle

**Advanced:**
- delay (0-50 ms) - Pre-delay
- feedback (0-80%) - Recirculation amount
- random_amt (0-100%) - Per-voice variation

## Highest Risk Component

**Mono-Safe Mode (All-Pass/Comb Filters):**
- Represents ~40% of project risk
- Algorithm not publicly documented (requires reverse-engineering Polyverse Wider)
- Requires deep DSP understanding (phase, group delay)
- No direct JUCE class (custom implementation)
- Verification requires unit testing (L + R = 0 in mono)

**Fallback:** Mid-side processing with careful phase management

## Professional Plugin Research

**Goodhertz Wow Control:**
- Three era modes (15 IPS, 7.5 IPS, Cassette)
- Multi-LFO for non-repeating patterns
- Noise modulated by signal like real tape machine

**XLN RC-20 Retro Color:**
- Dual-LFO wobble (wow 0.1-4 Hz, flutter 6-20 Hz)
- Stereo mode creates chorus effect

**Soundtoys MicroShift:**
- Hardware-inspired (Eventide H3000, AMS DMX 15-80s)
- Multi-voice detuning with time-varying delay
- Focus control for frequency-selective processing

**Polyverse Wider:**
- Mono-compatible via all-pass/comb filters
- Perfect phase coherence (L + R = 0 in mono)
- Stereo width up to 200%

**Valhalla Delay:**
- Delay-based pitch shifting
- Mod rate/depth controls for random modulation

## Known Issues

None

## Additional Notes

**Performance targets:**
- CPU: <50% single core @ 48kHz (7 voices + all features)
- Latency: 50ms (2400 samples @ 48kHz)
- Memory: <50MB

**Critical patterns to remember:**
- Pattern #3: DelayLine Lagrange3rd interpolation for smooth pitch modulation
- Pattern #17: juce::dsp API - Use prepare(ProcessSpec) NOT setSampleRate()
- Pattern #21: WebView ES6 modules - Add type="module" to script tags
- Pattern #12: WebSliderParameterAttachment requires 3 parameters in JUCE 8

**Planning documents:**
- Creative brief: `plugins/O-Detune/.planning/BRIEF.md`
- Parameter spec: `plugins/O-Detune/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Detune/.planning/research/ARCHITECTURE.md`
- Implementation plan: `plugins/O-Detune/.planning/ROADMAP.md`
- Stage 0 context: `plugins/O-Detune/.planning/stages/0-ideation/CONTEXT.md`

---

*Last updated: 2026-02-02*
