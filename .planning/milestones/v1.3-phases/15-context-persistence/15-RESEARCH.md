# Phase 15: Context Persistence - Research

**Researched:** 2026-02-08
**Domain:** Claude Code context management, compaction hooks, agent memory, plugin state persistence
**Confidence:** HIGH

## Summary

Phase 15 addresses information loss across three boundaries: (1) context compaction events that discard plugin-specific details like parameter IDs and DSP component names, (2) session boundaries where agent learnings are lost, and (3) express mode skipping phases instead of auto-generating them. The codebase already has a `PreCompact.sh` hook and `context-resume` skill, but both have significant gaps.

The current `PreCompact.sh` dumps all plugin contracts to stdout, but **PreCompact stdout is NOT injected into post-compaction context** -- only `SessionStart` stdout gets added as context (per official docs). This means the current hook's output is visible only in verbose mode and effectively lost after compaction. The fix requires a two-stage pipeline: PreCompact writes a snapshot file, then a `SessionStart` hook with matcher `"compact"` reads that file and outputs it as context.

For agent memory, Claude Code's `SubagentStart` hook supports an `additionalContext` field that gets injected into the subagent's context. This is the mechanism for loading per-agent persistent memory. Combined with `SubagentStop` for writing learned patterns, this creates a read/write cycle for agent memory files.

**Primary recommendation:** Implement a PreCompact-to-SessionStart(compact) pipeline for compaction survival, create per-plugin DIGEST.json files for fast cross-stage context loading, add `.claude/agent-memory/` files for the five key agents, and convert express mode to auto-generate plans instead of skipping them.

## Standard Stack

This phase does not introduce new libraries. It modifies hook scripts, agent definitions, and creates new structured data files.

### Core
| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| Claude Code Hooks | Current | PreCompact, SessionStart, SubagentStart lifecycle hooks | Official API for context persistence |
| Bash/Python | System | Hook scripts | Existing infrastructure |
| JSON | - | DIGEST.json structured data format | Machine-readable, token-efficient, parseable by any agent |

### Key API Capabilities (Verified)
| Hook Event | Key Feature | Confidence |
|------------|-------------|------------|
| `PreCompact` | Fires before compaction; stdout shown in verbose mode only; can write files | HIGH (official docs) |
| `SessionStart` (matcher: `compact`) | Fires after compaction; stdout injected as context; `additionalContext` field supported | HIGH (official docs) |
| `SubagentStart` | Fires when subagent spawns; `additionalContext` injected into subagent context | HIGH (official docs) |
| `SubagentStop` | Fires when subagent finishes; can run commands to persist state | HIGH (official docs) |
| CLAUDE.md `@import` | Imports files into context; supports relative/absolute paths | HIGH (official docs) |

## Architecture Patterns

### Recommended File Structure

```
.claude/
├── hooks/
│   ├── PreCompact.sh              # MODIFIED: Write domain-aware snapshot
│   ├── SessionStart.sh            # EXISTING: Environment validation
│   └── PostCompact-SessionStart.sh # NEW: Read snapshot, inject context
├── agent-memory/                   # NEW: Per-agent persistent memory
│   ├── troubleshoot-agent.md
│   ├── dsp-agent.md
│   ├── gui-agent.md
│   ├── research-planning-agent.md
│   └── validation-agent.md
├── compaction-snapshot.md          # NEW: Written by PreCompact, read by SessionStart(compact)
└── settings.json                   # MODIFIED: Add SessionStart compact handler

plugins/[Name]/.planning/
├── DIGEST.json                     # NEW: Structured stage decisions (< 500 tokens)
├── STATUS.md                       # EXISTING: Full state
├── BRIEF.md                        # EXISTING: Creative brief
├── parameter-spec.md               # EXISTING: Parameters
└── research/ARCHITECTURE.md        # EXISTING: DSP architecture
```

### Pattern 1: PreCompact-to-SessionStart(compact) Pipeline

**What:** Two-stage context preservation across compaction events. PreCompact writes a snapshot file; SessionStart(compact) reads it and injects into context.

**Why this pattern:** PreCompact stdout is NOT injected into post-compaction context (only `SessionStart` and `UserPromptSubmit` stdout is). The current `PreCompact.sh` prints to stdout, which means its output is effectively lost after compaction.

**Implementation:**

**Stage 1 - PreCompact.sh (write snapshot):**
```bash
#!/bin/bash
# PreCompact - Write domain-aware snapshot before compaction
# This script's stdout is NOT injected into post-compaction context
# Instead, it writes a snapshot file that SessionStart(compact) reads

SNAPSHOT=".claude/compaction-snapshot.md"

{
  echo "# Active Context Snapshot"
  echo "Generated: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo ""

  # Detect active/focused plugin from registry
  FOCUSED=$(python3 -c "
import json, sys
try:
    with open('.claude/plugin-registry.json') as f:
        reg = json.load(f)
    print(reg.get('focused', ''))
except: pass
" 2>/dev/null)

  # Find in-progress plugins from PLUGINS.md
  IN_PROGRESS=$(grep -E '🚧' PLUGINS.md 2>/dev/null | head -3)

  if [ -n "$FOCUSED" ]; then
    echo "## Active Plugin: $FOCUSED"
    echo ""

    PLUGIN_DIR="plugins/$FOCUSED/.planning"

    # Load DIGEST.json if exists (most efficient)
    if [ -f "$PLUGIN_DIR/DIGEST.json" ]; then
      echo "### Context Digest"
      cat "$PLUGIN_DIR/DIGEST.json"
      echo ""
    fi

    # Load STATUS.md frontmatter
    if [ -f "$PLUGIN_DIR/STATUS.md" ]; then
      echo "### Current State"
      sed -n '/^---$/,/^---$/p' "$PLUGIN_DIR/STATUS.md"
      echo ""
    fi

    # Load parameter IDs (compact format)
    if [ -f "$PLUGIN_DIR/parameter-spec.md" ]; then
      echo "### Parameter IDs"
      grep -E '^\|.*\|.*\|' "$PLUGIN_DIR/parameter-spec.md" | head -20
      echo ""
    fi

    # Contract paths
    echo "### Contract Paths"
    for f in BRIEF.md parameter-spec.md research/ARCHITECTURE.md ROADMAP.md; do
      [ -f "$PLUGIN_DIR/$f" ] && echo "- $PLUGIN_DIR/$f"
    done
    echo ""
  fi

  if [ -n "$IN_PROGRESS" ]; then
    echo "## In-Progress Plugins"
    echo "$IN_PROGRESS"
    echo ""
  fi

} > "$SNAPSHOT"

exit 0
```

**Stage 2 - SessionStart(compact) handler (inject context):**
```bash
#!/bin/bash
# PostCompact context injection via SessionStart(compact)
# Stdout from this script IS injected into Claude's context

SNAPSHOT=".claude/compaction-snapshot.md"

if [ -f "$SNAPSHOT" ]; then
  cat "$SNAPSHOT"
  echo ""
  echo "---"
  echo "Context restored from pre-compaction snapshot."
  echo "Use /continue [PluginName] to resume work."
fi

exit 0
```

**settings.json update:**
```json
{
  "hooks": {
    "SessionStart": [
      {
        "hooks": [{
          "type": "command",
          "command": "${CLAUDE_PROJECT_DIR}/.claude/hooks/SessionStart.sh",
          "timeout": 5000
        }]
      },
      {
        "matcher": "compact",
        "hooks": [{
          "type": "command",
          "command": "${CLAUDE_PROJECT_DIR}/.claude/hooks/PostCompact-SessionStart.sh",
          "timeout": 5000
        }]
      }
    ]
  }
}
```

**Source:** [Claude Code Hooks Reference](https://code.claude.com/docs/en/hooks) -- SessionStart matcher table shows `compact` matches "Auto or manual compaction"; SessionStart decision control shows stdout is added as context.

### Pattern 2: Per-Plugin DIGEST.json

**What:** A structured JSON file at `plugins/[Name]/.planning/DIGEST.json` that compiles stage decisions into a token-efficient format loadable by any agent.

**When to use:** Created/updated at each stage transition. Loaded by PreCompact for snapshot, by context-resume for fast status, and by any agent needing cross-stage context.

**Budget:** Must stay under 500 tokens (~2000 characters). This means: IDs and names only, no descriptions or rationale.

**Example:**
```json
{
  "plugin": "O-GrainScatter",
  "version": "1.0.0",
  "stage": "complete",
  "phase": "verified",
  "complexity": 48,
  "parameters": [
    "grainSize", "density", "spread", "reverse", "feedback",
    "dryWet", "pitchRandom", "panRandom", "scale", "rootNote",
    "pitchMode", "syncMode", "probability", "repeats",
    "stutterGate", "freeze", "euclidPulses", "euclidSteps"
  ],
  "dsp_components": [
    "DelayBuffer", "GrainPool", "GrainScheduler",
    "TempoTracker", "FreezeManager", "ScaleQuantizer",
    "EuclideanGenerator"
  ],
  "contracts": {
    "brief": ".planning/BRIEF.md",
    "params": ".planning/parameter-spec.md",
    "arch": ".planning/research/ARCHITECTURE.md",
    "roadmap": ".planning/ROADMAP.md"
  },
  "decisions": {
    "stage_0": "JI tuning with 5-limit for triads",
    "stage_1": "WebView editor, 17 APVTS params",
    "stage_2": "7 DSP components, granular engine",
    "stage_3": "Naturalist aesthetic, grain viz",
    "stage_4": "Freeze crossfade, soft-clipping"
  }
}
```

**Creation points:**
- Stage 0 complete (research-planning-agent) → initial DIGEST with params and DSP components
- Stage 1-4 complete (plugin-workflow) → append stage decision summary
- `/improve` complete → update version and add improvement note

### Pattern 3: Agent Persistent Memory via SubagentStart/SubagentStop

**What:** Each of the five key agents has a `.claude/agent-memory/{agent-name}.md` file. The `SubagentStart` hook injects the file content as `additionalContext` when the agent spawns. The agent writes learned patterns to the file at the end of its work.

**When to use:** For the five agents specified in CTXP-04: troubleshoot, dsp, gui, research-planning, validation.

**SubagentStart hook for memory injection:**
```json
{
  "SubagentStart": [
    {
      "matcher": "troubleshoot-agent|dsp-agent|gui-agent|research-planning-agent|validation-agent",
      "hooks": [{
        "type": "command",
        "command": "${CLAUDE_PROJECT_DIR}/.claude/hooks/inject-agent-memory.sh",
        "timeout": 3000
      }]
    }
  ]
}
```

```bash
#!/bin/bash
# inject-agent-memory.sh - Load persistent memory for agents
INPUT=$(cat)
AGENT_TYPE=$(echo "$INPUT" | jq -r '.agent_type // empty')

MEMORY_FILE=".claude/agent-memory/${AGENT_TYPE}.md"

if [ -f "$MEMORY_FILE" ]; then
  CONTENT=$(cat "$MEMORY_FILE")
  jq -n --arg ctx "$CONTENT" '{
    hookSpecificOutput: {
      hookEventName: "SubagentStart",
      additionalContext: $ctx
    }
  }'
else
  exit 0
fi
```

**Memory file structure:**
```markdown
# DSP Agent Memory

## Learned Patterns
- O-GrainScatter: std::tanh soft-clipping prevents digital clipping from grain overlap
- O-FreqPulse: FFT bin interpolation needed for smooth spectral sequencing
- O-Bells: Physical model requires 3-stage decay (attack transient, body, tail)

## Common Issues
- APVTS attachment must be destroyed BEFORE processor destructor
- ScopedNoDenormals critical for ARM/Apple Silicon performance

## Last Updated: 2026-02-08
```

**Memory write mechanism:** Each agent's instruction file (e.g., `dsp-agent.md`) gets a new section instructing it to append notable learnings to its memory file before returning its JSON report. This is simpler and more reliable than a SubagentStop hook.

**Size management:** Memory files should stay under 100 lines. Agents are instructed to prune old entries when appending new ones, keeping only the most useful patterns.

### Pattern 4: Express Auto Mode (--auto flag)

**What:** A new `--auto` flag for `/implement` that generates plans via auto mode instead of skipping planning entirely.

**Current behavior of `--express`:**
- Skips interactive discuss (creates CONTEXT.md from existing docs)
- Still runs research, plan, execute, verify
- Auto-progresses through stages without decision menus

**Problem (CTXP-03):** Express mode skips interactive discuss but still runs plan. However, the requirement says `/implement --auto` should "generate plans via auto mode instead of skipping planning entirely." This implies there is or should be a mode that auto-generates the discuss/research context without user interaction.

**New `--auto` flag behavior:**
- Auto-discuss: Generate CONTEXT.md from BRIEF.md and parameter-spec.md without user questions
- Auto-research: Run research-planning-agent with existing contracts as input, no interactive refinement
- Auto-plan: Generate PLAN.md from auto-research output
- Execute and verify: Same as normal
- Falls back to manual on any error (same as --express)

**Implementation location:** Modify `/implement` command (`commands/implement.md`) and `plugin-workflow` skill (`skills/plugin-workflow/SKILL.md`, `references/workflow-mode.md`).

### Pattern 5: Full Research Loading for Complex DSP (CTXP-01)

**What:** When `ROADMAP.md` shows complexity >= 4, the dsp-agent receives full research document paths in its handoff prompt instead of summaries.

**Current behavior:** The plugin-workflow dispatcher always passes the same contract file paths to dsp-agent regardless of complexity. The dsp-agent reads these files itself.

**Change needed:** The orchestrator prompt for dsp-agent should, for complexity >= 4:
1. Include all research documents from `plugins/[Name]/.planning/research/` directory
2. Include full ARCHITECTURE.md (not a summary)
3. Include any DSP-specific research documents (e.g., `research/circuit-modeling-fundamentals.md`)

**Mechanism:** The `stage-2-dsp.md` dispatcher reference file needs to add a complexity check that expands the file list passed in the Task prompt for dsp-agent.

### Anti-Patterns to Avoid
- **Printing to stdout in PreCompact and expecting context injection:** PreCompact stdout is only visible in verbose mode -- NOT injected into context. Use the PreCompact→snapshot→SessionStart(compact) pipeline instead.
- **Putting full contract content in DIGEST.json:** The digest must stay under 500 tokens. Use IDs, names, and one-line summaries only.
- **Agent memory files growing unbounded:** Without pruning, memory files will become noise. Cap at 100 lines and instruct agents to prune.
- **Using CLAUDE.md @import for dynamic plugin context:** CLAUDE.md is static and loaded at session start. It cannot dynamically import the "currently active" plugin's contracts because the active plugin changes.
- **Relying on PreCompact to block compaction:** PreCompact cannot block (exit code 2 just shows stderr to user). It is informational only.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Post-compaction context injection | Custom compaction interceptor | SessionStart(compact) hook with stdout | Official API; stdout from SessionStart is added as context |
| Agent memory loading | Custom file-read instructions per agent | SubagentStart hook with additionalContext | Official API; reliably injects into every subagent spawn |
| Plugin state summary | Full contract dump | DIGEST.json (< 500 tokens) | Compact, structured, parseable, fast to load |
| Express planning | Interactive mode bypass | Auto-generated CONTEXT.md from contracts | Produces proper planning artifacts instead of skipping |

**Key insight:** Claude Code's hook system already provides the lifecycle events needed for context persistence. The gap is not in the API but in how the existing hooks use it -- PreCompact output goes nowhere useful, and SubagentStart/SubagentStop are not used at all.

## Common Pitfalls

### Pitfall 1: PreCompact Stdout Misconception
**What goes wrong:** Developers assume PreCompact stdout is injected into post-compaction context, like SessionStart does.
**Why it happens:** The docs state "Exit 0 means success. Claude Code parses stdout for JSON output fields" and "for most events, stdout is only shown in verbose mode (Ctrl+O). The exceptions are UserPromptSubmit and SessionStart."
**How to avoid:** Always use the two-stage pipeline: PreCompact writes to file, SessionStart(compact) reads file and outputs to stdout.
**Warning signs:** After compaction, Claude doesn't know the current plugin name or stage -- the snapshot isn't reaching context.

### Pitfall 2: Snapshot File Staleness
**What goes wrong:** The `.claude/compaction-snapshot.md` file contains stale data from a previous session's compaction, and SessionStart(compact) injects outdated context.
**Why it happens:** The snapshot persists on disk between sessions.
**How to avoid:** PreCompact always overwrites the snapshot (not appends). Add a timestamp to the snapshot. SessionStart(compact) can optionally check if the snapshot is recent (within the current session).
**Warning signs:** After compaction, context refers to a different plugin than the one being worked on.

### Pitfall 3: DIGEST.json Token Budget Overflow
**What goes wrong:** DIGEST.json grows beyond 500 tokens as more stage decisions and parameters accumulate.
**Why it happens:** Complex plugins have 20+ parameters and 10+ DSP components.
**How to avoid:** Use short IDs only (not full descriptions). Limit decisions to one line per stage. Validate token count at creation time.
**Warning signs:** Agent loading becomes slow; DIGEST exceeds 2000 characters.

### Pitfall 4: Agent Memory Unbounded Growth
**What goes wrong:** Agent memory files grow to hundreds of lines, adding noise to every subagent invocation.
**Why it happens:** No pruning mechanism; agents keep appending but never removing.
**How to avoid:** Cap memory files at 100 lines. Instruct agents to review and prune when appending. Consider a rotation strategy (newest 50 entries kept).
**Warning signs:** SubagentStart hook timeout increases; agent context contains irrelevant old patterns.

### Pitfall 5: SessionStart Hook Ordering
**What goes wrong:** The compact-triggered SessionStart runs BEFORE the general SessionStart, and environment validation output appears after the snapshot context.
**Why it happens:** Multiple SessionStart entries fire in array order, and matchers may overlap.
**How to avoid:** Put the compact handler AFTER the general handler in the settings.json array. The general handler (no matcher) fires on ALL session starts; the compact handler (matcher: "compact") fires ONLY on compact events. Both will fire on compact, but the general one provides environment info first.
**Warning signs:** Post-compaction context appears without environment validation.

### Pitfall 6: SubagentStart Matcher Case Sensitivity
**What goes wrong:** The SubagentStart hook doesn't fire for agents because the matcher doesn't match the agent_type exactly.
**Why it happens:** Agent names are defined in frontmatter (e.g., "dsp-agent") and the matcher is a regex.
**How to avoid:** Test the matcher against actual agent_type values from SubagentStart input. Use exact agent names joined with `|`.
**Warning signs:** Agent spawns without its memory context; memory file is never read.

## Code Examples

### Example 1: settings.json Hook Configuration

```json
{
  "hooks": {
    "SessionStart": [
      {
        "hooks": [{
          "type": "command",
          "command": "${CLAUDE_PROJECT_DIR}/.claude/hooks/SessionStart.sh",
          "timeout": 5000
        }]
      },
      {
        "matcher": "compact",
        "hooks": [{
          "type": "command",
          "command": "${CLAUDE_PROJECT_DIR}/.claude/hooks/PostCompact-SessionStart.sh",
          "timeout": 5000
        }]
      }
    ],
    "PostToolUse": [
      {
        "matcher": "Write|Edit",
        "hooks": [{
          "type": "command",
          "command": "${CLAUDE_PROJECT_DIR}/.claude/hooks/PostToolUse.sh",
          "timeout": 2000
        }]
      }
    ],
    "PreCompact": [
      {
        "hooks": [{
          "type": "command",
          "command": "${CLAUDE_PROJECT_DIR}/.claude/hooks/PreCompact.sh",
          "timeout": 10000
        }]
      }
    ],
    "SubagentStart": [
      {
        "matcher": "troubleshoot-agent|dsp-agent|gui-agent|research-planning-agent|validation-agent",
        "hooks": [{
          "type": "command",
          "command": "${CLAUDE_PROJECT_DIR}/.claude/hooks/inject-agent-memory.sh",
          "timeout": 3000
        }]
      }
    ]
  }
}
```

Source: [Claude Code Hooks Reference](https://code.claude.com/docs/en/hooks)

### Example 2: DIGEST.json Creation Script

```bash
#!/bin/bash
# create-digest.sh - Generate DIGEST.json for a plugin
# Usage: create-digest.sh <plugin_name>

PLUGIN="$1"
PLANNING="plugins/$PLUGIN/.planning"
DIGEST="$PLANNING/DIGEST.json"

# Extract data from contracts
STAGE=$(grep -E '^stage:' "$PLANNING/STATUS.md" 2>/dev/null | sed 's/stage: *//' || echo "unknown")
PHASE=$(grep -E '^phase:' "$PLANNING/STATUS.md" 2>/dev/null | sed 's/phase: *//' || echo "unknown")
COMPLEXITY=$(grep -E 'complexity|score' "$PLANNING/ROADMAP.md" 2>/dev/null | grep -oE '[0-9]+' | head -1 || echo "0")

# Extract parameter IDs from parameter-spec.md
PARAMS=$(grep -E '^\| [a-z]' "$PLANNING/parameter-spec.md" 2>/dev/null | awk -F'|' '{print $2}' | tr -d ' ' | jq -R . | jq -s .)

# Extract DSP components from ARCHITECTURE.md
DSP=$(grep -E 'juce::dsp::|class [A-Z]' "$PLANNING/research/ARCHITECTURE.md" 2>/dev/null | sed 's/.*:://' | sed 's/<.*//' | sed 's/class //' | head -10 | jq -R . | jq -s .)

python3 -c "
import json
digest = {
    'plugin': '$PLUGIN',
    'stage': '$STAGE',
    'phase': '$PHASE',
    'complexity': int('$COMPLEXITY' or 0),
    'parameters': $PARAMS,
    'dsp_components': $DSP,
    'contracts': {
        'brief': '.planning/BRIEF.md',
        'params': '.planning/parameter-spec.md',
        'arch': '.planning/research/ARCHITECTURE.md',
        'roadmap': '.planning/ROADMAP.md'
    },
    'decisions': {}
}
print(json.dumps(digest, indent=2))
" > "$DIGEST"

echo "Created $DIGEST"
```

### Example 3: Agent Memory Instruction Block

Add to each of the five agents' `.md` files:

```markdown
<persistent_memory>
## Persistent Memory

At the START of your task, your memory file has been loaded via SubagentStart hook.
Review any patterns relevant to the current plugin.

At the END of your task (before returning your JSON report):
1. If you learned a notable pattern, workaround, or insight, append it to your memory file
2. Format: `- [PluginName]: [one-line description of learning]`
3. Only add genuinely useful patterns (not obvious things)
4. If memory file exceeds 80 lines, remove the oldest 20 entries
5. Write using: `.claude/agent-memory/{agent-name}.md`

Memory file path: `.claude/agent-memory/{agent-name}.md`
</persistent_memory>
```

### Example 4: Workflow Mode Auto Flag

Add to `commands/implement.md`:
```markdown
| `--auto` | Auto-generate discuss/research/plan without user interaction |
```

Add to `references/workflow-mode.md`:
```javascript
// Auto mode: generates plans without user interaction
// Different from express: express skips, auto generates
if (mode === "auto") {
  // Auto-discuss: Generate CONTEXT.md from existing contracts
  // Auto-research: Run research-planning-agent non-interactively
  // Auto-plan: Generate PLAN.md from auto-research
  // Execute and verify: Same as normal
  // Falls back to manual on error
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| PreCompact stdout for context | PreCompact write file + SessionStart(compact) read | Phase 15 | Fixes context loss after compaction |
| No agent memory | `.claude/agent-memory/` per agent | Phase 15 | Patterns persist across sessions |
| Full contract dump to context | DIGEST.json (< 500 tokens) | Phase 15 | Fast, token-efficient state loading |
| Express mode skips planning | Auto mode generates plans | Phase 15 | Proper plans for express creation |

**Deprecated/outdated:**
- Current `PreCompact.sh` printing to stdout: Ineffective -- output is not injected into post-compaction context
- Full contract dump in PreCompact: Token-wasteful -- DIGEST.json is more efficient

## Open Questions

1. **PreCompact stdout behavior verification**
   - What we know: Official docs say "For most events, stdout is only shown in verbose mode. The exceptions are UserPromptSubmit and SessionStart." PreCompact is not listed as an exception.
   - What's unclear: Whether PreCompact stdout gets included in the compaction summary or is truly discarded. The current PreCompact.sh outputs to stdout and the system has been functioning.
   - Recommendation: Implement the two-stage pipeline (PreCompact writes file, SessionStart(compact) reads it) regardless. This is the documented safe approach. If PreCompact stdout IS included, the snapshot file is just redundant (not harmful).

2. **SubagentStart hook agent_type matching**
   - What we know: SubagentStart docs show `agent_type` contains the agent name. The matcher is a regex.
   - What's unclear: Whether the agent_type for custom agents in `.claude/agents/` uses the frontmatter `name:` field (e.g., "dsp-agent") or the filename (e.g., "dsp-agent"). They match in this codebase, but this should be verified.
   - Recommendation: Test with one agent first (e.g., dsp-agent) before rolling out to all five. Check the SubagentStart input JSON to confirm the agent_type value.

3. **Auto mode scope for CTXP-03**
   - What we know: The requirement says "/implement --auto generates plans via auto mode instead of skipping planning entirely."
   - What's unclear: Whether "skipping planning entirely" means the current express mode behavior, or a hypothetical `--auto` flag that doesn't exist yet. The current `--express` flag still runs plan phase.
   - Recommendation: Implement `--auto` as a distinct flag separate from `--express`. `--auto` auto-generates all phases (discuss, research, plan) without user interaction. `--express` auto-advances between phases but still pauses at decision gates for discuss.

## Sources

### Primary (HIGH confidence)
- [Claude Code Hooks Reference](https://code.claude.com/docs/en/hooks) - Complete hook event documentation, input/output schemas, decision control, SessionStart context injection behavior
- [Claude Code Memory](https://code.claude.com/docs/en/memory) - CLAUDE.md, auto memory, @import syntax, memory hierarchy
- Codebase audit (this session) - PreCompact.sh, SessionStart.sh, settings.json, all 11 agents, STATUS.md format, plugin-workflow skill, implement command

### Secondary (MEDIUM confidence)
- [GitHub Issue #17237](https://github.com/anthropics/claude-code/issues/17237) - Feature request for PreCompact/PostCompact hooks, confirms community patterns for context preservation
- [Context7: Claude Code docs](/anthropics/claude-code) - SubagentStart additionalContext injection, hook handler patterns
- [Everything Claude Code](/affaan-m/everything-claude-code) - Community patterns for hook-based context management

### Tertiary (LOW confidence)
- [Medium: Context Recovery](https://medium.com/coding-nexus/claude-code-context-recovery-stop-losing-progress-when-context-compacts-772830ee7863) - Community approach to PreCompact context recovery (unverified)
- [DEV.to: Context Injection](https://dev.to/sasha_podles/claude-code-using-hooks-for-guaranteed-context-injection-2jg) - Context injection patterns (unverified)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Official Claude Code hooks documentation verified via WebFetch
- Architecture (PreCompact pipeline): HIGH - Based on official docs confirming SessionStart stdout injection and PreCompact non-injection
- Architecture (Agent memory): HIGH - SubagentStart additionalContext confirmed in official docs
- Architecture (DIGEST.json): HIGH - Pure file format design, no API dependency
- Architecture (Auto mode): MEDIUM - Requirement interpretation may need user validation
- Pitfalls: HIGH - PreCompact stdout non-injection confirmed by official docs; all other pitfalls from direct codebase analysis

**Research date:** 2026-02-08
**Valid until:** 2026-03-08 (30 days -- hooks API is stable; DIGEST format is project-specific)
