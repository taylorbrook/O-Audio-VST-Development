# Branding Configuration

This document explains how the Plugin Freedom System handles consistent branding across all plugins.

## Overview

All plugins built by this system automatically receive consistent branding metadata from a centralized configuration file. This ensures:

- Consistent company name across all plugins
- Uniform manufacturer codes for DAW recognition
- Proper developer attribution in source files
- Professional metadata in plugin binaries

## Configuration File

**Location:** `.claude/branding.json`

**Current Configuration:**

```json
{
  "company": {
    "full_name": "Ouaricon Audio",
    "short_name": "O-AUDIO",
    "manufacturer_code": "OuAu",
    "website": "https://ouaricon.audio",
    "copyright_year_start": 2025
  },
  "developer": {
    "name": "Taylor Brook",
    "role": "Developer"
  },
  "defaults": {
    "plugin_code_prefix": "Ou",
    "version_start": "1.0.0"
  },
  "metadata": {
    "description": "Professional audio plugins by Ouaricon Audio",
    "category": "Audio Effect / Instrument",
    "formats": ["VST3", "AU", "Standalone"]
  }
}
```

## How It Works

### During Plugin Creation

When you create a new plugin using `/implement`, the system:

1. **Reads branding.json** - The foundation-shell-agent automatically reads `.claude/branding.json`
2. **Extracts metadata** - Company name, manufacturer code, developer name, and other fields
3. **Injects into CMakeLists.txt** - Uses the values in the `juce_add_plugin()` macro
4. **Adds file headers** - Includes company and developer info in all source files

### What Gets Branded

#### CMakeLists.txt

```cmake
juce_add_plugin(PluginName
    COMPANY_NAME "Ouaricon Audio"              # From: company.full_name
    PLUGIN_MANUFACTURER_CODE OuAu              # From: company.manufacturer_code
    PLUGIN_CODE OuTr                           # From: defaults.plugin_code_prefix + unique suffix
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Plugin Name"
)
```

**Field Mapping:**
- `COMPANY_NAME` → `company.full_name`
- `PLUGIN_MANUFACTURER_CODE` → `company.manufacturer_code`
- `PLUGIN_CODE` → `defaults.plugin_code_prefix` + 2-character unique suffix

#### Source File Headers

All generated C++ files include a header with branding:

```cpp
/*
  ==============================================================================

    PluginName - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/
```

**Field Mapping:**
- Company name → `company.full_name`
- Developer → `developer.name`

### Plugin Codes

Each plugin receives a unique 4-character `PLUGIN_CODE`:

- **First 2 characters:** From `defaults.plugin_code_prefix` (e.g., "Ou")
- **Last 2 characters:** Derived from plugin name (e.g., "Tr" for Tremolo)

**Examples:**
- OuariconTremolo → `OuTr`
- GainKnob → `OuGk`
- TapeAge → `OuTa`

This ensures:
- All plugins are recognized as part of the Ouaricon Audio suite
- Each plugin has a unique identifier for DAW registration
- Codes are memorable and traceable to plugin names

## Manufacturer Codes

**PLUGIN_MANUFACTURER_CODE:** `OuAu` (Ouaricon Audio)

This 4-character code identifies the plugin manufacturer to DAWs and plugin hosts. It must be:
- Exactly 4 characters
- Unique to your company (registered with plugin hosts if commercial)
- Consistent across all plugins

**Current code:** `OuAu` is used for all Ouaricon Audio plugins.

## Updating Branding

### To Change Company Name

Edit `.claude/branding.json`:

```json
{
  "company": {
    "full_name": "New Company Name",
    ...
  }
}
```

All future plugins will use the new company name. Existing plugins will need to be rebuilt to reflect changes.

### To Change Manufacturer Code

**WARNING:** Changing the manufacturer code will cause DAWs to treat plugins as different products. Only change this if:
- You're rebranding the entire company
- You need to fix a conflict with another manufacturer's code
- You haven't released plugins commercially yet

```json
{
  "company": {
    "manufacturer_code": "NewC",
    ...
  }
}
```

### To Change Developer Name

```json
{
  "developer": {
    "name": "Your Name",
    "role": "Developer"
  }
}
```

This only affects file headers in newly generated code.

## Agent Integration

### foundation-shell-agent

The foundation-shell-agent (Stage 1) is responsible for:
1. Reading `.claude/branding.json` at the start of Stage 1
2. Extracting all branding fields
3. Using them in CMakeLists.txt generation
4. Adding file headers with branding to all source files
5. Reporting extracted branding in debug output

**Precondition:** The agent will validate that `branding.json` exists before proceeding. If missing, it will fail with a clear error message.

### plugin-workflow Orchestrator

The plugin-workflow skill passes the branding.json path to foundation-shell-agent:

```
**Contracts (read these files yourself):**
- ...
- branding.json: .claude/branding.json
- ...
```

This ensures the agent always has access to current branding configuration.

## Validation

### Build-Time Validation

The validation-agent (automatically run after Stage 1) checks:
- CMakeLists.txt has non-placeholder COMPANY_NAME
- PLUGIN_MANUFACTURER_CODE is exactly 4 characters
- PLUGIN_CODE is exactly 4 characters and includes the prefix
- Source files have proper headers with company/developer info

### Manual Verification

To verify branding in an existing plugin:

```bash
# Check CMakeLists.txt
grep -E "COMPANY_NAME|MANUFACTURER_CODE|PLUGIN_CODE" plugins/PluginName/CMakeLists.txt

# Check source headers
head -n 10 plugins/PluginName/Source/PluginProcessor.h
```

Expected output:
```
COMPANY_NAME "Ouaricon Audio"
PLUGIN_MANUFACTURER_CODE OuAu
PLUGIN_CODE OuXx
```

## Retroactive Branding

### Updating Existing Plugins

If you want to update branding on existing plugins (built before branding.json existed):

1. **Manual update** (recommended for learning):
   - Edit `CMakeLists.txt` to change company name and codes
   - Edit source file headers
   - Rebuild the plugin

2. **Rebuild from scratch** (if contracts exist):
   - Delete `Source/` directory
   - Delete `CMakeLists.txt`
   - Run `/continue PluginName` to regenerate with new branding

3. **Use /improve** (for multiple files):
   - Ask Claude to "update branding metadata in PluginName"
   - It will use the improve workflow to update all files

**Note:** Changing manufacturer/plugin codes may affect DAW plugin registration. Users may need to rescan plugins.

## Commercial Release Checklist

Before releasing plugins commercially:

- [ ] Verify `company.full_name` is your registered business name
- [ ] Verify `company.manufacturer_code` is registered (if required by plugin standards)
- [ ] Verify `company.website` points to your official site
- [ ] Verify `developer.name` is accurate
- [ ] Test that all plugins show correct company name in DAWs
- [ ] Verify copyright year in `company.copyright_year_start`
- [ ] Consider trademarking company name and plugin names

## FAQ

**Q: Can I use different branding for different plugins?**
A: Not automatically. The branding.json file applies to all plugins. If you need different branding (e.g., a separate product line), you would need to manually edit CMakeLists.txt after generation.

**Q: Will changing branding break existing plugins?**
A: Changing the company name and website is safe. Changing manufacturer codes or plugin codes may cause DAWs to treat plugins as new/different products.

**Q: Do I need to rebuild all plugins after updating branding.json?**
A: Only new plugins will use the updated branding automatically. Existing plugins keep their original branding until you manually update or rebuild them.

**Q: What if branding.json is missing?**
A: The foundation-shell-agent will fail with a clear error message. The file is required for all new plugin creation.

**Q: Can I add custom fields to branding.json?**
A: Yes, but they won't be used automatically. The agents only read the documented fields. You could manually reference custom fields in your plugin documentation.

## Technical Details

### File Format

`branding.json` uses standard JSON with these requirements:
- UTF-8 encoding
- Valid JSON syntax (no trailing commas, proper quoting)
- All documented fields must be present (agents expect them)
- Field types must match (strings for names, arrays for formats, etc.)

### Plugin Code Generation

The plugin code suffix is generated from the plugin name:
1. Remove spaces and special characters
2. Take the first character
3. Take the first character of the last word (or second character if single word)
4. Capitalize both
5. Prepend with prefix from `defaults.plugin_code_prefix`

**Examples:**
- "Gain Knob" → G + K → "Gk" → "OuGk"
- "Tremolo" → T + r → "Tr" → "OuTr"
- "Tape Age" → T + A → "Ta" → "OuTa"

### Validation Schema

While there's no formal JSON schema validation yet, agents expect this structure:

```typescript
interface BrandingConfig {
  company: {
    full_name: string;        // Required, non-empty
    short_name: string;       // Required, non-empty
    manufacturer_code: string; // Required, exactly 4 chars
    website: string;          // Required, valid URL format
    copyright_year_start: number; // Required, 4-digit year
  };
  developer: {
    name: string;             // Required, non-empty
    role: string;             // Required, non-empty
  };
  defaults: {
    plugin_code_prefix: string; // Required, exactly 2 chars
    version_start: string;      // Required, semver format
  };
  metadata: {
    description: string;        // Required, non-empty
    category: string;           // Required, non-empty
    formats: string[];          // Required, array of formats
  };
}
```

## References

- **Configuration:** `.claude/branding.json`
- **Agent prompt:** `.claude/agents/foundation-shell-agent.md`
- **Workflow reference:** `.claude/skills/plugin-workflow/references/stage-1-foundation-shell.md`
- **Plugin registry:** `PLUGINS.md`

---

**Last Updated:** 2026-01-06
**System Version:** Plugin Freedom System v1.0
