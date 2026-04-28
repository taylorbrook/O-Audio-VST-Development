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
