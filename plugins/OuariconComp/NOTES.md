# OuariconComp Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.1
- **Type:** Audio Effect (Compressor)

## Lifecycle Timeline

- **2026-01-11 (v1.0.1):** Bug fixes - fixed knob animations, real metering for input/output and envelope/GR display
- **2026-01-11:** Installed to system folders (VST3 + AU)
- **2026-01-11 (Stage 3):** GUI integration complete - WebView UI with v8 mockup
- **2026-01-11 (Stage 2):** DSP implementation complete - Custom compressor engine
- **2026-01-11 (Stage 1):** Foundation + Shell complete - 7 APVTS parameters
- **2026-01-11 (Stage 0):** Research and planning complete

## Known Issues

- None

## Additional Notes

**Description:**
Transparent, clean, utilitarian compressor with variable soft-knee. Feed-forward design for predictability. Zero-latency (no lookahead buffer).

**Parameters (7 total):**
1. Threshold (-60 to 0 dB, default -20 dB)
2. Ratio (1:1 to 20:1, default 4:1)
3. Attack (0.1 to 100 ms, default 10 ms)
4. Release (10 to 1000 ms, default 100 ms)
5. Knee (0 to 20 dB, default 6 dB)
6. Output Gain (-12 to +24 dB, default 0 dB)
7. Auto-Gain (On/Off, default Off)

**DSP Features:**
- Custom compressor engine with variable soft-knee (0-20 dB)
- Peak envelope follower with attack/release ballistics
- Stereo-linked detection (max of L/R channels)
- Auto-gain calculation: `makeupGain_dB = -threshold * (1 - 1/ratio)`
- Real-time safe parameter reads via atomic loads
- Per-sample processing for accurate envelope tracking

**GUI Features:**
- WebView UI with Ouaricon Naturalist aesthetic
- 6 seed knobs + 1 toggle button
- Transfer curve visualization
- LED meters for input/output levels
- Envelope and gain reduction display

**Installation Locations:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/OuariconComp.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/OuariconComp.component`

**Formats:** VST3, AU, Standalone
