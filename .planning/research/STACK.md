# Technology Stack: Multi-Agent AI Workflows for Plugin Development

**Project:** Plugin Freedom System Overhaul
**Researched:** 2026-01-29
**Dimension:** Stack/Framework for AI-assisted plugin development systems

## Executive Summary

GSD (Get Shit Done) is a context engineering framework that orchestrates Claude Code for spec-driven development. The core insight: **context rot is the enemy**. As conversations lengthen, LLMs "forget" previous decisions and generate inconsistent code. GSD solves this through phase-based isolation, atomic task execution in fresh 200k-token subagent contexts, and file-based state persistence.

For multi-agent systems in 2026, the field is undergoing its "microservices revolution." Single all-purpose agents are being replaced by orchestrated teams of specialized agents. The key is **simplicity with constraints**--successful implementations use surprisingly few tools (10-20), delegate complexity to code execution, and treat context as a finite resource to be carefully managed.

## GSD Framework Deep Dive

### What GSD Actually Is

**Confidence: HIGH** (Verified via GitHub repositories, multiple independent sources)

GSD is a meta-prompting and context engineering system for Claude Code that provides:

| Component | Purpose | How It Works |
|-----------|---------|--------------|
| Phase workflow | Structure work | discuss > research > plan > execute > verify |
| Subagent isolation | Prevent context rot | Each task runs in fresh 200k token window |
| File-based state | Cross-session persistence | PROJECT.md, ROADMAP.md, STATE.md track everything |
| Atomic commits | Traceability | Every task = one revertable commit |
| XML task format | Clear instructions | Structured `<task>` elements with verification steps |

### GSD Phase Model

```
/gsd:discuss-phase [N]  -> Capture implementation preferences
/gsd:plan-phase [N]     -> Research + create atomic tasks (max 3 per plan)
/gsd:execute-phase [N]  -> Parallel execution, fresh contexts
/gsd:verify-work [N]    -> Manual UAT + automated diagnostics
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
| **Sequential Pipeline** | Linear, deterministic workflows | Stage progression (foundation > DSP > GUI > polish) |
| **Coordinator/Dispatcher** | Route to specialists by intent | Phase commands dispatching to stage-specific agents |
| **Parallel Fan-out/Gather** | Independent investigations | Research agents investigating stack, features, architecture in parallel |
| **Generator/Critic** | Output reliability critical | Execute then verify pattern (DSP agent + validation agent) |
| **Hierarchical Decomposition** | Complex goals needing breakdown | Main orchestrator > stage agents > task subagents |

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

This injects full skill content at startup--agent doesn't need to discover and load during execution. Better for consistent behavior.

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

---

# v1.2 Stack Research: Agent Intelligence & Resource Orchestration

**Milestone:** v1.2 - Agent Intelligence & Resource Orchestration
**Domain:** Resource discovery, context injection, usage accountability, hook-based orchestration
**Researched:** 2026-02-04
**Overall confidence:** HIGH

---

## Executive Summary

The Plugin Freedom System already has the right infrastructure bones: 6 hooks, 11 agents, 24 skills, 23 research documents, and JSON Schema contracts. This milestone adds intelligence on top of that infrastructure -- making agents automatically discover relevant resources, injecting those resources into agent context, tracking what was used, and validating usage through hooks.

The key technical insight: **Claude Code's hook system already supports every integration point needed.** `SubagentStart` can inject `additionalContext` into spawned agents. `UserPromptSubmit` can inject context into the main conversation. `SubagentStop` can validate that agents used appropriate resources. No external services, vector databases, or runtime dependencies are needed. The entire system can be built with bash/Python scripts, JSON index files, and hook configuration.

The recommended approach is a **keyword-based resource index** (a static JSON manifest mapping topics to file paths) combined with **hook-driven context injection** (SubagentStart reads the manifest, matches against the task description, and injects relevant file paths as additionalContext). This is pragmatic, debuggable, and fits the file-based communication pattern the system already uses.

---

## v1.2 Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Python 3 | 3.9+ (already required) | Index builder, resource matcher, usage tracker | Already a dependency for validators; all hook scripts can leverage it. No new runtime required. |
| JSON | N/A | Resource index format, usage logs, manifest files | Native to the existing system (schemas, contracts, registry all use JSON). Parseable by both bash (`jq`) and Python natively. |
| Bash hooks | N/A | Hook entry points for SubagentStart, SubagentStop | Existing hook infrastructure is bash-based. Keeps consistency. Delegates to Python for complex logic. |
| jq | 1.6+ (already required) | JSON parsing in hook scripts | Already validated by SessionStart hook. Used for extracting agent_type, prompt text from hook stdin. |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `bm25s` | 0.2+ | Lightweight BM25 keyword scoring for resource matching | Optional upgrade path if keyword matching needs sophistication beyond exact match. Pure Python, scipy-based sparse matrices, no database. Install only if simple keyword matching proves insufficient. |
| `re` (stdlib) | N/A | Regex-based keyword extraction from agent prompts | Always -- extracting topic keywords from Task tool prompt text. |
| `hashlib` (stdlib) | N/A | SHA256 checksums for resource index staleness detection | Detect when research documents change and index needs rebuilding. |
| `pathlib` (stdlib) | N/A | Cross-platform path handling for resource discovery | Always -- scanning research/, skills/, templates/ directories. |
| `json` (stdlib) | N/A | Reading/writing resource index and usage logs | Always -- core data format. |

### New Scripts (to create)

| Script | Purpose | Called By |
|--------|---------|-----------|
| `.claude/scripts/build-resource-index.py` | Rebuild the resource index from current files | SessionStart hook (if stale) or manually |
| `.claude/scripts/match-resources.py` | Given agent type and context, return ranked resource matches | SubagentStart hook |
| `.claude/scripts/track-usage.py` | Parse subagent transcript, log resource usage | SubagentStop hook |
| `.claude/scripts/usage-report.py` | Generate accountability report from usage logs | Manual invocation or SessionEnd |

---

## Architecture: How the Pieces Fit

### Resource Index (Static JSON Manifest)

The cornerstone is a **resource index file** at `.claude/resource-index.json`. This is a pre-built JSON file that maps keywords/topics to resource file paths. It is NOT a database -- it is a flat file regenerated whenever documents change.

**Structure:**

```json
{
  "version": 1,
  "generated": "2026-02-04T12:00:00Z",
  "checksum": "sha256:abc123...",
  "resources": [
    {
      "path": "research/fft-processing-best-practices.md",
      "type": "research",
      "keywords": ["fft", "fourier", "spectral", "frequency", "stft", "windowing", "overlap-add"],
      "title": "FFT Processing Best Practices in JUCE",
      "summary": "Comprehensive guide to implementing high-quality FFT-based audio processing",
      "size_lines": 753,
      "domain": "dsp"
    },
    {
      "path": "research/reverb-comprehensive-research.md",
      "type": "research",
      "keywords": ["reverb", "reverberation", "room", "convolution", "algorithmic", "diffusion"],
      "title": "Reverb Comprehensive Research",
      "summary": "Comprehensive reverb implementation techniques and algorithms",
      "size_lines": 1580,
      "domain": "dsp"
    }
  ],
  "skills": [
    {
      "path": ".claude/skills/plugin-planning/",
      "name": "plugin-planning",
      "keywords": ["plan", "planning", "architecture", "roadmap", "stage-0", "research"],
      "domain": "workflow"
    }
  ],
  "agent_affinities": {
    "dsp-agent": {
      "domain": "dsp",
      "always_include": ["troubleshooting/patterns/stage-2-patterns.md"],
      "keyword_boost": ["processblock", "filter", "gain", "delay", "synthesis", "oscillator"]
    },
    "research-planning-agent": {
      "domain": "dsp",
      "always_include": ["troubleshooting/patterns/juce8-critical-patterns.md"],
      "keyword_boost": ["architecture", "planning", "research", "complexity"]
    },
    "gui-agent": {
      "domain": "ui",
      "always_include": ["troubleshooting/patterns/stage-3-patterns.md"],
      "keyword_boost": ["webview", "html", "javascript", "relay", "binding", "ui"]
    },
    "foundation-shell-agent": {
      "domain": "build",
      "always_include": ["troubleshooting/patterns/stage-1-patterns.md"],
      "keyword_boost": ["cmake", "build", "apvts", "parameter", "foundation"]
    }
  }
}
```

**Why this approach (not vector embeddings):**

1. **Debuggability.** You can open `resource-index.json` and see exactly what will match. No black-box similarity scores.
2. **Zero dependencies.** No vector database, no embedding model, no GPU. Just JSON + Python.
3. **Fits Claude Code's model.** Claude Code agents communicate through files. A JSON index is a file. Hooks read files. This is consistent.
4. **The corpus is small.** 23 research docs, 24 skills, 11 agents. BM25 or vector search adds complexity for a corpus that fits in a single grep. Keyword matching with a curated index is sufficient and more reliable for this scale.
5. **Keyword extraction is reliable for this domain.** DSP topics have distinctive vocabulary: "FFT", "reverb", "delay", "microtonality", "physical modeling". These are not ambiguous terms that need semantic understanding.
6. **Agent affinities are predictable.** dsp-agent always needs DSP resources. gui-agent always needs UI resources. This domain knowledge can be encoded directly rather than inferred.

### Hook Integration Points

**1. SubagentStart -- Resource Injection (PRIMARY)**

This is the main integration point. When any subagent is spawned, the hook:
- Reads the agent type (from `agent_type` field in stdin JSON)
- Reads the main session transcript to extract the Task prompt
- Matches keywords from the prompt against the resource index
- Combines keyword matches with agent-type affinity (always_include + boosted keywords)
- Returns `additionalContext` with relevant resource paths and summaries

```json
{
  "hookSpecificOutput": {
    "hookEventName": "SubagentStart",
    "additionalContext": "RELEVANT RESOURCES (auto-discovered for dsp-agent):\n\nALWAYS READ:\n- troubleshooting/patterns/stage-2-patterns.md -- Stage 2 specific JUCE patterns\n\nTASK-RELEVANT (matched: fft, spectral, frequency):\n- research/fft-processing-best-practices.md -- FFT implementation guide (753 lines)\n- research/fft-artifact-prevention.md -- Artifact prevention techniques (788 lines)\n- research/custom-fft-implementations.md -- Custom FFT approaches (875 lines)\n\nYou SHOULD read the ALWAYS READ files. Read TASK-RELEVANT files if they match your work. Report which resources you used in your JSON output under resources_used."
  }
}
```

**Why SubagentStart, not UserPromptSubmit:**
- SubagentStart fires when agents are spawned via Task tool, which is when resource context matters most
- UserPromptSubmit fires on EVERY prompt (including casual questions). Over-injection wastes context tokens
- SubagentStart has access to `agent_type`, enabling per-agent resource matching
- The existing UserPromptSubmit hook already handles `/continue` context injection. Adding resource injection there creates conflicts
- SubagentStart can inject `additionalContext` into the subagent's context (verified in official Claude Code docs)

**2. SubagentStop -- Usage Validation (SECONDARY)**

When a subagent finishes, the hook:
- Reads the agent's transcript (from `agent_transcript_path` in stdin JSON)
- Parses JSONL transcript for Read tool calls to identify files actually read
- Compares against resources recommended in SubagentStart
- Logs usage to `.claude/usage-logs/{session_id}.json`
- Warns (stderr) if always_include resources were NOT read

**Integration with existing SubagentStop.sh:**
The existing SubagentStop.sh runs contract validators for foundation/dsp/gui agents. Resource tracking should be added as an ADDITIONAL step, not a replacement. The dispatch pattern:

```bash
# In SubagentStop.sh, AFTER existing validation:

# Resource usage tracking (non-blocking)
python3 "$CLAUDE_PROJECT_DIR/.claude/scripts/track-usage.py" \
  --agent-type "$AGENT_TYPE" \
  --agent-id "$AGENT_ID" \
  --transcript "$AGENT_TRANSCRIPT" \
  --session "$SESSION_ID" 2>/dev/null || true
# Non-blocking: exit 0 even if tracking fails (don't break workflow)
```

**3. SessionStart -- Index Freshness Check**

On session start, check if the resource index is stale:
- Compare file modification times in research/ and skills/ against index generation timestamp
- If any source file is newer than the index, rebuild automatically
- Report status to stdout (becomes session context)

**Integration with existing SessionStart.sh:**
Add after the existing dependency validation (python3, jq, cmake, etc.):

```bash
# Resource index freshness check
if [ -f ".claude/resource-index.json" ]; then
  STALE=$(python3 .claude/scripts/build-resource-index.py --check-only 2>/dev/null)
  if [ "$STALE" = "stale" ]; then
    echo "Rebuilding resource index..."
    python3 .claude/scripts/build-resource-index.py
    echo "Resource index rebuilt"
  else
    echo "Resource index up to date"
  fi
else
  echo "Building initial resource index..."
  python3 .claude/scripts/build-resource-index.py
  echo "Resource index created"
fi
```

**4. PreCompact -- Resource Context Preservation**

Before compaction, output which resources have been referenced in the current conversation. This prevents resource awareness from being lost during context compaction.

**Integration with existing PreCompact.sh:**
Add after the existing contract preservation:

```bash
# Resource context preservation
if [ -f ".claude/resource-index.json" ]; then
  echo "=== Resource Context ==="
  echo "Available resources: $(jq '.resources | length' .claude/resource-index.json) research docs"
  echo "Resource index location: .claude/resource-index.json"
  echo ""
fi
```

---

## Keyword Extraction Strategy

The matching algorithm does NOT need to be sophisticated. The domain vocabulary is specific enough that simple keyword overlap works.

### Extraction from Agent Prompts

When SubagentStart fires, extract keywords from the task prompt using:

1. **Direct keyword matching** -- compare prompt words against index keywords (case-insensitive)
2. **Agent type affinity** -- if agent_type is "dsp-agent", include `always_include` resources and boost DSP-domain keywords
3. **Plugin name matching** -- if prompt contains a plugin name (e.g., "O-Spectral"), also check for that plugin's `.planning/research/ARCHITECTURE.md`

```python
import re
import json
from pathlib import Path

AGENT_DOMAIN_MAP = {
    "dsp-agent": "dsp",
    "gui-agent": "ui",
    "foundation-shell-agent": "build",
    "research-planning-agent": "dsp",
    "music-theory-agent": "dsp",
    "aesthetics-agent": "ui",
    "ui-design-agent": "ui",
    "ui-finalization-agent": "ui",
    "validation-agent": "workflow",
    "troubleshoot-agent": "dsp",
    "polish-agent": "workflow",
}

def match_resources(prompt_text: str, agent_type: str, index: dict) -> dict:
    """Match resources against prompt keywords and agent type."""
    prompt_words = set(re.findall(r'\b\w+\b', prompt_text.lower()))

    # Get agent affinity config
    affinity = index.get("agent_affinities", {}).get(agent_type, {})
    always_include = affinity.get("always_include", [])
    keyword_boost = set(k.lower() for k in affinity.get("keyword_boost", []))

    scored = []
    for resource in index.get("resources", []):
        keywords = set(k.lower() for k in resource["keywords"])

        # Base score: keyword overlap
        overlap = len(prompt_words & keywords)

        # Boost for agent-affinity keywords
        boost_overlap = len(prompt_words & keyword_boost & keywords)
        overlap += boost_overlap

        # Domain match bonus
        agent_domain = AGENT_DOMAIN_MAP.get(agent_type)
        if agent_domain and resource.get("domain") == agent_domain:
            overlap += 1

        if overlap > 0:
            scored.append((overlap, resource))

    scored.sort(key=lambda x: x[0], reverse=True)
    task_relevant = [r for _, r in scored[:5]]

    return {
        "always_read": always_include,
        "task_relevant": task_relevant,
    }
```

### Index Building from Documents

The index builder scans each document and extracts keywords using:

1. **Title extraction** -- parse first H1 heading from markdown
2. **Header keywords** -- extract H2/H3 headings as secondary keywords
3. **DSP vocabulary matching** -- check content against a curated domain vocabulary list
4. **Manual overrides** -- a `resource-keywords-override.json` file can supplement auto-extracted keywords

The domain vocabulary list covers standard audio/DSP terms:

```python
DSP_VOCABULARY = {
    "fft", "fourier", "spectral", "frequency", "stft", "windowing",
    "reverb", "reverberation", "convolution", "diffusion",
    "delay", "echo", "feedback", "tap",
    "filter", "lowpass", "highpass", "bandpass", "notch", "eq",
    "compressor", "dynamics", "threshold", "ratio", "attack", "release",
    "oscillator", "synthesis", "wavetable", "additive", "subtractive",
    "distortion", "saturation", "waveshaper", "overdrive",
    "modulation", "lfo", "envelope", "tremolo", "vibrato", "chorus",
    "pitch", "detune", "harmonics", "overtone",
    "microtonality", "tuning", "temperament", "scala",
    "granular", "grain", "stochastic",
    "physical", "modeling", "modal", "waveguide",
    "circuit", "analog", "emulation",
    # ... extended as needed
}
```

---

## Usage Tracking Format

Usage data is logged to `.claude/usage-logs/` as JSON files per session:

```json
{
  "session_id": "abc123",
  "date": "2026-02-04",
  "agents": [
    {
      "agent_type": "dsp-agent",
      "agent_id": "def456",
      "timestamp": "2026-02-04T12:05:00Z",
      "resources_recommended": {
        "always_read": ["troubleshooting/patterns/stage-2-patterns.md"],
        "task_relevant": [
          "research/fft-processing-best-practices.md",
          "research/fft-artifact-prevention.md",
          "research/custom-fft-implementations.md"
        ]
      },
      "resources_read": [
        "research/fft-processing-best-practices.md",
        "research/fft-artifact-prevention.md",
        "troubleshooting/patterns/stage-2-patterns.md"
      ],
      "resources_ignored": [
        "research/custom-fft-implementations.md"
      ],
      "always_read_compliance": true,
      "task_relevant_coverage_pct": 66.7
    }
  ]
}
```

### Transcript Parsing for Usage Detection

The SubagentStop hook receives `agent_transcript_path`, which is a JSONL file. Each line is a JSON object representing a conversation turn. Read tool calls appear as tool_use events with `tool_name: "Read"` and `tool_input.file_path`.

```python
import json

def extract_files_read(transcript_path: str) -> list:
    """Parse subagent transcript JSONL to find all files Read by the agent."""
    files_read = []
    with open(transcript_path, 'r') as f:
        for line in f:
            try:
                entry = json.loads(line)
                # Look for tool_use entries with Read tool
                if entry.get("type") == "tool_use" and entry.get("tool_name") == "Read":
                    file_path = entry.get("tool_input", {}).get("file_path", "")
                    if file_path:
                        files_read.append(file_path)
            except json.JSONDecodeError:
                continue
    return files_read
```

**Confidence note:** The exact transcript JSONL schema for subagents needs validation during implementation. The tool_use entry structure is inferred from Claude Code's general transcript format but may differ for subagent transcripts. This should be validated against an actual subagent transcript file early in implementation.

---

## Schema Extensions

### Subagent Report Schema

The existing schema at `.claude/schemas/subagent-report.json` uses `additionalProperties: false`. To add resource tracking, the schema must be updated to include:

```json
{
  "resources_used": {
    "type": "array",
    "items": {
      "type": "object",
      "properties": {
        "path": { "type": "string", "description": "File path of the resource" },
        "relevance": {
          "type": "string",
          "enum": ["high", "medium", "low"],
          "description": "How relevant the resource was to the task"
        },
        "used_for": { "type": "string", "description": "What the resource was used for" }
      },
      "required": ["path"]
    },
    "description": "Resources read by the agent during execution (auto-tracked + self-reported)"
  }
}
```

### Agent Prompt Updates

Each agent markdown file should include a section instructing agents to report resource usage:

```markdown
## Resource Reporting

When you receive auto-discovered resource recommendations, include a `resources_used`
field in your JSON report listing which resources you actually read and how they
informed your work. This enables usage tracking and continuous improvement of
resource recommendations.

Example:
"resources_used": [
  {"path": "research/fft-processing-best-practices.md", "relevance": "high", "used_for": "STFT overlap-add pattern"},
  {"path": "research/fft-artifact-prevention.md", "relevance": "medium", "used_for": "Windowing function selection"}
]
```

---

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| Keyword matching (JSON index) | BM25 scoring (`bm25s` library) | If the resource collection grows beyond ~100 documents and keyword overlap becomes unreliable. BM25 adds TF-IDF-style weighting and document length normalization. Currently overkill for 23 docs. |
| Static JSON manifest | SQLite FTS5 | If you need full-text search across document CONTENT, not just keywords. FTS5 is built into Python's sqlite3 module. Only needed if matching on document headers/keywords proves insufficient. |
| SubagentStart hook injection | Agent frontmatter `skills` field | If resource needs are completely predictable per agent type. The skills field preloads content at startup. But it cannot adapt to task-specific context (e.g., "this specific DSP task needs FFT docs, not reverb docs"). |
| File-based usage logs (JSON) | SQLite usage database | If you need to query usage patterns across many sessions (e.g., "which resources are most used?"). JSON files are simpler but harder to aggregate. Cross that bridge when the data justifies it. |
| Python keyword matcher | LLM-based routing (`type: "prompt"` hook) | If keyword matching has too many false negatives. A prompt hook could ask a small model "which resources are relevant?" But adds latency (LLM call per agent spawn) and cost. Reserve for complex routing only. |

---

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Vector databases (Chroma, Pinecone, Weaviate) | Massive overkill for 23 documents. Adds infrastructure, dependencies, and complexity that provide no benefit at this scale. The domain vocabulary is distinctive enough that keyword matching is more reliable than semantic similarity. | Static JSON keyword index |
| Embedding models (OpenAI, sentence-transformers) | Requires API calls or local model loading. Adds latency, cost, and a dependency. Keyword matching is faster, free, and more predictable for a curated corpus. | Python keyword extraction from headers and domain vocabulary |
| LangChain / LlamaIndex | Heavy frameworks designed for general-purpose RAG pipelines. The Plugin Freedom System has its own orchestration (hooks + agents). Adding a second orchestration layer creates conflicts and confusion. | Direct Python scripts called by hooks |
| UserPromptSubmit for resource injection | Fires on EVERY prompt, not just agent spawning. Would inject research doc references when the user asks "how do I build?" Context pollution. The existing UserPromptSubmit already handles `/continue`. | SubagentStart hook (fires only when agents spawn) |
| Automatic full-document injection | Injecting full document content into agent context wastes tokens. Research docs range from 425 to 2,336 lines. | Inject file PATHS and one-line summaries. Let the agent Read the files it needs. |
| Complex NLP for keyword extraction | NLTK, spaCy for topic extraction from research docs. Over-engineering for a domain with clear vocabulary. "FFT", "reverb", "microtonality" do not need lemmatization. | Simple regex tokenization + curated DSP vocabulary list |
| Real-time index rebuilding per SubagentStart | Rebuilding the index on every agent spawn adds latency. The research corpus changes rarely. | Rebuild on SessionStart if stale, or manually via script |
| `type: "agent"` hooks for resource matching | Agent hooks spawn a full subagent (with multi-turn tool access) to make routing decisions. Extreme overkill for keyword matching. Adds seconds of latency. | `type: "command"` hooks running Python scripts (milliseconds) |

---

## Integration with Existing Hooks

The existing hook system has specific hooks that must NOT be disrupted:

| Hook | Current Function | Integration Strategy |
|------|-----------------|---------------------|
| `SessionStart.sh` | Environment validation (python3, jq, cmake, JUCE) | APPEND index freshness check at end (after existing validation). Non-blocking. |
| `UserPromptSubmit.sh` | `/continue` context injection | DO NOT MODIFY. Resource injection happens in SubagentStart instead. |
| `PostToolUse.sh` | Real-time safety validation, contract immutability | DO NOT MODIFY. No resource injection needed here. |
| `SubagentStop.sh` | Contract validation (checksums, cross-contract, foundation, DSP, GUI) | APPEND usage tracking after existing validators. Non-blocking (exit 0 even if tracking fails). |
| `Stop.sh` | Stage enforcement | DO NOT MODIFY. |
| `PreCompact.sh` | Contract preservation | APPEND resource context note after existing contract output. |

**New hook to ADD in hooks.json:**

```json
{
  "SubagentStart": [
    {
      "matcher": ".*",
      "hooks": [
        {
          "type": "command",
          "command": "${CLAUDE_PROJECT_DIR}/.claude/hooks/SubagentStart-resources.sh",
          "timeout": 3,
          "description": "Auto-discover and inject relevant resources for subagents"
        }
      ]
    }
  ]
}
```

**Important:** The SubagentStart hook timeout should be short (3 seconds). The Python keyword matching script should complete in under 500ms for the current corpus size. If it exceeds timeout, the hook is silently skipped and the agent proceeds without resource injection (graceful degradation).

---

## Version Compatibility

| Component | Compatible With | Notes |
|-----------|-----------------|-------|
| Python 3.9+ | All existing validators | Already required by validate-checksums.py, validate-cross-contract.py |
| jq 1.6+ | All existing hooks | Already required by SubagentStop.sh, PostToolUse.sh |
| hooks.json SubagentStart | Claude Code v2.1+ | SubagentStart event and additionalContext verified in official docs |
| agent_transcript_path | Claude Code v2.1.16+ | Required for usage tracking (reading which files the agent Read) |
| additionalProperties: false schema | Existing subagent-report.json | Must be updated to include resources_used before agents can report |

---

## Installation

No new packages to install. Everything uses existing dependencies:

```bash
# Verify existing dependencies (already checked by SessionStart.sh)
python3 --version  # 3.9+
jq --version       # 1.6+

# Build the initial resource index
python3 .claude/scripts/build-resource-index.py

# Verify the index
python3 -c "import json; d=json.load(open('.claude/resource-index.json')); print(f'{len(d[\"resources\"])} resources indexed')"
# Expected: ~23 (one per research doc)
```

**Optional future upgrade (only if keyword matching proves insufficient):**
```bash
pip install "bm25s[full]"  # Lightweight BM25 scoring, no database
```

---

## v1.2 Confidence Assessment

| Area | Confidence | Reason |
|------|------------|--------|
| Hook integration points (SubagentStart, SubagentStop) | HIGH | Verified against official Claude Code docs. additionalContext injection confirmed. agent_transcript_path confirmed. |
| Keyword matching approach | HIGH | Domain vocabulary is distinctive (DSP terms). Corpus is small (23 docs). No evidence that semantic search would outperform curated keywords at this scale. |
| Usage tracking via transcript parsing | MEDIUM | Transcript JSONL format exists but exact schema for Read tool calls in subagent transcripts needs validation during implementation. |
| Schema extension for resources_used | HIGH | Current schema uses additionalProperties: false, so change is required. The pattern is established (other optional fields already exist). |
| Index freshness detection | HIGH | SHA256 and mtime checking is standard Python. Already used for contract checksums in existing validators. |
| Python script performance (<500ms) | HIGH | 23 documents with keyword matching is trivially fast. json.load + set intersection on small data. |

---

## v1.2 Sources

### Primary (HIGH Confidence)
- [Claude Code Hooks Reference](https://code.claude.com/docs/en/hooks) -- Official documentation for all hook events, input schemas, output formats. Verified SubagentStart additionalContext, SubagentStop agent_transcript_path.
- [Claude Code Subagents Documentation](https://code.claude.com/docs/en/sub-agents) -- Official docs on subagent creation, frontmatter fields, tool restrictions, skills preloading, lifecycle.
- Existing system analysis: `.claude/hooks/hooks.json`, `.claude/hooks/*.sh`, `.claude/agents/*.md`, `.claude/schemas/` -- Direct analysis of the current codebase.

### Secondary (MEDIUM Confidence)
- [Claude Code Hooks Mastery](https://github.com/disler/claude-code-hooks-mastery) -- Community patterns for hook-based agent orchestration
- [bm25s Library](https://github.com/xhluca/bm25s) -- Lightweight Python BM25 implementation (verified library capabilities)
- [Claude Code Context Optimization](https://gist.github.com/johnlindquist/849b813e76039a908d962b2f0923dc9a) -- 54% context reduction case study showing trigger-based routing
- [DEV Community: Claude Code Context Injection](https://dev.to/sasha_podles/claude-code-using-hooks-for-guaranteed-context-injection-2jg) -- Practical patterns for guaranteed context injection
- [Botpress: AI Agent Routing](https://botpress.com/blog/ai-agent-routing) -- Keyword-based routing patterns for multi-agent systems
- [BM25 for Developers](https://medium.com/@kimdoil1211/bm25-for-developers-a-guide-to-smarter-keyword-search-e6d83e8c8c8c) -- BM25 algorithm explanation and comparison with TF-IDF
- [DigitalOcean: RAG Without Embeddings](https://www.digitalocean.com/community/tutorials/beyond-vector-databases-rag-without-embeddings) -- Alternative retrieval approaches without vector databases

---

*v1.2 Research conducted: 2026-02-04*
*Confidence: HIGH for hook integration, HIGH for keyword matching, MEDIUM for transcript parsing*
