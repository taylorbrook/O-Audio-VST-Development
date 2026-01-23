# O-Bass Architecture

**Domain:** Psychoacoustic Bass Enhancement VST Plugin
**Researched:** 2026-01-22
**Confidence:** HIGH (based on existing Ouaricon codebase analysis + verified DSP techniques)

## Executive Summary

O-Bass integrates with the existing Ouaricon plugin architecture using the established PluginProcessor/PluginEditor pattern with WebView UI. The DSP pipeline uses a parallel processing approach: the original low frequencies are processed through either a psychoacoustic enhancer (Clean mode) or a harmonic saturator (Colored mode), then mixed back with the high-frequency content.

The architecture leverages two distinct enhancement techniques:
1. **Clean Mode**: Psychoacoustic harmonic synthesis using the "missing fundamental" phenomenon
2. **Colored Mode**: Analog-modeled saturation based on the existing OuariconSaturationModeling patterns

## Recommended Architecture

```
                              O-Bass Signal Flow
    ================================================================================

    INPUT ──────┬──────────────────────────────────────────────────────┬──────> HPF ─┐
                │                                                      │             │
                ▼                                                      │             │
           ┌─────────┐                                                 │             │
           │ SIDECHAIN│  (envelope for Clean mode)                     │             │
           │ DETECTOR │                                                │             │
           └────┬────┘                                                 │             │
                │                                                      │             │
                ▼                                                      │             │
    ┌───────────────────────────────────────────────────────────────┐  │             │
    │                       BASS PROCESSOR                          │  │             │
    │  ┌─────────────────────────────────────────────────────────┐  │  │             │
    │  │                     [MODE SWITCH]                       │  │  │             │
    │  │                                                         │  │  │             │
    │  │  CLEAN PATH                    COLORED PATH             │  │  │             │
    │  │  ──────────                    ────────────             │  │  │             │
    │  │  INPUT ─> LPF ─┐               INPUT ─> LPF ─┐          │  │  │             │
    │  │                │                             │          │  │  │             │
    │  │                ▼                             ▼          │  │  │             │
    │  │         ┌──────────────┐              ┌──────────────┐  │  │  │             │
    │  │         │  FULL-WAVE   │              │   ANALOG     │  │  │  │             │
    │  │         │  INTEGRATOR  │              │  SATURATION  │  │  │  │             │
    │  │         │  (Harmonic   │              │  (Tube/Mag   │  │  │  │             │
    │  │         │  Generator)  │              │   Models)    │  │  │  │             │
    │  │         └──────┬───────┘              └──────┬───────┘  │  │  │             │
    │  │                │                             │          │  │  │             │
    │  │                ▼                             ▼          │  │  │             │
    │  │         ┌──────────────┐              ┌──────────────┐  │  │  │             │
    │  │         │   BANDPASS   │              │  OUTPUT LPF  │  │  │  │             │
    │  │         │   FILTER     │              │  (Resonance) │  │  │  │             │
    │  │         │  (60-250Hz)  │              │              │  │  │  │             │
    │  │         └──────┬───────┘              └──────┬───────┘  │  │  │             │
    │  │                │                             │          │  │  │             │
    │  │                ▼                             ▼          │  │  │             │
    │  │         ┌──────────────────────────────────────────┐    │  │  │             │
    │  │         │            ENHANCEMENT GAIN              │    │  │  │             │
    │  │         │        (controlled by AMOUNT knob)       │    │  │  │             │
    │  │         └────────────────────┬─────────────────────┘    │  │  │             │
    │  └──────────────────────────────┼──────────────────────────┘  │  │             │
    │                                 │                             │  │             │
    └─────────────────────────────────┼─────────────────────────────┘  │             │
                                      │                                │             │
                                      ▼                                │             │
                              ┌──────────────┐                         │             │
                              │    MIXER     │ <────────────────────────┘             │
                              │  (dry/wet)   │ <──────────────────────────────────────┘
                              └──────┬───────┘
                                     │
                                     ▼
                              ┌──────────────┐
                              │ OUTPUT GAIN  │
                              └──────┬───────┘
                                     │
                                     ▼
                                  OUTPUT
```

## Component Breakdown

### 1. OBassAudioProcessor (NEW)

**Responsibility:** Core DSP processing, parameter management, state persistence

**Reuses from existing suite:**
- `juce::AudioProcessorValueTreeState` parameter pattern (from all Ouaricon plugins)
- `OuariconPresetManager` for preset system (from OuariconComp, OuariconPolystutter)
- Atomic metering pattern for UI updates (from OuariconSaturationModeling, OuariconComp)

**New DSP components:**
- Crossover filter (Linkwitz-Riley 2nd order)
- Full-wave integrator (for Clean mode harmonic generation)
- Bandpass filter bank (for harmonic shaping)
- Saturation models (can reference OuariconSaturationModeling but simplified)

```cpp
// Key members following existing patterns
class OBassAudioProcessor : public juce::AudioProcessor
{
public:
    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;

    // Metering (atomic for thread-safe UI access)
    std::atomic<float> inputLevelDB { -60.0f };
    std::atomic<float> outputLevelDB { -60.0f };
    std::atomic<float> enhancementLevel { 0.0f };  // Visual feedback

private:
    // DSP Components
    juce::dsp::ProcessSpec spec;

    // Crossover (splits at configurable frequency, default ~80Hz)
    using IIRFilter = juce::dsp::IIR::Filter<float>;
    juce::dsp::ProcessorDuplicator<IIRFilter, juce::dsp::IIR::Coefficients<float>> lowpassFilter;
    juce::dsp::ProcessorDuplicator<IIRFilter, juce::dsp::IIR::Coefficients<float>> highpassFilter;

    // Clean mode: harmonic generator
    std::vector<float> integratorState;  // Per-channel full-wave integrator
    juce::dsp::ProcessorDuplicator<IIRFilter, juce::dsp::IIR::Coefficients<float>> harmonicBandpass;

    // Colored mode: saturation (simplified from OuariconSaturationModeling)
    juce::dsp::WaveShaper<float> tubeSaturation;
    juce::dsp::ProcessorDuplicator<IIRFilter, juce::dsp::IIR::Coefficients<float>> warmthFilter;

    // Output
    juce::dsp::Gain<float> enhancementGain;
    juce::dsp::Gain<float> outputGain;
};
```

### 2. OBassAudioProcessorEditor (NEW)

**Responsibility:** WebView UI hosting, parameter relay management

**Reuses from existing suite:**
- `juce::WebBrowserComponent` with native integration (from all recent Ouaricon plugins)
- `juce::WebSliderRelay` / `juce::WebSliderParameterAttachment` pattern
- `juce::WebToggleButtonRelay` for mode switch
- Resource provider pattern for embedded HTML/CSS/JS
- Timer-based metering updates (30Hz refresh)

```cpp
class OBassAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
private:
    // Relays (created BEFORE WebView)
    std::unique_ptr<juce::WebSliderRelay> amountRelay;
    std::unique_ptr<juce::WebSliderRelay> frequencyRelay;
    std::unique_ptr<juce::WebSliderRelay> outputRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> modeRelay;  // Clean/Colored toggle

    // WebView (created with relay options)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Attachments (created AFTER WebView)
    std::unique_ptr<juce::WebSliderParameterAttachment> amountAttachment;
    // ... etc

    // Timer callback sends metering data to WebView
    void timerCallback() override;
};
```

### 3. DSP Modules

#### 3a. CrossoverFilter (utility class)

**Responsibility:** Split signal into bass (below crossover) and rest (above crossover)

```cpp
class CrossoverFilter
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setFrequency(float hz);

    // Returns lowpass-filtered signal, stores highpass in separate buffer
    void process(juce::AudioBuffer<float>& input,
                 juce::AudioBuffer<float>& lowOutput,
                 juce::AudioBuffer<float>& highOutput);

private:
    // Linkwitz-Riley 2nd order (two cascaded Butterworth)
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> lpf1, lpf2;
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> hpf1, hpf2;
};
```

#### 3b. HarmonicSynthesizer (Clean Mode)

**Responsibility:** Generate psychoacoustic harmonics for perceived bass enhancement

Based on the verified MATLAB algorithm (MathWorks documentation):
1. Full-wave integrate the bass signal (generates harmonics at integer multiples)
2. Bandpass filter to select useful harmonic range (crossover to ~250Hz)
3. Apply gain controlled by Amount parameter

```cpp
class HarmonicSynthesizer
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setLowerCutoff(float hz);  // Matches crossover
    void setUpperCutoff(float hz);  // User-adjustable or fixed at ~250Hz

    void process(juce::AudioBuffer<float>& bass);

private:
    // Full-wave integrator state per channel
    std::vector<float> integratorOutput;
    std::vector<float> prevInput;

    // Bandpass to shape harmonics
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> bandpass;
};
```

**Algorithm (per sample):**
```cpp
// Full-wave integrator (from MathWorks verified algorithm)
if (input[n] > 0.0f && prevInput[channel] <= 0.0f)
{
    // Zero crossing detected - reset integrator
    integratorOutput[channel] = 0.0f;
}
else
{
    integratorOutput[channel] += prevInput[channel];
}
prevInput[channel] = input[n];
output[n] = integratorOutput[channel];
```

#### 3c. BassSaturator (Colored Mode)

**Responsibility:** Add analog warmth with tube/transformer characteristics

Simplified from OuariconSaturationModeling - uses single saturation curve optimized for bass:

```cpp
class BassSaturator
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setDrive(float normalizedDrive);  // 0-1 from Amount knob

    void process(juce::AudioBuffer<float>& bass);

private:
    juce::dsp::WaveShaper<float> saturation;
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> warmthFilter;  // Subtle LF boost

    float currentDrive = 0.5f;
};
```

**Saturation transfer function (tube-like for bass):**
```cpp
// Asymmetric soft clipping with even harmonics
saturation.functionToUse = [this](float x) {
    float driven = x * (1.0f + currentDrive * 3.0f);
    if (driven >= 0.0f)
        return driven / (1.0f + std::abs(driven));
    else
        return std::tanh(driven * 1.2f) / 1.2f;
};
```

### 4. Parameter Layout

Following OuariconSaturationModeling/OuariconComp patterns:

| Parameter ID | Type | Range | Default | Purpose |
|--------------|------|-------|---------|---------|
| `AMOUNT` | Float | 0-100% | 50% | Enhancement intensity |
| `FREQUENCY` | Float | 40-120 Hz | 80 Hz | Crossover frequency |
| `OUTPUT` | Float | -12 to +12 dB | 0 dB | Output gain |
| `MODE` | Bool | false/true | false | Clean (false) / Colored (true) |

**Optional 5th control (if needed):**
| `WARMTH` | Float | 0-100% | 50% | Colored mode: saturation amount |

### 5. WebView UI Integration

Following the established Ouaricon pattern:

```
plugins/OBass/
  Source/
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp
    ui/
      public/
        index.html          # Single-file UI (CSS+JS inline)
        js/juce/
          index.js          # JUCE WebView bridge (from JUCE examples)
          check_native_interop.js
        img/
          paper1.jpg        # Ouaricon paper texture background
          [botanical-illustration].png  # Thematic artwork
```

## Integration Points

### Existing Modules to Reuse

| Module | Integration | Notes |
|--------|-------------|-------|
| `OuariconPresetManager` | Copy to `Source/` or create shared module | Header-only, minimal adaptation needed |
| WebView relay pattern | Copy JUCE bridge JS files | Standard across all Ouaricon WebView plugins |
| CMakeLists.txt pattern | Template from OuariconSaturationModeling | Just update plugin name and source files |
| Paper texture assets | Copy or reference shared resources | Maintain visual consistency |

### New Components Required

| Component | Type | Complexity | Dependencies |
|-----------|------|------------|--------------|
| `OBassAudioProcessor` | Core class | Medium | JUCE DSP, OuariconPresetManager |
| `OBassAudioProcessorEditor` | UI host | Low | WebBrowserComponent, WebSliderRelay |
| `HarmonicSynthesizer` | DSP module | Medium | JUCE IIR filters |
| `BassSaturator` | DSP module | Low | JUCE WaveShaper (reference existing) |
| `CrossoverFilter` | DSP utility | Low | JUCE IIR filters |
| `index.html` | WebView UI | Medium | JS/CSS (follow existing patterns) |

## Build Order (Suggested Phases)

### Phase 1: Scaffold
1. Create plugin directory structure (copy from OuariconSaturationModeling template)
2. Set up CMakeLists.txt with correct plugin name/codes
3. Create stub PluginProcessor/PluginEditor that compiles
4. Verify builds for VST3/AU/Standalone

### Phase 2: Core DSP
1. Implement parameter layout (AMOUNT, FREQUENCY, OUTPUT, MODE)
2. Implement crossover filter
3. Implement Clean mode (HarmonicSynthesizer)
4. Implement Colored mode (BassSaturator)
5. Wire up mode switching
6. Add metering (input/output levels)

### Phase 3: UI
1. Create basic HTML structure matching Ouaricon visual language
2. Implement JUCE↔WebView parameter binding
3. Add visual feedback (metering, mode indicator)
4. Style controls (knobs, toggle, meters)

### Phase 4: Polish
1. Integrate OuariconPresetManager
2. Create factory presets
3. Performance optimization (if needed)
4. Testing across formats (VST3, AU, Standalone)

## Anti-Patterns to Avoid

### 1. Allocations in processBlock
**What:** Creating new buffers or objects during audio processing
**Why bad:** Causes audio dropouts, especially at low buffer sizes
**Instead:** Pre-allocate all buffers in `prepareToPlay()`

### 2. Direct parameter access without atomic
**What:** Reading parameters without atomic load
**Why bad:** Data races between audio and UI threads
**Instead:** Use `getRawParameterValue()->load()` pattern from existing plugins

### 3. Overcomplicating the saturation
**What:** Porting full OuariconSaturationModeling complexity
**Why bad:** O-Bass needs ONE saturation character, not four models
**Instead:** Extract/simplify the tube model only

### 4. Multiband in v1
**What:** User-adjustable multiple crossover bands
**Why bad:** Violates "3-5 controls" requirement, adds complexity
**Instead:** Single crossover frequency, possibly with FREQUENCY knob

## Performance Considerations

| Concern | At 44.1kHz | At 96kHz | At 192kHz |
|---------|------------|----------|-----------|
| Filter updates | Real-time safe | Real-time safe | Real-time safe |
| Saturation | ~1% CPU | ~2% CPU | ~4% CPU |
| Harmonic synthesis | ~1% CPU | ~2% CPU | ~4% CPU |
| Total expected | <5% CPU | <10% CPU | <15% CPU |

**Mix bus suitability:** Yes - no feedback loops, minimal latency (only filter group delay), no FFT processing.

## Sources

- [MathWorks: Psychoacoustic Bass Enhancement](https://www.mathworks.com/help/audio/ug/psychoacoustic-bass-enhancement-for-band-limited-signals.html) - Verified algorithm for harmonic synthesis
- Existing Ouaricon codebase analysis (OuariconSaturationModeling, OuariconAnalogEQ, OuariconComp)
- [JUCE DSP Waveshaping Tutorial](https://docs.juce.com/master/tutorial_dsp_convolution.html) - WaveShaper patterns
- [Waves Bass Plugins Comparison](https://www.waves.com/bass-plugins-and-sub-enhancers-compared) - MaxxBass/RBass are harmonic enhancers (not subharmonic generators)
- [dbx Subharmonic Synthesizer](https://en.wikipedia.org/wiki/Subharmonic_synthesizer) - Reference for true subharmonic generation (NOT used in O-Bass v1)
