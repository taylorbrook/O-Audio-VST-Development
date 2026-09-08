# Superseded: the baked-geometry design (v1 planning, 2026-09-07)

Everything in this folder describes O-Strata as a *baked* geometry wavetable generator (mesh slicing, SDF/fBm volume orbits, terrain orbit sweeps → 256-frame tables). It was superseded on 2026-09-08 after a listening pass: the baked tables sound good but do not justify a separate plugin, so they ship as an O-Prism factory bank (`plugins/O-Prism/.planning/improvements/geometry-wavetables.md`), and O-Strata was re-planned around the **live wave-terrain oscillator** (research §7.2). Evidence: `../evidence/critique-check-2026-09-08.md`, `../evidence/replan-proposal-2026-09-08.md`.

Kept for history and for anything worth lifting into v1.1's baked source type: the ARCHITECTURE (mesh slicer algorithm, scheduler contract, persistence design), ROADMAP, locked parameter-spec v1 (219 params), mockup v1 and its scaffolding, and the Stage 0 / Stage 2 CONTEXT files. `../stages/1-foundation/` is **not** superseded — it records the fork that exists in `Source/` and is the base for the re-parameterise pass.

Do not plan from these files. The live design lives in `../BRIEF.md`, `../REQUIREMENTS.md`, `../parameter-spec-draft.md`.
