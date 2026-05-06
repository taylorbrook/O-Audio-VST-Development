---
phase: 14-full-system-review
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .planning/quick/14-full-system-review-of-plugin-freedom-sys/SYSTEM-REVIEW.md
autonomous: true
requirements: [REVIEW-01]

must_haves:
  truths:
    - "Every subsystem of .claude/ is analyzed for efficiency, quality, and context cost"
    - "Concrete prioritized recommendations exist with effort/impact ratings"
    - "Context window impact is quantified for each major subsystem"
  artifacts:
    - path: ".planning/quick/14-full-system-review-of-plugin-freedom-sys/SYSTEM-REVIEW.md"
      provides: "Comprehensive system review with prioritized recommendations"
      min_lines: 300
  key_links: []
---

<objective>
Perform a full system review of the Plugin Freedom System (.claude/ directory and supporting infrastructure) to identify efficiency improvements, quality gaps, redundancies, and context window optimization opportunities.

Purpose: The PFS has grown organically across 4 milestones (v1.0-v1.3, 17 phases, 51 plans). It needs a holistic audit to find what's working, what's wasteful, and what should change before v2 planning begins.

Output: A single comprehensive SYSTEM-REVIEW.md with findings organized by subsystem and prioritized recommendations.
</objective>

<execution_context>
@/Users/taylorbrook/.claude/get-shit-done/workflows/execute-plan.md
@/Users/taylorbrook/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@.claude/settings.json
@.claude/hooks/hooks.json
@.claude/agent-profiles.json
@.claude/preferences.json
@.claude/system-config.json
</context>

<tasks>

<task type="auto">
  <name>Task 1: Parallel deep investigation of all PFS subsystems</name>
  <files>.planning/quick/14-full-system-review-of-plugin-freedom-sys/SYSTEM-REVIEW.md</files>
  <action>
Launch parallel agent teams to investigate each major subsystem of the Plugin Freedom System simultaneously. Each agent should read ALL files in its assigned area, analyze them deeply, and return structured findings.

**Agent Team 1 — Hooks and Validators** (investigate .claude/hooks/)
Read every file: SessionStart.py, PostToolUse.py, PreCompact.py, Stop.py, SubagentStop.py, UserPromptSubmit.py, inject-agent-memory.py, task-validator-dispatch.py, detect-research-conflicts.py, merge-critic-reports.py, regenerate-manifest.py, PostCompact-SessionStart.py, hooks.json, and all validators/*.py files.
Analyze:
- Which hooks actually fire? Compare hooks.json (the PFS config) against settings.json (what Claude Code actually loads). Identify any hooks defined in hooks.json but NOT registered in settings.json (dead hooks).
- Note: settings.json uses the official Claude Code hook format. hooks.json appears to be a PFS-internal config. Determine which one is authoritative.
- For each active hook: what does it do, how much latency does it add (check timeout values), what value does it provide?
- Are there .sh versions alongside .py versions? If so, are the .sh versions dead code from a migration?
- Validators: which ones run, when, and what value do they add? Are any redundant?
- Context injection cost: does UserPromptSubmit.py inject content into every prompt? How much?

**Agent Team 2 — Agents and Agent Memory** (investigate .claude/agents/ and .claude/agent-memory/)
Read all 14 agent definition files and all 5 agent memory files.
Analyze:
- Total token cost: the agents/ directory is 332K (9,931 lines). That is massive context. How much of each agent definition is boilerplate vs. unique domain knowledge?
- Which agents are actually used in current workflows? Cross-reference with commands/*.md to find which commands spawn which agents.
- Are there redundant agents? (e.g., gui-agent vs ui-design-agent vs ui-finalization-agent — three UI agents?)
- Agent memory files: are they useful? Are they stale? Do they get injected (check inject-agent-memory.py)?
- Agent profiles (agent-profiles.json): is this aspirational documentation or does it drive behavior?

**Agent Team 3 — Skills** (investigate .claude/skills/)
Read ALL 27 SKILL.md files (the lightweight indexes).
Analyze:
- Total skill content: 1.8M, 44,792 lines across 202 .md files. This is the largest subsystem. Quantify context cost.
- Which skills are loaded and when? How does the skill loading mechanism work?
- Are there redundant or overlapping skills? (e.g., plugin-lifecycle vs plugin-phases vs plugin-workflow — three similar names)
- Which skills have assets/ and references/ subdirectories? Are those loaded on demand or always?
- Are any skills unused or obsolete after v1.3?

**Agent Team 4 — Commands** (investigate .claude/commands/)
Read all command .md files (6,569 total lines across ~40 commands).
Analyze:
- Which commands are actively used vs. vestigial?
- How much context does each command consume when invoked? (measure line count)
- Are there commands that could be consolidated?
- Do commands properly reference skills, or do they inline skill content (context waste)?
- Check for the /plugin-* family: discuss, research, plan, execute, verify, handoff, critique, resume, pause, focus, status, list — are all needed?

**Agent Team 5 — Research Documents, Templates, Schemas, and Supporting Infrastructure**
Read: resource-index.json (structure), schemas/README.md, templates/README.md, BRANDING.md, critics/*.md, references/handoff-protocol.md.
Sample 3-4 research docs from research/ for quality and format consistency.
Analyze:
- Research docs: 49 files, 48,754 lines. Are they well-indexed? Is resource-index.json accurate and current? Do research docs have consistent frontmatter?
- Templates: code-snippets (cmake, dsp, parameter-binding, webview) and prose-patterns (architecture, dsp, ui). Are these used? Are they current?
- Schemas: are they enforced or aspirational?
- Critics: 4 critic definitions (architecture, dsp, foundation, ui). Are they used? When?
- Aesthetics: 5 theme definitions. Are they loaded efficiently?
- Cache (validation-results.json): is caching working?

**After all agents return**, synthesize into SYSTEM-REVIEW.md with this structure:

```
# Plugin Freedom System — Full System Review
## Date: 2026-03-05

## Executive Summary
[3-5 bullet overview of findings]

## System Inventory
[Table: subsystem, file count, total lines, disk size, estimated token cost]

## Subsystem Analysis

### 1. Hooks and Validators
#### Current State
#### Findings
#### Issues

### 2. Agents and Agent Memory
#### Current State
#### Findings
#### Issues

### 3. Skills
#### Current State
#### Findings
#### Issues

### 4. Commands
#### Current State
#### Findings
#### Issues

### 5. Research Documents
#### Current State
#### Findings
#### Issues

### 6. Templates, Schemas, and Supporting Infrastructure
#### Current State
#### Findings
#### Issues

## Context Window Analysis
[Quantify: what gets loaded per session, per command invocation, per agent spawn]
[Identify: the top 5 context hogs]

## Redundancy Map
[Table: overlapping components with recommendation to merge/remove/keep]

## Prioritized Recommendations
[Numbered list, each with: Description, Impact (high/medium/low), Effort (high/medium/low), Category (efficiency/quality/cleanup)]
[Sort by impact DESC, effort ASC]

## Quick Wins (< 30 min each)
[Things that can be fixed immediately]

## Architecture Observations
[Higher-level patterns: what's working well, what's fighting against Claude Code's design, what should change structurally]
```

IMPORTANT: Be specific and quantitative. Every finding should reference actual file paths, line counts, or token estimates. Avoid vague statements like "could be improved" — say exactly what should change and why.
  </action>
  <verify>
    <automated>test -f /Users/taylorbrook/Dev/VST-development/.planning/quick/14-full-system-review-of-plugin-freedom-sys/SYSTEM-REVIEW.md && wc -l /Users/taylorbrook/Dev/VST-development/.planning/quick/14-full-system-review-of-plugin-freedom-sys/SYSTEM-REVIEW.md | awk '{if ($1 >= 300) print "PASS: " $1 " lines"; else print "FAIL: only " $1 " lines"}'</automated>
  </verify>
  <done>SYSTEM-REVIEW.md exists with 300+ lines covering all 6 subsystem areas, context window analysis, redundancy map, and prioritized recommendations with impact/effort ratings</done>
</task>

<task type="auto">
  <name>Task 2: Commit review document</name>
  <files>.planning/quick/14-full-system-review-of-plugin-freedom-sys/SYSTEM-REVIEW.md</files>
  <action>
Stage and commit the SYSTEM-REVIEW.md file with a descriptive commit message.

```bash
git add .planning/quick/14-full-system-review-of-plugin-freedom-sys/SYSTEM-REVIEW.md
git commit -m "docs: full system review of Plugin Freedom System

Comprehensive audit of all PFS subsystems: hooks, agents, skills,
commands, research docs, templates, schemas. Includes context window
analysis, redundancy map, and prioritized recommendations."
```
  </action>
  <verify>
    <automated>git log --oneline -1 | grep -q "system review" && echo "PASS" || echo "FAIL"</automated>
  </verify>
  <done>Review document committed to git</done>
</task>

</tasks>

<verification>
- SYSTEM-REVIEW.md covers all 6 subsystem areas (hooks, agents, skills, commands, research, infrastructure)
- Context window analysis section quantifies token costs
- Recommendations are prioritized with impact/effort ratings
- Document committed to git
</verification>

<success_criteria>
- Comprehensive review document exists at .planning/quick/14-full-system-review-of-plugin-freedom-sys/SYSTEM-REVIEW.md
- All major PFS subsystems analyzed with specific findings (not vague observations)
- At least 10 prioritized recommendations with impact/effort ratings
- Context window costs quantified for major subsystems
- Review committed to repository
</success_criteria>

<output>
After completion, create `.planning/quick/14-full-system-review-of-plugin-freedom-sys/14-SUMMARY.md`
</output>
