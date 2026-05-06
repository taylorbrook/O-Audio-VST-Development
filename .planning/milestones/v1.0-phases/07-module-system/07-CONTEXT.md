# Phase 7: Module System - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

<domain>
## Phase Boundary

Fix module add/remove/update reliability with tracked dependencies. Commands work consistently across all plugins. Registry tracks which plugins depend on which modules with version information. Manual rebuild documentation exists for edge cases.

</domain>

<decisions>
## Implementation Decisions

### Command behavior
- Partial state + error on failure: leave what succeeded, report what failed, let user fix
- Module removal keeps code in plugin, just stops future updates from propagating
- Verbose output by default: show each step as it happens
- Auto-rebuild plugin after /module:add completes

### Dependency resolution
- Each plugin gets its own version of a module (no forced single version)
- Warn on known conflicts between modules, but proceed unless blocked
- Notify on plugin focus when module updates are available
- Semver versioning (major.minor.patch) for all modules

### Registry structure
- Central registry only (.planning/workflow/registry.json)
- Full information per module: name, version, source path, dependents, description, author, changelog link, compatibility notes, install date, last updated, usage stats
- Strict JSON Schema validation on every registry read/write
- Prompt user on missing/corrupted entries (no auto-repair)

### Update workflow
- Per-plugin choice when module is updated (ask for each plugin)
- Offer rollback if update causes build failure
- /module:upgrade-all exists with preview before proceeding
- Warn on local customizations, give choice to keep or overwrite

### Per-plugin variation
- Modified flag in registry tracks plugins with local customizations
- No aliasing — module name is fixed everywhere
- Module listing shows installed version + update status

### Claude's Discretion
- CMake integration approach (explicit declaration vs. generated from registry)
- Exact diff detection algorithm for customization tracking
- Registry schema field details

</decisions>

<specifics>
## Specific Ideas

- Removal is "soft" — code stays in plugin, removal just stops update propagation
- Think of modules as "copied and tracked" rather than "linked"
- Update flow should feel conversational: "OuariconSmoothing has local changes. Keep your version or update?"

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 07-module-system*
*Context gathered: 2026-01-31*
