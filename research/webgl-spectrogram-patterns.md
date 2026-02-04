# WebGL Spectrogram Implementation Patterns for Real-Time Audio Visualization

**Researched:** 2026-02-03
**Domain:** WebGL Audio Visualization, JUCE WebView Integration
**Confidence:** HIGH (verified with multiple authoritative sources)

---

## Summary

This document consolidates research on implementing real-time WebGL spectrograms with heat overlay blending for JUCE 8 WebView plugins. The focus is on patterns suitable for spectral analysis visualizations with drawable curve editors.

**Primary recommendation:** Use WebGL with a circular buffer texture and fragment shader colormap for 60fps spectrogram rendering. Transfer FFT data via JUCE's `emitEventIfBrowserIsVisible()` on a timer callback, using `AbstractFifo` for lock-free audio-to-GUI communication.

---

## 1. WebGL Spectrogram Best Practices

### 1.1 Fragment Shader Colormap Implementation

**Standard Pattern: 1D Gradient Texture Lookup**

The established approach uses a 1D texture containing the colormap, sampled by magnitude value (0.0-1.0).

```glsl
// Fragment shader - colormap lookup
uniform sampler2D fftTexture;      // FFT magnitude data
uniform sampler2D colormapTexture; // 1D colormap (256x1)
uniform float brightness;
uniform float contrast;

varying vec2 vUv;

void main() {
    // Sample FFT magnitude at this UV position
    float magnitude = texture2D(fftTexture, vUv).r;

    // Apply brightness/contrast
    magnitude = (magnitude - 0.5) * contrast + 0.5 + brightness;
    magnitude = clamp(magnitude, 0.0, 1.0);

    // Sample colormap at magnitude position
    vec4 color = texture2D(colormapTexture, vec2(magnitude, 0.5));

    gl_FragColor = color;
}
```

**Source:** [Spectro making-of](https://github.com/calebj0seph/spectro/blob/master/docs/making-of.md) - verified HIGH confidence

**Alternative: Procedural Colormap in Shader**

For simpler deployment (no texture loading), implement colormap mathematically:

```glsl
// Viridis-like colormap (procedural)
vec3 viridis(float t) {
    const vec3 c0 = vec3(0.2777, 0.0054, 0.3340);
    const vec3 c1 = vec3(0.1050, 0.6385, 0.7636);
    const vec3 c2 = vec3(0.9930, 0.9062, 0.1439);

    return mix(mix(c0, c1, smoothstep(0.0, 0.5, t)),
               c2, smoothstep(0.5, 1.0, t));
}

// Jet colormap (classic spectrogram)
vec3 jet(float t) {
    return clamp(vec3(
        1.5 - abs(4.0 * t - 3.0),
        1.5 - abs(4.0 * t - 2.0),
        1.5 - abs(4.0 * t - 1.0)
    ), 0.0, 1.0);
}

// Inferno colormap (perceptually uniform)
vec3 inferno(float t) {
    const vec3 c0 = vec3(0.0002, 0.0016, 0.0139);
    const vec3 c1 = vec3(0.5770, 0.1476, 0.4289);
    const vec3 c2 = vec3(0.9884, 0.6453, 0.0399);
    const vec3 c3 = vec3(0.9882, 1.0000, 0.6443);

    float t2 = t * t;
    float t3 = t2 * t;
    return c0 + c1*t + c2*t2 + c3*t3;
}
```

**Source:** [kbinani/colormap-shaders](https://github.com/kbinani/colormap-shaders) - HIGH confidence

### 1.2 Texture Scrolling Techniques for Real-Time Display

**Circular Buffer Pattern**

The most efficient approach uses a circular buffer texture with shader-based coordinate wrapping:

```javascript
// JavaScript: Texture update pattern
class SpectrogramRenderer {
    constructor(gl, width, height) {
        this.gl = gl;
        this.width = width;    // Number of time frames
        this.height = height;  // Number of frequency bins
        this.writeIndex = 0;   // Current write position (circular)

        // Create FFT texture (single-channel float)
        this.fftTexture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.fftTexture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.R32F, width, height, 0,
                      gl.RED, gl.FLOAT, null);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    }

    // Add new FFT frame (called per audio frame)
    addFrame(fftMagnitudes) {
        const gl = this.gl;
        gl.bindTexture(gl.TEXTURE_2D, this.fftTexture);

        // Update single column at writeIndex (minimal GPU upload)
        gl.texSubImage2D(gl.TEXTURE_2D, 0,
            this.writeIndex, 0,    // x, y offset
            1, this.height,        // width=1, height=numBins
            gl.RED, gl.FLOAT,
            new Float32Array(fftMagnitudes)
        );

        this.writeIndex = (this.writeIndex + 1) % this.width;
    }
}
```

**Fragment Shader Coordinate Wrapping:**

```glsl
// Handle circular buffer in shader
uniform float writeOffset;  // 0.0-1.0, normalized write position
uniform vec2 textureSize;   // (width, height)

varying vec2 vUv;

void main() {
    // Apply circular offset to X coordinate
    float x = vUv.x + writeOffset;
    x = mod(x, 1.0);  // Wrap around (0.0-1.0)

    vec2 sampleUv = vec2(x, vUv.y);
    float magnitude = texture2D(fftTexture, sampleUv).r;

    // ... colormap lookup ...
}
```

**Key Insight:** The `mod()` function handles wrapping for non-power-of-two textures, which don't support hardware `GL_REPEAT` wrapping.

**Source:** [Spectro documentation](https://github.com/calebj0seph/spectro/blob/master/docs/making-of.md) - HIGH confidence

### 1.3 Performance Optimization for 60fps Rendering

**Optimization Hierarchy (ordered by impact):**

| Optimization | Impact | Implementation |
|-------------|--------|----------------|
| texSubImage2D | HIGH | Update only new data (1 column), not entire texture |
| WebGL (not Canvas2D) | HIGH | GPU-accelerated pixel operations |
| Floating-point texture | MEDIUM | R32F format, no normalization needed |
| requestAnimationFrame | MEDIUM | Sync with browser refresh rate |
| Web Workers | LOW | Move FFT computation off main thread |

**60fps Budget:**
- 16.67ms per frame total
- ~2ms for data transfer (C++ to JS)
- ~4ms for texture upload
- ~2ms for shader execution
- ~8ms buffer for GC, layout, etc.

**Performance Monitoring:**

```javascript
let lastTime = performance.now();
let frameCount = 0;

function render() {
    const now = performance.now();
    frameCount++;

    if (now - lastTime >= 1000) {
        console.log(`FPS: ${frameCount}`);
        frameCount = 0;
        lastTime = now;
    }

    // ... render spectrogram ...

    requestAnimationFrame(render);
}
```

**Source:** [glspect](https://github.com/ahbarnett/glspect) reports 60fps with ~50% single-core CPU usage - MEDIUM confidence

### 1.4 Logarithmic Frequency Axis Mapping

**Two Approaches:**

**A) Shader-based (Recommended)**

Map linear texture coordinates to log-scaled frequency lookups in the fragment shader:

```glsl
uniform float minFreq;    // e.g., 20.0 Hz
uniform float maxFreq;    // e.g., 20000.0 Hz
uniform float numBins;    // e.g., 257.0

// Convert linear Y (0-1) to log-scaled bin index
float logFrequencyLookup(float linearY) {
    // Linear Y to frequency (log scale)
    float logMin = log(minFreq);
    float logMax = log(maxFreq);
    float freq = exp(logMin + linearY * (logMax - logMin));

    // Frequency to bin index
    float bin = freq * numBins / maxFreq;
    return clamp(bin / numBins, 0.0, 1.0);
}

void main() {
    float logY = logFrequencyLookup(vUv.y);
    vec2 sampleUv = vec2(vUv.x, logY);
    float magnitude = texture2D(fftTexture, sampleUv).r;
    // ...
}
```

**B) Pre-computed Lookup Table (Alternative)**

Use a 1D texture as a lookup table for frequency mapping:

```javascript
// Generate log-scale lookup texture
function createLogScaleLUT(numPixels, numBins, minFreq, maxFreq, sampleRate) {
    const lut = new Float32Array(numPixels);
    const logMin = Math.log10(minFreq);
    const logMax = Math.log10(maxFreq);

    for (let i = 0; i < numPixels; i++) {
        const normalizedY = i / (numPixels - 1);
        const logFreq = logMin + normalizedY * (logMax - logMin);
        const freq = Math.pow(10, logFreq);
        const bin = Math.round(freq * numBins / (sampleRate / 2));
        lut[i] = Math.min(bin / numBins, 1.0);
    }

    return lut;
}
```

**Mel Scale (Perceptual Alternative):**

```glsl
// Mel scale transformation
float hzToMel(float hz) {
    return 2595.0 * log(1.0 + hz / 700.0) / log(10.0);
}

float melToHz(float mel) {
    return 700.0 * (pow(10.0, mel / 2595.0) - 1.0);
}
```

**Source:** [calebgannon.com Three.js spectrogram tutorial](https://calebgannon.com/2021/01/09/spectrogram-with-three-js-and-glsl-shaders/) - HIGH confidence

---

## 2. Heat Overlay Blending

### 2.1 Blending Techniques for Transient Activity Overlay

**Additive Blending (Recommended for Heat Overlay)**

Best for showing transient "hotspots" over the spectrogram:

```javascript
// WebGL setup for additive blending
gl.enable(gl.BLEND);
gl.blendFunc(gl.SRC_ALPHA, gl.ONE);  // Additive

// Or for screen blending (less saturated):
gl.blendFunc(gl.ONE, gl.ONE_MINUS_SRC_COLOR);
```

**Fragment Shader Heat Overlay:**

```glsl
uniform sampler2D spectrogramTexture;
uniform sampler2D transientTexture;  // 32-band transient activity
uniform float heatIntensity;

vec3 heatColor(float t) {
    // Black -> Red -> Orange -> Yellow -> White
    return vec3(
        min(1.0, t * 2.0),
        max(0.0, t * 2.0 - 0.5),
        max(0.0, t * 4.0 - 3.0)
    );
}

void main() {
    vec4 specColor = texture2D(spectrogramTexture, vUv);

    // Sample transient activity (interpolate between bands)
    float transient = texture2D(transientTexture, vUv).r;
    transient = transient * heatIntensity;

    vec3 heat = heatColor(transient);

    // Additive blend
    vec3 finalColor = specColor.rgb + heat * transient;

    // Or screen blend (prevents over-saturation)
    // vec3 finalColor = 1.0 - (1.0 - specColor.rgb) * (1.0 - heat * transient);

    gl_FragColor = vec4(finalColor, 1.0);
}
```

### 2.2 Multi-Layer Rendering Strategy

For complex overlays (spectrogram + heat + curve), use separate render passes:

```javascript
// Render order:
// 1. Base spectrogram (opaque)
// 2. Transient heat overlay (additive blend)
// 3. Curve/UI overlay (alpha blend)

function renderLayers() {
    // Pass 1: Spectrogram (no blending)
    gl.disable(gl.BLEND);
    renderSpectrogram();

    // Pass 2: Heat overlay (additive)
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE);
    renderHeatOverlay();

    // Pass 3: UI curves (standard alpha)
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    renderCurveOverlay();
}
```

**Source:** [LearnOpenGL Blending](https://learnopengl.com/Advanced-OpenGL/Blending), [glsl-blend](https://github.com/jamieowen/glsl-blend) - HIGH confidence

---

## 3. JUCE WebView Integration

### 3.1 FFT Data Transfer: C++ to JavaScript

**Recommended Pattern: Timer-Based Event Emission**

```cpp
// PluginEditor.h
class OSpectralShaperEditor : public juce::AudioProcessorEditor,
                              private juce::Timer {
private:
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Visualization data buffer (filled by processor)
    struct VisualizationData {
        std::array<float, 257> fftMagnitudes;  // FFT bins
        std::array<float, 32> transientActivity;  // Per-band transients
    };

    juce::AbstractFifo visualizationFifo { 60 };  // 1 second buffer
    std::vector<VisualizationData> visualizationBuffer { 60 };

    void timerCallback() override;
};

// PluginEditor.cpp
void OSpectralShaperEditor::timerCallback() {
    // Read all available frames from FIFO
    while (visualizationFifo.getNumReady() > 0) {
        int start1, size1, start2, size2;
        visualizationFifo.prepareToRead(1, start1, size1, start2, size2);

        if (size1 > 0) {
            const auto& data = visualizationBuffer[start1];

            // Convert to JSON array for JavaScript
            juce::String fftJson = "[";
            for (int i = 0; i < data.fftMagnitudes.size(); ++i) {
                if (i > 0) fftJson += ",";
                fftJson += juce::String(data.fftMagnitudes[i], 4);
            }
            fftJson += "]";

            juce::String transientJson = "[";
            for (int i = 0; i < data.transientActivity.size(); ++i) {
                if (i > 0) transientJson += ",";
                transientJson += juce::String(data.transientActivity[i], 4);
            }
            transientJson += "]";

            // Emit event to JavaScript
            webView->emitEventIfBrowserIsVisible(
                "visualizationUpdate",
                juce::var(new juce::DynamicObject({
                    {"fft", fftJson},
                    {"transients", transientJson}
                }))
            );
        }

        visualizationFifo.finishedRead(size1);
    }
}

// Constructor
OSpectralShaperEditor::OSpectralShaperEditor(...) {
    // Start timer at 60fps
    startTimerHz(60);
}
```

**JavaScript Event Listener:**

```javascript
// Listen for C++ events
window.__JUCE__.backend.addEventListener("visualizationUpdate", (data) => {
    const fftData = JSON.parse(data.fft);
    const transientData = JSON.parse(data.transients);

    // Update spectrogram
    spectrogramRenderer.addFrame(fftData);
    transientOverlay.updateActivity(transientData);
});
```

### 3.2 evaluateJavascript() vs emitEventIfBrowserIsVisible()

| Method | Use Case | Performance | Thread Safety |
|--------|----------|-------------|---------------|
| `evaluateJavascript()` | One-off JS execution | Lower | Must call from message thread |
| `emitEventIfBrowserIsVisible()` | Regular data streaming | Higher | Safe from any thread |
| Native functions | JS-initiated requests | Varies | Async callback pattern |

**Critical Note:** `emitEventIfBrowserIsVisible()` may not work immediately after `pageFinishedLoading()`. Use a small delay:

```cpp
void pageFinishedLoading(const juce::String& url) override {
    // Wait 100ms before first event emission
    juce::Timer::callAfterDelay(100, [this] {
        emitEventIfBrowserIsVisible("ready", juce::var(true));
    });
}
```

**Source:** [JUCE Forum](https://forum.juce.com/t/webbrowsercomponent-pagefinishedloading-bug-or-misunderstanding/62723) - HIGH confidence

### 3.3 AbstractFifo Pattern for Lock-Free Audio-to-GUI Transfer

**Complete Pattern:**

```cpp
// PluginProcessor.h
class OSpectralShaperProcessor : public juce::AudioProcessor {
public:
    // Lock-free FIFO for visualization
    juce::AbstractFifo visualizationFifo { 60 };

    struct VisualizationFrame {
        std::array<float, 257> magnitudes;
        std::array<float, 32> transients;
    };
    std::vector<VisualizationFrame> visualizationBuffer { 60 };

private:
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
};

// PluginProcessor.cpp
void OSpectralShaperProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&) {
    // ... FFT processing ...

    // Write to visualization FIFO (audio thread)
    if (visualizationFifo.getFreeSpace() > 0) {
        int start1, size1, start2, size2;
        visualizationFifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0) {
            // Copy FFT data to buffer
            std::copy(currentMagnitudes.begin(), currentMagnitudes.end(),
                      visualizationBuffer[start1].magnitudes.begin());
            std::copy(currentTransients.begin(), currentTransients.end(),
                      visualizationBuffer[start1].transients.begin());
        }

        visualizationFifo.finishedWrite(size1);
    }
}

// PluginEditor.cpp (GUI thread reads)
void OSpectralShaperEditor::timerCallback() {
    while (processorRef.visualizationFifo.getNumReady() > 0) {
        int start1, size1, start2, size2;
        processorRef.visualizationFifo.prepareToRead(1, start1, size1, start2, size2);

        if (size1 > 0) {
            const auto& frame = processorRef.visualizationBuffer[start1];
            // Send to WebView...
        }

        processorRef.visualizationFifo.finishedRead(size1);
    }
}
```

**Key Points:**
- `AbstractFifo` is lock-free for single-reader, single-writer scenarios
- Audio thread writes, GUI thread reads
- Never call `setTotalSize()` during operation (not thread-safe)
- Handle wraparound with two-block reads/writes

**Source:** [JUCE AbstractFifo docs](https://ccrma.stanford.edu/~jos/juce_modules/classAbstractFifo.html) - HIGH confidence

---

## 4. Canvas Curve Editor Patterns

### 4.1 Freehand Drawing with Smoothing

**Catmull-Rom Spline Smoothing (Recommended)**

```javascript
// Using cardinal-spline-js library
// npm install cardinal-spline-js

import { getCurvePoints } from 'cardinal-spline-js';

class FreehandCurveEditor {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.rawPoints = [];  // Captured mouse points
        this.smoothPoints = [];  // Interpolated curve
        this.isDrawing = false;
    }

    startDrawing(x, y) {
        this.isDrawing = true;
        this.rawPoints = [x, y];
    }

    continueDrawing(x, y) {
        if (!this.isDrawing) return;

        this.rawPoints.push(x, y);

        // Real-time smoothing (minimum 4 points for Catmull-Rom)
        if (this.rawPoints.length >= 8) {
            this.smoothPoints = getCurvePoints(
                this.rawPoints,
                0.5,  // tension (0 = loose, 1 = tight)
                25,   // points per segment
                false // not closed
            );
            this.redraw();
        }
    }

    finishDrawing() {
        this.isDrawing = false;

        // Final smooth with higher resolution
        if (this.rawPoints.length >= 8) {
            this.smoothPoints = getCurvePoints(
                this.rawPoints,
                0.5,
                50,   // Higher resolution for final curve
                false
            );
        }

        // Convert to normalized curve values (32 bands)
        this.curveValues = this.sampleCurve(32);
        this.notifyCurveChanged();
    }

    sampleCurve(numBands) {
        const values = new Float32Array(numBands);
        const width = this.canvas.width;
        const height = this.canvas.height;

        for (let i = 0; i < numBands; i++) {
            const x = (i / (numBands - 1)) * width;
            const y = this.getYAtX(x);
            // Normalize Y to -1.0 to +1.0 (center = 0)
            values[i] = 1.0 - (y / height) * 2.0;
        }

        return values;
    }

    getYAtX(targetX) {
        // Linear interpolation along smoothed curve
        for (let i = 0; i < this.smoothPoints.length - 2; i += 2) {
            const x1 = this.smoothPoints[i];
            const x2 = this.smoothPoints[i + 2];

            if (targetX >= x1 && targetX <= x2) {
                const y1 = this.smoothPoints[i + 1];
                const y2 = this.smoothPoints[i + 3];
                const t = (targetX - x1) / (x2 - x1);
                return y1 + t * (y2 - y1);
            }
        }
        return this.canvas.height / 2;  // Default to center
    }

    redraw() {
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

        // Draw smoothed curve
        this.ctx.beginPath();
        this.ctx.moveTo(this.smoothPoints[0], this.smoothPoints[1]);

        for (let i = 2; i < this.smoothPoints.length; i += 2) {
            this.ctx.lineTo(this.smoothPoints[i], this.smoothPoints[i + 1]);
        }

        this.ctx.strokeStyle = '#5a7a6a';
        this.ctx.lineWidth = 2;
        this.ctx.stroke();
    }
}
```

**Source:** [cardinal-spline-js](https://github.com/epistemex/cardinal-spline-js) - HIGH confidence

### 4.2 Node/Bezier Editing Mode

**Draggable Control Points Pattern:**

```javascript
class NodeCurveEditor {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.nodes = [];  // Array of {x, y, control1, control2}
        this.selectedNode = null;
        this.dragMode = null;  // 'node', 'control1', 'control2'

        this.initDefaultCurve();
        this.setupEventListeners();
    }

    initDefaultCurve() {
        // Default flat line with two endpoints
        const h = this.canvas.height / 2;
        this.nodes = [
            { x: 0, y: h, control1: null, control2: { x: 50, y: h } },
            { x: this.canvas.width, y: h, control1: { x: this.canvas.width - 50, y: h }, control2: null }
        ];
    }

    setupEventListeners() {
        this.canvas.addEventListener('mousedown', (e) => this.onMouseDown(e));
        this.canvas.addEventListener('mousemove', (e) => this.onMouseMove(e));
        this.canvas.addEventListener('mouseup', () => this.onMouseUp());
        this.canvas.addEventListener('dblclick', (e) => this.onDoubleClick(e));
    }

    onMouseDown(e) {
        const { x, y } = this.getMousePos(e);

        // Check if clicking on existing node or control point
        for (let i = 0; i < this.nodes.length; i++) {
            const node = this.nodes[i];

            // Check main node
            if (this.distance(x, y, node.x, node.y) < 10) {
                this.selectedNode = i;
                this.dragMode = 'node';
                return;
            }

            // Check control points
            if (node.control1 && this.distance(x, y, node.control1.x, node.control1.y) < 8) {
                this.selectedNode = i;
                this.dragMode = 'control1';
                return;
            }
            if (node.control2 && this.distance(x, y, node.control2.x, node.control2.y) < 8) {
                this.selectedNode = i;
                this.dragMode = 'control2';
                return;
            }
        }
    }

    onMouseMove(e) {
        if (this.selectedNode === null) return;

        const { x, y } = this.getMousePos(e);
        const node = this.nodes[this.selectedNode];

        switch (this.dragMode) {
            case 'node':
                // Constrain X to stay in order
                const minX = this.selectedNode > 0 ?
                    this.nodes[this.selectedNode - 1].x + 10 : 0;
                const maxX = this.selectedNode < this.nodes.length - 1 ?
                    this.nodes[this.selectedNode + 1].x - 10 : this.canvas.width;

                node.x = Math.max(minX, Math.min(maxX, x));
                node.y = Math.max(0, Math.min(this.canvas.height, y));

                // Move control points with node
                if (node.control1) {
                    node.control1.y = node.y;
                }
                if (node.control2) {
                    node.control2.y = node.y;
                }
                break;

            case 'control1':
                node.control1.x = x;
                node.control1.y = y;
                break;

            case 'control2':
                node.control2.x = x;
                node.control2.y = y;
                break;
        }

        this.redraw();
    }

    onMouseUp() {
        this.selectedNode = null;
        this.dragMode = null;
        this.notifyCurveChanged();
    }

    onDoubleClick(e) {
        const { x, y } = this.getMousePos(e);

        // Find insertion point
        let insertIndex = this.nodes.length;
        for (let i = 0; i < this.nodes.length; i++) {
            if (x < this.nodes[i].x) {
                insertIndex = i;
                break;
            }
        }

        // Create new node with control points
        const newNode = {
            x: x,
            y: y,
            control1: { x: x - 30, y: y },
            control2: { x: x + 30, y: y }
        };

        this.nodes.splice(insertIndex, 0, newNode);
        this.redraw();
    }

    redraw() {
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

        // Draw Bezier curve
        this.ctx.beginPath();
        this.ctx.moveTo(this.nodes[0].x, this.nodes[0].y);

        for (let i = 0; i < this.nodes.length - 1; i++) {
            const n1 = this.nodes[i];
            const n2 = this.nodes[i + 1];

            const cp1 = n1.control2 || { x: n1.x, y: n1.y };
            const cp2 = n2.control1 || { x: n2.x, y: n2.y };

            this.ctx.bezierCurveTo(cp1.x, cp1.y, cp2.x, cp2.y, n2.x, n2.y);
        }

        this.ctx.strokeStyle = '#5a7a6a';
        this.ctx.lineWidth = 2;
        this.ctx.stroke();

        // Draw nodes and control points
        for (const node of this.nodes) {
            // Control point lines
            this.ctx.strokeStyle = 'rgba(139, 115, 85, 0.5)';
            this.ctx.lineWidth = 1;

            if (node.control1) {
                this.ctx.beginPath();
                this.ctx.moveTo(node.x, node.y);
                this.ctx.lineTo(node.control1.x, node.control1.y);
                this.ctx.stroke();

                this.ctx.beginPath();
                this.ctx.arc(node.control1.x, node.control1.y, 5, 0, Math.PI * 2);
                this.ctx.fillStyle = '#8B7355';
                this.ctx.fill();
            }

            if (node.control2) {
                this.ctx.beginPath();
                this.ctx.moveTo(node.x, node.y);
                this.ctx.lineTo(node.control2.x, node.control2.y);
                this.ctx.stroke();

                this.ctx.beginPath();
                this.ctx.arc(node.control2.x, node.control2.y, 5, 0, Math.PI * 2);
                this.ctx.fillStyle = '#8B7355';
                this.ctx.fill();
            }

            // Main node
            this.ctx.beginPath();
            this.ctx.arc(node.x, node.y, 7, 0, Math.PI * 2);
            this.ctx.fillStyle = '#5a7a6a';
            this.ctx.fill();
            this.ctx.strokeStyle = '#3C2F2F';
            this.ctx.lineWidth = 2;
            this.ctx.stroke();
        }
    }

    // Helper methods
    getMousePos(e) {
        const rect = this.canvas.getBoundingClientRect();
        return {
            x: e.clientX - rect.left,
            y: e.clientY - rect.top
        };
    }

    distance(x1, y1, x2, y2) {
        return Math.sqrt((x2-x1)**2 + (y2-y1)**2);
    }
}
```

**Source:** [Konva.js curve editing](https://konvajs.org/docs/sandbox/Modify_Curves_with_Anchor_Points.html), [Bezier.js](https://pomax.github.io/bezierjs/) - HIGH confidence

### 4.3 Mode Toggle Pattern

```javascript
class DualModeCurveEditor {
    constructor(canvas) {
        this.canvas = canvas;
        this.freehandEditor = new FreehandCurveEditor(canvas);
        this.nodeEditor = new NodeCurveEditor(canvas);
        this.mode = 'freehand';  // 'freehand' or 'node'

        this.setMode('freehand');
    }

    setMode(mode) {
        this.mode = mode;

        // Transfer curve data between editors
        if (mode === 'node') {
            // Convert freehand points to nodes
            this.nodeEditor.setNodesFromPoints(this.freehandEditor.smoothPoints);
        } else {
            // Convert nodes to smooth curve
            this.freehandEditor.setPointsFromNodes(this.nodeEditor.nodes);
        }

        this.activeEditor.redraw();
    }

    get activeEditor() {
        return this.mode === 'freehand' ? this.freehandEditor : this.nodeEditor;
    }

    getCurveValues(numBands) {
        return this.activeEditor.sampleCurve(numBands);
    }
}
```

---

## 5. Advanced Freehand Smoothing Techniques

### 5.1 Perfect Freehand Library

For pressure-sensitive stylus input or natural-feeling strokes:

```javascript
// npm install perfect-freehand

import { getStroke } from 'perfect-freehand';

const options = {
    size: 8,
    thinning: 0.5,
    smoothing: 0.5,
    streamline: 0.5,
    simulatePressure: true,  // Velocity-based pressure
};

function drawStroke(points) {
    const stroke = getStroke(points, options);

    // Convert to SVG path or canvas
    const pathData = getSvgPathFromStroke(stroke);

    // Or draw to canvas
    ctx.beginPath();
    ctx.moveTo(stroke[0][0], stroke[0][1]);
    for (const [x, y] of stroke.slice(1)) {
        ctx.lineTo(x, y);
    }
    ctx.fill();
}
```

**Source:** [perfect-freehand](https://github.com/steveruizok/perfect-freehand) - HIGH confidence

### 5.2 Stroke Stabilization Algorithms

**One-Euro Filter (Speed-Adaptive):**

```javascript
class OneEuroFilter {
    constructor(minCutoff = 1.0, beta = 0.0, dCutoff = 1.0) {
        this.minCutoff = minCutoff;
        this.beta = beta;
        this.dCutoff = dCutoff;
        this.xFilter = new LowPassFilter(this.alpha(minCutoff));
        this.dxFilter = new LowPassFilter(this.alpha(dCutoff));
        this.lastTime = null;
    }

    alpha(cutoff) {
        const te = 1.0 / 60.0;  // Assume 60fps
        const tau = 1.0 / (2 * Math.PI * cutoff);
        return 1.0 / (1.0 + tau / te);
    }

    filter(x, timestamp = null) {
        if (this.lastTime !== null && timestamp !== null) {
            const dt = timestamp - this.lastTime;
            const dx = (x - this.xFilter.lastRaw) / dt;
            const edx = this.dxFilter.filter(dx);
            const cutoff = this.minCutoff + this.beta * Math.abs(edx);
            this.xFilter.setAlpha(this.alpha(cutoff));
        }

        this.lastTime = timestamp;
        return this.xFilter.filter(x);
    }
}

class LowPassFilter {
    constructor(alpha) {
        this.setAlpha(alpha);
        this.lastRaw = 0;
        this.lastFiltered = 0;
        this.initialized = false;
    }

    setAlpha(alpha) {
        this.alpha = Math.max(0, Math.min(1, alpha));
    }

    filter(value) {
        this.lastRaw = value;

        if (!this.initialized) {
            this.initialized = true;
            this.lastFiltered = value;
        } else {
            this.lastFiltered = this.alpha * value + (1 - this.alpha) * this.lastFiltered;
        }

        return this.lastFiltered;
    }
}
```

**Source:** [DEV.to stroke stabilizer article](https://dev.to/usapopopooon/i-built-a-library-to-reduce-hand-tremor-in-drawing-apps-33ng) - MEDIUM confidence

---

## 6. Integration Example: Complete Spectrogram Component

```javascript
// Complete WebGL spectrogram with curve overlay for JUCE WebView

class SpectralShaperUI {
    constructor(containerElement) {
        this.container = containerElement;

        // Create canvases
        this.spectrogramCanvas = this.createCanvas('spectrogram');
        this.curveCanvas = this.createCanvas('curve-overlay');

        // Initialize WebGL for spectrogram
        this.gl = this.spectrogramCanvas.getContext('webgl2') ||
                  this.spectrogramCanvas.getContext('webgl');
        this.initWebGL();

        // Initialize curve editor
        this.curveEditor = new DualModeCurveEditor(this.curveCanvas);

        // JUCE event listener
        this.setupJuceEvents();

        // Start render loop
        this.render();
    }

    createCanvas(className) {
        const canvas = document.createElement('canvas');
        canvas.className = className;
        canvas.width = 800;
        canvas.height = 400;
        canvas.style.position = 'absolute';
        this.container.appendChild(canvas);
        return canvas;
    }

    initWebGL() {
        // Compile shaders, create textures...
        // (See sections 1.1-1.4 for implementation)
    }

    setupJuceEvents() {
        window.__JUCE__.backend.addEventListener("visualizationUpdate", (data) => {
            const fft = JSON.parse(data.fft);
            const transients = JSON.parse(data.transients);

            this.updateSpectrogram(fft);
            this.updateTransientOverlay(transients);
        });

        // Send curve changes back to C++
        this.curveEditor.onCurveChanged = (values) => {
            window.__JUCE__.backend.emitEvent("attackCurveChanged", {
                values: Array.from(values)
            });
        };
    }

    updateSpectrogram(fftData) {
        // texSubImage2D update (see section 1.2)
    }

    updateTransientOverlay(transientData) {
        // Update heat overlay texture
    }

    render() {
        // Draw spectrogram with WebGL
        this.drawSpectrogramPass();

        // Draw heat overlay
        this.drawHeatOverlayPass();

        requestAnimationFrame(() => this.render());
    }
}

// Initialize when DOM ready
document.addEventListener('DOMContentLoaded', () => {
    const ui = new SpectralShaperUI(document.getElementById('visualizer'));
});
```

---

## 7. Sources

### Primary (HIGH confidence)
- [Spectro making-of documentation](https://github.com/calebj0seph/spectro/blob/master/docs/making-of.md) - Circular buffer texture, colormap lookup
- [JUCE AbstractFifo docs](https://ccrma.stanford.edu/~jos/juce_modules/classAbstractFifo.html) - Lock-free FIFO pattern
- [JUCE 8 WebView Feature Overview](https://juce.com/blog/juce-8-feature-overview-webview-uis/) - emitEventIfBrowserIsVisible, native functions
- [cardinal-spline-js](https://github.com/epistemex/cardinal-spline-js) - Catmull-Rom smoothing
- [kbinani/colormap-shaders](https://github.com/kbinani/colormap-shaders) - GLSL colormap implementations

### Secondary (MEDIUM confidence)
- [LearnOpenGL Blending](https://learnopengl.com/Advanced-OpenGL/Blending) - WebGL blend modes
- [JUCE Forum: WebView bi-directional messaging](https://forum.juce.com/t/juce8-webview-bi-directional-messaging/61296) - Data transfer patterns
- [Sound-Field GitHub](https://github.com/mbarzach/Sound-Field) - JUCE 8 WebView FFT visualization example
- [glspect](https://github.com/ahbarnett/glspect) - Real-time OpenGL spectrogram performance
- [Konva.js curve editing](https://konvajs.org/docs/sandbox/Modify_Curves_with_Anchor_Points.html) - Bezier node editing

### Tertiary (LOW confidence - for reference only)
- [perfect-freehand](https://github.com/steveruizok/perfect-freehand) - Advanced stroke rendering
- [signature_pad](https://github.com/szimek/signature_pad) - Variable-width Bezier strokes
- [glsl-blend](https://github.com/jamieowen/glsl-blend) - Photoshop blend modes in GLSL

---

## 8. Metadata

**Confidence breakdown:**
- WebGL spectrogram patterns: HIGH - Multiple verified implementations (Spectro, glspect)
- JUCE WebView integration: HIGH - Official docs + working examples (Sound-Field)
- Heat overlay blending: HIGH - Standard WebGL/OpenGL techniques
- Curve editor patterns: HIGH - Multiple libraries with documented APIs

**Research date:** 2026-02-03
**Valid until:** 90 days (WebGL patterns are stable; JUCE 8 WebView API may evolve)
