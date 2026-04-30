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

---

## §1 — Open Questions Resolved (rev-3)

### OQ#1-rev-3 — `juce::AudioBuffer::applyGainRamp` vs per-block `applyGain` for output_gain declick ✅ RESOLVED — **Lock: `applyGainRamp(0, numSamples, current, smoother.skip(N))`**

JUCE 8.0.4 source: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/buffers/juce_AudioSampleBuffer.h`:736–753 (per-channel) and :764–769 (all-channels overload).

```cpp
void applyGainRamp (int channel, int startSample, int numSamples,
                    Type startGain, Type endGain) noexcept
{
    if (isClear) return;
    const auto increment = (endGain - startGain) / (float) numSamples;
    auto* d = channels[channel] + startSample;
    while (--numSamples >= 0) { *d++ *= startGain; startGain += increment; }
}

void applyGainRamp (int startSample, int numSamples, Type startGain, Type endGain) noexcept
{
    for (int i = 0; i < numChannels; ++i)
        applyGainRamp (i, startSample, numSamples, startGain, endGain);
}
```

**Locked idiom:**
```cpp
const float gainStart = outputGainSmoother.getCurrentValue();
outputGainSmoother.setTargetValue(linearGain);
const float gainEnd = outputGainSmoother.skip(juce::jmax(0, numSamples));
buffer.applyGainRamp(0, numSamples, gainStart, gainEnd);
```

**Why this form, not `applyGain(smoother.getNextValue())`:**
- `applyGainRamp` performs **per-sample linear interpolation** between `startGain` and `endGain` across the buffer (256 samples ≈ 5.3 ms at 48 kHz). Eliminates block-boundary stair-stepping that `applyGain(getNextValue())` introduces (one gain value across 256 samples → audible at fast automation).
- `smoother.skip(N)` advances the SmoothedValue's internal countdown by N samples and returns the resulting current value (juce_SmoothedValue.h:330–342). It IS the canonical "advance smoother and read end-of-block target" call. Allocation-free.
- Two SmoothedValue reads (`getCurrentValue` BEFORE `skip`, `skip` returns endGain) capture the block's start→end gain window correctly.
- Matches Phase 2.2 `toneSmoother.skip(juce::jmax(0, numSamples))` precedent at `plugins/O-Bassoon/Source/PluginProcessor.cpp:187` for the dispatch-throttle path; the only delta is reading `getCurrentValue()` first and feeding both ends to `applyGainRamp` instead of dispatching the result to voices.

**Numeric edge:** `jmax(0, numSamples)` defends against zero-length blocks (JUCE may pass numSamples=0 in some host contexts). At numSamples=0, `applyGainRamp` divides by zero in the increment computation — but the inner `while (--numSamples >= 0)` loop never executes, so the `increment` is never used. Still, defensive `jmax` matches O-Contrabass `WaveguideString.cpp:275` precedent.

**Cross-check:** O-Bowed PluginProcessor.cpp:412–413 uses **`buffer.applyGain(juce::Decibels::decibelsToGain(outputLevel))`** with NO smoother — single block-rate gain step. O-Lyrica PluginProcessor.cpp:961 same pattern. Both rely on host-level smoothing of the output bus to mask the step. Phase 2.3 retains the CONTEXT-rev-3 30 ms smoother + applyGainRamp combination: the additional declick is cheap (~256 muls per block × 1 stereo pair = 512 muls vs. 256 for plain applyGain — negligible CPU) and forecloses the "rapid DAW automation lane on output_gain" risk class entirely. Documented as **deviation from O-Bowed/O-Lyrica precedent in favor of explicit declick guarantee** (D4-rev-3).

---

### OQ#2-rev-3 — `juce::ADSR` block-rate `setParameters` mid-envelope semantics ✅ RESOLVED — **Lock: no internal smoother needed**

JUCE 8.0.4 source: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_ADSR.h`:92–99.

```cpp
void setParameters (const Parameters& newParameters)
{
    jassert (sampleRate > 0.0);
    parameters = newParameters;
    recalculateRates();
}
```

**Mid-envelope behavior analysis (state-by-state):**

| State at `setParameters` call | What `recalculateRates()` changes | `envelopeVal` | Audible artifact |
|---|---|---|---|
| `attack` | `attackRate` recomputed (1/(attack·sr)) | preserved | Slope inflection, NOT click. Sub-perceptual unless attack changes >2× |
| `decay` | `decayRate` recomputed | preserved | Slope inflection during decay; we use `sustain=1.0` so `decay=0` and decay phase is skipped (juce_ADSR.h:136–145) |
| `sustain` | rates change but state stays sustain → `envelopeVal = parameters.sustain` constant 1.0 | preserved at 1.0 | None (envelope flat at 1.0) |
| `release` | `releaseRate` is recomputed at noteOff (line 155) — mid-release `setParameters` changes attack/decay rates which don't affect ongoing release | preserved | None |

**Why no smoother needed:**

1. ADSR mid-note parameter automation is rare (DAW automation lanes are the primary source; per-block dispatch at <50 Hz dispatch rate is well below audible smear threshold).
2. With `sustain=1.0`, `decay=0` (CONTEXT-rev-3 line 446: `adsr.setParameters({attack/1000, 0.0, 1.0, release/1000})`), the envelope effectively has only attack and release phases. Mid-attack rate change produces a slope inflection at the dispatch boundary — slope discontinuity is sub-perceptual at audio rate (no waveform discontinuity = no click).
3. Throttled-epsilon dispatch (`|new - lastDispatched| > 0.001f`) skips redundant `setParameters` calls when attack/release values haven't changed > 0.1 % of range — quiescent DAW playback hits zero ADSR re-shapings per block.

**Lock:** Block-rate `setParameters` with epsilon throttle. No internal SmoothedValue around `attack_time` or `release_time`. Matches CONTEXT-rev-3 default.

**Verify-phase fallback (deferred):** If a manual sweep of `attack_time` 0→2000 ms during sustained note produces an audible "wobble" at the dispatch boundary (Gate 3 item 1), add `juce::SmoothedValue<float>` around attack/release params at processor scope (50 ms `Linear`, dispatch from smoothed value not raw APVTS read). Cost: +24 bytes per param + 1 mul/add per block. Document as deviation from CONTEXT-rev-3 if applied.

---

### OQ#3-rev-3 — `juce::Random` thread-safety on the audio thread ✅ RESOLVED — **Lock: per-voice instance, allocation-free `nextFloat()`**

JUCE 8.0.4 source: `/Users/taylorbrook/JUCE/modules/juce_core/maths/juce_Random.cpp`:132–137.

```cpp
float Random::nextFloat() noexcept
{
    auto result = static_cast<float> (static_cast<uint32> (nextInt()))
                  / (static_cast<float> (std::numeric_limits<uint32>::max()) + 1.0f);
    return jmin (result, 1.0f - std::numeric_limits<float>::epsilon());
}
```

**Verified properties:**

- `noexcept` — RT-safe by signature.
- Allocation-free — `nextInt()` is LCG arithmetic on the `int64 seed` member (juce_Random.cpp:97–110); `nextFloat` is one cast + one float division. No `new` / `malloc` / heap touches.
- Per-instance state — each `juce::Random` carries its own `int64 seed` (juce_Random.h:38: `Random::Random(int64 seedValue)`). Per-voice instance is fully independent; no cross-voice mutex / atomic / shared mutable state.

**Per-voice seeding strategy:**

Two viable forms — research-phase locks the simpler O-Bowed precedent.

**Option A (CONTEXT-rev-3 default):** `seed = juce::Time::currentTimeMillis() ^ voiceIndex`
- Pro: time-based seed varies across plugin instances + DAW launches.
- Con: clock granularity on Windows is ~15 ms; voice constructors at plugin instantiation may all read the same millisecond → seed collision probability per instance ≈ 0 (XOR with voiceIndex 0–15 forces distinct seeds within a single plugin instance, but two plugin instances loaded in the same DAW project at the same wall-clock time share the per-voice-index seed). Acceptable but not deterministic.

**Option B (O-Bowed precedent — `BowNoiseGenerator.h:23`):** `seed = static_cast<juce::int64>(voiceIndex * 31337)`
- Pro: deterministic across runs (test reproducibility); no clock dependence; 31337 is prime → spread across full int64 range.
- Con: identical noise pattern across plugin instances and runs (audibly identical voices on different instances of same plugin if all 16 voices are active — practically irrelevant since DAW sessions don't simultaneously load 32+ voices, and the modal bank dominates the audible spectrum).

**Lock: Option B (O-Bowed precedent).** Simpler, deterministic, family-consistent. Set in voice constructor or `prepareToPlay`:

```cpp
// In NoiseExciter::prepare(double sampleRate, int voiceIndex)
rng.setSeed(static_cast<juce::int64>(voiceIndex) * 31337);
```

**Cross-thread call site:** Voice `prepareToPlay` runs on the message thread (or a worker thread before audio starts) — NOT the audio thread. `setSeed` is non-RT but called pre-audio. `nextFloat()` is the only call on the audio thread, and it's RT-safe per above.

---

### OQ#4-rev-3 — Continuous-noise `BASE_NOISE_GAIN` empirical tuning ✅ RESOLVED — **Lock: 0.05f starting point; verify-phase rev-1 ear-tunes within [0.03f, 0.20f]**

**Energy budget calculation** (Phase 2.3 modal bank fed by LP-filtered noise):

Given the Phase 2.2 working tree:
- 16 modes, formant-Gaussian × 1/k roll-off amplitudes (mean per-mode `amp` ≈ 0.4 across the table — peaks near the 475 Hz formant, decays to ~0.1 at high modes)
- Per-mode biquad has unity peak gain by construction (`G = (1−R)·amp`; voice paper at Phase 2.1 RESEARCH §2)
- `1/8` headroom scaler post-summation (Phase 2.2 OQ#5-rev-2)
- 1-pole LP-filtered noise input at cutoff 2 kHz (CONTEXT-rev-3 Q2-rev-3 b2)

**RMS analysis (continuous excitation, steady-state):**

White noise RMS = `1/sqrt(3)` ≈ 0.577 for uniform [-1, 1]. After 1-pole LP at 2 kHz / 48 kHz, RMS attenuated by ~0.6 → input RMS ≈ `0.577 × 0.6 × BASE_NOISE_GAIN × breath`.

Each mode is a narrowband resonator; sustained input RMS gets amplified by the mode's Q-factor (T60 at 2.5 s → Q ≈ π·f·T60 = π·475·2.5 ≈ 3700 for the formant-peak mode). However, only the spectral component near each mode's resonance gets the full amplification — wideband noise input has ≪ 1/Q of its energy in any given mode's bandwidth.

Net: each mode's output RMS ≈ `input RMS × amp × Q × (mode_BW / signal_BW)` ≈ `0.346 × BASE_NOISE_GAIN × breath × 0.4 × 3700 × (0.13 Hz / 2000 Hz)` ≈ `0.346 × BASE_NOISE_GAIN × breath × 0.4 × 0.24` ≈ `0.033 × BASE_NOISE_GAIN × breath`.

Summed across 16 modes (assuming spectral independence between mode bands): RMS ≈ `0.033 × BASE_NOISE_GAIN × breath × sqrt(16)` = `0.13 × BASE_NOISE_GAIN × breath`.

After 1/8 scaler: voice output RMS ≈ `0.0165 × BASE_NOISE_GAIN × breath`.

**Target: voice output RMS ≈ −18 dBFS at default `breath=0.7`** (matches typical sustained-tone "comfortable monitoring level"; corresponds to peak ≈ −12 dBFS for a vibrato-free sustain). Solving:

`10^(−18/20) ≈ 0.126 = 0.0165 × BASE_NOISE_GAIN × 0.7` → `BASE_NOISE_GAIN ≈ 11`.

That's clearly wrong by 2 orders of magnitude — the simple RMS-bandwidth approximation overstates spectral selectivity. **Empirical anchor: O-Bowed `BowNoiseGenerator` uses 0.03f as the gain coefficient on already-Q-selected wideband noise feeding a waveguide.** That's the closest family-precedent number for "bandpass-filtered noise into resonant structure."

**Lock: BASE_NOISE_GAIN = 0.05f** (CONTEXT-rev-3 default), with verify-phase rev-1 ear-tuning bracket **[0.03f, 0.20f]**:
- 0.03f if Phase 2.3 voice sounds noticeably "noisy" (broadband floor leaking through modal Q at low breath)
- 0.10–0.20f if voice sounds too quiet at default breath=0.7 (modal Q is suppressing more energy than the RMS calc suggests)

**Verify-phase rev-1 procedure:**
1. Build with `BASE_NOISE_GAIN = 0.05f`.
2. Hold C3, breath = 0.7, vibrato off, ADSR default. Listen for 5 seconds.
3. A/B against Phase 2.2 reference (the strike()-injected baseline; recordable by temporarily commenting out the noise call). If Phase 2.3 voice is clearly quieter than Phase 2.2 baseline → raise gain to 0.10f. If voice has audible "hissy" character → lower to 0.03f.
4. Document as-shipped value in ARCHITECTURE.md rev-3 backfill.

---

### OQ#5-rev-3 — CC2-takeover window value (250 / 500 / 1000 ms) ✅ RESOLVED — **Lock: 500 ms**

No exact precedent in O-Wind / O-Lyrica / O-Bowed (those plugins use **overlay takeover** — `if (cc > 0) breath = cc; else breath = ui;` — instantaneous switch with no idle-window state machine). CONTEXT-rev-3 introduces a multiplicative-with-takeover semantics that's family-novel.

**Reasoning lock at 500 ms:**

- Breath controllers (e.g., TEControl, TC Helicon Singthing, etc.) emit at 50–200 Hz update rate (every 5–20 ms between events). 500 ms = 25–100 events between gaps → robust against single dropped events but responsive to player intent.
- 250 ms is too short — a player who pauses to inhale (typical breath interval ≈ 2–5 s for human players, but momentary 200–400 ms gaps occur at note transitions) could trigger UI takeover mid-phrase.
- 1000 ms is too long — when a player switches from breath controller to mouse for parameter adjustment, the UI feels sluggish to take over.

**State machine (per voice):**

```cpp
// In BassoonVoice
bool      cc2EverActive       = false;
juce::int64 lastCC2SampleCount = 0;   // sample-count timestamp of last CC2 event
juce::int64 currentSampleCount = 0;   // advanced by numSamples each renderNextBlock

// In controllerMoved:
if (controllerNumber == 2) {
    cc2EverActive = true;
    lastCC2SampleCount = currentSampleCount;
    breathSmoother.setTargetValue(newControllerValue / 127.0f);
}

// In setExpression (called from processor each block):
if (cc2EverActive
    && (currentSampleCount - lastCC2SampleCount) < static_cast<juce::int64>(0.500 * sampleRate))
    return;  // CC2 active — ignore UI breath this block

breathSmoother.setTargetValue(uiBreath);
```

**Lock: 500 ms.** Revisable at verify-phase if Gate 3 item 4 (CC2 from controller produces real-time loudness change) reveals flicker at typical breath-controller rates.

---

### OQ#6-rev-3 — Pre-NE-drain dispatch ordering ✅ RESOLVED — **Lock: tone-dispatch → expression-dispatch → NE-drain → renderNextBlock → output_gain-applyGainRamp**

Phase 2.2 working tree at `plugins/O-Bassoon/Source/PluginProcessor.cpp`:

| Line | Operation |
|---|---|
| 174 | `juce::ScopedNoDenormals noDenormals;` |
| 177 | `buffer.clear();` |
| 179 | `const int numSamples = buffer.getNumSamples();` |
| 185–195 | Tone smoother dispatch (Phase 2.2) |
| 200 | `vst3Extensions.drainAndUpdate();` (Phase 2.1 NE drain) |
| 203 | `synthesiser.renderNextBlock(buffer, midiMessages, 0, numSamples);` |

**Phase 2.3 ordering insertion:**

```cpp
juce::ScopedNoDenormals noDenormals;
buffer.clear();
const int numSamples = buffer.getNumSamples();

// (1) Phase 2.2: tone smoother + dispatch (UNCHANGED)
const float toneTarget   = parameters.getRawParameterValue("tone")->load();
toneSmoother.setTargetValue(toneTarget);
const float toneSmoothed = toneSmoother.skip(juce::jmax(0, numSamples));
if (std::abs(toneSmoothed - lastDispatchedTone) > 0.001f) { /* dispatch */ }

// (2) Phase 2.3 NEW: expression dispatch (BEFORE NE drain)
const float attackMs   = parameters.getRawParameterValue("attack_time")->load();
const float releaseMs  = parameters.getRawParameterValue("release_time")->load();
const float vibRate    = parameters.getRawParameterValue("vibrato_rate")->load();
const float vibDepth   = parameters.getRawParameterValue("vibrato_depth")->load();
const float vibOnsetMs = parameters.getRawParameterValue("vibrato_onset")->load();
const float uiBreath   = parameters.getRawParameterValue("breath")->load();

const bool anyChanged =
       std::abs(attackMs   - lastDispatchedAttackMs)   > 0.001f
    || std::abs(releaseMs  - lastDispatchedReleaseMs)  > 0.001f
    || std::abs(vibRate    - lastDispatchedVibRate)    > 0.001f
    || std::abs(vibDepth   - lastDispatchedVibDepth)   > 0.001f
    || std::abs(vibOnsetMs - lastDispatchedVibOnsetMs) > 0.001f
    || std::abs(uiBreath   - lastDispatchedUiBreath)   > 0.001f;

if (anyChanged) {
    for (int v = 0; v < synthesiser.getNumVoices(); ++v)
        if (auto* bv = dynamic_cast<BassoonVoice*>(synthesiser.getVoice(v)))
            bv->setExpression(attackMs, releaseMs, vibRate, vibDepth, vibOnsetMs, uiBreath);
    lastDispatchedAttackMs   = attackMs;
    lastDispatchedReleaseMs  = releaseMs;
    lastDispatchedVibRate    = vibRate;
    lastDispatchedVibDepth   = vibDepth;
    lastDispatchedVibOnsetMs = vibOnsetMs;
    lastDispatchedUiBreath   = uiBreath;
}

// (3) Phase 2.1: NE drain (UNCHANGED)
vst3Extensions.drainAndUpdate();

// (4) Phase 2.1: render (UNCHANGED)
synthesiser.renderNextBlock(buffer, midiMessages, 0, numSamples);

// (5) Phase 2.3 NEW: output_gain post-summation applyGainRamp
const float outDb = parameters.getRawParameterValue("output_gain")->load();
const float linearGain = juce::Decibels::decibelsToGain(outDb);
const float gainStart  = outputGainSmoother.getCurrentValue();
outputGainSmoother.setTargetValue(linearGain);
const float gainEnd    = outputGainSmoother.skip(juce::jmax(0, numSamples));
buffer.applyGainRamp(0, numSamples, gainStart, gainEnd);
```

**Why this order is correct:**

- **Expression dispatch BEFORE NE drain:** Expression parameters (ADSR, vibrato, breath, output_gain) are pitch-orthogonal — they don't read the NE pending-tuning table. NE drain only updates per-noteId pitch deltas; it doesn't touch ADSR/breath/vibrato state. Order between tone-dispatch and expression-dispatch is irrelevant (both are pitch-orthogonal); placing expression after tone is cosmetic (matches the Phase 2.3 add-only delta).
- **Both dispatches BEFORE renderNextBlock:** Same principle as Phase 2.1 NE-drain-BEFORE-renderNextBlock invariant — voice state (NE pending tuning + tone + ADSR + vibrato + breath) must be fully up-to-date when JUCE iterates `renderVoices` and `startNote` callbacks for any new note-on events processed inside this `renderNextBlock`.
- **Output_gain AFTER renderNextBlock:** `output_gain` is post-summation across all voices. `applyGainRamp` operates on the rendered stereo buffer; running it before voices have rendered would multiply silence (`buffer.clear()` cleared the buffer at line 177).
- **Tone-dispatch and expression-dispatch independence:** Both use `synthesiser.getNumVoices()` + `dynamic_cast<BassoonVoice*>` — no cross-coupling. Aggregate `setExpression` is one virtual call per voice per block (when changed); separate-call alternative would be 6 virtual calls. Single call wins.

**Cross-check against family precedent:**
- O-Lyrica `PluginProcessor.cpp`: NE drain before renderNextBlock, `applyGain(Decibels::decibelsToGain(volumeDb))` post-render at line 961 — same shape, simpler smoothing.
- O-Bowed `PluginProcessor.cpp:412–413`: post-render `applyGain` with raw APVTS read — same shape as O-Lyrica.

---

### OQ#7-rev-3 — Vibrato compose-order with future NE/MPE multiplier ✅ RESOLVED — **Lock: `f_final = (NE_tuned_base) × vibratoMult × pitchBendMult`**

**Compose chain (locked for Phase 2.3 forward, including Phase 2.4 NE addition):**

```cpp
// At Phase 2.3 (NE table is read but ignored by voice):
const float baseFreq    = MidiMessage::getMidiNoteInHertz(midiNote);
const float NE_tuned    = baseFreq;  // Phase 2.4 will replace with NE multiplier
const float vibratoMult = std::pow(2.0f, vibratoCents / 1200.0f);
const float pbMult      = std::pow(2.0f, pitchBendSemitones / 12.0f);
const float f_final     = NE_tuned * vibratoMult * pbMult;
```

**Phase 2.4 will replace** `MidiMessage::getMidiNoteInHertz(midiNote)` with `tuningEngine->getFrequency(midiNote, pendingNETable)` — the NE-tuned base. Vibrato and pitch-bend continue to multiply on top of that.

**Why multiplicative not additive:**
- All three modulations are pitch-multiplicative in pitch-space (cents/semitones), so they compose multiplicatively in frequency-space (Hz × power-of-2).
- Order of multiplication doesn't matter mathematically (commutative); convention in O-Lyrica precedent puts NE → vibrato → pitch-bend (innermost = NE-tuned base, outermost = host pitch wheel).
- This is `f_final = baseFreq × pow(2, (NE_cents + vibratoCents + pitchBendSemitones × 100) / 1200)` — equivalent form, slightly cheaper (one `pow`); JUCE `juce::dsp::FastMathApproximations::pow2` available if the per-block compute matters.

**Document in ARCHITECTURE.md rev-3 backfill** (CONTEXT-rev-3 line 583):

> Phase 2.3+ frequency compose order:
> ```
> f_final = base × NE_tuning × vibrato_LFO × pitch_bend
> ```
> where `base = MidiMessage::getMidiNoteInHertz(midiNote)` (Phase 2.3) or `tuningEngine->getFrequency(midiNote, pendingNETable)` (Phase 2.4+). Vibrato and pitch-bend operate on the NE-tuned base, not the 12-TET base. Multiplicative composition; commutative; per-block `setFundamental` dispatch when `|Δf_final| > 0.1 Hz`.

---

### OQ#8-rev-3 — CC2 normalisation site ✅ RESOLVED — **Lock: divide by 127.0f at `controllerMoved`**

JUCE 8: `juce::SynthesiserVoice::controllerMoved(int controllerNumber, int newControllerValue)` delivers raw 7-bit CC value (0–127). Normalisation site choice:

**Option A: At controllerMoved callback** (O-Wind precedent — `FluteSynthVoice.cpp:227`):
```cpp
void controllerMoved (int controllerNumber, int newValue) override
{
    float normalized = static_cast<float>(newValue) / 127.0f;
    switch (controllerNumber) {
        case 2:   ccBreath = normalized; break;
        // ...
    }
}
```

**Option B: At smoother target site** (delay normalisation):
```cpp
case 2: breathSmoother.setTargetValue(static_cast<float>(newValue) / 127.0f); break;
```

**Lock: Option A** (O-Wind precedent). Reasons:
- All consumers downstream of `ccBreath` see normalised `[0, 1]` floats — single source of truth for the unit semantics.
- Allows unit-test seam at the value level (cc2 = 0.5f → expected breath_voice contribution = 0.5 × ui_breath).
- Negligible cost difference (one cast + one divide either way).

**Locked form (Phase 2.3):**

```cpp
void BassoonVoice::controllerMoved (int controllerNumber, int newControllerValue) override
{
    if (controllerNumber == 2)  // CC2 = MIDI breath controller
    {
        const float cc2Normalised = juce::jlimit(0.0f, 1.0f,
                                                  static_cast<float>(newControllerValue) / 127.0f);
        cc2EverActive       = true;
        lastCC2SampleCount  = currentSampleCount;
        // Multiplicative compose: breath_voice = ui_breath × cc2_normalised
        // Apply by setting breathSmoother target to product (UI breath shadow held in lastUiBreath)
        breathSmoother.setTargetValue(lastUiBreath * cc2Normalised);
    }
}
```

The `juce::jlimit(0.0f, 1.0f, ...)` guard defends against future MIDI 2.0 / 14-bit CC events that might exceed 127.

---

### OQ#9-rev-3 — Vibrato phase reset on startNote ✅ RESOLVED — **Lock: random phase per note (NOT instant-zero)**

**Family precedent — O-Wind `FluteSynthVoice.cpp:114–116`:**

```cpp
// Random initial vibrato phase per note (humanization)
vibratoPhase = juce::Random::getSystemRandom().nextFloat()
               * juce::MathConstants<float>::twoPi;
```

**O-Wind uses `Random::getSystemRandom()` (shared global random)** — that's a JUCE-thread-safe singleton (juce_Random.h:138 declaration; `noexcept`). It's RT-safe per JUCE 8 source guarantees. Per-voice instance is unnecessary for one-shot phase initialization at note-on (which runs on the audio thread but only once per note).

**Why random over instant-zero:**

- Instant-zero (`phase = 0.0f` → `sin(0) = 0`) gives every voice the same initial pitch offset. For polyphonic chord vibrato, all 8 voices at sin(0) start synchronised — the chord vibrato moves up-and-down as one block. Audibly artificial.
- Random phase per voice produces natural staggered chord vibrato — different voices at different points in the LFO cycle when the chord is struck, sound like 8 individual oscillators not one mixed signal.

**Lock (revises CONTEXT-rev-3 Q9 default):**

```cpp
// In Vibrato::reset(), called from BassoonVoice::startNote
void Vibrato::reset() noexcept
{
    phase = juce::Random::getSystemRandom().nextFloat() * juce::MathConstants<float>::twoPi;
    onset.reset(0.0f);
    onset.setTargetValue(1.0f);
}
```

**This is a deviation from CONTEXT-rev-3 Q9 default (instant-zero) → research-phase recommends RANDOM phase per voice.** Documented as discrepancy D2-rev-3.

**Cross-check vs. NoiseExciter seeding (OQ#3-rev-3):** NoiseExciter uses `voiceIndex * 31337` per-voice instance for noise generation. Vibrato uses `Random::getSystemRandom()` for one-shot phase init at note-on. Different patterns, different needs:
- NoiseExciter: continuous per-sample stochastic noise across the note's duration → per-voice instance with deterministic seed avoids cross-voice noise correlation.
- Vibrato: one float at note-on → shared global random is fine; outcome variability is the goal.

---

### OQ#10-rev-3 — 60 s long-tone QUAL-02 protocol ✅ RESOLVED — **Lock: Logic-AU 60 s render + Python `numpy.isfinite` scan**

**Protocol:**

1. **Logic-AU setup:**
   - Open Logic Pro project with O-Bassoon-dev as AU instrument on a track.
   - Set buffer 256 / 48 kHz.
   - Set parameters: `attack_time = 50 ms` (default), `release_time = 200 ms` (default), `breath = 0.7` (default), `vibrato_rate = 5 Hz`, `vibrato_depth = 50 cents`, `vibrato_onset = 0 ms`, `output_gain = 0 dB`.
   - Disable all plugins on master bus (no compression / EQ / limiter masking artifacts).

2. **Capture (60 s sustained note):**
   - Place a single C3 MIDI note from bar 1 beat 1 with duration 60 s (240 quarter notes at 120 bpm).
   - Bounce in place / Export → Audio File → 32-bit float WAV @ 48 kHz, post-fader pre-master.

3. **Numerical scan** (Python, < 30 s execute):
   ```python
   import numpy as np, soundfile as sf
   data, sr = sf.read('bassoon-c3-60s.wav')
   assert np.all(np.isfinite(data)), f"NaN/inf detected at sample {np.where(~np.isfinite(data))[0][0]}"
   # Drift check: 1-second RMS windows over 60 seconds
   rms = np.array([np.sqrt(np.mean(data[i*sr:(i+1)*sr]**2)) for i in range(60)])
   drift_db = 20 * np.log10(rms.max() / rms.min())
   assert drift_db < 1.0, f"Amplitude drift {drift_db:.2f} dB > 1.0 dB threshold"
   print(f"PASS: 60s clean. Drift = {drift_db:.3f} dB peak-to-peak across 1s windows.")
   ```

4. **Logic CPU steady-state check:**
   - After 10 s of playback, screenshot Logic Process bar reading.
   - After 60 s of playback (still playing), screenshot Logic Process bar reading.
   - Compare: ΔCPU should be < 2 % (allows for thermal-throttling drift; >2 % indicates denormal accumulation or memory leak).

5. **Ear-listen pass:**
   - Listen to the bounced 60 s WAV in Logic. Confirm: stable amplitude (no pumping), continuous vibrato (no dropouts), audible end-of-release at note-off.

**Cross-check Phase 2.1 / Phase 2.2 long-tone protocol:**
- Phase 2.1: ≥10 s sustain check via ear + tuner only (RESEARCH-rev-1 §6).
- Phase 2.2: ≥10 s sustain implicit in 8-voice CPU protocol (RESEARCH-rev-2 OQ#9-rev-2).
- Phase 2.3: 60 s + numerical scan + CPU drift check — **graduated rigor matches QUAL-02 final-gate status**.

**Tool dependencies:** Python 3.x with `numpy` + `soundfile` (validated at session start: `[OK] Python 3.14.2`). If `soundfile` not installed, fallback to `scipy.io.wavfile.read` or `wave + struct.unpack`.

**Output:** `plugins/O-Bassoon/research/reference-recordings/phase-2.3-60s-c3-vibrato-breath.wav` (archived) + numerical-scan results documented in VERIFICATION-rev-3.

---

## §2 — Pattern Confirmations (rev-3)

### Vibrato LFO + onset envelope (O-Wind precedent)

**File:** `plugins/O-Wind/Source/FluteSynthVoice.h`:90–103, `FluteSynthVoice.cpp`:114–125, 358–405.

**Pattern:**
- Per-voice phase accumulator (`float vibratoPhase`) + phase increment (`float vibratoPhaseInc`)
- Random initial phase at startNote (`Random::getSystemRandom().nextFloat() * twoPi`)
- Onset ramp via sample counter (`int vibratoOnsetSamples` + `int samplesSinceNoteOn`) — NOT `juce::SmoothedValue`
- Linear ramp computed inline: `onsetGain = samplesSinceNoteOn / vibratoOnsetSamples` (clamped 0–1)
- Vibrato shape: pure sine + 10 % second harmonic (`std::sin(phase) + 0.1f * std::sin(2.0f * phase)`)

**O-Bassoon Phase 2.3 deviations from O-Wind:**

| O-Wind | O-Bassoon Phase 2.3 | Reason |
|---|---|---|
| Counter-based onset (`int samplesSinceNoteOn / vibratoOnsetSamples`) | `juce::SmoothedValue<float, Linear>` onset | SmoothedValue is the JUCE-canonical declick primitive; counter-based works but reinvents linear interpolation |
| Sine + 10 % 2H harmonic shape | Pure sine | ROADMAP DSP-02 spec is "vibrato"; bassoon vibrato is conventionally pure-sine pitch (clarinet/flute have characteristic harmonic content; bassoon less so) |
| Continuous phase across notes (no reset) | Random phase per startNote | O-Wind precedent verified — random per-voice phase prevents synchronised chord vibrato (OQ#9-rev-3) |
| Per-sample phase advance in render loop | Per-block sample (1 sin / block) | CONTEXT-rev-3 Q4-rev-3 b1 — block-rate is psychoacoustically sufficient at ≤10 Hz LFO; per-sample is 256× more `sin` calls/voice with no audible benefit |

**Per-voice setup (Phase 2.3 lock):**
```cpp
class Vibrato {
public:
    void prepare(double sr) noexcept { sampleRate = sr; recomputeIncrement(); }

    void reset() noexcept {
        phase = juce::Random::getSystemRandom().nextFloat() * juce::MathConstants<float>::twoPi;
        onset.reset(0.0f);
        onset.setTargetValue(1.0f);
    }

    void setRateHz(float r) noexcept    { rateHz = r; recomputeIncrement(); }
    void setDepthCents(float d) noexcept{ depthCents = d; }
    void setOnsetMs(float ms) noexcept  {
        onset.reset(sampleRate, juce::jmax(0.0, ms / 1000.0));  // 0 ms → instant target
        onset.setTargetValue(1.0f);
    }

    float getCurrentCents() noexcept {
        const float onsetGain = onset.getNextValue();
        const float c = depthCents * onsetGain * std::sin(phase);
        phase += phaseIncrement;
        if (phase > juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;
        return c;
    }

private:
    double sampleRate = 48000.0;
    float  rateHz = 5.0f, depthCents = 0.0f, phase = 0.0f, phaseIncrement = 0.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> onset { 0.0f };

    void recomputeIncrement() noexcept {
        phaseIncrement = juce::MathConstants<float>::twoPi
                         * rateHz / static_cast<float>(sampleRate);
    }
};
```

---

### CC2 routing via `controllerMoved` (O-Wind precedent)

**File:** `plugins/O-Wind/Source/FluteSynthVoice.cpp`:225–243.

```cpp
void FluteSynthVoice::controllerMoved (int controllerNumber, int newValue)
{
    float normalized = static_cast<float> (newValue) / 127.0f;

    switch (controllerNumber)
    {
        case 2:   ccBreathPressure = normalized; break;  // CC2: Breath
        case 74:  ccEmbouchure     = normalized; break;  // CC74: MPE Y
        case 1:   ccVibratoDepth   = normalized; break;  // CC1: Mod wheel
        default:  break;
    }
}
```

**Phase 2.3 lock:** Same shape, narrowed to CC2-only at Phase 2.3. CC1 (mod wheel → vibrato_depth) and aftertouch routing deferred to v1.1 per Stage 0 D4. The normalisation site (at the callback) is the precedent (OQ#8-rev-3).

**Deviation from O-Wind:** O-Wind uses **overlay takeover** at FluteSynthVoice.cpp:564 (`if (ccBreathPressure > 0.0f) breathPressure = ccBreathPressure;` → CC overrides UI when CC > 0). O-Bassoon Phase 2.3 uses **multiplicative-with-takeover** (CONTEXT-rev-3 Q1-rev-3 b2 + OQ#5-rev-3 state machine). Reason: multiplicative composes velocity (initial UI seed) into ongoing CC behavior naturally; overlay would require special-casing first-CC-event to clobber the velocity seed.

---

### output_gain post-summation (O-Bowed / O-Lyrica precedent)

**File:** `plugins/O-Bowed/Source/PluginProcessor.cpp`:412–413.

```cpp
float outputLevel = parameters.getRawParameterValue ("outputLevel")->load();
buffer.applyGain (juce::Decibels::decibelsToGain (outputLevel));
```

**File:** `plugins/O-Lyrica/Source/PluginProcessor.cpp`:961.

```cpp
buffer.applyGain(juce::Decibels::decibelsToGain(volumeDb));
```

**Phase 2.3 deviation from precedent:** Both O-Bowed and O-Lyrica use **per-block `applyGain` with NO smoother** — single gain step per block. Phase 2.3 adds 30 ms `SmoothedValue<Linear>` + `applyGainRamp` for guaranteed declick under DAW automation (OQ#1-rev-3 lock). Documented as deviation D4-rev-3.

---

### Per-voice noise source (O-Bowed `BowNoiseGenerator` precedent)

**File:** `plugins/O-Bowed/Source/DSP/BowNoiseGenerator.h`:18–54.

```cpp
class BowNoiseGenerator
{
public:
    void prepare (double sampleRate, int voiceIndex) noexcept
    {
        noiseRandom.setSeed (static_cast<juce::int64> (voiceIndex * 31337));
        // ... bandpass coeffs
    }

    float processSample (float bowPressure, float bowSpeed, float noiseAmount) noexcept
    {
        if (noiseAmount < 0.001f) return 0.0f;
        float noise = noiseRandom.nextFloat() * 2.0f - 1.0f;
        float filtered = bandpassFilter.processSample (noise);
        float amplitude = bowPressure * bowSpeed * noiseAmount * 0.03f;
        return filtered * amplitude;
    }

    void reset() noexcept { bandpassFilter.reset(); }

private:
    juce::Random noiseRandom;
    juce::dsp::IIR::Filter<float> bandpassFilter;
};
```

**Phase 2.3 `NoiseExciter` derives from this pattern with three substitutions:**

| O-Bowed BowNoiseGenerator | O-Bassoon NoiseExciter |
|---|---|
| Bandpass filter (3464 Hz, Q=0.87) | 1-pole low-pass @ 2 kHz (CONTEXT-rev-3 Q2-rev-3 b2) |
| Three-input gating (`bowPressure × bowSpeed × noiseAmount`) | Single-input gating (`breath_voice`) |
| Coefficient `0.03f` | `BASE_NOISE_GAIN = 0.05f` (OQ#4-rev-3 starting point) |
| `voiceIndex * 31337` seed | Same — O-Bowed precedent confirmed (OQ#3-rev-3) |
| `juce::dsp::IIR::Filter<float>` (allocates internal coeff vector) | Hand-rolled 1-pole `lpState += lpCoeff × (rng - lpState)` (no allocation, no juce::dsp dependency) |

**Per-voice setup (Phase 2.3 lock):**
```cpp
class NoiseExciter {
public:
    void prepare(double sr, int voiceIndex) noexcept {
        sampleRate = sr;
        rng.setSeed(static_cast<juce::int64>(voiceIndex) * 31337);
        // 1-pole LP coefficient: alpha = 1 - exp(-2π · cutoff / sr)
        constexpr float CUTOFF_HZ = 2000.0f;
        lpCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi
                                    * CUTOFF_HZ / static_cast<float>(sampleRate));
        lpState = 0.0f;
    }

    void reset() noexcept { lpState = 0.0f; }

    float getNextSample(float breathScaled) noexcept {
        const float white = rng.nextFloat() * 2.0f - 1.0f;
        lpState += lpCoeff * (white - lpState);
        return lpState * BASE_NOISE_GAIN * breathScaled;
    }

private:
    static constexpr float BASE_NOISE_GAIN = 0.05f;
    double       sampleRate = 48000.0;
    float        lpCoeff = 0.0f, lpState = 0.0f;
    juce::Random rng;
};
```

---

### Aggregate per-voice setter (Phase 2.2 `setTone` precedent)

**File:** `plugins/O-Bassoon/Source/BassoonVoice.h`:51 (`setTone` declaration), `BassoonVoice.cpp` (impl).

Phase 2.3 extends the Phase 2.2 single-param `setTone(float)` pattern to a 6-param `setExpression(float, float, float, float, float, float)` aggregate. Same shape — processor-side throttled-epsilon dispatch + voice-side application via `dynamic_cast<BassoonVoice*>`.

---

### Pre-flight build verification (`baac74f`)

Working tree starting state confirmed via `ninja O-Bassoon_VST3` from `build/`: **no work to do** — Phase 2.2 binaries already built and installed at `~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3`. Logic-AU baseline check is deferred to plan-phase as a Phase 2.3 task-zero (visual confirmation only — no audit trail required).

---

## §3 — Implementation Skeletons (rev-3)

### `Source/Vibrato.h` (NEW)

```cpp
/*
  ==============================================================================

    Vibrato.h
    Modal Synthesis Bassoon - Per-voice sine LFO vibrato + onset envelope
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.3 deliverable. Multiplicative pitch-cents output composes with
    pitch-bend at the voice level: f_final = base × pow(2, vibratoCents/1200)
    × pow(2, pitchBendSemitones/12). Random initial phase per startNote
    (O-Wind FluteSynthVoice.cpp:114-116 precedent).

  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>

class Vibrato
{
public:
    void prepare (double sr) noexcept;
    void reset () noexcept;                       // call from BassoonVoice::startNote
    void setRateHz (float rateHz) noexcept;
    void setDepthCents (float depthCents) noexcept;
    void setOnsetMs (float onsetMs) noexcept;     // variable-duration ramp
    float getCurrentCents () noexcept;            // advances phase + onset

private:
    void recomputeIncrement () noexcept;

    double sampleRate    = 48000.0;
    float  rateHz        = 5.0f;
    float  depthCents    = 0.0f;
    float  phase         = 0.0f;
    float  phaseIncrement = 0.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> onset { 0.0f };
};
```

### `Source/Vibrato.cpp` (NEW)

```cpp
#include "Vibrato.h"

void Vibrato::prepare (double sr) noexcept
{
    sampleRate = sr;
    recomputeIncrement();
    onset.reset (sampleRate, 0.0);  // initial 0-ms ramp; setOnsetMs sets per-note
    onset.setCurrentAndTargetValue (1.0f);
}

void Vibrato::reset () noexcept
{
    // O-Wind precedent: random phase per note prevents synchronised chord vibrato.
    phase = juce::Random::getSystemRandom().nextFloat()
            * juce::MathConstants<float>::twoPi;
    onset.reset (0.0f);
    onset.setTargetValue (1.0f);
}

void Vibrato::setRateHz (float r) noexcept
{
    rateHz = r;
    recomputeIncrement();
}

void Vibrato::setDepthCents (float d) noexcept
{
    depthCents = d;
}

void Vibrato::setOnsetMs (float ms) noexcept
{
    // 0 ms → instant target (SmoothedValue::reset(sr, 0) → 0 steps)
    onset.reset (sampleRate, juce::jmax (0.0, ms / 1000.0));
    onset.setTargetValue (1.0f);
}

float Vibrato::getCurrentCents () noexcept
{
    const float onsetGain = onset.getNextValue();
    const float cents = depthCents * onsetGain * std::sin (phase);

    phase += phaseIncrement;
    if (phase > juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;

    return cents;
}

void Vibrato::recomputeIncrement () noexcept
{
    phaseIncrement = juce::MathConstants<float>::twoPi
                     * rateHz / static_cast<float> (sampleRate);
}
```

---

### `Source/NoiseExciter.h` (NEW)

```cpp
/*
  ==============================================================================

    NoiseExciter.h
    Modal Synthesis Bassoon - Per-voice continuous filtered-noise excitation
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.3 architectural pivot. 1-pole LP (cutoff 2 kHz) over white noise,
    scaled by breath_voice. Replaces struck-modal sustain with breath-driven
    sustain. Per-voice juce::Random with deterministic seed (O-Bowed
    BowNoiseGenerator.h:23 precedent).

  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>

class NoiseExciter
{
public:
    void prepare (double sr, int voiceIndex) noexcept;
    void reset () noexcept;
    float getNextSample (float breathScaled) noexcept;

private:
    static constexpr float CUTOFF_HZ        = 2000.0f;
    static constexpr float BASE_NOISE_GAIN  = 0.05f;   // OQ#4-rev-3 starting point

    double       sampleRate = 48000.0;
    float        lpCoeff    = 0.0f;
    float        lpState    = 0.0f;
    juce::Random rng;
};
```

### `Source/NoiseExciter.cpp` (NEW)

```cpp
#include "NoiseExciter.h"

void NoiseExciter::prepare (double sr, int voiceIndex) noexcept
{
    sampleRate = sr;
    rng.setSeed (static_cast<juce::int64> (voiceIndex) * 31337);

    // 1-pole LP coefficient: alpha = 1 - exp(-2π·cutoff/sr)
    lpCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                                * CUTOFF_HZ / static_cast<float> (sampleRate));
    lpState = 0.0f;
}

void NoiseExciter::reset () noexcept
{
    lpState = 0.0f;
}

float NoiseExciter::getNextSample (float breathScaled) noexcept
{
    const float white = rng.nextFloat() * 2.0f - 1.0f;
    lpState += lpCoeff * (white - lpState);
    return lpState * BASE_NOISE_GAIN * breathScaled;
}
```

---

### `Source/BassoonVoice.h` (rev-3 deltas to Phase 2.2)

**Add to includes:**
```cpp
#include "Vibrato.h"
#include "NoiseExciter.h"
```

**Add to public API:**
```cpp
// Phase 2.3 aggregate setter (called from PluginProcessor each block, throttled).
void setExpression (float attackMs, float releaseMs,
                    float vibRateHz, float vibDepthCents, float vibOnsetMs,
                    float uiBreath) noexcept;

// Phase 2.3 voice-construction-time setter for per-voice noise seed.
void setVoiceIndex (int idx) noexcept { voiceIndex = idx; }
```

**Add to private members:**
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

---

### `Source/BassoonVoice.cpp` (rev-3 deltas)

**`prepareToPlay` additions** (after Phase 2.2 ADSR setSampleRate + tone init):

```cpp
vibrato.prepare (sampleRate);
noiseExciter.prepare (sampleRate, voiceIndex);
breathSmoother.reset (sampleRate, 0.020);   // 20 ms ramp (CONTEXT-rev-3 line 521)
breathSmoother.setCurrentAndTargetValue (0.7f);
```

**`startNote` additions** (after Phase 2.2 setTone + setFundamental + strike()):

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

**`controllerMoved` (replaces stub):**
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

**`setExpression` (NEW)** — aggregate per-block setter with per-sub-param epsilon:
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

**`renderNextBlock` (replaces Phase 2.2 implementation):**
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

**Note on Phase 2.1 `Exciter` retention:** The `exciter` member stays declared in `BassoonVoice.h` (CONTEXT-rev-3 line 535 — Phase 2.4 re-wires for attack-character morph). The Phase 2.1 `exciter.getNextSample()` call in the per-sample render loop is REMOVED. `Exciter.{h,cpp}` files are NOT modified.

---

### `Source/PluginProcessor.h` (rev-3 deltas)

**Add to private members:**
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

### `Source/PluginProcessor.cpp` (rev-3 deltas)

**Voice construction loop (one-time `setVoiceIndex` wire):**
```cpp
// In constructor, after voice creation loop:
for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (i)))
        bv->setVoiceIndex (i);
```

**`prepareToPlay` additions** (after Phase 2.2 toneSmoother init):
```cpp
outputGainSmoother.reset (sampleRate, 0.030);   // 30 ms ramp
outputGainSmoother.setCurrentAndTargetValue (1.0f);

lastDispatchedAttackMs   = -1.0f;
lastDispatchedReleaseMs  = -1.0f;
lastDispatchedVibRate    = -1.0f;
lastDispatchedVibDepth   = -1.0f;
lastDispatchedVibOnsetMs = -1.0f;
lastDispatchedUiBreath   = -1.0f;
```

**`processBlock` (full rev-3 ordering — see OQ#6-rev-3 for inline form):**
```cpp
void OBassoonAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int numSamples = buffer.getNumSamples();

    // (1) Phase 2.2: tone smoother + dispatch (UNCHANGED)
    const float toneTarget   = parameters.getRawParameterValue ("tone")->load();
    toneSmoother.setTargetValue (toneTarget);
    const float toneSmoothed = toneSmoother.skip (juce::jmax (0, numSamples));
    if (std::abs (toneSmoothed - lastDispatchedTone) > 0.001f)
    {
        for (int v = 0; v < synthesiser.getNumVoices(); ++v)
            if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (v)))
                bv->setTone (toneSmoothed);
        lastDispatchedTone = toneSmoothed;
    }

    // (2) Phase 2.3 NEW: expression dispatch (BEFORE NE drain)
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

    // (3) Phase 2.1: NE drain (UNCHANGED)
    vst3Extensions.drainAndUpdate();

    // (4) Phase 2.1: render (UNCHANGED)
    synthesiser.renderNextBlock (buffer, midiMessages, 0, numSamples);

    // (5) Phase 2.3 NEW: output_gain post-summation applyGainRamp
    const float outDb     = parameters.getRawParameterValue ("output_gain")->load();
    const float linearTgt = juce::Decibels::decibelsToGain (outDb);
    const float gainStart = outputGainSmoother.getCurrentValue();
    outputGainSmoother.setTargetValue (linearTgt);
    const float gainEnd   = outputGainSmoother.skip (juce::jmax (0, numSamples));
    buffer.applyGainRamp (0, numSamples, gainStart, gainEnd);
}
```

---

### `plugins/O-Bassoon/CMakeLists.txt` (rev-3 deltas)

```cmake
target_sources(O-Bassoon PRIVATE
    Source/BassoonSound.h
    Source/ModeBank.h
    Source/ModeBank.cpp
    Source/Exciter.h         # Phase 2.1 — retained for Phase 2.4 attack-character morph
    Source/Exciter.cpp       # NOT called from BassoonVoice render path at Phase 2.3
    Source/Vibrato.h         # NEW Phase 2.3
    Source/Vibrato.cpp       # NEW Phase 2.3
    Source/NoiseExciter.h    # NEW Phase 2.3
    Source/NoiseExciter.cpp  # NEW Phase 2.3
    Source/BassoonVoice.h
    Source/BassoonVoice.cpp
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)
```

Stage 1 build flags + `juce_generate_juce_header` ordering + `JUCE_USE_WIN_WEBVIEW2*` + `NEEDS_WEBVIEW2 TRUE` unchanged.

---

## §4 — Discrepancies (rev-3)

### D1-rev-3 — CC takeover semantics: O-Wind overlay vs. Phase 2.3 multiplicative

**File:** O-Wind `FluteSynthVoice.cpp:564` uses `if (ccBreathPressure > 0.0f) breathPressure = ccBreathPressure;` (overlay — CC overrides UI when CC > 0).

**Phase 2.3** uses `breath_voice = ui_breath × cc2_normalised` with state-machine takeover (CONTEXT-rev-3 Q1-rev-3 b2 + OQ#5-rev-3).

**Resolution:** Multiplicative is the locked Phase 2.3 form. Reason: lets velocity-as-initial-UI-breath survive into CC2-controlled playback (overlay clobbers velocity on first CC2 event); composes cleanly with the 500 ms takeover window. Documented as deliberate deviation from O-Wind precedent. No replan required.

### D2-rev-3 — Vibrato phase reset: CONTEXT default instant-zero vs. research-locked random

**CONTEXT-rev-3 OQ#9 default:** `phase = 0.0f` at startNote (instant-zero).

**Research-phase OQ#9-rev-3 lock:** `phase = Random::getSystemRandom().nextFloat() * twoPi` per O-Wind precedent.

**Resolution:** Random phase wins (overrides CONTEXT default). Reason: prevents synchronised chord vibrato; one-line change in `Vibrato::reset()`. Documented in implementation skeleton (§3 Vibrato.cpp). No replan required.

### D3-rev-3 — `juce::Random` seed: CONTEXT `Time::currentTimeMillis() ^ voiceIndex` vs. research-locked `voiceIndex × 31337`

**CONTEXT-rev-3 line 445:** `seed per-voice from Time::currentTimeMillis() ^ voiceIndex`.

**Research-phase OQ#3-rev-3 lock:** `voiceIndex × 31337` (O-Bowed `BowNoiseGenerator.h:23` precedent).

**Resolution:** O-Bowed pattern wins. Reason: deterministic, family-consistent, simpler (no `Time::currentTimeMillis` cross-thread call). Documented in implementation skeleton (§3 NoiseExciter.cpp). No replan required.

### D4-rev-3 — output_gain smoothing: O-Bowed/O-Lyrica per-block applyGain vs. Phase 2.3 SmoothedValue + applyGainRamp

**O-Bowed/O-Lyrica precedent:** `buffer.applyGain(Decibels::decibelsToGain(level))` — single block-rate gain step, no smoother.

**Phase 2.3 lock (OQ#1-rev-3):** 30 ms `SmoothedValue<Linear>` + `applyGainRamp(0, numSamples, current, smoother.skip(N))`.

**Resolution:** Phase 2.3 retains the smoother + applyGainRamp combination. Reason: explicit declick guarantee under DAW automation; ROADMAP-spec'd 30 ms ramp; cost is negligible (~256 muls/block extra per channel). Documented as deliberate deviation from precedent. No replan required.

### D5-rev-3 — `vibrato_onset` SmoothedValue at zero-duration

**Edge case:** `vibrato_onset_ms = 0` → `SmoothedValue::reset(sampleRate, 0.0)` computes `floor(0 × sr) = 0` steps.

**JUCE 8.0.4 behavior** (juce_SmoothedValue.h:265–280): `reset(sr, 0)` → `reset((int)0)` → countdown set to 0 → next `setTargetValue(1.0f)` and `getNextValue()` returns 1.0 immediately (target). Confirmed allocation-free; no `std::numeric_limits` corner-case fault.

**Resolution:** Behaves correctly — zero-duration onset = instant full-vibrato at note-on. Documented for plan-phase confidence. No code change.

### D6-rev-3 — Phase 2.1 `Exciter` member retained but unused at Phase 2.3

**Working tree state:** `BassoonVoice` has `Exciter exciter;` member from Phase 2.1; `Exciter.{h,cpp}` files retained. Phase 2.3 STOPS calling `exciter.getNextSample()` from `renderNextBlock` but keeps the member declared.

**Compiler warning risk:** GCC/Clang/MSVC may emit `-Wunused-private-field` or equivalent on the `exciter` member if no source line references it. CMakeLists Stage 1 flags include `-Wall -Wextra` but NOT `-Werror=unused-private-field`.

**Resolution:** Keep the member declaration. Add `(void) exciter;` in `prepareToPlay` (next to existing prepares) to silence any unused-member warning OR call `exciter.prepare(sampleRate)` once in prepareToPlay (future-proofs Phase 2.4 re-wire). Phase 2.3 plan-phase locks the silence form. No replan required.

### D7-rev-3 — `juce::Random::getSystemRandom()` on the audio thread (Vibrato::reset)

**Concern:** `Vibrato::reset()` is called from `BassoonVoice::startNote`, which runs on the audio thread. `Random::getSystemRandom()` returns a shared global `juce::Random` — multiple voices' `startNote` calls in the same `renderNextBlock` could race on the shared seed.

**JUCE 8.0.4 source** (`juce_Random.cpp`:82): `Random::getSystemRandom()` returns a `static thread_local Random` instance (verify: look for `JUCE_DECLARE_THREAD_LOCAL` or equivalent in juce_Random.h).

**Verification:**
```cpp
// juce_Random.cpp:82
Random& Random::getSystemRandom() noexcept
{
    static Random sysRand;
    return sysRand;
}
```

The `static Random sysRand` is process-global, NOT thread-local. **Race risk exists** if startNote callbacks across voices fire from different threads. But `juce::Synthesiser::renderVoices` is single-threaded (one voice startNote callback at a time within one renderNextBlock call), so within a single audio thread the calls are serialised. Cross-thread races would only occur if a non-audio thread also calls `getSystemRandom().nextFloat()` concurrently.

**Resolution:** Acceptable on the audio thread for one-shot phase init at startNote. Per-voice instance like NoiseExciter is overkill for a single nextFloat call. Documented as accepted risk. If verify-phase reveals chord-vibrato phase artifacts (unlikely), upgrade to per-voice `juce::Random vibratoRng;` member. No code change at Phase 2.3 plan-phase.

---

## §5 — Outputs and Handoff Checklist (rev-3)

**Phase 2.3 research-phase deliverables:**

| Item | Status | Notes |
|---|---|---|
| 10 Open Questions resolved with JUCE 8.0.4 source citations | ✅ | OQ#1-rev-3 through OQ#10-rev-3 resolved §1 |
| Pattern confirmations against O-Wind / O-Lyrica / O-Bowed | ✅ | §2 — exact line citations for vibrato, controllerMoved, output_gain, noise source, aggregate setter |
| ARCHITECTURE.md continuous-noise compatibility verified | ✅ | Phase 2.2 modal bank `(1−R)·amp` G-normalisation accepts continuous bounded inputs (1-pole LP at 2 kHz × BASE_NOISE_GAIN × breath ≤ 1 stays well within unity) |
| Pre-flight `O-Bassoon_VST3` from `baac74f` | ✅ | `ninja: no work to do.` — Phase 2.2 baseline VST3 already installed |
| Implementation skeletons for Vibrato + NoiseExciter + BassoonVoice rev-3 + PluginProcessor rev-3 + CMakeLists | ✅ | §3 — verbatim-consumable by plan-phase |
| Discrepancies catalogued | ✅ | §4 — D1-rev-3 through D7-rev-3, all non-blocking |

**Handoff to plan-phase:**

- Plan-phase consumes RESEARCH-rev-3 §3 implementation skeletons verbatim.
- Plan-phase task breakdown should mirror Phase 2.2 PLAN structure (single-Wave, ~9 tasks): (1) Vibrato.{h,cpp} create, (2) NoiseExciter.{h,cpp} create, (3) BassoonVoice.{h,cpp} rev-3 deltas, (4) PluginProcessor.{h,cpp} rev-3 deltas (incl. setVoiceIndex wire), (5) CMakeLists target_sources extend, (6) ARCHITECTURE.md rev-3 backfill (continuous-noise spec + breath state machine + vibrato compose order), (7) build + install + 10 static-check grep gates (RT-safety, NE drain ordering, expression dispatch ordering, applyGainRamp form, mode-bank cadence, scaler retention, throttle epsilon, DSP-07, auval, pluginval-5), (8) manual Gate 3 verification (10-item checklist), (9) atomic commit `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`.
- Iteration ceiling: rev-3 (CONTEXT-rev-3 Q4-rev-3 b2). Inline at verify-phase.
- Atomic commit lands: `Source/{Vibrato,NoiseExciter}.{h,cpp}` (NEW) + `Source/{BassoonVoice,PluginProcessor}.{h,cpp}` (MOD) + `CMakeLists.txt` (MOD) + `research/ARCHITECTURE.md` rev-3 backfill + 5 planning artefacts (CONTEXT-rev-3 / RESEARCH-rev-3 / PLAN-rev-3 / SUMMARY-rev-3 / VERIFICATION-rev-3) + STATUS update + REQUIREMENTS update on Gate 3 PASS only.

**Static-check grep gates (locked for plan-phase consumption):**

| # | Pattern | Files | Expected |
|---|---|---|---|
| 1 | `\bnew\b\|make_unique\|make_shared\|push_back\|resize\|malloc` | `Source/Vibrato.cpp`, `Source/NoiseExciter.cpp`, `Source/BassoonVoice.cpp`, `Source/PluginProcessor.cpp` (within `processBlock` and `renderNextBlock`) | 0 hits (RT-safety) |
| 2 | `vst3Extensions.drainAndUpdate\(\)` | `Source/PluginProcessor.cpp` | 1 hit, BEFORE `synthesiser.renderNextBlock` |
| 3 | `setExpression\(.*\)` dispatch site | `Source/PluginProcessor.cpp` | 1 hit, in processBlock prologue (after tone dispatch, before NE drain) |
| 4 | `applyGainRamp\(0, numSamples` | `Source/PluginProcessor.cpp` | 1 hit, AFTER `synthesiser.renderNextBlock` |
| 5 | `modeBank.setFundamental` | `Source/BassoonVoice.cpp` | ≥ 2 hits (startNote + per-block vibrato compose) |
| 6 | `1.0f / 8.0f\|0.125f` (Phase 2.2 1/8 scaler retention) | `Source/ModeBank.cpp` | ≥ 1 hit (unchanged from Phase 2.2) |
| 7 | `0.001f` (epsilon throttle constant) | `Source/PluginProcessor.cpp` | ≥ 6 hits (one per expression sub-param) |
| 8 | `O-Reed\|OReed` (DSP-07 anti-dependency) | `plugins/O-Bassoon/**` | 0 hits |
| 9 | `auval -v aumu OBsn OuDv` | n/a | exit 0, "AU VALIDATION SUCCEEDED" |
| 10 | `pluginval --strictness-level 5` | n/a | exit 0 |

---

## Audit Trail (rev-3 addendum)

**rev-3 (this addendum, 2026-04-28):** Phase 2.3 research phase. All 10 CONTEXT-rev-3 open questions resolved with JUCE 8.0.4 source-line citations and family-precedent confirmation. 7 discrepancies surfaced (D1-rev-3 through D7-rev-3); none block planning. Key locks:

- **OQ#1-rev-3:** `buffer.applyGainRamp(0, numSamples, smoother.getCurrentValue(), smoother.skip(numSamples))` is the canonical declick-safe idiom (juce_AudioSampleBuffer.h:736–769 + juce_SmoothedValue.h:330–342).
- **OQ#2-rev-3:** Block-rate `juce::ADSR::setParameters` with epsilon throttle is sufficient — no internal smoother needed (juce_ADSR.h:92–99 confirms `recalculateRates` preserves `envelopeVal`; mid-envelope rate change produces sub-perceptual slope inflection, not click).
- **OQ#3-rev-3:** Per-voice `juce::Random` with `voiceIndex × 31337` seed (O-Bowed `BowNoiseGenerator.h:23` precedent). `nextFloat()` is allocation-free, lock-free per-instance (juce_Random.cpp:132–137).
- **OQ#4-rev-3:** `BASE_NOISE_GAIN = 0.05f` starting point; verify-phase rev-1 ear-tunes within bracket [0.03f, 0.20f]. Empirical anchor: O-Bowed BowNoiseGenerator uses 0.03f for filtered noise into resonant structure.
- **OQ#5-rev-3:** CC2-takeover window locked at **500 ms**. State machine: `cc2EverActive` + `lastCC2SampleCount` per voice; sample-count delta vs. `0.500 × sampleRate` threshold.
- **OQ#6-rev-3:** Dispatch ordering locked: tone-dispatch → expression-dispatch (NEW) → NE-drain → renderNextBlock → output_gain-applyGainRamp (NEW). Both Phase 2.3 dispatches sit BEFORE NE drain (pitch-orthogonal).
- **OQ#7-rev-3:** Frequency compose chain `f_final = (NE-tuned base) × vibratoMult × pitchBendMult`. Vibrato + pitch-bend operate on NE-tuned base, not 12-TET base. Documented in ARCHITECTURE.md rev-3 backfill.
- **OQ#8-rev-3:** CC2 normalisation site is `controllerMoved` callback (O-Wind `FluteSynthVoice.cpp:227` precedent).
- **OQ#9-rev-3:** Vibrato phase reset — **random per startNote** (O-Wind `FluteSynthVoice.cpp:114–116` precedent), NOT instant-zero. Overrides CONTEXT-rev-3 default.
- **OQ#10-rev-3:** 60 s long-tone QUAL-02 protocol locked — Logic-AU bounce + Python `numpy.isfinite` scan + 1-second-window RMS drift check + Logic Process bar steady-state CPU comparison.

Implementation skeletons for `Source/Vibrato.{h,cpp}` (NEW), `Source/NoiseExciter.{h,cpp}` (NEW), `Source/BassoonVoice.{h,cpp}` (rev-3 deltas), `Source/PluginProcessor.{h,cpp}` (rev-3 deltas), `CMakeLists.txt` (rev-3 deltas) prepared for plan-phase verbatim consumption (§3).

**Inherited verbatim from rev-1 / rev-2 (not re-litigated):**
- `juce::SmoothedValue<float, Linear>::skip(jmax(0, numSamples))` block-rate idiom
- `juce::ScopedNoDenormals` at processBlock entry
- `juce::ADSR::setSampleRate` BEFORE `setParameters` ordering
- Per-sample render-loop ordering (`excitation → modeBank → adsr → addSample`)
- 1/8 scaler retention (Phase 2.2 OQ#5-rev-2)
- NE drain BEFORE renderNextBlock invariant
- Logic Pro AU as primary listening DAW
- Throttled-epsilon dispatch (0.001f) at processor scope
- Mode-bank coefficient cadence: note-on + pitch-bend + tone > epsilon (Phase 2.3 ADDS |Δf_final| > 0.1 Hz)

**New in rev-3:**
- `applyGainRamp(0, numSamples, current, skip(N))` declick-safe idiom locked
- Block-rate ADSR `setParameters` with epsilon throttle (no internal smoother)
- Per-voice `juce::Random` with O-Bowed `voiceIndex × 31337` seed
- Continuous-noise excitation: 1-pole LP @ 2 kHz, BASE_NOISE_GAIN 0.05f, breath-scaled
- Per-voice sine LFO Vibrato class (random initial phase, variable-duration onset SmoothedValue, multiplicative cents output)
- CC2-takeover state machine (500 ms idle window, sample-count-based)
- Aggregate `setExpression(...)` per-voice setter (6 floats, sub-param epsilon throttling)
- Frequency compose chain `f_final = NE-tuned × vibratoMult × pitchBendMult` (block-rate setFundamental dispatch when |Δf| > 0.1 Hz)
- 60 s long-tone QUAL-02 protocol (Python numerical scan + Logic CPU drift check)
- Discrepancy register D1-rev-3 through D7-rev-3 (CC takeover semantics, vibrato phase, Random seed, output_gain smoothing, vibrato_onset zero-duration, Phase 2.1 Exciter retention, getSystemRandom thread-safety)

---

# rev-4 — Phase 2.4 Research (2026-04-29)

**Phase 2.4 — Voice Manager + Attack Character + Note Expression Integration.** Closes the 4 remaining DSP requirements: FUNC-02 (polyphony 1-16 cap), FUNC-05 (voice stealing), DSP-05 (attack-character morph), DSP-06 (NE per-voice + MPE per-channel + TuningEngine `getFrequency`). Plus QUAL-02 60 s gate and PERF-02 8-voice CPU final under enforced cap.

## §1 — Open Questions Resolved (rev-4)

### OQ#1-rev-4 — `juce::Synthesiser` `findFreeVoice` override pattern ✅ RESOLVED — **Lock: subclass + override `findFreeVoice` only, delegate to base `findVoiceToSteal`**

**JUCE 8.0.4 source** (`/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.{h,cpp}`):

| Symbol | Location | Visibility | Signature |
|--------|----------|------------|-----------|
| `findFreeVoice` | `juce_Synthesiser.h:600-603` | `protected: virtual` | `SynthesiserVoice* findFreeVoice (SynthesiserSound*, int channel, int note, bool stealIfNoneAvailable) const` |
| `findVoiceToSteal` | `juce_Synthesiser.h:610-612` | `protected: virtual` | `SynthesiserVoice* findVoiceToSteal (SynthesiserSound*, int channel, int note) const` |
| `setNoteStealingEnabled` | `juce_Synthesiser.h:381` | `public:` | `void setNoteStealingEnabled (bool)` — default `true` (line 640: `bool shouldStealNotes = true;`) |
| `voices` | `juce_Synthesiser.h:577` | `protected:` | `OwnedArray<SynthesiserVoice> voices` (subclass may iterate directly) |
| `getNumVoices()` | `juce_Synthesiser.h:336` | `public: noexcept` | `int getNumVoices() const noexcept` |
| `getVoice(int)` | `juce_Synthesiser.h:339` | `public:` | `SynthesiserVoice* getVoice (int index) const` |
| `isVoiceActive()` | `juce_Synthesiser.h:160` | `public: virtual` | `virtual bool isVoiceActive() const` (declared on `SynthesiserVoice`) |

**Default `findFreeVoice` body** (`juce_Synthesiser.cpp:509-523`):
```cpp
SynthesiserVoice* Synthesiser::findFreeVoice (SynthesiserSound* soundToPlay,
                                              int midiChannel, int midiNoteNumber,
                                              const bool stealIfNoneAvailable) const
{
    const ScopedLock sl (lock);
    for (auto* voice : voices)
        if ((! voice->isVoiceActive()) && voice->canPlaySound (soundToPlay))
            return voice;
    if (stealIfNoneAvailable)
        return findVoiceToSteal (soundToPlay, midiChannel, midiNoteNumber);
    return nullptr;
}
```

**Default `findVoiceToSteal` heuristic** (`juce_Synthesiser.cpp:525-`):
1. Same-pitch oldest match (line 582-584).
2. **Released-tail oldest** (line 587-589): `voice->isPlayingButReleased()` — voices with `isVoiceActive() && !(isKeyDown() || isSostenutoPedalDown() || isSustainPedalDown())`.
3. No-finger oldest (line 591-594): `! voice->isKeyDown()`.
4. Absolute oldest (subsequent fallthroughs).
5. Protects extremal pitches: lowest-and-highest sustained notes are reserved when possible.

**Lock — `BassoonSynthesiser` override pattern**:
```cpp
class BassoonSynthesiser : public juce::Synthesiser
{
public:
    BassoonSynthesiser() { setNoteStealingEnabled (true); /* explicit, JUCE default = true */ }

    void setActiveVoiceCap (int cap) noexcept
    {
        activeVoiceCap = juce::jlimit (1, 16, cap);
    }

protected:
    juce::SynthesiserVoice* findFreeVoice (juce::SynthesiserSound* sound,
                                           int channel, int noteNumber,
                                           bool stealIfNoneAvailable) const override
    {
        // Count active voices (allocation-free).
        int active = 0;
        const int n = getNumVoices();
        for (int i = 0; i < n; ++i)
            if (getVoice (i)->isVoiceActive())
                ++active;

        if (active < activeVoiceCap)
            return juce::Synthesiser::findFreeVoice (sound, channel, noteNumber, stealIfNoneAvailable);

        // At/over cap → steal if allowed; else null.
        return stealIfNoneAvailable
            ? findVoiceToSteal (sound, channel, noteNumber)
            : nullptr;
    }

private:
    int activeVoiceCap = 16;  // pre-allocated 16-voice pool from Stage 1
};
```

Notes:
- Override is `const` because base virtual is `const`.
- Use public `getNumVoices()` + `getVoice(int)` rather than touching protected `voices` array — same effect, no friend-of-protected-base concerns.
- `setNoteStealingEnabled(true)` is explicit-for-clarity; JUCE default is already `true`.
- DO NOT override `findVoiceToSteal` — JUCE default already implements release-tail-first semantics (locked Q2-rev-4 batch 2).

### OQ#2-rev-4 — `voice_count` APVTS read site ✅ RESOLVED — **Lock: processBlock prologue head, integer-comparison throttle**

**JUCE API** (`juce_AudioProcessorValueTreeState.h:340-346`):
```cpp
std::atomic<float>* getRawParameterValue (StringRef parameterID) const noexcept;
```
Returns `std::atomic<float>*` regardless of parameter type. For `AudioParameterInt`, the float holds the integer value (e.g., 8.0f for voice_count=8); cast back via `static_cast<int>`.

**Existing in-tree definition** (`PluginProcessor.cpp:92-96`):
```cpp
layout.add (std::make_unique<juce::AudioParameterInt> (
    juce::ParameterID { "voice_count", 1 },
    "Voice Count",
    1, 16, 8));   // min, max, default
```

**Existing processBlock prologue ordering** (`PluginProcessor.cpp:187-262`): tone-dispatch (201) → expression-dispatch (217) → NE-drain (249) → renderNextBlock (252) → output_gain applyGainRamp (262).

**Lock — voice_count snapshot site:** Insert at the very head of `processBlock`, BEFORE tone-dispatch. Snapshot via integer-comparison throttle:

```cpp
// Phase 2.4: voice_count snapshot at processBlock prologue head.
// Applies on next note-on (already-active voices unaffected) per ROADMAP.
const int requestedVoices = static_cast<int> (
    parameters.getRawParameterValue ("voice_count")->load());

if (requestedVoices != lastDispatchedVoiceCount)
{
    synthesiser.setActiveVoiceCap (requestedVoices);
    lastDispatchedVoiceCount = requestedVoices;
}
```

Member added to `OBassoonAudioProcessor`: `int lastDispatchedVoiceCount = -1;` (forces first-block dispatch).

Why the head, not after expression dispatch:
- Symmetric with tone+expression dispatch (all happen before renderNextBlock).
- Prevents a race where a note-on arrives in the SAME block that voice_count changed: snapshot fires first, cap applies to that note-on.
- Integer-comparison is exact (no float epsilon needed).

### OQ#3-rev-4 — Soft vs Tongued shape arrays ✅ RESOLVED — **Lock: existing 5 ms half-sine × exp stays as `softShape`; new `tonguedShape` = exp-decay × white noise, ~7.5 ms**

**Existing Phase 2.1 Exciter** (`Source/Exciter.{h,cpp}`):
- `MAX_ONSET_SAMPLES = 1024` (covers 5 ms @ 96 kHz with headroom)
- `DURATION_MS = 5.0f`, `TAU_MS = 1.5f`
- Shape: `window = sin(π·i/N)` × `decay = exp(-t / 1.5e-3)`
- Peak-normalised at `prepare()` end.
- Half-sine windowing acts as a natural low-pass envelope (peak energy ~200 Hz for 5 ms half-sine).

**ROADMAP Phase 2.4 spec**:
- `softShape`: "30-50 ms half-sine impulse, low amplitude, low-passed at ~600 Hz"
- `tonguedShape`: "5-10 ms exponentially decaying noise burst, full bandwidth, higher peak"

**Decision options**:
- (a) Treat existing 5 ms half-sine × exp as `tonguedShape`-equivalent; generate new long+lowpassed `softShape`.
- (b) Treat existing 5 ms half-sine × exp as `softShape` (shorter/quieter than ROADMAP target but in the right spectral neighbourhood — half-sine at 5 ms peaks ~100-200 Hz); generate new `tonguedShape` with noise + 7.5 ms exp-decay.

**Lock — option (b)**: keep existing 5 ms half-sine × exp as `softShape`, generate new `tonguedShape`. Rationale:
- Existing array is already peak-normalised and verified click-free at Phase 2.1 verify.
- The "30-50 ms half-pad" target in ROADMAP is an aesthetic ideal; the locked decision is **ear-only A/B at v1.0** (Q3-rev-4 batch 1 ear-only — no spectral A/B). The 5 ms shape sounds soft enough relative to the noise-burst tongued shape because its peak energy is concentrated in the 100-300 Hz band by the half-sine window.
- Migration cost is minimal: one new array, one new generation pass at `prepare()`, one new `mix()` call in `getNextSample()`.
- If verify-phase ear-judgment finds the contrast insufficient, rev-2 inline iteration can extend `softShape` to 30 ms with a low-pass filter — bounded scope.

**Lock — `tonguedShape` generation** (in `Exciter::prepare()`):
```cpp
const int tonguedSamples = juce::jmin (MAX_ONSET_SAMPLES,
                                        static_cast<int> (sampleRate * 7.5e-3));   // 7.5 ms
juce::Random rng (12345);   // deterministic seed; per-Exciter instance
for (int i = 0; i < tonguedSamples; ++i)
{
    const float noise = rng.nextFloat() * 2.0f - 1.0f;          // [-1, 1]
    const float decay = std::exp (-static_cast<float> (i) /
                                  static_cast<float> (tonguedSamples) * 4.0f);   // 4 time-constants over the window
    tonguedShape[i] = noise * decay;
}
// Peak-normalise to ±1.0
float peak = 0.0f;
for (int i = 0; i < tonguedSamples; ++i)
    peak = std::max (peak, std::abs (tonguedShape[i]));
if (peak > 1e-6f)
    for (int i = 0; i < tonguedSamples; ++i)
        tonguedShape[i] /= peak;
// Pad zeros for samples beyond tonguedSamples (in-class zero-init handles this).
```

Per-Exciter `juce::Random` instance (allocation-free `nextFloat`); deterministic seed (`12345`) avoids per-build variance for verify reproducibility. (Per-voice noise diversity is irrelevant during the 7.5 ms onset — too short to perceive a difference; if needed, swap to `voiceIndex × 31337` seed at PluginProcessor::setVoiceIndex time.)

**Onset window length** = `softSamples` (5 ms = 240 samples @ 48 kHz, 480 @ 96 kHz). `tonguedSamples` (7.5 ms) is shorter than `MAX_ONSET_SAMPLES = 1024`, fits within the buffer. After `onsetIdx >= softSamples`, `getNextSample()` returns 0 — both shapes auto-zero. Pad `tonguedShape` zeros for `i >= tonguedSamples` so `mix(softShape[i], tonguedShape[i], char)` at i=300 (between 240 and softSamples) does not blow up; in-class `std::array<float, MAX_ONSET_SAMPLES>` zero-initialization handles this cleanly.

### OQ#4-rev-4 — Velocity bias magnitude ✅ RESOLVED — **Lock: 0.3 (CONTEXT default)**

**Family precedent** (Ouaricon plugins with velocity → expression coupling):

| Plugin | Source | Coupling | Range |
|--------|--------|----------|-------|
| O-Lyrica `HarpSynthVoice.cpp:181` | `brightness *= juce::jmap(velocity, 0.0f, 1.0f, 0.85f, 1.10f);` | velocity → brightness multiplier | ±12.5 % around 1.0 |
| O-Wind `FluteSynthVoice.cpp:136` | `float settleMs = juce::jmap (velocity, 0.0f, 1.0f, 100.0f, 50.0f);` | velocity → attack settle time | ±33 % (vel 0 = 100 ms; vel 1 = 50 ms) |

**CONTEXT-rev-4 derived formula**: `effective = clamp(attackChar + (velocity - 0.5f) * 0.3f, 0.0f, 1.0f)`. Velocity range maps to ±0.15 attack_character bias → ±15 % of the [0,1] parameter range, centered at velocity=0.5.

**Lock — 0.3 magnitude** (default from CONTEXT):
- Slightly stronger coupling than O-Lyrica brightness (15 % vs 12.5 %) — appropriate because attack-character is the **primary** articulation parameter for a wind instrument; brightness is a secondary timbral parameter.
- Weaker than O-Wind's settle-time bias (15 % vs 33 %) — appropriate because attack_character morph is non-linear (mix of two arrays), whereas O-Wind's settle is a linear time scaling.
- Verify-phase ear-tunes: if velocity coupling feels too subtle, raise to 0.4 (±20 %); too aggressive, drop to 0.2 (±10 %). In-cycle iteration acceptable per rev-3 ceiling.

### OQ#5-rev-4 — `TuningEngine::getFrequency` signature + bit-identity ✅ RESOLVED — **Lock: `double getFrequency(int midiNote, int midiChannel = 0)`, global namespace, bit-identical to MidiMessage at default**

**Source** (`/Users/taylorbrook/Dev/VST-development/modules/tuning/scala-tuning-engine/cpp/TuningEngine.h:243`):
```cpp
double getFrequency(int midiNote, int midiChannel = 0);
```

**Namespace**: global (`class TuningEngine` at `TuningEngine.h:34`, no enclosing namespace). Stage 1 D2 confirmed.

**Default-construction state** (`TuningEngine.cpp:57-76`):
- `currentMode = Mode::TwelveTET` (header line 299)
- `a4Frequency = 440.0` (header line 291)
- `octaveStretch = 1.0f` (header line 293)
- Constructor calls `rebuildFrequencyTable()` immediately — `frequencyTable[midiNote]` populated before any `getFrequency` call.

**Internal `calculate12TETFrequency`** (`TuningEngine.cpp:770-775`):
```cpp
const double semitonesFromA4 = static_cast<double>(midiNote - 69);
const double stretchedSemitones = semitonesFromA4 * static_cast<double>(octaveStretch);   // ×1.0 default
return a4Frequency * std::pow(2.0, stretchedSemitones / 12.0);
```

**`getFrequency` body** (`TuningEngine.cpp:714-728`):
```cpp
double getFrequency(int midiNote, int midiChannel)
{
    juce::ignoreUnused(midiChannel);
    midiNote = juce::jlimit(0, 127, midiNote);
    double baseFreq = frequencyTable[midiNote].load(std::memory_order_relaxed);
    float bendAmount = notePitchBends[midiNote].load(std::memory_order_relaxed);
    if (bendAmount >= -1.0f && bendAmount <= 1.0f)
        return applyPitchBend(baseFreq, bendAmount);
    return baseFreq;
}
```

**Bit-identity check vs `juce::MidiMessage::getMidiNoteInHertz`** at default 12-TET A=440, no per-note pitch-bend:
- TuningEngine: `440.0 * pow(2.0, (midiNote - 69) * 1.0 / 12.0)` (with `1.0` from `octaveStretch` cast)
- MidiMessage: `440.0 * pow(2.0, (midiNote - 69) / 12.0)`
- The intermediate `* static_cast<double>(1.0f)` produces a double `1.0` that multiplies `semitonesFromA4` to produce a bit-identical product; subsequent `pow(2.0, x / 12.0) * 440.0` is identical.
- **Result**: bit-identical at default 12-TET. Phase 2.4 swap is regression-clean.

**Thread-safety on audio thread**:
- `frequencyTable[i]` is `std::atomic<double>::load(std::memory_order_relaxed)` — lock-free, RT-safe.
- `notePitchBends[i]` is `std::atomic<float>::load(std::memory_order_relaxed)` — lock-free, RT-safe.
- `juce::jlimit` is a templated clamp — no allocation.
- ✅ Confirmed RT-safe for Phase 2.4 `BassoonVoice::startNote` call site.

**Cast at call site**: `currentFrequencyBase` is `float`. Wrap with `static_cast<float>` after the compose chain (preserves precision through the compose, narrows once).

### OQ#6-rev-4 — `applyPendingTuning` API + compose order at startNote ✅ RESOLVED — **Lock: `Ouaricon::NoteExpression::applyPendingTuning`, double, multiplicative compose AFTER TuningEngine**

**Source** (`/Users/taylorbrook/Dev/VST-development/modules/tuning/note-expression/cpp/NoteExpression.h:66-79`):
```cpp
inline double applyPendingTuning (PendingTuningTable& table,
                                  int                 midiNoteNumber,
                                  double              currentFrequency)
{
    if (midiNoteNumber < 0 || midiNoteNumber >= 128)
        return currentFrequency;

    const double semis = table[(size_t) midiNoteNumber]
                             .exchange (0.0, std::memory_order_acq_rel);
    if (semis == 0.0)
        return currentFrequency;

    return currentFrequency * std::pow (2.0, semis / 12.0);
}
```

- Inline header function — no link cost.
- `exchange(0.0)`: consumes the delta (resets to 0.0 atomically) — guarantees retriggered notes at the same MIDI pitch in a later block don't inherit a stale offset. Critical correctness invariant.
- Multiplicative: `currentFrequency * pow(2, semis/12)` composes with whatever the caller passed.
- Returns `double`; voice's `currentFrequencyBase` is `float` — cast at call site.

**O-Lyrica precedent** (`Source/HarpSynthVoice.cpp:113-147`) — **canonical compose order**:
```cpp
// 1. TuningEngine first
if (tuningEngine != nullptr)
    currentFrequency = tuningEngine->getFrequency(midiNoteNumber);
else
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

// 2. (Optional humanize — O-Lyrica only; O-Bassoon skips)

// 3. NE second
if (pendingTuningSource != nullptr)
    currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
        *pendingTuningSource, midiNoteNumber, currentFrequency);
```

**Lock — `BassoonVoice::startNote` Phase 2.4 compose chain** (replaces line 59):
```cpp
// Phase 2.4: TuningEngine + Note Expression compose chain (O-Lyrica precedent).
double f_double = (tuningEngine != nullptr)
    ? tuningEngine->getFrequency (midiNoteNumber)
    : juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

if (pendingTuningSource != nullptr)
    f_double = Ouaricon::NoteExpression::applyPendingTuning (
        *pendingTuningSource, midiNoteNumber, f_double);

currentFrequencyBase = static_cast<float> (f_double);
```

Both pointer null-guards retained as defensive (TuningEngine is always wired by PluginProcessor at construction, but the guard mirrors O-Lyrica precedent and keeps the voice safe if used standalone in tests).

Vibrato + pitch-bend compose chain (per-block `f_final` recompute, line 215) is **unchanged** — operates on `currentFrequencyBase` which is now NE-tuned.

### OQ#7-rev-4 — MPE per-channel pitch-bend routing ✅ RESOLVED — **Lock: no new code; `juce::Synthesiser::handlePitchWheel` routes per-channel automatically**

**Source** (`/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp:403-410`):
```cpp
void Synthesiser::handlePitchWheel (const int midiChannel, const int wheelValue)
{
    const ScopedLock sl (lock);
    for (auto* voice : voices)
        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->pitchWheelMoved (wheelValue);
}
```

- Iterates all voices.
- Calls `voice->pitchWheelMoved(wheelValue)` ONLY if voice is playing on the matching channel (or `midiChannel <= 0` for global).
- `voice->isPlayingChannel(midiChannel)` returns `currentPlayingMidiChannel == midiChannel` (`juce_Synthesiser.cpp:45-47`).
- `currentPlayingMidiChannel` is set on `startVoice` (`juce_Synthesiser.cpp:341`).

**MPE behavior**:
- MPE controllers send each note on its own channel (typically 2-16 with global manager on channel 1).
- Pitch-bend on channel N → only voices currently playing on channel N receive `pitchWheelMoved`.
- Phase 2.1 `BassoonVoice::pitchWheelMoved` (already in tree at `BassoonVoice.cpp:111-125`) recomputes `pitchBendSemitones`; mode-bank picks up the new value via the per-block `f_final` compose chain (or the immediate `setFundamental` call at line 123 when `currentFrequencyBase > 0`).
- ✅ **No Phase 2.4 code changes required for MPE pitch-bend routing.**

**Verification path**: empirical DAW test with MPE-enabled controller (Bitwig + LinnStrument / ROLI Seaboard, or Logic Pro 11+ with MPE-enabled track + MPE controller). Per-note pitch-bend should retune individual voices independently. If only the most-recent voice retunes, `currentPlayingMidiChannel` was overwritten — but JUCE startVoice (line 341) sets per-voice channel correctly.

**Discrepancy w/ Stage 0 D3** (`juce::Synthesiser` not `MPESynthesiser`): D3 lock is correct — `juce::Synthesiser` already routes per-channel pitch-bend correctly in MPE mode. `MPESynthesiser` is for MPE-aware DSP semantics (pressure, slide, lift) which O-Bassoon does not need at v1.0 (CC1/aftertouch deferred to v1.1).

### OQ#8-rev-4 — NoiseExciter additive vs muted during onset window ✅ RESOLVED — **Lock: additive composition**

**Pattern**:
```cpp
// BassoonVoice::renderNextBlock per-sample inner loop (Phase 2.4 delta):
const float breath      = breathSmoother.getNextValue();
const float noiseSample = noiseExciter.getNextSample (breath);
const float exSample    = exciter.getNextSample();           // 0 after onset window
const float excitation  = noiseSample + exSample;
float voice             = modeBank.processSample (excitation);
voice                  *= adsr.getNextSample();
outputBuffer.addSample (0, startSample + i, voice);
outputBuffer.addSample (1, startSample + i, voice);
```

**Amplitude analysis**:
- `noiseExciter` peak: `BASE_NOISE_GAIN × breath ≈ 0.05 × velocity` (Phase 2.3 NoiseExciter).
- `exciter` peak: 1.0 (peak-normalised in `prepare()`).
- During onset window (~5-7.5 ms): Exciter dominates by ~20×; NoiseExciter contributes a quiet noise floor underneath ("air column hiss" texture).
- After onset window: Exciter returns 0; only NoiseExciter sustains.

**Why additive (not muted-during-onset)**:
- Air-column hiss texture under the attack adds realism (woodwind physics: the air-column noise starts immediately at note-on and continues, regardless of articulation type).
- Muting NoiseExciter during onset would create an audible "edge" at the onset-window boundary as noise starts up.
- Additive is the simpler code path — single per-sample add, no `if (exciter.inOnset())` branch on the hot path.

**Verification at Gate 4**: subjective ear-listen during attack-character A/B (Gate 4 items 4-6) — if attack feels "too noisy" or "muddy", surface for v1.1 enhancement (e.g., breath-scale ramp on NoiseExciter during onset). At Phase 2.4, additive is the locked default.

### OQ#9-rev-4 — Active voice count: manual loop ✅ RESOLVED — **Lock: manual loop via `getNumVoices` + `getVoice(i)->isVoiceActive()`**

**JUCE 8.0.4 has no `getNumActiveVoices()` method.** Verified by grep over `/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.h` — no such public method.

**Pattern** (used inside `BassoonSynthesiser::findFreeVoice` override per OQ#1-rev-4):
```cpp
int active = 0;
const int n = getNumVoices();
for (int i = 0; i < n; ++i)
    if (getVoice (i)->isVoiceActive())
        ++active;
```

- `getNumVoices()` is `noexcept` and `inline` (returns `voices.size()`).
- `getVoice(int)` returns `SynthesiserVoice*` (pre-allocated 16-voice pool from Stage 1).
- `isVoiceActive()` is `virtual` on `SynthesiserVoice` (line 160) — small v-table dispatch.
- Allocation-free, RT-safe. Cost: 16 v-table calls per noteOn — negligible.

**Alternative considered**: walk protected `voices` array directly (`for (auto* v : voices)`). Same cost; public-API path is preferred for readability.

### OQ#10-rev-4 — Gate 4 PASS bar — DAW vs synthetic test fixture ✅ RESOLVED — **Lock: synthetic test fixture for NE; Bitwig MPE for pitch-bend**

**Item 8 — VST3 NE pitch event verification**:
- Logic Pro: limited NE support (timbre yes, tuning no per `kTuningTypeID`); not viable.
- Reaper / Cubase / Bitwig: NE support varies by version; most don't natively send `kTuningTypeID` events without scripting.
- **Dorico**: full NE support; but Dorico parity is Stage 4 deliverable (not Phase 2.4).

**Lock — synthetic test fixture path**: at Phase 2.4 verify, use a runtime debug build that exposes a UI button or keyboard shortcut to write a known semitone delta into `pendingTuningSource->setTuning(noteId=midiNote, deltaCents)` BEFORE pressing the next note. Verify with tuner that voice plays at expected pitch.

Concrete fixture options:
- (a) Add a temporary debug `juce::TextButton` to `OBassoonAudioProcessorEditor` that writes `+50` cents into `getPendingTable()[60]` (C4). Press button, then play C4 → tuner reads C+50 cents. Remove the button before atomic commit.
- (b) Add a `loadTestNETuning(noteOffset, cents)` AAX-/AU-host-callable test method (not exposed in production UI). Driven by a small Python/AppleScript fixture or an externally-fired MIDI program-change event that triggers `pendingTable.store`.
- **Recommend (a)** — simplest path, removable trivially before atomic commit, runs entirely inside the build under test.

**Item 7 — MPE pitch-bend verification**:
- Bitwig Studio: well-documented MPE support, accepts MPE controllers natively.
- Logic Pro 11: MPE support added; mileage varies.
- **Lock — Bitwig Studio with any MPE controller** (LinnStrument / ROLI Seaboard / Equator2 / virtual MPE keyboard plugin). If unavailable: skip Gate 4 item 7 and document as Stage 4 deferred.

**Items 1-6, 9, 10 use Logic-AU** (existing Phase 2.3 verification harness):
- Items 1-3 (polyphony / cap / retrigger): Logic AU multi-key smoke test.
- Items 4-6 (attack-character A/B): Logic-AU manual velocity sweep at low / mid / high velocity × `attack_character = 0/0.5/1.0`.
- Item 9 (60 s long-tone): Logic-AU 60-second bounce → Python `numpy.isfinite` + RMS drift + Logic CPU drift.
- Item 10 (8-voice CPU): Logic-AU hold 8 simultaneous notes with vibrato + breath active → Logic Process bar reading.

**Automated invariant battery** (new for Phase 2.4):
1. RT-safety grep zero-match in `Source/BassoonSynthesiser.cpp`, `Source/Exciter.cpp` (Phase 2.4 deltas), `Source/BassoonVoice.cpp`, `Source/PluginProcessor.cpp`: `new\|make_unique\|make_shared\|push_back\|resize\|malloc\b`.
2. NE-drain ordering at `PluginProcessor.cpp`: `drainAndUpdate()` MUST appear BEFORE `synthesiser.renderNextBlock`.
3. `BassoonSynthesiser` type swap at `PluginProcessor.h`: `BassoonSynthesiser synthesiser` (NOT `juce::Synthesiser`).
4. `voice_count` snapshot site at `PluginProcessor.cpp` prologue head (BEFORE tone-dispatch).
5. `applyPendingTuning` call at `BassoonVoice.cpp:startNote` exactly once.
6. `tuningEngine->getFrequency` call at `BassoonVoice.cpp:startNote` exactly once.
7. `exciter.startOnset` call at `BassoonVoice.cpp:startNote` exactly once.
8. `excitation += exciter.getNextSample()` (or `noiseExciter + exciter` form) at `BassoonVoice.cpp:renderNextBlock` exactly once.
9. Phase 2.3 invariants regression: tone-dispatch / expression-dispatch / NE-drain / renderNextBlock / output_gain order preserved; 1/8 scaler at `ModeBank.cpp:114` retained; 0.001f epsilon throttle ≥10 hits in `PluginProcessor.cpp`.
10. DSP-07 grep: zero `O-Reed\|OReed\|reed-` matches in any Phase 2.4 source.
11. `auval -v aumu OBsn OuDv` SUCCESS.
12. `pluginval --strictness 5` SUCCESS.

---

## §2 — Pattern Confirmations (rev-4)

### `BassoonSynthesiser` subclass — no Ouaricon-family precedent for `findFreeVoice` override

Survey of plugin-suite Synthesiser usage:

| Plugin | Synthesiser type | findFreeVoice override |
|--------|------------------|-----------------------|
| O-Lyrica | `juce::Synthesiser` (vanilla) | No — uses default |
| O-Wind | `juce::Synthesiser` (vanilla) | No — uses default |
| O-Bowed | `juce::Synthesiser` (vanilla) | No — uses default |
| O-Bells | `juce::Synthesiser` (vanilla) | No — uses default |
| O-MicrotonalSampler | `juce::Synthesiser` (vanilla) | No — uses default |
| **O-Bassoon Phase 2.4** | `BassoonSynthesiser` (NEW subclass) | **Yes — first in family** |

**Implication**: O-Bassoon's voice-cap with stealing is a new pattern for the Ouaricon plugin family. Phase 2.4 sets the precedent for any future polyphony-capped plugin (e.g., O-Reed v1.x). The override surface is minimal (5-15 lines) — extraction into a shared `OuariconCappedSynthesiser` module is a v1.1+ refactor candidate, NOT a Phase 2.4 deliverable.

### `TuningEngine::getFrequency` call site — O-Lyrica + O-Wind precedent

**O-Lyrica** (`Source/HarpSynthVoice.cpp:113-121`): null-guard pattern with `juce::MidiMessage::getMidiNoteInHertz` fallback.
**O-Wind** (`Source/FluteSynthVoice.cpp` — analogous structure).

Phase 2.4 BassoonVoice mirrors O-Lyrica precedent verbatim (minus humanize step which O-Bassoon doesn't have at v1.0).

### `applyPendingTuning` call site — O-Lyrica precedent

**O-Lyrica** (`Source/HarpSynthVoice.cpp:143-147`):
```cpp
if (pendingTuningSource != nullptr)
{
    currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
                           *pendingTuningSource, midiNoteNumber, currentFrequency);
}
```

Phase 2.4 BassoonVoice mirrors verbatim. Ordering: AFTER TuningEngine, BEFORE pitchBend/vibrato compose chain.

### MPE per-channel routing — JUCE 8.0.4 default behavior

`juce::Synthesiser::handlePitchWheel` (`juce_Synthesiser.cpp:403-410`) iterates voices and calls `pitchWheelMoved` only on voices matching the message channel. Per-channel MPE pitch-bend works out of the box. **No `MPESynthesiser` subclass needed for Phase 2.4 v1.0 scope** (CC1/aftertouch deferred).

### Pre-flight build verification

Working tree at Phase 2.3 in-tree state (verify rev-4 baseline): `M plugins/O-Bassoon/Source/{BassoonVoice.{h,cpp},PluginProcessor.{h,cpp}}` + `?? Source/{NoiseExciter,Vibrato}.{h,cpp}` + planning artefacts. Phase 2.3 atomic commit pending user trigger; tree state is the Phase 2.4 baseline.

Confirmed at research-phase via `git status --short plugins/O-Bassoon`: matches CONTEXT-rev-4 §"Working-tree starting state" (line 778-787).

---

## §3 — Implementation Skeletons (rev-4)

### `Source/BassoonSynthesiser.h` (NEW)

```cpp
/*
  ==============================================================================

    BassoonSynthesiser.h
    Modal Synthesis Bassoon - Voice manager with active-cap + JUCE default stealing
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.4: subclass juce::Synthesiser to enforce voice_count cap.
    findFreeVoice override gates by activeVoiceCap; delegates to base for
    free-pool selection and to base findVoiceToSteal (release-tail-first,
    then oldest-noteOn — JUCE 8 default).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class BassoonSynthesiser : public juce::Synthesiser
{
public:
    BassoonSynthesiser() noexcept
    {
        // Explicit-for-clarity; JUCE 8 default is true.
        setNoteStealingEnabled (true);
    }

    /** Sets the active voice cap. Snapshot at processBlock prologue per
        ROADMAP — applies on next note-on; already-active voices unaffected. */
    void setActiveVoiceCap (int cap) noexcept
    {
        activeVoiceCap = juce::jlimit (1, 16, cap);
    }

    int getActiveVoiceCap() const noexcept { return activeVoiceCap; }

protected:
    juce::SynthesiserVoice* findFreeVoice (juce::SynthesiserSound* sound,
                                           int                    channel,
                                           int                    noteNumber,
                                           bool                   stealIfNoneAvailable) const override
    {
        // Count active voices (allocation-free, RT-safe).
        int active = 0;
        const int n = getNumVoices();
        for (int i = 0; i < n; ++i)
            if (getVoice (i)->isVoiceActive())
                ++active;

        if (active < activeVoiceCap)
            return juce::Synthesiser::findFreeVoice (sound, channel, noteNumber, stealIfNoneAvailable);

        // At/over cap — steal if allowed, else null.
        return stealIfNoneAvailable
            ? findVoiceToSteal (sound, channel, noteNumber)
            : nullptr;
    }

private:
    int activeVoiceCap = 16;   // pre-allocated 16-voice pool from Stage 1
};
```

Header-only — no `BassoonSynthesiser.cpp` translation unit needed (override body fits in the class). CMake source-list addition is the `.h` only.

**Update**: per CMake convention in this project, NEW translation units are listed even header-only. If the builder pattern requires a `.cpp`, supply an empty TU:
```cpp
// BassoonSynthesiser.cpp
#include "BassoonSynthesiser.h"   // ensures header inclusion in build
```
Plan-phase chooses based on ROADMAP-style consistency check (Phase 2.3 added Vibrato + NoiseExciter as `.h+.cpp` pairs).

### `Source/Exciter.h` (MOD: rev-4 deltas to Phase 2.1)

```cpp
#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <algorithm>

class Exciter
{
public:
    static constexpr int   MAX_ONSET_SAMPLES = 1024;
    static constexpr float SOFT_DURATION_MS    = 5.0f;     // Phase 2.1 half-sine × exp; renamed
    static constexpr float SOFT_TAU_MS         = 1.5f;
    static constexpr float TONGUED_DURATION_MS = 7.5f;     // Phase 2.4 NEW: exp-decay × white noise

    void prepare (double sampleRate);

    /** Phase 2.1 wrapper — equivalent to startOnset(0.0f, 1.0f). Retained for
        backwards compatibility; Phase 2.4 callers use startOnset(...) directly. */
    void start() noexcept { startOnset (0.0f, 1.0f); }

    /** Phase 2.4: snapshot effective attack-character (with velocity bias) for
        the lifetime of the onset window. Mid-onset automation does NOT affect
        the in-flight onset (zipper avoidance — CONTEXT-rev-4 risk #2 mitigation). */
    void startOnset (float attackChar01, float velocity01) noexcept
    {
        const float biased    = attackChar01 + (velocity01 - 0.5f) * VELOCITY_BIAS_MAGNITUDE;
        effectiveAttackChar   = juce::jlimit (0.0f, 1.0f, biased);
        onsetIdx              = 0;
        active                = true;
    }

    inline float getNextSample() noexcept
    {
        if (! active || onsetIdx >= onsetSamples)
        {
            active = false;
            return 0.0f;
        }
        const auto i = static_cast<size_t> (onsetIdx++);
        // Linear morph between two pre-baked shapes; tonguedShape is zero-padded
        // for indices beyond TONGUED_DURATION_MS so the mix stays clean.
        return juce::jmap (effectiveAttackChar,
                           softShape[i], tonguedShape[i]);
    }

    void reset() noexcept { onsetIdx = 0; active = false; }

private:
    static constexpr float VELOCITY_BIAS_MAGNITUDE = 0.3f;   // OQ#4-rev-4 locked

    std::array<float, MAX_ONSET_SAMPLES> softShape    {};   // Phase 2.1 5 ms half-sine × exp (renamed)
    std::array<float, MAX_ONSET_SAMPLES> tonguedShape {};   // Phase 2.4 NEW: 7.5 ms exp-decay × white noise

    int   onsetSamples         = 0;   // length of the longer of the two windows
    int   onsetIdx             = 0;
    float effectiveAttackChar  = 0.0f;
    bool  active               = false;
};
```

### `Source/Exciter.cpp` (MOD: rev-4 deltas)

```cpp
#include "Exciter.h"

void Exciter::prepare (double sampleRate)
{
    // Phase 2.1 softShape — 5 ms half-sine × exp (carry-forward, renamed from onsetBuffer)
    const int softN = std::min (MAX_ONSET_SAMPLES,
                                static_cast<int> (sampleRate * SOFT_DURATION_MS * 0.001));

    for (int i = 0; i < softN; ++i)
    {
        const float t      = static_cast<float> (i) / static_cast<float> (sampleRate);
        const float window = std::sin (juce::MathConstants<float>::pi
                                       * static_cast<float> (i) / static_cast<float> (softN));
        const float decay  = std::exp (-t / (SOFT_TAU_MS * 0.001f));
        softShape[static_cast<size_t> (i)] = window * decay;
    }

    // Peak-normalise softShape
    float softPeak = 0.0f;
    for (int i = 0; i < softN; ++i)
        softPeak = std::max (softPeak, std::abs (softShape[static_cast<size_t> (i)]));
    if (softPeak > 1e-6f)
        for (int i = 0; i < softN; ++i)
            softShape[static_cast<size_t> (i)] /= softPeak;

    // Phase 2.4 tonguedShape — 7.5 ms exp-decay × white noise (NEW)
    const int tonguedN = std::min (MAX_ONSET_SAMPLES,
                                    static_cast<int> (sampleRate * TONGUED_DURATION_MS * 0.001));

    juce::Random rng (12345);   // deterministic seed
    for (int i = 0; i < tonguedN; ++i)
    {
        const float noise = rng.nextFloat() * 2.0f - 1.0f;     // [-1, 1]
        const float decay = std::exp (-static_cast<float> (i)
                                      / static_cast<float> (tonguedN) * 4.0f);  // 4 time-constants
        tonguedShape[static_cast<size_t> (i)] = noise * decay;
    }

    // Peak-normalise tonguedShape
    float tonguedPeak = 0.0f;
    for (int i = 0; i < tonguedN; ++i)
        tonguedPeak = std::max (tonguedPeak, std::abs (tonguedShape[static_cast<size_t> (i)]));
    if (tonguedPeak > 1e-6f)
        for (int i = 0; i < tonguedN; ++i)
            tonguedShape[static_cast<size_t> (i)] /= tonguedPeak;

    // Onset window covers the longer of the two shapes (softShape after peak-normalise).
    // tonguedShape is zero-padded beyond tonguedN by std::array zero-init.
    onsetSamples = std::max (softN, tonguedN);

    reset();
}
```

### `Source/BassoonVoice.h` (MOD: rev-4 deltas)

No new members required — Phase 2.4 voice-level changes are all body-level:
```cpp
// (no header-level deltas)
```

The existing `Exciter exciter` member (Phase 2.3 retained per D6-rev-3 verbatim) is re-engaged at runtime.

### `Source/BassoonVoice.cpp` (MOD: rev-4 deltas)

**`startNote` body — replace lines 57-59** (plain MIDI freq) **with**:
```cpp
    // Phase 2.4 (DSP-06 + TuningEngine): compose chain TuningEngine → applyPendingTuning.
    // O-Lyrica precedent — HarpSynthVoice.cpp:113-147.
    double f_double = (tuningEngine != nullptr)
        ? tuningEngine->getFrequency (midiNoteNumber)
        : juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    if (pendingTuningSource != nullptr)
        f_double = Ouaricon::NoteExpression::applyPendingTuning (
                       *pendingTuningSource, midiNoteNumber, f_double);

    currentFrequencyBase = static_cast<float> (f_double);
```

**`startNote` body — replace line 64** (`exciter.start()`) **with**:
```cpp
    // Phase 2.4 (DSP-05): re-engage Exciter via attack-character morph + velocity bias.
    // attackChar snapshot at note-on; mid-onset automation does NOT affect in-flight onset.
    const float attackChar = parameters->getRawParameterValue ("attack_character")->load();
    exciter.startOnset (attackChar, velocity);
```

**`renderNextBlock` body — modify per-sample inner loop** (around line 220 — current Phase 2.3 form):
```cpp
    // Phase 2.3 form:
    //   const float breath      = breathSmoother.getNextValue();
    //   const float excitation  = noiseExciter.getNextSample (breath);
    //   float voice             = modeBank.processSample (excitation);
    //   ...
    //
    // Phase 2.4 form — additive Exciter contribution (auto-zeros after onset window):
    const float breath        = breathSmoother.getNextValue();
    const float noiseSample   = noiseExciter.getNextSample (breath);
    const float exciterSample = exciter.getNextSample();   // 0 after onset window
    const float excitation    = noiseSample + exciterSample;
    float voice               = modeBank.processSample (excitation);
    voice                    *= adsr.getNextSample();
    outputBuffer.addSample (0, startSample + i, voice);
    outputBuffer.addSample (1, startSample + i, voice);
```

**`stopNote` body — line 106** already includes `exciter.reset()`; carries forward.

### `Source/PluginProcessor.h` (MOD: rev-4 deltas)

**Type swap** — change `juce::Synthesiser synthesiser;` to `BassoonSynthesiser synthesiser;` and add include:
```cpp
#include "BassoonSynthesiser.h"
// ...
class OBassoonAudioProcessor : public juce::AudioProcessor
{
    // ...
    BassoonSynthesiser synthesiser;
    int                lastDispatchedVoiceCount = -1;   // Phase 2.4: forces first-block dispatch
};
```

### `Source/PluginProcessor.cpp` (MOD: rev-4 deltas)

**`processBlock` prologue head — INSERT before line 197** (tone-dispatch):
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

**`prepareToPlay` body** — no Phase 2.4 deltas needed (16-voice pool already pre-allocated at Stage 1; `setActiveVoiceCap` snapshot fires at first `processBlock` call).

**Constructor (line 110-)** — no deltas: `synthesiser.addVoice(...)` API is identical between `juce::Synthesiser` and `BassoonSynthesiser` (inherited, not overridden).

### `plugins/O-Bassoon/CMakeLists.txt` (MOD: rev-4 delta)

`target_sources` block — ADD `BassoonSynthesiser.h` (and `.cpp` if pair convention is enforced):
```cmake
target_sources(O-Bassoon PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/BassoonSound.h
    Source/BassoonVoice.cpp
    Source/BassoonVoice.h
    Source/BassoonSynthesiser.h           # Phase 2.4 NEW
    # Source/BassoonSynthesiser.cpp       # IF .cpp pair convention enforced
    Source/ModeBank.cpp
    Source/ModeBank.h
    Source/Exciter.cpp                    # Phase 2.4 MOD: tonguedShape + startOnset morph
    Source/Exciter.h                      # Phase 2.4 MOD
    Source/Vibrato.cpp
    Source/Vibrato.h
    Source/NoiseExciter.cpp
    Source/NoiseExciter.h
)
```

Plan-phase decides `.cpp` pair convention based on Phase 2.3 precedent (NoiseExciter + Vibrato shipped as `.h+.cpp` pairs). Recommend: ship `BassoonSynthesiser.{h,cpp}` pair for consistency, with the `.cpp` containing only `#include "BassoonSynthesiser.h"` + a single static-storage placeholder if needed (avoids "no symbols" linker warnings on some toolchains).

### `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` rev-4 backfill template

Append at end of file, after rev-3 note:
```markdown
## Architectural rev-4 (Phase 2.4 as-shipped, 2026-04-29)

Phase 2.4 closes the 4 remaining DSP requirements:

1. **Voice manager** (`Source/BassoonSynthesiser.{h,cpp}` — NEW): subclass `juce::Synthesiser`. Overrides `findFreeVoice` to gate by `activeVoiceCap` (snapshot from APVTS `voice_count` at processBlock prologue, default 8). At/over cap, delegates to JUCE-default `findVoiceToSteal` (release-tail-first, then oldest-noteOn). `setNoteStealingEnabled(true)` set explicitly in constructor.

2. **Attack-character morph** (`Source/Exciter.{h,cpp}` — MOD): Phase 2.1 `softShape` carries forward (5 ms half-sine × exp); NEW `tonguedShape` (7.5 ms exp-decay × white noise, deterministic seed 12345, peak-normalised) added. `startOnset(attackChar, velocity)` snapshots `effectiveAttackChar = clamp(attackChar + (velocity - 0.5) * 0.3, 0, 1)` at note-on for the lifetime of the onset window — mid-onset automation only affects next note-on. `getNextSample()` returns `juce::jmap(effectiveAttackChar, softShape[i], tonguedShape[i])` for `i < onsetSamples`, else 0.

3. **f_base compose chain** (`Source/BassoonVoice.cpp::startNote`): replaces Phase 2.1-2.3 `juce::MidiMessage::getMidiNoteInHertz`. New chain: `f_double = tuningEngine->getFrequency(midiNote)` → `f_double = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNote, f_double)` → `currentFrequencyBase = static_cast<float>(f_double)`. Both pointer null-guards retained (defensive). Per-block `f_final = currentFrequencyBase × vibratoMult × pitchBendMult` (Phase 2.3) carries forward unchanged — operates on the NE-tuned base.

4. **NoiseExciter additive composition during onset** (`Source/BassoonVoice.cpp::renderNextBlock`): per-sample `excitation = noiseExciter.getNextSample(breath) + exciter.getNextSample()`. Both contribute during the 5-7.5 ms onset window; after onset, only NoiseExciter sustains. NoiseExciter at low breath-scaled level adds air-column hiss texture under the attack.

**MPE per-channel pitch-bend routing**: confirmed automatic via `juce::Synthesiser::handlePitchWheel` (juce_Synthesiser.cpp:403-410) — per-channel routing dispatches `pitchWheelMoved` only to voices on the matching channel via `voice->isPlayingChannel(midiChannel)`. No new Phase 2.4 code required; Phase 2.1 `pitchWheelMoved` override handles the dispatch.

**Regression invariants preserved**: Phase 2.1 voice mono-to-stereo write; Phase 2.2 bassoon-tuned partial table + formant Gaussian + 1/8 headroom scaler + tone smoother + applyToneChange; Phase 2.3 ADSR + breath/CC2-takeover + Vibrato + output_gain + NoiseExciter continuous excitation + processBlock prologue ordering.
```

---

## §4 — Discrepancies (rev-4)

### D1-rev-4 — `BassoonSynthesiser.cpp` translation unit may be empty

The override body fits cleanly in the header (it's a small `findFreeVoice` body). Adding an empty `.cpp` may produce "no symbols" toolchain warnings on some platforms. **Resolution**: ship a `.cpp` pair for consistency with NoiseExciter + Vibrato precedent (Phase 2.3 standard); if the file would be empty, include a single static placeholder (e.g., a no-op `void touchBassoonSynthesiserTU() {}` symbol) or move ALL definitions to the `.cpp` (header just declares). Plan-phase locks the choice.

### D2-rev-4 — `tonguedShape` pad-zero region may produce a discontinuity at the boundary

The `tonguedShape` array is 7.5 ms long; `softShape` is 5 ms long; `onsetSamples = max(softN, tonguedN) = tonguedN ≈ 7.5 ms`. For samples `i ∈ [softN, tonguedN)`, `softShape[i]` is zero-init; `tonguedShape[i]` is non-zero. The `jmap(char, 0, tonguedShape[i])` yields `char × tonguedShape[i]` — fine. But once `i ≥ tonguedN`, both arrays return zero — clean exit.

Wait — actually Phase 2.4 has softSamples=5ms < tonguedSamples=7.5ms. So the longer is tongued. After softN passes, softShape[i]=0, tonguedShape[i] still non-zero for one extra few hundred samples. `jmap(char, 0, tonguedSample) = char × tonguedSample` — interpolation produces silent for char=0 (soft-only) and audible for char>0. **This is correct intended behaviour**: the soft attack is shorter than tongued; the morph carries the noise burst's tail proportional to char.

**Resolution**: no fix needed — `std::array` zero-init handles the pad transparently.

### D3-rev-4 — `Exciter` original member name `onsetBuffer` vs Phase 2.4 `softShape`

Phase 2.1 used `onsetBuffer`. Phase 2.4 renames to `softShape` for clarity. Mechanical refactor — keep grep cleanliness:
- `grep -rn "onsetBuffer" Source/` should return zero hits after Phase 2.4 changes land.
- Rename in `Exciter.h`, `Exciter.cpp`. No callers reference `onsetBuffer` directly (it's private).

### D4-rev-4 — Cast double→float at compose-chain end loses ~7-8 bits of precision

`tuningEngine->getFrequency` returns `double`; `applyPendingTuning` returns `double`; `currentFrequencyBase` is `float`. The `static_cast<float>` at the end loses precision but is consistent with Phase 2.1-2.3 (Phase 2.1 already cast `getMidiNoteInHertz` from double to float). For 12-TET A=440 at MIDI 60 (C4 = 261.6256 Hz), float quantisation is ~16 µHz — far below any audible threshold. **Resolution**: lock the float cast at the end of the compose chain; matches family precedent.

### D5-rev-4 — `voice_count = 1` `findFreeVoice` corner case

When `activeVoiceCap = 1` and one voice is currently active, `active = 1 ≥ activeVoiceCap`. `findVoiceToSteal` is called; returns the single active voice. The base `noteOn` then calls `voice->stopNote(0.0f, false)` BEFORE `voice->startNote(...)` (juce_Synthesiser.cpp:337-338). This is the standard "mono mode" behavior — the stolen voice's stopNote is hard (allowTailOff=false), then immediately re-triggered.

The hard stopNote call hits `BassoonVoice::stopNote(velocity=0.0f, allowTailOff=false)` → invokes `clearCurrentNote() + adsr.reset() + modeBank.reset() + exciter.reset() + currentFrequencyBase = 0.0f` (Phase 2.3 path at BassoonVoice.cpp:103-107). Then `startNote` re-initialises everything fresh — no stuck note.

**Resolution**: no fix needed. Mono mode is documented expected behavior; high-rate retrigger (Gate 4 item 3) verifies the bookkeeping.

### D6-rev-4 — `setNoteStealingEnabled(true)` already JUCE 8 default

CONTEXT-rev-4 §"JUCE 8 critical patterns" notes "JUCE 8.0.4 default is `true` already". Verified at `juce_Synthesiser.h:640`: `bool shouldStealNotes = true;`. Calling `setNoteStealingEnabled(true)` in `BassoonSynthesiser` constructor is redundant but explicit-for-clarity — no behavioral change. **Resolution**: keep the explicit call (matches family pattern of locking critical defaults explicitly).

### D7-rev-4 — `effectiveAttackChar` is unset on first `startOnset` call before `prepare()`

The default `effectiveAttackChar = 0.0f` (header initialiser). `startOnset` always overwrites before any `getNextSample` reads it. If `prepare()` is somehow not called before `startOnset` (host bug or pre-initialisation), `onsetSamples = 0` so `getNextSample` returns 0 immediately — safe. **Resolution**: no fix needed; defensive default + early-exit guard cover the edge case.

---

## §5 — Static-Check Grep Gates Locked for Execute-Phase

Phase 2.4 verify-phase MUST run these greps and confirm zero/expected-count results:

| # | Pattern | Path | Expected Result |
|---|---------|------|-----------------|
| 1 | `new\|make_unique\|make_shared\|push_back\|resize\|malloc\b` | `Source/BassoonSynthesiser.{h,cpp}` `Source/Exciter.{h,cpp}` `Source/BassoonVoice.{h,cpp}` `Source/PluginProcessor.{h,cpp}` (Phase 2.4 deltas) | 0 matches |
| 2 | `vst3Extensions.drainAndUpdate\|drainAndUpdate()` | `PluginProcessor.cpp:processBlock` | 1 match BEFORE renderNextBlock |
| 3 | `BassoonSynthesiser synthesiser` | `PluginProcessor.h` | 1 match (NOT `juce::Synthesiser synthesiser`) |
| 4 | `synthesiser.setActiveVoiceCap` | `PluginProcessor.cpp` | 1 match in `processBlock` prologue head, BEFORE tone-dispatch |
| 5 | `applyPendingTuning` | `BassoonVoice.cpp:startNote` | 1 match |
| 6 | `tuningEngine->getFrequency` | `BassoonVoice.cpp:startNote` | 1 match |
| 7 | `exciter.startOnset` | `BassoonVoice.cpp:startNote` | 1 match |
| 8 | `noiseExciter.getNextSample.*\+.*exciter.getNextSample\|exciter.getNextSample.*\+.*noiseExciter` (additive composition) | `BassoonVoice.cpp:renderNextBlock` | 1 match (form may be 2 lines + sum into local) |
| 9 | `O-Reed\|OReed\|reed-` | `Source/*` recursive | 0 matches (DSP-07 regression) |
| 10 | `// 1.0f / 8.0f\|0.125f\|/ 8.0f` (1/8 scaler) | `Source/ModeBank.cpp:processSample` | 1 match (Phase 2.2 retention) |
| 11 | `0.001f` (epsilon throttle) | `Source/PluginProcessor.cpp` | ≥10 hits (Phase 2.3 expression dispatch carry-forward) |
| 12 | `setExpression\b` | `Source/PluginProcessor.cpp` | 1 match (single dispatch site, Phase 2.3 carry-forward — Phase 2.4 does NOT add attack_character to setExpression) |
| 13 | `applyGainRamp` | `Source/PluginProcessor.cpp` | 1 match AFTER `synthesiser.renderNextBlock` (Phase 2.3 output_gain carry-forward) |
| 14 | `modeBank.setFundamental` | `Source/BassoonVoice.cpp` | 3 matches (startNote / pitchWheelMoved / renderNextBlock per-block — Phase 2.1 & Phase 2.3 carry-forward) |
| 15 | `auval -v aumu OBsn OuDv` | runtime | exit 0 / VALIDATION SUCCEEDED |
| 16 | `pluginval --strictness 5 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3` | runtime | exit 0 |

---

## §6 — Outputs and Handoff Checklist (rev-4)

**RESEARCH-rev-4 outputs locked** (this section):

- ✅ §1 — 10 OQs resolved with JUCE 8.0.4 source-line cites + O-Lyrica/O-Wind precedent cites
- ✅ §2 — Pattern confirmations (BassoonSynthesiser is first-in-family voice-cap subclass; TuningEngine/applyPendingTuning compose chain mirrors O-Lyrica)
- ✅ §3 — Implementation skeletons (BassoonSynthesiser NEW; Exciter MOD; BassoonVoice MOD; PluginProcessor MOD; CMakeLists MOD; ARCHITECTURE rev-4 backfill template)
- ✅ §4 — 7 discrepancies registered with resolutions
- ✅ §5 — 16 static-check grep gates locked for verify-phase

**Handoff to plan-phase**:
- Plan-phase reads CONTEXT-rev-4 + RESEARCH-rev-4. Writes PLAN-rev-4 task breakdown for Phase 2.4 execute-phase.
- Plan-phase Task #1 = "Verify Phase 2.3 atomic commit (`feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`) lands on `main`" — hard gate before Phase 2.4 execute-phase begins (CONTEXT-rev-4 process invariant).
- Plan-phase covers: BassoonSynthesiser NEW (`.h+.cpp` pair per Phase 2.3 precedent), Exciter MOD (rename `onsetBuffer` → `softShape`, ADD `tonguedShape` + `startOnset` + velocity bias snapshot), BassoonVoice MOD (4 lines in startNote + 1 line in renderNextBlock), PluginProcessor MOD (type swap + voice_count snapshot at prologue head), CMakeLists MOD (target_sources +1 or +2), ARCHITECTURE rev-4 backfill, REQUIREMENTS update, STATUS update.
- Plan-phase locks atomic commit subject: `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS`.
- Inline iteration ceiling at rev-3 (Phase 2.2/2.3 precedent).

**Handoff to verify-phase** (post-execute):
- Gate 4 PASS bar: 10 user-checkable items + 16-item static-check grep battery + auval + pluginval-5.
- Item 8 (NE pitch event) uses synthetic test fixture (debug-build temporary button writing into `pendingTable`); item 7 (MPE pitch-bend) uses Bitwig + MPE controller, OR document as Stage 4 deferred if unavailable.
- 60 s long-tone (item 9) uses Phase 2.3 protocol: Logic-AU 60 s bounce + Python `numpy.isfinite` + RMS drift + Logic CPU drift t=10s vs t=60s within ±2 %.
- 8-voice CPU (item 10) uses 8 simultaneously-held notes in Logic-AU under enforced `voice_count = 8` cap with vibrato + breath active; bar < 25 % per ROADMAP PERF-02.

---

## Audit Trail (rev-4 addendum)

**rev-4 (this section, 2026-04-29):** Phase 2.4 research-phase. Resolved 10 OQs with JUCE 8.0.4 source citations (juce_Synthesiser.{h,cpp} for findFreeVoice / findVoiceToSteal / handlePitchWheel; juce_AudioProcessorValueTreeState.h for getRawParameterValue) and module citations (TuningEngine.{h,cpp} for getFrequency signature + bit-identity confirmation; NoteExpression.h for applyPendingTuning inline body; HarpSynthVoice.cpp:113-147 for compose-chain precedent). Pattern-confirmed first-in-Ouaricon-family voice-cap subclass; locked 16-item static-check grep battery for verify-phase. 7 discrepancies registered with resolutions. Implementation skeletons cover BassoonSynthesiser NEW + Exciter MOD + BassoonVoice MOD (4 startNote lines + 1 renderNextBlock line) + PluginProcessor MOD (type swap + voice_count snapshot) + CMakeLists MOD + ARCHITECTURE rev-4 backfill template.

**Inherited verbatim from rev-3 + earlier (not re-litigated):**
- All Phase 2.1-2.3 patterns: per-sample render loop, mode-bank coefficient cadence, NE drain ordering, throttled-epsilon dispatch, 1/8 headroom scaler, vibrato compose chain, breath/CC2-takeover state machine, output_gain applyGainRamp idiom.
- DSP-07 regression principle.
- Atomic-commit gate-first principle.

**New in rev-4:**
- `BassoonSynthesiser` subclass pattern locked (override `findFreeVoice` only; delegate to base `findVoiceToSteal` for release-tail-first stealing).
- Active-voice manual count loop (`getNumVoices` + `getVoice(i)->isVoiceActive()`) — JUCE 8.0.4 has no public `getNumActiveVoices()`.
- `voice_count` snapshot site at processBlock prologue head with integer-comparison throttle.
- Exciter dual-shape (softShape rename + tonguedShape NEW exp-decay × white noise, 7.5 ms, deterministic seed) with `startOnset(attackChar, velocity)` velocity-bias snapshot at note-on.
- `f_base` compose chain at startNote (TuningEngine.getFrequency → applyPendingTuning → currentFrequencyBase) — O-Lyrica HarpSynthVoice precedent verbatim.
- TuningEngine bit-identity at default 12-TET A=440 (verified via internal `calculate12TETFrequency` body).
- MPE per-channel pitch-bend routing automatic via `juce::Synthesiser::handlePitchWheel` (no `MPESynthesiser` needed for v1.0 scope).
- NoiseExciter additive composition during onset window (per-sample `noise + exciter` sum).
- 16-item static-check grep battery locked for verify-phase (1 NEW: `BassoonSynthesiser synthesiser` type swap; 1 NEW: `synthesiser.setActiveVoiceCap` snapshot site; 1 NEW: `applyPendingTuning` call site; 1 NEW: `tuningEngine->getFrequency` call site; 1 NEW: `exciter.startOnset` call site; 1 NEW: noise+exciter additive composition site; 10 carry-forward regression checks).
- Phase 2.3 atomic commit dependency for Phase 2.4 execute (process invariant; PLAN-rev-4 task #1).

