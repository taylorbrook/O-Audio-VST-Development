# Architecture Patterns for Multi-Agent Quality Systems

**Domain:** Multi-agent AI systems for professional audio plugin development
**Researched:** 2026-01-29
**Overall Confidence:** HIGH (Context7 + official docs + industry sources)

## Executive Summary

Multi-agent systems that produce professional-quality output follow distinct architectural patterns that differ fundamentally from amateur approaches. The key insight from 2026 research: **quality comes from constraints, contracts, and verification loops** - not from more powerful models alone.

The dominant pattern emerging in 2026 is the **Planner-Executor-Verifier triad** with specialized agents, clear handoff contracts, and quality gates at each transition. Systems without these patterns produce "10x more tokens but only marginally better output" (Anthropic research: multi-agent outperformed single-agent by 90.2% but consumed 15x more tokens).

For professional audio plugin development specifically, quality requires:
1. **Domain expertise encoded in agent specs** (DSP real-time constraints, UI thread safety)
2. **Verification agents that understand the domain** (not generic code review)
3. **Quality gates with measurable criteria** (pluginval, auval, crash-free operation)

---

## Recommended Architecture

### System Overview

```
+---------------------------------------------------------------------+
|                         ORCHESTRATOR                                 |
|   (routes work, maintains state, enforces quality gates)            |
+---------------------------------------------------------------------+
          |                    |                    |
          v                    v                    v
+-----------------+  +-----------------+  +-----------------+
|   DISCUSS       |  |   RESEARCH      |  |     PLAN        |
|   PHASE         |  |   PHASE         |  |     PHASE       |
|                 |  |                 |  |                 |
| - Clarify scope |  | - Context7 docs |  | - Task breakdown|
| - Validate req  |  | - Domain survey |  | - Complexity    |
| - User approval |  | - Pattern match |  | - Dependencies  |
+--------+--------+  +--------+--------+  +--------+--------+
         |                    |                    |
         v                    v                    v
         +--------------------+--------------------+
                              |
                              v
         +----------------------------------------+
         |           EXECUTE PHASE                 |
         |                                         |
         |  +---------+  +---------+  +---------+ |
         |  | DSP     |  | GUI     |  | Polish  | |
         |  | Agent   |  | Agent   |  | Agent   | |
         |  +----+----+  +----+----+  +----+----+ |
         |       |            |            |      |
         +-------+------------+------------+------+
                 |            |            |
                 v            v            v
         +----------------------------------------+
         |           VERIFY PHASE                  |
         |                                         |
         |  +-------------------------------------+|
         |  |     VALIDATION AGENT                ||
         |  |                                     ||
         |  | - Contract compliance               ||
         |  | - Domain-specific checks            ||
         |  | - Runtime validation (pluginval)    ||
         |  | - Quality gate enforcement          ||
         |  +-------------------------------------+|
         |                                         |
         +--------------+-------------------------+
                       |
                       v
              QUALITY GATE DECISION
              +-- PASS -> Next Stage
              +-- FAIL -> Return to Execute with feedback
```

### Component Boundaries

| Component | Responsibility | Communicates With | Quality Constraint |
|-----------|---------------|-------------------|-------------------|
| Orchestrator | Route work, maintain state, enforce gates | All agents | Never executes domain work |
| Research Agent | Survey patterns, verify capabilities | Context7, WebSearch, Orchestrator | Must cite sources, confidence levels |
| Planning Agent | Decompose tasks, estimate complexity | Research output, Orchestrator | Must produce measurable milestones |
| DSP Agent | Implement audio processing | Planning contracts, Validator | Must follow real-time safety rules |
| GUI Agent | Implement user interface | Planning contracts, Validator | Must follow thread-safety rules |
| Validation Agent | Verify all quality criteria | All execution outputs | Cannot implement, only verify |

### Data Flow

**Contract Flow (Upstream):**
```
BRIEF.md -> parameter-spec.md -> ARCHITECTURE.md -> ROADMAP.md
    |              |                  |               |
    |              |                  |               +-- Task breakdown per phase
    |              |                  +-- Technical implementation plan
    |              +-- Parameter definitions with DSP mappings
    +-- Creative intent, sonic goals, user requirements
```

**Execution Flow (Downstream):**
```
ROADMAP.md -> Agent Invocation -> Code Output -> Validation -> Gate Decision
                    |                |             |
                    |                |             +-- JSON report with pass/fail
                    |                +-- Modified files list
                    +-- Contract files + complexity score
```

---

## Patterns to Follow

### Pattern 1: Generator-Critic Loop

**What:** One agent creates, another validates, with iterative refinement.

**When:**
- Complex implementations where first-pass quality is insufficient
- High-stakes outputs (DSP that must be real-time safe)
- Outputs requiring domain expertise to evaluate

**Why it produces professional output:**
Research shows "first-pass AI outputs are rarely optimal." The Generator-Critic pattern "converts AI from a generator into a self-correcting system, dramatically improving reliability."

**Implementation:**
```
1. Generator Agent produces output
2. Critic Agent evaluates against contract + domain rules
3. If FAIL: Critic returns specific feedback
4. Generator incorporates feedback, produces revision
5. Loop until PASS or max iterations reached
6. Human escalation if iterations exhausted
```

**Example for DSP:**
```json
{
  "generator": "dsp-agent",
  "critic_checks": [
    "real_time_safety: No allocations in processBlock()",
    "parameter_connection: All params from spec connected",
    "domain_correctness: DSP matches ARCHITECTURE.md",
    "denormal_protection: ScopedNoDenormals present"
  ],
  "max_iterations": 3,
  "on_iteration_exhausted": "escalate_to_human"
}
```

### Pattern 2: Contract-Driven Handoffs

**What:** Each agent receives explicit input contracts and produces explicit output contracts.

**When:** Always. This is foundational.

**Why it produces professional output:**
"Each agent has clear constraints (Planner can't write code, Implementer can't redesign) and produces structured documents that create an audit trail." This prevents scope creep and ensures accountability.

**Contract Structure:**
```yaml
# Input Contract for DSP Agent
input_contract:
  required_files:
    - path: ".planning/research/ARCHITECTURE.md"
      must_contain: ["DSP Components", "Processing Chain"]
    - path: ".planning/parameter-spec.md"
      must_contain: ["Parameter Mappings"]
    - path: ".planning/ROADMAP.md"
      must_contain: ["complexity_score"]

  precondition_checks:
    - "Stage 1 complete (APVTS exists)"
    - "All parameters defined in foundation"

# Output Contract from DSP Agent
output_contract:
  modified_files:
    - "Source/PluginProcessor.cpp"
    - "Source/PluginProcessor.h"

  verification_criteria:
    - "All DSP components from ARCHITECTURE.md implemented"
    - "All parameters connected to processing"
    - "Real-time safety maintained"

  json_report:
    required_fields: ["agent", "status", "outputs", "issues", "ready_for_next_stage"]
```

### Pattern 3: Quality Gates with Measurable Criteria

**What:** Stage transitions blocked until specific, measurable quality criteria pass.

**When:** Every stage boundary.

**Why it produces professional output:**
"To keep prompt or model changes from breaking production, you need versioning, eval gates, and safe rollout." Quality gates make problems visible before they cascade.

**Audio Plugin Quality Gate Example:**

| Stage | Gate Criteria | Verification Method | Blocking? |
|-------|--------------|---------------------|-----------|
| 0 (Research) | ARCHITECTURE.md complete | Contract validator | Yes |
| 1 (Foundation) | Plugin loads without crash | pluginval smoke test | Yes |
| 2 (DSP) | Audio processes correctly | pluginval functional test | Yes |
| 3 (GUI) | UI operates without crash | pluginval GUI test | Yes |
| 4 (Polish) | DAW compatibility verified | Multi-DAW test suite | Yes |

**Implementation:**
```python
# Quality gate enforcement
def quality_gate(stage: int, plugin_path: str) -> dict:
    """
    Returns: {"passed": bool, "checks": [...], "blocking": bool}
    """
    checks = []

    if stage == 2:  # DSP
        # Semantic checks
        checks.append(verify_realtime_safety(plugin_path))
        checks.append(verify_parameter_connections(plugin_path))
        checks.append(verify_contract_compliance(plugin_path))

        # Runtime checks (only if binary exists)
        if binary_exists(plugin_path):
            checks.append(run_pluginval_functional(plugin_path))

    passed = all(c["passed"] for c in checks)
    return {"passed": passed, "checks": checks, "blocking": True}
```

### Pattern 4: Tiered Verification Depth

**What:** Different verification depths for different complexity levels.

**When:** Allocating verification resources.

**Why it produces professional output:**
Prevents over-verification of simple tasks and under-verification of complex ones. "Complexity score" drives verification depth.

**Implementation:**
```yaml
verification_tiers:
  complexity_1_2:  # Simple plugins
    dsp_verification: "single-pass"
    pluginval_level: "smoke_test"
    human_review: "optional"

  complexity_3:    # Moderate plugins
    dsp_verification: "generator-critic (2 iterations)"
    pluginval_level: "functional_test"
    human_review: "recommended"

  complexity_4_5:  # Complex plugins
    dsp_verification: "generator-critic (3 iterations)"
    pluginval_level: "full_gui_test"
    human_review: "required"
    extended_thinking: true
```

### Pattern 5: Specialized Domain Agents

**What:** Agents with deeply encoded domain expertise, not generic capabilities.

**When:** Tasks requiring professional-grade domain knowledge.

**Why it produces professional output:**
"The reflection pattern enables an agent to critically evaluate its own output." But generic reflection misses domain-specific issues. Specialized agents catch domain-specific problems.

**Amateur Pattern (Generic):**
```
Agent: "Generate audio plugin DSP code"
Output: Code that compiles but has:
  - Allocations in processBlock()
  - Missing denormal protection
  - Parameter zipper noise
  - CPU spikes
Result: "Works" but sounds amateur
```

**Professional Pattern (Specialized):**
```
DSP Agent Spec includes:
  - Real-time safety rules (no allocation, no locks)
  - JUCE 8 specific patterns (ScopedNoDenormals, SmoothedValue)
  - Parameter smoothing requirements
  - DSP quality checklist (denormals, DC offset, numerical stability)

Output: Code that follows all domain constraints
Result: Professional-quality audio processing
```

---

## Anti-Patterns to Avoid

### Anti-Pattern 1: Single Monolithic Agent

**What:** One agent handles all phases (research, plan, execute, verify).

**Why bad:**
- No separation of concerns
- Verification bias (agent validates own work)
- Context window explosion
- No audit trail

**Instead:** Use specialized agents with clear boundaries.

### Anti-Pattern 2: Generic Verification

**What:** Using a generic "code review" agent instead of domain-specific validation.

**Why bad:**
- Misses domain-specific quality issues
- DSP real-time violations undetected
- UI thread-safety issues undetected
- "Looks correct" but crashes in production

**Instead:** Domain-specific validation agents with encoded expertise.

### Anti-Pattern 3: Trust-Based Handoffs

**What:** Assuming previous stage output is correct without verification.

**Why bad:**
- Errors cascade and compound
- Late detection = expensive fixes
- "Works on my machine" syndrome

**Instead:** Verify contracts at every boundary.

### Anti-Pattern 4: Unlimited Iteration Loops

**What:** Generator-Critic loops without max iterations or escalation.

**Why bad:**
- Infinite loops possible
- Token budget explosion
- Diminishing returns after ~3 iterations

**Instead:** Max 3 iterations, then escalate to human.

### Anti-Pattern 5: Premature Optimization

**What:** Adding complexity (more agents, more patterns) before validating simpler approaches work.

**Why bad:**
- "Latency accumulation" from multi-hop communication
- "Resource constraints ignored"
- Harder to debug

**Instead:** Start with simplest pattern that might work, add complexity only when needed.

---

## Professional Audio Plugin Quality Standards

### Real-Time Audio Constraints

**Absolute Requirements (violations = catastrophic failure):**

| Constraint | Reason | Detection |
|------------|--------|-----------|
| No allocations in processBlock() | Causes audio dropouts | Static analysis |
| No locks/mutex | Causes priority inversion | Static analysis |
| No file I/O | Unbounded latency | Static analysis |
| No exceptions | Unwinds audio stack | Compiler flags |
| Bounded execution time | Buffer underrun | Runtime profiling |

**Best Practices (violations = amateur quality):**

| Practice | Reason | Detection |
|----------|--------|-----------|
| ScopedNoDenormals | 10-100x CPU spikes | Pattern matching |
| SmoothedValue for params | Zipper noise | Pattern matching |
| Pre-allocated buffers | Runtime allocation | prepareToPlay() audit |
| Atomic parameter reads | Thread safety | APVTS usage patterns |

### DSP Quality Indicators

**Professional Quality:**
- Transparent when bypassed (no coloration)
- Smooth parameter transitions (no clicks/pops)
- Consistent across sample rates (44.1k to 192k)
- CPU efficient (measured, profiled)
- Numerically stable (no DC offset accumulation)

**Amateur Indicators:**
- Zipper noise on parameter changes
- CPU spikes with certain parameter values
- Different behavior at different sample rates
- Audible noise floor
- Crashes with extreme parameter values

### UI Quality Indicators

**Professional Quality:**
- Responsive (< 16ms update latency)
- Thread-safe (no audio thread access from UI)
- State-consistent (UI matches audio state)
- Accessible (keyboard navigation, contrast ratios)
- Polished (consistent spacing, alignment, typography)

**Amateur Indicators:**
- UI freezes during audio processing
- Parameters don't match visual state
- Inconsistent visual styling
- Crashes on rapid parameter changes
- No keyboard navigation

### Stability Quality Indicators

**Professional Quality:**
- Passes pluginval at strictness level 10
- Works in all major DAWs (Logic, Ableton, Pro Tools, Reaper)
- Handles edge cases (zero-length buffers, extreme parameters)
- Clean shutdown (no crash on close)
- State save/restore works perfectly

**Amateur Indicators:**
- Fails pluginval tests
- Works in some DAWs but not others
- Crashes with certain parameter combinations
- Crashes on plugin close
- Preset recall doesn't match saved state

---

## Verification Strategies

### Semantic Verification (Code Analysis)

**What to check:**

```python
semantic_checks = {
    "realtime_safety": {
        "forbidden_patterns": [
            r"new\s+\w+",           # heap allocation
            r"malloc|realloc",      # C allocation
            r"std::mutex",          # locks
            r"std::lock_guard",     # locks
            r"File::",              # file I/O
            r"throw\s+\w+",         # exceptions
        ],
        "required_patterns": [
            r"ScopedNoDenormals",   # denormal protection
        ],
        "scope": "processBlock() method only"
    },
    "parameter_smoothing": {
        "check": "All parameters in signal path use SmoothedValue",
        "exceptions": ["bypass", "choice parameters"]
    },
    "buffer_preallocation": {
        "check": "All buffers allocated in prepareToPlay()",
        "verify": "No setSize() calls in processBlock()"
    }
}
```

### Runtime Verification (Binary Testing)

**Tiered pluginval testing:**

| Stage | Test Level | Timeout | Flags |
|-------|-----------|---------|-------|
| 1 (Foundation) | Smoke | 10s | `--skip-gui-tests --validate-in-process` |
| 2 (DSP) | Functional | 3min | `--skip-gui-tests --strictness-level 10` |
| 3 (GUI) | Full | 10min | `--strictness-level 10` |

**Critical pluginval checks:**
- Plugin loads/unloads without crash
- Parameters are automatable
- State save/restore is idempotent
- Thread safety (no allocations in audio thread)
- GUI opens/closes without crash

### Cross-Contract Verification

**What to check:**

```python
cross_contract_checks = {
    "parameter_count_match": {
        "sources": ["parameter-spec.md", "ARCHITECTURE.md", "PluginProcessor.cpp"],
        "rule": "All three must have same parameter count"
    },
    "parameter_id_match": {
        "sources": ["parameter-spec.md", "APVTS definition"],
        "rule": "IDs must match exactly (zero drift)"
    },
    "complexity_score_valid": {
        "sources": ["ROADMAP.md"],
        "rule": "Score matches formula: params/8 + algorithms + features"
    },
    "dsp_components_match": {
        "sources": ["ARCHITECTURE.md", "PluginProcessor.h"],
        "rule": "All specified components declared"
    }
}
```

---

## Build Order Implications

### Phase Dependencies

```
Stage 0 (Research/Plan)
    |
    +-- ARCHITECTURE.md defines DSP components
    +-- parameter-spec.md defines all parameters
    +-- ROADMAP.md defines complexity and phases

Stage 1 (Foundation) [depends on Stage 0]
    |
    +-- CMakeLists.txt builds correctly
    +-- APVTS has all parameters from spec
    +-- Plugin loads without crash

Stage 2 (DSP) [depends on Stage 1]
    |
    +-- All DSP components from ARCHITECTURE.md implemented
    +-- All parameters connected to DSP
    +-- Plugin processes audio correctly

Stage 3 (GUI) [depends on Stage 2]
    |
    +-- WebView loads and displays
    +-- All parameters have UI controls
    +-- Parameters bidirectionally synced

Stage 4 (Polish) [depends on Stage 3]
    |
    +-- All pluginval tests pass
    +-- DAW compatibility verified
    +-- Presets work correctly
```

### Critical Ordering Constraints

| Constraint | Reason | Violation Consequence |
|------------|--------|----------------------|
| APVTS before DSP | DSP reads from APVTS | Crash or undefined behavior |
| DSP before GUI | GUI displays DSP state | UI shows wrong values |
| Relays before WebView (in editor) | WebView binds to relays | Crash on binding |
| WebView before Attachments (in editor) | Attachments connect to WebView | Crash on connect |

---

## Agent Contract Design

### Input Contract Template

```yaml
agent: "[agent-name]"
version: "1.0"

input_contract:
  # Required files that MUST exist
  required_files:
    - path: ".planning/ARCHITECTURE.md"
      validation: "contains 'DSP Components' section"
    - path: ".planning/parameter-spec.md"
      validation: "JSON parseable with 'parameters' array"

  # Preconditions that MUST pass
  preconditions:
    - name: "previous_stage_complete"
      check: "STATUS.md shows stage N-1 complete"
    - name: "contracts_consistent"
      check: "Cross-contract validation passes"

  # On precondition failure
  on_failure: "return_error_immediately"
```

### Output Contract Template

```yaml
agent: "[agent-name]"
version: "1.0"

output_contract:
  # Files that WILL be modified/created
  outputs:
    - path: "Source/PluginProcessor.cpp"
      type: "modified"
    - path: "Source/PluginProcessor.h"
      type: "modified"

  # Verification criteria for outputs
  verification:
    - name: "compiles"
      method: "build_check"
      blocking: true
    - name: "realtime_safe"
      method: "semantic_analysis"
      blocking: true
    - name: "contract_compliant"
      method: "cross_contract_check"
      blocking: true

  # Report format
  report:
    format: "json"
    schema: ".claude/schemas/subagent-report.json"
    required_fields:
      - "agent"
      - "status"
      - "outputs"
      - "issues"
      - "ready_for_next_stage"
```

---

## Sources

### HIGH Confidence (Context7, Official Docs)

- [JUCE Framework Documentation](https://juce.com/documentation) - AudioProcessor patterns, real-time safety
- Context7 `/juce-framework/juce` - JUCE 8 API patterns, SmoothedValue usage
- [JUCE Audio Plugin Development Protocol](https://deepwiki.com/cline/prompts/4.3-juce-audio-plugin-development) - Professional development standards

### MEDIUM Confidence (Verified Multiple Sources)

- [Google's Eight Multi-Agent Design Patterns](https://www.infoq.com/news/2026/01/multi-agent-design-patterns/) - Generator-Critic, Coordinator patterns
- [Azure AI Agent Orchestration Patterns](https://learn.microsoft.com/en-us/azure/architecture/ai-ml/guide/ai-agent-design-patterns) - Sequential, Concurrent, Group Chat patterns
- [Anthropic Agent Evaluations](https://www.anthropic.com/engineering/demystifying-evals-for-ai-agents) - Verification strategies, grader types
- [Addy Osmani LLM Coding Workflow](https://addyosmani.com/blog/ai-coding-workflow/) - Professional vs amateur patterns

### LOW Confidence (Single Source, Unverified)

- Multi-agent 90.2% performance improvement claim (Anthropic internal research, cited but not primary source)
- 15x token consumption statistic (same source)

---

## Confidence Assessment

| Area | Confidence | Reason |
|------|------------|--------|
| Multi-agent patterns | HIGH | Multiple authoritative sources (Google, Microsoft, Anthropic) |
| Audio plugin quality standards | HIGH | Context7 + JUCE official docs + industry practice |
| Quality gate patterns | HIGH | Azure architecture + Anthropic engineering blog |
| Agent contract design | MEDIUM | Synthesized from patterns, not single authoritative source |
| Build order implications | HIGH | Direct from JUCE documentation and project experience |

---

## Roadmap Implications

Based on this research, the Plugin Freedom System overhaul should prioritize:

### Phase 1: Contract Foundation
- Define explicit input/output contracts for all 9 agents
- Implement contract validation at every boundary
- Add precondition checking before agent invocation

### Phase 2: Verification Layer
- Enhance validation-agent with domain-specific checks
- Implement tiered pluginval testing
- Add Generator-Critic loops for complex stages

### Phase 3: Quality Gates
- Implement blocking quality gates at stage transitions
- Add measurable criteria for each gate
- Create escalation paths for gate failures

### Phase 4: Agent Specialization
- Audit each agent for domain expertise encoding
- Add real-time safety rules to DSP agent
- Add thread-safety rules to GUI agent
- Add professional UI standards to design agent

### Research Flags for Later Phases

- **DSP Agent Enhancement:** Needs deeper research into specific algorithm quality (compression curves, filter resonance, saturation harmonics)
- **UI Design Agent:** Needs research into audio plugin aesthetic standards (skeuomorphism vs flat, readability under stage lighting)
- **Cross-DAW Compatibility:** Needs testing research for DAW-specific quirks (Logic AU cache, Ableton sample rate handling)

---

## v1.1 Improvements: Plugin-Improve Planning Phase Integration

**Added:** 2026-02-01
**Purpose:** Architecture for adding planning phase to plugin-improve workflow

### Current Workflow (Phase 0.5 Investigation)

```
Phase 0: Specificity Detection
  |
Phase 0.3: Clarification Questions
  |
Phase 0.4: Decision Gate
  |
Phase 0.45: Research Detection (MANDATORY - scan for deep-research handoff)
  |
  +-- [Research found?] ----YES----> Skip to Phase 0.9
  |
Phase 0.5: Investigation (Tier 1/2/3 auto-detected)
  |           |
  |           +-- Tier 1: Basic Code Inspection (5-10 min)
  |           +-- Tier 2: Root Cause Analysis (15-30 min)
  |           +-- Tier 3: Deep Research Delegation (30-60 min)
  |
  v
Phase 0.9: Backup Verification (CRITICAL GATE)
  |
Phase 1: Pre-Implementation Checks
  |
[...remaining phases...]
```

### Proposed Changes (With Planning Phase)

```
Phase 0.5: Investigation (Tier 1/2/3 auto-detected)
  |
  +-- Tier 1 -----> Skip planning, proceed to Phase 0.9
  |
  +-- Tier 2/3 ---> PHASE 0.6: PLANNING (NEW)
                      |
                      +-- Read investigation findings
                      +-- Generate implementation plan
                      +-- Present plan for approval
                      +-- Create IMPROVEMENT-PLAN.md (optional artifact)
                      |
                      v
                    Phase 0.9: Backup Verification
```

### Decision: Planning Phase Trigger

**Trigger Condition:** Tier 2 or Tier 3 investigation completed

**Rationale:**
- Tier 1 fixes are simple (5-10 min) - planning overhead not justified
- Tier 2/3 improvements are complex and benefit from explicit planning
- Aligns with existing tier detection logic in Phase 0.5

**Implementation:** Add conditional branch after Phase 0.5 that checks investigation tier.

---

### New Artifacts Required

#### 1. Reference File: `references/planning-protocol.md`

**Location:** `.claude/skills/plugin-improve/references/planning-protocol.md`

**Purpose:** Define the planning phase process, decision gates, and output format.

**Contents:**
- When planning is triggered (Tier 2/3 only)
- Planning process steps
- Plan approval workflow
- Skip conditions (user can decline planning)

**Pattern:** Matches existing reference files (`investigation-tiers.md`, `regression-testing.md`)

#### 2. Template File: `assets/planning-template.md`

**Location:** `.claude/skills/plugin-improve/assets/planning-template.md`

**Purpose:** Structured format for improvement plans.

**Contents:**
```markdown
# Improvement Plan: [PluginName] v[X.Y.Z]

## Summary
[One-sentence description of the improvement]

## Investigation Findings
**Root Cause:** [From Phase 0.5]
**Affected Files:** [List]
**Complexity:** Tier [2/3]

## Implementation Steps
1. [Step with specific file and changes]
2. [Step with specific file and changes]
...

## Risk Assessment
- Breaking changes: [Yes/No - if yes, details]
- Regression risk: [Low/Medium/High]
- Rollback complexity: [Simple/Moderate/Complex]

## Testing Strategy
- [ ] Unit tests needed: [Yes/No]
- [ ] Manual verification: [Description]
- [ ] Regression tests: [If baseline exists]

## Estimated Duration
[X minutes/hours]
```

#### 3. Optional: Plugin-Local Plan File

**Location:** `plugins/[PluginName]/.planning/improvements/IMPROVEMENT-PLAN-[version].md`

**Purpose:** Persist complex plans for reference during implementation.

**When created:**
- Tier 3 improvements only (complex enough to warrant persistence)
- User requests plan persistence
- Multi-session improvements

**When skipped:**
- Tier 2 improvements (plan presented inline, not persisted)
- User declines planning

---

### Data Flow Through Planning Phase

```
INPUTS (from Phase 0.5 Investigation):
  |
  +-- Investigation tier (2 or 3)
  +-- Root cause analysis
  +-- Affected files list
  +-- Recommended approach
  +-- Alternative approaches
  +-- Breaking change assessment
  |
  v
PHASE 0.6: PLANNING PROCESS
  |
  +-- 1. Validate investigation tier >= 2
  +-- 2. Structure implementation steps from investigation
  +-- 3. Assess risks (breaking changes, regressions)
  +-- 4. Define testing strategy
  +-- 5. Estimate duration
  +-- 6. Generate plan (inline or file)
  |
  v
OUTPUTS (to Phase 0.9 and beyond):
  |
  +-- Structured implementation plan
  +-- Risk assessment
  +-- Testing checklist
  +-- (Optional) IMPROVEMENT-PLAN.md artifact
  |
  v
USER APPROVAL GATE
  |
  +-- Present plan
  +-- Options: Approve, Revise, Skip planning
  +-- Wait for user decision
```

---

### SKILL.md Modifications Required

**File:** `.claude/skills/plugin-improve/SKILL.md`

**Changes Required:**

1. **Add Phase 0.6 definition** after Phase 0.5
2. **Add conditional branch** in Phase 0.5 to trigger planning
3. **Update workflow diagram** in Overview section
4. **Update progress checklist** to include Phase 0.6
5. **Add reference link** to new `planning-protocol.md`

**Minimal Insertion Pattern:**

Insert between Phase 0.5 and Phase 0.9:

```markdown
## Phase 0.6: Planning (Tier 2/3 Only)

**CONDITIONAL:** Only executes if Phase 0.5 detected Tier 2 or Tier 3 complexity.

**Purpose:** Structure implementation approach before proceeding.

**Workflow:**

1. **Generate plan** from investigation findings using template
2. **Present plan** with decision menu
3. **Wait for approval** before proceeding

**See**: [references/planning-protocol.md](references/planning-protocol.md) for detailed process and plan template.

**If Tier 1:** Skip this phase, proceed directly to Phase 0.9.

**Decision Menu:**
```
Implementation plan ready.

1. Approve plan - Proceed to Phase 0.9 (Backup)
2. Revise plan - Adjust approach
3. Skip planning - Proceed without formal plan
4. Cancel - Stop improvement workflow

Choose (1-4): _
```
```

---

### Integration Points With Existing System

#### 1. Phase 0.5 Investigation Output

**Current behavior:** Investigation findings presented inline, user approves before proceeding.

**New behavior:** If Tier 2/3, investigation findings flow into Phase 0.6 planning.

**Interface:** No schema change needed - investigation output is already structured in SKILL.md.

#### 2. Handoff Protocol Compatibility

**deep-research handoff:** Phase 0.45 detection still works. If research detected:
- Skip Phase 0.5 (investigation)
- Skip Phase 0.6 (planning) - research already includes recommendations
- Proceed to Phase 0.9

**Rationale:** Deep research (Opus + extended thinking) already produces structured recommendations. Adding planning phase would be redundant.

#### 3. Tier Detection Enhancement

**Current tier detection** (in Phase 0.5):
- Tier 1: Simple fixes, single file, obvious cause
- Tier 2: Root cause analysis, integration issues
- Tier 3: Complex bugs, multi-component, unclear cause

**Planning trigger:** `tier >= 2`

**No change needed** to tier detection logic - just conditional branch after.

#### 4. Backup Verification (Phase 0.9)

**Unchanged.** Planning phase completes before backup verification.

**Dependency:** Phase 0.9 MUST NOT execute until Phase 0.6 approves plan (or skips).

---

### Implementation Order for v1.1

#### Phase 1: Reference Documentation (30 min)

1. Create `references/planning-protocol.md` with:
   - Planning trigger conditions
   - Planning process steps
   - Decision menu format
   - Skip conditions

2. Create `assets/planning-template.md` with:
   - Plan structure template
   - Risk assessment section
   - Testing checklist

#### Phase 2: SKILL.md Update (20 min)

1. Add Phase 0.6 section to SKILL.md
2. Update workflow diagram
3. Update progress checklist
4. Add conditional branch in Phase 0.5

#### Phase 3: Testing (15 min)

1. Test Tier 1 improvement (should skip planning)
2. Test Tier 2 improvement (should trigger planning)
3. Test deep-research handoff (should skip both investigation AND planning)

---

### Patterns Followed (Not Invented)

| Pattern | Source | Applied |
|---------|--------|---------|
| Phase numbering (0.X) | Existing phases 0.3, 0.4, 0.45, 0.5, 0.9 | Phase 0.6 |
| Reference file structure | `references/investigation-tiers.md` | `references/planning-protocol.md` |
| Asset templates | `assets/backup-template.sh` | `assets/planning-template.md` |
| Conditional phases | Phase 0.45 skip logic, Phase 5.5 conditional | Planning conditional on tier |
| Decision menus | All checkpoint protocols | Planning approval menu |
| Handoff compatibility | Phase 0.45 research detection | Deep-research skips planning |

### Anti-Patterns Avoided

| Anti-Pattern | Why Avoided |
|--------------|-------------|
| New skill creation | Planning is a phase, not a separate skill |
| Schema changes | Existing investigation output sufficient |
| Breaking handoff protocol | Deep-research integration preserved |
| Mandatory planning | Made conditional (skip option available) |
| Tier detection modification | Leverages existing tier detection unchanged |

---

### Open Questions

1. **Plan persistence threshold:** Should Tier 2 plans be persisted to files, or only Tier 3? Current recommendation: Tier 3 only (or user request).

2. **Plan revision workflow:** If user selects "Revise plan", should this loop back to Phase 0.5 or allow inline editing? Recommendation: Allow inline revision without re-investigation.

3. **Express mode interaction:** If plugin is in express mode (registry.json `expressMode: true`), should planning be skipped entirely? Recommendation: Yes, express mode skips planning.

---

*Architecture research: 2026-01-29*
*v1.1 addition: 2026-02-01*
*Researcher: gsd-project-researcher agent*
