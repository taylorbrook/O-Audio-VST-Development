/*
  ==============================================================================

    O-Contrabass render-harness — Phase 2.1/2.2 stability + audit test
    Ouaricon Audio
    Developer: Taylor Brook

    RESEARCH §Q1 + PLAN.md Task 7 (Phase 2.1) + RESEARCH §15.7-§15.11 (Phase 2.2).

    Drives notes through OContrabassAudioProcessor, accumulates output to WAV,
    writes a JSON summary covering: peak level, NaN/Inf counter, RMS curves,
    per-block wall-clock ratio (5× median = denormal-spike sentinel), and
    Phase 2.2 audit metrics for detune-sweep + note-sequence modes.

    CLI:
      O-Contrabass-render-test
        --note <midi=28>
        --velocity <0..1=0.7>
        --sustain <sec=60>
        --release <sec=5>
        --infinite-sustain <0..1=1.0>
        --string-stiffness <0..1=apvts>   (Phase 2.1c R16-pre; sentinel <0 = use APVTS factory default)
        --stiffness-sweep <0|1=0>         (Phase 2.1c R18; ramps STRING_STIFFNESS 0→1 across the sustain phase)
        --string <E|A|D|G>                (Phase 2.2 R23; overrides --note to open-string MIDI 28/33/38/43)
        --detune-sweep <E|A|D|G>          (Phase 2.2 R23; ramps DETUNE_<X> from −1200→+1200¢ across the sustain phase)
        --note-sequence "MIDI:dur,..."    (Phase 2.2 R23; pre-built sequence, e.g. "28:1.5,33:1.5,38:1.5,43:1.5,28:1.5")
        --out <wav=e1-max-sustain.wav>
        --json <json=e1-max-sustain.json>

    Auto-rewrite of default --out / --json:
      --string <X>            : string-<X>.wav / string-<X>.json
      --detune-sweep <X>      : detune-sweep-<X>.wav / detune-sweep-<X>.json
      --note-sequence ...     : note-sequence.wav / note-sequence.json

    Pass-conditions (exit 0):
      - sustained / stiffness-sweep: pass_nan && pass_peak && pass_blockTime && pass_rms.
      - detune-sweep:                pass_nan && pass_peak && pass_blockTime && pass_rmsContinuity (≥0.90).
      - note-sequence:               pass_nan && pass_peak && pass_blockTime
                                     && pass_allSegmentsAudible (per-segment RMS > 1e-3)
                                     && pass_rmsContinuityAtTransitions (≥0.50, 256-sample symmetric window).

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
constexpr int   kDefaultNote     = 28;
constexpr float kDefaultSustain  = 60.0f;
constexpr float kDefaultRelease  = 5.0f;

struct Args
{
    int   midiNote          = kDefaultNote;
    float velocity          = 0.7f;
    float sustainSeconds    = kDefaultSustain;
    float releaseSeconds    = kDefaultRelease;
    float infiniteSustain   = 1.0f;
    float stringStiffness   = -1.0f;   // sentinel: <0 = unset, use APVTS factory default
    bool  stiffnessSweep    = false;   // Phase 2.1c R18
    int   activeStrings     = -1;      // Phase 2.2 R23: sentinel <0 = use APVTS factory default (=4)
    char  stringOverride    = ' ';     // Phase 2.2 R23: 'E','A','D','G' or ' '
    char  detuneSweepString = ' ';     // Phase 2.2 R23: 'E','A','D','G' or ' '
    juce::String noteSequence;         // Phase 2.2 R23: "MIDI:dur,..." or empty

    bool         outWavSet   = false;
    bool         outJsonSet  = false;
    bool         sustainSet  = false;
    bool         releaseSet  = false;
    bool         noteSet     = false;

    juce::String outWav     = "e1-max-sustain.wav";
    juce::String outJson    = "e1-max-sustain.json";
};

static char parseStringLetter (const juce::String& s)
{
    const auto upper = s.toUpperCase();
    if (upper == "E") return 'E';
    if (upper == "A") return 'A';
    if (upper == "D") return 'D';
    if (upper == "G") return 'G';
    std::fprintf (stderr, "warning: --string / --detune-sweep arg '%s' invalid; ignoring\n",
                  s.toRawUTF8());
    return ' ';
}

static int impliedNoteForString (char letter)
{
    return (letter == 'E') ? 28
         : (letter == 'A') ? 33
         : (letter == 'D') ? 38
         : (letter == 'G') ? 43
         : kDefaultNote;
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

        if      (key == "--note")             { args.midiNote        = val.getIntValue();   args.noteSet     = true; }
        else if (key == "--velocity")         { args.velocity        = val.getFloatValue(); }
        else if (key == "--sustain")          { args.sustainSeconds  = val.getFloatValue(); args.sustainSet  = true; }
        else if (key == "--release")          { args.releaseSeconds  = val.getFloatValue(); args.releaseSet  = true; }
        else if (key == "--infinite-sustain") { args.infiniteSustain = val.getFloatValue(); }
        else if (key == "--string-stiffness") { args.stringStiffness = val.getFloatValue(); }
        else if (key == "--stiffness-sweep")  { args.stiffnessSweep  = (val.getIntValue() != 0); }
        else if (key == "--active-strings")   { args.activeStrings   = val.getIntValue(); }
        else if (key == "--string")           { args.stringOverride    = parseStringLetter (val); }
        else if (key == "--detune-sweep")     { args.detuneSweepString = parseStringLetter (val); }
        else if (key == "--note-sequence")    { args.noteSequence      = val; }
        else if (key == "--out")              { args.outWav          = val; args.outWavSet  = true; }
        else if (key == "--json")             { args.outJson         = val; args.outJsonSet = true; }
        else
        {
            std::fprintf (stderr, "Unknown arg: %s\n", argv[i - 1]);
            return false;
        }
    }
    return true;
}

struct ScheduledMidiEvent
{
    int sampleIndex;
    juce::MidiMessage message;
};

struct SequenceSegment
{
    int sampleStart = 0;
    int sampleCount = 0;
    int midiNote    = 0;
};
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // safe in console too

    Args args;
    if (! parseArgs (argc, argv, args))
        return 2;

    // Phase 2.2 R23: --string and --detune-sweep are mutually-exclusive in effect.
    if (args.stringOverride != ' ' && args.detuneSweepString != ' ')
    {
        std::fprintf (stderr, "warning: --string and --detune-sweep both set; --detune-sweep takes precedence\n");
        args.stringOverride = ' ';
    }

    // Phase 2.2 R23: --string overrides --note to the matching open-string MIDI.
    if (args.stringOverride != ' ')
    {
        const int impliedNote = impliedNoteForString (args.stringOverride);
        if (args.noteSet && args.midiNote != impliedNote)
            std::fprintf (stderr, "warning: --string=%c overrides --note=%d\n",
                          args.stringOverride, args.midiNote);
        args.midiNote = impliedNote;
    }

    // Phase 2.2 R23: --detune-sweep also overrides --note to the matching open-string,
    // and switches the sustain/release defaults to 30 s / 2 s if user didn't specify.
    if (args.detuneSweepString != ' ')
    {
        const int impliedNote = impliedNoteForString (args.detuneSweepString);
        if (args.noteSet && args.midiNote != impliedNote)
            std::fprintf (stderr, "warning: --detune-sweep=%c overrides --note=%d\n",
                          args.detuneSweepString, args.midiNote);
        args.midiNote = impliedNote;
        if (! args.sustainSet) args.sustainSeconds = 30.0f;
        if (! args.releaseSet) args.releaseSeconds = 2.0f;
    }

    // Auto-rewrite default WAV/JSON filenames per mode (Phase 2.1c R18 + Phase 2.2 R23).
    if (args.stiffnessSweep)
    {
        if (! args.outWavSet)  args.outWav  = "e1-stiffness-sweep.wav";
        if (! args.outJsonSet) args.outJson = "e1-stiffness-sweep.json";
    }
    else if (args.detuneSweepString != ' ')
    {
        if (! args.outWavSet)
            args.outWav  = juce::String ("detune-sweep-") + juce::String::charToString (args.detuneSweepString) + ".wav";
        if (! args.outJsonSet)
            args.outJson = juce::String ("detune-sweep-") + juce::String::charToString (args.detuneSweepString) + ".json";
    }
    else if (args.noteSequence.isNotEmpty())
    {
        if (! args.outWavSet)  args.outWav  = "note-sequence.wav";
        if (! args.outJsonSet) args.outJson = "note-sequence.json";
    }
    else if (args.stringOverride != ' ')
    {
        if (! args.outWavSet)
            args.outWav  = juce::String ("string-") + juce::String::charToString (args.stringOverride) + ".wav";
        if (! args.outJsonSet)
            args.outJson = juce::String ("string-") + juce::String::charToString (args.stringOverride) + ".json";
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

    // Phase 2.2 R23: optional ACTIVE_STRINGS override (sentinel <0 = factory default = 4).
    if (args.activeStrings >= 1 && args.activeStrings <= 4)
    {
        if (auto* p = proc.parameters.getParameter ("ACTIVE_STRINGS"))
        {
            // Int parameter range is [1,4]; normalised value = (val - 1) / 3.
            const float norm = static_cast<float> (args.activeStrings - 1) / 3.0f;
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
        }
    }

    proc.setPlayConfigDetails (/*numIns*/ 0, /*numOuts*/ 2, sampleRate, blockSize);
    proc.prepareToPlay (sampleRate, blockSize);

    int totalSeconds   = static_cast<int> (std::ceil (args.sustainSeconds + args.releaseSeconds));
    int totalSamples   = static_cast<int> (totalSeconds * sampleRate);
    int sustainSamples = static_cast<int> (args.sustainSeconds * sampleRate);

    const int channel  = 1;       // MPE legacy zone master channel
    const int velMidi  = juce::jlimit (1, 127, static_cast<int> (std::round (args.velocity * 127.0f)));

    // Phase 2.2 R23 — pre-build note-sequence event list, override sustain/total
    // sample counts when in sequence mode (sustain phase = sum of segment durations).
    std::vector<ScheduledMidiEvent> sequenceEvents;
    std::vector<SequenceSegment>    sequenceSegments;
    std::vector<int>                transitionSampleIndices;

    if (args.noteSequence.isNotEmpty())
    {
        juce::StringArray segments;
        segments.addTokens (args.noteSequence, ",", "");

        int cursor = 0;
        int prevNote = -1;
        for (const auto& segment : segments)
        {
            const int colon = segment.indexOfChar (':');
            if (colon < 0)
            {
                std::fprintf (stderr, "warning: malformed segment '%s'; skipping\n",
                              segment.toRawUTF8());
                continue;
            }
            const int   noteVal    = segment.substring (0, colon).getIntValue();
            const float dur        = segment.substring (colon + 1).getFloatValue();
            const int   durSamples = static_cast<int> (dur * sampleRate);
            if (durSamples <= 0) continue;

            sequenceEvents.push_back ({ cursor,                  juce::MidiMessage::noteOn  (channel, noteVal, (juce::uint8) velMidi) });
            sequenceEvents.push_back ({ cursor + durSamples - 1, juce::MidiMessage::noteOff (channel, noteVal) });

            SequenceSegment seg;
            seg.sampleStart = cursor;
            seg.sampleCount = durSamples;
            seg.midiNote    = noteVal;
            sequenceSegments.push_back (seg);

            if (prevNote >= 0)
                transitionSampleIndices.push_back (cursor);
            prevNote = noteVal;

            cursor += durSamples;
        }

        if (args.sustainSet)
            std::fprintf (stderr, "warning: --sustain ignored in --note-sequence mode\n");

        sustainSamples = cursor;
        totalSamples   = cursor + static_cast<int> (args.releaseSeconds * sampleRate);
        totalSeconds   = static_cast<int> (std::ceil (static_cast<float> (totalSamples) / sampleRate));
    }

    // Output accumulator (stereo)
    juce::AudioBuffer<float> output (2, totalSamples);
    output.clear();

    juce::AudioBuffer<float> blockBuffer (2, blockSize);

    // Block timing — wall clock per block to detect denormal CPU spikes.
    std::vector<double> blockMicros;
    blockMicros.reserve (static_cast<size_t> ((totalSamples / blockSize) + 8));

    // Phase 2.2 R23 — per-block sustain-phase RMS for continuity audits.
    std::vector<float> blockRmsHistory;
    blockRmsHistory.reserve (static_cast<size_t> ((sustainSamples / blockSize) + 8));

    int nanCount = 0;
    int infCount = 0;

    int sampleCursor = 0;
    bool noteOnSent  = false;
    bool noteOffSent = false;

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

        // Phase 2.2 R23: per-block detune-sweep ramp (linear −1200→+1200¢ across sustain phase).
        if (args.detuneSweepString != ' ')
        {
            const float fraction = juce::jlimit (0.0f, 1.0f,
                                                 static_cast<float> (sampleCursor)
                                                 / static_cast<float> (juce::jmax (1, sustainSamples)));
            const float cents    = -1200.0f + 2400.0f * fraction;
            const float norm     = (cents + 1200.0f) / 2400.0f;

            const juce::String paramId = (args.detuneSweepString == 'E') ? "DETUNE_E"
                                       : (args.detuneSweepString == 'A') ? "DETUNE_A"
                                       : (args.detuneSweepString == 'D') ? "DETUNE_D"
                                       :                                    "DETUNE_G";
            if (auto* p = proc.parameters.getParameter (paramId))
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
        }

        juce::MidiBuffer midi;

        if (! sequenceEvents.empty())
        {
            // Phase 2.2 R23 — note-sequence mode: drain pre-built events.
            for (const auto& e : sequenceEvents)
            {
                if (e.sampleIndex >= sampleCursor && e.sampleIndex < sampleCursor + thisBlock)
                    midi.addEvent (e.message, e.sampleIndex - sampleCursor);
            }
        }
        else
        {
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

        // Phase 2.2 R23 — per-block RMS over the sustain phase (continuity audit).
        if (sampleCursor < sustainSamples)
        {
            double sumSq = 0.0;
            int    count = 0;
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* src = blockBuffer.getReadPointer (ch);
                for (int i = 0; i < thisBlock; ++i)
                {
                    sumSq += static_cast<double> (src[i]) * src[i];
                    ++count;
                }
            }
            const float rms = (count > 0)
                            ? static_cast<float> (std::sqrt (sumSq / static_cast<double> (count)))
                            : 0.0f;
            blockRmsHistory.push_back (rms);
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

    // ---- Phase 2.2 R23 — detune-sweep + note-sequence audit metrics ----

    // RMS continuity (smallest adjacent-window ratio across the sustain phase).
    // Use 4096-sample non-overlapping windows (92 ms at 44.1 kHz). Bass-register
    // periods (e.g. ~22 Hz at deep detune = 2000-sample period) span multiple
    // 512-sample processBlocks, causing 40-50 % per-block RMS variation purely
    // from window-vs-period phase mismatch — that is a measurement artefact,
    // not a DSP glitch. A 92 ms window covers ≥3 cycles down to ~33 Hz and ≥2
    // cycles down to ~22 Hz, restoring window-to-window RMS stability.
    // Skip the first 250 ms (bow attack envelope ramp from silence to
    // steady-state) so the metric audits genuine sweep/glitch behaviour.
    constexpr int    kRmsContinuityWindow = 4096;
    constexpr double kAttackSkipSeconds   = 0.25;
    const int kAttackSkipSamples = static_cast<int> (kAttackSkipSeconds * sampleRate);

    auto rmsOverStereoWindow = [&] (int s, int e) -> float
    {
        if (e <= s) return 0.0f;
        double acc = 0.0;
        int count = 0;
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto* p = output.getReadPointer (ch);
            for (int i = s; i < e; ++i)
            {
                acc += static_cast<double> (p[i]) * p[i];
                ++count;
            }
        }
        return (count > 0)
             ? static_cast<float> (std::sqrt (acc / static_cast<double> (count)))
             : 0.0f;
    };

    std::vector<float> windowedRms;
    for (int s = kAttackSkipSamples; s + kRmsContinuityWindow <= sustainSamples;
         s += kRmsContinuityWindow)
    {
        windowedRms.push_back (rmsOverStereoWindow (s, s + kRmsContinuityWindow));
    }

    float rmsContinuityRatio = 1.0f;
    for (size_t i = 1; i < windowedRms.size(); ++i)
    {
        const float a = windowedRms[i - 1];
        const float b = windowedRms[i];
        const float ratio = juce::jmin (a, b)
                          / juce::jmax (juce::jmax (a, b), 1.0e-9f);
        rmsContinuityRatio = juce::jmin (rmsContinuityRatio, ratio);
    }
    const bool passRmsContinuity = (rmsContinuityRatio >= 0.90f);

    // Mono sustain-phase view (channel 0) for note-sequence transition + segment audits.
    auto rmsOverMono = [&] (int s, int e) -> float
    {
        if (e <= s) return 0.0f;
        double acc = 0.0;
        const auto* p0 = output.getReadPointer (0);
        for (int i = s; i < e; ++i)
            acc += static_cast<double> (p0[i]) * p0[i];
        return static_cast<float> (std::sqrt (acc / juce::jmax (1, e - s)));
    };

    // Transition-window RMS continuity (note-sequence): 256-sample symmetric window.
    float rmsContinuityAtTransitions = 1.0f;
    for (int t : transitionSampleIndices)
    {
        constexpr int N = 128;
        const int lo = juce::jmax (0, t - N);
        const int hi = juce::jmin (sustainSamples, t + N);
        const float beforeRms = rmsOverMono (lo, t);
        const float afterRms  = rmsOverMono (t,  hi);
        const float ratio = juce::jmin (beforeRms, afterRms)
                          / juce::jmax (juce::jmax (beforeRms, afterRms), 1.0e-9f);
        rmsContinuityAtTransitions = juce::jmin (rmsContinuityAtTransitions, ratio);
    }
    const bool passRmsContinuityAtTransitions = (rmsContinuityAtTransitions >= 0.50f);

    // Per-segment audibility (note-sequence): each segment's RMS must exceed 1e-3 (≈−60 dBFS).
    juce::Array<juce::var> perSegmentRmsArr;
    bool passAllSegmentsAudible = true;
    for (const auto& seg : sequenceSegments)
    {
        const int s0 = seg.sampleStart;
        const int s1 = juce::jmin (s0 + seg.sampleCount, totalSamples);
        const float segRms = rmsOverMono (s0, s1);
        perSegmentRmsArr.add (juce::var (static_cast<double> (segRms)));
        if (segRms <= 1.0e-3f)
            passAllSegmentsAudible = false;
    }

    // Mode resolution + overall PASS criterion per mode.
    const bool isStiffnessSweep = args.stiffnessSweep;
    const bool isDetuneSweep    = (args.detuneSweepString != ' ');
    const bool isNoteSequence   = (! sequenceEvents.empty());
    const char* modeStr = isStiffnessSweep ? "stiffness-sweep"
                        : isDetuneSweep    ? "detune-sweep"
                        : isNoteSequence   ? "note-sequence"
                                           : "sustained";

    bool overallPass;
    if (isDetuneSweep)
        overallPass = passNan && passPeak && passBlockTime && passRmsContinuity;
    else if (isNoteSequence)
        overallPass = passNan && passPeak && passBlockTime
                   && passAllSegmentsAudible && passRmsContinuityAtTransitions;
    else
        overallPass = passNan && passPeak && passBlockTime && passRms;

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
    summary->setProperty ("mode",                   modeStr);
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

    if (args.stringOverride != ' ')
        summary->setProperty ("string", juce::String::charToString (args.stringOverride));

    // Helper: rmsByDecade — sustain-phase RMS in 10 equal slices (used by sweep modes).
    auto computeRmsByDecade = [&] () -> juce::Array<juce::var>
    {
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
            decades.add (juce::var (rms));
        }
        return decades;
    };

    // Phase 2.2 R23: detune-sweep JSON extras.
    if (isDetuneSweep)
    {
        summary->setProperty ("string", juce::String::charToString (args.detuneSweepString));

        juce::DynamicObject::Ptr ramp (new juce::DynamicObject());
        ramp->setProperty ("startCents", -1200.0);
        ramp->setProperty ("endCents",    1200.0);
        ramp->setProperty ("shape",       "linear");
        summary->setProperty ("detuneRamp", juce::var (ramp.get()));

        summary->setProperty ("rmsByDecade",        juce::var (computeRmsByDecade()));
        summary->setProperty ("rmsContinuityRatio", static_cast<double> (rmsContinuityRatio));
        summary->setProperty ("pass_rmsContinuity", passRmsContinuity);
    }

    // Phase 2.2 R23: note-sequence JSON extras.
    if (isNoteSequence)
    {
        juce::Array<juce::var> seqArr;
        for (const auto& seg : sequenceSegments)
        {
            juce::DynamicObject::Ptr s (new juce::DynamicObject());
            s->setProperty ("midiNote", seg.midiNote);
            s->setProperty ("sampleStart", seg.sampleStart);
            s->setProperty ("sampleCount", seg.sampleCount);
            seqArr.add (juce::var (s.get()));
        }
        summary->setProperty ("sequence", juce::var (seqArr));

        juce::Array<juce::var> txArr;
        for (int t : transitionSampleIndices)
            txArr.add (juce::var (t));
        summary->setProperty ("transitionSampleIndices", juce::var (txArr));

        summary->setProperty ("perSegmentRms",                 juce::var (perSegmentRmsArr));
        summary->setProperty ("pass_allSegmentsAudible",        passAllSegmentsAudible);
        summary->setProperty ("rmsContinuityAtTransitions",     static_cast<double> (rmsContinuityAtTransitions));
        summary->setProperty ("pass_rmsContinuityAtTransitions", passRmsContinuityAtTransitions);
    }

    // Phase 2.1c R18: sweep-mode JSON extras (stiffnessRamp + rmsByDecade).
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
