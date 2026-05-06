# Quick Task 260408-vzl: Fix Tuning Module Integration - Research

**Researched:** 2026-04-09
**Domain:** WebView tuning panel module / CSS container queries
**Confidence:** HIGH

## Summary

The tuning module (`modules/tuning/scala-tuning-engine/`) and O-Prism's working tuning tab share identical JS/CSS files on disk, but **O-Prism does not actually use the module**. O-Prism has a fully inline implementation (~700 lines of procedural JS) in its `index.html` with absolute-positioned layout hardcoded for its exact window size. The module wraps the same logic in an ES6 class using CSS grid with fixed pixel column widths (`140px 1fr 200px`), which breaks when hosted in containers of different sizes.

Three core problems must be fixed in the module:

1. **Layout strategy mismatch:** Module uses CSS grid with fixed column widths. O-Prism uses absolute positioning. Neither approach adapts to varying container sizes. The module needs container queries.
2. **Missing JS methods:** O-Bells (the only plugin successfully using the module's class) added `noteOn()`, `noteOff()`, `updateHeldNotes()`, and `updateSpokeHighlights()` locally (123 extra lines). These belong in the module.
3. **Export mismatch:** O-Reed's integration calls `initTuningPanel()` and `module.default()` as a function, but the module exports a class. O-Reed's integration will silently fail.

**Primary recommendation:** Add container queries to the module CSS, backport O-Bells' missing methods to the module JS, and ensure the module fills any host container correctly.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Use CSS container queries so the module detects its container size and adjusts layout/font/spacing automatically
- Match O-Prism only -- find what's different between O-Prism's working integration and the module, fix those gaps
- No new features or API refactoring
- Module fills 100% of whatever container the host gives it
- Host plugin controls the size; module is fully responsive within that space

### Claude's Discretion
- Implementation details of container query breakpoints (thresholds, what changes at each size)
- Order of fixes if multiple gaps are found
</user_constraints>

## Findings

### Finding 1: O-Prism Does NOT Use the Module (HIGH confidence)

O-Prism's `index.html` contains ~700 lines of inline procedural JS implementing the tuning system directly. The tuning-panel.js and tuning-panel.css files exist in O-Prism's `Source/ui/public/js/` and `Source/ui/public/css/` but are identical copies of the module files -- they are NOT imported or used by the inline code.

O-Prism's inline implementation uses:
- **Absolute positioning** for all three columns (interval list, viz, controls)
- **Hardcoded pixel offsets:** `left: 14px`, `left: 150px; right: 230px`, `right: 14px; width: 210px`
- **Direct `Juce.getNativeFunction()` calls** (not through a class wrapper)
- **Additional UI elements** not in the module: held-notes-bar, deviation display per interval, color-coded interval cells in rotation view

### Finding 2: Module CSS Layout Will Not Adapt (HIGH confidence)

The module CSS (`.tuning-panel`) uses a fixed CSS grid:
```css
grid-template-columns: 140px 1fr 200px;
gap: 15px;
padding: 15px;
```

The "compact" variant only slightly shrinks columns:
```css
.tuning-panel.compact {
    grid-template-columns: 120px 1fr 160px;
}
```

**Problem:** At narrow widths (< ~500px), the fixed 140px + 200px sidebars plus gaps eat all the space, leaving the center viz with ~0 usable width. The compact class must be manually applied -- no auto-detection. There are zero `@container` queries.

**Container query strategy:**
- Module container needs `container-type: inline-size` on `.tuning-panel`
- Breakpoints based on container inline-size (not viewport):
  - **>= 600px:** Full 3-column grid (current layout)
  - **450-599px:** Compact 3-column (narrower sidebars, smaller fonts)
  - **< 450px:** Stack vertically -- controls below viz, interval list as horizontal scrollbar or collapsed

### Finding 3: Module JS Missing Methods Present in O-Bells (HIGH confidence)

O-Bells has a locally modified tuning-panel.js (1004 lines vs module's 881). The additions:

| Method | Lines | Purpose |
|--------|-------|---------|
| `noteOn(midiNote)` | 644-651 | Maps MIDI note to scale degree, highlights spoke |
| `noteOff(midiNote)` | 656-662 | Removes spoke highlight |
| `updateHeldNotes(notes, freqs)` | 631-638 | v3.1.0 method for accurate interval reporting |
| `updateSpokeHighlights()` | 667-677 | In-place SVG spoke color update without full redraw |

Also added constructor members:
```js
this.heldNotesMidi = [];
this.heldNotesFreqs = [];
this.activeScaleDegrees = new Set();
```

And `this.spokeElements` tracking in `drawPitchCircle()` for highlight updates.

These are needed for any plugin that wants real-time note highlighting on the pitch circle.

### Finding 4: O-Reed Integration Will Fail (HIGH confidence)

O-Reed's `index.html` line 1292 calls:
```js
module.initTuningPanel(container, Juce);
```

The module does NOT export `initTuningPanel`. It exports `class TuningPanel` as both named and default export. O-Reed's fallback:
```js
module.default(container, Juce);
```
This calls `TuningPanel(container, Juce)` as a function, but ES6 classes cannot be called without `new` -- this throws `TypeError: Class constructor TuningPanel cannot be invoked without 'new'`.

**Fix options (Claude's discretion):**
1. Add a convenience `initTuningPanel(container, juce)` named export to the module
2. Fix O-Reed's integration code to use `new module.TuningPanel(container, Juce)`

Option 1 is better since it prevents the same mistake in future plugins.

### Finding 5: SVG ViewBox is Fixed Size (MEDIUM confidence)

The pitch circle SVG uses `viewBox="0 0 188 188"` with hardcoded coordinates (cx=94, cy=94, r=88, r=73). The SVG viewBox itself scales correctly since it's responsive via `max-width`/`max-height`, but:
- O-Prism uses `.pitch-circle { width: 400px; height: 400px; }` and lets the SVG scale up
- Module uses `.pitch-circle svg { max-width: 188px; max-height: 188px; }` -- the SVG never grows beyond 188px even in a large container

The module should let the pitch circle SVG fill its container rather than capping at 188px.

### Finding 6: Polar Canvas is Hardcoded 180x180 (MEDIUM confidence)

Module HTML: `<canvas id="polar-canvas" width="180" height="180">`
O-Prism HTML: `<canvas id="polar-canvas" width="400" height="400">`

The module's polar canvas is tiny. The JS `drawPolarPlot()` reads `canvas.width/height` for rendering coordinates, so changing the HTML attribute directly affects render quality. This needs to be dynamic based on container size, ideally set in JS when the viz mode activates.

## Architecture Patterns

### Container Query Pattern for Module CSS
```css
.tuning-panel {
    container-type: inline-size;
    width: 100%;
    height: 100%;
}

/* Full layout >= 600px */
@container (min-inline-size: 600px) {
    .tuning-panel { grid-template-columns: 140px 1fr 200px; }
}

/* Compact layout 450-599px */
@container (min-inline-size: 450px) and (max-inline-size: 599px) {
    .tuning-panel { grid-template-columns: 110px 1fr 160px; font-size: 10px; }
}

/* Stacked layout < 450px */
@container (max-inline-size: 449px) {
    .tuning-panel {
        grid-template-columns: 1fr;
        grid-template-rows: auto 1fr auto;
    }
}
```

**Note:** Container queries require the container declaration on the **parent** of the queried element. Since `.tuning-panel` is the outermost element the module renders, the host's container div (e.g., `#tuning-container`) needs `container-type: inline-size`. The module should either:
- Set it on the host container in JS during `init()`
- Document that hosts must set it
- Use an inner wrapper div

### Convenience Export Pattern
```js
export class TuningPanel { ... }

export async function initTuningPanel(container, juceApi) {
    const panel = new TuningPanel(container, juceApi);
    await panel.init();
    return panel;
}

export default TuningPanel;
```

## Common Pitfalls

### Pitfall 1: Container Query Context
**What goes wrong:** `@container` queries on `.tuning-panel` won't work if `.tuning-panel` itself has `container-type` -- container queries query the **nearest containment context ancestor**, not the element itself.
**How to avoid:** The `container-type: inline-size` must be set on a parent wrapper. Either the module's render method wraps in `<div class="tuning-panel-root" style="container-type: inline-size">` or the host container gets it.

### Pitfall 2: Canvas DPR Scaling
**What goes wrong:** Canvas backing size (width/height attributes) differs from CSS display size, causing blurry rendering on Retina displays.
**How to avoid:** When resizing the polar canvas dynamically, set `canvas.width = clientWidth * devicePixelRatio` and scale the context.

### Pitfall 3: O-Prism Inline Code Drift
**What goes wrong:** Treating O-Prism as the source of truth but it has features the module doesn't need (held-notes-bar, deviation display). Backporting everything creates scope creep.
**How to avoid:** Only backport what the module already conceptually supports (noteOn/noteOff/updateHeldNotes from O-Bells). The held-notes-bar and deviation display are O-Prism-specific UI embellishments.

## Specific Gaps: Module vs O-Prism

| Feature | O-Prism (inline) | Module | O-Bells (modified module) | Action |
|---------|-------------------|--------|---------------------------|--------|
| Layout | Absolute positioning | CSS grid (fixed px) | CSS grid (fixed px) | Add container queries |
| SVG max size | 400x400 | 188x188 | 188x188 | Remove max cap, let SVG fill |
| Polar canvas | 400x400 | 180x180 | 180x180 | Make dynamic |
| noteOn/noteOff | Inline equivalent | Missing | Present (local) | Backport from O-Bells |
| updateHeldNotes | Inline equivalent | Missing | Present (local) | Backport from O-Bells |
| spokeHighlights | Inline equivalent | Missing | Present (local) | Backport from O-Bells |
| initTuningPanel export | N/A | Missing | Not needed (uses class) | Add convenience export |
| Held notes bar | Custom HTML element | Missing | Not present | Out of scope (O-Prism-specific) |
| Deviation display | Per-interval +/- cents | Missing | Not present | Out of scope (O-Prism-specific) |
| Color-coded intervals | Rotation view cells | Missing | Not present | Out of scope (O-Prism-specific) |

## Sources

### Primary (HIGH confidence)
- Direct file comparison: `modules/tuning/scala-tuning-engine/js/tuning-panel.js` (881 lines)
- Direct file comparison: `plugins/O-Prism/Source/ui/public/index.html` (inline tuning, ~700 lines JS)
- Direct file comparison: `plugins/O-Bells/Resources/ui/js/tuning-panel.js` (1004 lines, modified module)
- Direct file comparison: `plugins/O-Reed/Resources/ui/index.html` (broken integration pattern)

## Metadata

**Confidence breakdown:**
- Gap identification: HIGH - direct file comparison, byte-level diff
- Container query approach: HIGH - standard CSS spec, well-supported in WebView contexts
- O-Bells backport safety: HIGH - methods are additive, no breaking changes

**Research date:** 2026-04-09
**Valid until:** 2026-05-09
