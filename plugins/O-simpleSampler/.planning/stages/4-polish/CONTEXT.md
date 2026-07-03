# Stage 4: Polish — Context

## Discussion Summary

**Date:** 2026-06-26
**Participants:** User, Claude

Final stage. Stage 3 (GUI) verified complete (PASS 7/7 stage-3 reqs; fresh auval + pluginval@5 SUCCESS on VST3+AU; 21-param APVTS contract frozen). Stage 4 closes out v1.0: the deferred content (built-in found-sounds + preset param values), the documented carry-ins (render-harness re-run, RT-safety backlog, Windows verify), and the release sweep (validation + CHANGELOG v1.0.0 + local install).

## Requirements Confirmed

Stage-4 traceability = "COMPAT-* re-verify, preset tour content, validation sweep, all remaining."

- **FUNC-02 (must) — curated built-in found-sounds:** **User will provide the audio files.** I embed them as a second `juce_add_binary_data` target (`NAMESPACE BinaryData`, distinct from `UIBinaryData`), resample off-thread to engine rate, and seed each sample's default Root Key. Until assets are provided, `piano.wav` remains the sole built-in (engine already plays it). This is a **content dependency on the user** (see Open Questions).
- **FUNC-07 (should) — concept preset tour content:** author the parameter values for the 7 already-hooked named presets (Raw One-Shot, Tuned Across the Keyboard, Looped Pad, Reversed Swell, Repitch vs Stretch A/B, SP-1200 Crunch, Filtered & Enveloped). Hook (`applyFactoryPreset` + 7 buttons + `PRESET_LESSONS`) shipped in Stage 3.3; only the values remain. Presets are authored to read on whatever source is loaded (source-agnostic param settings), defaulting to the active built-in.
- **COMPAT-01 (must) — pluginval (VST3 + AU):** re-verify in the final sweep.
- **COMPAT-02 (must) — Windows WebView2 + dual NAMESPACE:** wiring verified in code; **runtime verification is the user's** on a Windows host/DAW (not CI).
- **QUAL-01 (must) — no audio artifacts:** re-assert via the render-harness (aliasing / loop-seam click / Vintage-clean-at-zero / RT) after the WebView-editor break is fixed.
- **All remaining FUNC/DSP/PERF** carried by stage-2/3 verification; re-confirmed by the final validation sweep.

## Constraints Identified

- **Render-harness is broken until fixed FIRST.** It compiles `PluginEditor.cpp` under `JUCE_WEB_BROWSER=0`; the Stage-3 editor gained WebView types, so the harness goes un-buildable. The `#if JUCE_WEB_BROWSER` guard on `createEditor` + dropping `PluginEditor.cpp` from the harness sources is already prepped — re-run the harness at the START of Stage 4 to confirm, before any DSP/QUAL re-assertion. (Project pattern: `render_harness_breaks_on_webview_editor`.)
- **21-param APVTS contract is FROZEN.** No parameter add/remove/rename in Stage 4. Preset values and built-in content must work within the existing 21 params.
- **Dual binary-data NAMESPACE split.** New samples go in the `NAMESPACE BinaryData` target; UI stays `UIBinaryData`. Distinct NAMESPACE *and* HEADER_NAME (O-simpleGrain Stage-3.1 collision lesson).
- **Sample loading off the audio thread** (PERF-01) — built-in decode/resample/hot-swap uses the established atomic-publish pattern; never blocks `processBlock`.
- **macOS install hygiene:** sweep both `-dev` and unsuffixed variants before install (project AU-registry shadowing lesson).

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Built-in found-sound set (FUNC-02) | **User provides the audio files**; Claude embeds, resamples, seeds default roots | User has the curated/license-clean material; piano-only weakens the `must`. Engine + loader already proven on piano. |
| Built-in count / character | Defer exact set to user delivery; target the brief's 4 categories (vocal fragment, instrument hit, found texture, percussive object) + piano | Matches the brief; final count is whatever the user supplies. |
| RT-safety backlog (3 items) | **Address all 3 in Stage 4** | Polish stage is the place; closes documented gaps before v1.0. Items: (1) message-thread reclaim queue for the source-swap `shared_ptr` free, (2) deprecated `std::atomic_load/store(shared_ptr)` under C++20, (3) `setValueNotifyingHost`-in-prepare advisability. |
| Windows runtime verification (COMPAT-02) | **User tests on a Windows host/DAW** | Wiring is in code; user has a Windows machine to confirm the UI renders + plays. Not gating CI for v1.0. |
| Stage-4 finish line | **Local install (-dev) + CHANGELOG v1.0.0** | Full validation sweep + render-harness + local install; stop short of a public GitHub release/packaged installer. |
| Preset values authoring | Source-agnostic param settings, default to active built-in | Presets isolate one *move* each; should read on any loaded source, not depend on a specific sample. |

## Open Questions

- **[USER DEPENDENCY] Built-in audio files (FUNC-02):** User to deliver the found-sound `.wav`/audio files (and ideally a suggested Root Key per sample). Drop location: `plugins/O-simpleSampler/Source/samples/`. Common formats fine (JUCE readers handle wav/aiff/flac; will normalize/resample to engine rate, ≤30 s cap). **Until delivered, execute proceeds on piano-only and the CHANGELOG notes the curated set.** Execute can implement the multi-built-in embedding + selector wiring against whatever set exists at execute time.
- **Preset → built-in coupling:** if a preset reads best on a specific found-sound (e.g. "Reversed Swell" on a sustained texture), should the preset also select that source? Default: presets set params only and leave the source as-is. Revisit if a delivered sound makes a specific pairing obviously better.

## Next Phase

Ready for: **research** phase — confirm the render-harness fix specifics, the multi-built-in embedding/selector pattern (mine O-simpleGrain), the 3 RT-safety remediations (message-thread reclaim queue + non-deprecated shared_ptr atomics), and preset param-value derivation. Then plan → execute → verify → install.
