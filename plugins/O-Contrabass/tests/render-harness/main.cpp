/*
  ==============================================================================

    O-Contrabass render-harness — Phase 2.1 stability test
    Ouaricon Audio
    Developer: Taylor Brook

    RESEARCH §Q1 + PLAN.md Task 7. Drives a single sustained note through the
    OContrabassAudioProcessor, accumulates output to WAV, writes a JSON
    summary covering: peak level, NaN/Inf counter, RMS curve at 1s windows,
    and per-block wall-clock ratio (5× median = denormal-spike sentinel).

    CLI:
      O-Contrabass-render-test
        --note <midi=28>
        --velocity <0..1=0.7>
        --sustain <sec=60>
        --release <sec=5>
        --infinite-sustain <0..1=1.0>
        --string-stiffness <0..1=apvts>   (Phase 2.1c R16-pre; sentinel <0 = use APVTS factory default)
        --stiffness-sweep <0|1=0>         (Phase 2.1c R18; ramps STRING_STIFFNESS 0→1 across the sustain phase)
        --out <wav=e1-max-sustain.wav>
        --json <json=e1-max-sustain.json>

    Pass-conditions (exit 0):
      - No NaN / Inf samples.
      - Peak ≤ 0 dBFS (|sample| ≤ 1.0f).
      - Max-block-time ratio (max / median) ≤ 5.0× — denormal-spike sentinel.
      - RMS over the final 1 s within 0.5–2.0× of RMS over seconds 5–6
        (runaway/dieout detection).

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{
struct Args
{
    int   midiNote          = 28;
    float velocity          = 0.7f;
    float sustainSeconds    = 60.0f;
    float releaseSeconds    = 5.0f;
    float infiniteSustain   = 1.0f;
    float stringStiffness   = -1.0f;   // sentinel: <0 = unset, use APVTS factory default
    bool  stiffnessSweep    = false;   // Phase 2.1c R18: linear 0→1 ramp across sustain phase
    juce::String outWav     = "e1-max-sustain.wav";
    juce::String outJson    = "e1-max-sustain.json";
};

bool parseArgs (int argc, char** argv, Args& args)
{
    for (int i = 1; i < argc; ++i)
    {
        juce::String key (argv[i]);
        if (i + 1 >= argc)
        {
            std::fprintf (stderr, "Missing value for %s\n", argv[i]);
            return false;
        }
        juce::String val (argv[++i]);

        if      (key == "--note")             args.midiNote        = val.getIntValue();
        else if (key == "--velocity")         args.velocity        = val.getFloatValue();
        else if (key == "--sustain")          args.sustainSeconds  = val.getFloatValue();
        else if (key == "--release")          args.releaseSeconds  = val.getFloatValue();
        else if (key == "--infinite-sustain") args.infiniteSustain = val.getFloatValue();
        else if (key == "--string-stiffness") args.stringStiffness = val.getFloatValue();
        else if (key == "--stiffness-sweep")  args.stiffnessSweep  = (val.getIntValue() != 0);
        else if (key == "--out")              args.outWav          = val;
        else if (key == "--json")             args.outJson         = val;
        else
        {
            std::fprintf (stderr, "Unknown arg: %s\n", argv[i - 1]);
            return false;
        }
    }
    return true;
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // safe in console too

    Args args;
    if (! parseArgs (argc, argv, args))
        return 2;

    // Phase 2.1c R18: in sweep mode, rewrite default output filenames so the
    // user doesn't have to pass --out / --json explicitly (mirrors README).
    if (args.stiffnessSweep)
    {
        if (args.outWav  == "e1-max-sustain.wav")  args.outWav  = "e1-stiffness-sweep.wav";
        if (args.outJson == "e1-max-sustain.json") args.outJson = "e1-stiffness-sweep.json";
    }

    constexpr double sampleRate = 44100.0;
    constexpr int    blockSize  = 512;

    OContrabassAudioProcessor proc;

    // Override INFINITE_SUSTAIN parameter before prepareToPlay so the voice
    // sees the harness-driven value on noteStarted.
    if (auto* infParam = proc.parameters.getParameter ("INFINITE_SUSTAIN"))
    {
        const float clamped = juce::jlimit (0.0f, 1.0f, args.infiniteSustain);
        infParam->setValueNotifyingHost (clamped);
    }

    // Phase 2.1c R16-pre: optional STRING_STIFFNESS override (sentinel <0 = unset).
    // In sweep mode, this is a starting value that gets immediately ramped over
    // by the per-block setValueNotifyingHost in the render loop below.
    if (args.stringStiffness >= 0.0f)
    {
        if (auto* p = proc.parameters.getParameter ("STRING_STIFFNESS"))
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, args.stringStiffness));
    }

    proc.setPlayConfigDetails (/*numIns*/ 0, /*numOuts*/ 2, sampleRate, blockSize);
    proc.prepareToPlay (sampleRate, blockSize);

    const int totalSeconds   = static_cast<int> (std::ceil (args.sustainSeconds + args.releaseSeconds));
    const int totalSamples   = static_cast<int> (totalSeconds * sampleRate);
    const int sustainSamples = static_cast<int> (args.sustainSeconds * sampleRate);

    // Output accumulator (stereo)
    juce::AudioBuffer<float> output (2, totalSamples);
    output.clear();

    juce::AudioBuffer<float> blockBuffer (2, blockSize);

    // Block timing — wall clock per block to detect denormal CPU spikes.
    std::vector<double> blockMicros;
    blockMicros.reserve (static_cast<size_t> ((totalSamples / blockSize) + 8));

    int nanCount = 0;
    int infCount = 0;

    int sampleCursor = 0;
    bool noteOnSent  = false;
    bool noteOffSent = false;

    const int channel  = 1;       // MPE legacy zone master channel
    const int velMidi  = juce::jlimit (1, 127, static_cast<int> (std::round (args.velocity * 127.0f)));

    while (sampleCursor < totalSamples)
    {
        const int thisBlock = std::min (blockSize, totalSamples - sampleCursor);
        blockBuffer.setSize (2, thisBlock, /*keep*/ false, /*clear*/ true, /*avoidRealloc*/ true);
        blockBuffer.clear();

        // Phase 2.1c R18: per-block STRING_STIFFNESS ramp (linear 0→1 across sustain phase).
        if (args.stiffnessSweep)
        {
            const float fraction      = static_cast<float> (sampleCursor)
                                      / static_cast<float> (juce::jmax (1, sustainSamples));
            const float stiffnessNorm = juce::jlimit (0.0f, 1.0f, fraction);
            if (auto* p = proc.parameters.getParameter ("STRING_STIFFNESS"))
                p->setValueNotifyingHost (stiffnessNorm);
        }

        juce::MidiBuffer midi;

        if (! noteOnSent)
        {
            midi.addEvent (juce::MidiMessage::noteOn (channel, args.midiNote, (juce::uint8) velMidi), 0);
            noteOnSent = true;
        }

        if (! noteOffSent && sampleCursor + thisBlock > sustainSamples)
        {
            const int offOffset = juce::jlimit (0, thisBlock - 1, sustainSamples - sampleCursor);
            midi.addEvent (juce::MidiMessage::noteOff (channel, args.midiNote), offOffset);
            noteOffSent = true;
        }

        const auto t0 = std::chrono::steady_clock::now();
        proc.processBlock (blockBuffer, midi);
        const auto t1 = std::chrono::steady_clock::now();
        blockMicros.push_back (std::chrono::duration<double, std::micro> (t1 - t0).count());

        // Copy into accumulator + scan for non-finite samples.
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto* src = blockBuffer.getReadPointer (ch);
            auto* dst       = output.getWritePointer (ch, sampleCursor);
            for (int i = 0; i < thisBlock; ++i)
            {
                const float s = src[i];
                if (std::isnan (s)) ++nanCount;
                else if (std::isinf (s)) ++infCount;
                dst[i] = s;
            }
        }

        sampleCursor += thisBlock;
    }

    // ---- Analyse ----
    float peak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto* p = output.getReadPointer (ch);
        for (int i = 0; i < totalSamples; ++i)
            peak = std::max (peak, std::abs (p[i]));
    }

    auto rmsOverWindow = [&] (int startSec, int endSec) -> float
    {
        const int s0 = juce::jlimit (0, totalSamples, static_cast<int> (startSec * sampleRate));
        const int s1 = juce::jlimit (s0, totalSamples, static_cast<int> (endSec * sampleRate));
        if (s1 <= s0) return 0.0f;
        double sumSq = 0.0;
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto* p = output.getReadPointer (ch);
            for (int i = s0; i < s1; ++i)
                sumSq += static_cast<double> (p[i]) * p[i];
        }
        const double n = static_cast<double> ((s1 - s0) * 2);
        return static_cast<float> (std::sqrt (sumSq / n));
    };

    const float rmsMid    = rmsOverWindow (5, 6);                                              // s5–s6
    const float rmsFinal  = rmsOverWindow (totalSeconds - 1, totalSeconds);                    // last 1s

    // Block-time stats — median + max for the spike sentinel.
    auto sortedTimes = blockMicros;
    std::sort (sortedTimes.begin(), sortedTimes.end());
    const double medianMicros = sortedTimes.empty() ? 0.0 : sortedTimes[sortedTimes.size() / 2];
    const double maxMicros    = sortedTimes.empty() ? 0.0 : sortedTimes.back();
    const double maxRatio     = (medianMicros > 0.0) ? (maxMicros / medianMicros) : 0.0;

    // ---- Pass conditions ----
    const bool passNan       = (nanCount == 0 && infCount == 0);
    const bool passPeak      = (peak <= 1.0f);
    const bool passBlockTime = (blockMicros.size() < 8) || (maxRatio <= 5.0);
    bool passRms = true;
    if (rmsMid > 1e-9f)
    {
        const float ratio = rmsFinal / rmsMid;
        passRms = (ratio >= 0.5f && ratio <= 2.0f);
    }
    else
    {
        // No mid-window energy ⇒ engine never started; harness fails.
        passRms = false;
    }

    const bool overallPass = passNan && passPeak && passBlockTime && passRms;

    // ---- Write WAV ----
    juce::File wavOut (juce::File::getCurrentWorkingDirectory().getChildFile (args.outWav));
    wavOut.deleteFile();
    juce::WavAudioFormat wav;
    if (auto stream = std::unique_ptr<juce::FileOutputStream> (wavOut.createOutputStream()))
    {
        if (auto* writer = wav.createWriterFor (stream.get(), sampleRate, 2, 24, {}, 0))
        {
            stream.release();   // writer owns the stream now
            std::unique_ptr<juce::AudioFormatWriter> w (writer);
            w->writeFromAudioSampleBuffer (output, 0, totalSamples);
        }
    }

    // ---- Write JSON summary ----
    juce::DynamicObject::Ptr summary (new juce::DynamicObject());
    summary->setProperty ("status",                 overallPass ? "PASS" : "FAIL");
    summary->setProperty ("mode",                   args.stiffnessSweep ? "stiffness-sweep" : "sustained-note");
    summary->setProperty ("midiNote",               args.midiNote);
    summary->setProperty ("velocity",               args.velocity);
    summary->setProperty ("sustainSeconds",         args.sustainSeconds);
    summary->setProperty ("releaseSeconds",         args.releaseSeconds);
    summary->setProperty ("infiniteSustain",        args.infiniteSustain);
    summary->setProperty ("stringStiffness",        args.stringStiffness);
    summary->setProperty ("totalSamples",           totalSamples);
    summary->setProperty ("peak",                   peak);
    summary->setProperty ("nanCount",               nanCount);
    summary->setProperty ("infCount",               infCount);
    summary->setProperty ("rmsMid_s5_s6",           rmsMid);
    summary->setProperty ("rmsFinal_lastSecond",    rmsFinal);
    summary->setProperty ("rmsRatio_final_over_mid", (rmsMid > 1e-9f ? rmsFinal / rmsMid : 0.0f));
    summary->setProperty ("blockMicros_median",     medianMicros);
    summary->setProperty ("blockMicros_max",        maxMicros);
    summary->setProperty ("blockTime_max_over_median", maxRatio);
    summary->setProperty ("pass_nan",               passNan);
    summary->setProperty ("pass_peak",              passPeak);
    summary->setProperty ("pass_blockTime",         passBlockTime);
    summary->setProperty ("pass_rms",               passRms);
    summary->setProperty ("outputWav",              args.outWav);

    // Phase 2.1c R18: sweep-mode JSON extras (mode + stiffnessRamp + rmsByDecade).
    if (args.stiffnessSweep)
    {
        juce::DynamicObject::Ptr ramp (new juce::DynamicObject());
        ramp->setProperty ("start", 0.0);
        ramp->setProperty ("end",   1.0);
        ramp->setProperty ("shape", "linear");
        summary->setProperty ("stiffnessRamp", juce::var (ramp.get()));

        // 10 deciles of the sustain phase (release excluded). RMS per decile.
        juce::Array<juce::var> decades;
        const int decileSamples = juce::jmax (1, sustainSamples / 10);
        for (int d = 0; d < 10; ++d)
        {
            const int s0 = d * decileSamples;
            const int s1 = juce::jmin (sustainSamples, (d + 1) * decileSamples);
            double sumSq = 0.0;
            int    count = 0;
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* p = output.getReadPointer (ch);
                for (int i = s0; i < s1; ++i)
                {
                    sumSq += static_cast<double> (p[i]) * p[i];
                    ++count;
                }
            }
            const double rms = (count > 0) ? std::sqrt (sumSq / static_cast<double> (count)) : 0.0;
            decades.add (juce::var (static_cast<double> (rms)));
        }
        summary->setProperty ("rmsByDecade", juce::var (decades));
    }

    juce::var summaryVar (summary.get());
    juce::File jsonOut (juce::File::getCurrentWorkingDirectory().getChildFile (args.outJson));
    jsonOut.replaceWithText (juce::JSON::toString (summaryVar, /*allOnOneLine*/ false));

    std::printf ("[render-harness] %s  peak=%.3f rmsMid=%.4f rmsFinal=%.4f "
                 "nan=%d inf=%d maxRatio=%.2f\n",
                 overallPass ? "PASS" : "FAIL",
                 peak, rmsMid, rmsFinal, nanCount, infCount, maxRatio);

    return overallPass ? 0 : 1;
}
