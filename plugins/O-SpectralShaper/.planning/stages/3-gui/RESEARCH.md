# Stage 3: GUI - Research Findings

**Researched:** 2026-02-03
**Plugin:** O-SpectralShaper
**Phase:** Research
**Confidence:** HIGH

---

## Summary

This document consolidates research for implementing the Stage 3 GUI, which requires:
1. **Phase 3.1:** WebView layout with 6 parameter knobs + dark botanical theme
2. **Phase 3.2:** Drawable curve editors (freehand + node modes) for attack/sustain
3. **Phase 3.3:** Real-time WebGL spectrogram with transient heat overlay

All patterns have been validated against existing Ouaricon plugins and JUCE 8 documentation.

---

## 1. WebView Layout & Parameter Binding (Phase 3.1)

### 1.1 Required Infrastructure (Already Implemented in Stage 1)

The existing PluginEditor.h/cpp already has:
- ✅ 7 WebSliderRelay instances (Mix, AttackTime, SustainTime, Sensitivity, LookaheadEnabled, LookaheadTime, OutputGain)
- ✅ WebBrowserComponent with resourceProvider
- ✅ WebSliderParameterAttachment with JUCE 8 3-parameter signature
- ✅ Correct member initialization order (relays → webView → attachments)

**Minimal Stage 3.1 Changes:**
- Replace placeholder index.html with full botanical-themed layout
- Add rotary knob visuals with CSS/SVG
- Wire existing JUCE relays to JavaScript controls

### 1.2 JavaScript Parameter Binding Pattern

From O-Lyrica reference implementation:

```javascript
import * as Juce from './js/juce/index.js';

function bindSlider(paramId, updateDisplay) {
    const sliderState = Juce.getSliderState(paramId);
    const element = document.getElementById(paramId);

    // Initialize with current value
    const initialValue = sliderState.getNormalisedValue();
    updateDisplay(initialValue);

    // UI → C++ (user drags knob)
    element.addEventListener('input', (e) => {
        const value = parseFloat(e.target.value);
        sliderState.setNormalisedValue(value);
        updateDisplay(value);
    });

    // C++ → UI (automation, preset load)
    // CRITICAL: No callback parameters in JUCE 8 valueChangedEvent
    sliderState.valueChangedEvent.addListener(() => {
        const value = sliderState.getNormalisedValue();
        element.value = value;
        updateDisplay(value);
    });
}
```

### 1.3 Rotary Knob Implementation

**Relative Drag Pattern (Industry Standard):**

```javascript
let rotation = 0;
let lastY = 0;

knob.addEventListener('mousedown', (e) => {
    isDragging = true;
    lastY = e.clientY;  // Store CURRENT position
});

document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;

    const deltaY = lastY - e.clientY;  // Distance since LAST FRAME
    rotation += deltaY * 0.5;  // Sensitivity factor
    rotation = Math.max(-135, Math.min(135, rotation));

    setRotation(rotation);
    lastY = e.clientY;  // Update for next frame
});
```

**Visual Rotation:**

```javascript
function setRotation(degrees) {
    knobElement.style.transform = `rotate(${degrees}deg)`;
    // Map rotation to normalized value
    const normalized = (degrees + 135) / 270;  // -135° to +135° → 0 to 1
    sliderState.setNormalisedValue(normalized);
}
```

### 1.4 Dark Botanical Theme CSS

From CONTEXT.md color palette:

```css
:root {
    --bg-charcoal: #1A1A1A;
    --text-primary: #E8E0D4;
    --text-secondary: #A89888;
    --attack-accent: #4A90D9;
    --sustain-accent: #D9944A;
    --transient-heat: linear-gradient(to right, #FF4444, #FF8844);
    --spectrogram-cold: #1A2440;
    --spectrogram-hot: #44FFFF;
}

body {
    background: var(--bg-charcoal);
    font-family: 'Georgia', 'Times New Roman', serif;
    color: var(--text-primary);
}
```

### 1.5 Asset Processing

**Image assets from CONTEXT.md:**
- Paper texture: `/Users/taylorbrook/Dev/Ouaricon Audio Images/paper/paper1.jpg`
- Sea slug illustration: `/Users/taylorbrook/Dev/Ouaricon Audio Images/RAW/insects/slug_olididae118771879trin_0119.jpg`

**Processing requirements:**
1. Darken/invert images BEFORE bundling (not at runtime)
2. Convert to WebP for smaller bundle size
3. Add to CMake binary data with explicit URL mapping

---

## 2. Drawable Curve Editors (Phase 3.2)

### 2.1 Architecture Overview

Two curve editors needed:
- **Attack Curve:** 32 bands, range -1.0 to +1.0, blue accent (#4A90D9)
- **Sustain Curve:** 32 bands, range -1.0 to +1.0, orange accent (#D9944A)

Each editor supports:
- **Freehand Mode:** Mouse drag draws curve, Catmull-Rom smoothing
- **Node Mode:** Click to place control points, drag for bezier precision
- **Mode Toggle:** Explicit button (per CONTEXT.md discussion)

### 2.2 Freehand Mode Implementation

**Catmull-Rom Spline Smoothing (cardinal-spline-js):**

```javascript
import { getCurvePoints } from 'cardinal-spline-js';

class FreehandCurveEditor {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.rawPoints = [];
        this.smoothPoints = [];
    }

    continueDrawing(x, y) {
        this.rawPoints.push(x, y);

        if (this.rawPoints.length >= 8) {
            this.smoothPoints = getCurvePoints(
                this.rawPoints,
                0.5,   // tension (0=loose, 1=tight)
                25,    // points per segment
                false  // not closed
            );
            this.redraw();
        }
    }

    finishDrawing() {
        // Sample at 32 band centers
        const values = this.sampleCurve(32);
        this.notifyCurveChanged(values);
    }

    sampleCurve(numBands) {
        const values = new Float32Array(numBands);
        const width = this.canvas.width;
        const height = this.canvas.height;

        for (let i = 0; i < numBands; i++) {
            // Logarithmic X mapping (20Hz to Nyquist)
            const logMin = Math.log10(20);
            const logMax = Math.log10(22050);
            const logFreq = logMin + (i / (numBands - 1)) * (logMax - logMin);
            const normalizedX = (logFreq - logMin) / (logMax - logMin);
            const x = normalizedX * width;

            const y = this.getYAtX(x);
            // Normalize Y: center=0, top=+1, bottom=-1
            values[i] = 1.0 - (y / height) * 2.0;
        }
        return values;
    }
}
```

### 2.3 Node Mode Implementation

**Draggable Bezier Control Points:**

```javascript
class NodeCurveEditor {
    constructor(canvas) {
        this.nodes = [
            { x: 0, y: canvas.height/2, cp1: null, cp2: {x: 50, y: canvas.height/2} },
            { x: canvas.width, y: canvas.height/2, cp1: {x: canvas.width-50, y: canvas.height/2}, cp2: null }
        ];
    }

    onMouseDown(e) {
        const {x, y} = this.getMousePos(e);

        // Check for node hit (10px radius)
        for (let i = 0; i < this.nodes.length; i++) {
            if (this.distance(x, y, this.nodes[i].x, this.nodes[i].y) < 10) {
                this.selectedNode = i;
                this.dragMode = 'node';
                return;
            }
            // Check control points (8px radius)
            if (this.nodes[i].cp1 && this.distance(x, y, this.nodes[i].cp1.x, this.nodes[i].cp1.y) < 8) {
                this.selectedNode = i;
                this.dragMode = 'cp1';
                return;
            }
        }
    }

    onDoubleClick(e) {
        // Insert new node at click position
        const {x, y} = this.getMousePos(e);
        const newNode = {
            x, y,
            cp1: {x: x-30, y},
            cp2: {x: x+30, y}
        };
        // Insert in sorted order by X
        // ...
    }

    redraw() {
        this.ctx.beginPath();
        this.ctx.moveTo(this.nodes[0].x, this.nodes[0].y);

        for (let i = 0; i < this.nodes.length - 1; i++) {
            const n1 = this.nodes[i];
            const n2 = this.nodes[i+1];
            const cp1 = n1.cp2 || {x: n1.x, y: n1.y};
            const cp2 = n2.cp1 || {x: n2.x, y: n2.y};

            this.ctx.bezierCurveTo(cp1.x, cp1.y, cp2.x, cp2.y, n2.x, n2.y);
        }

        this.ctx.stroke();
    }
}
```

### 2.4 C++ Communication (Curve Updates)

**JavaScript → C++ via Native Function:**

```javascript
// Get native function reference
const setAttackCurve = Juce.getNativeFunction('setAttackCurve');

// Send curve values to C++
async function notifyCurveChanged(curveValues) {
    await setAttackCurve(Array.from(curveValues));
}
```

**C++ Native Function Registration:**

```cpp
// In PluginEditor constructor
webView->addNativeFunction(
    "setAttackCurve",
    [this](const juce::Array<juce::var>& args) -> juce::var {
        if (args.size() == 32) {
            std::array<float, 32> curve;
            for (int i = 0; i < 32; ++i) {
                curve[i] = static_cast<float>(args[i]);
            }
            processorRef.setAttackCurve(curve);
        }
        return juce::var();
    }
);
```

**Thread-Safe Curve Update (Already in Processor):**

```cpp
void OSpectralShaperAudioProcessor::setAttackCurve(const std::array<float, 32>& curve) {
    // Copy to internal array (atomic via STFTProcessor)
    for (int i = 0; i < 32; ++i) {
        attackCurve[i] = curve[i];
    }
    // STFTProcessor reads this array atomically in processBlock
}
```

### 2.5 Grid Overlay (Frequency Labels)

```javascript
function drawGrid(ctx, width, height, numBands) {
    ctx.strokeStyle = 'rgba(168, 152, 136, 0.3)';  // --text-secondary at 30%
    ctx.lineWidth = 1;

    // Vertical lines at key frequencies
    const frequencies = [50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000];
    const logMin = Math.log10(20);
    const logMax = Math.log10(22050);

    frequencies.forEach(freq => {
        const logFreq = Math.log10(freq);
        const x = ((logFreq - logMin) / (logMax - logMin)) * width;

        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, height);
        ctx.stroke();

        // Label
        ctx.fillStyle = 'rgba(168, 152, 136, 0.6)';
        ctx.font = '10px Georgia';
        ctx.fillText(freq >= 1000 ? `${freq/1000}k` : `${freq}`, x + 2, height - 4);
    });

    // Horizontal lines at 0dB (center), ±6dB, ±12dB
    const dbLines = [-12, -6, 0, 6, 12];
    dbLines.forEach(db => {
        const y = height/2 - (db / 12) * (height/2);

        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(width, y);
        ctx.stroke();
    });
}
```

---

## 3. Real-Time Spectrogram (Phase 3.3)

### 3.1 Visualization Data Pipeline

**Lock-Free FIFO Pattern (AbstractFifo):**

```cpp
// PluginProcessor.h additions
struct VisualizationFrame {
    std::array<float, 257> fftMagnitudes;  // NUM_BINS
    std::array<float, 32> transientActivity;
};

juce::AbstractFifo visualizationFifo { 60 };  // 1 second buffer at 60fps
std::vector<VisualizationFrame> visualizationBuffer { 60 };

// In processBlock (audio thread) - write to FIFO
void writeVisualizationFrame(const std::array<float, 257>& mags,
                             const std::array<float, 32>& transients) {
    if (visualizationFifo.getFreeSpace() > 0) {
        int start1, size1, start2, size2;
        visualizationFifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0) {
            std::copy(mags.begin(), mags.end(),
                      visualizationBuffer[start1].fftMagnitudes.begin());
            std::copy(transients.begin(), transients.end(),
                      visualizationBuffer[start1].transientActivity.begin());
        }

        visualizationFifo.finishedWrite(size1);
    }
}
```

**Timer Callback (GUI Thread) - Read from FIFO:**

```cpp
// PluginEditor.h additions
class OSpectralShaperEditor : public juce::AudioProcessorEditor,
                              private juce::Timer {
private:
    void timerCallback() override;
};

// PluginEditor.cpp
void OSpectralShaperEditor::timerCallback() {
    while (processorRef.visualizationFifo.getNumReady() > 0) {
        int start1, size1, start2, size2;
        processorRef.visualizationFifo.prepareToRead(1, start1, size1, start2, size2);

        if (size1 > 0) {
            const auto& frame = processorRef.visualizationBuffer[start1];

            // Convert to JSON and emit to WebView
            sendVisualizationToWebView(frame);
        }

        processorRef.visualizationFifo.finishedRead(size1);
    }
}

void OSpectralShaperEditor::sendVisualizationToWebView(const VisualizationFrame& frame) {
    // Build JSON arrays
    juce::String fftJson = "[";
    for (int i = 0; i < 257; ++i) {
        if (i > 0) fftJson += ",";
        fftJson += juce::String(frame.fftMagnitudes[i], 4);
    }
    fftJson += "]";

    juce::String transientJson = "[";
    for (int i = 0; i < 32; ++i) {
        if (i > 0) transientJson += ",";
        transientJson += juce::String(frame.transientActivity[i], 4);
    }
    transientJson += "]";

    // Emit to JavaScript
    webView->emitEventIfBrowserIsVisible(
        "visualizationUpdate",
        juce::var(new juce::DynamicObject({
            {"fft", fftJson},
            {"transients", transientJson}
        }))
    );
}

// Constructor: Start timer at 60fps
OSpectralShaperEditor::OSpectralShaperEditor(...)
    : AudioProcessorEditor(&p), processorRef(p) {
    // ... existing initialization ...
    startTimerHz(60);
}
```

### 3.2 WebGL Spectrogram Renderer

**Circular Buffer Texture Pattern:**

```javascript
class SpectrogramRenderer {
    constructor(canvas) {
        this.gl = canvas.getContext('webgl2') || canvas.getContext('webgl');
        this.width = 512;   // Time frames
        this.height = 257;  // FFT bins
        this.writeIndex = 0;

        this.initTextures();
        this.initShaders();
    }

    initTextures() {
        const gl = this.gl;

        // FFT magnitude texture (circular buffer)
        this.fftTexture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.fftTexture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.R32F, this.width, this.height, 0,
                      gl.RED, gl.FLOAT, null);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);

        // Transient overlay texture (32 bands interpolated)
        this.transientTexture = gl.createTexture();
        // ...
    }

    addFrame(fftMagnitudes) {
        const gl = this.gl;
        gl.bindTexture(gl.TEXTURE_2D, this.fftTexture);

        // Update single column at writeIndex
        gl.texSubImage2D(gl.TEXTURE_2D, 0,
            this.writeIndex, 0,  // x, y offset
            1, this.height,      // width=1, height=numBins
            gl.RED, gl.FLOAT,
            new Float32Array(fftMagnitudes)
        );

        this.writeIndex = (this.writeIndex + 1) % this.width;
    }
}
```

**Fragment Shader (Colormap + Log Frequency Axis):**

```glsl
precision mediump float;

uniform sampler2D fftTexture;
uniform sampler2D transientTexture;
uniform float writeOffset;  // 0.0-1.0, normalized write position
uniform float minFreq;      // 20.0
uniform float maxFreq;      // 22050.0
uniform float heatIntensity;

varying vec2 vUv;

// Inferno-like colormap (perceptually uniform)
vec3 inferno(float t) {
    const vec3 c0 = vec3(0.0002, 0.0016, 0.0139);
    const vec3 c1 = vec3(0.5770, 0.1476, 0.4289);
    const vec3 c2 = vec3(0.9884, 0.6453, 0.0399);
    const vec3 c3 = vec3(0.9882, 1.0000, 0.6443);

    return clamp(c0 + c1*t + c2*t*t + c3*t*t*t, 0.0, 1.0);
}

// Heat overlay colormap (black → red → orange → yellow → white)
vec3 heatColor(float t) {
    return vec3(
        min(1.0, t * 2.0),
        max(0.0, t * 2.0 - 0.5),
        max(0.0, t * 4.0 - 3.0)
    );
}

// Logarithmic Y to bin index
float logFrequencyLookup(float linearY) {
    float logMin = log(minFreq);
    float logMax = log(maxFreq);
    float freq = exp(logMin + linearY * (logMax - logMin));
    return freq / maxFreq;  // Normalized bin position
}

void main() {
    // Circular buffer X offset
    float x = mod(vUv.x + writeOffset, 1.0);

    // Logarithmic frequency mapping
    float logY = logFrequencyLookup(vUv.y);

    // Sample FFT magnitude
    float magnitude = texture2D(fftTexture, vec2(x, logY)).r;

    // Apply dB scaling (0-60dB range)
    float dbMag = 20.0 * log(magnitude + 0.001) / log(10.0);
    dbMag = (dbMag + 60.0) / 60.0;  // Normalize to 0-1
    dbMag = clamp(dbMag, 0.0, 1.0);

    // Base spectrogram color
    vec3 specColor = inferno(dbMag);

    // Sample transient activity (interpolated from 32 bands)
    float transient = texture2D(transientTexture, vec2(x, vUv.y)).r;
    vec3 heat = heatColor(transient * heatIntensity);

    // Additive blend
    vec3 finalColor = specColor + heat * transient;

    gl_FragColor = vec4(finalColor, 1.0);
}
```

### 3.3 JavaScript Event Listener

```javascript
// Listen for C++ visualization events
window.__JUCE__.backend.addEventListener("visualizationUpdate", (data) => {
    const fftData = JSON.parse(data.fft);
    const transientData = JSON.parse(data.transients);

    // Update spectrogram texture
    spectrogramRenderer.addFrame(fftData);

    // Update transient overlay (interpolate 32 bands to 257 bins)
    spectrogramRenderer.updateTransients(transientData);
});

function render() {
    spectrogramRenderer.draw();
    requestAnimationFrame(render);
}

// Start render loop when WebView ready
render();
```

### 3.4 Performance Considerations

**Target: 60fps with <16ms per frame**

| Operation | Budget | Strategy |
|-----------|--------|----------|
| FIFO read | ~1ms | Single frame per callback |
| JSON parse | ~2ms | Pre-serialized in C++ |
| texSubImage2D | ~2ms | Single column update |
| Shader execution | ~2ms | GPU-accelerated |
| requestAnimationFrame | ~1ms | Browser sync |
| **Total** | ~8ms | 50% headroom |

**Fallback if WebGL issues:**
- Use HTML5 Canvas with ImageData API
- Reduce to 30fps update rate
- Downsample to 64 bands before JS transfer

---

## 4. Pitfalls & Mitigations

### 4.1 From Troubleshooting Knowledge Base

| Pattern | Risk | Prevention |
|---------|------|------------|
| valueChangedEvent no params | Knobs don't update | Read via `getNormalisedValue()` inside callback |
| Relative vs absolute drag | Jumpy knobs | Use `lastY` frame delta, not `startY` |
| ES6 module loading | getSliderState undefined | Use `type="module"` in script tags |
| 3-param attachment | Frozen knobs | Always pass `nullptr` as third param |
| emitEventIfBrowserIsVisible timing | Events lost | Add 100ms delay after pageFinishedLoading |

### 4.2 WebGL-Specific Risks

| Risk | Symptom | Mitigation |
|------|---------|------------|
| No WebGL support | Black canvas | Fallback to Canvas 2D |
| GL_R32F not supported | Texture errors | Use RGBA8 + pack floats |
| Memory leaks | Growing memory | Delete textures on cleanup |
| Context loss | Blank after tab switch | Handle webglcontextlost event |

### 4.3 Thread Safety

| Data | Direction | Mechanism |
|------|-----------|-----------|
| Parameters | UI ↔ Audio | APVTS (JUCE thread-safe) |
| Curve arrays | UI → Audio | Direct assignment (32 floats atomic) |
| FFT data | Audio → UI | AbstractFifo (lock-free) |
| Transient data | Audio → UI | AbstractFifo (lock-free) |

---

## 5. Module Opportunities

### 5.1 Potential Module Extraction

After O-SpectralShaper completion, consider extracting:

1. **webgl-spectrogram** - Reusable WebGL spectrogram renderer
   - Circular buffer texture management
   - Colormap shaders (inferno, viridis, jet)
   - Log frequency axis mapping

2. **drawable-curve-editor** - Freehand + node curve editing
   - Catmull-Rom smoothing
   - Bezier node mode
   - Mode toggle pattern

### 5.2 Existing Modules to Evaluate

| Module | Potential Use | Status |
|--------|---------------|--------|
| vu-meter | Meter animation patterns | Reference only |
| instrument-footer-panel | Layout patterns | Not applicable (effect, not instrument) |

---

## 6. Sources & References

### Primary (HIGH confidence)
- `/Users/taylorbrook/Dev/VST-development/research/webgl-spectrogram-patterns.md` - Comprehensive WebGL patterns
- `/Users/taylorbrook/Dev/VST-development/troubleshooting/patterns/stage-3-patterns.md` - JUCE 8 GUI patterns
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Lyrica/Resources/ui/js/app.js` - Parameter binding reference
- [Spectro making-of](https://github.com/calebj0seph/spectro/blob/master/docs/making-of.md) - WebGL spectrogram
- [JUCE 8 WebView Overview](https://juce.com/blog/juce-8-feature-overview-webview-uis/) - Official docs

### Secondary (MEDIUM confidence)
- [cardinal-spline-js](https://github.com/epistemex/cardinal-spline-js) - Catmull-Rom implementation
- [kbinani/colormap-shaders](https://github.com/kbinani/colormap-shaders) - GLSL colormaps
- [LearnOpenGL Blending](https://learnopengl.com/Advanced-OpenGL/Blending) - WebGL blend modes

---

## 7. Next Steps

**Research Phase Complete.** Ready for `/plugin-plan O-SpectralShaper 3-gui`.

Recommended phase breakdown:
1. **Phase 3.1:** WebView layout (index.html + CSS + knob binding) - Builds on existing infrastructure
2. **Phase 3.2:** Curve editors (Canvas + freehand + node modes + C++ native functions)
3. **Phase 3.3:** Spectrogram (AbstractFifo + WebGL + heat overlay)

---

*Research completed: 2026-02-03*
*Ready for planning phase*
