# HTML Generation Rules

## Key Principles

1. **Production-ready**: HTML IS the final plugin UI, not a prototype
2. **No external dependencies**: Pure HTML/CSS/JS only — everything is embedded via `juce_add_binary_data`
3. **JUCE 8 relay-state API**: `import * as Juce from './js/juce/index.js'`, then `Juce.getSliderState(id)`, `Juce.getToggleButtonState(id)`, `Juce.getComboBoxState(id)`. Every control reads and writes its parameter through that state object and nothing else.
4. **O-ReverseDelay is the reference implementation.** When this document is silent or ambiguous, do what O-ReverseDelay does. Its UI lives in four files plus the editor:
   - `plugins/O-ReverseDelay/Source/ui/public/index.html`
   - `plugins/O-ReverseDelay/Source/ui/public/css/styles.css`
   - `plugins/O-ReverseDelay/Source/ui/public/js/app.js`
   - `plugins/O-ReverseDelay/Source/ui/public/js/i18n.js`
   - `plugins/O-ReverseDelay/Source/PluginEditor.cpp`

   Read them with `git show HEAD:plugins/O-ReverseDelay/<path>`, never from the working tree: another session may be editing that plugin, and its uncommitted state is not the reference. Cite them by function or selector name (`bindKnob`, `updateKnobVisual`, `.knob-stem`), never by line number.
5. **Fixed frame by default** — see `ui-design-rules.md` Rule 4.

## HTML Structure

The production page is ONE complete file: inline `<style>` plus one inline `<script type="module">` controller. That is the shape gui-agent copies to `Source/ui/public/index.html`.

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>[PluginName]</title>
    <style>
        /* Inline CSS - no external stylesheets */
        body {
            margin: 0;
            padding: 0;
            font-family: Inter, system-ui, sans-serif;
            background: #2b2b2b;
            color: #ffffff;
            overflow: hidden;
        }

        /* Layout styles */
        /* Control styles — the knob rules are in "Rotary Knob — Family A" below */
        /* Animation styles */
    </style>
</head>
<body>
    <!-- UI structure matching YAML layout -->
    <div id="plugin-container">
        <div class="knob-cell">
            <div class="knob" id="knob-threshold" data-param="threshold"><div class="knob-stem"></div></div>
            <div class="knob-label">Threshold</div>
            <div class="knob-value" id="val-threshold">&#8212;</div>
        </div>
        <!-- ... more controls ... -->
    </div>

    <script type="module">
    import * as Juce from './js/juce/index.js';

    // ════ MODULE STATE — every module-level const/let lives in this block ════
    const KNOB_IDS = ['threshold' /* , ... every WebSliderRelay id */];

    // Units and decimals ONLY. Applied to st.getScaledValue(). No ranges here.
    const FORMAT = {
        threshold: (v) => `${v.toFixed(1)} dB`,
    };

    const KNOB_MIN_DEG       = -135;  // normalised 0.0
    const KNOB_MAX_DEG       = 135;   // normalised 1.0
    const DRAG_TRAVEL_PX     = 220;   // vertical px for a full 0→1 sweep
    const FINE_DRAG_RATE     = 0.2;   // Shift-drag speed
    const NUDGE_STEP         = 0.02;  // wheel / arrow-key step (floored at one param interval)
    const WHEEL_PX_PER_NUDGE = 100;
    const WHEEL_LINE_PX      = 33;    // deltaMode 1 (lines) → px
    const WHEEL_PAGE_PX      = 400;   // deltaMode 2 (pages) → px
    const WHEEL_MAX_NUDGES   = 4;     // per-event clamp
    const WHEEL_GESTURE_MS   = 250;   // one wheel burst = one automation gesture

    const sliderState = {};           // id -> Juce SliderState
    let paramDefaults = null;         // { id: engineeringDefault } from getParameterDefaults

    // ════ FUNCTION DECLARATIONS (hoisted — safe to call from init()) ════
    // normToDeg, scaledToNorm, updateKnobVisual, nudgeStep, stepBy, nudge,
    // resetToDefault, bindKnob, bindSelectCombo, bindToggle,
    // loadParameterDefaults ... (see the sections below)

    function init() {
        KNOB_IDS.forEach((id) => bindKnob(Juce, id));
        loadParameterDefaults(Juce);   // async; only dblclick-reset depends on it
    }

    // ════ ONE call, at the very BOTTOM of the module ════
    init();
    </script>
</body>
</html>
```

**Why that layout is mandatory:** a module-level statement that reaches a `const`/`let` binding declared further down throws a ReferenceError (the temporal dead zone) out of module evaluation. That kills the WHOLE UI, silently, while build, auval and pluginval all pass. So every binding is declared in the top block, everything else is a hoisted function declaration, and the single `init()` call is the last line of the module. Nothing is declared after it.

## Parameter Binding and Readouts

**The C++ `NormalisableRange` is the ONLY source of range and skew.** The page carries no range numbers at all:

- **Readout text:** `FORMAT[id](st.getScaledValue())`. `FORMAT[id]` holds units and decimals only. `getScaledValue()` is already in engineering units, with the parameter's own skew applied.
- **Knob angle:** `normToDeg(st.getNormalisedValue())`.
- **Defaults** (dblclick reset) come from the `getParameterDefaults` native function in engineering units, and convert back to normalised through `scaledToNorm(st, value)`, which reads the LIVE `st.properties` (`start`, `end`, `skew`).
- **Nudge step** (wheel, arrow keys) is floored at one parameter interval (`nudgeStep`). `setNormalisedValue()` snaps to the range's interval, so a nudge smaller than half a step rounds straight back and the control looks dead on stepped parameters.
- **Choice options** come from `st.properties.choices`, never from a JS list.
- **Every binder listens to BOTH `valueChangedEvent` and `propertiesChangedEvent`.** Properties (range, choices) can arrive after the first paint; a binder that only hears value changes renders against an empty range until the user touches it.

**Forbidden:** hand-mirrored min/max attributes on the markup, a JS table of ranges, and any linear `(value − min) / span` formula in the page. They agree with C++ only on linear parameters and silently drift on every skewed one (frequency, time, anything with a centre skew).

```javascript
function normToDeg(n) {
    return KNOB_MIN_DEG + n * (KNOB_MAX_DEG - KNOB_MIN_DEG);
}

// Inverse of getNormalisedValue(), from the LIVE properties the C++ range pushes.
function scaledToNorm(st, scaled) {
    const p = st.properties;
    const span = p.end - p.start;
    if (!isFinite(span) || span === 0) return 0;
    const proportion = Math.min(1, Math.max(0, (scaled - p.start) / span));
    return Math.pow(proportion, p.skew);
}

function updateKnobVisual(id) {
    const st = sliderState[id];
    if (!st) return;
    const text = FORMAT[id](st.getScaledValue());          // never a JS range map
    const knob = document.getElementById(`knob-${id}`);
    if (knob) {
        const stem = knob.querySelector('.knob-stem');
        if (stem) stem.style.transform =
            `translate(-50%, -100%) rotate(${normToDeg(st.getNormalisedValue())}deg)`;
        knob.setAttribute('aria-valuetext', text);         // same text as the readout
    }
    const valEl = document.getElementById(`val-${id}`);
    if (valEl) valEl.textContent = text;
}

function nudgeStep(st) {
    const p = st.properties;
    const span = p.end - p.start;
    if (!(p.interval > 0) || !isFinite(span) || span <= 0) return NUDGE_STEP;
    return Math.max(NUDGE_STEP, p.interval / span);
}

async function loadParameterDefaults(juce) {
    try {
        const raw = await juce.getNativeFunction('getParameterDefaults')();
        paramDefaults = typeof raw === 'string' ? JSON.parse(raw) : raw;
    } catch (e) {
        console.error('getParameterDefaults failed:', e);
        paramDefaults = null;   // dblclick becomes a no-op; nothing else is affected
    }
}
```

`FORMAT[id]` is called without a fallback on purpose: every `KNOB_IDS` member must have a `FORMAT` entry, and a missing one should fail loudly rather than render a unitless number.

## Control Implementations

### Rotary Knob — Family A (seed cross-section)

The house knob. The ring, segments and lighting never move; only the `.knob-stem` rotates (see `ui-design-rules.md` Rule 8).

**Markup** — the O-ReverseDelay `.knob-cell` shape. The value element starts with an em-dash until the first `updateKnobVisual()`:

```html
<div class="knob-cell">
    <div class="knob" id="knob-threshold" data-param="threshold"><div class="knob-stem"></div></div>
    <div class="knob-label">Threshold</div>
    <div class="knob-value" id="val-threshold">&#8212;</div>
</div>
```

`.knob-value` is **reserved for knob readouts.** UI gates census it to count knobs; a non-knob readout (a meter, a division display) gets its own class.

**CSS** — copied verbatim from O-ReverseDelay's committed `styles.css`. Copy the 20-stop conic-gradient; do not retype it.

```css
:root {
    --knob-ring:    #C9A27B;
    --knob-seg-1:   #F5DEB3;
    --knob-seg-2:   #E8D5B7;
    --knob-divider: #8B7355;
    --knob-core:    #FFF8DC;
    --knob-border:  #8B7355;
}

.knob {
  position: relative;
  width: 56px;
  height: 56px;
  border-radius: 50%;
  border: 2px solid var(--knob-border);
  cursor: ns-resize;
  transition: transform 0.1s ease;
  background:
    radial-gradient(circle, transparent 88%, var(--knob-ring) 88%, var(--knob-ring) 92%, var(--knob-divider) 92%, var(--knob-divider) 94%, transparent 94%),
    conic-gradient(from 0deg,
      var(--knob-seg-1) 0deg,   var(--knob-seg-1) 18deg,  var(--knob-divider) 18deg,  var(--knob-divider) 19deg,
      var(--knob-seg-2) 19deg,  var(--knob-seg-2) 36deg,  var(--knob-divider) 36deg,  var(--knob-divider) 37deg,
      var(--knob-seg-1) 37deg,  var(--knob-seg-1) 54deg,  var(--knob-divider) 54deg,  var(--knob-divider) 55deg,
      var(--knob-seg-2) 55deg,  var(--knob-seg-2) 72deg,  var(--knob-divider) 72deg,  var(--knob-divider) 73deg,
      var(--knob-seg-1) 73deg,  var(--knob-seg-1) 90deg,  var(--knob-divider) 90deg,  var(--knob-divider) 91deg,
      var(--knob-seg-2) 91deg,  var(--knob-seg-2) 108deg, var(--knob-divider) 108deg, var(--knob-divider) 109deg,
      var(--knob-seg-1) 109deg, var(--knob-seg-1) 126deg, var(--knob-divider) 126deg, var(--knob-divider) 127deg,
      var(--knob-seg-2) 127deg, var(--knob-seg-2) 144deg, var(--knob-divider) 144deg, var(--knob-divider) 145deg,
      var(--knob-seg-1) 145deg, var(--knob-seg-1) 162deg, var(--knob-divider) 162deg, var(--knob-divider) 163deg,
      var(--knob-seg-2) 163deg, var(--knob-seg-2) 180deg, var(--knob-divider) 180deg, var(--knob-divider) 181deg,
      var(--knob-seg-1) 181deg, var(--knob-seg-1) 198deg, var(--knob-divider) 198deg, var(--knob-divider) 199deg,
      var(--knob-seg-2) 199deg, var(--knob-seg-2) 216deg, var(--knob-divider) 216deg, var(--knob-divider) 217deg,
      var(--knob-seg-1) 217deg, var(--knob-seg-1) 234deg, var(--knob-divider) 234deg, var(--knob-divider) 235deg,
      var(--knob-seg-2) 235deg, var(--knob-seg-2) 252deg, var(--knob-divider) 252deg, var(--knob-divider) 253deg,
      var(--knob-seg-1) 253deg, var(--knob-seg-1) 270deg, var(--knob-divider) 270deg, var(--knob-divider) 271deg,
      var(--knob-seg-2) 271deg, var(--knob-seg-2) 288deg, var(--knob-divider) 288deg, var(--knob-divider) 289deg,
      var(--knob-seg-1) 289deg, var(--knob-seg-1) 306deg, var(--knob-divider) 306deg, var(--knob-divider) 307deg,
      var(--knob-seg-2) 307deg, var(--knob-seg-2) 324deg, var(--knob-divider) 324deg, var(--knob-divider) 325deg,
      var(--knob-seg-1) 325deg, var(--knob-seg-1) 342deg, var(--knob-divider) 342deg, var(--knob-divider) 343deg,
      var(--knob-seg-2) 343deg, var(--knob-seg-2) 360deg),
    radial-gradient(circle, var(--knob-core) 0%, var(--knob-core) 20%, transparent 20%);
  box-shadow:
    inset 1px 1px 3px rgba(0, 0, 0, 0.3),
    inset -1px -1px 2px rgba(255, 248, 220, 0.5),
    2px 2px 6px var(--shadow-medium);
}

.knob:hover  { transform: scale(1.05); }
.knob:active { transform: scale(0.98); }

.knob:focus-visible {
  outline: 2px dotted var(--green-mid);
  outline-offset: 3px;
}

/* Indicator: botanical stem line from centre to ~90% radius. */
.knob-stem {
  position: absolute;
  left: 50%;
  top: 50%;
  width: 2.5px;
  height: 24px;
  border-radius: 1.5px;
  background: var(--brown-frame);
  transform-origin: 50% 100%;
  transform: translate(-50%, -100%) rotate(0deg);
  pointer-events: none;
}
```

The stem is 24 px at a 56 px knob (~86 % of the radius). If a design scales the knob, scale the stem height with it (O-ReverseDelay's row-1 knobs are 62 px with a 27 px stem).

### Knob Interaction (bindKnob)

Every item below is present in O-ReverseDelay's committed `bindKnob`. Generate all of them for every knob; none is optional.

| Behaviour | What to emit | Why |
|---|---|---|
| **Focus + ARIA** | `tabindex="0"` and `role="slider"` on `.knob`; `updateKnobVisual` writes `aria-valuetext` with the same text as the readout | Keyboard and screen-reader users can reach and read every knob |
| **Arrow keys** | `keydown`: ArrowUp/ArrowRight nudge +1, ArrowDown/ArrowLeft nudge −1. Each nudge is bracketed by `sliderDragStarted()`/`sliderDragEnded()`, then `preventDefault()` | One host automation gesture per keypress; no page scroll |
| **Pointer down** | Close any open wheel gesture; record the origin; call `sliderDragStarted()`; `setPointerCapture(e.pointerId)` inside try/catch; attach `pointermove`, `pointerup`, `pointercancel` and `lostpointercapture` **on the knob, never on window** | Capture routes every later event for that pointer to the knob, even outside the WebView, and guarantees a terminating event |
| **Ending the gesture** | ONE idempotent `onUp`, guarded by the `dragging` flag: calls `sliderDragEnded()` exactly once, detaches all four listeners, `releasePointerCapture` inside try/catch | All three end events can fire for one drag; the guard makes that harmless |
| **Why capture matters** | — | Without it, a release outside the window, or a host modal grab, leaves the gesture open. In Logic and Live that latches automation write, and the knob keeps following the cursor with no button held |
| **Drag** | Relative vertical drag over `DRAG_TRAVEL_PX`. Shift drags at `FINE_DRAG_RATE`, and the origin is re-based whenever the modifier flips | A bare rate switch would jump the knob by the whole distance travelled so far |
| **Wheel** | Ignore horizontal-dominant events and do NOT `preventDefault` them. Register with `{ passive: false }`. One gesture bracket stays open for the burst, held by a `WHEEL_GESTURE_MS` timer | A sideways trackpad swipe must not walk the knob; one wheel burst = one automation touch, not a comb of tiny ones |
| **Double-click** | `resetToDefault`: the engineering default from `getParameterDefaults`, converted through `scaledToNorm`, bracketed as one gesture | The properties payload carries no default, and a JS default table would drift from C++ |

```javascript
function stepBy(st, nudges, id) {
    const n = Math.min(1, Math.max(0, st.getNormalisedValue() + nudges * nudgeStep(st)));
    st.setNormalisedValue(n);
    updateKnobVisual(id);
}

function nudge(st, dir, id) {
    st.sliderDragStarted();
    stepBy(st, dir, id);
    st.sliderDragEnded();
}

function resetToDefault(st, id) {
    if (!paramDefaults || !(id in paramDefaults)) return;
    const norm = scaledToNorm(st, Number(paramDefaults[id]));
    st.sliderDragStarted();
    st.setNormalisedValue(norm);
    st.sliderDragEnded();
    updateKnobVisual(id);
}

function bindKnob(juce, id) {
    const st = juce.getSliderState(id);
    sliderState[id] = st;

    st.valueChangedEvent.addListener(() => updateKnobVisual(id));
    st.propertiesChangedEvent.addListener(() => updateKnobVisual(id));
    updateKnobVisual(id);

    const knob = document.getElementById(`knob-${id}`);
    if (!knob) { console.error(`Missing knob element: knob-${id}`); return; }

    knob.setAttribute('tabindex', '0');
    knob.setAttribute('role', 'slider');
    knob.addEventListener('keydown', (e) => {
        let dir = 0;
        if (e.key === 'ArrowUp' || e.key === 'ArrowRight') dir = 1;
        else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') dir = -1;
        else return;
        endWheelGesture();
        nudge(st, dir, id);
        e.preventDefault();
    });

    let dragging = false, startY = 0, startNorm = 0, lastY = 0, fine = false;

    let wheelTimer = null;
    const endWheelGesture = () => {
        if (wheelTimer === null) return;
        clearTimeout(wheelTimer);
        wheelTimer = null;
        st.sliderDragEnded();
    };

    const onMove = (e) => {
        if (!dragging) return;
        if (e.shiftKey !== fine) {              // re-base on a modifier flip
            fine = e.shiftKey;
            startY = lastY;
            startNorm = st.getNormalisedValue();
        }
        lastY = e.clientY;
        const travel = fine ? DRAG_TRAVEL_PX / FINE_DRAG_RATE : DRAG_TRAVEL_PX;
        const n = Math.min(1, Math.max(0, startNorm + (startY - e.clientY) / travel));
        st.setNormalisedValue(n);
        updateKnobVisual(id);
        e.preventDefault();
    };

    // Idempotent: pointerup, pointercancel and lostpointercapture can all fire.
    const onUp = (e) => {
        if (!dragging) return;
        dragging = false;
        st.sliderDragEnded();
        knob.removeEventListener('pointermove', onMove);
        knob.removeEventListener('pointerup', onUp);
        knob.removeEventListener('pointercancel', onUp);
        knob.removeEventListener('lostpointercapture', onUp);
        if (e && e.pointerId !== undefined) {
            try { knob.releasePointerCapture(e.pointerId); } catch (_) { /* already released */ }
        }
    };

    knob.addEventListener('pointerdown', (e) => {
        endWheelGesture();
        dragging = true;
        startY = lastY = e.clientY;
        fine = e.shiftKey;
        startNorm = st.getNormalisedValue();
        st.sliderDragStarted();
        try { knob.setPointerCapture(e.pointerId); } catch (_) { /* older backends */ }
        knob.addEventListener('pointermove', onMove);
        knob.addEventListener('pointerup', onUp);
        knob.addEventListener('pointercancel', onUp);
        knob.addEventListener('lostpointercapture', onUp);
        e.preventDefault();
    });

    knob.addEventListener('wheel', (e) => {
        if (dragging) { e.preventDefault(); return; }
        if (Math.abs(e.deltaX) >= Math.abs(e.deltaY)) return;   // not ours; no preventDefault
        e.preventDefault();
        const px = Math.abs(e.deltaY) *
            (e.deltaMode === 1 ? WHEEL_LINE_PX : e.deltaMode === 2 ? WHEEL_PAGE_PX : 1);
        const nudges = Math.min(WHEEL_MAX_NUDGES, Math.max(1, px / WHEEL_PX_PER_NUDGE));
        if (wheelTimer === null) st.sliderDragStarted();
        else clearTimeout(wheelTimer);
        wheelTimer = setTimeout(endWheelGesture, WHEEL_GESTURE_MS);
        stepBy(st, e.deltaY < 0 ? nudges : -nudges, id);
    }, { passive: false });

    knob.addEventListener('dblclick', (e) => {
        endWheelGesture();
        resetToDefault(st, id);
        e.preventDefault();
    });
}
```

The C++ side must register `getParameterDefaults` (see `ui-finalization-agent.md` Phase 7). An unregistered native function never settles its promise, so dblclick reset would be silently dead.

### Toggles (Bool parameters)

A real `<button type="button">` carrying `aria-pressed`, bound to `getToggleButtonState`. State is painted with `aria-pressed` and classes only; the caption stays authored in the markup.

```html
<button type="button" class="toggle" id="toggle-bypass" aria-pressed="false">Bypass</button>
```

```javascript
function bindToggle(juce, id) {
    const st = juce.getToggleButtonState(id);
    const btn = document.getElementById(`toggle-${id}`);
    if (!btn) { console.error(`Missing toggle element: toggle-${id}`); return; }
    const paint = () => btn.setAttribute('aria-pressed', st.getValue() ? 'true' : 'false');
    st.valueChangedEvent.addListener(paint);
    st.propertiesChangedEvent.addListener(paint);
    paint();
    btn.addEventListener('click', () => st.setValue(!st.getValue()));
}
```

A Bool parameter has a ToggleState and NOT a SliderState or ComboBoxState. `getSliderState()` on a Bool id does not fail loudly; it builds a state the backend never updates. The C++ side must also register the toggle relay with `withOptionsFrom`, or `getToggleButtonState()` throws on load.

### Choices (Choice parameters)

A `<select>` rebuilt from `properties.choices` — O-ReverseDelay's `bindSelectCombo` pattern:

```html
<select class="combo" id="combo-mode"></select>
```

```javascript
function bindSelectCombo(juce, paramId) {
    const st = juce.getComboBoxState(paramId);
    const sel = document.getElementById(`combo-${paramId}`);
    if (!sel) { console.error(`Missing combo element: combo-${paramId}`); return null; }

    const buildOptions = () => {
        const choices = (st.properties && st.properties.choices) || [];
        if (choices.length === 0 || sel.options.length === choices.length) return;
        sel.textContent = '';
        choices.forEach((c, i) => {
            const opt = document.createElement('option');
            opt.value = String(i);
            opt.textContent = c;
            sel.appendChild(opt);
        });
    };

    const refresh = () => {
        buildOptions();                          // choices may arrive after first load
        const idx = st.getChoiceIndex();
        if (idx >= 0 && idx < sel.options.length) sel.selectedIndex = idx;
    };

    st.propertiesChangedEvent.addListener(refresh);
    st.valueChangedEvent.addListener(refresh);
    refresh();
    sel.addEventListener('change', () => st.setChoiceIndex(sel.selectedIndex));
    return st;
}
```

### Linear Fader (only when the design demands one)

Knobs are the default. When the design needs a fader, use a native `<input type="range">` over the **normalised** domain — never the engineering range — so the C++ range stays the only range:

```html
<div class="fader-cell">
    <input type="range" class="fader" id="fader-mix" min="0" max="1" step="any">
    <div class="fader-value" id="val-mix">&#8212;</div>
</div>
```

```javascript
function bindFader(juce, id) {
    const st = juce.getSliderState(id);
    const input = document.getElementById(`fader-${id}`);
    const valEl = document.getElementById(`val-${id}`);
    const paint = () => {
        input.value = st.getNormalisedValue();
        const text = FORMAT[id](st.getScaledValue());
        valEl.textContent = text;
        input.setAttribute('aria-valuetext', text);
    };
    st.valueChangedEvent.addListener(paint);
    st.propertiesChangedEvent.addListener(paint);
    paint();

    let open = false;
    const begin = () => { if (!open) { open = true; st.sliderDragStarted(); } };
    const end   = () => { if (open)  { open = false; st.sliderDragEnded(); } };
    input.addEventListener('pointerdown', begin);
    input.addEventListener('pointerup', end);
    input.addEventListener('pointercancel', end);
    input.addEventListener('lostpointercapture', end);
    input.addEventListener('input', () => {
        const adHoc = !open;                     // keyboard: bracket each change
        if (adHoc) st.sliderDragStarted();
        st.setNormalisedValue(parseFloat(input.value));
        if (adHoc) st.sliderDragEnded();
    });
}
```

## Styling Guidelines

**Use CSS custom properties for theming:**

```css
:root {
    --bg-color: #2b2b2b;
    --primary-color: #4a9eff;
    --text-color: #ffffff;
    --control-bg: #333333;
    --control-hover: #444444;
}

body {
    background: var(--bg-color);
    color: var(--text-color);
}

.control {
    background: var(--control-bg);
}

.control:hover {
    background: var(--control-hover);
}
```

**Ensure readability:**

- Minimum font size: 12px
- Sufficient contrast (WCAG AA)
- Clear visual hierarchy

## Responsive Design

**If window resizable:**

```css
@media (max-width: 500px) {
    .control {
        font-size: 0.9em;
    }
}

@media (min-width: 800px) {
    .control {
        font-size: 1.1em;
    }
}
```

## Performance

**Optimize for 60fps:**

- Use CSS transforms (GPU-accelerated)
- Throttle mouse events (requestAnimationFrame)
- Avoid layout thrashing
- Minimize DOM queries

```javascript
let animationId;
function updateKnob(paramID, value) {
    if (animationId) return;
    animationId = requestAnimationFrame(() => {
        // Update knob rotation
        animationId = null;
    });
}
```
