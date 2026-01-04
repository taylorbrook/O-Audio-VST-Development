# Ouaricon Tremolo - Parameter Specification

## Audio Parameters

### 1. Tremolo Speed
- **Parameter ID**: `SPEED_PARAM`
- **Display Name**: "Speed"
- **Type**: Float
- **Range**: 0.1 Hz - 20.0 Hz
- **Default**: 4.5 Hz
- **Unit**: "Hz"
- **Decimal Places**: 1
- **Description**: Controls the rate of amplitude modulation
- **Behavior**:
  - When Tempo Sync is OFF: Free-running LFO at specified Hz
  - When Tempo Sync is ON: Locked to host tempo (note divisions)
- **Automation**: Yes

### 2. Tremolo Depth
- **Parameter ID**: `DEPTH_PARAM`
- **Display Name**: "Depth"
- **Type**: Float
- **Range**: 0% - 100%
- **Default**: 75%
- **Unit**: "%"
- **Decimal Places**: 1
- **Description**: Controls the intensity of amplitude modulation
- **Behavior**:
  - 0% = no modulation (unity gain)
  - 100% = full modulation (down to silence)
  - Linear scaling of modulation depth
- **Automation**: Yes

### 3. Waveform Type
- **Parameter ID**: `WAVEFORM_PARAM`
- **Display Name**: "Waveform"
- **Type**: Choice (Integer 0-5)
- **Options**:
  - 0: "Sine" (smooth, classic tremolo)
  - 1: "Triangle" (linear ramps, slightly brighter)
  - 2: "Phasor" (sawtooth, asymmetric modulation)
  - 3: "Noise" (random fluctuation, textural)
  - 4: "Square" (hard on/off gating)
  - 5: "Pulse" (short bursts, rhythmic stutter)
- **Default**: 0 (Sine)
- **Description**: Selects the modulation waveform shape
- **Automation**: Yes (stepped parameter)

### 4. Waveform Smoothing
- **Parameter ID**: `SMOOTHING_PARAM`
- **Display Name**: "Smoothing"
- **Type**: Float
- **Range**: 0% - 100%
- **Default**: 30%
- **Unit**: "%"
- **Decimal Places**: 1
- **Description**: Controls curve softness via linear interpolation between samples
- **Behavior**:
  - 0% = sharp transitions (raw waveform)
  - 100% = very smooth (heavy interpolation)
  - Applies to all waveform types
  - Particularly useful for softening square/pulse edges
- **Automation**: Yes

### 5. Pan Sync
- **Parameter ID**: `PAN_SYNC_PARAM`
- **Display Name**: "Pan Sync"
- **Type**: Boolean
- **Default**: false (OFF)
- **Description**: Enables stereo panning modulation synchronized with amplitude
- **Behavior**:
  - OFF: Mono tremolo (both channels modulated identically)
  - ON: Stereo tremolo (left/right channels modulated 180° out of phase)
  - Creates auto-pan effect when enabled
- **Automation**: Yes

### 6. Tempo Sync
- **Parameter ID**: `TEMPO_SYNC_PARAM`
- **Display Name**: "Tempo Sync"
- **Type**: Boolean
- **Default**: false (OFF)
- **Description**: Locks tremolo speed to host DAW tempo
- **Behavior**:
  - OFF: Speed parameter operates in Hz (free-running)
  - ON: Speed parameter snaps to musical divisions (1/4, 1/8, 1/16, etc.)
  - Follows host transport tempo changes
- **Automation**: Yes

## Parameter Relationships

### Tempo Sync Mode
When `TEMPO_SYNC_PARAM` is enabled:
- Speed range changes from Hz to note divisions
- Suggested divisions: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 (whole to thirty-second notes)
- May also support dotted and triplet variations
- Display changes from "Hz" to note value (e.g., "1/4")

### Pan Sync Interaction
When `PAN_SYNC_PARAM` is enabled:
- Creates stereo width modulation
- Left channel = LFO at 0° phase
- Right channel = LFO at 180° phase (inverted)
- Combined with depth creates auto-pan tremolo effect

### Smoothing Impact
- Most audible on square, pulse, and noise waveforms
- Minimal impact on sine (already smooth)
- Moderate impact on triangle and phasor
- Higher values = more CPU usage (more interpolation)

## Preset System

### Factory Presets (Suggested)
1. **Default**: Sine, 4.5 Hz, 75% depth, 30% smoothing
2. **Classic Tremolo**: Sine, 6.0 Hz, 80% depth, 0% smoothing
3. **Vintage Vibrato**: Triangle, 5.5 Hz, 60% depth, 20% smoothing
4. **Hard Gate**: Square, 8.0 Hz, 100% depth, 0% smoothing
5. **Rhythmic Pulse**: Pulse, 4.0 Hz, 90% depth, 10% smoothing
6. **Stereo Swirl**: Sine, 3.0 Hz, 70% depth, 40% smoothing, Pan Sync ON
7. **Textural Noise**: Noise, 2.5 Hz, 50% depth, 60% smoothing
8. **Tempo Locked 1/8**: Sine, 1/8 note, 75% depth, Tempo Sync ON

### Preset Compatibility
- All presets save/load via JUCE state system
- Compatible with DAW automation recall
- Host-agnostic (works across different DAWs)

## UI Mapping

### JUCE WebView Relay Names
All parameters use WebView relays for bidirectional communication between JavaScript and C++:

- **Speed** → `WebSliderRelay` with name `"speed"`
- **Depth** → `WebSliderRelay` with name `"depth"`
- **Waveform** → `WebComboBoxRelay` with name `"waveform"`
- **Smoothing** → `WebSliderRelay` with name `"smoothing"`
- **Pan Sync** → `WebToggleButtonRelay` with name `"panSync"`
- **Tempo Sync** → `WebToggleButtonRelay` with name `"tempoSync"`

### UI Controls
- Speed knob → `getSliderState('speed')` (left panel, top)
- Depth knob → `getSliderState('depth')` (left panel, bottom)
- "Pan Sync" button → `getToggleState('panSync')` (left panel, top-left)
- "Tempo Sync" button → `getToggleState('tempoSync')` (left panel, top-right)
- "Waveform" dropdown → `getComboBoxState('waveform')` (right panel, top)
- "Smoothing" slider → `getSliderState('smoothing')` (right panel, bottom)

### Visual Feedback
- Waveform visualizer displays current `WAVEFORM_PARAM` shape as modified by `SMOOTHING_PARAM`
- Updates in real-time as parameters change
- Grid lines provide visual reference for amplitude
- All controls update bidirectionally (UI ↔ JUCE automation)

## Technical Implementation Notes

### JUCE APVTS
All parameters managed via `juce::AudioProcessorValueTreeState`:
- Automatic DAW automation support
- Thread-safe parameter access
- Undo/redo integration
- Preset system built-in

### WebView Relay Integration
Each UI control uses JUCE WebView relays for parameter binding:
- `WebSliderRelay` for knobs ("speed", "depth") and slider ("smoothing")
- `WebToggleButtonRelay` for toggle buttons ("panSync", "tempoSync")
- `WebComboBoxRelay` for waveform selector ("waveform")

JavaScript interop uses:
- `getSliderState(name)` → returns SliderState with `getNormalisedValue()`, `setNormalisedValue()`, `sliderDragStarted()`, `sliderDragEnded()`
- `getToggleState(name)` → returns ToggleState with `getValue()`, `setValue()`
- `getComboBoxState(name)` → returns ComboBoxState with `getChoiceIndex()`, `setChoiceIndex()`

### Value Formatting
- Hz values: 1 decimal place (e.g., "4.5 Hz")
- Percentage values: 1 decimal place (e.g., "75.0%")
- Waveform: Display option name (e.g., "Sine")
- Note divisions (when tempo synced): Fractional notation (e.g., "1/8")
