/*
   This file is part of O-simpleSampler, an Ouaricon Audio plugin.
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

    O-simpleSampler - Sample Voice (one polyphonic Repitch read head)

    A custom juce::SynthesiserVoice (NOT juce::SamplerVoice) that plays the shared
    source buffer through a fractional-read varispeed ("Repitch") head: each held
    MIDI note sets a read increment (keyRatio) relative to the live Root Key, the
    head reads the source with 4-point Lagrange interpolation, an anti-alias
    one-pole band-limits up-transposed notes, and a per-voice juce::ADSR + a
    velocity-sensitivity blend (velToAmp) shape the amplitude. Confined to the
    [startSamp, endSamp) region (one-shot in Phase 2.1 — no loop yet).

    Phase 2.1: Repitch read head + region isolation + AA + amp ADSR + velToAmp.
    Phase 2.2a (this file): loop (off / forward / ping-pong) with an equal-power
    dual-head crossfade + zero-cross-snapped bounds, reverse, a raised-cosine
    region-end declick, a clean-at-zero Vintage macro (sample&hold decimate ->
    bit-crush), and a per-voice resonant LP filter (juce::dsp::StateVariableTPTFilter).
    The downstream tail (Vintage -> Filter -> VCA·endRamp) is shared with Stretch.

    Phase 2.2b (this file): Stretch SOLA — a voice-local time axis (timePos) drives
    a fixed-hop synchronous-granular overlap-add (60 ms Hann grains, hop = grainSize/2
    => 2x overlap) whose grains ride the key rate (RESEARCH P6). pitchMode (LATCHED at
    note-on) forks ONLY the read head: Repitch advances readPos by voiceRate; Stretch
    advances timePos at 1x realtime (duration preserved, pitch tracks the key). Both
    feed the SAME shared downstream tail. The Hann window LUT is a single processor-
    owned table shared read-only by all 16 voices (NEVER built per-voice / in render).

    NB: named SampleVoice, NOT SamplerVoice. juce::SamplerVoice exists in
    juce_audio_formats, and the generated JuceHeader.h does `using namespace
    juce;`, so a class literally named `SamplerVoice` is AMBIGUOUS at every
    unqualified use (same class of bug as the regionStart/regionEnd vs juce::end
    collision). Keep this name `Sampler`-free.

    Real-time safety (every line of renderNextBlock): NO allocation / lock / I/O.
    The voice holds a RAW const float* + int snapshot of the source (the processor
    owns the shared_ptr and keeps it alive for the whole block) — it never owns or
    resizes an AudioBuffer. juce::ScopedNoDenormals + a final std::isfinite scrub
    live on the processor side.

    JUCE 8: SynthesiserVoice has NO virtual prepareToPlay — this declares a
    NON-VIRTUAL prepareToPlay(double,int) dispatched by the processor via
    dynamic_cast (O-simpleGrain / O-simpleSubtractive pattern). ADSR setSampleRate
    MUST precede setParameters (JUCE-8 order gate).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <array>                          // std::array<Grain,N> fixed grain pool (SOLA)
#include <utility>                        // std::pair (equal-power crossfade weights)
#include "PluginProcessor.h"             // engine constants (kMaxGrainsPerVoice, kStretchGrainMs)
#include "SampleSound.h"
#include "dsp/Grain.h"                    // Grain POD — SOLA-native (readPos=time, rate=pitch)
#include "dsp/WindowLuts.h"              // Hann LUT (shape 4); processor-owned, shared read-only
#include "dsp/LagrangeInterpolation.h"   // global lagrangeInterpolate(ym1,y0,y1,y2,frac)

//==============================================================================
// Block-pushed parameter bundle. The processor reads the APVTS atomics once per
// block and calls setParams(...) — voices never touch the APVTS. Region bounds
// arrive as absolute sample indices in the SOURCE frame (the processor computes
// them from the start/end % of the published source length).
struct SamplerVoiceParams
{
    int   rootKey   = 60;       // live Root Key (MIDI) — keyRatio reference (NOT kRootNote)
    int   tune      = 0;        // coarse transpose (semitones)
    float fine      = 0.0f;     // fine transpose (cents)
    int   startSamp = 0;        // region start (samples, source frame)
    int   endSamp   = 0;        // region end   (samples, source frame, exclusive)
    float velToAmp  = 50.0f;    // velocity sensitivity 0–100 %
    juce::ADSR::Parameters amp; // amp ADSR (A/D/S/R)

    //--- Phase 2.2a — tone chain (RESEARCH P1.1) ---------------------------
    // All ABS source-frame indices are computed processor-side (start/end as % of
    // source; loop bounds as % of region; then overwritten by the off-thread
    // zero-cross snap). The filter cutoff/Q arrive already clamped + smoothed by
    // the processor (one SmoothedValue each — NOT 16× per-voice).
    int   loopMode      = 0;        // 0 off / 1 forward / 2 ping-pong
    int   loopStartSamp = 0;        // ABS source frame (zero-cross-snapped)
    int   loopEndSamp   = 0;        // ABS source frame, exclusive (zero-cross-snapped)
    int   xfadeSamp     = 0;        // loop crossfade length (samples)
    bool  reverse       = false;    // seeds the read direction at note-on
    int   pitchMode     = 0;        // 0 Repitch / 1 Stretch (SOLA) — LATCHED per-voice at note-on
    float vintage       = 0.0f;     // 0–100 % (bit-for-bit clean at 0)
    float filterCutoff  = 20000.0f; // Hz (processor: jlimit 20..0.45·fs + 20 ms smoothed)
    float filterQ       = 0.707f;   // Q  (processor: jmap filterResonance% 0.707..12 + smoothed)
};

//==============================================================================
class SampleVoice : public juce::SynthesiserVoice
{
public:
    SampleVoice() = default;
    ~SampleVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SampleSound*> (s) != nullptr;
    }

    //==========================================================================
    // Block param-push from the processor (once per block). Re-apply the amp ADSR
    // parameters live so envelope-time knobs track without a note retrigger.
    void setParams (const SamplerVoiceParams& p) noexcept
    {
        params = p;
        ampEnv.setParameters (p.amp);
    }

    // Source snapshot the processor sets once per block: a raw pointer + length
    // (the processor holds the owning shared_ptr alive for the whole block). The
    // voice only ever READS this — it never owns an AudioBuffer. Null → silence.
    void setSource (const float* src, int len) noexcept
    {
        sourcePtr = src;
        sourceLen = len;
    }

    // Shared window-LUT table (owned by the processor — built ONCE off the audio
    // thread, shared read-only by all 16 voices). Set once after construction in the
    // processor's prepareToPlay; the Stretch overlap-add dereferences it for the Hann
    // envelope (shape 4). RT-CRITICAL: never construct a WindowLuts per-voice or in
    // the render path — WindowLuts::build() allocates (RESEARCH P6 / WindowLuts.h).
    void setWindowLuts (const WindowLuts* l) noexcept { windowLuts = l; }

    //==========================================================================
    // Non-virtual JUCE-8 prepare (dispatched by the processor via dynamic_cast).
    // setSampleRate MUST precede setParameters (RT-safety / JUCE-8 order gate).
    void prepareToPlay (double sr, int blockSize)
    {
        setCurrentPlaybackSampleRate (sr);
        ampEnv.setSampleRate (sr);
        ampEnv.setParameters (params.amp);

        // Per-voice resonant LP (RESEARCH P5.1). Prepared MONO — processSample(0,·).
        // Cutoff/Q are pushed from the processor's smoothed scalars ONCE per block
        // (one tan recompute), never per sample. reset() in startNote clears state.
        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sr;
        spec.maximumBlockSize = (juce::uint32) juce::jmax (1, blockSize);
        spec.numChannels      = 1;
        filt.prepare (spec);
        filt.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        filt.reset();
    }

    //==========================================================================
    void startNote (int midiNote, float velocity,
                    juce::SynthesiserSound*, int /*pitchWheel*/) override
    {
        const double fs = getSampleRate();

        // Repitch read-increment (keyRatio) relative to the LIVE Root Key + tune +
        // fine (never the kRootNote constant). 1.0 ⇒ the note that matches rootKey
        // plays at the recorded pitch; higher notes read faster (up-transpose).
        voiceRate = std::pow (2.0,
            (double) (midiNote - params.rootKey + params.tune + params.fine * 0.01) / 12.0);

        // Read direction + start position. reverse seeds dir=-1 and starts the head
        // at endSamp (RESEARCH P3.3 — reverse negates the base increment independent
        // of loop direction; ping-pong then reflects from there). Forward plays from
        // startSamp. dir is mutated only by the ping-pong reflect in renderNextBlock.
        dir     = params.reverse ? -1 : +1;
        readPos = params.reverse ? (double) params.endSamp : (double) params.startSamp;

        // Stretch time axis seeds at the SAME read start as readPos (RESEARCH P6.5 —
        // no jump at note-on). Harmless when Repitch is latched (timePos is only read
        // by the Stretch path). The time axis advances at 1x realtime in render.
        timePos = readPos;

        // pitchMode is LATCHED at note-on (RESEARCH P6.5 — a mid-note switch would
        // truncate in-flight grain windows / click). Repitch (continuous read) and
        // Stretch (SOLA overlap-add) fork ONLY the read head; the tail is shared.
        latchedPitchMode = params.pitchMode;

        // Stretch (SOLA): reset the grain pool and arm the scheduler to fire the FIRST
        // grain on sample 0 (samplesUntilNextGrain=0 => --<=0 immediately), so the
        // time axis produces output from the very first sample with no startup gap
        // (RESEARCH P6.5). A stolen voice thus never carries stale grains into a new note.
        if (latchedPitchMode == 1)
        {
            for (auto& g : grains) g.active = false;
            nextGrain             = 0;
            samplesUntilNextGrain = 0;
        }

        // Vintage sample&hold reset (RESEARCH P4.2). Prime shPhase=1 so the FIRST
        // vintage-on sample latches immediately (no startup decimation gap).
        shPhase = 1.0f;
        shHeld  = 0.0f;

        // Clear the per-voice filter state so a stolen voice carries no previous tail.
        filt.reset();
        lastAmpEnv = 0.0f;

        // Velocity sensitivity blend (net-new, RESEARCH §8): v=0 ⇒ level-independent
        // of velocity (1.0); v=1 ⇒ full velocity; default 50 % ⇒ 0.5 + 0.5·velocity.
        const float v = juce::jlimit (0.0f, 1.0f, params.velToAmp * 0.01f);
        velLevel = (1.0f - v) + v * juce::jlimit (0.0f, 1.0f, velocity);

        // AA one-pole: engage only for up-transposed notes (voiceRate > 1). The
        // cutoff fc = 0.5·fs/rate and its smoothing coefficient depend only on the
        // (per-note-constant) rate, so both are computed ONCE here — no per-sample
        // std::exp in the render loop (RESEARCH §2.3 / DSP-02). State primed to the
        // first read sample so the filter starts settled (no attack transient).
        aaEngaged = (voiceRate > 1.0);
        aaCoeff   = aaEngaged
                  ? 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                           * (0.5f * (float) fs / (float) voiceRate) / (float) fs)
                  : 0.0f;
        aaState   = readSourceLagrange (sourcePtr, sourceLen, (float) readPos);

        ampEnv.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();                 // release tail (click-free note-off)
        }
        else
        {
            clearCurrentNote();
            ampEnv.reset();                   // hard stop (voice steal)
        }
    }

    void pitchWheelMoved (int) override {}    // no bend in Phase 2.1
    void controllerMoved (int, int) override {}

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        // Voice lifetime is keyed on the amp envelope (a finished release frees it).
        if (! ampEnv.isActive())
            return;

        // No usable source / empty region → free the voice (never read OOB).
        if (sourcePtr == nullptr || sourceLen <= 0 || params.endSamp <= params.startSamp)
        {
            ampEnv.reset();
            clearCurrentNote();
            return;
        }

        const double fs    = getSampleRate();
        const int    numCh = out.getNumChannels();
        const float* src   = sourcePtr;
        const int    len   = sourceLen;

        // --- Loop config (live per block; bounds already %-of-region + zero-cross
        //     snapped processor-side). A degenerate loop (loopLen<=1) falls back to
        //     the one-shot path. -----------------------------------------------------
        const int  loopMode  = params.loopMode;                 // 0 off / 1 fwd / 2 ping-pong
        const int  loopStart = params.loopStartSamp;
        const int  loopEnd   = params.loopEndSamp;
        const int  loopLen   = loopEnd - loopStart;
        const int  xfade     = (loopLen > 1) ? juce::jlimit (0, loopLen / 2, params.xfadeSamp) : 0;
        const bool validLoop = (loopMode != 0 && loopLen > 1);

        // --- Region-end declick (one-shot only) — raised-cosine VCA tail (RESEARCH
        //     P2). The 0.2 s amp release is UNREACHABLE from the region-end path
        //     (ampEnv.reset() idles instantly), so a dedicated ≤5 ms ramp is required;
        //     do NOT widen ampRelease. ----------------------------------------------
        const int endRampSamp = juce::jmin ((params.endSamp - params.startSamp) / 4,
                                            (int) std::floor (0.005 * fs));

        // --- Vintage coefficients — precompute per block (RESEARCH P4). Both stages
        //     gate on vintage>0 → bit-for-bit clean at 0 (acceptance DSP-04). -------
        const float vintage   = params.vintage;
        const bool  vintageOn  = (vintage > 0.0f);
        const float shInc      = vintageOn ? juce::jmap (vintage, 0.0f, 100.0f, (float) fs, 3000.0f) / (float) fs
                                           : 0.0f;                                   // S&H rate / fs
        const float bits       = vintageOn ? juce::jmap (vintage, 0.0f, 100.0f, 16.0f, 8.0f) : 16.0f;
        const float qLevel     = vintageOn ? std::exp2 (bits - 1.0f) : 1.0f;        // = L/2
        const float qInv       = vintageOn ? 1.0f / qLevel : 1.0f;

        // --- Filter: push the processor-smoothed cutoff/Q ONCE per block (one tan
        //     recompute — never per sample). setResonance asserts >0 (RESEARCH P5). -
        filt.setCutoffFrequency (juce::jlimit (20.0f, 0.45f * (float) fs, params.filterCutoff));
        filt.setResonance       (params.filterQ > 0.0f ? params.filterQ : 0.707f);

        // --- Stretch SOLA per-block constants (RESEARCH P6.3). Fixed 60 ms grain
        //     (kStretchGrainMs) → lenSamp; hop = grainSize/2 ⇒ 2× Hann overlap (COLA,
        //     already unity-gain → NO √overlap normalizer). Cheap; computed even on the
        //     Repitch path (only consumed when latchedPitchMode==1). -----------------
        const float lenSamp  = juce::jmax (2.0f,
            (float) OSimpleSamplerAudioProcessor::kStretchGrainMs * 0.001f * (float) fs);
        const float phaseInc = 1.0f / lenSamp;

        for (int i = 0; i < numSamples; ++i)
        {
            // ================================================================
            // SOURCE-SAMPLE GENERATION — forks on the note-on-latched pitchMode
            // (RESEARCH P1 / P6). Repitch = one continuous read head + voice-level
            // AA; Stretch = SOLA overlap-add of the grain pool with per-grain AA.
            // Either way produces a single mono pre-tail sample `s`.
            // ================================================================
            float s;

            if (latchedPitchMode == 1)
            {
                // ---- STRETCH (SOLA, RESEARCH P6.2/P6.3) -----------------------
                // Scheduler: fire a grain when the fixed hop elapses. hop =
                // grainSize/2 ⇒ 2× Hann overlap, NO jitter (the spray/density
                // machinery of O-simpleGrain is intentionally absent). Each grain
                // captures the voice-local TIME axis (timePos) and rides the key
                // rate (voiceRate); duration is preserved because timePos advances
                // at 1× realtime in the advance section, independent of pitch.
                if (--samplesUntilNextGrain <= 0)
                {
                    spawnGrainSOLA (lenSamp, phaseInc);
                    samplesUntilNextGrain = juce::jmax (1, (int) (lenSamp * 0.5f));
                }

                // Overlap-add the active grains. The per-grain AA one-pole is INSIDE
                // the sum (there is NO voice-level aaOnePole in Stretch). The Hann
                // envelope comes from the processor-owned shared LUT (shape 4); null
                // → 0 (silent until the LUT pointer is wired). DROP the √overlap
                // normalizer — a 2× Hann overlap-add is already unity-gain (COLA).
                s = 0.0f;
                for (auto& g : grains)
                {
                    if (! g.active)
                        continue;

                    const float env = (windowLuts != nullptr) ? windowLuts->read (4, g.phase) : 0.0f;
                    float gs = readSourceLagrange (src, len, g.readPos);
                    gs = aaOnePole (gs, g.aaCoeff, g.aaEngaged, g.aaState);   // per-grain band-limit
                    s += gs * env;

                    g.readPos += g.rate;          // grain reads forward at the key rate
                    g.phase   += g.phaseInc;
                    ++g.age;
                    if (g.phase >= 1.0f)
                        g.active = false;          // grain done (Hann fully traversed)
                }
            }
            else
            {
                // ---- REPITCH (2.2a, unchanged) — continuous read + loop seam ---
                if (validLoop && loopMode == 1)
                {
                    // Forward loop (PORT of the O-MicrotonalSampler dual-head equal-power
                    // crossfade, RESEARCH P3.2) generalized to xfadeSamp. DEVIATION from
                    // the literal port: the incoming head reads (readPos − loopLen) — the
                    // PRE-loop content — NOT (readPos − loopLen + xfade). With +xfade the
                    // seam is discontinuous by ~xfade samples (fine for the shipped fixed
                    // 8-sample fade, but the 0–500 ms crossfade here must be sample-
                    // continuous at the wrap — incoming(loopEnd⁻)=read(loopStart) requires
                    // offset 0). Direction-aware so reverse (dir<0) loops backward.
                    if (dir > 0)
                    {
                        const double fadeStart = (double) loopEnd - xfade;
                        if (xfade > 0 && readPos >= fadeStart)
                        {
                            const float x = (float) ((readPos - fadeStart) / (double) xfade);
                            const auto  w = equalPowerWeights (x);
                            const float a = readSourceLagrange (src, len, (float) readPos);
                            const float b = readSourceLagrange (src, len, (float) (readPos - loopLen));
                            s = a * w.first + b * w.second;
                        }
                        else
                            s = readSourceLagrange (src, len, (float) readPos);
                    }
                    else
                    {
                        const double fadeStart = (double) loopStart + xfade;
                        if (xfade > 0 && readPos <= fadeStart)
                        {
                            const float x = (float) ((fadeStart - readPos) / (double) xfade);
                            const auto  w = equalPowerWeights (x);
                            const float a = readSourceLagrange (src, len, (float) readPos);
                            const float b = readSourceLagrange (src, len, (float) (readPos + loopLen));
                            s = a * w.first + b * w.second;
                        }
                        else
                            s = readSourceLagrange (src, len, (float) readPos);
                    }
                }
                else if (validLoop && loopMode == 2)
                {
                    // Ping-pong (NET-NEW, RESEARCH P3.3) — reflect at the bounds; soften
                    // the turnaround by crossfading the reflected head (reads the opposite
                    // direction: mirror of readPos across the near boundary).
                    s = readSourceLagrange (src, len, (float) readPos);
                    if (xfade > 0)
                    {
                        if (dir > 0 && readPos >= (double) loopEnd - xfade)
                        {
                            const float x = (float) ((readPos - ((double) loopEnd - xfade)) / (double) xfade);
                            const auto  w = equalPowerWeights (x);
                            const float b = readSourceLagrange (src, len, (float) (2.0 * (double) loopEnd - readPos));
                            s = s * w.first + b * w.second;
                        }
                        else if (dir < 0 && readPos <= (double) loopStart + xfade)
                        {
                            const float x = (float) (((double) loopStart + xfade - readPos) / (double) xfade);
                            const auto  w = equalPowerWeights (x);
                            const float b = readSourceLagrange (src, len, (float) (2.0 * (double) loopStart - readPos));
                            s = s * w.first + b * w.second;
                        }
                    }
                }
                else
                {
                    // One-shot (no loop) — plain Repitch read.
                    s = readSourceLagrange (src, len, (float) readPos);
                }

                // Anti-alias one-pole (band-limits up-pitch). Its state carries ACROSS
                // the loop wrap — that is why the wrap subtracts loopLen (preserves
                // sub-sample phase) rather than hard-jumping. (Stretch does its AA
                // per-grain inside the overlap-add sum, so this is Repitch-only.)
                s = aaOnePole (s, aaCoeff, aaEngaged, aaState);
            }

            // ================================================================
            // SHARED DOWNSTREAM TAIL — identical for Repitch and Stretch (the only
            // fork is the read head that produced `s`; RESEARCH P1 / P6.6):
            // Vintage → Filter → region-end declick → VCA → addSample.
            // ================================================================

            // ---- Vintage: S&H decimate → bit-crush (bit-clean bypass at 0).
            //      Order = decimate then quantize (RESEARCH P4). ----------------
            if (vintageOn)
            {
                shPhase += shInc;
                if (shPhase >= 1.0f) { shPhase -= 1.0f; shHeld = s; }
                s = shHeld;                                   // sample & hold (decimate)
                s = std::round (s * qLevel) * qInv;           // bit-crush (verbatim idiom)
            }

            // ---- Resonant LP filter — post-Vintage, pre-VCA (RESEARCH P5). -----
            s = filt.processSample (0, s);

            // ---- Region-end declick (one-shot only): raised-cosine VCA taper.
            //      distToEnd is measured on the LIVE read axis — readPos for Repitch,
            //      timePos for Stretch (RESEARCH P2 / P6.6). Reverse terminates at
            //      startSamp → taper distance from the start instead. ------------
            float endRamp = 1.0f;
            if (! validLoop && endRampSamp > 0)
            {
                const double pos       = (latchedPitchMode == 1) ? timePos : readPos;
                const double distToEnd = (dir > 0) ? ((double) params.endSamp - pos)
                                                   : (pos - (double) params.startSamp);
                if (distToEnd < (double) endRampSamp)
                    endRamp = (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi
                                       * juce::jmax (0.0, distToEnd) / (double) endRampSamp));
            }

            // ---- VCA: amp ADSR × velocity-sensitivity level × end-ramp. -------
            const float eAmp = ampEnv.getNextSample();
            lastAmpEnv = eAmp;                                // lead-voice display (Task 8)
            float outv = s * eAmp * velLevel * endRamp;
            if (! std::isfinite (outv)) outv = 0.0f;

            // Mono source → duplicated across all output channels (centred image).
            for (int ch = 0; ch < numCh; ++ch)
                out.addSample (ch, startSample + i, outv);

            // ================================================================
            // READ-HEAD ADVANCE — mode-specific axis (RESEARCH P6.2). Repitch
            // advances readPos by voiceRate (pitch+time coupled); Stretch advances
            // timePos at 1× realtime (duration preserved, pitch tracks the key via
            // the per-grain rate). Loop-wrap / ping-pong reflect / one-shot-end act
            // on whichever axis is live, with identical structure.
            // ================================================================
            if (latchedPitchMode == 1)
            {
                // The TIME axis is what loops / reflects / ends in Stretch; the grains
                // ride the key rate independently. NO dual-head crossfade at the seam —
                // the 2× Hann grain overlap already smooths the wrap (RESEARCH P6.6).
                timePos += 1.0 * (double) dir;

                if (validLoop && loopMode == 1)
                {
                    if (dir > 0) { while (timePos >= (double) loopEnd)   timePos -= (double) loopLen; }
                    else         { while (timePos <  (double) loopStart) timePos += (double) loopLen; }
                }
                else if (validLoop && loopMode == 2)
                {
                    if (dir > 0 && timePos >= (double) loopEnd)
                    {
                        timePos = 2.0 * (double) loopEnd - timePos;   dir = -1;
                    }
                    else if (dir < 0 && timePos <= (double) loopStart)
                    {
                        timePos = 2.0 * (double) loopStart - timePos; dir = +1;
                    }
                }
                else
                {
                    // One-shot end on the time axis: fwd stops at endSamp, rev at startSamp.
                    if ((dir > 0 && timePos >= (double) params.endSamp)
                        || (dir < 0 && timePos <= (double) params.startSamp))
                    {
                        ampEnv.reset();
                        break;
                    }
                }
            }
            else
            {
                // ---- REPITCH (2.2a, unchanged) advance / wrap / reflect / end ----
                readPos += voiceRate * (double) dir;

                if (validLoop && loopMode == 1)
                {
                    // Wrap by subtracting/adding loopLen (preserve fractional phase).
                    if (dir > 0) { while (readPos >= (double) loopEnd)   readPos -= (double) loopLen; }
                    else         { while (readPos <  (double) loopStart) readPos += (double) loopLen; }
                }
                else if (validLoop && loopMode == 2)
                {
                    // Reflect at the bounds and flip direction.
                    if (dir > 0 && readPos >= (double) loopEnd)
                    {
                        readPos = 2.0 * (double) loopEnd - readPos;   dir = -1;
                    }
                    else if (dir < 0 && readPos <= (double) loopStart)
                    {
                        readPos = 2.0 * (double) loopStart - readPos; dir = +1;
                    }
                }
                else
                {
                    // One-shot end: forward stops at endSamp, reverse stops at startSamp.
                    if ((dir > 0 && readPos >= (double) params.endSamp)
                        || (dir < 0 && readPos <= (double) params.startSamp))
                    {
                        ampEnv.reset();
                        break;
                    }
                }
            }
        }

        // Free the voice once the amp envelope has finished (release tail done, or
        // the one-shot region-end reset above).
        if (! ampEnv.isActive())
            clearCurrentNote();
    }

    //==========================================================================
    // Lead-voice display reporting (read by the processor → atomics → Stage-3
    // filter curve). lastAmpEnv is the most recent amp-env value rendered this block.
    bool  isSounding()   const noexcept { return ampEnv.isActive(); }
    float getAmpEnvVal() const noexcept { return lastAmpEnv; }

    // Phase 2.3 — live playhead on the read axis, NORMALIZED to [0,1] over the
    // current [startSamp,endSamp) region. Reports timePos for Stretch (the latched
    // time axis) and readPos for Repitch — i.e. whichever axis the loop / one-shot
    // end acts on (RESEARCH §12). The processor picks the loudest-active voice's
    // value once per block and publishes it to displayPlayhead. A degenerate region
    // (span ≤ 0) reports 0. Const + branch-only — RT-safe (called from processBlock).
    float getPlayheadPos() const noexcept
    {
        const double pos  = (latchedPitchMode == 1) ? timePos : readPos;
        const double span = (double) params.endSamp - (double) params.startSamp;
        if (span <= 0.0)
            return 0.0f;
        return (float) juce::jlimit (0.0, 1.0, (pos - (double) params.startSamp) / span);
    }

private:
    //==========================================================================
    // Random-access fractional read into the source (clamp at bounds — a read off
    // the region/loop boundary saturates to the edge sample, no OOB). Ported
    // verbatim from O-simpleGrain/Source/GrainVoice.h (RESEARCH §2.2).
    static float readSourceLagrange (const float* src, int len, float pos) noexcept
    {
        if (src == nullptr || len <= 0)
            return 0.0f;

        const int   i0   = (int) pos;
        const float frac = pos - (float) i0;
        const int   im1  = juce::jlimit (0, len - 1, i0 - 1);
        const int   ip0  = juce::jlimit (0, len - 1, i0);
        const int   ip1  = juce::jlimit (0, len - 1, i0 + 1);
        const int   ip2  = juce::jlimit (0, len - 1, i0 + 2);
        return lagrangeInterpolate (src[im1], src[ip0], src[ip1], src[ip2], frac);
    }

    // Anti-aliasing one-pole (RESEARCH §2.3 / DSP-02). Band-limits up-transposed
    // reads before the fractional read crosses Nyquist. The cutoff, its smoothing
    // coefficient, and the engage decision are precomputed ON note-on (constant per
    // note — see startNote), so this hot-path helper is one branch + one
    // multiply-add, NO transcendental. When not engaged the read passes through and
    // `state` is kept coherent for a later engage edge. Ported verbatim from
    // O-simpleGrain/Source/GrainVoice.h.
    static float aaOnePole (float x, float coeff, bool engaged, float& state) noexcept
    {
        if (! engaged)
        {
            state = x;                        // keep state coherent for the bypass→engage edge
            return x;
        }

        state += coeff * (x - state);         // one-pole y += g*(x - y), g precomputed
        return state;
    }

    // Equal-power (constant-energy) crossfade weights {fade-out, fade-in} for x∈[0,1]
    // — {cos, sin} of the quarter-turn. Used at the loop seam / ping-pong turnaround.
    // Ported verbatim from O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:31
    // (same primitive as O-simpleGrain's pan law). RESEARCH P3.1.
    static std::pair<float, float> equalPowerWeights (float x) noexcept
    {
        const float t = juce::jlimit (0.0f, 1.0f, x) * juce::MathConstants<float>::halfPi;
        return { std::cos (t), std::sin (t) };
    }

    //==========================================================================
    // Spawn a SOLA grain (RESEARCH P6.2/P6.3 — adapted from O-simpleGrain spawnGrain
    // with ALL spray/scatter machinery stripped). Find an inactive slot, else steal
    // the oldest (max age — already fading); bounded by kMaxGrainsPerVoice so it NEVER
    // allocates. The grain captures the voice-local TIME axis (g.readPos = timePos) and
    // rides the key rate (g.rate = voiceRate) — no grainPitch/pitchSpray (this plugin
    // has neither, and there is no per-voice RNG). Per-grain AA coeff (fc = 0.5·fs/rate)
    // is computed ONCE here from g.rate — the one std::exp on spawn is O(1), no alloc
    // (the same accepted cost as O-simpleGrain). Hann (shape 4) is the only SOLA window.
    void spawnGrainSOLA (float lenSamp, float phaseInc) noexcept
    {
        constexpr int N = OSimpleSamplerAudioProcessor::kMaxGrainsPerVoice;

        int target = -1, oldest = 0, maxAge = -1;
        for (int k = 0; k < N; ++k)
        {
            const int idx = (nextGrain + k) % N;
            if (! grains[(size_t) idx].active) { target = idx; break; }
            if (grains[(size_t) idx].age > maxAge) { maxAge = grains[(size_t) idx].age; oldest = idx; }
        }
        if (target < 0)
            target = oldest;                      // steal oldest (already fading out)

        auto& g = grains[(size_t) target];
        g.active        = true;
        g.age           = 0;
        g.phase         = 0.0f;
        g.phaseInc      = phaseInc;               // = 1.0f / lenSamp
        g.lengthSamples = lenSamp;
        g.shape         = 4;                       // Hann (the only SOLA window)

        // SOLA: read start = the voice-local TIME axis (NOT playhead + spray); read
        // rate = the key resample (NOT grainPitch/pitchSpray). Zero ALL spray.
        g.readPos = (float) timePos;
        g.rate    = (float) voiceRate;

        // Mono-centred. pan/panL/panR are vestigial here — the overlap-add output
        // ignores them (a single mono accumulated `s`); kept coherent for any viz tap.
        g.pan  = 0.5f;
        g.panL = 0.70710677f;
        g.panR = 0.70710677f;

        // Per-grain AA one-pole: engage only for up-transposed grains (rate > 1); the
        // cutoff and smoothing coeff depend only on the (per-grain-constant) rate, so
        // both are computed ONCE here — no per-sample std::exp in the overlap-add loop.
        const float fs = (float) getSampleRate();
        g.aaEngaged = (g.rate > 1.0f);
        g.aaCoeff   = g.aaEngaged
                    ? 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                             * (0.5f * fs / g.rate) / fs)
                    : 0.0f;
        // Prime the AA state to the first read sample so the filter starts settled.
        g.aaState   = readSourceLagrange (sourcePtr, sourceLen, g.readPos);

        nextGrain = (target + 1) % N;
    }

    //==========================================================================
    juce::ADSR         ampEnv;
    SamplerVoiceParams params;

    // Source snapshot (raw pointer set per block; processor holds the shared_ptr).
    const float* sourcePtr = nullptr;
    int          sourceLen = 0;

    double voiceRate = 1.0;     // Repitch read-increment ratio (keyRatio)
    double readPos   = 0.0;     // current fractional source position
    float  velLevel  = 1.0f;    // velocity-sensitivity-blended amplitude

    // AA one-pole state (engage + coeff computed once per note in startNote).
    bool   aaEngaged = false;
    float  aaCoeff   = 0.0f;
    float  aaState   = 0.0f;

    //--- Phase 2.2a state ---------------------------------------------------
    int    dir              = 1;        // read direction (+1 fwd, -1 reverse / ping-pong leg)
    int    latchedPitchMode = 0;        // pitchMode latched at note-on (0 Repitch / 1 Stretch)
    float  shPhase          = 0.0f;     // Vintage sample&hold phase accumulator
    float  shHeld           = 0.0f;     // Vintage sample&hold held value
    float  lastAmpEnv       = 0.0f;     // last rendered amp-env value (lead-voice display)

    // Per-voice resonant LP (RESEARCH P5). 16× is CPU-light; coeff updated once/block.
    juce::dsp::StateVariableTPTFilter<float> filt;

    //--- Phase 2.2b — Stretch SOLA state (RESEARCH P6) ----------------------
    // Fixed grain pool (kMaxGrainsPerVoice=4 = 2× headroom over the 2 active grains
    // at 2× overlap). std::array → no heap; spawnGrainSOLA's steal-oldest never allocates.
    std::array<Grain, OSimpleSamplerAudioProcessor::kMaxGrainsPerVoice> grains {};
    double timePos              = 0.0;   // voice-local TIME axis (advances at 1× realtime)
    int    samplesUntilNextGrain = 0;    // SOLA scheduler countdown (hop = grainSize/2)
    int    nextGrain             = 0;    // round-robin pool cursor for spawn search

    // Shared window-LUT table (processor-owned; built ONCE off the audio thread, shared
    // read-only by all 16 voices). RT-CRITICAL: never per-voice / never built in render.
    const WindowLuts* windowLuts = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleVoice)
};
