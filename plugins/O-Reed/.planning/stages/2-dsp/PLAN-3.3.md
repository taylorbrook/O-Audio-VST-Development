# Phase 3.3 Execution Plan: Tone Holes + Expression + Legato

**Created:** 2026-04-05
**Stage:** 2 (DSP)
**Phase:** 3.3
**Goal:** Add Keefe three-port scattering tone holes (4) and register hole to the bore waveguide, vibrato/growl/flutter/subtone expression modifiers, and bore-preserving mono legato. All features bypass at default values — Phase 3.2 regression guaranteed.

---

## Tasks

### 1. [ ] Restructure BoreWaveguide: 5 segments with tone hole + register hole scattering

**Files:** `Source/DSP/BoreWaveguide.h`

**Changes:**
This is the largest task — converts the single forward+backward delay pair into 5 segment pairs with 5 inter-segment scattering junctions.

**Replace member variables:**
- Remove single `forwardDelay`, `backwardDelay` (2 delay lines)
- Add `segForwardDelay[5]`, `segBackwardDelay[5]` (10 delay lines, 2048 max each instead of 40000)
- Replace single `currentScaleForward/targetScaleForward` with arrays `[5]`
- Same for `currentScaleBackward/targetScaleBackward`
- Add `float toneHoleScatter[4] = {0,0,0,0}` — scatter coefficients for 4 tone holes
- Add `float registerScatter = 0.0f` — scatter coefficient for register hole
- Add `float prevBellReflection = 0.0f` — one-sample memory for bell reflection at hole 4/bell boundary
- Add `float totalToneHoleRadiation = 0.0f` — accumulated per-sample
- Add `static constexpr float toneHoleRadiationMix = 0.4f`

**Rewrite `prepare()`:**
- Create ProcessSpec and prepare all 10 delay lines
- Same bell/visc filter setup as Phase 3.2

**Rewrite `setFrequency()`:**
- Same total delay and group delay compensation as Phase 3.2
- Split compensated delay into 5 segments: `fractions[5] = {0.10, 0.20, 0.20, 0.25, 0.25}`
- Each segment gets `halfDelay = compensatedDelay * fractions[i] * 0.5f`, clamped to min 2.0f
- Set forward and backward delay per segment

**Rewrite `updateParams()`:**
- Per-segment conical scaling using segment center positions `{0.05, 0.20, 0.40, 0.625, 0.875}`
- For each segment `i`: `r_at_seg = r_in + segCenters[i] * L * tan(halfAngle)`, `targetScaleForward[i] = r_in / r_at_seg`, `targetScaleBackward[i] = r_at_seg / r_in`
- At `halfAngle=0` (cylindrical): all scales = 1.0
- Bell and viscothermal filter coefficients unchanged from Phase 3.2

**Add `updateToneHoles(float toneHoleCutoff, float registerHole)`:**
- Map TONE_HOLE_CUTOFF (200-8000 Hz) to progressive hole openings:
  ```
  cutoff_norm = (toneHoleCutoff - 200.0f) / 7800.0f  // 0=dark, 1=bright
  hole4_opening = 1 - clamp((cutoff_norm - 0.00) / 0.25, 0, 1)  // nearest bell, opens first
  hole3_opening = 1 - clamp((cutoff_norm - 0.25) / 0.25, 0, 1)
  hole2_opening = 1 - clamp((cutoff_norm - 0.50) / 0.25, 0, 1)
  hole1_opening = 1 - clamp((cutoff_norm - 0.75) / 0.25, 0, 1)
  ```
- Convert each opening to scatter coefficient:
  ```
  holeRadiusRatio = 0.6f, tEffNormalized = 1.0f
  holeStrength = opening * holeRadiusRatio^2 / (2 * tEffNormalized)
  scatter = -holeStrength / (1 + holeStrength)
  ```
- Register hole: same formula but `registerHoleRadiusRatio = 0.3f`
  ```
  regStrength = registerHole * 0.3^2 / (2 * 1.0)
  registerScatter = -regStrength / (1 + regStrength)
  ```
- At defaults (cutoff=8000, registerHole=0): all scatter=0 (transparent, Phase 3.2 behavior)

**Rewrite `processSample()`:**
Per-sample signal flow (from RESEARCH-3.3 Section 2.6):
1. Smooth all 5 conical scale factor pairs
2. Pop all 10 delays, apply per-segment conical scaling
3. Compute 5 scattering junctions using popped values:
   - Register hole: `seg_fwd[0]` vs `seg_bwd[1]`
   - Tone holes 1-3: `seg_fwd[i]` vs `seg_bwd[i+1]` for i=1,2,3
   - Tone hole 4: `seg_fwd[4]` vs `prevBellReflection`
   Each junction: `p_scattered = scatter * (fwd + bwd)`, `fwd_out = fwd + p_scattered`, `bwd_out = bwd + p_scattered`, `radiated = -p_scattered`
4. Bell processing: bellFilter + negate on `th4_fwd`, viscFilter on reflected, store in `prevBellReflection`
5. Accumulate tone hole radiation: `totalToneHoleRadiation = sum of all 5 junction radiations`
6. Push into all 10 delays with correct routing (see research Section 2.6 for exact mapping)
7. Energy tracking on `lastRadiatedOutput`
8. Return `seg_bwd[0]` (wave arriving at reed from bore)

**Rewrite `getRadiatedOutput()`:**
- Return `lastRadiatedOutput + totalToneHoleRadiation * toneHoleRadiationMix`

**Rewrite `reset()`:**
- Reset all 10 delay lines, filters, all new state variables (prevBellReflection, scatter arrays, scale arrays)

**Rewrite `snapFiltersToZero()`:**
- Same bell/visc filter snap

**Depends on:** None

**Regression guarantee:** At toneHoleCutoff=8000 and registerHole=0, all scatter=0, junctions transparent (fwd/bwd pass through unchanged), total delay preserved, conical scaling approximates Phase 3.2 single-pair behavior. Bell/visc filters unchanged.

**Risk:** Bore segmentation changes internal delay structure. Thiran quality may degrade at very short segment delays (high frequencies, segment 0). Acceptable for typical reed instrument range.

---

### 2. [ ] Add expression modifier state and LFO computation to ReedWindVoice

**Files:** `Source/ReedWindVoice.h`, `Source/ReedWindVoice.cpp`

**Changes to .h:**
- Add per-voice LFO phase state:
  ```cpp
  float vibratoPhase = 0.0f;
  float growlPhase = 0.0f;
  float flutterPhase = 0.0f;
  float sr = 44100.0f;  // cached sample rate for LFO phase increment
  ```

**Changes to .cpp (renderNextBlock):**
After per-block parameter reads, before per-sample loop:
- Read 7 new expression parameters:
  ```cpp
  float vibratoDepth  = pVibratoDepth->load();
  float vibratoRate   = pVibratoRate->load();
  int   vibratoSrc    = (int)pVibratoSource->load();
  float growlAmount   = pGrowlAmount->load();
  float flutterTongue = pFlutterTongue->load();
  float subtone       = pSubtone->load();
  float airNoise      = pAirNoise->load();  // already read, keep reference for subtone
  ```

Inside per-sample loop, BEFORE reed model:

**Vibrato (sine LFO, 3 targets):**
```cpp
float embouchure_eff = embouchure;
if (vibratoDepth > 1e-6f)
{
    vibratoPhase += 6.2831853f * vibratoRate / sr;
    if (vibratoPhase >= 6.2831853f) vibratoPhase -= 6.2831853f;
    float vibMod = vibratoDepth * std::sin(vibratoPhase);
    switch (vibratoSrc) {
        case 0: embouchure_eff += vibMod * 0.15f; break;  // Lip
        case 1: p_mouth *= (1.0f + vibMod * 0.1f); break; // Breath
        case 2: /* throat: later — bore scale modulation */ break;
    }
}
```

**Growl (~120 Hz sine on mouth pressure):**
```cpp
if (growlAmount > 1e-6f)
{
    growlPhase += 6.2831853f * 120.0f / sr;
    if (growlPhase >= 6.2831853f) growlPhase -= 6.2831853f;
    p_mouth *= (1.0f + growlAmount * std::sin(growlPhase) * 0.3f);
}
```

**Flutter tongue (~25 Hz smoothed square on mouth pressure):**
```cpp
if (flutterTongue > 1e-6f)
{
    flutterPhase += 6.2831853f * 25.0f / sr;
    if (flutterPhase >= 6.2831853f) flutterPhase -= 6.2831853f;
    float squarish = std::tanh(4.0f * std::sin(flutterPhase));
    p_mouth *= (1.0f - flutterTongue * 0.4f * std::max(squarish, 0.0f));
}
```

**Subtone (parameter modifier — damping up, pressure down, noise up):**
```cpp
float airNoise_eff = airNoise;
if (subtone > 1e-6f)
{
    // Increase reed damping: pass modifier to reedModel via embouchure_eff
    // embouchure already controls damping indirectly; instead, directly scale
    // damping by modifying the reedDamping param:
    // reedModel internally uses damping — we'll modulate g_eff in updateParams
    // Simpler: reduce mouth pressure + increase noise + increase embouchure (tighter lip = more damping)
    p_mouth *= (1.0f - subtone * 0.3f);
    airNoise_eff = std::min(airNoise + subtone * 0.3f, 1.0f);
    embouchure_eff += subtone * 0.3f;  // tighter embouchure = more reed damping
}
```

Update `reedModel.updateParams()` to use `embouchure_eff` instead of `embouchure`.
Update `breathNoise.processSample()` to use `airNoise_eff` instead of `airNoise`.

**Changes to .cpp (prepare):**
- Cache `sr = static_cast<float>(sampleRate);`

**Changes to .cpp (noteStarted, normal onset path):**
- Reset LFO phases: `vibratoPhase = growlPhase = flutterPhase = 0.0f;`

**Depends on:** None (can be done in parallel with Task 1)

**Regression guarantee:** At vibratoDepth=0, growlAmount=0, flutterTongue=0, subtone=0: no modulation applied, `embouchure_eff = embouchure`, `airNoise_eff = airNoise`, `p_mouth` unchanged. Identical to Phase 3.2.

---

### 3. [ ] Implement bore-preserving mono legato in ReedWindVoice

**Files:** `Source/ReedWindVoice.cpp`

**Changes to `noteStarted()`:**
Replace the current full-reset logic with legato-aware logic:
```cpp
void ReedWindVoice::noteStarted()
{
    int polyMode = (pPolyMode != nullptr) ? (int)pPolyMode->load() : 1;

    if (polyMode == 0 && bore.getEnergy() > 0.001f)
    {
        // LEGATO: bore active in mono mode — retune only, preserve all DSP state
        auto note = getCurrentlyPlayingNote();
        float frequency = static_cast<float>(note.getFrequencyInHertz());

        // Read bore params for updateParams (needed for setFrequency group delay compensation)
        float boreCharacter = (pBoreCharacter != nullptr) ? pBoreCharacter->load() : 0.0f;
        float bellSize      = (pBellSize != nullptr) ? pBellSize->load() : 0.5f;
        float boreDiameter  = (pBoreDiameter != nullptr) ? pBoreDiameter->load() : 0.5f;
        float boreLength    = (pBoreLength != nullptr) ? pBoreLength->load() : 0.5f;

        bore.updateParams(boreCharacter, bellSize, boreDiameter, boreLength);
        bore.setFrequency(frequency);  // 50ms smoothing handles glitch-free transition
        return;
    }

    // NORMAL onset (Poly mode, or Mono with no active bore)
    // ... existing full reset + attack code ...
}
```

Keep the existing full-reset path for normal onset (poly mode or mono with gap).

Also reset LFO phases in the normal onset path (from Task 2).

**Depends on:** None (legato only uses existing bore.getEnergy() API)

**Regression guarantee:** At polyMode=1 (Poly, default): always takes normal onset path. Identical to Phase 3.2.

---

### 4. [ ] Wire tone hole + register hole parameters from voice to bore

**Files:** `Source/ReedWindVoice.cpp`

**Changes to `renderNextBlock()`:**
After per-block parameter reads, add:
```cpp
float toneHoleCutoff = (pToneHoleCutoff != nullptr) ? pToneHoleCutoff->load() : 8000.0f;
float registerHole   = (pRegisterHole != nullptr) ? pRegisterHole->load() : 0.0f;
```

After `bore.updateParams(...)`, add:
```cpp
bore.updateToneHoles(toneHoleCutoff, registerHole);
```

**Changes to `noteStarted()` (normal onset path):**
Add tone hole init after `bore.updateParams`:
```cpp
float toneHoleCutoff = (pToneHoleCutoff != nullptr) ? pToneHoleCutoff->load() : 8000.0f;
float registerHole   = (pRegisterHole != nullptr) ? pRegisterHole->load() : 0.0f;
bore.updateToneHoles(toneHoleCutoff, registerHole);
```

**Depends on:** Task 1 (BoreWaveguide must have `updateToneHoles()`)

---

### 5. [ ] Handle vibrato source=throat (bore scale modulation)

**Files:** `Source/DSP/BoreWaveguide.h`, `Source/ReedWindVoice.cpp`

**Changes to BoreWaveguide.h:**
Add method:
```cpp
void modulateScaleFactor(float modulation)
{
    // Temporarily adjust all forward/backward scales by modulation amount
    // This shifts the effective bore impedance slightly, producing timbral vibrato
    scaleModulation = modulation;  // applied in processSample smoothing step
}
```
Add `float scaleModulation = 0.0f;` member. In `processSample()`, apply modulation to conical scaling:
```cpp
float mod = 1.0f + scaleModulation;
currentScaleForward[i] *= mod;  // tiny +/-3% modulation
```
Reset `scaleModulation = 0.0f;` at end of processSample or at start of next call.

**Changes to ReedWindVoice.cpp:**
In vibrato `case 2` (Throat): `bore.modulateScaleFactor(vibMod * 0.03f);`

**Depends on:** Task 1 (BoreWaveguide restructure), Task 2 (vibrato framework)

---

### 6. [ ] Build, test, verify regression

**Files:** None (build/test step)

**Actions:**
1. `cmake --preset default` (if needed) then `ninja O-Reed_VST3 O-Reed_AU`
2. Fix any compile errors
3. Install and clear AU cache per CLAUDE.md protocol
4. Verify with `auval -v aumu Orwd Ouai`
5. Manual DAW test: confirm Phase 3.2 behavior at defaults (all new params at 0/8000/Poly)
6. Test each new feature:
   - TONE_HOLE_CUTOFF sweep: progressive darkening
   - REGISTER_HOLE: overblowing behavior
   - Vibrato (all 3 sources)
   - Growl: beating to multiphonic
   - Flutter tongue: ~25 Hz pressure modulation
   - Subtone: airy, soft tone
   - Mono legato: smooth pitch transitions

**Depends on:** Tasks 1-5

---

## File Summary

| File | Action | Tasks |
|------|--------|-------|
| `Source/DSP/BoreWaveguide.h` | Major rewrite | 1, 5 |
| `Source/ReedWindVoice.h` | Modify (add LFO state, sr cache) | 2 |
| `Source/ReedWindVoice.cpp` | Major modify (expression, legato, param wiring) | 2, 3, 4 |

**No new files created.** Research recommended inlining tone hole scattering directly in BoreWaveguide rather than creating a separate ToneHole.h class (3 lines of code per junction, zero overhead).

## Dependencies

```
Task 1 (BoreWaveguide) ──┬──> Task 4 (wire params) ──> Task 6 (build/test)
                          └──> Task 5 (throat vibrato) ──┘
Task 2 (expression) ─────┬──> Task 5 (throat vibrato)
                          └──> Task 6 (build/test)
Task 3 (legato) ──────────────> Task 6 (build/test)
```

Tasks 1, 2, 3 are independent and can execute in parallel.
Task 4 depends on Task 1.
Task 5 depends on Tasks 1 and 2.
Task 6 depends on all.

## Success Criteria

- [ ] TONE_HOLE_CUTOFF at 8000 Hz (all closed): identical to Phase 3.2
- [ ] TONE_HOLE_CUTOFF sweep 8000->200: progressive spectral darkening
- [ ] Register hole at 0: normal fundamental
- [ ] Register hole at 1 + cylindrical bore: overblows (12th or higher mode)
- [ ] Register hole at 1 + conical bore: overblows (octave)
- [ ] Vibrato lip/breath/throat: audible modulation at set rate, silent at depth=0
- [ ] Growl low: subtle beating; growl high: multiphonic texture; growl 0: no effect
- [ ] Flutter tongue: ~25 Hz pressure oscillation audible; 0: no effect
- [ ] Subtone: soft, airy tone; 0: no effect
- [ ] Mono legato: smooth pitch transition (no re-attack) on overlapping notes
- [ ] Mono with gap: normal attack on new note
- [ ] Poly mode: multiple notes play simultaneously (default, unchanged from 3.2)
- [ ] VST3 + AU build zero errors
- [ ] auval passes
- [ ] No clicks during any parameter changes
- [ ] CPU acceptable with tone holes active
