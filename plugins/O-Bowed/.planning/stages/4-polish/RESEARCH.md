# Stage 4: Polish - Research

**Date:** 2026-04-05
**Scope:** Factory presets (11), pluginval level 10, changelog, informal CPU check

---

## 1. Factory Preset System

### Module: OuariconPresetManager (Base Module)

Already integrated in processor at line 235:
```cpp
presetManager(parameters, "O-Bowed")
```

**Module location:** `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`

### API Pattern

```cpp
struct FactoryPresetDef {
    juce::String name;
    std::map<juce::String, float> parameters;  // paramID -> NORMALIZED value (0-1)
    juce::var customState;
};

void initializeFactoryPresets(const std::vector<FactoryPresetDef>& presets);
```

### CRITICAL: Values Must Be Normalized (0.0-1.0)

`applyPresetJson()` calls `setValueNotifyingHost(value)` which expects normalized 0-1 values. The `createPresetJson()` method uses `getValue()` which returns normalized values. Factory presets must match this convention.

### Initialization Pattern (follow O-AnalogEQ)

Add to `PluginProcessor.cpp`:
```cpp
// In constructor, after synthesiser setup:
initializeFactoryPresets();

// New private method:
void OBowedAudioProcessor::initializeFactoryPresets()
{
    auto factoryDir = presetManager.getFactoryPresetsDirectory();
    if (factoryDir.isDirectory() && 
        factoryDir.getNumberOfChildFiles(juce::File::findFiles) > 0)
        return;  // Already initialized

    std::vector<OuariconPresetManager::FactoryPresetDef> presets;
    // ... define 11 presets ...
    presetManager.initializeFactoryPresets(presets);
}
```

**Note:** Base module has no `factoryPresetsExist()` — use manual directory check as shown above. O-Bells has an extended version with this method, but O-Bowed uses the base module.

### Storage

- Factory presets: `~/Library/O-Bowed/Presets/Factory/*.json`
- User presets: `~/Library/O-Bowed/Presets/User/*.json`
- File format: JSON with `parameters`, `version`, `plugin`, `factory` fields
- No categories needed (11 presets, flat structure is fine)

---

## 2. Parameter Normalization Reference

### Normalization Formulas

**Skewed parameters** (JUCE NormalisableRange with skew):
```
normalized = pow((value - min) / (max - min), 1.0 / skew)
```

**Linear parameters:**
```
normalized = (value - min) / (max - min)
```

**Integer parameters:**
```
normalized = (value - min) / (max - min)
```

**Choice parameters:**
```
normalized = index / (numChoices - 1)
```

### Complete Parameter Table

| Parameter ID | Range | Skew | Default | Default Normalized | Formula |
|---|---|---|---|---|---|
| `bowSpeed` | 0.02-2.0 | 0.5 | 0.2 | 0.00826 | pow((v-0.02)/1.98, 2.0) |
| `bowPressure` | 0.01-5.0 | 0.5 | 0.5 | 0.00963 | pow((v-0.01)/4.99, 2.0) |
| `bowPosition` | 0.02-0.30 | 1.0 | 0.12 | 0.357 | (v-0.02)/0.28 |
| `rosin` | 0.0-1.0 | 1.0 | 0.5 | 0.5 | v |
| `bowNoise` | 0.0-1.0 | 1.0 | 0.0 | 0.0 | v |
| `bodyMaterial` | 0.0-1.0 | 1.0 | 0.4 | 0.4 | v |
| `bodySize` | 0.0-1.0 | 1.0 | 0.5 | 0.5 | v |
| `brightness` | 20-20000 | 0.25 | 8000 | 0.876 | pow((v-20)/19980, 4.0) |
| `stringCount` | 1-4 (int) | — | 1 | 0.0 | (v-1)/3.0 |
| `stringTuning1` | -2400-2400 | 1.0 | 0 | 0.5 | (v+2400)/4800 |
| `stringTuning2` | -2400-2400 | 1.0 | 0 | 0.5 | (v+2400)/4800 |
| `stringTuning3` | -2400-2400 | 1.0 | 0 | 0.5 | (v+2400)/4800 |
| `stringTuning4` | -2400-2400 | 1.0 | 0 | 0.5 | (v+2400)/4800 |
| `sympatheticAmount` | 0.0-1.0 | 1.0 | 0.0 | 0.0 | v |
| `sympatheticCount` | 0-12 (int) | — | 0 | 0.0 | v/12.0 |
| `width` | 0.0-2.0 | 1.0 | 1.0 | 0.5 | v/2.0 |
| `outputLevel` | -60-12 | 1.0 | 0.0 | 0.833 | (v+60)/72 |
| `infiniteSustain` | 0.0-1.0 | 1.0 | 0.0 | 0.0 | v |
| `reversedFriction` | 0.0-1.0 | 1.0 | 0.0 | 0.0 | v |
| `subHarmonics` | 0.0-1.0 | 1.0 | 0.0 | 0.0 | v |
| `frictionTier` | Choice(0,1,2) | — | 0 | 0.0 | index/2.0 |
| `referencePitch` | 220-880 | 1.0 | 440 | 0.333 | (v-220)/660 |
| `tuningSystem` | Choice(0,1,2) | — | 2 | 1.0 | index/2.0 |

---

## 3. Preset Specifications (Normalized Values)

### 3.1 Realistic Instruments (FUNC-10)

**Violin**
- bodyMaterial: 0.4 (wood), bodySize: 0.3 (small)
- stringCount: 1.0 (4 strings), stringTuning offsets: all 0.5 (centered)
- bowSpeed: ~0.01 (0.2 m/s), bowPressure: ~0.01 (0.5 N), bowPosition: 0.357 (0.12)
- rosin: 0.5, brightness: 0.876 (8kHz), width: 0.5 (stereo)
- All impossible physics: 0.0, frictionTier: 0.0 (Core)

**Cello**
- bodyMaterial: 0.4, bodySize: 0.7 (large)
- stringCount: 1.0 (4 strings)
- Deeper brightness: ~0.8 (lower cutoff ~5.5kHz)
- bowPressure slightly higher, bowSpeed slightly lower

**Viola**
- bodyMaterial: 0.4, bodySize: 0.45 (medium)
- stringCount: 1.0 (4 strings)
- Between violin/cello brightness

**Double Bass**
- bodyMaterial: 0.4, bodySize: 0.9 (very large)
- stringCount: 1.0 (4 strings)
- Lower brightness ~0.7, higher bow pressure

**Erhu**
- bodyMaterial: 0.15 (membrane), bodySize: 0.3 (small)
- stringCount: 0.0 (1 string)
- Higher rosin (0.65), nasal membrane body character
- No sympathetic strings

**Sarangi**
- bodyMaterial: 0.15 (membrane), bodySize: 0.45 (medium)
- stringCount: 0.0 (1 string)
- sympatheticCount: ~0.417-0.5 (5-6 strings), sympatheticAmount: 0.4
- Higher rosin, breathy

**Nyckelharpa**
- bodyMaterial: 0.4 (wood), bodySize: 0.4 (medium)
- stringCount: 1.0 (4 strings)
- sympatheticCount: ~0.833 (10 strings), sympatheticAmount: 0.5
- Prominent sympathetic resonance

### 3.2 Sound Design (FUNC-11)

**Glass Bow**
- bodyMaterial: 0.9 (glass), bodySize: 0.3
- brightness: ~0.95 (very high cutoff)
- infiniteSustain: 0.8, high brightness
- Ethereal, crystalline

**Metal Drone**
- bodyMaterial: 0.7 (metal), bodySize: 0.7
- subHarmonics: 0.6, reversedFriction: 0.4
- infiniteSustain: 0.5
- Dark, industrial drone

**Impossible Strings**
- All impossible physics active: infiniteSustain 0.7, reversedFriction 0.6, subHarmonics 0.8
- bodyMaterial: 0.5 (mixed), frictionTier: 1.0 (Quality)
- Maximum otherworldly character

**Breath of Strings**
- bodyMaterial: 0.4 (wood), bowNoise: 0.7 (high noise emphasis)
- bowPressure: very low (~0.003, ~0.3N), bowSpeed: low
- infiniteSustain: 0.3 (gentle sustain extension)
- Ethereal, barely-touching-the-string sound

---

## 4. Pluginval Level 10 Validation

### Command

```bash
# VST3
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 \
  --validate build/plugins/O-Bowed/O-Bowed_artefacts/Release/VST3/O-Bowed-dev.vst3

# AU
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 \
  --validate build/plugins/O-Bowed/O-Bowed_artefacts/Release/AU/O-Bowed-dev.component
```

### Level 10 vs Level 5

Level 10 adds over level 5:
- **Parameter thread safety** — multi-threaded parameter access
- **State restoration fuzzing** — random parameter combinations saved/restored
- **Automation stress testing** — rapid parameter changes during playback
- **Bus configuration tests** — different I/O arrangements
- **GUI editor tests** — editor lifecycle, resizing
- **Extended fuzz testing** — random parameters + MIDI sequences
- **Longer time-domain stress tests** — multiple sample rates/block sizes

### Common Level 10 Failure Modes (from codebase history)

1. **Voice cleanup/state reset** — O-Wind needed DSP reset in silent-counter voice cleanup path
2. **State persistence precision** — O-Formant replaced float epsilon comparison with int member for preset tracking
3. **Thread safety crashes** — Member declaration order bugs, UI thread accessing audio state
4. **Parameter automation glitches** — Rapid changes during playback
5. **Editor lifecycle** — Editor not handling resize or background thread state
6. **CPU timeout** — Heavy DSP can cause pluginval timeout (30s default)

### Pre-validation Checklist

Before running pluginval:
- [ ] Build Release config (`ninja O-Bowed_VST3 O-Bowed_AU`)
- [ ] All parameters have valid default values
- [ ] `processBlock` uses `ScopedNoDenormals`
- [ ] No allocations in audio thread
- [ ] State save/restore round-trips correctly
- [ ] Editor creates/destroys without leaks

### Potential O-Bowed Issues

- **Multi-string + sympathetic CPU** — 4 active + 12 sympathetic at level 10's stress test could timeout
- **Newton-Raphson convergence** — Fuzzing may find parameter combos that cause solver divergence
- **Body coefficient morphing** — Rapid material/size automation during playback
- **Elasto-plastic state** — Bristle displacement must reset cleanly on voice steal

---

## 5. Changelog Format

Follow codebase convention (see O-Bells CHANGELOG.md):

```markdown
# O-Bowed Changelog

All notable changes to O-Bowed will be documented in this file.

## [1.0.0] - 2026-04-05

### Added
- Initial release
- Physical modeling bowed string synthesis via digital waveguide + nonlinear friction junction
- Tiered friction model: Core (hyperbolic), Enhanced (elasto-plastic), Quality (thermal)
- Morphable body resonator with Material and Size controls (membrane/wood/metal/glass)
- 1-4 active bowed strings with per-string tuning offsets
- Sympathetic string coupling (0-12 passive waveguide strings)
- Impossible physics: Infinite Sustain, Reversed Friction, Sub-Harmonics
- MPE support (per-note pitch bend, pressure, slide)
- Microtonal tuning: Scala/TUN import, MTS-ESP, 12-TET
- WebView UI with Naturalist aesthetic
- 11 factory presets (7 realistic instruments + 4 sound design)
- Passes pluginval level 10 (VST3 + AU)

### Technical Notes
- 2x oversampling on friction junction
- 8-mode parallel biquad body resonator
- Zero algorithmic latency (waveguide is causal)
```

---

## 6. Implementation Order

Recommended task sequence:

1. **Define factory presets** — Add `initializeFactoryPresets()` method with 11 presets
2. **Build and test presets** — Verify each preset loads correctly and sounds appropriate
3. **Run pluginval level 10** — Fix any failures
4. **Informal CPU check** — Note performance with various configurations
5. **Write CHANGELOG.md** — v1.0.0 initial release
6. **Final build + install** — Fresh build, cache clear, install to system folders

---

## 7. Risks and Mitigations

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Preset values sound wrong | Medium | Test each preset aurally after creation; adjust normalized values |
| Pluginval level 10 timeout | Low-Medium | If timeout, add `--timeout-ms 120000` flag |
| Pluginval state fuzzing crash | Low | Check voice cleanup paths, ensure state round-trips |
| Preset overwrite on re-init | Low | Guard with directory file count check before writing |

---

## 8. Reference Implementations

- **O-AnalogEQ** (`PluginProcessor.cpp:92-203`) — Simple flat factory preset initialization, base module
- **O-Bells** (`PluginProcessor.cpp:906-1382`) — Extended version with categories and version checking
- **O-Lyrica** (`PluginProcessor.cpp:1094-1839`) — Version-checked regeneration pattern

O-Bowed should follow the **O-AnalogEQ pattern** (simplest, uses base module, flat presets) with the addition of a directory existence guard.
