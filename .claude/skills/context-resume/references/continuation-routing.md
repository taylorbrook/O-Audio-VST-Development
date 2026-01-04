# Continuation Routing

**Context:** This file is part of the context-resume skill.
**Invoked by:** After user confirms continuation from summary
**Purpose:** Route to appropriate continuation skill based on handoff content

---

## Step 4a: Determine Routing

Based on handoff content and user selection, route to appropriate skill.

### Step 4a-1: Check Orchestration Mode

**NOTE:** The orchestration mode protocol is defined as a top-level delegation rule in the main SKILL.md. See "Orchestration Protocol" section for the primary specification.

**IMPORTANT:** Before routing, check if handoff has orchestration mode enabled:

```yaml
orchestration_mode: true
next_action: invoke_[subagent]_agent
next_phase: X.Y
```

If `orchestration_mode: true`:
1. Present orchestration notice to user:
   ```
   ⚠️ ORCHESTRATION MODE ACTIVE

   This handoff requires explicit skill invocation:
   - Next action: [next_action value]
   - Target phase: [next_phase value]

   Ready to invoke plugin-workflow skill? (y/n)
   ```

2. After user confirmation, invoke plugin-workflow skill using Skill tool:
   ```
   Skill({ skill: "plugin-workflow" })
   ```

   Pass resume context through the skill invocation.

3. DO NOT implement directly in main thread. The skill will handle subagent invocation.

If `orchestration_mode: false` or not present, use legacy routing (below).

---

### Step 4a-2: Legacy Routing (orchestration_mode not enabled)

Based on handoff content and user selection, route to appropriate skill:

### Workflow resume (stage = 0-3)

```
User chose: Continue Stage [N]

Routing to: plugin-workflow skill
Parameters:
  - plugin_name: [PluginName]
  - resume_from: stage [N]
  - phase: [M] (if phased)
```

Invoke plugin-workflow skill with resume context.

plugin-workflow will:
- Read handoff file for context
- Skip completed stages
- Continue from current stage/phase
- Update handoff file as it progresses

### Ideation resume (stage = "ideation")

```
User chose option 1 (Start implementation) or 2 (Refine mockup)

If option 1:
  Routing to: plugin-workflow skill (Stage 0)

If option 2:
  Routing to: plugin-ideation skill (improvement mode) or ui-mockup skill
```

### Mockup resume (stage = "mockup")

```
User chose option 1 (Finalize) or 2 (Iterate)

If option 1 (Finalize):
  - Generate parameter-spec.md if not exists
  - Offer to start Stage 3 (GUI) with plugin-workflow
  - Or offer to start Stage 0 (Research) for new implementation

If option 2 (Iterate):
  Routing to: ui-mockup skill (creates v[N+1])
```

### Improvement resume (stage = "improvement_planning")

```
User chose: Start implementation

Routing to: plugin-improve skill
Parameters:
  - plugin_name: [PluginName]
  - improvement_file: [from YAML]
```

## Step 4b: Load Context Files

Before invoking continuation skill, load relevant context:

**For workflow resume:**

```bash
# Read all contract files
cat "plugins/$PLUGIN_NAME/.ideas/creative-brief.md"
cat "plugins/$PLUGIN_NAME/.ideas/parameter-spec.md"
cat "plugins/$PLUGIN_NAME/.ideas/architecture.md"
cat "plugins/$PLUGIN_NAME/.ideas/plan.md"

# Show recent commits
git log --oneline plugins/$PLUGIN_NAME/ -5

# Read source files mentioned in handoff (if any)
cat "plugins/$PLUGIN_NAME/Source/PluginProcessor.cpp"
```

**For ideation/mockup resume:**

```bash
# Read creative brief
cat "plugins/$PLUGIN_NAME/.ideas/creative-brief.md"

# Read latest mockup if exists
find "plugins/$PLUGIN_NAME/.ideas/mockups/" -name "v*-ui.yaml" | sort -V | tail -1 | xargs cat
```

**For improvement resume:**

```bash
# Read improvement proposal
cat "plugins/$PLUGIN_NAME/.ideas/improvements/$IMPROVEMENT_FILE"

# Read CHANGELOG for version context
cat "plugins/$PLUGIN_NAME/CHANGELOG.md"

# Read PLUGINS.md entry
grep -A 20 "^### $PLUGIN_NAME$" PLUGINS.md
```

## Step 4c: Invoke Continuation Skill

Execute the appropriate skill with full context:

**Example: Workflow continuation**

```
Loaded context:
- Creative brief: Tape delay with vintage character
- Parameter spec: 8 parameters defined
- Architecture: Schroeder topology with modulation
- Plan: Complexity 3.8, phased implementation

Invoking plugin-workflow at Stage 3.2...
```

Skill then continues from documented next steps.

---

**Return to:** Main context-resume orchestration in `SKILL.md`
