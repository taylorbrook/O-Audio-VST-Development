# O-Chorus Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.9.0 (last published release: v1.6.3, so v1.7.0–v1.9.0 are unreleased)
- **Type:** Audio Effect (Multi-Voice BBD-Style Chorus)
- **Formats:** VST3, AU (`aufx OuCh OuDv`), Standalone
- **Layouts:** stereo→stereo, mono→stereo, mono→mono (stereo→mono refused)
- **UI:** WebView, 700 × 125, languages en / fr / zh-Hans
- **Review:** `CODE_REVIEW.md` (2026-09-30), all findings resolved or acknowledged

## Overview

O-Chorus is a lush, analog-inspired multi-voice chorus plugin with 1-8 selectable voices. Inspired by classic hardware units like the Roland Juno-60 chorus and Boss CE-1, it combines modern flexibility with BBD-style warmth through modulated delay lines, soft saturation, and tone control.

## Parameters (8 Total)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `rate` | 0.05 – 5.0 Hz (skew 0.35) | 1.0 Hz | LFO speed |
| `depth` | 0 – 100% | 50% | Delay modulation, ±5 ms at full depth (× per-voice 0.85–1.15) |
| `voices` | 1 – 8 (int) | 4 | Voice count; changes crossfade over 50 ms |
| `spread` | 0 – 100% | 0% | One-sided per-voice delay offset, 0 – 15 ms across the voices |
| `width` | 0 – 100% | 70% | Equal-power pan spread of the voices |
| `tone` | −100% – +100% | 0% | Wet low-pass cutoff: 2 kHz → 8 kHz → 20 kHz, clamped to 0.45·fs |
| `mix` | 0 – 100% | 50% | Dry/wet |
| `drive` | 0 – 100% | 30% | Per-voice `tanh(d·x)/d`, d = 4^drive (negative half 4^(0.9·drive)); 0 = bit-exact bypass |

Factory presets (6): Classic, Lush, Shimmer, Ensemble, Vibrato, Warm.

## Lifecycle Timeline

- **2026-09-30 (v1.9.0):** `/improve-review-info` sweep of IN-01..07. Adds a `reset()` override, pointer-capture knob drag, a 2 Hz preset-name poll, and a Save… that honours the chosen folder (factory dir refused). The knobs are keyboard sliders. The stale header comment is fixed. IN-07 (−3 dB centred wet) is documented, not changed. DAW-checked OK 2026-09-30.
- **2026-09-30 (v1.8.0):** `/improve-review` of CR-01 and WR-01..07. Accepts mono→mono and mono→stereo. Tone clamp raised to 0.45·fs. Each Voices-crossfade layer now uses its own layout, and a count that arrives mid-fade is queued. Spread is one-sided (10–25 ms), so no tap is pinned. Drive is level-compensated `tanh(d·x)/d`. The mouse wheel steps Voices. The plugin-side factory guard is removed. auval passes. DAW listening pass pending.
- **2026-09-26 (v1.7.0):** 260924-nho Phase C UI pass via /improve: insect plate reduced to its central specimen behind the LFO ring (no knob face or caption over it), paper texture at 55% so text is AA on the painted ground (min 2.83 → 5.59:1; measure-ui's 0% was a false pass — it cannot see the jpg/img layers), palette custom properties, LFO caption 8 → 9px, bundled EB Garamond. Knob code untouched, so still mouse-only (R7 deferred). No DSP/param/state change.
- **v1.0.0–v1.6.3 (2026-02-08 → 2026-09-08):** see CHANGELOG.md; only the v1.2.2 review pass is summarised here.
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

## Architecture (as built)

```
In L/R → mono sum ─┬→ voice 1..N: LFO-modulated DelayLine (Lagrange3rd) → drive → equal-power pan ─┐
                   │                                                                               ├→ tone biquad LP (per side) → mix with dry → Out L/R
                   └────────────────────────────── dry ────────────────────────────────────────────┘
```

- **Delay:** base 10 ms + spread offset (0–15 ms) + LFO × depth × 5 ms; taps stay within 4.25–30.75 ms of a 50 ms line.
- **LFO:** one sine, per-voice phase offset 2π·v/N.
- **Voice-count changes:** 50 ms crossfade; each layer uses its own phase/pan/spread layout; a change arriving mid-fade is queued.
- **Tone:** 2nd-order low-pass (Q = 1/√2) on the wet sum, per side.
- **Mono→mono:** voices summed unpanned (unity), same tone filter.
- **Smoothing:** 50 ms on all continuous params, 100 ms on tone.
- **Host `reset()`** clears delay lines, filters, LFO phase and crossfade state.

## Known Issues

None open.

## JUCE Module Dependencies

- `juce_audio_processors` - AudioProcessor, APVTS, parameters
- `juce_dsp` - DelayLine, IIR filters, ProcessSpec
- `juce_gui_extra` - WebBrowserComponent
- Ouaricon module: `preset-manager`

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

---

**Last Updated:** 2026-09-30
