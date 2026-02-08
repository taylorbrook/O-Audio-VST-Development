# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-02-08
**Participants:** User, Claude

## Requirements Confirmed

- Fix all 3 issues identified in Stage 3 VERIFICATION.md
- Run pluginval validation (VST3 + AU)
- Create CHANGELOG.md for v1.0.0 release
- No factory presets (deferred to future improvement cycle)
- Keep `-dev` suffix on plugin name/ID (not renaming to production yet)
- No new features — pure polish pass

## Issues to Fix (from Stage 3 Verification)

### Issue 1: Documentation drift (Warning)
- parameter-spec.md and BRIEF.md document 6 parameters, but implementation has 7 (drive added in Stage 2)
- PluginEditor.h line 17 comment says "Parameters: 6 total" — stale
- **Action:** Update all documentation to reflect 7 parameters

### Issue 2: LFO frame rate assumption (Info)
- `lfoLoop` divides by 60 (assumes 60fps) instead of using `requestAnimationFrame` timestamp delta
- On 120Hz/144Hz displays, LFO orbit appears ~2x faster than intended
- **Action:** Refactor to use timestamp-based animation (deltaTime between frames)

### Issue 3: Mouse wheel missing gesture brackets (Info)
- Wheel handler calls `setNormalisedValue` without `sliderDragStarted`/`sliderDragEnded`
- DAW undo may not capture wheel-based changes as discrete operations
- **Action:** Add gesture brackets around wheel events (with debounce for rapid scrolling)

## Constraints Identified

- No preset system needed yet — user wants to add more functionality first
- Keep `-dev` identifiers — not ready for production naming
- v1.0.0 version tag despite `-dev` naming (marks feature-complete for current scope)

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Presets | Deferred | User wants more features before creating presets |
| Plugin naming | Keep -dev | Not ready for production identity |
| Version | v1.0.0 | First complete release of current feature set |
| Issue fixes | All 3 | Clean sweep of known issues before release |
| Performance optimization | Only if pluginval flags issues | DSP already verified in Stage 2 |

## Validation Plan

- Run `pluginval` on both VST3 and AU builds
- Verify AU detection with `auval`
- Build all 3 formats (VST3, AU, Standalone) with zero errors/warnings

## Open Questions

- None — scope is clear

## Next Phase

Ready for: research/plan phase
