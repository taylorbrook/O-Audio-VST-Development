# Phase 16: GSD Deduplication - Research

**Researched:** 2026-02-09
**Domain:** GSD framework alignment / custom code elimination
**Confidence:** HIGH

## Summary

Phase 16 replaces custom PFS code that duplicates GSD 1.18.0 functionality with framework equivalents. The investigation reveals three distinct deduplication domains: (1) STATE.md operations where the main execution pipeline already uses gsd-tools but peripheral workflows still do manual edits, (2) frontmatter validation where gsd-tools handles structural presence checks but the PFS research schema requires domain-specific semantic validation that must be retained, and (3) structural verification where gsd-tools already provides 6 verify commands that some agents use but others still perform ad-hoc inline checks.

A fourth requirement (GSDD-04) adds new functionality: deterministic post-plan context compliance validation that currently exists only as a soft agent dimension in gsd-plan-checker. This requires a new gsd-tools command rather than code removal.

The key risk is regression in the 35+ plugin production pipeline. Prior decisions mandate canary testing (O-SimpleReverb) after every change and classification of all custom code before removal.

**Primary recommendation:** Work in four stages -- audit and classify all dedup targets, migrate state operations to gsd-tools, replace structural validation callsites, then add the context-compliance verify command -- with O-SimpleReverb canary after each stage.

## Standard Stack

### Core

| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| gsd-tools.js | 1.18.0 (bundled) | CLI for state, frontmatter, verify, commit, scaffold operations | Single source of truth for GSD operations; already used by executor/planner/checker agents |
| Node.js | system | Runtime for gsd-tools.js | Already required by all GSD workflows |

### Supporting

| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| Python 3 | system | PFS domain validators | Retained for DSP/APVTS/WebView/checksum/cross-contract/resource-accountability validation |
| PyYAML | installed | Research frontmatter parsing | Retained for PFS-specific 10-field research schema validation |
| jq | system | JSON parsing in hooks | Used in SubagentStop.sh for input parsing |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Adding new gsd-tools commands for missing state operations | Manual inline edits (current approach) | Manual approach works but creates maintenance burden and inconsistency; gsd-tools commands are atomic and tested |
| Adding context-compliance to gsd-tools verify suite | Keep it as agent-only dimension in plan-checker | Agent dimension is non-deterministic; gsd-tools command ensures every run checks compliance consistently |
| Migrating PFS Python validators to JavaScript in gsd-tools | Keeping Python validators | Python validators are well-tested, domain-specific, and don't duplicate GSD -- no reason to port them |

## Architecture Patterns

### Current Architecture (Before Dedup)

```
Agent/Workflow Code
├── gsd-tools CLI calls (partial)
│   ├── state advance-plan         ← Used by executor, execute-plan
│   ├── state update-progress      ← Used by executor, execute-plan
│   ├── state record-metric        ← Used by executor, execute-plan
│   ├── state add-decision         ← Used by executor, execute-plan
│   ├── state record-session       ← Used by executor, execute-plan
│   ├── frontmatter get/validate   ← Used by plan-checker, verify-phase
│   ├── verify plan-structure      ← Used by plan-checker
│   └── phase complete             ← Used by transition
├── Manual STATE.md edits (to eliminate)
│   ├── quick.md: Quick Tasks table creation/row append/Last activity
│   ├── add-phase.md: Roadmap Evolution entries
│   ├── insert-phase.md: Roadmap Evolution entries
│   ├── new-milestone.md: Current Position section rewrite
│   ├── resume-project.md: Session Continuity rewrite + STATE.md reconstruction
│   ├── transition.md: Project Reference + Accumulated Context + Session Continuity
│   ├── check-todos.md: Pending Todos section update
│   └── complete-milestone.md: Post-milestone STATE.md cleanup
├── PFS Domain Validators (preserve)
│   ├── validate-dsp-components.py
│   ├── validate-parameters.py
│   ├── validate-gui-bindings.py
│   ├── validate-checksums.py
│   ├── validate-cross-contract.py
│   ├── validate-resource-accountability.py
│   ├── validate-silent-failures.py
│   └── validate-foundation.py
└── PFS Research Frontmatter Validator (preserve)
    └── validate-research-frontmatter.py (10-field schema)
```

### Target Architecture (After Dedup)

```
Agent/Workflow Code
├── gsd-tools CLI calls (complete)
│   ├── state advance-plan
│   ├── state update-progress
│   ├── state record-metric
│   ├── state add-decision / resolve-blocker / add-blocker
│   ├── state record-session
│   ├── state update <field> <value>       ← Used more broadly
│   ├── state patch --field val ...        ← Used for multi-field updates
│   ├── frontmatter get/set/merge/validate
│   ├── verify plan-structure
│   ├── verify phase-completeness
│   ├── verify references
│   ├── verify commits
│   ├── verify artifacts
│   ├── verify key-links
│   ├── verify context-compliance <phase>  ← NEW (GSDD-04)
│   └── phase complete
├── PFS Domain Validators (untouched)
│   └── [all 8 Python validators preserved as-is]
└── PFS Research Frontmatter Validator (untouched)
    └── validate-research-frontmatter.py
```

### Pattern 1: State Operation Migration

**What:** Replace inline Edit/Write STATE.md operations with gsd-tools state commands.
**When to use:** Any workflow/agent that reads STATE.md, modifies a section, and writes back.

**Current (manual):**
```markdown
<!-- In quick.md workflow -->
Read STATE.md and check for `### Quick Tasks Completed` section.
If section doesn't exist, create it.
Append new row to table.
Update "Last activity" line.
Use Edit tool to make these changes atomically.
```

**Target (gsd-tools):**
```bash
# Use existing state commands where they fit
node gsd-tools.js state update "Last activity" "${date} - Completed quick task ${next_num}: ${DESCRIPTION}"

# For section-level operations not yet in gsd-tools, use state patch
node gsd-tools.js state patch \
  --last-activity "${date} - Completed quick task ${next_num}" \
  --quick-task-row "| ${next_num} | ${DESCRIPTION} | ${date} | ${commit_hash} | ... |"
```

### Pattern 2: Structural Verification Delegation

**What:** Replace ad-hoc inline structural checks with gsd-tools verify commands.
**When to use:** Any agent/workflow that checks plan structure, references, or completeness.

**Current (inline):**
```bash
# Agent manually parses plan frontmatter
grep "must_haves:" "$PLAN_PATH"
# Agent manually checks task elements
grep -c "<task>" "$PLAN_PATH"
```

**Target (gsd-tools):**
```bash
RESULT=$(node gsd-tools.js verify plan-structure "$PLAN_PATH")
# Parse JSON: valid, errors, warnings, task_count, tasks
```

### Pattern 3: Context Compliance Verification

**What:** New deterministic check that CONTEXT.md decisions map to PLAN.md tasks.
**When to use:** After planning, before execution begins.

**Approach:**
```bash
# New command
node gsd-tools.js verify context-compliance "$PHASE_NUM"
# Returns: { compliant, decisions_covered, decisions_missing, deferred_violations, issues[] }
```

### Anti-Patterns to Avoid

- **Removing domain validators:** The 6 PFS domain validators (DSP, APVTS, WebView, checksums, cross-contract, resource accountability) do NOT duplicate GSD functionality. They validate VST plugin-specific correctness. Removing them would eliminate production safety nets.
- **Porting Python to JavaScript:** The PFS validators work correctly as Python. Rewriting them in JS for gsd-tools would create unnecessary risk with zero functional benefit.
- **Big-bang migration:** Changing all workflows at once risks subtle regression. Migrate one workflow at a time with canary testing.
- **Breaking STATE.md format:** gsd-tools state commands assume specific section headings and field patterns. If workflows have custom sections (like "Quick Tasks Completed"), new commands must handle those patterns.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Plan structure validation | Inline grep/regex parsing of PLAN.md frontmatter and tasks | `gsd-tools verify plan-structure` | Handles 8 required fields, task element checks, wave/depends_on consistency, checkpoint/autonomous alignment |
| Phase completeness checking | Manual ls + comparison of PLAN/SUMMARY file counts | `gsd-tools verify phase-completeness` | Handles plan/summary matching, orphan detection, proper ID extraction |
| Reference resolution | Manual file existence checks for @-refs | `gsd-tools verify references` | Handles @-references, backtick paths, tilde expansion, deduplication |
| Commit verification | Manual `git cat-file -t` loops | `gsd-tools verify commits` | Batch verification with proper error handling |
| Artifact checking | Manual file existence + line count + pattern matching | `gsd-tools verify artifacts` | Parses must_haves.artifacts, checks existence + min_lines + contains + exports |
| Key link verification | Manual file reading + pattern matching across source/target | `gsd-tools verify key-links` | Handles from/to/via/pattern with regex support |
| Frontmatter CRUD | Manual YAML regex parsing and splicing | `gsd-tools frontmatter get/set/merge` | Proper YAML extraction and reinsertion |
| STATE.md field updates | Manual regex find-and-replace with Write tool | `gsd-tools state update/patch` | Atomic updates with edge case handling |

**Key insight:** gsd-tools already implements all 6 structural verification commands and comprehensive state operations. The dedup work is primarily about replacing CALLSITES in agent/workflow instructions, not writing new verification logic.

## Common Pitfalls

### Pitfall 1: Confusing "Uses gsd-tools" with "Fully Migrated"

**What goes wrong:** Assuming a workflow is already migrated because it uses SOME gsd-tools commands, when it still has manual STATE.md edits for other sections.
**Why it happens:** The execution pipeline (executor, execute-plan) was migrated first. Peripheral workflows (transition, quick, add-phase) were not.
**How to avoid:** Grep every workflow file for `Edit tool` or `Write tool` operations on STATE.md. Each hit is a dedup target.
**Warning signs:** Workflow instructions that say "Read STATE.md" followed by "Use Edit tool to make these changes."

### Pitfall 2: Missing gsd-tools Commands for Edge Cases

**What goes wrong:** Trying to replace manual STATE.md edits with gsd-tools commands that don't exist yet.
**Why it happens:** gsd-tools state commands cover the main execution flow (advance-plan, record-metric, etc.) but not all section-level operations (Quick Tasks table, Roadmap Evolution, Project Reference).
**How to avoid:** Before rewriting workflow instructions, verify the target gsd-tools command exists. If not, the plan must include adding the command to gsd-tools.js.
**Warning signs:** Workflow instructions that create/modify STATE.md sections not covered by existing `state` subcommands.

### Pitfall 3: Breaking the SubagentStop Hook Chain

**What goes wrong:** Modifying the SubagentStop.sh hook's validator invocation flow breaks the PFS domain validation pipeline.
**Why it happens:** The hook dispatches to different Python validators based on subagent type. Changes to the hook structure can silently skip validators.
**How to avoid:** The 6 domain validators in SubagentStop.sh are explicitly OUT OF SCOPE. Do not touch SubagentStop.sh unless adding new structural verify calls alongside (not replacing) existing domain validators.
**Warning signs:** Any edit to SubagentStop.sh that removes a `python3 .claude/hooks/validators/` call.

### Pitfall 4: Context Compliance False Positives

**What goes wrong:** Automated CONTEXT.md-to-PLAN.md cross-referencing flags decisions as "not covered" when they're implicitly addressed.
**Why it happens:** A locked decision like "use card layout" might not appear verbatim in any task name but is implemented in a task action.
**How to avoid:** The context-compliance verify command should check both task names AND task action content for decision-related keywords. Use fuzzy matching, not exact string comparison.
**Warning signs:** Users getting frustrated by false positives and disabling the check.

### Pitfall 5: Research Frontmatter Validator Confusion

**What goes wrong:** Treating `validate-research-frontmatter.py` as a duplicate of `gsd-tools frontmatter validate`.
**Why it happens:** Both "validate frontmatter," but gsd-tools checks presence only while the Python validator checks semantic correctness (enum values, patterns, dates, JUCE version format).
**How to avoid:** Explicitly classify `validate-research-frontmatter.py` as a domain validator (retained), not a structural validator (replaced). It validates PFS-specific research document quality, not generic plan/summary structure.
**Warning signs:** Removing validate-research-frontmatter.py and losing enum/pattern validation on research documents.

## Code Examples

### Example 1: State Operation Currently in gsd-tools (USED by executor)
```bash
# Source: gsd-executor.md lines 330-348
node /Users/taylorbrook/.claude/get-shit-done/bin/gsd-tools.js state advance-plan
node /Users/taylorbrook/.claude/get-shit-done/bin/gsd-tools.js state update-progress
node /Users/taylorbrook/.claude/get-shit-done/bin/gsd-tools.js state record-metric \
  --phase "${PHASE}" --plan "${PLAN}" --duration "${DURATION}" \
  --tasks "${TASK_COUNT}" --files "${FILE_COUNT}"
node /Users/taylorbrook/.claude/get-shit-done/bin/gsd-tools.js state add-decision \
  --phase "${PHASE}" --summary "${decision}"
node /Users/taylorbrook/.claude/get-shit-done/bin/gsd-tools.js state record-session \
  --stopped-at "Completed ${PHASE}-${PLAN}-PLAN.md"
```

### Example 2: Manual STATE.md Edit in quick.md (DEDUP TARGET)
```markdown
<!-- Source: quick.md workflow Step 7 -->
Read STATE.md and check for `### Quick Tasks Completed` section.
If section doesn't exist, create it.
Insert after `### Blockers/Concerns` section.
Append new row to table.
Update "Last activity" line.
Use Edit tool to make these changes atomically.
```

### Example 3: Manual STATE.md Edit in transition.md (DEDUP TARGET)
```markdown
<!-- Source: transition.md step update_project_reference -->
Update Project Reference section in STATE.md:
## Project Reference
See: .planning/PROJECT.md (updated [today])
**Core value:** [Current core value from PROJECT.md]
**Current focus:** [Next phase name]
```

### Example 4: Structural Verify Already Used by Plan-Checker
```bash
# Source: gsd-plan-checker.md Step 2
for plan in "$PHASE_DIR"/*-PLAN.md; do
  PLAN_STRUCTURE=$(node /Users/taylorbrook/.claude/get-shit-done/bin/gsd-tools.js verify plan-structure "$plan")
  echo "$PLAN_STRUCTURE"
done
```

### Example 5: Context Compliance (Currently Agent Dimension, Target: CLI Command)
```markdown
<!-- Source: gsd-plan-checker.md Dimension 7 -->
Process:
1. Parse CONTEXT.md sections: Decisions, Claude's Discretion, Deferred Ideas
2. For each locked Decision, find implementing task(s)
3. Verify no tasks implement Deferred Ideas (scope creep)
4. Verify Discretion areas are handled (planner's choice is valid)
```

### Example 6: PFS Domain Validator (PRESERVED, NOT DEDUPED)
```bash
# Source: SubagentStop.sh lines 86-91
python3 .claude/hooks/validators/validate-parameters.py
PARAMS_RESULT=$?
if [ $PARAMS_RESULT -ne 0 ]; then
  echo "Parameter validation FAILED: Parameters from spec missing in code" >&2
  exit 2  # Block workflow
fi
```

### Example 7: PFS Research Frontmatter Validator (PRESERVED, NOT DEDUPED)
```python
# Source: validate-research-frontmatter.py lines 34-35
REQUIRED_FIELDS = {"title", "summary", "domain", "type", "keywords", "stages", "agents",
                   "created", "last_verified", "juce_version"}
# 10 fields with enum validation, pattern checking, date validation
# NOT replaceable by gsd-tools frontmatter validate (presence-only)
```

## Detailed Dedup Inventory

### GSDD-01: State Operations — Manual STATE.md Write Locations

| File | Section/Operation | Current Method | gsd-tools Replacement |
|------|-------------------|----------------|----------------------|
| `quick.md` | Quick Tasks table creation | Edit tool | New: `state add-quick-task` or `state patch` |
| `quick.md` | Last activity update | Edit tool | `state update "Last activity" "..."` |
| `add-phase.md` | Roadmap Evolution entry | Edit tool | `state add-decision --summary "Phase N added: ..."` |
| `insert-phase.md` | Roadmap Evolution entry | Edit tool | `state add-decision --summary "Phase N.X inserted: ..."` |
| `new-milestone.md` | Current Position rewrite | Write tool | `state patch` with multiple fields |
| `resume-project.md` | Session Continuity | Write tool | `state record-session` |
| `resume-project.md` | STATE.md reconstruction | Write tool | New: reconstruct from artifacts (complex) |
| `transition.md` | Project Reference section | Edit tool | `state update` for each field |
| `transition.md` | Accumulated Context decisions | Edit tool | `state add-decision` (already exists) |
| `transition.md` | Accumulated Context blockers | Edit tool | `state resolve-blocker` (already exists) |
| `transition.md` | Session Continuity | Edit tool | `state record-session` (already exists) |
| `check-todos.md` | Pending Todos section | Edit tool | `state update` or `state patch` |
| `complete-milestone.md` | Post-milestone cleanup | Edit tool | `state patch` with multiple fields |

### GSDD-02: Frontmatter Dedup Targets

| Validator | Schema | gsd-tools Compatible | Action |
|-----------|--------|---------------------|--------|
| gsd-tools `frontmatter validate --schema plan` | 8 fields (presence) | YES | Already in gsd-tools |
| gsd-tools `frontmatter validate --schema summary` | 6 fields (presence) | YES | Already in gsd-tools |
| gsd-tools `frontmatter validate --schema verification` | 4 fields (presence) | YES | Already in gsd-tools |
| `validate-research-frontmatter.py` | 10 fields + enums + patterns | NO (semantic) | PRESERVE as PFS domain validator |

### GSDD-03: Validator Classification

**REPLACE (structural -- gsd-tools verify suite covers these):**

| Structural Check | gsd-tools Command | Where Currently Duplicated |
|-----------------|-------------------|---------------------------|
| Plan structure | `verify plan-structure` | plan-checker uses it; check if any workflow does inline |
| Phase completeness | `verify phase-completeness` | verify-phase uses it; check for inline checks |
| Reference resolution | `verify references` | verify-phase may do inline @-ref checks |
| Commit verification | `verify commits` | executor does inline `git log --oneline` checks |
| Artifact existence | `verify artifacts` | verifier does manual file existence checks |
| Key link wiring | `verify key-links` | verifier does manual source-references-target checks |

**PRESERVE (domain -- PFS-specific, no GSD equivalent):**

| Domain Validator | What It Checks | Trigger Point |
|-----------------|----------------|---------------|
| `validate-dsp-components.py` | DSP components from architecture.md exist in PluginProcessor | SubagentStop (dsp-agent) |
| `validate-parameters.py` | Parameters from parameter-spec.md exist in APVTS code | SubagentStop (foundation-shell-agent) |
| `validate-gui-bindings.py` | HTML parameter IDs match C++ WebView relay IDs | SubagentStop (gui-agent) |
| `validate-checksums.py` | SHA256 checksums of contract files match STATUS.md | SubagentStop (all impl agents) |
| `validate-cross-contract.py` | Cross-contract consistency (params, components match) | SubagentStop (all impl agents) |
| `validate-resource-accountability.py` | Agents reported consulting injected MUST-READ resources | SubagentStop (all agents) |

**ALSO PRESERVE (additional domain validators not in the "6" list):**

| Validator | Classification | Reason |
|-----------|---------------|--------|
| `validate-silent-failures.py` | Domain (DSP safety) | JUCE 8 runtime failure pattern detection |
| `validate-foundation.py` | Domain (build safety) | CMakeLists.txt and source file existence |
| `validate-research-frontmatter.py` | Domain (research quality) | PFS 10-field semantic schema |
| `contract_validator.py` | Domain (contract library) | Shared library for checksum + cross-contract |

### GSDD-04: Context Compliance — New Capability

**Current state:** gsd-plan-checker has Dimension 7 (Context Compliance) as a soft, agent-judgment-based check. It relies on the LLM to parse CONTEXT.md and compare to PLAN.md tasks.

**Target state:** A deterministic `verify context-compliance <phase>` command in gsd-tools that:
1. Reads `{phase_dir}/*-CONTEXT.md` -- extracts Decisions, Discretion, Deferred
2. Reads all `{phase_dir}/*-PLAN.md` -- extracts task names, actions, must_haves
3. Cross-references:
   - Each Decision has at least one implementing task (keyword match in task name or action)
   - No task implements a Deferred Idea (keyword match against deferred items)
   - Discretion areas have some coverage (at least one task touches the area)
4. Returns JSON: `{ compliant, decisions: [{text, covered, covering_tasks}], deferred_violations: [{idea, violating_tasks}], issues: [] }`

**Complexity:** MEDIUM -- requires CONTEXT.md section parsing (already done by plan-checker agent in markdown) and fuzzy keyword matching against task content.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual STATE.md regex parsing in every workflow | gsd-tools state commands for execution pipeline | GSD 1.18.0 (Phase 14+) | Execution pipeline is clean; peripheral workflows still manual |
| No frontmatter validation | gsd-tools frontmatter validate (presence) | GSD 1.18.0 | Structural presence covered; semantic validation still custom |
| No structural verify suite | gsd-tools verify (6 commands) | GSD 1.18.0 | Commands exist; adoption in agents/workflows is partial |
| No context compliance checking | gsd-plan-checker Dimension 7 (soft check) | Phase 9 | Agent-judgment only; no deterministic enforcement |

## Open Questions

1. **Quick Tasks table in STATE.md**
   - What we know: quick.md creates a "Quick Tasks Completed" markdown table section with custom formatting. No gsd-tools command handles this.
   - What's unclear: Should we add a dedicated `state add-quick-task` command to gsd-tools, or should quick.md use the generic `state patch`?
   - Recommendation: Add a dedicated command -- the table format is structured enough to warrant it, and it prevents format drift.

2. **STATE.md reconstruction in resume-project.md**
   - What we know: When STATE.md is missing, resume-project.md manually reads PROJECT.md, ROADMAP.md, SUMMARYs, and todos to reconstruct.
   - What's unclear: Is this worth codifying as a gsd-tools command, or is it rare enough to keep as agent-driven?
   - Recommendation: Keep as agent-driven. Reconstruction requires judgment (interpreting summaries, deducing current position) that a deterministic command would struggle with.

3. **Context compliance keyword matching approach**
   - What we know: Decisions like "use card layout" need to match tasks that implement card layouts. Exact string matching will miss most cases.
   - What's unclear: How fuzzy should matching be? Tokenized keyword overlap? Substring matching? LLM-based semantic matching?
   - Recommendation: Use tokenized keyword overlap with stemming. Extract nouns/verbs from decisions, match against task names and action text. This gives reasonable coverage without LLM dependency. The plan-checker's Dimension 7 (LLM-based) serves as the semantic fallback.

4. **Scope of "structural verification replacement"**
   - What we know: gsd-tools verify commands exist. Some agents use them. The requirement says "replace 6 structural validators."
   - What's unclear: Are there separate custom structural validator scripts/files to delete, or is the work purely updating callsites in agent/workflow markdown?
   - Recommendation: The work is primarily callsite migration in agent/workflow instructions. No separate structural validator Python scripts exist to delete -- the structural validation was always inline in agent prompts. Grep for inline structural checks (frontmatter parsing, task counting, file existence loops) and replace with gsd-tools verify calls.

## Sources

### Primary (HIGH confidence)
- `/Users/taylorbrook/.claude/get-shit-done/bin/gsd-tools.js` lines 1-115 (full command reference), 1088-1304 (state operations), 1992-2344 (frontmatter + verify suite) -- read source directly
- `/Users/taylorbrook/.claude/agents/gsd-executor.md` lines 326-364 -- state update pattern using gsd-tools
- `/Users/taylorbrook/.claude/agents/gsd-plan-checker.md` lines 251-292 -- Dimension 7 context compliance
- `/Users/taylorbrook/.claude/get-shit-done/workflows/execute-plan.md` lines 336-391 -- gsd-tools state usage
- `/Users/taylorbrook/.claude/get-shit-done/workflows/transition.md` lines 122-334 -- mixed gsd-tools + manual edits
- `/Users/taylorbrook/.claude/get-shit-done/workflows/quick.md` lines 152-194 -- manual STATE.md edits
- `/Users/taylorbrook/Dev/VST-development/.claude/hooks/SubagentStop.sh` -- domain validator dispatch
- `/Users/taylorbrook/Dev/VST-development/.claude/hooks/validators/` -- all PFS domain validators

### Secondary (MEDIUM confidence)
- `.planning/REQUIREMENTS.md` lines 22-25, 84-87 -- GSDD requirement definitions
- `.planning/ROADMAP.md` -- phase descriptions and dependencies

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- gsd-tools.js source code read directly, all commands verified
- Architecture: HIGH -- complete inventory of manual vs gsd-tools callsites across all agents/workflows
- Pitfalls: HIGH -- based on direct observation of current code patterns and dual validation systems
- GSDD-04 (context compliance): MEDIUM -- new capability requires design decisions about keyword matching

**Research date:** 2026-02-09
**Valid until:** 2026-03-09 (stable -- gsd-tools and PFS validators change slowly)
