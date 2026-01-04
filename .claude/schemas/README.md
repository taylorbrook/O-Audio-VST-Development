# JSON Schemas

Unified JSON schemas for Plugin Freedom System data contracts.

## Purpose

Schemas ensure consistent data formats between system components, preventing breakage from undocumented assumptions. All JSON reports must validate against these schemas.

## Schemas

### subagent-report.json

**Used by:** foundation-shell-agent, dsp-agent, gui-agent

**Purpose:** Standardizes stage completion reports returned to plugin-workflow orchestrator

**Required fields:**
- `agent`: Which subagent generated the report
- `status`: "success" or "failure"
- `outputs`: Stage-specific data (plugin_name always required)
- `issues`: Array of issues (empty on success)
- `ready_for_next_stage`: Boolean for workflow progression

**Validation:** plugin-workflow MUST validate all subagent reports against this schema before processing.

### validator-report.json

**Used by:** validator agent

**Purpose:** Standardizes validation reports for stage completion checks

**Required fields:**
- `agent`: Always "validator"
- `stage`: 0-6
- `status`: "PASS" or "FAIL"
- `checks`: Array of check results (name, passed, message, severity)
- `recommendation`: What to do next
- `continue_to_next_stage`: Boolean for workflow progression

**Validation:** plugin-workflow MUST validate validator reports against this schema before processing.

## Schema Validation

### Python (using jsonschema)

```python
import json
from jsonschema import validate, ValidationError

def validate_subagent_report(report_data):
    """Validate subagent report against schema."""
    with open('.claude/schemas/subagent-report.json') as f:
        schema = json.load(f)

    try:
        validate(instance=report_data, schema=schema)
        return True
    except ValidationError as e:
        print(f"Schema validation failed: {e.message}")
        return False

def validate_validator_report(report_data):
    """Validate validator report against schema."""
    with open('.claude/schemas/validator-report.json') as f:
        schema = json.load(f)

    try:
        validate(instance=report_data, schema=schema)
        return True
    except ValidationError as e:
        print(f"Schema validation failed: {e.message}")
        return False
```

### JavaScript (using ajv)

```javascript
const Ajv = require('ajv');
const fs = require('fs');

const ajv = new Ajv();

function validateSubagentReport(reportData) {
    const schema = JSON.parse(fs.readFileSync('.claude/schemas/subagent-report.json'));
    const validate = ajv.compile(schema);
    const valid = validate(reportData);

    if (!valid) {
        console.error('Schema validation failed:', validate.errors);
        return false;
    }
    return true;
}
```

## Adding New Fields

When extending schemas:

1. Add field to appropriate schema file
2. Document field in schema description
3. Mark as required or optional
4. Update all agents/skills that generate reports
5. Test with sample data

## Testing Schemas

Validate sample reports:

```bash
# Using Python
python3 -c "
import json
from jsonschema import validate

# Load schema
with open('.claude/schemas/subagent-report.json') as f:
    schema = json.load(f)

# Test sample report
sample = {
    'agent': 'foundation-shell-agent',
    'status': 'success',
    'outputs': {
        'plugin_name': 'TestPlugin',
        'source_files_created': ['Source/PluginProcessor.h']
    },
    'issues': [],
    'ready_for_next_stage': True
}

validate(instance=sample, schema=schema)
print('✓ Schema validation passed')
"
```

## Schema Violations

When a report fails schema validation, the system MUST:

1. Log the validation error with details
2. Block workflow progression
3. Report to user with actionable error message
4. Never proceed with malformed data

This prevents cascading failures from corrupted state.
