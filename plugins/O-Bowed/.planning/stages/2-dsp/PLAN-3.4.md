# Stage 2 DSP -- Phase 3.4 Plan: Advanced Friction + Impossible Physics

**Plugin:** O-Bowed
**Phase:** 3.4
**Input:** CONTEXT-3.4.md, RESEARCH-3.4.md
**Date:** 2026-04-05

---

## Goal

Implement elasto-plastic (enhanced) and thermal (quality) friction tiers with runtime tier selection, reversed friction curve blending, sub-harmonics generator, and Newton-Raphson solver. Refactor WaveguideString to expose junction state for stateful friction models. Connect 3 new parameters (frictionTier, reversedFriction, subHarmonics) to per-voice DSP.

---

## Tasks

### Task 1: Create ElastoPlasticFriction.h (Enhanced Tier)
- **Create:** `Source/DSP/ElastoPlasticFriction.h`
- **Depends on:** none
- **Details:**
  - Header-only class (~100 lines)
  - Serafin/Avanzini 2003 single-state bristle model
  - State variable `z` (bristle displacement, per-instance)
  - `computeAlpha(z, z_ss, z_ba)` -- sinusoidal transition function:
    - `|z| < z_ss`: alpha = 0 (elastic stick)
    - `z_ss <= |z| < z_ba`: alpha = 0.5 * (1 + sin(pi*(|z| - z_m)/(z_ba - z_ss))) where z_m = 0.5*(z_ba + z_ss)
    - `|z| >= z_ba`: alpha = 1 (breakaway/slip)
  - Forward Euler integration: `z += v_rel * alpha * dt`, clamped to +/-1.5*z_ba
  - Passivity fix: `sigma_1_eff = sigma_1 * (1.0 + v_rel * v_rel)`
  - Friction force: `F = sigma_0 * z + sigma_1_eff * dz + sigma_2 * v_rel`
  - `computeReflectionCoefficient(v_delta, F_bow, dt)` -- NON-const, updates z state
  - `computeFrictionDerivative(v_rel, F_bow, dt)` -- for NR solver dF/dv_rel
  - `resetState()` -- z = 0
  - `setRosin(float)` / `setStringImpedance(float)` matching HyperbolicFriction API
  - Parameters: sigma_0=6000, sigma_1=0.5, sigma_2=0.002, mu_s=0.8, mu_d=0.3
  - Denormal protection: flush z to 0 when |z| < 1e-15f

### Task 2: Create ThermalFriction.h (Quality Tier)
- **Create:** `Source/DSP/ThermalFriction.h`
- **Depends on:** Task 1
- **Details:**
  - Header-only class (~80 lines)
  - Composes `ElastoPlasticFriction` (owns an instance)
  - Additional state: `T_contact` (temperature, Celsius)
  - Temperature ODE: `dT/dt = (Q_gen - Q_loss) / C_thermal`
    - `Q_gen = |F_friction * v_rel|` (frictional heating)
    - `Q_loss = k_cool * (T_contact - T_ambient)` (Newton cooling)
  - Friction coefficient modulation above glass transition:
    - `T <= T_glass`: thermalScale = 1.0 (no effect)
    - `T > T_glass`: thermalScale = exp(-alpha_T * (T - T_glass))
  - `computeReflectionCoefficient(v_delta, F_bow, dt)`:
    1. Get rho_base from internal ElastoPlasticFriction
    2. Approximate F_friction from rho_base for heating calc
    3. Update T_contact with Euler step
    4. Return `rho_base * thermalScale`
  - `computeFrictionDerivative(v_rel, F_bow, dt)` -- delegates to elasto-plastic (thermal modulation is slow, doesn't affect NR convergence)
  - `resetState()` -- resets elasto-plastic z AND T_contact = T_ambient
  - Thermal params: T_glass=49.0, T_ambient=20.0, alpha_T=0.15, C_thermal=0.002, k_cool=0.5, T_max=150.0
  - Optimization: precompute exp() LUT (256 entries, T_glass to T_max) in prepare(), linear interpolation at runtime

### Task 3: Create SubHarmonicsGenerator.h
- **Create:** `Source/DSP/SubHarmonicsGenerator.h`
- **Depends on:** none
- **Details:**
  - Header-only class (~30 lines)
  - Asymmetric tanh waveshaper
  - `process(float input, float amount)`:
    - Early return if amount < 0.001f
    - depth = amount * 3.0f
    - Positive peaks: `tanh(input * (1.0 + depth))` (harder clip)
    - Negative peaks: `tanh(input * (1.0 + depth * 0.3f))` (softer clip)
    - Wet/dry blend: `input + amount * (shaped - input)`
  - Post-waveguide, pre-body resonator placement (safe, avoids feedback instability)
  - No state -- const noexcept

### Task 4: Refactor WaveguideString -- Add Junction Split Interface
- **Modify:** `Source/DSP/WaveguideString.h`, `Source/DSP/WaveguideString.cpp`
- **Depends on:** none
- **Details:**
  - Add `JunctionState` struct: `{ float bridgeReflection, nutReflection, v_string_incoming }`
  - Add `readJunction(float v_bow)` -- steps 2-4: pop from delays, compute v_string_incoming, return JunctionState
  - Add `writeJunction(float rho, float v_delta, const JunctionState& state)` -- steps 6-8: compute newVelocity, push to delays, return output sample with energy tracking
  - **Keep existing** `processSample(v_bow, F_bow, const HyperbolicFriction&)` unchanged for DroneStringEngine compatibility
  - readJunction handles: filterDirty check, popSample from both delays, bridge loss filter
  - writeJunction handles: v_delta*rho injection, pushSample to both delays, energy tracking, denormal flush

### Task 5: Update BowedStringVoice -- Friction Tier Dispatch + Effects
- **Modify:** `Source/BowedStringVoice.h`, `Source/BowedStringVoice.cpp`
- **Depends on:** Tasks 1, 2, 3, 4
- **Details:**
  - **Header changes:**
    - Add includes: ElastoPlasticFriction.h, ThermalFriction.h, SubHarmonicsGenerator.h
    - Add members: `ElastoPlasticFriction enhancedFriction`, `ThermalFriction qualityFriction`, `SubHarmonicsGenerator subHarmonicsGen`
    - Add: `int currentTier = 0`, `int previousTier = 0`
    - Add: `float reversedAmount = 0.0f`, `float subHarmonicsAmount = 0.0f`
    - Add: `float dt = 1.0f / 44100.0f` (cached from prepareToPlay, for friction ODE)
  - **prepareToPlay changes:**
    - Cache `dt = 1.0 / sampleRate`
    - Call `qualityFriction.prepare(sampleRate)` for exp LUT initialization
  - **updateParametersFromAPVTS changes:**
    - Read frictionTier: `static_cast<int>(parameters->getRawParameterValue("frictionTier")->load())`
    - Read reversedFriction, subHarmonics from APVTS
    - Detect tier switch: if `currentTier != previousTier`, call resetState() on departing and arriving friction models
    - Update previousTier
    - Call `enhancedFriction.setRosin()` / `qualityFriction.setRosin()` (mirrors core)
    - Call `enhancedFriction.setStringImpedance()` / `qualityFriction.setStringImpedance()`
  - **renderNextBlock per-sample loop rewrite:**
    1. `bowModel.updateEnvelope()` -> get v_bow, F_bow
    2. `auto jState = waveguideString.readJunction(v_bow)`
    3. `float v_delta = v_bow - jState.v_string_incoming`
    4. Tier dispatch switch (0/1/2) -> compute rho
    5. Reversed friction transform: `rho = rho + reversedAmount * (1.0f - 2.0f * rho)` (skip if reversedAmount < 0.001f)
    6. `float sample = waveguideString.writeJunction(rho, v_delta, jState)`
    7. Sub-harmonics: `sample = subHarmonicsGen.process(sample, subHarmonicsAmount)` (skip if < 0.001f)
    8. Apply gain, hard-clip, write stereo with pan

### Task 6: Add frictionTier Parameter + Wire New Params in Processor
- **Modify:** `Source/PluginProcessor.cpp`
- **Depends on:** none (parameter layout is independent of voice code)
- **Details:**
  - Add frictionTier choice parameter to createParameterLayout():
    ```cpp
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "frictionTier", 1 },
        "Friction Tier",
        juce::StringArray { "Core", "Enhanced", "Quality" },
        0  // Default: Core
    ));
    ```
  - Place after SUB_HARMONICS in the "Impossible Physics" section (now 4 params in that group)
  - No processBlock changes needed -- frictionTier/reversedFriction/subHarmonics are read by BowedStringVoice directly from APVTS (per-voice, not processor-level)

### Task 7: Update CMakeLists.txt
- **Modify:** `CMakeLists.txt`
- **Depends on:** Tasks 1, 2, 3 (file names)
- **Details:**
  - Add to target_sources:
    - `Source/DSP/ElastoPlasticFriction.h`
    - `Source/DSP/ThermalFriction.h`
    - `Source/DSP/SubHarmonicsGenerator.h`

---

## Execution Waves

**Wave 1** (independent -- can execute in parallel):
- Task 1: ElastoPlasticFriction.h
- Task 3: SubHarmonicsGenerator.h
- Task 4: WaveguideString junction split
- Task 6: frictionTier parameter in PluginProcessor

**Wave 2** (depends on Task 1):
- Task 2: ThermalFriction.h

**Wave 3** (depends on all above):
- Task 5: BowedStringVoice rewrite
- Task 7: CMakeLists.txt update

---

## Files Summary

### Create (3)
| File | Lines | Description |
|------|-------|-------------|
| `Source/DSP/ElastoPlasticFriction.h` | ~100 | Enhanced tier: bristle ODE, sinusoidal alpha, NR derivative |
| `Source/DSP/ThermalFriction.h` | ~90 | Quality tier: composes elasto-plastic + thermal modulation |
| `Source/DSP/SubHarmonicsGenerator.h` | ~30 | Asymmetric tanh waveshaper, post-waveguide |

### Modify (5)
| File | Changes |
|------|---------|
| `Source/DSP/WaveguideString.h` | Add JunctionState struct, readJunction(), writeJunction() |
| `Source/DSP/WaveguideString.cpp` | Implement readJunction/writeJunction (refactor from processSample internals) |
| `Source/BowedStringVoice.h` | Add friction model members, tier state, dt cache |
| `Source/BowedStringVoice.cpp` | New per-sample loop with tier dispatch, reversed friction, sub-harmonics |
| `Source/PluginProcessor.cpp` | Add frictionTier choice parameter to layout |
| `CMakeLists.txt` | Add 3 new header files |

---

## Parameters After Phase 3.4

**Newly connected (3):**
- frictionTier (Choice: Core/Enhanced/Quality) -- NEW APVTS parameter
- reversedFriction (0-1, already in APVTS) -- now wired to voice
- subHarmonics (0-1, already in APVTS) -- now wired to voice

**Total connected: 20/23**

**Remaining for Phase 3.5 (3):**
- referencePitch, tuningSystem, bowNoise

---

## Success Criteria

- [ ] Core tier: identical behavior to Phase 3.3 baseline (regression check)
- [ ] Enhanced tier: attack transients have more "bite" vs core (audible stick-slip hysteresis)
- [ ] Quality tier: sustained notes evolve subtly vs enhanced (thermal softening over ~7ms)
- [ ] Tier switching: no clicks or glitches (hard reset converges in <5 vibration cycles)
- [ ] NR solver: converges reliably (6-iteration cap, bailout to previous rho on divergence)
- [ ] INFINITE_SUSTAIN at 100%: tone sustains indefinitely (unchanged from 3.3)
- [ ] REVERSED_FRICTION: creates unusual timbres without instability (rho bounded [0,1])
- [ ] SUB_HARMONICS: adds audible sub-octave content (asymmetric clipping)
- [ ] No runaway or instability with extreme "impossible physics" settings
- [ ] DroneStringEngine: unaffected (still uses core tier via existing processSample)
- [ ] Build: compiles with 0 errors
- [ ] pluginval: passes at strictness level 5

---

## Risk Mitigations

| Risk | Mitigation |
|------|------------|
| NR divergence | 6-iteration cap, NaN/Inf check, bailout to previous rho, hard-clip v output to +/-10 |
| Bristle state runaway | Clamp z to +/-1.5*z_ba, denormal flush at |z| < 1e-15 |
| Temperature runaway | Clamp T_contact to [T_ambient, T_max=150], reset on tier switch |
| Reversed friction instability | rho stays bounded [0,1] by construction (lerp between rho and 1-rho) |
| Core tier regression | Existing processSample unchanged; DroneStringEngine path unmodified |
| Tier switch click | Hard reset converges in <5 cycles (~11ms at 440Hz), below perception threshold |
