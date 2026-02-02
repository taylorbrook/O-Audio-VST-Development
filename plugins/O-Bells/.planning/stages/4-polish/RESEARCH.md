# Stage 4: Polish - Research

**Date:** 2026-02-02
**Plugin:** O-Bells
**Stage:** 4 - Polish (Final)

---

## Research Summary

This document investigates implementation approaches for O-Bells Stage 4 (Polish), focusing on:
1. Folder-based preset categories for OuariconPresetManager
2. Preset bar UI integration from O-Lyrica reference
3. Factory preset parameter values for bell archetypes

---

## 1. OuariconPresetManager Analysis

### Current Implementation

**Module Location:** `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`

**Key Features:**
- Header-only implementation (all code inline)
- JSON serialization for preset files
- Factory/User directory structure
- APVTS parameter persistence
- Custom state callbacks (for non-APVTS data)
- Navigation (getNextPreset/getPreviousPreset)
- DAW session state (getStateAsXml/setStateFromXml)

**Directory Structure:**
```
~/Library/{pluginName}/Presets/
├── Factory/
│   └── *.json
└── User/
    └── *.json
```

### Category Support Gap

**Current Behavior:**
- `getPresetList()` scans Factory then User directories (flat)
- Returns alphabetically sorted StringArray
- No support for subdirectories within Factory/User

**Required for O-Bells:**
- Folder-based categories within Factory: `Factory/Orchestral/`, `Factory/Sacred/`, etc.
- UI needs category-grouped dropdown (not flat list)

### Recommended Approach: UI-Side Category Grouping

Rather than modifying the module (which would require backwards compatibility), implement category support **client-side**:

1. **Use subdirectories** in Factory folder (module ignores them by default)
2. **Add new native function** `getPresetListWithCategories()` in PluginEditor
3. **Custom scanning logic** in editor that walks subdirectories
4. **UI dropdown** with category headers (similar to O-Lyrica but grouped)

**Implementation Pattern:**
```cpp
// In PluginEditor.cpp - new native function
.withNativeFunction("getPresetListWithCategories",
    [this](const juce::Array<juce::var>&, std::function<void(juce::var)> complete) {
    auto factoryDir = processorRef.getPresetManager().getFactoryPresetsDirectory();

    auto* result = new juce::DynamicObject();

    // Scan categories (subdirectories)
    for (const auto& categoryDir : factoryDir.findChildFiles(
            juce::File::findDirectories, false)) {
        juce::Array<juce::var> categoryPresets;
        for (const auto& file : categoryDir.findChildFiles(
                juce::File::findFiles, false, "*.json")) {
            categoryPresets.add(juce::var(file.getFileNameWithoutExtension()));
        }
        result->setProperty(categoryDir.getFileName(), juce::var(categoryPresets));
    }

    complete(juce::var(result));
})
```

**Preset Loading:**
The existing `loadPreset(name)` searches Factory root, then User. For categorized presets:
- Store presets in subdirectories: `Factory/Orchestral/Tubular Bells.json`
- Create wrapper: `loadPresetFromCategory(category, name)` that builds full path

---

## 2. Preset Bar UI Integration

### O-Lyrica Reference Implementation

**Source Files:**
- `plugins/O-Lyrica/Resources/ui/index.html` (lines 1650-1660, 2668-2831)
- `plugins/O-Lyrica/Source/PluginEditor.cpp` (native functions)

**UI Structure:**
```html
<div class="preset-browser">
    <div class="preset-arrow" id="preset-prev">◀</div>
    <div class="preset-name-wrapper">
        <div class="preset-name" id="preset-name-display">Default</div>
        <div class="preset-dropdown" id="preset-dropdown"></div>
    </div>
    <div class="preset-arrow" id="preset-next">▶</div>
    <div class="preset-action-btn" id="preset-save">Save</div>
    <div class="preset-action-btn" id="preset-load">Load</div>
</div>
```

**CSS Styling (Ouaricon Naturalist):**
```css
.preset-dropdown {
    display: none;
    position: absolute;
    top: 100%;
    left: 50%;
    transform: translateX(-50%);
    min-width: 180px;
    max-height: 300px;
    overflow-y: auto;
    background: linear-gradient(to bottom, #F5E6D3, #E8D5C4);
    border: 1px solid rgba(139, 115, 85, 0.4);
    border-radius: 4px;
    box-shadow: 0 4px 12px rgba(0,0,0,0.15);
}

.preset-dropdown-header {
    padding: 4px 12px;
    font-size: 8px;
    color: #8B7355;
    text-transform: uppercase;
    letter-spacing: 1px;
    border-bottom: 1px solid rgba(139, 115, 85, 0.3);
    background: rgba(212, 196, 176, 0.5);
}

.preset-dropdown-item {
    padding: 6px 12px;
    font-size: 10px;
    color: #3C2F2F;
    cursor: pointer;
}

.preset-dropdown-item:hover {
    background: rgba(139, 168, 112, 0.3);
}
```

**Native Functions Required:**
| Function | Purpose | Signature |
|----------|---------|-----------|
| `getPresetList` | Get flat preset list | `() => string[]` |
| `getCurrentPreset` | Get current preset name | `() => string` |
| `loadPreset` | Load by name | `(name: string) => boolean` |
| `savePreset` | Save by name | `(name: string) => boolean` |
| `selectNextPreset` | Navigate forward | `() => string` |
| `selectPreviousPreset` | Navigate backward | `() => string` |
| `savePresetWithDialog` | File dialog save | `() => string?` |
| `loadPresetFromFile` | File dialog load | `() => string?` |

**Additional for O-Bells:**
| Function | Purpose | Signature |
|----------|---------|-----------|
| `getPresetListWithCategories` | Get categorized presets | `() => {[category: string]: string[]}` |
| `loadPresetFromCategory` | Load from category folder | `(category: string, name: string) => boolean` |

### Header Layout for O-Bells

Per CONTEXT.md specification:
```
┌──────────────────────────────────────────────────────────────────┐
│  O-Bells                                    [◀] [Preset ▼] [▶]  │
│                                              [Save] [Load]       │
└──────────────────────────────────────────────────────────────────┘
```

**HTML Structure:**
```html
<div class="header-bar">
    <h1 class="plugin-title">O-Bells</h1>
    <div class="preset-browser">
        <!-- Same structure as O-Lyrica -->
    </div>
</div>
```

---

## 3. Factory Preset Parameter Research

### Bell Archetype Parameters

Based on the DSP implementation (BellVoice.cpp), the key parameters for bell character are:

| Parameter | Impact | Low Value | High Value |
|-----------|--------|-----------|------------|
| `inharmonicity` | Partial ratios | Harmonic (organ-like) | Gamelan (metallic) |
| `material` | Decay/brightness | Bronze (warm, 1.0x decay) | Crystal (ethereal, 5.0x decay) |
| `malletHardness` | Attack transient | Soft felt (dark) | Hard metal (bright) |
| `strikePosition` | Partial emphasis | Center (fundamental) | Edge (upper partials) |
| `damping` | Decay length | Short (hand-damped) | Long (free-ring) |
| `brightness` | High-freq content | Dark | Brilliant |

### Preset Category Definitions

#### Orchestral (5 presets)
Target: Traditional Western orchestral bells - tubular bells, chimes, glockenspiel

| Preset | Strike | Mallet | Damping | Bright | Material | Inharm | Notes |
|--------|--------|--------|---------|--------|----------|--------|-------|
| **Tubular Bells** | 0.4 | 0.6 | 0.8 | 0.55 | 0.1 (Bronze) | 0.45 | Classic orchestral |
| **Concert Chimes** | 0.5 | 0.7 | 0.85 | 0.65 | 0.15 | 0.5 | Bright, resonant |
| **Glockenspiel** | 0.6 | 0.8 | 0.7 | 0.75 | 0.4 (Steel) | 0.35 | High, metallic |
| **Celesta Mallet** | 0.35 | 0.45 | 0.6 | 0.6 | 0.7 (Glass) | 0.25 | Soft, delicate |
| **Vibraphone** | 0.45 | 0.5 | 0.75 | 0.5 | 0.25 | 0.3 | Warm, sustained |

#### Sacred (5 presets)
Target: Church bells, meditation bowls, ceremonial instruments

| Preset | Strike | Mallet | Damping | Bright | Material | Inharm | Notes |
|--------|--------|--------|---------|--------|----------|--------|-------|
| **Church Bell** | 0.3 | 0.65 | 0.95 | 0.5 | 0.15 | 0.6 | Deep, resonant |
| **Cathedral Carillon** | 0.35 | 0.55 | 0.9 | 0.45 | 0.1 | 0.55 | Grand, layered |
| **Meditation Bowl** | 0.25 | 0.3 | 0.85 | 0.4 | 0.2 | 0.3 | Smooth, calming |
| **Temple Gong** | 0.2 | 0.5 | 0.95 | 0.35 | 0.12 | 0.65 | Dark, sustained |
| **Singing Bowl** | 0.3 | 0.25 | 0.9 | 0.5 | 0.3 | 0.25 | Clear, harmonious |

#### World (5 presets)
Target: Gamelan, ethnic percussion, global metallophones

| Preset | Strike | Mallet | Damping | Bright | Material | Inharm | Notes |
|--------|--------|--------|---------|--------|----------|--------|-------|
| **Gamelan Saron** | 0.55 | 0.6 | 0.5 | 0.6 | 0.15 | 0.85 | Indonesian key |
| **Gamelan Bonang** | 0.5 | 0.55 | 0.6 | 0.55 | 0.2 | 0.75 | Pot gong |
| **Tibetan Bowl** | 0.25 | 0.2 | 0.88 | 0.45 | 0.25 | 0.35 | Meditation |
| **Steel Pan** | 0.6 | 0.65 | 0.65 | 0.7 | 0.35 | 0.4 | Caribbean |
| **Kalimba Bell** | 0.45 | 0.4 | 0.55 | 0.65 | 0.1 | 0.2 | Thumb piano |

#### Ambient (5 presets)
Target: Evolving pads, shimmering textures, atmospheric

| Preset | Strike | Mallet | Damping | Bright | Material | Inharm | Notes |
|--------|--------|--------|---------|--------|----------|--------|-------|
| **Frozen Shimmer** | 0.6 | 0.7 | 1.0 | 0.8 | 0.85 (Crystal) | 0.4 | Ethereal |
| **Bell Pad** | 0.4 | 0.35 | 0.95 | 0.5 | 0.5 | 0.5 | Sustained blend |
| **Crystal Drone** | 0.5 | 0.45 | 1.0 | 0.7 | 0.95 | 0.35 | Pure, sustained |
| **Ethereal Chime** | 0.55 | 0.6 | 0.9 | 0.75 | 0.8 | 0.3 | Delicate, airy |
| **Submerged Bells** | 0.3 | 0.25 | 0.92 | 0.3 | 0.6 | 0.55 | Dark, underwater |

#### Cinematic (5 presets)
Target: Film scoring, dramatic, tension

| Preset | Strike | Mallet | Damping | Bright | Material | Inharm | Notes |
|--------|--------|--------|---------|--------|----------|--------|-------|
| **Epic Bell** | 0.35 | 0.7 | 0.95 | 0.6 | 0.15 | 0.55 | Big, dramatic |
| **Tension Chime** | 0.7 | 0.85 | 0.6 | 0.85 | 0.45 | 0.7 | Unsettling |
| **Horror Stinger** | 0.8 | 0.95 | 0.4 | 0.9 | 0.5 | 0.8 | Sharp, jarring |
| **Dramatic Swell** | 0.4 | 0.55 | 0.98 | 0.55 | 0.2 | 0.5 | Building tension |
| **Distant Thunder** | 0.2 | 0.4 | 1.0 | 0.25 | 0.1 | 0.65 | Deep rumble |

### Ensemble Settings for Presets

| Preset Type | Unison | Detune | Sub | Oct | Spread |
|-------------|--------|--------|-----|-----|--------|
| Solo/Clean | 1 | 0 | 0 | 0 | 0.5 |
| Warm Ensemble | 2 | 8 | 0.2 | 0 | 0.6 |
| Lush Pad | 3 | 15 | 0.3 | 0.2 | 0.8 |
| Full Layered | 4 | 20 | 0.4 | 0.35 | 0.9 |
| Cinematic Wide | 4 | 12 | 0.5 | 0.3 | 1.0 |

### Advanced Parameter Defaults

| Parameter | Default | Preset-Specific Notes |
|-----------|---------|----------------------|
| `partialTuning` | 0.0 | ±20 for detuned presets |
| `nonlinearEffects` | 0.0 | 0.2-0.4 for aggressive presets |
| `strikeNoiseChar` | 0 (Click) | Thud=1 for soft, Ping=2 for metallic |
| `decayShape` | 1 (Exp) | Linear for clean, Multi-stage for realistic |
| `velocityCurve` | 0 (Linear) | Exp for expressive, Log for consistent |
| `pitchEnvelope` | 0.0 | 0.2-0.4 for large bells |
| `pitchEnvTime` | 50 | 80-150 for large bells |
| `outputGain` | 0.0 | Adjust per preset for consistent volume |

---

## 4. Implementation Patterns

### PluginProcessor Changes

```cpp
// In PluginProcessor.h
#include "OuariconPresetManager.h"

class OBellsAudioProcessor : public juce::AudioProcessor {
public:
    OuariconPresetManager& getPresetManager() { return presetManager; }

private:
    OuariconPresetManager presetManager;
    void initializeFactoryPresets();
};
```

```cpp
// In PluginProcessor.cpp constructor
OBellsAudioProcessor::OBellsAudioProcessor()
    : AudioProcessor(...)
    , parameters(...)
    , presetManager(parameters, "O-Bells")  // Initialize with APVTS
{
}

// In prepareToPlay
void OBellsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    initializeFactoryPresets();  // Creates presets on first run
    // ... existing code
}
```

### Factory Preset Initialization

```cpp
void OBellsAudioProcessor::initializeFactoryPresets()
{
    auto factoryDir = presetManager.getFactoryPresetsDirectory();

    // Check for any existing factory presets
    if (factoryDir.isDirectory() &&
        factoryDir.getNumberOfChildFiles(juce::File::findFilesAndDirectories, "*.json") > 0)
    {
        return;  // Already initialized
    }

    // Create category subdirectories
    factoryDir.getChildFile("Orchestral").createDirectory();
    factoryDir.getChildFile("Sacred").createDirectory();
    factoryDir.getChildFile("World").createDirectory();
    factoryDir.getChildFile("Ambient").createDirectory();
    factoryDir.getChildFile("Cinematic").createDirectory();

    // Helper to write preset JSON
    auto writePreset = [&](const juce::String& category,
                           const juce::String& name,
                           const std::map<juce::String, float>& params) {
        auto* preset = new juce::DynamicObject();
        auto* paramsObj = new juce::DynamicObject();
        for (const auto& [id, value] : params) {
            paramsObj->setProperty(id, value);
        }
        preset->setProperty("parameters", juce::var(paramsObj));
        preset->setProperty("version", "1.0.0");
        preset->setProperty("plugin", "O-Bells");
        preset->setProperty("category", category);

        auto file = factoryDir.getChildFile(category)
                              .getChildFile(name + ".json");
        file.replaceWithText(juce::JSON::toString(juce::var(preset), true));
    };

    // Orchestral presets
    writePreset("Orchestral", "Tubular Bells", {
        {"strikePosition", 0.4f}, {"malletHardness", 0.6f}, {"damping", 0.8f},
        {"brightness", 0.55f}, {"material", 0.1f}, {"inharmonicity", 0.45f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f}, {"stereoSpread", 0.5f},
        // ... remaining parameters
    });
    // ... remaining presets
}
```

### PluginEditor Native Functions

```cpp
// Additional native function for categorized listing
.withNativeFunction("getPresetListWithCategories",
    [this](const juce::Array<juce::var>&, std::function<void(juce::var)> complete) {
    auto factoryDir = processorRef.getPresetManager().getFactoryPresetsDirectory();
    auto* result = new juce::DynamicObject();

    // Scan category subdirectories
    auto categories = juce::StringArray{"Orchestral", "Sacred", "World", "Ambient", "Cinematic"};
    for (const auto& categoryName : categories) {
        auto categoryDir = factoryDir.getChildFile(categoryName);
        if (categoryDir.isDirectory()) {
            juce::Array<juce::var> presets;
            for (const auto& file : categoryDir.findChildFiles(
                    juce::File::findFiles, false, "*.json")) {
                presets.add(juce::var(file.getFileNameWithoutExtension()));
            }
            result->setProperty(categoryName, juce::var(presets));
        }
    }

    // Also add User presets
    auto userDir = processorRef.getPresetManager().getUserPresetsDirectory();
    if (userDir.isDirectory()) {
        juce::Array<juce::var> userPresets;
        for (const auto& file : userDir.findChildFiles(
                juce::File::findFiles, false, "*.json")) {
            userPresets.add(juce::var(file.getFileNameWithoutExtension()));
        }
        result->setProperty("User", juce::var(userPresets));
    }

    complete(juce::var(result));
})

// Load preset from specific category folder
.withNativeFunction("loadPresetFromCategory",
    [this](const juce::Array<juce::var>& args, std::function<void(juce::var)> complete) {
    if (args.size() < 2) { complete(juce::var(false)); return; }

    auto category = args[0].toString();
    auto presetName = args[1].toString();
    auto& pm = processorRef.getPresetManager();

    // Build path to preset
    juce::File presetFile;
    if (category == "User") {
        presetFile = pm.getUserPresetsDirectory().getChildFile(presetName + ".json");
    } else {
        presetFile = pm.getFactoryPresetsDirectory()
                       .getChildFile(category)
                       .getChildFile(presetName + ".json");
    }

    if (presetFile.existsAsFile()) {
        // Use loadPresetFromFile method
        auto success = pm.loadPresetFromFile(presetFile);
        complete(juce::var(success));
    } else {
        complete(juce::var(false));
    }
})
```

### WebView UI JavaScript

```javascript
// Populate categorized dropdown
async function showPresetDropdown() {
    const dropdown = document.getElementById('preset-dropdown');
    dropdown.innerHTML = '';

    try {
        // Get categorized presets
        const categoriesJson = await Juce.getNativeFunction('getPresetListWithCategories')();
        const categories = JSON.parse(categoriesJson);

        const categoryOrder = ['Orchestral', 'Sacred', 'World', 'Ambient', 'Cinematic', 'User'];

        for (const category of categoryOrder) {
            const presets = categories[category];
            if (!presets || presets.length === 0) continue;

            // Category header
            const header = document.createElement('div');
            header.className = 'preset-dropdown-header';
            header.textContent = category;
            dropdown.appendChild(header);

            // Presets in category
            for (const presetName of presets) {
                const item = document.createElement('div');
                item.className = 'preset-dropdown-item';
                item.textContent = presetName;
                item.dataset.category = category;

                item.addEventListener('click', async (e) => {
                    e.stopPropagation();
                    hidePresetDropdown();

                    await Juce.getNativeFunction('loadPresetFromCategory')(category, presetName);
                    document.getElementById('preset-name-display').textContent = presetName;
                });

                dropdown.appendChild(item);
            }
        }
    } catch (err) {
        console.error('[Presets] Failed to load categories:', err);
    }

    dropdown.classList.add('show');
}
```

---

## 5. Testing Considerations

### pluginval Tests
- State save/restore must preserve current preset name
- Parameter automation must work for all 22 parameters
- Factory preset loading must not crash

### DAW Testing Checklist
- [ ] Presets load correctly in Logic Pro
- [ ] Preset changes reflect in UI immediately
- [ ] Previous/Next navigation wraps correctly
- [ ] User presets save to correct location
- [ ] Category dropdown displays correctly
- [ ] State saves preset name with project
- [ ] No audio glitches during preset changes

### Stress Test
- Rapid preset switching (10+ changes in 5 seconds)
- Load while playing (no CPU spikes)
- Save while playing (no dropouts)

---

## 6. Files to Create/Modify

### New Files
| File | Purpose |
|------|---------|
| `Source/OuariconPresetManager.h` | Copy from module |
| `~/Library/O-Bells/Presets/Factory/*/` | Category directories |
| `*.json` (25 files) | Factory preset JSON files |

### Modified Files
| File | Changes |
|------|---------|
| `Source/PluginProcessor.h` | Add presetManager member, getPresetManager() |
| `Source/PluginProcessor.cpp` | Add initializeFactoryPresets(), presetManager init |
| `Source/PluginEditor.h` | Add fileChooser shared_ptr |
| `Source/PluginEditor.cpp` | Add 8 native functions for presets |
| `Resources/ui/index.html` | Add preset bar to header, JS preset logic |
| `CMakeLists.txt` | No changes needed (header-only preset manager) |

---

## 7. Summary

**Approach:** UI-side category grouping with folder-based factory presets

**Key Decisions:**
1. Copy OuariconPresetManager.h to plugin (no module modification)
2. Store factory presets in category subdirectories
3. Add `getPresetListWithCategories` and `loadPresetFromCategory` native functions
4. Adapt O-Lyrica preset bar HTML/CSS with category headers
5. Create 25 factory presets across 5 categories

**Risk Assessment:**
- Low: Pattern proven in O-Lyrica
- Low: Module is header-only, no linking issues
- Medium: Category scanning adds ~1ms to dropdown open

**Next Phase:** Plan (task breakdown and file modification details)

---

*Research completed: 2026-02-02*
