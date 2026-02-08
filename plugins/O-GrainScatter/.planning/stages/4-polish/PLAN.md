# Stage 4: Polish - Plan

## Goal
Polish O-GrainScatter for 1.0.0 release: fix audio artifacts, improve UI interaction, prepare release artifacts.

## Tasks

### Layer 1: Audio Fixes (Critical)

**Task 1: Fix FreezeManager release crossfade**
- Add `releasing` state to FreezeManager alongside `active`
- On release(): set releasing=true, start crossfade counter (5ms)
- During release crossfade: grains still read frozen buffer but with fading gain
- When crossfade completes: set active=false, releasing=false
- Update `isActive()` to return true during release transition
- Files: `Source/dsp/FreezeManager.h`, `Source/PluginProcessor.cpp`

**Task 2: Add output soft-clipping**
- Apply `std::tanh()` to wet L/R after GrainPool sum, before dry/wet mix
- Use same compensation factor as feedback path (1.00497f)
- This prevents digital clipping when many grains overlap
- Files: `Source/PluginProcessor.cpp`

### Layer 2: UI Polish

**Task 3: Add double-click reset to knobs**
- Add `dblclick` event listener in `setupKnob()` function
- On double-click: call `state.sliderDragStarted()`, `state.setNormalisedValue(defaultNorm)`, `state.sliderDragEnded()`
- Default values: grain_size=0.5 (100ms with skew), density=0.5, most others=0.0, dry_wet=0.5, probability=1.0, repeats=0.2, euclidean_pulses=0.2, euclidean_steps=0.428
- Simpler approach: JUCE parameters have default values; use a reasonable normalized default
- Files: `Source/ui/public/js/app.js`

### Layer 3: Release Preparation

**Task 4: Create CHANGELOG.md**
- Document all features in 1.0.0 release
- Follow Keep a Changelog format
- Files: `plugins/O-GrainScatter/CHANGELOG.md`

**Task 5: Build, validate, install**
- CMake configure + ninja build (VST3 + AU)
- Run pluginval at strictness 5
- Install to system folders
- Clear AU cache

**Task 6: Update registry files**
- Update STATUS.md with Stage 4 completion
- Update PLUGINS.md with Working status and v1.0.0
- Files: `STATUS.md`, `PLUGINS.md`

## Success Criteria

1. Freeze engage AND release are click-free (crossfade on both transitions)
2. Output never clips digitally even at max density + max feedback
3. Knobs reset to default on double-click
4. CHANGELOG.md exists with complete 1.0.0 entry
5. pluginval strictness 5 PASS after all changes
6. Plugin installed to system folders
7. STATUS.md and PLUGINS.md updated for release
