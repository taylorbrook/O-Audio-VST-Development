# Phase 3.3 Summary: Tone Holes + Expression + Legato

**Completed:** 2026-04-05
**Stage:** 2 (DSP)
**Phase:** 3.3
**Status:** COMPLETE — VST3 + AU build zero errors, auval PASS

---

## What Was Implemented

### 1. BoreWaveguide Restructured into 5 Segments
- Replaced single forward+backward delay pair with 5 segment pairs (10 Thiran delay lines, 2048 max each)
- Segment fractions: [10%, 20%, 20%, 25%, 25%] of total bore delay
- Per-segment conical scale factors using position-interpolated centers {0.05, 0.20, 0.40, 0.625, 0.875}
- At cylindrical (halfAngle=0): all scales = 1.0 (transparent)

### 2. Keefe Three-Port Tone Hole Scattering (4 Holes)
- 4 tone hole scattering junctions between segments 1-2, 2-3, 3-4, 4-bell
- TONE_HOLE_CUTOFF (200-8000 Hz) maps to progressive hole openings from bell toward reed
- Scatter coefficient from Keefe: `scatter = -holeStrength / (1 + holeStrength)` with holeRadiusRatio=0.6
- Tone hole radiation mixed into output (0.4 mix factor)
- At cutoff=8000 (default): all scatter=0, Phase 3.2 behavior preserved

### 3. Register Hole
- Single scattering junction between segment 0 and segment 1 (~10% from reed)
- REGISTER_HOLE (0-1) controls opening with smaller radius ratio (0.3)
- Cylindrical bore: overblows at 12th (3rd harmonic)
- Conical bore: overblows at octave (2nd harmonic)
- At registerHole=0 (default): scatter=0, transparent

### 4. Expression Modifiers
- **Vibrato**: Sine LFO, 3 sources (Lip→embouchure, Breath→pressure, Throat→bore scale)
- **Growl**: 120 Hz sine oscillator modulating mouth pressure (0-30% depth)
- **Flutter Tongue**: 25 Hz smoothed square (tanh-clipped sine) modulating mouth pressure (0-40% depth)
- **Subtone**: Parameter modifier — reduces pressure 30%, increases noise 30%, increases embouchure 30%
- All bypass at 0 (default): no modulation applied

### 5. Bore-Preserving Mono Legato
- POLY_MODE: Mono (0) / Poly (1)
- Mono + overlapping notes: only retune bore frequency, preserve all DSP state
- Detection via `bore.getEnergy() > 0.001f` in noteStarted()
- Mono + gap: normal full-reset onset
- Poly (default): unchanged Phase 3.2 behavior

### 6. Throat Vibrato (Bore Scale Modulation)
- `modulateScaleFactor()` method on BoreWaveguide
- Vibrato source=Throat applies ±3% modulation to conical scale factors
- Reset per sample for single-sample-accurate modulation

## Files Modified

| File | Action | Changes |
|------|--------|---------|
| Source/DSP/BoreWaveguide.h | Major rewrite | 5-segment delay, tone holes, register hole, modulateScaleFactor |
| Source/DSP/ReedModel.h | Minor add | setEmbouchure() for per-sample embouchure updates |
| Source/ReedWindVoice.h | Modify | LFO phase state (vibrato/growl/flutter), sr cache |
| Source/ReedWindVoice.cpp | Major modify | Expression mods, legato, tone hole param wiring, LFO resets |

## Parameters Connected This Phase

| Parameter | Range | Default | Effect |
|-----------|-------|---------|--------|
| toneHoleCutoff | 200-8000 Hz | 8000 | Progressive spectral darkening |
| registerHole | 0-1 | 0 | Overblowing (octave/12th) |
| vibratoDepth | 0-1 | 0 | LFO amplitude |
| vibratoRate | 1-10 Hz | 5 | LFO frequency |
| vibratoSource | Lip/Breath/Throat | Lip | Modulation target |
| growlAmount | 0-1 | 0 | 120 Hz pressure mod |
| flutterTongue | 0-1 | 0 | 25 Hz pressure mod |
| subtone | 0-1 | 0 | Airy soft tone |
| polyMode | Mono/Poly | Poly | Voice management |

## Regression Safety

All new features bypass at default values:
- toneHoleCutoff=8000, registerHole=0: all scatter=0, junctions transparent
- vibratoDepth=0, growlAmount=0, flutterTongue=0, subtone=0: no modulation
- polyMode=Poly: standard independent voice behavior

## Build Verification

- VST3: PASS (zero errors)
- AU: PASS (zero errors)
- auval: PASS (aumu ORed OuDv)
- Installed to ~/Library/Audio/Plug-Ins/

## Cumulative Parameters Active: 24

Phases 3.1 (11) + 3.2 (3) + 3.3 (9 new, polyMode shared) = 24 total active parameters
