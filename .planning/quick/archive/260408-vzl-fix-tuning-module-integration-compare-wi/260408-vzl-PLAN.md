---
phase: 260408-vzl
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - modules/tuning/scala-tuning-engine/js/tuning-panel.js
  - modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
autonomous: true
requirements: [TUNING-LAYOUT, TUNING-METHODS, TUNING-EXPORT]

must_haves:
  truths:
    - "Tuning module renders correctly in containers >= 600px wide (full 3-column grid)"
    - "Tuning module renders correctly in containers 450-599px wide (compact columns, smaller fonts)"
    - "Tuning module renders correctly in containers < 450px wide (stacked vertical layout)"
    - "Pitch circle SVG scales to fill its container instead of capping at 188px"
    - "Polar canvas resizes dynamically based on container size with DPR-aware rendering"
    - "noteOn/noteOff highlight spokes on the pitch circle in real time"
    - "initTuningPanel convenience export allows function-style initialization"
  artifacts:
    - path: "modules/tuning/scala-tuning-engine/js/tuning-panel.js"
      provides: "TuningPanel class with noteOn, noteOff, updateHeldNotes, updateSpokeHighlights, getNoteLabel, initTuningPanel export"
      contains: "export async function initTuningPanel"
    - path: "modules/tuning/scala-tuning-engine/snippets/tuning-panel.css"
      provides: "Container-query-based responsive layout"
      contains: "@container"
  key_links:
    - from: "tuning-panel.js drawPitchCircle()"
      to: "this.spokeElements"
      via: "Array of {line, dot, degree} refs stored during SVG generation"
      pattern: "this\\.spokeElements\\.push"
    - from: "tuning-panel.js noteOn()/noteOff()"
      to: "updateSpokeHighlights()"
      via: "activeScaleDegrees set add/delete triggers highlight update"
      pattern: "this\\.activeScaleDegrees\\.(add|delete)"
    - from: "tuning-panel.css @container queries"
      to: ".tuning-panel grid layout"
      via: "container-type on wrapper div, @container rules on .tuning-panel children"
      pattern: "@container.*min-inline-size"
---

<objective>
Fix the tuning module so it works correctly when integrated into any VST plugin regardless of window size. Backport missing methods from O-Bells' local modifications, add CSS container queries for responsive layout, fix SVG/canvas sizing, and add convenience export for easier integration.

Purpose: The module currently breaks in small containers (fixed pixel columns), lacks real-time note highlighting (only in O-Bells' local copy), and cannot be initialized as a function (breaking O-Reed).
Output: Updated tuning-panel.js and tuning-panel.css in the module directory.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/quick/260408-vzl-fix-tuning-module-integration-compare-wi/260408-vzl-CONTEXT.md
@.planning/quick/260408-vzl-fix-tuning-module-integration-compare-wi/260408-vzl-RESEARCH.md

<interfaces>
<!-- Current module exports (tuning-panel.js line 22, 881): -->
export class TuningPanel {
    constructor(containerElement, juceApi)
    async init()
    setHeldNotes(notes)
    // ... all other methods are internal
}
export default TuningPanel;

<!-- O-Bells additions to backport (tuning-panel.js lines 631-677, 995-1000): -->
// Constructor additions:
this.heldNotesMidi = [];
this.heldNotesFreqs = [];
this.activeScaleDegrees = new Set();

// Methods to add:
updateHeldNotes(notes, freqs)   // v3.1.0 held notes with frequencies
noteOn(midiNote)                // MIDI note -> scale degree -> highlight
noteOff(midiNote)               // Remove highlight
updateSpokeHighlights()         // Fast in-place SVG spoke color update
getNoteLabel(index, scaleSize)  // "C#" for 12-note, "3" for others

// drawPitchCircle changes:
// - Track this.spokeElements = [] array
// - Use activeScaleDegrees for initial spoke colors
// - Store {line, dot, degree} refs per spoke
// - SVG viewBox changed from "0 0 188 188" to "0 0 320 320"
// - cx/cy/radius changed from 94/94/73 to 160/160/125

<!-- O-Reed broken integration pattern (index.html line 1289-1296): -->
const module = await import('/js/tuning-panel.js');
if (module && module.initTuningPanel) {           // <-- needs this export
    module.initTuningPanel(container, Juce);
} else if (module && module.default) {
    module.default(container, Juce);              // <-- TypeError: class can't be called as function
}
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 1: Backport O-Bells methods and fix JS module</name>
  <files>modules/tuning/scala-tuning-engine/js/tuning-panel.js</files>
  <action>
Update the module's tuning-panel.js with all missing functionality from O-Bells' local copy. Specific changes:

**Constructor additions** (after `this.heldNotes = new Set();` at line 35):
- Add `this.heldNotesMidi = [];`
- Add `this.heldNotesFreqs = [];`
- Add `this.activeScaleDegrees = new Set();`

**SVG viewBox upgrade** (in render() method, line 74):
- Change `viewBox="0 0 188 188"` to `viewBox="0 0 320 320"`
- Update the three circle elements: outer `cx="160" cy="160" r="150"`, inner `cx="160" cy="160" r="125"`, center `cx="160" cy="160" r="4"`
- Change degree-labels font-size from `9` to `10`

**Polar canvas** (in render() method, line 85):
- Remove hardcoded `width="180" height="180"` attributes from the canvas element entirely (will be set dynamically in JS)

**drawPitchCircle() rewrite** (lines 382-432):
- Add `this.spokeElements = [];` at top of method
- Change `cx = 94, cy = 94, radius = 73` to `cx = 160, cy = 160, radius = 125`
- Use `activeScaleDegrees` for initial spoke colors: `const isActive = this.activeScaleDegrees.has(i);`
- Set spoke line color/width conditionally: active = `#C0392B`/`2.5`, default = `#5C4033`/`1.5`
- Set dot fill/radius conditionally: active = `#C0392B`/`6`, default = `#8B7355`/`5`
- Add `line.dataset.degree = i;` and `dot.dataset.degree = i;`
- Push `{ line, dot, degree: i }` to `this.spokeElements`
- Change label radius from `radius + 12` to `radius + 16`
- Use `this.getNoteLabel(i, count)` instead of `i.toString()` for label text

**drawPolarPlot() dynamic sizing** (lines 434-474):
- After getting the canvas element, add dynamic sizing:
  ```
  const rect = canvas.parentElement.getBoundingClientRect();
  const size = Math.min(rect.width, rect.height) - 20;
  const dpr = window.devicePixelRatio || 1;
  canvas.width = Math.max(size, 100) * dpr;
  canvas.height = Math.max(size, 100) * dpr;
  canvas.style.width = Math.max(size, 100) + 'px';
  canvas.style.height = Math.max(size, 100) + 'px';
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ```
- Use `Math.max(size, 100)` as the effective w/h for coordinate calculations instead of `canvas.width`/`canvas.height`

**Replace setHeldNotes() with updateHeldNotes()** (line 561-566):
- Replace existing `setHeldNotes(notes)` with `updateHeldNotes(notes, freqs)` matching O-Bells:
  ```
  updateHeldNotes(notes, freqs) {
      this.heldNotesMidi = notes || [];
      this.heldNotesFreqs = freqs || [];
      this.heldNotes = new Set(notes || []);
      if (this.currentVizMode === 'truekeys') {
          this.drawTrueKeys();
      }
  }
  ```
- Keep `setHeldNotes` as a backward-compatible alias: `setHeldNotes(notes) { this.updateHeldNotes(notes, []); }`

**Add noteOn(), noteOff(), updateSpokeHighlights()** (after updateHeldNotes):
- Backport all three methods exactly as in O-Bells (lines 644-677 of O-Bells file)

**Add getNoteLabel() helper** (before closing brace of class):
- Backport from O-Bells (lines 995-1000): returns note name for 12-note scales, degree number for others

**Use getNoteLabel in updateIntervalList()**: Replace `i.toString()` degree display in interval rows with `this.getNoteLabel(i, count)` where count = `this.intervals.length - 1`

**Add convenience export** (after `export default TuningPanel;` at end of file):
```js
export async function initTuningPanel(container, juceApi) {
    const panel = new TuningPanel(container, juceApi);
    await panel.init();
    return panel;
}
```

**Update version comment** at top from `v2.0.0` to `v3.0.0`.
  </action>
  <verify>
    <automated>grep -c "initTuningPanel\|noteOn\|noteOff\|updateSpokeHighlights\|updateHeldNotes\|getNoteLabel\|activeScaleDegrees\|spokeElements" modules/tuning/scala-tuning-engine/js/tuning-panel.js | xargs test 10 -le</automated>
  </verify>
  <done>Module JS has all O-Bells methods backported (noteOn, noteOff, updateHeldNotes, updateSpokeHighlights, getNoteLabel), SVG viewBox is 320x320, polar canvas is dynamically sized with DPR awareness, initTuningPanel convenience export exists, setHeldNotes remains as backward-compatible alias</done>
</task>

<task type="auto">
  <name>Task 2: Add CSS container queries for responsive layout</name>
  <files>modules/tuning/scala-tuning-engine/snippets/tuning-panel.css</files>
  <action>
Update the module CSS to use container queries so the panel adapts to any host container size. Per the user's locked decision: use CSS container queries, module fills 100% of host container, host controls size.

**Container query pitfall (from research):** `@container` queries query the nearest containment context ANCESTOR, not the element itself. So `container-type` must go on a wrapper, not `.tuning-panel` directly.

**Update render() wrapper in Task 1's JS:** The `render()` method wraps content in `<div class="tuning-panel">`. Update JS (Task 1) to wrap in `<div class="tuning-panel-root"><div class="tuning-panel">...</div></div>` where `.tuning-panel-root` gets the container-type.

**Add `.tuning-panel-root` rule** (new, at top of MAIN CONTAINER section):
```css
.tuning-panel-root {
    container-type: inline-size;
    width: 100%;
    height: 100%;
}
```

**Update `.tuning-panel` base rule** (line 28-38):
- Remove fixed `grid-template-columns: 140px 1fr 200px;` from the base rule
- Set base to: `width: 100%; height: 100%; box-sizing: border-box;`
- Keep existing properties (display: grid, gap, padding, background, etc.)
- Set default grid as single column for mobile-first: `grid-template-columns: 1fr;`

**Add container query breakpoints** (after the base `.tuning-panel` rule):

```css
/* Full 3-column layout >= 600px */
@container (min-inline-size: 600px) {
    .tuning-panel {
        grid-template-columns: 140px 1fr 200px;
        gap: 15px;
        padding: 15px;
        font-size: 11px;
    }
}

/* Compact 3-column layout 450-599px */
@container (min-inline-size: 450px) and (max-inline-size: 599px) {
    .tuning-panel {
        grid-template-columns: 110px 1fr 150px;
        gap: 8px;
        padding: 8px;
        font-size: 10px;
    }
    .tuning-panel .interval-list { max-height: 200px; }
    .tuning-panel .viz-container { min-height: 150px; }
    .tuning-panel .ref-knob { width: 40px; height: 40px; }
}

/* Stacked layout < 450px */
@container (max-inline-size: 449px) {
    .tuning-panel {
        grid-template-columns: 1fr;
        grid-template-rows: auto 1fr auto;
        gap: 6px;
        padding: 6px;
        font-size: 10px;
    }
    .tuning-panel .interval-list { max-height: 120px; }
    .tuning-panel .viz-container { min-height: 120px; }
    .tuning-panel .viz-mode-toggle { flex-wrap: wrap; }
    .tuning-panel .viz-btn { font-size: 8px; padding: 4px 6px; }
    .tuning-panel .tuning-file-buttons { grid-template-columns: 1fr; }
    .tuning-panel .ref-knob { width: 36px; height: 36px; }
}
```

**Remove the old `.tuning-panel.compact` section** (lines 599-611) -- it is superseded by container queries.

**Fix pitch circle SVG sizing** (line 215-218):
- Replace `.pitch-circle svg { max-width: 188px; max-height: 188px; }` with:
```css
.pitch-circle svg {
    width: 100%;
    height: auto;
    max-width: 320px;
}
```

**Fix polar canvas sizing** (add new rule):
```css
#polar-canvas {
    width: 100%;
    height: auto;
    max-width: 320px;
}
```

**IMPORTANT for Task 1 coordination:** The JS `render()` method must output the wrapper div:
```html
<div class="tuning-panel-root">
  <div class="tuning-panel">
    ...existing content...
  </div>
</div>
```
This was noted above as a JS change needed in Task 1. Ensure Task 1 includes this wrapper.
  </action>
  <verify>
    <automated>grep -c "@container\|tuning-panel-root\|container-type" modules/tuning/scala-tuning-engine/snippets/tuning-panel.css | xargs test 4 -le</automated>
  </verify>
  <done>CSS has container-type on .tuning-panel-root wrapper, three @container breakpoints (>=600px full, 450-599px compact, <450px stacked), SVG max-width is 320px not 188px, old .compact class removed, polar canvas styles added</done>
</task>

</tasks>

<verification>
1. `grep -c "initTuningPanel" modules/tuning/scala-tuning-engine/js/tuning-panel.js` returns >= 2 (function def + export)
2. `grep -c "@container" modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` returns >= 3 (one per breakpoint)
3. `grep "container-type" modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` returns `container-type: inline-size`
4. `grep "viewBox" modules/tuning/scala-tuning-engine/js/tuning-panel.js` returns `0 0 320 320`
5. `grep "noteOn\|noteOff\|updateSpokeHighlights\|updateHeldNotes\|getNoteLabel" modules/tuning/scala-tuning-engine/js/tuning-panel.js | wc -l` returns >= 10
6. `grep "188" modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` returns nothing (old 188px caps removed)
7. `grep "tuning-panel-root" modules/tuning/scala-tuning-engine/js/tuning-panel.js` confirms wrapper div in render()
</verification>

<success_criteria>
- Module JS exports `TuningPanel` class, `initTuningPanel` function, and default export
- All 5 O-Bells methods backported: noteOn, noteOff, updateHeldNotes, updateSpokeHighlights, getNoteLabel
- setHeldNotes still works as backward-compatible alias
- SVG pitch circle uses 320x320 viewBox, scales responsively via CSS
- Polar canvas sizes dynamically with DPR-aware rendering
- CSS uses 3 container query breakpoints (full, compact, stacked)
- No hardcoded 188px or 180px caps remain
- Old .compact class removed (replaced by container queries)
</success_criteria>

<output>
After completion, create `.planning/quick/260408-vzl-fix-tuning-module-integration-compare-wi/260408-vzl-01-SUMMARY.md`
</output>
