---
slug: readme-v15-microtonal
date: 2026-04-27
type: quick
status: in-progress
---

# Quick Task: README v1.5 Milestone Update

## Goal

Update README.md to reflect v1.5 milestone completion (Microtonal Shared Module & Suite Propagation, shipped 2026-04-27). Brief, clear info on the new `note-expression` shared module and how it works with Dorico.

## Source of Truth

- `.planning/PROJECT.md` — v1.5 shipped, 8 cohort plugins, Dorico Library Manager flow
- `.planning/STATE.md` — milestone status, key decisions
- `modules/tuning/note-expression/module.yaml` — module v1.1.0, public API
- `modules/registry.yaml` — module registry

## Edits

1. **Implementation Status section** (line 842): add v1.5 milestone row to status table; bump system status note.
2. **Key Features section** (after Modern Interface Design): add a short "Microtonal Dorico Playback" subsection — what the module does, the 8 plugins that consume it, and the one-time `Ouaricon-VST3-NoteExpression.doricolib` import step.

## Constraints

- Brief — no walls of text.
- Factual — only claims backed by PROJECT.md/STATE.md/module.yaml.
- Don't disturb existing structure or unrelated sections.
