# Changelog

All notable changes to O-FreqPulse will be documented in this file.

## [1.1.1] - 2026-02-04

### Fixed

- **Playhead no longer moves when no audio is present** - Added input RMS detection (~-60 dB threshold) that gates playhead advancement in both host-sync and standalone modes. Playhead fades out when signal drops below threshold and reappears when audio returns.

### Technical Notes

- Root cause: playhead step counter advanced unconditionally in processBlock regardless of input signal level
- RMS computed per buffer across all channels; threshold at 0.001f (~-60 dB)
- Signal state communicated to WebView via existing timer callback (30 Hz)
- Playhead opacity animated with 150ms CSS ease transition for smooth fade

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
