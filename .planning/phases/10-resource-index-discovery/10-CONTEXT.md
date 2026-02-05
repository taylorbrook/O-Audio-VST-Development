# Phase 10: Resource Index & Discovery - Context

**Gathered:** 2026-02-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Build a static manifest (`resource-index.json`) cataloging all ~23 research documents and a keyword-based discovery script that returns relevant resources given a task context (agent role, stage, plugin). Injection into agents is Phase 11. Accountability is Phase 12. Maintenance tooling is Phase 13.

</domain>

<decisions>
## Implementation Decisions

### Manifest structure
- Each document entry includes an inline summary (2-3 sentences) so discovery can rank without reading files
- YAML frontmatter in each research document is the source of truth for metadata (keywords, categories, stages, agents)
- Manifest (`resource-index.json`) is derived/generated from frontmatter — not manually maintained
- A hook validates that new research documents include required frontmatter fields before commit

### Discovery ranking
- Return all results above a relevance threshold (not a fixed count) — could be 1 or 10 depending on the query
- Stage and agent-role matching takes priority over keyword relevance — context trumps keywords
- When stage doesn't match, a strong keyword hit can still surface a doc, but it ranks lower than stage-matched results

### Resource categorization
- Each document gets both a domain tag (dsp, ui, build, workflow) AND a type tag (algorithm, pattern, guide, reference)
- Stage applicability listed explicitly per document (e.g., `stages: [1, 2, 3, 4]`) — no special "universal" concept
- Agent applicability uses roles (dsp, ui, build, research) not named agents — new agents just declare their role
- Cross-cutting resources tagged with all applicable stages, not a special flag

### Developer experience
- Discovery script is internal only — called programmatically by hooks/orchestrators, no human-facing CLI
- Empty results return an empty array silently — no warnings or noise
- Manifest validated against JSON schema on every run before querying

### Claude's Discretion
- Entry key format (file path vs slug-based ID)
- Metadata fields beyond keywords and summary (what downstream agents need)
- Manifest structure (flat vs nested)
- Query interface design (structured fields vs other approaches)
- Output format (JSON vs other)
- Priority tiering in results (must-read vs supplementary, or flat ranked list)

</decisions>

<specifics>
## Specific Ideas

- Hook-based validation for new research documents: when a research agent writes a new doc, a hook checks that YAML frontmatter exists with required fields (keywords, categories, stages, agent roles) before the document is committed
- Frontmatter as source of truth aligns with Phase 13 auto-generation — manifest rebuilds from frontmatter without manual JSON editing

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 10-resource-index-discovery*
*Context gathered: 2026-02-04*
