# Phase 4: Verification Infrastructure - Research

**Researched:** 2026-01-30
**Domain:** Generator-critic loops, iterative refinement, domain-specific validation, token budget management
**Confidence:** HIGH

## Summary

Phase 4 builds verification infrastructure through generator-critic loops that catch issues before stage transitions. The research investigated industry patterns for iterative refinement (Self-Refine, Evaluator-Optimizer), domain-specific validation approaches for DSP and UI, and token budget management strategies.

The existing infrastructure provides a strong foundation: GSD's verify-phase pattern with goal-backward analysis, the validation-agent with tiered pluginval execution, and JSON Schema validation in Phase 3's handoff system. The key extension is adding iterative refinement where the same agent receives critic feedback and fixes in-place (per CONTEXT.md decisions), with domain-specific critics that encode DSP real-time safety rules and UI polish standards.

The recommended approach: (1) create a critic-report schema extending validator-report.json with scores and fix suggestions, (2) implement domain critics as critic-dsp.md and critic-ui.md prompt templates, (3) add iteration orchestration with "Attempt N/3" awareness and early-stop conditions, (4) integrate token budget tracking with warn-not-block behavior.

**Primary recommendation:** Model critics on GSD's verification pattern but add numeric scoring (1-10) per category, fix suggestions with each issue, and iteration tracking. Critics run automatically at stage gates and on-demand via new /plugin-critique command.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JSON Schema | draft 2020-12 | Critic report schemas | Consistent with Phase 1-3 infrastructure |
| ajv-cli | 5.0+ | Schema validation | Already used in validate-handoff.sh |
| jq | 1.7.1+ | JSON parsing | Already available (verified in Phase 3) |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Claude token counting | API-provided | Token budget tracking | Cost awareness during iteration |
| Bash scripts | Any | Orchestration | Gate integration, iteration control |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Single critic agent | Separate generator + critic agents | User decided same agent fixes in-place; simpler |
| Hard token budget cut | Soft warning | User decided warn-not-block for flexibility |
| Manual critic selection | Auto-detect by stage | User decided stage determines applicable critics |

**No new installation needed:** Builds on existing infrastructure (jq, ajv-cli, bash scripts).

## Architecture Patterns

### Recommended Project Structure
```
.planning/
├── workflow/
│   ├── schemas/
│   │   ├── critic-report.schema.json       # NEW: Critic feedback format
│   │   ├── critic-dsp-report.schema.json   # NEW: DSP-specific extensions
│   │   ├── critic-ui-report.schema.json    # NEW: UI-specific extensions
│   │   └── validator-report.json           # EXISTS: Base validation
│   └── scripts/
│       ├── validate-handoff.sh             # EXISTS: Handoff validation
│       ├── stage-transition-gate.sh        # EXISTS: Gate checks
│       └── run-critic.sh                   # NEW: Critic orchestration
└── verification/                           # NEW: Failure reports only
    └── {plugin}/
        └── {stage}/
            └── critic-failure-{timestamp}.json

.claude/
├── agents/
│   ├── dsp-agent.md                        # EXISTS: Has DSP rules inline
│   ├── gui-agent.md                        # EXISTS: Has UI rules inline
│   └── validation-agent.md                 # EXISTS: Base validator
├── critics/                                # NEW: Domain critic templates
│   ├── critic-dsp.md                       # DSP real-time safety critic
│   └── critic-ui.md                        # UI polish standards critic
└── commands/
    └── plugin-critique.md                  # NEW: On-demand critic invocation
```

### Pattern 1: Generator-Critic Loop (Self-Refine Style)

**What:** Single agent generates output, receives critic feedback, fixes in-place, iterates until pass or max attempts
**When to use:** Every stage completion before handoff
**Example flow:**
```
┌─────────────────────────────────────────────────────────────┐
│                    Stage Completion                          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  Attempt 1/3                                                 │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │   Generate   │───▶│   Critique   │───▶│   Evaluate   │  │
│  │   (Agent)    │    │  (DSP/UI)    │    │   Scores     │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│                                                 │            │
│                                    PASS: all scores ≥ threshold
│                                    FAIL: any score < threshold
│                                                 │            │
│                              ┌──────────────────┴──────────┐ │
│                              │                              │ │
│                              ▼                              ▼ │
│                        ┌──────────┐               ┌──────────┐│
│                        │ Fix Loop │               │ Gate Pass││
│                        │ (same    │               └──────────┘│
│                        │  agent)  │                          │
│                        └────┬─────┘                          │
│                             │                                │
│                             ▼                                │
│                    Back to Critique                          │
└─────────────────────────────────────────────────────────────┘
                            │
                      (max 3 iterations)
                            │
                            ▼
                   ┌──────────────┐
                   │ Escalate to  │
                   │    User      │
                   └──────────────┘
```

### Pattern 2: Domain-Specific Critic Scoring

**What:** Numeric scores (1-10) per category with domain-specific thresholds
**When to use:** DSP critic for Stage 2, UI critic for Stage 3
**Example:**

```json
{
  "$schema": "../critic-dsp-report.schema.json",
  "critic": "dsp-critic",
  "plugin": "O-IntonationPad",
  "stage": "2-dsp",
  "attempt": 2,
  "maxAttempts": 3,
  "timestamp": "2026-01-30T14:30:00Z",

  "scores": {
    "realtime_safety": {
      "score": 9,
      "threshold": 8,
      "passed": true,
      "details": "No allocations in processBlock, ScopedNoDenormals used"
    },
    "buffer_handling": {
      "score": 7,
      "threshold": 7,
      "passed": true,
      "details": "Zero-length buffer check present, channel iteration safe"
    },
    "parameter_integration": {
      "score": 4,
      "threshold": 6,
      "passed": false,
      "details": "2 parameters not connected: 'resonance', 'filterType'"
    }
  },

  "overallStatus": "NEEDS_FIXES",
  "overallScore": 6.7,

  "issues": [
    {
      "id": "DSP-001",
      "severity": "error",
      "category": "parameter_integration",
      "location": "PluginProcessor.cpp:87",
      "description": "'resonance' parameter declared but not read in processBlock",
      "fixSuggestion": "Add resonance readout in processBlock and apply to filter Q parameter"
    },
    {
      "id": "DSP-002",
      "severity": "error",
      "category": "parameter_integration",
      "location": "PluginProcessor.cpp",
      "description": "'filterType' parameter not mapped to filter type switch",
      "fixSuggestion": "Read filterType in processBlock, use switch statement to set filter type"
    }
  ],

  "nextAction": "fix_and_resubmit"
}
```

### Pattern 3: Tiered Validation (Smoke vs Deep)

**What:** Quick smoke tests per task, deep validation at stage gates
**When to use:** Smoke tests during implementation, deep validation at stage boundaries
**Example:**

| Validation Level | When | Duration | Checks |
|------------------|------|----------|--------|
| Smoke Test | After each task | ~5 seconds | File exists, basic structure, no syntax errors |
| Deep Validation | Stage gate | ~2-5 minutes | Full critic scoring, pluginval, artifact verification |

```bash
# Smoke test (per-task)
run_smoke_test() {
    # Quick structure checks
    jq empty "$file" 2>/dev/null || return 1
    grep -q "processBlock" PluginProcessor.cpp || return 1
    return 0
}

# Deep validation (stage gate)
run_deep_validation() {
    # Full critic + pluginval
    ./run-critic.sh "$PLUGIN" "$STAGE" --deep
    pluginval --validate "$VST3_PATH" --strictness-level 10
}
```

### Pattern 4: Early Stop Conditions

**What:** Stop iteration when no progress detected or validation passes
**When to use:** Every iteration decision point
**Example:**

```
Stop conditions:
1. All scores ≥ thresholds → PASS (exit loop)
2. Same issues as previous iteration → NO_PROGRESS (escalate)
3. Iteration count ≥ 3 → MAX_ITERATIONS (escalate)
4. Token budget warning triggered → BUDGET_WARNING (warn but continue)
```

**No-progress detection:**
```python
# Pseudo-code for no-progress detection
def check_progress(current_issues, previous_issues):
    current_ids = {i['id'] for i in current_issues}
    previous_ids = {i['id'] for i in previous_issues}

    # No new issues resolved
    if current_ids == previous_ids:
        return "NO_PROGRESS"

    # Some issues resolved
    resolved = previous_ids - current_ids
    if len(resolved) > 0:
        return "PROGRESS"

    # New issues appeared (regression)
    new_issues = current_ids - previous_ids
    if len(new_issues) > 0:
        return "REGRESSION"
```

### Pattern 5: Token Budget Awareness

**What:** Track token usage across iterations, warn when budget exceeded
**When to use:** Throughout iterative refinement
**Example:**

```bash
# Token budget tracking (conceptual)
BUDGET_SOFT_LIMIT=50000  # Warn threshold
BUDGET_HARD_LIMIT=100000 # Would block if not user-discretion

track_tokens() {
    local iteration=$1
    local tokens_used=$2
    local cumulative_tokens=$3

    if [ "$cumulative_tokens" -gt "$BUDGET_SOFT_LIMIT" ]; then
        echo "WARNING: Token budget soft limit exceeded ($cumulative_tokens / $BUDGET_SOFT_LIMIT)" >&2
        echo "Consider simplifying or escalating to user" >&2
    fi

    # Per user decision: warn but don't interrupt
    # return 0 always (no hard block)
}
```

### Anti-Patterns to Avoid

- **Separate fixer agent:** User decided same agent fixes in-place; don't create separate agents
- **Silent token budget exhaustion:** Always warn when budget exceeded (even if not blocking)
- **Persisting passing reports:** Only failure reports go to `.planning/verification/`
- **Blocking on token budget:** User decided warn-not-block behavior
- **Critic without fix suggestions:** Every issue must include actionable fix approach

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Schema validation | Custom parser | ajv-cli (already used) | Consistent with Phase 3 infrastructure |
| Report structure | Ad-hoc JSON | Extend validator-report.json | DRY, established pattern |
| DSP safety rules | Invent from scratch | Extract from dsp-agent.md | Already documented, proven |
| UI standards | Invent from scratch | Extract from gui-agent.md | Already documented, proven |
| Progress detection | Manual comparison | Hash/set comparison of issue IDs | Reliable, deterministic |

**Key insight:** The existing validation-agent and agent contracts already encode domain expertise. Critics should reference and extend this knowledge, not duplicate it.

## Common Pitfalls

### Pitfall 1: Over-Strict Score Thresholds

**What goes wrong:** Critics block progression on minor issues that don't affect core functionality
**Why it happens:** Setting thresholds too high without calibration
**How to avoid:** Start with conservative thresholds, adjust based on real usage
**Warning signs:** High rate of user --force bypasses

**Recommended starting thresholds (Claude's discretion per CONTEXT.md):**
- DSP real-time safety: 8/10 (critical for audio software)
- DSP buffer handling: 7/10 (important but some slack)
- DSP parameter integration: 6/10 (can be incomplete initially)
- UI polish: 5/10 (lower bar, iterative improvement OK)
- UI consistency: 6/10 (moderate importance)

### Pitfall 2: Infinite Loop on Unfixable Issues

**What goes wrong:** Critic keeps flagging issue that agent cannot fix in current context
**Why it happens:** Missing context, external dependency, or genuinely out of scope
**How to avoid:** Track issue IDs across iterations; escalate if same issues persist
**Warning signs:** 3 iterations with identical issue list

### Pitfall 3: Critic Feedback Too Vague

**What goes wrong:** Agent receives feedback like "improve performance" with no actionable steps
**Why it happens:** Critic template doesn't require specificity
**How to avoid:** Schema enforces `fixSuggestion` field; suggestions must describe approach
**Warning signs:** Agent produces different but equally problematic fixes

### Pitfall 4: Token Budget Blindness

**What goes wrong:** Expensive iteration loops run without cost awareness
**Why it happens:** No tracking mechanism, tokens "invisible"
**How to avoid:** Log cumulative tokens per critique cycle; warn at soft limit
**Warning signs:** Unexpectedly high API costs on complex plugins

### Pitfall 5: Critic-Generator Mismatch

**What goes wrong:** DSP critic runs on UI stage, or vice versa
**Why it happens:** Stage-to-critic mapping unclear
**How to avoid:** Explicit stage → critic mapping (Stage 2 → DSP, Stage 3 → UI)
**Warning signs:** Irrelevant feedback, "check not applicable" results

## Code Examples

### Critic Report Schema (DSP)

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "critic-dsp-report.schema.json",
  "title": "DSP Critic Report",
  "description": "Feedback from DSP domain critic",
  "type": "object",
  "required": ["critic", "plugin", "stage", "attempt", "maxAttempts", "scores", "overallStatus", "issues"],
  "additionalProperties": false,
  "properties": {
    "critic": { "const": "dsp-critic" },
    "plugin": { "type": "string" },
    "stage": { "type": "string", "pattern": "^2-dsp$" },
    "attempt": { "type": "integer", "minimum": 1, "maximum": 3 },
    "maxAttempts": { "const": 3 },
    "timestamp": { "type": "string", "format": "date-time" },

    "scores": {
      "type": "object",
      "required": ["realtime_safety", "buffer_handling", "parameter_integration"],
      "additionalProperties": false,
      "properties": {
        "realtime_safety": { "$ref": "#/$defs/ScoreEntry" },
        "buffer_handling": { "$ref": "#/$defs/ScoreEntry" },
        "parameter_integration": { "$ref": "#/$defs/ScoreEntry" },
        "numerical_stability": { "$ref": "#/$defs/ScoreEntry" },
        "architecture_alignment": { "$ref": "#/$defs/ScoreEntry" }
      }
    },

    "overallStatus": {
      "type": "string",
      "enum": ["PASSED", "NEEDS_FIXES", "ESCALATE"]
    },
    "overallScore": { "type": "number", "minimum": 0, "maximum": 10 },

    "issues": {
      "type": "array",
      "items": { "$ref": "#/$defs/Issue" }
    },

    "previousIssueIds": {
      "type": "array",
      "items": { "type": "string" },
      "description": "Issue IDs from previous iteration (for progress detection)"
    },

    "nextAction": {
      "type": "string",
      "enum": ["gate_pass", "fix_and_resubmit", "escalate_to_user"]
    },

    "tokenMetrics": {
      "type": "object",
      "properties": {
        "thisIteration": { "type": "integer" },
        "cumulative": { "type": "integer" },
        "budgetWarning": { "type": "boolean" }
      }
    }
  },

  "$defs": {
    "ScoreEntry": {
      "type": "object",
      "required": ["score", "threshold", "passed"],
      "additionalProperties": false,
      "properties": {
        "score": { "type": "integer", "minimum": 1, "maximum": 10 },
        "threshold": { "type": "integer", "minimum": 1, "maximum": 10 },
        "passed": { "type": "boolean" },
        "details": { "type": "string" }
      }
    },
    "Issue": {
      "type": "object",
      "required": ["id", "severity", "category", "description", "fixSuggestion"],
      "additionalProperties": false,
      "properties": {
        "id": { "type": "string", "pattern": "^[A-Z]+-[0-9]+$" },
        "severity": { "type": "string", "enum": ["error", "warning"] },
        "category": { "type": "string" },
        "location": { "type": "string" },
        "description": { "type": "string" },
        "fixSuggestion": { "type": "string", "minLength": 10 }
      }
    }
  }
}
```

### Critic Orchestration Script

```bash
#!/bin/bash
# run-critic.sh - Orchestrate critic validation with iteration support
#
# Usage: run-critic.sh <plugin> <stage> [--attempt N] [--deep]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Defaults
ATTEMPT=1
MAX_ATTEMPTS=3
DEEP_MODE=false

# Parse arguments
PLUGIN="$1"
STAGE="$2"
shift 2

while [[ $# -gt 0 ]]; do
    case $1 in
        --attempt) ATTEMPT="$2"; shift 2 ;;
        --deep) DEEP_MODE=true; shift ;;
        *) shift ;;
    esac
done

# Determine applicable critic based on stage
case "$STAGE" in
    2-dsp) CRITIC="dsp-critic" ;;
    3-gui) CRITIC="ui-critic" ;;
    *) echo "No critic applicable for stage $STAGE"; exit 0 ;;
esac

echo "Critic Run: $CRITIC"
echo "Plugin: $PLUGIN"
echo "Stage: $STAGE"
echo "Attempt: $ATTEMPT / $MAX_ATTEMPTS"
echo "========================================"

# Run critic (Claude agent invocation would happen here)
# The actual critique is performed by Claude reading the critic template
# and evaluating the code against the criteria

# Output would be written to:
# .planning/verification/${PLUGIN}/${STAGE}/critic-report-attempt${ATTEMPT}.json

# Check for --force bypass
if [ "$FORCE_MODE" = true ]; then
    echo "WARNING: --force bypasses critic. Proceeding without validation." >&2
    exit 0
fi

echo "Critic complete. See report for results."
```

### DSP Critic Template Outline

```markdown
# DSP Critic Template (.claude/critics/critic-dsp.md)

## Purpose
Validate DSP implementation against real-time safety rules and professional audio standards.

## Scoring Categories

### 1. Real-Time Safety (Threshold: 8/10)
**Check for violations:**
- [ ] Memory allocation in processBlock (`new`, `malloc`, `vector::push_back`)
- [ ] File I/O in audio thread
- [ ] Locks/mutexes in processBlock
- [ ] System calls
- [ ] Exceptions
- [ ] Unbounded loops

**Evidence required:** Line numbers where violations found

### 2. Buffer Handling (Threshold: 7/10)
**Check for:**
- [ ] Zero-length buffer handling
- [ ] Channel count validation
- [ ] Sample rate handling in prepareToPlay
- [ ] Buffer preallocation

### 3. Parameter Integration (Threshold: 6/10)
**Check for:**
- [ ] All parameters from parameter-spec.md accessed
- [ ] Atomic reads used (getRawParameterValue()->load())
- [ ] Parameter values affect DSP correctly

## Fix Suggestion Format
Every issue MUST include:
- What is wrong (specific, with location)
- Why it matters (impact on audio/stability)
- How to fix it (approach, not code snippet)

## Escalation Criteria
Escalate to user if:
- Same issues persist after 3 attempts
- Issue requires architectural change beyond stage scope
- External dependency needed
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Single-pass validation | Iterative refinement (Self-Refine) | 2023-2024 | Higher quality outputs |
| Binary pass/fail | Numeric scores with thresholds | 2024-2025 | Nuanced feedback, gradual improvement |
| Generic critics | Domain-specific critics | Industry trend 2025 | Better detection of domain issues |
| Unlimited iteration | Budget-aware iteration | 2025-2026 | Cost management without hard limits |
| Separate generator/critic agents | Same-agent fix-in-place | Pattern choice | Simpler architecture, less context switching |

**Current project status:**
- GSD verify-phase pattern exists (goal-backward analysis)
- validation-agent exists with pluginval integration
- DSP/UI expertise encoded in dsp-agent.md and gui-agent.md
- No iterative refinement infrastructure yet
- No critic scoring system yet

## Open Questions

Things that couldn't be fully resolved:

1. **Exact score thresholds per category**
   - What we know: CONTEXT.md leaves to Claude's discretion
   - What's unclear: Optimal values without calibration data
   - Recommendation: Start with proposed values (8/7/6 for DSP, 5/6 for UI), adjust based on false positive rate

2. **No-progress detection granularity**
   - What we know: Compare issue IDs across iterations
   - What's unclear: Should partial resolution count as progress?
   - Recommendation: Count as progress if any issue resolved; only escalate if zero change

3. **Token budget values**
   - What we know: Should warn but not block
   - What's unclear: Appropriate soft limit value
   - Recommendation: Start with 50K tokens per critique cycle; log actual usage for calibration

4. **Report file naming**
   - What we know: Only failures persist; CONTEXT.md leaves to discretion
   - What's unclear: Exact naming convention
   - Recommendation: `{stage}-critic-failure-{timestamp}.json` in `.planning/verification/{plugin}/`

## Sources

### Primary (HIGH confidence)
- [AWS Evaluator Reflect-Refine Loop Patterns](https://docs.aws.amazon.com/prescriptive-guidance/latest/agentic-ai-patterns/evaluator-reflect-refine-loop-patterns.html) - Official AWS guidance on generator-critic patterns
- [Self-Refine: Iterative Refinement with Self-Feedback](https://selfrefine.info/) - Academic paper on self-refinement without RL
- GSD verify-phase.md - Existing project verification infrastructure
- validation-agent.md - Existing project validator with scoring concepts
- dsp-agent.md - DSP domain expertise already encoded
- gui-agent.md - UI domain expertise already encoded

### Secondary (MEDIUM confidence)
- [MongoDB Multi-Agent Design Patterns](https://medium.com/mongodb/here-are-7-design-patterns-for-agentic-systems-you-need-to-know-d74a4b5835a5) - Generator-critic pattern documentation
- [Langfuse Token and Cost Tracking](https://langfuse.com/docs/observability/features/token-and-cost-tracking) - Token budget management approaches
- [LLM Pricing Comparison 2026](https://www.cloudidr.com/blog/llm-pricing-comparison-2026) - Cost context for budget decisions

### Tertiary (LOW confidence)
- Web search results on iterative refinement patterns - General industry trends
- Multi-agent orchestration blog posts - Architecture inspiration

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Builds on existing Phase 1-3 infrastructure
- Architecture patterns: HIGH - Based on GSD patterns + industry standards
- Domain critic design: MEDIUM - Extracts from existing agents, needs calibration
- Token budget: LOW - Values are estimates, need real usage data
- Score thresholds: MEDIUM - Reasonable starting points, may need adjustment

**Research date:** 2026-01-30
**Valid until:** 2026-03-01 (60 days - patterns stable, thresholds may need earlier calibration)
