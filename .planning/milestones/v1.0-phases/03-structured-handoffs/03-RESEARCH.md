# Phase 3: Structured Handoffs - Research

**Researched:** 2026-01-30
**Domain:** Schema-validated stage transitions, decision audit trails, context handoff
**Confidence:** HIGH

## Summary

Phase 3 implements structured handoff documents for stage transitions in the plugin workflow. The research confirms that JSON Schema draft 2020-12 (already established in Phase 1) with `additionalProperties: false` is the standard approach for strict validation. Stage-specific schemas define what flows between stages (0->1, 1->2, 2->3, 3->4), with decision audit trails following the Architecture Decision Record (ADR) pattern adapted for plugin development context.

The key insight is that handoffs serve agents, not humans. They capture decisions with rationale, required artifacts with validation rules, and context needed to resume work. This differs from human documentation in that every field is machine-parseable and validation-mandatory.

**Primary recommendation:** Create 4 stage-boundary schemas (handoff-0-to-1, handoff-1-to-2, handoff-2-to-3, handoff-3-to-4) with hard fail validation, decision audit trail integrated inline, and copy-paste slash commands for context continuation.

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JSON Schema | draft 2020-12 | Schema definition | Already established in Phase 1, strict mode supported |
| jq | 1.7.1+ | JSON validation/query | Available on macOS (verified), fast CLI validation |
| npx ajv-cli | 5.0+ | Full JSON Schema validation | Supports draft 2020-12, can be used via npx |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| semver | Any | Version comparison | Schema version compatibility checks |
| git | Any | Audit trail history | Decision supersession tracking |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| ajv-cli | check-jsonschema (Python) | Python required, but better error messages |
| jq validation | Full ajv | jq is simpler for basic structure checks, ajv for full schema |
| Inline decisions | Separate DECISIONS.md | Inline keeps context together, separate allows summary views |

**Installation:**
```bash
# jq already available (verified: jq-1.7.1-apple)
# ajv-cli via npx (npx available: /usr/local/bin/npx)
npx ajv-cli validate -s schema.json -d document.json
```

## Architecture Patterns

### Recommended Project Structure

```
plugins/[PluginName]/.planning/
├── stages/
│   ├── 0-ideation/
│   │   ├── CONTEXT.md
│   │   └── HANDOFF.json          # Handoff to stage 1
│   ├── 1-foundation/
│   │   ├── CONTEXT.md
│   │   ├── PLAN.md
│   │   ├── SUMMARY.md
│   │   ├── VERIFICATION.md
│   │   └── HANDOFF.json          # Handoff to stage 2
│   ├── 2-dsp/
│   │   └── HANDOFF.json          # Handoff to stage 3
│   └── 3-gui/
│       └── HANDOFF.json          # Handoff to stage 4
├── STATUS.md                      # Source of truth for current state
├── DECISIONS.md                   # Central decision log (summary)
└── BRIEF.md, ROADMAP.md, etc.

.planning/workflow/schemas/
├── handoff-0-to-1.schema.json
├── handoff-1-to-2.schema.json
├── handoff-2-to-3.schema.json
├── handoff-3-to-4.schema.json
└── decision-entry.schema.json     # Reusable decision structure
```

### Pattern 1: Summary + Refs Handoff Structure

**What:** Inline critical state for immediate context, reference external files for details
**When to use:** Every handoff document
**Example:**
```json
{
  "$schema": "../../../.planning/workflow/schemas/handoff-1-to-2.schema.json",
  "schemaVersion": "1.0.0",
  "plugin": "O-IntonationPad",
  "timestamp": "2026-01-30T10:00:00Z",

  "summary": "Foundation complete: 15 APVTS parameters, VST3/AU built, auval verified",

  "stateSnapshot": {
    "stage": "1-foundation",
    "phase": "complete",
    "parameterCount": 15,
    "source": ["STATUS.md", "registry.json"]
  },

  "artifacts": {
    "required": [
      { "path": "Source/PluginProcessor.h", "exists": true },
      { "path": "Source/PluginProcessor.cpp", "exists": true },
      { "path": "CMakeLists.txt", "exists": true }
    ]
  },

  "decisions": [
    {
      "id": "FOUNDATION-001",
      "decision": "Use APVTS for parameter management",
      "rationale": "Thread-safe, supports automation, standard JUCE pattern",
      "date": "2026-01-29",
      "alternatives": ["Manual atomic parameters", "Custom message queue"],
      "supersedes": null
    }
  ],

  "contextForNextStage": {
    "receivingStage": "2-dsp",
    "keyInformation": [
      "15 parameters defined in APVTS (see parameter-spec.md)",
      "Plugin type: Synth (output-only bus)",
      "Complexity: 5.0 (phased DSP implementation required)"
    ],
    "continueCommand": "/plugin-execute O-IntonationPad 2-dsp"
  }
}
```

### Pattern 2: Decision Audit Trail (ADR-Inspired)

**What:** Immutable decisions with supersession pattern
**When to use:** Major architectural/behavioral choices
**Example:**
```json
{
  "id": "DSP-002",
  "decision": "Use global LFO instead of per-voice LFO",
  "rationale": "Unified pad movement, simpler CPU profile, matches Omnisphere reference",
  "date": "2026-01-29",
  "alternatives": ["Per-voice LFO with independent phases", "LFO bank with voice assignment"],
  "supersedes": null
}
```

**Supersession pattern:**
```json
{
  "id": "DSP-003",
  "decision": "Reduce max polyphony from 8 to 6 voices",
  "rationale": "CPU exceeded 80% with 96 oscillators, fallback applied",
  "date": "2026-01-30",
  "alternatives": ["Reduce chord voices", "Optimize oscillator code"],
  "supersedes": "DSP-001"
}
```

### Pattern 3: Artifact Validation

**What:** Explicit list of required files with existence checks
**When to use:** Stage boundaries to ensure completeness
**Example:**
```json
{
  "artifacts": {
    "required": [
      { "path": "Source/DSP/WavetableOscillator.h", "exists": true, "type": "header" },
      { "path": "Source/DSP/ChordGenerator.cpp", "exists": true, "type": "implementation" }
    ],
    "optional": [
      { "path": "Source/DSP/FallbackOscillator.h", "exists": false, "note": "Only if CPU optimization needed" }
    ],
    "validation": {
      "allRequiredPresent": true,
      "checkedAt": "2026-01-30T10:15:00Z"
    }
  }
}
```

### Pattern 4: Copy-Paste Slash Command Continuation

**What:** Machine-parseable command for context restoration
**When to use:** Every handoff for session resume
**Example:**
```json
{
  "contextForNextStage": {
    "continueCommand": "/plugin-execute O-IntonationPad 2-dsp",
    "alternativeCommands": [
      "/plugin-resume O-IntonationPad",
      "/plugin-status O-IntonationPad"
    ]
  }
}
```

### Anti-Patterns to Avoid

- **Optional fields in handoff schemas:** Every field must be required per user decision. Use separate schemas for stage-specific content rather than optional fields.
- **Human-only prose:** Handoffs serve agents. Every piece of information must be machine-parseable (use structured JSON, not prose paragraphs).
- **Detached decision logs:** Per discussion, decisions go both inline in handoff AND in central DECISIONS.md. Never only in one place.
- **Forward/backward schema compatibility:** Per discussion, strict version match required. Old handoffs stay on their schema version.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| JSON Schema validation | Custom parser | ajv-cli or jq | Edge cases in JSON Schema spec are complex |
| Version comparison | String compare | semver library | Semver has non-obvious rules (1.10.0 > 1.9.0) |
| Date-time parsing | Regex | ISO 8601 standard | Timezone handling is tricky |
| Decision ID generation | Random | Sequential (STAGE-NNN) | Human-readable, sortable |

**Key insight:** Schema validation seems simple but draft 2020-12 has subtle behaviors (items vs prefixItems, $dynamicRef, format-assertion vs format-annotation). Use established validators.

## Common Pitfalls

### Pitfall 1: Schema Version Drift

**What goes wrong:** Handoff created with schema v1.0.0, validation runs against v1.1.0, fails on new required field
**Why it happens:** Schema files update but existing handoffs don't migrate
**How to avoid:** Validate handoff against exact version declared in `$schema` field
**Warning signs:** "unexpected required field" errors on previously-valid handoffs

### Pitfall 2: Artifact Path Inconsistency

**What goes wrong:** Handoff says `Source/DSP/Filter.h` exists, file is actually at `Source/dsp/Filter.h`
**Why it happens:** Case sensitivity differences, path normalization issues
**How to avoid:** Validate artifact paths at handoff creation time, not just at boundary
**Warning signs:** "file not found" errors despite "exists: true" in handoff

### Pitfall 3: Decision ID Collisions

**What goes wrong:** Two decisions with same ID, supersession chain breaks
**Why it happens:** Manual ID assignment without checking existing IDs
**How to avoid:** Generate IDs from stage prefix + sequential number, check DECISIONS.md before assigning
**Warning signs:** Duplicate ID warnings, broken supersession chains

### Pitfall 4: Incomplete State Snapshot

**What goes wrong:** Handoff snapshot says stage=2, STATUS.md says stage=3, validation passes but state is inconsistent
**Why it happens:** Handoff created but state files not updated atomically
**How to avoid:** Validate handoff against live state at boundary, not just handoff internal consistency
**Warning signs:** Registry mismatch warnings (similar to Phase 2 validation)

### Pitfall 5: Missing Context for Recovery

**What goes wrong:** Agent reads handoff but doesn't know which command to run
**Why it happens:** `continueCommand` field missing or outdated
**How to avoid:** Every handoff MUST include copy-paste slash command
**Warning signs:** "what should I do next?" questions from agents after loading handoff

## Code Examples

### JSON Schema for Stage 1->2 Handoff

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "./handoff-1-to-2.schema.json",
  "title": "Stage 1 to 2 Handoff",
  "description": "Foundation to DSP stage transition handoff",
  "type": "object",
  "required": ["$schema", "schemaVersion", "plugin", "timestamp", "summary", "stateSnapshot", "artifacts", "decisions", "contextForNextStage"],
  "additionalProperties": false,
  "properties": {
    "$schema": {
      "type": "string",
      "description": "Reference to this schema file"
    },
    "schemaVersion": {
      "type": "string",
      "const": "1.0.0",
      "description": "Schema version (semver)"
    },
    "plugin": {
      "type": "string",
      "description": "Plugin name"
    },
    "timestamp": {
      "type": "string",
      "format": "date-time",
      "description": "Handoff creation time (ISO 8601)"
    },
    "summary": {
      "type": "string",
      "description": "Human-readable summary (one sentence)"
    },
    "stateSnapshot": {
      "$ref": "#/$defs/StateSnapshot"
    },
    "artifacts": {
      "$ref": "#/$defs/ArtifactList"
    },
    "decisions": {
      "type": "array",
      "items": { "$ref": "#/$defs/DecisionEntry" },
      "minItems": 0
    },
    "contextForNextStage": {
      "$ref": "#/$defs/NextStageContext"
    }
  },
  "$defs": {
    "StateSnapshot": {
      "type": "object",
      "required": ["stage", "phase", "parameterCount", "source"],
      "additionalProperties": false,
      "properties": {
        "stage": { "type": "string" },
        "phase": { "type": "string" },
        "parameterCount": { "type": "integer", "minimum": 0 },
        "source": { "type": "array", "items": { "type": "string" } }
      }
    },
    "ArtifactList": {
      "type": "object",
      "required": ["required", "validation"],
      "additionalProperties": false,
      "properties": {
        "required": {
          "type": "array",
          "items": { "$ref": "#/$defs/ArtifactEntry" }
        },
        "optional": {
          "type": "array",
          "items": { "$ref": "#/$defs/ArtifactEntry" }
        },
        "validation": {
          "type": "object",
          "required": ["allRequiredPresent", "checkedAt"],
          "additionalProperties": false,
          "properties": {
            "allRequiredPresent": { "type": "boolean" },
            "checkedAt": { "type": "string", "format": "date-time" }
          }
        }
      }
    },
    "ArtifactEntry": {
      "type": "object",
      "required": ["path", "exists"],
      "additionalProperties": false,
      "properties": {
        "path": { "type": "string" },
        "exists": { "type": "boolean" },
        "type": { "type": "string" },
        "note": { "type": "string" }
      }
    },
    "DecisionEntry": {
      "type": "object",
      "required": ["id", "decision", "rationale", "date", "alternatives", "supersedes"],
      "additionalProperties": false,
      "properties": {
        "id": { "type": "string", "pattern": "^[A-Z]+-[0-9]+$" },
        "decision": { "type": "string" },
        "rationale": { "type": "string" },
        "date": { "type": "string", "format": "date" },
        "alternatives": { "type": "array", "items": { "type": "string" } },
        "supersedes": { "type": ["string", "null"] }
      }
    },
    "NextStageContext": {
      "type": "object",
      "required": ["receivingStage", "keyInformation", "continueCommand"],
      "additionalProperties": false,
      "properties": {
        "receivingStage": { "type": "string" },
        "keyInformation": { "type": "array", "items": { "type": "string" } },
        "continueCommand": { "type": "string" },
        "alternativeCommands": { "type": "array", "items": { "type": "string" } }
      }
    }
  }
}
```

### Validation Script (Bash)

```bash
#!/bin/bash
# validate-handoff.sh - Validates handoff document against schema

HANDOFF_FILE=$1
SCHEMA_DIR=".planning/workflow/schemas"

# Extract schema version from handoff
SCHEMA_REF=$(jq -r '."$schema"' "$HANDOFF_FILE")
SCHEMA_FILE="${SCHEMA_DIR}/$(basename "$SCHEMA_REF")"

# Validate schema exists
if [ ! -f "$SCHEMA_FILE" ]; then
    echo "ERROR: Schema not found: $SCHEMA_FILE"
    exit 1
fi

# Validate with ajv-cli
npx ajv-cli validate -s "$SCHEMA_FILE" -d "$HANDOFF_FILE" --spec=draft2020

if [ $? -ne 0 ]; then
    echo "VALIDATION FAILED: Handoff does not match schema"
    exit 1
fi

# Additional checks: artifact existence
jq -r '.artifacts.required[] | select(.exists == true) | .path' "$HANDOFF_FILE" | while read -r path; do
    PLUGIN_DIR=$(dirname "$(dirname "$HANDOFF_FILE")")
    FULL_PATH="${PLUGIN_DIR}/${path}"
    if [ ! -f "$FULL_PATH" ]; then
        echo "ERROR: Artifact missing: $FULL_PATH (declared as exists: true)"
        exit 1
    fi
done

echo "VALIDATION PASSED"
exit 0
```

### Gate Check Integration

```bash
#!/bin/bash
# stage-transition-gate.sh - Blocks transition if handoff invalid

PLUGIN=$1
FROM_STAGE=$2
TO_STAGE=$3

HANDOFF_FILE="plugins/${PLUGIN}/.planning/stages/${FROM_STAGE}/HANDOFF.json"

# Check handoff exists
if [ ! -f "$HANDOFF_FILE" ]; then
    echo "GATE BLOCKED: No handoff document for stage ${FROM_STAGE}"
    echo "Create handoff with: /plugin-handoff ${PLUGIN} ${FROM_STAGE}"
    exit 1
fi

# Validate handoff
./validate-handoff.sh "$HANDOFF_FILE"
if [ $? -ne 0 ]; then
    echo "GATE BLOCKED: Handoff validation failed"
    exit 1
fi

# Verify receiving stage matches
DECLARED_NEXT=$(jq -r '.contextForNextStage.receivingStage' "$HANDOFF_FILE")
if [ "$DECLARED_NEXT" != "$TO_STAGE" ]; then
    echo "GATE BLOCKED: Handoff targets ${DECLARED_NEXT}, not ${TO_STAGE}"
    exit 1
fi

echo "GATE PASSED: Transition ${FROM_STAGE} -> ${TO_STAGE} approved"
exit 0
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Prose handoff docs | Schema-validated JSON | 2024 (GSD adoption) | Machine-parseable, validation-mandatory |
| Single ADR files | Inline + summary log | 2025 (Context7 patterns) | Decisions travel with handoff, summary allows audit |
| Optional fields | All required, stage-specific schemas | User decision | No ambiguity, validation catches missing data |
| Forward compat | Strict version match | User decision | No migration needed, old handoffs preserved |

**Deprecated/outdated:**
- Markdown-only handoffs: Cannot be validated, no structure guarantees
- JSON Schema draft-04/07: Use draft 2020-12 for prefixItems, $dynamicRef support

## Open Questions

1. **Bypass flag implementation**
   - What we know: User left --force flag to Claude's discretion
   - What's unclear: What warnings/logging should accompany bypass
   - Recommendation: Implement --force with mandatory warning log entry, require explicit acknowledgment

2. **Schema file naming convention**
   - What we know: Need 4 stage-boundary schemas
   - What's unclear: Use `handoff-0-to-1.schema.json` or `stage-0-handoff.schema.json`
   - Recommendation: `handoff-X-to-Y.schema.json` (clearer about direction)

3. **Central DECISIONS.md format**
   - What we know: Both inline in handoff + central summary
   - What's unclear: How to auto-generate central summary from handoff decisions
   - Recommendation: Use jq to extract and append decisions during handoff creation

## Sources

### Primary (HIGH confidence)
- [JSON Schema Understanding Guide](/websites/json-schema_understanding-json-schema) - additionalProperties, required patterns
- [JSON Schema Draft 2020-12 Spec](https://json-schema.org/draft/2020-12) - Format vocabulary, prefixItems changes
- [JUCE 8 Documentation](https://docs.juce.com) - Verified against existing agent contracts

### Secondary (MEDIUM confidence)
- [ADR GitHub Resources](https://adr.github.io/) - Architecture Decision Records patterns
- [AWS ADR Best Practices](https://aws.amazon.com/blogs/architecture/master-architecture-decision-records-adrs-best-practices-for-effective-decision-making/) - Enterprise ADR adoption patterns
- [Microsoft ADR Guide](https://learn.microsoft.com/en-us/azure/well-architected/architect-role/architecture-decision-record) - ADR maintenance and immutability
- [Snowplow SchemaVer](https://snowplow.io/blog/introducing-schemaver-for-semantic-versioning-of-schemas) - Schema versioning alternatives

### Tertiary (LOW confidence)
- [Context Engineering with Slash Commands](https://howibuild.ai/context-engineering-with-slash-commands) - Copy-paste continuation patterns (newer approach, limited track record)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - JSON Schema 2020-12 already established, jq/ajv verified available
- Architecture: HIGH - Patterns derived from existing agent contracts and Phase 1/2 infrastructure
- Pitfalls: MEDIUM - Based on common schema validation issues, not project-specific incidents yet

**Research date:** 2026-01-30
**Valid until:** 2026-02-28 (30 days - stable domain, schema standards don't change frequently)
