# Stage 4: Polish - Execution Plan

**Date:** 2026-04-05
**Goal:** Ship-ready O-Bowed v1.0.0 with 11 factory presets, pluginval level 10 validation, and changelog.

---

## Tasks

### 1. [ ] Add `initializeFactoryPresets()` method to processor

**Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
**Depends on:** none

- Declare private `void initializeFactoryPresets()` in header
- Implement method in .cpp following O-AnalogEQ pattern:
  - Guard: check if factory directory already has files (skip if populated)
  - Build `std::vector<OuariconPresetManager::FactoryPresetDef>` with all 11 presets
  - Call `presetManager.initializeFactoryPresets(factoryPresets)`
- Call `initializeFactoryPresets()` at end of constructor (after synthesiser setup)
- All values **normalized 0.0-1.0** per RESEARCH.md parameter table

### 2. [ ] Define 7 realistic instrument presets

**Files:** `Source/PluginProcessor.cpp` (inside `initializeFactoryPresets()`)
**Depends on:** Task 1

All 23 parameters must be specified per preset. Normalized values from RESEARCH.md Section 3.1:

| Preset | bodyMaterial | bodySize | stringCount | sympatheticCount | sympatheticAmount | Key Character |
|--------|-------------|----------|-------------|-----------------|-------------------|---------------|
| Violin | 0.4 | 0.3 | 1.0 (4) | 0.0 | 0.0 | Standard bow, wood, small |
| Cello | 0.4 | 0.7 | 1.0 (4) | 0.0 | 0.0 | Lower brightness ~0.8 |
| Viola | 0.4 | 0.45 | 1.0 (4) | 0.0 | 0.0 | Between violin/cello |
| Double Bass | 0.4 | 0.9 | 1.0 (4) | 0.0 | 0.0 | Lowest brightness ~0.7 |
| Erhu | 0.15 | 0.3 | 0.0 (1) | 0.0 | 0.0 | Membrane, nasal, rosin 0.65 |
| Sarangi | 0.15 | 0.45 | 0.0 (1) | 0.417-0.5 | 0.4 | Membrane, sympathetic |
| Nyckelharpa | 0.4 | 0.4 | 1.0 (4) | 0.833 (10) | 0.5 | Wood, prominent sympathetic |

Shared defaults for realistic: all impossible physics = 0.0, frictionTier = 0.0 (Core), tuningSystem = 1.0 (12-TET), referencePitch = 0.333 (440Hz), outputLevel = 0.833 (0dB).

### 3. [ ] Define 4 sound design presets

**Files:** `Source/PluginProcessor.cpp` (inside `initializeFactoryPresets()`)
**Depends on:** Task 1

| Preset | Key Character |
|--------|---------------|
| Glass Bow | bodyMaterial 0.9 (glass), infiniteSustain 0.8, brightness ~0.95 |
| Metal Drone | bodyMaterial 0.7 (metal), subHarmonics 0.6, reversedFriction 0.4, infiniteSustain 0.5 |
| Impossible Strings | All impossible physics cranked, frictionTier 1.0 (Quality) |
| Breath of Strings | bowNoise 0.7, very low pressure ~0.003, infiniteSustain 0.3, ethereal |

### 4. [ ] Build and aural-test presets

**Files:** none (build + listen)
**Depends on:** Tasks 2, 3

- `ninja O-Bowed_VST3 O-Bowed_AU`
- Install to system folders (with cache clear)
- Load each preset in DAW, verify:
  - Preset loads without error
  - Sound character matches intent (violin sounds violin-like, glass sounds crystalline, etc.)
  - All parameters update in UI when preset loads
- Adjust normalized values if any preset sounds wrong

### 5. [ ] Run pluginval level 10 (VST3 + AU)

**Files:** none (validation)
**Depends on:** Task 4

```bash
# VST3
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 --timeout-ms 120000 \
  --validate build/plugins/O-Bowed/O-Bowed_artefacts/Release/VST3/O-Bowed-dev.vst3

# AU
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 --timeout-ms 120000 \
  --validate build/plugins/O-Bowed/O-Bowed_artefacts/Release/AU/O-Bowed-dev.component
```

Watch for known risks:
- CPU timeout with multi-string + sympathetic (mitigate with --timeout-ms 120000)
- Newton-Raphson divergence under fuzz testing
- Elasto-plastic state not resetting on voice steal
- Body coefficient morphing during rapid automation

Fix any failures before proceeding.

### 6. [ ] Informal CPU check

**Files:** none (observation)
**Depends on:** Task 5

- Load plugin in DAW with Activity Monitor open
- Test configurations:
  - 1 string, Core tier (baseline)
  - 4 strings, Core tier
  - 4 strings + 12 sympathetic, Quality tier (worst case)
- Confirm "reasonable" CPU usage — no formal benchmark needed
- Note any concerns

### 7. [ ] Write CHANGELOG.md

**Files:** `plugins/O-Bowed/CHANGELOG.md` (new file)
**Depends on:** Task 5

Follow codebase convention (O-Bells format). Content from RESEARCH.md Section 5:
- v1.0.0 initial release
- List all features: waveguide synthesis, tiered friction, morphable body, multi-string, sympathetic, impossible physics, MPE, microtonal, WebView UI, 11 factory presets, pluginval level 10

### 8. [ ] Final build + install + verify

**Files:** none (build/install)
**Depends on:** Task 7

- Clean build: `ninja O-Bowed_VST3 O-Bowed_AU`
- Clear AU cache, remove old binaries, install fresh
- Verify AU appears: `auval -a | grep -i bowed`
- Quick smoke test in DAW: load preset, play notes, confirm sound

---

## Success Criteria

- [ ] 11 factory presets load correctly (7 realistic + 4 sound design)
- [ ] Each preset produces character matching its name
- [ ] Pluginval level 10 passes for both VST3 and AU
- [ ] CPU usage is reasonable across configurations
- [ ] CHANGELOG.md exists with v1.0.0 entry
- [ ] Fresh build installs and works in DAW

---

## Files Created/Modified

| File | Action |
|------|--------|
| `Source/PluginProcessor.h` | Add `initializeFactoryPresets()` declaration |
| `Source/PluginProcessor.cpp` | Add factory preset method + constructor call |
| `CHANGELOG.md` | New file (v1.0.0) |

## Estimated Scope

- 8 tasks, ~200 lines of new code (mostly preset data)
- Primary risk: preset tuning by ear, pluginval level 10 edge cases
