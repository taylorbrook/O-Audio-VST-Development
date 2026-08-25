# O-simpleFM Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.2.5
- **Type:** Synth (Pedagogical 2-Operator FM/PM)

## Lifecycle Timeline

- **2026-06-20:** Ideated — creative brief + requirements (pedagogical 2-op FM synth).
- **2026-06-20 (Stage 0):** Research & Planning complete — ARCHITECTURE.md + ROADMAP.md documented. Complexity 5.0 (raw 11.0), staged implementation.
- **2026-06-20 (Stages 1–4):** Implemented & shipped v1.0.0 — DSP voice, WebView field-guide UI, preset manager.
- **2026-06-20 (v1.0.1):** UI fix — frame made scrollable, sections de-overlapped (`.controls` no longer shrinks below content), editor grown 760×720 → 760×860.
- **2026-06-20 (v1.0.2):** Added a dedicated tooltip for the Signal Path readout (`C:M ratio` + modulation index `I`); fixed `focusin` bubbling so nested `[data-tip]` tips aren't overridden by their ancestor.
- **2026-06-21 (v1.1.0):** Teaching + cleanup release. Added an on-screen keyboard (mouse + computer keyboard, MIDI injected via `MidiMessageCollector`) so the Standalone build makes sound without external MIDI; spectrum frequency-axis labels (100/1k/10k via new `getSampleRate` native fn); live harmonic/inharmonic + Carson sideband-count readout. Fixed the dead/self-contradicting index-ceiling code in `FMVoice` (single `computeIndexCeiling()` armed per block + snapped on note-on). De-duplicated the Lesson Presets (now load the factory presets by name; captions only in JS). Window grown 760×860 → 760×980. Validation: harness 7/7, auval + pluginval L10 green.
- **2026-06-21 (v1.2.0):** Sideband + carrier-null teaching visuals (viz-only, no DSP change). Spectrum now overlays predicted FM component frequencies — carrier f_c (solid amber, labelled) + sidebands f_c ± k·f_m, k=1..8 (dashed sage) — on the existing log-freq axis, drawn only while a note sounds. Carrier Hz reaches JS via a new `carrierUpdate` event emitted just before `spectrumUpdate` (processor tracks most-recent note-on pitch + any-voice-active; `getCarrierHz()`→0 when silent; audio thread does only 2 relaxed atomic stores). Added a green "carrier null" badge by the Signal Path I readout, gated on the *effective* radian index β=20·(I/20)^1.7 ≈ 2.405 (matches the render-harness `carrier-null@2.405` test; fires ≈ readout I 5.75, where the carrier marker sits on a nulled peak). Validation: harness 7/7, auval PASS + pluginval L10 exit 0.

- **2026-06-21 (v1.2.1):** Added hover tooltips to the five Lesson Preset buttons (E-Piano, Tubular Bell, Brass, Clarinet, Clang Bell), each explaining how that voice is built in FM terms (C:M ratio harmonic/inharmonic, mod index + envelope behaviour, feedback, resulting spectrum). Copy mirrors `FactoryPresets.cpp`. Implemented by reusing the existing `setupTooltips()` engine — added `data-tip="lesson…"` to each `.tour-btn` plus matching `TIPS` entries in `app.js`; no new mechanism, so buttons inherit pointer-hover + keyboard-focus + Escape-dismiss. WebView-only (no C++/DSP/param change). Validation: auval PASS.
- **2026-07-15 (v1.2.2):** Resolved CODE_REVIEW.md findings CR-01 + WR-01..WR-06 via /improve-review. CR-01: `launchAsync` completions in both preset file dialogs now capture `Component::SafePointer` and bare-return on teardown (UAF fix, W12 pattern). WR-01/02: on-screen keyboard uses pointer capture + blur sweep (no stuck notes) and QWERTY notes ignore keystrokes while focus is on the preset bar/dropdown/inputs. WR-03: preset Delete now confirms via in-DOM dialog (`promptDelete` + `onConfirmDelete`). WR-04: Save dialog honors the chosen folder via `savePresetToFile`. WR-05: render harness version now derives from a single `OSIMPLEFM_VERSION` CMake variable (was hardcoded "1.0.0", rewriting the user's real factory presets on every test run). WR-06: editor timer pushes `sampleRateUpdate` on rate change so the spectrum axis/sideband markers track the live Nyquist. IN-01..IN-04 deferred (opt-in, see Known Limitations). Validation: auval PASS; render harness ALL PASS (7/7), factory presets correctly stamped 1.2.2.
- **2026-07-15 (v1.2.3):** Resolved the deferred Info findings IN-01..IN-04 via /improve-review. IN-01: index taper/range constants single-sourced as `OSimpleFM::kIndexMax`/`kIndexTaper` in `FMVoice.h` (consumed by the DSP taper, param range, `/kIndexMax` re-normalization, and render-harness carrier-null test; JS mirrors as `INDEX_MAX`/`INDEX_TAPER` with cross-refs). IN-02: `handleUiMidi` clamps the WebView-supplied note to 0–127. IN-03: `scaledMidi.ensureSize` 4 KB → 16 KB (removes the last audio-thread reallocation path). IN-04: knobs double-click-reset to default via new `getParameterDefaults` native fn (full drag gesture, so hosts record the automation touch). All CODE_REVIEW.md findings now resolved. Validation: auval PASS; render harness ALL PASS (7/7 — carrier-null@2.405 proves the taper refactor is bit-equivalent).

- **2026-08-25 (v1.2.5):** Fixed clicks on note-off (any settings) via /improve. Root cause: `pushParamsToVoices()` called `juce::ADSR::setParameters()` every block; its `recalculateRates()` derives the release slope from SUSTAIN, clobbering the envelope-value-based rate `noteOff()` computes — and with amp sustain = 0 (four factory presets) the zero rate makes `recalculateRates()` hard-reset the envelope one block after note-off, truncating the tail (the click). Fix: `FMVoice` caches incoming ADSR params, pushes to the live envelopes only on value change and never mid-release; deferred changes apply at the next note-on. New render-harness `noteoff-click` probe (tail-rings-on + max sample-to-sample jump); verified failing against v1.2.4 code (negative control: tailRms 0.0000 / maxJump 0.39) and passing with the fix (0.2826 / 0.0356). Validation: harness 8/8 ALL PASS, auval registered, built + installed.

## Known Issues

None. All findings from the 2026-07-15 full-plugin code review (CODE_REVIEW.md: CR-01, WR-01..WR-06, IN-01..IN-04) are resolved as of v1.2.3.

## Additional Notes

Deliberately simple 2-operator **phase-modulation** synth built for teaching FM. One carrier, one modulator, C:M ratio, raw radian modulation index (0–20), DX7-style self-feedback, independent mod + amp ADSRs (mod env → index is the headline feature). 16-voice polyphony. WebView UI with first-class teaching visuals: live spectrum analyzer (4096/Blackman-Harris FFT), oscilloscope, operator routing diagram, per-parameter hover tooltips, educational preset tour.

Key Stage 0 decisions: PM not true FM; raw radian index exposed; 16 voices; anti-aliasing via sine LUT + key-tracked index ceiling + 2× polyphase-IIR oversampling (4× + band-limited wavetables only for opt-in non-sine operators). Real-time-safe lock-free `AbstractFifo` audio→UI tap; FFT on message-thread Timer.

Architecture references: O-Bassoon (voice skeleton), O-Marimba (scope FIFO + native fn), O-Prism (APVTS/WebView relays/FFT), O-AnalogEQ (cross-platform WebView2 CMake).

See `.planning/research/ARCHITECTURE.md` and `.planning/ROADMAP.md` for full detail; upstream research at `research/fm-phase-modulation-synthesis-o-simplefm.md`.
