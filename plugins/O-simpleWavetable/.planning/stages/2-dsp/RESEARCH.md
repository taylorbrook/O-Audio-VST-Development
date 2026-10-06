# Stage 2 — DSP: RESEARCH

**Plugin:** O-simpleWavetable
**Stage:** 2 of 4 — DSP (Phases 2.1 Bank Engine · 2.2 Voice + Oscillator · 2.3 Modulation → Position · 2.4 Bank Switch / Import / Persistence)
**Phase:** research ✓
**Date:** 2026-10-05
**Input:** `stages/2-dsp/CONTEXT.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` §Stage 2, `parameter-spec.md`, `stages/1-foundation/*`, `Source/` (Stage 1), `mockups/v1-integration-checklist.md`
**Method:** two parallel researchers. Part A covers 2.1 + 2.2, Part B covers 2.3 + 2.4. `[VERIFIED: path:line]` means the source was read this session (JUCE 8.0.15 local tree, sibling plugins, memory notes). `[ASSUMED]` means it was not checked. Both parts ran numerical probes:
- an alias / mip model in numpy and a vDSP FFT benchmark
- a FLAC round-trip probe built against the pinned JUCE

All probes are in `research-probes/`.
**Confidence:** HIGH for 2.1 / 2.3 / persistence. MEDIUM-HIGH for 2.2 alias gates, which depend on decisions D-A and D-B, and for the 2.4 reaper, which needs amendment D-C.

---

## 0. Consolidated summary

### Decisions that need the user before / during planning

| ID | Issue | Recommendation |
|----|-------|----------------|
| **D-A** | At 96 kHz, notes ≤ MIDI 30 select mip level 0 (1023 harmonics). Aliases reach **−68.2 dB**, against QUAL-02's −70 dB sweep gate. Stage 0 never modelled A0 at 96 kHz. | **Floor the level at 1 when Band-limit is On.** The worst case becomes −74.9 dB at every rate. No note changes at 44.1 or 48 kHz. This edits a locked formula, so it needs sign-off. (Part A §5, §6) |
| **D-B** | Narrow Pulse Width frames (about 24 to 32) measure −55 dB at low notes against the strongest harmonic, which misses the 60 dB requirement. Against an equal-RMS sine they measure −67 dB. Pulse 32 at C8 / 96 kHz is −97 dB, missing the −100 dB bar. A Hermite read gains only 3.4 dB. | Judge narrow pulses on the **equal-RMS metric** and document them as a named exception in the QUAL-02 gate. (Part A §6, §12) |
| **D-C** | O-Prism's REG-01 reaper (`0f7d65ce`, `PluginProcessor.cpp:778-782, 803-807, 1062, 1089-1160`), ported verbatim, causes a **use-after-free** with our frozen-cycle crossfader. If the host is idle during an import, the old bank is freed, and the next block captures its cycle from it. | **Amend the port.** The audio thread publishes `audioHeldBank`, and the sweep never frees that pointer. Use seq_cst on the reaper atomics. The proof works without ASan: test builds mark reaped banks instead of freeing them, and a read of a marked bank is counted. With the fix off the count must be ≥ 1; with it on, it must be 0. (Part B §1, §7.3) |
| D-D | Who owns the 20 ms position-knob smoother? | **2.2**, so the listening checkpoint has no zipper noise. |
| D-E | Import API names | Use the Stage 3 template names `importFromFile` / `importFromMemory` / `getImportStatus` / `getImportStatusVersion`, not CONTEXT's example `requestImport`. Stage 3 then needs no rename. |
| D-F | QUAL-02's "1023-harmonic frame" | Use **Pulse frame 1**. The processor can reach it, and it measures the same as an ideal saw. |
| D-G | QUAL-03 coverage for toggles and level changes | Accept the **crossfade-exactness tier** for these: output must match an ideal 5 ms crossfade of the old and new renders within 1e-4. Bank and import swaps also get the ratio-tier click detector. |

Already settled. Do not re-ask: mono **true legato** and **squared velocity** (CONTEXT wins). parameter-spec and ARCHITECTURE §11 still say Mono "retriggers" and velocity is "linear". Fix those docs during execute.

### Interface reconciliation (Part A §8 vs Part B §0)

The two parts propose the same seams with slightly different shapes. Planner resolution:
- **Per-block context:** use Part B's `BlockContext` struct. It holds the per-sample `knobPos` / `lfo` / `lfoDepth` / `envAmount` arrays, the resolved `bank`, `interp`, `bandlimit` and `bitDepthIndex`. Voices index the arrays at the **absolute in-buffer index** `startSample + i`. Part A's chunk loop must therefore pass the full buffer with absolute offsets, which removes the `chunkOrigin` subtraction. Size every buffer to `preparedBlock`.
- **Read-config hook:** Part A's `ReadConfig {bank, level, interp, bandlimit}` + `onReadConfigChange` is the same seam as Part B's `lastCfg`/`triggerCrossfade`. 2.2 hard-switches; 2.4 adds the capture. A voice keeps `lastCfg.bank` **only while active**, and `startNote` overwrites it without a capture.
- **MipmapBuilder must be reentrant**, with an FFT and scratch buffers per call. The import worker and `setStateInformation` can both build at once. Import and restore share **one builder path that starts from int16**, which keeps FUNC-04 bit-identical.
- **Never call `synth.*` from the message thread.** `juce::Synthesiser` takes a `CriticalSection` inside `processNextBlock` `[VERIFIED: juce_Synthesiser.cpp:193]`. Detect Poly/Mono switches in `processBlock`.
- If the §2.6 block-size-invariance gate is adopted, call `setMinimumRenderingSubdivisionSize (1, true)`.

### Top pitfalls across the stage (ranked)

1. **Reaper UAF with the crossfader** (D-C). It reproduces only when the host is idle, so normal renders hide it.
2. **Out-of-bounds table reads.** A NaN position, or a latched frame left over from a larger bank, indexes past the table, and `jlimit` passes NaN through. Use a NaN-safe clamp (Part A §7).
3. **Do not port O-Prism's mip builder or read path as they are.** It keeps the Nyquist bin at level 0, has only 10 levels, and its trilinear blend fails QUAL-02 at −15.6 dB. Port only its flat table layout.
4. **Mono retrigger from the release tail clicks** if it copies O-simpleSubtractive's reset-to-0. Call note-on without the reset; the JUCE ADSR attack ramps from the current level.
5. **Stage 1 `state-check` P6** asserts all-zero output and will fail once 2.2 makes sound. Flip it in the same commit. Phase 2.4 also needs a new P4 variant: a valid blob, reopened twice.
6. **Base64:** use `juce::Base64` both ways. `MemoryBlock::toBase64Encoding` is JUCE's non-standard format. The FLAC writer owns its stream, so destroy it before reading the bytes. Worst-case blob (white noise, 256 frames): 1.05 MB FLAC / 1.40 M base64 characters. ARCHITECTURE's 0.6–1.0 MB estimate is low.
7. **2 ms one-pole click risk.** It may fail the 100%-depth Square/S&H click gate (estimate, not measured). Measure first; the fallback is two 1 ms poles in series.
8. **Determinism.** Draw S&H from a hash of the cycle index, so it repeats on a transport loop. In tempo mode, recompute LFO phase from PPQ every sample. `getIsPlaying()` returns a plain bool, not an Optional. Use exact triplet fractions, not Prism's `2.6667f`.
9. **Importer.** Average **all** channels; Prism averages two. O-simpleGrain has no worker thread, so reuse only its decode/publish shape. O-TextureForge's single-slot retirement is unsafe under rapid imports, so do not port it.
10. **Seeding.** Seed every per-voice smoother at note-on, and seed the output gain so `prepareToPlay` doesn't fade in from silence. Dirty-check both ADSRs.

### Numbers established

- Full built-in bank build: **~8 ms** (vDSP benchmark).
- All five banks pass the 2.1 gates in the model: peaks at 0 dB, DC and above-Kmax content far below threshold, Drive frame 1 h3 = −45.8 dB.
- At C8, on all three rates: Saw-1023, Pulse frame 1 and Drive frame 32 reach **−114 to −123 dB**.
- FLAC int16 round trip: 0 mismatches for 256 noise frames, 1 frame and the ±32767/−32768 edge values. The `pcm16gz` fallback is also lossless.

### Execution cadence (from CONTEXT)

2.1 + 2.2 → offline gates (`bank-check`, `dsp-check`, flipped `state-check`) → build/install → pedalboard check → **user listening checkpoint** (script in Part A §10) → 2.3 + 2.4.

---

# Part A — Phases 2.1 (Bank Engine) + 2.2 (Voice + Oscillator)

**Researched:** 2026-10-05 · **Scope:** 2.1 and 2.2 only. Seams that 2.3 and 2.4 need are listed in §8. **Confidence:** HIGH for the code and JUCE facts (every source was read this session). HIGH for the numbers (numpy model plus a vDSP benchmark, both run this session). MEDIUM for two items that need a user decision (D-A and D-B below).

Provenance tags: `[VERIFIED: path:lines]` means the file was opened this session and the quoted text is verbatim. `[MEASURED: script]` means a scratch script was run this session (scripts are in `research-probes/`). `[ASSUMED]` means training knowledge only.

---

## 0. Summary + top pitfalls

**Bottom line.** ARCHITECTURE.md's algorithms hold up. The bank formulas pass every 2.1 gate in a numpy model of the exact pipeline. QUAL-02 at C8 passes at all three sample rates with ≥ 14 dB margin. **But two measured findings contradict the Stage 0 claim "aliases verified 74 dB or more down at 44.1/48/96 kHz"**, and the planner has to resolve them before the QUAL-02 gate is written:

1. **D-A (96 kHz low notes fail the −70 dB sweep).** At 96 kHz, notes below about 47 Hz (MIDI ≤ 30) select level L=0, which keeps Kmax = 1023. The table is then only 2× oversampled, and the linear read's images fold back at **−68.2 dB** on a 1023-harmonic saw or square `[MEASURED: wt_model2.py]`. Stage 0 never modelled A0 at 96 kHz (§A2 has only a 96 kHz C8 row) `[VERIFIED: ARCHITECTURE.md:301-309]`. **Fix:** floor the level at 1 when band-limit is On: `L = clamp(ceil(log2(f·2048/fs)), 1, 10)`. The worst case then becomes −74.9 dB at every rate. At 44.1/48 kHz the change affects no MIDI note, and at 96 kHz it drops only harmonics above 24 kHz. Band-limit Off keeps L=0, so the aliasing lesson is unchanged. This edits a locked formula, so it needs a planner or user sign-off.
2. **D-B (narrow pulses fail the REQ's 60 dB).** Pulse Width frames from about 24 to 32, at low notes, measure **−55 dB** (frame 32) against the strongest harmonic at every rate. Measured against an equal-RMS sine they reach −67 dB `[MEASURED: wt_model2.py]`. Hermite gains only 3.4 dB, so the interpolator cannot fix it. The ROADMAP gate names only "Drive frame 32 and a 1023-harmonic frame", and both pass. But REQUIREMENTS QUAL-02 says "on the brightest frame". **The user decides:** (a) gate the named frames and document narrow pulses as a known exception; or (b) gate Pulse on the equal-RMS metric at 60 dB; or (c) both. Pulse frame 32 at **C8 / 96 kHz is −97.1 dB**, which also misses the −100 dB C8 bar if Pulse is ever added to that gate.

**Primary recommendation.** Build 2.1 as an offline-verified data layer: `WavetableBank`, `MipmapBuilder`, `BankFactory`, and `BuiltInBanks` behind `SharedResourcePointer`. Verify it with a processor-console `bank-check` driver. Then build 2.2's `WtVoice` around one shared read header (`WtRead.h`) and one quantizer header (`BitQuantizer.h`). Both are reused unchanged by the Stage 3 visualizer and the 2.4 frozen-cycle capture. Gate everything in a processor-console `dsp-check` driver, using a Kaiser-38 FFT analyzer and the O-Bells `malloc_logger` alloc gate.

**Top pitfalls, ranked** (detail in §7):
1. **A NaN or out-of-range position causes an out-of-bounds table read.** `(int)NaN` is undefined behaviour, and `jlimit` passes NaN through. A `latchedFrame` left over from a 256-frame bank overruns a 32-frame bank. Clamp the frame indices; never trust `jlimit` with a NaN.
2. **QUAL-02 drift**: D-A, D-B, a Nyquist bin left in, or any Prism-style level blend (Prism's blend reaches −15.6 dB at C8).
3. **Per-block `ADSR::setParameters` kills the release.** Dirty-check it, skip it while releasing, and apply deferred params at note-on (Additive pattern).
4. **The mono retrigger from a release tail clicks if you copy Subtractive verbatim.** It calls `ampEnv.reset()` before `noteOn()`, and resets the phase. Use `noteOn()` from the current level, and keep the phase while the voice is sounding.
5. **The `state-check` P6 "silent-render" probe will FAIL once 2.2 makes sound.** It asserts every sample is exactly 0.0f. 2.2 must flip it.
6. **Output gain and knob smoothers must be seeded in `prepareToPlay`**, or the output fades in from silence.
7. **Oversized host blocks.** Once a per-sample `posBuf` exists, processBlock needs a chunk loop with sliced MIDI.
8. **Alloc-gate traps.** An `operator new` counter is blind to HeapBlock. Use `malloc_logger` with volatile flags, scope it to the audio thread, and run a liveness probe first.
9. **Gates must measure the render, not voice state.** Each gate needs a negative control. DSP-02 (band-limit Off) is the liveness control for the QUAL-02 measurer.
10. **Spec drift on Mono.** The latest decision is CONTEXT "true legato", but parameter-spec and ARCHITECTURE §11 still say "retrigger". Implement CONTEXT.

---

<user_constraints>
## 1. User constraints (from 2-dsp/CONTEXT.md, verbatim where locked)

**Locked decisions** `[VERIFIED: .planning/stages/2-dsp/CONTEXT.md:25-35]`:
- Execution cadence: "**2.1 + 2.2 → build/install → user listening checkpoint → 2.3 + 2.4**". The generic editor is the test surface.
- Velocity → amplitude: "**Squared:** `gain = (v/127)²`".
- Mono legato: "**True legato:** an overlapping note changes pitch only; amp and mod env continue". Also "Last-note priority, and the release returns to the previous held note."
- Interp-Off step on Pulse/Imported: "**Accept as-is** (no micro-fade)".
- Algorithms: "**As ARCHITECTURE.md** (linear read, strict 11-level mips, latched stepping, frozen-cycle crossfade, mid-rise, …)". "Not re-litigated." D-A is a measured defect in a case the contract never modelled, not a re-litigation. It still needs sign-off.
- Prior sign-offs: ±2 st bend; mid-rise; per-frame normalize capped at +24 dB (2.4); Empty Imported = silence.

**Constraints** `[VERIFIED: CONTEXT.md:16-23]`:
- "Do **not** port O-Prism's trilinear mip blend: it fails QUAL-02 at −15.6 dB. Zero the Nyquist bin."
- "Every per-voice `SmoothedValue` / one-pole must still be seeded at note-on, and `prepareToPlay` must not fade in from silence."
- "Amp ADSR `setParameters` is dirty-checked, never called per block unconditionally."
- "phases reset at note-on, so offline renders are byte-stable for golden gates. Render jobs must not share a seed."
- "The installed binary is verified with pedalboard via `uv run --python 3.12 --with pedalboard`, with params set by raw_value."
- "Build/install via `./scripts/build-and-install.sh O-simpleWavetable`. Commits are path-scoped …"

**Deferred / out of scope here:** 2.3 (LFO, mod env, position smoothing beyond the seam), 2.4 (crossfader, import, reaper, state).
</user_constraints>

### 1.1 Spec inconsistencies the planner must resolve (all read this session)

| # | Conflict | Sources | Resolution |
|---|---|---|---|
| S1 | Mono = "retrigger on every new note" vs "true legato" | ARCHITECTURE.md:152 ("retrigger on every new note"); parameter-spec.md:196 ("Mono = last-note priority, retrigger, no glide"); CONTEXT.md:29 ("True legato") | CONTEXT is newest and is the user decision. Implement Subtractive **mode 2 (Legato)** semantics as this plugin's only "Mono". |
| S2 | Velocity linear vs squared | ARCHITECTURE.md:130 ("velocity → linear gain"); CONTEXT.md:28 ("Squared") | Squared: `velGain = v*v` with `v = velocity` in 0..1 (JUCE passes v/127). |
| S3 | 22 vs 21 params | ROADMAP.md:43,51; parameter-spec.md:13 | 21. Already fixed in Stage 1 (`jassert (params.size() == 21)`) `[VERIFIED: Source/PluginProcessor.cpp:141]`. |
| S4 | L formula floor 0 vs D-A | ARCHITECTURE.md:48,110 | Needs sign-off (§0). |

---

## 2. Current code baseline (Stage 1), read this session

- **Param IDs** `[VERIFIED: Source/PluginProcessor.h:48-68]`: `bank = "bank"`, `position = "position"`, `interp = "interp"`, `bandlimit = "bandlimit"`, `bitDepth = "bit_depth"`, `ampAttack = "amp_attack"`, `ampDecay = "amp_decay"`, `ampSustain = "amp_sustain"`, `ampRelease = "amp_release"`, `voiceMode = "voice_mode"`, `outputLevel = "output_level"`. Also present are the lfo_*/menv_*/env_amount IDs for 2.3.
- **Choices** `[VERIFIED: Source/PluginProcessor.cpp:107-113,120-121,138]`:
  - bank: `"Sine \xE2\x86\x92 Saw"`, `"Sine \xE2\x86\x92 Square"`, `"Pulse Width"`, `"Formant"`, `"Drive"`, `"Imported"` (index 5).
  - bit_depth: `{ "Full", "16", "15", "14", "13", "12", "11", "10", "9", "8", "7", "6", "5", "4", "3" }`.
  - voice_mode: `{ "Poly", "Mono" }, 0`.
- **Ranges** `[VERIFIED: PluginProcessor.cpp:42,139]`: amp times `{ 0.001f, 5.0f, 0.0f, 0.35f }`; output_level `{ -60.0f, 6.0f, 0.1f, 1.0f }, -6.0f`.
- `static constexpr int kNumVoices = 16;` `[VERIFIED: PluginProcessor.h:135]`. The ctor adds 16 `WtVoice` and `setNoteStealingEnabled (true)` `[VERIFIED: PluginProcessor.cpp:151-154]`.
- `processBlock` today `[VERIFIED: PluginProcessor.cpp:184-193]`: `ScopedNoDenormals`, `buffer.clear()`, `if (numSamples <= 0) return;` with the comment "(Stage 2: bump the reaper counters here too)", then collector drain, then `synth.renderNextBlock (buffer, midi, 0, numSamples)`.
- `WtVoice` `[VERIFIED: Source/WtVoice.h:48-64]`:
  - non-virtual `prepareToPlay (double sampleRate, int)`
  - `startNote` seeds `pitchWheelPos = currentPitchWheelPosition;`
  - `stopNote` → `clearCurrentNote()` at once
  - silent `renderNextBlock`
- Test block `[VERIFIED: CMakeLists.txt:73-80]`: `option(OUARICON_BUILD_TESTS … OFF)`, `if(OUARICON_BUILD_TESTS AND APPLE)` → `ouaricon_add_param_dump` and `ouaricon_add_processor_console(... tests/state-check/main.cpp state-check)`.
- **P6 will break in 2.2** `[VERIFIED: tests/state-check/main.cpp:478-526]`: the probe asserts `allExactZero (buf)` after note-on, after 20 note-ons, and after UI MIDI ("note-on/pitch-wheel block not silent", …).

---

## 3. Sibling code to reuse (exact files and lines)

### 3.1 O-Prism: layout and mip builder (port with fixes). Do NOT port its read path.

- **Layout** `[VERIFIED: plugins/O-Prism/Source/dsp/WavetableData.h:36-40,53]`: `kTableSize = 2048;`, `kGuardSamples = 1;`, `kFrameSize = kTableSize + kGuardSamples; // 2049`, `kMaxFrames = 256;`, `kNumMipmapLevels = 10;`. Index `((level * numFrames + frame) * kFrameSize + sampleIndex)`. Keep the flat layout. **Change** levels to 11, and make the struct immutable after build (ARCHITECTURE §1).
- **Mip builder** `[VERIFIED: plugins/O-Prism/Source/dsp/WavetableGenerator.cpp:144-201]`. The core is `int maxHarmonic = (fftSize / 2) >> level;` (line 163), the DC zero `workBuffer[0] = 0.0f; workBuffer[1] = 0.0f;` (169-170), and the zero loop `for (int bin = maxHarmonic + 1; bin <= fftSize / 2; ++bin)` (173). **Defect 1:** at level 0, maxHarmonic = 1024, so the loop starts at 1025 and the **Nyquist bin (1024) survives**. **Defect 2:** there are only 10 levels. **Redundancy:** the negative-bin zeroing loop (180-192) does nothing, because JUCE's real inverse reads only bins 0..N/2 (§4.1). It is harmless but should not be ported. The guard-sample discipline (the comment at 138-142, `setGuardSamples` at WavetableData.h:71-81) is worth keeping.
- **Why the read path must not be ported** `[VERIFIED: plugins/O-Prism/Source/dsp/WavetableOscillator.cpp:241-266]`. It computes `levelFloat = std::log2 (std::max (levelFreq, baseFreq) / baseFreq)` and then `level0 = (int) levelFloat`. That is a **floor**, which picks the richer level. It then lerps `level0` → `level1` by `levelFrac`: "Trilinear interpolation (8 lookups)". For any fractional `levelFrac`, the richer level contributes harmonics above Nyquist, measured at −15.6 dB alias at C8 `[VERIFIED: ARCHITECTURE.md:308]`. Strict selection is a `ceil` with no blend. Worth reusing from it: the IN-08 clamp `idx0 = std::min (idx0, WavetableData::kTableSize - 1);` (line 228) and its rationale (lines 214-227).

### 3.2 O-simpleSubtractive: mono path, bend, MonoStack

- **`MonoStack`** `[VERIFIED: plugins/O-simpleSubtractive/Source/PluginProcessor.h:85-131]`: fixed `kCap = 32`, no allocation, `push` moves a repeated note to the top, `topNote()`/`topVel()`. Copy it verbatim into `Source/MonoStack.h`.
- **`renderMonoLegato`** `[VERIFIED: plugins/O-simpleSubtractive/Source/PluginProcessor.cpp:363-409]`. Key lines:
  - `const bool retrigger = (mode == 1) || ! wasHeld; // Legato slurs while held` (385)
  - fallback `v0->setPitchTargetNote (monoStack.topNote()); // fall back (glide, no retrigger)` (394)
  - all-notes-off clears (396-400)
  - pitch wheel straight to v0 (401-404)

  **For this plugin:** there is one mono mode, which is Subtractive mode 2. So `retrigger = ! wasHeld`. Adapt the sub-range rendering to absolute chunk positions (§6.3).
- **Mode switch hard reset** `[VERIFIED: PluginProcessor.cpp:457-463]`: `synth.allNotesOff (0, false); monoStack.clear();`. Poly dispatch is at 465-474.
- **`noteOnDirect`** `[VERIFIED: plugins/O-simpleSubtractive/Source/SubVoice.h:200-236]`. Velocity is updated only on retrigger (206-207); phase and filter state reset only when `! wasActive` (210-215). **Trap:** on retrigger it calls `filterEnv.reset(); ampEnv.reset();` before `noteOn()` (230-233). For a voice still in its release tail, that resets the envelope from its tail level to 0 in one sample, which clicks. That was intended for Subtractive's Mono "re-pluck in sustain". Under true legato a retrigger only happens when the stack is empty, which means the voice is idle or releasing. Call `ampEnv.noteOn()` **without** `reset()`: JUCE's attack ramps from the current `envelopeVal` (§4.3).
- **Bend** `[VERIFIED: SubVoice.h:360-361]`: `bendSemis = ((double) pitchWheelPos - 8192.0) / 8192.0 * 2.0; // ±2 st` and `bendFactor = std::pow (2.0, bendSemis / 12.0)`.
- **Voice lifetime on the amp env only** `[VERIFIED: SubVoice.h:320-327]`.
- **Harness probes to copy** `[VERIFIED: plugins/O-simpleSubtractive/tests/render-harness/main.cpp:491-545,571-589]`: test 12 "mono-last-note", test 13 "legato-vs-mono", test 15 "noteoff-click" (sustain 0, decay 3 s, note-off at 0.85 s, `tailRms > 0.2 * preRms`).

### 3.3 O-simpleAdditive: voice hygiene, lead voice, output stage

- **Dirty-checked ADSR plus deferred apply** `[VERIFIED: plugins/O-simpleAdditive/Source/AdditiveVoice.h:225-238]` (setParams: `if (! releasing) { if (! sameAdsr (ap, appliedAmpParams)) { ampEnv.setParameters (ap); … } }`). startNote applies the deferred params before `noteOn` (269-276). prepare runs `setSampleRate` **before** `setParameters` (158-164). Note that `sameAdsr` uses raw `==` (426-430). In this plugin `-Wfloat-equal` is on `[VERIFIED: stages/1-foundation/SUMMARY.md:44]`, so use `juce::exactlyEqual` (Additive uses it for spectra, 200-208).
- **noteAge** `[VERIFIED: AdditiveVoice.h:257,627]`: `noteAge = ++sNoteCounter;` and `static inline std::atomic<std::uint64_t> sNoteCounter { 0 };`. Do not name a voice method `isVoiceActive`; the comment at 309-312 explains it would hijack JUCE allocation.
- **Lead-voice snapshot** `[VERIFIED: plugins/O-simpleAdditive/Source/PluginProcessor.cpp:298-319]`: the newest `noteAge` among `isAmpActive()` voices, with relaxed atomics.
- **Output stage** `[VERIFIED: PluginProcessor.cpp:184-186,321-334]`:
  - prepare seeds `outputGain.reset (sampleRate, 0.02); … outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));`
  - process runs `setTargetValue` → `g0 = getCurrentValue()`, `g1 = skip (numSamples)`, `applyGainRamp`, then the `isfinite` scrub.
- **Quantizer: do NOT port.** It is mid-tread `round(s·L)/L` (AdditiveVoice.h:391-395, "mid-tread"). This plugin needs mid-rise (ARCHITECTURE §6).
- **Choice index resolve** `[VERIFIED: PluginProcessor.cpp:257-262]`: `juce::jlimit (0, N-1, (int) std::round (get (bitDepth)))`. Never use a truncating cast.

### 3.4 Test infrastructure

- **Processor-console helper** `[VERIFIED: scripts/param-dump/ParamDump.cmake:67-271]`. It compiles **every `.cpp` in the plugin target's SOURCES** except `PluginEditor` (156-159), sets `JUCE_WEB_BROWSER=0` (200), and derives the identity macros. So **new DSP `.cpp` files must be added to the plugin's `target_sources`**, and every driver gets them automatically. Add drivers with `ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/tests/<dir>/main.cpp <suffix>)`.
- **Harness skeleton** `[VERIFIED: tests/state-check/main.cpp:81-98,644-646]`: `makePrepared()` → `setPlayConfigDetails (0, 2, kFs, kBlock); prepareToPlay (kFs, kBlock);`; `setParam` via `rp->setValueNotifyingHost (rp->convertTo0to1 (realValue))`; `juce::ScopedJuceInitialiser_GUI juceInit;` in `main`.
- **Out-of-repo Debug build recipe** `[VERIFIED: stages/1-foundation/PLAN.md:322-337]`. Run in bash: build a `SKIP` list of all other plugins, then `cmake -S . -B "$SCRATCH/build-oswt" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOUARICON_BUILD_TESTS=ON "-DSKIP_PLUGINS=$SKIP"`, then build the targets. Never set `OUARICON_BUILD_TESTS`/`SKIP_PLUGINS` in the shared `build/`.
- **Alloc gate** `[VERIFIED: plugins/O-Bells/tests/render-harness/main.cpp:89-124,285-295,353-366]`: the `extern malloc_logger_t* malloc_logger;` hook, `volatile bool allocArmed`, `volatile int allocCount`, `pthread_equal (pthread_self(), audioThread)`, `(type & 2u)` = allocate. A liveness probe (`std::malloc (64)` must count 1) runs first, and `backtrace_symbols_fd` names the culprit. Memory notes `pattern_malloc_logger_alloc_gate_two_traps` and `pattern_operator_new_counter_blind_to_heapblock…` explain why not to use an `operator new` counter.
- **Pitch and partial probes** `[VERIFIED: plugins/O-simpleFM/tests/render-harness/main.cpp:52-66]`: a single-bin DFT `binAmplitude`. Reusable for FUNC-07 (16 fundamentals present).

---

## 4. JUCE 8.0.15 APIs (local source, `project(JUCE VERSION 8.0.15 …)`)

### 4.1 `juce::dsp::FFT`: real-only layout and scaling

- `performRealOnlyForwardTransform (float*, bool onlyNonNeg = false)` and `performRealOnlyInverseTransform (float*)`. The array must be `2 * getSize()` floats. The output is interleaved complex `{re, im}` per bin `[VERIFIED: juce_dsp/frequency/juce_FFT.h:86-104]`.
- **Scaling (Apple vDSP engine, the one used on macOS):** `forwardNormalisation (0.5f)` cancels vDSP's 2× convention, so the forward transform is the standard unnormalised DFT `X[k] = Σ x[n] e^{-j2πkn/N}`. `inverseNormalisation (1.0f / (1 << order))` makes the inverse `(1/N) Σ X[k] e^{+j…}` `[VERIFIED: juce_FFT.cpp:456-457,483-511]`. This matches numpy `rfft`/`irfft`, so the numpy model in §5 is convention-exact.
- **The inverse reads only bins 0..N/2.** Apple packs the Nyquist real part into DC-imag: `inout[0] = Complex<float> (inout[0].real(), inout[size >> 1].real());` (line 506). The fallback engine overwrites bins N/2..N−1 with conjugates (lines 184-189). So **zeroing the mirrored negative bins is unnecessary**. **Zeroing Nyquist means `d[2*1024] = 0` (re); `d[2*1024+1]` is ignored.**
- **Sine-phase synthesis coefficient:** amplitude `b_n` of `sin(2πni/N)` → `d[2n] = 0`, `d[2n+1] = -0.5f * N * b_n`. Cosine amplitude `a_n` → `d[2n] = 0.5f * N * a_n`. Checked in the model: Sine→Saw frame 1 gives `s[512] = +1.0000` `[MEASURED: wt_banks.py]`. Gate this sign (§5.1 G-POL) so all banks share polarity. A polarity flip between banks would make 2.4's bank-switch crossfade pass through cancellation.
- The engine scratch for N=2048 is `16 + 2048*8` = 16 400 B. That is under `maxFFTScratchSpaceToAlloca = 256 * 1024` (line 134), so the fallback allocas rather than heap-allocating. Irrelevant anyway, because the FFT never runs on the audio thread. **Constructing** `juce::dsp::FFT (11)` allocates (engine instance), so construct it once per builder, off the audio thread.
- **Cost** `[MEASURED: fftbench.cpp, vDSP zrip -O2]`: 5 banks × 32 frames × 11 levels = 1760 inverse FFTs plus copies into a 14.4 MB flat array takes **7.3–7.8 ms**. Expect 10–20 ms for the full built-in build in Release, and more in Debug.

### 4.2 `juce::SharedResourcePointer<T>` (8.0.15 is a `shared_ptr`/`weak_ptr` implementation)

`[VERIFIED: juce_core/memory/juce_SharedResourcePointer.h:155-184]`:
- `std::shared_ptr<SharedObjectType> sharedObject = weak().lockOrCreate();`
- `lockOrCreate()` takes a `SpinLock` and runs `new SharedObjectType()` **inside the lock** (163-168).
- The object is destroyed when the last pointer dies. Doc: "When the last one is deleted, the underlying object is also immediately destroyed".

Consequences:
1. The ~15 ms build runs inside a spinlock. A concurrent second instance spins; that is acceptable.
2. **`BuiltInBanks()` must never construct a `SharedResourcePointer<BuiltInBanks>`**, or it deadlocks (doc warning).
3. pluginval's create/destroy cycles rebuild the banks every time the instance count drops to 0. That is fine.
4. Construct it as a processor member, on the message thread, never in `processBlock`.
5. "Never freed while any instance lives" holds. The audio thread holds a raw `const WavetableBank*` into it, which is valid for the processor's lifetime because the member keeps the ref.

### 4.3 `juce::ADSR`

`[VERIFIED: juce_audio_basics/utilities/juce_ADSR.h:92-99,130-160,170-222,262-279]`:
- `setParameters` → `recalculateRates()`, which computes `releaseRate = getRate (parameters.sustain, parameters.release, sampleRate)`. In release state with rate ≤ 0 it calls `goToNextState()` → `reset()`. That is the "per-block setParameters kills release" mechanism.
- `noteOff` sets `releaseRate = envelopeVal / (release * sampleRate)`.
- `noteOn` **does not zero `envelopeVal`**. The attack branch does `envelopeVal += attackRate` up to 1.0. So a retrigger from a release tail ramps up from the tail level with no discontinuity.
- `setSampleRate` must precede `setParameters` (jassert at line 95).
- With sustain = 1.0: `decayRate = 0` → attack goes straight to sustain, and the output is exactly `parameters.sustain` (1.0f). The DSP-03 "exactly 8 values" gate relies on this.

### 4.4 `juce::Synthesiser` (poly path)

`[VERIFIED: juce_audio_basics/synthesisers/juce_Synthesiser.cpp:180-237,307-351,509-611; .h:638-639]`:
- `minimumSubBlockSize = 32; subBlockSubdivisionIsStrict = false;`. A voice's `renderNextBlock (buf, start, num)` is called **several times per block**, split at MIDI events. Events closer than 32 samples are handled early (except the first).
- Note-on for a ringing note: `stopVoice (voice, 1.0f, true)` on the old voice, then `startVoice (findFreeVoice (…))`.
- `startVoice` calls `voice->stopNote (0.0f, false)` if the voice was busy (a hard cut on steal). It sets `currentlyPlayingNote` **before** `startNote(…, lastPitchWheelValues[ch-1])`. That means `getCurrentlyPlayingNote() >= 0` inside `startNote` is always true (memory `critical_synthesiser_startvoice_sets_note_before_startnote`). Use `! ampEnv.isActive()` read before `noteOn()` as the "was idle" signal.
- Steal order: same pitch first, then oldest released, then oldest without a finger down, then oldest unprotected. The lowest and highest held notes are protected. `usableVoicesToStealArray.ensureStorageAllocated (voices.size() + 1)` in `addVoice` (line 123) means a steal does not allocate.
- `handlePitchWheel` → `voice->pitchWheelMoved (wheelValue)` for every voice on the channel, between sub-blocks.
- `isVoiceActive()` is `getCurrentlyPlayingNote() >= 0` (cpp:55-58). The mono path never sets `currentlyPlayingNote` (it is private, and only `startVoice` sets it). So the lead-voice display must use the voice's own `isSounding()` (amp env active), not `isVoiceActive()`.
- `handleMidiEvent` is **protected** (h:572,632). In Mono mode the Synthesiser never sees pitch-wheel events, so `lastPitchWheelValues` goes stale. After a Mono→Poly switch, the first poly note can start with a stale bend. This is low-impact, and §6.3 has a mitigation.
- `setCurrentPlaybackSampleRate` calls `allNotesOff (0, false)` on a real rate change (cpp:166-176).
- A sub-range call `synth.renderNextBlock (buf, midi, start, n)` **handles every MIDI event at or after the end of the range immediately** (the `samplesToNextMidiMessage >= numSamples` branch, then the trailing `std::for_each`). So any chunk loop must pass a **sliced** MidiBuffer (§6.3).

### 4.5 `SmoothedValue`, `Decibels`, `MidiMessage`, `MidiBuffer`

- `SmoothedValue::reset (sr, secs)` → `reset ((int) floor(secs*sr))` → `setCurrentAndTargetValue (this->target)`. It preserves the target and does not clear a NaN. `skip(n)` snaps to the target when `n >= countdown` `[VERIFIED: juce_SmoothedValue.h:265-278,334-345]`. Seed it in `prepareToPlay` with `setCurrentAndTargetValue (paramValue)`.
- `decibelsToGain (db, minusInf)` returns `decibels > minusInfinityDb ? pow(10, db*0.05) : Type()`, so exactly 0 at −60 with floor −60 `[VERIFIED: juce_Decibels.h:54-59]`. `decibelsToGain (0.0f) == 1.0f` exactly, because `pow(10,0)` is 1.
- `getMidiNoteInHertz (n, a=440)` = `frequencyOfA * std::pow (2.0, (noteNumber - 69) / 12.0)` in double `[VERIFIED: juce_MidiMessage.cpp:1088-1091]`. A notes are exact.
- `MidiBuffer::clear()` is `data.clearQuick();`, which keeps capacity. `ensureSize (size_t)` exists. `addEvent (const void* raw, int bytes, int pos)` exists `[VERIFIED: juce_MidiBuffer.cpp:122; .h:210,230,275]`.

---

## 5. Phase 2.1: Bank Engine

### 5.1 Algorithms (formulas per ARCHITECTURE §A3–A7; values verified by model)

**Constants:** `kTableSize = 2048`, `kStride = 2049` (guard = sample 0), `kLevels = 11`, `Kmax_L = min(1023, 1024 >> L)` = 1023, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1 `[VERIFIED: ARCHITECTURE.md:93]`.

**Per-frame pipeline for built-ins** (message thread, at construction):
1. Build the packed spectrum `d[0..4095]` (bins 0..1024 used).
2. Force `d[0]=d[1]=0` (DC) and `d[2048]=d[2049]=0` (Nyquist).
3. **Normalise.** IFFT level 0 (bins 1..1023), take the sample peak `p` over 2048 samples, and scale the **spectrum** by `1/p`. Levels are never re-normalised (ARCHITECTURE §3).
4. For L = 0..10: copy the spectrum, zero bins `Kmax_L+1 .. 1024`, IFFT, write 2048 samples, then set `guard = s[0]`.

**Generators** (b_n = sine-phase amplitude unless noted):

| Bank | Formula | Notes / measured |
|---|---|---|
| Sine→Saw | `b_n(k) = 1/n` for n ≤ k | Frame k is exactly h1..k; above k ≤ −120 dB ✓ |
| Sine→Square | `c(k) = 1 + 15(k−1)/31`; odd `n = 2j−1`, `b_n = (1/n)·clamp(c−(j−1), 0, 1)`, j = 1..16 | Evens are exactly 0; frame 32 top = **h31** ✓; 32/32 distinct ✓ |
| Pulse Width | **Cosine** phase: `a_n = (2/(nπ))·sin(nπd_k)`, n = 1..1023, `x = Σ a_n (−1)^n cos(2πnφ)` → `d[2n] = 0.5·N·a_n·(−1)^n`; `d_k = 0.5·(1/16)^((k−1)/31)` | **Not zero at φ=0**: frame 1 `s[1] = −0.85`, frame 32 `s[0] = −0.03` `[MEASURED]`. Note-on onset and Interp-Off steps are attack-limited and accepted (CONTEXT). First null h2 → h32 ✓ |
| Formant | `f_ref = 110`, n = 1..48, `b_n = (1/n)·Π_i R_i(n·f_ref)`, `R(f) = (F²+(B/2)²)/sqrt(((f−F)²+(B/2)²)((f+F)²+(B/2)²))`, B = 90/110/170. Log-frequency interpolation of F1..F3 between anchors at frames 1, 8.75, 16.5, 24.25, 32 for vowels A, E, I, O, U | Anchor table `[VERIFIED: ARCHITECTURE.md:329-335]`. Interpolation reproduces the anchors exactly at the anchor frames ✓ |
| Drive | Time domain `x_k[i] = tanh(g_k·sin(2πi/2048))/tanh(g_k)`, `g_k = 0.25·100^((k−1)/31)` → forward FFT → same pipeline | Frame 1 h3 **−45.80 dB**, frame 32 h3 −9.59 dB; h3/h5/h7 strictly increasing ✓. **No oversampling needed**: an 8×-oversampled evaluation gives a bit-identical float32 table (tanh is analytic, so harmonics decay exponentially) `[MEASURED: wt_model.py]` |

`[MEASURED: wt_banks.py]` summary, all five banks: level-0 peak exactly 0.000 dB; DC ≤ −179 dB; max energy above `Kmax_L` at any level **≤ −160 dB** (float32 table noise); guards equal s[0]; all 32 frames distinct. **Gibbs overshoot at higher levels:** peak |sample| up to 1.287 (Pulse), 1.272 (Drive), 1.154 (Square), 1.103 (Formant). The quantizer clamps, and Full mode carries it through `kVoiceGain`, so it is harmless. Do not re-normalise.

### 5.2 Code skeleton (ready to adapt; ASCII only, per the Stage 1 gate)

```cpp
// Source/WavetableBank.h
#pragma once
#include <vector>
#include <cstddef>
#include <juce_core/juce_core.h>

struct WavetableBank
{
    static constexpr int kTableSize = 2048;
    static constexpr int kStride    = kTableSize + 1;   // guard sample = s[0]
    static constexpr int kLevels    = 11;
    static constexpr int kMaxFrames = 256;
    static constexpr int kmax (int L) noexcept { return L == 0 ? 1023 : (1024 >> L); }

    juce::String name;          // ASCII ("Sine->Saw"); UI uses the param choice text
    int numFrames = 0;
    std::vector<float> data;    // [level][frame][kStride]; immutable after build

    void allocate (int frames) { numFrames = frames; data.assign ((size_t) kLevels * (size_t) frames * kStride, 0.0f); }
    const float* frame (int level, int f) const noexcept { return data.data() + ((size_t) level * (size_t) numFrames + (size_t) f) * kStride; }
    float*       frame (int level, int f)       noexcept { return data.data() + ((size_t) level * (size_t) numFrames + (size_t) f) * kStride; }
};
```

```cpp
// Source/MipmapBuilder.h/.cpp  (message/worker thread ONLY)
class MipmapBuilder
{
public:
    MipmapBuilder() : fft (11), spec ((size_t) 2 * 2048), work ((size_t) 2 * 2048) {}
    enum class Normalise { peakToUnity, none };

    // spectrum: packed JUCE layout, 2*2048 floats (bins 0..1024 meaningful).
    void buildFromSpectrum (WavetableBank& b, int frame, const float* spectrum, Normalise n)
    {
        std::copy (spectrum, spectrum + 4096, spec.begin());
        spec[0] = spec[1] = 0.0f;                 // DC
        spec[2 * 1024] = spec[2 * 1024 + 1] = 0.0f; // Nyquist (Prism kept it at L0)
        if (n == Normalise::peakToUnity)
        {
            inverseLevel (0);
            float p = 0.0f;
            for (int i = 0; i < 2048; ++i) p = std::max (p, std::abs (work[(size_t) i]));
            if (p > 0.0f) for (auto& v : spec) v *= 1.0f / p;   // scale spectrum; levels never re-normalised
        }
        for (int L = 0; L < WavetableBank::kLevels; ++L)
        {
            inverseLevel (L);
            float* dst = b.frame (L, frame);
            std::copy (work.begin(), work.begin() + 2048, dst);
            dst[2048] = dst[0];                   // guard
        }
    }
    void buildFromTime (WavetableBank& b, int frame, const float* x2048, Normalise n)
    {
        std::fill (work.begin(), work.end(), 0.0f);
        std::copy (x2048, x2048 + 2048, work.begin());
        fft.performRealOnlyForwardTransform (work.data());   // standard DFT (unnormalised)
        buildFromSpectrum (b, frame, work.data(), n);        // copies into spec first: safe aliasing
    }
private:
    void inverseLevel (int L)
    {
        const int K = WavetableBank::kmax (L);
        std::copy (spec.begin(), spec.end(), work.begin());
        for (int bin = K + 1; bin <= 1024; ++bin) { work[(size_t) (2 * bin)] = 0.0f; work[(size_t) (2 * bin + 1)] = 0.0f; }
        fft.performRealOnlyInverseTransform (work.data());   // scaled 1/N by JUCE
    }
    juce::dsp::FFT fft;
    std::vector<float> spec, work;
};
// Helpers for BankFactory:
//   sine amp b_n -> spec[2n+1] = -0.5f * 2048 * b_n
//   cos  amp a_n -> spec[2n]   =  0.5f * 2048 * a_n
```

> **2.4 seam (imported bank):** CONTEXT/§A8 builds imported mips "from the int16 data" with no re-normalisation, so pass `Normalise::none`. FUNC-04 needs determinism, not identity to the int16 floats. Whether imported level 0 is copied verbatim from the canonical floats or regenerated by IFFT is 2.4's call. Keep `buildFromTime (…, Normalise::none)` available for it.

```cpp
// Source/BuiltInBanks.h/.cpp
struct BuiltInBanks
{
    static constexpr int kCount = 5, kFrames = 32;
    std::array<WavetableBank, kCount> banks;   // 0 Saw, 1 Square, 2 Pulse, 3 Formant, 4 Drive (== bank param 0..4)
    double buildMillis = 0.0;                  // logged by the harness, never gated (no wall-clock verdicts)
    BuiltInBanks();                            // MUST NOT create a SharedResourcePointer<BuiltInBanks>
};
// processor member, declared before anything that reads it:
juce::SharedResourcePointer<BuiltInBanks> builtIns;
```

"Build time logged" means storing `buildMillis` (measured with `juce::Time::getMillisecondCounterHiRes()`), printing it from the bank-check driver, and optionally a `DBG` in the ctor. Do not gate on it (memory `pattern_wallclock_inside_a_stability_verdict`).

### 5.3 Gates for 2.1: `tests/bank-check/main.cpp` (processor-console, Debug)

Construct `BuiltInBanks` directly. For each frame and level, run a **float `juce::dsp::FFT (11)`** over the 2048 table samples; `|X[k]|·2/N` is harmonic k's amplitude. Float precision is enough: the measured content above Kmax is −160 dB, against a −120 dB threshold `[MEASURED: fftfloor.py]`.

| ID | Check | Threshold | Model value |
|---|---|---|---|
| G-DIM | 5 banks × 32 frames × 11 levels × 2049; `data.size()` exact | exact | — |
| G-PEAK (DSP-04) | level-0 sample peak per frame | 1.0 ± 0.5 dB (expect 0.000) | 0.000 dB |
| G-DC (DSP-04) | level-0 mean per frame | ≤ −80 dB | ≤ −179 dB |
| G-GUARD | `s[2048] == s[0]` bitwise, every level and frame | exact | ✓ |
| G-SAW (FUNC-02) | frame k: bins 1..k > −60 dB rel h1; bins > k ≤ −120 dB; frame 1 = pure sine | — | ✓ |
| G-SQ | evens ≤ −60 dB; frame-32 top non-zero harmonic = 31; 32 distinct frames (bytewise) | — | evens −inf; h31; 32/32 |
| G-DRIVE | frame 1 h3 in [−47, −45] dB; h3, h5, h7 strictly increasing over k; evens ≤ −100 dB | — | −45.80 dB; ✓ |
| G-PULSE | frame k first null at `round(1/d_k)`, ±1 harmonic, frames 1 and 32 | — | h2, h32 |
| G-FORM | log-F interpolation hits the anchors at frames 1/8.75/16.5/24.25/32 (formula-level); 32 distinct | — | ✓ |
| G-BL | level L: max bin in `Kmax_L+1..1024` ≤ −120 dB rel max bin | ≤ −120 dB | ≤ −160 dB |
| G-POL | all sine-phase banks: frame 1 `s[512] > 0` (Saw, Square, Drive ≈ +1.0) | sign | +1.0000 |
| G-TIME | print `buildMillis` | log only | ≈ 8–20 ms |
| G-NEG | negative control: build one test frame with the Nyquist bin left in (a test-only builder flag, or a hand-built spectrum), then assert G-BL **fails** for it at L0 | must fail | — |

---

## 6. Phase 2.2: Voice + Oscillator

### 6.1 Shared read and quantizer headers (also used by Stage 3 viz and 2.4 capture)

```cpp
// Source/WtRead.h
namespace wt
{
    constexpr int kMinBandLimitedLevel = 1;   // D-A: 0 if the user rejects the floor

    // Strict level: no harmonic of level L reaches fs/2. bandlimit Off -> 0 (raw, intended aliasing).
    inline int selectLevel (double hz, double fs, bool bandlimit) noexcept
    {
        if (! bandlimit) return 0;
        const double x = hz * (double) WavetableBank::kTableSize / fs;     // Kmax_L * hz < fs/2  <=>  x < 2^L
        const int L = x > 0.0 ? (int) std::ceil (std::log2 (x)) : 0;
        return juce::jlimit (kMinBandLimitedLevel, WavetableBank::kLevels - 1, L);
    }

    inline float lerpCycle (const float* fr, int i0, float t) noexcept   // guard makes i0+1 safe for i0<=2047
    {
        const float a = fr[i0];
        return a + t * (fr[i0 + 1] - a);
    }

    // Exactly the voice's per-sample read (Interp On: frame lerp; Off: latched frame).
    inline float readSample (const WavetableBank& b, int level, double phase, bool interp,
                             float pos01, int latchedFrame) noexcept
    {
        const double idx = phase * (double) WavetableBank::kTableSize;
        int i0 = (int) idx;
        i0 = i0 < WavetableBank::kTableSize - 1 ? i0 : WavetableBank::kTableSize - 1;   // Prism IN-08
        const float t = (float) (idx - (double) i0);
        const int nF = b.numFrames;
        if (! interp)
            return lerpCycle (b.frame (level, juce::jlimit (0, nF - 1, latchedFrame)), i0, t);
        if (! (pos01 >= 0.0f)) pos01 = 0.0f;          // NaN-safe (jlimit passes NaN through)
        if (pos01 > 1.0f)      pos01 = 1.0f;
        const float fp = pos01 * (float) (nF - 1);
        int f0 = (int) fp; f0 = f0 < nF - 1 ? f0 : nF - 1;
        const int f1 = f0 + 1 < nF ? f0 + 1 : nF - 1;
        const float a = lerpCycle (b.frame (level, f0), i0, t);
        const float c = lerpCycle (b.frame (level, f1), i0, t);
        return a + (fp - (float) f0) * (c - a);
    }

    inline int latchFrame (float pos01, int nF) noexcept
    {
        if (! (pos01 >= 0.0f)) pos01 = 0.0f;
        if (pos01 > 1.0f) pos01 = 1.0f;
        return juce::jlimit (0, nF - 1, (int) std::lround (pos01 * (float) (nF - 1)));
    }
}
```

The voice should inline the hot loop: hoist the level base pointer and `nF`, use one `i0`/`t` for both frames, and read up to 4 table values per sample. `readSample` is the reference the viz renders the cycle with. **Gate:** the voice output equals `readSample` sample for sample, on a held note at sustain 1.0 (§6.6 G-READ). That gate is what makes the Stage 3 "bars = heard cycle" claim provable.

```cpp
// Source/BitQuantizer.h  (mid-rise, ARCHITECTURE section 6)
struct BitQuantizer
{
    int bits = 0; int Mi = 0; float M = 0.0f, invM = 0.0f;       // bits 0 = Full
    void setChoiceIndex (int idx) noexcept                        // 0 Full, i -> 17 - i  (16..3)
    {
        bits = idx <= 0 ? 0 : 17 - juce::jlimit (1, 14, idx);
        if (bits > 0) { Mi = 1 << (bits - 1); M = (float) Mi; invM = 1.0f / M; }
    }
    float apply (float x) const noexcept
    {
        if (bits == 0) return x;                                  // Full: bit-identical bypass (early-out)
        const float y = std::abs (x) * M;
        const int k = (y < (float) (Mi - 1)) ? (int) y : Mi - 1;  // floor; NaN/overshoot -> top level, never UB
        const float q = ((float) k + 0.5f) * invM;
        return x < 0.0f ? -q : q;                                 // sign(0) = +1 (also for -0.0f)
    }
};
// 3 bits: Mi = 4 -> levels +/-{1,3,5,7}/8 = 8 values.
```

### 6.2 `WtVoice` (replaces the Stage 1 silent voice)

Per-block inputs are pushed by the processor; voices never read APVTS (sibling rule).

```cpp
struct VoiceBlockParams
{
    const WavetableBank* bank;        // resolved ONCE per processBlock; nullptr = silence (Imported empty)
    bool interp, bandlimit;
    int  bitChoice;                   // 0 = Full
    juce::ADSR::Parameters amp;       // dirty-checked inside
    const float* posBuf;              // global per-sample position, indexed [absSample - chunkOrigin]
    int chunkOrigin;
};
```

Key behaviour:
- **prepareToPlay(sr, maxBlock):**
  - `setCurrentPlaybackSampleRate(sr)`
  - `ampEnv.setSampleRate(sr)`, then `setParameters(ampParams)`, then `appliedAmp = ampParams`, `releasing = false`
  - (2.3 adds the mod env the same way)
- **setBlockParams:**
  - Store the params.
  - ADSR dirty-check: `if (! releasing && ! sameAdsr (p, applied)) { ampEnv.setParameters (p); applied = p; }` with `juce::exactlyEqual` in `sameAdsr`.
  - `quant.setChoiceIndex`.
- **startNote(note, vel, sound, wheel):**
  - Read `wasIdle = ! ampEnv.isActive()` first.
  - Set `noteHz = getMidiNoteInHertz(note)`, `pitchWheelPos = wheel`, `updatePitch()`, `velGain = vel*vel` (S2), `noteAge = ++sNoteCounter`.
  - Set `phase = 0.0`, which is always correct here: the Synthesiser either picks an idle voice or hard-stopped the stolen one in `startVoice`.
  - Set `needsLatch = true` and `cfgFresh = true` (so the next render adopts the config with no crossfade).
  - Apply deferred ADSR params, then `releasing = false; ampEnv.noteOn();`.
- **stopNote(vel, tailOff):**
  - tailOff: `releasing = true; ampEnv.noteOff();`
  - else: `ampEnv.reset(); releasing = false; clearCurrentNote();`
  - Do **not** clear note-keyed state here (memory `pattern_per_note_state_cleared_at_noteoff_breaks_restrike`).
- **pitchWheelMoved(v):** `pitchWheelPos = v; updatePitch();`. The level is recomputed at the top of the next `renderNextBlock` call, which the Synthesiser makes right after the event.
- **updatePitch():**
  - `hz = noteHz * pow(2, ((wheel-8192)/8192*2)/12)`
  - `inc = min(hz/sr, 0.49)` (ARCHITECTURE: f clamped < 0.5·fs)
- **renderNextBlock(out, start, num):**
  1. `if (! ampEnv.isActive()) return;`
  2. `refreshReadConfig()`: `next = { bank, selectLevel(hz, sr, bandlimit), interp, bandlimit }`. If `cfgFresh`, adopt it. Else, if `next != active`, call `onReadConfigChange (active, next)`. **2.2: hard switch** (clicks are expected until 2.4); **2.4: frozen-cycle crossfade**. When `bank` or `interp` changed, re-latch (`needsLatch = true`), because a stale `latchedFrame` from a larger bank is out of bounds.
  3. `float* dst = out.getWritePointer (0);` Render mono into channel 0 only. The processor copies it to the other channels.
  4. Per sample, in this order (ARCHITECTURE Processing Order 5):
     - `env = ampEnv.getNextSample()`
     - `pos = posBuf[start - chunkOrigin + i]` (2.3 adds `+ envAmt·modEnv`, clamp, one-pole)
     - `if (needsLatch) { latched = latchFrame(pos, nF); needsLatch = false; }`
     - `s = bank ? quant.apply (read…) : 0.0f`
     - `dst[start+i] += s * env * velGain * kVoiceGain`
     - `phase += inc; if (phase >= 1.0) { phase -= 1.0; if (! interp) latched = latchFrame(pos, nF); }`
  5. Store `lastPos`, `lastLevel = active.level`, `lastFrame` for the display.
  6. `if (! ampEnv.isActive()) { clearCurrentNote(); releasing = false; }`. Lifetime follows the amp env only.
- **Mono-path API** (driven by the processor; adapted from Subtractive with the trap fixed):
  - `noteOnDirect (note, vel, retrigger)`:
    - `wasSounding = ampEnv.isActive()`
    - Set `noteHz`; `updatePitch()`.
    - If `retrigger`: `velGain = vel*vel`; if `! wasSounding`, set `phase = 0` and `needsLatch = true`; `noteAge = ++sNoteCounter`; apply deferred params; `releasing = false; ampEnv.noteOn();` with **no `reset()`**, so the attack ramps from the tail level.
    - If not `retrigger` (true legato): pitch only. Phase continues, and the envelopes are untouched. A level change goes through the config hook, which 2.4 crossfades.
  - `setPitchNote (note)`: fallback to the held note. Same as legato: pitch only, no retrigger, velocity unchanged.
  - `noteOffDirect (tailOff)`: as `stopNote`.
- **Display getters:** `isSounding() = ampEnv.isActive()`, `getNoteAge()`, `getLastPos()`, `getLastLevel()`, `getLastFrame()`.

`kVoiceGain = 0.5f` comes from ARCHITECTURE §15 ("tuned in Stage 2 against suite loudness") `[VERIFIED: ARCHITECTURE.md:174]`. At vel 127, a single voice on a peak-1.0 frame gives 0.5 × 0.5 (−6 dB default) = 0.25 peak. Confirm it at the listening checkpoint.

### 6.3 Processor `processBlock` (2.2 shape; seams marked)

```cpp
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    // 2.4: blockEntries++ HERE and blockGeneration++ on EVERY exit -> wrap in a scope guard now.
    const int numSamples = buffer.getNumSamples(), numCh = buffer.getNumChannels();
    buffer.clear();
    if (numSamples <= 0 || numCh <= 0) return;
    midiCollector.removeNextBlockOfMessages (midi, numSamples);

    const int mode = choiceIndex (voiceModeParam, 2);
    if (mode != lastVoiceMode) { synth.allNotesOff (0, false); monoStack.clear(); lastVoiceMode = mode; }

    const WavetableBank* bank = resolveBank (choiceIndex (bankParam, 6));   // once per block
    pushParamsToVoices (bank, ...);                                         // dirty-checked ADSR etc.

    for (int start = 0; start < numSamples; start += preparedBlock)        // oversized-host-block guard
    {
        const int n = juce::jmin (preparedBlock, numSamples - start);
        fillPosBuf (n);              // 2.2: knob SmoothedValue (20 ms) or raw knob; 2.3: + LFO
        for (auto* v : wtVoices) v->setChunkOrigin (start);
        chunkMidi.clear();           // ensureSize()'d in prepareToPlay -> no alloc
        for (const auto m : midi)
            if (m.samplePosition >= start && m.samplePosition < start + n)
                chunkMidi.addEvent (m.data, m.numBytes, m.samplePosition);   // ABSOLUTE positions
        if (mode == 0) synth.renderNextBlock (buffer, chunkMidi, start, n);
        else           renderMono (buffer, chunkMidi, start, n);            // Subtractive loop, absolute pos
    }

    updateDisplayFromLeadVoice();    // newest noteAge among isSounding(); relaxed atomics
    float* d = buffer.getWritePointer (0);
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));
    const float g0 = outputGain.getCurrentValue(), g1 = outputGain.skip (numSamples);
    buffer.applyGainRamp (0, 0, numSamples, g0, g1);
    for (int i = 0; i < numSamples; ++i) if (! std::isfinite (d[i])) d[i] = 0.0f;
    for (int ch = 1; ch < numCh; ++ch) buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}
```

Notes on this skeleton:
- `resolveBank(i)` returns `i < 5 ? &builtIns->banks[i] : importedBank.load (std::memory_order_acquire)`. **Declare `std::atomic<const WavetableBank*> importedBank { nullptr };` in 2.2.** It is always null for now, so Imported = silence; 2.4 publishes into it. A gate checks it: bank = 5 renders exactly 0 while the envelopes run.
- **prepareToPlay additions:**
  - `preparedBlock = max(1, samplesPerBlock)`
  - `posBuf.assign(preparedBlock, 0)`
  - `chunkMidi.ensureSize(32768)`
  - seed `outputGain` and the knob smoother from the current params (Additive:184-186)
  - `monoStack.clear()`
  - `lastVoiceMode = current`
- **Mono→Poly stale wheel (minor):** the Mono path hides wheel events from the Synthesiser. One option is to also feed wheel-only events to `synth.renderNextBlock (buffer, wheelMidi, start, 0)`. With numSamples 0 the render loop is skipped and the trailing `for_each` runs `handleMidiEvent`, which updates `lastPitchWheelValues` `[VERIFIED: juce_Synthesiser.cpp:196-235]`. Alternatively, accept and document it.
- **Knob smoother ownership:** whether 2.2 already smooths the knob is a coordination point with the 2.3 researcher. Recommendation: 2.2 ships the 20 ms `SmoothedValue` into `posBuf`. It is 5 lines and keeps the listening checkpoint free of zipper on generic-editor drags with Interp On. 2.3 then adds the LFO into the same buffer. The voice API is identical either way.

### 6.4 Pitch, level and amplitude facts (worked)

- **Level at C8 (4186 Hz):** 44.1k → x = 194.4 → L = 8 (Kmax 4; top harmonic 16.7 kHz). 96k → L = 7 (Kmax 8).
- **Level at A0:** 44.1k → L = 1 (Kmax 512). 96k with floor 0 → **L = 0**, which is the D-A failure. With floor 1 → L = 1.
- **Bend:** max wheel 16383 → +1.99976 st (sibling mapping /8192). Gate FUNC-01 with this exact formula, not with "+2.000".
- **Velocity:** `(64/127)² = 0.2540` → −11.90 dB relative to vel 127.

### 6.5 QUAL-02 feasibility (numerical model, this session)

Model `[MEASURED: wt_model.py, wt_model2.py]`:
- exact ARCHITECTURE pipeline: float32 tables, double phase, linear read, strict ceil level
- Kaiser β = 38 window, 2^17 samples, ±14-bin guard around each harmonic
- metric = max inharmonic bin relative to the max harmonic bin
- measurer floor −183.7 dB on a float32 sine, identical with JUCE's polynomial `besselI0` Kaiser

| Frame | C8 44.1k | C8 48k | C8 96k | Sweep worst 44.1k | 48k | 96k (floor 0) | 96k (**floor 1**) |
|---|---|---|---|---|---|---|---|
| Drive 32 | −122.9 | −122.8 | −115.9 | −107.1 | −107.0 | −107.0 | −107.0 |
| Saw (1023 h, test frame) | −120.2 | −120.2 | −114.3 | −74.9 | −74.9 | **−68.2 ✗** | −74.9 |
| Square = Pulse frame 1 (1023 h) | −122.8 | −122.8 | −115.6 | −74.9 | −75.0 | **−68.2 ✗** | −75.0 |
| Pulse 16 | −112.2 | −112.3 | −111.4 | −67.0 ✗ | −67.0 ✗ | −60.2 ✗ | −67.0 ✗ |
| Pulse 32 | −108.5 | −108.5 | **−97.1** | **−55.2 ✗** | −55.1 ✗ | −48.1 ✗ | −55.1 ✗ |

The same runs give **Pulse 32 measured against an equal-RMS sine** (floor 1): −67.2 / −67.0 / −67.0 dB. **4-point Hermite** (floor 1): Saw −78.1, Pulse 32 −58.7, which is not a fix. **DSP-02 demo:** band-limit Off, Saw frame 32 at C7, 44.1k = **−20.8 dB** (On: −114.4). That matches ARCHITECTURE §A2 (−20.8) `[VERIFIED: ARCHITECTURE.md:307]` and serves as the measurer's liveness control.

**Recommended gate** (for the planner; decisions D-A and D-B are flagged):
- **G-Q2-C8:** Drive 32 and Saw-1023 (a test-only bank built through the same `MipmapBuilder`; see the note below) plus Pulse frame 1, at C8 (MIDI 108), fs 44.1/48/96 kHz, band-limit On. Threshold ≤ −100 dB. Expected margin ≥ 14 dB.
- **G-Q2-SWEEP:** the same three frames, MIDI 21..108 (every note, or every 3rd plus all of 21..33), three rates. Threshold ≤ −70 dB. Requires D-A floor 1 at 96 kHz. Margin ≈ 5 dB, so do not loosen the measurer.
- **G-Q2-PULSE (pending D-B):** Pulse 16/24/32, metric relative to equal-RMS sine, ≤ −60 dB (model −67), **or** a documented exception.
- **Negative control (mandatory):** band-limit Off on Saw 32 at C7 must read > −40 dB. It also asserts the selected L and `Kmax_L·f < fs/2` per note, white-box but cheap.

A Saw-1023 frame cannot be selected through the processor (no built-in bank has it). Two options: (a) a voice-level arm that drives a `WtVoice` directly with a test bank pointer; or (b) Pulse frame 1 (square, odd harmonics to 1023), which is processor-reachable and measured identical to Saw-1023 (−74.9 / −68.2). **Recommendation:** use (b) for the processor gate, and (a) only if the planner wants the exact "1023-harmonic saw" wording.

**C++ analyzer recipe:**
- `juce::dsp::FFT fft (17)` (float; the model shows float = double to 0.1 dB at these levels `[MEASURED: fftfloor2.py]`)
- `juce::dsp::WindowingFunction<float>::fillWindowingTables (w, 131072, kaiser, false, 38.0f)`
- skip the attack (start the window ≥ 50 ms after note-on, sustain 1.0, vel 127, output 0 dB)
- `performFrequencyOnlyForwardTransform`
- mask ±14 bins around `k·f` for all `k·f < fs/2` plus DC
- report `20·log10(maxInh / maxHarm)`, and print the bin/frequency of the worst component

### 6.6 Gates for 2.2: `tests/dsp-check/main.cpp` (processor-console, Debug; plus P6 flip)

| ID | Req | Stimulus | Assertion | Negative control |
|---|---|---|---|---|
| G-PITCH | FUNC-01 | Sine→Saw pos 0 (pure sine), MIDI 21..108, 1 s each, 48k and 44.1k, sustain 1.0 | Single-bin DFT phase at f0 on two Hann windows 0.5 s apart → δf → **\|cents\| ≤ 1**. Same at pos 1.0 (frame 32), measuring h1, so Position changes timbre, not pitch | Wheel 8192+68 (≈ +1.7 cents) must FAIL |
| G-BEND | bend | Wheel 16383 and 0 on A4 | f = 440·2^(±1.99976/12), ±1 cent | — |
| G-Q2-* | QUAL-02 | §6.5 | §6.5 | DSP-02 arm |
| G-DSP02 | DSP-02 | Band-limit Off, Saw 32, C7, 44.1k | max inharmonic **> −40 dB** (model −20.8); On ≤ −100 | is itself the control |
| G-DSP01 | DSP-01 | **fs = 56320**, A4 (inc = 2^-7 exactly, so the period is 128 samples and the phase is exact), sustain 1.0, `position` stepped 0→1 by `setValueNotifyingHost` once per 128-sample block over ≥ 2000 blocks | Interp Off: count distinct 128-sample cycles (bitwise) = **32**, changes only at cycle boundaries. Interp On: distinct cycles ≫ 32 (≥ 500), and the max per-cycle spectral step (h1..h32 vector L2) < 1/10 of Off's max step | Off arm with the latch disabled (a test toggle is not available), so instead require Off ≠ On |
| G-DSP03 | DSP-03 | bit_depth "3", vel 127, sustain 1.0, output 0 dB, Saw 32, steady window | **exactly 8 distinct sample values**, = ±{1,3,5,7}/16 (×kVoiceGain 0.5) | bit_depth Full on the same window: > 1000 distinct values |
| G-FULL | DSP-03 | `BitQuantizer` bits 0 over 10^6 values (±0, subnormals, ±1.3, random) | bitwise identity; processor Full render after toggling 3→Full equals a fresh Full render bytewise | — |
| G-READ | viz seam | held note, sustain 1, Interp On and Off, bit 3 | voice output / (velGain·kVoiceGain·env) equals `wt::readSample` + `quant.apply` per sample within 1 ulp | — |
| G-POLY | FUNC-07 | 16 notes (C3 + 16 semitones…), then a 17th | all 16 f0 bins present (binAmplitude > −30 dB rel); after the 17th: 16 sounding, the 17th present, the oldest gone | — |
| G-MONO | FUNC-07 / S1 | Mono: C4 held, E4 overlap at 0.2 s, release E4 at 0.6 s | 0.25–0.55 s: E4 bin ≫ C4 (×4); **no re-attack**: RMS over the 20 ms after E4-on ≥ 0.9× RMS before; 0.65 s on: pitch back to C4, still no re-attack; release all → tail decays | Poly mode on the same events shows both notes |
| G-RETRIG | pitfall 4 | Mono: note, release (0.3 s), new note at 0.1 s into the tail | max \|Δsample\| at the new note-on ≤ steady max step × 1.5 (no reset to 0) | — |
| G-NOTEOFF | ADSR pitfall | sustain 0, decay 3 s, release 0.3 s, note-off at 0.85 s (Subtractive #15) | `tailRms > 0.2·preRms` | Calling `setParameters` every block reproduces the failure (documented, not shipped) |
| G-VEL | S2 | vel 127 vs 64, steady window | ratio = −11.90 ± 0.05 dB | — |
| G-OUT | output stage | output −60 dB | **every sample exactly 0.0f**; with 0 dB, peak at 10 ms after note-on ≈ 0.5 (not ramping from 0: seeding) | unseeded smoother gives ≈ 0.25 |
| G-IMPEMPTY | decision 10 | bank 5 (Imported) with a note held, then switch to bank 0 mid-note | exactly 0 while on 5; sound after the switch (env kept running) | — |
| G-BLOCK | oversized block | prepare(256) then one 1024-sample block vs prepare(256) + 4×256 | bit-identical (no per-block-rate state in 2.2) | — |
| G-ALLOC | PERF-01 | O-Bells `malloc_logger` (volatile, thread-scoped, liveness probe). Warm-up block unarmed; `midi.ensureSize`; armed blocks cover: note-ons, the 20-note steal, wheel sweep, bank 0→4→5→0, interp/bandlimit/bit toggles, Poly↔Mono switch, release tails, an oversized block | **0** allocations | the liveness malloc counts 1 |
| G-FINITE | — | all of the above | `allFinite` | — |
| P6' | state-check | flip P6 to "note-on/20-note/UI-MIDI blocks are finite and bounded (\|x\| ≤ 4); 0-sample block safe; output −60 → exact 0" | — | — |

Determinism: 2.2 has no RNG, and phase resets at note-on, so renders are byte-stable. Goldens, if any, are checksum-only (memory `pattern_golden_tracked_as_checksum_only`).

---

## 7. Pitfalls specific to 2.1/2.2 (ranked)

1. **Out-of-bounds table read.** It comes from (a) a NaN position (`jlimit` returns NaN; `(int)NaN` is undefined behaviour), or (b) `latchedFrame` surviving a bank change to fewer frames (256 → 32), or an Interp toggle. **Avoid:** NaN-safe clamps in `readSample`/`latchFrame` (§6.1); re-latch on any bank or interp config change; clamp `i0 ≤ 2047`. **Warning sign:** a crash under pluginval param fuzz.
2. **QUAL-02 drift:** a Nyquist bin left at L0 (Prism `WavetableGenerator.cpp:173`), any level blend (Prism `WavetableOscillator.cpp:245-266`), floor-0 at 96k (D-A), or narrow pulses (D-B). **Avoid:** G-BL, G-NEG, G-Q2 with the DSP-02 negative control.
3. **ADSR per-block push** (`juce_ADSR.h:262-279`). Dirty-check, skip while releasing, apply deferred at note-on. Gate G-NOTEOFF.
4. **Mono retrigger click** from Subtractive's `reset()` + `noteOn()` (`SubVoice.h:230-233`) on a releasing voice, and from resetting the phase while sounding. Gate G-RETRIG.
5. **The "was idle" test via `getCurrentlyPlayingNote()`** is always true in `startNote` (Synthesiser sets it first). Use `! ampEnv.isActive()` before `noteOn()`.
6. **P6 flip.** Stage 1's silent-render probe fails once 2.2 makes sound, so update it in the same change.
7. **Unseeded smoothers** (output gain, knob) fade in from 0 at every `prepareToPlay` (memory `pattern_gain_ramp_without_seeding…`). A NaN that reaches a `SmoothedValue` is sticky across `reset()` (memory `critical_smoothedvalue_reset_does_not_clear_a_nan`), so guard the sources. Gate G-OUT.
8. **Sub-block rendering.** The Synthesiser calls `renderNextBlock` several times per block (min sub-block 32). Index `posBuf` by `start − chunkOrigin`, and recompute the level and config at the top of every call.
9. **Oversized host block plus sliced MIDI.** Never pass the full MidiBuffer to a sub-range `synth.renderNextBlock`: it fires all later events early (`juce_Synthesiser.cpp:207-235`). Gate G-BLOCK.
10. **Lead voice.** Use `isSounding()` and `noteAge`. Not `isVoiceActive()` (false for the mono voice), and never shadow `isVoiceActive`.
11. **`SharedResourcePointer`:** no recursion in `BuiltInBanks()` (deadlock). The build runs under a SpinLock. Banks are rebuilt when the instance count reaches 0. Never construct it on the audio thread.
12. **Pulse frames are cosine-phase.** They are non-zero at φ=0 (frame 1 = −0.85). Onset and Interp-Off latch steps exist on Pulse, which CONTEXT accepts. Do not "fix" it by shifting the phase: that changes the locked §A6 formula.
13. **Hard switches in 2.2.** Bank, interp, bandlimit and level changes (bend or legato across an octave boundary) click until 2.4's crossfader. List them as known for the listening checkpoint so they are not reported as regressions.
14. **Build hygiene:**
    - ASCII-only sources, including comments (Stage 1 gate)
    - `-Wfloat-equal` → `juce::exactlyEqual`
    - choice params resolved by `round` + clamp
    - new `.cpp` added to `target_sources`, or the console drivers silently miss them
15. **Gate design:**
    - gate the render, not voice state (memory `pattern_pointer_moved_is_not_audio_changed`)
    - every gate gets a negative control (memory `pattern_gate_stimulus_below_threshold_is_vacuous`)
    - no wall-clock verdicts (build time is logged only)
    - the alloc gate uses `malloc_logger` with a liveness probe, not an `operator new` counter
16. **Gate scope** (memory `pattern_gate_inherits_the_review_findings_scope`): G-Q2 must not test only the one frame ARCHITECTURE named. Include Pulse 1 and record the D-B decision, so a green gate does not hide the narrow-pulse case.

---

## 8. Seams 2.2 must leave for 2.3 / 2.4

| Seam | 2.2 delivers | Consumer |
|---|---|---|
| Global position buffer | `posBuf[preparedBlock]` filled per chunk (smoothed knob); voice reads `posBuf[start - chunkOrigin + i]` | 2.3 adds the LFO into `posBuf`; adds `envAmt·modEnv`, the clamp, and the 2 ms one-pole per voice (seeded at note-on) |
| Mod env slot | voice has the ADSR dirty-check helper and a deferred-apply path that a second envelope can reuse; lifetime is gated on the amp env only | 2.3 |
| Chunk loop and sliced MIDI | in place | 2.3 (LFO buffer sized to `preparedBlock`) |
| Read-config hook | `ReadConfig {bank, level, interp, bandlimit}`, `onReadConfigChange(old, new)` called at the top of each render call and after mono pitch changes; 2.2 = hard switch + re-latch | 2.4 frozen-cycle capture (ping-pong `float[2049]` per voice; render the "current output cycle" via `wt::readSample` from the **old** config) |
| Bank pointer resolve | `resolveBank()` once per block; `std::atomic<const WavetableBank*> importedBank {nullptr}` declared | 2.4 publish/retire/reaper |
| Block-entry/exit counters | processBlock structured with a single exit or a scope guard | 2.4 REG-01 `blockEntries` / `blockGeneration` on every return path |
| Builder API | `MipmapBuilder::buildFromTime (…, Normalise::none)` | 2.4 importer and restore worker |
| Display atomics | `dispPos`, `dispLevel`, `dispFrame`, `dispSounding` (relaxed); add `bankGen` in 2.4 | Stage 3 viz |
| Shared read/quant | `WtRead.h`, `BitQuantizer.h` | Stage 3 viz (exact cycle) and 2.4 capture |

---

## 9. Proposed file layout (flat `Source/`, matching Stage 1 and the O-simple* family)

```
Source/
  WavetableBank.h        2.1  immutable flat bank (11 levels x N x 2049)
  MipmapBuilder.h/.cpp   2.1  FFT(11) owner, spectrum/time -> levels (DC+Nyquist zero, peak-normalise opt)
  BankFactory.h/.cpp     2.1  five generators (A3-A7) -> spectra/time -> builder
  BuiltInBanks.h/.cpp    2.1  std::array<WavetableBank,5> + buildMillis; via SharedResourcePointer
  WtRead.h               2.2  selectLevel / lerpCycle / readSample / latchFrame (shared with viz + 2.4)
  BitQuantizer.h         2.2  mid-rise, Full = early-out
  MonoStack.h            2.2  verbatim from O-simpleSubtractive PluginProcessor.h:85-131
  WtVoice.h(/.cpp)       2.2  replaces Stage 1 silent voice
  PluginProcessor.h/.cpp 2.2  builtIns member, importedBank atomic, posBuf, chunkMidi, outputGain, knob smoother,
                              monoStack, renderMono, display atomics, processBlock per section 6.3
tests/
  bank-check/main.cpp    2.1  section 5.3 gates (+ buildMillis print)
  dsp-check/main.cpp     2.2  section 6.6 gates (+ --alloc-check, Kaiser analyzer)
  state-check/main.cpp   2.2  P6 flipped
CMakeLists.txt           add the new .cpp to target_sources; two more ouaricon_add_processor_console() calls
```

ASCII-only throughout. Bank display names come from the param choice strings, so `BankFactory` names can be plain ASCII.

---

## 10. Build, verify and listening-checkpoint procedure (after 2.1 + 2.2)

1. **Offline gates first** (bash, out-of-repo, Debug, per the Stage 1 recipe): configure `$SCRATCH/build-oswt` with `-DOUARICON_BUILD_TESTS=ON -DSKIP_PLUGINS=$SKIP`. Build `O-simpleWavetable-bank-check O-simpleWavetable-dsp-check O-simpleWavetable-state-check` in the background (the 600 s watchdog, per memory). Then run each, requiring `exit=0` and `grep -c 'JUCE Assertion failure' = 0`. Run dsp-check once more with `--alloc-check`.
2. **Plugin build and install:** `./scripts/build-and-install.sh O-simpleWavetable`. It sweeps the alternate `-dev`/unsuffixed variants, clears the AU caches, and installs `O-simpleWavetable-dev.{vst3,component}`. The script does **not** rebuild the Standalone (memory `pattern_build_install_skips_standalone_stale_ui`). If the Standalone is the listening surface, also `cd build && ninja O-simpleWavetable_Standalone`. Its editor is still the Stage 1 generic editor.
3. **Host validation:**
   - `bash scripts/verify-au-link.sh O-simpleWavetable` (targeted `auval -v aumu OSiW OuDv`; background it, since a cold rescan can take minutes)
   - pluginval strictness 10, VST3 and AU. The NaN param fuzz exercises pitfall 1.
4. **Independent installed-binary check (pedalboard):** `uv run --python 3.12 --with pedalboard --with numpy python check.py`. Load `~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3` and set params by `raw_value` (the key for output_level is `output_level_db` `[VERIFIED: stages/1-foundation/VERIFICATION.md:92]`). Settle with `reset=True`, render C4 with `reset=False`, and assert `isfinite`. Check f0 within 1 cent, bank 5 silent, and band-limit Off vs On at C7 Saw 32 (alias jump > 60 dB).
5. **User listening checkpoint** (Logic AU or Reaper VST3, generic editor). Suggested script:
   - **Bank character:** each bank at Position 0 / 50 / 100 %, with C2–C5 chords in Poly.
   - **Stepping vs smooth:** Interp Off, drag Position slowly. Expect 32 audible steps on Sine→Saw and Sine→Square. Interp On should be continuous.
   - **Aliasing A/B:** Drive at Position 100 %, play C6–C8. Band-limit Off should be clearly aliased; On should be clean but duller at the top (strict mips).
   - **Bit depth:** Full → 8 → 4 → 3 on a sustained sine (Sine→Saw at 0 %). 3 bits = 8 levels; expect grit.
   - **Voices:** a 16-note chord; Mono true legato (overlap = pitch only, release returns to the held note); ±2 st bend.
   - **Velocity:** soft playing is genuinely quiet (squared).
   - **Known until 2.4** (do not report as regressions): clicks on bank switch, band-limit/interp toggles, and bends or legato moves that cross an octave level boundary. Imported is silent. Position has no LFO or mod env until 2.3.
6. **Commit** (orchestrator): path-scoped `plugins/O-simpleWavetable` (and `PLUGINS.md` if touched), using the shared-checkout temp-index discipline. Re-check `git branch --show-current` and `git status --short` immediately before committing.

---

## 11. Assumptions log

| # | Claim | Risk if wrong |
|---|---|---|
| A1 | `kVoiceGain = 0.5` gives suite-appropriate loudness `[ASSUMED, from ARCHITECTURE]` | Too loud or quiet at the checkpoint; it is one constant |
| A2 | Steal-induced hard cuts (JUCE `stopNote(0,false)`) are acceptable, as in the siblings `[ASSUMED]` | Clicks with 17+ held notes; fallback is a quick-fade steal (out of scope) |
| A3 | The equal-RMS-sine metric is a fair reading of "aliases ≥ 60 dB down" for narrow pulses `[ASSUMED]` | D-B; needs a user decision |
| A4 | 2.3 accepts the `posBuf` + `chunkOrigin` voice API `[ASSUMED]` | Small API churn between the parallel researchers |

## 12. Open questions (for planner and user)

1. **D-A:** accept the band-limit-On level floor of 1? (Recommended. It is required for the −70 dB sweep at 96 kHz.)
2. **D-B:** narrow-pulse frames vs QUAL-02: an exception, the equal-RMS metric, or both?
3. Does 2.2 or 2.3 own the 20 ms knob smoother? (Recommended: 2.2, so the checkpoint has no zipper.)
4. G-Q2's "1023-harmonic frame": use Pulse frame 1 (processor-reachable, measures identically), or add a voice-level Saw-1023 arm?

## 13. Environment

| Dependency | Available | Note |
|---|---|---|
| JUCE 8.0.15 at `/Users/taylorbrook/JUCE` | ✓ | `project(JUCE VERSION 8.0.15 …)` |
| uv at `~/.local/bin/uv` (numpy, pedalboard via `--python 3.12`) | ✓ | used this session for numpy |
| clang / Accelerate (vDSP) | ✓ | fftbench built and ran |
| pluginval, auval | ✓ (Stage 1 used them) | `auval -a` SIGABRTs, so use the targeted form |

No new packages enter the plugin (JUCE modules only), so a package legitimacy audit does not apply. numpy and pedalboard are verification-only tools in the uv cache.

## 14. Sources

- **Repo (read this session):**
  - O-simpleWavetable: `.planning/{stages/2-dsp/CONTEXT.md, research/ARCHITECTURE.md, ROADMAP.md, REQUIREMENTS.md, parameter-spec.md, stages/1-foundation/{SUMMARY,VERIFICATION,PLAN,RESEARCH}.md}`, `Source/{PluginProcessor.h,.cpp, WtVoice.h}`, `CMakeLists.txt`, `tests/state-check/main.cpp`
  - O-Prism: `Source/dsp/{WavetableData.h, WavetableGenerator.cpp, WavetableOscillator.h,.cpp}`
  - O-simpleSubtractive: `Source/{PluginProcessor.h,.cpp, SubVoice.h}`, `tests/render-harness/main.cpp`
  - O-simpleAdditive: `Source/{AdditiveVoice.h, PluginProcessor.cpp}`
  - O-simpleFM: `tests/render-harness/{CMakeLists.txt,main.cpp}`
  - O-Bells: `tests/render-harness/main.cpp`
  - shared: `scripts/param-dump/ParamDump.cmake`
- **JUCE 8.0.15 (read this session):** `juce_dsp/frequency/juce_FFT.{h,cpp}`, `juce_dsp/frequency/juce_Windowing.cpp`, `juce_dsp/maths/juce_SpecialFunctions.cpp`, `juce_core/memory/juce_SharedResourcePointer.h`, `juce_audio_basics/utilities/{juce_ADSR.h, juce_SmoothedValue.h, juce_Decibels.h}`, `juce_audio_basics/synthesisers/juce_Synthesiser.{h,cpp}`, `juce_audio_basics/midi/{juce_MidiMessage.cpp, juce_MidiBuffer.{h,cpp}}`.
- **Project memory:** `index_dsp_gate_design`, `pattern_adsr_setparameters_per_block_kills_release`, `pattern_gain_ramp_without_seeding_fades_in_from_silence`, `critical_smoothedvalue_reset_does_not_clear_a_nan`, `critical_iir_filter_default_ctor_is_first_order` (not applicable: no IIR here), `pattern_shared_seed_across_render_jobs_is_common_random_numbers` (not applicable to 2.2: no RNG), `pattern_verify_installed_binary_with_pedalboard_via_uv`, `pattern_oversampler_overruns_on_oversized_host_block`, `pattern_distinct_buffer_addresses_are_not_an_allocation_bound`, `pattern_gate_inherits_the_review_findings_scope`, `pattern_gate_stimulus_below_threshold_is_vacuous`, `pattern_malloc_logger_alloc_gate_two_traps`, `pattern_operator_new_counter_blind_to_heapblock…`, `critical_synthesiser_startvoice_sets_note_before_startnote`, `pattern_per_note_state_cleared_at_noteoff_breaks_restrike`, `pattern_exact_cycle_gate_needs_exactly_representable_f0`, `pattern_offline_dsp_render_harness`, `reference_scratch_processor_harness_recipe`, `pattern_render_harness_breaks_on_webview_editor`.
- **Scratch scripts (this session, in `research-probes/`):** `wt_model.py`, `wt_model2.py`, `wt_banks.py`, `fftfloor.py`, `fftfloor2.py`, `kaiserjuce.py`, `fftbench.cpp`.

**Valid until:** about 2026-11-05 (stable domain; JUCE pinned to 8.0.15).

---

# Part B — Phases 2.3 (Modulation → Position) + 2.4 (Bank Switch / Import / Persistence)

**Researched:** 2026-10-05 · **Scope:** 2.3 and 2.4 only. 2.1 and 2.2 are assumed (see §0 interface contract).
**Sources:** read this session: CONTEXT/ROADMAP/ARCHITECTURE/parameter-spec/REQUIREMENTS, Stage 1 SUMMARY/VERIFICATION, `Source/*`, `tests/state-check/main.cpp`, the v1 integration checklist and editor template, O-Prism, O-simpleGrain, O-TextureForge, O-GrainScatter, O-FreqPulse, O-Bells harness, JUCE 8.0.15 sources, and the memory notes listed in the brief.
**Tags:** `[VERIFIED: path:line]` means I read the file this session. `[PROBE]` means I compiled and ran a falsification probe this session against the pinned JUCE 8.0.15. `[ASSUMED]` means training or engineering judgement, not verified.

---

## Summary + Top pitfalls

**Bottom line:** 2.3 is straightforward if every modulation source is rendered **per sample** into a preallocated block buffer, and the per-voice smoother is seeded at note-on (lazily, at the first rendered sample). 2.4 is buildable on JUCE 8.0.15 as specified. The FLAC16 in-memory round trip is **proven lossless** by a probe run this session. But there is one design collision that the plan must resolve before any code is written.

**Top pitfalls (ranked)**

1. **CRITICAL: Prism's quiescent rule plus the frozen-cycle capture is a use-after-free.**
   - Prism's quiescence rule is sound only because "a block that starts after that read runs updateWavetableAssignments **before it touches a wavetable**" [VERIFIED: O-Prism/Source/PluginProcessor.cpp:1112-1121]. In Prism, a new block never reads the *old* table.
   - Our frozen-cycle crossfader does exactly that. At the first block after a swap, each voice renders its *current output cycle from the OLD bank* (ARCHITECTURE §5) [VERIFIED: ARCHITECTURE.md:113-118].
   - Failure sequence: the host idles the audio thread (`entries == exits`), the user imports, the reaper frees the old imported bank at once, and the next `processBlock` captures from freed memory.
   - **Fix:** port Prism REG-01 verbatim and add ONE predicate. The audio thread publishes `audioHeldBank` (the bank its voices will capture from next block), and the sweep never frees that pointer. See §1.3. Gate it with a test-build "graveyard" flag plus a negative control (§7.3), because ASan is unusable here.
2. **Memory ordering.** Prism uses release/acquire for the publish store, the stamp load and the counter loads. The "publish, then read exits" pair is a store→load sequence, which acq/rel does not order. Use `seq_cst` (the default) on the publish store, the stamp, the sweep loads, the audio-side counter increments and the pointer load. On AArch64 the cost is nil [ASSUMED]. §1.3.
3. **FUNC-04 breaks if import builds the bank from pre-quantization floats while reload builds it from int16.** Quantize first, then call ONE `buildImportedBank(const ImportedPcm&)` from the importer, from restore, and from the test (§5.2).
4. **`setStateInformation` threading and an in-flight import.**
   - Some hosts call `setStateInformation` off the message thread.
   - An import that finishes after a restore will overwrite the restored bank.
   - So bank ownership must sit under a `CriticalSection` that the audio thread never takes (not "message thread only"). Restore bumps the import generation to supersede any worker in flight (§6.3) [VERIFIED pattern: O-simpleGrain/Source/PluginProcessor.h:350-372; memory texture_forge_swap_needs_lock].
5. **Use standard base64 only.** Use `juce::Base64::toBase64` / `Base64::convertFromBase64` in both directions. ARCHITECTURE names `MemoryBlock::toBase64Encoding`, which is JUCE's proprietary `"<size>.<alt-alphabet>"` format [VERIFIED: JUCE juce_MemoryBlock.h:266-281; memory critical_webview_drag_drop_macos]. Lock the standard form now, while no released session exists.
6. **FLAC writer quirks (JUCE 8.0.15).**
   - The new API is `createWriterFor(std::unique_ptr<OutputStream>&, AudioFormatWriterOptions)`, and the writer **owns** the stream.
   - The writer must be destroyed before the `MemoryBlock` is read. `finish()` seeks back and rewrites STREAMINFO at that point.
   - Only 16/24 bits are accepted.
   - For exactness, write left-justified ints (`s << 16`) and read back through the `int*` API with `>> 16`.
   - All of this is [VERIFIED: juce_FlacAudioFormat.cpp:408-526, 575-619] and [PROBE].
7. **The click gate on a Square/S&H LFO at 100% depth may fail with a ONE-pole 2 ms smoother.** The one-pole starts at full velocity, so a square edge produces a slope kink of about 0.16/sample on Sine→Saw [ASSUMED model]. That matches memory `pattern_one_pole_delay_glide_is_a_pitch_jump`. Measure first with a negative control. The pre-authorized fallback is two cascaded 1 ms poles (velocity-continuous) (§3.3).
8. **Crossfade re-trigger mid-fade.** Fold the *mixed* cycle into the new frozen buffer, and never drop the outgoing frozen buffer. Keep a running-length member, and storm-proof bend jitter with a downward-only level hysteresis, which is alias-safe (§4) [memory jump_during_crossfade_drops_outgoing_head, per_jump_fade_needs_running_length_member].
9. **Determinism.**
   - S&H should be drawn from `hash(seed, cycleIndex)` rather than a sequential `juce::Random`. That makes it position-addressable, block-size invariant and loop-repeatable. Note that `juce::Random` seeds n and n+1 give near-identical first draws (memory).
   - Add a per-job test seed hook.
   - The `juce::Synthesiser` minimum sub-block (32, non-strict) breaks block-size invariance of note-on timing [VERIFIED: juce_Synthesiser.cpp:217] (§2.6).
10. **The console harness has no message loop.**
    - `JUCE_MODAL_LOOPS_PERMITTED` defaults to 0 [VERIFIED: juce_PlatformDefs.h:323,328], so `callAsync` and `Timer` never fire.
    - Design publish, retire and sweep to be callable from any thread under the lock.
    - Do the auto-select-Imported step through an `AsyncUpdater`, which the harness drives with `handleUpdateNowIfNeeded()` [VERIFIED: juce_AsyncUpdater.h:100].

**Naming conflict to resolve in PLAN:** CONTEXT says "e.g. `requestImport (const juce::File&)`". The Stage 3 editor template already calls `importFromFile(File)`, `importFromMemory(name, MemoryBlock&&)`, `getImportStatus()` and `getImportStatusVersion()` [VERIFIED: mockups/v1-PluginEditor.cpp:41-48]. **Use the template names** so Stage 3 needs no rename. `requestImport` was only an example.

**Spec conflict to flag:** ARCHITECTURE §11 says Mono "retrigger on every new note". CONTEXT locks **true legato**: amp and mod env continue [VERIFIED: CONTEXT.md:36]. CONTEXT wins. For 2.3 this means a legato transition does NOT retrigger the mod env and does NOT reseed the position smoother.

**Correction to ARCHITECTURE:** `PositionInfo::getIsPlaying()` returns `bool`, not `Optional` [VERIFIED: juce_AudioPlayHead.h:376]. `getBpm()` and `getPpqPosition()` are `Optional<double>` [VERIFIED: :313, :358].

---

## 0. Interfaces 2.3/2.4 need from 2.1/2.2 (contract for the parallel researcher)

All names are proposals. **The shapes and threading are what 2.3/2.4 rely on.**

```cpp
// --- 2.1 -------------------------------------------------------------------
struct WavetableBank                        // IMMUTABLE after construction
{
    static constexpr int kTableSize = 2048, kStride = 2049, kLevels = 11, kMaxFrames = 256;
    int numFrames = 0;                      // 32 built-in, 1..256 imported
    juce::String name;
    std::vector<float> data;                // [((level*numFrames)+frame)*kStride + i], [2048] = guard
    std::vector<float> thumbs;              // numFrames * 128, level 0 (Stage 3 bankUpdate)
    const float* frame (int level, int f) const noexcept
    { return data.data() + ((size_t) (level * numFrames + f)) * kStride; }
   #if OSIW_TEST_HOOKS
    mutable std::atomic<bool> testReaped { false };          // graveyard flag (§7.3)
    static inline std::atomic<int> testLiveCount { 0 };      // ctor ++ / dtor --  (leak gate)
   #endif
};

namespace MipmapBuilder
{
    // REENTRANT: owns its own juce::dsp::FFT + scratch per call (no static/shared state).
    // Required because the import worker and setStateInformation can build concurrently.
    std::unique_ptr<WavetableBank> buildFromLevel0 (const float* level0, int numFrames, const juce::String& name);
}
struct BuiltInBanks { const WavetableBank* get (int index) const noexcept; };   // SharedResourcePointer, never freed

// One read function shared by the voice, the frozen-cycle capture and the
// message-thread cycle renderer ("shared header", ARCHITECTURE §Visualization).
inline float readCycle (const WavetableBank& b, int level, float framePos /*0..N-1*/, bool interp,
                        int latchedFrame, double phase) noexcept;

// --- 2.2 -------------------------------------------------------------------
struct BlockContext                        // filled by the processor BEFORE synth.renderNextBlock
{
    const float* knobPos;                   // [blockSize] 20 ms smoothed position   (2.3)
    const float* lfo;                       // [blockSize] -1..1 global LFO           (2.3)
    const float* lfoDepth;                  // [blockSize] smoothed depth             (2.3, recommended)
    const float* envAmount;                 // [blockSize] smoothed env_amount        (2.3, recommended)
    const WavetableBank* bank;              // resolved ONCE per block; nullptr = silent (2.4)
    bool interp, bandlimit;
    int  bitDepthIndex;
};
class WtVoice  // additions 2.3/2.4 need
{
    void setBlockContext (const BlockContext* c) noexcept;          // pointer stable for the life of the voice
    // startNote: needsSeed = true; modEnv.noteOn(); lastCfg = {ctx->bank, level, interp, bandlimit}; xfade off
    // legato pitch change (Mono): NO modEnv retrigger, NO reseed (CONTEXT true legato)
    // renderNextBlock(out, startSample, n): reads ctx arrays at [startSample + i]
    // per-block + post-wheel-sub-block: compare cfg -> triggerCrossfade()      (2.4, §4)
};
```

Requirements on 2.2 that come from 2.3/2.4:
- `WtVoice` indexes the global buffers with the **absolute** in-block index `startSample + i`. `juce::Synthesiser` passes sub-block offsets.
- The voice keeps `lastCfg.bank` **only while active**. `startNote` overwrites it with no capture, which prevents a capture from a stale pointer held by an idle voice.
- `synth.setMinimumRenderingSubdivisionSize (1, true)`, if the block-size-invariance gate (§2.6) is adopted [VERIFIED semantics: juce_Synthesiser.cpp:217].
- **Synthesiser takes a `CriticalSection` inside `processNextBlock`** [VERIFIED: juce_Synthesiser.cpp:193]. Never call `synth.*` from the message thread (e.g. on a Poly/Mono switch). Detect the switch in `processBlock`.

---

## 1. RT-safe imported-bank publish and the REG-01 reaper

### 1.1 Prism REG-01 source (to port)

- **Commit:** `0f7d65ce` "improve(O-Prism): v1.27.0 — wavetable publish path (CR-01, CR-02) + REG-01..04". This is the commit that introduced `blockEntries` [VERIFIED: `git log -S blockEntries`]. The current file is at v1.30.1, `bd998d29`.
- **The entry-counter fix IS in it.**
- Files and lines:

| What | Where |
|---|---|
| Members: `blockGeneration` ("blocks FINISHED"), `blockEntries` ("Blocks STARTED"), `RetiredTable{unique_ptr, retiredAt}`, `retiredTables` ("message thread only") | [VERIFIED: O-Prism/Source/PluginProcessor.h:450-478] |
| Entry increment, first statement after `ScopedNoDenormals` | [VERIFIED: PluginProcessor.cpp:778-782] |
| Zero-channel early return also bumps exits | [VERIFIED: :803-807] |
| Exit increment, last statement | [VERIFIED: :1061-1062] |
| `retireTable`: sweep first (producer sweep), then push `{table, blockGeneration.load(acquire)}` | [VERIFIED: :1089-1102] |
| `sweepRetiredTables` | [VERIFIED: :1112-1160] |
| Timer: `startTimer (30)`, sweep in `timerCallback` | [VERIFIED: :642, :1162-1171] |
| Gate: `tests/edit_rotation_check.cpp` [A1]: 200 edits with no `processBlock` leave ≤ 3 held; counters, not addresses | [VERIFIED: O-Prism/CHANGELOG.md:1160-1173] |

The sweep body, quoted verbatim:

```cpp
    const auto exits    = blockGeneration.load (std::memory_order_acquire);
    const auto entries  = blockEntries.load (std::memory_order_acquire);
    const bool quiescent = (entries == exits);
    ...
    const auto unreachable = [exits, quiescent] (uint64_t stamp)
    {
        return quiescent || exits >= stamp + 2;
    };
```
[VERIFIED: PluginProcessor.cpp:1119-1143]. The rest of the function (coolingEditBuffer) is Prism-editor-specific. **Drop it.** Nothing else changes.

### 1.2 Siblings, and why neither is enough here

- **O-simpleGrain** [VERIFIED: O-simpleGrain/Source/PluginProcessor.cpp:396-418, .h:350-375]:
  - raw `std::atomic<AudioBuffer*>` view for the audio thread
  - `shared_ptr` ownership under `sourceStateLock` ("Taken ONLY off the audio thread (message + host-controlled threads…)")
  - `retiredSources` reaped at `blocksRendered >= parked + 2`
  - Problem: **no entry counter**, so it freezes when the host idles (bounded at one buffer only because it publishes rarely).
  - Its decode is synchronous on the calling thread. **There is no worker thread in O-simpleGrain.** The brief's "O-simpleGrain import worker" does not exist. Only its decode/publish shape and the drop-streaming module are reusable.
- **O-TextureForge** [VERIFIED: O-TextureForge/Source/PluginProcessor.cpp:335, .h:153-155]:
  - `retiredCorpus = currentCorpus;` is a **single-slot** retirement. A second load within one block frees a corpus the audio thread may still hold.
  - **Anti-pattern for rapid imports; do not port.**
  - Its loader is a `juce::Thread` subclass with `cancelLoad()` [VERIFIED: dsp/CorpusLoader.h:41-60].
  - Its callback guard is a `weak_ptr<int> lifetimeGuard`.

### 1.3 The collision with the frozen-cycle capture, and the fix

**Why Prism's quiescent rule is unsafe here.** A voice detects "bank pointer changed" at the start of the **next** block, then dereferences its `lastCfg.bank` (the OLD bank) to render the frozen cycle (ARCHITECTURE §5). Here is the failure:

```
host idle (entries == exits), voice active on Imported bank X (or held from the last block)
message: publish Y; retire X; sweep -> quiescent -> FREE X
audio  : next processBlock -> voice sees X != Y -> capture from X  => use-after-free
```

ARCHITECTURE's claim that "the reaper's '+2 generations / quiescent' rules then stay valid" [VERIFIED: ARCHITECTURE.md:118] is true for the +2 rule and **false for the quiescent rule**.

**Minimal amendment.** Keep REG-01 verbatim and add one audio-published pointer:

```cpp
// PluginProcessor.h
std::atomic<uint64_t> blockGeneration { 0 };            // blocks FINISHED  (REG-01, verbatim)
std::atomic<uint64_t> blockEntries    { 0 };            // blocks STARTED   (REG-01, verbatim)
// Amendment (frozen-cycle capture): the bank every ACTIVE voice holds as lastCfg.bank
// after the last finished block = that block's resolved pointer. The next block may
// dereference it once (frozen-cycle capture), so the reaper must never free it.
std::atomic<const WavetableBank*> audioHeldBank { nullptr };

struct RetiredBank { std::shared_ptr<const WavetableBank> bank; uint64_t retiredAt = 0; };
std::vector<RetiredBank> retiredBanks;                  // guarded by bankStateLock (never audio thread)
mutable juce::CriticalSection bankStateLock;            // owner ptr, retiredBanks, blob cache, import status
std::shared_ptr<const WavetableBank> importedOwner;     // under bankStateLock
std::atomic<const WavetableBank*> importedForAudio { nullptr };
```

```cpp
// PluginProcessor.cpp -- processBlock skeleton (2.4 parts only)
void OSimpleWavetableAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    blockEntries.fetch_add (1);                                   // seq_cst; FIRST, every path (REG-01)

    const int numSamples = buffer.getNumSamples();
    buffer.clear();
    if (numSamples <= 0 || buffer.getNumChannels() == 0)          // Stage 1 already returns here
    {
        blockGeneration.fetch_add (1);                            // keep the pair consistent (REG-01 WR-09)
        return;                                                   // audioHeldBank unchanged: voices unchanged
    }

    // Resolve ONCE per block (ARCHITECTURE §12, Processing Order 2)
    const int bankIdx = (int) bankParam->load();
    const WavetableBank* resolved = bankIdx < 5 ? builtIns->get (bankIdx)
                                                : importedForAudio.load();   // seq_cst; nullptr = silent
   #if OSIW_TEST_HOOKS
    if (resolved != nullptr && resolved->testReaped.load (std::memory_order_relaxed)) ++testDerefAfterReap;
   #endif
    blockCtx.bank = resolved;
    // ... 2.3 buffers, voice render (voices capture from lastCfg.bank on change) ...

    audioHeldBank.store (resolved);                               // seq_cst; BEFORE the exit increment
    blockGeneration.fetch_add (1);                                // LAST (REG-01)
}
```

```cpp
// Retire + sweep: REG-01 verbatim + ONE predicate. Any thread, caller holds bankStateLock.
void OSimpleWavetableAudioProcessor::retireBank (std::shared_ptr<const WavetableBank> b)
{
    if (b == nullptr) return;
    sweepRetiredBanks();                                          // producer sweep (REG-01)
    retiredBanks.push_back ({ std::move (b), blockGeneration.load() });   // stamp AFTER the publish store
}

void OSimpleWavetableAudioProcessor::sweepRetiredBanks()
{
    const auto exits    = blockGeneration.load();                 // exits FIRST (REG-01)
    const auto entries  = blockEntries.load();
    const bool quiescent = (entries == exits);
    const auto* held    = audioHeldBank.load();                   // read AFTER entries

    const auto unreachable = [exits, quiescent, held] (const RetiredBank& r)
    {
        return r.bank.get() != held                               // AMENDMENT: next block may capture from it
            && (quiescent || exits >= r.retiredAt + 2);           // REG-01 verbatim
    };
    retiredBanks.erase (std::remove_if (retiredBanks.begin(), retiredBanks.end(),
                                        [&] (const RetiredBank& r) { return unreachable (r); }),
                        retiredBanks.end());
}

void OSimpleWavetableAudioProcessor::publishImportedBank (std::shared_ptr<const WavetableBank> nb)  // holds lock
{
    auto old = std::move (importedOwner);
    importedOwner = std::move (nb);
    importedForAudio.store (importedOwner.get());                 // seq_cst publish
    retireBank (std::move (old));
    bankDisplayGen.fetch_add (1, std::memory_order_relaxed);      // Stage 3 bankUpdate seam
}
```

**Why the amendment is sufficient** (argument from the code above, [ASSUMED] until §7.3 passes):
- At the start of block N+1, every active voice's `lastCfg.bank` equals block N's `resolved`. Voices that started in N were seeded with it; voices active in N were updated to it. Idle voices are re-seeded at `startNote`.
- So the only bank that block N+1 can capture from without re-reading the atomic is `audioHeldBank`. Excluding it from **both** rules covers the quiescent hole.
- The +2 rule alone was already safe. The extra exclusion only delays the free by one block.

**Bound:** with the host idle, held = live + at most 1 (the audio-held bank). Rapid imports while idle free every intermediate bank immediately, because none of them is `held`. That satisfies the "held-bank count bounded" gate.

**Ordering:** use `seq_cst` (the default arguments above) for:
- `importedForAudio.store` and the stamp load
- `blockEntries` / `blockGeneration` increments and loads
- `audioHeldBank` store and load
- the audio-side pointer load

Prism's acq/rel is formally insufficient for the store→load pair "publish, then read exits". On AArch64, seq_cst loads and stores map to LDAR/STLR [ASSUMED], so the cost is negligible. Flag this as a deliberate hardening deviation from "verbatim".

**Timer:** sweep every 250 ms (ARCHITECTURE) and on every retire. The timer cannot run in the console harness, so the harness calls `sweepRetiredBanks()` (under the lock) through a test accessor.

**Alternative considered: hazard pointers.** Rejected. The amendment is a hazard pointer with one slot, and the "+2" rule already covers the load/publish race.

**Fallback (ARCHITECTURE Risk 1):** a preallocated 23 MB slot with "fade to silence, then overwrite". Keep it unbuilt unless §7.3 cannot be made green.

---

## 2. Global LFO (2.3)

### 2.1 Verified inputs
- Division table: "**Beats per cycle:** 16, 8, 4, 2, 1, 0.5, 0.25, 0.125, 3, 1.5, 0.75, 0.375, 4/3, 2/3, 1/3, 1/6" [VERIFIED: parameter-spec.md:99]
- Choice order: `"4 bars", "2 bars", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/2.", "1/4.", "1/8.", "1/16.", "1/2T", "1/4T", "1/8T", "1/16T"`, default index 2 [VERIFIED: Source/PluginProcessor.cpp:125-126]
- Shapes: `"Sine", "Triangle", "Saw", "Square", "S&H"`, default 1 [VERIFIED: :127]
- `lfo_sync`: `"Free", "Tempo"` [VERIFIED: :123]
- `lfo_rate`: `{ 0.01f, 20.0f, 0.0f, 0.3f }` default 0.5 [VERIFIED: :122]
- Do **not** copy Prism's `kDivBeats` triplet literals (`2.6667f, 1.3333f, ...`) [VERIFIED: O-Prism/Source/NoteDivisions.h:31-35]. They are rounded, so a 4-bar loop drifts. Use exact `4.0/3.0` etc. in double.
- Prism's tempo LFO is rate-only: BPM goes to Hz, with no PPQ lock [VERIFIED: O-Prism/Source/PluginProcessor.cpp:687-705, 812-818]. PPQ-lock references are O-GrainScatter `TempoTracker` (BPM ≤ 0 means "no position", 120 fallback) [VERIFIED: O-GrainScatter/Source/dsp/TempoTracker.h:41-83] and O-FreqPulse [VERIFIED: O-FreqPulse/Source/PluginProcessor.cpp:576-603].
- JUCE 8.0.15:
  - `AudioPlayHead::getPosition()` → `Optional<PositionInfo>` [VERIFIED: juce_AudioPlayHead.h:537]
  - `getBpm()` / `getPpqPosition()` → `Optional<double>` [VERIFIED: :313, :358]
  - `getIsPlaying()` → `bool` [VERIFIED: :376]
  - setters `setBpm`, `setPpqPosition`, `setIsPlaying` [VERIFIED: :316, :361, :379]
  - `AudioProcessor::setPlayHead` is virtual and `getPlayHead()` is public [VERIFIED: juce_audio_processors_headless/processors/juce_AudioProcessor.h:740, 1241]

### 2.2 Design
- One global phase in **double**. It is reset to 0 in `prepareToPlay` (for determinism), never by notes, and never on a sync change.
- Rendered **per sample** into `lfoBuf[maxBlock]`, preallocated in `prepareToPlay`.
- **Tempo + playing + valid PPQ and BPM:** the phase is computed **absolutely** per sample as `frac((ppq0 + i·dppq)/beats)`. This re-locks every block, absorbs loops and jumps, and has no accumulated drift.
- **Stopped or no playhead:** free-run from the current phase at `bpm/60/beats` Hz, where bpm = host BPM if finite and > 0, else 120.
- **S&H:** value = `hash(seed, cycleIndex)`, so it is position-addressable. In tempo mode `cycleIndex = floor(ppq/beats)`: a transport loop replays the same values and offline equals realtime. In free mode `cycleIndex` counts wraps since `prepareToPlay`. This meets the locked "deterministic, reseeded in prepareToPlay" requirement better than a sequential `juce::Random` (memory: `juce::Random` 48-bit LCG, seeds n/n+1 give near-identical first draws).
- Triangle starts at 0 going up (ARCHITECTURE §9).

```cpp
// Source/PositionLfo.h  -- ready to adapt
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>

class PositionLfo
{
public:
    enum Shape { Sine = 0, Triangle, Saw, Square, SampleHold };
    static constexpr double kDivBeats[16] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                              3.0, 1.5, 0.75, 0.375,
                                              4.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
    void setSeed (uint64_t s) noexcept { seed = s; }                       // test hook per render job

    void prepare (double sampleRate, int maxBlock)
    {
        fs = sampleRate > 0.0 && std::isfinite (sampleRate) ? sampleRate : 44100.0;
        buf.assign ((size_t) juce::jmax (1, maxBlock), 0.0f);
        phase = 0.0; cycleIndex = 0; shValue = draw (0);
    }

    struct Transport { bool valid = false; bool playing = false; double ppq = 0.0; double bpm = 120.0; };

    static Transport readTransport (juce::AudioPlayHead* ph) noexcept      // audio thread
    {
        Transport t;
        if (ph != nullptr)
            if (const auto pos = ph->getPosition())
            {
                const auto bpm = pos->getBpm();
                if (bpm.hasValue() && std::isfinite (*bpm) && *bpm > 0.0) { t.bpm = *bpm; t.valid = true; }
                const auto ppq = pos->getPpqPosition();
                t.playing = pos->getIsPlaying() && ppq.hasValue() && t.valid && std::isfinite (*ppq);
                if (t.playing) t.ppq = *ppq;
            }
        return t;                                                           // invalid -> 120 BPM free-run
    }

    // numSamples <= maxBlock (assert in processBlock; chunk if a host ever exceeds it)
    const float* render (int numSamples, Shape shape, bool tempoMode, double rateHz, int divIndex,
                         const Transport& t) noexcept
    {
        float* out = buf.data();
        if (tempoMode)
        {
            const double beats = kDivBeats[juce::jlimit (0, 15, divIndex)];
            const double bpm   = t.valid ? t.bpm : 120.0;
            if (t.playing)
            {
                const double dppq = bpm / (60.0 * fs);
                for (int i = 0; i < numSamples; ++i)
                {
                    const double cyc = (t.ppq + (double) i * dppq) / beats;   // ABSOLUTE: re-locks every block
                    const double fl  = std::floor (cyc);
                    advanceCycle ((int64_t) fl);
                    phase = cyc - fl;
                    out[i] = shapeAt (shape, phase);
                }
                phase += dppq / beats;  phase -= std::floor (phase);         // continue seamlessly if transport stops
                return out;
            }
            rateHz = bpm / 60.0 / beats;                                     // stopped / Standalone: free-run
        }
        const double inc = juce::jlimit (0.0, 0.5, rateHz / fs);
        for (int i = 0; i < numSamples; ++i)
        {
            out[i] = shapeAt (shape, phase);
            phase += inc;
            if (phase >= 1.0) { phase -= std::floor (phase); advanceCycle (cycleIndex + 1); }
        }
        return out;
    }

    float lastValue (int n) const noexcept { return n > 0 ? buf[(size_t) n - 1] : 0.0f; }   // display atomic

private:
    void advanceCycle (int64_t ci) noexcept { if (ci != cycleIndex) { cycleIndex = ci; shValue = draw (ci); } }

    float shapeAt (Shape s, double p) const noexcept
    {
        switch (s)
        {
            case Sine:       return (float) std::sin (juce::MathConstants<double>::twoPi * p);
            case Triangle:   return (float) (p < 0.25 ? 4.0 * p : p < 0.75 ? 2.0 - 4.0 * p : 4.0 * p - 4.0);
            case Saw:        return (float) (2.0 * p - 1.0);
            case Square:     return p < 0.5 ? 1.0f : -1.0f;
            case SampleHold: return shValue;
        }
        return 0.0f;
    }

    float draw (int64_t ci) const noexcept                                  // splitmix64 -> uniform [-1, 1)
    {
        uint64_t z = seed ^ ((uint64_t) ci * 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        z ^= z >> 31;
        return (float) ((double) (z >> 40) * (1.0 / 16777216.0) * 2.0 - 1.0);
    }

    double fs = 44100.0, phase = 0.0;
    int64_t cycleIndex = 0;
    float shValue = 0.0f;
    uint64_t seed = 0x4F53695753482D31ull;                                  // constant: byte-stable offline renders
    std::vector<float> buf;
};
```

### 2.3 Position assembly (global part, per block)
```cpp
// prepareToPlay: SEED every smoother from the live param (never fade in from 0; memory gain_ramp_without_seeding)
knobSmooth.reset (sr, 0.02);   knobSmooth.setCurrentAndTargetValue (finiteOr (positionParam->load(), 0.0f));
depthSmooth.reset (sr, 0.02);  depthSmooth.setCurrentAndTargetValue (finiteOr (lfoDepthParam->load(), 0.0f));
amtSmooth.reset (sr, 0.02);    amtSmooth.setCurrentAndTargetValue (finiteOr (envAmountParam->load(), 0.0f));
knobBuf.assign (maxBlock, 0); depthBuf.assign (maxBlock, 0); amtBuf.assign (maxBlock, 0);
lfo.prepare (sr, maxBlock);
// processBlock:
knobSmooth.setTargetValue (finiteOr (positionParam->load(), 0.0f));   // SmoothedValue reset() does NOT clear NaN (memory)
for (int i = 0; i < n; ++i) { knobBuf[i] = knobSmooth.getNextValue(); depthBuf[i] = depthSmooth.getNextValue(); amtBuf[i] = amtSmooth.getNextValue(); }
const float* lfoBuf = lfo.render (n, shape, tempo, rate, div, PositionLfo::readTransport (getPlayHead()));
```
- `juce::SmoothedValue::reset(double, double)` calls `setCurrentAndTargetValue(target)` [VERIFIED: juce_SmoothedValue.h:265-277]. So seeding with `setCurrentAndTargetValue` **after** `reset` is required.
- **Smoothing `lfo_depth` and `env_amount` is my recommendation, not in ARCHITECTURE.** Block-rate automation of depth/amount would otherwise step every block, softened only by the 2 ms pole, which gives a mild zipper. The cost is 2 global smoothers. Mark it as Claude's discretion in PLAN.

### 2.4 Per-voice part (2.3)
```cpp
// prepareToPlay (voice): modEnv.setSampleRate (sr); alpha = 1.0f - std::exp (-1.0f / (0.002f * (float) sr));
// startNote: needsSeed = true; modEnv.noteOn();      (legato transition: neither)
// per-block, dirty-checked (memory pattern_adsr_setparameters_per_block_kills_release):
if (menv != lastMenv) { modEnv.setParameters (menv); lastMenv = menv; }

for (int i = 0; i < n; ++i)
{
    const int k   = startSample + i;                                     // ABSOLUTE in-block index
    const float env = modEnv.getNextSample();                            // advanced only while the voice is active
    const float raw = juce::jlimit (0.0f, 1.0f,
                        ctx->knobPos[k] + ctx->lfoDepth[k] * 0.5f * ctx->lfo[k] + ctx->envAmount[k] * env);  // DSP-05 clamp
    if (needsSeed) { effPos = raw; needsSeed = false; }                  // seeded at first sample: no glide from old note
    effPos = ctx->interp ? effPos + alpha * (raw - effPos) : raw;        // Off: track raw, so an Off->On toggle has no stale glide
    // Interp Off: latchedFrame = round(raw*(N-1)) updated at phase wrap + note-on (2.2)
    ...
}
```
- Voice lifetime is gated on the **amp** env only. When the amp env ends, call `clearCurrentNote()`, even mid mod-env release (FUNC-06).
- `startNote` is called by `Synthesiser::startVoice` after it has already set `currentlyPlayingNote` (memory critical_synthesiser_startvoice). Use your own `needsSeed` flag, not `getCurrentlyPlayingNote()`.

### 2.5 Synthetic AudioPlayHead (FUNC-05 harness)
```cpp
struct TestPlayHead final : juce::AudioPlayHead
{
    double bpm = 120.0, ppq = 0.0; bool playing = true, hasPpq = true, hasBpm = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        if (hasBpm) p.setBpm (bpm);
        if (hasPpq) p.setPpqPosition (ppq);
        p.setIsPlaying (playing);
        return p;
    }
    void advance (int n, double fs) { if (playing) ppq += (double) n * bpm / (60.0 * fs); }
};
// proc.setPlayHead (&head);  per block: proc.processBlock (buf, midi); head.advance (n, fs);
```

### 2.6 Determinism notes
- **Free mode with static params:** the render must be bit-identical across block sizes 32/512/4096/random. That needs the per-sample LFO, mod env and smoothers. It also needs `synth.setMinimumRenderingSubdivisionSize (1, true)`. JUCE's default of 32 non-strict handles events that fall < 32 samples into a sub-block *early*, which depends on the partition [VERIFIED: juce_Synthesiser.cpp:217].
- **Tempo mode** is not bitwise across partitions (`ppq0 + i·d` rounding), so gate it at 1e-9.
- **Render jobs** that are compared with each other get `setSeed (splitmix64 (runSeed, jobIndex))` through a test hook (memory shared_seed_common_random_numbers).

---

## 3. Mod ADSR, the smoother and the click detector (2.3)

### 3.1 Mod envelope
`juce::ADSR` per voice, with `setSampleRate` set before `setParameters` in prepare. Dirty-check the parameters. Contribution = `env_amount · env`, clamped with the others (DSP-05).

### 3.2 2 ms smoother
- The locked spec is a one-pole with α = 1 − e^(−1/(τ·fs)), τ = 2 ms, seeded at the first sample after note-on. Float state is fine (it settles in about 10 ms).
- Reseed it on **Interp Off→On** (handled by tracking raw while Off).
- Do not reseed it on legato.

### 3.3 Risk: one-pole velocity kink
- A one-pole responding to a square/S&H edge starts at full velocity Δ/τ. For Sine→Saw at 100% depth that is about 0.0104 pos/sample × 31 frames × up to ~0.5 output change per frame step. That gives a slope kink up to ~0.16/sample, against a saw-32 plateau second difference of ~0.06 at A2/48k [ASSUMED estimate, from the frame formulas].
- Two one-poles in series start with zero velocity (memory pattern_one_pole_delay_glide: −40 dB vs −66 dB in O-SimpleReverb).
- **Plan:** implement `PositionSmoother` with a compile-time `kPoles = 1`. Run the Square/S&H click gate with a no-smoother negative control. If the one-pole fails, switch to `kPoles = 2` (τ = 1 ms each). This matches ARCHITECTURE Risk 4's spirit of pre-authorized fallbacks.

```cpp
struct PositionSmoother
{
    static constexpr int kPoles = 1;          // flip to 2 only if the 2.3 click gate fails (documented fallback)
    float a = 0.0f, s1 = 0.0f, s2 = 0.0f;
    void prepare (double fs) { const double tau = kPoles == 1 ? 0.002 : 0.001; a = (float) (1.0 - std::exp (-1.0 / (tau * fs))); }
    void seed (float v) noexcept { s1 = s2 = v; }
    float process (float x) noexcept { s1 += a * (x - s1); if constexpr (kPoles == 1) return s1; s2 += a * (s1 - s2); return s2; }
};
```

### 3.4 Click detector (shared by 2.3 and 2.4)
Per memory pattern_zipper_gate_absolute_step: an absolute threshold is a gain-staging statement. Gate the **excess** instead.

```cpp
// y: mono render; t: event sample; xf: crossfade/smoother settle length in samples
static float maxAbsD2 (const float* y, int a, int b)     // max |y[n] - 2y[n-1] + y[n-2]| over [a,b)
{ float m = 0; for (int n = juce::jmax (2, a); n < b; ++n) m = juce::jmax (m, std::abs (y[n] - 2*y[n-1] + y[n-2])); return m; }

struct ClickVerdict { float ratio, event, plateau; };
static ClickVerdict clickRatio (const std::vector<float>& y, int t, int xf, double fs)
{
    const int win = xf + (int) (0.002 * fs), plat = (int) (0.020 * fs), gap = (int) (0.005 * fs);
    const float ev  = maxAbsD2 (y.data(), t - 2, t + win);
    const float pre = maxAbsD2 (y.data(), t - gap - plat, t - gap);
    const float post = maxAbsD2 (y.data(), t + win + gap, t + win + gap + plat);
    const float ref = juce::jmax (pre, post, 1.0e-9f);
    return { ev / ref, ev, ref };
}
// PASS: ratio <= 1.5 at every event.  Print absolute values too.
// Liveness (memory gate_stimulus_below_threshold): |A(t) - B(t)| >= 0.25 at the event (old vs new waveform),
// else the stimulus cannot click and the gate is vacuous -> FAIL the gate itself.
// Negative control: test hook xfadeLenOverride = 0 (2.4) / smoother bypass (2.3) MUST give ratio >= 4.
```

**Stimuli that make the 2nd-difference test discriminate.** Use low notes (A1/A2) and smooth frames with a big instantaneous difference: Sine→Saw pos 0 (sine) ↔ Formant pos 0 (vowel A).

At high notes or on bright frames the waveform's own second difference is too large for the test to see anything. That covers bandlimit toggles, level changes on a bend, and Interp toggles with small frame differences. For those, use the **exactness gate** in §7.1 instead.

---

## 4. Frozen-cycle crossfader and per-block bank resolve (2.4)

- **Triggers:** bank pointer, mip level, `interp`, `bandlimit` [VERIFIED: ARCHITECTURE.md:116].
- **Not crossfaded:** Interp-Off frame steps and bit-depth changes [VERIFIED: :119].
- **Ping-pong buffers:** 2 × 2049 floats per voice, preallocated, guard sample `[2048] = [0]`.
- **Length:** `xfadeLen = round(0.005·fs)`, computed in `prepareToPlay`.
- **Fold rule:** on a trigger during a running fade, capture the **currently heard mixed cycle**: `(1−w)·frozenActive(i) + w·live_oldcfg(i)`. Write it into the free buffer, swap, and restart with `w = 0`. The heard output is then continuous by construction. Nothing is dropped (memory jump_during_crossfade_drops_outgoing_head).
- **Running length:** the fade always uses `xfLenActive`, set only on the branch that resets the counter (memory per_jump_fade_needs_running_length_member). Here every trigger resets the counter after folding, so this reduces to "set `xfLenActive = xfadeLen` at each restart". Keep the member anyway for the test override.
- **Capture is only legal from `lastCfg.bank`.** That is the bank §1.3 protects. Never capture from a pointer that was not this voice's config in the previous block.

```cpp
struct ReadCfg { const WavetableBank* bank = nullptr; int level = 0; bool interp = true, bandlimit = true;
                 bool operator!= (const ReadCfg& o) const noexcept
                 { return bank != o.bank || level != o.level || interp != o.interp || bandlimit != o.bandlimit; } };

void WtVoice::checkConfig (const ReadCfg& now) noexcept          // per block + after each pitch-wheel sub-block
{
    if (! (now != cfg)) return;
    float* dst = frozen[1 - active].data();
    const float w = xfActive ? (float) xfPos / (float) xfLenActive : 1.0f;
    for (int i = 0; i < 2048; ++i)
    {
        const double ph = (double) i / 2048.0;
        const float liveOld = cfg.bank != nullptr ? readCycle (*cfg.bank, cfg.level, framePosForCapture (), cfg.interp, latchedFrame, ph) : 0.0f;
        dst[i] = xfActive ? (1.0f - w) * frozen[active][(size_t) i] + w * liveOld : liveOld;   // FOLD, never drop
    }
    dst[2048] = dst[0];
    active = 1 - active;  xfActive = true;  xfPos = 0;  xfLenActive = juce::jmax (1, xfadeLen);  // test hook may override
    cfg = now;                                                    // old bank is never dereferenced again by this voice
}
// per sample:  s = live(cfg); if (xfActive) { const float w = (float) xfPos / xfLenActive;
//                 s = (1-w) * lerpRead (frozen[active], phase) + w * s; if (++xfPos >= xfLenActive) xfActive = false; }
```

- **Imported → empty (nullptr):** the live read is 0, so it fades to silence in 5 ms. **Empty → import:** the frozen cycle is zeros, so it fades in. Both satisfy ARCHITECTURE "importing during a held note fades the new bank in" [VERIFIED: :393].
- **Level storm (bend jitter at an octave boundary):** add downward-only hysteresis.
  - Going **up** in L (fewer harmonics) switches immediately. That is alias safety.
  - Going **down** switches only when `f < boundary · 2^(−1/24)`.
  - This stays strictly alias-free, because the higher level always has fewer harmonics [ASSUMED engineering; 2.2 owns level selection, so pass it to that researcher].
- **Cost:** 2048 lerps × 16 voices on one bank switch is about 200 k flops in one block [VERIFIED estimate: ARCHITECTURE.md:432]. Acceptable.
- **Bank resolve:** once per block, before any voice renders (§1.3 skeleton). The imported bank's `numFrames` (1..256) is read from the **resolved bank** every block. Never cache it from the param or the blob.

---

## 5. Importer (§A8) on a worker thread (2.4)

### 5.1 Verified JUCE APIs
- `AudioFormatManager::registerBasicFormats()`, `createReaderFor (const File&)` / `createReaderFor (std::unique_ptr<InputStream>)` return a raw `AudioFormatReader*` [VERIFIED: juce_AudioFormatManager.h:80,135,149].
- `read (float* const*, int numDestChannels, int64 start, int n)` [VERIFIED: juce_AudioFormatReader.h:98-99]. Use this for the N-channel mean. Note that the `AudioBuffer` overload only takes left/right flags [VERIFIED: :156-161]. Prism's `jlimit(1, 2, numChannels)` therefore only averages two channels [VERIFIED: O-Prism/Source/dsp/WavetableImporter.cpp:76-78, 134-137]. ARCHITECTURE says mean of **all** channels.
- `read (int* const*, …, bool fillLeftoverChannelsWithCopies)`: full-range int32 "regardless of the source's bit-depth" [VERIFIED: :101-143]. Use this for the bit-exact FLAC reload.
- `juce::dsp::FFT`: `performRealOnlyForwardTransform` needs 2·N floats. Bin k is at [2k, 2k+1], so DC is [0,1] and Nyquist is [N, N+1] [VERIFIED: juce_FFT.h:86-108; Prism zeroes exactly these at WavetableImporter.cpp:173-179].
- `ThreadPool (ThreadPoolOptions{}.withThreadName(...).withNumberOfThreads(1))`, `addJob (std::function<void()>)`, `removeAllJobs (bool, int)` [VERIFIED: juce_ThreadPool.h:166-191, 223, 301, 335].

### 5.2 Pipeline (pure function, testable without a processor)
```cpp
// Source/WavetableImporter.h
struct ImportedPcm { juce::String filename; int numFrames = 0; std::vector<int16_t> pcm; };   // numFrames*2048
enum class ImportError { none, unreadable, tooShort, tooLarge, cancelled };
struct ImportResult { ImportError error = ImportError::none; ImportedPcm pcm; };

namespace WavetableImporter
{
    constexpr int kFrame = 2048, kMaxFrames = 256, kMaxChannelsAveraged = 64;
    constexpr float kMaxBoost = 15.848932f;                        // 10^(24/20), +24 dB cap

    inline ImportResult decode (juce::AudioFormatReader& r, const juce::String& filename,
                                const std::function<bool()>& cancelled)
    {
        ImportResult res;
        if (! std::isfinite (r.sampleRate) || r.sampleRate <= 0.0 || r.numChannels == 0 || r.lengthInSamples <= 0)
            { res.error = ImportError::unreadable; return res; }
        const juce::int64 n64 = juce::jmin<juce::int64> (r.lengthInSamples, (juce::int64) kMaxFrames * kFrame); // never trust header
        if (n64 < kFrame) { res.error = ImportError::tooShort; return res; }
        const int frames = (int) (n64 / kFrame);                   // tail dropped; <= 256
        const int n = frames * kFrame;
        const int nc = (int) juce::jmin<unsigned int> (r.numChannels, (unsigned) kMaxChannelsAveraged);

        std::vector<float> mono ((size_t) n, 0.0f);
        juce::AudioBuffer<float> chunk (nc, 8192);
        for (int pos = 0; pos < n; pos += 8192)
        {
            if (cancelled()) { res.error = ImportError::cancelled; return res; }
            const int len = juce::jmin (8192, n - pos);
            if (! r.read (chunk.getArrayOfWritePointers(), nc, pos, len)) { res.error = ImportError::unreadable; return res; }
            for (int c = 0; c < nc; ++c)
            {
                const float* s = chunk.getReadPointer (c);
                for (int i = 0; i < len; ++i) mono[(size_t) (pos + i)] += (std::isfinite (s[i]) ? s[i] : 0.0f);   // float WAV NaN/inf scrub
            }
        }
        const float invC = 1.0f / (float) nc;

        juce::dsp::FFT fft (11);
        std::vector<float> work (2 * kFrame);
        res.pcm.filename = filename; res.pcm.numFrames = frames; res.pcm.pcm.resize ((size_t) n);
        for (int f = 0; f < frames; ++f)
        {
            std::fill (work.begin(), work.end(), 0.0f);
            for (int i = 0; i < kFrame; ++i) work[(size_t) i] = mono[(size_t) (f * kFrame + i)] * invC;
            fft.performRealOnlyForwardTransform (work.data());
            work[0] = work[1] = 0.0f;  work[kFrame] = work[kFrame + 1] = 0.0f;       // DC + Nyquist (DSP-04)
            fft.performRealOnlyInverseTransform (work.data());                      // scaled 1/N internally
            float peak = 0.0f; for (int i = 0; i < kFrame; ++i) peak = juce::jmax (peak, std::abs (work[(size_t) i]));
            const float g = peak > 0.0f ? juce::jmin (1.0f / peak, kMaxBoost) : 0.0f;   // p = 0 -> stays silent
            for (int i = 0; i < kFrame; ++i)
                res.pcm.pcm[(size_t) (f * kFrame + i)] =
                    (int16_t) std::lround (juce::jlimit (-1.0f, 1.0f, work[(size_t) i] * g) * 32767.0f); // canonical int16
        }
        return res;
    }

    inline float toFloat (int16_t q) noexcept { return (float) q / 32767.0f; }      // the ONLY int16->float mapping

    // ONE builder used by import, restore and the FUNC-04 gate.
    inline std::shared_ptr<const WavetableBank> buildImportedBank (const ImportedPcm& p)
    {
        std::vector<float> level0 (p.pcm.size());
        for (size_t i = 0; i < p.pcm.size(); ++i) level0[i] = toFloat (p.pcm[i]);
        return std::shared_ptr<const WavetableBank> (MipmapBuilder::buildFromLevel0 (level0.data(), p.numFrames, p.filename));
    }
}
```
- Requantizing to int16 re-introduces DC of at most ½ LSB, about −96 dBFS. That is within DSP-04's ≤ −80 dB [ASSUMED arithmetic]. The mip builder zeroes DC again on every level anyway.
- **Error codes (Stage 3 contract):** `tooShort`, `unreadable`, `tooLarge` [VERIFIED: mockups/v1-integration-checklist.md:59].
  - Long files are **not** an error. They truncate to 256 frames [VERIFIED: ROADMAP.md:105].
  - Use `tooLarge` only for `importFromMemory` bytes above 96 MB, matching the page's `DROP_MAX_BYTES` [VERIFIED: checklist:43].
  - `cancelled` is internal and never surfaced.

### 5.3 Processor import API, worker, generation, status
```cpp
public:
    struct ImportStatus { enum class State { idle, busy, done, error }; State state = State::idle;
                          juce::String filename; int frames = 0; juce::String error; };
    bool importFromFile   (const juce::File& f);                     // any thread; false = refused to start
    bool importFromMemory (const juce::String& name, juce::MemoryBlock&& bytes);
    ImportStatus getImportStatus() const        { const juce::ScopedLock sl (bankStateLock); return importStatus; }
    juce::uint32 getImportStatusVersion() const { return importStatusVersion.load (std::memory_order_acquire); }
    juce::uint32 getBankDisplayGeneration() const { return bankDisplayGen.load (std::memory_order_relaxed); }
private:
    std::atomic<juce::uint32> importGen { 0 }, importStatusVersion { 0 }, bankDisplayGen { 0 };
    ImportStatus importStatus;                                      // under bankStateLock
    // DECLARE LAST so it is destroyed FIRST; the destructor also calls removeAllJobs (true, 10000).
    juce::ThreadPool importPool { juce::ThreadPoolOptions{}.withThreadName ("OSiW import").withNumberOfThreads (1) };
```
```cpp
bool OSimpleWavetableAudioProcessor::importFromFile (const juce::File& file)
{
    if (! file.existsAsFile()) { setImportStatus (ImportStatus::State::error, file.getFileName(), 0, "unreadable"); return false; }
    const auto gen = ++importGen;                                   // supersedes any in-flight job
    setImportStatus (ImportStatus::State::busy, file.getFileName(), 0, {});
    importPool.addJob ([this, file, gen]
    {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (file));
        runImportJob (r.get(), sanitiseName (file.getFileName()), gen);
    });
    return true;
}

void OSimpleWavetableAudioProcessor::runImportJob (juce::AudioFormatReader* r, const juce::String& name, juce::uint32 gen)
{
    auto stale = [this, gen] { return importGen.load() != gen; };
    if (r == nullptr) { if (! stale()) setImportStatus (ImportStatus::State::error, name, 0, "unreadable"); return; }
    auto res = WavetableImporter::decode (*r, name, stale);
    if (res.error == ImportError::cancelled || stale()) return;
    if (res.error != ImportError::none) { setImportStatus (ImportStatus::State::error, name, 0, res.error == ImportError::tooShort ? "tooShort" : "unreadable"); return; }

    auto blob = encodeBlob (res.pcm);                               // flac16, or pcm16gz if the FLAC writer refuses (§6.1)
    auto bank = WavetableImporter::buildImportedBank (res.pcm);     // ~23 MB, on the worker
    {
        const juce::ScopedLock sl (bankStateLock);
        if (importGen.load() != gen) return;                        // re-check under the lock; the unpublished bank dies here (worker thread)
        publishImportedBank (std::move (bank));                     // §1.3: atomic store + retire + producer sweep
        cachedBlob = std::move (blob);
        importStatus = { ImportStatus::State::done, name, res.pcm.numFrames, {} };
        importStatusVersion.fetch_add (1, std::memory_order_release);
    }
    pendingAutoSelect.store (true);
    triggerAsyncUpdate();                                           // handleAsyncUpdate (message thread): set bank = Imported
}

void OSimpleWavetableAudioProcessor::handleAsyncUpdate()             // juce::AsyncUpdater
{
    if (pendingAutoSelect.exchange (false))
        if (auto* p = parameters.getParameter (OSimpleWavetable::ParamIDs::bank))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (5.0f));     // Imported is index 5 [VERIFIED: PluginProcessor.cpp:106-113]
            p->endChangeGesture();
        }
}
```
- **Publish BEFORE selecting the param.** If the order is reversed, the audio thread sees Imported with the old/null bank, which gives a fade to silence and then a second fade.
- **Status is polled, not pushed.** The editor's 30 Hz timer compares `getImportStatusVersion()` and emits `importStatus`. Events must not be emitted from the worker or host threads [VERIFIED rationale: O-simpleGrain/Source/PluginProcessor.h:218-222 "Polling because decodeAndPublish can run on host-controlled threads"; template: v1-PluginEditor.cpp:537-547].
- `importFromMemory (name, bytes)`:
  - Check the size cap (> 96 MB gives `tooLarge`).
  - The job reads from `std::make_unique<juce::MemoryInputStream> (std::move-owned block, false)`.
  - **Sanitise `name`.** It comes from JS and is persisted into state XML and shown in the UI: take the basename, strip control characters, limit to ≤ 128 chars.
  - The bytes are base64-decoded by the editor with `juce::Base64::convertFromBase64` [VERIFIED: O-simpleGrain/Source/PluginProcessor.cpp:709-714; drop-streaming module.yaml warning].
- **The webview-drop-streaming module** is header-only C++ (`cpp/WebViewDropStreaming.h`, `DropSessionGuard.h`, `SessionManager` with 4 native functions and a per-plugin `tempDirPrefix`) [VERIFIED: modules/core/webview-drop-streaming/module.yaml, README.md]. It is a **Stage 3** concern. The editor template uses a simpler single-shot `importDroppedAudio` [VERIFIED: v1-PluginEditor.cpp:333-358]. 2.4 needs only `importFromMemory`.
- **Rapid imports:** each request bumps `importGen`. Superseded jobs exit at the next chunk check (cancel), or discard their bank before publishing. Queued jobs are cheap closures. The retired count stays bounded by §1.3 (gate in §7).

---

## 6. IMPORTED_BANK persistence (2.4)

### 6.1 FLAC16 in memory: verified, lossless
- **API (JUCE 8.0.15).** `FlacAudioFormat::createWriterFor (std::unique_ptr<OutputStream>&, const AudioFormatWriterOptions&)` [VERIFIED: juce_FlacAudioFormat.h:66-67]:
  - It returns nullptr unless the bit depth is in `{16, 24}` [VERIFIED: .cpp:581-584, 606].
  - It takes ownership via `std::exchange(streamToWriteTo, {}).release()` [VERIFIED: :608].
  - The sample rate is not validated against `getPossibleSampleRates` [VERIFIED: :603-616], so a nominal 48000 is fine.
  - The quality index maps to `FLAC__stream_encoder_set_compression_level` only if > 0 [VERIFIED: :417-418].
- **Seek-back.** `~FlacWriter` calls `FLAC__stream_encoder_finish`, whose metadata callback seeks to `streamStartPos + 4` to rewrite STREAMINFO. It jasserts if the stream cannot seek [VERIFIED: :434-446, 519-526]. `MemoryOutputStream::setPosition` allows seeking backwards within the written size [VERIFIED: juce_MemoryOutputStream.cpp:171-181]. So **the writer must be destroyed before the block is used**, and the stream must not be appended to.
- **JUCE quirk:** if `!writer->ok`, `createWriterFor` returns nullptr after the stream was already released into the writer. The writer's destructor nulls `output`, so the stream leaks [VERIFIED: :439-443, 613-614]. This is a harmless one-off leak on failure, but do not loop retries.
- **The reader shifts samples to full-range int32:** `dest[j] = src[j] << bitsToShift` [VERIFIED: :327-341], so `>> 16` recovers int16 exactly.
- **[PROBE] run this session.** The scratch program `research-probes/flacprobe/probe.cpp` was built against `/Users/taylorbrook/JUCE/modules` (8.0.15) and encoded mono 16-bit at 48000 into a `MemoryOutputStream`. Then it did base64 (`juce::Base64`) → `AudioFormatManager` reader from a `MemoryInputStream` → `int*` read → `>> 16`:
  ```
  JUCE v8.0.15
  noise256: format=FLAC file rate=48000 ch=1 bits=16 len=524288 (want 524288) read=1 mismatches=0 flacBytes=1049814 (100.1% of pcm) b64chars=1399752
  sine256:  ... len=524288 ... mismatches=0 flacBytes=174609 (16.7% of pcm) b64chars=232812
  edges3:   ... len=6144 ... mismatches=0   (-32768 / 32767 / 0 pattern)
  sine1:    ... len=2048 ... mismatches=0
  pcm16gz: n=524288 mism=0 exhaustedAfter=1 bytes=1048902 (100.0%)
  ALL PASS
  ```
  - Worst case (white noise, 256 frames) is a 1.05 MB FLAC and **1.40 M base64 chars**. ARCHITECTURE's "0.6–1.0 MB" estimate is low for noisy material. Cap the accepted `data` length at 4 M chars.
  - Musical content compresses to about 17%.

```cpp
// Encode (worker). Quality 5 explicit. Writes left-justified ints: exact, no float conversion.
static juce::String encodeFlac16 (const std::vector<int16_t>& pcm)   // empty = failed -> caller falls back to pcm16gz
{
    juce::MemoryBlock mb;
    {
        juce::FlacAudioFormat flac;
        std::unique_ptr<juce::OutputStream> os = std::make_unique<juce::MemoryOutputStream> (mb, false);
        auto w = flac.createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (48000.0)
                                               .withNumChannels (1).withBitsPerSample (16).withQualityOptionIndex (5));
        if (w == nullptr) return {};
        std::vector<int> wide (pcm.size());
        for (size_t i = 0; i < pcm.size(); ++i) wide[i] = (int) pcm[i] * 65536;
        const int* ch[] = { wide.data(), nullptr };
        if (! w->write (ch, (int) wide.size())) return {};
    }                                                                 // writer (and stream) destroyed HERE -> STREAMINFO final
    return juce::Base64::toBase64 (mb.getData(), mb.getSize());
}

// Decode (restore). Validates everything against the declared frame count.
static bool decodeFlac16 (const juce::MemoryBlock& bytes, int numFrames, std::vector<int16_t>& out)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (std::make_unique<juce::MemoryInputStream> (bytes, false)));
    const juce::int64 want = (juce::int64) numFrames * 2048;
    if (r == nullptr || r->getFormatName() != "FLAC file" || r->numChannels != 1 || r->bitsPerSample != 16 || r->lengthInSamples != want)
        return false;
    std::vector<int> wide ((size_t) want);
    int* dst[] = { wide.data() };
    if (! r->read (dst, 1, 0, (int) want, false)) return false;
    out.resize ((size_t) want);
    for (size_t i = 0; i < out.size(); ++i) out[i] = (int16_t) (wide[i] >> 16);
    return true;
}

// pcm16gz fallback: gzip-wrapped (honest name), little-endian int16. Exact-length read = zip-bomb safe.
static juce::String encodePcm16Gz (const std::vector<int16_t>& pcm)
{
    juce::MemoryBlock mb;
    {
        juce::MemoryOutputStream mos (mb, false);
        juce::GZIPCompressorOutputStream z (mos, 9, juce::GZIPCompressorOutputStream::windowBitsGZIP);
        for (auto s : pcm) z.writeShort (s);
        z.flush();
    }
    return juce::Base64::toBase64 (mb.getData(), mb.getSize());
}
static bool decodePcm16Gz (const juce::MemoryBlock& bytes, int numFrames, std::vector<int16_t>& out)
{
    juce::MemoryInputStream mis (bytes, false);
    juce::GZIPDecompressorInputStream unz (&mis, false, juce::GZIPDecompressorInputStream::gzipFormat);
    out.resize ((size_t) numFrames * 2048);
    for (auto& s : out) { if (unz.isExhausted()) return false; s = unz.readShort(); }
    return unz.isExhausted();                                         // trailing data = reject
}
```
- **GZIP API:** `GZIPCompressorOutputStream (OutputStream&, int compressionLevel = -1, int windowBits = 0)` with `windowBitsGZIP = 15 + 16`; `GZIPDecompressorInputStream (InputStream*, bool, Format = zlibFormat, int64 = -1)` with `gzipFormat` [VERIFIED: juce_GZIPCompressorOutputStream.h:64-66,104-107; juce_GZIPDecompressorInputStream.h:53-74].
- The probe used the zlib default and round-tripped 524288 samples exactly [PROBE]. The gzip variant is the same API with the format flag.
- **Determinism note:** FLAC *encoded bytes* may differ across CPUs or builds. The *decoded* PCM cannot [ASSUMED: lossless codec]. Gates compare decoded PCM and bank floats, never blob bytes, except for the "save after restore is verbatim" gate (§6.3).

### 6.2 State child (exact attributes)
```
<IMPORTED_BANK version="1" filename="loop.wav" numFrames="215" encoding="flac16" data="ZkxhQwAAACIQ..."/>
```
[VERIFIED schema: ARCHITECTURE.md:243; parameter-spec.md:212]. Every attribute returns from XML as a **string** var. Gate each one on `isVoid()` and parse with `.toString().getIntValue()` [memory critical_valuetree_xml_roundtrip_loses_type; Stage 1 comment VERIFIED: Source/PluginProcessor.cpp:277-280].

### 6.3 Interplay with the Stage 1 strip-on-load stub
- **Keep the existing structure unchanged:**
  - `getStateInformation` strips then calls `writeImportedBank(state)` [VERIFIED: :230-239].
  - `setStateInformation` copies the first child, strips **all** copies before `replaceState`, then calls `restoreImportedBank(bankChild)` [VERIFIED: :241-266].
  - Stage 2.4 only fills the two seams. `writeImportedBank` is `const`, so `bankStateLock` must be `mutable`.

```cpp
struct CachedBlob { juce::String filename, encoding, data; int numFrames = 0;
                    juce::ValueTree passthrough; };                   // unknown version/encoding: re-emit verbatim

void OSimpleWavetableAudioProcessor::writeImportedBank (juce::ValueTree& state) const
{
    const juce::ScopedLock sl (bankStateLock);                        // never taken on the audio thread
    if (cachedBlob.passthrough.isValid()) { state.appendChild (cachedBlob.passthrough.createCopy(), nullptr); return; }
    if (cachedBlob.data.isEmpty()) return;                            // no import -> no child (pre-import sessions unchanged)
    juce::ValueTree c (kImportedBankTag);
    c.setProperty ("version", 1, nullptr);
    c.setProperty ("filename", cachedBlob.filename, nullptr);
    c.setProperty ("numFrames", cachedBlob.numFrames, nullptr);
    c.setProperty ("encoding", cachedBlob.encoding, nullptr);
    c.setProperty ("data", cachedBlob.data, nullptr);                 // EXACTLY one child, cached string, no work here
    state.appendChild (c, nullptr);
}

void OSimpleWavetableAudioProcessor::restoreImportedBank (const juce::ValueTree& child)   // any non-audio thread
{
    ++importGen;                                                      // supersede any in-flight import (restore wins)
    auto clearToEmpty = [this] (const juce::String& err)
    {
        const juce::ScopedLock sl (bankStateLock);
        publishImportedBank (nullptr);                                // RETIRE, never free here (Stage 1 seam note)
        cachedBlob = {};
        importStatus = err.isEmpty() ? ImportStatus{} : ImportStatus { ImportStatus::State::error, {}, 0, err };
        importStatusVersion.fetch_add (1, std::memory_order_release);
    };
    if (! child.isValid()) { clearToEmpty ({}); return; }             // absent -> empty Imported (ARCHITECTURE §State)

    auto str = [&child] (const char* k) { const juce::var v = child.getProperty (k); return v.isVoid() ? juce::String() : v.toString(); };
    const int version = str ("version").getIntValue();
    const auto enc    = str ("encoding");
    if (version > 1 || (version == 1 && enc != "flac16" && enc != "pcm16gz"))
    {                                                                 // FORWARD COMPAT: keep it, don't destroy a newer save
        const juce::ScopedLock sl (bankStateLock);
        publishImportedBank (nullptr); cachedBlob = {}; cachedBlob.passthrough = child.createCopy();
        importStatus = { ImportStatus::State::error, str ("filename"), 0, "unsupported" };
        importStatusVersion.fetch_add (1, std::memory_order_release);
        return;
    }
    const int frames = str ("numFrames").getIntValue();
    const auto data  = str ("data");
    if (version != 1 || frames < 1 || frames > 256 || data.isEmpty() || data.length() > 4 * 1024 * 1024)
        { clearToEmpty ("unreadable"); return; }                       // malformed v1 is dropped (Stage 1 P3/P4 fixture)

    juce::MemoryBlock bytes;
    { juce::MemoryOutputStream mos (bytes, false); if (! juce::Base64::convertFromBase64 (mos, data)) { clearToEmpty ("unreadable"); return; } }
    ImportedPcm pcm; pcm.filename = sanitiseName (str ("filename")); pcm.numFrames = frames;
    const bool ok = enc == "flac16" ? decodeFlac16 (bytes, frames, pcm.pcm) : decodePcm16Gz (bytes, frames, pcm.pcm);
    if (! ok) { clearToEmpty ("unreadable"); return; }

    auto bank = WavetableImporter::buildImportedBank (pcm);           // SAME builder as import -> bit-identical (FUNC-04)
    const juce::ScopedLock sl (bankStateLock);
    publishImportedBank (std::move (bank));
    cachedBlob = { pcm.filename, enc, data, frames, {} };             // VERBATIM incoming string: save-after-load is byte-identical
    importStatus = { ImportStatus::State::done, pcm.filename, frames, {} };
    importStatusVersion.fetch_add (1, std::memory_order_release);
}
```
- **Synchronous restore** (decode + 11-level mips, about 50 ms worst case [VERIFIED estimate: ARCHITECTURE.md:245]) is deliberate. A host or pluginval that calls `getStateInformation` right after `setStateInformation` must get the same blob, and the FUNC-04 harness needs no message loop.
- **Stage 1 probes to update in 2.4** [VERIFIED: tests/state-check/main.cpp:35-38, 155-162, 332-400]:
  - **P3/P4's fake child** (`numFrames="3"`, `encoding="flac16"`, `data="AAAA"`) is malformed v1. It still produces 0 children on save and no crash, so keep P3 as the "malformed tolerated" probe.
  - **P4** says "Stage 2.4 flips this to exactly 1". Add a **valid-blob** two-reopen probe: exactly 1 child per save, the `data` string identical across saves, and the bank bit-identical. Also add the memory `critical_preset_manager_stale_customstate_child` variant: reopen → import a different file → save → reopen must restore the **second** file.
- **Presets:** `applyFactoryPreset` sets params only. It never touches the bank or the blob, so lessons keep the user's import (consistent with ARCHITECTURE §A9 "built-in banks only").

---

## 7. Gates and how to drive them offline

**Harness:**
- Add console targets through the existing helper, e.g. `ouaricon_add_processor_console(O-simpleWavetable … tests/mod-gates/main.cpp mod-gates)` and `… tests/import-gates/main.cpp import-gates`. The target is named `O-simpleWavetable-<suffix>`. It is compiled with `JUCE_WEB_BROWSER=0` and no editor TU [VERIFIED: scripts/param-dump/ParamDump.cmake:81, 136-200; CMakeLists.txt:72-79].
- After the call, add `target_compile_definitions(O-simpleWavetable-import-gates PRIVATE OSIW_TEST_HOOKS=1)`. The test target compiles its own copies of the sources, so hook-only members cause no ODR mixing.
- There is no message loop: `JUCE_MODAL_LOOPS_PERMITTED` = 0 [VERIFIED]. So:
  - drive the async step with `handleUpdateNowIfNeeded()`
  - call `sweepRetiredBanks()` through a test accessor
  - poll `getImportStatusVersion()`

**Test hooks (`#if OSIW_TEST_HOOKS` only):**
- `xfadeLenOverride`, `forceLevel`, `smootherBypass`, `lfoSeed`
- `disableHeldExclusion` (negative control for §1.3)
- graveyard mode: the reaper parks banks instead of freeing them and sets `testReaped`
- `testDerefAfterReap` counter, `WavetableBank::testLiveCount`, `getHeldBankCount()`
- `midBlockCallback` (invoked between entries++ and exits++)
- `getLfoBuffer()`, `WtVoice::getEffPos()`

**Alloc gate:** reuse O-Bells' `malloc_logger` block. The flag and counter are `volatile`, a liveness probe (one deliberate malloc must count 1) runs first, and counting is scoped to `pthread_equal (pthread_self(), audioThread)`. The worker and the harness's import thread allocate freely [VERIFIED: O-Bells/tests/render-harness/main.cpp:59-66, 90-120, 352-367; memory malloc_logger_alloc_gate_two_traps]. Warm up one unarmed block (memory operator_new_counter_blind…).

### 7.1 QUAL-03 (two tiers)
1. **Exactness (implementation-level, strong; covers every trigger, including high-frequency ones).**
   - Render A (old config) and B (new config) as steady renders with the same note and phase origin, static position, LFO depth 0 and env 0.
   - Expected: `y_ideal = (1−w)A + w·B`, with w a linear ramp of `xfadeLen` starting at the trigger block's first sample.
   - Assert `max|y − y_ideal| ≤ 1e-4`.
   - Use `forceLevel` for the bend-across-boundary case (A forced to L, B to L+1, same bend automation).
   - Negative control `xfadeLenOverride = 0` must give ≥ 0.1.
   - Triggers covered: bank param switch, import publish (empty→X, X→Y, X→empty), bandlimit toggle, interp toggle, level change.
2. **Excess click ratio (implementation-independent), §3.4.**
   - Bank switch Sine→Saw(0) ↔ Formant(0) at A1/A2 with 8 phase offsets of the switch instant.
   - Import swap with a sine-like source → formant-like source.
   - Imported → empty.
   - Import during 16 held notes (§7.4).
   - Ratio ≤ 1.5. Run a liveness check on |A−B|, and the negative control must be ≥ 4.
   - Also run the **fold** case: a second trigger 1 ms into the first fade (bank switch, then a bandlimit toggle) must still pass both tiers (memory splice/jump-during-fade).

### 7.2 DSP-06 / PERF-01
The alloc gate stays armed across:
- 200 bank-param switches between built-ins and Imported
- 50 import publishes from a second thread, plus restore-publishes, during rendering

Expect 0 audio-thread allocations, with the liveness probe proven first.

### 7.3 Reaper rules (deterministic, single-threaded where possible)
- **Quiescent:** with no `processBlock` calls:
  - With bank = Drive, import A then B: A is freed immediately (`retired == 0`).
  - With bank = Imported, voices held on X (one block rendered with a held note, then the host goes idle): import Y. X must **stay** (`retired == 1`, `held == X`). Render 1 block, then sweep: `retired == 0`.
- **Negative control for the amendment:** `disableHeldExclusion = true` in graveyard mode, same idle-import scenario. The next block must report `testDerefAfterReap ≥ 1`. This proves both the gate and the need for the amendment. With the amendment on it must be 0.
- **+2 rule:** `midBlockCallback` calls the sweep while a block is in flight (`entries != exits`):
  - With a stamp at exits = e: not freed while exits < e+2, freed at e+2.
  - Separately, the quiescent rule must not fire in flight.
- **Concurrent soak:**
  - The audio loop renders 16 held notes on Imported (main thread).
  - A thread calls `importFromFile` 50× with random 0–20 ms gaps, alternating 3 files.
  - A thread sweeps every 5 ms.
  - Assertions: output finite; `testDerefAfterReap == 0`; `getHeldBankCount() ≤ 3` at every sample point; after the threads stop plus 3 blocks plus a sweep, retired == 0 and `WavetableBank::testLiveCount == builtIns(5) + 1`.
  - Use the count, **not addresses** (memory distinct_buffer_addresses_are_not_an_allocation_bound; Prism [A1] precedent).

### 7.4 FUNC-03 / COMPAT-03
- **Generate fixtures in the harness** with JUCE writers into a temp dir:
  - 10 s at 44.1 kHz = 441000 samples → **215** frames (441000/2048 = 215.3) [arithmetic]
  - 600000 samples → **256**
  - exactly 2048 → 1
  - 2·2048 + 100 → 2 (tail dropped)
  - 2047 → `tooShort`, and the existing bank must be untouched
  - garbage bytes named `.wav` → `unreadable`
  - 97 MB memory import → `tooLarge`
- **COMPAT-03:** the same 16-bit content written as WAV/AIFF/FLAC at 44.1/48/96 kHz. All 9 imports must produce **identical int16 PCM** (the sample rate is irrelevant to slicing) and bit-identical banks.
- Also: 24-bit WAV, float WAV containing a NaN (scrubbed, finite bank), stereo (mean), 6-channel (mean of 6, not 2).
- **"No dropouts while notes are held":** structural. The audio thread takes no locks (grep for `ScopedLock` in the render path, which `juce::Synthesiser`'s own lock excepts) and the alloc gate passes. Never use wall-clock in a verdict (memory wallclock_inside_a_stability_verdict).

### 7.5 FUNC-04
- P1 imports F (synchronous helper: `importFromFile`, then wait on the status version, sleeping the harness thread).
- Snapshot `bank->data` (all 11 levels) and `filename`.
- `getStateInformation`, then a fresh P2 `setStateInformation`. Assert:
  - `memcmp` of the whole `data` vector is equal
  - `numFrames` and filename are equal
  - P2's save `data` string == P1's (verbatim cache)
- **Negative control:** decode, change one int16, re-encode → the compare must fail.
- **pcm16gz path:** same PCM via `encodePcm16Gz` → bit-identical bank.
- **Two reopens with a change between them** (§6.3).
- **Absent child:** a pre-import blob restores to an empty Imported, and Bank=Imported renders exact silence.

### 7.6 2.3 gates
- **DSP-05.** Clamp: pos 1, depth 1, Square, env +1 gives `0 ≤ effPos ≤ 1` on every sample (test accessor). Zipper: step `position` 0→1 at a block boundary with Interp On, then run §3.4. Ratio ≤ 1.5, and the negative control (`smootherBypass` + knob ramp 0) must be ≥ 4.
- **FUNC-05:**
  - `getLfoBuffer()` vs the analytic shapes (1e-6) for all 5 shapes.
  - Free 0.5 Hz: period 96000 samples at 48k (±1).
  - Tempo: synthetic playhead at bpm 120 and 97.3, all 16 divisions, period = beats·60/bpm·fs. Phase = `frac(ppq/beats)` at each block start within 1e-9, with random block sizes 1..4096.
  - A transport loop jump re-locks in the next block.
  - Stopped → free-run at the host BPM. No playhead or BPM ≤ 0 → 120.
  - **One global phase:** two voices started 0.37 s apart with env 0 have equal `effPos` per sample.
- **S&H determinism:**
  - two renders are byte-identical
  - re-prepare gives a byte-identical render
  - different seeds give different renders
  - a tempo-mode loop region replays identical S&H values
- **FUNC-06:**
  - env +1 rises from the knob following A/D/S; −1 falls.
  - `amp_release` 0.05 s with `menv_release` 10 s: the voice is inactive within 0.1 s.
  - Re-pushing identical ADSR params every block during release must not change the release length (dirty-check regression).
- **Click gate, Square/S&H at 100% depth, Interp On (§3.3):** Sine→Saw, pos 0.5, 2 Hz, A2, ratio ≤ 1.5 at every edge. The negative control is the smoother bypassed.
- **Block-size invariance (recommended):** free mode, static params, notes on fixed samples. 64/512/4096/random partitions must be bit-identical (§2.6).

---

## 8. Seams for Stage 3, and the remaining pitfalls

**Stage 3 seams (shapes from the template [VERIFIED: v1-PluginEditor.cpp:41-58]):**
- `importFromFile`, `importFromMemory`, `getImportStatus`, `getImportStatusVersion`: done in 2.4 (§5.3).
- `getBankDisplayGeneration()`: bumped by `publishImportedBank`, by a bank-param change (detected in `processBlock` as a relaxed atomic bump; no allocation), and by restore. `getBankThumbnails()` reads `WavetableBank::thumbs` from the owner `shared_ptr` copied under `bankStateLock` (built-ins come from `BuiltInBanks`).
- `buildCycleView()` (Stage 3.2) reuses `readCycle` from 2.1/2.2. 2.3 must publish these display atomics after rendering: lead-voice `dispPos`, `dispLevel`, `dispFrame` (−1 when Interp On), `dispSounding`, plus `dispLfo` (`lfo.lastValue(n)`), `dispMenv` and `dispAmp`, matching the optional `cycleUpdate` fields `lfo`, `menv`, `amp` [VERIFIED: checklist:57].
- `importStatus` error vocabulary: `tooShort | unreadable | tooLarge`. The extra `unsupported` (forward-compat) maps to "generic" on the page [VERIFIED: checklist:59, "anything else → generic"].
- The page sends `uiReady` and C++ re-sends bank and status, because `emitEventIfBrowserIsVisible` drops events while hidden [VERIFIED: checklist:61].

**Remaining pitfalls (beyond the top 10):**
- **ThreadPool lifetime:** call `removeAllJobs (true, 10000)` in the processor destructor, and declare `importPool` last so it is destroyed first. Jobs capture `this`.
- **Worker-thread FFT:** `MipmapBuilder` must be reentrant (§0). Restore on a host thread and a worker import can build at the same time.
- **Memory peak:** live (23 MB) + held (23 MB) + building (23 MB + 2 MB mono + 4 MB FLAC/base64) ≈ 75 MB transient per instance. Mip-level aliasing for imported banks saves nothing for real audio (content up to Nyquist), so accept about 23 MB steady-state. That answers CONTEXT's open question [ASSUMED].
- **Interp toggles:** Off→On must not glide from a stale `effPos` (§2.4). On→Off latches `raw` at the next wrap.
- **`lfo_depth = 0` still renders the LFO** for the display (cheap). Do not skip it, or the UI's `lfo` field freezes.
- **Imported bank with `numFrames = 1`:** `framePos = effPos·0`, so modulation is inert. Correct and harmless. Guard `N−1 = 0` divisions in the display code.
- **`releaseResources()`** (Stage 1 calls `synth.allNotesOff`) [VERIFIED: :171-174]: also store `audioHeldBank = nullptr` there and in `prepareToPlay`. Both are non-concurrent with `processBlock`, which lets the reaper free the last bank sooner. This is optional.

---

## Assumptions log

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | The `audioHeldBank` amendment fully closes the quiescent-rule UAF | §1.3 | UAF on idle-host import. **Gated by §7.3's negative control.** |
| A2 | seq_cst on AArch64 costs nothing extra vs acq/rel for these ops | §1.3 | Negligible perf only |
| A3 | A one-pole kink (~0.16/sample) fails a 1.5× excess-ratio gate on Square at 100% depth | §3.3 | The planner may build an unneeded 2-pole path. Measure first. |
| A4 | Downward-only level hysteresis is alias-safe and stops storms | §4 | 2.2 owns this. Verify against QUAL-02. |
| A5 | Smoothing `lfo_depth`/`env_amount` (20 ms) is a welcome addition | §2.3 | It deviates from ARCHITECTURE. Needs user/planner acceptance. |
| A6 | Some hosts call `setStateInformation` off the message thread | §6.3 | If false, the lock is merely unnecessary |
| A7 | FLAC-encoded bytes may differ across CPUs, while decoded PCM is identical | §6.1 | Gates already avoid byte compares of the encode |
| A8 | Hash-addressed S&H satisfies the locked "deterministic RNG reseeded in prepareToPlay" | §2.2 | If the user insists on `juce::Random`, keep it sequential (still deterministic, not loop-repeatable) |

## Open questions for PLAN
1. Approve the REG-01 **amendment** (`audioHeldBank`) and the seq_cst hardening as documented deviations from "port verbatim". Recommendation: yes. The verbatim rule is unsafe with the locked frozen-cycle design.
2. API names: `importFromFile`/`importFromMemory` (template) vs `requestImport` (CONTEXT example). Recommendation: the template names.
3. Mono legato vs ARCHITECTURE §11 retrigger. CONTEXT's true legato is assumed. Confirm that 2.2 implements it, so the mod env and smoother are not retriggered.
4. Should the click gates on toggles/level changes accept the exactness tier as satisfying QUAL-03? Recommendation: yes, with the ratio tier on the bank/import swaps where a valid stimulus exists.

## Sources
- **Primary (read this session):**
  - plugin `.planning/*`, `Source/*`, `tests/state-check/main.cpp`, `mockups/v1-*`
  - O-Prism `Source/PluginProcessor.{h,cpp}`, `NoteDivisions.h`, `dsp/WavetableImporter.{h,cpp}`, `CHANGELOG.md`
  - O-simpleGrain `PluginProcessor.{h,cpp}`
  - O-TextureForge `PluginProcessor.{h,cpp}`, `dsp/CorpusLoader.h`
  - O-GrainScatter `TempoTracker.h`, O-FreqPulse `PluginProcessor.cpp`
  - O-Bells harness, `scripts/param-dump/ParamDump.cmake`, `modules/core/webview-drop-streaming/*`
  - JUCE 8.0.15: `juce_FlacAudioFormat.{h,cpp}`, `juce_AudioFormatReader.h`, `juce_AudioFormatWriterOptions.h`, `juce_AudioFormatManager.h`, `juce_MemoryOutputStream.cpp`, `juce_MemoryBlock.h`, `juce_Base64.h`, `juce_GZIP*`, `juce_AudioPlayHead.h`, `juce_SmoothedValue.h`, `juce_Synthesiser.{h,cpp}`, `juce_ThreadPool.h`, `juce_Thread.h`, `juce_MessageManager.h`, `juce_AsyncUpdater.h`, `juce_PlatformDefs.h`, `juce_FFT.h`, `juce_AudioProcessor.h`
- **Probe:** `research-probes/flacprobe/probe.cpp` (built and run, ALL PASS)
- **Memory notes:** as listed in the brief, plus malloc_logger_alloc_gate, operator_new_counter_blind, zipper_gate_absolute_step, sh_lfo_clock_seeded_random, offline_dsp_render_harness, block_rate_envelope, gate_stimulus_below_threshold, synthesiser_startvoice

**Confidence:**
- Stack/APIs: HIGH (source-read plus the probe).
- Reaper amendment: MEDIUM until the §7.3 negative control runs.
- 2.3 click thresholds: MEDIUM-LOW (estimated, must be measured).

**Valid until:** about 30 days, or until the JUCE pin changes.
