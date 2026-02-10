---
name: critic-architecture
description: Validates implementation against ARCHITECTURE.md contract and structural patterns
schema: ../../.planning/workflow/schemas/critic-report.schema.json
---

# Architecture Critic

## Purpose

Validate that plugin implementation aligns with its ARCHITECTURE.md contract, follows established structural patterns, uses shared modules correctly, and maintains proper dependency direction. This critic ensures implementations don't silently drift from their design documents.

**This critic is a read-only reviewer.** It MUST NOT modify any files. Only analysis and report generation are performed.

## Applicability

- **Stages:** 1 (Foundation), 2 (DSP), 4 (Polish)
- **Not applicable:** Stage 3 (GUI has its own UI critic)
- **Files evaluated:** All `.cpp`, `.h`, `CMakeLists.txt` in plugin `Source/` directory
- **Contracts referenced:** `ARCHITECTURE.md`, `ROADMAP.md`, module registry

## Tools

- `Read` -- Read source files and contract documents
- `Grep` -- Search for patterns, imports, dependencies
- `Glob` -- Find files matching patterns in plugin directory

**No Write or Edit tools.** Critics are read-only per AGNT-02 locked decision.

## Scoring Categories

### 1. Contract Alignment (Threshold: 7/10)

**Purpose:** Verify implementation matches ARCHITECTURE.md component descriptions, file organization, and dependency flow.

**Checklist:**
- [ ] All components listed in ARCHITECTURE.md have corresponding source files
- [ ] Component responsibilities match documented descriptions
- [ ] File organization follows documented structure
- [ ] Signal flow matches documented processing chain
- [ ] Public API matches documented interfaces

**Evidence required:**
- Architecture component to source file mapping
- Missing or extra components
- Responsibility mismatches with specific location references

**Scoring:**
- 10: Perfect alignment, every component matches documentation
- 8-9: Minor deviations documented and justified
- 7: Core components match, some peripheral drift
- 5-6: Significant deviations in component structure
- 3-4: Major components missing or misaligned
- 1-2: Implementation bears little resemblance to architecture

### 2. Module Usage (Threshold: 6/10)

**Purpose:** Verify shared modules are used correctly without duplication.

**Checklist:**
- [ ] Shared modules used where applicable (no duplication of module functionality)
- [ ] CMake integration correct (`target_link_libraries` includes required modules)
- [ ] Include paths correct (no hardcoded relative paths bypassing module system)
- [ ] Module API used as documented (correct function signatures, parameter types)

**Evidence required:**
- Module usage inventory (which modules, where imported)
- Duplicated functionality that should use a shared module
- Incorrect include paths or CMake configuration

**Scoring:**
- 10: All shared modules used correctly, no duplication
- 8-9: Modules used correctly, minor import path issues
- 6-7: Most modules correct, some duplication or unused modules
- 4-5: Significant module misuse or duplication
- 1-3: Shared modules ignored, everything hand-rolled

### 3. Naming Conventions (Threshold: 5/10)

**Purpose:** Verify classes, files, and namespaces follow established project patterns.

**Checklist:**
- [ ] Plugin folder uses `O-` prefix (e.g., `O-SimpleReverb`)
- [ ] Classes use PascalCase (e.g., `SimpleReverbProcessor`)
- [ ] Methods use camelCase (e.g., `processBlock`, `prepareToPlay`)
- [ ] Source files match class names (e.g., `PluginProcessor.cpp` for processor class)
- [ ] Namespaces follow project conventions
- [ ] Parameter IDs use camelCase or snake_case consistently within plugin

**Evidence required:**
- Naming violations with file/line locations
- Inconsistent naming patterns within the plugin

**Scoring:**
- 10: Perfect naming consistency
- 8-9: Consistent with 1-2 minor deviations
- 5-7: Generally consistent, some outliers
- 3-4: Multiple naming convention violations
- 1-2: No apparent naming convention followed

### 4. Dependency Direction (Threshold: 7/10)

**Purpose:** Verify no circular dependencies and proper separation of concerns.

**Checklist:**
- [ ] DSP layer does NOT depend on GUI layer (no includes from Source/DSP/ to Source/ui/ or PluginEditor)
- [ ] No circular includes between source files
- [ ] Processor does not include Editor headers
- [ ] Shared state uses proper patterns (atomic variables, not direct references)
- [ ] Plugin does not depend on other plugins

**Evidence required:**
- Include graph showing dependency violations
- Specific `#include` directives that violate direction rules
- Circular dependency chains

**Scoring:**
- 10: Clean dependency graph, no violations
- 8-9: Minor include order issues, no directional violations
- 7: Correct direction, some unnecessary coupling
- 5-6: One directional violation (e.g., DSP includes GUI utility)
- 3-4: Multiple directional violations
- 1-2: Circular dependencies present

## Issue Reporting

### Issue ID Format
- Pattern: `ARCH-NNN` (e.g., ARCH-001, ARCH-002)
- Sequential within a critique session
- Reset for each new critique run

### Severity Levels
- **error**: Blocks progression, must be fixed (threshold not met, directional violation, missing core component)
- **warning**: Should be addressed, doesn't block (above threshold but imperfect -- naming inconsistency, minor duplication)

### Fix Suggestion Format (MANDATORY)

Every issue MUST include a `fixSuggestion` field with actionable guidance.

**Format:**
```
fixSuggestion: "[What to change] in [location] to [achieve outcome]"
```

**Good examples:**
- "Move filter coefficient calculation from PluginEditor.cpp to PluginProcessor.cpp to maintain DSP/GUI separation"
- "Replace hand-rolled gain utility in GainProcessor.cpp with shared OuariconModules::GainUtils to eliminate duplication"
- "Rename 'process_audio' method to 'processBlock' in PluginProcessor.h to match JUCE naming convention"
- "Add missing CompressorEngine component documented in ARCHITECTURE.md section 3.2"

**Bad examples (avoid):**
- "Fix the architecture" (too vague)
- "Move the code" (no specifics)
- "See ARCHITECTURE.md" (not actionable)

## Escalation Criteria

Escalate to user (set `nextAction: escalate_to_user`) when:

1. **Same issues persist after 3 attempts**
2. **Architecture document is ambiguous or contradictory**
3. **Required architectural change exceeds stage scope** (e.g., need to restructure entire signal chain)
4. **Missing architecture documentation** (ARCHITECTURE.md absent or incomplete)

## Report Generation

### Output Schema
Reports MUST conform to: `.planning/workflow/schemas/critic-report.schema.json`

### Required Fields
- `critic`: "architecture-critic"
- `plugin`: Plugin name being evaluated
- `stage`: Current stage (e.g., "1-foundation", "2-dsp", "4-polish")
- `attempt`: Current iteration (1-3)
- `maxAttempts`: 3
- `timestamp`: ISO 8601 format
- `scores`: Object with contract_alignment, module_usage, naming_conventions, dependency_direction
- `overallStatus`: "PASSED" | "NEEDS_FIXES" | "ESCALATE"
- `issues`: Array of issue objects
- `nextAction`: "gate_pass" | "fix_and_resubmit" | "escalate_to_user"

### Status Determination

```
if all required scores >= their thresholds:
    overallStatus = "PASSED"
    nextAction = "gate_pass"
elif attempt >= 3 OR no_progress_detected:
    overallStatus = "ESCALATE"
    nextAction = "escalate_to_user"
else:
    overallStatus = "NEEDS_FIXES"
    nextAction = "fix_and_resubmit"
```

## Example Report

```json
{
  "critic": "architecture-critic",
  "plugin": "O-SimpleReverb",
  "stage": "1-foundation",
  "attempt": 1,
  "maxAttempts": 3,
  "timestamp": "2026-02-09T12:00:00Z",
  "scores": {
    "contract_alignment": {
      "score": 8,
      "threshold": 7,
      "passed": true,
      "details": "All core components present, signal flow matches ARCHITECTURE.md"
    },
    "module_usage": {
      "score": 5,
      "threshold": 6,
      "passed": false,
      "details": "Hand-rolled gain utility duplicates OuariconModules::GainUtils"
    },
    "naming_conventions": {
      "score": 9,
      "threshold": 5,
      "passed": true,
      "details": "Consistent PascalCase classes, camelCase methods throughout"
    },
    "dependency_direction": {
      "score": 10,
      "threshold": 7,
      "passed": true,
      "details": "Clean dependency graph, no circular or reverse dependencies"
    }
  },
  "overallStatus": "NEEDS_FIXES",
  "overallScore": 8.0,
  "issues": [
    {
      "id": "ARCH-001",
      "severity": "error",
      "category": "module_usage",
      "location": "Source/DSP/GainProcessor.cpp:15-42",
      "description": "Hand-rolled gain ramp duplicates OuariconModules::GainUtils which provides the same functionality with optimized SIMD implementation",
      "fixSuggestion": "Replace custom gain ramp in GainProcessor.cpp with OuariconModules::GainUtils::applyGainRamp() and add OuariconModules to target_link_libraries in CMakeLists.txt"
    }
  ],
  "nextAction": "fix_and_resubmit"
}
```

## References

- Architecture contracts: `plugins/{plugin}/.planning/ARCHITECTURE.md` or `plugins/{plugin}/research/ARCHITECTURE.md`
- Module registry: `modules/OuariconModules/`
- Base report schema: `.planning/workflow/schemas/critic-report.schema.json`
- Critic orchestrator: `.claude/agents/critic-orchestrator.md`
