# Phase 20: Research Governance & Review - Context

**Gathered:** 2026-03-06
**Status:** Ready for planning

<domain>
## Phase Boundary

Organize and standardize the research corpus: complete the resource index to cover all docs, normalize YAML frontmatter across all 49 research documents, audit coverage gaps against existing plugins, fill gaps with new research docs (user-approved), and flag stale content referencing deprecated APIs. Does NOT include rewriting stale docs or creating research for planned/future plugins.

</domain>

<decisions>
## Implementation Decisions

### Frontmatter Standard
- All 49 research docs must have valid YAML frontmatter
- Required fields (5): title, created, domain, type, keywords
- Additional fields (last_verified, juce_version, summary, stages) are optional — not enforced
- Auto-generate frontmatter for the 24 docs currently missing it by inferring values from content (title from H1, created from git history, domain/type/keywords from content analysis)
- Batch apply and commit without per-doc review

### Domain Vocabulary (Fixed Set)
- Controlled vocabulary for `domain` field: dsp, ui, architecture, tooling, market-research, ml, spatial-audio, cross-platform
- No freeform domain values allowed

### Type Vocabulary (Fixed Set)
- Controlled vocabulary for `type` field: research, algorithm, guide, market-research
- No freeform type values allowed

### Coverage Gap Criteria
- Gaps defined by: domains/techniques used in existing plugins that have no corresponding research doc
- Audit scope: existing plugins only (not planned/future plugins)
- One doc per gap topic (not grouped)
- Produce a standalone `research/coverage-audit.md` as a living reference listing all domains and their coverage status

### Gap-Fill Doc Quality
- Full deep-dives matching existing doc quality (comprehensive coverage, references, implementation notes)
- Include JUCE C++ code examples but don't limit to JUCE — broader techniques and approaches welcome
- Use web research (deep-research agents) for accuracy and currency
- Identify every gap in the audit, but each new research doc requires user approval before creation

### Staleness Policy
- Stale trigger: doc references deprecated APIs or pre-JUCE 8 patterns
- Detection method: known deprecated pattern list (deterministic, not content analysis)
- Flag method: add `status: stale` to frontmatter
- Flag only in this phase — refreshing/rewriting stale docs is deferred to future work

### Claude's Discretion
- Exact deprecated API pattern list (curated from JUCE 8 migration knowledge)
- Keyword inference strategy for docs missing frontmatter
- Coverage audit document format and organization
- Order of operations across the sub-tasks

</decisions>

<specifics>
## Specific Ideas

- "Use JUCE C++ but don't allow yourself to be limited by that" — gap-fill docs should include JUCE implementation but also cover broader techniques and cross-platform approaches
- User wants manual approval gate on each new research doc creation — identify all gaps first, then create docs one-by-one with approval

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 20-research-governance-review*
*Context gathered: 2026-03-06*
