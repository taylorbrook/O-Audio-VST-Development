# AutoClip Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.1
- **Type:** Audio Effect (Hard Clipper)
- **Complexity:** 4.4

## Lifecycle Timeline

- **2025-11-13:** Creative brief completed
- **2025-11-13:** Draft parameters captured (2 parameters)
- **2025-11-13 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 4.4)
- **2025-11-13 (Stage 2):** Foundation complete - Build system operational, APVTS with 2 parameters
- **2025-11-13 (Stage 3):** DSP complete - Hard clipping, lookahead buffer, gain matching, clip solo (all 3 phases)
- **2025-11-13 (Stage 4):** GUI complete - WebView UI with meters, parameter binding (all 3 phases)
- **2025-11-13 (Stage 5):** Validation complete - 5 factory presets, CHANGELOG.md generated
- **2025-11-13 (v1.0.0):** Installed to system folders (VST3 + AU)
- **2025-11-15 (v1.0.1):** Fixed frozen WebView UI controls - Added missing check_native_interop.js

## Known Issues

None (v1.0.1 fixed UI interaction issue)

## Additional Notes

**Description:**
Hard clipper with automatic peak-based gain matching designed for drum processing. Maintains consistent perceived loudness as clipping intensity increases through real-time peak analysis and gain compensation. Fixed 5ms lookahead ensures smooth transient anticipation without perceptible latency.

**Parameters (2 total):**
- clipThreshold: 0-100%, default 0% (0% = no clipping, 100% = maximum clipping)
- soloClipped: Off/On, default Off (output only clipped portion for monitoring)

**DSP:** Hard clipping with peak-based automatic gain compensation. Fixed 5ms lookahead buffer for transient anticipation. Gain smoothing (50ms) prevents zipper noise. Clip solo (delta monitoring) outputs difference signal for artifact inspection.

**GUI:** Vintage Bakelite aesthetic (300×500px). Large threshold knob, clip solo toggle, input/output meters with ballistic motion, clipping indicator. WebView UI with ES6 modules and two-way parameter binding.

**Validation:**
- ✓ Factory presets: 5 presets created (Default, Subtle Clip, Moderate Clip, Heavy Clip, Extreme Clip)
- ✓ CHANGELOG.md: Generated in Keep a Changelog format
- ✓ Build verification: Compiles successfully (VST3, AU) - 4.1MB binaries
- ✓ Installation verified: Both formats installed to system folders

**Formats:** VST3, AU, Standalone

**Installation Locations:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/AutoClip.vst3` (4.1 MB)
- AU: `~/Library/Audio/Plug-Ins/Components/AutoClip.component` (4.1 MB)
