# Tuning Tab Issues

**Created:** 2026-01-19
**Status:** Open
**Priority:** Medium

## Fixed in v1.7.2
- Tuning mode buttons (12-TET, Custom, MTS-ESP) now clickable and functional

## Remaining Issues

### 1. Scala/KBM File Loading Not Working
- Load .SCL button does not load files
- Load .KBM button does not load files
- Save .SCL button does not work
- Save .KBM button does not work

### 2. Interval List Not Functioning
- Interval inputs may not be editable in Custom mode
- Changes to intervals may not be applied

## Technical Notes

The mode button fix required using inline `onclick` handlers instead of `addEventListener` in the ES6 module. The module's event listeners were not attaching properly (root cause unknown - possibly a JUCE WebView quirk with ES6 modules).

The fix uses:
1. A global `window.handleModeClick()` function defined in a non-module script
2. Inline `onclick` attributes on the buttons
3. A module callback `window.onTuningModeChanged()` for JUCE sync

## Files Involved
- `Resources/ui/index.html` - UI and JavaScript
- `Source/PluginEditor.cpp` - Native functions for file dialogs
