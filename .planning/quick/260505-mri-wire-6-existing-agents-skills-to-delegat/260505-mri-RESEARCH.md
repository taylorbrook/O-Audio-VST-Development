# Quick Task 260505-mri: Wire 6 agents/skills to dorico-agent — Research

**Researched:** 2026-05-05
**Domain:** Per-file insertion playbook for routing/cross-reference subsections
**Confidence:** HIGH (all 6 target files read in full; insertion anchors verified against verbatim file content)

## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D1** — Each file gets a `## Dorico Delegation` (or `### Dorico Delegation`) subsection. Match peer-section heading depth.
- **D2** — `plugin-publishing` SMOKE-TEST gate is **advisory only**: failures surface as warnings, release proceeds. Does NOT block `/publish`.
- **D3** — `generalize-microtones` Phase B invocation is **sequential**: one `Task()` per plugin, serial.
- Block contents: one-line purpose + **Triggers** + **Handoff** + **Reference docs**.
- Hard constraint: minimal-edit. NO body rewrites.

### Claude's Discretion
- Exact wording inside each routing block (≤ ~14 lines per insertion).
- Exact insertion anchor (researcher chose; verbatim quotes below).
- For files 5 & 6 (gui/dsp): cross-reference language, not delegation rules.

### Deferred Ideas
- (none)

---

## File 1: `.claude/agents/troubleshoot-agent.md`

**Summary.** Long agent file (807 lines). Top-level `## Purpose` / `## PFS Context` / `## Responsibilities` / `<workflow>` sections; uses `## H2` for major sections, `### H3` for sub-steps. Body has graduated 4-level research protocol (Level 0 → 3) followed by `<constraints>` block at line 256. The natural insertion point is **after the `## Responsibilities` numbered list (line 56) and before the `<workflow>` block (line 57)** — this is where domain-specific routing rules belong before the generic graduated protocol kicks in.

**Insertion anchor (verbatim, line 56):**
```
6. **Stop when confident** (don't over-research simple problems)
```
Insert the new `## Dorico Delegation` block **immediately after** this line, before the blank line preceding `<workflow>` on line 57. Anchor is unique in the file (verified by grep on the exact 6-prefix bullet).

**Recommended subsection heading.** `## Dorico Delegation` (level 2, matches `## Purpose` / `## Responsibilities` peers).

**Subsection body draft:**
```markdown
## Dorico Delegation

When the symptom is Dorico-specific, recommend handoff to `dorico-agent` instead of running the graduated 4-level research yourself. The Dorico agent is write-capable and already knows the 12 known landmines.

**Triggers (any of):**
- File extensions: `.doricolib`, `.doricoexpmap`, `endpointconfig.xml`, `playbacktemplatespec.xml`
- Symptoms: "microtonal regression", "quarter-sharp wrong", "keyswitch not firing", "KS failure", "Playback Template", "DefaultLibraryAdditions"
- Keywords in error/log: `kVST3NoteExpression`, `kKeySwitch`, `pitchBendRange`, `EndpointConfig`

**Handoff:** Return to invoker recommending `Task(subagent_type="dorico-agent", description="<plugin> Dorico <symptom>", prompt="<full symptom + repro>")`. Do NOT spawn it yourself (you lack Task tool by design — same rule as `deep-research`).

**Reference docs:** `.claude/agents/dorico-agent.md`; spike research `.planning/quick/260505-ayr-create-a-dorico-agent-that-helps-with-vs/260505-ayr-RESEARCH.md` §6 (12 landmines).
```

**Notes / risks.** Anchor is unique and stable. The agent file enforces "no Task tool" by design (line 18, 762), so the handoff phrasing matches existing precedent ("Return to invoker recommending..." mirrors line 646 deep-research handoff exactly). Low risk.

---

## File 2: `.claude/skills/plugin-improve/SKILL.md`

**Summary.** SKILL with phased workflow (Phase 0 → 8). Uses `## H2` for phases. Bug classification happens in **Phase 0.5: Investigation (Auto-Tiered)** at line 267, which auto-detects Tier 1/2/3 and may delegate to `deep-research` for Tier 3. The natural insertion point is **immediately after Phase 0.5** (before Phase 0.6 at line 281) — this is the bug-classification step CONTEXT.md identifies as the routing point.

**Insertion anchor (verbatim, lines 278-280):**
```
**See**: [references/investigation-tiers.md](references/investigation-tiers.md) for complete tier detection algorithm and protocols for each tier (1: Basic Code Inspection, 2: Root Cause Analysis, 3: Deep Research Delegation).

## Phase 0.6: Implementation Planning
```
Insert the new `## Dorico Delegation` block **between** the "See: references/investigation-tiers.md..." line and `## Phase 0.6`. Anchor is unique (only one `## Phase 0.6` heading in file).

**Recommended subsection heading.** `## Dorico Delegation` (level 2, matches `## Phase 0.5` / `## Phase 0.6` peers — sits **as a sub-routing rule of Phase 0.5**, not a new phase).

**Subsection body draft:**
```markdown
## Dorico Delegation

After Phase 0.5 tier detection, if the bug report is Dorico-tagged, route directly to `dorico-agent` regardless of tier. Skip the Tier 1/2/3 escalation — `dorico-agent` runs its own graduated protocol scoped to Dorico landmines.

**Triggers (any of):**
- User report mentions: "Dorico", "microtonal in Dorico", "expression map", "exp map", "playback template", "keyswitch in Dorico", "endpoint config"
- Plugin-name + Dorico-tagged TC-failure (TC-1..TC-5 from `Resources/dorico/SMOKE-TEST.md`)

**Handoff:** Invoke via `Task(subagent_type="dorico-agent", description="<Plugin> Dorico bug", prompt="<user report + plugin context>")`. Wait for its DIAGNOSIS / ROOT CAUSE / FIX APPLIED report, then resume Phase 0.6 planning around the proposed fix.

**Reference docs:** `.claude/agents/dorico-agent.md`; the agent's own memory at `.claude/agent-memory/dorico-agent.md`.
```

**Notes / risks.** Anchor is unique (the `**See**:` line ends Phase 0.5 cleanly). The block sits as a **sibling** to Phase 0.6, which is consistent with how the file already uses `## Phase 0.45`, `## Phase 0.5`, `## Phase 0.6` as peer routing checkpoints. Low risk.

---

## File 3: `.claude/skills/plugin-publishing/SKILL.md`

**Summary.** SKILL with 7-step linear release workflow inside a `<critical_sequence>` block (lines 16-225). Heading depth is `### N. Step` (level 3) for each numbered step. `## H2` for major sections (Workflow, Decision Menu, Integration Points, etc.). The CONTEXT specifies the gate is **advisory** — best location is **before Step 5 "Commit Changes"** (so the warning gets surfaced before the release commit is created and pushed). That puts validation at the latest point where a release author still has all changes uncommitted.

**Insertion anchor (verbatim, lines 163-165):**
```
**Validation:** Verify all three files modified before proceeding.

---
```
This `**Validation:**` line ends Step 4 and is followed by `---` then `### 5. Commit Changes`. Insert the new step **between the `---` and `### 5. Commit Changes`** so it becomes a new step `### 4b. Microtonal Cohort SMOKE-TEST (Advisory)`. Anchor combo (`Validation:** Verify all three files modified before proceeding.` + the trailing `---`) is unique in file.

**Recommended subsection heading.** `### 4b. Microtonal Cohort SMOKE-TEST (Advisory)` (level 3, matches `### 4. Update Files` / `### 5. Commit Changes` peers). Note: NOT `## Dorico Delegation` here — this skill's heading convention is numbered steps, so a numbered sub-step reads more naturally. The block STILL contains the standard Triggers/Handoff/Reference docs structure inside.

**Subsection body draft:**
```markdown
### 4b. Microtonal Cohort SMOKE-TEST (Advisory)

If the plugin being published is in the **microtonal cohort**, run a Dorico smoke-test sweep via `dorico-agent` and surface results as a warning block in the publish summary. **This is advisory — does NOT block release.**

**Cohort:** O-Lyrica, O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant, O-MicrotonalSampler.

**Trigger:** `[PluginName]` matches one of the cohort entries above.

**Handoff:**
```
Task(subagent_type="dorico-agent",
     description="[PluginName] pre-release SMOKE-TEST",
     prompt="Run TC-1..TC-5 from plugins/[PluginName]/Resources/dorico/SMOKE-TEST.md against the v[NEW_VERSION] build. Report PASS/FAIL per TC. Do not edit anything; this is a validation pass only.")
```

**Result handling:**
- **All TC pass** → continue to Step 5, no warning shown.
- **Any TC fails** → append an "⚠ Dorico SMOKE-TEST advisory" block to the publish summary listing the failing TCs and the agent's diagnosis. **Continue to Step 5 anyway** — release proceeds. User decides follow-up.

**Reference docs:** `.claude/agents/dorico-agent.md` (Output Contract section); per-plugin `Resources/dorico/SMOKE-TEST.md`.
```

**Notes / risks.**
- HEAVIER insertion than the others (~25 lines), but appropriate because step-format files use longer numbered steps.
- Anchor is the `**Validation:**` line + `---` separator — unique combo, locatable.
- "Cohort" list is hardcoded in the inserted text. If the cohort grows, this block must be updated. Acceptable trade-off vs. introducing a separate config file (which would expand task scope).
- The file's progress checklist (lines 19-29) does NOT need updating — Step 4b is implicitly covered by Step 4 in that checklist. Optional: planner could add a new `- [ ] 4b. Microtonal cohort smoke-test (if applicable)` line, but CONTEXT says minimal-edit, so leave it.
- **Risk flag:** This is the most structurally-involved insertion of the 6 files. Recommend executor read lines 145-200 of the SKILL before editing to confirm the `### 4. Update Files` step structure is exactly as researched.

---

## File 4: `.claude/commands/generalize-microtones.md`

**Summary.** Short slash-command file (66 lines). Free-form prose with `## Scope` (numbered subsections 1/2/3), `## Known landmines`, `## Out of scope`, `## Kickoff`. Phase B is described in `## Kickoff` at line 56 as a bullet inside a "**Phase structure**" sub-block. The Phase B per-plugin loop is the bullet at line 60: `**Phase B — Propagate:** apply the module to the remaining 7 pitched plugins.` The "for each plugin" loop body is line 38 (inside `## Scope` §2): `For each plugin: /module-add → wire getVST3ClientExtensions() → ...`. The natural insertion point is **between the `## Scope` numbered table (Section 2) and `### 3. End-user deliverable`** — right after the per-plugin loop body description at line 38, so the dorico-agent delegation rule sits next to the per-plugin recipe it modifies.

**Insertion anchor (verbatim, line 38):**
```
For each plugin: `/module-add` → wire `getVST3ClientExtensions()` → add per-voice tuning source + `applyPendingTuning` in `startNote` → build + install + pluginval → Dorico quarter-sharp smoke test.
```
Insert the new `## Dorico Delegation` block **immediately after this line** (before `### 3. End-user deliverable` at line 40). Anchor is unique (only `For each plugin:` instance in file).

**Recommended subsection heading.** `### 2a. Dorico Delegation (Phase B per-plugin)` (level 3, matches `### 1. Shared module extraction` / `### 2. Target plugins` / `### 3. End-user deliverable` peers under `## Scope`).

**Subsection body draft:**
```markdown
### 2a. Dorico Delegation (Phase B per-plugin)

The "Dorico quarter-sharp smoke test" step in the per-plugin loop above delegates to `dorico-agent` rather than re-deriving the smoke-test pattern inline.

**Phase B invocation pattern (sequential — one Task() per plugin, serial):**
```
for plugin in [O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant]:
    Task(subagent_type="dorico-agent",
         description="${plugin} Phase B Dorico bring-up",
         prompt="Apply Dorico expression-map + endpoint config + playback template entry for ${plugin} based on the O-MicrotonalSampler v1.16.x reference. Run TC-1..TC-5 smoke test. Report status.")
    # Wait for completion, inspect report, halt-on-failure for triage.
```

**Why sequential:** one-time propagation — wall-clock cost acceptable; per-plugin failure isolation simpler; clean stop-on-first-failure for debugging.

**Reference docs:** `.claude/agents/dorico-agent.md` (Scope, Capabilities, Output Contract); `plugins/O-MicrotonalSampler/Resources/dorico/` (canonical reference).
```

**Notes / risks.**
- Inserted block uses level-3 heading (`### 2a.`) which inserts cleanly between `### 2.` (line 26) and `### 3.` (line 40). Subsequent renumbering NOT needed — `2a` is a recognized sub-step convention.
- The for-each-plugin pseudocode lists 8 plugins (matches the table at lines 28-37), but CONTEXT.md mentions a 7-plugin microtonal cohort for plugin-publishing. These are different cohorts (publishing skips O-IntonationPad and O-Prism but includes O-MicrotonalSampler; generalize-microtones includes O-IntonationPad/O-Prism but starts from O-Lyrica as reference). DO NOT cross-pollinate the lists. Each block uses its own list.
- Low risk; inserted block is purely additive within an already structured Scope section.

---

## File 5: `.claude/agents/gui-agent.md`

**Cross-reference callout, NOT a delegation rule.**

**Summary.** Very long agent file (1444 lines). Heading conventions: `## H2` for major sections (`## Preconditions`, `## YOUR ROLE`, `## Implementation Steps`, `## Common Issues and Resolutions`, `## State Management`, `## MANDATORY Thread Safety Patterns`, `## Persistent Memory`); `### H3` for sub-steps. The `<critical_patterns>` block at lines 512-556 contains the `### CRITICAL PATTERNS (Member Order and Parameter Bindings)` section — perfect peer location for a Dorico cross-reference because it sits in the same conceptual zone as parameter-binding caveats. However, the cleaner spot is **immediately after the `<required_reading>` block (line 143)** so the Dorico note appears alongside the other "before you start" required references like the cross-platform WebView research doc.

**Insertion anchor (verbatim, lines 142-144):**
```
8. **Cross-platform WebView best practices:** `research/cross-platform-webview-best-practices.md` - CRITICAL for Windows compatibility
</required_reading>

<template_library>
```
Insert the new `## Dorico Delegation` block **between the `</required_reading>` close-tag and the `<template_library>` open-tag**. Anchor is unique (only one `</required_reading>` and one `<template_library>` in file).

**Recommended subsection heading.** `## Dorico Delegation` (level 2, matches `## Template Library` / `## Implementation Steps` peers).

**Subsection body draft:**
```markdown
## Dorico Delegation

**Cross-reference (NOT a delegation rule).** When editing `createParameterLayout()` for plugins in the microtonal cohort, the parameter defaults you set are **Layer 2 of the 3-layer Dorico keyswitch routing stack**. Wrong defaults silently absorb correctly-routed Dorico keyswitches with no error.

**Trigger:** any edit to `createParameterLayout()` involving these parameter IDs (or their per-plugin equivalents): `ks_enabled`, `technique_count`, `cc_select_enabled`, `pc_enabled`, KS-range gates.

**Required pattern (from `critical_dorico_keyswitch_routing.md` Layer 2):**
- `ks_enabled` MUST default `true` (gates that default false silently absorb KS).
- `technique_count` MUST default to actual technique count, NOT `1` (clamps would route every KS to slot 0).
- Same rule applies to `cc_select_enabled`, `pc_enabled`.

**When in doubt:** consult `dorico-agent` before changing these defaults. Do NOT spawn it from gui-agent — return to invoker (plugin-workflow) recommending a separate `Task(subagent_type="dorico-agent", ...)` invocation.

**Reference docs:** `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_keyswitch_routing.md`; `.claude/agents/dorico-agent.md` (Landmine 4).
```

**Notes / risks.**
- Inserted block sits BETWEEN two XML-style tags (`</required_reading>` and `<template_library>`). Both are real markup in this file; insertion preserves both tag boundaries cleanly.
- The block deliberately does NOT prescribe spawning `dorico-agent` from inside gui-agent (gui-agent has Task tool? Check tools list line 4: `Read, Edit, Write, Bash, mcp__context7__...`. **No Task tool.** So the "return to invoker" phrasing is accurate and necessary).
- The named parameter IDs (`ks_enabled`, `technique_count`, etc.) come from `critical_dorico_keyswitch_routing.md` Layer 2 (verified). They are O-MicrotonalSampler-canonical; other plugins may use different names. Block phrasing "or their per-plugin equivalents" handles this.
- Low risk.

---

## File 6: `.claude/agents/dsp-agent.md`

**Cross-reference callout, NOT a delegation rule.**

**Summary.** Long agent file (1240 lines). Heading conventions: `## H2` for major sections (`## Precondition Verification`, `## YOUR ROLE`, `## Implementation Steps`, `## Real-Time Safety Rules`, `## State Management`, `## JSON Report Format`, `## Real-Time Safety Checklist`, `## JUCE DSP Best Practices`); `### N. StepName` (level 3) for numbered implementation steps. **NOTE:** dsp-agent.md does NOT mention `trigger()` or `currentFrequency` anywhere (verified by grep — only finds `triggerAsyncUpdate` and "UI rebuild trigger" in unrelated comments). This means the cross-reference is purely PROACTIVE; there's no existing line tied to note-expression voice triggering for the Dorico note to anchor against. The natural insertion point is **immediately after the `<required_reading>` block (around line 165) and before `<complexity_aware>`** — same structural slot as in gui-agent for symmetry.

**Insertion anchor (verbatim, lines 164-167):**
```
5. Modern juce::dsp API: Use ProcessSpec/AudioBlock/ProcessContext (not old API)
</required_reading>

<complexity_aware>
```
Insert the new `## Dorico Delegation` block **between the `</required_reading>` close-tag and the `<complexity_aware>` open-tag**. Anchor is unique (only one `</required_reading>` and one `<complexity_aware>` in file).

**Recommended subsection heading.** `## Dorico Delegation` (level 2, matches `## Inputs (Contracts)` / `## Task` peers — and matches the same heading used in gui-agent for cross-file consistency).

**Subsection body draft:**
```markdown
## Dorico Delegation

**Cross-reference (NOT a delegation rule).** When implementing or editing voice-allocation / `startNote` code for plugins that use VST3 Note Expression for microtonal playback, **apply NE tuning to `currentFrequency` BEFORE the DSP model's `trigger(...)` call**. Wrong order = first sample renders at untuned pitch → audible zipper at attack.

**Trigger:** edits to per-voice `startNote` / `triggerVoice` / equivalent that touch `currentFrequency` and any subsequent `model.trigger(...)` / oscillator `setFrequency(...)` call sequence.

**Required pattern (from spike-findings landmine):**
1. Compute final `currentFrequency` (apply pending NE tuning from `noteId → cents-deviation` map).
2. THEN call `trigger(...)` / `setFrequency(...)`.

Spike validated this order on O-Lyrica end-to-end. Other plugins consuming the shared `dsp/note-expression` module inherit the constraint.

**When in doubt:** consult `dorico-agent` before re-ordering or refactoring. Do NOT spawn it from dsp-agent — dsp-agent has no Task tool. Return to invoker (plugin-workflow) recommending a separate `Task(subagent_type="dorico-agent", ...)`.

**Reference docs:** `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/project_o_lyrica_spike_reference.md`; `.claude/agents/dorico-agent.md`; `Skill("spike-findings-VST-development")` (Pattern 6 + Landmine 6 — the trigger-order rule).
```

**Notes / risks.**
- dsp-agent has NO `Task` tool (verified line 4: `Read, Edit, Write, mcp__context7__...`) — handoff phrasing must be "return to invoker," not "spawn." Block reflects this.
- Anchor (`</required_reading>` + `<complexity_aware>` adjacency) is unique and stable.
- The block intentionally cites `Skill("spike-findings-VST-development")` because that's where Pattern 6 / Landmine 6 (the trigger-order rule) is canonically stored — the project memory file referenced in CONTEXT (`critical_juce_webview_namespace_vs_postmessage.md`) is **NOT relevant here** as CONTEXT explicitly notes. Only `project_o_lyrica_spike_reference.md` and the spike-findings skill are correct references.
- Low risk.

---

## Cross-cutting concerns

### Heading-name consistency
All 6 files use `## Dorico Delegation` as the section name **except** File 3 (`plugin-publishing`) which uses `### 4b. Microtonal Cohort SMOKE-TEST (Advisory)` because that file's convention is numbered steps, not free section headings. The triggers/handoff/reference-docs structure is preserved inside the body in all 6 cases. The CONTEXT D1 instruction permits depth variation ("level depends on local file convention").

### Block structure (canonical)
Every block contains:
1. **One-line purpose statement** (or "cross-reference (NOT a delegation rule)" for files 5 & 6).
2. **Triggers** — bulleted symptom keywords / file extensions / parameter IDs / cohort-membership rules.
3. **Handoff** — `Task(subagent_type="dorico-agent", ...)` syntax for files with Task tool (plugin-improve, plugin-publishing, generalize-microtones); "return to invoker recommending..." for files without (troubleshoot-agent, gui-agent, dsp-agent).
4. **Reference docs** — repo-relative paths to `.claude/agents/dorico-agent.md`, the spike research, and any relevant `critical_dorico_*.md` memory file.

### Tool-availability matrix (verified against each frontmatter)
| File | Has Task tool? | Handoff phrasing |
|---|---|---|
| `troubleshoot-agent.md` | ❌ | "Return to invoker recommending..." |
| `plugin-improve/SKILL.md` | ✅ (allowed-tools includes Task) | Direct `Task(subagent_type=...)` |
| `plugin-publishing/SKILL.md` | (skill — runs in main agent context, full tool access) | Direct `Task(subagent_type=...)` |
| `generalize-microtones.md` | (slash command — runs in main agent context, full tool access) | Direct `Task(subagent_type=...)` in pseudocode loop |
| `gui-agent.md` | ❌ | "Return to invoker recommending..." |
| `dsp-agent.md` | ❌ | "Return to invoker recommending..." |

### Atomic-commit convention
Per CONTEXT.md `<specifics>`: one commit per file, message format `feat(dorico-agent): wire {short-target} to delegate to dorico-agent`. Six commits total. Suggested short-target slugs:
- `troubleshoot-agent`
- `plugin-improve-skill`
- `plugin-publishing-skill`
- `generalize-microtones-cmd`
- `gui-agent-cross-ref`
- `dsp-agent-cross-ref`

### Risk-flag summary
- **File 3 (`plugin-publishing/SKILL.md`)** — heaviest insertion (~25 lines for new step `### 4b.`). Recommend executor re-read lines 145-200 before editing to confirm step structure is unchanged.
- **All other files** — low risk; insertion anchors are unique, structural fit is clean, no body rewrites required.

### Cohort-list discrepancy (intentional)
- **plugin-publishing cohort** (7 plugins): O-Lyrica, O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant, O-MicrotonalSampler.
- **generalize-microtones cohort** (8 plugins, Phase B loop): O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant.
These lists are deliberately different (one is "what to ship & sweep at release time", the other is "what to propagate the shared module to during one-time generalization"). Do NOT unify them.

### Reference-doc paths (verbatim, used across blocks)
- `.claude/agents/dorico-agent.md` (the agent itself)
- `.claude/agent-memory/dorico-agent.md` (the agent's seeded memory)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_keyswitch_routing.md` (3-layer KS stack)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_microtonal_top_level_fields.md` (recurring regression)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_distribution_mechanism.md` (Playback Template requirement)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/project_o_lyrica_spike_reference.md` (validated spike, auval DEF-24-01 benign)
- `.planning/quick/260505-ayr-create-a-dorico-agent-that-helps-with-vs/260505-ayr-RESEARCH.md` (spike research, §6 = 12 landmines, §7 = integration hooks)

### Out-of-scope (do NOT do in this task)
- Do NOT update `.claude/agent-memory/troubleshoot-agent.md` line 4 — that's recurring-regression memory the dorico-agent already references; leaving it in troubleshoot-agent's memory ensures redundancy in case dorico-agent isn't invoked.
- Do NOT add a "## Dorico Delegation" block to any file beyond the 6 named in CONTEXT.md (e.g., do NOT add to `validation-agent.md` or `foundation-shell-agent.md` — out of scope).
- Do NOT update the `<integration_points>` / `## Integration Points` sections at the bottom of any file. The new sections are self-documenting.

---

## RESEARCH COMPLETE

**Phase:** quick-task 260505-mri — Wire 6 agents/skills to dorico-agent
**Confidence:** HIGH — all 6 target files read in full; insertion anchors quoted verbatim and verified unique.

### File Created
`/Users/taylorbrook/Dev/VST-development/.planning/quick/260505-mri-wire-6-existing-agents-skills-to-delegat/260505-mri-RESEARCH.md`

### Key Findings
- All 6 insertion points identified and anchored with verbatim line quotes.
- File 3 (plugin-publishing) is the only structurally heavier insertion (new numbered step `### 4b.`); all others slot in cleanly between existing sections.
- Tool-availability matrix verified per file frontmatter: 3 of 6 files (troubleshoot/gui/dsp) lack Task tool → "return to invoker" handoff phrasing; the other 3 use direct `Task(subagent_type="dorico-agent", ...)`.
- Cohort lists for plugin-publishing (7) vs generalize-microtones (8) are deliberately different; do NOT unify.
- Files 5 (gui-agent) & 6 (dsp-agent) get cross-reference callouts (NOT delegation rules) — `createParameterLayout()` defaults (Layer 2 of KS stack) and tuning-before-trigger ordering, respectively.

### Ready for Planning
Planner can produce a 6-task plan, one task per file, each task = `Edit(old_string=<verbatim anchor>, new_string=<anchor + inserted block>)` against the named target file. Each task = one atomic commit. Executor can follow blind from this document.
