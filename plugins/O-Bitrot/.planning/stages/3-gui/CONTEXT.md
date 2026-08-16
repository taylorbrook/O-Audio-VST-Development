# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-08-15
**Participants:** User, Claude

## Requirements Confirmed

- WebView UI covering all 31 parameters from `parameter-spec.md` (BINDING — IDs/types/ranges/defaults locked at Stage 1; layout/label refinement only)
- Six family panels (Tape / CD Skip / Vinyl / Packet / Codec / Crush) + global strip (clock, seed/dice, hard edges, mix) per BRIEF
- Reseed dice button writes a random 0–9999 to SEED via the param API; seed readout visible (UI-02, FUNC-04)
- CLOCK_MODE sync/free toggle swaps the visible clock control (SYNC_DIV ↔ FREE_RATE) with no dead params
- Enable-dimming: disabled family panels visually dim
- **NEW scope beyond ROADMAP as written:** per-panel event LEDs — each family panel gets an indicator that lights while its failure event is active (DSP→UI activity bridge)

## Constraints Identified

- parameter-spec.md is BINDING; STATUS.md carries its checksum — no param changes in Stage 3
- Repo WebView patterns apply (from memory/ROADMAP): unique_ptr member order, explicit resource-provider mapping with bare paths, `type="module"` + pass the `Juce` ES-module namespace (not `window.__JUCE__`), readouts via `SliderState.getScaledValue()` (skew-safe), 3-arg JUCE 8 relays/attachments, `getToggleState` for the 7 Bools, `getComboBoxState` for the 5 Choices, grep-diff getNativeFunction vs withNativeFunction, `NEEDS_WEB_BROWSER TRUE`, guard `createEditor` with `#if JUCE_WEB_BROWSER` so the render harness keeps building
- WebView native-fn completions are dropped while the view is hidden — the LED bridge must tolerate missed frames (stateless polling, no completion-dependent state machine)
- LED bridge must be RT-safe: audio thread publishes family-active flags to atomics only; message thread (timer) reads and forwards to JS
- **Aged-paper texture: do NOT reuse the watermarked "Adobe Stock" texture shipped in O-Lyrica/O-Gain — source a clean, licensed/generated texture**
- Fixed window size — no responsive/resize testing axis
- Non-blocking listening items carried from Stage-2 verify (do during Stage-3 DAW sessions): Logic smoke check across families; MIX 50%/0% + HARD_EDGES on; ENV_AMT ±100% voicing; Standalone SEED persistence eyeball

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Aesthetic template | Ouaricon Naturalist (`ouaricon-naturalist-001`) | Official brand; suite consistency; decay theme fits the naturalist specimen framing |
| Illustration | Decomposing specimen (rotting fruit / fungal bloom, classical botanical-mycological plate) | Literal "bitrot"; strong synergy with aged-paper brand look |
| Panel layout | 3×2 grid, chain-ordered: top row Tape/CD/Vinyl (transport families), bottom row Packet/Codec/Crush (signal chain), global strip across the bottom | Layout teaches the architecture |
| Activity feedback | Per-panel event LEDs | Big usability win for a stochastic plugin; modest single-bridge scope |
| Window sizing | Fixed (≈900×620, final size set in mockup) | Simplest; matches prior O- plugins |

## Open Questions

- LED bridge mechanism: timer-polled native function vs `emitEventIfBrowserIsVisible` push — research phase picks one (must survive hidden-view completion drops)
- LED semantics per family: momentary flash per event vs held while the failure state is active (state machine families like CD loop hold state; packet loss is per-packet) — research/plan defines per-family mapping
- Exact fixed window dimensions and per-panel control arrangement — settled in the ui-mockup workflow
- Whether the naturalist template's existing panel/knob prose adapts cleanly to six dense panels + strip, or the mockup needs a compact-panel variant

## Next Phase

Ready for: research phase (`/plugin-research O-Bitrot 3-gui`)
