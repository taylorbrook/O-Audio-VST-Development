/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - MediaNoise (v1.5.0, improvement brief item 4)

    The three per-family media-noise beds: what a failing machine sounds like
    in the GAPS between its failures.

    Before this the engine synthesised only EVENT-triggered artifacts —
    ArtifactSynth's pop, tick and chirp all fire at jump instants, and nothing
    at all ran between clock ticks. So a "dying media" patch was a clean
    signal punctuated by breakage, and the illusion collapsed the moment the
    events stopped. Real failing media has a continuous floor, and it is the
    floor, not the events, that carries the sense of a machine:

      * VinylBed  — pinked bearing rumble under a Poisson rain of surface
                    micro-ticks with a power-law amplitude distribution.
      * TapeBed   — decorrelated stereo hiss behind a gentle HF shelf-down,
                    riding the transport speed so a stop mutes it.
      * CodecBed  — deterministic 50/60 Hz mains hum plus two harmonics, with
                    Poisson crackle bursts inside the phone band.

    WHERE EACH BED IS INJECTED, AND WHY IT MATTERS
    ----------------------------------------------
    The tape and vinyl beds join the mono artifact bus (PluginProcessor step
    6), upstream of the packet stage — a lost packet must conceal the hiss
    along with the programme, because on real media they are the same signal.

    The codec bed CANNOT go there. CodecStage is a 300-3400 Hz phone chain,
    and 50 Hz injected in front of it is annihilated by the passband: the hum
    would be inaudible at every setting. It is added AFTER the codec (step
    8b), which is also where the physics puts it — mains hum is induced on the
    line, not recorded at the source.

    LEVELS, AND WHY THEY ARE SAMPLE-RATE INVARIANT
    ----------------------------------------------
    Filtering white noise to a fixed bandwidth in Hz gives an output power
    proportional to that bandwidth over fs, so a bed calibrated with a bare
    constant is quieter at 96 kHz than at 48 kHz. Every bed here therefore
    normalises its FIRST (and only white-fed) filter stage by that stage's
    exact analytic noise gain sqrt(a/(2-a)); everything downstream operates on
    an already-shaped signal whose spectrum is fixed in Hz, so its gain is a
    plain sample-rate-independent fraction folded into one measured constant.
    Probe M4 asserts 48 kHz vs 96 kHz agreement for all three beds.

    DETERMINISM
    -----------
    Each bed draws a fixed count per sample from its OWN RngBank stream, on
    its own sample schedule. That is block-size invariant: the interleave
    hazard is two subsystems SHARING a stream at different block-relative
    instants (pattern_rng_stream_interleave_blocksize), which is why the
    conditional extra draws taken when a tick or a crackle burst fires are
    also safe — they depend on the private stream's own position, never on a
    block boundary. Crush jitter and dither have drawn per-sample since 2.4.

    TRANSPARENCY
    ------------
    Every bed returns EXACTLY 0.0f whenever its ramped level is exactly 0 —
    the filters and the RNG schedule still run, so state stays continuous and
    the draw sequence is a pure function of the sample count, but nothing
    reaches the bus. All three parameters default to 0, so the FUNC-02
    bit-exact null and every v1.4.0 render are untouched.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/** Unity-RMS white from juce::Random::nextFloat().

    nextFloat() is uniform on [0, 1), so 2u - 1 is uniform on [-1, 1) with RMS
    1/sqrt(3). Scaling to unity RMS here — rather than carrying the 0.577
    around — is what lets every level constant below be read directly as a
    target RMS in dBFS. */
inline float unitRmsWhite (juce::Random& rng) noexcept
{
    return (rng.nextFloat() * 2.0f - 1.0f) * 1.73205081f;   // sqrt(3)
}

/** One-pole lowpass coefficient for a corner in Hz. */
inline float onePoleCoeff (double hz, double fs) noexcept
{
    return static_cast<float> (
        1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * hz / juce::jmax (1.0, fs)));
}

/** RMS gain of `y += a * (x - y)` for white input: sqrt(a / (2 - a)).

    Exact, and the whole reason the beds are sample-rate invariant — see the
    header note. */
inline float onePoleNoiseGain (float a) noexcept
{
    return std::sqrt (a / juce::jmax (1.0e-9f, 2.0f - a));
}

//==============================================================================
/** Linear level ramp that lands EXACTLY on its target.

    Exactness is load-bearing, not tidiness: an epsilon residue at the bottom
    would leave a bed multiplying noise by 1e-9 forever instead of by exactly
    zero, and the plugin's bit-exact null (FUNC-02) would be gone at the
    default settings with nothing audible to show for it. WowFlutter's depth
    ramp lands the same way for the same reason.

    ~30 ms is short compared with WowFlutter's 3 s because this ramp's slope
    is amplitude, not pitch. */
class BedLevel
{
public:
    void prepare (double sampleRate, double rampSeconds) noexcept
    {
        step = 1.0f / static_cast<float> (juce::jmax (1.0, rampSeconds * sampleRate));
        reset();
    }

    void reset() noexcept
    {
        level  = 0.0f;
        target = 0.0f;
    }

    void setTarget (float t) noexcept { target = juce::jlimit (0.0f, 1.0f, t); }

    float next() noexcept
    {
        if (level < target)      level = juce::jmin (target, level + step);
        else if (level > target) level = juce::jmax (target, level - step);
        return level;
    }

private:
    float level  = 0.0f;
    float target = 0.0f;
    float step   = 1.0f;
};

//==============================================================================
/** Vinyl surface: bearing rumble + Poisson micro-ticks.

    MONO on purpose, and for a different reason than ArtifactSynth's "the
    failure is the player, not the channels": here it is literal mechanics.
    One platter bearing makes one rumble and one stylus rides one groove, so a
    tick is a single event, not two. (Tape hiss is the opposite case — two
    tracks, two independent noise sources — and TapeBed below is stereo.) */
class VinylBed
{
public:
    // Micro-tick density at VINYL_WEAR = 100%. Dense enough to read as a
    // continuous surface rather than as countable clicks.
    static constexpr double kMaxTicksPerSec = 140.0;

    // Power-law amplitude: amp = peak * u^3, u uniform. Many small, rare
    // large — the shape real surface noise has. Median tick lands at 1/8 of
    // the peak (-18 dB), 1% of ticks above 0.79 of it. Cubing a uniform is
    // BOUNDED, which an inverse-CDF Pareto draw would not be; nothing on the
    // audio thread should be able to draw an arbitrarily large impulse.
    static constexpr float  kTickPeak   = 0.05f;      // ~ -26 dBFS, loudest ticks
    static constexpr double kTickMinHz  = 2000.0;
    static constexpr double kTickMaxHz  = 6000.0;
    static constexpr float  kTickQ      = 0.9f;

    // Rough bandpass makeup, tuned by harness render (probe M1) — the same
    // approach ArtifactSynth::triggerTick takes for the CD tick.
    static constexpr float  kTickMakeup = 4.0f;

    // Rumble chain: one pinking stage, then two 55 Hz poles. -18 dB/oct above
    // the corner, which is what puts it under the programme rather than in it.
    static constexpr double kRumblePinkHz = 120.0;
    static constexpr double kRumbleHz     = 55.0;

    // Target rumble RMS at VINYL_WEAR = 100%: -42 dBFS, the unweighted region
    // a worn consumer deck actually sits in.
    static constexpr float  kRumbleFullRms = 0.00794f;

    // Combined gain of the two 55 Hz stages on the (already normalised,
    // sample-rate-invariant) output of the pinking stage. MEASURED by probe
    // N1, not derived: a cascade's noise gain has no convenient closed form,
    // and the per-stage normalisation above is only valid for the white-fed
    // stage — normalising the later stages the same way would reintroduce the
    // sample-rate dependence it exists to remove.
    static constexpr float  kRumbleCascadeGain = 0.4101f;

    // Once-per-revolution amplitude modulation. Bearing rumble is eccentric
    // by construction, and this is what separates "low noise" from "a
    // turntable" — it beats at the platter rate, which VINYL_RPM already
    // knows. Stays strictly positive at this depth, so it never inverts.
    static constexpr float  kRumbleAmDepth = 0.35f;

    void prepare (double sampleRate)
    {
        fs = sampleRate;

        aPink = onePoleCoeff (kRumblePinkHz, fs);
        aRum  = onePoleCoeff (kRumbleHz,     fs);

        // Normalise ONLY the white-fed stage (see the header note).
        pinkNorm = 1.0f / juce::jmax (1.0e-9f, onePoleNoiseGain (aPink));

        rumbleGain = kRumbleFullRms / kRumbleCascadeGain;

        const juce::dsp::ProcessSpec spec { fs, 512u, 1u };
        tickFilter.prepare (spec);
        tickFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        tickFilter.setResonance (kTickQ);
        tickFilter.setCutoffFrequency (4000.0f);

        level.prepare (fs, kLevelRampSeconds);

        reset();
    }

    void reset() noexcept
    {
        tickFilter.reset();
        level.reset();

        pink = rum1 = rum2 = 0.0f;
        platterPhase = 0.0;
    }

    /** Per-block snapshot. `wear01` arrives already gated by VINYL_ENABLE —
        the caller passes 0 when the family is off, so the bed fades out
        rather than switching. */
    void setParams (float wear01, int rpmIndex) noexcept
    {
        const float wear = juce::jlimit (0.0f, 1.0f, wear01);

        // Wear drives BOTH the tick density and (through the ramp) the bed's
        // amplitude, which is what makes one knob read as "how worn": a
        // worn record is not just louder crackle, it is more of it.
        tickThreshold = static_cast<float> (kMaxTicksPerSec * wear / juce::jmax (1.0, fs));

        const double revsPerSec = (rpmIndex == 1 ? 45.0 : 100.0 / 3.0) / 60.0;
        platterInc = juce::MathConstants<double>::twoPi * revsPerSec / juce::jmax (1.0, fs);

        level.setTarget (wear);
    }

    /** Mono bed sample. Two unconditional draws (rumble excitation, then the
        tick schedule) plus two more only when a tick fires. */
    float renderSample (juce::Random& rng) noexcept
    {
        const float white = unitRmsWhite (rng);
        const float sched = rng.nextFloat();

        // Rumble: pinking stage (normalised) -> 55 Hz -> 55 Hz.
        pink += aPink * (white - pink);
        const float shaped = pink * pinkNorm;
        rum1 += aRum * (shaped - rum1);
        rum2 += aRum * (rum1   - rum2);

        platterPhase += platterInc;
        if (platterPhase >= juce::MathConstants<double>::twoPi)
            platterPhase -= juce::MathConstants<double>::twoPi;

        const float am     = 1.0f + kRumbleAmDepth * static_cast<float> (std::sin (platterPhase));
        const float rumble = rum2 * rumbleGain * am;

        // Poisson micro-tick: one Bernoulli trial per sample at rate/fs. The
        // filter runs every sample regardless, so its ring is continuous.
        float tickIn = 0.0f;
        if (sched < tickThreshold)
        {
            const float u   = rng.nextFloat();
            const float cut = static_cast<float> (kTickMinHz
                                  + rng.nextFloat() * (kTickMaxHz - kTickMinHz));

            tickFilter.setCutoffFrequency (cut);
            tickIn = kTickPeak * u * u * u * kTickMakeup;      // cubic power law
        }

        const float tick = tickFilter.processSample (0, tickIn);

        const float g = level.next();
        if (g == 0.0f)
            return 0.0f;

        return g * (rumble + tick);
    }

private:
    static constexpr double kLevelRampSeconds = 0.030;

    double fs = 48000.0;

    float aPink      = 0.0f;
    float aRum       = 0.0f;
    float pinkNorm   = 1.0f;
    float rumbleGain = 0.0f;

    float pink = 0.0f, rum1 = 0.0f, rum2 = 0.0f;

    double platterPhase = 0.0;
    double platterInc   = 0.0;

    float tickThreshold = 0.0f;

    juce::dsp::StateVariableTPTFilter<float> tickFilter;

    BedLevel level;
};

//==============================================================================
/** Tape hiss: decorrelated stereo, gentle HF shelf-down, riding tape speed.

    STEREO, unlike every other artifact in this engine. Two tracks carry two
    independent noise sources, and mono hiss collapses to a centred buzz that
    sounds like a fault in the plugin rather than a floor on the tape.

    RIDING SPEED is not decoration either. A tape head is a dPhi/dt
    transducer, so hiss is recorded material like everything else: when
    TapeStopGain takes the programme down to silence, hiss that kept running
    at full level would announce that the noise is synthetic. The caller
    passes TapeStopGain's current gain, so the stop mutes both together. */
class TapeBed
{
public:
    static constexpr double kHissHpHz    = 60.0;     // keep it out of the sub
    static constexpr double kHissShelfHz = 3500.0;

    // Shelf floor above the corner. -9 dB: the reproduce-head gap loss is a
    // gentle tilt, not a brick wall — cassette hiss is still bright.
    static constexpr float  kHissShelfFloor = 0.35f;

    // Target RMS at TAPE_HISS = 100%: -48 dBFS, roughly a Type I cassette
    // with no noise reduction.
    static constexpr float  kHissFullRms = 0.00398f;

    // Measured RMS the HP + shelf chain leaves of unity-RMS white (probe N2).
    static constexpr float  kHissChainGain = 0.6140f;

    void prepare (double sampleRate)
    {
        fs = sampleRate;

        aHp    = onePoleCoeff (kHissHpHz,    fs);
        aShelf = onePoleCoeff (kHissShelfHz, fs);

        hissGain = kHissFullRms / kHissChainGain;

        level.prepare (fs, kLevelRampSeconds);

        reset();
    }

    void reset() noexcept
    {
        level.reset();

        for (auto& s : st)
            s = {};
    }

    /** Per-block snapshot. `hiss01` arrives already gated by TAPE_ENABLE. */
    void setParams (float hiss01) noexcept
    {
        level.setTarget (juce::jlimit (0.0f, 1.0f, hiss01));
    }

    /** Stereo bed sample. Two unconditional draws — one per channel, so the
        channels are independent rather than a delayed copy of each other.

        speedGain — TapeStopGain::currentGain(), exactly 1.0 whenever the stop
                    law is not applying, so this is a no-op outside stops. */
    void renderSample (juce::Random& rng, float speedGain, float& outL, float& outR) noexcept
    {
        const float wl = unitRmsWhite (rng);
        const float wr = unitRmsWhite (rng);

        const float sl = shape (0, wl);
        const float sr = shape (1, wr);

        const float g = level.next() * juce::jlimit (0.0f, 1.0f, speedGain) * hissGain;

        if (g == 0.0f)
        {
            outL = 0.0f;
            outR = 0.0f;
            return;
        }

        outL = sl * g;
        outR = sr * g;
    }

private:
    static constexpr double kLevelRampSeconds = 0.030;

    struct Chan
    {
        float lpHp    = 0.0f;
        float lpShelf = 0.0f;
    };

    /** One-pole HP (complement form), then a first-order high shelf built as
        a blend between the lowpass and its own complement: below the corner
        the lowpass dominates and the gain is exactly 1; above it, only the
        floor survives. */
    float shape (int ch, float x) noexcept
    {
        auto& s = st[ch];

        s.lpHp += aHp * (x - s.lpHp);
        const float hp = x - s.lpHp;

        s.lpShelf += aShelf * (hp - s.lpShelf);
        return s.lpShelf + kHissShelfFloor * (hp - s.lpShelf);
    }

    double fs = 48000.0;

    float aHp      = 0.0f;
    float aShelf   = 0.0f;
    float hissGain = 0.0f;

    Chan st[2];

    BedLevel level;
};

//==============================================================================
/** Phone line: mains hum + Poisson crackle bursts.

    MONO because the line is mono — CodecStage already collapses to a single
    channel, and mains hum is common-mode by definition.

    The hum takes NO RNG draws at all: it is three sine partials off one phase
    accumulator, i.e. a pure function of the sample count. That is deliberate.
    A hum whose phase depended on the seed would drift against the programme
    between renders of the same session, and mains hum is the one artifact in
    this plugin that is genuinely not random. */
class CodecBed
{
public:
    static constexpr double kHum50Hz = 50.0;
    static constexpr double kHum60Hz = 60.0;

    // Transformer saturation puts harmonics on the fundamental. Two of them,
    // per the research recipe.
    static constexpr float  kHum2ndLevel = 0.40f;
    static constexpr float  kHum3rdLevel = 0.25f;

    // Unity-RMS normaliser for that partial sum: RMS = sqrt(0.5 * (1 + 0.40^2
    // + 0.25^2)) = 0.78182, and the reciprocal is what makes kHumFullRms
    // readable as dBFS.
    static constexpr float  kHumNorm = 1.27906f;

    // Target hum RMS at CODEC_NOISE = 100%: -40 dBFS.
    static constexpr float  kHumFullRms = 0.0100f;

    static constexpr double kMaxCracklePerSec = 6.0;
    static constexpr float  kCracklePeak      = 0.09f;    // ~ -21 dBFS
    static constexpr double kCrackleMinMs     = 3.0;
    static constexpr double kCrackleMaxMs     = 25.0;
    static constexpr double kCrackleHpHz      = 300.0;    // the phone passband
    static constexpr double kCrackleLpHz      = 3000.0;

    void prepare (double sampleRate)
    {
        fs = sampleRate;

        aHp = onePoleCoeff (kCrackleHpHz, fs);
        aLp = onePoleCoeff (kCrackleLpHz, fs);

        level.prepare (fs, kLevelRampSeconds);

        setMains (0);
        reset();
    }

    void reset() noexcept
    {
        level.reset();

        lpHp = lpTop = 0.0f;
        humPhase     = 0.0;
        burstRemain  = 0;
        burstLen     = 1;
        burstAmp     = 0.0f;
    }

    /** Per-block snapshot.

        noise01 — the raw CODEC_NOISE knob; it sets crackle DENSITY, which is
                  a property of the line rather than of how much of the line
                  you are listening to.
        mix01   — CODEC_MIX. Folded into the level target instead of applied
                  separately so that the ramp smooths it: Blend is "how much
                  phone", and the hum IS the phone, so at Blend 0 there is no
                  line to hum. (CodecStage smooths its own copy over 20 ms;
                  this ramp is 30 ms — near enough that they track.)
        enabled — CODEC_ENABLE. */
    void setParams (float noise01, float mix01, int mainsIndex, bool enabled) noexcept
    {
        const float n = juce::jlimit (0.0f, 1.0f, noise01);

        crackleThreshold = static_cast<float> (kMaxCracklePerSec * n / juce::jmax (1.0, fs));

        setMains (mainsIndex);

        level.setTarget (enabled ? n * juce::jlimit (0.0f, 1.0f, mix01) : 0.0f);
    }

    /** Mono bed sample. Two unconditional draws (crackle excitation, then the
        burst schedule) plus two more only when a burst starts. */
    float renderSample (juce::Random& rng) noexcept
    {
        const float white = unitRmsWhite (rng);
        const float sched = rng.nextFloat();

        // Band-limit to the phone passband. Runs every sample so the state is
        // continuous across bursts.
        lpHp += aHp * (white - lpHp);
        const float hp = white - lpHp;
        lpTop += aLp * (hp - lpTop);

        // One burst at a time — a crackle is a discrete event on the line,
        // and the guard is what keeps overlapping bursts from stacking into a
        // continuous rasp at high density.
        if (burstRemain <= 0 && sched < crackleThreshold)
        {
            burstLen = juce::jmax (1, static_cast<int> (
                           fs * 0.001 * (kCrackleMinMs
                               + rng.nextFloat() * (kCrackleMaxMs - kCrackleMinMs))));
            burstRemain = burstLen;

            const float u = rng.nextFloat();
            burstAmp = kCracklePeak * u * u;         // power-law, as the vinyl ticks
        }

        float crackle = 0.0f;
        if (burstRemain > 0)
        {
            // Linear decay to exactly 0 at the end of the burst: the ATTACK is
            // meant to be a step (that is what a crackle is), the release is
            // not, and landing on 0 makes the tail click-free with no extra
            // bookkeeping.
            crackle = lpTop * burstAmp
                      * (static_cast<float> (burstRemain) / static_cast<float> (burstLen));
            --burstRemain;
        }

        humPhase += humInc;
        if (humPhase >= juce::MathConstants<double>::twoPi)
            humPhase -= juce::MathConstants<double>::twoPi;

        const float hum = static_cast<float> (
                              std::sin (humPhase)
                              + kHum2ndLevel * std::sin (2.0 * humPhase)
                              + kHum3rdLevel * std::sin (3.0 * humPhase))
                          * kHumNorm;

        const float g = level.next();
        if (g == 0.0f)
            return 0.0f;

        return g * (hum * kHumFullRms + crackle);
    }

private:
    static constexpr double kLevelRampSeconds = 0.030;

    void setMains (int mainsIndex) noexcept
    {
        humInc = juce::MathConstants<double>::twoPi
                 * (mainsIndex == 1 ? kHum60Hz : kHum50Hz) / juce::jmax (1.0, fs);
    }

    double fs = 48000.0;

    float aHp = 0.0f, aLp = 0.0f;
    float lpHp = 0.0f, lpTop = 0.0f;

    double humPhase = 0.0;
    double humInc   = 0.0;

    float crackleThreshold = 0.0f;
    int   burstRemain      = 0;
    int   burstLen         = 1;
    float burstAmp         = 0.0f;

    BedLevel level;
};
