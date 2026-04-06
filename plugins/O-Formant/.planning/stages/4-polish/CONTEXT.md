# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Requirements Confirmed

- **FUNC-12:** 16 factory presets across 4 categories (Cinematic, Electronic, Ambient, Speech) — 4 presets per category
- **COMPAT-01:** pluginval level 10 validation (upgrade from level 5 at Stage 3)
- **Preset system:** OuariconPresetManager module (JSON-based, category browser, save/load) — matches O-Bells/O-Bass/O-DigiDelay pattern
- **Preset UI:** WebView preset browser integrated into existing Naturalist UI (prev/next arrows, category dropdown, save/load buttons)
- **CHANGELOG.md:** Document v1.0.0 release
- **Build and install:** Final VST3 + AU to system folders

## Issues Found During Audit

### Critical: Phase 2.3 (Output Stage) Was Never Implemented

Two parameters are **defined in APVTS, cached in FormantVoice, and present in WebView UI** but **never applied in DSP**:

| Parameter | ID | Status |
|-----------|-----|--------|
| Output Gain | outputGain | Cached in FormantVoice.h:105 but never read in renderNextBlock. processBlock has no post-processing gain. |
| Stereo Width | stereoWidth | Cached in FormantVoice.h:106 but never used. Voice writes mono to both channels (FormantVoice.cpp:270 comment: "stereo spread is Phase 2.3"). |

**Action:** Wire both parameters in DSP as part of Stage 4 polish.

- **outputGain:** Apply juce::dsp::Gain (dB) in processBlock after synthesiser.renderNextBlock
- **stereoWidth:** Per-voice panning by pitch in FormantVoice::renderNextBlock (pan = pitchNorm * stereoWidth, apply to L/R channels)

### Minor: Parameter Smoothing Assessment

| Component | Smoothing | Risk |
|-----------|-----------|------|
| breathiness | SmoothedValue (20ms) | None |
| Vowel params (X, Y, Focus) | 32-sample block update | Low — coefficient updates smooth transitions |
| Formant Shift/Spread | 32-sample block update | Low |
| Vibrato Rate/Depth/Delay | Block-rate | None — LFO rate changes are inherently smooth |
| ADSR params | Block-rate | None — ADSR recalc is block-rate by design |
| Consonant params | Block-rate | Low |
| pitchGlide | Own one-pole smoother | None |
| **outputGain** | **NOT IMPLEMENTED** | **Must add SmoothedValue when wiring** |
| **stereoWidth** | **NOT IMPLEMENTED** | **Must add SmoothedValue when wiring** |

**Action:** Add SmoothedValue for outputGain (~50ms ramp to avoid clicks on gain changes). stereoWidth can use block-rate update (panning changes are slow).

## Constraints Identified

- OuariconPresetManager.h must be copied from O-Bells (module pattern — each plugin gets its own copy)
- Factory presets stored as JSON in ~/Library/O-Formant/Presets/Factory/{Category}/
- Preset browser HTML/CSS/JS must match Naturalist aesthetic (paper bg, Garamond, moss-green accents)
- pluginval level 10 includes heavy parameter randomization — outputGain and stereoWidth must be wired and safe before running

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Preset system | OuariconPresetManager module | Consistency with suite (O-Bells, O-Bass, O-DigiDelay pattern) |
| Preset count | 16 factory (4 per category) | Matches BRIEF.md spec |
| Preset UI | WebView browser (prev/next, category dropdown, save/load) | Matches O-Bells pattern, integrated into existing WebView |
| pluginval level | 10 (strictest) | Polish stage — ship-quality validation |
| Output gain impl | juce::dsp::Gain in processBlock (post-synth) | Standard pattern, SmoothedValue for click-free |
| Stereo width impl | Per-voice pan by pitch in renderNextBlock | Matches ARCHITECTURE.md spec |
| Missing DSP | Wire outputGain + stereoWidth as part of polish | Must be done before presets (presets need all params functional) |

## Preset Definitions

### Cinematic (4)
| Preset | Character | Key Parameters |
|--------|-----------|----------------|
| Creature Growl | Low, pressed, dark consonants | Rd=0.4, vowelX=0.8, breathiness=0.3, consonantLevel=0.6, formantShift=-12 |
| Alien Whisper | Breathy, high formants, sibilant | Rd=2.5, breathiness=0.7, sibilance=0.6, formantShift=+12, vowelFocus=1.5 |
| Sci-Fi Choir | Modal, wide stereo, slow vibrato | Rd=1.0, stereoWidth=0.9, vibratoRate=4.0, vibratoDepth=20, sustain=1.0 |
| Spectral Voice | Ethereal, spread formants, focused | Rd=1.8, formantSpread=1.8, vowelFocus=4.0, breathiness=0.2, release=3.0 |

### Electronic (4)
| Preset | Character | Key Parameters |
|--------|-----------|----------------|
| Formant Bass | Low, pressed, tight envelope | Rd=0.3, formantShift=-18, attack=0.001, decay=0.5, sustain=0.4, vowelX=0.8 |
| Vowel Pad | Warm, wide, slow morph | Rd=1.2, attack=0.8, sustain=1.0, release=2.0, stereoWidth=0.7, vibratoDepth=8 |
| Glitch Vocal | Bright, consonant-heavy, fast attack | consonantLevel=0.8, sibilance=0.7, autoConsonant=on, attack=0.001, Rd=0.5 |
| Robotic Speech | Modal, tight focus, no vibrato | Rd=1.0, vowelFocus=5.5, vibratoDepth=0, formantSpread=0.8, breathiness=0.0 |

### Ambient (4)
| Preset | Character | Key Parameters |
|--------|-----------|----------------|
| Ethereal Drone | Breathy, long attack, wide | Rd=2.2, attack=2.0, sustain=1.0, release=5.0, breathiness=0.5, stereoWidth=0.8 |
| Breath Texture | Pure noise, minimal formant | Rd=2.7, breathiness=0.9, vowelFocus=1.0, consonantLevel=0.4, formantSpread=1.5 |
| Overtone Chant | Modal, focused, narrow spread | Rd=0.8, vowelFocus=5.0, formantSpread=0.7, vibratoRate=5.5, vibratoDelay=500 |
| Wind Voice | Breathy with consonant air | Rd=2.0, breathiness=0.6, consonantLevel=0.5, consonantTone=0.3, release=4.0 |

### Speech (4)
| Preset | Character | Key Parameters |
|--------|-----------|----------------|
| Natural Tenor | Modal male voice, balanced | Rd=1.0, formantShift=0, breathiness=0.1, vibratoRate=5.5, vibratoDepth=15 |
| Breathy Soprano | High, airy, gentle vibrato | Rd=2.0, formantShift=+8, breathiness=0.4, vibratoRate=6.0, vibratoDepth=12 |
| Pressed Baritone | Low, tight, powerful | Rd=0.4, formantShift=-8, breathiness=0.0, vibratoDepth=10, vowelFocus=3.0 |
| Child Voice | High formants, light, breathy | Rd=1.5, formantShift=+18, breathiness=0.2, formantSpread=1.3, vibratoDepth=5 |

## Implementation Phases

Stage 4 should be split into 2 phases:

### Phase 4.1: DSP Completion + Presets
1. Wire outputGain (SmoothedValue, dB->linear, post-synth in processBlock)
2. Wire stereoWidth (per-voice pan by pitch in renderNextBlock)
3. Copy OuariconPresetManager.h from O-Bells
4. Add PresetManager to PluginProcessor (include, member, accessor, initializeFactoryPresets)
5. Define 16 factory presets with all 21 parameter values
6. Add 10 native functions to PluginEditor for preset communication
7. Add preset browser HTML/CSS/JS to index.html (Naturalist style)
8. Build, test preset loading, verify outputGain + stereoWidth audible

### Phase 4.2: Validation + Release
1. pluginval level 10 (VST3 + AU)
2. Fix any issues found
3. CHANGELOG.md (v1.0.0)
4. Final build and install to system folders
5. State persistence verification (save/load DAW session with preset selected)

## Open Questions

None — all decisions made.

## Next Phase

Ready for: research phase
