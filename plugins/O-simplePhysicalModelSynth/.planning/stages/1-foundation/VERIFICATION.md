# Stage 1: Foundation + Shell — Verification

## Verification Date

2026-06-26

## Goal-Backward Analysis

### Original Goal (from CONTEXT.md / PLAN.md)

Silent synth shell that loads, appears as an instrument in the DAW (IS_SYNTH), passes
pluginval (strictness 5+), with the full **17-param APVTS** wired and state persistence
working. **Zero DSP, zero WebView** — note input produces no audio, but causes no crashes.
Render-harness scaffold builds/links under `JUCE_WEB_BROWSER=0` to lock the Stage-2 gate seam.

### Deliverables (from SUMMARY.md + code inspection)

1. `CMakeLists.txt` — `IS_SYNTH` synth target, WebView2 flags, `juce_dsp`, harness option, 4 divergences applied
2. `Source/PluginProcessor.h` — `ParamIDs` namespace (17 IDs), processor class, inlined `createEditor` behind `#if JUCE_WEB_BROWSER` seam
3. `Source/PhysicalModelVoice.h` — header-only silent `PhysicalModelVoice` / `PhysicalModelSound`
4. `Source/PluginProcessor.cpp` — `createParameterLayout` (17 params), ctor/prepare/processBlock, plain-APVTS state round-trip
5. `tests/render-harness/CMakeLists.txt` + `main.cpp` — build/link smoke-stub under `JUCE_WEB_BROWSER=0`

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| Silent synth shell loads as instrument (IS_SYNTH) | ✅ Achieved | `auval -a` → `aumu OsPM OuDv` (AU music device = instrument) |
| Full 17-param APVTS wired | ✅ Achieved | `auval -v` → "17 Global Scope Parameters"; 17 `push_back` in source; 17 `ParamIDs` constants; IDs/ranges/defaults verbatim vs `parameter-spec.md` |
| pluginval passes (strictness 5+) | ✅ Achieved | pluginval `--strictness-level 5` → **SUCCESS** |
| State persistence round-trips | ✅ Achieved | Harness `state-roundtrip` PASS (828 B); plain APVTS XML in `get/setStateInformation` |
| Silent / no crashes on note input | ✅ Achieved | Harness `shell-silent` peak=0.00000000; note-on/off storm produces finite output, voice frees via `clearCurrentNote()` |
| Render-harness scaffold builds/links (`JUCE_WEB_BROWSER=0`) | ✅ Achieved | Harness target builds clean + runs ALL PASS; no editor/WebView symbols in the processor TU |
| Zero drift — percent params 0–100 (D3 hazard) | ✅ Achieved | `percentRange()` = `{0,100,0.01}`; no `unitRange()` on percent params |

## Requirements Verification

**Stage:** stage-1
**Requirements for this stage:** 2 total (2 must, 0 should, 0 nice)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| COMPAT-01: Passes pluginval validation (VST3 and AU) | must | ✅ Complete | pluginval-5 VST3 = SUCCESS; auval AU = VALIDATION SUCCEEDED |
| COMPAT-02: Windows WebView2 flags set | must | ✅ Complete | `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` present in CMakeLists.txt |

**Requirements Summary:**
- ✅ Complete: 2 (COMPAT-01, COMPAT-02)
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 22 (FUNC-*, DSP-*, PERF-01, QUAL-01 → stage-2; UI-* → stage-3; FUNC-07 → stage-4)
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU) | ✅ Pass | Clean incremental rebuild, **0 warnings / 0 errors** |
| AU registration | ✅ Pass | `auval -a` → `aumu OsPM OuDv` (instrument, dev branding) |
| AU validation | ✅ Pass | `auval -v aumu OsPM OuDv` → **AU VALIDATION SUCCEEDED**, 17 Global Scope Parameters |
| pluginval strictness 5 (VST3) | ✅ Pass | **SUCCESS** (0-in / 2-out synth bus) |
| Parameter count (source) | ✅ Pass | 17 `push_back` + 17 `ParamIDs` constants; zero drift vs `parameter-spec.md` |
| WebView2 flags (COMPAT-02) | ✅ Pass | Both flags present in CMakeLists.txt |
| D3 percent-range guard | ✅ Pass | Percent params `{0,100,0.01}`, NOT `unitRange()` 0–1 |
| Render-harness build/link | ✅ Pass | Builds under `-DOUARICON_BUILD_TESTS=ON` (`JUCE_WEB_BROWSER=0`, no editor) |
| Harness run (finite/silent/state) | ✅ Pass | output-finite PASS; shell-silent peak=0.0; state-roundtrip 828 B; ALL PASS exit 0 |

## Human Verification

- [ ] Load `-dev` build in Logic/DAW → confirm it appears in the **instrument** list (not effect)
- [ ] Play notes → confirm silence + no crash + no CPU spike
- [ ] All 17 params visible/automatable in the generic editor; save/reload session restores values

## Issues Found

- None. All Stage-1 exit-gate criteria met on fresh re-run of the automated checks.

## Notes / Carry-forward

- **Gate bypass (benign):** the 0→1 quality gate's `build` check was `--force`-bypassed at ideation
  because Stage 1 *creates* the CMake target (nothing exists to build at ideation). Standard
  new-plugin path; logged to `gate-bypasses.log`.
- **Stage 2 entry:** add the **autocorrelation** pitch probe to the harness at Stage 2.1
  (NOT spectral — the KS loop comb fools a single-bin DFT). Resolved DSP mappings for the 17 params
  are recorded in `parameter-spec.md` §Resolved DSP Mappings.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes

**Blockers (if any):** None
