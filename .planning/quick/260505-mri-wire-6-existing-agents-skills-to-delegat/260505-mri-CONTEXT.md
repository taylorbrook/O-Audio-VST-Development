---
quick_id: 260505-mri
status: ready-for-planning
gathered: 2026-05-05
---

# Quick Task 260505-mri: Wire 6 agents/skills to dorico-agent — Context

<domain>
## Task Boundary

Wire 6 existing agents/skills to delegate to the new `dorico-agent` (created in quick task `260505-ayr`). Each target file gets a **short routing pointer / handoff subsection** added — no body rewrites.

**Targets (each = one atomic commit):**

1. `.claude/agents/troubleshoot-agent.md` — when symptom is Dorico-specific (microtonal regression, KS failure, `.doricolib` / `.doricoexpmap` / EndpointConfig issues, Playback Template), recommend handoff to `dorico-agent` instead of doing graduated web research itself.
2. `.claude/skills/plugin-improve/SKILL.md` — bug-classification step routes Dorico-tagged user reports to `dorico-agent`.
3. `.claude/skills/plugin-publishing/SKILL.md` — pre-release validation invokes `dorico-agent` for SMOKE-TEST.md TC-1..TC-5 battery on every microtonal-cohort plugin (O-Lyrica, O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant, O-MicrotonalSampler).
4. `.claude/commands/generalize-microtones.md` — Phase B (propagate to 7 plugins) delegates per-plugin Dorico bring-up to `dorico-agent` rather than re-deriving the pattern inline.
5. `.claude/agents/gui-agent.md` — Stage 3 KS-routing parameter cross-reference (Layer 2 of the 3-layer KS stack lives in `createParameterLayout()`).
6. `.claude/agents/dsp-agent.md` — note-expression tuning/trigger order callout (apply tuning to `currentFrequency` BEFORE `trigger()` call).

**Reference docs (planner consults):**
- `.planning/quick/260505-ayr-create-a-dorico-agent-that-helps-with-vs/260505-ayr-RESEARCH.md` §7
- `.claude/agents/dorico-agent.md`
- `.claude/agent-memory/dorico-agent.md`

**Hard constraint:** minimal-edit. Do NOT rewrite agent bodies. Insertion only.

</domain>

<decisions>
## Implementation Decisions

### D1 — Routing block format: Named subsection with triggers

Each of the 6 files gets a consistently-named subsection. Use **`## Dorico Delegation`** (level depends on local file convention — match nearest existing peer-section level).

Block contents:
- One-line purpose statement
- **Triggers:** bulleted list of explicit symptom keywords / file extensions / conditions (file-specific)
- **Handoff:** how to invoke (`Task(subagent_type="dorico-agent", …)` for agents/orchestrators; "route to dorico-agent" for SKILL routing-step language)
- **Reference docs:** pointers into `dorico-agent.md`, `agent-memory/dorico-agent.md`, and the spike RESEARCH.md §7

For files 5–6 (gui-agent, dsp-agent) the subsection is a **cross-reference callout**, not a delegation rule — same block name (`## Dorico Delegation`) but content is "before editing X, see dorico-agent for the load-bearing detail Y." Keeps naming consistent across all 6 files.

### D2 — plugin-publishing gate semantics: Advisory warning only

When `dorico-agent` reports TC-1..TC-5 failures during pre-release validation, plugin-publishing surfaces an **advisory warning block** in the publish summary and proceeds with release. Does NOT block `/publish`.

Rationale: matches user preference for low-friction publishing; the cohort sweep is added value, not a hard gate. Failures still get logged and visible for follow-up. Future tightening (block-on-failure) can be added later as opt-in config.

### D3 — generalize-microtones Phase B invocation: Sequential, one Task() per plugin

The command loops over the 7 microtonal-cohort plugins and spawns `dorico-agent` once per plugin, **serially**. Each call is a discrete handoff with the plugin name as input.

Rationale: easier to triage a per-plugin failure, lower context pressure on the orchestrator, clean stop-on-first-failure for debugging. Wall-clock cost is acceptable for a one-time propagation phase.

### Claude's Discretion
- Exact wording inside each routing block (keep terse, ≤ ~12 lines per insertion).
- Section heading depth (`##` vs `###`) — match peer sections in the file being edited.
- Insertion anchor inside each file — researcher will identify the natural location (top of body, after frontmatter, before "## Workflow"-style sections).

</decisions>

<specifics>
## Specific Ideas

- File-by-file trigger keywords (researcher to confirm against actual file contents):
  - `troubleshoot-agent.md`: `.doricolib`, `.doricoexpmap`, `EndpointConfig`, "Playback Template", "kVST3NoteExpression", "microtonal regression", "keyswitch", "KS failure"
  - `plugin-improve/SKILL.md`: bug-classification keywords — "Dorico", "microtonal in Dorico", "exp map", "playback template"
  - `plugin-publishing/SKILL.md`: cohort match — plugin name in {O-Lyrica, O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant, O-MicrotonalSampler}
  - `generalize-microtones.md`: the Phase B for-each-plugin loop body
  - `gui-agent.md`: cross-ref keyed off `createParameterLayout()` mention
  - `dsp-agent.md`: cross-ref keyed off `trigger()` / `currentFrequency` mention

- Commit message convention (mirroring `260505-ayr` series):
  `feat(dorico-agent): wire {short-target} to delegate to dorico-agent` per file.

</specifics>

<canonical_refs>
## Canonical References

- `.planning/quick/260505-ayr-create-a-dorico-agent-that-helps-with-vs/260505-ayr-RESEARCH.md` — full research (§7 = handoff/integration recommendations)
- `.claude/agents/dorico-agent.md` — agent definition (delegation target)
- `.claude/agent-memory/dorico-agent.md` — persistent memory (load-bearing facts the dorico-agent already knows)
- Project memory (`MEMORY.md`):
  - `critical_dorico_microtonal_top_level_fields.md` — recurring regression
  - `critical_dorico_keyswitch_routing.md` — 3-layer KS stack
  - `critical_dorico_distribution_mechanism.md` — Playback Template requirement
  - `project_o_lyrica_spike_reference.md` — validated spike

</canonical_refs>
