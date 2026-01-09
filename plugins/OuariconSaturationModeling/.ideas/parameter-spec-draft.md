# Ouaricon Saturation Modeling - Parameter Specification (Draft)

## Parameter Summary

| ID | Name | Type | Range | Default | Unit | Automatable |
|----|------|------|-------|---------|------|-------------|
| INTENSITY | Intensity | Float | 0.0 - 100.0 | 50.0 | % | Yes |
| MODEL | Model | Choice | 0-3 | 0 | - | Yes |
| QUALITY | Quality | Choice | 0-2 | 1 | - | No |
| AUTOGAIN | Auto Gain | Bool | Off/On | Off | - | Yes |

## Detailed Parameter Definitions

### INTENSITY
- **Purpose**: Controls the amount of saturation applied
- **Range**: 0.0 to 100.0
- **Default**: 50.0
- **Skew**: 1.0 (linear) - model-specific mapping handles perceived linearity
- **Smoothing**: 20ms (prevents zipper noise on automation)
- **Notes**: Model-specific behavior - each algorithm maps this differently

### MODEL
- **Purpose**: Selects the saturation algorithm
- **Options**:
  - 0 = MAGNETIC (Jiles-Atherton tape hysteresis)
  - 1 = TUBE (Triode waveshaping via Koren equations)
  - 2 = TRANSFORMER (Core saturation + resonance)
  - 3 = DIODE (Soft clipping via Newton-Raphson)
- **Default**: 0 (MAGNETIC)
- **Notes**: Switching models resets internal state to prevent artifacts

### QUALITY
- **Purpose**: CPU/quality tradeoff selection
- **Options**:
  - 0 = LOW (no oversampling, lookup tables)
  - 1 = MID (2x oversampling, standard algorithms)
  - 2 = HIGH (4x oversampling, full physical models)
- **Default**: 1 (MID)
- **Not Automatable**: Switching quality may cause brief audio discontinuity
- **Notes**: Affects latency reporting to host

### AUTOGAIN
- **Purpose**: Automatic output level compensation
- **Default**: Off
- **Notes**: RMS-based level matching with ~100ms time constant

## Internal Parameters (Not Exposed to User)

These are derived from MODEL and QUALITY selections:

### Per-Model Parameters (Hardcoded/Preset)

**MAGNETIC Model:**
- Ms (saturation magnetization): 350000
- a (domain wall density): 25.0
- alpha (mean field parameter): 1.6e-3
- k (pinning coefficient): 20.0
- c (reversibility): 0.2
- Head bump frequency: 80 Hz
- Head bump gain: +2.5 dB
- HF rolloff frequency: 12000 Hz

**TUBE Model:**
- mu (amplification factor): 100.0
- Kp (plate coefficient): 600.0
- Ex (plate current exponent): 1.4
- Kg1 (grid coefficient): 1060.0
- Presence frequency: 3000 Hz
- Presence gain: +1.5 dB

**TRANSFORMER Model:**
- Core saturation threshold: 0.8
- Core saturation knee: 0.2
- LF bump frequency: 60 Hz
- LF bump Q: 0.7
- LF bump gain: +2.0 dB
- HF sheen frequency: 8000 Hz
- HF sheen gain: +1.0 dB

**DIODE Model:**
- Is (saturation current): 2.52e-9
- n (ideality factor): 1.752
- Vt (thermal voltage): 0.026
- Newton-Raphson iterations: 4 (LOW), 6 (MID), 8 (HIGH)

## Complexity Assessment

- **Parameter Count**: 4 user-facing parameters
- **DSP Complexity**: HIGH (4 distinct algorithms, oversampling, Newton-Raphson solvers)
- **State Management**: MEDIUM (hysteresis state, filter states, envelope follower)
- **UI Complexity**: LOW (one knob, three button groups)

## APVTS Layout (JUCE)

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Intensity (main control)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"INTENSITY", 1},
        "Intensity",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // Model selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"MODEL", 1},
        "Model",
        juce::StringArray{"Magnetic", "Tube", "Transformer", "Diode"},
        0  // Default to Magnetic
    ));

    // Quality selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"QUALITY", 1},
        "Quality",
        juce::StringArray{"Low", "Mid", "High"},
        1  // Default to Mid
    ));

    // Auto gain toggle
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"AUTOGAIN", 1},
        "Auto Gain",
        false
    ));

    return layout;
}
```
