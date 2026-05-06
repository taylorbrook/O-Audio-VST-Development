# Phase 5: Quality Gates - Research

**Researched:** 2026-01-30
**Domain:** Stage transition gates, blocking verification, retry logic, code review integration
**Confidence:** HIGH

## Summary

Phase 5 implements blocking quality gates at stage boundaries (0->1, 1->2, 2->3, 3->4) that enforce measurable success criteria before progression. The research confirms that the Phase 4 infrastructure provides all necessary components: critic templates (DSP/UI), orchestration scripts (run-critic.sh, stage-transition-gate.sh), validation scripts (validate-handoff.sh), and report schemas. Phase 5's role is to compose these into unified gate checks with the specific behaviors decided in CONTEXT.md.

The key pattern is a **gate composition** approach: each stage boundary runs a sequence of checks (schema validation, build, domain critic, pluginval, test suite) in parallel where possible, with automatic retry on transient failures and blocking on critical failures. The critical + advisory model separates blocking checks from informational reports.

Phase 5 builds on proven CI/CD quality gate patterns: fail-fast on critical issues, automatic retry for flaky checks, explicit bypass with audit trail, and parallelization within a 2-minute budget. Code review integration happens at stage boundaries as a blocking human checkpoint with domain-specific focus.

**Primary recommendation:** Create a unified `run-gate.sh` script that composes existing checks, implements the critical/advisory split, handles retry logic, and enforces the `--force` bypass pattern with justification logging.

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Bash | 5.x | Gate orchestration | Universal availability, process control, parallel execution |
| GNU Parallel | 2022+ | Check parallelization | Safe parallel execution with timeout and job control |
| jq | 1.7.1+ | JSON report parsing | Already used in Phase 3-4, fast CLI JSON processing |
| ajv-cli | 5.0+ | Schema validation | Already used in Phase 3-4, draft 2020-12 support |
| pluginval | 0.2.9+ | Plugin validation | Industry standard VST3/AU validation, CI/CD friendly |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| timeout (GNU coreutils) | Any | Per-check timeout | Prevent hung checks from blocking gate |
| tee | Any | Output capture | Log check output while displaying |
| wait | Bash built-in | Background job synchronization | Parallel check coordination |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Bash parallel | xargs -P | xargs is simpler but less control over exit codes |
| GNU Parallel | Background jobs (&) | Parallel handles timeout/retry natively |
| Sequential checks | Always parallel | User decided uniform depth; parallel only if exceeds 2min |

**No new installation needed:** Builds on existing Phase 3-4 infrastructure. GNU Parallel is optional (fallback to sequential if not available).

## Architecture Patterns

### Recommended Project Structure

```
.planning/workflow/
├── scripts/
│   ├── validate-handoff.sh        # EXISTS (Phase 3)
│   ├── stage-transition-gate.sh   # EXISTS (Phase 3) - extend
│   ├── run-critic.sh              # EXISTS (Phase 4)
│   ├── run-gate.sh                # NEW: Unified gate orchestration
│   └── run-code-review.sh         # NEW: Human review checkpoint
├── schemas/
│   ├── gate-report.schema.json    # NEW: Gate execution report
│   ├── critic-*.schema.json       # EXISTS (Phase 4)
│   └── handoff-*.schema.json      # EXISTS (Phase 3)
└── templates/
    └── code-review-checklist.md   # NEW: Domain-specific review prompts

plugins/[PluginName]/.planning/
├── stages/[N]-[name]/
│   ├── HANDOFF.json               # EXISTS
│   ├── gate-report.json           # NEW: Gate execution results
│   └── review-notes.md            # NEW: Human review notes
└── gate-bypasses.log              # NEW: Audit trail for --force
```

### Pattern 1: Gate Composition with Critical/Advisory Split

**What:** Unified gate that runs all checks, separates blocking (critical) from reporting (advisory)
**When to use:** Every stage transition
**Example:**

```bash
#!/bin/bash
# run-gate.sh - Unified quality gate for stage transitions

run_gate() {
    local PLUGIN=$1
    local FROM_STAGE=$2
    local TO_STAGE=$3

    # Track results
    local CRITICAL_PASSED=true
    local ADVISORY_ISSUES=()

    # Critical checks (all must pass)
    run_check "schema" validate_schema && mark_passed "schema" || CRITICAL_PASSED=false
    run_check "build" run_build && mark_passed "build" || CRITICAL_PASSED=false
    run_check "pluginval" run_pluginval && mark_passed "pluginval" || CRITICAL_PASSED=false

    # Stage-dependent critical checks
    if [[ "$TO_STAGE" == "3-gui" || "$TO_STAGE" == "4-polish" ]]; then
        run_check "dsp-critic" run_dsp_critic && mark_passed "dsp-critic" || CRITICAL_PASSED=false
    fi
    if [[ "$TO_STAGE" == "4-polish" ]]; then
        run_check "ui-critic" run_ui_critic && mark_passed "ui-critic" || CRITICAL_PASSED=false
    fi

    # Advisory checks (report only)
    run_advisory "style" check_code_style || ADVISORY_ISSUES+=("style")
    run_advisory "naming" check_naming_conventions || ADVISORY_ISSUES+=("naming")
    run_advisory "docs" check_documentation || ADVISORY_ISSUES+=("docs")

    # Gate decision
    if [ "$CRITICAL_PASSED" = true ]; then
        echo "GATE PASSED"
        [ ${#ADVISORY_ISSUES[@]} -gt 0 ] && echo "Advisory: ${ADVISORY_ISSUES[*]}"
        return 0
    else
        echo "GATE BLOCKED"
        return 1
    fi
}
```

### Pattern 2: Automatic Retry with Exponential Backoff

**What:** Retry transient failures once automatically, then on user retry attempts
**When to use:** Checks that can fail due to transient issues (pluginval, build)
**Example:**

```bash
# Retry logic per CONTEXT.md decisions:
# - 1 automatic retry on failure (handles flaky)
# - Up to 3 total attempts before requiring --force
# - Exponential backoff between retries

run_with_retry() {
    local CHECK_NAME=$1
    local CHECK_CMD=$2
    local ATTEMPT=1
    local MAX_ATTEMPTS=3
    local RETRY_DELAY=2  # seconds, doubles each retry

    while [ $ATTEMPT -le $MAX_ATTEMPTS ]; do
        echo "[$CHECK_NAME] Attempt $ATTEMPT/$MAX_ATTEMPTS"

        if eval "$CHECK_CMD"; then
            return 0
        fi

        # Automatic retry on first failure
        if [ $ATTEMPT -eq 1 ]; then
            echo "[$CHECK_NAME] Transient failure, retrying in ${RETRY_DELAY}s..."
            sleep $RETRY_DELAY
            RETRY_DELAY=$((RETRY_DELAY * 2))
            ATTEMPT=$((ATTEMPT + 1))
            continue
        fi

        # Subsequent retries require user action
        echo "[$CHECK_NAME] Failed (attempt $ATTEMPT)"
        if [ $ATTEMPT -lt $MAX_ATTEMPTS ]; then
            read -p "Retry? [y/N/--force]: " RESPONSE
            case $RESPONSE in
                y|Y)
                    sleep $RETRY_DELAY
                    RETRY_DELAY=$((RETRY_DELAY * 2))
                    ATTEMPT=$((ATTEMPT + 1))
                    ;;
                --force)
                    return 2  # Force bypass code
                    ;;
                *)
                    return 1
                    ;;
            esac
        else
            echo "[$CHECK_NAME] Max attempts reached"
            return 1
        fi
    done
}
```

### Pattern 3: Parallelization with Timeout

**What:** Run independent checks in parallel, enforce per-check timeout
**When to use:** When total gate time exceeds 2-minute target
**Example:**

```bash
# Parallel execution with GNU Parallel (if available)
run_checks_parallel() {
    local PLUGIN=$1
    local STAGE=$2
    local TIMEOUT=60  # seconds per check

    # Define independent checks
    local CHECKS=(
        "schema:validate_schema $PLUGIN"
        "build:run_build $PLUGIN"
        "pluginval:run_pluginval $PLUGIN"
    )

    if command -v parallel &> /dev/null; then
        # GNU Parallel with timeout
        printf '%s\n' "${CHECKS[@]}" | \
            parallel --timeout $TIMEOUT --jobs 4 --halt soon,fail=1 \
            'name=$(echo {} | cut -d: -f1); cmd=$(echo {} | cut -d: -f2-); echo "Running $name..."; eval "$cmd"'
    else
        # Fallback: background jobs with manual timeout
        local PIDS=()
        for check in "${CHECKS[@]}"; do
            name=$(echo "$check" | cut -d: -f1)
            cmd=$(echo "$check" | cut -d: -f2-)
            ( timeout $TIMEOUT bash -c "$cmd" ) &
            PIDS+=($!)
        done

        # Wait for all and check exit codes
        local ALL_PASSED=true
        for pid in "${PIDS[@]}"; do
            wait $pid || ALL_PASSED=false
        done

        [ "$ALL_PASSED" = true ]
    fi
}
```

### Pattern 4: Force Bypass with Justification Logging

**What:** Allow `--force` bypass with mandatory justification, logged for audit
**When to use:** When gate fails but user needs to proceed
**Example:**

```bash
handle_force_bypass() {
    local PLUGIN=$1
    local STAGE=$2
    local FAILED_CHECKS=$3

    echo ""
    echo "=========================================="
    echo "GATE BLOCKED - Force bypass requested"
    echo "=========================================="
    echo ""
    echo "Failed checks: $FAILED_CHECKS"
    echo ""
    echo "Bypassing quality gate is discouraged."
    echo "Please provide justification for the audit trail:"
    echo ""
    read -p "Justification: " JUSTIFICATION

    if [ -z "$JUSTIFICATION" ]; then
        echo "ERROR: Justification required for --force bypass"
        return 1
    fi

    # Log to audit file
    local BYPASS_LOG="plugins/${PLUGIN}/.planning/gate-bypasses.log"
    local TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

    cat >> "$BYPASS_LOG" <<EOF

## Bypass: $STAGE gate
**Date:** $TIMESTAMP
**Failed checks:** $FAILED_CHECKS
**Justification:** $JUSTIFICATION
**User:** $(whoami)
---
EOF

    echo ""
    echo "Bypass logged to: $BYPASS_LOG"
    echo "Proceeding with stage transition..."
    return 0
}
```

### Pattern 5: Domain-Specific Code Review Checklists

**What:** Human review checkpoint with stage-appropriate focus areas
**When to use:** End of each stage (4 review points total)
**Example:**

```markdown
# Code Review Checklist - Stage 2 (DSP)

## Review Focus: DSP Patterns, Realtime Safety

### Critical (must verify)
- [ ] No memory allocations in processBlock
- [ ] All parameters read atomically
- [ ] ScopedNoDenormals present
- [ ] Buffer handling is safe

### Domain-Specific
- [ ] Signal flow matches ARCHITECTURE.md
- [ ] Parameter smoothing where needed
- [ ] CPU usage within budget

### Simplification Pass
- [ ] Remove dead code paths
- [ ] Consolidate duplicate logic
- [ ] Extract magic numbers to constants
- [ ] Simplify complex conditionals

## Reviewer Notes
[Free-form notes from human reviewer]

## Verdict
- [ ] Approved - proceed to Stage 3
- [ ] Changes requested - iterate
- [ ] Blocked - major issues found
```

### Anti-Patterns to Avoid

- **Tiered verification depth:** User decided uniform depth at every gate. Never implement `--quick` or smoke-only options.
- **Blocking on advisory checks:** Advisory checks report only. Never block stage progression on style, naming, or documentation issues.
- **Silent bypass:** Every `--force` MUST prompt for justification and log it. Never silently skip checks.
- **No automatic retry:** Always retry once automatically on first failure to handle transient issues.
- **Sequential when parallel possible:** If gate exceeds 2 minutes, parallelize. Time is developer productivity.
- **Critics at wrong stages:** DSP critic is critical only at Stage 2+, UI critic only at Stage 3+. Earlier stages are advisory.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Plugin validation | Custom tests | pluginval (strictness 10) | Comprehensive test coverage, industry standard |
| Schema validation | Custom parser | ajv-cli | JSON Schema edge cases are complex |
| Parallel execution | Manual & | GNU Parallel | Proper timeout, job control, exit codes |
| Critic scoring | New framework | Phase 4 critics | DSP/UI critics already encode domain expertise |
| Handoff validation | New checks | validate-handoff.sh | Already validates schema + artifact existence |
| Build verification | Parse output | ninja exit code | Build systems return meaningful exit codes |

**Key insight:** Phase 4 already built the verification components. Phase 5 composes them into gates, adds the blocking/bypass behavior, and integrates code review. Don't re-implement what exists.

## Common Pitfalls

### Pitfall 1: Gate Timeout Without Graceful Degradation

**What goes wrong:** Check hangs indefinitely, gate never completes
**Why it happens:** External tool (pluginval, build) deadlocks or hits infinite loop
**How to avoid:** Enforce per-check timeout (60s default), treat timeout as failure
**Warning signs:** Gate times exceeding 5 minutes, CI/CD job timeouts

### Pitfall 2: Flaky Check Cascade Failures

**What goes wrong:** Single transient failure blocks entire gate
**Why it happens:** No retry logic, treating transient as permanent failure
**How to avoid:** Automatic retry once on first failure (per CONTEXT.md)
**Warning signs:** "Works on retry" pattern, inconsistent gate results

### Pitfall 3: Force Bypass Without Justification

**What goes wrong:** Quality debt accumulates without tracking
**Why it happens:** `--force` silently skips without logging
**How to avoid:** Require justification, log to gate-bypasses.log
**Warning signs:** Multiple bypasses for same plugin, untracked technical debt

### Pitfall 4: Critic Stage Mismatch

**What goes wrong:** DSP critic runs on Stage 0->1 transition (before DSP code exists)
**Why it happens:** Applying all checks at all gates
**How to avoid:** Stage-dependent critic applicability (per CONTEXT.md)
**Warning signs:** "Not applicable" critic results, irrelevant feedback

### Pitfall 5: Code Review Blocking Without Bypass

**What goes wrong:** Human unavailable, progress blocked for hours/days
**Why it happens:** No `--skip-review` escape hatch
**How to avoid:** Allow `--skip-review` with justification logging (like `--force`)
**Warning signs:** Extended gate wait times, developer frustration

### Pitfall 6: Parallel Check Race Conditions

**What goes wrong:** Checks interfere with each other (same build output, same file)
**Why it happens:** Running incompatible checks simultaneously
**How to avoid:** Only parallelize truly independent checks, use isolation
**Warning signs:** Intermittent failures only in parallel mode

## Code Examples

### Gate Report Schema

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "gate-report.schema.json",
  "title": "Quality Gate Report",
  "description": "Results from quality gate execution at stage transition",
  "type": "object",
  "required": ["gate", "plugin", "fromStage", "toStage", "timestamp", "checks", "overallStatus"],
  "additionalProperties": false,
  "properties": {
    "gate": { "type": "string", "pattern": "^gate-[0-9]+-to-[0-9]+$" },
    "plugin": { "type": "string" },
    "fromStage": { "type": "string" },
    "toStage": { "type": "string" },
    "timestamp": { "type": "string", "format": "date-time" },

    "checks": {
      "type": "object",
      "required": ["critical", "advisory"],
      "additionalProperties": false,
      "properties": {
        "critical": {
          "type": "array",
          "items": { "$ref": "#/$defs/CheckResult" }
        },
        "advisory": {
          "type": "array",
          "items": { "$ref": "#/$defs/CheckResult" }
        }
      }
    },

    "overallStatus": {
      "type": "string",
      "enum": ["PASSED", "BLOCKED", "BYPASSED"]
    },

    "totalDuration": { "type": "integer", "description": "milliseconds" },

    "bypass": {
      "type": "object",
      "properties": {
        "used": { "type": "boolean" },
        "justification": { "type": "string" },
        "failedChecks": { "type": "array", "items": { "type": "string" } }
      }
    },

    "review": {
      "type": "object",
      "properties": {
        "status": { "type": "string", "enum": ["approved", "requested_changes", "skipped"] },
        "notes": { "type": "string" },
        "skipped": { "type": "boolean" },
        "skipJustification": { "type": "string" }
      }
    }
  },

  "$defs": {
    "CheckResult": {
      "type": "object",
      "required": ["name", "status", "duration"],
      "additionalProperties": false,
      "properties": {
        "name": { "type": "string" },
        "status": { "type": "string", "enum": ["passed", "failed", "skipped", "timeout"] },
        "duration": { "type": "integer", "description": "milliseconds" },
        "attempts": { "type": "integer", "minimum": 1, "maximum": 3 },
        "output": { "type": "string" },
        "error": { "type": "string" }
      }
    }
  }
}
```

### Unified Gate Script (run-gate.sh)

```bash
#!/bin/bash
# run-gate.sh - Unified quality gate for stage transitions
#
# Usage: run-gate.sh <plugin> <from-stage> <to-stage> [--force] [--skip-review]
#
# Runs all verification checks for a stage transition:
# - Critical: schema validation, build, pluginval, domain critics (stage-dependent)
# - Advisory: code style, naming conventions, documentation
# - Review: human code review checkpoint (end of gate)
#
# Exit codes:
#   0 - GATE PASSED
#   1 - GATE BLOCKED (critical check failed)
#   2 - GATE BYPASSED (--force used)
#   3 - Usage error

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Source helper scripts
source "$SCRIPT_DIR/validate-handoff.sh" 2>/dev/null || true
source "$SCRIPT_DIR/run-critic.sh" 2>/dev/null || true

# Configuration
TARGET_DURATION=120000  # 2 minutes in milliseconds
CHECK_TIMEOUT=60        # seconds per check
RETRY_DELAY=2           # initial delay, doubles each retry
MAX_RETRIES=3

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# Parse arguments
PLUGIN=""
FROM_STAGE=""
TO_STAGE=""
FORCE_MODE=false
SKIP_REVIEW=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --force) FORCE_MODE=true; shift ;;
        --skip-review) SKIP_REVIEW=true; shift ;;
        --help|-h) show_help; exit 0 ;;
        *)
            if [ -z "$PLUGIN" ]; then PLUGIN="$1"
            elif [ -z "$FROM_STAGE" ]; then FROM_STAGE="$1"
            elif [ -z "$TO_STAGE" ]; then TO_STAGE="$1"
            fi
            shift
            ;;
    esac
done

# Validate arguments
if [ -z "$PLUGIN" ] || [ -z "$FROM_STAGE" ] || [ -z "$TO_STAGE" ]; then
    echo -e "${RED}ERROR: Required arguments: plugin, from-stage, to-stage${NC}"
    exit 3
fi

# Initialize report
REPORT_FILE="$PROJECT_ROOT/plugins/${PLUGIN}/.planning/stages/${FROM_STAGE}/gate-report.json"
START_TIME=$(date +%s%3N)

echo ""
echo -e "${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "${BOLD}                    QUALITY GATE${NC}"
echo -e "${BOLD}════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Plugin:      ${CYAN}${PLUGIN}${NC}"
echo -e "Transition:  ${FROM_STAGE} -> ${TO_STAGE}"
echo -e "Target:      <${TARGET_DURATION}ms"
echo ""

# Track results
declare -A CRITICAL_RESULTS
declare -A ADVISORY_RESULTS
CRITICAL_PASSED=true
FAILED_CHECKS=()

# Run a check with retry logic
run_check() {
    local NAME=$1
    local CMD=$2
    local IS_CRITICAL=${3:-true}
    local ATTEMPT=1
    local DELAY=$RETRY_DELAY

    while [ $ATTEMPT -le $MAX_RETRIES ]; do
        echo -ne "  [$NAME] Attempt $ATTEMPT/$MAX_RETRIES... "

        CHECK_START=$(date +%s%3N)
        if timeout $CHECK_TIMEOUT bash -c "$CMD" > /tmp/gate_${NAME}.log 2>&1; then
            CHECK_END=$(date +%s%3N)
            DURATION=$((CHECK_END - CHECK_START))
            echo -e "${GREEN}PASSED${NC} (${DURATION}ms)"
            return 0
        fi
        CHECK_END=$(date +%s%3N)
        DURATION=$((CHECK_END - CHECK_START))

        # Automatic retry on first failure
        if [ $ATTEMPT -eq 1 ]; then
            echo -e "${YELLOW}RETRY${NC} (transient failure)"
            sleep $DELAY
            DELAY=$((DELAY * 2))
            ATTEMPT=$((ATTEMPT + 1))
            continue
        fi

        echo -e "${RED}FAILED${NC} (${DURATION}ms)"
        ATTEMPT=$((ATTEMPT + 1))
    done

    return 1
}

# Critical checks
echo -e "${BOLD}Critical Checks${NC}"
echo "────────────────────────────────────────"

# 1. Schema validation
if ! run_check "schema" "ajv-cli validate -s ... -d ... 2>/dev/null || true"; then
    CRITICAL_PASSED=false
    FAILED_CHECKS+=("schema")
fi

# 2. Build
if ! run_check "build" "cd $PROJECT_ROOT/build && ninja ${PLUGIN}_VST3 ${PLUGIN}_AU"; then
    CRITICAL_PASSED=false
    FAILED_CHECKS+=("build")
fi

# 3. Pluginval
VST3_PATH="$PROJECT_ROOT/build/plugins/${PLUGIN}/${PLUGIN}_artefacts/Release/VST3/${PLUGIN}.vst3"
if ! run_check "pluginval" "pluginval --strictness-level 10 --validate '$VST3_PATH'"; then
    CRITICAL_PASSED=false
    FAILED_CHECKS+=("pluginval")
fi

# 4. Domain critics (stage-dependent)
case "$TO_STAGE" in
    3-gui|4-polish)
        if ! run_check "dsp-critic" "$SCRIPT_DIR/run-critic.sh $PLUGIN 2-dsp"; then
            CRITICAL_PASSED=false
            FAILED_CHECKS+=("dsp-critic")
        fi
        ;;
esac

case "$TO_STAGE" in
    4-polish)
        if ! run_check "ui-critic" "$SCRIPT_DIR/run-critic.sh $PLUGIN 3-gui"; then
            CRITICAL_PASSED=false
            FAILED_CHECKS+=("ui-critic")
        fi
        ;;
esac

echo ""

# Advisory checks
echo -e "${BOLD}Advisory Checks${NC}"
echo "────────────────────────────────────────"

run_check "style" "echo 'Style check placeholder'" false || echo "  [style] Issues found (advisory)"
run_check "naming" "echo 'Naming check placeholder'" false || echo "  [naming] Issues found (advisory)"
run_check "docs" "echo 'Docs check placeholder'" false || echo "  [docs] Issues found (advisory)"

echo ""

# Gate decision
END_TIME=$(date +%s%3N)
TOTAL_DURATION=$((END_TIME - START_TIME))

echo "────────────────────────────────────────"
echo -e "Total duration: ${TOTAL_DURATION}ms"
echo ""

if [ "$CRITICAL_PASSED" = true ]; then
    # Code review checkpoint
    if [ "$SKIP_REVIEW" = true ]; then
        echo -e "${YELLOW}Code review skipped (--skip-review)${NC}"
        read -p "Justification for skipping review: " SKIP_JUSTIFICATION
    else
        echo -e "${CYAN}Code Review Required${NC}"
        echo "Review focus for $TO_STAGE:"
        case "$TO_STAGE" in
            1-foundation) echo "  - Build patterns, APVTS setup, parameter definitions" ;;
            2-dsp) echo "  - DSP patterns, realtime safety, buffer handling" ;;
            3-gui) echo "  - GUI patterns, threading, APVTS binding" ;;
            4-polish) echo "  - Integration, polish, optimization" ;;
        esac
        echo ""
        echo "Checklist:"
        echo "  [ ] Code simplification pass complete"
        echo "  [ ] Domain-specific review complete"
        echo ""
        read -p "Review complete? [y/N]: " REVIEW_RESPONSE
        if [ "$REVIEW_RESPONSE" != "y" ] && [ "$REVIEW_RESPONSE" != "Y" ]; then
            echo -e "${YELLOW}Review pending - gate waiting${NC}"
            exit 1
        fi
    fi

    echo ""
    echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}                    GATE PASSED${NC}"
    echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"
    exit 0
else
    if [ "$FORCE_MODE" = true ]; then
        echo -e "${YELLOW}Gate blocked, but --force requested${NC}"
        echo "Failed checks: ${FAILED_CHECKS[*]}"
        read -p "Justification (required): " JUSTIFICATION

        if [ -z "$JUSTIFICATION" ]; then
            echo -e "${RED}ERROR: Justification required for --force${NC}"
            exit 1
        fi

        # Log bypass
        BYPASS_LOG="$PROJECT_ROOT/plugins/${PLUGIN}/.planning/gate-bypasses.log"
        echo "" >> "$BYPASS_LOG"
        echo "## Bypass: $FROM_STAGE -> $TO_STAGE" >> "$BYPASS_LOG"
        echo "**Date:** $(date -u +"%Y-%m-%dT%H:%M:%SZ")" >> "$BYPASS_LOG"
        echo "**Failed:** ${FAILED_CHECKS[*]}" >> "$BYPASS_LOG"
        echo "**Justification:** $JUSTIFICATION" >> "$BYPASS_LOG"
        echo "---" >> "$BYPASS_LOG"

        echo ""
        echo -e "${YELLOW}════════════════════════════════════════════════════════${NC}"
        echo -e "${YELLOW}                    GATE BYPASSED${NC}"
        echo -e "${YELLOW}════════════════════════════════════════════════════════${NC}"
        exit 2
    else
        echo -e "${RED}════════════════════════════════════════════════════════${NC}"
        echo -e "${RED}                    GATE BLOCKED${NC}"
        echo -e "${RED}════════════════════════════════════════════════════════${NC}"
        echo ""
        echo "Failed checks: ${FAILED_CHECKS[*]}"
        echo ""
        echo "Options:"
        echo "  1. Fix issues and retry"
        echo "  2. Use --force to bypass (requires justification)"
        exit 1
    fi
fi
```

### Code Review Prompt Template

```markdown
# Code Review - Stage [N]: [StageName]

## Plugin
[PluginName]

## Review Focus
[Stage-specific focus from checklist]

## Mandatory Simplification Pass

Before approving, verify:
- [ ] Dead code removed
- [ ] Duplicate logic consolidated
- [ ] Magic numbers extracted to constants
- [ ] Complex conditionals simplified
- [ ] Unused includes/imports removed

## Domain Checklist

### Stage 1 (Foundation)
- [ ] CMakeLists.txt follows project patterns
- [ ] APVTS parameter setup correct
- [ ] Plugin metadata accurate

### Stage 2 (DSP)
- [ ] No allocations in processBlock
- [ ] ScopedNoDenormals present
- [ ] All parameters affect DSP
- [ ] Buffer handling safe

### Stage 3 (GUI)
- [ ] Member declaration order correct
- [ ] APVTS attachment patterns used
- [ ] No audio thread access from GUI
- [ ] Responsive layout implemented

### Stage 4 (Polish)
- [ ] pluginval strictness 10 passes
- [ ] Presets functional
- [ ] Documentation complete
- [ ] Build artifacts verified

## Reviewer Notes

[Free-form notes]

## Verdict

- [ ] APPROVED - proceed to next stage
- [ ] CHANGES REQUESTED - address before proceeding
- [ ] BLOCKED - major issues require redesign
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual verification | Automated gates | 2023-2024 | Consistent quality, faster feedback |
| Binary pass/fail | Critical + advisory split | 2024-2025 | Reduces false blocking on low-priority issues |
| No retry | Automatic retry for flaky | 2024-2025 | Handles transient CI/CD failures |
| Silent bypass | Audited --force bypass | 2025-2026 | Technical debt tracking |
| Sequential checks | Parallelized checks | 2024-2025 | Faster gate completion |
| Generic code review | Domain-specific checklists | 2025-2026 | More effective human review |

**Current project status:**
- Phase 4 delivered: run-critic.sh, stage-transition-gate.sh, validate-handoff.sh, DSP/UI critics
- Missing: Unified gate composition, retry logic, bypass logging, code review integration
- Phase 5 composes existing infrastructure into blocking gates

## Open Questions

1. **Parallelization strategy details**
   - What we know: User decided parallel if exceeds 2 min target
   - What's unclear: Which checks are safe to parallelize together
   - Recommendation: Build is independent; critics depend on build success; run build first, then parallelize critics with pluginval

2. **Advisory display format**
   - What we know: Advisory checks report only, don't block
   - What's unclear: Inline vs summary display (per CONTEXT.md: Claude's discretion)
   - Recommendation: Inline during gate, summary in gate-report.json; avoid cluttering terminal

3. **Retry delay values**
   - What we know: Should use exponential backoff
   - What's unclear: Initial delay value (per CONTEXT.md: Claude's discretion)
   - Recommendation: Start at 2 seconds, double each retry (2s, 4s, 8s)

4. **Review prompt format**
   - What we know: Should be domain-specific, include simplification pass
   - What's unclear: Exact format (per CONTEXT.md: Claude's discretion)
   - Recommendation: Use markdown checklist template, stage-specific sections

## Sources

### Primary (HIGH confidence)
- Phase 4 Research: `.planning/phases/04-verification-infrastructure/04-RESEARCH.md` - Critic patterns, scoring thresholds
- Phase 4 Summary: `.planning/phases/04-verification-infrastructure/04-02-SUMMARY.md` - Delivered infrastructure
- Phase 3 Research: `.planning/phases/03-structured-handoffs/03-RESEARCH.md` - Validation patterns, gate checks
- Existing scripts: `run-critic.sh`, `stage-transition-gate.sh`, `validate-handoff.sh`
- [pluginval GitHub](https://github.com/Tracktion/pluginval) - Strictness levels, exit codes

### Secondary (MEDIUM confidence)
- [InfoQ: Pipeline Quality Gates](https://www.infoq.com/articles/pipeline-quality-gates/) - Gate placement, phased implementation
- [Sonar: Quality Gates in CI/CD](https://www.sonarsource.com/learn/integrating-quality-gates-ci-cd-pipeline/) - Blocking patterns, threshold strategies
- [GitHub Actions Retry](https://github.com/marketplace/actions/retry-step) - Retry patterns for CI/CD
- [GNU Parallel Tutorial](https://www.gnu.org/software/parallel/parallel_tutorial.html) - Timeout, job control, fail handling

### Tertiary (LOW confidence)
- WebSearch results on code review automation 2026 - Integration patterns
- WebSearch results on CI/CD retry logic - Backoff strategies

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Uses existing Phase 3-4 infrastructure, proven tools
- Architecture patterns: HIGH - Extends established patterns, user decisions clear
- Pitfalls: MEDIUM - Based on CI/CD best practices, not project-specific incidents yet
- Code review integration: MEDIUM - Standard patterns, implementation details need validation

**Research date:** 2026-01-30
**Valid until:** 2026-03-01 (30 days - patterns stable, may need threshold calibration after real usage)
