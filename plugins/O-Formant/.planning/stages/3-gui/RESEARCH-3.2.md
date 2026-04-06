# Stage 3: GUI Phase 3.2 - Research

## Date: 2026-04-05

## 1. Formant Peaks Overlay (JS-side Computation)

### Data to Port from C++

**VowelData.h constants** (5 vowels, each with F1-F5 freq/bw/gain + XY position):

```javascript
const VOWELS = [
  { name: 'A', x: 0.83, y: 0.00,
    freq: [600, 1040, 2250, 2450, 2750],
    bw: [60, 70, 110, 120, 130],
    gain: [1.0, 0.4467, 0.3548, 0.3548, 0.1000] },
  { name: 'E', x: 0.31, y: 0.43,
    freq: [400, 1620, 2400, 2800, 3100],
    bw: [40, 80, 100, 120, 120],
    gain: [1.0, 0.2512, 0.3548, 0.2512, 0.1259] },
  { name: 'I', x: 0.00, y: 1.00,
    freq: [250, 1750, 2600, 3050, 3340],
    bw: [60, 90, 100, 120, 120],
    gain: [1.0, 0.0316, 0.1585, 0.0794, 0.0398] },
  { name: 'O', x: 1.00, y: 0.35,
    freq: [400, 750, 2400, 2600, 2900],
    bw: [40, 80, 100, 120, 120],
    gain: [1.0, 0.2818, 0.0891, 0.1000, 0.0100] },
  { name: 'U', x: 0.98, y: 0.93,
    freq: [350, 600, 2400, 2675, 2950],
    bw: [40, 80, 100, 120, 120],
    gain: [1.0, 0.1000, 0.0251, 0.0398, 0.0158] },
];
```

### Shepard IDW Interpolation (from VowelMorpher.h)

```javascript
function computeFormants(cursorX, cursorY, focus) {
  const weights = [];
  let weightSum = 0;
  for (const v of VOWELS) {
    const dx = cursorX - v.x;
    const dy = cursorY - v.y;
    const dist = Math.sqrt(dx * dx + dy * dy);
    if (dist < 1e-6) {
      // Snap to this vowel
      return { freq: [...v.freq], bw: [...v.bw], gain: [...v.gain] };
    }
    const w = 1.0 / Math.pow(dist, focus);
    weights.push(w);
    weightSum += w;
  }
  // Normalize
  const invSum = 1.0 / weightSum;
  const freq = [0, 0, 0, 0, 0];
  const bw = [0, 0, 0, 0, 0];
  const gain = [0, 0, 0, 0, 0];
  for (let v = 0; v < 5; v++) {
    const nw = weights[v] * invSum;
    for (let f = 0; f < 5; f++) {
      freq[f] += nw * Math.log(VOWELS[v].freq[f]); // log-domain freq
      bw[f]   += nw * VOWELS[v].bw[f];
      gain[f] += nw * VOWELS[v].gain[f];
    }
  }
  for (let f = 0; f < 5; f++) freq[f] = Math.exp(freq[f]);
  return { freq, bw, gain };
}
```

### FormantShift + FormantSpread (from FormantFilterBank.h lines 42-61)

```javascript
function applyShiftSpread(freq, shift, spread) {
  const shiftFactor = Math.pow(2, shift / 12);
  const shifted = freq.map(f => f * shiftFactor);
  const centerOfMass = shifted.reduce((a, b) => a + b, 0) / 5;
  return shifted.map(f => {
    const distance = f - centerOfMass;
    return Math.max(20, centerOfMass + distance * spread);
  });
}
```

### Display Strategy

**Chosen approach** (from CONTEXT-3.2.md): F1-F5 dot markers along a frequency axis near the bottom of the XY pad.

- X axis: log-frequency mapping. F1-F5 range spans ~200-5000 Hz.
  - `xNorm = (Math.log(freq) - Math.log(200)) / (Math.log(5000) - Math.log(200))`
  - This maps 200 Hz to left edge, 5000 Hz to right edge
- Y position: Use interpolated gain values to set height. Higher gain = higher dot.
  - `yNorm = gain * 0.3` (scale to use bottom ~30% of pad)
  - Position dots in lower portion so they don't obscure cursor/vowel labels
- Dot styling: 3-4px radius circles, moss green (#8BA870) at 0.5 opacity
- Labels: "F1"-"F5" in 8px font below each dot

### Relay Dependencies

Recompute formants on change of any of these 5 relays:
- `vowelXState` (cursor position)
- `vowelYState` (cursor position)
- `vowelFocusState` (Shepard IDW power)
- `formantShiftState` (semitone shift, range -24 to +24)
- `formantSpreadState` (spread multiplier, range 0.5 to 2.0)

**Integration point:** Add formant dot drawing at the end of `drawXYPad()`. Also add listeners on formantShift/formantSpread/vowelFocus to trigger `drawXYPad()`.

### Performance

~50 FLOPs per formant computation (5 vowels x 5 formants, a few log/exp calls). Triggered only on relay value changes, not per-frame. No performance concern.

## 2. Cursor Glow

### Canvas radialGradient

Already used in codebase:
- `O-SpectralShaper/Resources/ui/js/components/CurveEditor.js:282`
- `O-Orbit/Resources/ui/js/app.js:375`

Pattern:
```javascript
const grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, 28);
grad.addColorStop(0, 'rgba(139, 168, 112, 0.3)');
grad.addColorStop(1, 'rgba(139, 168, 112, 0.0)');
ctx.fillStyle = grad;
ctx.beginPath();
ctx.arc(cx, cy, 28, 0, Math.PI * 2);
ctx.fill();
```

**Insert before** the existing cursor dot/crosshair drawing in `drawXYPad()`, after computing `cx`/`cy` (line ~199 of main.js).

## 3. ADSR Curve Display

### JUCE ADSR Curve Math

**All segments are LINEAR** (verified from `juce_ADSR.h`):
- Attack: linear ramp 0 → 1.0
- Decay: linear ramp 1.0 → sustain level
- Release: linear ramp sustain → 0

No exponential curves. Canvas `lineTo()` is sufficient for each segment.

### Drawing Algorithm

Given attack (ms), decay (ms), sustain (0-1), release (ms):

```javascript
function drawADSR(ctx, w, h, attack, decay, sustain, release) {
  const total = attack + decay + release;
  const scale = total > 0 ? (w - 8) / total : 1; // 4px padding each side
  const x0 = 4;
  const yBot = h - 4;
  const yTop = 4;

  ctx.beginPath();
  ctx.moveTo(x0, yBot);
  // Attack: 0 → peak
  const xA = x0 + attack * scale;
  ctx.lineTo(xA, yTop);
  // Decay: peak → sustain
  const xD = xA + decay * scale;
  const ySus = yBot - sustain * (yBot - yTop);
  ctx.lineTo(xD, ySus);
  // Sustain: hold (fixed-width visual segment, ~20% of canvas)
  const sustainWidth = (w - 8) * 0.2;
  const xS = xD + sustainWidth;
  ctx.lineTo(xS, ySus);
  // Release: sustain → 0
  const xR = xS + release * scale;
  ctx.lineTo(xR, yBot);

  ctx.strokeStyle = '#8BA870';
  ctx.lineWidth = 1.5;
  ctx.stroke();
}
```

**Note:** Sustain has no time component (it's a level, not a duration). Standard ADSR visualizations allocate a fixed-width segment for the sustain hold phase. Use ~20% of canvas width.

**Revised total width:** Scale A+D+R proportionally into 80% of canvas, sustain hold gets 20%.

### HTML Changes

Add ADSR canvas in the Envelope bottom-group, after the knob row:

```html
<div class="bottom-group">
  <h3>Envelope</h3>
  <div class="param-row">
    <!-- existing 4 knobs -->
  </div>
  <canvas id="adsr-canvas" width="240" height="50"></canvas>
</div>
```

CSS: `#adsr-canvas { width: 100%; height: 50px; margin-top: 4px; }` — DPR scaling in JS.

### Layout Impact

The Envelope bottom-group has `flex: 1` in `.bottom-row`. Currently the param-row with 4 knobs is ~90px tall (55px knob + labels). The bottom-row grid row is 170px. Adding a 50px canvas below the knobs fits within the 170px allocation: 8px padding top + ~70px knobs + 4px gap + 50px canvas + 8px padding = 140px.

### Relay Dependencies

Redraw ADSR canvas on change of: `attackState`, `decayState`, `sustainState`, `releaseState`.

## 4. Files to Modify

| File | Changes |
|------|---------|
| `Source/ui/public/js/main.js` | Add VOWELS constant, computeFormants(), applyShiftSpread(), formant dot drawing in drawXYPad(), cursor glow gradient, ADSR canvas setup/draw, additional relay listeners |
| `Source/ui/public/index.html` | Add `<canvas id="adsr-canvas">` in Envelope group, CSS for canvas |

**No C++ changes.** No new relays needed.

## 5. Risks & Pitfalls

| Risk | Mitigation |
|------|------------|
| Formant dots obscure vowel labels | Position dots in lower 30% of pad (gain-based Y), labels are in upper/middle |
| DPR not applied to ADSR canvas | Use same setupCanvas pattern: `canvas.width = clientWidth * dpr`, `ctx.setTransform(dpr, ...)` |
| formantShift/formantSpread relay value is scaled (not normalised) | Use `state.getScaledValue()` not `getNormalisedValue()` for shift/spread in formant computation |
| ADSR draws outside canvas when times are very large | Clamp total A+D+R to canvas width; sustain gets fixed 20% |
| JS formant values don't match C++ exactly | Use identical constants from VowelData.h and identical Shepard algorithm. Minor float differences acceptable for visualization. |

## 6. Existing Patterns to Reuse

- **Radial gradient**: O-SpectralShaper `CurveEditor.js:282`, O-Orbit `app.js:375`
- **DPR canvas setup**: Current `setupCanvas()` in main.js (lines 104-115)
- **Relay value listeners**: Current `valueChangedEvent.addListener()` pattern (lines 85-95)
