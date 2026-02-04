# O-FreqPulse - Parameter Specification

**Contract Status:** COMPLETE
**Generated:** 2026-02-03
**Total Parameters:** 165 (5 global + 32 band + 128 step)

---

## Global Parameters (5)

| Parameter ID | Display Name | Type | Range | Default | Skew | Unit | Notes |
|-------------|--------------|------|-------|---------|------|------|-------|
| `mix` | Mix | Float | 0.0 - 1.0 | 1.0 | 1.0 | % | Dry/wet blend |
| `steps` | Steps | Choice | 4, 8, 16, 32 | 16 | N/A | - | Sequence length |
| `rate` | Rate | Choice | 0-9 | 4 | N/A | - | Tempo sync rate (see enum) |
| `swing` | Swing | Float | 0.0 - 1.0 | 0.0 | 1.0 | % | Timing swing |
| `smoothing` | Smoothing | Float | 0.0 - 100.0 | 5.0 | 1.0 | ms | Gate transition time |

### Rate Enum Values
```
0 = "1/1"   (whole note, 4.0 PPQ)
1 = "1/2"   (half note, 2.0 PPQ)
2 = "1/4"   (quarter note, 1.0 PPQ)
3 = "1/8"   (0.5 PPQ)
4 = "1/16"  (0.25 PPQ) - DEFAULT
5 = "1/32"  (0.125 PPQ)
6 = "1/8T"  (triplet, 0.333 PPQ)
7 = "1/16T" (triplet, 0.166 PPQ)
8 = "1/4D"  (dotted, 1.5 PPQ)
9 = "1/8D"  (dotted, 0.75 PPQ)
```

### Steps Choice Values
```
0 = 4 steps
1 = 8 steps
2 = 16 steps (DEFAULT)
3 = 32 steps
```

---

## Per-Band Parameters (8 × 4 bands = 32)

| Parameter ID Pattern | Display Name | Type | Range | Default | Skew | Unit |
|---------------------|--------------|------|-------|---------|------|------|
| `band{N}_enable` | Band {N+1} Enable | Bool | 0/1 | 1 (On) | N/A | - |
| `band{N}_low` | Band {N+1} Low | Float | 20.0 - 20000.0 | varies | 0.3 | Hz |
| `band{N}_high` | Band {N+1} High | Float | 20.0 - 20000.0 | varies | 0.3 | Hz |
| `band{N}_depth` | Band {N+1} Depth | Float | 0.0 - 1.0 | 1.0 | 1.0 | % |
| `band{N}_euc_on` | Band {N+1} Euclidean | Bool | 0/1 | 0 (Off) | N/A | - |
| `band{N}_euc_steps` | Band {N+1} Euc Steps | Int | 1 - 32 | 16 | N/A | - |
| `band{N}_euc_pulses` | Band {N+1} Euc Pulses | Int | 1 - 32 | 8 | N/A | - |
| `band{N}_euc_offset` | Band {N+1} Euc Offset | Int | 0 - 31 | 0 | N/A | - |

**{N}** = 0, 1, 2, 3 (band indices)

### Default Band Frequencies

| Band Index | Name | Low Default | High Default |
|------------|------|-------------|--------------|
| 0 | Sub | 20 Hz | 120 Hz |
| 1 | Low | 120 Hz | 500 Hz |
| 2 | Mid | 500 Hz | 4000 Hz |
| 3 | High | 4000 Hz | 20000 Hz |

---

## Step Grid Parameters (32 × 4 bands = 128)

| Parameter ID Pattern | Display Name | Type | Range | Default |
|---------------------|--------------|------|-------|---------|
| `step_b{N}_s{M}` | B{N+1} Step {M+1} | Bool | 0/1 | 0 (Off) |

**{N}** = 0, 1, 2, 3 (band index)
**{M}** = 0-31 (step index)

### Examples
- `step_b0_s0` = Band 1, Step 1
- `step_b0_s15` = Band 1, Step 16
- `step_b3_s31` = Band 4, Step 32

---

## APVTS Parameter Groups

```cpp
// Group structure for APVTS
APVTS Parameters:
├── Global
│   ├── mix
│   ├── steps
│   ├── rate
│   ├── swing
│   └── smoothing
├── Band 0 (Sub)
│   ├── band0_enable
│   ├── band0_low
│   ├── band0_high
│   ├── band0_depth
│   ├── band0_euc_on
│   ├── band0_euc_steps
│   ├── band0_euc_pulses
│   └── band0_euc_offset
├── Band 1 (Low)
│   └── ... (same pattern)
├── Band 2 (Mid)
│   └── ... (same pattern)
├── Band 3 (High)
│   └── ... (same pattern)
└── Step Grid
    ├── step_b0_s0 ... step_b0_s31
    ├── step_b1_s0 ... step_b1_s31
    ├── step_b2_s0 ... step_b2_s31
    └── step_b3_s0 ... step_b3_s31
```

---

## Parameter Creation Code Reference

```cpp
// Global parameters
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "mix", "Mix",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

layout.add(std::make_unique<juce::AudioParameterChoice>(
    "steps", "Steps",
    juce::StringArray{"4", "8", "16", "32"}, 2));  // Default index 2 = "16"

layout.add(std::make_unique<juce::AudioParameterChoice>(
    "rate", "Rate",
    juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T", "1/4D", "1/8D"}, 4));

layout.add(std::make_unique<juce::AudioParameterFloat>(
    "swing", "Swing",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

layout.add(std::make_unique<juce::AudioParameterFloat>(
    "smoothing", "Smoothing",
    juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 5.0f));

// Per-band parameters (loop N = 0..3)
for (int n = 0; n < 4; ++n) {
    juce::String prefix = "band" + juce::String(n);

    layout.add(std::make_unique<juce::AudioParameterBool>(
        prefix + "_enable", "Band " + juce::String(n+1) + " Enable", true));

    // Frequency params with log skew
    float lowDefault = (n == 0) ? 20.0f : (n == 1) ? 120.0f : (n == 2) ? 500.0f : 4000.0f;
    float highDefault = (n == 0) ? 120.0f : (n == 1) ? 500.0f : (n == 2) ? 4000.0f : 20000.0f;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        prefix + "_low", "Band " + juce::String(n+1) + " Low",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), lowDefault));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        prefix + "_high", "Band " + juce::String(n+1) + " High",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), highDefault));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        prefix + "_depth", "Band " + juce::String(n+1) + " Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        prefix + "_euc_on", "Band " + juce::String(n+1) + " Euclidean", false));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        prefix + "_euc_steps", "Band " + juce::String(n+1) + " Euc Steps", 1, 32, 16));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        prefix + "_euc_pulses", "Band " + juce::String(n+1) + " Euc Pulses", 1, 32, 8));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        prefix + "_euc_offset", "Band " + juce::String(n+1) + " Euc Offset", 0, 31, 0));
}

// Step grid parameters (loop N = 0..3, M = 0..31)
for (int n = 0; n < 4; ++n) {
    for (int m = 0; m < 32; ++m) {
        juce::String id = "step_b" + juce::String(n) + "_s" + juce::String(m);
        juce::String name = "B" + juce::String(n+1) + " Step " + juce::String(m+1);
        layout.add(std::make_unique<juce::AudioParameterBool>(id, name, false));
    }
}
```

---

## Validation Rules

1. **Band Frequency Order:** `band{N}_low` must be < `band{N}_high`
2. **Euclidean Pulses:** `band{N}_euc_pulses` must be ≤ `band{N}_euc_steps`
3. **Step Visibility:** Only steps 0 to (steps-1) are active based on global `steps` parameter

---

**Status:** PARAMETER SPEC COMPLETE - Ready for Stage 1 Implementation
