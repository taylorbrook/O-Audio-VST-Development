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
