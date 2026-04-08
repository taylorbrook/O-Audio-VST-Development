# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Requirements Confirmed

- COMPAT-01: pluginval level 10 for both VST3 and AU (upgrade from level 5)
- COMPAT-03: Wind controller CC2 mapping verified via standard MIDI (no physical wind controller available)
- PERF-02: CPU per voice <2.5% measurement and verification
- PERF-03: Zero algorithmic latency verification (expect ~8 samples from 2x oversampling)
- UI placeholders: Reserve layout space for future breath visualization (UI-06), register indicator (UI-07), and visual polish (UI-08) — no implementation, just reserved DOM elements

## Constraints Identified

- No installer/packaging — build artifacts only; user needs extended manual DAW testing before release
- No physical wind controller for COMPAT-03 — verify CC2 code path works with standard MIDI CC messages
- Plugin has not been tested in DAW yet — Stage 4 may surface issues from real-world use
- Deferred UI items (UI-06, UI-07, UI-08) are nice-tier — placeholders only, no implementation

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| pluginval strictness | Level 10 | Full validation for release confidence |
| Wind controller testing | Standard MIDI CC2 verification | No physical hardware available |
| Deferred UI features | Reserved space in layout, no implementation | Keep scope tight for v1.0 |
| Packaging | None — build artifacts only | User wants manual DAW testing first |
| Installer | Deferred to post-testing | Not needed until user is satisfied with quality |

## Scope

### In Scope
- pluginval level 10 (VST3 + AU)
- CPU profiling and verification
- Latency reporting verification
- CC2 breath mapping verification with standard MIDI
- UI placeholder elements for future visualization features
- Any bug fixes surfaced by pluginval level 10

### Out of Scope
- Breath/jet visualization implementation (UI-06)
- Register indicator implementation (UI-07)
- Visual polish and animations (UI-08)
- PKG installer / distribution packaging
- Expansion presets
- Scala/MTS-ESP tuning

## Open Questions

- Will pluginval level 10 surface issues with the waveguide DSP under rapid parameter automation?
- Is the 2x oversampling latency (~8 samples) correctly reported to the host?

## Next Phase

Ready for: research phase
