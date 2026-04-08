# Stage 2: DSP Phase 3.4 - Verification

## Verification Date

2026-04-05

## Goal-Backward Analysis

### Original Goals (from CONTEXT-3.4.md)

1. Infinite sustain: reduce bore loop losses (viscothermal + bell reflection) toward zero
2. Reverse bore: cross-fade per-segment scale factors from normal to inverted (hichiriki-like)
3. Dual bore: second BoreWaveguide per voice, reed sees combined impedance
4. Drone pitch: cents (-2400 to +2400), parameter migrated from semitones to cents
5. Feedback path: cross-couples bore backward waves with stability clamping
6. Bore profile: multi-segment taper ratios (throat/body/bell) on 5 existing segments
7. All 6 features bypass at default values (Phase 3.3 regression guaranteed)
8. 30 cumulative active parameters (24 + 6 new)

### Deliverables (from SUMMARY-3.4.md + Code Inspection)

1. **Infinite Sustain** (BoreWaveguide.h:164): `g = 0.995f + infiniteSustain * 0.005f` (visc gain approaches 1.0). Bell cutoff: `sustainedBellCutoff = bellCutoff + infiniteSustain * (sr * 0.499f - bellCutoff)` (BoreWaveguide.h:150). At 0: g=0.995, bellCutoff unchanged.

2. **Reverse Bore** (BoreWaveguide.h:99-100): `normalCenters[5]` and `reversedCenters[5]` arrays. Center interpolation at line 121: `center = normalCenters[i] + reverseBore * (reversedCenters[i] - normalCenters[i])`. Only audible with halfAngle > 0 (cylindrical = all scales 1.0).

3. **Bore Profile** (BoreWaveguide.h:103-104): `taperSimple[5] = {1,1,1,1,1}`, `taperMulti[5] = {0.3,0.5,1.0,1.2,2.0}`. Per-segment ratio interpolation at line 124. Effective halfAngle clamped to 5 degrees (line 127). Near-zero guard at line 130.

4. **Dual Bore** (ReedWindVoice.h:47-48): `BoreWaveguide bore2` + `prevBore2Minus`. Prepared alongside bore1 (ReedWindVoice.cpp:82-83). Wired in renderNextBlock (lines 331-338): same params, shared tone holes, frequency offset by dronePitchCents/1200. Output mixed (lines 453-457).

5. **Drone Pitch** (PluginProcessor.cpp:256): `ParameterID{"dronePitch", 2}`, range -2400..2400, step 1 cent. Migration v1->v2 (PluginProcessor.cpp:414-423): detects old semitone range (|val| <= 24.5) and multiplies by 100.

6. **Feedback Path** (ReedWindVoice.cpp:425-428): `safeFeedback = feedbackPath * 0.5f` caps at 50%. Backward wave: `prevBoreMinus * (1 - safeFeedback) + prevBore2Minus * safeFeedback`.

7. **Voice Lifecycle** (ReedWindVoice.cpp): bore2 reset in normal onset (148), hard stop (213,218). Legato path updates bore2 when dualBoreActive (129-138). Post-block: bore2.snapFiltersToZero (480). Voice cleanup checks both bore energies (484).

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| Infinite sustain | ✅ Achieved | BoreWaveguide.h:150,164 — visc gain + bell cutoff ramp to lossless |
| Reverse bore | ✅ Achieved | BoreWaveguide.h:99-100,121 — center position interpolation |
| Bore profile (multi-segment) | ✅ Achieved | BoreWaveguide.h:103-104,124,127 — per-segment taper ratios with 5-deg clamp |
| Dual bore | ✅ Achieved | ReedWindVoice.h:47-48, .cpp:331-338,453-457 — full lifecycle + output mix |
| Drone pitch (cents) | ✅ Achieved | PluginProcessor.cpp:256 — range -2400..2400, v1->v2 migration |
| Feedback path | ✅ Achieved | ReedWindVoice.cpp:425-428 — 50% cap for stability |
| Regression at defaults | ✅ Achieved | All 6 params at default = Phase 3.3 behavior (see analysis below) |
| 30 active parameters | ✅ Achieved | 24 prior + 6 new reads confirmed in code |

## Regression Safety Analysis

| Feature | Default | Bypass Mechanism | Phase 3.3 Identical |
|---------|---------|-----------------|---------------------|
| Infinite sustain | 0 | g=0.995 (unchanged), bellCutoff unchanged | ✅ |
| Reverse bore | 0 | normalCenters used (unchanged) | ✅ |
| Bore profile | Simple (0) | taperSimple = all 1.0 (unchanged) | ✅ |
| Dual bore | Off (false) | bore2 not processed, zero CPU cost | ✅ |
| Drone pitch | 0 cents | Unison (only used when dualBore=on) | ✅ |
| Feedback path | 0 | safeFeedback=0, only prevBoreMinus used | ✅ |

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU) | ✅ Pass | ninja O-Reed_VST3 O-Reed_AU — zero errors, no rebuild needed |
| Plugin Install | ✅ Pass | O-Reed-dev.vst3 + O-Reed-dev.component in system folders |
| AU Validation | ✅ Pass | `auval -v aumu ORed OuDv` — AU VALIDATION SUCCEEDED |
| pluginval Level 5 | ✅ Pass | SUCCESS |

## Code Quality Checks

| Check | Result | Notes |
|-------|--------|-------|
| ScopedNoDenormals | ✅ Pass | ReedWindVoice.cpp:262 |
| No heap allocation in audio | ✅ Pass | bore2 is stack member, no containers in render |
| Atomic parameter reads | ✅ Pass | All 30 via ->load() per-block |
| Feedback stability guard | ✅ Pass | feedbackPath * 0.5f caps energy injection at 50% |
| halfAngle clamp (5 deg) | ✅ Pass | BoreWaveguide.h:127 prevents extreme conical scaling |
| Near-zero halfAngle guard | ✅ Pass | BoreWaveguide.h:130-135 treats sub-1e-6 as cylindrical |
| bore2 lifecycle complete | ✅ Pass | prepare, reset (onset+hardstop), legato, render, snapFilters, energy check |
| dronePitch v1->v2 migration | ✅ Pass | PluginProcessor.cpp:414-423, detects old range |
| Dual bore skip when off | ✅ Pass | `if (dualBoreActive)` guards all bore2 processing |
| NaN/Inf guard | ✅ Pass | ReedWindVoice.cpp:465-469 output + feedback reset |
| Safety clip (tanh) | ✅ Pass | ReedWindVoice.cpp:471 |
| Post-block snapFiltersToZero | ✅ Pass | ReedWindVoice.cpp:479-480 both bores |
| Energy-based voice cleanup | ✅ Pass | ReedWindVoice.cpp:483-487 checks both bore energies |
| addSample (polyphonic safe) | ✅ Pass | ReedWindVoice.cpp:475 |

## Success Criteria (from PLAN-3.4.md)

| Criterion | Status | Evidence |
|-----------|--------|----------|
| INFINITE_SUSTAIN at 0: identical to Phase 3.3 | ✅ Code verified | g=0.995, bell unchanged |
| INFINITE_SUSTAIN at 1: tone sustains indefinitely | ⏸️ Manual | g=1.0, bell->Nyquist (lossless loop) |
| REVERSE_BORE at 0: identical to Phase 3.3 | ✅ Code verified | normalCenters used |
| REVERSE_BORE at 1 + boreCharacter > 0: unusual timbre | ⏸️ Manual | Scale factor inversion confirmed |
| DUAL_BORE off: zero additional CPU | ✅ Code verified | All bore2 processing gated by `if (dualBoreActive)` |
| DUAL_BORE on: second bore audible | ⏸️ Manual | bore2 processes same excitation, output mixed |
| DRONE_PITCH at 0: unison chorusing | ⏸️ Manual | `pow(2, 0/1200) = 1.0` |
| DRONE_PITCH at -1200: octave below | ⏸️ Manual | `pow(2, -1200/1200) = 0.5` |
| DRONE_PITCH at -700: fifth below (arghul-like) | ⏸️ Manual | `pow(2, -700/1200) ~ 0.66` |
| FEEDBACK_PATH at 0: bores independent | ✅ Code verified | safeFeedback=0 |
| FEEDBACK_PATH at 1 + INFINITE_SUSTAIN at 1: stable | ⏸️ Manual | safeFeedback capped at 0.5 + tanh clip |
| BORE_PROFILE Simple: identical to Phase 3.3 | ✅ Code verified | All ratios=1.0 |
| BORE_PROFILE Multi-segment: audible difference | ⏸️ Manual | Per-segment taper ratios applied |
| All 6 params at default: identical to Phase 3.3 | ✅ Code verified | Full regression analysis above |
| VST3 + AU build zero errors | ✅ Verified | Build output confirmed |
| auval PASS | ✅ Verified | AU VALIDATION SUCCEEDED |
| pluginval Level 5 PASS | ✅ Verified | SUCCESS |
| 30 active parameters confirmed | ✅ Verified | Code inspection count confirmed |

## Human Verification

- [ ] INFINITE_SUSTAIN at 100%: confirm tone sustains after note-off release
- [ ] REVERSE_BORE sweep with boreCharacter > 0: confirm unusual timbral shift
- [ ] DUAL_BORE on + DRONE_PITCH 0: confirm chorusing from two bores in unison
- [ ] DUAL_BORE on + DRONE_PITCH -1200: confirm octave-below drone
- [ ] DUAL_BORE on + DRONE_PITCH -700: confirm fifth-below drone (arghul-like)
- [ ] FEEDBACK_PATH sweep 0->1 with dual bore: confirm cross-modulation, no instability
- [ ] FEEDBACK_PATH 1.0 + INFINITE_SUSTAIN 1.0: confirm stable, no runaway
- [ ] BORE_PROFILE Simple vs Multi: confirm audible difference in timbral character
- [ ] All 6 new params at default: confirm identical sound to Phase 3.3

## Issues Found

None. Implementation matches plan exactly. No deviations reported in SUMMARY-3.4.md.

## Phase Verdict

**Status:** ✅ VERIFIED

**Ready for next phase:** Yes — Phase 3.5 (Oversampling + Tuning + MPE + Optimization)

**Blockers:** None
