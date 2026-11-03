/*
   This file is part of O-SpectralShaper, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/

// Preset + state gate (v1.10.3). Prints every observable preset-manager outcome as
// TSV — the 7 parameters AND the two 32-band curves (customState) — so the same
// driver built against two header versions can be diffed. Uses only API present
// since preset-manager 1.0.2.
//   preset-gate <blobDir> write     save each preset's state blob into blobDir
//   preset-gate <blobDir> restore   reopen blobs written by another build
#include "PluginProcessor.h"

static std::unique_ptr<OSpectralShaperAudioProcessor> makeProcessor()
{
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    auto* typed = dynamic_cast<OSpectralShaperAudioProcessor*>(p.get());
    if (typed == nullptr) { std::cerr << "wrong processor type\n"; std::exit(2); }
    p.release();
    return std::unique_ptr<OSpectralShaperAudioProcessor>(typed);
}

// Every parameter and every curve band away from its default, so an omitted key
// or a skipped customLoad cannot pass by luck.
static void perturb(OSpectralShaperAudioProcessor& p)
{
    for (auto* param : p.getParameters())
        param->setValueNotifyingHost(param->getDefaultValue() > 0.5f ? 0.13f : 0.87f);

    std::array<float, 32> a, s;
    for (int i = 0; i < 32; ++i) { a[(size_t) i] = 0.42f - 0.01f * i; s[(size_t) i] = -0.37f + 0.02f * i; }
    p.setAttackCurve(a);
    p.setSustainCurve(s);
}

static void dump(const juce::String& tag, OSpectralShaperAudioProcessor& p)
{
    for (auto* param : p.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(param))
            std::cout << tag << '\t' << r->getParameterID() << '\t'
                      << juce::String::formatted("%.9g", r->getValue()) << '\t'
                      << param->getText(param->getValue(), 64) << '\n';

    auto curve = [&](const char* name, const std::array<float, 32>& c)
    {
        std::cout << tag << '\t' << name;
        for (float v : c) std::cout << '\t' << juce::String::formatted("%.9g", v);
        std::cout << '\n';
    };
    curve("attackCurve", p.getAttackCurve());
    curve("sustainCurve", p.getSustainCurve());
}

static juce::MemoryBlock stateOf(juce::AudioProcessor& p)
{
    juce::MemoryBlock b;
    p.getStateInformation(b);
    return b;
}

int main(int argc, char** argv)
{
    if (argc < 3) { std::cerr << "usage: preset-gate <blobDir> write|restore\n"; return 2; }
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

        // Reopen into a perturbed instance: the restore must overwrite everything.
        auto fresh = makeProcessor();
        perturb(*fresh);
        fresh->setStateInformation(blob.getData(), (int) blob.getSize());
        dump("reopen:" + name, *fresh);

        auto blobFile = blobDir.getChildFile(name + ".bin");
        if (restore)
        {
            juce::MemoryBlock old;
            if (! blobFile.loadFileAsData(old))
                continue;   // a preset the baseline never had
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

    // Session round trip twice on one instance (stale <CustomState> child pattern).
    {
        auto twice = makeProcessor();
        pm.loadPreset(names[0]);
        auto b1 = stateOf(*proc);
        twice->setStateInformation(b1.getData(), (int) b1.getSize());
        pm.loadPreset(names[names.size() - 1]);
        auto b2 = stateOf(*proc);
        twice->setStateInformation(b2.getData(), (int) b2.getSize());
        dump("secondReopen", *twice);
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
    proc->setAttackCurve({});
    std::cout << "userLoad\t" << (int) pm.loadPreset(userName) << '\n';
    dump("userLoaded", *proc);
    std::cout << "userIsFactory\t" << (int) pm.isFactoryPreset(userName) << '\n';
    std::cout << "userDelete\t" << (int) pm.deletePreset(userName) << '\n';
    std::cout << "factorySaveRefused\t" << (int) ! pm.savePreset("Gentle Shaping") << '\n';
    std::cout << "factoryDeleteRefused\t" << (int) ! pm.deletePreset("Gentle Shaping") << '\n';
    std::cout << "missingLoadRefused\t" << (int) ! pm.loadPreset("__no_such_preset__") << '\n';
    return 0;
}
