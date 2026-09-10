// O-Strata Stage 1 headless smoke — verify-phase evidence for the SUMMARY's
// "manual smoke left for verify" items that do not need a window:
//   [1] a held note sounds on Osc A alone and on Osc B alone (sine placeholder
//       through the unchanged voice/FX path), with the other oscillator's level
//       as the negative control
//   [2] TuningEngine::loadScalaFile() loads test-tunings/just-major.scl
//   [3] state round-trip: all 205 params + tuning + uiLanguage restored into a
//       FRESH processor; the XML carries an empty <terrainImports/> child
//   [4] (second pass, CONTEXT D3) a mod slot routed to a NEW destination
//       (OscA Terrain Freq, index 31) leaves the sine placeholder sample-identical,
//       with LFO1 -> Pitch as the positive control; getModDestNames() == 46
//   [5] (second pass, CONTEXT D2) the on-disk factory bank written by the first
//       construction is exactly Factory/Init/Init.json (198 keys) + .factory-version
//   [6] (second pass) choice lists and the exact-log TerFreq range, in-process
// Literals (205 / 198 / 31 / 46) are asserted ALONGSIDE the live values so a
// later spec drift fails loudly (memory pattern_test_fixture_mirrors_drift_silently).
// Links the O-Strata-param-dump objects (JUCE_WEB_BROWSER=0, no editor TU).

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "StrataParamIds.h"
#include "dsp/ModulationMatrix.h"
#include <cmath>
#include <cstdio>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

static int failures = 0;
static void check (bool ok, const char* what)
{
    std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (! ok) ++failures;
}

static juce::AudioProcessorParameterWithID* findParam (juce::AudioProcessor& p, const juce::String& id)
{
    for (auto* prm : p.getParameters())
        if (auto* w = dynamic_cast<juce::AudioProcessorParameterWithID*> (prm))
            if (w->getParameterID() == id)
                return w;
    return nullptr;
}

static void setNorm (juce::AudioProcessor& p, const juce::String& id, float v)
{
    auto* prm = findParam (p, id);
    jassert (prm != nullptr);
    prm->setValueNotifyingHost (v);
}

// Render a held middle C for `blocks` blocks and return RMS over the second half.
static double renderNoteRms (juce::AudioProcessor& p, int blocks, bool& sawNaN)
{
    const double sr = 48000.0; const int bs = 512;
    p.setPlayConfigDetails (0, 2, sr, bs);
    p.prepareToPlay (sr, bs);
    juce::AudioBuffer<float> buf (2, bs);
    juce::MidiBuffer midi;
    double sum = 0.0; long n = 0; sawNaN = false;
    for (int b = 0; b < blocks; ++b)
    {
        midi.clear();
        if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
        buf.clear();
        p.processBlock (buf, midi);
        if (b >= blocks / 2)
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < bs; ++i)
                {
                    const float s = buf.getSample (ch, i);
                    if (! std::isfinite (s)) sawNaN = true;
                    sum += (double) s * s; ++n;
                }
    }
    p.releaseResources();
    return std::sqrt (sum / (double) juce::jmax (1L, n));
}

// Render a held middle C for `blocks` blocks and return every sample (both channels).
static std::vector<float> renderNoteSamples (juce::AudioProcessor& p, int blocks)
{
    const double sr = 48000.0; const int bs = 512;
    p.setPlayConfigDetails (0, 2, sr, bs);
    p.prepareToPlay (sr, bs);
    juce::AudioBuffer<float> buf (2, bs);
    juce::MidiBuffer midi;
    std::vector<float> out; out.reserve ((size_t) (2 * bs * blocks));
    for (int b = 0; b < blocks; ++b)
    {
        midi.clear();
        if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
        buf.clear();
        p.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < bs; ++i)
                out.push_back (buf.getSample (ch, i));
    }
    p.releaseResources();
    return out;
}

static float maxAbsDiff (const std::vector<float>& a, const std::vector<float>& b)
{
    if (a.size() != b.size()) return 1.0e9f;
    float m = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) m = juce::jmax (m, std::abs (a[i] - b[i]));
    return m;
}

static juce::String u8 (const char* bytes) { return juce::String (juce::CharPointer_UTF8 (bytes)); }

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File scl = juce::File::getCurrentWorkingDirectory().getChildFile ("test-tunings/just-major.scl");
    std::printf ("scl: %s (exists=%d)\n", scl.getFullPathName().toRawUTF8(), (int) scl.existsAsFile());

    // ── [1] notes sound ────────────────────────────────────────────────────
    {
        bool nanA = false, nanB = false, nanA0 = false, nanB0 = false;

        std::unique_ptr<juce::AudioProcessor> pA (createPluginFilter());
        setNorm (*pA, "oscMix", 0.0f);                  // A only
        const double rmsA = renderNoteRms (*pA, 80, nanA);

        std::unique_ptr<juce::AudioProcessor> pA0 (createPluginFilter());
        setNorm (*pA0, "oscMix", 0.0f); setNorm (*pA0, "oscALevel", 0.0f);   // A only, A muted
        const double rmsA0 = renderNoteRms (*pA0, 80, nanA0);

        std::unique_ptr<juce::AudioProcessor> pB (createPluginFilter());
        setNorm (*pB, "oscMix", 1.0f); setNorm (*pB, "oscBLevel", 0.8f);   // B only (oscBLevel defaults to 0, as in O-Prism)
        const double rmsB = renderNoteRms (*pB, 80, nanB);

        std::unique_ptr<juce::AudioProcessor> pB0 (createPluginFilter());
        setNorm (*pB0, "oscMix", 1.0f); setNorm (*pB0, "oscBLevel", 0.0f);   // B only, B muted (the default)
        const double rmsB0 = renderNoteRms (*pB0, 80, nanB0);

        std::printf ("rms  A=%.4f  A(muted)=%.4f  B=%.4f  B(muted)=%.4f\n", rmsA, rmsA0, rmsB, rmsB0);
        check (rmsA > 0.01, "[1] held note sounds with oscMix=0 (Osc A)");
        check (rmsB > 0.01, "[1] held note sounds with oscMix=1 (Osc B)");
        check (rmsA0 < rmsA * 0.1, "[1] negative control: muting Osc A level kills the A-only note");
        check (rmsB0 < rmsB * 0.1, "[1] negative control: muting Osc B level kills the B-only note");
        check (! (nanA || nanB || nanA0 || nanB0), "[1] no NaN/Inf in rendered output");
    }

    // ── [2] Scala load ─────────────────────────────────────────────────────
    {
        std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
        auto* strata = dynamic_cast<OStrataAudioProcessor*> (p.get());
        check (strata != nullptr, "[2] createPluginFilter() yields OStrataAudioProcessor");
        auto* te = strata->getTuningEngine();
        const double f60Before = te->getFrequency (60), f64Before = te->getFrequency (64);
        const bool loaded = te->loadScalaFile (scl);
        const double f60 = te->getFrequency (60), f62 = te->getFrequency (62), f64 = te->getFrequency (64), f67 = te->getFrequency (67);
        std::printf ("scala: before 64/60=%.6f  after 62/60=%.6f 64/60=%.6f 67/60=%.6f  (f60=%.3f Hz; one key per scale degree)\n",
                     f64Before / f60Before, f62 / f60, f64 / f60, f67 / f60, f60);
        check (loaded, "[2] TuningEngine::loadScalaFile(just-major.scl) returns true");
        check (std::abs (f62 / f60 - 1.25) < 1e-3, "[2] note 62 / note 60 = 5/4 (degree 2) after load");
        check (std::abs (f64 / f60 - 1.5) < 1e-3, "[2] note 64 / note 60 = 3/2 (degree 4) after load");
        check (std::abs (f67 / f60 - 2.0) < 1e-3, "[2] note 67 / note 60 = 2/1 (degree 7) after load");
    }

    // ── [3] state round-trip ───────────────────────────────────────────────
    {
        std::unique_ptr<juce::AudioProcessor> p1 (createPluginFilter());
        auto* s1 = dynamic_cast<OStrataAudioProcessor*> (p1.get());
        // deterministic pseudo-random value on every parameter
        juce::uint32 seed = 0x5EED1234u;
        std::vector<float> want;
        for (auto* prm : p1->getParameters())
        {
            seed = seed * 1664525u + 1013904223u;
            const float v = (float) (seed >> 8) / (float) (1u << 24);
            auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm);
            const juce::String id = r != nullptr ? r->getParameterID() : juce::String();
            // The five tuning params are applied to the engine by the audio thread /
            // setStateInformation; P1 never runs processBlock, so leave them at default
            // to keep P1's engine and P2's restored engine comparable.
            const bool tuningParam = id == "tuningPreset" || id == "tonic" || id == "masterTune"
                                  || id == "octaveStretch" || id == "pitchBendRange";
            if (! tuningParam) prm->setValueNotifyingHost (v);
            // The contract is the value AFTER the parameter's own snap (a bool stores the
            // raw float until it round-trips through convertFrom0to1/convertTo0to1).
            const float raw = prm->getValue();
            want.push_back (r != nullptr ? r->convertTo0to1 (r->convertFrom0to1 (raw)) : raw);
        }
        s1->uiLanguage.store (1);
        check (s1->getTuningEngine()->loadScalaFile (scl), "[3] scala loaded into P1 before save");

        juce::MemoryBlock block;
        p1->getStateInformation (block);
        check (block.getSize() > 9, "[3] getStateInformation produced data");

        // copyXmlToBinary layout: int32 magic, int32 len, xml text, NUL
        const char* txt = static_cast<const char*> (block.getData()) + 8;
        const auto len = (int) block.getSize() - 9;
        std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (juce::String::fromUTF8 (txt, len)));
        check (xml != nullptr, "[3] state XML parses");
        const auto* geo = xml != nullptr ? xml->getChildByName ("terrainImports") : nullptr;
        check (geo != nullptr, "[3] <terrainImports> child present");
        check (geo != nullptr && geo->getNumChildElements() == 0 && geo->getNumAttributes() == 0,
               "[3] <terrainImports/> is empty");
        check (xml != nullptr && xml->getChildByName ("tuningEngine") != nullptr, "[3] <tuningEngine> child present");
        check (xml != nullptr && xml->getStringAttribute ("uiLanguage") == "fr", "[3] uiLanguage=\"fr\" stored as a string");
        int paramNodes = 0;
        if (xml != nullptr) for (auto* c : xml->getChildIterator()) if (c->hasTagName ("PARAM")) ++paramNodes;
        std::printf ("state: %d bytes, %d PARAM nodes, root=%s\n", (int) block.getSize(), paramNodes,
                     xml != nullptr ? xml->getTagName().toRawUTF8() : "?");
        check (paramNodes == 205 && paramNodes == p1->getParameters().size(), "[3] XML carries 205 PARAM nodes (== live getParameters().size())");

        std::unique_ptr<juce::AudioProcessor> p2 (createPluginFilter());
        auto* s2 = dynamic_cast<OStrataAudioProcessor*> (p2.get());
        p2->setStateInformation (block.getData(), (int) block.getSize());

        int mismatches = 0, differsFromDefault = 0; size_t i = 0;
        const auto& params2 = p2->getParameters();
        check (params2.size() == 205 && want.size() == 205 && params2.size() == p2->getParameters().size(), "[3] both instances expose 205 parameters (literal == live count)");
        for (auto* prm : params2)
        {
            const float got = prm->getValue();
            if (std::abs (got - want[i]) > 1e-6f)
            {
                if (mismatches < 5)
                    std::printf ("  mismatch %s want=%.6f got=%.6f\n",
                                 dynamic_cast<juce::AudioProcessorParameterWithID*> (prm)->getParameterID().toRawUTF8(),
                                 want[i], got);
                ++mismatches;
            }
            if (std::abs (want[i] - prm->getDefaultValue()) > 1e-6f) ++differsFromDefault;
            ++i;
        }
        std::printf ("round-trip: %d mismatches; %d of %d saved values differ from default\n", mismatches, differsFromDefault, (int) want.size());
        check (mismatches == 0, "[3] all 205 parameter values restored into a fresh processor");
        check (differsFromDefault > 150, "[3] the test actually moved the parameters (>150 of the 200 randomised = 205 - 5 tuning IDs differ from default; not a vacuous pass)");
        check (s2->uiLanguage.load() == 1, "[3] uiLanguage restored (fr)");
        auto* t1 = s1->getTuningEngine(); auto* t2 = s2->getTuningEngine();
        bool tuningEqual = true;
        for (int n = 48; n <= 84; ++n)
            if (std::abs (t1->getFrequency (n) - t2->getFrequency (n)) > 1e-6) tuningEqual = false;
        std::printf ("tuning: P1 64/60=%.6f  P2 64/60=%.6f\n",
                     t1->getFrequency (64) / t1->getFrequency (60), t2->getFrequency (64) / t2->getFrequency (60));
        check (tuningEqual, "[3] tuning (just-major) restored: notes 48..84 identical in P1 and P2");
        check (std::abs (t2->getFrequency (64) / t2->getFrequency (60) - 1.5) < 1e-3, "[3] restored tuning is the Scala scale (64/60 = 3/2), not 12-EDO");
    }


    // ── [4] D3: a slot routed to a NEW destination is inert (with positive control) ──
    {
        const auto dests = getModDestNames();
        const int nDests = dests.size();
        check (nDests == 46 && nDests == static_cast<int> (ModDest::NumDests), "[4] getModDestNames().size() == 46 == ModDest::NumDests");
        check (dests[1] == "OscA Orbit Size" && dests[2] == "OscB Orbit Size", "[4] destinations 1/2 relabelled OscA/OscB Orbit Size");
        check (dests[26] == "OscA Orbit Aspect", "[4] destination 26 == OscA Orbit Aspect (first appended)");
        check (dests[31] == "OscA Terrain Freq", "[4] destination 31 == OscA Terrain Freq");
        check (dests[45] == "OscB Saturation", "[4] destination 45 == OscB Saturation (last)");
        {
            std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
            auto* dst = dynamic_cast<juce::AudioParameterChoice*> (findParam (*p, "modSlot0Dst"));
            check (dst != nullptr && dst->choices.size() == 46 && dst->choices == dests, "[4] modSlot0Dst exposes the same 46 choices");
        }
        const int kBlocks = 40;
        // osc?Phase == 0 means "random start phase" (StrataVoice.cpp: resetWithRandomPhases seeds
        // from the oscillator's address), so pin a deterministic start phase on every instance.
        auto pinPhase = [] (juce::AudioProcessor& p) { setNorm (p, "oscMix", 0.0f); setNorm (p, "oscAPhase", 0.25f); setNorm (p, "oscBPhase", 0.25f); };
        const float srcLfo1 = 1.0f / 10.0f;                       // 11 sources -> index 1
        const float dstTerFreq = 31.0f / (float) (nDests - 1);    // OscA Terrain Freq
        const float dstPitch   = 23.0f / (float) (nDests - 1);    // Pitch (positive control)

        std::unique_ptr<juce::AudioProcessor> p0 (createPluginFilter());
        pinPhase (*p0);
        const auto ref = renderNoteSamples (*p0, kBlocks);

        std::unique_ptr<juce::AudioProcessor> p0b (createPluginFilter());
        pinPhase (*p0b);
        const auto ref2 = renderNoteSamples (*p0b, kBlocks);
        check (maxAbsDiff (ref, ref2) == 0.0f, "[4] placeholder render is deterministic (two unrouted renders identical)");

        std::unique_ptr<juce::AudioProcessor> pNeg (createPluginFilter());
        pinPhase (*pNeg);
        setNorm (*pNeg, "modSlot0Src", srcLfo1); setNorm (*pNeg, "modSlot0Dst", dstTerFreq);
        setNorm (*pNeg, "modSlot0Amt", 1.0f);    setNorm (*pNeg, "modSlot0On", 1.0f);
        check (dynamic_cast<juce::AudioParameterChoice*> (findParam (*pNeg, "modSlot0Dst"))->getIndex() == 31, "[4] modSlot0Dst index == 31 after setNorm");
        const auto neg = renderNoteSamples (*pNeg, kBlocks);

        std::unique_ptr<juce::AudioProcessor> pPos (createPluginFilter());
        pinPhase (*pPos);
        setNorm (*pPos, "modSlot0Src", srcLfo1); setNorm (*pPos, "modSlot0Dst", dstPitch);
        setNorm (*pPos, "modSlot0Amt", 1.0f);    setNorm (*pPos, "modSlot0On", 1.0f);
        const auto pos = renderNoteSamples (*pPos, kBlocks);

        const float dNeg = maxAbsDiff (ref, neg), dPos = maxAbsDiff (ref, pos);
        std::printf ("route: LFO1->OscA Terrain Freq max|d|=%.3e   LFO1->Pitch max|d|=%.3e  (%d samples)\n", dNeg, dPos, (int) ref.size());
        check (dNeg == 0.0f, "[4] LFO1 -> OscA Terrain Freq (index 31) leaves the placeholder render sample-identical (D3)");
        check (dPos > 1.0e-3f, "[4] positive control: LFO1 -> Pitch (index 23) changes the render (route check is not vacuous)");
    }

    // ── [5] D2: on-disk factory bank is exactly Init ───────────────────────
    {
        const juce::File presets = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                                       .getChildFile ("Library/O-Strata/Presets");
        const juce::File factory = presets.getChildFile ("Factory");
        juce::Array<juce::File> jsons;
        factory.findChildFiles (jsons, juce::File::findFiles, true, "*.json");
        juce::Array<juce::File> allFiles;
        presets.findChildFiles (allFiles, juce::File::findFiles, true, "*");
        std::printf ("bank: %s  json=%d  files=%d\n", presets.getFullPathName().toRawUTF8(), jsons.size(), allFiles.size());
        for (auto& f : allFiles) std::printf ("  %s\n", f.getRelativePathFrom (presets).toRawUTF8());
        check (factory.isDirectory(), "[5] ~/Library/O-Strata/Presets/Factory exists after construction");
        check (jsons.size() == 1 && jsons[0].getRelativePathFrom (factory) == "Init/Init.json", "[5] exactly one factory JSON: Init/Init.json");
        check (! presets.getChildFile ("User").exists() || presets.getChildFile ("User").getNumberOfChildFiles (juce::File::findFiles) == 0, "[5] no User presets on disk");
        check (factory.getChildFile (".factory-version").loadFileAsString().trim() == "1.0.0", "[5] .factory-version == 1.0.0");
        check (allFiles.size() == 2, "[5] the Presets tree holds exactly two files (.factory-version + Init.json)");

        const auto json = juce::JSON::parse (jsons.size() == 1 ? jsons[0].loadFileAsString() : juce::String());
        auto* obj = json.getDynamicObject();
        const juce::var params = obj != nullptr ? obj->getProperty ("parameters") : juce::var();
        auto* pobj = params.getDynamicObject();
        const int nKeys = pobj != nullptr ? pobj->getProperties().size() : -1;
        std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
        const int live = p->getParameters().size() - 7;     // 7 excluded tuning IDs
        std::printf ("Init.json: %d parameter keys (live %d - 7 = %d)  category=%s\n", nKeys, p->getParameters().size(), live,
                     obj != nullptr ? obj->getProperty ("category").toString().toRawUTF8() : "?");
        check (nKeys == 198 && nKeys == live, "[5] Init.json parameters has 198 keys (== live count - 7 excluded)");
        bool excludedPresent = false;
        for (const auto* id : { "tuningPreset", "tonic", "masterTune", "octaveStretch", "pitchBendRange", "glideMode", "glideTime" })
            if (pobj != nullptr && pobj->hasProperty (id)) excludedPresent = true;
        check (! excludedPresent, "[5] none of the 7 excluded tuning IDs is in Init.json");
        auto key = [&] (const char* id) { return pobj != nullptr ? (double) pobj->getProperty (id) : -1.0; };
        check (std::abs (key ("oscAPos") - 0.5) < 1e-6, "[5] Init oscAPos == 0.5 (Orbit Size default)");
        check (std::abs (key ("oscAUnison") - 0.0) < 1e-6, "[5] Init oscAUnison == 0.0 (1 of 1..4)");
        check (std::abs (key ("oscAQuality") - 0.5) < 1e-6, "[5] Init oscAQuality == 0.5 (index 1 of 3 = 2x)");
        check (std::abs (key ("oscATerFreq") - 0.4) < 1e-5, "[5] Init oscATerFreq ~= 0.4 (exact-log norm of 1.0)");
        check (obj != nullptr && obj->getProperty ("category").toString() == "Init" && obj->getProperty ("plugin").toString() == "O-Strata"
               && (bool) obj->getProperty ("factory") && obj->getProperty ("version").toString() == "1.0.0"
               && jsons.size() == 1 && jsons[0].getFileNameWithoutExtension() == "Init", "[5] category Init, plugin O-Strata, factory true, version 1.0.0, file name Init (the name is the file name)");
    }

    // ── [6] choice lists and TerFreq range (in-process) ────────────────────
    {
        std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
        auto choice = [&] (const char* id) { return dynamic_cast<juce::AudioParameterChoice*> (findParam (*p, id)); };
        auto* terrain = choice ("oscATerrain"); auto* edge = choice ("oscATerEdge");
        auto* orbit = choice ("oscAOrbit");     auto* quality = choice ("oscAQuality");
        check (terrain != nullptr && terrain->choices.size() == 7 && terrain->choices[0] == "Sine Product"
               && terrain->choices[6] == u8 ("Imported\xE2\x80\xA6"), "[6] oscATerrain: 7 choices, Imported... last");
        check (edge != nullptr && edge->choices.size() == 2 && edge->choices[0] == "Mirror" && edge->choices[1] == "Window", "[6] oscATerEdge: exactly Mirror / Window");
        check (orbit != nullptr && orbit->choices.size() == 11 && orbit->choices[0] == "Ellipse"
               && orbit->choices[2] == u8 ("Lima\xC3\xA7on") && orbit->choices[10] == "Squarcle", "[6] oscAOrbit: 11 choices incl. Limacon at [2]");
        check (quality != nullptr && quality->choices.size() == 3 && quality->choices[0] == "Bandlimited"
               && quality->choices[1] == u8 ("2\xC3\x97") && quality->choices[2] == u8 ("4\xC3\x97")
               && quality->getIndex() == 1, "[6] oscAQuality: Bandlimited / 2x / 4x, default index 1");
        for (const auto* id : { "oscBTerrain", "oscBTerEdge", "oscBOrbit", "oscBQuality" })
            check (choice (id) != nullptr, "[6] Osc B combo parameter exists");
        auto* tf = dynamic_cast<juce::AudioParameterFloat*> (findParam (*p, "oscATerFreq"));
        juce::RangedAudioParameter* tfr = tf;   // getDefaultValue/getNumSteps are private overrides on the concrete type
        const auto& r = tf->getNormalisableRange();
        std::printf ("terFreq: start=%.4f end=%.4f default=%.6f numSteps=%d  from0to1(0.4)=%.6f\n",
                     r.start, r.end, tfr->getDefaultValue(), tfr->getNumSteps(), r.convertFrom0to1 (0.4f));
        check (tf != nullptr && r.start == 0.25f && r.end == 8.0f, "[6] oscATerFreq range 0.25 .. 8.0");
        check (std::abs (tfr->getDefaultValue() - 0.4f) < 1e-6f, "[6] oscATerFreq getDefaultValue() == 0.400000 (norm of 1.0)");
        check (tfr->getNumSteps() == 0x7fffffff, "[6] oscATerFreq getNumSteps() continuous (0x7fffffff)");
        check (std::abs (r.convertFrom0to1 (0.0f) - 0.25f) < 1e-6f && std::abs (r.convertFrom0to1 (1.0f) - 8.0f) < 1e-5f
               && std::abs (r.convertFrom0to1 (0.4f) - 1.0f) < 1e-5f, "[6] exact-log: norm 0/0.4/1 -> 0.25/1.0/8.0");
        auto* pos = findParam (*p, "oscAPos"); auto* uni = dynamic_cast<juce::AudioParameterInt*> (findParam (*p, "oscAUnison"));
        check (pos != nullptr && std::abs (pos->getDefaultValue() - 0.5f) < 1e-6f && pos->getName (64) == "Osc A Orbit Size", "[6] oscAPos default 0.5, host name Osc A Orbit Size");
        check (uni != nullptr && uni->getRange().getEnd() == 4, "[6] oscAUnison max 4");
        check (StrataParamIds::allSliderIds().size() == 166 && StrataParamIds::allComboIds().size() == 8, "[6] allSliderIds() == 166, allComboIds() == 8");
        int bound = 0;
        for (const auto& id : StrataParamIds::allSliderIds()) if (findParam (*p, id) != nullptr) ++bound;
        for (const auto& id : StrataParamIds::allComboIds())  if (findParam (*p, id) != nullptr) ++bound;
        check (bound == 174, "[6] every slider + combo relay ID (166 + 8) resolves to a parameter");
    }

    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL SMOKE CHECKS PASSED" : "SMOKE FAILED", failures);
    return failures == 0 ? 0 : 1;
}
