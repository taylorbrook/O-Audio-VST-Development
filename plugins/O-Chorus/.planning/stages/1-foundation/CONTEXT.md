# Stage 1: Foundation - Context

## Discussion Summary

**Date:** 2026-02-07
**Participants:** User, Claude

## Requirements Confirmed

- 6 parameters confirmed: Rate, Depth, Voices, Width, Tone, Mix (no changes from ARCHITECTURE.md)
- Full cross-platform WebView setup: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
- Zero latency (no lookahead or latency compensation)
- Stereo in / Stereo out
- VST3 + AU formats
- Single-pass implementation (complexity 2.8)

## Constraints Identified

- Must follow juce8-critical-patterns for WebView setup
- Must include `juce_generate_juce_header()` after `target_link_libraries()`
- Windows WebView2 user data folder required for DAW plugin hosts
- Parameter IDs: RATE, DEPTH, VOICES, WIDTH, TONE, MIX

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| WebView config | Full cross-platform | NEEDS_WEBVIEW2 TRUE + static linking for Windows |
| Parameter count | 6 | Rate, Depth, Voices, Width, Tone, Mix as specified |
| Latency | Zero | Chorus is modulation effect, no lookahead needed |
| APVTS smoothing | 50ms for float params, none for Voices (int) | Prevents clicks during automation |
| Rate skew | Logarithmic | More control in musical range (0.5-2 Hz) |

## Open Questions

- None (all requirements confirmed)

## Next Phase

Ready for: plan phase (then execute)
