# Phase 12: Accountability & Validation - Context

**Gathered:** 2026-02-05
**Status:** Ready for planning

<domain>
## Phase Boundary

Track which resources agents actually consulted and warn when expected resources were skipped. Agents report via `resources_consulted` in their reports; a SubagentStop hook validates usage. Warning-level only — never blocks workflow.

</domain>

<decisions>
## Implementation Decisions

### Report format
- Claude's Discretion: entry detail level (filepath only vs filepath + relevance note)
- Only list resources that were consulted — absence from the list implies not consulted (no explicit skipped[] list needed)
- Claude's Discretion: whether to include injection metadata (resources_injected count) in the report or let the hook derive it independently
- Claude's Discretion: whether field is conditionally required (when resources were injected) vs always optional — decide based on backward compatibility needs

### Warning behavior
- Warnings must be visible in real-time during workflow execution — user wants to see skipped-resource warnings as they happen, not buried in post-run logs
- Claude's Discretion: whether warnings also appear in verification reports (dual surface) or console-only is sufficient given real-time requirement
- Claude's Discretion: per-agent independent warnings vs accumulated summary — decide based on hook architecture constraints
- Claude's Discretion: warning message detail level (filename only vs filename + why it mattered)

### Agent integration
- Shared schema, flexible content: all agents use the same `resources_consulted` field name and structure, but can add agent-specific context if useful
- Claude's Discretion: when agents populate the field (end of execution vs progressively) — decide based on subagent architecture
- Claude's Discretion: whether agents with no injected resources include an empty field or omit it
- Universal accountability: ANY agent that receives injected resources should report `resources_consulted`, not just the 5 stage agents listed in ACCT-02 — scope expands to all resource-receiving agents

### Compliance criteria
- Claude's Discretion: definition of "consulted" (self-report vs evidence-based) — pick a practical balance
- Claude's Discretion: whether to use tiered importance (only MUST-READ triggers warnings) or treat all injected resources equally — leverage existing discovery scoring tiers
- Claude's Discretion: how to handle missing field vs empty list distinction
- Claude's Discretion: strictly warnings per v1.2 roadmap decision (warning-level accountability, not blocking) — configurable escalation deferred

### Claude's Discretion
- Report entry detail level and injection metadata inclusion
- Warning surface (console-only vs dual), aggregation strategy, and message detail
- Field population timing and empty-field handling
- Compliance definition, importance tiers, and missing-field semantics
- All of the above should be decided during planning based on what's practical given the existing hook and subagent architecture

</decisions>

<specifics>
## Specific Ideas

- User explicitly wants real-time warning visibility during workflow runs — this is the strongest preference expressed
- Universal accountability (not just 5 stage agents) — any agent receiving resources should report what it consulted
- Shared schema with flexible content — uniform enough for the hook to parse, flexible enough for agents to add context

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 12-accountability-validation*
*Context gathered: 2026-02-05*
