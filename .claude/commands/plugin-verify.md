---
name: plugin-verify
description: Validate stage goal achievement through goal-backward analysis (GSD verify phase)
skill: plugin-phases
args: "[plugin_name?] [stage?]"
---

# /plugin-verify

**Purpose:** Execute the VERIFY phase of the GSD cycle for a completed plugin stage. Confirms the stage goal was achieved through goal-backward analysis and produces VERIFICATION.md.

## Usage

```
/plugin-verify [plugin_name] [stage]      # Specific plugin and stage
/plugin-verify [stage]                    # Focused plugin, specific stage
/plugin-verify                            # Focused plugin, current stage
```

## Arguments

- `plugin_name` - Plugin to verify (optional, defaults to focused)
- `stage` - Stage: `0-ideation`, `1-foundation`, `2-dsp`, `3-gui`, `4-polish`

## Examples

```
/plugin-verify O-IntonationPad 2-dsp     # Verify DSP stage
/plugin-verify 2-dsp                     # Use focused plugin
/plugin-verify                           # Use focused plugin and current stage
```

## What This Command Does

1. **Loads stage context:**
   - CONTEXT.md (what was discussed/decided)
   - PLAN.md (what was planned)
   - SUMMARY.md (what was implemented)
   - BRIEF.md (original vision)
   - REQUIREMENTS.md (formal requirements with acceptance criteria)

2. **Runs goal-backward verification:**
   - What did we set out to achieve? (from CONTEXT.md, PLAN.md)
   - What did we actually deliver? (from SUMMARY.md, code inspection)
   - Does delivery match goals?

3. **Checks requirements for this stage:**
   - Loads REQUIREMENTS.md
   - Filters requirements where `verifiedAt` matches current stage
   - Checks acceptance criteria for each requirement
   - Updates requirement status (pending → complete/partial)

3. **Runs automated checks (stage-dependent):**
   - Stage 1: Build passes, parameters exist, pass-through works
   - Stage 2: DSP processes audio, no artifacts, CPU acceptable
   - Stage 3: UI renders, controls work, parameter binding verified
   - Stage 4: pluginval passes, installer works

4. **Produces:**
   - `plugins/[Name]/.planning/stages/[N]-[name]/VERIFICATION.md`

5. **Updates:**
   - `plugins/[Name]/.planning/STATUS.md` with stage complete
   - `plugins/[Name]/.planning/REQUIREMENTS.md` requirement statuses (pending → complete/partial)

## Output: VERIFICATION.md

```markdown
# Stage [N]: [StageName] - Verification

## Verification Date

[YYYY-MM-DD]

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md)

1. [Goal 1]
2. [Goal 2]
3. [Goal 3]

### Deliverables (from SUMMARY.md)

1. [What was built for Goal 1]
2. [What was built for Goal 2]
3. [What was built for Goal 3]

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| [Goal 1] | ✅ Achieved | [How verified] |
| [Goal 2] | ✅ Achieved | [How verified] |
| [Goal 3] | ⚠️ Partial | [What's missing] |

## Requirements Verification

**Stage:** [current stage]
**Requirements for this stage:** [N] total ([M] must, [K] should, [L] nice)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-01: [Description] | must | ✅ Complete | All criteria met |
| DSP-01: [Description] | must | ✅ Complete | All criteria met |
| PERF-01: Real-time safe | must | ✅ Complete | No allocations in processBlock |
| UI-01: [Description] | should | ⏸️ Deferred | Verified at stage-3 |

**Requirements Summary:**
- ✅ Complete: [N]
- ⚠️ Partial: [N]
- ⏸️ Deferred (later stage): [N]
- ❌ Failed: [N]

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build | ✅ Pass | Clean compile, no warnings |
| [Stage-specific check] | ✅ Pass | [Details] |
| [Stage-specific check] | ✅ Pass | [Details] |

## Human Verification

- [ ] [Manual check 1]
- [ ] [Manual check 2]

## Issues Found

- [Issue 1]: [Description and resolution]
- [Issue 2]: [Description and resolution]

## Stage Verdict

**Status:** ✅ VERIFIED / ⚠️ PARTIAL / ❌ FAILED

**Ready for next stage:** Yes / No

**Blockers (if any):**
- [Blocker 1]
```

## Stage-Specific Checks

### Stage 1 (Foundation)
```bash
# Build check
ninja [PluginName]_VST3 [PluginName]_AU

# Parameter check
grep -l "AudioParameterFloat\|AudioParameterChoice" plugins/[Name]/Source/*.cpp

# Pass-through check (manual)
# Load in DAW, verify audio passes through unchanged
```

### Stage 2 (DSP)
```bash
# Build check
ninja [PluginName]_VST3 [PluginName]_AU

# CPU check (manual)
# Process audio, monitor CPU usage

# Artifact check (manual)
# Listen for clicks, pops, aliasing
```

### Stage 3 (UI)
```bash
# Build check
ninja [PluginName]_VST3 [PluginName]_AU [PluginName]_Standalone

# Open standalone, verify UI renders
open "build/plugins/[Name]/[Name]_artefacts/Release/Standalone/[Name].app"
```

### Stage 4 (Polish)
```bash
# pluginval check
pluginval --strictness-level 10 --validate "build/plugins/[Name]/[Name]_artefacts/Release/VST3/[Name].vst3"

# AU validation
auval -v aufx [code] [manu]
```

## MANDATORY Completion Handoff

**After completing the verify phase, you MUST present the two-step handoff and STOP. Do NOT skip this.**

**If verification PASSES — stage is complete, handoff to next stage:**

```
━━━ STAGE COMPLETE ━━━

**[PluginName]** — Stage [N] ([StageName]) verified

| Phase | Status |
|-------|--------|
| discuss | ✓ |
| research | ✓ |
| plan | ✓ |
| execute | ✓ |
| verify | ✓ |

━━━━━━━━━━━━━━━━━━━━━━━━━━━

## ▶ Next Up

**Stage [N+1]: [NextStageName]** — [objective]

**Step 1:** `/clear` — fresh context window
**Step 2:** `/implement [PluginName]`

━━━━━━━━━━━━━━━━━━━━━━━━━━━

**Also available:**

- `/test [PluginName]` → Run additional tests
- `/plugin-status [PluginName]` → Review stage artifacts
- Save for later (handoff file created)

━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**If this is Stage 4 (final stage) and verification passes:**

```
━━━ PLUGIN COMPLETE ━━━

**[PluginName]** — All stages verified

**Step 1:** `/clear` — fresh context window
**Step 2:** `/install-plugin [PluginName]`

━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**Rules:**
1. Always include full plugin name in commands (copy-paste ready)
2. Always include `/clear` as Step 1 before the next command
3. Present the handoff and STOP — do NOT auto-invoke the next stage
4. Do NOT relegate `/clear` to a footnote or omit it

See: `.claude/references/handoff-protocol.md`

## Integration

**Follows:** SUMMARY.md (execute phase output)
**Precedes:** Next stage CONTEXT.md (discuss phase)
**Updates:** STATUS.md to mark stage complete
**Triggers:** Decision menu for next action
