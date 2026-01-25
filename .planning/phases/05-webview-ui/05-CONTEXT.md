# Phase 5: WebView UI - Context

**Gathered:** 2026-01-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Visual interface for O-Bass exposing 4 controls (Frequency, Enhance, Output, Mode toggle) using WebView technology. UI matches Ouaricon suite visual language with paper texture and botanical illustration style. Parameter binding and automation reflection are in scope; presets and format builds are Phase 6.

</domain>

<decisions>
## Implementation Decisions

### Control layout
- Grid 2x2 arrangement
- Top row: Frequency + Enhance (signal processing controls)
- Bottom row: Output + Mode (gain staging and character)
- Header area at top with O-Bass name and Ouaricon logo
- Medium window size (~500x450px) for comfortable spacing

### Knob visual style
- Illustrated/botanical style consistent with Ouaricon aesthetic
- Pointer line indicator showing current position
- Value displayed below knob, always visible (e.g., "80 Hz", "50%")
- Labels above each knob (FREQUENCY, ENHANCE, OUTPUT)

### Mode toggle design
- Illustrated toggle matching the botanical knob aesthetic
- Icon/symbol change to distinguish Clean vs Colored modes
- 100ms smooth slide animation when switching
- Position in bottom row: Claude's discretion (visual balance)

### Feedback elements
- LED-style illustrated light for limit indicator (glows when soft-clipping)
- Subtle pulse/glow tied to audio activity when processing
- Subtle highlight on controls when hovering
- Double-click resets knob to default value

### Claude's Discretion
- Toggle position within bottom row (left or right)
- Exact botanical illustration details
- Specific hover highlight treatment
- Activity pulse/glow timing and intensity

</decisions>

<specifics>
## Specific Ideas

- Ouaricon suite visual language: paper texture background, botanical illustrations
- Illustrated controls should feel hand-drawn but functional
- Clean mode icon vs Colored mode icon should visually suggest the character difference

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 05-webview-ui*
*Context gathered: 2026-01-25*
