# Stage 1 (Foundation) — PLAN

**Plugin:** O-simpleFM · **Stage:** 1 Foundation · **Date:** 2026-06-20
**Goal:** Buildable, host-loadable silent synth shell with the full 17-param APVTS and state persistence. No audio, no WebView.

## Tasks

### T1 — CMakeLists.txt
- `juce_add_plugin(O-simpleFM ...)`: `PLUGIN_CODE OSiF`, `FORMATS VST3 AU Standalone`, `PRODUCT_NAME "O-simpleFM${OUARICON_DEV_SUFFIX}"`, `VERSION 1.0.0`, `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`.
- Sources: `Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp` (+ headers).
- Link standard JUCE module set (incl. `juce_dsp`, `juce_gui_extra`); recommended config/lto/warning flags.
- `juce_generate_juce_header(O-simpleFM)` after link.
- Licensing block (compile-flag gated, OFF for dev) — suite standard.
- Compile defs: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`.
- **No** `juce_add_binary_data` / UI resources (Stage 3).

### T2 — PluginProcessor.h / .cpp
- `OSimpleFMAudioProcessor : juce::AudioProcessor`; standard overrides; `acceptsMidi()=true`, `producesMidi()=false`, `isMidiEffect()=false`, `getTailLengthSeconds()=0`.
- Ctor builds APVTS with `createParameterLayout()`; output-only stereo bus.
- `createParameterLayout()` declares all **17 params** (exact IDs/ranges/defaults/skews from table below).
- `processBlock`: `ScopedNoDenormals`, clear buffer (silent), no allocation.
- `getStateInformation`/`setStateInformation`: APVTS value-tree XML round-trip.
- `getAPVTS()` accessor for editor.
- `createEditor()` → `GenericAudioProcessorEditor` (temporary).

### T3 — PluginEditor.h / .cpp
- Minimal: subclass not strictly required — `createEditor()` returns `new juce::GenericAudioProcessorEditor(*this)`. Keep a thin `PluginEditor.{h,cpp}` placeholder for Stage 3 to grow into, OR return Generic directly from processor. **Decision: return GenericAudioProcessorEditor directly from processor; keep PluginEditor.cpp minimal/absent.** (Avoids dead WebView scaffolding now.)
  - To keep the CMake source list stable across stages, create a stub `PluginEditor.h/.cpp` that defines a trivial editor wrapping `GenericAudioProcessorEditor`.

### T4 — Build + validate
- `cmake -B build -G Ninja` (reconfigure to glob new plugin) then `ninja O-simpleFM_VST3 O-simpleFM_AU`.
- Clear AU cache, install per CLAUDE.md, `auval -v aumu OSiF Ouar`.

### T5 — SUMMARY.md
- Record files created, param count, build/auval result.

## Parameter table (the contract — exact)

| ID | Type | Range | Default | Skew/step |
|----|------|-------|---------|-----------|
| `ratio` | Float | 0.5–16.0 | 1.0 | step 0.01, linear |
| `ratioSnap` | Bool | off/on | off (false) | — |
| `modIndex` | Float | 0–20 | 0 | skew 0.3 |
| `feedback` | Float | 0–1 (0–100%) | 0 | skew 0.5 |
| `modFixedMode` | Bool | Ratio/Fixed | Ratio (false) | — |
| `modFixedHz` | Float | 1–8000 Hz | 220 | log skew (~0.25) |
| `modEnvToIndex` | Float | 0–1 (0–100%) | **1.0** | linear |
| `velToIndex` | Float | 0–1 (0–100%) | 0 | linear |
| `modAttack` | Float | 0.001–5 s | 0.01 | skew 0.35 |
| `modDecay` | Float | 0.001–5 s | 0.3 | skew 0.35 |
| `modSustain` | Float | 0–1 | 0.0 | linear |
| `modRelease` | Float | 0.001–5 s | 0.3 | skew 0.35 |
| `ampAttack` | Float | 0.001–5 s | 0.01 | skew 0.35 |
| `ampDecay` | Float | 0.001–5 s | 0.3 | skew 0.35 |
| `ampSustain` | Float | 0–1 | 0.8 | linear |
| `ampRelease` | Float | 0.001–5 s | 0.3 | skew 0.35 |
| `outputLevel` | Float | -60–0 dB | 0 | linear (dB domain) |

> `outputLevel` range: ARCHITECTURE says "−inf–0 dB"; implement as **−60 dB → 0 dB** with −60 treated as silence (−inf is not a usable APVTS bound). Float % params stored 0–1; UI shows ×100 in Stage 3.

## Success criteria (goal-backward)

- [ ] Builds clean (VST3 + AU).
- [ ] `auval -v aumu OSiF Ouar` passes.
- [ ] Loads as instrument in DAW; MIDI accepted; silent (expected).
- [ ] All 17 params visible with correct ranges/defaults in GenericAudioProcessorEditor.
- [ ] Session save/recall preserves param values.
- [ ] No allocation / denormal issues in processBlock (silent path).

## Dependencies
- T1 → T2/T3 (CMake before sources compile) → T4 → T5. T2 and T3 parallel-authorable.
