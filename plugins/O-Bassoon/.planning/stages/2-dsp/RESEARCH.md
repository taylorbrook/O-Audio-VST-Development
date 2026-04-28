# Stage 2: DSP — Research (rev-1)

**Date:** 2026-04-27
**Plugin:** O-Bassoon
**Stage:** 2 of 4 (DSP)
**Phase:** research
**Cycle Scope:** **Phase 2.1 — Core Modal Voice + First Audio**
**Inputs consumed:** `stages/2-dsp/CONTEXT.md` (rev-1), `.planning/research/ARCHITECTURE.md`, `.planning/ROADMAP.md`, `Source/{BassoonVoice,BassoonSound,PluginProcessor}.{h,cpp}`, JUCE 8.0.4 source at `/Users/taylorbrook/JUCE/`, sibling-plugin precedents (O-Wind / O-Lyrica / O-Formant).

---

## §1 — Open Questions Resolved

### OQ#1 — JUCE 8.0.4 `juce::ADSR` API exact signatures ✅ RESOLVED

**Source:** `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_ADSR.h`

- **`Parameters` constructor** (juce_ADSR.h:71–80): brace-init form `juce::ADSR::Parameters{attack, decay, sustain, release}` is supported via the four-arg constructor `Parameters (float attackTimeSeconds, float decayTimeSeconds, float sustainLevel, float releaseTimeSeconds)`. Aggregate-style `Parameters{0.010f, 0.0f, 1.0f, 0.200f}` compiles and is the family idiom.
- **`setSampleRate` ordering** (juce_ADSR.h:115–119 + jassert at 95): **`setSampleRate` MUST be called before `setParameters`.** The setter contains `jassert (sampleRate > 0.0)` and sample rate is initialised to 44100.0 by default — calling `setParameters` first uses the wrong rate for `recalculateRates`. Phase 2.1 ordering in `prepareToPlay`: `adsr.setSampleRate(sr); adsr.setParameters(juce::ADSR::Parameters{0.010f, 0.0f, 1.0f, 0.200f});`.
- **`getNextSample` idle behavior** (juce_ADSR.h:170–223): when `state == State::idle` the function returns `0.0f` immediately (line 174–177). After `noteOff` reaches release-end, `goToNextState()` calls `reset()` (line 296) which sets `state = State::idle` AND `envelopeVal = 0.0f`. So the canonical exit check is `if (! adsr.isActive()) { … }` (line 108: `isActive() { return state != State::idle; }`).
- **Determinism guarantee:** all ADSR methods are `noexcept`, no allocation, pure float arithmetic. Safe in `processBlock` and `startNote`.

**Phase 2.1 wiring (locked):**
```cpp
// BassoonVoice::prepareToPlay (called from PluginProcessor::prepareToPlay via setCurrentPlaybackSampleRate hook)
adsr.setSampleRate (sampleRate);
adsr.setParameters (juce::ADSR::Parameters { 0.010f, 0.0f, 1.0f, 0.200f });

// BassoonVoice::startNote
adsr.noteOn();

// BassoonVoice::stopNote
if (allowTailOff) adsr.noteOff();
else { clearCurrentNote(); modeBank.reset(); }

// BassoonVoice::renderNextBlock — per-sample
float env = adsr.getNextSample();
// … voice multiply + write
if (! adsr.isActive()) { clearCurrentNote(); modeBank.reset(); return; }
```

**Note on `setCurrentPlaybackSampleRate`:** `juce::SynthesiserVoice` has a virtual `setCurrentPlaybackSampleRate(double)` (called by `Synthesiser::setCurrentPlaybackSampleRate`). Override it in `BassoonVoice` to forward to `adsr.setSampleRate`, `modeBank.prepare`, `exciter.prepare`. This is the standard JUCE seam — the voice does NOT have its own `prepareToPlay`. (Verified by precedent: O-Lyrica's `HarpSynthVoice` exposes a custom `prepareToPlay` that the processor calls inside its own `prepareToPlay` over each voice; O-Wind's `FluteSynthVoice::prepareToPlay` follows the same pattern. **Phase 2.1 follows the O-Lyrica/O-Wind pattern: add `void prepareToPlay (double sampleRate, int maxBlockSize)` to `BassoonVoice` and have `OBassoonAudioProcessor::prepareToPlay` iterate `synthesiser.getNumVoices()` and call it per voice.**)

---

### OQ#2 — `juce::SynthesiserVoice::renderNextBlock` voice-output convention ✅ RESOLVED

**Source:** `/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp`

- **Voices SUM into the buffer** (juce_Synthesiser.cpp:254–258, `renderVoices`): the function iterates `voices` and calls `voice->renderNextBlock(buffer, startSample, numSamples)` directly. **No `buffer.clear()` is performed by `Synthesiser::renderVoices` or `Synthesiser::processNextBlock`.** Voices must use `addSample` (sum) — the host's `processBlock` is responsible for clearing the buffer before calling `synthesiser.renderNextBlock`.
- **Buffer-clearing responsibility** lies with the AudioProcessor. **O-Bassoon already does this correctly** (`PluginProcessor.cpp:165` — `buffer.clear();` before `synthesiser.renderNextBlock` at line 174). Identical pattern in O-Wind's `PluginProcessor.cpp:538` (`buffer.clear();` before line 550 `synthesiser.renderNextBlock(...)`).
- **Sub-range sub-blocking:** `Synthesiser::processNextBlock` may call `renderVoices` over a sub-range `[startSample, startSample + samplesToNextMidiMessage)` between MIDI events (juce_Synthesiser.cpp:200, 211, 226). Voice's `renderNextBlock` MUST honour `startSample` and `numSamples` parameters, writing only into that sub-range via `addSample(channel, startSample + i, value)`. **Voices must NOT touch samples outside this sub-range** (other voices may have written there in previous sub-blocks within the same processBlock).
- **Canonical write call:** `outputBuffer.addSample (channel, startSample + i, voice_out)` is the family standard. Confirmed in O-Wind `FluteSynthVoice.cpp:530–531` and O-Lyrica `HarpSynthVoice.cpp:666–668`.

**Phase 2.1 voice-write loop (locked):**
```cpp
const int numChannels = outputBuffer.getNumChannels();
for (int i = 0; i < numSamples; ++i)
{
    float ex     = exciter.getNextSample();
    float voice  = modeBank.processSample (ex);
    float env    = adsr.getNextSample();
    float out    = voice * env;

    for (int ch = numChannels; --ch >= 0;)
        outputBuffer.addSample (ch, startSample + i, out);

    if (! adsr.isActive())
    {
        clearCurrentNote();
        modeBank.reset();
        return;  // remaining samples in sub-block stay at their cleared/summed value
    }
}
```

The `for (int ch = numChannels; --ch >= 0;)` loop handles 1-channel (mono test host) and 2-channel (stereo Logic AU) identically — matches O-Wind line 530 / O-Lyrica line 667 precedent.

---

### OQ#3 — Direct-form biquad numerical stability at long T60 ✅ RESOLVED

**Decision: use `float` state. Match O-Formant precedent. Direct-Form II Transposed (TDF-II), not Direct-Form I.**

**Evidence from O-Formant** (`plugins/O-Formant/Source/dsp/FormantBiquad.h`): O-Formant's biquad is a 32-byte struct using `float z1, z2` state in TDF-II topology, used at vocal-formant Q values that imply pole radii similar to or higher than ours (Q=10–30 at formant centres → pole radii 0.96–0.99). It also includes a NaN/Inf clamp (line 34–39) that resets `z1 = z2 = 0` and returns 0 if state goes non-finite — a belt-and-braces guard against pathological coefficient updates.

**Stability analysis at our worst case:**
- Phase 2.1 BASE_T60 = 2.5 s (mode 0). `R = exp(-1/(τ·fs))` with τ = T60/6.91 = 0.362 s → at fs = 48 kHz, `R = exp(-1/17,360) = 0.999942`. At fs = 96 kHz, `R = 0.999971`.
- The G-normalisation `G = (1-R)·amp_k` keeps the per-sample input contribution to ~`5.8e-5 · amp_k` — far below float ULP threshold for typical signals (peak ~1.0).
- TDF-II keeps state magnitudes bounded by the input scale (no internal accumulation beyond the resonant build-up); float mantissa has 24 bits ≈ 144 dB dynamic range, well above the 60 dB span between peak signal and silence at T60-end.
- **Limit-cycle floor in float TDF-II at R = 0.99994** sits ≥120 dB below the input — inaudible. Documented across the JUCE / Will Pirkle / Julius Smith DSP literature.

**Why TDF-II (not direct-form I) for the modal bank:**
- Same 5 multiplies + 4 adds per sample (no CPU difference)
- Two state variables `z1, z2` instead of two delayed inputs and two delayed outputs (4 vars in DF-I) → smaller cache footprint per mode (24 bytes coefficients + 8 bytes state = 32 bytes total → 16 modes × 32 bytes = 512 bytes per voice → fits in L1 trivially)
- TDF-II is famously the most numerically robust direct-form variant for IIR filters at long-decay / high-Q
- Matches the O-Formant precedent — code reuse pattern across the family

**Phase 2.1 risk mitigation (carries OQ#3 closure):**
- Use `float` state (b0, b1, b2, a1, a2, z1, z2 — though for the pole-only resonator b1 = b2 = 0, so we can specialise to a 2-pole-no-zero form for ~25% fewer multiplies; see §3 implementation skeleton)
- `juce::ScopedNoDenormals` already in place at `PluginProcessor::processBlock` entry (line 162)
- `modeBank.reset()` on `clearCurrentNote()` zeroes all `z1, z2` per mode (already in voice exit path)
- NaN/Inf guard: `if (! std::isfinite(z1) || ! std::isfinite(z2)) { z1 = z2 = 0; return 0; }` per-mode (lift O-Formant pattern at FormantBiquad.h:34–39 verbatim — single isfinite check is ~1ns on M1, negligible per-sample cost)

**Open follow-ups (non-blocking for Phase 2.1):** if at Phase 2.3 we add per-sample vibrato with per-sample coefficient updates, TDF-II's "filter morphing artefacts" warrant re-checking — but architecture says vibrato is block-rate coefficient updates, so safe for the foreseeable phases.

---

### OQ#4 — Exciter impulse shape — exactly what curve at Phase 2.1? ✅ RESOLVED

**Decision: Half-sine windowed against an exponential decay envelope. Single static shape, computed once at `prepare()` time. No morph at Phase 2.1.**

**Formula:**
```cpp
constexpr float EXCITER_DURATION_MS = 5.0f;
constexpr float EXCITER_TAU_MS      = 1.5f;

void Exciter::prepare (double sampleRate)
{
    const int N = static_cast<int> (sampleRate * EXCITER_DURATION_MS * 0.001);
    onsetSamples = std::min (N, static_cast<int> (onsetBuffer.size()));

    for (int i = 0; i < onsetSamples; ++i)
    {
        const float t       = static_cast<float> (i) / static_cast<float> (sampleRate);  // seconds
        const float window  = std::sin (juce::MathConstants<float>::pi
                                         * static_cast<float> (i) / static_cast<float> (onsetSamples));
        const float decay   = std::exp (-t / (EXCITER_TAU_MS * 0.001f));
        onsetBuffer[i]      = window * decay;
    }
    // Normalise peak to 1.0 to avoid headroom uncertainty
    const float peak = *std::max_element (onsetBuffer.begin(),
                                           onsetBuffer.begin() + onsetSamples,
                                           [](float a, float b){ return std::abs(a) < std::abs(b); });
    if (peak > 1e-6f)
        for (int i = 0; i < onsetSamples; ++i)
            onsetBuffer[i] /= peak;
}
```

**Buffer storage:** class-level `std::array<float, 1024> onsetBuffer{};` per voice. At fs = 96 kHz, 5 ms = 480 samples — fits comfortably. Per-voice memory cost: 4 KB × 16 voices = 64 KB total. Allocation-free at runtime (in-class storage).

**Sketch of the resulting waveform (5 ms at 48 kHz = 240 samples):**
```
1.0 ┤  ╱⎺⎺⎺⎺⎺╮
    │ ╱       ╲___
0.5 ┤╱            ╲____
    │                   ╲___
0.0 ┼──────────────────────────╲────╲────────╲────────→ time
    0ms      1ms      2ms      3ms     4ms     5ms
```

The half-sine window guarantees a smooth zero-cross at both ends (no DC click); the exponential decay weights the first ~2 ms while the sine envelope provides the soft fade-out. At 5 ms total, well within the ADSR's 10 ms attack ramp — listener hears `(impulse * adsr-ramp)` which is monotonically rising for ~5 ms then continuing to ramp to peak at 10 ms while the impulse decays. **No standalone-impulse audibility risk.**

**Why this and not pure exponential or pure sine?**
- Pure exp(-t/τ) starts at peak amplitude → sharp onset transient → click risk if ADSR attack is too slow
- Pure half-sine has equal energy across the duration → less broadband energy at high frequencies (modes 8–15 may be under-excited at low f0)
- Half-sine × exponential gives broadband excitation with smooth onset/offset zero-cross. Standard modal-synthesis exciter pattern (Smith STK, Roads textbook).

**Phase 2.4 expansion:** the same `onsetBuffer` will be replaced with two arrays (`softShape` + `tonguedShape`) and the morph parameter crossfades them. Storage doubles, formula changes — but the API (`getNextSample`) stays identical. **Forward-compat: keep the buffer addressing and onset-index advance in `Exciter::getNextSample` decoupled from the shape formula.**

---

### OQ#5 — Mode-bank Nyquist muting policy ✅ RESOLVED

**Decision (locked): Mute modes with `f_k > 0.45 × fs` by setting `amp_k = 0` (option b). The biquad coefficients are still computed for those modes (so the mode reactivates if pitch-bend brings it back below threshold), but the output gain `G = (1-R) * amp_k` becomes 0 → the mode contributes nothing.**

**Justification:**
- 0.45 × fs leaves 5% Nyquist headroom — robust against pitch-bend overshoot or NE deltas pushing partials slightly above the limit between mode-bank recomputes
- Option (a) "clamp θ at π·0.99" produces audible aliasing at the limit and is numerically pathological (cos(π·0.99) ≈ -0.9995, pole spread compresses)
- Option (c) "set R = 0" makes the biquad pass DC unchanged — incorrect (the mode still has a `G·x[n]` term contributing broadband energy)
- Option (b) is the cleanest: silence the mode entirely; coefficient compute is still cheap (~10 flops); reactivation is automatic on next note-on or pitch-bend recompute

**Implementation:**
```cpp
void ModeBank::setFundamental (float f0)
{
    const float NYQ_LIMIT = 0.45f * static_cast<float>(currentSampleRate);

    for (int k = 0; k < NUM_MODES; ++k)
    {
        const float f_k = f0 * PARTIAL_RATIOS[k];
        const float amp = computeModeAmplitude (k, f0);

        if (f_k > NYQ_LIMIT)
        {
            // Mute: zero gain, but keep stable pole-pair coefficients
            modes[k].b0 = 0.0f;
            modes[k].a1 = 0.0f;
            modes[k].a2 = 0.0f;
            continue;
        }

        const float theta = juce::MathConstants<float>::twoPi * f_k / static_cast<float>(currentSampleRate);
        const float tau   = BASE_T60[k] / 6.91f;
        const float R     = std::exp (-1.0f / (tau * static_cast<float>(currentSampleRate)));

        modes[k].b0 = (1.0f - R) * amp;
        modes[k].a1 = -2.0f * R * std::cos (theta);
        modes[k].a2 = R * R;
    }
}
```

**Phase 2.1 placeholder values (per CONTEXT):** `PARTIAL_RATIOS = {1, 2, 3, …, 16}` integer harmonics; `computeModeAmplitude(k, f0) = 1.0f` flat. So at MIDI 84 (C6 = 1046.5 Hz), modes with `k * 1046.5 > 21,600` Hz get muted at 48k → modes 21+ would be muted, but we only have 16 modes so the 16th (16,744 Hz) gets muted, the 15th (15,697 Hz) gets muted, the 14th (14,651 Hz) gets muted… Counting: 0.45 × 48000 = 21,600 Hz → only modes where `k+1 ≥ 21` are muted at C6 (none in our 16-mode bank). At fs = 96 kHz the limit is 43,200 Hz → never reached.

**Wait — at MIDI 84 with INTEGER PARTIAL RATIOS:** k=0 → 1046.5, k=15 → 16,744 Hz. NYQ_LIMIT at 48k = 21,600. So **no modes are muted at MIDI 84 with integer partials at fs=48k**. The muting becomes relevant only when (a) Phase 2.2 introduces inharmonic ratios that may push partials higher, or (b) Phase 2.4 introduces NE/MPE pitch deltas that could shift the fundamental upward. Phase 2.1 will not exercise the muting path in normal play but the policy is locked for future phases.

**Self-check:** at MIDI 84 (C6 = 1046.5 Hz) the 16th partial sits at 16.7 kHz — well below the 21.6 kHz Nyquist limit at 48 k. The CONTEXT.md "high-frequency modes near or above Nyquist" risk (#2) was based on the original 0.5·fs = 24 kHz limit; with the locked 0.45·fs = 21.6 kHz limit, the 16th partial of C6 is still safely below.

**At fs = 44.1 kHz** (Logic-default with no host SRC): 0.45 × 44100 = 19,845 Hz. The 16th partial of C6 (16.7 kHz) is still safely below. The 15th partial of C5 (7917 Hz) is well below. **No muting in the normal play range** — but the policy guards against future inharmonicity / pitch-bend edge cases.

---

### OQ#6 — `juce::MidiMessage::getMidiNoteInHertz` JUCE 8 signature ✅ RESOLVED

**Source:** `/Users/taylorbrook/JUCE/modules/juce_audio_basics/midi/juce_MidiMessage.h:956`

```cpp
static double getMidiNoteInHertz (int noteNumber, double frequencyOfA = 440.0) noexcept;
```

- **Static method** — no instance needed
- **`noexcept`** — no allocation, no exception path
- **Returns `double`** — Phase 2.1 voice stores `float currentFrequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));`
- **Default `frequencyOfA = 440.0`** — matches Stage-1 contract of plain 12-TET A=440 (no TuningEngine call until Phase 2.4)
- **Implementation (juce_MidiMessage.cpp, single-line lambda):** `return frequencyOfA * pow(2.0, (noteNumber - 69) / 12.0);` — pure float arithmetic, RT-safe

**Precedent confirmation:** O-Wind `FluteSynthVoice.cpp:81` uses exactly this call as the default frequency source when `tuningEngine == nullptr`; O-Lyrica `HarpSynthVoice.cpp:120` uses it as the unconditional default. Both are RT-safe in `startNote`.

---

### OQ#7 — Reference bassoon C3 sourcing — concrete URLs + licenses ✅ RESOLVED

**Primary recommendation: VSCO 2 Community Edition (VSCO-2-CE) — sgossner/VSCO-2-CE GitHub repo, CC0**

| Field | Value |
|-------|-------|
| Repo | https://github.com/sgossner/VSCO-2-CE |
| License | CC0 1.0 Universal — public domain dedication, no attribution required |
| License file | https://github.com/sgossner/VSCO-2-CE/blob/master/LICENSE |
| C3 file | `Woodwinds/Bassoon/sus/PSBassoon_C3_v1_1.wav` (and `_v2_1.wav` — likely two velocities) |
| Direct download (v1) | https://raw.githubusercontent.com/sgossner/VSCO-2-CE/master/Woodwinds/Bassoon/sus/PSBassoon_C3_v1_1.wav |
| Direct download (v2) | https://raw.githubusercontent.com/sgossner/VSCO-2-CE/master/Woodwinds/Bassoon/sus/PSBassoon_C3_v2_1.wav |
| Format | WAV (uncompressed; sample rate / bit depth verified at download time) |
| Recorded by | Sam Gossner & Simon Dalzell, sample cutting Elan Hickler |

**Acquisition procedure (Phase 2.1 execute step):**
```bash
mkdir -p plugins/O-Bassoon/research/reference-recordings
cd plugins/O-Bassoon/research/reference-recordings

curl -fLO https://raw.githubusercontent.com/sgossner/VSCO-2-CE/master/Woodwinds/Bassoon/sus/PSBassoon_C3_v1_1.wav
curl -fLO https://raw.githubusercontent.com/sgossner/VSCO-2-CE/master/Woodwinds/Bassoon/sus/PSBassoon_C3_v2_1.wav

# Rename for clarity in the planning artefact
mv PSBassoon_C3_v1_1.wav bassoon-c3-sustain-v1.wav
mv PSBassoon_C3_v2_1.wav bassoon-c3-sustain-v2.wav

# Authoritative LICENSE.md sidecar
cat > LICENSE.md <<'EOF'
# Reference Recording License

Source: VSCO 2 Community Edition (VSCO-2-CE)
Repository: https://github.com/sgossner/VSCO-2-CE
License: CC0 1.0 Universal (Public Domain Dedication)
LICENSE file: https://github.com/sgossner/VSCO-2-CE/blob/master/LICENSE

Files:
- bassoon-c3-sustain-v1.wav (originally Woodwinds/Bassoon/sus/PSBassoon_C3_v1_1.wav)
- bassoon-c3-sustain-v2.wav (originally Woodwinds/Bassoon/sus/PSBassoon_C3_v2_1.wav)

Use: Internal A/B comparison reference for Ouaricon O-Bassoon Phase 2.2 timbre tuning.
The CC0 dedication permits redistribution and use without restriction; this LICENSE.md is
provenance documentation only, not a license obligation.

Recorded by Sam Gossner & Simon Dalzell. Sample cutting by Elan Hickler/Soundemote.
EOF
```

**Backup #1: University of Iowa Electronic Music Studios (EMS) Musical Instrument Samples (MIS)**

| Field | Value |
|-------|-------|
| Index | https://theremin.music.uiowa.edu/MIS.html |
| License | "Freely available… and may be downloaded and used for any projects, without restrictions" (since 1997) |
| Format | 16-bit / 44.1 kHz mono AIFF (legacy collection); 24-bit / 96 kHz mono+stereo WAV with three mics (post-2012, packaged in zip files). Single chromatic-scale 16/44.1 files online for in-browser listening. |
| Recording environment | Anechoic chamber (Wendell Johnson Speech and Hearing Center, U Iowa) — no room reverb, ideal for spectral analysis |
| Bassoon page | (URL inferred to match `MISbassoon.html` pattern; index page above) |

**Backup #2: Philharmonia Orchestra Sound Sample Library**

| Field | Value |
|-------|-------|
| URL | https://philharmonia.co.uk/resources/sound-samples/ |
| License | "Free to use as you wish, including commercial". **Restriction: samples must NOT be sold or made available 'as is' (i.e. as samples or as a sampler instrument).** |
| Use compatibility | OK for local-only A/B reference (we are not redistributing the raw WAVs as a sample library). **Caution: do NOT commit to the public repo.** Local use only. |
| Format | Per-instrument samples; user-confirmed at download time |

**Recommendation rationale:** VSCO-2-CE wins on three axes — (a) **CC0 license** is the cleanest possible (no attribution friction, may commit to public repo if desired), (b) **direct GitHub raw download** with no auth wall, (c) **two velocity layers** (v1 + v2) gives an early Phase 2.2 hint about dynamic spectral variation. The U Iowa anechoic recording is the best technical reference (no room acoustic colouration) — keep as the backup if VSCO-2-CE turns out to have unwanted noise floor or vibrato bleed.

**Phase 2.1 acceptance criterion:** sourcing succeeds = `plugins/O-Bassoon/research/reference-recordings/bassoon-c3-sustain-v1.wav` exists, plays back as a clean held C3 (~130.81 Hz), and `LICENSE.md` documents source URL + license. Spectrum verification deferred to Phase 2.2.

---

### OQ#8 — Logic CPU meter accuracy for "1-voice < 5 % @ 48 k / 256" verification ✅ RESOLVED

**Decision (locked): Logic Pro X "CPU/HD" performance meter, **System Performance** view, **Process** display. This is the canonical Ouaricon family verification (matches O-Wind / O-Lyrica practice — neither has a CLI render harness; both verified PERF criteria via Logic in-app meters).**

**Reading procedure:**
1. Logic Pro → menu bar → **View** → **Show Performance Meter** (or `⌥-Y` keyboard shortcut)
2. Click the meter's gear icon → enable **CPU**, **HD**, **Process**
3. Set Logic project to 48 kHz (Project Settings → Audio → Sample Rate → 48000) and 256-sample I/O buffer (Logic Pro → Settings → Audio → I/O Buffer Size → 256). Save the project as a verification template.
4. Insert O-Bassoon AU on a stereo software-instrument track. Confirm `aumu OBsn OuDv` shows under "AU Instruments → Ouaricon".
5. Hold a single MIDI C3 (use Caps Lock keyboard or a sustained MIDI region — avoid percussive retriggers).
6. **Read the rightmost "Process" bar** in the Performance Meter. This shows real-time process-thread utilisation as a percentage of one core. Expected: < 5%.
7. Capture a screenshot for the SUMMARY.md verification artifact: `Cmd-Shift-4` → cross-hair → drag over the meter region.

**Confounders / caveats (document in VERIFICATION.md):**
- Logic's "CPU" bar shows aggregated I/O thread utilisation. The "Process" bar is the more reliable per-plugin proxy. Both should be reported.
- M1 Pro / M1 Max single-core capacity is ~10 GFLOPs sustained; 16 biquads at 48 kHz = ~7 MFLOPs worst case → **mathematical CPU < 0.1%** — actual reading inflated by JUCE message thread, AU host overhead, MIDI buffer scan. Realistic projection: 0.5–2.0%.
- If reading shows > 5%: open Activity Monitor → AudioComponentRegistrar / Logic Pro → check thread breakdown. Usually reveals AU bridge overhead, not voice DSP.
- Re-test on cold boot (no other plugins loaded in the project, no other DAWs / browsers open) — Logic's meter is sensitive to OS scheduling pressure.

**Fallback verification path** (if Logic meter is ambiguous): build the AudioPluginHost (JUCE example) and run with Activity Monitor. AudioPluginHost gives the cleanest per-plugin profile but adds ~30 min setup. Hold in reserve; not required for Phase 2.1.

---

### OQ#9 — `pitchWheelMoved` Phase 2.1 semantics ✅ RESOLVED

**Source:** `/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp:403–410`

`Synthesiser::handlePitchWheel` reads the raw 14-bit wheel value from the MIDI event and forwards it directly to each voice via `voice->pitchWheelMoved (wheelValue)`. The value is the **raw 14-bit MIDI pitch-bend value: range `[0, 16383]`, centre = 8192.** No normalisation by JUCE.

**Precedent:** O-Wind `FluteSynthVoice.cpp:75` and `:215` uses exactly the formula in CONTEXT Q-Pitch-bend:
```cpp
pitchBendSemitones = ((static_cast<float> (newValue) - 8192.0f) / 8192.0f) * pitchBendRange;
```
where `pitchBendRange = 2.0f` (default ±2 semitones). This matches the GM standard and Ableton/Logic default.

**Phase 2.1 implementation (locked):**
```cpp
class BassoonVoice : public juce::SynthesiserVoice
{
    static constexpr float PITCH_BEND_RANGE_SEMITONES = 2.0f;
    int   pitchWheelValue       = 8192;        // centre at construction
    float pitchBendSemitones    = 0.0f;
    float currentFrequencyBase  = 0.0f;        // pre-bend, set in startNote
    // …
};

void BassoonVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int currentPitchWheelPos)
{
    pitchWheelValue       = currentPitchWheelPos;
    pitchBendSemitones    = ((static_cast<float>(pitchWheelValue) - 8192.0f) / 8192.0f) * PITCH_BEND_RANGE_SEMITONES;
    currentFrequencyBase  = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz (midiNote));

    const float fBent = currentFrequencyBase * std::pow (2.0f, pitchBendSemitones / 12.0f);
    modeBank.setFundamental (fBent);
    exciter.start();
    adsr.noteOn();
}

void BassoonVoice::pitchWheelMoved (int newPitchWheelValue)
{
    pitchWheelValue       = newPitchWheelValue;
    pitchBendSemitones    = ((static_cast<float>(pitchWheelValue) - 8192.0f) / 8192.0f) * PITCH_BEND_RANGE_SEMITONES;

    if (currentFrequencyBase > 0.0f)  // guard against pitch-bend before any note-on
    {
        const float fBent = currentFrequencyBase * std::pow (2.0f, pitchBendSemitones / 12.0f);
        modeBank.setFundamental (fBent);
    }
}
```

**Phase 2.1 testable behaviour:** holding C3 and sweeping a pitch-bend wheel ±100% should smoothly retune the held note ±2 semitones (B2 to D3). Audible during Logic AU smoke (Gate 1 item 9). No automated MPE testing in scope (Phase 2.4).

---

### OQ#10 — Spectrum-analyzer plugin recommendation for the baseline-capture step ✅ RESOLVED

**Decision: Voxengo SPAN (free)**

| Field | Value |
|-------|-------|
| Plugin | Voxengo SPAN 3.x (current as of 2026) |
| Cost | Free |
| Compatibility | macOS 10.12+ Apple Silicon native; AU + VST3 + AAX |
| Download URL | https://www.voxengo.com/product/span/ |
| Format | macOS DMG installer; standard `/Library/Audio/Plug-Ins/Components/SPAN.component` and `/Library/Audio/Plug-Ins/VST3/SPAN.vst3` install paths |

**Why SPAN over alternatives:**
- **MeldaProduction MMultiAnalyzer:** free tier exists but the install bundle pulls all 100+ Melda plugins; SPAN is a single-purpose 5 MB install.
- **Bertom EQ Curve Analyzer:** modern alternative (free), but newer and less battle-tested in the Ouaricon workflow.
- **iZotope Insight:** referenced in ARCHITECTURE.md (line 385) as the Phase 2.2 acceptance-test tool — but it is paid ($499), so cannot be the Phase 2.1 baseline tool. SPAN's spectrum view is functionally equivalent for our needs (peak detection, dB scale, frequency cursor).
- **Logic's stock "Channel EQ" analyzer:** built-in, but limited frequency resolution and no screenshot-friendly export.

**Phase 2.1 spectrum-baseline-capture procedure (Gate 1 item 10):**
1. Install SPAN (DMG → drag-install). Validate it loads in Logic AU plugin manager (`/Applications/Logic Pro.app` → Window → Plug-In Manager).
2. In the Phase 2.1 verification Logic project, insert SPAN as the **last AU effect** on the O-Bassoon track (post-O-Bassoon, post any send returns).
3. Hold C3 for ~5 seconds (sustained MIDI region).
4. SPAN window → set:
   - **Mode:** Stereo (or Sum)
   - **Block size:** 8192 samples (high resolution for low-frequency partials)
   - **Window:** Hann
   - **Slope:** 4.5 dB/oct (reveals partial structure clearly; matches mastering-engineer convention)
   - **Average:** Infinite (snapshot the steady-state spectrum)
5. Wait 3 seconds for the spectrum to stabilise.
6. `Cmd-Shift-4` → cross-hair → drag over the SPAN window to screenshot.
7. Save as `plugins/O-Bassoon/research/reference-recordings/phase-2.1-baseline-c3-spectrum.png`.

**This baseline image is the pre-tuning snapshot that Phase 2.2's listening loop A/B compares against** (placeholder integer-harmonic spectrum vs. bassoon-tuned-partial spectrum). Cheap to capture now (~5 min); expensive to backfill once the partial table changes.

---

## §2 — Pattern Confirmations

### Voice mono-to-stereo write loop (canonical Ouaricon pattern)

**O-Wind `FluteSynthVoice.cpp:518–532`:**
```cpp
const float* outData = tempBuffer.getReadPointer (0);
for (int s = 0; s < numSamples; ++s)
{
    float sample = outData[s];
    // … (release-fade application elided)
    for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
        outputBuffer.addSample (ch, startSample + s, sample);
}
```

**O-Lyrica `HarpSynthVoice.cpp:660–670`:**
```cpp
float sample = stringSample + sympatheticContribution;
voiceOutputBuffer.addSample(0, startSample, sample);    // O-Lyrica-specific per-voice scratch buffer

for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
    outputBuffer.addSample(i, startSample, sample);

++startSample;
```

**Phase 2.1 BassoonVoice render loop (canonical pattern, locked):**
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
        return;
    }
}
```

The descending `--ch >= 0` channel loop matches the family idiom (handles arbitrary channel count without a special mono-host case). The early-return-on-ADSR-idle pattern matches O-Wind's `if (! stringModel.isActive()) { clearCurrentNote(); return; }` (HarpSynthVoice.cpp:621–633).

### Voice clearing in `PluginProcessor::processBlock`

**O-Bassoon already correct** (`PluginProcessor.cpp:162–174`):
```cpp
juce::ScopedNoDenormals noDenormals;
buffer.clear();                                          // line 165 — host clears, voices SUM
// … parameter snapshots …
vst3Extensions.drainAndUpdate (synthesiser, …);           // line 170 — NE drain BEFORE renderNextBlock
synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());  // line 174
```

**No change needed at Phase 2.1.** The voice's `addSample` writes will sum into a clean buffer.

### `Synthesiser::renderVoices` does NOT zero the buffer

**Source:** `/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp:254–258`. Confirmed: the host's `processBlock` is the only buffer-clear point. CONTEXT Q1 / Risk #7 closed: O-Bassoon's `buffer.clear()` at PluginProcessor.cpp:165 satisfies the invariant. The "switch from `addSample` to `setSample` for the first voice" fallback documented in CONTEXT Risk #7 is **not necessary** — the host-clear pattern is the one the family uses.

### O-Formant biquad implementation reuse

**Decision: re-implement (not lift verbatim).**

**Reason:** O-Formant's `FormantBiquad` is a generic 5-coefficient biquad (b0, b1, b2, a1, a2) tailored to formant filtering (BPF-style). The bassoon mode-bank biquad is **pole-only** (b1 = b2 = 0) — a 2-pole-no-zero resonator. Lifting the FormantBiquad struct as-is would:
- Carry unused `b1, b2` coefficients (16 bytes wasted per mode → 256 bytes wasted per voice)
- Miss the per-sample multiply-add savings (b1 * input + z2 → drops to 0 when b1 = 0)
- Add unwanted attribution-tracking overhead in the source tree

**Re-implement as `Source/ModeBank.{h,cpp}`** with a **specialised** `Biquad` struct:

```cpp
// Per-mode pole-only resonator: y[n] = b0 * x[n] - a1 * y[n-1] - a2 * y[n-2]
struct ModeBiquad
{
    float b0 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float y1 = 0.0f;  // y[n-1]
    float y2 = 0.0f;  // y[n-2]

    inline float processSample (float input) noexcept
    {
        const float y0 = b0 * input - a1 * y1 - a2 * y2;
        y2 = y1;
        y1 = y0;

        // O-Formant-style NaN/Inf guard (FormantBiquad.h:34–39)
        if (! std::isfinite (y1) || ! std::isfinite (y2))
        {
            y1 = 0.0f;
            y2 = 0.0f;
            return 0.0f;
        }
        return y0;
    }

    void reset() noexcept
    {
        y1 = 0.0f;
        y2 = 0.0f;
    }
};
```

**Footprint:** 20 bytes per mode × 16 modes = 320 bytes per voice × 16 voices = 5120 bytes total. Fits in L1 trivially.

**Operations per sample:** 2 multiplies + 2 multiply-adds + 1 isfinite-check + 2 stores = ~6 cycles on M1 → 16 modes × 6 cycles × 48000 samples/s = ~4.6 Mcycles/s → 0.05% of one M1 core per voice. Headroom is vast.

(Note: Direct-Form I — `y0 = b0*x - a1*y1 - a2*y2` — instead of TDF-II for the pole-only case, because there are no zeros to consider and DF-I's state vars are the previous outputs `y[n-1], y[n-2]` which match the resonator's natural state representation. **Numerical stability is identical to TDF-II for the 2-pole-no-zero case** — both have the same pole locations and the same input-scaled state.)

---

## §3 — Implementation Skeletons

### `Source/ModeBank.h`
```cpp
#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

class ModeBank
{
public:
    static constexpr int NUM_MODES = 16;

    // Phase 2.1: integer harmonics, flat amplitudes (placeholders).
    // Phase 2.2 replaces with bassoon-tuned ratios + formant-Gaussian × 1/k roll-off.
    static constexpr std::array<float, NUM_MODES> PARTIAL_RATIOS = {
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f
    };

    static constexpr std::array<float, NUM_MODES> BASE_T60 = {
        2.5f, 2.2f, 2.0f, 1.8f, 1.6f, 1.4f, 1.2f, 1.0f,
        0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.35f, 0.30f, 0.25f
    };

    void prepare (double sampleRate);
    void setFundamental (float f0);
    float processSample (float excitation) noexcept;
    void reset() noexcept;

    // Phase 2.1 stub — wired live in Phase 2.2
    void setTone (float /*tone01*/) noexcept {}

private:
    struct ModeBiquad
    {
        float b0 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;

        inline float processSample (float x) noexcept
        {
            const float y0 = b0 * x - a1 * y1 - a2 * y2;
            y2 = y1; y1 = y0;
            if (! std::isfinite (y1) || ! std::isfinite (y2))
            {
                y1 = y2 = 0.0f;
                return 0.0f;
            }
            return y0;
        }

        void reset() noexcept { y1 = y2 = 0.0f; }
    };

    static constexpr float NYQ_RATIO = 0.45f;

    std::array<ModeBiquad, NUM_MODES> modes {};
    double currentSampleRate = 48000.0;
};
```

### `Source/ModeBank.cpp`
```cpp
#include "ModeBank.h"

void ModeBank::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    reset();
}

void ModeBank::setFundamental (float f0)
{
    const float fs       = static_cast<float>(currentSampleRate);
    const float nyqLimit = NYQ_RATIO * fs;

    for (int k = 0; k < NUM_MODES; ++k)
    {
        const float f_k = f0 * PARTIAL_RATIOS[k];

        if (f_k > nyqLimit || f_k <= 0.0f)
        {
            modes[k].b0 = 0.0f;
            modes[k].a1 = 0.0f;
            modes[k].a2 = 0.0f;
            continue;
        }

        // Phase 2.1 placeholder amplitude: flat
        const float amp = 1.0f;

        const float theta = juce::MathConstants<float>::twoPi * f_k / fs;
        const float tau   = BASE_T60[k] / 6.91f;
        const float R     = std::exp (-1.0f / (tau * fs));

        modes[k].b0 = (1.0f - R) * amp;
        modes[k].a1 = -2.0f * R * std::cos (theta);
        modes[k].a2 = R * R;
    }
}

float ModeBank::processSample (float excitation) noexcept
{
    float sum = 0.0f;
    for (auto& m : modes)
        sum += m.processSample (excitation);

    // Per-voice headroom: 16 modes summed at unity gain can peak near +24 dB.
    // Phase 2.1 attenuates to keep within [-1, 1] without parameter dependency.
    return sum * (1.0f / static_cast<float>(NUM_MODES));
}

void ModeBank::reset() noexcept
{
    for (auto& m : modes) m.reset();
}
```

### `Source/Exciter.h`
```cpp
#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <algorithm>

class Exciter
{
public:
    static constexpr int   MAX_ONSET_SAMPLES = 1024;   // 5 ms @ 96 kHz fits in 480; 1024 leaves headroom
    static constexpr float DURATION_MS       = 5.0f;
    static constexpr float TAU_MS            = 1.5f;

    void prepare (double sampleRate);
    void start() noexcept            { onsetIdx = 0; active = true; }

    inline float getNextSample() noexcept
    {
        if (! active || onsetIdx >= onsetSamples)
        {
            active = false;
            return 0.0f;
        }
        return onsetBuffer[static_cast<size_t>(onsetIdx++)];
    }

    void reset() noexcept            { onsetIdx = 0; active = false; }

private:
    std::array<float, MAX_ONSET_SAMPLES> onsetBuffer {};
    int  onsetSamples = 0;
    int  onsetIdx     = 0;
    bool active       = false;
};
```

### `Source/Exciter.cpp`
```cpp
#include "Exciter.h"

void Exciter::prepare (double sampleRate)
{
    const int N = std::min (static_cast<int>(MAX_ONSET_SAMPLES),
                            static_cast<int>(sampleRate * DURATION_MS * 0.001));
    onsetSamples = N;

    for (int i = 0; i < N; ++i)
    {
        const float t      = static_cast<float>(i) / static_cast<float>(sampleRate);
        const float window = std::sin (juce::MathConstants<float>::pi
                                       * static_cast<float>(i) / static_cast<float>(N));
        const float decay  = std::exp (-t / (TAU_MS * 0.001f));
        onsetBuffer[static_cast<size_t>(i)] = window * decay;
    }

    // Normalise peak to 1.0
    float peak = 0.0f;
    for (int i = 0; i < N; ++i)
        peak = std::max (peak, std::abs (onsetBuffer[static_cast<size_t>(i)]));
    if (peak > 1e-6f)
        for (int i = 0; i < N; ++i)
            onsetBuffer[static_cast<size_t>(i)] /= peak;

    reset();
}
```

### `Source/BassoonVoice.h` (Phase 2.1 deltas to Stage 1 stub)
```cpp
#pragma once
#include <JuceHeader.h>
#include "TuningEngine.h"
#include "NoteExpression.h"
#include "BassoonSound.h"
#include "ModeBank.h"
#include "Exciter.h"

class BassoonVoice : public juce::SynthesiserVoice
{
public:
    BassoonVoice() = default;
    ~BassoonVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void prepareToPlay (double sampleRate, int maxBlockSize);  // NEW Phase 2.1

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound,
                    int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    void setAPVTS               (juce::AudioProcessorValueTreeState* p) { parameters = p; }
    void setTuningEngine        (TuningEngine* engine)                  { tuningEngine = engine; }
    void setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* src) { pendingTuningSource = src; }

private:
    static constexpr float PITCH_BEND_RANGE_SEMITONES = 2.0f;

    juce::AudioProcessorValueTreeState*           parameters          = nullptr;
    TuningEngine*                                 tuningEngine        = nullptr;
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

    ModeBank   modeBank;
    Exciter    exciter;
    juce::ADSR adsr;

    int   pitchWheelValue       = 8192;
    float pitchBendSemitones    = 0.0f;
    float currentFrequencyBase  = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassoonVoice)
};
```

### `Source/BassoonVoice.cpp` (Phase 2.1 implementation skeleton)
```cpp
#include "BassoonVoice.h"

bool BassoonVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<BassoonSound*> (sound) != nullptr;
}

void BassoonVoice::prepareToPlay (double sampleRate, int /*maxBlockSize*/)
{
    setCurrentPlaybackSampleRate (sampleRate);   // JUCE bookkeeping
    modeBank.prepare (sampleRate);
    exciter.prepare  (sampleRate);
    adsr.setSampleRate (sampleRate);
    adsr.setParameters (juce::ADSR::Parameters { 0.010f, 0.0f, 1.0f, 0.200f });
}

void BassoonVoice::startNote (int midiNote, float /*velocity*/,
                              juce::SynthesiserSound*, int currentPitchWheelPos)
{
    pitchWheelValue       = currentPitchWheelPos;
    pitchBendSemitones    = ((static_cast<float>(pitchWheelValue) - 8192.0f) / 8192.0f)
                            * PITCH_BEND_RANGE_SEMITONES;
    currentFrequencyBase  = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz (midiNote));

    const float fBent = currentFrequencyBase * std::pow (2.0f, pitchBendSemitones / 12.0f);
    modeBank.setFundamental (fBent);
    exciter.start();
    adsr.noteOn();
}

void BassoonVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        adsr.reset();
        modeBank.reset();
        exciter.reset();
        currentFrequencyBase = 0.0f;
    }
}

void BassoonVoice::pitchWheelMoved (int newPitchWheelValue)
{
    pitchWheelValue    = newPitchWheelValue;
    pitchBendSemitones = ((static_cast<float>(pitchWheelValue) - 8192.0f) / 8192.0f)
                         * PITCH_BEND_RANGE_SEMITONES;

    if (currentFrequencyBase > 0.0f)
    {
        const float fBent = currentFrequencyBase * std::pow (2.0f, pitchBendSemitones / 12.0f);
        modeBank.setFundamental (fBent);
    }
}

void BassoonVoice::controllerMoved (int /*ccNum*/, int /*ccVal*/) {}   // Phase 2.3

void BassoonVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                    int startSample, int numSamples)
{
    if (! adsr.isActive())
        return;   // nothing to do; voice is silent

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
}
```

### `PluginProcessor::prepareToPlay` voice-prepare extension
Add (or insert) inside the existing `prepareToPlay`:
```cpp
for (int v = 0; v < synthesiser.getNumVoices(); ++v)
    if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (v)))
        bv->prepareToPlay (sampleRate, samplesPerBlock);
```

### `CMakeLists.txt` source-list addition
```cmake
target_sources(O-Bassoon PRIVATE
    Source/BassoonSound.h
    Source/BassoonVoice.h
    Source/BassoonVoice.cpp
    Source/ModeBank.h            # NEW
    Source/ModeBank.cpp          # NEW
    Source/Exciter.h             # NEW
    Source/Exciter.cpp           # NEW
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)
```
(Mirror the **existing** `target_sources(...)` block in O-Bassoon's CMakeLists; do not duplicate the call.)

---

## §4 — Discrepancies / Surprises

### D1 — `BassoonVoice::prepareToPlay` is NOT inherited from `juce::SynthesiserVoice`

**Surprise:** `juce::SynthesiserVoice` does NOT have a virtual `prepareToPlay (double sampleRate, int maxBlockSize)`. It has `setCurrentPlaybackSampleRate (double newRate)` (called by `Synthesiser::setCurrentPlaybackSampleRate` at juce_Synthesiser.cpp:175). To get a per-voice prepare hook, the plugin's `PluginProcessor::prepareToPlay` must explicitly iterate voices and call a custom `prepareToPlay` method on each.

**Resolution (already absorbed into §3):** Add `void prepareToPlay (double, int)` as a **non-virtual** custom method on `BassoonVoice` (matches O-Wind `FluteSynthVoice::prepareToPlay` and O-Lyrica `HarpSynthVoice::prepareToPlay` precedents). Have `OBassoonAudioProcessor::prepareToPlay` iterate `synthesiser.getNumVoices()` and dispatch via `dynamic_cast<BassoonVoice*>`.

**Impact on PLAN.md:** the plan needs an extra line item: "extend `OBassoonAudioProcessor::prepareToPlay` to iterate voices and call `BassoonVoice::prepareToPlay`". This is a 4-line change inside the existing prepareToPlay; does not break Stage 1 contract.

**Does NOT contradict CONTEXT.md** (Q3 Approach Decisions assume a per-voice prepare hook exists, just doesn't pin down which one). CONTEXT compatible.

### D2 — Per-voice scaling factor needed to keep summed-mode output bounded

**Surprise:** with 16 modes summed at unity gain (impulse-excited, Q ~ 5000), the per-voice output can momentarily peak > 1.0 even before the 16-voice polyphony sum. Without scaling, a single C3 voice may clip.

**Resolution (added to §3 ModeBank::processSample):** divide the summed mode output by `NUM_MODES`. This gives a peak per-voice ≤ ~1.0 before envelope. With ADSR envelope ≤ 1.0 and 8-voice typical polyphony summed, the bus level stays well within [-1, 1] without explicit limiter. **Phase 2.3** will replace this naïve `1/N` scaling with proper output-gain APVTS read + per-mode amplitude shaping (formant-Gaussian × 1/k); for now `1/N` is the right placeholder.

**Does NOT contradict CONTEXT.md or ARCHITECTURE.md** — neither pins down the per-voice gain trim. ARCHITECTURE.md §"Mode Bank Coefficient Update Strategy" leaves output scaling implementation-defined.

### D3 — JUCE `Synthesiser::renderVoices` does NOT zero the buffer

**Surprise risk closed:** CONTEXT.md Risk #7 hedged on whether `Synthesiser` zeros the buffer before iterating voices. **Confirmed: it does NOT.** The host's `processBlock` is the only buffer-clear point. O-Bassoon's `PluginProcessor.cpp:165` correctly calls `buffer.clear()` before `synthesiser.renderNextBlock` — so the addSample-summing voice convention works correctly.

**Impact on PLAN.md:** none — already correct in Stage 1. Document the invariant in PLAN.md success criteria so it doesn't get accidentally removed in a future refactor.

### D4 — VSCO-2-CE `PSBassoon_C3` octave convention assumption

**Surprise:** VSCO-2-CE filename "C3" likely refers to the sample library's internal octave convention. Some libraries use C3 = MIDI 48 (Yamaha / scientific pitch C3 = 130.81 Hz); others use C3 = MIDI 60 (DAW middle-C convention). Without auditioning, we cannot be 100% sure which C3 the file is.

**Resolution:** Phase 2.1 execute step downloads BOTH C3 sample variants (`v1_1`, `v2_1`) AND the C2 variant for cross-reference. The reference recording's actual fundamental can be measured with a tuner during Phase 2.1 verify (Gate 1 item 1 already requires a tuner setup); if PSBassoon_C3 turns out to be MIDI 48 = ~131 Hz, that matches scientific-bassoon-C3 and is the right reference for Phase 2.2 verification. If it turns out to be MIDI 60 = ~262 Hz, it's the bassoon's mid-C — also useful, just not in the lowest formant region. Either way, archive all three for Phase 2.2 listening loop.

**Impact on PLAN.md:** include the curl-download step + a measurement-and-rename step ("audition reference recording in Logic; rename file to bassoon-{measured-pitch}-sustain.wav for clarity"). 5-min addition.

### D5 — CONTEXT Risk #2 ("High-frequency modes near or above Nyquist") slightly overstated

**Surprise:** the risk text said "16th partial sits at 16.7 kHz — above Nyquist at 48 kHz". Nyquist at 48 kHz is **24 kHz**, not 16.7 kHz; 16.7 kHz is below Nyquist. The actual concern is the 0.45·fs muting threshold (21.6 kHz at 48 k), which the 16th partial of C6 (16.7 kHz) is also safely below.

**Resolution:** the muting policy locked in OQ#5 (mute when `f_k > 0.45·fs`) is still the correct mitigation — the policy guards against future inharmonicity (Phase 2.2's near-integer ratios push partial 16 slightly higher: `16.186 × 1046.5 = 16,940 Hz`, still below 21,600 Hz at 48 k) and against pitch-bend edge cases. **At Phase 2.1 with placeholder integer ratios, no muting will fire in normal play.** Acceptance: locked policy is still correct, risk severity downgraded from "real" to "future-proofing".

**Impact on PLAN.md:** none. Document the closure in VERIFICATION.md.

### D6 — Headroom: `1/N` mode summing scales TOO conservatively at low note density

**Surprise:** dividing by `NUM_MODES = 16` is overly conservative when only 4–6 modes have meaningful amplitude (e.g., MIDI 84 with formant-weighted amplitudes — Phase 2.2). At Phase 2.1 with flat amplitudes all 16 modes contribute roughly equally — `1/N` scaling produces audible-but-quiet signal. Phase 2.3 (output_gain APVTS read) will compensate; Phase 2.1 verification may need to crank the Logic track gain to hear the placeholder voice clearly. Document this in the verification procedure: "expected Phase 2.1 output is ~-24 dB peak — quiet but clearly audible; do not interpret 'quiet' as 'no audio'."

**Impact on PLAN.md:** add to verification notes; not a code change.

---

## §5 — Pre-flight Reference Recording Audition Checklist (Phase 2.1 execute)

Before committing the reference recording to `research/reference-recordings/`:

1. ☐ `curl -fLO` both PSBassoon_C3 files succeeded (HTTP 200, file size > 100 KB)
2. ☐ `afinfo bassoon-c3-sustain-v1.wav` reports valid WAV format, sample rate, bit depth
3. ☐ Audition in Logic / QuickLook: file plays without artefacts, contains a sustained held note (not staccato or loop-truncated)
4. ☐ Tuner check (MTuner or Logic stock tuner): measure fundamental Hz; rename to match measured pitch (`bassoon-c3-sustain-v1.wav` if 130.81 Hz; `bassoon-c4-sustain-v1.wav` if 261.6 Hz; etc.)
5. ☐ Open SPAN on the playback channel: confirm a clean harmonic spectrum (well-defined partials, peak around the first formant region 450–500 Hz for low/mid bassoon notes — the very region Phase 2.2 will target)
6. ☐ Write `LICENSE.md` per the template in §1 OQ#7 with finalised filenames
7. ☐ Write a short `README.md` in `reference-recordings/` documenting: source, license, intended use (Phase 2.2 A/B reference), audition notes (vibrato level, presence of attack transient, room tone)

If any check fails, fall back to the U Iowa MIS bassoon archive (backup #1).

---

## §6 — Outputs and Handoff Checklist

✅ **All 10 CONTEXT Open Questions resolved** (§1)
✅ **2 family precedents confirmed** (§2 — voice mono-to-stereo write loop, host buffer-clear pattern)
✅ **O-Formant biquad pattern decision: re-implement (specialise to pole-only)** (§2)
✅ **Implementation skeletons for ModeBank, Exciter, BassoonVoice, PluginProcessor::prepareToPlay extension, CMakeLists target_sources** (§3)
✅ **6 discrepancies documented with resolutions** (§4)
✅ **Pre-flight audition checklist for reference recording** (§5)

**Handoff to plan-phase:** PLAN.md should consume §3 implementation skeletons verbatim as task body content, sequence the 5 file edits + 1 CMakeLists edit + 1 reference-download step + planning artefact updates as a single atomic Wave (no parallel sub-waves — all changes coupled, gate-first principle), and pin the 10-item Gate 1 PASS bar from CONTEXT Q7 as the wave's verification condition.

**No CONTEXT amendments required.** All discrepancies in §4 are non-blocking (D1: prepare-hook clarification; D2/D6: per-voice gain placeholder; D3: confirms existing pattern; D4: build-time decision; D5: risk severity downgrade).

---

## Audit Trail

**rev-1 (this document, 2026-04-27):** Phase 2.1 research phase. All 10 CONTEXT open questions resolved with JUCE 8.0.4 source-line citations and family-precedent confirmation. 6 discrepancies surfaced (D1–D6); none block planning. Reference recording sourcing locked: VSCO-2-CE (CC0) primary, U Iowa MIS (no restrictions) backup, Philharmonia (commercial-OK) tertiary. Spectrum-baseline tool locked: Voxengo SPAN. Logic CPU verification protocol locked: System Performance Meter, Process bar. Re-implementation of pole-only biquad decided over verbatim O-Formant lift (specialisation saves 16 bytes + 25% multiplies per mode). Implementation skeletons prepared for plan-phase consumption.

---

# Stage 2: DSP — Research (rev-2 addendum)

**Date:** 2026-04-27
**Plugin:** O-Bassoon
**Stage:** 2 of 4 (DSP)
**Phase:** research
**Cycle Scope:** **Phase 2.2 — Bassoon Spectral Tuning + Tone Control**
**Inputs consumed:** `stages/2-dsp/CONTEXT.md` (rev-2 addendum), `.planning/research/ARCHITECTURE.md` §"Bassoon Partial Table" + §"Tone / Brightness Control" + §"Modal Resonator Biquad", `.planning/ROADMAP.md` lines 124–152, current Phase 2.1 source (`Source/{ModeBank,Exciter,BassoonVoice,PluginProcessor}.{h,cpp}` at commit `d1b3370`), JUCE 8.0.4 source at `/Users/taylorbrook/JUCE/`, sibling-plugin precedents (O-Wind, O-Lyrica, O-Bass, O-Contrabass).

---

## §1 — Open Questions Resolved (rev-2)

### OQ#1-rev-2 — `juce::SmoothedValue<float, ValueSmoothingTypes::Linear>` block-rate advance idiom ✅ RESOLVED

**Source:** `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_SmoothedValue.h`

- **`reset(double sampleRate, double rampLengthInSeconds)`** (juce_SmoothedValue.h:265–269): converts seconds → samples via `floor(rampLengthInSeconds * sampleRate)`, then sets `stepsToTarget` and calls `setCurrentAndTargetValue(target)` to clear in-flight smoothing. `noexcept`. Call once in `prepareToPlay`.
- **`setTargetValue(FloatType newValue)`** (juce_SmoothedValue.h:284–303): if `newValue == target` (within `approximatelyEqual`), early-return — no allocation, no work. Otherwise sets `target`, `countdown = stepsToTarget`, recomputes `step`. `noexcept`. **Implication:** safe to call every block unconditionally; the function self-no-ops when the parameter hasn't moved.
- **`getNextValue()`** (juce_SmoothedValue.h:309–322): if `! isSmoothing()` returns `target` immediately (target is already reached). Otherwise decrements `countdown`, increments `currentValue` by `step` (Linear) or multiplies by `step` (Multiplicative), returns the new `currentValue`. `noexcept`. Per-sample idiom.
- **`skip(int numSamples)`** (juce_SmoothedValue.h:330–342): block-rate advance. If `numSamples >= countdown`, snaps to `target` (smoothing complete) and returns it. Otherwise advances `currentValue += step * numSamples` (Linear branch via `skipCurrentValue`), decrements `countdown`, returns the new `currentValue`. **`noexcept`, no allocation. ~3 flops + branch — cheaper than a numSamples-long `getNextValue()` loop, and identical end-state value.**

**Canonical block-rate dispatch idiom (locked for Phase 2.2):**

```cpp
// processBlock (once per block, before voice dispatch)
const float toneTarget = parameters.getRawParameterValue ("tone")->load();
toneSmoother.setTargetValue (toneTarget);
const float toneSmoothed = toneSmoother.skip (numSamples);   // advance ramp by numSamples, return new current
```

**Why `skip(numSamples)` over alternatives:**
- (a) `getNextValue()` then `skip(numSamples - 1)` — works but reads two values where one suffices; the first-sample value is never used.
- (b) `numSamples`-long `getNextValue()` loop — ~3× the flops, identical answer.
- (c) `skip(numSamples)` then `getCurrentValue()` — `skip` already returns the new `currentValue`, so the second call is redundant.

**Family precedent (skip(numSamples) is the established Ouaricon pattern):**

- **O-Bass `Source/PluginProcessor.cpp:230`:** `float smoothedEnhanceValue = smoothedEnhance.skip(numSamples);` — block-rate read of a coefficient-driving smoother dispatched to all voices. **Direct one-line analog of the Phase 2.2 tone dispatch.**
- **O-Bass `Source/PluginProcessor.cpp:289`:** `limitIndicator.store(limitIndicatorSmooth.skip(numSamples));` — block-rate UI-bound advance (Phase 2.2 tone is processor→voice; pattern is the same).
- **O-Bass `Source/PluginProcessor.cpp:309`:** `outputLevelDB.store(outputLevelSmooth.skip(numSamples));` — same idiom.
- **O-Contrabass `Source/DSP/WaveguideString.cpp:275`:** `stiffnessSmoothed.skip (juce::jmax (0, numSamples));` — defensive `jmax(0, …)` against zero/negative `numSamples`; recommended for Phase 2.2 since `processBlock` may receive `numSamples == 0` in edge cases (host-driven sub-blocks). **Adopt the `jmax(0, numSamples)` idiom in O-Bassoon.**
- **O-Wind `Source/FluteSynthVoice.cpp:269–271`:** uses `getNextValue()` per (oversampled) sample — that's an internal-rate / per-sample consumer (different use case from Phase 2.2's block-rate processor-level coefficient dispatch). Cited for completeness; **NOT the Phase 2.2 pattern.**

**Locked Phase 2.2 idiom:**

```cpp
// PluginProcessor.h
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> toneSmoother;
float lastDispatchedTone = -1.0f;   // sentinel: forces first dispatch

// PluginProcessor::prepareToPlay
toneSmoother.reset (sampleRate, 0.050);   // 50 ms ramp per CONTEXT-rev-2 §Q3-rev-2

// PluginProcessor::processBlock — BEFORE NE drain (which is BEFORE renderNextBlock)
const float toneTarget = parameters.getRawParameterValue ("tone")->load();
toneSmoother.setTargetValue (toneTarget);   // self-no-ops when target unchanged
const float toneSmoothed = toneSmoother.skip (juce::jmax (0, buffer.getNumSamples()));

if (std::abs (toneSmoothed - lastDispatchedTone) > 0.001f)
{
    for (int v = 0; v < synthesiser.getNumVoices(); ++v)
        if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (v)))
            bv->setTone (toneSmoothed);
    lastDispatchedTone = toneSmoothed;
}
```

**Allocation audit:** `setTargetValue` is `noexcept` and only mutates four floats + an int. `skip` is `noexcept` and is a single multiply + add + countdown decrement. `dynamic_cast` is non-allocating (RTTI table lookup). `synthesiser.getNumVoices()` returns a cached int. **Zero allocations in the dispatch path. PERF-01-safe.**

---

### OQ#2-rev-2 — Tone recompute path inside `ModeBank` — lazy vs. explicit ✅ RESOLVED — **Lock: explicit `applyToneChange()`**

**Recompute decomposition** (per ARCHITECTURE.md §"Modal Resonator Biquad", lines 393–404):

For each mode `k`, the three biquad coefficients are derived from `(f_k, tau_k, fs, amp_k)` via:

```
theta_k = 2π · f_k / fs                       depends on:  f0 (via f_k = f0 · ratio[k]), fs
R_k     = exp(−1 / (tau_k · fs))              depends on:  tau_k (= T60_k_scaled / 6.91), fs
a1_k    = −2 · R_k · cos(theta_k)             depends on:  R_k, theta_k
a2_k    = R_k · R_k                           depends on:  R_k
G_k     = (1 − R_k) · amp_k                   depends on:  R_k, amp_k (= computeModeAmplitude(k, f0))
```

**Tone scope (CONTEXT-rev-2 Q4-rev-2 (b) + ARCHITECTURE §6 line 133):** tone scales `T60_k_scaled = BASE_T60[k] · mix(0.3, 1.5, tone)` for **upper modes only** (k > 4, zero-indexed → modes 5–15). Modes 0–4 are tone-invariant.

**Decomposition by trigger:**

| Trigger | What changes | What needs recompute (modes 5–15 only for tone trigger) |
|---|---|---|
| `setFundamental(f0)` (note-on, pitch-bend) | f0 → all f_k | **All 16 modes:** theta_k (cos), R_k (unchanged if tone unchanged — but cheap to recompute), amp_k (formant-Gaussian on f_k), G_k, a1_k, a2_k |
| `setTone(t)` then `applyToneChange()` | tau_k for modes 5–15 only | **Modes 5–15 only:** R_k (exp), G_k (R_k changed, amp_k unchanged), a1_k (R_k × cos(theta_k) — theta unchanged so cos is **cached**), a2_k (R_k²) |

**Decision: explicit `applyToneChange()` over lazy-on-next-setFundamental.**

**Reasons:**
1. **Responsiveness (the user-perceptible reason).** A held note with no pitch change has no `setFundamental` trigger source for the entire sustain. With lazy recompute, a tone slider sweep would only retake effect on the next note-on — an audible dead zone for the most expressive use case (long-tone sustained playing). Explicit fires immediately on dispatch.
2. **Cost asymmetry.** Lazy collapses both triggers into one recompute, but every `setFundamental` is then unconditionally a full 16-mode recompute. Explicit splits into a cheaper 11-mode tone-only path (no theta recompute, cos cached) for the throttled-epsilon-clean tone updates. **Estimated cost:** lazy ≈ 16 × (1 exp + 1 cos + 4 mults + 2 adds) per setFundamental ≈ 16 × ~12 flops + 16 × ~80 ns transcendental cost ≈ 1.5 µs. Explicit per `applyToneChange` ≈ 11 × (1 exp + 0 cos + 3 mults + 1 add) ≈ 11 × ~9 flops + 11 × ~50 ns ≈ 0.7 µs (cheaper). With throttled-epsilon dispatch, `applyToneChange` fires at most once per block (rare during steady-state); `setFundamental` fires at most on note-on/pitch-bend (similar cadence). Total cost is comparable; the explicit form has the better cost-per-trigger profile.
3. **Composability with pitch-bend.** Pitch-bend retrigger via `pitchWheelMoved → setFundamental` already exists at Phase 2.1 (BassoonVoice.cpp:81–82). With lazy, a tone change followed by a pitch-bend would surprise-recompute tone simultaneously — fine, but the composition is implicit. With explicit, every code path is unambiguous: `applyToneChange()` ↔ tone-only; `setFundamental()` ↔ both-via-explicit-cache-of-current-tone.
4. **Cosine cache is a single float per mode.** Adds 16 × 4 = 64 bytes per voice. Negligible.

**Implementation note — cache `cos(theta_k)` as a per-mode field.** `setFundamental` writes `cosTheta[k]` after computing theta; `applyToneChange` reads `cosTheta[k]` to derive new `a1_k = -2 · R_k_new · cosTheta[k]`. No second `cos()` call.

**Locked recompute sequence (verbatim for plan-phase):**

```cpp
// Per-mode state (added to ModeBiquad struct):
float cosTheta = 0.0f;   // cached cos(theta_k) for tone-only recompute (reused by applyToneChange)
float amp      = 0.0f;   // cached formant-Gaussian × roll-off — reused if only tone changes

// ModeBank::setFundamental(float f0) — full 16-mode recompute, populates cosTheta + amp caches
for (int k = 0; k < NUM_MODES; ++k)
{
    const float f_k = f0 * PARTIAL_RATIOS[k];
    if (f_k > NYQ_RATIO * fs || f_k <= 0.0f) { mute mode; continue; }

    const float theta = juce::MathConstants<float>::twoPi * f_k / fs;
    const float cosT  = std::cos (theta);
    const float amp_k = computeModeAmplitude (k, f0);                       // formant × 1/k roll-off
    const float toneScale = (k > 4)
                          ? juce::jmap (currentTone, 0.0f, 1.0f, 0.3f, 1.5f) // mix(0.3, 1.5, tone)
                          : 1.0f;
    const float tau_k = (BASE_T60[k] * toneScale) / 6.91f;
    const float R_k   = std::exp (-1.0f / (tau_k * fs));

    modes[k].cosTheta = cosT;
    modes[k].amp      = amp_k;
    modes[k].b0       = (1.0f - R_k) * amp_k;
    modes[k].a1       = -2.0f * R_k * cosT;
    modes[k].a2       = R_k * R_k;
}

// ModeBank::applyToneChange() — modes 5-15 only, reuses cached cosTheta + amp
const float fs = static_cast<float> (currentSampleRate);
for (int k = 5; k < NUM_MODES; ++k)
{
    if (modes[k].b0 == 0.0f && modes[k].a1 == 0.0f && modes[k].a2 == 0.0f) continue;  // muted (Nyquist)

    const float toneScale = juce::jmap (currentTone, 0.0f, 1.0f, 0.3f, 1.5f);
    const float tau_k     = (BASE_T60[k] * toneScale) / 6.91f;
    const float R_k       = std::exp (-1.0f / (tau_k * fs));

    modes[k].b0 = (1.0f - R_k) * modes[k].amp;        // amp unchanged — formant-Gaussian is f0-only
    modes[k].a1 = -2.0f * R_k * modes[k].cosTheta;    // cosTheta unchanged — depends on f0 only
    modes[k].a2 = R_k * R_k;
}
```

**Public ModeBank API (locked for Phase 2.2):**

```cpp
void setTone (float tone01) noexcept;          // stores currentTone; does NOT recompute (cheap)
void applyToneChange () noexcept;              // recomputes upper-half R_k for current tone (modes 5-15)
void setFundamental (float f0) noexcept;       // full 16-mode recompute (uses currentTone)
```

**Dispatch pattern at the processor (Phase 2.2):** `voice->setTone(t)` then `voice->applyToneChange()` (voice forwards both to the mode bank). Two-call pattern keeps state-update separate from work-trigger — tested invariant: code reading `voice.currentTone` between the two calls sees the new value before the recompute fires (immaterial in practice; documented for clarity).

**Alternative considered (and rejected):** combine into a single `setToneAndApply(t)` call. Rejected because plan-phase may want the throttle gate to set tone state on every block (unconditionally) and only fire `applyToneChange` when the threshold trips — separating the two enables that pattern if needed (currently CONTEXT Q4-rev-2 (b) gates BOTH at the throttle, but the seam stays open).

---

### OQ#3-rev-2 — Bassoon partial-ratio table source verification ✅ RESOLVED — **Lock: author-curated synthesis from cited references**

**ARCHITECTURE.md citation** (line 347): "Bassoon Operator: Formants in a bassoon spectrum" + Kopp Reeds harmonic analysis (research note, not a literal quote). The 16-element ratio table itself (`{1.000, 2.005, 3.010, ..., 16.186}`) is **not lifted verbatim from any single cited paper.** Provenance trace:

- **The pattern** (near-integer ratios with progressive drift `delta_k ≈ 0.005k + small higher-order term`) reflects standard observations from acoustic-instrument inharmonicity research — bassoon partials are slightly stretched above the harmonic series due to non-conical bore irregularities + reed coupling. Carillon DAFx 2017 ("Modal Synthesis of Tubular Bells", N. Ho) and the CCRMA modal-synthesis tutorials are the foundational technique sources but address bells (much more inharmonic than bassoons).
- **The specific delta values** in the ARCHITECTURE.md table (`+0.005, +0.010, +0.018, +0.024, ...`) appear to be a smooth interpolation chosen by the architecture author to bias the table toward "near-integer with monotonic stretching" without committing to any one published measurement. **No primary academic source provides these exact 16 values.**
- **No "more correct" canonical table exists** in publicly accessible literature for bassoon timbre at the 16-mode resolution. The Kopp Reeds tonal analyses and the "Bassoon Operator" blog are qualitative formant-region observations (peaks at ~450–500 Hz), not full per-mode ratio tables. SWAM-style commercial models use proprietary data.

**Conclusion:** The ARCHITECTURE.md table is **the author's curated synthesis** — informed by inharmonicity-trend literature but not bit-traceable to a single paper. **Document this in ARCHITECTURE.md as part of the Phase 2.2 backfill** (see OQ#10-rev-2 backfill format below): replace the bare citation with a footnote acknowledging the curated nature: "Values are author-curated near-integer ratios reflecting general bassoon inharmonicity trends; no single primary reference provides this exact 16-element table."

**Candidate alternatives surveyed (none adopted at rev-2):**

| Source | Approach | Why not adopted |
|---|---|---|
| Strong Sound (analysis-resynthesis from SWAM Bassoon recording) | Spectrum-extracted per-partial frequencies/decays | Requires full sinusoidal-modeling toolchain; ARCHITECTURE Risk #2 Fallback 3 framing — deferred to v1.1+ |
| Two-register tables (low/mid vs. tenor) | Different inharmonicity per register | ARCHITECTURE Risk #2 Fallback 1 framing — deferred unless rev-3 listening fails |
| Pure integer harmonics | `{1, 2, ..., 16}` (Phase 2.1 placeholder) | Sounds too "harmonic-organ" — not bassoon character |
| Literature-cited bell tables (CCRMA examples) | Strongly inharmonic, octave-stretched | Sounds bell-like; rejected at Stage 0 |

**Verify-phase listening criterion stays unchanged:** if rev-1 ratios produce a recognizable bassoon-like timbre (Gate 2 ear-only A/B + 400–600 Hz peak confirmation), ship them. If rev-2/3 listening reveals the table is off, the in-cycle iteration ceiling (rev-3) gives 2 inline tweaks before the v1.1 deferral kicks in.

---

### OQ#4-rev-2 — Formant-Gaussian peak normalisation across f0 ✅ RESOLVED — **Lock: (a) accept natural loudness variation; defer per-note normalisation to v1.1**

**Quantitative analysis** (single voice, post-mode-bank-summation, before `1/N` scaler, at unity excitation):

For any held f0, the per-mode amplitude is `amp_k = formantWeight(f_k) · rollOff(k)` where `f_k = f0 · PARTIAL_RATIOS[k]`. With unity-peak-gain biquad `G_k = (1−R_k) · amp_k` (ARCHITECTURE.md line 400), the steady-state magnitude of mode k at its own resonance peaks at `amp_k`. The mode-bank sum is a parallel filter; total RMS energy on a broadband-impulse excitation is bounded by `sqrt(sum_k amp_k²)`.

**Sum-of-amps** (proxy for perceived loudness on impulse excitation):

| Note | f0 (Hz) | Dominant modes | Sum of amp_k | Notes |
|---|---|---|---|---|
| C1 | 32.7 | k=12–15 (the harmonics nearest 475 Hz are the 14th–15th) | ~0.35 | rollOff(15) = 1/(1+7.5) = 0.118 caps loudness |
| C2 | 65.4 | k=6–8 | ~0.6 | |
| C3 | 130.8 | k=2–4 (390, 520, 657 Hz) | ~1.79 | mid-range |
| C4 | 261.6 | k=1–2 (262, 525 Hz) | ~1.55 | rollOff bites the formant-mode at k=1 |
| C5 | 523.3 | k=0 (= f0 ≈ formant) | ~1.40 | **single dominant mode** at amp 0.972 — narrow spectrum |
| C6 | 1046.5 | k=0 only (f_k > 1 kHz beyond formant) | ~0.21 | Gaussian weight × 0.5 rollOff at k=0 |

**Loudness range across C1–C6:** ~0.21 to ~1.79 → **≈ 18 dB span** (worst case, rough constructive-sum upper bound; actual perceived loudness depends on spectrum shape too).

**Recommendation: (a) accept the natural variation.**

**Rationale:**
1. **Mirrors real bassoon physics.** Real bassoons are physically quieter at the extremes (C1 needs heroic breath; C6 is barely playable). The 18 dB synthesised variation is in the same direction as acoustic reality.
2. **Phase 2.3 has the right tools.** The `breath` parameter (BRIEF default 0.7, range 0–1, Phase 2.3) and `output_gain` (default 0 dB, range −24 to +6 dB, Phase 2.3) provide ~30 dB of user-controllable trim — more than enough to compensate the ~18 dB pitch-induced variation. Per-note normalisation now would forfeit the natural physicality and force a flat loudness profile that no real bassoon has.
3. **Per-note total-weight normalisation (option b) breaks the formant invariant.** Normalising `sum_k amp_k = const` would scale the entire spectrum by an f0-dependent factor — louder for low notes, quieter for mid notes. The amplitude would compensate, but the **relative spectrum shape stays the same**. So normalisation doesn't fix the timbre — it just hides the loudness symptom. In exchange, mid-range notes (which already have the strongest formant-peaking) get attenuated, which makes them sound thinner.
4. **Hybrid (option c) adds f0-dependent code paths** for marginal benefit. Defer until evidence accumulates that listeners want flat loudness.

**Document this decision in ARCHITECTURE.md backfill** (Phase 2.2 deliverable per OQ#10-rev-2): one sentence after the `computeModeAmplitude` listing — "Per-note total-amplitude normalisation deferred to v1.1+; v1.0 accepts natural ~18 dB pitch-induced loudness variation, compensable via `breath` and `output_gain` (Phase 2.3)."

**Verify-phase listening criterion:** at Phase 2.2 we test C3 specifically (the bassoon long-tone sweet spot). C1/C5/C6 audibility checks are Phase 2.3 territory (after `breath` is wired and the user can normalise themselves).

---

### OQ#5-rev-2 — `1/N` headroom scaler retention vs. relaxation ✅ RESOLVED — **Lock: relax `1/N` to `1/8` as Phase 2.2 in-cycle tuning constant**

**Phase 2.1 baseline (measured):** voice peak ≈ −24 dBFS at C3, `1/16` scaler, flat-amplitude 16-mode bank. This is the user-confirmed Gate 1 reading.

**Phase 2.2 projection (analytical):**

- Phase 2.1 effective amp-sum at C3 = 16 (unity per mode × 16 modes).
- Phase 2.2 effective amp-sum at C3 ≈ **1.79** (per OQ#4-rev-2 table).
- Ratio: `16 / 1.79 ≈ 8.94 ≈ −19 dB`.

**Without changing the scaler**, Phase 2.2 voice peak at C3 ≈ `−24 dBFS − 19 dB = −43 dBFS`. **Far too quiet** for ear-A/B against a reference WAV at typical listening levels. The user would have to crank the Logic track gain ~20 dB to compare timbre — adds noise floor, fatigue.

**Relaxation options:**

| Scaler | dB lift over `1/16` | Projected C3 peak (Phase 2.2) | Risk on C5 (single dominant mode) | Polyphony headroom (8 voices) |
|---|---|---|---|---|
| `1/16` (keep) | 0 dB | ~−43 dBFS | safe (~−24 dBFS) | safe |
| `1/12` | +2.5 dB | ~−40 dBFS | safe (~−21 dBFS) | safe |
| `1/8` (recommended) | +6 dB | **~−37 dBFS** | safe (~−18 dBFS) | safe (~−9 dBFS at 8 voices, room for breath/gain) |
| `1/4` | +12 dB | ~−31 dBFS | tight (~−12 dBFS) | risky (~−3 dBFS at 8 voices — clipping near margin) |
| `1/2` | +18 dB | ~−25 dBFS | clip risk (~−6 dBFS) | clips at 8 voices |
| `1/1` (no scaler) | +24 dB | ~−19 dBFS | clips on impulse (~0 dBFS) | clips heavily |

**Lock: `1/8` scaler.** Delivers +6 dB lift over Phase 2.1 — enough to escape the "too quiet for A/B" zone — while preserving 8-voice polyphony headroom and avoiding clip risk at C5 (where formant lands at fundamental → narrow spectrum → highest single-mode peak).

**Why not relax further:** C5 is the worst-case loudness pitch under the formant-Gaussian model (single dominant mode at amp 0.97). At `1/4` scaler the 8-voice C5 chord projection is ~−3 dBFS — the kind of "I'm not actually clipping, but my limiter is sweating" zone that produces inter-voice intermodulation under any pitch-bend or vibrato modulation. `1/8` keeps that comfortably clear.

**Implementation:** change `1.0f / static_cast<float>(NUM_MODES)` to `1.0f / 8.0f` in `ModeBank::processSample` (`Source/ModeBank.cpp:60`), with a comment noting Phase 2.2 in-cycle tuning constant per OQ#5-rev-2 / CONTEXT Q5-rev-2 exception:

```cpp
// Phase 2.2 in-cycle tuning constant (RESEARCH §1 OQ#5-rev-2):
// formant-Gaussian + 1/k roll-off attenuates voice peak ~19 dB vs. Phase 2.1 flat-amp.
// 1/8 lifts back ~6 dB to keep C3 peak around -37 dBFS while preserving 8-voice headroom
// and C5 clip safety. Phase 2.3 wiring of output_gain replaces this with APVTS read.
return sum * (1.0f / 8.0f);
```

**Verify-phase contingency** (per CONTEXT-rev-2 Q5-rev-2 inline-iteration ceiling): if rev-1 listening reveals the chosen scaler is wrong, plan-phase budgets a single in-cycle tweak. Direction of tweak:
- **Too quiet** → relax further to `1/4` (re-listen 8-voice for clip).
- **Too loud / clipping at C5 chord** → tighten back to `1/12` or `1/16`.
- **Just right** → keep `1/8` as the as-shipped value.

The `1/8` choice IS NOT an APVTS read (still strict-ROADMAP `tone`-only wiring per CONTEXT Q2-rev-2). It's a code constant with a Phase 2.3 deletion plan.

---

### OQ#6-rev-2 — Logic Channel EQ Analyzer peak-region readout protocol ✅ RESOLVED

**Tool:** Logic Pro stock **Channel EQ** plugin with **Analyzer** mode enabled. Free, pre-installed, adequate for Phase 2.2 visual-overlay confirmation.

**Protocol** (locked for Gate 2 verification):

1. **Logic project setup:** 48 kHz sample rate, 256-sample I/O buffer (matches Phase 2.1 baseline).
2. **Track signal flow:** O-Bassoon AU on a stereo software-instrument track → **Channel EQ** as the FIRST insert post-instrument → no other processing.
3. **Channel EQ configuration:**
   - Enable **Analyzer** button (top-left of EQ window — small spectrum-graph icon).
   - **Analyzer mode = "Pre EQ"** (signal upstream of EQ curves; EQ band states don't alter what we're measuring).
   - **All 8 EQ bands DISABLED / bypassed** (we want unaltered spectrum, not a filtered measurement).
   - **Resolution: "High"** (smaller `getBlockSize` → tighter freq bins; trades update rate for resolution — fine for sustained tone capture).
   - **Frequency-axis: log scale** (default; readable across full range).
   - **Y-axis range: −60 to 0 dB** (default; covers expected −37 dBFS peak with margin).
4. **Capture procedure:**
   - In Logic's piano roll, hold a sustained MIDI C3 (MIDI note 48) for ≥ 5 s.
   - Wait 1.5 s for steady-state (initial impulse-driven transient settles within ~0.5 s; allow margin).
   - Observe the Analyzer overlay in the EQ window — visible peaks should appear at the formant-region harmonics.
   - **Confirm peak is in the 400–600 Hz region.** Use the EQ's frequency-axis labels (400 Hz and 1 kHz are typically labelled; 500 Hz reads via interpolation).
   - **Screenshot capture** (macOS: ⌘⇧4 then drag over the Channel EQ window): save to `plugins/O-Bassoon/research/reference-recordings/phase-2.2-as-shipped-c3-spectrum.png`.

**Caveats and gotchas:**

- **Analyzer is post-instrument, pre-EQ-bands** when Pre-EQ mode is selected. If the Analyzer's "Post EQ" button is toggled (right side of the analyzer toolbar), all readings reflect EQ shaping — leave it on **Pre EQ** for unbiased measurement.
- **Analyzer window only updates while audio is playing.** If the held note ends or Logic transport stops, the spectrum freezes on its last frame. Capture during active playback.
- **Single-frame snapshot may show a non-peak instant.** The mode-bank ringing has slow attack/decay envelopes — for a stationary peak, look at the spectrum 2–5 s into the held note (long after the impulse-driven onset transient).
- **Inadequate dB resolution above the formant-peak floor.** At very low levels (< −60 dBFS), some Logic versions clamp the display floor. For Phase 2.2 the peak is around −37 dBFS — well above any floor — so this is not a concern. Document for completeness.

**Acceptance criterion (Gate 2 secondary, Phase 2.2):** Visible spectral peak between 400 Hz and 600 Hz on held C3, ≥ 6 dB above the surrounding frequency bins. (Primary acceptance is the ear-only A/B per CONTEXT Q5-rev-2 (d) primary).

---

### OQ#7-rev-2 — Tone descriptors `tone=0` (woody) ↔ `tone=1` (bright) ✅ RESOLVED — **Math + perceptual prediction confirmed**

**Spec from ARCHITECTURE.md** (line 133): `T60_k_scaled = T60_k · mix(0.3, 1.5, tone)` for upper modes (k > 4).

**Numerical predictions at extremes:**

| Mode k | BASE_T60[k] (s) | T60 at tone=0 (×0.3) | T60 at tone=0.5 (×0.9) | T60 at tone=1 (×1.5) |
|---|---|---|---|---|
| 5 | 1.4 | 0.42 | 1.26 | 2.10 |
| 8 | 0.8 | **0.24** | 0.72 | 1.20 |
| 10 | 0.6 | 0.18 | 0.54 | 0.90 |
| 12 | 0.4 | 0.12 | 0.36 | 0.60 |
| 15 | 0.25 | **0.075** | 0.225 | 0.375 |

**Perceptual prediction:**

- **`tone=0` ("woody, dark"):** Upper modes 5–15 decay with T60 in the 75–420 ms range. Modes 12–15 (above 1 kHz at C3) ring for under 200 ms — sub-perceptual ("thunk" decay, not "drone"). Audible result: the formant-region modes (0–4) dominate the sustain; upper-harmonic content is heard only as a brief transient at the onset. **Predicted character: "muffled bassoon with a soft tongued attack" — woody, hollow, dark. Risk: at extreme tone=0, may sound "band-limited fundamental + formant only," approaching the band-pass-through-static-formant edge. Listen-check at verify.**
- **`tone=1` ("bright, present"):** Upper modes ring for 375 ms to 2.1 s. Modes 5–8 (in the 1–2 kHz region at C3) sustain audibly across the held note. Audible result: the upper-formant region (~2 kHz, where bassoons gain "buzz" and "edge") becomes prominent in the sustain; the sound is perceptually brighter and more articulated. **Predicted character: "open, present, slightly buzzy bassoon."**
- **`tone=0.5` (default):** A "neutral" position — upper modes decay at ~90% of their natural T60. Predicted close to a real-bassoon ambient sustain with moderate formant emphasis.

**Risk acknowledged from CONTEXT-rev-2 Risk #5:** at `tone=0`, mode-8 T60 = 240 ms — the "thunk" zone is on the edge of subjective "perceived ringing" (most listeners hear ≥ 300 ms decays as "ringing" and < 200 ms as "thumpy"). If verify-phase listening reveals tone=0 sounds artificial or band-limited, narrow the mix range to `mix(0.5, 1.3, tone)` — a smaller deviation from `T60[k]` that preserves the dark/bright shift while keeping all modes audible. Document as Phase 2.2 deviation in ARCHITECTURE.md backfill.

**Family precedent for T60-driven tone control:**

- **O-Wind: no exact analog.** Wind models tone via embouchure / cutoff-frequency, not modal damping. Different mechanism, same parameter intent.
- **O-Bowed (per `Source/DSP/StereoWidthProcessor.h` and waveguide patterns): also no direct T60-tone mechanism** — bowed strings use bow-pressure / bow-velocity for tonal shaping.
- **Closest precedent in the family is the Phase 2.2 implementation itself** — modal-synth T60 modulation is novel for Ouaricon. ARCHITECTURE.md is the primary spec; verify-phase listening is the empirical confirmation.

**Acceptance criterion (Gate 2 primary):** `tone` slider sweep [0, 1] produces audibly continuous dark↔bright character with no clicks, no zipper noise, no NaN/inf — and the extremes are recognisable as "darker" / "brighter" by the listener. **Quantitative criterion** (informal): tone=0 should sound more like Phase 2.1 (placeholder spectrum, mostly fundamental + formant); tone=1 should sound more like an idealised orchestral bassoon with sustain richness.

---

### OQ#8-rev-2 — Reference WAV pre-flight pitch audition (D4-rev-1 carry-forward) ✅ RESOLVED — **Audition is a verify-phase task, not research-phase**

**Status:** Phase 2.1 RESEARCH §1 OQ#7 sourced VSCO-2-CE files; README.md at `plugins/O-Bassoon/research/reference-recordings/README.md` contains the pre-flight checklist and explicitly notes the octave-convention check. Phase 2.1 dropped the audition from Gate 1 (item-10 dropped per user authority).

**Phase 2.2 disposition:** the audition is a **verify-phase task within Phase 2.2**, not a research-phase task. Reasons:

1. **Audition requires the Phase 2.2 build to be running** (compare reference WAV against O-Bassoon held C3). Research-phase has no built artefact to A/B against.
2. **Tuner check is a 30-second task** — load the WAV in Logic, drop a tuner on its track, hit play, read the Hz reading. Trivial during verify-phase listening; expensive to context-switch to during research-phase.
3. **The README pre-flight checklist already exists** (see Phase 2.1 RESEARCH.md `bassoon-c3-sustain-v1.wav` audition steps).

**Plan-phase deliverable:** PLAN.md should include a verify-phase checklist item: "Tuner-confirm `bassoon-c3-sustain-v1.wav` fundamental ≈ 130.81 Hz (C3, MIDI 48); if measured ≈ 261.6 Hz (C4), rename file to `bassoon-c4-sustain-v1.wav`, update README, switch reference to v2 (cross-check)."

**Contingency (if both v1 AND v2 read C4):** the user-supplied fallback path is the U Iowa MIS bassoon archive (Phase 2.1 RESEARCH OQ#7 backup #1) — sourcing from there is a Phase 2.2 verify-phase fallback (~30 minutes including download + audition).

**No research-phase action required.** Audition is locked to verify-phase.

---

### OQ#9-rev-2 — 8-voice CPU measurement protocol in Logic-AU ✅ RESOLVED

**Tool:** Logic Pro **System Performance Meter** (Window menu → Show Performance Meter), **Process bar** (top bar — current CPU usage), **48 kHz / 256-sample I/O buffer** (matches Phase 2.1 + ROADMAP test criterion).

**Protocol** (locked for Gate 2 PERF-02 early signal):

1. **Logic project state:**
   - 48 kHz / 256-sample buffer (Logic Settings → Audio → Buffer = 256, Sample Rate = 48000).
   - Single stereo software-instrument track with O-Bassoon AU.
   - **No other audio sources active** (silence all other tracks, freeze unrelated regions).
   - Performance Meter visible (Window → Show Performance Meter) before measurement begins.

2. **Trigger 8 simultaneous voices** (the key part — voice cap is Phase 2.4, so the host's note routing is what determines how many of the 16 pre-allocated `juce::Synthesiser` voices ring concurrently):
   - **Recommended chord:** C3 + E3 + G3 + Bb3 + C4 + E4 + G4 + Bb4 (eight notes — two C7 chords spanning two octaves). Distinct pitches → no voice-stealing collisions; no octave doubling that might mask voice count under JUCE's `juce::Synthesiser` default behaviour (which by default does NOT enforce a fixed cap until Phase 2.4 wires `voice_count`, BUT does steal voices on retrigger of the same note number — the chord avoids retriggers entirely).
   - **Hold method:** record the chord into a MIDI region, set the region to loop, play; OR use the Logic on-screen MIDI keyboard with two hands on a USB controller; OR send the chord from a static MIDI region with all 8 notes overlapping in time.
   - **Verification that 8 voices are actually active:** during sustained chord, check Logic's Channel EQ Analyzer (per OQ#6-rev-2) — confirm the spectrum shows the 8 distinct fundamental peaks at C3/E3/G3/Bb3/C4/E4/G4/Bb4 frequencies. If the spectrum shows only 4–6 peaks, voice-stealing is firing and the measurement is invalid.

3. **Read the Process bar** during sustained 8-note ringing (≥ 3 s into the chord). Note the percentage. Single-bar reading is sufficient at Phase 2.2 — Phase 2.4 will tighten the protocol when polyphony enforcement is wired.

**Bar:** **< 20 % CPU on Process bar** (CONTEXT-rev-2 Q8-rev-2 + ROADMAP Phase 2.2 test criterion).

**Caveats:**

- **8 simultaneous note-ons vs. 8 voices:** `juce::Synthesiser` allocates ONE voice per simultaneous note-on (until 16 voices are exhausted). Voice cap is enforced at Phase 2.4; at Phase 2.2 the host can drive up to 16 voices simultaneously. The 8-note chord is therefore a clean 8-voice probe.
- **Logic CPU meter is host-aggregated** — includes Logic's own overhead, plug-in scan threads, GUI repaint costs. Practical noise floor: ~3–5% with a single empty stereo track. Subtract this baseline mentally (or via a "no notes playing" reference reading) for the cleanest signal.
- **System Performance Meter > Activity Monitor:** Logic's meter reflects the audio thread specifically; Activity Monitor reflects all threads including UI. For DSP regression checks, Logic is the canonical reading.

**Contingency** (per CONTEXT-rev-2 Q8-rev-2): if 8-voice CPU > 20 %, **trigger ARCHITECTURE Risk #1 Fallback 1 BEFORE finalising the partial table.** Drop `NUM_MODES` from 16 to 8 in `ModeBank.h`, regenerate `PARTIAL_RATIOS` and `BASE_T60` arrays as 8-element subsets (keep modes 0–4 plus the most prominent of 5–7 by formant-weight at C3), re-tune. Cheaper to swap pre-tune than post-tune.

---

### OQ#10-rev-2 — ARCHITECTURE.md backfill format ✅ RESOLVED

**Lock: append-rev-note format, with optional as-shipped subsection ONLY if rev-2/3 listening tweaks the partial table.**

**File:** `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md`

**Default case (rev-1 ratios shipped unchanged at Gate 2 PASS):**

Append AT THE END of the existing §"Bassoon Partial Table" section (line 386, after the "Acceptance test for FUNC-01" paragraph):

```markdown
**Phase 2.2 As-Shipped Note (added 2026-04-27, atomic commit `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`):**

The Phase 2.2 implementation lands the partial-ratio table verbatim from this section
(`PARTIAL_RATIOS = {1.000, 2.005, ..., 16.186}`) and the `computeModeAmplitude` formula
verbatim. The values are author-curated near-integer ratios reflecting general
bassoon-inharmonicity trends (per Phase 2.2 RESEARCH.md §1 OQ#3-rev-2); no single
primary academic reference provides this exact 16-element table. Per-note total-amplitude
normalisation across f0 was deferred to v1.1+ (per Phase 2.2 RESEARCH.md §1 OQ#4-rev-2);
v1.0 accepts ~18 dB natural pitch-induced loudness variation, compensable via `breath`
and `output_gain` (Phase 2.3). The `1/N` per-voice headroom scaler in `ModeBank.cpp` was
relaxed from `1/16` (Phase 2.1) to `1/8` (Phase 2.2) as an in-cycle tuning constant
(per Phase 2.2 RESEARCH.md §1 OQ#5-rev-2); replacement by `output_gain` APVTS read
remains a Phase 2.3 deliverable.
```

**Iteration case (rev-2 or rev-3 listening adjusts any partial-ratio value):**

Insert a new subsection after line 384 (between `}` and the FUNC-01 acceptance test paragraph):

```markdown
**Phase 2.2 As-Shipped Partial Table (rev-N, NNNN-NN-NN):**

The Phase 2.2 listening loop revealed that <one-line description of issue>. The
following table is the as-shipped partial-ratio table; the ratios above represent the
original spec.

```cpp
static constexpr std::array<float, 16> PARTIAL_RATIOS = {
    1.000f, 2.005f, ...   // <as-shipped values>
};
```

Reasoning: <one paragraph describing the listening evidence and the rationale for the change>.
```

**Append rev note at the end of the file** (regardless of which case):

Add to the end of ARCHITECTURE.md:

```markdown
---

## Revision History

- **2026-04-27** (Phase 2.2 as-shipped): partial-ratio table + formant-Gaussian
  amplitude shaping landed verbatim from §"Bassoon Partial Table" / §"Tone /
  Brightness Control"; `1/N` headroom scaler relaxed to `1/8` (Phase 2.3 supersedes);
  per-note loudness normalisation deferred to v1.1+. See Phase 2.2 RESEARCH.md §1
  OQ#3-rev-2, OQ#4-rev-2, OQ#5-rev-2 for full rationale. Atomic commit:
  `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`.
```

If a `## Revision History` section already exists, append the new entry as a new bullet.

**Plan-phase deliverable:** PLAN.md task list MUST include `research/ARCHITECTURE.md` as a Phase 2.2 atomic-commit artefact. Verify-phase grep MUST confirm the partial-ratio block in `Source/ModeBank.h` exactly matches the values in ARCHITECTURE.md (or the as-shipped subsection if rev-N changes were made).

**Verification grep template (verify-phase):**

```bash
# Extract partial-ratio block from ARCHITECTURE.md (text between PARTIAL_RATIOS = { and };)
arch_ratios=$(awk '/PARTIAL_RATIOS = \{/,/\};/' plugins/O-Bassoon/.planning/research/ARCHITECTURE.md \
              | grep -oE '[0-9]+\.[0-9]+f' | sort | tr '\n' ' ')
# Extract from source
src_ratios=$(awk '/PARTIAL_RATIOS = \{/,/\};/' plugins/O-Bassoon/Source/ModeBank.h \
              | grep -oE '[0-9]+\.[0-9]+f' | sort | tr '\n' ' ')
# Compare
[ "$arch_ratios" = "$src_ratios" ] && echo "PASS: ratios match" || echo "FAIL: ratios diverge"
```

---

## §2 — Pattern Confirmations (rev-2)

### Processor-level SmoothedValue dispatch loop precedent

**Primary precedent (lift verbatim):** **O-Bass `Source/PluginProcessor.cpp:230`**

```cpp
float smoothedEnhanceValue = smoothedEnhance.skip(numSamples);
// dispatched downstream to all voices
```

This is the exact one-line idiom Phase 2.2 will adopt for the `tone` smoother. The same file demonstrates the `setTargetValue` pattern at line 196: `smoothedEnhance.setTargetValue(targetEnhance);` — called once per block before `skip(numSamples)`.

**Secondary precedent (defensive idiom):** **O-Contrabass `Source/DSP/WaveguideString.cpp:275`**

```cpp
stiffnessSmoothed.skip (juce::jmax (0, numSamples));
```

The `juce::jmax(0, numSamples)` guard handles the edge case of zero-length sub-blocks (host-driven). **Adopt this guard in Phase 2.2 dispatch.**

**Throttled-epsilon dispatch precedent:** No exact precedent in O-Wind / O-Lyrica / O-Contrabass (those plugins all per-sample read smoothers internal to their voice — not processor→voice dispatch). Phase 2.2's throttle gate at the dispatch site is **a new pattern for the family** but the underlying smoother and the `dynamic_cast`-driven voice iteration both follow established idioms. The new pattern is documented in CONTEXT-rev-2 Q4-rev-2 and `applyToneChange` decomposition is documented in OQ#2-rev-2 above. **Family-novel but well-grounded.**

**Voice-iteration pattern:** O-Bassoon's existing Phase 2.1 `prepareToPlay` already iterates voices via `dynamic_cast<BassoonVoice*>(synthesiser.getVoice(v))` — see `Source/PluginProcessor.cpp:144–146`. Phase 2.2 lifts that pattern verbatim into the tone-dispatch site.

---

### Reuse vs. extend at Phase 2.2

| Phase 2.1 Code | Phase 2.2 Action | Notes |
|---|---|---|
| `Source/Exciter.{h,cpp}` | **No changes** | Sustain-noise + attack-character morph deferred to Phase 2.4 |
| `Source/BassoonVoice` per-sample render loop | **No changes** | Loop ordering preserved verbatim |
| `Source/BassoonVoice::startNote / pitchWheelMoved` | **No changes** | f0 update path unchanged; voice's setTone is independent |
| `Source/PluginProcessor::processBlock` NE-drain ordering | **Preserve verbatim** | Tone dispatch inserts BEFORE NE drain (which is BEFORE renderNextBlock) |
| `Source/ModeBank` mute policy (Nyquist), NaN guard | **Preserve verbatim** | Phase 2.2 only changes amplitude + tone-driven R_k path |
| `1/N` headroom scaler | **Relax to `1/8`** | Per OQ#5-rev-2; Phase 2.3 supersedes via output_gain |

---

## §3 — Implementation Skeletons (rev-2)

### `Source/ModeBank.h` (rev-2 deltas to Phase 2.1)

```cpp
#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

class ModeBank
{
public:
    static constexpr int   NUM_MODES   = 16;
    static constexpr float FORMANT_F1  = 475.0f;   // Hz, first formant centre (ARCHITECTURE §"Bassoon Partial Table")
    static constexpr float FORMANT_BW  = 200.0f;   // Hz, formant bandwidth

    // rev-2: bassoon-tuned near-integer ratios (replaces Phase 2.1 integer placeholders).
    // Source: ARCHITECTURE.md §"Bassoon Partial Table" (author-curated; see RESEARCH.md §1 OQ#3-rev-2).
    static constexpr std::array<float, NUM_MODES> PARTIAL_RATIOS = {
        1.000f,  2.005f,  3.010f,  4.018f,  5.024f,  6.032f,  7.041f,  8.052f,
        9.064f, 10.078f, 11.092f, 12.108f, 13.125f, 14.144f, 15.164f, 16.186f
    };

    // BASE_T60 unchanged from Phase 2.1.
    static constexpr std::array<float, NUM_MODES> BASE_T60 = {
        2.5f, 2.2f, 2.0f, 1.8f, 1.6f, 1.4f, 1.2f, 1.0f,
        0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.35f, 0.30f, 0.25f
    };

    void  prepare         (double sampleRate);
    void  setFundamental  (float f0);
    void  setTone         (float tone01) noexcept;     // rev-2: stores currentTone (no recompute)
    void  applyToneChange () noexcept;                 // rev-2: recomputes upper-half R_k for currentTone
    float processSample   (float excitation) noexcept;
    void  reset           () noexcept;

private:
    static float computeModeAmplitude (int k, float f0) noexcept;     // rev-2: formant-Gaussian × roll-off

    struct ModeBiquad
    {
        float b0       = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float y1       = 0.0f, y2 = 0.0f;
        float cosTheta = 0.0f;   // rev-2: cached cos(theta_k) for tone-only recompute
        float amp      = 0.0f;   // rev-2: cached formant-Gaussian × roll-off

        inline float processSample (float x) noexcept
        {
            const float y0 = b0 * x - a1 * y1 - a2 * y2;
            y2 = y1;
            y1 = y0;

            if (! std::isfinite (y1) || ! std::isfinite (y2))
            {
                y1 = 0.0f;
                y2 = 0.0f;
                return 0.0f;
            }
            return y0;
        }

        void reset() noexcept { y1 = 0.0f; y2 = 0.0f; }
    };

    static constexpr float NYQ_RATIO = 0.45f;

    std::array<ModeBiquad, NUM_MODES> modes {};
    double currentSampleRate = 48000.0;
    float  currentTone       = 0.5f;     // rev-2: default to 0.5 (matches APVTS default)
};
```

### `Source/ModeBank.cpp` (rev-2 implementation)

```cpp
#include "ModeBank.h"

void ModeBank::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    reset();
}

void ModeBank::setFundamental (float f0)
{
    const float fs       = static_cast<float> (currentSampleRate);
    const float nyqLimit = NYQ_RATIO * fs;

    for (int k = 0; k < NUM_MODES; ++k)
    {
        const float f_k = f0 * PARTIAL_RATIOS[static_cast<size_t> (k)];

        if (f_k > nyqLimit || f_k <= 0.0f)
        {
            modes[static_cast<size_t> (k)].b0       = 0.0f;
            modes[static_cast<size_t> (k)].a1       = 0.0f;
            modes[static_cast<size_t> (k)].a2       = 0.0f;
            modes[static_cast<size_t> (k)].cosTheta = 0.0f;
            modes[static_cast<size_t> (k)].amp      = 0.0f;
            continue;
        }

        const float theta     = juce::MathConstants<float>::twoPi * f_k / fs;
        const float cosT      = std::cos (theta);
        const float amp_k     = computeModeAmplitude (k, f0);
        const float toneScale = (k > 4)
                              ? juce::jmap (currentTone, 0.0f, 1.0f, 0.3f, 1.5f)
                              : 1.0f;
        const float tau_k     = (BASE_T60[static_cast<size_t> (k)] * toneScale) / 6.91f;
        const float R_k       = std::exp (-1.0f / (tau_k * fs));

        modes[static_cast<size_t> (k)].cosTheta = cosT;
        modes[static_cast<size_t> (k)].amp      = amp_k;
        modes[static_cast<size_t> (k)].b0       = (1.0f - R_k) * amp_k;
        modes[static_cast<size_t> (k)].a1       = -2.0f * R_k * cosT;
        modes[static_cast<size_t> (k)].a2       = R_k * R_k;
    }
}

void ModeBank::setTone (float tone01) noexcept
{
    currentTone = juce::jlimit (0.0f, 1.0f, tone01);
}

void ModeBank::applyToneChange() noexcept
{
    const float fs        = static_cast<float> (currentSampleRate);
    const float toneScale = juce::jmap (currentTone, 0.0f, 1.0f, 0.3f, 1.5f);

    // Modes 5-15 only — modes 0-4 are tone-invariant per ARCHITECTURE §"Tone / Brightness Control".
    for (int k = 5; k < NUM_MODES; ++k)
    {
        auto& m = modes[static_cast<size_t> (k)];
        // Skip muted (above-Nyquist) modes — leave coefficients zeroed.
        if (m.amp == 0.0f) continue;

        const float tau_k = (BASE_T60[static_cast<size_t> (k)] * toneScale) / 6.91f;
        const float R_k   = std::exp (-1.0f / (tau_k * fs));

        m.b0 = (1.0f - R_k) * m.amp;
        m.a1 = -2.0f * R_k * m.cosTheta;
        m.a2 = R_k * R_k;
    }
}

float ModeBank::computeModeAmplitude (int k, float f0) noexcept
{
    const float f_k          = f0 * PARTIAL_RATIOS[static_cast<size_t> (k)];
    const float dist         = (f_k - FORMANT_F1) / FORMANT_BW;
    const float formantWeight = std::exp (-0.5f * dist * dist);
    const float rollOff       = 1.0f / (1.0f + 0.5f * static_cast<float> (k));
    return formantWeight * rollOff;
}

float ModeBank::processSample (float excitation) noexcept
{
    float sum = 0.0f;
    for (auto& m : modes)
        sum += m.processSample (excitation);

    // Phase 2.2 in-cycle tuning constant (RESEARCH §1 OQ#5-rev-2):
    // formant-Gaussian + 1/k roll-off attenuates voice peak ~19 dB vs. Phase 2.1 flat-amp.
    // 1/8 lifts back ~6 dB to keep C3 peak around -37 dBFS while preserving 8-voice headroom
    // and C5 clip safety. Phase 2.3 wiring of output_gain replaces this with APVTS read.
    return sum * (1.0f / 8.0f);
}

void ModeBank::reset() noexcept
{
    for (auto& m : modes)
        m.reset();
}
```

### `Source/BassoonVoice.h` (rev-2 deltas to Phase 2.1)

```cpp
class BassoonVoice : public juce::SynthesiserVoice
{
public:
    // ... Phase 2.1 surface unchanged ...

    void setTone (float tone01) noexcept;     // rev-2: NEW — forwards to modeBank, then triggers applyToneChange

    // ... all other methods unchanged ...
};
```

### `Source/BassoonVoice.cpp` (rev-2 single addition)

```cpp
void BassoonVoice::setTone (float tone01) noexcept
{
    modeBank.setTone (tone01);
    modeBank.applyToneChange();   // dispatched from processor only when delta > 0.001 (throttle gate)
}
```

**No other BassoonVoice.cpp changes required.** `startNote`, `stopNote`, `pitchWheelMoved`, `controllerMoved`, `renderNextBlock`, `prepareToPlay` all unchanged from Phase 2.1.

### `Source/PluginProcessor.h` (rev-2 deltas)

```cpp
private:
    juce::AudioProcessorValueTreeState        parameters;
    juce::Synthesiser                         synthesiser;
    TuningEngine                              tuningEngine;
    Ouaricon::NoteExpression::VST3Extensions  vst3Extensions;

    // rev-2 NEW members:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> toneSmoother;
    float                                                          lastDispatchedTone = -1.0f;
```

### `Source/PluginProcessor.cpp` (rev-2 deltas)

```cpp
// In prepareToPlay (after synthesiser.setCurrentPlaybackSampleRate, before per-voice loop):
toneSmoother.reset (sampleRate, 0.050);   // 50 ms ramp per CONTEXT-rev-2 §Q3-rev-2

// In processBlock (BEFORE vst3Extensions.drainAndUpdate()):
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

vst3Extensions.drainAndUpdate();
synthesiser.renderNextBlock (buffer, midiMessages, 0, numSamples);
```

**Critical ordering invariant** (preserved from Phase 2.1, extended at Phase 2.2):

1. `buffer.clear()` (already at PluginProcessor.cpp:173)
2. **`tone` smoother advance + voice dispatch** (rev-2 NEW)
3. `vst3Extensions.drainAndUpdate()` (PluginProcessor.cpp:178)
4. `synthesiser.renderNextBlock()` (PluginProcessor.cpp:182)

The tone dispatch sits BEFORE the NE drain (so voice state is fully up-to-date when JUCE iterates voice events) — same principle as the Phase 2.1 NE drain ordering.

### `plugins/O-Bassoon/CMakeLists.txt`

**No source-list changes required.** `ModeBank.{h,cpp}`, `BassoonVoice.{h,cpp}`, `PluginProcessor.{h,cpp}` are already in `target_sources` (Phase 2.1 commit). All Phase 2.2 changes are edits to existing files.

---

## §4 — Discrepancies (rev-2)

### D1-rev-2 — `juce::jmap` is the canonical `mix(a, b, t)` in JUCE 8

**ARCHITECTURE.md** (line 133) writes `mix(0.3, 1.5, tone)` in pseudo-code. JUCE 8 provides `juce::jmap(value, sourceMin, sourceMax, targetMin, targetMax)` for this exact purpose: `juce::jmap(currentTone, 0.0f, 1.0f, 0.3f, 1.5f)`. Locked in skeletons above.

**Resolution:** Non-blocking. Implementation skeleton uses `juce::jmap` verbatim.

---

### D2-rev-2 — `currentTone` default value: ARCHITECTURE silent, APVTS default = 0.5

**ARCHITECTURE.md** does not specify a default `currentTone` in the ModeBank's pre-first-setTone state. APVTS parameter spec (PluginProcessor.cpp:62) sets `tone` default = 0.5. To avoid a discontinuity when the first `setTone(0.5)` arrives (mode bank coefficients computed with `currentTone = 0` before the first APVTS read), **ModeBank initialises `currentTone = 0.5f` to match the APVTS default**.

**Resolution:** Non-blocking. Locked in `ModeBank.h` member initialiser.

---

### D3-rev-2 — APVTS `tone` parameter default = 0.5 (verified)

PluginProcessor.cpp lines 58–63 define `tone` with default 0.5, range [0.0, 1.0], step 0.001. **Confirms CONTEXT-rev-2 / parameter-spec-draft.md default.** No discrepancy; documented for the as-shipped record.

**Resolution:** No action required.

---

### D4-rev-2 — Mode-index convention: ARCHITECTURE.md zero-indexed, but says "k > 4"

**ARCHITECTURE.md line 133** says "for upper modes (k > 4)" — at zero-indexed `k`, this is modes 5–15 (eleven modes). CONTEXT-rev-2 lock confirms zero-indexed (Q4-rev-2 / "Mode-index convention for 'k > 4'"). Implementation skeletons above use `for (int k = 5; k < NUM_MODES; ++k)`.

**Resolution:** Non-blocking. Convention locked. Verify-phase grep should confirm `for (int k = 5;` appears in `applyToneChange` (not `for (int k = 4;` or `k >= 4`).

---

### D5-rev-2 — Throttle-gate epsilon = 0.001 corresponds to 0.1% of tone range

CONTEXT-rev-2 Q4-rev-2 (b) locks `epsilon = 0.001f`. With `tone ∈ [0, 1]`, this is 0.1% of full range. At `mix(0.3, 1.5, tone)`, that's 0.0012 in T60-scale-factor space → for mode 8 (BASE_T60 = 0.8 s), a T60 shift of 0.96 ms → R_k shift of `~exp(-1/(0.000139 · fs))` change ≈ 4e-7 in R_k. **Sub-LSB on 32-bit float coefficients.** The throttle is conservative — the audible threshold for T60 jitter is well above 1 ms shift on a 240 ms decay.

**Resolution:** No action. Lock confirmed audibly safe.

---

### D6-rev-2 — `dynamic_cast` overhead per dispatch acceptable

Each voice-dispatch fires up to 16 `dynamic_cast<BassoonVoice*>` calls when the throttle trips. Cost per cast: ~10–20 ns on M1 (RTTI table lookup + type-check). 16 casts ≈ 240 ns worst case, fired at most once per block (post-throttle). At 48 kHz / 256-sample blocks (5.3 ms / block), 240 ns is 0.005% of block time. **Negligible.**

Alternative: store voices as `std::vector<BassoonVoice*>` pre-cast in the constructor, iterate by raw pointer. **Rejected at Phase 2.2** — adds a redundant container, breaks the single-source-of-truth invariant (`juce::Synthesiser` is the voice owner). Preserve `dynamic_cast` for clarity.

**Resolution:** No action. Phase 2.4 may revisit if 8-voice CPU profiling reveals a hotspot (currently projected ~16 % at 8 voices per ARCHITECTURE.md, well under the 25 % bar).

---

## §5 — Outputs and Handoff Checklist (rev-2)

✅ **All 10 CONTEXT-rev-2 Open Questions resolved** (§1)
✅ **Family precedents confirmed** (§2 — `skip(numSamples)` block-rate dispatch idiom from O-Bass + O-Contrabass; voice-iteration pattern from existing Phase 2.1 code)
✅ **`setTone` recompute path locked: explicit `applyToneChange()` with cached `cosTheta` + `amp` per mode** (§1 OQ#2-rev-2 + §3)
✅ **`1/N` scaler relaxation locked: `1/8` (Phase 2.2 in-cycle tuning constant)** (§1 OQ#5-rev-2)
✅ **Implementation skeletons for ModeBank rev-2, BassoonVoice rev-2 (single addition), PluginProcessor rev-2 (smoother + dispatch + ordering)** (§3)
✅ **6 discrepancies documented with resolutions; none block planning** (§4)
✅ **ARCHITECTURE.md backfill template locked** (§1 OQ#10-rev-2)
✅ **Verify-phase audition + 8-voice CPU + EQ Analyzer protocols documented** (§1 OQ#6/8/9-rev-2)

**Handoff to plan-phase:** PLAN-rev-2.md should consume §3 implementation skeletons verbatim as task body content, sequence the changes as a single atomic Wave (3 source edits + 1 ARCHITECTURE.md backfill + 0 CMakeLists changes — no parallel sub-waves; gate-first principle), and pin the Gate 2 PASS bar from CONTEXT-rev-2 Q5-rev-2 + Q8-rev-2 as the wave's verification condition. Verify-phase tasks include: (1) tuner-confirm v1 WAV pitch (D4-rev-1 carry-forward), (2) Logic Channel EQ Analyzer screenshot capture, (3) 8-voice CPU < 20 % reading, (4) ear-only A/B against v1 WAV, (5) regression check that Phase 2.1 invariants (RT-safety grep zero, NE drain ordering, etc.) still hold.

**No CONTEXT amendments required.** All discrepancies in §4 are non-blocking (D1: standard JUCE helper; D2/D3/D4: defaults align across spec/code; D5: epsilon is sub-LSB safe; D6: dynamic_cast cost negligible).

---

## Audit Trail (rev-2 addendum)

**rev-2 (this addendum, 2026-04-27):** Phase 2.2 research phase. All 10 CONTEXT-rev-2 open questions resolved with JUCE 8.0.4 source-line citations and family-precedent confirmation. 6 discrepancies surfaced (D1-rev-2 through D6-rev-2); none block planning. Key locks:

- **OQ#1-rev-2:** `juce::SmoothedValue::skip(juce::jmax(0, numSamples))` is the locked block-rate advance idiom (precedent: O-Bass `PluginProcessor.cpp:230`, O-Contrabass `WaveguideString.cpp:275`).
- **OQ#2-rev-2:** **explicit `applyToneChange()`** wins over lazy-on-next-setFundamental — preserves slider responsiveness on held notes; `cosTheta` and `amp` cached per-mode for cheap upper-half (k=5–15) recompute.
- **OQ#3-rev-2:** ARCHITECTURE partial-ratio table is **author-curated synthesis**; no single primary academic source. Document in backfill.
- **OQ#4-rev-2:** Per-note total-amplitude normalisation **deferred to v1.1+**; v1.0 accepts ~18 dB natural pitch-induced loudness variation.
- **OQ#5-rev-2:** `1/N` scaler **relaxed from `1/16` to `1/8`** as in-cycle tuning constant (+6 dB lift, preserves polyphony headroom + C5 clip safety).
- **OQ#6-rev-2:** Logic Channel EQ Analyzer protocol locked — Pre-EQ mode, all bands disabled, screenshot to `research/reference-recordings/phase-2.2-as-shipped-c3-spectrum.png`.
- **OQ#7-rev-2:** Tone descriptors verified — `tone=0` produces 75–420 ms upper-mode T60 (woody/dark), `tone=1` produces 375 ms–2.1 s (bright/present); narrow-range fallback `mix(0.5, 1.3, tone)` available if extreme sounds artificial.
- **OQ#8-rev-2:** Reference WAV pitch audition is a **verify-phase task**, not research-phase — checklist already in `research/reference-recordings/README.md`.
- **OQ#9-rev-2:** 8-voice CPU protocol locked — C3+E3+G3+Bb3+C4+E4+G4+Bb4 chord, hold ≥ 3 s, read Logic Process bar, < 20 % bar.
- **OQ#10-rev-2:** ARCHITECTURE.md backfill format locked — append-rev-note default; as-shipped subsection only if rev-N changes the partial table.

Implementation skeletons for ModeBank.{h,cpp}, BassoonVoice.{h,cpp} (single addition), PluginProcessor.{h,cpp} prepared for plan-phase consumption (§3). CMakeLists requires no edits at Phase 2.2.

**Inherited verbatim from rev-1 (not re-litigated):**
- JUCE 8 ADSR API + ordering
- SynthesiserVoice render convention (sum into buffer, host clears)
- Biquad pole-only specialisation
- Voice exit path via `clearCurrentNote() + modeBank.reset()`
- Reference recording sourcing (VSCO-2-CE CC0, files already archived at Phase 2.1)
- Spectrum baseline capture procedure (SPAN tool dropped, replaced by Logic EQ Analyzer per OQ#6-rev-2)
- Logic CPU verification protocol (System Performance Meter, Process bar)
- Per-sample render-loop ordering

**New in rev-2:**
- Block-rate `skip(numSamples)` SmoothedValue idiom locked
- `applyToneChange` explicit recompute path with cached `cosTheta` / `amp`
- Mode-index convention `k > 4` zero-indexed → modes 5–15 verified
- Throttled-epsilon dispatch precedent acknowledged as family-novel; gate location at processor dispatch site
- `1/N` relaxation to `1/8` with quantitative justification
- Logic Channel EQ Analyzer Pre-EQ-mode protocol (replaces SPAN)
- 8-note chord protocol for 8-voice CPU early signal
- ARCHITECTURE.md backfill default (append-rev-note) + iteration-case (as-shipped subsection) templates
- Discrepancy register (D1-rev-2 through D6-rev-2) covering jmap, currentTone default, mode-index convention, throttle epsilon math, dynamic_cast overhead
