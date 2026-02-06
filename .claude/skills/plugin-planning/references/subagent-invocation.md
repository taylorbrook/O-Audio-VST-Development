# Subagent Invocation Pattern

Detailed protocol for invoking research-planning-agent from plugin-planning orchestrator.

## Planning Location

All planning files are stored in `plugins/[Name]/.planning/` (plugin-local).

## Prerequisites

Before invoking subagent, read contracts:

```
Read tools (in parallel):
- plugins/[Name]/.planning/BRIEF.md (REQUIRED)
- plugins/[Name]/.planning/parameter-spec.md (if exists)
- plugins/[Name]/.planning/parameter-spec-draft.md (if parameter-spec.md missing)
- Glob: plugins/[Name]/.planning/mockups/*.yaml (if directory exists)
```

**Performance:** Use parallel Read calls in single tool invocation block to minimize round trips.

## Research Context Injection

Before constructing the prompt, retrieve research context for Stage 0:

```bash
# Get auto-discovered research resources for this agent
python3 .claude/scripts/inject-context.py --stage 0 --agent research-planning-agent --plugin [PluginName]
```

Store the output as `research_context`. If the script fails or returns empty, proceed with an empty string (harmless when embedded in prompt).

## Prompt Construction

Construct prompt with contracts prepended to provide full context:

```
You are research-planning-agent. Execute Stage 0 (Research & Planning) for [PluginName].

Creative brief:
[content of BRIEF.md]

Parameters:
[content of parameter-spec.md or parameter-spec-draft.md]

[If mockup exists:]
UI mockup:
[content of mockup YAML file]

Execute the full Stage 0 protocol (GSD-style discuss → research → plan):

PART 1 - Research:
1. Complexity detection (Tier 1-6)
2. Feature identification (meta-research)
3. Per-feature deep research (algorithmic understanding, professional research, JUCE API mapping, validation)
4. Integration analysis
5. Create plugins/[Name]/.planning/research/ARCHITECTURE.md from template

PART 2 - Planning:
1. Calculate complexity score from parameters and architecture
2. Determine implementation strategy (single-pass or phased)
3. Create stage breakdown if complex (score ≥ 3.0)
4. Generate plugins/[Name]/.planning/ROADMAP.md from template

PART 3 - State:
1. Create plugins/[Name]/.planning/stages/0-ideation/CONTEXT.md with discuss findings
2. Update plugins/[Name]/.planning/STATUS.md with stage progress
3. Commit changes

{research_context}

Return JSON report with file locations and status.
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
   # Check ARCHITECTURE.md created (plugin-local path)
   test -f "plugins/${PLUGIN_NAME}/.planning/research/ARCHITECTURE.md" || exit 1

   # Check ROADMAP.md created
   test -f "plugins/${PLUGIN_NAME}/.planning/ROADMAP.md" || exit 1

   # Verify ARCHITECTURE.md has required sections
   grep -q "## Core Components" "plugins/${PLUGIN_NAME}/.planning/research/ARCHITECTURE.md" || exit 1
   grep -q "## Processing Chain" "plugins/${PLUGIN_NAME}/.planning/research/ARCHITECTURE.md" || exit 1

   # Verify ROADMAP.md has complexity score
   grep -q "complexity_score:" "plugins/${PLUGIN_NAME}/.planning/ROADMAP.md" || exit 1
   ```

3. **Execute checkpoint protocol:**
   - Commit changes (see references/git-operations.md)
   - Update state files (see references/state-updates.md)
   - Present decision menu (use assets/decision-menu-stage-0.md template)
   - WAIT for user response

## Error Handling

**If subagent fails:**
- Read subagent's error message
- Check if partial outputs exist (ARCHITECTURE.md without ROADMAP.md)
- Report specific failure to user
- Offer to retry with same inputs or revise BRIEF.md

**If verification fails:**
- Display which check failed (ARCHITECTURE.md missing, ROADMAP.md incomplete, etc.)
- Do NOT proceed to decision menu
- Return to precondition validation

## Dispatcher Pattern Compliance

- **Fresh context:** Subagent runs in isolated context (5-35 min session doesn't pollute orchestrator)
- **No direct implementation:** Orchestrator never implements research or planning - only delegates
- **Return message validation:** Always read and verify subagent outputs before proceeding
- **Contract passing:** Subagent receives all necessary context via prompt (no implicit state)
