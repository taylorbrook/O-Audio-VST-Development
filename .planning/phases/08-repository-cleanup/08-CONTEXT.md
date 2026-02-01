# Phase 8: Repository Cleanup - Context

**Gathered:** 2026-02-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Reduce repository size from ~584MB to under 100MB by purging binary artifacts from git history. Establish comprehensive .gitignore to prevent re-accumulation. Maintain permanent backup branch for full recovery. This is infrastructure cleanup only — no plugin renaming or workflow changes.

</domain>

<decisions>
## Implementation Decisions

### Cleanup scope
- Binary artifacts only: .o, .a, .dylib, build outputs
- Preserve all source code history intact
- Remove `backups/` folder entirely (both history and working tree)
- Protected paths determined by Claude based on what's essential

### Safety & recovery
- Pre-cleanup backup branch kept permanently (no expiration)
- Selective recovery script: cherry-pick specific files/commits from backup branch
- Dry-run required before execution — show files/sizes, require approval
- Single developer environment — no team coordination needed

### Naming convention
- Existing plugins keep their current names (no renaming during cleanup)
- New plugins going forward: `O-PluginName` prefix convention
- Convention documentation/enforcement at Claude's discretion

### Post-cleanup workflow
- Comprehensive .gitignore update (all binary patterns, build outputs, .DS_Store)
- No pre-commit hook — .gitignore is sufficient protection
- Full verification: size report, clone test, build test, CI pass confirmation
- GitHub Actions cache must be cleared after force-push

### Claude's Discretion
- Which paths to protect from cleanup (essential files)
- Whether to document naming convention in CONTRIBUTING.md
- Exact .gitignore patterns beyond the obvious binary types
- Recovery script implementation details

</decisions>

<specifics>
## Specific Ideas

- "Fast recovery" matters less than "selective recovery" — want to pull specific things back if needed
- Dry-run approval is non-negotiable — need to see what's happening before it happens
- Trust the comprehensive .gitignore over hooks for ongoing protection

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 08-repository-cleanup*
*Context gathered: 2026-02-01*
