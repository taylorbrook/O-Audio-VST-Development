# Stage 4: Polish & Validation - Execution Plan

## Stage Goal
Deliver a production-ready O-FreqPulse with optimized performance, factory presets, and full validation.

## Prerequisites
- Stage 3 GUI: ✅ VERIFIED (2026-02-03)
- Build: ✅ PASSED
- pluginval Level 5: ✅ PASSED

## Tasks

### Phase 1: Performance Profiling (Tasks 1-2)

#### Task 1: CPU Performance Audit
**Objective:** Measure baseline CPU usage at 44.1kHz stereo
**Actions:**
1. Create benchmark test scenario (sustained audio with all bands active)
2. Measure CPU using Activity Monitor / Instruments
3. Document current CPU percentage
4. Identify if optimization needed (target: <5%)

**Acceptance:** CPU usage documented, optimization plan if >5%

#### Task 2: Memory Footprint Check
**Objective:** Verify memory usage <50MB
**Actions:**
1. Launch plugin in DAW
2. Monitor memory via Activity Monitor
3. Document peak memory usage

**Acceptance:** Memory <50MB documented

### Phase 2: Audio Quality Validation (Tasks 3-5)

#### Task 3: Artifact Testing
**Objective:** Verify no audible FFT artifacts
**Actions:**
1. A/B test with bypassed signal (solo a sustained tone)
2. Test with drum transients - verify transient preservation
3. Listen for phase issues on stereo content
4. Document any issues found

**Acceptance:** No audible artifacts on typical material

#### Task 4: Smoothing Validation
**Objective:** Confirm gate transitions are click-free
**Actions:**
1. Test with smoothing at 0ms (may click)
2. Test with smoothing at 5ms (should be clean)
3. Test with smoothing at 50ms (definitely clean)
4. Verify the default (5ms) provides good balance

**Acceptance:** Gate transitions clean at default smoothing

#### Task 5: Sample Rate Testing
**Objective:** Verify operation at 44.1/48/96kHz
**Actions:**
1. Test plugin at 44.1kHz (primary)
2. Test plugin at 48kHz
3. Test plugin at 96kHz
4. Verify FFT processing correct at each rate

**Acceptance:** All three sample rates produce correct behavior

### Phase 3: Factory Presets (Tasks 6-7)

#### Task 6: Create Preset Infrastructure
**Objective:** Implement preset save/load if not present
**Actions:**
1. Verify APVTS state save/load works
2. Determine preset file format (XML chunks)
3. Create presets/ folder structure if needed

**Acceptance:** Preset mechanism functional

#### Task 7: Create 12 Factory Presets
**Objective:** Create diverse preset library covering all use cases
**Presets:**
1. **Init** - Default starting point (all manual, clean settings)
2. **Classic Sidechain** - Sub solid, mids pump at 1/4
3. **Trance Gate 16th** - All bands 16th note gating
4. **Dubstep Pulse** - Heavy sub gate, minimal highs
5. **Ambient Shimmer** - Slow Euclidean on highs only, high smoothing
6. **Polyrhythm 5-7-11** - Different Euclidean ratios per band
7. **Bass Foundation** - Sub always on, other bands gated
8. **Hi-Hat Chop** - Only high band gated fast
9. **Full Spectrum Gate** - Unified gating across all bands
10. **Euclidean Groove** - All bands in Euclidean mode, musical ratios
11. **Half-Time Feel** - Slower rate, dramatic pumping
12. **Triplet Bounce** - Triplet timing, groove preset

**Acceptance:** All 12 presets saved and loadable

### Phase 4: Final Validation (Tasks 8-10)

#### Task 8: pluginval Level 10
**Objective:** Pass strictest automated validation
**Actions:**
1. Run pluginval --strictness-level 10
2. Fix any failures
3. Document results

**Acceptance:** pluginval Level 10 PASSED

#### Task 9: DAW Compatibility Testing
**Objective:** Verify in Logic Pro, Ableton, Reaper
**Actions:**
1. Test in Logic Pro (AU and VST3)
2. Test in Ableton Live (VST3)
3. Test in Reaper (VST3 and AU)
4. Verify automation works in each DAW

**Acceptance:** Works in all three major DAWs

#### Task 10: Documentation Update
**Objective:** Update STATUS.md with final state
**Actions:**
1. Mark Stage 4 as complete
2. Document final CPU/memory stats
3. List all presets created
4. Add validation results
5. Mark plugin as PRODUCTION READY

**Acceptance:** STATUS.md reflects completed state

## Execution Order

```
[1] CPU Audit ────────┐
                      ├──> [3] Artifact Test ──┐
[2] Memory Check ─────┘                        │
                                               ├──> [6] Preset Infrastructure
[4] Smoothing Validation ──────────────────────┤
                                               │
[5] Sample Rate Test ──────────────────────────┘

[6] Preset Infrastructure ──> [7] Create 12 Presets

[8] pluginval Level 10 ────┐
                           ├──> [10] Documentation
[9] DAW Compatibility ─────┘
```

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| CPU >5% at 96kHz | SIMD optimization or FFT size reduction |
| Presets not loading | Use APVTS XML state mechanism |
| DAW-specific issues | Test early, workaround if needed |

## Success Criteria

- [ ] CPU <5% at 44.1kHz stereo
- [ ] Memory <50MB
- [ ] No audible artifacts
- [ ] 12 factory presets
- [ ] pluginval Level 10 PASSED
- [ ] Works in Logic, Ableton, Reaper

---

*Generated: 2026-02-04*
*Ready for execution*
