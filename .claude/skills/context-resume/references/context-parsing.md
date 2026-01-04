# Context Parsing and Presentation

**Context:** This file is part of the context-resume skill.
**Invoked by:** After handoff file is located
**Purpose:** Parse YAML frontmatter and markdown body, present clear summary to user

---

## Step 2a: Read Handoff File

Read the complete `.continue-here.md` file:

```bash
cat "plugins/$PLUGIN_NAME/.continue-here.md"
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

Construct user-facing summary combining all parsed information:

**Example for workflow resume:**

```
Resuming TapeDelay at Stage 3 (DSP Implementation)...

Status: Stage 3.2 in progress (modulation system)
Last session: 2 hours ago

Progress so far:
✓ Stage 0: Research
✓ Stage 1: Planning (complexity 3.8, phased)
✓ Stage 1: Foundation
✓ Stage 2: Shell
✓ Stage 3.1: Core delay algorithm
🚧 Stage 3.2: Modulation system (in progress)

Current work:
- Implementing LFOs for wow/flutter modulation
- Next: Add modulation to delay read pointer

Last build: ✓ Success
Last test: ✓ Pass (4/5 tests)

Next steps:
1. ⭐ Continue Stage 3.2 (implement modulation matrix)
2. Review Stage 3.1 code
3. Test wow/flutter parameters manually
4. Pause here

Ready to continue with modulation implementation?
```

**Present summary and wait for user confirmation.**

Do not auto-proceed - user must explicitly choose next action.

---

**Return to:** Main context-resume orchestration in `SKILL.md`
