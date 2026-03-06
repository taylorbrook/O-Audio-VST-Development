# Phase 18: Dead Code Removal - Context

**Gathered:** 2026-03-05
**Status:** Ready for planning

<domain>
## Phase Boundary

Delete all dead `.sh` hooks (10 files, 816 lines), vestigial `hooks.json`, 3 unreferenced agent definitions (aesthetics-agent, dynamic-researcher, music-theory-agent — 473 lines), deprecated `plugin-registry.json`, and add `__pycache__` to `.gitignore`. This establishes a clean baseline before Phase 19 activates new hooks.

</domain>

<decisions>
## Implementation Decisions

### Reference cleanup depth
- Full scrub: delete target files AND remove all references across skill files, workflow docs, agent-profiles.json entries, memory files, and configs
- Clean settings.json of any lingering shell hook references and related documentation
- Full cleanup for plugin-registry.json — delete the file and remove any code/scripts/tools that reference it
- For hooks.json deletion documentation: Claude's discretion on whether a comment in settings.json is warranted

### Additional dead code sweep
- Delete obviously dead code encountered while working on the 5 named targets
- After completing DEAD-01 through DEAD-05, perform a deep scan of `.claude/` directory for any file not referenced by active skills, hooks, or workflows
- Document all additional findings in `.planning/phases/18-dead-code-removal/dead-code-audit.md` — do NOT delete additional findings in this phase (separate tracking for future phases)

### Commit strategy
- One commit per DEAD-xx requirement (5 commits total)
- One additional commit for the dead-code-audit.md document
- Reference cleanup (scrubbing stale mentions) bundled in same or separate commit at Claude's discretion based on volume
- Commit message prefix at Claude's discretion (conventional commits)

### Safety verification
- Trust the v1.4 milestone audit that identified these targets, with quick confirmation grep before deletion
- For .sh hook files: verify each has a corresponding .py hook in settings.json before deleting
- For __pycache__ (DEAD-05): run `git rm --cached` on any tracked `__pycache__/*.pyc` files, then add patterns to `.gitignore`
- Unexpected findings (e.g., hook with no Python equivalent, agent that IS referenced): Claude uses judgment — stop for risky items, continue for trivial ones

### Claude's Discretion
- Commit message prefixes (chore, refactor, etc.) per deletion type
- Whether hooks.json deletion warrants a documentation note in settings.json
- Grouping reference cleanup with deletions vs. separate commits based on volume
- Handling unexpected verification findings (stop vs. flag based on risk)

</decisions>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches. The success criteria in the roadmap are binary (file exists or doesn't) and well-defined.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 18-dead-code-removal*
*Context gathered: 2026-03-05*
