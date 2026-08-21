# Stage 3: GUI — Research

**Date:** 2026-08-21
**Inputs:** `stages/3-gui/CONTEXT.md` (design decisions), `parameter-spec.md` (BINDING — 1 choice + 4 floats), `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`, ROADMAP phases 3.1/3.2
**All findings verified against files in this repo on this date.**

---

## 1. Primary template: O-Bitrot

O-Bitrot is the single best copy source — same Naturalist aesthetic, a **proven segmented-button choice binding**, WebView + render-harness coexistence, and the clean paper texture. Exact files:

| What | Where | Notes |
|------|-------|-------|
| Segmented control CSS | `plugins/O-Bitrot/Source/ui/public/index.html` ~line 482 (`.seg` block) | inline-flex, green-dark border, 4px radius; `.sel` = active fill; divider = `button + button` left border |
| Segmented control JS | same file ~line 1315 | Generic `.seg[data-param]` binding via `Juce.getComboBoxState()` — **already generic over button count** (grew 2→3 with zero JS change); 5 buttons works as-is |
| Knob drag/wheel/pointer-capture | same file, `setupKnob` ~line 1233 | Pointer capture, relative drag (0.005/px, shift = 0.001 fine), wheel ±0.02, `getScaledValue()` readouts, `propertiesChangedEvent` re-render |
| C++ relay + attachment wiring | `plugins/O-Bitrot/Source/PluginEditor.{h,cpp}` | `WebComboBoxRelay` (line 43+) + 3-arg `WebComboBoxParameterAttachment`; `.withOptionsFrom(*relay)` chain; `getResource()` explicit URL map (line 543) |
| CMake binary-data target | `plugins/O-Bitrot/CMakeLists.txt` line 101 | Single `juce_add_binary_data` target: index.html, js/juce/index.js, check_native_interop.js, img assets |
| Clean paper texture | `plugins/O-Bitrot/Source/ui/public/img/paper.jpg` | md5 `40c5f97e25bd2492a6c8fe2ef0882541` — verified **NOT** the watermarked Adobe Stock texture (`b7c865c45f2fb95a7a8651071da186e6`, O-Lyrica/O-Gain). Originally from O-Tremolo. Copy this one; write an `img/PROVENANCE.md` like O-Bitrot's |

O-Prism supplies the one piece O-Bitrot lacks — typed value entry (section 5).

---

## 2. Per-console accent colors (open question → resolved)

Design goal: distinct at a glance, but muted/ink-like inside the earth-tone palette. Each hue is the console's brand color pulled down to naturalist saturation. Contrast measured against primary paper `#F5E6D3` (template's own green-mid `#6B8E4E` reads 3.06:1 — every accent below beats it):

| Console | Accent | Hex | vs paper | Derivation |
|---------|--------|-----|----------|-----------|
| SNES | Muted plum | `#7B6480` | 4.31:1 | SNES purple → inked plum |
| PS1 | Slate blue-gray | `#5F7386` | 4.00:1 | PlayStation gray/blue → slate |
| NES | Brick oxide | `#9B5A48` | 4.34:1 | NES red → terracotta |
| Game Boy | DMG olive | `#6F7D3F` | 3.66:1 | DMG screen pea-green (yellower + darker than template moss — hue carries the distinction; thematically the "green console") |
| Genesis | Deep teal | `#4E7A70` | 3.95:1 | Sega blue → inked teal |

**Where the accent applies** (accent-only theming per CONTEXT): active segment fill + border, knob indicator stems, info-readout text tint. Everything else stays template palette.

**Active-segment treatment recommendation: accent-tinted, not template green.** The per-console accent IS the theming mechanism and the selector is its natural home — a green active segment would leave the accent visible only in knob stems. Construction: active fill = accent at ~0.45–0.55 alpha over the paper tone, active border/text = the solid accent darkened ~15% (or `--green-darkest`-style dark ink). Hover keeps the template's light-green tint (`rgba(139,168,112,0.3)`) so the toggle language survives. Implement as CSS custom properties swapped on a `body[data-console="snes"]`-style attribute set from the same `getComboBoxState` listener that updates the readout — one write point, preset-safe.

Plan phase may fine-tune exact alphas; the five base hexes above are the deliverable.

---

## 3. Segmented selector construction (open question → resolved)

Copy O-Bitrot's `.seg` CSS verbatim and extend:

- **5 buttons** in one `.seg[data-param="console"]` container, `data-value="0..4"`, labels `SNES / PS1 / NES / GB / GENESIS` (UI labels — host-facing strings stay the APVTS ASCII set; `GB` abbreviation on the button keeps the row compact, full `Game Boy` name can go in the tooltip/readout).
- O-Bitrot's `.seg3` shows the precedent for padding compression as button count grows; at 5 buttons in a ~580px content width there is ample room (~10px side padding each).
- Divider: existing `button + button { border-left: 1px solid rgba(60,92,26,0.5) }` — keep; with accent-tinted active states, consider neutralizing to a brown divider (`rgba(92,64,51,0.4)`) so the green doesn't clash with non-green accents. Plan decision.
- JS binding: O-Bitrot's generic block works unmodified:
  ```js
  const state = Juce.getComboBoxState(seg.dataset.param);
  // render: toggle .sel on idx match; click: state.setChoiceIndex(+b.dataset.value)
  ```

**Choice-relay pattern decision** (CONTEXT open question: template `combobox-relay.yaml` vs `getComboBoxState`): these are the same pattern — the yaml template *is* `WebComboBoxRelay` + `getComboBoxState`; O-Bitrot's segmented variant is the proven live instance. Use: `WebComboBoxRelay("console")` + 3-arg `WebComboBoxParameterAttachment` (nullptr UndoManager) + `.withOptionsFrom(*consoleRelay)` — juce8-critical-patterns #7/#12 satisfied.

---

## 4. Dinosaur specimen asset (open question → resolved)

Source verified: `~/Dev/Ouaricon Audio Images/skeletons/fulldino_dinoceratamonogr00mars_0487.png`

- **1007×665 RGBA**, real alpha channel: 79.2% fully transparent, 17.8% opaque, 3.0% feathered edges — clean cut-out, no background removal needed.
- Content bbox **(42, 85) → (971, 560)** = 929×475 content, aspect ≈ **1.96:1 (wide)**. This is a *landscape* specimen — unlike O-Bitrot's tall fungi plate, it will underlap the knob row, which the template explicitly allows (z above background, below controls, `pointer-events: none`).
- 484 KB PNG. **Recommendation: crop to content bbox (+~10px margin) and convert to WebP q90** (O-Bitrot precedent: 876×1400 WebP). Expect well under 200 KB. Name it `specimen.webp` — no hyphens (juce_add_binary_data strips them; `preset-manager.js` → `presetmanager_js` is the known example).
- **Placement numbers** (620-wide window, ~430 tall — section 6): height ~60% ≈ 260px → displayed width ≈ 508px at content aspect. Anchor `right: -140px` to bleed roughly a quarter off the right edge, vertically centered slightly low (`top: 55%`) so the skull area clears the selector row. Opacity 0.35 per template. Tune visually at execute; these are starting values.
- Write `img/PROVENANCE.md` (O-Bitrot precedent): plate is from Marsh, *The Dinocerata: a monograph of an extinct order of gigantic mammals* (USGS, 1886) — public domain; record the processing steps.

---

## 5. Knob interaction: merge the two house families

CONTEXT wants O-ReverseDelay-family drag visuals + O-Prism double-click typed entry. Findings:

- **O-Bitrot's `setupKnob` is already the modernized drag family**: pointer capture (no document-level listeners), relative drag with shift-fine, wheel, `getScaledValue()` readouts, skew-aware `scaledToNormalised` helper. Use it as the base — it is newer and cleaner than O-ReverseDelay's mousedown/mousemove version.
- **One change**: O-Bitrot's `dblclick` resets to default. Replace with O-Prism's pattern:
  - `dblclick` → `attachValueEntry(...)` (`plugins/O-Prism/Source/ui/public/index.html` line 1974) — overlay `<input>` on the readout span; span text turns transparent (box keeps its size), ink color captured *before* hiding, untouched-text guard (Enter on unedited text writes nothing — prevents quantizing to the rounded readout), Escape cancels, blur commits, re-render from live state on close.
  - Reset moves to **Alt/Option-click** on the capture phase (O-Prism line ~2100) so it preempts the drag handler.
- All four floats are linear 0–100% (spec) so value parsing is trivial: parse float, clamp 0–100, `setNormalisedValue(v/100)` — but keep the skew-aware helper anyway (zero cost, future-proof).
- Knobs: 60px per CONTEXT — template's 10-segment conic-gradient seed cross-section, stems tinted by the active console accent (section 2).

---

## 6. Window size (open question → resolved): 620×430

Vertical budget at real proportions (Garamond metrics, template spacing):

| Element | Height |
|---------|--------|
| Top padding + container border | ~22 |
| Header row: title (24px type + spacing) with reserved preset-bar slot (~28px) beside/below | ~56 |
| Segmented selector (button 26px + border) | ~30 |
| Info readout line (11px type + gap) | ~22 |
| Section gaps (2 × ~22) | ~44 |
| Knob row: label 14 + knob 60 + readout 16 + internal gaps | ~104 |
| Bottom padding | ~24 |
| **Total** | **~302 + breathing room** |

620×400 *fits*, but the Naturalist spec calls for generous spacing and the header must reserve the Stage-4 preset bar without cramping. **Recommend 620×430 fixed** (`setSize(620, 430)`, non-resizable) — splits the difference, keeps the wide-specimen layout airy. Final ±10px is an execute-phase visual call.

---

## 7. C++ / CMake wiring facts

**Editor (new files `Source/PluginEditor.{h,cpp}`):**
- Members (std::unique_ptr — pattern #11): 1 `WebComboBoxRelay` + 4 `WebSliderRelay`; 1 `WebComboBoxParameterAttachment` + 4 `WebSliderParameterAttachment` (all 3-arg with nullptr — #12); `WebBrowserComponent` last in declaration order.
- Options chain: `.withNativeIntegrationEnabled()` + `.withResourceProvider(...)` + 5 × `.withOptionsFrom(*relay)`; `goToURL(getResourceProviderRoot())`.
- Resource provider: **explicit URL map** (#8) — copy O-Bitrot's `getResource()` shape. Provider receives bare paths. Entries: `index.html` (also for `/`), `js/juce/index.js`, `js/juce/check_native_interop.js` (#13), `img/paper.jpg`, `img/specimen.webp`.
- **The harness guard is pre-planted**: `PluginProcessor.cpp` line ~41 already documents it — the `PluginEditor.h` include goes inside `#if JUCE_WEB_BROWSER` directly above `createEditor()`, with the `GenericAudioProcessorEditor` fallback in the `#else` branch (pattern_render_harness_breaks_on_webview_editor). The harness CMake (`tests/render-harness/CMakeLists.txt`) lists DSP sources explicitly and never compiles PluginEditor.cpp — **do not add editor sources there**.

**CMake (`plugins/O-Emulator/CMakeLists.txt`):**
- Already done in Stage 1: `NEEDS_WEB_BROWSER TRUE` (#9), `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`. Stage 3 touches **none** of the identity config.
- Add: single `juce_add_binary_data(OEmulator_UIResources ...)` at the marked spot (line ~64 comment) + `target_link_libraries(OEmulator PRIVATE OEmulator_UIResources)`. Exactly one binary-data target (namespace collision pattern); no hyphens in any resource filename.
- Add `Source/PluginEditor.cpp` to the plugin target sources.
- `js/juce/index.js` + `check_native_interop.js`: copy from O-Bitrot's `Source/ui/public/js/juce/` (JUCE 8.0.14 frontend library, identical across plugins).
- Preset-manager module: **deferred to Stage 4** per CONTEXT — ignore the CMake comment's mention of `ouaricon_add_module`; only reserve header space in the layout.

---

## 8. Info readout + preset-load freshness

Static JS lookup keyed by choice index (values locked to ARCHITECTURE.md):

```js
const CONSOLE_SPECS = [
  { name: "SNES",     spec: "BRR 4-bit · 32 kHz · Gaussian" },
  { name: "PS1",      spec: "SPU-ADPCM · 22.05 kHz · Gaussian" },
  { name: "NES",      spec: "DPCM · 33.144 kHz · ZOH" },
  { name: "Game Boy", spec: "4-bit wave · 16.384 kHz · ZOH" },
  { name: "Genesis",  spec: "8-bit DAC · 26.32 kHz · ZOH" },
];
```

(`·` is fine — UI-side text may use rich glyphs; only host-facing C++ strings are ASCII-bound.)

Drive readout + accent attribute from the **same `consoleState.valueChangedEvent` listener** that renders the segments. Relay events fire on host automation and preset load alike, so every element (segments, readout, accent theme, knobs) updates on preset load with **no customLoad revision counter needed** — that pattern exists for one-shot C++→JS pushes, and this UI has none: v1.0 has zero native functions and zero backend event bridges. The 3.2 grep-diff bridge audit is correspondingly trivial (expected count: 0 native fns; `window.__JUCE__` must appear nowhere).

Console switching needs nothing beyond the relay — the DSP's 30 ms crossfade handles it; latency is never re-reported.

---

## 9. Pitfalls checklist (mapped to this stage)

| Pitfall | Mitigation |
|---------|-----------|
| Watermarked Adobe Stock paper texture (O-Lyrica/O-Gain) | Copy O-Bitrot/O-Tremolo `paper.jpg`, verify md5 `40c5f9…`, write PROVENANCE.md |
| `juce_add_binary_data` strips hyphens | All resource filenames hyphen-free (`specimen.webp`, `paper.jpg`) |
| Two binary-data targets collide on `BinaryData` namespace | Exactly one `OEmulator_UIResources` target |
| Render harness breaks when editor becomes WebView | Guarded include pre-planted in PluginProcessor.cpp; harness source list untouched |
| ES-module `Juce` namespace, never `window.__JUCE__` for params | O-Bitrot import pattern; audit at 3.2 |
| 3-arg attachments (JUCE 8) | nullptr UndoManager on all 5 attachments |
| JS state updater overwriting HTML-authored labels | Readout `<div>`s are dedicated elements; segment button labels are never written by JS (only `classList`) |
| SVG/label children intercepting pointer events | Knob stems + specimen overlay get `pointer-events: none` |
| Knob readouts must use `getScaledValue()` | O-Bitrot `fmtValue` pattern copied |
| `build-and-install.sh` skips Standalone → stale UI | Visual checks via installed VST3/AU or rebuild Standalone target explicitly |
| Dev/release variant shadowing on install | Use `./scripts/build-and-install.sh O-Emulator` (Phase 4 dual-variant sweep) |
| WebView native-fn completions dropped while hidden | N/A in v1.0 — no native functions |

---

## 10. Handed to plan phase

1. Task breakdown across ROADMAP 3.1 (layout + basic controls: HTML/CSS scaffold, binary-data CMake, editor + relays, knobs, segmented selector) and 3.2 (binding completeness + polish: value entry, accent theming, tooltips if desired, bridge audit, pluginval + install).
2. Asset prep task: crop/convert dino PNG → `specimen.webp`, copy `paper.jpg` + provenance, copy `js/juce/` library.
3. Decisions left open deliberately: divider color under accent theming (§3), exact accent alphas (§2), ±10px window height (§6).
4. Verification gates: harness still ALL PASS with digests unchanged (GUI must not touch DSP), pluginval strictness 10 VST3+AU, auval, manual DAW pass for all 5 consoles × preset-load freshness.
