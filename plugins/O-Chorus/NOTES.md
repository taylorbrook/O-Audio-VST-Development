# O-Chorus Notes

## Status
- **Current Status:** 🚧 Stage 0 (Research & Planning Complete)
- **Version:** N/A (not yet implemented)
- **Type:** Audio Effect (Multi-Voice BBD-Style Chorus)
- **Complexity Score:** 2.8 (Moderate)
- **Implementation Strategy:** Single-pass

## Overview

O-Chorus is a lush, analog-inspired multi-voice chorus plugin with 1-8 selectable voices. Inspired by classic hardware units like the Roland Juno-60 chorus and Boss CE-1, it combines modern flexibility with BBD-style warmth through modulated delay lines, soft saturation, and tone control.

**Key Features:**
- Multi-voice chorus engine (1-8 voices)
- BBD-inspired analog warmth (saturation + filtering)
- Per-voice LFO phase distribution for rich, non-mechanical modulation
- Stereo imaging control (0-100% width)
- Tone control (dark to bright BBD character)

## Parameters (6 Total)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Rate | 0.05 - 5.0 Hz | 1.0 Hz | LFO modulation speed |
| Depth | 0 - 100% | 50% | Modulation amount (delay time variation) |
| Voices | 1 - 8 | 4 | Number of chorus voices |
| Width | 0 - 100% | 70% | Stereo spread of voices |
| Tone | -100% to +100% | 0% | High-frequency rolloff/boost on wet signal |
| Mix | 0 - 100% | 50% | Dry/wet blend |

## Lifecycle Timeline

- **2026-09-30 (v1.9.0):** `/improve-review-info` sweep of IN-01..07. Adds a `reset()` override, pointer-capture knob drag, a 2 Hz preset-name poll, and a Save… that honours the chosen folder (factory dir refused). The knobs are keyboard sliders. The stale header comment is fixed. IN-07 (−3 dB centred wet) is documented, not changed.
- **2026-09-30 (v1.8.0):** `/improve-review` of CR-01 and WR-01..07. Accepts mono→mono and mono→stereo. Tone clamp raised to 0.45·fs. Each Voices-crossfade layer now uses its own layout, and a count that arrives mid-fade is queued. Spread is one-sided (10–25 ms), so no tap is pinned. Drive is level-compensated `tanh(d·x)/d`. The mouse wheel steps Voices. The plugin-side factory guard is removed. auval passes. DAW listening pass pending.
- **2026-09-26 (v1.7.0):** 260924-nho Phase C UI pass via /improve: insect plate reduced to its central specimen behind the LFO ring (no knob face or caption over it), paper texture at 55% so text is AA on the painted ground (min 2.83 → 5.59:1; measure-ui's 0% was a false pass — it cannot see the jpg/img layers), palette custom properties, LFO caption 8 → 9px, bundled EB Garamond. Knob code untouched, so still mouse-only (R7 deferred). No DSP/param/state change.
- **2026-06-30 (v1.2.2):** Code-review fixes (WR-01/WR-02/WR-03) — clamp per-voice delay to a
  positive range (fixes voice collapse at high Spread), pop/push each delay line exactly once
  during a voice-count crossfade (fixes 2× pointer advance / doubling glitch), and clamp the tone
  cutoff to 0.49×Nyquist (fixes filter blow-up at SR ≤ ~40 kHz). No param/state changes. auval PASS.
- **2026-02-07 (Stage 0):** Research & Planning complete
  - Plugin type defined: Multi-voice BBD-style chorus effect
  - Professional examples researched: Strymon Ola, Boss CE-1, Roland Juno-60, D16 Syntorus 2
  - JUCE modules identified: `juce_dsp` (DelayLine, IIR), `juce_audio_processors` (APVTS)
  - DSP feasibility verified: Lagrange3rd interpolation, tanh saturation, one-pole filtering
  - Complexity score: 2.8 (Moderate, single-pass implementation)
  - ARCHITECTURE.md documented (complete DSP specification with JUCE API mappings)
  - ROADMAP.md documented (stage breakdown, ~3.25 hour timeline)

## Known Limitations

- **A centred wet voice plays at −3 dB on stereo outputs** (review IN-07, acknowledged v1.9.0, not changed). Pan is equal-power: centre = cos(π/4) = 0.707 per side. At Width 0 every voice is centred, so Vibrato (mix 1.0, 1 voice) sits 3 dB below bypass. The mono→mono path sums unpanned and is unity. A √2 boost at Voices 1 only would put a 3 dB step between 1 and 2 voices; a Width-aware compensation would move the level of every preset. Left as is.
- v1.8.0 changes the sound of existing presets and sessions with Drive > 0 (the wet path is quieter on quiet material) and with Spread > 0 (voices spread one-sided). Presets were not re-voiced.

## Architecture Highlights

**DSP Chain:**
- **Delay lines:** `juce::dsp::DelayLine` with Lagrange3rd interpolation
- **LFO:** Sine wave with fixed phase distribution: `(2π * voiceIndex) / numVoices`
- **Saturation:** Tanh soft-clipping with subtle asymmetry for BBD warmth
- **Tone:** One-pole IIR lowpass (2kHz - 20kHz, default 8kHz)
- **Stereo imaging:** Equal-power panning across voice array

**Signal Flow:**
```
Input L/R → Mono Sum → [Voice 1-N: LFO → Delay → Sat → Tone → Pan] → Mix → Output L/R
```

**Key Design Decisions:**
- Mono sum input for phase coherence (prevents comb filtering in mono)
- Fixed phase distribution for predictable, symmetrical stereo image
- Lagrange3rd interpolation (optimal quality/CPU balance for slow LFO rates)
- Tanh saturation over WDF diode clipper (90% warmth at 10% CPU cost)

## Implementation Risks

**HIGH:**
- Delay modulation artifacts (clicks/pops at high rate/depth)
  - Mitigation: Lagrange3rd interpolation, 5 Hz max rate, parameter smoothing
  - Fallback: Switch to Thiran interpolation if needed

**MEDIUM:**
- CPU usage with 8 voices (target <10% at 48kHz)
  - Mitigation: Optimize saturation (lookup table), SIMD, block processing
  - Fallback: Reduce max voices to 4, offer "Lite" mode

- Stereo phase coherence in mono sum
  - Mitigation: Mono sum input with stereo from panning only
  - Fallback: "Mono Safe" mode if compatibility critical

**LOW:**
- Parameter smoothing latency (50ms ramps may feel sluggish)
  - Mitigation: 50ms smoothing (fast enough for real-time)
  - Fallback: Reduce to 20ms for "Fast" mode

- Denormal numbers in delay lines when idle
  - Mitigation: ScopedNoDenormals, DC blocker, flush on silence

## Known Issues

None (not yet implemented)

## JUCE Module Dependencies

- `juce_audio_processors` - AudioProcessor, APVTS, parameters
- `juce_dsp` - DelayLine, IIR filters, ProcessSpec
- `juce_core` - MathConstants, Random, utilities

## Timeline Estimate

**Total:** ~3.25 hours

- Stage 1 (Foundation): 30 minutes
- Stage 2 (DSP Implementation): 60 minutes
- Stage 3 (GUI Implementation): 45 minutes
- Stage 4 (Testing & Polish): 30 minutes

## Professional References

**Plugins analyzed during research:**
- **Strymon Ola dBucket Chorus** - Tri-chorus architecture, phase distribution
- **Boss CE-1 Chorus Ensemble** - BBD chip MN3207, analog warmth
- **Roland Juno-60 Chorus** - BBD chip MN3009, quadrature LFO
- **D16 Syntorus 2** - Analog BBD emulation mode, per-voice randomization

**Technical resources:**
- JUCE DelayLine API documentation
- Stanford CCRMA - Delay-Line Interpolation
- Internal research: `delay-effects-comprehensive-guide.md`, `circuit-modeling-fundamentals.md`

## Additional Notes

- Single-pass implementation (no phase breakdown within stages)
- All components available in JUCE 8 (no custom implementations needed)
- Mono compatibility ensured via mono sum input strategy
- Fixed phase distribution ensures repeatable sound across sessions
- Parameter ranges researched from professional chorus implementations

## Next Steps

**Ready for Stage 1 (Foundation):**
- CMakeLists.txt configuration
- PluginProcessor skeleton
- APVTS parameter definition (6 parameters)
- State management (save/load)

**Command:** `/implement O-Chorus`

---

**Last Updated:** 2026-02-07
