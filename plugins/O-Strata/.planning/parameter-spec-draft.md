# O-Strata - Parameter Specification (Draft)

> Draft extracted from BRIEF.md (Parameters section) and O-Prism v1.24.0's parameter registry (`plugins/O-Prism/.planning/params.tsv`, 173 parameters). Full specification will be generated during UI mockup finalization. IDs below follow O-Prism's `osc{A,B}` prefix convention (`PrismParamIds::oscIds`); the exact suffix strings are a Stage 1 decision.

## Inherited from O-Prism v1.24.0 (unchanged)

All O-Prism parameters carry over with identical IDs, ranges, defaults and flags **except** `oscATable` and `oscBTable`, which are removed (replaced by the Geometry block). Inherited count: **171**.

| Section | IDs (from params.tsv) | Count |
|---------|-----------------------|-------|
| Osc A / Osc B (minus Table) | `osc{A,B}{Pos,Level,Pan,Coarse,Fine,Phase,Unison,Detune,Width,WarpType,WarpAmt}` | 22 |
| Sub / Noise | `subShape subOctave subLevel subRouting noiseType noiseLevel` | 6 |
| Envelopes | `amp{Attack,Decay,Sustain,Release}`, `filt{Attack,Decay,Sustain,Release}`, `filt{A,B}EnvDepth` | 10 |
| Filters | `filt{A,B}{Type,Cutoff,Res,Drive,KeyTrack}`, `filtRouting` | 11 |
| Tuning / Global pitch | `tuningPreset tonic masterTune octaveStretch pitchBendRange glideMode glideTime` | 7 |
| Effects | reverb (7), delay (7), chorus (4), distortion (4), EQ (5) | 27 |
| LFOs | `lfo{1..4}{Rate,Shape,Sync,Division,FreeRun}` | 20 |
| Mod matrix | `modSlot{0..15}{Src,Dst,Amt,On}` | 64 |
| Master | `masterVol stereoWidth oscMix velocityCurve` | 4 |

`osc{A,B}Pos` (Position) keeps its ID and range; its meaning becomes the frame index through the geometry sweep.

## New: Geometry block (per oscillator, A and B)

Every parameter below exists twice (`oscA…`, `oscB…`). All are **bake** parameters: automatable APVTS parameters that trigger a debounced background re-bake on change (assumption carried from BRIEF.md; confirmed or overturned in Stage 0).

### Geometry — common

| Parameter | ID (draft) | Type | Range | Default | Unit | Description |
|-----------|------------|------|-------|---------|------|-------------|
| Source Type | osc?GeoSource | Choice | Mesh / Volume / Terrain | Mesh | - | Which generator family feeds this oscillator |
| Frame Count | osc?GeoFrames | Choice | 64 / 128 / 256 | 256 | frames | Frames in the baked table |
| Shape Drive | osc?GeoDrive | Float | 0.0 - 1.0 | 0.0 | - | Post-unwrap tanh drive before normalisation |

### Geometry — Mesh (slice sweep)

| Parameter | ID (draft) | Type | Range | Default | Unit | Description |
|-----------|------------|------|-------|---------|------|-------------|
| Mesh | osc?Mesh | Choice | Sphere / Torus / Twisted Star / Crescent / Torus Knot / fBm Blob / … / Imported | Torus | - | Built-in library entry or the imported OBJ/STL |
| Slice Tilt X | osc?MeshTiltX | Float | -90.0 - 90.0 | 0.0 | ° | Slicing-plane rotation about X (second morph axis) |
| Slice Tilt Y | osc?MeshTiltY | Float | -90.0 - 90.0 | 0.0 | ° | Slicing-plane rotation about Y |
| Projection Angle | osc?MeshPhi | Float | 0.0 - 360.0 | 0.0 | ° | Emits cos φ·x(s) + sin φ·y(s) of the contour |
| Unwrap Mode | osc?MeshUnwrap | Choice | XY Projection / Centroid Distance | XY Projection | - | Centroid Distance sits behind a flatness floor |
| Loop Policy | osc?MeshLoop | Choice | Largest / Sum | Largest | - | Multi-loop slice handling (never concatenate) |

### Geometry — Volume (orbit through a field)

| Parameter | ID (draft) | Type | Range | Default | Unit | Description |
|-----------|------------|------|-------|---------|------|-------------|
| Field | osc?VolField | Choice | Torus SDF / Box SDF / Gyroid / Sphere SDF / fBm Noise | Torus SDF | - | Scalar field f(x,y,z) sampled by the orbit |
| Orbit | osc?VolOrbit | Choice | Torus Knot (2,3) / (3,5) / (5,7) / Lissajous Knot | Torus Knot (2,3) | - | Closed 3D curve traced once per cycle |
| Sweep Axis | osc?VolSweepAxis | Choice | Orbit Scale / Knot Phase / Z Offset | Orbit Scale | - | Orbit property advanced frame-to-frame |
| Sweep Range | osc?VolSweepRange | Float | 0.0 - 1.0 | 0.6 | - | Travel of the sweep axis across the frames |
| Field Detail | osc?VolDetail | Float | 0.0 - 1.0 | 0.3 | - | Spatial frequency (noise octaves / SDF repeat) |

### Geometry — Terrain (orbit sweep over a heightmap)

| Parameter | ID (draft) | Type | Range | Default | Unit | Description |
|-----------|------------|------|-------|---------|------|-------------|
| Terrain | osc?Terrain | Choice | Sine Product / Radial Rings / … / Imported | Sine Product | - | Built-in analytic terrain or the imported PNG |
| Orbit Shape | osc?TerOrbit | Choice | Ellipse / Epitrochoid 3·5·7 / Hypocycloid 3·5·7 / Superellipse | Ellipse | - | Closed 2D curve scanned once per cycle |
| Orbit Centre X | osc?TerCX | Float | -1.0 - 1.0 | 0.0 | - | Centre offset (asymmetry / even harmonics); driven by view drag |
| Orbit Centre Y | osc?TerCY | Float | -1.0 - 1.0 | 0.0 | - | Centre offset; driven by view drag |
| Orbit Aspect | osc?TerAspect | Float | 0.1 - 1.0 | 1.0 | - | Ellipse minor/major ratio |
| Orbit Rotation | osc?TerRot | Float | 0.0 - 360.0 | 0.0 | ° | X/Y phase relationship (PWM / phase-distortion-like) |
| Sweep Axis | osc?TerSweepAxis | Choice | Radius / Rotation / Centre X / Centre Y | Radius | - | Orbit property advanced frame-to-frame |
| Sweep Range | osc?TerSweepRange | Float | 0.0 - 1.0 | 0.8 | - | Travel of the sweep axis; orbit clamped inside [-1,1]² |
| Image Blur | osc?TerBlur | Float | 0.0 - 1.0 | 0.2 | - | Gaussian pre-blur on PNG terrains |
| Edge Mode | osc?TerEdge | Choice | Mirror / Window | Mirror | - | Mitsuhashi edge handling for PNG terrains |

## Non-parameter state (ValueTree, not APVTS parameters)

- Imported OBJ/STL/PNG source per oscillator: gzip + base64 blob under a size cap, path + SHA-256 above it (cap is a Stage 0 decision).
- Baked tables are never stored; regenerated from parameters on load.

## Parameter Count Summary

- **Inherited from O-Prism:** 171 (173 minus `oscATable`, `oscBTable`)
- **New Geometry block:** 23 per oscillator × 2 = 46
  - Float: 13 per osc (26)
  - Choice: 10 per osc (20)
- **Total:** 217 APVTS parameters

## Notes for Stage 0

- Bake parameters are automatable but automating them triggers re-bakes, not per-sample modulation; they must not appear as mod-matrix destinations (the `modSlotNDst` choice list stays O-Prism's 26 entries).
- `Mesh` / `Terrain` choice lists carry a trailing "Imported" entry so a preset can point at the non-parameter import blob; library indices must be stable across versions (append-only).
- Stage 1 should confirm the ID suffixes and whether the three per-family groups are always present in the APVTS (fixed layout, 217 params) or gated by Source Type (they cannot be — APVTS layout is static), i.e. all 46 exist and only the active family is baked.
