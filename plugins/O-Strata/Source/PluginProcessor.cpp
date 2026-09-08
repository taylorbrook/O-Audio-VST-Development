/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    PluginProcessor.cpp
    O-Strata - Microtonal Wavetable Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
// PluginEditor.h is deliberately NOT included here — the include lives inside
// the #if JUCE_WEB_BROWSER guard above createEditor() so a console target that
// compiles this TU with JUCE_WEB_BROWSER=0 and no editor sources (the param-dump
// tool, the render harnesses) still builds. A top-of-file include breaks the
// moment the editor references WebView types
// (pattern_render_harness_breaks_on_webview_editor).
#include "FactoryPresets.h"
#include "NoteDivisions.h"
#include "dsp/ModulationMatrix.h"
#include "dsp/WavetableOscillator.h"

// ═══════════════════════════════════════════════════════════════════
// FX bypass-and-process helper (used by processBlock)
// ═══════════════════════════════════════════════════════════════════

namespace
{
    // Runs an FX block: short-circuits when the bypass param is on, otherwise
    // hands the FX + AudioBlock to the supplied configure lambda. The lambda
    // is responsible for setting parameters and (when not also short-circuited
    // by mix≈0) calling fx.process(block) — the per-FX `mix > 0.001f` gate
    // is preserved inside the lambda because not every FX `process()` is
    // RT-safe at mix=0.
    // Returns whether the effect actually processed this block, so the caller
    // can reset stale FX buffers on the active -> inactive transition (WR-07).
    template <typename FX, typename ConfigureFn>
    inline bool runEffect (std::atomic<float>* pBypass, FX& fx,
                           juce::dsp::AudioBlock<float>& block,
                           ConfigureFn&& configure)
    {
        if (pBypass->load() > 0.5f)
            return false;
        return configure (fx, block);
    }

}

// ═══════════════════════════════════════════════════════════════════
// Parameter Helper Functions
// ═══════════════════════════════════════════════════════════════════

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createOscParameters (const juce::String& prefix)
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto label = "Osc " + juce::String::charToString (prefix.getLastCharacter());
    float levelDefault = (prefix == "oscA") ? 0.8f : 0.0f;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Pos", 1 }, label + " Position",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Level", 1 }, label + " Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), levelDefault));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Pan", 1 }, label + " Pan",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "Coarse", 1 }, label + " Coarse", -24, 24, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Fine", 1 }, label + " Fine",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Phase", 1 }, label + " Phase",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "Unison", 1 }, label + " Unison", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Detune", 1 }, label + " Detune",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Width", 1 }, label + " Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "WarpType", 1 }, label + " Warp Type",
        juce::StringArray { "Off", "Sync", "Bend", "FM", "Window" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "WarpAmt", 1 }, label + " Warp Amount",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    // ─── Geometry (Stage 1: inert bake parameters, parameter-spec.md v1) ───
    // Plain APVTS parameters read by Phase 2.1's message-thread bake scheduler
    // at bake time; deliberately NOT mod-matrix destinations and NOT cached as
    // raw-value pointers. Host names are family-prefixed so the 24 lanes per
    // oscillator read unambiguously (spec locks IDs/ranges/defaults/choices,
    // not the host name). Non-ASCII choice strings (U+2026, U+00B7) are built
    // through CharPointer_UTF8 with hex escapes so the source stays ASCII.
    const juce::String imported  (juce::CharPointer_UTF8 ("Imported\xE2\x80\xA6"));
    const juce::String epitroch  (juce::CharPointer_UTF8 ("Epitrochoid 3\xC2\xB7" "5\xC2\xB7" "7"));
    const juce::String hypocycl  (juce::CharPointer_UTF8 ("Hypocycloid 3\xC2\xB7" "5\xC2\xB7" "7"));

    auto choice = [&] (const char* suffix, const char* name, juce::StringArray choices, int def)
    {
        jassert (choices.size() >= 2);   // a one-entry choice list births NaN (critical_choice_param_needs_two_choices)
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { prefix + suffix, 1 }, label + name, std::move (choices), def));
    };
    auto knob = [&] (const char* suffix, const char* name, float lo, float hi, float step, float def)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + suffix, 1 }, label + name,
            juce::NormalisableRange<float> (lo, hi, step), def));
    };

    // Source family
    choice ("GeoSource",     " Source",              { "Mesh", "Volume", "Terrain" }, 0);
    choice ("GeoFrames",     " Frames",              { "64", "128", "256" }, 2);
    knob   ("GeoDrive",      " Shape Drive",         0.0f, 1.0f, 0.001f, 0.0f);
    // Mesh family
    choice ("Mesh",          " Mesh",                { "Sphere", "Torus", "Twisted Star", "Crescent",
                                                       "Torus Knot", "fBm Blob", imported }, 1);
    knob   ("MeshTiltX",     " Mesh Tilt X",         -90.0f, 90.0f, 0.1f, 0.0f);
    knob   ("MeshTiltY",     " Mesh Tilt Y",         -90.0f, 90.0f, 0.1f, 0.0f);
    knob   ("MeshPhi",       " Mesh Projection Phi", 0.0f, 360.0f, 0.1f, 0.0f);
    choice ("MeshUnwrap",    " Mesh Unwrap Mode",    { "XY Projection", "Centroid Distance" }, 0);
    choice ("MeshLoop",      " Mesh Loop Policy",    { "Largest", "Sum" }, 0);
    // Volume family
    choice ("VolField",      " Vol Field",           { "Torus SDF", "Box SDF", "Gyroid", "Sphere SDF", "fBm Noise" }, 0);
    choice ("VolOrbit",      " Vol Orbit",           { "Torus Knot (2,3)", "Torus Knot (3,5)",
                                                       "Torus Knot (5,7)", "Lissajous Knot" }, 0);
    choice ("VolSweepAxis",  " Vol Sweep Axis",      { "Orbit Scale", "Knot Phase", "Z Offset" }, 0);
    knob   ("VolSweepRange", " Vol Sweep Range",     0.0f, 1.0f, 0.001f, 0.6f);
    knob   ("VolDetail",     " Vol Field Detail",    0.0f, 1.0f, 0.001f, 0.3f);
    // Terrain family
    choice ("Terrain",       " Terrain",             { "Sine Product", "Radial Rings", imported }, 0);
    choice ("TerOrbit",      " Ter Orbit Shape",     { "Ellipse", epitroch, hypocycl, "Superellipse" }, 0);
    knob   ("TerCX",         " Ter Centre X",        -1.0f, 1.0f, 0.01f, 0.0f);
    knob   ("TerCY",         " Ter Centre Y",        -1.0f, 1.0f, 0.01f, 0.0f);
    knob   ("TerAspect",     " Ter Aspect",          0.1f, 1.0f, 0.001f, 1.0f);
    knob   ("TerRot",        " Ter Rotation",        0.0f, 360.0f, 0.1f, 0.0f);
    choice ("TerSweepAxis",  " Ter Sweep Axis",      { "Radius", "Rotation", "Centre X", "Centre Y" }, 0);
    knob   ("TerSweepRange", " Ter Sweep Range",     0.0f, 1.0f, 0.001f, 0.8f);
    knob   ("TerBlur",       " Ter Image Blur",      0.0f, 1.0f, 0.001f, 0.2f);
    choice ("TerEdge",       " Ter Edge Mode",       { "Mirror", "Window" }, 0);

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createSubNoiseParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "subShape", 1 }, "Sub Shape",
        juce::StringArray { "Sine", "Triangle", "Saw", "Square" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "subOctave", 1 }, "Sub Octave",
        juce::StringArray { "-1 Oct", "-2 Oct", "-3 Oct", "-4 Oct" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "subLevel", 1 }, "Sub Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "noiseType", 1 }, "Noise Type",
        juce::StringArray { "White", "Pink", "Brown", "Digital", "Vinyl", "Wind" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noiseLevel", 1 }, "Noise Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "subRouting", 1 }, "Sub Routing",
        juce::StringArray { "Post-Filter", "Pre-Filter" }, 0));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createAmpEnvelopeParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampAttack", 1 }, "Amp Attack",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.35f), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampDecay", 1 }, "Amp Decay",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.35f), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampSustain", 1 }, "Amp Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampRelease", 1 }, "Amp Release",
        juce::NormalisableRange<float> (0.001f, 20.0f, 0.001f, 0.3f), 0.5f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createFilterEnvelopeParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtAttack", 1 }, "Filter Attack",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.35f), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtDecay", 1 }, "Filter Decay",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.35f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtSustain", 1 }, "Filter Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtRelease", 1 }, "Filter Release",
        juce::NormalisableRange<float> (0.001f, 20.0f, 0.001f, 0.3f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtAEnvDepth", 1 }, "Filter A Env Depth",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtBEnvDepth", 1 }, "Filter B Env Depth",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createFilterParameters (const juce::String& prefix)
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto label = "Filter " + juce::String::charToString (prefix.getLastCharacter());

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "Type", 1 }, label + " Type",
        juce::StringArray { "LP12", "LP24", "HP12", "HP24", "BP12", "BP24", "Notch" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Cutoff", 1 }, label + " Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 20000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Res", 1 }, label + " Resonance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Drive", 1 }, label + " Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "KeyTrack", 1 }, label + " KeyTrack",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createFilterRoutingParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filtRouting", 1 }, "Filter Routing",
        juce::StringArray { "Serial", "Parallel" }, 0));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createTuningParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "tuningPreset", 1 }, "Tuning Preset",
        juce::StringArray { "12-TET", "Pythagorean", "Zarlino", "Meantone 1/4",
                            "Werckmeister III", "Kirnberger III", "Vallotti",
                            "Well Tempered", "Just Intonation", "Bohlen-Pierce", "Custom" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "tonic", 1 }, "Tonic",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "masterTune", 1 }, "Master Tune",
        juce::NormalisableRange<float> (420.0f, 460.0f, 0.1f), 440.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "octaveStretch", 1 }, "Octave Stretch",
        juce::NormalisableRange<float> (0.95f, 1.25f, 0.001f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "pitchBendRange", 1 }, "Pitch Bend Range", 1, 48, 2));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "glideMode", 1 }, "Glide Mode",
        juce::StringArray { "Off", "Legato", "Always" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "glideTime", 1 }, "Glide Time",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.35f), 0.1f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createReverbParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "reverbBypass", 1 }, "Reverb Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbSize", 1 }, "Reverb Size",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbDamp", 1 }, "Reverb Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbPredelay", 1 }, "Reverb Pre-delay",
        juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f, 0.5f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbMix", 1 }, "Reverb Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbModDepth", 1 }, "Reverb Mod Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbModRate", 1 }, "Reverb Mod Rate",
        juce::NormalisableRange<float> (0.1f, 5.0f, 0.01f, 0.5f), 1.0f));

    return params;
}

static const juce::StringArray& getLfoDivisionNames();

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createDelayParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "delayBypass", 1 }, "Delay Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayTime", 1 }, "Delay Time",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.35f), 0.375f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayFeedback", 1 }, "Delay Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "delaySync", 1 }, "Delay Sync", false));
    // WR-03: previously referenced by ~20 factory presets but never registered
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "delayDivision", 1 }, "Delay Division",
        getLfoDivisionNames(), 2)); // default 1/4
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "delayMode", 1 }, "Delay Mode",
        juce::StringArray { "Normal", "PingPong" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayMix", 1 }, "Delay Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createChorusParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "chorusBypass", 1 }, "Chorus Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusRate", 1 }, "Chorus Rate",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.4f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusDepth", 1 }, "Chorus Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusMix", 1 }, "Chorus Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createDistortionParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "distBypass", 1 }, "Distortion Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "distType", 1 }, "Distortion Type",
        juce::StringArray { "SoftClip", "HardClip", "Tube", "Fold" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "distDrive", 1 }, "Distortion Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "distMix", 1 }, "Distortion Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createEQParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "eqBypass", 1 }, "EQ Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqLowGain", 1 }, "EQ Low Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqMidGain", 1 }, "EQ Mid Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqMidFreq", 1 }, "EQ Mid Freq",
        juce::NormalisableRange<float> (200.0f, 8000.0f, 0.1f, 0.35f), 1000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqHighGain", 1 }, "EQ High Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    return params;
}

static const juce::StringArray& getLfoDivisionNames()
{
    static const juce::StringArray names {
        "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
        "1/1D", "1/2D", "1/4D", "1/8D", "1/16D", "1/32D",
        "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"
    };
    return names;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createLFOParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const auto& divNames = getLfoDivisionNames();

    for (int i = 1; i <= 4; ++i)
    {
        auto n = juce::String (i);

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "lfo" + n + "Rate", 1 }, "LFO " + n + " Rate",
            juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.35f), 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "lfo" + n + "Shape", 1 }, "LFO " + n + " Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "lfo" + n + "Sync", 1 }, "LFO " + n + " Sync", false));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "lfo" + n + "Division", 1 }, "LFO " + n + " Division",
            divNames, 2)); // default 1/4
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "lfo" + n + "FreeRun", 1 }, "LFO " + n + " Free Run", false));
    }

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createModMatrixParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto sourceNames = getModSourceNames();
    auto destNames = getModDestNames();

    for (int i = 0; i < 16; ++i)
    {
        auto prefix = "modSlot" + juce::String (i);
        auto label = "Mod " + juce::String (i + 1);

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { prefix + "Src", 1 }, label + " Source",
            sourceNames, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { prefix + "Dst", 1 }, label + " Dest",
            destNames, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "Amt", 1 }, label + " Amount",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { prefix + "On", 1 }, label + " Enabled", false));
    }

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createGlobalParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "masterVol", 1 }, "Master Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "stereoWidth", 1 }, "Stereo Width",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscMix", 1 }, "Osc Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "velocityCurve", 1 }, "Velocity Curve",
        juce::StringArray { "Linear", "Soft", "Hard", "Fixed" }, 0));

    return params;
}

// ═══════════════════════════════════════════════════════════════════
// Parameter Layout
// ═══════════════════════════════════════════════════════════════════

juce::AudioProcessorValueTreeState::ParameterLayout OStrataAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> allParams;

    auto addSection = [&allParams] (auto sectionParams) {
        for (auto& p : sectionParams)
            allParams.push_back (std::move (p));
    };

    addSection (createOscParameters ("oscA"));     // 10
    addSection (createOscParameters ("oscB"));     // 10
    addSection (createSubNoiseParameters());       //  5
    addSection (createAmpEnvelopeParameters());    //  4
    addSection (createFilterEnvelopeParameters()); //  5
    addSection (createFilterParameters ("filtA")); //  5
    addSection (createFilterParameters ("filtB")); //  5
    addSection (createFilterRoutingParameters()); //  1
    addSection (createTuningParameters());       //  7
    addSection (createReverbParameters());       //  6
    addSection (createDelayParameters());        //  5
    addSection (createChorusParameters());       //  3
    addSection (createDistortionParameters());   //  3
    addSection (createEQParameters());           //  4
    addSection (createLFOParameters());          //  8 (rate + shape only, routing via matrix)
    addSection (createModMatrixParameters());    // 64 (16 slots x 4 params)
    addSection (createGlobalParameters());       //  3

    return { allParams.begin(), allParams.end() };
}

// ═══════════════════════════════════════════════════════════════════
// Constructor / Destructor
// ═══════════════════════════════════════════════════════════════════

OStrataAudioProcessor::OStrataAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("OStrataParameters"), createParameterLayout()),
      presetManager (parameters, "O-Strata")
{
    // Tuning parameters are excluded from presets so that loading a preset
    // never changes the user's current tuning/tonic/pitch-bend/glide settings.
    presetManager.excludedParameterIds = {
        "tuningPreset", "tonic", "masterTune", "octaveStretch",
        "pitchBendRange", "glideMode", "glideTime"
    };

    // Initialize factory presets on first run, and regenerate whenever the
    // plugin version changes — otherwise on-disk factory JSON stays pinned to
    // the first-installed version's parameter set forever (WR-08)
    const juce::String factoryVersion (JucePlugin_VersionString);
    if (! presetManager.factoryPresetsExist()
        || presetManager.getFactoryPresetsVersion() != factoryVersion)
        presetManager.initializeFactoryPresets (FactoryPresets::build (parameters), factoryVersion);

    // Stage 1 placeholder: one sine frame (10 mipmap levels, guard samples set).
    // Phase 2.1's GeometryBakeScheduler replaces oscTablePtr[] with baked tables
    // and retires the previous ones through retireTable().
    placeholderTable = WavetableGenerator::generateProceduralTable (WaveShape::Sine);
    oscTablePtr[0].store (placeholderTable.get(), std::memory_order_release);
    oscTablePtr[1].store (placeholderTable.get(), std::memory_order_release);

    // Create 16 voices
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new StrataVoice();
        voice->setAPVTS (&parameters);
        voice->setTuningEngine (&tuningEngine);
        voice->setProcessor (this);
        voice->setPendingTuningSource (&vst3Extensions.getPendingTable()); // Phase 24: NE
        voice->setWavetableA (placeholderTable.get());
        voice->setWavetableB (placeholderTable.get());
        synthesiser.addVoice (voice);
    }

    synthesiser.addSound (new StrataSound());
    lastAssignedTable[0] = lastAssignedTable[1] = placeholderTable.get();

    // Cache APVTS atomic pointers for the FX configure step (read every block).
    pDistBypass     = parameters.getRawParameterValue ("distBypass");
    pDistType       = parameters.getRawParameterValue ("distType");
    pDistDrive      = parameters.getRawParameterValue ("distDrive");
    pDistMix        = parameters.getRawParameterValue ("distMix");
    pChorusBypass   = parameters.getRawParameterValue ("chorusBypass");
    pChorusRate     = parameters.getRawParameterValue ("chorusRate");
    pChorusDepth    = parameters.getRawParameterValue ("chorusDepth");
    pChorusMix      = parameters.getRawParameterValue ("chorusMix");
    pDelayBypass    = parameters.getRawParameterValue ("delayBypass");
    pDelayTime      = parameters.getRawParameterValue ("delayTime");
    pDelayFeedback  = parameters.getRawParameterValue ("delayFeedback");
    pDelayMode      = parameters.getRawParameterValue ("delayMode");
    pDelayMix       = parameters.getRawParameterValue ("delayMix");
    pReverbBypass   = parameters.getRawParameterValue ("reverbBypass");
    pReverbSize     = parameters.getRawParameterValue ("reverbSize");
    pReverbDamp     = parameters.getRawParameterValue ("reverbDamp");
    pReverbPredelay = parameters.getRawParameterValue ("reverbPredelay");
    pReverbMix      = parameters.getRawParameterValue ("reverbMix");
    pReverbModDepth = parameters.getRawParameterValue ("reverbModDepth");
    pReverbModRate  = parameters.getRawParameterValue ("reverbModRate");
    pEqBypass       = parameters.getRawParameterValue ("eqBypass");
    pEqLowGain      = parameters.getRawParameterValue ("eqLowGain");
    pEqMidGain      = parameters.getRawParameterValue ("eqMidGain");
    pEqMidFreq      = parameters.getRawParameterValue ("eqMidFreq");
    pEqHighGain     = parameters.getRawParameterValue ("eqHighGain");
    pDelaySync      = parameters.getRawParameterValue ("delaySync");
    pDelayDivision  = parameters.getRawParameterValue ("delayDivision");

    // Global LFO params (CR-06) — read every block by advanceGlobalLfoPhases
    for (int i = 0; i < 4; ++i)
    {
        auto n = juce::String (i + 1);
        pLfoSync[i]  = parameters.getRawParameterValue ("lfo" + n + "Sync");
        pLfoRate[i]  = parameters.getRawParameterValue ("lfo" + n + "Rate");
        pLfoDiv[i]   = parameters.getRawParameterValue ("lfo" + n + "Division");
        pLfoShape[i] = parameters.getRawParameterValue ("lfo" + n + "Shape");
    }

    // Tuning + global params (CR-05 / IN-01) — read every block
    pMasterTune     = parameters.getRawParameterValue ("masterTune");
    pOctaveStretch  = parameters.getRawParameterValue ("octaveStretch");
    pPitchBendRange = parameters.getRawParameterValue ("pitchBendRange");
    pTuningPreset   = parameters.getRawParameterValue ("tuningPreset");
    pTonic          = parameters.getRawParameterValue ("tonic");
    pStereoWidth    = parameters.getRawParameterValue ("stereoWidth");
    pMasterVol      = parameters.getRawParameterValue ("masterVol");

    // Processor-level mod matrix for global FX destinations (WR-02)
    fxModMatrix.setAPVTS (&parameters);

    // Reaper for retired wavetables (see retireTable / timerCallback)
    startTimer (500);
}

OStrataAudioProcessor::~OStrataAudioProcessor()
{
    cancelPendingUpdate();
    stopTimer();
}

// ═══════════════════════════════════════════════════════════════════
// Audio Processing
// ═══════════════════════════════════════════════════════════════════

void OStrataAudioProcessor::advanceGlobalLfoPhases (int numSamples, double sampleRate)
{
    // Keep in sync with the rate calculation in StrataVoice::renderNextBlock.
    // Param pointers are cached in the constructor — juce::String construction
    // heap-allocates and must never happen on the audio thread (CR-06).
    const double bpm = currentBPM.load (std::memory_order_relaxed);

    for (int i = 0; i < 4; ++i)
    {
        float rateHz;
        if (pLfoSync[i]->load() > 0.5f)
        {
            const int divIdx = juce::jlimit (0, 17, static_cast<int> (pLfoDiv[i]->load()));
            const float beats = NoteDiv::kDivBeats[divIdx];
            const float seconds = static_cast<float> (beats * 60.0 / bpm);
            rateHz = 1.0f / seconds;
        }
        else
        {
            rateHz = pLfoRate[i]->load();
        }

        double& phase = globalLfoPhase[static_cast<size_t> (i)];
        phase += (static_cast<double> (rateHz) / sampleRate) * static_cast<double> (numSamples);
        phase -= std::floor (phase); // wrap to [0, 1)
    }
}

void OStrataAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);

    globalLfoPhase.fill (0.0);

    // Prepare all voices
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<StrataVoice*> (synthesiser.getVoice (i)))
            voice->prepare (sampleRate, samplesPerBlock);
    }

    // Effects chain
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
    distortion.prepare (spec);
    chorus.prepare (spec);
    delay.prepare (spec);
    eq.prepare (spec);
    reverbProcessor.prepare (spec);

    for (auto& lfo : fxLfo)
        lfo.prepare (sampleRate);

    distWasActive = chorusWasActive = delayWasActive = reverbWasActive = eqWasActive = false;

    masterVolSmoothed.reset (sampleRate, 0.02);
    stereoWidthSmoothed.reset (sampleRate, 0.02);

    // IN-04: distortion (oversampler) is the only latency source and is
    // skipped entirely when bypassed — report 0 then, or the host delay-
    // compensates a path with no latency. Kept current by timerCallback.
    setLatencySamples (pDistBypass->load() > 0.5f
        ? 0 : static_cast<int> (distortion.getLatencyInSamples()));
}

void OStrataAudioProcessor::releaseResources() {}

void OStrataAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
    // correlate tuning deltas to their NoteOn's MIDI pitch.
    vst3Extensions.drainAndUpdate();

    // Read BPM from host transport for tempo-synced LFOs
    if (auto* playHead = getPlayHead())
    {
        if (auto posInfo = playHead->getPosition())
        {
            if (auto bpm = posInfo->getBpm())
                currentBPM.store (*bpm, std::memory_order_relaxed);
        }
    }

    // TuningEngine sync (CR-05): detect parameter changes here, but defer the
    // engine mutation to the message thread. setBuiltInPreset heap-allocates
    // and setCustomIntervals/rebuildFrequencyTable take intervalMutex — none
    // of that may run on the audio thread. Voices keep reading the lock-free
    // frequencyTable atomics, which handleAsyncUpdate republishes.
    {
        const float masterTune = pMasterTune->load();
        const float octaveStretch = pOctaveStretch->load();
        const float pbRange = pPitchBendRange->load();
        const int tuningPreset = static_cast<int> (pTuningPreset->load());
        const int tonic = static_cast<int> (pTonic->load());

        bool changed = false;

        if (masterTune != lastMasterTune || octaveStretch != lastOctaveStretch
            || pbRange != lastPitchBendRange || tonic != lastTonic)
        {
            lastMasterTune = masterTune;
            lastOctaveStretch = octaveStretch;
            lastPitchBendRange = pbRange;
            lastTonic = tonic;
            changed = true;
        }

        if (tuningPreset != lastTuningPreset)
        {
            lastTuningPreset = tuningPreset;
            // Preset application is flagged separately: handleAsyncUpdate must
            // NOT reapply the preset when only a scalar changed, or it would
            // clobber a user-loaded .scl (engine preset = Custom).
            pendingTuningPresetChange.store (true, std::memory_order_release);
            changed = true;
        }

        if (changed)
            triggerAsyncUpdate();
    }

    // Update wavetable assignments if table selection changed
    updateWavetableAssignments();

    // Track active MIDI notes and extract CC data for mod matrix
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            noteStates[static_cast<size_t> (msg.getNoteNumber())].store (true, std::memory_order_relaxed);
        else if (msg.isNoteOff())
            noteStates[static_cast<size_t> (msg.getNoteNumber())].store (false, std::memory_order_relaxed);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            for (auto& s : noteStates) s.store (false, std::memory_order_relaxed);
        else if (msg.isController() && msg.getControllerNumber() == 1) // Mod wheel
            modWheelValue.store (msg.getControllerValue() / 127.0f, std::memory_order_relaxed);
        else if (msg.isChannelPressure()) // Channel aftertouch
            aftertouchValue.store (msg.getChannelPressureValue() / 127.0f, std::memory_order_relaxed);
    }

    // Render synth voices
    synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Advance shared free-running LFO phases for next block
    advanceGlobalLfoPhases (buffer.getNumSamples(), getSampleRate());

    // Global FX mod destinations (WR-02): evaluate the processor-level matrix
    // once per block. Sources are the global LFOs (sampled at the shared
    // phases), mod wheel, and aftertouch; per-voice sources read as 0 here.
    fxModMatrix.updateFromAPVTS();
    for (int i = 0; i < 4; ++i)
    {
        fxLfo[i].setShape (static_cast<LFO::Shape> (static_cast<int> (pLfoShape[i]->load())));
        fxLfo[i].setPhase (globalLfoPhase[static_cast<size_t> (i)]);
        fxModMatrix.setSourceValue (
            static_cast<ModSource> (static_cast<int> (ModSource::LFO1) + i),
            fxLfo[i].getNextSample());
    }
    fxModMatrix.setSourceValue (ModSource::ModWheel, modWheelValue.load (std::memory_order_relaxed));
    fxModMatrix.setSourceValue (ModSource::Aftertouch, aftertouchValue.load (std::memory_order_relaxed));
    fxModMatrix.evaluate();

    // Effects chain (float precision)
    juce::dsp::AudioBlock<float> block (buffer);

    // 1. Distortion
    const bool distRan = runEffect (pDistBypass, distortion, block, [this] (auto& fx, auto& blk)
    {
        fx.setType (static_cast<int> (pDistType->load()));
        fx.setDrive (pDistDrive->load());
        const float mix = juce::jlimit (0.0f, 1.0f,
            pDistMix->load() + fxModMatrix.getModOffset (ModDest::DistMix));
        fx.setMix (mix);
        if (mix <= 0.001f)
            return false;
        fx.process (blk);
        return true;
    });
    if (! distRan && distWasActive)
        distortion.reset();
    distWasActive = distRan;

    // 2. Chorus
    const bool chorusRan = runEffect (pChorusBypass, chorus, block, [this] (auto& fx, auto& blk)
    {
        fx.setRate (pChorusRate->load());
        fx.setDepth (pChorusDepth->load());
        const float mix = juce::jlimit (0.0f, 1.0f,
            pChorusMix->load() + fxModMatrix.getModOffset (ModDest::ChorusMix));
        fx.setMix (mix);
        if (mix <= 0.001f)
            return false;
        fx.process (blk);
        return true;
    });
    if (! chorusRan && chorusWasActive)
        chorus.reset();
    chorusWasActive = chorusRan;

    // 3. Delay (WR-03: tempo sync — delaySync + delayDivision drive the time
    // from the host BPM, mirroring the LFO division table)
    const bool delayRan = runEffect (pDelayBypass, delay, block, [this] (auto& fx, auto& blk)
    {
        float timeSec;
        if (pDelaySync->load() > 0.5f)
        {
            const int divIdx = juce::jlimit (0, 17, static_cast<int> (pDelayDivision->load()));
            const double bpm = currentBPM.load (std::memory_order_relaxed);
            timeSec = static_cast<float> (NoteDiv::kDivBeats[divIdx] * 60.0 / bpm);
        }
        else
        {
            timeSec = pDelayTime->load();
        }
        fx.setTime (timeSec); // clamped to kMaxDelaySeconds internally

        fx.setFeedback (pDelayFeedback->load());
        fx.setMode (static_cast<int> (pDelayMode->load()));
        const float mix = juce::jlimit (0.0f, 1.0f,
            pDelayMix->load() + fxModMatrix.getModOffset (ModDest::DelayMix));
        fx.setMix (mix);
        if (mix <= 0.001f)
            return false;
        fx.process (blk);
        return true;
    });
    if (! delayRan && delayWasActive)
        delay.reset();
    delayWasActive = delayRan;

    // 4. Reverb
    const bool reverbRan = runEffect (pReverbBypass, reverbProcessor, block, [this] (auto& fx, auto& blk)
    {
        fx.setSize (pReverbSize->load());
        fx.setDamping (pReverbDamp->load());
        fx.setPredelay (pReverbPredelay->load());
        const float mix = juce::jlimit (0.0f, 1.0f,
            pReverbMix->load() + fxModMatrix.getModOffset (ModDest::ReverbMix));
        fx.setMix (mix);
        fx.setModDepth (pReverbModDepth->load());
        fx.setModRate (pReverbModRate->load());
        if (mix <= 0.001f)
            return false;
        fx.process (blk);
        return true;
    });
    if (! reverbRan && reverbWasActive)
        reverbProcessor.reset();
    reverbWasActive = reverbRan;

    // 5. EQ
    const bool eqRan = runEffect (pEqBypass, eq, block, [this] (auto& fx, auto& blk)
    {
        const float lowGain  = pEqLowGain->load();
        const float midGain  = pEqMidGain->load();
        const float highGain = pEqHighGain->load();
        fx.setLowGain (lowGain);
        fx.setMidGain (midGain);
        fx.setMidFreq (pEqMidFreq->load());
        fx.setHighGain (highGain);
        if (std::abs (lowGain) > 0.1f || std::abs (midGain) > 0.1f || std::abs (highGain) > 0.1f)
        {
            fx.process (blk);
            return true;
        }
        return false;
    });
    if (! eqRan && eqWasActive)
        eq.reset();
    eqWasActive = eqRan;

    // Stereo width (mid-side processing) + master volume (smoothed per-sample).
    // Master volume takes the WR-02 MasterVol mod offset.
    float stereoWidth = pStereoWidth->load();
    float masterVol = juce::jlimit (0.0f, 1.0f,
        pMasterVol->load() + fxModMatrix.getModOffset (ModDest::MasterVol));
    stereoWidthSmoothed.setTargetValue (stereoWidth);
    masterVolSmoothed.setTargetValue (masterVol);

    if (buffer.getNumChannels() >= 2)
    {
        auto* leftData = buffer.getWritePointer (0);
        auto* rightData = buffer.getWritePointer (1);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float width = stereoWidthSmoothed.getNextValue();
            float gain = masterVolSmoothed.getNextValue();

            float mid = (leftData[sample] + rightData[sample]) * 0.5f;
            float side = (leftData[sample] - rightData[sample]) * 0.5f;

            leftData[sample] = (mid + side * width) * gain;
            rightData[sample] = (mid - side * width) * gain;
        }
    }
    else
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float gain = masterVolSmoothed.getNextValue();
            buffer.setSample (0, sample, buffer.getSample (0, sample) * gain);
        }
    }

    // Publish "this block is done reading wavetables" for the retired-table reaper
    blockGeneration.fetch_add (1, std::memory_order_release);
}

// ═══════════════════════════════════════════════════════════════════
// Deferred TuningEngine sync (CR-05)
// ═══════════════════════════════════════════════════════════════════

void OStrataAudioProcessor::handleAsyncUpdate()
{
    // Message thread. The scalar setters are idempotent (internal change
    // detection), so they are applied unconditionally with the latest values.
    tuningEngine.setMasterTune (static_cast<double> (pMasterTune->load()));
    tuningEngine.setOctaveStretch (pOctaveStretch->load());
    tuningEngine.setPitchBendRange (pPitchBendRange->load());
    tuningEngine.setTonicNote (static_cast<int> (pTonic->load()));

    // The preset is only applied when the parameter actually changed —
    // otherwise it would clobber a user-loaded .scl (engine preset = Custom).
    if (pendingTuningPresetChange.exchange (false, std::memory_order_acq_rel))
        tuningEngine.setBuiltInPreset (
            static_cast<TuningEngine::BuiltInPreset> (static_cast<int> (pTuningPreset->load())));
}

// ═══════════════════════════════════════════════════════════════════
// Retired-table reaper
// ═══════════════════════════════════════════════════════════════════

void OStrataAudioProcessor::retireTable (std::unique_ptr<WavetableData> table)
{
    if (table != nullptr)
        retiredTables.push_back ({ std::move (table),
                                   blockGeneration.load (std::memory_order_acquire) });
}

void OStrataAudioProcessor::timerCallback()
{
    // IN-04: follow distortion bypass with the reported latency (message
    // thread — setLatencySamples notifies the host).
    const int wantedLatency = pDistBypass->load() > 0.5f
        ? 0 : static_cast<int> (distortion.getLatencyInSamples());
    if (wantedLatency != getLatencySamples())
        setLatencySamples (wantedLatency);

    if (retiredTables.empty())
        return;

    // A table is safe to free once two generations have passed since
    // retirement: at least one full processBlock has then started AFTER the
    // new pointers were published (its updateWavetableAssignments repointed
    // every voice) and completed. If the host stops calling processBlock the
    // generation freezes and tables are simply held — never freed unsafely.
    const auto gen = blockGeneration.load (std::memory_order_acquire);
    retiredTables.erase (
        std::remove_if (retiredTables.begin(), retiredTables.end(),
                        [gen] (const RetiredTable& r) { return gen >= r.retiredAt + 2; }),
        retiredTables.end());
}

void OStrataAudioProcessor::updateWavetableAssignments()
{
    const WavetableData* targetA = oscTablePtr[0].load (std::memory_order_acquire);
    const WavetableData* targetB = oscTablePtr[1].load (std::memory_order_acquire);

    if (targetA != lastAssignedTable[0] || targetB != lastAssignedTable[1])
    {
        for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<StrataVoice*> (synthesiser.getVoice (i)))
            {
                if (targetA != lastAssignedTable[0])
                    voice->setWavetableA (targetA);
                if (targetB != lastAssignedTable[1])
                    voice->setWavetableB (targetB);
            }
        }
        lastAssignedTable[0] = targetA;
        lastAssignedTable[1] = targetB;
    }
}

// ═══════════════════════════════════════════════════════════════════
// Published oscillator tables
// ═══════════════════════════════════════════════════════════════════

const WavetableData* OStrataAudioProcessor::getActiveOscTable (int oscIndex) const
{
    return oscTablePtr[juce::jlimit (0, 1, oscIndex)].load (std::memory_order_acquire);
}

// ═══════════════════════════════════════════════════════════════════
// State Persistence
// ═══════════════════════════════════════════════════════════════════

void OStrataAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Full tuning engine state: intervals, name, tonic, mode, preset, and the
    // KBM mapping block (WR-17: KBM/mode/preset were previously lost on reload)
    auto tuningState = state.getOrCreateChildWithName ("tuningEngine", nullptr);
    tuningEngine.writeStateTo (tuningState);

    // v1.21.0: the UI language rides the same tree. Written as a STRING
    // ("en"/"fr") rather than the atomic's int index, so a hand-inspected
    // session file says what it means.
    state.setProperty ("uiLanguage",
                       languageCode (uiLanguage.load (std::memory_order_acquire)), nullptr);

    // Stage 1: reserve the child Phase 4.1 fills (ARCHITECTURE "State Persistence"); written
    // empty so the round-trip test can assert its presence and a 4.1 reader never sees a
    // missing node.
    state.getOrCreateChildWithName ("geometryImports", nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void OStrataAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        parameters.replaceState (state);

        // Sync cached values so processBlock doesn't overwrite restored TuningEngine state
        lastTuningPreset = static_cast<int> (parameters.getRawParameterValue ("tuningPreset")->load());
        lastTonic = static_cast<int> (parameters.getRawParameterValue ("tonic")->load());
        lastMasterTune = parameters.getRawParameterValue ("masterTune")->load();
        lastOctaveStretch = parameters.getRawParameterValue ("octaveStretch")->load();
        lastPitchBendRange = parameters.getRawParameterValue ("pitchBendRange")->load();

        // Apply the restored scalar params to the engine once (message-thread
        // path of the CR-05 deferral; the audio thread only change-detects)
        tuningEngine.setMasterTune (static_cast<double> (lastMasterTune));
        tuningEngine.setOctaveStretch (lastOctaveStretch);
        tuningEngine.setPitchBendRange (lastPitchBendRange);

        // v1.21.0: the UI language. A NON-PARAMETER property round-trips
        // through XML as a STRING var, never a bool or an int, so isVoid() is
        // the ONLY correct guard and toString() the only correct read
        // (critical_valuetree_xml_roundtrip_loses_type). A pre-1.21.0 session
        // has no such property and the default (English) stands;
        // languageIndex() clamps anything that is not "fr" to 0, so a
        // hand-edited value degrades to English rather than to a bad index.
        const juce::var lang = parameters.state.getProperty ("uiLanguage");

        if (! lang.isVoid())
            uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);

        // Restore full tuning state (intervals + mode + preset + KBM, WR-17;
        // legacy sessions with only intervals/scaleName/tonic still load)
        auto tuningState = state.getChildWithName ("tuningEngine");
        if (tuningState.isValid())
            tuningEngine.restoreStateFrom (tuningState);

        // Stage 1: nothing to restore yet — an absent or empty geometryImports child is
        // the pre-4.1 state.
        juce::ignoreUnused (state.getChildWithName ("geometryImports"));
    }
}

// ═══════════════════════════════════════════════════════════════════
// Editor
// ═══════════════════════════════════════════════════════════════════

#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OStrataAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OStrataAudioProcessorEditor (*this);
#else
    return new juce::GenericAudioProcessorEditor (*this);   // console-target build
#endif
}

// ═══════════════════════════════════════════════════════════════════
// Plugin Instantiation
// ═══════════════════════════════════════════════════════════════════

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OStrataAudioProcessor();
}
