# Stage 4 Phase 4.2: Validation + Release - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Requirements Confirmed

- **COMPAT-01:** pluginval level 10 validation (VST3 + AU) — strictest level, includes heavy parameter randomization
- **CHANGELOG.md:** Standard format documenting v1.0.0 release across all stages
- **State persistence:** Preset name survives DAW save/load (verified in 4.1, re-confirm after any fixes)
- **Final build + install:** VST3 + AU to system folders

## Constraints Identified

- pluginval level 5 already passes (verified in Phase 4.1)
- Level 10 adds aggressive parameter randomization, rapid state changes, edge-case buffer sizes
- Any failures at level 10 must be fixed before release
- No cross-platform or DAW-specific testing required for v1.0.0

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| pluginval level | 10 (strictest) | Ship-quality validation for v1.0 release |
| CHANGELOG format | Standard (all stages summarized) | Consistent with project conventions |
| State persistence scope | Preset name round-trip | Full parameter persistence already handled by APVTS XML |
| Additional testing | None beyond pluginval + auval | User confirmed no extras needed |

## Phase 4.2 Tasks

1. Run pluginval level 10 on VST3
2. Run pluginval level 10 on AU (or auval re-verify)
3. Fix any issues found at level 10
4. Write CHANGELOG.md (v1.0.0, standard format)
5. Final ninja build (clean)
6. Install VST3 + AU to system folders
7. Verify preset name persists across DAW save/load

## Open Questions

None — all decisions made.

## Next Phase

Ready for: research phase
