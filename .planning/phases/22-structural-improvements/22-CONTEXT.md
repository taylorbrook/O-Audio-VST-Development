# Phase 22: Structural Improvements - Context

**Gathered:** 2026-03-06
**Status:** Ready for planning

<domain>
## Phase Boundary

Build an agent memory write-back mechanism so agents persist learnings across sessions, and resolve two pieces of dead infrastructure: the validation cache system and duplicate canary test scripts. Phase 21's agent memory cleanup (seed patterns) must be complete before this phase builds on top of it.

</domain>

<decisions>
## Implementation Decisions

### Agent memory write-back
- Trigger: Post-agent hook that runs automatically after every subagent completes
- Content: Focus on error patterns and fixes encountered during the session — most actionable for future sessions
- Approval: No user approval required — hook writes directly to memory files, fully automatic
- Target: Write to existing per-agent memory files (researcher, planner, executor, verifier) based on which agent ran
- No manual capture command needed — the hook handles everything

### Validation cache removal
- Decision: Remove all dead infrastructure entirely — the system works fine without caching
- Scope: Full scrub — delete files (validation-cache.md, validation-cache.sh, empty cache file) AND remove all references to validation-cache in skills, docs, or config
- No activation path — simpler to remove dead code than maintain unused caching

### Canary test cleanup
- If canary-test.sh duplicates canary-test.py: delete the .sh version
- If canary-test.sh has unique functionality: port that functionality into canary-test.py, then delete .sh — single source of truth in Python
- End state: only canary-test.py exists, with complete functionality

### Claude's Discretion
- How to extract learnings from agent session transcripts (parsing strategy)
- Memory file format for appended learnings (headings, dedup logic)
- Which specific validation-cache references exist and need scrubbing
- How to compare canary-test.sh vs .py (diff strategy)

</decisions>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 22-structural-improvements*
*Context gathered: 2026-03-06*
