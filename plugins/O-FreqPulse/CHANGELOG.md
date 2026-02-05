# Changelog

All notable changes to O-FreqPulse will be documented in this file.

## [1.1.2] - 2026-02-04

### Fixed

- **Euclidean patterns now display on the grid** - When Euclidean mode is enabled for a band, the generated pattern visually populates the step grid cells with a distinct warm brown color. Previously, the C++ processor computed Euclidean patterns internally but never communicated them to the UI, so the grid always showed the stale manual pattern.

### Technical Notes

- Root cause: `euclideanPatterns[band]` array in C++ was used for audio processing but never written to `step_b{N}_s{M}` parameters or sent to the WebView
- Fix is UI-only: Bresenham euclidean algorithm replicated in JavaScript (`generateEuclidean()`)
- When euclidean mode is active: grid shows computed pattern, manual step clicking is disabled, clear/random buttons are blocked
- When euclidean mode is off: manual step parameters are restored to display, clicking re-enabled
- Manual patterns are preserved in step parameters and never overwritten by euclidean mode
- Euclidean grid updates reactively when euc_steps, euc_pulses, or euc_offset parameters change

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
