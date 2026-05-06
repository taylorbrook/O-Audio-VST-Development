# Phase 11: Context Injection Pipeline - Context

**Gathered:** 2026-02-05
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver discovered research resources to agents automatically as part of their execution context. Skill orchestrators call the discovery engine (Phase 10) and inject relevant excerpted content into agent prompts before Task() calls. Agents receive knowledge without manual prompt construction. The resource index, discovery scoring, and manifest format are already built (Phase 10) — this phase connects discovery output to agent input.

</domain>

<decisions>
## Implementation Decisions

### Injection trigger point
- **Skills only** — orchestrator-level injection, not hooks (aligns with Phase 10 decision to avoid hook timeout constraints)
- **All stage-spawning skills** get injection — any skill that calls Task() with a stage agent (dsp-agent, gui-agent, foundation-shell-agent, research-planning-agent, etc.), not just the big three
- **Shared utility function** — one callable function/script that any skill invokes with (plugin_name, stage, agent_type) and gets back a formatted context block. DRY, consistent across all skills

### Content format & presentation
- **Excerpted content** — the utility reads top-ranked resource files and extracts key sections (frontmatter summary + relevant portions). Agents receive actual content inline, not just paths
- Fits within the 4,000 token budget cap per agent invocation
- Agents don't need to Read files themselves — knowledge is delivered ready to consume

### Tier-based delivery rules
- **Primary tier (score >= 0.75) = MUST-READ** — flagged for Phase 12 accountability. Agents are expected to consult these
- **Stage-specific pattern files auto-inject** — files like stage-2-patterns.md are always injected for matching agents, bypassing score threshold. These are curated for that exact context
- Claude decides: tier budget split, resource cap per agent, and whether secondary tier gets one-line mentions vs full exclusion

### Fallback & missing resources
- **Script failure = continue without injection** — log warning, skip injection, agent proceeds normally. No workflow interruption (aligns with Phase 13 graceful degradation requirement)
- **Missing files on disk = skip with warning** — if a manifest reference points to a deleted/moved file, skip it and log a warning. Don't crash injection for one bad reference
- Claude decides: zero-result behavior, logging approach (console vs file vs silent)

### Claude's Discretion
- Prompt delimiter style (XML tags vs markdown sections) for injected content
- Whether to show relevance scores to agents or just order by relevance
- Resource cap per agent (3 vs 5 vs budget-limited)
- Token budget split between primary and secondary tiers
- Logging implementation (console, file, or both)
- Zero-result behavior (silent skip vs brief note)

</decisions>

<specifics>
## Specific Ideas

- The shared utility should be a Python script consistent with existing Phase 10 tooling (discover-resources.py, generate-resource-index.py)
- Stage-specific troubleshooting patterns (e.g., stage-2-patterns.md) should feel like "always-on" resources for their matching agent — the agent shouldn't have to score into them
- The 4,000 token budget was established as a hard cap in Phase 10 decisions — injection must respect this

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 11-context-injection-pipeline*
*Context gathered: 2026-02-05*
