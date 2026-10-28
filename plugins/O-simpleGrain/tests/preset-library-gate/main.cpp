/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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

    O-simpleGrain v1.9.0 — preset / sample-library gate.

    The preset and library calls are WebView native functions in the plugin, so
    pedalboard cannot reach them; this console app drives the same processor
    methods the native functions forward to.

    RUN IT IN A THROWAWAY HOME. NSHomeDirectory() honours CFFIXED_USER_HOME, so:
        CFFIXED_USER_HOME=$(mktemp -d) ./O-simpleGrain-library-gate
    The gate REFUSES to run when the library folder resolves under the real
    home — it creates, renames and deletes files there.

    Sections (each verdict prints PASS/FAIL; exit code = number of FAILs):
      A  library add: a drop is copied in (FLAC for an integer WAV); the same
         bytes again add nothing; a different sound with the same name gets " 2".
      B  preset save: the JSON carries no sourceSample / outputLevel, and its
         sample reference names the library entry with the entry's own hash.
      C  preset load: non-excluded params come back (and unsaved ones reset to
         default), Output Level and the built-in choice are untouched, the sample
         is fetched from the library; Sample Lock keeps the sound; a built-in
         reference loads the built-in and moves the choice.
      D  resolution: a renamed entry is found by content; a deleted one keeps the
         current sound, still applies the params, and reports the name; a stray
         sourceSample / outputLevel key in a hand-edited preset is ignored.
      E  sessions: presetName + sampleLock round-trip; a pre-1.9.0 session
         (no such properties, a dropped source embedded) restores its sound and
         defaults the new state; a library sound restores from the library and,
         with the entry gone, from the session's embedded copy.
      F  a lesson clears the preset name; delete removes the file.

  ==============================================================================
*/

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>

namespace PID = OSimpleGrain::ParamIDs;

static int fails = 0;
static void verdict (bool ok, const juce::String& msg)
{
    fails += ok ? 0 : 1;
    std::printf ("%s %s\n", ok ? "PASS" : "FAIL", msg.toRawUTF8());
}

static void setReal (OSimpleGrainAudioProcessor& p, const char* id, float real)
{
    auto* par = p.getAPVTS().getParameter (id);
    par->setValueNotifyingHost (par->convertTo0to1 (real));
}

static float getReal (OSimpleGrainAudioProcessor& p, const char* id)
{
    auto* par = p.getAPVTS().getParameter (id);
    return par->convertFrom0to1 (par->getValue());
}

static void pump (int ms = 150)
{
    if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
        mm->runDispatchLoopUntil (ms);
}

// A 16-bit WAV (integer -> the library copy is FLAC) of a sine at `hz`.
static juce::MemoryBlock makeWav (double hz, double seconds, double fs = 48000.0)
{
    juce::AudioBuffer<float> buf (1, (int) (seconds * fs));
    for (int i = 0; i < buf.getNumSamples(); ++i)
        buf.setSample (0, i, 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * hz * i / fs));
    juce::MemoryBlock mb;
    {
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> os = std::make_unique<juce::MemoryOutputStream> (mb, false);
        auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}
                                              .withSampleRate (fs).withNumChannels (1)
                                              .withBitsPerSample (16));
        w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
    }
    return mb;
}

static bool drop (OSimpleGrainAudioProcessor& p, const juce::String& name, const juce::MemoryBlock& wav)
{
    const auto b64 = juce::Base64::toBase64 (wav.getData(), wav.getSize());
    const juce::String sid = "gate-" + juce::String (juce::Random::getSystemRandom().nextInt());
    return p.dropSessionStart (sid, {}) && p.dropSessionCommitFile (sid, name, b64);
}

static juce::StringArray entries (OSimpleGrainAudioProcessor& p)
{
    juce::StringArray out;
    const auto info = p.getLibraryInfo();                // keep the var alive while iterating
    const auto list = info.getProperty ("entries", {});
    if (auto* arr = list.getArray())
        for (auto& v : *arr)
            out.add (v.toString());
    return out;
}

static juce::String libCurrent (OSimpleGrainAudioProcessor& p)
{
    return p.getLibraryInfo().getProperty ("current", {}).toString();
}

static int sourceLen (OSimpleGrainAudioProcessor& p)
{
    return (int) p.getSourceThumbnail (64).size();   // 0 = no source
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const double fs = 48000.0;

    auto make = [fs]() {
        auto p = std::make_unique<OSimpleGrainAudioProcessor>();
        p->setPlayConfigDetails (0, 2, fs, 512);
        p->prepareToPlay (fs, 512);
        pump();
        return p;
    };

    auto proc = make();
    auto& p = *proc;

    // ── safety: never touch the real library ───────────────────────────────
    const auto lib = p.getSampleLibraryFolder();
    const char* fixedHome = std::getenv ("CFFIXED_USER_HOME");
    // Compared by the home's own (random) folder name: NSHomeDirectory() may hand
    // back /tmp/... for a /private/tmp/... CFFIXED_USER_HOME.
    if (fixedHome == nullptr
        || ! lib.getFullPathName().contains ("/" + juce::File (juce::String (fixedHome)).getFileName() + "/Library/"))
    {
        std::printf ("ABORT: library folder %s is not under CFFIXED_USER_HOME — run in a throwaway home\n",
                     lib.getFullPathName().toRawUTF8());
        return 99;
    }
    const auto presetsDir = lib.getParentDirectory().getChildFile ("Presets").getChildFile ("User");
    std::printf ("library: %s\npresets: %s\n", lib.getFullPathName().toRawUTF8(),
                 presetsDir.getFullPathName().toRawUTF8());

    verdict (entries (p).isEmpty(), "fresh instance: library empty, construction wrote nothing ("
                                    + juce::String (lib.exists() ? "folder exists" : "no folder") + ")");
    verdict (! lib.getParentDirectory().exists(), "construction + prepare create no product folder (auval-safe)");

    // ── A: library add ─────────────────────────────────────────────────────
    const auto toneA = makeWav (440.0, 1.0);
    const auto toneB = makeWav (660.0, 1.0);
    verdict (drop (p, "tone.wav", toneA), "A drop tone.wav decodes");
    auto e = entries (p);
    verdict (e == juce::StringArray { "tone.flac" }, "A library holds tone.flac after the drop: [" + e.joinIntoString (", ") + "]");
    verdict (libCurrent (p) == "tone.flac", "A the live sound is named as library entry tone.flac: '" + libCurrent (p) + "'");
    verdict (p.getSourceIdentity() == "dropped:tone.wav", "A the source identity is unchanged by the copy (dropped:tone.wav)");
    verdict (drop (p, "tone-again.wav", toneA) && entries (p).size() == 1 && libCurrent (p) == "tone.flac",
             "A the same bytes under another name add nothing and resolve to tone.flac");
    verdict (drop (p, "tone.wav", toneB) && entries (p) == juce::StringArray { "tone 2.flac", "tone.flac" },
             "A a different sound named tone.wav becomes 'tone 2.flac': [" + entries (p).joinIntoString (", ") + "]");

    // ── B: save ────────────────────────────────────────────────────────────
    verdict (p.loadLibrarySample ("tone.flac") && libCurrent (p) == "tone.flac"
                 && p.getSourceIdentity() == lib.getChildFile ("tone.flac").getFullPathName(),
             "B loading library entry tone.flac: identity = its path");
    setReal (p, PID::grainSize, 80.0f);
    setReal (p, PID::density, 20.0f);
    setReal (p, PID::grainTilt, -40.0f);
    setReal (p, PID::grainSkip, 25.0f);
    setReal (p, PID::outputLevel, -20.0f);
    auto r = p.saveUserPreset ("Alpha");
    verdict ((bool) r.getProperty ("ok", false) && r.getProperty ("name", {}).toString() == "Alpha", "B saveUserPreset Alpha ok");
    const auto alphaFile = presetsDir.getChildFile ("Alpha.json");
    auto alpha = juce::JSON::parse (alphaFile.loadFileAsString());
    auto* params = alpha.getProperty ("parameters", {}).getDynamicObject();
    verdict (params != nullptr && ! params->hasProperty (PID::sourceSample) && ! params->hasProperty (PID::outputLevel)
                 && params->hasProperty (PID::grainSize) && params->hasProperty (PID::grainSkip),
             "B preset JSON has grainSize/grainSkip, no sourceSample, no outputLevel");
    auto sample = alpha.getProperty ("customState", {}).getProperty ("sample", {});
    verdict (sample.getProperty ("kind", {}).toString() == "library" && sample.getProperty ("name", {}).toString() == "tone.flac"
                 && sample.getProperty ("hash", {}).toString() == SampleLibrary::hashOf (lib.getChildFile ("tone.flac"))
                 && sample.getProperty ("size", {}).toString().getLargeIntValue() == lib.getChildFile ("tone.flac").getSize(),
             "B sample reference = { library, tone.flac, hash + size of the entry }: " + juce::JSON::toString (sample, true));
    verdict (alphaFile.getSize() < 4096, "B the preset is small (" + juce::String (alphaFile.getSize()) + " bytes): no audio inside");

    // A built-in preset.
    setReal (p, PID::sourceSample, 3.0f);     // piano
    pump();
    verdict (p.getSourceIdentity() == "embedded:piano", "B built-in piano selected");
    setReal (p, PID::grainSize, 15.0f);
    p.saveUserPreset ("Beta");
    auto beta = juce::JSON::parse (presetsDir.getChildFile ("Beta.json").loadFileAsString());
    auto bs = beta.getProperty ("customState", {}).getProperty ("sample", {});
    verdict (bs.getProperty ("kind", {}).toString() == "builtin" && bs.getProperty ("name", {}).toString() == "piano",
             "B Beta's reference = { builtin, piano }");

    // ── C: load ────────────────────────────────────────────────────────────
    setReal (p, PID::sourceSample, 2.0f);     // water
    pump();
    setReal (p, PID::grainSize, 33.0f);
    setReal (p, PID::scan, 50.0f);            // Alpha was saved at Scan 0 (default)
    setReal (p, PID::outputLevel, -3.0f);
    r = p.loadUserPreset ("Alpha");
    pump();
    verdict ((bool) r.getProperty ("ok", false) && r.getProperty ("missing", {}).toString().isEmpty(), "C load Alpha ok, nothing missing");
    verdict (std::abs (getReal (p, PID::grainSize) - 80.0f) < 0.05f && std::abs (getReal (p, PID::density) - 20.0f) < 0.05f
                 && std::abs (getReal (p, PID::grainTilt) + 40.0f) < 0.05f && std::abs (getReal (p, PID::grainSkip) - 25.0f) < 0.05f,
             "C Alpha's controls are back (size " + juce::String (getReal (p, PID::grainSize), 2) + ", density "
                 + juce::String (getReal (p, PID::density), 2) + ", tilt " + juce::String (getReal (p, PID::grainTilt), 2) + ")");
    verdict (std::abs (getReal (p, PID::scan)) < 1e-4f, "C Scan (moved after the save) reset to its default: " + juce::String (getReal (p, PID::scan), 3));
    verdict (std::abs (getReal (p, PID::outputLevel) + 3.0f) < 0.05f, "C Output Level untouched (-3 dB): " + juce::String (getReal (p, PID::outputLevel), 2));
    verdict (p.getSourceIdentity() == lib.getChildFile ("tone.flac").getFullPathName() && libCurrent (p) == "tone.flac",
             "C the sample came back from the library: " + p.getSourceIdentity());
    verdict ((int) getReal (p, PID::sourceSample) == 2, "C the built-in choice is untouched (water)");

    // Sample Lock.
    setReal (p, PID::sourceSample, 1.0f);     // voice
    pump();
    p.setSampleLock (true);
    r = p.loadUserPreset ("Alpha");
    pump();
    verdict (p.getSourceIdentity() == "embedded:voice" && std::abs (getReal (p, PID::grainSize) - 80.0f) < 0.05f,
             "C Sample Lock on: Alpha's controls apply, the sound stays voice (" + p.getSourceIdentity() + ")");
    r = p.loadUserPreset ("Beta");
    pump();
    verdict (p.getSourceIdentity() == "embedded:voice", "C Sample Lock on: Beta's piano reference is ignored");
    p.setSampleLock (false);

    // Built-in reference from a library sound.
    p.loadLibrarySample ("tone 2.flac");
    r = p.loadUserPreset ("Beta");
    pump();
    verdict (p.getSourceIdentity() == "embedded:piano" && (int) getReal (p, PID::sourceSample) == 3
                 && std::abs (getReal (p, PID::grainSize) - 15.0f) < 0.05f,
             "C Beta from a library sound: piano loaded, the choice moved to piano, size 15");
    const int pianoLen = sourceLen (p);
    r = p.loadUserPreset ("Beta");
    pump();
    verdict (p.getSourceIdentity() == "embedded:piano" && sourceLen (p) == pianoLen, "C re-loading Beta keeps piano (no reload needed)");

    // Navigation.
    auto s1 = p.stepUserPreset (1).getProperty ("name", {}).toString();
    auto s2 = p.stepUserPreset (1).getProperty ("name", {}).toString();
    verdict (s1 == "Alpha" && s2 == "Beta", "C next from Beta wraps to Alpha, then Beta: " + s1 + ", " + s2);

    // ── D: resolution ──────────────────────────────────────────────────────
    lib.getChildFile ("tone.flac").moveFileTo (lib.getChildFile ("renamed.flac"));
    r = p.loadUserPreset ("Alpha");
    pump();
    verdict (r.getProperty ("missing", {}).toString().isEmpty() && libCurrent (p) == "renamed.flac",
             "D a renamed entry is found by content: " + libCurrent (p));

    setReal (p, PID::sourceSample, 0.0f);     // fire
    pump();
    lib.getChildFile ("renamed.flac").deleteFile();
    setReal (p, PID::grainSize, 44.0f);
    r = p.loadUserPreset ("Alpha");
    pump();
    verdict ((bool) r.getProperty ("ok", false) && r.getProperty ("missing", {}).toString() == "tone.flac"
                 && p.getSourceIdentity() == "embedded:fire" && std::abs (getReal (p, PID::grainSize) - 80.0f) < 0.05f,
             "D a deleted entry: params applied, fire kept, missing = '" + r.getProperty ("missing", {}).toString() + "'");

    // A hand-edited preset carrying the excluded keys.
    {
        auto obj = juce::JSON::parse (alphaFile.loadFileAsString());
        auto* prm = obj.getProperty ("parameters", {}).getDynamicObject();
        prm->setProperty (PID::sourceSample, 1.0f);
        prm->setProperty (PID::outputLevel, 1.0f);
        presetsDir.getChildFile ("Stray.json").replaceWithText (juce::JSON::toString (obj, true));
    }
    setReal (p, PID::outputLevel, -12.0f);
    p.loadUserPreset ("Stray");
    pump();
    verdict (std::abs (getReal (p, PID::outputLevel) + 12.0f) < 0.05f && (int) getReal (p, PID::sourceSample) == 0,
             "D stray sourceSample / outputLevel keys in a preset file are ignored");

    // A user sound from an OLD session (not in the library) is added on save.
    {
        juce::MemoryBlock state;
        auto fresh = make();
        drop (*fresh, "old.wav", makeWav (330.0, 0.5));
        // Forget the library copy, then save: the save must put it back.
        const auto entry = libCurrent (*fresh);
        lib.getChildFile (entry).deleteFile();
        fresh->saveUserPreset ("Gamma");
        auto g = juce::JSON::parse (presetsDir.getChildFile ("Gamma.json").loadFileAsString())
                     .getProperty ("customState", {}).getProperty ("sample", {});
        verdict (g.getProperty ("kind", {}).toString() == "library" && lib.getChildFile (g.getProperty ("name", {}).toString()).existsAsFile(),
                 "D saving with a sound missing from the library re-adds it: " + g.getProperty ("name", {}).toString());
    }

    // ── E: sessions ────────────────────────────────────────────────────────
    {
        p.loadUserPreset ("Beta");
        pump();
        p.setSampleLock (true);
        juce::MemoryBlock st;
        p.getStateInformation (st);
        auto q = make();
        q->setStateInformation (st.getData(), (int) st.getSize());
        pump();
        auto info = q->getPresetInfo();
        verdict (info.getProperty ("current", {}).toString() == "Beta" && q->getSampleLock(),
                 "E presetName (Beta) and Sample Lock (on) round-trip through the session");
        p.setSampleLock (false);
    }
    {
        // Pre-1.9.0 session: no presetName / sampleLock, a dropped source embedded.
        auto src = make();
        drop (*src, "legacy.wav", makeWav (550.0, 0.75));
        juce::MemoryBlock st;
        src->getStateInformation (st);
        auto xml = juce::AudioProcessor::getXmlFromBinary (st.getData(), (int) st.getSize());
        xml->removeAttribute ("presetName");
        xml->removeAttribute ("sampleLock");
        juce::MemoryBlock old;
        juce::AudioProcessor::copyXmlToBinary (*xml, old);
        lib.getChildFile (libCurrent (*src)).deleteFile();   // the library knows nothing of it

        auto q = make();
        q->setSampleLock (true);                    // must reset to off
        q->setStateInformation (old.getData(), (int) old.getSize());
        pump();
        verdict (q->getSourceIdentity() == "dropped:legacy.wav" && sourceLen (*q) > 0 && q->takeRestoreFailedName().isEmpty(),
                 "E a pre-1.9.0 session restores its embedded dropped sound (" + q->getSourceIdentity() + ")");
        verdict (! q->getSampleLock() && q->getPresetInfo().getProperty ("current", {}).toString().isEmpty(),
                 "E a pre-1.9.0 session: Sample Lock off, no preset named");
    }
    {
        // Library sound: from the library, then from the embedded copy once the entry is gone.
        auto src = make();
        drop (*src, "keep.wav", makeWav (770.0, 0.5));
        const auto entry = libCurrent (*src);
        src->loadLibrarySample (entry);
        juce::MemoryBlock st;
        src->getStateInformation (st);
        auto q = make();
        q->setStateInformation (st.getData(), (int) st.getSize());
        pump();
        verdict (libCurrent (*q) == entry, "E a library sound restores from the library (" + entry + ")");
        lib.getChildFile (entry).deleteFile();
        auto q2 = make();
        q2->setStateInformation (st.getData(), (int) st.getSize());
        pump();
        verdict (q2->getSourceIdentity() == lib.getChildFile (entry).getFullPathName() && sourceLen (*q2) > 0
                     && q2->takeRestoreFailedName().isEmpty(),
                 "E with the entry deleted, the session's embedded copy restores it (no fallback)");
    }

    // ── F: lesson + delete ─────────────────────────────────────────────────
    p.loadUserPreset ("Alpha");
    p.applyFactoryPreset ("Smooth Cloud");
    verdict (p.getPresetInfo().getProperty ("current", {}).toString().isEmpty(), "F a lesson clears the preset name");
    verdict (p.deleteUserPreset ("Stray") && ! presetsDir.getChildFile ("Stray.json").exists(), "F delete removes the file");
    verdict (! p.deleteUserPreset ("Nope"), "F deleting an unknown preset fails");
    verdict (p.removeLibrarySample ("tone 2.flac") && ! entries (p).contains ("tone 2.flac"), "F remove a library entry");
    verdict (! p.removeLibrarySample ("../Presets/User/Alpha.json") && alphaFile.existsAsFile(),
             "F a path-like library name is refused");

    std::printf ("\n%d FAIL\n", fails);
    return fails;
}
