# O-ParametricEQ Changelog

## [0.3.0] - 2026-11-13

Stage 2, Phase 2.3 (continuous modulation). Parameter moves are smoothed.

### Changed
- Frequency, gain, Q and `gain_scale` moves are smoothed over 20 ms and no longer step. The filter keeps ringing through the move; a band that has arrived is bit-identical to one set there directly.
- The smoothing runs on a fixed grid counted from the start of playback (16, 32 or 64 samples by sample rate), so a move renders the same whatever the host's block size.
- A gain or `gain_scale` edit on a Low Cut, High Cut, Notch or Band Pass does no work.

### Fixed
- A non-finite input sample (NaN or infinity) no longer leaves the filters stuck: they are cleared at the end of the block and the output recovers from the next one.

### Known in this build
- Type, slope, placement and enable changes click: the band restarts from rest.
- Global `bypass` is a hard switch.
- `output_gain` and `auto_gain` do nothing yet.

## [0.2.0] - 2026-11-13

Stage 2, Phase 2.2 (static engine). The plugin equalises; parameter changes are not smoothed yet.

### Added
- The 24-band engine: eight types (Bell, Low Shelf, High Shelf, Low Cut, High Cut, Notch, Band Pass, Tilt Shelf), slopes from 6 to 96 dB/oct on cuts, shelves and tilt, five placements (Stereo, Left, Right, Mid, Side), `gain_scale` applied per band. Zero latency.
- Bit-exact output with no active band. A Left or Right band leaves the other channel bit-exact.
- 2 s tail reported to the host.

### Known in this build
- Parameter moves step: a band is redesigned at the next block start, with no ramp.
- Type, slope, placement and enable changes click: the band restarts from rest.
- Global `bypass` is a hard switch.
- `output_gain` and `auto_gain` do nothing yet.

## [0.1.0] - 2026-11-12

Stage 1 (Foundation). The plugin builds, loads and saves; it does not equalise yet.

### Added
- VST3, AU and Standalone targets (`aufx OuPQ OuDv`), mono and stereo layouts, zero latency.
- 196 host parameters in 25 groups: a global block (`bypass`, `output_gain`, `gain_scale`, `auto_gain`) and 24 bands (`b01_…` to `b24_…`: enabled, bypass, type, freq, gain, Q, slope, placement). Every parameter carries version hint 1. The 72 dynamic-EQ IDs (`bNN_dyn_*`) are reserved and not registered.
- `bypass` is the host bypass parameter.
- State format v1: `stateVersion`, `pluginVersion` and `uiLanguage` on the root, one `UI` child (analyzer mode, display range, selected band, editor size). A state with an unknown parameter, a missing parameter or a later `stateVersion` loads; a blob that is not ours changes nothing.
- Render harness (`tests/render-harness`) and a committed parameter-layout golden (`tests/golden/params.tsv`), tied to `parameter-spec.md` by `tests/golden/check_spec.py`.

### Notes
- Audio passes through bit-exact. DSP arrives in Stage 2.
- The editor is JUCE's generic parameter list until Stage 3.
