---
plugin: JUCE
date: 2025-11-07
problem_type: state_management
component: juce_processor
symptoms:
  - Plugin parameters reset to defaults when closing and reopening Ableton
  - getStateInformation() called successfully but setStateInformation() not called on reload
  - DAW preset browser not visible despite getNumPrograms() returning 1
root_cause: state_sync
juce_version: 8.0.9
resolution_type: code_fix
severity: moderate
tags: [preset, state, ableton, getNumPrograms, setCurrentProgram, apvts, daw-preset]
---

# Troubleshooting: Ableton Preset System Interferes with State Restoration

## Problem
Plugin parameters reset to default values when closing and reopening Ableton Live, even though getStateInformation() is being called and the state appears to be saved. The issue occurs in both VST3 and AU formats.

## Environment
- JUCE Version: 8.0.9
- Affected: AudioProcessor preset methods (getNumPrograms, setCurrentProgram), all development stages
- DAW: Ableton Live (macOS)
- Date: 2025-11-07

## Symptoms
- Plugin parameters reset to defaults when closing and reopening Ableton project
- Right-click → "Save as Default Configuration" in Ableton doesn't persist values
- Debug logging shows getStateInformation() IS being called (state save works)
- Debug logging shows setStateInformation() is NEVER called on plugin reload
- Plugin works fine when using Ableton's built-in preset save feature (Device → Save Preset)
- No preset browser visible in Ableton despite declaring presets exist

## What Didn't Work

**Attempted Solution 1:** Verified getStateInformation()/setStateInformation() implementation
- **Why it failed:** The implementation was correct per official JUCE tutorial. The issue wasn't in the save/load code itself.

**Attempted Solution 2:** Added updateHostDisplay() calls to notify Ableton of state changes
- **Why it failed:** This addresses a different Ableton issue (forcing getStateInformation() to be called when saving). Our logs showed getStateInformation() WAS being called, so the issue was on the restore side.

**Attempted Solution 3:** Fixed VST3 category mismatch (changed from "Generator" to "Fx")
- **Why it failed:** This fixed plugin loading issues but didn't address state restoration. The category fix was needed but solved a different problem.

**Attempted Solution 4:** Investigated JUCE's VST3/AU preset implementation
- **Why it failed:** Research showed JUCE's preset file generation (.vstpreset/.aupreset) is incomplete/broken in some DAWs, but this doesn't explain why basic state save/restore wasn't working.

## Solution

The root cause was Ableton calling setCurrentProgram(0) immediately after setStateInformation(), which wipes the restored state.

**Two-part fix:**

### Part 1: Stop Ableton from calling setCurrentProgram()

Tell Ableton the plugin has no preset system by returning 0 from getNumPrograms():

```cpp
// PluginProcessor.h

// Before (broken):
int getNumPrograms() override { return 1; }

// After (fixed):
int getNumPrograms() override { return 0; }
```

**Why this works:**
- When getNumPrograms() returns 1, Ableton thinks: "This plugin has 1 preset (index 0)"
- After loading state, Ableton helpfully calls setCurrentProgram(0) to "restore" to the first preset
- Since setCurrentProgram() isn't implemented (stub does nothing), Ableton doesn't change values
- BUT if setCurrentProgram() were implemented, it would wipe the just-restored state
- Returning 0 tells Ableton: "No preset system exists, don't touch state after loading"

### Part 2: Implement custom preset manager (optional but recommended)

Since DAW preset systems are unreliable, implement file-based presets:

```cpp
// PresetManager.h
class PresetManager
{
public:
    PresetManager(juce::AudioProcessorValueTreeState& apvts);

    void savePreset(const juce::String& presetName);
    void loadPreset(const juce::String& presetName);
    void deletePreset(const juce::String& presetName);
    juce::StringArray getAllPresets() const;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::File getPresetDirectory() const;
};
```

```cpp
// PresetManager.cpp
void PresetManager::savePreset(const juce::String& presetName)
{
    auto presetFile = getPresetDirectory().getChildFile(presetName + ".xml");
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->writeTo(presetFile);
}

void PresetManager::loadPreset(const juce::String& presetName)
{
    auto presetFile = getPresetDirectory().getChildFile(presetName + ".xml");
    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(presetFile));

    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::File PresetManager::getPresetDirectory() const
{
    auto userAppData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return userAppData.getChildFile("LexPlugins").getChildFile("PluginName").getChildFile("Presets");
}
```

Add UI controls (ComboBox + Save/Delete buttons) in PluginEditor to expose preset functionality to users.

## Why This Works

**The Ableton preset interference bug:**

When a plugin declares it has presets (getNumPrograms() > 0), Ableton's loading sequence is:
1. Call setStateInformation(data) - restores all parameter values ✓
2. Call setCurrentProgram(0) - "restore" to first preset ✗

The second call wipes the values that were just restored in step 1.

This is a known Ableton behavior documented in JUCE forums since 2016. Ableton assumes plugins with presets want to be restored to a preset index, not raw parameter state.

**Why getNumPrograms() = 0 fixes it:**

When getNumPrograms() returns 0, Ableton knows there's no preset system and skips step 2 entirely. State restoration works as expected.

**Why custom preset manager is better:**

DAW preset interfaces are:
- Inconsistent across DAWs (Ableton, Logic, Reaper all behave differently)
- JUCE's VST3 preset implementation is broken/incomplete in some DAWs
- Often invisible to users (Ableton doesn't show AU preset browsers for many plugins)

A custom file-based preset system:
- Works identically in all DAWs
- Gives you full control over UI/UX
- Stores presets in a consistent location
- Survives DAW updates and cache clears

## Prevention

**For all future JUCE plugins:**

1. **Always return 0 from getNumPrograms() unless implementing full preset system:**
   ```cpp
   int getNumPrograms() override { return 0; }
   int getCurrentProgram() override { return 0; }
   void setCurrentProgram(int) override {}
   const juce::String getProgramName(int) override { return {}; }
   void changeProgramName(int, const juce::String&) override {}
   ```

2. **If you want presets, implement custom preset manager instead of DAW presets:**
   - Save/load APVTS state to XML files
   - Store in user application data directory
   - Add UI controls (ComboBox, Save/Delete buttons)
   - Cross-DAW compatibility guaranteed

3. **Test state persistence early in development:**
   - Create a test project in Ableton
   - Change parameter values
   - Save project
   - Close Ableton completely (Cmd+Q)
   - Reopen project
   - Verify parameters restored correctly

4. **Add debug logging during development:**
   ```cpp
   void getStateInformation(juce::MemoryBlock& destData) override
   {
       juce::File("/tmp/plugin_state.log").appendText("getStateInformation called\n");
       // ... actual save code
   }

   void setStateInformation(const void* data, int sizeInBytes) override
   {
       juce::File("/tmp/plugin_state.log").appendText("setStateInformation called\n");
       // ... actual load code
   }
   ```
   This reveals when Ableton is/isn't calling these methods.

5. **Never assume DAW preset systems work consistently:**
   - Ableton: Interferes with state restoration if getNumPrograms() > 0
   - Cubase: Calls wrong methods for VST3 presets
   - Reaper: Works mostly correctly but limited UI
   - Logic: AU presets work but VST3 doesn't show browser

## Related Issues

- See also: vst3-bus-config-effect-missing-input.md - Different Ableton loading issue related to bus configuration
