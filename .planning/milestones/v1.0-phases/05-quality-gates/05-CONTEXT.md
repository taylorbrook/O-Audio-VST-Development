# Phase 5: Quality Gates - Context

**Gathered:** 2026-01-30
**Status:** Ready for planning

<domain>
## Phase Boundary

Implement blocking gates at stage boundaries (0→1, 1→2, 2→3, 3→4) that enforce measurable success criteria before progression. Gates run automated verification and require passing checks before stage advancement.

</domain>

<decisions>
## Implementation Decisions

### Gate Failure Behavior
- Block by default, `--force` flag allows bypass
- Bypass requires user to provide justification (prompted)
- Justification logged in handoff document for audit trail
- Allow 3 retry attempts before requiring `--force` or escalation
- Retry once automatically on failure to handle transient/flaky issues

### Verification Depth
- Uniform depth at every gate — no tiered or fast-path options
- Full suite at every gate: schema validation, build, domain critic, pluginval, test suite
- Target ~2 minute completion; parallelize if longer
- Quality is non-negotiable — no `--quick` bypass for lighter checks

### Success Criteria
- Critical + advisory model: critical checks block, advisory checks report
- **Critical (must pass):** schema validation, build success, pluginval pass, realtime safety
- **Advisory (reports only):** code style, naming, documentation completeness, polish scores
- Domain critics are stage-dependent:
  - Stage 2+: DSP critic is critical (realtime safety must pass)
  - Stage 3+: UI critic is critical (thread safety, APVTS patterns)
  - Critics are advisory before their relevant stage

### Code Review Integration
- Human code review at end of each stage (4 review points)
- Blocking by default, `--skip-review` flag to bypass
- Review focus is domain-specific per stage:
  - Stage 1: Foundation/build patterns
  - Stage 2: DSP patterns, realtime safety
  - Stage 3: GUI patterns, threading
  - Stage 4: Integration, polish
- Mandatory code simplification pass included in every review

### Claude's Discretion
- Exact parallelization strategy for checks exceeding 2 minute budget
- How to display advisory findings (inline vs summary)
- Retry delay between automatic flaky-check retries
- Review prompt format and checklist presentation

</decisions>

<specifics>
## Specific Ideas

- Flaky check handling follows CI/CD best practice (GitHub Actions, CircleCI pattern)
- Critical + advisory split prevents low-priority issues from blocking high-value work
- Stage-dependent critic criticality ensures domain experts block at decision points

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 05-quality-gates*
*Context gathered: 2026-01-30*
