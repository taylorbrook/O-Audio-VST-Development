# Stage 4 (Validation / Polish) — Plan

**Strategy:** Single validation pass; no source changes to the render path. Mechanical release prep.

## Tasks & success criteria

| # | Task | Success criteria |
|---|------|------------------|
| 1 | Rebuild VST3 + AU | `ninja O-simpleAdditive_VST3 O-simpleAdditive_AU` clean (exit 0) |
| 2 | pluginval VST3 | strictness 8 → `ALL TESTS PASSED` / `SUCCESS` |
| 3 | pluginval AU | strictness 8 → `ALL TESTS PASSED` / `SUCCESS` |
| 4 | Factory-preset sweep | 6 lessons (`applyFactoryPreset`) reset-to-default then write finite, jlimited/normalized values; no out-of-range writes |
| 5 | Aliasing audit | Code-verify `computeKmax` + `nyquistGain` taper + headroom divide + `isfinite` phase wrap; pluginval render/fuzz at high notes finds no NaN/Inf/denormal |
| 6 | Version → 1.0.0 | CMake `VERSION "1.0.0"` |
| 7 | CHANGELOG.md | v1.0.0 entry covering Stages 1–4 |
| 8 | Registry | PLUGINS.md row → ✅ Working / 1.0.0 / 2026-06-22; STATUS.md → Stage 4 complete |
| 9 | Verify | SUMMARY.md + VERIFICATION.md (goal-backward) written |

## Verification gate (goal-backward)

Stage 4 goal = "a validated, release-ready v1.0.0." Achieved iff: both formats pass pluginval at
strictness 8, the 6 presets are provably finite/in-range, the band-limit math is confirmed exact,
and the registry + changelog reflect a shipped v1.0.0 — with the default patch unregressed.
