---
title: "Playhead Modulator Architecture"
created: 2026-01-15
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Design for a TimeShaper-style playhead modulator with drawable envelope control, multiband time manipulation, harmonic locking during speed changes, and physics-based envelope behavior."
domain: dsp
type: algorithm
keywords:
  - stutter
  - stutter-effects
  - playhead-modulation
  - time-stretching
  - harmonic-locking
  - multiband
  - drawable-envelope
stages: [0, 2]
agents: [dsp, research]
---

# Path C: Playhead Modulator (TimeShaper Style)

**Drawable Envelope Controls Playback Position**

**Estimated Development Time:** 4-6 weeks
**Complexity:** High
**Starting Point:** New plugin from scratch

---

## Unique Value Proposition

**"Spectral Time Shaper"** - Differentiated by:
- Multiband time manipulation (lows normal, highs glitch)
- Harmonic locking during speed changes (pitch snaps to intervals)
- Physics-based envelope behavior (points have mass, bounce)
- Motion recording (perform scratches in real-time, save as preset)
- Waveform-guided curve editing

No plugin offers harmonic-aware time manipulation. TimeShaper ($29) is closest but treats pitch as byproduct. This makes pitch musical during speed changes.

---

## Architecture Overview

```
                              ┌─────────────────┐
                              │ Drawable Envelope│
                              │   (0-1 range)    │
                              └────────┬────────┘
                                       │
                                       ▼
                              ┌─────────────────┐
                              │ Envelope → Time │
                              │    Mapper       │
                              └────────┬────────┘
                                       │
                    ┌──────────────────┼──────────────────┐
                    ▼                  ▼                  ▼
            ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
            │  Low Band   │    │  Mid Band   │    │  High Band  │
            │  Playhead   │    │  Playhead   │    │  Playhead   │
            └──────┬──────┘    └──────┬──────┘    └──────┬──────┘
                   │                  │                  │
                   ▼                  ▼                  ▼
            ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
Input ─────▶│  Low Buffer │    │  Mid Buffer │    │ High Buffer │
            │  Reader     │    │  Reader     │    │  Reader     │
            └──────┬──────┘    └──────┬──────┘    └──────┬──────┘
                   │                  │                  │
                   └──────────────────┼──────────────────┘
                                      ▼
                              ┌─────────────────┐
                              │  Band Mixer +   │
                              │ Harmonic Lock   │───▶ Output
                              └─────────────────┘
```

---

## Implementation Phases

### Phase 1: Core Time Engine (1 week)

#### 1.1 Rolling Buffer with Playhead

```cpp
// RollingBuffer.h
class RollingBuffer
{
public:
    void prepare(double sampleRate, int numChannels, float maxOffsetSeconds = 4.0f)
    {
        this->sampleRate = sampleRate;
        int bufferSize = static_cast<int>(sampleRate * maxOffsetSeconds) + 4096;

        buffer.setSize(numChannels, bufferSize);
        buffer.clear();

        writePosition = 0;
        this->bufferSize = bufferSize;
        this->numChannels = numChannels;
    }

    // Write incoming audio (continuous)
    void write(const juce::AudioBuffer<float>& input, int numSamples)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* src = input.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                buffer.setSample(ch, (writePosition + i) % bufferSize, src[i]);
            }
        }

        writePosition = (writePosition + numSamples) % bufferSize;
    }

    // Read with fractional offset and interpolation
    float readWithOffset(int channel, float offsetSamples) const
    {
        // Offset is how many samples behind write position
        float readPosFloat = writePosition - offsetSamples;
        while (readPosFloat < 0) readPosFloat += bufferSize;

        // Lagrange interpolation for quality
        return lagrangeInterpolate(channel, readPosFloat);
    }

    // Read entire block with variable offset per sample
    void readBlock(juce::AudioBuffer<float>& output,
                   const std::vector<float>& offsetsPerSample)
    {
        jassert(offsetsPerSample.size() >= output.getNumSamples());

        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            float* dest = output.getWritePointer(ch);
            for (int i = 0; i < output.getNumSamples(); ++i)
            {
                dest[i] = readWithOffset(ch, offsetsPerSample[i]);
            }
        }
    }

    float getMaxOffset() const { return bufferSize - 4096; }  // Leave safety margin
    int getWritePosition() const { return writePosition; }

private:
    float lagrangeInterpolate(int channel, float position) const
    {
        int pos0 = static_cast<int>(position);
        float frac = position - pos0;

        // 4-point Lagrange interpolation
        float y0 = buffer.getSample(channel, (pos0 - 1 + bufferSize) % bufferSize);
        float y1 = buffer.getSample(channel, pos0 % bufferSize);
        float y2 = buffer.getSample(channel, (pos0 + 1) % bufferSize);
        float y3 = buffer.getSample(channel, (pos0 + 2) % bufferSize);

        float c0 = y1;
        float c1 = y2 - y0 * (1.0f/3.0f) - y1 * 0.5f - y3 * (1.0f/6.0f);
        float c2 = (y0 + y2) * 0.5f - y1;
        float c3 = (y3 - y0) * (1.0f/6.0f) + (y1 - y2) * 0.5f;

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    juce::AudioBuffer<float> buffer;
    int writePosition = 0;
    int bufferSize = 0;
    int numChannels = 2;
    double sampleRate = 44100.0;
};
```

#### 1.2 Envelope to Time Mapper

```cpp
// TimeMapper.h
class TimeMapper
{
public:
    struct Settings
    {
        float loopLengthBeats = 4.0f;    // Length of one envelope cycle
        float maxOffsetBeats = 4.0f;     // Maximum time displacement
        bool sync = true;                 // Sync to host tempo
    };

    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
    }

    void updateTempo(double bpm, double ppqPosition)
    {
        currentBpm = bpm;
        samplesPerBeat = (60.0 / bpm) * sampleRate;

        // Calculate position within loop (0.0 - 1.0)
        double beatsInLoop = std::fmod(ppqPosition, settings.loopLengthBeats);
        loopPosition = beatsInLoop / settings.loopLengthBeats;
    }

    // Convert envelope value (0-1) to offset in samples
    float envelopeToOffset(float envelopeValue) const
    {
        // Envelope 1.0 = no offset (realtime)
        // Envelope 0.0 = maximum offset (oldest audio)
        float normalizedOffset = 1.0f - envelopeValue;
        float offsetBeats = normalizedOffset * settings.maxOffsetBeats;
        return offsetBeats * samplesPerBeat;
    }

    // Get envelope position for current sample
    float getEnvelopePosition(int sampleIndex, int blockSize) const
    {
        // Advance position through block
        float samplesPerLoop = settings.loopLengthBeats * samplesPerBeat;
        float positionOffset = static_cast<float>(sampleIndex) / samplesPerLoop;

        return std::fmod(loopPosition + positionOffset, 1.0f);
    }

    float getLoopPosition() const { return loopPosition; }
    void setSettings(const Settings& s) { settings = s; }

private:
    Settings settings;
    double sampleRate = 44100.0;
    double currentBpm = 120.0;
    double samplesPerBeat = 22050.0;
    float loopPosition = 0.0f;
};
```

#### 1.3 Basic Time Processor

```cpp
// TimeProcessor.h
class TimeProcessor
{
public:
    void prepare(double sampleRate, int samplesPerBlock)
    {
        rollingBuffer.prepare(sampleRate, 2, 8.0f);  // 8 second buffer
        timeMapper.prepare(sampleRate);

        offsetsPerSample.resize(samplesPerBlock);
        this->sampleRate = sampleRate;
    }

    void processBlock(juce::AudioBuffer<float>& buffer,
                      const Envelope& envelope,
                      double bpm, double ppqPosition)
    {
        timeMapper.updateTempo(bpm, ppqPosition);

        // Write input to rolling buffer
        rollingBuffer.write(buffer, buffer.getNumSamples());

        // Calculate offset for each sample based on envelope
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float envPosition = timeMapper.getEnvelopePosition(i, buffer.getNumSamples());
            float envValue = envelope.getValueAt(envPosition);
            offsetsPerSample[i] = timeMapper.envelopeToOffset(envValue);
        }

        // Apply smooth step to reduce clicks
        smoothOffsets(offsetsPerSample);

        // Read from buffer with calculated offsets
        rollingBuffer.readBlock(buffer, offsetsPerSample);
    }

private:
    void smoothOffsets(std::vector<float>& offsets)
    {
        // Apply smoothing to prevent clicks on sharp envelope transitions
        const float smoothingCoeff = 0.99f;
        float smoothedOffset = offsets[0];

        for (size_t i = 0; i < offsets.size(); ++i)
        {
            smoothedOffset = smoothedOffset * smoothingCoeff +
                            offsets[i] * (1.0f - smoothingCoeff);
            offsets[i] = smoothedOffset;
        }
    }

    RollingBuffer rollingBuffer;
    TimeMapper timeMapper;
    std::vector<float> offsetsPerSample;
    double sampleRate = 44100.0;
};
```

---

### Phase 2: Drawable Envelope System (1 week)

#### 2.1 Envelope Data Structure

```cpp
// Envelope.h
class Envelope
{
public:
    struct Point
    {
        float x;           // 0.0 - 1.0 (position in loop)
        float y;           // 0.0 - 1.0 (time offset amount)
        float curve;       // -1 to +1 (curvature of segment after this point)
        bool isLocked;     // Prevent editing
    };

    Envelope()
    {
        // Default: straight line at top (no effect)
        points.push_back({ 0.0f, 1.0f, 0.0f, false });
        points.push_back({ 1.0f, 1.0f, 0.0f, false });
    }

    float getValueAt(float position) const
    {
        if (points.empty()) return 1.0f;

        // Find surrounding points
        size_t rightIdx = 0;
        for (size_t i = 0; i < points.size(); ++i)
        {
            if (points[i].x >= position)
            {
                rightIdx = i;
                break;
            }
            if (i == points.size() - 1)
                rightIdx = i;
        }

        size_t leftIdx = (rightIdx > 0) ? rightIdx - 1 : 0;

        if (leftIdx == rightIdx)
            return points[leftIdx].y;

        // Interpolate between points
        const Point& left = points[leftIdx];
        const Point& right = points[rightIdx];

        float t = (position - left.x) / (right.x - left.x);

        // Apply curve
        if (std::abs(left.curve) > 0.01f)
        {
            // Exponential curve
            if (left.curve > 0)
                t = std::pow(t, 1.0f + left.curve * 2.0f);
            else
                t = 1.0f - std::pow(1.0f - t, 1.0f - left.curve * 2.0f);
        }

        return left.y + (right.y - left.y) * t;
    }

    void addPoint(float x, float y)
    {
        Point p = { x, y, 0.0f, false };

        // Insert in sorted order
        auto it = std::lower_bound(points.begin(), points.end(), p,
            [](const Point& a, const Point& b) { return a.x < b.x; });

        points.insert(it, p);
    }

    void removePoint(size_t index)
    {
        if (index > 0 && index < points.size() - 1)  // Keep first and last
            points.erase(points.begin() + index);
    }

    void movePoint(size_t index, float newX, float newY)
    {
        if (index < points.size())
        {
            // Clamp to valid range
            if (index == 0)
                newX = 0.0f;
            else if (index == points.size() - 1)
                newX = 1.0f;
            else
                newX = juce::jlimit(points[index-1].x + 0.01f,
                                    points[index+1].x - 0.01f, newX);

            newY = juce::jlimit(0.0f, 1.0f, newY);

            points[index].x = newX;
            points[index].y = newY;
        }
    }

    void setCurve(size_t index, float curve)
    {
        if (index < points.size())
            points[index].curve = juce::jlimit(-1.0f, 1.0f, curve);
    }

    // Preset shapes
    void setRampUp()
    {
        points.clear();
        points.push_back({ 0.0f, 0.0f, 0.0f, false });
        points.push_back({ 1.0f, 1.0f, 0.0f, false });
    }

    void setRampDown()
    {
        points.clear();
        points.push_back({ 0.0f, 1.0f, 0.0f, false });
        points.push_back({ 1.0f, 0.0f, 0.0f, false });
    }

    void setStaircase(int steps)
    {
        points.clear();
        for (int i = 0; i <= steps; ++i)
        {
            float x = static_cast<float>(i) / steps;
            float y = 1.0f - static_cast<float>(i) / steps;
            points.push_back({ x, y, 0.0f, false });
            if (i < steps)
                points.push_back({ x + 0.001f, y, 0.0f, false });  // Sharp step
        }
    }

    void setZigzag(int cycles)
    {
        points.clear();
        for (int i = 0; i <= cycles * 2; ++i)
        {
            float x = static_cast<float>(i) / (cycles * 2);
            float y = (i % 2 == 0) ? 1.0f : 0.0f;
            points.push_back({ x, y, 0.0f, false });
        }
    }

    const std::vector<Point>& getPoints() const { return points; }

    // Serialization
    juce::String toJSON() const
    {
        juce::Array<juce::var> pointsArray;
        for (const auto& p : points)
        {
            juce::DynamicObject::Ptr obj = new juce::DynamicObject();
            obj->setProperty("x", p.x);
            obj->setProperty("y", p.y);
            obj->setProperty("curve", p.curve);
            pointsArray.add(obj.get());
        }

        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty("points", pointsArray);
        return juce::JSON::toString(root.get());
    }

    void fromJSON(const juce::String& json)
    {
        auto parsed = juce::JSON::parse(json);
        if (auto* obj = parsed.getDynamicObject())
        {
            points.clear();
            if (auto* arr = obj->getProperty("points").getArray())
            {
                for (const auto& item : *arr)
                {
                    if (auto* pointObj = item.getDynamicObject())
                    {
                        Point p;
                        p.x = pointObj->getProperty("x");
                        p.y = pointObj->getProperty("y");
                        p.curve = pointObj->getProperty("curve");
                        p.isLocked = false;
                        points.push_back(p);
                    }
                }
            }
        }
    }

private:
    std::vector<Point> points;
};
```

#### 2.2 Envelope Editor Component (WebView)

```javascript
// envelope-editor.js
class EnvelopeEditor {
    constructor(canvas, onUpdate) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.onUpdate = onUpdate;

        this.points = [
            { x: 0, y: 1, curve: 0 },
            { x: 1, y: 1, curve: 0 }
        ];

        this.selectedPoint = null;
        this.isDragging = false;
        this.playheadPosition = 0;

        this.setupEventListeners();
        this.draw();
    }

    setupEventListeners() {
        this.canvas.addEventListener('mousedown', (e) => this.onMouseDown(e));
        this.canvas.addEventListener('mousemove', (e) => this.onMouseMove(e));
        this.canvas.addEventListener('mouseup', () => this.onMouseUp());
        this.canvas.addEventListener('dblclick', (e) => this.onDoubleClick(e));
        this.canvas.addEventListener('contextmenu', (e) => this.onRightClick(e));
    }

    // Convert canvas coords to envelope coords
    canvasToEnvelope(canvasX, canvasY) {
        return {
            x: canvasX / this.canvas.width,
            y: 1 - (canvasY / this.canvas.height)
        };
    }

    // Convert envelope coords to canvas coords
    envelopeToCanvas(envX, envY) {
        return {
            x: envX * this.canvas.width,
            y: (1 - envY) * this.canvas.height
        };
    }

    findNearestPoint(envX, envY, threshold = 0.05) {
        for (let i = 0; i < this.points.length; i++) {
            const dx = this.points[i].x - envX;
            const dy = this.points[i].y - envY;
            if (Math.sqrt(dx * dx + dy * dy) < threshold) {
                return i;
            }
        }
        return null;
    }

    onMouseDown(e) {
        const rect = this.canvas.getBoundingClientRect();
        const canvasX = e.clientX - rect.left;
        const canvasY = e.clientY - rect.top;
        const env = this.canvasToEnvelope(canvasX, canvasY);

        this.selectedPoint = this.findNearestPoint(env.x, env.y);

        if (this.selectedPoint !== null) {
            this.isDragging = true;
        }
    }

    onMouseMove(e) {
        if (!this.isDragging || this.selectedPoint === null) return;

        const rect = this.canvas.getBoundingClientRect();
        const canvasX = e.clientX - rect.left;
        const canvasY = e.clientY - rect.top;
        const env = this.canvasToEnvelope(canvasX, canvasY);

        // Clamp and update point
        let newX = Math.max(0, Math.min(1, env.x));
        let newY = Math.max(0, Math.min(1, env.y));

        // Keep first and last points at x=0 and x=1
        if (this.selectedPoint === 0) newX = 0;
        if (this.selectedPoint === this.points.length - 1) newX = 1;

        // Prevent crossing adjacent points
        if (this.selectedPoint > 0) {
            newX = Math.max(this.points[this.selectedPoint - 1].x + 0.01, newX);
        }
        if (this.selectedPoint < this.points.length - 1) {
            newX = Math.min(this.points[this.selectedPoint + 1].x - 0.01, newX);
        }

        this.points[this.selectedPoint].x = newX;
        this.points[this.selectedPoint].y = newY;

        this.draw();
        this.notifyUpdate();
    }

    onMouseUp() {
        this.isDragging = false;
    }

    onDoubleClick(e) {
        // Add new point
        const rect = this.canvas.getBoundingClientRect();
        const canvasX = e.clientX - rect.left;
        const canvasY = e.clientY - rect.top;
        const env = this.canvasToEnvelope(canvasX, canvasY);

        // Don't add if too close to existing point
        if (this.findNearestPoint(env.x, env.y, 0.05) !== null) return;

        // Insert in sorted order
        const newPoint = { x: env.x, y: env.y, curve: 0 };
        let insertIdx = this.points.findIndex(p => p.x > env.x);
        if (insertIdx === -1) insertIdx = this.points.length;

        this.points.splice(insertIdx, 0, newPoint);
        this.draw();
        this.notifyUpdate();
    }

    onRightClick(e) {
        e.preventDefault();

        const rect = this.canvas.getBoundingClientRect();
        const canvasX = e.clientX - rect.left;
        const canvasY = e.clientY - rect.top;
        const env = this.canvasToEnvelope(canvasX, canvasY);

        const pointIdx = this.findNearestPoint(env.x, env.y, 0.05);

        // Don't delete first or last point
        if (pointIdx !== null && pointIdx > 0 && pointIdx < this.points.length - 1) {
            this.points.splice(pointIdx, 1);
            this.draw();
            this.notifyUpdate();
        }
    }

    draw() {
        const ctx = this.ctx;
        const w = this.canvas.width;
        const h = this.canvas.height;

        // Clear
        ctx.fillStyle = '#1a1a2e';
        ctx.fillRect(0, 0, w, h);

        // Grid
        ctx.strokeStyle = '#333';
        ctx.lineWidth = 1;
        for (let i = 0; i <= 4; i++) {
            const x = (i / 4) * w;
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, h);
            ctx.stroke();

            const y = (i / 4) * h;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(w, y);
            ctx.stroke();
        }

        // Playhead
        const playheadX = this.playheadPosition * w;
        ctx.strokeStyle = '#ff0';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(playheadX, 0);
        ctx.lineTo(playheadX, h);
        ctx.stroke();

        // Envelope curve
        ctx.strokeStyle = '#00ffff';
        ctx.lineWidth = 2;
        ctx.beginPath();

        for (let px = 0; px < w; px++) {
            const envX = px / w;
            const envY = this.getValueAt(envX);
            const canvasY = (1 - envY) * h;

            if (px === 0) {
                ctx.moveTo(px, canvasY);
            } else {
                ctx.lineTo(px, canvasY);
            }
        }
        ctx.stroke();

        // Fill under curve
        ctx.lineTo(w, h);
        ctx.lineTo(0, h);
        ctx.closePath();
        ctx.fillStyle = 'rgba(0, 255, 255, 0.1)';
        ctx.fill();

        // Points
        for (let i = 0; i < this.points.length; i++) {
            const canvas = this.envelopeToCanvas(this.points[i].x, this.points[i].y);

            ctx.beginPath();
            ctx.arc(canvas.x, canvas.y, 6, 0, Math.PI * 2);

            if (i === this.selectedPoint) {
                ctx.fillStyle = '#fff';
            } else {
                ctx.fillStyle = '#00ffff';
            }
            ctx.fill();

            ctx.strokeStyle = '#fff';
            ctx.lineWidth = 1;
            ctx.stroke();
        }
    }

    getValueAt(x) {
        if (this.points.length === 0) return 1;

        // Find surrounding points
        let rightIdx = this.points.findIndex(p => p.x >= x);
        if (rightIdx === -1) rightIdx = this.points.length - 1;
        const leftIdx = Math.max(0, rightIdx - 1);

        if (leftIdx === rightIdx) return this.points[leftIdx].y;

        const left = this.points[leftIdx];
        const right = this.points[rightIdx];

        let t = (x - left.x) / (right.x - left.x);

        // Apply curve
        if (Math.abs(left.curve) > 0.01) {
            if (left.curve > 0) {
                t = Math.pow(t, 1 + left.curve * 2);
            } else {
                t = 1 - Math.pow(1 - t, 1 - left.curve * 2);
            }
        }

        return left.y + (right.y - left.y) * t;
    }

    setPlayheadPosition(position) {
        this.playheadPosition = position;
        this.draw();
    }

    setPoints(points) {
        this.points = points.map(p => ({ ...p }));
        this.draw();
    }

    notifyUpdate() {
        if (this.onUpdate) {
            this.onUpdate(this.points);
        }
    }

    // Preset shapes
    setPreset(name) {
        switch (name) {
            case 'rampUp':
                this.points = [
                    { x: 0, y: 0, curve: 0 },
                    { x: 1, y: 1, curve: 0 }
                ];
                break;
            case 'rampDown':
                this.points = [
                    { x: 0, y: 1, curve: 0 },
                    { x: 1, y: 0, curve: 0 }
                ];
                break;
            case 'staircase4':
                this.points = [];
                for (let i = 0; i <= 4; i++) {
                    this.points.push({ x: i/4, y: 1 - i/4, curve: 0 });
                    if (i < 4) this.points.push({ x: i/4 + 0.001, y: 1 - i/4, curve: 0 });
                }
                break;
            case 'zigzag':
                this.points = [
                    { x: 0, y: 1, curve: 0 },
                    { x: 0.25, y: 0, curve: 0 },
                    { x: 0.5, y: 1, curve: 0 },
                    { x: 0.75, y: 0, curve: 0 },
                    { x: 1, y: 1, curve: 0 }
                ];
                break;
            case 'tapeStop':
                this.points = [
                    { x: 0, y: 1, curve: 0.5 },
                    { x: 1, y: 0, curve: 0 }
                ];
                break;
        }
        this.draw();
        this.notifyUpdate();
    }
}
```

---

### Phase 3: Multiband Processing (1 week)

#### 3.1 Band Splitter

```cpp
// BandSplitter.h
class BandSplitter
{
public:
    struct Settings
    {
        float lowCrossover = 200.0f;   // Hz
        float highCrossover = 4000.0f; // Hz
        float slopeDb = 24.0f;         // 12, 24, or 48 dB/oct
    };

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2;

        // Low band: Low-pass at lowCrossover
        lowFilter.prepare(spec);

        // Mid band: Band-pass between crossovers
        midLowCut.prepare(spec);
        midHighCut.prepare(spec);

        // High band: High-pass at highCrossover
        highFilter.prepare(spec);

        updateFilters();
    }

    void updateFilters()
    {
        // Linkwitz-Riley crossover (flat summed response)
        *lowFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate, settings.lowCrossover, 0.707f);

        *midLowCut.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            sampleRate, settings.lowCrossover, 0.707f);
        *midHighCut.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate, settings.highCrossover, 0.707f);

        *highFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            sampleRate, settings.highCrossover, 0.707f);
    }

    void split(const juce::AudioBuffer<float>& input,
               juce::AudioBuffer<float>& low,
               juce::AudioBuffer<float>& mid,
               juce::AudioBuffer<float>& high)
    {
        // Copy input to all bands
        low.makeCopyOf(input);
        mid.makeCopyOf(input);
        high.makeCopyOf(input);

        // Apply filters
        juce::dsp::AudioBlock<float> lowBlock(low);
        juce::dsp::AudioBlock<float> midBlock(mid);
        juce::dsp::AudioBlock<float> highBlock(high);

        juce::dsp::ProcessContextReplacing<float> lowCtx(lowBlock);
        juce::dsp::ProcessContextReplacing<float> midCtx(midBlock);
        juce::dsp::ProcessContextReplacing<float> highCtx(highBlock);

        lowFilter.process(lowCtx);

        midLowCut.process(midCtx);
        midHighCut.process(midCtx);

        highFilter.process(highCtx);
    }

    void recombine(const juce::AudioBuffer<float>& low,
                   const juce::AudioBuffer<float>& mid,
                   const juce::AudioBuffer<float>& high,
                   juce::AudioBuffer<float>& output)
    {
        output.clear();

        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            output.addFrom(ch, 0, low, ch, 0, output.getNumSamples());
            output.addFrom(ch, 0, mid, ch, 0, output.getNumSamples());
            output.addFrom(ch, 0, high, ch, 0, output.getNumSamples());
        }
    }

    void setSettings(const Settings& s)
    {
        settings = s;
        updateFilters();
    }

private:
    Settings settings;
    double sampleRate = 44100.0;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> lowFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> midLowCut;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> midHighCut;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> highFilter;
};
```

#### 3.2 Per-Band Time Processing

```cpp
// MultibandTimeProcessor.h
class MultibandTimeProcessor
{
public:
    void prepare(double sampleRate, int samplesPerBlock)
    {
        bandSplitter.prepare(sampleRate, samplesPerBlock);

        lowProcessor.prepare(sampleRate, samplesPerBlock);
        midProcessor.prepare(sampleRate, samplesPerBlock);
        highProcessor.prepare(sampleRate, samplesPerBlock);

        lowBuffer.setSize(2, samplesPerBlock);
        midBuffer.setSize(2, samplesPerBlock);
        highBuffer.setSize(2, samplesPerBlock);
    }

    void processBlock(juce::AudioBuffer<float>& buffer,
                      const Envelope& lowEnvelope,
                      const Envelope& midEnvelope,
                      const Envelope& highEnvelope,
                      double bpm, double ppqPosition)
    {
        // Split into bands
        bandSplitter.split(buffer, lowBuffer, midBuffer, highBuffer);

        // Process each band with its own envelope
        lowProcessor.processBlock(lowBuffer, lowEnvelope, bpm, ppqPosition);
        midProcessor.processBlock(midBuffer, midEnvelope, bpm, ppqPosition);
        highProcessor.processBlock(highBuffer, highEnvelope, bpm, ppqPosition);

        // Recombine
        bandSplitter.recombine(lowBuffer, midBuffer, highBuffer, buffer);
    }

    void setBandEnabled(int band, bool enabled)
    {
        bandEnabled[band] = enabled;
    }

    void setCrossoverFrequencies(float low, float high)
    {
        BandSplitter::Settings settings;
        settings.lowCrossover = low;
        settings.highCrossover = high;
        bandSplitter.setSettings(settings);
    }

private:
    BandSplitter bandSplitter;

    TimeProcessor lowProcessor;
    TimeProcessor midProcessor;
    TimeProcessor highProcessor;

    juce::AudioBuffer<float> lowBuffer;
    juce::AudioBuffer<float> midBuffer;
    juce::AudioBuffer<float> highBuffer;

    std::array<bool, 3> bandEnabled = { true, true, true };
};
```

---

### Phase 4: Harmonic Locking (3-4 days)

#### 4.1 Pitch Detection During Speed Change

```cpp
// HarmonicLocker.h
class HarmonicLocker
{
public:
    enum class Scale
    {
        Chromatic,
        Major,
        Minor,
        Pentatonic,
        None  // No quantization
    };

    struct Settings
    {
        Scale scale = Scale::Major;
        int rootNote = 0;  // 0 = C
        float strength = 1.0f;  // 0 = off, 1 = full quantization
    };

    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        initScaleTables();
    }

    // Calculate corrected playback rate to snap to harmonic interval
    float getHarmonicRate(float originalRate) const
    {
        if (settings.scale == Scale::None || settings.strength < 0.01f)
            return originalRate;

        // Convert rate to semitones
        // rate = 2^(semitones/12)
        // semitones = 12 * log2(rate)
        float semitones = 12.0f * std::log2(originalRate);

        // Quantize to scale
        float quantized = quantizeToScale(semitones);

        // Blend based on strength
        float blendedSemitones = semitones + (quantized - semitones) * settings.strength;

        // Convert back to rate
        return std::pow(2.0f, blendedSemitones / 12.0f);
    }

    void setSettings(const Settings& s)
    {
        settings = s;
    }

private:
    void initScaleTables()
    {
        // Semitone intervals from root
        scaleIntervals[static_cast<int>(Scale::Chromatic)] =
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        scaleIntervals[static_cast<int>(Scale::Major)] =
            {0, 2, 4, 5, 7, 9, 11};
        scaleIntervals[static_cast<int>(Scale::Minor)] =
            {0, 2, 3, 5, 7, 8, 10};
        scaleIntervals[static_cast<int>(Scale::Pentatonic)] =
            {0, 2, 4, 7, 9};
    }

    float quantizeToScale(float semitones) const
    {
        const auto& scale = scaleIntervals[static_cast<int>(settings.scale)];

        // Find octave and position within octave
        int octave = static_cast<int>(std::floor(semitones / 12.0f));
        float noteInOctave = std::fmod(semitones, 12.0f);
        if (noteInOctave < 0) noteInOctave += 12.0f;

        // Find nearest scale degree
        float nearestDegree = scale[0];
        float minDistance = std::abs(noteInOctave - scale[0]);

        for (int degree : scale)
        {
            float distance = std::abs(noteInOctave - degree);
            // Also check wrapping (e.g., B to C)
            float wrapDistance = std::min(distance, 12.0f - distance);

            if (wrapDistance < minDistance)
            {
                minDistance = wrapDistance;
                nearestDegree = degree;
            }
        }

        // Apply root note offset and return
        return (octave * 12.0f) + nearestDegree + settings.rootNote;
    }

    Settings settings;
    double sampleRate = 44100.0;
    std::array<std::vector<int>, 5> scaleIntervals;
};
```

#### 4.2 Integration with Time Processor

```cpp
// In TimeProcessor::processBlock()
void processBlock(juce::AudioBuffer<float>& buffer,
                  const Envelope& envelope,
                  double bpm, double ppqPosition)
{
    timeMapper.updateTempo(bpm, ppqPosition);
    rollingBuffer.write(buffer, buffer.getNumSamples());

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float envPosition = timeMapper.getEnvelopePosition(i, buffer.getNumSamples());
        float envValue = envelope.getValueAt(envPosition);

        // Calculate playback rate from envelope derivative
        float prevEnvValue = envelope.getValueAt(envPosition - 0.001f);
        float envDerivative = (envValue - prevEnvValue) / 0.001f;

        // Envelope slope determines playback speed
        // Flat = normal speed, steep down = fast rewind, steep up = fast forward
        float playbackRate = 1.0f - envDerivative;

        // Apply harmonic locking
        if (harmonicLockEnabled)
        {
            playbackRate = harmonicLocker.getHarmonicRate(playbackRate);
        }

        // Convert rate to time offset
        offsetsPerSample[i] = calculateOffsetFromRate(playbackRate, i);
    }

    smoothOffsets(offsetsPerSample);
    rollingBuffer.readBlock(buffer, offsetsPerSample);
}
```

---

### Phase 5: Physics-Based Envelope (3-4 days)

#### 5.1 Point Physics Simulation

```javascript
// physics-envelope.js
class PhysicsEnvelope {
    constructor(baseEnvelope) {
        this.basePoints = baseEnvelope.getPoints();
        this.velocities = this.basePoints.map(() => ({ x: 0, y: 0 }));

        this.gravity = 0.0001;      // Downward force
        this.friction = 0.98;       // Velocity decay
        this.bounce = 0.7;          // Energy retained on bounce
        this.stiffness = 0.01;      // Return-to-rest force
    }

    update(dt) {
        for (let i = 1; i < this.basePoints.length - 1; i++) {  // Skip first/last
            const point = this.basePoints[i];
            const vel = this.velocities[i];

            // Apply gravity
            vel.y -= this.gravity * dt;

            // Apply friction
            vel.x *= this.friction;
            vel.y *= this.friction;

            // Apply spring force toward rest position
            // (optional: pull toward original position)

            // Update position
            point.x += vel.x * dt;
            point.y += vel.y * dt;

            // Boundary collisions
            if (point.y < 0) {
                point.y = 0;
                vel.y = -vel.y * this.bounce;
            }
            if (point.y > 1) {
                point.y = 1;
                vel.y = -vel.y * this.bounce;
            }

            // Keep x within bounds of neighbors
            const minX = this.basePoints[i - 1].x + 0.01;
            const maxX = this.basePoints[i + 1].x - 0.01;

            if (point.x < minX) {
                point.x = minX;
                vel.x = -vel.x * this.bounce;
            }
            if (point.x > maxX) {
                point.x = maxX;
                vel.x = -vel.x * this.bounce;
            }
        }
    }

    // "Throw" a point
    applyImpulse(pointIndex, forceX, forceY) {
        if (pointIndex > 0 && pointIndex < this.velocities.length - 1) {
            this.velocities[pointIndex].x += forceX;
            this.velocities[pointIndex].y += forceY;
        }
    }

    // Randomize all velocities
    shake(intensity) {
        for (let i = 1; i < this.velocities.length - 1; i++) {
            this.velocities[i].x = (Math.random() - 0.5) * intensity;
            this.velocities[i].y = (Math.random() - 0.5) * intensity;
        }
    }

    // Settle all points
    freeze() {
        for (const vel of this.velocities) {
            vel.x = 0;
            vel.y = 0;
        }
    }

    setPhysicsParams(gravity, friction, bounce, stiffness) {
        this.gravity = gravity;
        this.friction = friction;
        this.bounce = bounce;
        this.stiffness = stiffness;
    }
}
```

#### 5.2 Motion Recording

```javascript
// motion-recorder.js
class MotionRecorder {
    constructor(envelopeEditor) {
        this.editor = envelopeEditor;
        this.isRecording = false;
        this.recordedMotion = [];
        this.recordStartTime = 0;
    }

    startRecording() {
        this.isRecording = true;
        this.recordedMotion = [];
        this.recordStartTime = performance.now();
    }

    stopRecording() {
        this.isRecording = false;
        return this.normalizeRecording();
    }

    recordMousePosition(x, y) {
        if (!this.isRecording) return;

        const elapsed = performance.now() - this.recordStartTime;
        this.recordedMotion.push({
            time: elapsed,
            x: x,
            y: y
        });
    }

    normalizeRecording() {
        if (this.recordedMotion.length < 2) return null;

        const totalTime = this.recordedMotion[this.recordedMotion.length - 1].time;

        // Convert to envelope points
        const points = this.recordedMotion.map(m => ({
            x: m.time / totalTime,
            y: m.y,
            curve: 0
        }));

        // Simplify (reduce point count while preserving shape)
        return this.simplifyPoints(points, 16);  // Max 16 points
    }

    simplifyPoints(points, maxPoints) {
        if (points.length <= maxPoints) return points;

        // Douglas-Peucker simplification
        const simplified = [points[0]];
        this.douglasPeucker(points, 0, points.length - 1, 0.02, simplified);
        simplified.push(points[points.length - 1]);

        // If still too many, take evenly spaced subset
        if (simplified.length > maxPoints) {
            const step = simplified.length / maxPoints;
            const result = [];
            for (let i = 0; i < maxPoints; i++) {
                result.push(simplified[Math.floor(i * step)]);
            }
            return result;
        }

        return simplified;
    }

    douglasPeucker(points, start, end, epsilon, result) {
        let maxDist = 0;
        let maxIdx = 0;

        for (let i = start + 1; i < end; i++) {
            const dist = this.perpendicularDistance(
                points[i], points[start], points[end]
            );
            if (dist > maxDist) {
                maxDist = dist;
                maxIdx = i;
            }
        }

        if (maxDist > epsilon) {
            this.douglasPeucker(points, start, maxIdx, epsilon, result);
            result.push(points[maxIdx]);
            this.douglasPeucker(points, maxIdx, end, epsilon, result);
        }
    }

    perpendicularDistance(point, lineStart, lineEnd) {
        const dx = lineEnd.x - lineStart.x;
        const dy = lineEnd.y - lineStart.y;

        const t = ((point.x - lineStart.x) * dx + (point.y - lineStart.y) * dy) /
                  (dx * dx + dy * dy);

        const nearestX = lineStart.x + t * dx;
        const nearestY = lineStart.y + t * dy;

        return Math.sqrt(
            Math.pow(point.x - nearestX, 2) +
            Math.pow(point.y - nearestY, 2)
        );
    }
}
```

---

## Complete UI Layout

```html
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            background: linear-gradient(180deg, #0a0a1a 0%, #1a1a3a 100%);
            color: #fff;
            font-family: 'Segoe UI', sans-serif;
            margin: 0;
            padding: 20px;
        }

        .main-container {
            display: grid;
            grid-template-columns: 1fr 200px;
            gap: 20px;
        }

        .envelope-section {
            background: rgba(0,0,0,0.3);
            border-radius: 10px;
            padding: 15px;
        }

        .envelope-header {
            display: flex;
            justify-content: space-between;
            margin-bottom: 10px;
        }

        .envelope-canvas {
            width: 100%;
            height: 200px;
            background: #111;
            border-radius: 5px;
        }

        .multiband-envelopes {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
            margin-top: 20px;
        }

        .band-envelope {
            background: rgba(0,0,0,0.3);
            padding: 10px;
            border-radius: 5px;
        }

        .band-envelope canvas {
            width: 100%;
            height: 100px;
        }

        .band-label {
            font-size: 12px;
            margin-bottom: 5px;
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .band-low .band-label { color: #ff6b6b; }
        .band-mid .band-label { color: #4ecdc4; }
        .band-high .band-label { color: #45b7d1; }

        .controls-panel {
            background: rgba(0,0,0,0.3);
            border-radius: 10px;
            padding: 15px;
        }

        .control-group {
            margin-bottom: 20px;
        }

        .control-label {
            font-size: 11px;
            opacity: 0.7;
            margin-bottom: 5px;
        }

        .preset-buttons {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 5px;
        }

        .preset-btn {
            background: #333;
            border: none;
            color: #fff;
            padding: 8px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 11px;
        }

        .preset-btn:hover {
            background: #444;
        }

        .preset-btn.active {
            background: #00ffff;
            color: #000;
        }

        .slider-control {
            margin: 10px 0;
        }

        .slider-control input {
            width: 100%;
        }

        .toggle-row {
            display: flex;
            gap: 10px;
            margin: 10px 0;
        }

        .toggle-btn {
            flex: 1;
            padding: 8px;
            background: #333;
            border: none;
            color: #fff;
            border-radius: 5px;
            cursor: pointer;
        }

        .toggle-btn.active {
            background: #00ffff;
            color: #000;
        }

        .harmonic-section {
            border-top: 1px solid #333;
            padding-top: 15px;
            margin-top: 15px;
        }

        .scale-select {
            width: 100%;
            background: #222;
            color: #fff;
            border: 1px solid #444;
            padding: 8px;
            border-radius: 5px;
        }

        .physics-section {
            border-top: 1px solid #333;
            padding-top: 15px;
            margin-top: 15px;
        }

        .physics-btn {
            width: 100%;
            padding: 10px;
            margin: 5px 0;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            border: none;
            color: #fff;
            border-radius: 5px;
            cursor: pointer;
        }

        .record-btn {
            background: linear-gradient(135deg, #f5576c 0%, #f093fb 100%);
        }

        .record-btn.recording {
            animation: pulse-red 0.5s infinite alternate;
        }

        @keyframes pulse-red {
            from { background: #f00; }
            to { background: #c00; }
        }
    </style>
</head>
<body>
    <div class="main-container">
        <!-- Main envelope editor -->
        <div class="envelope-section">
            <div class="envelope-header">
                <span>TIME ENVELOPE</span>
                <div class="toggle-row">
                    <button class="toggle-btn active" id="link-bands">LINK</button>
                    <button class="toggle-btn" id="split-bands">SPLIT</button>
                </div>
            </div>

            <canvas class="envelope-canvas" id="main-envelope" width="800" height="200"></canvas>

            <!-- Multiband envelopes (shown when split) -->
            <div class="multiband-envelopes" id="multiband-section" style="display: none;">
                <div class="band-envelope band-low">
                    <div class="band-label">
                        <input type="checkbox" checked> LOW
                        <span id="low-crossover">200 Hz</span>
                    </div>
                    <canvas id="low-envelope" width="250" height="100"></canvas>
                </div>
                <div class="band-envelope band-mid">
                    <div class="band-label">
                        <input type="checkbox" checked> MID
                    </div>
                    <canvas id="mid-envelope" width="250" height="100"></canvas>
                </div>
                <div class="band-envelope band-high">
                    <div class="band-label">
                        <input type="checkbox" checked> HIGH
                        <span id="high-crossover">4000 Hz</span>
                    </div>
                    <canvas id="high-envelope" width="250" height="100"></canvas>
                </div>
            </div>
        </div>

        <!-- Controls panel -->
        <div class="controls-panel">
            <!-- Presets -->
            <div class="control-group">
                <div class="control-label">PRESETS</div>
                <div class="preset-buttons">
                    <button class="preset-btn" data-preset="rampUp">Ramp ↑</button>
                    <button class="preset-btn" data-preset="rampDown">Ramp ↓</button>
                    <button class="preset-btn" data-preset="staircase4">Stairs</button>
                    <button class="preset-btn" data-preset="zigzag">Zigzag</button>
                    <button class="preset-btn" data-preset="tapeStop">Tape Stop</button>
                    <button class="preset-btn" data-preset="scratch">Scratch</button>
                </div>
            </div>

            <!-- Loop settings -->
            <div class="control-group">
                <div class="control-label">LOOP LENGTH</div>
                <select class="scale-select" id="loop-length">
                    <option value="0.5">1/2 Beat</option>
                    <option value="1">1 Beat</option>
                    <option value="2">2 Beats</option>
                    <option value="4" selected>1 Bar</option>
                    <option value="8">2 Bars</option>
                    <option value="16">4 Bars</option>
                </select>

                <div class="control-label" style="margin-top: 10px;">MAX OFFSET</div>
                <select class="scale-select" id="max-offset">
                    <option value="1">1 Beat</option>
                    <option value="2">2 Beats</option>
                    <option value="4" selected>1 Bar</option>
                    <option value="8">2 Bars</option>
                </select>
            </div>

            <!-- Harmonic lock -->
            <div class="control-group harmonic-section">
                <div class="control-label">HARMONIC LOCK</div>
                <div class="toggle-row">
                    <button class="toggle-btn" id="harmonic-off">OFF</button>
                    <button class="toggle-btn active" id="harmonic-on">ON</button>
                </div>

                <div class="control-label" style="margin-top: 10px;">SCALE</div>
                <select class="scale-select" id="scale-select">
                    <option value="0">Chromatic</option>
                    <option value="1" selected>Major</option>
                    <option value="2">Minor</option>
                    <option value="3">Pentatonic</option>
                </select>

                <div class="control-label" style="margin-top: 10px;">ROOT</div>
                <select class="scale-select" id="root-select">
                    <option value="0">C</option>
                    <option value="1">C#</option>
                    <option value="2">D</option>
                    <option value="3">D#</option>
                    <option value="4">E</option>
                    <option value="5">F</option>
                    <option value="6">F#</option>
                    <option value="7">G</option>
                    <option value="8">G#</option>
                    <option value="9">A</option>
                    <option value="10">A#</option>
                    <option value="11">B</option>
                </select>

                <div class="slider-control">
                    <div class="control-label">STRENGTH</div>
                    <input type="range" min="0" max="100" value="100" id="harmonic-strength">
                </div>
            </div>

            <!-- Physics -->
            <div class="control-group physics-section">
                <div class="control-label">PHYSICS</div>
                <button class="physics-btn" id="shake-btn">SHAKE</button>
                <button class="physics-btn" id="freeze-btn">FREEZE</button>

                <div class="slider-control">
                    <div class="control-label">GRAVITY</div>
                    <input type="range" min="0" max="100" value="50" id="gravity">
                </div>
                <div class="slider-control">
                    <div class="control-label">BOUNCE</div>
                    <input type="range" min="0" max="100" value="70" id="bounce">
                </div>
            </div>

            <!-- Motion recording -->
            <div class="control-group">
                <button class="physics-btn record-btn" id="record-btn">
                    ● RECORD MOTION
                </button>
            </div>

            <!-- Mix -->
            <div class="control-group">
                <div class="slider-control">
                    <div class="control-label">DRY / WET</div>
                    <input type="range" min="0" max="100" value="100" id="mix">
                </div>
            </div>
        </div>
    </div>

    <script src="envelope-editor.js"></script>
    <script src="physics-envelope.js"></script>
    <script src="motion-recorder.js"></script>
    <script src="main.js"></script>
</body>
</html>
```

---

## Testing Checklist

### Core Time Engine
- [ ] Rolling buffer captures audio correctly
- [ ] Playhead position tracks host PPQ
- [ ] Envelope values map to time offsets correctly
- [ ] Smooth step prevents clicks on sharp transitions
- [ ] Interpolation quality is artifact-free

### Envelope Editor
- [ ] Points can be added with double-click
- [ ] Points can be removed with right-click
- [ ] Points can be dragged
- [ ] First/last points locked to x=0 and x=1
- [ ] Curve parameter adjusts segment shape
- [ ] Playhead position displays correctly
- [ ] Preset shapes load correctly

### Multiband Processing
- [ ] Band splitter separates frequencies correctly
- [ ] Each band can have independent envelope
- [ ] Bands recombine without phase issues
- [ ] Crossover frequencies are adjustable
- [ ] Bands can be soloed/muted

### Harmonic Locking
- [ ] Pitch snaps to scale degrees during speed change
- [ ] All scale types work correctly
- [ ] Root note transpose works
- [ ] Strength parameter blends between locked/unlocked

### Physics System
- [ ] Points respond to gravity
- [ ] Points bounce off boundaries
- [ ] Friction slows motion over time
- [ ] Shake randomizes velocities
- [ ] Freeze stops all motion

### Motion Recording
- [ ] Recording captures mouse movement
- [ ] Playback reproduces recorded motion
- [ ] Recording can be saved as preset
- [ ] Point simplification reduces complexity

---

## File Structure

```
SpectralTimeShaper/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.cpp
│   ├── PluginProcessor.h
│   ├── PluginEditor.cpp
│   ├── PluginEditor.h
│   ├── RollingBuffer.h
│   ├── TimeMapper.h
│   ├── TimeProcessor.cpp
│   ├── TimeProcessor.h
│   ├── Envelope.cpp
│   ├── Envelope.h
│   ├── BandSplitter.cpp
│   ├── BandSplitter.h
│   ├── MultibandTimeProcessor.cpp
│   ├── MultibandTimeProcessor.h
│   ├── HarmonicLocker.cpp
│   ├── HarmonicLocker.h
│   └── ui/
│       └── public/
│           ├── index.html
│           ├── envelope-editor.js
│           ├── physics-envelope.js
│           ├── motion-recorder.js
│           ├── main.js
│           └── styles.css
├── Presets/
│   ├── Default.txt
│   ├── TapeStop.txt
│   ├── HalfTime.txt
│   ├── Scratch.txt
│   └── GlitchHigbs.txt
└── .ideas/
    ├── creative-brief.md
    ├── architecture.md
    ├── parameter-spec.md
    └── plan.md
```

---

## Future Enhancements

1. **Waveform Display:** Show audio waveform behind envelope
2. **LFO Modulation:** Modulate envelope position with LFO
3. **MIDI Trigger:** Different envelopes on different MIDI notes
4. **Automation Recording:** Record parameter automation as envelope
5. **Morph Between Presets:** Crossfade between two envelope shapes
6. **AI Curve Generation:** "Generate build-up" or "Generate breakdown"
7. **Spectral Display:** Show frequency content of each band
8. **Freeze Frame:** Capture and hold specific moment
