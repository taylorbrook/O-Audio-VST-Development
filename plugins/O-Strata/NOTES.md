# O-Strata Notes

## Status
- **Current Status:** 🚧 Stage 1 (Foundation)
- **Version:** 1.0.0 (unreleased)
- **Type:** Synth (3D-Geometry Microtonal Wavetable)

## Lifecycle Timeline

- 2026-09-07 — Research (Level 2 + Level 3): `research/wavetable-synthesis-3d-geometry.md`
- 2026-09-07 — Ideation complete: BRIEF.md + REQUIREMENTS.md
- 2026-09-07 — Stage 0 complete: ARCHITECTURE.md + ROADMAP.md; mockup v1 finalized; parameter-spec.md locked (219 params)
- 2026-09-07 — Stage 1 (Foundation) executed: forked O-Prism v1.24.0 (commit e88ec412) into `plugins/O-Strata/` as an independent plugin (`OuSt`, VERSION 1.0.0, every `Prism` identifier renamed); wavetable library removed (`WavetableFactory`, `UserWavetableManager`, `WavetableImporter`, `WavetableEditor`, `oscATable`/`oscBTable`, 14 native functions, wavetable tab); 48 inert geometry parameters added; `juce_cryptography` linked; both oscillators play the sine placeholder via `oscTablePtr[]`. Details: `.planning/stages/1-foundation/SUMMARY.md`

## Known Issues

- All 96 factory presets were authored against O-Prism's wavetable library and now play the sine placeholder — re-authored in Stage 4.1.
- `stereoWidth` has no UI binding (inherited from O-Prism).

## Additional Notes

- Fork base: O-Prism v1.24.0 (engine, UI shell, tuning engine, preset manager, i18n).
- v1.0 is baked-only. Live wave-terrain oscillator (Chebyshev bandlimited mode) planned for v1.1.
- Research roadmap stage numbers (§7.4) do not map 1:1 onto workflow stages — see BRIEF.md Technical Notes.
