/*
   This file is part of O-Comp, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/

// Preset + state gate (v1.11.4). Prints every observable preset-manager outcome as
// TSV, so the same driver built against two header versions can be diffed.
// Uses only API present since preset-manager 1.0.2.
//   preset-gate <blobDir> write     save each preset's state blob into blobDir
//   preset-gate <blobDir> restore   reopen blobs written by another build
// Optional 3rd arg "scdefault": perturb only the 7 keys the factory presets carry and
// leave the four sc_* parameters at their defaults. 1.11.3 never reset omitted keys on
// load, so in the full mode the sc_* values a factory preset leaves behind differ by
// design; in this mode every line, state blob included, must match the old header.
#include "PluginProcessor.h"

static std::unique_ptr<OCompAudioProcessor> makeProcessor()
{
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    auto* typed = dynamic_cast<OCompAudioProcessor*>(p.get());
    if (typed == nullptr) { std::cerr << "wrong processor type\n"; std::exit(2); }
    p.release();
    return std::unique_ptr<OCompAudioProcessor>(typed);
}

// Every parameter away from its default, so an omitted key cannot pass by luck.
static bool scDefault = false;

static void perturb(juce::AudioProcessor& p)
{
    for (auto* param : p.getParameters())
    {
        auto* r = dynamic_cast<juce::RangedAudioParameter*>(param);
        if (r != nullptr && scDefault && r->getParameterID().startsWith("sc_"))
            param->setValueNotifyingHost(param->getDefaultValue());
        else
            param->setValueNotifyingHost(param->getDefaultValue() > 0.5f ? 0.13f : 0.87f);
    }
}

static void dump(const juce::String& tag, juce::AudioProcessor& p)
{
    for (auto* param : p.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(param))
            std::cout << tag << '\t' << r->getParameterID() << '\t'
                      << juce::String::formatted("%.9g", r->getValue()) << '\t'
                      << param->getText(param->getValue(), 64) << '\n';
}

static juce::MemoryBlock stateOf(juce::AudioProcessor& p)
{
    juce::MemoryBlock b;
    p.getStateInformation(b);
    return b;
}

int main(int argc, char** argv)
{
    if (argc < 3) { std::cerr << "usage: preset-gate <blobDir> write|restore [scdefault]\n"; return 2; }
    scDefault = argc > 3 && juce::String(argv[3]) == "scdefault";
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File blobDir(argv[1]);
    const bool restore = juce::String(argv[2]) == "restore";
    blobDir.createDirectory();

    auto proc = makeProcessor();
    auto& pm = proc->presetManager;

    // Navigation from a fresh instance (current preset not in the list), then in-list.
    std::cout << "navFreshNext\t" << pm.getNextPreset() << '\n';
    std::cout << "navFreshPrev\t" << pm.getPreviousPreset() << '\n';

    auto names = pm.getPresetList();
    names.sort(true);
    std::cout << "count\t" << names.size() << '\n';

    for (const auto& name : names)
    {
        perturb(*proc);
        std::cout << "load\t" << name << '\t' << (int) pm.loadPreset(name)
                  << "\tcurrent=" << pm.getCurrentPresetName()
                  << "\tfactory=" << (int) pm.isFactoryPreset(name) << '\n';
        dump("applied:" + name, *proc);

        auto blob = stateOf(*proc);
        std::cout << "state\t" << name << '\t' << blob.toBase64Encoding() << '\n';

        auto fresh = makeProcessor();
        fresh->setStateInformation(blob.getData(), (int) blob.getSize());
        dump("reopen:" + name, *fresh);

        auto blobFile = blobDir.getChildFile(name + ".bin");
        if (restore)
        {
            juce::MemoryBlock old;
            if (! blobFile.loadFileAsData(old))
                continue;   // a preset the baseline never had (e.g. an old file left in the bank)
            auto other = makeProcessor();
            other->setStateInformation(old.getData(), (int) old.getSize());
            dump("otherBuildBlob:" + name, *other);
        }
        else
        {
            blobFile.replaceWithData(blob.getData(), blob.getSize());
            auto same = makeProcessor();
            same->setStateInformation(blob.getData(), (int) blob.getSize());
            dump("otherBuildBlob:" + name, *same);
        }
    }

    pm.loadPreset(names[0]);
    std::cout << "navNext\t" << pm.getNextPreset() << '\n';
    std::cout << "navPrev\t" << pm.getPreviousPreset() << '\n';

    // User preset life cycle, plus the factory guards.
    const juce::String userName("__preset_gate__");
    perturb(*proc);
    std::cout << "userSave\t" << (int) pm.savePreset(userName) << '\n';
    dump("userSaved", *proc);
    proc->getParameters()[0]->setValueNotifyingHost(0.5f);
    std::cout << "userLoad\t" << (int) pm.loadPreset(userName) << '\n';
    dump("userLoaded", *proc);
    std::cout << "userIsFactory\t" << (int) pm.isFactoryPreset(userName) << '\n';
    std::cout << "userDelete\t" << (int) pm.deletePreset(userName) << '\n';
    std::cout << "factorySaveRefused\t" << (int) ! pm.savePreset("Gentle Glue") << '\n';
    std::cout << "factoryDeleteRefused\t" << (int) ! pm.deletePreset("Gentle Glue") << '\n';
    std::cout << "missingLoadRefused\t" << (int) ! pm.loadPreset("__no_such_preset__") << '\n';
    return 0;
}
