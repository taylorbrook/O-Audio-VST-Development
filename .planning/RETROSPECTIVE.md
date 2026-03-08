# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.4 -- System Hygiene & Quality Gates

**Shipped:** 2026-03-07
**Phases:** 5 | **Plans:** 13 | **Tasks:** 24

### What Was Built
- Dead code purge: removed 10 .sh hooks, hooks.json, 3 dead agents, deprecated registry, __pycache__ from git
- 3 quality gates activated: SubagentStop contract validation, research frontmatter validation, resource index auto-regeneration
- Research corpus expanded from 27 to 64 documents with standardized 5-field YAML frontmatter
- Skill consolidation: plugin-phases merged into plugin-workflow
- Agent memory infrastructure: 4 agents seeded with production patterns + write-back mechanism for persistent learning
- Infrastructure cleanup: validation cache removed, canary-test deduplicated, doc files relocated

### What Worked
- Full system review (QT-14) as milestone input was highly effective -- data-driven prioritization eliminated guesswork about what to clean up
- Yolo mode for hygiene/cleanup work was appropriate -- low-risk changes with clear scope don't need confirmation gates
- Gap closure plans (18-04) caught a real miss -- hooks.json was still on disk despite Phase 18-01 declaring it absent
- Coverage audit as living document pattern -- reusable for future audits without rebuilding from scratch
- Milestone audit before completion caught no gaps -- clean execution across 23 requirements

### What Was Inefficient
- Phase 18 needed 4 plans for what should have been 2 -- the hooks.json miss (18-04) was a verification failure in 18-01
- SUMMARY.md files lack `one_liner` frontmatter field -- had to manually extract accomplishments during milestone completion
- Some plans touched only 1 file but still went through full plan/summary cycle -- overhead for trivial changes

### Patterns Established
- Full system review -> milestone requirements is a reliable pipeline for hygiene work
- 5-field minimum frontmatter standard enforced by active hook (not just documentation)
- Agent memory read/write loop: inject on start, write-back on stop, deduplication prevents bloat
- Quality gates as PostToolUse hooks (not pre-commit) for zero-friction enforcement
- Gap closure plans for audit findings -- decimal phases or additional plans to close verified gaps

### Key Lessons
1. Verify deletion claims immediately -- "file doesn't exist" assertions need `ls` confirmation, not just grep
2. Cleanup milestones benefit from deep audit before starting -- the system review identified 15 items, the milestone addressed all actionable ones
3. Agent memory files need the write-back mechanism to be useful long-term -- seed patterns alone decay without real session data
4. Research corpus governance requires active enforcement (hooks) not passive standards (documentation)

### Cost Observations
- Model mix: 100% Opus 4.6
- Sessions: ~6 sessions across 2 days
- Notable: Most plans completed in 1-5 minutes each -- cleanup work is fast when scope is clear
- 78 commits in milestone range (includes some non-milestone work interleaved)

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Phases | Plans | Key Change |
|-----------|--------|-------|------------|
| v1.0 | 7 | 21 | Foundation -- agents, contracts, quality gates |
| v1.1 | 2 | 4 | Cleanup -- repo size, planning workflow |
| v1.2 | 4 | 12 | Intelligence -- resource discovery, context injection |
| v1.3 | 4 | 14 | Modernization -- Opus 4.6, persistence, deduplication |
| v1.4 | 5 | 13 | Hygiene -- dead code, quality gates, research governance |

### Cumulative Stats

| Milestone | Phases | Plans | Requirements | Timeline |
|-----------|--------|-------|--------------|----------|
| v1.0 | 7 | 21 | 35 | 2 days |
| v1.1 | 2 | 4 | 13 | 2 days |
| v1.2 | 4 | 12 | 15 | 2 days |
| v1.3 | 4 | 14 | 22 | 3 days |
| v1.4 | 5 | 13 | 23 | 2 days |
| **Total** | **22** | **64** | **108** | **11 days** |

### Top Lessons (Verified Across Milestones)

1. Data-driven prioritization beats guesswork -- system reviews, coverage audits, and milestone audits consistently produce better outcomes than intuition-based planning
2. Active enforcement > passive documentation -- hooks that block bad input are more reliable than README instructions (validated in v1.2 resource discovery, v1.4 frontmatter validation)
3. Archive before delete -- always create milestone archives before removing files (consistent pattern since v1.1)
4. Small cleanup milestones are fast -- hygiene work with clear scope completes in 2-3 days vs. 3+ for feature milestones
