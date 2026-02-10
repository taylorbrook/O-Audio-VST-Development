# Phase 17: Agent Intelligence - Research

**Researched:** 2026-02-09
**Domain:** Claude Code agent orchestration, parallel agent teams, quality hooks, task validation
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Research team composition
- Team size: 2-3 researchers depending on plugin complexity (simple plugins get 2, complex get 3)
- Researcher domains are fully dynamic — determined at runtime based on creative brief, complexity, and plugin type (not a fixed set like "DSP + UI + module audit")
- Findings merge via debate format — researchers read each other's outputs and produce a synthesis, catching contradictions
- Researcher conflicts block planning — if researchers disagree on incompatible approaches, planning pauses until the conflict is resolved (not just surfaced as a warning)

#### Review/critic workflow
- Parallel critics run after every stage completion (Stage 1, 2, 3, 4) — not just at cross-stage boundaries
- Critic domains: Claude's Discretion — Claude picks which critics are relevant based on the stage and plugin type (e.g., DSP critic irrelevant after Stage 1 foundation)
- Unified review report: severity-ranked issues — all critic findings merged into one list sorted by severity (blocker > warning > note)
- Enforcement: blocker-severity findings prevent stage progression; warnings and notes are advisory

#### Approval gates & delegation
- Plan approval: auto-approve low-risk plans (small scope, few files, no DSP changes); complex plans require team lead review
- Delegate mode tools: coordination + read + lightweight bash (git status, build checks) — no file writes or edits
- Rejection flow: teammate revises and resubmits the plan with lead's feedback
- Retry limit: 3 revision attempts before escalating (to user or lead takes over)

#### Task-level validation hooks
- Validator-to-task mapping: auto-detect from task content (e.g., task mentions 'processBlock' triggers DSP safety validator)
- Failure flow: block + auto-fix attempt on first failure; escalate to user on second failure
- Scope: only code-touching tasks get validated (.cpp, .h, .cmake, .html, .js) — docs/config tasks skip validation

### Claude's Discretion
- Exact critic domain selection per stage
- Complexity threshold for auto-approve vs gated plans
- Validation feedback format (structured JSON vs freeform — balance machine-parseability with validator authoring simplicity)

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope
</user_constraints>

## Summary

Phase 17 introduces parallel agent intelligence into the existing sequential Stage 0-4 plugin pipeline. The implementation leverages Claude Code's native Agent Teams feature (experimental, `CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS`) for research parallelism, subagent spawning for parallel critic reviews, and the `TaskCompleted` hook (exit code 2 blocks completion) for per-task validation. The existing 6 domain validators (DSP safety, APVTS matching, WebView bindings, checksums, cross-contract, resource accountability) are wired into TaskCompleted hooks that auto-detect which validators apply based on task content keywords.

The architecture splits into three integration layers: (1) a research orchestrator that spawns 2-3 agent team researchers with dynamic domain assignment and debate-based synthesis, (2) a critic orchestrator that spawns parallel subagent critics after each stage and merges findings into a unified severity-ranked report, and (3) TaskCompleted hooks that map domain validators to individual plan tasks via keyword matching on task content.

**Primary recommendation:** Build on Claude Code's native Agent Teams and TaskCompleted hooks rather than hand-rolling orchestration. Use subagents (not agent teams) for critic reviews since critics are read-only and report back to a single orchestrator. Use agent teams only for research where inter-researcher debate requires peer-to-peer messaging.

## Standard Stack

### Core

| Technology | Version | Purpose | Why Standard |
|-----------|---------|---------|--------------|
| Claude Code Agent Teams | Experimental (2026) | Parallel research with peer messaging | Native feature with shared task list, mailbox, teammate-to-teammate communication — exactly what debate format requires |
| Claude Code Subagents | Stable | Parallel critic reviews (read-only) | Each critic runs independently, reports back to orchestrator — no inter-critic communication needed |
| Claude Code TaskCompleted Hook | Stable | Per-task validation within plans | Exit code 2 prevents task completion with feedback message — maps directly to AGNT-05 requirement |
| Claude Code `permissionMode: delegate` | Stable | Restrict orchestrator to coordination-only | Built-in mode that restricts lead to team management tools (no direct file writes) — maps to AGNT-04 |
| Existing Python validators | Current | 6 domain validators | Already implemented at `.claude/hooks/validators/` — reuse rather than rebuild |

### Supporting

| Technology | Version | Purpose | When to Use |
|-----------|---------|---------|-------------|
| Agent definition files (`.claude/agents/*.md`) | Stable | Define researcher and critic agents | For each new agent role (dynamic researchers, domain critics) |
| `hooks.json` configuration | Stable | Wire TaskCompleted hooks to validators | For task-level validation enforcement |
| `jq` / Python | System | Parse task content for keyword matching | For validator-to-task auto-detection in TaskCompleted hook |
| Critic report schema | Current (`.planning/workflow/schemas/critic-report.schema.json`) | Structured critic output | For unified review report generation |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Agent Teams for research | Multiple subagents | Subagents cannot message each other — debate format impossible. Agent Teams required for peer-to-peer messaging |
| Subagents for critics | Agent Teams for critics | Agent Teams add unnecessary coordination overhead for independent read-only reviews. Subagents are cheaper and simpler |
| TaskCompleted hook for validation | PostToolUse hook on Write/Edit | PostToolUse fires on every file write — too granular. TaskCompleted fires at task boundaries — correct granularity |
| Python validators in hooks | Agent-based hooks (`type: "agent"`) | Agent hooks are more flexible but slower and costlier. Python validators are deterministic and fast. Use agent hooks only as fallback for semantic validation |

## Architecture Patterns

### Recommended Integration Structure

```
.claude/
├── agents/
│   ├── research-lead.md              # Orchestrates research team (delegate mode)
│   ├── dynamic-researcher.md         # Generic researcher (domain assigned at runtime)
│   ├── critic-orchestrator.md        # Spawns parallel critics, merges reports
│   ├── critic-dsp.md                 # (exists) DSP domain critic
│   ├── critic-ui.md                  # (exists) UI domain critic
│   ├── critic-architecture.md        # NEW: Architecture alignment critic
│   ├── critic-foundation.md          # NEW: Foundation/build system critic
│   └── [existing agents...]
├── hooks/
│   ├── hooks.json                    # Updated with TaskCompleted hooks
│   └── validators/
│       ├── task-validator-dispatch.sh # NEW: Routes task content to validators
│       ├── validate-dsp-components.py # (exists)
│       ├── validate-parameters.py     # (exists)
│       ├── validate-gui-bindings.py   # (exists)
│       ├── validate-checksums.py      # (exists)
│       ├── validate-cross-contract.py # (exists)
│       └── validate-resource-accountability.py # (exists)
├── skills/
│   └── plugin-workflow/              # Updated: research + critic integration
└── settings.json                     # Updated: enable Agent Teams, add hooks
```

### Pattern 1: Research Team Orchestration (Agent Teams)

**What:** The research-lead agent spawns 2-3 researcher teammates via Agent Teams. Each researcher gets a dynamic domain assignment based on the creative brief. Researchers read each other's findings and produce a synthesis via the mailbox system.

**When to use:** Stage 0 research phase, before planning begins.

**How it works:**

1. Research-lead reads creative brief, determines complexity, assigns domains dynamically
2. Lead spawns teammates with domain-specific prompts (e.g., "DSP algorithm research", "UI patterns and accessibility", "module compatibility audit")
3. Each researcher produces a findings document in a shared location
4. Researchers read each other's findings via Read tool and produce synthesis
5. If contradictions found, researchers message each other to debate
6. If debate reaches impasse (incompatible approaches), research-lead flags conflict and blocks planning
7. Lead merges synthesis into final research output

**Agent definition (research-lead):**
```yaml
---
name: research-lead
description: Orchestrates parallel research team for plugin planning
permissionMode: delegate
tools: Task(dynamic-researcher), Read, Bash, Grep, Glob
model: inherit
---
```

**Agent definition (dynamic-researcher):**
```yaml
---
name: dynamic-researcher
description: Performs deep domain research. Domain assigned at spawn time via prompt.
tools: Read, Grep, Glob, Bash, WebSearch, WebFetch, mcp__context7__resolve-library-id, mcp__context7__query-docs
model: inherit
---
```

**Spawning pattern (in research-lead prompt):**
```
Create an agent team to research [PluginName] from these angles:
- Teammate 1: [dynamically determined domain, e.g., "DSP algorithm approaches for shimmer reverb"]
- Teammate 2: [dynamically determined domain, e.g., "JUCE API mapping and module compatibility"]
- Teammate 3 (if complex): [dynamically determined domain, e.g., "UI/UX patterns for spectrum visualization"]

Require each researcher to read others' findings and flag contradictions.
```

### Pattern 2: Parallel Critic Review (Subagents)

**What:** After each stage completion, the critic-orchestrator spawns parallel subagent critics. Each critic produces a structured JSON report. The orchestrator merges all reports into a unified severity-ranked list.

**When to use:** After Stage 1, 2, 3, and 4 completion.

**How it works:**

1. Stage completes, orchestrator determines which critics are relevant
2. Orchestrator spawns critic subagents in parallel (read-only tools)
3. Each critic produces JSON report conforming to `critic-report.schema.json`
4. Orchestrator merges all reports: sorts issues by severity (blocker > warning > note)
5. If any blocker-severity issues exist, stage progression is blocked
6. Warnings and notes are advisory — included in report but don't block

**Critic domain selection (Claude's Discretion):**

| Stage | Always Run | Conditionally Run | Skip |
|-------|-----------|-------------------|------|
| Stage 1 (Foundation) | foundation, cross-contract | architecture (if complex) | DSP, UI |
| Stage 2 (DSP) | DSP safety, parameter integration, buffer handling | numerical stability, architecture alignment | UI |
| Stage 3 (GUI) | UI polish, parameter bindings, WebView integrity | accessibility | DSP (already validated) |
| Stage 4 (Polish) | cross-contract, resource accountability | all domain critics if changes detected | — |

**Unified report structure:**
```json
{
  "stage": "2-dsp",
  "plugin": "O-SimpleReverb",
  "timestamp": "2026-02-09T12:00:00Z",
  "critics_run": ["dsp-critic", "architecture-critic"],
  "unified_issues": [
    { "severity": "blocker", "critic": "dsp-critic", "id": "DSP-001", "description": "...", "fixSuggestion": "..." },
    { "severity": "warning", "critic": "architecture-critic", "id": "ARCH-002", "description": "...", "fixSuggestion": "..." },
    { "severity": "note", "critic": "dsp-critic", "id": "DSP-003", "description": "...", "fixSuggestion": "..." }
  ],
  "progression_allowed": false,
  "blocking_count": 1,
  "warning_count": 1,
  "note_count": 1
}
```

### Pattern 3: Plan Approval Gates

**What:** Before implementation begins, plans are evaluated for risk. Low-risk plans auto-approve; complex plans require team lead review.

**When to use:** After planning, before execution of any plan.

**How it works:**

1. Plan generated by planner agent
2. Risk assessment: count files touched, check for DSP changes, assess scope
3. If low-risk (small scope, few files, no DSP): auto-approve
4. If complex: team lead reviews, can approve or reject with feedback
5. On rejection: planner revises incorporating feedback, resubmits
6. After 3 rejections: escalate to user

**Complexity threshold recommendation (Claude's Discretion):**

Auto-approve when ALL of:
- Touches fewer than 5 files
- No `.cpp` or `.h` files in `Source/DSP/` directory
- No changes to `processBlock()` or `prepareToPlay()`
- No new JUCE module dependencies
- Complexity score < 2.0

Gate review when ANY of:
- Touches 5+ files
- Modifies DSP source files
- Adds new JUCE module dependencies
- Complexity score >= 2.0
- Creates new files in `Source/DSP/`

### Pattern 4: TaskCompleted Validation Hooks

**What:** A TaskCompleted hook dispatches domain validators based on task content keywords. Exit code 2 blocks task completion with feedback.

**When to use:** During plan execution, for every code-touching task.

**How it works:**

1. Agent marks task as completed via TaskUpdate tool
2. TaskCompleted hook fires with `task_id`, `task_subject`, `task_description`
3. Dispatch script parses task content for validator keywords
4. Matching validators run against the plugin's current files
5. Exit 0: task completes normally
6. Exit 2: task blocked, stderr feedback sent to agent
7. On first failure: agent attempts auto-fix, retries
8. On second failure: escalate to user

**Validator keyword mapping:**

| Keyword Pattern | Validator | File |
|----------------|-----------|------|
| `processBlock`, `prepareToPlay`, `DSP`, `audio processing` | DSP safety | `validate-dsp-components.py` + `validate-silent-failures.py` |
| `parameter`, `APVTS`, `createParameterLayout`, `parameter-spec` | APVTS matching | `validate-parameters.py` |
| `WebView`, `relay`, `index.html`, `binding`, `GUI` | WebView bindings | `validate-gui-bindings.py` |
| `contract`, `ARCHITECTURE.md`, `ROADMAP.md`, `BRIEF.md` | Cross-contract | `validate-cross-contract.py` |
| `checksum`, `STATUS.md`, `SHA` | Checksums | `validate-checksums.py` |
| `research`, `resource`, `frontmatter` | Resource accountability | `validate-resource-accountability.py` |

**Validation feedback format recommendation (Claude's Discretion):**

Use structured JSON for machine-parseability, but with a human-readable `message` field:

```json
{
  "validator": "validate-parameters",
  "status": "FAIL",
  "errors": [
    {
      "type": "missing_parameter",
      "parameter_id": "resonance",
      "expected_in": "PluginProcessor.cpp",
      "message": "Parameter 'resonance' defined in parameter-spec.md but not found in createParameterLayout()"
    }
  ],
  "suggestion": "Add 'resonance' parameter to createParameterLayout() in PluginProcessor.cpp"
}
```

This balances parseability (the agent can read `errors[].type` programmatically) with authoring simplicity (validators output readable `message` strings).

### Anti-Patterns to Avoid

- **Agent Teams for critics:** Critics don't need inter-agent communication. Using Agent Teams wastes tokens and adds unnecessary coordination overhead. Use subagents instead.
- **Implementation parallelization:** Per P36 constraint, Agent Teams must NOT be used for parallel code implementation. Only research (read-heavy) and review (read-only) workflows.
- **Hardcoded researcher domains:** Domains must be dynamic, not a fixed set. The research-lead determines domains at runtime based on the specific plugin.
- **Swallowing conflicts:** Research conflicts must BLOCK planning, not just warn. An unresolved DSP/UI incompatibility carried into implementation causes expensive rework.
- **Validating documentation tasks:** Only code-touching tasks (.cpp, .h, .cmake, .html, .js) need validation hooks. Running DSP validators on CHANGELOG.md edits wastes time.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parallel agent spawning | Custom process management | Claude Code Agent Teams (`CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS`) | Built-in task list, mailbox, teammate management, session isolation |
| Inter-agent messaging | File-based message queues | Agent Teams mailbox (message/broadcast) | Native race-condition-free messaging with automatic delivery |
| Task completion validation | Custom post-execution checks | `TaskCompleted` hook (exit code 2 = block) | Built-in hook fires at correct granularity, feedback loop to agent |
| Delegate mode restrictions | Tool allowlist in prompts | `permissionMode: delegate` | Built-in mode that restricts to coordination-only tools |
| Plan approval gates | Custom approval workflow | Agent Teams' built-in plan approval (`Require plan approval before they make any changes`) | Native plan mode -> review -> approve/reject flow |
| Critic report merging | Custom report aggregation | Python script reading JSON schemas | Deterministic, fast, schema-validated — no LLM overhead for merging |

**Key insight:** Claude Code already provides the orchestration primitives (Agent Teams, subagents, TaskCompleted hooks, delegate mode, plan approval). The implementation work is wiring these into the existing plugin workflow, not building orchestration infrastructure.

## Common Pitfalls

### Pitfall 1: Agent Teams Session Management

**What goes wrong:** Agent teams are experimental. Sessions cannot be resumed with in-process teammates. If the lead session compacts or crashes, teammates may become orphaned.

**Why it happens:** Agent Teams store team state in `~/.claude/teams/` and `~/.claude/tasks/`. Session resumption doesn't restore teammate connections.

**How to avoid:** Keep research team lifetimes short (single research session). Clean up teams explicitly after research completes. Design for idempotent research — if a session crashes, research can restart from scratch without data loss.

**Warning signs:** Teammates not responding to messages, lead reporting "teammate not found" errors.

### Pitfall 2: TaskCompleted Hook Performance

**What goes wrong:** TaskCompleted hooks run synchronously. If validators are slow, every task completion is delayed, making the pipeline feel sluggish.

**Why it happens:** Python validators do file I/O and regex parsing. Multiple validators on a single task multiply the delay.

**How to avoid:** Keep individual validators fast (< 2 seconds). Use the dispatch script to run ONLY relevant validators based on keyword matching. Consider using the `timeout` field in hooks.json (default 600s is too long — set to 10-15s for validators).

**Warning signs:** Task completion taking more than 5 seconds, agent appearing to "hang" between tasks.

### Pitfall 3: Debate Deadlocks in Research

**What goes wrong:** Two researchers disagree on fundamentally incompatible approaches and the debate never resolves, blocking planning indefinitely.

**Why it happens:** LLM agents can be persuasive and persistent. Without a termination condition, debate cycles.

**How to avoid:** Set a maximum debate round count (3 rounds). After 3 rounds, if no consensus, research-lead escalates to user with both positions clearly documented. Use the research-lead's delegate mode to prevent it from picking sides — it surfaces the conflict, doesn't resolve it.

**Warning signs:** Researchers sending more than 3 messages to each other on the same topic.

### Pitfall 4: Critic Domain Over-Selection

**What goes wrong:** Running all critics at every stage wastes tokens and produces noise. DSP critic at Stage 1 (foundation) has nothing meaningful to evaluate.

**Why it happens:** "More validation is better" instinct. But critics that find nothing still consume context and tokens.

**How to avoid:** Use the stage-to-critic mapping table (see Pattern 2). Only run critics whose domain is relevant to the completed stage's work.

**Warning signs:** Critic reports with all "info" severity and no actionable findings.

### Pitfall 5: Conflicting TaskCompleted and SubagentStop Hooks

**What goes wrong:** Both TaskCompleted and existing SubagentStop hooks fire validation logic, causing duplicate validation or conflicting decisions.

**Why it happens:** The existing SubagentStop hook already runs contract validation. Adding TaskCompleted creates a second validation layer.

**How to avoid:** Clearly separate concerns: SubagentStop validates the agent's overall output (contract consistency). TaskCompleted validates individual task artifacts (code quality per task). Don't duplicate validators across both hooks.

**Warning signs:** Same validation error reported twice for the same issue.

## Code Examples

### TaskCompleted Hook Dispatch Script

```bash
#!/bin/bash
# task-validator-dispatch.sh
# Called by TaskCompleted hook. Routes task content to relevant validators.
# Exit 0 = task completes. Exit 2 = task blocked with feedback.

INPUT=$(cat)
TASK_SUBJECT=$(echo "$INPUT" | jq -r '.task_subject // empty')
TASK_DESCRIPTION=$(echo "$INPUT" | jq -r '.task_description // empty')
TASK_CONTENT="$TASK_SUBJECT $TASK_DESCRIPTION"

# Skip non-code tasks
CODE_EXTENSIONS="\.cpp|\.h|\.cmake|\.html|\.js|\.css"
if ! echo "$TASK_CONTENT" | grep -qEi "$CODE_EXTENSIONS|processBlock|prepareToPlay|parameter|APVTS|WebView|relay|CMakeLists"; then
  exit 0  # Not a code task, skip validation
fi

# Detect active plugin from task content or environment
PLUGIN_NAME="${ACTIVE_PLUGIN:-}"
if [ -z "$PLUGIN_NAME" ]; then
  # Try to detect from task content
  PLUGIN_NAME=$(echo "$TASK_CONTENT" | grep -oE 'O-[A-Za-z]+' | head -1)
fi

if [ -z "$PLUGIN_NAME" ]; then
  exit 0  # Cannot determine plugin, skip validation
fi

PLUGIN_PATH="plugins/$PLUGIN_NAME"
ERRORS=""
VALIDATORS_DIR="$(dirname "$0")/validators"

# DSP safety check
if echo "$TASK_CONTENT" | grep -qEi "processBlock|prepareToPlay|DSP|audio.processing|dsp.component"; then
  RESULT=$(python3 "$VALIDATORS_DIR/validate-dsp-components.py" "$PLUGIN_PATH" 2>&1)
  if [ $? -ne 0 ]; then
    ERRORS="${ERRORS}DSP validation failed: ${RESULT}\n"
  fi
fi

# Parameter matching check
if echo "$TASK_CONTENT" | grep -qEi "parameter|APVTS|createParameterLayout|parameter.spec"; then
  RESULT=$(python3 "$VALIDATORS_DIR/validate-parameters.py" "$PLUGIN_PATH" 2>&1)
  if [ $? -ne 0 ]; then
    ERRORS="${ERRORS}Parameter validation failed: ${RESULT}\n"
  fi
fi

# WebView binding check
if echo "$TASK_CONTENT" | grep -qEi "WebView|relay|index\.html|binding|GUI|editor"; then
  RESULT=$(python3 "$VALIDATORS_DIR/validate-gui-bindings.py" "$PLUGIN_PATH" 2>&1)
  if [ $? -ne 0 ]; then
    ERRORS="${ERRORS}GUI binding validation failed: ${RESULT}\n"
  fi
fi

# Cross-contract check
if echo "$TASK_CONTENT" | grep -qEi "contract|ARCHITECTURE|ROADMAP|BRIEF|cross.contract"; then
  RESULT=$(python3 "$VALIDATORS_DIR/validate-cross-contract.py" "$PLUGIN_PATH" 2>&1)
  if [ $? -ne 0 ]; then
    ERRORS="${ERRORS}Cross-contract validation failed: ${RESULT}\n"
  fi
fi

# Checksum verification
if echo "$TASK_CONTENT" | grep -qEi "checksum|STATUS\.md|SHA|integrity"; then
  RESULT=$(python3 "$VALIDATORS_DIR/validate-checksums.py" "$PLUGIN_PATH" 2>&1)
  if [ $? -ne 0 ]; then
    ERRORS="${ERRORS}Checksum validation failed: ${RESULT}\n"
  fi
fi

# Report results
if [ -n "$ERRORS" ]; then
  echo -e "Task validation failed:\n$ERRORS" >&2
  exit 2  # Block task completion
fi

exit 0  # All validations passed
```

### hooks.json TaskCompleted Configuration

```json
{
  "hooks": {
    "TaskCompleted": [
      {
        "hooks": [
          {
            "type": "command",
            "command": "\"$CLAUDE_PROJECT_DIR\"/.claude/hooks/task-validator-dispatch.sh",
            "timeout": 15
          }
        ]
      }
    ]
  }
}
```

### Unified Critic Report Merger (Python)

```python
#!/usr/bin/env python3
"""
merge-critic-reports.py
Merges multiple critic JSON reports into a unified severity-ranked report.
"""
import json
import sys
from pathlib import Path

SEVERITY_ORDER = {"blocker": 0, "error": 1, "warning": 2, "note": 3, "info": 4}

def merge_reports(report_paths: list[Path]) -> dict:
    """Merge critic reports into unified severity-ranked output."""
    all_issues = []
    critics_run = []

    for path in report_paths:
        with open(path) as f:
            report = json.load(f)

        critic_name = report.get("critic", "unknown")
        critics_run.append(critic_name)

        for issue in report.get("issues", []):
            severity = issue.get("severity", "note")
            # Normalize "error" to "blocker" for unified report
            if severity == "error":
                severity = "blocker"
            all_issues.append({
                "severity": severity,
                "critic": critic_name,
                "id": issue.get("id", "UNKNOWN"),
                "category": issue.get("category", ""),
                "location": issue.get("location", ""),
                "description": issue.get("description", ""),
                "fixSuggestion": issue.get("fixSuggestion", "")
            })

    # Sort by severity
    all_issues.sort(key=lambda x: SEVERITY_ORDER.get(x["severity"], 99))

    blocker_count = sum(1 for i in all_issues if i["severity"] == "blocker")
    warning_count = sum(1 for i in all_issues if i["severity"] == "warning")
    note_count = sum(1 for i in all_issues if i["severity"] in ("note", "info"))

    return {
        "critics_run": critics_run,
        "unified_issues": all_issues,
        "progression_allowed": blocker_count == 0,
        "blocking_count": blocker_count,
        "warning_count": warning_count,
        "note_count": note_count
    }
```

### Research Conflict Detection Logic

```python
#!/usr/bin/env python3
"""
detect-research-conflicts.py
Analyzes researcher outputs for contradictions.
Returns conflicts that should block planning.
"""
import json
import re
import sys
from pathlib import Path

def detect_conflicts(findings: list[dict]) -> list[dict]:
    """
    Compare researcher findings for incompatible approaches.

    Each finding dict has:
    - domain: str (e.g., "dsp-algorithms", "ui-patterns")
    - recommendations: list[str]
    - juce_modules: list[str]
    - approach: str (description of recommended approach)
    """
    conflicts = []

    for i, f1 in enumerate(findings):
        for f2 in findings[i+1:]:
            # Check for module conflicts
            shared_modules = set(f1.get("juce_modules", [])) & set(f2.get("juce_modules", []))
            # Check for approach incompatibility
            # (Semantic analysis — would need LLM for deep comparison)
            # For deterministic check: look for explicit contradictions
            f1_approach = f1.get("approach", "").lower()
            f2_approach = f2.get("approach", "").lower()

            # Detect explicit contradictions
            contradiction_pairs = [
                ("time-domain", "frequency-domain"),
                ("mono processing", "stereo processing"),
                ("zero-latency", "lookahead"),
                ("stateless", "stateful"),
            ]

            for term_a, term_b in contradiction_pairs:
                if (term_a in f1_approach and term_b in f2_approach) or \
                   (term_b in f1_approach and term_a in f2_approach):
                    conflicts.append({
                        "type": "incompatible_approach",
                        "domain_a": f1["domain"],
                        "domain_b": f2["domain"],
                        "conflict": f"Researcher A recommends '{term_a}' but Researcher B recommends '{term_b}'",
                        "blocking": True
                    })

    return conflicts
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Sequential single-agent research | Parallel Agent Teams for research | Claude Code Agent Teams (2026, experimental) | 2-3x research throughput for complex plugins |
| Post-stage gate validation only | Per-task validation via TaskCompleted hooks | Claude Code hooks system (2025-2026) | Catches issues earlier, before full stage completion |
| Manual critic invocation | Automatic parallel critic spawning | Subagent improvements (2025-2026) | Consistent quality review at every stage boundary |
| Fixed critic domains per stage | Dynamic critic selection based on stage+plugin type | This phase implementation | Reduces token waste from irrelevant critics |
| Flat issue reporting | Severity-ranked unified reports (blocker/warning/note) | This phase implementation | Clearer prioritization, blocking only on true blockers |

**Experimental features used:**
- Agent Teams: requires `CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS=1` in settings. Known limitations: no session resumption with in-process teammates, task status can lag, one team per session.

## Open Questions

1. **Agent Teams stability for production use**
   - What we know: Agent Teams are experimental with known limitations (no session resumption, task status lag, shutdown can be slow)
   - What's unclear: How reliable are Agent Teams for a 10-30 minute research session with 2-3 teammates?
   - Recommendation: Design for graceful degradation. If Agent Teams fail, fall back to sequential subagent research (each researcher runs as a subagent, debate happens via shared files rather than mailbox). Test with canary plugin (O-SimpleReverb) first.

2. **Token cost of parallel critics at every stage**
   - What we know: Each critic subagent consumes its own context window. Running 2-3 critics per stage = 8-12 critic invocations per plugin.
   - What's unclear: Is the token cost acceptable for the quality improvement?
   - Recommendation: Start with minimal critic sets per stage (see domain selection table). Add critics only when value demonstrated. Track token metrics via existing `tokenMetrics` field in critic reports.

3. **Debate format specifics**
   - What we know: Researchers read each other's outputs and produce synthesis. Agent Teams support direct messaging.
   - What's unclear: Exact protocol for debate rounds (how many messages, what triggers escalation, what format for synthesis).
   - Recommendation: Start with 3-round maximum: Round 1 = initial findings, Round 2 = read others + identify contradictions, Round 3 = synthesis attempt. If no consensus after Round 3, escalate. Refine protocol based on canary testing.

4. **Auto-fix behavior in TaskCompleted hooks**
   - What we know: Decision says "block + auto-fix attempt on first failure". TaskCompleted hook can only block — it cannot modify code.
   - What's unclear: How does "auto-fix" work if the hook can only send feedback via stderr?
   - Recommendation: The hook blocks and sends feedback. The executing agent receives the feedback as a message and attempts the fix. The fix attempt is the agent's normal behavior (it reads the error, edits code, retries). This is not a hook-level auto-fix — it's the agent responding to hook feedback.

## Sources

### Primary (HIGH confidence)
- [Claude Code Subagents Documentation](https://code.claude.com/docs/en/sub-agents) — Agent definition, tools restriction, delegate mode, permission modes
- [Claude Code Agent Teams Documentation](https://code.claude.com/docs/en/agent-teams) — Team spawning, mailbox, plan approval, delegate mode, TaskCompleted hook reference
- [Claude Code Hooks Reference](https://code.claude.com/docs/en/hooks) — TaskCompleted hook input schema, exit code 2 behavior, hook configuration
- Context7 `/anthropics/claude-code` — Agent frontmatter fields, tool restrictions, SubagentStop hooks
- Existing codebase: `.claude/hooks/hooks.json` — Current hook configuration
- Existing codebase: `.claude/hooks/validators/` — 6 domain validators (Python)
- Existing codebase: `.claude/critics/critic-dsp.md`, `critic-ui.md` — Critic templates with scoring
- Existing codebase: `.planning/workflow/schemas/critic-report.schema.json` — Report schema
- Existing codebase: `.planning/workflow/schemas/gate-report.schema.json` — Gate report schema

### Secondary (MEDIUM confidence)
- Existing codebase: `.claude/agents/validation-agent.md` — Validator agent patterns, tiered pluginval
- Existing codebase: `.claude/agents/research-planning-agent.md` — Stage 0 research protocol
- Existing codebase: `.planning/workflow/scripts/run-critic.sh` — Critic orchestration framework

### Tertiary (LOW confidence)
- Token cost estimates for agent teams: based on documentation notes "significantly more tokens" — exact multipliers unknown, needs empirical measurement

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — Built entirely on documented, verified Claude Code features (Agent Teams, subagents, TaskCompleted hooks, delegate mode)
- Architecture: HIGH — Patterns map directly to existing infrastructure (agents, hooks, validators, critic schemas) with clear integration points
- Pitfalls: HIGH — Derived from official documentation limitations sections and existing codebase knowledge of validator performance

**Research date:** 2026-02-09
**Valid until:** 2026-03-09 (Agent Teams experimental status may change)
