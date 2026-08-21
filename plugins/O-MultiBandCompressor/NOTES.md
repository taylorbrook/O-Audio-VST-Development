# O-MultiBandCompressor Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.9.0
- **Type:** Audio Effect (Dynamics Processor - Multiband Compressor)
- **Complexity:** 5.0 (Maximum complexity - 56 parameters, 10 DSP components)

## Lifecycle Timeline

- **2026-08-20 (v1.9.0):** Detector-level fixes behind "bands only engage at really low thresholds". The stereo link was averaging *signed* samples — `(L+R)/2` — before rectification: a mono fold-down of the sidechain that read hard-panned content −6 dB low, decorrelated stereo ~3–6 dB low, and could not see anti-phase content at all. Linked mode now filters each channel on its own SC state (both states already existed for the unlinked path), rectifies, and links on the max — the standard topology; SC Listen in linked mode became true stereo as a side effect. Second fix: RMS calibrated +3 dB (AES-17 style) so a steady sine's RMS reads its peak, keeping the peak/RMS blend from shifting the effective threshold as it moves toward RMS. Harness A/B against the v1.8.0 detector (baseline rebuilt from backup): every active preset deepens ~2–3.5 dB GR, none changes which bands engage, Init presets stay inert, order-independence clean. On the correlated harness bed the RMS calibration dominates; wide stereo program gets more from the link fix. No parameter/state changes; presets compress somewhat harder on stereo material.
- **2026-08-19 (v1.7.0):** Twenty-five more factory presets (50 total) and a categorised preset browser. Seven groups — Init, Mastering, Mix Bus, Corrective, Instruments, Voice, Creative — plus a User group that appears once something has been saved, with headings that stick while their group scrolls. The category lives in this plugin's own `kFactoryPresets` table, not in the shared `OuariconPresetManager`, so the ~20 other plugins vendoring that module are untouched; it is deliberately NOT written into the `Factory/*.json` files but recovered by name lookup, so a renamed or hand-edited preset file cannot carry a stale category. Anything absent from the factory table reports as "User" — exactly the set that should be deletable — which let the dropdown drop its per-name `isFactoryPreset()` fan-out (N native round trips per list change) for one `getPresetCategories()` call. Load-bearing consequence of grouping: ◀ / ▶ had been walking `getPresetList()`'s flat alphabetical order, which *was* the dropdown order; once grouped the two disagree, so navigation is now driven from the same flattened group order the browser renders or ▶ from the last Mastering preset lands mid-Instruments. The harness's "should be inert" check now keys off the Init *category* rather than the name "Init Flat", so later Init presets are covered automatically — but an Init preset shipping a ratio above 1:1 now fails there. Two new presets were retuned after their first measurement: *Sub Rumble Control* at -40 dB/8:1 measured 31 dB of gain reduction (a gate, not a control — the band-level staircase is calibrated for bands carrying program material, and below 45 Hz there should be almost nothing, so the threshold must sit above a clean recording's noise floor) and *Dark Tilt*'s high band at -42 dB/8:1 measured 27 dB, i.e. compressing continuously, which is the exact behaviour its own comment claimed it avoided. Every v1.6.0 preset measures byte-identical gain reduction to its v1.6.0 baseline. Also fixed: `.preset-dropdown-header` had been dead CSS since v1.5.0 (never referenced by app.js) and is now what the group headings use, with an opaque background since a translucent sticky heading lets the rows under it show through; the delete-confirmation strip is pinned too, since prepending it into a 50-entry scroll container rendered it off-screen. Harness 50/50 + reverse pass, pluginval strictness 10 + auval pass.
- **2026-08-19 (v1.6.1):** Deep code review resolution (CR-01, WR-01..WR-05 from CODE_REVIEW.md). The headline: M/S "Both" mode had never been independent M/S compression — the stereo-linked detector averages its channels, and (M+S)/2 = L/√2, so the detector heard only the left channel and one shared gain on M and S is algebraically identical to L/R processing. `Compressor` grew an unlinked dual-mono topology (per-channel detectors, ballistics, SC filter state, makeup state) selected by a `linkedDetector` flag only mode 3 clears; the "up to 8 compressors in Both mode" claim is now actually true. Also: locale-safe JS injection for the three 30 Hz meter pushes (comma-decimal Windows hosts got a JS syntax error every tick and frozen meters), a new `PhaseMatchChain` (AP(f1)·AP(f2)·AP(f3)) applied to both the M/S passthrough channel (decode no longer mirrors the image near crossovers) and the Mix dry path (no more parallel-compression combing), ~15 ms one-pole smoothing on manual+auto makeup (no more block-boundary zipper / AUTO_MAKEUP click), and NaN entry-guard + self-heal on the RMS detector (one upstream NaN used to disable a band's compression until re-prepare). No parameter/preset/state changes. Verified: CR-01 probe (right-only noise −16..−29 dB GR vs exactly 0 before; side/mid separation 45:1 → 2.1:1), preset harness order-independence clean, auval pass.
- **2026-07-23 (v1.6.0):** Nine more factory presets (25 total) — three harshness (Cymbal Sizzle Control, Nasal Honk Control, Amp Fizz Control) and six instrument (Electric Guitar, Piano, Strings Ensemble, Brass Section, Mallet Percussion, Woodwind Breath) — plus a real sidechain bug the verification found. `Compressor::updateSidechainFilters()` set its enabled flag only inside the "frequency changed" branch, so `freq > 0 && freq == currentFreq` fell through both branches and inherited whatever enabled state was there before: SC HPF/LPF silently stayed off while the UI showed a frequency. Reachable with one knob (100 Hz -> Off -> 100 Hz) and on every preset switch, since the cached frequency survives it — which is how it surfaced, as presets reading 3–4 dB differently depending only on load order. Same pass moved the coefficient update off `Coefficients::makeHighPass` (heap-allocates in processBlock) onto `ArrayCoefficients` + `operator=(std::array)`, with prepare() seeding a real biquad so the assignment reuses storage. Gotcha worth remembering: `Coefficients` stores 5 NORMALISED values, not the 6 raw ones ArrayCoefficients returns — copying the array over getRawCoefficients() silently produces a wrong filter. The harness now re-measures every preset in reverse order and fails on any band differing by >0.01 dB, which is the regression guard for exactly this class of leaked-state bug. Version now single-sourced as OMBC_VERSION so the harness cannot drift and restamp the real factory-preset directory. pluginval strictness 10 ×3 + auval pass.
- **2026-07-22 (v1.5.0):** Preset management added via the shared `preset-manager` module (v1.0.5, CMake-included rather than vendored) plus 16 research-informed factory presets, and the three per-band Detector/Sidechain parameters (`*_PEAK_RMS`, `*_SC_HPF`, `*_SC_LPF`) finally got on-screen controls — they had been fully wired into the DSP since launch but reachable only through host automation. Preset bar sits in the header (prev/next, click-to-browse, Save/Load via native dialogs, delete-with-confirmation on user presets only). Two preset-design constraints are load-bearing and easy to get wrong: thresholds must follow a band-level staircase (threshold is measured on the *band* signal, so the 8 kHz+ band sits 25–30 dB below the low band and one shared threshold would never engage up top), and factory values must be authored in engineering units and converted with `convertTo0to1()` — `ATTACK`/`RELEASE`/`XOVER*`/`SC_*` all carry skew 0.3, so hand-written normalised fractions recall 10–30× wrong. New per-band knob row grew the window to 900×640. State format unchanged apart from an added `currentPreset` attribute; v1.4.x sessions load unchanged. New `tests/render-harness` (off by default) loads every factory preset, renders correlated stereo pink noise and reports per-band gain reduction; it caught three over-compressing presets (Vocal Bus, Podcast Voice, Acoustic Guitar low bands) which were retuned. pluginval strictness 10 ×3 + auval pass.
- **2026-01-25 (Ideation):** Creative brief created - 4-band multiband compressor with Linkwitz-Riley crossovers, M/S processing, sidechain filtering, and real-time FFT visualization
- **2026-01-25 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 5.0, phased implementation)
- **2026-01-25 (Stage 1):** Foundation complete - Build system operational, 56 parameters implemented in APVTS
- **2026-01-25 (v1.0.0):** Production ready - All stages complete (DSP, crossover network, M/S processing, WebView UI, metering)
- **2026-01-25 (v1.1.0):** Added draggable crossover controls - click and drag to adjust XOVER1/2/3 frequency split points
- **2026-01-26 (v1.2.0):** Real-time FFT spectrum analyzer - 2048-sample FFT with lock-free audio→UI communication
- **2026-07-01 (v1.2.1):** RT-safety pass (CODE-REVIEW.md) - removed all audio-thread allocation/locking (CR-01/02/03, WR-01) + hot-loop/cleanup (IN-01..04). No sonic change (crossover verified bit-identical); pluginval strictness 10 + auval pass.
- **2026-07-01 (v1.2.2):** Correctness + polish pass (CODE-REVIEW.md) - WR-02 M/S detection −6 dB fixed (active-channel count threaded into the detector), WR-04 Attack/Release readouts now use the skew-0.3 mapping, IN-05 resource provider matches full relative path, IN-06 spectrum bins log-spaced 20 Hz–20 kHz (peak per bin). auval pass.
- **2026-07-22 (v1.4.2):** The three buttons under each band now read SOLO / BYPASS / SC LISTEN. They had all been showing "Off" (or "On" when engaged) because `updateToggleUI()` — shared by every toggle in the plugin — unconditionally rewrote the button text on bind and on every state change, so the `S` / `B` / `SC` glyphs in `index.html` were overwritten the moment the UI connected and had never been visible in a DAW. Each band button now carries its name in `data-label`, which `updateToggleUI()` preserves; engaged state is shown by the existing `.active` olive fill alone, with `aria-pressed` added since colour is now the only visual cue. Buttons size to their text (163 px row inside a 189 px band column, `white-space: nowrap`). The global Auto-MU toggle has no `data-label` and still reads On/Off beside its own caption. No DSP or state changes. Browser harness (four-band geometry + toggle round-trip) + auval pass.
- **2026-07-22 (v1.4.1):** Added a "?" button in the header that switches the v1.4.0 tooltips off and on. State lives in a machine-wide preference file (`~/Library/Application Support/Ouaricon/`) via two new WebView native functions, deliberately outside the APVTS so presets never carry somebody else's help setting; defaults to on. Also fixed a v1.4.0 positioning bug where tips near the right edge re-wrapped into a ~70 px ribbon (a fixed-position box with `left` set shrink-to-fits the space to its right, so measure-then-place under-reported the width) — the tip width is now pinned in px before placement. No DSP or state changes. Browser harness (11-control geometry sweep) + auval pass.
- **2026-07-22 (v1.4.0):** UI pass - tooltips on all 41 interactive controls (styled parchment layer, 120 ms delay, edge-aware flip/clamp, hides on drag); band header frequency ranges now track the crossovers live on drag, automation, and preset recall (they were static markup strings that never updated); `juce_add_plugin VERSION` added so the bundle stops reporting 1.0.0; crossover lines start at their true log positions; `applyOrderingConstraints` no longer misreads a crossover parked at its range minimum. No DSP or state changes. Browser harness + auval pass.
- **2026-07-01 (v1.3.0):** WR-03 crossover all-pass compensation - LOW passes through AP(f2)·AP(f3), LOMID through AP(f3); the 4-band sum is now pure all-pass (magnitude-flat at unity). Old ripple up to 0.63 dB → new ≤0.014 dB, verified by offline impulse-FFT + stepped swept-sine A/B harness. pluginval strictness 10 + auval pass.

## Known Issues

None. All items from the v1.2.0 code review (`.planning/CODE-REVIEW.md`) are resolved as
of v1.3.0.

### Note for future WebView work on this plugin

`initializeUI()` is invoked at module top level in `app.js` (line 20), *above* most of the
file's `let`/`const` declarations. Anything it calls must therefore avoid touching module
state declared further down — those bindings are still in the temporal dead zone, and the
resulting `ReferenceError` escapes module evaluation and silently kills every initializer
below it (this is how v1.4.0 development briefly broke crossover dragging). The C++ build,
`auval`, and static selector checks all pass in that state; only loading the page catches
it. Initialize late-declared subsystems at the foot of the file, as `initializeCrossoverDrag`
and `initializeTooltips` both do.

## Additional Notes

### Description
Professional 4-band multiband compressor designed for mixing and mastering workflows. Features Linkwitz-Riley 24dB/oct crossovers, per-band sidechain filtering, Peak/RMS blend detection, Mid/Side processing, auto-makeup gain, and real-time spectrum analyzer with gain reduction metering - all wrapped in the Botanical/Ouaricon aesthetic.

### Key Features
- **4-band crossover:** Linkwitz-Riley 4th order (24 dB/octave) at 200Hz, 2kHz, 8kHz (adjustable)
- **Per-band compression:** Threshold, Ratio (1:1 to 20:1), Attack (0.1-200ms), Release (10-2000ms), Knee (0-24dB)
- **Detection modes:** Continuous Peak/RMS blend (0-100%) per band
- **Sidechain filtering:** HPF and LPF per band (frequency-selective compression)
- **Mid/Side processing:** Off/Mid/Side/Both modes (up to 8 compressors in Both mode)
- **Auto-makeup gain:** Automatic gain compensation with slow ballistics (prevents pumping)
- **Parallel compression:** Global dry/wet mix (New York compression technique)
- **Visualization:** Real-time FFT spectrum analyzer with band overlays and per-band GR meters

### Parameters (56 total)
**Global (8 parameters):**
- INPUT_GAIN (-24 to +24 dB)
- OUTPUT_GAIN (-24 to +24 dB)
- MIX (0-100%)
- AUTO_MAKEUP (bool)
- MS_MODE (choice: Off/Mid/Side/Both)
- XOVER1 (20-500Hz, logarithmic)
- XOVER2 (200-5kHz, logarithmic)
- XOVER3 (2-16kHz, logarithmic)

**Per-Band (12 parameters × 4 bands = 48 parameters):**
- [BAND]_THRESHOLD (-60 to 0 dB)
- [BAND]_RATIO (1:1 to 20:1)
- [BAND]_ATTACK (0.1 to 200 ms)
- [BAND]_RELEASE (10 to 2000 ms)
- [BAND]_KNEE (0 to 24 dB)
- [BAND]_MAKEUP (-12 to +24 dB)
- [BAND]_PEAK_RMS (0-100%)
- [BAND]_SOLO (bool)
- [BAND]_BYPASS (bool)
- [BAND]_SC_HPF (0-2000 Hz, 0=off)
- [BAND]_SC_LPF (0-20000 Hz, 0=off)
- [BAND]_SC_LISTEN (bool)

**Band Prefixes:** LOW, LOMID, HIMID, HIGH

### DSP Architecture
- **Crossover:** Cascaded 2nd order Butterworth filters (Linkwitz-Riley 4th order)
- **Compressor:** Custom feed-forward topology with soft knee and Peak/RMS blend
- **M/S Encoding:** Power-preserving matrix (√2 scaling)
- **FFT Analysis:** 2048 samples, Hann window, 30-60fps updates (separate thread)
- **Latency:** ~10-12ms (IIR filters + attack lookahead)
- **CPU Target:** <30% single core @ 48kHz stereo (Off mode), <50% (Both mode)

### GUI
- WebView-based UI with Botanical/Ouaricon aesthetic
- Real-time spectrum analyzer (20Hz-20kHz, -80dB to 0dB)
- Draggable crossover handles on spectrum display
- Per-band gain reduction meters (vertical bars, 0 to -24dB)
- Input/output meters (stereo peak + RMS)
- 4-column layout (one per band) with all compression controls

### Implementation Strategy
**Phased implementation** (Complexity score 5.0):
1. **Phase 4.1:** Single-band compressor foundation (all features on single band)
2. **Phase 4.2:** Linkwitz-Riley crossover + 4-band architecture
3. **Phase 4.3:** Sidechain filtering + M/S processing + Auto-makeup + Dry/Wet
4. **Phase 5.1:** WebView layout + Spectrum analyzer + Crossover handles
5. **Phase 5.2:** Parameter binding (57 parameters, two-way communication)
6. **Phase 5.3:** GR meters + FFT visualization threading

### Risk Assessment
- **MEDIUM Risk:** Linkwitz-Riley coefficient smoothing (clicks on crossover frequency changes)
- **MEDIUM Risk:** FFT thread communication (lock-free audio → FFT → UI pipeline)
- **MEDIUM Risk:** Performance optimization (25-35% CPU target, may need SIMD)
- **LOW Risk:** M/S encoding/decoding (simple matrix math)
- **LOW Risk:** Soft knee gain computer (well-documented quadratic formula)

### Validation
- **Formats:** VST3, AU
- **Target Sample Rates:** 44.1kHz to 192kHz
- **Target DAWs:** Logic Pro, Ableton Live, FL Studio, Reaper
- **Expected Duration:** 18-28 hours (highly complex plugin)

### Contracts
- Creative Brief: `plugins/O-MultiBandCompressor/.ideas/creative-brief.md`
- Architecture: `plugins/O-MultiBandCompressor/.contracts/architecture.md`
- Implementation Plan: `plugins/O-MultiBandCompressor/.contracts/plan.md`

### Build Artifacts
- **Source Files:**
  - `plugins/O-MultiBandCompressor/CMakeLists.txt`
  - `plugins/O-MultiBandCompressor/Source/PluginProcessor.{h,cpp}`
  - `plugins/O-MultiBandCompressor/Source/PluginEditor.{h,cpp}`
- **Build Location:** `build/plugins/O-MultiBandCompressor/`
- **Installed Formats:**
  - VST3: `~/Library/Audio/Plug-Ins/VST3/O-MultiBandCompressor.vst3`
  - AU: `~/Library/Audio/Plug-Ins/Components/O-MultiBandCompressor.component`

---

## v1.8.0 — house dials

The 39 controls were `<input type="range">` circles until v1.8.0 and are now the
suite's seed cross-section knob, ported from **O-ReverseDelay** — the reference
copy for both the face and the pointer-captured drag. There is no shared knob
module in this repo: every plugin carries its own copy, so a fix here does not
propagate and a fix elsewhere does not arrive.

Two things worth knowing before touching them again:

- **Sizes are local, the face is not.** The 36 / 28 / 44 px sizes are this
  plugin's own — the reference knob is 56 px, which does not fit four band
  columns of three knob rows in 900×640. Stem lengths are set per size to reach
  ~85% of each radius; changing a knob size means changing its stem too.
- **Everything the binders touch is declared above the `initializeUI()` call.**
  That call sits at module top level in `app.js`, so a `const` moved below it
  would still be in its temporal dead zone and would throw, taking every later
  initializer on the module with it.

Value entry converts typed text through the live `start`/`end`/`skew` from C++
rather than the display formulas, so it stays correct even though the six main
readouts still use hand-written range maths rather than `getScaledValue()`.

---

*Last updated: 2026-08-20*
