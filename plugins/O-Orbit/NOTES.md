# O-Orbit Notes

## Status
- **Current Status:** 💡 Ideated
- **Version:** N/A
- **Type:** Audio Effect (Spatial Orbiter)

## Lifecycle Timeline

- **2026-02-09:** Creative brief created. Universal orbital spatializer with VBAP rendering to arbitrary speaker arrays.

## Known Issues

None (pre-implementation)

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
