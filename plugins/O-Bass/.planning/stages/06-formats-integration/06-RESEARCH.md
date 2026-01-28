# Phase 6: Formats & Integration - Research

**Researched:** 2026-01-25
**Domain:** JUCE Audio Plugin Formats (VST3, AU) + Preset Management
**Confidence:** HIGH

## Summary

This phase integrates the OuariconPresetManager system with OBass and validates plugin builds across VST3 and AU formats. The project already has a working preset manager module used by O-Tremolo, O-SimpleReverb, and other Ouaricon plugins. OBass currently builds and registers as AU (`aufx OuBa OuAu`) but lacks preset management integration.

The standard approach is to:
1. Copy the OuariconPresetManager.h header to OBass/Source/
2. Modify PluginProcessor to include preset manager member and factory preset initialization
3. Update getStateInformation/setStateInformation to use preset manager
4. Define 8-12 factory presets with descriptive names
5. Validate with pluginval (strictness 5+) and auval

**Primary recommendation:** Use the existing OuariconPresetManager pattern from O-Tremolo as the reference implementation - copy, adapt parameter IDs, define factory presets.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.x | Audio plugin framework | Already in use, handles VST3/AU format details |
| OuariconPresetManager | 1.5.0+ | Preset persistence | Project standard, lazy initialization for AU validation |

### Supporting
| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| pluginval | 1.0+ | Cross-platform plugin validation | CI/development testing, strictness 5+ |
| auval | 1.10.0 | Apple AU validation | macOS AU format compliance |
| AudioComponentRegistrar | system | AU registration cache | Clear after build/install |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| OuariconPresetManager | Raw APVTS state | No factory presets, no navigation, reinventing wheel |
| pluginval | Manual DAW testing only | Misses edge cases, no CI integration |

**Installation:**
```bash
# pluginval is installed at /Applications/pluginval.app
# auval is built into macOS at /usr/bin/auval

# Verify pluginval
/Applications/pluginval.app/Contents/MacOS/pluginval --help

# Verify auval
auval -v aumu OuBa OuAu  # For effect plugins use: auval -v aufx OuBa OuAu
```

## Architecture Patterns

### Recommended Project Structure
```
plugins/OBass/Source/
    PluginProcessor.h          # Add OuariconPresetManager member
    PluginProcessor.cpp        # Add factory presets, update state methods
    OuariconPresetManager.h    # Copy from modules/persistence/preset-manager/cpp/
    PluginEditor.h             # No changes needed (WebView handles preset UI)
    PluginEditor.cpp
    DSP/                       # No changes
    ui/                        # No changes
```

### Pattern 1: Preset Manager Integration
**What:** Add OuariconPresetManager to PluginProcessor, initialize factory presets in constructor
**When to use:** All Ouaricon plugins requiring preset persistence
**Example:**
```cpp
// Source: O-Tremolo/Source/PluginProcessor.cpp (verified working)

// In PluginProcessor.h:
#include "OuariconPresetManager.h"

class OBassAudioProcessor : public juce::AudioProcessor
{
public:
    // ... existing members ...
    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;  // ADD THIS
    // ...
};

// In PluginProcessor.cpp constructor:
OBassAudioProcessor::OBassAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "OBass")  // Plugin name for preset directory
{
    // Initialize factory presets
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        {
            "Default",
            {{"crossover_freq", 0.25f}, {"enhance", 0.50f}, {"enhanceMode", 0.0f}, {"output", 0.5f}},
            juce::var()
        },
        // ... more presets ...
    };
    presetManager.initializeFactoryPresets(factoryPresets);
}
```

### Pattern 2: State Persistence via PresetManager
**What:** Delegate getStateInformation/setStateInformation to preset manager
**When to use:** All plugins with OuariconPresetManager
**Example:**
```cpp
// Source: O-Tremolo/Source/PluginProcessor.cpp (verified working)

void OBassAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OBassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        presetManager.setStateFromXml(xml.get());
}
```

### Pattern 3: Factory Preset Definition with Normalized Values
**What:** Preset parameters use normalized 0.0-1.0 values, not actual ranges
**When to use:** All FactoryPresetDef definitions
**Example:**
```cpp
// OBass parameter ranges (from PluginProcessor.cpp):
// crossover_freq: 40-200Hz, default 80Hz, skew 0.5
// enhance: 0-100%, default 50%
// enhanceMode: 0=Clean, 1=Colored
// output: -18 to +18dB, default 0dB

// Normalized value calculation:
// For crossover_freq with skew 0.5:
//   norm = (actual - min) / (max - min), then apply inverse skew
//   80Hz -> ~0.25 normalized (with skew)
//   100Hz -> ~0.375 normalized
// For linear parameters:
//   enhance 50% -> 0.5
//   output 0dB -> 0.5 (midpoint of -18 to +18)
```

### Anti-Patterns to Avoid
- **Creating directories in constructor:** The v1.5.0 preset manager uses lazy initialization - directory creation deferred to first use. This is critical for AU validation which instantiates plugins during scanning.
- **Using actual parameter values in FactoryPresetDef:** All values must be normalized 0.0-1.0. Using "80" for 80Hz will break - use the normalized equivalent.
- **Calling file I/O from audio thread:** PresetManager methods are NOT real-time safe. Only call load/save from message thread.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Preset save/load | Custom JSON/XML handler | OuariconPresetManager | Handles Factory vs User distinction, DAW state, navigation |
| AU cache clearing | Manual rm commands | Standard sequence (below) | AudioComponentRegistrar must be killed first |
| Plugin validation | Manual DAW testing | pluginval + auval | Catches edge cases like parameter fuzz, threading issues |
| Normalized value math | Manual calculation | juce::NormalisableRange::convertTo0to1() | Handles skew correctly |

**Key insight:** The OuariconPresetManager has already solved Factory/User preset separation, alphabetical sorting, DAW session state integration, and preset navigation. Don't duplicate this logic.

## Common Pitfalls

### Pitfall 1: AU Validation Fails Due to Constructor I/O
**What goes wrong:** AU validation times out or fails because constructor performs file I/O
**Why it happens:** macOS AU scanner instantiates plugins to read metadata. Slow constructors fail validation.
**How to avoid:** Use v1.5.0+ OuariconPresetManager with lazy directory initialization. Never do file I/O in constructor.
**Warning signs:** auval shows "FATAL ERROR: Component instantiation timed out" or "Problem opening component"

### Pitfall 2: Stale Plugin Cache After Build
**What goes wrong:** DAW shows old version of plugin after rebuild
**Why it happens:** macOS caches AU metadata. VST3 has similar DAW-specific caches.
**How to avoid:** Always run cache clear sequence after build:
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/OBass.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/OBass.component
cp -R build/plugins/OBass/OBass_artefacts/Release/VST3/OBass.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/OBass/OBass_artefacts/Release/AU/OBass.component ~/Library/Audio/Plug-Ins/Components/
```
**Warning signs:** Parameter changes don't appear, old UI shows up, plugin behaves differently than code suggests

### Pitfall 3: Normalized Values Calculated Incorrectly for Skewed Parameters
**What goes wrong:** Preset loads but parameters are at wrong values
**Why it happens:** OuariconPresetManager uses normalized 0-1 values. Parameters with skew (like frequency) don't map linearly.
**How to avoid:** Use JUCE's NormalisableRange::convertTo0to1() to calculate correct normalized values, or test empirically.
**Warning signs:** 100Hz preset loads as 200Hz, or crossover is too high/low

### Pitfall 4: Parameter Listener Leaks
**What goes wrong:** pluginval crashes with "parameter listener referenced after destruction"
**Why it happens:** UI components add themselves as parameter listeners but don't remove in destructor
**How to avoid:** Always call removeListener() in Component destructor. With WebView UI, this is less common but still possible if native parameter observers are used.
**Warning signs:** pluginval fails at strictness 5+, crashes on plugin close

### Pitfall 5: pluginval "No Types Found" Error
**What goes wrong:** pluginval cannot find any plugin types in the binary
**Why it happens:** Multiple causes: wrong path, architecture mismatch (Intel vs ARM), AU not in system folder
**How to avoid:**
1. Verify binary architecture: `file path/to/plugin.vst3/Contents/MacOS/PluginName`
2. For AU, ensure plugin is in ~/Library/Audio/Plug-Ins/Components/
3. Use exact path to .vst3 bundle
**Warning signs:** "Unable to load VST-3 plug-in file", "Unable to create juce::AudioPluginInstance"

## Code Examples

Verified patterns from existing Ouaricon plugins:

### Factory Preset Definitions (from O-Tremolo)
```cpp
// Source: plugins/O-Tremolo/Source/PluginProcessor.cpp

std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
    {
        "Default",
        {{"SPEED_PARAM", 0.221f}, {"DEPTH_PARAM", 0.75f}, {"WAVEFORM_PARAM", 0.0f},
         {"SMOOTHING_PARAM", 0.30f}, {"PAN_SYNC_PARAM", 0.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
        juce::var()  // No custom state
    },
    {
        "Slow Pulse",
        {{"SPEED_PARAM", 0.05f}, {"DEPTH_PARAM", 0.85f}, {"WAVEFORM_PARAM", 0.0f},
         {"SMOOTHING_PARAM", 0.50f}, {"PAN_SYNC_PARAM", 0.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
        juce::var()
    },
    // ... more presets
};

presetManager.initializeFactoryPresets(factoryPresets);
```

### OBass Parameter IDs and Ranges
```cpp
// Source: plugins/OBass/Source/PluginProcessor.cpp

// crossover_freq: 40-200Hz, default 80Hz, skew 0.5
//   Normalized values: 40Hz=0.0, 80Hz~0.25, 100Hz~0.375, 200Hz=1.0

// enhance: 0-100%, default 50%
//   Normalized values: 0%=0.0, 50%=0.5, 100%=1.0 (linear)

// enhanceMode: Choice [Clean, Colored], default 0
//   Normalized values: Clean=0.0, Colored=1.0

// output: -18 to +18dB, default 0dB
//   Normalized values: -18dB=0.0, 0dB=0.5, +18dB=1.0 (linear)

// latency_mode: Choice [Low Latency, High Fidelity], default 0
//   Normalized values: Low Latency=0.0, High Fidelity=1.0

// bypass: Bool, default false
//   Normalized values: false=0.0, true=1.0
```

### Validation Commands
```bash
# auval for AU validation (OBass is type 'aufx' = effect)
auval -v aufx OuBa OuAu

# pluginval at strictness 5 (minimum for compatibility)
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 5 \
  --validate ~/Library/Audio/Plug-Ins/VST3/OBass.vst3

# pluginval at strictness 10 (maximum - includes auval automatically)
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 \
  --validate ~/Library/Audio/Plug-Ins/Components/OBass.component
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Constructor I/O for presets | Lazy initialization (v1.5.0) | 2025 | AU validation passes |
| Manual auval + pluginval | pluginval runs auval at strictness 5+ | v1.0 | Single validation command |
| Plugin installed manually | CMake COPY_PLUGIN_AFTER_BUILD | JUCE 6+ | Streamlined dev workflow |

**Deprecated/outdated:**
- Using `auval -strict` directly: pluginval now handles this automatically at strictness 5+
- OuariconPresetManager versions < 1.5.0: Constructor creates directories, causing AU validation issues

## Open Questions

Things that couldn't be fully resolved:

1. **Exact normalized values for skewed crossover_freq parameter**
   - What we know: 80Hz default, skew 0.5, range 40-200Hz
   - What's unclear: Exact formula for skew 0.5 inverse calculation
   - Recommendation: Test empirically or use JUCE's NormalisableRange to calculate

2. **pluginval crash reports on system**
   - What we know: Multiple crash reports from 2026-01-24 in ~/Library/Logs/DiagnosticReports/
   - What's unclear: Which plugin(s) caused crashes, whether fixed
   - Recommendation: Run pluginval after OBass integration to verify no new crashes

## Sources

### Primary (HIGH confidence)
- `/Users/taylorbrook/Dev/VST-development/modules/persistence/preset-manager/cpp/OuariconPresetManager.h` - v1.5.0 implementation with lazy init
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Tremolo/Source/PluginProcessor.cpp` - Reference implementation of preset integration
- `/Users/taylorbrook/Dev/VST-development/plugins/OBass/Source/PluginProcessor.cpp` - Current OBass parameter definitions

### Secondary (MEDIUM confidence)
- [Tracktion/pluginval GitHub](https://github.com/Tracktion/pluginval) - Strictness levels 1-10, auval integration at level 5+
- [Melatonin: Pluginval is a plugin dev's best friend](https://melatonin.dev/blog/pluginval-is-a-plugin-devs-best-friend/) - Practical usage tips, common failures
- [Apple Technical Note TN2204](https://developer.apple.com/library/archive/technotes/tn2204/_index.html) - auval validation phases

### Tertiary (LOW confidence)
- [JUCE Forum discussions](https://forum.juce.com/) - Community patterns (verify before implementing)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Using existing project patterns (O-Tremolo reference)
- Architecture: HIGH - Pattern verified in multiple working plugins
- Pitfalls: HIGH - Documented from actual project experience and official sources

**Research date:** 2026-01-25
**Valid until:** 2026-02-25 (30 days - stable domain)
