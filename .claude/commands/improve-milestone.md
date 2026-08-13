---
name: improve-milestone
description: Complex improvements with GSD-style phase cycles
argument-hint: "[PluginName] [description?]"
---

# /improve-milestone

When user runs `/improve-milestone [PluginName] [description?]`, execute a 5-phase improvement cycle with context clearing between phases.

## Preconditions

<preconditions enforcement="blocking">
  <status_verification target="PLUGINS.md" required="true">
    <check condition="plugin_exists">
      Plugin entry MUST exist in PLUGINS.md
    </check>

    <check condition="status_in(✅ Working, 📦 Installed)">
      Status MUST be ✅ Working OR 📦 Installed

      <on_violation status="🚧 In Development">
        REJECT with message:
        "[PluginName] is still in development.
        Complete the workflow first with /continue [PluginName].
        Cannot use /improve-milestone on in-progress plugins."
      </on_violation>

      <on_violation status="💡 Ideated">
        REJECT with message:
        "[PluginName] is not implemented yet.
        Use /implement [PluginName] to build it first."
      </on_violation>
    </check>
  </status_verification>
</preconditions>

## Routing

**If no plugin name provided:**
1. Read PLUGINS.md and filter for plugins with status ✅ Working or 📦 Installed
2. Display numbered menu: "Which plugin would you like to improve?"
3. Wait for user selection, then proceed

**If plugin name provided:**
1. Check `plugins/[Name]/.planning/STATUS.md` frontmatter for an `activeMilestone` field
2. If active milestone exists → Present resume menu
3. If no active milestone and no description → Prompt for description
4. If plugin + description → Start new milestone

## Milestone State

All state stored in: `plugins/[Name]/.planning/improvements/[slug]/`

Files created per phase:
- CONTEXT.md (discuss)
- RESEARCH.md (research)
- PLAN.md (plan)
- SUMMARY.md (execute)
- VERIFICATION.md (verify)
- STATUS.yaml (tracking)

## Phase Execution

### On Invocation - Determine Current Phase

1. Read `plugins/[Name]/.planning/STATUS.md` frontmatter to check for `activeMilestone`
2. If found, read `plugins/[Name]/.planning/improvements/[slug]/STATUS.yaml`
3. Find current phase (first with status: pending or in_progress)
4. Execute that phase

### Phase 1: Discuss (Skippable)

**Purpose:** Gather requirements through clarifying questions

**Execute:**
1. Read plugin context (BRIEF.md, CHANGELOG.md, PluginProcessor.h header)
2. Ask 4-6 clarifying questions about the improvement
3. Synthesize answers into requirements
4. Generate CONTEXT.md using template: `.claude/skills/improve-milestone/assets/context-template.md`
5. Save to `plugins/[Name]/.planning/improvements/[slug]/CONTEXT.md`
6. Update STATUS.yaml: discuss.status = complete
7. Present handoff (see Handoff Protocol below)

**Skip if:** `--skip-discuss` flag or CONTEXT.md already exists

### Phase 2: Research (Skippable)

**Purpose:** Investigate implementation approach

**Execute:**
1. Read CONTEXT.md
2. Search codebase for relevant patterns
3. Query Context7 for JUCE docs if needed
4. Detect domain (DSP/GUI/polish) using keyword analysis
5. Generate RESEARCH.md using template: `.claude/skills/improve-milestone/assets/research-template.md`
6. Save to `plugins/[Name]/.planning/improvements/[slug]/RESEARCH.md`
7. Update STATUS.yaml: research.status = complete, domain detected
8. Present handoff

**Skip if:** `--skip-research` flag or RESEARCH.md already exists

### Phase 3: Plan (Required)

**Purpose:** Create task breakdown with dependencies

**Execute:**
1. Read CONTEXT.md and RESEARCH.md
2. Determine version bump (PATCH/MINOR/MAJOR)
3. Select execute agent based on domain:
   - DSP keywords (processBlock, filter, buffer) → dsp-agent
   - GUI keywords (WebView, relay, CSS) → gui-agent
   - Polish keywords (preset, pluginval) → general-purpose
   - Mixed → general-purpose
4. Break improvement into atomic tasks with dependencies
5. Generate PLAN.md using template: `.claude/skills/improve-milestone/assets/plan-template.md`
6. Include YAML frontmatter with domain, execute_agent, version info
7. Present plan for user approval (GATE - must approve to continue)
8. Save to `plugins/[Name]/.planning/improvements/[slug]/PLAN.md`
9. Update STATUS.yaml: plan.status = complete, domain, executeAgent, versions
10. Present handoff

### Phase 4: Execute (Required)

**Purpose:** Implement changes per plan

**Pre-execute - Create Backup:**
```bash
mkdir -p backups/[PluginName]/v[currentVersion]
rsync -av --exclude='build/' plugins/[PluginName]/ backups/[PluginName]/v[currentVersion]/
```
HALT if backup fails.

**Execute:**
1. Read PLAN.md for task breakdown
2. Read execute_agent from PLAN.md frontmatter
3. Execute tasks in dependency order
4. After each task, verify completion
5. Generate SUMMARY.md using template: `.claude/skills/improve-milestone/assets/summary-template.md`
6. Save to `plugins/[Name]/.planning/improvements/[slug]/SUMMARY.md`
7. Update STATUS.yaml: execute.status = complete
8. Present handoff

### Phase 5: Verify (Skippable)

**Purpose:** Validate goal achievement

**Execute:**
1. Read CONTEXT.md (requirements) and SUMMARY.md (what was done)
2. Compare requirements against implementation
3. Build: `cd build && ninja [PluginName]_VST3 [PluginName]_AU`
4. Install with cache clearing (per CLAUDE.md)
5. Run pluginval (Level 5 minimum)
6. Generate VERIFICATION.md using template: `.claude/skills/improve-milestone/assets/verification-template.md`
7. Update STATUS.yaml: verify.status = complete, result

**On Success:**
- Update CHANGELOG.md with version entry
- Update CMakeLists.txt version
- Git commit and tag
- Clear `activeMilestone` from the plugin's STATUS.md frontmatter
- Present completion menu

**On Issues Found:**
- Present options: fix, accept with notes, rollback

**Skip if:** `--skip-verify` flag

## Handoff Protocol

After each phase completes, present:

```
✓ [Phase] Complete

[Brief summary]

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │     ✓      │   next     │            │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

## Resume Menu

When milestone exists:
```
Found existing milestone: "[slug]"
Current phase: [phase] ([status])

1. Continue from current phase
2. Restart this phase
3. Start new milestone (abandons current)
4. Other

Choose (1-4): _
```

## STATUS.yaml Format

```yaml
milestone: add-chorus-effect
created: 2026-02-02T10:00:00Z
lastActivity: 2026-02-02T14:30:00Z
phases:
  discuss:
    status: complete
  research:
    status: complete
  plan:
    status: in_progress
    domain: dsp
    executeAgent: dsp-agent
  execute:
    status: pending
  verify:
    status: pending
versionBump: minor
baseVersion: 1.1.1
targetVersion: 1.2.0
```

## Plugin State Update

State is plugin-local: `plugins/[Name]/.planning/STATUS.md` is authoritative.

On milestone start, in that file's YAML frontmatter:
```yaml
activeMilestone: add-chorus-effect
```

On milestone complete: remove `activeMilestone` (or set it to null).

## Reference Documentation

Full skill documentation: `.claude/skills/improve-milestone/SKILL.md`
Templates: `.claude/skills/improve-milestone/assets/`
State management: `.claude/skills/improve-milestone/references/state-management.md`
