---
slug: readme-v15-microtonal
date: 2026-04-27
type: quick
status: complete
---

# Summary: README v1.5 Milestone Update

## What changed

`README.md` — two additions, brief and factual:

1. **New "Microtonal Dorico Playback (v1.5)" subsection** (under Modern Interface Design). Explains the `note-expression` v1.1.0 shared module, the 8 cohort consumer plugins, and the one-time Dorico Library Manager import flow for `Ouaricon-VST3-NoteExpression.doricolib`.
2. **Implementation Status — added Milestone History table** with v1.0–v1.5 rows. v1.5 row highlights the Microtonal Shared Module & Suite Propagation theme. Bumped system status footnote to reflect 25 phases / 6 milestones / 141 requirements.

## Why this scope

User asked for brief + clear. README previously had no mention of Dorico/microtonal capability and Implementation Status stopped at v1.0. The two-edit approach gets the new capability into the visible "Key Features" zone without restructuring the doc.

## Source verification

- `note-expression` module v1.1.0 confirmed in `modules/tuning/note-expression/module.yaml`
- 8-plugin cohort confirmed in `.planning/PROJECT.md`
- v1.5 ship date (2026-04-27) confirmed in `.planning/STATE.md` and `PROJECT.md`
- Doricolib distribution mechanism confirmed in `PROJECT.md` ("user activates via one-time Dorico Library Manager Import")

## Files touched

- `README.md` (+19 lines)
- `.planning/quick/260427-readme-v15-microtonal/PLAN.md` (new)
- `.planning/quick/260427-readme-v15-microtonal/SUMMARY.md` (new)
- `.planning/STATE.md` (Quick Tasks Completed table appended)
