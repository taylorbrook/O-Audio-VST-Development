---
plugin: LushPad
date: 2025-11-13
problem_type: runtime_error
component: cmake
symptoms:
  - "No sound output when playing MIDI notes in DAW"
  - "Synth plugin loads successfully but produces silence"
  - "Plugin shows in DAW effects list instead of instruments"
root_cause: config_error
juce_version: 8.0.9
resolution_type: config_change
severity: critical
tags: [synth, midi, is-synth, cmake, instrument, no-audio, bus-config]
---

# Troubleshooting: Synth Plugin Produces No Audio - Missing IS_SYNTH Flag

## Problem
Synth/instrument plugin (LushPad) loaded successfully in DAW but produced no audio output when playing MIDI notes. The plugin appeared in the effects category instead of instruments, and DAWs did not route MIDI to it.

## Environment
- Plugin: LushPad v1.0.0 (Stage 5)
- JUCE Version: 8.0.9
- Affected: CMakeLists.txt configuration, MIDI routing
- Date: 2025-11-13
- DAW: User's DAW (symptom applies to all DAWs)

## Symptoms
- Plugin loads without errors
- No audio output when playing MIDI notes
- Plugin shows in effects category, not instruments
- PluginProcessor has correct synth implementation:
  - `acceptsMidi()` returns `true`
  - MIDI handling in `processBlock()`
  - Voice allocation and synthesis code present
  - Output-only bus configuration (correct for synth)

## What Didn't Work

**Attempted Solution 1:** Added `isSynth()` override to PluginProcessor.h returning `true`
- **Why it failed:** `isSynth()` is not a virtual method in JUCE's AudioProcessor base class. Compiler error: "only virtual member functions can be marked 'override'". This method doesn't exist in the base class.

**Direct investigation:** Checked CMakeLists.txt and found missing `IS_SYNTH TRUE` flag.

## Solution

The problem was in CMakeLists.txt - the plugin was not declared as a synth, so JUCE didn't generate the proper VST3/AU metadata for instrument plugins, and DAWs didn't route MIDI to it.

**Configuration changes:**
```cmake
# CMakeLists.txt

# Before (broken - defaults to effect plugin):
juce_add_plugin(LushPad
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lush
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "LushPad"
    NEEDS_WEB_BROWSER TRUE
)

# After (fixed - declared as synth):
juce_add_plugin(LushPad
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lush
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "LushPad"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_WEB_BROWSER TRUE
)
```

**Rebuild steps:**
```bash
# Rebuild and reinstall
./scripts/build-and-install.sh LushPad

# The build system automatically:
# - Removes old VST3/AU versions
# - Installs new versions
# - Clears DAW caches
```

**DAW restart:** Restart DAW and rescan plugins. LushPad now appears under instruments category.

## Why This Works

**JUCE plugin flags and metadata generation:**

When `IS_SYNTH TRUE` is set in CMakeLists.txt, JUCE generates compile-time defines that control plugin behavior:

1. **VST3 metadata:**
   - Sets `JucePlugin_IsSynth=1` (preprocessor define)
   - Sets `Vst3Category="Instrument|Synth"` in moduleinfo.json
   - VST3 host reads category and routes MIDI accordingly

2. **AU metadata:**
   - Sets `AUMainType='aumu'` (Audio Unit Music Device)
   - macOS AU host recognizes plugin as instrument
   - MIDI events routed to plugin

3. **DAW behavior:**
   - DAWs check plugin category metadata
   - Instruments appear in instrument browser
   - MIDI routing enabled automatically
   - Effects appear in effects browser, no MIDI routing

**Why the code alone wasn't enough:**

The PluginProcessor.cpp had correct synth implementation:
- Output-only bus (`.withOutput()` only - correct for synth)
- `acceptsMidi()` returns `true`
- MIDI note handling in `processBlock()`
- Voice allocation and synthesis DSP

BUT without `IS_SYNTH TRUE` in CMakeLists.txt:
- JUCE generates effect metadata (not instrument)
- VST3/AU hosts see effect category
- DAWs don't route MIDI to effects
- Result: Synth receives no MIDI, produces silence

**The missing flags:**

1. `IS_SYNTH TRUE` - Primary flag that controls plugin category
2. `NEEDS_MIDI_INPUT TRUE` - Explicit declaration of MIDI requirement (optional but recommended for clarity)

## Prevention

**When creating a synth/instrument plugin:**

1. **Set CMakeLists.txt flags BEFORE Stage 2 (Foundation):**
   ```cmake
   juce_add_plugin(MyPlugin
       IS_SYNTH TRUE           # Declares this as instrument plugin
       NEEDS_MIDI_INPUT TRUE   # Explicit MIDI requirement
       VST3_CATEGORIES Instrument Synth  # Optional: explicit VST3 category
       AU_MAIN_TYPE kAudioUnitType_MusicDevice  # Optional: explicit AU type
   )
   ```

2. **Match BusesProperties configuration:**
   ```cpp
   // Synth/instrument: output-only bus (generates audio from scratch)
   AudioProcessor(BusesProperties()
       .withOutput("Output", juce::AudioChannelSet::stereo(), true))
   ```

3. **Verify during planning (Stage 0):**
   - Review architecture.md and plan.md
   - Confirm "Synth" or "Instrument" plugin type
   - Ensure foundation-shell-agent generates correct CMakeLists.txt

4. **Test early (Stage 3 or 4):**
   - Load plugin in DAW
   - Verify it appears under instruments
   - Play MIDI notes and confirm audio output
   - Don't wait until Stage 5 to discover MIDI routing issues

5. **Plugin category decision tree:**
   ```
   Does plugin CREATE audio from scratch (no audio input needed)?
   ├─ YES → IS_SYNTH TRUE + output-only bus
   │         Examples: Synth, drum machine, noise generator
   └─ NO  → IS_SYNTH FALSE + input+output bus
             Examples: Delay, reverb, compressor, EQ
   ```

## Related Issues

- See also: [vst3-bus-config-effect-missing-input-Shake-20251106.md](vst3-bus-config-effect-missing-input-Shake-20251106.md) - Inverse problem (effect missing input bus)
- Pattern: CMakeLists.txt flags must match PluginProcessor bus configuration for consistent VST3/AU behavior

## Added to Required Reading

✅ **Promoted to Critical Pattern #22** in `troubleshooting/patterns/juce8-critical-patterns.md`

This pattern is now mandatory reading for all subagents before Stage 2 (Foundation) to prevent repeat occurrences.
