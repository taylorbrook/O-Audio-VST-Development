/*
   This file is part of O-simpleReverb, an Ouaricon Audio plugin.
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

    O-simpleReverb - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Reverb engines live in Source/dsp/; this class hosts them in two ring-out
    slots and keeps everything after the reverb (type EQ, CHARACTER, LOW CUT,
    wet/dry, meter).

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OuariconPresetManager.h"
#include "dsp/FdnEngine.h"
#include "dsp/PlateEngine.h"
#include "dsp/SpringEngine.h"
#include "dsp/EarlyReflections.h"
#include "MeterFrame.h"
#include "TripleBuffer.h"
#include "PresetRecipes.h"

// Test hooks: defined on the render-check target only (CMakeLists.txt). A
// mutant is a deliberately broken build of one behaviour, which render-check
// --mutants uses to show that each gate fails when the thing it gates is gone.
#if OSIMPLEREVERB_TEST_HOOKS
 #define OSR_MUTANT(m) (testMutant == Mutant::m)
#else
 #define OSR_MUTANT(m) false
#endif

class OSimpleReverbAudioProcessor : public juce::AudioProcessor
{
public:
    // ==================== DSP Constants ====================

    // TYPE change: the playing slot's input falls to 0 and the slot rings out;
    // the other slot's input rises. The duck is 90 % there after kTypeDuckMs.
    // A slot that has to be taken back while it still sounds fades its OUTPUT
    // over kStealFadeMs first.
    static constexpr float kTypeDuckMs = 10.0f;
    static constexpr float kStealFadeMs = 10.0f;

    // A ringing slot is retired once its output has stayed under kRetireLevel
    // (-100 dB) for kRetireHoldMs - longer than the longest delay line, so a
    // gap between echoes is not mistaken for the end of the tail.
    static constexpr float kRetireLevel = 1.0e-5f;
    static constexpr float kRetireHoldMs = 300.0f;

    // The level law. A tank's steady level follows its decay time over its
    // length: left alone, the wet signal moves 4.1..4.9 dB across DECAY and
    // 2.1..4.9 dB across SIZE (small is louder). The slot's input gain takes
    // that back, in dB per octave of each control.
    //   SIZE: all of it, per engine (measured; render-check --levels). The tail
    //   then starts at the same level at every SIZE as well as averaging the
    //   same, since its length does not move.
    //   DECAY: half of the 2.2 dB per octave measured. All of it would start a
    //   long tail 4.4 dB under a short one; none of it runs DECAY 2.0x 2.4 dB
    //   hot. Half leaves 2.2 dB across the knob, split between the two.
    static constexpr float kDecayLevelDbPerOctave = 1.1f;
    static constexpr float kSizeLevelDbPerOctave[3] = { 2.25f, 2.05f, 2.6f };   // Fdn, Plate, Spring
    static constexpr float kLevelSlewMs = 60.0f;    // two poles of half this each, as the duck

    // Per-type voicing. DECAY multiplies baseT60; SIZE scales the engine's
    // lengths geometrically from sizeLo (0 %) to sizeHi (100 %), and moves
    // nothing else - the tail time holds.
    struct TypePreset {
        enum class Engine { Fdn, Plate, Spring } engine;
        float baseT60;              // mid-band RT60 in seconds at DECAY 1.0x
        float sizeLo, sizeHi;       // length scale at SIZE 0 / SIZE 100
        osr::FdnConfig fdn;         // the delay set and absorption (FDN types only: all zero otherwise)
        float earlySpanMs;          // last early-reflection tap at size scale 1 (0 = no taps)
        float earlyLevel;           // early reflections at the slot output, re the tank
        float preDelayMs;           // ahead of the taps and the tank; not scaled by SIZE
        float eqFreq;               // Type-specific EQ frequency
        float eqGain;               // Type-specific EQ gain (dB)
        float eqQ;                  // Type-specific EQ Q
        enum class EqType { None, LowShelf, HighShelf, Peak, HighPass } eqType;
        float wetTrimDb;            // level match across types at defaults
        float monoTrimDb;           // on a mono bus, on top of the fold: what the type's L/R correlation adds
    };

    static const TypePreset typePresets[6];

    // Which engine a type runs.
    static TypePreset::Engine engineOf(int typeIndex) { return typePresets[typeIndex].engine; }

    // Engine length scale for a SIZE in percent.
    static float sizeScale(const TypePreset& preset, float sizePercent)
    {
        return preset.sizeLo * std::pow(preset.sizeHi / preset.sizeLo, juce::jlimit(0.0f, 1.0f, sizePercent / 100.0f));
    }

    // v3.0.0: the tone stage's maps, as pure functions. The DSP below and the
    // editor's getToneCurve native both call these, so the drawn curve cannot
    // drift from the filters that run (render-check gates the agreement).
    //   warmCutoffHz        CHARACTER's Warm low-pass cutoff (parked at 0.49 fs from 0 up)
    //   brightGainDb        CHARACTER's Bright shelf gain at kBrightShelfHz
    //   typeEqCoefficients  the type's EQ biquad, as the slot's ArrayCoefficients
    static constexpr float kBrightShelfHz = 4000.0f, kBrightShelfQ = 0.707f;
    static float warmCutoffHz(float characterValue, double sampleRate);
    static float brightGainDb(float characterValue);
    static std::array<float, 6> typeEqCoefficients(int typeIndex, double sampleRate);

    // v3.0.0 - the models behind the editor's tone and early-reflection wells.
    // Pure functions of their arguments; the editor serialises them and
    // render-check holds them to the filters and taps that actually run.
    //   toneGainDb    the steady-state gain of the wet path after the tank at
    //                 one frequency: type EQ x Warm x Bright (x Low Cut when on).
    //                 The TPT SVFs (R = 1/sqrt 2) are bilinear Butterworths
    //                 prewarped at their cutoff: the RBJ low / high-pass at the
    //                 same frequency is the same filter.
    //   lowCutGainDb  the Low Cut alone (drawn as its own curve while OUT).
    //   earlyTaps     where a type's taps land at a SIZE, read from an
    //                 EarlyReflections the CALLER prepared at `modelRate` with
    //                 kEarlyModelSpanMs; Spring's three echo times instead.
    static constexpr int kToneBins = 128;                  // the page's tone-well resolution
    //   typeEqGainDb, characterGainDb: the two other parts, for the page's
    //   per-stage curves; toneGainDb is their sum (plus Low Cut when on).
    double toneGainDb(int typeIndex, float characterValue, float lowCutHz, bool lowCutOn, double sampleRate, double hz) const;
    static double typeEqGainDb(int typeIndex, double sampleRate, double hz);
    double characterGainDb(float characterValue, double sampleRate, double hz) const;
    static double lowCutGainDb(float lowCutHz, double sampleRate, double hz);
    struct EarlyTaps {
        TypePreset::Engine engine = TypePreset::Engine::Fdn;
        double preDelayMs = 0.0, earlyLevel = 0.0;
        int numTaps = 0;                                   // 8 on the FDN types, 0 on Plate and Spring
        std::array<double, osr::EarlyReflections::kTaps> msL {}, gainL {}, msR {}, gainR {};
        int numSpringEchoes = 0;                           // 3 on Spring
        std::array<double, osr::SpringEngine::kSprings> springEchoMs {};
    };
    static constexpr float kEarlyModelSpanMs = 57.5f;      // the longest earlySpanMs of the six types
    static EarlyTaps earlyTaps(osr::EarlyReflections& model, double modelRate, int typeIndex, float sizePercent);

#if OSIMPLEREVERB_TEST_HOOKS
    enum class Mutant {
        none,
        decayDead,      // DECAY does not reach the engine
        sizeIsDecay,    // SIZE scales the decay time as well as the lengths
        sizeDead,       // SIZE does not reach the engine
        sameDelays,     // every FDN type runs Hall's delay set
        earlyMuted,     // early reflections at level 0
        monoTail,       // right output = left output
        noRingOut,      // a TYPE change clears the old slot instead of letting it ring
        noNanGuard,     // a non-finite slot output is passed on
        noChunking,     // an oversized host block is processed in one piece
        noGlide,        // SIZE and DECAY land at once instead of gliding
        hardSteal,      // a sounding slot is taken back without its output fade
        noShimmer,      // the plate's cross-feeds bypass the octave shifter
        decayStuck,     // the engine is asked for an endless tail (the plate sits on its decay ceiling)
        springNoChirp,  // the spring's allpass cascade is out of its loop
        springOpenBand, // the spring's low-pass is out of its loop
        noLevelLaw,     // the slot's input gain ignores DECAY and SIZE
        frameNoReset,   // the meter frame never starts again after a read (v3.0.0)
        toneCurveStale  // getToneCurve ignores CHARACTER (v3.0.0; read by the editor's model)
    };
    Mutant testMutant = Mutant::none;
    bool testInjectNaN = false;     // poisons the playing slot's input once, then clears itself
#endif

    OSimpleReverbAudioProcessor();
    ~OSimpleReverbAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // WR-05: constrain layouts to mono/stereo. The default returns true for
    // anything, letting surround hosts negotiate >2-channel layouts that the
    // reverb (stereo-max) would silently pass through dry.
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        auto out = layouts.getMainOutputChannelSet();
        return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
            && layouts.getMainInputChannelSet() == out;
    }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-simpleReverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 15.0; }   // Ambient at DECAY 2.0x: 14 s, plus pre-delay

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;

    // v3.0.0 - the visualisation tap (Source/MeterFrame.h): one frame per
    // host block, every field held since the editor's last read. Replaces
    // the v2 outputPeak atomic.
    osr::TripleBuffer<osr::MeterFrame> meterFrames;

    // ------------------------------------------------------------------------
    // v1.6.0 - the UI language. 0 = en, 1 = fr, 2 = zh-Hans.
    //
    // An INDEX rather than a string because std::atomic<juce::String> does not
    // compile (juce::String is not trivially copyable), so the audio-safe form
    // is an index behind the two-function codec below while the PERSISTED form
    // stays a readable language code.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their plugin in. It rides the APVTS state tree as a
    // non-parameter property instead.
    // ------------------------------------------------------------------------
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is neither "fr" nor
        "zh-Hans" to 0, so a hand-edited session or an unexpected argument from
        the page degrades to English rather than being stored unvalidated.

        PURE ASCII, AND THAT IS THE CONTRACT. Not one Han character exists
        anywhere under Source/ — every Chinese string lives in the UI table
        (Source/ui/public/js/i18n.js), and the one Han string in the markup is
        written as numeric character references. What is persisted here is a
        language CODE, which is an ASCII identifier in every language. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

    // ------------------------------------------------------------------------
    // v3.0.0 - the editor's window scale (width / 1180), persisted in the
    // state as one more plain property beside uiLanguage. 0 = never sized:
    // the editor fits the display at open. Written as "0.850" only when > 0,
    // restored only when finite and within 0.6 .. 1.5.
    // ------------------------------------------------------------------------
    std::atomic<float> editorScale { 0.0f };

    // ------------------------------------------------------------------------
    // v3.0.0 - presets and lessons. The state stays the preset module's XML
    // (its currentPreset attribute is the name); this is the family SURFACE
    // over it: Init + 10 lessons from Source/PresetRecipes.h (never on disk),
    // the 48 utility presets of the factory bank, and user presets.
    // Message thread unless noted; presetLock guards the identity, is never
    // taken on the audio thread and never held across setValueNotifyingHost.
    // ------------------------------------------------------------------------
    static constexpr int kNumParams = 8;
    static constexpr const char* kParamIds[kNumParams] = { "TYPE", "CHARACTER", "WET", "DRY", "DECAY", "SIZE", "LPFREQ", "LPON" };
    using Targets = std::array<float, (size_t) kNumParams>;   // normalised, kParamIds order; NaN = default

    // The factory bank (8 per type, TYPE order), as the module writes it.
    static juce::StringArray factoryBankNames();
    static bool isBankPresetName(const juce::String& name);
    static bool isFactoryPresetName(const juce::String& name);       // a lesson or a bank preset (ignoring case)
    // v2.0.0 had no lessons, so a user preset could be called "Init" (or a
    // lesson file name); v3 would hide it behind the lesson. Renames each such
    // file in userDir to "<name> (user).json" ("<name> (user 2)" etc. if taken). Never
    // deletes; a failed rename leaves the file as it was. Returns the count.
    static int migrateLessonNameCollisions(const juce::File& userDir);
    static juce::String migratedPresetName(const juce::String& lessonFileName);   // "<name> (user)"
    // A restored lesson name whose session values match the renamed user
    // preset's (and not the lesson's) was that user preset.
    static bool restoredValuesAreMigratedPreset(const Targets& current, const Targets& lesson, const Targets& file);

    // The lesson entry: any table id; unknown -> false, nothing touched. Each
    // apply is a full reset to defaults, then the recipe.
    bool applyFactoryPreset(const juce::String& id);
    // The browser entry: a lesson file name, a bank name or a LISTED user
    // preset (anything else, including a path, is refused).
    bool loadPresetByName(const juce::String& name);
    bool loadPresetFile(const juce::File& file);                      // the gear menu's "Load file..."

    // Identity. Any non-audio thread.
    juce::String getPresetName() const;
    juce::String getPresetId() const;                                 // the lesson id, or ""
    bool isPresetFactory() const;
    juce::uint32 getPresetRevision() const noexcept { return presetRevision.load(); }
    // Any parameter > 1e-4 from the values the preset set. A restored session
    // whose targets are not known (yet) is unmodified.
    bool isPresetModified() const;
    // A restored USER name re-reads its file once (editor timer, message
    // thread) - never inside setStateInformation.
    void refreshPresetTargetsIfNeeded();

    // Walk order: the bank in TYPE order, then user presets sorted ignoring
    // case. The list, the arrows and the dropdown all use this order.
    juce::StringArray getPresetWalkOrder() const;
    juce::String getNeighbourPreset(int dir) const;

    // "" = refused: trim; control and bidi characters stripped;
    // File::createLegalFileName; leading dots stripped; 64 characters.
    static juce::String sanitisePresetName(const juce::String& raw);
    bool saveUserPreset(const juce::String& rawName);                 // refuses bank and lesson names
    bool deleteUserPreset(const juce::String& name);
    juce::var getPresetCatalog() const;                               // { lessons:[{name,id}], factory:[...], user:[...] }

    // Recipe -> normalised targets (unlisted = NaN), and NaN -> default.
    Targets recipeTargets(const srpresets::Recipe& recipe) const;
    Targets resolveTargets(const Targets& targets) const;
    Targets currentNormalisedValues() const;
    bool applyNormalisedTargets(const Targets& targets);

private:
    mutable juce::CriticalSection presetLock;
    juce::String presetName;
    Targets presetTargets {};
    bool presetTargetsValid = false;
    bool presetTargetsStale = false;
    juce::String migratedCandidate;   // restored lesson name that may be a renamed v2 user preset
    bool readUserPresetTargets(const juce::String& name, Targets& out) const;
    std::atomic<juce::uint32> presetRevision { 0 };
    void setCurrentPreset(const juce::String& name, const Targets& resolved, bool valid, bool stale);
    void restorePresetIdentity();   // after setStateInformation
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components
    // CHARACTER = two filters that ALWAYS run and never change family: a Warm
    // low-pass (parked near Nyquist unless warm) into a Bright high shelf (0 dB
    // unless bright). Switching one biquad between low-pass and shelf, or
    // resetting it, clicked at every crossing of centre. The low-pass is a TPT
    // state-variable filter: a biquad parked near Nyquist has poles by z = -1
    // and its state explodes on the next coefficient change.
    juce::dsp::StateVariableTPTFilter<float> warmFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> brightFilter;
    juce::SmoothedValue<float> characterSmoothed;
    static constexpr int kCharacterSubBlock = 32;  // LOW CUT cutoff update interval (samples)
    void updateCharacterCoefficients(float characterValue);

    // === Reverb slots ===
    // Two of them. One plays the current TYPE. On a TYPE change its input
    // ducks to zero and it keeps running - the old tail rings out under its
    // own type, size, decay and EQ - while the other slot starts the new type
    // from silence. Clearing the tank at the switch would cut the tail on
    // every type and preset change. A third change while both still sound
    // takes the older slot back behind a fast output fade.
    struct Slot {
        enum class State { idle, playing, ringing, stolen };

        // One engine of each kind; only the one the slot's type runs is processed
        osr::FdnEngine fdn;
        osr::PlateEngine plate;
        osr::SpringEngine spring;
        osr::EarlyReflections early;
        osr::RingDelay preDelayL, preDelayR;
        // Type EQ: one per slot, so a ringing tail keeps its own type's EQ
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> eq;
        juce::AudioBuffer<float> out;   // the slot's output for one chunk, bus-wide

        State state = State::idle;
        int type = -1;
        int preDelaySamples = 1;
        float inputGain = 0.0f;         // the duck: rises to 1 while playing, falls to 0 otherwise
        float inputLead = 0.0f;         //   its first pole (see renderSlot)
        float outputGain = 1.0f;        // the steal fade's position: 1, except while a stolen slot fades
        float trim = 1.0f;              // the type's wetTrimDb and the level law, at the INPUT (see renderSlot)
        float trimLead = 1.0f;          //   its first pole
        float trimTarget = 1.0f;        //   where DECAY and SIZE now put it (see driveSlot)
        float earlyLevel = 0.0f;
        float monoFold = 0.70710678f;   // what a mono bus multiplies L + R by (see renderSlot)
        int quietSamples = 0;           // how long the output has been under kRetireLevel
    };
    std::array<Slot, 2> slots;
    int currentSlot = 0;                // the slot that plays, or is about to play, the current TYPE

    void startSlot(Slot& slot, int typeIndex, float decayValue, float sizeValue, bool inputOpen);
    void clearSlot(Slot& slot);
    void driveSlot(Slot& slot, float decayValue, float sizeValue);
    void renderSlot(Slot& slot, const float* inL, const float* inR, int numSamples);
    void setTypeEq(Slot& slot, int typeIndex);

    // User LOW CUT (a high-pass, 20-400 Hz; the LPFREQ/LPON IDs are historical).
    // v1.12.0: always runs, into lowCutBuffer, so ON/OFF is a crossfade rather
    // than a switch and the filter state is never stale. A TPT SVF because its
    // cutoff is modulated (smoothed) every kCharacterSubBlock samples.
    juce::dsp::StateVariableTPTFilter<float> lowCutFilter;
    juce::AudioBuffer<float> lowCutBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> lowCutFreqSmoothed;
    juce::SmoothedValue<float> lowCutMixSmoothed;

    // Pre-allocated buffers (sized in prepareToPlay — CR-04)
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> wetBuffer;

    // Wet/dry gain smoothing (WR-03, 20ms linear ramp)
    juce::SmoothedValue<float> wetGainSmoothed;
    juce::SmoothedValue<float> dryGainSmoothed;

    // Cached parameter pointers (never change after construction)
    std::atomic<float>* typeParam = nullptr;
    std::atomic<float>* characterParam = nullptr;
    std::atomic<float>* sizeParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* wetParam = nullptr;
    std::atomic<float>* dryParam = nullptr;
    std::atomic<float>* lpFreqParam = nullptr;
    std::atomic<float>* lpOnParam = nullptr;

    // The meter frame's accumulators (audio thread only). They start again
    // only once the editor has read the last frame (hasNewData() false), so a
    // 30 Hz reader sees every burst. With no editor open nothing reads, and the
    // wet sums are halved whenever they cover more than a second (the mean
    // holds; nothing overflows).
    float meterInPeak = 0.0f, meterOutPeakL = 0.0f, meterOutPeakR = 0.0f;
    double meterWetSumSq = 0.0, meterWetCount = 0.0, meterSpanSamples = 0.0;
    void publishMeterFrame(const juce::AudioBuffer<float>& buffer);

    // State tracking
    double currentSampleRate = 44100.0;
    int maxChunkSamples = 512;             // the prepared block size: nothing below is sized for more
    float duckCoeff = 1.0f;                // of each of the duck's two poles
    float levelCoeff = 1.0f;               // of each of the level law's two poles
    float stealStep = 0.0f;                // per sample, for kStealFadeMs
    int retireHoldSamples = 0;
    float previousCharacterValue = 0.0f;   // seeded out of range in prepareToPlay

    // Helper methods
    void processChunk(juce::AudioBuffer<float>& buffer, int start, int numSamples);
    static float wetTrimGain(int typeIndex) { return juce::Decibels::decibelsToGain(typePresets[typeIndex].wetTrimDb); }
    static float levelLawDb(const TypePreset& preset, float decayValue, float scale);
    void initializeFactoryPresets();

    template<typename FilterType>
    void prepareFilterAsAllPass(FilterType& filter, const juce::dsp::ProcessSpec& spec, double sampleRate) {
        filter.prepare(spec);
        filter.reset();
        // ArrayCoefficients assignment primes the state's coefficient storage so
        // later audio-thread coefficient updates never reallocate (CR-03)
        *filter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(sampleRate, 1000.0f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSimpleReverbAudioProcessor)
};
