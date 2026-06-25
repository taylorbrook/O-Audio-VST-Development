# Stage 1 (Foundation) — CONTEXT

**Plugin:** O-simpleSampler
**Stage:** 1 of 4 — Foundation + Shell
**Date:** 2026-06-25
**Source:** Auto-compiled from contracts (BRIEF.md, parameter-spec.md, research/ARCHITECTURE.md, ROADMAP.md, STATUS.md). No interactive discuss session — momentum path; Stage 0 resolved every open question and the 21-param set is research-locked. (Same path O-simpleGrain took at its Stage 1.)

## Stage Goal

A **silent, valid 16-voice synth shell**: builds and loads as an instrument (VST3 + AU + Standalone) on macOS, exposes the full **21-parameter APVTS** (exact IDs/ranges/defaults/skews per `parameter-spec.md`), persists state (incl. the loaded-source identity as a custom ValueTree child), and carries a correct cross-platform WebView/CMake config so Stage 3 inherits it. **No audio yet** (Stage 2), **no WebView UI yet** (Stage 3). The plugin instantiates, shows a minimal placeholder editor, accepts MIDI without crashing, produces silence, and round-trips its parameter + source-identity state.

## In Scope (Stage 1)

- `plugins/O-simpleSampler/CMakeLists.txt` — `juce_add_plugin` with `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; FORMATS `VST3 AU Standalone`; `PLUGIN_CODE OsSm` (unique — see Decisions); `PRODUCT_NAME "O-simpleSampler${OUARICON_DEV_SUFFIX}"`; `VERSION "0.1.0"`; compile defs incl. `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_CURL=0`, `JUCE_VST3_CAN_REPLACE_VST2=0`. Mirror O-simpleGrain's foundation CMake. **Defer** the `juce_add_binary_data` UI-resources block + `ouaricon_add_module` (Stage 3 — no UI files yet) AND the embedded-`.wav` samples target (Stage 2.3 — no blobs yet); leave clearly-marked TODOs documenting the **dual-NAMESPACE split** (`UIBinaryData` + `BinaryData`) so the collision lesson is encoded before the targets exist.
- `Source/PluginProcessor.{h,cpp}` — `juce::AudioProcessor` owning an `AudioProcessorValueTreeState` with all **21 params**. `createParameterLayout()` static. Cached `std::atomic<float>*` raw-param pointers (assigned in ctor; unused while silent). `prepareToPlay`/`releaseResources`/`processBlock` present but **processBlock is silent** (`ScopedNoDenormals`, clears buffer, consumes MIDI, allocation-free). `getStateInformation`/`setStateInformation` serialize the APVTS tree **plus** a custom `SOURCE/identity` child (default `embedded:piano`). `setLatencySamples(0)`. `isBusesLayoutSupported` = mono/stereo out, no input bus (synth).
- `Source/PluginEditor.{h,cpp}` — minimal `juce::AudioProcessorEditor` placeholder painting an "O-simpleSampler — Stage 1 shell" label (visibly alive). The real WebView editor lands Stage 3.
- State persistence: APVTS tree + custom source-identity child round-trips through `get/setStateInformation`.

## Out of Scope (Stage 1 — deferred)

- **All DSP / audio** → Stage 2 (sampler voice + Repitch read head, region, amp ADSR, built-in decode in 2.1; Stretch/loop/Vintage/filter in 2.2; AA-hardening/viz/voice-stealing/render-harness in 2.3). `processBlock` stays silent.
- **WebView UI, parameter binding, waveform editor, visualizations** → Stage 3 (relays/attachments, `index.html`, `app.js`, resource provider, drag-drop native fns).
- **Sample loading logic** (decode/resample/hot-swap/drag-drop) + `.wav` embedding → Stage 2.3 + 3.1. Stage 1 only reserves the `sourceSample` choice param + the custom-state slot, and documents the dual-NAMESPACE targets as TODOs.
- **Presets / concept-preset tour** → Stage 3.3 / Stage 4.

## Locked Contract (must match `parameter-spec.md` exactly — 21 params)

`sourceSample` (Choice: piano/vocal/flute/vinyl, default 0) · `start` (0–100%, 0) · `end` (0–100%, 100) · `loopMode` (Choice off/forward/ping-pong, 0) · `loopStart` (0–100%, 0) · `loopEnd` (0–100%, 100) · `loopCrossfade` (0–500 ms, 10, skew≈0.4) · `reverse` (bool, off) · `rootKey` (Int 0–127, 60) · `pitchMode` (Choice Repitch/Stretch, 0) · `tune` (Int −24–+24, 0) · `fine` (−100–+100 cents, 0) · `vintage` (0–100%, 0) · `filterCutoff` (20–20000 Hz, 20000, log skew-for-centre≈1 kHz) · `filterResonance` (0–100%, 0) · `ampAttack` (0–5 s, 0.005, skew≈0.35) · `ampDecay` (0–5 s, 0.3, skew≈0.35) · `ampSustain` (0–1, 1.0) · `ampRelease` (0–5 s, 0.2, skew≈0.35) · `velToAmp` (0–100%, 50) · `outputLevel` (−60–0 dB, 0).

**Non-APVTS:** `Load…` = native-fn action + custom `SOURCE/identity` ValueTree state; default `embedded:piano`.

**Engine constants (declare as `static constexpr` now; consumed Stage 2):** `kMaxVoices = 16`, `kMaxGrainsPerVoice = 4` (Stretch pool), `kRootNote = 60`, `kMaxSourceSeconds = 30`, `kStretchGrainMs = 60`, `kNumBuiltIns = 4`.

## Key Decisions (from Stage 0, locked 2026-06-25)

- Template = **O-simpleGrain foundation** (silent-shell Stage-1 pattern is identical: synth + WebView2 flags + full APVTS + custom source-identity state). Mirror its file layout, CMake, and state-persistence idiom.
- **PLUGIN_CODE `OsSm`** — unique across the suite (`OSiS` is taken by O-simpleSubtractive; follows O-simpleGrain's `OsGr` "Os" convention).
- Cross-platform WebView flags set NOW at Foundation so Stage 3 inherits a correct config. **Both** Windows flags required or WebView silently blanks (project memory).
- `setLatencySamples(0)` — `getLatencySamples()` is non-virtual in JUCE 8; do NOT override.
- No alloc/locks in `processBlock` (even silent — establish the pattern).
- Percent params stored 0–100; `ampSustain` stored 0–1 (UI ×100) so it feeds `juce::ADSR` directly. `rootKey`/`tune` are `AudioParameterInt`.
- `sourceSample` is a plain `AudioParameterChoice` of built-in names; load-your-own is custom state (NOT a 5th choice).

## Success Criteria (Stage 1)

1. `ninja O-simpleSampler_VST3 O-simpleSampler_AU O-simpleSampler_Standalone` builds clean (no errors).
2. Plugin auto-discovered by root CMake (`file(GLOB plugins/*)` → has `plugins/O-simpleSampler/CMakeLists.txt`).
3. All 21 params present in APVTS with correct IDs/ranges/defaults (pluginval param dump / generic editor).
4. AU validates structurally (`auval -a | grep -i simplesampler` after install) / VST3 loads.
5. State round-trips: params + `currentSourceIdentity` → `getStateInformation` → `setStateInformation` restores both.
6. Loads in a DAW as an instrument, accepts MIDI, **produces silence**, does not crash.
7. `processBlock` allocation-free (silent path).

## Open Items / Risks

- **Embedded `.wav` assets + UI files do not exist yet.** Stage 1 must NOT reference them in `juce_add_binary_data` or the build breaks at configure (the O-simpleGrain foundation deferred both with TODOs — same here). `sourceSample` stays a plain choice param; identity round-trips as custom state.
- **Built-in names (piano/vocal/flute/vinyl) are a working placeholder.** ARCHITECTURE specifies ≈4–6 curated found-sounds with illustrative names; the concrete set + per-sample default roots finalize at Stage-2 asset sourcing. Trivially renameable then; not a foundation blocker.
- **Dual-NAMESPACE collision** (samples target `BinaryData` vs UI target `UIBinaryData`) — documented as TODO comments now so the lesson is in place before the targets are added.
