# Phase 14: Platform Alignment - Context

**Gathered:** 2026-02-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Update all Plugin Freedom System agents, skills, and scripts to run on Opus 4.6 without deprecation warnings, broken paths, or API errors. Replace binary Sonnet/Opus model switching with effort-tuned profiles. Canary-test after changes to protect 35+ production plugins.

</domain>

<decisions>
## Implementation Decisions

### Effort Level Mapping
- 4-tier system: max / high / medium / low
- All agents stay on Opus 4.6 — effort level is the only tuning knob, no model switching
- Central config file (e.g., agent-profiles.json) with per-agent override capability
- Max-tier agents (DSP, research-planning) always run at max — no complexity-adaptive scaling
- Remove the binary Sonnet/Opus model selection infrastructure entirely (don't rewire, delete it)

### Migration Strategy
- Audit-then-fix: first pass catalogs all deprecation issues (thinking config, stale paths, prefilled messages, model selection), second pass fixes them in batches
- After all changes, verify with canary test

### Canary Testing Scope
- Primary canary: O-SimpleReverb (build + validate)
- Secondary spot-check: one WebView-based plugin (e.g., O-AnalogEQ) to verify WebView paths still work
- Both canaries run after all Phase 14 changes are complete

### Stale Path Handling
- Migrate content from .ideas/ and .continue-here.md into each plugin's own .planning/ folder before removing references
- Audit for other stale paths/deprecated conventions beyond .ideas/ and .continue-here.md — discover as part of the Phase 14 audit pass
- Update all script references to use only .planning/ paths

### Claude's Discretion
- Canary test cadence (after each fix category vs. only at end) — based on risk assessment
- Whether to create a reusable canary-test.sh script or run ad-hoc commands
- Failure policy (revert vs. debug in place) — based on severity
- Whether to add .gitignore rules to prevent stale paths from reappearing
- Audit output format (structured AUDIT.md vs. direct feed into plan)

</decisions>

<specifics>
## Specific Ideas

- Roadmap constraint P40: canary plugin testing (O-SimpleReverb) after EVERY change
- The effort profile central config should be easy for the user to adjust — one file to see all agent effort levels at a glance
- WebView plugin spot-check ensures the more complex plugin type still works after system changes

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 14-platform-alignment*
*Context gathered: 2026-02-08*
