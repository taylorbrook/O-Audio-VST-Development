# Project Research Summary

**Project:** Plugin Freedom System v1.2 - Agent Intelligence & Resource Orchestration
**Domain:** AI agent resource discovery and context injection for JUCE audio plugin development
**Researched:** 2026-02-04
**Confidence:** HIGH

## Executive Summary

The Plugin Freedom System has 23 research documents, 11 agents, 25 skills, and rich plugin planning artifacts — but no mechanism connects research knowledge to agent execution. Research docs exist only when a human manually adds them to prompts. The v1.2 milestone solves this by adding **resource discovery** (which docs are relevant?), **context injection** (deliver resources to agents before execution), and **usage accountability** (verify agents consulted appropriate resources).

The key technical insight: **static manifest + keyword matching beats semantic search for a 23-document corpus**. Vector databases and embedding pipelines are over-engineering. A JSON index mapping topics to file paths, combined with keyword matching against agent prompts, delivers 100ms discovery with zero external dependencies. Context injection happens via **SubagentStart hooks** (guaranteed delivery to subagents) or **skill orchestrator prompt augmentation** (for main workflow agents). Accountability is **warning-level, not blocking** — agents self-report resources consulted in JSON reports, validated by SubagentStop hooks, but missing resources trigger warnings not workflow failures.

The main risks: (1) **context window budget exhaustion** if full documents are injected instead of summaries + paths, (2) **false relevance** from DSP keyword ambiguity (every doc mentions "frequency" and "filter"), and (3) **breaking existing JSON Schema contracts** with `additionalProperties: false`. Prevention: inject 2,000-4,000 token summaries maximum, use structured metadata tags not content search, and make all new schema fields optional with defaults.

## Key Findings

### Recommended Stack

The existing system has the right infrastructure bones: 6 hooks, 11 agents, 24 skills, 23 research documents, and JSON Schema contracts. This milestone adds intelligence on top of that infrastructure without external services, vector databases, or runtime dependencies. Everything is bash/Python scripts, JSON index files, and hook configuration.

**Core technologies:**
- **Python 3.9+ with stdlib only** — Index builder, resource matcher, usage tracker. Already a dependency for validators. No new packages needed except optionally `bm25s` if keyword matching proves insufficient (defer to v1.3+).
- **JSON manifest (`.claude/resource-index.json`)** — Static catalog of all resources with keywords, categories, agents, summaries. Follows `plugin-registry.json` pattern. Native to system (schemas, contracts, registry all use JSON). Parseable by both bash (`jq`) and Python.
- **Bash hooks** — SubagentStart for context injection, SubagentStop for usage validation, SessionStart for index freshness. Consistent with existing hook infrastructure.
- **jq 1.6+** — JSON parsing in hook scripts. Already validated by SessionStart hook. Used for extracting agent_type, prompt text from hook stdin.

### Expected Features

**Must have (table stakes):**
- Keyword-based resource discovery matching task prompts against manifest
- SubagentStart context injection (guaranteed delivery mechanism)
- Resource injection as file paths + summaries (not full content)
- Agent usage reporting in JSON reports (`resources_consulted` field)
- Agent invocation audit (SubagentStop validates usage)
- Static resource manifest with keywords, categories, stages, summaries
- Hook-based pre-agent context loading for guaranteed injection

**Should have (differentiators):**
- Stage-aware resource filtering (FFT research relevant at Stage 2, not Stage 3)
- Freshness tracking with stale-doc warnings (YAML frontmatter with `last_verified` dates)
- Auto-generated manifest from document frontmatter (no manual maintenance drift)
- Pattern auto-injection (stage-specific patterns like `stage-2-patterns.md` injected via hooks)
- Resource gap detection ("No research found for granular synthesis — consider /plugin-research")
- Automatic resource recommendation at `/start` command (surfaces docs based on BRIEF.md)

**Defer (v2+):**
- Cross-plugin knowledge transfer (requires knowledge graph, only valuable after 10+ completed plugins)
- Decision provenance chain (track which resource influenced which ARCHITECTURE.md decision)
- Module-research cross-referencing (extend manifest to include module associations)
- Adaptive injection depth based on agent usage patterns (learn whether summaries suffice or full docs needed)

### Architecture Approach

Discovery happens in the **skill orchestrator** before Task() invocation, not in hooks or agents themselves. Orchestrators already construct prompts with contracts; adding resource discovery is a natural extension. A Python script (`resource-discovery.py`) matches task context (plugin name, stage, agent type, extracted keywords) against the resource index and returns ranked results. The orchestrator appends discovered resources to the agent prompt as file paths with summaries and read instructions. The agent uses Read tool to load full content on demand.

**Major components:**
1. **Resource Index (`.claude/resource-index.json`)** — Static catalog with metadata: path, keywords, tags, dsp_topics, relevant_agents, relevant_plugins, summary, size_kb. Regenerated when docs change via `build-resource-index.py` script.
2. **Discovery Script (`resource-discovery.py`)** — Scores resources against task context using keyword overlap, agent affinity, and domain matching. Returns ranked list (MUST-READ vs SHOULD-READ) in <100ms.
3. **Skill Orchestrator (modified `plugin-workflow`, `plugin-improve`, `plugin-planning`)** — Calls discovery script, formats results, appends to Task() prompt before spawning agent.
4. **SubagentStop Hook (extended)** — Validates agent JSON report for `resources_consulted` field. Warns if MUST-READ resources skipped but does not block workflow (warning-level accountability).
5. **SessionStart Hook (extended)** — Checks if resource index is stale (source files newer than index). Rebuilds automatically if needed.

### Critical Pitfalls

1. **Context Window Budget Exhaustion (Pitfall 24)** — Injecting full research docs (10-72KB each) pushes agents past effective performance threshold. Agent prompts already 25K tokens (dsp-agent). Research shows LLM performance drops below 50% at 32K tokens. **Avoid:** Inject summaries + paths only (2,000-4,000 token budget max). Agent reads full docs via Read tool on demand.

2. **False Relevance from DSP Keyword Ambiguity (Pitfall 25)** — Every research doc mentions "frequency", "filter", "envelope", "phase" because all are audio DSP. Simple keyword matching returns wrong documents. **Avoid:** Use structured metadata tags (not content search), match on plugin BRIEF.md context (not generic keywords), curate static mapping for 23-doc corpus.

3. **Breaking Existing Contracts with `additionalProperties: false` (Pitfall 26)** — JSON Schema contracts use strict validation. Adding `resources_consulted` field breaks ALL agents unless ALL updated atomically. **Avoid:** Make new fields optional with defaults. Never add to `required` array. Update all 6 consumer agents + schema + validators in same commit.

4. **Resource Discovery Becoming Single Point of Failure (Pitfall 27)** — If discovery crashes, agents either run without resources (silent failure) or can't run at all (workflow blocked). **Avoid:** Graceful degradation — discovery failures never block agents. Log warnings. Static fallback mapping ensures basic functionality if dynamic discovery fails.

5. **Hook Timeout Violations (Pitfall 28)** — PostToolUse.sh has 2s timeout with ~500ms available budget. Adding discovery (1-3s with file I/O) will cause failures. **Avoid:** Do NOT add discovery to PostToolUse.sh. Run discovery in orchestrator skill (no timeout constraint) or SessionStart.sh (3s available). Benchmark: discovery must complete <1s.

## Implications for Roadmap

Based on research, suggested **4-phase build order** with linear dependencies:

### Phase 1: Resource Index & Discovery Foundation
**Rationale:** All subsequent phases depend on a working index and discovery script. This must exist before any injection or accountability code is written.

**Delivers:**
- `.claude/resource-index.json` with all 23 research docs manually cataloged
- `.claude/scripts/resource-discovery.py` matching keywords to resources
- `.claude/schemas/resource-index.schema.json` validation
- Decision documentation: static manifest (not vector search), keyword matching (not semantic), orchestrator-level discovery (not hooks)

**Addresses:**
- TS-5 (Resource manifest file) — foundational for all other features
- TS-1 (Keyword-based resource discovery) — basic matching engine
- Pitfall 31 (over-engineering) — explicitly choose static manifest as design decision

**Avoids:**
- Pitfall 31 — No vector databases or embedding pipelines for 23 documents
- Pitfall 25 — Structured tags prevent keyword ambiguity
- Pitfall 32 — YAML frontmatter with freshness metadata included from start

**Research flag:** No additional research needed — pattern well-documented (follows `plugin-registry.json`).

---

### Phase 2: Context Injection (Skill Orchestrators)
**Rationale:** Once discovery works, modify orchestrators to inject discovered resources into agent prompts. This is where context delivery happens. Depends on Phase 1 (index + script must exist).

**Delivers:**
- Modified `.claude/skills/plugin-workflow/SKILL.md` with discovery + injection in execute/research phases
- Modified `.claude/skills/plugin-planning/SKILL.md` with discovery before research-planning-agent
- Modified `.claude/skills/plugin-improve/SKILL.md` with discovery in Phase 0.5 investigation
- Injection format: file paths + summaries in `<reference_material>` section at end of prompt

**Addresses:**
- TS-2 (SubagentStart context injection) — guaranteed delivery via hooks
- TS-3 (Resource injection into agent prompts) — summary + path strategy
- TS-7 (Hook-based pre-agent context loading) — extension of existing patterns
- D-7 (Pattern auto-injection) — stage-specific patterns via SubagentStart

**Uses:**
- Python 3 keyword extraction from ARCHITECTURE.md and parameter-spec.md
- Bash hook infrastructure for SubagentStart integration
- Existing prompt construction patterns in plugin-workflow

**Implements:**
- Discovery in orchestrator (before Task() invocation)
- Prompt augmentation (append resources after agent instructions)
- SubagentStart hook for guaranteed subagent injection

**Avoids:**
- Pitfall 24 — Token budget enforced: 2,000-4,000 tokens max injected content
- Pitfall 27 — Graceful degradation: agents run without resources if discovery fails
- Pitfall 28 — Discovery in orchestrator (no timeout) not PostToolUse.sh
- Pitfall 29 — Resources injected at end with priority directive to preserve instruction adherence

**Research flag:** Moderate — needs testing with known-good plugins (O-Bells) to verify injection doesn't disrupt output quality.

---

### Phase 3: Accountability (Schema + Validation)
**Rationale:** After agents receive resources (Phase 2), add reporting and validation. This phase depends on injection working so there's something to report on.

**Delivers:**
- Extended `.claude/schemas/subagent-report.json` with optional `resources_consulted` field
- Modified agent definitions (dsp-agent, gui-agent, research-planning-agent, foundation-shell-agent, polish-agent) documenting `resources_consulted` in JSON reports
- Extended `.claude/hooks/SubagentStop.sh` with resource usage validation case
- `.claude/hooks/validators/validate-resource-usage.py` comparing injected vs consulted resources

**Addresses:**
- TS-6 (Agent usage reporting in JSON reports) — self-reporting field
- TS-4 (Agent invocation audit) — SubagentStop validation
- D-4 (Decision provenance) — foundation for tracking resource influence (full implementation deferred to v1.3+)

**Implements:**
- Optional schema field with default (backward compatible)
- Warning-level validation (not blocking)
- Transcript parsing for Read tool calls (usage detection)
- Per-session usage logs (`.claude/usage-logs/{session_id}.json`)

**Avoids:**
- Pitfall 26 — Optional fields with defaults, MINOR version bump, atomic updates to all 6 consumers
- Pitfall 30 — Focus on output-based verification not just self-reports (validate agents use content, not just list filenames)

**Research flag:** Low — JSON Schema evolution pattern well-documented, validation follows existing `validate-checksums.py` pattern.

---

### Phase 4: Maintenance Tooling & Enhancements
**Rationale:** Once core discovery + injection + accountability works (Phases 1-3), add automation to reduce manual maintenance burden. This is quality-of-life, not blocking for core functionality.

**Delivers:**
- `.claude/scripts/generate-resource-index.py` auto-generating manifest from YAML frontmatter
- `.claude/scripts/extract-context-terms.py` extracting keywords from plugin artifacts
- Modified `deep-research` skill to emit YAML frontmatter in new research docs
- SessionStart.sh index freshness check and auto-rebuild

**Addresses:**
- D-3 (Freshness tracking) — auto-warn on stale docs
- D-5 (Auto resource recommendation) — surface docs at `/start` command
- D-6 (Resource gap detection) — warn when no docs match plugin type
- Pitfall 33 (manifest maintenance drift) — auto-generation prevents manual updates

**Implements:**
- YAML frontmatter extraction (created, last_verified, juce_version, topics, plugins)
- Default tags from filename if frontmatter missing
- Validation: all research/*.md files indexed, all manifest entries reference existing files
- Non-blocking warnings for freshness and coverage gaps

**Avoids:**
- Pitfall 33 — Auto-generation from metadata prevents manual maintenance drift
- Pitfall 32 — Frontmatter standardization with freshness dates

**Research flag:** Low — maintenance scripting is straightforward (scan files, parse frontmatter, write JSON).

---

### Phase Ordering Rationale

```
Phase 1 (Index + Discovery)
  |
  +--- Foundation for all other phases. Testable independently via CLI.
  |
Phase 2 (Injection)
  |
  +--- Requires Phase 1. Delivers value to agents. Testable via agent output comparison.
  |
Phase 3 (Accountability)
  |
  +--- Requires Phase 2. No value without injection working first. Testable via usage logs.
  |
Phase 4 (Maintenance)
  |
  +--- Quality-of-life improvements. Not blocking for core functionality.
```

**Dependency justification:**
- Phase 2 cannot proceed without Phase 1 (no index = no discovery results)
- Phase 3 cannot validate usage without Phase 2 (agents don't receive resources yet)
- Phase 4 is parallel/independent (can start anytime after Phase 1)

**Grouping justification:**
- Each phase delivers independently testable value
- Each phase has clear success criteria
- Linear dependencies prevent partial-completion confusion

**Pitfall avoidance:**
- Building in order prevents rework (e.g., don't validate usage before injection works)
- Static manifest first prevents over-engineering temptation (semantic search)
- Graceful degradation designed-in from Phase 1 (not retrofitted)

### Research Flags

**Phases needing deeper research during planning:**
- **Phase 2 (Injection):** Needs A/B testing with known-good plugins to verify injection format doesn't disrupt agent behavior. Moderate complexity — prompt engineering requires validation.

**Phases with standard patterns (skip research-phase):**
- **Phase 1 (Index):** Follows `plugin-registry.json` pattern exactly. No novel patterns.
- **Phase 3 (Accountability):** Follows existing validator patterns (`validate-checksums.py`, `validate-dsp-components.py`). JSON Schema evolution well-documented.
- **Phase 4 (Maintenance):** Straightforward file scanning and YAML parsing. No research needed.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Verified against official Claude Code docs (SubagentStart additionalContext, SubagentStop agent_transcript_path). All dependencies already present (Python 3.9+, jq 1.6+). No new packages required. |
| Features | HIGH | Table stakes validated via direct codebase inspection (11 agents, 6 hooks, 23 docs measured). Differentiators grounded in comparable systems (Superpowers, AGENTS.md standard). Anti-features based on RAG false positive research + Claude context window studies. |
| Architecture | HIGH | Discovery location (orchestrator), injection mechanism (prompt append), and validation approach (SubagentStop) all follow existing system patterns. Static manifest follows `plugin-registry.json` pattern. No novel architectural primitives. |
| Pitfalls | HIGH | Context exhaustion, false relevance, and schema breaking verified via direct measurement (agent prompt sizes, research doc sizes, `additionalProperties: false` in schemas). Hook timeouts measured from `hooks.json`. DSP keyword ambiguity confirmed by inspecting all 23 doc filenames and headers. |

**Overall confidence:** HIGH

All core claims verified via:
1. Direct codebase inspection (measured prompts, hooks, schemas, research docs)
2. Official Claude Code documentation (SubagentStart/SubagentStop hooks, context windows)
3. Published research (Chroma context rot, NoLiMa benchmark, RAG false positives)

### Gaps to Address

**Transcript parsing for usage detection:** The exact JSONL schema for subagent transcripts (how Read tool calls appear) needs validation during Phase 3 implementation. Inferred from Claude Code's general transcript format but not verified against actual subagent transcript file. **Handle:** Early validation in Phase 3 — spawn test subagent, inspect transcript structure before building validator.

**Score threshold tuning (MUST-READ vs SHOULD-READ):** Starting at score >= 8 for MUST-READ classification. May need adjustment after observing real discovery results across multiple plugins. **Handle:** Monitor discovery precision in Phase 1 testing. Tune threshold based on false positive/negative rates.

**Agent behavior regression risk:** Injecting research content may subtly change agent output in unexpected ways (different code patterns, shifted attention from contracts). **Handle:** A/B testing in Phase 2 with completed plugins (O-Bells, O-Freeze). Compare output with/without injection. Reject injection format if quality degrades.

**Maximum resources per agent:** Starting with 5-resource limit via `--limit` flag. May need per-agent tuning (dsp-agent might benefit from more, foundation-shell-agent from fewer). **Handle:** Monitor token usage and agent feedback in Phase 2. Adjust limits per-agent if needed.

**Cross-plugin research visibility:** Should discovery consider research from other plugins' `.planning/research/` folders? Currently scoped to shared `research/` only. **Handle:** Defer to v1.3+ after validating shared research orchestration works. Per-plugin research is already loaded via contract injection.

## Sources

### Primary (HIGH confidence)
- [Claude Code Hooks Reference](https://code.claude.com/docs/en/hooks) — Official documentation for all hook events, SubagentStart additionalContext, SubagentStop agent_transcript_path
- [Claude Code Subagents Documentation](https://code.claude.com/docs/en/sub-agents) — Official docs on subagent creation, frontmatter, skills preloading, lifecycle
- Direct codebase analysis — `.claude/hooks/*.sh`, `.claude/agents/*.md`, `.claude/schemas/`, `research/*.md` (measured 11 agents, 6 hooks, 23 docs)
- [Claude API: Context Windows](https://platform.claude.com/docs/en/build-with-claude/context-windows) — 200K token window documentation
- [Creek Service: Evolving JSON Schemas](https://www.creekservice.org/articles/2024/01/08/json-schema-evolution-part-1.html) — `additionalProperties: false` evolution rules

### Secondary (MEDIUM confidence)
- [Chroma Research: Context Rot](https://research.trychroma.com/context-rot) — 18 LLMs measured, performance degrades with input length
- [NoLiMa Benchmark / Towards Data Science](https://towardsdatascience.com/your-1m-context-window-llm-is-less-powerful-than-you-think/) — 11/12 models below 50% at 32K tokens
- [InfoQ: Reducing RAG False Positives](https://www.infoq.com/articles/reducing-false-positives-retrieval-augmented-generation/) — Banking case study showing keyword ambiguity in narrow domains
- [DEV Community: Guaranteed Context Injection](https://dev.to/sasha_podles/claude-code-using-hooks-for-guaranteed-context-injection-2jg) — Skills skipped 56% of time (Vercel research), hooks are guaranteed
- [Claude Code Context Optimization](https://gist.github.com/johnlindquist/849b813e76039a908d962b2f0923dc9a) — 54% context reduction via trigger-based routing
- [bm25s Library](https://github.com/xhluca/bm25s) — Lightweight Python BM25 implementation (verified library capabilities)
- [Anthropic: Building Effective Agents](https://www.anthropic.com/research/building-effective-agents) — Tool management (10-20 tools maximum), progressive disclosure, cache strategically

### Tertiary (LOW confidence, patterns only)
- [Augment Code: Why Multi-Agent LLM Systems Fail](https://www.augmentcode.com/guides/why-multi-agent-llm-systems-fail-and-how-to-fix-them) — Specification ambiguity 41.77%, coordination failures 36.94%, verification gaps 21.30%
- [Composio: The 2025 AI Agent Report](https://composio.dev/blog/why-ai-agent-pilots-fail-2026-integration-roadmap) — Cost explosion, "many teams only notice pitfalls when the bill arrives"
- [WolfSound: Don't Use AI for Audio Programming](https://thewolfsound.com/dont-use-ai-for-audio-programming/) — "Not enough training data for real-time-safe audio DSP"
- Various community patterns on multi-agent orchestration, RAG retrieval, and context engineering (used for pattern validation, not specific claims)

---
*Research completed: 2026-02-04*
*Ready for roadmap: yes*
