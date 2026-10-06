# Stage 2 (DSP) — PLAN

**Plugin:** O-simpleWavetable · **Stage:** 2 of 4 (DSP) · **Phases:** 2.1 Bank Engine · 2.2 Voice + Oscillator · 2.3 Modulation → Position · 2.4 Bank Switch / Import / Persistence · **Date:** 2026-10-05
**Inputs:** `stages/2-dsp/CONTEXT.md` (user decisions), `stages/2-dsp/RESEARCH.md` (authoritative on *how*; Part A = 2.1/2.2, Part B = 2.3/2.4), `parameter-spec.md` (LOCKED, 21 params), `research/ARCHITECTURE.md`, `ROADMAP.md` §Stage 2, `REQUIREMENTS.md`, `stages/1-foundation/{SUMMARY,VERIFICATION}.md`, the Stage 1 source, `mockups/v1-PluginEditor.cpp` (API names) and `mockups/v1-integration-checklist.md`.
**Precedence when sources disagree:** user decisions (this plan's D-table + CONTEXT) > RESEARCH > parameter-spec > ARCHITECTURE > ROADMAP > agent-template defaults.

---

## Decisions locked for this plan

### User sign-offs (2026-10-05, at plan time)

| ID | Decision | Effect |
|----|----------|--------|
| **D-A** | **Floor the band-limited mip level at 1.** `L = clamp(ceil(log2(f·2048/fs)), 1, 10)` when Band-limit is On; Off stays L = 0. | `wt::kMinBandLimitedLevel = 1`. The worst-case sweep is −74.9 dB at every rate. No note changes at 44.1/48 kHz. ARCHITECTURE's L formula is amended (Task 21). |
| **D-B** | **Narrow pulses are judged on the equal-RMS metric, as a named exception.** Pulse 16/24/32 must be ≤ −60 dB relative to an equal-RMS sine (model −67 dB). The strict QUAL-02 gates (C8 ≤ −100, sweep ≤ −70) run on Drive 32, Pulse frame 1 and Saw-1023. | Gate G-Q2-PULSE. The exception is written into the dsp-check output, SUMMARY and the REQUIREMENTS QUAL-02 note, so a green gate cannot hide it. |
| **D-C** | **Amend the REG-01 reaper port.** The audio thread publishes `audioHeldBank` and the sweep never frees it. The reaper atomics use seq_cst. | Proved without ASan. Test builds park reaped banks in a graveyard. With `disableHeldExclusion` the dereference-after-reap count must be ≥ 1; with the fix on it must be 0. |

### Planner resolutions (RESEARCH recommendations adopted; no user input needed)

| ID | Resolution |
|----|-----------|
| D-D | **2.2 owns the 20 ms position-knob `SmoothedValue`**, so the listening checkpoint has no zipper noise. |
| D-E | **Import API names come from the Stage 3 template:** `importFromFile`, `importFromMemory`, `getImportStatus`, `getImportStatusVersion`, `getBankDisplayGeneration`. CONTEXT's `requestImport` was only an example. |
| D-F | **The "1023-harmonic frame" for QUAL-02 is Pulse frame 1.** The processor can reach it, and it measures the same as Saw-1023. A voice-level Saw-1023 arm is added as well, so the ROADMAP wording is met literally. |
| D-G | **QUAL-03 has two tiers.** The exactness tier (output = ideal 5 ms crossfade of the old and new renders, within 1e-4) covers every trigger. The ratio-tier click detector covers bank and import swaps, where a discriminating stimulus exists. |
| D-H | **One per-block context:** RESEARCH Part B's `BlockContext` (per-sample `knobPos`, `lfo`, `lfoDepth`, `envAmount` arrays plus the resolved `bank`, `interp`, `bandlimit`, `bitDepthIndex`). Voices index it at the **absolute** in-buffer index `startSample + i`. The chunk loop passes absolute offsets, so there is no `chunkOrigin`. In 2.2 `lfo`/`lfoDepth`/`envAmount` point at zero-filled buffers. |
| D-I | **20 ms smoothing for `lfo_depth` and `env_amount` too** (RESEARCH A5). It adds two global smoothers, and depth/amount automation then has no zipper. This is a documented addition to ARCHITECTURE. |
| D-J | **S&H draws from a hash of (seed, cycleIndex)** (splitmix64), not a sequential `juce::Random`. That is deterministic, independent of block size, and repeats on a transport loop. |
| D-K | **Downward-only mip-level hysteresis** (2.4, together with the crossfader). Moving up a level switches immediately; moving down needs `f < boundary·2^(−1/24)`. This stays alias-safe and stops crossfade storms from bend jitter. G-Q2 is re-run after 2.4 to prove it. |
| D-L | **`synth.setMinimumRenderingSubdivisionSize (1, true)`** (2.3), so renders are invariant to block size (gated). |
| D-M | **Auto-select Imported** uses a `juce::AsyncUpdater` (the harness drives it with `handleUpdateNowIfNeeded()`), not `callAsync` as ROADMAP has it. The console harness has no message loop. |
| D-N | **Position smoother starts as one pole** (`kPoles = 1`, τ = 2 ms, the locked spec). If the Square/S&H 100%-depth click gate fails against its negative control, switch to `kPoles = 2` (2 × 1 ms). That fallback is pre-authorized and must be logged in SUMMARY. |
| D-O | **VERSION stays `0.1.0`** through Stages 2 and 3 (sibling convention: O-simpleAdditive went 0.1.0 → 1.0.0 at Stage 4). Bus layout is unchanged, so Logic's per-version AU I/O cache is not affected. |

### Already settled (do not re-ask)

- **Mono = true legato.** An overlapping note changes pitch only; the amp env, mod env and position smoother all continue. Last-note priority, and release returns to the previous held note.
- **Velocity is squared:** `velGain = v²`, with JUCE's v already in 0..1.
- Interp-Off step click accepted; empty Imported = silence; ±2 st bend; mid-rise quantizer; reject imports under 2048 samples; per-frame normalise capped at +24 dB; `lfo_div` default 1/1.
- parameter-spec and ARCHITECTURE §11 still say Mono "retriggers" and velocity is "linear". Task 21 corrects them.

---

## Goal

Make O-simpleWavetable a playable, RT-safe wavetable synth with **every DSP requirement gated offline**:
- five built-in 32-frame banks with strict 11-level mipmaps;
- a 16-voice Poly / true-legato Mono oscillator with a band-limited linear read, latched or interpolated frames, a mid-rise bit quantizer, amp ADSR, squared velocity, ±2 st bend and a seeded output stage;
- position modulation from a global LFO (free or tempo, five shapes, deterministic S&H) and a per-voice mod envelope, through a clamp and a 2 ms smoother;
- bank, level, interp and bandlimit changes crossfaded through a frozen cycle;
- WAV/AIFF/FLAC import on a worker thread, published lock-free with the amended REG-01 reaper;
- an `IMPORTED_BANK` (FLAC16 base64) that restores bit-identically.

The editor stays generic until Stage 3. Stage 3 drives import through the same public API that the harness uses.

---

## Execution model

The work splits into **Part 1 (2.1 + 2.2)** and **Part 2 (2.3 + 2.4)**, with a **blocking user listening checkpoint** between them (CONTEXT cadence). Each part ends in its own commit.

| Who | Tools | Tasks |
|-----|-------|-------|
| **dsp-agent**: dispatch A (2.1 + 2.2), dispatch B (2.3), dispatch C (2.4) | Read / Write / Edit only. No Bash and no builds. | 2–8, 13–15, 16–19 |
| **Orchestrator** (plugin-workflow execute) | Bash. Long commands run with `run_in_background: true` and log to `$SCRATCH`, the execute session's scratchpad (memory `pattern_executor_watchdog_stall_run_long_commands_in_background`). | 1, 9–10, 12, 20–24 |
| **Taylor** (human checkpoint) | DAW, generic editor | 11 |

**Execute-phase stop rule:** `/plugin-execute O-simpleWavetable 2-dsp` runs Tasks 1–10, presents Task 11 (the listening checkpoint) and **STOPS**. STATUS is then `stage_2_part1_complete_listening_pending`. After Taylor's sign-off, a second `/plugin-execute O-simpleWavetable 2-dsp` reads STATUS and resumes at Task 12. If the checkpoint finds tone or feel problems, fix them first: amend Part 1, re-run Tasks 9–10, and repeat the checkpoint.

**Agent overrides** (they apply to every dispatch and supersede the dsp-agent template):
- **Read these contracts:** CONTEXT, RESEARCH (the sections named in each task), this PLAN, parameter-spec, and `troubleshooting/patterns/` for Stage 2. Where RESEARCH gives ready code, **transcribe it** with the amendments listed in the task.
- **ASCII-only sources, comments included** (Stage 1 gate). RESEARCH snippets contain → — … ± ² · in comments; replace them with `->`, `-`, `...`, `+/-`, `^2`, `*`.
- **`-Wfloat-equal` is on.** Use `juce::exactlyEqual`, never `==` on floats.
- Resolve choice params with `jlimit (0, N-1, (int) std::round (v))`, never a truncating cast.
- **Every new `.cpp` goes in `target_sources`.** The processor-console helper only compiles what is listed there.
- **Never call `synth.*` from the message thread.** Detect Poly/Mono switches in `processBlock`.
- Do **not** edit STATUS.md, PLUGINS.md, ARCHITECTURE, parameter-spec, REQUIREMENTS or CHANGELOG. The report must say `"stateUpdated": false` (deferred to the orchestrator).
- Do **not** touch `Source/PluginEditor.*` (generic editor; Stage 3), `mockups/`, or other plugins.
- `OSIW_TEST_HOOKS` members and branches compile **only** under `#if OSIW_TEST_HOOKS`, which is defined only on test targets. The shipped binary must not contain them.

---

## Tasks

### Part 1: Phases 2.1 + 2.2

#### Task 1 — Pre-flight  *(orchestrator)*
- **Do:**
  - `git branch --show-current` must be `main`.
  - `git status --short -- plugins/O-simpleWavetable` must be clean. Otherwise stop and report: something else is touching this plugin.
  - Contract tamper check: the `shasum -a 256` of BRIEF, parameter-spec, ARCHITECTURE and ROADMAP must match STATUS `contract_checksums`. Stop on a mismatch.
  - Confirm Stage 1 is VERIFIED (`stages/1-foundation/VERIFICATION.md`).
  - Run `bash -c '.planning/workflow/scripts/run-gate.sh O-simpleWavetable 1-foundation 2-dsp --skip-review'`.
    - Exit 0 → proceed.
    - Exit 1 → read the failed check. Bypass with `--force` and a justification on stdin only if the failure is a known gate limitation, never a real build or pluginval failure.
- **Depends on:** none.
- **Done:** on `main`, plugin tree clean, contracts intact, gate passed or bypass logged.

#### Task 2 — Bank data layer  *(agent, dispatch A)*
- **Files:** create `Source/WavetableBank.h`, `Source/MipmapBuilder.{h,cpp}`.
- **Do:** transcribe RESEARCH §5.2, with these amendments:
  - `WavetableBank`: `kTableSize 2048`, `kStride 2049` (guard = s[0]), `kLevels 11`, `kMaxFrames 256`, `kmax(L) = L==0 ? 1023 : 1024>>L`, a flat `data` vector, and `frame(level, f)` accessors. It is **immutable after build**.
    - Add `std::vector<float> thumbs` (`numFrames × 128`, decimated from level 0) for the Stage 3 `bankUpdate` seam.
    - Under `#if OSIW_TEST_HOOKS`, add `mutable std::atomic<bool> testReaped` and `static inline std::atomic<int> testLiveCount` (ctor ++, dtor --).
  - `MipmapBuilder` is **reentrant**: each instance owns its own `juce::dsp::FFT(11)` and scratch, with no statics.
    - API: `buildFromSpectrum`, `buildFromTime (…, Normalise)`, plus a free function `std::unique_ptr<WavetableBank> buildFromLevel0 (const float* level0, int numFrames, const juce::String& name)` for 2.4.
    - Zero the DC bins (`d[0]`, `d[1]`) and the **Nyquist bin** (`d[2048]`, `d[2049]`).
    - The peak normalisation scales the **spectrum** once, from level 0. Levels are never re-normalised.
    - Do not port Prism's negative-bin loop.
- **Must not:** port O-Prism's read path or trilinear level blend (RESEARCH §3.1).
- **Depends on:** none.

#### Task 3 — `BankFactory` + `BuiltInBanks`  *(agent, dispatch A)*
- **Files:** create `Source/BankFactory.{h,cpp}`, `Source/BuiltInBanks.{h,cpp}`.
- **Do:** the five generators, exactly per RESEARCH §5.1:
  - Sine→Saw: `b_n = 1/n` for n ≤ k.
  - Sine→Square: the `c(k)` odd-harmonic ramp.
  - Pulse Width: **cosine phase**, `d_k = 0.5·(1/16)^((k−1)/31)`.
  - Formant: `f_ref` 110, 48 harmonics, three resonators, log-F interpolation between the five vowel anchors.
  - Drive: time domain `tanh(g·sin)/tanh(g)` with `g_k = 0.25·100^((k−1)/31)`, through the forward FFT. No oversampling.
  - Sine coefficient `d[2n+1] = −0.5·N·b_n`; cosine `d[2n] = 0.5·N·a_n`.
- `BuiltInBanks`:
  - `std::array<WavetableBank,5>`, index == bank param 0..4.
  - `buildMillis` via `Time::getMillisecondCounterHiRes()`.
  - `get(int)` accessor.
  - Its ctor **must not** construct a `SharedResourcePointer<BuiltInBanks>` (deadlock).
- Bank `name` fields are ASCII (`"Sine->Saw"`, …). The UI uses the param choice strings.
- **Depends on:** Task 2.

#### Task 4 — Shared read + quantizer + mono stack headers  *(agent, dispatch A)*
- **Files:** create `Source/WtRead.h`, `Source/BitQuantizer.h`, `Source/MonoStack.h`.
- **Do:**
  - `WtRead.h`: RESEARCH §6.1 verbatim, with `kMinBandLimitedLevel = 1` (**D-A**). It contains `selectLevel`, `lerpCycle`, `readSample` and `latchFrame`.
    - Clamps must be NaN-safe: `! (x >= 0)` → 0; `i0 ≤ 2047`; frame indices clamped to `nF−1`.
    - `readSample` is the single reference read. The voice inlines the same arithmetic, and G-READ proves they agree.
  - `BitQuantizer.h`: RESEARCH §6.1, mid-rise. Choice index 0 = Full, an early-out with a bit-identical bypass; index i → `17 − i` bits.
  - `MonoStack.h`: copy verbatim from `plugins/O-simpleSubtractive/Source/PluginProcessor.h:85-131` (fixed `kCap = 32`, no allocation).
- **Depends on:** Task 2.

#### Task 5 — `WtVoice` (replaces the Stage 1 silent voice)  *(agent, dispatch A)*
- **Files:** rewrite `Source/WtVoice.h` (add `WtVoice.cpp` to `target_sources` if one is created).
- **Do:** RESEARCH §6.2, adapted to the **D-H** `BlockContext`:
  - **Context:** `setBlockContext (const BlockContext*)` is set once in `prepareToPlay`; the pointer stays stable. Each render reads `ctx->knobPos[startSample + i]`. In 2.2 the effective position is the knob value plus 0 from the zero buffers.
  - **prepare:** call `ampEnv.setSampleRate` **before** `setParameters`, then record `appliedAmp`.
  - **ADSR updates:** `setBlockParams` dirty-checks with `juce::exactlyEqual`, skips while releasing, and applies deferred params at note-on (O-simpleAdditive pattern).
  - **`startNote`:**
    - Read `wasIdle = ! ampEnv.isActive()` first.
    - Set `noteHz`, the wheel position and the pitch.
    - `velGain = v*v`; `noteAge = ++sNoteCounter`; `phase = 0`.
    - `needsLatch = cfgFresh = true`.
    - Then `ampEnv.noteOn()`.
  - **`stopNote`:** with tailOff, release; otherwise reset and clear. **Do not** clear note-keyed state.
  - **Bend:** `bendSemis = (wheel − 8192)/8192·2`; `inc = min(hz/sr, 0.49)`.
  - **Render** (RESEARCH §6.2 step 4 order):
    - Mono output to channel 0 only.
    - The level and `ReadCfg` are recomputed at the top of **every** render call (sub-blocks).
    - A config change in 2.2 is a **hard switch** that also re-latches the frame. That is a `checkConfig` hook with an empty body, which 2.4 fills.
    - `ReadCfg {bank, level, interp, bandlimit}` is kept only while the voice is active; `startNote` overwrites it with no capture.
  - **Lifetime:** the voice ends with the amp env only. Then call `clearCurrentNote()`.
  - **Mono API:**
    - `noteOnDirect (note, vel, retrigger)`. On retrigger, call `ampEnv.noteOn()` with **no `reset()`**, and reset the phase only if the voice was not sounding. Without retrigger, change pitch only.
    - `setPitchNote`, `noteOffDirect`.
  - **Display getters:** `isSounding()`, `getNoteAge()`, `getLastPos()`, `getLastLevel()`, `getLastFrame()`.
  - **Must not** declare a method named `isVoiceActive`.
  - `kVoiceGain = 0.5f` (tuned at the listening checkpoint).
- **Depends on:** Tasks 2 and 4.

#### Task 6 — Processor: 2.2 `processBlock`, mono path, output stage  *(agent, dispatch A)*
- **Files:** edit `Source/PluginProcessor.{h,cpp}`.
- **Do:** RESEARCH §6.3 with **D-H**.
  - **New members:**
    - `juce::SharedResourcePointer<BuiltInBanks> builtIns`, declared before anything that reads it;
    - `std::atomic<const WavetableBank*> importedForAudio { nullptr }` (2.4 publishes into it; for now Imported = silence);
    - `BlockContext blockCtx`;
    - preallocated `knobBuf` and `zeroBuf`;
    - `chunkMidi` (`ensureSize (32768)` in prepare);
    - `outputGain`, `knobSmooth` (20 ms, **D-D**);
    - `monoStack`, `lastVoiceMode`, `preparedBlock`;
    - display atomics `dispPos`, `dispLevel`, `dispFrame`, `dispSounding` (relaxed).
  - **`processBlock`:**
    - Structure: one exit path or a scope guard. 2.4 bumps `blockEntries` first and `blockGeneration` on **every** return.
    - Order: `buffer.clear()`, the 0-sample/0-channel early return, then the collector drain.
    - Detect a mode change: `synth.allNotesOff(0,false)` and `monoStack.clear()`.
    - Resolve the bank **once** per block.
    - Push block params to the voices.
    - Chunk loop of `preparedBlock` with **sliced MIDI at absolute positions** (never the full buffer to a sub-range render).
      - Fill `knobBuf` from `knobSmooth`, which takes `finiteOr (param, 0)`.
      - Poly: `synth.renderNextBlock (buffer, chunkMidi, start, n)`.
      - Mono: `renderMono` (Subtractive `renderMonoLegato` with `retrigger = ! wasHeld`).
    - Lead-voice display update: newest `noteAge` among `isSounding()`.
    - Output: `outputGain` ramp (`decibelsToGain (db, −60)`, so −60 dB is exactly 0), the `isfinite` scrub on channel 0, then copy to the other channels.
  - **`prepareToPlay`:** `preparedBlock = max(1, spb)`; size the buffers; **seed** `outputGain` and `knobSmooth` with `setCurrentAndTargetValue` after `reset`; voice prepare; `monoStack.clear()`; set `lastVoiceMode` to the current mode.
  - **Mono wheel:** feed wheel-only events to `synth.renderNextBlock (buffer, wheelMidi, start, 0)` so that `lastPitchWheelValues` stays current across a Mono→Poly switch (RESEARCH §6.3).
- **Must not:** allocate in `processBlock`, call `setParameters` unconditionally per block, or touch the editor.
- **Depends on:** Tasks 3 and 5.

#### Task 7 — CMake: sources + Part 1 test targets  *(agent, dispatch A)*
- **Files:** edit `CMakeLists.txt`.
- **Do:**
  - Add every new `.h`/`.cpp` to `target_sources`.
  - Inside the existing `if(OUARICON_BUILD_TESTS AND APPLE)` block, add `ouaricon_add_processor_console(… tests/bank-check/main.cpp bank-check)` and `(… tests/dsp-check/main.cpp dsp-check)`.
  - Add `target_compile_definitions(O-simpleWavetable-dsp-check PRIVATE OSIW_TEST_HOOKS=1)`.
  - Leave `VERSION "0.1.0"` and everything else unchanged.
- **Depends on:** Tasks 2–6.

#### Task 8 — Part 1 harnesses + P6 flip  *(agent, dispatch A)*
- **Files:**
  - create `tests/bank-check/main.cpp`;
  - create `tests/dsp-check/main.cpp`;
  - edit `tests/state-check/main.cpp` (P6 only).
- **Do:**
  - **`bank-check`:** every gate in RESEARCH §5.3: G-DIM, G-PEAK, G-DC, G-GUARD, G-SAW, G-SQ, G-DRIVE, G-PULSE, G-FORM, G-BL, G-POL, G-TIME (log only), plus the **G-NEG** negative control (a Nyquist bin left in must FAIL G-BL). Print every measured value. Exit non-zero on any failure.
  - **`dsp-check`:** every gate in RESEARCH §6.6, with these amendments:
    - **G-Q2-C8:** Drive 32, Pulse frame 1, plus a voice-level Saw-1023 arm (**D-F**). MIDI 108 at 44.1/48/96 kHz, band-limit On, threshold ≤ −100 dB.
    - **G-Q2-SWEEP:** the same frames, MIDI 21..108, all three rates, ≤ −70 dB. Requires D-A.
    - **G-Q2-PULSE** (**D-B**): Pulse 16/24/32 against an equal-RMS sine, ≤ −60 dB. It prints `NAMED EXCEPTION (D-B): narrow pulses judged on equal-RMS metric` together with the strongest-harmonic value, for the record.
    - The Kaiser-38 analyzer runs per RESEARCH §6.5 (FFT 17, ±14-bin mask, print the worst bin).
    - The rest of §6.6: G-PITCH, G-BEND, G-DSP02 (also the liveness control for the analyzer), G-DSP01 (fs 56320), G-DSP03, G-FULL, G-READ, G-POLY, G-MONO, G-RETRIG, G-NOTEOFF, G-VEL, G-OUT, G-IMPEMPTY, G-BLOCK, G-FINITE.
    - Every gate has the negative control that §6.6 lists.
    - `--alloc-check` mode: the O-Bells `malloc_logger` gate (volatile flag and counter, audio-thread scoped, a liveness `malloc(64)` must count 1, warm-up block unarmed), over the G-ALLOC stimulus list.
  - **`state-check` P6 → P6′:** note-on, 20-note and UI-MIDI blocks are finite and bounded (|x| ≤ 4); a 0-sample block is safe; output −60 dB gives exactly 0. All other probes stay unchanged.
- **Depends on:** Tasks 2–7.

#### Task 9 — Static gates + offline gate run  *(orchestrator)*
- **Static gates**, over `plugins/O-simpleWavetable/{Source,tests,CMakeLists.txt}`:
  - Non-ASCII byte count = 0. The only allowed exception is the existing hex escapes, which are ASCII text.
  - `grep -c PLUGIN_VERSION` = 0.
  - `grep -n 'isVoiceActive'` in `Source/` = 0 definitions.
  - `grep -n 'ScopedLock\|new \|std::vector<.*>.*(push_back|resize|assign)'` inside `processBlock`/`renderNextBlock`: inspect every hit (the alloc gate is the real proof).
  - Every `Source/*.cpp` appears in `target_sources`.
- **Offline gates:** the Stage 1 out-of-repo **Debug** recipe (`$SCRATCH/build-oswt`, `-DOUARICON_BUILD_TESTS=ON`, `SKIP_PLUGINS` = every other plugin; never set these in the shared `build/`).
  - Build `O-simpleWavetable-{bank-check,dsp-check,state-check,param-dump}` in the background.
  - Run each one. Each needs exit 0 **and** `grep -c 'JUCE Assertion failure'` = 0. Then run `dsp-check --alloc-check`.
  - Save the full logs in `$SCRATCH`; SUMMARY quotes the key lines.
- **On failure:** route the fix back through dsp-agent with the failing gate's output. Do not loosen thresholds. The QUAL-02 sweep margin is about 5 dB.
- **Depends on:** Task 8.

#### Task 10 — Build, install, host-validate, pedalboard, commit Part 1  *(orchestrator)*
- **Do:**
  1. `./scripts/build-and-install.sh O-simpleWavetable` in the background. It sweeps both `-dev`/unsuffixed variants and clears the AU caches. Then `ninja -C build O-simpleWavetable_Standalone`, because the script skips the Standalone.
  2. Targeted AU check: `bash scripts/verify-au-link.sh O-simpleWavetable` (`auval -v aumu OSiW OuDv`, backgrounded). Never run `auval -a`.
  3. pluginval strictness 10 on the VST3 and the AU. The NaN param fuzz exercises the out-of-bounds/NaN clamps.
  4. **Pedalboard on the installed VST3** (`uv run --python 3.12 --with pedalboard --with numpy`; params set by `raw_value`; the output key is `output_level_db`). Assert:
     - finite output;
     - C4 f0 within 1 cent;
     - bank Imported renders exactly 0;
     - at C7 on Saw 32, Band-limit Off vs On differ by more than 60 dB of alias.
  5. **Commit Part 1.** Re-check `git branch --show-current` and `git status --short` immediately before.
     - Use the temp-index CAS commit, scoped to `plugins/O-simpleWavetable` (memory `index_git_shared_checkout`; run under `bash -c`).
     - PLUGINS.md is not touched in Part 1: the row stays "🚧 Stage 1" until Task 24.
     - Message: `feat(O-simpleWavetable): Stage 2 Part 1 - bank engine + voice/oscillator (2.1+2.2)`. The body lists the gates that passed and D-A/D-B.
  6. **STATUS.md:** `status: stage_2_part1_complete_listening_pending`, `next_action: user_listening_checkpoint_then_plugin_execute_stage_2`, plus a line under Completed So Far. Commit it with the same temp-index discipline (or fold it into the Part 1 commit).
- **Depends on:** Task 9 all green.

#### Task 11 — **Listening checkpoint**  *(human-verify, Taylor; BLOCKING)*
- **Surface:** Logic AU or Reaper VST3 with the generic editor, or the Standalone.
- **Script** (RESEARCH §10.5):
  - **Bank character:** each bank at Position 0 / 50 / 100 %, with C2–C5 chords.
  - **Interp Off:** 32 audible steps on Saw and Square. **Interp On:** continuous.
  - **Band-limit A/B:** Drive at 100 %, C6–C8.
  - **Bit depth:** Full → 8 → 4 → 3 on a sine.
  - **Voices:** a 16-note chord; Mono legato (overlap = pitch only; release returns to the held note); ±2 st bend.
  - **Velocity:** soft playing is genuinely quiet.
  - **Loudness:** `kVoiceGain` 0.5 against the other O-simple* synths.
- **Known until Part 2** (not regressions):
  - clicks on bank switches, band-limit/interp toggles, and octave-crossing bends or legato moves;
  - Imported is silent;
  - Position has no LFO or mod env yet.
- **Outcomes:**
  - **Sign-off** → Task 12.
  - **Tone or feel issue** → fix it within Part 1 (for example `kVoiceGain`, or a bank formula only by explicit user decision), re-run Tasks 9–10, then repeat this checkpoint.
- The execute phase **STOPS here** and presents the checkpoint script. The handoff is: Step 1 `/clear`, Step 2 `/plugin-execute O-simpleWavetable 2-dsp`, after listening.

---

### Part 2: Phases 2.3 + 2.4  (resume after Task 11 sign-off)

#### Task 12 — Part 2 pre-flight  *(orchestrator)*
- STATUS shows a Part 1 sign-off. Record Taylor's notes in STATUS.
- Re-check `main` and a clean plugin tree. The Part 1 commit must be an ancestor of HEAD.
- **Depends on:** Task 11.

#### Task 13 — `PositionLfo` + global modulation buffers  *(agent, dispatch B)*
- **Files:** create `Source/PositionLfo.h`; edit `Source/PluginProcessor.{h,cpp}`.
- **Do:** RESEARCH §2.2 code, transcribed:
  - `kDivBeats` as **exact doubles** (`4.0/3.0`, …), never Prism's `2.6667f`.
  - One double phase, reset in `prepare`.
  - Tempo + playing + valid PPQ/BPM: the phase is **absolute per sample**, `frac((ppq0 + i·dppq)/beats)`.
  - Stopped, or no playhead: free-run at the host BPM, or 120.
  - Free mode: 0.01–20 Hz.
  - S&H uses the splitmix64 hash of the cycle index (**D-J**), with `setSeed` as a test hook.
  - Triangle starts at 0 going up.
  - `readTransport`: `getIsPlaying()` returns a `bool`; `getBpm`/`getPpqPosition` return `Optional`.
- **Processor (RESEARCH §2.3):**
  - Add `depthSmooth` and `amtSmooth` (20 ms, **D-I**). All smoothers are seeded with `finiteOr (param, 0)` in prepare.
  - Per chunk, fill `knobBuf`/`depthBuf`/`amtBuf` and `lfoBuf = lfo.render(...)`.
  - Point `blockCtx.lfo`/`lfoDepth`/`envAmount` at them, replacing the zero buffers.
  - Render the LFO even when depth is 0 (the display needs it).
  - Add display atomics `dispLfo`, `dispMenv`, `dispAmp`.
  - `synth.setMinimumRenderingSubdivisionSize (1, true)` (**D-L**).
- **Depends on:** Task 12.

#### Task 14 — Per-voice mod env, clamp, smoother  *(agent, dispatch B)*
- **Files:** edit `Source/WtVoice.h`; create `Source/PositionSmoother.h`.
- **Do:** RESEARCH §2.4 and §3.2–3.3:
  - Per-voice `modEnv`: `setSampleRate` before `setParameters`; dirty-checked; `noteOn` at `startNote` and on a Mono **retrigger** only; **no retrigger on a legato move**.
  - `raw = clamp01 (knob + depth·0.5·lfo + amt·modEnv)`, NaN-safe.
  - `PositionSmoother` with `kPoles = 1`, τ = 2 ms (**D-N**). It is **seeded at the first rendered sample after note-on** (`needsSeed`). With Interp Off it tracks `raw`, so an Off→On toggle never glides from a stale value. It is not reseeded on legato.
  - Interp Off latches `round(raw·(N−1))` at the phase wrap and at note-on.
  - Voice lifetime still ends with the **amp** env only (FUNC-06).
  - Test hooks: `smootherBypass`, `getEffPos()`.
- **Depends on:** Task 13.

#### Task 15 — 2.3 harness  *(agent, dispatch B)*
- **Files:** create `tests/mod-check/main.cpp`; edit `CMakeLists.txt` to add the `mod-check` console target with `OSIW_TEST_HOOKS=1`, and add `PositionLfo.h` and `PositionSmoother.h` to `target_sources`.
- **Do:** every gate in RESEARCH §7.6, with the shared click detector from §3.4 (2nd-difference **excess ratio** ≤ 1.5, a liveness check |A−B| ≥ 0.25, and a negative control that must be ≥ 4):
  - **DSP-05:**
    - clamp: `0 ≤ effPos ≤ 1` on every sample;
    - knob-step zipper: the negative control is `smootherBypass`.
  - **FUNC-05:**
    - all 5 shapes against analytic values within 1e-6;
    - free 0.5 Hz: the period is 96000 ± 1 samples at 48 kHz;
    - tempo: a synthetic `TestPlayHead` at 120 and 97.3 BPM, all 16 divisions, with random block sizes 1..4096; phase within 1e-9; loop-jump re-lock; stopped/no-playhead fallback;
    - **one global phase**: two voices 0.37 s apart have equal `effPos`.
  - **S&H determinism:**
    - byte-identical across two runs and across a re-prepare;
    - a different seed gives a different render;
    - a tempo loop replays the same values;
    - each render job gets its own seed.
  - **FUNC-06:**
    - ±amount gives up/down sweeps;
    - amp release 0.05 s with mod release 10 s: the voice is inactive within 0.1 s;
    - re-pushing the same ADSR every block does not change the release length.
  - **Click gate:** Square and S&H at 100 % depth, Interp On, Sine→Saw, pos 0.5, 2 Hz, A2, every edge. If it fails, apply **D-N** (`kPoles = 2`) and re-run; record which path shipped.
  - **Block-size invariance:** free mode, partitions 64/512/4096/random give a bit-identical render (needs D-L).
- **Depends on:** Task 14.

#### Task 16 — Frozen-cycle crossfader + level hysteresis  *(agent, dispatch C)*
- **Files:** edit `Source/WtVoice.h`.
- **Do:** RESEARCH §4. Fill the 2.2 `checkConfig` hook:
  - **Buffers:** ping-pong `float[2][2049]` per voice, preallocated, guard sample; `xfadeLen = round(0.005·fs)`, set in prepare.
  - **Triggers:** the bank pointer, level, interp or bandlimit changed. **Not** Interp-Off frame steps, and not bit depth.
  - **Capture:** render the current output cycle with `wt::readSample` from the **old** `ReadCfg`. Capture only from this voice's own `cfg.bank`.
  - **Fold rule:** a trigger during a running fade writes the mixed cycle `(1−w)·frozen + w·liveOld` into the free buffer, swaps, and restarts the fade. Never drop the outgoing buffer.
  - **Running length:** the `xfLenActive` member, overridable by the test hook `xfadeLenOverride`.
  - **D-K hysteresis:** going up a level switches immediately; going down needs `f < boundary·2^(−1/24)`.
  - **Checked** at the top of every render call: per block, and after each pitch-wheel sub-block.
  - Imported → nullptr fades to silence; nullptr → bank fades in.
  - Test hook: `forceLevel`.
- **Depends on:** Task 15.

#### Task 17 — Reaper (REG-01 + D-C amendment), publish, importer  *(agent, dispatch C)*
- **Files:**
  - create `Source/WavetableImporter.h` (and a `.cpp` if needed);
  - edit `Source/PluginProcessor.{h,cpp}` (make it `juce::AsyncUpdater` too).
- **Reaper** (RESEARCH §1.3, transcribed):
  - `blockEntries.fetch_add(1)` is the **first** statement after `ScopedNoDenormals`.
  - `blockGeneration.fetch_add(1)` runs on **every** exit, including the 0-sample/0-channel early return.
  - The audio thread stores `audioHeldBank` (= this block's resolved pointer) **before** the exit increment.
  - `bankStateLock` is a `mutable CriticalSection` that the audio thread never takes.
  - Members: `importedOwner` (`shared_ptr`) and `retiredBanks` vector, both under the lock.
  - `retireBank`: a producer sweep first, then push `{bank, blockGeneration.load()}`.
  - `sweepRetiredBanks`: load exits first, then entries; `quiescent = entries == exits`; load `held` after entries. Free when `bank != held && (quiescent || exits >= stamp + 2)`.
  - **seq_cst everywhere on these atomics**: a documented hardening deviation from Prism's acq/rel.
  - Sweep from a 250 ms `juce::Timer` and on every retire.
  - Also store `audioHeldBank = nullptr` in `prepareToPlay` and in `releaseResources`.
  - The bank is resolved **once** per block: `idx < 5 ? builtIns->get(idx) : importedForAudio.load()`.
  - `bankDisplayGen` is bumped on a param change (detected in `processBlock`, relaxed), on publish, and on restore.
- **Importer** (RESEARCH §5.2–5.3):
  - `WavetableImporter::decode`:
    - never trust the header length; cap at 256 × 2048 samples;
    - average **all** channels (cap 64);
    - scrub NaN/inf in float WAVs;
    - per frame: FFT, zero DC + Nyquist, IFFT, peak-normalise capped at +24 dB (`kMaxBoost`), then lround to int16 via `·32767`.
  - `toFloat (q) = q/32767` is the **only** int16 → float mapping.
  - **One** `buildImportedBank (const ImportedPcm&)`, shared by import, restore and the tests (FUNC-04).
  - Processor API (**D-E**):
    - `importFromFile (const File&)`;
    - `importFromMemory (name, MemoryBlock&&)`: > 96 MB → `tooLarge`; the name is sanitised (basename, no control characters, ≤ 128 chars);
    - `getImportStatus()`, `getImportStatusVersion()`, `getBankDisplayGeneration()`.
  - A single-thread `juce::ThreadPool importPool`, **declared last**; the destructor calls `removeAllJobs (true, 10000)`.
  - `importGen` supersedes in-flight jobs: cancel at each chunk check, and re-check under the lock before publishing.
  - Order: **publish, then auto-select.** `pendingAutoSelect` + `triggerAsyncUpdate()`; `handleAsyncUpdate` sets bank = 5 inside a begin/end gesture (**D-M**).
  - Status is polled (version counter), never pushed from the worker.
  - Error vocabulary: `tooShort | unreadable | tooLarge` (plus internal `cancelled`, never surfaced). Long files truncate to 256 frames and are not an error. A rejected import leaves the existing bank untouched.
- **Test hooks** (`#if OSIW_TEST_HOOKS`): `disableHeldExclusion`; graveyard mode (park banks instead of freeing them, set `testReaped`); `testDerefAfterReap` (counted at bank resolve); `getHeldBankCount()`; `sweepNow()`; `midBlockCallback`; `xfadeLenOverride`; `forceLevel`; `lfoSeed`.
- **Depends on:** Task 16.

#### Task 18 — `IMPORTED_BANK` persistence  *(agent, dispatch C)*
- **Files:** edit `Source/PluginProcessor.cpp` (fill the Stage 1 seams `writeImportedBank` / `restoreImportedBank`); edit `WavetableImporter.*` for the codecs.
- **Do:** RESEARCH §6.1–6.3, transcribed.
  - **Codecs:**
    - `encodeFlac16`: the new `createWriterFor (unique_ptr<OutputStream>&, AudioFormatWriterOptions)` API, mono 16-bit, nominal 48000, quality 5, left-justified `s·65536`. The **writer is destroyed before the block is read**.
    - `decodeFlac16`: validates the format name, 1 channel, 16 bits and the exact length; reads through the `int*` API, `>> 16`.
    - `encodePcm16Gz` / `decodePcm16Gz`: GZIP format; an exact-length read; trailing data is rejected.
  - **Base64:** `juce::Base64::toBase64` / `convertFromBase64` **only**. Never `MemoryBlock::toBase64Encoding`.
  - **Child format:** `<IMPORTED_BANK version="1" filename numFrames encoding data/>`. Every attribute is read as a string with an `isVoid()` gate.
  - **Restore validation:** `1 ≤ numFrames ≤ 256`, `data.length() ≤ 4 M` characters, decoded length == `numFrames·2048`.
  - **Forward compatibility:** an unknown version or encoding is re-emitted verbatim (passthrough) and sets status `unsupported`.
  - **Malformed v1** → empty bank, status `unreadable`.
  - **Restore is synchronous.** It bumps `importGen` to supersede any in-flight import, publishes through `publishImportedBank` (it retires, never frees), and caches the **incoming data string verbatim**, so a save after load is byte-identical.
  - **Save** writes exactly one child from the cache (no work in `getStateInformation`), or no child if nothing was imported.
  - Keep the Stage 1 strip-on-load structure exactly.
  - Presets never touch the bank.
- **Depends on:** Task 17.

#### Task 19 — 2.4 harness + state-check additions  *(agent, dispatch C)*
- **Files:**
  - create `tests/import-check/main.cpp`;
  - edit `tests/state-check/main.cpp` (add probes; P3 stays as the "malformed tolerated" probe);
  - edit `CMakeLists.txt` (`import-check` target with `OSIW_TEST_HOOKS=1`; add `WavetableImporter` to `target_sources`).
- **Do:** every gate in RESEARCH §7.1–7.5. The fixtures are generated in the harness with JUCE writers into a temp dir.
  - **QUAL-03, exactness tier** (**D-G**):
    - triggers: bank param, import publish (empty→X, X→Y, X→empty), bandlimit, interp, a level change via `forceLevel`;
    - `max|y − y_ideal| ≤ 1e-4`;
    - negative control `xfadeLenOverride = 0` must give ≥ 0.1.
  - **QUAL-03, ratio tier:**
    - Sine→Saw(0) ↔ Formant(0) at A1/A2 with 8 switch phases;
    - an import swap;
    - Imported → empty;
    - import during 16 held notes;
    - the **fold** case (a second trigger 1 ms into a fade);
    - ratio ≤ 1.5, with the liveness check and a negative control ≥ 4.
  - **DSP-06 / PERF-01:** alloc gate armed across 200 bank switches and 50 import publishes plus restore-publishes from a second thread while rendering. Expect 0 audio-thread allocations, with the liveness probe proven first.
  - **Reaper (§7.3):**
    - quiescent: with bank = Drive, the A→B import frees A at once;
    - held: an idle host with voices held on X, import Y → X stays (`held == X`); after 1 block and a sweep, retired = 0;
    - **D-C negative control:** with `disableHeldExclusion` in graveyard mode, `testDerefAfterReap ≥ 1`; with the amendment on, it is 0;
    - +2 rule via `midBlockCallback`;
    - the quiescent rule must not fire while a block is in flight;
    - **concurrent soak:** 16 held notes, 50 imports with random gaps from 3 files, a sweep thread. Finite output, deref = 0, held count ≤ 3 at every sample point. At the end, retired = 0 and `testLiveCount == 5 + 1`. Count banks, never addresses.
  - **FUNC-03 / COMPAT-03:**
    - frame counts: 441000 samples → 215; 600000 → 256; 2048 → 1; 2·2048+100 → 2; 2047 → `tooShort` (bank untouched); garbage `.wav` → `unreadable`; 97 MB memory import → `tooLarge`;
    - the same 16-bit content as WAV/AIFF/FLAC × 44.1/48/96 kHz → identical int16 PCM and bit-identical banks;
    - 24-bit WAV; float WAV with a NaN; stereo mean; 6-channel mean of 6.
  - **FUNC-04:**
    - import → save → fresh instance → restore: `memcmp` of all 11 levels, filename, `numFrames`, and P2's saved `data` == P1's;
    - negative control: one int16 changed must fail;
    - the pcm16gz path is bit-identical;
    - an absent child gives an empty Imported that renders exact silence.
  - **state-check additions:**
    - a **valid-blob two-reopen** probe: exactly 1 child per save, identical `data`, bit-identical bank;
    - the stale-child variant: reopen → import a different file → save → reopen restores the **second** file.
  - **No-locks-on-audio-thread structural check:** documented grep evidence for the render path (`juce::Synthesiser`'s own lock excepted). No wall-clock verdicts anywhere.
- **Depends on:** Task 18.

#### Task 20 — Part 2 static + offline gates  *(orchestrator)*
- Re-run the Task 9 static gates.
- Add a check that `OSIW_TEST_HOOKS` occurs only inside `#if` blocks and test CMake lines.
- Build and run `bank-check`, `dsp-check` (+ `--alloc-check`), `mod-check`, `import-check` and `state-check` in Debug, out of repo. Each needs exit 0 and 0 assertions.
- **Re-run G-Q2-SWEEP and G-Q2-C8** after the D-K hysteresis lands. They must still pass.
- On failure, route the fix back through dsp-agent dispatch B or C with the gate output. Thresholds are not loosened.
- **Depends on:** Task 19.

#### Task 21 — Contract doc corrections  *(orchestrator)*
- **Files:** `research/ARCHITECTURE.md`, `parameter-spec.md`, `REQUIREMENTS.md`, `STATUS.md` (checksums).
- **Do:** apply minimal, dated amendment notes. **Do not rewrite** anything else.
  - **ARCHITECTURE:**
    - the L formula floor (D-A);
    - §11 Mono = true legato and velocity squared;
    - the REG-01 `audioHeldBank` amendment + seq_cst (D-C);
    - `getIsPlaying()` is `bool`;
    - standard `juce::Base64`, not `toBase64Encoding`;
    - auto-select via AsyncUpdater (D-M);
    - smoothed depth/amount (D-I);
    - the worst-case blob size is 1.05 MB / 1.40 M characters.
  - **parameter-spec:** the `voice_mode` text (Mono = last-note priority, true legato) and velocity squared.
  - **REQUIREMENTS:** a QUAL-02 note naming the D-B narrow-pulse exception and its equal-RMS metric.
  - **STATUS:** recompute `contract_checksums` for the changed files.
- **Depends on:** Task 20.

#### Task 22 — Build, install, host-validate, pedalboard (Part 2)  *(orchestrator)*
- **Do:** as Task 10, steps 1–4. Pedalboard adds:
  - the LFO audibly moves the spectrum (spectral centroid varies over 2 s at depth 1);
  - a state round trip with the IMPORTED_BANK child, via pedalboard's raw state if available; otherwise rely on FUNC-04 from the harness and note it.
- pluginval strictness 10 must pass on VST3 and AU. Its state-restore tests now hit the real `setStateInformation`.
- **Depends on:** Task 21.

#### Task 23 — DAW smoke  *(human-verify, Taylor; non-blocking for Task 24, feeds the verify phase)*
- Import a WAV via the harness-equivalent path. There is no UI yet: the orchestrator can provide a one-line Standalone debug hook only if Taylor asks; otherwise this is covered offline.
- Play while the LFO is set to Square/S&H.
- Switch banks while notes are held: no clicks.
- Bend across an octave: no clicks.
- Save the project, reopen it, and confirm Imported is restored.

#### Task 24 — SUMMARY, STATUS, PLUGINS row, commit Part 2  *(orchestrator)*
- **SUMMARY.md** (`stages/2-dsp/SUMMARY.md`):
  - the files created and changed;
  - D-A through D-O as applied;
  - which smoother shipped (D-N);
  - the measured gate values (QUAL-02 table with margins, the D-B exception line, the reaper counters, the alloc counts, the FUNC-04 memcmp);
  - deviations and the Task 11 checkpoint notes.
- **STATUS.md:** `status: stage_2_execute_complete`, `current_phase: verify`, `next_action: plugin_verify_stage_2`; the phase table shows execute ✓.
- **PLUGINS.md:** the `O-simpleWavetable` row becomes `🚧 Stage 2`, still `0.1.0`, with today's date. **Only that row.** PLUGINS.md is foreign-staged (`MM`), so build the blob from `HEAD:PLUGINS.md` with just this row substituted.
- **Commit:**
  - Re-check the branch and staging immediately before. Temp-index CAS commit under `bash -c`, scope `plugins/O-simpleWavetable` + that one PLUGINS.md row.
  - Post-commit resync: reset only our own paths. If the other session's staged PLUGINS.md would revert this row, patch only this row in their staged blob.
  - **Never** `git add -A`, `commit -a`, or a bare `reset`.
  - Message: `feat(O-simpleWavetable): Stage 2 Part 2 - modulation, crossfader, import, persistence (2.3+2.4)`.
- **Hand off:** Step 1 `/clear`, Step 2 `/plugin-verify O-simpleWavetable 2-dsp`, then STOP.
- **Depends on:** Task 22 (Task 23 is not a precondition).

---

## Files to create / modify

| Path | Action | Task |
|------|--------|------|
| `Source/WavetableBank.h` | create | 2 |
| `Source/MipmapBuilder.{h,cpp}` | create | 2 |
| `Source/BankFactory.{h,cpp}` | create | 3 |
| `Source/BuiltInBanks.{h,cpp}` | create | 3 |
| `Source/WtRead.h`, `Source/BitQuantizer.h`, `Source/MonoStack.h` | create | 4 |
| `Source/WtVoice.h` | rewrite → extend | 5, 14, 16 |
| `Source/PluginProcessor.{h,cpp}` | extend | 6, 13, 17, 18 |
| `Source/PositionLfo.h`, `Source/PositionSmoother.h` | create | 13, 14 |
| `Source/WavetableImporter.h(/.cpp)` | create | 17, 18 |
| `CMakeLists.txt` | `target_sources` + 4 console targets | 7, 15, 19 |
| `tests/bank-check/main.cpp`, `tests/dsp-check/main.cpp` | create | 8 |
| `tests/mod-check/main.cpp`, `tests/import-check/main.cpp` | create | 15, 19 |
| `tests/state-check/main.cpp` | P6 flip; 2.4 probes | 8, 19 |
| `.planning/research/ARCHITECTURE.md`, `parameter-spec.md`, `REQUIREMENTS.md` | dated amendment notes | 21 |
| `.planning/stages/2-dsp/SUMMARY.md` | create | 24 |
| `.planning/STATUS.md` | update | 10, 12, 21, 24 |
| `PLUGINS.md` (this row only) | update | 24 |

All paths are relative to `plugins/O-simpleWavetable/` except `PLUGINS.md`.

**Must not touch:**
- `Source/PluginEditor.*`, `Source/ui/**`, `mockups/` (Stage 3);
- root `CMakeLists.txt`, `.gitignore`, `modules/registry.yaml`;
- `build/CMakeCache.txt` options;
- other plugins.

---

## Dependency graph / waves

```
PART 1
Wave 0 [orch]   T1 pre-flight + 1->2 gate
Wave 1 [dsp-agent A]  T2 bank data -> T3 factory/built-ins
                      T2 -> T4 read/quant/monostack -> T5 voice -> T6 processor -> T7 cmake -> T8 harnesses + P6'
Wave 2 [orch]   T9 static + offline gates (Debug, out-of-repo) -> T10 build/install/auval/pluginval/pedalboard -> commit P1
Wave 3 [HUMAN]  T11 LISTENING CHECKPOINT  ---- execute STOPS here ----

PART 2  (second /plugin-execute)
Wave 4 [orch]   T12 pre-flight
Wave 5 [dsp-agent B]  T13 LFO + global buffers -> T14 mod env/clamp/smoother -> T15 mod-check
Wave 6 [dsp-agent C]  T16 crossfader + hysteresis -> T17 reaper/publish/importer -> T18 persistence -> T19 import-check + state probes
Wave 7 [orch]   T20 gates (all 5 drivers; QUAL-02 re-run) -> T21 contract notes -> T22 build/host/pedalboard
Wave 8 [orch]   T24 SUMMARY/STATUS/PLUGINS + commit P2      [human] T23 DAW smoke (parallel; feeds verify)
```

Dispatch B can be started only after its gates (T15) are green under T20's recipe. The orchestrator may run a T20 subset after T15, before dispatching C, to localise failures. That is recommended.

---

## Success criteria (goal-backward; all must be TRUE for the verify phase)

| # | Observable truth | Evidence | Task |
|---|------------------|----------|------|
| S1 | The banks are correct | All bank-check gates pass, incl. G-NEG failing as designed. `buildMillis` is logged. | 8, 9 |
| S2 | Pitch is exact, and Position changes timbre, not pitch | G-PITCH ±1 cent A0–C8 at 44.1/48 kHz, with its negative control; G-BEND | 8, 9 |
| S3 | QUAL-02 holds with D-A | C8 ≤ −100 dB and sweep ≤ −70 dB at 44.1/48/96 kHz on Drive 32, Pulse 1 and Saw-1023, with margins printed. G-DSP02 liveness > −40 dB. Re-run after 2.4. | 9, 20 |
| S4 | The D-B exception is explicit | G-Q2-PULSE ≤ −60 dB equal-RMS. The exception line appears in the log, SUMMARY and the REQUIREMENTS note. | 9, 21, 24 |
| S5 | Stepping, interp and bit depth behave as taught | G-DSP01 (32 distinct cycles Off; ≥ 500 On), G-DSP03 (exactly 8 values), G-FULL (bit-identical bypass), G-READ (voice == `readSample`) | 9 |
| S6 | Voices and modes | G-POLY (16 + steal), G-MONO (true legato, no re-attack), G-RETRIG, G-NOTEOFF, G-VEL (−11.90 ± 0.05 dB) | 9 |
| S7 | Output stage and seeding | G-OUT (−60 dB = exact 0; no fade-in), G-IMPEMPTY, G-BLOCK, G-FINITE | 9 |
| S8 | Modulation | DSP-05 clamp + zipper; FUNC-05 shapes/tempo/global phase; FUNC-06; S&H determinism; Square/S&H click gate; block-size invariance | 20 |
| S9 | Clickless switching | QUAL-03 exactness tier on every trigger and the ratio tier on bank/import swaps, incl. the fold case; negative controls fire | 20 |
| S10 | RT safety | Alloc gate = 0 (with the liveness probe) across Part 1 stimuli, 200 bank switches and 50 concurrent imports (PERF-01, DSP-06) | 9, 20 |
| S11 | The reaper is sound | Quiescent, held and +2 rules; **D-C negative control ≥ 1 and the amended build = 0**; soak bounded (held ≤ 3, live = 6 at the end) | 20 |
| S12 | Import is correct | FUNC-03 counts (215/256/1/2/reject), the error vocabulary, COMPAT-03 9-way bit-identity, channel mean of all channels | 20 |
| S13 | Persistence is bit-identical | FUNC-04 memcmp over all levels + verbatim data string; negative control; pcm16gz; absent child = silence; two-reopen and stale-child probes | 20 |
| S14 | Hosts accept it | VST3 + AU + Standalone build clean (0 warnings from this plugin); targeted auval; pluginval VST3/AU strictness 10 (after Part 1 and after Part 2); pedalboard checks | 10, 22 |
| S15 | Taylor signed off on the sound | Task 11 sign-off recorded in STATUS | 11 |
| S16 | Contracts are consistent | ARCHITECTURE/parameter-spec/REQUIREMENTS amendment notes for D-A, D-B, D-C, legato, velocity, Base64, AsyncUpdater, D-I; checksums refreshed | 21 |
| S17 | Clean, scoped commits | Two commits; each touches only `plugins/O-simpleWavetable/**` (+ 1 PLUGINS.md row in Part 2); no foreign rows; no test hooks in the shipped build | 10, 24 |

### Coverage audit

| Source item | Covered by |
|-------------|-----------|
| ROADMAP 2.1 (DSP-04, FUNC-02, Square/Drive/levels, build time) | T2, T3, T8 → S1 |
| ROADMAP 2.2 (FUNC-01, QUAL-02, DSP-01/02/03, FUNC-07, PERF-01) | T4–T8 → S2–S7, S10 |
| ROADMAP 2.3 (DSP-05, FUNC-05, FUNC-06, Square/S&H clicks) | T13–T15 → S8 |
| ROADMAP 2.4 (QUAL-03, DSP-06, FUNC-03, COMPAT-03, FUNC-04, reaper, rapid imports) | T16–T19 → S9–S13 |
| CONTEXT cadence (checkpoint after 2.1+2.2) | T10/T11 stop rule → S15 |
| CONTEXT: squared velocity, true legato, Interp-Off step accepted | T5, T6, T14 → S6; T21 docs |
| CONTEXT: import triggerable from code | T17 public API (D-E) |
| CONTEXT: ASan unusable → counters + graveyard | T17 hooks, T19 → S11 |
| CONTEXT: reaper entry counter (host-idle freeze) | T17 `blockEntries` → S11 |
| CONTEXT: no Prism trilinear blend, Nyquist zeroed | T2, T4 must-nots; G-BL/G-NEG → S1, S3 |
| CONTEXT: seeding, dirty-checked ADSR | T5, T6, T13, T14; G-OUT, G-NOTEOFF, FUNC-06 |
| CONTEXT: determinism, no shared seeds | D-J, T15 |
| CONTEXT: pedalboard verification | T10, T22 |
| CONTEXT open Q: imported memory | Accepted ~23 MB steady state (RESEARCH §8); no aliasing |
| RESEARCH D-A…D-G | Decisions table; T4, T8, T17, T19 |
| RESEARCH top pitfalls 1–10 | T17 (1), T4 (2), T2 (3), T5 (4), T8 (5), T18 (6), D-N (7), D-J (8), T17 (9), T6/T13 (10) |
| Stage 3 seams (display atomics, thumbs, bankDisplayGen, import API, readSample) | T2, T4, T6, T13, T17 |

No unplanned items.

---

## Out of scope (Stage 2)

- **WebView UI:** relays, attachments, binary-data UI target, the `cycleUpdate`/`bankUpdate`/`importStatus` emitters, `uiReady`, drop streaming, `buildCycleView()` and `getBankThumbnails()` wiring (Stage 3). The data (`thumbs`, display atomics, `bankDisplayGen`, `readSample`) is provided here.
- **Polish:** factory presets, CPU optimisation beyond the gates, Windows build, CHANGELOG, version 1.0.0, the CODE_REVIEW pass (Stage 4).
- **Not built:** the Risk 1 fallback (preallocated 23 MB slot); a quick-fade voice steal (RESEARCH A2: hard-cut steal accepted, as in the siblings); an Interp-Off micro-fade (CONTEXT: accepted).

---

## Risks and gotchas

| # | Risk | Mitigation |
|---|------|-----------|
| R1 | Reaper UAF with the frozen-cycle capture. It reproduces only with the host idle. | D-C amendment; graveyard negative control; soak (T17, T19). |
| R2 | Out-of-bounds table read from a NaN position or a stale `latchedFrame` after a bank shrinks (256 → 32). | NaN-safe clamps in `WtRead.h`; re-latch on any bank or interp change; `i0 ≤ 2047`; pluginval fuzz (T4, T5, T10). |
| R3 | QUAL-02 margin is only about 5 dB on the sweep. The temptation is to loosen the measurer. | Thresholds frozen. The DSP-02 liveness control proves the analyzer can see aliasing. |
| R4 | Per-block `ADSR::setParameters` kills the release; the mod env doubles the exposure. | Dirty-checks on both envelopes; G-NOTEOFF; FUNC-06 re-push gate. |
| R5 | Mono retrigger from the release tail clicks (Subtractive's `reset()`). | `noteOn()` without `reset()`; G-RETRIG. |
| R6 | Unseeded smoothers fade in from 0; a NaN in a `SmoothedValue` is sticky. | Seed after `reset`; `finiteOr` on every param read; G-OUT. |
| R7 | A sub-range Synthesiser render fires later MIDI early. | Sliced `chunkMidi` at absolute positions; G-BLOCK. |
| R8 | The 2 ms one-pole has a velocity kink on Square/S&H edges. | Measure first; pre-authorized two-pole fallback (D-N). |
| R9 | A crossfade storm from bend jitter at an octave boundary. | D-K downward hysteresis; fold rule; QUAL-02 re-run (T20). |
| R10 | FLAC writer quirks: the writer owns the stream, STREAMINFO is rewritten on destruction, a failed writer leaks once. | Scoped writer; no retry loops; pcm16gz fallback (T18). |
| R11 | Non-standard JUCE base64 locked into sessions. | `juce::Base64` only; grep gate `toBase64Encoding` = 0 (T20). |
| R12 | Import and restore race; a late import overwrites a restore. | `importGen` bump in restore; re-check under `bankStateLock` before publish (T17, T18). |
| R13 | The console harness has no message loop (Timer and `callAsync` are dead). | Test accessors `sweepNow()` and `handleUpdateNowIfNeeded()`; status version polling. |
| R14 | Gates that measure state instead of the render, or a stimulus below threshold (vacuous). | Every gate renders audio and has a negative control or liveness check (memories `pattern_pointer_moved_is_not_audio_changed`, `pattern_gate_stimulus_below_threshold_is_vacuous`). |
| R15 | `operator new` counter blind to HeapBlock. | O-Bells `malloc_logger` gate with a liveness probe (T8, T19). |
| R16 | Stage 1 P6 fails as soon as sound exists. | P6′ flip in the same commit (T8). |
| R17 | Shared checkout: PLUGINS.md foreign-staged; other sessions' staging joins the commit. | Temp-index CAS commits under `bash -c`; re-check immediately before; reset own paths only (T10, T24). |
| R18 | Test hooks leak into the shipped binary. | Hooks only under `#if OSIW_TEST_HOOKS`, defined only on console targets; T20 grep. |
| R19 | Untrusted input (ASVS V5): state blobs, imported files and names, JS-supplied bytes. | `numFrames`/length validation, a 4 M char cap, header length never trusted, channel cap 64, name sanitising, the 96 MB memory cap, NaN scrub, gzip exact-length read (T17, T18). |
| R20 | Memory: about 75 MB transient per instance during import (live + held + building). | Accepted (RESEARCH §8). The steady state is about 23 MB. Recorded in SUMMARY. |
