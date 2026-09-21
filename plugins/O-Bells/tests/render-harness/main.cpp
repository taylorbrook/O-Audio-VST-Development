/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-Bells render-harness — processor-level offline renderer.

    Constructs the REAL OBellsAudioProcessor (no editor: this target is built
    with JUCE_WEB_BROWSER=0 and never compiles PluginEditor.cpp), recalls a
    factory preset through the real preset manager, plays one note over MIDI
    and writes the processor's output as raw interleaved-stereo float32.
    report.py drives it and computes the pairwise-distance / T40 / centroid
    report.

        O-Bells-render-test --list
            one "category|name" line per factory preset

        O-Bells-render-test --render <jobs.txt> <outdir>
                            [--note=60] [--vel=0.8] [--hold=0.25] [--total=6]
                            [--seed=1] [--fx] [--alloc-check]
            jobs.txt, one job per line, '|'-separated:
                <label>|preset=<category>/<name>|<paramId>=<value>|...
            `preset=` is optional (omitted = parameter defaults). Overrides
            are in ENGINEERING units and land after the preset. Job N is
            written to <outdir>/N.f32.

        O-Bells-render-test --check-user-owned
            Output Gain is the user's: sets it to -6 dB and Damping to a marker
            value, recalls every factory preset and fails if Output Gain moved.
            Control: Damping MUST have moved, or the recall did nothing and the
            check would pass on a dead load.

    Every job gets a FRESH processor, so nothing rings over from the previous
    render.

    Voice isolation (default; --fx turns it off): chorus / delay / reverb / EQ
    are bypassed and High Fidelity is on (no partial culling), so the render is
    the voice through the processor's limiter and one-pole LP only. That is the
    signal the differentiation metric was defined on — a voice-level tap that
    still ran through the reverb would measure `reverbMix`, not the engine
    (pattern_voice_level_tap_still_passes_through_processor_fx).

    --alloc-check (macOS): counts heap allocations made INSIDE processBlock, via
    libmalloc's malloc_logger hook — it sees malloc / calloc / realloc whoever
    calls them, so juce::HeapBlock and operator new are both covered — on the
    thread calling processBlock only (JUCE's TimerThread allocates at will). Any
    allocation fails the job and prints its backtrace. The hook is proven live first (a deliberate malloc
    must be counted), so a dead hook cannot read as a clean render.

    Determinism: BellVoice seeds its RNG from clock ^ `this`. --seed routes
    through the OBELLS_TEST_HOOKS seed hook (defined on this target only), so
    the same seed gives the same file and two different seeds give the
    self-noise measurement. Job N is seeded from (seed, N), so presets within
    one run stay statistically independent of each other.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdlib>
#if JUCE_MAC
 #include <execinfo.h>
 #include <pthread.h>
#endif
#include <fstream>
#include <iostream>
#include <string>

#if JUCE_MAC
// libmalloc's stack-logging hook (what MallocStackLogging / Instruments attach to).
extern "C"
{
    typedef void (malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                    uintptr_t result, uint32_t numHotFramesToSkip);
    extern malloc_logger_t* malloc_logger;
}
#endif

namespace
{
#if JUCE_MAC
// volatile: clang knows malloc() as a builtin that cannot touch globals, so it
// folds "armed = true; malloc(); armed = false" into "armed = false" and the hook
// never sees the flag up. Single-threaded harness: processBlock runs on main.
volatile bool allocArmed = false;
volatile int allocCount = 0;
bool allocTrace = false;
pthread_t audioThread;

void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
{
    // The hook is process-wide and JUCE's TimerThread mallocs (MessageQueue::post)
    // whenever it likes: only the thread that calls processBlock is the audio thread.
    if (allocArmed && (type & 2u) != 0 && pthread_equal (pthread_self(), audioThread))   // MALLOC_LOG_TYPE_ALLOCATE
    {
        allocArmed = false;   // the first one is the finding; say where it came from
        allocCount = allocCount + 1;
        if (allocTrace)
        {
            void* frames[32];
            backtrace_symbols_fd (frames, backtrace (frames, 32), 2);   // does not malloc
        }
    }
}
#endif

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

int fail (const juce::String& message)
{
    std::cerr << "O-Bells-render-test: " << message << "\n";
    return 1;
}

bool setEngineering (OBellsAudioProcessor& processor, const juce::String& id, float value)
{
    auto* param = processor.getAPVTS().getParameter (id);
    if (param == nullptr)
        return false;

    param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (value));
    return true;
}

int listPresets()
{
    OBellsAudioProcessor processor;

    for (const auto& [category, names] : processor.getPresetManager().getPresetListWithCategories())
    {
        if (category == "User")
            continue;

        for (const auto& name : names)
            std::cout << category << "|" << name << "\n";
    }

    return 0;
}

int checkUserOwned()
{
    OBellsAudioProcessor processor;
    auto* gain = processor.getAPVTS().getParameter ("outputGain");
    auto* damping = processor.getAPVTS().getParameter ("damping");

    if (gain == nullptr || damping == nullptr)
        return fail ("--check-user-owned: outputGain / damping parameter is gone");

    int checked = 0;

    for (const auto& [category, names] : processor.getPresetManager().getPresetListWithCategories())
    {
        if (category == "User")
            continue;

        for (const auto& name : names)
        {
            const float marker = 0.1234f;
            setEngineering (processor, "outputGain", -6.0f);
            damping->setValueNotifyingHost (marker);
            const float before = gain->getValue();

            if (! processor.getPresetManager().loadPresetFromCategory (category, name))
                return fail ("--check-user-owned: could not load '" + category + "/" + name + "'");

            if (std::abs (damping->getValue() - marker) < 1.0e-4f)
                return fail ("--check-user-owned: '" + name + "' left Damping on the marker - the recall did nothing");

            if (std::abs (gain->getValue() - before) > 1.0e-6f)
                return fail ("--check-user-owned: '" + name + "' moved Output Gain to "
                             + gain->getCurrentValueAsText() + " (it was -6 dB)");

            ++checked;
        }
    }

    if (checked == 0)
        return fail ("--check-user-owned: no factory presets found");

    std::cout << "Output Gain survived " << checked << " factory preset recalls\n";
    return 0;
}

struct RenderOptions
{
    int note = 60;
    float velocity = 0.8f;
    double holdSeconds = 0.25;
    double totalSeconds = 6.0;
    juce::int64 seed = 1;
    bool isolateVoice = true;
    bool allocCheck = false;
};

int renderJob (const juce::String& line, int index, const juce::File& outDir, const RenderOptions& options)
{
    auto fields = juce::StringArray::fromTokens (line, "|", "");
    const auto label = fields[0];

    OBellsAudioProcessor processor;

    for (int f = 1; f < fields.size(); ++f)
    {
        const auto key = fields[f].upToFirstOccurrenceOf ("=", false, false).trim();
        const auto value = fields[f].fromFirstOccurrenceOf ("=", false, false).trim();

        if (key == "preset")
        {
            const auto category = value.upToFirstOccurrenceOf ("/", false, false);
            const auto name = value.fromFirstOccurrenceOf ("/", false, false);

            if (! processor.getPresetManager().loadPresetFromCategory (category, name))
                return fail ("job " + juce::String (index) + " (" + label + "): factory preset '" + value + "' not found");
        }
        else if (! setEngineering (processor, key, value.getFloatValue()))
        {
            return fail ("job " + juce::String (index) + " (" + label + "): no parameter '" + key + "'");
        }
    }

    if (options.isolateVoice)
    {
        for (const auto* id : { "chorusBypass", "delayBypass", "reverbBypass", "eqBypass", "highFidelity" })
            if (! setEngineering (processor, id, 1.0f))
                return fail (juce::String ("isolation parameter '") + id + "' is gone — the voice is no longer isolated");
    }

    processor.setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
    processor.prepareToPlay (kSampleRate, kBlockSize);
    // Every job draws from its OWN stream. One seed shared across a run is
    // common random numbers: the per-note randomisation cancels BETWEEN presets
    // and near-neighbour distances read ~1.5 dB low — below the self-noise
    // measured on the same presets.
    processor.setVoiceSeedsForTesting (options.seed * 65536 + index);

    const int totalSamples = (int) (options.totalSeconds * kSampleRate);
    const int holdSamples = (int) (options.holdSeconds * kSampleRate);

    juce::AudioBuffer<float> block (2, kBlockSize);
    std::vector<float> interleaved;
    interleaved.reserve ((size_t) totalSamples * 2);

    bool released = false;
    float peak = 0.0f;

    for (int pos = 0; pos < totalSamples; pos += kBlockSize)
    {
        const int n = juce::jmin (kBlockSize, totalSamples - pos);
        block.setSize (2, n, false, false, true);

        juce::MidiBuffer midi;
        if (pos == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, options.note, options.velocity), 0);

        // Same block-boundary release the seed harness used, so tap/held
        // renders line up with the brief's baseline.
        if (! released && pos >= holdSamples)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, options.note), 0);
            released = true;
        }

       #if JUCE_MAC
        allocArmed = options.allocCheck;
       #endif
        processor.processBlock (block, midi);
       #if JUCE_MAC
        allocArmed = false;

        if (allocCount > 0)
            return fail ("job " + juce::String (index) + " (" + label + "): " + juce::String (allocCount)
                         + " heap allocation(s) inside processBlock, block at sample " + juce::String (pos));
       #endif

        for (int i = 0; i < n; ++i)
        {
            const float l = block.getSample (0, i), r = block.getSample (1, i);

            if (! std::isfinite (l) || ! std::isfinite (r))
                return fail ("job " + juce::String (index) + " (" + label + "): non-finite output at sample " + juce::String (pos + i));

            peak = juce::jmax (peak, std::abs (l), std::abs (r));
            interleaved.push_back (l);
            interleaved.push_back (r);
        }
    }

    processor.releaseResources();

    if (peak < 1.0e-6f)
        return fail ("job " + juce::String (index) + " (" + label + "): silent render (peak " + juce::String (peak) + ")");

    const auto outFile = outDir.getChildFile (juce::String (index) + ".f32");
    std::ofstream out (outFile.getFullPathName().toStdString(), std::ios::binary);
    out.write (reinterpret_cast<const char*> (interleaved.data()), (std::streamsize) (interleaved.size() * sizeof (float)));

    if (! out.good())
        return fail ("could not write " + outFile.getFullPathName());

    return 0;
}
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    juce::ArgumentList args (argc, argv);

    if (args.containsOption ("--list"))
        return listPresets();

    if (args.containsOption ("--check-user-owned"))
        return checkUserOwned();

    if (! args.containsOption ("--render") || args.size() < 3)
        return fail ("usage: --list | --render <jobs.txt> <outdir> [--note=n] [--vel=v] [--hold=s] [--total=s] [--seed=n] [--fx] [--alloc-check]");

    const int renderIndex = args.indexOfOption ("--render");
    const juce::File jobsFile = args[renderIndex + 1].resolveAsFile();
    const juce::File outDir = args[renderIndex + 2].resolveAsFile();

    RenderOptions options;
    if (args.containsOption ("--note"))  options.note = args.getValueForOption ("--note").getIntValue();
    if (args.containsOption ("--vel"))   options.velocity = args.getValueForOption ("--vel").getFloatValue();
    if (args.containsOption ("--hold"))  options.holdSeconds = args.getValueForOption ("--hold").getDoubleValue();
    if (args.containsOption ("--total")) options.totalSeconds = args.getValueForOption ("--total").getDoubleValue();
    if (args.containsOption ("--seed"))  options.seed = args.getValueForOption ("--seed").getLargeIntValue();
    options.isolateVoice = ! args.containsOption ("--fx");
    options.allocCheck = args.containsOption ("--alloc-check");

    if (options.allocCheck)
    {
       #if JUCE_MAC
        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);

        if (allocCount != 1)
            return fail ("--alloc-check: the malloc hook is not live (a deliberate malloc counted " + juce::String (allocCount) + ")");

        allocCount = 0;
        allocTrace = true;
       #else
        return fail ("--alloc-check is macOS-only");
       #endif
    }

    if (! jobsFile.existsAsFile())
        return fail ("jobs file '" + jobsFile.getFullPathName() + "' does not exist");

    if (! outDir.createDirectory())
        return fail ("could not create '" + outDir.getFullPathName() + "'");

    juce::StringArray lines;
    jobsFile.readLines (lines);
    lines.removeEmptyStrings();

    for (int i = 0; i < lines.size(); ++i)
        if (const int result = renderJob (lines[i], i, outDir, options); result != 0)
            return result;

    std::cout << "rendered " << lines.size() << "\n";
    return 0;
}
