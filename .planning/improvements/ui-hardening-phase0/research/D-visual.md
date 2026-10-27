# D-visual — visual design audit of the Ouaricon suite (46 WebView UIs)

2026-10-26 · read-only on the repo · working tree at `ea44af28` + uncommitted edits (see Caveats)

All paths below are relative to this folder:
`/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/39983b81-85a2-44fc-8aff-422a28a35353/scratchpad/D-visual/`

## Method in one paragraph

Each plugin was rendered with the repo's own `scripts/serve-ui.js` (`buildRoot` to temp, `readEditorSize` and `serve` on port 0) in headless Chromium through Playwright. The viewport is the `setSize(W,H)` parsed from that plugin's `PluginEditor.cpp`, at deviceScaleFactor 2. The stub was the generic one for 41 plugins and the plugin's own for 5. A second, text-hidden screenshot was taken for pixel-probed contrast. In the same page, a census recorded every visible text leaf (size, family, colour, opacity, clipping), every knob, button, select, radius, border, ground and scroller. CDP `CSS.getPlatformFontsForNode` reported the face that actually rendered. **Boot verdict: 46/46 clean** (0 page errors, 0 console errors, 0 failed requests, 0 404s). No servers were left running.

## Artifacts

| What | Path |
|---|---|
| Report (this file) | `D-visual.md` |
| 46 screenshots, 2x, shipping frame | `shots/<Plugin>.png` |
| Text-hidden grounds (contrast probe) | `bg/<Plugin>.png` |
| **Contact sheet, all 46** | `sheets/contact-all.png` |
| **Contact sheet grouped by tier** | `sheets/contact-by-tier.png` |
| Effects / instruments sheets (larger) | `sheets/contact-effects-{1,2}.png`, `sheets/contact-instruments-{1,2}.png` |
| Header band, all 46, 1x unscaled | `sheets/headers-{1..4}.png` |
| Footer band, all 46 | `sheets/footers-{1..3}.png` |
| One knob per plugin, 1x unscaled | `sheets/knobs.png` |
| Ground colour per plugin | `sheets/grounds.png` |
| Gear / prev-arrow glyph per plugin + resolved font | `sheets/glyph-gear.png`, `sheets/glyph-prev.png` |
| Defect crops (26) | `sheets/defects-{1,2}.png`, `crops/*.png` |
| Raw census, contrast and stats | `data/<Plugin>.json`, `data/_analysis*.json`, `data/_suite_stats.json`, `data/_grounds.json`, `data/_spacing.json` |
| Scripts | `scripts/capture.js`, `analyze.py`, `sheets.py`, `suite_stats.py`, `crops.py`, `tiers.py`, `spacing.js`, `booldefaults.js` |

## Headline

1. **The house exists and is good.** Its best form is the "field guide": O-ReverseDelay and the nine O-simple* plugins, plus O-Contrabass. Paper `#F5E6D3`, ink `#3C2F2F`/`#5C4033`, walnut rules `#8B7355`, sage accents, EB Garamond, and seed-cross-section knobs.
2. **10 of 46 pages do not fit their own shipping frame.** Rows, keyboards and columns are cut, a title wraps, and one label overprints another. This is the most visible defect class. Only a geometry census at the parsed frame finds it.
3. **The chrome is the least consistent part.** The suite has:
   - 3 naming schemes;
   - 4 header archetypes;
   - titles at 14–28px;
   - preset arrows in 5 glyph shapes;
   - SAVE/LOAD order reversed in 8 plugins;
   - gears 17–28px, in 2 shapes and 5 positions, always drawn from a system fallback font.
4. **Two papers.** 29 plugins sit on light cream. 14 sit on an orange-stained JPG, 2–3 tone steps darker. Side by side, that split is the strongest "not one product" signal.
5. **Contrast is much better than at the 260924-nho review.** O-Prism has gone from 75% below AA to 0%, and O-TextureForge from 69% to 7%. 9.2% of text nodes are now below AA (pixel-probed). Half of that is opacity-dimmed "disabled" text. Most of the rest is in 4 plugins.

---

## (a) The de facto house style, as tokens

Values are **measured** on the best-looking plugins: O-ReverseDelay, O-simpleSubtractive, O-simpleReverb, O-simpleAdditive, O-simpleWavetable and O-Contrabass. Suite frequencies are in brackets.

```css
:root {
  /* ── ground ─────────────────────────────────────────────── */
  --paper:        #F5E6D3;  /* body ground; 29 plugins sit at #E2D0BF–#F6E9DB */
  --paper-2:      #EBD9C7;  /* deep paper: Contrabass ground, simple* signal-path strip */
  --paper-3:      #E8D0B8;  /* deepest gradient stop (GrainScatter, TextureForge) */
  --linen:        #FAF0E6;  /* field / button / select fill */
  --panel:        rgba(250,240,230,.45); /* group wash (simple*); ReverseDelay: rgba(212,196,176,.42) */
  --panel-accent: #D4C4B0;
  --well:         #241C14;  /* scope/visualiser well; simple* measure #1F1811–#292017 */
  --grain: repeating-linear-gradient(0deg, rgba(92,64,51,.02) 0 1px, transparent 1px 3px); /* ReverseDelay */

  /* ── ink (all ≥ 4.5:1 on --paper) ──────────────────────── */
  --ink:          #3C2F2F;  /* values, primary text 10.45:1 */
  --ink-2:        #5C4033;  /* title, labels 7.67:1; also the 3px window frame + Family-C header bar */
  --ink-3:        #715D45;  /* subtitle, captions, footer; ≥4.56 on both papers (#6A5641 over gradients) */
  --ink-btn:      #2C3E10;  /* button text, section heads in simple* */
  --sage-text:    #4E6839;  /* italic hints */

  /* ── line ──────────────────────────────────────────────── */
  --rule:         #8B7355;  /* 1–2px borders (35 of ReverseDelay's bordered boxes) — never text */
  --rule-soft:    rgba(139,115,85,.35); /* sub-panel hairline (simple*) */

  /* ── accent ────────────────────────────────────────────── */
  --sage:         #6B8E4E;  /* italic title accent word (25px, large-text 3.06 ok), decoration */
  --sage-fill:    rgba(139,168,112,.25);  /* toggle / gear default fill */
  --sage-line:    #3C5C1A;  /* toggle border */
  --sage-on:      #4E6839;  /* pressed fill, text #F5E6D3 (5.10:1) */
  --trace:        #8BA870;  /* scope trace 1 */
  --trace-2:      #E8B04A;  /* scope trace 2 (amber; simpleReverb) */

  /* ── type: one face ────────────────────────────────────── */
  --font: "EB Garamond", Georgia, "Times New Roman", "PingFang SC", "Microsoft YaHei", serif;
  --fs-title:   25px;   /* 400, +3px tracking, "O – simple" + italic --sage word (simple* 25–26, ReverseDelay 25) */
  --fs-section: 12px;   /* UPPERCASE 500 +1.4px (simple*); ReverseDelay group titles 11px +1.6px */
  --fs-sub:     10.5px; /* UPPERCASE +1.4–1.6px --ink-3 (subtitle, scope captions) */
  --fs-value:   11px;   /* 400, tabular figures (EB Garamond digits are 480u) */
  --fs-button:  10px;   /* UPPERCASE +1px */
  --fs-label:   9.5px;  /* UPPERCASE +0.6–0.9px --ink-2 */
  --fs-micro:   9px;    /* floor: footer hint, ticks */
  /* suite: 93% of 3,315 text nodes already sit at 9–13px; 22 distinct sizes in use */

  /* ── shape ─────────────────────────────────────────────── */
  --r-chip: 2px;   /* keys, chips      (suite #1: 728 boxes) */
  --r-ctl:  4px;   /* buttons, selects, scope wells (385) */
  --r-panel: 5px;  /* group panels (ReverseDelay, simple*); Bitrot/Contrabass use 6 */
  --r-round: 50%;  /* knobs, gear (607) */
  --bw-window: 3px;  /* --ink-2 outer frame (ReverseDelay, simple*) */
  --bw-group:  2px;  /* --rule, primary groups (ReverseDelay) — or 1px --rule-soft (simple*) */

  /* ── space (4px base) ──────────────────────────────────── */
  --sp-1: 4px; --sp-2: 8px; --sp-3: 12px; --sp-4: 16px; --sp-6: 24px;
  --frame-margin: 16px;    /* simple* 15–19, Bitrot 17, ReverseDelay 29 */
  --group-gap: 10px;       /* measured 8–12 */
  --group-pad: 8px 12px 10px; /* measured 6/10/8/10 (simple*) … 15/14/11/14 (ReverseDelay) */
  --knob-label-gap: 6px;   /* measured 6–7 */
  --label-value-gap: 2px;  /* measured 2 (simple*) – 7 (ReverseDelay) */

  /* ── controls ──────────────────────────────────────────── */
  --knob-s: 44px; --knob-m: 54px; --knob-l: 64px; --knob-hero: 90px;
  --btn-h: 24px; --btn-h-compact: 20px;  /* suite modes 20/22/23/24 */
  --select-h: 26px;
  --gear: 24px;
}
```

**Components, as the best plugins build them:**

| Component | House spec (measured) |
|---|---|
| Window | `--paper` + grain, 3px `--ink-2` frame, 16px margin |
| Header | Left title `--fs-title`. Uppercase subtitle "… · A FIELD GUIDE". 1px `--rule` underline. Gear top-right. ReverseDelay and Tapestop centre the same block. |
| Preset group | `[◀][name][▶] SAVE LOAD (DELETE)`, 23–25px tall, linen fill, 1px rule, r4, right-aligned |
| Gear | 22–26px round, `--sage-fill`, 1px `--sage` border, top-right |
| Group panel | `--panel` wash, 1px `--rule-soft` or 2px `--rule`, r5. Section head inside, with a dotted hairline under it. |
| Knob | Family A "seed cross-section": cream 20-wedge conic, 2px walnut rim, dark stem rotating −135°→+135°. Label under it, then the value. |
| Button / toggle | Linen fill or sage-fill, 1px border, r4, 10px uppercase. "On" = `--sage-on` with paper text. Segmented pairs as in simpleWavetable. |
| Scope well | `--well` fill, 2px `--rule` border, r4, sage/amber traces. Caption above: uppercase label + italic sage hint. |
| Footer | 9–10px uppercase `--ink-3` hint row, or the "OUARICON AUDIO · O-NAME … credits" signature row (simpleReverb). Fleurons only at ≤0.3 alpha. |

**How far the suite sits from these tokens (measured):**

- **Title.** Naming splits three ways: "Ouaricon X" ×13, "O-X" ×19 and "O – X" ×13, and Wind has none. Size runs 14–28px, tracking 0.4–4px, weight 300/400/500/700.
- **Type.** Each plugin uses 4–13 sizes; Bitrot, simpleAmbisonics and simpleWavetable use 13. Several plugins are mostly bold: MBC 96% of text, simpleBeatmaker 60%, Bass 53%, Tremolo 47%, IntonationPad 31%, Gain 26%.
- **Knobs.** There are 30+ diameters, from 14 to 192px. The commonest are 52 (91 knobs), 48 (75), 55 (65), 60 (58) and 56 (47). Even the simple* family uses 44, 46, 48, 54 and 56.
- **Knob families.**
  - Seed (A/A′): 25 plugins.
  - Vine arc (B, teal, no pointer): 9 — Chorus, Comp, Detune, DigiDelay, Freeze, Prism, Strata, IntonationPad, MicrotonalSampler.
  - Arc + indicator (C): 5 — Bassoon, Bowed, Contrabass, Reed, Wind.
  - Bespoke: 2 — Polystutter, Texture.
  - Sliders only: 5 — Bells, Lyrica, FreqPulse, Gain, Octagon.
- **Frames.** 35 distinct sizes across 46 plugins, from 700×125 to 1180×1000.

---

## (b) Tier list (46)

The contact sheet `sheets/contact-by-tier.png` shows all four tiers. Each plugin's screenshot is `shots/<Plugin>.png`.

### Exemplary (6): ship as the reference

| Plugin | Why |
|---|---|
| O-ReverseDelay | The reference. Centred field-guide header, 2px-walnut group grid, one seed-knob family (56/62px), 6 type sizes, hint footer, no solid contrast fails. |
| O-Contrabass | The best instrument. Roman-numeral section heads with italic captions, Schelleng and body-mode diagrams, a VU meter. Dense but ordered. |
| O-simpleSubtractive | The field-guide template at full strength: scopes, signal-path strip, lesson row and keyboard all fit. Its only issue is the 1000px-tall frame. |
| O-simpleReverb | Dark wells balanced against paper groups, consistent 54px knobs, signature footer. |
| O-simpleAdditive | The drawbar row is a strong hero. Clean group rhythm, and the plate is washed behind the controls, never under them. |
| O-simpleWavetable | The most polished control set: segmented OFF/ON and the waveform-shape picker. Its drift is 13 type sizes and 4 knob sizes. |

### Minor drift (24): clearly the house, one or two things off

| Plugin | Why |
|---|---|
| O-Bitrot | The catalogue-plate header and "Tab. I–VIII" panels are charming. 13 type sizes and 16–20px buttons make it the densest page. |
| O-simpleAmbisonics | On-template but crowded (172 text nodes, 13 sizes). Inactive orders and channel letters at 0.32–0.35 alpha read 1.6–1.9:1. |
| O-simplePhysicalModelSynth | Disabled knobs at 0.38 alpha read 1.9:1. SVG captions are 7px. Panels are half-empty, and a ~90px band under the keyboard is dead. |
| O-MultiBandCompressor | Its ground (`#DBD9C5`, sage-grey) is unique, 96% of text is bold, and the "Off" toggle and select fall back to Arial. The band-coloured borders are a good idea. |
| O-Tapestop | ReverseDelay's twin, but it still renders **Times New Roman** (no EB Garamond embed). Lit segments are paper on light sage (2.3:1). Tick labels are 8px. |
| O-Bells | The main page is sliders only. 59% of the Instrument tab is below the fold, in a scroller with no visible scrollbar. |
| O-Lyrica | Stained paper. Section heads butt against the row above, and values float ~60px right of their sliders (`crops/lyrica-rhythm.png`). |
| O-Marimba | The heaviest orange stain. Two-line labels knock values off a common baseline (`crops/marimba-baseline.png`), and the visualiser box is empty. |
| O-IntonationPad | A colour-emoji die sits among the tabs, the active tab is a saturated lime, and 31% of text is bold (`crops/intonationpad-emoji.png`). |
| O-Prism | Header and footer bars in `#3C2F2F`. Teal arc knobs have no pointer. Footer readouts touch the frame edge (`crops/prism-footer-cut.png`). |
| O-Strata | Same template and same issues as O-Prism. |
| O-Bassoon | Its 16px italic title in the `#5C4033` bar is unique. Controls end at y≈445 of 600, so the bottom 26% is blank. |
| O-Reed | The accordion leaves 4 knobs visible. Most of the canvas is blank paper, including the right ~70% of the top band. |
| O-TextureForge | The empty-state panel dominates the frame. The title is only 14px, and labels sit left of the knobs, which no other plugin does. |
| O-GrainScatter | Dense. 37 captions in disabled sections are at 0.25–0.4 alpha (1.0–1.9:1), and the "activate" hint pill sits on the note chips. |
| O-Chorus | The strip format works, but it uses Family B teal arcs (no pointer) instead of sage seed knobs. |
| O-Detune | A teal accent word in the title, an iOS-style switch, and washed-out preset buttons. |
| O-DigiDelay | White sticker chips sit behind the values, and the butterfly plate runs under every knob. Teal arcs. |
| O-Freeze | `#3C2F2F` title bar. Blurred white glow-chips behind the values read as smudges (`crops/freeze-chips.png`). |
| O-Comp | The bold Title-Case header is jammed against the top edge, with 16px-tall preset buttons (`crops/comp-top-edge.png`). |
| O-AnalogSaturation | Sparse: one 90px knob and two tiny VU windows on a big stained field. |
| O-Bass | The darkest stained paper (`#C9A169`), olive-filled preset chrome, a gold-wedge knob variant, and 53% bold. |
| O-Tremolo | Stained paper, olive preset chrome, tall rounded toggles, and 47% bold. |
| O-Gain | A 14px bold title, and seven chunky bold two-line buttons pinned to the bottom edge (`crops/gain-bottom-row.png`). |

### Off-brand (6): reads as another product line, or renders the wrong face

| Plugin | Why |
|---|---|
| O-Octagon | A deliberate dark "venue" theme: `#211B17` ground, gold accent, Iowan Old Style + **Menlo for 59% of glyphs**. Internally consistent, but not the naturalist house. |
| O-Polystutter | Dark bakelite knobs, filled walnut buttons and Arial "<" ">" (`crops/polystutter-arial-nav.png`). Lanes 3–4 are dimmed to 0.4 by default. |
| O-FreqPulse | A near-black sequencer slab (`#1A1410`) with teal bars, native range sliders, and a 28px Title-Case centred header. |
| O-MicrotonalSampler | A gold accent instead of sage, Menlo readouts (11% of glyphs) and pill chips. Velocity and trim labels are `#6A5641` on tan (2.7–3.3:1). |
| O-Texture | A flat, generic layout: a big empty pad, icon tiles, Family-D knobs. The coral plate is drawn through the FREEZE control (`crops/texture-coral-freeze.png`). |
| O-AnalogEQ | It still renders **Times New Roman** (no embed). 47 nodes are under 9px (down to 6.75px), and 8px white readouts on tan read 2.0–2.4:1. |

### Broken at the shipping frame (10): visible defect on first open

| Plugin | Defect | Evidence |
|---|---|---|
| O-Emulator | The title wraps to "O-" / "EMULATOR" because the preset bar squeezes the header. | `crops/emulator-title-wrap.png` |
| O-Orbit | The MOTION panel is cut by the footer. The Spatial section is hidden in a scroller that shows 226 of 544px. The ELEVATION thumb covers the "O" of OFF. | `crops/orbit-panel-cut.png`, `crops/orbit-toggle-overlap.png` |
| O-SpectralShaper | The "ATTACK CURVE" title overprints the "+6dB" axis label, and "20k" is clipped to "2". Undo/redo use Lucida Grande glyphs. | `crops/spectralshaper-label-collision.png` |
| O-Bowed | HUMANIZE's second knob row is cut by the footer bar (left panel shows 520 of 583px). | `crops/bowed-humanize-clipped.png` |
| O-Formant | The right column is clipped (340 of 698px). The third consonant knob is half-hidden, and further controls can't be reached. | `crops/formant-consonant-clipped.png` |
| O-Wind | There is no product name in the header. OUTPUT and IMPOSSIBLE PHYSICS values are cut at the frame bottom. | `crops/wind-no-title-bottom-cut.png`, `crops/wind-bottom-cut.png` |
| O-simpleBeatmaker | 1,041px of content in an 894px frame cuts the Open Hat and Tom voices. Its bold Title-Case heads differ from its siblings. | `crops/simplebeatmaker-bottom-cut.png` |
| O-simpleFM | The keyboard runs off the bottom of its own 760×980 design frame (1,027 vs 974px), and the subtitle wraps to two lines. | `crops/simplefm-keyboard-cut.png` |
| O-simpleGrain | The side rail shows 472 of 1,184px, cutting WINDOW mid-control. It renders Times New Roman and has 23 solid contrast fails. | `crops/simplegrain-rail-clipped.png` |
| O-simpleSampler | The PLAY header is visible but the keyboard is below the frame (836 vs 714px). | `crops/simplesampler-keyboard-missing.png` |

Note: simpleGrain and simpleSampler comment that "the frame scrolls on shorter screens", and simpleBeatmaker and simpleFM scale to the display. The overflow measured here is **at the design frame itself**, so first open looks cut even on a large screen.

---

## (c) Top 10 suite-wide visual improvements

Ranked by visible impact over effort. Every fix is per plugin, with a version bump and before/after crops. The previous review's "no big-bang retrofit" stance still holds.

**1. Make every page fit its own frame, and gate it.** 10 plugins (tier "Broken") cut or overlap content at the parsed `setSize` frame.
- **Fixes:**
  - O-Emulator: move the preset group to a second header row, like ReverseDelay and Tapestop.
  - O-simpleSampler, O-simpleFM, O-simpleBeatmaker: trim 40–150px from the hero scope or grid, or make the design frame match the content (they already scale to the display).
  - O-Formant, O-Bowed, O-Orbit: give the clipped column its height, or move a section to a tab.
  - O-SpectralShaper: inset the curve title and right-pad the axis.
  - O-Wind: add the product name.
- **Gate:** report any visible text leaf or knob outside the viewport, or outside a clipping ancestor, at the parsed frame. `scripts/capture.js` here already computes this in about 2s per plugin and could become a `boot-all-uis --fit` arm.
- **Evidence:** `sheets/defects-1.png`, `sheets/defects-2.png`, `crops/*`.

**2. One header, preset bar and gear.**
- **What drifts:**
  - 4 header archetypes: field-guide (13), dark bar (8, `#5C4033` or `#3C2F2F`), catalogue plate (2), and plain spaced caps on paper (the rest).
  - 3 naming schemes.
  - Preset arrows in 5 glyph shapes (◀ ‹ < and Arial <).
  - LOAD→SAVE in 8 plugins (Bass, Chorus, Comp, Detune, DigiDelay, FreqPulse, Marimba, Tremolo) against SAVE→LOAD in 11.
  - About 14 plugins have no header preset bar.
  - Gears range 17–28px: 36 round and 10 square-ish. 33 sit top-right, 5 bottom-left, 5 bottom-right, 2 top-left and 1 top-centre.
- **Fix:** standardise on the field-guide header:
  - "O – " plus an italic sage word, 25px/400/+3px;
  - an uppercase subtitle and a 1px rule;
  - a right-aligned `[◀ name ▶] SAVE LOAD` group, 24px tall;
  - a 24px round gear, top-right, inset 16px.
- **Family-C bars:** the dark-bar plugins can keep the bar, but should use the same type, order and sizes.
- **Evidence:** `sheets/headers-1.png`…`headers-4.png`, `sheets/glyph-prev.png`, `sheets/glyph-gear.png`.

**3. One paper.**
- **Problem:** 14 plugins sit on an orange-stained JPG, with medians from `#C4985D` to `#DCBC90`: AnalogEQ, AnalogSaturation, Bass, Chorus, Comp, DigiDelay, Emulator, Freeze, Gain, IntonationPad, Lyrica, Marimba, Polystutter and Tremolo. The other 29 are light cream (`#E2D0BF`–`#F6E9DB`). The stained ones look older and dirtier, and they drag text contrast down: O-Gain's in-flight R4 change had to switch its text to near-black because of it.
- **Fix:** use Bitrot's recipe. Keep the JPG but wash it with `linear-gradient(rgba(245,230,211,.42), rgba(240,222,200,.5)), url(paper.jpg)`, which lands at a `#F2E0C6` median. Or move to ReverseDelay's CSS grain.
- **Evidence:** `sheets/grounds.png`, `sheets/contact-effects-1.png` (stained) next to `sheets/contact-effects-2.png` (ReverseDelay, simpleReverb).

**4. Draw icons as an inline-SVG sprite, not font glyphs.**
- **Gear:** EB Garamond has no ⚙, so it renders in Menlo in 37 plugins and Apple Symbols in 9. On Windows it will be Segoe UI Symbol.
- **Arrows:** ◀▶ come from EB Garamond, Hiragino Mincho, Apple Symbols or Arial, depending on the stack. 8 plugins use ASCII `<` `>`.
- **Other glyphs:** IntonationPad shows a colour emoji 🎲, and SpectralShaper draws ↩↪ in Lucida Grande.
- **Fix:** one `modules/ui/icons` set (gear, prev, next, caret, dice, undo, redo) at 14px in `currentColor`. That also stops the platform drift.
- **Evidence:** `sheets/glyph-gear.png`, `sheets/glyph-prev.png`, `crops/intonationpad-emoji.png`.

**5. Finish the face.**
- **Still on Times New Roman:** AnalogEQ, Tapestop and simpleGrain render 97–99.5% of glyphs in it. None of them embeds `modules/ui/eb-garamond`, and their stacks lead with `Garamond`.
- **At HEAD:** Bassoon, Freeze, Gain, Reed and simpleFM also lack the face. Their migration exists only as uncommitted working-tree edits.
- **Arial leaks:** MBC's "Off" toggle and select, and Polystutter's nav. `<button>` and `<select>` do not inherit `font-family`.
- **Fix:** embed the module, add `button, select, input { font: inherit }`, and fold Menlo stacks into the serif. EB Garamond figures are already tabular. That covers MicrotonalSampler's readouts; Octagon's Menlo is part of its theme.
- **Evidence:** `crops/times-vs-ebgaramond.png` next to `crops/ebgaramond-reference.png`, `crops/mbc-arial-off.png`, `crops/polystutter-arial-nav.png`.

**6. A five-step type scale, weight 400, and a fixed rhythm.**
- **Problem:** 22 sizes are in use suite-wide, and 4–13 per plugin. Bold drift: MBC 96% of text, simpleBeatmaker 60%, Bass 53%, Tremolo 47%, IntonationPad 31%, Gain 26%. The template asks for 400/500. Label and value rhythm breaks in Lyrica and Marimba.
- **Fix:**
  - Sizes: 9 / 9.5 / 11 / 12 / 25, plus a 10.5 subtitle.
  - Build hierarchy from case and tracking, not weight.
  - Fixed gaps: 6px from knob to label, 2px from label to value. Clamp labels to one line, or reserve two lines for every cell.
- **Evidence:** `shots/O-MultiBandCompressor.png`, `shots/O-simpleBeatmaker.png`, `crops/gain-bottom-row.png`, `crops/lyrica-rhythm.png`, `crops/marimba-baseline.png`.

**7. Three knob sizes, and one accent for the arc family.**
- **Problem:** there are 30+ diameters. Family B (9 plugins) uses teal arcs with no pointer, so the value can't be read at a glance and the brand colour is wrong.
- **Fix:**
  - Sizes: S 44 / M 54 / L 64, plus a 90 hero for single-knob pages.
  - Leave the families in place, as 260924-nho advised against unifying them. But recolour B's arc from teal to `--sage` and add a 2px pointer tick.
  - Build new and refreshed plugins on Family A.
- **Evidence:** `sheets/knobs.png`, `shots/O-Prism.png`, `shots/O-Chorus.png`.

**8. Disabled is not transparent text; fix the four hot spots.**
- **Scale:** pixel-probed, 308 of 3,340 text nodes (9.2%) are below AA, across 21 plugins.
- **153 are opacity-dimmed disabled states:** Polystutter lanes 3–4 at 0.4 (1.9:1), GrainScatter (37 nodes), simpleAmbisonics, simplePhysicalModelSynth.
- **128 are solid colours in 9 plugins:**
  - AnalogEQ 50 (8px white on tan, 2.0–2.4:1);
  - simpleGrain 23 (sage at 0.85 alpha, ≈2.6:1);
  - MicrotonalSampler 18 (`#6A5641` on `#C6A375`);
  - Octagon 13 (`#6F6459` on `#241E1A`, 2.9:1).
- **Fix:** dim the graphic (arc, wedges) and keep the caption in `--ink-3` with an italic "off". That is the suite's own documented pattern.
- **Evidence:** `shots/O-Polystutter.png`, `shots/O-GrainScatter.png`, `shots/O-AnalogEQ.png`, and the worst-node lists in `data/_analysis_slim.json`.

**9. One rule for plates, value chips and dark wells.**
- **Plates under controls:** Texture's coral crosses FREEZE, and DigiDelay's butterfly sits behind every knob. Chorus shows the right way: the insect sits between the knobs.
- **Three value treatments:** plain ink (house), white sticker chips (DigiDelay) and blurred glow chips (Freeze).
- **Three dark-well colours:** `#241C14` (simple*), `#1A1410` (FreqPulse) and `#000` (SpectralShaper).
- **Fix:**
  - Plates go in a reserved empty zone at ≤0.3 alpha and never intersect a control box. The census can check this.
  - Values are plain `--ink`; use the zero-layout paper chip only where a busy ground is unavoidable.
  - Use one `--well` token.
- **Evidence:** `crops/texture-coral-freeze.png`, `crops/freeze-chips.png`, `shots/O-DigiDelay.png`, `shots/O-SpectralShaper.png`.

**10. Right-size the frames.**
- **Problem:** there are 35 distinct frame sizes. 5 frames are ≥860px tall (simpleSubtractive 1000, simpleFM 980, simpleAdditive 930, simpleBeatmaker 900, simplePhysicalModelSynth 860), which overflows a 13" laptop. At the other end, Reed, Bassoon (26% blank) and the empty state of TextureForge and AnalogSaturation leave large areas of bare paper.
- **Fix:**
  - Adopt a few standard frames, for example 600×400 compact, 900×600 standard and 1180×780 field-guide.
  - Port simpleFM's and simpleBeatmaker's scale-to-display code everywhere.
  - Fill blank bands by enlarging the hero (Reed's morph pad, Bassoon's panels), or shrink the frame.
- **Evidence:** `sheets/contact-all.png`, `shots/O-Reed.png`, `shots/O-Bassoon.png`, `shots/O-simpleSubtractive.png`.

---

## What already works (keep)

- The palette core is shared almost everywhere. Ink `#3C2F2F`, walnut `#8B7355` and paper `#F5E6D3` appear across the suite, and that is why the contact sheet reads as one family even with the drift.
- EB Garamond renders 97–100% of glyphs in 42 of 46 plugins (working tree). It takes the place of the Times / Georgia / Iowan split the September review found.
- The simple* field-guide system is the most coherent sub-family. Its header, groups, scopes, lesson row and keyboard are a ready-made template to move other plugins toward.

## Caveats

- **Renderer.** Headless Chromium at 2x, not WKWebView in a DAW. Fonts resolve through the same CoreText set, but nothing was checked inside a host.
- **Stub values are fixtures.**
  - Dropdowns may show "1", values may read "1 Hz" or "1%", scopes are empty and version labels say "V0.0.0-STUB". None of these was used as a finding.
  - Boolean parameters were seeded from their C++ `AudioParameterBool` defaults, in the temp tree only. This changed Polystutter: lanes 3–4 now show their real default-off state.
- **Uncommitted work by another session.** O-Bassoon, O-Freeze, O-Gain, O-Reed and O-simpleFM have uncommitted EB Garamond and R4-palette edits. Their screenshots show that in-flight state, not HEAD.
- **Scope.** English, default tab and first view only. Other tabs, popovers and languages were not audited.
- **Contrast method.**
  - Text is hidden with CSS, and the 2x background is sampled inside each text box: the median, plus the worse of p10 and p90.
  - The foreground is composited with its effective opacity.
  - Canvas-drawn text is not covered. Fleurons are counted separately, as decorative.
- **Knob census heuristic.** It counts elements whose class contains knob, dial or rotary, or with `role=slider`, that are square and 14–220px, keeping only the outermost.

## Reproduce

```bash
D=/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/39983b81-85a2-44fc-8aff-422a28a35353/scratchpad/D-visual
TMPDIR=$D/tmp node $D/scripts/capture.js            # 46 shots + bg + census, ~2 min, read-only on the repo
python3 $D/scripts/analyze.py && python3 $D/scripts/sheets.py && python3 $D/scripts/suite_stats.py \
  && python3 $D/scripts/crops.py && python3 $D/scripts/tiers.py
```

## Appendix — per-plugin metrics

The "Below AA" column splits failures into three causes:
- solid colour;
- opacity-dimmed;
- decorative fleuron.

"Non-EB glyph faces" lists the system fonts that render any glyphs other than EB Garamond.

| Plugin | Kind | Frame | Text nodes | Below AA (solid / dimmed / fleuron) | <9px | Type sizes | Knob Ø (px: n) | Bold % | Ground (median) | Non-EB glyph faces |
|---|---|---|---|---|---|---|---|---|---|---|
| O-AnalogEQ | fx | 920×220 | 64 | 50 / 0 / 0 | 47 | 8 | 41:4, 70:1, 74:1, 77:1, 92:1 | 6 | `#C99B63` | Times New Roman 99.1%, Hiragino Mincho ProN 0.3% |
| O-AnalogSaturation | fx | 600×450 | 29 | 0 / 0 / 0 | 0 | 5 | 90:1 | 14 | `#D1B48F` | Menlo 0.8% |
| O-Bass | fx | 420×320 | 15 | 0 / 0 / 0 | 0 | 6 | 65:3 | 53 | `#C9A169` | Menlo 1.3% |
| O-Bassoon | inst | 900×600 | 32 | 0 / 0 / 0 | 0 | 7 | 60:1, 78:9 | 3 | `#F5E6D3` | Menlo 0.5% |
| O-Bells | inst | 800×600 | 74 | 0 / 0 / 0 | 0 | 5 | — (sliders) | 1 | `#ECDDC5` | Menlo 0.1%, Hiragino Sans 0.1% |
| O-Bitrot | fx | 900×740 | 122 | 0 / 0 / 4 | 1 | 13 | 40:1, 50:25, 56:1, 58:3 | 0 | `#F2E0C6` | Menlo 0.3%, Georgia 0.3% |
| O-Bowed | inst | 900×600 | 73 | 0 / 0 / 0 | 0 | 6 | 30:11, 55:12, 72:3 | 3 | `#F5E6D3` | Menlo 0.2% |
| O-Chorus | fx | 700×125 | 24 | 0 / 0 / 0 | 0 | 5 | 48:8 | 0 | `#DCBC90` | Menlo 1.0% |
| O-Comp | fx | 620×420 | 30 | 0 / 0 / 0 | 0 | 4 | 34:2, 52:6 | 17 | `#DABA90` | Menlo 0.5% |
| O-Contrabass | inst | 1000×650 | 95 | 0 / 1 / 0 | 0 | 7 | 55:23, 70:1 | 14 | `#EAD9CA` | Georgia 0.3%, Menlo 0.1% |
| O-Detune | fx | 600×480 | 35 | 0 / 0 / 0 | 0 | 4 | 16:1, 44:2, 64:2, 91:4 | 0 | `#F0E1CD` | Menlo 0.6% |
| O-DigiDelay | fx | 700×196 | 24 | 0 / 0 / 0 | 0 | 6 | 60:6 | 8 | `#CCB894` | Menlo 0.8% |
| O-Emulator | fx | 620×430 | 27 | 0 / 0 / 3 | 0 | 8 | 60:4 | 4 | `#CEBA96` | Menlo 0.5% |
| O-Formant | inst | 800×600 | 69 | 1 / 0 / 0 | 0 | 7 | 38:4, 55:17 | 9 | `#ECDFCE` | Menlo 0.2% |
| O-Freeze | fx | 550×530 | 27 | 0 / 2 / 0 | 0 | 5 | 60:8 | 4 | `#D9B98D` | Menlo 0.6% |
| O-FreqPulse | fx | 850×550 | 56 | 0 / 0 / 0 | 0 | 6 | — (sliders) | 23 | `#28201B` | Menlo 3.6% |
| O-Gain | fx | 380×500 | 73 | 0 / 0 / 0 | 0 | 7 | — (sliders) | 26 | `#C49D6B` | Menlo 0.4% |
| O-GrainScatter | fx | 900×800 | 96 | 0 / 37 / 1 | 0 | 4 | 48:31 | 1 | `#EEDCCA` | Menlo 0.2% |
| O-IntonationPad | inst | 800×500 | 54 | 0 / 0 / 0 | 0 | 6 | 52:5 | 31 | `#DABD96` | Apple Color Emoji 0.4%, Menlo 0.4% |
| O-Lyrica | inst | 700×450 | 74 | 0 / 0 / 0 | 0 | 5 | — (sliders) | 4 | `#D5B891` | Menlo 0.2% |
| O-Marimba | inst | 600×400 | 37 | 0 / 0 / 0 | 0 | 5 | 55:7, 60:1 | 3 | `#C4985D` | Menlo 0.5% |
| O-MicrotonalSampler | inst | 900×640 | 111 | 18 / 1 / 5 | 0 | 7 | 44:9 | 25 | `#E8D3BE` | Menlo 11.0% |
| O-MultiBandCompressor | fx | 900×640 | 126 | 0 / 0 / 0 | 0 | 6 | 44:3, 60:36 | 96 | `#DBD9C5` | Arial 0.4%, Menlo 0.1% |
| O-Octagon | fx | 1100×720 | 104 | 13 / 0 / 0 | 0 | 6 | 22:1 | 0 | `#241E1A` | Menlo 59.1%, Iowan Old Style 40.9% |
| O-Orbit | fx | 800×600 | 28 | 0 / 0 / 0 | 0 | 6 | 55:6 | 0 | `#E2D2BE` | Lucida Grande 0.3%, Menlo 0.3% |
| O-Polystutter | fx | 1000×690 | 151 | 0 / 54 / 0 | 1 | 8 | 32:8, 42:40 | 5 | `#DCBA8F` | Arial 0.4%, Menlo 0.2% |
| O-Prism | inst | 1200×800 | 146 | 0 / 0 / 0 | 0 | 6 | 44:2, 52:40, 64:6 | 1 | `#EDDECB` | Hiragino Sans 0.1%, Menlo 0.1% |
| O-Reed | inst | 900×600 | 97 | 0 / 0 / 0 | 0 | 5 | 68:26 | 1 | `#F5E6D3` | Menlo 0.1% |
| O-ReverseDelay | fx | 940×693 | 84 | 0 / 4 / 0 | 0 | 6 | 46:2, 56:6, 62:13 | 2 | `#E4D5C2` | Menlo 0.2%, Lucida Grande 0.2% |
| O-SpectralShaper | fx | 700×500 | 34 | 0 / 0 / 0 | 0 | 6 | 56:6 | 0 | `#493923` | Lucida Grande 2.5%, Menlo 0.5% |
| O-Strata | inst | 1200×800 | 152 | 0 / 0 / 0 | 0 | 6 | 44:2, 52:40, 64:6 | 1 | `#EDDECB` | Apple Symbols 0.5%, Hiragino Sans 0.1% |
| O-Tapestop | fx | 860×580 | 46 | 8 / 0 / 4 | 4 | 9 | 56:5 | 0 | `#E5D5C2` | Times New Roman 97.9%, Zapf Dingbats 1.2% |
| O-Texture | inst | 800×600 | 23 | 0 / 0 / 0 | 0 | 5 | 64:3 | 0 | `#E2D0BF` | Menlo 0.8% |
| O-TextureForge | inst | 900×600 | 29 | 0 / 0 / 2 | 0 | 6 | 38:5, 44:6 | 0 | `#EAD7C5` | Menlo 0.4% |
| O-Tremolo | fx | 600×400 | 17 | 0 / 0 / 0 | 0 | 7 | 60:2 | 47 | `#BCA66E` | Menlo 0.9% |
| O-Wind | inst | 900×600 | 70 | 0 / 0 / 0 | 0 | 5 | 72:20 | 2 | `#F6E9DB` | Menlo 0.2% |
| O-simpleAdditive | inst | 860×930 | 100 | 0 / 0 / 0 | 0 | 8 | 48:9, 56:6 | 1 | `#F2E2CF` | Apple Symbols 0.1% |
| O-simpleAmbisonics | fx | 1180×780 | 172 | 12 / 38 / 0 | 0 | 13 | 44:1, 54:6, 92:1 | 0 | `#E2D2BC` | Apple Symbols 0.3%, Georgia 0.1% |
| O-simpleBeatmaker | inst | 1060×900 | 109 | 0 / 0 / 0 | 0 | 9 | 38:8, 50:5, 118:4 | 60 | `#F2E7DC` | Georgia 0.2%, Menlo 0.2% |
| O-simpleFM | inst | 760×980 | 74 | 0 / 3 / 3 | 2 | 10 | 48:8, 56:7 | 0 | `#F0E2D2` | Arial Unicode MS 0.4%, Apple Symbols 0.3% |
| O-simpleGrain | inst | 900×760 | 65 | 23 / 2 / 2 | 15 | 9 | 48:4, 56:4 | 0 | `#EADECF` | Times New Roman 99.5%, Arial Unicode MS 0.3% |
| O-simplePhysicalModelSynth | inst | 1040×860 | 72 | 0 / 7 / 1 | 3 | 11 | 46:4, 56:10 | 0 | `#F4E5D2` | Arial Unicode MS 0.2%, Menlo 0.2% |
| O-simpleReverb | fx | 1180×780 | 115 | 2 / 3 / 0 | 0 | 10 | 54:4, 192:1 | 0 | `#EBDDCB` | Apple Symbols 0.2% |
| O-simpleSampler | inst | 980×720 | 66 | 0 / 0 / 2 | 0 | 8 | 48:15, 56:2 | 0 | `#F4E5D2` | Arial Unicode MS 0.3%, Apple Symbols 0.1% |
| O-simpleSubtractive | inst | 1180×1000 | 120 | 1 / 0 / 0 | 0 | 9 | 46:13, 54:12 | 0 | `#ECDDCC` | Apple Symbols 0.2% |
| O-simpleWavetable | inst | 1120×780 | 99 | 0 / 1 / 0 | 0 | 13 | 14:1, 44:9, 54:4, 92:1 | 0 | `#D1C0AD` | Apple Symbols 0.7% |
