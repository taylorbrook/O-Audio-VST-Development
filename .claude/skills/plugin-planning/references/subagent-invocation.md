# Subagent Invocation Pattern

Detailed protocol for invoking research-planning-agent from plugin-planning orchestrator.

## Prerequisites

Before invoking subagent, read contracts:

```
Read tools (in parallel):
- plugins/[Name]/.ideas/creative-brief.md (REQUIRED)
- plugins/[Name]/.ideas/parameter-spec.md (if exists)
- plugins/[Name]/.ideas/parameter-spec-draft.md (if parameter-spec.md missing)
- Glob: plugins/[Name]/.ideas/mockups/*.yaml (if directory exists)
```

**Performance:** Use parallel Read calls in single tool invocation block to minimize round trips.

## Prompt Construction

Construct prompt with contracts prepended to provide full context:

```
You are research-planning-agent. Execute Stage 0 (Research & Planning) for [PluginName].

Creative brief:
[content of creative-brief.md]

Parameters:
[content of parameter-spec.md or parameter-spec-draft.md]

[If mockup exists:]
UI mockup:
[content of mockup YAML file]

Execute the full Stage 0 protocol:

PART 1 - Research:
1. Complexity detection (Tier 1-6)
2. Feature identification (meta-research)
3. Per-feature deep research (algorithmic understanding, professional research, JUCE API mapping, validation)
4. Integration analysis
5. Create architecture.md from template

PART 2 - Planning:
1. Calculate complexity score from parameters and architecture
2. Determine implementation strategy (single-pass or phased)
3. Create phase breakdown if complex (score ≥ 3.0)
4. Generate plan.md from template

Update state files and commit both architecture.md and plan.md.
Return JSON report with both file locations and status.
```

**Variables to replace:**
- `[PluginName]` → Actual plugin name
- `[content of ...]` → Actual file contents from Read calls

## Task Tool Invocation

```
Task(
  subagent_type="research-planning-agent",
  description="[prompt with contracts prepended]",
  model="sonnet"
)
```

**Required parameters:**
- `subagent_type`: Must be "research-planning-agent" (matches .claude/agents/research-planning-agent.md)
- `description`: Full prompt with contracts included
- `model`: "sonnet" (Stage 0 requires deep reasoning)

## Post-Invocation Protocol

After subagent completes:

1. **Read return message:**
   ```
   Read subagent's final message containing JSON report
   ```

2. **Verify outputs:**
   ```bash
   # Check architecture.md created
   test -f "plugins/${PLUGIN_NAME}/.ideas/architecture.md" || exit 1

   # Check plan.md created
   test -f "plugins/${PLUGIN_NAME}/.ideas/plan.md" || exit 1

   # Verify architecture.md has required sections
   grep -q "## Core Components" "plugins/${PLUGIN_NAME}/.ideas/architecture.md" || exit 1
   grep -q "## Processing Chain" "plugins/${PLUGIN_NAME}/.ideas/architecture.md" || exit 1

   # Verify plan.md has complexity score
   grep -q "complexity_score:" "plugins/${PLUGIN_NAME}/.ideas/plan.md" || exit 1
   ```

3. **Execute checkpoint protocol:**
   - Commit changes (see references/git-operations.md)
   - Update state files (see references/state-updates.md)
   - Present decision menu (use assets/decision-menu-stage-0.md template)
   - WAIT for user response

## Error Handling

**If subagent fails:**
- Read subagent's error message
- Check if partial outputs exist (architecture.md without plan.md)
- Report specific failure to user
- Offer to retry with same inputs or revise creative brief

**If verification fails:**
- Display which check failed (architecture.md missing, plan.md incomplete, etc.)
- Do NOT proceed to decision menu
- Return to precondition validation

## Dispatcher Pattern Compliance

- **Fresh context:** Subagent runs in isolated context (5-35 min session doesn't pollute orchestrator)
- **No direct implementation:** Orchestrator never implements research or planning - only delegates
- **Return message validation:** Always read and verify subagent outputs before proceeding
- **Contract passing:** Subagent receives all necessary context via prompt (no implicit state)
