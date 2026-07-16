/*
  ==============================================================================

    O-simpleFM render-harness — Stage 2 DSP correctness gate.

    Instantiates OSimpleFMAudioProcessor directly and verifies the 2-operator
    phase-modulation math without a DAW:

      1. Makes sound        — default params, MIDI note -> non-trivial RMS.
      2. Correct pitch      — index 0 -> dominant partial at the played freq.
      3. Index -> sidebands — raising index grows sideband energy.
      4. Carrier null       — at I ~= 2.405 (Bessel J0 zero) the carrier bin
                              is strongly suppressed vs the first sideband.
      5. Feedback stability — feedback = 100% stays bounded + finite (no
                              Nyquist limit-cycle blow-up / NaN).

    Exit 0 iff all checks pass.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"

#include <vector>
#include <cmath>
#include <cstdio>

//==============================================================================
static void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float real)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (real));
}

// Single-bin DFT amplitude estimate at frequency f over [off, off+len).
static double binAmplitude (const std::vector<float>& x, int off, int len, double f, double fs)
{
    double re = 0.0, im = 0.0;
    for (int n = 0; n < len; ++n)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * f * n / fs;
        const double s = x[(size_t) (off + n)];
        re += s * std::cos (w);
        im -= s * std::sin (w);
    }
    return 2.0 * std::sqrt (re * re + im * im) / (double) len;
}

static double rms (const std::vector<float>& x, int off, int len)
{
    double acc = 0.0;
    for (int n = 0; n < len; ++n) { const double s = x[(size_t) (off + n)]; acc += s * s; }
    return std::sqrt (acc / (double) len);
}

static double peakAbs (const std::vector<float>& x)
{
    double p = 0.0;
    for (float s : x) p = juce::jmax (p, (double) std::abs (s));
    return p;
}

static bool allFinite (const std::vector<float>& x)
{
    for (float s : x) if (! std::isfinite (s)) return false;
    return true;
}

//==============================================================================
// Render `seconds` of mono output (channel 0). Note-on at sample 0; note-off at
// 85% (so the [0.5s, 1.0s] analysis window is steady-state sustain).
static std::vector<float> render (OSimpleFMAudioProcessor& proc,
                                  int midiNote, double seconds, double fs)
{
    const int block = 512;
    const int total = (int) (seconds * fs);
    const int offSample = (int) (total * 0.85);

    juce::AudioBuffer<float> buf (2, block);
    std::vector<float> out;
    out.reserve ((size_t) total);

    int pos = 0;
    bool sentOn = false;
    while (pos < total)
    {
        buf.clear();
        juce::MidiBuffer midi;
        if (! sentOn) { midi.addEvent (juce::MidiMessage::noteOn (1, midiNote, (juce::uint8) 100), 0); sentOn = true; }
        if (offSample >= pos && offSample < pos + block)
            midi.addEvent (juce::MidiMessage::noteOff (1, midiNote), offSample - pos);

        proc.processBlock (buf, midi);

        const int n = juce::jmin (block, total - pos);
        for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
        pos += block;
    }
    return out;
}

// Reset all 17 params to factory defaults so each scenario starts clean.
static void resetDefaults (juce::AudioProcessorValueTreeState& a)
{
    using namespace OSimpleFM::ParamIDs;
    setParam (a, ratio, 1.0f);        setParam (a, ratioSnap, 0.0f);
    setParam (a, modIndex, 0.0f);     setParam (a, feedback, 0.0f);
    setParam (a, modFixedMode, 0.0f); setParam (a, modFixedHz, 220.0f);
    setParam (a, modEnvToIndex, 1.0f);setParam (a, velToIndex, 0.0f);
    setParam (a, modAttack, 0.01f);   setParam (a, modDecay, 0.3f);
    setParam (a, modSustain, 1.0f);   setParam (a, modRelease, 0.3f);  // sustain=1 -> steady index
    setParam (a, ampAttack, 0.01f);   setParam (a, ampDecay, 0.3f);
    setParam (a, ampSustain, 0.8f);   setParam (a, ampRelease, 0.3f);
    setParam (a, outputLevel, 0.0f);
}

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;  // message manager for plugin internals

    const double fs = 44100.0;
    OSimpleFMAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, fs, 512);
    proc.prepareToPlay (fs, 512);
    auto& apvts = proc.getAPVTS();

    const int   note  = 69;                                  // A4
    const double f0   = juce::MidiMessage::getMidiNoteInHertz (note);  // 440
    const int   aOff  = (int) (0.5 * fs);                    // analysis window [0.5s, 1.0s]
    const int   aLen  = (int) (0.5 * fs);

    int failures = 0;
    auto check = [&] (const char* name, bool ok, const juce::String& detail)
    {
        std::printf ("  [%s] %-22s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    };

    std::printf ("O-simpleFM render-harness — fs=%.0f, note=%d (%.1f Hz)\n", fs, note, f0);

    // --- 1 & 2: makes sound + correct pitch (index 0 = pure carrier sine) -----
    {
        resetDefaults (apvts);
        setParam (apvts, OSimpleFM::ParamIDs::modIndex, 0.0f);
        auto y = render (proc, note, 1.5, fs);

        const double r   = rms (y, aOff, aLen);
        const double a440 = binAmplitude (y, aOff, aLen, f0, fs);
        const double a220 = binAmplitude (y, aOff, aLen, f0 * 0.5, fs);
        const double a880 = binAmplitude (y, aOff, aLen, f0 * 2.0, fs);

        check ("makes-sound", r > 0.02 && allFinite (y),
               juce::String ("rms=") + juce::String (r, 4));
        check ("pitch=fundamental", a440 > a220 * 4.0 && a440 > a880 * 4.0,
               juce::String ("a440=") + juce::String (a440, 4)
                 + " a220=" + juce::String (a220, 4) + " a880=" + juce::String (a880, 4));
    }

    // --- 3: index -> sidebands (ratio 4 -> first upper sideband at f0+4*f0) ----
    double sbLow = 0.0, sbHigh = 0.0;
    {
        const double fSide = f0 + 4.0 * f0;   // 2200 Hz (ratio=4, fm=1760)
        resetDefaults (apvts);
        setParam (apvts, OSimpleFM::ParamIDs::ratio, 4.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modIndex, 0.0f);
        auto yLow = render (proc, note, 1.5, fs);
        sbLow = binAmplitude (yLow, aOff, aLen, fSide, fs);

        setParam (apvts, OSimpleFM::ParamIDs::modIndex, 18.0f);  // high index
        auto yHigh = render (proc, note, 1.5, fs);
        sbHigh = binAmplitude (yHigh, aOff, aLen, fSide, fs);

        check ("index->sidebands", sbHigh > sbLow * 10.0 + 0.01 && allFinite (yHigh),
               juce::String ("sb@2200: idx0=") + juce::String (sbLow, 5)
                 + " idxHigh=" + juce::String (sbHigh, 5));
    }

    // --- 4: carrier null at I ~= 2.405 (J0 zero), ratio 4 (no fold onto f0) ----
    {
        // modIndex stored such that baseIndex = kIndexMax*(stored/kIndexMax)^kIndexTaper = 2.405
        const float idxStored = OSimpleFM::kIndexMax
                              * std::pow (2.405f / OSimpleFM::kIndexMax, 1.0f / OSimpleFM::kIndexTaper);
        resetDefaults (apvts);
        setParam (apvts, OSimpleFM::ParamIDs::ratio, 4.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modSustain, 1.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modEnvToIndex, 1.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modIndex, idxStored);
        auto y = render (proc, note, 1.5, fs);

        const double aCar  = binAmplitude (y, aOff, aLen, f0, fs);          // 440  (J0~=0)
        const double aSide = binAmplitude (y, aOff, aLen, f0 + 4.0 * f0, fs); // 2200 (J1)
        check ("carrier-null@2.405", aCar < aSide * 0.30 && aSide > 0.01,
               juce::String ("carrier=") + juce::String (aCar, 5)
                 + " sideband=" + juce::String (aSide, 5)
                 + " ratio=" + juce::String (aSide > 0 ? aCar / aSide : 0.0, 4));
    }

    // --- 5: feedback stability at 100% ---------------------------------------
    {
        resetDefaults (apvts);
        setParam (apvts, OSimpleFM::ParamIDs::ratio, 1.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modIndex, 10.0f);
        setParam (apvts, OSimpleFM::ParamIDs::feedback, 1.0f);   // max
        auto y = render (proc, note, 2.0, fs);

        const double pk = peakAbs (y);
        const double r  = rms (y, aOff, aLen);
        check ("feedback-stable", allFinite (y) && pk < 4.0 && r > 0.001 && r < 2.0,
               juce::String ("peak=") + juce::String (pk, 4) + " rms=" + juce::String (r, 4));
    }

    // --- 6: aliasing audit — high pitch + max index + feedback (QUAL-01) ------
    // The 2x polyphase-IIR oversampling + key-tracked Carson index ceiling must
    // keep alias products (energy at NON-integer multiples of f0) well below the
    // legitimate harmonic energy, and the output finite/bounded.
    {
        const int    hiNote = 96;                                          // C7
        const double hf0    = juce::MidiMessage::getMidiNoteInHertz (hiNote); // ~2093 Hz
        resetDefaults (apvts);
        setParam (apvts, OSimpleFM::ParamIDs::ratio, 1.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modSustain, 1.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modEnvToIndex, 1.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modIndex, 20.0f);            // max (ceiling clamps)
        setParam (apvts, OSimpleFM::ParamIDs::feedback, 0.8f);
        auto y = render (proc, hiNote, 1.5, fs);

        double harm = 0.0;
        for (int k = 1; k <= 4 && k * hf0 < 0.45 * fs; ++k)
            harm += binAmplitude (y, aOff, aLen, k * hf0, fs);
        double alias = 0.0;
        for (double m : { 0.5, 1.37, 2.4, 3.3 })                          // inter-harmonic probes
            if (m * hf0 < 0.45 * fs)
                alias += binAmplitude (y, aOff, aLen, m * hf0, fs);

        check ("aa-highpitch", allFinite (y) && peakAbs (y) < 4.0 && alias < harm * 0.35 + 1.0e-4,
               juce::String ("harm=") + juce::String (harm, 4)
                 + " alias=" + juce::String (alias, 4)
                 + " ratio=" + juce::String (harm > 0 ? alias / harm : 0.0, 4));
    }

    // --- 7: fixed-mode modulator at high Hz + max index stays bounded ---------
    {
        resetDefaults (apvts);
        setParam (apvts, OSimpleFM::ParamIDs::modFixedMode, 1.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modFixedHz, 8000.0f);        // clamps to range max
        setParam (apvts, OSimpleFM::ParamIDs::modSustain, 1.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modEnvToIndex, 1.0f);
        setParam (apvts, OSimpleFM::ParamIDs::modIndex, 20.0f);
        auto y = render (proc, note, 1.5, fs);

        check ("aa-fixed-highHz", allFinite (y) && peakAbs (y) < 4.0 && rms (y, aOff, aLen) > 0.0005,
               juce::String ("peak=") + juce::String (peakAbs (y), 4)
                 + " rms=" + juce::String (rms (y, aOff, aLen), 4));
    }

    proc.releaseResources();

    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
