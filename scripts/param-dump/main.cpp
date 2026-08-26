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

    Ouaricon param-dump — a RUNTIME parameter inventory.

    Why runtime and not a source regex:

      - O-Emulator builds four of its five parameters from a local `percent()`
        factory lambda (Source/PluginProcessor.cpp:61-74), so a regex over
        `make_unique<juce::AudioParameter...>` finds TWO.
      - O-Prism concatenates its parameter IDs
        (Source/PluginProcessor.cpp:76-87), so the literal ID string never
        appears anywhere in the source at all.

    Only a walk of AudioProcessor::getParameters() on a constructed processor
    sees the real set. This program constructs the plugin via the standard
    JUCE factory `createPluginFilter()` — which every plugin in this repo
    defines at the foot of PluginProcessor.cpp — so the tool is entirely
    plugin-agnostic: no plugin class name appears here.

    Output: TSV on stdout, `#`-prefixed comment/header lines first, then one
    row per parameter. The caller redirects:

        ./O-Prism-param-dump > plugins/O-Prism/.planning/params.tsv

    Columns:
        id            getParameterID() (empty for a parameter that is not
                      hosted/ID-bearing — reported as `<no-id>` so a blank
                      cell can never be mistaken for a successful read)
        name          getName(128)
        label         getLabel()          — the unit, where one was given
        numSteps      getNumSteps()       — 0x7fffffff for a continuous param
        textAtMin     getText(0.0f, 64)
        textAtMax     getText(1.0f, 64)
        defaultNorm   getDefaultValue()   — NORMALISED 0..1
        defaultText   getText(getDefaultValue(), 64)
        flags         comma-joined subset of: automatable, discrete, boolean,
                      meta, inverted

    The editor is never constructed. This target is built with
    JUCE_WEB_BROWSER=0 and does not compile the editor TU, matching the
    render-harness rule that keeps a Stage-3 WebView swap from breaking a
    console target (pattern_render_harness_breaks_on_webview_editor).

  ==============================================================================
*/

#include <JuceHeader.h>

#include <iostream>

// Every plugin in this repo defines this at the foot of PluginProcessor.cpp.
// juce_audio_plugin_client (which normally declares it) is deliberately NOT
// linked into this console app, so declare it here.
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{

// TSV is only unambiguous if no cell can contain a tab or a newline.
juce::String tsvEscape (const juce::String& in)
{
    return in.replace ("\\", "\\\\")
             .replace ("\t", "\\t")
             .replace ("\r", "\\r")
             .replace ("\n", "\\n");
}

juce::String flagsOf (const juce::AudioProcessorParameter& p)
{
    juce::StringArray f;

    if (p.isAutomatable())          f.add ("automatable");
    if (p.isDiscrete())             f.add ("discrete");
    if (p.isBoolean())              f.add ("boolean");
    if (p.isMetaParameter())        f.add ("meta");
    if (p.isOrientationInverted())  f.add ("inverted");

    return f.joinIntoString (",");
}

juce::String idOf (juce::AudioProcessorParameter& p)
{
    // AudioProcessorParameterWithID (what APVTS creates) derives from
    // HostedAudioProcessorParameter, which is where getParameterID() lives.
    // A parameter that is neither still has an index, and reporting an empty
    // cell would look like a successful read of an empty ID.
    if (auto* hosted = dynamic_cast<juce::HostedAudioProcessorParameter*> (&p))
        return hosted->getParameterID();

    return "<no-id>";
}

} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::unique_ptr<juce::AudioProcessor> proc { createPluginFilter() };

    if (proc == nullptr)
    {
        std::cerr << "param-dump: createPluginFilter() returned nullptr\n";
        return 1;
    }

    const auto& params = proc->getParameters();

    std::cout << "# plugin\t"  << tsvEscape (proc->getName()) << "\n";
    std::cout << "# params\t"  << params.size() << "\n";
    std::cout << "# note\tdefaultNorm is NORMALISED 0..1; numSteps 2147483647 means continuous\n";
    std::cout << "#id\tname\tlabel\tnumSteps\ttextAtMin\ttextAtMax\tdefaultNorm\tdefaultText\tflags\n";

    for (auto* p : params)
    {
        if (p == nullptr)
            continue;

        const auto def = p->getDefaultValue();

        std::cout << tsvEscape (idOf (*p))                    << '\t'
                  << tsvEscape (p->getName (128))             << '\t'
                  << tsvEscape (p->getLabel())                << '\t'
                  << p->getNumSteps()                         << '\t'
                  << tsvEscape (p->getText (0.0f, 64))        << '\t'
                  << tsvEscape (p->getText (1.0f, 64))        << '\t'
                  << juce::String (def, 6).toStdString()      << '\t'
                  << tsvEscape (p->getText (def, 64))         << '\t'
                  << tsvEscape (flagsOf (*p))                 << '\n';
    }

    std::cout.flush();
    return 0;
}
