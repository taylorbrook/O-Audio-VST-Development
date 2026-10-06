# Stage 2: DSP - Verification

## Verification Date

2026-10-06

## Method

Goal-backward against CONTEXT.md and PLAN.md success criteria. Every automated claim in SUMMARY.md was **re-run independently** in this verify session. Nothing was taken on SUMMARY's word.

- **Build:**
  - `ninja O-simpleWavetable_VST3 O-simpleWavetable_AU` had no work to do, so the build is current with HEAD `c9727200`. The plugin tree is clean.
  - The installed `-dev` bundles are byte-identical (`diff -rq`) to the Release artefacts.
- **Offline gates:**
  - Rebuilt from scratch in a fresh out-of-repo Debug tree (the `$SCRATCH/build-oswt` recipe with `OUARICON_BUILD_TESTS=ON` and every other plugin skipped).
  - All 5 drivers re-run, plus `dsp-check --alloc-check`.
- **Host checks:** run on the **installed** binaries:
  - targeted `auval`
  - pluginval VST3 and AU at strictness 10
  - a **new, independent** pedalboard probe (`uv run --python 3.12 --with pedalboard`). It was written in this session, does not reuse the execute-phase harness, and sets params through `plugin.parameters[name]`.
- **Code review:** post-execute critic review (DSP + architecture critics, read-only). See Issues Found.

## Goal-Backward Analysis

### Original Goals (CONTEXT.md)

1. **2.1 Bank Engine:**
   - five 32 × 2048 built-in banks
   - strict 11-level FFT mips with DC and Nyquist zeroed
   - shared across instances
   - the offline FFT gates
2. **2.2 Voice + Oscillator:**
   - band-limited linear read
   - mid-rise quantizer
   - amp ADSR
   - Poly 16 / true-legato Mono
   - ±2 st bend
   - output stage
   - Gates: FUNC-01, QUAL-02, DSP-01/02/03, FUNC-07 and PERF-01.
3. **2.3 Modulation → Position:**
   - 20 ms knob smoothing
   - a global LFO (5 shapes, free / tempo)
   - deterministic S&H
   - a per-voice mod ADSR
   - the clamp
   - a 2 ms per-voice smoother
   - Gates: DSP-05, FUNC-05/06 and the click detector.
4. **2.4 Bank Switch / Import / Persistence (HIGH risk):**
   - the frozen-cycle 5 ms crossfader
   - the atomic imported-bank publish
   - the REG-01 reaper with the D-C amendment
   - the importer worker
   - `IMPORTED_BANK` (flac16 + pcm16gz)
   - Gates: QUAL-03, DSP-06, FUNC-03, COMPAT-03, FUNC-04 and the reaper rules.
5. **Cadence:** 2.1 + 2.2, then a listening checkpoint, then 2.3 + 2.4.

### Deliverables (SUMMARY.md + code)

1. `WavetableBank`, `MipmapBuilder`, `BankFactory`, `BuiltInBanks` (`SharedResourcePointer`). Build time is logged at about 25 ms (Debug).
2. `WtVoice`, `WtRead`, `BitQuantizer`, `MonoStack`, plus `WtSynthesiser`, an alloc-free port of JUCE's voice steal.
3. `PositionLfo`, `PositionSmoother` (one-pole, D-N), and the per-voice mod env in `WtVoice`.
4. The crossfader (a raised-cosine law, which deviates from the plan; ARCHITECTURE amendment 11), the reaper + `audioHeldBank`, `WavetableImporter`, and the import API (D-E) plus persistence.
5. Part 1 commit `9d1911ac`. Task 11 listening was signed off by Taylor on 2026-10-05. Part 2 commit `c9727200`.

### Goal Achievement

| Goal | Status | Evidence (re-run this session) |
|------|--------|-------------------------------|
| 1. Bank engine | ✅ Achieved | **bank-check ALL PASS:** G-PEAK 0.000003 dB, G-DC −181.8 dB, G-BL, Saw/Square/Drive harmonic gates. The G-NEG negative control fires (−34.0 dB with Nyquist kept). |
| 2. Voice + oscillator | ✅ Achieved | **dsp-check ALL PASS:** G-PITCH 0.0009 c, G-Q2-C8 −114.3 dB, G-Q2-SWEEP −74.9 dB, G-Q2-PULSE −67.0 dB (D-B named exception), G-VEL −11.905 dB. **`--alloc-check`:** 0 allocations (liveness counted 1). **pedalboard:** pitch A0..C8 × 44.1/48/96k worst 0.0001 c, velocity −11.905 dB, Poly 3 notes / Mono last-note (others ≤ −119 dB), 16 voices finite. |
| 3. Modulation → Position | ✅ Achieved | **mod-check ALL PASS:** G-CLICK-SQ 1.414, G-CLICK-SH 1.144 (≤ 1.5). The negative controls fire at ≥ 10.9. **pedalboard:** the LFO Square sweeps the centroid 131→696 Hz, and depth 0 stays static (1.0006). The mod env +1 rises and returns (274/1024/132 Hz); −1 falls and returns (969/168/1029 Hz). |
| 4. Switch / import / persistence | ✅ Achieved | **import-check ALL PASS:** QUAL-03 exactness ≤ 6e-8 and ratio worst 1.31. G-ALLOC-24 0 allocations. **D-C:** without the amendment deref-after-reap = 1, with it 0. +2 rule held. Soak: 2801 blocks / 50 imports, deref 0, max held 2. COMPAT-03 9/9 bit-identical. FUNC-04 memcmp equal, and the negative control fails as it should. **state-check 11/11.** **pedalboard:** bank, bandlimit and interp switches mid-note, d2 ratio 0.93–1.02; empty Imported = exact 0. |
| 5. Cadence | ✅ Achieved | Two commits with the listening sign-off between them (STATUS, SUMMARY). |

## Requirements Verification

**Stage:** stage-2
**Requirements for this stage:** 18 total (17 must, 1 should: COMPAT-03)

| Requirement | Priority | Status | Evidence |
|-------------|----------|--------|----------|
| FUNC-01: Oscillator, pitch-true | must | ✅ Complete | G-PITCH 0.0009 c. pedalboard worst 0.0001 c across A0–C8 at 3 rates. Position 0→1 shifts the centroid 131→1029 Hz with a 0.000 c f0 shift. |
| FUNC-02: Five 32-frame banks | must | ✅ Complete | bank-check (frame count, Saw h1..k, Square evens, Drive monotonic) |
| FUNC-03: Import slicing | must | ✅ Complete | 441000@44.1k → 215, 600000 → 256, 2048 → 1. tooShort / unreadable / tooLarge are rejected and leave the bank untouched. The no-dropout criterion is covered by the soak (16 held notes, 50 imports, finite) and the 0-alloc gate. |
| FUNC-04: Imported bank persists | must | ✅ Complete | G-FUNC04-ROUNDTRIP memcmp of 450780 bytes, plus filename and numFrames. pcm16gz is bit-identical. The negative control fails as it should. state-check P9/P10. |
| FUNC-05: Global LFO | must | ✅ Complete | mod-check: shapes vs analytic ≤ 3e-8, tempo phase error 1.3e-15, global phase bit-identical. pedalboard sweep. |
| FUNC-06: Mod ADSR → Position | must | ✅ Complete | mod-check FUNC-06. pedalboard ±1 sweeps. |
| FUNC-07: Poly 16 / Mono, amp ADSR, velocity | must | ✅ Complete | G-POLY, G-MONO, G-VEL. pedalboard Poly/Mono/velocity/16 voices. |
| DSP-01: Interpolation | must | ✅ Complete | G-DSP01. pedalboard: Off renders pos 0.300 and 0.305 bit-identically (snap); On differs by 4.6e-3 (morph). |
| DSP-02: Band-limiting | must | ✅ Complete | G-DSP02 Off −20.8 / On −114.4 dB. pedalboard C7 Saw32 Off −21.5 dB (aliases) / On −98.2 dB. |
| DSP-03: Bit depth | must | ✅ Complete | G-DSP03, G-FULL. pedalboard: 3-bit = 8 distinct levels, and Full is bit-identical to the default. |
| DSP-04: Normalization | must | ✅ Complete | G-PEAK 0.000003 dB, G-DC −181.8 dB |
| DSP-05: Effective position | must | ✅ Complete | mod-check DSP-05: the clamp holds, zipper 1.003, and the negative control fires |
| DSP-06: Off-thread build, RT-safe swap | must | ✅ Complete | G-ALLOC-24: 0 allocations over 200 switches + 50 publishes from a 2nd thread. Reaper gates. |
| PERF-01: RT-safe processBlock | must | ✅ Complete | dsp-check `--alloc-check` 0, mod-check 0, G-ALLOC-24 0. pluginval strictness 10. |
| QUAL-01: No unintended artifacts | must | ⚠️ Partial | Click gates, G-FINITE, G-BLOCK, block-size invariance and pluginval fuzz all pass. **But the critic found 2 clicks that pedalboard confirmed:** a voice-steal hard stop (W1) and a Mono retrigger velocity step (W2). See Issues Found. |
| QUAL-02: Alias ≥ 60 dB down to C8 | must | ✅ Complete | Strict set −114.3 / −74.9 dB. Narrow pulses −67.0 dB equal-RMS (**D-B NAMED EXCEPTION**, user-approved). pedalboard C8 Drive32 −93.2 / −93.4 / −98.2 dB @ 44.1/48/96k; this is the analyzer floor. |
| QUAL-03: No clicks on switch / import / toggles | must | ✅ Complete | import-check exactness + ratio (worst 1.31). pedalboard host-side switches 0.93–1.02. |
| COMPAT-03: WAV / AIFF / FLAC at any rate | should | ✅ Complete | 9/9 bit-identical banks, plus 24-bit, float NaN scrub, and stereo / 6-ch mean |

**Requirements Summary:**
- ✅ Complete: 17 (stage-2) + 1 (COMPAT-01, stage-1) = 18
- ⚠️ Partial: 1 (QUAL-01)
- ⏸️ Deferred (later stage): 11 (stage-3: 7, stage-4: 4)
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3/AU) | ✅ Pass | `ninja: no work to do` at HEAD `c9727200`. The installed bundles are byte-identical to the artefacts. |
| bank-check | ✅ ALL PASS | Fresh Debug tree |
| dsp-check + `--alloc-check` | ✅ ALL PASS, 0 allocs | Values match SUMMARY exactly |
| mod-check | ✅ ALL PASS | |
| import-check | ✅ ALL PASS | Soak this run: 2801 blocks (SUMMARY: 3163). The count is timing-dependent; deref 0 and max held 2 in both. |
| state-check | ✅ 11/11 | |
| `auval -v aumu OSiW OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED. Same 2 benign skew-default warnings as Stage 1 (`lfo_rate`, `amp_attack`). |
| pluginval VST3 strictness 10 | ✅ Pass | exit 0, 0 FAILED |
| pluginval AU strictness 10 | ✅ Pass | exit 0, 0 FAILED |
| Independent pedalboard probe | ✅ 27/27 (+2 critic repros FAIL, see Issues) | See the table above. State round trip: all 21 params randomised → fresh instance → 0 mismatches, render bit-identical. |
| Test hooks absent from the shipped binary | ✅ Pass | `nm` / `strings` on the installed VST3: 0 hook symbols |
| Non-ASCII in C++ Source | ✅ 0 | The only hits are comments in the vendored `ui/public/modules/webview-drop-streaming.js` |

## Human Verification (non-blocking)

Task 23 DAW smoke. Run whenever convenient; the installed-binary checks above cover the substance.

- [ ] Square and S&H LFO at 100% depth on a held pad: no clicks
- [ ] Bank switch while notes are held: no click
- [ ] Octave pitch-bend sweep: no zipper or level-step click
- [ ] Save/reopen a project with Imported. Without a UI yet, this needs a session that already carries `IMPORTED_BANK`; it can wait for Stage 3.
- [ ] Stage 1 Task 14 checks (Logic AU instrument listing, VST3 load, save/reopen)

## Issues Found

### Critic review (DSP + architecture, read-only): 0 blockers, 5 warnings, 8 notes

The critic cleared the high-risk areas:
- RT-safety, including the voice steal
- the reaper and the D-C amendment: no use-after-free in any traced case; at most 1 bank pinned when the host goes idle
- smoother seeding, NaN and denormal handling
- oversized blocks
- malformed `IMPORTED_BANK`
- test-hook isolation

| # | Finding | Repro on installed VST3 | Severity |
|---|---------|------------------------|----------|
| W1 | **A voice steal clicks.** `WtVoice.h:279-283`: `stopNote(..., false)` resets the amp env, so the stolen voice drops to 0 in one sample. | **Confirmed.** 16 held sines + a 17th note: max \|Δy\| is 8.7× steady state (no-steal control 1.5×). | QUAL-01 |
| W2 | **The velocity gain steps on a reused voice.** `WtVoice.h:418-422`: `velGain = v²` is set instantly. | **Confirmed in Mono:** vel 127 note released, then retriggered at vel 38 → 0.489 → 0.042 in one sample, \|Δy\| 10× steady state. **Not in Poly**: JUCE starts a fresh voice and the old one keeps releasing. | QUAL-01 |
| W3 | **The first Mono note after Poly→Mono can play bent.** An idle voice 0 never saw the wheel return to centre (`WtVoice.h:412-416`, `PluginProcessor.cpp:384-389`). | Not reproduced; needs a wheel move then a mode switch. | Edge case |
| W4 | **A big upward legato jump aliases for about 5 ms.** The frozen crossfade source is the old, richer mip level read at the new pitch (`WtVoice.h:604-610`). | Not reproduced; the critic estimated −21 dB for a 60→96 jump. | QUAL-02 edge (transient) |
| W5 | **ARCHITECTURE §17 is wrong about bank reads.** It still says the message thread may read the bank pointer directly. Since Amendment 8 the worker and `setStateInformation` publish, so Stage 3 must use `getImportedBankSnapshot()`. | Doc only | **Fix before Stage 3** |

Notes, none blocking:
1. Poly↔Mono switch hard-stops voices; user-initiated, so accepted.
2. Interp-Off frame-step ticks on Formant and Imported frames. This is the accepted CONTEXT decision.
3. The MIDI scratch buffers are pre-sized and could reallocate past about 3,600 events per block.
4. There is no guard for `processBlock` being called before `prepareToPlay`.
5. Mono ignores sustain.
6. Import jobs can't be cancelled mid-decode.
7. Save/restore hold `bankStateLock` while decoding.
8. `#if OSIW_TEST_HOOKS` is tested without the defining include (`-Wundef`).

### Other

- **Harness note:** a first pedalboard state check compared `string_value` after a binary-search parameter set and flagged 1-ulp display-rounding flips (0.4195 → "0.420" vs "0.419"). The render was bit-identical. Re-running with exact normalised values gave 0 mismatches. This is not a defect.
- The soak block count differs from SUMMARY (2801 vs 3163) because the run is wall-clock driven. The pass criteria are count-independent.

## Stage Verdict

**Status:** ⚠️ PARTIAL. Every planned gate passes, and 17 of the 18 stage-2 requirements are complete. QUAL-01 is partial because of 2 confirmed clicks in normal playing (W1, W2).

**Ready for next stage:** No. The user chose **fix now** on 2026-10-06, so a gap-closure execute run is next.

## Gap Closure (for `/plugin-execute O-simpleWavetable 2-dsp`)

**Scope:** W1 + W2 + W5 only. W3 and W4 are deferred to Stage 4 and logged here.

1. **W1, steal tail fade.**
   - On a hard stop (`stopNote(…, false)` from a steal and from the Poly↔Mono switch), capture the voice's last output sample per channel.
   - Add it back to the new note as a 1–2 ms linear or raised-cosine decay to 0.
   - RT-safe: per-voice members only, no allocation.
2. **W2, velocity ramp.**
   - When `startNote` / `noteOnDirect` retriggers a voice that is still sounding (amp env active), ramp `velGain` from the old value to the new one over about 2–5 ms (per-sample one-pole or linear).
   - A start from idle keeps the instant `velGain`, so G-VEL and the goldens stay unchanged.
3. **Gates to add in `dsp-check`:**
   - **G-STEAL:** 16 held + a 17th note. Max \|Δy\| at the steal ≤ 1.5× steady state. Negative control with the fade disabled must be ≥ 4×.
   - **G-RETRIG-VEL:** Mono vel 127 → release → vel 38 retrigger, same ratio rule, with a negative control.
   - **Expect** G-POLY, G-MONO, G-RETRIG, G-VEL and the QUAL-03 exactness/ratio gates to stay unchanged. If a golden moves, explain why.
4. **W5, doc fix:**
   - ARCHITECTURE §17: Stage 3 reads the imported bank **only** through `getImportedBankSnapshot()`, because the worker and `setStateInformation` publish (Amendment 8).
   - Correct the `unique_ptr` wording at line 157.
5. **Re-run:**
   - all 5 drivers plus `--alloc-check`
   - `build-and-install.sh`
   - auval, and pluginval VST3/AU strictness 10
   - the W1/W2 pedalboard repros (scratch `w12.py` / `w2m.py` recipe: 16 sines + a 17th at 0.5 s; Mono 127 → off → 38)

**Deferred to Stage 4:**
- W3 (wheel seed on the Mono idle start)
- W4 (crossfade source at the new mip level on upward legato)
- Notes 3–8 (MIDI buffer limit, pre-prepare guard, Mono sustain, import cancel, restore lock comment, `-Wundef` include)

**Blockers:** None from the critic. W1 and W2 are confirmed QUAL-01 shortfalls.
