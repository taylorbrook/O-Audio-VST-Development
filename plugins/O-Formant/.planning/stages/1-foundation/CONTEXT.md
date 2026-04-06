# Stage 1: Foundation - Context

**Plugin:** O-Formant
**Stage:** 1 of 4 (Foundation)
**Goal:** Build system, APVTS with all 21 parameters, MPESynthesiser skeleton with 16 silent voices

---

## What We're Building

A compilable JUCE plugin shell that:
1. Registers as a synth instrument (VST3 + AU)
2. Accepts MIDI input via MPESynthesiser with legacy mode fallback
3. Allocates 16 FormantVoice instances (MPESynthesiserVoice subclass)
4. Exposes all 21 parameters via APVTS with correct ranges, defaults, and skews
5. Outputs stereo silence (voices produce no audio yet)
6. Passes pluginval basic scan

## Key Decisions (from Stage 0)

- **Voice framework:** `juce::MPESynthesiser` + `MPESynthesiserVoice` with `enableLegacyMode(2, Range<int>(1, 17))`
- **Bus layout:** Output-only stereo (`BusesProperties().withOutput(...)`)
- **Plugin code:** OuFm (unique 4-char)
- **JUCE modules:** juce_audio_basics, juce_audio_processors, juce_dsp, juce_core, juce_gui_basics, juce_gui_extra
- **No WebView yet** - generic editor placeholder for Stage 1
- **No Ouaricon modules yet** - webview-relay-manager added in Stage 3

## Parameters (21 total)

### Vowel Morph (3)
| ID | Range | Default | Skew |
|----|-------|---------|------|
| vowelX | 0.0-1.0 | 0.5 | 1.0 (linear) |
| vowelY | 0.0-1.0 | 0.5 | 1.0 (linear) |
| vowelFocus | 1.0-6.0 | 2.5 | 1.0 (linear) |

### Glottal Source (5)
| ID | Range | Default | Skew |
|----|-------|---------|------|
| glottalRd | 0.3-2.7 | 1.0 | 1.0 (linear) |
| breathiness | 0.0-1.0 | 0.1 | 1.0 (linear) |
| vibratoRate | 0.5-12.0 | 5.5 | 1.0 (linear) |
| vibratoDepth | 0.0-100.0 | 15.0 | 1.0 (linear) |
| vibratoDelay | 0.0-2000.0 | 300.0 | 0.4 (fast-skewed) |

### Consonant / Noise (4)
| ID | Range | Default | Skew |
|----|-------|---------|------|
| consonantLevel | 0.0-1.0 | 0.3 | 1.0 (linear) |
| consonantTone | 0.0-1.0 | 0.5 | 1.0 (linear) |
| sibilance | 0.0-1.0 | 0.0 | 1.0 (linear) |
| autoConsonant | off/on | off | N/A (bool) |

### Envelope (4)
| ID | Range | Default | Skew |
|----|-------|---------|------|
| attack | 0.001-5.0 | 0.01 | 0.3 (fast-skewed) |
| decay | 0.001-5.0 | 0.3 | 0.3 (fast-skewed) |
| sustain | 0.0-1.0 | 0.8 | 1.0 (linear) |
| release | 0.001-10.0 | 0.5 | 0.3 (fast-skewed) |

### Voice Character (3)
| ID | Range | Default | Skew |
|----|-------|---------|------|
| formantShift | -24.0-24.0 | 0.0 | 1.0 (linear) |
| formantSpread | 0.5-2.0 | 1.0 | 1.0 (linear) |
| pitchGlide | 0.0-1000.0 | 0.0 | 0.3 (fast-skewed) |

### Output (2)
| ID | Range | Default | Skew |
|----|-------|---------|------|
| outputGain | -60.0-12.0 | 0.0 | 1.0 (linear, dB scale) |
| stereoWidth | 0.0-1.0 | 0.5 | 1.0 (linear) |

## Reference Patterns

- **O-Prism:** CMakeLists.txt structure, APVTS layout, voice architecture (uses basic Synthesiser, we use MPESynthesiser)
- **ARCHITECTURE.md:** Immutable DSP contract for all stages

## Success Criteria

- [ ] Plugin builds (VST3 + AU) with no warnings
- [ ] Plugin loads in DAW as instrument
- [ ] MIDI notes trigger voice allocation (debug log)
- [ ] All 21 parameters visible in DAW automation
- [ ] pluginval passes basic scan
