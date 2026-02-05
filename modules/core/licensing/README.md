# Licensing Module

License activation, offline validation, and UI overlay for Ouaricon plugins.

## Overview

Integrates with the Supabase licensing backend to provide:

- **Online activation** — user enters `OUA-XXXX-XXXX-XXXX` key, plugin calls `activate-license` endpoint
- **Offline validation** — ES256-signed JWT token stored locally, valid for 30 days without network
- **Periodic re-validation** — background refresh every 24 hours when online
- **Native overlay UI** — covers the plugin editor until a valid license is activated
- **Compile-flag gated** — `OUARICON_LICENSING_ENABLED` define, OFF by default for development

## Platform Support

| Platform | Machine ID Source | Token Storage |
|----------|-------------------|---------------|
| macOS | IOKit serial number | `~/Library/Application Support/Ouaricon/{product}/` |
| Windows | Registry MachineGuid | `%APPDATA%/Ouaricon/{product}/` |
| Linux | `/etc/machine-id` | `~/.config/Ouaricon/{product}/` |

## Files

| File | Description |
|------|-------------|
| `cpp/OuariconLicense.h` | Main license manager class (header) |
| `cpp/OuariconLicense.cpp` | Implementation (HTTP, JWT, machine ID, file I/O) |
| `cpp/OuariconLicenseUI.h` | Native JUCE overlay component (header-only) |

## Quick Start

See `snippets/INTEGRATION-CHECKLIST.md` for step-by-step instructions.

### Minimal integration (3 files to edit):

**1. Plugin CMakeLists.txt:**
```cmake
if(OUARICON_LICENSING)
    ouaricon_add_module(YourPlugin licensing)
endif()
```

**2. PluginEditor.h:**
```cpp
#if OUARICON_LICENSING_ENABLED
 #include "OuariconLicense.h"
 #include "OuariconLicenseUI.h"
#endif

// In class declaration:
#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicense> licenseManager;
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
#endif
```

**3. PluginEditor.cpp:**
```cpp
// Constructor:
#if OUARICON_LICENSING_ENABLED
    licenseManager = std::make_unique<OuariconLicense> (
        "your-product-id", OUARICON_SUPABASE_URL, OUARICON_SUPABASE_ANON_KEY);
    licenseOverlay = std::make_unique<OuariconLicenseOverlay> (*licenseManager);
    addAndMakeVisible (licenseOverlay.get());
#endif

// resized():
#if OUARICON_LICENSING_ENABLED
    if (licenseOverlay != nullptr)
        licenseOverlay->setBounds (getLocalBounds());
#endif
```

## How It Works

```
Plugin Launch
  |
  +-- Load stored token from disk
  |     |
  |     +-- Token valid & not expired --> Licensed (start 24h re-validation timer)
  |     |     |
  |     |     +-- Token expiring within 7 days --> Background refresh
  |     |
  |     +-- Token expired --> Try online refresh
  |     |     |
  |     |     +-- Online success --> Licensed (new token saved)
  |     |     +-- Network error --> Show overlay (Expired)
  |     |
  |     +-- No token found --> Show overlay (Unlicensed)
  |
  +-- User enters key in overlay
        |
        +-- POST /activate-license
              |
              +-- 200 OK --> Save token, hide overlay
              +-- 403 activation_limit_reached --> Show "deactivate at portal"
              +-- 403 license_revoked --> Show "license revoked"
              +-- Network error --> Show "check connection"
```

## API Endpoints Used

| Endpoint | Purpose |
|----------|---------|
| `POST /functions/v1/activate-license` | Register machine, get offline token |
| `POST /functions/v1/validate-license` | Periodic re-validation |
| `POST /functions/v1/deactivate-license` | Free up activation slot |

## Dependencies

- `juce_core` (HTTP, JSON, File, SHA256, Base64, SystemStats)
- `juce_gui_basics` (Component, Label, TextEditor, TextButton)
- **No external libraries** — uses only JUCE and OS platform APIs

## Build Configuration

The licensing system is controlled by a single CMake option:

```cmake
# Root CMakeLists.txt
option(OUARICON_LICENSING "Enable license checking in plugins" OFF)
```

- `OFF` (default): No licensing code compiled. Plugins run freely.
- `ON`: Licensing code included. Set in GitHub Actions for publish builds.

### Required CMake defines (when ON):

```cmake
-DOUARICON_SUPABASE_URL="https://your-project.supabase.co"
-DOUARICON_SUPABASE_ANON_KEY="your-anon-key"
```

These are passed in the GitHub Actions workflow via repository secrets.
