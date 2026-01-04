# FlutterVerb Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.3
- **Type:** Audio Effect (Reverb)
- **Complexity:** 5.0 (Complex)

## Lifecycle Timeline

- **2025-11-11:** Creative brief completed
- **2025-11-11:** UI mockup v6 finalized (600×640px, TapeAge-inspired design)
- **2025-11-11:** design-sync validation passed (no drift detected)
- **2025-11-11:** Implementation files generated (ready for `/implement FlutterVerb`)
- **2025-11-11 (Stage 0):** Research completed - DSP architecture documented
- **2025-11-11 (Stage 1):** Planning - Complexity 5.0 (phased implementation)
- **2025-11-11 (Stage 2):** Foundation complete - build system operational
- **2025-11-11 (Stage 3):** Shell complete - 7 parameters implemented
- **2025-11-11 (Stage 4.1):** Core reverb complete - juce::dsp::Reverb + DryWetMixer operational (SIZE, DECAY, MIX parameters)
- **2025-11-11 (Stage 4.2):** Modulation system complete - dual LFO (wow + flutter) with delay-based pitch shifting (AGE parameter)
- **2025-11-11 (Stage 4.3):** Saturation and filter complete - tape warmth via tanh() + DJ-style IIR filter (DRIVE, TONE parameters)
- **2025-11-11 (Stage 4.4):** MOD_MODE routing complete - wet-only vs wet+dry modulation routing
- **2025-11-11 (Stage 5.1):** GUI layout complete - WebView infrastructure with 7 relays + attachments
- **2025-11-11 (Stage 5.2):** Parameter binding complete - all 7 parameters bound to UI controls (Pattern #15, #16)
- **2025-11-11 (Stage 5.3):** VU meter complete - real-time output level display with ballistics (fast attack, slow release)
- **2025-11-11 (Stage 6):** Validation complete - 7 factory presets created, CHANGELOG.md generated, ready for installation
- **2025-11-11 (v1.0.0):** Installed to system folders (VST3 + AU)
- **2025-11-11 (v1.0.1):** Bug fixes - decay independence, drive latency compensation, age scaling, MOD_MODE toggle, VU meter
- **2025-11-11 (v1.0.2):** Bug fixes - VU meter animation, MOD_MODE toggle UI, MOD_MODE functionality, SIZE/DECAY independence
- **2025-11-12 (v1.0.3):** Fixed knob hover shake - replaced scale transform with border color change

## Known Issues

None

## Additional Notes

**Description:**
Tape-driven plate reverb with extreme wow and flutter modulation for textured, analog-sounding spaces. Combines modern algorithmic clarity with vintage tape character.

**Parameters (7 total):**
- SIZE: 0-100%, default 50% (room dimensions)
- DECAY: 0.1s-10s, default 2.5s (reverb tail length)
- MIX: 0-100%, default 25% (dry/wet blend)
- AGE: 0-100%, default 20% (tape character intensity - combines wow and flutter modulation depth)
- DRIVE: 0-100%, default 20% (tape saturation/warmth)
- TONE: -100% to +100%, default 0% (DJ-style filter: negative=low-pass, positive=high-pass, 0%=bypass)
- MOD_MODE: Toggle (Wet Only / Wet + Dry) - applies modulation to dry signal when enabled

**DSP:** Plate reverb algorithm with size-controlled early reflections. Dual LFO wow/flutter modulation (different rates) controlled by Age parameter. Soft-clipping tape saturation with adjustable drive. DJ-style exponential filter (same implementation as GainKnob). User-selectable modulation routing (wet-only or wet+dry).

**GUI:** WebView UI (600×640px). TapeAge-inspired dark radial gradient with brass accents. VU meter at top (output peak with ballistics), two rows of knobs (Size/Decay/Mix, Age/Drive/Tone), horizontal toggle below Tone knob. Typewriter/monospace typography. All 7 parameters bound with bidirectional sync (UI ↔ DSP).

**Features:**
- Dual LFO modulation (wow: 1Hz, flutter: 6Hz)
- Lagrange 3rd-order interpolation for smooth pitch modulation
- Exponential filter cutoff mapping for musical response
- VU meter ballistics (fast attack, slow release)
- Bypass zone for center filter position (transparent)
- State reset on filter type transitions (no burst artifacts)
- 7 factory presets showcasing different use cases

**Validation:**
- ✓ Factory presets: 7 presets created (Default, Small Room, Large Hall, Tape Warble, Dark Ambient, Bright Plate, Lo-Fi Tape)
- ✓ CHANGELOG.md: Generated in Keep a Changelog format
- ✓ Manual testing: Plugin loads and processes audio correctly
- ✓ All parameters functional with proper ranges
- ✓ VU meter animates correctly
- ⚠ pluginval: Skipped (not installed on system)

**Formats:** VST3, AU, Standalone

**Installation Locations:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/FlutterVerb.vst3` (4.2 MB)
- AU: `~/Library/Audio/Plug-Ins/Components/FlutterVerb.component` (4.1 MB)

**UI Mockup:** v6 (finalized 2025-11-11)
- Design validated against creative brief (no drift)
- 7 implementation files generated (HTML, C++ header/impl, CMake, checklist)
- Fully integrated in Stage 5 (WebView + parameter binding + VU meter)

**Use Cases:**
- Textured lead vocals with evolving, warbling space
- Piano and keys with lush, moving reverb tails
- Guitar ambience with vintage spring-like wobble
- Mix bus texture for cohesive analog glue
- Creative sound design with extreme pitch-shifted textures
