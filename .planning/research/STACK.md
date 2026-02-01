# Technology Stack: Multi-Agent AI Workflows for Plugin Development

**Project:** Plugin Freedom System Overhaul
**Researched:** 2026-01-29
**Dimension:** Stack/Framework for AI-assisted plugin development systems

## Executive Summary

GSD (Get Shit Done) is a context engineering framework by TACHES that orchestrates Claude Code for spec-driven development. The core insight: **context rot is the enemy**. As conversations lengthen, LLMs "forget" previous decisions and generate inconsistent code. GSD solves this through phase-based isolation, atomic task execution in fresh 200k-token subagent contexts, and file-based state persistence.

For multi-agent systems in 2026, the field is undergoing its "microservices revolution." Single all-purpose agents are being replaced by orchestrated teams of specialized agents. The key is **simplicity with constraints**—successful implementations use surprisingly few tools (10-20), delegate complexity to code execution, and treat context as a finite resource to be carefully managed.

## GSD Framework Deep Dive

### What GSD Actually Is

**Confidence: HIGH** (Verified via GitHub repositories, multiple independent sources)

GSD is a meta-prompting and context engineering system for Claude Code that provides:

| Component | Purpose | How It Works |
|-----------|---------|--------------|
| Phase workflow | Structure work | discuss → research → plan → execute → verify |
| Subagent isolation | Prevent context rot | Each task runs in fresh 200k token window |
| File-based state | Cross-session persistence | PROJECT.md, ROADMAP.md, STATE.md track everything |
| Atomic commits | Traceability | Every task = one revertable commit |
| XML task format | Clear instructions | Structured `<task>` elements with verification steps |

### GSD Phase Model

```
/gsd:discuss-phase [N]  → Capture implementation preferences
/gsd:plan-phase [N]     → Research + create atomic tasks (max 3 per plan)
/gsd:execute-phase [N]  → Parallel execution, fresh contexts
/gsd:verify-work [N]    → Manual UAT + automated diagnostics
```

**Critical principle:** Plans contain maximum 3 tasks. Each task runs in isolation. This prevents the cascading confusion that occurs when Claude tries to track too much in one context window.

### Why GSD Works

**Confidence: HIGH** (Anthropic research confirms these patterns)

1. **Context is finite**: LLMs have an "attention budget" where each token depletes capacity. Context engineering means "filling the context window with just the right information for the next step."

2. **Isolation prevents pollution**: Subagents run with separate context windows, tools, and instructions. Only relevant summaries return to the orchestrator.

3. **File-based state survives sessions**: Writing intermediate results, plans, and progress to files prevents information loss. Git history communicates progress across agent loops.

4. **Atomicity enables rollback**: One task = one commit. Failed tasks can be reverted without unwinding unrelated work.

## Multi-Agent Design Patterns for 2026

### Recommended Patterns

**Confidence: HIGH** (Google, Anthropic, LangChain all converge on these)

| Pattern | When to Use | Your System's Application |
|---------|-------------|---------------------------|
| **Sequential Pipeline** | Linear, deterministic workflows | Stage progression (foundation → DSP → GUI → polish) |
| **Coordinator/Dispatcher** | Route to specialists by intent | Phase commands dispatching to stage-specific agents |
| **Parallel Fan-out/Gather** | Independent investigations | Research agents investigating stack, features, architecture in parallel |
| **Generator/Critic** | Output reliability critical | Execute then verify pattern (DSP agent + validation agent) |
| **Hierarchical Decomposition** | Complex goals needing breakdown | Main orchestrator → stage agents → task subagents |

### Subagent Design Principles

**Confidence: HIGH** (Claude Code official documentation)

```yaml
# Effective subagent frontmatter pattern
---
name: dsp-implementer
description: Expert DSP implementation for audio plugins. Use for Stage 2 work.
tools: Read, Write, Edit, Bash, Grep, Glob
model: inherit  # Use sonnet for analysis, inherit for implementation
permissionMode: default
skills:
  - dsp-patterns
  - juce-conventions
---

Focused system prompt here. One clear responsibility.
```

**Key principles:**
- **Preserve context** by keeping exploration and implementation out of main conversation
- **Enforce constraints** by limiting which tools a subagent can use
- **Specialize behavior** with focused system prompts for specific domains
- **Control costs** by routing tasks to faster, cheaper models when appropriate

### Tool Management

**Confidence: HIGH** (Anthropic engineering blog)

| Recommendation | Rationale |
|----------------|-----------|
| Use 10-20 tools maximum | More tools = context overload + model confusion |
| Delegate to Bash/code | Complex operations via shell, not proliferating tools |
| Progressive disclosure | Load tool definitions on-demand, not all upfront |
| Cache strategically | Cache hit rate is the #1 production metric |

**From Anthropic:** "Many popular general purpose agents use a surprisingly small number of tools. Claude Code uses around a dozen tools. Manus uses fewer than 20 tools."

## What NOT To Do (Anti-Patterns)

### Critical Anti-Patterns to Avoid

**Confidence: HIGH** (Multiple enterprise case studies, 2025-2026)

| Anti-Pattern | Why It Fails | What To Do Instead |
|--------------|--------------|-------------------|
| **Monolithic Agent** | Single agent with too many responsibilities degrades as complexity increases, hallucinations compound | Decompose into specialized agents with clear boundaries |
| **Context Overload** | Dumping all context hoping LLM figures it out is like "dumping entire hard drive into RAM" | Curate context deliberately; use file-based storage |
| **Premature Tool Proliferation** | Tool definitions consume tokens; overlapping tools confuse models | Start minimal; add tools only when needed |
| **Ignoring Atomicity** | Partial failures leave inconsistent state | Treat atomicity as infrastructure requirement; implement rollback |
| **Treating Agents as Drop-ins** | Agents are architectural components, not replacements for existing code | Redesign workflows around agent capabilities |
| **Layering on Legacy** | Adding agents to old processes without redesign consistently fails to scale | Rethink workflows rather than automate existing friction |

### Your System's Specific Risks

Based on PROJECT.md known problems:

| Risk | Current Symptom | Recommended Fix |
|------|-----------------|-----------------|
| Agent boundary confusion | "agents try to do too much or too little" | Define explicit contracts: inputs, outputs, success criteria |
| Context loss between sessions | "state tracking breaks between sessions" | Strengthen file-based state; use GSD's STATE.md pattern |
| Rework loops | "output quality requires many iteration cycles" | Add Generator/Critic pattern; verify before proceeding |
| Unclear handoffs | "transitions aren't smooth" | Document handoff protocols with required artifacts |

## Agent Design Best Practices

### Contract-First Design

**Confidence: HIGH** (Production multi-agent systems consensus)

Every agent needs:

```markdown
## Agent Contract

### Inputs (Required before invocation)
- [Specific artifact, e.g., "parameter-spec.md exists"]
- [Context file, e.g., "STATUS.md shows stage=2"]

### Outputs (Guaranteed on success)
- [Deliverable, e.g., "PluginProcessor.cpp with DSP implementation"]
- [State update, e.g., "STATUS.md updated to stage=2, phase=verify"]

### Success Criteria (How to verify completion)
- [ ] [Testable condition, e.g., "ninja build succeeds"]
- [ ] [Quality gate, e.g., "pluginval passes level 5"]
```

### Permission Modes by Agent Role

| Role | Permission Mode | Tools | Rationale |
|------|----------------|-------|-----------|
| Researcher | `plan` (read-only) | Read, Grep, Glob, WebSearch | Exploration shouldn't modify |
| Implementer | `default` | All | Needs write access with approval |
| Verifier | `dontAsk` | Read, Bash (test commands) | Automated validation, no prompts |
| Troubleshooter | `default` | All | May need investigative modifications |

### Skill Injection vs. Dynamic Loading

**Confidence: MEDIUM** (Emerging best practice)

Use `skills` frontmatter to **preload** domain knowledge:
```yaml
skills:
  - dsp-patterns
  - juce-audio-threading
```

This injects full skill content at startup—agent doesn't need to discover and load during execution. Better for consistent behavior.

## Production Readiness Checklist

**Confidence: MEDIUM** (Based on 2026 enterprise patterns)

| Requirement | Implementation |
|-------------|----------------|
| **Observability** | Log every user request, agent plan, tool call |
| **Evaluation metrics** | Define success criteria per agent, track over time |
| **Human-in-the-loop** | Gate risky operations; hooks suggest, humans approve |
| **Graceful degradation** | When agents fail, preserve state for recovery |
| **Atomicity** | Idempotent operations, checkpointing, safe rollbacks |
| **Caching** | Cache hit rate is economically critical |

## Recommended Stack for Your System

### Core Framework

| Technology | Purpose | Why |
|------------|---------|-----|
| GSD methodology | Workflow orchestration | Phase isolation prevents context rot; atomic tasks enable rollback |
| Claude Code subagents | Agent execution | Native integration; fresh contexts per task |
| File-based state (.planning/) | Persistence | Survives sessions; human-readable; version-controlled |
| Git integration | Traceability | Every task = commit; enables bisect for debugging |

### Agent Architecture

| Layer | Components | Notes |
|-------|------------|-------|
| Orchestrator | Slash commands (plugin-*) | Routes to appropriate phase/stage |
| Stage Agents | foundation, dsp, gui, polish | One clear responsibility each |
| Support Agents | validation, troubleshoot, research | Cross-cutting concerns |
| Skills | 20+ domain knowledge packages | Progressive disclosure; load when needed |

### State Management

| File | Purpose | Update Frequency |
|------|---------|------------------|
| PROJECT.md | Vision, scope, constraints | Rarely (init, major changes) |
| ROADMAP.md | Phase structure, completion status | Per phase completion |
| STATE.md | Current progress, blockers, decisions | Per task |
| STATUS.md (per plugin) | Plugin-specific progress | Per stage transition |

## Implications for Roadmap

### Phase 1: Agent Contract Audit
- Define explicit input/output contracts for all 9 agents
- Identify missing agents (e.g., dedicated parameter validator?)
- Document tool requirements and permission modes

### Phase 2: State Management Hardening
- Align with GSD's file-based state pattern
- Add validation that state files exist before agent invocation
- Implement recovery from corrupted state

### Phase 3: Quality Gates
- Add Generator/Critic pattern to DSP and GUI agents
- Require verification before stage transitions
- Implement pluginval integration at key checkpoints

### Phase 4: Workflow Friction Reduction
- Streamline handoff artifacts
- Add resume capability from any state
- Implement progress visibility

## Confidence Assessment

| Area | Confidence | Reason |
|------|------------|--------|
| GSD principles | HIGH | Multiple implementations, creator documentation, consistent patterns |
| Multi-agent patterns | HIGH | Google, Anthropic, LangChain all converge |
| Subagent design | HIGH | Official Claude Code documentation |
| Production metrics | MEDIUM | Enterprise patterns emerging but not standardized |
| Your system's specific risks | MEDIUM | Based on PROJECT.md; need validation with actual usage |

## Gaps to Address

1. **Evaluation framework**: How will you measure agent quality over time?
2. **Caching strategy**: No mention in current system of cache management
3. **Hooks infrastructure**: GSD uses hooks extensively; your system has hooks but integration unclear
4. **Module system**: Listed as "unreliable" in PROJECT.md; research needed on root cause

## Sources

### Primary (HIGH Confidence)
- [GSD GitHub (glittercowboy/get-shit-done)](https://github.com/glittercowboy/get-shit-done) - Original implementation
- [GSD GitHub (b-r-a-n/gsd-claude)](https://github.com/b-r-a-n/gsd-claude) - Enhanced fork with phase-based planning
- [Claude Code Subagents Documentation](https://code.claude.com/docs/en/sub-agents) - Official Anthropic guidance
- [Anthropic: Building Effective Agents](https://www.anthropic.com/research/building-effective-agents) - Foundational principles

### Secondary (MEDIUM Confidence)
- [Google Developers: Multi-Agent Patterns in ADK](https://developers.googleblog.com/developers-guide-to-multi-agent-patterns-in-adk/) - Eight essential patterns
- [Agent Design Patterns (rlancemartin)](https://rlancemartin.github.io/2026/01/09/agent_design/) - Production considerations
- [Vellum: 2026 Guide to AI Agent Workflows](https://www.vellum.ai/blog/agentic-workflows-emerging-architectures-and-design-patterns) - Architectural vocabulary
- [LangChain: Choosing Multi-Agent Architecture](https://www.blog.langchain.com/choosing-the-right-multi-agent-architecture/) - Pattern selection guidance

### Supporting (LOW Confidence - Used for Context Only)
- Various Medium articles on GSD adoption
- Enterprise AI agent trend reports

---
*Research conducted: 2026-01-29*
*Next step: Roadmap creation using these findings*

---

# v1.1 Stack Research: Repository Cleanup & Workflow Enhancement

**Milestone:** v1.1 Improvements
**Domain:** Git history cleanup, workflow phase insertion
**Researched:** 2026-02-01
**Overall confidence:** HIGH

---

## Git History Cleanup

### Current State Analysis

| Metric | Value | Notes |
|--------|-------|-------|
| `.git` directory | 584 MB | Measured via `du -sh .git` |
| Pack size | 557 MB | From `git count-objects -v` |
| Largest objects | 30-55 MB each | Static library archives (.a files) |
| System git version | 2.50.1 | Exceeds git-filter-repo requirement (2.36.0+) |

**Primary offenders identified:**
- `build/plugins/*/lib*.a` - JUCE shared code archives (30-55 MB each)
- `plugins/*/build/` - Local plugin build directories
- `.o` files - Object files tracked in history
- `.DS_Store` files - 40+ macOS metadata files

### Recommended Tool: git-filter-repo

**Version:** 2.47.0 (current stable via Homebrew)
**Install:** `brew install git-filter-repo`
**Requirements:** Python 3.6+, Git 2.36.0+ (both satisfied)

**Why git-filter-repo over BFG Repo-Cleaner:**

| Criterion | git-filter-repo | BFG Repo-Cleaner |
|-----------|-----------------|------------------|
| Path-based filtering | YES - full path support | NO - filename only |
| Active maintenance | YES - actively developed | Less active |
| Dependency | Python (already on macOS) | Java 11+ required |
| Speed | Fast (seconds for this repo) | Faster for huge repos |
| HEAD protection | Applies to all commits | Skips HEAD (confusing) |

**Decision:** Use git-filter-repo because:
1. We need path-based filtering (e.g., `build/plugins/*` vs just `*.a`)
2. Python already available on macOS
3. Consistent behavior - filters apply to HEAD too
4. Better documentation and community support

### Cleanup Approach

**Phase 1: Pre-cleanup (Safety)**
```bash
# 1. Create full backup branch
git branch backup-before-cleanup

# 2. Ensure working directory is clean
git status --porcelain  # must be empty

# 3. Document current state
git count-objects -v > pre-cleanup-stats.txt
du -sh .git >> pre-cleanup-stats.txt

# 4. Record current remotes (git-filter-repo deletes .git/config)
git remote -v > remotes-backup.txt
```

**Phase 2: Install tool**
```bash
brew install git-filter-repo
```

**Phase 3: Execute cleanup**
```bash
# Remove build directories from history
git filter-repo --invert-paths --path build/ --path-glob 'plugins/*/build/'

# Remove object files from history
git filter-repo --invert-paths --path-glob '**/*.o'

# Remove .DS_Store files from history
git filter-repo --invert-paths --path-glob '**/.DS_Store'

# Remove backup directory from history (redundant with tags)
git filter-repo --invert-paths --path backups/
```

**Phase 4: Post-cleanup**
```bash
# Re-add remote (git-filter-repo clears .git/config)
git remote add origin <url-from-remotes-backup.txt>

# Verify size reduction
git count-objects -v > post-cleanup-stats.txt
du -sh .git >> post-cleanup-stats.txt

# Aggressive garbage collection
git reflog expire --expire=now --all
git gc --aggressive --prune=now

# Force push (DESTRUCTIVE - coordinate with team first)
git push origin --force --all
git push origin --force --tags
```

### Integration Considerations

**Impact on existing clones:**
- ALL existing clones become incompatible (different commit hashes)
- Team members MUST delete and re-clone
- CI/CD pipelines MUST be updated to fresh clone

**GitHub-specific notes:**
- GitHub caches refs; size reduction may not appear immediately
- Contact GitHub support if size doesn't decrease after 24 hours
- Consider enabling branch protection rules after cleanup

**Mitigation strategy:**
1. Announce cleanup window to team (if any)
2. Merge all open PRs before cleanup
3. Create release tag at current HEAD before cleanup
4. Execute cleanup
5. Verify size reduction locally
6. Force push
7. Notify team to re-clone

### Expected Results

| Metric | Before | After (estimated) |
|--------|--------|-------------------|
| `.git` size | 584 MB | ~50-80 MB |
| Clone time | ~60s | ~10s |
| CI cache | Large | Minimal |

**Confidence:** HIGH - based on identified objects (557 MB in packs, mostly build artifacts)

---

## Workflow Enhancement: Planning Phase Insertion

### Current plugin-improve Flow Analysis

From `/Users/taylorbrook/Dev/VST-development/.claude/skills/plugin-improve/SKILL.md`:

```
Phase 0: Specificity Detection
  |
Phase 0.3: Clarification Questions
  |
Phase 0.4: Decision Gate
  |
Phase 0.45: Research Detection (MANDATORY)
  |
Phase 0.5: Investigation (Tier 1/2/3)  <-- GAP: No planning for Tier 2/3
  |
Phase 0.9: Backup Verification
  |
Phase 1: Pre-Implementation
  |
Phase 3: Implementation
```

**Problem:** For complex (Tier 2/3) improvements, the workflow jumps directly from investigation findings to implementation without a planning phase to:
- Decompose work into steps
- Identify module dependencies
- Estimate scope/risk
- Get user approval on approach

### Patterns for Phase Insertion

**Pattern 1: Conditional Middleware (Recommended)**

Insert a new phase that activates only when conditions are met, similar to existing Phase 5.5 (Regression Testing).

```
Phase 0.5: Investigation (Tier 1/2/3)
  |
  +-- [Tier 1] --> Skip to Phase 0.9 (simple, no planning needed)
  |
  +-- [Tier 2/3] --> Phase 0.6: Planning (NEW)
                       |
                       v
                     Phase 0.9: Backup Verification
```

**Implementation approach:**
```markdown
## Phase 0.6: Implementation Planning (Conditional)

**GATE CONDITION:** Only runs for Tier 2 or Tier 3 investigations

**If Tier 1:** Skip to Phase 0.9 (simple fixes don't need plans)

**If Tier 2/3:**

1. **Decompose work:**
   - Break investigation findings into discrete tasks
   - Identify file touchpoints
   - List module dependencies

2. **Risk assessment:**
   - Breaking change potential
   - Regression risk areas
   - Rollback complexity

3. **Present plan for approval:**
   ```
   ## Implementation Plan

   ### Tasks
   1. [Task 1] - [file(s)]
   2. [Task 2] - [file(s)]

   ### Dependencies
   - Requires: [module/component]
   - Affects: [downstream components]

   ### Risk Assessment
   - Breaking changes: [YES/NO]
   - Regression areas: [list]
   - Estimated complexity: [LOW/MEDIUM/HIGH]

   Proceed with this plan? (y/n): _
   ```

4. **On approval:** Continue to Phase 0.9
5. **On rejection:** Return to Phase 0.5 with refinements
```

**Pattern 2: Hierarchical State Machine**

For more complex workflows, use nested state machines where complex states expand into sub-workflows.

```
Investigation (Tier 3)
  |
  +-- Sub-workflow:
        Research --> Planning --> Approval --> Return to parent
```

**Not recommended for v1.1** because:
- Increases complexity significantly
- Current workflow is linear and debuggable
- Conditional phase insertion is sufficient

**Pattern 3: Interceptor/Hook Pattern**

Allow phases to register hooks that execute before/after other phases.

```javascript
workflow.registerHook('after:investigation', (context) => {
  if (context.tier >= 2) {
    return executePhase('planning', context);
  }
});
```

**Not recommended** because:
- Requires workflow engine refactoring
- Implicit control flow is harder to debug
- Overkill for adding one phase

### Recommended Implementation

**1. Modify SKILL.md:**
- Add Phase 0.6 definition after Phase 0.5
- Update workflow diagram
- Update progress checklist

**2. Modify investigation-tiers.md:**
- Add planning handoff at end of Tier 2/3 protocols

**3. Create new reference file:**
- `.claude/skills/plugin-improve/references/planning-protocol.md`
- Contains plan template, risk matrix, approval flow

**4. Update JSON schemas (optional):**
- Add `planning_required: boolean` to output contract
- Add `plan_approved: boolean` to track state

### Why Conditional Middleware

| Criterion | Conditional Phase | Hierarchical FSM | Hook Pattern |
|-----------|-------------------|------------------|--------------|
| Implementation effort | LOW | HIGH | MEDIUM |
| Debugging ease | HIGH | MEDIUM | LOW |
| Consistency with existing | HIGH | LOW | MEDIUM |
| Future extensibility | MEDIUM | HIGH | HIGH |

**Decision:** Use conditional middleware pattern because:
1. Matches existing Phase 5.5 pattern (regression testing)
2. Linear flow preserved - easy to trace
3. Minimal changes to existing workflow
4. Can evolve to hierarchical later if needed

---

## Not Recommended

### git filter-branch

**Why not:**
- Deprecated by Git project itself
- 10-100x slower than git-filter-repo
- Memory issues on large repos
- Confusing semantics

### BFG Repo-Cleaner

**Why not for this project:**
- Requires Java installation
- Filename-only filtering (can't target `build/plugins/*` specifically)
- Skips HEAD by default (confusing protection feature)
- Less actively maintained

### git gc alone

**Why not:**
- Only cleans unreferenced objects
- Cannot remove objects still in history
- Must use filter-repo first, then gc

### Rewriting workflow as state machine library

**Why not:**
- Current linear workflow is working
- Adding formal FSM adds cognitive overhead
- Conditional phases achieve the goal with minimal change
- Can evolve later if workflow becomes more complex

### Adding planning to ALL improvements

**Why not:**
- Tier 1 fixes are simple (typos, cosmetic)
- Planning overhead would slow down trivial changes
- Conditional activation preserves fast path for simple fixes

---

## v1.1 Sources

### Git History Cleanup
- [git-filter-repo GitHub](https://github.com/newren/git-filter-repo) - PRIMARY
- [BFG vs git-filter-repo comparison](https://github.com/newren/git-filter-repo/blob/main/Documentation/converting-from-bfg-repo-cleaner.md)
- [Remove large binaries - Azure DevOps](https://learn.microsoft.com/en-us/azure/devops/repos/git/remove-binaries?view=azure-devops)
- [Git Tower - git-filter-repo guide](https://www.git-tower.com/learn/git/faq/git-filter-repo)

### Workflow Patterns
- [Google Multi-Agent Design Patterns](https://www.infoq.com/news/2026/01/multi-agent-design-patterns/)
- [Microsoft Agent Framework](https://learn.microsoft.com/en-us/agent-framework/overview/agent-framework-overview)
- [Extensible State Machine Pattern (Springer)](https://link.springer.com/chapter/10.1007/978-3-540-70592-5_24)
- [State Machine Design Pattern (LinkedIn)](https://www.linkedin.com/pulse/state-machine-design-pattern-concepts-examples-python-sajad-rahimi)

---

*v1.1 Research conducted: 2026-02-01*
*Confidence: HIGH for git cleanup (verified tool versions), HIGH for workflow (matches existing patterns)*
