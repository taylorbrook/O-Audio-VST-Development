# Roadmap: Plugin Freedom System

## Milestones

- ✅ **v1.0 MVP** -- Phases 1-7 (shipped 2026-02-01)
- ✅ **v1.1 Cleanup & Workflow Polish** -- Phases 8-9 (shipped 2026-02-02)
- ✅ **v1.2 Agent Intelligence & Resource Orchestration** -- Phases 10-13 (shipped 2026-02-06)
- ✅ **v1.3 System Modernization** -- Phases 14-17 (shipped 2026-02-10)
- ✅ **v1.4 System Hygiene & Quality Gates** -- Phases 18-22 (shipped 2026-03-07)
- 🚧 **v1.5 Microtonal Shared Module & Suite Propagation** -- Phases 23-25 (in progress)

## Phases

<details>
<summary>✅ v1.0 MVP (Phases 1-7) -- SHIPPED 2026-02-01</summary>

- [x] Phase 1: Agent Contracts (3 plans) -- completed
- [x] Phase 2: State Management (4 plans) -- completed
- [x] Phase 3: Structured Handoffs (2 plans) -- completed
- [x] Phase 4: Verification Infrastructure (2 plans) -- completed
- [x] Phase 5: Quality Gates (3 plans) -- completed
- [x] Phase 6: Domain Specialization (3 plans) -- completed
- [x] Phase 7: Module System (4 plans) -- completed

</details>

<details>
<summary>✅ v1.1 Cleanup & Workflow Polish (Phases 8-9) -- SHIPPED 2026-02-02</summary>

- [x] Phase 8: Repository Cleanup (2 plans) -- completed
- [x] Phase 9: Workflow Planning Phase (2 plans) -- completed

</details>

<details>
<summary>✅ v1.2 Agent Intelligence & Resource Orchestration (Phases 10-13) -- SHIPPED 2026-02-06</summary>

- [x] Phase 10: Resource Discovery (3 plans) -- completed
- [x] Phase 11: Context Injection (3 plans) -- completed
- [x] Phase 12: Accountability (3 plans) -- completed
- [x] Phase 13: Self-Maintenance (3 plans) -- completed

</details>

<details>
<summary>✅ v1.3 System Modernization (Phases 14-17) -- SHIPPED 2026-02-10</summary>

- [x] Phase 14: Platform Alignment (4 plans) -- completed 2026-02-08
- [x] Phase 15: Context Persistence (4 plans) -- completed 2026-02-09
- [x] Phase 16: GSD Deduplication (3 plans) -- completed 2026-02-09
- [x] Phase 17: Agent Intelligence (3 plans) -- completed 2026-02-10

</details>

<details>
<summary>✅ v1.4 System Hygiene & Quality Gates (Phases 18-22) -- SHIPPED 2026-03-07</summary>

- [x] Phase 18: Dead Code Removal (4 plans) -- completed 2026-03-06
- [x] Phase 19: Quality Gate Activation (1 plan) -- completed 2026-03-06
- [x] Phase 20: Research Governance & Review (3 plans) -- completed 2026-03-07
- [x] Phase 21: Skill & Infrastructure Consolidation (3 plans) -- completed 2026-03-07
- [x] Phase 22: Structural Improvements (2 plans) -- completed 2026-03-07

</details>

### 🚧 v1.5 Microtonal Shared Module & Suite Propagation (In Progress)

**Milestone Goal:** Promote the validated VST3 Note Expression pattern (O-Lyrica spikes 001–003) into a shared Ouaricon module and propagate Dorico microtonal playback across all pitched plugins, with every per-plugin rollout tracked through the standard `/improve` workflow.

- [x] **Phase 23 (A): Extract** -- Build shared microtonal module from cleaned spike code; prove it on O-Lyrica as the reference consumer (completed 2026-04-25)
- [ ] **Phase 24 (B): Propagate** -- Apply the module to the remaining 7 pitched plugins via `/improve`, each with version bump, changelog, and regression test
- [ ] **Phase 25 (C): Package & Internal Technical Notes** -- Bundle the Dorico expression map in every affected plugin's installer and capture internal developer-reference notes

## Phase Details

### Phase 23 (A): Extract
**Goal**: A new shared Ouaricon microtonal module exists, is registered in the module system, and is proven as a working consumer integration in O-Lyrica — replacing the embedded spike code with module-based consumption while composing cleanly with O-Lyrica's existing `TuningEngine`.
**Depends on**: Phase 22 (v1.4 shipped); spike-findings-VST-development skill (implementation bible)
**Requirements**: MOD-01, MOD-02, MOD-03, MOD-04, MOD-05, MOD-06, MOD-07, MOD-08, LYR-01, LYR-02, LYR-03, LYR-04
**Success Criteria** (what must be TRUE):
  1. The new microtonal module appears in `/module-list` with a semver version, and `/module-info [name]` shows its description, contents, and consumer integration steps.
  2. O-Lyrica builds cleanly after `/module-add [module-name]` with no remaining `detail::neTrace` / `detail::iidToHex` call sites or stray `#include <fstream>` in module or O-Lyrica sources (diagnostic spike code fully stripped).
  3. O-Lyrica passes the Dorico quarter-sharp smoke test after refactor — pitch lands at +50¢ above C4 with NE events correlated by `noteId`, no attack zipper, and the existing `TuningEngine` still composes with NE offsets (no raw `pow()` bypass).
  4. O-Lyrica `CHANGELOG.md` has a version-bumped entry documenting shared-module adoption and microtonal NE support.
  5. The local JUCE patch is committed as a named patch file in `scripts/` (or equivalent) with a documented re-apply procedure for JUCE-version bumps.
**Plans:** 5/5 plans complete

Plans:
- [x] 23-01-module-scaffolding-PLAN.md — Create note-expression module (yaml/README stub/cpp/NoteExpression.h) + registry entry (MOD-01/02/03/04/06/08) -- completed 2026-04-25
- [x] 23-02-juce-patch-tooling-PLAN.md — Committable .patch file + idempotent apply-juce-patches.sh + CMake-time marker check via module.cmake (MOD-07) -- completed 2026-04-25
- [x] 23-03-olyrica-consume-refactor-PLAN.md — Wire ouaricon_add_module(OLyrica note-expression); refactor PluginProcessor + HarpSynthVoice; delete VST3/NoteExpressionSupport.h; clean build (LYR-01/02) -- completed 2026-04-25
- [x] 23-04-version-readme-dorico-smoketest-PLAN.md — VERSION 2.3.0 + CHANGELOG entry + comprehensive module README + clean install + Dorico quarter-sharp smoke test (MOD-05, LYR-03/04) -- completed 2026-04-25 (Dorico VST3 5/5 PASS; AU re-link failure exposed, deferred to Plan 05)
- [x] 23-05-fix-au-link-steinberg-symbols-PLAN.md — Two-TU split + custom-deleter pimpl + dual dispatch slots restore clean OLyrica_VST3 + AU + Standalone link; per-format module-source convention added to OuariconModules.cmake; reusable scripts/verify-au-link.sh AU gate; LYR-03 5/5 re-passed via VST3. Phase 24 unblocked. -- completed 2026-04-25

### Phase 24 (B): Propagate
**Goal**: The shared microtonal module is consumed by all 7 remaining pitched plugins (O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant) via the standard `/improve [PluginName]` workflow — each rollout produces a version bump, CHANGELOG entry, STATUS.md update, regression test, and a fresh system install, with zero direct source edits bypassing the tracked improvement cycle.
**Depends on**: Phase 23 (module must exist and be proven on O-Lyrica before other plugins consume it)
**Requirements**: PROP-01, PROP-02, PROP-03, PROP-04, PROP-05, PROP-06, PROP-07, TRACK-01, TRACK-02, TRACK-03, TRACK-04, TRACK-05
**Success Criteria** (what must be TRUE):
  1. All 7 target plugins (O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant) appear as consumers of the microtonal module in `/module-list --consumers [module]` (or equivalent module-registry query).
  2. Each of the 7 plugins passes the Dorico quarter-sharp smoke test — pitch lands at +50¢ above C4, no attack zipper, NE correlated by `noteId`.
  3. Every Phase 24 rollout is traceable to an `/improve [PluginName]` cycle: each plugin has a version bump applied in `CMakeLists.txt`, a CHANGELOG entry naming "adds VST3 Note Expression microtonal support for Dorico", and a STATUS.md update — with no direct source edits that bypass plugin-level versioning/changelog/state tracking (TRACK-01 enforced).
  4. All 8 affected plugins (the 7 above plus O-Lyrica from Phase 23) are rebuilt and freshly installed per CLAUDE.md rules — AU cache cleared, old bundles removed, fresh `.vst3` and `.component` in `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/`.
  5. Each plan under Phase 24 names `/improve [PluginName]` as its execution mechanism (not direct edits), and each plugin's post-`/improve` STATUS.md reflects the microtonal integration.
**Plans**: 8 plans

Plans:
- [x] 24-01-O-Bells-PLAN.md — O-Bells consumes note-expression module via /improve (PROP-01 + TRACK-01..05). Version 4.0.0 → 4.1.0. -- completed 2026-04-26 (Dorico 3-point smoke gate 3/3 PASS; canary)
- [ ] 24-02-O-Prism-PLAN.md — O-Prism consumes note-expression via /improve (PROP-03 + TRACK-01..05). Version 1.16.1 → 1.17.0.
- [ ] 24-03-O-Wind-PLAN.md — O-Wind consumes note-expression via /improve; adds missing PLUGIN_VERSION line (PROP-04 + TRACK-01..05). Version 1.15.1 → 1.16.0.
- [ ] 24-04-O-IntonationPad-PLAN.md — O-IntonationPad consumes note-expression via /improve; multi-sub-voice neRatio propagation (PROP-02 + TRACK-01..05). Version 2.7.2 → 2.8.0.
- [ ] 24-05-O-Reed-PLAN.md — O-Reed consumes note-expression via /improve; first MPE plugin, helper-based composition (PROP-05 + TRACK-01..05). Version 1.0.11 → 1.1.0.
- [ ] 24-06-O-Bowed-PLAN.md — O-Bowed consumes note-expression via /improve; second MPE plugin (PROP-06 + TRACK-01..05). Version 1.2.1 → 1.3.0.
- [ ] 24-07-O-Formant-PLAN.md — O-Formant consumes note-expression via /improve; adds missing OuariconModules.cmake include (PROP-07 + TRACK-01..05). Version 1.24.2 → 1.25.0.
- [ ] 24-08-final-sweep-PLAN.md — Rebuild + freshly install all 8 affected plugins; registry audit; aggregate Dorico smoke results (TRACK-05; re-claims PROP-01..07 via post-sweep regression smoke).

### Phase 25 (C): Package & Internal Technical Notes
**Goal**: A canonical pre-configured Dorico expression map file is authored once, stored as single source of truth in the microtonal module's resources, and bundled into every affected plugin's installer — with internal developer-reference notes (not end-user manuals) captured under `research/` to serve as source material for future website manual/quickstart authoring.
**Depends on**: Phase 24 (installer/docs reflect the completed set of 8 integrated plugins)
**Requirements**: INST-01, INST-02, INST-03, INST-04, DOCS-01, DOCS-02, DOCS-03, DOCS-04, DOCS-05
**Success Criteria** (what must be TRUE):
  1. A canonical `Ouaricon-VST3-NoteExpression.doricoexpmap` (or similarly named) exists at `modules/[microtonal-module]/resources/` with Microtonality explicitly set to "VST3 Note Expression" — confirmed as the single source of truth referenced by all 8 plugin installers.
  2. All 8 affected plugins' installers (PKG on macOS, EXE on Windows per existing `build-installer` / `package` workflows) bundle the `.doricoexpmap` file, and at least one dry-run install + Dorico quarter-sharp test validates the bundled expression map produces correct microtonal playback.
  3. Internal technical notes live under `research/microtonal-dorico-integration.md` (or per-topic sub-files) and cover all four required topics: module architecture (DOCS-01), canonical Dorico expression-map setup procedure (DOCS-02), host-side behavior quirks (DOCS-03), and troubleshooting signatures for the expression-map-skipped UX trap (DOCS-04).
  4. Notes are developer-facing only — no end-user manual or quickstart copy is published this milestone; DOCS-01..04 are structured to translate cleanly into future website authoring (DOCS-05 constraint honored).
  5. Installed `.doricoexpmap` lands at a discoverable location for end users, or the installer emits a README pointing to the file's install path with a one-line Dorico import instruction.
**Plans**: 8 plans

## Progress

**Execution Order:**
Phases execute in numeric order: 23 → 24 → 25

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1-7 | v1.0 | 21/21 | Complete | 2026-02-01 |
| 8-9 | v1.1 | 4/4 | Complete | 2026-02-02 |
| 10-13 | v1.2 | 12/12 | Complete | 2026-02-06 |
| 14-17 | v1.3 | 14/14 | Complete | 2026-02-10 |
| 18-22 | v1.4 | 13/13 | Complete | 2026-03-07 |
| 23 (A) | v1.5 | 5/5 | Complete    | 2026-04-25 |
| 24 (B) | v1.5 | 1/8 | In progress (Plan 24-01 O-Bells canary complete) | - |
| 25 (C) | v1.5 | 0/TBD | Not started | - |

**Cumulative: 22 phases complete, 65 plans complete, 5 milestones shipped. v1.5 = 3 phases planned (23-25), 33 requirements mapped. Phase 24 Plan 24-01 (O-Bells canary) complete — Dorico 3-point smoke gate 3/3 PASS.**

---
*Roadmap created: 2026-01-30*
*Last updated: 2026-04-26 -- Phase 24 Plan 24-01 (O-Bells) complete; canary PASS, propagation playbook validated*
