# Stage 1: Foundation — Context

**Plugin:** O-Gain
**Stage:** 1 (Foundation + Shell)
**Mode:** Express (auto-generated from Stage 0 artifacts)

## What This Stage Must Produce

1. **CMakeLists.txt** — Build system with JUCE plugin target, WebView2 support
2. **PluginProcessor.h/.cpp** — Audio processor with APVTS and all 10 parameters
3. **PluginEditor.h/.cpp** — Editor stub (WebView setup comes in Stage 3)
4. **Plugin builds and passes pluginval** — Clean compile, no crashes

## Parameters (10 total)

| Parameter | ID | Type | Range | Default |
|-----------|-----|------|-------|---------|
| Gain Offset | `gain_offset` | Float | -40 to +40 dB | 0 |
| Trim | `trim` | Float | -6 to +6 dB | 0 |
| Target Level | `target_level` | Float | -36 to 0 | -18 |
| Measurement Mode | `measurement_mode` | Choice (2) | LUFS / RMS | LUFS |
| Meter Mode | `meter_mode` | Choice (4) | Peak / RMS / VU / LUFS | VU |
| Phase Invert L | `phase_invert_l` | Bool | On/Off | Off |
| Phase Invert R | `phase_invert_r` | Bool | On/Off | Off |
| Channel Swap | `channel_swap` | Bool | On/Off | Off |
| Mono Sum | `mono_sum` | Bool | On/Off | Off |
| M/S Mode | `ms_mode` | Choice (3) | Off/Encode/Decode | Off |

## Key Constraints

- Zero latency (report 0 samples)
- Plugin ID: `com.ouaricon.ogain`
- Manufacturer: Ouaricon
- Plugin code: `OGan`
- Format: VST3 + AU (Standalone for testing)
- Needs WebView2 support for future GUI stage
- JUCE modules: juce_audio_processors, juce_audio_basics, juce_dsp, juce_gui_extra

## Signal Chain (for processBlock stub)

Pass-through audio (identity) — DSP implementation comes in Stage 2.

## Sources

- BRIEF.md: `plugins/O-Gain/.planning/BRIEF.md`
- Parameter spec: `plugins/O-Gain/.planning/parameter-spec.md`
- Architecture: `plugins/O-Gain/.planning/research/ARCHITECTURE.md`
- Roadmap: `plugins/O-Gain/.planning/ROADMAP.md`
