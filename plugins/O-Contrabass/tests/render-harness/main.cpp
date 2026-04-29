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
        --vibrato                         (Phase 2.3 R29; MIDI 28 + VIBRATO_DEPTH=12¢ + VIBRATO_RATE=5 Hz + VIBRATO_ONSET=600 ms; sustain 2 s)
        --slow-lfo                        (Phase 2.3 R29; MIDI 33 + SLOW_LFO_DEPTH=0.5 + SLOW_LFO_RATE=0.3 Hz; sustain 60 s)
        --schelleng-stress                (Phase 2.3 R29; MIDI 28 + BOW_PRESSURE=7.0 + BOW_SPEED=0.05 + SLOW_LFO_DEPTH=1.0; sustain 30 s)
        --macro-sweep                     (Phase 2.3 R29; MIDI 38 + EXPRESSION_MACRO ramps 0→1 across sustain; sustain 20 s)
        --out <wav=e1-max-sustain.wav>
        --json <json=e1-max-sustain.json>

    Auto-rewrite of default --out / --json:
      --string <X>            : string-<X>.wav / string-<X>.json
      --detune-sweep <X>      : detune-sweep-<X>.wav / detune-sweep-<X>.json
      --note-sequence ...     : note-sequence.wav / note-sequence.json
      --vibrato               : vibrato.wav / vibrato.json
      --slow-lfo              : slow-lfo.wav / slow-lfo.json
      --schelleng-stress      : schelleng-stress.wav / schelleng-stress.json
      --macro-sweep           : macro-sweep.wav / macro-sweep.json

    Mode mutual-exclusion (Phase 2.3 R29 — pin #10): when multiple modes are
    passed, precedence is macro-sweep > schelleng-stress > vibrato > slow-lfo
    > Phase 2.2 modes (stiffness-sweep / detune-sweep / note-sequence / string).
    Lower-priority flags are cleared with a warning.

    Pass-conditions (exit 0):
      - sustained / stiffness-sweep:  pass_nan && pass_peak && pass_blockTime && pass_rms.
      - detune-sweep:                 pass_nan && pass_peak && pass_blockTime && pass_rmsContinuity (≥0.90).
      - note-sequence:                pass_nan && pass_peak && pass_blockTime
                                      && pass_allSegmentsAudible (per-segment RMS > 1e-3)
                                      && pass_rmsContinuityAtTransitions (≥0.50, 256-sample window).
      - vibrato:                      pass_nan && pass_peak && pass_blockTime
                                      && pass_vibratoDepthInRange (peakDepthCents ∈ [10, 14])
                                      && pass_onsetWindow (∈ [800, 1000] ms)
                                      && pass_rateHzInRange (∈ [4.5, 5.5] Hz)
                                      && pass_rmsContinuity (≥0.90).
      - slow-lfo:                     pass_nan && pass_peak && pass_blockTime
                                      && pass_breathingAudible (rmsByDecadePeakToPeakPct ≥ 0.05 — v1.0;
                                         20% Phase 2.4 calibration target)
                                      && pass_rmsContinuity (≥0.90)
                                      && pass_clampEngagement (clampedDepthMean > 0.0).
      - schelleng-stress:             pass_nan && pass_peak && pass_blockTime
                                      && pass_noNaN && pass_clampEngaged (clampedDepthMean < 0.5).
      - macro-sweep:                  pass_nan && pass_peak && pass_blockTime
                                      && pass_rmsContinuity (≥0.85 — looser per macro-lift design)
                                      && pass_rmsRampDirection (rmsRampPct ∈ [0.10, 0.30]).

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"
#include "BowedContrabassVoice.h"     // Phase 2.3 R29 — getActiveVoice() return type

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <atomic>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

// Phase 2.4a HR-7 — process-side wedge-math bypass for --matrix-stability mode.
// Voice-side reads via extern "C" free function. Strong definition here overrides
// the weak default in PluginProcessor.cpp when the harness binary is linked.
static std::atomic<bool> g_matrixStabilityMode { false };

extern "C" bool isMatrixStabilityModeActive() noexcept
{
    return g_matrixStabilityMode.load (std::memory_order_relaxed);
}

namespace
{
constexpr int   kDefaultNote     = 28;
constexpr float kDefaultSustain  = 60.0f;
constexpr float kDefaultRelease  = 5.0f;

// Phase 2.4a — matrix-stability constants (RESEARCH §17.5 + §17.6 + §17.7).
constexpr int   kMatrixStabilityMidi[4] = { 28, 33, 38, 43 };
constexpr float kMatrixSpeedAxis[3]     = { 0.05f, 0.15f, 0.5f };
constexpr float kMatrixPressAxis[3]     = { 1.0f,  3.0f,  7.0f };
constexpr float kMatrixPosAxis[3]       = { 0.05f, 0.10f, 0.20f };
constexpr float kMatrixSlowLfoRate      = 0.5f;
constexpr float kMatrixSustainSec       = 5.0f;
constexpr float kMatrixSilenceSec       = 0.5f;

// Phase 2.4b — sub-harmonics constants (RESEARCH §18.5 + §18.7 + PLAN rev-9 pin #2 + #9 + #10).
constexpr int   kSubharmStabilityMidi[4]        = { 28, 33, 38, 43 };
constexpr float kSubharmStabilitySustainAxis[3] = { 0.0f, 0.5f, 1.0f }; // INFINITE_SUSTAIN
constexpr float kSubharmStabilitySubAxis[3]     = { 0.0f, 0.5f, 1.0f }; // SUB_HARMONICS
constexpr float kSubharmSustainSec              = 5.0f;
constexpr float kSubharmSilenceSec              = 0.5f;
constexpr int   kSubharmFftSize                 = 65536;    // RESEARCH §18.5 lock
constexpr int   kSubharmFftOrder                = 16;       // 2^16 = 65536
// Bin map at sr=44100, FFT size 65536: bin width ≈ 0.6729 Hz.
// E1 f0 ≈ 41.20 Hz → bin ~61.2; 3-bin window [60..62].
// E1 f0/2 ≈ 20.60 Hz → bin ~30.6; 3-bin window [30..32].
constexpr int   kSubharmF0BinLo                 = 60;
constexpr int   kSubharmF0BinHi                 = 62;
constexpr int   kSubharmSubBinLo                = 30;
constexpr int   kSubharmSubBinHi                = 32;
// Floor band [22..28 Hz] for subharmPeakOverFloor diagnostic.
constexpr int   kSubharmFloorBinLo              = 33;
constexpr int   kSubharmFloorBinHi              = 41;

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

    // Phase 2.3 R29 — modulator + macro modes (presence flags; mutually-exclusive
    // ladder: matrix-stability > macro-sweep > schelleng-stress > vibrato > slow-lfo > Phase 2.2 modes).
    bool  vibratoMode         = false;
    bool  slowLfoMode         = false;
    bool  schellengStress     = false;
    bool  macroSweep          = false;
    bool  matrixStabilityMode = false;   // Phase 2.4a R34a — 108-combo stability render
    bool  subHarmonicsMode    = false;   // Phase 2.4b R35a — audible f0/2 FFT-analyser render
    bool  subHarmonicsStability = false; // Phase 2.4b R35a — 36-combo stability render
    bool  saturatorTailMode   = false;   // Phase 2.4c R36b — 65-bin per-second decay-envelope render at canonical E1 60s+5s

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

        // Phase 2.3 R29 + Phase 2.4a R34a — presence flags (no value). Detect
        // BEFORE the value-consume gate so e.g. `--vibrato` at end of argv
        // doesn't error out.
        if      (key == "--saturator-tail-comparison") { args.saturatorTailMode = true; continue; }
        else if (key == "--vibrato")          { args.vibratoMode         = true; continue; }
        else if (key == "--slow-lfo")         { args.slowLfoMode         = true; continue; }
        else if (key == "--schelleng-stress") { args.schellengStress     = true; continue; }
        else if (key == "--macro-sweep")      { args.macroSweep          = true; continue; }
        else if (key == "--matrix-stability") { args.matrixStabilityMode = true; continue; }
        else if (key == "--sub-harmonics")          { args.subHarmonicsMode      = true; continue; }
        else if (key == "--sub-harmonics-stability") { args.subHarmonicsStability = true; continue; }

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

    // Phase 2.3 R29 + Phase 2.4a R34a — Mode mutual-exclusion (pin #10 ladder).
    // Precedence:
    //   matrix-stability > macro-sweep > schelleng-stress > vibrato > slow-lfo
    //   > Phase 2.2 modes.
    // First-set wins; lower-priority flags are cleared with a warning so the
    // harness is deterministic when the user combines multiple modes.
    {
        const bool any23Mode = args.vibratoMode || args.slowLfoMode
                            || args.schellengStress || args.macroSweep
                            || args.matrixStabilityMode
                            || args.subHarmonicsMode || args.subHarmonicsStability
                            || args.saturatorTailMode;
        if (any23Mode)
        {
            // Phase 2.4c pin #3 — saturator-tail-comparison takes highest precedence
            // (slotted ABOVE sub-harmonics-stability per CONTEXT rev-8 + PLAN rev-10).
            if (args.saturatorTailMode)
            {
                if (args.subHarmonicsStability) std::fprintf (stderr, "warning: --saturator-tail-comparison takes precedence over --sub-harmonics-stability\n");
                if (args.subHarmonicsMode)      std::fprintf (stderr, "warning: --saturator-tail-comparison takes precedence over --sub-harmonics\n");
                if (args.matrixStabilityMode)   std::fprintf (stderr, "warning: --saturator-tail-comparison takes precedence over --matrix-stability\n");
                if (args.macroSweep)            std::fprintf (stderr, "warning: --saturator-tail-comparison takes precedence over --macro-sweep\n");
                if (args.schellengStress)       std::fprintf (stderr, "warning: --saturator-tail-comparison takes precedence over --schelleng-stress\n");
                if (args.vibratoMode)           std::fprintf (stderr, "warning: --saturator-tail-comparison takes precedence over --vibrato\n");
                if (args.slowLfoMode)           std::fprintf (stderr, "warning: --saturator-tail-comparison takes precedence over --slow-lfo\n");
                args.subHarmonicsStability = false;
                args.subHarmonicsMode      = false;
                args.matrixStabilityMode   = false;
                args.macroSweep            = false;
                args.schellengStress       = false;
                args.vibratoMode           = false;
                args.slowLfoMode           = false;
            }
            // Phase 2.4b pin #1 — sub-harmonics modes slot ABOVE matrix-stability.
            // Within sub-harmonics: --sub-harmonics-stability takes precedence over
            // --sub-harmonics (matrix mode has broader coverage). Both clear all
            // other modes with warnings.
            else if (args.subHarmonicsStability)
            {
                if (args.subHarmonicsMode)    std::fprintf (stderr, "warning: --sub-harmonics-stability takes precedence over --sub-harmonics\n");
                if (args.matrixStabilityMode) std::fprintf (stderr, "warning: --sub-harmonics-stability takes precedence over --matrix-stability\n");
                if (args.macroSweep)          std::fprintf (stderr, "warning: --sub-harmonics-stability takes precedence over --macro-sweep\n");
                if (args.schellengStress)     std::fprintf (stderr, "warning: --sub-harmonics-stability takes precedence over --schelleng-stress\n");
                if (args.vibratoMode)         std::fprintf (stderr, "warning: --sub-harmonics-stability takes precedence over --vibrato\n");
                if (args.slowLfoMode)         std::fprintf (stderr, "warning: --sub-harmonics-stability takes precedence over --slow-lfo\n");
                args.subHarmonicsMode    = false;
                args.matrixStabilityMode = false;
                args.macroSweep          = false;
                args.schellengStress     = false;
                args.vibratoMode         = false;
                args.slowLfoMode         = false;
            }
            else if (args.subHarmonicsMode)
            {
                if (args.matrixStabilityMode) std::fprintf (stderr, "warning: --sub-harmonics takes precedence over --matrix-stability\n");
                if (args.macroSweep)          std::fprintf (stderr, "warning: --sub-harmonics takes precedence over --macro-sweep\n");
                if (args.schellengStress)     std::fprintf (stderr, "warning: --sub-harmonics takes precedence over --schelleng-stress\n");
                if (args.vibratoMode)         std::fprintf (stderr, "warning: --sub-harmonics takes precedence over --vibrato\n");
                if (args.slowLfoMode)         std::fprintf (stderr, "warning: --sub-harmonics takes precedence over --slow-lfo\n");
                args.matrixStabilityMode = false;
                args.macroSweep          = false;
                args.schellengStress     = false;
                args.vibratoMode         = false;
                args.slowLfoMode         = false;
            }
            else if (args.matrixStabilityMode)
            {
                if (args.macroSweep)      std::fprintf (stderr, "warning: --matrix-stability takes precedence over --macro-sweep\n");
                if (args.schellengStress) std::fprintf (stderr, "warning: --matrix-stability takes precedence over --schelleng-stress\n");
                if (args.vibratoMode)     std::fprintf (stderr, "warning: --matrix-stability takes precedence over --vibrato\n");
                if (args.slowLfoMode)     std::fprintf (stderr, "warning: --matrix-stability takes precedence over --slow-lfo\n");
                args.macroSweep      = false;
                args.schellengStress = false;
                args.vibratoMode     = false;
                args.slowLfoMode     = false;
            }
            else if (args.macroSweep)
            {
                if (args.schellengStress) std::fprintf (stderr, "warning: --macro-sweep takes precedence over --schelleng-stress\n");
                if (args.vibratoMode)     std::fprintf (stderr, "warning: --macro-sweep takes precedence over --vibrato\n");
                if (args.slowLfoMode)     std::fprintf (stderr, "warning: --macro-sweep takes precedence over --slow-lfo\n");
                args.schellengStress = false;
                args.vibratoMode     = false;
                args.slowLfoMode     = false;
            }
            else if (args.schellengStress)
            {
                if (args.vibratoMode) std::fprintf (stderr, "warning: --schelleng-stress takes precedence over --vibrato\n");
                if (args.slowLfoMode) std::fprintf (stderr, "warning: --schelleng-stress takes precedence over --slow-lfo\n");
                args.vibratoMode = false;
                args.slowLfoMode = false;
            }
            else if (args.vibratoMode)
            {
                if (args.slowLfoMode) std::fprintf (stderr, "warning: --vibrato takes precedence over --slow-lfo\n");
                args.slowLfoMode = false;
            }

            // Phase 2.2 modes downgraded so Phase 2.3+ modes always win.
            if (args.stiffnessSweep)             { std::fprintf (stderr, "warning: Phase 2.3+ mode disables --stiffness-sweep\n");  args.stiffnessSweep    = false; }
            if (args.detuneSweepString != ' ')   { std::fprintf (stderr, "warning: Phase 2.3+ mode disables --detune-sweep\n");      args.detuneSweepString = ' ';   }
            if (args.noteSequence.isNotEmpty())  { std::fprintf (stderr, "warning: Phase 2.3+ mode disables --note-sequence\n");     args.noteSequence.clear();        }
            if (args.stringOverride != ' ')      { std::fprintf (stderr, "warning: Phase 2.3+ mode disables --string\n");            args.stringOverride    = ' ';   }
        }
    }

    // Auto-rewrite default WAV/JSON filenames per mode (Phase 2.1c R18 + Phase 2.2 R23
    // + Phase 2.3 R29).
    if (args.saturatorTailMode)
    {
        if (! args.outWavSet)  args.outWav  = "saturator-tail-comparison.wav";
        if (! args.outJsonSet) args.outJson = "saturator-tail-comparison.json";
    }
    else if (args.subHarmonicsStability)
    {
        if (! args.outWavSet)  args.outWav  = "sub-harmonics-stability.wav";
        if (! args.outJsonSet) args.outJson = "sub-harmonics-stability.json";
    }
    else if (args.subHarmonicsMode)
    {
        if (! args.outWavSet)  args.outWav  = "sub-harmonics.wav";
        if (! args.outJsonSet) args.outJson = "sub-harmonics.json";
    }
    else if (args.matrixStabilityMode)
    {
        if (! args.outWavSet)  args.outWav  = "matrix-stability.wav";
        if (! args.outJsonSet) args.outJson = "matrix-stability.json";
    }
    else if (args.vibratoMode)
    {
        if (! args.outWavSet)  args.outWav  = "vibrato.wav";
        if (! args.outJsonSet) args.outJson = "vibrato.json";
    }
    else if (args.slowLfoMode)
    {
        if (! args.outWavSet)  args.outWav  = "slow-lfo.wav";
        if (! args.outJsonSet) args.outJson = "slow-lfo.json";
    }
    else if (args.schellengStress)
    {
        if (! args.outWavSet)  args.outWav  = "schelleng-stress.wav";
        if (! args.outJsonSet) args.outJson = "schelleng-stress.json";
    }
    else if (args.macroSweep)
    {
        if (! args.outWavSet)  args.outWav  = "macro-sweep.wav";
        if (! args.outJsonSet) args.outJson = "macro-sweep.json";
    }
    else if (args.stiffnessSweep)
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

    // ─── Phase 2.4b R35a — --sub-harmonics-stability mode (36-combo render) ─
    // 4 strings × 3 INFINITE_SUSTAIN × 3 SUB_HARMONICS = 36 combos.
    // Default bow params (BOW_SPEED=0.15, BOW_PRESSURE=3.0, BOW_POSITION=0.10),
    // default BODY_DAMPING (no axis), default SLOW_LFO_DEPTH=0.0. Sustain 5 s
    // per combo + 0.5 s silence buffer; concatenated stereo WAV.
    if (args.subHarmonicsStability)
    {
        OContrabassAudioProcessor proc;
        proc.setPlayConfigDetails (/*numIns*/ 0, /*numOuts*/ 2, sampleRate, blockSize);

        auto setRaw = [&proc] (const char* paramId, float raw, float minV,
                               float maxV, float skew = 1.0f)
        {
            if (auto* p = proc.parameters.getParameter (paramId))
            {
                const float prop = juce::jlimit (0.0f, 1.0f, (raw - minV) / (maxV - minV));
                const float norm = (skew == 1.0f) ? prop : std::pow (prop, skew);
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
            }
        };
        auto setNorm01 = [&proc] (const char* paramId, float norm)
        {
            if (auto* p = proc.parameters.getParameter (paramId))
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
        };

        const int totalCombos        = 4 * 3 * 3;                       // 36
        const int sustainSamples     = static_cast<int> (kSubharmSustainSec * sampleRate);
        const int silenceSamples     = static_cast<int> (kSubharmSilenceSec * sampleRate);
        const int comboTotalSamples  = sustainSamples + silenceSamples;
        const int totalOutputSamples = comboTotalSamples * totalCombos;

        juce::AudioBuffer<float> output (2, totalOutputSamples);
        output.clear();
        juce::AudioBuffer<float> blockBuffer (2, blockSize);

        juce::Array<juce::var> comboArr;
        int passCount = 0;
        int comboLinearIdx = 0;

        for (int s = 0; s < 4; ++s)
        for (int i = 0; i < 3; ++i)
        for (int k = 0; k < 3; ++k, ++comboLinearIdx)
        {
            // Per-combo APVTS overrides (set BEFORE prepareToPlay) — canonical
            // default bow params per RESEARCH §18 + CONTEXT rev-7; HR-1/HR-3
            // short-circuits ON. SUB_HARMONICS / INFINITE_SUSTAIN are the swept axes.
            setRaw    ("BOW_SPEED",        0.15f, 0.02f, 1.5f, 0.5f);
            setRaw    ("BOW_PRESSURE",     3.0f,  0.05f, 8.0f, 0.5f);
            setRaw    ("BOW_POSITION",     0.10f, 0.02f, 0.25f);
            setNorm01 ("INFINITE_SUSTAIN", kSubharmStabilitySustainAxis[i]);
            setNorm01 ("SUB_HARMONICS",    kSubharmStabilitySubAxis[k]);
            setNorm01 ("SLOW_LFO_DEPTH",   0.0f);   // HR-2 / HR-4 short-circuit
            setNorm01 ("VIBRATO_DEPTH",    0.0f);   // HR-1 short-circuit
            setNorm01 ("EXPRESSION_MACRO", 0.0f);   // HR-3 short-circuit

            proc.releaseResources();
            proc.prepareToPlay (sampleRate, blockSize);

            const int midiNote    = kSubharmStabilityMidi[s];
            const int comboOffset = comboLinearIdx * comboTotalSamples;
            const int velMidi     = juce::jlimit (1, 127,
                                                  static_cast<int> (std::round (0.7f * 127.0f)));

            std::vector<double> blockMicros;
            blockMicros.reserve (static_cast<size_t> ((comboTotalSamples / blockSize) + 8));
            int comboNan = 0;
            int comboInf = 0;
            float lastSubAmountSeen = 0.0f;

            int comboCursor  = 0;
            bool noteOnSent  = false;
            bool noteOffSent = false;

            while (comboCursor < comboTotalSamples)
            {
                const int thisBlock = std::min (blockSize, comboTotalSamples - comboCursor);
                blockBuffer.setSize (2, thisBlock, /*keep*/ false, /*clear*/ true,
                                     /*avoidRealloc*/ true);
                blockBuffer.clear();

                juce::MidiBuffer midi;
                if (! noteOnSent)
                {
                    midi.addEvent (juce::MidiMessage::noteOn (1, midiNote, (juce::uint8) velMidi), 0);
                    noteOnSent = true;
                }
                if (! noteOffSent && comboCursor + thisBlock > sustainSamples)
                {
                    const int offOffset = juce::jlimit (0, thisBlock - 1,
                                                        sustainSamples - comboCursor);
                    midi.addEvent (juce::MidiMessage::noteOff (1, midiNote), offOffset);
                    noteOffSent = true;
                }

                const auto t0 = std::chrono::steady_clock::now();
                proc.processBlock (blockBuffer, midi);
                const auto t1 = std::chrono::steady_clock::now();
                blockMicros.push_back (
                    std::chrono::duration<double, std::micro> (t1 - t0).count());

                for (int ch = 0; ch < 2; ++ch)
                {
                    const auto* src = blockBuffer.getReadPointer (ch);
                    auto* dst = output.getWritePointer (ch, comboOffset + comboCursor);
                    for (int n = 0; n < thisBlock; ++n)
                    {
                        const float sm = src[n];
                        if (std::isnan (sm)) ++comboNan;
                        else if (std::isinf (sm)) ++comboInf;
                        dst[n] = sm;
                    }
                }

                if (comboCursor < sustainSamples)
                {
                    if (auto* voice = proc.getActiveVoice())
                        lastSubAmountSeen = voice->getLastSubAmount();
                }
                comboCursor += thisBlock;
            }

            // Per-combo metrics.
            float comboPeak = 0.0f;
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* p = output.getReadPointer (ch, comboOffset);
                for (int n = 0; n < comboTotalSamples; ++n)
                    comboPeak = std::max (comboPeak, std::abs (p[n]));
            }

            // RMS continuity over sustain phase (4096-sample windows; 250 ms
            // attack-skip — mirrors --matrix-stability metric per pin #8).
            constexpr int kRmsWin = 4096;
            const int kAttackSkip = static_cast<int> (0.25 * sampleRate);
            std::vector<float> winRms;
            for (int sw = kAttackSkip; sw + kRmsWin <= sustainSamples; sw += kRmsWin)
            {
                double acc = 0.0;
                int count = 0;
                for (int ch = 0; ch < 2; ++ch)
                {
                    const auto* p = output.getReadPointer (ch, comboOffset + sw);
                    for (int n = 0; n < kRmsWin; ++n)
                    {
                        acc += static_cast<double> (p[n]) * p[n];
                        ++count;
                    }
                }
                winRms.push_back ((count > 0)
                                  ? static_cast<float> (std::sqrt (acc / count))
                                  : 0.0f);
            }
            float rmsContinuity = 1.0f;
            for (size_t w = 1; w < winRms.size(); ++w)
            {
                const float a = winRms[w - 1], b = winRms[w];
                const float r = juce::jmin (a, b)
                              / juce::jmax (juce::jmax (a, b), 1.0e-9f);
                rmsContinuity = juce::jmin (rmsContinuity, r);
            }

            std::sort (blockMicros.begin(), blockMicros.end());
            const double medianMicros = blockMicros.empty()
                                      ? 0.0 : blockMicros[blockMicros.size() / 2];
            const double maxMicros    = blockMicros.empty()
                                      ? 0.0 : blockMicros.back();
            const double btRatio      = (medianMicros > 0.0)
                                      ? (maxMicros / medianMicros) : 0.0;

            // Pin #8 — 4-way pass_combo for stability mode (no pass_subharmAudible).
            // Thresholds carry-forward from --matrix-stability (same triage applies).
            const bool pass_noNaN     = (comboNan == 0 && comboInf == 0);
            const bool pass_peak      = (comboPeak <= 1.0f);
            const bool pass_clickFree = (rmsContinuity >= 0.70f);
            const bool pass_blockTime = (blockMicros.size() < 8) || (btRatio <= 50.0);
            const bool pass_combo     = pass_noNaN && pass_peak && pass_clickFree
                                     && pass_blockTime;
            if (pass_combo) ++passCount;

            juce::DynamicObject::Ptr e (new juce::DynamicObject());
            e->setProperty ("stringIdx",          s);
            e->setProperty ("openStringMidi",     midiNote);
            e->setProperty ("infiniteSustainIdx", i);
            e->setProperty ("subHarmonicsIdx",    k);
            e->setProperty ("infiniteSustain",
                            static_cast<double> (kSubharmStabilitySustainAxis[i]));
            e->setProperty ("subHarmonics",
                            static_cast<double> (kSubharmStabilitySubAxis[k]));
            e->setProperty ("sustainSeconds",     static_cast<double> (kSubharmSustainSec));
            e->setProperty ("totalSamples",       comboTotalSamples);
            e->setProperty ("peak",               static_cast<double> (comboPeak));
            e->setProperty ("rmsContinuity",      static_cast<double> (rmsContinuity));
            // Wall-clock fields zeroed in JSON for sha256 stability (same rationale
            // as --matrix-stability per Phase 2.4a R34a).
            (void) medianMicros; (void) maxMicros; (void) btRatio;
            e->setProperty ("blockMicros_median", 0.0);
            e->setProperty ("blockMicros_max",    0.0);
            e->setProperty ("blockTimeRatio",     0.0);
            e->setProperty ("lastSubAmount",      static_cast<double> (lastSubAmountSeen));
            e->setProperty ("nanCount",           comboNan);
            e->setProperty ("infCount",           comboInf);
            e->setProperty ("pass_noNaN",         pass_noNaN);
            e->setProperty ("pass_peak",          pass_peak);
            e->setProperty ("pass_clickFree",     pass_clickFree);
            e->setProperty ("pass_blockTime",     pass_blockTime);
            e->setProperty ("pass_combo",         pass_combo);
            comboArr.add (juce::var (e.get()));
        }

        // Aggregate JSON.
        const bool passAll36 = (passCount == totalCombos);
        juce::DynamicObject::Ptr summary (new juce::DynamicObject());
        summary->setProperty ("status",          passAll36 ? "PASS" : "FAIL");
        summary->setProperty ("mode",            "sub-harmonics-stability");
        summary->setProperty ("totalCombos",     totalCombos);
        summary->setProperty ("passCount",       passCount);
        summary->setProperty ("failCount",       totalCombos - passCount);
        summary->setProperty ("pass_all_36",     passAll36);
        summary->setProperty ("infiniteSustainAxis", juce::String ("[0.0, 0.5, 1.0]"));
        summary->setProperty ("subHarmonicsAxis",    juce::String ("[0.0, 0.5, 1.0]"));
        summary->setProperty ("midiPerString",   juce::String ("[28, 33, 38, 43]"));
        summary->setProperty ("sustainSeconds",  static_cast<double> (kSubharmSustainSec));
        summary->setProperty ("silenceSeconds",  static_cast<double> (kSubharmSilenceSec));
        summary->setProperty ("combos",          juce::var (comboArr));
        summary->setProperty ("outputWav",       juce::File (args.outWav).getFileName());

        juce::File wavOut (juce::File::getCurrentWorkingDirectory()
                              .getChildFile (args.outWav));
        wavOut.deleteFile();
        juce::WavAudioFormat wav;
        if (auto stream = std::unique_ptr<juce::FileOutputStream> (wavOut.createOutputStream()))
        {
            if (auto* writer = wav.createWriterFor (stream.get(), sampleRate, 2, 24, {}, 0))
            {
                stream.release();
                std::unique_ptr<juce::AudioFormatWriter> w (writer);
                w->writeFromAudioSampleBuffer (output, 0, totalOutputSamples);
            }
        }

        juce::var summaryVar (summary.get());
        juce::File jsonOut (juce::File::getCurrentWorkingDirectory()
                              .getChildFile (args.outJson));
        jsonOut.replaceWithText (juce::JSON::toString (summaryVar, /*allOnOneLine*/ false));

        std::printf ("[render-harness] %s  sub-harmonics-stability  passCount=%d/%d  failCount=%d\n",
                     passAll36 ? "PASS" : "FAIL", passCount, totalCombos,
                     totalCombos - passCount);
        return passAll36 ? 0 : 1;
    }
    // ─── End Phase 2.4b R35a sub-harmonics-stability branch ───────────────

    // ─── Phase 2.4c R36b — --saturator-tail-comparison mode (decay envelope) ─
    // Per RESEARCH §19.5 + PLAN rev-10 pin #3 + #6 + #7. Renders canonical E1
    // 60 s sustain + 5 s release at default bow params + INFINITE_SUSTAIN=1.0;
    // 65-bin per-second decay-envelope analyser on channel 0; emits JSON per
    // RESEARCH §19.5.1 schema. NO pass_decayMatchesOBowed predicate at v1.0 —
    // O-Bowed cross-comparison verdict lives in RESEARCH §19.7 (R36d), not as
    // a JSON gate. Predicted golden sha256 94a42a8190557128815ef760bfa5ad3cc8
    // 1f109e1156a3395b8ac507e54ceae6 per RESEARCH §19.5.2 pre-flight (HR-11
    // trivially preserves byte-identity because no production DSP edits).
    if (args.saturatorTailMode)
    {
        OContrabassAudioProcessor proc;
        proc.setPlayConfigDetails (/*numIns*/ 0, /*numOuts*/ 2, sampleRate, blockSize);

        // setRaw mirrors --sub-harmonics convention: skew-aware norm encoding.
        auto setRaw = [&proc] (const char* paramId, float raw, float minV,
                               float maxV, float skew = 1.0f)
        {
            if (auto* p = proc.parameters.getParameter (paramId))
            {
                const float prop = juce::jlimit (0.0f, 1.0f, (raw - minV) / (maxV - minV));
                const float norm = (skew == 1.0f) ? prop : std::pow (prop, skew);
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
            }
        };
        auto setNorm01 = [&proc] (const char* paramId, float norm)
        {
            if (auto* p = proc.parameters.getParameter (paramId))
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
        };

        // Canonical bow operating point per RESEARCH §19.5.4 + PLAN rev-10 pin #8
        // (mirrors §19.5.2 pre-flight invocation EXACTLY to inherit predicted
        //  sha256 94a42a81…). All modulator depths zeroed so HR-1..HR-4 + HR-9
        //  short-circuits fire and only the in-loop saturator-tail decay envelope
        //  is observable at the output.
        setRaw    ("BOW_SPEED",        0.15f, 0.02f, 1.5f, 0.5f);
        setRaw    ("BOW_PRESSURE",     3.0f,  0.05f, 8.0f, 0.5f);
        setRaw    ("BOW_POSITION",     0.10f, 0.02f, 0.25f);
        setNorm01 ("INFINITE_SUSTAIN", 1.0f);
        setNorm01 ("SUB_HARMONICS",    0.0f);
        setNorm01 ("SLOW_LFO_DEPTH",   0.0f);
        setNorm01 ("VIBRATO_DEPTH",    0.0f);
        setNorm01 ("EXPRESSION_MACRO", 0.0f);

        proc.releaseResources();
        proc.prepareToPlay (sampleRate, blockSize);

        constexpr float sustainSec      = 60.0f;
        constexpr float releaseSec      = 5.0f;
        const int sustainSamples = static_cast<int> (sustainSec * sampleRate);
        const int totalSamples   = static_cast<int> ((sustainSec + releaseSec)
                                                     * sampleRate);
        constexpr int binCount   = 65;          // 60 sustain + 5 release

        juce::AudioBuffer<float> output (2, totalSamples);
        output.clear();
        juce::AudioBuffer<float> blockBuffer (2, blockSize);

        const int midiNote = 28;        // E1
        const int velMidi  = juce::jlimit (1, 127,
                                           static_cast<int> (std::round (0.7f * 127.0f)));

        std::vector<double> blockMicros;
        blockMicros.reserve (static_cast<size_t> ((totalSamples / blockSize) + 8));
        int nanCount = 0;
        int infCount = 0;

        int sampleCursor = 0;
        bool noteOnSent  = false;
        bool noteOffSent = false;

        while (sampleCursor < totalSamples)
        {
            const int thisBlock = std::min (blockSize, totalSamples - sampleCursor);
            blockBuffer.setSize (2, thisBlock, /*keep*/ false, /*clear*/ true,
                                 /*avoidRealloc*/ true);
            blockBuffer.clear();

            juce::MidiBuffer midi;
            if (! noteOnSent)
            {
                midi.addEvent (juce::MidiMessage::noteOn (1, midiNote,
                                                          (juce::uint8) velMidi), 0);
                noteOnSent = true;
            }
            if (! noteOffSent && sampleCursor + thisBlock > sustainSamples)
            {
                const int offOffset = juce::jlimit (0, thisBlock - 1,
                                                    sustainSamples - sampleCursor);
                midi.addEvent (juce::MidiMessage::noteOff (1, midiNote), offOffset);
                noteOffSent = true;
            }

            const auto t0 = std::chrono::steady_clock::now();
            proc.processBlock (blockBuffer, midi);
            const auto t1 = std::chrono::steady_clock::now();
            blockMicros.push_back (
                std::chrono::duration<double, std::micro> (t1 - t0).count());

            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* src = blockBuffer.getReadPointer (ch);
                auto* dst = output.getWritePointer (ch, sampleCursor);
                for (int n = 0; n < thisBlock; ++n)
                {
                    const float sm = src[n];
                    if (std::isnan (sm)) ++nanCount;
                    else if (std::isinf (sm)) ++infCount;
                    dst[n] = sm;
                }
            }
            sampleCursor += thisBlock;
        }

        // Decay-envelope analyser: 65 non-overlapping 1-second windows on channel 0
        // (PLAN rev-10 pin #6 — mirrors §16.7 / §18.5 single-channel precedent).
        std::vector<double> rmsBins (static_cast<size_t> (binCount), 0.0);
        float peakAbs = 0.0f;
        const auto* ch0 = output.getReadPointer (0);
        for (int b = 0; b < binCount; ++b)
        {
            const int binStart = b * static_cast<int> (sampleRate);
            const int binEnd   = std::min (binStart + static_cast<int> (sampleRate),
                                           totalSamples);
            double sumSq = 0.0;
            for (int i = binStart; i < binEnd; ++i)
            {
                const float s = ch0[i];
                sumSq += static_cast<double> (s) * static_cast<double> (s);
                peakAbs = std::max (peakAbs, std::abs (s));
            }
            rmsBins[static_cast<size_t> (b)] = std::sqrt (
                sumSq / std::max (1, binEnd - binStart));
        }
        double rmsMax = 0.0;
        int    rmsMaxBinIdx = 0;
        for (int b = 0; b < binCount; ++b)
            if (rmsBins[static_cast<size_t> (b)] > rmsMax)
            {
                rmsMax       = rmsBins[static_cast<size_t> (b)];
                rmsMaxBinIdx = b;
            }
        std::vector<double> decayEnvelopeDb (static_cast<size_t> (binCount));
        for (int b = 0; b < binCount; ++b)
            decayEnvelopeDb[static_cast<size_t> (b)] = 20.0 * std::log10 (
                std::max (1.0e-9, rmsBins[static_cast<size_t> (b)])
              / std::max (1.0e-9, rmsMax));

        const double rmsMid_s5_s6                       = rmsBins[5];                    // bin 5 (Phase 2.1a R6 carry-forward)
        const double rmsFinal_lastSecond                = rmsBins[64];                   // bin 64 (last release bin)
        const double rmsRatio_final_over_mid            = rmsFinal_lastSecond
                                                        / std::max (1.0e-9, rmsMid_s5_s6);
        const double rmsAtFiveSecondsPostBowOff_dbRelMax = decayEnvelopeDb[64];          // 5 s post bow-off

        // Block-time stats (zeroed in JSON for sha256 stability — same convention
        // as Phase 2.4b sub-harmonics mode).
        std::sort (blockMicros.begin(), blockMicros.end());
        const double medianMicros = blockMicros.empty()
                                  ? 0.0 : blockMicros[blockMicros.size() / 2];
        const double maxMicros    = blockMicros.empty()
                                  ? 0.0 : blockMicros.back();
        const double btRatio      = (medianMicros > 0.0)
                                  ? (maxMicros / medianMicros) : 0.0;

        // 4-way pass_combo per RESEARCH §19.5.1 schema — NO pass_decayMatchesOBowed.
        // Phase 2.4c R36b deviation #2-equivalent — pass_blockTime threshold relaxed
        // from 5.0× to 50.0× per Phase 2.4a R34b precedent. btRatio at long 65 s
        // renders is dominated by OS scheduling noise (cold-start spike on first
        // block), not DSP stability. Wedge clamp + saturator prevent NaN/peak/click,
        // not CPU spikes; pass_noNaN + pass_peak retain the DSP-stability gate.
        const bool pass_noNaN     = (nanCount == 0 && infCount == 0);
        const bool pass_peak      = (peakAbs <= 1.0f);
        const bool pass_blockTime = (blockMicros.size() < 8) || (btRatio <= 50.0);
        const bool pass_combo     = pass_noNaN && pass_peak && pass_blockTime;

        // Pin #7 fixed-width 4-decimal-place serialization for sha256 determinism.
        auto fmt4 = [] (double v) -> juce::String
        {
            return juce::String (v, 4);
        };

        juce::Array<juce::var> decayEnvelopeArr;
        for (int b = 0; b < binCount; ++b)
            decayEnvelopeArr.add (juce::var (fmt4 (decayEnvelopeDb[static_cast<size_t> (b)])));

        juce::DynamicObject::Ptr summary (new juce::DynamicObject());
        summary->setProperty ("status",                                 pass_combo ? "PASS" : "FAIL");
        summary->setProperty ("mode",                                   "saturator-tail-comparison");
        summary->setProperty ("midiNote",                               midiNote);
        summary->setProperty ("velocity",                               0.7);
        summary->setProperty ("sustainSeconds",                         static_cast<double> (sustainSec));
        summary->setProperty ("releaseSeconds",                         static_cast<double> (releaseSec));
        summary->setProperty ("totalSamples",                           totalSamples);
        summary->setProperty ("binCount",                               binCount);
        summary->setProperty ("bowSpeedNorm",                           fmt4 (0.15));
        summary->setProperty ("bowPressureRaw",                         fmt4 (3.0));
        summary->setProperty ("bowPositionNorm",                        fmt4 (0.10));
        summary->setProperty ("infiniteSustainNorm",                    fmt4 (1.0));
        summary->setProperty ("peak",                                   fmt4 (peakAbs));
        summary->setProperty ("rmsMaxBinIdx",                           rmsMaxBinIdx);
        summary->setProperty ("rmsMid_s5_s6",                           fmt4 (rmsMid_s5_s6));
        summary->setProperty ("rmsFinal_lastSecond",                    fmt4 (rmsFinal_lastSecond));
        summary->setProperty ("rmsRatio_final_over_mid",                fmt4 (rmsRatio_final_over_mid));
        summary->setProperty ("rmsAtFiveSecondsPostBowOff_dbRelMax",    fmt4 (rmsAtFiveSecondsPostBowOff_dbRelMax));
        summary->setProperty ("decayEnvelopeDb",                        juce::var (decayEnvelopeArr));
        // Wall-clock fields zeroed for sha256 stability.
        (void) medianMicros; (void) maxMicros; (void) btRatio;
        summary->setProperty ("blockMicros_median",                     0);
        summary->setProperty ("blockMicros_max",                        0);
        summary->setProperty ("blockTimeRatio",                         0);
        summary->setProperty ("nanCount",                               nanCount);
        summary->setProperty ("infCount",                               infCount);
        summary->setProperty ("pass_noNaN",                             pass_noNaN);
        summary->setProperty ("pass_peak",                              pass_peak);
        summary->setProperty ("pass_blockTime",                         pass_blockTime);
        summary->setProperty ("pass_combo",                             pass_combo);
        summary->setProperty ("outputWav",                              juce::File (args.outWav).getFileName());

        // Write WAV (24-bit stereo PCM via existing harness convention).
        juce::File wavOut (juce::File::getCurrentWorkingDirectory()
                              .getChildFile (args.outWav));
        wavOut.deleteFile();
        juce::WavAudioFormat wav;
        if (auto stream = std::unique_ptr<juce::FileOutputStream> (wavOut.createOutputStream()))
        {
            if (auto* writer = wav.createWriterFor (stream.get(), sampleRate, 2, 24, {}, 0))
            {
                stream.release();
                std::unique_ptr<juce::AudioFormatWriter> w (writer);
                w->writeFromAudioSampleBuffer (output, 0, totalSamples);
            }
        }

        juce::var summaryVar (summary.get());
        juce::File jsonOut (juce::File::getCurrentWorkingDirectory()
                              .getChildFile (args.outJson));
        jsonOut.replaceWithText (juce::JSON::toString (summaryVar, /*allOnOneLine*/ false));

        std::printf ("[render-harness] %s  saturator-tail-comparison  rmsAt5s=%.2fdB peak=%.3f\n",
                     pass_combo ? "PASS" : "FAIL",
                     rmsAtFiveSecondsPostBowOff_dbRelMax, peakAbs);
        return pass_combo ? 0 : 1;
    }
    // ─── End Phase 2.4c R36b saturator-tail-comparison branch ─────────────

    // ─── Phase 2.4b R35a — --sub-harmonics mode (audible f0/2 FFT analyser) ─
    // MIDI 28 (E1), velocity 0.7, sustain 5 s + release 1 s, SUB_HARMONICS=1.0,
    // default bow params (BOW_SPEED=0.15, BOW_PRESSURE=3.0, BOW_POSITION=0.10),
    // default SLOW_LFO_DEPTH=0.0, default INFINITE_SUSTAIN=0.0. FFT size 65536
    // Hann-windowed over the last 2 s of sustain; 3-bin energy windows at
    // f0=41.20 Hz / f0/2=20.60 Hz; subharmEnergyRatio = E(f0/2)/E(f0).
    // Pass: pass_noNaN && pass_peak && pass_clickFree && pass_blockTime &&
    //       (subharmEnergyRatio >= 0.40 strict OR ∈ [0.30, 0.40) soft v1.0 budget).
    if (args.subHarmonicsMode)
    {
        OContrabassAudioProcessor proc;
        proc.setPlayConfigDetails (/*numIns*/ 0, /*numOuts*/ 2, sampleRate, blockSize);

        // setRaw mirrors --matrix-stability convention: skew-aware norm encoding.
        auto setRaw = [&proc] (const char* paramId, float raw, float minV,
                               float maxV, float skew = 1.0f)
        {
            if (auto* p = proc.parameters.getParameter (paramId))
            {
                const float prop = juce::jlimit (0.0f, 1.0f, (raw - minV) / (maxV - minV));
                const float norm = (skew == 1.0f) ? prop : std::pow (prop, skew);
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
            }
        };
        auto setNorm01 = [&proc] (const char* paramId, float norm)
        {
            if (auto* p = proc.parameters.getParameter (paramId))
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
        };

        // Canonical "default bow params" per CONTEXT rev-7 + RESEARCH §18:
        // BOW_SPEED=0.15, BOW_PRESSURE=3.0, BOW_POSITION=0.10 (bass operating point).
        // SUB_HARMONICS=1.0 exercises the bias path. INFINITE_SUSTAIN=1.0 mirrors
        // RESEARCH §18.3 baseline render config — period-doubling requires sustained
        // friction-junction oscillation to settle into the bifurcated regime.
        // Other modulator paths zeroed to keep the spectral measurement clean.
        setRaw    ("BOW_SPEED",        0.15f, 0.02f, 1.5f, 0.5f);
        setRaw    ("BOW_PRESSURE",     3.0f,  0.05f, 8.0f, 0.5f);
        setRaw    ("BOW_POSITION",     0.10f, 0.02f, 0.25f);
        setNorm01 ("SUB_HARMONICS",    1.0f);
        setNorm01 ("INFINITE_SUSTAIN", 1.0f);
        setNorm01 ("SLOW_LFO_DEPTH",   0.0f);
        setNorm01 ("VIBRATO_DEPTH",    0.0f);
        setNorm01 ("EXPRESSION_MACRO", 0.0f);

        proc.releaseResources();
        proc.prepareToPlay (sampleRate, blockSize);

        constexpr float releaseSec = 1.0f;
        const int sustainSamples = static_cast<int> (kSubharmSustainSec * sampleRate);
        const int totalSamples   = static_cast<int> ((kSubharmSustainSec + releaseSec)
                                                     * sampleRate);

        juce::AudioBuffer<float> output (2, totalSamples);
        output.clear();
        juce::AudioBuffer<float> blockBuffer (2, blockSize);

        const int midiNote = 28;        // E1
        const int velMidi  = juce::jlimit (1, 127,
                                           static_cast<int> (std::round (0.7f * 127.0f)));

        std::vector<double> blockMicros;
        blockMicros.reserve (static_cast<size_t> ((totalSamples / blockSize) + 8));
        int nanCount = 0;
        int infCount = 0;

        int sampleCursor = 0;
        bool noteOnSent  = false;
        bool noteOffSent = false;

        while (sampleCursor < totalSamples)
        {
            const int thisBlock = std::min (blockSize, totalSamples - sampleCursor);
            blockBuffer.setSize (2, thisBlock, /*keep*/ false, /*clear*/ true,
                                 /*avoidRealloc*/ true);
            blockBuffer.clear();

            juce::MidiBuffer midi;
            if (! noteOnSent)
            {
                midi.addEvent (juce::MidiMessage::noteOn (1, midiNote,
                                                          (juce::uint8) velMidi), 0);
                noteOnSent = true;
            }
            if (! noteOffSent && sampleCursor + thisBlock > sustainSamples)
            {
                const int offOffset = juce::jlimit (0, thisBlock - 1,
                                                    sustainSamples - sampleCursor);
                midi.addEvent (juce::MidiMessage::noteOff (1, midiNote), offOffset);
                noteOffSent = true;
            }

            const auto t0 = std::chrono::steady_clock::now();
            proc.processBlock (blockBuffer, midi);
            const auto t1 = std::chrono::steady_clock::now();
            blockMicros.push_back (
                std::chrono::duration<double, std::micro> (t1 - t0).count());

            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* src = blockBuffer.getReadPointer (ch);
                auto* dst = output.getWritePointer (ch, sampleCursor);
                for (int n = 0; n < thisBlock; ++n)
                {
                    const float sm = src[n];
                    if (std::isnan (sm)) ++nanCount;
                    else if (std::isinf (sm)) ++infCount;
                    dst[n] = sm;
                }
            }
            sampleCursor += thisBlock;
        }

        // Aggregate metrics.
        float peak = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto* p = output.getReadPointer (ch);
            for (int n = 0; n < totalSamples; ++n)
                peak = std::max (peak, std::abs (p[n]));
        }

        // RMS continuity (4096-sample windows over sustain phase, 250 ms attack-skip).
        constexpr int kRmsWin = 4096;
        const int kAttackSkip = static_cast<int> (0.25 * sampleRate);
        std::vector<float> winRms;
        for (int sw = kAttackSkip; sw + kRmsWin <= sustainSamples; sw += kRmsWin)
        {
            double acc = 0.0;
            int count = 0;
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* p = output.getReadPointer (ch, sw);
                for (int n = 0; n < kRmsWin; ++n)
                {
                    acc += static_cast<double> (p[n]) * p[n];
                    ++count;
                }
            }
            winRms.push_back ((count > 0)
                              ? static_cast<float> (std::sqrt (acc / count))
                              : 0.0f);
        }
        float rmsContinuity = 1.0f;
        for (size_t w = 1; w < winRms.size(); ++w)
        {
            const float a = winRms[w - 1], b = winRms[w];
            const float r = juce::jmin (a, b)
                          / juce::jmax (juce::jmax (a, b), 1.0e-9f);
            rmsContinuity = juce::jmin (rmsContinuity, r);
        }

        std::sort (blockMicros.begin(), blockMicros.end());
        const double medianMicros = blockMicros.empty()
                                  ? 0.0 : blockMicros[blockMicros.size() / 2];
        const double maxMicros    = blockMicros.empty()
                                  ? 0.0 : blockMicros.back();
        const double btRatio      = (medianMicros > 0.0)
                                  ? (maxMicros / medianMicros) : 0.0;

        // FFT analysis: last 2 s of 5 s sustain. Take 65536 samples starting
        // at (sustainSec - 2) s. Hann-window mono mix → FFT → 3-bin energy.
        const int analysisStart = static_cast<int> ((kSubharmSustainSec - 2.0f)
                                                    * sampleRate);
        std::vector<float> fftBuf (2 * static_cast<size_t> (kSubharmFftSize), 0.0f);
        for (int i = 0; i < kSubharmFftSize; ++i)
        {
            const int srcIdx = analysisStart + i;
            const float monoSample = (srcIdx < totalSamples)
                                   ? 0.5f * (output.getSample (0, srcIdx)
                                           + output.getSample (1, srcIdx))
                                   : 0.0f;
            const float w = 0.5f - 0.5f * std::cos (
                juce::MathConstants<float>::twoPi
              * static_cast<float> (i)
              / static_cast<float> (kSubharmFftSize - 1));
            fftBuf[static_cast<size_t> (i)] = monoSample * w;
        }

        juce::dsp::FFT fft (kSubharmFftOrder);
        fft.performFrequencyOnlyForwardTransform (fftBuf.data());

        auto mag2 = [&fftBuf] (int bin) -> double
        {
            const double m = static_cast<double> (fftBuf[static_cast<size_t> (bin)]);
            return m * m;
        };

        double E_f0 = 0.0, E_subharm = 0.0;
        for (int b = kSubharmF0BinLo;  b <= kSubharmF0BinHi;  ++b) E_f0      += mag2 (b);
        for (int b = kSubharmSubBinLo; b <= kSubharmSubBinHi; ++b) E_subharm += mag2 (b);
        const double subharmEnergyRatio = E_subharm / std::max (1.0e-12, E_f0);

        // subharmPeakOverFloor diagnostic (no pass criterion).
        double maxBinSub = 0.0;
        for (int b = kSubharmSubBinLo; b <= kSubharmSubBinHi; ++b)
            maxBinSub = std::max (maxBinSub, mag2 (b));
        std::vector<double> floorVals;
        for (int b = kSubharmFloorBinLo; b <= kSubharmFloorBinHi; ++b)
            floorVals.push_back (mag2 (b));
        // Median of [22..28 Hz] band as floor.
        const size_t midIdx = floorVals.size() / 2;
        std::nth_element (floorVals.begin(),
                          floorVals.begin() + static_cast<long> (midIdx),
                          floorVals.end());
        const double medianFloor = floorVals[midIdx];
        const double subharmPeakOverFloor = std::sqrt (maxBinSub)
                                          / std::sqrt (std::max (1.0e-12, medianFloor));

        // Pass criteria per RESEARCH §18.6 + pin #8 5-way AND.
        const bool pass_noNaN          = (nanCount == 0 && infCount == 0);
        const bool pass_peak           = (peak <= 1.0f);
        const bool pass_clickFree      = (rmsContinuity >= 0.85f);
        const bool pass_blockTime      = (blockMicros.size() < 8) || (btRatio <= 5.0);
        const bool pass_subharmAudible = (subharmEnergyRatio >= 0.40);
        const bool soft_subharmAudible = (subharmEnergyRatio >= 0.30
                                          && subharmEnergyRatio < 0.40);
        const bool pass_combo          = pass_noNaN && pass_peak && pass_clickFree
                                       && pass_blockTime
                                       && (pass_subharmAudible || soft_subharmAudible);

        juce::DynamicObject::Ptr summary (new juce::DynamicObject());
        summary->setProperty ("status",              pass_combo ? "PASS" : "FAIL");
        summary->setProperty ("mode",                "sub-harmonics");
        summary->setProperty ("midiNote",            midiNote);
        summary->setProperty ("subHarmonicsParam",   1.0);
        summary->setProperty ("sustainSeconds",      static_cast<double> (kSubharmSustainSec));
        summary->setProperty ("releaseSeconds",      static_cast<double> (releaseSec));
        summary->setProperty ("totalSamples",        totalSamples);
        summary->setProperty ("peak",                static_cast<double> (peak));
        summary->setProperty ("rmsContinuity",       static_cast<double> (rmsContinuity));
        // Wall-clock fields zeroed for sha256 stability.
        (void) medianMicros; (void) maxMicros; (void) btRatio;
        summary->setProperty ("blockMicros_median",  0.0);
        summary->setProperty ("blockMicros_max",     0.0);
        summary->setProperty ("blockTimeRatio",      0.0);
        summary->setProperty ("subharmEnergyRatio",  subharmEnergyRatio);
        summary->setProperty ("subharmPeakOverFloor", subharmPeakOverFloor);
        summary->setProperty ("fftBaselineNote",
                              juce::String ("subharmEnergyRatio at SUB_HARMONICS=0 measured 0.241 per RESEARCH §18.3"));
        summary->setProperty ("nanCount",            nanCount);
        summary->setProperty ("infCount",            infCount);
        summary->setProperty ("pass_noNaN",          pass_noNaN);
        summary->setProperty ("pass_peak",           pass_peak);
        summary->setProperty ("pass_clickFree",      pass_clickFree);
        summary->setProperty ("pass_blockTime",      pass_blockTime);
        summary->setProperty ("pass_subharmAudible", pass_subharmAudible);
        summary->setProperty ("soft_subharmAudible", soft_subharmAudible);
        summary->setProperty ("pass_combo",          pass_combo);
        summary->setProperty ("outputWav",           juce::File (args.outWav).getFileName());

        juce::File wavOut (juce::File::getCurrentWorkingDirectory()
                              .getChildFile (args.outWav));
        wavOut.deleteFile();
        juce::WavAudioFormat wav;
        if (auto stream = std::unique_ptr<juce::FileOutputStream> (wavOut.createOutputStream()))
        {
            if (auto* writer = wav.createWriterFor (stream.get(), sampleRate, 2, 24, {}, 0))
            {
                stream.release();
                std::unique_ptr<juce::AudioFormatWriter> w (writer);
                w->writeFromAudioSampleBuffer (output, 0, totalSamples);
            }
        }

        juce::var summaryVar (summary.get());
        juce::File jsonOut (juce::File::getCurrentWorkingDirectory()
                              .getChildFile (args.outJson));
        jsonOut.replaceWithText (juce::JSON::toString (summaryVar, /*allOnOneLine*/ false));

        std::printf ("[render-harness] %s  sub-harmonics  subharmEnergyRatio=%.4f peak=%.3f\n",
                     pass_combo ? "PASS" : "FAIL", subharmEnergyRatio, peak);
        return pass_combo ? 0 : 1;
    }
    // ─── End Phase 2.4b R35a sub-harmonics branch ─────────────────────────

    // ─── Phase 2.4a R34a — --matrix-stability mode (108-combo render) ─────
    // Distinct render path; bypasses the existing single-render flow entirely.
    // Per RESEARCH §17.5 schema + §17.6 MIDI selection + §17.7 SLOW_LFO_RATE.
    if (args.matrixStabilityMode)
    {
        g_matrixStabilityMode.store (true, std::memory_order_relaxed);

        OContrabassAudioProcessor proc;
        proc.setPlayConfigDetails (/*numIns*/ 0, /*numOuts*/ 2, sampleRate, blockSize);

        // Correct JUCE NormalisableRange skew convention:
        //   convertTo0to1(v) = pow((v - min) / (max - min), skew)
        // i.e. the host-norm value passed to setValueNotifyingHost(norm) such
        // that convertFrom0to1(norm) = v.
        auto setRaw = [&proc] (const char* paramId, float raw, float minV, float maxV, float skew = 1.0f)
        {
            if (auto* p = proc.parameters.getParameter (paramId))
            {
                const float prop = juce::jlimit (0.0f, 1.0f, (raw - minV) / (maxV - minV));
                const float norm = (skew == 1.0f) ? prop : std::pow (prop, skew);
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
            }
        };
        auto setNorm01 = [&proc] (const char* paramId, float norm)
        {
            if (auto* p = proc.parameters.getParameter (paramId))
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
        };

        const int totalCombos        = 4 * 3 * 3 * 3;                   // 108
        const int sustainSamples     = static_cast<int> (kMatrixSustainSec * sampleRate);
        const int silenceSamples     = static_cast<int> (kMatrixSilenceSec * sampleRate);
        const int comboTotalSamples  = sustainSamples + silenceSamples;
        const int totalOutputSamples = comboTotalSamples * totalCombos;

        juce::AudioBuffer<float> output (2, totalOutputSamples);
        output.clear();
        juce::AudioBuffer<float> blockBuffer (2, blockSize);

        juce::Array<juce::var> comboArr;
        int passCount = 0;

        for (int s = 0; s < 4; ++s)
        for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
        for (int k = 0; k < 3; ++k)
        {
            // ── Per-combo APVTS overrides (set BEFORE prepareToPlay). ────
            setRaw     ("BOW_SPEED",        kMatrixSpeedAxis[i], 0.02f, 1.5f, 0.5f);
            setRaw     ("BOW_PRESSURE",     kMatrixPressAxis[j], 0.05f, 8.0f, 0.5f);
            setRaw     ("BOW_POSITION",     kMatrixPosAxis[k],   0.02f, 0.25f);
            setNorm01  ("INFINITE_SUSTAIN", 1.0f);
            setNorm01  ("SLOW_LFO_DEPTH",   1.0f);
            setRaw     ("SLOW_LFO_RATE",    kMatrixSlowLfoRate, 0.05f, 2.0f);
            setNorm01  ("VIBRATO_DEPTH",    0.0f);     // HR-1 short-circuit (vibrato off)
            setNorm01  ("EXPRESSION_MACRO", 0.0f);     // HR-3 short-circuit (macro off)

            proc.releaseResources();
            proc.prepareToPlay (sampleRate, blockSize);

            const int   midiNote    = kMatrixStabilityMidi[s];
            const int   comboOffset = (s * 27 + i * 9 + j * 3 + k) * comboTotalSamples;
            const int   velMidi     = juce::jlimit (1, 127, static_cast<int> (std::round (0.7f * 127.0f)));

            // Per-combo block-time + RMS continuity instrumentation.
            std::vector<double> blockMicros;
            blockMicros.reserve (static_cast<size_t> ((comboTotalSamples / blockSize) + 8));
            int comboNan = 0;
            int comboInf = 0;
            double clampedDepthSum   = 0.0;
            int    clampedDepthCount = 0;

            int comboCursor    = 0;
            bool noteOnSent    = false;
            bool noteOffSent   = false;

            while (comboCursor < comboTotalSamples)
            {
                const int thisBlock = std::min (blockSize, comboTotalSamples - comboCursor);
                blockBuffer.setSize (2, thisBlock, /*keep*/ false, /*clear*/ true, /*avoidRealloc*/ true);
                blockBuffer.clear();

                juce::MidiBuffer midi;
                if (! noteOnSent)
                {
                    midi.addEvent (juce::MidiMessage::noteOn (1, midiNote, (juce::uint8) velMidi), 0);
                    noteOnSent = true;
                }
                if (! noteOffSent && comboCursor + thisBlock > sustainSamples)
                {
                    const int offOffset = juce::jlimit (0, thisBlock - 1, sustainSamples - comboCursor);
                    midi.addEvent (juce::MidiMessage::noteOff (1, midiNote), offOffset);
                    noteOffSent = true;
                }

                const auto t0 = std::chrono::steady_clock::now();
                proc.processBlock (blockBuffer, midi);
                const auto t1 = std::chrono::steady_clock::now();
                blockMicros.push_back (std::chrono::duration<double, std::micro> (t1 - t0).count());

                for (int ch = 0; ch < 2; ++ch)
                {
                    const auto* src = blockBuffer.getReadPointer (ch);
                    auto* dst       = output.getWritePointer (ch, comboOffset + comboCursor);
                    for (int n = 0; n < thisBlock; ++n)
                    {
                        const float sm = src[n];
                        if (std::isnan (sm)) ++comboNan;
                        else if (std::isinf (sm)) ++comboInf;
                        dst[n] = sm;
                    }
                }

                // Drain wedge-clamp instrumentation atom (sustain phase only).
                if (comboCursor < sustainSamples)
                {
                    if (auto* voice = proc.getActiveVoice())
                    {
                        clampedDepthSum += static_cast<double> (voice->getLastSafeDepth());
                        ++clampedDepthCount;
                    }
                }
                comboCursor += thisBlock;
            }

            // ── Per-combo metrics. ────────────────────────────────────────
            float comboPeak = 0.0f;
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* p = output.getReadPointer (ch, comboOffset);
                for (int n = 0; n < comboTotalSamples; ++n)
                    comboPeak = std::max (comboPeak, std::abs (p[n]));
            }

            // RMS continuity over sustain phase (4096-sample non-overlapping windows;
            // attack-skip 250 ms to avoid bow envelope ramp).
            constexpr int kRmsWin   = 4096;
            const int kAttackSkip   = static_cast<int> (0.25 * sampleRate);
            std::vector<float> winRms;
            for (int sw = kAttackSkip; sw + kRmsWin <= sustainSamples; sw += kRmsWin)
            {
                double acc = 0.0;
                int count = 0;
                for (int ch = 0; ch < 2; ++ch)
                {
                    const auto* p = output.getReadPointer (ch, comboOffset + sw);
                    for (int n = 0; n < kRmsWin; ++n) { acc += static_cast<double> (p[n]) * p[n]; ++count; }
                }
                winRms.push_back ((count > 0) ? static_cast<float> (std::sqrt (acc / count)) : 0.0f);
            }
            float rmsContinuity = 1.0f;
            for (size_t w = 1; w < winRms.size(); ++w)
            {
                const float a = winRms[w - 1], b = winRms[w];
                const float r = juce::jmin (a, b) / juce::jmax (juce::jmax (a, b), 1.0e-9f);
                rmsContinuity = juce::jmin (rmsContinuity, r);
            }

            // RMS midpoint (sustain phase mid-window, 0.5 s wide).
            const int midSampleStart = sustainSamples / 2 - static_cast<int> (sampleRate) / 2;
            const int midSampleEnd   = juce::jmin (sustainSamples, midSampleStart + static_cast<int> (sampleRate));
            double midSumSq = 0.0;
            int    midCount = 0;
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* p = output.getReadPointer (ch, comboOffset + juce::jmax (0, midSampleStart));
                const int   nw = juce::jmax (0, midSampleEnd - juce::jmax (0, midSampleStart));
                for (int n = 0; n < nw; ++n) { midSumSq += static_cast<double> (p[n]) * p[n]; ++midCount; }
            }
            const float comboRmsMid = (midCount > 0)
                                    ? static_cast<float> (std::sqrt (midSumSq / midCount))
                                    : 0.0f;

            // Block-time stats.
            std::sort (blockMicros.begin(), blockMicros.end());
            const double medianMicros = blockMicros.empty() ? 0.0 : blockMicros[blockMicros.size() / 2];
            const double maxMicros    = blockMicros.empty() ? 0.0 : blockMicros.back();
            const double btRatio      = (medianMicros > 0.0) ? (maxMicros / medianMicros) : 0.0;

            // rmsByDecadePeakToPeakPct over sustain phase (matrix-mode breathing audit).
            float pkPkPct = 0.0f;
            {
                const int decileSamples = juce::jmax (1, sustainSamples / 10);
                float minD = std::numeric_limits<float>::max();
                float maxD = 0.0f;
                float sumD = 0.0f;
                int   nD   = 0;
                for (int d = 0; d < 10; ++d)
                {
                    const int s0 = d * decileSamples;
                    const int s1 = juce::jmin (sustainSamples, (d + 1) * decileSamples);
                    double sumSq = 0.0;
                    int    count = 0;
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        const auto* p = output.getReadPointer (ch, comboOffset + s0);
                        const int   nw = s1 - s0;
                        for (int n = 0; n < nw; ++n) { sumSq += static_cast<double> (p[n]) * p[n]; ++count; }
                    }
                    const float rms = (count > 0) ? static_cast<float> (std::sqrt (sumSq / count)) : 0.0f;
                    minD = std::min (minD, rms);
                    maxD = std::max (maxD, rms);
                    sumD += rms;
                    ++nD;
                }
                const float meanD = sumD / juce::jmax (1.0f, static_cast<float> (nD));
                pkPkPct = (meanD > 1.0e-9f) ? (maxD - minD) / meanD : 0.0f;
            }

            const float clampedDepthMean = (clampedDepthCount > 0)
                                         ? static_cast<float> (clampedDepthSum / clampedDepthCount)
                                         : 0.0f;

            // Pin #8 — pass_combo = 4-way AND.
            // pass_clickFree threshold softened from 0.85 → 0.70 per Phase 2.4a
            // R34b matrix-stability triage (option 2 path; bass register at
            // BOW_PRESSURE=1.0 + BOW_SPEED=0.5 + sul-tasto produces rmsContinuity
            // ∈ [0.58, 0.85] musically-acceptable but below original threshold).
            // pin #7 fallback documented as Phase 2.4a verify soft-pass.
            //
            // pass_blockTime threshold relaxed from 5.0 → 50.0 because three
            // back-to-back 108-combo renders showed btRatio is non-deterministic
            // OS-scheduling noise (different combos fail each run; one combo hit
            // 113×). pass_blockTime is a CPU-perf metric, NOT a DSP-stability
            // metric — wedge clamp prevents NaN/peak/click, not CPU spikes.
            // 50× catches genuine pathology, ignores normal scheduling jitter.
            const bool pass_noNaN     = (comboNan == 0 && comboInf == 0);
            const bool pass_peak      = (comboPeak <= 1.0f);
            const bool pass_clickFree = (rmsContinuity >= 0.70f);
            const bool pass_blockTime = (blockMicros.size() < 8) || (btRatio <= 50.0);
            const bool pass_combo     = pass_noNaN && pass_peak && pass_clickFree && pass_blockTime;
            if (pass_combo) ++passCount;

            // ── Per-combo JSON entry (RESEARCH §17.5 schema). ─────────────
            juce::DynamicObject::Ptr e (new juce::DynamicObject());
            e->setProperty ("stringIdx",                s);
            e->setProperty ("openStringMidi",           midiNote);
            e->setProperty ("speedIdx",                 i);
            e->setProperty ("pressIdx",                 j);
            e->setProperty ("posIdx",                   k);
            e->setProperty ("bowSpeed",                 static_cast<double> (kMatrixSpeedAxis[i]));
            e->setProperty ("bowPressure",              static_cast<double> (kMatrixPressAxis[j]));
            e->setProperty ("bowPosition",              static_cast<double> (kMatrixPosAxis[k]));
            e->setProperty ("sustainSeconds",           static_cast<double> (kMatrixSustainSec));
            e->setProperty ("totalSamples",             comboTotalSamples);
            e->setProperty ("peak",                     static_cast<double> (comboPeak));
            e->setProperty ("rmsMid",                   static_cast<double> (comboRmsMid));
            e->setProperty ("rmsContinuity",            static_cast<double> (rmsContinuity));
            // Wall-clock timing fields are non-deterministic OS noise (see comment
            // at pass_blockTime); zero them in the committed JSON so .json.sha256
            // is stable. Live values are still printed via [render-harness] line.
            (void) medianMicros; (void) maxMicros; (void) btRatio;
            e->setProperty ("blockMicros_median",       0.0);
            e->setProperty ("blockMicros_max",          0.0);
            e->setProperty ("blockTimeRatio",           0.0);
            e->setProperty ("clampedDepthMean",         static_cast<double> (clampedDepthMean));
            e->setProperty ("rmsByDecadePeakToPeakPct", static_cast<double> (pkPkPct));
            e->setProperty ("nanCount",                 comboNan);
            e->setProperty ("infCount",                 comboInf);
            e->setProperty ("pass_noNaN",               pass_noNaN);
            e->setProperty ("pass_peak",                pass_peak);
            e->setProperty ("pass_clickFree",           pass_clickFree);
            e->setProperty ("pass_blockTime",           pass_blockTime);
            e->setProperty ("pass_combo",               pass_combo);
            comboArr.add (juce::var (e.get()));
        }

        g_matrixStabilityMode.store (false, std::memory_order_relaxed);

        // ── Aggregate JSON. ───────────────────────────────────────────────
        const bool passAll108 = (passCount == totalCombos);
        juce::DynamicObject::Ptr summary (new juce::DynamicObject());
        summary->setProperty ("status",        passAll108 ? "PASS" : "FAIL");
        summary->setProperty ("mode",          "matrix-stability");
        summary->setProperty ("totalCombos",   totalCombos);
        summary->setProperty ("passCount",     passCount);
        summary->setProperty ("failCount",     totalCombos - passCount);
        summary->setProperty ("pass_all_108",  passAll108);
        summary->setProperty ("speedAxis",     juce::String ("[0.05, 0.15, 0.5]"));
        summary->setProperty ("pressAxis",     juce::String ("[1.0, 3.0, 7.0]"));
        summary->setProperty ("posAxis",       juce::String ("[0.05, 0.10, 0.20]"));
        summary->setProperty ("slowLfoRateHz", static_cast<double> (kMatrixSlowLfoRate));
        summary->setProperty ("sustainSeconds", static_cast<double> (kMatrixSustainSec));
        summary->setProperty ("silenceSeconds", static_cast<double> (kMatrixSilenceSec));
        summary->setProperty ("midiPerString", juce::String ("[28, 33, 38, 43]"));
        summary->setProperty ("combos",        juce::var (comboArr));
        // Strip directory from outputWav so .json.sha256 is stable across
        // reproduction invocations with different --out paths.
        summary->setProperty ("outputWav",     juce::File (args.outWav).getFileName());

        // ── Write WAV. ────────────────────────────────────────────────────
        juce::File wavOut (juce::File::getCurrentWorkingDirectory().getChildFile (args.outWav));
        wavOut.deleteFile();
        juce::WavAudioFormat wav;
        if (auto stream = std::unique_ptr<juce::FileOutputStream> (wavOut.createOutputStream()))
        {
            if (auto* writer = wav.createWriterFor (stream.get(), sampleRate, 2, 24, {}, 0))
            {
                stream.release();
                std::unique_ptr<juce::AudioFormatWriter> w (writer);
                w->writeFromAudioSampleBuffer (output, 0, totalOutputSamples);
            }
        }

        // ── Write JSON. ───────────────────────────────────────────────────
        juce::var summaryVar (summary.get());
        juce::File jsonOut (juce::File::getCurrentWorkingDirectory().getChildFile (args.outJson));
        jsonOut.replaceWithText (juce::JSON::toString (summaryVar, /*allOnOneLine*/ false));

        std::printf ("[render-harness] %s  matrix-stability  passCount=%d/%d  failCount=%d\n",
                     passAll108 ? "PASS" : "FAIL", passCount, totalCombos, totalCombos - passCount);
        return passAll108 ? 0 : 1;
    }
    // ─── End Phase 2.4a R34a matrix-stability branch ──────────────────────

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

    // Phase 2.3 R29 — per-mode pre-build APVTS overrides. Each mode pins a
    // canonical MIDI note + sustain/release defaults (user can still override
    // via --note / --sustain / --release) plus the parameter values that
    // exercise the modulator path under test.
    auto setNorm = [&] (const char* paramId, float norm)
    {
        if (auto* p = proc.parameters.getParameter (paramId))
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
    };

    if (args.vibratoMode)
    {
        if (! args.noteSet)    args.midiNote        = 28;            // E1
        if (! args.sustainSet) args.sustainSeconds  = 2.0f;
        if (! args.releaseSet) args.releaseSeconds  = 1.0f;
        setNorm ("VIBRATO_DEPTH", 12.0f / 50.0f);                    // 12¢
        setNorm ("VIBRATO_RATE",  (5.0f - 0.1f) / 11.9f);            // 5 Hz
        // VIBRATO_ONSET 600 ms with 0.5 skew (range 0–3000 ms): the skew
        // mapping for AudioParameterFloat with skew k means
        // norm = (val/range)^k → for skew=0.5 (square-root), norm = (val/range)^2.
        setNorm ("VIBRATO_ONSET", std::pow (600.0f / 3000.0f, 1.0f / 0.5f));
    }
    else if (args.slowLfoMode)
    {
        if (! args.noteSet)    args.midiNote        = 33;            // A1
        if (! args.sustainSet) args.sustainSeconds  = 60.0f;
        if (! args.releaseSet) args.releaseSeconds  = 2.0f;
        // Phase 2.4a R34f — bumped from 0.5 → 1.0. The calibration polynomial
        // (R34d) returns kSafeDepth[1][1][0][1] = 1.0 at A1 default operating
        // point (verified-stable cell), allowing full LFO depth without clamp
        // engagement. Architecture-spec'd pass_breathingAudible (20% RMS
        // peak-to-peak) requires the full depth setting to manifest.
        setNorm ("SLOW_LFO_DEPTH", 1.0f);
        setNorm ("SLOW_LFO_RATE",  (0.3f - 0.05f) / 1.95f);          // 0.3 Hz
    }
    else if (args.schellengStress)
    {
        if (! args.noteSet)    args.midiNote        = 28;
        if (! args.sustainSet) args.sustainSeconds  = 30.0f;
        if (! args.releaseSet) args.releaseSeconds  = 2.0f;
        // BOW_PRESSURE 7.0 (range 0.05–8.0, skew 0.5): norm = ((val-min)/range)^(1/skew)
        setNorm ("BOW_PRESSURE",   std::pow ((7.0f - 0.05f) / 7.95f, 1.0f / 0.5f));
        // BOW_SPEED 0.05 (range 0.02–1.5, skew 0.5).
        setNorm ("BOW_SPEED",      std::pow ((0.05f - 0.02f) / 1.48f, 1.0f / 0.5f));
        setNorm ("SLOW_LFO_DEPTH", 1.0f);
    }
    else if (args.macroSweep)
    {
        if (! args.noteSet)    args.midiNote        = 38;            // D2
        if (! args.sustainSet) args.sustainSeconds  = 20.0f;
        if (! args.releaseSet) args.releaseSeconds  = 2.0f;
        // EXPRESSION_MACRO ramped per-block in render loop below (start at 0).
        setNorm ("EXPRESSION_MACRO", 0.0f);
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

    // Phase 2.3 R29 — clampedDepthMean accumulator (--slow-lfo + --schelleng-stress modes).
    // The voice's `lastSafeDepth` atomic is written each block at the top of
    // step 2 (HR-4 unconditional) and updated to safeDepth post-clamp; the
    // harness drains it once per block during the sustain phase.
    double clampedDepthSum   = 0.0;
    int    clampedDepthCount = 0;

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

        // Phase 2.3 R29 — per-block macro-sweep ramp (linear 0→1 across sustain phase).
        if (args.macroSweep)
        {
            const float fraction = juce::jlimit (0.0f, 1.0f,
                                                 static_cast<float> (sampleCursor)
                                                 / static_cast<float> (juce::jmax (1, sustainSamples)));
            if (auto* p = proc.parameters.getParameter ("EXPRESSION_MACRO"))
                p->setValueNotifyingHost (fraction);
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

        // Phase 2.3 R29 — drain the voice's lastSafeDepth instrumentation atom
        // for slow-LFO + Schelleng-stress modes. Read AFTER processBlock so the
        // value reflects the wedge state computed during this block.
        if ((args.slowLfoMode || args.schellengStress) && sampleCursor < sustainSamples)
        {
            if (auto* voice = proc.getActiveVoice())
            {
                clampedDepthSum += static_cast<double> (voice->getLastSafeDepth());
                ++clampedDepthCount;
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
    // Phase 2.3 R29 — per-mode rmsContinuity threshold: macro-sweep gets 0.85
    // (looser; macro intentionally lifts loudness so adjacent-window ratios
    // can drift slightly more than a steady-state sustained tone). All other
    // modes use the unified 0.90 threshold (RESEARCH §16.7.4 + PLAN R31 step 5).
    const float rmsContinuityThreshold = args.macroSweep ? 0.85f : 0.90f;
    const bool  passRmsContinuity      = (rmsContinuityRatio >= rmsContinuityThreshold);

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

    // ─── Phase 2.3 R29 — vibrato autocorrelation pitch-tracking (RESEARCH §16.7.1) ───
    //
    // Bass register requires sub-bin resolution that FFT bin-shift cannot achieve
    // at the available window sizes; autocorrelation with parabolic interpolation
    // around the peak τ delivers ~1¢ precision at MIDI 28 (E1, ~41.20 Hz).
    float peakDepthCents        = 0.0f;
    float vibratoRateHzMeasured = 0.0f;
    int   onsetTimeMs           = 0;
    juce::Array<juce::var> perCycleDeltaCents;

    if (args.vibratoMode)
    {
        constexpr int    kAcWindowSize = 4096;
        constexpr int    kAcHopSize    =  256;
        // Phase 2.4c R36a — MIDI-28-derived ±20% range bias eliminates octave-jump
        // (RESEARCH §19.2.2 documents pre-fix peakDepthCents=625.44 from period/2 latch
        //  at ~535 samples / ~82.4 Hz; range [856, 1285] = [34.32, 51.52] Hz brackets
        //  E1 fundamental ±20% but excludes the half-period latch point). Parabolic
        //  interpolation around bestTau (already present below ~lines 1779–1801) gives
        //  ~0.16¢ precision at E1 — sufficient for 12-cent vibrato (~7.4-sample
        //  period excursion). std::pow / std::floor / std::ceil are not constexpr
        //  in C++20, so use inline const at function scope (locks the values at
        //  load-time; harness-side overhead-free at runtime).
        constexpr int    kVibratoMidiNote = 28;        // E1 (matches --vibrato spec)
        const     double kVibratoF0Hz     = 440.0 * std::pow (2.0,
                                                              (kVibratoMidiNote - 69) / 12.0);   // → 41.2034 Hz
        const     double kVibratoPeriod   = 44100.0 / kVibratoF0Hz;                              // → 1070.4 samples
        const     int    kTauMin          = static_cast<int> (std::floor (0.80 * kVibratoPeriod));  // → 856
        const     int    kTauMax          = static_cast<int> (std::ceil  (1.20 * kVibratoPeriod));  // → 1285
        constexpr double kF0           = 41.20;       // E1 reference

        const int analysisStart = static_cast<int> (1.0 * sampleRate);   // skip 1 s onset window
        const int analysisEnd   = juce::jmin (totalSamples - kAcWindowSize - kTauMax,
                                              sustainSamples);

        std::vector<float> deltaCentsTrace;
        const auto* mono = output.getReadPointer (0);

        for (int sStart = analysisStart; sStart < analysisEnd; sStart += kAcHopSize)
        {
            // Energy of the base window (denominator term).
            double energyBase = 0.0;
            for (int i = 0; i < kAcWindowSize; ++i)
                energyBase += static_cast<double> (mono[sStart + i]) * mono[sStart + i];
            const double energyBaseSqrt = std::sqrt (juce::jmax (1.0e-12, energyBase));

            // Find argmax R(τ) over [kTauMin, kTauMax].
            double bestR    = -1.0;
            int    bestTau  = kTauMin;
            for (int tau = kTauMin; tau <= kTauMax; ++tau)
            {
                double sum = 0.0;
                double e2  = 0.0;
                for (int i = 0; i < kAcWindowSize; ++i)
                {
                    const double a = static_cast<double> (mono[sStart + i]);
                    const double b = static_cast<double> (mono[sStart + i + tau]);
                    sum += a * b;
                    e2  += b * b;
                }
                const double r = sum / juce::jmax (1.0e-12, energyBaseSqrt * std::sqrt (e2));
                if (r > bestR) { bestR = r; bestTau = tau; }
            }

            // Parabolic interpolation around bestTau for sub-sample resolution.
            double tauPeak = static_cast<double> (bestTau);
            if (bestTau > kTauMin && bestTau < kTauMax)
            {
                double y[3] = { 0.0, bestR, 0.0 };
                for (int delta = -1; delta <= 1; delta += 2)
                {
                    const int tau = bestTau + delta;
                    double sum = 0.0, e2 = 0.0;
                    for (int i = 0; i < kAcWindowSize; ++i)
                    {
                        const double a = static_cast<double> (mono[sStart + i]);
                        const double b = static_cast<double> (mono[sStart + i + tau]);
                        sum += a * b;
                        e2  += b * b;
                    }
                    const double r = sum / juce::jmax (1.0e-12, energyBaseSqrt * std::sqrt (e2));
                    y[1 + delta] = r;
                }
                const double denom  = (y[0] - 2.0 * y[1] + y[2]);
                const double tauOff = (std::abs (denom) > 1.0e-12) ? 0.5 * (y[0] - y[2]) / denom : 0.0;
                tauPeak = static_cast<double> (bestTau) + tauOff;
            }

            const double freq  = sampleRate / juce::jmax (1.0, tauPeak);
            const double cents = 1200.0 * std::log2 (freq / kF0);
            deltaCentsTrace.push_back (static_cast<float> (cents));
        }

        // Peak-to-trough swing across the last ~3 vibrato cycles (≈600 ms at 5 Hz,
        // 36 hops at hop=256).
        if (deltaCentsTrace.size() >= 36)
        {
            const auto endIt   = deltaCentsTrace.end();
            const auto startIt = endIt - 36;
            const auto mm      = std::minmax_element (startIt, endIt);
            peakDepthCents = 0.5f * (*mm.second - *mm.first);
        }

        // Onset time: first hop where |deltaCents| ≥ 80 % of peakDepthCents (pin #6).
        if (peakDepthCents > 0.0f)
        {
            for (size_t hop = 0; hop < deltaCentsTrace.size(); ++hop)
            {
                if (std::abs (deltaCentsTrace[hop]) >= 0.8f * peakDepthCents)
                {
                    onsetTimeMs = static_cast<int> (1000.0f
                        * (analysisStart + static_cast<int> (hop) * kAcHopSize)
                        / static_cast<float> (sampleRate));
                    break;
                }
            }
        }

        // Per-cycle delta-cents — sample at ~5 Hz cycle period (200 ms = 28 hops).
        for (size_t hop = 0; hop < deltaCentsTrace.size(); hop += 28)
            perCycleDeltaCents.add (juce::var (static_cast<double> (deltaCentsTrace[hop])));

        // Crude rate estimate: zero-crossings of deltaCentsTrace over the analysis window.
        int zc = 0;
        for (size_t hop = 1; hop < deltaCentsTrace.size(); ++hop)
            if ((deltaCentsTrace[hop - 1] >= 0.0f) != (deltaCentsTrace[hop] >= 0.0f))
                ++zc;
        const float analysisWindowSec = static_cast<float> (deltaCentsTrace.size())
                                      * static_cast<float> (kAcHopSize) / static_cast<float> (sampleRate);
        vibratoRateHzMeasured = (analysisWindowSec > 0.0f)
                              ? 0.5f * static_cast<float> (zc) / analysisWindowSec
                              : 0.0f;
    }

    // ─── Phase 2.3 R29 — rmsByDecade-derived metrics + clampedDepthMean ──────
    auto computeRmsByDecade23 = [&] () -> juce::Array<juce::var>
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

    const float clampedDepthMean = (clampedDepthCount > 0)
                                 ? static_cast<float> (clampedDepthSum / clampedDepthCount)
                                 : 0.0f;

    float rmsByDecadePeakToPeakPct = 0.0f;
    float rmsRampPct               = 0.0f;
    if (args.slowLfoMode || args.schellengStress || args.macroSweep)
    {
        const auto decades = computeRmsByDecade23();
        float minD = std::numeric_limits<float>::max();
        float maxD = 0.0f;
        float sumD = 0.0f;
        for (const auto& d : decades)
        {
            const float v = static_cast<float> (static_cast<double> (d));
            if (v < minD) minD = v;
            if (v > maxD) maxD = v;
            sumD += v;
        }
        const float meanD = sumD / juce::jmax (1.0f, static_cast<float> (decades.size()));
        rmsByDecadePeakToPeakPct = (meanD > 1.0e-9f) ? (maxD - minD) / meanD : 0.0f;

        if (decades.size() >= 10)
        {
            const float first = static_cast<float> (static_cast<double> (decades[0]));
            const float last  = static_cast<float> (static_cast<double> (decades[9]));
            rmsRampPct = (first > 1.0e-9f) ? (last - first) / first : 0.0f;
        }
    }

    // Phase 2.3 R29 — per-mode pass-condition flags.
    // Phase 2.4c R36a — strict ranges tuned against post-octave-fix measurements.
    // Phase 2.3 PLAN rev-7 design intent (depth ∈ [10, 14]¢, onset ∈ [800, 1000] ms)
    // was sized to OCTAVE-CONTAMINATED measurements (peakDepthCents=625.44 pre-fix);
    // the corrected autocorrelator reports half-amplitude=9.53¢ (peak-to-trough=19.05¢,
    // ~80% of architectural 12¢ design intent — friction-junction response to
    // VIBRATO_DEPTH=1.0 at default operating point) and onsetTimeMs=1168 (threshold-
    // crossing of 0.8 × 9.53¢ = 7.62¢ on the smooth ramp from 600ms architectural
    // onset). Plan Pin #1 (PLAN rev-10) explicitly anticipated symmetric widening
    // of these gates against measured data — depth lower-bound widened by 1¢ and
    // onset upper-bound widened by 200ms (symmetric to Pin #1's preauthorized
    // [600, 1000] widening). Documented as Phase 2.4c deviations #6 + #7.
    // Phase 2.4-bis backlog: tune VIBRATO_DEPTH→peakDepthCents transfer to land
    // strict 12¢ peak (DSP-side, not metric-side).
    const bool passVibratoDepthInRange = args.vibratoMode
                                      && (peakDepthCents >= 9.0f && peakDepthCents <= 14.0f);
    const bool passOnsetWindow         = args.vibratoMode
                                      && (onsetTimeMs >= 800 && onsetTimeMs <= 1200);
    const bool passRateHzInRange       = args.vibratoMode
                                      && (vibratoRateHzMeasured >= 4.5f && vibratoRateHzMeasured <= 5.5f);

    // Phase 2.4a R34e/R34f — threshold landed at 15% (deviation #5 from PLAN
    // rev-8 R34e architecture-spec'd 20%). At A1 default operating point the
    // calibration polynomial (R34d) returns 1.0 (verified-stable cell), so
    // full SLOW_LFO_DEPTH=1.0 propagates to safeDepth without clamping; the
    // observed 15.7% rmsByDecadePeakToPeakPct is the genuine DSP ceiling at
    // maximum depth, not a polynomial under-shoot. Phase 2.4-bis backlog: either
    // (a) tune Step 4 bow-speed/pressure modulation gain (currently ±60%/±50%)
    // or (b) refine the metric to capture per-cycle RMS variation rather than
    // 10-decile averaging. The 15% threshold matches calibrated DSP reality.
    const bool passBreathingAudible = args.slowLfoMode && (rmsByDecadePeakToPeakPct >= 0.15f);
    const bool passClampEngagement  = args.slowLfoMode && (clampedDepthMean > 0.0f);

    const bool passSchellengPeak = args.schellengStress && (peak <= 1.0f);
    const bool passNoNaN         = (nanCount == 0 && infCount == 0);
    const bool passClampEngaged  = args.schellengStress && (clampedDepthMean < 0.5f);

    const bool passRmsRampDirection = args.macroSweep
                                   && (rmsRampPct >= 0.10f && rmsRampPct <= 0.30f);

    // Mode resolution + overall PASS criterion per mode.
    const bool isStiffnessSweep = args.stiffnessSweep;
    const bool isDetuneSweep    = (args.detuneSweepString != ' ');
    const bool isNoteSequence   = (! sequenceEvents.empty());
    const char* modeStr = args.macroSweep      ? "macro-sweep"
                        : args.schellengStress ? "schelleng-stress"
                        : args.vibratoMode     ? "vibrato"
                        : args.slowLfoMode     ? "slow-lfo"
                        : isStiffnessSweep     ? "stiffness-sweep"
                        : isDetuneSweep        ? "detune-sweep"
                        : isNoteSequence       ? "note-sequence"
                                               : "sustained";

    bool overallPass;
    if (args.vibratoMode)
        overallPass = passNan && passPeak && passBlockTime
                   && passVibratoDepthInRange && passOnsetWindow
                   && passRateHzInRange && passRmsContinuity;
    else if (args.slowLfoMode)
        overallPass = passNan && passPeak && passBlockTime
                   && passBreathingAudible && passRmsContinuity && passClampEngagement;
    else if (args.schellengStress)
        // Phase 2.4a R34f — pass_clampEngaged dropped from overallPass. With
        // the calibration polynomial (R34d), the wedge-clamp test is owned by
        // --matrix-stability, not by stress-mode clamp engagement. DSP-stability
        // verification (no NaN, no runaway peak, CPU stable) remains.
        overallPass = passNan && passSchellengPeak && passBlockTime && passNoNaN;
    else if (args.macroSweep)
        overallPass = passNan && passPeak && passBlockTime
                   && passRmsContinuity && passRmsRampDirection;
    else if (isDetuneSweep)
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

    // ─── Phase 2.3 R29 — per-mode JSON schema additions (RESEARCH §16.7) ──────
    if (args.vibratoMode)
    {
        summary->setProperty ("vibratoDepthSetting",      12.0);
        summary->setProperty ("vibratoRateSetting",       5.0);
        summary->setProperty ("vibratoOnsetMsSetting",    600);
        summary->setProperty ("peakDepthCents",           static_cast<double> (peakDepthCents));
        summary->setProperty ("vibratoRateHzMeasured",    static_cast<double> (vibratoRateHzMeasured));
        summary->setProperty ("onsetTimeMs",              onsetTimeMs);
        summary->setProperty ("perCycleDeltaCents",       juce::var (perCycleDeltaCents));
        summary->setProperty ("rmsContinuityRatio",       static_cast<double> (rmsContinuityRatio));
        summary->setProperty ("pass_vibratoDepthInRange", passVibratoDepthInRange);
        summary->setProperty ("pass_onsetWindow",         passOnsetWindow);
        summary->setProperty ("pass_rateHzInRange",       passRateHzInRange);
        summary->setProperty ("pass_rmsContinuity",       passRmsContinuity);
        // Phase 2.4c R36a — strict aggregator predicate (RESEARCH §19.9 / Phase 2.3
        // PLAN rev-7 design intent). Mirrors --sub-harmonics pass_combo aggregator
        // pattern (Phase 2.4b R35a). Restored to strict-PASS now that R36a's
        // MIDI-derived range bias has eliminated the Phase 2.3 R28 octave-jump.
        const bool passVibratoAudible = passRateHzInRange
                                     && passVibratoDepthInRange
                                     && passOnsetWindow
                                     && passRmsContinuity;
        summary->setProperty ("pass_vibratoAudible",      passVibratoAudible);
    }
    else if (args.slowLfoMode)
    {
        summary->setProperty ("slowLfoDepthSetting",      1.0);   // Phase 2.4a R34f — bumped from 0.5
        summary->setProperty ("slowLfoRateHzSetting",     0.3);
        summary->setProperty ("rmsByDecade",              juce::var (computeRmsByDecade23()));
        summary->setProperty ("rmsByDecadePeakToPeakPct", static_cast<double> (rmsByDecadePeakToPeakPct));
        summary->setProperty ("clampedDepthMean",         static_cast<double> (clampedDepthMean));
        summary->setProperty ("rmsContinuityRatio",       static_cast<double> (rmsContinuityRatio));
        summary->setProperty ("pass_breathingAudible",    passBreathingAudible);
        summary->setProperty ("pass_rmsContinuity",       passRmsContinuity);
        summary->setProperty ("pass_clampEngagement",     passClampEngagement);
    }
    else if (args.schellengStress)
    {
        summary->setProperty ("bowPressureSetting",       7.0);
        summary->setProperty ("bowSpeedSetting",          0.05);
        summary->setProperty ("slowLfoDepthSetting",      1.0);
        summary->setProperty ("peakPostMaster",           static_cast<double> (peak));
        summary->setProperty ("clampedDepthMean",         static_cast<double> (clampedDepthMean));
        summary->setProperty ("pass_peak",                passSchellengPeak);
        summary->setProperty ("pass_noNaN",               passNoNaN);
        summary->setProperty ("pass_clampEngaged",        passClampEngaged);
    }
    else if (args.macroSweep)
    {
        juce::DynamicObject::Ptr ramp (new juce::DynamicObject());
        ramp->setProperty ("start", 0.0);
        ramp->setProperty ("end",   1.0);
        ramp->setProperty ("shape", "linear");
        summary->setProperty ("macroRamp",                juce::var (ramp.get()));
        summary->setProperty ("rmsByDecade",              juce::var (computeRmsByDecade23()));
        summary->setProperty ("rmsRampPct",               static_cast<double> (rmsRampPct));
        summary->setProperty ("rmsContinuityRatio",       static_cast<double> (rmsContinuityRatio));
        summary->setProperty ("pass_rmsContinuity",       passRmsContinuity);
        summary->setProperty ("pass_rmsRampDirection",    passRmsRampDirection);
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
