# Stage 2: DSP — Plan (rev-1)

**Date:** 2026-04-27
**Plugin:** O-Bassoon
**Stage:** 2 of 4 (DSP)
**Phase:** plan
**Cycle Scope:** **Phase 2.1 — Core Modal Voice + First Audio**
**Inputs consumed:** `stages/2-dsp/CONTEXT.md` (rev-1), `stages/2-dsp/RESEARCH.md` (rev-1), `Source/{BassoonVoice,BassoonSound,PluginProcessor}.{h,cpp}` (Stage 1 R-foundation), `plugins/O-Bassoon/CMakeLists.txt`.

---

## Goal

Replace the Stage-1 silent voice stub with a working modal-synthesis voice that produces a sustained, in-tune tone for any single MIDI note (C1–C6) with no clicks, no NaN/inf, > 10 s sustain, < 5 % CPU @ 48 k / 256 (1 voice). Bassoon-specific spectral character (partial ratios + amplitude shaping + `tone` parameter), vibrato, breath, attack-character, polyphony, NE/MPE pitch consumption, and TuningEngine integration are explicitly **deferred** to Phases 2.2 → 2.4 per ROADMAP. Phase 2.1 proves the architectural seams (excitation → resonator bank → envelope → stereo write) and archives the reference bassoon C3 + spectrum baseline that Phase 2.2 will consume.

**Single atomic commit on Gate 1 PASS** — subject `feat(O-Bassoon): Phase 2.1 first audio - Gate 1 PASS`. No partial / WIP commits.

---

## Wave Structure

**One Wave only.** All tasks are tightly coupled (mode bank, exciter, ADSR, voice integration, CMakeLists, voice-prepare extension) and the gate-first principle requires every commit on `main` to keep build + auval + pluginval green. No parallel sub-waves.

Tasks 1–6 are file edits; Task 7 sources the reference recording (parallel-eligible with builds, but kept in sequence for clean commit ordering); Task 8 verifies the 10-item Gate 1 bar; Task 9 lands the atomic commit + planning artefact updates.

---

## Tasks

### Task 1 — Create `Source/ModeBank.{h,cpp}` (16-mode parallel pole-only resonator bank)

**Files:**
- `plugins/O-Bassoon/Source/ModeBank.h` (NEW)
- `plugins/O-Bassoon/Source/ModeBank.cpp` (NEW)

**Body:** Lift verbatim from RESEARCH.md §3 implementation skeletons. Specifics:

- `ModeBank::NUM_MODES = 16`
- `PARTIAL_RATIOS = {1, 2, 3, …, 16}` (Phase 2.1 placeholder integer harmonics — Phase 2.2 replaces with bassoon-tuned ratios)
- `BASE_T60 = {2.5, 2.2, 2.0, 1.8, 1.6, 1.4, 1.2, 1.0, 0.8, 0.7, 0.6, 0.5, 0.4, 0.35, 0.30, 0.25}` seconds
- Inner `struct ModeBiquad` — pole-only Direct-Form I (2-pole-no-zero specialisation): `b0`, `a1`, `a2`, `y1`, `y2` (20 bytes per mode). Per-sample: `y0 = b0*x − a1*y1 − a2*y2; y2 = y1; y1 = y0;` plus `std::isfinite` guard (resets `y1=y2=0`, returns `0.0f` if non-finite)
- `prepare(double sampleRate)` — stores `currentSampleRate`, calls `reset()`
- `setFundamental(float f0)` — recomputes coefficients per mode: `theta = 2π·f_k/fs; tau = T60_k/6.91; R = exp(-1/(tau·fs)); b0 = (1-R)·amp; a1 = -2R·cos(theta); a2 = R²`. Mute (`b0=a1=a2=0`) when `f_k > 0.45·fs` or `f_k <= 0` (Nyquist policy per OQ#5). Phase 2.1 `amp = 1.0f` (flat).
- `processSample(float excitation)` — sums all 16 modes, **multiplies result by `1.0f / NUM_MODES`** (per-voice headroom scaling, D2 in RESEARCH §4 — Phase 2.3 replaces with proper output_gain APVTS)
- `reset()` — zeroes `y1,y2` per mode
- `setTone(float)` — declared as inline no-op stub returning early (wired live in Phase 2.2)

**Constraints:**
- Header-only `ModeBiquad` struct inside `ModeBank` private section
- All math `noexcept`; no allocation in any method
- `static constexpr` arrays for `PARTIAL_RATIOS` and `BASE_T60`

**Depends on:** none

---

### Task 2 — Create `Source/Exciter.{h,cpp}` (5 ms half-sine × exp impulse, pre-baked at prepare)

**Files:**
- `plugins/O-Bassoon/Source/Exciter.h` (NEW)
- `plugins/O-Bassoon/Source/Exciter.cpp` (NEW)

**Body:** Lift verbatim from RESEARCH.md §3 implementation skeletons. Specifics:

- `MAX_ONSET_SAMPLES = 1024` (5 ms @ 96 kHz = 480; 1024 leaves headroom)
- `DURATION_MS = 5.0f`, `TAU_MS = 1.5f`
- Class-level `std::array<float, MAX_ONSET_SAMPLES> onsetBuffer{}` per voice
- `prepare(double sampleRate)`:
  - Compute `N = min(MAX_ONSET_SAMPLES, sampleRate · 0.005)`
  - Populate `onsetBuffer[i] = sin(π·i/N) · exp(-t/(τ·0.001))` for `i ∈ [0, N)`
  - Normalise peak to 1.0 (find max abs, divide all)
  - Call `reset()`
- `start()` — sets `onsetIdx = 0; active = true;` (allocation-free)
- `getNextSample()` — returns `onsetBuffer[onsetIdx++]` while `active && onsetIdx < onsetSamples`; otherwise sets `active = false` and returns `0.0f`
- `reset()` — `onsetIdx = 0; active = false;`

**Constraints:**
- All methods `noexcept`; no `std::vector`, no `make_unique`, no `Array::resize`
- Onset-buffer storage is in-class `std::array` (per-instance, populated once in `prepare`) — D2 decision in CONTEXT (per-instance over static-shared to avoid future "I bet I can mutate it" footgun)

**Depends on:** none

---

### Task 3 — Extend `Source/BassoonVoice.h` (add ModeBank/Exciter/ADSR members, prepareToPlay decl, pitch-bend state)

**Files:**
- `plugins/O-Bassoon/Source/BassoonVoice.h` (MODIFY)

**Body:** Edits per RESEARCH.md §3 implementation skeleton. Specifics:

- Add includes: `#include "ModeBank.h"`, `#include "Exciter.h"` (after existing `BassoonSound.h` line)
- Add **public** declaration: `void prepareToPlay (double sampleRate, int maxBlockSize);` (non-virtual; called from `OBassoonAudioProcessor::prepareToPlay` per-voice — the JUCE 8 `SynthesiserVoice` has NO virtual `prepareToPlay`, only `setCurrentPlaybackSampleRate`; D1 in RESEARCH §4)
- Add `static constexpr float PITCH_BEND_RANGE_SEMITONES = 2.0f;` (private)
- Add private members:
  - `ModeBank modeBank;`
  - `Exciter exciter;`
  - `juce::ADSR adsr;`
  - `int   pitchWheelValue       = 8192;`
  - `float pitchBendSemitones    = 0.0f;`
  - `float currentFrequencyBase  = 0.0f;`
- **Do NOT touch** existing `parameters`, `tuningEngine`, `pendingTuningSource` raw pointers or their setters — Phase 2.1 inherits Stage 1 wiring unchanged (strict ROADMAP minimal wiring, locked Q2)

**Constraints:**
- Member layout: new members appended after existing pointer members (ABI-incompatible class layout doesn't matter — internal class)
- `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` macro stays at end
- No new public methods other than `prepareToPlay`

**Depends on:** Tasks 1, 2

---

### Task 4 — Implement `Source/BassoonVoice.cpp` (replace silent stub with first-audio body)

**Files:**
- `plugins/O-Bassoon/Source/BassoonVoice.cpp` (MODIFY)

**Body:** Lift verbatim from RESEARCH.md §3 implementation skeleton. Specifics:

- `canPlaySound(juce::SynthesiserSound*)` — `return dynamic_cast<BassoonSound*>(sound) != nullptr;` (unchanged from Stage 1)
- `prepareToPlay(double sampleRate, int /*maxBlockSize*/)`:
  ```cpp
  setCurrentPlaybackSampleRate (sampleRate);
  modeBank.prepare (sampleRate);
  exciter.prepare  (sampleRate);
  adsr.setSampleRate (sampleRate);
  adsr.setParameters (juce::ADSR::Parameters { 0.010f, 0.0f, 1.0f, 0.200f });
  ```
  **CRITICAL: `setSampleRate` MUST be called BEFORE `setParameters`** (JUCE 8 ADSR contract — OQ#1, juce_ADSR.h:115–119).
- `startNote(int midiNote, float /*velocity*/, juce::SynthesiserSound*, int currentPitchWheelPos)`:
  - Capture `pitchWheelValue = currentPitchWheelPos;`
  - Compute `pitchBendSemitones = ((float(pwv) - 8192.0f) / 8192.0f) * 2.0f;`
  - Compute `currentFrequencyBase = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNote));` (plain MIDI, NOT TuningEngine — locked Q2)
  - Compute `fBent = currentFrequencyBase * std::pow(2.0f, pitchBendSemitones / 12.0f);`
  - Call `modeBank.setFundamental(fBent); exciter.start(); adsr.noteOn();`
- `stopNote(float, bool allowTailOff)`:
  - `if (allowTailOff) adsr.noteOff();`
  - `else { clearCurrentNote(); adsr.reset(); modeBank.reset(); exciter.reset(); currentFrequencyBase = 0.0f; }`
- `pitchWheelMoved(int newPitchWheelValue)`:
  - Recompute `pitchBendSemitones`
  - **Guard:** `if (currentFrequencyBase > 0.0f) modeBank.setFundamental(fBent);` (prevents recompute before any note-on)
- `controllerMoved(int, int)` — empty body (Phase 2.3 wires CC2 → breath)
- `renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)`:
  - Early return if `! adsr.isActive()`
  - Per-sample loop:
    ```cpp
    const int numChannels = outputBuffer.getNumChannels();
    for (int i = 0; i < numSamples; ++i)
    {
        const float ex    = exciter.getNextSample();
        const float voice = modeBank.processSample (ex);
        const float env   = adsr.getNextSample();
        const float out   = voice * env;

        for (int ch = numChannels; --ch >= 0;)
            outputBuffer.addSample (ch, startSample + i, out);

        if (! adsr.isActive())
        {
            clearCurrentNote();
            modeBank.reset();
            exciter.reset();
            currentFrequencyBase = 0.0f;
            return;
        }
    }
    ```
  - **MUST use `addSample` (sum, not overwrite)** — host's `processBlock` clears the buffer at PluginProcessor.cpp:165 before `synthesiser.renderNextBlock`; `Synthesiser::renderVoices` does NOT zero (OQ#2 / D3).
  - **MUST honour `[startSample, startSample + numSamples)` sub-range** — sub-blocking between MIDI events.

**Constraints:**
- No APVTS reads, no `tuningEngine->getFrequency()` call, no `pendingTuningSource->...` access (locked Q2)
- No `new`, `make_unique`, `make_shared`, `push_back`, `resize`, or `malloc` anywhere in this file
- All audio-thread paths `noexcept`-compatible
- Voice exit path: `clearCurrentNote()` → `modeBank.reset()` → `exciter.reset()` → `currentFrequencyBase = 0.0f` → `return`

**Depends on:** Task 3

---

### Task 5 — Extend `Source/PluginProcessor.cpp::prepareToPlay` (iterate voices, call `BassoonVoice::prepareToPlay`)

**Files:**
- `plugins/O-Bassoon/Source/PluginProcessor.cpp` (MODIFY — `prepareToPlay` only)

**Body:** Insert per-voice prepare loop INSIDE the existing `prepareToPlay` (currently at line 134), AFTER `synthesiser.setCurrentPlaybackSampleRate(sampleRate);`:

```cpp
void OBassoonAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Modal synthesis is feed-forward; latency = 0 — do NOT call setLatencySamples.
    // (getLatencySamples is non-virtual in JUCE 8; default returns 0.)
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);

    for (int v = 0; v < synthesiser.getNumVoices(); ++v)
        if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (v)))
            bv->prepareToPlay (sampleRate, samplesPerBlock);
}
```

**Constraints:**
- Change the `int /*samplesPerBlock*/` parameter to `int samplesPerBlock` (drop the comment-out — it's now used)
- DO NOT touch `releaseResources`, `processBlock`, `isBusesLayoutSupported`, parameter layout, voice/sound construction, or any other method
- DO NOT change the NE drain ordering at PluginProcessor.cpp:170 (Stage 1 contract — drain BEFORE `renderNextBlock`)

**Depends on:** Task 4

---

### Task 6 — Update `plugins/O-Bassoon/CMakeLists.txt` (add ModeBank + Exciter to source list)

**Files:**
- `plugins/O-Bassoon/CMakeLists.txt` (MODIFY)

**Body:** Inside the existing `target_sources(O-Bassoon PRIVATE …)` block (currently lines ~24–34), add four lines after `Source/BassoonVoice.cpp`:

```cmake
        Source/BassoonVoice.h
        Source/BassoonVoice.cpp
        Source/ModeBank.h            # Phase 2.1
        Source/ModeBank.cpp          # Phase 2.1
        Source/Exciter.h             # Phase 2.1
        Source/Exciter.cpp           # Phase 2.1
```

**Constraints:**
- DO NOT duplicate the `target_sources` call — modify the existing one
- DO NOT touch `juce_add_plugin` flags (`IS_SYNTH`, `NEEDS_MIDI_INPUT`, `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING`, `PLUGIN_CODE OBsn`)
- DO NOT touch `ouaricon_add_module(O-Bassoon note-expression)` line
- DO NOT touch include paths, `target_link_libraries`, `juce_generate_juce_header`, or licensing block
- DO NOT add `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` here — it's already set in Stage 1 via the `juce_add_plugin` flags (`NEEDS_WEBVIEW2 TRUE`)

**Depends on:** Tasks 1, 2

---

### Task 7 — Source reference bassoon C3 sustain (VSCO-2-CE / CC0) + LICENSE.md sidecar

**Files (NEW):**
- `plugins/O-Bassoon/research/reference-recordings/bassoon-c3-sustain-v1.wav`
- `plugins/O-Bassoon/research/reference-recordings/bassoon-c3-sustain-v2.wav`
- `plugins/O-Bassoon/research/reference-recordings/LICENSE.md`
- `plugins/O-Bassoon/research/reference-recordings/README.md`

**Body:** Per RESEARCH.md §1 OQ#7 + §5 audition checklist:

```bash
mkdir -p plugins/O-Bassoon/research/reference-recordings
cd plugins/O-Bassoon/research/reference-recordings

curl -fLO https://raw.githubusercontent.com/sgossner/VSCO-2-CE/master/Woodwinds/Bassoon/sus/PSBassoon_C3_v1_1.wav
curl -fLO https://raw.githubusercontent.com/sgossner/VSCO-2-CE/master/Woodwinds/Bassoon/sus/PSBassoon_C3_v2_1.wav

mv PSBassoon_C3_v1_1.wav bassoon-c3-sustain-v1.wav
mv PSBassoon_C3_v2_1.wav bassoon-c3-sustain-v2.wav

afinfo bassoon-c3-sustain-v1.wav   # validate WAV format
afinfo bassoon-c3-sustain-v2.wav
```

Audition checklist (RESEARCH §5) — at least 4 of 7 items mandatory before commit:
1. [ ] curl HTTP 200 + file size > 100 KB
2. [ ] `afinfo` reports valid WAV
3. [ ] Plays without artefacts in Logic / QuickLook
4. [ ] Tuner-measured fundamental matches filename octave (D4 in RESEARCH §4 — VSCO octave convention may differ; rename file if needed)
5. [ ] Spectrum (SPAN) shows clean harmonic structure with peak in 450–500 Hz formant region
6. [ ] `LICENSE.md` written with VSCO-2-CE provenance + CC0 attribution
7. [ ] `README.md` documents source, license, intended use, audition notes

**Fallback:** If VSCO-2-CE 404s or audition fails, fall back to U Iowa MIS bassoon archive (https://theremin.music.uiowa.edu/MIS.html) — RESEARCH §1 OQ#7 backup #1.

**Constraints:**
- This task does NOT block Phase 2.1 verification (verification uses tuner + SPAN baseline, not vs.-recording A/B)
- Reference recording is archived for Phase 2.2 kickoff
- LICENSE.md template per RESEARCH §1 OQ#7

**Depends on:** none (parallel-eligible with builds)

---

### Task 8 — Build, install, run 10-item Gate 1 PASS verification

**Files:** none (build artefacts only)

**Body:** Per CONTEXT Q7 (10-item Gate 1 bar). Execute:

```bash
# Build
cd build
ninja O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone

# Clear macOS AU cache (mandatory — CLAUDE.md project guidelines)
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache

# Install fresh
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Bassoon-dev.component
cp -R plugins/O-Bassoon/O-Bassoon_artefacts/Release/VST3/O-Bassoon-dev.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R plugins/O-Bassoon/O-Bassoon_artefacts/Release/AU/O-Bassoon-dev.component ~/Library/Audio/Plug-Ins/Components/

# Validate
auval -v aumu OBsn OuDv
pluginval --strictness-level 5 --validate ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3
```

Then in Logic Pro AU at 48 k / 256-sample buffer, sustain a single MIDI C3 and verify:

| # | Gate 1 Item | Pass criterion | Evidence |
|---|-------------|----------------|----------|
| 1 | Sustained tone at correct pitch | Tuner reads ±2 cents on A4=440 Hz | tuner screenshot |
| 2 | No clicks at note-on/note-off | Subjective listen + zoom-in waveform inspection | DAW screenshot |
| 3 | No NaN/inf in render | Output remains finite, spectrum analyzer shows no DC spike or runaway | SPAN screenshot |
| 4 | > 10 s sustain without drift | Hold note, watch level meter — amplitude does not decay below -6 dB or grow > +6 dB | DAW screenshot at 12 s mark |
| 5 | 1-voice CPU < 5 % @ 48 k / 256 | Logic Performance Meter "Process" bar < 5 % | Performance meter screenshot |
| 6 | Plays C1–C6 without instability | Sweep MIDI 24→84, no pitch detuning at extremes, no resonator breakdown | DAW screenshot of scale region |
| 7 | `auval -v aumu OBsn OuDv` SUCCESS | exit code 0; final line `AU VALIDATION SUCCEEDED.` | terminal log |
| 8 | `pluginval --strictness-level 5` SUCCESS | exit code 0; final line `*** ALL TESTS COMPLETED SUCCESSFULLY ***` | terminal log |
| 9 | Logic AU manual smoke | Hold C3, play scale C1→C6, confirm pitch + sustain audible (subjective — will NOT sound bassoon-like; placeholder partials) | DAW recording |
| 10 | DAW spectrum baseline capture | Render sustained C3, screenshot SPAN spectrum at `plugins/O-Bassoon/research/reference-recordings/phase-2.1-baseline-c3-spectrum.png` (settings: 8192 block, Hann, Stereo, 4.5 dB/oct slope, Infinite avg) | PNG file committed |

**Pre-commit `grep` checks** (mandatory RT-safety scan):

```bash
# Must return zero matches in audio-thread sources
grep -nE '\bnew\b|make_unique|make_shared|push_back|resize|malloc' \
    plugins/O-Bassoon/Source/ModeBank.h \
    plugins/O-Bassoon/Source/ModeBank.cpp \
    plugins/O-Bassoon/Source/Exciter.h \
    plugins/O-Bassoon/Source/Exciter.cpp \
    plugins/O-Bassoon/Source/BassoonVoice.cpp
```

(Constructor `new BassoonVoice()` in `PluginProcessor.cpp:120` is permitted — non-audio-thread, run once at construction.)

**Constraints:**
- All 10 items must PASS — no partial gates, no "we'll fix it next phase" deferrals
- If item 5 (CPU) > 5 %: fall back to 8 modes per voice (ROADMAP "Fallback 1") and file an ARCHITECTURE deviation note before commit
- If items 7–8 fail: investigate before retrying (do NOT skip pluginval/auval — these catch RT-safety regressions that are dormant under the silent stub)
- Item 10 PNG goes in `research/reference-recordings/` not in `Source/` — keeps the Source/ tree code-only

**Depends on:** Tasks 1–6 (build dependencies); Task 7 (LICENSE in same commit)

---

### Task 9 — Atomic commit + planning artefact updates

**Files (UPDATE):**
- `plugins/O-Bassoon/.planning/STATUS.md` (mark Phase 2.1 complete; advance next-action to Phase 2.2 discuss)
- `plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md` (NEW — 10-item Gate 1 evidence summary)
- `plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md` (NEW — Gate 1 verification log with screenshots referenced)

**Files (CREATE — new):**
- All Phase 2.1 source files (ModeBank, Exciter, BassoonVoice updates)
- `plugins/O-Bassoon/research/reference-recordings/{bassoon-c3-sustain-v1.wav, bassoon-c3-sustain-v2.wav, LICENSE.md, README.md, phase-2.1-baseline-c3-spectrum.png}`

**Commit subject (locked Q5):**
```
feat(O-Bassoon): Phase 2.1 first audio - Gate 1 PASS
```

**Commit body (template):**
```
Replace silent voice stub with modal-synthesis first audio.

- ModeBank: 16-mode parallel pole-only resonator bank, integer harmonics
  placeholder, T60 = 2.5s (mode 0) → 0.25s (mode 15). Direct-form pole-only
  biquads with std::isfinite NaN guard (lifted from O-Formant pattern).
  Nyquist mute policy: f_k > 0.45 * fs zeroes mode amplitude. 1/N output
  scaling for headroom (Phase 2.3 replaces with output_gain APVTS).
- Exciter: 5 ms half-sine windowed exp decay, normalised peak 1.0,
  pre-baked at prepare(). Class-level std::array storage (per-voice,
  no static-shared). Allocation-free start()/getNextSample()/reset().
- BassoonVoice: per-sample inner loop (excite -> resonate -> envelope ->
  stereo addSample), juce::ADSR with hardcoded {10ms, 0, 1.0, 200ms},
  early return on !adsr.isActive() with full state reset. Pitch-bend
  wired (note-on + pitchWheelMoved trigger full coefficient recompute).
  No APVTS reads, no TuningEngine call (strict ROADMAP minimal wiring;
  Phase 2.3-2.4 wires expression + NE/MPE).
- PluginProcessor::prepareToPlay extended to dispatch BassoonVoice::
  prepareToPlay per voice (juce::SynthesiserVoice has no virtual
  prepareToPlay in JUCE 8; mirrors O-Wind/O-Lyrica pattern).
- CMakeLists: ModeBank.{h,cpp} + Exciter.{h,cpp} added to target_sources.

Verification: 10-item Gate 1 bar PASS — pitch ±2c (C3 / 130.81 Hz), no
clicks, no NaN, > 10 s sustain stable, 1-voice CPU < 5%, C1-C6 sweep
clean, auval SUCCESS, pluginval --strictness-level 5 SUCCESS, Logic AU
manual smoke PASS, SPAN baseline captured at research/reference-
recordings/phase-2.1-baseline-c3-spectrum.png.

Reference recording archived: VSCO-2-CE bassoon C3 (CC0) at research/
reference-recordings/ for Phase 2.2 A/B listening loop.
```

**Constraints:**
- **Single commit** — all source edits + reference recording + planning artefacts in one atomic unit. No staged commits, no separate "feat: source" + "docs: planning" split.
- DO NOT use `--no-verify` (run hooks)
- DO NOT amend a previous commit; this is a fresh commit on `main`
- Co-author trailer per CLAUDE.md commit protocol

**Depends on:** Task 8 (Gate 1 PASS) — only commit on green

---

## Files Created / Modified Summary

| Type | Path | Task |
|------|------|------|
| NEW  | `plugins/O-Bassoon/Source/ModeBank.h` | 1 |
| NEW  | `plugins/O-Bassoon/Source/ModeBank.cpp` | 1 |
| NEW  | `plugins/O-Bassoon/Source/Exciter.h` | 2 |
| NEW  | `plugins/O-Bassoon/Source/Exciter.cpp` | 2 |
| MOD  | `plugins/O-Bassoon/Source/BassoonVoice.h` | 3 |
| MOD  | `plugins/O-Bassoon/Source/BassoonVoice.cpp` | 4 |
| MOD  | `plugins/O-Bassoon/Source/PluginProcessor.cpp` (`prepareToPlay` only) | 5 |
| MOD  | `plugins/O-Bassoon/CMakeLists.txt` (`target_sources` block only) | 6 |
| NEW  | `plugins/O-Bassoon/research/reference-recordings/bassoon-c3-sustain-v1.wav` | 7 |
| NEW  | `plugins/O-Bassoon/research/reference-recordings/bassoon-c3-sustain-v2.wav` | 7 |
| NEW  | `plugins/O-Bassoon/research/reference-recordings/LICENSE.md` | 7 |
| NEW  | `plugins/O-Bassoon/research/reference-recordings/README.md` | 7 |
| NEW  | `plugins/O-Bassoon/research/reference-recordings/phase-2.1-baseline-c3-spectrum.png` | 8 |
| MOD  | `plugins/O-Bassoon/.planning/STATUS.md` | 9 |
| NEW  | `plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md` | 9 |
| NEW  | `plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md` | 9 |

**Total: 12 new + 4 modified = 16 file operations in one atomic commit.**

---

## Dependencies (DAG)

```
Task 1 (ModeBank)  ──┐
Task 2 (Exciter)   ──┴──> Task 3 (Voice.h) ──> Task 4 (Voice.cpp) ──> Task 5 (Processor.cpp)
                                                                          │
Task 6 (CMake)  ──── parallel after 1, 2 ─────────────────────────────────┤
                                                                          │
Task 7 (Reference)  ─── parallel any time (no code dependency) ───────────┤
                                                                          ▼
                                                                  Task 8 (Build + Gate 1 verify)
                                                                          │
                                                                          ▼
                                                                  Task 9 (Atomic commit)
```

Critical path: 1/2 → 3 → 4 → 5 → 8 → 9. ETA estimate: 3–5 hours total (most time in Task 8 verification — Logic project setup, screenshots, scale sweep, pluginval).

---

## Success Criteria

**Gate 1 PASS bar (must be 10/10 — locked CONTEXT Q7):**

- [ ] (1) Sustained tone at correct pitch ±2 cents
- [ ] (2) No clicks at note-on/note-off
- [ ] (3) No NaN/inf in render
- [ ] (4) > 10 s sustain without amplitude drift
- [ ] (5) 1-voice CPU < 5 % @ 48 k / 256
- [ ] (6) C1–C6 sweep without resonator instability
- [ ] (7) `auval -v aumu OBsn OuDv` SUCCESS
- [ ] (8) `pluginval --strictness-level 5` SUCCESS
- [ ] (9) Logic AU manual smoke PASS
- [ ] (10) SPAN baseline spectrum PNG captured + archived

**Pre-commit invariants:**

- [ ] `grep` for `\bnew\b|make_unique|make_shared|push_back|resize|malloc` in audio-thread sources returns zero matches
- [ ] No APVTS reads in `BassoonVoice::*` (verify by `grep -n "parameters" plugins/O-Bassoon/Source/BassoonVoice.cpp` returns zero functional matches — only the unread member declaration)
- [ ] No `tuningEngine->` or `pendingTuningSource->` dereferences in `BassoonVoice.cpp`
- [ ] `synthesiser.renderNextBlock` ordering at `PluginProcessor.cpp` AFTER `vst3Extensions.drainAndUpdate()` (Stage 1 contract — DO NOT reorder)
- [ ] `setLatencySamples` NOT called in `prepareToPlay` (modal synthesis is feed-forward; latency stays at 0)
- [ ] Reference recording sourced + LICENSE.md written + README.md written (audition checklist 6/7 minimum)

**Stage-1 invariants preserved (regression check):**

- [ ] All 10 APVTS parameters present (IDs, ranges, defaults unchanged)
- [ ] `juce::Synthesiser` still has 16 voices (`for (int i = 0; i < 16; ++i)` in ctor)
- [ ] `vst3Extensions.drainAndUpdate()` called BEFORE `synthesiser.renderNextBlock` (PluginProcessor.cpp:170 → :174 ordering)
- [ ] `BassoonSound::appliesToNote/Channel` returns `true` for all input
- [ ] `juce_generate_juce_header` AFTER `target_link_libraries` in CMakeLists
- [ ] `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `PLUGIN_CODE OBsn` flags unchanged

---

## Risks (Phase 2.1-specific, carried from CONTEXT + RESEARCH)

| # | Risk | Mitigation | Verify path |
|---|------|------------|-------------|
| 1 | Mode-bank IIR instability at long T60 | TDF-style state, `ScopedNoDenormals`, `modeBank.reset()` on note exit, `std::isfinite` per-mode guard | Gate 1 item 4 (10s sustain) + item 3 (no NaN) |
| 2 | High-frequency modes near Nyquist (D5: severity downgraded — Phase 2.1 placeholder ratios stay below 0.45·fs even at C6) | Nyquist mute policy `f_k > 0.45·fs → amp=0` (OQ#5) | Gate 1 item 6 (C6 sweep clean) |
| 3 | Click on note-on (impulse audible standalone) | 5 ms half-sine × exp decay shape (smooth zero-crosses); 10 ms ADSR attack covers impulse fully | Gate 1 item 2 (no clicks) |
| 4 | CPU > 5 % | 16 biquads × 48k = ~0.05 % CPU mathematically; if Logic shows > 5 %, fallback to 8 modes/voice + ARCHITECTURE deviation note | Gate 1 item 5 |
| 5 | Allocation in startNote/processBlock | Pre-commit `grep` scan; pluginval --strictness-level 5 catches at runtime | Gate 1 item 8 + grep |
| 6 | Reference recording sourcing falls through | VSCO-2-CE primary; U Iowa MIS / Philharmonia backups (RESEARCH OQ#7) | Audition checklist (RESEARCH §5) |
| 7 | JUCE Synthesiser sub-buffer zeroing assumption (D3: closed — host's `buffer.clear()` at PluginProcessor.cpp:165 is the canonical zero point; `Synthesiser::renderVoices` does NOT zero) | Pattern already in place; `addSample` is correct family idiom | Gate 1 items 1–9 (audio works) |
| 8 | Pitch-bend semantics (raw 14-bit `[0, 16383]` not normalized — confirmed OQ#9) | Formula locked per O-Wind precedent | Gate 1 item 9 (manual pitch-wheel sweep on held C3) |

---

## Out of Scope (deferred per ROADMAP)

- Bassoon-tuned partial ratios + formant-Gaussian amplitude shaping + `tone` parameter wiring → **Phase 2.2**
- A/B listening loop vs. reference recording → **Phase 2.2**
- APVTS reads of `attack_time`, `release_time`, `breath`, `output_gain` + `juce::SmoothedValue` ramps → **Phase 2.3**
- Vibrato LFO + onset envelope → **Phase 2.3**
- CC2 → breath routing → **Phase 2.3**
- Voice manager / `voice_count` cap / oldest-note stealing → **Phase 2.4**
- `attack_character` morph (soft ↔ tongued shape crossfade) + sustain noise + velocity bias → **Phase 2.4**
- VST3 Note Expression consumption per-voice (`applyPendingTuning`) → **Phase 2.4**
- MPE pitch-bend per-voice multiplier → **Phase 2.4**
- TuningEngine `getFrequency()` call in `startNote` → **Phase 2.4**
- pluginval `--strictness 10` + Windows VST3 build → **Stage 4**
- Dorico parity testing → **Stage 4**
- GUI (any visual change beyond `juce::GenericAudioProcessorEditor`) → **Stage 3**

---

## Next Phase

Ready for: **execute** phase — `/plugin-execute O-Bassoon 2-dsp`

Execute-phase consumes:
- This PLAN.md (task list + Gate 1 bar)
- RESEARCH.md §3 implementation skeletons (verbatim source code)
- CONTEXT.md constraints (locked decisions Q1–Q8)

Execute-phase performs Tasks 1–9 in dependency order. Atomic commit lands on Gate 1 PASS only.

---

## Audit Trail

**rev-1 (this document, 2026-04-27):** Phase 2.1 plan phase. Single Wave (no parallel sub-waves), 9 tasks (5 file edits + 1 CMakeLists edit + 1 reference-download + 1 build/verify + 1 atomic commit). 16 file operations total. Implementation skeletons consumed verbatim from RESEARCH.md §3. 10-item Gate 1 PASS bar pinned per CONTEXT Q7. Pre-commit grep + invariant checks listed. 8 risks carried from CONTEXT + RESEARCH with mitigation + verification paths. Out-of-scope register matches ROADMAP Phase 2.1 boundary exactly.

**Inherited verbatim from CONTEXT (rev-1) + RESEARCH (rev-1):**
- Cycle scope = Phase 2.1 only (Q1)
- Strict ROADMAP minimal wiring — no APVTS reads, plain MIDI for f_base (Q2)
- Coefficient cadence: note-on + pitch-bend only (Q3)
- Reference recording sourced during Phase 2.1 (Q4)
- Single atomic commit on Gate 1 PASS (Q5)
- DAW + tuner verification, no CLI harness (Q6)
- 10-item Gate 1 bar (Q7)
- Centered equal L+R per-sample voice write (Q8)
- ModeBank pole-only specialisation (RESEARCH §2 — re-implement, not lift O-Formant)
- TDF-I direct-form for pole-only resonator with `std::isfinite` guard (RESEARCH §2)
- 5 ms half-sine × exp impulse, peak-normalized (RESEARCH OQ#4)
- Nyquist mute at `f_k > 0.45·fs` (RESEARCH OQ#5)
- Pitch-bend ±2 semitones, raw 14-bit value (RESEARCH OQ#9)
- VSCO-2-CE bassoon C3 (CC0) primary; U Iowa MIS backup (RESEARCH OQ#7)
- Voxengo SPAN for spectrum baseline (RESEARCH OQ#10)
- Logic Performance Meter "Process" bar for CPU verification (RESEARCH OQ#8)
- `BassoonVoice::prepareToPlay` non-virtual custom method (RESEARCH D1)
- `1/NUM_MODES` per-voice scaling (RESEARCH D2 — placeholder until Phase 2.3 output_gain)

---

## rev-2 — Phase 2.2 Plan (2026-04-27)

**Cycle:** Phase 2.2 — Bassoon Spectral Tuning + Tone Control. Single GSD cycle. Working tree starts from Phase 2.1 atomic commit `d1b3370` on `main`.

**Inputs (consumed verbatim):**
- `stages/2-dsp/CONTEXT.md` rev-2 addendum — 9 user-confirmed approach decisions (Q1–Q9-rev-2) + 4 derived locks
- `stages/2-dsp/RESEARCH.md` rev-2 addendum — 10 OQ resolutions + §3 implementation skeletons (ModeBank.{h,cpp}, BassoonVoice.{h,cpp}, PluginProcessor.{h,cpp}) lifted as task body content
- `research/ARCHITECTURE.md` §"Bassoon Partial Table" + §"Tone / Brightness Control" — partial ratios, formant constants (475 Hz / 200 Hz BW), T60 mix function `mix(0.3, 1.5, tone)` consumed verbatim into ModeBank

**Atomic commit subject (locked CONTEXT-rev-2 Q9-rev-2):** `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`

### Goal (rev-2)

Replace Phase 2.1's placeholder integer-harmonic ratios + flat amplitudes with **bassoon-tuned near-integer ratios + first-formant-Gaussian × 1/k roll-off amplitude shaping**, and wire the **`tone` APVTS parameter** (per-mode T60 scaling for upper modes k > 4, zero-indexed → modes 5–15) end-to-end via processor-level `juce::SmoothedValue<float, ValueSmoothingTypes::Linear>` (50 ms ramp) + throttled-epsilon dispatch (epsilon = 0.001).

**Gate 2 PASS** = ear-only A/B against archived `bassoon-c3-sustain-v1.wav` at held C3 sounds "same neighborhood" bassoon-like + Logic Channel EQ Analyzer (Pre-EQ mode, all bands disabled) overlay shows peak in 400-600 Hz region + tone sweep across [0, 1] is click/zipper/NaN-free + 8-voice simultaneous-hold CPU in Logic-AU stays under 20 % + Phase 2.1 invariants regress-clean.

### Wave Layout (rev-2)

**Single Wave** (gate-first; no parallel sub-waves). Sequence: ModeBank.h → ModeBank.cpp → BassoonVoice.{h,cpp} → PluginProcessor.h → PluginProcessor.cpp → ARCHITECTURE.md backfill → build/install/static-checks → manual Gate 2 verify → atomic commit.

Tasks 1–6 are file edits; tasks 7–9 are verification + commit. **No CMakeLists.txt edits at Phase 2.2** — `ModeBank.{h,cpp}`, `BassoonVoice.{h,cpp}`, `PluginProcessor.{h,cpp}` are already in `target_sources` from Phase 2.1.

---

### Tasks (rev-2)

#### 1. [ ] Extend `ModeBank.h` with rev-2 surface (constants + cached members + new method signatures)

**Files:** `plugins/O-Bassoon/Source/ModeBank.h` (MODIFY)

**Depends on:** none

**Source:** RESEARCH-rev-2 §3 "Source/ModeBank.h (rev-2 deltas to Phase 2.1)" — lift verbatim.

**Concrete changes:**

- Add public constants after `NUM_MODES = 16`:
  ```cpp
  static constexpr float FORMANT_F1 = 475.0f;   // Hz, first formant centre
  static constexpr float FORMANT_BW = 200.0f;   // Hz, formant bandwidth
  ```
- Replace `PARTIAL_RATIOS` initialiser with bassoon-tuned near-integer ratios (verbatim from ARCHITECTURE.md §"Bassoon Partial Table"):
  ```cpp
  { 1.000f,  2.005f,  3.010f,  4.018f,  5.024f,  6.032f,  7.041f,  8.052f,
    9.064f, 10.078f, 11.092f, 12.108f, 13.125f, 14.144f, 15.164f, 16.186f }
  ```
  Update the trailing comment to past-tense ("rev-2: bassoon-tuned near-integer ratios; source ARCHITECTURE.md §Bassoon Partial Table — author-curated synthesis per RESEARCH §1 OQ#3-rev-2").
- Replace inline stub `void setTone (float /*tone01*/) noexcept {}` with real declarations:
  ```cpp
  void setTone         (float tone01) noexcept;
  void applyToneChange () noexcept;
  ```
  Both implemented in ModeBank.cpp (no inline body).
- Add private static helper declaration:
  ```cpp
  static float computeModeAmplitude (int k, float f0) noexcept;
  ```
- Extend `ModeBiquad` struct: add cached members after the existing state vars:
  ```cpp
  float cosTheta = 0.0f;   // rev-2: cached cos(theta_k) for tone-only recompute
  float amp      = 0.0f;   // rev-2: cached formant-Gaussian × roll-off
  ```
- Add private member after `currentSampleRate`:
  ```cpp
  float currentTone = 0.5f;   // matches APVTS default (D2-rev-2)
  ```

**Invariants preserved:** `NYQ_RATIO`, `BASE_T60`, `ModeBiquad::processSample` body + isfinite NaN guard, `ModeBiquad::reset()`, APVTS-free header — all unchanged. Sentinel `currentTone = 0.5f` initialiser ensures no discontinuity on first APVTS read.

---

#### 2. [ ] Implement `ModeBank.cpp` rev-2 (setFundamental rev-2 + setTone + applyToneChange + computeModeAmplitude + 1/8 scaler)

**Files:** `plugins/O-Bassoon/Source/ModeBank.cpp` (MODIFY)

**Depends on:** Task 1

**Source:** RESEARCH-rev-2 §3 "Source/ModeBank.cpp (rev-2 implementation)" — lift verbatim.

**Concrete changes:**

- `prepare(sampleRate)` — body unchanged (calls `reset()`).
- `setFundamental(f0)` — full rewrite per skeleton:
  - Per-mode loop computes `f_k = f0 * PARTIAL_RATIOS[k]`.
  - **Mute path:** if `f_k > nyqLimit || f_k <= 0` → zero `b0/a1/a2` AND `cosTheta` AND `amp` (so `applyToneChange` skips muted modes via `m.amp == 0.0f` test).
  - **Active path:** `theta = twoPi * f_k / fs`; `cosT = cos(theta)`; `amp_k = computeModeAmplitude(k, f0)`; `toneScale = (k > 4) ? juce::jmap(currentTone, 0.0f, 1.0f, 0.3f, 1.5f) : 1.0f`; `tau_k = (BASE_T60[k] * toneScale) / 6.91f`; `R_k = exp(-1 / (tau_k * fs))`. Cache `cosTheta = cosT`, `amp = amp_k`. Set biquad coefs `b0 = (1 - R_k) * amp_k`, `a1 = -2 * R_k * cosT`, `a2 = R_k * R_k`.
- `setTone(tone01)`:
  ```cpp
  currentTone = juce::jlimit (0.0f, 1.0f, tone01);
  ```
- `applyToneChange()`:
  - Compute `toneScale = juce::jmap(currentTone, 0.0f, 1.0f, 0.3f, 1.5f)` once.
  - Loop `for (int k = 5; k < NUM_MODES; ++k)` (zero-indexed; modes 5–15 only — D4-rev-2 lock).
  - Skip muted: `if (m.amp == 0.0f) continue;`
  - For non-muted modes: `tau_k = (BASE_T60[k] * toneScale) / 6.91f`; `R_k = exp(-1 / (tau_k * fs))`; refresh `b0 = (1 - R_k) * m.amp`, `a1 = -2 * R_k * m.cosTheta`, `a2 = R_k * R_k`.
  - **Modes 0–4 are tone-invariant — DO NOT touch.**
- `computeModeAmplitude(k, f0)` — new private static:
  ```cpp
  const float f_k           = f0 * PARTIAL_RATIOS[k];
  const float dist          = (f_k - FORMANT_F1) / FORMANT_BW;
  const float formantWeight = std::exp (-0.5f * dist * dist);
  const float rollOff       = 1.0f / (1.0f + 0.5f * static_cast<float>(k));
  return formantWeight * rollOff;
  ```
- `processSample(excitation)` — relax headroom scaler from `1.0f / NUM_MODES` (= 1/16) to `1.0f / 8.0f` (locked OQ#5-rev-2; +6 dB lift). Add the in-cycle-tuning-constant comment from skeleton (referencing OQ#5-rev-2 + Phase 2.3 output_gain handoff).
- `reset()` — body unchanged.

**Invariants preserved:** zero allocation in any method (pre-commit grep verifies — Task 7); `ModeBank` still has zero APVTS reads; `noexcept` annotations on `setTone`, `applyToneChange`, `processSample`, `reset` preserved.

---

#### 3. [ ] Add `setTone` to `BassoonVoice.{h,cpp}` (single public method, single .cpp body)

**Files:**
- `plugins/O-Bassoon/Source/BassoonVoice.h` (MODIFY — one new public method declaration)
- `plugins/O-Bassoon/Source/BassoonVoice.cpp` (MODIFY — one new method body)

**Depends on:** Task 1 (ModeBank.h `setTone` + `applyToneChange` declarations must exist)

**Source:** RESEARCH-rev-2 §3 "Source/BassoonVoice.h (rev-2 deltas)" + "Source/BassoonVoice.cpp (rev-2 single addition)".

**Concrete changes:**

- `BassoonVoice.h` — add public method declaration after `controllerMoved` / before `setAPVTS` cluster:
  ```cpp
  void setTone (float tone01) noexcept;   // Phase 2.2: dispatched from processor (throttled)
  ```
- `BassoonVoice.cpp` — add method body (location: after `controllerMoved`, before wiring-setter cluster):
  ```cpp
  void BassoonVoice::setTone (float tone01) noexcept
  {
      modeBank.setTone (tone01);
      modeBank.applyToneChange();   // throttle gate is at processor dispatch site, not here
  }
  ```

**Invariants preserved:** `startNote`, `stopNote`, `pitchWheelMoved`, `controllerMoved`, `renderNextBlock`, `prepareToPlay`, `canPlaySound` all unchanged. Per-sample render loop (`exciter → modeBank → adsr → addSample(L) + addSample(R)`) unchanged. Voice still reads zero APVTS parameters directly (locked Q2-rev-2 strict-ROADMAP). No new private members in BassoonVoice — tone state lives in `modeBank.currentTone`. `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` macro intact.

---

#### 4. [ ] Add `toneSmoother` + `lastDispatchedTone` to `PluginProcessor.h`

**Files:** `plugins/O-Bassoon/Source/PluginProcessor.h` (MODIFY)

**Depends on:** none

**Source:** RESEARCH-rev-2 §3 "Source/PluginProcessor.h (rev-2 deltas)".

**Concrete change:** Append after `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;` (PluginProcessor.h:63) inside `private:`:

```cpp

    // Phase 2.2: tone smoother + dispatch throttle (CONTEXT-rev-2 Q3/Q4-rev-2).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> toneSmoother;
    float                                                          lastDispatchedTone = -1.0f;
```

Sentinel `lastDispatchedTone = -1.0f` ensures the first dispatch fires unconditionally (any valid `tone ∈ [0, 1]` differs from -1 by > 0.001).

**Invariants preserved:** `parameters`, `synthesiser`, `tuningEngine`, `vst3Extensions` untouched. Public surface untouched. `JuceHeader.h` (line 15) already provides `juce::SmoothedValue`.

---

#### 5. [ ] Wire smoother in `prepareToPlay` + dispatch loop in `processBlock` (PluginProcessor.cpp)

**Files:** `plugins/O-Bassoon/Source/PluginProcessor.cpp` (MODIFY)

**Depends on:** Task 3 (BassoonVoice::setTone must exist) + Task 4 (header members must exist)

**Source:** RESEARCH-rev-2 §3 "Source/PluginProcessor.cpp (rev-2 deltas)".

**Concrete changes:**

**(a) `prepareToPlay`** (current body lines 134–148): after the existing per-voice prepareToPlay dispatch loop (line ~146 `bv->prepareToPlay (sampleRate, samplesPerBlock);`), add two lines:
```cpp
toneSmoother.reset (sampleRate, 0.050);   // 50 ms ramp (CONTEXT-rev-2 Q3-rev-2)
lastDispatchedTone = -1.0f;               // force first dispatch on next processBlock
```

**(b) `processBlock`** (current body lines 167–183): insert tone advance + dispatch BEFORE the existing `vst3Extensions.drainAndUpdate()` call (line 178). Insert verbatim:

```cpp
const int numSamples = buffer.getNumSamples();
const float toneTarget = parameters.getRawParameterValue ("tone")->load();
toneSmoother.setTargetValue (toneTarget);
const float toneSmoothed = toneSmoother.skip (juce::jmax (0, numSamples));

if (std::abs (toneSmoothed - lastDispatchedTone) > 0.001f)
{
    for (int v = 0; v < synthesiser.getNumVoices(); ++v)
        if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (v)))
            bv->setTone (toneSmoothed);
    lastDispatchedTone = toneSmoothed;
}
```

Update the existing `synthesiser.renderNextBlock(...)` call to use the local `numSamples` (not `buffer.getNumSamples()`) for consistency.

**New ordering invariant** (Phase 2.2 extends Phase 2.1 chain):

1. `juce::ScopedNoDenormals noDenormals;` (existing — first line)
2. `buffer.clear()` (existing — line 173)
3. **NEW:** tone smoother advance + voice dispatch
4. `vst3Extensions.drainAndUpdate()` (existing — was line 178)
5. `synthesiser.renderNextBlock (buffer, midiMessages, 0, numSamples)` (existing — was line 182)

The tone dispatch sits BEFORE the NE drain — same principle as the Phase 2.1 NE-drain-BEFORE-renderNextBlock invariant: voice state is fully up-to-date when JUCE iterates voice events.

**Invariants preserved:** `juce::ScopedNoDenormals noDenormals;` first line of processBlock; `buffer.clear()` before any voice rendering; `drainAndUpdate()` BEFORE `renderNextBlock`; `numSamples` cached as single local; no allocation (`getRawParameterValue` returns cached `std::atomic<float>*`; `dynamic_cast` is RTTI lookup; `SmoothedValue::skip` is stack arithmetic).

---

#### 6. [ ] Backfill `research/ARCHITECTURE.md` with Phase 2.2 as-shipped rev-note

**Files:** `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` (MODIFY — append rev-note only; partial table values shipped unchanged from §"Bassoon Partial Table")

**Depends on:** Task 2 (so as-shipped values are confirmed against source code post-edit)

**Source:** RESEARCH-rev-2 §1 OQ#10-rev-2 — locked default (append-rev-note; iteration-case as-shipped subsection only invoked if rev-3 listening loop changes any partial value).

**Concrete change:** Append at end of document (after existing "Implementation Risks" / "Decision Log" sections — match house style):

```markdown

---

## rev-note: Phase 2.2 As-Shipped (2026-04-27)

**Cycle:** GSD Phase 2.2 — Spectral Tuning + Tone Control. Atomic commit subject:
`feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`.

**As-shipped state matches §"Bassoon Partial Table" verbatim** — partial-ratio values,
formant centre (475 Hz), bandwidth (200 Hz), per-mode amplitude formula
`formantWeight × rollOff = exp(-0.5 × ((f_k - 475) / 200)²) × 1 / (1 + 0.5k)`, and
tone T60 scale `mix(0.3, 1.5, tone)` for upper modes (k > 4, zero-indexed → modes 5–15)
all shipped as specified. No iteration-driven deviations.

**One in-cycle tuning constant deviation** (RESEARCH-rev-2 §1 OQ#5-rev-2):
`processSample` output scaler relaxed from `1 / NUM_MODES` (= 1/16) to `1 / 8` (+6 dB)
to compensate for the formant-Gaussian + 1/k roll-off attenuation vs. Phase 2.1's
flat-amplitude baseline. Phase 2.3 wiring of `output_gain` APVTS read replaces this
with a user-controllable scaler.

**Verification:** Gate 2 PASS — see
`plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md` (rev-2). A/B-vs-reference
listening + Logic Channel EQ Analyzer overlay (peak in 400-600 Hz at C3) +
8-voice CPU < 20 % + tone sweep clean.
```

**If rev-3 listening loop revs the partial table** (CONTEXT-rev-2 Q6-rev-2 inline iteration ceiling at rev-3): replace the "as-shipped state matches §... verbatim" paragraph with a §"Phase 2.2 As-Shipped Partial Table" subsection listing the new values + a short rationale. Default path is unchanged-from-spec.

**Invariants preserved:** ARCHITECTURE.md above the new section is read-only at Phase 2.2 — original §"Bassoon Partial Table" / §"Tone / Brightness Control" / §"Implementation Risks" / §"Decision Log" untouched.

---

#### 7. [ ] Build + install fresh + run static checks

**Files:** none (build artefacts only)

**Depends on:** Tasks 1–6

**Build commands** (from `/Users/taylorbrook/Dev/VST-development`):

```bash
cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone
```

Treat any new compile warning as blocking — surface to user.

**Install fresh** (per CLAUDE.md "Plugin Cache Clearing" protocol):

```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Bassoon-dev.component
cp -R build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/VST3/O-Bassoon-dev.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/AU/O-Bassoon-dev.component ~/Library/Audio/Plug-Ins/Components/
```

**Pre-commit static checks** (block on any failure):

1. **RT-safety grep — zero hits in all touched files:**
   ```bash
   grep -nE 'new |make_unique|make_shared|push_back|resize\(|malloc|calloc' \
     plugins/O-Bassoon/Source/ModeBank.cpp \
     plugins/O-Bassoon/Source/ModeBank.h \
     plugins/O-Bassoon/Source/BassoonVoice.cpp \
     plugins/O-Bassoon/Source/BassoonVoice.h \
     plugins/O-Bassoon/Source/PluginProcessor.cpp \
     plugins/O-Bassoon/Source/PluginProcessor.h
   ```
   Expected: zero matches.

2. **NE drain ordering preserved:**
   ```bash
   grep -nC2 'vst3Extensions.drainAndUpdate\|synthesiser.renderNextBlock' \
     plugins/O-Bassoon/Source/PluginProcessor.cpp
   ```
   Expected: `drainAndUpdate()` line still precedes `renderNextBlock()`; new tone-dispatch block precedes BOTH.

3. **Mode-index zero-indexed convention:**
   ```bash
   grep -n 'for (int k = 5; k < NUM_MODES' plugins/O-Bassoon/Source/ModeBank.cpp
   ```
   Expected: ONE match in `applyToneChange`. Reject `k = 4` or `k > 4` (off-by-one — modes 0–4 are tone-invariant per D4-rev-2).

4. **Headroom scaler relaxation locked:**
   ```bash
   grep -n '1.0f / 8.0f\|1\.0f \* 0\.125f' plugins/O-Bassoon/Source/ModeBank.cpp
   grep -n '1.0f / NUM_MODES\|1.0f / 16.0f' plugins/O-Bassoon/Source/ModeBank.cpp
   ```
   Expected: first grep ≥ 1 match in `processSample`; second grep zero matches.

5. **Throttle epsilon locked:**
   ```bash
   grep -n '0.001f' plugins/O-Bassoon/Source/PluginProcessor.cpp
   ```
   Expected: ONE match in the tone-dispatch block.

6. **DSP-07 (no O-Reed dependency) regress:**
   ```bash
   grep -rE '(O-?Reed|OReed|reed_phys|libreedphys|ReedSimulator)' \
     plugins/O-Bassoon/Source plugins/O-Bassoon/CMakeLists.txt
   ```
   Expected: zero matches.

7. **AU validation:**
   ```bash
   auval -v aumu OBsn OuDv
   ```
   Expected: `AU VALIDATION SUCCEEDED`.

8. **pluginval --strictness 5:**
   ```bash
   /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate \
     ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3
   ```
   Expected: exit code 0.

---

#### 8. [ ] Manual Gate 2 verification in Logic-AU + reference WAV audition

**Files:**
- `plugins/O-Bassoon/research/reference-recordings/phase-2.2-as-shipped-c3-spectrum.png` (NEW — Logic Channel EQ Analyzer screenshot)
- `plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md` (UPDATE — append Phase 2.2 rev-2 verification report)
- `plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md` (UPDATE — append Phase 2.2 execution summary)

**Depends on:** Task 7 (build install + static checks all pass)

**Manual checklist** (user runs in Logic-AU; planner provides protocol):

1. **Pre-flight — reference WAV pitch audition (D4-rev-1 / OQ#8-rev-2 carry-forward):**
   - Load `bassoon-c3-sustain-v1.wav` into a Logic audio track. Insert Logic stock Tuner. Loop sustained mid-WAV section (avoid attack transient).
   - **PASS bar:** tuner reads C3 (130.8 Hz ± 30 cents). If v1 reads C2 / C4 / mismatched: switch to v2 and repeat. If both fail: document deviation in VERIFICATION, update README pitch annotation, proceed with measured pitch as listening reference.

2. **A/B listening — held C3 timbre match (Gate 2 primary):**
   - Insert O-Bassoon-dev (AU) on software-instrument track. Set `tone = 0.5`. Hold C3 sustained. Loop reference WAV on adjacent track.
   - **PASS bar:** ear judgment "yes, that's bassoon-like" — qualitative. If FAIL: trigger inline rev-3 iteration (CONTEXT-rev-2 Q6-rev-2). Tweak `PARTIAL_RATIOS` or `FORMANT_F1` / `FORMANT_BW`, rebuild, re-audit. Ceiling at rev-3.

3. **Spectrum overlay — peak-region readout (Gate 2 secondary, locked OQ#6-rev-2 protocol):**
   - On the O-Bassoon track, insert Logic stock Channel EQ post-O-Bassoon. Switch to **Pre-EQ Analyzer mode**. All bands disabled / bypassed.
   - Hold sustained C3. Identify peak energy region.
   - **PASS bar:** visible peak in 400–600 Hz region. If peak is below 300 Hz or above 700 Hz: iteration trigger.
   - Capture screenshot to `phase-2.2-as-shipped-c3-spectrum.png`.

4. **Tone-sweep cleanliness (QUAL-01):**
   - Hold sustained C3. Sweep `tone` slider from 0.0 → 1.0 → 0.0 over ~3 s.
   - **PASS bar:** no zipper, no clicks, no pops, no NaN. Audible character change.
   - If zipper detected: inline iteration → fall back to per-block coefficient lerp (Risk #2 mitigation).

5. **Tone descriptor verification (OQ#7-rev-2):**
   - `tone = 0` / hold C3: should sound "woody, dark" (upper modes ~75–420 ms T60).
   - `tone = 1` / hold C3: should sound "brighter, present" (upper modes 375 ms–2.1 s).
   - **PASS bar:** clearly audible character difference; both ends musical.
   - If extreme sounds artificial: narrow mix range to `mix(0.5, 1.3, tone)` (Risk #5 mitigation; ARCHITECTURE.md deviation — document).

6. **8-voice CPU early signal (PERF-02 / OQ#9-rev-2):**
   - Logic CPU meter → System Performance Meter / Process bar. Hold the locked 8-note chord: **C3 + E3 + G3 + Bb3 + C4 + E4 + G4 + Bb4**. Sustain ≥ 3 s.
   - **PASS bar:** Process bar < 20 %.
   - If exceeded: trigger ARCHITECTURE Risk #1 Fallback 1 — drop NUM_MODES from 16 to 8, re-tune partial table BEFORE finalising. Cheaper to swap pre-tune than post-tune.

7. **1-voice CPU regress (carry forward Phase 2.1 Gate 1 item 5):**
   - Hold single C3.
   - **PASS bar:** Process bar < 5 %.

8. **Pitch range C1–C6 regress (carry forward Phase 2.1 Gate 1 item 6):**
   - Sweep MIDI C1 → C6, single notes, sustained ~1 s each.
   - **PASS bar:** all notes track pitch, no glitches, no NaN, no muting. C5+ thinning expected (Nyquist-muted upper modes — not a defect).

9. **Long-tone stability (carry forward Phase 2.1 Gate 1 item 4 subset):**
   - Hold C3 ≥ 10 s.
   - **PASS bar:** no dropouts, no NaN, no exponential drift, no DC bias buildup. (Full 60 s = Phase 2.3 scope.)

10. **Write VERIFICATION-rev-2.md** with results table mapping items 1–9 above to PASS / PARTIAL / DEVIATION; regression confirmation that Phase 2.1 invariants (RT-safety grep zero, NE drain ordering, locked-Q2 grep zero) still hold.

**Final Gate 2 score** = sum of item PASS counts; bar = all 10 items PASS. Items 1, 5, 6, 7 may be downgraded to "PARTIAL — minor deviation, documented" without blocking the gate (per CONTEXT-rev-2 Q6-rev-2 inline iteration). Items 2, 3, 4, 8, 9 must PASS clean.

---

#### 9. [ ] Atomic commit on Gate 2 PASS

**Files:** all source edits from Tasks 1–5 + ARCHITECTURE.md backfill from Task 6 + Phase 2.2 planning artefacts (CONTEXT.md / RESEARCH.md / PLAN.md / SUMMARY.md / VERIFICATION.md) + STATUS.md update + spectrum screenshot

**Depends on:** Task 8 with all blocking items PASS

**Pre-commit `git status` expected files:**

```
M plugins/O-Bassoon/Source/ModeBank.h
M plugins/O-Bassoon/Source/ModeBank.cpp
M plugins/O-Bassoon/Source/BassoonVoice.h
M plugins/O-Bassoon/Source/BassoonVoice.cpp
M plugins/O-Bassoon/Source/PluginProcessor.h
M plugins/O-Bassoon/Source/PluginProcessor.cpp
M plugins/O-Bassoon/.planning/research/ARCHITECTURE.md
M plugins/O-Bassoon/.planning/STATUS.md
M plugins/O-Bassoon/.planning/stages/2-dsp/CONTEXT.md          (rev-2 already written at discuss-phase)
M plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md         (rev-2 already written at research-phase)
M plugins/O-Bassoon/.planning/stages/2-dsp/PLAN.md             (rev-2, this addendum)
M plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md          (Phase 2.2 subsection appended at Task 8)
M plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md     (Phase 2.2 rev-2 subsection appended at Task 8)
?? plugins/O-Bassoon/research/reference-recordings/phase-2.2-as-shipped-c3-spectrum.png
```

**Commit (explicit file list per CLAUDE.md "stage specific files by name"):**

```bash
git add plugins/O-Bassoon/Source/ModeBank.h \
        plugins/O-Bassoon/Source/ModeBank.cpp \
        plugins/O-Bassoon/Source/BassoonVoice.h \
        plugins/O-Bassoon/Source/BassoonVoice.cpp \
        plugins/O-Bassoon/Source/PluginProcessor.h \
        plugins/O-Bassoon/Source/PluginProcessor.cpp \
        plugins/O-Bassoon/.planning/research/ARCHITECTURE.md \
        plugins/O-Bassoon/.planning/STATUS.md \
        plugins/O-Bassoon/.planning/stages/2-dsp/CONTEXT.md \
        plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md \
        plugins/O-Bassoon/.planning/stages/2-dsp/PLAN.md \
        plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md \
        plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md \
        plugins/O-Bassoon/research/reference-recordings/phase-2.2-as-shipped-c3-spectrum.png
```

**Commit message (locked subject from CONTEXT-rev-2 Q9-rev-2):**

```
feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS

Replace Phase 2.1 placeholder integer-harmonic ratios + flat amplitudes
with bassoon-tuned near-integer ratios (16 partials, ARCHITECTURE.md
§Bassoon Partial Table) + first-formant-Gaussian (475 Hz / 200 Hz BW) ×
1/k roll-off per-mode amplitude shaping. Wire `tone` APVTS parameter
(per-mode T60 scaling for upper modes k>4, zero-indexed → modes 5–15)
end-to-end via processor-level juce::SmoothedValue<float, Linear>
(50 ms ramp) + throttled-epsilon dispatch (epsilon = 0.001).

Headroom scaler relaxed from 1/16 to 1/8 (+6 dB) to compensate for
formant-Gaussian + 1/k attenuation; output_gain APVTS wiring (Phase 2.3)
will replace.

Gate 2 PASS:
- A/B listen vs. v1 reference WAV: bassoon-like timbre
- Logic Channel EQ Analyzer: peak in 400-600 Hz at C3
- Tone sweep [0,1]: clean (no zipper / clicks / NaN)
- 8-voice CPU < 20 %, 1-voice CPU < 5 %
- Phase 2.1 invariants regress-clean (RT-safety, NE drain ordering)

Files:
- Source/ModeBank.{h,cpp}: bassoon partial table + setTone +
  applyToneChange + computeModeAmplitude + 1/8 scaler
- Source/BassoonVoice.{h,cpp}: setTone forwarder
- Source/PluginProcessor.{h,cpp}: toneSmoother + dispatch in processBlock
  before NE drain (preserves Phase 2.1 ordering invariant)
- research/ARCHITECTURE.md: Phase 2.2 as-shipped rev-note appended
- .planning/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}.md: rev-2
- research/reference-recordings/phase-2.2-as-shipped-c3-spectrum.png

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
```

**Pre-commit grep regression suite** (re-run before commit):
- RT-safety grep zero (Task 7 #1)
- NE drain ordering preserved (Task 7 #2)
- Mode-index `for (int k = 5;` zero-indexed (Task 7 #3)
- `1.0f / 8.0f` present, `1.0f / NUM_MODES` absent (Task 7 #4)
- Throttle epsilon `0.001f` present (Task 7 #5)
- DSP-07 zero matches (Task 7 #6)

---

### Success Criteria (Gate 2)

A Gate 2 PASS requires ALL of:

- [ ] Build clean (Task 7 — VST3 + AU + Standalone, zero warnings)
- [ ] AU validation SUCCEEDED (Task 7)
- [ ] pluginval --strictness 5 exit 0 (Task 7)
- [ ] RT-safety grep zero (Task 7)
- [ ] NE drain ordering preserved (Task 7)
- [ ] Mode-index `for (int k = 5;` (Task 7)
- [ ] Headroom scaler `1.0f / 8.0f`; `1.0f / NUM_MODES` absent (Task 7)
- [ ] DSP-07 zero matches (Task 7)
- [ ] A/B listen test passes — bassoon-like vs. v1 reference (Task 8 #2)
- [ ] Spectrum peak in 400–600 Hz at C3 (Task 8 #3)
- [ ] Tone sweep clean across [0, 1] (Task 8 #4)
- [ ] Tone descriptors audible — woody/bright (Task 8 #5)
- [ ] 8-voice CPU < 20 % (Task 8 #6)
- [ ] 1-voice CPU < 5 % (Task 8 #7)
- [ ] Pitch range C1–C6 clean (Task 8 #8)
- [ ] Long-tone ≥ 10 s stable (Task 8 #9)
- [ ] Reference WAV pitch audited / documented (Task 8 #1)
- [ ] Spectrum screenshot captured (Task 8 #3)
- [ ] VERIFICATION-rev-2 written (Task 8 #10)
- [ ] ARCHITECTURE.md rev-note appended (Task 6)
- [ ] Atomic commit landed with locked subject (Task 9)

**Iteration ceiling** (CONTEXT-rev-2 Q6-rev-2): if Task 8 items 2 / 3 / 5 fail, iterate inline (no replan loop) — tweak partial-ratio constants or formant params, rebuild, re-audit. Ceiling at rev-3. After rev-3, ship and document v1.0 gap as v1.1 partial-table refinement candidate (ARCHITECTURE Risk #2 Fallback 2 framing — "bassoon-inspired").

### Risks Carried (from CONTEXT-rev-2)

1. **Partial table doesn't sound bassoon-like at rev-1 listening** — inline iteration to rev-3, then ship as "bassoon-inspired".
2. **Tone sweep zipper noise** — fallback per-block coefficient lerp.
3. **8-voice CPU > 20 %** — fallback NUM_MODES = 8, re-tune partials.
4. **Loudness regression vs. Phase 2.1** — mitigated by `1/8` scaler relaxation (locked).
5. **Tone extremes sound artificial** — fallback narrow mix range `(0.5, 1.3, tone)`.
6. **Tone smoothing latency** — 50 ms is spec; 25 ms fallback only if subjectively soggy.
7. **Reference WAV pitch mismatch** — fallback v2; document if both fail.
8. **ARCHITECTURE.md backfill drift** — pre-commit grep verifies partial-ratio block matches spec.

### Out of Scope (deferred per ROADMAP)

- All other 9 APVTS reads in voice DSP (vibrato_*, breath, attack_character, attack_time, release_time, voice_count, output_gain) — Phase 2.3 / 2.4
- Vibrato LFO + onset envelope — Phase 2.3
- Sustain noise component in `Exciter` — Phase 2.4
- Attack-character morph — Phase 2.4
- Voice manager / `voice_count` enforcement — Phase 2.4
- VST3 NE consumption per-voice / `applyPendingTuning` — Phase 2.4
- TuningEngine `getFrequency()` call in startNote — Phase 2.4
- Two-register-table fallback — only invoked if rev-3 listening fails the bar
- pluginval `--strictness 10` + Windows VST3 build — Stage 4
- 60 s long-tone stability test — Phase 2.3 (after envelope wired)
- CMakeLists.txt edits — none required at Phase 2.2

### Inputs Consumed Verbatim (rev-2)

- **CONTEXT-rev-2** §"Cycle Scope (rev-2)" + §"Constraints Identified (rev-2)" + §"Approach Decisions (rev-2)" Q1–Q9 — locked scope, dispatch design, Gate 2 bar
- **RESEARCH-rev-2** §3 "Implementation Skeletons (rev-2)" — ModeBank.h/.cpp + BassoonVoice.h/.cpp + PluginProcessor.h/.cpp consumed as task body content
- **RESEARCH-rev-2** §1 OQ#5-rev-2 — `1/8` headroom scaler relaxation locked
- **RESEARCH-rev-2** §1 OQ#9-rev-2 — 8-note chord (C3+E3+G3+Bb3+C4+E4+G4+Bb4) for CPU early signal
- **RESEARCH-rev-2** §1 OQ#10-rev-2 — ARCHITECTURE.md backfill format (append-rev-note default)
- **ARCHITECTURE.md** §"Bassoon Partial Table" + §"Tone / Brightness Control" — partial ratios, formant constants, T60 mix function consumed verbatim into ModeBank

### Audit Trail (rev-2 addendum)

**rev-2 (this addendum, 2026-04-27):** Phase 2.2 plan — Bassoon Spectral Tuning + Tone Control. 9 single-Wave tasks (6 source/spec edits + 1 build/static-check + 1 manual Gate 2 verify + 1 atomic commit). Lifts RESEARCH-rev-2 §3 implementation skeletons verbatim. Pins Gate 2 PASS bar from CONTEXT-rev-2 Q5-rev-2 (ear A/B + Logic EQ Analyzer) + Q8-rev-2 (8-voice CPU < 20 %). Atomic commit subject locked: `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`.

**Inherited from Phase 2.1 PLAN-rev-1 + Phase 2.2 CONTEXT-rev-2 + RESEARCH-rev-2 (not re-litigated):**
- Per-sample render-loop ordering (`exciter → modeBank → adsr → addSample`)
- Centred equal L+R per-sample voice write
- NE drain BEFORE renderNextBlock at PluginProcessor.cpp:178 → :182
- DSP-07 (no O-Reed dependency) carry-forward
- Reference WAVs at `research/reference-recordings/bassoon-c3-sustain-v{1,2}.wav` (sourced Phase 2.1)
- Atomic-commit gate-first principle
- Logic Pro (AU) primary listening DAW; Logic CPU System Performance Meter / Process bar

**New in rev-2:**
- Cycle scope = Phase 2.2 only (partial-table replacement + formant-Gaussian amplitudes + `tone` wiring + A/B listening)
- Single processor-level `tone` smoother + throttled-epsilon dispatch BEFORE NE drain
- Mode-index `k > 4` zero-indexed → modes 5–15 in `applyToneChange`
- Headroom scaler relaxed `1/16 → 1/8`
- Logic Channel EQ Analyzer (Pre-EQ mode) replaces SPAN for spectrum check
- 8-note chord protocol (C3+E3+G3+Bb3+C4+E4+G4+Bb4) for CPU early signal
- ARCHITECTURE.md as-shipped rev-note (append-default)
- No CMakeLists.txt edits at Phase 2.2

### Next Phase

Ready for: **execute** phase — `/clear` then `/plugin-execute O-Bassoon 2-dsp`
