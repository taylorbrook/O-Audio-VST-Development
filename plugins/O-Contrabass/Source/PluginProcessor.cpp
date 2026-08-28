/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
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

    O-Contrabass — Audio Processor Implementation
    (Stage 1: Foundation — APVTS shell, silent output, no DSP)

    Parameter ID convention: UPPER_SNAKE_CASE per parameter-spec.md
    (sha256:ae956e9487465dcaa57cf1d1cf6a640f0856614cb2e1b4c93d240cf789490a52).
    This differs from sibling plugins (which use lowerCamelCase) — IDs are
    a frozen contract; renaming breaks DAW automation persistence.

  ==============================================================================
*/

#include "PluginProcessor.h"
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"   // WebView editor — excluded from the render harness (JUCE_WEB_BROWSER=0)
#endif
#include "BowedContrabassVoice.h"
#include <cmath>

// Phase 2.4a HR-7 — production-side weak default for harness wedge-math bypass.
// Harness binary overrides this with a strong symbol in tests/render-harness/main.cpp.
// Outside the harness this returns false unconditionally, leaving the production
// Schelleng wedge / calibration-polynomial path always live.
//
// macOS / Linux: weak symbols compose with extern "C" linkage so the harness
// strong symbol takes precedence at link time. Windows MSVC would require
// __declspec(selectany) — out of scope for Phase 2.4a (O-Contrabass macOS-AU first).
extern "C" __attribute__((weak)) bool isMatrixStabilityModeActive() noexcept
{
    return false;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OContrabassAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using APC = juce::AudioParameterChoice;
    using APB = juce::AudioParameterBool;
    using NR  = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // -- Tier 1: Primary Controls (5 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_SPEED", 1},     "Bow Speed",
        NR(0.02f, 1.5f, 0.001f, 0.5f),     0.15f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_PRESSURE", 1},  "Bow Pressure",
        NR(0.05f, 8.0f, 0.01f, 0.5f),      1.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_POSITION", 1},  "Bow Position",
        NR(0.02f, 0.25f, 0.001f),          0.10f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BRIGHTNESS", 1},    "Brightness",
        NR(80.0f, 12000.0f, 1.0f, 0.25f),  4500.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"OUTPUT_GAIN", 1},   "Output Level",
        NR(-60.0f, 12.0f, 0.1f),           0.0f));

    // -- Tier 2: Secondary Controls (5 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"ROSIN", 1},         "Rosin",
        NR(0.0f, 1.0f, 0.001f),            0.65f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_NOISE", 1},     "Bow Noise",
        NR(0.0f, 1.0f, 0.001f),            0.35f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BODY_SIZE", 1},     "Body Size",
        NR(0.0f, 1.0f, 0.001f),            0.75f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BODY_DAMPING", 1},  "Body Damping",
        NR(0.0f, 1.0f, 0.001f),            0.40f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BODY_MIX", 1},      "Body Mix",
        NR(0.0f, 1.0f, 0.001f),            0.80f));

    // -- Tier 3: String Configuration (3 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"STRING_TENSION", 1},   "String Tension",
        NR(0.0f, 1.0f, 0.001f),            0.50f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"STRING_STIFFNESS", 1}, "String Stiffness",
        NR(0.0f, 1.0f, 0.001f),            0.30f));
    layout.add(std::make_unique<API>(juce::ParameterID{"ACTIVE_STRINGS", 1},   "Active Strings",
        1, 4, 4));

    // -- Per-String Detune (4 params, scordatura / just-intonation drones) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"DETUNE_E", 1}, "E String Detune",
        NR(-1200.0f, 1200.0f, 0.1f),       0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"DETUNE_A", 1}, "A String Detune",
        NR(-1200.0f, 1200.0f, 0.1f),       0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"DETUNE_D", 1}, "D String Detune",
        NR(-1200.0f, 1200.0f, 0.1f),       0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"DETUNE_G", 1}, "G String Detune",
        NR(-1200.0f, 1200.0f, 0.1f),       0.0f));

    // -- Expression (6 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"VIBRATO_RATE", 1},     "Vibrato Rate",
        NR(0.1f, 12.0f, 0.01f),            5.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"VIBRATO_DEPTH", 1},    "Vibrato Depth",
        NR(0.0f, 50.0f, 0.1f),             0.0f));    // Phase 2.3 — default flipped 12.0 → 0.0 to preserve Phase 2.2 strict byte-equal regression bar (HR-1 short-circuit). Mirrors EXPRESSION_MACRO Q7a precedent.
    layout.add(std::make_unique<APF>(juce::ParameterID{"VIBRATO_ONSET", 1},    "Vibrato Onset",
        NR(0.0f, 3000.0f, 1.0f, 0.5f),     600.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"SLOW_LFO_RATE", 1},    "Slow Bow LFO Rate",
        NR(0.05f, 2.0f, 0.001f),           0.3f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"SLOW_LFO_DEPTH", 1},   "Slow Bow LFO Depth",
        NR(0.0f, 1.0f, 0.001f),            0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"EXPRESSION_MACRO", 1}, "Expression Macro",
        NR(0.0f, 1.0f, 0.001f),            0.0f));   // Phase 2.3 Q7a — preserves Phase 2.2 strict byte-equal regression bar.

    // -- Drone Features (2 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"INFINITE_SUSTAIN", 1}, "Infinite Sustain",
        NR(0.0f, 1.0f, 0.001f),            0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"SUB_HARMONICS", 1},    "Sub-Harmonics",
        NR(0.0f, 1.0f, 0.001f),            0.0f));

    // -- Release (v1.3, 1 param) --
    // Sits with the drone pair because it is the other half of the same control:
    // INFINITE_SUSTAIN governs decay while the bow is DOWN, RELEASE governs it
    // after the bow lifts. Before v1.3 there was no second half — the loop gain
    // was unchanged by note-off, so a released string rang for minutes and held
    // its voice slot the whole time.
    //
    // 0.05-20 s, skewed 0.35 so the musical 0.5-4 s span occupies the middle of
    // the knob instead of its first eighth. Default 2.0 s = a released orchestral
    // bass note; NOT a no-op default, because the v1.2 behaviour it replaces is
    // the defect. Sessions saved before v1.3 therefore adopt 2.0 s on load.
    layout.add(std::make_unique<APF>(juce::ParameterID{"RELEASE", 1},          "Release",
        NR(0.05f, 20.0f, 0.01f, 0.35f),    2.0f));

    // -- Output (1 param) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"WIDTH", 1},            "Width",
        NR(0.0f, 2.0f, 0.001f),            1.0f));

    // -- Output Chain (Phase 2.6a additions) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"MASTER_SAT_AMOUNT", 1},  "Master Saturator",
        NR(0.0f, 1.0f, 0.001f),            0.50f));   // 50% wet/dry default
    layout.add(std::make_unique<APF>(juce::ParameterID{"LIMITER_CEILING_DB", 1}, "Limiter Ceiling",
        NR(-6.0f, 0.0f, 0.01f),            -0.3f));   // -0.3 dBFS per CONTEXT rev-11 Q4 LOCKED

    // -- Microtonal Tuning (3 params, Ouaricon convention) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"REFERENCE_PITCH", 1},  "Reference Pitch",
        NR(220.0f, 880.0f, 0.01f),         440.0f));
    layout.add(std::make_unique<APC>(juce::ParameterID{"TUNING_SYSTEM", 1},    "Tuning System",
        // Stage 3 D1: label "Scala" (was "Scala/TUN") — TuningEngine 2.1.0 has
        // no TUN parser; a label promising .tun would be a silent-failure trap.
        // Choice INDEX mapping is frozen (0=Scala, 1=MTS-ESP, 2=12-TET); label
        // text is cosmetic and does not affect automation or state.
        juce::StringArray { "Scala", "MTS-ESP", "12-TET" }, 2));
    layout.add(std::make_unique<APB>(juce::ParameterID{"NOTE_EXPRESSION", 1},  "Note Expression",
        true));

    return layout;
}

//==============================================================================
OContrabassAudioProcessor::OContrabassAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Phase 2.6b R40a — pass tuningEngine ptr to voice so noteStarted /
    // notePitchbendChanged can resolve frequency through the engine
    // (ESCALATION-RP1 voice-side ratio multiplication; cache field Q17).
    // tuningEngine is declared BEFORE synth in the header → constructed
    // first → pointer is valid here (Risk #32 mitigation).
    //
    // WR-10: add 4 voices (one per EADG string) so double-stop drones — the core
    // use case, and what ACTIVE_STRINGS / the four DETUNE_* params / INFINITE_SUSTAIN
    // are built for — don't steal each other. A single voice made the synth
    // effectively monophonic (2nd note-on stole the 1st). Physical-model voices are
    // CPU-heavy (2× oversampled waveguide + 8-mode body resonator each), so 4 balances
    // polyphony against cost; MPESynthesiser allocates per note-on.
    // Phase 2.6c R41a — hand each voice a pointer to the module-owned pending
    // NE tuning table (O-Lyrica .cpp:512 D-09 pattern). vst3Extensions is
    // declared AFTER tuningEngine, BEFORE synth (member-order contract), so
    // the table address is valid here.
    for (int i = 0; i < kNumVoices; ++i)
    {
        auto* voice = new BowedContrabassVoice(&parameters, &tuningEngine);
        voice->setPendingTuningSource(&vst3Extensions.getPendingTable());
        synth.addVoice(voice);
    }

    // v1.3 CRITICAL: MPESynthesiser defaults shouldStealVoices to FALSE
    // (juce_MPESynthesiser.h:317). With it false, noteAdded() calls
    // findFreeVoice(note, false), which returns nullptr once all four voices are
    // busy and then SILENTLY DISCARDS the note-on — there is no fallback path.
    // Combined with voices that took minutes to free (see WaveguideString::
    // startRelease), the instrument went dead after four note-ons. Measured:
    // notes 5-8 of an eight-note probe produced no output at all.
    // Stealing must stay enabled even now that voices free properly — it is what
    // makes a dropped note structurally impossible rather than merely unlikely.
    synth.setVoiceStealingEnabled (true);

    // MPE legacy mode for non-MPE DAWs (RESEARCH §5 pitfall #8).
    // Pitchbend range 24 semitones, channels 1..16 — covers omni MIDI input.
    // CR-02: juce::Range is END-EXCLUSIVE. Range(1, 16) covers channels 1..15 only,
    // silently dropping channel 16 (MPEInstrument gates notes with
    // channelRange.contains()). JUCE's own default is Range(1, 17). Use 17.
    synth.enableLegacyMode(/*pitchbendRange*/ 24, juce::Range<int>(1, 17));

    // Phase 2.6b R40a — APVTS Listener registration for TUNING_SYSTEM Choice.
    parameters.addParameterListener ("TUNING_SYSTEM", this);

    // WR-02: resolve the master-chain parameter atomics once (avoids the per-block
    // O(log n) std::map walk in processBlock). Valid for the processor's lifetime.
    satAmountParam      = parameters.getRawParameterValue ("MASTER_SAT_AMOUNT");
    limiterCeilingParam = parameters.getRawParameterValue ("LIMITER_CEILING_DB");
    widthParam          = parameters.getRawParameterValue ("WIDTH");
    outputGainParam     = parameters.getRawParameterValue ("OUTPUT_GAIN");

    // Initial seed: apply the default/restored tuning mode synchronously. We are on
    // the message thread at construction time and no audio is rendering yet, so a
    // direct apply is safe — CR-03's RT-safe async path is only needed for runtime
    // audio-thread automation. Reuses handleAsyncUpdate()'s choice→Mode mapping and
    // guarantees the mode is set even where the message loop is not pumped (e.g. the
    // offline render harness).
    pendingTuningChoice.store (
        static_cast<int> (parameters.getRawParameterValue ("TUNING_SYSTEM")->load()));
    handleAsyncUpdate();

    // Stage 3 Task 8 (D6) — persist tuning-engine state (intervals, scale name,
    // tonic, octave stretch) through user presets AND DAW session state.
    // Without this the APVTS restores TUNING_SYSTEM=Scala on reload but the
    // engine silently reverts to 12-TET intervals (O-Wind/O-Lyrica pattern).
    // Tuning MODE itself needs no custom slot: it rides the TUNING_SYSTEM
    // APVTS choice → parameterChanged → handleAsyncUpdate dispatch.
    presetManager.setCustomStateCallbacks (
        // Save — tuning state as JSON
        [this]() -> juce::var
        {
            auto* obj = new juce::DynamicObject();

            juce::Array<juce::var> intervalsArray;
            for (double cents : tuningEngine.getIntervals())
                intervalsArray.add (cents);
            obj->setProperty ("intervals",     intervalsArray);
            obj->setProperty ("scaleName",     tuningEngine.getActiveTuningName());
            obj->setProperty ("tonic",         tuningEngine.getTonicNote());
            obj->setProperty ("octaveStretch", tuningEngine.getOctaveStretch());

            return juce::var (obj);
        },
        // Load — restore tuning state from JSON (message thread)
        [this] (const juce::var& customState)
        {
            auto* obj = customState.getDynamicObject();
            if (obj == nullptr)
                return;

            if (obj->hasProperty ("intervals"))
            {
                auto intervalsVar = obj->getProperty ("intervals");
                if (intervalsVar.isArray())
                {
                    std::vector<double> intervals;
                    for (int i = 0; i < intervalsVar.size(); ++i)
                        intervals.push_back (static_cast<double> (intervalsVar[i]));

                    if (! intervals.empty())
                    {
                        juce::String name = obj->getProperty ("scaleName").toString();
                        if (name.isEmpty()) name = "Custom";
                        tuningEngine.setCustomIntervals (intervals, name);
                    }
                }
            }

            // Tonic AFTER intervals so scale rotation applies correctly.
            if (obj->hasProperty ("tonic"))
                tuningEngine.setTonicNote (static_cast<int> (obj->getProperty ("tonic")));

            if (obj->hasProperty ("octaveStretch"))
                tuningEngine.setOctaveStretch (static_cast<float> (obj->getProperty ("octaveStretch")));
        });

    // ── Stage 4 (Polish) FUNC-04 — factory presets ────────────────────────────
    // Four 5-preset banks (Orchestral + Drone + v1.6 Expressive + v1.6 Texture),
    // authored in ENGINEERING UNITS and
    // converted once through each param's NormalisableRange (skew-safe). The 4
    // skewed params (BOW_SPEED 0.5, BOW_PRESSURE 0.5, BRIGHTNESS 0.25,
    // VIBRATO_ONSET 0.5) recall 4×–30× wrong if authored as raw normalized
    // fractions — convertTo0to1 handles skew + int/choice/bool uniformly
    // (pattern_factory_preset_normalized_ignores_skew). Omitted keys revert to
    // APVTS default on load (applyPresetJson resets all params first —
    // pattern_preset_apply_needs_reset_to_defaults), so STRING_TENSION is left
    // out entirely: it stays inert at default 0.5 (v1.1 deferral — authoring it
    // away from 0.5 would re-baseline the frozen render goldens if ever wired).
    // Drone presets carry explicit TUNING_SYSTEM(=2 12-TET) + NOTE_EXPRESSION(=1)
    // so the tuning-reset-to-defaults doesn't clobber intent; per-string pitch is
    // via the independent DETUNE_* params (they don't touch the Scala engine).
    // "Cinematic Bass Sustain" is first alphabetically → the default landing preset.
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        // ── Orchestral bank ───────────────────────────────────────────────────
        {
            "Cinematic Bass Sustain",
            {{"BOW_SPEED", 0.15f}, {"BOW_PRESSURE", 1.2f}, {"BOW_POSITION", 0.10f},
             {"BRIGHTNESS", 4500.0f}, {"OUTPUT_GAIN", 0.0f}, {"ROSIN", 0.65f},
             {"BOW_NOISE", 0.30f}, {"BODY_SIZE", 0.85f}, {"BODY_DAMPING", 0.30f},
             {"BODY_MIX", 0.85f}, {"STRING_STIFFNESS", 0.30f}, {"ACTIVE_STRINGS", 4.0f},
             {"VIBRATO_RATE", 5.0f}, {"VIBRATO_DEPTH", 8.0f}, {"VIBRATO_ONSET", 900.0f},
             {"SLOW_LFO_RATE", 0.30f}, {"SLOW_LFO_DEPTH", 0.10f}, {"WIDTH", 1.1f},
             {"MASTER_SAT_AMOUNT", 0.45f}, {"LIMITER_CEILING_DB", -0.3f}}
        },
        {
            "Section Bass",
            {{"BOW_SPEED", 0.18f}, {"BOW_PRESSURE", 1.4f}, {"BOW_POSITION", 0.11f},
             {"BRIGHTNESS", 3800.0f}, {"OUTPUT_GAIN", 0.0f}, {"ROSIN", 0.60f},
             {"BOW_NOISE", 0.28f}, {"BODY_SIZE", 0.80f}, {"BODY_DAMPING", 0.55f},
             {"BODY_MIX", 0.80f}, {"STRING_STIFFNESS", 0.30f}, {"ACTIVE_STRINGS", 4.0f},
             {"VIBRATO_RATE", 4.5f}, {"VIBRATO_DEPTH", 6.0f}, {"VIBRATO_ONSET", 800.0f},
             {"SLOW_LFO_RATE", 0.25f}, {"SLOW_LFO_DEPTH", 0.15f}, {"WIDTH", 1.5f},
             {"MASTER_SAT_AMOUNT", 0.40f}}
        },
        {
            "Solo Arco Bass",
            {{"BOW_SPEED", 0.20f}, {"BOW_PRESSURE", 1.0f}, {"BOW_POSITION", 0.09f},
             {"BRIGHTNESS", 5200.0f}, {"OUTPUT_GAIN", 0.5f}, {"ROSIN", 0.78f},
             {"BOW_NOISE", 0.45f}, {"BODY_SIZE", 0.75f}, {"BODY_DAMPING", 0.35f},
             {"BODY_MIX", 0.82f}, {"STRING_STIFFNESS", 0.28f}, {"ACTIVE_STRINGS", 4.0f},
             {"VIBRATO_RATE", 5.5f}, {"VIBRATO_DEPTH", 14.0f}, {"VIBRATO_ONSET", 500.0f},
             {"WIDTH", 1.0f}, {"MASTER_SAT_AMOUNT", 0.45f}}
        },
        {
            "Pianissimo Bass",
            {{"BOW_SPEED", 0.08f}, {"BOW_PRESSURE", 0.35f}, {"BOW_POSITION", 0.20f},
             {"BRIGHTNESS", 3000.0f}, {"OUTPUT_GAIN", -6.0f}, {"ROSIN", 0.50f},
             {"BOW_NOISE", 0.25f}, {"BODY_SIZE", 0.78f}, {"BODY_DAMPING", 0.45f},
             {"BODY_MIX", 0.80f}, {"STRING_STIFFNESS", 0.30f}, {"ACTIVE_STRINGS", 4.0f},
             {"VIBRATO_RATE", 4.5f}, {"VIBRATO_DEPTH", 5.0f}, {"VIBRATO_ONSET", 1100.0f},
             {"WIDTH", 1.0f}, {"MASTER_SAT_AMOUNT", 0.35f}}
        },
        {
            "Forte Bass",
            {{"BOW_SPEED", 0.35f}, {"BOW_PRESSURE", 3.2f}, {"BOW_POSITION", 0.07f},
             {"BRIGHTNESS", 6500.0f}, {"OUTPUT_GAIN", 0.0f}, {"ROSIN", 0.72f},
             {"BOW_NOISE", 0.40f}, {"BODY_SIZE", 0.82f}, {"BODY_DAMPING", 0.28f},
             {"BODY_MIX", 0.85f}, {"STRING_STIFFNESS", 0.32f}, {"ACTIVE_STRINGS", 4.0f},
             {"VIBRATO_RATE", 5.5f}, {"VIBRATO_DEPTH", 10.0f}, {"VIBRATO_ONSET", 400.0f},
             {"WIDTH", 1.2f}, {"MASTER_SAT_AMOUNT", 0.60f}}
        },
        // ── Drone bank (explicit TUNING_SYSTEM/NOTE_EXPRESSION per Decision 2) ──
        {
            "Infinite Drone",
            {{"BOW_SPEED", 0.12f}, {"BOW_PRESSURE", 1.5f}, {"BOW_POSITION", 0.10f},
             {"BRIGHTNESS", 3500.0f}, {"ROSIN", 0.60f}, {"BOW_NOISE", 0.30f},
             {"BODY_SIZE", 0.85f}, {"BODY_DAMPING", 0.30f}, {"BODY_MIX", 0.85f},
             {"STRING_STIFFNESS", 0.30f}, {"ACTIVE_STRINGS", 4.0f}, {"VIBRATO_DEPTH", 0.0f},
             {"VIBRATO_ONSET", 600.0f}, {"SLOW_LFO_RATE", 0.18f}, {"SLOW_LFO_DEPTH", 0.45f},
             {"INFINITE_SUSTAIN", 1.0f}, {"SUB_HARMONICS", 0.35f}, {"WIDTH", 1.4f},
             {"MASTER_SAT_AMOUNT", 0.50f}, {"TUNING_SYSTEM", 2.0f}, {"NOTE_EXPRESSION", 1.0f}}
        },
        {
            "Just-Intoned Drone",
            {{"BOW_SPEED", 0.12f}, {"BOW_PRESSURE", 1.4f}, {"BOW_POSITION", 0.10f},
             {"BRIGHTNESS", 3800.0f}, {"ROSIN", 0.60f}, {"BOW_NOISE", 0.28f},
             {"BODY_SIZE", 0.85f}, {"BODY_DAMPING", 0.32f}, {"BODY_MIX", 0.85f},
             {"ACTIVE_STRINGS", 4.0f}, {"DETUNE_E", 0.0f}, {"DETUNE_A", 204.0f},
             {"DETUNE_D", -14.0f}, {"DETUNE_G", 182.0f}, {"VIBRATO_DEPTH", 0.0f},
             {"SLOW_LFO_RATE", 0.20f}, {"SLOW_LFO_DEPTH", 0.30f}, {"INFINITE_SUSTAIN", 1.0f},
             {"SUB_HARMONICS", 0.25f}, {"WIDTH", 1.3f}, {"MASTER_SAT_AMOUNT", 0.50f},
             {"TUNING_SYSTEM", 2.0f}, {"NOTE_EXPRESSION", 1.0f}}
        },
        {
            "Scordatura Bass",
            {{"BOW_SPEED", 0.13f}, {"BOW_PRESSURE", 1.4f}, {"BOW_POSITION", 0.10f},
             {"BRIGHTNESS", 3600.0f}, {"ROSIN", 0.60f}, {"BOW_NOISE", 0.28f},
             {"BODY_SIZE", 0.85f}, {"BODY_DAMPING", 0.33f}, {"BODY_MIX", 0.85f},
             {"ACTIVE_STRINGS", 4.0f}, {"DETUNE_E", -400.0f}, {"DETUNE_A", -200.0f},
             {"DETUNE_D", 0.0f}, {"DETUNE_G", 200.0f}, {"VIBRATO_DEPTH", 0.0f},
             {"SLOW_LFO_RATE", 0.22f}, {"SLOW_LFO_DEPTH", 0.30f}, {"INFINITE_SUSTAIN", 1.0f},
             {"SUB_HARMONICS", 0.30f}, {"WIDTH", 1.3f}, {"MASTER_SAT_AMOUNT", 0.50f},
             {"TUNING_SYSTEM", 2.0f}, {"NOTE_EXPRESSION", 1.0f}}
        },
        {
            "Sub Drone",
            {{"BOW_SPEED", 0.10f}, {"BOW_PRESSURE", 1.6f}, {"BOW_POSITION", 0.10f},
             {"BRIGHTNESS", 1200.0f}, {"OUTPUT_GAIN", -1.0f}, {"ROSIN", 0.55f},
             {"BOW_NOISE", 0.22f}, {"BODY_SIZE", 0.90f}, {"BODY_DAMPING", 0.35f},
             {"BODY_MIX", 0.88f}, {"ACTIVE_STRINGS", 2.0f}, {"VIBRATO_DEPTH", 0.0f},
             {"SLOW_LFO_RATE", 0.15f}, {"SLOW_LFO_DEPTH", 0.35f}, {"INFINITE_SUSTAIN", 1.0f},
             {"SUB_HARMONICS", 0.75f}, {"WIDTH", 1.2f}, {"MASTER_SAT_AMOUNT", 0.55f},
             {"TUNING_SYSTEM", 2.0f}, {"NOTE_EXPRESSION", 1.0f}}
        },
        {
            "Dark Pad Bass",
            {{"BOW_SPEED", 0.09f}, {"BOW_PRESSURE", 0.9f}, {"BOW_POSITION", 0.15f},
             {"BRIGHTNESS", 2200.0f}, {"ROSIN", 0.50f}, {"BOW_NOISE", 0.20f},
             {"BODY_SIZE", 0.80f}, {"BODY_DAMPING", 0.65f}, {"BODY_MIX", 0.75f},
             {"ACTIVE_STRINGS", 4.0f}, {"VIBRATO_DEPTH", 0.0f}, {"VIBRATO_ONSET", 1500.0f},
             {"SLOW_LFO_RATE", 0.12f}, {"SLOW_LFO_DEPTH", 0.40f}, {"INFINITE_SUSTAIN", 0.85f},
             {"SUB_HARMONICS", 0.20f}, {"WIDTH", 1.5f}, {"MASTER_SAT_AMOUNT", 0.40f},
             {"TUNING_SYSTEM", 2.0f}, {"NOTE_EXPRESSION", 1.0f}}
        },
        // ── Expressive bank (v1.6.0 — bowing techniques) ──────────────────────
        // All names sort after "Cinematic Bass Sustain" so the default landing
        // preset is unchanged. STRING_TENSION stays omitted (v1.1 deferral).
        {
            "Sul Ponticello Bass",
            {{"BOW_SPEED", 0.22f}, {"BOW_PRESSURE", 1.8f}, {"BOW_POSITION", 0.03f},
             {"BRIGHTNESS", 8000.0f}, {"OUTPUT_GAIN", -1.0f}, {"ROSIN", 0.80f},
             {"BOW_NOISE", 0.55f}, {"BODY_SIZE", 0.75f}, {"BODY_DAMPING", 0.30f},
             {"BODY_MIX", 0.75f}, {"STRING_STIFFNESS", 0.34f}, {"ACTIVE_STRINGS", 4.0f},
             {"VIBRATO_RATE", 5.0f}, {"VIBRATO_DEPTH", 6.0f}, {"VIBRATO_ONSET", 700.0f},
             {"WIDTH", 1.1f}, {"MASTER_SAT_AMOUNT", 0.50f}}
        },
        {
            "Sul Tasto Bass",
            {{"BOW_SPEED", 0.12f}, {"BOW_PRESSURE", 0.8f}, {"BOW_POSITION", 0.24f},
             {"BRIGHTNESS", 2600.0f}, {"OUTPUT_GAIN", 0.0f}, {"ROSIN", 0.55f},
             {"BOW_NOISE", 0.22f}, {"BODY_SIZE", 0.80f}, {"BODY_DAMPING", 0.50f},
             {"BODY_MIX", 0.82f}, {"STRING_STIFFNESS", 0.28f}, {"ACTIVE_STRINGS", 4.0f},
             {"VIBRATO_RATE", 4.2f}, {"VIBRATO_DEPTH", 7.0f}, {"VIBRATO_ONSET", 900.0f},
             {"WIDTH", 1.0f}, {"MASTER_SAT_AMOUNT", 0.35f}}
        },
        {
            "Overpressure Scratch",
            {{"BOW_SPEED", 0.06f}, {"BOW_PRESSURE", 5.5f}, {"BOW_POSITION", 0.06f},
             {"BRIGHTNESS", 5500.0f}, {"OUTPUT_GAIN", -2.0f}, {"ROSIN", 0.90f},
             {"BOW_NOISE", 0.70f}, {"BODY_SIZE", 0.78f}, {"BODY_DAMPING", 0.25f},
             {"BODY_MIX", 0.78f}, {"STRING_STIFFNESS", 0.35f}, {"ACTIVE_STRINGS", 4.0f},
             {"VIBRATO_DEPTH", 0.0f}, {"RELEASE", 0.6f}, {"WIDTH", 1.0f},
             {"MASTER_SAT_AMOUNT", 0.65f}}
        },
        {
            "Flautando Bass",
            {{"BOW_SPEED", 0.30f}, {"BOW_PRESSURE", 0.25f}, {"BOW_POSITION", 0.22f},
             {"BRIGHTNESS", 3200.0f}, {"OUTPUT_GAIN", 1.0f}, {"ROSIN", 0.45f},
             {"BOW_NOISE", 0.15f}, {"BODY_SIZE", 0.80f}, {"BODY_DAMPING", 0.45f},
             {"BODY_MIX", 0.80f}, {"STRING_STIFFNESS", 0.26f}, {"ACTIVE_STRINGS", 4.0f},
             {"VIBRATO_RATE", 4.8f}, {"VIBRATO_DEPTH", 4.0f}, {"VIBRATO_ONSET", 1000.0f},
             {"WIDTH", 1.2f}, {"MASTER_SAT_AMOUNT", 0.30f}}
        },
        {
            "Espressivo Bass",
            {{"BOW_SPEED", 0.24f}, {"BOW_PRESSURE", 1.6f}, {"BOW_POSITION", 0.08f},
             {"BRIGHTNESS", 5000.0f}, {"OUTPUT_GAIN", 0.5f}, {"ROSIN", 0.75f},
             {"BOW_NOISE", 0.40f}, {"BODY_SIZE", 0.78f}, {"BODY_DAMPING", 0.32f},
             {"BODY_MIX", 0.84f}, {"STRING_STIFFNESS", 0.30f}, {"ACTIVE_STRINGS", 4.0f},
             {"VIBRATO_RATE", 6.0f}, {"VIBRATO_DEPTH", 22.0f}, {"VIBRATO_ONSET", 250.0f},
             {"WIDTH", 1.1f}, {"MASTER_SAT_AMOUNT", 0.45f}}
        },
        // ── Texture bank (v1.6.0 — sound design; explicit TUNING_SYSTEM(=2
        // 12-TET) + NOTE_EXPRESSION(=1) like the Drone bank so the reset-to-
        // defaults pass doesn't clobber intent) ───────────────────────────────
        {
            "Glass Drone",
            {{"BOW_SPEED", 0.16f}, {"BOW_PRESSURE", 0.6f}, {"BOW_POSITION", 0.04f},
             {"BRIGHTNESS", 9000.0f}, {"OUTPUT_GAIN", -1.5f}, {"ROSIN", 0.70f},
             {"BOW_NOISE", 0.35f}, {"BODY_SIZE", 0.70f}, {"BODY_DAMPING", 0.25f},
             {"BODY_MIX", 0.72f}, {"ACTIVE_STRINGS", 4.0f}, {"VIBRATO_DEPTH", 0.0f},
             {"SLOW_LFO_RATE", 0.10f}, {"SLOW_LFO_DEPTH", 0.35f}, {"INFINITE_SUSTAIN", 1.0f},
             {"SUB_HARMONICS", 0.10f}, {"WIDTH", 1.6f}, {"MASTER_SAT_AMOUNT", 0.40f},
             {"TUNING_SYSTEM", 2.0f}, {"NOTE_EXPRESSION", 1.0f}}
        },
        {
            "Quarter-Tone Drone",
            {{"BOW_SPEED", 0.12f}, {"BOW_PRESSURE", 1.4f}, {"BOW_POSITION", 0.10f},
             {"BRIGHTNESS", 3600.0f}, {"ROSIN", 0.60f}, {"BOW_NOISE", 0.28f},
             {"BODY_SIZE", 0.85f}, {"BODY_DAMPING", 0.32f}, {"BODY_MIX", 0.85f},
             {"ACTIVE_STRINGS", 4.0f}, {"DETUNE_E", 0.0f}, {"DETUNE_A", 50.0f},
             {"DETUNE_D", -50.0f}, {"DETUNE_G", 50.0f}, {"VIBRATO_DEPTH", 0.0f},
             {"SLOW_LFO_RATE", 0.20f}, {"SLOW_LFO_DEPTH", 0.30f}, {"INFINITE_SUSTAIN", 1.0f},
             {"SUB_HARMONICS", 0.25f}, {"WIDTH", 1.3f}, {"MASTER_SAT_AMOUNT", 0.50f},
             {"TUNING_SYSTEM", 2.0f}, {"NOTE_EXPRESSION", 1.0f}}
        },
        {
            "Tectonic Sub",
            {{"BOW_SPEED", 0.09f}, {"BOW_PRESSURE", 1.8f}, {"BOW_POSITION", 0.11f},
             {"BRIGHTNESS", 900.0f}, {"OUTPUT_GAIN", -1.0f}, {"ROSIN", 0.55f},
             {"BOW_NOISE", 0.18f}, {"BODY_SIZE", 0.95f}, {"BODY_DAMPING", 0.35f},
             {"BODY_MIX", 0.90f}, {"ACTIVE_STRINGS", 2.0f}, {"VIBRATO_DEPTH", 0.0f},
             {"SLOW_LFO_RATE", 0.08f}, {"SLOW_LFO_DEPTH", 0.50f}, {"INFINITE_SUSTAIN", 1.0f},
             {"SUB_HARMONICS", 0.85f}, {"WIDTH", 1.0f}, {"MASTER_SAT_AMOUNT", 0.60f},
             {"TUNING_SYSTEM", 2.0f}, {"NOTE_EXPRESSION", 1.0f}}
        },
        {
            "Warped Tape Bass",
            {{"BOW_SPEED", 0.13f}, {"BOW_PRESSURE", 1.3f}, {"BOW_POSITION", 0.12f},
             {"BRIGHTNESS", 2800.0f}, {"OUTPUT_GAIN", 0.0f}, {"ROSIN", 0.58f},
             {"BOW_NOISE", 0.25f}, {"BODY_SIZE", 0.82f}, {"BODY_DAMPING", 0.40f},
             {"BODY_MIX", 0.80f}, {"ACTIVE_STRINGS", 4.0f}, {"DETUNE_E", -8.0f},
             {"DETUNE_A", 6.0f}, {"DETUNE_D", -5.0f}, {"DETUNE_G", 8.0f},
             {"VIBRATO_DEPTH", 0.0f}, {"SLOW_LFO_RATE", 0.60f}, {"SLOW_LFO_DEPTH", 0.60f},
             {"INFINITE_SUSTAIN", 0.90f}, {"SUB_HARMONICS", 0.15f}, {"WIDTH", 1.4f},
             {"MASTER_SAT_AMOUNT", 0.75f}, {"TUNING_SYSTEM", 2.0f}, {"NOTE_EXPRESSION", 1.0f}}
        },
        {
            "Whisper Pad",
            {{"BOW_SPEED", 0.07f}, {"BOW_PRESSURE", 0.30f}, {"BOW_POSITION", 0.18f},
             {"BRIGHTNESS", 2000.0f}, {"OUTPUT_GAIN", -4.0f}, {"ROSIN", 0.45f},
             {"BOW_NOISE", 0.50f}, {"BODY_SIZE", 0.85f}, {"BODY_DAMPING", 0.60f},
             {"BODY_MIX", 0.80f}, {"ACTIVE_STRINGS", 4.0f}, {"VIBRATO_DEPTH", 0.0f},
             {"SLOW_LFO_RATE", 0.12f}, {"SLOW_LFO_DEPTH", 0.30f}, {"INFINITE_SUSTAIN", 1.0f},
             {"SUB_HARMONICS", 0.10f}, {"RELEASE", 8.0f}, {"WIDTH", 1.7f},
             {"MASTER_SAT_AMOUNT", 0.35f}, {"TUNING_SYSTEM", 2.0f}, {"NOTE_EXPRESSION", 1.0f}}
        }
    };

    // Skew-safe: convert every engineering-unit value to normalized 0..1 through
    // its NormalisableRange right before seeding. initializeFactoryPresets stores
    // the normalized value verbatim; applyPresetJson feeds it back via
    // convertFrom0to1 on load. Only re-seeds when JucePlugin_VersionString changes.
    for (auto& preset : factoryPresets)
        for (auto& [id, value] : preset.parameters)
            if (auto* p = parameters.getParameter (id))
                value = p->convertTo0to1 (value);

    presetManager.initializeFactoryPresets (factoryPresets);
}

OContrabassAudioProcessor::~OContrabassAudioProcessor()
{
    parameters.removeParameterListener ("TUNING_SYSTEM", this);
    // WR-01: drain any queued tuning-mode dispatch before members are destroyed —
    // handleAsyncUpdate() dereferences tuningEngine, so a pending update outliving
    // the processor would be a use-after-free.
    cancelPendingUpdate();
}

//==============================================================================
bool OContrabassAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void OContrabassAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<BowedContrabassVoice*>(synth.getVoice(i)))
            v->prepareToPlay(sampleRate, samplesPerBlock);

    // Phase 2.6a — master output chain prepare.
    masterSaturator.prepare(sampleRate);
    masterLimiter.prepare(sampleRate);
    stereoWidth.prepare(sampleRate, samplesPerBlock);
    outputGainSmoothed.reset(sampleRate, 0.030);
    outputGainSmoothed.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(parameters.getRawParameterValue("OUTPUT_GAIN")->load()));

   #if defined (OCBS_DISABLE_DECORRELATOR)
    // Phase 2.6a-bis Risk #22 — bit-equivalence build path. Force APVTS to
    // bypass values (sat=0 / limiter=0 dB / width=1.0) and seed master-chain
    // smoothers immediately so default-state output matches Phase 2.5 sha256s
    // byte-for-byte (output chain becomes a mathematical no-op when
    // decorrelator is off at compile time AND user-set bypass params are
    // active). Test-only build path; production builds compile this block
    // out entirely and preserve the post-Phase-2.6a R39 audible-golden bar.
    if (auto* p = parameters.getParameter("MASTER_SAT_AMOUNT"))
        p->setValueNotifyingHost(0.0f);                  // norm 0 → raw 0.0
    if (auto* p = parameters.getParameter("LIMITER_CEILING_DB"))
        p->setValueNotifyingHost(1.0f);                  // norm 1.0 → raw 0.0 dB
    if (auto* p = parameters.getParameter("WIDTH"))
        p->setValueNotifyingHost(0.5f);                  // norm 0.5 → raw 1.0
    masterSaturator.setAmountImmediate(0.0f);
    masterLimiter.setCeilingDbImmediate(0.0f);
   #endif

    // Report oversampler latency to host (RESEARCH §3.1; CLAUDE.md memory:
    // getLatencySamples() is non-virtual in JUCE 8 — never override; always use
    // setLatencySamples in prepareToPlay).
    if (auto* v = dynamic_cast<BowedContrabassVoice*>(synth.getVoice(0)))
        setLatencySamples(static_cast<int>(std::ceil(v->getOversamplingLatency())));
    else
        setLatencySamples(0);
}

void OContrabassAudioProcessor::releaseResources()
{
    // Phase 2.6a — master output chain reset.
    masterSaturator.reset();
    masterLimiter.reset();
    stereoWidth.reset();
    outputGainSmoothed.reset(1.0f);
}

void OContrabassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Synth pattern: clear any stray output channels not backed by inputs.
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // MPESynthesiser additively writes into outputBuffer — clear first so we
    // don't accumulate prior block content (synth.addVoice path uses addSample).
    buffer.clear();

    // Phase 2.6c R41a — HR-13: drain the VST3 Note Expression raw-event queue
    // EXACTLY ONCE per processBlock, BEFORE renderNextBlock, so pending
    // per-note tuning deltas are staged in the table before any voice's
    // noteStarted consumes them this block. Alloc-free in steady state per the
    // module contract (§24.2.2: vectors reserved 64 at ctor; only blocks that
    // actually carry VST3 NE events may allocate in the correlation map —
    // never in AU/Standalone/harness, where the dispatch slots stay nullptr
    // and this degrades to a ~ns no-op).
    vst3Extensions.drainAndUpdate();

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // ─── Phase 2.6a — Master Output Chain ──────────────────────────────────
    // Voice writes mono L=R clone (no per-voice OUTPUT_GAIN applied).
    // Chain: Saturator → StereoWidth → Limiter → OUTPUT_GAIN.
    // WR-04: StereoWidth moved BEFORE the limiter so the limiter is the last
    // dynamics stage and its ceiling genuinely bounds the widened stereo output.
    // (Parameter atomics are cached members per WR-02 — no per-block map walk.)

    // Step 10: Master Saturator (polynomial x − x³/3, wet/dry mix).
    masterSaturator.setAmount(satAmountParam->load());
    masterSaturator.processBlock(buffer);

    // Step 11: Stereo width (allpass decorrelator + M/S width). WR-04: runs BEFORE
    // the limiter — with width>1 the M/S side gain has no bound and, when it ran
    // after the limiter, pushed peaks back above the ceiling the limiter had just
    // enforced (e.g. +2·ceiling ≈ +5.7 dBFS at width=2, −0.3 dBFS ceiling), voiding
    // the ceiling guarantee and hard-clipping downstream.
    stereoWidth.setWidth(widthParam->load());
    stereoWidth.processBlock(buffer);

    // Step 12: Zero-latency feedforward limiter (3 ms attack / 50 ms release) —
    // now the final ceiling enforcer before output gain.
    masterLimiter.setCeilingDb(limiterCeilingParam->load());
    masterLimiter.processBlock(buffer);

    // Step 13: Output Gain (relocated from voice-side; ARCHITECTURE §258 final stage).
    const float gainTarget = juce::Decibels::decibelsToGain(outputGainParam->load());
    outputGainSmoothed.setTargetValue(gainTarget);

    {
        const int numSamples = buffer.getNumSamples();
        const int numChans   = buffer.getNumChannels();
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = outputGainSmoothed.getNextValue();
            for (int ch = 0; ch < numChans; ++ch)
                buffer.getWritePointer(ch)[i] *= g;
        }
    }

    // Stage 3 Task 12 — VU feed: post-limiter/post-gain RMS (dB), READ-ONLY tap
    // AFTER the output-gain loop (the true final signal). Relaxed store; the
    // editor timer polls at 30 Hz. Never inserts/reorders signal-path
    // arithmetic — goldens-safe by construction.
    {
        const int numSamples = buffer.getNumSamples();
        const int numChans   = buffer.getNumChannels();
        if (numSamples > 0 && numChans > 0)
        {
            double sumSq = 0.0;
            for (int ch = 0; ch < numChans; ++ch)
            {
                const float* r = buffer.getReadPointer (ch);
                for (int i = 0; i < numSamples; ++i)
                    sumSq += static_cast<double> (r[i]) * static_cast<double> (r[i]);
            }
            const float rms = std::sqrt (static_cast<float> (
                sumSq / static_cast<double> (numSamples * numChans)));
            outputRmsDb.store (juce::Decibels::gainToDecibels (rms, -80.0f),
                               std::memory_order_relaxed);
        }
    }
}

//==============================================================================
// Stage 3 Task 8 (D6) — session state routed through OuariconPresetManager so
// the tuning custom state (intervals/tonic/stretch) survives DAW reload.
// Backward compatible: getStateAsXml() wraps the SAME APVTS XML root (extra
// "CustomState" child + "currentPreset" attribute), and setStateFromXml()
// accepts plain pre-Stage-3 APVTS XML unchanged.
void OContrabassAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
    {
        // v1.7.0 — hover-help preference rides the session as a root XML
        // attribute, NOT a ValueTree property: the ValueTree XML round-trip
        // rebuilds properties as strings, so an isBool() guard on restore
        // would never fire (critical_valuetree_xml_roundtrip_loses_type).
        // getBoolAttribute below sidesteps that class of bug entirely.
        xml->setAttribute("tooltipsEnabled",
                          tooltipsEnabled.load(std::memory_order_acquire));

        // v1.8.0 — the interface LANGUAGE rides the same root, beside the
        // toggle it belongs with, and as a root XML attribute for the same
        // reason. Written as a STRING ("en"/"fr") rather than the atomic's int
        // index, so a hand-inspected session file says what it means.
        xml->setAttribute("uiLanguage",
                          languageCode(uiLanguage.load(std::memory_order_acquire)));
        copyXmlToBinary(*xml, destData);
    }
}

void OContrabassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        // Pre-1.7.0 sessions have no attribute — the default (OFF) stands.
        // The editor PULLS this via the getTooltipsEnabled native fn at page
        // init rather than being pushed — a push from here would race the
        // WebView's load (the O-FreqPulse WR-01 bug).
        if (xml->hasAttribute("tooltipsEnabled"))
            tooltipsEnabled.store(xml->getBoolAttribute("tooltipsEnabled"),
                                  std::memory_order_release);

        // v1.8.0 — same treatment. Pre-1.8.0 sessions have no attribute and the
        // default (English) stands. languageIndex() clamps anything that is not
        // "fr" to 0, so a hand-edited value degrades to English rather than to a
        // bad index. Also PULLED by the page at init, never pushed.
        if (xml->hasAttribute("uiLanguage"))
            uiLanguage.store(languageIndex(xml->getStringAttribute("uiLanguage")),
                             std::memory_order_release);

        presetManager.setStateFromXml(xml.get());
    }
}

//==============================================================================
juce::AudioProcessorEditor* OContrabassAudioProcessor::createEditor()
{
   #if JUCE_WEB_BROWSER
    return new OContrabassAudioProcessorEditor(*this);
   #else
    return nullptr;   // render-harness build (JUCE_WEB_BROWSER=0) — headless
   #endif
}

//==============================================================================
// Phase 2.3 R29 — harness instrumentation accessor (lastSafeDepth read).
BowedContrabassVoice* OContrabassAudioProcessor::getActiveVoice() noexcept
{
    if (synth.getNumVoices() == 0)
        return nullptr;
    return dynamic_cast<BowedContrabassVoice*> (synth.getVoice(0));
}

//==============================================================================
// Stage 3 Task 13 (D5) — most-recently-started active voice's effective bow
// state (post-LFO/macro/MPE). All reads are relaxed atomics published by the
// voices (single-writer audio thread); called from the editor timer @30 Hz.
OContrabassAudioProcessor::BowStateViz OContrabassAudioProcessor::getBowStateViz() noexcept
{
    BowStateViz out { 0.0f, 0.0f, 0.10f, false };
    juce::uint32 bestOrdinal = 0;

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* v = dynamic_cast<BowedContrabassVoice*> (synth.getVoice(i)))
        {
            if (! v->getVizActive())
                continue;

            const auto ord = v->getVizStartOrdinal();
            if (! out.active || ord >= bestOrdinal)
            {
                bestOrdinal  = ord;
                out.active   = true;
                out.speed    = v->getVizBowSpeed();
                out.pressure = v->getVizBowPressure();
                out.beta     = v->getVizBowBeta();
            }
        }
    }
    return out;
}

//==============================================================================
// Phase 2.6b R40a — APVTS Listener: TUNING_SYSTEM Choice → TuningEngine::Mode.
// ESCALATION-MTS1 LOCK: setMode briefly holds intervalMutex inside
// rebuildFrequencyTable — not RT-safe. CR-03: parameterChanged fires SYNCHRONOUSLY
// on the setter's thread, which is the AUDIO thread when a host automates
// TUNING_SYSTEM. The prior MessageManager::callAsync constructed a std::function +
// new'd a message → audio-thread heap allocation (the exact class pluginval-10
// "Parameter thread safety" fails on), and captured `this` with no lifetime guard
// (WR-01 teardown UAF). Instead: store the choice atomically and triggerAsyncUpdate()
// — RT-safe (reuses a preallocated message) — and apply setMode on the message thread
// in handleAsyncUpdate(); the destructor cancelPendingUpdate()s to close the UAF.
void OContrabassAudioProcessor::parameterChanged (const juce::String& parameterID,
                                                  float newValue)
{
    if (parameterID != "TUNING_SYSTEM")
        return;

    pendingTuningChoice.store (static_cast<int> (newValue));
    triggerAsyncUpdate();
}

// CR-03 / WR-01 — message-thread apply of the staged tuning mode. Choice index →
// module Mode swap (plugin index 0 "Scala/TUN" = Mode::Scala; index 1 "MTS-ESP" =
// Mode::MTSESP; index 2 "12-TET" = Mode::TwelveTET). Also invoked directly from the
// constructor for the synchronous initial seed (message thread, pre-render).
void OContrabassAudioProcessor::handleAsyncUpdate()
{
    TuningEngine::Mode mode;
    switch (pendingTuningChoice.load())
    {
        case 0:  mode = TuningEngine::Mode::Scala;     break;
        case 1:  mode = TuningEngine::Mode::MTSESP;    break;
        case 2:
        default: mode = TuningEngine::Mode::TwelveTET; break;
    }
    tuningEngine.setMode (mode);
}

//==============================================================================
// Phase 2.6b R40a — Scala/TUN file-load entry point (ESCALATION-FPK1).
// Public method invoked from the harness `--microtonal --scl <path>` handler
// on the main thread before render begins; Stage 3 GUI Editor replaces this
// invocation with juce::FileChooser::launchAsync + AsyncUpdater callback.
// Module's loadScalaFile briefly holds intervalMutex during
// setCustomIntervals → rebuildFrequencyTable; main-thread invocation is the
// design contract.
bool OContrabassAudioProcessor::loadScalaFile (const juce::File& sclFile)
{
    return tuningEngine.loadScalaFile (sclFile);
}

//==============================================================================
// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OContrabassAudioProcessor();
}
