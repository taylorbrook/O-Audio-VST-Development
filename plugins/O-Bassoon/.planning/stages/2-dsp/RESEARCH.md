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
