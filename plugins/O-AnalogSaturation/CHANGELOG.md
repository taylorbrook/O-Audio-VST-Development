# Changelog

All notable changes to O-AnalogSaturation will be documented in this file.

## [1.1.0] - 2026-01-24

### Changed
- Renamed plugin from "OuariconSaturationModeling" to "O-AnalogSaturation"
- Updated class names: `OuariconSaturationModelingAudioProcessor` → `OAnalogSaturationAudioProcessor`
- New plugin code: OaSa (was OsSM)
- Consistent branding with O-series plugins (O-Tremolo, O-Comp, O-AnalogEQ, O-DigiDelay)

**Note:** Parameter IDs unchanged - existing presets and automation compatible.

## [1.0.1] - 2026-01-14

### Fixed
- Snake PNG opacity now transitions smoothly with intensity knob movement
- Opacity no longer snaps back when releasing the knob

**Root Cause:** Visual updates were triggered twice per frame during drag - once directly in the mousemove handler and once via the `valueChangedEvent` listener. When the two values differed slightly, it caused jitter and snap-back on release.

**Fix:** Removed direct `updateKnobVisual()` call from mousemove handler. The `valueChangedEvent` listener is now the single source of truth for visual updates, allowing the CSS transition to work properly.

## [1.0.0] - 2026-01-09

### Added
- Initial release
- Four saturation models: Magnetic, Tube, Transformer, Diode
- Intensity knob with visual feedback
- Input/Output VU meters
- Quality settings (Low, Mid, High)
- Autogain toggle
- Vintage botanical illustration theme with snake imagery
