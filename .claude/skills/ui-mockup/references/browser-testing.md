# Browser Testing Guide

## Purpose

Test UI in browser before integrating into plugin. Catch layout issues, interaction bugs, and visual problems early.

## Browser Test HTML Features

**v[N]-browser-test.html** is a standalone HTML file with:

1. **Production UI code** - Same HTML/CSS/JS as v[N]-ui.html
2. **Mock backend** - Simulates C++ parameter state
3. **Console logging** - Shows parameter messages
4. **No dependencies** - Opens directly in any browser
5. **No JUCE required** - Test without building plugin

## Testing Workflow

### Step 1: Open in Browser

```bash
open "plugins/[PluginName]/.ideas/mockups/v1-browser-test.html"
```

Or drag file to browser window.

### Step 2: Interact with Controls

- **Rotate knobs** - Click and drag vertically
- **Move sliders** - Click and drag horizontally
- **Toggle buttons** - Click to switch on/off
- **Select combos** - Click to show dropdown

### Step 3: Verify Behavior

**Check:**

- Controls respond to mouse/touch
- Values update in real-time
- Parameter ranges are correct
- Value formatting displays properly
- Layout adapts to window size (if resizable)

### Step 4: Check Console

Open browser console (F12 or Cmd+Option+I):

```
Parameter change: threshold = -30.5
Parameter change: ratio = 4.0
Parameter change: attack = 10.0
```

**Verify:**

- Parameter IDs match spec
- Values are in correct range
- Messages sent on every control interaction

### Step 5: Iterate if Needed

**Common issues:**

- Controls not aligned → Adjust CSS layout
- Values out of range → Fix min/max in YAML
- Labels cut off → Increase container width
- Poor contrast → Adjust colors

**Fix and regenerate:**

1. Update YAML spec
2. Regenerate files (re-run ui-mockup skill)
3. Test again in browser

### Step 6: Approve Design

When satisfied:

```
✓ Layout looks correct
✓ All controls functional
✓ Values display properly
✓ Console shows parameter messages
✓ No visual glitches

Approve this design and proceed to parameter-spec.md generation? (y/n): _
```

## Mock Backend Implementation

**Browser test includes mock C++ backend:**

```javascript
// Simulate C++ parameter state
const mockBackend = {
    parameters: {
        threshold: -20.0,
        ratio: 4.0,
        attack: 10.0,
        release: 100.0,
        gain: 0.0
    },

    // Simulate parameter change from C++ (automation/presets)
    simulateAutomation() {
        setInterval(() => {
            // Randomly adjust threshold
            const newValue = -60 + Math.random() * 60;
            receiveParameterUpdate('threshold', newValue);
        }, 2000);
    }
};

// Test automation simulation
// mockBackend.simulateAutomation(); // Uncomment to test automation
```

## Testing Checklist

- [ ] All controls visible and styled correctly
- [ ] Window size matches spec (600x400 or as specified)
- [ ] Controls respond to mouse interactions
- [ ] Values update in real-time
- [ ] Value formatting correct (units, decimals)
- [ ] Parameter ranges enforced (can't exceed min/max)
- [ ] Console logs parameter changes
- [ ] Layout adapts to window resize (if resizable)
- [ ] No visual glitches (overlapping, clipping)
- [ ] Text readable (sufficient contrast)
- [ ] Special elements functional (VU meter, analyzer, etc.)

## Cross-Browser Testing

**Test in multiple browsers:**

- Chrome/Edge (Chromium)
- Safari (WebKit)
- Firefox (Gecko)

**Why:** JUCE WebView uses platform-native engines:
- macOS → WebKit (Safari)
- Windows → Chromium (Edge)

**Ensure consistent behavior across browsers.**

## Performance Testing

**Open console Performance tab:**

1. Record while interacting with controls
2. Check frame rate (should be 60fps)
3. Look for layout thrashing
4. Verify no memory leaks

**Optimize if needed:**

- Use CSS transforms (GPU-accelerated)
- Throttle mouse events (requestAnimationFrame)
- Minimize DOM queries
- Avoid expensive reflows
