# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-02-14
**Participants:** User, Claude

## Plugin State at Entry

- **Stages 1-3 complete:** Foundation (CMake + ANIRA + ONNX Runtime), DSP (overlap-add, Perlin evolve, tilt filter, stereo decorrelation, full processBlock), GUI (Ouaricon Naturalist WebView, 10 parameters bound)
- **Placeholder models:** Using tiny ONNX models (32-dim latent -> 4096-sample blocks). Real trained models not yet available.
- **pluginval:** Passed at strictness 5 during Stage 2 (before GUI changes)
- **Build status:** VST3, AU, Standalone all compile cleanly
- **AU registration:** `aumu OuTx OuDv - Ouaricon Audio Development: O-Texture-dev`

## Requirements Confirmed

1. **Full polish pass** — pluginval, cross-DAW testing, CHANGELOG, installer prep, code cleanup
2. **Skip presets** — Defer preset creation until real trained models produce meaningful audio
3. **No known bugs** — Focus on systematic testing to uncover issues
4. **Version as v0.1.0** — Pre-release milestone with CHANGELOG noting placeholder models
5. **Skip perceptual quality testing** — Not meaningful with placeholder models

## Constraints Identified

- Placeholder models limit perceptual/audio quality testing (4.1 from ROADMAP)
- Presets deferred (4.3 from ROADMAP) — parameter combinations not meaningful without real textures
- Real model integration is a future milestone (not part of this stage)
- Plugin name is `O-Texture-dev` (development suffix)

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Polish scope | Full (minus perceptual + presets) | Maximize value of placeholder milestone |
| Perceptual testing (4.1) | Skip | Placeholder models don't produce real textures |
| Preset creation (4.3) | Skip/Defer | No meaningful parameter-to-audio mapping yet |
| pluginval | Run at strictness 5-10 | Validate after Stage 3 GUI changes |
| Cross-DAW testing | Systematic | Test in available DAWs (Logic, Ableton, Reaper, Standalone) |
| Version tag | v0.1.0 pre-release | Marks functional milestone with placeholder caveat |
| CHANGELOG | Create for v0.1.0 | Document all implemented features across Stages 1-3 |
| Code cleanup | Yes | Review for dead code, TODO comments, unused imports |
| Installer prep | Basic | Code-sign, verify system install works |

## Scope for Stage 4

### In Scope
1. **pluginval validation** — VST3 + AU at strictness 5-10, fix any failures
2. **Cross-DAW testing** — Logic Pro, Ableton Live, Reaper, Standalone
3. **Code cleanup** — Remove dead code, unused imports, stale TODO comments
4. **CHANGELOG.md** — v0.1.0 entry documenting all features
5. **Version tagging** — Update version string to 0.1.0
6. **Build verification** — Clean build from fresh CMake configure
7. **Plugin install verification** — VST3 + AU install and register correctly
8. **Code signing** — Ad-hoc sign for local testing

### Out of Scope (Deferred)
- Perceptual audio quality testing (needs real models)
- Preset creation (needs real models)
- Windows build/testing (CI/CD for later)
- Public release packaging (DMG/installer)
- INT8 quantization
- User documentation / README

## Open Questions

- None. All decisions made during discuss phase.

## Next Phase

Ready for: research phase (investigate pluginval strictness levels, cross-DAW gotchas)
