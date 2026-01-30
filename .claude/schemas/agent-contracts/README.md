# Agent Contract Schemas

JSON Schema definitions for agent input/output contracts.

## Purpose

This directory contains formal JSON Schema definitions that specify:
- Required inputs for each orchestration agent
- Expected outputs from each agent
- Shared type definitions used across multiple agents
- Validation rules for contract enforcement

## Schema Naming Conventions

- Input schemas: `{agent-name}.input.json`
- Output schemas: `{agent-name}.output.json`
- Shared types: `common/{type-name}.json`

Example:
```
agent-contracts/
├── plugin-workflow.input.json
├── plugin-workflow.output.json
├── build-automation.input.json
├── build-automation.output.json
└── common/
    └── plugin-reference.json
```

## Schema Version

All schemas use JSON Schema draft 2020-12.

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://oaudio.io/schemas/agent-contracts/plugin-workflow.input.json"
}
```

## Validation Rules

Per Phase 01 CONTEXT.md decisions:

### Strictness
- **Most fields required** - Agents need predictable inputs
- **`additionalProperties: false`** - Rejects unknown fields to catch typos
- **Error messages must be actionable** - Tell user how to fix

### Example Schema Structure

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://oaudio.io/schemas/agent-contracts/plugin-workflow.input.json",
  "title": "Plugin Workflow Input",
  "description": "Input contract for plugin-workflow orchestration agent",
  "type": "object",
  "required": ["plugin_name"],
  "additionalProperties": false,
  "properties": {
    "plugin_name": {
      "type": "string",
      "description": "Name of plugin to implement",
      "pattern": "^[A-Za-z][A-Za-z0-9-_]*$"
    },
    "start_stage": {
      "type": "integer",
      "description": "Stage to begin at (1-4)",
      "minimum": 1,
      "maximum": 4,
      "default": 1
    },
    "skip_phases": {
      "type": "array",
      "description": "Phases to skip",
      "items": {
        "type": "string",
        "enum": ["discuss", "research", "verify"]
      },
      "default": []
    },
    "express_mode": {
      "type": "boolean",
      "description": "Auto-advance without menus",
      "default": false
    }
  }
}
```

## Directory Structure

```
agent-contracts/
├── README.md                          # This file
├── common/                            # Shared type definitions
│   ├── plugin-reference.json          # Plugin identifier schema
│   ├── stage-reference.json           # Stage identifier schema
│   ├── phase-reference.json           # Phase identifier schema
│   └── file-path.json                 # File path schema
├── plugin-workflow.input.json         # Main orchestrator input
├── plugin-workflow.output.json        # Main orchestrator output
├── build-automation.input.json        # Build system input
├── build-automation.output.json       # Build system output
├── plugin-ideation.input.json         # Ideation input
├── plugin-ideation.output.json        # Ideation output
├── plugin-planning.input.json         # Planning input
├── plugin-planning.output.json        # Planning output
├── plugin-testing.input.json          # Testing input
├── plugin-testing.output.json         # Testing output
├── plugin-improve.input.json          # Improvement input
├── plugin-improve.output.json         # Improvement output
├── ui-mockup.input.json               # UI mockup input
├── ui-mockup.output.json              # UI mockup output
├── plugin-lifecycle.input.json        # Lifecycle input
├── plugin-lifecycle.output.json       # Lifecycle output
├── deep-research.input.json           # Research input
└── deep-research.output.json          # Research output
```

## Adding a New Schema

1. **Create input schema:** `{agent-name}.input.json`
   - Include `$schema` and `$id` fields
   - Define all required and optional properties
   - Use `$ref` to reference common types

2. **Create output schema:** `{agent-name}.output.json`
   - Define expected output structure
   - Include success/failure variants

3. **Reference from agent's SKILL.md**
   - Add schema reference in frontmatter
   - Document validation in preconditions section

4. **Add to CHANGELOG.md**
   - Record addition with version and date

## Versioning

Per Phase 01 CONTEXT.md: Full semver (MAJOR.MINOR.PATCH)

- **MAJOR:** Breaking changes (removed fields, renamed fields, type changes)
- **MINOR:** New optional fields, expanded enums
- **PATCH:** Documentation, formatting, typo fixes

### Version in Schema

Each schema includes version in `$id`:

```json
{
  "$id": "https://oaudio.io/schemas/agent-contracts/v1.0.0/plugin-workflow.input.json"
}
```

### Changelog

Central changelog: `.claude/schemas/CHANGELOG.md`

Format:
```markdown
## [1.1.0] - 2026-01-30

### Added
- plugin-workflow.input.json: Added `dry_run` optional field

### Changed
- build-automation.input.json: `stage` field now accepts null

### Removed
- None
```

## Validation Integration

Schemas are validated at runtime by agents before execution:

```python
# Pseudo-code for schema validation in agent
def validate_input(input_data, agent_name):
    schema_path = f".claude/schemas/agent-contracts/{agent_name}.input.json"
    schema = load_json(schema_path)

    errors = jsonschema.validate(input_data, schema)
    if errors:
        return {
            "status": "failure",
            "error_type": "contract_violation",
            "errors": [format_error(e) for e in errors]
        }
    return None  # Valid
```

## Cross-References

- **Source of truth:** `.planning/phases/01-agent-contracts/01-01-AUDIT.md`
- **Agent definitions:** `.claude/skills/*/SKILL.md` and `.claude/agents/*.md`
- **Phase context:** `.planning/phases/01-agent-contracts/01-CONTEXT.md`

## Implementation Status

| Agent | Input Schema | Output Schema | Status |
|-------|--------------|---------------|--------|
| plugin-workflow | Pending | Pending | Plan 02 |
| build-automation | Pending | Pending | Plan 02 |
| plugin-ideation | Pending | Pending | Plan 02 |
| plugin-planning | Pending | Pending | Plan 02 |
| plugin-testing | Pending | Pending | Plan 02 |
| plugin-improve | Pending | Pending | Plan 02 |
| ui-mockup | Pending | Pending | Plan 02 |
| plugin-lifecycle | Pending | Pending | Plan 02 |
| deep-research | Pending | Pending | Plan 02 |
