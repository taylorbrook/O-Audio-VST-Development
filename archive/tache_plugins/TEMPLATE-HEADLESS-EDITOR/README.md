# Headless Editor Template

This template provides a minimal `AudioProcessorEditor` for plugins that use DAW-provided parameter controls instead of custom UI.

## Usage

This template is automatically applied when user chooses "Ship headless" during Stage 3 → Stage 4 decision gate.

## Files

- `PluginEditor.h` - Minimal editor with plugin name display
- `PluginEditor.cpp` - Empty (all logic inline in header)

## Placeholders

- `[PluginName]` - Replaced with actual plugin name during generation

## Customization

Users can modify the `paint()` method to customize the minimal window appearance:
- Change background color
- Adjust text size/font
- Add version number or description

## Upgrading to WebView UI

Users can add custom UI later via `/improve [PluginName]` → "Create custom UI" option.
