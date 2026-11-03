/*
   This file is part of O-DigiDelay, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/

// Preset + state gate (v1.9.4). Prints every observable preset-manager outcome as
// TSV, so the same driver built against two header versions can be diffed.
// Uses only API present in both the 1.9.3 local header and the module.
//   preset-gate <blobDir> write     save each preset's state blob into blobDir
//   preset-gate <blobDir> restore   reopen blobs written by another build
//
// Beyond values, it checks the two local behaviours the module must keep:
//   * bypass (v1.8.0): never saved, never applied, never reset by a preset load.
//     perturb() leaves bypass ON, so every dump line for it must read On.
//   * IN-07: every parameter set during a load is inside a host gesture, gestures
//     balance, values are clamped, and a non-number value loads as the default.
//     Gesture COUNTS are not printed: 1.9.3 sets each parameter once, the module
//     sets it twice (reset pass, then value pass); only the invariants are.
#include "PluginProcessor.h"

static std::unique_ptr<OuariconDigitalDelayAudioProcessor> makeProcessor()
{
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    auto* typed = dynamic_cast<OuariconDigitalDelayAudioProcessor*>(p.get());
    if (typed == nullptr) { std::cerr << "wrong processor type\n"; std::exit(2); }
    p.release();
    return std::unique_ptr<OuariconDigitalDelayAudioProcessor>(typed);
}

// Every parameter away from its default, so an omitted key cannot pass by luck.
// bypass goes ON: a preset load must leave it there.
static void perturb(juce::AudioProcessor& p)
{
    for (auto* param : p.getParameters())
    {
        if (param == p.getBypassParameter())
            param->setValueNotifyingHost(1.0f);
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

// Records gesture depth per parameter while armed.
struct GestureProbe : juce::AudioProcessorParameter::Listener
{
    explicit GestureProbe(juce::AudioProcessor& p) : proc(p)
    {
        for (auto* param : proc.getParameters()) param->addListener(this);
        depth.assign((size_t) proc.getParameters().size(), 0);
    }
    ~GestureProbe() override
    {
        for (auto* param : proc.getParameters()) param->removeListener(this);
    }
    void parameterValueChanged(int index, float) override
    {
        if (! armed) return;
        ++sets;
        if (depth[(size_t) index] <= 0) ++outside;
        if (proc.getParameters()[index] == proc.getBypassParameter()) ++bypassTouched;
    }
    void parameterGestureChanged(int index, bool starting) override
    {
        if (! armed) return;
        depth[(size_t) index] += starting ? 1 : -1;
        if (depth[(size_t) index] < 0) ++unbalanced;
    }
    void arm()
    {
        std::fill(depth.begin(), depth.end(), 0);
        sets = outside = unbalanced = bypassTouched = 0;
        armed = true;
    }
    juce::String disarm()
    {
        armed = false;
        for (int d : depth) if (d != 0) ++unbalanced;
        return "setsInsideGesture=" + juce::String((int) (sets > 0 && outside == 0))
             + "\tunbalanced=" + juce::String(unbalanced)
             + "\tbypassTouched=" + juce::String(bypassTouched);
    }

    juce::AudioProcessor& proc;
    std::vector<int> depth;
    int sets = 0, outside = 0, unbalanced = 0, bypassTouched = 0;
    bool armed = false;
};

int main(int argc, char** argv)
{
    if (argc < 3) { std::cerr << "usage: preset-gate <blobDir> write|restore\n"; return 2; }
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File blobDir(argv[1]);
    const bool restore = juce::String(argv[2]) == "restore";
    blobDir.createDirectory();

    auto proc = makeProcessor();
    auto& pm = proc->presetManager;
    GestureProbe probe(*proc);

    // Navigation from a fresh instance (current preset not in the list), then in-list.
    std::cout << "navFreshNext\t" << pm.getNextPreset() << '\n';
    std::cout << "navFreshPrev\t" << pm.getPreviousPreset() << '\n';

    auto names = pm.getPresetList();
    names.sort(true);
    std::cout << "count\t" << names.size() << '\n';

    for (const auto& name : names)
    {
        perturb(*proc);
        probe.arm();
        const bool ok = pm.loadPreset(name);
        std::cout << "load\t" << name << '\t' << (int) ok
                  << "\tcurrent=" << pm.getCurrentPresetName()
                  << "\tfactory=" << (int) pm.isFactoryPreset(name) << '\n';
        std::cout << "gesture\t" << name << '\t' << probe.disarm() << '\n';
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

    pm.loadPreset(names[0]);
    std::cout << "navNext\t" << pm.getNextPreset() << '\n';
    std::cout << "navPrev\t" << pm.getPreviousPreset() << '\n';

    // User preset life cycle, plus the factory guards. Saved while bypassed:
    // the file must not carry bypass.
    const juce::String userName("__preset_gate__");
    perturb(*proc);
    std::cout << "userSave\t" << (int) pm.savePreset(userName) << '\n';
    const auto userFile = pm.getUserPresetsDirectory().getChildFile(userName + ".json");
    const auto saved = juce::JSON::parse(userFile);
    std::cout << "userFileHasBypass\t"
              << (int) saved.getProperty("parameters", {}).hasProperty("bypass") << '\n';
    dump("userSaved", *proc);
    proc->getParameters()[0]->setValueNotifyingHost(0.5f);
    std::cout << "userLoad\t" << (int) pm.loadPreset(userName) << '\n';
    dump("userLoaded", *proc);
    std::cout << "userIsFactory\t" << (int) pm.isFactoryPreset(userName) << '\n';
    std::cout << "userDelete\t" << (int) pm.deletePreset(userName) << '\n';

    // IN-07: a hand-edited file. time is a string, spread an object, feedback and
    // mod out of range, wet omitted, bypass present (must be ignored).
    const juce::String badName("__preset_gate_malformed__");
    const auto badFile = pm.getUserPresetsDirectory().getChildFile(badName + ".json");
    pm.getUserPresetsDirectory().createDirectory();
    badFile.replaceWithText(R"({"name":"__preset_gate_malformed__","version":"1.9.3","parameters":)"
                            R"({"time":"abc","spread":{},"feedback":1.7,"mod":-0.5,"sync":true,)"
                            R"("division":0.25,"dry":0.6,"bypass":0.0}})");
    perturb(*proc);
    probe.arm();
    std::cout << "malformedLoad\t" << (int) pm.loadPreset(badName) << '\n';
    std::cout << "gesture\t" << badName << '\t' << probe.disarm() << '\n';
    dump("malformed", *proc);
    badFile.deleteFile();

    std::cout << "factorySaveRefused\t" << (int) ! pm.savePreset(names[0]) << '\n';
    std::cout << "factoryDeleteRefused\t" << (int) ! pm.deletePreset(names[0]) << '\n';
    std::cout << "missingLoadRefused\t" << (int) ! pm.loadPreset("__no_such_preset__") << '\n';
    return 0;
}
