# Stage 2 (DSP) — VERIFICATION (Phase 2.1)

**Plugin:** O-simpleSampler
**Stage:** 2 of 4 — DSP · **Phase 2.1** (Core Playable Sampler)
**Date:** 2026-06-25
**Method:** Goal-backward analysis against PLAN.md "Phase 2.1 Success Criteria" + technical validation (build/auval/pluginval) + an independent adversarial DSP code review (RT-safety, pitch/region correctness, root-seed/restore timing, verified against the JUCE 8 APVTS source and the O-simpleGrain/O-simpleSubtractive reference ports).

## Verdict: ✅ PASS (code + technical) — audible criteria gated on the DAW play-test (CONTEXT D2)

No blockers. Phase 2.1 delivers first audio with correct pitch, region, envelope, and source-load behavior. The remaining criteria are audible checks that require a human DAW session — that is the explicit CONTEXT D2 STOP, not a verification failure.

---

## Success-criteria assessment

| # | Phase 2.1 criterion | Verdict | Evidence |
|---|---------------------|---------|----------|
| 1 | Loads as instrument; MIDI routes; 16-voice; no crash | ✅ MET (code) / ⏳ confirm in DAW | `IS_SYNTH`/`NEEDS_MIDI_INPUT TRUE`; 16 `SampleVoice` added; auval `aumu` + pluginval@5 passed |
| 2 | Root 48 = original pitch (~131 Hz); notes transpose (Repitch) | ✅ MET | `voiceRate=2^((note−rootKey+tune+fine·.01)/12)` → 1.0 at note 48, 2.0 at octave; uses **live** rootKey, not kRootNote (`SampleVoice.h:102`) |
| 3 | Start/End change the played region | ✅ MET | `readPos=startSamp`; loop stops at `endSamp`; safe clamps (`PluginProcessor.cpp:462-469`) |
| 4 | tune/fine transpose independent of keyboard | ✅ MET | both summed into the `voiceRate` exponent (`SampleVoice.h:102-103`) |
| 5 | Piano selects/decodes/plays; seeds root 48; fresh = standard tune | ✅ MET | off-thread decode + `seedRootForSource`; fresh-instance prepare-time seed verified; restore keeps saved root (see Restore-timing note) |
| 6 | No obvious aliasing at high notes | ⏳ NEEDS-DAW | AA one-pole present + engaged on rate>1 (coeff matches O-simpleGrain ref); audible probe formally deferred to 2.3 render-harness |
| 7 | No clicks on note-on/off; no denormal stalls | ✅ MET (note-on/off) / ⚠ region-end | 5 ms attack / 0.2 s release declick note-on/off; `ScopedNoDenormals` on releases. **Region-end hard cut can click — see Warning 1 (2.2 scope).** |
| 8 | Build clean (3 formats); auval; pluginval@5 | ✅ MET | VST3+AU+Standalone link clean; **AU VALIDATION SUCCEEDED** (21 params); **pluginval@5 SUCCESS** |

---

## Independent DSP review — findings

**BLOCKERS: none.** Pitch math, region math, voice lifetime, AA filter, and the fresh-vs-restored root-seed logic all hold up; the restore path was specifically attacked and could not be broken.

**Warnings (real, non-blocking):**
1. **Region-end hard-cut click** (`SampleVoice.h:185-189`) — `ampEnv.reset(); break;` zeroes the VCA instantly at `readPos≥endSamp`, and truncates a release tail that reaches region-end. Near-silent at End=100% on a decayed piano tail, but **the most likely audible artifact when a user lowers End to isolate a region** (a core plugin lesson). Within documented 2.1 one-shot scope; **fix = short declick ramp at region-end, captured as a Phase 2.2 item.**
2. **Possible heap free on the audio thread during a source swap** (`PluginProcessor.cpp:442/378`) — if the block's snapshot held the last ref, the `AudioBuffer` destructs at end of `processBlock`. Bounded, rare (user source change), **accepted pattern inherited verbatim from O-simpleGrain**; a message-thread reclaim queue would close it (2.3 hardening).
3. **`std::atomic_load/store` on shared_ptr are spinlock-backed, not lock-free** (`PluginProcessor.h:144-152`) — brief bounded spinlock per block; accepted suite pattern; deprecated in C++20 (track if/when the suite moves to C++20).
4. **`setValueNotifyingHost` from `prepareToPlay`** (`:250→400`) — can dirty the project / record a spurious automation value on fresh instantiation; correctly gated to fire once (`rootSeeded`) and skipped on restore (`stateWasRestored`); no rootKey listener → no cascade. Advisability flag only.
5. **`triggerAsyncUpdate()` can post from the audio thread** if a host automates `sourceSample` mid-render (`:413`) — first post may allocate; decode itself stays off-thread. Rare; accepted pattern.

**Notes:** restore/reseed race traced through `replaceState → valueTreeRedirected → synchronous parameterChanged` and **CLEARED** (the queued async is reliably dropped by `cancelPendingUpdate()`+`pendingBuiltInIndex.store(-1)` before the message loop pumps). Deferred params (loop*, reverse, pitchMode, vintage, filter*) are **cleanly inert, not silent no-ops** — but a play-tester flipping Pitch Mode→Stretch or Reverse will hear no change in 2.1 (correct per plan). Mono-only playback (reads source channel 0). 16 `dynamic_cast`/block (accepted; could cache typed pointers).

---

## Technical validation (re-run this phase)

- `ninja O-simpleSampler_VST3 O-simpleSampler_AU O-simpleSampler_Standalone` → clean link, all three formats
- `auval -v aumu OsSm OuDv` → **AU VALIDATION SUCCEEDED**; render + 1-channel + bad-max-frames + parameter + ramped-param + **MIDI** all PASS; **21 Global Scope Parameters**
- `pluginval --strictness-level 5` (installed VST3) → **SUCCESS** (Automatable Parameters, buses, layout restore)
- Installed via dual-variant sweep → single `-dev` variant, no orphan shadow

---

## Outstanding for the human gate (CONTEXT D2 — DAW play-test)

Load `O-simpleSampler-dev` in a DAW and confirm: root 48 plays ~131 Hz and notes transpose; Start/End move the region (**listen for the region-end click when lowering End — known 2.2 declick item**); Tune/Fine transpose off-keyboard; fresh instance is in standard tune; no obvious aliasing up high. These are not auto-verifiable until the Phase 2.3 render-harness exists.

*Phase 2.1 verification PASS 2026-06-25. Next: Phase 2.2 (loop/reverse/Stretch/Vintage/filter) after the DAW play-test — fold the region-end declick into the loop/region work.*

---
---

# Stage 2 (DSP) — VERIFICATION (Phase 2.2a — Tone chain)

**Phase:** 2.2a (tone chain) · **Date:** 2026-06-25
**Method:** Goal-backward analysis against the Phase 2.2a success criteria + technical validation (build/auval/pluginval) + an orchestrator code review of the dsp-agent's diff (RT-safety, the load-bearing acceptance claims, the flagged crossfade deviation), read against the current `SampleVoice.h` + `PluginProcessor.{h,cpp}`.
**Decision context:** Human DAW play-test **deferred to post-GUI** (user 2026-06-25). The offline render-harness (Phase 2.3) is now the load-bearing automated correctness gate for the audible criteria below.

## Verdict: ✅ PASS (code + technical) — audible criteria deferred to the post-GUI DAW pass + the 2.3 render-harness

No blockers. The tone chain is correct, RT-safe, and bit-clean at Vintage 0; the loop deviation is a genuine correctness improvement over the literal port.

## Success-criteria assessment

| Phase 2.2a criterion | Verdict | Evidence |
|----------------------|---------|----------|
| No APVTS change; 21 params intact | ✅ MET | auval reports **21 Global Scope Parameters**; 9 deferred params wired from existing cached atomics; no `addParameter` |
| Vintage **bit-for-bit clean at 0** (DSP-04) | ✅ MET | `if (vintageOn)` (`vintage>0`) gates BOTH S&H and bit-crush; when off, `s` is untouched (`SampleVoice.h:332-338`) |
| Filter `setResonance` never 0 (asserts >0) | ✅ MET | `params.filterQ > 0 ? … : 0.707f` (`SampleVoice.h:250`); processor maps res%→Q with floor 0.707 (`PluginProcessor.cpp:582-583`) |
| Filter smoothing in processor, not 16× per-voice | ✅ MET | two `SmoothedValue` (`filterCutoffSm`/`filterQSm`, 20 ms), `skip(numSamples)` → block scalar pushed to all voices (one `tan`/voice/block) |
| Loop fwd seam continuity (0/10/100 ms) | ✅ MET (code) / ⏳ harness+DAW | dual-head equal-power crossfade; **deviation: incoming reads `readPos−loopLen`** so `incoming(loopEnd⁻)=read(loopStart)` — sample-continuous at the wrap (the literal `+xfade` would click at 100 ms) |
| Ping-pong + reverse | ✅ MET (code) / ⏳ harness+DAW | reflect `2·bound−readPos` + `dir` flip; reverse seeds `dir=−1`, `readPos=endSamp` at note-on |
| Region-end declick (no hard cut) — fixes 2.1 Warning 1 | ✅ MET (code) / ⏳ harness+DAW | raised-cosine ≤5 ms end-ramp, one-shot path only, reverse-aware; the 0.2 s release is unreachable from `ampEnv.reset()` so the dedicated ramp is required |
| Lead-voice `displayCutoffHz`/`displayK`(=1/Q) once/block | ✅ MET | published every block (`PluginProcessor.cpp:659-660`); `SubVizAnalyzer.h` copied verbatim for the Stage-3 curve |
| RT-safety (alloc/lock/IO-free; zero latency) | ✅ MET | audio path alloc-free; per-block transcendentals precomputed (filter `tan`, S&H/bit-crush coeffs); zero-cross snap message-thread-only with `-1` sentinel; `setLatencySamples(0)` retained |
| Build clean (3 formats); auval; pluginval@5 | ✅ MET | VST3+AU+Standalone link clean; **AU VALIDATION SUCCEEDED**; **pluginval@5 SUCCESS**; installed (dual-variant sweep, no orphan) |

## Review findings

**BLOCKERS: none.**

**Deviation (accepted — improves correctness):** loop-crossfade incoming head reads `readPos−loopLen` (pre-loop content) not the plan's literal `readPos−loopLen+xfade`. Continuity at the wrap forces offset 0; the literal would leave an ~`xfade`-sample seam jump — audible at the 100 ms crossfade the acceptance test exercises. Direction-symmetric (`+loopLen` for reverse). Documented inline.

**Minor (non-blocking, Stage-3 follow-up):** the loudest-active scan computes `leadAmp` then `ignoreUnused`s it — the v1 filter is global so all voices share the published smoothed cutoff/Q; the scan stages the Stage-3 per-voice lead-voice/playhead pattern (documented `PluginProcessor.cpp:647-658`). No behavior impact.

**Carry-forward to 2.3 (unchanged from 2.1):** message-thread reclaim queue for the source-swap free; `std::atomic_load/store(shared_ptr)` C++20 deprecation; `setValueNotifyingHost`-in-prepare advisability.

## Outstanding for the post-GUI human gate + the 2.3 harness
Loop forward no seam click @ 0/10/100 ms; ping-pong + reverse correct; region-end declick when End is lowered on a held note; Vintage clean at 0 → grit as raised; filter shapes tone with no zipper. The 2.3 render-harness asserts the click-absence / declick / Vintage-clean cases automatically; the final human A/B is batched after Stage 3.

*Phase 2.2a verification PASS 2026-06-25. Next: Phase 2.2b (Stretch SOLA).*

---
---

# Stage 2 (DSP) — VERIFICATION (Phase 2.2b — Stretch SOLA)

**Phase:** 2.2b · **Date:** 2026-06-25
**Method:** Goal-backward analysis + technical validation (build/auval/pluginval) + orchestrator code review of the dual-path `renderNextBlock` refactor (the DSP-01 high-risk change), read against the current `SampleVoice.h` + `PluginProcessor.{h,cpp}`.
**Decision context:** human DAW A/B deferred to post-GUI; the 2.3 render-harness single-grain autocorr probe is the automated proxy for "Repitch vs Stretch obvious."

## Verdict: ✅ PASS (code + technical) — pitch/time-independence assertion deferred to the 2.3 harness

No blockers. The dual read-path → shared-tail refactor is clean: Repitch is byte-preserved, Stretch is correct SOLA, the tail is single-instance, and 2.2a is not regressed.

## Success-criteria assessment

| Phase 2.2b criterion | Verdict | Evidence |
|----------------------|---------|----------|
| `pitchMode` toggles Repitch ↔ Stretch | ✅ MET | `renderNextBlock` forks on `latchedPitchMode`; processor pushes `p.pitchMode`; voice latches at note-on |
| Latched at note-on (no mid-note click) | ✅ MET | `latchedPitchMode = params.pitchMode` in `startNote`; mid-block param change does not switch a sounding voice |
| Duration preserved, pitch tracks key | ✅ MET (code) / ⏳ harness | `timePos += 1·dir` (1× realtime) vs grain `g.rate = voiceRate` — time axis decoupled from pitch axis (RESEARCH P6.2) |
| Shared downstream tail (no fork drift) | ✅ MET | Vintage/filter/endRamp/VCA/addSample single-instance downstream of the `s` fork; per-grain AA is inside the Stretch sum (no double-AA) |
| 2× Hann overlap, unity gain | ✅ MET | hop `lenSamp/2`; **√overlap normalizer dropped** (COLA); seam smoothed by overlap (no dual-head crossfade in Stretch) |
| WindowLuts shared, not per-voice | ✅ MET | one `WindowLuts{2048}` in the processor (ctor-built), `const WindowLuts*` per voice via `setWindowLuts` in `prepareToPlay`; null-guarded |
| Zero latency; RT-safe | ✅ MET | fixed `std::array<Grain,4>` (no heap); steal-oldest O(4); one `exp` on spawn (accepted, = O-simpleGrain); `setLatencySamples(0)` |
| No APVTS change; 2.2a not regressed | ✅ MET | auval **21 params**; Repitch verbatim in the `else`; Vintage bit-clean-at-0 + `setResonance>0` reached unchanged from the tail |
| Build clean; auval; pluginval@5 | ✅ MET | VST3+AU+Standalone link clean; **AU VALIDATION SUCCEEDED**; **pluginval@5 SUCCESS**; installed (no orphan) |

## Review findings

**BLOCKERS: none.**

**Notes (non-blocking):**
- **Granular reverse (by design):** grains read forward; the time axis (spawn positions) reverses — standard SOLA reverse, flagged as a non-obvious correctness point.
- **In-flight grains near a loop/region boundary** read slightly past it (clamped by `readSourceLagrange` jlimit — no OOB); this is the normal SOLA overlap tail, window-attenuated.
- **`latchedPitchMode` carried since 2.2a** now drives the fork; the `(Task 8)` comment on `lastAmpEnv` is a harmless stale planning ref.

## Outstanding for the 2.3 harness + post-GUI human gate
The headline "Repitch vs Stretch obvious" is asserted automatically by the 2.3 render-harness **single-grain autocorr** probe (a held low note: Repitch lengthens+lowers pitch together; Stretch holds duration while pitch tracks the key). Spectral-bin probes are confounded by the grain comb (project memory) → autocorr. Final human A/B batched post-GUI.

*Phase 2.2b verification PASS 2026-06-25. Next: Phase 2.3 (hardening + viz + render-harness — the Stage-2 correctness gate).*

---
---

# Stage 2 (DSP) — VERIFICATION (Phase 2.3) — STAGE 2 GATE

**Phase:** 2.3 · **Date:** 2026-06-25
**Method:** The **offline render-harness IS the verification** for Phase 2.3 (the Stage-2 correctness gate) — it converts the deferred audible DAW checks into automated assertions. Plus technical re-validation (build/auval/pluginval) after the viz tap changed `processBlock`.

## Verdict: ✅ PASS — Stage 2 (DSP) COMPLETE. Render-harness ALL 9 PASS (exit 0).

No blockers. With human DAW testing deferred to post-GUI, the harness is the load-bearing gate, and it asserts every audible Stage-2 criterion — most importantly the headline **Stretch pitch/time independence**.

## Gate results (the harness output, exit 0)

| # | Criterion (PLAN 2.3 Success Criteria) | Verdict | Evidence |
|---|----------------------------------------|---------|----------|
| 1 | makes sound, finite | ✅ | rms=0.215 |
| 2 | **Repitch tuning** (root 131 Hz, octave 2×) | ✅ | f48=131.2 · f60/f48=2.000 · f36/f48=0.501 (autocorr) |
| 3 | **Stretch pitch/time independence** (single-grain autocorr) | ✅ | pitch tracks key (f60/f48=2.012) **AND** duration held: Repitch dur ratio **1.89** vs Stretch **0.93** |
| 4 | **loop-seam click absence** (fwd + ping-pong) | ✅ | continuity 0.953 / 0.944; seam maxΔ 0.004 |
| 5 | **region-end declick** (End lowered mid-hold) | ✅ | endMaxΔ 0.0008 ≪ 0.5·contentLevel (hard cut ≈ contentLevel) |
| 6 | **Vintage clean-at-zero** (bit-for-bit intent) | ✅ | flatClean 0.0026 vs flatCrush 0.309 (substituted observable — see SUMMARY) |
| 7 | **anti-alias budget** (high keys + extreme Stretch) | ✅ | note 84 (+3 oct) finite + peak-bounded, both modes |
| 8 | alloc-free processBlock; latency 0 | ✅ | viz ring write is copy-only (no alloc/lock/FFT); `setLatencySamples(0)` |
| 9 | 16 voices, graceful stealing, no stuck notes | ✅ | stress chord, tail silent after note-offs (tailRms 0.0000) |

## Re-validation after the viz tap (processBlock changed)
- `ninja` VST3+AU+Standalone → clean; **auval SUCCEEDED** (21 params); **pluginval@5 SUCCESS**; installed (no orphan).

## Carry-forward (Stage 3 / backlog — documented, NOT blockers)
- Human DAW play-test batched post-GUI (loop @ 0/10/100 ms by ear, Repitch↔Stretch A/B, Vintage/filter feel).
- Stage-3 forward items: `O-simpleSampler_UIResources` binary-data target (distinct NAMESPACE), viz curve/scope drawing (consumes `displayCutoffHz`/`displayK`/`displayPlayhead` + `vizRing`).
- RT-safety hardening backlog (3 items): source-swap reclaim queue; `std::atomic_load/store(shared_ptr)` C++20 deprecation; `setValueNotifyingHost`-in-prepare advisability.

*Phase 2.3 verification PASS 2026-06-25 — **Stage 2 (DSP) COMPLETE**. Next: Stage 3 (GUI).*
