# Critic Review Protocol

**Context:** This file is part of the plugin-workflow skill.
**Referenced by:** SKILL.md Post-Stage Critic Review section
**Purpose:** Detailed protocol for post-stage critic reviews via the critic-orchestrator agent

---

## When Critics Run

Critic reviews run after **every stage completion** (Stage 1, 2, 3, 4). This is a locked decision -- not just at cross-stage boundaries.

The review step is inserted between execute and verify in the phase flow:

```
execute -> critic-review -> verify
```

If critic review finds blockers, the flow becomes:

```
execute -> critic-review [BLOCKERS] -> fix -> execute -> critic-review -> verify
```

## Stage-to-Critic Mapping

The critic-orchestrator determines which critics to run based on the completed stage. The table below is the default mapping; the orchestrator may adjust based on plugin complexity.

| Stage | Always Run | Conditionally Run | Skip |
|-------|-----------|-------------------|------|
| Stage 1 (Foundation) | foundation, cross-contract | architecture (if complex plugin) | DSP, UI |
| Stage 2 (DSP) | DSP safety, parameter integration | architecture alignment, numerical stability | UI |
| Stage 3 (GUI) | UI polish, parameter bindings, WebView integrity | accessibility | DSP (already validated) |
| Stage 4 (Polish) | cross-contract, resource accountability | all domain critics if changes detected | -- |

### Conditional Triggers

- **Architecture critic at Stage 1:** When plugin has 3+ DSP components, custom module dependencies, or complex signal chain
- **Numerical stability at Stage 2:** When plugin uses feedback paths, IIR filters, or recursive algorithms
- **Accessibility at Stage 3:** When plugin targets professional market or has complex UI layout
- **All critics at Stage 4:** When Stage 4 changes touch files already validated by earlier critics

## Unified Report Format

Individual critic reports are merged into a unified severity-ranked report using `.claude/hooks/merge-critic-reports.py`.

**Severity levels (ranked):**

| Severity | Rank | Action | Examples |
|----------|------|--------|----------|
| blocker | 0 | PREVENTS stage progression | Missing null checks in processBlock, heap allocation in audio thread, broken parameter binding |
| warning | 2 | Advisory, does not block | Suboptimal algorithm choice, missing edge case handling, naming convention violation |
| note | 3 | Informational | Suggested improvement, style preference, future optimization opportunity |
| info | 4 | Low-priority observation | Documentation suggestion, code organization note |

**Severity normalization:** Individual critic reports may use "error" severity. The merger normalizes "error" to "blocker" in the unified report for consistent ranking.

**Issue ID format:**
- Architecture critic: `ARCH-NNN`
- Foundation critic: `FND-NNN`
- DSP critic: `DSP-NNN`
- UI critic: `UI-NNN`

## Enforcement Rules

**Blockers PREVENT stage progression.** The critic-orchestrator MUST NOT allow the pipeline to advance when any blocker exists.

```
if unified_report.blocking_count > 0:
    progression_allowed = false
    -> return to execute phase with fix list
else:
    progression_allowed = true
    -> proceed to verify phase
```

**Warnings and notes are advisory.** They are included in the report but do not block progression. They should be addressed when practical but are not mandatory for advancement.

## Token Budget Awareness

To manage token consumption:

1. **Start with minimal critic sets** -- only run critics that are "Always Run" for the stage
2. **Expand conditionally** -- add conditional critics only when triggers are met
3. **Skip already-validated domains** -- DSP critics skip at Stage 3 if no DSP files were modified

This prevents running unnecessary critics that consume tokens without adding value.

## Integration with Existing Infrastructure

The critic-orchestrator replaces manual critic invocation (`.planning/workflow/scripts/run-critic.sh`) but uses the same underlying critic definitions and schemas:

- **Critic definitions:** `.claude/critics/critic-dsp.md`, `.claude/critics/critic-ui.md`, `.claude/critics/critic-architecture.md`, `.claude/critics/critic-foundation.md`
- **Report schema:** `.planning/workflow/schemas/critic-report.schema.json`
- **Unified report schema:** `.planning/workflow/schemas/critic-report-unified.schema.json`
- **Report merger:** `.claude/hooks/merge-critic-reports.py`

## Spawning Pattern

Critics are spawned as **parallel subagents** (NOT Agent Teams). Each critic runs independently and reports back to the orchestrator. No inter-critic communication is needed.

```
For each applicable critic:
  Task(critic-{domain}): "Review {plugin} at {stage} completion.
    Plugin path: plugins/{plugin}/
    Stage: {stage}
    Produce JSON report conforming to critic-report.schema.json.
    Evaluate all scoring categories for your domain.
    Include issue IDs, severity, locations, and fix suggestions."
```

All applicable critics are spawned simultaneously. The orchestrator waits for all to complete before merging.

## Error Handling

| Scenario | Action |
|----------|--------|
| Critic subagent fails | Log failure, continue with remaining critics. Note in unified report. |
| Merge script fails | Return raw critic reports without merging. Flag as degraded mode. |
| All critics fail | Return error report with `progression_allowed: false` and escalation recommendation. |
| No critics applicable | Return empty report with `progression_allowed: true`. |

## Report Storage

- **Individual critic reports:** `.planning/verification/{plugin}/{stage}/`
- **Unified report:** `.planning/verification/{plugin}/{stage}/unified-report.json`
