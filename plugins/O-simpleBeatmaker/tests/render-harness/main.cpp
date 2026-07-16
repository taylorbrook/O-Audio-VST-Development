/*
  ==============================================================================

    O-simpleBeatmaker render-harness — Stage 2 DSP correctness gate.

    Instantiates OSimpleBeatmakerAudioProcessor directly, injects a synthetic
    AudioPlayHead, and verifies the timing/voice spine WITHOUT a DAW:

      Probe 1  grid accuracy        — straight time: each ON step fires at exactly
                                      round(k*samplesPer16th) within the bar (±0).
      Probe 2  swing                — swing=75%: odd 16ths delayed by exactly
                                      round((1/3)*T8*fs); even 16ths unmoved.
      Probe 3  humanize + quantize  — hum=1,q=0: bounded spread (late-only +30ms,
                                      vel ±24). hum=1,swing=75%,q=1: humanize
                                      collapses to ~0 BUT swing survives (DSP-04).
      Probe 4  block-boundary       — a (swung) step straddling a block edge fires
                                      once at the correct absolute sample.
      Probe 5  MIDI-playable voices — host note-on per GM note -> correct voice,
                                      non-trivial RMS; closed-hat chokes open-hat;
                                      velocity scales RMS; band-clean + bounded.
      Probe 6  viz truth (QUAL-02)  — emitted appliedSampleInBar − nominalSampleInBar
                                      == the Δt baked into the emitted MidiMessage,
                                      and the viz FIFO agrees with the MIDI buffer.
      Probe 7  mono parity (CR-02)  — a mono render matches one stereo channel's
                                      RMS (no +6 dB double-add on 1-ch buffers).

    Exit 0 iff all probes pass.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"

#include <vector>
#include <array>
#include <tuple>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdio>

using Proc  = OSimpleBeatmakerAudioProcessor;
using VizEv = OSimpleBeatmaker::VizEvent;
namespace PID = OSimpleBeatmaker::ParamIDs;

//==============================================================================
// Synthetic transport. getPosition() is the only pure-virtual; return a known
// bpm/ppq/isPlaying so probes assert EXACT sample offsets.
struct FakePlayHead : juce::AudioPlayHead
{
    double bpm = 120.0, ppq = 0.0, fs = 44100.0;
    bool   playing = true;

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo pos;
        pos.setBpm (bpm);
        pos.setPpqPosition (ppq);
        pos.setPpqPositionOfLastBarStart (std::floor (ppq / 4.0) * 4.0);   // 4/4 bar = 4 ppq
        pos.setTimeInSamples ((juce::int64) std::llround (ppq * (60.0 / bpm) * fs));
        pos.setIsPlaying (playing);
        pos.setTimeSignature (juce::AudioPlayHead::TimeSignature{});       // 4/4 default
        return pos;
    }
};

//==============================================================================
static void setParam (juce::AudioProcessorValueTreeState& a, juce::StringRef id, float real)
{
    if (auto* p = a.getParameter (id))
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (real));
}

static void setDefaults (Proc& proc)
{
    using namespace OSimpleBeatmaker;
    auto& a = proc.getAPVTS();
    setParam (a, PID::swing, 0.0f);
    setParam (a, PID::humanize, 0.0f);
    setParam (a, PID::quantizeStrength, 1.0f);
    setParam (a, PID::patternLength, 1.0f);          // idx 1 -> 16
    setParam (a, PID::tempo, 120.0f);
    setParam (a, PID::outputLevel, 0.0f);
    for (int v = 0; v < kNumVoices; ++v)
    {
        setParam (a, voiceParamID (v, PID::sufTune),  0.0f);
        setParam (a, voiceParamID (v, PID::sufDecay), 0.5f);
        setParam (a, voiceParamID (v, PID::sufTone),  0.5f);
        setParam (a, voiceParamID (v, PID::sufLevel), 0.0f);
        setParam (a, voiceParamID (v, PID::sufMute),  0.0f);
        setParam (a, voiceParamID (v, PID::sufSolo),  0.0f);
    }
}

static int patternIdx (int steps) { return steps == 8 ? 0 : (steps == 32 ? 2 : 1); }

//==============================================================================
static double rms (const std::vector<float>& x, int off, int len)
{
    if (off < 0 || len <= 0 || off + len > (int) x.size()) return 0.0;
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

//==============================================================================
// One collected sequencer hit (test hook + the real MIDI buffer offset).
struct CollHit
{
    int voiceIndex, stepIndex, velocity;
    juce::int64 firedAbs;            // absolute sample the note-on was placed at
    int appliedOffsetSamples;
    juce::int32 nominalSampleInBar, appliedSampleInBar;
};

struct SeqResult
{
    std::vector<CollHit> hits;                       // from the processor test hook
    std::vector<VizEv>   viz;                         // drained from the real FIFO (source 0)
    std::vector<std::pair<int, juce::int64>> midiAbs; // (note, absSample) from the real merged buffer
    std::vector<float>   audioL;
    double samplesPerPpq = 0.0, samplesPer16th = 0.0, samplesPerBar = 0.0;
    int    patternLen = 16;
};

// Render `numBlocks` blocks host-synced, collecting emitted hits, viz events,
// the real sequencer MIDI offsets, and audio.
static SeqResult renderSynced (Proc& proc, double fs, double bpm, int patternLen,
                               float swing01, float hum01, float q,
                               const std::vector<std::pair<int,int>>& onCells,
                               int numBlocks, int block)
{
    using namespace OSimpleBeatmaker;
    proc.prepareToPlay (fs, block);
    setDefaults (proc);
    auto& a = proc.getAPVTS();
    setParam (a, PID::patternLength, (float) patternIdx (patternLen));
    setParam (a, PID::tempo, (float) bpm);
    setParam (a, PID::swing, swing01);
    setParam (a, PID::humanize, hum01);
    setParam (a, PID::quantizeStrength, q);

    proc.clearGrid();
    for (auto& c : onCells) proc.setStep (c.first, c.second, 100);

    FakePlayHead fake; fake.bpm = bpm; fake.fs = fs; fake.playing = true;
    proc.setPlayHead (&fake);

    SeqResult r;
    r.samplesPerPpq  = (60.0 / bpm) * fs;
    r.samplesPer16th = r.samplesPerPpq * 0.25;
    r.samplesPerBar  = patternLen * 0.25 * r.samplesPerPpq;
    r.patternLen     = patternLen;

    juce::AudioBuffer<float> buf (2, block);
    auto& viz = proc.getVizAnalyzer();
    std::array<VizEv, 256> drainBuf;

    for (int b = 0; b < numBlocks; ++b)
    {
        const juce::int64 blockStartAbs = (juce::int64) b * block;
        fake.ppq = (double) blockStartAbs / r.samplesPerPpq;   // exact, no drift

        buf.clear();
        juce::MidiBuffer midi;                                  // no host MIDI
        proc.processBlock (buf, midi);

        for (const auto& h : proc.getTestEmittedHits())
            r.hits.push_back ({ h.voiceIndex, h.stepIndex, h.velocity,
                                h.blockStartAbs + h.finalOffsetInBlock,
                                h.appliedOffsetSamples, h.nominalSampleInBar, h.appliedSampleInBar });

        const int n = viz.drain (drainBuf.data(), 256);
        for (int i = 0; i < n; ++i)
            if (drainBuf[(size_t) i].source == 0)
                r.viz.push_back (drainBuf[(size_t) i]);

        for (const auto meta : proc.getLastSequencerMidi())
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn())
                r.midiAbs.push_back ({ m.getNoteNumber(), blockStartAbs + meta.samplePosition });
        }

        for (int i = 0; i < block; ++i) r.audioL.push_back (buf.getSample (0, i));
    }

    proc.setPlayHead (nullptr);
    return r;
}

// Render host MIDI events (no sequencer): events = (timeSec, note, vel).
static std::vector<float> renderHost (Proc& proc, double fs, int block, double seconds,
                                      const std::vector<std::tuple<double,int,int>>& events)
{
    proc.prepareToPlay (fs, block);
    setDefaults (proc);
    proc.clearGrid();
    proc.setPlayHead (nullptr);

    const int total = (int) (seconds * fs);
    std::vector<float> out; out.reserve ((size_t) total);
    juce::AudioBuffer<float> buf (2, block);

    int pos = 0;
    while (pos < total)
    {
        buf.clear();
        juce::MidiBuffer midi;
        for (auto& e : events)
        {
            const int s = (int) (std::get<0> (e) * fs);
            if (s >= pos && s < pos + block)
                midi.addEvent (juce::MidiMessage::noteOn (1, std::get<1> (e), (juce::uint8) std::get<2> (e)), s - pos);
        }
        proc.processBlock (buf, midi);
        const int n = juce::jmin (block, total - pos);
        for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
        pos += block;
    }
    return out;
}

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    using namespace OSimpleBeatmaker;

    const double fs = 44100.0;
    const int    block = 512;

    int failures = 0;
    auto check = [&] (const char* name, bool ok, const juce::String& detail)
    {
        std::printf ("  [%s] %-26s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    };

    std::printf ("O-simpleBeatmaker render-harness — fs=%.0f, block=%d\n", fs, block);

    Proc proc;
    proc.setPlayConfigDetails (0, 2, fs, block);

    // Helper: confirm a real MIDI note-on exists at firedAbs for the hit's voice.
    auto midiHasHit = [] (const SeqResult& r, const CollHit& h) -> bool
    {
        const int note = kGmNotes[(size_t) h.voiceIndex];
        for (const auto& e : r.midiAbs)
            if (e.first == note && std::llabs (e.second - h.firedAbs) <= 1)
                return true;
        return false;
    };

    // Generic per-hit invariants used across the timing probes.
    auto barRelativeOK = [] (const SeqResult& r, const CollHit& h, juce::int64 tol) -> bool
    {
        const juce::int64 nominalAbs = h.firedAbs - h.appliedOffsetSamples;
        const juce::int64 barIndex   = (juce::int64) std::llround (
            (double) (nominalAbs - h.nominalSampleInBar) / r.samplesPerBar);
        const juce::int64 expected   = (juce::int64) std::llround (barIndex * r.samplesPerBar)
                                       + h.nominalSampleInBar;
        const bool barOK   = std::llabs (nominalAbs - expected) <= tol;
        const bool vizOK   = (h.appliedSampleInBar - h.nominalSampleInBar) == h.appliedOffsetSamples;
        return barOK && vizOK;
    };

    //==========================================================================
    // PROBE 5 — MIDI-playable + voices (Phase 2.1 gate).
    //==========================================================================
    {
        bool allVoicesSound = true;
        juce::String detail;
        const std::array<int,6> notes { 36, 38, 39, 42, 46, 45 };
        for (int v = 0; v < 6; ++v)
        {
            auto y = renderHost (proc, fs, block, 0.6, { { 0.0, notes[(size_t) v], 110 } });
            const double r = rms (y, 0, (int) (0.2 * fs));
            const bool ok = r > 0.003 && allFinite (y);
            allVoicesSound = allVoicesSound && ok;
            detail += juce::String (kVoiceName[(size_t) v]) + "=" + juce::String (r, 4) + " ";
        }
        check ("voices-make-sound", allVoicesSound, detail);

        // Hi-hat choke: closed (42) chokes open (46).
        auto openOnly  = renderHost (proc, fs, block, 0.7, { { 0.0, 46, 110 } });
        auto openChoke = renderHost (proc, fs, block, 0.7, { { 0.0, 46, 110 }, { 0.15, 42, 110 } });
        const int wOff = (int) (0.30 * fs), wLen = (int) (0.30 * fs);
        const double tailOpen  = rms (openOnly,  wOff, wLen);
        const double tailChoke = rms (openChoke, wOff, wLen);
        check ("hat-choke", tailChoke < tailOpen * 0.5 && tailOpen > 0.002,
               juce::String ("openTail=") + juce::String (tailOpen, 4)
                 + " chokedTail=" + juce::String (tailChoke, 4));

        // Velocity scales loudness (kick).
        auto soft = renderHost (proc, fs, block, 0.6, { { 0.0, 36, 30 } });
        auto loud = renderHost (proc, fs, block, 0.6, { { 0.0, 36, 120 } });
        const double rSoft = rms (soft, 0, (int) (0.2 * fs));
        const double rLoud = rms (loud, 0, (int) (0.2 * fs));
        check ("velocity-scales", rLoud > rSoft * 1.5 && rSoft > 0.0,
               juce::String ("soft=") + juce::String (rSoft, 4) + " loud=" + juce::String (rLoud, 4));

        // High-rate hits: finite + bounded peak.
        std::vector<std::tuple<double,int,int>> burst;
        for (int i = 0; i < 20; ++i) burst.push_back ({ i * 0.05, 36, 110 });
        auto stress = renderHost (proc, fs, block, 1.2, burst);
        check ("high-rate-bounded", allFinite (stress) && peakAbs (stress) < 4.0,
               juce::String ("peak=") + juce::String (peakAbs (stress), 4));

        // Band-clean tonal body: tom (pure fastSine) -> energy at f0, ~none at
        // inter-harmonic probes (aliasing budget / DSP-06 / QUAL-01).
        {
            proc.prepareToPlay (fs, block);
            setDefaults (proc);
            auto& a = proc.getAPVTS();
            setParam (a, voiceParamID (Tom, PID::sufTone),  0.0f);  // minimal pitch env -> steady
            setParam (a, voiceParamID (Tom, PID::sufDecay), 1.0f);  // long ring
            proc.clearGrid();
            proc.setPlayHead (nullptr);
            const int total = (int) (0.6 * fs);
            std::vector<float> y; y.reserve ((size_t) total);
            juce::AudioBuffer<float> buf (2, block);
            int pos = 0;
            while (pos < total)
            {
                buf.clear();
                juce::MidiBuffer midi;
                if (pos == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 110), 0);
                proc.processBlock (buf, midi);
                const int n = juce::jmin (block, total - pos);
                for (int i = 0; i < n; ++i) y.push_back (buf.getSample (0, i));
                pos += block;
            }
            const int aOff = (int) (0.2 * fs), aLen = (int) (0.3 * fs);
            const double f0   = 120.0;                       // tom base @ tune 0
            const double harm = binAmplitude (y, aOff, aLen, f0, fs);
            double alias = 0.0;
            for (double mlt : { 1.37, 2.4, 3.3 })
                alias += binAmplitude (y, aOff, aLen, mlt * f0, fs);
            check ("band-clean-tonal", allFinite (y) && harm > 0.002 && alias < harm * 0.5 + 1.0e-4,
                   juce::String ("harm=") + juce::String (harm, 4)
                     + " alias=" + juce::String (alias, 4));
        }
    }

    //==========================================================================
    // PROBE 1 — grid accuracy (straight time).
    //==========================================================================
    {
        // Kick on 0,4,8,12 ; snare on 4,12 ; closed hat on every step.
        std::vector<std::pair<int,int>> cells;
        for (int s : { 0, 4, 8, 12 }) cells.push_back ({ Kick, s });
        for (int s : { 4, 12 })       cells.push_back ({ Snare, s });
        for (int s = 0; s < 16; ++s)  cells.push_back ({ ClosedHat, s });

        auto r = renderSynced (proc, fs, 120.0, 16, 0.0f, 0.0f, 1.0f, cells, 360, block); // ~2 bars

        bool gridOK = ! r.hits.empty();
        juce::int64 maxErr = 0;
        for (const auto& h : r.hits)
        {
            const juce::int64 expectNominal = (juce::int64) std::llround (h.stepIndex * r.samplesPer16th);
            gridOK = gridOK && (h.appliedOffsetSamples == 0);
            gridOK = gridOK && (h.nominalSampleInBar == expectNominal);
            gridOK = gridOK && barRelativeOK (r, h, 0);
            gridOK = gridOK && midiHasHit (r, h);
            maxErr = juce::jmax (maxErr, (juce::int64) std::llabs (h.nominalSampleInBar - expectNominal));
        }
        // viz FIFO must mirror the emitted hits exactly (count + content).
        const bool vizCount = (r.viz.size() == r.hits.size());
        check ("grid-accuracy", gridOK && vizCount && r.hits.size() >= 30,
               juce::String ("hits=") + juce::String ((int) r.hits.size())
                 + " viz=" + juce::String ((int) r.viz.size())
                 + " maxNominalErr=" + juce::String ((int) maxErr));
    }

    //==========================================================================
    // PROBE 2 — swing (swing=75% -> swing01=1).
    //==========================================================================
    {
        std::vector<std::pair<int,int>> cells;
        for (int s = 0; s < 16; ++s) cells.push_back ({ ClosedHat, s });   // every 16th

        auto r = renderSynced (proc, fs, 120.0, 16, 1.0f, 0.0f, 1.0f, cells, 360, block);

        const double T8 = (60.0 / 120.0) / 2.0;
        const int    expectSwing = (int) std::llround ((1.0 / 3.0) * T8 * fs);  // 3675 @ 120
        bool swingOK = ! r.hits.empty();
        for (const auto& h : r.hits)
        {
            const int want = (h.stepIndex % 2 == 1) ? expectSwing : 0;
            swingOK = swingOK && (h.appliedOffsetSamples == want);
            swingOK = swingOK && barRelativeOK (r, h, 0);
            swingOK = swingOK && midiHasHit (r, h);
        }
        check ("swing-offset", swingOK,
               juce::String ("expectSwing=") + juce::String (expectSwing)
                 + " hits=" + juce::String ((int) r.hits.size()));
    }

    //==========================================================================
    // PROBE 3 — humanize + quantize (the DSP-04 lesson).
    //==========================================================================
    {
        // 3A: humanize=1, swing=0, q=0 -> bounded spread, vel ±24, late-only.
        std::vector<std::pair<int,int>> cells;
        for (int s = 0; s < 16; ++s) cells.push_back ({ Kick, s });
        auto rA = renderSynced (proc, fs, 120.0, 16, 0.0f, 1.0f, 0.0f, cells, 1500, block); // ~8 bars

        const int maxLate = (int) std::llround (TimingFeelEngine::kHumanizeMaxSec * fs);  // 1323 @ 44.1k
        int  minOff = 1 << 30, maxOff = -(1 << 30), minVel = 200, maxVel = -1;
        bool boundsA = ! rA.hits.empty();
        for (const auto& h : rA.hits)
        {
            boundsA = boundsA && (h.appliedOffsetSamples >= 0 && h.appliedOffsetSamples <= maxLate);
            boundsA = boundsA && (h.velocity >= 100 - 24 && h.velocity <= 100 + 24);
            minOff = juce::jmin (minOff, h.appliedOffsetSamples);
            maxOff = juce::jmax (maxOff, h.appliedOffsetSamples);
            minVel = juce::jmin (minVel, h.velocity);
            maxVel = juce::jmax (maxVel, h.velocity);
        }
        const bool spreadA = (maxOff - minOff) > 0 && (maxVel - minVel) > 0;
        check ("humanize-spread", boundsA && spreadA,
               juce::String ("offset[") + juce::String (minOff) + "," + juce::String (maxOff)
                 + "] vel[" + juce::String (minVel) + "," + juce::String (maxVel)
                 + "] maxLate=" + juce::String (maxLate));

        // 3B: humanize=1, swing=75%, q=1 -> humanize collapses, swing SURVIVES.
        auto rB = renderSynced (proc, fs, 120.0, 16, 1.0f, 1.0f, 1.0f, cells, 360, block);
        const double T8 = (60.0 / 120.0) / 2.0;
        const int    expectSwing = (int) std::llround ((1.0 / 3.0) * T8 * fs);
        bool dsp04 = ! rB.hits.empty();
        for (const auto& h : rB.hits)
        {
            const int want = (h.stepIndex % 2 == 1) ? expectSwing : 0;
            dsp04 = dsp04 && (h.appliedOffsetSamples == want);   // humanize gone, swing exact
            dsp04 = dsp04 && (h.velocity == 100);                // velocity humanize gone
        }
        check ("quantize-preserves-swing", dsp04,
               juce::String ("q=1: swing=") + juce::String (expectSwing)
                 + " survives, humanize->0, hits=" + juce::String ((int) rB.hits.size()));
    }

    //==========================================================================
    // PROBE 4 — block-boundary independence (no double-fire / miss).
    //==========================================================================
    {
        // Straight time, odd block size so step onsets straddle edges.
        std::vector<std::pair<int,int>> cells;
        for (int s = 0; s < 16; ++s) cells.push_back ({ Kick, s });
        const int oddBlock = 423;
        auto r = renderSynced (proc, fs, 120.0, 16, 0.0f, 0.0f, 1.0f, cells, 900, oddBlock); // ~5 bars

        std::map<std::tuple<int,int,int>, int> fireCount;
        bool placeOK = ! r.hits.empty();
        for (const auto& h : r.hits)
        {
            const juce::int64 barIdx = (juce::int64) std::llround (
                (double) (h.firedAbs - h.nominalSampleInBar) / r.samplesPerBar);
            fireCount[ { h.voiceIndex, h.stepIndex, (int) barIdx } ]++;
            placeOK = placeOK && barRelativeOK (r, h, 0) && midiHasHit (r, h);
        }
        bool onceEach = true;
        for (auto& kv : fireCount) onceEach = onceEach && (kv.second == 1);

        // Swung straddle: swing pushes odd steps past block edges -> carry-over.
        auto rs = renderSynced (proc, fs, 120.0, 16, 1.0f, 0.0f, 1.0f, cells, 900, oddBlock);
        std::map<std::tuple<int,int,int>, int> swCount;
        bool swPlaceOK = ! rs.hits.empty();
        for (const auto& h : rs.hits)
        {
            const juce::int64 nominalAbs = h.firedAbs - h.appliedOffsetSamples;
            const juce::int64 barIdx = (juce::int64) std::llround (
                (double) (nominalAbs - h.nominalSampleInBar) / r.samplesPerBar);
            swCount[ { h.voiceIndex, h.stepIndex, (int) barIdx } ]++;
            swPlaceOK = swPlaceOK && barRelativeOK (rs, h, 0) && midiHasHit (rs, h);
        }
        bool swOnce = true;
        for (auto& kv : swCount) swOnce = swOnce && (kv.second == 1);

        check ("block-boundary", placeOK && onceEach && swPlaceOK && swOnce,
               juce::String ("straight: ") + juce::String ((int) fireCount.size())
                 + " unique fires once=" + (onceEach ? "Y" : "N")
                 + " | swung once=" + (swOnce ? "Y" : "N"));
    }

    //==========================================================================
    // PROBE 6 — viz truth (QUAL-02 by construction).
    //==========================================================================
    {
        std::vector<std::pair<int,int>> cells;
        for (int s = 0; s < 16; ++s) { cells.push_back ({ Kick, s }); cells.push_back ({ Snare, s }); }
        // swing + humanize on, partial quantize -> non-trivial Δt on every hit.
        auto r = renderSynced (proc, fs, 120.0, 16, 0.6f, 0.7f, 0.3f, cells, 720, block); // ~4 bars

        // (a) Each emitted hit's viz Δt == the Δt baked into the real MidiMessage.
        bool vizTruth = ! r.hits.empty();
        for (const auto& h : r.hits)
        {
            vizTruth = vizTruth && ((h.appliedSampleInBar - h.nominalSampleInBar) == h.appliedOffsetSamples);
            vizTruth = vizTruth && midiHasHit (r, h);     // viz Δt corresponds to the actual note-on sample
        }

        // (b) The viz FIFO stream agrees with the emit capture (multiset of
        //     voice/step/nominal/applied). Two independent recordings must match.
        auto keyHit = [] (const CollHit& h) {
            return std::make_tuple ((int) h.voiceIndex, (int) h.stepIndex,
                                    (int) h.nominalSampleInBar, (int) h.appliedSampleInBar); };
        auto keyViz = [] (const VizEv& e) {
            return std::make_tuple ((int) e.voiceIndex, (int) e.stepIndex,
                                    (int) e.nominalSampleInBar, (int) e.appliedSampleInBar); };
        std::vector<std::tuple<int,int,int,int>> kh, kv;
        for (const auto& h : r.hits) kh.push_back (keyHit (h));
        for (const auto& e : r.viz)  kv.push_back (keyViz (e));
        std::sort (kh.begin(), kh.end());
        std::sort (kv.begin(), kv.end());
        const bool fifoAgrees = (kh == kv) && ! kh.empty();

        check ("viz-truth", vizTruth && fifoAgrees,
               juce::String ("hits=") + juce::String ((int) r.hits.size())
                 + " viz=" + juce::String ((int) r.viz.size())
                 + " fifoAgrees=" + (fifoAgrees ? "Y" : "N"));
    }

    //==========================================================================
    // PROBE 7 — mono/stereo gain parity (CR-02 regression): a mono render must
    // match one stereo channel's level, not double-add every voice (+6 dB).
    //==========================================================================
    {
        auto stereo = renderHost (proc, fs, block, 0.6, { { 0.0, 36, 110 } });
        const double rStereo = rms (stereo, 0, (int) (0.2 * fs));

        // Same kick hit through a 1-channel buffer (renderHost is stereo-only).
        proc.setPlayConfigDetails (0, 1, fs, block);
        proc.prepareToPlay (fs, block);
        setDefaults (proc);
        proc.clearGrid();
        proc.setPlayHead (nullptr);
        const int total = (int) (0.6 * fs);
        std::vector<float> mono; mono.reserve ((size_t) total);
        juce::AudioBuffer<float> mbuf (1, block);
        int pos = 0;
        while (pos < total)
        {
            mbuf.clear();
            juce::MidiBuffer midi;
            if (pos == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 110), 0);
            proc.processBlock (mbuf, midi);
            const int n = juce::jmin (block, total - pos);
            for (int i = 0; i < n; ++i) mono.push_back (mbuf.getSample (0, i));
            pos += block;
        }
        proc.setPlayConfigDetails (0, 2, fs, block);
        const double rMono = rms (mono, 0, (int) (0.2 * fs));
        const double ratio = rStereo > 0.0 ? rMono / rStereo : 0.0;
        check ("mono-parity", ratio > 0.9 && ratio < 1.1 && rMono > 0.003,
               juce::String ("stereoRMS=") + juce::String (rStereo, 4)
                 + " monoRMS=" + juce::String (rMono, 4)
                 + " ratio=" + juce::String (ratio, 3));
    }

    proc.releaseResources();

    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
