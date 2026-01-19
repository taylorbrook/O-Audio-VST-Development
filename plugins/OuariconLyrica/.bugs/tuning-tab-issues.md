# Tuning Tab Issues

**Created:** 2026-01-19
**Status:** ✅ Resolved (v1.7.3)
**Priority:** Medium

## Fixed in v1.7.2
- Tuning mode buttons (12-TET, Custom, MTS-ESP) now clickable and functional

## Fixed in v1.7.3
- All Scala/KBM file buttons now working (Load .SCL, Load .KBM, Save .SCL, Save .KBM)
- Interval list editing now functional in Custom mode
- Tonic arrows now respond to clicks

## Root Cause

Same issue as mode buttons in v1.7.2: ES6 module `addEventListener` calls were not attaching properly in JUCE's WebView. Additionally, an undefined `scalaFileLoaded` variable was causing a JavaScript error that silently broke the handler.

## Resolution

Applied the same fix pattern as v1.7.2:
1. Global handler functions defined in non-module script (handleLoadSCL, handleLoadKBM, handleSaveSCL, handleSaveKBM, handleIntervalChange, handleTonicDown, handleTonicUp)
2. Inline `onclick`/`onchange` attributes on buttons and inputs
3. Module sets the global functions which are then called by inline handlers

## Files Involved
- `Resources/ui/index.html` - UI and JavaScript
- `Source/PluginEditor.cpp` - Native functions for file dialogs
