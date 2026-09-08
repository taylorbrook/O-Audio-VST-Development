// O-Strata Stage 1 headless smoke — verify-phase evidence for the SUMMARY's
// "manual smoke left for verify" items that do not need a window:
//   [1] a held note sounds on Osc A alone and on Osc B alone (sine placeholder
//       through the unchanged voice/FX path), with the other oscillator's level
//       as the negative control
//   [2] TuningEngine::loadScalaFile() loads test-tunings/just-major.scl
//   [3] state round-trip: all 219 params + tuning + uiLanguage restored into a
//       FRESH processor; the XML carries an empty <geometryImports/> child
// Links the O-Strata-param-dump objects (JUCE_WEB_BROWSER=0, no editor TU).

#include <JuceHeader.h>
#include "PluginProcessor.h"
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
        const auto* geo = xml != nullptr ? xml->getChildByName ("geometryImports") : nullptr;
        check (geo != nullptr, "[3] <geometryImports> child present");
        check (geo != nullptr && geo->getNumChildElements() == 0 && geo->getNumAttributes() == 0,
               "[3] <geometryImports/> is empty");
        check (xml != nullptr && xml->getChildByName ("tuningEngine") != nullptr, "[3] <tuningEngine> child present");
        check (xml != nullptr && xml->getStringAttribute ("uiLanguage") == "fr", "[3] uiLanguage=\"fr\" stored as a string");
        int paramNodes = 0;
        if (xml != nullptr) for (auto* c : xml->getChildIterator()) if (c->hasTagName ("PARAM")) ++paramNodes;
        std::printf ("state: %d bytes, %d PARAM nodes, root=%s\n", (int) block.getSize(), paramNodes,
                     xml != nullptr ? xml->getTagName().toRawUTF8() : "?");
        check (paramNodes == 219, "[3] XML carries 219 PARAM nodes");

        std::unique_ptr<juce::AudioProcessor> p2 (createPluginFilter());
        auto* s2 = dynamic_cast<OStrataAudioProcessor*> (p2.get());
        p2->setStateInformation (block.getData(), (int) block.getSize());

        int mismatches = 0, differsFromDefault = 0; size_t i = 0;
        const auto& params2 = p2->getParameters();
        check (params2.size() == 219 && want.size() == 219, "[3] both instances expose 219 parameters");
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
        std::printf ("round-trip: %d mismatches; %d of 219 saved values differ from default\n", mismatches, differsFromDefault);
        check (mismatches == 0, "[3] all 219 parameter values restored into a fresh processor");
        check (differsFromDefault > 150, "[3] the test actually moved the parameters (>150 of the 214 randomised differ from default; not a vacuous pass)");
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

    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL SMOKE CHECKS PASSED" : "SMOKE FAILED", failures);
    return failures == 0 ? 0 : 1;
}
