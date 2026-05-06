# Phase 3: Structured Handoffs - Context

**Gathered:** 2026-01-30
**Status:** Ready for planning

<domain>
## Phase Boundary

Stage transitions in the plugin workflow preserve context through schema-validated handoff documents. Each stage boundary (0→1, 1→2, 2→3, 3→4) has a structured handoff format that captures decisions, artifacts, and state needed by the receiving stage. This is infrastructure for agents — handoffs define what information flows between stages.

</domain>

<decisions>
## Implementation Decisions

### Handoff Content
- **Summary + refs pattern:** Key decisions summarized inline, refs to full artifacts for details (matches GSD)
- **Stage-specific schemas:** Each boundary has its own schema (0→1 differs from 2→3)
- **What + why:** Include reasoning so receiving agent understands trade-offs considered
- **All defined fields required:** Every field in schema must be present — no optional fields
- **Hybrid state refs:** Reference external files (STATUS.md, registry.json) but inline critical state for resilience
- **Human-readable summary:** Quick overview at top before structured data
- **Location:** Claude decides based on plugin isolation (likely stage directory under plugin's .planning/)

### Validation Strictness
- **Hard fail on validation errors:** Stage transition blocked until handoff passes validation
- **Bypass flag:** Claude decides whether --force flag appropriate for advanced users
- **Strict version match:** Handoff must match exact schema version — no forward/backward compatibility
- **Validate on write + at boundary:** Validate at creation (fail fast) and again at transition (confirm at gate)

### Decision Audit Trail
- **Key decisions only:** Track major choices affecting architecture/behavior, not micro-decisions
- **Full metadata:** Decision + rationale + date + alternatives considered
- **Both inline + summary log:** Decisions in handoff document + central DECISIONS.md per plugin
- **Mutable with git history:** Edits allowed, git tracks history; use supersession pattern for real changes

### Versioning Strategy
- **Semver (major.minor.patch):** Standard semantic versioning
- **Major bump trigger:** Required field added = breaking change = major version
- **No migration:** Old handoffs stay on their schema version; new plugins get current schemas
- **Version declared both places:** Schema file declares canonical version, handoff references specific version

### Claude's Discretion
- Handoff document file location (stage directory vs workflow directory)
- Whether to implement --force bypass flag
- Schema file naming conventions
- Specific structure of human-readable summary section

</decisions>

<specifics>
## Specific Ideas

- Follow GSD's summary + refs pattern throughout — agents get quick context without loading multiple files
- Pattern for state snapshot in handoffs:
  ```markdown
  ## State Snapshot
  Stage: 2 (DSP)
  Phase: Execute
  Parameters: 5 defined
  Source: STATUS.md, registry.json
  ```
- Decision supersession pattern for audit trail:
  ```markdown
  | Pagination (supersedes infinite scroll) | Performance issues | Keep infinite | 2026-01-25 |
  ```
- Handoff version declaration pattern:
  ```yaml
  ---
  schema: stage-1-to-2-handoff
  schema_version: 1.2.0
  ---
  ```

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 03-structured-handoffs*
*Context gathered: 2026-01-30*
