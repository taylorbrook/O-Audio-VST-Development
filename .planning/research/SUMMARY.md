# Research Summary: v1.1 Cleanup & Workflow Polish

**Project:** Plugin Freedom System
**Milestone:** v1.1
**Researched:** 2026-02-01
**Confidence:** HIGH

## Executive Summary

Milestone v1.1 addresses two distinct but complementary improvements to the Plugin Freedom System: (1) repository cleanup to remove ~500MB of build artifacts from git history, and (2) adding a planning phase to the plugin-improve workflow for complex (Tier 2/3) improvements. Both improvements are well-understood with established tooling and patterns.

**Repository cleanup** should use git-filter-repo (not BFG or deprecated filter-branch) because it supports path-based filtering needed to target `build/` directories specifically. The current `.git` directory is 584MB with 557MB in packs, primarily from 30-55MB static library archives accidentally committed. Expected reduction is 80-90%, bringing the repository to a manageable ~50-80MB. The critical coordination requirement is that all existing clones become invalid after history rewrite - this is a solo project so the impact is minimal, but CI/CD caches must be cleared.

**Workflow enhancement** should follow the conditional middleware pattern already established in the plugin-improve skill (e.g., Phase 5.5 regression testing). A new Phase 0.6 Planning triggers only for Tier 2/3 investigations, preserving the fast path for simple fixes. This matches existing phase numbering (0.3, 0.4, 0.45, 0.5, 0.9) and requires minimal changes: one new reference file, one template, and a SKILL.md update.

## Key Findings

### Repository Cleanup

**From STACK.md:**
- git-filter-repo is the recommended tool (Python-based, path filtering, actively maintained)
- Current `.git` size: 584MB with 557MB in packs
- Largest offenders: `build/plugins/*/lib*.a` files (30-55MB each)
- System git version (2.50.1) exceeds requirements (2.36.0+)
- BFG Repo-Cleaner rejected: requires Java, filename-only filtering, skips HEAD by default

**From FEATURES.md:**
- Table stakes: backup creation, large file detection, garbage collection, verification report
- Differentiators: dry-run mode, pattern-based cleanup, .gitignore validation
- Anti-features: automatic cleanup without backup, force-push automation

**From PITFALLS.md (Pitfalls 14-19):**
- Pitfall 14: Breaking existing clones (HIGH impact) - all clones invalid after rewrite
- Pitfall 15: git-filter-repo removes origin remote intentionally - must re-add
- Pitfall 17: CI/CD pipeline failures from cached SHA references
- Pitfall 18: Size not reduced immediately - requires aggressive gc and waiting for GitHub
- Pitfall 19: Using deprecated filter-branch - 10-720x slower and error-prone

### Workflow Planning Phase

**From STACK.md:**
- Recommended pattern: Conditional Middleware (matches existing Phase 5.5)
- Trigger condition: Tier 2 or Tier 3 investigation complexity
- Skip planning for Tier 1 (simple fixes don't need overhead)
- GSD methodology emphasizes atomic tasks with verification

**From FEATURES.md:**
- Table stakes: implementation plan document, task decomposition, architecture decisions, verification criteria
- Differentiators: affected component analysis, backward compatibility check, complexity estimation
- Anti-features: planning without investigation, overly detailed plans, auto-generated code from plans

**From ARCHITECTURE.md:**
- Insert Phase 0.6 between Phase 0.5 (Investigation) and Phase 0.9 (Backup)
- Deep-research handoff (Phase 0.45) skips planning - research already includes recommendations
- New artifacts: `references/planning-protocol.md`, `assets/planning-template.md`
- Pattern: Generator-Critic validation recommended for complex improvements

**From PITFALLS.md (Pitfalls 20-23):**
- Pitfall 20: Complexity creep (HIGH impact) - preserve simple path for quick fixes
- Pitfall 21: In-flight plugins (O-IntonationPad Stage 4, O-Freeze Stage 0) - new phases must be additive
- Pitfall 22: GitHub Actions trigger compatibility - test in fork first
- Pitfall 23: Workflow schema drift - update schemas when adding fields

## Recommended Stack

### Git Cleanup
| Tool | Version | Purpose |
|------|---------|---------|
| git-filter-repo | 2.47.0+ | Path-based history rewriting |
| Python 3 | Already installed | git-filter-repo dependency |
| Git | 2.50.1 (installed) | Exceeds 2.36.0+ requirement |

**Install:** `brew install git-filter-repo`

### Workflow Enhancement
| Component | Purpose |
|-----------|---------|
| SKILL.md modification | Add Phase 0.6 section with conditional trigger |
| planning-protocol.md | Reference file documenting planning process |
| planning-template.md | Asset template for improvement plans |
| Existing tier detection | Reuse Phase 0.5 tier detection unchanged |

## Critical Pitfalls

**Top 5 pitfalls to watch for in v1.1:**

1. **Breaking existing clones after history rewrite** (Pitfall 14) - Announce cleanup, clear CI caches, document re-clone requirement. Since this is a solo project, impact is limited to CI/CD pipelines.

2. **Complexity creep in plugin-improve** (Pitfall 20) - Design for simple path first. Tier 1 fixes must remain as fast as before. Planning phase is conditional, not mandatory.

3. **In-flight plugin incompatibility** (Pitfall 21) - O-IntonationPad (Stage 4) and O-Freeze (Stage 0) are active. New phases must be additive only, not modify existing stage definitions.

4. **Using wrong cleanup tool** (Pitfall 19) - Explicitly document git-filter-repo requirement. Reject any suggestion to use deprecated filter-branch.

5. **Remote origin removed by git-filter-repo** (Pitfall 15) - Document the expected post-rewrite step to re-add origin. This is intentional safety behavior, not a bug.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Git Repository Cleanup

**Rationale:** Independent of workflow changes, can be done first. High impact (reduced clone times, smaller repo). Low complexity with well-documented tooling.

**Delivers:**
- Repository reduced from ~584MB to ~50-80MB
- Clean history without build artifacts
- Updated .gitignore validation

**Tasks:**
1. Install git-filter-repo (`brew install git-filter-repo`)
2. Create backup branch and document current state
3. Execute cleanup (remove build/, *.o, .DS_Store from history)
4. Re-add origin remote and force push
5. Clear GitHub Actions caches
6. Verify size reduction

**Avoids:** Pitfalls 14, 15, 17, 18, 19

**Research needed:** None - patterns well-documented

### Phase 2: Planning Phase Implementation

**Rationale:** Builds on existing plugin-improve workflow. Minimal changes required. High value for complex improvements (reduces rework).

**Delivers:**
- Phase 0.6 Planning in plugin-improve workflow
- Conditional trigger on Tier 2/3 investigations
- Planning template and protocol documentation
- Preserved fast path for Tier 1 fixes

**Tasks:**
1. Create `references/planning-protocol.md` with:
   - Planning trigger conditions (Tier 2/3 only)
   - Planning process steps
   - Decision menu format (Approve/Revise/Skip/Cancel)

2. Create `assets/planning-template.md` with:
   - Plan structure (objective, architecture decisions, tasks)
   - Risk assessment section
   - Testing checklist

3. Update SKILL.md:
   - Add Phase 0.6 section after Phase 0.5
   - Add conditional branch based on tier
   - Update workflow diagram
   - Update progress checklist

4. Test the three paths:
   - Tier 1 improvement (should skip planning)
   - Tier 2 improvement (should trigger planning)
   - Deep-research handoff (should skip both investigation AND planning)

**Avoids:** Pitfalls 20, 21, 22, 23

**Research needed:** None - patterns documented in existing SKILL.md

### Phase Ordering Rationale

1. **Git cleanup first** because it's independent and provides immediate benefit (faster clones, smaller repo). No dependencies on workflow changes.

2. **Workflow enhancement second** because it builds on existing patterns and is safer to test after repository cleanup is complete.

3. **Both phases are low-to-medium complexity** with well-established patterns. Neither requires `/gsd:research-phase` during planning - the research is already complete.

### Research Flags

**Phases with standard patterns (skip research-phase):**
- **Phase 1 (Git Cleanup):** Tool usage is well-documented, commands verified in STACK.md
- **Phase 2 (Workflow):** Pattern matches existing Phase 5.5, implementation path clear from ARCHITECTURE.md

**No phases need additional research** - this milestone is incremental improvement using established tools and patterns.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | git-filter-repo verified, version requirements checked |
| Features | HIGH | Table stakes clear, derived from existing codebase patterns |
| Architecture | HIGH | Conditional phase pattern matches existing Phase 5.5 |
| Pitfalls | HIGH | Multiple authoritative sources, including tool maintainers |

**Overall confidence:** HIGH

### Gaps to Address

1. **CI/CD cache clearing:** Exact GitHub Actions cache clearing process should be verified during Phase 1 execution (Settings > Actions > Caches)

2. **Express mode interaction:** The ARCHITECTURE.md research asks whether express mode should skip planning. Recommendation: Yes, but defer decision to implementation since express mode is not currently active.

3. **Plan persistence threshold:** Should Tier 2 plans persist to files or only Tier 3? Recommendation: Tier 3 only (or user request). Simple enough to adjust later.

## Sources

### Primary (HIGH confidence)
- [git-filter-repo GitHub](https://github.com/newren/git-filter-repo) - Official tool documentation
- [BFG vs git-filter-repo comparison](https://github.com/newren/git-filter-repo/blob/main/Documentation/converting-from-bfg-repo-cleaner.md)
- [GitHub Docs: Removing Sensitive Data](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository)
- Local codebase: `.claude/skills/plugin-improve/SKILL.md` - Existing phase patterns

### Secondary (MEDIUM confidence)
- [Architecture Decision Records](https://adr.github.io/) - ADR format for planning template
- [Addy Osmani's LLM Coding Workflow](https://addyosmani.com/blog/ai-coding-workflow/) - Planning-first approach
- [GitHub Actions Breaking Changes](https://github.blog/changelog/2025-02-12-notice-of-upcoming-deprecations-and-breaking-changes-for-github-actions/)

### Local Analysis
- `registry.json` - Verified in-flight plugins (O-IntonationPad Stage 4, O-Freeze Stage 0)
- `.git` directory analysis - 584MB total, 557MB pack size
- `git count-objects -v` - Object statistics confirming cleanup targets

---
*Research synthesized: 2026-02-01*
*Ready for roadmap: yes*
