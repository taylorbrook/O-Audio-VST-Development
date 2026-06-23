# Stage 4 (Validation / Polish) — Verification

**Date:** 2026-06-22
**Verdict:** ✓ PASS — release-ready v1.0.0

## Goal-backward check

Stage 4 goal = "a validated, release-ready v1.0.0." Achieved iff every clause below holds:

| Clause | Evidence | Result |
|--------|----------|--------|
| Both formats pass strict validation | pluginval strictness 8 → VST3 `SUCCESS`, AU `SUCCESS`; `auval` SUCCEEDED (Stage 3, unchanged binary path) | ✓ |
| Full parameter space is stable | pluginval **Fuzz parameters** + **Automation** (incl. background thread) passed, zero fail/error | ✓ |
| 6 factory presets are finite / in-range | `applyFactoryPreset` resets-to-default then writes jlimited/normalized values; covered at runtime by the fuzz pass | ✓ |
| Anti-aliasing is exact across the keyboard | `computeKmax` floor-band-limit + `nyquistGain` taper + headroom divide + `isfinite` phase wrap (code) + clean high-note render/fuzz | ✓ |
| Default patch unregressed | No Stage 4 source change to the render path; default = pure sine (H1=100%, rest 0) | ✓ |
| Registry + changelog reflect shipped v1.0.0 | CMake `VERSION "1.0.0"`, `CHANGELOG.md` [1.0.0], PLUGINS.md ✅ Working / 1.0.0 | ✓ |

## Gates

| Gate | Result |
|------|--------|
| VST3 build | ✓ clean |
| AU build | ✓ clean |
| pluginval VST3 (strictness 8) | ✓ SUCCESS |
| pluginval AU (strictness 8) | ✓ SUCCESS |
| `auval -v aumu OSiA OuDv` | ✓ SUCCEEDED (Stage 3) |
| Version / changelog / registry | ✓ updated to 1.0.0 |

## Residual (manual, in-DAW — same as Stage 3)

- Interactive drag / live drawbar-glow / scope-morph / audible lesson presets / on-screen keyboard
  are best confirmed by playing in a DAW or Standalone. auval + pluginval cover headless render,
  MIDI, automation, fuzz, and state; the viz + keyboard follow the shipped O-simpleFM wiring.

## Deferred to v1.1

- Persistent user preset save/load bar (`OuariconPresetManager`).
- Per-partial mod-env decay routing.
- Windows VST3 build (cross-platform flags wired; Windows CI gate).
