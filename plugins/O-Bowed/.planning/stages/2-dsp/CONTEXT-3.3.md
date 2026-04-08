# Stage 2: DSP Phase 3.3 - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User (Taylor Brook), Claude
**Phase:** Multi-String + Sympathetic Coupling

## Requirements Confirmed

- STRING_COUNT (1-4) controls processor-level drone strings, NOT per-voice multi-string
- A single MIDI note-on triggers exactly one voice with one WaveguideString
- Double-stops are achieved by holding two MIDI notes (two voices), not by STRING_COUNT
- STRING_TUNING_1-4 set the pitches of the drone strings (always bowed, independent of MIDI)
- SYMPATHETIC_COUNT (0-12) controls passive KS waveguide resonators (no bow, excited by bridge output)
- Sympathetic strings tune relative to played note's overtone series with full microtonal support
- Per-string panning across stereo field — body resonator must process stereo

## Constraints Identified

- KS waveguides for sympathetic strings (~0.1-0.3% CPU each, ~3% for 12)
- Dynamic voice cap when drone count increases (fewer polyphonic voices to stay in CPU budget)
- Body resonator refactored from mono to stereo (to support per-string panning)
- Max simultaneous waveguides: 8 voices + 4 drones + 12 sympathetic = 24

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Multi-string architecture | Processor-level drones | STRING_COUNT strings are always-bowed drones, not per-voice internal. A note = one voice, one string. |
| Per-string bow behavior | Slight randomization (±5%) | Natural character — real instruments have per-string variation even with same bow |
| Sympathetic implementation | KS waveguides (delay + loss filter) | More physically accurate than bandpass resonators; CPU budget allows it |
| Sympathetic tuning | Relative to played note + microtonal | Overtone series of current pitch, respecting active tuning system (Scala/MTS-ESP/12TET) |
| Sympathetic excitation source | Both pre-body and post-body | Mix of raw bridge output + body-colored signal feeds sympathetic strings |
| String panning | Per-string pan before sum (stereo body) | Drone strings and voice strings panned across stereo field; body resonator processes both channels |
| CPU polyphony scaling | Dynamic voice cap | Reduce max polyphonic voices when STRING_COUNT > 1 to stay within budget |

## Architecture Implications

### Drone String Engine (new, processor-level)
- Owns 1-4 WaveguideString + BowModel + HyperbolicFriction instances
- Always running when STRING_COUNT >= 1
- Reads STRING_TUNING_1-4 for pitch (cents offset from reference or absolute — TBD in research)
- Bow parameters shared from APVTS with ±5% per-string randomization
- Each drone string panned to a position in stereo field
- Output fed into body resonator (now stereo)

### Sympathetic String Engine (new, processor-level)
- 0-12 passive KS waveguide strings (delay line + one-pole loss filter each)
- Tuned to overtone series of currently active voice(s)
- Retune dynamically when played notes change
- Excitation: blend of pre-body (raw bridge) and post-body (resonated) signal
- Gating: skip processing when string amplitude < threshold
- Full microtonal support via TuningEngine

### Body Resonator Refactor
- Currently mono (processes channel 0 only)
- Must become stereo to preserve per-string pan positions
- Process both L/R channels independently (8 biquads per channel, or shared coefficients)

### Dynamic Voice Cap
- STRING_COUNT=1: 8 polyphonic voices (current)
- STRING_COUNT=2: ~6 voices
- STRING_COUNT=3: ~5 voices
- STRING_COUNT=4: ~4 voices
- Exact numbers TBD based on CPU profiling in research phase

### Signal Flow (Phase 3.3 Target)
```
MIDI -> Synthesiser -> BowedStringVoice[N] (mono per voice)
  -> per-voice pan position -> stereo buffer

DroneStringEngine (1-4 always-bowed strings)
  -> per-drone pan position -> add to stereo buffer

BodyResonator (stereo, both channels)

Sympathetic excitation = mix(pre-body bridge sum, post-body output)
  -> SympatheticStringEngine (0-12 passive KS waveguides)
  -> add sympathetic output to stereo buffer

StereoWidthProcessor (M/S width)
  -> Output (L, R)
```

## Open Questions (for research phase)

- Drone string pitch model: STRING_TUNING as absolute Hz, cents offset from A4, or MIDI note + cents?
- Sympathetic string retune strategy: instant retune on note change, or smooth portamento?
- Pre-body / post-body excitation blend ratio: fixed or parameterized?
- Body resonator stereo: duplicate filter bank (16 biquads) or shared coefficients with separate state (8 coefficients, 16 filter states)?
- Gating threshold for sympathetic strings: amplitude-based or energy-based?
- How to handle sympathetic tuning when multiple voices play different notes simultaneously

## Next Phase

Ready for: research phase
