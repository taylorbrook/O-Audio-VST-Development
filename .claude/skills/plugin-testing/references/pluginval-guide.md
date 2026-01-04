# Pluginval Validation Guide

**Pluginval:** Third-party validation tool by Tracktion (https://github.com/Tracktion/pluginval)

**macOS Installation:** The standard installation places pluginval at `/Applications/pluginval.app/Contents/MacOS/pluginval`

## What Pluginval Tests

- **Plugin load/unload**: No crashes, memory leaks, global state
- **Parameter automation**: All parameters respond to automation
- **State recall**: Save/load works correctly
- **Thread safety**: No allocations in `processBlock()`, thread-safe state access
- **Audio validation**: Valid sample rates, buffer sizes, no silent outputs
- **Bus layouts**: Mono, stereo, sidechain configurations
- **MIDI handling**: Note on/off, CC, pitch bend
- **Preset handling**: Factory presets load correctly
- **UI validation**: Editor opens/closes without crashes

## Test Strictness Levels

- **5**: Basic validation (load/unload, process audio)
- **10**: Industry standard (recommended for release)
- **15**: Paranoid mode (extreme edge cases)

## Running Pluginval

```bash
# Step 1: Locate pluginval (check .app bundle first, then PATH)
if [ -x "/Applications/pluginval.app/Contents/MacOS/pluginval" ]; then
    PLUGINVAL_PATH="/Applications/pluginval.app/Contents/MacOS/pluginval"
elif command -v pluginval >/dev/null 2>&1; then
    PLUGINVAL_PATH="pluginval"
else
    echo "Error: Pluginval not found"
    echo "Install via: brew install --cask pluginval"
    echo "Or download from: https://github.com/Tracktion/pluginval/releases"
    exit 1
fi

# Step 2: Build Release mode
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel

# Step 3: Locate plugin binaries
VST3_PATH="build/plugins/$PLUGIN_NAME/${PLUGIN_NAME}_artefacts/Release/VST3/${PRODUCT_NAME}.vst3"
AU_PATH="build/plugins/$PLUGIN_NAME/${PLUGIN_NAME}_artefacts/Release/AU/${PRODUCT_NAME}.component"

# Step 4: Run pluginval on VST3
"${PLUGINVAL_PATH}" --validate "$VST3_PATH" \
          --strictness-level 10 \
          --timeout-ms 30000 \
          --verbose

# Step 5: Run pluginval on AU
"${PLUGINVAL_PATH}" --validate "$AU_PATH" \
          --strictness-level 10 \
          --timeout-ms 30000 \
          --verbose
```

## Expected Output (Success)

```
Pluginval 1.0.3 - Plugin Validation

Testing: [ProductName].vst3

Running tests with strictness level 10...

[1/50] Loading plugin... PASS (23ms)
[2/50] Parameter validation... PASS (45ms)
[3/50] State save/load... PASS (67ms)
[4/50] Thread safety check... PASS (120ms)
...
[50/50] Memory leak detection... PASS (89ms)

===============================================================================
All tests PASSED (50/50)

Total time: 4.2s
```

## Common Failures

### Parameter Automation Failure

```
❌ [12/50] Parameter automation... FAIL

Test: Automate all parameters
Issue: Parameter "threshold" does not respond to automation

Details:
- Set parameter via setParameter() → No audio change
- Expected: Automation should affect processBlock output
- Actual: Output unchanged

Fix: Verify parameter is read in processBlock() using apvts.getRawParameterValue()
```

### Thread Safety Failure

```
❌ [23/50] Thread safety check... FAIL

Test: Detect allocations in processBlock()
Issue: Memory allocation detected during audio processing

Allocations:
- std::vector::resize() called in processBlock() (PluginProcessor.cpp:147)
- juce::String constructor (PluginProcessor.cpp:203)

Fix: Pre-allocate buffers in prepareToPlay(), never allocate in processBlock()
```

### State Recall Failure

```
❌ [8/50] State save/load... FAIL

Test: Save state, change parameters, load state
Issue: Plugin state not restoring correctly

Parameters with incorrect values after load:
- bypass: expected 1.0, actual 0.5
- mix: expected 0.73, actual 0.5

Fix: Check getStateInformation() and setStateInformation() include all parameters
```

## Installation Issues

**If pluginval not found:**

```
Pluginval not found at /Applications/pluginval.app or in PATH.

Install options:
1. Via Homebrew: brew install --cask pluginval
   (Installs to /Applications/pluginval.app - RECOMMENDED)
2. Manual download: https://github.com/Tracktion/pluginval/releases
   (Place pluginval.app in /Applications/ folder)
3. Skip pluginval, try automated tests instead

After installation:
- Verify: ls -la /Applications/pluginval.app/Contents/MacOS/pluginval
- No need to add to PATH or create symlinks
- The system detects it automatically at the standard location

Choose (1-3): _
```
