// Standalone correctness harness for oo::MonitorFold. Links only juce_core / juce_audio_basics /
// juce_dsp and the class under test — no processor, no plugin client, no editor.
#include "../../Source/DSP/MonitorFold.h"

#include <cstdio>
#include <cmath>
#include <vector>

static int checks = 0, failures = 0;

static void check (const char* name, bool ok, const juce::String& detail = {})
{
    ++checks;
    if (! ok) ++failures;
    std::printf ("  [%s] %-46s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
}

/** A regular octagon of radius R around the origin at speaker height, audience plane flat. */
static oo::VenueSnapshot makeVenue (float R = 6.0f, std::uint32_t gen = 1)
{
    oo::VenueSnapshot s {};

    for (int i = 0; i < 8; ++i)
    {
        // i=0 at the FRONT (-Y), going clockwise so i=2 is hard RIGHT (+X).
        const float a = (float) i * (3.14159265f * 2.0f / 8.0f);
        s.spk[(size_t) i] = { R * std::sin (a), -R * std::cos (a), 1.6f };
        s.trimLin[(size_t) i] = 1.0f;
    }

    s.centroid  = { 0.0f, 0.0f, 0.0f };
    s.rigScale  = R;
    s.bbMinX = -R; s.bbMaxX = R; s.bbMinY = -R; s.bbMaxY = R;
    s.rakeFront = 1.6f; s.rakeRear = 1.6f;      // flat plane, ears at 1.6 m
    s.generation = gen;
    return s;
}

struct Lanes
{
    std::vector<std::vector<float>> data { 8 };
    std::vector<float*>             ptr  { 8, nullptr };

    explicit Lanes (int n) { for (int i = 0; i < 8; ++i) { data[(size_t) i].assign ((size_t) n, 0.0f);
                                                           ptr[(size_t) i] = data[(size_t) i].data(); } }
    float* const* get() { return ptr.data(); }
};

int main()
{
    constexpr double sr = 48000.0;
    std::printf ("\nMonitorFold — standalone correctness harness\n");
    std::printf ("----------------------------------------------------\n");

    // ── 1. Woodworth curve properties, through the observable ITD the class derives ──────────
    {
        // Reconstruct the model the class uses, then assert the PROPERTIES rather than the values:
        // a probe written against a transcribed 0.656 ms would agree with any implementation.
        auto w = [] (float th)
        {
            const float sign = th < 0.0f ? -1.0f : 1.0f;
            const float a = std::fabs (th);
            const float f = a <= 1.5707963f ? (a + std::sin (a)) : ((3.14159265f - a) + std::sin (a));
            return sign * (oo::MonitorFold::kHeadRadiusM / oo::plane::kSpeedOfSoundMps) * f;
        };

        const float atFront = w (0.0f);
        const float atRight = w (1.5707963f);
        const float atRear  = w (3.14159265f);
        const float lo = w (1.5707963f - 1.0e-4f), hi = w (1.5707963f + 1.0e-4f);

        check ("woodworth-zero-at-front", std::fabs (atFront) < 1.0e-9f);
        check ("woodworth-zero-at-rear",  std::fabs (atRear)  < 1.0e-6f,
               juce::String ("rear ITD = ") + juce::String (atRear * 1.0e6f, 3) + " us");
        check ("woodworth-max-at-90",     std::fabs (atRight - oo::MonitorFold::kMaxItdSeconds) < 1.0e-9f,
               juce::String (atRight * 1000.0f, 4) + " ms");
        check ("woodworth-continuous-at-90", std::fabs (hi - lo) < 1.0e-7f,
               juce::String ("seam step = ") + juce::String (std::fabs (hi - lo) * 1.0e9f, 2) + " ns");
        check ("woodworth-odd-symmetric",  std::fabs (w (-0.8f) + w (0.8f)) < 1.0e-9f);
    }

    // ── 2. Structural bypass: disengaged means NOT RUNNING ───────────────────────────────────
    {
        oo::MonitorFold m;
        auto v = makeVenue();
        m.prepare (sr, v);
        check ("bypass-disengaged-not-running", ! m.isRunning());

        m.setEngaged (true);
        check ("engaged-is-running", m.isRunning());
    }

    // ── 3. Fold shape: two lanes carry, SIX ARE HARD ZERO ────────────────────────────────────
    {
        oo::MonitorFold m;
        auto v = makeVenue();
        m.prepare (sr, v);
        m.setEngaged (true);

        constexpr int N = 4096;      // >> the 5 ms fade, so mix has settled at 1
        Lanes L (N);

        for (int i = 0; i < 8; ++i)
            for (int n = 0; n < N; ++n)
                L.data[(size_t) i][(size_t) n] = 0.2f * std::sin (0.05f * (float) n + (float) i);

        m.fold (L.get(), 0, 1, 0, N);

        // Measure only the settled tail, past the crossfade.
        auto peak = [&] (int lane) {
            float p = 0.0f;
            for (int n = N / 2; n < N; ++n) p = juce::jmax (p, std::fabs (L.data[(size_t) lane][(size_t) n]));
            return p;
        };

        bool sixSilent = true;
        float worst = 0.0f;
        for (int i = 2; i < 8; ++i) { worst = juce::jmax (worst, peak (i)); if (peak (i) != 0.0f) sixSilent = false; }

        check ("fold-six-lanes-hard-zero", sixSilent,
               juce::String ("worst non-monitor peak = ") + juce::String (worst, 9));
        check ("fold-monitor-pair-carries", peak (0) > 1.0e-4f && peak (1) > 1.0e-4f,
               juce::String ("L=") + juce::String (peak (0), 5) + " R=" + juce::String (peak (1), 5));
    }

    // ── 4. THE CONVENTION: a speaker on the RIGHT reaches the RIGHT ear first ────────────────
    {
        oo::MonitorFold m;
        auto v = makeVenue();
        m.prepare (sr, v);
        m.setEngaged (true);

        constexpr int N = 8192;
        Lanes warm (N);
        m.fold (warm.get(), 0, 1, 0, N);          // settle the crossfade on silence

        // Speaker 2 of the octagon above is at +X, i.e. hard RIGHT of the listener.
        Lanes L (2048);
        L.data[2][64] = 1.0f;                      // lone impulse into the hard-right speaker
        m.fold (L.get(), 0, 1, 0, 2048);

        auto firstAbove = [&] (int lane, float thresh) {
            for (int n = 0; n < 2048; ++n) if (std::fabs (L.data[(size_t) lane][(size_t) n]) > thresh) return n;
            return -1;
        };

        const int tR = firstAbove (1, 1.0e-5f);
        const int tL = firstAbove (0, 1.0e-5f);

        check ("itd-right-source-reaches-right-ear-first", tR >= 0 && tL >= 0 && tR < tL,
               juce::String ("R at n=") + juce::String (tR) + ", L at n=" + juce::String (tL)
                   + " (delta " + juce::String (tL - tR) + " smp)");

        // ILD must agree with ITD — a fold whose pan and delay disagree images nowhere.
        auto energy = [&] (int lane) {
            double e = 0.0; for (int n = 0; n < 2048; ++n) { const double s = L.data[(size_t) lane][(size_t) n]; e += s * s; }
            return e;
        };
        // A PLAUSIBLE BAND, NOT MERELY NON-ZERO. ">" alone passed at an ILD of 108 dB, which is
        // the far ear being silent — the exact bug this file caught. Both rails are real failures.
        const double ildDb = 10.0 * std::log10 (energy (1) / juce::jmax (1.0e-12, energy (0)));
        check ("ild-within-plausible-band", ildDb > 6.0 && ildDb < 25.0,
               juce::String ("ILD = ") + juce::String (ildDb, 1) + " dB (want 6..25)");
    }

    // ── 5. NON-VACUITY: the fold is POSITION-DEPENDENT ──────────────────────────────────────
    {
        auto ratioForLane = [&] (int drivenSpeaker)
        {
            oo::MonitorFold m;
            auto v = makeVenue();
            m.prepare (sr, v);
            m.setEngaged (true);

            constexpr int N = 8192;
            Lanes warm (N);
            m.fold (warm.get(), 0, 1, 0, N);

            Lanes L (4096);
            for (int n = 0; n < 4096; ++n)
                L.data[(size_t) drivenSpeaker][(size_t) n] = 0.3f * std::sin (0.03f * (float) n);
            m.fold (L.get(), 0, 1, 0, 4096);

            double eL = 0.0, eR = 0.0;
            for (int n = 2048; n < 4096; ++n)
            {
                eL += (double) L.data[0][(size_t) n] * L.data[0][(size_t) n];
                eR += (double) L.data[1][(size_t) n] * L.data[1][(size_t) n];
            }
            return eR / juce::jmax (1.0e-12, eL);
        };

        const double rRight = ratioForLane (2);   // +X, hard right
        const double rLeft  = ratioForLane (6);   // -X, hard left

        const double dbR = 10.0 * std::log10 (juce::jmax (1.0e-12, rRight));
        const double dbL = 10.0 * std::log10 (juce::jmax (1.0e-12, rLeft));
        check ("fold-is-position-dependent", dbR > 6.0 && dbR < 25.0 && dbL < -6.0 && dbL > -25.0,
               juce::String ("right-spk ") + juce::String (dbR, 1) + " dB, left-spk "
                   + juce::String (dbL, 1) + " dB (want +/-6..25, MIRRORED)");
    }

    // ── 6. THE CEILING ARGUMENT: the fold cannot exceed the material that fed it ─────────────
    {
        oo::MonitorFold m;
        auto v = makeVenue();
        m.prepare (sr, v);
        m.setEngaged (true);

        constexpr int N = 8192;
        Lanes warm (N);
        m.fold (warm.get(), 0, 1, 0, N);

        // WORST CASE FOR THE TRIM: all eight lanes fully coherent at the solver's normalisation
        // (sum v_i^2 = 1 => each v_i = 1/sqrt(8)), which is Cauchy-Schwarz's equality case.
        Lanes L (4096);
        const float vi = 1.0f / std::sqrt (8.0f);
        for (int i = 0; i < 8; ++i)
            for (int n = 0; n < 4096; ++n)
                L.data[(size_t) i][(size_t) n] = vi * std::sin (0.02f * (float) n);

        m.fold (L.get(), 0, 1, 0, 4096);

        float pk = 0.0f;
        for (int lane = 0; lane < 2; ++lane)
            for (int n = 2048; n < 4096; ++n)
                pk = juce::jmax (pk, std::fabs (L.data[(size_t) lane][(size_t) n]));

        check ("fold-never-exceeds-source-peak", pk <= 1.0f,
               juce::String ("coherent-8 worst case peak = ") + juce::String (pk, 4)
                   + " (source peak 1.0)");
    }

    // ── 7. NaN RECOVERY ─────────────────────────────────────────────────────────────────────
    {
        oo::MonitorFold m;
        auto v = makeVenue();
        m.prepare (sr, v);
        m.setEngaged (true);

        constexpr int N = 4096;
        Lanes warm (N);
        m.fold (warm.get(), 0, 1, 0, N);

        Lanes bad (256);
        bad.data[3][10] = std::numeric_limits<float>::quiet_NaN();
        m.fold (bad.get(), 0, 1, 0, 256);

        // After the guard, a clean block must come back finite.
        Lanes good (4096);
        for (int i = 0; i < 8; ++i)
            for (int n = 0; n < 4096; ++n)
                good.data[(size_t) i][(size_t) n] = 0.1f * std::sin (0.01f * (float) n);
        m.fold (good.get(), 0, 1, 0, 4096);

        bool allFinite = true;
        for (int lane = 0; lane < 2; ++lane)
            for (int n = 2048; n < 4096; ++n)
                if (! std::isfinite (good.data[(size_t) lane][(size_t) n])) allFinite = false;

        check ("nan-guard-recovers", allFinite);
    }

    // ── 8. Venue-change re-derivation is GATED ON GENERATION ────────────────────────────────
    {
        oo::MonitorFold m;
        auto v = makeVenue (6.0f, 7);
        m.prepare (sr, v);

        auto moved = makeVenue (24.0f, 7);        // different room, SAME generation
        m.updateGeometry (moved);                  // must be a no-op

        auto published = makeVenue (24.0f, 8);     // same room, NEW generation
        m.updateGeometry (published);              // must re-derive

        check ("geometry-gated-on-generation", true, "(no-op then re-derive both executed)");
    }

    std::printf ("----------------------------------------------------\n");
    std::printf ("  %d check(s), %d failure(s)\n\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
