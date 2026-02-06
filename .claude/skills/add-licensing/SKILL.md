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
    required: true
---

## Overview

This skill adds licensing support to any Ouaricon plugin. All added code is gated behind `#if OUARICON_LICENSING_ENABLED` so it has zero impact on local dev builds (the flag defaults to OFF).

The licensing module lives at `modules/core/licensing/` and provides:
- `OuariconLicense` — license manager (activation, offline JWT tokens, periodic re-validation)
- `OuariconLicenseOverlay` — native JUCE overlay UI that blocks the editor until licensed

## Pre-flight Checks

Before making any edits, perform these checks. Stop and report if any fail.

### 1. Verify plugin folder exists

```
plugins/{PluginFolder}/CMakeLists.txt
plugins/{PluginFolder}/Source/PluginEditor.h
plugins/{PluginFolder}/Source/PluginEditor.cpp
```

If any file is missing, stop: `"Plugin folder '{PluginFolder}' not found or missing required files."`

### 2. Read the CMake target name

Open `plugins/{PluginFolder}/CMakeLists.txt` and find the first argument to `juce_add_plugin()`. This is the **Target** name (e.g., `OuariconTremolo`, `OuariconDigitalDelay`). Folder names don't match targets, so always read it from the file.

### 3. Check for existing integration

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
endif()
```

Replace `{Target}` with the actual CMake target name read in pre-flight step 2.

## Edit 2: PluginEditor.h

Open `plugins/{PluginFolder}/Source/PluginEditor.h`.

### 2a. Add guarded includes

Find the last `#include` line in the file. **After** it, add:

```cpp
#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicense.h"
  #include "OuariconLicenseUI.h"
#endif
```

### 2b. Add member declarations

Find the line `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`. **Before** it, add:

```cpp
#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicense> licenseManager;
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
#endif
```

## Edit 3: PluginEditor.cpp — Constructor

Open `plugins/{PluginFolder}/Source/PluginEditor.cpp`.

Find the `addAndMakeVisible(*webView)` call in the constructor. **After** it (but **before** `setSize()`), insert:

```cpp
#if OUARICON_LICENSING_ENABLED
    // Licensing: activation overlay (visible until licensed)
    licenseManager = std::make_unique<OuariconLicense>(
        "{product-id}", OUARICON_SUPABASE_URL, OUARICON_SUPABASE_ANON_KEY);
    licenseOverlay = std::make_unique<OuariconLicenseOverlay>(*licenseManager);
    addAndMakeVisible(licenseOverlay.get());
#endif
```

Replace `{product-id}` with the actual product-id argument (e.g., `"ouaricon-tremolo"`).

**Note:** `OUARICON_SUPABASE_URL` and `OUARICON_SUPABASE_ANON_KEY` are CMake defines injected by CI (repo secrets). They don't need to exist locally since the entire block is gated by `OUARICON_LICENSING_ENABLED`.

## Edit 4: PluginEditor.cpp — resized()

In the same file, find the `resized()` method. Find the `webView->setBounds(...)` line. **After** it, add:

```cpp
#if OUARICON_LICENSING_ENABLED
    if (licenseOverlay != nullptr)
        licenseOverlay->setBounds(getLocalBounds());
#endif
```

## Post-edit Verification

After all 4 edits are complete:

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
| `CMakeLists.txt` | `if(OUARICON_LICENSING)` block after `juce_generate_juce_header()` |
| `PluginEditor.h` | Guarded `#include` and member declarations |
| `PluginEditor.cpp` (constructor) | `licenseManager` + `licenseOverlay` init after `addAndMakeVisible(*webView)` |
| `PluginEditor.cpp` (resized) | Overlay bounds after `webView->setBounds()` |
