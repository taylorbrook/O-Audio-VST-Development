# Architecture Patterns: Plugin Freedom System v1.3 Modernization

**Domain:** AI agent orchestration for JUCE audio plugin development
**Researched:** 2026-02-08
**Confidence:** HIGH (based on official Claude Code documentation, GSD 1.18.0 source, and thorough codebase analysis)

## Recommended Architecture

The v1.3 modernization is NOT a rewrite. It is a targeted integration of three capability upgrades -- Opus 4.6 subagent improvements, Agent Teams (experimental), and GSD 1.18.0 tooling -- into the existing Plugin Freedom System architecture. The core workflow (Stage 0-4, discuss-research-plan-execute-verify) is preserved.

### Architecture Overview

```
                    USER
                     |
              /start, /plan, /implement, /improve
                     |
            .claude/commands/ (Skills)
                     |
         +-----------+-----------+
         |                       |
    GSD Workflows          PFS Orchestrators
    (milestone/phase)      (plugin-workflow)
         |                       |
    gsd-tools CLI          Stage Routing
    (deterministic)        (0 -> 1 -> 2 -> 3 -> 4)
         |                       |
    +----+----+          +-------+-------+
    |         |          |       |       |
  planner  executor   research  dsp    gui    polish
  verifier debugger   -planning -agent -agent  -agent
                        -agent
                         |
                    Specialist Agents
                    (music-theory, aesthetics,
                     troubleshoot, validation)
```

**What changes in v1.3:**
1. GSD mechanical operations migrate to `gsd-tools` CLI (deterministic, not AI)
2. Agent model assignments upgrade to leverage Opus 4.6 reasoning
3. Persistent memory enables cross-session learning for key agents
4. Agent Teams available as opt-in for parallel plugin workflows
5. Duplicate code between PFS and GSD eliminated

### Component Boundaries

| Component | Responsibility | Communicates With | Change in v1.3 |
|-----------|---------------|-------------------|-----------------|
| `.claude/commands/` | User-facing CLI entry points | GSD workflows, PFS skills | PRESERVE - add new commands only |
| `.claude/agents/` | Agent definitions (11 custom) | Invoked by orchestrators via Task tool | MODIFY - add memory, update models |
| `~/.claude/get-shit-done/` | GSD framework (global) | PFS skills, gsd-tools CLI | UPDATE to 1.18.0 (already done) |
| `.planning/` | Project state, research, roadmaps | All agents read/write | PRESERVE - state format unchanged |
| `plugins/*/` | Plugin source code + per-plugin state | Stage agents read/write | PRESERVE - no changes |
| `.claude/hooks/` | Lifecycle hooks (6 existing) | SubagentStart/Stop, SessionStart | MODIFY - leverage gsd-tools |
| `.claude/scripts/` | Utility scripts (template-lookup, etc.) | Orchestrator skills | PRESERVE or REPLACE with gsd-tools |
| `.claude/schemas/` | JSON Schema contracts | Validation hooks, agents | PRESERVE - extend optionally |

### Data Flow

```
Session Start
     |
  SessionStart hook -> validate environment, check manifest freshness
     |
  User command (/implement PluginName)
     |
  Skill (implement.md) -> resolve plugin, check preconditions
     |
  Orchestrator determines stage -> reads STATUS.md
     |
  For each stage:
     |
     +-- SubagentStart hook -> inject research context
     |
     +-- Stage Agent spawned (foundation-shell, dsp, gui, polish)
     |     |
     |     +-- Agent reads contracts (BRIEF.md, ARCHITECTURE.md, etc.)
     |     +-- Agent reads persistent memory (NEW in v1.3)
     |     +-- Agent executes task
     |     +-- Agent writes files + JSON report
     |     +-- Agent updates persistent memory (NEW in v1.3)
     |     |
     +-- SubagentStop hook -> validate report, check accountability
     |
     +-- Orchestrator processes report
     |     +-- gsd-tools CLI updates state (NEW in v1.3)
     |     +-- Build automation (if applicable)
     |     +-- Validation agent
     |
     +-- Handoff to next stage (or checkpoint menu)
```

## Integration Analysis: Three Capability Sources

### Source 1: Opus 4.6 Capabilities

**What Opus 4.6 brings to the existing system:**

| Capability | Impact on PFS | Integration Point |
|-----------|--------------|-------------------|
| Improved reasoning | Better first-pass DSP/GUI code | Model assignment in agent YAML |
| Better tool orchestration | More reliable multi-step agent execution | Already benefits from model upgrade |
| Persistent memory | Cross-session learning for agents | New `memory` field in agent YAML |
| Agent Teams (experimental) | Optional parallel plugin workflows | New workflow mode, not default |
| 1M token context | Larger contracts, more research injection | Increase context injection budget |
| Effort tuning | Route simple tasks to faster models | Refine model profile strategy |

**Confidence: HIGH** -- sourced from official Claude Code documentation at code.claude.com/docs/en/sub-agents

### Source 2: GSD 1.18.0 Features

**What GSD 1.18.0 adds that PFS should adopt:**

| Feature | GSD Version | PFS Current State | Integration |
|---------|------------|-------------------|-------------|
| `gsd-tools` CLI for state management | 1.17.0 | Manual markdown parsing in agents | Replace manual state updates |
| Frontmatter CRUD (get/set/merge/validate) | 1.17.0 | Custom Python scripts | Replace with gsd-tools |
| Verification suite | 1.17.0 | Custom validation-agent | Augment (not replace) validation |
| Template filling | 1.17.0 | Custom template-lookup.py | Evaluate replacement |
| `--auto` flag for new-project | 1.18.0 | N/A (PFS has own ideation) | N/A |
| Mechanical operations CLI | 1.16.0 | Custom state file parsing | Adopt for phase/milestone ops |
| Local patch preservation | 1.17.0 | No equivalent | Adopt for safe GSD updates |
| Optimized context loading | 1.15.0 | Some optimization | Already benefiting |

**Confidence: HIGH** -- sourced from GSD CHANGELOG.md on GitHub and local VERSION file

### Source 3: Existing PFS Architecture (What Stays)

**Components that should NOT change:**

| Component | Why Preserve |
|-----------|-------------|
| Stage 0-4 pipeline | Proven workflow, 35+ plugins built successfully |
| Agent contracts (BRIEF.md, ARCHITECTURE.md, etc.) | Zero-drift guarantee depends on these |
| JSON Schema validation | Prevents report format drift |
| WebView UI architecture | Working cross-platform solution |
| Module system (OuariconModules.cmake) | Reliable code reuse with semver |
| Resource discovery + injection pipeline | Just shipped in v1.2, working well |
| Handoff protocol | Prevents context loss between stages |
| Build automation skill | Handles cmake/ninja/install/cache clearing |

**Confidence: HIGH** -- based on production usage across 35+ plugins

## Patterns to Follow

### Pattern 1: Persistent Memory for Key Agents

**What:** Add `memory: project` to agents that accumulate expertise across sessions.

**When:** Agents that learn from repeated invocations -- troubleshoot-agent learns failure patterns, dsp-agent learns JUCE API quirks, research-planning-agent learns plugin architecture patterns.

**Implementation:**

In agent YAML frontmatter:
```yaml
---
name: troubleshoot-agent
description: Deep research agent for build failures...
tools: Read, Grep, Glob, Bash, WebSearch, WebFetch
model: opus
memory: project
---
```

This creates `.claude/agent-memory/troubleshoot-agent/MEMORY.md` which persists across sessions. The agent reads it on startup and updates it with findings.

**Which agents get memory:**

| Agent | Memory Scope | Rationale |
|-------|-------------|-----------|
| troubleshoot-agent | project | Accumulate JUCE gotchas, build failure patterns |
| dsp-agent | project | Learn DSP implementation patterns, API quirks |
| gui-agent | project | Learn WebView binding patterns, CSS gotchas |
| research-planning-agent | project | Remember architecture patterns across plugins |
| validation-agent | project | Track common validation failures and false positives |
| foundation-shell-agent | none | Deterministic template execution, no learning needed |
| music-theory-agent | none | Static domain knowledge, no learning needed |
| aesthetics-agent | none | Specification only, not implemented |
| polish-agent | none | Infrequent use, limited learning value |
| ui-design-agent | none | Follows spec, minimal cross-session value |
| ui-finalization-agent | none | Template execution, no learning needed |

### Pattern 2: gsd-tools CLI for State Management

**What:** Replace manual markdown parsing in agent state management with `gsd-tools` CLI commands.

**When:** Any time an agent or orchestrator needs to update STATE.md, advance plans, record metrics, or validate state.

**Current pattern (manual):**
```bash
# Agent currently reads STATUS.md, parses YAML, writes updated YAML manually
# Error-prone, token-expensive, inconsistent formatting
```

**New pattern (gsd-tools):**
```bash
# Deterministic, consistent, token-efficient
node ~/.claude/get-shit-done/bin/gsd-tools.js advance-plan "${PHASE}" "${PLAN_ID}"
node ~/.claude/get-shit-done/bin/gsd-tools.js update-progress "${PERCENTAGE}"
node ~/.claude/get-shit-done/bin/gsd-tools.js record-metric "plugins_completed" "35"
```

**Integration points in PFS:**
- Each stage agent's "State Management" section (foundation-shell, dsp, gui, polish)
- plugin-workflow orchestrator state transitions
- validation-agent report handling
- STATUS.md / PLUGINS.md updates

### Pattern 3: Model Profile Refinement

**What:** Update model assignments to leverage Opus 4.6's improved reasoning for critical agents, while using Sonnet/Haiku for mechanical tasks.

**Current assignments:**

| Agent | Current Model | Recommended |
|-------|--------------|-------------|
| foundation-shell-agent | sonnet | sonnet (no change -- template execution) |
| dsp-agent | sonnet (default), opus (complexity >= 4) | **opus** (always -- DSP quality is critical) |
| gui-agent | sonnet | sonnet (no change -- follows spec) |
| research-planning-agent | sonnet | **opus** (architectural decisions matter) |
| validation-agent | opus | opus (no change -- reasoning critical) |
| troubleshoot-agent | opus | opus (no change -- deep investigation) |
| ui-design-agent | sonnet | sonnet (no change) |
| ui-finalization-agent | sonnet | sonnet (no change) |
| music-theory-agent | sonnet | **haiku** (static lookups, no reasoning needed) |
| polish-agent | (unspecified) | sonnet (standard tasks) |
| aesthetics-agent | sonnet | N/A (not implemented) |

**Rationale:** Opus 4.6 is significantly better at architectural decisions and complex reasoning. DSP implementation quality directly impacts plugin quality -- upgrading dsp-agent to always-opus eliminates the complexity threshold check and produces better first-pass code. Research-planning-agent makes architecture decisions that cascade through all subsequent stages.

### Pattern 4: Agent Teams for Parallel Plugin Workflows (Optional)

**What:** Use Agent Teams to build multiple plugins simultaneously or parallelize independent stages.

**When:** User explicitly requests parallel work, OR when building multiple independent plugins.

**NOT the default pattern.** Agent Teams are experimental and add significant token overhead. The sequential stage pipeline remains the primary workflow.

**Viable parallel patterns:**

| Pattern | How | When |
|---------|-----|------|
| Multi-plugin research | Team of 3 research-planning-agents, each investigating a different plugin | Batch plugin ideation |
| Independent stage work | Frontend teammate builds GUI while backend teammate does DSP | Only if no stage dependencies |
| Parallel validation | Multiple validators checking different aspects simultaneously | Already possible with subagents |

**NOT viable (avoid):**

| Anti-pattern | Why |
|-------------|-----|
| Same-plugin parallel stages | Stages have dependencies (DSP before GUI) |
| Same-file editing | File conflicts between teammates |
| Sequential tasks in parallel | Coordination overhead exceeds benefit |

### Pattern 5: Deduplication of PFS vs GSD Code

**What:** Identify and remove PFS custom code that GSD now handles natively.

**Candidates for removal/replacement:**

| PFS Component | GSD Equivalent | Action |
|--------------|----------------|--------|
| Manual STATE.md parsing in agents | `gsd-tools advance-plan`, `update-progress` | Replace agent state sections |
| Custom progress tracking | `gsd-tools progress` (multiple formats) | Replace /progress command |
| Manual phase completion | `gsd-tools phase complete` | Replace orchestrator code |
| Custom verification in skills | `gsd-tools validate consistency` | Augment validation-agent |
| Template filling scripts | `gsd-tools template fill` | Evaluate case-by-case |
| PLUGINS.md manual updates | No GSD equivalent | Keep (plugin-specific) |
| Contract checksum validation | No GSD equivalent | Keep (domain-specific) |
| Resource discovery/injection | No GSD equivalent | Keep (v1.2 feature) |

**Components that stay PFS-specific:**
- Plugin registry (PLUGINS.md) -- domain-specific
- Contract enforcement (parameter-spec, BRIEF.md) -- domain-specific
- Build automation (cmake/ninja) -- domain-specific
- Resource discovery and injection -- v1.2 investment, working well
- Research document frontmatter schema -- domain-specific
- Handoff protocol -- domain-specific

## Anti-Patterns to Avoid

### Anti-Pattern 1: Replacing Sequential Pipeline with Agent Teams

**What:** Converting the Stage 0-4 pipeline to parallel agent teams
**Why bad:** Stages have strict dependencies. Foundation must exist before DSP. DSP must work before GUI. GUI must render before polish. Parallelizing dependent stages produces broken artifacts.
**Instead:** Keep sequential pipeline as primary workflow. Agent Teams only for genuinely independent work (multi-plugin batch, parallel research).

### Anti-Pattern 2: Universal Persistent Memory

**What:** Adding `memory: project` to every agent
**Why bad:** Agents doing template execution (foundation-shell, ui-finalization) don't learn meaningfully. Memory files accumulate noise. Each agent's startup loads memory, adding latency.
**Instead:** Memory only for agents that demonstrate learning behavior: troubleshooting patterns, API quirks, architectural decisions.

### Anti-Pattern 3: Removing PFS Orchestration for GSD Orchestration

**What:** Replacing plugin-workflow with GSD's execute-phase workflow
**Why bad:** PFS orchestration has deep domain knowledge: stage routing, contract enforcement, build automation, complexity-based model selection, handoff protocol. GSD orchestration is generic.
**Instead:** Use GSD's mechanical operations (state management, progress tracking) while keeping PFS orchestration logic for domain-specific decisions.

### Anti-Pattern 4: Breaking JSON Schema Contracts

**What:** Adding required fields to subagent-report.json or validator-report.json
**Why bad:** All 11 agents emit reports conforming to these schemas. Adding required fields breaks all agents simultaneously.
**Instead:** New fields are always `optional` with sensible defaults. Add to schemas with `additionalProperties: true` or in a separate extension schema.

### Anti-Pattern 5: Eager Migration of All State Updates

**What:** Replacing every STATE.md/PLUGINS.md write with gsd-tools in one pass
**Why bad:** gsd-tools manages GSD-specific state (.planning/STATE.md). Plugin-specific state (PLUGINS.md, per-plugin STATUS.md) has different schema. Blindly replacing all state management risks losing plugin-specific tracking.
**Instead:** Adopt gsd-tools for GSD state (project STATE.md, phase progression). Keep PFS state management for plugin-specific files (PLUGINS.md, per-plugin STATUS.md). Evaluate convergence later.

## Scalability Considerations

| Concern | Current (35 plugins) | At 100 plugins | At 500 plugins |
|---------|---------------------|----------------|----------------|
| PLUGINS.md size | ~200 lines, manageable | ~600 lines, slow to parse | Needs database or split files |
| Research docs | 27 docs, 63ms discovery | ~50 docs, <200ms | May need categorized indices |
| Agent memory files | 0 (new) | 5 files, ~20KB each | Monitor for bloat, add curation |
| Build time | ~30s per plugin | Same (parallel builds possible) | CI/CD pipeline needed |
| Context injection | 4K token budget | Same budget, better relevance needed | May need semantic search |

## Sources

- [Claude Code Subagents Documentation](https://code.claude.com/docs/en/sub-agents) -- Official docs on subagent architecture, YAML format, tools, memory, hooks (verified 2026-02-08)
- [Claude Code Agent Teams Documentation](https://code.claude.com/docs/en/agent-teams) -- Official docs on experimental agent teams feature (verified 2026-02-08)
- [Claude Agent SDK Subagents](https://platform.claude.com/docs/en/agent-sdk/subagents) -- SDK documentation for programmatic agent definition (verified 2026-02-08)
- [GSD CHANGELOG.md](https://github.com/glittercowboy/get-shit-done/blob/main/CHANGELOG.md) -- Versions 1.15.0-1.18.0 feature details (verified 2026-02-08)
- [GSD Framework Repository](https://github.com/glittercowboy/get-shit-done) -- Source for GSD architecture and tooling (verified 2026-02-08)
- Local codebase analysis: 11 agent definitions, 44 command files, 6 hooks, 27 research docs (verified 2026-02-08)
