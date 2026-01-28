# Phase 6: Formats & Integration - Context

**Gathered:** 2026-01-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Plugin builds in all required formats (VST3, AU) with OuariconPresetManager preset system. Factory presets demonstrate both modes across instrument and mix sources. All formats pass validation (pluginval, auval) and load correctly in target DAWs.

</domain>

<decisions>
## Implementation Decisions

### Factory Presets
- 8-12 factory presets total
- Target both bass instruments (bass guitar, synth, 808s) AND full mix enhancement
- Descriptive naming style ("Warm Bass Guitar", "Subtle Mix Glue", "Punchy 808")
- Full intensity range (10-90% Enhance) to demonstrate capabilities
- Include both subtle and aggressive presets

### Preset Organization
- Flat list, alphabetically sorted (no folders/categories)
- Mode (Clean/Colored) NOT included in preset name — user sees mode in UI after loading
- Include a "Default" init preset with neutral starting point (50% enhance, Clean mode, ~100Hz crossover)

### Format Priorities
- AU and VST3 equally important
- Skip Standalone — only plugin formats needed
- Identical behavior across formats (no format-specific tuning)
- Pluginval validation required: must pass at strictness level 5+

### Validation Workflow
- DAW testing: Logic Pro for AU, any other DAW (Ableton/Reaper) for VST3
- Full parameter verification: all controls work, automation records/plays back, presets load
- Preset validation: smoke test each preset loads without crash
- auval validation required: must pass `auval -v aumu`

### Claude's Discretion
- Exact preset parameter values
- Pluginval strictness level (5 minimum, higher if passes)
- Order of validation steps
- Which secondary DAW for VST3 testing

</decisions>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches for preset naming and organization.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 06-formats-integration*
*Context gathered: 2026-01-25*
