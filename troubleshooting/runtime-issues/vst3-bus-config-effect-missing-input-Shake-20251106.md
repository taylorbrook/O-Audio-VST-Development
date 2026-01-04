---
plugin: Shake
date: 2025-11-06
problem_type: runtime_error
component: juce_processor
symptoms:
  - "VST3: plugin has instrument category, but no valid event input bus"
  - "VST3: No valid input bus could be found"
  - Plugin fails to load in Ableton Live
  - Ableton database caches old category/version despite moduleinfo.json being correct
root_cause: config_error
juce_version: 8.0.9
resolution_type: code_fix
severity: critical
tags: [vst3, bus-config, input-bus, effect, category, ableton, audioprocessor]
---

# Troubleshooting: VST3 Effect Plugin Missing Input Bus

## Problem
VST3 plugin configured as an effect (IS_SYNTH=FALSE, VST3_CATEGORIES Fx Generator) fails to load in Ableton Live with error "plugin has instrument category, but no valid event input bus." The plugin's PluginProcessor constructor only configured an output bus (generator/instrument configuration) despite CMakeLists.txt declaring it as an effect.

## Environment
- JUCE Version: 8.0.9
- Affected: PluginProcessor.cpp constructor, BusesProperties configuration
- Date: 2025-11-06
- Plugin: Shake v1.1.2
- DAW: Ableton Live 12.2.6

## Symptoms
- Ableton Log shows:
  ```
  VST3: plugin processor successfully loaded: LexAudio 'Shake' v1.0.0
  error: Vst3: plugin has instrument category, but no valid event input bus
  error: VST3: No valid input bus could be found
  error: VST3: Failed: Shake
  ```
- Plugin loads successfully in AU format
- VST3 bundle's moduleinfo.json shows correct categories (Fx, Generator)
- Ableton plugin database (`~/Library/Application Support/Ableton/Live Database/Live-plugins-1.db`) caches old category/version
- Clearing Ableton cache and restarting doesn't fix the issue

## What Didn't Work

**Attempted Solution 1:** Changed VST3_CATEGORIES in CMakeLists.txt from "Instrument Synth" to "Fx Generator"
- **Why it failed:** This only updates the moduleinfo.json metadata. It doesn't change the actual bus configuration in the code, which is what VST3 hosts validate.

**Attempted Solution 2:** Changed IS_SYNTH from TRUE to FALSE and AU_MAIN_TYPE from kAudioUnitType_MusicDevice to kAudioUnitType_Effect
- **Why it failed:** Same as above - these flags control metadata and VST/AU category hints, but don't affect the actual audio bus configuration.

**Attempted Solution 3:** Manually edited moduleinfo.json to change categories
- **Why it failed:** moduleinfo.json is generated during build from the binary's embedded metadata. Manual edits are overwritten on next build.

**Attempted Solution 4:** Cleared Ableton's plugin cache (`~/Library/Caches/Ableton/`) multiple times
- **Why it failed:** Cleared the wrong cache. Ableton stores plugin metadata in a SQLite database at `~/Library/Application Support/Ableton/Live Database/Live-plugins-1.db`, not in the Caches folder.

**Attempted Solution 5:** Deleted Ableton's plugin database entries for Shake
- **Why it failed:** Ableton rescanned the plugin but still found the same bus configuration mismatch because the code hadn't changed.

## Solution

The actual problem was in `PluginProcessor.cpp` - the BusesProperties only configured an output bus (generator configuration) but CMakeLists.txt declared it as an effect (which requires both input and output).

**Code changes:**
```cpp
// PluginProcessor.cpp

// Before (broken - generator/instrument configuration):
ShakeAudioProcessor::ShakeAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // ...
}

// After (fixed - effect configuration):
ShakeAudioProcessor::ShakeAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // ...
}
```

**Rebuild steps:**
```bash
# Clean rebuild to ensure CMake cache is cleared
rm -rf build/plugins/Shake
cmake -B build -DCMAKE_BUILD_TYPE=Release
./scripts/build-and-install.sh Shake

# Clear Ableton's plugin database cache (SQLite)
sqlite3 ~/Library/Application\ Support/Ableton/Live\ Database/Live-plugins-1.db \
  "DELETE FROM plugins WHERE name='Shake' AND vendor='LexAudio';
   DELETE FROM plugin_modules WHERE path LIKE '%Shake.vst3%';
   DELETE FROM plugin_domains WHERE id LIKE '%Shake%';"

# Restart Ableton Live - it will rescan the plugin
```

## Why This Works

**VST3 host validation requirements:**
- **Generators/Instruments** (IS_SYNTH=TRUE): Must have MIDI event input bus OR audio output bus. No audio input required (they create audio from scratch).
- **Effects** (IS_SYNTH=FALSE): Must have audio input bus to receive incoming audio, plus audio output bus.

**The mismatch:**
- CMakeLists.txt declared: `IS_SYNTH FALSE` + `VST3_CATEGORIES Fx Generator` → "This is an effect"
- PluginProcessor.cpp configured: `.withOutput()` only → "This is a generator"

**VST3 hosts validate bus configuration at load time:**
1. Host reads moduleinfo.json: "This plugin is an effect"
2. Host checks bus layout: "Only has output bus, no input bus"
3. Host rejects: "Effect category requires input bus - FAILED"

**Why moduleinfo.json edits didn't work:**
The moduleinfo.json file is generated by JUCE's VST3 wrapper during build. It reads metadata from:
- CMakeLists.txt flags (IS_SYNTH, VST3_CATEGORIES)
- The binary's compiled BusesProperties configuration

Even if you manually edit moduleinfo.json, the VST3 host still validates the actual bus configuration by querying the binary's `IAudioProcessor::getBusCount()` interface.

**Ableton's cache layer:**
Ableton maintains a SQLite database (`Live-plugins-1.db`) with plugin metadata to speed up loading. This cache includes:
- Plugin version
- Categories
- Bus configuration
- Fingerprint

Clearing `~/Library/Caches/Ableton/` doesn't touch this database. You must delete the specific plugin entries from the SQLite database or delete the entire `Live-plugins-1.db` file (Ableton will rebuild it on next launch).

## Prevention

**When creating a new JUCE plugin:**

1. **Decide the plugin category FIRST:**
   - **Generator/Instrument**: Creates audio from scratch (synth, drum machine, noise generator)
   - **Effect**: Processes incoming audio (delay, reverb, compressor, EQ)

2. **Set CMakeLists.txt flags to match:**
   ```cmake
   # For generators/instruments:
   juce_add_plugin(MyPlugin
       IS_SYNTH TRUE
       VST3_CATEGORIES Instrument Synth
       AU_MAIN_TYPE kAudioUnitType_MusicDevice
   )

   # For effects:
   juce_add_plugin(MyPlugin
       IS_SYNTH FALSE
       VST3_CATEGORIES Fx
       AU_MAIN_TYPE kAudioUnitType_Effect
   )
   ```

3. **Set PluginProcessor.cpp BusesProperties to match:**
   ```cpp
   // For generators/instruments (output only):
   AudioProcessor(BusesProperties()
       .withOutput("Output", juce::AudioChannelSet::stereo(), true))

   // For effects (input + output):
   AudioProcessor(BusesProperties()
       .withInput("Input", juce::AudioChannelSet::stereo(), true)
       .withOutput("Output", juce::AudioChannelSet::stereo(), true))
   ```

4. **Verify consistency:**
   - CMakeLists.txt IS_SYNTH flag must match BusesProperties bus configuration
   - IS_SYNTH=TRUE → output only
   - IS_SYNTH=FALSE → input + output

5. **Test in multiple DAWs:**
   - Ableton Live (strict VST3 validation)
   - Logic Pro (AU validation)
   - Standalone mode (basic functionality)

6. **When changing plugin category mid-development:**
   - Update CMakeLists.txt flags
   - Update PluginProcessor.cpp BusesProperties
   - Clean rebuild: `rm -rf build/plugins/[PluginName]`
   - Clear DAW caches (especially Ableton's SQLite database)
   - Increment plugin version to force DAW rescan

## Related Issues
- See also: [synth-no-audio-missing-is-synth-LushPad-20251113.md](synth-no-audio-missing-is-synth-LushPad-20251113.md) - Inverse problem (synth missing IS_SYNTH flag)
- See also: dsp-filters-not-responding-to-parameters.md - Another Shake plugin issue (different root cause)

**Note:** The Shake plugin is a rhythm pattern generator that creates audio from scratch (no input needed), so it should logically be IS_SYNTH=TRUE with output-only bus config. However, because it was changed to IS_SYNTH=FALSE to fix AU/VST3 category display, the bus configuration needed to match by adding an input bus (even though that input is never used by the DSP code).
