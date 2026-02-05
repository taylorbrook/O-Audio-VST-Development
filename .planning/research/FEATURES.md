# Feature Research: Agent Intelligence & Resource Orchestration

**Domain:** LLM agent resource orchestration for file-based development systems
**Researched:** 2026-02-04
**Confidence:** HIGH (verified against Claude Code hooks documentation, existing system code, and community patterns)

## Feature Landscape

### Table Stakes (System Must Have These)

Missing these = agents operate without relevant knowledge, producing subpar results that require rework.

| Feature | Why Expected | Complexity | Dependencies | Notes |
|---------|--------------|------------|--------------|-------|
| **TS-1: Keyword-based resource discovery** | Agents working on FFT plugins must find FFT research docs. Without this, the 23 research docs are invisible to agents. | MEDIUM | Requires resource manifest (TS-5) | Match task prompt keywords against document metadata. Pattern: SubagentStart hook reads agent_type + task prompt, scans manifest for keyword matches. |
| **TS-2: SubagentStart context injection** | Claude Code's SubagentStart hook is the only guaranteed mechanism to inject context into subagents. Skills are skipped 56% of the time (Vercel research). CLAUDE.md is read once. Hooks run every time. | LOW | Existing hook infrastructure (.claude/hooks/) | Hook receives agent_type, outputs additionalContext via hookSpecificOutput JSON. Already proven pattern in the PostToolUse and SubagentStop hooks. |
| **TS-3: Resource injection into agent prompts** | Agents must receive relevant documents before executing, not discover them mid-task (which wastes context and may not happen). Pre-injection is the standard pattern for reliable agent behavior. | MEDIUM | TS-1 (discovery), TS-2 (injection mechanism) | Inject document summaries + file paths. Full document injection burns context; reference injection risks the agent never reading the file. Use summary + path as the default strategy. |
| **TS-4: Agent invocation audit** | The system has 13 agents but no verification that the right agents are invoked for a given task. If dsp-agent skips research docs, there is no detection. | MEDIUM | TS-6 (usage reporting) | PostToolUse or SubagentStop hook validates that expected resources were consulted. Audit log written to .planning/workflow/ or per-plugin staging area. |
| **TS-5: Resource manifest file** | The 23 research docs and troubleshooting patterns have no metadata. File names are the only signal. A manifest maps each document to keywords, categories, applicable plugin types, and freshness dates. | LOW | None (foundational) | Single JSON or YAML file: research/MANIFEST.yaml. Each entry: path, keywords[], categories[], applicable_stages[], freshness_date. This is the index that makes discovery possible. |
| **TS-6: Agent usage reporting in JSON reports** | Agents already return JSON reports (subagent-report.json schema). Add resources_consulted field. Without this, you cannot know if agents used available knowledge. | LOW | Existing JSON report schema (.claude/schemas/subagent-report.json) | Add optional fields: resources_consulted[], resources_recommended[], resources_ignored[]. Backward compatible with existing schema. |
| **TS-7: Hook-based pre-agent context loading** | The UserPromptSubmit hook already injects /continue context. Extend pattern: when a plugin-plan or plugin-research command runs, inject relevant research docs based on plugin type and current stage. | MEDIUM | TS-1 (discovery), TS-5 (manifest) | Pattern already proven in UserPromptSubmit.sh. Extend to cover /plugin-research, /plugin-plan, /implement commands. Output relevant doc summaries as additionalContext. |

### Differentiators (What Makes This System Exceptional)

Features that go beyond basic resource orchestration into genuine intelligence.

| Feature | Value Proposition | Complexity | Dependencies | Notes |
|---------|-------------------|------------|--------------|-------|
| **D-1: Cross-plugin knowledge transfer** | When building a new FFT plugin, the system should surface lessons learned from previous FFT-related plugins (e.g., O-SpectralShaper's architecture decisions, fft-artifact-prevention.md findings). No existing system does this well. | HIGH | TS-5 (manifest), plugin-registry.json, per-plugin .planning/ artifacts | Build a knowledge graph: plugin A used research doc X, had architecture decision Y, encountered pitfall Z. New plugin B with similar characteristics gets those insights injected. Requires cross-referencing plugin types with research docs and completed plugin artifacts. |
| **D-2: Stage-aware resource filtering** | Not all resources are relevant at all stages. FFT research is critical during Stage 0 (planning) and Stage 2 (DSP), irrelevant during Stage 3 (GUI). Filter by applicable_stages in manifest. | LOW | TS-5 (manifest with stage metadata) | Add applicable_stages[] to manifest entries. SubagentStart hook reads current stage from STATUS.md frontmatter, filters resources accordingly. Prevents context pollution from irrelevant docs. |
| **D-3: Freshness tracking and stale-doc warnings** | Research docs age. JUCE API patterns from 6 months ago may be outdated. Track when each doc was last verified and warn agents when using stale resources. | LOW | TS-5 (manifest with freshness_date) | Manifest entry includes last_verified date. If stale (>90 days), inject warning: "This document was last verified on [date]. Verify critical claims with Context7 before relying on them." Low-effort, high-trust improvement. |
| **D-4: Decision provenance chain** | Track not just what resources an agent consulted, but which specific resource influenced which decision. Connect ARCHITECTURE.md decisions back to their source documents. | HIGH | TS-6 (usage reporting), existing architecture/planning artifacts | Agents annotate decisions: "Chose StateVariableTPTFilter based on reverb-comprehensive-research.md Section 3.2." Stored in CONTEXT.md per stage. Enables tracing why a decision was made months later. Requires agent prompt engineering, not just hooks. |
| **D-5: Automatic resource recommendation** | When a new plugin's BRIEF.md mentions "spectral" or "frequency domain," the system automatically recommends relevant research docs before the user asks. Surfaces in /plugin-plan output. | MEDIUM | TS-1 (discovery), TS-5 (manifest) | Run discovery at BRIEF.md creation time (/start command). Write recommendations to .planning/RECOMMENDED-RESOURCES.md. Agent prompts reference this file. User sees "3 research docs found relevant to this plugin." |
| **D-6: Resource gap detection** | When planning a plugin type with no existing research docs, flag it. "No research found for granular synthesis. Consider running /plugin-research before proceeding." | LOW | TS-5 (manifest), TS-1 (discovery) | If discovery returns zero matches for a plugin's domain keywords, emit warning in SubagentStart hook output and in /plugin-plan output. Prevents agents from proceeding without knowledge when knowledge should exist. |
| **D-7: Troubleshooting pattern auto-injection** | The system has juce8-critical-patterns.md and stage-specific pattern files. These should be automatically injected based on current stage, not rely on agent prompt text saying "read this file." | MEDIUM | Existing pattern files, TS-2 (injection mechanism) | SubagentStart hook for foundation-shell-agent injects stage-1-patterns.md content. For dsp-agent, inject stage-2-patterns.md. Already referenced in agent prompts but relies on agent compliance. Hook injection guarantees it. |
| **D-8: Module-research cross-referencing** | When a module (e.g., scala-tuning-engine) is relevant to a plugin being planned, surface the module documentation alongside research docs. Modules are code; research docs explain the domain. | MEDIUM | TS-5 (manifest), module system (existing) | Extend manifest to include module associations. When microtonality research docs are surfaced, also note: "Module available: scala-tuning-engine@1.0.0 - provides Scala file parsing and frequency table generation." |

### Anti-Features (Deliberately NOT Building)

Features that seem appealing but create problems in this specific context.

| Anti-Feature | Why Requested | Why Problematic | Alternative |
|--------------|---------------|-----------------|-------------|
| **AF-1: Vector embedding / semantic search for docs** | "Use RAG to find relevant documents" is the default industry answer. Embeddings could match research docs to task prompts semantically. | Massive over-engineering for 23 documents. Requires embedding infrastructure (database, model, indexing pipeline). Keyword matching on a manifest is sufficient for a collection this size. Adds operational complexity (embedding model updates, index rebuilds) without proportional benefit. Would become relevant only at 200+ documents. | Keyword-based manifest matching (TS-1 + TS-5). Simple, deterministic, debuggable. |
| **AF-2: Full document injection into context** | "Just inject the entire document so the agent has everything." | Research docs range from 2KB to 50KB. Injecting full documents burns 5,000-50,000 tokens of context window per document. With multiple relevant docs, this leaves insufficient context for the actual task. Context rot -- the agent loses focus when overloaded with information. | Summary injection with read-on-demand paths (TS-3). Inject 200-word summary + file path. Agent reads the full document only if needed. |
| **AF-3: Dynamic agent spawning based on task analysis** | "The system should analyze the task and automatically decide which agents to spawn." | Introduces unpredictable behavior. The current explicit pipeline (research-planning-agent -> foundation-shell-agent -> dsp-agent -> gui-agent) is reliable because it is deterministic. Dynamic spawning creates debugging nightmares: "Why did it spawn aesthetics-agent instead of dsp-agent?" | Keep deterministic pipeline. Improve what each agent receives (context injection) rather than which agents run. Add audit logging (TS-4) to verify correct invocation, not automatic re-routing. |
| **AF-4: Agent-to-agent direct communication** | "Agents should be able to message each other directly for coordination." | Claude Code subagents are context-isolated by design. Direct communication breaks this isolation, creates race conditions, and makes behavior non-reproducible. The current handoff-via-files pattern (STATUS.md, JSON reports) is the correct architecture. | File-based handoffs (existing). Enhance what is written to handoff files (include resource usage reports). SubagentStop hooks can enrich handoff data. |
| **AF-5: Real-time resource relevance scoring with LLM** | "Use an LLM to score which documents are most relevant to the current task." | Adds latency (1-3 seconds per hook invocation for LLM call). Costs money per invocation. Non-deterministic -- same task might get different resources on different runs. Makes debugging harder. | Deterministic keyword matching on manifest (TS-1). Same inputs always produce same outputs. Fast (<100ms). Free. Debuggable. |
| **AF-6: Mandatory resource consumption enforcement** | "Block agent execution if it did not read every recommended resource." | Agents sometimes have good reasons to skip a resource (irrelevant section, already incorporated knowledge from training). Mandatory enforcement creates friction, slows agents, and burns context on documents that may not be useful. Enforcement should be at the reporting level, not the blocking level. | Usage reporting (TS-6) + audit logging (TS-4). Agents report what they used. Post-hoc audit flags when expected resources were not consulted. Human reviews outliers rather than blanket enforcement. |
| **AF-7: Building a custom knowledge base UI/dashboard** | "Create a web interface to manage the research document collection." | Gold-plating. The research collection is managed by the developer (you) via text files and git. A UI adds maintenance burden, creates another thing that can break, and solves a problem that does not exist -- you already know your documents. | Manifest file (TS-5) managed in any text editor. /plugin-research command surfaces relevant docs in terminal. Git tracks changes. |

## Feature Dependencies

```
[TS-5: Resource Manifest]
    |
    +--required-by--> [TS-1: Keyword Discovery]
    |                      |
    |                      +--required-by--> [TS-3: Resource Injection]
    |                      |                      |
    |                      |                      +--required-by--> [D-1: Cross-Plugin Knowledge]
    |                      |                      +--required-by--> [D-5: Auto Recommendation]
    |                      |
    |                      +--required-by--> [TS-7: Hook-Based Context Loading]
    |                      +--required-by--> [D-6: Resource Gap Detection]
    |
    +--required-by--> [D-2: Stage-Aware Filtering]
    +--required-by--> [D-3: Freshness Tracking]
    +--required-by--> [D-8: Module Cross-Reference]

[TS-2: SubagentStart Injection]
    |
    +--required-by--> [TS-3: Resource Injection]
    +--required-by--> [D-7: Pattern Auto-Injection]
    +--required-by--> [TS-7: Hook-Based Context Loading]

[TS-6: Usage Reporting]
    |
    +--required-by--> [TS-4: Invocation Audit]
    +--required-by--> [D-4: Decision Provenance]

[Existing: JSON Report Schema] --enhances--> [TS-6]
[Existing: SubagentStop Hook] --enhances--> [TS-4]
[Existing: UserPromptSubmit Hook] --pattern-for--> [TS-7]
[Existing: PostToolUse Hook] --pattern-for--> [TS-2]
```

### Dependency Notes

- **TS-5 (Manifest) is foundational:** Nearly everything depends on having a structured index of documents. Build this first.
- **TS-2 (SubagentStart) is the delivery mechanism:** The manifest tells you what to inject; SubagentStart is how you inject it. These two together enable the entire pipeline.
- **TS-6 (Usage Reporting) is independent:** Can be built in parallel with discovery/injection features. Requires only schema changes to existing JSON reports.
- **D-1 (Cross-Plugin Knowledge) is the hardest:** Requires manifest, discovery, and cross-referencing with plugin-registry.json and per-plugin artifacts. Build last.
- **D-7 (Pattern Auto-Injection) has quick wins:** The pattern files already exist and are already referenced in agent prompts. Moving from "agent should read this" to "hook guarantees injection" is a small change with large reliability improvement.

## MVP Definition

### Launch With (v1.2.0)

Minimum viable resource orchestration -- agents receive relevant context and report what they used.

- [ ] **TS-5: Resource manifest** -- Create research/MANIFEST.yaml with keywords, categories, stages for all 23 research docs + troubleshooting patterns
- [ ] **TS-1: Keyword-based discovery** -- Python script that matches task keywords against manifest entries, returns ranked list of relevant resources
- [ ] **TS-2: SubagentStart context injection** -- Hook that injects discovered resources into agent context via additionalContext
- [ ] **TS-6: Usage reporting** -- Extend subagent-report.json schema with resources_consulted field
- [ ] **D-2: Stage-aware filtering** -- Filter manifest by current stage (trivial addition to TS-1 once manifest has stage metadata)
- [ ] **D-7: Pattern auto-injection** -- Inject stage-specific patterns via SubagentStart hook (quick win, high impact)

### Add After Validation (v1.2.x)

Features to add once core resource orchestration is working and validated.

- [ ] **TS-4: Agent invocation audit** -- Trigger: after validating that SubagentStart injection works reliably, add post-execution audit
- [ ] **TS-7: Hook-based pre-agent context loading** -- Trigger: when /plugin-research and /plugin-plan commands are exercised with new plugins
- [ ] **D-3: Freshness tracking** -- Trigger: when a research doc is found to contain outdated information during actual use
- [ ] **D-5: Auto resource recommendation** -- Trigger: after manifest is validated with several plugins, auto-surface recommendations at /start time
- [ ] **D-6: Resource gap detection** -- Trigger: after discovery is working, add zero-result detection with warning

### Future Consideration (v1.3+)

Features to defer until resource orchestration is proven and the knowledge base grows.

- [ ] **D-1: Cross-plugin knowledge transfer** -- Why defer: requires significant graph-building infrastructure, only valuable after 10+ completed plugins
- [ ] **D-4: Decision provenance chain** -- Why defer: requires agent prompt engineering across all 13 agents, high effort for incremental benefit
- [ ] **D-8: Module-research cross-referencing** -- Why defer: module system is stable but module count is small; benefit scales with module count
- [ ] **TS-3 refinement: Adaptive injection depth** -- Why defer: need usage data to know whether summaries are sufficient or full docs needed

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority | Phase |
|---------|------------|---------------------|----------|-------|
| TS-5: Resource Manifest | HIGH | LOW | P1 | Phase 1 |
| TS-1: Keyword Discovery | HIGH | MEDIUM | P1 | Phase 1 |
| TS-2: SubagentStart Injection | HIGH | LOW | P1 | Phase 1 |
| TS-6: Usage Reporting | HIGH | LOW | P1 | Phase 1 |
| D-7: Pattern Auto-Injection | HIGH | LOW | P1 | Phase 1 |
| D-2: Stage-Aware Filtering | MEDIUM | LOW | P1 | Phase 1 |
| TS-4: Agent Invocation Audit | MEDIUM | MEDIUM | P2 | Phase 2 |
| TS-7: Pre-Agent Context Loading | MEDIUM | MEDIUM | P2 | Phase 2 |
| D-3: Freshness Tracking | MEDIUM | LOW | P2 | Phase 2 |
| D-5: Auto Recommendation | MEDIUM | MEDIUM | P2 | Phase 2 |
| D-6: Resource Gap Detection | MEDIUM | LOW | P2 | Phase 2 |
| TS-3 refinement | LOW | MEDIUM | P3 | Phase 3 |
| D-1: Cross-Plugin Knowledge | HIGH | HIGH | P3 | v1.3+ |
| D-4: Decision Provenance | MEDIUM | HIGH | P3 | v1.3+ |
| D-8: Module Cross-Reference | LOW | MEDIUM | P3 | v1.3+ |

**Priority key:**
- P1: Must have for v1.2.0 launch -- agents reliably receive and report on relevant resources
- P2: Should have, add when P1 is validated -- audit, freshness, auto-recommendation
- P3: Nice to have, future milestone -- cross-plugin intelligence, provenance tracking

## Comparable System Analysis

| Feature | Superpowers (obra) | AGENTS.md Standard | Agentic RAG Systems | Our Approach |
|---------|-------------------|-------------------|---------------------|--------------|
| Context injection | SessionStart hook (misses subagents - known bug) | Progressive disclosure via file structure | Vector embeddings + retrieval | SubagentStart hook (guaranteed per-agent injection) |
| Document discovery | Not addressed | Manual references in AGENTS.md | Semantic search | Keyword matching on structured manifest |
| Usage accountability | Not addressed | Not addressed | Trace logging (LangWatch, Langfuse) | JSON report schema extension + audit hooks |
| Knowledge organization | Not addressed | Nested AGENTS.md files | Vector databases + metadata | YAML manifest with keywords, stages, freshness |
| Stale content handling | Not addressed | "File paths change constantly" warning | TTL-based cache invalidation | Freshness dates in manifest + stale-doc warnings |

**Key insight:** No comparable system in the Claude Code ecosystem addresses all five dimensions (discovery, injection, accountability, hooks, knowledge management) as an integrated system. Most focus on one dimension. The Plugin Freedom System has an opportunity to be the reference implementation for resource-aware agent orchestration.

## Implementation Architecture Summary

The features compose into a three-layer architecture:

**Layer 1: Knowledge Index (TS-5)**
- research/MANIFEST.yaml -- structured metadata for all knowledge resources
- Format: path, keywords[], categories[], applicable_stages[], freshness_date, summary (200 words max)
- Maintained manually (new research docs get manifest entries) or via /research command post-hook

**Layer 2: Discovery Engine (TS-1, D-2, D-6)**
- Python script: .claude/scripts/resource-discovery.py
- Input: task keywords (from agent prompt or command args), current stage, plugin type
- Output: ranked list of relevant resources with paths and summaries
- Deterministic, fast (<100ms), no external dependencies

**Layer 3: Injection & Reporting (TS-2, TS-6, TS-7, D-7)**
- SubagentStart hook: calls discovery engine, injects results as additionalContext
- SubagentStop hook (extended): validates usage reporting in agent JSON output
- UserPromptSubmit hook (extended): injects context for /plugin-research, /plugin-plan, /implement
- Agent JSON reports: extended schema with resources_consulted field

## Sources

### Authoritative (HIGH confidence)
- [Claude Code Hooks Reference](https://code.claude.com/docs/en/hooks) -- Complete hook event schemas, SubagentStart/SubagentStop input/output, additionalContext injection capability
- [Claude Code Subagents Documentation](https://code.claude.com/docs/en/sub-agents) -- Task tool, agent types, context isolation design
- Existing system code reviewed: `.claude/hooks/SessionStart.sh`, `PostToolUse.sh`, `SubagentStop.sh`, `UserPromptSubmit.sh`, `Stop.sh`, `PreCompact.sh`
- Existing agent definitions reviewed: `.claude/agents/research-planning-agent.md`, `dsp-agent.md`, `foundation-shell-agent.md`
- Existing schemas reviewed: `.claude/schemas/subagent-report.json`, `agent-contracts/`

### Verified (MEDIUM confidence)
- [Guaranteed Context Injection via Hooks](https://dev.to/sasha_podles/claude-code-using-hooks-for-guaranteed-context-injection-2jg) -- Skills skipped 56% of the time (Vercel research); hooks are guaranteed
- [Superpowers Subagent Context Gap](https://github.com/obra/superpowers/issues/237) -- SubagentStart hook as solution for missing subagent context
- [Context Engineering for AI Agents](https://www.siddharthbharath.com/a-guide-to-context-engineering-setting-agents-up-for-success/) -- Selective context outperforms full-context; context engineering as discipline
- [Keeping Claude Code Subagents Aligned](https://andrebremer.com/articles/keeping-your-claude-code-subagents-aligned/) -- Policy reference resolution pattern for SubagentStart hooks
- [LLM Observability Guide](https://portkey.ai/blog/the-complete-guide-to-llm-observability/) -- Audit trail patterns: traces, metrics, events for agent systems
- [Agentic Orchestration Frameworks](https://research.aimultiple.com/agentic-orchestration/) -- Multi-agent coordination patterns, specialized agent teams
- [Best Practices for Claude Code Subagents](https://www.pubnub.com/blog/best-practices-for-claude-code-sub-agents/) -- Context isolation benefits, skill injection, tool restriction patterns

### Supplementary (LOW confidence -- patterns, not specific claims)
- [Audit Trails for Accountability in LLMs](https://arxiv.org/html/2601.20727) -- Lifecycle framework for audit events, tamper-evident logging
- [Context Engineering Hybrid Search](https://www.elastic.co/search-labs/blog/context-engineering-hybrid-search-evolution-agentic-ai) -- Keyword + semantic hybrid approach (valuable pattern, overkill for our scale)
- [AGENTS.md Guide](https://www.aihero.dev/a-complete-guide-to-agents-md) -- Progressive disclosure pattern, "describe capabilities not structure"
- [Dynamic Context Injection in Claude Code](https://www.365iwebdesign.co.uk/news/2026/01/29/how-to-use-dynamic-context-injection-claude-code/) -- DCI patterns with shell command interpolation in skills

## Quality Gate Verification

- [x] Categories are clear (table stakes vs differentiators vs anti-features)
- [x] Complexity noted for each feature (LOW/MEDIUM/HIGH)
- [x] Dependencies on existing features identified
- [x] Features are practical for Claude Code CLI context
- [x] MVP definition with phased approach
- [x] Anti-features documented with alternatives
- [x] Sources documented with confidence levels
- [x] Feature dependency graph showing build order

---
*Feature research for: Agent Intelligence & Resource Orchestration (v1.2)*
*Researched: 2026-02-04*
