# Phase 1: Agent Contracts - Context

**Gathered:** 2026-01-30
**Status:** Ready for planning

<domain>
## Phase Boundary

Define explicit input/output contracts for the 9 existing agents in the Plugin Freedom System. This iterates on the current system — adds formal contracts, documents boundaries, identifies gaps — does NOT replace or rebuild the workflow.

</domain>

<decisions>
## Implementation Decisions

### Schema strictness
- Most fields required (strict) — force explicit decisions at invocation time
- Extra/unknown fields rejected (strict) — catches typos, enforces contract discipline
- Error messages must be actionable — explain what's wrong AND how to fix it

### Agent boundaries
- Key exclusions only — document actual confusion points, not exhaustive hypotheticals
- Add exclusions as confusion surfaces in practice

### Gap analysis approach
- Two-pass method: workflow tracing first, then capability matrix as verification
- Workflow scope: full lifecycle (ideation → planning → implementation → testing → packaging → distribution)
- For each gap: produce full agent spec (name, purpose, inputs, outputs, boundaries) — ready to implement
- Identify both gaps AND overlaps/redundancy — flag agents that should potentially merge
- Prioritize gaps with rationale — rank by impact on workflow quality

### Contract evolution
- Full semver versioning (MAJOR.MINOR.PATCH)
- Centralized changelog for all agent contract changes (not per-agent)

### Claude's Discretion
- Validation behavior per agent (fail fast vs warn and attempt)
- Overlap resolution when two agents could handle something
- Whether boundaries reference other agents explicitly
- Whether contracts include pre/post conditions vs just inputs/outputs
- Breaking change handling (deprecation period vs clean break)
- Contract file location (dedicated dir vs embedded in agent files)

</decisions>

<specifics>
## Specific Ideas

- Strict schema enforcement aligns with Phase 1's goal of establishing discipline
- Gap analysis should reveal the *real* confusion points to document as exclusions

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-agent-contracts*
*Context gathered: 2026-01-30*
