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
