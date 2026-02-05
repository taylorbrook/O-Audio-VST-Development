---
phase: quick-004
plan: 01
subsystem: build
tags: [cmake, ci, branding, github-actions, plugin-naming]

# Dependency graph
requires: []
provides:
  - "OUARICON_RELEASE CMake option for dev vs release branding"
  - "Variable-based plugin branding across all 17 plugins"
  - "CI workflow with release flag on all 3 platforms"
affects: [ci-cd, plugin-releases, new-plugin-scaffolding]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "CMake option-driven branding: OUARICON_RELEASE controls company name, manufacturer code, product name suffix"
    - "Variable propagation: root CMakeLists.txt sets variables, plugin CMakeLists.txt consumes them"

key-files:
  modified:
    - CMakeLists.txt
    - .claude/branding.json
    - .github/workflows/build-and-release.yml
    - plugins/O-AnalogEQ/CMakeLists.txt
    - plugins/O-AnalogSaturation/CMakeLists.txt
    - plugins/O-Bass/CMakeLists.txt
    - plugins/O-Bells/CMakeLists.txt
    - plugins/O-Comp/CMakeLists.txt
    - plugins/O-Detune/CMakeLists.txt
    - plugins/O-DigiDelay/CMakeLists.txt
    - plugins/O-Freeze/CMakeLists.txt
    - plugins/O-FreqPulse/CMakeLists.txt
    - plugins/O-IntonationPad/CMakeLists.txt
    - plugins/O-Lyrica/CMakeLists.txt
    - plugins/O-Marimba/CMakeLists.txt
    - plugins/O-MultiBandCompressor/CMakeLists.txt
    - plugins/O-Polystutter/CMakeLists.txt
    - plugins/O-SimpleReverb/CMakeLists.txt
    - plugins/O-SpectralShaper/CMakeLists.txt
    - plugins/O-Tremolo/CMakeLists.txt

key-decisions:
  - "OUARICON_RELEASE defaults to OFF so local builds always produce -dev suffix plugins"
  - "Dev company name is 'Ouaricon Audio Development' (not 'Ouaricon Development') for consistency with production 'Ouaricon Audio'"
  - "Manufacturer code OuDv for dev, OuAu for release — 4-char codes as required by JUCE"

patterns-established:
  - "New plugins must use ${OUARICON_COMPANY_NAME}, ${OUARICON_MANUFACTURER_CODE}, and ${OUARICON_DEV_SUFFIX} in juce_add_plugin()"
  - "CI release builds always pass -DOUARICON_RELEASE=ON alongside -DOUARICON_LICENSING=ON"

# Metrics
duration: 5min
completed: 2026-02-05
---

# Quick Task 004: Dev vs Release Plugin Naming Summary

**CMake OUARICON_RELEASE option with -dev suffix and "Ouaricon Audio Development" branding for local builds, clean "Ouaricon Audio" branding for CI releases**

## Performance

- **Duration:** ~5 min
- **Started:** 2026-02-05T22:58:27Z
- **Completed:** 2026-02-05T23:03:00Z
- **Tasks:** 3/3
- **Files modified:** 20

## Accomplishments
- Root CMakeLists.txt defines OUARICON_RELEASE option (OFF by default) with three derived variables
- All 17 plugin CMakeLists.txt files use variables instead of hardcoded branding values
- GitHub Actions workflow passes -DOUARICON_RELEASE=ON on macOS, Windows, and Linux
- PKG installer Conclusion.txt corrected from "Ouaricon Development" to "Ouaricon Audio"
- branding.json updated with "Ouaricon Audio Development" for development builds

## Task Commits

Each task was committed atomically:

1. **Task 1: Add OUARICON_RELEASE option and branding variables to root CMakeLists.txt** - `25ae8fc` (feat)
2. **Task 2: Update all 17 plugin CMakeLists.txt to use branding variables** - `6048215` (feat)
3. **Task 3: Add OUARICON_RELEASE=ON to GitHub Actions workflow and fix PKG installer text** - `73b0e10` (feat)

## Files Created/Modified
- `CMakeLists.txt` - Added OUARICON_RELEASE option and branding variable definitions
- `.claude/branding.json` - Updated development.full_name to "Ouaricon Audio Development"
- `.github/workflows/build-and-release.yml` - Added -DOUARICON_RELEASE=ON on all 3 platforms, fixed PKG Conclusion.txt
- `plugins/O-*/CMakeLists.txt` (17 files) - Replaced hardcoded COMPANY_NAME, PLUGIN_MANUFACTURER_CODE, and PRODUCT_NAME with CMake variables

## Decisions Made
- OUARICON_RELEASE defaults to OFF so developers always get clearly-labeled dev builds
- Dev company name changed from "Ouaricon Development" to "Ouaricon Audio Development" for consistency with the production "Ouaricon Audio" brand
- The -dev suffix appends to PRODUCT_NAME (e.g., "O-Tremolo-dev") so dev and release plugins appear as distinct entries in DAW plugin lists

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Steps
- New plugins created in the future must follow the variable-based pattern (use ${OUARICON_COMPANY_NAME}, etc.)
- Consider updating plugin scaffolding/template to include these variables by default

## Self-Check: PASSED

---
*Quick task: 004-dev-vs-release-plugin-naming*
*Completed: 2026-02-05*
