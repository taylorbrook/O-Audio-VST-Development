---
plugin: MicroGlitch
date: 2025-11-06
problem_type: state_management
component: juce_audio_utils
symptoms:
  - Waveform display disappears when plugin window is closed and reopened
  - AudioThumbnail shows blank/empty on editor reconstruction
  - Sample audio continues to play but visual waveform is missing
  - Problem occurs across DAW project save/reload cycles
root_cause: state_sync
juce_version: 8.0.9
resolution_type: code_fix
severity: moderate
tags: [audiothumbnail, waveform, state, persistence, getstateinformation, setstateinformation]
---

# Troubleshooting: AudioThumbnail Waveform Not Persisting

## Problem
Loaded sample waveform disappeared when closing and reopening the plugin window. The sample buffer remained loaded (audio continued to process correctly), but the AudioThumbnail visual display showed blank until a new sample was dragged in.

## Environment
- JUCE Version: 8.0.9
- Affected: PluginProcessor state management, PluginEditor AudioThumbnail display
- Plugin: MicroGlitch v1.1.0 → v1.1.1
- Date: 2025-11-06

## Symptoms
- Waveform displays correctly after initial drag-and-drop
- Closing plugin window and reopening shows blank waveform
- Sample audio continues to play (buffer is still loaded)
- AudioThumbnail doesn't reload on editor reconstruction
- Problem persists across DAW project save/reload
- Plugin instance duplication also loses waveform

## What Didn't Work

**Direct solution:** The problem was identified and fixed on the first attempt after analyzing the state management flow.

## Solution

The loaded sample file path was not being saved to plugin state, so the AudioThumbnail couldn't reload when the editor was reconstructed.

**Code changes:**

**1. Add file path tracking to PluginProcessor.h:**
```cpp
// Added member to track loaded sample path
public:
    juce::AudioBuffer<float> loadedSample;
    juce::AudioFormatManager formatManager;
    juce::String loadedSamplePath;  // NEW: Track the file path
```

**2. Save path to state in PluginProcessor.cpp:**
```cpp
// Before (broken):
void MicroGlitchAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

// After (fixed):
void MicroGlitchAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Add loaded sample path to state
    if (loadedSamplePath.isNotEmpty())
        state.setProperty("samplePath", loadedSamplePath, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
```

**3. Reload sample from saved path in PluginProcessor.cpp:**
```cpp
// Before (broken):
void MicroGlitchAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// After (fixed):
void MicroGlitchAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xmlState);
        apvts.replaceState(state);

        // Reload sample if path exists
        if (state.hasProperty("samplePath"))
        {
            juce::String path = state.getProperty("samplePath").toString();
            juce::File sampleFile(path);

            if (sampleFile.existsAsFile())
            {
                auto* reader = formatManager.createReaderFor(sampleFile);
                if (reader != nullptr)
                {
                    auto sampleRate = reader->sampleRate;
                    auto numChannels = static_cast<int>(reader->numChannels);
                    auto lengthInSamples = static_cast<int>(reader->lengthInSamples);

                    juce::AudioBuffer<float> tempBuffer(numChannels, lengthInSamples);
                    reader->read(&tempBuffer, 0, lengthInSamples, 0, true, true);

                    loadedSample = tempBuffer;
                    loadedSamplePath = path;

                    delete reader;
                }
            }
        }
    }
}
```

**4. Store path when loading in PluginEditor.cpp:**
```cpp
void MicroGlitchAudioProcessorEditor::loadSampleFile(const juce::File& file)
{
    // ... existing code to load sample buffer ...

    // Copy to processor's loaded sample buffer (thread-safe)
    audioProcessor.loadedSample = tempBuffer;
    audioProcessor.loadedSamplePath = file.getFullPathName();  // NEW: Store the path

    // ... rest of code ...
}
```

**5. Reload thumbnail in PluginEditor.cpp constructor:**
```cpp
MicroGlitchAudioProcessorEditor::MicroGlitchAudioProcessorEditor(MicroGlitchAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // ... existing initialization code ...

    // NEW: Reload thumbnail if sample was previously loaded
    if (audioProcessor.loadedSamplePath.isNotEmpty())
    {
        juce::File sampleFile(audioProcessor.loadedSamplePath);
        if (sampleFile.existsAsFile())
            thumbnail.setSource(new juce::FileInputSource(sampleFile));
    }

    // ... rest of constructor ...
}
```

## Why This Works

**Root cause:** The AudioProcessor and AudioProcessorEditor have different lifecycles:
- **AudioProcessor:** Persists for the lifetime of the plugin instance (survives window close/reopen)
- **AudioProcessorEditor:** Destroyed when window closes, recreated when reopened

The original implementation:
1. Loaded sample buffer into processor ✅ (persists)
2. Loaded waveform into editor's AudioThumbnail ✅ (displays)
3. Did NOT save file path to processor state ❌
4. Did NOT reload thumbnail on editor reconstruction ❌

When the editor was destroyed and recreated:
- Sample buffer still existed in processor → audio worked
- AudioThumbnail was blank → no visual display
- No file path saved → couldn't reload

**The fix adds two critical pieces:**

1. **State persistence:** Save the file path to the plugin's state via `getStateInformation()` and reload in `setStateInformation()`. This ensures both the buffer AND the path persist across:
   - Window close/reopen
   - DAW project save/reload
   - Plugin instance duplication

2. **Editor reload:** Check for existing path in editor constructor and reload the AudioThumbnail from that path. This handles the window close/reopen case.

**Why file path, not the audio buffer itself?**
- Audio buffers can be large (megabytes)
- Serializing them to XML state would be slow and bloat project files
- File path is just a string (tiny)
- AudioFormatManager can reload from path quickly

## Prevention

**When using AudioThumbnail for sample/waveform display:**

1. **Always track the source file path** in your AudioProcessor (not just the buffer)
2. **Serialize the path in getStateInformation():**
   ```cpp
   state.setProperty("samplePath", loadedFilePath, nullptr);
   ```
3. **Reload from path in setStateInformation():**
   ```cpp
   if (state.hasProperty("samplePath")) {
       juce::File file(state.getProperty("samplePath").toString());
       // Reload buffer and update path
   }
   ```
4. **Check for existing path in editor constructor:**
   ```cpp
   if (processor.loadedFilePath.isNotEmpty()) {
       thumbnail.setSource(new juce::FileInputSource(juce::File(processor.loadedFilePath)));
   }
   ```

5. **Remember the lifecycle split:**
   - Processor = persistent data (buffers, paths, parameters)
   - Editor = transient UI (thumbnails, graphics, user input)
   - Editor must reload UI state from processor on construction

6. **Test across all persistence scenarios:**
   - Close/reopen plugin window (editor destroyed/recreated)
   - Save/reload DAW project (full state serialization)
   - Duplicate plugin instance (state copied to new instance)

## Related Issues

No related issues documented yet.
