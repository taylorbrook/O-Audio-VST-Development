/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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

    O-Bowed render-harness — Phase 2.1b module-extraction regression baseline
    Ouaricon Audio
    Developer: Taylor Brook

    Mirrors plugins/O-Contrabass/tests/render-harness/main.cpp verbatim,
    substituting the processor class (OBowedAudioProcessor) and the
    canonical-preset CLI defaults (A4 / vel 0.7 / 5s / no release tail /
    no INFINITE_SUSTAIN flag — O-Bowed has no such APVTS parameter).

    Captures the golden reference WAV before R10/R12 friction-module
    extraction. The bit-exact `cmp` between pre- and post-extraction
    renders is Phase 2.1b Gate 2's pass-bar (PLAN rev-4 R14c).

    CLI:
      O-Bowed-render-test
        --note <midi=69>           (A4)
        --velocity <0..1=0.7>
        --sustain <sec=5>
        --release <sec=0>
        --out <wav=o-bowed-pre-extraction-canonical.wav>
        --json <json=o-bowed-pre-extraction-canonical.json>
        --brightness <norm>        (v1.9.1, WR-05 pitch check)
        --ne-semis <semitones>     (v1.9.1, WR-01: seed a Note Expression tuning
                                    delta for --note before note-on)
        --bend-vibrato <sec>       (v1.9.1, CR-02: from 1.0 s, one pitch-wheel
                                    message per block, 6 Hz / +-1 semitone, then
                                    back to centre)
        --sample-rate <hz=44100>   (v1.9.2, CR-01: the waveguide runs at 2x this)
        --param <id>=<norm>        (v1.9.2, CR-01: pin any APVTS parameter;
                                    repeatable, applied before the named flags,
                                    so a whole factory preset can be rendered)

    Pass-conditions (exit 0):
      - No NaN / Inf samples.
      - Peak ≤ 0 dBFS (|sample| ≤ 1.0f).
      - Max-block-time ratio (max / median) ≤ 5.0× — denormal-spike sentinel.

    The pass_rms invariant is intentionally dropped (RESEARCH §13.5):
    bow-on/bow-off envelope at A4 / 5 s is too short for a meaningful
    s5–s6 vs final-second ratio. The bit-exact `cmp` of the WAV is the
    actual gate; this JSON exists for traceability.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"
#include "ScaleGenerator.h"

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
    int   midiNote          = 69;     // A4 (canonical preset)
    float velocity          = 0.7f;
    float sustainSeconds    = 5.0f;
    float releaseSeconds    = 0.0f;
    juce::String outWav     = "o-bowed-pre-extraction-canonical.wav";
    juce::String outJson    = "o-bowed-pre-extraction-canonical.json";

    // Phase 2.4c R36b — Option B value-consume flags (RESEARCH §19.4.3 + PLAN
    // rev-10 pin #2). Sentinel -1.0f = "unset, use factory APVTS default".
    // Values are 0..1 normalized (`setValueNotifyingHost` consumes norm form
    // directly). When ALL four are unset, behaviour is identical to HEAD —
    // factory defaults consumed verbatim — preserving canonical-preset.wav
    // sha256 byte-identical (Risk #13 mitigation).
    float bowSpeedNorm        = -1.0f;
    float bowPressureNorm     = -1.0f;
    float bowPositionNorm     = -1.0f;
    float infiniteSustainNorm = -1.0f;

    // v1.9.1 pitch-path flags. Unset = HEAD behaviour (golden stays byte-identical).
    float brightnessNorm      = -1.0f;
    float neSemis             = 0.0f;
    float bendVibratoSeconds  = 0.0f;

    // v1.9.2 register gate flags (CR-01). Unset = HEAD behaviour.
    double sampleRate         = 44100.0;
    juce::StringArray params;           // "id=norm"

    // v1.9.3 tuning-ownership flags (CR-03, WR-10). 0 = unset (HEAD behaviour).
    int edo                   = 0;      // apply an N-EDO scale the way the panel does
    int roundtripEdo          = 0;      // save -> restore -> apply M-EDO -> save -> restore
};

// Mirrors the editor's applyGeneratedScale native function exactly.
void applyEdo (OBowedAudioProcessor& p, int divisions)
{
    p.getTuningEngine()->setCustomIntervals (ScaleGenerator::generateEDO (divisions, 1200.0),
                                             juce::String (divisions) + "-EDO");
    p.selectTuningSystem (0);
}

// getStateInformation -> fresh instance -> setStateInformation, as a DAW reopen.
std::unique_ptr<OBowedAudioProcessor> reopen (OBowedAudioProcessor& from)
{
    juce::MemoryBlock state;
    from.getStateInformation (state);
    auto to = std::make_unique<OBowedAudioProcessor>();
    to->setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    return to;
}

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
        else if (key == "--out")              args.outWav          = val;
        else if (key == "--json")             args.outJson         = val;
        // Phase 2.4c R36b — Option B value-consume flags. Mirrors O-Contrabass
        // --infinite-sustain pattern (O-Contrabass main.cpp:218). 0..1 norm.
        else if (key == "--bow-speed")        args.bowSpeedNorm        = val.getFloatValue();
        else if (key == "--bow-pressure")     args.bowPressureNorm     = val.getFloatValue();
        else if (key == "--bow-position")     args.bowPositionNorm     = val.getFloatValue();
        else if (key == "--infinite-sustain") args.infiniteSustainNorm = val.getFloatValue();
        else if (key == "--brightness")       args.brightnessNorm      = val.getFloatValue();
        else if (key == "--ne-semis")         args.neSemis             = val.getFloatValue();
        else if (key == "--bend-vibrato")     args.bendVibratoSeconds  = val.getFloatValue();
        else if (key == "--sample-rate")      args.sampleRate          = val.getDoubleValue();
        else if (key == "--param")            args.params.add (val);
        else if (key == "--edo")              args.edo                 = val.getIntValue();
        else if (key == "--roundtrip-edo")    args.roundtripEdo        = val.getIntValue();
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

    const double sampleRate = args.sampleRate;
    constexpr int    blockSize  = 512;

    OBowedAudioProcessor proc;

    proc.setPlayConfigDetails (/*numIns*/ 0, /*numOuts*/ 2, sampleRate, blockSize);
    proc.prepareToPlay (sampleRate, blockSize);

    // Phase 2.4c R36b — Option B sentinel-conditional APVTS pinning. When ALL
    // four flags are unset (sentinel -1.0f), behaviour is identical to HEAD
    // (factory APVTS defaults consumed verbatim → canonical-preset.wav
    // byte-identical). When set, pinning happens AFTER prepareToPlay so the
    // first processBlock observes the override (mirrors O-Contrabass pattern).
    auto pinNorm = [&proc] (const char* paramId, float norm)
    {
        if (auto* p = proc.getAPVTS().getParameter (paramId))
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
    };
    for (const auto& kv : args.params)
    {
        const auto id = kv.upToFirstOccurrenceOf ("=", false, false);
        if (proc.getAPVTS().getParameter (id) == nullptr)
        {
            std::fprintf (stderr, "Unknown --param id: %s\n", id.toRawUTF8());
            return 2;
        }
        pinNorm (id.toRawUTF8(), kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue());
    }
    if (args.bowSpeedNorm        >= 0.0f) pinNorm ("bowSpeed",        args.bowSpeedNorm);
    if (args.bowPressureNorm     >= 0.0f) pinNorm ("bowPressure",     args.bowPressureNorm);
    if (args.bowPositionNorm     >= 0.0f) pinNorm ("bowPosition",     args.bowPositionNorm);
    if (args.infiniteSustainNorm >= 0.0f) pinNorm ("infiniteSustain", args.infiniteSustainNorm);
    if (args.brightnessNorm      >= 0.0f) pinNorm ("brightness",      args.brightnessNorm);

    // WR-01: seed the NE pending slot the way drainAndUpdate() would for a
    // Dorico kTuningTypeID event arriving with the note-on.
    if (args.neSemis != 0.0f)
        if (auto* ne = dynamic_cast<Ouaricon::NoteExpression::VST3Extensions*> (proc.getVST3ClientExtensions()))
            ne->getPendingTable()[(size_t) juce::jlimit (0, 127, args.midiNote)].store ((double) args.neSemis);

    // v1.9.3 (CR-03): a scale applied through the panel's own path must be heard.
    if (args.edo > 0)
        applyEdo (proc, args.edo);

    // v1.9.3 (WR-10): reopen, change the tuning, reopen again, render the last
    // instance. The second reopen is the one a stale <CustomState> child in the
    // restored tree would break.
    std::unique_ptr<OBowedAudioProcessor> reopened;
    if (args.roundtripEdo > 0)
    {
        auto middle = reopen (proc);
        applyEdo (*middle, args.roundtripEdo);
        reopened = reopen (*middle);
        reopened->setPlayConfigDetails (0, 2, sampleRate, blockSize);
        reopened->prepareToPlay (sampleRate, blockSize);
    }
    auto& renderProc = reopened != nullptr ? *reopened : proc;

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

        // CR-02: a held-note vibrato as a stream of pitch-wheel messages (legacy
        // mode, +-2 st range, so +-4096 = +-1 st), then a final centre message.
        if (args.bendVibratoSeconds > 0.0f)
        {
            const int vibStart = static_cast<int> (1.0 * sampleRate);
            const int vibEnd   = vibStart + static_cast<int> (args.bendVibratoSeconds * sampleRate);
            if (sampleCursor >= vibStart && sampleCursor < vibEnd)
            {
                const double t = (sampleCursor - vibStart) / sampleRate;
                const int wheel = 8192 + static_cast<int> (std::round (4096.0 * std::sin (2.0 * juce::MathConstants<double>::pi * 6.0 * t)));
                midi.addEvent (juce::MidiMessage::pitchWheel (channel, juce::jlimit (0, 16383, wheel)), 0);
            }
            else if (sampleCursor >= vibEnd && sampleCursor - thisBlock < vibEnd)
            {
                midi.addEvent (juce::MidiMessage::pitchWheel (channel, 8192), 0);
            }
        }

        const auto t0 = std::chrono::steady_clock::now();
        renderProc.processBlock (blockBuffer, midi);
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

    const bool overallPass = passNan && passPeak && passBlockTime;

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
    summary->setProperty ("midiNote",               args.midiNote);
    summary->setProperty ("velocity",               args.velocity);
    summary->setProperty ("sustainSeconds",         args.sustainSeconds);
    summary->setProperty ("releaseSeconds",         args.releaseSeconds);
    summary->setProperty ("totalSamples",           totalSamples);
    summary->setProperty ("peak",                   peak);
    summary->setProperty ("nanCount",               nanCount);
    summary->setProperty ("infCount",               infCount);
    summary->setProperty ("blockMicros_median",     medianMicros);
    summary->setProperty ("blockMicros_max",        maxMicros);
    summary->setProperty ("blockTime_max_over_median", maxRatio);
    summary->setProperty ("pass_nan",               passNan);
    summary->setProperty ("pass_peak",              passPeak);
    summary->setProperty ("pass_blockTime",         passBlockTime);
    summary->setProperty ("outputWav",              args.outWav);

    // v1.9.3: what the rendering instance's tuning says the note should be.
    // The engine is held at A4 = 440 and the voice applies referencePitch.
    {
        auto& apvts = renderProc.getAPVTS();
        const double refHz = apvts.getRawParameterValue ("referencePitch")->load();
        const double engineHz = renderProc.getTuningEngine()->getFrequency (args.midiNote);
        summary->setProperty ("tuningSystem",   static_cast<int> (apvts.getRawParameterValue ("tuningSystem")->load()));
        summary->setProperty ("tuningName",     renderProc.getTuningEngine()->getActiveTuningName());
        summary->setProperty ("referencePitch", refHz);
        summary->setProperty ("engineHz",       engineHz);
        summary->setProperty ("expectedHz",     engineHz * refHz / 440.0);
    }

    juce::var summaryVar (summary.get());
    juce::File jsonOut (juce::File::getCurrentWorkingDirectory().getChildFile (args.outJson));
    jsonOut.replaceWithText (juce::JSON::toString (summaryVar, /*allOnOneLine*/ false));

    std::printf ("[render-harness] %s  peak=%.3f "
                 "nan=%d inf=%d maxRatio=%.2f\n",
                 overallPass ? "PASS" : "FAIL",
                 peak, nanCount, infCount, maxRatio);

    return overallPass ? 0 : 1;
}
