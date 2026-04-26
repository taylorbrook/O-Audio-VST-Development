# Phase 24: Propagate — Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `24-CONTEXT.md` — this log preserves the alternatives considered.

**Date:** 2026-04-25
**Phase:** 24-propagate
**Areas discussed:** Plan granularity, Per-plugin spec depth, Smoke test rigor, Sequencing & failure handling

---

## Plan granularity

### Q1: How should Phase 24 be structured into plans?

| Option | Description | Selected |
|--------|-------------|----------|
| 7 per-plugin plans | One plan per plugin (24-01..24-07). Each = full /improve cycle. Atomic rollback. Maps cleanly to TRACK-01..05. | ✓ |
| Grouped by complexity (3 plans) | e.g., easy synths / physical-models / formant. Fewer files but multi-plugin plans complicate rollback. | |
| Single batch plan | One PLAN.md sequences all 7. Simplest but defeats atomic-plan-commit pattern. | |

**User's choice:** 7 per-plugin plans (Recommended).

### Q2: Should there be a phase-end sweep plan in addition to the per-plugin work?

| Option | Description | Selected |
|--------|-------------|----------|
| Yes — 24-08-final-sweep | Closing plan: rebuild + freshly install all 8 plugins, AU verify all 8, audit registry used_by, regenerate /module-info. Maps to Success Criterion #1 and #4. | ✓ |
| No — fold into last per-plugin plan | Last plan carries closeout. Fewer files; muddies that plan's purpose. | |
| No — phase-verify catches it | Standard /gsd-verify-phase 24 handles cleanup. | |

**User's choice:** Yes — 24-08-final-sweep (Recommended).

### Q3: Should there be a phase-start prep plan before per-plugin work?

| Option | Description | Selected |
|--------|-------------|----------|
| No — first plugin plan picks up clean | Module is extracted, registry exists, OuariconModules.cmake finalized. Nothing universal to do first. | ✓ |
| Yes — 24-00-prep with shared setup | Cross-plugin scaffolding upfront. Risks ceremony if no real shared work. | |

**User's choice:** No — first plugin plan picks up clean (Recommended).

### Q4: How does each per-plugin plan invoke /improve?

| Option | Description | Selected |
|--------|-------------|----------|
| Plan task = '/improve [Plugin]' invocation | Each PLAN.md has a top-level task that explicitly invokes /improve. Honors TRACK-01 literally. | ✓ |
| Plan inlines steps; /improve at end | Plan does work directly; /improve runs as record-keeping. Bypasses backup gate. | |
| Plan invokes /improve interactively per plugin | Full interactive /improve per plugin. Heavyweight when spec is locked. | |

**User's choice:** Plan task = '/improve [Plugin]' invocation (Recommended).

---

## Per-plugin spec depth

### Q1: How specific should each per-plugin PLAN.md be about integration points?

| Option | Description | Selected |
|--------|-------------|----------|
| Pre-discovered, file:line specific | Each PLAN.md names exact voice file, exact base-frequency assignment line, TuningEngine vs raw frequency. Mirrors Phase 23 specificity. | ✓ |
| High-level shape, /improve discovers | PLAN.md gives shape only; /improve discovers per plugin. Faster planning, risks late surprises. | |
| Hybrid | Plans identify file + source class; /improve resolves line numbers. | |

**User's choice:** Pre-discovered, file:line specific (Recommended).

### Q2: Should the planner build a single integration-point matrix as a phase artifact?

| Option | Description | Selected |
|--------|-------------|----------|
| Yes — 24-INTEGRATION-MATRIX.md | Single markdown table, one row per plugin. Each plan references its row. Single source of truth. | ✓ |
| No — inline per-plan | Each PLAN.md carries its own integration block. More duplication; plans more self-contained. | |

**User's choice:** Yes — 24-INTEGRATION-MATRIX.md (Recommended).

### Q3: Should the planner verify which of the 7 plugins compose with a TuningEngine vs raw frequency?

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, planner inspects all 7 | Confirmed already: O-Bells, O-IntonationPad have TuningEngine. Inspect remaining 5 explicitly. | ✓ |
| Assume non-TuningEngine, /improve discovers | Default to raw-frequency composition. Risks late surprises. | |

**User's choice:** Yes, planner inspects all 7 (Recommended).

---

## Smoke test rigor

### Q1: What's the per-plugin Dorico smoke-test acceptance gate?

| Option | Description | Selected |
|--------|-------------|----------|
| Minimum quarter-sharp + zipper check | 3-point: +50¢ at C4, no attack zipper, NE correlated by noteId. ~2 min/plugin. Hits all three landmines. | ✓ |
| Full LYR-03 5-test battery per plugin | Re-run all 5 Phase 23 tests for each. Maximum confidence; ~70 min Dorico time total. | |
| Minimum + canary plugin gets full battery | One plugin (likely O-Bells) gets full battery; other 6 get minimum. ~95% issue-catch in ~30% time. | |

**User's choice:** Minimum quarter-sharp + zipper check (Recommended).

### Q2: What's the build-side gate per plugin (before Dorico is opened)?

| Option | Description | Selected |
|--------|-------------|----------|
| Clean link + auval + verify-au-link.sh | VST3 + AU + Standalone link clean, no Steinberg::* undefineds, then auval runs. Matches D-30/D-31. | ✓ |
| Clean link only | Skips runtime AU validation. Risks the same class of defect as O-Lyrica Plan 23-04. | |
| Add pluginval gate | Above + pluginval at strictness 5. Adds ~30s/plugin; orthogonal to NE. | |

**User's choice:** Clean link + auval + verify-au-link.sh (Recommended).

### Q3: Should regression tests beyond Dorico smoke run per plugin?

| Option | Description | Selected |
|--------|-------------|----------|
| Standard /improve regression only | /improve Phase 5.5 runs whatever baseline each plugin has. NE wires in additively. | ✓ |
| Add per-plugin 'no-NE' regression check | Confirm 12-TET output unchanged in non-Dorico hosts. Light sanity check. | |

**User's choice:** Standard /improve regression only (Recommended).

### Q4: Where do the per-plugin Dorico test results get recorded?

| Option | Description | Selected |
|--------|-------------|----------|
| Per-plan SUMMARY.md | Each 24-NN-<Plugin>-SUMMARY.md captures result inline. Matches Phase 23 convention. | ✓ |
| Single 24-DORICO-RESULTS.md aggregate | One phase-level test-results doc. Cleaner; risks racing per-plugin commits. | |
| Both | Per-plan SUMMARY.md + final-sweep aggregate table. | |

**User's choice:** Per-plan SUMMARY.md (Recommended). [Note: 24-08-final-sweep also aggregates a table per D-10.]

---

## Sequencing & failure handling

### Q1: What ordering principle drives the 7-plugin sequence?

| Option | Description | Selected |
|--------|-------------|----------|
| Easy-first | Plugins closest to O-Lyrica (TuningEngine + simple voice) go first. Validates pattern; physical-models last. | ✓ |
| Complex-first | Hard cases first to catch surprises early. Risks burning team on hard cases. | |
| Alphabetical | Deterministic but no technical rationale. Risks landing hard case (O-Bowed) second. | |

**User's choice:** Easy-first (Recommended). Final exact order finalized in plan-phase using 24-INTEGRATION-MATRIX.md.

### Q2: How should mid-batch failure be handled?

| Option | Description | Selected |
|--------|-------------|----------|
| Stop on first failure, triage in same plan | Halt, diagnose, fix in same plan (or promote to fix plan if structural). Same playbook as Phase 23 Plan 23-05. | ✓ |
| Skip and continue, defer to follow-up plan | Mark deferred, keep moving. Phase verify must reconcile. Risks half-done state. | |
| Pause for user triage | Halt and ask user. Maximum control; minimum autonomy. | |

**User's choice:** Stop on first failure, triage in same plan (Recommended).

### Q3: Can /improve cycles run in parallel for multiple plugins?

| Option | Description | Selected |
|--------|-------------|----------|
| Strictly serial | AU cache is OS-shared; install paths shared; Dorico testing inherently serial. | ✓ |
| Source edits parallel, build/install/test serial | Marginal savings; complicates rollback. | |

**User's choice:** Strictly serial (Recommended).

### Q4: Module registry (used_by list) update timing?

| Option | Description | Selected |
|--------|-------------|----------|
| Per-plugin in /improve cycle | /module-add appends used_by entry as part of each atomic plugin commit. Self-aggregates. | ✓ |
| Bulk update in 24-08-final-sweep | Defer registry write to phase end. Risks consumed/registry divergence mid-phase. | |

**User's choice:** Per-plugin in /improve cycle (Recommended).

---

## Claude's Discretion

Areas the user explicitly delegated to planner judgment:

- Exact ordering of plugins 1–7 within easy-first principle (D-11) — planner uses integration matrix.
- Exact `/improve` invocation flags (express/auto vs interactive) per plan (D-03).
- PLAN.md task breakdown shape (one delegating task vs multiple) (D-03).
- Format of `24-INTEGRATION-MATRIX.md` (markdown table vs YAML) (D-05).
- Whether the canary plugin in slot 1 gets a heavier smoke battery than the minimum (default: same minimum).
- Whether `24-08-final-sweep` re-runs Dorico smoke on all 8 (recommended: yes).
- PLAN.md naming convention (recommend `24-NN-O-<Name>-PLAN.md`).

## Deferred Ideas

Captured in `24-CONTEXT.md` `<deferred>` section:

- Automated Dorico smoke harness (scripted DAW driving)
- Preset-format compatibility audit (likely no-op, but worth noting)
- Per-plugin docs/README updates (Phase 25 territory)
- Windows VST3 verification (FUT-01)
- Custom NE types beyond kTuningTypeID (FUT-02)
- Per-plugin atomic-table performance audit
- Cross-plugin /module-info schema scaling
