# Schema Changelog

Centralized version history for all agent contract schemas.

## Versioning Policy

Per CONTEXT.md: Full semver (MAJOR.MINOR.PATCH)
- **MAJOR**: Breaking changes (removed/renamed required fields, type changes)
- **MINOR**: New optional fields, loosened constraints
- **PATCH**: Documentation, formatting, examples

## Version History

### v1.0.0 - 2026-01-30

**Initial Release**

Created input/output schemas for 9 core agents:
- plugin-workflow (orchestrates implementation stages)
- build-automation (handles builds and installation)
- plugin-ideation (gathers creative vision)
- plugin-planning (creates architecture and roadmap)
- plugin-testing (runs tests and validation)
- plugin-improve (post-completion improvements)
- ui-mockup (generates UI previews)
- plugin-lifecycle (manages status transitions)
- deep-research (investigates technical topics)

Common types:
- plugin-reference (standard plugin identifier)

All schemas:
- Use JSON Schema draft 2020-12
- Set additionalProperties: false
- Include descriptive field documentation
- Validate against strict type checking

**Files Created:**

| File | Purpose |
|------|---------|
| `common/plugin-reference.json` | Shared plugin identifier type |
| `plugin-workflow.input.json` | Workflow orchestration inputs |
| `plugin-workflow.output.json` | Workflow orchestration outputs |
| `build-automation.input.json` | Build pipeline inputs |
| `build-automation.output.json` | Build pipeline outputs |
| `plugin-ideation.input.json` | Brainstorming inputs |
| `plugin-ideation.output.json` | Brainstorming outputs |
| `plugin-planning.input.json` | Planning inputs |
| `plugin-planning.output.json` | Planning outputs |
| `plugin-testing.input.json` | Testing inputs |
| `plugin-testing.output.json` | Testing outputs |
| `plugin-improve.input.json` | Improvement inputs |
| `plugin-improve.output.json` | Improvement outputs |
| `ui-mockup.input.json` | UI design inputs |
| `ui-mockup.output.json` | UI design outputs |
| `plugin-lifecycle.input.json` | Lifecycle inputs |
| `plugin-lifecycle.output.json` | Lifecycle outputs |
| `deep-research.input.json` | Research inputs |
| `deep-research.output.json` | Research outputs |

---

## Schema Index

| Agent | Input Schema | Output Schema | Version |
|-------|--------------|---------------|---------|
| plugin-workflow | plugin-workflow.input.json | plugin-workflow.output.json | 1.0.0 |
| build-automation | build-automation.input.json | build-automation.output.json | 1.0.0 |
| plugin-ideation | plugin-ideation.input.json | plugin-ideation.output.json | 1.0.0 |
| plugin-planning | plugin-planning.input.json | plugin-planning.output.json | 1.0.0 |
| plugin-testing | plugin-testing.input.json | plugin-testing.output.json | 1.0.0 |
| plugin-improve | plugin-improve.input.json | plugin-improve.output.json | 1.0.0 |
| ui-mockup | ui-mockup.input.json | ui-mockup.output.json | 1.0.0 |
| plugin-lifecycle | plugin-lifecycle.input.json | plugin-lifecycle.output.json | 1.0.0 |
| deep-research | deep-research.input.json | deep-research.output.json | 1.0.0 |

---

## Future Changes

When modifying schemas:

1. Determine change type (MAJOR/MINOR/PATCH)
2. Update affected schema files
3. Add changelog entry with:
   - Version number
   - Date
   - Description of changes
   - Migration notes for MAJOR changes
4. Update Schema Index table
5. Update `$id` in schema files if version tracked there

---

## Breaking Change Examples

For reference when determining MAJOR vs MINOR:

**MAJOR (breaking):**
- Removing a required field
- Changing a field's type (string -> integer)
- Renaming a required field
- Tightening constraints on required fields (adding stricter pattern)
- Changing enum values that are commonly used

**MINOR (non-breaking):**
- Adding new optional fields
- Adding new enum values
- Loosening constraints (wider pattern, larger range)
- Adding new properties to nested objects

**PATCH (documentation):**
- Fixing typos in descriptions
- Adding examples
- Clarifying field documentation
- Formatting changes
