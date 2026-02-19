# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-02-18
**Participants:** User, Claude

## Requirements Confirmed

### Bug Fixes (from user testing)

1. **Wavetable canvas displays not rendering waveforms**
   - Both Osc A and Osc B canvas areas are blank (no waveform drawn)
   - Canvas background renders (#EBD9C7 aged paper) but no waveform stroke/fill
   - Root cause investigation needed: `getWavetableFrameForPosition` native function may not be returning data, or `fetchAndDraw()` promise resolution may be failing silently (try/catch swallows errors)
   - C++ side: `processorRef.getFactoryTable(oscId)` returns from `factoryTables` vector (4 entries: Saw, Square, Triangle, Sine) -- data exists
   - JS side: `WavetableDisplay.fetchAndDraw()` calls native function, parses JSON array of samples, then draws. The catch block at line 981 silently ignores errors
   - Likely causes: timing issue with initial `setTimeout(refresh, 200)`, JSON format mismatch, or native function promise resolution issue

2. **Filter Routing dropdown misaligned**
   - The "Filter Routing" dropdown (Serial/Parallel) is centered on the full page width
   - Visually misaligned with the Filter A and Filter B type dropdowns above it
   - Fix: align routing dropdown to match filter section layout (e.g., between the two filter sections or right-aligned with Filter B)

3. **Noise generator sounds crackly**
   - User reports crackling artifacts from the noise generator
   - Potential causes identified in code review:
     - Brown noise output gain: `brownState * 3.5` -- may cause clipping
     - Wind noise output gain: `windLPState * 5.0` -- very high, likely clipping
     - Vinyl noise has intentional crackle (by design) but may be too aggressive
     - Digital noise is sample-and-hold quantized (8 levels) -- inherently harsh
   - Need to: audit gain staging for all 6 noise types, add soft clipping or gain normalization, verify no pipeline clipping

4. **Effects UI: remove sub-tabs, show all effects at once**
   - Current: Effects tab has a secondary tab bar (Reverb | Delay | Chorus | Distortion | EQ) -- only one effect visible at a time
   - Requested: Show all 5 effects on a single scrollable view within the Effects tab
   - Each effect keeps its section header and param row
   - Remove `effect-tab-bar`, `switchEffectTab()`, and `.effect-panel` show/hide logic

### Release Preparation

5. **Version: v0.9.0 (beta)**
   - Not v1.0.0 -- presets are deferred to v1.1
   - Plugin is functionally complete but shipping without factory presets
   - Version bump in CMakeLists.txt + any version display in UI

6. **CPU profiling and optimization**
   - Profile at max polyphony (16 voices) with unison (8 voices per osc)
   - Target: < 25% CPU on modern hardware
   - Optimize only if clearly problematic (> 40% at typical use)
   - Key concern: 16 voices x 8 unison x 2 osc = 256 simultaneous wavetable oscillators

7. **Testing: pluginval + Ableton + Logic**
   - pluginval at strictness 10 (already passing -- re-verify after fixes)
   - Verify in Ableton Live (VST3)
   - Verify in Logic Pro (AU)
   - Standalone testing for bug fix verification
   - State save/restore across DAW sessions

8. **CHANGELOG.md creation**
   - Document all features for v0.9.0 beta release

### Deferred Items (NOT in Stage 4)

- Factory presets (all 6 categories) -- deferred to v1.1
- Botanical watermark image -- deferred (CSS classes in place)
- Installer packaging -- not requested for beta

## Constraints Identified

- No presets means this is a beta release (v0.9.0)
- Bug fixes must not break existing pluginval pass
- Effects layout change is HTML/CSS only (no C++ changes needed)
- Noise generator fix may require gain staging changes in DSP code
- Wavetable display fix may require both C++ and JS investigation

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Version | v0.9.0 beta | No factory presets -- save v1.0 for preset release |
| Presets | Skip entirely | User preference -- add in v1.1 |
| Effects UI | All effects visible at once | User finds sub-tabs unnecessary for 5 effects |
| CPU target | Profile and optimize if needed | 25% target, optimize if >40% typical |
| Testing scope | pluginval + Ableton + Logic | Core DAW verification |
| Noise fix approach | Audit gain staging + soft clip | High output gains (3.5x, 5.0x) likely causing clipping |

## Bug Priority

| Bug | Severity | Impact |
|-----|----------|--------|
| Wavetable canvas blank | High | Core visual feature not working |
| Noise generator crackly | High | Audio quality issue |
| Effects sub-tabs removal | Medium | UX improvement (functional, not broken) |
| Filter routing alignment | Low | Cosmetic alignment issue |

## Open Questions

- Wavetable canvas: Is the native function returning data at all? Need to add console.log debugging to verify
- Noise crackling: Which specific noise types are affected? (All, or specific ones?)
- Effects layout: Should effects have vertical spacing/dividers between them on the single view?

## Next Phase

Ready for: research phase (investigate wavetable canvas and noise issues) or plan phase (if fixes are straightforward enough)
