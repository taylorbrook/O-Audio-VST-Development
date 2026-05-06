# Phase 13: Maintenance Tooling & Hardening - Context

**Gathered:** 2026-02-06
**Status:** Ready for planning

<domain>
## Phase Boundary

Auto-generation of the resource manifest from frontmatter, freshness tracking with staleness warnings, graceful degradation when pipeline components fail, and enforced frontmatter standards on all research docs. This phase makes the resource system self-maintaining and resilient.

</domain>

<decisions>
## Implementation Decisions

### Auto-generation behavior
- Manifest regenerates **on file write** via Claude hooks (not on commit, not manual-only)
- When a research doc has no frontmatter or malformed frontmatter: **skip the doc with a warning** and create a trackable todo/bug report for fixing the malformed frontmatter
- Agents that create research docs (e.g., deep-research, gsd-phase-researcher) must **auto-populate valid frontmatter** at creation time

### Freshness & staleness policy
- `juce_version` represents the version the doc was **last verified against** (not "written for") — updates alongside `last_verified`
- Stale resources (>90 days) are still injected but flagged with a warning
- Verification is both manual (edit frontmatter directly) and script-assisted (convenience command for batch operations)

### Graceful degradation strategy
- When manifest is missing: agents **proceed silently** with no research context, warning logged to stderr only
- Agents never see a notice in their prompt about missing infrastructure — the system just works without it
- On discovery script crash: return empty (clean failure, no corrupted partial results)

### Frontmatter standards
- **Required fields:** created, last_verified, juce_version, keywords, category (5 mandatory fields)
- Docs missing any required field are **rejected from manifest** until all fields are present
- Validation warns per missing field so authors know exactly what to fix

### Claude's Discretion
- Staleness warning format (inline vs stderr — pick what's most practical)
- Whether to show a diff when regenerating manifest
- Staleness threshold configurability (fixed 90 days vs configurable)
- Degradation event persistence (stderr only vs log file)
- Test coverage depth for failure scenarios (main 3 vs edge cases)

</decisions>

<specifics>
## Specific Ideas

- Hook-triggered regeneration should fire on research file writes, not require manual runs
- Malformed frontmatter should produce actionable output: "file X missing fields Y, Z" — not just "invalid"
- The todo/bug report for malformed frontmatter should be visible enough that it gets fixed, not buried

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 13-maintenance-tooling-hardening*
*Context gathered: 2026-02-06*
