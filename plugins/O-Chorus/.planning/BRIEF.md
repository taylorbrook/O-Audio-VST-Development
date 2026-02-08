# O-Chorus - Creative Brief

## Concept
A lush, analog-inspired chorus plugin with rich multi-voice architecture. Inspired by the warmth and thickness of classic hardware units like the Roland Juno-60 chorus and Boss CE-1, but with modern flexibility through a scalable voice count parameter.

## Character
- **Lush analog** — warm, thick, organic modulation
- Gentle high-frequency rolloff on wet signal to emulate analog bucket-brigade delays (BBD)
- Subtle nonlinear saturation for warmth
- Slight randomization in modulation for organic, non-mechanical feel

## Architecture
- **Multi-voice chorus engine** with 1-8 voices (user-selectable)
- Each voice has slightly offset LFO phase for natural spread
- Independent per-voice modulation depth variation for richness
- Stereo spread across the voice array for wide imaging

## Controls (6 parameters)
| Parameter | Range | Description |
|-----------|-------|-------------|
| **Rate** | 0.05 - 5.0 Hz | LFO modulation speed |
| **Depth** | 0 - 100% | Modulation amount (delay time variation) |
| **Voices** | 1 - 8 | Number of chorus voices |
| **Width** | 0 - 100% | Stereo spread of voices |
| **Tone** | -100% to +100% | High-frequency rolloff/boost on wet signal |
| **Mix** | 0 - 100% | Dry/wet blend |

## Signal Flow
```
Input → [Dry path] ──────────────────────────────┐
  │                                                │
  └→ [Voice 1: LFO → Delay → Saturation → Tone] ─┤
  └→ [Voice 2: LFO → Delay → Saturation → Tone] ─┤
  └→ [Voice N: LFO → Delay → Saturation → Tone] ─┼→ [Stereo Panner] → Mix → Output
```

## DSP Notes
- Base delay time: ~5-15ms range (classic chorus territory)
- LFO waveforms: Sine with slight triangle blend for smoothness
- Per-voice LFO phase offset: `(2 * PI * voiceIndex) / numVoices`
- Per-voice depth variation: slight randomized offset (5-15%) for organic quality
- Analog warmth: gentle soft-clipping saturation + 1-pole lowpass on wet signal
- Interpolated delay lines (cubic or allpass) to avoid zipper artifacts

## UI Vision
- Clean, modern interface with warm color palette
- Visual LFO indicator showing modulation movement
- Voice count displayed prominently
- Compact layout — focused and uncluttered

## Target Format
- VST3 + AU (macOS, Windows)
- WebView UI
- Stereo in / Stereo out

## Inspiration
- Roland Juno-60 built-in chorus (the gold standard of lush)
- Boss CE-1 Chorus Ensemble (warm, thick)
- Strymon Ola (modern take on classic chorus)
- TAL-Chorus-LX (software reference)
