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

---

## rev-3 — Phase 2.3 Plan (2026-04-28)

**Cycle:** Phase 2.3 — Per-Note Expression: Envelope, Breath, Vibrato, Output Gain. Single GSD cycle. Working tree starts from Phase 2.2 atomic commit `baac74f` on `main` with the rev-3 `strike()` patch in place + bassoon-tuned partial table + `tone` SmoothedValue dispatch + 1/8 headroom scaler.

**Inputs (consumed verbatim):**
- `stages/2-dsp/CONTEXT.md` rev-3 addendum — 8 user-confirmed approach decisions across two batches (Q1–Q4-rev-3 batch 1 + Q1–Q4-rev-3 batch 2) + 12 derived locks
- `stages/2-dsp/RESEARCH.md` rev-3 addendum — 10 OQ resolutions + §2 Pattern Confirmations + §3 Implementation Skeletons (Vibrato.{h,cpp}, NoiseExciter.{h,cpp}, BassoonVoice rev-3 deltas, PluginProcessor rev-3 deltas, CMakeLists rev-3 deltas) lifted as task body content + §4 Discrepancies D1–D7-rev-3 + §5 10 static-check grep gates
- `research/ARCHITECTURE.md` (Phase 2.2 as-shipped rev-note appended) — read-only at Phase 2.3; rev-3 backfill is append-only

**Atomic commit subject (locked CONTEXT-rev-3 Q4-rev-3 batch 2):** `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`

### Goal (rev-3)

Wire four APVTS-driven expression systems end-to-end and pivot the voice excitation source from struck-modal-only to continuous breath-driven sustain:

1. **`juce::ADSR`** wired to `attack_time` (0–2000 ms) + `release_time` (0–3000 ms), block-rate `setParameters` with epsilon-throttled dispatch (no internal smoother — locked OQ#2-rev-3).
2. **Breath / dynamics** with multiplicative composition `breath_voice = ui_breath × cc2_normalised`, velocity-as-initial-UI-breath at startNote, CC2-takeover state machine (500 ms idle window — locked OQ#5-rev-3), per-voice 20 ms `Linear` smoother sampled per-sample.
3. **Per-voice sine-LFO vibrato** (`Vibrato` class, NEW) with random initial phase per startNote (locked OQ#9-rev-3), variable-duration onset SmoothedValue, multiplicative cents output composed with pitch-bend at `f_final = currentFrequencyBase × pow(2, vibratoCents/1200) × pow(2, pitchBendSemitones/12)`, block-rate setFundamental dispatch when `|Δf_final| > 0.1 Hz`.
4. **Post-summation `output_gain`** (-24 to +6 dB), processor-level 30 ms `Linear` smoother + `buffer.applyGainRamp(0, numSamples, current, smoother.skip(N))` (canonical declick-safe idiom — locked OQ#1-rev-3).

**Architectural pivot (Phase 2.3 only):** Introduce `NoiseExciter` (NEW, per-voice 1-pole LP @ 2 kHz over white noise, `BASE_NOISE_GAIN = 0.05f`, scaled by `breath_voice`) as the primary continuous excitation source feeding `ModeBank`. Drop the Phase 2.1 `exciter.getNextSample()` call from `BassoonVoice::renderNextBlock` (file retained verbatim for Phase 2.4 attack-character morph re-introduction). Retain rev-3 `strike()` at `startNote` as the attack transient. Per-voice `juce::Random` seeded `voiceIndex × 31337` (O-Bowed `BowNoiseGenerator.h:23` precedent — locked OQ#3-rev-3, overrides CONTEXT-rev-3 default).

**Gate 3 PASS** = 10-item manual checklist (3 ADSR + 1 breath + 3 vibrato + 1 output_gain + 1 long-tone-60s + 1 polyphony-CPU) + automated invariant battery (RT-safety grep zero, NE drain ordering, expression dispatch ordering, applyGainRamp form, mode-bank cadence, scaler retention, throttle epsilon, DSP-07, auval, pluginval-5).

### Wave Layout (rev-3)

**Single Wave** (gate-first; no parallel sub-waves). Sequence: Vibrato.h → Vibrato.cpp → NoiseExciter.h → NoiseExciter.cpp → BassoonVoice.h → BassoonVoice.cpp → PluginProcessor.h → PluginProcessor.cpp → CMakeLists.txt → ARCHITECTURE.md backfill → build/install/static-checks → manual Gate 3 verify → atomic commit.

Tasks 1–4 create new sources; tasks 5–6 modify existing sources; tasks 7–8 are CMake + spec backfill; tasks 9–11 are verification + commit.

---

### Tasks (rev-3)

#### 1. [ ] Create `Source/Vibrato.{h,cpp}` (NEW — per-voice sine LFO + onset envelope)

**Files:**
- `plugins/O-Bassoon/Source/Vibrato.h` (NEW)
- `plugins/O-Bassoon/Source/Vibrato.cpp` (NEW)

**Depends on:** none

**Source:** RESEARCH-rev-3 §3 "Source/Vibrato.h (NEW)" + "Source/Vibrato.cpp (NEW)" — lift verbatim.

**Concrete content:**

`Vibrato.h` — public API: `prepare(double sr) noexcept`, `reset() noexcept`, `setRateHz(float) noexcept`, `setDepthCents(float) noexcept`, `setOnsetMs(float) noexcept`, `getCurrentCents() noexcept`. Private: `recomputeIncrement() noexcept`, `sampleRate = 48000.0`, `rateHz = 5.0f`, `depthCents = 0.0f`, `phase = 0.0f`, `phaseIncrement = 0.0f`, `juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> onset { 0.0f }`. Header guard via `#pragma once`; include `<JuceHeader.h>` only.

`Vibrato.cpp` — five methods + one private helper:
- `prepare(sr)`: cache `sampleRate`, call `recomputeIncrement()`, `onset.reset(sampleRate, 0.0)`, `onset.setCurrentAndTargetValue(1.0f)`.
- `reset()`: **random phase per startNote** — `phase = juce::Random::getSystemRandom().nextFloat() * juce::MathConstants<float>::twoPi` (locked OQ#9-rev-3, O-Wind `FluteSynthVoice.cpp:114-116` precedent — overrides CONTEXT-rev-3 default instant-zero per D2-rev-3); `onset.reset(0.0f)`; `onset.setTargetValue(1.0f)`.
- `setRateHz(r)`: `rateHz = r; recomputeIncrement();`.
- `setDepthCents(d)`: `depthCents = d;` (no smoothing — locked CONTEXT-rev-3 line 526; LFO modulation masks zipper).
- `setOnsetMs(ms)`: `onset.reset(sampleRate, juce::jmax(0.0, ms / 1000.0)); onset.setTargetValue(1.0f);`. Edge case `ms = 0` → SmoothedValue 0-step countdown → instant target on next `getNextValue()` (D5-rev-3 confirmed safe).
- `getCurrentCents()`: `const float onsetGain = onset.getNextValue(); const float cents = depthCents * onsetGain * std::sin(phase); phase += phaseIncrement; if (phase > twoPi) phase -= twoPi; return cents;`.
- `recomputeIncrement()`: `phaseIncrement = juce::MathConstants<float>::twoPi * rateHz / static_cast<float>(sampleRate);`.

**Invariants required:** zero allocation in any method; `noexcept` on every public method; `juce::Random::getSystemRandom()` call documented as accepted shared-state risk per D7-rev-3 (`Synthesiser::renderVoices` is single-threaded within a renderNextBlock call).

**File header comment:** include the standard banner (Phase 2.3 deliverable, Ouaricon Audio, Developer: Taylor Brook, multiplicative pitch-cents output, random phase O-Wind precedent) per RESEARCH-rev-3 §3 skeleton.

---

#### 2. [ ] Create `Source/NoiseExciter.{h,cpp}` (NEW — per-voice continuous filtered-noise excitation)

**Files:**
- `plugins/O-Bassoon/Source/NoiseExciter.h` (NEW)
- `plugins/O-Bassoon/Source/NoiseExciter.cpp` (NEW)

**Depends on:** none

**Source:** RESEARCH-rev-3 §3 "Source/NoiseExciter.h (NEW)" + "Source/NoiseExciter.cpp (NEW)" — lift verbatim.

**Concrete content:**

`NoiseExciter.h` — public API: `prepare(double sr, int voiceIndex) noexcept`, `reset() noexcept`, `getNextSample(float breathScaled) noexcept`. Private: `static constexpr float CUTOFF_HZ = 2000.0f`, `static constexpr float BASE_NOISE_GAIN = 0.05f` (with sourcing comment "OQ#4-rev-3 starting point; verify-phase rev-1 ear-tunes within [0.03f, 0.20f]"), `double sampleRate = 48000.0`, `float lpCoeff = 0.0f`, `float lpState = 0.0f`, `juce::Random rng`. Header guard via `#pragma once`; include `<JuceHeader.h>` only.

`NoiseExciter.cpp` — three methods:
- `prepare(sr, voiceIndex)`: cache `sampleRate`; **deterministic per-voice seed** — `rng.setSeed(static_cast<juce::int64>(voiceIndex) * 31337)` (locked OQ#3-rev-3, O-Bowed `BowNoiseGenerator.h:23` precedent — overrides CONTEXT-rev-3 default `Time::currentTimeMillis() ^ voiceIndex` per D3-rev-3); `lpCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * CUTOFF_HZ / static_cast<float>(sampleRate))`; `lpState = 0.0f`.
- `reset()`: `lpState = 0.0f` only (rng state preserved across notes; mode-bank reset already handles transient cleanup).
- `getNextSample(breathScaled)`: `const float white = rng.nextFloat() * 2.0f - 1.0f; lpState += lpCoeff * (white - lpState); return lpState * BASE_NOISE_GAIN * breathScaled;`.

**Invariants required:** zero allocation in any method; `noexcept` on every public method; `juce::Random::nextFloat()` is allocation-free per-instance (juce_Random.cpp:132–137); per-voice instance avoids shared mutable state.

**File header comment:** include the standard banner (Phase 2.3 architectural pivot, replaces struck-modal sustain with breath-driven sustain, O-Bowed BowNoiseGenerator.h:23 precedent for seed) per RESEARCH-rev-3 §3 skeleton.

---

#### 3. [ ] Add Phase 2.3 surface to `BassoonVoice.h` (members + setters)

**Files:** `plugins/O-Bassoon/Source/BassoonVoice.h` (MODIFY)

**Depends on:** Tasks 1, 2 (Vibrato + NoiseExciter classes must exist for member declarations)

**Source:** RESEARCH-rev-3 §3 "Source/BassoonVoice.h (rev-3 deltas to Phase 2.2)" — lift verbatim.

**Concrete changes:**

- Add to includes (with existing `ModeBank.h` / `Exciter.h`):
  ```cpp
  #include "Vibrato.h"
  #include "NoiseExciter.h"
  ```
- Add to public API after existing `setTone` declaration (Phase 2.2):
  ```cpp
  // Phase 2.3 aggregate setter (called from PluginProcessor each block, throttled).
  void setExpression (float attackMs, float releaseMs,
                      float vibRateHz, float vibDepthCents, float vibOnsetMs,
                      float uiBreath) noexcept;

  // Phase 2.3 voice-construction-time setter for per-voice noise seed.
  void setVoiceIndex (int idx) noexcept { voiceIndex = idx; }
  ```
- Add to private members (preserve existing Phase 2.1/2.2 layout — `parameters / tuningEngine / pendingTuningSource` raw pointers + `modeBank / exciter / adsr / pitchWheelValue / pitchBendSemitones / currentFrequencyBase`):
  ```cpp
  // Phase 2.3 systems
  Vibrato      vibrato;
  NoiseExciter noiseExciter;
  juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> breathSmoother { 0.0f };

  // Phase 2.3 frequency-change throttle (vibrato + pitch-bend block-rate compose)
  float lastDispatchedFrequency = 0.0f;

  // Phase 2.3 expression dispatch shadows (per-voice; processor-scope shadows separate)
  float lastAppliedAttackMs   = -1.0f;
  float lastAppliedReleaseMs  = -1.0f;
  float lastAppliedVibRate    = -1.0f;
  float lastAppliedVibDepth   = -1.0f;
  float lastAppliedVibOnsetMs = -1.0f;

  // Phase 2.3 CC2-takeover state machine (500 ms idle window — OQ#5-rev-3)
  bool        cc2EverActive       = false;
  juce::int64 lastCC2SampleCount  = 0;
  juce::int64 currentSampleCount  = 0;
  float       lastUiBreath        = 0.7f;   // shadow for multiplicative compose

  // Phase 2.3 voice-index for noise seeding
  int voiceIndex = 0;
  ```

**Invariants preserved:** Phase 2.1/2.2 public API unchanged (`canPlaySound`, `startNote`, `stopNote`, `pitchWheelMoved`, `controllerMoved`, `renderNextBlock`, `prepareToPlay`, `setAPVTS`, `setTuningEngine`, `setPendingTuningSource`, `setTone`); existing private members untouched; `Exciter exciter;` member declared verbatim (D6-rev-3 retention — Phase 2.4 re-wires); `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` macro intact. Sentinel `-1.0f` initialisers ensure first dispatch fires unconditionally on every startNote.

---

#### 4. [ ] Implement Phase 2.3 deltas in `BassoonVoice.cpp` (prepareToPlay + startNote + controllerMoved + setExpression + renderNextBlock)

**Files:** `plugins/O-Bassoon/Source/BassoonVoice.cpp` (MODIFY)

**Depends on:** Task 3 (header declarations must exist)

**Source:** RESEARCH-rev-3 §3 "Source/BassoonVoice.cpp (rev-3 deltas)" — lift verbatim.

**Concrete changes:**

**(a) `prepareToPlay`** (currently lines 24–32) — append after Phase 2.2 ADSR setSampleRate + tone init, before closing brace:
```cpp
vibrato.prepare (sampleRate);
noiseExciter.prepare (sampleRate, voiceIndex);
breathSmoother.reset (sampleRate, 0.020);   // 20 ms ramp (CONTEXT-rev-3 line 521)
breathSmoother.setCurrentAndTargetValue (0.7f);
```

**(b) `startNote`** (currently calls `exciter.start()` at line 51) — keep existing setTone/setFundamental/strike() ordering from Phase 2.2; APPEND after `exciter.start()`:
```cpp
// Phase 2.3: ADSR APVTS reads at note-on (one-shot)
const float attackMs  = parameters->getRawParameterValue ("attack_time")->load();
const float releaseMs = parameters->getRawParameterValue ("release_time")->load();
adsr.setParameters ({ attackMs / 1000.0f, 0.0f, 1.0f, releaseMs / 1000.0f });
adsr.noteOn();

// Phase 2.3: reset Phase 2.3 systems
vibrato.reset();
noiseExciter.reset();
breathSmoother.setCurrentAndTargetValue (velocity);  // velocity-as-initial-UI-breath
lastUiBreath = velocity;

// Phase 2.3: CC2 state reset on every note-on
cc2EverActive      = false;
lastCC2SampleCount = 0;

// Phase 2.3: shadow init — force first setExpression dispatch
lastAppliedAttackMs   = -1.0f;
lastAppliedReleaseMs  = -1.0f;
lastAppliedVibRate    = -1.0f;
lastAppliedVibDepth   = -1.0f;
lastAppliedVibOnsetMs = -1.0f;
lastDispatchedFrequency = 0.0f;
```
Remove any prior unconditional `adsr.noteOn()` from Phase 2.1/2.2 startNote body; the Phase 2.3 ADSR-reads-then-noteOn block above is the single canonical noteOn site.

**(c) `controllerMoved`** (currently lines 87–89, stub `void BassoonVoice::controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/)`) — replace body:
```cpp
void BassoonVoice::controllerMoved (int controllerNumber, int newControllerValue)
{
    if (controllerNumber == 2)  // CC2: MIDI breath controller
    {
        const float cc2Normalised = juce::jlimit (0.0f, 1.0f,
                                                   static_cast<float> (newControllerValue) / 127.0f);
        cc2EverActive       = true;
        lastCC2SampleCount  = currentSampleCount;
        // Multiplicative compose: breath_voice = ui_breath × cc2_normalised
        breathSmoother.setTargetValue (lastUiBreath * cc2Normalised);
    }
}
```
CC2 normalisation site is `controllerMoved` (locked OQ#8-rev-3, O-Wind `FluteSynthVoice.cpp:227` precedent). Other controllers ignored at Phase 2.3 (CC1 / aftertouch deferred to v1.1 per Stage 0 D4).

**(d) `setExpression` (NEW)** — add new method after `setTone` (currently lines 92–97), before `renderNextBlock`:
```cpp
void BassoonVoice::setExpression (float attackMs, float releaseMs,
                                   float vibRateHz, float vibDepthCents, float vibOnsetMs,
                                   float uiBreath) noexcept
{
    constexpr float EPS = 0.001f;

    // ADSR — re-shape only when changed
    if (std::abs (attackMs  - lastAppliedAttackMs)  > EPS
     || std::abs (releaseMs - lastAppliedReleaseMs) > EPS)
    {
        adsr.setParameters ({ attackMs / 1000.0f, 0.0f, 1.0f, releaseMs / 1000.0f });
        lastAppliedAttackMs  = attackMs;
        lastAppliedReleaseMs = releaseMs;
    }

    if (std::abs (vibRateHz     - lastAppliedVibRate)    > EPS) { vibrato.setRateHz (vibRateHz);     lastAppliedVibRate    = vibRateHz; }
    if (std::abs (vibDepthCents - lastAppliedVibDepth)   > EPS) { vibrato.setDepthCents (vibDepthCents); lastAppliedVibDepth = vibDepthCents; }
    if (std::abs (vibOnsetMs    - lastAppliedVibOnsetMs) > EPS) { vibrato.setOnsetMs (vibOnsetMs);   lastAppliedVibOnsetMs = vibOnsetMs; }

    // Breath UI shadow (always update — small cost; CC2-takeover gate decides effective target)
    lastUiBreath = uiBreath;

    // CC2-takeover gate: only apply UI breath if CC2 idle for >500 ms
    const juce::int64 cc2WindowSamples = static_cast<juce::int64> (0.500 * getSampleRate());
    const bool cc2RecentlyActive = cc2EverActive
                                && (currentSampleCount - lastCC2SampleCount) < cc2WindowSamples;
    if (! cc2RecentlyActive)
        breathSmoother.setTargetValue (uiBreath);
    // else: CC2 is the active source; controllerMoved sets the smoother target
}
```

**(e) `renderNextBlock`** (currently lines 99–129) — replace per-block prologue + per-sample inner loop:
```cpp
void BassoonVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                     int startSample, int numSamples)
{
    if (! adsr.isActive())
        return;

    // Phase 2.3 per-block prologue: vibrato compose + setFundamental throttle
    const float vibratoCents = vibrato.getCurrentCents();
    const float vibratoMult  = std::pow (2.0f, vibratoCents / 1200.0f);
    const float pbMult       = std::pow (2.0f, pitchBendSemitones / 12.0f);
    const float f_final      = currentFrequencyBase * vibratoMult * pbMult;

    if (std::abs (f_final - lastDispatchedFrequency) > 0.1f)
    {
        modeBank.setFundamental (f_final, getSampleRate());
        lastDispatchedFrequency = f_final;
    }

    // Per-sample render: continuous noise excitation → modeBank → ADSR → output
    for (int i = 0; i < numSamples; ++i)
    {
        const float breath     = breathSmoother.getNextValue();
        const float excitation = noiseExciter.getNextSample (breath);
        const float voice      = modeBank.processSample (excitation);
        const float env        = adsr.getNextSample();
        const float sample     = voice * env;

        outputBuffer.addSample (0, startSample + i, sample);
        outputBuffer.addSample (1, startSample + i, sample);
    }

    // Phase 2.3: advance sample-count for CC2-takeover state machine
    currentSampleCount += numSamples;

    // ADSR-idle exit (unchanged from Phase 2.1)
    if (! adsr.isActive())
    {
        clearCurrentNote();
        modeBank.reset();
        noiseExciter.reset();
    }
}
```

**Phase 2.1 `Exciter` call removal:** the existing `const float ex = exciter.getNextSample();` line in the per-sample loop (current line 109) is **deleted** along with its downstream `modeBank.processSample (ex)` call. The `exciter` MEMBER stays declared in `BassoonVoice.h` per Task 3 (D6-rev-3 retention — Phase 2.4 re-wires for attack-character morph). `Exciter.{h,cpp}` source files are NOT modified. Optional: silence unused-private-field warning by retaining the existing `exciter.prepare(sampleRate)` call in `prepareToPlay` (already present at line 28 — keep verbatim) and the existing `exciter.reset()` call in the ADSR-idle exit path (already present at line 124 — replace with `noiseExciter.reset()` per the new render block above; preserve `exciter.reset()` call too if desired for Phase 2.4 re-wire safety).

The existing `exciter.start()` call at line 51 in `startNote` may stay (kept by Task 4(b) preamble) — `Exciter::start()` is a single state assignment, not allocation, and serves as a no-op trigger that costs zero CPU when `getNextSample` is never called from renderNextBlock. Documented as "retained for Phase 2.4 re-wire" per D6-rev-3.

**Invariants preserved:** `juce::ScopedNoDenormals` already at processBlock entry (PluginProcessor — unchanged); per-sample render-loop ordering (`excitation → modeBank → adsr → addSample(L) + addSample(R)`) preserved; equal L+R per-sample voice write preserved; ADSR-idle exit (`clearCurrentNote() + modeBank.reset()`) preserved; Phase 2.2 `setTone` forwarder + `modeBank.applyToneChange()` chain unchanged; `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` macro intact. Voice continues to read **only `attack_time` and `release_time` from APVTS at startNote** (matches CONTEXT-rev-3 strict-ROADMAP wiring); all other expression params arrive via `setExpression` from the processor.

---

#### 5. [ ] Add Phase 2.3 surface to `PluginProcessor.h` (output_gain smoother + 6 dispatch shadows)

**Files:** `plugins/O-Bassoon/Source/PluginProcessor.h` (MODIFY)

**Depends on:** none

**Source:** RESEARCH-rev-3 §3 "Source/PluginProcessor.h (rev-3 deltas)" — lift verbatim.

**Concrete change:** Append inside `private:` after the Phase 2.2 `lastDispatchedTone` member (currently line 69):

```cpp

    // Phase 2.3: output_gain post-summation smoother (30 ms Linear)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother { 1.0f };

    // Phase 2.3: expression dispatch shadows (processor scope; separate from per-voice shadows)
    float lastDispatchedAttackMs   = -1.0f;
    float lastDispatchedReleaseMs  = -1.0f;
    float lastDispatchedVibRate    = -1.0f;
    float lastDispatchedVibDepth   = -1.0f;
    float lastDispatchedVibOnsetMs = -1.0f;
    float lastDispatchedUiBreath   = -1.0f;
```

Sentinel `-1.0f` initialisers ensure first dispatch fires unconditionally (any valid value differs from -1 by > 0.001).

**Invariants preserved:** `parameters`, `synthesiser`, `tuningEngine`, `vst3Extensions`, `toneSmoother`, `lastDispatchedTone` untouched. Public surface untouched. `JuceHeader.h` already provides `juce::SmoothedValue`. `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` macro intact.

---

#### 6. [ ] Wire Phase 2.3 in `PluginProcessor.cpp` (constructor setVoiceIndex + prepareToPlay + processBlock expression dispatch + output_gain applyGainRamp)

**Files:** `plugins/O-Bassoon/Source/PluginProcessor.cpp` (MODIFY)

**Depends on:** Task 4 (BassoonVoice::setExpression + setVoiceIndex must exist) + Task 5 (header members must exist)

**Source:** RESEARCH-rev-3 §3 "Source/PluginProcessor.cpp (rev-3 deltas)" — lift verbatim.

**Concrete changes:**

**(a) Constructor `OBassoonAudioProcessor()`** (currently lines 110–129) — after the existing 16-voice creation loop (line 125 `synthesiser.addVoice(voice);` closes), BEFORE `synthesiser.addSound(...)` (line 128), insert one-time setVoiceIndex wire:
```cpp
// Phase 2.3: per-voice noise-seed wire (NoiseExciter uses voiceIndex × 31337)
for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (i)))
        bv->setVoiceIndex (i);
```

**(b) `prepareToPlay`** (currently lines 134–151) — append after the Phase 2.2 `lastDispatchedTone = -1.0f;` line (currently line 150), before closing brace:
```cpp
// Phase 2.3: output_gain smoother + processor-scope dispatch shadows
outputGainSmoother.reset (sampleRate, 0.030);   // 30 ms ramp
outputGainSmoother.setCurrentAndTargetValue (1.0f);

lastDispatchedAttackMs   = -1.0f;
lastDispatchedReleaseMs  = -1.0f;
lastDispatchedVibRate    = -1.0f;
lastDispatchedVibDepth   = -1.0f;
lastDispatchedVibOnsetMs = -1.0f;
lastDispatchedUiBreath   = -1.0f;
```

**(c) `processBlock`** (currently lines 171–204) — insert Phase 2.3 expression dispatch BETWEEN the Phase 2.2 tone-dispatch block (currently ends line 195) and the existing `vst3Extensions.drainAndUpdate()` call (currently line 200). Insert verbatim:

```cpp
// Phase 2.3: expression dispatch (BEFORE NE drain — locked OQ#6-rev-3).
// Reads 6 APVTS values; dispatches single aggregated setExpression(...) per voice
// only when ANY sub-param changes > epsilon (saves ~6 × 16 = 96 virtual hops/block
// when expression is static, the 99 % case during sustained playback).
const float attackMs   = parameters.getRawParameterValue ("attack_time")->load();
const float releaseMs  = parameters.getRawParameterValue ("release_time")->load();
const float vibRate    = parameters.getRawParameterValue ("vibrato_rate")->load();
const float vibDepth   = parameters.getRawParameterValue ("vibrato_depth")->load();
const float vibOnsetMs = parameters.getRawParameterValue ("vibrato_onset")->load();
const float uiBreath   = parameters.getRawParameterValue ("breath")->load();

const bool anyChanged =
       std::abs (attackMs   - lastDispatchedAttackMs)   > 0.001f
    || std::abs (releaseMs  - lastDispatchedReleaseMs)  > 0.001f
    || std::abs (vibRate    - lastDispatchedVibRate)    > 0.001f
    || std::abs (vibDepth   - lastDispatchedVibDepth)   > 0.001f
    || std::abs (vibOnsetMs - lastDispatchedVibOnsetMs) > 0.001f
    || std::abs (uiBreath   - lastDispatchedUiBreath)   > 0.001f;

if (anyChanged)
{
    for (int v = 0; v < synthesiser.getNumVoices(); ++v)
        if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (v)))
            bv->setExpression (attackMs, releaseMs, vibRate, vibDepth, vibOnsetMs, uiBreath);

    lastDispatchedAttackMs   = attackMs;
    lastDispatchedReleaseMs  = releaseMs;
    lastDispatchedVibRate    = vibRate;
    lastDispatchedVibDepth   = vibDepth;
    lastDispatchedVibOnsetMs = vibOnsetMs;
    lastDispatchedUiBreath   = uiBreath;
}
```

**(d) `processBlock` — output_gain applyGainRamp** (currently ends at line 203 with `synthesiser.renderNextBlock(...)`) — append AFTER the renderNextBlock call, before closing brace:

```cpp
// Phase 2.3: output_gain post-summation declick-safe applyGainRamp
// (locked OQ#1-rev-3: applyGainRamp(0, numSamples, current, smoother.skip(N))
// is JUCE 8 canonical idiom for SmoothedValue-driven buffer-level gain).
const float outDb     = parameters.getRawParameterValue ("output_gain")->load();
const float linearTgt = juce::Decibels::decibelsToGain (outDb);
const float gainStart = outputGainSmoother.getCurrentValue();
outputGainSmoother.setTargetValue (linearTgt);
const float gainEnd   = outputGainSmoother.skip (juce::jmax (0, numSamples));
buffer.applyGainRamp (0, numSamples, gainStart, gainEnd);
```

**New ordering invariant** (Phase 2.3 extends Phase 2.1/2.2 chain — locked OQ#6-rev-3):

1. `juce::ScopedNoDenormals noDenormals;` (existing — first line)
2. `buffer.clear()` (existing — line 177)
3. `const int numSamples = buffer.getNumSamples();` (existing — line 179)
4. **Phase 2.2:** tone smoother advance + voice dispatch (existing — lines 181–195)
5. **Phase 2.3 NEW:** expression dispatch (6 reads + epsilon throttle + setExpression dispatch) — BEFORE NE drain
6. `vst3Extensions.drainAndUpdate()` (existing — line 200)
7. `synthesiser.renderNextBlock (buffer, midiMessages, 0, numSamples)` (existing — line 203)
8. **Phase 2.3 NEW:** output_gain post-summation applyGainRamp — AFTER renderNextBlock

The expression dispatch sits BETWEEN tone-dispatch and NE-drain because vibrato/ADSR/breath are pitch-orthogonal — NE delivers per-noteId tuning at startNote, and Phase 2.3's f_final compose chain (`NE-tuned base × vibratoMult × pitchBendMult` per OQ#7-rev-3) leaves NE's role unchanged.

**Invariants preserved:** `juce::ScopedNoDenormals noDenormals;` first line of processBlock; `buffer.clear()` before any voice rendering; tone-dispatch BEFORE NE drain (Phase 2.2); `drainAndUpdate()` BEFORE `renderNextBlock` (Phase 2.1); `numSamples` cached as single local; no allocation (`getRawParameterValue` returns cached `std::atomic<float>*`; `dynamic_cast` is RTTI lookup; `SmoothedValue::skip` is stack arithmetic; `applyGainRamp` is in-place buffer arithmetic — no allocation per JUCE 8.0.4 source). Constructor's existing `synthesiser.setCurrentPlaybackSampleRate / addVoice / setAPVTS / setTuningEngine / setPendingTuningSource / addSound` chain unchanged except for the inserted setVoiceIndex loop.

---

#### 7. [ ] Extend `CMakeLists.txt` `target_sources` with Vibrato + NoiseExciter (4 new file entries)

**Files:** `plugins/O-Bassoon/CMakeLists.txt` (MODIFY)

**Depends on:** Tasks 1, 2 (source files must exist on disk before CMake config)

**Source:** RESEARCH-rev-3 §3 "plugins/O-Bassoon/CMakeLists.txt (rev-3 deltas)" — lift verbatim.

**Concrete change:** Insert four entries inside the existing `target_sources(O-Bassoon PRIVATE ...)` block (currently lines 25–41), positioned alphabetically/grouped after the `Source/Exciter.cpp` entry (line 35) and before the tuning-module entries (lines 37–40):

```cmake
        Source/Vibrato.h         # Phase 2.3 — per-voice sine LFO + onset envelope
        Source/Vibrato.cpp
        Source/NoiseExciter.h    # Phase 2.3 — per-voice continuous filtered-noise excitation
        Source/NoiseExciter.cpp
```

**Invariants preserved:** Stage 1 build flags untouched (`juce_add_plugin OBsn`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`); `juce_generate_juce_header` ordering untouched; `ouaricon_add_module(O-Bassoon note-expression)` invocation untouched (line 44); scala-tuning-engine direct-source entries untouched; `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (memory-locked Windows requirement) untouched. **Comment Exciter.h/cpp lines** as "retained for Phase 2.4 attack-character morph re-introduction; not called from BassoonVoice render path at Phase 2.3" (D6-rev-3 documentation surface) — optional but recommended for read-clarity.

---

#### 8. [ ] Backfill `research/ARCHITECTURE.md` with Phase 2.3 rev-3 note (architectural pivot + breath state machine + vibrato compose)

**Files:** `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` (MODIFY — append rev-3 note after the existing Phase 2.2 as-shipped rev-note)

**Depends on:** Task 6 (so as-shipped values for `BASE_NOISE_GAIN`, LP cutoff, CC2 takeover window, expression dispatch ordering, vibrato compose order are confirmed against source code post-edit)

**Source:** CONTEXT-rev-3 line 450 (rev-3 backfill required) + RESEARCH-rev-3 §1 OQ#7-rev-3 (compose chain documentation) + RESEARCH-rev-3 §1 OQ#5-rev-3 (CC2 takeover window value).

**Concrete change:** Append at end of document (after the existing Phase 2.2 as-shipped rev-note — match house style):

```markdown

---

## rev-note: Phase 2.3 As-Shipped (2026-04-28)

**Cycle:** GSD Phase 2.3 — Per-Note Expression: Envelope, Breath, Vibrato, Output Gain. Atomic commit subject:
`feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`.

**Architectural pivot (Phase 2.3 only):** Voice excitation source pivoted from
struck-modal-only (Phase 2.1 impulse `Exciter` + Phase 2.2 rev-3 `strike()`) to
**continuous breath-driven sustain** via new per-voice `NoiseExciter` class. Phase
2.1 `Exciter::getNextSample()` call dropped from `BassoonVoice::renderNextBlock`
(file retained verbatim for Phase 2.4 attack-character morph re-introduction).
Phase 2.2 rev-3 `strike()` retained at `BassoonVoice::startNote` as the attack
transient.

**As-shipped continuous-noise excitation spec:**
- Per-voice `juce::Random` seeded `voiceIndex × 31337` (deterministic, family-
  consistent — O-Bowed `BowNoiseGenerator.h:23` precedent).
- 1-pole low-pass filter, cutoff **2 kHz**, coefficient
  `alpha = 1 - exp(-2π × 2000 / sampleRate)`.
- Output: `lpState × BASE_NOISE_GAIN × breath_voice`.
- `BASE_NOISE_GAIN = 0.05f` as-shipped (verify-phase rev-1 ear-tuning bracket
  was `[0.03f, 0.20f]`; ship value to be inserted on Gate 3 PASS).

**Breath / dynamics state machine (CC2 takeover):**
- Composition: `breath_voice = ui_breath × cc2_normalised`.
- Velocity sets initial UI breath at `startNote` (`lastUiBreath = velocity`).
- On first CC2 event (controllerNumber == 2): `cc2EverActive = true`,
  `lastCC2SampleCount = currentSampleCount`,
  `breathSmoother.setTargetValue(lastUiBreath × cc2Normalised)`.
- CC2-takeover window: **500 ms** of CC2-idle samples before UI breath value
  resumes targeting the smoother.
- CC2 = 0 mutes the voice (audible silence; ADSR may still be active).
- Per-voice 20 ms `Linear` smoother sampled per-sample.

**Vibrato compose-order (`f_final` chain — Phase 2.3 + Phase 2.4 future-compatible):**
```
f_final = (NE-tuned base) × pow(2, vibratoCents / 1200) × pow(2, pitchBendSemitones / 12)
```
Phase 2.3 ships with `NE-tuned base = currentFrequencyBase` (12-TET fallback —
TuningEngine `getFrequency()` consumption deferred to Phase 2.4). Phase 2.4 will
plug NE-tuned base into the same multiplicative chain — vibrato + pitch-bend
operate on the NE-tuned base, not the 12-TET base. Block-rate `setFundamental`
dispatch fires when `|Δf_final| > 0.1 Hz` (sub-perceptual at C3 — 1.3 cents).

**Per-voice sine LFO vibrato spec:**
- Random initial phase per `startNote`:
  `phase = juce::Random::getSystemRandom().nextFloat() × twoPi` (O-Wind
  `FluteSynthVoice.cpp:114-116` precedent).
- Variable-duration onset `juce::SmoothedValue<float, Linear>` ramp 0→1 over
  `vibrato_onset` ms (0 ms = instant target).
- Output: `depthCents × onsetGain × std::sin(phase)`; advance phase by
  `2π × rateHz / sampleRate` per sample.
- No smoothing on rate/depth — LFO modulation masks zipper.

**Throttled-epsilon expression dispatch (processor-scope, BEFORE NE drain):**
Six APVTS reads (`attack_time`, `release_time`, `vibrato_rate`, `vibrato_depth`,
`vibrato_onset`, `breath`); single aggregated `BassoonVoice::setExpression(...)`
call per voice per block ONLY when any sub-param changes > 0.001. Per-voice
sub-param epsilon throttling inside `setExpression`. CC2 routing via
`controllerMoved` is independent of this dispatch path.

**Post-summation `output_gain` declick:**
30 ms `Linear` `juce::SmoothedValue<float>` at processor scope; per-block
`buffer.applyGainRamp(0, numSamples, smoother.getCurrentValue(),
smoother.skip(numSamples))` — JUCE 8 canonical declick-safe idiom for
`SmoothedValue`-driven buffer-level gain (deviation from O-Bowed/O-Lyrica's
plain `applyGain(decibelsToGain(level))` precedent in favour of explicit
DAW-automation declick guarantee — D4-rev-3).

**ADSR cadence:** block-rate `juce::ADSR::setParameters({attack/1000, 0, 1.0,
release/1000})` with epsilon throttle inside `setExpression`. No internal
SmoothedValue around attack/release params (JUCE ADSR re-shapes envelope
smoothly mid-note — locked OQ#2-rev-3).

**Verification:** Gate 3 PASS — see
`plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md` (rev-3). 10-item
manual checklist (3 ADSR + 1 breath + 3 vibrato + 1 output_gain + 1 long-tone-60s
+ 1 polyphony-CPU) + automated invariant battery. Closes FUNC-04, DSP-02,
DSP-04, QUAL-02, QUAL-01.
```

**If verify-phase rev-1 ear-tunes `BASE_NOISE_GAIN` outside `0.05f`** (CONTEXT-rev-3 Q4-rev-3 b2 inline iteration ceiling at rev-3): update the "As-shipped value" line with the final value before atomic commit. If the CC2 window is revised from 500 ms during iteration, update the corresponding line in the "Breath / dynamics state machine" subsection.

**Invariants preserved:** ARCHITECTURE.md above the new section is read-only at Phase 2.3 — Phase 2.2 as-shipped rev-note untouched; original §"Bassoon Partial Table" / §"Tone / Brightness Control" / §"Implementation Risks" / §"Decision Log" untouched.

---

#### 9. [ ] Build + install fresh + run static checks (10 grep gates)

**Files:** none (build artefacts only)

**Depends on:** Tasks 1–8

**Build commands** (from `/Users/taylorbrook/Dev/VST-development`):

```bash
cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone
```

Treat any new compile warning as blocking — surface to user (especially `-Wunused-private-field` on the retained-but-unused `Exciter exciter;` member per D6-rev-3; if it fires, add `(void) exciter;` in `prepareToPlay` per the D6-rev-3 mitigation).

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

**Pre-commit static checks (10 grep gates — locked from RESEARCH-rev-3 §5 — block on any failure):**

1. **RT-safety grep — zero hits in all touched files (rev-3):**
   ```bash
   grep -nE 'new |make_unique|make_shared|push_back|resize\(|malloc|calloc' \
     plugins/O-Bassoon/Source/Vibrato.h \
     plugins/O-Bassoon/Source/Vibrato.cpp \
     plugins/O-Bassoon/Source/NoiseExciter.h \
     plugins/O-Bassoon/Source/NoiseExciter.cpp \
     plugins/O-Bassoon/Source/BassoonVoice.h \
     plugins/O-Bassoon/Source/BassoonVoice.cpp \
     plugins/O-Bassoon/Source/PluginProcessor.h \
     plugins/O-Bassoon/Source/PluginProcessor.cpp
   ```
   Expected: zero matches (Phase 2.3 RT-safety regression).

2. **NE drain ordering preserved + expression dispatch placement:**
   ```bash
   grep -nC2 'vst3Extensions.drainAndUpdate\|synthesiser.renderNextBlock\|setExpression\|toneSmoother.skip' \
     plugins/O-Bassoon/Source/PluginProcessor.cpp
   ```
   Expected: ordering tone-dispatch (toneSmoother.skip block) → expression-dispatch (`setExpression` loop) → `drainAndUpdate()` → `renderNextBlock()` → output_gain (`applyGainRamp`). Reject any rearrangement.

3. **Expression dispatch site present:**
   ```bash
   grep -n 'bv->setExpression\b' plugins/O-Bassoon/Source/PluginProcessor.cpp
   ```
   Expected: ONE match in processBlock (inside the `if (anyChanged)` voice loop).

4. **`applyGainRamp` declick form locked (OQ#1-rev-3):**
   ```bash
   grep -n 'applyGainRamp *(0, numSamples,' plugins/O-Bassoon/Source/PluginProcessor.cpp
   ```
   Expected: ONE match, AFTER `synthesiser.renderNextBlock`.

5. **Mode-bank cadence — setFundamental call sites in BassoonVoice (Phase 2.3 ADDS one):**
   ```bash
   grep -n 'modeBank.setFundamental' plugins/O-Bassoon/Source/BassoonVoice.cpp
   ```
   Expected: ≥ 2 matches (startNote — Phase 2.1 carry-forward; renderNextBlock per-block compose — Phase 2.3 NEW). Phase 2.1 pitchWheelMoved match also expected; total ≥ 2 (often 3 if pitchWheelMoved still calls setFundamental directly — both acceptable, ≥ 2 is the floor).

6. **Phase 2.2 1/8 scaler retention (regression):**
   ```bash
   grep -n '1.0f / 8.0f\|0.125f' plugins/O-Bassoon/Source/ModeBank.cpp
   grep -n '1.0f / NUM_MODES\|1.0f / 16.0f' plugins/O-Bassoon/Source/ModeBank.cpp
   ```
   Expected: first grep ≥ 1 match in `processSample`; second grep zero matches.

7. **Throttle epsilon `0.001f` count locked (Phase 2.3 ADDS 6, Phase 2.2 retains 1):**
   ```bash
   grep -nc '0.001f' plugins/O-Bassoon/Source/PluginProcessor.cpp
   ```
   Expected: ≥ 7 matches (1 from Phase 2.2 tone dispatch, 6 from Phase 2.3 expression dispatch). The `setExpression` body in BassoonVoice.cpp adds a separate `EPS = 0.001f` constant — also expected to be present in BassoonVoice.cpp (≥ 1 match).

8. **DSP-07 (no O-Reed dependency) regress:**
   ```bash
   grep -rE '(O-?Reed|OReed|reed_phys|libreedphys|ReedSimulator)' \
     plugins/O-Bassoon/Source plugins/O-Bassoon/CMakeLists.txt
   ```
   Expected: zero matches.

9. **AU validation:**
   ```bash
   auval -v aumu OBsn OuDv
   ```
   Expected: `AU VALIDATION SUCCEEDED`.

10. **pluginval --strictness 5:**
    ```bash
    /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate \
      ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3
    ```
    Expected: exit code 0.

---

#### 10. [ ] Manual Gate 3 verification in Logic-AU (10-item checklist + 60 s long-tone QUAL-02)

**Files:**
- `plugins/O-Bassoon/research/reference-recordings/phase-2.3-60s-c3-vibrato-breath.wav` (NEW — 60 s Logic-AU bounce; OQ#10-rev-3 protocol)
- `plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md` (UPDATE — append Phase 2.3 rev-3 verification report)
- `plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md` (UPDATE — append Phase 2.3 execution summary)

**Depends on:** Task 9 (build install + 10 static checks all pass)

**Manual checklist** (user runs in Logic-AU; planner provides protocol — locked CONTEXT-rev-3 line 527 + OQ#10-rev-3):

1. **ADSR attack 0→2000 ms sweep** — hold sustained C3 with `breath = 0.7`, `vibrato_depth = 0`. Sweep `attack_time` slider 0 → 2000 ms across multiple note-ons. **PASS bar:** audibly different onset slopes at 0 / 500 / 1000 / 2000 ms; no clicks at any setting.

2. **ADSR release 0→3000 ms sweep** — hold then release sustained C3 with same baseline. Sweep `release_time` slider 0 → 3000 ms across multiple key releases. **PASS bar:** audibly different release tails at 0 / 1000 / 2000 / 3000 ms; no clicks at any setting.

3. **Breath UI sweep 0→1** — hold sustained C3, `vibrato_depth = 0`, no CC2 controller connected. Slowly drag `breath` slider 0.0 → 1.0 → 0.0 over ~3 s. **PASS bar:** audibly modulates sustained level; no zipper, no clicks; voice is audible-silent at breath = 0 (no clip-cut, smooth fade).

4. **CC2 real-time loudness** — connect a CC2-capable controller (or insert MIDI Learn → CC2 to a hardware fader). Hold sustained C3. Move CC2 controller. **PASS bar:** real-time loudness change tracks CC2 input; CC2 = 0 mutes voice (audible silence); CC2-takeover engages on first event (UI slider drag is ignored within the 500 ms window after CC2 activity).

5. **Vibrato 5 Hz / 50 cents at `vibrato_onset = 0` — instant audible** — set `vibrato_rate = 5`, `vibrato_depth = 50`, `vibrato_onset = 0`. Hold sustained C3. **PASS bar:** clearly audible sine pitch modulation immediately at note-on; depth ~50 cents (verifiable with stock Logic Tuner — pitch oscillates ±50 cents around C3).

6. **Vibrato `vibrato_onset = 1000 ms` fade-in measurable** — same baseline as item 5 but `vibrato_onset = 1000`. Hold sustained C3. **PASS bar:** vibrato fades in over ~1 second from no-vibrato → full ±50 cents; smooth linear ramp (no step).

7. **Vibrato `vibrato_onset = 0` instant-vibrato across multiple note-ons** — same as item 5; play C3, C4, C5 in succession with same vibrato settings + `vibrato_onset = 0`. **PASS bar:** every note-on starts with full vibrato; per-voice phase stagger is audible (not synchronised — confirms random initial phase per `Vibrato::reset()` per OQ#9-rev-3).

8. **60 s held single-note QUAL-02 final gate** — set `vibrato_rate = 5`, `vibrato_depth = 50`, `vibrato_onset = 0`, `breath = 0.7`, ADSR default. Hold C3 for 60 s in Logic-AU. Bounce to `phase-2.3-60s-c3-vibrato-breath.wav` (16-bit / 44.1 kHz stereo). **PASS bar:**
   - **(a) NaN/inf scan:** Python one-liner `python3 -c "import numpy as np, soundfile as sf; d, _ = sf.read('phase-2.3-60s-c3-vibrato-breath.wav'); print('isfinite', np.all(np.isfinite(d)), 'max', np.abs(d).max(), 'shape', d.shape)"` → must report `isfinite True`.
   - **(b) RMS-drift check:** 1-second-window RMS over the full 60 s — drift < 1 dB end-to-end (initial sustain RMS at t=10s vs t=55s within ±1 dB).
   - **(c) CPU drift:** Logic Process bar reading at t=10s vs t=60s — within ±2 % absolute.
   - **(d) ear-listen:** no audible amplitude drift, no denormal slowdown, no DC bias buildup.

9. **`output_gain` -24 dB → +6 dB sweep** — hold sustained C3 with `breath = 0.7`. Sweep `output_gain` slider -24 dB → +6 dB → -24 dB over ~5 s. **PASS bar:** smooth declick, no zipper, no clipping at +6 dB (in-resonator 1/8 scaler + breath-scaling provides headroom).

10. **8-voice CPU < 20 % with vibrato + breath active** — Logic CPU meter → System Performance Meter / Process bar. Hold the locked 8-note chord (C3+E3+G3+Bb3+C4+E4+G4+Bb4 — Phase 2.2 protocol carry-forward) with `breath = 0.7`, `vibrato_rate = 5`, `vibrato_depth = 50`, `vibrato_onset = 0`. Sustain ≥ 5 s. **PASS bar:** Process bar < 20 %. If exceeded: surface for Phase 2.4 polyphony budget; do **NOT** trigger ARCHITECTURE Risk #1 Fallback 1 (drop modes) — partial table is locked from Phase 2.2.

11. **Write VERIFICATION-rev-3.md** with results table mapping items 1–10 above to PASS / PARTIAL / DEVIATION; regression confirmation that Phase 2.1 + Phase 2.2 invariants (RT-safety grep zero, NE drain ordering, locked-Q2 grep zero, mode-index `for (int k = 5;`, 1/8 scaler, throttle epsilon, DSP-07) still hold; record final `BASE_NOISE_GAIN` value as-shipped (default 0.05f; document any rev-1 ear-tune within bracket [0.03f, 0.20f]); record any inline iteration revs (rev-1 / rev-2 / rev-3) and what changed at each.

**Final Gate 3 score** = sum of item PASS counts; bar = all 10 items PASS. Inline iteration ceiling at rev-3 (CONTEXT-rev-3 Q4-rev-3 b2): if items 5/6/7 fail, tweak `Vibrato.cpp` (phase reset / onset behaviour); if item 8 fails on (a) — rebuild with sanitiser, root-cause; if item 8 fails on (b)/(c)/(d) — adjust `BASE_NOISE_GAIN` within bracket [0.03f, 0.20f] OR add SmoothedValue around attack/release per OQ#2-rev-3 fallback; if item 10 fails — surface for Phase 2.4. Items 1/2/3/4/9 must PASS clean.

---

#### 11. [ ] Atomic commit on Gate 3 PASS

**Files:** all source edits from Tasks 1–7 + ARCHITECTURE.md backfill from Task 8 + Phase 2.3 planning artefacts (CONTEXT.md / RESEARCH.md / PLAN.md / SUMMARY.md / VERIFICATION.md) + STATUS.md update + REQUIREMENTS.md update + 60 s reference WAV

**Depends on:** Task 10 with all blocking items PASS (or documented partials within ceiling)

**Pre-commit `git status` expected files:**

```
?? plugins/O-Bassoon/Source/Vibrato.h
?? plugins/O-Bassoon/Source/Vibrato.cpp
?? plugins/O-Bassoon/Source/NoiseExciter.h
?? plugins/O-Bassoon/Source/NoiseExciter.cpp
M  plugins/O-Bassoon/Source/BassoonVoice.h
M  plugins/O-Bassoon/Source/BassoonVoice.cpp
M  plugins/O-Bassoon/Source/PluginProcessor.h
M  plugins/O-Bassoon/Source/PluginProcessor.cpp
M  plugins/O-Bassoon/CMakeLists.txt
M  plugins/O-Bassoon/.planning/research/ARCHITECTURE.md
M  plugins/O-Bassoon/.planning/STATUS.md
M  plugins/O-Bassoon/.planning/REQUIREMENTS.md
M  plugins/O-Bassoon/.planning/stages/2-dsp/CONTEXT.md          (rev-3 already written at discuss-phase)
M  plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md         (rev-3 already written at research-phase)
M  plugins/O-Bassoon/.planning/stages/2-dsp/PLAN.md             (rev-3, this addendum)
M  plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md          (Phase 2.3 subsection appended at Task 10)
M  plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md     (Phase 2.3 rev-3 subsection appended at Task 10)
?? plugins/O-Bassoon/research/reference-recordings/phase-2.3-60s-c3-vibrato-breath.wav
```

**Commit (explicit file list per CLAUDE.md "stage specific files by name"):**

```bash
git add plugins/O-Bassoon/Source/Vibrato.h \
        plugins/O-Bassoon/Source/Vibrato.cpp \
        plugins/O-Bassoon/Source/NoiseExciter.h \
        plugins/O-Bassoon/Source/NoiseExciter.cpp \
        plugins/O-Bassoon/Source/BassoonVoice.h \
        plugins/O-Bassoon/Source/BassoonVoice.cpp \
        plugins/O-Bassoon/Source/PluginProcessor.h \
        plugins/O-Bassoon/Source/PluginProcessor.cpp \
        plugins/O-Bassoon/CMakeLists.txt \
        plugins/O-Bassoon/.planning/research/ARCHITECTURE.md \
        plugins/O-Bassoon/.planning/STATUS.md \
        plugins/O-Bassoon/.planning/REQUIREMENTS.md \
        plugins/O-Bassoon/.planning/stages/2-dsp/CONTEXT.md \
        plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md \
        plugins/O-Bassoon/.planning/stages/2-dsp/PLAN.md \
        plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md \
        plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md \
        plugins/O-Bassoon/research/reference-recordings/phase-2.3-60s-c3-vibrato-breath.wav
```

**Commit message (locked subject from CONTEXT-rev-3 Q4-rev-3 batch 2):**

```
feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS

Wire four APVTS-driven expression systems and pivot voice excitation
from struck-modal-only to continuous breath-driven sustain:
- juce::ADSR wired to attack_time (0-2000 ms) + release_time (0-3000 ms),
  block-rate setParameters with epsilon-throttled dispatch.
- Breath/dynamics: breath_voice = ui_breath × cc2_normalised, velocity-as-
  initial-UI-breath, CC2-takeover state machine (500 ms idle window),
  per-voice 20 ms Linear smoother sampled per-sample.
- Per-voice sine-LFO Vibrato class (NEW): random initial phase per
  startNote, variable-duration onset SmoothedValue, multiplicative cents
  output composed with pitch-bend at f_final = base × pow(2, c/1200) ×
  pow(2, pb/12); block-rate setFundamental dispatch when |Δf| > 0.1 Hz.
- Post-summation output_gain (-24..+6 dB), 30 ms Linear smoother +
  applyGainRamp(0, numSamples, current, smoother.skip(N)).

Architectural pivot: NoiseExciter (NEW) — per-voice 1-pole LP @ 2 kHz
over white noise, BASE_NOISE_GAIN = 0.05f, scaled by breath_voice. Drops
Phase 2.1 impulse exciter from voice render path (file retained for
Phase 2.4 attack-character morph). Retains rev-3 strike() at startNote
as attack transient. Per-voice juce::Random seeded voiceIndex × 31337
(O-Bowed BowNoiseGenerator.h:23 precedent).

Aggregate setExpression(...) per-voice setter receives 6 floats per
block; processor-scope epsilon throttle (0.001f) skips dispatch when
all six APVTS values are quiescent (the 99 % case). Ordering invariant
(locked OQ#6-rev-3): tone-dispatch → expression-dispatch → NE-drain →
renderNextBlock → output_gain-applyGainRamp.

Gate 3 PASS:
- ADSR attack/release sweeps audibly different, no clicks
- Breath UI sweep + CC2 real-time loudness, mute at CC2=0
- Vibrato 5 Hz / 50 cents instant-audible; vibrato_onset 1 s fade-in
  measurable; per-voice phase stagger audible (random phase reset)
- 60 s held C3 + vibrato + breath: isfinite True, RMS drift < 1 dB,
  CPU drift < 2 % (QUAL-02 final gate)
- output_gain -24..+6 dB sweep clean
- 8-voice + vibrato + breath CPU < 20 % (PERF-02 early signal)
- Phase 2.1 + Phase 2.2 invariants regress-clean

Closes: FUNC-04, DSP-02, DSP-04, QUAL-02, QUAL-01.
Defers to Phase 2.4: DSP-05 (attack_character), DSP-06 (NE/MPE
consumption), FUNC-02/05 (polyphony cap + voice stealing).

Files:
- Source/Vibrato.{h,cpp} (NEW): per-voice sine LFO + onset envelope
- Source/NoiseExciter.{h,cpp} (NEW): per-voice continuous LP-noise
- Source/BassoonVoice.{h,cpp}: setExpression aggregate setter, CC2
  takeover state machine, vibrato compose, NoiseExciter integration,
  Phase 2.1 exciter call dropped (member retained for Phase 2.4)
- Source/PluginProcessor.{h,cpp}: outputGainSmoother + 6 dispatch
  shadows; setVoiceIndex wire in constructor; expression dispatch +
  applyGainRamp in processBlock
- CMakeLists.txt: target_sources adds Vibrato + NoiseExciter
- research/ARCHITECTURE.md: Phase 2.3 as-shipped rev-3 note appended
  (architectural pivot, breath state machine, vibrato compose chain)
- .planning/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}.md: rev-3
- research/reference-recordings/phase-2.3-60s-c3-vibrato-breath.wav

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
```

**Pre-commit grep regression suite** (re-run before commit — locked Task 9 #1–#10):
- RT-safety grep zero hits across all 8 touched source files (Task 9 #1)
- NE drain ordering preserved + expression-dispatch placement (Task 9 #2)
- `setExpression` dispatch site present in processBlock (Task 9 #3)
- `applyGainRamp(0, numSamples,` form locked (Task 9 #4)
- `modeBank.setFundamental` ≥ 2 hits in BassoonVoice.cpp (Task 9 #5)
- Phase 2.2 1/8 scaler retained, 1/16 absent (Task 9 #6)
- Throttle epsilon `0.001f` ≥ 7 hits in PluginProcessor.cpp (Task 9 #7)
- DSP-07 zero matches (Task 9 #8)
- AU validation SUCCEEDED (Task 9 #9)
- pluginval --strictness 5 exit 0 (Task 9 #10)

---

### Files Created / Modified Summary (rev-3)

| Operation | File |
|---|---|
| NEW | `plugins/O-Bassoon/Source/Vibrato.h` |
| NEW | `plugins/O-Bassoon/Source/Vibrato.cpp` |
| NEW | `plugins/O-Bassoon/Source/NoiseExciter.h` |
| NEW | `plugins/O-Bassoon/Source/NoiseExciter.cpp` |
| MODIFY | `plugins/O-Bassoon/Source/BassoonVoice.h` (Phase 2.3 members + setExpression + setVoiceIndex) |
| MODIFY | `plugins/O-Bassoon/Source/BassoonVoice.cpp` (prepareToPlay + startNote + controllerMoved + setExpression + renderNextBlock pivot) |
| MODIFY | `plugins/O-Bassoon/Source/PluginProcessor.h` (outputGainSmoother + 6 dispatch shadows) |
| MODIFY | `plugins/O-Bassoon/Source/PluginProcessor.cpp` (setVoiceIndex wire + prepareToPlay + processBlock expression dispatch + applyGainRamp) |
| MODIFY | `plugins/O-Bassoon/CMakeLists.txt` (target_sources +4 entries) |
| MODIFY | `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` (Phase 2.3 rev-3 note appended) |
| MODIFY | `plugins/O-Bassoon/.planning/STATUS.md` (plan_complete → execute → verify) |
| MODIFY | `plugins/O-Bassoon/.planning/REQUIREMENTS.md` (FUNC-04 / DSP-02 / DSP-04 / QUAL-02 / QUAL-01 → complete on Gate 3 PASS) |
| MODIFY | `plugins/O-Bassoon/.planning/stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}.md` (rev-3 addenda) |
| NEW | `plugins/O-Bassoon/research/reference-recordings/phase-2.3-60s-c3-vibrato-breath.wav` |
| UNCHANGED | `plugins/O-Bassoon/Source/Exciter.{h,cpp}` (D6-rev-3 retention — Phase 2.4 re-wires) |
| UNCHANGED | `plugins/O-Bassoon/Source/ModeBank.{h,cpp}` (Phase 2.2 locked: bassoon partial table, formant Gaussian, tone wiring, 1/8 scaler, rev-3 strike()) |
| UNCHANGED | `plugins/O-Bassoon/Source/{BassoonSound.h, PluginEditor.{h,cpp}}` |

### Dependencies (DAG — rev-3)

```
Task 1 (Vibrato) ─┐
Task 2 (NoiseExciter) ─┐
                       ├─→ Task 3 (BassoonVoice.h) ─┐
                       │                            ├─→ Task 4 (BassoonVoice.cpp) ─┐
                       │                            │                              │
                       └─────────────────────────────┘                              │
                                                                                   │
Task 5 (PluginProcessor.h) ────────────────────────────────────────────────────────┤
                                                                                   │
                                                Task 6 (PluginProcessor.cpp) ←─────┘
                                                          │
Task 7 (CMakeLists) ←─ Tasks 1, 2 ────────────────────────┤
                                                          │
                                                          ↓
                                                Task 8 (ARCHITECTURE.md) ←─ Task 6
                                                          │
                                                          ↓
                                                Task 9 (build + 10 grep gates)
                                                          │
                                                          ↓
                                                Task 10 (manual Gate 3 — 10 items)
                                                          │
                                                          ↓
                                                Task 11 (atomic commit)
```

### Success Criteria (Gate 3)

A Gate 3 PASS requires ALL of:

- [ ] Build clean (Task 9 — VST3 + AU + Standalone, zero warnings; D6-rev-3 unused-private-field mitigated if it fires)
- [ ] AU validation SUCCEEDED (Task 9 #9)
- [ ] pluginval --strictness 5 exit 0 (Task 9 #10)
- [ ] RT-safety grep zero across all 8 touched source files (Task 9 #1)
- [ ] NE drain ordering + expression-dispatch placement preserved (Task 9 #2)
- [ ] `setExpression` dispatch site present (Task 9 #3)
- [ ] `applyGainRamp(0, numSamples,` form locked (Task 9 #4)
- [ ] `modeBank.setFundamental` ≥ 2 hits in BassoonVoice.cpp (Task 9 #5)
- [ ] Phase 2.2 1/8 scaler retained; 1/16 absent (Task 9 #6)
- [ ] Throttle epsilon `0.001f` ≥ 7 hits in PluginProcessor.cpp (Task 9 #7)
- [ ] DSP-07 zero matches (Task 9 #8)
- [ ] ADSR attack/release sweeps audibly different, no clicks (Task 10 #1, #2)
- [ ] Breath UI sweep audible + CC2 real-time loudness + mute at CC2=0 (Task 10 #3, #4)
- [ ] Vibrato 5 Hz / 50 cents at onset=0 instant-audible (Task 10 #5)
- [ ] Vibrato onset=1000 ms fade-in measurable (Task 10 #6)
- [ ] Vibrato per-voice phase stagger audible (Task 10 #7)
- [ ] 60 s held C3 + vibrato + breath QUAL-02 (a) isfinite True (b) RMS drift < 1 dB (c) CPU drift < 2 % (d) ear-stable (Task 10 #8)
- [ ] output_gain -24..+6 dB sweep clean (Task 10 #9)
- [ ] 8-voice + vibrato + breath CPU < 20 % (Task 10 #10)
- [ ] VERIFICATION-rev-3 written + SUMMARY-rev-3 appended (Task 10 #11)
- [ ] ARCHITECTURE.md rev-3 note appended (Task 8)
- [ ] Atomic commit landed with locked subject (Task 11)

**Iteration ceiling** (CONTEXT-rev-3 Q4-rev-3 b2): if Task 10 items 5/6/7/8/9 fail, iterate inline (no replan loop) — tweak `BASE_NOISE_GAIN` within bracket [0.03f, 0.20f], or add SmoothedValue around attack/release per OQ#2-rev-3 fallback, or upgrade `Vibrato::reset` to per-voice `juce::Random` per D7-rev-3 fallback. Ceiling at rev-3. After rev-3, ship and document v1.0 gap as v1.1 candidate. If item 10 (8-voice CPU) fails — do **NOT** drop modes (partial table is locked from Phase 2.2); surface for Phase 2.4 polyphony budget. Items 1/2/3/4 must PASS clean.

### Risks Carried (from CONTEXT-rev-3)

1. **Continuous-noise excitation regresses Phase 2.2 timbre** — mitigated by `BASE_NOISE_GAIN = 0.05f` starting point + verify-phase rev-1 ear-tune within bracket [0.03f, 0.20f]. FUNC-01 ear A/B vs Phase 2.2 reference at item 11 PASS bar.
2. **CC2-takeover state machine flicker** — mitigated by 500 ms window (locked OQ#5-rev-3); revisable to 1000 ms inline if flicker observed.
3. **Vibrato rate/depth zipper without smoothing** — locked accept; LFO modulation masks. Inline mitigation: 50 ms `Linear` SmoothedValue per parameter (per-voice) if audible.
4. **60 s sustain CPU drift / denormal accumulation** — `juce::ScopedNoDenormals` already in place; verify at QUAL-02 item 8 (a)/(c).
5. **`output_gain` post-summation declick on rapid sweep** — `applyGainRamp(0, numSamples, current, skip(N))` is the locked declick-safe idiom (OQ#1-rev-3). Verify at item 9.
6. **Breath multiplicative compose with `breath_voice = 0` mute → stuck-voice perception** — ADSR release proceeds normally (timer-based); voice-state transitions unaffected. Verify: hold note with breath=0, release key, confirm voice exits at ADSR-release-end.
7. **`juce::Random` per-voice seeding race** — mitigated by deterministic `voiceIndex × 31337` (OQ#3-rev-3 lock).
8. **Drop of Phase 2.1 impulse exciter — Phase 2.4 re-wire risk** — `Exciter.{h,cpp}` files unchanged; `exciter` member retained per D6-rev-3.
9. **8-voice CPU regression with vibrato + breath active** — early signal at item 10; if exceeded, surface for Phase 2.4 budget.

### Out of Scope (deferred per ROADMAP)

- `attack_character` morph (DSP-05) — Phase 2.4. `Exciter.{h,cpp}` retained for re-wire.
- Sustain-noise component placed via `attack_character` — Phase 2.4 (Phase 2.3's `NoiseExciter` is the primary continuous excitation source; Phase 2.4 may augment with attack-only impulse layer but does NOT replace `NoiseExciter`).
- `voice_count` APVTS read + voice manager / `findFreeVoice` override (FUNC-02 / FUNC-05) — Phase 2.4
- VST3 NE per-voice consumption (`applyPendingTuning`) — Phase 2.4 (drain wires already in place; voices ignore the table at Phase 2.3)
- MPE pitch-bend per-channel (raw 14-bit `pitchWheelValue` already used at Phase 2.1/2.2 — Phase 2.4 may refine)
- TuningEngine `getFrequency()` call in `startNote` — Phase 2.4
- Aftertouch → vibrato_depth modulation — v1.1 (Stage 0 D4)
- CC1 (mod wheel → vibrato_depth) — v1.1
- Two-register-table fallback (ARCHITECTURE Risk #2 Fallback 1) — Phase 2.2 verify cleared with rev-3; not invoked
- pluginval `--strictness 10` + Windows VST3 build — Stage 4
- Dorico parity — Stage 4

### Inputs Consumed Verbatim (rev-3)

- **CONTEXT-rev-3** §"Cycle Scope (rev-3)" + §"Constraints Identified (rev-3)" + §"Approach Decisions (rev-3)" Q1–Q4 batch 1 + Q1–Q4 batch 2 + 12 derived locks — locked scope, dispatch design, Gate 3 bar, atomic commit subject
- **RESEARCH-rev-3** §3 "Implementation Skeletons (rev-3)" — Vibrato.h/.cpp + NoiseExciter.h/.cpp + BassoonVoice rev-3 deltas + PluginProcessor rev-3 deltas + CMakeLists rev-3 deltas consumed as task body content (Tasks 1–7)
- **RESEARCH-rev-3** §1 OQ#1-rev-3 — `applyGainRamp(0, numSamples, current, skip(N))` declick-safe idiom locked
- **RESEARCH-rev-3** §1 OQ#2-rev-3 — block-rate ADSR `setParameters` with epsilon throttle (no internal smoother)
- **RESEARCH-rev-3** §1 OQ#3-rev-3 — per-voice `juce::Random` with `voiceIndex × 31337` seed (D3-rev-3 override)
- **RESEARCH-rev-3** §1 OQ#4-rev-3 — `BASE_NOISE_GAIN = 0.05f` starting point; verify bracket [0.03f, 0.20f]
- **RESEARCH-rev-3** §1 OQ#5-rev-3 — CC2 takeover window 500 ms
- **RESEARCH-rev-3** §1 OQ#6-rev-3 — dispatch ordering tone → expression → NE-drain → render → output_gain
- **RESEARCH-rev-3** §1 OQ#7-rev-3 — `f_final = NE-tuned × vibratoMult × pitchBendMult` compose chain
- **RESEARCH-rev-3** §1 OQ#8-rev-3 — CC2 normalisation at `controllerMoved` callback
- **RESEARCH-rev-3** §1 OQ#9-rev-3 — random vibrato phase per startNote (D2-rev-3 override)
- **RESEARCH-rev-3** §1 OQ#10-rev-3 — 60 s QUAL-02 protocol (Logic-AU bounce + Python `numpy.isfinite` + RMS drift + CPU drift)
- **RESEARCH-rev-3** §5 — 10 static-check grep gates locked verbatim (Task 9)

### Audit Trail (rev-3 addendum)

**rev-3 (this addendum, 2026-04-28):** Phase 2.3 plan — Per-Note Expression: Envelope, Breath, Vibrato, Output Gain. 11 single-Wave tasks (4 source-file creates + 4 source-file modifies + 1 CMake edit + 1 spec backfill + 1 build/static-check + 1 manual Gate 3 verify + 1 atomic commit). Lifts RESEARCH-rev-3 §3 implementation skeletons verbatim. Pins Gate 3 PASS bar from CONTEXT-rev-3 line 527 (10-item: 3 ADSR + 1 breath + 3 vibrato + 1 output_gain + 1 long-tone-60s + 1 polyphony-CPU). Atomic commit subject locked: `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`.

**Inherited from Phase 2.1 PLAN-rev-1 + Phase 2.2 PLAN-rev-2 + Phase 2.3 CONTEXT-rev-3 + RESEARCH-rev-3 (not re-litigated):**
- Per-sample render-loop ordering (`excitation → modeBank → adsr → addSample`) — Phase 2.3 changes excitation source (impulse → continuous noise), preserves pipeline
- Centred equal L+R per-sample voice write
- NE drain BEFORE renderNextBlock at PluginProcessor.cpp:200 → :203
- DSP-07 (no O-Reed dependency) carry-forward
- Reference WAVs at `research/reference-recordings/bassoon-c3-sustain-v{1,2}.wav` (Phase 2.1, read-only at Phase 2.3)
- Atomic-commit gate-first principle
- Logic Pro (AU) primary listening DAW; Logic CPU System Performance Meter / Process bar
- Bassoon-tuned partial table + formant Gaussian × 1/k roll-off (Phase 2.2 — locked)
- Tone smoother (50 ms Linear) + throttled-epsilon (0.001) dispatch (Phase 2.2 — locked)
- 1/8 headroom scaler (Phase 2.2 — relaxed from 1/16)
- rev-3 strike() at startNote (Phase 2.2 patch — retained as attack transient)
- Phase 2.2 8-note CPU chord protocol (C3+E3+G3+Bb3+C4+E4+G4+Bb4)

**New in rev-3:**
- Cycle scope = Phase 2.3 only (4 APVTS-driven systems: ADSR + breath + vibrato + output_gain; architectural pivot continuous-noise excitation)
- 4 NEW source files: `Source/{Vibrato,NoiseExciter}.{h,cpp}`
- 4 MODIFY source files: `Source/{BassoonVoice,PluginProcessor}.{h,cpp}`
- 1 MODIFY CMakeLists.txt (`target_sources` +4 entries)
- 1 MODIFY ARCHITECTURE.md (Phase 2.3 as-shipped rev-3 note: pivot + breath state machine + vibrato compose chain)
- Aggregate `setExpression(...)` per-voice setter (6 floats, sub-param epsilon throttling)
- CC2-takeover state machine (500 ms idle window, sample-count-based)
- Per-voice 20 ms breath smoother (sample-rate `getNextValue` per sample)
- Processor-level 30 ms output_gain smoother + `applyGainRamp(0, numSamples, current, skip(N))` declick-safe idiom
- Variable-duration vibrato_onset SmoothedValue (Linear; 0 ms = instant target)
- Random vibrato phase per startNote (D2-rev-3 override of CONTEXT default)
- Deterministic `voiceIndex × 31337` noise seed (D3-rev-3 override of CONTEXT default)
- Block-rate ADSR `setParameters` with epsilon throttle (no internal smoother — OQ#2-rev-3)
- Frequency compose chain `f_final = NE-tuned × vibratoMult × pitchBendMult` (block-rate setFundamental dispatch when `|Δf| > 0.1 Hz`)
- 60 s long-tone QUAL-02 protocol (Python `numpy.isfinite` + 1-sec-window RMS drift + Logic CPU drift)
- 10-item Gate 3 PASS bar
- 10 static-check grep gates (RESEARCH-rev-3 §5)
- Phase 2.1 impulse `Exciter` call dropped from `BassoonVoice::renderNextBlock` (file + member retained per D6-rev-3 for Phase 2.4 re-wire)
- Inline iteration ceiling at rev-3 (Phase 2.2 precedent)
- Atomic commit on Gate 3 PASS with subject `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`

### Next Phase (rev-3)

Ready for: **execute** phase — `/clear` then `/plugin-execute O-Bassoon 2-dsp`

---

## rev-4 — Phase 2.4 Plan (2026-04-29)

### Goal

Close the 4 remaining DSP requirements in a single coupled cycle: **FUNC-02** (polyphony 1–16, default 8), **FUNC-05** (voice stealing, release-tail-first), **DSP-05** (attack-character morph between `softShape` and new `tonguedShape` with velocity bias), and **DSP-06** (microtonal pitch via `TuningEngine::getFrequency` + per-noteId NE `applyPendingTuning` snapshot at `startNote` + per-channel MPE pitch-bend). Plus **QUAL-02** 60 s long-tone gate (Phase 2.3 deferred) and **PERF-02** 8-voice CPU final under enforced cap. Stage 2 reaches feature-complete on Gate 4 PASS.

### Wave Structure

Single wave; tasks sequenced by data dependencies. Phase 2.3 atomic commit landing on `main` is a hard gate before execute-phase begins (CONTEXT-rev-4 process invariant).

### Tasks

#### Task 0 — Hard gate: Phase 2.3 atomic commit landed on `main`

- [ ] Verify `git log --oneline plugins/O-Bassoon | head -1` shows `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS` as the most recent O-Bassoon commit.
- [ ] If absent, STOP — surface to user that Phase 2.3 atomic commit must land before Phase 2.4 execute begins (CONTEXT-rev-4 §"Phase 2.3 atomic commit MUST land BEFORE Phase 2.4 execute-phase begins").
- Files: none.
- Depends on: none.
- Acceptance: most recent O-Bassoon commit subject matches Phase 2.3 atomic commit lock.

#### Task 1 — Create `Source/BassoonSynthesiser.{h,cpp}` (NEW)

- [ ] Create `Source/BassoonSynthesiser.h` — subclass `juce::Synthesiser`. Constructor calls `setNoteStealingEnabled(true)` (explicit-for-clarity; JUCE 8 default). Public `setActiveVoiceCap(int cap) noexcept` clamped via `juce::jlimit(1, 16, cap)`. Public `getActiveVoiceCap()`. Protected `findFreeVoice` override (`const`, matches base virtual signature): walk active voices via `getNumVoices() + getVoice(i)->isVoiceActive()` (manual loop — no `getNumActiveVoices()` in JUCE 8 per OQ#9-rev-4); if `active < activeVoiceCap`, delegate to `juce::Synthesiser::findFreeVoice(...)`; else if `stealIfNoneAvailable`, return `findVoiceToSteal(...)` (JUCE default — release-tail-first, then oldest-noteOn per OQ#1-rev-4); else `nullptr`. Private `int activeVoiceCap = 16;` (matches Stage 1 pre-allocated 16-voice pool).
- [ ] Create `Source/BassoonSynthesiser.cpp` — `.h+.cpp` pair convention per Phase 2.3 precedent (NoiseExciter, Vibrato shipped as pairs). Body contains `#include "BassoonSynthesiser.h"` and a single placeholder definition (e.g., `void touchBassoonSynthesiserTU() noexcept {}`) to avoid "no symbols" linker warnings on toolchains that flag empty TUs (D1-rev-4 resolution).
- Files: `plugins/O-Bassoon/Source/BassoonSynthesiser.h` (NEW), `plugins/O-Bassoon/Source/BassoonSynthesiser.cpp` (NEW).
- Depends on: Task 0.
- Acceptance: header lifts RESEARCH-rev-4 §3 skeleton verbatim (lines 3601–3666); compiles standalone via `target_sources` addition (Task 6).

#### Task 2 — Modify `Source/Exciter.{h,cpp}` (MOD)

- [ ] Rename Phase 2.1 `onsetBuffer` → `softShape` (D3-rev-4 mechanical refactor; member is private — no caller updates). Verify post-edit: `grep -rn "onsetBuffer" plugins/O-Bassoon/Source/` returns zero hits.
- [ ] Add new `std::array<float, MAX_ONSET_SAMPLES> tonguedShape{};` private member (zero-init by default).
- [ ] Add `static constexpr float TONGUED_DURATION_MS = 7.5f;` and `static constexpr float VELOCITY_BIAS_MAGNITUDE = 0.3f;` (OQ#3-rev-4, OQ#4-rev-4 locked).
- [ ] Add public `startOnset(float attackChar01, float velocity01) noexcept`: snapshots `effectiveAttackChar = juce::jlimit(0.0f, 1.0f, attackChar01 + (velocity01 - 0.5f) * VELOCITY_BIAS_MAGNITUDE)` for the lifetime of the onset window (mid-onset automation does NOT affect in-flight onset — risk #2 mitigation). Resets `onsetIdx = 0; active = true`.
- [ ] Retain `start() noexcept { startOnset(0.0f, 1.0f); }` as thin wrapper (D6-rev-4 backwards compatibility — though Phase 2.4 callers use `startOnset` directly).
- [ ] Modify `getNextSample()` to return `juce::jmap(effectiveAttackChar, softShape[i], tonguedShape[i])` for `onsetIdx < onsetSamples`; auto-zero past `onsetSamples` (cleared `active` flag + return 0).
- [ ] Add private `int onsetSamples = 0;` (length of longer of the two windows — set at `prepare()`); private `float effectiveAttackChar = 0.0f;` (default-init, overwritten at every `startOnset`); private `bool active = false;`.
- [ ] In `Exciter.cpp::prepare(double sampleRate)`:
  - Generate `softShape[i] = sin(π·i/softN) × exp(-t / SOFT_TAU_MS·0.001f)` for `i ∈ [0, softN)` where `softN = min(MAX_ONSET_SAMPLES, sampleRate × SOFT_DURATION_MS × 0.001)`. Peak-normalise.
  - Generate `tonguedShape[i] = (rng.nextFloat() × 2.0f - 1.0f) × exp(-i / tonguedN × 4.0f)` for `i ∈ [0, tonguedN)` where `tonguedN = min(MAX_ONSET_SAMPLES, sampleRate × TONGUED_DURATION_MS × 0.001)`. Use `juce::Random rng(12345)` (deterministic seed per OQ#3-rev-4). Peak-normalise.
  - Set `onsetSamples = max(softN, tonguedN)` (D2-rev-4 — `softShape` zero-pad beyond `softN` is auto-handled by `std::array` zero-init).
  - Call `reset()`.
- Files: `plugins/O-Bassoon/Source/Exciter.h`, `plugins/O-Bassoon/Source/Exciter.cpp`.
- Depends on: Task 0.
- Acceptance: lifts RESEARCH-rev-4 §3 skeleton verbatim (lines 3677–3795); `start()` wrapper preserved; `softShape` rename grep-clean.

#### Task 3 — Modify `Source/BassoonVoice.cpp::startNote` (MOD)

- [ ] **Replace** the Phase 2.3 `currentFrequencyBase = ...` assignment block with the Phase 2.4 compose chain (RESEARCH-rev-4 §3, lines 3808–3821):
  ```cpp
  double f_double = (tuningEngine != nullptr)
      ? tuningEngine->getFrequency (midiNoteNumber)
      : juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

  if (pendingTuningSource != nullptr)
      f_double = Ouaricon::NoteExpression::applyPendingTuning (
                     *pendingTuningSource, midiNoteNumber, f_double);

  currentFrequencyBase = static_cast<float> (f_double);
  ```
- [ ] **Replace** the existing `exciter.start();` call with:
  ```cpp
  const float attackChar = parameters->getRawParameterValue ("attack_character")->load();
  exciter.startOnset (attackChar, velocity);
  ```
  Note: `velocity` is the `startNote` parameter (already in scope per JUCE `SynthesiserVoice::startNote` signature).
- [ ] Verify `#include "BassoonSynthesiser.h"` is NOT needed in voice (voice doesn't reference the synthesiser type).
- [ ] Verify `Ouaricon::NoteExpression::applyPendingTuning` include is in place — header `modules/tuning/note-expression/cpp/NoteExpression.h` should already be transitively pulled via Stage 1 wiring; if not, add explicit `#include`.
- Files: `plugins/O-Bassoon/Source/BassoonVoice.cpp`.
- Depends on: Tasks 1, 2.
- Acceptance: 4 new lines (TuningEngine call + applyPendingTuning + currentFrequencyBase assignment + Exciter.startOnset) replace Phase 2.3 form; vibrato/pitch-bend/`modeBank.setFundamental` per-block compose chain is unchanged.

#### Task 4 — Modify `Source/BassoonVoice.cpp::renderNextBlock` (MOD)

- [ ] **Modify** the per-sample inner loop to add additive Exciter contribution (RESEARCH-rev-4 §3, lines 3839–3848):
  ```cpp
  const float breath        = breathSmoother.getNextValue();
  const float noiseSample   = noiseExciter.getNextSample (breath);
  const float exciterSample = exciter.getNextSample();   // 0 after onset window
  const float excitation    = noiseSample + exciterSample;
  float voice               = modeBank.processSample (excitation);
  voice                    *= adsr.getNextSample();
  outputBuffer.addSample (0, startSample + i, voice);
  outputBuffer.addSample (1, startSample + i, voice);
  ```
  Single-line addition: `exciter.getNextSample()` summed with `noiseExciter.getNextSample(breath)`. After onset window, Exciter auto-returns 0 — only NoiseExciter sustains (OQ#8-rev-4 additive composition lock).
- [ ] Verify `stopNote` path still calls `exciter.reset()` (Phase 2.3 retention — already in tree at BassoonVoice.cpp:106).
- Files: `plugins/O-Bassoon/Source/BassoonVoice.cpp`.
- Depends on: Task 3.
- Acceptance: 1-line Exciter contribution added to render loop; per-block `f_final` recompute + `modeBank.setFundamental` dispatch chain (Phase 2.3) unchanged.

#### Task 5 — Modify `Source/PluginProcessor.{h,cpp}` (MOD)

- [ ] In `PluginProcessor.h`: add `#include "BassoonSynthesiser.h"`. **Type-swap** `juce::Synthesiser synthesiser;` → `BassoonSynthesiser synthesiser;` (single-line member type change; name preserved). Add private member `int lastDispatchedVoiceCount = -1;` (forces first-block dispatch — OQ#2-rev-4).
- [ ] In `PluginProcessor.cpp::processBlock` prologue head (BEFORE tone-dispatch — OQ#2-rev-4 lock site):
  ```cpp
  // Phase 2.4 (FUNC-02): voice_count snapshot at processBlock prologue head.
  // Applies on next note-on; already-active voices unaffected per ROADMAP.
  // Integer-comparison throttle (no float epsilon needed for AudioParameterInt).
  const int requestedVoices = static_cast<int> (
      parameters.getRawParameterValue ("voice_count")->load());
  if (requestedVoices != lastDispatchedVoiceCount)
  {
      synthesiser.setActiveVoiceCap (requestedVoices);
      lastDispatchedVoiceCount = requestedVoices;
  }
  ```
- [ ] Verify the existing constructor `synthesiser.addVoice(...)` loop still compiles unchanged (`addVoice` is inherited from `juce::Synthesiser`, not overridden).
- [ ] Verify `prepareToPlay` requires no Phase 2.4 deltas (16-voice pool already pre-allocated at Stage 1; cap snapshot fires at first `processBlock` call).
- Files: `plugins/O-Bassoon/Source/PluginProcessor.h`, `plugins/O-Bassoon/Source/PluginProcessor.cpp`.
- Depends on: Task 1.
- Acceptance: `BassoonSynthesiser synthesiser` member at PluginProcessor.h; voice_count snapshot at `processBlock` prologue head BEFORE tone-dispatch; Phase 2.3 prologue ordering preserved (tone → expression → NE-drain → render → output_gain).

#### Task 6 — Modify `plugins/O-Bassoon/CMakeLists.txt` (MOD)

- [ ] Add `Source/BassoonSynthesiser.h` and `Source/BassoonSynthesiser.cpp` to the `target_sources(...)` block. Maintain alphabetical-ish grouping consistent with Phase 2.3 (NoiseExciter, Vibrato pairs).
- [ ] Verify build flags unchanged: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `PLUGIN_CODE OBsn`, `juce_generate_juce_header` after `target_link_libraries`.
- Files: `plugins/O-Bassoon/CMakeLists.txt`.
- Depends on: Task 1.
- Acceptance: `target_sources` lists +2 entries (BassoonSynthesiser.h + .cpp); flags unchanged.

#### Task 7 — Append `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` rev-4 note

- [ ] Append the Phase 2.4 as-shipped backfill section (RESEARCH-rev-4 §3 lines 3917–3932 template). Documents: (a) `BassoonSynthesiser` subclass + cap-with-stealing semantics; (b) Exciter dual-shape morph + velocity bias formula `effective = clamp(attackChar + (vel-0.5)*0.3, 0, 1)` + onset-window latch; (c) `f_base` compose chain at startNote (`TuningEngine.getFrequency → applyPendingTuning → currentFrequencyBase`); (d) NoiseExciter additive-during-onset behaviour; (e) MPE per-channel pitch-bend routing via `juce::Synthesiser::handlePitchWheel` (OQ#7-rev-4); (f) regression invariants list (Phase 2.1–2.3 patterns preserved).
- Files: `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md`.
- Depends on: Tasks 1–6.
- Acceptance: rev-4 note appended at end of file; existing rev-1/rev-2/rev-3 notes unmodified.

#### Task 8 — Build + 16-item static-check grep battery (auto-Gate 4)

- [ ] Build: `cmake --build build --config Release --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone --parallel` (or `ninja O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone`). Zero warnings.
- [ ] macOS cache clear + fresh install per CLAUDE.md project guidelines:
  ```bash
  killall -9 AudioComponentRegistrar 2>/dev/null || true
  rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
  rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3
  rm -rf ~/Library/Audio/Plug-Ins/Components/O-Bassoon-dev.component
  cp -R build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/VST3/O-Bassoon-dev.vst3 ~/Library/Audio/Plug-Ins/VST3/
  cp -R build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/AU/O-Bassoon-dev.component ~/Library/Audio/Plug-Ins/Components/
  ```
- [ ] **16-item static-check grep battery** (RESEARCH-rev-4 §5 lines 3982–3999):
  1. RT-safety zero-match — `grep -rn "new\\|make_unique\\|make_shared\\|push_back\\|resize\\|malloc\\b" Source/{BassoonSynthesiser,Exciter,BassoonVoice,PluginProcessor}.{h,cpp}` → 0 matches.
  2. NE-drain ordering — `grep -n "drainAndUpdate\\|renderNextBlock" Source/PluginProcessor.cpp` → drain BEFORE renderNextBlock (1 hit each, drain line < render line).
  3. Type swap — `grep -n "BassoonSynthesiser synthesiser" Source/PluginProcessor.h` → 1 match; `grep -n "juce::Synthesiser synthesiser" Source/PluginProcessor.h` → 0 matches.
  4. voice_count snapshot — `grep -n "synthesiser.setActiveVoiceCap" Source/PluginProcessor.cpp` → 1 match in `processBlock` prologue head, BEFORE tone-dispatch.
  5. NE per-voice — `grep -n "applyPendingTuning" Source/BassoonVoice.cpp` → 1 match in `startNote`.
  6. TuningEngine call — `grep -n "tuningEngine->getFrequency" Source/BassoonVoice.cpp` → 1 match in `startNote`.
  7. Exciter morph — `grep -n "exciter.startOnset" Source/BassoonVoice.cpp` → 1 match in `startNote`.
  8. Additive composition — `grep -n "exciter.getNextSample" Source/BassoonVoice.cpp` → 1 match in `renderNextBlock` (paired with `noiseExciter.getNextSample`).
  9. DSP-07 — `grep -rn "O-Reed\\|OReed\\|reed-" Source/` → 0 matches.
  10. 1/8 scaler retention — `grep -n "0.125f\\|/ 8.0f" Source/ModeBank.cpp` → 1 match in `processSample` (Phase 2.2 retention).
  11. Throttle epsilon — `grep -c "0.001f" Source/PluginProcessor.cpp` → ≥10 hits (Phase 2.3 expression dispatch carry-forward).
  12. setExpression site — `grep -c "setExpression\\b" Source/PluginProcessor.cpp` → 1 match (Phase 2.3 carry-forward; Phase 2.4 does NOT add `attack_character` to setExpression — it's a startNote-only snapshot).
  13. applyGainRamp form — `grep -n "applyGainRamp" Source/PluginProcessor.cpp` → 1 match AFTER `synthesiser.renderNextBlock`.
  14. modeBank.setFundamental — `grep -c "modeBank.setFundamental" Source/BassoonVoice.cpp` → 3 matches (startNote / pitchWheelMoved / renderNextBlock per-block).
  15. `auval -v aumu OBsn OuDv` → exit 0 / VALIDATION SUCCEEDED.
  16. `pluginval --strictness 5 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3` → exit 0.
- Files: none (verification only).
- Depends on: Tasks 1–7.
- Acceptance: build clean, all 16 grep gates PASS.

#### Task 9 — Manual Gate 4 — 10-item user-checkable bar

- [ ] **Item 1** — 8 simultaneous notes audibly distinct: Logic-AU, hold C2/E2/G2/Bb2/C3/E3/G3/Bb3 (8-note chord), confirm 8 voices ring distinctly.
- [ ] **Item 2** — Voice cap + stealing: in Logic-AU, set `voice_count = 3`, play 4 sequential notes — only 3 voices sound; the oldest (or release-tail-first) is stolen. Verify with chord-spread test.
- [ ] **Item 3** — Rapid retrigger: 10 Hz alternating noteOn/noteOff × 30 s with `voice_count = 1` (mono mode). No stuck notes; no audio artefacts beyond the expected per-note retrigger transient.
- [ ] **Item 4** — Attack-character soft + low velocity: `attack_character = 0.0`, velocity ≈ 20, play C3 — audibly soft, gentle attack onset.
- [ ] **Item 5** — Attack-character tongued + high velocity: `attack_character = 1.0`, velocity ≈ 120, play C3 — audibly percussive, noise-burst attack.
- [ ] **Item 6** — Mid-morph: `attack_character = 0.5`, velocity ≈ 70, play C3 — smooth blend; no audible discontinuity sweeping `attack_character` 0 → 1 across successive note-ons.
- [ ] **Item 7** — MPE per-channel pitch-bend: Bitwig Studio (or equivalent MPE-enabled DAW) with MPE controller (LinnStrument / ROLI Seaboard / virtual MPE keyboard plugin). Hold a chord; bend one note independently — only that voice retunes. **If no MPE controller available**, document as Stage 4 deferred and skip without penalty (OQ#10-rev-4 fallback).
- [ ] **Item 8** — VST3 NE pitch event verification (synthetic test fixture per OQ#10-rev-4): add a temporary debug `juce::TextButton` to `OBassoonAudioProcessorEditor` that writes `+0.5` semitones (50 cents) into `pendingTuningSource[60]` (C4). Press button, then play C4 → tuner reads C4 + 50 cents. **Remove the debug button before atomic commit** (Task 11). Alternative: AAX-/AU-host-callable test method invoked via a small Python/AppleScript fixture.
- [ ] **Item 9** — QUAL-02 60 s long-tone (Phase 2.3 protocol carry-forward): Logic-AU 60 s bounce of held C3 with vibrato 5 Hz / 50 c + breath = 0.7 + `attack_character = 0.5`. Python `numpy.isfinite(audio).all()` → True. 1-second windowed RMS drift max-min < 0.5 dB. Logic Process bar CPU drift t=10 s vs t=60 s within ±2 % steady-state. Ear-listen: no amplitude drift, no denormal CPU spike.
- [ ] **Item 10** — 8-voice CPU under enforced cap (PERF-02 final): set `voice_count = 8`, hold 8 simultaneous notes (chord spread C2–Bb3) with vibrato + breath active + `attack_character = 0.5`. Logic Process bar reading at steady-state < 25 %.
- Files: capture verification artefacts at `plugins/O-Bassoon/research/reference-recordings/phase-2.4-{60s-c3,8voice-cpu,attack-character-AB}.{wav,png}` as needed.
- Depends on: Task 8.
- Acceptance: items 1–6, 9, 10 PASS; item 7 PASS or documented Stage 4 deferral; item 8 PASS via synthetic fixture.

#### Task 10 — Update planning artefacts (rev-4 addenda)

- [ ] `plugins/O-Bassoon/.planning/STATUS.md` — update `phase: research_complete` → `plan_complete` (after PLAN-rev-4 lands; this task) → `execute_complete` (Task 8 complete) → `verify_complete` (Task 9 complete). Update `next_action` accordingly. After Gate 4 PASS, update `status: in_progress` → `complete` for Stage 2.
- [ ] `plugins/O-Bassoon/.planning/REQUIREMENTS.md` — transition FUNC-02, FUNC-05, DSP-05, DSP-06, PERF-02, QUAL-02 from pending/partial → complete on Gate 4 PASS. Closes Stage 2 traceability except COMPAT-01/02 (Stage 4) and UI-01/02 (Stage 3).
- [ ] `plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md` — append rev-4 section: 4 systems shipped (BassoonSynthesiser + Exciter morph + NE/TuningEngine compose + NoiseExciter additive); Gate 4 results; closing of Stage 2.
- [ ] `plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md` — append rev-4 section with the 16 static-check grep results + 10 manual Gate 4 results (numerical evidence: tuner readouts, CPU drift, RMS drift, isfinite scan, AB recordings paths).
- Files: `STATUS.md`, `REQUIREMENTS.md`, `SUMMARY.md`, `VERIFICATION.md`.
- Depends on: Task 9.
- Acceptance: all 4 planning artefacts updated; STATUS reflects Stage 2 complete.

#### Task 11 — Remove debug fixtures + atomic commit on Gate 4 PASS

- [ ] Confirm any Item 8 synthetic test fixture (debug `juce::TextButton` in PluginEditor) is REMOVED. Re-build clean. Re-run AU validation + pluginval-5 to confirm production-clean.
- [ ] Stage and commit (single atomic commit on Gate 4 PASS):
  ```
  feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS

  Closes Stage 2 with FUNC-02 (polyphony 1-16, default 8), FUNC-05 (voice
  stealing, release-tail-first), DSP-05 (attack-character morph), DSP-06
  (microtonal pitch via TuningEngine + NE per-voice + MPE per-channel),
  PERF-02 (8-voice CPU < 25%), QUAL-02 (60s long-tone stability).

  Voice manager: BassoonSynthesiser (NEW) subclass overrides findFreeVoice
  to gate by activeVoiceCap (snapshot at processBlock prologue from
  voice_count APVTS, integer-comparison throttle). At cap, delegates to
  JUCE-default findVoiceToSteal (release-tail-first, then oldest-noteOn).

  Attack-character morph: Exciter (MOD) gains tonguedShape (7.5 ms exp-decay
  × white noise, deterministic seed 12345, peak-normalised). startOnset
  snapshots effectiveAttackChar = clamp(attackChar + (vel-0.5)×0.3, 0, 1)
  for the onset window lifetime (zipper avoidance). getNextSample mixes
  softShape ↔ tonguedShape via jmap. Phase 2.1 onsetBuffer renamed softShape.

  Microtonal pitch: BassoonVoice::startNote compose chain replaces plain
  MidiMessage::getMidiNoteInHertz with TuningEngine.getFrequency →
  applyPendingTuning → static_cast<float>(currentFrequencyBase). Bit-identical
  to Phase 2.3 baseline at default 12-TET A=440. NE per-noteId snapshot at
  startNote (locked for voice lifetime). MPE per-channel pitch-bend routes
  automatically via juce::Synthesiser::handlePitchWheel (no new code).

  Render loop: additive Exciter contribution during onset window;
  noiseExciter + exciter sum into excitation; after onset window auto-zeros,
  only NoiseExciter sustains.

  Files:
  - Source/BassoonSynthesiser.{h,cpp} (NEW): subclass + findFreeVoice override
  - Source/Exciter.{h,cpp} (MOD): tonguedShape + startOnset + onsetBuffer rename
  - Source/BassoonVoice.cpp (MOD): startNote compose chain (4 lines) +
    renderNextBlock additive Exciter (1 line)
  - Source/PluginProcessor.{h,cpp} (MOD): BassoonSynthesiser type swap +
    voice_count snapshot at processBlock prologue head
  - CMakeLists.txt (MOD): target_sources +2 entries
  - research/ARCHITECTURE.md: rev-4 as-shipped note appended
  - .planning/{STATUS,REQUIREMENTS}.md: Stage 2 complete
  - .planning/stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}.md:
    rev-4 addenda

  Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
  ```
- [ ] Verify post-commit: `git log --oneline plugins/O-Bassoon | head -1` matches the locked subject.
- Files: all touched in Tasks 1–10.
- Depends on: Task 10.
- Acceptance: single atomic commit on `main` with locked subject; debug fixtures absent; AU + pluginval-5 SUCCESS.

### Files Created / Modified Summary (rev-4)

| Operation | File |
|---|---|
| NEW | `plugins/O-Bassoon/Source/BassoonSynthesiser.h` |
| NEW | `plugins/O-Bassoon/Source/BassoonSynthesiser.cpp` |
| MODIFY | `plugins/O-Bassoon/Source/Exciter.h` (tonguedShape array + startOnset + VELOCITY_BIAS_MAGNITUDE + onsetBuffer→softShape rename) |
| MODIFY | `plugins/O-Bassoon/Source/Exciter.cpp` (prepare body — tonguedShape generation + peak-normalise + onsetSamples = max) |
| MODIFY | `plugins/O-Bassoon/Source/BassoonVoice.cpp` (startNote: 4 lines TuningEngine + applyPendingTuning + currentFrequencyBase + Exciter.startOnset; renderNextBlock: 1 line additive Exciter) |
| MODIFY | `plugins/O-Bassoon/Source/PluginProcessor.h` (BassoonSynthesiser type swap + #include + lastDispatchedVoiceCount member) |
| MODIFY | `plugins/O-Bassoon/Source/PluginProcessor.cpp` (processBlock prologue head: voice_count snapshot before tone-dispatch) |
| MODIFY | `plugins/O-Bassoon/CMakeLists.txt` (target_sources +2 entries: BassoonSynthesiser.h + .cpp) |
| MODIFY | `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` (Phase 2.4 as-shipped rev-4 note appended) |
| MODIFY | `plugins/O-Bassoon/.planning/STATUS.md` (research_complete → plan_complete → execute → verify_complete; Stage 2 complete on Gate 4 PASS) |
| MODIFY | `plugins/O-Bassoon/.planning/REQUIREMENTS.md` (FUNC-02 / FUNC-05 / DSP-05 / DSP-06 / PERF-02 / QUAL-02 → complete on Gate 4 PASS) |
| MODIFY | `plugins/O-Bassoon/.planning/stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}.md` (rev-4 addenda) |
| OPTIONAL | `plugins/O-Bassoon/research/reference-recordings/phase-2.4-{60s-c3,8voice-cpu,attack-character-AB}.{wav,png}` (verification artefacts) |
| UNCHANGED | `plugins/O-Bassoon/Source/{ModeBank,Vibrato,NoiseExciter,BassoonSound,PluginEditor,BassoonVoice}.h` (BassoonVoice.h has no Phase 2.4 deltas — body-level only) |

### Dependencies (DAG — rev-4)

```
Task 0 (hard gate: Phase 2.3 commit on main)
    │
    ├─→ Task 1 (BassoonSynthesiser NEW) ─────┐
    │                                         │
    ├─→ Task 2 (Exciter MOD) ─────────────────┤
    │                                         │
    │                                         ↓
    │                                   Task 3 (BassoonVoice startNote)
    │                                         │
    │                                         ↓
    │                                   Task 4 (BassoonVoice renderNextBlock)
    │                                         │
    │                                         ↓
    └─→ Task 5 (PluginProcessor type swap + voice_count snapshot)
                                              │
                                              ↓
                                        Task 6 (CMakeLists target_sources +2)
                                              │
                                              ↓
                                        Task 7 (ARCHITECTURE.md rev-4 note)
                                              │
                                              ↓
                                        Task 8 (build + 16-grep battery)
                                              │
                                              ↓
                                        Task 9 (manual Gate 4 — 10 items)
                                              │
                                              ↓
                                        Task 10 (planning artefacts rev-4)
                                              │
                                              ↓
                                        Task 11 (remove fixtures + atomic commit)
```

### Success Criteria (Gate 4)

A Gate 4 PASS requires ALL of:

- [ ] Phase 2.3 atomic commit landed on `main` BEFORE Phase 2.4 execute begins (Task 0)
- [ ] Build clean — VST3 + AU + Standalone, zero warnings (Task 8)
- [ ] AU validation SUCCEEDED (`auval -v aumu OBsn OuDv`) (Task 8 #15)
- [ ] pluginval --strictness 5 exit 0 (Task 8 #16)
- [ ] 16 static-check grep gates PASS (Task 8 #1–#14: RT-safety + NE-drain ordering + type-swap + voice_count snapshot site + applyPendingTuning + tuningEngine.getFrequency + exciter.startOnset + additive composition + DSP-07 + 1/8 scaler + throttle epsilon + setExpression site + applyGainRamp form + modeBank.setFundamental count)
- [ ] 8 simultaneous notes audibly distinct (Task 9 #1)
- [ ] voice_count cap + stealing — release-tail-first preference (Task 9 #2)
- [ ] Rapid retrigger 10 Hz × 30 s, `voice_count = 1` mono — no stuck notes (Task 9 #3)
- [ ] Attack-character soft (low vel) audibly gentle (Task 9 #4)
- [ ] Attack-character tongued (high vel) audibly percussive (Task 9 #5)
- [ ] Mid-morph (`= 0.5`) audibly intermediate; no discontinuity in 0→1 sweep (Task 9 #6)
- [ ] MPE per-channel pitch-bend retunes individual voices (Task 9 #7) OR documented Stage 4 deferral
- [ ] VST3 NE pitch event applies via synthetic fixture; tuner confirms (Task 9 #8)
- [ ] QUAL-02 60 s long-tone (Task 9 #9): isfinite True, RMS drift < 0.5 dB, CPU drift < 2 %
- [ ] PERF-02 8-voice CPU < 25 % under enforced cap (Task 9 #10)
- [ ] STATUS / REQUIREMENTS / SUMMARY / VERIFICATION rev-4 written; ARCHITECTURE.md rev-4 note appended (Tasks 7, 10)
- [ ] Debug fixtures removed; atomic commit landed with locked subject (Task 11)

**Iteration ceiling** (CONTEXT-rev-4 Q3-rev-4 batch 2): rev-3 ceiling per Phase 2.2 + Phase 2.3 precedent. Allows in-cycle: rev-1 (initial plan), rev-2 (research/plan refinement), rev-3 (inline verify-phase fix). Forces fresh discuss-phase cycle if verify-phase finds defects requiring rev-4+.

In-cycle adjustments allowed at rev-3 ceiling:
- Velocity bias magnitude: bracket [0.2f, 0.4f] if 0.3f feels too subtle / too aggressive (OQ#4-rev-4).
- `tonguedShape` decay time: bracket [5 ms, 10 ms] if 7.5 ms feels too short / too long (OQ#3-rev-4).
- `softShape` extend to 30 ms with low-pass filter if 5 ms half-sine doesn't read as "soft enough" against tongued (OQ#3-rev-4 fallback).
- Voice-cap clamp: relax max from 16 to 8 if PERF-02 fails at 16 (PERF-02 currently passes per Phase 2.3 verify; this is a fallback only).

### Risks Carried (from CONTEXT-rev-4)

1. **Voice stealing audible click** — JUCE default `findVoiceToSteal` prefers release-tail-first; oldest-noteOn click is psychoacoustically masked by the simultaneous new note-on. Verify item 2; if audibly bad, surface as v1.1 internal forced fast-fade.
2. **Attack-character mid-onset zipper** — eliminated by `Exciter::startOnset` snapshot latch; mid-onset automation only affects next note-on.
3. **NE applyPendingTuning thread-safety** — Stage 1 enforces NE-drain BEFORE renderNextBlock; verify gate #2 confirms ordering preserved. Atomic table reads lock-free per O-Lyrica spike.
4. **TuningEngine API mismatch** — research-phase OQ#5 verified `double getFrequency(int midiNote, int midiChannel = 0)` global namespace. Phase 2.4 cast `static_cast<float>` at compose-chain end (D4-rev-4 lock).
5. **MPE per-channel pitch-bend not routed** — research-phase OQ#7 confirmed `juce::Synthesiser::handlePitchWheel` routes per-channel automatically. No new code; verify item 7.
6. **Exciter file rot Phase 2.1/2.4 API divergence** — `start()` retained as thin wrapper; `softShape` rename mechanical; D6-rev-4 backwards-compat lock.
7. **`voice_count = 1` mono-mode edge case** — D5-rev-4 verified standard mono-cycle (stopNote → startNote atomic w.r.t. voice state); no special-case needed. Verify item 3.
8. **Polyphony stealing high-rate retrigger race** — JUCE 8 stop-then-start is atomic w.r.t. voice state; verify item 3.
9. **60 s QUAL-02 regression** — Phase 2.4 additions are cold paths during sustain (voice manager, NE consumption, TuningEngine, Exciter morph). QUAL-02 should pass cleanly. Verify item 9.
10. **Phase 2.3 atomic commit dependency** — Task 0 hard gate; if Phase 2.3 commit not landed, STOP and surface to user.

### Out of Scope (deferred per ROADMAP)

- pluginval `--strictness 10` — Stage 4
- Windows VST3 build + WebView2 verification — Stage 4
- Dorico Playback Template integration + microtonal score parity — Stage 4
- Aftertouch → vibrato_depth modulation — v1.1 (Stage 0 D4)
- CC1 → vibrato_depth additive routing — v1.1
- UI mockup integration — Stage 3 (blocked on UI mockup pass)
- Factory presets + CHANGELOG.md — Stage 4
- Per-note loudness normalisation — v1.1 (Phase 2.2 RESEARCH-rev-2 §1.6)
- Mid-note retuning via per-block applyPendingTuning re-read — v1.1 (Phase 2.4 startNote-only snapshot per Q2-rev-4 batch 1)
- Tongued-articulation reference recording sourcing — v1.0 ear-only A/B (Q3-rev-4 batch 1); post-v1.0 if needed
- `OuariconCappedSynthesiser` shared module extraction — v1.1+ refactor (RESEARCH-rev-4 §2 first-in-family note)

### Inputs Consumed Verbatim (rev-4)

- **CONTEXT-rev-4** §"Cycle Scope (rev-4)" + §"Constraints Identified (rev-4)" + §"Approach Decisions (rev-4)" Q1–Q4 batch 1 + Q1–Q4 batch 2 + 14 derived locks — locked scope, dispatch design, Gate 4 bar, atomic commit subject
- **RESEARCH-rev-4** §1 OQ#1-rev-4 — `findFreeVoice` override pattern + JUCE 8.0.4 source-line cites (juce_Synthesiser.h:600–603 / 610–612 / 381 / 577 / 336 / 339 / 160; juce_Synthesiser.cpp:509–523 / 525–594)
- **RESEARCH-rev-4** §1 OQ#2-rev-4 — voice_count snapshot site at processBlock prologue head + integer-comparison throttle + `lastDispatchedVoiceCount = -1` first-block forcing
- **RESEARCH-rev-4** §1 OQ#3-rev-4 — `softShape` retention as Phase 2.1 5 ms half-sine × exp; `tonguedShape` NEW = 7.5 ms exp-decay × white noise (rng seed 12345, peak-normalised); 4 time-constants over decay
- **RESEARCH-rev-4** §1 OQ#4-rev-4 — velocity bias magnitude 0.3f locked
- **RESEARCH-rev-4** §1 OQ#5-rev-4 — TuningEngine.getFrequency `double getFrequency(int, int = 0)` global namespace, bit-identical to MidiMessage at default 12-TET A=440 (octaveStretch=1.0f cast preserves)
- **RESEARCH-rev-4** §1 OQ#6-rev-4 — `applyPendingTuning` inline header function (NoteExpression.h:66–79); compose order TuningEngine → applyPendingTuning → static_cast<float>; O-Lyrica HarpSynthVoice.cpp:113–147 precedent
- **RESEARCH-rev-4** §1 OQ#7-rev-4 — `juce::Synthesiser::handlePitchWheel` (juce_Synthesiser.cpp:403–410) per-channel routing; no MPESynthesiser needed
- **RESEARCH-rev-4** §1 OQ#8-rev-4 — additive composition (`noiseSample + exciterSample`) during onset window
- **RESEARCH-rev-4** §1 OQ#9-rev-4 — manual active-voice loop (no `getNumActiveVoices` in JUCE 8.0.4)
- **RESEARCH-rev-4** §1 OQ#10-rev-4 — synthetic test fixture for NE (debug TextButton); Bitwig + MPE controller for pitch-bend or Stage 4 deferral
- **RESEARCH-rev-4** §3 — Implementation skeletons (BassoonSynthesiser.{h,cpp} NEW; Exciter.{h,cpp} rev-4 MOD; BassoonVoice.cpp rev-4 deltas; PluginProcessor.{h,cpp} rev-4 deltas; CMakeLists rev-4 delta; ARCHITECTURE rev-4 backfill template)
- **RESEARCH-rev-4** §4 — 7 discrepancies registered with resolutions (D1: `.cpp` pair convention; D2: tonguedShape pad-zero clean; D3: onsetBuffer→softShape rename; D4: float cast precision; D5: voice_count=1 mono-mode standard; D6: setNoteStealingEnabled redundant but explicit; D7: effectiveAttackChar default safe)
- **RESEARCH-rev-4** §5 — 16 static-check grep gates locked verbatim (Task 8)

### Audit Trail (rev-4 addendum)

**rev-4 (this addendum, 2026-04-29):** Phase 2.4 plan — Voice Manager + Attack Character + Note Expression Integration. 12 tasks (Task 0 hard gate + Task 1 NEW BassoonSynthesiser .h+.cpp pair + Task 2 Exciter MOD + Tasks 3–4 BassoonVoice startNote+renderNextBlock + Task 5 PluginProcessor type-swap + voice_count snapshot + Task 6 CMakeLists +2 + Task 7 ARCHITECTURE rev-4 note + Task 8 build + 16-grep battery + Task 9 manual Gate 4 10-item bar + Task 10 planning artefact rev-4 + Task 11 remove debug fixtures + atomic commit). Lifts RESEARCH-rev-4 §3 implementation skeletons verbatim. Pins Gate 4 PASS bar from CONTEXT-rev-4 line 776 (10 user-checkable + 16 static-check grep + auval + pluginval-5). Atomic commit subject locked: `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS`. Closes Stage 2 (FUNC-02, FUNC-05, DSP-05, DSP-06, PERF-02, QUAL-02 → complete).

**Inherited from Phase 2.1 PLAN-rev-1 + Phase 2.2 PLAN-rev-2 + Phase 2.3 PLAN-rev-3 + Phase 2.4 CONTEXT-rev-4 + RESEARCH-rev-4 (not re-litigated):**
- Per-sample render-loop ordering (`excitation → modeBank → adsr → addSample`)
- Centred equal L+R per-sample voice write
- NE drain BEFORE renderNextBlock at PluginProcessor.cpp prologue
- DSP-07 (no O-Reed dependency)
- Reference WAVs at `research/reference-recordings/bassoon-c3-sustain-v{1,2}.wav` (Phase 2.1, read-only)
- Atomic-commit gate-first principle
- Logic Pro (AU) primary listening DAW; Logic CPU System Performance Meter / Process bar
- Bassoon-tuned partial table + formant Gaussian × 1/k roll-off (Phase 2.2 — locked)
- Tone smoother (50 ms Linear) + throttled-epsilon (0.001) dispatch (Phase 2.2)
- 1/8 headroom scaler (Phase 2.2)
- rev-3 strike() at startNote (Phase 2.2 patch)
- Phase 2.3 architectural pivot — continuous-noise excitation as primary sustain source (Phase 2.4 layers Exciter onset-window contribution additively)
- Phase 2.3 setExpression aggregate setter + 6-shadow throttled dispatch + CC2-takeover state machine + breath smoother + vibrato compose + output_gain `applyGainRamp` declick
- Phase 2.1 `pitchWheelMoved` override at BassoonVoice.cpp:111–125 (MPE per-channel pitch-bend dispatch)

**New in rev-4:**
- Cycle scope = Phase 2.4 only (4 systems: voice manager + attack-character morph + NE per-voice + TuningEngine compose chain)
- 1 NEW translation unit pair: `Source/BassoonSynthesiser.{h,cpp}` (subclass `juce::Synthesiser`, override `findFreeVoice` only, delegate `findVoiceToSteal` to base)
- 4 MODIFY source files: `Source/{Exciter.{h,cpp}, BassoonVoice.cpp, PluginProcessor.{h,cpp}}`
- 1 MODIFY CMakeLists.txt (`target_sources` +2 entries)
- 1 MODIFY ARCHITECTURE.md (Phase 2.4 as-shipped rev-4 note: BassoonSynthesiser + Exciter morph + f_base compose chain + NoiseExciter additive + MPE routing + regression invariants)
- BassoonSynthesiser type swap at PluginProcessor.h (single-line `juce::Synthesiser` → `BassoonSynthesiser`)
- voice_count snapshot at processBlock prologue head, integer-comparison throttle (`lastDispatchedVoiceCount = -1` first-block forcing)
- Exciter: `softShape` (rename from `onsetBuffer`) + `tonguedShape` NEW (7.5 ms exp-decay × white noise, deterministic seed 12345, peak-normalised) + `startOnset(attackChar, velocity)` snapshot latch + `effectiveAttackChar = clamp(attackChar + (vel-0.5)×0.3, 0, 1)`
- BassoonVoice::startNote compose chain (4 lines): `tuningEngine->getFrequency` → `applyPendingTuning` → `static_cast<float>(currentFrequencyBase)` → `exciter.startOnset(attackChar, velocity)`
- BassoonVoice::renderNextBlock (1 line): additive Exciter contribution `noiseSample + exciterSample`
- 16 static-check grep gates (RESEARCH-rev-4 §5) — adds 7 Phase 2.4-new gates (BassoonSynthesiser type swap, voice_count snapshot site, applyPendingTuning, tuningEngine.getFrequency, exciter.startOnset, additive composition, modeBank.setFundamental count)
- 10 user-checkable Gate 4 items (8 polyphony / 1 mono retrigger / 3 attack-character / 1 MPE / 1 NE / 1 QUAL-02 60 s / 1 PERF-02 8-voice CPU)
- Synthetic test fixture for NE verification (debug TextButton, removed before atomic commit)
- Inline iteration ceiling at rev-3 (Phase 2.2 + Phase 2.3 precedent)
- Atomic commit on Gate 4 PASS with locked subject `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS`
- Closes Stage 2: FUNC-02 + FUNC-05 + DSP-05 + DSP-06 + PERF-02 + QUAL-02 → complete

### Next Phase (rev-4)

Ready for: **execute** phase — `/clear` then `/plugin-execute O-Bassoon 2-dsp`

**Hard gate before execute:** verify Phase 2.3 atomic commit (`feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`) lands on `main` (Task 0). If not yet landed, surface to user.
