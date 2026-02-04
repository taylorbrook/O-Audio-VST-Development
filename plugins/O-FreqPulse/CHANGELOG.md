# Changelog

All notable changes to O-FreqPulse will be documented in this file.

## [1.1.0] - 2026-02-04

### Added

- **Clear button per lane** - Resets all 32 steps in a band to OFF state
- **Random button per lane** - Fills steps with 50% probability pattern
- Both buttons appear in each band row between the step grid and mode indicator

### Technical Notes

- Lane actions use Unicode symbols for compact display (⌀ for clear, ⚄ for random)
- Random pattern applies to all 32 steps regardless of current step count setting
- Parameter changes propagate immediately to JUCE backend via existing toggle bindings

## [1.0.0] - 2026-02-04

### Added

- Initial release
- 4-band spectral step sequencer (SUB, LOW, MID, HIGH)
- 32-step grid with variable step counts (4, 8, 16, 32)
- Euclidean rhythm generator per band
- Tempo-synced playback with swing
- WebView UI with naturalist aesthetic
- 12 factory presets
