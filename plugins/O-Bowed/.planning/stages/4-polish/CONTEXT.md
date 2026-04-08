# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Requirements Confirmed

- FUNC-10: All 7 instrument presets (violin, cello, viola, bass, erhu, sarangi, nyckelharpa)
- FUNC-11: All 4 sound design presets (glass bow, metal drone, impossible strings, breath of strings)
- COMPAT-01: Pluginval level 10 validation (VST3 and AU)
- PERF-02/03: Informal CPU check — "reasonable" performance, no formal benchmark
- Changelog: v1.0.0 CHANGELOG.md in plugin folder

## Constraints Identified

- No preset directory exists yet — create from scratch
- 11 total presets: 7 realistic instruments + 4 sound design
- Presets use OuariconPresetManager (already integrated in processor from Stage 3)
- No issues flagged from Stage 3 — no bug fixes needed

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Preset count | All 11 from BRIEF | User confirmed full set |
| Pluginval level | 10 (strictest) | Final validation for release |
| CPU profiling | Informal check | User wants "reasonable", not formal benchmark |
| Versioning | v1.0.0 | Standard initial release |
| Bug fixes | None | Stage 3 looks good per user |

## Preset Specifications

### Realistic Instruments (FUNC-10)

| Preset | Body Material | Body Size | Strings | Sympathetic | Key Settings |
|--------|--------------|-----------|---------|-------------|--------------|
| Violin | ~40% (wood) | ~30% (small) | 4 (G-D-A-E) | 0 | Standard bow params |
| Cello | ~40% (wood) | ~70% (large) | 4 (C-G-D-A) | 0 | Deeper brightness |
| Viola | ~40% (wood) | ~45% (medium) | 4 (C-G-D-A) | 0 | Between violin/cello |
| Double Bass | ~40% (wood) | ~90% (large) | 4 (E-A-D-G) | 0 | Low brightness, high size |
| Erhu | ~15% (membrane) | ~30% (small) | 1 | 0 | High rosin, nasal body |
| Sarangi | ~15% (membrane) | ~45% (medium) | 1 | 4-6 | Sympathetic active |
| Nyckelharpa | ~40% (wood) | ~40% (medium) | 4 | 8-12 | Sympathetic strings prominent |

### Sound Design (FUNC-11)

| Preset | Body Material | Key Settings |
|--------|--------------|--------------|
| Glass Bow | ~90% (glass) | High brightness, infinite sustain |
| Metal Drone | ~70% (metal) | Sub-harmonics, reversed friction |
| Impossible Strings | Mixed | All impossible physics active |
| Breath of Strings | ~40% (wood) | Low pressure, bow noise emphasis, ethereal |

## Open Questions

- None — scope is clear.

## Next Phase

Ready for: research phase
