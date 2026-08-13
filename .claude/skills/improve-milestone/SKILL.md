---
name: improve-milestone
description: Complex improvements with GSD-style phase cycles (discuss → research → plan → execute → verify) and context clearing between phases. Use for Tier 3 improvements, multi-session work, or research-heavy changes.
allowed-tools:
  - Read
  - Write
  - Edit
  - Bash
  - Task # For domain-specific execute agents
  - AskUserQuestion
preconditions:
  - Plugin status must be ✅ Working OR 📦 Installed
  - Plugin must NOT be 🚧 In Development
---

## Contract

- Input schema: `.claude/schemas/agent-contracts/improve-milestone.input.json`
- Output schema: `.claude/schemas/agent-contracts/improve-milestone.output.json`
- Boundaries: See `BOUNDARIES.md` in this directory

## Contract Validation

Before processing any request, validate inputs against the contract:

1. **Load schema:** `.claude/schemas/agent-contracts/improve-milestone.input.json`
2. **Validate:** Check all required fields present, types match, constraints satisfied
3. **On violation:** Stop immediately with error message
4. **On success:** Proceed to main agent logic

# improve-milestone Skill

**Purpose:** Manage complex improvements through GSD-style phase cycles with context clearing between phases.

## Workflow Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                       MILESTONE ORCHESTRATOR                         │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐       │
│  │ DISCUSS  │───>│ RESEARCH │───>│  PLAN    │───>│ EXECUTE  │───>   │
│  │(skippable)│   │(skippable)│   │          │    │(domain)  │       │
│  └──────────┘    └──────────┘    └──────────┘    └──────────┘       │
│       │               │               │               │              │
│       v               v               v               v              │
│  CONTEXT.md      RESEARCH.md      PLAN.md       SUMMARY.md          │
│       │               │               │               │              │
│       └───────────────┴───────────────┴───────────────┘              │
│                           │                                          │
│                           v                                          │
│                    ┌──────────┐                                      │
│                    │  VERIFY  │                                      │
│                    │(skippable)│                                     │
│                    └──────────┘                                      │
│                           │                                          │
│                           v                                          │
│                   VERIFICATION.md                                    │
│                           │                                          │
│                           v                                          │
│              [Version Bump + CHANGELOG + Git Tag]                    │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘

/clear between each phase transition
```

## Phase Overview

| Phase | Agent | Output | Skippable | Purpose |
|-------|-------|--------|-----------|---------|
| discuss | general-purpose | CONTEXT.md | Yes | Gather requirements, clarify scope |
| research | general-purpose | RESEARCH.md | Yes | Investigate approach, find patterns |
| plan | general-purpose | PLAN.md | No | Create task breakdown with dependencies |
| execute | **domain-specific** | SUMMARY.md | No | Implement changes |
| verify | general-purpose | VERIFICATION.md | Yes | Validate goal achievement |

## State Location

All milestone state stored in plugin-local directory:

```
plugins/[Name]/.planning/improvements/[milestone-slug]/
├── CONTEXT.md           # Discuss phase output
├── RESEARCH.md          # Research phase output
├── PLAN.md              # Plan phase output
├── SUMMARY.md           # Execute phase output
├── VERIFICATION.md      # Verify phase output
└── STATUS.yaml          # Phase tracking
```

**STATUS.yaml format:**
```yaml
milestone: add-chorus-effect
created: 2026-02-02T10:00:00Z
lastActivity: 2026-02-02T14:30:00Z
phases:
  discuss:
    status: complete  # pending | in_progress | complete | skipped
    completedAt: 2026-02-02T10:15:00Z
  research:
    status: complete
    completedAt: 2026-02-02T11:00:00Z
  plan:
    status: in_progress
    startedAt: 2026-02-02T11:05:00Z
    domain: dsp  # Detected during plan phase
    executeAgent: dsp-agent
  execute:
    status: pending
  verify:
    status: pending
versionBump: minor  # Determined during plan phase
baseVersion: 1.1.1
targetVersion: 1.2.0
```

## Progress Checklist

Copy this checklist and check off phases as you complete them:

```
Milestone Progress: [PluginName] - [milestone-slug]
- [ ] Phase 1: Discuss - Gather requirements
- [ ] Phase 2: Research - Investigate approach
- [ ] Phase 3: Plan - Create task breakdown
- [ ] Phase 4: Execute - Implement changes
- [ ] Phase 5: Verify - Validate achievement
- [ ] Post: Version bump, CHANGELOG, git tag
```

---

## Entry Point: Invocation Routing

When `/improve-milestone` is invoked:

### 1. Parse Arguments

```
/improve-milestone [PluginName?] [description?]
```

- **No arguments**: Present plugin selection menu
- **Plugin only**: Check for existing milestone or prompt for description
- **Plugin + description**: Check for existing milestone or start new

### 2. Check Existing Milestone

Read `plugins/[Name]/.planning/STATUS.md` frontmatter and check for `activeMilestone`:

```javascript
const status = readFrontmatter(`plugins/${pluginName}/.planning/STATUS.md`);
if (status.activeMilestone) {
  // Resume existing milestone
  presentResumeMenu(status.activeMilestone);
} else {
  // Start new milestone
  startNewMilestone(pluginName, description);
}
```

**Resume menu:**
```
Found existing milestone: "[milestone-slug]"
Current phase: [phase] ([status])

1. Continue from current phase
2. Restart this phase
3. Start new milestone (abandons current)
4. Other

Choose (1-4): _
```

### 3. Start New Milestone

1. Generate slug from description (e.g., "add chorus effect" → "add-chorus-effect")
2. Create directory: `plugins/[Name]/.planning/improvements/[slug]/`
3. Initialize STATUS.yaml
4. Set `activeMilestone` in `plugins/[Name]/.planning/STATUS.md` frontmatter
5. Begin discuss phase

---

## Phase 1: Discuss

**Purpose:** Gather requirements, clarify scope, capture user intent.

**Agent:** general-purpose

**Invocation:**
```
Task: Discuss improvement requirements for [PluginName]

Context:
- Plugin: [PluginName]
- Milestone: [slug]
- Description: [user description]
- Files to read: BRIEF.md, CHANGELOG.md, PluginProcessor.cpp header

Instructions:
1. Read plugin context files
2. Present 4-6 clarifying questions about the improvement
3. Collect user answers
4. Generate CONTEXT.md with requirements summary
5. Save to plugins/[Name]/.planning/improvements/[slug]/CONTEXT.md
```

**Output template:** See `assets/context-template.md`

**Skip conditions:**
- User provides `--skip-discuss` flag
- CONTEXT.md already exists and user confirms it's current

**On completion:**
```
✓ Discuss Phase Complete

Requirements captured in CONTEXT.md

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │   next     │            │            │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

---

## Phase 2: Research

**Purpose:** Investigate implementation approach, find patterns, assess complexity.

**Agent:** general-purpose

**Invocation:**
```
Task: Research implementation approach for [PluginName] improvement

Context:
- Read: plugins/[Name]/.planning/improvements/[slug]/CONTEXT.md
- Plugin source: plugins/[Name]/Source/

Instructions:
1. Read CONTEXT.md requirements
2. Search codebase for relevant patterns
3. Check JUCE documentation if needed (Context7)
4. Identify affected files and components
5. Assess complexity and domain
6. Generate RESEARCH.md with findings
7. Save to plugins/[Name]/.planning/improvements/[slug]/RESEARCH.md
```

**Output template:** See `assets/research-template.md`

**Skip conditions:**
- User provides `--skip-research` flag
- RESEARCH.md already exists and user confirms it's current
- Approach is straightforward (no novel patterns needed)

**On completion:**
```
✓ Research Phase Complete

Findings documented in RESEARCH.md

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │     ✓      │   next     │            │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

---

## Phase 3: Plan

**Purpose:** Create task breakdown, determine dependencies, select domain agent.

**Agent:** general-purpose

**This phase is NOT skippable.** Planning is required for milestone-tracked improvements.

**Invocation:**
```
Task: Create implementation plan for [PluginName] improvement

Context:
- Read: CONTEXT.md, RESEARCH.md
- Plugin source: plugins/[Name]/Source/

Instructions:
1. Read CONTEXT.md and RESEARCH.md
2. Detect domain from content (see Domain Detection below)
3. Determine version bump type (PATCH/MINOR/MAJOR)
4. Break improvement into atomic tasks
5. Identify dependencies between tasks
6. Add verification criteria per task
7. Generate PLAN.md with task breakdown
8. Update STATUS.yaml with domain and executeAgent
9. Save to plugins/[Name]/.planning/improvements/[slug]/PLAN.md
```

**Domain Detection Logic:**

```python
def detect_domain(context_md, research_md):
    text = (context_md + research_md).lower()

    dsp_keywords = ["processblock", "filter", "gain", "dsp", "algorithm",
                    "buffer", "sample", "frequency", "audio processing",
                    "realtime", "latency", "fft"]
    gui_keywords = ["webview", "ui", "relay", "attachment", "css", "html",
                    "interface", "parameter binding", "editor", "visual",
                    "layout", "component"]
    polish_keywords = ["preset", "optimization", "validation", "polish",
                       "performance", "cpu", "pluginval", "factory preset"]

    dsp_score = sum(1 for k in dsp_keywords if k in text)
    gui_score = sum(1 for k in gui_keywords if k in text)
    polish_score = sum(1 for k in polish_keywords if k in text)

    max_score = max(dsp_score, gui_score, polish_score)

    if max_score == 0 or (dsp_score > 0 and gui_score > 0):
        return "general-purpose"  # Mixed or unclear
    elif dsp_score == max_score:
        return "dsp-agent"
    elif gui_score == max_score:
        return "gui-agent"
    else:
        return "polish-agent"
```

**PLAN.md header format:**
```yaml
---
milestone: add-chorus-effect
domain: dsp
execute_agent: dsp-agent
version_bump: minor
base_version: 1.1.1
target_version: 1.2.0
created: 2026-02-02
---
```

**Output template:** See `assets/plan-template.md`

**Approval gate:** Plan MUST be approved before execute phase begins.

**On completion:**
```
✓ Plan Phase Complete

Task breakdown in PLAN.md
Domain detected: DSP (dsp-agent will execute)
Version: 1.1.1 → 1.2.0 (MINOR)

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │     ✓      │     ✓      │   next     │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

---

## Phase 4: Execute

**Purpose:** Implement changes according to plan using domain-appropriate agent.

**Agent:** Domain-specific (from plan phase):
- `dsp-agent` - For DSP/audio processing changes
- `gui-agent` - For WebView/UI changes
- `polish-agent` - For presets/optimization (if exists)
- `general-purpose` - For mixed or unclear domain

**Pre-execute: Create Backup**

Before any implementation:
1. Create backup: `backups/[PluginName]/v[CurrentVersion]/`
2. Verify backup integrity
3. HALT if backup fails

**Research Context Injection:**

Before invoking the domain-specific execute agent, retrieve research context:

```python
# Determine stage number from domain for discovery context
domain_stage_map = {
    "dsp-agent": 2,
    "gui-agent": 3,
    "polish-agent": 4,
    "general-purpose": 0
}
execute_agent = read_status_yaml(plugin_name, slug)["executeAgent"]
stage_for_discovery = domain_stage_map.get(execute_agent, 0)

# Get research context via injection utility
research_context = run_bash(
    f"python3 .claude/scripts/inject-context.py --stage {stage_for_discovery} --agent {execute_agent} --plugin {plugin_name}"
)
```

**Invocation:**
```
Task: Execute improvement plan for [PluginName]

Context:
- Read: PLAN.md (contains task breakdown)
- Domain: [domain from PLAN.md]
- Version: [base] → [target]

Instructions:
1. Read PLAN.md task breakdown
2. Execute tasks in dependency order
3. Mark completed tasks in PLAN.md
4. Generate SUMMARY.md with implementation notes
5. Save to plugins/[Name]/.planning/improvements/[slug]/SUMMARY.md

Important:
- Follow real-time safety rules for DSP changes
- Maintain member declaration order for WebView (relays before attachments)
- Test incrementally after each task

{research_context}
```

**Output template:** See `assets/summary-template.md`

**On completion:**
```
✓ Execute Phase Complete

Changes implemented per PLAN.md
Implementation notes in SUMMARY.md

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │     ✓      │     ✓      │     ✓      │   next     │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

---

## Phase 5: Verify

**Purpose:** Validate that improvement achieves stated goals, not just that tasks completed.

**Agent:** general-purpose

**Invocation:**
```
Task: Verify improvement achievement for [PluginName]

Context:
- Read: CONTEXT.md (original requirements)
- Read: PLAN.md (what was planned)
- Read: SUMMARY.md (what was implemented)
- Plugin source: plugins/[Name]/Source/

Instructions:
1. Compare CONTEXT.md requirements against implementation
2. Check each requirement is satisfied
3. Build and test (invoke build-automation)
4. Run pluginval validation
5. Generate VERIFICATION.md with results
6. Save to plugins/[Name]/.planning/improvements/[slug]/VERIFICATION.md
```

**Output template:** See `assets/verification-template.md`

**Skip conditions:**
- User provides `--skip-verify` flag (trusted changes)
- User explicitly requests skip at completion menu

**On completion (success):**
```
✓ Verify Phase Complete - All Goals Achieved

Verification results in VERIFICATION.md

Completing milestone:
- Version: 1.1.1 → 1.2.0
- Updating CHANGELOG.md
- Creating git tag v1.2.0
- Clearing activeMilestone from registry

Step 1: /clear (optional - milestone complete)

What would you like to do next?

1. Test in DAW
2. Make another improvement
3. Create new plugin
4. Other

Choose (1-4): _
```

**On completion (issues found):**
```
⚠ Verify Phase Complete - Issues Found

See VERIFICATION.md for details.

1. Return to execute phase (fix issues)
2. Accept as-is (document known issues)
3. Rollback to backup
4. Other

Choose (1-4): _
```

---

## Post-Milestone: Version Integration

After successful verification:

### 1. Version Bump

Read `targetVersion` from STATUS.yaml and apply to:
- CMakeLists.txt (project version)
- PluginProcessor.cpp (version string if present)

### 2. CHANGELOG Update

Add entry to CHANGELOG.md:

```markdown
## [1.2.0] - 2026-02-02

### Added
- [Feature from CONTEXT.md]

### Changed
- [Changes from SUMMARY.md]

### Technical Notes
- Domain: DSP
- Milestone: add-chorus-effect
- Investigation: [summary from RESEARCH.md]
```

### 3. Git Operations

```bash
git add plugins/[PluginName]/ .planning/
git commit -m "improve: [PluginName] v[version] - [milestone-slug]"
git tag -a "v[version]" -m "[PluginName] v[version]: [description]"
```

### 4. State Cleanup

Remove `activeMilestone` from `plugins/[Name]/.planning/STATUS.md` frontmatter.

---

## Handoff Protocol

Between each phase, present two-step handoff:

```
✓ [Phase] Complete

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │     ✓      │   next     │            │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

**Why /clear between phases:**
- Prevents context overflow on large improvements
- Each phase starts fresh with focused context
- State persisted in files, not conversation history

See `references/handoff-protocol.md` for complete protocol.

---

## Error Handling

### Phase Failure

If any phase fails:
1. Save partial progress to STATUS.yaml
2. Present recovery menu:
   - Retry current phase
   - Skip to next phase (if skippable)
   - Rollback to previous checkpoint
   - Abandon milestone

### Build Failure (Execute Phase)

1. Capture build error
2. Present investigation menu
3. Allow retry after fix
4. Option to rollback

### Verification Failure

1. Document issues in VERIFICATION.md
2. Present options:
   - Fix and re-verify
   - Accept with known issues
   - Rollback to backup

---

## Integration Points

**Invoked by:**
- `/improve-milestone` command (direct)
- `/improve` command (when Tier 3 detected, user chooses milestone)
- `/continue` command (when activeMilestone exists)

**Invokes:**
- `general-purpose` agent (discuss, research, plan, verify phases)
- `dsp-agent` (execute phase, DSP domain)
- `gui-agent` (execute phase, GUI domain)
- `build-automation` skill (verify phase)
- `plugin-testing` skill (verify phase)

**Updates:**
- `plugins/[Name]/.planning/improvements/[slug]/` (all phase outputs)
- `plugins/[Name]/.planning/STATUS.md` (activeMilestone frontmatter field)
- `CHANGELOG.md` (version entry)
- Source files (execute phase)

**Creates:**
- Backup in `backups/[PluginName]/v[CurrentVersion]/`
- Git commit and tag

---

## Success Criteria

Milestone is successful when:

1. All non-skipped phases complete
2. VERIFICATION.md shows goals achieved
3. Build succeeds without errors
4. Pluginval passes (if run)
5. CHANGELOG updated with version entry
6. Git tag created
7. Registry cleared of activeMilestone
