# Stage 3: GUI — Context

**Plugin:** O-Gain
**Stage:** 3 (GUI Implementation)
**Mode:** Express (auto-generated)

## What This Stage Must Produce

1. **PluginEditor.h/.cpp** — WebView-based editor with parameter bindings and metering
2. **Source/ui/public/index.html** — Single-file HTML/CSS/JS UI
3. **Source/ui/public/js/juce/index.js** — JUCE WebView interop script
4. **Source/ui/public/js/juce/check_native_interop.js** — Native interop check
5. **CMakeLists.txt update** — Add BinaryData for UI assets

## UI Layout Vision

Compact, professional interface for mixer placement (350x500px):

```
┌─────────────────────────────────┐
│         O-GAIN                  │
│                                 │
│  [INPUT]              [OUTPUT]  │
│  ┃ ┃                    ┃ ┃    │
│  ┃ ┃    ┌──────────┐    ┃ ┃    │
│  ┃ ┃    │   GAIN   │    ┃ ┃    │
│  ┃ ┃    │  +0.0 dB │    ┃ ┃    │
│  ┃ ┃    └──────────┘    ┃ ┃    │
│  ┃ ┃                    ┃ ┃    │
│  ┃ ┃    [TRIM: 0.0]    ┃ ┃    │
│  ┃ ┃                    ┃ ┃    │
│  ┗ ┛                    ┗ ┛    │
│                                 │
│  [LEARN]  Target: -18 dB       │
│  Mode: LUFS  Meter: VU         │
│                                 │
│  ┌─ DURING LEARN ────────────┐ │
│  │ Momentary: -18.2 LUFS     │ │
│  │ Short-term: -17.8 LUFS    │ │
│  │ Integrated: -18.0 LUFS    │ │
│  │ Elapsed: 12.4s  Conf: HIGH│ │
│  └────────────────────────────┘ │
│                                 │
│  [ΦL] [ΦR] [SWAP] [MONO] [M/S]│
└─────────────────────────────────┘
```

## Parameters Needing Relays

### Float Parameters (WebSliderRelay + WebSliderParameterAttachment)
- `gain_offset` — large central knob (-40 to +40 dB)
- `trim` — small knob (-6 to +6 dB)
- `target_level` — dropdown or knob (-36 to 0 dB)

### Choice Parameters (WebComboBoxRelay + WebComboBoxParameterAttachment)
- `measurement_mode` — LUFS / RMS selector
- `meter_mode` — Peak / RMS / VU / LUFS selector
- `ms_mode` — Off / Encode / Decode selector

### Bool Parameters (WebToggleButtonRelay + WebToggleButtonParameterAttachment)
- `phase_invert_l` — Phase L button
- `phase_invert_r` — Phase R button
- `channel_swap` — Swap button
- `mono_sum` — Mono button

## Native Functions Needed

- `toggleLearn` — Toggle learn mode on/off (sets processor.learnActive)
- `getLearnState` — Get current learn state (0=idle, 1=learning, 2=complete)

## Timer-Based Metering (30Hz)

Send these atomic values to JS via evaluateJavascript:
```javascript
updateMeters({
  inputPeakL, inputPeakR, inputRmsL, inputRmsR,
  outputPeakL, outputPeakR, outputRmsL, outputRmsR,
  vuLevelL, vuLevelR,
  momentaryLUFS, shortTermLUFS, integratedLUFS, truePeakDBTP,
  learnState, learnElapsedSeconds, learnConfidence
})
```

## Reference Pattern

Use O-Comp editor pattern (plugins/O-Comp/Source/PluginEditor.h/.cpp):
- Declaration order: Relays → WebView → Attachments
- Resource provider serves BinaryData
- Timer polls processor atomics → evaluateJavascript
- Native functions for non-parameter interactions

## Atomic Members Available on Processor

```cpp
// Input metering
std::atomic<float> inputPeakL, inputPeakR, inputRmsL, inputRmsR;
// Output metering
std::atomic<float> outputPeakL, outputPeakR, outputRmsL, outputRmsR;
// VU metering
std::atomic<float> vuLevelL, vuLevelR;
// Learn state
std::atomic<bool> learnActive;
std::atomic<int> learnState; // 0=idle, 1=learning, 2=complete
// LUFS metering (during Learn)
std::atomic<float> momentaryLUFS, shortTermLUFS, integratedLUFS, truePeakDBTP;
// Learn results
std::atomic<float> learnElapsedSeconds;
std::atomic<int> learnConfidence; // 0=none, 1=low, 2=medium, 3=high
```

## APVTS Access

```cpp
processorRef.parameters.getParameter("param_id")
```
