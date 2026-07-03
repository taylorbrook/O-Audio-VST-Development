# Stage 1 (Foundation + Shell) — Execution Summary

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 1 of 4 (Foundation + Shell)
**Phase:** execute
**Date:** 2026-06-26
**Result:** ✅ Complete — silent synth shell builds, installs, and validates

---

## What was built

A silent synth shell: CMake target + full **17-param APVTS** + 16-voice `juce::Synthesiser`
with a header-only silent `PhysicalModelVoice` + plain-APVTS state round-trip + render-harness
scaffold. **Zero DSP, zero WebView** (those are Stages 2 and 3). Built from the O-simpleFM
template with the 4 planned divergences applied.

## Files created (6)

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Plugin target — `IS_SYNTH`, WebView2 flags, `juce_dsp`, harness option |
| `Source/PluginProcessor.h` | `ParamIDs` namespace, processor class, inlined `createEditor` seam |
| `Source/PhysicalModelVoice.h` | Header-only silent voice + sound (`PhysicalModelVoice`/`PhysicalModelSound`) |
| `Source/PluginProcessor.cpp` | `createParameterLayout` (17 params), ctor/prepare/processBlock, state, factory |
| `tests/render-harness/CMakeLists.txt` | Console-app harness (`JUCE_WEB_BROWSER=0`, no editor, no UIResources) |
| `tests/render-harness/main.cpp` | Stage-1 stub: note-on → render → assert finite + silent + state round-trip |

**Deliberately NOT created:** `Source/PluginEditor.{h,cpp}` (createEditor inlined behind the
`#if JUCE_WEB_BROWSER` seam; real WebView editor is Stage 3), `Source/ui/`, `juce_add_binary_data`,
`FactoryPresets.*`, preset-manager module (Stage 4). Root `CMakeLists.txt` needed **no** edit —
auto-discovers via `file(GLOB plugins/*)`.

## Key implementation facts

- **PLUGIN_CODE `OsPM`** (collision-checked free); `PRODUCT_NAME "O-simplePhysicalModelSynth${OUARICON_DEV_SUFFIX}"`,
  `VERSION 1.0.0`. Registers as **`aumu OsPM OuDv`** (AU music device = instrument, dev branding).
- **17 params, D3 hazard cleared:** the 9 percent params use `NormalisableRange<float>{0.0f, 100.0f, 0.01f}`
  — **NOT** O-simpleFM's `unitRange()` 0–1. `coarseTune` `AudioParameterInt(-24,24,0)`; `fineTune`
  float −100…+100/0; `ampAttack` 0–2/0.001 + `ampRelease` 0–5/0.2 (mild 0.35 perceptual skew,
  endpoints/defaults intact); `outputLevel` −60…0/−6 with `.withLabel("dB")`. Every param uses
  `juce::ParameterID{ id, 1 }`. auval confirms exactly **17 Global Scope Parameters**.
- **Divergences applied:** D1 no binary-data/WebView/ui; D2 plain APVTS XML round-trip (no preset
  manager); D3 0–100 percent ranges; D4 no oversampler, `setLatencySamples(0)`. `juce_dsp` linked
  now; `juce_generate_juce_header` after `target_link_libraries`.
- **Voice/synth:** output-only stereo bus; 16 `PhysicalModelVoice` + one `PhysicalModelSound`;
  note-stealing on; `renderNextBlock` no-op (silent); `stopNote` calls `clearCurrentNote()` so
  voices free under pluginval note storms. Non-virtual `prepareToPlay` dispatched from the processor.
- **createEditor seam** inlined in `PluginProcessor.h` behind `#if JUCE_WEB_BROWSER / #else / #endif`
  (both branches `GenericAudioProcessorEditor` at Stage 1). Harness compiles the processor TU with
  no editor/WebView symbols under `JUCE_WEB_BROWSER=0`.

## Verification results (smoke — formal checks are the verify phase)

| Check | Result |
|-------|--------|
| VST3 + AU build + ad-hoc sign | ✅ clean |
| AU registration (`auval -a`) | ✅ `aumu OsPM OuDv` — instrument |
| `auval -v aumu OsPM OuDv` | ✅ **AU VALIDATION SUCCEEDED**, 17 params all PASS |
| pluginval strictness 5 (VST3) | ✅ **SUCCESS** (0 in / 2 out bus) |
| Render-harness build/link (`-DOUARICON_BUILD_TESTS=ON`) | ✅ builds & links under `JUCE_WEB_BROWSER=0` |
| Harness stub run | ✅ output-finite, shell-silent (peak=0.0), state-roundtrip (828 B) |
| Installed (VST3 + AU) | ✅ via `build-and-install.sh` (no orphan variants to sweep) |

## Stage-1 exit gate (from PLAN.md) — status

- [x] VST3 + AU build cleanly; appears in DAW **instrument** list (IS_SYNTH / `aumu`)
- [x] pluginval passes at **strictness 5+** (COMPAT-01)
- [x] All **17 params** visible (auval: 17 Global Scope Parameters); IDs/ranges/defaults match `parameter-spec.md`
- [x] State **save/restore round-trips** (plain APVTS — harness state-roundtrip PASS)
- [x] **Silent** — note input produces no audio (peak=0.0), no crashes under note storms
- [x] WebView2 flags present: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (COMPAT-02)
- [x] Render-harness **builds & links** (`JUCE_WEB_BROWSER=0`, no editor)
- [x] Percent params stored **0–100** (NOT 0–1) — D3 copy hazard did not slip through

## Notes / carry-forward

- **Gate bypass:** the 0→1 quality gate was `--force`-bypassed because its `build` check is
  structurally premature (Stage 1 *creates* the CMake target; nothing exists to build at ideation).
  Logged to `gate-bypasses.log`. This is the standard new-plugin path.
- LSP diagnostics on the headers (`JuceHeader.h not found` + cascading `juce` undeclared) were
  pre-configure false positives — `JuceHeader.h` is generated by `juce_generate_juce_header` at
  configure time. They resolved on build; VST3/AU/harness all compile clean.
- Harness `JucePlugin_ManufacturerCode` is a cosmetic placeholder (`0x4f756172`) for the console
  app only; the real VST3/AU derive codes from `OUARICON_MANUFACTURER_CODE` (dev `OuDv`).
- **Stage 2 (DSP) entry:** the autocorrelation pitch probe is added to the harness at Stage 2.1
  (NOT spectral — the KS loop comb fools single-bin DFT). Resolved DSP mappings for the 17 params
  are recorded in `parameter-spec.md` §Resolved DSP Mappings.

---
*Execute phase complete. Next: verify phase → `/plugin-verify O-simplePhysicalModelSynth 1-foundation`.*
