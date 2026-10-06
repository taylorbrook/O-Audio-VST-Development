# Stage 1: Foundation — SUMMARY

**Plugin:** O-simpleWavetable · **Stage:** 1 (Foundation) · **Date:** 2026-10-05
**Agent:** foundation-shell-agent (Tasks 2–9) · **Orchestrator:** plugin-workflow (manual, `/plugin-execute`), Tasks 1, 10–13, 15
**Result:** Silent, buildable, host-validated 16-voice wavetable synth shell. It has the full 21-param APVTS, an APVTS-XML state with `uiLanguage` and a strip-on-load `IMPORTED_BANK` stub, zero latency, and a generic editor under the final editor class name. VST3, AU and Standalone all build clean, and auval, pluginval (VST3 + AU) and 9/9 state probes pass.

---

## Files created (7)

| File | Lines | Content |
|------|------:|---------|
| `CMakeLists.txt` | 80 | `juce_add_plugin(O-simpleWavetable)` with `PLUGIN_CODE OSiW`, `VERSION "0.1.0"` and `OUARICON_*` branding vars. Flags: `IS_SYNTH`, `NEEDS_MIDI_INPUT`, `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`. Formats: VST3/AU/Standalone. Also `ouaricon_add_module(webview-drop-streaming)`, 13 juce modules, the 4 WebView defines, `juce_generate_juce_header`, and an `OUARICON_BUILD_TESTS` block (param-dump + state-check). |
| `Source/WtVoice.h` | 65 | `WtSound` (all notes and channels) and `WtVoice`: silent `renderNextBlock`, `pitchWheelPos` seeded on `startNote` (D2 seam). |
| `Source/PluginProcessor.h` | 158 | `OSimpleWavetable::ParamIDs` (21 IDs plus an `all` array in spec order). The processor exposes the mockup-contract API: `getAPVTS`, `handleUiMidi`, `uiLanguage`, `languageCode`/`languageIndex`. Its synth has 16 voices plus a `MidiMessageCollector`, and the `IMPORTED_BANK` helpers live here too. |
| `Source/PluginProcessor.cpp` | 298 | Contains the 21-param layout and a silent RT-safe `processBlock` with `setLatencySamples(0)`. State save/restore strips every `IMPORTED_BANK` child before `replaceState` and from `copyState()`. The editor include is guarded (`#if JUCE_WEB_BROWSER`, else the generic editor). |
| `Source/PluginEditor.h` / `.cpp` | 52 / 55 | `OSimpleWavetableAudioProcessorEditor` wraps a `GenericAudioProcessorEditor` at 420×640. Stage 3 drops in `mockups/v1-PluginEditor.*`. |
| `tests/state-check/main.cpp` | 661 | Console probe driver, P0–P8. |

## Parameter layout: 21 (13 Float, 6 Choice, 2 Bool), spec order

`bank, position, interp, bandlimit, bit_depth, lfo_rate, lfo_sync, lfo_div, lfo_shape, lfo_depth, menv_attack, menv_decay, menv_sustain, menv_release, env_amount, amp_attack, amp_decay, amp_sustain, amp_release, voice_mode, output_level`

- `bank` choices 0/1 are built as `CharPointer_UTF8 ("Sine \xE2\x86\x92 Saw/Square")`; Imported is index 5.
- `bit_depth` is Full,16..3 (mid-rise, D3).
- `lfo_div` defaults to 1/1 (D5).
- `output_level` shows "-inf" at its −60 dB floor.
- Every range uses only the 4-arg `NormalisableRange`.
- The layout asserts both `jassert (params.size() == 21)` and `== ParamIDs::all.size()`.

## RESEARCH amendments applied

- All sources are ASCII-only, comments included. RESEARCH snippets had non-ASCII comment characters.
- Uses a literal-21 jassert, not 22 (ROADMAP C1).
- `IMPORTED_BANK`: Stage 1 writes no child. On restore, the child is copied out, then **every** copy is stripped before `replaceState` and again on save (C2/C3).
- `uiLanguage` is persisted as a code string, and restore is gated on `!isVoid()` (`isInt` count = 0).
- Probes build in Debug so jasserts are live (P-A).

## Deviations from PLAN.md

1. **Bank `StringArray`:** built with `add()` calls instead of a mixed braced initializer. Contents and order are unchanged.
2. **Layout ID access:** `createParameterLayout` uses `namespace ids = OSimpleWavetable::ParamIDs;` instead of a using-directive. This avoids class-scope lookup hijacking IDs such as `position`.
3. **Editor ctor:** calls `juce::ignoreUnused (processorRef)` to stay clean under `-Wunused-private-field`.
4. **state-check:** uses `juce::exactlyEqual` because `-Wfloat-equal` is on. In P2, the "de" case starts at `uiLanguage = 2`, so a skipped restore gets caught.
5. **Orchestrator: shared-build dependency fix.** The first `cmake -B build` configure failed in a foreign FetchContent dependency. `umappp` pulls `aarand` at `master`, upstream force-pushed master, and the cached `build/_deps/aarand-src` rebase conflicted on `.github/workflows/run-tests.yaml`. The old and new commits differ **only** in that CI yaml, and the headers are identical. Fixed with `git reset --hard origin/master` inside the build cache (not repo source). Configure then succeeded.
6. **auval warnings: 2 instead of the 1 the plan allowed.** "Parameter did not retain default value when set" appears on `lfo_rate` (0.328712) and `amp_attack` (0.082435). Both are skewed params, which is the known-benign class (O-simpleAdditive Stage 1). Recorded, not failed.

## Build/validation evidence

**Static gates (Task 10)**
- All Task 2–8 grep targets were met:
  - OSiW and 0.1.0 present once each.
  - 0 matches for `PLUGIN_VERSION`, `OuAu`/`OuDv`, `juce_add_binary_data`, lambda ranges, `isInt` and `withInput`.
  - 21 add-call sites.
  - Editor include is guarded.
  - Call order is correct.
- The ASCII gate came back empty.

**Build (Task 11)**
- Configure succeeded after fix 5 above.
- `build-and-install.sh` finished OK in 26 s and installed `O-simpleWavetable-dev.{vst3,component}`.
- The Standalone (`O-simpleWavetable-dev.app`) built.
- 0 warnings or errors from `plugins/O-simpleWavetable/{Source,tests}`.
- AU Info.plist: version = **256** (0.1.0), triple `aumu/OSiW/OuDv`.
- The drop JS is copied and gitignored.

**Probes (Task 12, Debug, out-of-repo)**

```
PASS P0 param-contract          PASS P5 non-default-params
PASS P1 uiLanguage-roundtrip    PASS P6 silent-render
PASS P2 uiLanguage-absent-unknown  PASS P7 shell-identity
PASS P3 imported-bank-tolerance PASS P8 hostile-blobs
PASS P4 imported-bank-two-reopen
exit=0   JUCE Assertion failure: 0 (state-check), 0 (param-dump)
```

- param-dump has 21 rows, IDs in spec order.
- The label, min-text, max-text and default-text columns match the PLAN table exactly. Examples: `Sine → Saw`, `Full`, `1/1`, `Triangle`, `-inf`/`6.0`/`-6.0`.
- UTF-8 arrow bytes are intact.

**Host validation (Task 13)**
- `auval -v aumu OSiW OuDv`: `AU VALIDATION SUCCEEDED`, `21 Global Scope Parameters`; Latency PASS and Test MIDI PASS. The two benign warnings are deviation 6.
- Negative control `auval -v aumu ZZZZ OuDv` returns "didn't find the component".
- pluginval VST3 (strictness 10, GUI on): exit 0, SUCCESS, 0 FAILED.
- pluginval AU (strictness 10): exit 0, SUCCESS, 0 FAILED.

## Pending (verify phase)

- Task 14: DAW/Standalone smoke by Taylor. Logic AU and Reaper/Live VST3 should load as an instrument and show 21 rows with correct text. MIDI should play silently, and a project save/reopen should restore values.
