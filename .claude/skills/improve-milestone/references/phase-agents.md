# Phase Agent Specifications

This document defines the agent invocation protocol for each milestone phase.

---

## General Invocation Pattern

All phase agents receive a structured prompt via the Task tool:

```javascript
{
  subagent_type: "[agent-type]",
  prompt: "[structured prompt]",
  description: "[Phase] phase for [PluginName]"
}
```

---

## Phase 1: Discuss

**Agent:** general-purpose
**Purpose:** Gather requirements, clarify scope, capture user intent

### Invocation

```
Task: Discuss improvement requirements for [PluginName]

## Context
- Plugin: [PluginName]
- Milestone: [slug]
- User description: [original description]

## Files to Read
- plugins/[Name]/BRIEF.md (if exists)
- plugins/[Name]/CHANGELOG.md
- plugins/[Name]/Source/PluginProcessor.h (header only)

## Instructions
1. Read the plugin context files to understand current state
2. Present 4-6 clarifying questions about the improvement:
   - What specific behavior should change?
   - What is the expected user experience?
   - Are there constraints (CPU, compatibility)?
   - What's the priority if trade-offs needed?
3. Collect user answers via AskUserQuestion tool
4. Synthesize into requirements summary
5. Generate CONTEXT.md using template at:
   .claude/skills/improve-milestone/assets/context-template.md
6. Save to: plugins/[Name]/.planning/improvements/[slug]/CONTEXT.md

## Output
- CONTEXT.md file created
- STATUS.yaml updated with discuss phase complete
```

---

## Phase 2: Research

**Agent:** general-purpose
**Purpose:** Investigate implementation approach, find patterns, assess complexity

### Invocation

```
Task: Research implementation approach for [PluginName] improvement

## Context
- Plugin: [PluginName]
- Milestone: [slug]
- Requirements: Read from CONTEXT.md

## Files to Read
- plugins/[Name]/.planning/improvements/[slug]/CONTEXT.md
- plugins/[Name]/Source/ (relevant source files)
- troubleshooting/ (for known patterns)

## Instructions
1. Read CONTEXT.md to understand requirements
2. Search codebase for relevant patterns:
   - Similar implementations in other plugins
   - JUCE API usage patterns
   - Module patterns that might apply
3. Query Context7 for JUCE documentation if needed
4. Identify affected files and estimate complexity
5. Detect domain (DSP/GUI/Polish) using keyword analysis
6. Generate RESEARCH.md using template at:
   .claude/skills/improve-milestone/assets/research-template.md
7. Save to: plugins/[Name]/.planning/improvements/[slug]/RESEARCH.md

## Output
- RESEARCH.md file created
- Domain detected and documented
- STATUS.yaml updated with research phase complete
```

---

## Phase 3: Plan

**Agent:** general-purpose
**Purpose:** Create task breakdown, determine dependencies, select domain agent

### Invocation

```
Task: Create implementation plan for [PluginName] improvement

## Context
- Plugin: [PluginName]
- Milestone: [slug]
- Requirements: Read from CONTEXT.md
- Research: Read from RESEARCH.md

## Files to Read
- plugins/[Name]/.planning/improvements/[slug]/CONTEXT.md
- plugins/[Name]/.planning/improvements/[slug]/RESEARCH.md
- plugins/[Name]/CHANGELOG.md (for current version)

## Instructions
1. Read CONTEXT.md and RESEARCH.md
2. Confirm domain detection from research phase
3. Determine version bump type:
   - PATCH: Bug fix, no new features
   - MINOR: New feature, backward compatible
   - MAJOR: Breaking changes
4. Break improvement into atomic tasks:
   - Each task independently verifiable
   - Clear "done" state per task
   - Outcome-focused, not file-focused
5. Identify dependencies between tasks
6. Add verification criteria per task
7. Generate PLAN.md using template at:
   .claude/skills/improve-milestone/assets/plan-template.md
8. Include YAML frontmatter with domain and agent
9. Save to: plugins/[Name]/.planning/improvements/[slug]/PLAN.md
10. Present plan to user for approval

## Output
- PLAN.md file created with YAML frontmatter
- User approval obtained (approval gate)
- STATUS.yaml updated with:
  - plan phase complete
  - domain and executeAgent fields
  - versionBump, baseVersion, targetVersion
```

---

## Phase 4: Execute

**Agent:** Domain-specific (from PLAN.md frontmatter)
**Purpose:** Implement changes according to plan

### Agent Selection

Read `execute_agent` from PLAN.md frontmatter:

| Value | Agent | When Used |
|-------|-------|-----------|
| dsp-agent | DSP specialist | Audio processing, filters, buffers |
| gui-agent | GUI specialist | WebView, UI components, styling |
| polish-agent | Polish specialist | Presets, optimization, validation |
| general-purpose | Flexible | Mixed domain or unclear |

### Research Context Injection

Before invoking the domain-specific execute agent, retrieve research context:

```bash
# Map agent to primary stage for discovery
# dsp-agent -> stage 2, gui-agent -> stage 3, polish-agent -> stage 4
python3 .claude/scripts/inject-context.py --stage {stage_number} --agent {execute_agent} --plugin [PluginName]
```

Include the output in the Task prompt after the "Critical Rules" section. If the script returns empty, the prompt works without modification.

### Invocation (dsp-agent example)

```
Task: Execute DSP improvement plan for [PluginName]

## Context
- Plugin: [PluginName]
- Milestone: [slug]
- Domain: DSP
- Plan: Read from PLAN.md

## Files to Read
- plugins/[Name]/.planning/improvements/[slug]/PLAN.md
- plugins/[Name]/Source/PluginProcessor.cpp
- plugins/[Name]/Source/PluginProcessor.h

## Critical Rules (DSP Domain)
- No allocations in processBlock
- No blocking calls in audio thread
- Use atomic operations for cross-thread communication
- Prefer juce::dsp module utilities
- Smooth parameter changes to avoid clicks

{research_context}

## Instructions
1. Read PLAN.md task breakdown
2. Verify backup exists: backups/[PluginName]/v[baseVersion]/
3. Execute tasks in dependency order
4. After each task:
   - Verify the task outcome
   - Build and quick-test if significant
   - Update task status in local notes
5. Generate SUMMARY.md using template at:
   .claude/skills/improve-milestone/assets/summary-template.md
6. Save to: plugins/[Name]/.planning/improvements/[slug]/SUMMARY.md

## Output
- All plan tasks completed
- SUMMARY.md file created
- STATUS.yaml updated with execute phase complete
```

### Domain-Specific Rules

**dsp-agent:**
- No allocations in processBlock
- Atomic operations for thread safety
- Smooth parameter transitions
- Real-time safe data structures

**gui-agent:**
- Relays declared before attachments (member order)
- Proper null checks for WebView
- Consistent relay naming
- Resource cleanup in destructor

**polish-agent:**
- Factory vs user preset paths
- Pluginval compliance
- CPU profiling considerations
- Edge case handling

---

## Phase 5: Verify

**Agent:** general-purpose
**Purpose:** Validate that improvement achieves stated goals

### Invocation

```
Task: Verify improvement achievement for [PluginName]

## Context
- Plugin: [PluginName]
- Milestone: [slug]
- Requirements: Read from CONTEXT.md
- Implementation: Read from SUMMARY.md

## Files to Read
- plugins/[Name]/.planning/improvements/[slug]/CONTEXT.md
- plugins/[Name]/.planning/improvements/[slug]/PLAN.md
- plugins/[Name]/.planning/improvements/[slug]/SUMMARY.md
- plugins/[Name]/Source/ (verify implementation)

## Instructions
1. Read CONTEXT.md success criteria
2. Read SUMMARY.md implementation notes
3. Compare requirements against implementation
4. Build release version:
   - ninja [PluginName]_VST3 [PluginName]_AU
5. Run pluginval validation (Level 5 minimum)
6. Verify each success criterion:
   - Document evidence for each
   - Mark PASS/PARTIAL/FAIL
7. Check for regressions against existing features
8. Generate VERIFICATION.md using template at:
   .claude/skills/improve-milestone/assets/verification-template.md
9. Save to: plugins/[Name]/.planning/improvements/[slug]/VERIFICATION.md

## Output
- VERIFICATION.md file created
- Build and test results documented
- Final status (PASSED/PASSED WITH NOTES/FAILED)
- STATUS.yaml updated with verify phase complete
```

---

## Error Handling

### Agent Failure

If any phase agent fails:

1. Capture error state
2. Update STATUS.yaml with failure info
3. Present recovery options:
   - Retry current phase
   - Skip (if skippable)
   - Rollback
   - Abandon

### Context Overflow

If agent hits context limits:

1. Save partial progress
2. Recommend breaking into smaller tasks
3. Offer to restart with narrower scope
