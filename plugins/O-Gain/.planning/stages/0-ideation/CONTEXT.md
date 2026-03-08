# O-Gain Stage 0: Ideation Context

**Date:** 2026-03-07
**Agent:** research-planning-agent
**Purpose:** Document decisions, constraints, and approach from Stage 0 research and planning

---

## Key Decisions

### 1. Plugin Type: Audio Effect (Utility)

O-Gain is a stereo-in/stereo-out audio effect. It processes existing audio (applies gain, channel utilities) and measures incoming signal levels. No MIDI input, no sound generation.

- `IS_SYNTH FALSE` (default, omit from CMakeLists.txt)
- BusesProperties: stereo input + stereo output
- Zero latency: critical requirement (designed for every track in a session)

### 2. Measurement Algorithm: Custom BS.1770 Implementation

Chose custom implementation over libebur128 dependency:
- ITU-R BS.1770-5 coefficients are publicly available
- Custom implementation integrates naturally with JUCE's `juce::dsp::IIR::Filter<double>`
- No external build dependency
- Full control over threading (audio-thread-safe accumulation)

The K-weighting filter chain uses two cascaded biquad IIR filters (pre-filter high-shelf + RLB high-pass) with published coefficients for 48 kHz. Other sample rates require bilinear transform recalculation -- pre-calculate for common rates.

### 3. True Peak: MVP Uses Digital Peak, V1.0 Uses Polyphase FIR

For MVP, simple `std::abs(sample)` peak detection is sufficient. The polyphase FIR approach (12 taps per phase, 4 phases from BS.1770) will be added in refinement. This avoids blocking the initial implementation on multi-rate signal processing.

### 4. Measurement on Pre-Gain Signal

Learn mode measures the INPUT signal before gain application. This prevents a feedback loop where the plugin would re-measure its own gain adjustment. Standard practice in all auto-gain plugins (HoRNet VU MK4, TheNormalizer).

### 5. Double Precision for Measurement Path

LUFS calculation uses `double` precision throughout (IIR filters, power accumulation, gating). This prevents numerical drift when accumulating squared samples over 10-30 second Learn sessions. Single precision (`float`) is adequate for the gain stage and channel utilities.

### 6. Phased DSP Implementation

Despite being a "utility" plugin, the LUFS measurement subsystem is non-trivial. The DSP stage is split into two phases:
- Phase 3.1: Core Processing (gain + channel utilities + basic metering) -- gets the plugin functional quickly
- Phase 3.2: Learn Mode + LUFS Measurement -- adds the measurement engine and VU ballistics

This ensures a working plugin after Phase 3.1 even if Phase 3.2 encounters issues.

---

## Constraints

### Performance Budget
- Target: < 1% CPU per instance when idle (40+ instances per session)
- Learn mode: < 15% CPU per instance (only active on one track at a time typically)
- No allocations on audio thread
- Timer-based UI updates (30Hz), not processBlock-driven

### Zero Latency
- Non-negotiable: gain staging plugin must add zero latency
- No FFT, no lookahead, no oversampling in the signal path
- True peak detection is a measurement side-chain (does not affect audio output timing)

### Thread Safety
- Audio thread: reads learn flag (atomic), accumulates measurement, writes meter values (atomic)
- UI thread: toggles learn flag, reads meter values, reads final results after learn stops
- Single-producer/single-consumer pattern for learn accumulator data
- APVTS handles parameter thread safety automatically

---

## Approach Summary

O-Gain combines three subsystems:

1. **Channel Utilities** (trivial): Phase inversion, channel swap, mono sum, M/S encode/decode. All sample-level operations, zero complexity.

2. **Gain Stage** (trivial): `juce::dsp::Gain<float>` with SmoothedValue for click-free transitions. Applies gain_offset + trim.

3. **Measurement Engine** (moderate): K-weighted LUFS measurement per ITU-R BS.1770-5 with EBU R128 dual-gate system. This is the heart of the plugin and the primary source of implementation complexity.

The measurement engine only runs during Learn mode. In normal operation (gain applied, no learning), the plugin is a trivially lightweight gain utility with metering -- suitable for running on every track in a large session.

---

## Market Context

From the market research, O-Gain occupies an empty niche:
- **VUMT Deluxe** has best metering + utilities but no auto-learn
- **HoRNet VU MK4** has auto-gain but VU-only targeting
- **TheNormalizer** has flexible targets but no utilities or metering display
- No single plugin combines: auto-learn + multi-standard metering + channel utilities + compact UI

The "Measure. Learn. Set. Done." workflow is validated by user demand on KVR and Gearspace forums.

---

## Risk Assessment

**Highest risk:** K-weight filter coefficient recalculation for non-48kHz sample rates
- Mitigation: pre-calculate for common rates, validate against reference meters
- Fallback: use libebur128 if custom implementation proves inaccurate

**Medium risk:** LUFS measurement accuracy validation
- Mitigation: test with EBU test signals, compare with Youlean Loudness Meter output

**Low risk:** Everything else (gain, utilities, metering display, UI)
- These are well-understood patterns with existing JUCE classes

---

## Files Created

- `plugins/O-Gain/.planning/research/ARCHITECTURE.md` -- Complete DSP architecture specification
- `plugins/O-Gain/.planning/ROADMAP.md` -- Implementation plan with complexity assessment
- `plugins/O-Gain/.planning/stages/0-ideation/CONTEXT.md` -- This file
