# Phase 1: Agent Contracts - Research

**Researched:** 2026-01-30
**Domain:** Agent contract design, JSON Schema validation, scope documentation
**Confidence:** HIGH

## Summary

Phase 1 focuses on adding formal input/output contracts to the 9 existing agents in the Plugin Freedom System. The research investigated JSON Schema standards, agent contract design patterns, validation approaches, and scope documentation best practices.

The current system already has some schema infrastructure (`.claude/schemas/` with `subagent-report.json`, `validator-report.json`, `plugin-registry.schema.json`), but input schemas and scope boundaries are largely absent. The 9 agents (plugin-ideation, plugin-planning, build-automation, plugin-testing, plugin-workflow, plugin-improve, ui-mockup, plugin-lifecycle, deep-research) have well-documented behaviors but lack formal machine-validated contracts.

The recommended approach: (1) audit existing agents to catalog actual inputs/outputs, (2) define JSON Schema draft 2020-12 schemas with strict validation, (3) implement validation that fails fast with actionable errors, (4) document scope boundaries in a standardized does/doesn't format, (5) identify gaps and overlaps through workflow tracing.

**Primary recommendation:** Use JSON Schema draft 2020-12 with `additionalProperties: false` for strict contracts, embed schemas alongside SKILL.md files, and implement validation via a lightweight validation utility that runs before agent invocation.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JSON Schema | draft 2020-12 | Schema definition | Latest stable draft, widely supported, improved over draft-07 |
| Ajv | 8.x | JavaScript validation | Fastest validator, strict mode, good error messages |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Pydantic | 2.x | Python validation | If Python tooling needed |
| Zod | 3.x | TypeScript validation | If TypeScript tooling preferred |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| JSON Schema | TypeScript interfaces | Less portable, no runtime validation |
| Separate schema files | Inline in SKILL.md | Harder to validate programmatically |
| Ajv | jsonschema (Python) | Slower, less strict mode support |

**No installation needed:** Validation is conceptual for Claude agents. Schemas are human-readable JSON files Claude can read and enforce. Machine validation optional for CI/CD.

## Architecture Patterns

### Recommended Project Structure
```
.claude/
├── schemas/
│   ├── README.md                    # Schema usage guide
│   ├── agent-contracts/             # NEW: Input/output schemas per agent
│   │   ├── plugin-ideation.input.json
│   │   ├── plugin-ideation.output.json
│   │   ├── plugin-planning.input.json
│   │   └── ... (18 files total: 9 agents x 2 schemas)
│   ├── subagent-report.json         # EXISTS: Output reports
│   ├── validator-report.json        # EXISTS: Validation reports
│   └── plugin-registry.schema.json  # EXISTS: Registry state
├── skills/
│   └── [skill-name]/
│       ├── SKILL.md                 # Existing skill docs
│       ├── BOUNDARIES.md            # NEW: Scope documentation
│       └── references/
└── utils/
    └── validate-contract.md         # NEW: Validation utility guide
```

### Pattern 1: Centralized Schema Directory
**What:** All agent input/output schemas live in `.claude/schemas/agent-contracts/`
**When to use:** Always - enables schema discovery, cross-referencing, and shared definitions
**Example:**
```json
// Source: JSON Schema draft 2020-12 specification
// .claude/schemas/agent-contracts/plugin-ideation.input.json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "plugin-ideation.input",
  "title": "Plugin Ideation Input Contract",
  "description": "Required inputs for plugin-ideation skill invocation",
  "type": "object",
  "required": ["trigger_type", "plugin_name"],
  "properties": {
    "trigger_type": {
      "type": "string",
      "enum": ["new_plugin", "improvement"],
      "description": "Whether ideating new plugin or improving existing"
    },
    "plugin_name": {
      "type": ["string", "null"],
      "description": "Plugin name if improving, null if new"
    },
    "user_description": {
      "type": "string",
      "description": "User's initial idea or improvement request"
    },
    "context_files": {
      "type": "array",
      "items": { "type": "string" },
      "description": "Optional files to load for context"
    }
  },
  "additionalProperties": false
}
```

### Pattern 2: Boundary Documentation Standard
**What:** Each agent has BOUNDARIES.md with explicit does/doesn't lists
**When to use:** For all 9 existing agents
**Example:**
```markdown
# plugin-ideation Boundaries

## This Agent DOES
- Gather creative vision through adaptive questioning
- Generate creative briefs (BRIEF.md)
- Route between new plugin and improvement modes
- Update PLUGINS.md with new entries

## This Agent DOES NOT
- Implement any code changes
- Create technical specifications (use plugin-planning)
- Modify existing plugin code (use plugin-improve)
- Run builds or tests

## Handoff Points
- Outputs to: plugin-planning (via BRIEF.md)
- Receives from: User commands, plugin-improve (for brainstorming)

## Overlap Clarification
- **vs plugin-improve**: ideation explores ideas, improve implements them
- **vs plugin-planning**: ideation captures vision, planning creates specs
```

### Pattern 3: Fail-Fast Validation
**What:** Validation runs at skill entry, rejects invalid inputs with actionable errors
**When to use:** Every agent invocation
**Example:**
```markdown
## Contract Validation Protocol

At skill entry, before any processing:

1. Load input schema: `.claude/schemas/agent-contracts/{skill-name}.input.json`
2. Validate invocation context against schema
3. If invalid:
   - Extract which field failed
   - Explain what was expected vs received
   - Suggest fix (don't guess)

Error format:
```
CONTRACT VIOLATION: {skill-name} input invalid

Field: {field_path}
Expected: {schema_expectation}
Received: {actual_value}

Fix: {actionable_instruction}
```
```

### Anti-Patterns to Avoid
- **Implicit contracts:** Don't assume inputs exist; validate explicitly
- **Over-broad schemas:** Avoid `additionalProperties: true` - catches typos
- **Silent failures:** Never proceed with invalid inputs; always fail visibly
- **Enum without reason:** Document why each enum value exists
- **Deep nesting:** Keep schemas flat where possible; use $ref for shared types

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Schema definition | Custom format | JSON Schema draft 2020-12 | Industry standard, tooling support, portable |
| Error messages | Generic "invalid input" | Schema path + expected + received | Users need actionable errors |
| Schema versioning | Ad-hoc version fields | $id with semver in path | Standard approach, tools understand it |
| Shared types | Copy-paste definitions | $ref to common schemas | DRY, single source of truth |
| Enum documentation | Inline comments | description field | Machine-readable, visible in tools |

**Key insight:** JSON Schema is mature enough that reinventing it creates maintenance burden. The existing schemas in `.claude/schemas/` follow draft-07; upgrading to 2020-12 provides better `prefixItems`, `unevaluatedProperties`, and vocabulary support.

## Common Pitfalls

### Pitfall 1: Overly Strict Initial Schemas
**What goes wrong:** Schemas break existing workflows because they enforce constraints that weren't previously explicit
**Why it happens:** Formalizing implicit contracts reveals hidden flexibility
**How to avoid:** Audit actual usage before defining schemas; start permissive, tighten after testing
**Warning signs:** Multiple agents break simultaneously when schemas added

### Pitfall 2: Missing Optional Field Handling
**What goes wrong:** Schemas require fields that are legitimately optional in some invocation paths
**Why it happens:** Confusing "always present" with "always required"
**How to avoid:** Mark truly optional fields; use `oneOf` for conditional requirements
**Warning signs:** Workarounds like passing empty strings or nulls

### Pitfall 3: Scope Creep Through Undefined Boundaries
**What goes wrong:** Agents gradually accumulate responsibilities outside their design
**Why it happens:** No explicit "doesn't do" list; path of least resistance
**How to avoid:** BOUNDARIES.md with exclusions updated when confusion surfaces
**Warning signs:** Same functionality implemented in multiple agents

### Pitfall 4: Breaking Changes Without Versioning
**What goes wrong:** Schema updates break downstream consumers
**Why it happens:** Semver not enforced; changes seem minor
**How to avoid:** MAJOR bump for removed/renamed fields; MINOR for new optional fields
**Warning signs:** Complaints about "it used to work"

### Pitfall 5: Validation Without Actionable Errors
**What goes wrong:** Users see "validation failed" with no guidance
**Why it happens:** Using raw validator output instead of contextualizing
**How to avoid:** Transform errors into: what failed, what was expected, how to fix
**Warning signs:** Users ask "what does this error mean?"

## Code Examples

Verified patterns from official sources:

### JSON Schema 2020-12 Input Contract
```json
// Source: https://json-schema.org/draft/2020-12
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://plugin-freedom-system/schemas/agent-contracts/build-automation.input.json",
  "title": "Build Automation Input Contract",
  "description": "Required inputs for build-automation skill",
  "type": "object",
  "required": ["plugin_name", "invoker"],
  "properties": {
    "plugin_name": {
      "type": "string",
      "minLength": 1,
      "description": "Name of plugin to build (must exist in plugins/)"
    },
    "invoker": {
      "type": "string",
      "enum": ["plugin-workflow", "plugin-improve", "plugin-lifecycle", "manual"],
      "description": "Which skill/command invoked this build"
    },
    "stage": {
      "type": ["integer", "null"],
      "minimum": 0,
      "maximum": 5,
      "description": "Current workflow stage (null for manual builds)"
    },
    "build_flags": {
      "type": "array",
      "items": {
        "type": "string",
        "enum": ["--no-install", "--dry-run"]
      },
      "default": [],
      "description": "Optional build flags"
    }
  },
  "additionalProperties": false
}
```

### Output Contract with Conditional Fields
```json
// Source: JSON Schema 2020-12 conditional schemas
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "build-automation.output",
  "title": "Build Automation Output Contract",
  "type": "object",
  "required": ["status", "build_time_seconds"],
  "properties": {
    "status": {
      "type": "string",
      "enum": ["success", "failure"]
    },
    "build_time_seconds": {
      "type": "number",
      "minimum": 0
    },
    "log_path": {
      "type": "string",
      "description": "Path to build log"
    },
    "installed_paths": {
      "type": "object",
      "properties": {
        "vst3": { "type": "string" },
        "au": { "type": "string" }
      }
    },
    "error": {
      "type": "object",
      "properties": {
        "type": { "type": "string" },
        "message": { "type": "string" },
        "file": { "type": "string" },
        "line": { "type": "integer" }
      }
    }
  },
  "if": {
    "properties": { "status": { "const": "failure" } }
  },
  "then": {
    "required": ["error"]
  },
  "additionalProperties": false
}
```

### Boundary Documentation Template
```markdown
// Source: Multi-agent design pattern best practices
# {Agent Name} Boundaries

**Version:** 1.0.0
**Last Updated:** {date}

## Purpose
{One-sentence description of this agent's core responsibility}

## This Agent DOES
- {Responsibility 1}
- {Responsibility 2}
- {Responsibility 3}

## This Agent DOES NOT
- {Exclusion 1} (→ use {other-agent} instead)
- {Exclusion 2} (→ use {other-agent} instead)
- {Exclusion 3}

## Input Requirements
See: `.claude/schemas/agent-contracts/{agent}.input.json`

## Output Guarantees
See: `.claude/schemas/agent-contracts/{agent}.output.json`

## Handoff Points
| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | {agent} | {artifact} | {when} |
| Outputs to | {agent} | {artifact} | {when} |

## Tool Inventory
Maximum 20 tools per agent (enforced by design, not tooling):
1. {Tool 1} - {purpose}
2. {Tool 2} - {purpose}
...

## Overlap Resolution
| Similar Agent | How to Decide |
|---------------|---------------|
| {agent} | {decision criteria} |
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| JSON Schema draft-07 | JSON Schema draft 2020-12 | 2020 | Better prefixItems, unevaluatedProperties |
| Implicit agent contracts | Explicit JSON Schema | Industry trend 2024-2026 | Required for multi-agent reliability |
| Error codes | Structured error objects | Industry standard | Actionable debugging |
| Global tool access | Scoped tool inventories | Multi-agent patterns 2025 | Least-privilege, security |

**Current project status:**
- Using draft-07 for existing schemas
- No input schemas defined
- No formal boundary documentation
- Some output schemas exist (subagent-report, validator-report)

## Open Questions

Things that couldn't be fully resolved:

1. **Contract file location preference**
   - What we know: Centralized (.claude/schemas/) vs distributed (alongside SKILL.md)
   - What's unclear: User preference not stated in CONTEXT.md (left to discretion)
   - Recommendation: Use centralized `.claude/schemas/agent-contracts/` for discoverability; reference from SKILL.md

2. **Validation enforcement mechanism**
   - What we know: Claude can read schemas and validate conceptually
   - What's unclear: Whether machine validation (CI) is desired
   - Recommendation: Start with Claude-enforced validation; add CI check later if needed

3. **Exact agent count**
   - What we know: Requirements mention "9 agents" but skills directory shows 20 SKILL.md files
   - What's unclear: Which 9 are the "core" agents vs supporting skills
   - Recommendation: Audit all 20, identify 9 core orchestration agents, document the distinction

## Sources

### Primary (HIGH confidence)
- [JSON Schema draft 2020-12 specification](https://json-schema.org/draft/2020-12) - Schema definition standards
- [Ajv documentation](https://ajv.js.org/strict-mode.html) - Strict mode, additionalProperties behavior
- Existing `.claude/schemas/` files - Current project patterns
- Existing SKILL.md files (9 read) - Current agent documentation

### Secondary (MEDIUM confidence)
- [Google Multi-Agent Design Patterns](https://www.infoq.com/news/2026/01/multi-agent-design-patterns/) - Generator-critic pattern
- [Agentic AI Design Patterns 2026](https://medium.com/@dewasheesh.rana/agentic-ai-design-patterns-2026-ed-e3a5125162c5) - Input/output validation best practices
- [2026 Playbook for Reliable Agentic Workflows](https://promptengineering.org/agents-at-work-the-2026-playbook-for-building-reliable-agentic-workflows/) - Validation coverage guidance
- [Claude Structured Outputs documentation](https://platform.claude.com/docs/en/build-with-claude/structured-outputs) - Claude-native validation

### Tertiary (LOW confidence)
- Web search results on LLM orchestration patterns - General industry trends

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - JSON Schema is well-documented, Ajv is battle-tested
- Architecture patterns: HIGH - Based on existing project structure + industry patterns
- Pitfalls: MEDIUM - Derived from multi-agent literature + general schema experience
- Gap analysis approach: HIGH - Workflow tracing is documented best practice

**Research date:** 2026-01-30
**Valid until:** 2026-03-01 (60 days - JSON Schema is stable)
