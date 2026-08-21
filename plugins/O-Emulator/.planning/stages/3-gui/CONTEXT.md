# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-08-21
**Participants:** User, Claude

**Note:** The UI mockup phase was skipped at ideation (user decision, 2026-08-20), so this discuss phase carried the actual UI design decisions. `parameter-spec.md` is the BINDING contract — layout/labels may be refined here, but parameter IDs, types, ranges, and defaults may NOT change.

## Requirements Confirmed

- **UI-01** (WebView GUI with all 5 controls bound two-way) and **UI-02 groundwork** are the stage-3 scope; 13/15 requirements already complete through Stage 2.
- Controls: 1 console selector (choice, 5 entries) + 4 macro knobs (`crush`, `age`, `reverb`, `mix`) per the binding parameter-spec.
- Two GUI phases per ROADMAP: 3.1 layout + basic controls, 3.2 binding completeness + polish.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Aesthetic | **Ouaricon Naturalist** (`.claude/aesthetics/ouaricon-naturalist-001/`) | Brand consistency over a bespoke retro-console look; template chosen over custom |
| Botanical specimen | **Dinosaur skeleton** — `skeletons/fulldino_dinoceratamonogr00mars_0487.png` | Extinct species → discontinued consoles; specimen-of-obsolete-hardware metaphor; unused by any other plugin |
| Console selector | **Segmented buttons** — five labeled segments (SNES / PS1 / NES / GB / GENESIS) in a row | One click per console, current mode always visible; maps cleanly to the choice relay. Note: template's default for 4+ choices is a dropdown — the segmented control is a deliberate deviation, styled with the green toggle-button language |
| Per-console theming | **Accent color only** | One accent hue shifts per console on selector/knob indicators/highlights. Hues must stay inside the Naturalist earth-tone palette (muted, ink-like tints — not saturated console-brand colors); exact values designed in research/plan |
| Extra visualization | **Static info readout** | Per-mode spec line (e.g. `BRR 4-bit · 32 kHz · Gaussian`) directly under the selector. Static lookup table updated on mode switch only — NO C++→JS metering bridge in v1.0 |
| Layout | **Selector top, knobs row** (~620×400) | Title → full-width segmented selector → info readout → single row of 4 medium (60 px) knobs; dino overlay right side at ~0.35 opacity per template |
| Knob interaction | **Drag + value entry** | Full house pattern: O-ReverseDelay relative-drag visuals + O-Prism double-click typed value entry; `getScaledValue` readouts |
| Preset manager | **Deferred to Stage 4** | Stage 3 stays focused on layout + binding; preset-manager v1.0.6 lands with the factory presets Stage 4 authors anyway. Header reserves space for the preset bar |

### Agreed layout sketch

```
┌──────────────────────────────────────┐
│  O-EMULATOR              ❦           │
│                                      │
│  [SNES][PS1][NES][GB][GENESIS]  🦕   │
│   BRR 4-bit · 32 kHz · Gaussian      │
│                                      │
│   (○)    (○)    (○)    (○)           │
│  CRUSH   AGE  REVERB  MIX            │
└──────────────────────────────────────┘
```

(🦕 = dinosaur-skeleton botanical overlay bleeding off the right edge, behind controls.)

## Constraints Identified

- Parameter IDs/types/ranges/defaults are frozen by `parameter-spec.md` (binding since 2026-08-20).
- ASCII-safe host-facing strings (`juce::String(const char*)` is ASCII-only); UI-side labels may use richer glyphs.
- WebView house patterns apply: juce8-critical-patterns #8/#9/#11/#13/#21 setup, 3-arg attachments (#12), relative-drag (#16), ES-module `Juce` namespace (not `window.__JUCE__`), native-fn completions dropped while hidden, grep-diff bridge audit at 3.2.
- Console switch from UI must ride the existing 30 ms DSP crossfade — no latency renegotiation, nothing UI-side to add beyond the choice relay.
- Aged-paper background texture must NOT reuse the watermarked "Adobe Stock" texture asset that shipped in O-Lyrica / O-Gain — use a clean texture (CSS/SVG noise or verified-clean image).
- Info readout values come from a static per-console table in JS (rates: 32000 / 22050 / 33144 / 16384 / 26320 Hz; codecs: BRR 4-bit / SPU-ADPCM / DPCM / 4-bit wave / 8-bit DAC; interpolation: Gaussian ×2, ZOH ×3) — keep it consistent with ARCHITECTURE.md.
- Preset changes must update all elements (customLoad revision-counter pattern if one-shot pushes go stale).

## Open Questions (for research phase)

- Exact per-console accent hex values that read as distinct at a glance yet sit inside the earth-tone palette.
- Segmented-control styling details: active-segment treatment (deeper green fill per template's toggle language vs. accent-tinted), divider/border construction.
- Copy source for the dino PNG (`~/Dev/Ouaricon Audio Images/skeletons/fulldino_dinoceratamonogr00mars_0487.png` → `Source/ui/public/img/`); verify transparency and right-edge crop behavior at ~60–75% height.
- Combobox/choice relay pattern for the segmented selector (template `parameter-binding/combobox-relay.yaml` vs. `getComboBoxState` per ROADMAP 3.1).
- Whether the fixed window needs 620×400 or slightly taller once real Garamond metrics + preset-bar reservation are in place.

## Next Phase

Ready for: **research** phase (`/plugin-research O-Emulator 3-gui`)
