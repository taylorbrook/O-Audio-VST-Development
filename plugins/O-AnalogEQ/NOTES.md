# O-AnalogEQ Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.5.4
- **Type:** Audio Effect (4-Band Parametric/Shelving EQ)
- **Complexity Score:** 4.0 (Complex - Phase-based implementation)

## Lifecycle Timeline

- **2026-01-11 (Ideation):** Creative brief created - Neve 1081-inspired analog EQ
- **2026-01-11 (UI Mockup v1):** Initial design iteration - dual-layer knobs
- **2026-01-11 (UI Mockup v2):** Second design iteration - layout refinements
- **2026-01-11 (UI Mockup v3):** Finalized mockup - paper texture + large rotated botanical overlay
- **2026-01-11 (Parameter Spec):** Finalized 16 parameters (10 float, 2 choice, 5 bool)
- **2026-01-11 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 4.0)
- **2026-01-11 (Stage 1):** Foundation + Shell complete
- **2026-01-11 (Stage 2):** DSP Implementation complete
- **2026-01-11 (Stage 3):** GUI Integration complete
- **2026-01-11 (v1.0.0):** Initial release - Installed to system folders
- **2026-01-11 (v1.0.1):** Fixed GUI connectivity - Q parameters now use correct WebComboBoxRelay
- **2026-01-11 (v1.0.2):** Added missing check_native_interop.js - WebView bridge now functional
- **2026-01-11 (v1.0.3):** Fixed dual-layer knobs + live VU meter with Marimba-style C++ events
- **2026-01-11 (v1.0.4):** UI polish - centered knobs, widened Q toggles, output label, vertical defaults, double-click reset, green gradient, frequency notches
- **2026-01-11 (v1.0.5):** Simplified UI - removed output gain, moved analog under VU, band labels now act as on/off toggles
- **2026-01-11 (v1.0.6):** UI polish - widened Q toggles, centered labels, resized flower overlay, updated title
- **2026-01-11 (v1.0.7):** UI refinements - single-line title, removed sublabels, Q toggles down 10px, flower centered, LF/HF SHELF labels
- **2026-01-11 (v1.0.8):** VU meter 2x size, shifted right; analog button centered between HF dial and VU meter
- **2026-01-11 (v1.0.9):** VU meter reduced to 80% (112px); analog button moved right 20px
- **2026-01-11 (v1.0.10):** VU meter left 40px; analog saturation retuned for warmth without gain boost
- **2026-01-24 (v1.1.0):** Renamed plugin from OuariconAnalogEQ to O-AnalogEQ
- **2026-02-05 (v1.1.4):** Licensing module + Windows WebView2 backend support
- **2026-02-09 (v1.1.7):** Preset system + UI/EQ algorithm improvements
- **2026-06-30 (v1.1.8):** Code-review fixes — CR-01 (audio-thread coefficient allocation removed via change-gated rebuild) + WR-01 (frequency tooltips now honor the 0.3 skew)
- **2026-06-30 (v1.1.9):** Remaining code-review warnings — WR-02 (per-band freq/gain SmoothedValue with 32-sample sub-block coefficient rebuild via allocation-free ArrayCoefficients, kills zipper noise while staying RT-safe) + WR-03 (all cutoffs clamped to 0.99×Nyquist; verified via auval at 11025/22050 Hz) + WR-04 (FileChooser callbacks guarded with Component::SafePointer)
- **2026-09-25 (v1.5.1):** Second thorough code review (v1.5.0) resolved — CR-02 (`isBusesLayoutSupported` override; the base class advertised asymmetric layouts, and a 2-in/1-out negotiation prepared one mono filter per band against a 2-channel buffer, so `ProcessorDuplicator` indexed `processors[1]` → `nullptr` on the audio thread in Release), WR-05 (`output_gain` now ramps — `dsp::Gain`'s default `rampDurationSeconds` is 0, and `setRampDurationSeconds` was never called), WR-06 (band on/off is a 30 ms wet/dry crossfade and the filter runs unconditionally, which fixes the toggle click AND the stale-`z⁻¹` burst on re-enable in one mechanism), WR-07 (Save Preset writes to the chosen path via `savePresetToFile`; it had been discarding the directory and could silently overwrite a library preset), WR-08 (hover-help switch gained 13 behavioural assertions — deleting the show gate now fails the suite, where it previously left 415 PASS), WR-09 (the switch's Off arm is geometry-measured; `check-ui-labels` states may now carry a selector array), WR-10 (`check-i18n` derives its served set from `juce_add_binary_data` and gained a markup-in-JS title detector; preset-manager → 1.0.9 with `title=` → `aria-label`)
- **2026-09-26 (v1.5.2):** Two residual defects from the v1.5.1 fixes themselves — WR-07 follow-up (that release swapped `savePreset(name)` → `savePresetToFile(file)` to honour the chosen directory and silently dropped the `isFactoryPreset()` early-return the first API carried; the guard is back at the call site as `oaeq::presetSaveRefusal`, **location-aware** rather than name-only so arbitrary-path export still works) and WR-10 follow-up (the served-set scan's regex excluded `${…}`, so `${CMAKE_SOURCE_DIR}/…` entries resolved under the *plugin* root, missed, and were dropped with no report — O-ReverseDelay, O-Contrabass, O-Marimba and O-MicrotonalSampler were embedding modules nothing scanned while their gates printed green)
- **2026-09-26 (v1.5.3):** Info tier of the v1.5.0 review cleared — IN-06..09, all four. IN-07 (16 per-block `getRawParameterValue` lookups → 16 pointers resolved in the constructor; waste, NOT an RT-safety violation — APVTS keys `adapterTable` on `StringRef` so a literal `find` allocates nothing, and the header records that so it is not re-raised as a blocker) and IN-08 (a second preset dialog destroyed the first `FileChooser` mid-flight; closed with a `presetDialogInFlight` bool, **not** the review's prescribed `fileChooser.reset()` in the completion — that would destroy the chooser and the executing `std::function` from inside its own callback, and the `!= nullptr` guard without a clear latches shut forever). IN-06/IN-09 were stale comments and a stale version literal; both deleted rather than corrected, because the count in IN-06 had **inverted** (claimed 2 of 43 plugins carried the hover-help toggle; measured 25 of 44) and a hand-maintained version string has no gate. New gates: `tests/check-param-cache.js` (11 assertions, gates IN-07's mapping by NAME since G1–G4 all pass with two same-band reads swapped) and `ui_tip_render_check.js` check [10] (no `vX.Y.Z` in any `console.*` call). Both seen to FAIL before acceptance. Review's open list now empty.
- **2026-09-26 (v1.5.4):** A leftover of v1.5.3's own IN-09 fix, and the reason it was left. The harness banner in `tests/render-harness/main.cpp` printed `(v1.5.2)` against a 1.5.3 build, with two more copies in the `main.cpp` and harness-`CMakeLists.txt` title comments (`v1.5.2` and `v1.5.1` — already disagreeing with each other). IN-09 was worded as *the page's* init log, so the sweep deleted the page copy, gated `index.html`, and never looked at the other announcing surface: **the fix was scoped to the instance, the gate was written to the instance, and the class went on printing green for a version.** Banner now reads `JucePlugin_VersionString` — already in the TU from `OAEQ_VERSION` for the factory-preset sentinel, so the literal was pure duplication — concatenated into the format string, not `%s`, so a missing define breaks the build instead of printing something plausible. Title comments **drop** the version rather than being corrected, per IN-06/IN-09's reasoning. Check [10] widened from `console.*` in `index.html` to the announcing call on both surfaces (`console.*` in the page, `printf` in the harness), negative-controlled in five arms against the *shipped* scan text sliced out of the test file: clean tree, each literal restored separately, both at once (both files named in one verdict — a stop-at-first-hit scan passes that arm), and a `v9.9.9` in a non-call comment left alone, which is what keeps the history comments legal. Harness 19/19, identical verdicts.
- **2026-06-30 (v1.1.10):** Code-review info items — IN-01 (documented `output_gain` as intentionally UI-hidden/host-only), IN-02 (double-click reset restores true APVTS freq defaults via skew inverse), IN-03 (removed dead `currentParamName`), IN-04+IN-05 (shared `preset-manager` module → 1.0.1: bounded `_waitForNative` poll + robust `promptDelete` with `onConfirmDelete` hook)

## Known Issues

- **Deferred from the v1.5.0 review (Info tier, opt-in):** IN-06 (settings-popover contract comments still describe a one-row panel), IN-07 (16 `getRawParameterValue(StringRef)` lookups per block — a red-black-tree walk, NOT an allocation; deliberately not escalated), IN-08 (a second dialog launch destroys the first `FileChooser` mid-flight — PLAUSIBLE, needs the page to open two native dialogs at once), IN-09 (the page's init log still announces v1.3.1). Sweep them with `/improve-review-info O-AnalogEQ`.
- **`check-i18n` prose rules do not scan served shared modules.** A deliberate scope boundary, not an oversight: firing [12]/[13]/[15] unrestricted turns seven plugins red on unkeyed English captions (O-Bassoon/O-Bowed/O-Reed/O-Wind 39 each, O-Contrabass 46, O-ReverseDelay 6, O-Marimba 5). That is a shared-module keying rollout. The §4 native-title rule IS now enforced over the served set — genuinely so as of v1.5.2, which fixed the `${CMAKE_SOURCE_DIR}` resolution that had been silently excluding four plugins from that very set — and the gate names the modules it is not prose-scanning on every run.
- **18 other `preset-manager.js` consumers carry the v1.0.9 markup fix but were not re-versioned or rebuilt.** Seven are git-ignored copies regenerated from canonical at configure time (no action needed). Five tracked copies were overwritten to canonical. Six tracked copies remain diverged 24–184 lines and took a surgical edit — none of them uses `ouaricon_add_module` for this module, so their committed copy is authoritative and they need `/module-upgrade` on their own schedule to converge.
- **`savePresetToFile()` is still unguarded in `modules/persistence/`.** v1.5.2 puts the factory-preset check at O-AnalogEQ's call site, not in the shared module, because 19 consumers share it. Nine other plugins call that API from a save dialog (O-Bitrot, O-Contrabass, O-Emulator, O-FreqPulse, O-Orbit, O-ReverseDelay, O-simpleFM, O-Tapestop, O-Wind) and have **not** been audited for the same gap; several are mid-stage. Worth a pass of its own.
- All 2026-06-30 code-review findings (CR-01, WR-01..04, IN-01..05) remain resolved; the v1.5.0 review re-adjudicated all ten as FIXED with no regressions across thirteen versions.

## Additional Notes

### Description

A lightweight, knob-based 4-band EQ plugin inspired by the Waves V-EQ4 and Neve 1081 console module. Emphasizes simplicity and musicality with dual-layer knob controls and subtle analog warmth.

### Key Features

**EQ Bands:**
- **LF (Low Frequency):** Shelving filter, 30-500 Hz, ±12 dB
- **LMF (Low-Mid Frequency):** Parametric bell filter, 100-2000 Hz, ±12 dB, 3 Q settings (WIDE/MED/TIGHT)
- **HMF (High-Mid Frequency):** Parametric bell filter, 500-8000 Hz, ±12 dB, 3 Q settings (WIDE/MED/TIGHT)
- **HF (High Frequency):** Shelving filter, 2-20 kHz, ±12 dB

**Global Controls:**
- **Output Gain:** ±12 dB post-EQ trim
- **Analog Toggle:** Enable/disable subtle harmonic saturation

**Sound Character:**
- Neve-inspired musical EQ curves (minimum-phase IIR filters)
- Subtle analog warmth via tanh waveshaping
- Gentle harmonic saturation (0.5-5% THD depending on level)
- Console-style signal flow (LF → HF → Saturation → Output)

### Parameters

Total: 16 parameters (10 float, 2 choice, 5 bool)

**Frequency Parameters (4):**
- `lf_freq`: 30-500 Hz (log scale)
- `lmf_freq`: 100-2000 Hz (log scale)
- `hmf_freq`: 500-8000 Hz (log scale)
- `hf_freq`: 2000-20000 Hz (log scale)

**Gain Parameters (5):**
- `lf_gain`: ±12 dB (linear dB)
- `lmf_gain`: ±12 dB (linear dB)
- `hmf_gain`: ±12 dB (linear dB)
- `hf_gain`: ±12 dB (linear dB)
- `output_gain`: ±12 dB (linear dB)

**Q Parameters (2):**
- `lmf_q`: Choice (0=WIDE/0.5, 1=MED/1.0, 2=TIGHT/2.0)
- `hmf_q`: Choice (0=WIDE/0.5, 1=MED/1.0, 2=TIGHT/2.0)

**Toggle Parameters (5):**
- `lf_on`: Band enable (default: On)
- `lmf_on`: Band enable (default: On)
- `hmf_on`: Band enable (default: On)
- `hf_on`: Band enable (default: On)
- `analog`: Saturation enable (default: On)

### DSP Architecture

**Processing Chain:**
```
Input → LF Shelf → LMF Bell → HMF Bell → HF Shelf → Saturation → Output Gain → Output
```

**JUCE Components:**
- `juce::dsp::IIR::Filter` - All 4 EQ bands (shelving + parametric)
  - Coefficients: `makeLowShelf()`, `makeHighShelf()`, `makePeakFilter()`
- `juce::dsp::WaveShaper` - Analog saturation (`tanh(x * 1.5) * 1.1`)
- `juce::dsp::Gain` - Output trim

**Filter Details:**
- LF/HF shelving: Fixed Q = 0.707 (Butterworth, musical slope)
- LMF/HMF parametric: Variable Q = 0.5/1.0/2.0 (user selectable)
- All filters are IIR biquads (minimum-phase, analog-like)
- Hard bypass when band disabled (skip processing, not mute)

**Saturation:**
- Transfer function: `tanh(x * 1.5) * 1.1`
- Drive: 1.5x (gentle, not aggressive)
- Post-gain: 1.1x (compensate tanh output range)
- Position: Post-EQ (saturate EQ'd signal, console workflow)
- No oversampling (gentle drive, band-limited by EQ)

**Performance:**
- Estimated CPU: ~11-12% single core @ 48kHz
- Latency: < 5 samples (~0.1ms, negligible)
- Denormal protection: `ScopedNoDenormals` in processBlock

### GUI Design

**Layout:** Compact rack-unit (920×220px)
- Paper texture background (`paper1.jpg`)
- Large rotated botanical overlay (90° clockwise, 45% opacity)
- Single horizontal row with tight vertical spacing

**Controls:**
- **4 Dual-Layer Knobs:** Seed cross-section pattern
  - Outer ring: Frequency (10-segment conic gradient)
  - Inner dial: Gain (center core + radial segments)
  - Independent rotation for each layer
- **2 Q Toggles:** 3-way switches (WIDE/MED/TIGHT) for LMF/HMF
- **5 Band Toggles:** Enable/disable per band (botanical green when active)
- **1 Output Gain Knob:** Standard single-layer knob
- **1 Analog Toggle:** Large botanical toggle
- **1 VU Meter:** Circular meter (far right, responds to output level)

**Color Palette:** Aged paper (walnut brown, cream, botanical green)

### Implementation Strategy

**Complexity:** 4.0 (Complex) → Phase-based implementation

**DSP Phases:**
- **Phase 4.1:** Core EQ Processing (LF shelf + output gain validation)
- **Phase 4.2:** Full EQ Chain (LMF, HMF, HF bands)
- **Phase 4.3:** Analog Saturation (tanh waveshaping)

**GUI Phases:**
- **Phase 5.1:** Layout and Basic Controls (WebView rendering, v3 mockup)
- **Phase 5.2:** Parameter Binding (dual-layer knobs, toggles, VU meter)

### Professional References

- **UAD Neve 1081 Classic Console EQ:** Primary inspiration (shelving filters, switchable to bell)
- **Waves V-EQ4:** Neve 1081 emulation (±18dB shelving, analog saturation)
- **FabFilter Pro-Q:** Surgical EQ (opposite design goal - too clean/transparent)

### Technical Specifications

- **Plugin Formats:** VST3, AU (planned)
- **Sample Rates:** 44.1kHz - 192kHz
- **Latency:** Zero (< 5 samples, not reported to host)
- **CPU:** Lightweight (~11-12% single core @ 48kHz)
- **Thread Safety:** Atomic parameter reads, non-allocating coefficient updates

### Validation Checklist (Stage 6)

- [ ] Build successful (CMake + JUCE 8)
- [ ] VST3 and AU formats built
- [ ] Pluginval passes (all tests green)
- [ ] No memory leaks (Valgrind or Instruments)
- [ ] Presets created (at least 5 factory presets)
- [ ] Changelog created (CHANGELOG.md)
- [ ] Installation successful (VST3 + AU in system folders)
- [ ] DAW compatibility tested (Logic Pro, Ableton Live)

### Contract Files

- Creative brief: `plugins/O-AnalogEQ/.ideas/creative-brief.md`
- Parameter spec: `plugins/O-AnalogEQ/.ideas/parameter-spec.md`
- DSP architecture: `plugins/O-AnalogEQ/.ideas/architecture.md`
- Implementation plan: `plugins/O-AnalogEQ/.ideas/plan.md`
- UI mockup: `plugins/O-AnalogEQ/.ideas/mockups/v3-ui.yaml`

### Next Steps

1. Run `/implement OuariconAnalogEQ` to start Stage 1 (Foundation + Shell)
2. Implement DSP in 3 phases (4.1, 4.2, 4.3)
3. Implement GUI in 2 phases (5.1, 5.2)
4. Validate with pluginval, create presets, changelog
5. Install and test in DAW

---

**Last Updated:** 2026-06-30
**Version:** 1.1.8
**Status:** 📦 Installed
