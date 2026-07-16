/*
  ==============================================================================

    FactoryPresets.cpp
    O-simplePhysicalModelSynth — factory preset library.

    Each preset is a partial parameter snapshot in RAW (real) units. Raw values
    are converted to APVTS-normalized [0,1] via convertTo0to1 at build time so
    the on-disk JSON (which stores normalized values, fed straight to
    setValueNotifyingHost on load) round-trips correctly. Omitted IDs fall back
    to createParameterLayout() defaults.

    Authoring convention (RESEARCH §2 — load-bearing):
      - String presets (Bright Steel, Muted Nylon, Koto/Harp, Bowed String):
        set `material` (raw %), OMIT `damping`/`decay` — the Material macro
        derives both on load and the sliders visibly track.
      - Modal presets (Struck Bar, Bell): set `damping`/`decay` (raw %), OMIT
        `material` — leaving material at its default fires no listener, so the
        explicit damping/decay survive.
      - NEVER co-author `material` AND `damping`/`decay` in one preset.

    Choice params (excitationType, resonatorType) are authored as raw indices;
    convertTo0to1 maps index → normalized. stringModel is omitted everywhere
    (KS only in v1.0, default 0).

  ==============================================================================
*/

#include "FactoryPresets.h"
#include "PluginProcessor.h"   // ParamIDs (top-level namespace)
#include <map>

namespace
{
using Preset = OuariconPresetManager::FactoryPresetDef;
using RawMap = std::map<juce::String, float>;

/** Convert a raw-value map to APVTS-normalized [0,1] (skips unknown IDs). */
static std::map<juce::String, float>
normalize (juce::AudioProcessorValueTreeState& apvts, const RawMap& raw)
{
    std::map<juce::String, float> out;
    for (const auto& [id, value] : raw)
        if (auto* p = apvts.getParameter (id))
            out[id] = p->convertTo0to1 (value);
    return out;
}

static Preset makePreset (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& name, const RawMap& raw)
{
    Preset def;
    def.name = name;
    def.parameters = normalize (apvts, raw);
    return def;
}
} // namespace

namespace FactoryPresets
{
std::vector<OuariconPresetManager::FactoryPresetDef>
build (juce::AudioProcessorValueTreeState& apvts)
{
    using namespace ParamIDs;

    std::vector<Preset> presets;

    // ── String resonator (KS) — Material macro drives damping + decay ──────────

    // Bright Steel — Pluck → String. Low material (steel): bright + long ring.
    presets.push_back (makePreset (apvts, "Bright Steel", {
        { excitationType, 0.0f }, { resonatorType, 0.0f },
        { excitationPosition, 18.0f }, { excitationColor, 75.0f },
        { material, 8.0f },
        { ampAttack, 0.001f }, { ampRelease, 0.3f },
        { outputLevel, -6.0f } }));

    // Muted Nylon — Pluck → String. High material (nylon): dark + short decay.
    presets.push_back (makePreset (apvts, "Muted Nylon", {
        { excitationType, 0.0f }, { resonatorType, 0.0f },
        { excitationPosition, 30.0f }, { excitationColor, 35.0f },
        { material, 85.0f },
        { ampAttack, 0.001f }, { ampRelease, 0.15f },
        { outputLevel, -5.0f } }));

    // Koto Harp — Pluck → String. Mid material, plucky mid position/color.
    // NB: no '/' in the name — OuariconPresetManager uses the preset name verbatim
    // as the JSON filename (getChildFile), and '/' is a path separator that would
    // silently drop the file (the bar shows getFileNameWithoutExtension()).
    presets.push_back (makePreset (apvts, "Koto Harp", {
        { excitationType, 0.0f }, { resonatorType, 0.0f },
        { excitationPosition, 22.0f }, { excitationColor, 60.0f },
        { material, 35.0f },
        { ampAttack, 0.001f }, { ampRelease, 0.25f },
        { outputLevel, -6.0f } }));

    // ── Modal resonator — explicit damping/decay (material left at default) ─────

    // Struck Bar — Strike → Modal. Low inharmonicity (near-harmonic bar).
    presets.push_back (makePreset (apvts, "Struck Bar", {
        { excitationType, 1.0f }, { resonatorType, 1.0f },
        { excitationPosition, 30.0f }, { excitationColor, 55.0f },
        { inharmonicity, 10.0f }, { modeBrightness, 45.0f },
        { damping, 55.0f }, { decay, 60.0f },
        { ampAttack, 0.001f }, { ampRelease, 0.3f },
        { outputLevel, -8.0f } }));

    // Bell — Strike → Modal. High inharmonicity (inharmonic bell), long ring.
    presets.push_back (makePreset (apvts, "Bell", {
        { excitationType, 1.0f }, { resonatorType, 1.0f },
        { excitationPosition, 25.0f }, { excitationColor, 65.0f },
        { inharmonicity, 80.0f }, { modeBrightness, 70.0f },
        { damping, 70.0f }, { decay, 88.0f },
        { ampAttack, 0.001f }, { ampRelease, 0.6f },
        { outputLevel, -8.0f } }));

    // ── Bow → String (KS) — Material macro drives damping + decay ──────────────

    // Bowed String — Bow → String. Sustained, bright; bow force mid.
    presets.push_back (makePreset (apvts, "Bowed String", {
        { excitationType, 2.0f }, { resonatorType, 0.0f },
        { excitationPosition, 30.0f }, { excitationColor, 50.0f },
        { bowForce, 55.0f },
        { material, 15.0f },
        { ampAttack, 0.04f }, { ampRelease, 0.25f },
        { outputLevel, -6.0f } }));

    return presets;
}
} // namespace FactoryPresets
