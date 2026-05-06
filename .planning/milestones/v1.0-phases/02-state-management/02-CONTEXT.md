# Phase 2: State Management - Context

**Gathered:** 2026-01-30
**Status:** Ready for planning

<domain>
## Phase Boundary

Harden file-based state persistence and session continuity for the Plugin Freedom System. Workflow state persists reliably across sessions with automatic corruption detection, recovery options, and multi-plugin isolation. This covers the infrastructure that agents and users interact with — not the agents themselves (Phase 1) or handoff formats (Phase 3).

</domain>

<decisions>
## Implementation Decisions

### State file structure
- All workflow state moves to `.planning/workflow/` (aligns with GSD pattern)
- JSON format for machine-readable state files (matches Phase 1 contract schemas)
- Git-tracked (visible in commits, reviewable, rollback-able)
- Structure:
  ```
  .planning/
  ├── STATE.md          # GSD project state (existing)
  ├── ROADMAP.md        # GSD roadmap (existing)
  ├── PROJECT.md        # GSD project info (existing)
  ├── phases/           # GSD phase work (existing)
  └── workflow/         # NEW: Plugin Freedom System runtime state
      ├── registry.json     # Plugin registry
      ├── active-plugin.json # Current focus state
      └── checkpoints/      # Resume points
  ```

### Session resume behavior
- Full restoration with cap — load current phase's CONTEXT.md + PLAN.md
- Checkpoints at task-level granularity (fine-grained, after each significant task)
- Context clear transitions show copy-paste slash command only (no verbose summaries)

### Corruption detection & recovery
- Corruption defined as: schema validation failures OR cross-file inconsistency
- Cross-file inconsistency = STATUS.md and registry.json disagree on plugin state
- Do NOT detect staleness (too many false positives, arbitrary thresholds)
- When corruption detected: prompt user before repair (don't auto-fix silently)
- Recovery options presented to user:
  1. Manual repair instructions
  2. Reset to last known good checkpoint
  3. Rebuild state from filesystem scan
- Validation runs automatically on `/continue` and `/focus`
- Manual validation available via `/reconcile`

### Plugin isolation boundaries
- `/focus` loads only that plugin's `.planning/` state (complete workflow isolation)
- Awareness of shared modules and dependencies (know what code is shared)
- Module tracking in both directions:
  - Each plugin lists its deps in `plugins/X/.planning/dependencies.json`
  - Central registry maps modules → plugins in `.planning/workflow/module-deps.json`
- Strictly single-plugin operations (switch focus to work on another plugin)
- Read-only peek at other plugins allowed (useful for comparisons, no modification)
- Parallel Claude Code instances supported (each can focus different plugin)

### Claude's Discretion
- Exact checkpoint file format and naming
- Lock-free vs file-locking for central registry (based on implementation complexity)
- Schema validation library choice
- How to handle corrupted checkpoints specifically

</decisions>

<specifics>
## Specific Ideas

- Should feel like picking up where you left off, not reconstructing from scratch
- Plugin development state moving to `.planning/workflow/` makes GSD and PFS state cohesive
- Parallel instances should "just work" for the common case of different plugins

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 02-state-management*
*Context gathered: 2026-01-30*
