# Stage 1 Critical Patterns - Foundation & Build

**Purpose:** Prevent repeat mistakes during Stage 1 (Foundation + Shell) implementation.

**When to read:** Before creating CMakeLists.txt and initial source files.

**Patterns included:** 7 of 22 total patterns (build system, configuration, deployment)

---

## 1. CMakeLists.txt - Header Generation (ALWAYS REQUIRED)

### ❌ WRONG (Will fail with "JuceHeader.h not found")
```cmake
target_link_libraries(MyPlugin
    PRIVATE
        juce::juce_audio_processors
)

target_compile_definitions(MyPlugin
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
)
```

### ✅ CORRECT
```cmake
target_link_libraries(MyPlugin
    PRIVATE
        juce::juce_audio_processors
)

# CRITICAL: Generate JuceHeader.h (JUCE 8 requirement)
juce_generate_juce_header(MyPlugin)

target_compile_definitions(MyPlugin
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
)
```

**Why:** JUCE 8.x does NOT ship a pre-built JuceHeader.h. The `juce_generate_juce_header()` function:
1. Scans linked JUCE modules
2. Auto-generates JuceHeader.h in build artifacts
3. Adds it to compiler include paths

**Placement:** MUST come after `target_link_libraries()`, BEFORE `target_compile_definitions()`

---

## 2. Include Style - Prefer Module Headers

### ✅ PREFERRED (Modern JUCE 8)
```cpp
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
```

### ⚠️ ACCEPTABLE (But requires juce_generate_juce_header())
```cpp
#include <JuceHeader.h>
```

**Recommendation:** Use individual module headers. They're explicit, don't require CMake generation, and match JUCE 8 best practices.

---

## 3. Bus Configuration - Effects vs Instruments

### Effects (Audio In → Audio Out)
```cpp
AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

### Instruments (MIDI In → Audio Out)
```cpp
AudioProcessor(BusesProperties()
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

**Common mistake:** Adding input bus to instruments causes "missing input" errors in DAWs.

---

## 4. CMakeLists.txt - NEEDS_WEB_BROWSER for VST3 (ALWAYS REQUIRED)

### ❌ WRONG (VST3 won't appear in DAW)
```cmake
juce_add_plugin(MyPlugin
    COMPANY_NAME "YourCompany"
    PLUGIN_CODE Plug
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "My Plugin"
    # Missing NEEDS_WEB_BROWSER - VST3 will be built but won't load
)
```

### ✅ CORRECT
```cmake
juce_add_plugin(MyPlugin
    COMPANY_NAME "YourCompany"
    PLUGIN_CODE Plug
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "My Plugin"
    NEEDS_WEB_BROWSER TRUE  # REQUIRED for VST3 WebView support
)
```

**Why:** VST3 format requires explicit WebView flag even if AU works without it. Missing this flag causes:
- VST3 builds successfully
- Binary exists in build artifacts
- But plugin doesn't appear in DAW VST3 scanner
- Only AU format visible

**When:** ANY plugin using WebBrowserComponent for UI

---

## 5. CMakeLists.txt - IS_SYNTH Flag for Instruments (ALWAYS REQUIRED)

### ❌ WRONG (No audio - MIDI not routed)
```cmake
# Missing IS_SYNTH flag - plugin treated as effect
juce_add_plugin(LushPad
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lush
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "LushPad"
    NEEDS_WEB_BROWSER TRUE
)
```

### ✅ CORRECT
```cmake
# IS_SYNTH TRUE declares plugin as instrument
juce_add_plugin(LushPad
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lush
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "LushPad"
    IS_SYNTH TRUE           # REQUIRED for instruments
    NEEDS_MIDI_INPUT TRUE   # Explicit MIDI requirement
    NEEDS_WEB_BROWSER TRUE
)
```

**Why:** JUCE uses `IS_SYNTH TRUE` to generate plugin metadata that tells DAWs this is an instrument:
- Sets `JucePlugin_IsSynth=1` preprocessor define
- Sets VST3 category to "Instrument|Synth" in moduleinfo.json
- Sets AU type to `'aumu'` (Audio Unit Music Device)
- DAWs read this metadata and enable MIDI routing automatically

**Plugin type decision:**
```
Does plugin CREATE audio from scratch (no audio input needed)?
├─ YES → IS_SYNTH TRUE + output-only BusesProperties
│         Examples: Synth, drum machine, noise generator
└─ NO  → IS_SYNTH FALSE + input+output BusesProperties
          Examples: Delay, reverb, compressor, EQ
```

---

## 6. Changing PRODUCT_NAME - Manual Cleanup Required (CRITICAL)

### ❌ WRONG (Leaves orphaned plugins)
```bash
# Change PRODUCT_NAME in CMakeLists.txt
PRODUCT_NAME "TAPE AGE"  →  PRODUCT_NAME "TapeAge"

# Build and install
./scripts/build-and-install.sh PluginName

# Result: Both "TAPE AGE" and "TapeAge" exist in system folders
```

### ✅ CORRECT
```bash
# BEFORE changing PRODUCT_NAME, manually remove old versions:
rm -rf ~/Library/Audio/Plug-Ins/VST3/"TAPE AGE.vst3"
rm -rf ~/Library/Audio/Plug-Ins/Components/"TAPE AGE.component"

# THEN change PRODUCT_NAME in CMakeLists.txt
PRODUCT_NAME "TapeAge"

# THEN build and install
./scripts/build-and-install.sh PluginName

# Clear caches
killall -9 AudioComponentRegistrar
rm ~/Library/Preferences/Ableton/*/PluginDatabase.cfg
```

**Why:** Build script searches for plugins matching the NEW product name, so old plugins aren't found and removed.

---

## 7. macOS Plugin Code Signing & Cache Management (ALWAYS REQUIRED)

### ❌ WRONG (Will cause plugins to not load or show old code)
```bash
# Using --deep flag when re-signing (corrupts binary!)
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/MyPlugin.vst3
```

### ✅ CORRECT
```bash
# Sign WITHOUT --deep flag (prevents corruption)
codesign --force --sign - ~/Library/Audio/Plug-Ins/VST3/MyPlugin.vst3
codesign --force --sign - ~/Library/Audio/Plug-Ins/Components/MyPlugin.component

# Verify signatures
codesign --verify --deep --strict ~/Library/Audio/Plug-Ins/VST3/MyPlugin.vst3

# Nuclear cache clear
killall "Ableton Live 12 Suite" "Logic Pro" 2>/dev/null
rm ~/Library/Preferences/Ableton/Live*/PluginScanner.txt 2>/dev/null
rm -rf ~/Library/Caches/AudioUnitCache 2>/dev/null
killall -9 AudioComponentRegistrar 2>/dev/null
```

**Critical mistakes:**
- **Using `codesign --deep`**: Modifies nested bundle contents, corrupting plugin
- **Not clearing caches**: DAW loads cached old version even with new binary
- **Not restarting**: AudioComponentRegistrar cache survives process kills

---

## Quick Reference

| Pattern | What It Prevents |
|---------|-----------------|
| Header Generation | "JuceHeader.h not found" build errors |
| Include Style | Inconsistent header inclusion |
| Bus Configuration | "Missing input" errors in DAWs |
| NEEDS_WEB_BROWSER | VST3 not appearing in DAW |
| IS_SYNTH Flag | No audio from synth plugins |
| PRODUCT_NAME Cleanup | Duplicate plugins in DAW |
| Code Signing | "Invalid signature" loading errors |

---

**Full patterns file:** `troubleshooting/patterns/juce8-critical-patterns.md`
