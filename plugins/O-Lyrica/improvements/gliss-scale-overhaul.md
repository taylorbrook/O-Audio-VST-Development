# Glissando Scale Overhaul — O-Lyrica v1.30.0

## Summary

Overhaul the scale-locked glissando scale system to support:
1. Independent tonic selection (separate from tuning tab)
2. Custom scale degree toggle buttons (replace chromatic passthrough)
3. Proper handling of non-12-note tuning systems

## Current Problems

- **Tonic**: Uses `midiNoteNumber % 12` — the played note is always the root. No independent tonic.
- **Custom mode**: Just passes all chromatic frequencies through — no degree selection.
- **Non-12-note scales**: Filtering code uses hardcoded `% 12` semitone patterns — breaks for 19-EDO, 31-EDO, Bohlen-Pierce, etc.

## User Decisions

- **Tonic**: Independent gliss tonic selector (not tied to tuning tab tonic)
- **Non-12 scales**: Disable Major/Minor/Pentatonic when non-12 tuning is active, force Custom only
- **Custom UI**: Toggle buttons per degree (adapts to current scale size)

## Implementation Plan

### 1. New APVTS Parameter: `glissandoTonic`

**File: `PluginProcessor.cpp` — `createParameterLayout()`** (after line 182, near other glissando params)

```cpp
layout.add(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { "glissandoTonic", 1 },
    "Glissando Tonic",
    juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" },
    0  // Default: C
));
```

### 2. Custom Degree Bitmask Storage

**File: `PluginProcessor.h`** — Add member:
```cpp
// Glissando custom degree bitmask (bit N = degree N enabled)
// Default: all bits set (all degrees enabled)
std::atomic<uint64_t> glissCustomDegrees { 0xFFFFFFFFFFFFFFFF };
```

**File: `PluginProcessor.cpp`** — Save/load in custom state callbacks:
- Save: `obj->setProperty("glissCustomDegrees", static_cast<int64_t>(glissCustomDegrees.load()));`
- Load: `glissCustomDegrees.store(static_cast<uint64_t>(static_cast<int64_t>(obj->getProperty("glissCustomDegrees"))));`

Add public accessor:
```cpp
uint64_t getGlissCustomDegrees() const { return glissCustomDegrees.load(std::memory_order_acquire); }
void setGlissCustomDegrees(uint64_t mask) { glissCustomDegrees.store(mask, std::memory_order_release); }
```

### 3. Generalized Scale Filtering (HarpSynthVoice.cpp)

**Replace lines 205-238** with generalized logic:

```cpp
int scaleIndex = static_cast<int>(parameters->getRawParameterValue("glissandoScale")->load());
int glissTonic = static_cast<int>(parameters->getRawParameterValue("glissandoTonic")->load());
int numScaleDegrees = tuningEngine->getScaleDegrees(); // 12 for 12-TET, 19 for 19-EDO, etc.

// For non-12-note tunings, force Custom mode
if (numScaleDegrees != 12 && scaleIndex < 3)
    scaleIndex = 3; // Force Custom

if (scaleIndex < 3) // Major=0, Minor=1, Pentatonic=2 (only for 12-note scales)
{
    static const std::vector<std::vector<int>> scalePatterns = {
        {0, 2, 4, 5, 7, 9, 11},   // Major
        {0, 2, 3, 5, 7, 8, 10},    // Minor (natural)
        {0, 2, 4, 7, 9}             // Pentatonic
    };

    // Use gliss tonic instead of played note
    int anchorNote = 60 + glissTonic;
    std::vector<double> filtered;

    for (int n = scaleStart; n < scaleStart + scaleCount && n <= 127; ++n)
    {
        if (n < 0) continue;
        int degreeInScale = ((n - anchorNote) % 12 + 12) % 12;
        bool inScale = false;
        for (int d : scalePatterns[scaleIndex])
        {
            if (d == degreeInScale) { inScale = true; break; }
        }
        if (inScale)
            filtered.push_back(tuningEngine->getFrequency(n));
    }

    if (filtered.size() >= 2)
        glissandoController.setScale(filtered);
    else
        glissandoController.setScale(scaleFreqs);
}
else // Custom mode — use bitmask for any scale size
{
    // Get bitmask from processor (need to pass through or access via APVTS parent)
    // Voice needs access to the bitmask — add getter to processor or pass via atomic pointer
    uint64_t degreeMask = customDegreeMaskPtr ? customDegreeMaskPtr->load(std::memory_order_acquire)
                                               : 0xFFFFFFFFFFFFFFFF;

    int anchorNote = 60 + glissTonic;
    std::vector<double> filtered;

    for (int n = scaleStart; n < scaleStart + scaleCount && n <= 127; ++n)
    {
        if (n < 0) continue;
        int degreeInScale = ((n - anchorNote) % numScaleDegrees + numScaleDegrees) % numScaleDegrees;
        if ((degreeMask >> degreeInScale) & 1)
            filtered.push_back(tuningEngine->getFrequency(n));
    }

    if (filtered.size() >= 2)
        glissandoController.setScale(filtered);
    else
        glissandoController.setScale(scaleFreqs);
}
```

### 4. Voice Access to Bitmask

**File: `HarpSynthVoice.h`** — Add member:
```cpp
std::atomic<uint64_t>* customDegreeMaskPtr = nullptr;
```

Add setter:
```cpp
void setCustomDegreeMask(std::atomic<uint64_t>* ptr);
```

**File: `PluginProcessor.cpp`** — Wire in constructor (line ~398):
```cpp
voice->setCustomDegreeMask(&glissCustomDegrees);
```

### 5. PluginEditor Changes

**File: `PluginEditor.h`** — Add:
```cpp
std::unique_ptr<juce::WebComboBoxRelay> glissandoTonicRelay;
std::unique_ptr<juce::WebComboBoxParameterAttachment> glissandoTonicAttachment;
```

**File: `PluginEditor.cpp`** — Add:
- Create relay: `glissandoTonicRelay = std::make_unique<juce::WebComboBoxRelay>("glissandoTonic");`
- Wire to WebView: `.withOptionsFrom(*glissandoTonicRelay)`
- Create attachment: `glissandoTonicAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(*apvts.getParameter("glissandoTonic"), *glissandoTonicRelay, nullptr);`

**Native functions to add:**
```cpp
.withNativeFunction("setGlissCustomDegrees", [this](const auto& args, auto complete) {
    if (args.size() < 2) { complete(juce::var(false)); return; }
    uint32_t low = static_cast<uint32_t>(static_cast<int>(args[0]));
    uint32_t high = static_cast<uint32_t>(static_cast<int>(args[1]));
    uint64_t mask = (static_cast<uint64_t>(high) << 32) | low;
    processorRef.setGlissCustomDegrees(mask);
    complete(juce::var(true));
})
.withNativeFunction("getGlissCustomDegrees", [this](const auto& args, auto complete) {
    juce::ignoreUnused(args);
    uint64_t mask = processorRef.getGlissCustomDegrees();
    auto* obj = new juce::DynamicObject();
    obj->setProperty("low", static_cast<int>(mask & 0xFFFFFFFF));
    obj->setProperty("high", static_cast<int>((mask >> 32) & 0xFFFFFFFF));
    complete(juce::var(obj));
})
.withNativeFunction("getScaleDegreeCount", [this](const auto& args, auto complete) {
    juce::ignoreUnused(args);
    complete(juce::var(processorRef.getTuningEngine()->getScaleDegrees()));
})
```

### 6. WebView UI Changes

In the Scale-Locked glissando section HTML:
- Add `<juce-combobox id="glissandoTonic">` dropdown labeled "Key"
- Add a `<div id="customDegreeToggles">` container
- JavaScript: on scale type change, show/hide degree toggles (visible only when Custom selected)
- JavaScript: on tuning change, update number of toggle buttons and disable Major/Minor/Pentatonic if non-12
- Each toggle button calls `setGlissCustomDegrees()` with updated bitmask
- On load, call `getGlissCustomDegrees()` and `getScaleDegreeCount()` to initialize toggle state

### 7. Timer Callback Additions (PluginEditor.cpp)

Sync scale degree count to WebView periodically (for when tuning changes):
```cpp
static int lastScaleDegreeCount = -1;
int currentDegreeCount = processorRef.getTuningEngine()->getScaleDegrees();
if (currentDegreeCount != lastScaleDegreeCount)
{
    lastScaleDegreeCount = currentDegreeCount;
    webComponent->emitEventIfBrowserIsVisible("scaleDegreeCountChanged",
        juce::var(currentDegreeCount));
}
```

## Files Modified

| File | Changes |
|------|---------|
| `PluginProcessor.h` | Add `glissCustomDegrees` atomic, getter/setter |
| `PluginProcessor.cpp` | New parameter, state save/load, wire voice pointer |
| `HarpSynthVoice.h` | Add `customDegreeMaskPtr` member and setter |
| `HarpSynthVoice.cpp` | Rewrite scale filtering (lines 205-238) |
| `PluginEditor.h` | Add tonic relay/attachment declarations |
| `PluginEditor.cpp` | Relay, attachment, native functions, WebView HTML, timer sync |

## Version

MINOR bump: 1.29.2 → 1.30.0
