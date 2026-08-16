# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-08-15
**Participants:** User, Claude

## Requirements Confirmed

- **UI-01** — Drawable bipolar speed-vs-time envelope editor for Scratch mode (canvas, add/drag/delete points, per-segment curve, labelled 1× line, reverse zone, pass playhead). Path C §2.2 WebView editor is the reference implementation.
- **UI-02** — Prominent engage control usable as a live performance gesture; UI click and host automation are identical (both via setValueNotifyingHost).
- Parameter set is FIXED (14 APVTS params + scratchEnvelope blob, parameter-spec.md already promoted). The UI owns layout only — no parameter changes in this stage.
- SYNC_MODE drives show/hide of sync-division vs free-ms controls (ROADMAP Phase 3.2).

## Constraints Identified

- **No mockup phase** — user chose to design directly in Stage 3. Phase 3.1 becomes design+build in one pass, iterated in Standalone (`/show-standalone`). ROADMAP's "finalized mockup → index.html" step is replaced by in-place authoring.
- House style is a fixed-px frame with **no viewport units** (WebView hosts size the frame, not the viewport); frame size mirrored in PluginEditor.cpp setSize — keep in sync.
- Frame must fit a 1080p screen **including DAW plugin header + menu bar** (O-ReverseDelay v1.7.1 lesson — measure rendered boxes, don't derive slack from row sums).
- Canvas is a CSS replaced element: explicit width/height + DPR backing store (memory pattern; already called out in ROADMAP Phase 3.3).
- Knob readouts must use `SliderState.getScaledValue()` (skew-correct).
- Native-fn bridge (commitEnvelope/requestEnvelope) SafePointer-hardened; grep-diff getNativeFunction vs withNativeFunction both sides.
- UI readback: atomics + editor timer, `emitEventIfBrowserIsVisible` (completions dropped when hidden).

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Design path | Direct in Stage 3 (no ui-mockup workflow) | User preference; house style is established, so design iteration happens in Standalone against the real bridge |
| Aesthetic | Ouaricon house style — **ouaricon-naturalist-001** | Suite consistency ("as always"). Closest reference: O-ReverseDelay `Source/ui/public/css/styles.css` (header, 44 px preset band, framed group panels, footer); its Row-2 WINDOW envelope display is the nearest canvas precedent |
| Stop/Scratch layout | Mode-switched center panel | One center area swaps by MODE: Stop shows stop/start time+curve controls; Scratch shows the envelope canvas. Compact, no dead space in either mode |
| Live visualization | Playback-ratio indicator only | Live speed readout (incl. reverse) driven by the existing atomic+timer readback. No reel/platter animation, no transport-state lamp. Envelope pass playhead is already required by UI-01 |

## Open Questions

- Frame dimensions for O-Tapestop (14 params ≪ O-ReverseDelay's set — likely a smaller frame; settle in research/plan by sketching the group-panel inventory: TRIGGER/MODE, TIMES, CURVES, OUTPUT + mode-switched center)
- Playback-ratio indicator form (needle vs horizontal bar vs numeric ×-readout) and its readback rate
- Whether the preset band (preset-manager module, now v1.0.5) ships in Phase 3.1 layout or is deferred to Stage 4 with the factory presets
- Ratio indicator + envelope playhead: shared timer or separate cadences
- Mode-switch transition treatment (instant vs brief fade) — keep cheap, no layout thrash

## Next Phase

Ready for: research phase
