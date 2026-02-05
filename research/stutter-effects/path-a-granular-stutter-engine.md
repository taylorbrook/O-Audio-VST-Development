---
title: "Granular Stutter Engine Architecture"
summary: "Design and implementation plan for a granular stutter engine extending the Scatter plugin with beat-synchronized triggering, 64-voice granular synthesis, and musical scale quantization."
domain: dsp
type: algorithm
keywords:
  - stutter
  - stutter-effects
  - granular
  - beat-sync
  - grain-scheduling
  - scale-quantization
  - voice-pool
stages: [0, 2]
agents: [dsp, research]
---

# Path A: Granular Stutter Engine

**Extend Scatter with Beat-Synchronized Triggering**

**Estimated Development Time:** 1-2 weeks
**Complexity:** Medium
**Starting Point:** Existing Scatter plugin (80% complete)

---

## Unique Value Proposition

**"Harmonic Stutter"** - The only stutter effect that combines:
- 64-voice granular synthesis
- Musical scale quantization (pitches snap to chords)
- Beat-synchronized triggering
- Density-based texture morphing (stutter → cloud)

No commercial plugin offers this combination. Portal ($99) has granular + scales. Stutter Edit ($99) has beat-sync + gestures. This merges both paradigms.

---

## Architecture Overview

```
                                    ┌─────────────────┐
                                    │   Beat Clock    │
                                    │  (PPQ-based)    │
                                    └────────┬────────┘
                                             │
                                             ▼
┌─────────┐    ┌──────────────┐    ┌─────────────────┐    ┌────────────────┐
│  Input  │───▶│ Delay Buffer │───▶│ Grain Scheduler │───▶│  Voice Pool    │
└─────────┘    │  (2 sec)     │    │  + Beat Sync    │    │  (64 voices)   │
               └──────────────┘    └─────────────────┘    └───────┬────────┘
                      ▲                     │                     │
                      │                     ▼                     ▼
                      │            ┌─────────────────┐    ┌────────────────┐
                      │            │ Freeze Control  │    │ Scale Quantizer│
                      │            └─────────────────┘    └───────┬────────┘
                      │                                           │
                      │                                           ▼
                      │                                   ┌────────────────┐
                      └───────────────────────────────────│   Dry/Wet Mix  │───▶ Output
                                                          └────────────────┘
```

---

## Implementation Phases

### Phase 1: Beat-Sync Infrastructure (2-3 days)

**Goal:** Add tempo-synchronized grain triggering alongside existing density-based scheduling.

#### 1.1 Add New Parameters

```cpp
// In createParameterLayout()

// Sync mode: Free (density-based) vs Sync (beat-based)
layout.add(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { "sync_mode", 1 },
    "Sync Mode",
    juce::StringArray { "Free", "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T" },
    0  // Default: Free (existing behavior)
));

// Stutter probability: chance of triggering on each beat
layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "stutter_probability", 1 },
    "Probability",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    100.0f,  // Default: always trigger
    "%"
));

// Repeat count: how many times to repeat before returning to live
layout.add(std::make_unique<juce::AudioParameterInt>(
    juce::ParameterID { "repeat_count", 1 },
    "Repeats",
    1, 16, 4
));

// Freeze toggle
layout.add(std::make_unique<juce::AudioParameterBool>(
    juce::ParameterID { "freeze", 1 },
    "Freeze",
    false
));
```

#### 1.2 Tempo Tracking System

```cpp
// Add to PluginProcessor.h
class TempoTracker
{
public:
    struct SyncInfo
    {
        double bpm = 120.0;
        double ppqPosition = 0.0;
        double samplesPerBeat = 22050.0;
        bool isPlaying = false;
    };

    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        lastPpqPosition = 0.0;
    }

    SyncInfo update(juce::AudioPlayHead* playHead, int numSamples)
    {
        SyncInfo info;

        if (playHead != nullptr)
        {
            if (auto pos = playHead->getPosition())
            {
                info.bpm = pos->getBpm().orFallback(120.0);
                info.ppqPosition = pos->getPpqPosition().orFallback(manualPpq);
                info.isPlaying = pos->getIsPlaying();
            }
        }
        else
        {
            // Fallback for standalone/offline
            info.bpm = 120.0;
            info.ppqPosition = manualPpq;
            info.isPlaying = true;
        }

        info.samplesPerBeat = (60.0 / info.bpm) * sampleRate;

        // Advance manual position
        double beatsThisBuffer = numSamples / info.samplesPerBeat;
        manualPpq += beatsThisBuffer;
        lastPpqPosition = info.ppqPosition;

        return info;
    }

    // Check if we crossed a subdivision boundary
    bool crossedSubdivision(double oldPpq, double newPpq, double subdivision)
    {
        double oldQuantized = std::floor(oldPpq / subdivision);
        double newQuantized = std::floor(newPpq / subdivision);
        return newQuantized > oldQuantized;
    }

private:
    double sampleRate = 44100.0;
    double lastPpqPosition = 0.0;
    double manualPpq = 0.0;
};

// Subdivision values in beats (quarter notes)
static constexpr double SUBDIV_QUARTER = 1.0;
static constexpr double SUBDIV_EIGHTH = 0.5;
static constexpr double SUBDIV_SIXTEENTH = 0.25;
static constexpr double SUBDIV_THIRTYSECOND = 0.125;
static constexpr double SUBDIV_EIGHTH_TRIPLET = 1.0 / 3.0;
static constexpr double SUBDIV_SIXTEENTH_TRIPLET = 1.0 / 6.0;
```

#### 1.3 Beat-Aligned Grain Spawning

```cpp
// Modify updateGrainScheduler() in PluginProcessor.cpp

void ScatterAudioProcessor::updateGrainScheduler(/* existing params */,
                                                   int syncModeIndex,
                                                   float stutterProbability)
{
    // Get sync info
    auto syncInfo = tempoTracker.update(getPlayHead(), getBlockSize());

    if (syncModeIndex == 0)
    {
        // FREE MODE: Use existing density-based scheduling
        // ... existing code ...
    }
    else
    {
        // SYNC MODE: Spawn on beat subdivisions
        double subdivision = getSubdivisionFromIndex(syncModeIndex);

        // Calculate PPQ position at each sample
        double ppqIncrement = 1.0 / syncInfo.samplesPerBeat;

        for (int sample = 0; sample < getBlockSize(); ++sample)
        {
            double currentPpq = syncInfo.ppqPosition + (sample * ppqIncrement);
            double prevPpq = currentPpq - ppqIncrement;

            if (tempoTracker.crossedSubdivision(prevPpq, currentPpq, subdivision))
            {
                // Probability gate
                if (juce::Random::getSystemRandom().nextFloat() * 100.0f < stutterProbability)
                {
                    spawnSyncedGrain(grainSizeMs, pitchRandomPercent,
                                     panRandomPercent, scaleIndex, rootNote);
                }
            }
        }
    }
}

double ScatterAudioProcessor::getSubdivisionFromIndex(int index)
{
    switch (index)
    {
        case 1: return SUBDIV_QUARTER;
        case 2: return SUBDIV_EIGHTH;
        case 3: return SUBDIV_SIXTEENTH;
        case 4: return SUBDIV_THIRTYSECOND;
        case 5: return SUBDIV_EIGHTH_TRIPLET;
        case 6: return SUBDIV_SIXTEENTH_TRIPLET;
        default: return SUBDIV_EIGHTH;
    }
}
```

---

### Phase 2: Freeze Mode (1-2 days)

**Goal:** Capture current buffer state and loop indefinitely until released.

#### 2.1 Freeze State Management

```cpp
// Add to PluginProcessor.h
struct FreezeState
{
    bool active = false;
    int capturePosition = 0;      // Where in delay buffer freeze started
    int captureLength = 0;        // Length of frozen segment (in samples)
    juce::AudioBuffer<float> frozenBuffer;  // Copy of frozen audio
};

FreezeState freezeState;

// Add methods
void engageFreeze();
void releaseFreeze();
```

#### 2.2 Freeze Implementation

```cpp
void ScatterAudioProcessor::engageFreeze()
{
    if (freezeState.active) return;

    // Calculate freeze length based on current grain size
    auto* grainSizeParam = parameters.getRawParameterValue("grain_size");
    float grainSizeMs = grainSizeParam->load();
    int grainSizeSamples = static_cast<int>(currentSampleRate * grainSizeMs / 1000.0f);

    // Capture from delay buffer
    freezeState.captureLength = grainSizeSamples * 4;  // Capture 4x grain size
    freezeState.captureLength = juce::jmin(freezeState.captureLength, currentDelayBufferSize);

    freezeState.frozenBuffer.setSize(2, freezeState.captureLength);

    // Copy from delay buffer to frozen buffer
    for (int channel = 0; channel < 2; ++channel)
    {
        for (int i = 0; i < freezeState.captureLength; ++i)
        {
            float sample = delayBuffer.popSample(channel, i, false);
            freezeState.frozenBuffer.setSample(channel, i, sample);
        }
    }

    freezeState.active = true;

    // Reset all grains to read from frozen buffer
    for (auto& grain : grainVoices)
    {
        if (grain.active)
        {
            grain.readPosition = std::fmod(grain.readPosition,
                                            static_cast<float>(freezeState.captureLength));
        }
    }
}

void ScatterAudioProcessor::releaseFreeze()
{
    freezeState.active = false;
    // Grains will naturally transition back to live delay buffer
}
```

#### 2.3 Modify Grain Reading for Freeze

```cpp
void ScatterAudioProcessor::processGrainVoices(juce::AudioBuffer<float>& buffer)
{
    // ... existing setup ...

    for (auto& grain : grainVoices)
    {
        if (!grain.active) continue;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // ... existing window calculation ...

            // READ FROM FROZEN BUFFER OR DELAY BUFFER
            float delayedSample;
            if (freezeState.active)
            {
                // Read from frozen buffer with wrap
                int readPos = static_cast<int>(grain.readPosition) % freezeState.captureLength;
                delayedSample = freezeState.frozenBuffer.getSample(0, readPos);
            }
            else
            {
                // Original: read from delay buffer
                delayedSample = delayBuffer.popSample(0, grain.readPosition);
            }

            // ... rest of existing processing ...
        }
    }
}
```

---

### Phase 3: Unique Features (3-4 days)

#### 3.1 Pitch Ladder Mode

Each successive grain shifts up by a scale degree, creating arpeggiated stutters.

```cpp
// Add parameter
layout.add(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { "pitch_mode", 1 },
    "Pitch Mode",
    juce::StringArray { "Random", "Ladder Up", "Ladder Down", "Pendulum" },
    0
));

// Add state tracking
int ladderPosition = 0;  // Current position in scale
bool ladderDirection = true;  // true = up, false = down

// Modify spawnNewGrain()
void ScatterAudioProcessor::spawnNewGrain(/* params */, int pitchMode)
{
    int quantizedPitch;

    switch (pitchMode)
    {
        case 0:  // Random (existing behavior)
            quantizedPitch = quantizePitchToScale(randomPitch, scaleIndex, rootNote);
            break;

        case 1:  // Ladder Up
        {
            const auto& scale = scaleIntervals[scaleIndex];
            quantizedPitch = scale[ladderPosition % scale.size()] + rootNote;
            ladderPosition++;
            break;
        }

        case 2:  // Ladder Down
        {
            const auto& scale = scaleIntervals[scaleIndex];
            int scaleSize = static_cast<int>(scale.size());
            quantizedPitch = scale[(scaleSize - 1 - (ladderPosition % scaleSize))] + rootNote;
            ladderPosition++;
            break;
        }

        case 3:  // Pendulum (up then down)
        {
            const auto& scale = scaleIntervals[scaleIndex];
            int scaleSize = static_cast<int>(scale.size());

            if (ladderDirection)
            {
                quantizedPitch = scale[ladderPosition] + rootNote;
                ladderPosition++;
                if (ladderPosition >= scaleSize)
                {
                    ladderPosition = scaleSize - 2;
                    ladderDirection = false;
                }
            }
            else
            {
                quantizedPitch = scale[ladderPosition] + rootNote;
                ladderPosition--;
                if (ladderPosition < 0)
                {
                    ladderPosition = 1;
                    ladderDirection = true;
                }
            }
            break;
        }
    }

    // Convert to playback rate
    float playbackRate = std::pow(2.0f, quantizedPitch / 12.0f);
    // ... rest of grain initialization
}
```

#### 3.2 Density Morph (Stutter → Cloud)

Smooth transition from single repeating grain to full granular texture.

```cpp
// Add parameter
layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "texture", 1 },
    "Texture",
    juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
    0.0f,  // 0% = pure stutter, 100% = full cloud
    "%"
));

// Modify grain spawning based on texture
void ScatterAudioProcessor::updateGrainScheduler(/* params */, float texture)
{
    // At 0% texture: All grains spawn at same position (unison stutter)
    // At 100% texture: Grains spawn at random positions (original behavior)

    float positionSpread = texture / 100.0f;

    // When spawning grain:
    float basePosition = 0.0f;  // Most recent audio
    float randomOffset = juce::Random::getSystemRandom().nextFloat()
                         * currentDelayBufferSize * positionSpread;

    newGrain.readPosition = basePosition + randomOffset;

    // Also modulate density based on texture
    // Low texture = fewer grains (1-4), high texture = many grains
    int maxActiveGrains = static_cast<int>(4 + (60 * texture / 100.0f));
    // ... enforce grain limit ...
}
```

#### 3.3 Euclidean Rhythm Patterns

Mathematically-distributed trigger patterns.

```cpp
// Add parameter
layout.add(std::make_unique<juce::AudioParameterInt>(
    juce::ParameterID { "euclidean_pulses", 1 },
    "Pulses",
    1, 16, 4
));

layout.add(std::make_unique<juce::AudioParameterInt>(
    juce::ParameterID { "euclidean_steps", 1 },
    "Steps",
    2, 16, 8
));

// Euclidean pattern generator
std::vector<bool> generateEuclideanPattern(int steps, int pulses)
{
    std::vector<bool> pattern(steps, false);

    if (pulses >= steps)
    {
        std::fill(pattern.begin(), pattern.end(), true);
        return pattern;
    }

    if (pulses == 0) return pattern;

    // Bjorklund's algorithm
    int bucket = 0;
    for (int i = 0; i < steps; ++i)
    {
        bucket += pulses;
        if (bucket >= steps)
        {
            bucket -= steps;
            pattern[i] = true;
        }
    }

    return pattern;
}

// Use in scheduler
int euclideanStep = 0;
std::vector<bool> euclideanPattern;

void ScatterAudioProcessor::updateEuclideanPattern(int pulses, int steps)
{
    euclideanPattern = generateEuclideanPattern(steps, pulses);
    euclideanStep = 0;
}

// In beat-sync mode, check euclidean pattern
if (crossedSubdivision && euclideanPattern[euclideanStep])
{
    spawnNewGrain(/* ... */);
}
euclideanStep = (euclideanStep + 1) % euclideanPattern.size();
```

---

### Phase 4: UI Integration (2-3 days)

#### 4.1 New WebView Controls

Add to `index.html`:

```html
<!-- Sync Section -->
<div class="control-group sync-section">
    <div class="dropdown-control">
        <label>Sync</label>
        <select id="sync_mode">
            <option value="0">Free</option>
            <option value="1">1/4</option>
            <option value="2">1/8</option>
            <option value="3">1/16</option>
            <option value="4">1/32</option>
            <option value="5">1/8T</option>
            <option value="6">1/16T</option>
        </select>
    </div>

    <div class="knob-control">
        <div class="knob" data-param="stutter_probability" data-min="0" data-max="100"></div>
        <label>Probability</label>
    </div>

    <div class="knob-control">
        <div class="knob" data-param="repeat_count" data-min="1" data-max="16"></div>
        <label>Repeats</label>
    </div>

    <button id="freeze_btn" class="toggle-button">FREEZE</button>
</div>

<!-- Texture Section -->
<div class="control-group texture-section">
    <div class="slider-control">
        <label>Stutter ←→ Cloud</label>
        <input type="range" id="texture" min="0" max="100" value="0">
    </div>

    <div class="dropdown-control">
        <label>Pitch Mode</label>
        <select id="pitch_mode">
            <option value="0">Random</option>
            <option value="1">Ladder ↑</option>
            <option value="2">Ladder ↓</option>
            <option value="3">Pendulum</option>
        </select>
    </div>
</div>

<!-- Euclidean Section -->
<div class="control-group euclidean-section">
    <div class="knob-control">
        <div class="knob" data-param="euclidean_pulses" data-min="1" data-max="16"></div>
        <label>Pulses</label>
    </div>
    <div class="knob-control">
        <div class="knob" data-param="euclidean_steps" data-min="2" data-max="16"></div>
        <label>Steps</label>
    </div>
    <div class="euclidean-visualizer" id="euclidean_viz"></div>
</div>
```

#### 4.2 Euclidean Pattern Visualizer

```javascript
// In JS module
function drawEuclideanPattern(canvas, pattern) {
    const ctx = canvas.getContext('2d');
    const steps = pattern.length;
    const radius = Math.min(canvas.width, canvas.height) / 2 - 10;
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Draw circle
    ctx.strokeStyle = '#333';
    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, 0, Math.PI * 2);
    ctx.stroke();

    // Draw steps
    for (let i = 0; i < steps; i++) {
        const angle = (i / steps) * Math.PI * 2 - Math.PI / 2;
        const x = centerX + Math.cos(angle) * radius;
        const y = centerY + Math.sin(angle) * radius;

        ctx.beginPath();
        ctx.arc(x, y, pattern[i] ? 8 : 4, 0, Math.PI * 2);
        ctx.fillStyle = pattern[i] ? '#00ffff' : '#444';
        ctx.fill();
    }
}
```

---

## Testing Checklist

### Functional Tests
- [ ] Free mode works exactly as before (no regression)
- [ ] Beat sync triggers at correct positions for all subdivisions
- [ ] Triplet timing is accurate
- [ ] Probability gate works (0% = silence, 100% = every beat)
- [ ] Freeze captures current audio correctly
- [ ] Freeze release transitions smoothly
- [ ] Pitch ladder cycles through scale correctly
- [ ] Pendulum mode reverses direction at boundaries
- [ ] Texture morph smoothly transitions stutter → cloud
- [ ] Euclidean patterns generate correctly

### Edge Cases
- [ ] Tempo change mid-playback doesn't cause clicks
- [ ] Standalone mode (no playhead) works with manual tempo
- [ ] Offline rendering produces same results as real-time
- [ ] Very fast subdivisions (1/32 at 180 BPM) don't overload CPU
- [ ] Freeze with feedback doesn't cause runaway gain

### DAW Compatibility
- [ ] Ableton Live: PPQ position reads correctly
- [ ] Logic Pro: Tempo sync works
- [ ] FL Studio: Transport state detection
- [ ] Reaper: Offline bounce produces correct output

---

## File Changes Summary

| File | Changes |
|------|---------|
| `PluginProcessor.h` | Add TempoTracker, FreezeState, new parameters |
| `PluginProcessor.cpp` | Beat-sync scheduler, freeze logic, pitch modes |
| `index.html` | New control sections for sync/texture/euclidean |
| `main.js` | Euclidean visualizer, freeze button handling |
| `styles.css` | Styling for new sections |

---

## Future Enhancements

After core implementation:

1. **Transient Detection:** Auto-trigger on drum hits using envelope follower
2. **MIDI Learn:** Trigger specific patterns from MIDI notes
3. **Modulation Matrix:** LFO → any parameter
4. **Preset Morphing:** Crossfade between two preset states
5. **Sidechain Trigger:** External audio triggers stutter
