---
plugin: O-Wind
date: 2026-09-27
problem_type: ui_layout
component: webview
symptoms:
  - "measure-ui --contrast: disabled ADSR knob captions 1.65:1 and readouts 2.07:1 in the DEFAULT state (ADSR off) — worst text on the page"
  - "Template AA walnut #715D45 still fails (4.44) on the shared tuning panel's selected interval row #E0DAB8"
  - "White text on --tuning-accent fills (Generate, Export HTML) 3.75:1"
  - "After moving to bundled EB Garamond, check-ui-labels [4] 'Tonique' 41.5>40.0 and [7] #octave-stretch dx=1.0 in fr"
root_cause: logic_error
juce_version: 8.0.15
resolution_type: code_fix
severity: moderate
tags: [contrast, wcag-aa, opacity, disabled-state, tuning-panel, eb-garamond, width-pin, r4, r5]
---

# Troubleshooting: disabled-row opacity, shared-panel contrast and face-change width pins (R4/R5 pass)

## Problem
The O-Wind v1.22.0 UI pass (review 260924-nho, R4 and R5) turned up three
defects the palette swap alone would not have fixed:
1. Whole-row opacity on disabled or bypassed knob rows, which is the page's
   default state.
2. The template's AA walnut is bound to paper, and it fails on the shared tuning
   panel's highlight tint.
3. Width floors measured in the old face broke when the face changed.

## Environment
- Plugin: O-Wind 1.21.0 → 1.22.0 (commit 7c06d1a3)
- JUCE Version: 8.0.15
- Affected: `Resources/ui/index.html` (inline CSS), with the shared scala-tuning-engine panel mounted inside it
- Date: 2026-09-27

## Symptoms
- **50 of 177 text nodes below AA** in each language (min 1.65). The two
  largest groups:
  - `.adsr-disabled` (opacity 0.35 on the row) composited `#6B5B4F` captions to
    `#C7B9AC`;
  - `--tuning-text-muted: #8B7355` on the interval list read 3.42, across 12
    rows × 2 spans.
- **A real-pixel probe found 87 per language**, because it also covered the
  bypassed Effects rows (opacity 0.35, 1.8:1) and the bypass button's
  `rgba(60,47,47,0.4)` text (2.06).

## What Didn't Work

**Using the template's `#715D45` for the tuning muted text.** It passes on
`#EFDECC` (4.77) but reads **4.44** on the selected-row tint `#E0DAB8`. The
template binds each variant to paper grounds, and the shared panel adds grounds
the template never lists.

**Dimming only the knob graphic in bypassed FX rows (first pass).** It passed
AA, but a bypassed row then looked active, because bold `#3C2F2F` captions stayed
at full strength. The before/after screenshot caught this; the gates could not.

## Solution

**1. Dim the graphic, step the text down to an AA colour:**
```css
/* before */
.adsr-disabled { opacity: 0.35; pointer-events: none; }
.fx-section.bypassed .fx-knobs { opacity: 0.35; pointer-events: none; }

/* after */
.adsr-disabled { pointer-events: none; }
.adsr-disabled .knob-wrapper { opacity: 0.35; }
.adsr-disabled .knob-value { color: var(--text-walnut-mid); }   /* #715D45, 5.38 */
.fx-section.bypassed .fx-knobs { pointer-events: none; }
.fx-section.bypassed .knob-visual { opacity: 0.35; }
#tab-effects .fx-section.bypassed .knob-label,
#tab-effects .fx-section.bypassed .knob-value { color: var(--text-walnut-mid); }
```
Leave a dropdown inside the row (`.fx-dropdown`) at full opacity. Its text is text.

**2. Fix the shared panel from the plugin's side:**
```css
#tuning-container .tuning-panel { --tuning-text-muted: #6A5640; } /* 4.93 on #E0DAB8 */
#tuning-container .generator-btn,
#tuning-container .tuning-export-btn {
    background: var(--text-sage-mid);            /* #4E6839: white 6.24 */
    border-color: var(--text-sage-mid);
}
```
Do NOT darken `--tuning-accent` itself. It also drives input focus borders, the
active viz-tab border and the stretch-slider gradient, so the change would spread
past the two text-bearing fills. Do not edit
`modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` either, because it
has 5 consumers.

**3. Re-pin width floors in the new face.** The panel had resolved to Times New
Roman. In EB Garamond, the uppercase, letter-spaced French captions got
**wider**: "Tonique" went from 39.83 to 41.5, and "Étirement" from 51 to 52.02.
```css
#tuning-container .tonic-label          { min-width: 41.6px; }  /* was 39.83 */
#tuning-container .octave-stretch-label { min-width: 52.1px; }  /* was 51 */
```

## Why This Works
1. **Opacity is a contrast change,** and a disabled or bypassed state is a state
   users look at. On O-Wind it is the state they see first: ADSR is off by
   default. WCAG exempts inactive controls, but the readouts still carry values
   the user reads. Dimming the graphic and stepping the text to an AA-passing
   walnut keeps the state legible and still distinct.
2. **The AA variants are bound to a background.** A shared panel brings its own
   tints (selected row, hover), so the worst ground a colour actually lands on
   decides the colour, not the paper the template assumed.
3. **EB Garamond is about 5% narrower than Times in lowercase, but not in every
   string.** Uppercase with letter-spacing, the case every one of these captions
   uses, can come out wider. Any floor recorded as "the widest arm measured" is a
   measurement of the old face.

## Prevention
- **Search the page for every opacity** that sits on a container holding text:
  `grep -n "opacity: 0\.[0-4]"`. Check each disabled or bypassed state with the
  pixel probe. `measure-ui` only measures the states in `tests/i18n-states.json`.
- **For a shared-panel colour, compute the ratio on every tint the panel paints**
  (selected, hover and active rows), not only on its base background.
- **After a face change, run check-ui-labels before anything else.** Re-measure
  and re-pin every `min-width` or `width` floor that has a "measured" comment.
  Don't widen by guesswork.
- **Take a before/after screenshot of each disabled state.** Passing AA can still
  erase the state's meaning.

## Related Issues
- Memory: `pattern_text_opacity_is_a_contrast_change_use_a_chip`
- Memory: `pattern_eb_garamond_migration_glyph_and_figure_traps`
- Memory: `pattern_paper_jpg_contrast_needs_image_probe` (probe method and its traps)
