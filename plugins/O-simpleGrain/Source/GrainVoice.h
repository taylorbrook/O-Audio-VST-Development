/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-simpleGrain - Grain Voice (one polyphonic granular voice)

    A juce::SynthesiserVoice that owns a preallocated bounded grain pool
    (std::array<Grain, kMaxGrainsPerVoice>), a per-sample scheduler, a seeded
    counter-based RNG (v1.6.0), and a juce::ADSR amp envelope. Each held MIDI note transposes
    the grain cloud (key-tracked resample); grains read windowed fragments from
    the shared static source buffer and are overlap-added into the output.

    Phase 2.1: core grain engine + overlap-add + amp ADSR + key resample.
    Phase 2.2 (this file): per-voice spray & scatter (position/pitch/period/pan),
    velToDensity into the effective density, and the anti-aliasing one-pole filled
    in the grain read loop (v1.7.0: replaced by the band-limited mipmap read). The processor advances a moving/freezable global
    playhead and hands the voices its per-sample values via setPlayheadTrack.

    Real-time safety (every line of renderNextBlock): NO allocation / lock / I/O.
    Preallocated pool; steal-oldest when full (never resize). Stateless RNG draws.
    ADSR setSampleRate BEFORE setParameters in the (non-virtual) prepareToPlay.

    JUCE 8: SynthesiserVoice has NO virtual prepareToPlay — this declares a
    NON-VIRTUAL prepareToPlay(double,int) dispatched by the processor via
    dynamic_cast (O-simpleFM / O-Bassoon pattern).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include "PluginProcessor.h"            // engine constants (kMaxGrainsPerVoice, kRootNote)
#include "GrainSound.h"
#include "dsp/Grain.h"
#include "dsp/WindowLuts.h"
#include "dsp/LagrangeInterpolation.h"
#include "dsp/GrainSource.h"            // v1.7.0 — band-limited mipmap
#include "CounterRng.h"            // grain-engine module (ouaricon::grain::CounterRng), v2.0.1
#include "dsp/GrainCloudFrame.h"        // Phase 2.3 — grain-event viz tap

//==============================================================================
// Block-pushed parameter bundle. Defined generously so Phase 2.2 can add
// spray/scatter/velToDensity fields with low churn (the voice ignores any field
// it does not yet use). Voices never touch the APVTS — the processor reads it
// once per block and calls setParams(...).
struct GrainVoiceParams
{
    float grainSizeMs = 30.0f;   // 2..500 ms
    float density     = 40.0f;   // grains/sec (clamped >= 1 by the scheduler)
    int   windowShape = 4;       // 0=rect .. 4=Hann, 5=Tukey
    float windowTaper = 0.5f;    // Tukey α 0..1 (v1.4.0)
    float grainPitch  = 0.0f;    // global transposition in semitones

    // --- Phase 2.2 spray & scatter (per-grain RNG; processor pushes these) ---
    float positionSpray = 0.0f;  // 0..100 % of source length — read-start jitter
    float pitchSpray    = 0.0f;  // 0..12 st — ± per-grain rate jitter
    float scatter       = 0.0f;  // 0..100 % — scheduler period jitter (sync↔async)
    float panSpray      = 0.0f;  // 0..100 % — per-grain stereo spread
    float velToDensity  = 0.0f;  // 0..100 % (stored 0..1) — velocity -> density depth

    // --- v1.8.0 (research 2026 update §5 item 6, Tempera 2.3) ---
    float grainSkip     = 0.0f;  // 0..1 — probability a scheduled grain is skipped
    float grainTilt     = 0.0f;  // -1..1 — window peak: -1 plucked .. +1 reverse swell

    // v1.1.0 — amplitude-envelope bypass. false: flat velocity level while the
    // note is held; on release the voice stops LAUNCHING grains and the cloud
    // fades out over one grain length through the window envelopes (no click).
    bool adsrEnabled = true;

    juce::ADSR::Parameters amp { 0.01f, 0.3f, 0.8f, 0.4f };

    // --- v2.0.0 Pulsar mode (Cloud never reads these) ---
    int   grainMode      = 0;      // 0 = Cloud, 1 = Pulsar
    float formant        = 4.0f;   // f_formant / f_p = 1 / duty, 1..32
    int   pulsaretSource = 0;      // 0 = Sine, 1 = Sample
    int   pulseMask      = 0;      // 0 = Off, else N (2..8): pulse k muted when k mod N == N - 1
    float fmDepth        = 0.0f;   // linear rate deviation D, 0..0.95
    float fmRatio        = 0.5f;   // modulator rate / f_p, 0.25..8 (whole numbers are inert)
    float glisson        = 0.0f;   // st across one pulsaret, -24..24
};

//==============================================================================
class GrainVoice : public juce::SynthesiserVoice
{
public:
    GrainVoice() = default;
    ~GrainVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<GrainSound*> (sound) != nullptr;
    }

    //==========================================================================
    // Non-virtual JUCE-8 prepare. setSampleRate MUST precede setParameters.
    void prepareToPlay (double sr, int /*maxBlockSize*/)
    {
        setCurrentPlaybackSampleRate (sr);
        sampleRate = sr;

        ampEnv.setSampleRate (sr);              // BEFORE setParameters (RT-safety gate 8)
        ampEnv.setParameters (ampParams);
        appliedAmpParams = ampParams;
        releasing = false;

        // Clear the pool so a sample-rate change never leaves stale grains running.
        for (auto& g : grains) g.active = false;
        nextGrain = 0;
        samplesUntilNextGrain = 0;

        // v2.0.0 Pulsar: smoothers + clock restart on the next Pulsar block.
        formantSmoothed.reset (sr, kPulsarSmoothSeconds);
        fmDepthSmoothed.reset (sr, kPulsarSmoothSeconds);
        pulsarRunning = false;

        // Drop our viz-count contribution (the processor zeroes the shared atomic
        // in its prepareToPlay; we forget our last contribution so the next
        // publishActiveCount() recomputes a clean delta from 0).
        myActiveCount = 0;
    }

    // Shared window-LUT table set (owned by the processor). Set once after
    // construction (a const pointer the read loop dereferences — no copy).
    void setWindowLuts (const WindowLuts* luts) noexcept { windowLuts = luts; }

    //==========================================================================
    // Block param-push from the processor (once per block). Carries the 2.1 core
    // params + the 2.2 spray/scatter/velToDensity fields. Voices never touch APVTS.
    void setParams (const GrainVoiceParams& p) noexcept
    {
        params = p;
        ampParams = p.amp;
        // Push envelope params only when they actually CHANGED, and never while
        // the voice is releasing. juce::ADSR::setParameters() recalculates
        // releaseRate from the SUSTAIN level, clobbering the envelopeVal-based
        // rate noteOff() computed — and with sustain == 0 the zeroed rate makes
        // recalculateRates() reset() the envelope mid-tail: instant cut = click
        // on every note-off. Changes made mid-release apply at the next noteOn.
        // (O-simpleFM v1.2.5 pattern.)
        if (! releasing && ! sameAdsr (ampParams, appliedAmpParams))
        {
            ampEnv.setParameters (ampParams);
            appliedAmpParams = ampParams;
        }
    }

    // Source snapshot the processor sets once per block (it holds the shared_ptr
    // alive for the whole block; the voice only reads the raw pointers/len). A
    // null source -> the voice renders silence (no grains read). `mip` is the
    // same source's band-limited mipmap (v1.7.0), read by rate > 1 grains.
    void setSource (const float* src, const GrainSource::Snapshot* mip, int len) noexcept
    {
        sourcePtr = src;
        sourceMip = mip;
        sourceLen = len;
    }

    // The moving/freezable global read head, one value per sample of this block
    // (in source samples), set by the processor once per block. A grain spawned
    // on sample i reads from track[i] (± position spray). v1.6.0: was the
    // block-start value for every spawn in the block, so with Scan ≠ 0 the host
    // block size moved where grains started. Indices past `len` (a host block
    // larger than prepared) clamp to the last entry — never read out of range.
    // double end-to-end (IN-04): no float re-quantization on long sources.
    void setPlayheadTrack (const double* track, int len) noexcept
    {
        playheadTrack    = track;
        playheadTrackLen = len;
    }

    // v1.6.0 — seeded randomness. `seed` is the processor's active instance seed
    // and `noteCounter` its running note-on index (both audio-thread only). Each
    // note-on draws its own seed from (instance seed, note index), so which voice
    // slot the synth picks never changes a note's grains. Set once at construction.
    void setRngSource (const juce::uint64* seed, juce::uint64* noteCounter) noexcept
    {
        instanceSeed     = seed;
        noteOnCounterPtr = noteCounter;
    }

    //==========================================================================
    // Phase 2.3 viz taps. The processor sets these once per block (like the
    // playhead seam) so the voice spawn signature stays untouched:
    //   setGrainCloudFrame : the block's GrainCloudFrame the voice appends a
    //                        GrainEvent to at each spawn (bounded by kMaxEvents —
    //                        extras dropped, never grown). Null between blocks.
    //   setActiveGrainCount: the processor-owned atomic the voice increments on
    //                        spawn and decrements when a grain finishes (UI-05).
    //   setBlockStartSample: the global sample offset of this block's render start
    //                        (for GrainEvent::spawnSample). 0 here since the synth
    //                        renders the whole block at once (startSample passed to
    //                        renderNextBlock is the buffer offset).
    void setGrainCloudFrame  (GrainCloudFrame* frame) noexcept { cloudFrame = frame; }
    void setActiveGrainCount (std::atomic<int>* counter) noexcept { activeGrainCount = counter; }
    //   setPulsarBus: v2.0.0 — the processor-owned pulsaret bus (L, R, length in
    //                 samples, and a flag the voice raises when it writes). Pulsarets
    //                 take their own output gain there, so a mode switch never moves
    //                 the Cloud bus gain under draining grains of the other kind.
    //                 Samples past the bus length fall back to the main buffer.
    void setPulsarBus (float* l, float* r, int len, bool* used) noexcept
    {
        pulsarBusL = l; pulsarBusR = r; pulsarBusLen = len; pulsarBusUsed = used;
    }

    //==========================================================================
    void startNote (int midiNote, float velocity,
                    juce::SynthesiserSound*, int /*pitchWheel*/) override
    {
        // Key-tracked resample: held note transposes the cloud (C3 = recorded pitch).
        voiceRate = std::pow (2.0f, (float) (midiNote - OSimpleGrainAudioProcessor::kRootNote) / 12.0f);

        // Clear the pool — a new note starts a fresh cloud.
        for (auto& g : grains) g.active = false;
        nextGrain = 0;
        samplesUntilNextGrain = 0;               // fire a grain on the first sample

        // A fresh random sequence for this note (v1.6.0).
        if (instanceSeed != nullptr && noteOnCounterPtr != nullptr)
            noteSeed = ouaricon::grain::CounterRng::hash (*instanceSeed, (*noteOnCounterPtr)++, kNoteSeedSlot);
        spawnTick = 0;

        // v2.0.0 Pulsar: the key is the pulse rate (A4 = 440 Hz); the pulse clock
        // (and its smoothers) restart at the first Pulsar sample of the note.
        noteHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        pulsarRunning = false;

        velLevel = juce::jlimit (0.0f, 1.0f, velocity);
        noteHeld = true;

        // Apply any envelope params deferred while the previous tail released.
        releasing = false;
        if (! sameAdsr (ampParams, appliedAmpParams))
        {
            ampEnv.setParameters (ampParams);
            appliedAmpParams = ampParams;
        }

        ampEnv.noteOn();          // ticked regardless of adsrEnabled so a mid-note
                                  // toggle lands on a coherent envelope state
    }

    void stopNote (float, bool allowTailOff) override
    {
        noteHeld = false;
        if (allowTailOff)
        {
            releasing = true;   // freeze envelope rates until the tail ends
            ampEnv.noteOff();
            // adsrEnabled == false: no envelope tail — the scheduler stops
            // launching grains (see renderNextBlock) and the in-flight grains
            // window out within one grain length.
        }
        else
        {
            clearCurrentNote();
            releasing = false;
            ampEnv.reset();
            for (auto& g : grains) g.active = false;
            publishActiveCount();           // hard stop -> our grains drop to 0 (UI-05)
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        // Voice liveness (v1.1.0): with the ADSR enabled the envelope is the
        // lifetime key (unchanged from 2.1). Bypassed, the voice lives while the
        // note is held OR grains are still draining after release (each finishes
        // within one grain length via its window — the click-free "gate" mode).
        const bool adsrOn = params.adsrEnabled;
        const bool alive  = adsrOn ? ampEnv.isActive()
                                   : (noteHeld || myActiveCount != 0);
        if (! alive)
        {
            // If we were contributing grains last block, drop our contribution to
            // the shared count so a silent voice doesn't pin the readout (UI-05).
            if (myActiveCount != 0)
            {
                for (auto& g : grains) g.active = false;
                publishActiveCount();
            }
            return;
        }

        const int numCh = out.getNumChannels();

        // Effective density -> base inter-grain interval (samples). velToDensity
        // (0..1 depth) makes harder notes spawn a denser cloud: a note above the
        // mid-velocity (0.5) thickens, below thins. velLevel is fixed at note-on,
        // so the base interval is constant across the block; scatter jitters the
        // ACTUAL interval per spawn (below) — the sync↔async axis (DSP-05).
        const float effectiveDensity = juce::jlimit (1.0f, 200.0f,
            params.density * (1.0f + params.velToDensity * (velLevel - 0.5f) * 2.0f));
        const float baseInterval = (float) sampleRate / effectiveDensity;

        // Grain length in samples (clamp >= 2 so phaseInc is finite).
        const float lenSamp = juce::jmax (2.0f, (params.grainSizeMs * 0.001f) * (float) sampleRate);
        const float phaseInc = 1.0f / lenSamp;

        // v2.0.0 — Pulsar mode replaces the Cloud scheduler for this block (the
        // grain pool is shared, so grains of the other kind window out
        // undisturbed after a switch). Cloud keeps its scheduler verbatim below.
        const bool pulsarMode = (params.grainMode == 1);
        if (pulsarMode)
            beginPulsarBlock();
        else
            pulsarRunning = false;

        for (int i = 0; i < numSamples; ++i)
        {
            if (pulsarMode)
            {
                tickPulseClock (adsrOn, startSample + i);
            }
            else
            {
            // --- Scheduler: fire a grain when the countdown elapses -----------
            // ADSR bypassed + note released → stop LAUNCHING (the release
            // behaviour is "cloud drains over one grain length"); the countdown
            // keeps running so a re-toggle mid-drain stays coherent. With the
            // ADSR on, spawning continues through the release tail (unchanged —
            // the envelope shapes it out).
            if (--samplesUntilNextGrain <= 0)
            {
                // v1.8.0 Skip: each scheduled grain is dropped with probability
                // grainSkip, decided here, per grain, from its own RNG slot. The
                // clock keeps running, so a skipped grain leaves a hole in the
                // pattern rather than shifting the next one. Skip 0 draws nothing.
                if ((adsrOn || noteHeld) && ! skipThisGrain())
                    spawnGrain (lenSamp, phaseInc, startSample + i);
                // Scatter (RESEARCH §2.6): jitter the period ± scatter% of the
                // base interval. 0% = constant period (synchronous → discrete
                // sidebands); 100% = ±full random (asynchronous → noise).
                const float jitter = (params.scatter * 0.01f) * baseInterval
                                   * bipolarDraw (kSlotScatter);
                samplesUntilNextGrain = juce::jmax (1, (int) (baseInterval + jitter));
                ++spawnTick;
            }
            }   // Cloud scheduler

            // --- Overlap-add the active grains -------------------------------
            float outL = 0.0f, outR = 0.0f;
            float pulL = 0.0f, pulR = 0.0f;       // v2.0.0 — pulsarets, own bus
            bool  anyPulsaret = false;

            for (auto& g : grains)
            {
                if (! g.active)
                    continue;

                if (g.pulsaret)                   // v2.0.0 — Pulsar grains render apart
                {
                    renderPulsaret (g, pulL, pulR);
                    anyPulsaret = true;
                    continue;
                }

                const float env = (windowLuts == nullptr) ? 0.0f
                                : g.tilted ? windowLuts->readTilted (g.shape, g.phase, g.tiltPeak, g.tiltTeA, g.tiltTeR)
                                           : windowLuts->read (g.shape, g.phase, g.taperEnd);
                // rate > 1: band-limited mipmap read (v1.7.0, DSP-08); else Lagrange.
                const float src = g.bandlimited ? readSourceBandlimited (g)
                                                : readSourceLagrange (sourcePtr, sourceLen, g.readPos);

                const float s = src * env * g.fadeGain;
                // Equal-power pan gains were precomputed on spawn (constant per grain).
                outL += s * g.panL;
                outR += s * g.panR;

                g.readPos += g.rate;
                g.phase   += g.phaseInc;
                ++g.age;
                if (g.phase >= 1.0f)
                    g.active = false;             // grain done

                if (g.fadeStep > 0.0f)            // v1.5.4: stolen grain fading out
                {
                    g.fadeGain -= g.fadeStep;
                    if (g.fadeGain <= 0.0f)
                        g.active = false;
                }
            }

            // --- Voice amplitude (amp ADSR * velocity) -----------------------
            // The envelope is ALWAYS ticked (so an ADSR-off→on toggle mid-note
            // resumes from a coherent point) but only shapes the output when
            // enabled; bypassed, the note plays at its flat velocity level.
            const float envVal = ampEnv.getNextSample();
            const float ampVal = (adsrOn ? envVal : 1.0f) * velLevel;
            outL *= ampVal;
            outR *= ampVal;

            if (anyPulsaret)
            {
                const int idx = startSample + i;
                if (pulsarBusL != nullptr && idx < pulsarBusLen)
                {
                    pulsarBusL[idx] += pulL * ampVal;
                    pulsarBusR[idx] += pulR * ampVal;
                    *pulsarBusUsed = true;
                }
                else                              // oversized host block: main bus
                {
                    outL += pulL * ampVal;
                    outR += pulR * ampVal;
                }
            }

            // Add into the output buffer (mono: sum L+R for a centred image).
            if (numCh >= 2)
            {
                out.addSample (0, startSample + i, outL);
                out.addSample (1, startSample + i, outR);
            }
            else if (numCh == 1)
            {
                out.addSample (0, startSample + i, 0.5f * (outL + outR));
            }
        }

        // Publish this voice's live grain count to the shared atomic (UI-05).
        // Recomputed by scanning the pool (O(24)) so steal-oldest / grain-done /
        // note-clear all stay consistent — robust vs. fragile inc/dec.
        // (Also refreshes myActiveCount for the ADSR-bypass done check below.)
        publishActiveCount();

        // Voice lifetime: amp envelope (ADSR on) / released-and-drained (bypass).
        const bool done = adsrOn ? ! ampEnv.isActive()
                                 : (! noteHeld && myActiveCount == 0);
        if (done)
        {
            for (auto& g : grains) g.active = false;
            publishActiveCount();           // release tail finished -> drop to 0
            ampEnv.reset();
            clearCurrentNote();
            releasing = false;
        }
    }

private:
    //==========================================================================
    static bool sameAdsr (const juce::ADSR::Parameters& a, const juce::ADSR::Parameters& b) noexcept
    {
        return a.attack  == b.attack  && a.decay   == b.decay
            && a.sustain == b.sustain && a.release == b.release;
    }

    //==========================================================================
    // Spawn a grain: find an inactive slot, else steal the oldest (max age).
    // Bounded by kMaxGrainsPerVoice -> never allocates, never xruns (PERF-02).
    void spawnGrain (float lenSamp, float phaseInc, int spawnSample) noexcept
    {
        constexpr int N = OSimpleGrainAudioProcessor::kMaxGrainsPerVoice;

        int target = -1, oldest = 0, maxAge = -1;
        for (int k = 0; k < N; ++k)
        {
            const int idx = (nextGrain + k) % N;
            if (! grains[(size_t) idx].active) { target = idx; break; }
            if (grains[(size_t) idx].age > maxAge) { maxAge = grains[(size_t) idx].age; oldest = idx; }
        }
        if (target < 0)
        {
            target = oldest;                      // steal oldest ...
            moveToFadeSlot (grains[(size_t) target]);   // ... which fades out in a spare slot (v1.5.4)
        }

        auto& g = grains[(size_t) target];
        g.active        = true;
        g.pulsaret      = false;                  // v2.0.0 — the slot may have held a pulsaret
        g.fadeGain      = 1.0f;
        g.fadeStep      = 0.0f;
        g.age           = 0;
        g.phase         = 0.0f;
        g.phaseInc      = phaseInc;
        g.lengthSamples = lenSamp;
        g.shape         = params.windowShape;
        // v1.4.0 — taper end (phase units) for the two remapped shapes. Fixed on
        // spawn because it depends on THIS grain's length: the rect guard is a
        // time (1 ms), not a phase fraction, so it stays inaudible at 500 ms and
        // still fits inside a 2 ms grain (clamped to a full Hann at 0.5).
        g.taperEnd      = WindowLuts::taperEndFor (params.windowShape, params.windowTaper,
                                                   lenSamp, sampleRate);
        // v1.8.0 — tilt, fixed on spawn (its 1 ms click floor depends on THIS
        // grain's length, like the taper). Tilt 0 leaves the grain untilted.
        g.tilted = (params.grainTilt != 0.0f);
        if (g.tilted)
        {
            const auto t = WindowLuts::tiltFor (params.windowShape, params.grainTilt,
                                                g.taperEnd, lenSamp, sampleRate);
            g.tiltPeak = t.peak;
            g.tiltTeA  = t.teA;
            g.tiltTeR  = t.teR;
        }

        // --- Position spray: scatter the read start ± positionSpray% of the
        // source length around the current playhead. Under freeze the playhead is
        // pinned, so spray still scatters reads WITHIN the frozen region → a
        // frozen pad shimmers rather than buzzing on one grain (FUNC-03).
        // Wrapped into [0, sourceLen) (IN-07, v1.1.2), consistent with the
        // playhead's own wrap: an out-of-range spawn used to pin all four Lagrange
        // taps to the edge sample and emit a constant-value (windowed DC) thump —
        // at 100% spray up to half the spawns were affected.
        const float posSprayRand = bipolarDraw (kSlotPosition)
                                 * (params.positionSpray * 0.01f) * (float) sourceLen;
        double spawnPos = playheadAt (spawnSample) + (double) posSprayRand;
        if (sourceLen > 0)
        {
            spawnPos = std::fmod (spawnPos, (double) sourceLen);
            if (spawnPos < 0.0)
                spawnPos += (double) sourceLen;
        }
        g.readPos = spawnPos;

        // --- Pitch spray: fresh ± pitchSpray semitones per grain, combined with
        // the key rate and the global grainPitch offset (decision #2 — all three
        // multiply into the read increment). rate > 1 = up-transposed (read band-
        // limited from the mipmap; see readSourceBandlimited).
        const float pitchSprayRand = bipolarDraw (kSlotPitch) * params.pitchSpray;
        g.rate = voiceRate * std::pow (2.0f, (params.grainPitch + pitchSprayRand) / 12.0f);

        // --- Pan spray: equal-power pan scattered ± panSpray% around centre.
        g.pan = juce::jlimit (0.0f, 1.0f,
            0.5f + bipolarDraw (kSlotPan) * (params.panSpray * 0.01f) * 0.5f);
        // Equal-power gains evaluated ONCE here (constant for the grain's life) —
        // the render loop just multiplies, no per-sample cos/sin.
        g.panL = std::cos (g.pan * juce::MathConstants<float>::halfPi);
        g.panR = std::sin (g.pan * juce::MathConstants<float>::halfPi);

        // --- Band-limited read (v1.7.0): only up-transposed grains (rate > 1)
        // alias. The octave level is latched from the rate (floor (log2 rate),
        // so rate 2..4 reads the half-rate level) and the sinc set from the
        // residual rate on that level; the rate is fixed for the grain, so both
        // are chosen here, once. rate <= 1 keeps the Lagrange read, bit-identical
        // to v1.6.0. Replaces the v1.6.0 one-pole (fc = fs / 2rate, 6 dB/oct:
        // the aliases passed almost untouched and the top end dulled).
        g.bandlimited = (g.rate > 1.0f && sourceMip != nullptr);
        g.mipLevel    = g.bandlimited ? ouaricon::grain::latchLevel ((double) g.rate, GrainSource::kMipLevels) : 0;
        g.sincSet     = g.bandlimited ? ouaricon::grain::SincBank::setFor ((double) g.rate / (double) (1 << g.mipLevel)) : 0;

        nextGrain = (target + 1) % N;

        // --- Viz tap (Phase 2.3): append a GrainEvent to the block's cloud frame
        // (UI-01 scatter + UI-02 playheads). Bounded by kMaxEvents — extras are
        // DROPPED, never grown (RT-safe, copy-only). The processor reset count=0
        // at block start and publish()es once per block.
        if (cloudFrame != nullptr && cloudFrame->count < GrainCloudFrame::kMaxEvents)
        {
            auto& ev = cloudFrame->events[(size_t) cloudFrame->count++];
            ev.readPosNorm = (sourceLen > 0)
                           ? juce::jlimit (0.0f, 1.0f, (float) (g.readPos / (double) sourceLen)) : 0.0f;
            ev.sizeMs      = (float) (lenSamp / juce::jmax (1.0, sampleRate) * 1000.0);
            // Pitch shown relative to the source's recorded pitch: voice key +
            // global offset + this grain's spray (semitones).
            const float keySemis = 12.0f * std::log2 (juce::jmax (1.0e-6f, voiceRate));
            ev.pitchSemis  = keySemis + params.grainPitch + pitchSprayRand;
            ev.pan         = g.pan;
            ev.spawnSample = spawnSample;
        }
    }

    //==========================================================================
    // v2.0.0 — PULSAR MODE (research 2026 update §3.6, §3.1; RESEARCH.md).
    //
    // The key sets the pulse rate f_p; each pulse fires one pulsaret of length
    // L = d / f_p, duty d = 1 / Formant, whose content is one sine cycle or one
    // root-key period of the source squeezed into L (so the content sits at the
    // formant f_p / d). The clock is a double phase accumulator ticked per
    // sample; each onset carries its sub-sample offset δ into the window phase,
    // the content phase and the read position (fractional onsets, §3.1), so the
    // output depends only on the sample count since note-on, never on the block.
    //
    // Anti-aliasing: the highest content rate a pulsaret reaches (formant x
    // glisson end x pitch-spray ceiling) is held under kFormantCeilRatio · fs
    // by clamping the duty — and f_p itself when a top key alone would pass it.
    // Pulsarets under 1.25 ms crossfade to Blackman-Harris (fully BH under
    // 0.75 ms) with per-window power gains, so the band has no level step.

    // Per-block constants (static parameters hold for the block; Formant and
    // FM Depth are smoothed per sample in tickPulseClock).
    void beginPulsarBlock() noexcept
    {
        if (! pulsarRunning)
        {
            // First Pulsar block of this note (or after a Cloud stretch): the
            // next sample is a pulse, and the smoothers start on their targets.
            pulsarRunning     = true;
            pulseStartPending = true;
            pulsePhase        = 0.0;
            fmPhase           = 0.0;
            pulseK            = 0;
            pulseVizCountdown = 0;
            formantSmoothed.setCurrentAndTargetValue (params.formant);
            fmDepthSmoothed.setCurrentAndTargetValue (params.fmDepth);
        }
        formantSmoothed.setTargetValue (params.formant);
        fmDepthSmoothed.setTargetValue (params.fmDepth);

        const double ceilHz  = OSimpleGrainAudioProcessor::kFormantCeilRatio * sampleRate;
        const double glideUp = std::exp2 ((double) juce::jmax (0.0f, params.glisson) / 12.0);
        const double sprayUp = std::exp2 ((double) juce::jmax (0.0f, params.pitchSpray) / 12.0);
        pulseCeilHz = ceilHz / (glideUp * sprayUp);
        pulseHz     = juce::jmin (noteHz * std::exp2 ((double) params.grainPitch / 12.0), pulseCeilHz);
        pulseGlideMaxUp = glideUp;
        fmStep = (double) params.fmRatio * pulseHz / sampleRate;            // Δm, modulator cycles / sample
        fmSinc = std::sin (juce::MathConstants<double>::pi * fmStep) / (juce::MathConstants<double>::pi * fmStep);
        pulseVizInterval = juce::jmax (1, (int) (kPulsarVizSeconds * sampleRate));
    }

    // One sample of the pulse clock; spawns the pulsaret (if any) BEFORE the
    // overlap-add of this sample, so it sounds on its own onset sample.
    void tickPulseClock (bool adsrOn, int spawnSample) noexcept
    {
        const double formantNow = (double) formantSmoothed.getNextValue();
        const double fmD        = (double) fmDepthSmoothed.getNextValue();

        // Linear rate FM around f_p (mean rate stays f_p). D = 0 skips the FM
        // arithmetic; the modulator phase always runs, so FM stays locked to the
        // note-on whenever it is turned on. With FM on, the increment is the
        // EXACT integral of f_p (1 + D sin 2πφm) over this sample (the modulator
        // runs at up to 8 f_p, a few samples per cycle at the top of the
        // keyboard: a rectangle-rule step would jitter the onsets by up to a
        // sample per pulse and smear the spectrum, measured -38 dB). The
        // integral of sin over [φm, φm + Δm] is sin(2π(φm + Δm/2)) · sinc(Δm).
        const double inc0 = pulseHz / sampleRate;
        const double phm0 = fmPhase;
        double inc = inc0;
        if (fmD > 0.0)
            inc = inc0 * (1.0 + fmD * fmSinc
                                 * std::sin (juce::MathConstants<double>::twoPi * (phm0 + 0.5 * fmStep)));
        fmPhase += fmStep;
        fmPhase -= std::floor (fmPhase);

        if (pulseVizCountdown > 0)
            --pulseVizCountdown;

        bool onset = false;
        double delta = 0.0;               // samples since the crossing, [0, 1)
        if (pulseStartPending)
        {
            pulseStartPending = false;
            onset = true;                 // the note's first Pulsar sample is a pulse
        }
        else
        {
            const double before = pulsePhase;
            pulsePhase += inc;
            if (pulsePhase >= 1.0)
            {
                pulsePhase -= 1.0;
                onset = true;
                delta = (fmD > 0.0) ? fmCrossingDelta (before, inc0, inc, fmD, phm0, fmStep)
                                    : juce::jlimit (0.0, 0.999999, pulsePhase / inc);
            }
        }
        if (! onset)
            return;

        const juce::uint64 k = pulseK++;
        // Mask (mute 1 of N): pulse k is silent when k mod N == N - 1. Off
        // evaluates no modulo. Skip: the v1.8.0 random drop, one draw per pulse.
        const int maskN = params.pulseMask;
        const bool masked = (maskN > 1) && ((int) (k % (juce::uint64) maskN) == maskN - 1);
        if ((adsrOn || noteHeld) && ! masked && ! skipThisPulse (k))
            spawnPulsaret (delta, formantNow, spawnSample, k);
    }

    // Under FM the phase is not linear inside the sample: solve
    //   before + inc0 [s + D (cos 2πφ0 − cos 2π(φ0 + Δm s)) / (2πΔm)] = 1
    // for the crossing s ∈ [0, 1] from the linear guess, and return δ = 1 − s.
    // The left side rises monotonically from < 1 at s = 0 to >= 1 at s = 1, so
    // the root is bracketed: Newton steps that leave the bracket fall back to
    // bisection. (Plain Newton drifted by up to 0.7 sample once the modulator
    // step Δm passed ~0.4, e.g. FM Ratio 8 above f_p ≈ 1.2 kHz at 48 kHz.)
    // Converges to 1e-9 sample, ~8 steps on average; bounded at 50. Public and
    // static for the render harness (P7 checks it against bisection).
public:
    static double fmCrossingDelta (double before, double inc0, double inc, double fmD, double phm0,
                                   double step) noexcept
    {
        constexpr double twoPi = juce::MathConstants<double>::twoPi;
        constexpr double tol   = 1.0e-9;
        const double c0 = std::cos (twoPi * phm0);
        const double k  = fmD / (twoPi * step);
        double lo = 0.0, hi = 1.0;
        double s = juce::jlimit (0.0, 1.0, (1.0 - before) / inc);
        for (int it = 0; it < 50; ++it)
        {
            const double a  = twoPi * (phm0 + step * s);
            const double f  = before + inc0 * (s + k * (c0 - std::cos (a))) - 1.0;
            const double fp = inc0 * (1.0 + fmD * std::sin (a));
            if (f < 0.0) lo = s;
            else         hi = s;
            double next = s - f / fp;
            if (! (next > lo && next < hi))
                next = 0.5 * (lo + hi);
            const bool done = std::abs (next - s) <= tol || hi - lo <= tol;
            s = next;
            if (done)
                break;
        }
        return juce::jlimit (0.0, 0.999999, 1.0 - s);
    }
private:

    void spawnPulsaret (double delta, double formantNow, int spawnSample, juce::uint64 k) noexcept
    {
        constexpr int N = OSimpleGrainAudioProcessor::kMaxGrainsPerVoice;

        // Duty: 1 / Formant, floored so the content's top rate stays under the
        // ceiling (pulseCeilHz already divides out glisson and pitch spray).
        const double d     = juce::jlimit (pulseHz / pulseCeilHz, 1.0, 1.0 / formantNow);
        const double lSamp = d * sampleRate / pulseHz;           // >= fs / ceil >= 10 samples
        const double lMs   = lSamp * 1000.0 / sampleRate;

        int target = -1, oldest = 0, maxAge = -1;
        for (int j = 0; j < N; ++j)
        {
            const int idx = (nextGrain + j) % N;
            if (! grains[(size_t) idx].active) { target = idx; break; }
            if (grains[(size_t) idx].age > maxAge) { maxAge = grains[(size_t) idx].age; oldest = idx; }
        }
        if (target < 0)
        {
            target = oldest;
            moveToFadeSlot (grains[(size_t) target]);
        }

        auto& g = grains[(size_t) target];
        g.active        = true;
        g.pulsaret      = true;
        g.fadeGain      = 1.0f;
        g.fadeStep      = 0.0f;
        g.age           = 0;
        g.phaseInc      = (float) (1.0 / lSamp);
        g.phase         = (float) (delta / lSamp);              // fractional onset (window)
        g.lengthSamples = (float) lSamp;
        g.shape         = params.windowShape;
        g.taperEnd      = WindowLuts::taperEndFor (params.windowShape, params.windowTaper,
                                                   (float) lSamp, sampleRate);
        g.tilted        = false;   // tilt's 3 ms floor makes it inert on every pulsaret

        // Blackman-Harris share: smoothstep over 1.25 ms (none) .. 0.75 ms (all).
        {
            const double t = juce::jlimit (0.0, 1.0, (1.25 - lMs) / 0.5);
            g.bhMix = (float) (t * t * (3.0 - 2.0 * t));
        }
        // Power gains vs Hann, for the content this pulsaret carries (sine: the
        // sine-weighted window moment; sample: μ₂). See WindowLuts.
        const bool sineContent = (params.pulsaretSource == 0);
        g.gUser = (windowLuts != nullptr) ? windowLuts->powerGainVsHann (g.shape, g.taperEnd, sineContent) : 1.0f;
        g.gBh   = (windowLuts != nullptr) ? windowLuts->bhPowerGain (sineContent) : 1.0f;

        // Per-pulse draws (tick = pulse index k, the Cloud slots).
        const float pitchSprayRand = pulseDraw (kSlotPitch, k) * params.pitchSpray;
        const double sprayMul = std::exp2 ((double) pitchSprayRand / 12.0);

        g.pan  = juce::jlimit (0.0f, 1.0f,
            0.5f + pulseDraw (kSlotPan, k) * (params.panSpray * 0.01f) * 0.5f);
        g.panL = std::cos (g.pan * juce::MathConstants<float>::halfPi);
        g.panR = std::sin (g.pan * juce::MathConstants<float>::halfPi);

        // Glisson: geometric glide start -> start x 2^(glisson/12) over L.
        g.glide    = (params.glisson > 0.0f || params.glisson < 0.0f);
        g.glideMul = g.glide ? std::exp2 ((double) params.glisson / (12.0 * lSamp)) : 1.0;

        double contentHz = pulseHz / d * sprayMul;               // the content's start rate
        g.sineContent = sineContent;
        if (g.sineContent)
        {
            // One cycle across the pulsaret (more/less with pitch spray).
            g.contentInc   = sprayMul / lSamp;
            g.contentPhase = delta * g.contentInc;
            g.bandlimited  = false;
            g.readPos      = 0.0;
            g.rate         = (float) (contentHz / rootHz());
        }
        else
        {
            spawnSampleContent (g, delta, contentHz, lSamp, spawnSample, k);
        }

        nextGrain = (target + 1) % N;

        // Viz (decimated): at most one event per kPulsarVizSeconds per voice, on
        // a countdown that never touches the clock or the RNG.
        if (pulseVizCountdown <= 0)
        {
            pulseVizCountdown = pulseVizInterval;
            if (cloudFrame != nullptr && cloudFrame->count < GrainCloudFrame::kMaxEvents)
            {
                auto& ev = cloudFrame->events[(size_t) cloudFrame->count++];
                const double posNow = g.sineContent ? playheadAt (spawnSample) : g.readPos;
                ev.readPosNorm = (sourceLen > 0)
                               ? juce::jlimit (0.0f, 1.0f, (float) (posNow / (double) sourceLen)) : 0.0f;
                ev.sizeMs      = (float) lMs;
                ev.pitchSemis  = (float) (12.0 * std::log2 (juce::jmax (1.0e-6, contentHz / rootHz())));
                ev.pan         = g.pan;
                ev.spawnSample = spawnSample;
            }
        }
    }

    // Sample content: one period of the source's recorded pitch (the root key,
    // kRootNote) squeezed into L, i.e. read at r = f_content / f_root — so at
    // Formant 1x the source plays at the same rate as a Cloud grain on that key.
    // The read starts at the playhead + Position Spray (Position, Scan and Freeze
    // all work), offset by r·δ for the fractional onset.
    // Anti-aliasing: the window spreads the source's own spectrum by about
    // ±4 / L cycles per sample (BH), so for short pulsarets the reader's cutoff
    // is pulled in: the mip level and sinc set are latched from the over-filtered
    // rate r·γ (γ = 0.5 / max(0.05, 0.5 − 4/L)) at the glide's TOP rate, while
    // readPos still advances by r. Above ~50-sample pulsarets γ ≈ 1 and r·γ <= 1
    // keeps the Lagrange read.
    void spawnSampleContent (Grain& g, double delta, double contentHz, double lSamp,
                             int spawnSample, juce::uint64 k) noexcept
    {
        const double r = contentHz / rootHz();

        const float posSprayRand = pulseDraw (kSlotPosition, k)
                                 * (params.positionSpray * 0.01f) * (float) sourceLen;
        double spawnPos = playheadAt (spawnSample) + (double) posSprayRand;
        if (sourceLen > 0)
        {
            spawnPos = std::fmod (spawnPos, (double) sourceLen);
            if (spawnPos < 0.0)
                spawnPos += (double) sourceLen;
        }
        g.readPos      = spawnPos + r * delta;
        g.contentInc   = r;
        g.contentPhase = 0.0;
        g.rate         = (float) r;

        const double gamma = 0.5 / juce::jmax (0.05, 0.5 - 4.0 / lSamp);
        const double rTop  = r * gamma * pulseGlideMaxUp;
        g.bandlimited = (rTop > 1.0 && sourceMip != nullptr);
        g.mipLevel    = g.bandlimited ? ouaricon::grain::latchLevel (rTop, GrainSource::kMipLevels) : 0;
        g.sincSet     = g.bandlimited ? ouaricon::grain::SincBank::setFor (rTop / (double) (1 << g.mipLevel)) : 0;
    }

    // One sample of one pulsaret (mirrors the Cloud body: window x content x
    // steal fade, equal-power pan, advance, done / fade checks).
    void renderPulsaret (Grain& g, float& outL, float& outR) noexcept
    {
        float env = 0.0f;
        if (windowLuts != nullptr)
        {
            const float a = g.bhMix;
            if (a <= 0.0f)
                env = g.gUser * windowLuts->read (g.shape, g.phase, g.taperEnd);
            else if (a >= 1.0f)
                env = g.gBh * windowLuts->readBH (g.phase);
            else
                env = (1.0f - a) * g.gUser * windowLuts->read (g.shape, g.phase, g.taperEnd)
                    + a * g.gBh * windowLuts->readBH (g.phase);
        }

        float src;
        if (g.sineContent)
        {
            src = kSineContentTrim * (float) std::sin (juce::MathConstants<double>::twoPi * g.contentPhase);
            g.contentPhase += g.contentInc;
        }
        else
        {
            src = g.bandlimited ? readSourceBandlimited (g)
                                : readSourceLagrange (sourcePtr, sourceLen, g.readPos);
            g.readPos += g.contentInc;
        }
        if (g.glide)
            g.contentInc *= g.glideMul;

        const float s = src * env * g.fadeGain;
        outL += s * g.panL;
        outR += s * g.panR;

        g.phase += g.phaseInc;
        ++g.age;
        if (g.phase >= 1.0f)
            g.active = false;

        if (g.fadeStep > 0.0f)
        {
            g.fadeGain -= g.fadeStep;
            if (g.fadeGain <= 0.0f)
                g.active = false;
        }
    }

    // The root key's frequency (the source's recorded pitch, kRootNote).
    static double rootHz() noexcept
    {
        return juce::MidiMessage::getMidiNoteInHertz (OSimpleGrainAudioProcessor::kRootNote);
    }

    float pulseDraw (juce::uint32 slot, juce::uint64 k) const noexcept
    {
        return ouaricon::grain::CounterRng::uniform (noteSeed, k, slot) * 2.0f - 1.0f;
    }

    bool skipThisPulse (juce::uint64 k) const noexcept
    {
        return params.grainSkip > 0.0f
            && ouaricon::grain::CounterRng::uniform (noteSeed, k, kSlotSkip) < params.grainSkip;
    }

    // Uniform in [-1, 1) for this note's current spawn tick. One tick per
    // scheduler countdown, a fixed slot per use, so every value is a pure function
    // of (note seed, tick) — independent of block size and draw order (v1.6.0).
    float bipolarDraw (juce::uint32 slot) const noexcept
    {
        return ouaricon::grain::CounterRng::uniform (noteSeed, spawnTick, slot) * 2.0f - 1.0f;
    }

    // v1.8.0 — true if this tick's grain is skipped. A pure function of (note
    // seed, tick) on its own slot, so it is block-size independent and never
    // moves the position / pitch / pan / scatter draws. At 0 nothing is drawn;
    // at 1 every grain is skipped (uniform() < 1 always).
    bool skipThisGrain() const noexcept
    {
        return params.grainSkip > 0.0f
            && ouaricon::grain::CounterRng::uniform (noteSeed, spawnTick, kSlotSkip) < params.grainSkip;
    }

    // The global playhead on buffer sample `sample` of this block (see
    // setPlayheadTrack). 0 with no track (never set / not yet prepared).
    double playheadAt (int sample) const noexcept
    {
        if (playheadTrack == nullptr || playheadTrackLen <= 0)
            return 0.0;
        return playheadTrack[juce::jlimit (0, playheadTrackLen - 1, sample)];
    }

    // Random-access fractional read into the static source (clamp at bounds —
    // a grain that runs off the end tapers out via its window, RESEARCH §2.5).
    // pos is double (IN-04) so the fractional part stays exact on long sources.
    static float readSourceLagrange (const float* src, int len, double pos) noexcept
    {
        if (src == nullptr || len <= 0)
            return 0.0f;

        const int   i0   = (int) pos;
        const float frac = (float) (pos - (double) i0);
        const int   im1  = juce::jlimit (0, len - 1, i0 - 1);
        const int   ip0  = juce::jlimit (0, len - 1, i0);
        const int   ip1  = juce::jlimit (0, len - 1, i0 + 1);
        const int   ip2  = juce::jlimit (0, len - 1, i0 + 2);
        return lagrangeInterpolate (src[im1], src[ip0], src[ip1], src[ip2], frac);
    }

    // Band-limited fractional read (v1.7.0, DSP-08): one 16-tap sinc on the
    // grain's latched mipmap level. The snapshot's origin is source sample 0, so
    // readPos is its position as-is; the mipmap is padded with the edge samples,
    // so a grain running off either end holds the edge value like the Lagrange
    // clamp (RESEARCH §2.5). Allocation- and lock-free (grain-engine contract).
    float readSourceBandlimited (const Grain& g) const noexcept
    {
        return sourceMip->read (g.mipLevel, 0, g.readPos, g.sincSet);
    }

    // v1.5.4: copy a stolen grain into a spare slot and fade it out over kStealFadeMs,
    // instead of cutting it mid-window. The new grain still takes the stolen slot, so
    // steal-free renders are bit-identical to v1.5.3. With every spare already fading,
    // the spare nearest silence is reused.
    void moveToFadeSlot (const Grain& victim) noexcept
    {
        constexpr int N = OSimpleGrainAudioProcessor::kMaxGrainsPerVoice;
        int slot = N;
        float minGain = 2.0f;
        for (int k = N; k < N + kFadeSlots; ++k)
        {
            const auto& f = grains[(size_t) k];
            if (! f.active) { slot = k; break; }
            if (f.fadeGain < minGain) { minGain = f.fadeGain; slot = k; }
        }

        auto& f = grains[(size_t) slot];
        f = victim;
        if (f.fadeStep <= 0.0f)
            f.fadeStep = (float) (1.0 / juce::jmax (1.0, sampleRate * kStealFadeMs / 1000.0));
    }

    // Recompute this voice's active grain count (scan the bounded pool, O(24))
    // and apply the delta to the shared atomic. Robust vs. steal-oldest /
    // grain-done / note-clear (which a fragile inc/dec would mishandle). The
    // atomic is a relaxed running sum across all voices (UI-05).
    void publishActiveCount() noexcept
    {
        int live = 0;
        for (const auto& g : grains)
            if (g.active) ++live;

        if (activeGrainCount != nullptr && live != myActiveCount)
            activeGrainCount->fetch_add (live - myActiveCount, std::memory_order_relaxed);

        myActiveCount = live;
    }

    //==========================================================================
    double sampleRate = 44100.0;
    float  voiceRate  = 1.0f;            // key-tracked read-increment ratio
    float  velLevel   = 1.0f;
    bool   noteHeld   = false;           // v1.1.0 — lifetime key when the ADSR is bypassed

    // Source snapshot (raw pointer set per block; processor holds the shared_ptr).
    const float* sourcePtr = nullptr;
    const GrainSource::Snapshot* sourceMip = nullptr;   // v1.7.0 — same source's mipmap
    int          sourceLen = 0;

    // Per-sample global playhead for the current block (processor-owned buffer;
    // see setPlayheadTrack). double (IN-04): no float re-quantization.
    const double* playheadTrack    = nullptr;
    int           playheadTrackLen = 0;

    // Phase 2.3 viz-tap seams (processor sets per block; null between blocks).
    GrainCloudFrame*  cloudFrame       = nullptr;   // grain events appended at spawn
    std::atomic<int>* activeGrainCount = nullptr;   // shared running grain count (UI-05)
    int               myActiveCount    = 0;         // this voice's last-published contribution
    float*            pulsarBusL       = nullptr;   // v2.0.0 — pulsaret bus (setPulsarBus)
    float*            pulsarBusR       = nullptr;
    int               pulsarBusLen     = 0;
    bool*             pulsarBusUsed    = nullptr;

    // Preallocated bounded grain pool + scheduler state. The last kFadeSlots entries
    // only ever hold stolen grains fading out (v1.5.4).
    static constexpr int    kFadeSlots     = 4;
    static constexpr double kStealFadeMs   = 5.0;
    std::array<Grain, OSimpleGrainAudioProcessor::kMaxGrainsPerVoice + kFadeSlots> grains {};
    int nextGrain = 0;
    int samplesUntilNextGrain = 0;

    // Counter-based RNG (v1.6.0; was a clock-seeded juce::Random per voice, so no
    // two renders matched). noteSeed is drawn at note-on, spawnTick counts the
    // scheduler countdowns since then; the slots keep the four draws per tick apart.
    static constexpr juce::uint32 kNoteSeedSlot = 0x6e6f7465u;
    static constexpr juce::uint32 kSlotPosition = 0;
    static constexpr juce::uint32 kSlotPitch    = 1;
    static constexpr juce::uint32 kSlotPan      = 2;
    static constexpr juce::uint32 kSlotScatter  = 3;
    static constexpr juce::uint32 kSlotSkip     = 4;   // v1.8.0
    const juce::uint64* instanceSeed     = nullptr;
    juce::uint64*       noteOnCounterPtr = nullptr;
    juce::uint64        noteSeed  = 0;
    juce::uint64        spawnTick = 0;

    // v2.0.0 Pulsar clock + smoothers (see beginPulsarBlock / tickPulseClock).
    static constexpr double kPulsarSmoothSeconds = 0.02;   // Formant / FM Depth ramps
    static constexpr double kPulsarVizSeconds    = 0.01;   // viz: <= 1 event / 10 ms / voice
    static constexpr float  kSineContentTrim     = 0.0625f; // -24.1 dB (2^-4): full-scale Sine pulsarets down to Cloud / Sample level
    double       noteHz            = 440.0;
    bool         pulsarRunning     = false;
    bool         pulseStartPending = false;
    double       pulsePhase        = 0.0;   // φp, [0, 1)
    double       fmPhase           = 0.0;   // φm, [0, 1)
    juce::uint64 pulseK            = 0;     // pulse index (mask + RNG tick)
    double       pulseHz           = 440.0; // f_p, capped at the ceiling
    double       pulseCeilHz       = 4410.0;
    double       pulseGlideMaxUp   = 1.0;
    double       fmStep            = 0.0;   // Δm = fmRatio f_p / fs
    double       fmSinc            = 1.0;   // sin(πΔm) / (πΔm)
    int          pulseVizCountdown = 0;
    int          pulseVizInterval  = 441;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> formantSmoothed { 4.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> fmDepthSmoothed { 0.0f };

    juce::ADSR ampEnv;
    juce::ADSR::Parameters ampParams { 0.01f, 0.3f, 0.8f, 0.4f };
    juce::ADSR::Parameters appliedAmpParams = ampParams;   // last values pushed to the live envelope
    bool releasing = false;

    GrainVoiceParams params;
    const WindowLuts* windowLuts = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainVoice)
};
