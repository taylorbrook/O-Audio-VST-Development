# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-10-06
**Participants:** User, Claude
**Source contracts:** BRIEF.md (§UI, lesson row, keyboard, tooltips), ROADMAP.md §Stage 3 (3 phases), REQUIREMENTS.md (UI-01..06, PERF-03), parameter-spec.md (21 params, locked), research/ARCHITECTURE.md (row 17, §Visualization Data Path, Amendments 12–13), mockups/v1-* + v1-integration-checklist.md, stages/2-dsp/VERIFICATION.md.

Mockup v1 is finalized with Phase B scaffolding: layout, visual style, control mapping, i18n table (en / fr / zh-Hans) and the C++ → page event contract are already decided. This discussion covered only what those artifacts left open: execution cadence, the lesson buttons, and where the hands-on happens.

## Requirements Confirmed

- **Phase 3.1, Layout and Bindings:** `mockups/v1-ui.html` → `Source/ui/public/index.html`, `v1-i18n.js` → `js/i18n.js`, JUCE 8 frontend + `check_native_interop.js` + `img/insects.png` from O-simpleAdditive, EB Garamond embedded from `modules/ui/eb-garamond`, `webview-drop-streaming` via `ouaricon_add_module`. Editor from the `v1-PluginEditor.{h,cpp}` templates (adapt, don't paste): member order 21 relays → `webView` → 21 attachments; `.withOptionsFrom()` for all 21 (13 slider, 6 combo, 2 toggle). Rate ↔ Division swap on `lfo_sync`; `bit_depth` / `lfo_div` stepped knobs bind through `getComboBoxState`. Fixed 1120 × 780, not resizable.
- **Phase 3.2, Visualization Panels (UI-01..03, PERF-03):** 30 Hz editor timer → `buildCycleView` (exact 2048-sample cycle with the voice's shared read code → quantizer → 2048 real FFT → bins 1..32, dB re heard max, −60 floor → 256-point point-sampled cycle) → hash-gated `cycleUpdate`; `bankUpdate` (N × 128 level-0 thumbnails) on bank change / import publish / state restore / `uiReady`. JS: stacked bank panel with highlight + marker (fractional for Interp On, jumping for Off), cycle panel showing staircases, harmonics 1–32 bars incl. band-limit ceiling and rust aliasing bars.
- **Phase 3.3, Import UX, Tooltips, i18n (UI-04..06):** Import button (FileChooser via native function, `complete()` before `launchAsync`, SafePointer completion) + WebView drop (`importDroppedAudio`, `Base64::convertFromBase64`, 96 MB cap); filename + frame count; empty-Imported state (dashed stack, prompt, amber button, silence); `importStatus` errors localized (`tooShort`, `unreadable`, `tooLarge`). Plain-language tooltips on every control; the Band-limiting, Formant and Import tips name the aliasing, formant-shift and loop-buzz lessons.
- **Lesson buttons (new decision):** `applyFactoryPreset(lessonId)` is wired **live in Stage 3** with the five mockup recipes (checklist §Lesson preset ids). Reset to defaults first, never set `output_level`; `aliasDemo` also jumps the on-screen keyboard to octave 6 (page side). FUNC-08 stays Stage 4: Stage 4 refines the recipes per ARCHITECTURE §A9 and adds the preset-manager bank.
- **On-screen keyboard:** 2 octaves + C, A–K / W–U, Z/X octave, via existing `handleUiMidi` → `MidiMessageCollector`; window blur releases held notes.
- **Native functions** (all registered, or the control is silently dead): `getParameterDefaults`, `getUiLanguage`, `setUiLanguage`, `uiReady`, `importAudio`, `importDroppedAudio`, `uiMidi`, `applyFactoryPreset`.

## Constraints Identified

- **Imported bank reads go only through `getImportedBankSnapshot()`** (ARCHITECTURE Amendment 12). Never load the raw `importedForAudio` atomic on the message thread: the worker and `setStateInformation` also publish, so a raw load can race a retire.
- **Processor API still to add:** `applyFactoryPreset`, `getBankThumbnails(BankThumbs&)`, `buildCycleView(CycleView&)`. Already present from Stage 2: `handleUiMidi`, `uiLanguage`, `importFromFile` / `importFromMemory`, `getImportStatus` / `getImportStatusVersion`, `getBankDisplayGeneration`, `getImportedBankSnapshot`. The import status is polled by version on the timer (D-E), not pushed from the worker.
- **Cycle render must share the voice's read code** (`WtRead.h`, `BitQuantizer.h`), so the bars equal an offline FFT of the heard cycle by construction. The UI-03 gate compares the `cycleUpdate` JSON payload against an offline FFT of a rendered cycle.
- **No audio-thread cost:** the audio thread publishes display atomics only (already in place from Stage 2). Rendering, FFT and JSON build happen on the message thread. The 30 Hz timer must not allocate per tick beyond the JSON payload. Pre-size the FFT and cycle buffers.
- `emitEventIfBrowserIsVisible` drops events while hidden, so the page re-sends `uiReady` on `visibilitychange` → visible, and C++ re-sends `bankUpdate` + `importStatus` + a forced `cycleUpdate` (project memory: completions dropped when hidden; one-shot state push stale on preset load). A lesson preset or host preset load must also force a `bankUpdate` / `cycleUpdate`.
- The bank `<select>` stays English (its entries are the automation names).
- i18n layout pins (`.title-block` 384 px, `.import-btn` 124 px, `.bank-readout` 300 px, `.tour-buttons` 458 px, etc.) must survive integration. check-ui-labels [7] gates them.
- **UI gates before the Stage 3 commit, on the real tree** (the finalization fixture is not the build): `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint` (zh-Hans at `reviewed: 'mt'`), `check-ui-labels`, `boot-all-uis --strict-tips`. Exit 77 = Playwright not resolved = nothing verified, not a pass.
- `Source/ui/public/modules/webview-drop-streaming.js` is gitignored and written at configure time. Do not commit it. A `git archive` snapshot lacks it (project memory).
- Windows: `withUserDataFolder()` always, static WebView2 linking. The Windows build itself is a Stage 4 item (COMPAT-02).
- Build/install via `./scripts/build-and-install.sh O-simpleWavetable`. The Standalone stays stale under that script (project memory), so rebuild it explicitly for the hands-on. Commits are path-scoped with the shared-checkout temp-index discipline.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Execution cadence | **3.1 + 3.2 → build/install → user visual checkpoint → 3.3** | The visual panels are the teaching core (UI-01..03). Check marker motion, the 3-bit staircase and the alias bars by eye before adding import UX, tooltips and i18n gates on top. Mirrors the Stage 2 listening checkpoint. |
| Lesson buttons | **Wire `applyFactoryPreset` live now** with the mockup recipes | The row is in the finalized mockup, and the buttons are how a class demos each concept. Stage 4 refines the recipes and adds the FUNC-08 preset bank. |
| Hands-on venue | **Standalone + one DAW (Logic or Live)** | Standalone covers the file drop and the keyboard. The DAW pass covers automation → UI, hide/re-show (`uiReady` re-send) and save/reopen with Imported (deferred from Stage 2 VERIFICATION). |
| Layout, style, controls, event contract, i18n | **As mockup v1 + integration checklist** | Finalized 2026-10-06 and pre-verified on a fixture tree (check-i18n 16/16, fr/zh lint clean, check-ui-labels 8 states, boot-all-uis clean). Not re-litigated. |
| Live output scope / spectrum | **Omitted** | ARCHITECTURE §Visualization: one truthful cycle picture; not in the brief. |

## Open Questions (for research)

- Exact `CycleView` / `BankThumbs` struct shapes and the hash inputs for the `cycleUpdate` gate (pos, frame, level, bank gen, interp, bit_depth, bandlimit, sounding, note). Is a float hash of the 256-point cycle cheaper than hashing the inputs?
- Which lead-voice display atomics Stage 2 already publishes vs what `buildCycleView` still needs (mip level read, latched frame, effective Position, `f0` / `nyquistH` for the band-limit ceiling marker).
- JSON payload cost at 30 Hz: `juce::var` / `DynamicObject` building vs a pre-formatted string. Does it stay within PERF-03 with the bank panel at N = 256?
- The reference editor to adapt from for the timer, import chooser and drop path: O-simpleAdditive vs O-simpleFM (the latest in-flight UI work).
- Harness for UI-03: a scratch driver that calls `buildCycleView` and compares against an offline FFT of a `WtVoice` render, vs. a Playwright capture of the live payload.
- How `applyFactoryPreset` resets to defaults without a host-visible preset load. Check the reset-defaults-first pattern and the gesture begin/end per param.

## Next Phase

Ready for: research phase
