/*
  ==============================================================================

    PresetManager.cpp
    v1.3.0: Preset system implementation

  ==============================================================================
*/

#include "PresetManager.h"
#include "TuningEngine.h"

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& apvts, TuningEngine& tuning)
    : parameters(apvts)
    , tuningEngine(tuning)
{
    // Ensure directories exist
    getFactoryPresetsDirectory().createDirectory();
    getUserPresetsDirectory().createDirectory();
}

juce::File PresetManager::getPresetsDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("O-Marimba")
        .getChildFile("Presets");
}

juce::File PresetManager::getFactoryPresetsDirectory() const
{
    return getPresetsDirectory().getChildFile("Factory");
}

juce::File PresetManager::getUserPresetsDirectory() const
{
    return getPresetsDirectory().getChildFile("User");
}

bool PresetManager::isFactoryPreset(const juce::String& presetName) const
{
    auto factoryFile = getFactoryPresetsDirectory().getChildFile(presetName + ".json");
    return factoryFile.existsAsFile();
}

juce::var PresetManager::createPresetJson() const
{
    auto* preset = new juce::DynamicObject();

    // Save all APVTS parameters
    auto* paramsObj = new juce::DynamicObject();
    for (auto* param : parameters.processor.getParameters())
    {
        if (auto* paramWithID = dynamic_cast<juce::RangedAudioParameter*>(param))
        {
            paramsObj->setProperty(paramWithID->getParameterID(),
                                   paramWithID->getValue());
        }
    }
    preset->setProperty("parameters", juce::var(paramsObj));

    // Save tuning state
    auto* tuningObj = new juce::DynamicObject();

    // Intervals array
    juce::Array<juce::var> intervalsArray;
    for (const auto& cents : tuningEngine.getIntervals())
    {
        intervalsArray.add(cents);
    }
    tuningObj->setProperty("intervals", intervalsArray);
    tuningObj->setProperty("scaleName", tuningEngine.getActiveTuningName());
    tuningObj->setProperty("scaleDegrees", tuningEngine.getScaleDegrees());
    tuningObj->setProperty("tonic", tuningEngine.getTonicNote());
    tuningObj->setProperty("referencePitch", tuningEngine.getReferencePitch());

    preset->setProperty("tuning", juce::var(tuningObj));

    // Metadata
    preset->setProperty("version", "1.6.2");
    preset->setProperty("plugin", "O-Marimba");

    return juce::var(preset);
}

bool PresetManager::applyPresetJson(const juce::var& presetData)
{
    if (!presetData.isObject())
        return false;

    auto* preset = presetData.getDynamicObject();
    if (preset == nullptr)
        return false;

    // Restore parameters
    if (preset->hasProperty("parameters"))
    {
        auto paramsVar = preset->getProperty("parameters");
        if (auto* paramsObj = paramsVar.getDynamicObject())
        {
            for (auto& prop : paramsObj->getProperties())
            {
                if (auto* param = parameters.getParameter(prop.name.toString()))
                {
                    param->setValueNotifyingHost(static_cast<float>(prop.value));
                }
            }
        }
    }

    // Restore tuning state
    if (preset->hasProperty("tuning"))
    {
        auto tuningVar = preset->getProperty("tuning");
        if (auto* tuningObj = tuningVar.getDynamicObject())
        {
            // Restore intervals
            if (tuningObj->hasProperty("intervals"))
            {
                auto intervalsVar = tuningObj->getProperty("intervals");
                if (auto* intervalsArray = intervalsVar.getArray())
                {
                    std::vector<double> cents;
                    for (const auto& val : *intervalsArray)
                    {
                        cents.push_back(static_cast<double>(val));
                    }

                    juce::String scaleName = tuningObj->getProperty("scaleName").toString();
                    if (scaleName.isEmpty())
                        scaleName = "Custom";

                    tuningEngine.setCustomIntervals(cents, scaleName);
                }
            }

            // Restore tonic
            if (tuningObj->hasProperty("tonic"))
            {
                int tonic = static_cast<int>(tuningObj->getProperty("tonic"));
                tuningEngine.setTonicNote(tonic);
            }

            // Restore reference pitch (also handled by parameter, but ensure sync)
            if (tuningObj->hasProperty("referencePitch"))
            {
                double refPitch = static_cast<double>(tuningObj->getProperty("referencePitch"));
                tuningEngine.setReferencePitch(refPitch);
            }
        }
    }

    return true;
}

bool PresetManager::savePreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return false;

    // Don't allow overwriting factory presets
    if (isFactoryPreset(presetName))
    {
        juce::Logger::writeToLog("Cannot overwrite factory preset: " + presetName);
        return false;
    }

    auto presetFile = getUserPresetsDirectory().getChildFile(presetName + ".json");

    auto presetJson = createPresetJson();
    auto jsonString = juce::JSON::toString(presetJson, true);

    if (presetFile.replaceWithText(jsonString))
    {
        currentPresetName = presetName;
        juce::Logger::writeToLog("Preset saved: " + presetName);
        return true;
    }

    return false;
}

bool PresetManager::loadPreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return false;

    // Check factory presets first, then user presets
    juce::File presetFile = getFactoryPresetsDirectory().getChildFile(presetName + ".json");
    if (!presetFile.existsAsFile())
    {
        presetFile = getUserPresetsDirectory().getChildFile(presetName + ".json");
    }

    if (!presetFile.existsAsFile())
    {
        juce::Logger::writeToLog("Preset not found: " + presetName);
        return false;
    }

    auto jsonString = presetFile.loadFileAsString();
    auto presetData = juce::JSON::parse(jsonString);

    if (applyPresetJson(presetData))
    {
        currentPresetName = presetName;
        juce::Logger::writeToLog("Preset loaded: " + presetName);
        return true;
    }

    return false;
}

bool PresetManager::deletePreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return false;

    // Don't allow deleting factory presets
    if (isFactoryPreset(presetName))
    {
        juce::Logger::writeToLog("Cannot delete factory preset: " + presetName);
        return false;
    }

    auto presetFile = getUserPresetsDirectory().getChildFile(presetName + ".json");
    if (presetFile.existsAsFile())
    {
        if (presetFile.deleteFile())
        {
            if (currentPresetName == presetName)
                currentPresetName = "Default";
            return true;
        }
    }

    return false;
}

juce::StringArray PresetManager::getPresetList() const
{
    juce::StringArray presets;

    // Add factory presets first
    auto factoryDir = getFactoryPresetsDirectory();
    if (factoryDir.isDirectory())
    {
        for (const auto& file : factoryDir.findChildFiles(juce::File::findFiles, false, "*.json"))
        {
            presets.add(file.getFileNameWithoutExtension());
        }
    }

    // Add user presets
    auto userDir = getUserPresetsDirectory();
    if (userDir.isDirectory())
    {
        for (const auto& file : userDir.findChildFiles(juce::File::findFiles, false, "*.json"))
        {
            auto name = file.getFileNameWithoutExtension();
            if (!presets.contains(name))  // Avoid duplicates
                presets.add(name);
        }
    }

    presets.sort(true);  // Case-insensitive sort
    return presets;
}

juce::String PresetManager::getNextPreset() const
{
    auto presets = getPresetList();
    if (presets.isEmpty())
        return currentPresetName;

    int currentIndex = presets.indexOf(currentPresetName);
    if (currentIndex < 0)
        return presets[0];

    int nextIndex = (currentIndex + 1) % presets.size();
    return presets[nextIndex];
}

juce::String PresetManager::getPreviousPreset() const
{
    auto presets = getPresetList();
    if (presets.isEmpty())
        return currentPresetName;

    int currentIndex = presets.indexOf(currentPresetName);
    if (currentIndex < 0)
        return presets[0];

    int prevIndex = (currentIndex - 1 + presets.size()) % presets.size();
    return presets[prevIndex];
}

std::unique_ptr<juce::XmlElement> PresetManager::getStateAsXml() const
{
    // Start with APVTS state
    auto state = parameters.copyState();
    auto xml = state.createXml();

    if (xml != nullptr)
    {
        // Add tuning state as child element
        auto* tuningXml = xml->createNewChildElement("TuningState");

        // Serialize intervals
        juce::String intervalsStr;
        for (const auto& cents : tuningEngine.getIntervals())
        {
            if (!intervalsStr.isEmpty())
                intervalsStr += ",";
            intervalsStr += juce::String(cents, 6);
        }
        tuningXml->setAttribute("intervals", intervalsStr);
        tuningXml->setAttribute("scaleName", tuningEngine.getActiveTuningName());
        tuningXml->setAttribute("scaleDegrees", tuningEngine.getScaleDegrees());
        tuningXml->setAttribute("tonic", tuningEngine.getTonicNote());
        tuningXml->setAttribute("referencePitch", tuningEngine.getReferencePitch());

        // Save current preset name
        tuningXml->setAttribute("presetName", currentPresetName);
    }

    return xml;
}

void PresetManager::setStateFromXml(const juce::XmlElement* xml)
{
    if (xml == nullptr)
        return;

    // Restore APVTS state
    if (xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }

    // Restore tuning state
    if (auto* tuningXml = xml->getChildByName("TuningState"))
    {
        // Restore intervals
        juce::String intervalsStr = tuningXml->getStringAttribute("intervals");
        if (intervalsStr.isNotEmpty())
        {
            std::vector<double> cents;
            juce::StringArray tokens;
            tokens.addTokens(intervalsStr, ",", "");

            for (const auto& token : tokens)
            {
                cents.push_back(token.getDoubleValue());
            }

            juce::String scaleName = tuningXml->getStringAttribute("scaleName", "Custom");
            tuningEngine.setCustomIntervals(cents, scaleName);
        }

        // Restore tonic
        int tonic = tuningXml->getIntAttribute("tonic", 0);
        tuningEngine.setTonicNote(tonic);

        // Restore reference pitch
        double refPitch = tuningXml->getDoubleAttribute("referencePitch", 440.0);
        tuningEngine.setReferencePitch(refPitch);

        // Restore preset name
        currentPresetName = tuningXml->getStringAttribute("presetName", "Default");
    }
}

const std::vector<PresetManager::FactoryPreset>& PresetManager::getFactoryPresetData()
{
    // v1.6.2: Updated with new timbre parameters (strikePosition, overtoneDamping, tone)
    static const std::vector<FactoryPreset> factoryPresets = {
        // Default Marimba - warm, natural sound with 12-TET
        {
            "Default Marimba",
            { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 },
            "12-TET Standard",
            0,      // tonic = C
            0.5f,   // mallet hardness
            0.5f,   // bar material
            0.6f,   // resonance
            0.5f,   // strike position (center)
            0.5f,   // overtone damping (natural)
            0.75f,  // tone (default)
            0,      // tuning mode = 12-TET
            440.0f, // reference pitch
            0.5f,   // velocity curve
            0.0f    // output gain (dB)
        },
        // Bright Marimba - harder mallet, edge strike, bright tone
        {
            "Bright Marimba",
            { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 },
            "12-TET Standard",
            0,
            0.85f,  // harder mallet
            0.8f,   // brighter material
            0.45f,  // less resonance
            0.15f,  // edge strike (more overtones)
            0.3f,   // less damping (shimmery)
            0.95f,  // bright tone
            0,
            440.0f,
            0.6f,   // more dynamic
            0.0f
        },
        // Soft Marimba - gentle, mellow tone, center strike
        {
            "Soft Marimba",
            { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 },
            "12-TET Standard",
            0,
            0.2f,   // soft mallet
            0.25f,  // warmer material
            0.8f,   // more resonance
            0.5f,   // center strike (pure fundamental)
            0.6f,   // moderate damping
            0.5f,   // warm tone
            0,
            440.0f,
            0.35f,  // less dynamic
            0.0f
        },
        // Pad Marimba - long sustain, shimmering overtones
        {
            "Pad Marimba",
            { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 },
            "12-TET Standard",
            0,
            0.4f,   // medium-soft mallet
            0.6f,   // moderate material
            0.95f,  // maximum resonance
            0.5f,   // center strike
            0.1f,   // minimal damping (all modes sustain)
            0.7f,   // balanced tone
            0,
            440.0f,
            0.45f,
            0.0f
        },
        // Staccato Marimba - tight, focused
        {
            "Staccato Marimba",
            { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 },
            "12-TET Standard",
            0,
            0.7f,   // harder mallet
            0.4f,   // moderate material
            0.15f,  // short resonance
            0.5f,   // center strike
            0.9f,   // heavy damping (tight)
            0.6f,   // slightly warm
            0,
            440.0f,
            0.55f,
            0.0f
        },
        // Just Intonation - pure intervals
        {
            "Just Intonation",
            { 0, 112, 204, 316, 386, 498, 590, 702, 814, 884, 996, 1088 },
            "Just Intonation",
            0,
            0.5f,
            0.5f,
            0.6f,
            0.5f,   // center
            0.5f,   // natural
            0.75f,  // default
            1,      // tuning mode = Scala
            440.0f,
            0.5f,
            0.0f
        },
        // Pythagorean - based on perfect fifths
        {
            "Pythagorean",
            { 0, 90, 204, 294, 408, 498, 612, 702, 792, 906, 996, 1110 },
            "Pythagorean",
            0,
            0.5f,
            0.5f,
            0.6f,
            0.5f,
            0.5f,
            0.75f,
            1,
            440.0f,
            0.5f,
            0.0f
        },
        // Meantone - historical temperament
        {
            "Quarter-Comma Meantone",
            { 0, 76, 193, 310, 386, 503, 579, 697, 773, 890, 1007, 1083 },
            "1/4-Comma Meantone",
            0,
            0.5f,
            0.5f,
            0.6f,
            0.5f,
            0.5f,
            0.75f,
            1,
            440.0f,
            0.5f,
            0.0f
        },
        // Baroque A=415
        {
            "Baroque A=415",
            { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 },
            "12-TET Standard",
            0,
            0.35f,  // slightly softer (gut mallets)
            0.35f,  // warmer
            0.7f,   // more resonance
            0.5f,
            0.55f,  // slightly more damped
            0.6f,   // warmer tone
            0,
            415.0f, // Baroque pitch
            0.5f,
            0.0f
        },
        // Concert A=442
        {
            "Concert A=442",
            { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 },
            "12-TET Standard",
            0,
            0.5f,
            0.5f,
            0.6f,
            0.5f,
            0.5f,
            0.75f,
            0,
            442.0f, // European concert pitch
            0.5f,
            0.0f
        }
    };

    return factoryPresets;
}

void PresetManager::initializeFactoryPresets()
{
    auto factoryDir = getFactoryPresetsDirectory();
    factoryDir.createDirectory();

    for (const auto& preset : getFactoryPresetData())
    {
        auto presetFile = factoryDir.getChildFile(preset.name + ".json");

        // v1.6.2: Always regenerate factory presets to include new parameters
        // This ensures all users get the updated presets with new timbre controls
        if (true)  // Force regeneration
        {
            auto* presetObj = new juce::DynamicObject();

            // Parameters
            auto* paramsObj = new juce::DynamicObject();

            // Convert parameter values to normalized 0-1 range
            paramsObj->setProperty("MALLET_HARDNESS", preset.malletHardness);
            paramsObj->setProperty("BAR_MATERIAL", preset.barMaterial);
            paramsObj->setProperty("RESONANCE", preset.resonance);
            // v1.6.0: New timbre parameters
            paramsObj->setProperty("STRIKE_POSITION", preset.strikePosition);
            paramsObj->setProperty("OVERTONE_DAMPING", preset.overtoneDamping);
            paramsObj->setProperty("TONE", preset.tone);
            paramsObj->setProperty("TUNING_MODE", static_cast<float>(preset.tuningMode) / 2.0f);
            // Reference pitch: 400-480 range, normalize
            paramsObj->setProperty("REFERENCE_PITCH", (preset.referencePitch - 400.0f) / 80.0f);
            paramsObj->setProperty("VEL_CURVE", preset.velCurve);
            // Output gain: -24 to +12 range, normalize
            paramsObj->setProperty("OUTPUT_GAIN", (preset.outputGain + 24.0f) / 36.0f);

            presetObj->setProperty("parameters", juce::var(paramsObj));

            // Tuning
            auto* tuningObj = new juce::DynamicObject();
            juce::Array<juce::var> intervalsArray;
            for (const auto& cents : preset.intervals)
            {
                intervalsArray.add(cents);
            }
            tuningObj->setProperty("intervals", intervalsArray);
            tuningObj->setProperty("scaleName", preset.scaleName);
            tuningObj->setProperty("scaleDegrees", static_cast<int>(preset.intervals.size()));
            tuningObj->setProperty("tonic", preset.tonic);
            tuningObj->setProperty("referencePitch", static_cast<double>(preset.referencePitch));

            presetObj->setProperty("tuning", juce::var(tuningObj));

            // Metadata
            presetObj->setProperty("version", "1.6.2");
            presetObj->setProperty("plugin", "O-Marimba");
            presetObj->setProperty("factory", true);

            auto jsonString = juce::JSON::toString(juce::var(presetObj), true);
            presetFile.replaceWithText(jsonString);
        }
    }

    juce::Logger::writeToLog("Factory presets initialized in: " + factoryDir.getFullPathName());
}
