# Ouaricon Saturation Modeling Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.0
- **Type:** Audio Effect (Saturation)

## Lifecycle Timeline

- **2026-01-09:** Creative brief and UI mockup created
- **2026-01-09 (Stage 1-3):** Full implementation completed
- **2026-01-09 (v1.0.0):** First release - VST3 and AU installed

## Features

### Saturation Models
1. **MAGNETIC** - Jiles-Atherton tape hysteresis simulation
2. **TUBE** - Asymmetric soft saturation (12AX7 character)
3. **TRANSFORMER** - Core saturation with 60Hz LF bump and 8kHz HF sheen
4. **DIODE** - Symmetric soft clipping waveshaper

### Controls
- **Intensity** (0-100%) - Dry/wet mix with drive scaling
- **Model** - Select saturation type (4 options)
- **Quality** - Oversampling: LOW (1x), MID (2x), HIGH (4x)
- **Autogain** - Automatic output level compensation

### UI
- Vintage botanical WebView interface
- Paper texture background
- Snake illustration (changes per model, opacity tied to intensity)
- Dual VU meters with green-to-red gradient needles

## Known Issues

None

## Installation Locations

- **AU:** `~/Library/Audio/Plug-Ins/Components/Ouaricon Saturation Modeling.component`
- **VST3:** `~/Library/Audio/Plug-Ins/VST3/Ouaricon Saturation Modeling.vst3`

## Git Tag

`v1.0.0-OuariconSaturationModeling`
