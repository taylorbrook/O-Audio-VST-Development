---
name: contract-validation
description: Protocol for validating agent input/output contracts. Read at agent entry to enforce contracts. Not invoked directly - embedded in agent startup sequence.
---

# Contract Validation Protocol

## Purpose

Enforce agent contracts by validating inputs at entry and outputs at exit. Per CONTEXT.md decisions: fail fast with actionable errors.

This is a **protocol skill** - it documents how Claude should validate contracts, not a runtime validator. Claude reads this at agent entry to know how to validate.

## When to Validate

**Input Validation:**
- At agent entry, before any processing
- Load: `.claude/schemas/agent-contracts/{agent-name}.input.json`
- Validate invocation context against schema

**Output Validation:**
- Before agent exit, after processing complete
- Load: `.claude/schemas/agent-contracts/{agent-name}.output.json`
- Validate return value against schema

## Validation Process

### Step 1: Load Schema

```
schema_path = ".claude/schemas/agent-contracts/{agent-name}.{input|output}.json"
schema = read_json(schema_path)
```

### Step 2: Validate Against Schema

For each required field:
- Check field exists
- Check type matches
- Check constraints (enum, pattern, minLength, etc.)

For additionalProperties: false:
- Reject any fields not in schema

### Step 3: On Failure - Produce Actionable Error

Error format (MUST follow exactly):

```
CONTRACT VIOLATION: {agent-name} {input|output} invalid

Field: {json_path_to_field}
Expected: {schema_expectation}
Received: {actual_value_or_type}

Fix: {specific_instruction_to_resolve}
```

### Error Format Examples

**Example 1: Pattern mismatch**

```
CONTRACT VIOLATION: plugin-workflow input invalid

Field: plugin_name
Expected: string matching pattern ^[A-Z][a-zA-Z0-9-]*$
Received: "my-plugin" (lowercase start)

Fix: Capitalize plugin name: "My-plugin" or "MyPlugin"
```

**Example 2: Invalid enum value**

```
CONTRACT VIOLATION: build-automation input invalid

Field: invoker
Expected: one of ["plugin-workflow", "plugin-improve", "plugin-lifecycle", "manual"]
Received: "user"

Fix: Use "manual" for direct user invocation
```

**Example 3: Missing required field**

```
CONTRACT VIOLATION: plugin-ideation output invalid

Field: brief_path
Expected: string (required field)
Received: undefined

Fix: Ensure BRIEF.md path is returned in output
```

**Example 4: Type mismatch**

```
CONTRACT VIOLATION: plugin-testing input invalid

Field: test_mode
Expected: integer (1, 2, or 3)
Received: "2" (string)

Fix: Pass test_mode as integer, not string
```

**Example 5: Invalid stage number**

```
CONTRACT VIOLATION: plugin-workflow input invalid

Field: start_stage
Expected: integer in range 1-4
Received: 5

Fix: Valid stages are 1 (Foundation), 2 (DSP), 3 (GUI), 4 (Polish)
```

### Step 4: On Success - Continue

If validation passes, proceed with agent logic.

## Validation Strictness

Per CONTEXT.md decisions:
- **Required fields**: Most fields required (strict)
- **Unknown fields**: Rejected (additionalProperties: false)
- **Type coercion**: None - types must match exactly

## Embedding in Agents

Each core agent SKILL.md should include at startup:

```markdown
## Contract Validation

Before processing, validate inputs against contract:

1. **Load schema:** `.claude/schemas/agent-contracts/{agent-name}.input.json`
2. **Validate:** Check all required fields present, types match, constraints satisfied
3. **On violation:** Stop immediately. Report error using format from `.claude/skills/contract-validation/SKILL.md`:
   ```
   CONTRACT VIOLATION: {agent-name} input invalid

   Field: {field_name}
   Expected: {from_schema}
   Received: {actual_value}

   Fix: {actionable_instruction}
   ```
4. **On success:** Proceed to main agent logic

See `.claude/skills/contract-validation/SKILL.md` for full validation protocol.
```

## Error Severity

All contract violations are **blocking**. Do not:
- Attempt to guess missing values
- Coerce types
- Proceed with partial data
- Warn and continue

Stop immediately and report the violation.

## Common Validation Scenarios

### Plugin Name Validation

Plugin names follow pattern: `^[A-Z][a-zA-Z0-9-]*$`
- Must start with uppercase letter
- Can contain letters, numbers, hyphens
- No spaces, underscores, or special characters

### Stage/Phase Validation

Stages are integers 1-4:
- 1 = Foundation
- 2 = DSP
- 3 = GUI
- 4 = Polish

Phases are strings from enum:
- "discuss", "research", "plan", "execute", "verify"

### Status Validation

Plugin statuses must match:
- Ideated, Planning, Stage 1-4, Working, Installed

### Path Validation

File paths should:
- Start with `plugins/` for plugin-relative paths
- Use forward slashes (not backslashes)
- Not contain `..` (parent directory traversal)

## Output Validation

Before returning from agent, validate outputs:

1. Load output schema
2. Check all required fields in return object
3. Validate file creation claims (if `brief_path` returned, file should exist)
4. Report violations same as input violations

## Integration with Agents

All 9 core agents reference this protocol:
- plugin-workflow
- build-automation
- plugin-ideation
- plugin-planning
- plugin-testing
- plugin-improve
- ui-mockup
- plugin-lifecycle
- deep-research

Each agent's SKILL.md includes a "## Contract Validation" section that:
1. References their specific input schema
2. References this protocol for validation rules
3. Specifies blocking behavior on violation

## Schema Location

All agent contract schemas are located at:
```
.claude/schemas/agent-contracts/
├── common/
│   └── plugin-reference.json
├── {agent-name}.input.json
└── {agent-name}.output.json
```

## Version Tracking

Schema versions tracked in `.claude/schemas/CHANGELOG.md`
- Current version: 1.0.0
- Versioning policy: Semver (MAJOR.MINOR.PATCH)
