# Stage 4: Polish - Research

**Date:** 2026-04-06
**Scope:** Factory presets (24), pluginval level 10, changelog, build verification
**Confidence:** HIGH

---

## 1. Factory Preset System

### Module: OuariconPresetManager (Base Module)

**Module location:** `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`

**NOT yet integrated in O-Reed.** Must add:

1. CMakeLists.txt include path: `${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp`
2. `#include "OuariconPresetManager.h"` in PluginProcessor.h
3. Member: `OuariconPresetManager presetManager;`
4. Constructor init: `presetManager(parameters, "O-Reed")`
5. Replace `getStateInformation`/`setStateInformation` with preset manager delegates
6. Add `initializeFactoryPresets()` method
7. Add `getPresetManager()` public accessor for editor
8. Add preset native functions in PluginEditor.cpp WebView builder
9. Add `std::shared_ptr<juce::FileChooser> fileChooser;` member in PluginEditor.h

### API Pattern (Base Module -- 3-field struct, no categories)

```cpp
struct FactoryPresetDef {
    juce::String name;
    std::map<juce::String, float> parameters;  // paramID -> NORMALIZED value (0-1)
    juce::var customState;
};

void initializeFactoryPresets(const std::vector<FactoryPresetDef>& presets);
```

**CRITICAL: Values must be normalized 0.0-1.0.** `applyPresetJson()` calls `setValueNotifyingHost(value)` which expects normalized values.

### Normalization Approach: Use `normalize()` Lambda

O-Wind uses a runtime `normalize()` helper in `initializeFactoryPresets()` to convert raw values to normalized values. This avoids manual math for skewed/ranged parameters. O-Reed MUST use the same pattern because of its many non-linear parameters.

```cpp
auto normalize = [this](const juce::String& paramId, float rawValue) -> float {
    if (auto* param = parameters.getParameter(paramId))
        return param->convertTo0to1(rawValue);
    return rawValue;
};
```

### Initialization Guard Pattern (from O-Bowed)

```cpp
void OReedAudioProcessor::initializeFactoryPresets()
{
    auto factoryDir = presetManager.getFactoryPresetsDirectory();
    if (factoryDir.isDirectory() &&
        factoryDir.getNumberOfChildFiles(juce::File::findFiles) > 0)
        return;  // Already initialized

    // ... define presets ...
    presetManager.initializeFactoryPresets(presets);
}
```

### State Delegation Pattern (from O-Bowed/O-Wind)

```cpp
void getStateInformation(juce::MemoryBlock& destData) override
{
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void setStateInformation(const void* data, int sizeInBytes) override
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}
```

**NOTE:** O-Reed currently has a `dronePitch` v1->v2 migration in `setStateInformation()`. This migration must be preserved in the new implementation. Handle it AFTER `presetManager.setStateFromXml()` by reading the state from `parameters.state` and patching old values.

### WebView Preset Native Functions (from O-Bowed PluginEditor.cpp)

Must add these `.withNativeFunction()` calls to the WebView builder chain:

- `getPresetList` -- returns array of preset names
- `getCurrentPreset` -- returns current preset name string
- `loadPreset` -- takes preset name, returns bool
- `savePreset` -- takes preset name, returns bool
- `selectNextPreset` -- navigates forward, returns new name
- `selectPreviousPreset` -- navigates backward, returns new name
- `savePresetWithDialog` -- launches file chooser, returns saved name

### Storage Location

- Factory presets: `~/Library/O-Reed/Presets/Factory/*.json`
- User presets: `~/Library/O-Reed/Presets/User/*.json`
- No category subdirectories (base module stores flat in Factory/)

---

## 2. Complete Parameter Reference (All 35 Parameters)

### Parameter Table with Normalization

| # | Parameter ID | Type | Range | Skew | Default | Default Norm | Notes |
|---|---|---|---|---|---|---|---|
| 1 | `breathPressure` | Float | 0.0-1.0 | 1.0 | 0.5 | 0.5 | Linear 0-1 |
| 2 | `embouchure` | Float | 0.0-1.0 | 1.0 | 0.4 | 0.4 | Linear 0-1 |
| 3 | `reedHardness` | Float | 0.0-1.0 | 1.0 | 0.5 | 0.5 | Linear 0-1 |
| 4 | `boreCharacter` | Float | 0.0-1.0 | 1.0 | 0.0 | 0.0 | Linear 0-1 |
| 5 | `instrumentPreset` | Choice | 21 items | -- | 0 | 0/20 | normalize(paramId, index) |
| 6 | `reedOpening` | Float | 0.0-1.0 | 1.0 | 0.4 | 0.4 | Linear 0-1 |
| 7 | `bellSize` | Float | 0.0-1.0 | 1.0 | 0.5 | 0.5 | Linear 0-1 |
| 8 | `airNoise` | Float | 0.0-1.0 | 1.0 | 0.15 | 0.15 | Linear 0-1 |
| 9 | `doubleReed` | Float | 0.0-1.0 | 1.0 | 0.0 | 0.0 | Psi confinement |
| 10 | `boreDiameter` | Float | 0.0-1.0 | 1.0 | 0.5 | 0.5 | Linear 0-1 |
| 11 | `reedMass` | Float | 0.0-1.0 | 1.0 | 0.3 | 0.3 | Linear 0-1 |
| 12 | `reedDamping` | Float | 0.0-1.0 | 1.0 | 0.5 | 0.5 | Linear 0-1 |
| 13 | `mouthpieceVol` | Float | 0.0-1.0 | 1.0 | 0.3 | 0.3 | Linear 0-1 |
| 14 | `toneHoleCutoff` | Float | 200-8000 Hz | 0.3 | 1500 | USE normalize() | Skewed! |
| 15 | `registerHole` | Float | 0.0-1.0 | 1.0 | 0.0 | 0.0 | Linear 0-1 |
| 16 | `boreLength` | Float | 0.0-1.0 | 1.0 | 0.5 | 0.5 | Linear 0-1 |
| 17 | `boreProfile` | Choice | 2 items | -- | 0 | 0.0 | Simple/Multi-segment |
| 18 | `vibratoDepth` | Float | 0.0-1.0 | 1.0 | 0.0 | 0.0 | Linear 0-1 |
| 19 | `vibratoRate` | Float | 1.0-10.0 Hz | 1.0 | 5.0 | USE normalize() | Non-0-1 range |
| 20 | `vibratoSource` | Choice | 3 items | -- | 0 | 0.0 | Lip/Breath/Throat |
| 21 | `growlAmount` | Float | 0.0-1.0 | 1.0 | 0.0 | 0.0 | Linear 0-1 |
| 22 | `flutterTongue` | Float | 0.0-1.0 | 1.0 | 0.0 | 0.0 | Linear 0-1 |
| 23 | `subtone` | Float | 0.0-1.0 | 1.0 | 0.0 | 0.0 | Linear 0-1 |
| 24 | `attackChiff` | Float | 0.0-1.0 | 1.0 | 0.3 | 0.3 | Linear 0-1 |
| 25 | `infiniteSustain` | Float | 0.0-1.0 | 1.0 | 0.0 | 0.0 | Linear 0-1 |
| 26 | `reverseBore` | Float | 0.0-1.0 | 1.0 | 0.0 | 0.0 | Linear 0-1 |
| 27 | `dualBore` | Bool | false/true | -- | false | 0.0 | false=0.0, true=1.0 |
| 28 | `dronePitch` | Float | -2400-2400 cents | 1.0 | 0 | USE normalize() | Non-0-1 range |
| 29 | `feedbackPath` | Float | 0.0-1.0 | 1.0 | 0.0 | 0.0 | Linear 0-1 |
| 30 | `referencePitch` | Float | 220-880 Hz | 1.0 | 440 | USE normalize() | Non-0-1 range |
| 31 | `tuningSystem` | Choice | 3 items | -- | 2 | 1.0 | Scala/MTS-ESP/12-TET |
| 32 | `polyMode` | Choice | 2 items | -- | 0 | 0.0 | Mono/Poly |
| 33 | `maxVoices` | Int | 1-16 | -- | 8 | USE normalize() | Non-0-1 range |
| 34 | `oversampling` | Choice | 2 items | -- | 0 | 0.0 | 2x/4x |
| 35 | `outputGain` | Float | -60-12 dB | 1.0 | 0.0 | USE normalize() | Non-0-1 range |

### Parameters Requiring normalize() (non-0-1 range or skewed)

Must use `normalize(paramId, rawValue)` for these 7 parameters:
- `toneHoleCutoff` (200-8000 Hz, skew 0.3)
- `vibratoRate` (1.0-10.0 Hz)
- `dronePitch` (-2400-2400 cents)
- `referencePitch` (220-880 Hz)
- `maxVoices` (1-16 int)
- `outputGain` (-60-12 dB)
- `instrumentPreset` (21-item choice -- use normalize for index)

Plus all Choice parameters should use normalize() for safety.

### Parameters Safe to Use Raw (linear 0-1 range, skew=1.0)

All other 24 float parameters have range 0.0-1.0 and can use raw values directly.
`dualBore` (bool) uses 0.0 for false, 1.0 for true.

---

## 3. Factory Preset Specifications

### 3.1 Western Reed Instruments (9)

All Western presets share these defaults unless noted:
- `referencePitch`: 440 Hz, `tuningSystem`: 12-TET (idx 2)
- `polyMode`: Monophonic (idx 0), `maxVoices`: 8, `oversampling`: 2x (idx 0)
- `outputGain`: 0 dB, `vibratoDepth`: 0.0, `vibratoRate`: 5.0 Hz
- `vibratoSource`: Lip (idx 0), `growlAmount`: 0.0, `flutterTongue`: 0.0
- `subtone`: 0.0, `infiniteSustain`: 0.0, `reverseBore`: 0.0
- `dualBore`: false, `dronePitch`: 0, `feedbackPath`: 0.0
- `boreProfile`: Simple (idx 0), `registerHole`: 0.0

#### Bb Clarinet
- `breathPressure`: 0.5, `embouchure`: 0.4, `reedHardness`: 0.5
- `boreCharacter`: 0.0 (cylindrical), `doubleReed`: 0.0
- `reedOpening`: 0.4, `bellSize`: 0.5, `airNoise`: 0.1
- `boreDiameter`: 0.45, `reedMass`: 0.3, `reedDamping`: 0.5
- `mouthpieceVol`: 0.3, `toneHoleCutoff`: 1500 Hz, `boreLength`: 0.5
- `attackChiff`: 0.25, `instrumentPreset`: idx 0

#### Bass Clarinet
- `breathPressure`: 0.55, `embouchure`: 0.35, `reedHardness`: 0.4
- `boreCharacter`: 0.0, `doubleReed`: 0.0
- `reedOpening`: 0.5, `bellSize`: 0.6, `airNoise`: 0.12
- `boreDiameter`: 0.6, `reedMass`: 0.4, `reedDamping`: 0.45
- `mouthpieceVol`: 0.35, `toneHoleCutoff`: 1000 Hz, `boreLength`: 0.7
- `attackChiff`: 0.2, `instrumentPreset`: idx 1

#### Alto Saxophone
- `breathPressure`: 0.55, `embouchure`: 0.45, `reedHardness`: 0.5
- `boreCharacter`: 0.8 (conical), `doubleReed`: 0.0
- `reedOpening`: 0.45, `bellSize`: 0.65, `airNoise`: 0.12
- `boreDiameter`: 0.55, `reedMass`: 0.3, `reedDamping`: 0.45
- `mouthpieceVol`: 0.35, `toneHoleCutoff`: 2000 Hz, `boreLength`: 0.5
- `attackChiff`: 0.35, `instrumentPreset`: idx 2

#### Tenor Saxophone
- `breathPressure`: 0.5, `embouchure`: 0.42, `reedHardness`: 0.45
- `boreCharacter`: 0.75, `doubleReed`: 0.0
- `reedOpening`: 0.5, `bellSize`: 0.7, `airNoise`: 0.1
- `boreDiameter`: 0.6, `reedMass`: 0.35, `reedDamping`: 0.4
- `mouthpieceVol`: 0.35, `toneHoleCutoff`: 1800 Hz, `boreLength`: 0.55
- `attackChiff`: 0.3, `subtone`: 0.15, `instrumentPreset`: idx 3

#### Soprano Saxophone
- `breathPressure`: 0.55, `embouchure`: 0.5, `reedHardness`: 0.55
- `boreCharacter`: 0.85, `doubleReed`: 0.0
- `reedOpening`: 0.4, `bellSize`: 0.5, `airNoise`: 0.12
- `boreDiameter`: 0.4, `reedMass`: 0.25, `reedDamping`: 0.5
- `mouthpieceVol`: 0.3, `toneHoleCutoff`: 2500 Hz, `boreLength`: 0.4
- `attackChiff`: 0.35, `instrumentPreset`: idx 4

#### Baritone Saxophone
- `breathPressure`: 0.6, `embouchure`: 0.38, `reedHardness`: 0.4
- `boreCharacter`: 0.7, `doubleReed`: 0.0
- `reedOpening`: 0.55, `bellSize`: 0.8, `airNoise`: 0.1
- `boreDiameter`: 0.7, `reedMass`: 0.45, `reedDamping`: 0.4
- `mouthpieceVol`: 0.4, `toneHoleCutoff`: 1200 Hz, `boreLength`: 0.7
- `attackChiff`: 0.25, `instrumentPreset`: idx 5

#### Oboe
- `breathPressure`: 0.45, `embouchure`: 0.5, `reedHardness`: 0.6
- `boreCharacter`: 0.8, `doubleReed`: 0.4 (Psi confinement)
- `reedOpening`: 0.3, `bellSize`: 0.35, `airNoise`: 0.08
- `boreDiameter`: 0.35, `reedMass`: 0.2, `reedDamping`: 0.55
- `mouthpieceVol`: 0.25, `toneHoleCutoff`: 3000 Hz, `boreLength`: 0.45
- `attackChiff`: 0.35, `instrumentPreset`: idx 6

#### English Horn
- `breathPressure`: 0.45, `embouchure`: 0.48, `reedHardness`: 0.55
- `boreCharacter`: 0.75, `doubleReed`: 0.35
- `reedOpening`: 0.35, `bellSize`: 0.3 (pear-shaped = lowpass), `airNoise`: 0.06
- `boreDiameter`: 0.4, `reedMass`: 0.25, `reedDamping`: 0.5
- `mouthpieceVol`: 0.3, `toneHoleCutoff`: 2200 Hz, `boreLength`: 0.55
- `attackChiff`: 0.3, `instrumentPreset`: idx 7

#### Bassoon
- `breathPressure`: 0.5, `embouchure`: 0.45, `reedHardness`: 0.45
- `boreCharacter`: 0.7, `doubleReed`: 0.3
- `reedOpening`: 0.4, `bellSize`: 0.45, `airNoise`: 0.1
- `boreDiameter`: 0.45, `reedMass`: 0.35, `reedDamping`: 0.45
- `mouthpieceVol`: 0.35, `toneHoleCutoff`: 1800 Hz, `boreLength`: 0.65
- `attackChiff`: 0.25, `instrumentPreset`: idx 8

### 3.2 Non-Western Reed Instruments (9)

All share same tuning/voice/output defaults as Western unless noted.

#### Duduk
- `breathPressure`: 0.35, `embouchure`: 0.45, `reedHardness`: 0.35
- `boreCharacter`: 0.0 (cylindrical), `doubleReed`: 0.25
- `reedOpening`: 0.55, `bellSize`: 0.2, `airNoise`: 0.05
- `boreDiameter`: 0.5, `reedMass`: 0.4, `reedDamping`: 0.35
- `mouthpieceVol`: 0.35, `toneHoleCutoff`: 1200 Hz, `boreLength`: 0.5
- `attackChiff`: 0.15, `vibratoDepth`: 0.15, `vibratoRate`: 4.5 Hz
- `instrumentPreset`: idx 9

#### Shehnai
- `breathPressure`: 0.7, `embouchure`: 0.55, `reedHardness`: 0.6
- `boreCharacter`: 0.85, `doubleReed`: 0.7 (quadruple reed)
- `reedOpening`: 0.35, `bellSize`: 0.7, `airNoise`: 0.1
- `boreDiameter`: 0.35, `reedMass`: 0.2, `reedDamping`: 0.6
- `mouthpieceVol`: 0.2, `toneHoleCutoff`: 4000 Hz, `boreLength`: 0.4
- `attackChiff`: 0.4, `instrumentPreset`: idx 10

#### Suona
- `breathPressure`: 0.75, `embouchure`: 0.6, `reedHardness`: 0.55
- `boreCharacter`: 0.9, `doubleReed`: 0.5
- `reedOpening`: 0.3, `bellSize`: 0.85, `airNoise`: 0.08
- `boreDiameter`: 0.35, `reedMass`: 0.2, `reedDamping`: 0.65
- `mouthpieceVol`: 0.15, `toneHoleCutoff`: 5000 Hz, `boreLength`: 0.35
- `attackChiff`: 0.45, `instrumentPreset`: idx 11

#### Hichiriki
- `breathPressure`: 0.5, `embouchure`: 0.5, `reedHardness`: 0.5
- `boreCharacter`: 0.0 (cylindrical), `doubleReed`: 0.45, `reverseBore`: 0.4
- `reedOpening`: 0.45, `bellSize`: 0.3, `airNoise`: 0.1
- `boreDiameter`: 0.4, `reedMass`: 0.3, `reedDamping`: 0.45
- `mouthpieceVol`: 0.3, `toneHoleCutoff`: 2500 Hz, `boreLength`: 0.4
- `attackChiff`: 0.3, `instrumentPreset`: idx 12

#### Zurna
- `breathPressure`: 0.75, `embouchure`: 0.55, `reedHardness`: 0.6
- `boreCharacter`: 0.85, `doubleReed`: 0.6
- `reedOpening`: 0.3, `bellSize`: 0.75, `airNoise`: 0.08
- `boreDiameter`: 0.35, `reedMass`: 0.2, `reedDamping`: 0.6
- `mouthpieceVol`: 0.2, `toneHoleCutoff`: 4500 Hz, `boreLength`: 0.35
- `attackChiff`: 0.4, `instrumentPreset`: idx 13

#### Piri
- `breathPressure`: 0.45, `embouchure`: 0.5, `reedHardness`: 0.5
- `boreCharacter`: 0.0 (cylindrical), `doubleReed`: 0.35
- `reedOpening`: 0.45, `bellSize`: 0.15, `airNoise`: 0.1
- `boreDiameter`: 0.4, `reedMass`: 0.3, `reedDamping`: 0.45
- `mouthpieceVol`: 0.3, `toneHoleCutoff`: 2000 Hz, `boreLength`: 0.45
- `attackChiff`: 0.25, `vibratoDepth`: 0.1, `vibratoRate`: 5.0 Hz
- `instrumentPreset`: idx 14

#### Arghul
- `breathPressure`: 0.45, `embouchure`: 0.4, `reedHardness`: 0.45
- `boreCharacter`: 0.0 (cylindrical), `doubleReed`: 0.0 (idioglot)
- `reedOpening`: 0.45, `bellSize`: 0.2, `airNoise`: 0.1
- `boreDiameter`: 0.45, `reedMass`: 0.3, `reedDamping`: 0.5
- `mouthpieceVol`: 0.3, `toneHoleCutoff`: 1500 Hz, `boreLength`: 0.5
- `attackChiff`: 0.2
- **`dualBore`: true, `dronePitch`: -1200 cents (-12 semitones)**
- `instrumentPreset`: idx 15

#### Launeddas
- `breathPressure`: 0.4, `embouchure`: 0.42, `reedHardness`: 0.45
- `boreCharacter`: 0.0 (cylindrical), `doubleReed`: 0.0 (idioglot)
- `reedOpening`: 0.4, `bellSize`: 0.15, `airNoise`: 0.08
- `boreDiameter`: 0.4, `reedMass`: 0.3, `reedDamping`: 0.5
- `mouthpieceVol`: 0.3, `toneHoleCutoff`: 1800 Hz, `boreLength`: 0.45
- `attackChiff`: 0.2
- **`dualBore`: true, `dronePitch`: -50 cents (slight detune)**
- `instrumentPreset`: idx 16

#### Mijwiz
- `breathPressure`: 0.45, `embouchure`: 0.4, `reedHardness`: 0.45
- `boreCharacter`: 0.0 (cylindrical), `doubleReed`: 0.0 (idioglot)
- `reedOpening`: 0.45, `bellSize`: 0.2, `airNoise`: 0.1
- `boreDiameter`: 0.45, `reedMass`: 0.3, `reedDamping`: 0.5
- `mouthpieceVol`: 0.3, `toneHoleCutoff`: 1500 Hz, `boreLength`: 0.5
- `attackChiff`: 0.2
- **`dualBore`: true, `dronePitch`: 0 cents (unison beating)**
- `instrumentPreset`: idx 17

### 3.3 Sound Design (6)

#### Glass Reed
- `breathPressure`: 0.4, `embouchure`: 0.5, `reedHardness`: 0.9
- `boreCharacter`: 0.0, `doubleReed`: 0.0
- `reedOpening`: 0.25, `bellSize`: 0.3, `airNoise`: 0.0
- `boreDiameter`: 0.3, `reedMass`: 0.15, `reedDamping`: 0.15
- `mouthpieceVol`: 0.2, `toneHoleCutoff`: 6000 Hz, `boreLength`: 0.35
- `attackChiff`: 0.0, `infiniteSustain`: 0.6
- `instrumentPreset`: idx 18

#### Metal Wind
- `breathPressure`: 0.55, `embouchure`: 0.5, `reedHardness`: 0.6
- `boreCharacter`: 0.5, `doubleReed`: 0.2
- `reedOpening`: 0.35, `bellSize`: 0.7, `airNoise`: 0.15
- `boreDiameter`: 0.5, `reedMass`: 0.25, `reedDamping`: 0.6
- `mouthpieceVol`: 0.3, `toneHoleCutoff`: 3000 Hz, `boreLength`: 0.5
- `attackChiff`: 0.3, `infiniteSustain`: 0.7
- `instrumentPreset`: idx 19

#### Impossible Bore
- `breathPressure`: 0.5, `embouchure`: 0.5, `reedHardness`: 0.5
- `boreCharacter`: 0.5, `doubleReed`: 0.3, `reverseBore`: 0.6
- `reedOpening`: 0.4, `bellSize`: 0.5, `airNoise`: 0.2
- `boreDiameter`: 0.5, `reedMass`: 0.3, `reedDamping`: 0.4
- `mouthpieceVol`: 0.3, `toneHoleCutoff`: 3000 Hz, `boreLength`: 0.5
- `attackChiff`: 0.25, `infiniteSustain`: 0.3
- **`dualBore`: true, `dronePitch`: 700 cents, `feedbackPath`: 0.5**
- `instrumentPreset`: idx 20

#### Breath Drone
- `breathPressure`: 0.3, `embouchure`: 0.35, `reedHardness`: 0.3
- `boreCharacter`: 0.0, `doubleReed`: 0.0
- `reedOpening`: 0.2, `bellSize`: 0.3, `airNoise`: 0.7
- `boreDiameter`: 0.5, `reedMass`: 0.4, `reedDamping`: 0.3
- `mouthpieceVol`: 0.35, `toneHoleCutoff`: 800 Hz, `boreLength`: 0.6
- `attackChiff`: 0.0, `infiniteSustain`: 0.3
- Custom `instrumentPreset`: N/A (use idx 0 as base)

#### Giant Clarinet
- `breathPressure`: 0.6, `embouchure`: 0.35, `reedHardness`: 0.35
- `boreCharacter`: 0.0 (cylindrical), `doubleReed`: 0.0
- `reedOpening`: 0.55, `bellSize`: 0.5, `airNoise`: 0.12
- `boreDiameter`: 0.8, `reedMass`: 0.5, `reedDamping`: 0.35
- `mouthpieceVol`: 0.4, `toneHoleCutoff`: 600 Hz, `boreLength`: 1.0
- `attackChiff`: 0.15
- Custom `instrumentPreset`: idx 0 (Bb Clarinet base)

#### Micro Reed
- `breathPressure`: 0.4, `embouchure`: 0.6, `reedHardness`: 0.7
- `boreCharacter`: 0.5, `doubleReed`: 0.2
- `reedOpening`: 0.2, `bellSize`: 0.15, `airNoise`: 0.05
- `boreDiameter`: 0.15, `reedMass`: 0.1, `reedDamping`: 0.7
- `mouthpieceVol`: 0.15, `toneHoleCutoff`: 7000 Hz, `boreLength`: 0.1
- `attackChiff`: 0.4
- Custom `instrumentPreset`: N/A (use idx 0 as base)

---

## 4. Pluginval Level 10 Validation

### Command

```bash
# VST3
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 \
  --validate build/plugins/O-Reed/O-Reed_artefacts/Release/VST3/O-Reed-dev.vst3

# AU
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 \
  --validate build/plugins/O-Reed/O-Reed_artefacts/Release/AU/O-Reed-dev.component
```

### Level 10 vs Level 5 Differences

Level 10 adds:
- **Parameter thread safety** -- multi-threaded parameter access stress
- **State restoration fuzzing** -- random parameter combinations saved/restored
- **Automation stress testing** -- rapid parameter changes during playback
- **Bus configuration tests** -- different I/O arrangements
- **GUI editor tests** -- editor lifecycle, resizing
- **Extended fuzz testing** -- random parameters + MIDI sequences
- **Longer time-domain stress tests** -- multiple sample rates/block sizes

### Existing Strengths (O-Reed already has)

- `ScopedNoDenormals` in `processBlock` (line 377)
- Clean buffer clear at start of `processBlock`
- APVTS-based parameter access (thread-safe atomics)
- All voice DSP in header-only classes (no dynamic allocation in audio thread)
- Already passes pluginval L10 from Stage 2 Phase 3.5

### Potential Issues After Preset Integration

1. **State save/restore with preset manager** -- Adding `OuariconPresetManager` changes the state format from raw APVTS XML to preset-manager-wrapped XML. If the dronePitch migration is not preserved, old saved states could break.

2. **Factory preset file I/O in constructor** -- `initializeFactoryPresets()` writes JSON files to disk during plugin construction. This could fail during pluginval's rapid create/destroy cycles if the filesystem is locked. The guard check (`factoryDir.isDirectory() && fileCount > 0`) prevents rewrites but still does a directory listing.

3. **FileChooser in editor** -- The `savePresetWithDialog` native function uses async `FileChooser`. This is fine for pluginval since it uses `--skip-gui-tests` by default at level 10.

### Pre-validation Checklist

- [ ] Build Release config (`ninja O-Reed_VST3 O-Reed_AU`)
- [ ] All 35 parameters have valid default values (CONFIRMED)
- [ ] `processBlock` uses `ScopedNoDenormals` (CONFIRMED)
- [ ] No allocations in audio thread (CONFIRMED -- header-only DSP)
- [ ] State save/restore round-trips correctly with preset manager
- [ ] dronePitch v1->v2 migration preserved
- [ ] Editor creates/destroys without leaks
- [ ] Factory presets written only once (guard check)

### Timeout Strategy

If pluginval times out (level 10 can be slow with physical modeling):
```bash
--timeout-ms 120000  # 2 minutes per test instead of default 30s
```

---

## 5. Files to Modify

### PluginProcessor.h Changes

```cpp
// Add include
#include "OuariconPresetManager.h"

// Add public accessor
OuariconPresetManager& getPresetManager() { return presetManager; }

// Add private member (AFTER parameters declaration)
OuariconPresetManager presetManager;

// Add private method declaration
void initializeFactoryPresets();
```

### PluginProcessor.cpp Changes

1. Constructor: add `presetManager(parameters, "O-Reed")` to init list, call `initializeFactoryPresets()`
2. `getStateInformation()`: delegate to `presetManager.getStateAsXml()`
3. `setStateInformation()`: delegate to `presetManager.setStateFromXml()`, then apply dronePitch migration
4. New `initializeFactoryPresets()` method with 24 preset definitions

### CMakeLists.txt Changes

Add to `target_include_directories`:
```cmake
${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
```

### PluginEditor.h Changes

Add `std::shared_ptr<juce::FileChooser> fileChooser;` member.

### PluginEditor.cpp Changes

Add 7 preset native functions to WebView builder chain (after tuning functions).

---

## 6. Changelog Format

Follow codebase convention:

```markdown
# O-Reed Changelog

## [1.0.0] - 2026-04-06

### Added
- Initial release
- Physical modeling reed wind instrument synthesis (Guillemain Psi model)
- 35 parameters across 8 categories
- 9 Western reed presets (Bb Clarinet, Bass Clarinet, Alto/Tenor/Soprano/Baritone Saxophone, Oboe, English Horn, Bassoon)
- 9 Non-Western reed presets (Duduk, Shehnai, Suona, Hichiriki, Zurna, Piri, Arghul, Launeddas, Mijwiz)
- 6 Sound Design presets (Glass Reed, Metal Wind, Impossible Bore, Breath Drone, Giant Clarinet, Micro Reed)
- Impossible physics: Infinite Sustain, Reverse Bore, Dual Bore with drone, Feedback Path
- Conical bore waveguide with viscothermal losses and Thiran allpass interpolation
- Mouthpiece chamber (Helmholtz resonance) and tone hole modeling
- Expressive controls: vibrato, growl, flutter tongue, subtone, attack chiff
- MPE support (per-note pitch bend, pressure, slide)
- Microtonal tuning: Scala/TUN import, MTS-ESP, 12-TET
- WebView UI with 3-tab layout (Instrument, Expression, Advanced)
- 2x/4x oversampling
- Passes pluginval level 10 (VST3 + AU)

### Technical Notes
- Mass-spring-damper reed ODE with symplectic Euler integration
- Strategy C conical bore waveguide (2x Thiran delay lines)
- Guillemain Psi nonlinear reed-bore coupling
- Zero algorithmic latency (waveguide is causal, oversampling latency reported)
```

---

## 7. Implementation Order

1. **CMakeLists.txt** -- Add preset-manager include path
2. **PluginProcessor.h** -- Add include, member, accessor, method declaration
3. **PluginProcessor.cpp** -- Integrate preset manager, define 24 factory presets
4. **PluginEditor.h** -- Add fileChooser member
5. **PluginEditor.cpp** -- Add 7 preset native functions
6. **Build** -- `ninja O-Reed_VST3 O-Reed_AU`
7. **Test presets** -- Verify each loads and sounds appropriate
8. **pluginval level 10** -- Run for both VST3 and AU, fix any failures
9. **CHANGELOG.md** -- Write v1.0.0 changelog
10. **Final install** -- Cache clear, install, DAW verify

---

## 8. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Preset values sound wrong | Medium | Low | Use normalize() lambda, test each aurally |
| dronePitch migration breaks | Low | Medium | Preserve migration code after setStateFromXml |
| pluginval L10 regression | Low | Medium | Already passes L10 from Stage 2; preset manager adds no audio-thread changes |
| Factory preset file I/O during validation | Low | Low | Guard check prevents rewrites; file I/O is lazy |
| Normalized value precision | Low | Low | Use normalize() for all non-0-1 params |

---

## 9. Environment Availability

| Dependency | Required By | Available | Version |
|------------|------------|-----------|---------|
| pluginval | Validation | Yes | /Applications/pluginval.app (JUCE v8.0.3 based) |
| ninja | Build | Yes | Build system configured |
| auval | AU validation | Yes | macOS built-in |

No missing dependencies.
