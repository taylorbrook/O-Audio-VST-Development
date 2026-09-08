# O-Strata Changelog

## v1.0.0 (unreleased)

**Foundation.** O-Strata is forked from O-Prism v1.24.0 (commit `e88ec412`,
2026-09-07) as a new, independent plugin: `PLUGIN_CODE OuSt`, product name
`O-Strata`, APVTS state identifier `OStrataParameters`, its own preset folder.
O-Prism is untouched and the two plugins coexist in a host.

Stage 1 (this entry):
- Removed the wavetable library: `WavetableFactory`, `UserWavetableManager`,
  `WavetableImporter`, `WavetableEditor`, the `oscATable`/`oscBTable`
  parameters, the user-wavetable state child, 14 wavetable native functions,
  the wavetable tab/editor/selector UI and their i18n rows. Both oscillators
  read a published table pointer (`oscTablePtr[]`), initialised to the sine
  placeholder; the retire/reaper machinery stays for Phase 2.1's bake scheduler.
- Added the 48 geometry parameters of `parameter-spec.md` v1 (24 per
  oscillator: Source/Frames/Shape Drive, Mesh ×6, Volume ×5, Terrain ×10).
  Inert in Stage 1 — automatable, persisted, not mod-matrix destinations.
  219 parameters total.
- `delayDivision` gains a slider relay (it was bound in O-Prism's page but
  never followed host automation).
- Linked `juce_cryptography` (SHA-256 for Stage 4.1 path-linked imports).
- Factory presets carried over with their table entries stripped; every preset
  now plays the sine placeholder and is re-authored in Stage 4.1.
