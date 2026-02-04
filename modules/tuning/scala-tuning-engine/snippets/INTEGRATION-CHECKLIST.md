# scala-tuning-engine Integration Checklist

Use this checklist when adding the tuning module to a new plugin.

## Prerequisites

- [ ] WebView-based plugin with JUCE 8+
- [ ] Working APVTS parameter system
- [ ] C++20 standard enabled

---

## Step 1: Copy C++ Source Files

Copy from `modules/tuning/scala-tuning-engine/cpp/` to your plugin's `Source/` folder:

- [ ] `TuningEngine.h`
- [ ] `TuningEngine.cpp`
- [ ] `ScaleGenerator.h`
- [ ] `ScaleGenerator.cpp`
- [ ] `TuningExporter.h`
- [ ] `TuningExporter.cpp`
- [ ] `EmbeddedTunings.h`
- [ ] `EmbeddedTunings.cpp`

---

## Step 2: Update CMakeLists.txt

Add to your `target_sources()`:

```cmake
target_sources(${PROJECT_NAME} PRIVATE
    Source/TuningEngine.cpp
    Source/ScaleGenerator.cpp
    Source/TuningExporter.cpp
    Source/EmbeddedTunings.cpp
)
```

- [ ] CMakeLists.txt updated

---

## Step 3: Add APVTS Parameters

Reference: `snippets/parameters.cpp`

Add to your `createParameterLayout()`:
- [ ] `tuning_masterTune` (float, 400-480, default 440)
- [ ] `tuning_tuningMode` (choice: 12-TET/Custom/MTS-ESP)
- [ ] `tuning_octaveStretch` (float, 0.95-1.25, default 1.0)
- [ ] `tuning_pitchBendRange` (float, 1-48, default 2)
- [ ] `tuning_temperamentPreset` (choice: 11 presets)

---

## Step 4: Update PluginProcessor.h

Reference: `snippets/processor-header.h`

- [ ] `#include "TuningEngine.h"`
- [ ] `#include "ScaleGenerator.h"`
- [ ] `#include "TuningExporter.h"`
- [ ] `#include "EmbeddedTunings.h"`
- [ ] Add `TuningEngine tuningEngine;` member
- [ ] Add `ScaleGenerator scaleGenerator;` member
- [ ] Add `TuningExporter tuningExporter;` member
- [ ] (Optional) Implement `AudioProcessorValueTreeState::Listener`

---

## Step 5: Register Native Functions

Reference: `snippets/native-functions.cpp`

In your PluginEditor WebView setup, register all native functions:

**Tuning Data:**
- [ ] `getTuningIntervals`
- [ ] `setTuningIntervals`
- [ ] `getTuningName`
- [ ] `setSingleInterval`
- [ ] `setSingleIntervalEncoded`

**Tonic/Rotation:**
- [ ] `setTonicNote`
- [ ] `getTonicNote`

**Octave Stretch:**
- [ ] `getOctaveStretch`
- [ ] `setOctaveStretch`

**Master Tune:**
- [ ] `getMasterTune`
- [ ] `setMasterTune`

**Presets:**
- [ ] `setTemperamentPreset`
- [ ] `getTemperamentPreset`

**File I/O:**
- [ ] `loadScalaFile`
- [ ] `saveScalaFile`
- [ ] `loadKBMFile`
- [ ] `saveKBMFile`

**Scale Generator:**
- [ ] `generateEDO`
- [ ] `generateHarmonicSeries`
- [ ] `generateRank2`
- [ ] `applyGeneratedScale`

**Embedded Library:**
- [ ] `getEmbeddedTuningList`
- [ ] `getEmbeddedTuningCategories`
- [ ] `loadEmbeddedTuning`

**Export:**
- [ ] `exportTuningHTML`

---

## Step 6: Add State Persistence

Reference: `snippets/persistence.cpp`

- [ ] Update `getStateInformation()` to save tuning state
- [ ] Update `setStateInformation()` to restore tuning state

---

## Step 7: Copy JavaScript Module

Copy `js/tuning-panel.js` to your Resources folder:

- [ ] Copy to `Resources/ui/js/tuning-panel.js`
- [ ] Add to BinaryData in CMakeLists.txt

---

## Step 8: Add CSS Styles

Reference: `snippets/tuning-panel.css`

- [ ] Copy CSS to your plugin's `<style>` block or separate file
- [ ] Adjust CSS variables to match your plugin's aesthetic

---

## Step 9: Add HTML Container

In your index.html, add:

```html
<div id="tuning-container"></div>

<script type="module">
    import { TuningPanel } from './js/tuning-panel.js';

    const panel = new TuningPanel(
        document.getElementById('tuning-container'),
        window.__JUCE__
    );

    await panel.init();
</script>
```

- [ ] Container div added
- [ ] Import and initialization added

---

## Step 10: Use in Audio Processing

In `processBlock()`, get frequencies from TuningEngine:

```cpp
double freq = tuningEngine.getFrequency(midiNoteNumber);
```

- [ ] Audio processing uses `tuningEngine.getFrequency()`

---

## Step 11: Build and Test

- [ ] Build succeeds with no errors
- [ ] Plugin loads in DAW
- [ ] Tuning panel appears in UI
- [ ] 12-TET mode works (verify A4 = 440 Hz)
- [ ] Loading .scl file changes pitch
- [ ] Built-in presets work
- [ ] Scale generator produces valid scales
- [ ] State saves/restores correctly in DAW session

---

## Troubleshooting

### "Native function not found" errors in console
- Verify all native functions are registered before WebView loads
- Check function name spelling matches exactly

### Intervals not updating
- Verify `setSingleInterval` calls `tuningEngine.setSingleInterval()`
- Check that visualization calls `refreshState()` after changes

### State not persisting
- Verify `tuningEngine` child in ValueTree is saved/restored
- Check intervals string format (comma-separated, no spaces)

### Pitch sounds wrong
- Check that `getFrequency()` is called for every note
- Verify master tune parameter is connected
- Check tonic offset (0 = C)
