# Phase 4: Verification Infrastructure - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

<domain>
## Phase Boundary

Build independent verification through generator-critic loops that catch issues before stage transitions. Critic agent validates outputs against contracts, with iterative refinement up to 3 attempts. Domain-specific critics for DSP (real-time safety) and UI (polish standards). Token budget awareness for cost management.

</domain>

<decisions>
## Implementation Decisions

### Critic Invocation
- Run automatically at stage gates AND on-demand when user invokes manually
- Run all applicable critics for the output type (not explicit selection)
- Tiered validation: quick smoke tests per task, deep validation at stage gates
- Stage determines which critics are applicable (Stage 2 → DSP, Stage 3 → UI)

### Failure Handling
- Severity-based: Errors block progression, warnings proceed with acknowledgment
- `--force` flag allows bypassing errors with stderr warning (user discretion)
- Escalate to user after 3 failed iterations (matches max iteration count)
- Never silently continue past errors — require explicit acknowledgment

### Iteration Behavior
- Same agent receives critic feedback and fixes in-place (no separate fixer agent)
- Include attempt count visible to agent ("Attempt 2/3") for urgency awareness
- Stop early when: validation passes OR no new issues found (no progress)
- Token budget: warn when exceeded but don't interrupt iteration

### Feedback Format
- Dual format: structured for agent consumption, human-readable summaries
- Numeric scores (1-10) per category with category-specific thresholds
  - DSP safety: higher threshold (Claude determines exact value)
  - UI polish: lower threshold (Claude determines exact value)
- Include fix suggestions with each issue (describe approach, not code snippets)
- Persist only failure reports to `.planning/verification/` — skip passing reports

### Claude's Discretion
- Exact score thresholds per category
- Specific critic rule implementations
- Report file naming and structure
- How to detect "no progress" for early stopping

</decisions>

<specifics>
## Specific Ideas

- Modeled on GSD's verification pattern: verify-phase gives feedback to executor who fixes in-place
- Aligns with Phase 3's `--force` bypass pattern (gate composition)
- Failure reports create audit trail for debugging recurring patterns

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 04-verification-infrastructure*
*Context gathered: 2026-01-31*
