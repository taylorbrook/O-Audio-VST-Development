# Stage 2 / Phase 2.1 — Verification

**Plugin:** O-Bassoon
**Stage:** 2 of 4 (DSP)
**Phase:** 2.1 — Core Modal Voice + First Audio
**Verification Date:** 2026-04-27
**Verdict:** ✅ VERIFIED — automated subset PASS, manual Logic-AU subset PASS, item 10 (SPAN baseline PNG) dropped from gate per user decision (SPAN not installed; Phase 2.2 A/B will use ear + reference WAV instead of pre-tuning PNG diff)

---

## Goal-Backward Analysis

### Original Goal (from CONTEXT.md / PLAN.md / ROADMAP)

Replace the Stage-1 silent voice stub with a working modal-synthesis voice that produces a sustained, in-tune tone for any single MIDI note (C1–C6) with: no clicks, no NaN/inf, > 10 s sustain, < 5 % CPU @ 48 k / 256 (1 voice). Phase 2.1 proves the architectural seams (excitation → resonator bank → envelope → stereo write) before bassoon-specific timbre work in Phase 2.2.

**Phase 2.1 explicitly defers:** bassoon-tuned partial ratios + tone parameter (Phase 2.2), vibrato/breath/attack-character/output-gain APVTS reads (Phase 2.3), polyphony cap + voice stealing + NE/MPE consumption + TuningEngine call (Phase 2.4), strictness-10 + Windows + Dorico parity (Stage 4).

### Deliverables (from SUMMARY.md + code inspection)

1. `Source/ModeBank.{h,cpp}` — 16-mode parallel pole-only resonator bank, integer harmonics placeholder, T60 = 2.5 s → 0.25 s, Direct-Form-I biquad with `std::isfinite` guard, Nyquist mute at `f_k > 0.45 · fs`, `1/N` headroom scaling.
2. `Source/Exciter.{h,cpp}` — 5 ms half-sine × exp impulse, peak-normalised, in-class `std::array<float, 1024>` storage (per-voice, populated once).
3. `Source/BassoonVoice.{h,cpp}` — silent stub replaced; per-sample render loop (`exciter → modeBank → ADSR → addSample`), pitch-bend ±2 semitones (raw 14-bit), full state-reset on voice exit, ZERO APVTS reads / ZERO TuningEngine calls.
4. `Source/PluginProcessor.cpp::prepareToPlay` — per-voice `BassoonVoice::prepareToPlay` dispatch loop; `processBlock` / NE drain ordering / param layout / bus contract untouched.
5. `CMakeLists.txt` — `target_sources` block extended with ModeBank + Exciter; flags (`IS_SYNTH`, `NEEDS_MIDI_INPUT`, `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `PLUGIN_CODE OBsn`) untouched.
6. `research/reference-recordings/{bassoon-c3-sustain-v1.wav, bassoon-c3-sustain-v2.wav, LICENSE.md, README.md}` — VSCO-2-CE bassoon C3 (CC0) archived for Phase 2.2 A/B listening.

### Goal Achievement (architectural seams only — audible verification deferred)

| Goal | Status | Evidence |
|------|--------|----------|
| Mode bank exists, pre-allocated, no allocations in audio thread | ✅ Achieved | `ModeBank::modes` is `std::array<ModeBiquad, 16>` (header storage); RT-safety grep returns ZERO matches for `new`/`make_unique`/`push_back`/`resize`/`malloc` |
| Exciter pre-baked at `prepare()`, allocation-free `start()`/`getNextSample()`/`reset()` | ✅ Achieved | `Exciter::onsetBuffer` is `std::array<float, MAX_ONSET_SAMPLES>` populated once in `prepare()`; `start()` resets `onsetIdx` only |
| ADSR wired with hardcoded `{10ms, 0, 1.0, 200ms}`; `setSampleRate` BEFORE `setParameters` | ✅ Achieved | `BassoonVoice.cpp:33-34` |
| Per-sample inner loop: excite → resonate → envelope → stereo `addSample` | ✅ Achieved | `BassoonVoice.cpp:99-120` |
| Voice exit path: `clearCurrentNote()` → `modeBank.reset()` → `exciter.reset()` → return | ✅ Achieved | `BassoonVoice.cpp:112-119` |
| Plain MIDI frequency (no TuningEngine call); no APVTS reads | ✅ Achieved | `BassoonVoice.cpp:46`; grep for `parameters` / `tuningEngine->` / `pendingTuningSource->` returns ZERO functional matches |
| Pitch-bend ±2 semitones (raw 14-bit) | ✅ Achieved | `BassoonVoice.cpp:40-42, 70-84` |
| Per-voice `prepareToPlay` dispatch loop | ✅ Achieved | `PluginProcessor.cpp:144-146` |
| Sustained, in-tune, click-free, drift-free audio in DAW | ✅ Achieved | Logic-AU manual subset (items 1–6, 9) reported PASS by user 2026-04-27 |
| SPAN baseline PNG archived | 🚫 Dropped | Item 10 removed from gate per user decision (SPAN not installed); Phase 2.2 A/B uses ear + reference WAV instead — non-blocking |

---

## Requirements Verification

**Stage:** 2-dsp
**Requirements verified at this stage (per traceability):** FUNC-01..05, DSP-01..06, PERF-01, PERF-02, QUAL-01, QUAL-02 (DSP-07 already complete at Stage 1; COMPAT-01 final gate at Stage 4).

**Phase 2.1 deliberately satisfies only the structural subset of these.** Spectral/expression/polyphony halves land in Phases 2.2 → 2.4.

| Requirement | Priority | Status (post-Phase-2.1) | Notes |
|-------------|----------|-------------------------|-------|
| FUNC-01 — Sustained bassoon-like tones via modal synthesis | must | ⚠️ partial | Modal voice runs, holds pitch ±2 c, sustains > 10 s without drift (manual Gate 1 items 1, 4 PASS). "Bassoon-like" timbre awaits Phase 2.2 partial-table. |
| FUNC-02 — Polyphonic 1-16 voices (default 8) | must | ⏸️ deferred | Synth allocates 16 voices but voice manager + cap + stealing land Phase 2.4. |
| FUNC-03 — Range C1-C6 | must | ✅ complete | C1-C6 sweep PASS (Gate 1 item 6 / item 9 manual smoke). All notes track pitch; modal bank reconfigures cleanly with expected high-mode thinning at C6 (Nyquist mute policy working as designed). |
| FUNC-04 — Long-tone amplitude envelope | must | ⚠️ partial | ADSR present, hardcoded `{10ms, 0, 1.0, 200ms}`; click-free at note-on/off (Gate 1 item 2 PASS). Parameter wiring + 0-2000/0-3000 ms ranges land Phase 2.3. |
| FUNC-05 — Voice stealing | should | ⏸️ deferred | Phase 2.4. |
| DSP-01 — Modal-synthesis voice (bank of damped resonators, pre-allocated) | must | ⚠️ partial | Structural half COMPLETE — bank exists, pre-allocated, RT-safe, audible (Gate 1 items 1-6, 9 PASS). Spectral half (bassoon-tuned ratios) awaits Phase 2.2. |
| DSP-02 — Vibrato | must | ⏸️ deferred | Phase 2.3. |
| DSP-03 — Tone / brightness | must | ⏸️ deferred | `ModeBank::setTone` present as no-op stub; wired live Phase 2.2. |
| DSP-04 — Breath / dynamics (CC2 + velocity) | must | ⏸️ deferred | Phase 2.3. |
| DSP-05 — Attack-character | should | ⏸️ deferred | Phase 2.4. |
| DSP-06 — VST3 NE + MPE pitch-bend per-voice | must | ⏸️ deferred | NE drain wired at Stage 1; per-voice `applyPendingTuning` lands Phase 2.4. Host pitch-bend ±2 semi already wired in `pitchWheelMoved`. |
| DSP-07 — No O-Reed dependency | must | ✅ complete | Stage 1; carried forward — no new sources reference O-Reed (grep clean). |
| PERF-01 — Real-time safe (no allocations in `processBlock`) | must | ✅ complete | RT-safety grep returns ZERO matches in audio-thread sources; pluginval-5 fuzz/state tests PASS; auval render-rate matrix PASS. |
| PERF-02 — 8-voice <25 % CPU | should | ⏸️ deferred | Phase 2.4 (8-voice). 1-voice CPU < 5 % @ 48 k / 256 PASS (Gate 1 item 5). |
| QUAL-01 — No clicks, NaN/inf, aliasing | must | ⚠️ partial | NaN guard in `ModeBiquad::processSample`; Nyquist mute at `0.45·fs`. Click-free at note-on/off + finite spectrum (Gate 1 items 2, 3 PASS). Full parameter-sweep + extreme-vibrato aliasing coverage Phase 2.3. |
| QUAL-02 — Stable long-tone (no drift over 60 s) | nice | ⚠️ partial | > 10 s subset PASS (Gate 1 item 4). Full 60 s bar Phase 2.3. |
| COMPAT-01 — pluginval (VST3 + AU) | must | ⚠️ partial | strictness-5 PASS this run; strictness-10 + Windows = Stage 4. |

**Requirements summary (Phase 2.1 contribution):**
- ✅ Complete: 3 (DSP-07 carried, PERF-01 enforced, FUNC-03 range verified)
- ⚠️ Partial: 6 (FUNC-01, FUNC-04, DSP-01, QUAL-01, QUAL-02, COMPAT-01) — structural halves verified audibly; remainders deferred to Phase 2.2-2.4 / Stage 4
- ⏸️ Deferred (later phase/stage): 7
- ❌ Failed: 0

---

## Automated Checks

| # | Check | Result | Evidence |
|---|-------|--------|----------|
| 1 | `ninja O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone` | ✅ PASS | `ninja: no work to do.` (incremental clean re-run; full link from execute step) |
| 2 | AU cache cleared + VST3/AU installed fresh | ✅ PASS | `~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3` + `~/Library/Audio/Plug-Ins/Components/O-Bassoon-dev.component` present post-install |
| 3 | `auval -v aumu OBsn OuDv` | ✅ PASS | Final line: `AU VALIDATION SUCCEEDED.` (Gate 1 item 7) — full render-rate matrix (11k/22k/44.1k/48k/96k/192k), Bad-Max-Frames render-fail, parameter-set + ramped-schedule, MIDI tests all PASS |
| 4 | `pluginval --strictness-level 5 --validate ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3` | ✅ PASS | exit=0; `SUCCESS`; output-only bus confirmed (0 in / 2 out) (Gate 1 item 8) |
| 5 | RT-safety grep — `\bnew\b\|make_unique\|make_shared\|push_back\|resize\|malloc` in `ModeBank.{h,cpp} + Exciter.{h,cpp} + BassoonVoice.cpp` | ✅ PASS | ZERO matches |
| 6 | No APVTS reads in `BassoonVoice.cpp` (locked Q2) | ✅ PASS | `grep -n "parameters" Source/BassoonVoice.cpp` → ZERO matches |
| 7 | No `tuningEngine->` / `pendingTuningSource->` dereferences in `BassoonVoice.cpp` | ✅ PASS | Only one match — a doc comment at line 45 referencing the Phase 2.4 future call. ZERO functional dereferences. |
| 8 | No `setLatencySamples` functional call (modal synthesis is feed-forward) | ✅ PASS | Only Stage-1 comment at line 136; ZERO functional calls. |
| 9 | 16 voices in `Synthesiser` (`for (int i = 0; i < 16; ++i)`) | ✅ PASS | `PluginProcessor.cpp:118` |
| 10 | NE drain BEFORE `synthesiser.renderNextBlock` | ✅ PASS | `vst3Extensions.drainAndUpdate()` at `PluginProcessor.cpp:178` precedes `synthesiser.renderNextBlock` at `:182` |
| 11 | `buffer.clear()` BEFORE drain (host clears so voices `addSample` sums correctly) | ✅ PASS | `PluginProcessor.cpp:173` |
| 12 | Output-only bus (`isBusesLayoutSupported` rejects input bus) | ✅ PASS | `PluginProcessor.cpp:154-165` + pluginval reports 0 in / 2 out |
| 13 | 10 APVTS parameters present | ✅ PASS | 9× `AudioParameterFloat` + 1× `AudioParameterInt` `make_unique` calls in `createParameterLayout()` |
| 14 | CMakeLists flags unchanged (`IS_SYNTH`, `NEEDS_MIDI_INPUT`, `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `PLUGIN_CODE OBsn`) | ✅ PASS | grep all six tokens present in `plugins/O-Bassoon/CMakeLists.txt` |
| 15 | `juce_generate_juce_header` AFTER `target_link_libraries` (JUCE 8 requirement) | ✅ PASS | line 76 follows line 54 |
| 16 | `ouaricon_add_module(O-Bassoon note-expression)` present | ✅ PASS | `CMakeLists.txt:44` |

**Automated PASS rate: 16/16.**

---

## Gate 1 Bar Status (originally 10-item; item 10 dropped per user — final 9-item bar)

| # | Item | Status | Evidence |
|---|------|--------|----------|
| 1 | Sustained tone at correct pitch (±2 cents on C3 / 130.81 Hz) | ✅ PASS | User confirmed via Logic-AU + tuner 2026-04-27 |
| 2 | No clicks at note-on/note-off | ✅ PASS | User confirmed via Logic-AU listen 2026-04-27 |
| 3 | No NaN/inf in render | ✅ PASS | User confirmed audible — finite output, no DC spike or runaway 2026-04-27 |
| 4 | > 10 s sustain without amplitude drift | ✅ PASS | User confirmed via Logic-AU level meter 2026-04-27 |
| 5 | 1-voice CPU < 5 % @ 48 k / 256 | ✅ PASS | User confirmed via Logic Performance Meter 2026-04-27 |
| 6 | C1-C6 sweep without resonator instability | ✅ PASS | User confirmed via MIDI 24-84 sweep in Logic 2026-04-27 |
| 7 | `auval -v aumu OBsn OuDv` SUCCESS | ✅ PASS | `AU VALIDATION SUCCEEDED.` |
| 8 | `pluginval --strictness-level 5` SUCCESS | ✅ PASS | exit 0, `SUCCESS` |
| 9 | Logic AU manual smoke (hold C3, sweep C1→C6) | ✅ PASS | User confirmed 2026-04-27 |
| ~~10~~ | ~~SPAN baseline spectrum PNG captured~~ | 🚫 DROPPED | User decision 2026-04-27 — SPAN not installed; Phase 2.2 A/B will use ear + reference WAV instead. Non-blocking deviation from PLAN.md Q7 (Gate item 10 was originally locked; user authorized drop in verify-phase). |

**Gate 1 score: 9/9 PASS** (item 10 dropped from gate by user authority).

---

## Human Verification Checklist (completed 2026-04-27)

User-driven Logic-AU verification (mirrors PLAN.md Task 8):

- [x] Open Logic Pro, create instrument track, load O-Bassoon-dev (AU), set buffer 256 / 48 kHz.
- [x] Hold a single C3 (MIDI 60) → tuner reads 130.81 Hz ±2 cents (Gate 1 item 1).
- [x] Listen at note-on / note-off transients — no clicks (Gate 1 item 2).
- [x] Logic stock metering — no DC spike, no runaway energy growth, finite output (Gate 1 item 3).
- [x] Hold C3 for ≥ 12 s — level meter stable within ±6 dB (Gate 1 item 4).
- [x] Logic Performance Meter "Process" bar — single voice < 5 % (Gate 1 item 5).
- [x] MIDI scale C1 → C6 (notes 24-84) — pitch tracks, no resonator breakdown (Gate 1 item 6 + item 9 manual smoke).
- [ ] ~~Render sustained C3 → SPAN screenshot~~ — DROPPED from gate (item 10) per user 2026-04-27; SPAN not installed.

---

## Issues Found

None blocking. All static checks PASS.

**Notes carried forward (no action required):**

- **Voice will not sound bassoon-like at Phase 2.1.** Placeholder integer harmonics + flat amplitudes + ADSR-only envelope produce a bright, organ-like timbre. This is intentional per ROADMAP — bassoon spectrum is the Phase 2.2 deliverable. Manual smoke (Gate 1 item 9) is a structural pass/fail, not a tonal-quality judgment.
- **Reference recording octave-convention caveat (D4 in RESEARCH §4).** VSCO-2-CE filename "C3" may not match Helmholtz/MIDI C3 conventions. Tuner check at audition time will confirm; rename file if needed before Phase 2.2.
- **`-24 dB peak expected at Phase 2.1` (D6 in RESEARCH §4).** The `1/N=1/16` scaling is intentional headroom margin for 16 modes summed at unity. Phase 2.3 replaces with proper `output_gain` APVTS read; the user may need to push the Logic track fader up to hear the held note clearly.

---

## Stage Verdict

**Status:** ✅ VERIFIED

**Phase 2.1 architectural seams:** ✅ all delivered, RT-safety enforced, build-system clean, both static validators (auval / pluginval-5) PASS.

**Phase 2.1 audible verification:** ✅ Gate 1 items 1-6, 9 reported PASS by user via Logic Pro AU 2026-04-27. Item 10 (SPAN baseline PNG) dropped from gate per user decision (SPAN not installed) — non-blocking, Phase 2.2 A/B will use ear + archived reference WAV instead.

**Atomic Phase 2.1 commit (Task 9 of PLAN.md):** PENDING explicit user request. Per CLAUDE.md commit protocol, the orchestrator does NOT auto-commit. Ready to land on user's "commit it" / "land it" / "ship it" trigger.

**Ready for next phase (Phase 2.2 — Bassoon Partial Table):** **Yes**, after the atomic commit lands on `main`. The reference recording archive (VSCO-2-CE C3 sustains + LICENSE + README) is in place and ready for Phase 2.2 A/B listening.

**Blockers:** None.

**Pending action (user-triggered):**
- Atomic commit `feat(O-Bassoon): Phase 2.1 first audio - Gate 1 PASS` (PLAN.md Task 9 — single commit lands sources + reference recordings + planning artefacts on `main`).

---

## Audit Trail

**rev-1 (2026-04-27):** Phase 2.1 verify-phase. Automated subset (auval + pluginval-5 + 16-item invariant battery + RT-safety grep + locked-Q2 grep) PASS. Manual subset (Logic-AU items 1-6, 9) reported PASS by user same day. Item 10 (SPAN baseline PNG) DROPPED from gate per user authority — SPAN not installed, Phase 2.2 A/B uses ear + reference WAV. Verdict **✅ VERIFIED**. REQUIREMENTS.md updates: PERF-01 → complete, FUNC-03 → complete (range verified), QUAL-02 pending → partial (>10s subset PASS), DSP-07 unchanged at complete, COMPAT-01 unchanged at partial (final gate Stage 4); FUNC-01, FUNC-04, DSP-01, QUAL-01 remain partial pending Phase 2.2-2.3 spectral/expression deliverables. STATUS.md advances to `phase: verify_complete`; atomic commit (PLAN.md Task 9) PENDING explicit user trigger per CLAUDE.md commit protocol.

**Inherited verbatim from CONTEXT (rev-1) + RESEARCH (rev-1) + PLAN (rev-1):**
- 10-item Gate 1 bar (CONTEXT Q7)
- Single atomic commit on Gate 1 PASS (CONTEXT Q5; PLAN Task 9)
- Strict ROADMAP minimal wiring — no APVTS reads, no TuningEngine call (CONTEXT Q2)
- ModeBank pole-only specialisation with NaN guard (RESEARCH §2)
- Nyquist mute at `f_k > 0.45 · fs` (RESEARCH OQ#5)
- 5 ms half-sine × exp impulse, peak-normalised (RESEARCH OQ#4)
- VSCO-2-CE C3 sustain + LICENSE archived for Phase 2.2 (CONTEXT Q4 / RESEARCH OQ#7)
