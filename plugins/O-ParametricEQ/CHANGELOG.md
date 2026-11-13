# O-ParametricEQ Changelog

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
