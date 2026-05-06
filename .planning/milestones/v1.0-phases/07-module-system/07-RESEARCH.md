# Phase 7: Module System - Research

**Researched:** 2026-01-31
**Domain:** Module dependency management, semver versioning, registry design, diff tracking
**Confidence:** HIGH

## Summary

Phase 7 focuses on making the module add/remove/update system reliable with proper dependency tracking. The research investigated JSON Schema validation (aligning with Phase 1/2 patterns), semver comparison, CMake integration approaches, and diff detection algorithms for customization tracking.

The current system has a foundational module infrastructure: `modules/registry.yaml` (module catalog), `modules/cmake/OuariconModules.cmake` (CMake integration), and per-module `module.yaml` definitions. The `/module:add`, `/module:remove`, `/module:upgrade` commands exist but lack the robust dependency tracking, schema validation, and customization detection that the user has specified.

The phase must implement: (1) migrate from `modules/registry.yaml` to central `.planning/workflow/registry.json` with full module metadata, (2) strict JSON Schema validation on every registry read/write (per Phase 1/2 patterns), (3) semver comparison using the `semver` npm package's algorithm (or equivalent Python implementation), (4) customization tracking via content hash comparison, (5) per-plugin update workflow with rollback capability.

**Primary recommendation:** Extend the existing registry schema to include a `modules` section with full metadata (name, version, source path, dependents, description, author, changelog link, compatibility notes, install date, last updated, usage stats). Use content hashing for customization detection rather than line-level diff.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JSON Schema | draft 2020-12 | Schema validation | Matches Phase 1 contracts, strict mode support |
| semver (npm) | 7.x | Version comparison | npm's official semver parser, comprehensive API |
| SHA-256 | builtin | Content hashing | Fast, collision-resistant, standard for integrity |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| PyYAML | 6.x | YAML parsing | Existing scripts use YAML; bridge to JSON |
| js-yaml | 4.x | YAML parsing | If Node.js tooling preferred |
| hashlib (Python) | stdlib | SHA-256 hashing | Content hash for customization detection |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| SHA-256 hash | Line diff | Hash is O(n), diff is O(n^2); hash sufficient for "modified yes/no" |
| semver npm | Manual parsing | npm version battle-tested; handles edge cases |
| JSON registry | YAML registry | JSON has better schema validation tooling; matches Phase 1/2 |

**No npm install needed:** Claude can implement semver comparison logic directly. The patterns are documented; critical operations should match npm/semver behavior.

## Architecture Patterns

### Recommended Registry Structure
```json
// .planning/workflow/registry.json
{
  "$schema": "./schemas/registry.schema.json",
  "version": "3.0.0",
  "focused": "O-IntonationPad",
  "plugins": {
    "O-IntonationPad": {
      "path": "plugins/O-IntonationPad",
      "stage": "4-wavetable",
      "phase": "complete",
      "status": "active",
      "created": "2026-01-28",
      "lastActivity": "2026-01-30T10:15:00Z",
      "modules": [
        {
          "name": "scala-tuning-engine",
          "version": "1.0.0",
          "installedAt": "2026-01-15T09:30:00Z",
          "updatedAt": "2026-01-15T09:30:00Z",
          "modified": false,
          "contentHash": "sha256:abc123..."
        }
      ],
      "expressMode": false,
      "blockedBy": null
    }
  },
  "modules": {
    "scala-tuning-engine": {
      "version": "1.0.0",
      "path": "modules/tuning/scala-tuning-engine",
      "category": "tuning",
      "description": "Complete microtonal tuning system",
      "author": "Ouaricon Audio",
      "changelogUrl": "modules/tuning/scala-tuning-engine/CHANGELOG.md",
      "compatibilityNotes": "Requires juce_core",
      "dependents": ["O-IntonationPad"],
      "lastUpdated": "2026-01-12T00:00:00Z",
      "usageStats": {
        "addCount": 1,
        "removeCount": 0
      }
    }
  }
}
```

### Pattern 1: Central Registry with Module Metadata
**What:** All module metadata lives in `.planning/workflow/registry.json` under a `modules` section
**When to use:** Always - single source of truth for module versions and dependents
**Example:**
```json
// Source: User decision (07-CONTEXT.md) - Central registry only
{
  "modules": {
    "preset-manager": {
      "version": "1.0.0",
      "path": "modules/persistence/preset-manager",
      "category": "persistence",
      "description": "JSON-based preset persistence with factory/user separation",
      "author": "Ouaricon Audio",
      "changelogUrl": "modules/persistence/preset-manager/CHANGELOG.md",
      "compatibilityNotes": null,
      "dependents": ["O-Marimba", "O-Tremolo"],
      "lastUpdated": "2026-01-14T00:00:00Z",
      "usageStats": {
        "addCount": 2,
        "removeCount": 0
      }
    }
  }
}
```

### Pattern 2: Per-Plugin Module Entry with Content Hash
**What:** Each plugin's modules array tracks installed version AND content hash for customization detection
**When to use:** Every module installation
**Example:**
```json
// Source: User decision - Modified flag tracks local customizations
{
  "modules": [
    {
      "name": "vu-meter",
      "version": "1.0.0",
      "installedAt": "2026-01-20T14:00:00Z",
      "updatedAt": "2026-01-20T14:00:00Z",
      "modified": true,
      "contentHash": "sha256:def456...",
      "originalHash": "sha256:abc123..."
    }
  ]
}
```

### Pattern 3: CMake Explicit Declaration (Recommended)
**What:** Modules declared explicitly in CMakeLists.txt via `ouaricon_add_module()` calls
**When to use:** Always (Claude's discretion recommendation)
**Rationale:**
- CMake is the build system authority; registry tracks metadata only
- Explicit declarations are auditable and Git-diffable
- No magic generation step that could fail silently
- Existing pattern already works this way
**Example:**
```cmake
# plugins/O-IntonationPad/CMakeLists.txt
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Module dependencies - explicit declaration
ouaricon_add_module(${PROJECT_NAME} scala-tuning-engine)
ouaricon_add_module(${PROJECT_NAME} playable-keyboard)
```

### Pattern 4: Content Hash for Customization Detection
**What:** SHA-256 hash of module files stored on install; compare on update
**When to use:** Every module add and update check
**Algorithm:**
```python
# Source: Claude's discretion recommendation
import hashlib
from pathlib import Path

def compute_module_hash(module_path: Path) -> str:
    """Compute SHA-256 hash of all module source files."""
    hasher = hashlib.sha256()

    # Sort for deterministic ordering
    for file_path in sorted(module_path.rglob("*")):
        if file_path.is_file() and not file_path.name.startswith('.'):
            hasher.update(file_path.read_bytes())

    return f"sha256:{hasher.hexdigest()[:16]}"

def is_modified(installed_hash: str, original_hash: str) -> bool:
    """Check if plugin's copy differs from original."""
    return installed_hash != original_hash
```

### Pattern 5: Semver Comparison Logic
**What:** Full semver comparison including major/minor/patch and prerelease
**When to use:** Version compatibility checks, update detection
**Algorithm:**
```python
# Source: npm/node-semver semantics
import re
from typing import Tuple, Optional

def parse_version(v: str) -> Tuple[int, int, int, Optional[str]]:
    """Parse semver string into (major, minor, patch, prerelease)."""
    match = re.match(r'^(\d+)\.(\d+)\.(\d+)(?:-(.+))?$', v)
    if not match:
        raise ValueError(f"Invalid semver: {v}")
    major, minor, patch = int(match[1]), int(match[2]), int(match[3])
    prerelease = match[4]
    return (major, minor, patch, prerelease)

def compare_versions(v1: str, v2: str) -> int:
    """Compare semver versions. Returns -1, 0, or 1."""
    p1, p2 = parse_version(v1), parse_version(v2)

    # Compare major.minor.patch
    for i in range(3):
        if p1[i] < p2[i]:
            return -1
        if p1[i] > p2[i]:
            return 1

    # Prerelease handling: version without prerelease > version with prerelease
    if p1[3] is None and p2[3] is not None:
        return 1
    if p1[3] is not None and p2[3] is None:
        return -1
    if p1[3] is not None and p2[3] is not None:
        return -1 if p1[3] < p2[3] else (1 if p1[3] > p2[3] else 0)

    return 0

def is_major_update(old: str, new: str) -> bool:
    """Check if update is a breaking change."""
    p_old, p_new = parse_version(old), parse_version(new)
    return p_new[0] > p_old[0]

def satisfies_range(version: str, range_spec: str) -> bool:
    """Check if version satisfies a range (^1.2.0, ~1.2.0, >=1.0.0)."""
    # Caret (^): Allow minor and patch updates
    if range_spec.startswith('^'):
        base = parse_version(range_spec[1:])
        v = parse_version(version)
        return v[0] == base[0] and (v[1], v[2]) >= (base[1], base[2])

    # Tilde (~): Allow patch updates only
    if range_spec.startswith('~'):
        base = parse_version(range_spec[1:])
        v = parse_version(version)
        return v[0] == base[0] and v[1] == base[1] and v[2] >= base[2]

    # Exact match
    return version == range_spec
```

### Anti-Patterns to Avoid
- **Auto-repair without prompting:** User decided no silent auto-repair; always prompt
- **Force single version:** User decided each plugin gets its own version; don't force alignment
- **Silent removal:** Module removal keeps code in plugin; only stops update propagation
- **Terse output:** Verbose output by default; show each step as it happens
- **Generated CMakeLists.txt:** Use explicit declaration; generated files hide state

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Semver comparison | Simple string split | Full semver logic | Prerelease, build metadata, edge cases |
| Content hashing | MD5 | SHA-256 | Collision resistance, standard practice |
| JSON Schema validation | Manual field checks | JSON Schema 2020-12 | Matches Phase 1, actionable errors |
| Atomic registry writes | Direct file write | write-file-atomic pattern | Corruption prevention (Phase 2 pattern) |
| Module conflict detection | Ad-hoc checks | Predefined conflict map | Known incompatibilities should be documented |

**Key insight:** The module system is dependency management at a smaller scale than npm/cargo, but the same patterns apply: content integrity (hashes), version semantics (semver), atomic state updates, and explicit over implicit.

## Common Pitfalls

### Pitfall 1: Registry Corruption During Partial Operations
**What goes wrong:** /module:add fails midway; registry shows module added but CMakeLists.txt not updated
**Why it happens:** Multi-step operations without atomic transaction
**How to avoid:** User decision is "partial state + error on failure"; leave what succeeded, report what failed clearly
**Warning signs:** Build errors after /module:add "succeeded"

### Pitfall 2: Stale Content Hashes
**What goes wrong:** Module marked as "modified" but user intentionally edited; or marked "clean" but files changed outside system
**Why it happens:** Hashes not updated after manual edits
**How to avoid:** Recompute hash on every operation; show hash mismatch explicitly
**Warning signs:** Update warning when no actual changes made

### Pitfall 3: Breaking Changes Without Warning
**What goes wrong:** Major version update applied; plugin code breaks
**Why it happens:** Treating all version bumps the same
**How to avoid:** Detect major version updates; show changelog prominently; warn explicitly
**Warning signs:** Build failures after /module:upgrade

### Pitfall 4: Lost Customizations on Update
**What goes wrong:** User customized module locally; update overwrites without asking
**Why it happens:** Update didn't check modified flag
**How to avoid:** User decided: "Warn on local customizations, give choice to keep or overwrite"
**Warning signs:** Users losing work after upgrades

### Pitfall 5: Orphaned Module Entries
**What goes wrong:** Plugin deleted but registry still lists it as dependent
**Why it happens:** Deletion didn't clean up module dependents
**How to avoid:** Plugin deletion must update module dependents array; reconcile detects orphans
**Warning signs:** Module shows phantom dependents

### Pitfall 6: Registry Schema Migration Failures
**What goes wrong:** Schema updated to 3.0.0 but old plugins have 2.0.0 format
**Why it happens:** No migration path for existing data
**How to avoid:** Version field in registry; migration script for schema updates
**Warning signs:** Validation errors after system update

## Code Examples

Verified patterns from official sources and project decisions:

### Registry Schema Extension for Modules
```json
// Source: JSON Schema 2020-12 + User decisions (07-CONTEXT.md)
// .planning/workflow/schemas/registry.schema.json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "./registry.schema.json",
  "title": "Plugin Registry with Module Tracking",
  "type": "object",
  "required": ["$schema", "version", "focused", "plugins", "modules"],
  "additionalProperties": false,
  "properties": {
    "$schema": { "type": "string" },
    "version": { "type": "string", "pattern": "^\\d+\\.\\d+\\.\\d+$" },
    "focused": { "type": ["string", "null"] },
    "plugins": {
      "type": "object",
      "additionalProperties": { "$ref": "#/$defs/PluginEntry" }
    },
    "modules": {
      "type": "object",
      "additionalProperties": { "$ref": "#/$defs/ModuleEntry" }
    }
  },
  "$defs": {
    "PluginEntry": {
      "type": "object",
      "required": ["path", "stage", "phase", "status", "created", "modules"],
      "additionalProperties": false,
      "properties": {
        "path": { "type": "string" },
        "stage": { "type": "string" },
        "phase": { "type": ["string", "null"] },
        "status": { "type": "string" },
        "created": { "type": "string", "format": "date" },
        "lastActivity": { "type": "string", "format": "date-time" },
        "modules": {
          "type": "array",
          "items": { "$ref": "#/$defs/InstalledModule" }
        },
        "expressMode": { "type": "boolean", "default": false },
        "blockedBy": { "type": ["string", "null"] }
      }
    },
    "InstalledModule": {
      "type": "object",
      "required": ["name", "version", "installedAt", "modified", "contentHash"],
      "additionalProperties": false,
      "properties": {
        "name": { "type": "string" },
        "version": { "type": "string", "pattern": "^\\d+\\.\\d+\\.\\d+(-[a-zA-Z0-9.]+)?$" },
        "installedAt": { "type": "string", "format": "date-time" },
        "updatedAt": { "type": "string", "format": "date-time" },
        "modified": { "type": "boolean" },
        "contentHash": { "type": "string", "pattern": "^sha256:[a-f0-9]+$" },
        "originalHash": { "type": "string", "pattern": "^sha256:[a-f0-9]+$" }
      }
    },
    "ModuleEntry": {
      "type": "object",
      "required": ["version", "path", "category", "description", "dependents"],
      "additionalProperties": false,
      "properties": {
        "version": { "type": "string", "pattern": "^\\d+\\.\\d+\\.\\d+(-[a-zA-Z0-9.]+)?$" },
        "path": { "type": "string" },
        "category": { "type": "string" },
        "description": { "type": "string" },
        "author": { "type": "string" },
        "changelogUrl": { "type": ["string", "null"] },
        "compatibilityNotes": { "type": ["string", "null"] },
        "dependents": {
          "type": "array",
          "items": { "type": "string" }
        },
        "lastUpdated": { "type": "string", "format": "date-time" },
        "usageStats": {
          "type": "object",
          "properties": {
            "addCount": { "type": "integer", "minimum": 0 },
            "removeCount": { "type": "integer", "minimum": 0 }
          }
        }
      }
    }
  }
}
```

### Module Add Workflow
```markdown
## /module:add [plugin] [module] Workflow

1. **Validate inputs**
   - Check plugin exists in registry
   - Check module exists in modules/ directory
   - Check module not already installed in plugin

2. **Read module metadata**
   - Parse modules/{category}/{module}/module.yaml
   - Extract version, files, dependencies

3. **Compute content hash**
   - Hash all source files in module directory
   - Store as "originalHash" for future comparison

4. **Update CMakeLists.txt**
   - Add `ouaricon_add_module(${PROJECT_NAME} {module})` line
   - Verify include for OuariconModules.cmake exists

5. **Copy JS files** (if present)
   - Copy to plugins/{plugin}/Source/ui/public/modules/
   - Preserve directory structure

6. **Update registry.json**
   - Add entry to plugins.{plugin}.modules array
   - Add plugin to modules.{module}.dependents array
   - Increment modules.{module}.usageStats.addCount

7. **Trigger build**
   - Run: ninja {plugin}_VST3 {plugin}_AU
   - User decision: auto-rebuild after add

8. **Report result**
   - Show each step completed (verbose by default)
   - Show integration instructions (includes, imports)
```

### Module Update Workflow with Customization Check
```markdown
## /module:upgrade [module] Workflow

1. **Check for updates**
   - Compare module.yaml version with installed versions
   - List all plugins using this module

2. **For each dependent plugin** (per-plugin choice):
   a. **Check for customizations**
      - Compute current content hash of installed files
      - Compare with originalHash stored at install time

   b. **If modified:**
      - Show warning: "Local customizations detected"
      - Options: Keep local | Update (lose customizations) | Diff view

   c. **If not modified:**
      - Show changelog since installed version
      - Ask: Update this plugin? [y/n]

3. **Apply update** (if confirmed):
   - Copy new module files
   - Update version in registry
   - Store new contentHash and originalHash
   - Trigger rebuild

4. **On build failure:**
   - Offer rollback to previous version
   - User decision: "Offer rollback if update causes build failure"
```

### Update Notification on Focus
```markdown
## /plugin:focus Update Check

When focusing a plugin, check for module updates:

1. Load plugin's installed modules from registry
2. For each module:
   - Compare installed version with current module version
   - If update available, add to notification list
3. If updates available:
   - Display: "Module updates available:"
   - List: "{module}: {installed} -> {available}"
   - Suggest: "Run /module:upgrade {module} to update"
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| YAML registry (modules/registry.yaml) | JSON registry (.planning/workflow/registry.json) | Phase 7 | Better schema validation, matches Phase 1/2 |
| Simple string comparison | Full semver comparison | Standard practice | Handles prerelease, major/minor/patch semantics |
| No customization tracking | Content hash + modified flag | Phase 7 | Protects user changes from being lost |
| Auto-repair | Prompt user on issues | User decision | Builds trust, prevents surprises |

**Deprecated/outdated:**
- `modules/registry.yaml`: Will be replaced by JSON in `.planning/workflow/`
- `modules.lock.yaml` per-plugin: Replaced by registry.json modules array
- Implicit CMake generation: Use explicit declaration pattern

## Open Questions

Things that couldn't be fully resolved:

1. **Conflict detection between modules**
   - What we know: User wants warning on known conflicts
   - What's unclear: How to define/store known conflicts
   - Recommendation: Add optional `conflicts` array to ModuleEntry; populate based on experience

2. **Module file layout changes on upgrade**
   - What we know: Hash comparison detects content changes
   - What's unclear: How to handle added/removed files in module
   - Recommendation: Hash includes file list; any structural change triggers modified flag

3. **Rollback mechanism implementation**
   - What we know: User wants rollback on build failure
   - What's unclear: How to store previous version for rollback
   - Recommendation: Store previous version files in `.planning/workflow/rollback/{plugin}/{module}/` temporarily

4. **Module creation workflow (/module:create)**
   - What we know: Command exists in docs, extracts from plugin
   - What's unclear: How to handle first-time module extraction versioning
   - Recommendation: Start at 1.0.0 for new modules; require changelog entry

## Sources

### Primary (HIGH confidence)
- [npm/node-semver GitHub](https://github.com/npm/node-semver) - Semver comparison API and semantics
- [Ajv JSON Schema validator GitHub](https://github.com/ajv-validator/ajv) - JSON Schema 2020-12 validation
- Phase 1 RESEARCH.md - JSON Schema patterns for this project
- Phase 2 RESEARCH.md - State management patterns for this project
- 07-CONTEXT.md - User decisions constraining implementation

### Secondary (MEDIUM confidence)
- [JSON Schema official docs](https://json-schema.org/draft/2020-12) - Schema definition standard
- [CMake Using Dependencies Guide](https://cmake.org/cmake/help/latest/guide/using-dependencies/index.html) - CMake dependency patterns
- [Registry Pattern - GeeksforGeeks](https://www.geeksforgeeks.org/system-design/registry-pattern/) - Registry design patterns

### Tertiary (LOW confidence)
- WebSearch results on file diff algorithms - Background on diff approaches
- Existing modules/ codebase - Current implementation patterns

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Using established Phase 1/2 patterns plus npm semver
- Architecture patterns: HIGH - Based on user decisions and existing CMake integration
- Pitfalls: MEDIUM - Derived from dependency management experience
- Diff algorithm recommendation: MEDIUM - Hash is simpler than diff; suitable for yes/no modified check

**Research date:** 2026-01-31
**Valid until:** 2026-03-02 (30 days - module patterns are stable)

---

## Appendix: CMake Integration Recommendation

**Decision:** Explicit declaration (Claude's discretion area)

**Rationale:**
1. **Transparency:** CMakeLists.txt changes are visible in Git diffs
2. **Debuggability:** Build issues can be traced to explicit lines
3. **Existing pattern:** OuariconModules.cmake already works this way
4. **No magic:** No hidden generation step that could fail silently

**Alternative considered:** Generate CMake from registry
- Rejected because: Hidden coupling, harder to debug, more moving parts
- When it would make sense: If managing 50+ modules per plugin (not our case)

## Appendix: Dependencies.schema.json (Forward Declaration from Phase 2)

This schema was forward-declared in Phase 2. Here is the recommended implementation:

```json
// .planning/workflow/schemas/dependencies.schema.json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "./dependencies.schema.json",
  "title": "Plugin Dependencies",
  "description": "Forward dependency declaration for a single plugin",
  "type": "object",
  "required": ["$schema", "modules"],
  "additionalProperties": false,
  "properties": {
    "$schema": {
      "type": "string",
      "description": "Reference to this schema"
    },
    "modules": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["name", "version"],
        "properties": {
          "name": { "type": "string" },
          "version": { "type": "string" },
          "purpose": { "type": "string" }
        }
      }
    },
    "ouariconModules": {
      "type": "array",
      "items": { "type": "string" },
      "description": "Ouaricon module names (without version)"
    },
    "juceModules": {
      "type": "array",
      "items": { "type": "string" },
      "description": "JUCE module names"
    }
  }
}
```
