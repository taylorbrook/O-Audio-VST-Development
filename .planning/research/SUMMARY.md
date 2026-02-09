# Project Research Summary

**Project:** Plugin Freedom System v1.3 System Modernization
**Domain:** AI agent orchestration for audio plugin development
**Researched:** 2026-02-08
**Confidence:** HIGH

## Executive Summary

The Plugin Freedom System v1.3 modernization is a targeted integration of three capability upgrades into the proven Stage 0-4 workflow that has successfully delivered 35+ production plugins. Opus 4.6 brings adaptive thinking (GA), effort tuning, 1M context (beta), persistent memory, and Agent Teams (experimental). GSD 1.18.0 provides `gsd-tools` CLI for mechanical state operations, verification suite for structural checks, and auto mode for express workflows. The core research finding: this is NOT a rewrite — it is a surgical deduplication of PFS custom code that GSD now handles natively, plus selective adoption of Opus 4.6 capabilities where they solve existing pain points.

The recommended approach prioritizes (1) preventing breakage from deprecations, (2) reducing custom code maintenance burden, (3) improving agent quality through better model assignments and context persistence. Adaptive thinking migration (T1) and prefill removal audit (T6) prevent future breakage. GSD verification suite adoption (T5) reduces 12 custom validators to 6. Domain-aware compaction (D6) replaces the fragile PreCompact.sh cat-all-files approach with server-side intelligence. Effort parameter adoption (D1) eliminates binary Sonnet/Opus switching for fine-grained cost-quality control. Agent Teams (D2-D5) remain experimental and applicable only for genuinely parallel independent work — the sequential stage pipeline stays primary.

Key risks: (1) removing PFS-specific custom code that appears duplicate but encodes domain expertise (real-time safety, contract immutability, JUCE quirks), (2) breaking JSON Schema contracts that 11 agents depend on, (3) adopting Agent Teams for inherently sequential work with file conflicts, (4) scope creep beyond the 62-requirement modernization into architectural rewrites. Mitigation: exhaustive classification of all custom code before removal, canary plugin testing after every agent change, parallel suitability matrix to prevent misuse of Agent Teams, strict timebox with must/should/could/won't prioritization.

## Key Findings

### Platform Capabilities (Opus 4.6 + GSD 1.18.0)

**Opus 4.6 adds:**
- **Adaptive thinking (GA):** Replaces deprecated `thinking: {type: "enabled"}` + `budget_tokens`. Migration required before future model release breaks explicit thinking config.
- **Effort parameter (GA):** Four levels (low/medium/high/max) for fine-grained cost-quality tuning. Eliminates binary model selection logic.
- **1M context window (beta):** Premium pricing ($10/$37.50 per MTok vs $5/$25). Use selectively for complexity >= 4 DSP agents with cross-reference research needs.
- **Persistent memory:** `memory: project` in agent YAML creates `.claude/agent-memory/{agent}/MEMORY.md` for cross-session learning.
- **Agent Teams (experimental):** Parallel agent coordination with debate. Best for read-heavy independent work. NOT viable for sequential stages or file conflicts. 3-5x token cost.
- **Compaction API (beta):** `instructions` parameter for domain-aware summarization. `pause_after_compaction` for contract injection.
- **Prefill removal:** Returns 400 error if assistant messages are pre-filled. Audit all skills for format control prefills.

**GSD 1.18.0 adds:**
- **gsd-tools CLI:** 10+ commands for state operations (phase add/complete, milestone complete, progress, validate consistency). Replaces manual markdown parsing.
- **Frontmatter operations:** get/set/merge/validate for YAML manipulation. Could replace custom Python validators if schema support matches.
- **Verification suite:** `verify plan-structure`, `verify phase-completeness`, `verify references`, `verify commits`, `verify artifacts`, `verify key-links`. Covers structural concerns PFS custom validators handle.
- **Auto mode:** `--auto` flag runs research -> requirements -> roadmap without human intervention. Better than PFS's `--no-plan` which skips planning entirely.
- **Template variants:** Minimal/standard/complex summaries based on task complexity.
- **Configurable branching:** none/phase/milestone branches with optional squash merge.

### Expected Modernizations (Table Stakes vs Differentiators)

**Table Stakes (T1-T7) — prevent breakage, reduce maintenance:**
1. **T1: Adaptive thinking migration** — All agents using `thinking: {type: "enabled"}` or `budget_tokens` migrate to `thinking: {type: "adaptive"}` with effort parameter
2. **T2: PreCompact path correction** — Update legacy `.ideas/` and `.continue-here.md` paths to `.planning/` structure
3. **T3: gsd-tools CLI adoption** — Replace custom state management for structural operations, keep plugin-specific state (PLUGINS.md, per-plugin STATUS.md)
4. **T4: Frontmatter operations** — Adopt GSD frontmatter get/set if custom schema validation supported, else keep Python validator
5. **T5: GSD verification suite** — Replace 6 structural validators with GSD equivalents, keep 6 domain validators (DSP real-time safety, APVTS matching, WebView bindings, checksums, cross-contract, resource accountability)
6. **T6: Prefill removal audit** — Verify no skills prefill assistant messages for format control
7. **T7: Context compliance** — Add post-plan validation that PLAN.md does not contradict CONTEXT.md user decisions

**Differentiators — Agent Quality (D1-D5):**
- **D1: Effort parameter** — Replace Sonnet/Opus binary switch with Opus at varying effort: foundation-shell (medium), simple DSP (high), complex DSP (max), research (high)
- **D2: Agent Teams for research** — Parallel researchers (DSP algorithms, UI patterns, module reuse) share findings and debate. Read-heavy, no file conflicts. Prove pattern works before expanding.
- **D3: Agent Teams for review** — Critics (DSP, UI, architecture) review in parallel with debate. Read-only validation. Depends on D2 succeeding first.
- **D4: Plan approval gates** — Agent Teams lead reviews teammate plans before implementation. Catches architectural mistakes pre-code.
- **D5: Delegate mode** — Restrict orchestrator to coordination-only tools, no implementation. Press Shift+Tab to enable.

**Differentiators — Context Persistence (D6-D9):**
- **D6: Domain-aware compaction** — Use Compaction API `instructions` parameter: "Preserve plugin name, stage/phase, parameter IDs, DSP components, contract paths, architecture decisions. Discard intermediate file reads, build output, exploration."
- **D7: 1M context for complex DSP** — Load full research documents for complexity >= 4 plugins instead of 200-word summaries. 2x cost, use selectively.
- **D8: History digest** — GSD's `history-digest` compiles cross-stage memory: "Stage 0 chose FDN reverb, Stage 1 implemented 13 parameters, Stage 2 added smoothing."
- **D9: Compaction pause** — Inject STATUS.md, parameter IDs, contract paths after compaction but before response continues. Belt-and-suspenders with D6.

**Differentiators — Workflow (D10-D13):**
- **D10: GSD auto mode** — Map `/implement --auto` to auto-discuss -> auto-research -> auto-plan -> execute. Better than skip-plan.
- **D11: TaskCompleted hooks** — Per-task validation within a plan. Exit code 2 prevents completion with feedback. Wire PFS domain validators here.
- **D12: Configurable branching** — Per-stage branches isolate work. Squash merge at plugin completion.
- **D13: Template variants** — Simple plugins get concise summaries, complex plugins get detailed summaries.

**Anti-Features (explicitly avoid):**
- Agent Teams for implementation stages (sequential dependencies, file conflicts, 3-5x cost)
- Replacing all PFS validators with GSD (domain expertise lost)
- 1M context for all agents (2x cost, wasted context)
- Auto-parallelizing serial pipeline (breaks data dependencies)
- Nested Agent Teams (unsupported)
- Removing PreCompact entirely (safety net needed)
- Custom monitoring dashboard (Claude Code has native monitoring)
- Fast mode for all agents (6x cost for negligible speed gain)

### Architecture Approach

The v1.3 modernization preserves the Stage 0-4 pipeline with discuss-research-plan-execute-verify workflow. 11 custom PFS agents continue handling domain-specific orchestration. GSD handles mechanical state operations deterministically. Agent Teams are opt-in for genuinely parallel work only.

**Component boundaries remain stable:**
- `.claude/commands/` — user-facing CLI, no changes
- `.claude/agents/` — 11 agent definitions, update model + memory
- `.planning/` — project state, preserve format
- `plugins/*/` — plugin source code, no changes
- `.claude/hooks/` — 6 lifecycle hooks, leverage gsd-tools
- `.claude/scripts/` — evaluate gsd-tools replacement case-by-case
- `.claude/schemas/` — preserve contracts, extend with optional fields only

**Integration points:**
1. **Model assignments:** dsp-agent always-opus, research-planning-agent always-opus, music-theory-agent downgrade to haiku, foundation-shell/gui/ui-finalization stay sonnet
2. **Persistent memory:** troubleshoot-agent, dsp-agent, gui-agent, research-planning-agent, validation-agent get `memory: project`
3. **gsd-tools CLI:** Replace manual STATE.md parsing in agents, keep PFS plugin-specific state
4. **Domain-aware compaction:** Custom `instructions` parameter injecting contract preservation rules
5. **Agent Teams (optional):** Research phase only initially, expand to review after validation

**Anti-patterns to avoid:**
- Replacing sequential pipeline with Agent Teams (strict dependencies exist)
- Universal persistent memory (template execution agents don't learn)
- Removing PFS orchestration for GSD orchestration (domain expertise lost)
- Breaking JSON Schema contracts (all 11 agents emit conforming reports)
- Eager migration of all state updates (plugin-specific state != GSD state)

### Critical Pitfalls

**P34: Removing PFS-specific custom code without classification**
All custom code must be classified as duplicate (GSD handles it), extension (PFS-specific but could use GSD primitives), or workaround (compensating for gap). The 12 validators split 6/6 structural vs domain. Removing domain validators (real-time safety, contract immutability, silent failure detection) loses JUCE expertise. **Avoid:** Exhaustive classification with test cases before removal.

**P35: Breaking agent contracts via required schema fields**
All 11 agents emit `subagent-report.json` and `validator-report.json` conforming to schemas with `additionalProperties: false`. Adding required fields breaks all agents simultaneously. **Avoid:** New fields are always optional with defaults. Update schemas FIRST, then agents can use new features.

**P36: Agent Teams for inherently sequential work**
Implementation stages (foundation, DSP, GUI) modify overlapping files (PluginProcessor.cpp, PluginEditor.cpp) and have strict dependencies (DSP before GUI). Agent Teams require file ownership separation and parallel independent work. **Avoid:** Parallel suitability matrix classifying all workflows. Use Agent Teams exclusively for research (read-heavy) and review (read-only).

**P37: Terminology confusion (subagent vs agent team vs skill vs command)**
Opus 4.6 docs use "subagent" (single AI invoked via Task tool, can nest) and "agent team" (multiple AIs with coordinator, cannot nest). PFS uses "agent" for both. GSD uses "skill" for reusable workflows. **Avoid:** Create terminology mapping document referenced in all plans.

**P38: Relaxing token budgets assuming 1M context prevents compaction**
Auto-compaction fires at ~33K tokens regardless of context window size. Relaxing injection budgets from 4K to 20K increases compaction frequency and cost. **Avoid:** Measure compaction events before/after. Keep 4K budget with smarter injection via D6.

**P39: GSD update silently breaking PFS integration**
GSD 1.19+ could change gsd-tools CLI signatures, template paths, or config schema. PFS hardcodes paths like `~/.claude/get-shit-done/bin/gsd-tools.js`. **Avoid:** Document all GSD integration points. Use config/environment variables. Test against GSD updates in isolation.

**P40: Plugin build regression from modernization changes**
Any change to agents, schemas, or orchestration risks breaking the proven workflow. 35 production plugins depend on current behavior. **Avoid:** Canary plugin (simple effect, fast build) passes after EVERY change before committing.

**P41: Persistent memory without hygiene**
`memory: project` accumulates data indefinitely. Foundation-shell-agent (template execution) doesn't learn meaningfully — memory accumulates noise. **Avoid:** Memory only for agents demonstrating learning (troubleshooting patterns, API quirks, architectural decisions). Consider curation at 100 plugins.

**P42: State file format drift between PFS and GSD**
gsd-tools manages `.planning/STATE.md` with GSD-specific schema. PFS has `PLUGINS.md` and per-plugin `STATUS.md` with different schema. Blindly adopting gsd-tools for all state loses plugin tracking. **Avoid:** Adopt gsd-tools for GSD state (project STATE.md, phase progression). Keep PFS state management for plugin-specific files.

**P43: Scope creep from 62 requirements to architectural rewrites**
Feature landscape has 7 table stakes, 13 differentiators. Easy to expand into Agent Teams for all workflows, replacing all validators, 1M context everywhere. **Avoid:** Re-read MVP recommendation. Timebox to 2 days. Must/should/could/won't prioritization enforced.

## Implications for Roadmap

Based on research, suggested four-phase structure prioritizing (1) prevent breakage, (2) reduce maintenance, (3) improve quality, (4) experimental features.

### Phase 1: Platform Alignment (prevent breakage, reduce maintenance)

**Rationale:** Address deprecations and broken hooks before adding new capabilities. All low-effort, high-value changes.

**Delivers:**
- All agents migrated to adaptive thinking (T1)
- Prefill removal audit complete (T6)
- PreCompact.sh paths updated to `.planning/` structure (T2)
- Effort parameter adopted for all agents (D1)

**Addresses:**
- Table stakes T1, T2, T6
- Differentiator D1

**Avoids:**
- P40 (build regression) — via canary testing
- Future model breakage from deprecated thinking config

**Research flag:** SKIP — straightforward migration, official docs comprehensive

---

### Phase 2: GSD Deduplication (remove custom code GSD handles)

**Rationale:** Reduce PFS maintenance burden by adopting GSD equivalents for structural operations. Keep domain-specific code.

**Delivers:**
- 6 structural validators replaced with GSD verification suite (T5)
- gsd-tools CLI adopted for state operations (T3)
- Context compliance verification added (T7)
- Frontmatter operations evaluated and adopted if applicable (T4)

**Uses:**
- GSD 1.18.0 `gsd-tools` CLI
- GSD verification suite commands
- GSD frontmatter operations

**Implements:**
- Integration contract documenting all GSD dependencies
- Smoke tests validating PFS + GSD integration

**Addresses:**
- Table stakes T3, T4, T5, T7

**Avoids:**
- P34 (removing domain code) — via exhaustive classification with test cases
- P39 (GSD update breakage) — via integration contract and smoke tests
- P42 (state format drift) — via clear ownership (GSD state vs PFS state)

**Research flag:** MEDIUM DEPTH — needs audit of 12 validators to classify structural vs domain, mapping gsd-tools commands to PFS state operations

---

### Phase 3: Context Persistence (address information loss pain point)

**Rationale:** Directly addresses user pain point #2 (context lost between sessions). D6 is highest-value individual feature.

**Delivers:**
- Domain-aware compaction via custom instructions (D6)
- Compaction pause for contract preservation (D9)
- History digest for cross-stage memory (D8)
- Auto mode for express plugins (D10)

**Uses:**
- Opus 4.6 Compaction API (beta)
- GSD 1.13 history digest

**Implements:**
- Custom compaction instructions preserving contracts, parameters, decisions
- Per-plugin history digest compiling Stage 0-4 decisions
- Express mode that auto-generates plans instead of skipping them

**Addresses:**
- Differentiators D6, D8, D9, D10

**Avoids:**
- P38 (context overconfidence) — measure compaction events before/after, keep 4K budget
- P41 (memory pollution) — defer persistent memory to Phase 4 evaluation

**Research flag:** LOW — Compaction API well-documented, history digest straightforward, auto mode maps to existing express workflow

---

### Phase 4: Agent Teams (experimental, address serial execution pain)

**Rationale:** Highest impact but highest risk. Prove with research (read-only, no file conflicts) before expanding.

**Delivers:**
- Agent Teams for parallel research (D2) — DSP + UI + module audit researchers in parallel
- Agent Teams for cross-stage review (D3) — DSP critic + UI critic + architecture critic with debate
- Plan approval gates (D4) and delegate mode (D5) if Agent Teams succeed
- TaskCompleted hooks for fine-grained quality gates (D11)

**Uses:**
- Opus 4.6 Agent Teams (experimental)
- Claude Code hooks (TaskCompleted)

**Implements:**
- Parallel suitability matrix classifying all PFS workflows
- Terminology mapping (subagent vs agent team vs skill)
- Agent Teams opt-in mode (not default)

**Addresses:**
- Differentiators D2, D3, D4, D5, D11

**Avoids:**
- P36 (Agent Teams for serial work) — via parallel suitability matrix, Stage 1-4 pipeline marked serial-only
- P37 (terminology confusion) — via terminology mapping document
- P43 (scope creep) — Agent Teams research only, do NOT expand to implementation unless validated

**Research flag:** HIGH DEPTH — experimental feature with known limitations (no session resumption, file conflicts). Needs proof-of-concept with canary plugin research phase before production use.

---

### Phase Ordering Rationale

1. **Phase 1 first:** Prevents future breakage from deprecations. Low-effort, immediate value. Establishes canary testing discipline.
2. **Phase 2 second:** Reduces maintenance burden before adding new capabilities. Creates integration contract that later phases depend on.
3. **Phase 3 third:** Addresses highest-value pain point (context loss) after platform stabilization. D6 domain-aware compaction depends on T2 PreCompact fix.
4. **Phase 4 last:** Experimental feature with highest token cost and coordination complexity. Only proceed if Phase 3 reveals need for parallel research.

**Dependencies:**
- Phase 2 depends on Phase 1 (agents must use correct model config before gsd-tools integration)
- Phase 3 depends on Phase 1 (T2 PreCompact fix) and Phase 2 (state operations aligned)
- Phase 4 depends on Phases 1-3 (agent config stable, state operations working, terminology mapping exists)

### Research Flags

**Needs deeper research during planning:**
- **Phase 2 (GSD Deduplication):** Medium depth — audit 12 validators for structural vs domain classification, map gsd-tools commands to PFS state operations, verify GSD frontmatter schema supports PFS research document 10-field schema
- **Phase 4 (Agent Teams):** High depth — experimental feature, needs proof-of-concept with canary plugin research phase, measure actual token cost vs quality improvement, document file ownership strategy

**Standard patterns (skip research):**
- **Phase 1 (Platform Alignment):** Official Opus 4.6 docs comprehensive, migration path clear
- **Phase 3 (Context Persistence):** Compaction API well-documented, history digest straightforward GSD feature

### Deferred Features (not in v1.3 scope)

- **D7 (1M context):** Beta, premium pricing, evaluate after GA
- **D12 (Branching):** Nice-to-have, not blocking any pain point
- **D13 (Template variants):** Low impact, adopt opportunistically

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Table Stakes (T1-T7) | **HIGH** | Verified against official Opus 4.6 docs (deprecation notices, breaking changes), GSD changelog (v1.10-1.18 features), and PFS codebase analysis (PreCompact.sh legacy paths, 12 validators inventoried) |
| Effort Parameter (D1) | **HIGH** | GA feature, no beta header, documented with four levels in official API docs |
| Agent Teams (D2-D5) | **MEDIUM** | Experimental feature with known limitations (no session resumption, no nested teams, file conflicts documented). Feature details verified against official Claude Code docs. Cost and coordination overhead are real risks. |
| Compaction (D6, D9) | **HIGH** | Beta but comprehensive API documentation with code examples. Custom `instructions` parameter well-documented. PFS use case straightforward. |
| GSD Integration (D8, D10, D13) | **MEDIUM** | GSD changelog confirms features exist. Integration with PFS's plugin-centric model needs design work — GSD is project-centric. State file schema differences require careful mapping. |
| 1M Context (D7) | **LOW** | Still in beta. Premium pricing confirmed ($10/$37.50 per MTok). Real-world performance for DSP agents not validated. Deferred to post-v1.3. |

**Overall confidence:** **HIGH**

### Gaps to Address

**PFS vs GSD state schema mapping:**
GSD manages project-centric state (`.planning/STATE.md`). PFS has plugin-centric state (`PLUGINS.md`, per-plugin `plugins/*/STATUS.md`). The relationship is not 1:1. Needs explicit ownership document during Phase 2: GSD owns project STATE.md, PFS owns plugin-specific files. Migration strategy if convergence desired later.

**Agent Teams file ownership strategy:**
Agent Teams require clear file ownership to prevent conflicts. Research phase is read-only (safe). Review phase is read-only (safe). If expanding beyond research/review, need explicit directory-based ownership model: teammate A owns `plugins/PluginA/`, teammate B owns `plugins/PluginB/`.

**Persistent memory hygiene at scale:**
At 100 plugins, 5 agents with persistent memory = 5 memory files accumulating data. No curation strategy defined. Needs evaluation during Phase 3 (context persistence): compare value of persistent memory vs existing resource discovery system. If persistent memory adopted, define hygiene rules (max size, auto-summarization, manual curation checkpoints).

**GSD frontmatter schema compatibility:**
PFS research documents use custom 10-field YAML schema validated by `validate-research-frontmatter.py`. GSD 1.17 added frontmatter operations. Unknown if GSD supports custom schemas or only GSD's own schema format. Needs evaluation during Phase 2 to determine if custom validator can be replaced.

**Compaction instruction effectiveness:**
Custom compaction instructions are best-effort — the model interprets them. Unknown if "preserve all parameter IDs" actually prevents parameter loss during compaction. Needs measurement during Phase 3: manually verify post-compaction context includes all contract data, compare D6 (instructions) vs D9 (pause) effectiveness.

## Sources

### Primary (HIGH confidence)
- [Anthropic: Introducing Claude Opus 4.6](https://www.anthropic.com/news/claude-opus-4-6) — agent teams overview, capability summary
- [Claude API Docs: What's new in Claude 4.6](https://platform.claude.com/docs/en/about-claude/models/whats-new-claude-4-6) — adaptive thinking deprecation, effort GA, prefill removal, output_config migration
- [Claude Code Docs: Agent Teams](https://code.claude.com/docs/en/agent-teams) — architecture, task coordination, file ownership, delegate mode, plan approval, hooks, limitations
- [Claude Code Docs: Create custom subagents](https://code.claude.com/docs/en/sub-agents) — subagent configuration including memory, skills, hooks, permissionMode
- [Claude API Docs: Compaction](https://platform.claude.com/docs/en/build-with-claude/compaction) — API parameters, custom instructions, pause_after_compaction, token budget enforcement
- [Claude Code Docs: Hooks Reference](https://code.claude.com/docs/en/hooks) — TaskCompleted, TeammateIdle hook specs
- [GSD Changelog](https://github.com/glittercowboy/get-shit-done/blob/main/CHANGELOG.md) — v1.10-1.18 features: gsd-tools CLI, verification suite, frontmatter ops, history digest, auto mode, branching

### Secondary (MEDIUM confidence)
- [GSD Releases](https://github.com/glittercowboy/get-shit-done/releases) — release-level feature summaries
- [Opus 4.6 vs 4.5 Comparison (ssntpl.com)](https://ssntpl.com/blog-claude-opus-4-6-vs-4-5-benchmarks-testing/) — benchmark comparisons, context window differences
- [TechCrunch: Opus 4.6 Agent Teams](https://techcrunch.com/2026/02/05/anthropic-releases-opus-4-6-with-new-agent-teams/) — feature overview
- [VentureBeat: Claude Code Tasks + Agent Coordination](https://venturebeat.com/orchestration/claude-codes-tasks-update-lets-agents-work-longer-and-coordinate-across/) — TaskCompleted/TeammateIdle hooks announcement
- [Vivek Haldar: Subagents, Commands and Skills Are Converging](https://www.vivekhaldar.com/articles/claude-code-subagents-commands-skills-converging/) — terminology convergence analysis

### Codebase Analysis (HIGH confidence — direct inspection)
- `.claude/hooks/PreCompact.sh` — confirmed legacy `.ideas/` paths, `.continue-here.md` references
- `.claude/hooks/SubagentStop.sh` — 12 validators inventoried, domain vs structural classification
- `.claude/hooks/hooks.json` — 6 hook event types registered
- `.claude/agents/dsp-agent.md` — model selection logic, budget_tokens usage pattern
- `.claude/skills/plugin-workflow/skill.md` — orchestrator pattern, serial phase execution
- `.claude/skills/session-checkpoint/skill.md` — per-task checkpoints, no cross-stage digest
- `.claude/skills/context-resume/skill.md` — STATUS.md-based resume, no history aggregation
- All 11 agent definitions, 44 command files, 6 hooks, 27 research docs analyzed

---
*Research completed: 2026-02-08*
*Ready for roadmap: yes*
