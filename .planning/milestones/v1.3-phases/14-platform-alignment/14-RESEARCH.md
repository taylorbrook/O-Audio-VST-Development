# Phase 14: Platform Alignment - Research

**Researched:** 2026-02-08
**Domain:** Claude Code agent/skill infrastructure migration to Opus 4.6
**Confidence:** HIGH

## Summary

Phase 14 requires updating the Plugin Freedom System's agent definitions, skill documentation, hook scripts, and validator Python files to work cleanly on Claude Opus 4.6. The codebase audit identified **four categories of changes** across 60+ files:

1. **Thinking config deprecation** (PLAT-01): 11 agent/skill files reference `extended thinking`, `budget_tokens`, or explicit thinking configuration. Opus 4.6 uses adaptive thinking (`thinking: {type: "adaptive"}`) and the `effort` parameter instead. However, Claude Code's agent frontmatter does NOT have an `effort` field -- effort is controlled at the API/session level, not per-agent. The `<extended_thinking>` XML tags in agent prompts are natural-language instructions (not API configuration), so they need to be reworded to remove stale references but their intent (think deeply) is preserved by Opus 4.6's adaptive thinking.

2. **Stale path references** (PLAT-02): 18+ files reference `.ideas/` and `.continue-here.md` -- the legacy planning directory convention. Half of existing plugins (10/20) still use `.ideas/` while the other half have migrated to `.planning/`. Hook scripts, validators, and reconciliation rules all need updating.

3. **Assistant message prefills** (PLAT-03): Opus 4.6 returns a 400 error on prefilled assistant messages. The audit found zero actual prefill patterns in the codebase -- the only mention is a comment in `validate-resource-accountability.py`. This requirement is already satisfied.

4. **Model selection removal** (PLAT-04, PLAT-07): 11 agent files have `model: sonnet` or `model: opus` frontmatter. The GSD system has a separate `model-profiles.md` with Sonnet/Opus/Haiku selection tables. The decision is to remove binary model switching entirely and replace with effort-tuned profiles via a central `agent-profiles.json`.

**Primary recommendation:** Execute an audit-then-fix approach in 4 batches (one per category), followed by canary testing of O-SimpleReverb and O-AnalogEQ.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Effort Level Mapping
- 4-tier system: max / high / medium / low
- All agents stay on Opus 4.6 -- effort level is the only tuning knob, no model switching
- Central config file (e.g., agent-profiles.json) with per-agent override capability
- Max-tier agents (DSP, research-planning) always run at max -- no complexity-adaptive scaling
- Remove the binary Sonnet/Opus model selection infrastructure entirely (don't rewire, delete it)

#### Migration Strategy
- Audit-then-fix: first pass catalogs all deprecation issues (thinking config, stale paths, prefilled messages, model selection), second pass fixes them in batches
- After all changes, verify with canary test

#### Canary Testing Scope
- Primary canary: O-SimpleReverb (build + validate)
- Secondary spot-check: one WebView-based plugin (e.g., O-AnalogEQ) to verify WebView paths still work
- Both canaries run after all Phase 14 changes are complete

#### Stale Path Handling
- Migrate content from .ideas/ and .continue-here.md into each plugin's own .planning/ folder before removing references
- Audit for other stale paths/deprecated conventions beyond .ideas/ and .continue-here.md -- discover as part of the Phase 14 audit pass
- Update all script references to use only .planning/ paths

### Claude's Discretion
- Canary test cadence (after each fix category vs. only at end) -- based on risk assessment
- Whether to create a reusable canary-test.sh script or run ad-hoc commands
- Failure policy (revert vs. debug in place) -- based on severity
- Whether to add .gitignore rules to prevent stale paths from reappearing
- Audit output format (structured AUDIT.md vs. direct feed into plan)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

## Standard Stack

This phase does not introduce new libraries. It modifies configuration files, markdown agent definitions, shell scripts, and Python validators.

### Core
| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| Claude Code CLI | Current | Agent/skill runtime | Platform we are migrating on |
| Claude Opus 4.6 | `claude-opus-4-6` | Model for all agents | Target platform |
| Bash/Python | System | Hook scripts and validators | Existing infrastructure |

### Key API Changes (Opus 4.6)
| Old | New | Impact |
|-----|-----|--------|
| `thinking: {type: "enabled", budget_tokens: N}` | `thinking: {type: "adaptive"}` + `effort` param | Deprecated; still functional but will be removed |
| `model: sonnet` / `model: opus` in frontmatter | All agents inherit or use single model | Remove model switching |
| Assistant message prefills | Not supported (400 error) | Breaking change -- already clear in codebase |
| `budget_tokens` | `effort: max/high/medium/low` | API-level control, not per-agent frontmatter |

## Architecture Patterns

### Pattern 1: Effort Profile Central Config

**What:** A single `agent-profiles.json` file that maps agent names to effort levels. This is a documentation/convention file, not API configuration -- Claude Code does NOT have an `effort` frontmatter field.

**Critical finding:** The Claude Code subagent frontmatter supports `model` (sonnet/opus/haiku/inherit) but does NOT support an `effort` field. Effort is an API-level parameter (`output_config.effort`), not a per-agent setting. In Claude Code CLI sessions, effort is controlled at the session level, not per-subagent.

**Implication for the plan:** The `agent-profiles.json` serves as a **human-readable reference document** and **convention guide** for the orchestrator prompts. When orchestrators spawn subagents via Task, they cannot directly set effort -- the effort level is determined by the session/API configuration. The profiles file documents the INTENDED effort for each agent role so the user can configure their session appropriately.

**When to use:** All orchestrator and skill documentation that currently references model selection.

**Example structure:**
```json
{
  "_comment": "Agent effort profiles for Opus 4.6. All agents run on Opus 4.6.",
  "_effort_levels": "max > high > medium > low",
  "profiles": {
    "dsp-agent":                { "effort": "max",    "rationale": "Complex DSP algorithm design" },
    "research-planning-agent":  { "effort": "max",    "rationale": "Deep research and architecture" },
    "troubleshoot-agent":       { "effort": "max",    "rationale": "Complex debugging synthesis" },
    "validation-agent":         { "effort": "high",   "rationale": "Semantic judgment layer" },
    "foundation-shell-agent":   { "effort": "medium", "rationale": "Template-based code generation" },
    "gui-agent":                { "effort": "medium", "rationale": "Follows established patterns" },
    "ui-design-agent":          { "effort": "medium", "rationale": "Creative but structured" },
    "ui-finalization-agent":    { "effort": "medium", "rationale": "Scaffolding generation" },
    "aesthetics-agent":         { "effort": "medium", "rationale": "Specification only" },
    "music-theory-agent":       { "effort": "high",   "rationale": "Specialized domain reasoning" },
    "polish-agent":             { "effort": "medium", "rationale": "Follows plan directives" },
    "critic-dsp":               { "effort": "high",   "rationale": "Nuanced safety evaluation" },
    "critic-ui":                { "effort": "high",   "rationale": "Visual quality judgment" }
  }
}
```

### Pattern 2: Stale Path Migration

**What:** Dual-path lookup during transition, then cleanup.

**Current state:**
- 10 plugins use `.ideas/` (O-AnalogSaturation, O-MultiBandCompressor, O-Comp, O-Marimba, O-DigiDelay, O-AnalogEQ, O-SimpleReverb, O-Lyrica, O-Polystutter, O-Tremolo)
- 10 plugins use `.planning/` (O-GrainScatter, O-Bass, O-SpectralShaper, O-FreqPulse, O-Chorus, O-DigiDelay, O-Bells, O-Freeze, O-Detune, O-IntonationPad)
- Note: O-DigiDelay appears in both lists (has both directories)
- 4 plugins have `.continue-here.md` (O-MultiBandCompressor, O-AnalogEQ, O-SimpleReverb, O-Polystutter)

**Migration approach:** For each `.ideas/` plugin, move files to `.planning/` (creating the directory if needed). For `.continue-here.md` files, migrate content into `.planning/STATUS.md` or equivalent. Then update all script/hook references to use `.planning/` only.

### Pattern 3: Agent Frontmatter Cleanup

**What:** Remove `model:` field from all agent frontmatter (or set to `inherit`). Remove `<model_selection>`, `<model_and_thinking>`, `<extended_thinking>` sections from agent body text.

**Current frontmatter model fields:**
| Agent | Current `model:` | New `model:` |
|-------|-------------------|--------------|
| dsp-agent | sonnet | (remove or inherit) |
| research-planning-agent | sonnet | (remove or inherit) |
| foundation-shell-agent | sonnet | (remove or inherit) |
| gui-agent | sonnet | (remove or inherit) |
| ui-design-agent | sonnet | (remove or inherit) |
| ui-finalization-agent | sonnet | (remove or inherit) |
| aesthetics-agent | sonnet | (remove or inherit) |
| music-theory-agent | sonnet | (remove or inherit) |
| troubleshoot-agent | opus | (remove or inherit) |
| validation-agent | opus | (remove or inherit) |
| polish-agent | (none -- no frontmatter) | (no change needed) |
| critic-dsp | opus | (remove or inherit) |
| critic-ui | opus | (remove or inherit) |

### Anti-Patterns to Avoid
- **Adding `effort:` to agent frontmatter:** Claude Code does not support this field. It will be silently ignored, creating a false sense of configuration.
- **Leaving model: sonnet/opus in frontmatter:** These will still work but contradict the "all agents on Opus 4.6" design. Remove them to avoid confusion.
- **Rewriting extended_thinking tags as effort instructions:** Agent body text cannot control API-level effort. Instead, remove the tags and rely on the session effort level + adaptive thinking.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Per-agent effort control | Custom API wrapper | Session-level effort + documentation | Claude Code CLI doesn't support per-agent effort in frontmatter |
| Thinking budget management | Custom budget_tokens logic | Adaptive thinking (default in Opus 4.6) | Deprecated; adaptive handles it automatically |
| Path migration scripting | Complex regex replacements | Simple find/mv + sed for scripts | 20 plugins, straightforward moves |

**Key insight:** The effort parameter is a session/API-level setting, not a per-agent setting in Claude Code. The central config file is a convention document, not runtime configuration.

## Common Pitfalls

### Pitfall 1: Confusing Agent Prompt Text with API Configuration
**What goes wrong:** Developers treat `<extended_thinking>` XML tags in agent markdown as API configuration. They are actually natural-language instructions to the model.
**Why it happens:** The tags look like configuration but are just prompt formatting.
**How to avoid:** Remove the tags entirely. Opus 4.6 with adaptive thinking automatically decides when to think deeply. The prompt text "Think deeply about X" works fine without special tags.
**Warning signs:** Agent still has `<extended_thinking>` tags after migration.

### Pitfall 2: Forgetting Script Path References
**What goes wrong:** Agent definitions and skill docs are updated to `.planning/` but hook scripts and Python validators still reference `.ideas/`.
**Why it happens:** The grep audit shows references spread across 18+ files in 5+ directories.
**How to avoid:** Use the complete file inventory in the Architecture Patterns section. Update ALL files, not just the obvious ones.
**Warning signs:** Validators fail silently (can't find contract files at `.ideas/` path).

### Pitfall 3: Breaking the GSD Model Profile System
**What goes wrong:** The GSD system (`model-profiles.md`, `model-profile-resolution.md`) has its own Sonnet/Opus/Haiku selection logic. If only the PFS agents are updated but the GSD profiles are left intact, the systems will conflict.
**Why it happens:** The GSD system is in `~/.claude/get-shit-done/` (user-level), separate from the project.
**How to avoid:** Update `model-profiles.md` to reflect the new all-Opus-4.6 paradigm. The GSD profiles table should show Opus 4.6 for all agents (or be simplified).
**Warning signs:** GSD workflows still spawn Sonnet/Haiku subagents.

### Pitfall 4: Stale Co-Authored-By Lines
**What goes wrong:** Commit templates still reference "Claude Opus 4.5" instead of "Claude Opus 4.6".
**Why it happens:** Two files in plugin-publishing skill hardcode the model name.
**How to avoid:** Update both `plugin-publishing/SKILL.md` and `plugin-publishing/assets/changelog-entry-template.md`.
**Warning signs:** New commits show wrong model attribution.

### Pitfall 5: Incomplete Plugin Content Migration
**What goes wrong:** `.ideas/` directory is referenced in scripts, but actual plugin content is not moved to `.planning/`.
**Why it happens:** Updating references without migrating data.
**How to avoid:** Move content FIRST (for all 10 `.ideas/` plugins), THEN update references.
**Warning signs:** Hooks crash on missing files after reference update.

## Code Examples

### Example 1: Agent Frontmatter Before/After

**Before (dsp-agent.md):**
```yaml
---
name: dsp-agent
description: Implement audio processing...
tools: Read, Edit, Write, mcp__context7__resolve-library-id, mcp__context7__get-library-docs
model: sonnet
color: yellow
---
```

**After:**
```yaml
---
name: dsp-agent
description: Implement audio processing...
tools: Read, Edit, Write, mcp__context7__resolve-library-id, mcp__context7__get-library-docs
color: yellow
---
```

The `model:` line is removed entirely. The agent inherits the session model (Opus 4.6).

### Example 2: Removing Extended Thinking Section

**Before (dsp-agent.md body):**
```markdown
<model_selection>
## Model Selection

**Orchestrator responsibility:** The plugin-workflow skill selects the model based on complexity score from ROADMAP.md:

- **Complexity >=4:** Invokes dsp-agent with Opus model + extended thinking
- **Complexity <=3:** Invokes dsp-agent with Sonnet model (default)

**Note:** This subagent does not self-select models.
</model_selection>
```

**After:** Delete entire section. No replacement needed -- the agent always runs on Opus 4.6 at the session's effort level.

### Example 3: Removing Extended Thinking Tags from research-planning-agent

**Before:**
```markdown
<extended_thinking>
Think deeply to thoroughly analyze creative brief and detect complexity tier:
...
Output: Tier (1-6) and research depth (QUICK/MODERATE/DEEP)
</extended_thinking>
```

**After:**
```markdown
Thoroughly analyze the creative brief to detect complexity tier:
...
Output: Tier (1-6) and research depth (QUICK/MODERATE/DEEP)
```

The `<extended_thinking>` wrapper is removed. The instruction "thoroughly analyze" is preserved as plain text. Opus 4.6 adaptive thinking handles the rest.

### Example 4: PreCompact.sh Path Migration

**Before:**
```bash
if [ -f "$PLUGIN/.ideas/creative-brief.md" ]; then
    echo "--- creative-brief.md ---"
    cat "$PLUGIN/.ideas/creative-brief.md"
fi
```

**After:**
```bash
if [ -f "$PLUGIN/.planning/BRIEF.md" ]; then
    echo "--- BRIEF.md ---"
    cat "$PLUGIN/.planning/BRIEF.md"
fi
```

### Example 5: Deep Research Level References

**Before (deep-research/SKILL.md):**
```markdown
## Level 1: Quick Check (5-10 min, Sonnet, no extended thinking)
## Level 2: Moderate Investigation (15-30 min, Sonnet, no extended thinking)
## Level 3: Deep Research (30-60 min, Opus, extended thinking 15k budget)

**Model requirements:** claude-opus-4-1-20250805 with extended-thinking (15k budget)
```

**After:**
```markdown
## Level 1: Quick Check (5-10 min, low effort)
## Level 2: Moderate Investigation (15-30 min, medium effort)
## Level 3: Deep Research (30-60 min, max effort)

**Effort:** max (deepest reasoning for novel problems)
```

### Example 6: Subagent Invocation in Workflow Docs

**Before (stage-2-dsp.md):**
```typescript
const model = complexityScore >= 4 ? "opus" : "sonnet";

const phaseResult = Task({
  subagent_type: "dsp-agent",
  description: `Stage 2...`,
  model: model
});
```

**After:**
```typescript
const phaseResult = Task({
  subagent_type: "dsp-agent",
  description: `Stage 2...`
});
```

Model parameter removed entirely -- agent inherits session model.

## Complete File Inventory

### Category 1: Thinking Config (PLAT-01)

Files containing `extended_thinking`, `budget_tokens`, or explicit thinking configuration:

| File | Lines | What to Change |
|------|-------|----------------|
| `.claude/agents/dsp-agent.md` | 20, 221, 1225-1242 | Remove `<model_selection>`, `<model_and_thinking>` sections |
| `.claude/agents/research-planning-agent.md` | 146-166, 170-180, 188-199, 224-233, 269-278, 295-309, 783-790, 980 | Remove all `<extended_thinking>` tags, keep inner instructions |
| `.claude/agents/troubleshoot-agent.md` | 335-348, 699, 728 | Remove "Use Extended Thinking" section and references |
| `.claude/skills/deep-research/SKILL.md` | 94, 143-144, 147, 160, 165, 177, 189, 193 | Replace Sonnet/Opus+thinking with effort levels |
| `.claude/skills/deep-research/BOUNDARIES.md` | 14, 27 | Update model references |
| `.claude/skills/deep-research/references/research-protocol.md` | 76-113, 262, 289-298, 309-319 | Remove model/thinking config, replace with effort |
| `.claude/skills/deep-research/references/example-scenarios.md` | 61, 66 | Update model switch references |
| `.claude/skills/deep-research/assets/level3-report-template.md` | 6-7, 33-34 | Remove model/thinking metadata |
| `.claude/skills/deep-research/assets/research-progress.md` | 57 | Update checklist item |
| `.claude/skills/plugin-planning/archive/stage-0-research.md` | 14, 71, 94-103, 112, 124-135, 167, 176-183, 272, 280-287, 436, 447-454, 565, 578-592 | Archive file -- update or annotate as legacy |
| `.claude/skills/plugin-planning/archive/stage-1-planning.md` | 14 | Archive file -- update or annotate as legacy |
| `.claude/commands/research.md` | 84, 93, 103, 151-152 | Replace model/thinking references with effort levels |

### Category 2: Model Selection (PLAT-04, PLAT-07)

Files containing `model: sonnet`, `model: opus`, or model selection logic:

| File | What to Change |
|------|----------------|
| `.claude/agents/dsp-agent.md` | Remove `model: sonnet` from frontmatter |
| `.claude/agents/research-planning-agent.md` | Remove `model: sonnet` from frontmatter |
| `.claude/agents/foundation-shell-agent.md` | Remove `model: sonnet` from frontmatter |
| `.claude/agents/gui-agent.md` | Remove `model: sonnet` from frontmatter |
| `.claude/agents/ui-design-agent.md` | Remove `model: sonnet` from frontmatter |
| `.claude/agents/ui-finalization-agent.md` | Remove `model: sonnet` from frontmatter |
| `.claude/agents/aesthetics-agent.md` | Remove `model: sonnet` from frontmatter |
| `.claude/agents/music-theory-agent.md` | Remove `model: sonnet` from frontmatter |
| `.claude/agents/troubleshoot-agent.md` | Remove `model: opus` from frontmatter |
| `.claude/agents/validation-agent.md` | Remove `model: opus` from frontmatter; update body text |
| `.claude/critics/critic-dsp.md` | Remove `model: opus` from frontmatter |
| `.claude/critics/critic-ui.md` | Remove `model: opus` from frontmatter |
| `.claude/skills/deep-research/SKILL.md` | Remove MUST use/NEVER use model directives |
| `.claude/skills/deep-research/references/research-protocol.md` | Remove model: "claude-opus-4-1-20250805" references |
| `.claude/skills/plugin-planning/SKILL.md` | Remove `model="sonnet"` from Task examples |
| `.claude/skills/plugin-planning/references/subagent-invocation.md` | Remove `model="sonnet"` from Task invocation |
| `.claude/skills/plugin-workflow/references/stage-2-dsp.md` | Remove model selection logic |
| `.claude/skills/plugin-improve/SKILL.md` | Update Opus+thinking references |
| `.claude/skills/plugin-improve/references/handoff-protocols.md` | Update model references |
| `.claude/skills/plugin-improve/references/research-detection.md` | Update model references |
| `.claude/skills/plugin-improve/references/investigation-tiers.md` | Update model references |
| `.claude/skills/plugin-publishing/SKILL.md` | Update Co-Authored-By to Opus 4.6 |
| `.claude/skills/plugin-publishing/assets/changelog-entry-template.md` | Update Co-Authored-By to Opus 4.6 |

### Category 3: Stale Paths (PLAT-02)

Files referencing `.ideas/` or `.continue-here.md`:

| File | References | What to Change |
|------|------------|----------------|
| `.claude/hooks/PreCompact.sh` | Lines 20-54 | Replace `.ideas/` with `.planning/`, `.continue-here.md` with `STATUS.md` |
| `.claude/hooks/UserPromptSubmit.sh` | Lines 15-34 | Replace `.continue-here.md` with `.planning/STATUS.md`, `.ideas/` with `.planning/` |
| `.claude/hooks/PostToolUse.sh` | Line 18 | Replace `.ideas/` with `.planning/` |
| `.claude/hooks/validators/validate-parameters.py` | Lines 21, 125 | Replace `.ideas/` with `.planning/` |
| `.claude/hooks/validators/validate-dsp-components.py` | Lines 21, 113 | Replace `.ideas/` with `.planning/` |
| `.claude/hooks/validators/contract_validator.py` | Lines 67-68, 80, 166, 312-317, 325, 361 | Replace `.ideas` with `.planning`, `.continue-here.md` with `STATUS.md` |
| `.claude/hooks/validators/validate-checksums.py` | Lines 5, 22, 25, 85, 99 | Replace `.continue-here.md` references |
| `.claude/hooks/SubagentStop.sh` | (indirect via validators) | No direct changes, but validators it calls need updating |
| `.claude/schemas/subagent-report.json` | Line 155 | Replace `.continue-here.md` reference |
| `.claude/skills/workflow-reconciliation/assets/reconciliation-rules.json` | Lines 5-42, 51, 57, 73, 83 | Replace ALL `.ideas/` with `.planning/`, `.continue-here.md` with `STATUS.md` |
| `.claude/skills/plugin-workflow/references/precondition-checks.sh` | Line 6 | Replace `.ideas` with `.planning` |
| `.claude/utils/sync-brief-from-mockup.sh` | Line 18 | Replace `.ideas` with `.planning` |

**Plugin content to migrate (move files):**
| Plugin | Has `.ideas/` | Has `.planning/` | Action |
|--------|--------------|------------------|--------|
| O-AnalogSaturation | Yes | No | Move `.ideas/` to `.planning/` |
| O-MultiBandCompressor | Yes | No | Move `.ideas/` to `.planning/`, migrate `.continue-here.md` |
| O-Comp | Yes | No | Move `.ideas/` to `.planning/` |
| O-Marimba | Yes | No | Move `.ideas/` to `.planning/` |
| O-DigiDelay | Yes | Yes | Merge `.ideas/` into `.planning/` |
| O-AnalogEQ | Yes | No | Move `.ideas/` to `.planning/`, migrate `.continue-here.md` |
| O-SimpleReverb | Yes | No | Move `.ideas/` to `.planning/`, migrate `.continue-here.md` |
| O-Lyrica | Yes | No | Move `.ideas/` to `.planning/` |
| O-Polystutter | Yes | No | Move `.ideas/` to `.planning/`, migrate `.continue-here.md` |
| O-Tremolo | Yes | No | Move `.ideas/` to `.planning/` |

### Category 4: Assistant Prefills (PLAT-03)

**Status: ALREADY SATISFIED.** The codebase audit found zero assistant message prefill patterns. The only related text is a comment in `validate-resource-accountability.py` (line 109) which is just a code comment, not a prefill.

### Additional Items Discovered

| File | Issue | Category |
|------|-------|----------|
| `.claude/skills/plugin-publishing/SKILL.md` | "Co-Authored-By: Claude Opus 4.5" | Stale model name |
| `.claude/skills/plugin-publishing/assets/changelog-entry-template.md` | "Co-Authored-By: Claude Opus 4.5" | Stale model name |
| `~/.claude/get-shit-done/references/model-profiles.md` | Sonnet/Opus/Haiku selection table | GSD system model selection (user-level) |
| `~/.claude/get-shit-done/references/model-profile-resolution.md` | Model profile resolution logic | GSD system model selection (user-level) |
| `.planning/config.json` | `"model_profile": "quality"` | GSD config -- may need updating |
| `.claude/utils/sync-brief-from-mockup.sh` | Hardcoded wrong user path + `.ideas` refs | Both stale path and wrong username |

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `thinking: {type: "enabled", budget_tokens: N}` | `thinking: {type: "adaptive"}` | Opus 4.6 (Feb 2026) | Deprecated; remove explicit config |
| `budget_tokens: 15000` | `effort: max/high/medium/low` | Opus 4.6 (Feb 2026) | API-level only, not per-agent |
| `model: sonnet` / `model: opus` in frontmatter | Remove `model:` (inherits session) | Phase 14 decision | All agents on Opus 4.6 |
| Assistant message prefills | Not supported | Opus 4.6 (Feb 2026) | Breaking change (400 error) |
| `.ideas/` directory | `.planning/` directory | v1.2 migration (ongoing) | Half complete; 10 plugins remaining |
| `.continue-here.md` | `.planning/STATUS.md` | v1.2 migration (ongoing) | 4 plugins still have legacy files |
| `claude-opus-4-1-20250805` model ID | `claude-opus-4-6` | Feb 2026 | Update hardcoded model IDs |

**Deprecated/outdated:**
- `extended_thinking` XML tags in agent prompts: No API effect, confusing
- `budget_tokens` parameter: Deprecated on Opus 4.6, will be removed
- Binary Sonnet/Opus model selection: Replaced by single model + effort tuning
- `interleaved-thinking-2025-05-14` beta header: Ignored on Opus 4.6

## Discretion Recommendations

### Canary Test Cadence
**Recommendation: Run canary tests ONCE after all changes, not per-category.**
- Rationale: The changes are config/documentation edits, not code changes. The risk of breaking a build is limited to path changes in hook scripts. Running canary after each of 4 categories adds overhead without proportional safety benefit.
- Exception: If a path migration script fails, fix and retest immediately.

### Reusable canary-test.sh Script
**Recommendation: Create a reusable `canary-test.sh` script.**
- It will be useful beyond Phase 14 (any future system change can use it)
- Script should: build O-SimpleReverb (VST3+AU), validate with `auval`, build O-AnalogEQ (WebView plugin), verify both install correctly
- Store at `.claude/scripts/canary-test.sh`

### Failure Policy
**Recommendation: Debug in place.**
- These are documentation/config changes, not risky code changes
- Git provides easy revert if a batch goes wrong
- Revert entire batch only if canary test fails on something unexpected

### .gitignore Rules
**Recommendation: Add .gitignore rules to prevent `.ideas/` and `.continue-here.md` from reappearing.**
```
# Legacy paths (migrated to .planning/)
plugins/*/.ideas/
plugins/*/.continue-here.md
```

### Audit Output Format
**Recommendation: No separate AUDIT.md -- feed directly into the plan.**
- The file inventory in this RESEARCH.md serves as the audit
- Creating a separate AUDIT.md adds a document to maintain with no consumer
- The planner can reference the inventories directly

## Open Questions

1. **GSD System Updates**
   - What we know: The GSD system at `~/.claude/get-shit-done/references/model-profiles.md` has its own Sonnet/Opus/Haiku model selection table
   - What's unclear: Whether these GSD files should be updated as part of Phase 14 or separately (they are user-level, not project-level)
   - Recommendation: Update them as part of Phase 14 since the model-profiles.md directly affects how GSD workflows spawn subagents. The table should be simplified to show all agents at Opus 4.6 (inherit) with effort recommendations as comments.

2. **sync-brief-from-mockup.sh Wrong Username**
   - What we know: Line 17 has `/Users/lexchristopherson/Developer/plugin-freedom-system/` -- a completely wrong path
   - What's unclear: Whether this script is actively used or a dead artifact
   - Recommendation: Fix the path to use the correct user directory or make it relative. Also update `.ideas` references.

3. **Archive Files in plugin-planning/archive/**
   - What we know: `stage-0-research.md` and `stage-1-planning.md` have extensive extended_thinking references
   - What's unclear: Whether these archived files are ever loaded/referenced at runtime
   - Recommendation: Update them minimally (add a header note that they use legacy conventions) rather than full rewrite, since they are archived.

## Sources

### Primary (HIGH confidence)
- [Anthropic: What's new in Claude 4.6](https://platform.claude.com/docs/en/about-claude/models/whats-new-claude-4-6) - Deprecations, breaking changes, adaptive thinking
- [Anthropic: Effort parameter docs](https://platform.claude.com/docs/en/build-with-claude/effort) - Effort levels, API usage, interaction with thinking
- [Claude Code: Create custom subagents](https://code.claude.com/docs/en/sub-agents) - Supported frontmatter fields (no `effort` field)
- Codebase audit (this session) - Complete file inventory via Grep/Read of `.claude/` directory

### Secondary (MEDIUM confidence)
- [Anthropic: Introducing Claude Opus 4.6](https://www.anthropic.com/news/claude-opus-4-6) - Feature overview

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Direct codebase audit + official Anthropic documentation
- Architecture: HIGH - Verified Claude Code frontmatter fields against official docs; effort parameter behavior confirmed
- Pitfalls: HIGH - All identified from actual codebase patterns, not hypothetical
- File inventory: HIGH - Complete grep-based audit of all `.claude/` files

**Research date:** 2026-02-08
**Valid until:** 2026-03-08 (30 days -- stable platform, unlikely to change)
