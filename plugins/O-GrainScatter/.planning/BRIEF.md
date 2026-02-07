# O-GrainScatter Creative Brief

## Plugin Identity
- **Name:** O-GrainScatter
- **Tagline:** "Harmonic Stutter — From rhythmic repeats to granular clouds"
- **Type:** Audio Effect (Granular Stutter Engine)
- **Brand:** Ouaricon Audio

## Core Concept

O-GrainScatter is a **granular stutter engine** that bridges the gap between rhythmic stutter effects and ambient granular textures. It captures incoming audio into a delay buffer and re-triggers it as grains — either beat-synchronized for precise stutters, or density-based for evolving cloud textures. A Texture morphing control provides seamless transition between tight, unison stutters and scattered granular clouds.

### Unique Value Proposition: "Harmonic Stutter"
The only stutter effect that combines:
- **64-voice granular synthesis** with voice pool management
- **Musical scale quantization** (pitches snap to chords/scales)
- **Beat-synchronized triggering** with Euclidean rhythm patterns
- **Density-based texture morphing** (stutter to cloud continuum)

No commercial plugin offers this combination. Portal ($99) has granular + scales. Stutter Edit ($99) has beat-sync + gestures. O-GrainScatter merges both paradigms into one focused instrument.

## Heritage

Built on the granular foundation from the **Scatter** plugin (TACHES), which provides:
- 64-voice grain pool with Lagrange3rd interpolation
- Hann window envelope per grain
- Scale-quantized pitch randomization (Chromatic, Major, Minor, Pentatonic, Whole Tone)
- Density-based grain scheduling
- Dry/wet mixing with feedback
- WebView UI with grain visualization

O-GrainScatter is a **new Ouaricon plugin** that takes this foundation and extends it with beat-sync, freeze, pitch ladder modes, Euclidean rhythms, and the stutter-to-cloud texture morph.

## Target Users
- **Sound designers** creating unique textures from any source material
- **Electronic producers** wanting glitchy, rhythmic grain effects on drums/synths
- **Ambient artists** building evolving granular soundscapes
- **Live performers** seeking real-time granular manipulation
- **Beat makers** looking for musical stutter effects that stay in key

## Use Cases
1. **Rhythmic stutter** — Beat-synced grain repeats on drums, perfectly in time
2. **Harmonic clouds** — Freeze a chord and scatter grains across a scale for ambient pads
3. **Glitch textures** — Euclidean patterns + pitch ladder for evolving rhythmic textures
4. **Transition effects** — Morph from clean stutter to dense cloud over a build
5. **Live grain manipulation** — Freeze, scatter, release in real-time performance

## Architecture Reference

Detailed DSP architecture, code snippets, and implementation phases documented in:
`research/stutter-effects/path-a-granular-stutter-engine.md`

### Signal Flow
```
Input -> Delay Buffer (2s) -> Grain Scheduler -> Voice Pool (64 voices)
                                   |                      |
                              Beat Clock            Scale Quantizer
                              Euclidean              Pitch Ladder
                              Freeze Control         Texture Morph
                                                          |
                                                    Dry/Wet Mix -> Output
```

## Parameters

### Core Granular Engine (from Scatter heritage)
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Grain Size | 10-500 ms | 100 ms | Duration of each grain |
| Density | 1-100% | 50% | Grain spawn rate (Free mode) |
| Pitch Random | 0-100% | 0% | Pitch randomization amount |
| Pan Random | 0-100% | 0% | Stereo spread of grains |
| Scale | Chromatic/Major/Minor/Penta/Whole | Chromatic | Pitch quantization scale |
| Root Note | C-B | C | Root note for scale quantization |
| Reverse | 0-100% | 0% | Chance of reverse grain playback |
| Feedback | 0-100% | 0% | Grain output fed back to delay buffer |
| Dry/Wet | 0-100% | 50% | Mix control |

### Beat Sync (new)
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Sync Mode | Free/1/4/1/8/1/16/1/32/1/8T/1/16T | Free | Grain trigger timing |
| Probability | 0-100% | 100% | Chance of triggering per beat division |
| Repeats | 1-16 | 4 | Repeat count before returning to live |

### Texture & Pitch (new)
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Texture | 0-100% | 0% | Stutter (0%) to Cloud (100%) morph |
| Pitch Mode | Random/Ladder Up/Ladder Down/Pendulum | Random | Grain pitch sequencing mode |
| Freeze | On/Off | Off | Capture and loop current buffer |

### Euclidean Rhythms (new)
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Pulses | 1-16 | 4 | Active hits in Euclidean pattern |
| Steps | 2-16 | 8 | Total steps in Euclidean pattern |

**Total: ~20 parameters** (deep & flexible, with sensible defaults for quick use)

## Technical Requirements
- **Audio:** Stereo in/stereo out effect
- **Buffer:** 2-second delay buffer with Lagrange3rd interpolation
- **Voices:** 64 pre-allocated grain voices
- **Tempo:** DAW tempo sync via AudioPlayHead (PPQ-based), manual fallback for standalone
- **Window:** Hann window envelope per grain
- **UI:** WebView-based with grain visualization, Euclidean pattern visualizer
- **Formats:** VST3 + AU

## Inspiration & References
- **Output Portal** — Granular FX with scale quantization
- **iZotope Stutter Edit 2** — Beat-synced gesture-based stutter
- **Granulator II (Robert Henke)** — Max/MSP granular instrument
- **Scatter (TACHES)** — Direct granular engine ancestor
- **O-Freeze** — Ouaricon granular freeze effect (related but distinct approach)

## Differentiation from O-Freeze
O-Freeze is a **spectral freeze** effect (FFT-based, infinite sustain of frozen moments). O-GrainScatter is a **granular stutter engine** (time-domain grain manipulation with rhythmic and harmonic control). They complement each other — O-Freeze for static textures, O-GrainScatter for rhythmic and evolving grain effects.
