# O-Strata Notes

## Status
- **Current Status:** 🚧 Stage 4 (Polish) — Round A (Phase 4.1) executed, Round B (Phase 4.2) next
- **Version:** 1.0.0 (unreleased)
- **Type:** Synth (Microtonal Wave Terrain)

## Lifecycle Timeline

- 2026-09-07 — Research (Level 2 + Level 3): `research/wavetable-synthesis-3d-geometry.md`
- 2026-09-07 — Ideation complete: BRIEF.md + REQUIREMENTS.md
- 2026-09-07 — Stage 0 complete: ARCHITECTURE.md + ROADMAP.md; mockup v1 finalized; parameter-spec.md locked (219 params)
- 2026-09-07 — Stage 1 (Foundation) executed: forked O-Prism v1.24.0 (commit e88ec412) into `plugins/O-Strata/` as an independent plugin (`OuSt`, VERSION 1.0.0, every `Prism` identifier renamed); wavetable library removed (`WavetableFactory`, `UserWavetableManager`, `WavetableImporter`, `WavetableEditor`, `oscATable`/`oscBTable`, 14 native functions, wavetable tab); 48 inert geometry parameters added; `juce_cryptography` linked; both oscillators play the sine placeholder via `oscTablePtr[]`. Details: `.planning/stages/1-foundation/first-pass-baked/SUMMARY.md`
- 2026-09-08 — Design v2 (live wave-terrain oscillator): the baked-geometry design is superseded (`.planning/superseded-baked-v1/`); ARCHITECTURE / ROADMAP / REQUIREMENTS v2
- 2026-09-10 — Mockup v2 finalised + scaffolded; `parameter-spec.md` v2 locked (205 params, 46 mod destinations)
- 2026-09-11 / 12 — Stage 2 (DSP, live wave-terrain oscillator) VERIFIED in two rounds (`stages/2-dsp/`); Stage 3 (GUI: Terrain tab, 3D view, PNG import) VERIFIED in two rounds (`stages/3-gui/`)
- 2026-09-12 — Stage 4 Round A (Phase 4.1) executed: `terrainImports` persistence (bytes ≤ 2 MiB inline / path form above, SHA-256 checked), Sine Product fallback for a missing source + Locate…, the 18-preset factory bank under a content stamp, preset-manager v1.0.7. Details: `.planning/stages/4-polish/SUMMARY.md`
- 2026-09-10 — Stage 1 second pass (re-parameterise) executed: 48 baked params → 34 live-oscillator params (205 total), mod destinations 26 → 46, `terrainImports` state child, 8 combo relays, factory bank reset to a single `Init`, COMPAT-01 re-verified. Details: `.planning/stages/1-foundation/SUMMARY.md`

## Known Issues

- ~~Factory bank is `Init` only until Phase 4.1~~ — closed 2026-09-12: 18 presets, and the constructor sweeps `Factory/` on a content-stamp mismatch (`1.0.0+<sha256(bank)[0:12]>`), so orphans cannot survive a bank edit.
- `stereoWidth` has no UI binding (inherited from O-Prism).
- A preset's imported PNG above 2 MiB is linked by absolute path; on another machine it opens with the "Source missing" notice and Locate…. A preset without an image clears both terrain slots on load.
- Human rows still open: Stage 3's six rows (`stages/3-gui/VERIFICATION.md` §Human) and the Round A rows of `stages/4-polish/SUMMARY.md` §Hands-on checklist (Locate… by hand, a PNG preset through Logic save / reload, the live restore re-push, the dropdown order, a first listen to the bank). The listening pass is Round B.

## Additional Notes

- Fork base: O-Prism v1.24.0 (engine, UI shell, tuning engine, preset manager, i18n).
- v1.0 is the live wave-terrain oscillator (design v2, 2026-09-08); baked sources (the v1 mesh / volume / terrain bakes) are v1.1.
- Research roadmap stage numbers (§7.4) do not map 1:1 onto workflow stages — see BRIEF.md Technical Notes.
