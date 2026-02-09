# Feature Landscape: v1.3 System Modernization

**Domain:** Plugin Freedom System Modernization (Opus 4.6 + GSD 1.18.0 Alignment)
**Researched:** 2026-02-08
**Mode:** Ecosystem research -- mapping Opus 4.6 and GSD 1.18.0 capabilities to PFS improvements
**Supersedes:** v1.2 Feature Research (2026-02-04)

## Table Stakes

Features the system needs to stay current with the platform. Without these, PFS accumulates technical debt against Opus 4.6 deprecations and misses GSD improvements that reduce custom code maintenance.

| # | Feature | Why Expected | Complexity | Tied To | Notes |
|---|---------|--------------|------------|---------|-------|
| T1 | Adaptive thinking migration | Opus 4.6 deprecates `thinking: {type: "enabled"}` + `budget_tokens`. These remain functional but will break in a future model release. All PFS agents using explicit thinking config must migrate. | Low | Opus 4.6 deprecation notice | Agent frontmatter currently specifies `model: sonnet` or delegates to orchestrator for Opus selection with budget_tokens. Migrate to `thinking: {type: "adaptive"}` with effort parameter. The effort parameter is GA (no beta header). Adaptive thinking automatically enables interleaved thinking -- remove `interleaved-thinking-2025-05-14` beta headers if present. |
| T2 | PreCompact hook path correction | Current `PreCompact.sh` references legacy paths: `.ideas/creative-brief.md`, `.ideas/parameter-spec.md`, `.ideas/architecture.md`, `.ideas/plan.md`, `.continue-here.md`. These paths no longer exist -- contracts live under `plugins/{name}/.planning/`. The hook silently does nothing for active plugins. | Low | Opus 4.6 compaction | Update all paths to `.planning/` structure. Also update to reference `STATUS.md` instead of `.continue-here.md`. Compaction is now built into Claude Code (auto-compact at ~33K tokens), so this hook should focus on injecting domain-critical data that generic compaction might lose -- contract paths, parameter IDs, DSP component names. |
| T3 | GSD gsd-tools CLI adoption for mechanical state operations | PFS has `plugin-registry.py` (custom), `workflow-reconciliation` (custom skill with 5-phase protocol), and manual STATUS.md YAML editing across multiple skills. GSD 1.16-1.17 added 10+ CLI commands for state operations: `phase add/insert/remove/complete`, `milestone complete`, `validate consistency`, `progress`, `state advance-plan`, `state update-progress`, `state record-metric`. | Med | GSD 1.16-1.17 | Audit overlap between PFS custom state management and gsd-tools commands. PFS's state model is plugin-centric (per-plugin STATUS.md + global registry.json + active-plugin.json), while GSD's is project-centric (single STATE.md + roadmap). The mapping is not 1:1 -- PFS needs plugin-level state that GSD does not model. Target: adopt gsd-tools for structural operations (phase tracking, progress), keep PFS custom code for plugin-specific state (per-plugin STATUS.md, registry.json). |
| T4 | GSD frontmatter operations adoption | PFS has `validate-research-frontmatter.py` (88 lines) for 10-field YAML validation. GSD 1.17 added `frontmatter get/set/merge/validate` with schema validation. | Low | GSD 1.17 | If GSD's frontmatter validate supports custom schemas, migrate the 10-field research document schema to GSD format and replace the Python validator. If GSD only validates its own schema format, keep the custom validator but use GSD frontmatter get/set for read/write operations in scripts that currently parse YAML manually. |
| T5 | GSD verification suite for structural checks | PFS has 12 custom Python validators. Six handle structural concerns that GSD 1.17 now covers: `verify plan-structure`, `verify phase-completeness`, `verify references`, `verify commits`, `verify artifacts`, `verify key-links`. Six handle domain-specific concerns GSD cannot know about: `validate-dsp-components.py`, `validate-parameters.py`, `validate-gui-bindings.py`, `validate-checksums.py`, `validate-cross-contract.py`, `validate-resource-accountability.py`. | Med | GSD 1.17 verification suite | Replace structural validators with GSD equivalents. The six domain validators encode audio plugin expertise (real-time safety, APVTS parameter matching, WebView binding consistency) that must remain custom. Net result: maintain 6 custom validators instead of 12, with GSD handling the generic structural validation. |
| T6 | Prefill removal compatibility | Opus 4.6 removes support for prefilling assistant messages (returns 400 error). If any PFS skill or agent pre-fills assistant responses for format control, it will break. | Low | Opus 4.6 breaking change | Audit all skills and agent invocations for assistant message prefills. Alternatives: use `output_config.format` for structured outputs (replaces `output_format` which is also deprecated), or system prompt instructions for response style. The JSON report format agents use should work via system prompts rather than prefills. |
| T7 | Context compliance verification | GSD 1.11.1 added verification that plans do not contradict user decisions from CONTEXT.md. PFS's plan phase generates PLAN.md from CONTEXT.md + RESEARCH.md but does not verify consistency. A plan could specify an approach the user explicitly rejected in the discuss phase. | Low | GSD 1.11.1 | Add a post-plan validation step that cross-references CONTEXT.md user decisions with PLAN.md tasks. Can adopt GSD's compliance verification directly or add as a SubagentStop check after the planner agent completes. |

## Differentiators

Features that leverage Opus 4.6 and GSD 1.18.0 for capabilities beyond catching up. These address the three pain points: agent quality, context loss, and serial execution.

### Agent Quality Improvements

| # | Feature | Value Proposition | Complexity | Tied To | Notes |
|---|---------|-------------------|------------|---------|-------|
| D1 | Effort parameter for per-agent cost-quality tuning | PFS currently uses binary model selection: Sonnet for complexity <= 3, Opus for complexity >= 4. The effort parameter (GA) adds four levels -- low, medium, high, max -- enabling fine-grained control. Replace binary Sonnet/Opus switch with single Opus model at varying effort levels. | Low | Opus 4.6 effort parameter | Foundation-shell-agent (template-following, well-constrained): effort "medium". DSP-agent on simple plugins: effort "high". DSP-agent on complex plugins: effort "max". Research agents: effort "high" (default). This simplifies agent configuration (remove model selection logic, remove complexity-based routing) while improving quality where it matters. "Max" effort is new and provides highest capability. |
| D2 | Agent Teams for parallel research exploration | Currently, discuss/research/plan phases run serially. With Agent Teams, PFS could spawn parallel researchers during Stage 0: one investigating DSP algorithms, one checking UI patterns for this plugin type, one auditing module reuse opportunities. They share findings and challenge each other. | Med | Opus 4.6 Agent Teams (experimental) | Best fit: the "competing hypotheses" pattern maps directly to DSP algorithm selection ("Should this reverb use FDN vs convolution vs algorithmic?"). Research is read-heavy with no file conflicts -- the ideal Agent Teams use case. PFS's `research-planning-agent` becomes the team lead. Token cost: 3-5x for the research phase, but first-pass quality of ARCHITECTURE.md improves significantly. Requires `CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS=1` in settings.json. |
| D3 | Agent Teams for cross-stage review with debate | After each implementation stage, spawn a review team: one teammate validates real-time safety, another checks parameter consistency, a third reviews architecture compliance. They actively challenge each other's findings before producing a consensus report. | Med | Opus 4.6 Agent Teams | Maps to "parallel code review" pattern. PFS already has critic agents (DSP critic, UI critic) that run serially. Making them Agent Teams teammates with debate creates adversarial review that catches issues serial review misses. Requires read-only file access for reviewers (no file conflicts). Depends on D2 succeeding first -- prove Agent Teams work for PFS before expanding scope. |
| D4 | Plan approval gates for Agent Team teammates | Agent Teams support requiring plan approval before teammates implement. The lead reviews and approves/rejects plans with feedback. Maps to PFS's generator-critic pattern -- critics review agent plans before implementation begins. | Low | Opus 4.6 Agent Teams plan approval | Currently PFS agents plan and implement in a single pass. Adding plan approval creates a review checkpoint between plan and execute within an agent invocation. Adds latency but catches architectural mistakes before code is written. Only applicable if Agent Teams (D2/D3) are adopted. |
| D5 | Delegate mode for orchestrator discipline | Agent Teams support "delegate mode" where the lead is restricted to coordination-only tools -- cannot implement, only spawn, message, and manage tasks. PFS's plugin-workflow orchestrator sometimes drifts into doing implementation work. | Low | Opus 4.6 Agent Teams delegate mode | Aligns with existing PFS design principle: "plugin-workflow orchestrates, subagents implement." Delegate mode enforces this at the tool level instead of relying on prompt instructions. Press Shift+Tab to enable. Only applicable if Agent Teams are adopted. |

### Context Persistence Improvements

| # | Feature | Value Proposition | Complexity | Tied To | Notes |
|---|---------|-------------------|------------|---------|-------|
| D6 | Domain-aware compaction via custom instructions | Use the Compaction API's `instructions` parameter to create plugin-development-aware summaries. Instead of generic summarization, instruct compaction to preserve: contract file paths, current stage/phase, all parameter IDs, DSP component specifications, architectural decisions, and module dependencies. | Low | Opus 4.6 Compaction API (beta) | The `instructions` parameter completely replaces the default summary prompt. PFS-specific prompt: "Preserve the plugin name, current implementation stage and phase, all APVTS parameter IDs and their types, DSP component names and processing chain order, contract file paths under .planning/, and any architectural decisions made in this session. Discard intermediate file reads, build output, and exploration that did not lead to decisions." This replaces the fragile PreCompact.sh cat-all-files approach with server-side intelligence that understands what matters. |
| D7 | 1M context window for complex DSP agents | Complex plugins (complexity >= 4) often need to consult multiple research documents, architecture specs, and reference implementations simultaneously. The 1M context window (beta) allows loading full references without exceeding limits. | Low | Opus 4.6 1M context (beta) | Currently PFS budgets 4K tokens for injected research context -- deliberately conservative. With 1M context, complex DSP agents could receive full research documents (reverb algorithms, filter design, modulation architectures) instead of 200-word summaries. Premium pricing: $10/$37.50 per MTok (2x standard). Use selectively: only for complexity >= 4 DSP/research agents. Default to 200K for all other agents. |
| D8 | GSD history digest for cross-stage memory | GSD 1.13 added `gsd-tools history-digest` that compiles phase summaries into structured JSON for fast context loading. PFS has per-task checkpoints (`session-checkpoint`) and STATUS.md state, but no aggregated history across stages for a given plugin. | Low | GSD 1.13 history digest | A per-plugin history digest would compile: "Stage 0 decisions (chose FDN reverb, 6-parameter design), Stage 1 outcomes (13 parameters implemented, WebView foundation set up), Stage 2 issues (had to add parameter smoothing to prevent zipper noise)." This gives agents in later stages context about earlier decisions without reading every SUMMARY.md and VERIFICATION.md from previous stages. Reduces token usage while improving cross-stage coherence. |
| D9 | Compaction pause for contract preservation | The Compaction API's `pause_after_compaction` parameter lets PFS inject critical content after compaction but before the response continues. Use this to ensure contracts are always in context even after compaction. | Low | Opus 4.6 Compaction API | When compaction fires, it returns `stop_reason: "compaction"`. PFS can then inject the current plugin's STATUS.md, parameter IDs, and contract paths before the API continues. Belt-and-suspenders with D6 (custom instructions). |

### Workflow Improvements

| # | Feature | Value Proposition | Complexity | Tied To | Notes |
|---|---------|-------------------|------------|---------|-------|
| D10 | GSD auto mode for express plugin creation | GSD 1.18 added `--auto` flag for `/gsd:new-project` that runs research -> requirements -> roadmap without human intervention. PFS's express mode (`--no-plan`) skips planning instead of automating it. The GSD approach is better: it produces plans without human intervention rather than proceeding without plans. | Med | GSD 1.18 auto mode | Map: `/implement --auto PluginName` -> auto-discuss (compile from BRIEF.md + ARCHITECTURE.md) -> auto-research (Context7 + existing research docs) -> auto-plan (generate from research) -> execute. The key insight from GSD: auto mode runs all phases automatically, it does not skip them. This produces better-quality implementation because agents always have plans, even in express mode. |
| D11 | TaskCompleted hooks for fine-grained stage gates | PFS's `SubagentStop.sh` validates at the subagent level (per stage). The new `TaskCompleted` hook fires when any task is marked complete, enabling per-task validation within a plan. Exit code 2 prevents completion and sends feedback. | Low | Claude Code 2.1.33+ | Could supplement SubagentStop.sh for finer-grained quality gates. Each task in a plan could have completion criteria enforced by hooks, not just the final subagent output. PFS's domain validators could be wired as TaskCompleted checks when running Agent Teams. Hook receives: task_id, task_subject, optionally task_description, teammate_name, team_name. |
| D12 | GSD configurable branching strategy | GSD 1.11.1 added three branching modes: `none` (current branch), `phase` (per-phase branches), `milestone` (per-milestone branches) with optional squash merge. PFS currently commits everything to the working branch. | Low | GSD 1.11.1 branching config | Per-stage branching would isolate Stage 1/2/3/4 work. If Stage 3 (GUI) fails badly, roll back without losing Stage 2 (DSP). Squash merge at plugin completion keeps history clean. Low risk: this is a configuration change, not architectural. |
| D13 | GSD template variants for summary complexity | GSD 1.13 added automatic template selection based on complexity: minimal (~30 lines), standard (~60 lines), complex (~100 lines). PFS generates uniform summaries regardless of task complexity. | Low | GSD 1.13 template variants | Simple plugins get concise summaries. Complex plugins (multi-band compressor, spectral analyzer) get detailed summaries. Reduces documentation overhead for simple tasks while maintaining thoroughness for complex ones. |

## Anti-Features

Features to explicitly NOT build during this modernization.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Agent Teams for implementation stages | Implementation stages (foundation, DSP, GUI) are inherently sequential and modify overlapping files (PluginProcessor.cpp, PluginEditor.cpp). Agent Teams require file ownership separation and are designed for parallel, independent work. Two teammates editing PluginProcessor.cpp leads to overwrites. Token cost is 3-5x with significant coordination overhead. | Keep subagents for implementation (proven pattern). Use Agent Teams exclusively for research (read-heavy, no file conflicts) and review (read-only analysis). |
| Replacing all PFS validators with GSD verify commands | GSD's verification suite handles structural concerns (plan format, phase completeness, commit history). PFS's domain validators encode audio plugin expertise: DSP real-time safety rules (no allocation in processBlock), APVTS parameter matching against spec, WebView HTML ID-to-relay binding consistency, contract checksum integrity. GSD has no awareness of these concerns and should not. | Replace 6 structural validators with GSD equivalents. Keep 6 domain validators. Net reduction: 12 custom validators to 6. |
| 1M context for all agents | Costs 2x per token ($10/$37.50 vs $5/$25 per MTok). Foundation-shell-agent follows templates and works well within 200K. Simple DSP agents need a single architecture doc. Most agents would waste money on context they do not need. | Use 1M context selectively: complexity >= 4 DSP agents and research phases requiring cross-reference of multiple full documents. Default to 200K for everything else. |
| Auto-parallelizing all workflow phases | Research depends on discuss output. Plan depends on research output. Execute depends on plan output. These are real data dependencies, not arbitrary serialization. Forcing parallelism would require speculative execution (research without context) or break contracts. | Parallelize only where truly independent: multiple research domains in parallel (DSP research + UI research + module audit). Keep the sequential pipeline for discuss -> research -> plan -> execute -> verify within each domain. |
| Nested Agent Teams | Agent Teams explicitly do not support nesting -- teammates cannot spawn their own teams. The lead is fixed for the team's lifetime. Attempting workarounds creates complexity with no official support and known limitations around session resumption. | Keep hierarchy flat: one team lead (orchestrator) with direct teammates. If a teammate needs sub-tasks, use subagents within the teammate session. |
| Removing PreCompact hook entirely | Even with D6 (domain-aware compaction instructions), the PreCompact hook serves as a safety net. Compaction instructions are best-effort -- the model interprets them. The hook provides deterministic data injection that compaction cannot lose. | Update PreCompact.sh to current paths (T2) and make it complement D6's custom instructions. Hook preserves exact contract paths and parameter lists; compaction preserves architectural understanding and session narrative. |
| Custom agent monitoring dashboard | Claude Code provides built-in teammate monitoring: in-process mode (Shift+Up/Down to cycle), split-pane mode (tmux for side-by-side view), and TeammateIdle hooks for automation. Building a custom UI adds significant scope, maintenance, and testing burden. | Use Claude Code's native monitoring. Add structured logging to agent JSON reports for post-hoc analysis if visibility is insufficient. |
| Fast mode (research preview) for all agents | Fast mode is 2.5x faster but costs 6x more ($30/$150 vs $5/$25 per MTok). For agents that run for seconds, the speed improvement is negligible relative to the cost increase. | Evaluate fast mode only for the longest-running agents (complex DSP implementation) if latency becomes a measurable pain point. Default to standard inference. |

## Feature Dependencies

```
T1 (Adaptive thinking) --> standalone, no dependencies
    |
    +--> D1 (Effort parameter) -- replaces current model selection, requires T1
    +--> D2 (Agent Teams research) -- agents need correct model config first
    +--> D7 (1M context) -- agent config must be updated first

T2 (PreCompact update) --> standalone
    |
    +--> D6 (Domain compaction) -- PreCompact fix is prerequisite

T3 (gsd-tools state) --> standalone
    |
    +--> D10 (Auto mode) -- state operations must be aligned first
    +--> D8 (History digest) -- state operations provide the data

T4 (Frontmatter ops) --> standalone
T5 (GSD verification) --> standalone
T6 (Prefill removal) --> standalone
T7 (Context compliance) --> standalone

D2 (Agent Teams research) --> requires T1
    |
    +--> D3 (Agent Teams review) -- prove Agent Teams work before expanding
    +--> D4 (Plan approval) -- requires Agent Teams
    +--> D5 (Delegate mode) -- requires Agent Teams
    +--> D11 (TaskCompleted) -- most useful with Agent Teams

D12 (Branching) --> standalone
D13 (Template variants) --> standalone
```

## MVP Recommendation

Prioritize by: (1) prevent breakage from deprecations, (2) reduce custom code, (3) improve quality.

### Phase 1 -- Platform Alignment (prevent breakage, reduce maintenance)

1. **T1: Adaptive thinking migration** -- prevents future model breakage, low effort
2. **T6: Prefill removal audit** -- prevents immediate 400 errors, low effort
3. **T2: PreCompact.sh path fix** -- fixes silent hook failure, low effort
4. **D1: Effort parameter adoption** -- replaces model selection logic, immediate quality improvement

Rationale: All low-effort, all address real problems (deprecation, broken hooks, cost).

### Phase 2 -- GSD Deduplication (remove custom code that GSD handles)

1. **T5: GSD verification adoption** -- 12 validators to 6, medium effort
2. **T3: gsd-tools state operations** -- reduce custom state management, medium effort
3. **T7: Context compliance verification** -- adopt GSD pattern, low effort
4. **T4: Frontmatter ops evaluation** -- assess during T5 work, low effort

Rationale: Medium effort with clear payoff -- less custom code to maintain, better alignment with GSD updates.

### Phase 3 -- Context Persistence (address "information lost between sessions")

1. **D6: Domain-aware compaction** -- high-value, low effort, replaces fragile PreCompact approach
2. **D8: History digest** -- cross-stage memory without manual context loading
3. **D9: Compaction pause for contracts** -- safety net for D6
4. **D10: Auto mode for express plugins** -- better express experience

Rationale: Directly addresses user pain point #2 (context loss). D6 is the highest-value individual feature.

### Phase 4 -- Agent Teams (address "serial execution" and "agent quality")

1. **D2: Agent Teams for parallel research** -- prove the pattern works for PFS
2. **D3: Agent Teams for cross-stage review** -- expand after D2 validation
3. **D4/D5: Plan approval + delegate mode** -- tighten orchestrator discipline
4. **D11: TaskCompleted hooks** -- fine-grained quality gates with Agent Teams

Rationale: Highest impact but highest risk (experimental feature, 3-5x token cost). Prove with research (read-only, no file conflicts) before expanding to review.

**Defer:**
- D7 (1M context): Beta, premium pricing, evaluate after GA
- D12 (Branching): Nice-to-have, not blocking any pain point
- D13 (Template variants): Low impact, adopt opportunistically

## Confidence Assessment

| Feature Category | Confidence | Rationale |
|-----------------|------------|-----------|
| Table Stakes (T1-T7) | HIGH | Verified against official Opus 4.6 docs (deprecation notices, breaking changes), GSD changelog (v1.10-1.18 features), and PFS codebase analysis (PreCompact.sh legacy paths, validator inventory) |
| Effort Parameter (D1) | HIGH | GA feature, no beta header, documented with four levels and clear semantics in official API docs |
| Agent Teams (D2-D5) | MEDIUM | Experimental feature with known limitations (no session resumption, no nested teams, file conflicts). Feature details verified against official Claude Code docs. Cost and coordination overhead are real risks. |
| Compaction (D6, D9) | HIGH | Beta but comprehensive API documentation with code examples. Custom `instructions` parameter is well-documented. PFS use case is straightforward. |
| GSD Integration (D8, D10, D13) | MEDIUM | GSD changelog confirms features exist. Integration with PFS's plugin-centric model needs design work -- GSD is project-centric. |
| 1M Context (D7) | LOW | Still in beta. Premium pricing confirmed ($10/$37.50 per MTok). Real-world performance for DSP agents not validated. |

## Sources

### Authoritative (HIGH confidence)
- [Anthropic: Introducing Claude Opus 4.6](https://www.anthropic.com/news/claude-opus-4-6) -- agent teams overview, capability summary
- [Claude API Docs: What's new in Claude 4.6](https://platform.claude.com/docs/en/about-claude/models/whats-new-claude-4-6) -- adaptive thinking deprecation, effort GA, prefill removal, output_config migration
- [Claude Code Docs: Agent Teams](https://code.claude.com/docs/en/agent-teams) -- architecture, task coordination, file ownership, delegate mode, plan approval, hooks, limitations
- [Claude API Docs: Compaction](https://platform.claude.com/docs/en/build-with-claude/compaction) -- API parameters, custom instructions, pause_after_compaction, token budget enforcement
- [Claude Code Docs: Hooks Reference](https://code.claude.com/docs/en/hooks) -- TaskCompleted, TeammateIdle hook specs

### Verified (MEDIUM confidence)
- [GSD Changelog](https://github.com/glittercowboy/get-shit-done/blob/main/CHANGELOG.md) -- v1.10-1.18 features: gsd-tools CLI, verification suite, frontmatter ops, history digest, auto mode, branching
- [GSD Releases](https://github.com/glittercowboy/get-shit-done/releases) -- release-level feature summaries
- [Opus 4.6 vs 4.5 Comparison (ssntpl.com)](https://ssntpl.com/blog-claude-opus-4-6-vs-4-5-benchmarks-testing/) -- benchmark comparisons, context window differences
- [TechCrunch: Opus 4.6 Agent Teams](https://techcrunch.com/2026/02/05/anthropic-releases-opus-4-6-with-new-agent-teams/) -- feature overview
- [VentureBeat: Claude Code Tasks + Agent Coordination](https://venturebeat.com/orchestration/claude-codes-tasks-update-lets-agents-work-longer-and-coordinate-across/) -- TaskCompleted/TeammateIdle hooks announcement

### PFS Codebase Analysis (direct inspection)
- `.claude/hooks/PreCompact.sh` -- confirmed legacy `.ideas/` paths, `.continue-here.md` references
- `.claude/hooks/SubagentStop.sh` -- 12 validators inventoried, domain vs structural classification
- `.claude/hooks/hooks.json` -- 6 hook event types registered
- `.claude/agents/dsp-agent.md` -- model selection logic, budget_tokens usage pattern
- `.claude/skills/plugin-workflow/skill.md` -- orchestrator pattern, serial phase execution
- `.claude/skills/session-checkpoint/skill.md` -- per-task checkpoints, no cross-stage digest
- `.claude/skills/context-resume/skill.md` -- STATUS.md-based resume, no history aggregation

---
*Feature research for: v1.3 System Modernization (Opus 4.6 + GSD Alignment)*
*Researched: 2026-02-08*
