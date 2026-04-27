---
gsd_state_version: 1.0
milestone: v1.5
milestone_name: Microtonal Shared Module & Suite Propagation
status: executing
stopped_at: Phase 25 Plan 25-01 v3 complete (Path B canary PASS); Plan 25-02 next
last_updated: "2026-04-27T16:30:00.000Z"
last_activity: 2026-04-27 -- Plan 25-01 v3 closed; canary PASS end-to-end
progress:
  total_phases: 20
  completed_phases: 19
  total_plans: 67
  completed_plans: 66
  percent: 98
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-04-24)

**Core value:** Reliable collaborative workflow that produces professional-quality plugins -- where agents execute quality work that doesn't require constant rework.
**Current focus:** Phase 25 — package-docs

## Current Position

Milestone: v1.5 Microtonal Shared Module & Suite Propagation -- ACTIVE
Phase: 25 (package-docs) — EXECUTING
Plan: 2 of 3 (25-01 v3 complete; 25-02 next)
Status: Plan 25-01 closed; Plan 25-02 (8-plugin installer-bundling sweep) unblocked
Last activity: 2026-04-27 -- Plan 25-01 v3 closed; canary PASS end-to-end on O-Lyrica-dev

Progress: [██████████] 99%

History (Phase 25):

- v1: drop .doricoexpmap into Dorico's User/ → reverted at d2c86c5 (Dorico does not recognize .doricoexpmap extension). See `25-FINDING-playback-template-pivot.md`.
- v2: ship .dorico_pt Playback Template + .doricolib → merged at 819b2b4 BUT three blocking issues found at A2 checkpoint:
  1. .doricolib lacks Dorico's required 48-container kScoreLibrary skeleton (cd2c2c6 recovery is a fragment, not a library)
  2. endpointconfig.xml.in has hardcoded prod plugin names but dev/prod CIDs → silent slot drop on dev installs
  3. Playback Template architecture is over-engineered for v1.5 use case
- Path B (validated 2026-04-27): ship single Dorico-valid .doricolib only; user assigns expression map manually in Play → Endpoints → Expression Map dropdown after loading their plugin. Verified end-to-end with quarter-sharp C4 playback at ~269 Hz on O-Lyrica-dev.
- v3 Plan 25-01 (executed 2026-04-27): canonical .doricolib reauthored from factory skeleton + injected ExpressionMapDefinition (byte-identical to verified reference); Path A artifacts surgically excised under D-10 amend-forward (playback-template/ subtree, ouaricon_extract_vst3_cids helper, .dorico_pt packing, dual-write logic); install.cmake.in collapsed to single-write to Ouaricon shared path; module READMEs rewritten for Library Manager Import flow; O-Lyrica canary PASS end-to-end on macOS 26.3.1 / Dorico 6 / O-Lyrica-dev (build → cmake-install → Library Manager Import → quarter-sharp ~269 Hz, no zipper, polyphonic isolation). Wave 0 v3 auto-discovery probe FAIL (informational; non-blocking; logged for v1.6 deferred-ideas per D-08 carry-forward).

Finding docs:

- `.planning/phases/25-package-docs/25-FINDING-playback-template-pivot.md` (v1 → v2)
- `.planning/phases/25-package-docs/25-FINDING-path-b-validation.md` (v2 → v3, NEW)

Test log: `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` (full A2 + Path B trace)

Reference asset: `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` (6,431 B, Dorico-valid skeleton, import-confirmed)

Next: `/clear` then `/gsd-execute-phase 25` to run Plan 25-02 (8-plugin installer-bundling sweep — INST-03, INST-04). Plan 25-01 v3 closed with canary PASS; install component `ouaricon_note_expression_<TARGET>` proven; canonical asset + companion README at `~/Library/Application Support/Ouaricon/Microtonal Suite/` on macOS install.

## Performance Metrics

**Cumulative (v1.0-v1.4):**

- Total phases completed: 22
- Total plans completed: 69
- Total requirements satisfied: 108

**By Milestone:**

| Milestone | Phases | Plans | Requirements | Timeline |
|-----------|--------|-------|--------------|----------|
| v1.0 | 1-7 | 21 | 35 | 2 days |
| v1.1 | 8-9 | 4 | 13 | 2 days |
| v1.2 | 10-13 | 12 | 15 | 2 days |
| v1.3 | 14-17 | 14 | 22 | 3 days |
| v1.4 | 18-22 | 13 | 23 | 2 days |
| v1.5 | 23-25 | TBD | 33 | in progress |

**v1.4 Plan Metrics:**

| Phase | Plan | Duration | Tasks | Files |
|-------|------|----------|-------|-------|
| 18 | 01 | 1min | 3 | 30 |
| 18 | 02 | 5min | 2 | 20 |
| 18 | 03 | 4min | 1 | 1 |
| 18 | 04 | 1min | 1 | 1 |
| 19 | 01 | 4min | 2 | 3 |
| 20 | 01 | 15min | 2 | 44 |
| 20 | 02 | 1min | 2 | 6 |
| 20 | 03 | 20min | 2 | 12 |
| 21 | 01 | 2min | 2 | 8 |
| 21 | 02 | 2min | 2 | 4 |
| 21 | 03 | 1min | 1 | 76 |
| 22 | 01 | 2min | 2 | 9 |
| 22 | 02 | 2min | 2 | 2 |
| Phase 23 P01 | 4min | 2 tasks | 4 files |
| Phase Phase 23 P02 P23-02 | 4min | 3 tasks tasks | 4 files files |
| 23 | 03 | ~25min | 4 | 6 |
| 23 | 04 | ~12min + human | 4 | 3 |
| 23 | 05 | ~25min + human | 7 | 6 |
| 24 | 01 | ~30min + human | 5 | 8 |
| 25 | 01 | ~25min + human canary | 5 | 7 modified, 3 deleted |

## Accumulated Context

### Decisions

All v1.0-v1.4 decisions logged in PROJECT.md Key Decisions table.
Milestone-specific decisions archived in `.planning/milestones/`.

v1.5 decisions (to be logged as phase execution progresses):

- Phase 24 per-plugin rollouts MUST use `/improve [PluginName]` — enforces versioning, changelog, STATUS.md update, regression test (TRACK-01).
- Shared module candidate name `dsp/note-expression` — confirm against `/module-list` before creation (MOD-01).
- Local JUCE patch tracked as a named patch file in `scripts/` with re-apply procedure for JUCE upgrades (MOD-07).
- Phase 23 Plan 01: note-expression module shipped at modules/tuning/note-expression v1.0.0 — header-only under Ouaricon::NoteExpression, owns PendingTuningTable on VST3Extensions (D-09), no dependency on scala-tuning-engine (D-11)
- Phase 23 Plan 02: JUCE patch shipped as scripts/juce-patches/note-expression-juce-8.0.4.patch (1112 lines, generated via diff -u against pristine JUCE 8.0.4); idempotent apply-juce-patches.sh wrapper preflights JUCE_DIR and skips when JUCE-NE-PATCH marker already present (T-23-04); modules/tuning/note-expression/module.cmake gates only future consumers via opt-in module.cmake hook auto-included by OuariconModules.cmake (T-23-05, D-15)
- Phase 23 Plan 03: O-Lyrica refactored onto Ouaricon::NoteExpression — 4 source files edited, spike header VST3/NoteExpressionSupport.h deleted, OLyrica_VST3 + OLyrica_AU build clean (LYR-01/02). Build-gate discovery (D-23-03-A): Steinberg SDK symbols only link for VST3 client target — Controller class + queryIEditController body + nec member guarded behind #if JucePlugin_Build_VST3 in NoteExpression.h so AU/Standalone/VST2 link cleanly. Fix committed as f85ff38 against plan 23-01's deliverable. NOTE: Plan 23-04 later discovered Plan 23-03's "OLyrica_AU built clean" claim was incorrect — the on-disk AU artefact pre-dated the refactor commits and the AU re-link was never actually exercised. The f85ff38 guard does not solve the issue because guards evaluate at every TU compile site and SharedCode is compiled with VST3=1.
- Phase 23 Plan 04: O-Lyrica v2.3.0 shipped (CMakeLists VERSION + CHANGELOG [2.3.0] entry, LYR-04). Comprehensive note-expression module README published — 223 lines covering Quick Start / Features / Installation / Dorico Setup / Patch Management / Integration Approach (MOD-05). Dorico quarter-sharp smoke test 5/5 PASS via VST3 (LYR-03 gate cleared). Build-gate discovery (D-23-04-A): AU re-link failure exposed module-level architectural defect — JucePlugin_Build_VST3 guards in NoteExpression.h evaluate at TU-compile site, so SharedCode (compiled with VST3=1) carries Steinberg symbol references that AU's link line cannot resolve. Resolution deferred to Plan 23-05 (move Controller + Steinberg-touching code from header to VST3-only .cpp). Phase 24 BLOCKED until 23-05 lands.
- Phase 23 Plan 05: D-23-04-A AU-link Steinberg-symbol leak RESOLVED via two-TU split + custom function-pointer deleter pimpl + dual dispatch slots (g_neUpdate, g_neQuery). cpp/NoteExpression.cpp (SharedCode-bound, Steinberg-free) hosts ctor/dtor/drainAndUpdate/queryIEditController bodies; cpp/vst3/NoteExpression_VST3.cpp (VST3-only) hosts Controller body, vst3QueryIEditController free helper, updatePendingFromEvents, and the static-init DispatchRegistrar. modules/cmake/OuariconModules.cmake gained per-format source routing convention (cpp/<format>/ → ${TARGET}_<FORMAT>) used project-wide going forward. scripts/verify-au-link.sh provides reusable AU-link gate for Phase 24. Mid-flight Rule-1 q-slot fix (commit 0e00826) addressed plan-checker's prescient warning about VST3Extensions vtable references. LYR-03 Dorico smoke test re-passed 5/5 via VST3 (regression check). Public API + consumer call-sites preserved verbatim (D-23/D-32). Out-of-scope discovery: pre-existing APVTS Meta-Flag failure on parameter ID 1275870432 logged to deferred-items.md (was previously masked by AU re-link failure; not a Plan 23-05 regression). Phase 24 unblocked.
- Phase 24 Plan 01: O-Bells v4.1.0 propagation canary COMPLETE (atomic commit 8fee3a8). PROP-01 + TRACK-01..05 satisfied. 8-file atomic commit (CMakeLists.txt + PluginProcessor.{h,cpp} + BellVoice.{h,cpp} + CHANGELOG.md + STATUS.md + registry.yaml). Tri-format ninja exit 0 — no Steinberg-symbol leak (per-format module-source convention from Phase 23 D-22..D-29 holds). scripts/verify-au-link.sh O-Bells PASS — `AU VALIDATION SUCCEEDED. auval accepted O-Bells (aumu OBls OuDv)`. Dorico 3-point smoke gate (D-07) ALL PASS — gate 1 (~269.29 Hz at +50¢ above C4, Pattern 3 240-semitone full-scale validated); gate 2 (no attack zipper, Pattern 2 apply-before-DSP-trigger validated); gate 3 (polyphonic chord — only C4 detuned, E4 plays 12-TET 329.63 Hz, Pattern 1 noteId-correlation validated). System-environment notes: OUARICON_DEV_SUFFIX=-dev produces dev-suffixed bundles alongside prod-named bundles (intentional dev branding); auval -a system listing returns zero aumu entries on this machine (host-environment quirk affecting all plugins; verify-au-link.sh is canonical D-08 path and PASSES). Float→double cast at applyPendingTuning helper boundary works as designed (BellVoice uses float fundamental). Plan-checker phrasing: CHANGELOG body lowercase "adds" matches plan §verify grep; pattern propagates to plans 24-02..24-07. Feeds 24-08-final-sweep-SUMMARY.md row 1 of 8.
- Phase 24 Plans 02-07: 6 atomic propagation commits landed in single `/gsd-execute-phase 24` session 2026-04-26 (deferred-batch Dorico flow per user direction): O-Prism v1.17.0 (0393d0d), O-Wind v1.16.0 (4ae4600), O-IntonationPad v2.8.0 (a935830), O-Reed v1.1.0 (c829350), O-Bowed v1.3.0 (7b20d14), O-Formant v1.25.0 (d0e101a). Each: 8-file atomic commit, tri-format ninja exit 0, verify-au-link.sh PASS, fresh dual-bundle install. Three Rule-3 inline fixes uncovered/resolved during sweep: (a) O-IntonationPad CMakeLists missing `juce::juce_audio_utils` + `juce::juce_audio_devices` (blocked Standalone build); (b) O-Bowed missing `isBusesLayoutSupported` override (caused auval segfault, NOT NE regression); (c) O-Formant missing `OuariconModules.cmake` include (planned 2-step structural edit). Five propagation patterns catalogued for v1.5 retrospective: classic-Synthesiser-multi-osc (Prism), classic-Synthesiser-physical-period (Bells/Wind), classic-Synthesiser-multi-sub-voice (IntonationPad: applyPendingTuning at root with multiplicative ratio derivation), MPE-helper-based (Reed 3 sites / Bowed 2 sites), MPE-per-call-site (Formant). Note D pattern (explicit PLUGIN_VERSION arg in juce_add_plugin) applied to Wind/Reed/Formant/Bowed CMakeLists.
- Phase 24 Plan 08 final sweep COMPLETE (commit 0ec32e9). All 16 build targets `ninja: no work to do` (incremental no-op confirms cumulative tri-format link cleanliness across 8 plugins). All 8 plugins PASS substantive Dorico runtime + AU loading gates (O-Lyrica is the validated spike/reference plugin from Phase 23 Plans 01-05; the auval static-check finding DEF-24-01 was DOWNGRADED 2026-04-26 to a benign parameter-meta-flag annotation gap, not a defect). modules/registry.yaml note-expression.used_by has all 8 expected consumers; module v1.0.0 unchanged (D-33 honored). Dorico 3-point batch validation 2026-04-26: ALL 8 PLUGINS PASS (24/24 individual gate-points). VERIFICATION.md status=passed. Phase 24 closes v1.5 propagation cycle.
- Phase 25 Plan 01 v3 (Path B locked, completed 2026-04-27): single canonical Dorico-valid `.doricolib` (full 48-container kScoreLibrary skeleton + injected ExpressionMapDefinition; byte-identical to verified `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` reference) authored at `modules/tuning/note-expression/resources/library/`. Path A artifacts surgically excised under D-10 amend-forward (3 deletion groups in commit db20a04: playback-template/ subtree, ouaricon_extract_vst3_cids helper, .dorico_pt packing in module.cmake). install-microtonal-suite.cmake.in collapsed from 87-line dual-write to ~40-line single-write to platform-specific Ouaricon shared path only (D-07; v1.6 revival reference preserved as `git show 819b2b4:...`). Module READMEs rewritten for Library Manager Import flow (Path B); technical-mechanics paragraph (kVST3NoteExpression invariant) preserved verbatim. O-Lyrica canary PASS end-to-end on macOS 26.3.1 / Dorico 6 / O-Lyrica-dev: ninja build exit 0, cmake --install lands assets at `~/Library/Application Support/Ouaricon/Microtonal Suite/`, Dorico Library Manager Import succeeds (no invalid-file-format error), quarter-sharp C4 plays at ~269 Hz target with no attack zipper, polyphonic isolation as expected. Wave 0 v3 auto-discovery probe FAIL (informational only; non-blocking; logged for v1.6 deferred-ideas per D-08 carry-forward). 6 atomic commits: ad9e5e4 (Task 1), db20a04 (Task 2), 98479ba (Task 3), 93d29d6 (Task 4), c45703b (Task 5 verification log), f15b7e8 (v3 SUMMARY). INST-01 + INST-02 satisfied. Plan 25-02 unblocked.

### Pending Todos

1. CI/CD pipeline verification (test tag push)
2. ~~APVTS Meta-Flag audit across O-Lyrica~~ — **DOWNGRADED 2026-04-26**: O-Lyrica is the validated spike/reference plugin (Phase 23 Plans 01-05) and PASSES all substantive runtime + Dorico gates. The auval parameter-meta-flag finding (parameter ID 1275870432) is a benign tool-static-check artifact, not a defect. Optional cosmetic 1-line fix only if a release-pipeline code-signing audit surfaces it. See `.planning/phases/24-propagate/deferred-items.md` DEF-24-01 [DOWNGRADED].

### Blockers/Concerns

- **RESOLVED 2026-04-25 by Plan 23-05.** AU build was broken module-wide (D-23-04-A). Two-TU split with custom-deleter pimpl + dispatch slots restores clean OLyrica_VST3 + OLyrica_AU + OLyrica_Standalone link. Per-format module-source convention prevents Phase 24 plugins from regressing into the same defect class. Phase 24 unblocked.

### Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 6 | add a new slash command to build a windows exe installer for a vst3 plugin | 2026-02-11 | bf03372 | [6-add-a-new-slash-command-to-build-a-windo](./quick/6-add-a-new-slash-command-to-build-a-windo/) |
| 7 | Move O-Bells to top of Ouaricon Plugins table in PLUGINS.md | 2026-02-26 | 2efe535 | [7-move-o-bells-up-to-the-top-of-the-produc](./quick/7-move-o-bells-up-to-the-top-of-the-produc/) |
| 8 | Fix free purchase flow: bundle display, cart clearing, email logging | 2026-02-26 | bfd4368 | [8-fix-free-purchase-flow-products-land-in-](./quick/8-fix-free-purchase-flow-products-land-in-/) |
| 9 | Fix email confirmation landing page to show thank-you message | 2026-02-26 | 0ebc554 | [9-fix-email-confirmation-landing-page-to-s](./quick/9-fix-email-confirmation-landing-page-to-s/) |
| 10 | Fix Google OAuth provider not enabled on oaudio.io | 2026-02-26 | -- | [10-fix-google-oauth-provider-not-enabled-on](./quick/10-fix-google-oauth-provider-not-enabled-on/) |
| 11 | Review manuscript for grammar and spelling | 2026-03-02 | 0df9e01 | [11-review-manuscript-for-grammar-and-spelli](./quick/11-review-manuscript-for-grammar-and-spelli/) |
| 12 | Reformat manuscript as Word document for Leonardo submission | 2026-03-02 | -- | [12-reformat-manuscript-md-as-word-document-](./quick/12-reformat-manuscript-md-as-word-document-/) |
| 13 | Codebase refactoring audit (13 opportunities, ~3,200+ lines eliminable) | 2026-03-03 | 386e107 | [13-look-through-this-project-for-opportunit](./quick/13-look-through-this-project-for-opportunit/) |
| 14 | Full system review of PFS (632K tokens, 15 recommendations, 9 quick wins) | 2026-03-05 | 564c8bd | [14-full-system-review-of-plugin-freedom-sys](./quick/14-full-system-review-of-plugin-freedom-sys/) |
| e4q | Sweep repo root: remove 24 debug artifacts, add gitignore rules | 2026-03-19 | 523c74e | [260319-e4q-sweep-repo-root-for-misplaced-screenshot](./quick/260319-e4q-sweep-repo-root-for-misplaced-screenshot/) |
| 260408-vzl | Fix tuning module integration - compare with O-Prism, add container queries | 2026-04-09 | b6df44a | [260408-vzl-fix-tuning-module-integration-compare-wi](./quick/260408-vzl-fix-tuning-module-integration-compare-wi/) |

## Session Continuity

Last session: 2026-04-27T16:30:00.000Z
Stopped at: Phase 25 Plan 25-01 v3 closed; canary PASS end-to-end; Plan 25-02 unblocked
Resume file: None

Next: `/clear` then `/gsd-execute-phase 25` to run Plan 25-02 (8-plugin installer-bundling sweep — INST-03, INST-04). Plan 25-03 (internal docs — DOCS-01..DOCS-05) follows.

---
*v1.4 shipped 2026-03-07. v1.5 Microtonal Shared Module & Suite Propagation started 2026-04-24. Running total: 5 milestones shipped, 22 phases, 64 plans, 108 requirements. v1.5 adds 3 phases (23-25) and 33 requirements.*

**Planned Phase:** 24 (propagate) — 8 plans — 2026-04-26T05:25:14.989Z
