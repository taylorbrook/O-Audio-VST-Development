# Quick Task 260408-vzl: Fix Tuning Module Integration - Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Task Boundary

The tuning module isn't working well when integrated into new VSTs. O-Prism's tuning tab/module is fully functioning. Compare O-Prism's integration with the module to identify what the module is missing. Make the module adapt to different sized VST instruments without breaking the general layout.

</domain>

<decisions>
## Implementation Decisions

### Sizing Strategy
- Use CSS container queries so the module detects its container size and adjusts layout/font/spacing automatically
- Most portable approach — works in any host container without coordination

### Missing Features Scope
- Match O-Prism only — find what's different between O-Prism's working integration and the module, fix those gaps
- No new features or API refactoring

### Layout Constraints
- Module fills 100% of whatever container the host gives it
- Host plugin controls the size; module is fully responsive within that space

### Claude's Discretion
- Implementation details of container query breakpoints (thresholds, what changes at each size)
- Order of fixes if multiple gaps are found

</decisions>

<specifics>
## Specific Ideas

- O-Prism's tuning tab is the reference implementation — diff its integration code against the module's public API
- Container queries should handle font sizes, spacing, and potentially layout shifts (e.g., stacking vs side-by-side elements)

</specifics>
