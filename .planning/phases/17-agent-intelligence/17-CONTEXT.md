# Phase 17: Agent Intelligence - Context

**Gathered:** 2026-02-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Add parallel agent teams for read-heavy research and review workflows, with fine-grained quality hooks and configurable branching. The sequential Stage 0-4 pipeline remains the primary implementation path. Agent teams are strictly read-heavy — no implementation parallelization.

</domain>

<decisions>
## Implementation Decisions

### Research team composition
- Team size: 2-3 researchers depending on plugin complexity (simple plugins get 2, complex get 3)
- Researcher domains are fully dynamic — determined at runtime based on creative brief, complexity, and plugin type (not a fixed set like "DSP + UI + module audit")
- Findings merge via debate format — researchers read each other's outputs and produce a synthesis, catching contradictions
- Researcher conflicts block planning — if researchers disagree on incompatible approaches, planning pauses until the conflict is resolved (not just surfaced as a warning)

### Review/critic workflow
- Parallel critics run after every stage completion (Stage 1, 2, 3, 4) — not just at cross-stage boundaries
- Critic domains: Claude's Discretion — Claude picks which critics are relevant based on the stage and plugin type (e.g., DSP critic irrelevant after Stage 1 foundation)
- Unified review report: severity-ranked issues — all critic findings merged into one list sorted by severity (blocker > warning > note)
- Enforcement: blocker-severity findings prevent stage progression; warnings and notes are advisory

### Approval gates & delegation
- Plan approval: auto-approve low-risk plans (small scope, few files, no DSP changes); complex plans require team lead review
- Delegate mode tools: coordination + read + lightweight bash (git status, build checks) — no file writes or edits
- Rejection flow: teammate revises and resubmits the plan with lead's feedback
- Retry limit: 3 revision attempts before escalating (to user or lead takes over)

### Task-level validation hooks
- Validator-to-task mapping: auto-detect from task content (e.g., task mentions 'processBlock' triggers DSP safety validator)
- Failure flow: block + auto-fix attempt on first failure; escalate to user on second failure
- Scope: only code-touching tasks get validated (.cpp, .h, .cmake, .html, .js) — docs/config tasks skip validation

### Claude's Discretion
- Exact critic domain selection per stage
- Complexity threshold for auto-approve vs gated plans
- Validation feedback format (structured JSON vs freeform — balance machine-parseability with validator authoring simplicity)

</decisions>

<specifics>
## Specific Ideas

- Research debate should catch contradictions between domains (e.g., DSP approach that's incompatible with a UI requirement)
- Quality hooks should wire the existing 6 domain validators (DSP safety, APVTS matching, WebView bindings, checksums, cross-contract, resource accountability) into per-task checks
- Canary plugin testing (O-SimpleReverb) required after all changes per P40 constraint

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 17-agent-intelligence*
*Context gathered: 2026-02-09*
