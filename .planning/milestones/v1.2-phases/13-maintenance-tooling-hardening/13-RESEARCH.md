# Phase 13: Maintenance Tooling & Hardening - Research

**Researched:** 2026-02-06
**Domain:** YAML frontmatter extension, manifest auto-generation hooks, staleness tracking, graceful degradation
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Auto-generation behavior
- Manifest regenerates **on file write** via Claude hooks (not on commit, not manual-only)
- When a research doc has no frontmatter or malformed frontmatter: **skip the doc with a warning** and create a trackable todo/bug report for fixing the malformed frontmatter
- Agents that create research docs (e.g., deep-research, gsd-phase-researcher) must **auto-populate valid frontmatter** at creation time

#### Freshness & staleness policy
- `juce_version` represents the version the doc was **last verified against** (not "written for") -- updates alongside `last_verified`
- Stale resources (>90 days) are still injected but flagged with a warning
- Verification is both manual (edit frontmatter directly) and script-assisted (convenience command for batch operations)

#### Graceful degradation strategy
- When manifest is missing: agents **proceed silently** with no research context, warning logged to stderr only
- Agents never see a notice in their prompt about missing infrastructure -- the system just works without it
- On discovery script crash: return empty (clean failure, no corrupted partial results)

#### Frontmatter standards
- **Required fields:** created, last_verified, juce_version, keywords, category (5 mandatory fields)
- Docs missing any required field are **rejected from manifest** until all fields are present
- Validation warns per missing field so authors know exactly what to fix

### Claude's Discretion

- Staleness warning format (inline vs stderr -- pick what's most practical)
- Whether to show a diff when regenerating manifest
- Staleness threshold configurability (fixed 90 days vs configurable)
- Degradation event persistence (stderr only vs log file)
- Test coverage depth for failure scenarios (main 3 vs edge cases)

### Deferred Ideas (OUT OF SCOPE)

None -- discussion stayed within phase scope

</user_constraints>

## Summary

Phase 13 extends the Phase 10 resource discovery infrastructure with four capabilities: (1) auto-regeneration of `resource-index.json` when research files are written, triggered via a PostToolUse Claude hook; (2) staleness tracking via three new frontmatter fields (`created`, `last_verified`, `juce_version`) added to all 26 research documents and the schema; (3) graceful degradation throughout the pipeline so agents never crash on missing/broken infrastructure; and (4) enforcement of frontmatter standards where documents missing required fields are rejected from the manifest with actionable per-field error messages.

The existing infrastructure is solid: `generate-resource-index.py` (159 lines), `discover-resources.py` (259 lines), `inject-context.py` (381 lines), `validate-research-frontmatter.py` (233 lines), and the manifest with 26 indexed documents are all verified and working. Phase 13 modifies these files in-place rather than creating new ones, except for: (1) a new hook script for auto-regeneration, (2) a batch verification script for staleness updates, and (3) any agent instruction updates for frontmatter auto-population.

The key challenge is the hook-triggered regeneration. The manifest generator currently takes ~63ms (based on Phase 10 verification), well within the 2000ms PostToolUse hook timeout. However, the generator imports `jsonschema` which has measurable import overhead (~200ms). The hook must be selective (only fire on `research/*.md` writes) and must handle failures silently to avoid blocking agent workflow.

**Primary recommendation:** Extend existing scripts in-place. Add 3 new frontmatter fields to schema/validator/generator. Create a thin PostToolUse hook that calls the generator only when research files are written. Add staleness checking to the discovery/injection scripts. Test all degradation paths.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Python | 3.14.2 | Runtime for all scripts | Already installed, project standard |
| PyYAML | 6.0.3 | Parse YAML frontmatter | Already installed and used in all validators |
| jsonschema | 4.26.0 | Validate manifest schema | Already installed, used by generator and discovery |
| json (stdlib) | builtin | Read/write manifest JSON | Zero dependency |
| pathlib (stdlib) | builtin | File path operations | Project convention |
| datetime (stdlib) | builtin | Date parsing for staleness calculation | Zero dependency |
| re (stdlib) | builtin | Frontmatter delimiter parsing | Already in use |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| argparse (stdlib) | builtin | CLI for batch verification script | Verification script only |
| sys (stdlib) | builtin | stderr output for warnings | Hook/degradation logging |
| subprocess (stdlib) | builtin | Hook calls generator script | PostToolUse hook only |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| PostToolUse hook trigger | Git pre-commit hook | Pre-commit runs outside Claude context, misses agent writes |
| subprocess from bash hook | Direct Python import in hook | Bash hook is simpler, avoids Python environment issues in hooks |
| Fixed 90-day threshold | Configurable via env/config | Fixed is simpler; configurability adds complexity for no immediate benefit |

**Installation:**
```bash
# No new dependencies needed -- all already installed
```

## Architecture Patterns

### Current File Layout (Phase 10 artifacts to modify)
```
.claude/
  resource-index.json                    # MODIFY: add created, last_verified, juce_version per document
  resource-index.schema.json             # MODIFY: add 3 new required fields to DocumentEntry
  hooks/
    hooks.json                           # MODIFY: add manifest regeneration hook entry
    PostToolUse.sh                       # DO NOT MODIFY (it's the broad matcher hook)
    validators/
      validate-research-frontmatter.py   # MODIFY: add 3 new required fields to validation
  scripts/
    generate-resource-index.py           # MODIFY: extract + propagate 3 new fields
    discover-resources.py                # MODIFY: add staleness warning logic
    inject-context.py                    # MODIFY: propagate staleness warnings to output

research/
  *.md (26 files)                        # MODIFY: add created, last_verified, juce_version frontmatter
```

### New Files to Create
```
.claude/
  hooks/
    regenerate-manifest.sh               # NEW: PostToolUse hook for research file writes
  scripts/
    verify-freshness.py                  # NEW: batch verification script (convenience command)
```

### Pattern 1: Hook-Triggered Manifest Regeneration

**What:** A PostToolUse hook that detects writes to `research/*.md` files and triggers `generate-resource-index.py` to rebuild the manifest.

**When to use:** Fires automatically on every Write/Edit operation via Claude hooks.

**Design:**
```bash
#!/bin/bash
# regenerate-manifest.sh -- PostToolUse hook for manifest auto-generation
INPUT=$(cat)
TOOL_NAME=$(echo "$INPUT" | jq -r '.tool_name // empty' 2>/dev/null)

# Only trigger on Write/Edit
if [[ ! "$TOOL_NAME" =~ ^(Write|Edit)$ ]]; then
  exit 0
fi

FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // empty' 2>/dev/null)

# Only trigger for research markdown files
if [[ ! "$FILE_PATH" =~ research/.*\.md$ ]]; then
  exit 0
fi

# Skip README.md
if [[ "$(basename "$FILE_PATH")" == "README.md" ]]; then
  exit 0
fi

# Regenerate manifest silently -- never block workflow
python3 "${CLAUDE_PROJECT_DIR}/.claude/scripts/generate-resource-index.py" 2>/dev/null || true

exit 0
```

**Hook registration in hooks.json:**
```json
{
  "matcher": "write|edit",
  "hooks": [
    {
      "type": "command",
      "command": "${CLAUDE_PROJECT_DIR}/.claude/hooks/regenerate-manifest.sh",
      "timeout": 3000,
      "description": "Auto-regenerate resource manifest on research file writes"
    }
  ]
}
```

**Key design decisions:**
- **Separate hook file** (not added to PostToolUse.sh): PostToolUse.sh is the broad-matcher hook for plugin source validation. Research manifest regeneration is a different concern with a `write|edit` matcher.
- **3000ms timeout**: Generator runs in ~63ms plus ~200ms jsonschema import = ~263ms typical. 3000ms gives 11x headroom.
- **Silent failure** (`2>/dev/null || true`): Per decision, never block workflow on regeneration failure.
- **Exit 0 always**: Hook must never return non-zero or it blocks Claude's workflow.

### Pattern 2: Frontmatter Field Extension

**What:** Add `created`, `last_verified`, and `juce_version` to every research document's YAML frontmatter.

**Current frontmatter (7 fields):**
```yaml
---
title: "FFT Processing Best Practices in JUCE"
summary: "..."
domain: dsp
type: guide
keywords: [fft, stft, spectral-processing]
stages: [1, 2, 3]
agents: [dsp]
---
```

**Extended frontmatter (10 fields, 3 new):**
```yaml
---
title: "FFT Processing Best Practices in JUCE"
created: 2026-01-15
last_verified: 2026-02-05
juce_version: "8.0.4"
summary: "..."
domain: dsp
type: guide
keywords: [fft, stft, spectral-processing]
stages: [1, 2, 3]
agents: [dsp]
---
```

**Field semantics:**
- `created`: Date document was originally written (YYYY-MM-DD format, immutable after creation)
- `last_verified`: Date content was last confirmed accurate against current JUCE version (YYYY-MM-DD, updates on re-verification)
- `juce_version`: JUCE version the content was last verified against (string like "8.0.4", updates alongside `last_verified`)

**Field ordering convention:** Place the 3 new temporal fields after `title` and before `summary` for visual grouping (dates together, then description fields).

**Note on "category" in the CONTEXT.md:** The user's required fields list says `created, last_verified, juce_version, keywords, category`. The existing `domain` field already serves as the category (values: dsp, ui, build, workflow). Rather than adding a redundant `category` field, the `domain` field IS the category. The validator already enforces it. No new field needed for "category" -- it is already covered.

### Pattern 3: Staleness Detection in Discovery/Injection

**What:** When injecting research context, detect documents where `last_verified` is older than 90 days and flag them.

**Implementation location:** `inject-context.py` (the script that formats discovery results for agent prompts).

**Staleness check logic:**
```python
from datetime import date

STALENESS_THRESHOLD_DAYS = 90

def check_staleness(last_verified_str):
    """Check if a document is stale based on last_verified date.

    Returns (is_stale, days_old) tuple.
    """
    try:
        last_verified = date.fromisoformat(last_verified_str)
        days_old = (date.today() - last_verified).days
        return days_old > STALENESS_THRESHOLD_DAYS, days_old
    except (ValueError, TypeError):
        # Can't parse date -- treat as stale with unknown age
        return True, -1
```

**Recommendation for Claude's Discretion items:**

1. **Staleness warning format:** Use **stderr** for staleness warnings. Rationale: The decision says "agents never see a notice in their prompt about missing infrastructure." Staleness is a maintenance concern, not an agent concern. Emit to stderr so the orchestrator/user sees it, but the injected context block stays clean. However, within the `<research_context>` block, add a subtle annotation like `(verified: 2025-10-01, 128 days ago)` next to the document title so agents have the information but are not "warned" per se.

2. **Diff on manifest regeneration:** **No diff**. Rationale: The hook runs silently. Showing diffs would add output noise on every research file write. The generator already reports document counts to stdout.

3. **Staleness threshold configurability:** **Fixed 90 days**. Rationale: Simpler implementation, single source of truth. If the threshold needs changing later, it's a one-line constant change. No config file complexity needed for a single integer.

4. **Degradation event persistence:** **stderr only** (no log file). Rationale: Claude hooks already capture stderr output. A separate log file adds maintenance burden (rotation, location, cleanup) for minimal benefit. The warnings are transient -- if the manifest is missing, the fix is to run the generator.

5. **Test coverage depth:** **Main 3 scenarios** (manifest missing, discovery crash, malformed frontmatter) plus **2 edge cases** (empty manifest, schema file missing). Five tests total cover the critical paths without over-testing.

### Pattern 4: Graceful Degradation at Every Layer

**What:** Ensure every component in the resource pipeline degrades gracefully without crashing or surfacing errors to agents.

**Layer-by-layer degradation:**

| Layer | Failure Mode | Degradation Behavior | Implementation |
|-------|-------------|---------------------|----------------|
| Hook (regenerate-manifest.sh) | Generator crashes | `|| true` swallows error, exit 0 | Already in hook pattern |
| Generator (generate-resource-index.py) | Malformed frontmatter in file | Skip file, warn to stderr, continue | Already implemented (line 63-66) |
| Generator (generate-resource-index.py) | No research directory | Exit 1 with message | Already implemented (line 102-104) |
| Discovery (discover-resources.py) | Missing manifest | Return empty `[]` | Already implemented (line 82-83) |
| Discovery (discover-resources.py) | Schema validation fails | Return empty `[]` | Already implemented (line 102-103) |
| Discovery (discover-resources.py) | jsonschema import fails | Exit 1 | **NEEDS FIX**: Should return `[]`, not crash |
| Injection (inject-context.py) | Discovery returns empty | Return "no resources" message | Already implemented (line 276-277) |
| Injection (inject-context.py) | Exception in any code path | Return empty string | Already implemented (line 334-336) |
| Frontmatter validator | File not in research/ | Return 0 (pass) silently | Already implemented (line 220-221) |

**Key gap found:** `discover-resources.py` lines 23-26 call `sys.exit(1)` if jsonschema is not importable. Per the graceful degradation decision, this should instead log to stderr and return empty results. Same issue exists in `generate-resource-index.py` lines 22-24.

### Pattern 5: Malformed Frontmatter Bug Reporting

**What:** When the generator encounters a research doc with missing or malformed frontmatter, create a trackable issue.

**Design options considered:**
1. **GitHub Issue** via `gh issue create` -- heavyweight, requires API access
2. **Local TODO file** at `.claude/frontmatter-issues.txt` -- simple, greppable
3. **Stderr with structured format** -- ephemeral, lost after session

**Recommendation:** Use option 2 (local TODO file). The generator writes one line per skipped file to `.claude/frontmatter-issues.txt` with structured format:

```
[2026-02-06] research/plugin-development-without-juce.md: missing fields: created, last_verified, juce_version
[2026-02-06] research/new-doc.md: invalid YAML in frontmatter
```

The file is:
- **Visible**: agents and users can easily find/read it
- **Trackable**: date-stamped, shows accumulation
- **Actionable**: lists exactly which fields are missing per file
- **Durable**: survives across sessions (unlike stderr)
- **Mergeable**: git-tracked, shows up in status

The generator overwrites this file on each run (regeneration is idempotent). If all docs have valid frontmatter, the file is empty or deleted.

### Anti-Patterns to Avoid

- **Blocking on regeneration failure**: The hook MUST exit 0 regardless of generator outcome. Never block agent workflow because the manifest couldn't be rebuilt.
- **Adding staleness warnings to agent prompts as blocking notices**: Per decision, agents proceed normally. Staleness is metadata, not a stop signal.
- **Modifying PostToolUse.sh for regeneration**: This file handles plugin source code validation with a `.*` matcher. Adding research manifest logic to it creates coupling. Use a separate hook file with `write|edit` matcher.
- **Using `additionalProperties: false` and then adding new fields**: The schema already has `additionalProperties: false` on DocumentEntry. Adding `created`, `last_verified`, `juce_version` to the schema must happen in the SAME change as adding them to the generator, or the generator output will fail validation.
- **Date strings without consistent format**: Use ISO 8601 `YYYY-MM-DD` (not datetime with timezone) for frontmatter dates. This matches YAML's native date type and is parseable by `date.fromisoformat()`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Date parsing for staleness | Custom regex date parser | `datetime.date.fromisoformat()` | Handles YYYY-MM-DD natively, raises ValueError on bad input |
| YAML frontmatter parsing | Custom delimiter scanner | Existing `parse_frontmatter()` in generator | Already tested, handles edge cases |
| Schema validation | Per-field type checking | jsonschema library (already installed) | Already used, Draft 2020-12 support, proper error messages |
| File write detection in hooks | Custom file watcher / inotify | Claude PostToolUse hook system | Already integrated, handles Write and Edit operations |

**Key insight:** Every building block needed already exists in the codebase. Phase 13 is about extending existing code, not writing new infrastructure.

## Common Pitfalls

### Pitfall 1: Schema/Generator Desync on Field Addition
**What goes wrong:** Adding new fields to the schema but not the generator (or vice versa) causes validation failure. Since `additionalProperties: false` is set, the generator output will be rejected if it contains fields not in the schema, OR if the schema requires fields the generator doesn't produce.
**Why it happens:** Schema and generator are separate files that must stay in sync.
**How to avoid:** Update schema, generator, and validator as a single atomic unit. Test by running the generator after all three are updated. The existing `jsonschema.validate()` call in the generator will catch any desync.
**Warning signs:** Generator exits with "Manifest validation failed" error.

### Pitfall 2: Hook Timeout on Cold jsonschema Import
**What goes wrong:** The PostToolUse hook triggers the generator, which imports jsonschema. On first import in a session, jsonschema takes ~200-300ms to load. Combined with PyYAML import and file I/O, total time can approach 500ms.
**Why it happens:** Python module import caching only helps within the same process. Each hook invocation is a new process.
**How to avoid:** The 3000ms timeout provides 6x headroom. Do NOT reduce the timeout below 2000ms. Monitor for timeout warnings in hook output.
**Warning signs:** Intermittent "hook timed out" messages in stderr.

### Pitfall 3: Frontmatter Retrofit Date Bootstrapping
**What goes wrong:** All 26 existing research documents need `created`, `last_verified`, `juce_version` added. The `created` date is historical -- it should reflect when the document was actually written, not when the frontmatter was added.
**Why it happens:** Frontmatter is being added retroactively, so the "created" date requires looking up git history.
**How to avoid:** For the initial retrofit: use `git log --follow --format=%aI -- <file> | tail -1` to get the first commit date for each file. For `last_verified`, use today's date (the retrofit is a verification act). For `juce_version`, use "8.0.4" (current JUCE version at time of verification).
**Warning signs:** All documents having identical `created` dates (means someone used today's date instead of historical dates).

### Pitfall 4: Race Condition in Hook-Triggered Regeneration
**What goes wrong:** If an agent writes multiple research files in rapid succession, multiple hook invocations trigger the generator concurrently, potentially corrupting the manifest JSON.
**Why it happens:** PostToolUse hooks fire per-tool-call, and agent workflows may write multiple files.
**How to avoid:** The generator uses Python's `json.dump()` which is atomic-write at the OS level for small files (the manifest is ~15KB). However, for extra safety, use a write-to-temp-then-rename pattern in the generator:
```python
import tempfile
with tempfile.NamedTemporaryFile(mode='w', dir=OUTPUT_PATH.parent, delete=False, suffix='.tmp') as f:
    json.dump(manifest, f, indent=2, ensure_ascii=False)
    temp_path = f.name
os.replace(temp_path, OUTPUT_PATH)
```
**Warning signs:** Truncated or malformed `resource-index.json` after multiple rapid writes.

### Pitfall 5: YAML Date Type Auto-Parsing
**What goes wrong:** PyYAML's `safe_load()` automatically parses `YYYY-MM-DD` strings as `datetime.date` objects, not strings. If code expects string comparison or string concatenation, it fails.
**Why it happens:** YAML spec says `2026-02-06` without quotes is a date, not a string.
**How to avoid:** In frontmatter, dates CAN be unquoted (YAML handles them as date objects). In the validator and generator, handle both `str` and `datetime.date` types:
```python
if isinstance(value, date):
    date_str = value.isoformat()
elif isinstance(value, str):
    date_str = value
```
Alternatively, require quotes in frontmatter (`"2026-02-06"`) to force string type. **Recommendation:** Allow unquoted (more natural for authors) and handle both types in code.
**Warning signs:** `TypeError: '<' not supported between instances of 'datetime.date' and 'str'`.

### Pitfall 6: Agent Frontmatter Auto-Population Scope
**What goes wrong:** The CONTEXT.md says agents that create research docs must auto-populate frontmatter. But the agents that create research docs (deep-research, gsd-phase-researcher) write to `research/` only occasionally, and their templates don't include frontmatter.
**Why it happens:** These agents' instructions were written before the frontmatter system existed (Phase 10).
**How to avoid:** Update agent instructions for `research-planning-agent.md` and the deep-research skill's report template to include frontmatter. The GSD phase researcher writes to `.planning/phases/` not `research/`, so it is NOT in scope (the frontmatter validator already skips non-research paths). Only agents that write to `research/` need updating.
**Warning signs:** New research documents appearing in `research/` without frontmatter, triggering the frontmatter issues file.

## Code Examples

### Extended Schema (resource-index.schema.json additions)
```json
{
  "$defs": {
    "DocumentEntry": {
      "type": "object",
      "required": ["path", "title", "summary", "domain", "type", "keywords", "stages", "agents", "created", "last_verified", "juce_version"],
      "properties": {
        "created": {
          "type": "string",
          "pattern": "^\\d{4}-\\d{2}-\\d{2}$",
          "description": "Date document was originally created (YYYY-MM-DD)"
        },
        "last_verified": {
          "type": "string",
          "pattern": "^\\d{4}-\\d{2}-\\d{2}$",
          "description": "Date content was last verified against current JUCE version (YYYY-MM-DD)"
        },
        "juce_version": {
          "type": "string",
          "pattern": "^\\d+\\.\\d+\\.\\d+$",
          "description": "JUCE version content was last verified against (e.g., 8.0.4)"
        }
      }
    }
  }
}
```

**Note:** Dates are stored as strings (not JSON date format) because YAML frontmatter dates parsed by PyYAML may be `datetime.date` objects. The generator must `.isoformat()` them before writing to JSON.

### Extended Frontmatter Validator (additions to validate-research-frontmatter.py)
```python
from datetime import date

# Add to REQUIRED_FIELDS
REQUIRED_FIELDS = {"title", "summary", "domain", "type", "keywords", "stages", "agents",
                   "created", "last_verified", "juce_version"}

# Add to validate_frontmatter() function
def validate_date_field(fm, field_name, errors):
    """Validate a date field (created or last_verified)."""
    value = fm.get(field_name)
    if value is None:
        return  # Missing field caught by required check
    if isinstance(value, date):
        return  # PyYAML parsed as date object -- valid
    if isinstance(value, str):
        try:
            date.fromisoformat(value)
        except ValueError:
            errors.append(f"Invalid {field_name} date '{value}' (must be YYYY-MM-DD)")
    else:
        errors.append(f"'{field_name}' must be a date (YYYY-MM-DD)")

def validate_juce_version(fm, errors):
    """Validate juce_version field."""
    value = fm.get("juce_version")
    if value is None:
        return
    value_str = str(value)
    if not re.match(r'^\d+\.\d+\.\d+$', value_str):
        errors.append(f"Invalid juce_version '{value}' (must be semver like 8.0.4)")
```

### Extended Generator (additions to generate-resource-index.py)
```python
from datetime import date

def build_document_entry(file_path, frontmatter):
    """Build a document entry dict from file path and parsed frontmatter."""
    rel_path = file_path.relative_to(PROJECT_ROOT)
    path_str = str(rel_path).replace("\\", "/")

    # Handle PyYAML date objects -> ISO string
    created = frontmatter["created"]
    if isinstance(created, date):
        created = created.isoformat()

    last_verified = frontmatter["last_verified"]
    if isinstance(last_verified, date):
        last_verified = last_verified.isoformat()

    juce_version = str(frontmatter["juce_version"])

    return {
        "path": path_str,
        "title": frontmatter["title"],
        "created": created,
        "last_verified": last_verified,
        "juce_version": juce_version,
        "summary": frontmatter["summary"],
        "domain": frontmatter["domain"],
        "type": frontmatter["type"],
        "keywords": frontmatter["keywords"],
        "stages": frontmatter["stages"],
        "agents": frontmatter["agents"],
    }
```

### Staleness Warning in inject-context.py
```python
from datetime import date

STALENESS_THRESHOLD_DAYS = 90

def format_resource_with_staleness(result, doc_metadata):
    """Format a primary resource entry, adding staleness annotation if needed."""
    title_line = f"**{result['title']}** (relevance: {result['relevance']})"

    # Check staleness
    last_verified = doc_metadata.get("last_verified")
    if last_verified:
        try:
            if isinstance(last_verified, str):
                verified_date = date.fromisoformat(last_verified)
            else:
                verified_date = last_verified
            days_old = (date.today() - verified_date).days
            if days_old > STALENESS_THRESHOLD_DAYS:
                title_line += f" [STALE: verified {days_old} days ago on {verified_date.isoformat()}]"
                # Also warn to stderr for orchestrator visibility
                print(
                    f"Warning: Stale resource injected: {result['path']} "
                    f"(last verified {days_old} days ago)",
                    file=sys.stderr
                )
        except (ValueError, TypeError):
            pass

    return title_line
```

### Batch Verification Script (verify-freshness.py)
```python
#!/usr/bin/env python3
"""
Batch verification utility for research document freshness.

Updates last_verified and juce_version in frontmatter for specified files
or all research documents.

Usage:
    # Verify all research documents
    python3 .claude/scripts/verify-freshness.py --all

    # Verify specific files
    python3 .claude/scripts/verify-freshness.py research/fft-processing-best-practices.md

    # Check which docs are stale (dry run)
    python3 .claude/scripts/verify-freshness.py --check
"""

import argparse
import re
import sys
from datetime import date
from pathlib import Path

import yaml

JUCE_VERSION = "8.0.4"
STALENESS_DAYS = 90

def update_verification(filepath, juce_version=JUCE_VERSION):
    """Update last_verified and juce_version in a file's frontmatter."""
    content = filepath.read_text(encoding="utf-8")
    match = re.match(r'^(---\n)(.*?)(\n---)', content, re.DOTALL)
    if not match:
        print(f"  Skip: {filepath} (no frontmatter)")
        return False

    fm_text = match.group(2)
    fm = yaml.safe_load(fm_text)

    today = date.today().isoformat()
    fm["last_verified"] = today
    fm["juce_version"] = juce_version

    # Rebuild frontmatter preserving field order
    new_fm = yaml.dump(fm, default_flow_style=False, sort_keys=False).rstrip()
    new_content = f"---\n{new_fm}\n---{content[match.end():]}"
    filepath.write_text(new_content, encoding="utf-8")
    print(f"  Updated: {filepath} -> last_verified={today}, juce_version={juce_version}")
    return True
```

### Frontmatter Template for Agent Auto-Population
```yaml
---
title: "[Document Title]"
created: YYYY-MM-DD
last_verified: YYYY-MM-DD
juce_version: "8.0.4"
summary: "[2-3 sentence summary sufficient for relevance ranking]"
domain: dsp  # One of: dsp, ui, build, workflow
type: guide  # One of: algorithm, pattern, guide, reference
keywords:
  - keyword-one
  - keyword-two
  - keyword-three
stages: [0, 2]  # Which pipeline stages (0-4) this is relevant to
agents: [dsp]   # Which agent roles benefit: dsp, ui, build, research
---
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual `generate-resource-index.py` runs | Hook-triggered auto-regeneration | Phase 13 (this phase) | Manifest stays up-to-date automatically |
| No freshness tracking | `last_verified` + `juce_version` fields | Phase 13 (this phase) | Stale research flagged before injection |
| 7 required frontmatter fields | 10 required frontmatter fields | Phase 13 (this phase) | Temporal metadata enables staleness tracking |
| Generator crash = no manifest | Silent degradation + empty results | Phase 13 (this phase) | Agents never blocked by infrastructure failures |

**Not changing in this phase:**
- Discovery scoring algorithm (stage/role/keyword weights stay 0.4/0.35/0.25)
- Token budget management (stays at 4000 tokens)
- Agent-to-role mapping (stays in discover-resources.py)
- Schema file location (stays at `.claude/resource-index.schema.json`)

## Open Questions

1. **Which agents actually write to `research/`?**
   - What we know: `research-planning-agent` creates architecture docs in `plugins/[Plugin]/.planning/`, NOT in `research/`. The deep-research skill is read-only (explicitly stated in its SKILL.md). The `/research` command may produce output to `research/` but this is user-initiated.
   - What's unclear: Do any automated agents actually create files in `research/`? The user decision says agents "must auto-populate valid frontmatter" but the agents I examined don't write to `research/`.
   - Recommendation: Identify which agents/skills/commands write to `research/` and update only those. If none currently do, the requirement is forward-looking -- document the template so future agents use it.

2. **How to determine `created` date for existing documents?**
   - What we know: Git history can provide first-commit dates via `git log --follow --format=%aI -- <file> | tail -1`.
   - What's unclear: Some docs may have been bulk-committed (e.g., stutter-effects docs may all have the same commit date). Whether the user wants exact dates or approximate dates.
   - Recommendation: Use git first-commit dates. Where multiple files share a commit, that's acceptable -- the `created` date is approximate and informational, not used for staleness calculations.

3. **Should `juce_version` in frontmatter be a quoted string or unquoted?**
   - What we know: YAML treats `8.0.4` as a string (not a number) because it has two dots. `yaml.safe_load("8.0.4")` returns `"8.0.4"` (string).
   - Recommendation: Use quoted strings (`"8.0.4"`) for explicitness and consistency, even though unquoted works. The validator should accept both.

4. **What about `research/plugin-development-without-juce.md` (untracked)?**
   - What we know: This file exists in the working tree but is untracked (`??` in git status). It has no frontmatter.
   - Recommendation: Either add frontmatter and track it, or exclude it from the manifest. The generator already skips files without valid frontmatter, so it will be naturally excluded until frontmatter is added.

## Sources

### Primary (HIGH confidence)
- Direct codebase audit of all Phase 10 artifacts (generate-resource-index.py, discover-resources.py, inject-context.py, validate-research-frontmatter.py, resource-index.json, resource-index.schema.json)
- Direct codebase audit of hooks.json, PostToolUse.sh, SubagentStop.sh
- Phase 10 RESEARCH.md and VERIFICATION.md (comprehensive documentation of existing system)
- All 26 research document frontmatter blocks examined
- Python 3.14.2 and PyYAML 6.0.3 and jsonschema 4.26.0 confirmed installed

### Secondary (MEDIUM confidence)
- [Claude Code hooks documentation](https://code.claude.com/docs/en/hooks) - PostToolUse input format (tool_name, tool_input.file_path) confirmed via official docs
- Deep-research SKILL.md - confirmed read-only, does not write to research/
- Agent role definitions from all 11 agent files

### Tertiary (LOW confidence)
- None -- all findings verified against codebase

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All libraries already installed and in use by Phase 10 artifacts
- Architecture: HIGH - Patterns derived from existing codebase conventions, hook system verified via official docs
- Pitfalls: HIGH - All identified from direct code audit (schema desync, YAML date types, hook timeouts, race conditions)
- Graceful degradation: HIGH - Existing code already implements most patterns; gaps identified with specific line references

**Research date:** 2026-02-06
**Valid until:** 2026-03-08 (30 days -- stable domain, no fast-moving dependencies)
