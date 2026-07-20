# Stage 2 (DSP) — SUMMARY (Phase 2.1)

**Plugin:** O-simpleSampler
**Stage:** 2 of 4 — DSP
**Phase executed:** **2.1 only** (Core Playable Sampler) — STOP for DAW play-test per CONTEXT D2
**Date:** 2026-06-25
**Result:** ✅ First audio. Build clean (VST3+AU+Standalone), auval SUCCEEDED (21 params), pluginval@5 SUCCESS, installed.

---

## What was built

Turned the silent Stage-1 16-voice shell into a polyphonic, MIDI-playable sampler: the embedded
`piano.wav` is decoded/resampled off the audio thread and played through a per-voice fractional-read
varispeed ("Repitch") head, isolated by the start/end region, anti-aliased on up-transposition, and
shaped by a per-voice amp ADSR + VCA + velocity-sensitivity blend, tuned relative to the live Root Key.

### Tasks completed (Phase 2.1 = Tasks 1–8)

1. **Embed `piano.wav` + 2nd binary-data target** — `Source/samples/piano.wav` copied from O-simpleGrain;
   `juce_add_binary_data(O-simpleSampler_Samples NAMESPACE BinaryData HEADER_NAME BinaryData.h)` added
   AFTER `juce_generate_juce_header` and linked. UI/`UIBinaryData` target left as a Stage-3 TODO with the
   distinct-NAMESPACE collision note preserved.
2. **Anti-alias read helpers (verbatim)** — `dsp/LagrangeInterpolation.h` copied verbatim;
   `readSourceLagrange` (4-tap, jlimit-clamped) + `aaOnePole` (precomputed-coeff one-pole) ported into
   `SampleVoice.h` as private statics.
3. **`SampleSound` + `SampleVoice`** — custom `juce::SynthesiserVoice` (NOT `juce::SamplerVoice`): Repitch
   read head (`voiceRate = 2^((note − rootKey + tune + fine·0.01)/12)`, live APVTS values), region clamp
   `[startSamp,endSamp)`, AA one-pole engaged on `rate>1` (coeff `1−exp(−2π·(0.5·fs/rate)/fs)` computed once
   on note-on, state primed), amp ADSR via `juce::ADSR`, `velLevel=(1−v)+v·velocity` (`v=velToAmp·0.01`),
   non-virtual `prepareToPlay` (setSampleRate **before** setParameters — JUCE-8 order gate), lifetime keyed
   on the amp env, `SamplerVoiceParams` push struct.
4. **Source decode → resample → atomic publish** — `std::shared_ptr<AudioBuffer> currentSource` with
   `std::atomic_load/store` helpers; `decodeAndPublish` (`AudioFormatManager` + `createReaderFor(MemoryInputStream)`
   → `resampleToEngineRate` offline `juce::LagrangeInterpolator`, capped at `kMaxSourceSeconds`) → atomic swap;
   `loadBuiltInSource` + `builtInBlob` switch (index 0 → `BinaryData::piano_wav`; 1–3 fall back to the piano
   blob, documented TODO — no selection is silent).
5. **`sourceSample` listener → AsyncUpdater + per-source root seed** — processor is now an
   `APVTS::Listener` + `AsyncUpdater`; `parameterChanged`→`triggerAsyncUpdate`→`handleAsyncUpdate` does
   `loadBuiltInSource` + `seedRootForSource`. Per-source root table `kBuiltInRoot={48,69,72,48}` — **piano=48**
   (probed f0 ≈131.25 Hz; root 60 plays an octave flat). APVTS `rootKey` *default* stays 60 (frozen); the
   **live** value is seeded. Prepare-time guarded seed (`rootSeeded`/`stateWasRestored`) seeds a fresh
   instance once and leaves a restored session's saved `rootKey` intact.
6. **16-voice synth + per-block param push** — `juce::Synthesiser` with 16 `SampleVoice` + 1 `SampleSound`,
   note-stealing on; `prepareToPlay` dispatches each voice's non-virtual prepare via `dynamic_cast`, decodes
   the active/restored source, resets `outputGain`; `processBlock` (`ScopedNoDenormals`) snapshots the source
   `shared_ptr` once, reads the 21 cached atomics, builds `SamplerVoiceParams` + region samples, pushes to
   every voice, `synth.renderNextBlock`, smoothed output trim (`decibelsToGain(outputLevel,−60)` ramp), final
   `std::isfinite` scrub. Allocation-free.
7. **Restore-aware `setStateInformation`** — after `replaceState`, re-decodes the restored `SOURCE/identity`
   source at the current engine rate, sets `stateWasRestored` (skips the prepare-time root seed), and
   `cancelPendingUpdate()` + clears `pendingBuiltInIndex` so the listener-queued rebuild can't clobber it.
8. **Build / validate / install** — see Validation below.

---

## Files

**New:** `Source/samples/piano.wav`, `Source/dsp/LagrangeInterpolation.h`, `Source/SampleSound.h`, `Source/SampleVoice.h`
**Modified:** `CMakeLists.txt`, `Source/PluginProcessor.{h,cpp}`

---

## Deviation — class naming collision (NEW gotcha)

`SamplerSound` / `SamplerVoice` (the names in PLAN Task 3) **collide with `juce::SamplerSound` /
`juce::SamplerVoice`** (`juce_audio_formats/sampler/juce_Sampler.h`). The generated `JuceHeader.h` does
`using namespace juce;`, so the unqualified `Sampler`-prefixed names are **ambiguous at every use** —
the build failed with `reference to 'SamplerVoice' is ambiguous`. This is the same class of bug as the
documented `regionStart`/`regionEnd` vs `juce::end` collision.

**Fix:** classes renamed to `SampleSound` / `SampleVoice` (drop the "r" — `Sampler`-free, collision-checked
against JUCE), files renamed to match, with a header comment in each documenting why. The `SamplerVoiceParams`
struct keeps its name (no `juce::SamplerVoiceParams` exists). Rebuild then linked all three formats.

---

## Validation (Task 8)

| Check | Result |
|-------|--------|
| `ninja` VST3 + AU + Standalone | ✅ clean (after the naming-collision fix) |
| `auval -v aumu OsSm OuDv` | ✅ **AU VALIDATION SUCCEEDED** (render + 1-channel + MIDI + ramped-param) |
| Parameter count | ✅ **21 Global Scope Parameters** (frozen, unchanged from Stage 1) |
| `pluginval --strictness-level 5` (VST3) | ✅ **SUCCESS** |
| Bus layout | ✅ 0 input / 2 output (instrument); Mono+Stereo output layouts |
| Install (build-and-install.sh dual-variant sweep) | ✅ single variant installed, no orphan shadow |

### Phase 2.1 success criteria
- ✅ Loads as an instrument; MIDI routes; 16-voice; no crash (auval MIDI + pluginval).
- ✅ Build clean (3 formats); auval SUCCEEDED; pluginval@5 SUCCESS.
- ⏳ **DAW play-test gate (CONTEXT D2)** — the audible criteria below need a DAW and are the explicit STOP:
  - Root Key (48 for piano) plays at original pitch (~131 Hz); notes transpose by varispeed.
  - Start/End change the played region; tune/fine transpose independent of the keyboard.
  - Piano selects/decodes/plays; selecting it seeds root=48; fresh instance in standard tune.
  - No clicks on note-on/off (5 ms ADSR); no obvious aliasing at high notes (formal probe deferred to 2.3).

---

## Forward scope (NOT in this execute)

- **Phase 2.2:** loop (fwd/ping-pong + equal-power crossfade) + reverse + Stretch (synchronous-granular SOLA)
  + Vintage (S&H + bit-crush) + resonant LP filter.
- **Phase 2.3:** AA hardening + lock-free viz taps + voice-stealing audit + RT-safety + offline render-harness
  (the Stage-2 correctness gate).

*Phase 2.1 complete 2026-06-25. STOP for DAW play-test per CONTEXT D2 before 2.2.*

---
---

# Stage 2 (DSP) — SUMMARY (Phase 2.2a — Tone chain)

**Phase executed:** **2.2a only** (tone chain) — STOP for DAW checkpoint per CONTEXT D4 split. **2.2b (Stretch SOLA) NOT in this pass.**
**Date:** 2026-06-25
**Result:** ✅ Sustaining, tone-shaped sampler. Build clean (VST3+AU+Standalone), auval SUCCEEDED (still 21 params), pluginval@5 SUCCESS, installed (dual-variant sweep, no orphan). Implemented by dsp-agent (Tasks 1–8); built/validated by orchestrator (Task 9).

## What was built

Inserted the shared downstream tail **Vintage → Filter → VCA(·endRamp)** between source-generation and the
existing VCA, and gave the Repitch read head a full **loop engine** (forward equal-power crossfade, ping-pong,
reverse) plus a **region-end declick**. The 9 previously-inert APVTS params (`loopMode`/`loopStart`/`loopEnd`/
`loopCrossfade`/`reverse`/`pitchMode`/`vintage`/`filterCutoff`/`filterResonance`) are now live — **no APVTS change**
(all were already in the layout + cached as atomic pointers).

### Tasks completed (Phase 2.2a = Tasks 1–8)

1. **`SamplerVoiceParams` extended + processor param-push + filter smoothers** — 9 fields added
   (`loopMode`, `loopStartSamp`, `loopEndSamp`, `xfadeSamp`, `reverse`, `pitchMode`, `vintage`, `filterCutoff`, `filterQ`).
   Processor reads the cached atomics; loop bounds = % of region (`loopAbsStart/End`, clamped `< … ≤ endSamp`);
   `xfadeSamp = jlimit(0, loopLen/2, ms·fs/1000)`; cutoff `jlimit(20, 0.45·fs)`; Q via the **net-new** map
   `jmap(filterResonance,0,100, 0.707f, 12.0f)` (NOT `resonanceToK`). Two processor `SmoothedValue` (cutoff/Q,
   20 ms) — filter smoothing lives in the processor, **not 16× per-voice**.
2. **Region-end declick (raised-cosine end-ramp)** — fixes the 2.1 verify warning. `endRampSamp =
   jmin((endSamp−startSamp)/4, 0.005·fs)` (≤5 ms); `0.5−0.5·cos(π·max(0,distToEnd)/endRampSamp)` into the VCA,
   **one-shot path only** (bypassed when looping), reverse-aware (tapers `readPos−startSamp`). The 0.2 s amp
   release is structurally unreachable from `ampEnv.reset()` — a dedicated ramp is required.
3. **Forward loop equal-power crossfade (PORT)** — dual-head + `equalPowerWeights` (`{cos,sin}`) +
   wrap-by-subtracting-loopLen (never hard-jumps; preserves sub-sample phase), generalized from MicrotonalSampler's
   fixed-8 fade to `xfadeSamp`. **One justified deviation** (see below).
4. **Ping-pong reflect + reverse seed (NET-NEW)** — per-voice `int dir`; reflect `readPos = 2·bound − readPos` +
   flip `dir` at each loop bound, equal-power crossfade across the turnaround. `reverse` composes independently
   (`startNote` seeds `dir=−1`, `readPos=endSamp`).
5. **Zero-cross snap (NET-NEW, off-thread)** — extended the existing Listener to `start`/`end`/`loopStart`/`loopEnd`;
   `parameterChanged` sets `pendingSnap`+`triggerAsyncUpdate()`; `computeZeroCrossSnaps()` (message thread) scans
   ±256 samples for the sign change minimizing `|src[i]|`, publishes four `std::atomic<int>` (sentinel −1). Audio
   thread overrides the %-bounds when valid, re-clamps. Secondary defense (crossfade is primary).
6. **Vintage — S&H decimate (net-new) → bit-crush (verbatim)** — both stages gated `if (vintage>0)` ⇒
   **bit-for-bit clean at 0** (DSP-04), **before** the filter, order decimate→quantize. S&H: `shInc =
   jmap(v,0,100, fs,3000)/fs`, latch on phase wrap. Bit-crush (O-simpleAdditive idiom): `bits=jmap(v,0,100,16,8)`,
   `qLevel=exp2(bits−1)`, `round(s·qLevel)·qInv`.
7. **Per-voice resonant LP** — `juce::dsp::StateVariableTPTFilter<float>` prepared **mono** in `prepareToPlay`
   (`setType(lowpass)`), `reset()` in `startNote`. `setCutoffFrequency`/`setResonance` called **once per block**
   from the processor-smoothed scalars (one `tan`); `processSample(0,s)` after Vintage, before the VCA.
   `setResonance` guarded `>0` (floor 0.707 — never asserts).
8. **Lead-voice display atomics + `SubVizAnalyzer.h`** — `displayCutoffHz`/`displayK` atomics + getters; once-per-block
   loudest-active scan publishes `displayCutoffHz = smoothedCutoff`, `displayK = 1/Q` (= JUCE's `R2`). Copied
   `O-simpleSubtractive/Source/SubVizAnalyzer.h` → `Source/SubVizAnalyzer.h` **verbatim** (byte-identical; Stage-3
   curve consumes it — drawing deferred). Header not yet compiled anywhere.

## Files

**New:** `Source/SubVizAnalyzer.h` (verbatim copy; Stage-3 curve bridge)
**Modified:** `Source/SampleVoice.h` (+266/−… loop/declick/Vintage/filter + extended `SamplerVoiceParams`),
`Source/PluginProcessor.{h,cpp}` (param-push loop bounds/xfade/Vintage/filter; processor `SmoothedValue` cutoff/Q;
zero-cross-snap AsyncUpdater extension; loudest-active display atomics) — **490 insertions across 3 source files.**

## Deviation — loop-crossfade incoming-head offset (justified)

The plan (P3.2) ports MicrotonalSampler's `incoming = read(readPos − loopLen + xfadeSamp)`. The agent used
**`read(readPos − loopLen)`** (pre-loop content). Continuity at the wrap requires
`incoming(loopEnd⁻) == read(loopStart)`, which forces offset 0; the literal `+xfade` leaves an ~`xfade`-sample
seam jump — inaudible for MicrotonalSampler's fixed 8-sample (sub-ms) fade, but would **audibly click at the
100 ms crossfade** the acceptance test exercises (`crossfade 0/10/100 ms`). The continuous form is the classic
crossfade-loop technique, direction-symmetric (`+loopLen` for reverse-loop). Documented inline at the call site.

## Validation (Task 9)

| Check | Result |
|-------|--------|
| `ninja` VST3 + AU + Standalone | ✅ clean |
| `auval -v aumu OsSm OuDv` | ✅ **AU VALIDATION SUCCEEDED** (still **21** Global Scope Parameters — frozen) |
| `pluginval --strictness-level 5` (VST3) | ✅ **SUCCESS** |
| Install (build-and-install.sh dual-variant sweep) | ✅ single variant, no orphan shadow |

### Phase 2.2a success criteria
- ✅ No APVTS change; 21 params intact; RT-safety preserved (audio path alloc/lock/IO-free; per-block transcendentals
  precomputed; zero-cross snap message-thread-only; `setLatencySamples(0)` retained).
- ✅ Build clean (3 formats); auval SUCCEEDED; pluginval@5 SUCCESS; installed.
- ⏳ **DAW play-test gate (CONTEXT D4 — the explicit 2.2a STOP):**
  - Loop forward sustains a short sound with **no seam click** (crossfade 0/10/100 ms); **ping-pong + reverse** correct.
  - **Region-end no longer clicks** when End is dragged down on a held one-shot note.
  - **Vintage clean at 0** → grit as raised; no NaNs across the range.
  - **Filter** shapes tone, LP open at default, **no zipper** on cutoff/resonance sweeps.

## Forward scope (NOT in this execute)

- **Phase 2.2b:** Stretch SOLA (synchronous-granular; `pitchMode` toggle) — copy `Grain.h`/`WindowLuts.h`,
  voice-local `timePos` + fixed `grainSize/2` hop, latch `pitchMode` at note-on → DAW A/B.
- **Phase 2.3:** viz taps + voice-stealing audit + AA hardening + RT-safety + offline render-harness (Stage-2 gate).

*Phase 2.2a complete 2026-06-25. STOP for DAW checkpoint per CONTEXT D4 before 2.2b.*

---
---

# Stage 2 (DSP) — SUMMARY (Phase 2.2b — Stretch SOLA)

**Phase executed:** **2.2b** (Stretch SOLA — synchronous-granular pitch engine). Human DAW A/B **deferred to post-GUI** (user decision); the 2.3 render-harness proves pitch/time independence automatically.
**Date:** 2026-06-25
**Result:** ✅ `pitchMode` toggles Repitch ↔ Stretch. Build clean (VST3+AU+Standalone), auval SUCCEEDED (21 params), pluginval@5 SUCCESS, installed. Task 10 (grain-machinery copy) by orchestrator; Tasks 11–12 + WindowLuts wiring by dsp-agent; Task 13 (build/validate) by orchestrator.

## What was built

`latchedPitchMode==1` swaps the Repitch read head for a **SOLA synchronous-granular** engine on a voice-local
**time axis**: `timePos` advances at **1× realtime** (duration preserved) while each grain reads the source at the
**key rate** `voiceRate` (pitch tracks the note) — "same length, different pitch." Both paths feed the **same**
Vintage → Filter → VCA(·endRamp) tail; loop-wrap / ping-pong / region-end act on `timePos` in Stretch.

### Tasks (Phase 2.2b = Tasks 10–12)

10. **Grain machinery copied (verbatim)** — `Source/dsp/Grain.h` + `Source/dsp/WindowLuts.h` from O-simpleGrain.
11. **Voice-local time axis + SOLA scheduler** — per-voice `std::array<Grain,4>` + `double timePos` +
    `samplesUntilNextGrain` + `nextGrain`. `spawnGrainSOLA` (steal-oldest, **`g.readPos=timePos`**, **`g.rate=voiceRate`**,
    Hann shape 4, mono-centred, per-grain AA on spawn, **zero spray, no RNG**). Fixed hop `lenSamp = max(2, 60 ms·fs)`,
    `samplesUntilNextGrain = lenSamp/2` ⇒ **2× Hann overlap** (COLA, **√overlap normalizer dropped**). `timePos += 1·dir`
    per output sample; loop-wrap / ping-pong reflect / one-shot-end mirror Repitch on the time axis. **No dual-head
    crossfade in Stretch — the grain overlap smooths the seam.**
12. **`pitchMode` latched + shared tail** — `renderNextBlock` refactored into **dual read-path → shared tail → dual
    advance**: source-gen forks on `latchedPitchMode` (Repitch continuous read + voice AA · Stretch overlap-add +
    per-grain AA); the tail (Vintage → filter → endRamp → VCA → addSample) is single-instance; advance forks
    (`readPos += voiceRate·dir` vs `timePos += 1·dir`). `startNote` seeds `timePos=readPos`, and when Stretch is
    latched resets the pool + arms `samplesUntilNextGrain=0` (fires grain 0 — no startup gap).

**WindowLuts ownership (RT-critical):** one processor-owned `WindowLuts windowLuts { 2048 }` (built in the ctor,
off the audio thread), shared read-only via `const WindowLuts*` set per-voice in `prepareToPlay`. Never per-voice,
never in the render path. SOLA reads shape 4 (Hann) only; null-guarded.

## Files

**New (Task 10):** `Source/dsp/Grain.h`, `Source/dsp/WindowLuts.h` (verbatim copies)
**Modified:** `Source/SampleVoice.h` (grain pool + SOLA scheduler + dual-path render + WindowLuts setter),
`Source/PluginProcessor.{h,cpp}` (`#include dsp/WindowLuts.h` + `WindowLuts windowLuts{2048}` member; per-voice
`setWindowLuts(&windowLuts)` in `prepareToPlay`)

## Deviation / note

- **Granular reverse semantics (by design):** grains always read forward (`g.rate=voiceRate`, positive); the **time
  axis** (`timePos`, hence successive grain spawn positions) reverses. Standard SOLA reverse (RESEARCH P6.2).
- **Repitch path unchanged** (verbatim inside the `else`); **2.2a not regressed** (Vintage bit-clean at 0,
  `setResonance` floored >0 — both reached unchanged from the shared tail). No APVTS change (21 params).

## Validation (Task 13)

| Check | Result |
|-------|--------|
| `ninja` VST3 + AU + Standalone | ✅ clean (incl. the new `SampleVoice.h → PluginProcessor.h` include) |
| `auval -v aumu OsSm OuDv` | ✅ **AU VALIDATION SUCCEEDED** (still **21** params) |
| `pluginval --strictness-level 5` (VST3) | ✅ **SUCCESS** |
| Install (dual-variant sweep) | ✅ single variant, no orphan shadow |

### Phase 2.2b success criteria
- ✅ `pitchMode` toggles Repitch/Stretch; latched at note-on (no mid-note click); zero latency retained.
- ✅ RT-safe (fixed grain `std::array`, shared LUT, one `exp` on spawn); no APVTS change.
- ✅ Build clean (3 formats); auval SUCCEEDED; pluginval@5 SUCCESS; installed.
- ⏳ **Repitch-vs-Stretch obviousness** — proven by the 2.3 render-harness **single-grain autocorr** probe
  (pitch/time independence); final human A/B batched post-GUI.

## Forward scope (NOT in this execute)

- **Phase 2.3 (next):** `displayPlayhead` + lock-free viz tap (copy `VizAnalyzer.h`); voice-stealing audit +
  AA hardening + RT-safety (2.1 hardening backlog); **offline render-harness — the Stage-2 correctness gate**
  (Repitch tuning · Stretch pitch/time independence via single-grain autocorr · loop-seam + region-end continuity ·
  Vintage clean-at-0 · AA budget).

*Phase 2.2b complete 2026-06-25. Next: Phase 2.3 (hardening + viz + render-harness).*

---
---

# Stage 2 (DSP) — SUMMARY (Phase 2.3 — Hardening + Viz + Render-Harness) — STAGE 2 COMPLETE

**Phase executed:** **2.3** (final DSP pass). **Render-harness is the Stage-2 correctness gate** (load-bearing — human DAW testing deferred to post-GUI).
**Date:** 2026-06-25
**Result:** ✅ **Stage 2 DSP complete.** Harness **ALL 9 PASS (exit 0)**; build clean (3 formats); auval SUCCEEDED (21 params); pluginval@5 SUCCESS; installed. Tasks 14–16 by dsp-agent; build/run/validate by orchestrator.

## What was built

A lock-free **viz tap** (`VizRing` + `displayPlayhead`) for the Stage-3 UI, a hardening **audit** (no risky edits — stability first), and the **offline render-harness** that converts the deferred audible DAW checks into automated assertions.

### Tasks (Phase 2.3 = Tasks 14–16)

14. **`displayPlayhead` + lock-free viz tap** — `SamplerVizAnalyzer.h` (ported from O-simpleGrain `VizAnalyzer.h`; `VizRing` verbatim, analyzer renamed). Processor: `VizRing vizRing` + `std::atomic<float> displayPlayhead` + getters; `vizRing.write` at the **tail** of `processBlock` (post-gain, post-isfinite, ≤4096-sample stack chunks — no alloc/lock/FFT on the audio thread). Lead (loudest-active) voice publishes its playhead via new `SampleVoice::getPlayheadPos()` (live axis: `timePos` Stretch / `readPos` Repitch, normalized [0,1] over the region). FFT/drawing = Stage 3.
15. **Hardening audit (stability-first; no risky edits)** — confirmed: 16-voice stealing (`setNoteStealingEnabled(true)`; harness stress asserts tail-silence = no stuck voice), `ScopedNoDenormals` + final `isfinite` scrub + per-voice VCA `isfinite` guard, AA covers up-transposition (voice + per-grain). **Did NOT** re-architect the working source-swap path. Carried-forward forward items (documented, NOT fixed): message-thread reclaim queue for the source-swap free; `std::atomic_load/store(shared_ptr)` C++20 deprecation; `setValueNotifyingHost`-in-prepare advisability.
16. **Offline render-harness (the Stage-2 gate)** — `tests/render-harness/{main.cpp,CMakeLists.txt}` ported from O-simpleGrain; helpers (`continuityFraction`, `autocorrPitchHz`, `render`, `pumpMessages`, …) reused. CMake: `option(OUARICON_BUILD_TESTS OFF)` + `add_subdirectory(tests/render-harness)`; deps = `O-simpleSampler` + `O-simpleSampler_Samples` only (no UIResources — Stage 3); `JUCE_WEB_BROWSER=0` (placeholder editor needs no WebView to link); `JUCE_MODAL_LOOPS_PERMITTED=1` (pump the decode). All pitch checks use **`autocorrPitchHz`** (grain comb confounds spectral bins — project memory).

## Harness results (ALL 9 PASS, exit 0)

| Check | Result |
|-------|--------|
| makes-sound | rms=0.215 |
| repitch-tuning | f48=131.2 Hz · f60/f48=**2.000** · f36/f48=**0.501** |
| stretch-pitch-tracks | f60/f48=**2.012** (pitch tracks key in Stretch) |
| **stretch-time-independence (headline)** | **Repitch dur ratio 1.89** (pitch+time coupled) vs **Stretch 0.93** (duration held) |
| loop-seam-continuity | fwd cont 0.953 / ping-pong 0.944 · seam Δ 0.004 (no click) |
| region-end-declick | endΔ 0.0008 ≪ 0.5·contentLevel (raised-cosine, no hard cut) |
| vintage-clean-at-zero | flatClean 0.0026 vs flatCrush 0.309 · relDiff 0.19 (gate keeps 0 clean) |
| aa-uptranspose-stable | note 84 (+3 oct) bounded both modes |
| stress-bounded | 5-note chord, peak 4.8, tail silent (no stuck voice) |

## Substitution (documented in `main.cpp`)
**vintage-clean-at-zero** could not use the planned "quantization-grid delta" probe: the resonant LP filter sits **after** Vintage, smearing the 8-bit amplitude grid into continuous floats for both v0 and v100 → not observable on the output. Substituted two robust observables proving the same intent (v0 = clean end, raising vintage degrades): **aligned sample-by-sample RMS diff** (deterministic engine, no RNG) + **spectral-flatness rise** (quant noise + S&H aliasing fill the inter-harmonic valleys). Engineering call by the dsp-agent; verified PASS.

## Files

**New:** `Source/SamplerVizAnalyzer.h`, `tests/render-harness/main.cpp`, `tests/render-harness/CMakeLists.txt`
**Modified:** `Source/PluginProcessor.{h,cpp}` (VizRing + displayPlayhead + tail write + lead-voice playhead), `Source/SampleVoice.h` (`getPlayheadPos`), `CMakeLists.txt` (OUARICON_BUILD_TESTS option + add_subdirectory)

## Validation (Task 16 run + plugin re-validate after the viz tap)
- `O-simpleSampler-render-test` → **ALL 9 PASS, exit 0** (built via `-DOUARICON_BUILD_TESTS=ON`, target `O-simpleSampler-render-test`)
- `ninja` VST3+AU+Standalone → clean; **auval SUCCEEDED** (21 params); **pluginval@5 SUCCESS**; installed (dual-variant sweep, no orphan)

## Stage 2 (DSP) — COMPLETE
2.1 Repitch + region + amp ADSR + source decode · 2.2a loop/reverse/Vintage/filter tone chain · 2.2b Stretch SOLA ·
2.3 viz tap + hardening audit + render-harness. The Stage-2 correctness gate (render-harness) passes. **Outstanding:
the batched human DAW play-test (deferred to post-GUI) + Stage-3 forward items (UIResources target, viz curve/scope
drawing, the 3 documented RT-safety hardening backlog items).**

*Phase 2.3 complete 2026-06-25 — Stage 2 (DSP) COMPLETE. Next: Stage 3 (GUI).*
