# Stage 1 (Foundation) — CONTEXT

**Plugin:** O-simpleGrain
**Stage:** 1 of 4 — Foundation + Shell
**Date:** 2026-06-24
**Source:** Auto-compiled from contracts (BRIEF.md, parameter-spec.md, research/ARCHITECTURE.md, ROADMAP.md, STATUS.md). No interactive discuss session (momentum path; Stage 0 resolved all 11 open questions).

## Stage Goal

A **silent, valid synth shell**: builds and loads as an instrument (VST3 + AU + Standalone) on macOS, exposes the full **18-parameter APVTS**, persists state (incl. loaded-source identity as custom ValueTree state), and carries a correct cross-platform WebView/CMake config so Stage 3 inherits it. **No audio yet** (Stage 2), **no WebView UI yet** (Stage 3). The plugin instantiates, shows the default JUCE generic editor (or empty editor), accepts MIDI without crashing, and round-trips its parameter state.

## In Scope (Stage 1)

- `plugins/O-simpleGrain/CMakeLists.txt` — `juce_add_plugin` with `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; FORMATS `VST3 AU Standalone`; `PLUGIN_CODE` unique 4-char; `PRODUCT_NAME "O-simpleGrain${OUARICON_DEV_SUFFIX}"`; compile defs incl. `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_CURL=0`. Mirror O-simpleFM CMake. **Defer** `juce_add_binary_data` UI-resources block and `ouaricon_add_module` to Stage 3 (no UI files exist yet) — but DO add the **embedded-sample binary-data target stub** for built-in `.wav` sources OR leave a clearly-marked TODO if no `.wav` assets exist yet (see Open Items).
- `Source/PluginProcessor.{h,cpp}` — `juce::AudioProcessor` subclass owning an `AudioProcessorValueTreeState` with all **18 params** (exact IDs/ranges/defaults/skews per parameter-spec.md). `createParameterLayout()` static. Cached `std::atomic<float>*` raw-param pointers. `prepareToPlay`/`releaseResources`/`processBlock` present but **processBlock is silent** (clears buffer, `ScopedNoDenormals`, consumes MIDI without producing audio). `getStateInformation`/`setStateInformation` serialize the APVTS tree **plus** a custom ValueTree child for the loaded-source identity (`"embedded:fire"` default). `setLatencySamples(0)`. `isBusesLayoutSupported` = mono/stereo out, no audio in (synth).
- `Source/PluginEditor.{h,cpp}` — minimal editor for Stage 1. Use `getGenericAudioProcessorEditor`-style or an empty `AudioProcessorEditor` placeholder (the real WebView editor lands Stage 3). Keep it compiling and resizable.
- State persistence: APVTS tree + custom loaded-source-identity child round-trips through `get/setStateInformation`.

## Out of Scope (Stage 1 — deferred)

- **All DSP / audio** → Stage 2 (grain engine, voices, ADSR, window LUTs, read head). `processBlock` stays silent.
- **WebView UI, parameter binding, visualizations** → Stage 3 (relays/attachments, `index.html`, `app.js`, resource provider, `NativeFunction` drag-drop).
- **Sample loading logic** (decode/resample/hot-swap/drag-drop) → Stage 2.3. Stage 1 only reserves the `sourceSample` choice param + custom-state slot + (optionally) the binary-data target scaffold.
- **Presets / preset-manager module** → Stage 4.

## Locked Contract (must match exactly)

**18 APVTS params** (parameter-spec.md is authoritative): `sourceSample` (choice: fire/voice/water/piano, default fire), `grainSize` (2–200 ms, 30, skew~0.4), `density` (1–200 g/s, 40, log skew), `position` (0–100%, 50), `scan` (−200–+200%, 0, bipolar), `freeze` (bool, off), `windowShape` (choice rect/tri/Welch/Gauss/Hann, default Hann), `pitchSpray` (0–12 st, 0), `positionSpray` (0–100%, 0), `scatter` (0–100%, 0), `grainPitch` (−24–+24 st, 0), `panSpray` (0–100%, 0), `velToDensity` (0–100%, 0), `ampAttack` (0–5 s, 0.01, skew~0.35), `ampDecay` (0–5 s, 0.3, skew~0.35), `ampSustain` (0–100%/0–1, 0.8), `ampRelease` (0–5 s, 0.4, skew~0.35), `outputLevel` (−inf–0 dB, 0).

**Non-APVTS:** `Load…` = native-fn action + custom ValueTree state (loaded-source identity). Default custom state = `"embedded:fire"`.

**Engine constants (define now as `static constexpr`, used Stage 2):** polyphony 8, `MaxGrainsPerVoice` 24, global cap 192, `ROOT_NOTE` 60 (C3), source-length cap 10 s, window LUT size 2048.

## Key Decisions (from Stage 0, locked 2026-06-24)

- Template = **O-simpleFM** (silent-shell Stage-1 pattern is identical: synth + WebView2 flags + full APVTS + state persistence). Mirror its file layout and CMake.
- `density` exposed as grains/sec; overlap readout derived (display-only, Stage 3).
- Cross-platform WebView flags set NOW at Foundation (Stage 3 inherits correct config). **Both** Windows flags required or WebView silently blanks (project memory).
- `setLatencySamples(0)` (`getLatencySamples()` non-virtual in JUCE 8 — do NOT override).
- No alloc/locks in `processBlock` (even silent — establish the pattern).

## Success Criteria (Stage 1)

1. `ninja O-simpleGrain_VST3 O-simpleGrain_AU O-simpleGrain_Standalone` builds clean (no errors).
2. Plugin auto-discovered by root CMake (has `plugins/O-simpleGrain/CMakeLists.txt`).
3. All 18 params present in APVTS with correct IDs/ranges/defaults (verifiable via generic editor / pluginval param dump).
4. `auval -a | grep -i simplegrain` shows the AU (after install) — or at minimum the AU validates structurally.
5. State round-trips: set params + loaded-source identity → `getStateInformation` → `setStateInformation` restores them.
6. Loads in a DAW as an instrument, accepts MIDI, **produces silence**, does not crash.
7. `processBlock` allocation-free (silent path).

## Open Items / Risks

- **Embedded `.wav` assets:** built-in sources (fire/voice/water/piano) may not exist as files yet. Stage 1 should NOT block on them. Either (a) add a `juce_add_binary_data` target referencing placeholder/real `.wav`s if present, or (b) leave a clearly-marked `# TODO(Stage 2.3): embed built-in .wav sources` and keep `sourceSample` as a plain choice param for now. Prefer (b) if no assets exist — do not invent binary blobs.
- **`PLUGIN_CODE` uniqueness:** pick a unique 4-char code (e.g. `OsGr`) not colliding with existing suite plugins.
- Mockup deferred to Stage 3 per user decision — APVTS built against the research-locked spec; minor reconciliation risk accepted.
