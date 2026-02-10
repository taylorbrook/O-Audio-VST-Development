# add-licensing

---
name: add-licensing
description: Integrates the Ouaricon licensing module into a plugin. Usage `/add-licensing {PluginFolder} {product-id}` (e.g., `/add-licensing O-Tremolo ouaricon-tremolo`).
arguments:
  - name: PluginFolder
    description: Plugin directory name under plugins/ (e.g., O-Tremolo, O-DigiDelay)
    required: true
  - name: product-id
    description: Product identifier matching backend (e.g., ouaricon-tremolo, ouaricon-delay)
    required: false
---

## Prompt for Missing Product ID

If `product-id` was NOT provided, you MUST ask the user for it before proceeding. Use `AskUserQuestion` with a text prompt like:

> "What is the Supabase product ID for this plugin? (e.g., `ouaricon-tremolo`, `ouaricon-delay`)"

Do NOT guess or auto-generate the product ID — it must match what exists in the Supabase backend.

## Overview

This skill adds licensing support to any Ouaricon plugin. All added code is gated behind `#if OUARICON_LICENSING_ENABLED` so it has zero impact on local dev builds (the flag defaults to OFF).

The licensing module lives at `modules/core/licensing/` and provides:
- `OuariconLicense` — license manager (activation, offline JWT tokens, periodic re-validation)
- `OuariconLicenseOverlay` — native JUCE overlay UI that blocks the editor until licensed

### Architecture: License Manager on Processor

The license manager lives on the **Processor** (not the Editor). This is critical because DAWs destroy and recreate the Editor whenever the plugin window is closed/reopened, but the Processor persists for the entire plugin lifetime. If the license manager lived on the Editor, activation state would be lost on every UI reload, causing the activation window to reappear even after successful activation.

The Editor gets a reference to the Processor's license manager and listens for status changes.

## Pre-flight Checks

Before making any edits, perform these checks. Stop and report if any fail.

### 1. Verify plugin folder exists

```
plugins/{PluginFolder}/CMakeLists.txt
plugins/{PluginFolder}/Source/PluginProcessor.h
plugins/{PluginFolder}/Source/PluginProcessor.cpp
plugins/{PluginFolder}/Source/PluginEditor.h
plugins/{PluginFolder}/Source/PluginEditor.cpp
```

If any file is missing, stop: `"Plugin folder '{PluginFolder}' not found or missing required files."`

### 2. Read the CMake target name

Open `plugins/{PluginFolder}/CMakeLists.txt` and find the first argument to `juce_add_plugin()`. This is the **Target** name (e.g., `OuariconTremolo`, `OuariconDigitalDelay`). Folder names don't match targets, so always read it from the file.

### 3. Read the Editor and Processor class names

Open `PluginEditor.h` and `PluginProcessor.h` to find the actual class names. Never assume — always read.

### 4. Check for existing integration

Search `plugins/{PluginFolder}/Source/PluginEditor.h` for the string `OUARICON_LICENSING_ENABLED`. If found, stop: `"Licensing is already integrated into {PluginFolder}."`

## Edit 1: CMakeLists.txt

Open `plugins/{PluginFolder}/CMakeLists.txt`.

### 1a. Ensure OuariconModules.cmake is included

Check if `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` is already present. If NOT, add it near the top of the file (after `cmake_minimum_required`).

### 1b. Add the licensing module block

Find the line `juce_generate_juce_header({Target})`. **Immediately after** that line, insert:

```cmake
# Licensing module (compile-flag gated, OFF for local dev)
if(OUARICON_LICENSING)
    ouaricon_add_module({Target} licensing)
    target_compile_definitions({Target} PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries({Target} PRIVATE juce::juce_cryptography)
endif()
```

Replace `{Target}` with the actual CMake target name read in pre-flight step 2.

**Critical:** The `juce_cryptography` link is required because the licensing module uses `juce::SHA256` for machine ID hashing. Without it, CI builds (which enable `OUARICON_LICENSING`) will fail with unresolved `SHA256` linker errors.

## Edit 2: PluginProcessor.h

Open `plugins/{PluginFolder}/Source/PluginProcessor.h`.

### 2a. Add guarded include

Find the last `#include` line in the file. **After** it, add:

```cpp
#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicense.h"
#endif
```

### 2b. Add public getter

Find a suitable location in the `public:` section (after parameter declarations like `juce::AudioProcessorValueTreeState parameters;` or after any atomic meter variables). Add:

```cpp
#if OUARICON_LICENSING_ENABLED
    OuariconLicense& getLicenseManager() { return *licenseManager; }
#endif
```

### 2c. Add private member

Find the line `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`. **Before** it, add:

```cpp
#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicense> licenseManager;
#endif
```

## Edit 3: PluginProcessor.cpp

Open `plugins/{PluginFolder}/Source/PluginProcessor.cpp`.

### 3a. Initialize license manager in constructor

Find the Processor constructor body. At the **end** of the constructor body (before the closing `}`), add:

```cpp
#if OUARICON_LICENSING_ENABLED
    licenseManager = std::make_unique<OuariconLicense>(
        "{product-id}", OUARICON_SUPABASE_URL, OUARICON_SUPABASE_ANON_KEY);
#endif
```

Replace `{product-id}` with the actual product-id argument (e.g., `"ouaricon-tremolo"`).

**Note:** `OUARICON_SUPABASE_URL` and `OUARICON_SUPABASE_ANON_KEY` are CMake defines injected by CI (repo secrets). They don't need to exist locally since the entire block is gated by `OUARICON_LICENSING_ENABLED`.

## Edit 4: PluginEditor.h

Open `plugins/{PluginFolder}/Source/PluginEditor.h`.

### 4a. Add guarded include

Find the last `#include` line in the file. **After** it, add:

```cpp
#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicenseUI.h"
#endif
```

Note: Only `OuariconLicenseUI.h` is needed here (not `OuariconLicense.h`) — the Editor accesses the license manager through the Processor header which already includes it.

### 4b. Add Listener inheritance

Find the Editor class declaration. It will look something like:

```cpp
class {EditorClass} : public juce::AudioProcessorEditor,
                      private juce::Timer
```

Add the licensing listener as a conditional base class:

```cpp
class {EditorClass} : public juce::AudioProcessorEditor,
                      private juce::Timer
#if OUARICON_LICENSING_ENABLED
                    , private OuariconLicense::Listener
#endif
```

### 4c. Add member declarations

Find the line `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`. **Before** it, add:

```cpp
#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
    void licenseStatusChanged(OuariconLicense&, OuariconLicense::Status) override;
#endif
```

## Edit 5: PluginEditor.cpp — Constructor

Open `plugins/{PluginFolder}/Source/PluginEditor.cpp`.

Find the `addAndMakeVisible(*webView)` call in the constructor. **After** it (but **before** attachments or `setSize()`), insert:

```cpp
#if OUARICON_LICENSING_ENABLED
    // Licensing: activation overlay (visible until licensed)
    // Native WebView renders on top of JUCE components, so we must
    // hide the WebView while the overlay is showing.
    // License manager lives on the processor (persists across editor open/close).
    auto& license = audioProcessor.getLicenseManager();
    licenseOverlay = std::make_unique<OuariconLicenseOverlay>(license);
    addAndMakeVisible(licenseOverlay.get());

    license.addListener(this);

    if (! license.isLicensed())
        webView->setVisible(false);
    else
        licenseOverlay->setVisible(false);
#endif
```

**Important:** The reference name `audioProcessor` must match the actual member variable name that references the Processor. Read the constructor to confirm.

## Edit 6: PluginEditor.cpp — Destructor

Find the Editor destructor. **Before** any existing cleanup (typically before `stopTimer()` or at the top of the destructor body), add:

```cpp
#if OUARICON_LICENSING_ENABLED
    audioProcessor.getLicenseManager().removeListener(this);
#endif
```

This prevents dangling listener callbacks after the editor is destroyed.

## Edit 7: PluginEditor.cpp — resized()

In the same file, find the `resized()` method. Find the `webView->setBounds(...)` line. **After** it, add:

```cpp
#if OUARICON_LICENSING_ENABLED
    if (licenseOverlay != nullptr)
        licenseOverlay->setBounds(getLocalBounds());
#endif
```

## Edit 8: PluginEditor.cpp — licenseStatusChanged callback

At the **bottom** of the file (before the closing of any namespace, or at the end), add:

```cpp
#if OUARICON_LICENSING_ENABLED
//==============================================================================
void {EditorClass}::licenseStatusChanged(
    OuariconLicense&, OuariconLicense::Status newStatus)
{
    juce::MessageManager::callAsync([this, newStatus]()
    {
        bool licensed = (newStatus == OuariconLicense::Status::Licensed);
        webView->setVisible(licensed);

        if (licenseOverlay)
            licenseOverlay->setVisible(! licensed);
    });
}
#endif
```

Replace `{EditorClass}` with the actual Editor class name read in pre-flight step 3.

## Post-edit Verification

After all 8 edits are complete:

1. **Review the diff** — Confirm all `#if OUARICON_LICENSING_ENABLED` guards are properly closed with `#endif`.

2. **Run a local build** to confirm clean compile with licensing OFF (default):

```bash
cd /Users/taylorbrook/Dev/VST-development/build
ninja {Target}_VST3 {Target}_AU
```

3. If the build succeeds, report success. If it fails, the licensing code shouldn't be the cause (it's gated OFF), so investigate the build error normally.

## Summary of Changes

| File | What was added |
|------|---------------|
| `CMakeLists.txt` | `if(OUARICON_LICENSING)` block after `juce_generate_juce_header()` — includes `juce_cryptography` link |
| `PluginProcessor.h` | Guarded `#include`, public `getLicenseManager()` getter, private `licenseManager` member |
| `PluginProcessor.cpp` | License manager initialization in constructor |
| `PluginEditor.h` | Guarded `#include`, `OuariconLicense::Listener` inheritance, `licenseOverlay` member + callback declaration |
| `PluginEditor.cpp` (constructor) | Get license ref from processor, create overlay, add listener, set initial visibility |
| `PluginEditor.cpp` (destructor) | `removeListener(this)` cleanup |
| `PluginEditor.cpp` (resized) | Overlay bounds after `webView->setBounds()` |
| `PluginEditor.cpp` (bottom) | `licenseStatusChanged` callback implementation |
