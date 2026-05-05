---
quick_id: 260505-mri
mode: quick
task_count: 6
description: Wire 6 existing agents/skills to delegate to the new dorico-agent. Insertion-only routing/cross-reference subsections. Each target file = one atomic commit.
---

# Quick Task 260505-mri — PLAN

## Goal

Insert a `## Dorico Delegation` (or numbered-step equivalent for File 3) subsection into each of 6 existing agent/skill/command files, wiring them to delegate Dorico-specific work to the new `dorico-agent` (created in quick task `260505-ayr`).

**Honoring CONTEXT.md user decisions:**
- **D1 — Routing block format:** Each file gets a `## Dorico Delegation` subsection (or `### 4b.` numbered-step variant for File 3, per peer-section convention). Block contents = one-line purpose + Triggers + Handoff + Reference docs.
- **D2 — plugin-publishing gate semantics:** SMOKE-TEST is **advisory only** — failures surface as a warning block in the publish summary; release proceeds. Does NOT block `/publish`.
- **D3 — generalize-microtones Phase B invocation:** Sequential — one `Task()` per plugin, serial; halt-on-first-failure for triage.

**Hard constraint:** minimal-edit. Insertion-only. Do NOT rewrite agent bodies.

**Atomic commit convention:** one commit per task, six commits total. Subject pattern: `feat(dorico-agent): wire <short-target> to delegate to dorico-agent`.

**Cross-cutting note (handoff phrasing varies by tool availability):**
- Files WITHOUT `Task` tool (troubleshoot-agent, gui-agent, dsp-agent) → "Return to invoker recommending..." phrasing.
- Files WITH `Task` tool / running in main agent context (plugin-improve, plugin-publishing, generalize-microtones) → Direct `Task(subagent_type="dorico-agent", ...)` syntax.
This split is preserved verbatim in each task's body draft below — do NOT collapse to a single template.

---

## Task 1 — Wire `troubleshoot-agent.md`

**files:** `.claude/agents/troubleshoot-agent.md`

**action:** Insert `## Dorico Delegation` subsection (level 2) immediately after the `## Responsibilities` numbered list (line 56), before the `<workflow>` block. Single Edit. No body rewrites elsewhere.

**old_string** (verbatim anchor — unique in file):
```
6. **Stop when confident** (don't over-research simple problems)
```

**new_string** (anchor + inserted block — note "return to invoker" phrasing because troubleshoot-agent has NO Task tool):
```
6. **Stop when confident** (don't over-research simple problems)

## Dorico Delegation

When the symptom is Dorico-specific, recommend handoff to `dorico-agent` instead of running the graduated 4-level research yourself. The Dorico agent is write-capable and already knows the 12 known landmines.

**Triggers (any of):**
- File extensions: `.doricolib`, `.doricoexpmap`, `endpointconfig.xml`, `playbacktemplatespec.xml`
- Symptoms: "microtonal regression", "quarter-sharp wrong", "keyswitch not firing", "KS failure", "Playback Template", "DefaultLibraryAdditions"
- Keywords in error/log: `kVST3NoteExpression`, `kKeySwitch`, `pitchBendRange`, `EndpointConfig`

**Handoff:** Return to invoker recommending `Task(subagent_type="dorico-agent", description="<plugin> Dorico <symptom>", prompt="<full symptom + repro>")`. Do NOT spawn it yourself (you lack Task tool by design — same rule as `deep-research`).

**Reference docs:** `.claude/agents/dorico-agent.md`; spike research `.planning/quick/260505-ayr-create-a-dorico-agent-that-helps-with-vs/260505-ayr-RESEARCH.md` §6 (12 landmines).
```

**verify:**
- `grep -c "^## Dorico Delegation$" .claude/agents/troubleshoot-agent.md` returns `1`
- `grep -c "Return to invoker recommending" .claude/agents/troubleshoot-agent.md` returns `>= 1`
- `grep -n "6. \*\*Stop when confident\*\*" .claude/agents/troubleshoot-agent.md` followed immediately by the new section header (verify with `grep -A2 "Stop when confident"`)

**done:** One atomic commit on current branch touching exactly `.claude/agents/troubleshoot-agent.md`. Commit subject: `feat(dorico-agent): wire troubleshoot-agent to delegate to dorico-agent`.

---

## Task 2 — Wire `plugin-improve/SKILL.md`

**files:** `.claude/skills/plugin-improve/SKILL.md`

**action:** Insert `## Dorico Delegation` subsection (level 2, sibling to Phase 0.5/0.6) between the `**See**: references/investigation-tiers.md` line and the `## Phase 0.6: Implementation Planning` heading. Single Edit. No body rewrites elsewhere.

**old_string** (verbatim anchor — unique in file via combo of See: line + Phase 0.6 heading):
```
**See**: [references/investigation-tiers.md](references/investigation-tiers.md) for complete tier detection algorithm and protocols for each tier (1: Basic Code Inspection, 2: Root Cause Analysis, 3: Deep Research Delegation).

## Phase 0.6: Implementation Planning
```

**new_string** (anchor + inserted block — direct `Task(...)` invocation because plugin-improve SKILL has Task in allowed-tools):
```
**See**: [references/investigation-tiers.md](references/investigation-tiers.md) for complete tier detection algorithm and protocols for each tier (1: Basic Code Inspection, 2: Root Cause Analysis, 3: Deep Research Delegation).

## Dorico Delegation

After Phase 0.5 tier detection, if the bug report is Dorico-tagged, route directly to `dorico-agent` regardless of tier. Skip the Tier 1/2/3 escalation — `dorico-agent` runs its own graduated protocol scoped to Dorico landmines.

**Triggers (any of):**
- User report mentions: "Dorico", "microtonal in Dorico", "expression map", "exp map", "playback template", "keyswitch in Dorico", "endpoint config"
- Plugin-name + Dorico-tagged TC-failure (TC-1..TC-5 from `Resources/dorico/SMOKE-TEST.md`)

**Handoff:** Invoke via `Task(subagent_type="dorico-agent", description="<Plugin> Dorico bug", prompt="<user report + plugin context>")`. Wait for its DIAGNOSIS / ROOT CAUSE / FIX APPLIED report, then resume Phase 0.6 planning around the proposed fix.

**Reference docs:** `.claude/agents/dorico-agent.md`; the agent's own memory at `.claude/agent-memory/dorico-agent.md`.

## Phase 0.6: Implementation Planning
```

**verify:**
- `grep -c "^## Dorico Delegation$" .claude/skills/plugin-improve/SKILL.md` returns `1`
- `grep -c "subagent_type=\"dorico-agent\"" .claude/skills/plugin-improve/SKILL.md` returns `>= 1`
- Order check: `grep -n "^## Phase 0.5\|^## Dorico Delegation\|^## Phase 0.6" .claude/skills/plugin-improve/SKILL.md` must list Phase 0.5 → Dorico Delegation → Phase 0.6 in that order.

**done:** One atomic commit on current branch touching exactly `.claude/skills/plugin-improve/SKILL.md`. Commit subject: `feat(dorico-agent): wire plugin-improve bug-classification to delegate to dorico-agent`.

---

## Task 3 — Wire `plugin-publishing/SKILL.md` ⚠ RISK-FLAGGED (heaviest insertion)

**files:** `.claude/skills/plugin-publishing/SKILL.md`

**action:** Insert `### 4b. Microtonal Cohort SMOKE-TEST (Advisory)` (level 3 numbered step, peer to `### 4.` and `### 5.`) between the Step 4 `**Validation:** Verify all three files modified before proceeding.` line + trailing `---` separator and the `### 5. Commit Changes` heading. Single Edit. No body rewrites elsewhere.

**⚠ Risk note for executor:** This is the most structurally-involved insertion of the 6 files (~25 new lines, new numbered step). **Re-read lines 145-200 of `.claude/skills/plugin-publishing/SKILL.md` before editing** to confirm Step 4 structure is exactly as quoted in the anchor. Heading depth uses `### N.` (level 3) — do NOT use `## Dorico Delegation` here; the file's convention is numbered steps. Triggers/Handoff/Reference docs structure is preserved inside the body.

**old_string** (verbatim anchor — combo of `**Validation:**` line + `---` + `### 5.` heading is unique in file):
```
**Validation:** Verify all three files modified before proceeding.

---

### 5. Commit Changes
```

**new_string** (anchor + inserted advisory step — direct `Task(...)` invocation because skill runs in main agent context with full tool access):
```
**Validation:** Verify all three files modified before proceeding.

---

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

---

### 5. Commit Changes
```

**verify:**
- `grep -c "^### 4b\. Microtonal Cohort SMOKE-TEST (Advisory)$" .claude/skills/plugin-publishing/SKILL.md` returns `1`
- `grep -c "advisory — does NOT block release" .claude/skills/plugin-publishing/SKILL.md` returns `>= 1` (D2 semantics carried)
- `grep -c "O-MicrotonalSampler" .claude/skills/plugin-publishing/SKILL.md` returns `>= 1` (cohort list landed)
- Order check: `grep -nE "^### (4\.|4b\.|5\.) " .claude/skills/plugin-publishing/SKILL.md` must list `### 4.` → `### 4b.` → `### 5.` in that order.

**done:** One atomic commit on current branch touching exactly `.claude/skills/plugin-publishing/SKILL.md`. Commit subject: `feat(dorico-agent): wire plugin-publishing pre-release smoke-test to delegate to dorico-agent`.

---

## Task 4 — Wire `commands/generalize-microtones.md`

**files:** `.claude/commands/generalize-microtones.md`

**action:** Insert `### 2a. Dorico Delegation (Phase B per-plugin)` (level 3, peer to `### 1.`/`### 2.`/`### 3.` under `## Scope`) immediately after the `For each plugin: ...` line at line 38 and before `### 3. End-user deliverable`. Single Edit. No body rewrites elsewhere.

**old_string** (verbatim anchor — unique; only `For each plugin:` instance in file):
```
For each plugin: `/module-add` → wire `getVST3ClientExtensions()` → add per-voice tuning source + `applyPendingTuning` in `startNote` → build + install + pluginval → Dorico quarter-sharp smoke test.
```

**new_string** (anchor + inserted block — direct `Task(...)` in pseudocode loop because slash command runs in main agent context; **D3 sequential invocation** explicitly stated; **8-plugin cohort distinct from publishing's 7-plugin cohort** — do NOT unify):
```
For each plugin: `/module-add` → wire `getVST3ClientExtensions()` → add per-voice tuning source + `applyPendingTuning` in `startNote` → build + install + pluginval → Dorico quarter-sharp smoke test.

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

**verify:**
- `grep -c "^### 2a\. Dorico Delegation (Phase B per-plugin)$" .claude/commands/generalize-microtones.md` returns `1`
- `grep -c "sequential — one Task() per plugin" .claude/commands/generalize-microtones.md` returns `>= 1` (D3 carried)
- `grep -c "O-IntonationPad" .claude/commands/generalize-microtones.md` returns `>= 1` (8-plugin cohort landed, distinct from publishing's 7)
- Order check: `grep -nE "^### (2\.|2a\.|3\.) " .claude/commands/generalize-microtones.md` must list `### 2.` → `### 2a.` → `### 3.` in that order.

**done:** One atomic commit on current branch touching exactly `.claude/commands/generalize-microtones.md`. Commit subject: `feat(dorico-agent): wire generalize-microtones Phase B to delegate to dorico-agent`.

---

## Task 5 — Cross-reference `gui-agent.md` (NOT a delegation rule)

**files:** `.claude/agents/gui-agent.md`

**action:** Insert `## Dorico Delegation` subsection (level 2 — cross-reference callout, NOT a delegation rule) between the `</required_reading>` close-tag and the `<template_library>` open-tag. Single Edit. No body rewrites elsewhere.

**old_string** (verbatim anchor — only one `</required_reading>` and one `<template_library>` in file):
```
8. **Cross-platform WebView best practices:** `research/cross-platform-webview-best-practices.md` - CRITICAL for Windows compatibility
</required_reading>

<template_library>
```

**new_string** (anchor + inserted block — "return to invoker recommending" phrasing because gui-agent has NO Task tool):
```
8. **Cross-platform WebView best practices:** `research/cross-platform-webview-best-practices.md` - CRITICAL for Windows compatibility
</required_reading>

## Dorico Delegation

**Cross-reference (NOT a delegation rule).** When editing `createParameterLayout()` for plugins in the microtonal cohort, the parameter defaults you set are **Layer 2 of the 3-layer Dorico keyswitch routing stack**. Wrong defaults silently absorb correctly-routed Dorico keyswitches with no error.

**Trigger:** any edit to `createParameterLayout()` involving these parameter IDs (or their per-plugin equivalents): `ks_enabled`, `technique_count`, `cc_select_enabled`, `pc_enabled`, KS-range gates.

**Required pattern (from `critical_dorico_keyswitch_routing.md` Layer 2):**
- `ks_enabled` MUST default `true` (gates that default false silently absorb KS).
- `technique_count` MUST default to actual technique count, NOT `1` (clamps would route every KS to slot 0).
- Same rule applies to `cc_select_enabled`, `pc_enabled`.

**When in doubt:** consult `dorico-agent` before changing these defaults. Do NOT spawn it from gui-agent — return to invoker (plugin-workflow) recommending a separate `Task(subagent_type="dorico-agent", ...)` invocation.

**Reference docs:** `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_keyswitch_routing.md`; `.claude/agents/dorico-agent.md` (Landmine 4).

<template_library>
```

**verify:**
- `grep -c "^## Dorico Delegation$" .claude/agents/gui-agent.md` returns `1`
- `grep -c "Cross-reference (NOT a delegation rule)" .claude/agents/gui-agent.md` returns `>= 1` (cross-ref vs. delegation distinction preserved)
- `grep -c "Layer 2 of the 3-layer Dorico keyswitch routing stack" .claude/agents/gui-agent.md` returns `>= 1`
- `grep -c "Return to invoker\|return to invoker" .claude/agents/gui-agent.md` returns `>= 1` (gui-agent has no Task tool — handoff phrasing correct)
- Tag adjacency check: `grep -nE "^</required_reading>$|^## Dorico Delegation$|^<template_library>$" .claude/agents/gui-agent.md` must list `</required_reading>` → `## Dorico Delegation` → `<template_library>` in that order.

**done:** One atomic commit on current branch touching exactly `.claude/agents/gui-agent.md`. Commit subject: `feat(dorico-agent): wire gui-agent KS-routing cross-ref to dorico-agent`.

---

## Task 6 — Cross-reference `dsp-agent.md` (NOT a delegation rule)

**files:** `.claude/agents/dsp-agent.md`

**action:** Insert `## Dorico Delegation` subsection (level 2 — cross-reference callout, NOT a delegation rule) between the `</required_reading>` close-tag and the `<complexity_aware>` open-tag. Single Edit. No body rewrites elsewhere.

**old_string** (verbatim anchor — only one `</required_reading>` and one `<complexity_aware>` in file):
```
5. Modern juce::dsp API: Use ProcessSpec/AudioBlock/ProcessContext (not old API)
</required_reading>

<complexity_aware>
```

**new_string** (anchor + inserted block — "return to invoker" phrasing because dsp-agent has NO Task tool; cites `Skill("spike-findings-VST-development")` Pattern 6 + Landmine 6 as canonical source for trigger-order rule):
```
5. Modern juce::dsp API: Use ProcessSpec/AudioBlock/ProcessContext (not old API)
</required_reading>

## Dorico Delegation

**Cross-reference (NOT a delegation rule).** When implementing or editing voice-allocation / `startNote` code for plugins that use VST3 Note Expression for microtonal playback, **apply NE tuning to `currentFrequency` BEFORE the DSP model's `trigger(...)` call**. Wrong order = first sample renders at untuned pitch → audible zipper at attack.

**Trigger:** edits to per-voice `startNote` / `triggerVoice` / equivalent that touch `currentFrequency` and any subsequent `model.trigger(...)` / oscillator `setFrequency(...)` call sequence.

**Required pattern (from spike-findings landmine):**
1. Compute final `currentFrequency` (apply pending NE tuning from `noteId → cents-deviation` map).
2. THEN call `trigger(...)` / `setFrequency(...)`.

Spike validated this order on O-Lyrica end-to-end. Other plugins consuming the shared `dsp/note-expression` module inherit the constraint.

**When in doubt:** consult `dorico-agent` before re-ordering or refactoring. Do NOT spawn it from dsp-agent — dsp-agent has no Task tool. Return to invoker (plugin-workflow) recommending a separate `Task(subagent_type="dorico-agent", ...)`.

**Reference docs:** `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/project_o_lyrica_spike_reference.md`; `.claude/agents/dorico-agent.md`; `Skill("spike-findings-VST-development")` (Pattern 6 + Landmine 6 — the trigger-order rule).

<complexity_aware>
```

**verify:**
- `grep -c "^## Dorico Delegation$" .claude/agents/dsp-agent.md` returns `1`
- `grep -c "Cross-reference (NOT a delegation rule)" .claude/agents/dsp-agent.md` returns `>= 1`
- `grep -c "BEFORE the DSP model's \`trigger" .claude/agents/dsp-agent.md` returns `>= 1` (load-bearing trigger-order rule preserved)
- `grep -c "Return to invoker\|return to invoker" .claude/agents/dsp-agent.md` returns `>= 1` (dsp-agent has no Task tool)
- Tag adjacency check: `grep -nE "^</required_reading>$|^## Dorico Delegation$|^<complexity_aware>$" .claude/agents/dsp-agent.md` must list `</required_reading>` → `## Dorico Delegation` → `<complexity_aware>` in that order.

**done:** One atomic commit on current branch touching exactly `.claude/agents/dsp-agent.md`. Commit subject: `feat(dorico-agent): wire dsp-agent NE-tuning cross-ref to dorico-agent`.

---

## Done Criteria (whole quick task)

- 6 atomic commits on current branch (one per task, in canonical order Task 1 → Task 6).
- Each commit touches **exactly one file** under `.claude/`:
  1. `.claude/agents/troubleshoot-agent.md`
  2. `.claude/skills/plugin-improve/SKILL.md`
  3. `.claude/skills/plugin-publishing/SKILL.md`
  4. `.claude/commands/generalize-microtones.md`
  5. `.claude/agents/gui-agent.md`
  6. `.claude/agents/dsp-agent.md`
- No other files modified by these 6 code commits. (Updates to `.planning/quick/260505-mri-.../{PLAN,CONTEXT,RESEARCH,SUMMARY}.md` and `.planning/STATE.md` are handled by the orchestrator in a separate docs commit, not included in the 6.)
- Per-file verify commands all pass (see each task's `verify:` block).
- All 6 commits use the `feat(dorico-agent): wire <short-target> to delegate to dorico-agent` subject pattern.
- Cross-cutting integrity: insertion-only — no body rewrites; existing structure of each file preserved; D1 (block format), D2 (advisory-only for plugin-publishing), and D3 (sequential per-plugin Task() in generalize-microtones) all carried verbatim into the relevant blocks.
