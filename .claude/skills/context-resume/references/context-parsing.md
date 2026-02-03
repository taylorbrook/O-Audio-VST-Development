# Context Parsing and Presentation

**Context:** This file is part of the context-resume skill.
**Invoked by:** After handoff file is located
**Purpose:** Parse YAML frontmatter and markdown body, present clear summary to user

---

## Step 2a: Read Handoff File

Read the complete `.planning/STATUS.md` file:

```bash
cat "plugins/$PLUGIN_NAME/.planning/STATUS.md"
```

## Step 2b: Parse YAML Frontmatter

Extract structured metadata from YAML header:

**Required fields:**

```yaml
---
plugin: PluginName # String: Plugin directory name
stage: N # Integer: Current stage (0-6) or ideation marker
status: in_progress # String: in_progress, paused, blocked, ready
last_updated: 2025-11-10 14:30:00 # Timestamp: When handoff was written
---
```

**Optional fields:**

```yaml
phase: M # Integer: Current phase within stage (for complex plugins)
complexity_score: 3.2 # Float: Calculated complexity score
phased_implementation: true # Boolean: Whether using phase-based workflow
improvement: feature-name # String: Improvement proposal filename
mockup_version: 2 # Integer: Current mockup version number
```

**Field meanings:**

- **plugin**: Directory name exactly as appears in `plugins/`
- **stage**:
  - 0-6: Workflow stage number
  - "ideation": Creative brief phase
  - "mockup": UI mockup phase
  - "improvement_planning": Improvement proposal phase
- **status**:
  - `in_progress`: Actively working, normal workflow
  - `paused`: User explicitly paused, resume where left off
  - `blocked`: Waiting on external dependency
  - `ready`: Phase complete, ready for next action
- **last_updated**: ISO 8601 timestamp for "time ago" calculation

## Step 2c: Parse Markdown Body

Extract narrative context from markdown sections:

**Expected structure:**

```markdown
# Resume Point

## Current State: [Stage/Phase Description]

[Prose description of where we are]

## Completed So Far

**Stage 0-N:** ✓ Complete
- [Accomplishment 1]
- [Accomplishment 2]

**Stage N:** 🚧 In Progress
- [What's done in current stage]
- [What's next]

## Next Steps

1. [Specific next action - most immediate]
2. [Following action]
3. [Alternative action]

## Context to Preserve

**Key Decisions:**
- [Design choice and rationale]
- [Technical approach]

**Files Modified:**
- plugins/[Name]/Source/PluginProcessor.cpp:123-145

**Current Build Status:**
- Last build: [Success/Failed]
- Last test: [Pass/Fail]

**Research References:**
- [JUCE doc link]
- [Example plugin]
```

**Extract key information:**

1. Current state description
2. Completed work
3. Next steps (in priority order)
4. Key decisions
5. Modified files (file:line references)
6. Build status
7. Research links

## Step 2d: Calculate Time Ago

Parse `last_updated` timestamp and calculate human-readable "time ago":

```bash
last_updated="2025-11-10 14:30:00"
# Calculate as: "5 minutes ago", "2 hours ago", "1 day ago", etc.
```

## Step 3: Build Summary

Construct user-facing summary using the standard handoff format (see `.claude/references/handoff-protocol.md`).

**Required format elements:**
1. `━━━` separator lines for visual structure
2. Boxed "SESSION RESUMED" header
3. Current state section with plugin name bolded
4. Phase progress table
5. Key context preserved from handoff
6. Explicit two-step "Next Up" section with `/clear` + command
7. Alternative options listed

**Example for workflow resume:**

```
━━━ SESSION RESUMED ━━━

**TapeDelay** — Stage 3 (DSP Implementation)

| Phase | Status |
|-------|--------|
| Stage 0: Research | ✓ |
| Stage 1: Foundation | ✓ |
| Stage 2: DSP | → in progress |

**Current Position:** Stage 2, Phase plan (research complete)
**Last Session:** 2 hours ago
**Complexity:** C4 (3.8/5) — phased implementation

**Key Context:**
- Implementing LFOs for wow/flutter modulation
- Core delay algorithm complete

━━━━━━━━━━━━━━━━━━━━━━━━━━━

## ▶ Next Up

**Stage 2: DSP** — Continue modulation system implementation

**Step 1:** `/clear` — fresh context window
**Step 2:** `/implement TapeDelay`

━━━━━━━━━━━━━━━━━━━━━━━━━━━

**Also available:**

- `/plugin-plan TapeDelay 2-dsp` → Create execution plan first
- `/test TapeDelay` → Run validation tests
- Review stage artifacts
- Save for later (checkpoint preserved)

━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**CRITICAL:** Present summary and STOP. Do not auto-proceed — user must explicitly choose an action.

The two-step format (`/clear` then command) is mandatory. Never present a prose-style "Next:" instruction.

---

**Return to:** Main context-resume orchestration in `SKILL.md`
