# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Phase Restructuring

ROADMAP originally specified 3 phases (3.1 Layout, 3.2 Binding, 3.3 Polish). User merged 3.1+3.2 into a single phase:

| Phase | Goal |
|-------|------|
| 3.1 | Layout + Controls + Parameter Binding (all 21 params wired) |
| 3.2 | Formant peaks overlay, cursor glow, visual polish |

No mockup iteration -- straight to code based on BRIEF layout spec.

## Requirements Targeted

| Requirement | Priority | Phase |
|-------------|----------|-------|
| UI-01: 2D XY vowel morph pad with draggable cursor and vowel labels | must | 3.1 |
| UI-02: Real-time formant peaks overlay (F1-F5 frequency bars) | nice | 3.2 |
| UI-03: Organized parameter layout (Glottal, Consonant, Character groups) | nice | 3.1 |

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Visual aesthetic | Ouaricon Naturalist | Brand consistency across all plugins |
| Window size | 800x600 | Good balance: ~350x350 XY pad, room for all groups |
| Vowel labels | IPA phonetic symbols | Scientific/academic feel matching naturalist theme |
| Botanical illustration | Songbird / throat anatomy | Direct reference to vocal production and voice synthesis |
| Mockup step | Skipped | BRIEF layout spec is clear; straight to implementation |
| Phase merge | 3.1+3.2 combined | No value in non-functional UI step; build layout and wire params together |

## Layout Specification (from BRIEF)

**Two-column layout:**
- **Left:** Large XY vowel morph pad (~350x350) with 5 IPA vowel labels at acoustic positions
- **Right:** Parameter groups stacked vertically (Glottal, Consonant, Character)
- **Bottom:** ADSR envelope + Output (gain, stereo width)

### Vowel Positions (normalized 0-1, from VowelData)
| Vowel | IPA | X | Y |
|-------|-----|---|---|
| I | i | 0.00 | 1.00 |
| E | e | 0.31 | 0.43 |
| A | ɑ | 0.83 | 0.00 |
| O | o | 1.00 | 0.35 |
| U | u | 0.98 | 0.93 |

### Parameter Groups (Right Column)
**Glottal Source (5):** glottalRd, breathiness, vibratoRate, vibratoDepth, vibratoDelay
**Consonant/Noise (4):** consonantLevel, consonantTone, sibilance, autoConsonant (toggle)
**Voice Character (3):** formantShift, formantSpread, pitchGlide

### Bottom Row
**Envelope (4):** attack, decay, sustain, release
**Output (2):** outputGain, stereoWidth
**Vowel Focus (1):** vowelFocus (could go near XY pad or with character)

## Visual Style: Ouaricon Naturalist

- **Background:** Aged paper #F5E6D3 with subtle grain texture
- **Text:** Garamond serif, dark warm brown #3C2F2F
- **Knobs:** 10-segment botanical seed cross-section pattern (55-60px)
- **Accents:** Moss green #8BA870 for active states
- **Borders:** 2px solid walnut brown #8B7355
- **Botanical:** Songbird throat anatomy illustration, right side, 0.35 opacity
- **Shadows:** Subtle depth (inset + drop shadows on controls)

## WebView Technical Requirements

- JUCE 8 WebView with resource provider
- `juce://juce.backend/` scheme (macOS), `https://juce.backend/` (Windows)
- WebSliderRelay + WebSliderParameterAttachment for 21 parameters
- Custom XY pad relay: single 2D control binding to vowelX + vowelY simultaneously
- autoConsonant: WebToggleButtonRelay + WebToggleButtonParameterAttachment
- NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 for Windows

## Parameters to Bind (21 total)

### Sliders (20)
vowelX, vowelY, vowelFocus, glottalRd, breathiness, vibratoRate, vibratoDepth, vibratoDelay, consonantLevel, consonantTone, sibilance, formantShift, formantSpread, pitchGlide, attack, decay, sustain, release, outputGain, stereoWidth

### Toggle (1)
autoConsonant

### Special Binding
- XY pad canvas: drag sends both vowelX + vowelY updates per frame
- Host automation of vowelX/vowelY must update cursor position on pad

## Constraints

- No viewport units in CSS (WebView limitation)
- Resource provider receives bare paths (not full URLs)
- Must set WinWebView2 user data folder to temp directory
- Formant peaks overlay deferred to Phase 3.2 (requires DSP -> UI data channel)
- Cursor glow deferred to Phase 3.2

## Open Questions

- Exact vowelFocus knob placement (near XY pad as "focus" control, or with Character group?)
- Formant peak data channel for Phase 3.2 (polling vs push from processBlock?)

## Dependencies

- **Requires:** Stage 2 Phase 2.3 completion (outputGain + stereoWidth params must be functional)
- **References:** Ouaricon Naturalist aesthetic at `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`
- **Pattern references:** O-Prism WebView (synth with similar relay setup), O-AnalogEQ (aesthetic reference)

## Next Phase

Ready for: research phase (WebView relay patterns, XY pad implementation, reference plugin code)
