# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-07-24
**Participants:** User, Claude

## Requirements Confirmed

- **UI-01:** Two-way binding for all 10 parameters — controls drive DSP, host automation/preset changes update UI.
- **UI-02:** Sync/Free conditional time control — `syncMode` toggle swaps the visible control between `noteDivision` (Sync) and `delayTime` (Free); no dead controls in either mode.
- WebView UI (JUCE 8 WebBrowserComponent) per suite standard; mockup does not exist yet and must be produced before/at Phase 3.1.
- Knob readouts must use `SliderState.getScaledValue()` — 6 of 10 params are skewed (delayTime, grainSize, lowCut, highCut log-skew; never a JS min/max map).

## Constraints Identified

- **D6 Standalone audition deliberately deferred** (user decision): GUI work touches no DSP, so Stage 3 proceeds without it. The audition (smear/wash/width by ear, incl. the ~−7.3 dB/generation wash-decay finding and possible feedback-tap makeup constant) becomes a **required entry check for Stage 4**. If makeup gain is added later it is DSP-only and does not invalidate Stage 3.
- Render harness must survive the WebView editor: guard `createEditor` with `#if JUCE_WEB_BROWSER`, keep PluginEditor.cpp out of harness sources, re-run harness after the editor lands (pattern_render_harness_breaks_on_webview_editor).
- Second `juce_add_binary_data` target (UI resources) needs a distinct `NAMESPACE` (dual-BinaryData collision pattern).
- Editor construction order: relays → WebView → attachments (3-arg WebSliderParameterAttachment with nullptr).
- Pass the `Juce` ES-module namespace to panels, not `window.__JUCE__`; explicit resource-provider URL mapping; `type="module"` scripts.
- Verify UI in a browser against a ~20-line JUCE-bridge stub before DAW testing; grep-diff getNativeFunction vs withNativeFunction for silent bridge gaps.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| D7: D6 audition timing | Deferred — proceed to Stage 3 now | GUI doesn't touch DSP; audition moves to Stage 4 entry gate |
| D8: Aesthetic | **Ouaricon Naturalist** (`.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`) | Suite house identity — aged-paper background, botanical specimen styling, seed-cross-section knobs |
| D9: Layout | **Grouped sections** reading as signal flow: TIME (syncMode, noteDivision/delayTime), GRAIN (grainSize, density), FEEDBACK (feedback, lowCut, highCut), OUTPUT (width, mix) | Matches the DSP topology; gives UI-02's swapping control a natural home in the TIME group |
| D10: Visualization | **None — knobs and readouts only** | Fastest path; keeps Stage 3 at the planned 2 phases; no C++→JS polling bridge |

## Open Questions

- Mockup production path: apply the Naturalist template with an adaptive grouped layout via the ui-mockup workflow (ui-design-agent) vs. hand-author directly from the aesthetic doc — resolve in research phase. Layout/aesthetic/no-viz decisions above fully constrain the design, so a non-interactive single-pass mockup is expected to suffice.
- Window size: pick during mockup (grouped 10-control layout suggests a mid-size fixed window; Naturalist template has spacing guidance).
- noteDivision control style in the TIME group (stepped knob vs. selector) — mockup-phase choice; must show the 13 divisions incl. dotted/triplet.

## Next Phase

Ready for: research phase (`/plugin-research O-ReverseDelay 3-gui`)
