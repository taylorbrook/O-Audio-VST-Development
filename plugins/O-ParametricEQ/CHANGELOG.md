# O-ParametricEQ Changelog

## [0.5.0] - 2026-11-14

Stage 2, Phase 2.5 (output stage and auto gain).

### Added
- `output_gain` (±36 dB) and `auto_gain` work. Both move without clicks, and both are in effect from the first sample of a session and from the first sample after un-bypass. Nothing fades in.
- Auto gain follows the EQ curve, not the programme: it pulls the level back by the curve's average boost (or cut) over 40 Hz – 16 kHz, within −18 … +12 dB. Renders and null tests repeat exactly, offline or in real time.

### Changed
- Global bypass now crossfades whenever the output gain or auto gain is doing something, also with no band on.

### Known in this build
- A steep low cut switched in under sustained bass can thump at a slope change (under listening).

## [0.4.0] - 2026-11-14

Stage 2, Phase 2.4 (structural crossfades and bypass). Switching things no longer clicks.

### Changed
- Type, slope, placement, enable and band bypass changes crossfade over 20 ms and no longer click. A change made during a crossfade waits for it to end; the latest one wins.
- Global `bypass` crossfades to the dry signal over 20 ms, and the engine rests while bypassed. The host's own bypass route takes the same path.
- A band at exactly 0 dB costs nothing and leaves the signal bit-identical: 50 ms after its gain arrives at 0 dB it drops out of the processing, and with all bands at 0 dB (or `gain_scale` at 0 %) the output equals the input.

### Fixed
- An edit made while bypassed is in effect, settled, at un-bypass. Before, a frequency, gain or Q edit made during bypass ramped in for 20 ms afterwards.

### Known in this build
- `output_gain` and `auto_gain` do nothing yet.
- A steep low cut switched in under sustained bass dips for a few milliseconds.

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
