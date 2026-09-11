// O-Strata offline render harness — the Stage 2 DSP gate (ARCHITECTURE "Harness
// design", stages/2-dsp/PLAN.md Task 12 / 16 / 25). Console target
// O-Strata-render-test built by ouaricon_add_processor_console (JUCE_WEB_BROWSER=0,
// no editor TU, no UIResources). Round A: gates H1–H9, `tuning`, `smoke`, the
// FUNC-02/03 centroid checks, latency, crossfade, export. No message loop is ever
// pumped — every gate drives the processor directly (README: the dispatch-loop token
// does not appear in this file in Round A).
//
// Exit code = number of failed checks. Every check prints its measured value and
// its threshold on one line. Fixture paths come from STRATA_FIXTURES_DIR (or
// --fixtures), never from cwd; every README command runs from any directory.

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "StrataParamIds.h"
#include "FactoryPresets.h"
#include "ScaleGenerator.h"
#include "dsp/ModulationMatrix.h"
#include "dsp/TerrainOscillator.h"
#include "dsp/HalfbandDecimator.h"
#include "reference/theta_reference.h"
#include "reference/spectrum.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

//==============================================================================
// ── Allocation counter (H8, PERF-01 / DSP-05) ──────────────────────────────────
//
// Copied verbatim from plugins/O-Octagon/tests/render-harness/main.cpp:110-241 (its
// probe AO), provenance kept: -fsanitize=realtime is UNSUPPORTED by Apple clang 17,
// so allocation is MEASURED by replacing the global operator new family; every
// variant is replaced, including both std::align_val_t overloads and every matching
// delete (an un-replaced aligned-new is silently uncounted). The counter is
// THREAD-FILTERED to the arming thread and the foreign-thread tally is printed
// beside every verdict. Coverage statement (RESEARCH §2.5, plan Decision 4):
// covered = every new-expression on the audio thread; not covered = HeapBlock /
// malloc paths (AudioBuffer, Array, MidiBuffer, ReferenceCountedArray, MemoryBlock)
// and the AsyncUpdater post path — grep + inspection.

namespace rtcheck
{
    std::atomic<long long> allocations { 0 };
    std::atomic<bool>      armed { false };
    std::atomic<long long> foreignAllocations { 0 };
    std::atomic<bool>      ownerValid { false };
    std::thread::id        ownerThread {};

    inline void note() noexcept
    {
        if (! armed.load (std::memory_order_relaxed))
            return;

        if (ownerValid.load (std::memory_order_relaxed)
            && std::this_thread::get_id() != ownerThread)
        {
            foreignAllocations.fetch_add (1, std::memory_order_relaxed);
            return;
        }

        allocations.fetch_add (1, std::memory_order_relaxed);
    }

    inline void arm (bool resetForeign = true) noexcept
    {
        allocations.store (0);
        if (resetForeign)
            foreignAllocations.store (0);
        ownerThread = std::this_thread::get_id();
        ownerValid.store (true);
        armed.store (true);
    }

    inline void disarm() noexcept { armed.store (false); }

    inline std::string foreignNote()
    {
        const auto f = foreignAllocations.load();
        if (f == 0) return {};
        return " [+" + std::to_string (f) + " foreign-thread, NOT counted — see rtcheck]";
    }
}

void* operator new (std::size_t size)
{
    rtcheck::note();
    if (void* p = std::malloc (size == 0 ? 1 : size))
        return p;
    throw std::bad_alloc();
}

void* operator new[] (std::size_t size)                              { return ::operator new (size); }

void* operator new (std::size_t size, std::align_val_t alignment)
{
    rtcheck::note();
    std::size_t a = static_cast<std::size_t> (alignment);
    if (a < sizeof (void*))
        a = sizeof (void*);
    void* p = nullptr;
    if (posix_memalign (&p, a, size == 0 ? a : size) == 0 && p != nullptr)
        return p;
    throw std::bad_alloc();
}

void* operator new[] (std::size_t size, std::align_val_t alignment)
{
    return ::operator new (size, alignment);
}

void operator delete (void* p) noexcept                              { std::free (p); }
void operator delete[] (void* p) noexcept                            { std::free (p); }
void operator delete (void* p, std::size_t) noexcept                 { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept               { std::free (p); }
void operator delete (void* p, std::align_val_t) noexcept            { std::free (p); }
void operator delete[] (void* p, std::align_val_t) noexcept          { std::free (p); }
void operator delete (void* p, std::size_t, std::align_val_t) noexcept   { std::free (p); }
void operator delete[] (void* p, std::size_t, std::align_val_t) noexcept { std::free (p); }

//==============================================================================
// ── Bookkeeping ───────────────────────────────────────────────────────────────

namespace
{
    int failures = 0;
    int checksRun = 0;

    void check (bool ok, const std::string& line)
    {
        ++checksRun;
        std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", line.c_str());
        if (! ok) ++failures;
    }

    /** A negative control: the measurement MUST fail; the verdict line passes when it does. */
    void checkFailsAsExpected (bool innerPassed, const std::string& line)
    {
        ++checksRun;
        std::printf ("%s  %s%s\n", innerPassed ? "FAIL" : "PASS", line.c_str(),
                     innerPassed ? "  (control PASSED — the probe is vacuous)" : "  FAIL-as-expected");
        if (innerPassed) ++failures;
    }

    std::string fmt (const char* f, ...)
    {
        char buf[1024];
        va_list ap; va_start (ap, f); std::vsnprintf (buf, sizeof (buf), f, ap); va_end (ap);
        return buf;
    }

    double secondsSince (std::chrono::steady_clock::time_point t0)
    {
        return std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    }

    const char* kTerrainNames[kNumAnalyticTerrains] = { "SineProduct", "RadialRings", "Saddle", "RidgedCosines", "Mitsuhashi", "CosineWells" };
    const char* kOrbitNames[kNumOrbitKinds] = { "Ellipse", "Superellipse", "Limacon", "Epitrochoid3", "Epitrochoid5", "Epitrochoid7",
                                                "Hypocycloid3", "Hypocycloid5", "Hypocycloid7", "Butterfly", "Squarcle" };
    const char* kNoteNames[3] = { "C2", "C4", "C6" };
    const int   kCNotes[3] = { 36, 60, 84 };

    // Mod matrix indices (asserted at start-up against the live lists)
    constexpr int kSrcLFO1 = 1, kSrcVelocity = 7, kSrcModWheel = 9;
    constexpr int kDstLFO1Rate = 10, kDstPitch = 23, kDstOscAOrbCX = 28, kDstOscATerFreq = 31;

    // ── Options ──
    struct Options
    {
        std::vector<std::string> gates;
        int note = 60;
        float velocity = 0.8f;
        double seconds = 1.0;
        int terrain = -1, orbit = -1, quality = -1;
        std::vector<std::pair<std::string, float>> sets;
        double fs = 48000.0;
        int block = 512;
        uint32_t seed = 0x5EED0001u;
        std::string fixtures = STRATA_FIXTURES_DIR;
        std::string exportsDir = STRATA_EXPORTS_DIR;
        std::string exportName;
        bool printOnly = false;
        bool withDisk = false;
    } opt;

    // ── Instance: one processor + the setters the gates need ──
    struct Instance
    {
        std::unique_ptr<juce::AudioProcessor> owner;
        OStrataAudioProcessor& p;
        double fs = 48000.0;
        int block = 512;
        bool prepared = false;

        Instance() : owner (createPluginFilter()), p (dynamic_cast<OStrataAudioProcessor&> (*owner)) {}

        juce::RangedAudioParameter* param (const std::string& id) const
        {
            auto* r = p.getAPVTS().getParameter (juce::String (id));
            if (r == nullptr) { std::printf ("!! unknown parameter %s\n", id.c_str()); std::abort(); }
            return r;
        }
        void setNorm (const std::string& id, float norm)  { param (id)->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm)); }
        void setReal (const std::string& id, float real)  { auto* r = param (id); r->setValueNotifyingHost (r->convertTo0to1 (real)); }
        void setChoice (const std::string& id, int index) { setReal (id, static_cast<float> (index)); }
        float getReal (const std::string& id) const       { auto* r = param (id); return r->convertFrom0to1 (r->getValue()); }

        /** The "clean patch" (RESEARCH §2.6): A only, instant full-level envelope, deterministic phases. */
        void cleanPatch()
        {
            setReal ("oscMix", 0.0f);
            setReal ("oscBLevel", 0.0f);
            setReal ("ampAttack", 0.001f);
            setReal ("ampSustain", 1.0f);
            setReal ("ampRelease", 0.001f);
            setReal ("oscAPhase", 0.25f);
            setReal ("oscBPhase", 0.25f);
            p.setHarnessPhaseSeed (opt.seed);
        }

        void setTerrainOrbit (int osc, int terrain, int orbit)
        {
            const std::string pre = osc == 0 ? "oscA" : "oscB";
            if (terrain >= 0) setChoice (pre + "Terrain", terrain);
            if (orbit >= 0)   setChoice (pre + "Orbit", orbit);
        }

        void applyCliOverrides()
        {
            if (opt.terrain >= 0) setChoice ("oscATerrain", opt.terrain);
            if (opt.orbit >= 0)   setChoice ("oscAOrbit", opt.orbit);
            if (opt.quality >= 0) setChoice ("oscAQuality", opt.quality);
            for (const auto& kv : opt.sets) setNorm (kv.first, kv.second);
        }

        void prepare (double sampleRate, int blockSize)
        {
            fs = sampleRate; block = blockSize;
            p.setPlayConfigDetails (0, 2, fs, block);
            p.prepareToPlay (fs, block);
            prepared = true;
        }
    };

    struct MidiEvent { int sample; juce::MidiMessage msg; };

    struct RenderSpec
    {
        double seconds = 1.0;
        int note = 60;
        float velocity = 0.8f;
        int noteOnSample = 0;
        double noteOffAt = -1.0;            // < 0: held for the whole render
        std::vector<MidiEvent> extra;       // additional events (sample index absolute)
        std::function<void (int blockStart)> onBlock;   // called BEFORE each block (setValueNotifyingHost etc.)
        bool armAllocations = false;
    };

    struct Rendered
    {
        std::vector<double> L, R;
        double fs = 48000.0;
        long long allocations = 0;
    };

    /** prepareToPlay if needed, pre-sized buffers, MidiBuffer::ensureSize, one warm-up
        block (absorbs the first-block triggerAsyncUpdate post), then the render. */
    Rendered render (Instance& in, const RenderSpec& spec)
    {
        if (! in.prepared) in.prepare (opt.fs, opt.block);
        const int total = static_cast<int> (std::llround (spec.seconds * in.fs));
        juce::AudioBuffer<float> buf (2, in.block);
        juce::MidiBuffer midi;
        midi.ensureSize (256);

        // warm-up (never counted): first processBlock posts the tuning async update
        buf.clear(); midi.clear();
        in.p.processBlock (buf, midi);

        Rendered out; out.fs = in.fs;
        out.L.reserve ((size_t) total); out.R.reserve ((size_t) total);
        const int noteOff = spec.noteOffAt >= 0.0 ? static_cast<int> (spec.noteOffAt * in.fs) : -1;
        std::vector<MidiEvent> events = spec.extra;
        events.push_back ({ spec.noteOnSample, juce::MidiMessage::noteOn (1, spec.note, spec.velocity) });
        if (noteOff >= 0) events.push_back ({ noteOff, juce::MidiMessage::noteOff (1, spec.note) });

        long long counted = 0;
        for (int start = 0; start < total; start += in.block)
        {
            const int n = juce::jmin (in.block, total - start);
            if (spec.onBlock) spec.onBlock (start);
            midi.clear();
            for (const auto& e : events)
                if (e.sample >= start && e.sample < start + n)
                    midi.addEvent (e.msg, e.sample - start);
            buf.clear();
            if (spec.armAllocations) rtcheck::arm (false);
            in.p.processBlock (buf, midi);
            if (spec.armAllocations) { rtcheck::disarm(); counted += rtcheck::allocations.load(); }
            for (int i = 0; i < n; ++i)
            {
                out.L.push_back (buf.getSample (0, i));
                out.R.push_back (buf.getSample (1, i));
            }
        }
        out.allocations = counted;
        return out;
    }

    // ── Signal helpers ──
    bool allFinite (const std::vector<double>& v) { for (double x : v) if (! std::isfinite (x)) return false; return true; }
    double maxAbs (const std::vector<double>& v, size_t from = 0) { double m = 0; for (size_t i = from; i < v.size(); ++i) m = std::max (m, std::abs (v[i])); return m; }
    double maxAbsDiff (const std::vector<double>& a, const std::vector<double>& b)
    {
        if (a.size() != b.size()) return 1.0e9;
        double m = 0; for (size_t i = 0; i < a.size(); ++i) m = std::max (m, std::abs (a[i] - b[i])); return m;
    }
    double maxStep (const std::vector<double>& v, size_t from)
    {
        double m = 0; for (size_t i = std::max<size_t> (from, 1); i < v.size(); ++i) m = std::max (m, std::abs (v[i] - v[i - 1])); return m;
    }
    double rms (const std::vector<double>& v, size_t from) { double s = 0; size_t n = 0; for (size_t i = from; i < v.size(); ++i) { s += v[i] * v[i]; ++n; } return n ? std::sqrt (s / (double) n) : 0.0; }
    double mean (const std::vector<double>& v, size_t from) { double s = 0; size_t n = 0; for (size_t i = from; i < v.size(); ++i) { s += v[i]; ++n; } return n ? s / (double) n : 0.0; }
    bool bitIdentical (const std::vector<double>& a, const std::vector<double>& b)
    {
        return a.size() == b.size() && std::memcmp (a.data(), b.data(), a.size() * sizeof (double)) == 0;
    }

    /** Hann-windowed power spectrum of y[from, from+frame) zero-padded to nfft. */
    std::vector<double> spec (const std::vector<double>& y, size_t from, size_t frame, size_t nfft)
    {
        frame = std::min (frame, y.size() - from);
        return spectrum::windowedSpectrum (y.data() + from, frame, nfft);
    }

    /** Frequency near fGuess by phase progression of a Hann-windowed DFT over two
        consecutive half-windows (robust to ~1e-4 cent; harmonics ≥ 2·f are far outside
        the 1/Thalf lobe). */
    double measureFrequency (const std::vector<double>& y, double fs, double fGuess, size_t from)
    {
        const size_t n = y.size() - from;
        const size_t half = n / 2;
        auto dft = [&] (size_t start) {
            std::complex<double> acc (0.0, 0.0);
            for (size_t i = 0; i < half; ++i)
            {
                const double w = 0.5 - 0.5 * std::cos (2.0 * spectrum::kPi * double (i) / double (half));
                const double ph = -2.0 * spectrum::kPi * fGuess * double (start + i) / fs;
                acc += y[from + start + i] * w * std::complex<double> (std::cos (ph), std::sin (ph));
            }
            return acc;
        };
        const auto a = dft (0), b = dft (half);
        const double dphi = std::arg (b / a);
        return fGuess + dphi / (2.0 * spectrum::kPi * (double (half) / fs));
    }

    /** DC over the last `seconds` of y: Hann-windowed mean (the plain mean over a
        non-integer number of C2 cycles is contaminated by the fundamental). */
    double dcOf (const std::vector<double>& y, double fs, double seconds)
    {
        const size_t n = std::min (y.size(), (size_t) (seconds * fs)), from = y.size() - n;
        double num = 0.0, den = 0.0;
        for (size_t i = 0; i < n; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (2.0 * spectrum::kPi * double (i) / double (n));
            num += y[from + i] * w; den += w;
        }
        return den > 0.0 ? num / den : 0.0;
    }

    double centroidOf (const std::vector<double>& y, double fs)
    {
        const size_t from = (size_t) (0.1 * fs), frame = 32768, nfft = 32768;
        const auto s = spec (y, from, frame, nfft);
        return spectrum::spectralCentroid (s, fs / double (nfft), fs * 0.5);
    }

    //==========================================================================
    // ── --smoke: Stage 1 checks [1]–[6] on the terrain oscillator ────────────

    void gateSmoke()
    {
        std::printf ("\n== --smoke (Stage 1 checks [1]-[6], [4] inverted, [5] under --with-disk) ==\n");
        const juce::File scl = juce::File (juce::String (opt.fixtures)).getChildFile ("test-tunings/just-major.scl");
        std::printf ("scl: %s (exists=%d)\n", scl.getFullPathName().toRawUTF8(), (int) scl.existsAsFile());

        auto rmsOf = [] (Instance& in) {
            RenderSpec s; s.seconds = 80 * 512 / 48000.0;
            auto r = render (in, s);
            return std::make_pair (rms (r.L, r.L.size() / 2), allFinite (r.L) && allFinite (r.R));
        };
        // [1]
        {
            Instance a;  a.setNorm ("oscMix", 0.0f);
            Instance a0; a0.setNorm ("oscMix", 0.0f); a0.setNorm ("oscALevel", 0.0f);
            Instance b;  b.setNorm ("oscMix", 1.0f); b.setNorm ("oscBLevel", 0.8f);
            Instance b0; b0.setNorm ("oscMix", 1.0f); b0.setNorm ("oscBLevel", 0.0f);
            const auto rA = rmsOf (a), rA0 = rmsOf (a0), rB = rmsOf (b), rB0 = rmsOf (b0);
            std::printf ("rms  A=%.4f  A(muted)=%.4f  B=%.4f  B(muted)=%.4f\n", rA.first, rA0.first, rB.first, rB0.first);
            check (rA.first > 0.01, "[1] held note sounds with oscMix=0 (Osc A, terrain oscillator)");
            check (rB.first > 0.01, "[1] held note sounds with oscMix=1 (Osc B)");
            check (rA0.first < rA.first * 0.1, "[1] negative control: muting Osc A level kills the A-only note");
            check (rB0.first < rB.first * 0.1, "[1] negative control: muting Osc B level kills the B-only note");
            check (rA.second && rA0.second && rB.second && rB0.second, "[1] no NaN/Inf in rendered output");
        }
        // [2]
        {
            Instance in;
            auto* te = in.p.getTuningEngine();
            const double f60Before = te->getFrequency (60), f64Before = te->getFrequency (64);
            const bool loaded = te->loadScalaFile (scl);
            const double f60 = te->getFrequency (60), f62 = te->getFrequency (62), f64 = te->getFrequency (64), f67 = te->getFrequency (67);
            std::printf ("scala: before 64/60=%.6f  after 62/60=%.6f 64/60=%.6f 67/60=%.6f (f60=%.3f Hz)\n",
                         f64Before / f60Before, f62 / f60, f64 / f60, f67 / f60, f60);
            check (loaded, "[2] TuningEngine::loadScalaFile(just-major.scl) returns true (fixture from STRATA_FIXTURES_DIR)");
            check (std::abs (f62 / f60 - 1.25) < 1e-3, "[2] note 62 / note 60 = 5/4 after load");
            check (std::abs (f64 / f60 - 1.5) < 1e-3, "[2] note 64 / note 60 = 3/2 after load");
            check (std::abs (f67 / f60 - 2.0) < 1e-3, "[2] note 67 / note 60 = 2/1 after load");
        }
        // [3]
        {
            Instance i1;
            juce::uint32 seed = 0x5EED1234u;
            std::vector<float> want;
            for (auto* prm : i1.p.getParameters())
            {
                seed = seed * 1664525u + 1013904223u;
                const float v = (float) (seed >> 8) / (float) (1u << 24);
                auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm);
                const juce::String id = r != nullptr ? r->getParameterID() : juce::String();
                const bool tuningParam = id == "tuningPreset" || id == "tonic" || id == "masterTune"
                                      || id == "octaveStretch" || id == "pitchBendRange";
                if (! tuningParam) prm->setValueNotifyingHost (v);
                const float raw = prm->getValue();
                want.push_back (r != nullptr ? r->convertTo0to1 (r->convertFrom0to1 (raw)) : raw);
            }
            i1.p.uiLanguage.store (1);
            check (i1.p.getTuningEngine()->loadScalaFile (scl), "[3] scala loaded into P1 before save");
            juce::MemoryBlock block;
            i1.p.getStateInformation (block);
            const char* txt = static_cast<const char*> (block.getData()) + 8;
            const auto len = (int) block.getSize() - 9;
            std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (juce::String::fromUTF8 (txt, len)));
            check (xml != nullptr, "[3] state XML parses");
            const auto* geo = xml != nullptr ? xml->getChildByName ("terrainImports") : nullptr;
            check (geo != nullptr && geo->getNumChildElements() == 0, "[3] <terrainImports/> child present and empty");
            int paramNodes = 0;
            if (xml != nullptr) for (auto* c : xml->getChildIterator()) if (c->hasTagName ("PARAM")) ++paramNodes;
            check (paramNodes == 205 && paramNodes == i1.p.getParameters().size(), fmt ("[3] XML carries 205 PARAM nodes (live %d)", paramNodes));
            Instance i2;
            i2.p.setStateInformation (block.getData(), (int) block.getSize());
            int mismatches = 0, differs = 0; size_t i = 0;
            for (auto* prm : i2.p.getParameters())
            {
                if (std::abs (prm->getValue() - want[i]) > 1e-6f) ++mismatches;
                if (std::abs (want[i] - prm->getDefaultValue()) > 1e-6f) ++differs;
                ++i;
            }
            check (mismatches == 0, fmt ("[3] all 205 parameter values restored into a fresh processor (%d mismatches)", mismatches));
            check (differs > 150, fmt ("[3] the test moved the parameters (%d of %d differ from default)", differs, (int) want.size()));
            check (i2.p.uiLanguage.load() == 1, "[3] uiLanguage restored (fr)");
            auto* t1 = i1.p.getTuningEngine(); auto* t2 = i2.p.getTuningEngine();
            bool eq = true;
            for (int n = 48; n <= 84; ++n) if (std::abs (t1->getFrequency (n) - t2->getFrequency (n)) > 1e-6) eq = false;
            check (eq && std::abs (t2->getFrequency (64) / t2->getFrequency (60) - 1.5) < 1e-3, "[3] tuning (just-major) restored: notes 48..84 identical, 64/60 = 3/2");
        }
        // [4] inverted (plan Decision 21): a route to OscA Terrain Freq CHANGES the render
        {
            const auto dests = getModDestNames();
            check (dests.size() == 46 && dests.size() == static_cast<int> (ModDest::NumDests), "[4] getModDestNames().size() == 46 == ModDest::NumDests");
            check (dests[31] == "OscA Terrain Freq" && dests[23] == "Pitch" && dests[28] == "OscA Orbit CX", "[4] destinations 23 / 28 / 31 = Pitch / OscA Orbit CX / OscA Terrain Freq");
            auto pin = [] (Instance& in) { in.setNorm ("oscMix", 0.0f); in.setNorm ("oscAPhase", 0.25f); in.setNorm ("oscBPhase", 0.25f); in.p.setHarnessPhaseSeed (opt.seed); };
            auto route = [] (Instance& in, int src, int dst) {
                in.setChoice ("modSlot0Src", src); in.setChoice ("modSlot0Dst", dst); in.setNorm ("modSlot0Amt", 1.0f); in.setNorm ("modSlot0On", 1.0f);
            };
            RenderSpec s; s.seconds = 40 * 512 / 48000.0;
            Instance r0; pin (r0); auto ref = render (r0, s);
            Instance r1; pin (r1); auto ref2 = render (r1, s);
            check (maxAbsDiff (ref.L, ref2.L) == 0.0, "[4] unrouted render is deterministic (two renders identical) — the control");
            Instance rt; pin (rt); route (rt, kSrcLFO1, kDstOscATerFreq);
            auto* dstP = dynamic_cast<juce::AudioParameterChoice*> (rt.param ("modSlot0Dst"));
            check (dstP != nullptr && dstP->getIndex() == 31, "[4] modSlot0Dst index == 31 after set");
            auto tf = render (rt, s);
            Instance rp; pin (rp); route (rp, kSrcLFO1, kDstPitch);
            auto pt = render (rp, s);
            const double dT = maxAbsDiff (ref.L, tf.L), dP = maxAbsDiff (ref.L, pt.L);
            std::printf ("route: LFO1->OscA Terrain Freq max|d|=%.3e   LFO1->Pitch max|d|=%.3e\n", dT, dP);
            check (dT > 1.0e-3, fmt ("[4] LFO1 -> OscA Terrain Freq (index 31) CHANGES the render: max|d| = %.3e (need > 1e-3)", dT));
            check (dP > 1.0e-3, fmt ("[4] positive control: LFO1 -> Pitch (index 23) changes the render: max|d| = %.3e", dP));
        }
        // [5] on-disk factory bank — only with --with-disk (the harness never touches ~/Library by default)
        if (opt.withDisk)
        {
            const juce::File presets = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/O-Strata/Presets");
            const juce::File factory = presets.getChildFile ("Factory");
            juce::Array<juce::File> jsons; factory.findChildFiles (jsons, juce::File::findFiles, true, "*.json");
            juce::Array<juce::File> all; presets.findChildFiles (all, juce::File::findFiles, true, "*");
            check (factory.isDirectory(), "[5] ~/Library/O-Strata/Presets/Factory exists");
            check (jsons.size() == 1 && jsons[0].getRelativePathFrom (factory) == "Init/Init.json", "[5] exactly one factory JSON: Init/Init.json");
            check (factory.getChildFile (".factory-version").loadFileAsString().trim() == "1.0.0", "[5] .factory-version == 1.0.0");
            check (all.size() == 2, fmt ("[5] the Presets tree holds exactly two files (%d)", all.size()));
        }
        else
            std::printf ("skip  [5] on-disk factory bank (pass --with-disk; verified in Stage 1)\n");
        // [6]
        {
            Instance in;
            auto choice = [&] (const char* id) { return dynamic_cast<juce::AudioParameterChoice*> (in.param (id)); };
            auto* terrain = choice ("oscATerrain"); auto* orbit = choice ("oscAOrbit"); auto* quality = choice ("oscAQuality"); auto* edge = choice ("oscATerEdge");
            check (terrain != nullptr && terrain->choices.size() == 7 && terrain->choices[0] == "Sine Product", "[6] oscATerrain: 7 choices, Sine Product first");
            check (orbit != nullptr && orbit->choices.size() == 11 && orbit->choices[10] == "Squarcle", "[6] oscAOrbit: 11 choices, Squarcle last");
            check (quality != nullptr && quality->choices.size() == 3 && quality->getIndex() == 1, "[6] oscAQuality: 3 choices, default index 1 (2x)");
            check (edge != nullptr && edge->choices.size() == 2, "[6] oscATerEdge: exactly 2 choices");
            const auto& r = dynamic_cast<juce::AudioParameterFloat*> (in.param ("oscATerFreq"))->getNormalisableRange();
            check (r.start == 0.25f && r.end == 8.0f && std::abs (r.convertFrom0to1 (0.4f) - 1.0f) < 1e-5f, "[6] oscATerFreq exact-log 0.25..8, norm 0.4 = 1.0");
            check (StrataParamIds::allSliderIds().size() == 166 && StrataParamIds::allComboIds().size() == 8, "[6] allSliderIds() == 166, allComboIds() == 8");
            check (in.p.getParameters().size() == 205, "[6] 205 parameters");
        }
    }

    //==========================================================================
    // ── --gate tuning: FUNC-09 / FUNC-10 through getTuningEngine() ───────────

    void gateTuning()
    {
        std::printf ("\n== --gate tuning (Scala + 31-EDO, >= 5 keys, |d| <= 0.5 cent) ==\n");
        const int keys[5] = { 48, 55, 60, 64, 67 };
        auto run = [&] (const char* label, std::function<bool (TuningEngine&)> load) {
            Instance in; in.cleanPatch();
            in.setChoice ("oscATerrain", 0); in.setChoice ("oscAOrbit", 0);
            in.p.harnessPreFilterTap.store (true);
            const bool ok = load (*in.p.getTuningEngine());
            check (ok, fmt ("[tuning] %s loaded into the engine", label));
            in.prepare (opt.fs, opt.block);
            double worst = 0.0;
            for (int k : keys)
            {
                const double want = in.p.getTuningEngine()->getFrequency (k);
                RenderSpec s; s.seconds = 1.5; s.note = k; s.velocity = 1.0f;
                auto r = render (in, s);
                // stop the note before the next key
                RenderSpec off; off.seconds = 0.3; off.note = k; off.noteOnSample = -1; off.extra = { { 0, juce::MidiMessage::noteOff (1, k) } };
                render (in, off);
                const double got = measureFrequency (r.L, r.fs, want, (size_t) (0.25 * r.fs));
                const double c = spectrum::cents (got, want);
                worst = std::max (worst, std::abs (c));
                std::printf ("  %s key %d: want %.4f Hz got %.4f Hz  d = %+.4f cent\n", label, k, want, got, c);
            }
            check (worst <= 0.5, fmt ("[tuning] %s: worst |d| = %.4f cent over 5 keys (need <= 0.5)", label, worst));
        };
        const juce::File scl = juce::File (juce::String (opt.fixtures)).getChildFile ("test-tunings/just-major.scl");
        run ("Scala just-major.scl", [&] (TuningEngine& te) { return te.loadScalaFile (scl); });
        run ("31-EDO", [&] (TuningEngine& te) {
            const auto cents = ScaleGenerator::generateEDO (31, 1200.0);
            te.setCustomIntervals (cents, "31-EDO");
            const double r = te.getFrequency (61) / te.getFrequency (60);
            std::printf ("  31-EDO step ratio 61/60 = %.6f (want %.6f)\n", r, std::pow (2.0, 1.0 / 31.0));
            return std::abs (r - std::pow (2.0, 1.0 / 31.0)) < 1e-6;
        });
    }

    //==========================================================================
    // ── H1: θ parity against the analytic DC-blocked cosine ──────────────────

    struct H1Row { const char* name; theta_ref::WarpType warp; int warpIndex; float amount; int unison; };

    double h1Error (const H1Row& row, float renderAmount, bool print)
    {
        Instance in; in.cleanPatch();
        for (int osc = 0; osc < 2; ++osc)
        {
            const std::string pre = osc == 0 ? "oscA" : "oscB";
            in.setChoice (pre + "Orbit", 0);
            in.setReal (pre + "OrbAspect", 1.0f); in.setReal (pre + "OrbRot", 0.0f);
            in.setReal (pre + "OrbCX", 0.0f); in.setReal (pre + "OrbCY", 0.0f);
            in.setReal (pre + "Pos", 1.0f);
            in.setChoice (pre + "Quality", 0);   // Bandlimited = analytic 1× in Round A
            in.p.harnessTerrainOverride[osc].store (static_cast<int> (TerrainKind::HarnessIdentityX));
        }
        in.setReal ("oscAUnison", (float) row.unison); in.setReal ("oscADetune", 0.2f); in.setReal ("oscAWidth", 0.5f);
        in.setChoice ("oscAWarpType", row.warpIndex); in.setReal ("oscAWarpAmt", renderAmount);
        in.p.harnessPreFilterTap.store (true);
        RenderSpec s; s.seconds = 1.0; s.note = 60; s.velocity = 1.0f;
        auto r = render (in, s);

        theta_ref::OscParams a, b;
        a.frequency = in.p.getTuningEngine()->getFrequency (60);
        a.unison = row.unison; a.detune = 0.2f; a.width = 0.5f; a.warp = row.warp; a.warpAmount = row.amount; a.startPhase = 0.25;
        b.frequency = a.frequency; b.unison = 1; b.startPhase = 0.25;
        std::vector<double> refL, refR;
        theta_ref::renderCosReference (a, b, (int) r.L.size(), r.fs, refL, refR);

        const size_t from = (size_t) (0.05 * r.fs);
        auto err = [&] (const std::vector<double>& y, const std::vector<double>& ref) {
            double num = 0, den = 0;
            for (size_t i = from; i < y.size(); ++i) { num += y[i] * ref[i]; den += ref[i] * ref[i]; }
            const double g = den > 0 ? num / den : 0.0;
            double e = 0, p = 0;
            for (size_t i = from; i < y.size(); ++i) { const double d = y[i] - g * ref[i]; e += d * d; p += ref[i] * ref[i]; }
            return std::make_pair (spectrum::db (e, p), g);
        };
        const auto eL = err (r.L, refL), eR = err (r.R, refR);
        const double worst = std::max (eL.first, eR.first);
        if (std::getenv ("STRATA_H1_DUMP") != nullptr)
        {
            const double g = eL.second;
            std::printf ("    dump %s u%d: ampAttack=%.4f ampSustain=%.4f ampDecay=%.4f; y/(g*ref) at n=1,25,50,100,200,400,800,1600,3200:", row.name, row.unison,
                         in.getReal ("ampAttack"), in.getReal ("ampSustain"), in.getReal ("ampDecay"));
            for (size_t n : { 1, 25, 50, 100, 200, 400, 800, 1600, 3200 }) std::printf (" %.4f", r.L[n] / (g * refL[n]));
            std::printf ("\n    dump %s u%d: first samples (y, g*ref, d):", row.name, row.unison);
            for (size_t i = 0; i < 4; ++i) std::printf (" (%.6f %.6f %.1e)", r.L[i], g * refL[i], r.L[i] - g * refL[i]);
            std::printf ("\n    per-0.1s error dB:");
            for (size_t seg = 0; seg < 10; ++seg)
            {
                double e = 0, pw = 0;
                for (size_t i = seg * 4800; i < (seg + 1) * 4800 && i < r.L.size(); ++i) { const double d = r.L[i] - g * refL[i]; e += d * d; pw += refL[i] * refL[i]; }
                std::printf (" %.1f", spectrum::db (e, pw));
            }
            std::vector<double> d (r.L.size()); for (size_t i = 0; i < d.size(); ++i) d[i] = r.L[i] - g * refL[i];
            const auto ds = spec (d, from, 32768, 32768);
            std::printf ("\n    error peak at %.1f Hz\n", spectrum::peakFrequencyInterpolated (ds, r.fs / 32768.0, 1.0, 20000.0));
        }
        if (print)
            std::printf ("  H1 %-10s unison %d: L %.1f dB (g=%.4f)  R %.1f dB (g=%.4f)\n", row.name, row.unison, eL.first, eL.second, eR.first, eR.second);
        if (row.unison == 1 && row.warp == theta_ref::WarpType::Off && print)
        {
            const double f = measureFrequency (r.L, r.fs, a.frequency, from);
            const double c = spectrum::cents (f, 261.6256);
            check (std::abs (c) <= 1.0, fmt ("[H1] C4 pitch = %.4f Hz, %+.3f cent from 261.6256 (need |d| <= 1)", f, c));
        }
        return worst;
    }

    void gateH1()
    {
        std::printf ("\n== H1 theta parity (Identity-X + centred unit circle vs DC-blocked analytic cosine, <= -80 dB) ==\n");
        const H1Row rows[] = {
            { "Off",    theta_ref::WarpType::Off,    0, 0.0f, 1 },
            { "Sync",   theta_ref::WarpType::Sync,   1, 0.5f, 1 },
            { "Bend",   theta_ref::WarpType::Bend,   2, 0.5f, 1 },
            { "FM",     theta_ref::WarpType::FM,     3, 0.5f, 1 },
            { "Window", theta_ref::WarpType::Window, 4, 0.5f, 1 },
        };
        for (int unison : { 1, 4 })
            for (auto row : rows)
            {
                row.unison = unison;
                const double e = h1Error (row, row.amount, true);
                check (e <= -80.0, fmt ("[H1] %s unison %d: %.1f dB (need <= -80)", row.name, unison, e));
            }
        // negative control: Bend 0.5 reference vs a Bend 0.6 render must fail
        H1Row neg { "Bend-ctrl", theta_ref::WarpType::Bend, 2, 0.5f, 1 };
        const double e = h1Error (neg, 0.6f, false);
        checkFailsAsExpected (e <= -80.0, fmt ("[H1 neg] Bend 0.5 reference vs Bend 0.6 render: %.1f dB", e));
    }

    //==========================================================================
    // ── H2: symmetry gate (h1 >= max - 6 dB in >= 95 % of windows) ───────────

    struct H2Result { double passFraction; double worstH1RelDb; int windows; };

    H2Result h2Analyse (const std::vector<double>& y, double fs, double f0)
    {
        const size_t frame = 8192, hop = (size_t) (0.1 * fs);
        const double binHz = fs / double (frame);
        int windows = 0, passed = 0; double worst = 0.0;
        for (size_t from = (size_t) (0.05 * fs); from + frame <= y.size(); from += hop)
        {
            const auto s = spec (y, from, frame, frame);
            const double h1 = spectrum::peakPowerNear (s, binHz, f0, 1.5 * binHz);
            double maxP = 0.0;
            for (size_t b = (size_t) (0.5 * f0 / binHz); b < s.size(); ++b) maxP = std::max (maxP, s[b]);
            const double rel = spectrum::db (h1, maxP);
            ++windows;
            if (rel >= -6.0) ++passed;
            worst = std::min (worst, rel);
        }
        return { windows ? double (passed) / windows : 0.0, worst, windows };
    }

    H2Result h2Render (Instance& in, int note = 60)
    {
        in.p.harnessPreFilterTap.store (true);
        RenderSpec s; s.seconds = 1.0; s.note = note; s.velocity = 1.0f;
        auto r = render (in, s);
        return h2Analyse (r.L, r.fs, in.p.getTuningEngine()->getFrequency (note));
    }

    void gateH2()
    {
        std::printf ("\n== H2 symmetry (6 x 11 grid at defaults, C4, Hann 8192 / hop 100 ms, h1 >= max - 6 dB in >= 95 %% of windows) ==\n");
        int passCount = 0;
        for (int t = 0; t < kNumAnalyticTerrains; ++t)
            for (int o = 0; o < kNumOrbitKinds; ++o)
            {
                Instance in; in.cleanPatch(); in.setTerrainOrbit (0, t, o);
                const auto r = h2Render (in);
                const bool ok = r.passFraction >= 0.95;
                if (ok) ++passCount;
                std::printf ("  [H2] %-13s x %-13s h1-max worst = %6.1f dB, %3.0f %% windows %s\n",
                             kTerrainNames[t], kOrbitNames[o], r.worstH1RelDb, 100.0 * r.passFraction, ok ? "PASS" : "FAIL");
                if (! ok)
                {
                    std::printf ("      neighbouring centres (+-0.1):");
                    for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx)
                    {
                        if (dx == 0 && dy == 0) continue;
                        Instance n; n.cleanPatch(); n.setTerrainOrbit (0, t, o);
                        n.setReal ("oscAOrbCX", 0.13f + 0.1f * dx); n.setReal ("oscAOrbCY", 0.21f + 0.1f * dy);
                        const auto rn = h2Render (n);
                        std::printf ("  (%+.2f,%+.2f) %.1f dB", 0.13 + 0.1 * dx, 0.21 + 0.1 * dy, rn.worstH1RelDb);
                    }
                    std::printf ("\n");
                }
            }
        check (passCount == 66, fmt ("[H2] terrain x orbit grid: %d / 66 pairs pass", passCount));

        // Factory presets through the disk-free apply loop (RESEARCH §2.11)
        {
            Instance in;
            const auto defs = FactoryPresets::build (in.p.getAPVTS());
            std::printf ("  presets: %d (%s)\n", (int) defs.size(), defs.empty() ? "-" : defs[0].name.toRawUTF8());
            int presetPass = 0;
            for (const auto& def : defs)
            {
                Instance pi;
                for (auto* prm : pi.p.getParameters())
                    if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
                        if (! pi.p.getPresetManager().excludedParameterIds.contains (rp->getParameterID()))
                            rp->setValueNotifyingHost (rp->getDefaultValue());
                for (const auto& kv : def.parameters)
                    if (auto* prm = pi.p.getAPVTS().getParameter (kv.first)) prm->setValueNotifyingHost (kv.second);
                pi.cleanPatch();
                const auto r = h2Render (pi);
                const bool ok = r.passFraction >= 0.95;
                if (ok) ++presetPass;
                std::printf ("  [H2] preset %-10s h1-max worst = %6.1f dB, %3.0f %% windows %s\n", def.name.toRawUTF8(), r.worstH1RelDb, 100.0 * r.passFraction, ok ? "PASS" : "FAIL");
            }
            check (presetPass == (int) defs.size(), fmt ("[H2] factory presets: %d / %d pass (presets: %d — Init only; Phase 4.1 re-runs over the full bank)", presetPass, (int) defs.size(), (int) defs.size()));
        }

        // negative control: Sine Product + Ellipse, Centre (0, 0), Aspect 1 → period halves → h1 absent
        {
            Instance in; in.cleanPatch(); in.setTerrainOrbit (0, 0, 0);
            in.setReal ("oscAOrbCX", 0.0f); in.setReal ("oscAOrbCY", 0.0f); in.setReal ("oscAOrbAspect", 1.0f);
            const auto r = h2Render (in);
            checkFailsAsExpected (r.passFraction >= 0.95, fmt ("[H2 neg] centred circle over Sine Product: h1-max = %.1f dB, %.0f %% windows", r.worstH1RelDb, 100.0 * r.passFraction));
        }
    }

    //==========================================================================
    // ── H5: zipper (10 % steps every 100 ms, max |y[n]-y[n-1]| <= 0.1) ───────

    const char* kH5Params[22] = { "oscAPos", "oscAOrbAspect", "oscAOrbRot", "oscAOrbCX", "oscAOrbCY", "oscAOrbMod",
                                  "oscATerFreq", "oscATerModX", "oscATerModY", "oscAOrbFeedback", "oscATerSat",
                                  "oscBPos", "oscBOrbAspect", "oscBOrbRot", "oscBOrbCX", "oscBOrbCY", "oscBOrbMod",
                                  "oscBTerFreq", "oscBTerModX", "oscBTerModY", "oscBOrbFeedback", "oscBTerSat" };

    /** Full-level patch on the real output (tap off): the default gain staging
        (level 0.8 × pan 0.707 × sustain 0.7 × velocity 0.8 × master 0.8 × mix 0.5)
        shrinks a raw 0.6 step below the 0.1 threshold, which made the ramp-0 control
        vacuous — so the zipper rows run at unity gain, A or B alone. */
    void h5Patch (Instance& in, bool oscB)
    {
        in.setReal ("oscAPhase", 0.25f); in.setReal ("oscBPhase", 0.25f); in.p.setHarnessPhaseSeed (opt.seed);
        in.setReal ("oscALevel", 1.0f); in.setReal ("oscBLevel", 1.0f); in.setReal ("masterVol", 1.0f);
        in.setReal ("ampAttack", 0.001f); in.setReal ("ampSustain", 1.0f);
        in.setReal ("oscMix", oscB ? 1.0f : 0.0f);
    }

    struct ZipperResult { double absStep; double excessRatio; };

    /** Zipper = an excess sample step AT the automation instant. For each step instant
        t_k the max |y[n]−y[n−1]| inside the 10 ms after it is compared with the larger
        of the two adjacent 100 ms plateaus (the tone's own steps at those parameter
        values — at unity gain a Terrain Freq of 8 has natural steps > 0.1, so the absolute
        criterion cannot separate zipper from the waveform). Returns the worst ratio and
        the absolute max step (the REQUIREMENTS literal, printed for the record). */
    ZipperResult zipperOf (const std::vector<double>& y, double fs, const std::vector<int>& stepSamples, int plateauSamples)
    {
        auto segMax = [&] (int from, int to) { double m = 0; for (int i = std::max (from, 1); i < to && i < (int) y.size(); ++i) m = std::max (m, std::abs (y[i] - y[i - 1])); return m; };
        const int after = (int) (0.010 * fs), guard = (int) (0.020 * fs);
        double worst = 0.0;
        for (int t : stepSamples)
        {
            if (t <= 0) continue;
            const double a = segMax (t, t + after);
            const double before = segMax (std::max (0, t - plateauSamples + guard), t);
            const double next = segMax (t + guard, t + plateauSamples);
            const double plateau = std::max (before, next);
            if (plateau > 1.0e-6) worst = std::max (worst, a / plateau);
        }
        return { segMax ((int) (0.05 * fs), (int) y.size()), worst };
    }

    ZipperResult h5Step (const char* id, bool rampZero)
    {
        Instance in;
        h5Patch (in, std::strncmp (id, "oscB", 4) == 0);
        if (rampZero) in.p.harnessRampSeconds.store (0.0f);
        in.setNorm (id, 0.0f);
        in.prepare (48000.0, 480);       // 100 ms = 10 blocks exactly
        RenderSpec s; s.seconds = 2.0; s.note = 60; s.velocity = 1.0f;
        std::vector<int> steps;
        s.onBlock = [&] (int start) {
            const int k = start / 4800;
            if (start % 4800 == 0)
            {
                const int m = k % 20;
                in.setNorm (id, m < 10 ? 0.1f * m : 1.0f - 0.1f * (m - 10));
                steps.push_back (start);
            }
        };
        auto r = render (in, s);
        const auto zl = zipperOf (r.L, r.fs, steps, 4800), zr = zipperOf (r.R, r.fs, steps, 4800);
        return { std::max (zl.absStep, zr.absStep), std::max (zl.excessRatio, zr.excessRatio) };
    }

    void gateH5()
    {
        std::printf ("\n== H5 zipper (22 destinations + ModWheel route stepped 10 %% / 100 ms at C4, unity gain; excess step at the instant <= 1.5 x plateau) ==\n");
        double worst = 0.0, worstAbs = 0.0; const char* worstId = "";
        for (const char* id : kH5Params)
        {
            const auto z = h5Step (id, false);
            std::printf ("  [H5] %-16s excess ratio = %.2f   (abs max step %.4f)\n", id, z.excessRatio, z.absStep);
            if (z.excessRatio > worst) { worst = z.excessRatio; worstId = id; }
            worstAbs = std::max (worstAbs, z.absStep);
        }
        check (worst <= 1.5, fmt ("[H5] worst destination %s: excess step ratio %.2f (need <= 1.5); abs max step over all rows %.4f at unity gain", worstId, worst, worstAbs));
        const auto ctrl = h5Step ("oscAOrbCX", true);
        checkFailsAsExpected (ctrl.excessRatio <= 1.5, fmt ("[H5 neg] harnessRampSeconds = 0 on oscAOrbCX: excess ratio %.2f (abs step %.4f)", ctrl.excessRatio, ctrl.absStep));

        // ModWheel route: CC1 stepped by 16 (8 values) every 100 ms → OscA Orbit CX at amount 1
        {
            Instance in; h5Patch (in, false);
            in.setChoice ("modSlot0Src", kSrcModWheel); in.setChoice ("modSlot0Dst", kDstOscAOrbCX);
            in.setReal ("modSlot0Amt", 1.0f); in.setNorm ("modSlot0On", 1.0f);
            in.prepare (48000.0, 480);
            RenderSpec s; s.seconds = 2.0; s.note = 60; s.velocity = 1.0f;
            std::vector<int> steps;
            for (int k = 0; k < 16; ++k)
            {
                s.extra.push_back ({ k * 4800, juce::MidiMessage::controllerEvent (1, 1, (k % 8) * 16) });
                steps.push_back (k * 4800);
            }
            auto r = render (in, s);
            const auto z = zipperOf (r.L, r.fs, steps, 4800);
            check (z.excessRatio <= 1.5, fmt ("[H5] ModWheel -> OscA Orbit CX, CC1 stepped by 16 every 100 ms: excess ratio %.2f (need <= 1.5; abs step %.4f)", z.excessRatio, z.absStep));
        }

        // FUNC-05 sideband proof: LFO1 at 40 Hz (10 Hz x 2^(2 · 1) via Velocity -> LFO1 Rate) on OscA Orbit CX
        {
            Instance in; in.cleanPatch();
            in.setReal ("lfo1Rate", 10.0f); in.setChoice ("lfo1Shape", 0);
            in.setChoice ("modSlot0Src", kSrcVelocity); in.setChoice ("modSlot0Dst", kDstLFO1Rate); in.setReal ("modSlot0Amt", 1.0f); in.setNorm ("modSlot0On", 1.0f);
            in.setChoice ("modSlot1Src", kSrcLFO1); in.setChoice ("modSlot1Dst", kDstOscAOrbCX); in.setReal ("modSlot1Amt", 0.5f); in.setNorm ("modSlot1On", 1.0f);
            in.p.harnessPreFilterTap.store (true);
            RenderSpec s; s.seconds = 2.0; s.note = 60; s.velocity = 1.0f;
            auto r = render (in, s);
            const size_t nfft = 65536, frame = 48000;
            const auto sp = spec (r.L, (size_t) (0.5 * r.fs), frame, nfft);
            const double binHz = r.fs / double (nfft);
            const double f0 = in.p.getTuningEngine()->getFrequency (60);
            bool ok = true;
            for (int h = 1; h <= 2; ++h)
            {
                const double pc = spectrum::peakPowerNear (sp, binHz, h * f0, 2.0);
                const double pl = spectrum::peakPowerNear (sp, binHz, h * f0 - 40.0, 2.0);
                const double pu = spectrum::peakPowerNear (sp, binHz, h * f0 + 40.0, 2.0);
                const double dl = spectrum::db (pl, pc), du = spectrum::db (pu, pc);
                std::printf ("  [H5 sidebands] h%d: -40 Hz %.1f dB, +40 Hz %.1f dB rel. h%d\n", h, dl, du, h);
                if (dl < -40.0 || du < -40.0) ok = false;
            }
            check (ok, "[H5] 40 Hz LFO on OscA Orbit CX: +-40 Hz sidebands >= -40 dB around h1 and h2 (FUNC-05)");
        }
    }

    //==========================================================================
    // ── H8: allocation (terrain / orbit changes under held notes) ────────────

    void gateH8()
    {
        std::printf ("\n== H8 allocation (operator-new family, arming thread; terrain / orbit change every 50 ms under 8 held notes, 3 s) ==\n");
        Instance in;
        in.prepare (48000.0, 480);
        const int notes[8] = { 48, 52, 55, 59, 60, 64, 67, 71 };
        RenderSpec s; s.seconds = 3.0; s.note = notes[0]; s.armAllocations = true;
        for (int i = 1; i < 8; ++i) s.extra.push_back ({ 0, juce::MidiMessage::noteOn (1, notes[i], 0.8f) });
        int step = 0;
        s.onBlock = [&] (int start) {
            if (start % 2400 == 0)   // every 50 ms; setValueNotifyingHost runs OUTSIDE the armed window
            {
                in.setChoice ("oscATerrain", step % kNumAnalyticTerrains);
                in.setChoice ("oscAOrbit", (step * 7) % kNumOrbitKinds);
                in.setChoice ("oscBTerrain", (step + 3) % kNumAnalyticTerrains);
                in.setChoice ("oscBOrbit", (step * 5 + 2) % kNumOrbitKinds);
                ++step;
            }
        };
        auto r = render (in, s);
        check (r.allocations == 0, fmt ("[H8] %lld allocations counted across %d terrain / orbit changes under 8 held notes%s (need 0)",
                                        r.allocations, step, rtcheck::foreignNote().c_str()));
        check (allFinite (r.L) && allFinite (r.R), "[H8] output finite through every change");
    }

    //==========================================================================
    // ── H9: block-size invariance ────────────────────────────────────────────

    void gateH9()
    {
        std::printf ("\n== H9 block-size invariance (10 s default patch, Phase 0.25 + seed, 64 / 256 / 1024) ==\n");
        auto renderAt = [] (int block, int noteOnSample) {
            Instance in; in.setReal ("oscAPhase", 0.25f); in.setReal ("oscBPhase", 0.25f); in.p.setHarnessPhaseSeed (opt.seed);
            in.setReal ("oscBLevel", 0.8f);
            in.prepare (48000.0, block);
            RenderSpec s; s.seconds = 10.0; s.note = 60; s.noteOnSample = noteOnSample;
            return render (in, s);
        };
        const auto a = renderAt (64, 0), b = renderAt (256, 0), c = renderAt (1024, 0);
        const double dab = std::max (maxAbsDiff (a.L, b.L), maxAbsDiff (a.R, b.R));
        const double dbc = std::max (maxAbsDiff (b.L, c.L), maxAbsDiff (b.R, c.R));
        const double dac = std::max (maxAbsDiff (a.L, c.L), maxAbsDiff (a.R, c.R));
        std::printf ("  [H9] max|d| 64-256 = %.3e, 256-1024 = %.3e, 64-1024 = %.3e (rms %.4f)\n", dab, dbc, dac, rms (a.L, 0));
        check (std::max ({ dab, dbc, dac }) <= 1.0e-5, fmt ("[H9] pairwise max|d| = %.3e (need <= 1e-5)", std::max ({ dab, dbc, dac })));
        // note-on at sample 37 of block 0 (juce::Synthesiser sub-block split)
        const auto a37 = renderAt (64, 37), c37 = renderAt (1024, 37);
        const double d37 = std::max (maxAbsDiff (a37.L, c37.L), maxAbsDiff (a37.R, c37.R));
        check (d37 <= 1.0e-5, fmt ("[H9] note-on at sample 37: 64 vs 1024 max|d| = %.3e (need <= 1e-5)", d37));
        std::printf ("  note: the unseeded path (osc?Phase = 0, harnessPhaseSeed = 0) is address-seeded and non-deterministic by design\n");
    }

    //==========================================================================
    // ── FUNC-02 / FUNC-03 centroids ──────────────────────────────────────────

    struct CentroidRow { double centroid; std::vector<double> partialDb; };   // partials h1..h12 in dB re the strongest bin

    CentroidRow centroidRender (std::function<void (Instance&)> setup)
    {
        Instance in; in.cleanPatch(); in.setTerrainOrbit (0, 0, 0);
        setup (in);
        in.p.harnessPreFilterTap.store (true);
        RenderSpec s; s.seconds = 1.0; s.note = 60; s.velocity = 1.0f;
        auto r = render (in, s);
        CentroidRow row; row.centroid = centroidOf (r.L, r.fs);
        const size_t nfft = 32768;
        const auto sp = spec (r.L, (size_t) (0.1 * r.fs), nfft, nfft);
        const double binHz = r.fs / double (nfft), f0 = in.p.getTuningEngine()->getFrequency (60);
        double maxP = 0.0; for (size_t b = 1; b < sp.size(); ++b) maxP = std::max (maxP, sp[b]);
        for (int h = 1; h <= 12; ++h) row.partialDb.push_back (spectrum::db (spectrum::peakPowerNear (sp, binHz, h * f0, 1.5 * binHz), maxP));
        return row;
    }

    /** Largest level change (dB) of any partial h1..h12 that sits above −40 dB in either row. */
    double partialMove (const CentroidRow& a, const CentroidRow& b)
    {
        double m = 0.0;
        for (size_t h = 0; h < a.partialDb.size(); ++h)
            if (a.partialDb[h] > -40.0 || b.partialDb[h] > -40.0)
                m = std::max (m, std::abs (a.partialDb[h] - b.partialDb[h]));
        return m;
    }

    void gateCentroids()
    {
        std::printf ("\n== FUNC-02 / FUNC-03 centroids (Mod X / Mod Y / Orbit Mod over {0, .25, .5, .75, 1}: max pairwise delta >= 5 %%; Orbit Size monotone) ==\n");
        // Endpoints alone are degenerate for the phase-type shape inputs (Sine Product Mod X
        // 0 vs 1 is a sign flip; Ridged Cosines Mod X 0 == 1; Radial Rings Mod Y spans a full
        // turn so 0 == 0.5 == 1 up to sign), so the gate reads a five-point sweep and requires
        // the spectrum to move somewhere across the range.
        // Verdict per shape input: centroid delta >= 5 % (the plan's metric) OR some partial
        // above -40 dB moves by >= 3 dB across the sweep (the audibility-grounded metric —
        // a centroid can sit still while partials trade energy; JND ~ 1 dB). Both printed.
        auto sweep = [&] (const char* what, std::function<void (Instance&, float)> set) {
            CentroidRow c[5];
            for (int i = 0; i < 5; ++i) c[i] = centroidRender ([&] (Instance& in) { set (in, 0.25f * i); });
            double cmax = 0.0, cmin = 1.0e9, move = 0.0;
            for (int i = 0; i < 5; ++i) { cmax = std::max (cmax, c[i].centroid); cmin = std::min (cmin, c[i].centroid); for (int j = i + 1; j < 5; ++j) move = std::max (move, partialMove (c[i], c[j])); }
            const double delta = (cmax - cmin) / cmax;
            std::printf ("  %-28s centroid 0/.25/.5/.75/1 = %6.0f %6.0f %6.0f %6.0f %6.0f Hz  delta %.1f %%  max partial move %.1f dB%s\n",
                         what, c[0].centroid, c[1].centroid, c[2].centroid, c[3].centroid, c[4].centroid, 100.0 * delta, move,
                         (delta >= 0.05 || move >= 3.0) ? "" : "  BELOW");
            return delta >= 0.05 || move >= 3.0;
        };
        int fails = 0;
        for (int t = 0; t < kNumAnalyticTerrains; ++t)
        {
            const bool dx = sweep (fmt ("%s ModX", kTerrainNames[t]).c_str(), [t] (Instance& in, float v) { in.setChoice ("oscATerrain", t); in.setReal ("oscATerModX", v); });
            const bool dy = sweep (fmt ("%s ModY", kTerrainNames[t]).c_str(), [t] (Instance& in, float v) { in.setChoice ("oscATerrain", t); in.setReal ("oscATerModY", v); });
            if (! dx) ++fails;
            if (! dy) ++fails;
        }
        check (fails == 0, fmt ("[FUNC-02] every terrain's Mod X and Mod Y change the spectrum (centroid >= 5 %% or a partial moves >= 3 dB): %d of 12 below", fails));
        int ofails = 0;
        for (int o = 1; o < kNumOrbitKinds; ++o)
        {
            if (! sweep (fmt ("%s OrbMod", kOrbitNames[o]).c_str(), [o] (Instance& in, float v) { in.setChoice ("oscAOrbit", o); in.setReal ("oscAOrbMod", v); }))
                ++ofails;
        }
        std::printf ("  Ellipse: Orbit Mod inert (documented)\n");
        check (ofails == 0, fmt ("[FUNC-03] every non-Ellipse orbit's Orbit Mod changes the spectrum (centroid >= 5 %% or a partial moves >= 3 dB): %d of 10 below", ofails));
        // Orbit Size 0.05 → 1.0 in 8 steps over Sine Product: monotone non-decreasing (0.5 % tolerance per step)
        {
            double prev = 0.0; bool mono = true; std::string line = "  Orbit Size centroids:";
            for (int i = 0; i < 8; ++i)
            {
                const float pos = float (i) / 7.0f;
                const double c = centroidRender ([pos] (Instance& in) { in.setReal ("oscAPos", pos); }).centroid;
                line += fmt (" %.0f", c);
                if (i > 0 && c < prev * (1.0 - 0.005)) mono = false;
                prev = c;
            }
            std::printf ("%s Hz\n", line.c_str());
            check (mono, "[FUNC-03] Orbit Size 0.05 -> 1.0 over Sine Product: centroid monotone non-decreasing (0.5 % tolerance per step)");
        }
    }

    //==========================================================================
    // ── Phase 2.2: H4 pitch tracking, H3 feedback grid, DSP-07 saturation ────

    /** Non-harmonic energy relative to total over [0, fmax] on a Hann-32768 frame:
        every bin within ± 3 bins of h·f0 counts as harmonic (Hann main lobe ± 2). */
    double nonHarmonicDb (const std::vector<double>& y, double fs, double f0, double fmax)
    {
        const size_t nfft = 32768;
        const auto sp = spec (y, (size_t) (0.1 * fs), nfft, nfft);
        const double binHz = fs / double (nfft);
        double harm = 0.0, non = 0.0;
        for (size_t b = 1; b < sp.size() && b * binHz <= fmax; ++b)
        {
            const double h = b * binHz / f0;
            const double dist = std::abs (h - std::round (h)) * f0 / binHz;   // bins from the nearest harmonic
            (dist <= 3.0 ? harm : non) += sp[b];
        }
        return spectrum::db (non, harm + non);
    }

    int partialCount (const std::vector<double>& y, double fs, double f0)
    {
        const size_t nfft = 32768;
        const auto sp = spec (y, (size_t) (0.1 * fs), nfft, nfft);
        return spectrum::partialsAbove (sp, fs / double (nfft), f0, std::min (20000.0, 0.5 * fs), -40.0);
    }

    Rendered tapRender (Instance& in, int note, double seconds, float velocity = 1.0f)
    {
        in.p.harnessPreFilterTap.store (true);
        RenderSpec s; s.seconds = seconds; s.note = note; s.velocity = velocity;
        return render (in, s);
    }

    void gateH4()
    {
        std::printf ("\n== H4 pitch tracking (Track 1: partials > -40 dB at C2 / C4 / C6 differ <= 2 at F = 1 and F = 4; Track 0 / F = 8 / C6 control) ==\n");
        auto countsAt = [] (float F, float track, int counts[3], std::string* levels) {
            for (int n = 0; n < 3; ++n)
            {
                Instance in; in.cleanPatch(); in.setTerrainOrbit (0, 0, 0);
                in.setReal ("oscATerFreq", F); in.setReal ("oscATerTrack", track);
                auto r = tapRender (in, kCNotes[n], 1.0);
                const double f0 = in.p.getTuningEngine()->getFrequency (kCNotes[n]);
                counts[n] = partialCount (r.L, r.fs, f0);
                if (levels != nullptr)
                {
                    const size_t nfft = 32768;
                    const auto sp = spec (r.L, (size_t) (0.1 * r.fs), nfft, nfft);
                    double maxP = 0.0; for (size_t b = 1; b < sp.size(); ++b) maxP = std::max (maxP, sp[b]);
                    *levels += fmt (" %s:", kNoteNames[n]);
                    for (int h = 1; h <= 6; ++h) *levels += fmt (" %.0f", spectrum::db (spectrum::peakPowerNear (sp, r.fs / double (nfft), h * f0, 2.0), maxP));
                }
            }
        };
        for (float F : { 1.0f, 4.0f })
        {
            int counts[3]; std::string levels;
            countsAt (F, 1.0f, counts, &levels);
            std::printf ("  [H4] F = %.0f, Track 1: h1..h6 levels (dB re max)%s\n", F, levels.c_str());
            const int spread = std::max ({ counts[0], counts[1], counts[2] }) - std::min ({ counts[0], counts[1], counts[2] });
            check (spread <= 2, fmt ("[H4] F = %.0f, Track 1: partials > -40 dB at C2 / C4 / C6 = %d / %d / %d (spread %d, need <= 2)", F, counts[0], counts[1], counts[2], spread));
            // diagnostic rows for verify / Round B (not gated): Track 0 and Track 0.5
            for (float tr : { 0.0f, 0.5f })
            {
                int c[3]; countsAt (F, tr, c, nullptr);
                std::printf ("  [H4 diag] F = %.0f, Track %.1f: partials > -40 dB at C2 / C4 / C6 = %d / %d / %d\n", F, tr, c[0], c[1], c[2]);
            }
        }
        // negative control: Track 0 at F = 8 → non-harmonic energy >= 20 dB above the Track 1 run.
        // The plan's row (Ellipse, C6) is measured first; Ellipse at F = 8 / C6 tops out near
        // 17 kHz and never reaches Nyquist at 1x, so the control also runs Epitrochoid 7
        // (K = 8) at C6 and C8 and gates on the largest rise it finds.
        double bestRise = -1.0e9; std::string bestAt;
        for (int orbit : { 0, 5 })
            for (int note : { 84, 108 })
            {
                double nh[2];
                for (int track = 0; track < 2; ++track)
                {
                    Instance in; in.cleanPatch(); in.setTerrainOrbit (0, 0, orbit);
                    in.setReal ("oscATerFreq", 8.0f); in.setReal ("oscATerTrack", (float) track);
                    auto r = tapRender (in, note, 1.0);
                    nh[track] = nonHarmonicDb (r.L, r.fs, in.p.getTuningEngine()->getFrequency (note), 0.5 * r.fs);
                }
                std::printf ("  [H4 neg] F = 8, %s, %s: non-harmonic / total = %.1f dB (Track 1) vs %.1f dB (Track 0), rise %.1f dB\n",
                             kOrbitNames[orbit], note == 84 ? "C6" : "C8", nh[1], nh[0], nh[0] - nh[1]);
                if (nh[0] - nh[1] > bestRise) { bestRise = nh[0] - nh[1]; bestAt = fmt ("%s %s", kOrbitNames[orbit], note == 84 ? "C6" : "C8"); }
            }
        check (bestRise >= 20.0, fmt ("[H4 neg] Track 0 raises non-harmonic energy by %.1f dB over Track 1 at %s (need >= 20)", bestRise, bestAt.c_str()));
    }

    void gateH3()
    {
        std::printf ("\n== H3 feedback (Feedback {.25 .5 .75 1} x Damp {0 .5 1} x 6 terrains x 11 orbits x {C2 C4 C6}: finite, |y| <= 1 (0.5 s, pre-blocker), |DC| < 1e-3 over the last 250 ms of 1 s; fb = 0 memcmp) ==\n");
        const auto t0 = std::chrono::steady_clock::now();
        const float fbs[4] = { 0.25f, 0.5f, 0.75f, 1.0f }, damps[3] = { 0.0f, 0.5f, 1.0f };
        // Two passes over the grid: (a) DC blocker bypassed → |y| <= 1 is the scan-value
        // bound ARCH Core 4 argues (the blocker itself is LTI with time-domain gain <= 2,
        // so a post-blocker bound of 1 is not a property of the oscillator); (b) the
        // production path → finite + |DC| < 1e-3 (Hann-windowed) + the post-blocker peak reported.
        int renders = 0, badFinite = 0, badPeak = 0, badDc = 0, boundedJumps = 0;
        double worstPeak = 0.0, worstDc = 0.0, worstPostPeak = 0.0, worstJump = 0.0;
        std::string worstPeakAt, worstDcAt, worstPostAt;
        for (int t = 0; t < kNumAnalyticTerrains; ++t)
            for (int o = 0; o < kNumOrbitKinds; ++o)
            {
                Instance in; in.cleanPatch(); in.setTerrainOrbit (0, t, o);
                in.setReal ("oscALevel", 1.0f); in.setReal ("oscAPan", -1.0f);   // tap L = the raw oscillator output
                in.prepare (48000.0, 512);
                for (float fb : fbs) for (float damp : damps) for (int n = 0; n < 3; ++n)
                {
                    in.setReal ("oscAOrbFeedback", fb); in.setReal ("oscAOrbFbDamp", damp);
                    in.p.harnessPreFilterTap.store (true);
                    const std::string at = fmt ("%s x %s fb %.2f damp %.1f %s", kTerrainNames[t], kOrbitNames[o], fb, damp, kNoteNames[n]);
                    for (int pass = 0; pass < 2; ++pass)
                    {
                        in.p.harnessDcBlockerBypass.store (pass == 0);
                        // pass 0: 0.5 s peak scan; pass 1: 1 s — the damp-1 attractor transient
                        // lasts ~0.5 s through the 5 Hz blocker, so DC is read over [0.75, 1.0] s
                        RenderSpec s; s.seconds = pass == 0 ? 0.55 : 1.05; s.note = kCNotes[n]; s.velocity = 1.0f; s.noteOffAt = pass == 0 ? 0.5 : 1.0;
                        auto r = render (in, s);
                        const double peak = maxAbs (r.L);
                        if (pass == 0)
                        {
                            if (peak > 1.0) ++badPeak;
                            if (peak > worstPeak) { worstPeak = peak; worstPeakAt = at; }
                            continue;
                        }
                        ++renders;
                        if (! allFinite (r.L)) ++badFinite;
                        std::vector<double> held (r.L.begin(), r.L.begin() + 48000);
                        const double dc = std::abs (dcOf (held, r.fs, 0.25));
                        // bounded chaotic jumps (the displacement hops to a new region and the
                        // blocker re-settles): any earlier 250 ms window after 0.5 s above 1e-2 — reported, not gated
                        {
                            std::vector<double> mid (r.L.begin() + 24000, r.L.begin() + 36000);
                            const double dcMid = std::abs (dcOf (mid, r.fs, 0.25));
                            if (dcMid > 1.0e-2) { ++boundedJumps; worstJump = std::max (worstJump, dcMid); }
                        }
                        if (dc >= 1.0e-3)
                        {
                            ++badDc;
                            if (std::getenv ("STRATA_H3_DUMP") != nullptr)
                            {
                                // 2 s render: DC per 250 ms window — runaway grows, wander stays bounded
                                in.p.harnessDcBlockerBypass.store (false);
                                RenderSpec s2; s2.seconds = 2.0; s2.note = kCNotes[n]; s2.velocity = 1.0f;
                                auto r2 = render (in, s2);
                                std::printf ("    DC-dump %s: |DC| %.2e; 2 s render DC per 250 ms:", at.c_str(), dc);
                                for (int w = 0; w < 8; ++w)
                                {
                                    std::vector<double> seg (r2.L.begin() + w * 12000, r2.L.begin() + (w + 1) * 12000);
                                    std::printf (" %+.3f", dcOf (seg, r2.fs, 0.25));
                                }
                                std::printf ("  peak %.3f\n", maxAbs (r2.L));
                            }
                        }
                        if (dc > worstDc) { worstDc = dc; worstDcAt = at; }
                        if (peak > worstPostPeak) { worstPostPeak = peak; worstPostAt = at; }
                    }
                }
                in.p.harnessDcBlockerBypass.store (false);
            }
        std::printf ("  [H3] %d grid points x 2 passes in %.1f s; worst pre-blocker |y| = %.4f (%s); worst post-blocker |y| = %.4f (%s); worst |DC| = %.2e (%s); bounded DC jumps in [0.5, 0.75] s: %d (worst %.3f)\n",
                     renders, secondsSince (t0), worstPeak, worstPeakAt.c_str(), worstPostPeak, worstPostAt.c_str(), worstDc, worstDcAt.c_str(), boundedJumps, worstJump);
        check (badFinite == 0, fmt ("[H3] every sample finite (%d renders with NaN/Inf)", badFinite));
        check (badPeak == 0, fmt ("[H3] |y| <= 1 at the oscillator output (pre-blocker scan sum) in every render (%d over; worst %.4f)", badPeak, worstPeak));
        check (badDc == 0, fmt ("[H3] |DC| < 1e-3 (Hann-windowed) over the last 250 ms in every render (%d over; worst %.2e)", badDc, worstDc));

        // memcmp row: Feedback 0 vs harnessFeedbackPathEnabled = false on the 66 pairs, 1 s, seed pinned
        int identical = 0;
        for (int t = 0; t < kNumAnalyticTerrains; ++t)
            for (int o = 0; o < kNumOrbitKinds; ++o)
            {
                Instance a; a.cleanPatch(); a.setTerrainOrbit (0, t, o); a.setReal ("oscAOrbFeedback", 0.0f);
                Instance b; b.cleanPatch(); b.setTerrainOrbit (0, t, o); b.setReal ("oscAOrbFeedback", 0.0f);
                b.p.harnessFeedbackPathEnabled.store (false);
                auto ra = tapRender (a, 60, 1.0), rb = tapRender (b, 60, 1.0);
                if (bitIdentical (ra.L, rb.L) && bitIdentical (ra.R, rb.R)) ++identical;
                else std::printf ("  [H3] memcmp differs: %s x %s max|d| = %.3e\n", kTerrainNames[t], kOrbitNames[o], maxAbsDiff (ra.L, rb.L));
            }
        check (identical == 66, fmt ("[H3] Feedback 0 byte-identical to feedbackPathEnabled = false on %d / 66 pairs", identical));

        // Nyquist-hunting control: Damp 0, Feedback 1, at 1x (Quality Bandlimited = the
        // analytic 1x path in Round A): the hunting mode of a per-sub-sample loop sits at
        // the OVERSAMPLED Nyquist, which the halfband decimator removes at 2x / 4x, so the
        // two-sample average can only be observed doing its job at 1x (the 2x figure is
        // printed for the record). The plan's single row (Ridged Cosines + Epitrochoid 5,
        // C4, F = 1) is measured first; because the loop gain scales with the terrain
        // slope (~ πF), the control also sweeps F ∈ {1, 4, 8} x {C4, C6} over the six
        // terrains and reports the largest rise it can find.
        {
            auto nyquistPeak = [] (int terrain, int orbit, float F, int note, bool single, double& absNyq, int quality = 0) {
                Instance in; in.cleanPatch(); in.setTerrainOrbit (0, terrain, orbit);
                in.setReal ("oscAOrbFeedback", 1.0f); in.setReal ("oscAOrbFbDamp", 0.0f); in.setReal ("oscATerFreq", F);
                in.setChoice ("oscAQuality", quality);
                in.p.harnessSingleSampleFeedback.store (single);
                auto r = tapRender (in, note, 1.0);
                const size_t nfft = 32768;
                const auto sp = spec (r.L, (size_t) (0.1 * r.fs), nfft, nfft);
                double maxP = 0.0; for (size_t b = 1; b < sp.size(); ++b) maxP = std::max (maxP, sp[b]);
                double ny = 0.0; for (size_t b = sp.size() - 3; b < sp.size(); ++b) ny = std::max (ny, sp[b]);
                absNyq = spectrum::db (ny, 1.0);
                return spectrum::db (ny, maxP);
            };
            double a0, a1;
            const double rel0 = nyquistPeak (3, 4, 1.0f, 60, false, a0), rel1 = nyquistPeak (3, 4, 1.0f, 60, true, a1);
            std::printf ("  [H3] plan row Ridged x Epi5 C4 F1: Nyquist-region peak rel. strongest bin: average %.1f dB, single-sample %.1f dB (rise %.1f dB)\n", rel0, rel1, a1 - a0);
            check (rel0 < 0.0, fmt ("[H3] Damp 0 / Feedback 1 / Ridged Cosines + Epitrochoid 5: fs/2 region is not the strongest bin (%.1f dB)", rel0));
            double bestRise = a1 - a0; std::string bestAt = "Ridged x Epi5 C4 F1"; double worstRel0 = rel0;
            for (int t = 0; t < kNumAnalyticTerrains; ++t)
                for (float F : { 1.0f, 4.0f, 8.0f })
                    for (int note : { 60, 84 })
                    {
                        double b0, b1;
                        const double r0 = nyquistPeak (t, 4, F, note, false, b0);
                        nyquistPeak (t, 4, F, note, true, b1);
                        worstRel0 = std::max (worstRel0, r0);
                        if (b1 - b0 > bestRise) { bestRise = b1 - b0; bestAt = fmt ("%s x Epi5 %s F%.0f", kTerrainNames[t], note == 60 ? "C4" : "C6", F); }
                    }
            std::printf ("  [H3] sweep (1x): worst average-form Nyquist peak rel. strongest bin %.1f dB; largest single-sample rise %.1f dB at %s\n", worstRel0, bestRise, bestAt.c_str());
            {
                double c0, c1;
                nyquistPeak (0, 4, 8.0f, 84, false, c0, 1); nyquistPeak (0, 4, 8.0f, 84, true, c1, 1);
                std::printf ("  [H3] at 2x the decimator removes the oversampled-Nyquist hunt: SineProduct x Epi5 C6 F8 single-sample rise %.1f dB (record only)\n", c1 - c0);
            }
            check (worstRel0 < 0.0, fmt ("[H3] two-sample average: fs/2 region never the strongest bin across the sweep (worst %.1f dB)", worstRel0));
            check (bestRise >= 20.0, fmt ("[H3 ctrl] single-sample feedback raises the Nyquist-region peak by %.1f dB at %s (need >= 20)", bestRise, bestAt.c_str()));
        }
    }

    void gateSaturation()
    {
        std::printf ("\n== DSP-07 saturation (Sat 0 memcmp-identical to the branch compiled out; Sat 1 raises the partial count) ==\n");
        Instance a; a.cleanPatch(); a.setTerrainOrbit (0, 0, 0); a.setReal ("oscATerSat", 0.0f);
        Instance b; b.cleanPatch(); b.setTerrainOrbit (0, 0, 0); b.setReal ("oscATerSat", 0.0f); b.p.harnessSaturationBypass.store (true);
        auto ra = tapRender (a, 60, 1.0), rb = tapRender (b, 60, 1.0);
        check (bitIdentical (ra.L, rb.L) && bitIdentical (ra.R, rb.R), "[DSP-07] Saturation 0 render byte-identical to harnessSaturationBypass = true");
        Instance c; c.cleanPatch(); c.setTerrainOrbit (0, 0, 0); c.setReal ("oscATerSat", 1.0f);
        auto rc = tapRender (c, 60, 1.0);
        const double f0 = a.p.getTuningEngine()->getFrequency (60);
        const int p0 = partialCount (ra.L, ra.fs, f0), p1 = partialCount (rc.L, rc.fs, f0);
        check (p1 > p0, fmt ("[DSP-07] partials > -40 dB: Sat 0 = %d, Sat 1 = %d (need more)", p0, p1));
        check (allFinite (rc.L) && maxAbs (rc.L) <= 1.0001, fmt ("[DSP-07] Sat 1 output finite, |y| max %.4f", maxAbs (rc.L)));
    }

    //==========================================================================
    // ── Phase 2.3: decimator, H6 aliasing, H7 CPU, H8 across Quality, crossfade, latency, export ──

    void gateDecimator()
    {
        std::printf ("\n== decimator (HalfbandStage2 at 96 k -> 48 k: 30 kHz alias <= -68 dB; designed latencies) ==\n");
        const auto& c = HalfbandCoeffs::get();
        std::printf ("  coefficients 2x: direct %.6f %.6f %.6f delayed %.6f %.6f | 4x: direct %.6f %.6f delayed %.6f\n",
                     c.direct2[0], c.direct2[1], c.direct2[2], c.delayed2[0], c.delayed2[1], c.direct4[0], c.direct4[1], c.delayed4[0]);
        std::printf ("  latency: L2 = %.4f base samples, L4 = %.4f base samples (report +1 constant)\n", c.latency2, c.latency4);
        check (c.latency2 <= 2.0 && c.latency4 <= 2.0 && c.latency2 > 1.0 && c.latency4 > c.latency2, fmt ("[dec] L2 = %.3f, L4 = %.3f (need 1 < L2 < L4 <= 2)", c.latency2, c.latency4));
        HalfbandStage2 st;
        const double fsOs = 96000.0; const size_t N = 65536;
        std::vector<double> y (N);
        for (size_t n = 0; n < N; ++n)
        {
            const double t0 = double (2 * n) / fsOs, t1 = double (2 * n + 1) / fsOs;
            const float e = float (std::sin (2 * spectrum::kPi * 1000.0 * t0) + std::sin (2 * spectrum::kPi * 30000.0 * t0));
            const float o = float (std::sin (2 * spectrum::kPi * 1000.0 * t1) + std::sin (2 * spectrum::kPi * 30000.0 * t1));
            y[n] = st.down (e, o, c);
        }
        const size_t nfft = 65536;
        const auto sp = spec (y, 0, nfft, nfft);
        const double binHz = 48000.0 / double (nfft);
        const double p1k = spectrum::peakPowerNear (sp, binHz, 1000.0, 5.0), p18k = spectrum::peakPowerNear (sp, binHz, 18000.0, 5.0);
        check (spectrum::db (p18k, p1k) <= -68.0, fmt ("[dec] 30 kHz -> 18 kHz alias after 2x decimation: %.1f dB re 1 kHz (need <= -68)", spectrum::db (p18k, p1k)));
    }

    // ── H6: exact-cycle aliasing at fs = 440·65536/k ──
    struct H6Row { double nonHarmMax, nonHarmFund; int highestHarm; };

    H6Row h6Render (int terrain, int orbit, int note, int quality, int k, double fs)
    {
        Instance in; in.cleanPatch(); in.setTerrainOrbit (0, terrain, orbit);
        in.setChoice ("oscAQuality", quality); in.setReal ("oscATerTrack", 1.0f);
        in.p.harnessPreFilterTap.store (true);
        in.prepare (fs, 512);
        const size_t N = 65536;
        RenderSpec s; s.seconds = 0.35 + double (N) / fs + 0.01; s.note = note; s.velocity = 1.0f;
        auto r = render (in, s);
        const size_t from = (size_t) (0.35 * fs);
        std::vector<double> y (r.L.begin() + (long) from, r.L.begin() + (long) (from + N));
        const auto a = spectrum::analyseExactCycle (y.data(), N, k, (size_t) (22000.0 / (fs / double (N))));
        return { a.nonHarmOverMax(), a.nonHarmOverFund(), a.maxHarmIdxAboveMinus100dB };
    }

    void gateH6()
    {
        std::printf ("\n== H6 aliasing (66 x {A2, A4, A6} exact-cycle at fs = 440*65536/600, 2x: nonharm/max <= -60 dB; 4x and 1x reported; rate rows within 3 dB) ==\n");
        const double fs = 440.0 * 65536.0 / 600.0;
        const int notes[3] = { 45, 69, 93 }; const int cycles[3] = { 150, 600, 2400 }; const char* names[3] = { "A2", "A4", "A6" };
        int fails2 = 0; double worst2 = -1e9, worst4 = -1e9, worst1 = -1e9; std::string worst2At, worst4At;
        for (int t = 0; t < kNumAnalyticTerrains; ++t)
            for (int o = 0; o < kNumOrbitKinds; ++o)
            {
                std::string line = fmt ("  [H6] %-13s x %-13s", kTerrainNames[t], kOrbitNames[o]);
                for (int n = 0; n < 3; ++n)
                {
                    const auto r2 = h6Render (t, o, notes[n], 1, cycles[n], fs);
                    const auto r4 = h6Render (t, o, notes[n], 2, cycles[n], fs);
                    const auto r1 = h6Render (t, o, notes[n], 0, cycles[n], fs);
                    line += fmt ("  %s 2x %6.1f (fund %6.1f, h<=%d) 4x %6.1f 1x %6.1f", names[n], r2.nonHarmMax, r2.nonHarmFund, r2.highestHarm, r4.nonHarmMax, r1.nonHarmMax);
                    if (r2.nonHarmMax > -60.0) ++fails2;
                    if (r2.nonHarmMax > worst2) { worst2 = r2.nonHarmMax; worst2At = fmt ("%s x %s %s", kTerrainNames[t], kOrbitNames[o], names[n]); }
                    if (r4.nonHarmMax > worst4) { worst4 = r4.nonHarmMax; worst4At = fmt ("%s x %s %s", kTerrainNames[t], kOrbitNames[o], names[n]); }
                    worst1 = std::max (worst1, r1.nonHarmMax);
                }
                std::printf ("%s\n", line.c_str());
            }
        check (fails2 == 0, fmt ("[H6] 2x: nonharm/max <= -60 dB on %d / 198 rows; worst %.1f dB at %s", 198 - fails2, worst2, worst2At.c_str()));
        std::printf ("  [H6] 4x reported (not gated in Round A): worst %.1f dB at %s; 1x (analytic) worst %.1f dB\n", worst4, worst4At.c_str(), worst1);
        // rate rows: Sine Product + Ellipse and Ridged Cosines + Epitrochoid 7 at 44.1 k / 96 k analogues
        struct Rate { const char* name; double fs; int k[3]; } rates[] = { { "48k", 440.0 * 65536 / 600, { 150, 600, 2400 } }, { "44.1k", 440.0 * 65536 / 652, { 163, 652, 2608 } }, { "96k", 440.0 * 65536 / 300, { 75, 300, 1200 } } };
        bool rateOk = true;
        for (auto pr : { std::make_pair (0, 0), std::make_pair (3, 5) })
        {
            double ref[3] = {};
            for (auto& rt : rates)
            {
                std::string line = fmt ("  [H6 rate] %-13s x %-13s %-5s 2x:", kTerrainNames[pr.first], kOrbitNames[pr.second], rt.name);
                for (int n = 0; n < 3; ++n)
                {
                    const auto r = h6Render (pr.first, pr.second, notes[n], 1, rt.k[n], rt.fs);
                    line += fmt (" %s %6.1f", names[n], r.nonHarmMax);
                    if (&rt == &rates[0]) ref[n] = r.nonHarmMax;
                    else if (std::abs (r.nonHarmMax - ref[n]) > 3.0 && r.nonHarmMax > -60.0) rateOk = false;
                }
                std::printf ("%s\n", line.c_str());
            }
        }
        check (rateOk, "[H6] 44.1 k / 96 k rows within 3 dB of the 48 k row (or below -60 dB)");
    }

    // ── H7: CPU delta ──
    double h7Wall (int quality, int unison, bool kernelBypass)
    {
        double best = 1.0e9;
        for (int run = 0; run < 3; ++run)
        {
            Instance in;
            in.setChoice ("oscAQuality", quality); in.setChoice ("oscBQuality", quality);
            in.setReal ("oscAUnison", (float) unison); in.setReal ("oscBUnison", (float) unison);
            in.p.harnessTerrainKernelBypass.store (kernelBypass);
            in.prepare (48000.0, 512);
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer midi; midi.ensureSize (256);
            for (int v = 0; v < 16; ++v) midi.addEvent (juce::MidiMessage::noteOn (1, 40 + v * 2, 0.8f), 0);
            buf.clear(); in.p.processBlock (buf, midi);   // note-ons + warm-up
            midi.clear();
            const int blocks = 10 * 48000 / 512;
            const auto t0 = std::chrono::steady_clock::now();
            for (int b = 0; b < blocks; ++b) { buf.clear(); in.p.processBlock (buf, midi); }
            best = std::min (best, secondsSince (t0));
        }
        return best / (10.0 * 48000.0 / 512.0 * 512.0 / 48000.0) * 100.0;   // % of one core over 10 s of audio
    }

    void gateH7()
    {
        std::printf ("\n== H7 CPU (16 voices x 2 osc, unison 1, 2x, default patch, 48 kHz, block 512, 10 s, best of 3; oscillator delta <= 12 %%) ==\n");
        std::printf ("  machine: Apple M4 Max (Release build)\n");
        const double total = h7Wall (1, 1, false), base = h7Wall (1, 1, true);
        std::printf ("  [H7] total = %.2f %%  baseline (kernel bypass) = %.2f %%  delta = %.2f %%\n", total, base, total - base);
        check (total - base <= 12.0, fmt ("[H7] oscillator delta = %.2f %% (need <= 12; total %.2f, baseline %.2f)", total - base, total, base));
        const double t4 = h7Wall (1, 4, false), b4 = h7Wall (1, 4, true);
        std::printf ("  [H7 rows] unison 4, 2x: total %.2f %% baseline %.2f %% delta %.2f %%\n", t4, b4, t4 - b4);
        const double tq4 = h7Wall (2, 1, false), bq4 = h7Wall (2, 1, true);
        std::printf ("  [H7 rows] unison 1, 4x: total %.2f %% baseline %.2f %% delta %.2f %%\n", tq4, bq4, tq4 - bq4);
        const double tq1 = h7Wall (0, 1, false), bq1 = h7Wall (0, 1, true);
        std::printf ("  [H7 rows] unison 1, Bandlimited (= 1x analytic in Round A): total %.2f %% baseline %.2f %% delta %.2f %%\n", tq1, bq1, tq1 - bq1);
    }

    // ── H8 across Quality ──
    void gateH8Quality()
    {
        std::printf ("\n== H8 across Quality (Bandlimited -> 2x -> 4x every 50 ms + terrain / orbit changes under 16 held notes, 5 s) ==\n");
        Instance in;
        in.prepare (48000.0, 480);
        RenderSpec s; s.seconds = 5.0; s.note = 40; s.armAllocations = true;
        for (int v = 1; v < 16; ++v) s.extra.push_back ({ 0, juce::MidiMessage::noteOn (1, 40 + v * 2, 0.8f) });
        int step = 0;
        s.onBlock = [&] (int start) {
            if (start % 2400 == 0)
            {
                in.setChoice ("oscAQuality", step % 3); in.setChoice ("oscBQuality", (step + 1) % 3);
                in.setChoice ("oscATerrain", step % kNumAnalyticTerrains); in.setChoice ("oscAOrbit", (step * 7) % kNumOrbitKinds);
                ++step;
            }
        };
        auto r = render (in, s);
        check (r.allocations == 0, fmt ("[H8] %lld allocations across %d Quality / terrain / orbit changes under 16 held notes%s (need 0)", r.allocations, step, rtcheck::foreignNote().c_str()));
        check (allFinite (r.L) && allFinite (r.R), "[H8] output finite through every Quality switch");
    }

    // ── crossfade click + latency ──
    void gateCrossfade()
    {
        std::printf ("\n== Quality switch (2x -> 4x at t = 1 s mid-note: no excess step outside the 64-sample crossfade) ==\n");
        Instance in; h5Patch (in, false); in.setChoice ("oscAQuality", 1);
        in.p.harnessPreFilterTap.store (true);
        in.prepare (48000.0, 480);
        RenderSpec s; s.seconds = 2.0; s.note = 60; s.velocity = 1.0f;
        s.onBlock = [&] (int start) { if (start == 48000) in.setChoice ("oscAQuality", 2); };
        auto r = render (in, s);
        auto segMax = [&] (int from, int to) { double m = 0; for (int i = std::max (from, 1); i < to; ++i) m = std::max (m, std::abs (r.L[(size_t) i] - r.L[(size_t) i - 1])); return m; };
        const double inside = segMax (48000, 48064), after = segMax (48064, 48000 + 4800), plateau = std::max (segMax (43200, 48000), segMax (52800, 57600));
        std::printf ("  [xfade] max step inside the 64-sample window %.4f, outside (next 100 ms) %.4f, plateau %.4f\n", inside, after, plateau);
        check (after <= 1.5 * plateau, fmt ("[xfade] max step outside the window %.4f <= 1.5 x plateau %.4f", after, plateau));
        check (inside <= 1.5 * plateau, fmt ("[xfade] max step inside the window %.4f <= 1.5 x plateau %.4f (equal-gain fade of the same theta)", inside, plateau));
    }

    void gateLatency()
    {
        std::printf ("\n== latency (getLatencySamples() after prepareToPlay = distortion latency + 1 for every Quality pair x bypass) ==\n");
        int bypassed[9], on[9]; int i = 0;
        for (int qa = 0; qa < 3; ++qa) for (int qb = 0; qb < 3; ++qb)
        {
            Instance a; a.setChoice ("oscAQuality", qa); a.setChoice ("oscBQuality", qb); a.setNorm ("distBypass", 1.0f); a.prepare (48000.0, 512); bypassed[i] = a.p.getLatencySamples();
            Instance b; b.setChoice ("oscAQuality", qa); b.setChoice ("oscBQuality", qb); b.setNorm ("distBypass", 0.0f); b.prepare (48000.0, 512); on[i] = b.p.getLatencySamples();
            ++i;
        }
        bool ok = true; for (int j = 0; j < 9; ++j) if (bypassed[j] != 1 || on[j] != on[0] || on[j] < 2) ok = false;
        std::printf ("  latency: distortion bypassed %d, distortion on %d (all 9 Quality pairs)\n", bypassed[0], on[0]);
        check (ok, fmt ("[latency] bypassed = 1 and on = %d (= distortion latency %d + 1) for all 9 Quality pairs", on[0], on[0] - 1));
        std::printf ("  note: the timerCallback follow-up uses the same expression (inspected; no message loop in Round A)\n");
    }

    // ── export grid ──
    void gateExport()
    {
        std::printf ("\n== export (6 x 11 x {C2, C4, C6}, 2 s, defaults, 2x, 24-bit WAV -> %s; golden/round-a-grid.sha256) ==\n", opt.exportsDir.c_str());
        juce::File dir (juce::String (opt.exportsDir));
        dir.createDirectory();
        juce::String sha;
        int written = 0;
        for (int t = 0; t < kNumAnalyticTerrains; ++t)
            for (int o = 0; o < kNumOrbitKinds; ++o)
                for (int n = 0; n < 3; ++n)
                {
                    Instance in; in.setReal ("oscAPhase", 0.25f); in.p.setHarnessPhaseSeed (opt.seed);
                    in.setTerrainOrbit (0, t, o);
                    RenderSpec s; s.seconds = 2.0; s.note = kCNotes[n]; s.velocity = 0.8f; s.noteOffAt = 1.5;
                    auto r = render (in, s);
                    const juce::String name = juce::String (kTerrainNames[t]) + "-" + kOrbitNames[o] + "-" + kNoteNames[n] + ".wav";
                    juce::File f = dir.getChildFile (name);
                    f.deleteFile();
                    juce::WavAudioFormat wav;
                    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), r.fs, 2, 24, {}, 0));
                    if (w == nullptr) { std::printf ("!! cannot write %s\n", name.toRawUTF8()); continue; }
                    juce::AudioBuffer<float> buf (2, (int) r.L.size());
                    for (size_t i = 0; i < r.L.size(); ++i) { buf.setSample (0, (int) i, (float) r.L[i]); buf.setSample (1, (int) i, (float) r.R[i]); }
                    w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
                    w.reset();
                    juce::MemoryBlock bytes; f.loadFileAsData (bytes);
                    sha << juce::SHA256 (bytes.getData(), bytes.getSize()).toHexString() << "  " << name << "\n";
                    ++written;
                }
        const juce::File golden = juce::File (juce::String (opt.fixtures)).getParentDirectory().getChildFile ("golden/round-a-grid.sha256");
        golden.getParentDirectory().createDirectory();
        golden.replaceWithText (sha);
        check (written == 198, fmt ("[export] %d / 198 WAVs written; checksums -> %s", written, golden.getFullPathName().toRawUTF8()));
    }

    //==========================================================================
    // ── CLI ──────────────────────────────────────────────────────────────────

    void usage()
    {
        std::printf ("O-Strata-render-test --gate <H1..H9|tuning|smoke|centroids|saturation|decimator|crossfade|latency|export|all> [--gate ...]\n"
                     "  [--note N] [--velocity V] [--seconds S] [--terrain I] [--orbit I] [--quality I]\n"
                     "  [--set id=norm]... [--fs F] [--block B] [--seed S] [--fixtures DIR] [--export NAME]\n"
                     "  [--print-only] [--with-disk]\n");
    }

    bool parse (int argc, char** argv)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string a = argv[i];
            auto next = [&] () -> std::string { if (i + 1 >= argc) { usage(); std::exit (2); } return argv[++i]; };
            if (a == "--gate") opt.gates.push_back (next());
            else if (a == "--all") opt.gates.push_back ("all");
            else if (a == "--smoke") opt.gates.push_back ("smoke");
            else if (a == "--note") opt.note = std::stoi (next());
            else if (a == "--velocity") opt.velocity = std::stof (next());
            else if (a == "--seconds") opt.seconds = std::stod (next());
            else if (a == "--terrain") opt.terrain = std::stoi (next());
            else if (a == "--orbit") opt.orbit = std::stoi (next());
            else if (a == "--quality") opt.quality = std::stoi (next());
            else if (a == "--set") { const auto kv = next(); const auto eq = kv.find ('='); if (eq == std::string::npos) return false; opt.sets.push_back ({ kv.substr (0, eq), std::stof (kv.substr (eq + 1)) }); }
            else if (a == "--fs") opt.fs = std::stod (next());
            else if (a == "--block") opt.block = std::stoi (next());
            else if (a == "--seed") opt.seed = (uint32_t) std::stoul (next());
            else if (a == "--fixtures") opt.fixtures = next();
            else if (a == "--export") opt.exportName = next();
            else if (a == "--print-only") opt.printOnly = true;
            else if (a == "--with-disk") opt.withDisk = true;
            else { usage(); return false; }
        }
        return true;
    }

    bool wants (const char* g)
    {
        for (const auto& s : opt.gates) if (s == g || s == "all") return true;
        return false;
    }
    bool wantsExact (const char* g)   // not part of --all (writes files)
    {
        for (const auto& s : opt.gates) if (s == g) return true;
        return false;
    }
}

//==============================================================================
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;   // juce_events; no message loop is ever pumped
    if (! parse (argc, argv)) return 2;
    if (opt.gates.empty()) { usage(); return 2; }

    const auto t0 = std::chrono::steady_clock::now();
    std::printf ("O-Strata-render-test  fixtures=%s  fs=%.2f block=%d seed=%u\n", opt.fixtures.c_str(), opt.fs, opt.block, opt.seed);

    // Mod-matrix index literals asserted against the live lists (pattern_test_fixture_mirrors_drift_silently)
    {
        const auto src = getModSourceNames(), dst = getModDestNames();
        check (src[kSrcLFO1] == "LFO1" && src[kSrcVelocity] == "Velocity" && src[kSrcModWheel] == "ModWheel",
               "[index] source indices 1 / 7 / 9 = LFO1 / Velocity / ModWheel");
        check (dst[kDstLFO1Rate] == "LFO1 Rate" && dst[kDstPitch] == "Pitch" && dst[kDstOscAOrbCX] == "OscA Orbit CX" && dst[kDstOscATerFreq] == "OscA Terrain Freq",
               "[index] destination indices 10 / 23 / 28 / 31 = LFO1 Rate / Pitch / OscA Orbit CX / OscA Terrain Freq");
    }

    if (opt.printOnly)
    {
        Instance in; in.cleanPatch(); in.applyCliOverrides();
        in.prepare (opt.fs, opt.block);
        std::printf ("latency: %d samples (halfband L2 = %.4f, L4 = %.4f base samples)\n", in.p.getLatencySamples(), HalfbandCoeffs::get().latency2, HalfbandCoeffs::get().latency4);
        RenderSpec s; s.seconds = opt.seconds; s.note = opt.note; s.velocity = opt.velocity;
        auto r = render (in, s);
        std::printf ("render: %d samples, rms %.4f, max %.4f, finite %d\n", (int) r.L.size(), rms (r.L, 0), maxAbs (r.L), (int) allFinite (r.L));
        return 0;
    }

    if (wants ("smoke"))     gateSmoke();
    if (wants ("tuning"))    gateTuning();
    if (wants ("H1"))        gateH1();
    if (wants ("H2"))        gateH2();
    if (wants ("centroids")) gateCentroids();
    if (wants ("H3"))        gateH3();
    if (wants ("H4"))        gateH4();
    if (wants ("saturation")) gateSaturation();
    if (wants ("H5"))        gateH5();
    if (wants ("H8"))        gateH8();
    if (wants ("H9"))        gateH9();
    if (wants ("decimator")) gateDecimator();
    if (wants ("H6"))        gateH6();
    if (wants ("H8"))        gateH8Quality();
    if (wants ("crossfade")) gateCrossfade();
    if (wants ("latency"))   gateLatency();
    if (wants ("H7"))        gateH7();
    if (wantsExact ("export")) gateExport();

    std::printf ("\n%s — %d check(s), %d failure(s), %.1f s\n", failures == 0 ? "ALL GATES PASSED" : "GATES FAILED", checksRun, failures, secondsSince (t0));
    return failures;
}
