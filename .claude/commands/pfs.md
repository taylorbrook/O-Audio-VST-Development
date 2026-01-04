---
name: pfs
description: Load Plugin Freedom System architecture context for meta-level analysis
---

# Plugin Freedom System Context Loader

<objective>
Load the complete Plugin Freedom System architecture from `.claude/` directory to enable meta-level analysis, optimization, and system improvement discussions.

This is NOT about building plugins WITH the system - this is about analyzing and improving the system ITSELF.
</objective>

<context_framing>
After loading context, all subsequent user questions should be interpreted as:
- Questions about system architecture and design
- Optimization opportunities within the system
- Improvements to skills, agents, commands, or hooks
- System-level refactoring or enhancement
- Meta-analysis of the Plugin Freedom System implementation

NOT as questions about plugin development or using the system to build plugins.
</context_framing>

<loading_strategy>

## Phase 1: Discover Structure

Use Glob to discover all architectural files:

<file_discovery>
- `.claude/skills/*/SKILL.md` - All skill definitions
- `.claude/agents/*.md` - All subagent definitions
- `.claude/commands/*.md` - All slash command definitions
- `.claude/hooks/*.md` - All hook definitions
</file_discovery>

## Phase 2: Present Inventory

After discovery, present a brief structural summary:

```
✓ Plugin Freedom System inventory complete

**System Architecture:**
- Skills: [count] available
- Agents: [count] available
- Commands: [count] available
- Hooks: [count] available

**Context set:** Meta-analysis mode (system optimization, not plugin development)

What would you like to analyze or improve?
```

## Phase 3: Load on Demand

**DO NOT load any files yet.** Wait for the user's question.

When the user asks their question:
1. Determine which specific files are relevant (typically 2-5 files)
2. Read those files in parallel
3. Answer the question with loaded context

**Why:** Loading everything upfront triggers compaction and wipes context. This approach keeps the inventory in memory and loads only what's needed for each specific question.

</loading_strategy>

<exclusions>

**DO NOT load:**
- `.claude/aesthetics/*` (visual templates, not system architecture)
- `PLUGINS.md` (plugin tracking, not system architecture)
- `.continue-here.md` (workflow state, not architecture)
- `system-config.json` (environment config, not architecture)
- Any files under `plugins/` (plugin implementations, not system code)
- Any backup directories

**WHY:** These are data/state managed BY the system, not part of the system's architectural implementation.

</exclusions>

<execution_guidelines>

1. **Discover, don't load** - Use Glob to inventory files, but don't Read them yet
2. **Set the frame** - Establish meta-analysis mode in your response
3. **Wait for the question** - Let user ask what they want to analyze
4. **Load on demand** - Only read the 2-5 files relevant to their specific question
5. **No premature context burn** - Avoid compaction by not loading everything upfront

</execution_guidelines>

<claude_code_integration>

## After User Asks Question

Once the user poses their question, determine if it involves Claude Code primitives or best practices:

**Invoke `claude-code-knowledge` skill if the question involves:**
- How hooks/skills/agents/commands work (Claude Code features)
- Best practices for these Claude Code features
- MCP server integration patterns
- Configuration and settings
- Tool usage patterns
- Optimization strategies for Claude Code primitives
- How Claude Code handles specific scenarios

**Proceed directly if the question is:**
- Pure PFS-specific logic (workflow stages, checkpoint protocol specifics)
- PFS file structure or naming conventions
- Plugin-specific implementation details
- System state management unique to PFS

**WHY:** The Plugin Freedom System is built ON Claude Code primitives. Understanding the underlying framework helps identify optimization opportunities and ensures suggestions align with Claude Code best practices. Many optimization questions require knowing both what PFS does AND how Claude Code's primitives work.

**Example flow:**
```
User: "How can we optimize the checkpoint protocol?"
→ This involves hooks and best practices → Invoke claude-code-knowledge
→ Then analyze with both Claude Code docs + loaded PFS context

User: "Should we rename the .continue-here.md file?"
→ Pure PFS naming convention → Proceed directly with PFS context
```

</claude_code_integration>

<success_criteria>

The context loading is successful when:
- All `.claude/` architectural files are loaded into context
- System structure is clear (skills/agents/commands/hooks)
- Framing is established (this is about the system, not plugin development)
- Claude is ready to answer meta-level questions about system optimization/improvement

</success_criteria>
