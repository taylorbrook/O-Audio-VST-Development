# Licensing Module — Integration Checklist

Follow these steps to add license activation to any Ouaricon plugin.

---

## 1. Plugin CMakeLists.txt

Add the module conditionally so it only compiles in publish builds:

```cmake
# After: include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

if(OUARICON_LICENSING)
    ouaricon_add_module(YourPluginTarget licensing)
endif()
```

- [ ] Added `ouaricon_add_module` call inside `if(OUARICON_LICENSING)` block

---

## 2. PluginEditor.h

Add the licensing members behind the compile flag:

```cpp
// At top of file:
#if OUARICON_LICENSING_ENABLED
 #include "OuariconLicense.h"
 #include "OuariconLicenseUI.h"
#endif

class YourPluginEditor : public juce::AudioProcessorEditor
{
    // ... existing members ...

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicense> licenseManager;
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
#endif
};
```

- [ ] Added `#include` guards
- [ ] Added `licenseManager` and `licenseOverlay` members

---

## 3. PluginEditor.cpp — Constructor

Initialize the license manager after your existing setup:

```cpp
#if OUARICON_LICENSING_ENABLED
    licenseManager = std::make_unique<OuariconLicense> (
        "your-product-id",          // Must match backend product_id
        OUARICON_SUPABASE_URL,      // Set via CMake define
        OUARICON_SUPABASE_ANON_KEY  // Set via CMake define
    );

    licenseOverlay = std::make_unique<OuariconLicenseOverlay> (*licenseManager);
    addAndMakeVisible (licenseOverlay.get());
#endif
```

**Product IDs** (must match the website backend):
- `ouaricon-tremolo`
- `ouaricon-reverb`
- `ouaricon-chorus`
- `ouaricon-delay`
- `ouaricon-eq`

- [ ] Initialized `licenseManager` with correct product ID
- [ ] Created and added `licenseOverlay`

---

## 4. PluginEditor.cpp — resized()

Add at the **end** of `resized()` so the overlay covers everything:

```cpp
#if OUARICON_LICENSING_ENABLED
    if (licenseOverlay != nullptr)
        licenseOverlay->setBounds (getLocalBounds());
#endif
```

- [ ] Overlay bounds set in `resized()`

---

## 5. Verify Local Build Still Works

Local dev builds should compile without licensing (flag is OFF by default):

```bash
# From build directory — should compile cleanly with no licensing code
ninja YourPlugin_VST3 YourPlugin_AU
```

- [ ] Local build succeeds without licensing

---

## 6. GitHub Actions (already configured)

The root `CMakeLists.txt` and workflow are already set up. Verify:

- [ ] Root `CMakeLists.txt` has `option(OUARICON_LICENSING ...)`
- [ ] `.github/workflows/build-and-release.yml` passes `-DOUARICON_LICENSING=ON`
- [ ] Secrets `SUPABASE_URL` and `SUPABASE_ANON_KEY` are set in GitHub repo settings

---

## Done!

After completing these steps:
- **Local builds**: Plugin runs without any license checks
- **Published builds**: Plugin shows license overlay until user enters their key
- **Licensed users**: Overlay auto-hides, plugin works normally with 30-day offline token
