# Stage 3 (GUI) — SUMMARY

**Mode:** express / direct integration (no separate mockup loop). Ouaricon-Naturalist
WebView UI built from scratch and wired to the frozen Stage 2 DSP.

**Result:** Builds VST3 + AU + Standalone clean; AU VALIDATION SUCCEEDED; render-harness
DSP gate still 5/5; Standalone launches with no WebView/JS errors.

---

## Files created

- `Source/ui/public/index.html` — single page: header, spectrum + scope viz panel,
  live routing diagram (SVG), four control groups, 2 toggles, preset-tour bar, tooltip node.
- `Source/ui/public/css/styles.css` — full ouaricon-naturalist system: aged-paper bg +
  paper-fiber texture, Garamond stack, 10-segment seed-knob conic-gradient, green botanical
  toggles with fleuron corners, dark viz canvases, DPR-ready canvas sizing (explicit
  width/height — no viewport units, no replaced-element trap), botanical overlay (right
  side, 0.16 opacity, `pointer-events:none`).
- `Source/ui/public/js/app.js` — binds all 17 controls two-way, value readouts (range/skew
  aware), relative vertical-drag knobs (+ wheel fine-adjust), tooltips, live routing diagram,
  5-preset tour, DPR-aware spectrum/scope canvas drawing.
- `Source/ui/public/js/juce/index.js` — copied verbatim from O-Prism (JUCE 8 JS bridge).
- `Source/ui/public/js/juce/check_native_interop.js` — copied verbatim from O-Prism.
- `Source/ui/public/img/insects.png` — botanical overlay asset (see Theme below).

## Files rewritten

- `Source/PluginEditor.h` — removed `GenericAudioProcessorEditor`; added relay/WebView/
  attachment members in the correct order (relays -> webView -> attachments), `getResource`,
  kept `FmVizAnalyzer` + `juce::Timer`.
- `Source/PluginEditor.cpp` — relay creation, WebView options (native integration +
  keep-loaded-when-hidden + resource provider + `#if JUCE_WINDOWS` user-data-folder),
  attachments (3-arg, `nullptr` undoManager), `goToURL(getResourceProviderRoot())`,
  `getResource` (bare-path equality), timer emits `spectrumUpdate` + `scopeUpdate`.

## Files edited

- `CMakeLists.txt` — added `juce_add_binary_data(O-simpleFM_UIResources ...)` (6 sources)
  and linked it into `O-simpleFM`. WebView2 flags + `JUCE_WEB_BROWSER=1` were already set
  at Foundation (untouched).
- `tests/render-harness/CMakeLists.txt` — the harness compiles `PluginEditor.cpp` so
  `createEditor()` resolves at link time; with the editor now a WebView, flipped
  `JUCE_WEB_BROWSER=0 -> 1` and linked `O-simpleFM_UIResources` (supplies `BinaryData::*`).
  No DSP source touched — harness still 5/5.

## Untouched (DSP frozen)

`PluginProcessor.{h,cpp}`, `Operator.h`, `FMVoice.h`, `FmVizAnalyzer.h`, `tests/render-harness/main.cpp`.

---

## 17 control -> parameter map

| UI control            | APVTS id        | Type   | Range (skew)              | Default | Readout |
|-----------------------|-----------------|--------|---------------------------|---------|---------|
| Ratio C:M (knob)      | `ratio`         | float  | 0.5-16 (lin)              | 1.0     | `x.xx : 1` |
| Mod Index (knob)      | `modIndex`      | float  | 0-20 (skew 0.3)           | 0.0     | `x.xx` |
| Feedback (knob)       | `feedback`      | float  | 0-1 (skew 0.5)            | 0.0     | `%` |
| Fixed Hz (knob)       | `modFixedHz`    | float  | 1-8000 (skew 0.25)        | 220     | `Hz`/`kHz` |
| Env->Index (knob)     | `modEnvToIndex` | float  | 0-1 (lin)                 | 1.0     | `%` |
| Vel->Index (knob)     | `velToIndex`    | float  | 0-1 (lin)                 | 0.0     | `%` |
| Mod Attack (knob)     | `modAttack`     | float  | 0.001-5 s (skew 0.35)     | 0.01    | `ms`/`s` |
| Mod Decay (knob)      | `modDecay`      | float  | 0.001-5 s (skew 0.35)     | 0.3     | `ms`/`s` |
| Mod Sustain (knob)    | `modSustain`    | float  | 0-1 (lin)                 | 0.0     | `%` |
| Mod Release (knob)    | `modRelease`    | float  | 0.001-5 s (skew 0.35)     | 0.3     | `ms`/`s` |
| Amp Attack (knob)     | `ampAttack`     | float  | 0.001-5 s (skew 0.35)     | 0.01    | `ms`/`s` |
| Amp Decay (knob)      | `ampDecay`      | float  | 0.001-5 s (skew 0.35)     | 0.3     | `ms`/`s` |
| Amp Sustain (knob)    | `ampSustain`    | float  | 0-1 (lin)                 | 0.8     | `%` |
| Amp Release (knob)    | `ampRelease`    | float  | 0.001-5 s (skew 0.35)     | 0.3     | `ms`/`s` |
| Output Level (knob)   | `outputLevel`   | float  | -60-0 dB (lin)            | 0.0     | `dB` |
| Ratio Snap (toggle)   | `ratioSnap`     | bool   | —                         | false   | active class |
| Fixed Mode (toggle)   | `modFixedMode`  | bool   | —                         | false   | active class |

Knobs use relative vertical drag (220 px = full sweep) + mouse-wheel fine adjust, with
`sliderDragStarted/Ended` bracketing for host gesture/automation recording. Value readouts
invert each param's real `NormalisableRange` (start/end/skew read from `state.properties`).

## Viz event contract (C++ -> JS, 30 Hz)

Emitted from `OSimpleFMAudioProcessorEditor::timerCallback()` AFTER the existing
`vizAnalyzer.process(...)`, via `webView->emitEventIfBrowserIsVisible(...)`:

| Event            | Payload                              | JS receiver |
|------------------|--------------------------------------|-------------|
| `spectrumUpdate` | `var` array, 256 floats, dB ~[-100,0], log-freq (baked by analyzer) | `drawSpectrum(arr)` — bar per bin, height = (dB+100)/100 |
| `scopeUpdate`    | `var` array, 128 floats, [-1,1]      | `drawScope(arr)` — polyline |

JS subscribes on the low-level backend: `window.__JUCE__.backend.addEventListener("spectrumUpdate"/"scopeUpdate", ...)`
(NOT `Juce.*` — the namespace split is the documented project gotcha).

## Pedagogical layer

- **Routing diagram (SVG):** MOD -> CAR arrow thickness scales with Mod Index; MOD
  self-feedback arc thickness/opacity scale with Feedback; live `ratio : 1 . I=...` readout.
  Updated from `ratio`/`modIndex`/`feedback` `valueChangedEvent`.
- **Tooltips:** every knob + both toggles + the routing panel carry `data-tip`; floating
  dark tooltip with plain-language copy (e.g. integer ratio = harmonic, irrational = bell).
- **Preset tour (5):** E-Piano, Tubular Bell, Brass, Clarinet, Clang Bell. Each writes a
  full scaled-value snapshot through the slider/toggle states (proper scaled->normalised
  inversion via live `properties`) and shows a one-line concept caption. Each isolates one
  idea: harmonic vs inharmonic ratio, env->index evolution, feedback smear, odd-harmonic
  (2:1 clarinet), dense clang.

## Theme / botanical asset choice

**Aesthetic:** Ouaricon Audio Naturalist (`ouaricon-naturalist-001`) — aged paper, Garamond,
seed cross-section knobs, green toggles, fleurons.

**Overlay:** `insects.png` (reused from O-Chorus, 634x868 transparent PNG). Rationale: the
aesthetic guide maps insects to "buzzing synths / clear structure" — the perfect visual
metaphor for FM's buzzing sidebands. Placed right-side, 0.16 opacity, sepia-toned,
`pointer-events:none`, leaving the left ~65% clear for controls + viz. No new asset invented.

## Build / validation results (macOS)

- `cmake --build ... O-simpleFM_VST3 O-simpleFM_AU O-simpleFM_Standalone` -> all linked clean.
- BinaryData embedded (verified in generated `BinaryData.h`): `index_html` 9750 B,
  `styles_css` 13486 B, `app_js` 19171 B, `index_js` 17959 B, `check_native_interop_js`
  4376 B, `insects_png` 477572 B.
- Installed via `scripts/build-and-install.sh O-simpleFM` (cache-clear + dual-variant sweep):
  VST3 4.7 M, AU 4.6 M.
- `auval -v aumu OSiF OuDv` -> AU VALIDATION SUCCEEDED (dev-suffix manufacturer = `OuDv`).
- Render-harness (`-DOUARICON_BUILD_TESTS=ON`) rebuilt with the new editor and run ->
  ALL PASS — 5/5 (makes-sound, pitch, index->sidebands, carrier-null@2.405, feedback-stable).
- Standalone launched headless: process stayed alive, zero WebKit/console errors, no
  "Frame load interrupted", no ReferenceError in system log, clean shutdown — strong
  signal the WebView rendered (blank/missing-resource cases log loudly).

## Deferred / needs manual verification

- **Human DAW visual check:** screenshot capture is unavailable in this headless shell
  (no Screen-Recording permission). The error-free Standalone run is strong evidence the
  UI renders, but a human should open the Standalone (`/show-standalone O-simpleFM`) or a
  DAW to confirm: spectrum sidebands bloom as Mod Index rises, Ratio snaps
  harmonic<->inharmonic, Feedback smears, scope morphs live, all knobs/toggles two-way, and
  the 5 presets load + sound as captioned.
- **Windows:** cross-platform wiring is in place (`NEEDS_WEBVIEW2`,
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `#if JUCE_WINDOWS` user-data-folder), but
  not built/run on Windows in this session.
- v1.0 out-of-scope (unchanged): non-sine operators, fine detune, master tune, LFO/vibrato,
  A/B compare.
