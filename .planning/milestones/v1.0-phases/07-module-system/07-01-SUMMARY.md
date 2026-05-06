---
phase: 07-module-system
plan: 01
subsystem: state-management
tags: [json-schema, registry, modules, migration, semver]

dependency-graph:
  requires:
    - 02-state-management (registry.json foundation)
    - 05-gate-infrastructure (ISO 8601 datetime pattern)
  provides:
    - registry.schema.json v3.0.0 with ModuleEntry and InstalledModule
    - dependencies.schema.json for per-plugin declarations
    - registry.json migrated with modules section
  affects:
    - 07-02 (module commands will read/write this structure)
    - 07-03 (update workflow depends on version tracking)

tech-stack:
  added: []
  patterns:
    - ModuleEntry for central module metadata tracking
    - InstalledModule for per-plugin version and hash tracking
    - Content hash pattern sha256:[hex] for customization detection
    - Atomic write pattern for registry updates

file-tracking:
  key-files:
    created:
      - .planning/workflow/schemas/dependencies.schema.json
      - modules/scripts/migrate-registry.py
    modified:
      - .planning/workflow/schemas/registry.schema.json
      - .planning/workflow/registry.json

decisions:
  - ModuleEntry requires: version, path, category, description, dependents, lastUpdated, usageStats
  - InstalledModule requires: name, version, installedAt, modified, contentHash
  - Content hash format: sha256:[hex] (variable length, typically 16 chars truncated)
  - Empty InstalledModule array on migration (actual installs populate later)

metrics:
  duration: 3 minutes
  completed: 2026-02-01
---

# Phase 7 Plan 1: Registry Schema Extension Summary

**One-liner:** Extended registry to v3.0.0 with ModuleEntry/InstalledModule schemas and migrated 8 modules from YAML

## Outcome

All 3 tasks completed successfully. The registry schema now supports full module dependency tracking with version management and customization detection via content hashing.

## What Was Built

### 1. Registry Schema v3.0.0

Extended `.planning/workflow/schemas/registry.schema.json` with:

- **ModuleEntry** definition with 10 properties:
  - version, path, category, description (core metadata)
  - author, changelogUrl, compatibilityNotes (optional metadata)
  - dependents array (which plugins use this module)
  - lastUpdated (ISO 8601 timestamp)
  - usageStats (addCount, removeCount for analytics)

- **InstalledModule** definition with 7 properties:
  - name, version, installedAt (core tracking)
  - updatedAt (optional, for upgrade tracking)
  - modified (boolean for customization detection)
  - contentHash, originalHash (sha256 for diff detection)

- **modules** added as required top-level property

### 2. Dependencies Schema

Created `.planning/workflow/schemas/dependencies.schema.json` for per-plugin forward declarations:

- modules array with name, version, purpose
- ouariconModules array (module names without version)
- juceModules array (JUCE module dependencies)

### 3. Migration Script

Created `modules/scripts/migrate-registry.py`:

- Reads modules/registry.yaml and extracts module metadata
- Builds dependents arrays from used_by data
- Calculates usageStats from dependent count
- Converts plugin modules from string array to empty InstalledModule array
- Uses atomic write pattern (temp file + rename)

### 4. Migrated Registry

Updated `.planning/workflow/registry.json`:

- Version bumped from 2.0.0 to 3.0.0
- Added modules section with 8 entries
- Module dependents correctly populated:
  - preset-manager: OuariconMarimba, OuariconTremolo
  - vu-meter: OuariconComp, OuariconAnalogEQ
  - scala-tuning-engine, analog-eq-unit, compressor-unit, playable-keyboard: OuariconMarimba

## Technical Details

### Schema Patterns Used

- ISO 8601 datetime pattern from Phase 5: `^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d+)?Z?$`
- Semver pattern with prerelease: `^\d+\.\d+\.\d+(-[a-zA-Z0-9.]+)?$`
- Content hash pattern: `^sha256:[a-f0-9]+$`

### Migration Approach

The migration preserves existing plugin data while adding the modules section. Plugin modules arrays are reset to empty because the actual module installations (with content hashes) will be tracked when `/module:add` is used.

## Deviations from Plan

None - plan executed exactly as written.

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| Empty InstalledModule array on migration | Actual content hashes require file scanning; will be populated on explicit /module:add |
| 16-char truncated hash in examples | Full SHA-256 is 64 chars; pattern allows any length for flexibility |
| usageStats.addCount from dependents.length | Historical data unavailable; count current dependents as baseline |

## Verification Results

All checks passed:

1. registry.schema.json validates with ModuleEntry and InstalledModule definitions
2. dependencies.schema.json validates with ouariconModules array
3. registry.json is version 3.0.0 with 8 modules
4. All module dependents arrays correctly populated
5. Plugin modules arrays are InstalledModule objects (currently empty)

## Next Phase Readiness

Ready for 07-02: Module Commands Implementation

- Schema is in place for `/module:add`, `/module:remove`, `/module:upgrade`
- dependents tracking enables safe removal checks
- contentHash pattern ready for customization detection
- usageStats ready for analytics tracking
