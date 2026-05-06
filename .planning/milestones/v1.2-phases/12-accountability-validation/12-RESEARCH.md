# Phase 12: Accountability & Validation - Research

**Researched:** 2026-02-05
**Domain:** Hook architecture, subagent report schema, resource injection tracing
**Confidence:** HIGH

## Summary

Phase 12 adds accountability to the resource injection pipeline built in Phases 10-11. The system currently injects research resources into agent prompts but has no way to verify agents actually consulted those resources. This phase extends the existing subagent report JSON schema with an optional `resources_consulted` field, updates agent instructions to self-report what they consulted, and adds validation logic to the SubagentStop hook that warns (never blocks) when MUST-READ resources were injected but not reported as consulted.

The implementation surface is well-defined: one schema file, 5+ agent markdown files, and one hook script. All integration points are documented and verified. The existing SubagentStop hook already runs Python validators and writes to stderr for real-time warning visibility, establishing the exact pattern needed for accountability validation.

**Primary recommendation:** Extend the existing infrastructure with minimal additions -- schema field, agent instructions, and a lightweight Python validator script called from SubagentStop.sh. No new architecture needed.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Report format: Only list resources that were consulted -- absence from the list implies not consulted (no explicit skipped[] list needed)
- Warning behavior: Warnings must be visible in real-time during workflow execution -- user wants to see skipped-resource warnings as they happen, not buried in post-run logs
- Agent integration: Shared schema, flexible content -- all agents use the same `resources_consulted` field name and structure, but can add agent-specific context if useful
- Agent integration: Universal accountability -- ANY agent that receives injected resources should report `resources_consulted`, not just the 5 stage agents listed in ACCT-02

### Claude's Discretion
- Report entry detail level (filepath only vs filepath + relevance note) and injection metadata inclusion
- Warning surface (console-only vs dual), aggregation strategy, and message detail
- Field population timing and empty-field handling
- Compliance definition, importance tiers, and missing-field semantics
- All of the above should be decided during planning based on what's practical given the existing hook and subagent architecture

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

## Standard Stack

This phase uses no new libraries. All work modifies existing infrastructure.

### Core Files to Modify
| File | Purpose | Change Type |
|------|---------|-------------|
| `.claude/schemas/subagent-report.json` | Unified subagent report schema | Add `resources_consulted` field |
| `.claude/agents/dsp-agent.md` | DSP agent instructions | Add reporting instructions |
| `.claude/agents/gui-agent.md` | GUI agent instructions | Add reporting instructions |
| `.claude/agents/foundation-shell-agent.md` | Foundation agent instructions | Add reporting instructions |
| `.claude/agents/research-planning-agent.md` | Research agent instructions | Add reporting instructions |
| `.claude/agents/polish-agent.md` | Polish agent instructions | Add reporting instructions |
| `.claude/hooks/SubagentStop.sh` | Post-subagent validation hook | Add accountability validation |

### New Files to Create
| File | Purpose | Why |
|------|---------|-----|
| `.claude/hooks/validators/validate-resource-accountability.py` | Resource accountability validation logic | Keeps SubagentStop.sh lean; follows existing validator pattern |

### Supporting Files (Reference Only)
| File | Role in Phase 12 | How Used |
|------|-------------------|----------|
| `.claude/scripts/inject-context.py` | Produces the `<research_context>` block injected into agent prompts | Hook needs to independently discover what was injected to compare against report |
| `.claude/scripts/discover-resources.py` | Discovery engine with scoring tiers | Hook calls this to determine what resources were injected (stage + agent -> primary results) |
| `.claude/hooks/hooks.json` | Hook configuration | Already configured for SubagentStop, no changes needed |

## Architecture Patterns

### Pattern 1: Schema Extension (Backward-Compatible)
**What:** Add `resources_consulted` as an optional field in the existing `subagent-report.json` schema.
**When to use:** ACCT-01 requirement.
**Why backward-compatible:** Field is optional with no `required` constraint. Existing agents that omit it still validate. Schema already has `"additionalProperties": false` so the field MUST be explicitly added.

**Recommended schema addition:**
```json
{
  "resources_consulted": {
    "type": "array",
    "items": {
      "type": "object",
      "required": ["path"],
      "properties": {
        "path": {
          "type": "string",
          "description": "Relative path to the research document that was consulted"
        },
        "relevance": {
          "type": "string",
          "description": "Optional note on how the resource informed implementation"
        }
      },
      "additionalProperties": false
    },
    "description": "Research resources the agent actually consulted from injected context. Absence from this list means the resource was not consulted."
  }
}
```

**Discretion recommendation -- Entry detail level:** Use `path` (required) + optional `relevance` note. Filepath-only is the minimum needed for hook validation. The optional relevance note lets agents add context without burden. This balances parsability (hook only needs `path`) with flexibility (agents can explain why they used a resource).

**Discretion recommendation -- Field optionality:** Always optional. When resources were injected, agents SHOULD populate it but omitting it does not break anything. The hook treats a missing field as "agent did not report" and emits a warning if MUST-READ resources were injected. An empty array `[]` means "agent acknowledged resources but consulted none." A missing field means "agent did not implement accountability reporting." Both trigger warnings if MUST-READ resources were injected, but the warning message differs.

### Pattern 2: Agent Self-Report Instructions
**What:** Add instructions to each agent's markdown file telling it to populate `resources_consulted` in its JSON report.
**When to use:** ACCT-02 requirement (and universal accountability per CONTEXT.md).

**Recommended instruction block (add to each agent's JSON report section):**
```markdown
### Resource Accountability

If you received a `<research_context>` block in your prompt, include `resources_consulted` in your JSON report listing the resources you actually read and used:

```json
"resources_consulted": [
  {"path": "research/circuit-modeling-fundamentals.md", "relevance": "Used waveshaper algorithm from section 3"},
  {"path": "research/dsp-click-prevention-debugging.md"}
]
```

Only list resources you actually consulted. Do not list resources you ignored.
If no `<research_context>` was provided, omit this field entirely.
```

**Discretion recommendation -- Population timing:** End of execution, as part of the final JSON report assembly. Subagents produce a single report at completion. Progressive population is impractical because agents emit one JSON blob at the end, not incremental updates.

**Discretion recommendation -- Agents with no injected resources:** Omit the field entirely. If no `<research_context>` block was in the prompt, the agent has nothing to report and the hook has nothing to validate. Including an empty field adds noise.

### Pattern 3: SubagentStop Validation (Warning-Only)
**What:** After each subagent completes, the SubagentStop hook independently discovers what resources SHOULD have been injected, reads the agent's report from the transcript, and warns if MUST-READ resources were not reported as consulted.
**When to use:** ACCT-03 requirement.

**Architecture:**
```
SubagentStop.sh receives input
    ├── Extract agent_type (agent name)
    ├── Extract plugin_name
    ├── Call validate-resource-accountability.py
    │   ├── Run discover() with same (stage, agent) params
    │   ├── Filter to primary tier (MUST-READ) resources
    │   ├── Read agent transcript to find JSON report
    │   ├── Extract resources_consulted from report
    │   ├── Compare: injected MUST-READ vs reported consulted
    │   └── Return: warnings to stderr, exit 0 (never blocks)
    └── Continue to existing validation (checksums, etc.)
```

**Critical constraint:** This validation MUST be warning-only. Exit code 0 always. Warnings go to stderr for real-time visibility. The hook MUST NOT exit 2 (block) for accountability failures.

**Discretion recommendation -- Importance tiers:** Only MUST-READ (primary tier, score >= 0.75) triggers warnings. Supplementary resources (lower scores) are informational and should not generate warnings. This leverages the existing scoring tiers from discover-resources.py. Rationale: primary tier resources match BOTH stage AND role -- they are genuinely relevant. Supplementary resources may be tangential.

**Discretion recommendation -- Warning message detail:** Include filename and tier. Example: `"WARNING: dsp-agent did not report consulting MUST-READ resource: research/circuit-modeling-fundamentals.md"`. This gives enough context to understand what was skipped without verbose explanations of why it mattered. The resource path is sufficient for the user to investigate.

**Discretion recommendation -- Warning surface:** Console-only via stderr. The user explicitly wants real-time visibility during workflow execution. stderr messages appear immediately in the terminal. Dual-surface (also in verification reports) adds complexity without clear benefit since the warnings appear during the run when action can be taken. Verification reports are post-hoc and don't change behavior.

**Discretion recommendation -- Per-agent independent warnings:** Yes, emit warnings per-agent as each SubagentStop fires. This is how the hook naturally works -- it fires once per subagent. No accumulation or summary needed. Each warning is self-contained with the agent name and resource path.

### Pattern 4: Resource Discovery in the Hook
**What:** The hook needs to independently determine what resources were injected to compare against the agent's report.
**When to use:** ACCT-03 implementation.

**Two approaches evaluated:**

**Option A (Recommended): Re-run discovery in the hook**
- Call `discover()` from discover-resources.py with the same (stage, agent_role) parameters
- Filter to primary tier results
- Compare paths against agent's `resources_consulted`
- Pro: Uses the single source of truth (the discovery engine itself)
- Pro: No additional data needs to flow through the pipeline
- Con: Requires mapping agent_type to stage number in the validator
- Performance: discover() is fast (~50ms on 23-doc corpus)

**Option B (Rejected): Injection metadata in report**
- Have inject-context.py record what it injected, pass metadata to the hook
- Con: Requires plumbing injection metadata through the orchestrator -> agent -> report chain
- Con: Adds coupling between inject-context.py and the validation hook
- Con: The hook can't trust metadata from the agent (the agent could fabricate it)

**Agent-to-stage mapping** (needed for the hook to call discover()):
```python
AGENT_STAGE_MAP = {
    "foundation-shell-agent": 1,
    "dsp-agent": 2,
    "gui-agent": 3,
    "research-planning-agent": 0,
    "polish-agent": 4,
}
```
This mirrors the `domain_stage_map` already in improve-milestone SKILL.md (decision 11-02).

### Pattern 5: Transcript Parsing for Report Extraction
**What:** The SubagentStop hook receives `agent_transcript_path` which contains the subagent's conversation. The validator needs to extract the JSON report from this transcript.
**When to use:** Reading what the agent actually reported.

**Approach:** Parse the JSONL transcript file, find the last assistant message containing the JSON report, extract `resources_consulted` from it. The transcript is a JSONL file where each line is a conversation turn.

**Alternative:** The existing SubagentStop.sh reads the report via `jq` from the hook input. But the hook input only contains `agent_type`, `agent_id`, `agent_transcript_path`, and common fields -- NOT the report content directly. The report is in the transcript.

**Important discovery:** The existing SubagentStop.sh uses `$INPUT` which comes from stdin. It extracts `.subagent_name` and `.plugin_name` from this. But the official Claude Code docs show the input contains `agent_type` (not `subagent_name`) and does NOT contain `plugin_name`. The existing hook works because these fields may have been added at some earlier version or the documentation and actual implementation differ. The Phase 12 validator should try both field names for robustness.

**Discretion recommendation -- Definition of "consulted":** Self-report only. The agent lists what it read. We do not attempt to verify this against the transcript (checking if the agent actually used Read tool on the resource paths). Rationale: (1) Agents receive resource CONTENT inline in the prompt, not file paths to read -- so there would be no Read tool call to verify. (2) Self-report is practical and honest -- if an agent claims it consulted a resource, that claim is useful even if unverifiable. (3) Evidence-based checking would require parsing the full transcript for content references, which is complex and fragile.

### Anti-Patterns to Avoid
- **Blocking on accountability failures:** The v1.2 roadmap explicitly says "warning only, never blocks workflow." Exit code 2 MUST NOT be used for accountability.
- **Requiring explicit skipped[] list:** CONTEXT.md decision says absence implies not consulted. No need for agents to enumerate what they ignored.
- **Complex evidence verification:** Don't try to prove agents read resources by scanning their tool calls. Content is injected inline.
- **Modifying inject-context.py:** The injection utility should remain unchanged. Accountability is a validation-side concern.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Resource discovery | Custom discovery in the hook | `discover-resources.py` discover() function | Already handles scoring, tiers, filtering. Single source of truth |
| Agent-to-role mapping | Custom mapping in validator | `discover-resources.py` AGENT_ROLE_MAP | Already maps all 11 agent names to 4 roles |
| JSON schema validation | Manual field checking | jsonschema library (already a dependency) | Used by discover-resources.py, consistent validation |
| Transcript parsing | Custom JSONL parser | Python json module line-by-line | JSONL is one JSON object per line, trivial to parse |

## Common Pitfalls

### Pitfall 1: Hook Timeout
**What goes wrong:** The SubagentStop hook has a 10-second timeout (configured in hooks.json). If the accountability validator takes too long, it gets killed.
**Why it happens:** Adding a Python subprocess call adds latency. Discover() + transcript parsing + comparison could approach the timeout if the corpus grows.
**How to avoid:** discover() runs in ~50ms on 23 docs. Transcript parsing is I/O-bound but fast for typical subagent transcripts (< 1MB). Keep the validator focused. No WebSearch or network calls.
**Warning signs:** Hook timeout errors in debug mode.

### Pitfall 2: Missing plugin_name Breaks Discovery
**What goes wrong:** discover() accepts a `plugin_name` parameter (currently unused but reserved). More critically, the hook needs to extract the agent type correctly. If `subagent_name` / `agent_type` extraction fails, the validator cannot determine what was injected.
**Why it happens:** The hook input field names may differ between Claude Code versions.
**How to avoid:** Try both `agent_type` and `subagent_name` fields. Use `// empty` fallback in jq. Skip validation gracefully if agent type cannot be determined.
**Warning signs:** "Hook not relevant: no subagent_name" messages for agents that should be validated.

### Pitfall 3: Schema `additionalProperties: false` Blocks New Fields
**What goes wrong:** The existing schema has `"additionalProperties": false` at the top level AND inside `outputs`. Adding `resources_consulted` at the wrong level will cause existing reports to fail validation.
**Why it happens:** JSON Schema strict mode.
**How to avoid:** Add `resources_consulted` as a top-level property in the schema (alongside `agent`, `status`, `outputs`, `issues`, etc.), NOT inside `outputs`. The field is about the agent's process, not its stage-specific outputs.
**Warning signs:** Schema validation errors on reports that previously passed.

### Pitfall 4: Stage Pattern Files vs Research Resources
**What goes wrong:** inject-context.py injects BOTH stage pattern files (stage-1-patterns.md etc.) AND discovered research resources. The accountability check should only validate against discovered research resources (MUST-READ tier), not the auto-injected stage patterns.
**Why it happens:** Stage patterns are always injected for stages 1-3 regardless of discovery scores. They are not in the resource-index.json manifest.
**How to avoid:** The validator should only call discover() and check primary-tier results. Stage patterns are not in the manifest and will not appear in discover() results.
**Warning signs:** False warnings about stage pattern files not being consulted.

### Pitfall 5: improve-milestone Uses Different Stage Mapping
**What goes wrong:** improve-milestone maps execute agents to stages differently (dsp-agent -> 2, gui-agent -> 3, polish-agent -> 4, general-purpose -> 0). If the hook doesn't know the invocation context, it might use the wrong stage for discovery.
**Why it happens:** The same agent can be invoked from different workflows at different stages.
**How to avoid:** The hook's agent-to-stage mapping should use the canonical stage for each agent (which matches both plugin-workflow and improve-milestone mappings). The mapping is deterministic: dsp-agent is always stage 2, gui-agent is always stage 3, etc.
**Warning signs:** Discovery returning different results than what was actually injected.

## Code Examples

### Example 1: Schema Addition
```json
// In .claude/schemas/subagent-report.json, add to "properties" object:
"resources_consulted": {
  "type": "array",
  "items": {
    "type": "object",
    "required": ["path"],
    "properties": {
      "path": {
        "type": "string",
        "description": "Relative path to the research document that was consulted"
      },
      "relevance": {
        "type": "string",
        "description": "Optional note on how the resource informed implementation"
      }
    },
    "additionalProperties": false
  },
  "description": "Research resources the agent actually consulted from injected context"
}
```

### Example 2: Agent Report with Accountability
```json
{
  "agent": "dsp-agent",
  "status": "success",
  "outputs": {
    "plugin_name": "O-FreqPulse",
    "dsp_components": ["juce::dsp::StateVariableTPTFilter<float>"],
    "processing_chain": "Input -> Filter -> Gain -> Output"
  },
  "resources_consulted": [
    {"path": "research/circuit-modeling-fundamentals.md", "relevance": "Used waveshaper algorithm for saturation stage"},
    {"path": "research/dsp-click-prevention-debugging.md"}
  ],
  "issues": [],
  "ready_for_next_stage": true,
  "stateUpdated": true
}
```

### Example 3: SubagentStop.sh Addition
```bash
# Add after existing case block, before final exit 0:

# Accountability validation (warning-only, NEVER blocks)
if [[ "$SUBAGENT" =~ ^(foundation-shell-agent|dsp-agent|gui-agent|research-planning-agent|polish-agent)$ ]]; then
  python3 .claude/hooks/validators/validate-resource-accountability.py \
    "$SUBAGENT" "$PLUGIN_NAME" 2>&1 | while read -r line; do
    echo "$line" >&2  # Ensure warnings go to stderr for real-time visibility
  done
  # Intentionally ignore exit code -- accountability never blocks
fi
```

### Example 4: Validator Script Core Logic (Python)
```python
#!/usr/bin/env python3
"""
Resource accountability validator.
Compares injected MUST-READ resources against agent's resources_consulted report.
WARNING-ONLY: Never exits non-zero.
"""
import json
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
CLAUDE_DIR = SCRIPT_DIR.parent.parent  # .claude/hooks/validators -> .claude
SCRIPTS_DIR = CLAUDE_DIR / "scripts"

# Import discover-resources.py
import importlib.util
spec = importlib.util.spec_from_file_location("discover_resources", SCRIPTS_DIR / "discover-resources.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
discover = mod.discover

AGENT_STAGE_MAP = {
    "foundation-shell-agent": 1,
    "dsp-agent": 2,
    "gui-agent": 3,
    "research-planning-agent": 0,
    "polish-agent": 4,
}

def validate(agent_type, plugin_name=None):
    stage = AGENT_STAGE_MAP.get(agent_type)
    if stage is None:
        return  # Unknown agent, skip

    # Discover what SHOULD have been injected
    results = discover(stage=stage, agent_role=agent_type, plugin_name=plugin_name)
    must_read = [r for r in results if r["tier"] == "primary"]

    if not must_read:
        return  # No MUST-READ resources, nothing to validate

    # Read the agent transcript to find resources_consulted
    # (transcript path comes from stdin in actual hook; here simplified)
    # ... parse transcript for JSON report ...
    # ... extract resources_consulted ...

    consulted_paths = set()  # populated from report parsing
    must_read_paths = {r["path"] for r in must_read}

    skipped = must_read_paths - consulted_paths
    for path in sorted(skipped):
        print(f"WARNING: {agent_type} did not report consulting MUST-READ resource: {path}", file=sys.stderr)

if __name__ == "__main__":
    agent_type = sys.argv[1] if len(sys.argv) > 1 else None
    plugin_name = sys.argv[2] if len(sys.argv) > 2 else None
    if agent_type:
        validate(agent_type, plugin_name)
    sys.exit(0)  # ALWAYS exit 0 -- never block
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| No resource tracking | Phase 10-11: Resources injected into prompts | 2026-02-05 | Agents receive relevant research but usage unverified |
| Required Reading (manual) | inject-context.py (automatic) | Phase 11 | Stage patterns and research auto-injected inline |

**After Phase 12:**
| Current | After Phase 12 | Impact |
|---------|---------------|--------|
| No accountability for injected resources | Self-report + hook warning | Visibility into whether agents use injected knowledge |
| Resources injected silently | Warnings when MUST-READ resources skipped | Real-time feedback on resource utilization |

## Open Questions

1. **Transcript format reliability**
   - What we know: SubagentStop receives `agent_transcript_path` pointing to a JSONL file. The agent's JSON report should be in the last assistant message.
   - What's unclear: Exact JSONL structure for the subagent transcript. Need to verify during implementation by running a test workflow and inspecting the transcript file.
   - Recommendation: Build the validator to gracefully handle transcript parsing failures (warn about inability to validate, never crash).

2. **Field name in hook input: `subagent_name` vs `agent_type`**
   - What we know: Existing SubagentStop.sh uses `.subagent_name` successfully. Official Claude Code docs show `agent_type`. Both may be present.
   - What's unclear: Whether `.subagent_name` was renamed to `agent_type` in a version update, or both coexist.
   - Recommendation: Try `agent_type` first, fall back to `subagent_name`. The existing hook already works with `subagent_name`, so don't break that.

3. **Universal accountability scope**
   - What we know: User wants ANY agent receiving resources to report, not just the 5 named agents. Currently 5 main stage agents + improve-milestone domain agents receive injection.
   - What's unclear: Whether other agents (troubleshoot-agent, music-theory-agent, aesthetics-agent, validation-agent, ui-design-agent, ui-finalization-agent) will ever receive injection.
   - Recommendation: Update all 11 agent files listed in `AGENT_ROLE_MAP` with the accountability instructions. For the hook, validate any agent that appears in `AGENT_STAGE_MAP`. This makes the system ready for future expansion without blocking on non-receiving agents.

## Sources

### Primary (HIGH confidence)
- `.claude/schemas/subagent-report.json` -- Current schema structure, additionalProperties constraint
- `.claude/hooks/SubagentStop.sh` -- Current hook architecture, validator pattern, stderr for warnings
- `.claude/hooks/hooks.json` -- Hook configuration, 10s timeout
- `.claude/scripts/inject-context.py` -- Injection utility, MUST-READ tier labeling
- `.claude/scripts/discover-resources.py` -- Discovery engine, AGENT_ROLE_MAP, tier thresholds
- `.claude/agents/*.md` -- All 11 agent files, JSON report formats
- `.planning/phases/11-context-injection-pipeline/11-02-SUMMARY.md` -- Phase 11 readiness for Phase 12
- `.planning/ROADMAP.md` -- Phase 12 success criteria and requirements

### Secondary (MEDIUM confidence)
- [Claude Code hooks reference](https://code.claude.com/docs/en/hooks) -- SubagentStop input schema (agent_type, agent_transcript_path)

### Tertiary (LOW confidence)
- SubagentStop transcript JSONL format -- Need to verify during implementation by inspecting an actual transcript file

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all files are in the codebase and verified
- Architecture: HIGH -- follows established patterns (validators, stderr warnings, discover() reuse)
- Pitfalls: HIGH -- identified from direct code inspection of existing hooks and scripts
- Transcript parsing: MEDIUM -- official docs confirm `agent_transcript_path` exists but JSONL structure needs runtime verification

**Research date:** 2026-02-05
**Valid until:** 2026-03-05 (stable infrastructure, low churn rate)
