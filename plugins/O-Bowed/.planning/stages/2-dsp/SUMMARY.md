# Stage 2 (DSP) - Execution Summary

**Plugin:** O-Bowed
**Stage:** 2-dsp (Phases 3.1 + 3.2 + 3.3 + 3.4)
**Completed:** 2026-04-05

## Phase 3.1: Core Waveguide + Basic Friction

**Goal:** Single bowed string producing sound with core hyperbolic friction model

### Files Created
- `Source/DSP/HyperbolicFriction.h` -- Header-only memoryless friction curve (STK-style)
- `Source/DSP/BowModel.h` + `.cpp` -- Bow envelope (velocity-dependent attack 5-50ms, 30ms release)
- `Source/DSP/WaveguideString.h` + `.cpp` -- Bidirectional Thiran delay lines + bridge loss filter
- `Source/BowedStringVoice.h` + `.cpp` -- SynthesiserVoice owning waveguide + bow + friction

### Files Modified
- `Source/PluginProcessor.h` -- Added BowedStringVoice include
- `Source/PluginProcessor.cpp` -- 8 voices, prepareToPlay, renderNextBlock wiring
- `CMakeLists.txt` -- Added 7 source files

### Parameters Connected (7)
bowSpeed, bowPressure, bowPosition, rosin, brightness, infiniteSustain, outputLevel

### Verification: PASSED (10/10)

---

## Phase 3.2: Body Resonator + Stereo Width

**Goal:** Morphable body resonator gives instrument identity, stereo output with width control

### Files Created
- `Source/DSP/BodyResonator.h` + `.cpp` -- 8-section parallel peaking EQ with 4 morphable presets (Membrane/Wood/Metal/Glass), log-domain frequency interpolation, body size scaling (3-octave range), per-preset normalization
- `Source/DSP/StereoWidthProcessor.h` -- Header-only allpass decorrelator + M/S width processing with SmoothedValue

### Files Modified
- `Source/PluginProcessor.h` -- Added BodyResonator + StereoWidthProcessor includes and members
- `Source/PluginProcessor.cpp` -- prepare calls + processBlock body resonator + stereo width
- `CMakeLists.txt` -- Added 3 source files

### Parameters Connected (3)
bodyMaterial, bodySize, width

### Build: Compiled with 0 errors, 1 minor sign-conversion warning

---

## Signal Flow

```
MIDI -> Synthesiser -> BowedStringVoice[8] (mono per-voice)
  -> HyperbolicFriction + BowModel + WaveguideString
  -> addSample to stereo buffer
  -> BodyResonator (channel 0, per-sample parallel 8-band peaking EQ)
  -> StereoWidthProcessor (allpass decorrelation + M/S width)
  -> Stereo output
```

---

## Phase 3.3: Multi-String + Sympathetic Coupling

**Goal:** Processor-level drone strings (1-4), passive sympathetic KS waveguides (0-12), body resonator stereo refactor, per-string panning, dynamic voice cap

### Files Created
- `Source/DSP/DroneStringEngine.h` + `.cpp` -- 1-4 always-bowed WaveguideString instances with per-string equal-power panning, cents-offset tuning from reference pitch, ±5% deterministic bow parameter variation
- `Source/DSP/SympatheticStringEngine.h` + `.cpp` -- 0-12 passive Karplus-Strong waveguide strings with Thiran delay, one-pole loss filter, harmonic series tuning, energy-gated CPU optimization, stereo panning

### Files Modified
- `Source/DSP/BodyResonator.h` + `.cpp` -- Refactored from mono to stereo: dual filter banks (bodyModesL/bodyModesR) with shared coefficients and separate state per channel. Replaced `process(float)` with `processStereo(float&, float&)`
- `Source/DSP/StereoWidthProcessor.h` -- Removed allpass decorrelator, now pure M/S width on true stereo input
- `Source/BowedStringVoice.h` + `.cpp` -- Added per-voice panL/panR, setPan(), getCurrentFrequency(). Render uses pan values instead of equal-channel write
- `Source/PluginProcessor.h` + `.cpp` -- Added DroneStringEngine + SympatheticStringEngine members. Complete processBlock rewrite: dynamic voice cap, drone parameter updates, per-sample drone/body-stereo/sympathetic loop, new signal flow
- `CMakeLists.txt` -- Added 4 new source files

### Parameters Connected (7 new)
stringCount, stringTuning1, stringTuning2, stringTuning3, stringTuning4, sympatheticAmount, sympatheticCount

### Build: Compiled with 0 errors (sign-conversion warnings only)
### Pluginval: PASSED at strictness level 5

---

## Signal Flow (Phase 3.3)

```
MIDI -> Synthesiser -> BowedStringVoice[4-8] (panned stereo per-voice)
  -> Dynamic voice cap (8/6/5/4 based on STRING_COUNT)

DroneStringEngine (1-4 always-bowed WaveguideString instances)
  -> Per-drone equal-power panning -> add to stereo buffer

Pre-body bridge sum (mono downmix for sympathetic excitation)

BodyResonator (stereo, shared coefficients, separate L/R filter state)

Post-body mono for sympathetic excitation

SympatheticStringEngine (0-12 passive KS waveguides)
  -> Excitation: 50/50 pre/post body mix
  -> Energy-gated per-string processing
  -> Per-string stereo panning -> add to stereo buffer

StereoWidthProcessor (M/S encode/decode, width scaling)
  -> Stereo output
```

## Total Parameters Connected: 17/22
bowSpeed, bowPressure, bowPosition, rosin, brightness, infiniteSustain, outputLevel, bodyMaterial, bodySize, width, stringCount, stringTuning1-4, sympatheticAmount, sympatheticCount

## Parameters NOT YET Connected (5)
reversedFriction, subHarmonics, referencePitch (partially -- used by drones but not voice tuning engine), tuningSystem, bowNoise

---

## Phase 3.4: Advanced Friction + Impossible Physics

**Goal:** Elasto-plastic (enhanced) and thermal (quality) friction tiers with runtime selection, reversed friction blending, sub-harmonics generator, and waveguide junction split for stateful friction models.

### Files Created
- `Source/DSP/ElastoPlasticFriction.h` -- Header-only Serafin/Avanzini 2003 bristle model with sinusoidal alpha transition, forward Euler integration, passivity fix (sigma_1_eff = sigma_1 * (1 + v_rel^2)), denormal protection
- `Source/DSP/ThermalFriction.h` -- Header-only quality tier composing ElastoPlasticFriction + temperature ODE (Newton cooling), glass transition at 49°C, 256-entry exp() LUT for thermal scale
- `Source/DSP/SubHarmonicsGenerator.h` -- Header-only asymmetric tanh waveshaper, stateless, const noexcept, wet/dry blend

### Files Modified
- `Source/DSP/WaveguideString.h` + `.cpp` -- Added JunctionState struct, readJunction() (steps 2-4), writeJunction() (steps 6-8). Original processSample() unchanged for DroneStringEngine compatibility
- `Source/BowedStringVoice.h` + `.cpp` -- Added ElastoPlasticFriction, ThermalFriction, SubHarmonicsGenerator members. New per-sample loop: readJunction → tier dispatch → reversed friction → writeJunction → sub-harmonics. Tier switch resets bristle/temperature state. prepareToPlay caches dt and calls qualityFriction.prepare()
- `Source/PluginProcessor.cpp` -- Added frictionTier AudioParameterChoice (Core/Enhanced/Quality, default Core)
- `CMakeLists.txt` -- Added 3 new header files

### Parameters Connected (3 new)
frictionTier (NEW Choice param), reversedFriction (now wired to voice), subHarmonics (now wired to voice)

### Build: Compiled with 0 errors (unused parameter warnings only)
### Pluginval: PASSED at strictness level 5

---

## Signal Flow (Phase 3.4)

```
MIDI -> Synthesiser -> BowedStringVoice[4-8] (panned stereo per-voice)
  -> BowModel envelope -> readJunction (pop delays, bridge filter)
  -> Friction Tier Dispatch:
     Tier 0 (Core): HyperbolicFriction (memoryless, O(1))
     Tier 1 (Enhanced): ElastoPlasticFriction (bristle state z, ~3x CPU)
     Tier 2 (Quality): ThermalFriction (bristle z + temperature T, ~5x CPU)
  -> Reversed Friction Transform (lerp rho ↔ 1-rho)
  -> writeJunction (inject velocity, push delays)
  -> SubHarmonicsGenerator (asymmetric tanh waveshaper)
  -> Gain + hard-clip -> stereo panning

DroneStringEngine (1-4 strings, core tier only)
  -> Per-drone panning -> add to stereo buffer

BodyResonator (stereo) -> SympatheticStringEngine -> StereoWidthProcessor
```

## Total Parameters Connected: 20/23
bowSpeed, bowPressure, bowPosition, rosin, brightness, infiniteSustain, outputLevel, bodyMaterial, bodySize, width, stringCount, stringTuning1-4, sympatheticAmount, sympatheticCount, frictionTier, reversedFriction, subHarmonics

## Parameters NOT YET Connected (3)
referencePitch (voice tuning), tuningSystem, bowNoise → Phase 3.5
