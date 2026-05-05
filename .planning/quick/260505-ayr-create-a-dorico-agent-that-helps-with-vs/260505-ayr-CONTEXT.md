---
quick_id: 260505-ayr
description: Create a Dorico agent that helps with VST instrument implementation in Dorico — microtonal playback, playback templates, expression maps, etc.
gathered: 2026-05-05
status: Ready for planning
---

# Quick Task 260505-ayr: Dorico Agent — Context

<domain>
## Task Boundary

Create a specialized Claude Code subagent that assists with all aspects of integrating the Ouaricon VST instrument suite into Steinberg Dorico — covering microtonal/note-expression playback, Playback Templates, EndpointConfigs, Expression Maps, keyswitch routing, CC/Program Change technique triggers, and `.doricolib` distribution.

Deliverable: a single agent definition file at `.claude/agents/dorico-agent.md` (with optional supporting `.claude/agent-memory/dorico-agent.md`) plus a slash-command entry point so the agent can be user-invoked or spawned by other agents/skills.
</domain>

<decisions>
## Implementation Decisions

### Form factor
- **Subagent only.** Single `.claude/agents/dorico-agent.md` with YAML frontmatter, mirroring the `troubleshoot-agent` pattern.
- Spawned via the `Task` tool with `subagent_type: "dorico-agent"`.
- Tool surface: Read, Edit, Write, Bash, Grep, Glob, WebSearch, WebFetch, mcp__context7__*. (Same surface as troubleshoot-agent + write-capable, since capability decision is "diagnose + advise + edit".)

### Scope coverage
- **Full Dorico stack.** Agent must understand:
  - **Microtonal / note-expression playback** (VST3 NoteExpression, O-Lyrica spike pattern)
  - **Playback Templates** (.doricoplaybacktemplate / .doricolib distribution)
  - **EndpointConfigs** (`Resources/dorico/EndpointConfigs/<Plugin>/`)
  - **Expression Maps** (.doricoexpmap schema, technique → keyswitch / CC / PC mapping)
  - **Keyswitch routing** (the 3-layer stack: exp-map schema + plugin trigger defaults + fresh-instance reset — see `critical_dorico_keyswitch_routing.md`)
  - **CC / Program Change technique triggers** (recently added in O-MicrotonalSampler v1.15.0)
  - **Dynamics audit** (CC1 / CC11 / Velocity routing as audited in v1.15.0)
  - **Distribution mechanism** — Dorico needs a Playback Template, NOT a standalone .doricoexpmap drop (`critical_dorico_distribution_mechanism.md`).

### Trigger / invocation
- **Slash command** `/dorico [PluginName] [question-or-task]` is the primary user-facing entry point.
- The agent definition must also be reachable via `Task(subagent_type="dorico-agent", ...)` so other agents (troubleshoot-agent, gui-agent, dsp-agent, polish-agent) and skills (plugin-improve, plugin-publishing) can delegate Dorico-specific work.
- No filesystem hooks / auto-trigger. Keeps invocation explicit and predictable.

### Capabilities
- **Diagnose + advise + edit.** Agent can:
  - Read plugin source, .doricolib, .doricoexpmap, EndpointConfig XML, build artifacts, repo memory.
  - Edit Dorico artifacts (`.doricolib`, `.doricoexpmap`, EndpointConfig XML) AND plugin C++ source when keyswitch defaults / NoteExpression handling / technique triggers need code-side changes.
  - Run shell commands for build / install / Dorico cache reset when needed.
  - Spawn its own subagents (deep-research, troubleshoot-agent) only if explicitly delegated by the orchestrator — do NOT auto-fan-out by default.

### Claude's Discretion
- Exact YAML frontmatter shape (model, tools list, description heuristics) — match the `troubleshoot-agent.md` and `gui-agent.md` conventions.
- Whether the slash command lives at `.claude/commands/dorico.md` or as a `.claude/skills/dorico-helper/SKILL.md` thin wrapper — pick whichever matches existing repo convention for command-only entry points.
- Knowledge embedding strategy: prefer pulling from existing memory + reference files (`critical_dorico_*.md`, `project_o_lyrica_spike_reference.md`, O-MicrotonalSampler v1.16.x source as canonical reference) rather than duplicating large blocks of Dorico XML schema inline. The agent should *know where to look*, not *embed everything*.
- Whether to add `.claude/agent-memory/dorico-agent.md` from day one or defer until first run produces a notable lesson.

</decisions>

<specifics>
## Specific Ideas / References

- **Reference plugin (canonical Dorico integration):** `plugins/O-MicrotonalSampler` — recent commits implement the full Dorico stack:
  - `9bd0909 fix(O-MicrotonalSampler): v1.16.5 — Dorico Playback Template family routing via endpoint-config instrument enumeration`
  - `e8b6a2c fix(O-MicrotonalSampler): v1.16.2 — Dorico keyswitch-from-notation routing (TC-5)`
  - `5c823bc fix(O-MicrotonalSampler): v1.16.1 — Dorico launch-crash + DefaultLibraryAdditions distribution path`
  - `7e56e16 feat(O-MicrotonalSampler): v1.16.0 — Dorico distribution (EndpointConfig + PlaybackTemplate)`
  - `69208e2 feat(O-MicrotonalSampler): v1.15.0 — CC + Program Change technique triggers + Dynamics audit`
- **Reference plugin (note-expression / microtonal):** `plugins/O-Lyrica` — validated spike for VST3 NoteExpression microtonal playback. Per memory: "auval static-check finding (DEF-24-01) is benign, not a defect."
- **Existing Dorico distribution dir:** `plugins/O-MicrotonalSampler/Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib` — currently modified in working tree.
- **Memory references the agent must read on first invocation:**
  - `critical_dorico_distribution_mechanism.md`
  - `critical_dorico_keyswitch_routing.md`
  - `project_o_lyrica_spike_reference.md`
  - `MEMORY.md` (full index — agent should orient itself)
- **Existing similar agent patterns to emulate:**
  - `.claude/agents/troubleshoot-agent.md` (graduated research depth, uses mcp__context7__*)
  - `.claude/agents/gui-agent.md` (write-capable, edits source + CMake)
  - `.claude/agent-memory/<name>.md` convention for persistent agent learnings
- **Skill that should learn to delegate to dorico-agent:**
  - `plugin-improve` (when user reports Dorico-specific bugs)
  - `plugin-publishing` (Dorico-distribution validation before release)
  - `generalize-microtones` (already exists per skill list — propagating note-expression across pitched plugins)

</specifics>

<canonical_refs>
## Canonical References

External docs the agent must consult during investigation:
- Steinberg Dorico SDK / "Note Performer" expression-map authoring docs (web search at runtime)
- VST3 NoteExpression API (already covered in spike-findings-VST-development skill)
- Dorico `.doricolib` / Playback Template authoring guide (Steinberg help system)

In-repo references:
- `Skill("spike-findings-VST-development")` — auto-loaded; covers VST3 NoteExpression for Dorico microtonal playback
- `research/` directory — algorithm references and technical deep-dives (per CLAUDE.md, NOT `docs/`)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/` — auto-memory (`critical_dorico_*.md`, `project_o_lyrica_spike_reference.md`)

</canonical_refs>
