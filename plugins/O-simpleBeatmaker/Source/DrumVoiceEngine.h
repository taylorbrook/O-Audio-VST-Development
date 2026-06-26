/*
  ==============================================================================

    O-simpleBeatmaker - DrumVoiceEngine (6 synthesized 808/909-lineage voices)

    Custom per-voice structs (NOT juce::Synthesiser — these are one-shot
    percussion with no note-off semantics, monophonic-retrigger with a tail).
    Each voice exposes a uniform interface:

        prepareToPlay (double fs)
        setParams (float tuneSt, float decay01, float tone01, float levelDb)
        trigger   (juce::uint8 velocity)
        render    (float* L, float* R, int start, int n)   // ADDS into the span

    Names are suffixed *Voice to avoid colliding with the existing
    `enum Voice { Kick, ... }` in PluginProcessor.h and with juce:: types
    (Random/Synthesiser/SynthesiserVoice). HatVoice serves BOTH the closed (42)
    and open (46) hat from a shared filtered-noise source with separate envelopes
    plus a choke (closed fires -> open tail fast-fades).

    The per-voice decay-ms mapping lives HERE (kick boom vs hat tick differ by an
    order of magnitude), NOT in the APVTS param range (stored 0-1).

    RT-safety: exponential envelopes (env *= coef), flush below 1e-6 to kill
    denormal tails; noise from a pre-seeded inline xorshift (seeded in
    prepareToPlay, never on the audio thread).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>
#include "BeatmakerIDs.h"
#include "fastSine.h"

namespace OSimpleBeatmaker
{
    //==========================================================================
    // Tiny RT-safe white-noise source (inline xorshift32). Seeded once.
    struct NoiseGen
    {
        juce::uint32 state = 0x12345678u;

        void seed (juce::uint32 s) noexcept { state = (s == 0u ? 0x1u : s); }

        // Uniform white noise in [-1, 1).
        float next() noexcept
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return ((float) state / (float) 0xFFFFFFFFu) * 2.0f - 1.0f;
        }
    };

    inline float dbToGain (float db) noexcept
    {
        return db <= -59.5f ? 0.0f : juce::Decibels::decibelsToGain (db, -60.0f);
    }

    inline float coefForSeconds (double seconds, double fs) noexcept
    {
        seconds = juce::jmax (1.0e-4, seconds);
        return (float) std::exp (-1.0 / (seconds * fs));
    }

    constexpr float kEnvFloor = 1.0e-6f;

    //==========================================================================
    // KICK (808): sine body + fast exp pitch env (+ ~1-2 oct) + amp env + click.
    struct KickVoice
    {
        void prepareToPlay (double fsIn) noexcept { fs = fsIn; }

        void setParams (float tuneSt, float decay01, float tone01, float levelDb) noexcept
        {
            fBase    = 50.0f * std::pow (2.0f, tuneSt / 12.0f);          // ~50 Hz +/- 1 oct
            const double decaySec = 0.05 + (double) decay01 * 1.15;      // 50 ms .. 1.2 s boom
            ampCoef  = coefForSeconds (decaySec, fs);
            pitchCoef = coefForSeconds (0.030, fs);                      // 30 ms pitch sweep
            pitchAmtHz = fBase * (0.5f + tone01 * 2.5f);                 // tone -> pitch-env amount
            clickCoef = coefForSeconds (0.0015, fs);                    // ~1.5 ms click
            clickLevel = tone01 * 0.5f;
            level    = dbToGain (levelDb);
        }

        void trigger (juce::uint8 velocity) noexcept
        {
            vel01   = (float) velocity / 127.0f;
            ampEnv  = 1.0f;
            pitchEnv = 1.0f;
            clickEnv = 1.0f;
            active  = true;
        }

        void render (float* L, float* R, int start, int n) noexcept
        {
            if (! active) return;
            const float velGain = 0.25f + 0.75f * vel01;
            const float twoPi = juce::MathConstants<float>::twoPi;
            for (int i = 0; i < n; ++i)
            {
                const float fInst = fBase + pitchEnv * pitchAmtHz;
                phase += twoPi * fInst / (float) fs;
                float body = fastSine (phase) * ampEnv;
                float click = noise.next() * clickEnv * clickLevel;
                const float s = (body + click) * level * velGain;
                L[start + i] += s;
                R[start + i] += s;
                ampEnv   *= ampCoef;
                pitchEnv *= pitchCoef;
                clickEnv *= clickCoef;
            }
            if (ampEnv < kEnvFloor) { ampEnv = 0.0f; active = false; }
        }

        double fs = 44100.0;
        float  fBase = 50.0f, pitchAmtHz = 60.0f, level = 1.0f, clickLevel = 0.25f;
        float  ampCoef = 0.0f, pitchCoef = 0.0f, clickCoef = 0.0f;
        float  phase = 0.0f, ampEnv = 0.0f, pitchEnv = 0.0f, clickEnv = 0.0f, vel01 = 1.0f;
        bool   active = false;
        NoiseGen noise;
    };

    //==========================================================================
    // TOM (808): pitched sine body + mild pitch+amp env.
    struct TomVoice
    {
        void prepareToPlay (double fsIn) noexcept { fs = fsIn; }

        void setParams (float tuneSt, float decay01, float tone01, float levelDb) noexcept
        {
            fBase    = 120.0f * std::pow (2.0f, tuneSt / 12.0f);
            const double decaySec = 0.08 + (double) decay01 * 0.6;
            ampCoef  = coefForSeconds (decaySec, fs);
            pitchCoef = coefForSeconds (0.045, fs);
            pitchAmtHz = fBase * (0.2f + tone01 * 1.0f);
            level    = dbToGain (levelDb);
        }

        void trigger (juce::uint8 velocity) noexcept
        {
            vel01 = (float) velocity / 127.0f;
            ampEnv = 1.0f; pitchEnv = 1.0f; active = true;
        }

        void render (float* L, float* R, int start, int n) noexcept
        {
            if (! active) return;
            const float velGain = 0.25f + 0.75f * vel01;
            const float twoPi = juce::MathConstants<float>::twoPi;
            for (int i = 0; i < n; ++i)
            {
                const float fInst = fBase + pitchEnv * pitchAmtHz;
                phase += twoPi * fInst / (float) fs;
                const float s = fastSine (phase) * ampEnv * level * velGain;
                L[start + i] += s;
                R[start + i] += s;
                ampEnv   *= ampCoef;
                pitchEnv *= pitchCoef;
            }
            if (ampEnv < kEnvFloor) { ampEnv = 0.0f; active = false; }
        }

        double fs = 44100.0;
        float  fBase = 120.0f, pitchAmtHz = 60.0f, level = 1.0f;
        float  ampCoef = 0.0f, pitchCoef = 0.0f;
        float  phase = 0.0f, ampEnv = 0.0f, pitchEnv = 0.0f, vel01 = 1.0f;
        bool   active = false;
    };

    //==========================================================================
    // SNARE (808/909 hybrid): two detuned tonal oscs + band-passed noise burst.
    // tone = body <-> noise balance.
    struct SnareVoice
    {
        void prepareToPlay (double fsIn) noexcept
        {
            fs = fsIn;
            bp.prepare ({ fsIn, 512u, 1u });
            bp.setType (juce::dsp::StateVariableTPTFilter<float>::Type::bandpass);
            bp.setResonance (0.7f);
            bp.reset();
        }

        void setParams (float tuneSt, float decay01, float tone01, float levelDb) noexcept
        {
            const float mul = std::pow (2.0f, tuneSt / 12.0f);
            f1 = 180.0f * mul;
            f2 = 330.0f * mul;
            const double bodyDecay  = 0.05 + (double) decay01 * 0.30;
            const double noiseDecay = 0.04 + (double) decay01 * 0.25;
            bodyCoef  = coefForSeconds (bodyDecay, fs);
            noiseCoef = coefForSeconds (noiseDecay, fs);
            noiseMix  = tone01;                 // 0 = pure body, 1 = pure noise
            bodyMix   = 1.0f - 0.6f * tone01;
            bp.setCutoffFrequency (juce::jlimit (200.0f, 8000.0f, 1500.0f + tone01 * 2500.0f));
            level     = dbToGain (levelDb);
        }

        void trigger (juce::uint8 velocity) noexcept
        {
            vel01 = (float) velocity / 127.0f;
            bodyEnv = 1.0f; noiseEnv = 1.0f; active = true;
        }

        void render (float* L, float* R, int start, int n) noexcept
        {
            if (! active) return;
            const float velGain = 0.25f + 0.75f * vel01;
            const float twoPi = juce::MathConstants<float>::twoPi;
            for (int i = 0; i < n; ++i)
            {
                ph1 += twoPi * f1 / (float) fs;
                ph2 += twoPi * f2 / (float) fs;
                const float body = (fastSine (ph1) + 0.7f * fastSine (ph2)) * bodyEnv * bodyMix;
                const float nz   = bp.processSample (0, noise.next()) * noiseEnv * noiseMix * 2.0f;
                const float s    = (body + nz) * level * velGain;
                L[start + i] += s;
                R[start + i] += s;
                bodyEnv  *= bodyCoef;
                noiseEnv *= noiseCoef;
            }
            if (bodyEnv < kEnvFloor && noiseEnv < kEnvFloor)
            { bodyEnv = noiseEnv = 0.0f; active = false; }
        }

        double fs = 44100.0;
        float  f1 = 180.0f, f2 = 330.0f, level = 1.0f, noiseMix = 0.5f, bodyMix = 0.7f;
        float  bodyCoef = 0.0f, noiseCoef = 0.0f;
        float  ph1 = 0.0f, ph2 = 0.0f, bodyEnv = 0.0f, noiseEnv = 0.0f, vel01 = 1.0f;
        bool   active = false;
        NoiseGen noise;
        juce::dsp::StateVariableTPTFilter<float> bp;
    };

    //==========================================================================
    // CLAP (808): 3-4 retriggered noise bursts ~10 ms apart -> band-pass +
    // a short diffuse tail. tone = burst spread vs tail.
    struct ClapVoice
    {
        void prepareToPlay (double fsIn) noexcept
        {
            fs = fsIn;
            bp.prepare ({ fsIn, 512u, 1u });
            bp.setType (juce::dsp::StateVariableTPTFilter<float>::Type::bandpass);
            bp.setResonance (0.8f);
            bp.reset();
        }

        void setParams (float /*tuneSt*/, float decay01, float tone01, float levelDb) noexcept
        {
            burstCoef = coefForSeconds (0.008, fs);                       // each burst ~8 ms
            const double tailSec = 0.05 + (double) decay01 * 0.35;
            tailCoef  = coefForSeconds (tailSec, fs);
            burstSpacing = (int) ((0.006 + (double) tone01 * 0.010) * fs); // 6..16 ms spacing
            bp.setCutoffFrequency (juce::jlimit (400.0f, 4000.0f, 900.0f + tone01 * 1500.0f));
            level     = dbToGain (levelDb);
        }

        void trigger (juce::uint8 velocity) noexcept
        {
            vel01 = (float) velocity / 127.0f;
            burstEnv = 1.0f; tailEnv = 1.0f; active = true;
            burstsRemaining = 3;
            samplesToNextBurst = burstSpacing;
        }

        void render (float* L, float* R, int start, int n) noexcept
        {
            if (! active) return;
            const float velGain = 0.25f + 0.75f * vel01;
            for (int i = 0; i < n; ++i)
            {
                if (burstsRemaining > 0)
                {
                    if (--samplesToNextBurst <= 0)
                    {
                        burstEnv = 1.0f;                   // re-seed burst env (no alloc)
                        --burstsRemaining;
                        samplesToNextBurst = burstSpacing;
                    }
                }
                const float src  = noise.next();
                const float band = bp.processSample (0, src);
                const float s = band * (burstEnv + 0.5f * tailEnv) * level * velGain;
                L[start + i] += s;
                R[start + i] += s;
                burstEnv *= burstCoef;
                tailEnv  *= tailCoef;
            }
            if (tailEnv < kEnvFloor && burstsRemaining == 0)
            { tailEnv = 0.0f; active = false; }
        }

        double fs = 44100.0;
        float  level = 1.0f, burstCoef = 0.0f, tailCoef = 0.0f;
        int    burstSpacing = 441, samplesToNextBurst = 0, burstsRemaining = 0;
        float  burstEnv = 0.0f, tailEnv = 0.0f, vel01 = 1.0f;
        bool   active = false;
        NoiseGen noise;
        juce::dsp::StateVariableTPTFilter<float> bp;
    };

    //==========================================================================
    // HAT (closed 42 + open 46): shared high-passed noise source, separate
    // envelopes. Closed = short decay; open = long decay. Closed trigger chokes
    // the open tail (fast ~3 ms release).
    struct HatVoice
    {
        void prepareToPlay (double fsIn) noexcept
        {
            fs = fsIn;
            hp.prepare ({ fsIn, 512u, 1u });
            hp.setType (juce::dsp::StateVariableTPTFilter<float>::Type::highpass);
            hp.reset();
            chokeCoef = coefForSeconds (0.003, fs);                      // ~3 ms choke
        }

        void setClosedParams (float tuneSt, float decay01, float tone01, float levelDb) noexcept
        {
            const float mul = std::pow (2.0f, tuneSt / 12.0f);
            cutoff = juce::jlimit (3000.0f, 12000.0f, 7000.0f * mul);
            const double dec = 0.015 + (double) decay01 * 0.08;          // 15..95 ms
            closedCoef = coefForSeconds (dec, fs);
            closedLevel = dbToGain (levelDb);
            brightness = 0.5f + tone01 * 0.5f;
            hp.setResonance (0.5f + tone01 * 0.6f);
            hp.setCutoffFrequency (cutoff);
        }

        void setOpenParams (float /*tuneSt*/, float decay01, float /*tone01*/, float levelDb) noexcept
        {
            const double dec = 0.15 + (double) decay01 * 0.55;          // 150..700 ms
            openDecayCoef = coefForSeconds (dec, fs);
            openLevel = dbToGain (levelDb);
        }

        void triggerClosed (juce::uint8 velocity) noexcept
        {
            closedVel = (float) velocity / 127.0f;
            closedEnv = 1.0f; active = true;
        }

        void triggerOpen (juce::uint8 velocity) noexcept
        {
            openVel = (float) velocity / 127.0f;
            openEnv = 1.0f; openCoef = openDecayCoef; active = true;
        }

        // Closed-hat trigger chokes the open-hat tail.
        void applyChoke() noexcept { openCoef = chokeCoef; }

        void render (float* L, float* R, int start, int n) noexcept
        {
            if (! active) return;
            for (int i = 0; i < n; ++i)
            {
                const float nz = hp.processSample (0, noise.next()) * brightness;
                const float amp = closedEnv * closedLevel * (0.25f + 0.75f * closedVel)
                                + openEnv   * openLevel   * (0.25f + 0.75f * openVel);
                const float s = nz * amp;
                L[start + i] += s;
                R[start + i] += s;
                closedEnv *= closedCoef;
                openEnv   *= openCoef;
            }
            if (closedEnv < kEnvFloor) closedEnv = 0.0f;
            if (openEnv   < kEnvFloor) openEnv   = 0.0f;
            if (closedEnv == 0.0f && openEnv == 0.0f) active = false;
        }

        double fs = 44100.0;
        float  cutoff = 7000.0f, brightness = 1.0f;
        float  closedCoef = 0.0f, openDecayCoef = 0.0f, openCoef = 0.0f, chokeCoef = 0.0f;
        float  closedLevel = 1.0f, openLevel = 1.0f;
        float  closedEnv = 0.0f, openEnv = 0.0f, closedVel = 1.0f, openVel = 1.0f;
        bool   active = false;
        NoiseGen noise;
        juce::dsp::StateVariableTPTFilter<float> hp;
    };

    //==========================================================================
    // 6-voice container in voice-row order:
    //   0 Kick, 1 Snare, 2 Clap, 3 ClosedHat, 4 OpenHat, 5 Tom
    // Closed/open hat share a single HatVoice.
    class DrumVoiceEngine
    {
    public:
        void prepareToPlay (double fs, int /*maxBlock*/) noexcept
        {
            kick.prepareToPlay (fs);
            snare.prepareToPlay (fs);
            clap.prepareToPlay (fs);
            hat.prepareToPlay (fs);
            tom.prepareToPlay (fs);

            // Deterministic, distinct noise seeds (pre-seeded; never on audio thread).
            kick.noise.seed (0xA1B2C3D4u);
            snare.noise.seed (0x5F6E7D8Cu);
            clap.noise.seed (0x13572468u);
            hat.noise.seed (0x9E8D7C6Bu);
        }

        // Per-voice param push from cached APVTS (index = Voice enum).
        void setParams (int voiceIndex, float tuneSt, float decay01, float tone01, float levelDb) noexcept
        {
            switch (voiceIndex)
            {
                case Kick:      kick.setParams (tuneSt, decay01, tone01, levelDb); break;
                case Snare:     snare.setParams (tuneSt, decay01, tone01, levelDb); break;
                case Clap:      clap.setParams (tuneSt, decay01, tone01, levelDb); break;
                case ClosedHat: hat.setClosedParams (tuneSt, decay01, tone01, levelDb); break;
                case OpenHat:   hat.setOpenParams (tuneSt, decay01, tone01, levelDb); break;
                case Tom:       tom.setParams (tuneSt, decay01, tone01, levelDb); break;
                default: break;
            }
        }

        void trigger (int voiceIndex, juce::uint8 velocity) noexcept
        {
            switch (voiceIndex)
            {
                case Kick:      kick.trigger (velocity); break;
                case Snare:     snare.trigger (velocity); break;
                case Clap:      clap.trigger (velocity); break;
                case ClosedHat: hat.applyChoke(); hat.triggerClosed (velocity); break;  // choke open tail
                case OpenHat:   hat.triggerOpen (velocity); break;
                case Tom:       tom.trigger (velocity); break;
                default: break;
            }
        }

        void renderAll (juce::AudioBuffer<float>& buffer, int start, int n) noexcept
        {
            if (n <= 0) return;
            float* L = buffer.getWritePointer (0);
            float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
            kick.render  (L, R, start, n);
            snare.render (L, R, start, n);
            clap.render  (L, R, start, n);
            hat.render   (L, R, start, n);
            tom.render   (L, R, start, n);
        }

    private:
        KickVoice  kick;
        SnareVoice snare;
        ClapVoice  clap;
        HatVoice   hat;
        TomVoice   tom;
    };
}
