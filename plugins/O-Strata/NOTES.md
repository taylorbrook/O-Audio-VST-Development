# O-Strata Notes

## Status
- **Current Status:** 🚧 Stage 1 (Foundation)
- **Version:** 1.0.0 (unreleased)
- **Type:** Synth (Microtonal Wave Terrain)

## Lifecycle Timeline

- 2026-09-07 — Research (Level 2 + Level 3): `research/wavetable-synthesis-3d-geometry.md`
- 2026-09-07 — Ideation complete: BRIEF.md + REQUIREMENTS.md
- 2026-09-07 — Stage 0 complete: ARCHITECTURE.md + ROADMAP.md; mockup v1 finalized; parameter-spec.md locked (219 params)
- 2026-09-07 — Stage 1 (Foundation) executed: forked O-Prism v1.24.0 (commit e88ec412) into `plugins/O-Strata/` as an independent plugin (`OuSt`, VERSION 1.0.0, every `Prism` identifier renamed); wavetable library removed (`WavetableFactory`, `UserWavetableManager`, `WavetableImporter`, `WavetableEditor`, `oscATable`/`oscBTable`, 14 native functions, wavetable tab); 48 inert geometry parameters added; `juce_cryptography` linked; both oscillators play the sine placeholder via `oscTablePtr[]`. Details: `.planning/stages/1-foundation/first-pass-baked/SUMMARY.md`
- 2026-09-08 — Design v2 (live wave-terrain oscillator): the baked-geometry design is superseded (`.planning/superseded-baked-v1/`); ARCHITECTURE / ROADMAP / REQUIREMENTS v2
- 2026-09-10 — Mockup v2 finalised + scaffolded; `parameter-spec.md` v2 locked (205 params, 46 mod destinations)
- 2026-09-10 — Stage 1 second pass (re-parameterise) executed: 48 baked params → 34 live-oscillator params (205 total), mod destinations 26 → 46, `terrainImports` state child, 8 combo relays, factory bank reset to a single `Init`, COMPAT-01 re-verified. Details: `.planning/stages/1-foundation/SUMMARY.md`

## Known Issues

- Factory bank is `Init` only until Phase 4.1 (the inherited O-Prism presets were dropped in the Stage 1 second pass). The preset manager never deletes orphaned factory files — Phase 4.1 must clear the old bank itself when it re-stamps `.factory-version`.
- `stereoWidth` has no UI binding (inherited from O-Prism).

## Additional Notes

- Fork base: O-Prism v1.24.0 (engine, UI shell, tuning engine, preset manager, i18n).
- v1.0 is the live wave-terrain oscillator (design v2, 2026-09-08); baked sources (the v1 mesh / volume / terrain bakes) are v1.1.
- Research roadmap stage numbers (§7.4) do not map 1:1 onto workflow stages — see BRIEF.md Technical Notes.
