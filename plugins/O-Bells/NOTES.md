# O-Bells Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 4.8.0
- **Type:** Synth (Physical Modeling Bells)

## Lifecycle Timeline

- **2026-02-02:** Initial development completed (v1.0.0)
- **2026-02-02 (v1.0.0):** Physical modeling bell synthesizer with 18 parameters, WebView UI, 25 factory presets
- **2026-02-02 (v1.1.0):** Added reverb control for spaciousness (MINOR feature addition)
- **2026-02-02 (v1.1.1):** Fixed output clipping with proper DSP gain staging normalization
- **2026-02-02 (v1.2.0):** Implemented proper multi-stage decay envelope with 4 new parameters
- **2026-02-03 (v1.3.0):** Attack parameter, shimmer quality, material differentiation, bloom fix
- **2026-02-03 (v1.4.0):** Bloom split into Speed + Amount controls (BREAKING - presets need resave)
- **2026-02-03 (v1.4.1):** Expanded bloom speed ranges for more dramatic effects
- **2026-02-03 (v1.5.0):** Bloom Fine Controls - per-band independent speed/amount (6 new params)
- **2026-02-03 (v1.5.1):** Bloom speed readouts now display milliseconds instead of percentages
- **2026-02-03 (v1.5.2):** Attack Amount slider added to UI (was missing), parameter renamed for clarity
- **2026-02-03 (v1.5.3):** Fixed Strike parameter producing no sound at 0%/100% extremes
- **2026-02-03 (v1.5.4):** UI reorganization - Onsets section consolidates strike/transient controls
- **2026-02-03 (v1.6.0):** Complete factory preset redesign - 25 research-informed presets with new names
- **2026-02-03 (v1.6.1):** Brightness parameter range expanded [0.1, 2.0] for wider tonal control
- **2026-02-03 (v2.0.0):** BREAKING - Split brightness into Overtone + Acoustic brightness (31 params)
- **2026-02-03 (v2.1.0):** Air Absorption parameter - time-varying lowpass filter for acoustic realism (32 params)
- **2026-02-03 (v2.2.0):** GUI keyboard in footer panel with Gain slider relocated
- **2026-02-03 (v2.2.1):** Complete factory preset redesign with descriptive names
- **2026-02-03 (v2.3.0):** 16-voice polyphony (increased from 8 voices)
- **2026-02-04 (v2.4.0):** Humanize parameter for per-note organic variation (strike, mallet, decay, attack, inharmonicity)
- **2026-02-05 (v3.1.0):** TrueKeys interval reporting - real-time frequency-based interval display with note names and interval labels (ported from O-Lyrica)
- **2026-02-09 (v3.2.0):** Gain staging overhaul: -6dB synthesis normalization reduction + expanded velocity dynamic range (~18dB wider at low velocities)
- **2026-04-13 (v4.0.0):** Effects tab — Chorus, Delay, EQ, 8-channel FDN Reverb
- **2026-04-26 (v4.1.0):** VST3 Note Expression microtonal support for Dorico
- **2026-07-08 (v4.1.1):** Code-review resolution — 3 critical / 12 warning / 13 info findings from `CODE_REVIEW.md`. Factory presets fixed (were recalling at rails), EQ RT-safety (ArrayCoefficients), FileChooser UAF (SafePointer), preset reset-to-defaults + name sanitization, dead `material` control fixed, all knob readouts migrated to `getScaledValue()`, tuning APVTS↔engine bridge, post-EQ safety limiter, high-SR delay overflow, tail length, per-voice RNG. auval PASS.
- **2026-08-02 (v4.1.5):** Fixed the v4.1.1 CR-02 EQ coefficient update: `std::copy` of 6 RAW ArrayCoefficients values into the 5-slot NORMALISED `IIR::Coefficients` array → EQ unstable to Inf on any gain/freq change (same bug as O-IntonationPad v2.8.4, caught there by Windows CI pluginval fuzz). Now assigns via `Coefficients::operator=(std::array)` — normalises by a0, still allocation-free after prepare(). pluginval strictness-10 clean.
- **2026-08-29 (v4.2.0):** The PAGE speaks French. 122 label entries over 123 keyed elements and 19 keyed accessible names, a gear popover with the language selector, and the C++ language pair persisted as a non-parameter property on the APVTS state tree. The Tuning tab is included: `Resources/ui/js/tuning-panel.js` is a plugin-owned copy 279 lines diverged from the scala-tuning-engine module, so its 34 captions are localized here and the divergence deliberately widens. Fixed two pre-existing ENGLISH defects the keying exposed: "True Keys" wrapped to two lines inside its own button (10px of phantom row height in the tuning panel, present at v4.1.5), and the header's version label read v4.0.0. Twelve geometry pins, each negative-controlled. All French is a machine draft, `reviewed: false`.
- **2026-09-20 (v4.5.1):** Visual polish. The snail plate's two shells are now two independently placed overlays at 3x size and 0.11 opacity (upper shell off the top-right corner, lower shell off the bottom-left); value readouts 9px brown -> 11px black; Save/Load and the Strike/Velocity choice buttons centre their captions. Footer version label caught up from v4.3.2.
- **2026-09-20 (v4.5.2):** Preset-differentiation Step 1. In-repo processor-level render harness (`tests/render-harness/`: `O-Bells-render-test` + `report.py` + `probes.py`, behind `OUARICON_BUILD_TESTS`) reproducing the v4.5.1 baseline within 1 dB; test-only RNG seed hook compiled into the harness target only. No audio change. The brief's held pair-min (4.1) was a one-off draw — re-anchored at the 8-seed mean 3.2.
- **2026-09-20 (v4.6.0):** Preset-differentiation Step 2 — three voice bugs, all AUDIBLE in saved sessions. RC-1: Material choice index was scaled as if normalised, so Brass / Steel / Aluminum all played as Cast Iron (now 8–14 dB apart). RC-2: Sub / Oct layers never decayed while Bloom > 0 (`applyBloom` never ran on them) and had `initialFraction` applied twice. Unison: `initializePartials` always wrote voice 0, so Unison ≥ 2 lost the negative side of the detune and sat sharp (Unison 2 = no detune). "Noted" startNote frequency overwrite decided: DELETE — fundamental layer is exactly tuned. Damping untouched. New `probes.py` gates (material ≥ 5 dB, sub-band fall, unison peaks, bit-identity vs v4.5.2); `report.py` re-anchored to v4.6.0.
- **2026-09-20 (v4.7.0):** Preset-differentiation Step 3 (RC-4) — four append-only parameters (version hint 2), defaults BIT-IDENTICAL to 4.6.0: `partialModel` (Classic / Tubular / Plate / Bowl / Glass ratio + amplitude tables; prime stays 1.0 in every model), `humLevel` / `primeLevel` (−24…+6 dB), `humFollow` (hum-stage τ tracks Body Time, all partials). No UI yet (Step 4). The tables were not in `research/`; written in-step as `research/idiophone-partial-models.md` with per-row provenance (Glass row recalled, amplitudes are design values). Random-range median tap 11.3 → 16.4 (gate 16), held 10.2 → 15.5 (gated at equal gain 14.8 — does not reach 16). Harness gained `--alloc-check` (malloc_logger, audio-thread-scoped, liveness-proven): 0 allocations. Left alone: Classic has no Nyquist guard; Hum Follow is masked by the release on taps.
- **2026-09-20 (v4.7.1):** Preset-differentiation Step 4 — UI for the four v4.7.0 parameters. New **Partials** section between Synthesis and Ensemble, one `.param-row-4`: Model (5-way slider relay, as Material — option words exempt, D-01 arms 1 + 3), Hum Level, Prime Level, Hum Follow; four relays + attachments; four tips bound to `.param-control`. en / fr / zh-Hans rows for 5 labels + 4 tips — fr `reviewed: false` (9 on the worklist), zh-Hans `'mt'` (9 below ship bar). Width pin is structural: `.param-row-4` columns are 174.5 px on every arm, widest caption `Niveau bourdon` 101.5 px. `check-ui-labels` 0 moved on fr / zh-Hans in all 15 states; fr-lint / zh-lint 268 rows 0 findings; boot clean; `ui_tip_render_check` `SLIDER_COUNT` 35 → 39. No audio or state change.
- **2026-09-21 (v4.8.0):** Preset-differentiation Step 5 (last) — factory bank re-voiced and grown 25 → 40 (the 25 old names kept; 3 new per category: bowls, tubes, plates, glass); no parameter / state change. RC-3: Damping was authored inverted (Large Bells 0.88–1.0 → 0–0.15; bars / plates / anvils → 0.75–1.0). Partial Model across the 40: Classic 13 / Tubular 8 / Plate 7 / Bowl 6 / Glass 6; RC-5: each preset has its own delay / EQ / reverb-shape signature. **Two review corrections before release:** (1) NO preset names Output Gain — the user's control; bank balanced by voicing (10.9 dB RMS spread, peak −7.0 dBFS). (2) Presets titled after real instruments wobbled in pitch — causes measured: wide Unison detune (14–31 c RMS), chorus (12–30 c), Reverb Mod ≥ 0.3 (~10 c), and a Sub layer stacked on a lifted hum (slow beat + 8 dB note-to-note swing on one voice). 28 realism presets are now Unison 1 / no chorus / Reverb Mod ≤ 0.2 and read ≤ 1.7 c; 12 declared stylised, roughly halved. Factory sentinel 4.1.1 → 4.8.0. Harness, voice-isolated: pair median 13.3 → 26.4 tap / 14.7 → 27.1 held; worst nearest neighbour 3.5 → 9.5 over both seeds (gate ≥ 8); tap T40 medians Large 9.08 > Warm 5.17 > Bright 3.94 ≥ short Metallic 1.44–2.69. `report.py` gates: nearest neighbour, T40 order, wobble (metric + causes), no Output Gain, stale-bank guard. auval + pluginval 5 pass. **Hands-on listen in the Standalone: pending (human gate).**

## Known Issues / Limitations

- **Tuning UI vs DAW automation (WR-08):** Live automation of `tuning_*` params updates the
  engine but does not move the tuning-tab UI knob (no APVTS→UI push). Persistence/recall work.
- **MPE bends (IN-06):** Pitch bends are per-note, not per-channel — two simultaneous
  same-numbered notes on different MPE channels share one bend slot. Fine for the Dorico
  per-note-expression use case; a limit only if true MPE is expected.
- **Upgrade note:** on first launch of v4.1.1 the factory presets are regenerated (a
  `.factory_version` sentinel in the Factory dir); user presets are untouched.

## Description

O-Bells is a physical modeling bell synthesizer that creates realistic tubular bells, chimes, gongs, and other metallic percussion through modal synthesis. It features:

- **Modal synthesis** with configurable inharmonicity for authentic bell partials
- **Strike modeling** with position and mallet hardness controls
- **Material simulation** spanning bronze, steel, glass, and crystal
- **Ensemble section** with unison, detune, and octave layering
- **Built-in reverb** for spacious, ambient bell tones

## Parameters (33 total)

### Synthesis (6)
- Strike Position (0-100%) - Center to edge strike point
- Mallet Hardness (0-100%) - Soft to hard striker
- Damping (0-100%) - Hand-damped to free-ring
- Brightness (0-100%) - Dark to brilliant tone
- Material (0-100%) - Bronze → Steel → Glass → Crystal
- Inharmonicity (0-100%) - Pure harmonic to gamelan-style

### Ensemble (5)
- Unison Count (1-4) - Number of detuned copies
- Unison Detune (0-50 cents) - Detune spread
- Octave Blend Sub (0-100%) - Sub-octave layer
- Octave Blend Oct (0-100%) - Upper octave layer
- Stereo Spread (0-100%) - Ensemble panning width

### Character (3 choices)
- Strike Noise: Click / Thud / Ping
- Velocity Curve: Linear / Exponential / Logarithmic
- Decay Shape: Linear / Exponential / Multi-stage

### Advanced (5)
- Partial Tuning (-100 to +100 cents) - Minor-third partial adjustment
- Pitch Envelope (0-100%) - Initial pitch drop
- Pitch Env Time (5-200ms) - Pitch envelope return time
- Nonlinear Effects (0-100%) - Bell warping/distortion
- Humanize (0-100%) - Per-note organic variation [v2.4.0]

### Multi-Stage Envelope (4) - visible when Decay Shape = Multi-stage [v1.2.0]
- Strike Time (5-100ms) - Duration of bright metallic transient
- Brilliance (0-100%) - High-frequency sustain (0=warm, 100=bright)
- Body Time (100-5000ms) - Duration of main tonal decay
- Hum Sustain (0-100%) - Extension of low partial sustain

### Output (3)
- Reverb Mix (0-100%) - Spaciousness control [v1.1.0]
- Output Gain (-24 to +12 dB) - Master level
- Level Meter - Real-time stereo output metering

## Factory Presets (25)

- **Orchestral:** Tubular Bells, Concert Chimes, Glockenspiel, Celesta Mallet, Vibraphone
- **Sacred:** Church Bell, Cathedral Carillon, Meditation Bowl, Temple Gong, Singing Bowl
- **World:** Gamelan Saron, Gamelan Bonang, Tibetan Bowl, Steel Pan, Kalimba Bell
- **Ambient:** Frozen Shimmer, Bell Pad, Crystal Drone, Ethereal Chime, Submerged Bells
- **Cinematic:** Epic Bell, Tension Chime, Horror Stinger, Dramatic Swell, Distant Thunder

## Technical Details

- **DSP:** Modal synthesis with velocity-sensitive excitation
- **Reverb:** JUCE dsp::Reverb with bell-optimized settings (roomSize 0.7, damping 0.4)
- **UI:** WebView-based with botanical aesthetic theme
- **Formats:** VST3, AU
- **Installation:** ~/Library/Audio/Plug-Ins/VST3/ and ~/Library/Audio/Plug-Ins/Components/

## Improvement History

### v1.1.0 (2026-02-02)
**Request:** Add reverb control to make bells sound more spacious, positioned left of gain slider

**Implementation:**
- Added `reverbMix` parameter to APVTS (ID: "reverbMix")
- Implemented `juce::dsp::Reverb` in processBlock after synthesiser rendering
- Created WebSliderRelay/Attachment for WebView binding
- Added UI slider in Output section before Gain
- Optimized reverb settings for metallic resonance

**Files Modified:**
- PluginProcessor.h/cpp - DSP and parameter
- PluginEditor.h/cpp - Relay and attachment
- Resources/ui/index.html - UI element and JS binding

**Validation:** auval PASS, pluginval SUCCESS (strictness 5)

### v1.1.1 (2026-02-02)
**Request:** Default output level too loud, causes clipping

**Root Cause:** Signal exceeded 0 dBFS due to:
- 8 partials summing to ~2.7x amplitude (harmonic series)
- Unison voices stacking
- Octave layers (sub + upper) adding signal without normalization

**Fix:** Added proper gain staging normalization in BellVoice::renderNextBlock():
- Partial normalization: 0.4x factor to compensate for partial summing
- Unison normalization: 1/sqrt(unisonCount) - already existed
- Layer normalization: 1/(1 + subBlend + octBlend) for octave layers
- Output gain at 0 dB now produces unity gain as expected

**Files Modified:**
- BellVoice.cpp - renderNextBlock() gain staging

**Validation:** pluginval SUCCESS, DAW testing confirmed

### v1.2.0 (2026-02-02)
**Request:** Implement proper multi-stage decay envelope (previously a placeholder)

**Research-Based Implementation:**
- Comprehensive research from CCRMA, IRCAM, Arturia Pigments, AAS Chromaphone
- Academic formula: R_k = b₁ + b₃×f_k² for frequency-dependent damping
- Industry-standard parameter naming (Brilliance, Strike Time, Body Time, Hum Sustain)

**New Parameters (4):**
- **Strike Time** (5-100ms): Duration of bright metallic transient
- **Brilliance** (0-100%): High-frequency sustain (0=warm/woody, 100=bright/glassy)
- **Body Time** (0.1-5s): Duration of main tonal decay phase
- **Hum Sustain** (0-100%): Extension of low partial sustain

**Key Features:**
- Physics-based three-phase envelope (Strike → Body → Hum)
- Per-partial decay coefficients pre-calculated for performance
- Damping parameter affects only Hum phase (user requirement)
- New "Envelope" UI section appears only when Multi-stage is selected

**Files Modified:**
- PluginProcessor.h/cpp - 4 new APVTS parameters
- BellVoice.h/cpp - Multi-stage envelope DSP implementation
- PluginEditor.h/cpp - 4 new WebSlider relays and attachments
- Resources/ui/index.html - New Envelope section with show/hide logic

**Validation:** pluginval SUCCESS (strictness 5), auval PASS

### v2.2.0 (2026-02-03)
**Request:** Add GUI keyboard to footer panel, move Gain slider to footer

**Implementation:**
- Expanded footer from 40px to 55px height
- Added 2-octave interactive keyboard (C3-B4) with QWERTY support
- Moved Gain slider from Output section to footer
- Added `sendMidiNote` native function for keyboard → synth communication
- Added `triggerNoteOn`/`triggerNoteOff` methods to PluginProcessor

**Files Modified:**
- PluginProcessor.h/cpp - Added note trigger methods
- PluginEditor.cpp - Added sendMidiNote native function
- Resources/ui/index.html - Footer expansion, keyboard CSS/JS

**Validation:** Manual DAW testing confirmed

---

## Footer Module Integration Notes (v2.2.0 Lessons Learned)

When integrating the `instrument-footer-panel` module (from `modules/ui/instrument-footer-panel/`) into a plugin, **DO NOT** use the module's JS file directly. Instead, follow this surgical approach:

### What Works

1. **Add CSS inline** - Copy only the CSS styles you need into the plugin's `<style>` block. Don't replace existing footer CSS entirely.

2. **Modify existing footer HTML** - Expand the existing `<div class="footer">` to include:
   - Gain slider (with same `data-param` binding pattern as other sliders)
   - Keyboard container `<div class="footer-keyboard-viz" id="keyboard-viz">`
   - Branding text

3. **Add keyboard JS after existing bindings** - The keyboard JS must come AFTER the slider parameter binding code so `parameterStates.get('outputGain')` is available.

4. **Adjust tab-content height** - Change `calc(100% - 130px)` to `calc(100% - 145px)` for 55px footer.

### What Breaks

1. **Replacing footer CSS entirely** - This can accidentally remove styles for expandable sections, bloom fine controls, etc. that share CSS with the footer area.

2. **Using the module's standalone JS** - The module expects its own initialization pattern. Instead, inline the keyboard-building code and use the plugin's existing JUCE binding pattern.

3. **Reordering HTML sections** - Moving the Output section or changing its structure can break meter bindings.

### C++ Requirements

Add to PluginProcessor.h:
```cpp
void triggerNoteOn(int midiNote, float velocity);
void triggerNoteOff(int midiNote);
```

Add to PluginProcessor.cpp:
```cpp
void MyProcessor::triggerNoteOn(int midiNote, float velocity) {
    midiNote = juce::jlimit(0, 127, midiNote);
    velocity = juce::jlimit(0.0f, 1.0f, velocity);
    synthesiser.noteOn(1, midiNote, velocity);
}

void MyProcessor::triggerNoteOff(int midiNote) {
    midiNote = juce::jlimit(0, 127, midiNote);
    synthesiser.noteOff(1, midiNote, 0.0f, true);
}
```

Add native function in PluginEditor.cpp WebView options:
```cpp
.withNativeFunction("sendMidiNote", [this](const juce::Array<juce::var>& args,
                                            std::function<void(juce::var)> complete) {
    if (args.size() >= 3) {
        int midiNote = static_cast<int>(args[0]);
        float velocity = static_cast<float>(args[1]);
        bool isNoteOn = static_cast<bool>(args[2]);
        if (isNoteOn) processorRef.triggerNoteOn(midiNote, velocity);
        else processorRef.triggerNoteOff(midiNote);
    }
    complete({});
})
