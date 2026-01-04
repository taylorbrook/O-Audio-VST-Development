---
plugin: LushVerb
date: 2025-11-11
problem_type: api_misuse
component: webview
symptoms:
  - WebView knobs display correctly but don't respond to mouse drag
  - Click/depress visual feedback works but knobs don't rotate
  - DAW automation works but UI doesn't reflect parameter changes
  - Browser console shows "Failed to get slider state" errors
root_cause: wrong_api
juce_version: 8.0.9
resolution_type: code_fix
severity: critical
tags: [webview, javascript, es6-modules, parameter-binding, frozen-ui, getsliderstate]
---

# WebView Knobs Frozen - ES6 Module Loading Mismatch

## Problem

LushVerb WebView UI displayed correctly with visible, clickable knobs, but all parameter knobs were completely unresponsive to mouse drag. The issue was an ES6 module export/import mismatch causing `getSliderState()` to be unavailable, preventing drag handlers from being attached.

## Environment

- Plugin: LushVerb (lush reverb with shimmer)
- JUCE Version: 8.0.9
- Affected: WebView UI (Stage 5 complete, post-implementation improvement)
- OS: macOS Darwin 24.6.0
- Date: 2025-11-11

## Symptoms

- **Knobs display correctly**: HTML/CSS renders properly, knobs visible with correct styling
- **Click/depress works**: CSS `:active` transform provides visual feedback on mousedown
- **Knobs don't rotate**: No rotation when dragging, knobs stay at default position
- **DAW automation works**: Parameters can be controlled from Ableton automation panel
- **UI doesn't reflect automation**: When automating from DAW, UI knobs don't move to show current value
- **Console error**: Browser console shows `"Failed to get slider state for [paramId]"` for all four parameters

## What Didn't Work

**Attempted Solution 1: Suspected stale build**
- Initially thought plugin binary was cached from old build (based on similar TapeAge pattern)
- Recommended clean rebuild and reinstall
- **Why it failed:** The issue wasn't a stale build - it was a fundamental JavaScript module loading error that would persist across any number of rebuilds

**Successful Investigation: Deep code analysis via troubleshooter agent**
- Analyzed `index.html` script loading mechanism (line 304)
- Examined `index.js` export syntax (lines 570-577)
- Traced `getSliderState()` call in HTML (line 340)
- Identified the module loading mismatch

## Solution

**Fix the ES6 module loading in `index.html`:**

### Change 1: Add `type="module"` to script tag

```html
<!-- Line 304 - BEFORE (broken): -->
<script src="js/juce/index.js"></script>

<!-- Line 304 - AFTER (fixed): -->
<script type="module" src="js/juce/index.js"></script>
```

### Change 2: Add import statement and module type to inline script

```html
<!-- Line 305 - BEFORE (broken): -->
<script>
    // Disable context menu
    document.addEventListener('contextmenu', e => e.preventDefault());
    // ... rest of code

<!-- Line 305-306 - AFTER (fixed): -->
<script type="module">
    import { getSliderState } from './js/juce/index.js';

    // Disable context menu
    document.addEventListener('contextmenu', e => e.preventDefault());
    // ... rest of code
```

### Change 3: Use imported function directly

```javascript
// Line 340 - BEFORE (broken):
if (window.__JUCE__ && window.__JUCE__.backend) {
    sliderState = window.__JUCE__.backend.getSliderState(paramId);
}

// Line 342 - AFTER (fixed):
if (window.__JUCE__ && window.__JUCE__.backend) {
    sliderState = getSliderState(paramId);
}
```

### Rebuild and install

```bash
# Build with updated HTML
./scripts/build-and-install.sh LushVerb

# Restart DAW to load new plugin binary
```

## Why This Works

### Root Cause: ES6 Module Export/Import Mismatch

**What was broken:**

1. **`index.js` uses ES6 exports** (line 570-577):
   ```javascript
   export {
     getNativeFunction,
     getSliderState,  // <-- exported function
     getToggleState,
     getComboBoxState,
     getBackendResourceAddress,
     ControlParameterIndexUpdater,
   };
   ```

2. **`index.html` loaded as regular script** (line 304):
   ```html
   <script src="js/juce/index.js"></script>
   <!-- Missing: type="module" -->
   ```

3. **Consequence:**
   - When ES6 modules are loaded via regular `<script>` tag (without `type="module"`), the browser ignores `export` statements
   - Functions are defined in module scope but NOT exposed globally
   - `getSliderState()` is unavailable to the inline script

4. **`index.html` tries to access as global** (line 340):
   ```javascript
   sliderState = window.__JUCE__.backend.getSliderState(paramId);
   // getSliderState is NOT on window.__JUCE__.backend - it's an ES6 export!
   ```

5. **Returns null** → Error check exits early (line 343-346):
   ```javascript
   if (!sliderState) {
       console.error(`Failed to get slider state for ${paramId}`);
       return;  // <-- No drag handlers attached, no listeners registered
   }
   ```

**Why the fix works:**

1. **`type="module"` enables ES6 module system:**
   - Browser treats script as ES6 module
   - Enables `import`/`export` syntax
   - Module scope is isolated (not global)

2. **Import statement accesses exported function:**
   ```javascript
   import { getSliderState } from './js/juce/index.js';
   ```
   - Brings `getSliderState()` into current module scope
   - Direct function reference (not via `window.__JUCE__.backend`)

3. **Direct call works:**
   ```javascript
   sliderState = getSliderState(paramId);  // ✅ Function is now accessible
   ```
   - Returns valid `SliderState` object
   - Passes null check → continues to attach drag handlers and listeners

4. **Handlers and listeners attach successfully:**
   - Lines 369-372: `valueChangedEvent.addListener()` → UI updates when DAW automates
   - Lines 378-408: Mousedown/mousemove handlers → Dragging works
   - Lines 411-421: Mouse wheel handlers → Scroll works
   - Lines 424-437: Double-click reset → Default value reset works

### Why All Symptoms Match

1. ✅ **Visual display works** → Static HTML/CSS renders regardless of JavaScript errors
2. ✅ **Click/depress works** → Pure CSS `:active` transform on line 147-149, no JavaScript needed
3. ✅ **DAW automation works** → C++ `WebSliderParameterAttachment` → APVTS is fine, backend binding works
4. ✅ **UI doesn't update from automation** → No listeners registered (line 369-372 never executes because `sliderState` is null)
5. ✅ **Dragging doesn't work** → Drag handlers never attached (lines 378-408 skipped after early return)
6. ✅ **Console error** → `console.error()` on line 344 executes because `getSliderState()` returns undefined

## Prevention

### 1. Always Use ES6 Module Syntax with JUCE's Official `index.js`

JUCE 8's WebView integration uses ES6 modules. When using the official JUCE `index.js`:

**Always load with `type="module"`:**
```html
<script type="module" src="js/juce/index.js"></script>
```

**Always import functions:**
```html
<script type="module">
    import { getSliderState, getToggleState } from './js/juce/index.js';

    // Now use getSliderState() directly
    const state = getSliderState('PARAM_ID');
</script>
```

### 2. Check Browser Console During Development

If knobs aren't responding:
1. Open browser dev tools (Safari: Develop menu → Show JavaScript Console)
2. Look for errors like `"Failed to get slider state for [paramId]"`
3. Check if functions are undefined: `console.log(typeof getSliderState)`

### 3. Verify Function Availability Early

Add debug logging to catch module loading issues:

```javascript
// Debug: Verify getSliderState is available
if (typeof getSliderState === 'undefined') {
    console.error('getSliderState is undefined - check module loading!');
}
```

### 4. Don't Mix Module Systems

**Avoid:**
```html
<!-- ❌ WRONG: ES6 exports loaded as regular script -->
<script src="js/juce/index.js"></script>
<script>
    // getSliderState is undefined here
    const state = window.__JUCE__.backend.getSliderState('PARAM');
</script>
```

**Correct:**
```html
<!-- ✅ CORRECT: ES6 module with proper import -->
<script type="module" src="js/juce/index.js"></script>
<script type="module">
    import { getSliderState } from './js/juce/index.js';
    const state = getSliderState('PARAM');
</script>
```

### 5. Reference Working Examples

When creating new WebView plugins, copy from working implementations that use proper ES6 module loading (e.g., after this fix, LushVerb v1.0.1 is a reference).

## Related Issues

- See also: [webview-knobs-frozen-multiple-causes-TapeAge-20251111.md](../gui-issues/webview-knobs-frozen-multiple-causes-TapeAge-20251111.md) - Different frozen knobs root causes (missing nullptr, missing check_native_interop.js, duplicate PLUGIN_CODE)
- See also: [webview-parameter-not-updating-wrong-event-format-JUCE-20251108.md](webview-parameter-not-updating-wrong-event-format-JUCE-20251108.md) - Different parameter binding issue (wrong event format)
- Related pattern: troubleshooting/patterns/juce8-critical-patterns.md #21 (WebView ES6 module loading)

## References

- JUCE 8 WebView API: ES6 module exports in `/Applications/JUCE/modules/juce_gui_extra/native/javascript/index.js`
- ES6 Module Specification: `import`/`export` syntax requires `type="module"`
- MDN Web Docs: [JavaScript modules](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Modules)
