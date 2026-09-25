# O-Comp Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.10.2
- **Type:** Audio Effect (Compressor)

## Lifecycle Timeline

- **2026-09-24 (v1.10.2):** zh-Hans sidechain Source caption and tooltip title changed from 来源 to the glossary root 源, which clears the i18n-zh-lint Z5 gate failure.
- **2026-09-14 (v1.10.1):** Four defects in the v1.10.0 sidechain, all found in verify. (1) The External auto-fallback tested the *bus*, not the *signal*. An AU host does not disable an unrouted sidechain — it negotiates the bus, reports it enabled, and hands it silence — so all three bus properties read true in Logic with nothing patched in, the detector read zeros, the envelope parked at −60 dB and the compressor went inert. The verdict now comes from signal presence, latched one-way so a key with real dynamics cannot flap back mid-phrase. (2) SC LPF above Nyquist built a biquad with poles at |z| = 1.32; below a 40 kHz rate anything between Nyquist and the absolute 20 kHz "Off" ceiling diverged and the plugin output digital silence. Both detector filters now clamp to 0.45 × rate. (3) `paramDefaults[id] || 0.5` ate the 0.0 Off default, so double-clicking SC HPF/LPF switched the filter *on* at 198/1984 Hz; the handler now tests the type, not the truthiness. (4) `IIR::Filter<float>` default-constructs first-order, so the first biquad assignment in `processBlock` changed the order and triggered `HeapBlock::malloc()` on the audio thread — `prepareToPlay` now seeds a real biquad, as O-MultiBandCompressor has since its v1.6.0.

  **The v1.10.0 gate for (1) was vacuous, and that is the transferable lesson.** It rendered the key bus *disabled*, which is the VST3 case where `isEnabled()` is already false — never the Logic case. Worse, it drove the plugin at −40 dBFS against a −20 dB default threshold, so the compressor did nothing at all and Internal, a working fallback and a detector reading pure silence all measured −40 dB. The reference was right; the stimulus could not discriminate. Both fallback probes now run at −6 dBFS with a liveness assertion on the reference, and a new probe renders the key bus routed-but-silent. Every new probe was negative-controlled: reverted individually, each fails.
- **2026-07-01 (v1.5.0):** Bundled code-review fixes CR-01 + WR-01/02/03 — soft-knee /0→NaN guard (hit "Parallel Crush" preset), channel-loop OOB cap + `isBusesLayoutSupported` (mono/stereo), smoothed makeup gain (de-zipper), preset Prev/Next wrap fix for imported/deleted presets. See `.planning/REVIEW.md`.
- **2026-03-06 (v1.4.3):** Fixed auto-gain overcompensation (50% makeup-gain scaling)
- **2026-01-24 (v1.2.0):** Renamed from OuariconComp to O-Comp
- **2026-01-12 (v1.1.1):** Added preset dropdown menu
- **2026-01-12 (v1.1.0):** Preset Manager integration, 8 factory presets
- **2026-01-11 (v1.0.2):** Changed default ratio to 2:1, fixed double-click reset for all knobs
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

**Names:**
- Short name: O-Comp (DAW, file system, plugin identifiers)
- Full name: Ouaricon Compressor (UI display)

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
- VST3: `~/Library/Audio/Plug-Ins/VST3/O-Comp.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/O-Comp.component`

**Presets:**
- Location: `~/Library/O-Comp/Presets/`

**Formats:** VST3, AU, Standalone
