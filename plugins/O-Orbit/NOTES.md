# O-Orbit Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.1
- **Type:** Audio Effect (Spatial Orbiter)

## Lifecycle Timeline

- **2026-02-09:** Creative brief created. Universal orbital spatializer with VBAP rendering to arbitrary speaker arrays.
- **2026-02-11:** v1.0.0 implemented and installed.
- **2026-08-19:** Full code review produced `.planning/improvements/v1.1-review-findings.md` (defects + feature backlog).
- **2026-08-19:** v1.0.1 — Part A defect fixes: Depth param wired to the distance model, RT-safe IIR coefficients (ArrayCoefficients), per-sample mix smoothing, multichannel dry/wet semantics, skew-aware double-click knob reset. pluginval strictness 10 + auval clean.

## Known Issues

- Feature backlog (preset-manager migration, hover help, PPQ sync, ping-pong path, speaker-editor upgrades) tracked in `.planning/improvements/v1.1-review-findings.md` Parts B–D, deferred to a v1.1.0 milestone.
- No offline render harness — regression coverage is pluginval/auval + manual DAW testing only.

## Additional Notes

### Description
Orbital spatializer that moves sound through space using orbital paths, pendulum swings, linear sweeps, and random drift. Rendered via VBAP (Vector Base Amplitude Panning) to any speaker array -- from stereo to 7.1.4 Atmos to fully custom non-equidistant layouts.

### Key Differentiators
- VBAP rendering to ANY speaker configuration (custom layout editor)
- Auto-downmix fallback when DAW can't provide enough channels
- Tempo-syncable motion paths
- Optional L/R split for independent stereo source orbits
- 2D default with optional 3D elevation paths

### Based On
Expanded from `research/spatial-plugin-briefs.md` Brief 1 (O-Orbit), with universal speaker array support added via VBAP.
