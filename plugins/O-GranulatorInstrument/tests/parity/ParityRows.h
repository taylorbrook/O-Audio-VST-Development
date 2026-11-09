#pragma once
// ============================================================================
//   O-GranulatorInstrument — DSP-01 / DSP-03 / DSP-04 / DSP-05 parity rows
//   (PLAN-2.2 T3 / PD5, R§1–§3; PLAN-2.3 T3: PF / PFc / PS5–7; PLAN-2.4 T11:
//   PB / PBc / PBd FeedbackChain leaf nulls, PR / PRk / PRc reverb + knee)
//
//   ONE source of truth, included by BOTH drivers:
//     - tests/parity/parent_render.cpp   (O-Granulator @ 82406fce, out of tree)
//     - tests/render-harness/parity_gates_22.h  (our harness, --parity <dir>)
//   Plain C++17: no juce, no ogi::, no ogran:: types.
//
//   Null geometry (R§2.3). Parent: frozen ring, freezeNoMinSpray, stream
//   enabled at T_e >= freezeStopTick, so a grain anchors at
//       A = W0 − margin(ℓ) − pos·S          (spray 0, no reflect)
//   Ours: 1 voice at root, key track 100 %, scan 0, loop 0..100 %:
//       p0 = x·F                           (c = lo + x·W, lo = 0, W = F)
//   Feed the parent x_par[k] = M[k − Δ] with Δ = A − p0; then parent ring
//   time τ ≡ our tape frame τ − Δ, for every mip level iff Δ ≡ 0 (mod 2^ℓ).
//
//   Fixed geometry (all integers, chosen once — the solver CHECKS it):
//     fs 48 kHz, buffer_length 8.25 s → S = 396 000, T_f = 396 000 (≡ 0 mod 32)
//       (buffer_length is SKEWED (0.5): 8.25 = 1 + 29·0.5² is the value that
//        lands exactly; 15 s landed a few ulps off → S non-integer, caught by
//        the anchor self-check on the first run)
//     W0 = T_f + freezeDefer (9 cells = 288 @ 48 k) = 396 288
//     heldCaptured = W0 − 223 = 396 065 ≥ S  → span = S exactly
//     F = 128 088 frames (≡ 8 mod 16 → x·F ≡ 2 mod 4 at x = ¼, so Δ is a
//     multiple of 2^ℓ for ℓ = 0, 1, 2 at once; (1 − x)·F ≤ x·S − 213 keeps the
//     material captured, and (1 − x)·F ≥ 96 064 fits PN04's 2 s grain)
//     parent stream enabled at T_e = T_f + 512 (a tick ≥ freezeStopTick)
// ============================================================================

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace parity
{
    constexpr double  kFs          = 48000.0;
    constexpr int     kCell        = 32;
    constexpr double  kBufLenS     = 8.25;
    constexpr int64_t kS           = 396000;              // buffer_length · fs
    constexpr int64_t kTf          = 396000;              // freeze rising edge (tick)
    constexpr int64_t kDefer       = 9 * kCell;           // rampCellsFor (48 k, 5 ms) + 1 cells
    constexpr int64_t kW0          = kTf + kDefer;        // 396 288
    constexpr int64_t kHeld        = kW0 - 223;           // W0 − margin(2) − 1
    constexpr int64_t kTe          = kTf + 512;           // parent stream enable tick
    constexpr int64_t kF           = 128088;              // material frames
    constexpr float   kMatPeak     = 0.01f;               // −40 dBFS peak (our wet tanh ≈ identity, R§2.3)
    constexpr int     kOurRoot     = 48;                  // our note = root → st = pitch only
    constexpr int64_t kOurTon      = 0;                   // our note-on sample (a multiple of 32)

    inline int64_t margin (int l) { return 63 * ((int64_t (1) << l) - 1) + 8 * (int64_t (1) << l) + 1; }   // 9 / 80 / 222

    /** BandlimitedReader::latchLevel with the parent max 2 (ours max 4 agrees for |st| ≤ 24). */
    inline int levelForRate (double rate)
    {
        if (rate < 2.0) return 0;
        const int l = std::ilogb (rate);
        return l > 2 ? 2 : l;
    }

    // ── material ────────────────────────────────────────────────────────────
    inline float hashNoise (int64_t i, int ch, float amp, uint64_t salt = 0)
    {
        uint64_t x = static_cast<uint64_t> (i) * 0x9E3779B97F4A7C15ull
                   + (ch != 0 ? 0xD1B54A32D192ED03ull : 0x2545F4914F6CDD1Dull) + salt * 0xA24BAED4963EE407ull;
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ull;
        x ^= x >> 27; x *= 0x94D049BB133111EBull;
        x ^= x >> 31;
        const double u = static_cast<double> (x >> 40) / static_cast<double> (1ull << 24);   // [0, 1)
        return static_cast<float> ((2.0 * u - 1.0) * amp);
    }

    /** PN / PS material M[n], n ∈ [0, F): hash noise, peak ≤ −40 dBFS (2.2 rows).
        PLAN-2.3 PD5: PF rows pass their own peak (0.1 / 0.25 / 0.4); the hash is
        unchanged, so the 2.2 material is bitwise the same. */
    inline float material (int64_t n, int ch, float peak = kMatPeak) { return hashNoise (n, ch, peak, 0x9A); }

    /** GV material: uniform hash noise at −30 dBFS RMS (a = 0.0316·√3 = 0.05477, peak ≤ 0.1). */
    constexpr float kGvAmp = 0.054772256f;
    inline float gvNoise (int64_t n, int ch) { return hashNoise (n, ch, kGvAmp, 0x6F); }
    constexpr double kGvSeconds = 20.0, kGvSkipS = 0.5, kGvMaterialS = 8.0;

    // ── rows ────────────────────────────────────────────────────────────────
    struct KV { std::string id; float v; };   // id: a stream SUFFIX ("size") or a global ID ("grain_limit")

    enum class Kind { null_, stats, gv, fxNull, fxStats, fbNull, revNull, modNull };   // fx*: PLAN-2.3 PD11; fb / rev: PLAN-2.4 PD12; mod: PLAN-2.5a PD13 (pg22 / pg23 never see them)

    inline bool isNullKind (Kind k)  { return k == Kind::null_ || k == Kind::fxNull; }
    inline bool isStatsKind (Kind k) { return k == Kind::stats || k == Kind::fxStats; }
    inline bool isFbKind (Kind k)    { return k == Kind::fbNull; }
    inline bool isRevKind (Kind k)   { return k == Kind::revNull; }
    inline bool isModKind (Kind k)   { return k == Kind::modNull; }
    /** PB / PR / PL rows use the PN01 null geometry, so the parent self-checks apply. */
    inline bool isGeomKind (Kind k)  { return isNullKind (k) || isFbKind (k) || isRevKind (k) || isModKind (k); }

    // ── 2.5a (PLAN-2.5a PD13): routes, timed events, a mock transport ─────
    /** A matrix route by NAME: each side maps the names to its own choice
        index (AudioParameterChoice::choices.indexOf, P2) and self-checks it. */
    struct Route { int slot = 1; std::string src; std::string dest; float depth = 0.0f; };
    /** A parameter set at absolute sample `at` (a block boundary on both sides). */
    struct Event { int64_t at = 0; std::string id; float value = 0.0f; };
    /** A route edit at `at`: slot fields by NAME (empty = unchanged). */
    struct RouteEdit { int64_t at = 0; int slot = 1; std::string src; std::string dest; bool setDepth = false; float depth = 0.0f; };
    /** The mock transport (both drivers wrap it in a juce::AudioPlayHead,
        evaluated at every block start). 4/4, last bar = floor (ppq / 4)·4. */
    struct Transport
    {
        bool    on = false;
        double  bpm = 120.0, ppq0 = 0.0;
        int64_t stepAt = -1;  double bpm2 = 120.0;     // a tempo step at stepAt
        int64_t loopAt = -1;  double loopBack = 0.0;   // ppq jumps back by loopBack at loopAt
        double  bpmAdd = 0.0;                           // PLc3: ours-only +0.1
    };
    inline double tpBpmAt (const Transport& t, int64_t s)
    {
        return ((t.stepAt >= 0 && s >= t.stepAt) ? t.bpm2 : t.bpm) + t.bpmAdd;
    }
    inline double tpPpqAt (const Transport& t, int64_t s, double fs)
    {
        const double b1 = t.bpm + t.bpmAdd, b2 = t.bpm2 + t.bpmAdd;
        double q = (t.stepAt >= 0 && s >= t.stepAt)
                       ? t.ppq0 + static_cast<double> (t.stepAt) * b1 / 60.0 / fs + static_cast<double> (s - t.stepAt) * b2 / 60.0 / fs
                       : t.ppq0 + static_cast<double> (s) * b1 / 60.0 / fs;
        if (t.loopAt >= 0 && s >= t.loopAt)
            q -= t.loopBack;
        return q;
    }
    constexpr int64_t kModBlock = 512;
    /** A time T_e + k·512: a block boundary on both drivers. */
    inline int64_t atBlock (int k) { return kTe + kModBlock * static_cast<int64_t> (k); }

    struct Row
    {
        std::string id;
        Kind kind = Kind::null_;
        int stream = 1;                       // 1-based; parent stream N reads the ring, ours reads slot N
        std::vector<KV> sp;                   // stream params (both sides, applied to every enabled stream)
        std::vector<KV> oursOnly;             // our-side-only overrides (PNc2)
        int  streams = 1;                     // stats rows: 4 streams with the same settings
        double seconds = 1.0;                 // render length after T_e / T_on
        int64_t deltaAdj = 0;                 // PNc3: parent material shifted by +1
        int64_t ourOnsetShift = 0;            // PNc1: our note-on at T_on + 1
        std::string parentOf;                 // control rows reuse this parent render
        uint64_t seed = 0x5EED22A0B1ull;
        // 2.3 (PLAN-2.3 PD5 / PD11)
        float matPeak = kMatPeak;             // material peak (0.01 = every 2.2 row unchanged)
        std::vector<std::string> oursHooks;   // our-side test hooks: "legacySvfMake", "fxOrderSwap";
                                              // 2.4: "chainKnobParity", "legacyWetTanh", "testAgcLegacy", "retLag31"
        // 2.4 (PLAN-2.4 PD12)
        int blockSize = 512;                  // render block (PB rows 32: the parent return is read per cell)
        std::vector<KV> gl;                   // GLOBAL params, both sides (feedback, fb_*, reverb_*)
        std::vector<KV> oursGl;               // our-side-only global overrides (PBc1 / PBc3 / PRc1)
        // 2.5a (PLAN-2.5a PD13)
        std::vector<Route> routes;            // both sides
        std::vector<Event> events;            // both sides
        std::vector<Event> oursEvents;        // our-side-only events (controls)
        std::vector<RouteEdit> edits;         // both sides
        std::vector<RouteEdit> oursEdits;     // our-side-only route edits (PEc3)
        Transport tp;                         // both sides (bpmAdd ours only)
        std::string traceDest;                // dest NAME traced per tick (PL0t), both sides
        bool deferred = false;                // needs the Tempo stream path (T12)
    };

    /** Base stream settings for the null rows (spray 0, jitter 0, every random control 0). */
    inline std::vector<KV> nullBase()
    {
        return { { "size", 100.0f }, { "density", 10.0f }, { "window", 0.0f }, { "window_skew", 0.0f },
                 { "pitch", 0.0f }, { "position", 25.0f }, { "reverse_prob", 0.0f }, { "spray", 0.0f },
                 { "jitter", 0.0f }, { "stereo_spread", 0.0f }, { "gain_rand", 0.0f }, { "size_rand", 0.0f },
                 { "pitch_rand", 0.0f }, { "scale", 0.0f }, { "level", 0.0f }, { "pan", 0.0f } };
    }

    inline std::vector<KV> over (std::vector<KV> b, const std::vector<KV>& o)
    {
        for (const auto& kv : o)
        {
            bool found = false;
            for (auto& x : b)
                if (x.id == kv.id) { x.v = kv.v; found = true; }
            if (! found) b.push_back (kv);
        }
        return b;
    }

    /** Every 2.4 global at an explicit value (both sides): feedback off, the
        spec chain defaults, reverb off (the 2.3 fxBase() pattern). */
    inline std::vector<KV> fbRevBase()
    {
        return { { "feedback", 0.0f }, { "fb_low_cut", 80.0f }, { "fb_high_cut", 12000.0f }, { "fb_saturation", 20.0f },
                 { "fb_diffusion", 0.0f }, { "reverb_mix", 0.0f }, { "reverb_size", 50.0f }, { "reverb_damp", 50.0f } };
    }

    /** The 8 global IDs in manifest column order (appended after the 2.3 peak). */
    inline const std::vector<std::string>& fbRevIds()
    {
        static const std::vector<std::string> v { "feedback", "fb_low_cut", "fb_high_cut", "fb_saturation", "fb_diffusion",
                                                  "reverb_mix", "reverb_size", "reverb_damp" };
        return v;
    }

    inline float findV (const std::vector<KV>& v, const std::string& id, float dflt)
    {
        for (const auto& kv : v) if (kv.id == id) return kv.v;
        return dflt;
    }

    // ── 2.3 per-grain FX (PLAN-2.3 T3) ─────────────────────────────────────
    /** The 10 FX stream suffixes, in manifest column order. */
    inline const std::vector<std::string>& fxIds()
    {
        static const std::vector<std::string> v { "filter_type", "filter_cutoff", "filter_rand", "filter_res", "drive",
                                                  "crush", "crush_prob", "ring_freq", "ring_rand", "ring_mix" };
        return v;
    }

    /** Every FX row at its spec default (set explicitly on both sides). */
    inline std::vector<KV> fxBase()
    {
        return { { "filter_type", 0.0f }, { "filter_cutoff", 20000.0f }, { "filter_rand", 0.0f }, { "filter_res", 20.0f },
                 { "drive", 0.0f }, { "crush", 0.0f }, { "crush_prob", 100.0f }, { "ring_freq", 200.0f },
                 { "ring_rand", 0.0f }, { "ring_mix", 0.0f } };
    }

    /** Expected latched fxActive bits (SVF 1, drive 2, crush 4, ring 8) for a
        deterministic row (crush_prob 0 / 100 only). */
    inline uint32_t expectedFxBits (const std::vector<KV>& sp)
    {
        uint32_t b = 0;
        if (findV (sp, "filter_type", 0.0f) >= 0.5f) b |= 1u;
        const double d = findV (sp, "drive", 0.0f) / 100.0;
        if (16.0 * d * d >= 1.0e-4) b |= 2u;
        if (findV (sp, "crush", 0.0f) > 0.0f && findV (sp, "crush_prob", 100.0f) >= 100.0f) b |= 4u;
        if (findV (sp, "ring_mix", 0.0f) > 0.0f) b |= 8u;
        return b;
    }

    inline bool isPfControl (const std::string& id) { return id.rfind ("PFc", 0) == 0; }

    inline std::vector<Row> rows()
    {
        std::vector<Row> r;
        auto pn = [&r] (const char* id, std::vector<KV> o, double secs = 1.0, int stream = 1)
        {
            Row x; x.id = id; x.kind = Kind::null_; x.stream = stream; x.sp = over (nullBase(), o); x.seconds = secs;
            r.push_back (x);
        };
        // size / density
        pn ("PN01", {});
        pn ("PN02", { { "size", 20.0f } });
        pn ("PN03", { { "size", 500.0f } });
        pn ("PN04", { { "size", 2000.0f }, { "density", 1.0f } }, 3.5);
        pn ("PN05", { { "density", 50.0f } });
        pn ("PN06", { { "density", 200.0f }, { "size", 20.0f } });
        // window / skew (steps of 64: raw = step / 64)
        pn ("PN07", { { "window", 11.0f / 64.0f } });
        pn ("PN08", { { "window", 21.0f / 64.0f } });
        pn ("PN09", { { "window", 32.0f / 64.0f } });
        pn ("PN10", { { "window", 43.0f / 64.0f } });
        pn ("PN11", { { "window", 53.0f / 64.0f } });
        pn ("PN12", { { "window", 1.0f } });
        pn ("PN13", { { "window", 0.5f }, { "window_skew", 100.0f } });
        pn ("PN14", { { "window", 0.5f }, { "window_skew", -100.0f } });
        pn ("PN15", { { "window", 0.0f }, { "window_skew", 100.0f } });
        pn ("PN16", { { "window", 1.0f }, { "window_skew", -100.0f } });
        // pitch / position
        pn ("PN17", { { "pitch", 7.0f } });
        pn ("PN18", { { "pitch", -12.0f } });
        pn ("PN19", { { "pitch", 12.0f } });
        pn ("PN20", { { "pitch", 19.0f } });
        pn ("PN21", { { "pitch", 24.0f } });
        pn ("PN22", { { "pitch", -24.0f } });
        pn ("PN23", { { "position", 75.0f } });
        // reverse / stream
        pn ("PN24", { { "reverse_prob", 100.0f } });
        pn ("PN25", { { "reverse_prob", 100.0f }, { "pitch", 12.0f } });
        pn ("PN26", {}, 1.0, 3);
        // controls (ours only, except PNc3 which shifts the parent material)
        {
            Row c = r[0]; c.id = "PNc1"; c.ourOnsetShift = 1; c.parentOf = "PN01"; r.push_back (c);
            Row d = r[8]; d.id = "PNc2"; d.oursOnly = { { "window", 33.0f / 64.0f } }; d.parentOf = "PN09"; r.push_back (d);
            Row e = r[18]; e.id = "PNc3"; e.deltaAdj = 1; r.push_back (e);   // parent rendered with Δ + 1
        }

        // statistics (R§3.3): 4 streams × 200 g/s × 20 ms, 5 s per side
        auto ps = [&r] (const char* id, std::vector<KV> o, double secs = 5.0)
        {
            Row x; x.id = id; x.kind = Kind::stats; x.streams = 4;
            x.sp = over (over (nullBase(), { { "density", 200.0f }, { "size", 20.0f }, { "position", 50.0f } }), o);
            x.seconds = secs;
            r.push_back (x);
        };
        ps ("PS1", { { "spray", 30.0f }, { "size_rand", 50.0f }, { "stereo_spread", 70.0f }, { "pitch_rand", 7.0f },
                     { "reverse_prob", 30.0f } });
        ps ("PS2", { { "gain_rand", 50.0f } });
        ps ("PS3", { { "jitter", 50.0f } }, 5.02);   // displacements drop k = 0 per sequence: 5.02 s keeps n ≥ 4000
        ps ("PS4", { { "scale", 2.0f /* Major */ }, { "pitch_rand", 12.0f } });

        // ── 2.3 DSP-03 null rows (PLAN-2.3 T3, PD5–PD7): PN01 geometry + FX ──
        auto pf = [&r] (const std::string& id, std::vector<KV> o, float peak)
        {
            Row x; x.id = id; x.kind = Kind::fxNull; x.stream = 1;
            x.sp = over (over (nullBase(), fxBase()), o); x.seconds = 1.0; x.matPeak = peak;
            r.push_back (x);
        };
        pf ("PF0", {}, 0.25f);
        {
            const float cut[3] { 200.0f, 2000.0f, 12000.0f };
            const float res[3] { 0.0f, 50.0f, 100.0f };
            const char* fam[3] { "PFL", "PFB", "PFH" };
            for (int t = 0; t < 3; ++t)
                for (int c = 0; c < 3; ++c)
                    for (int q = 0; q < 3; ++q)
                        pf (std::string (fam[t]) + std::to_string (3 * c + q + 1),
                            { { "filter_type", (float) (t + 1) }, { "filter_cutoff", cut[c] }, { "filter_res", res[q] } }, 0.1f);
        }
        {
            const float dv[4] { 1.0f, 25.0f, 50.0f, 100.0f };
            for (int i = 0; i < 4; ++i)
                pf ("PFD" + std::to_string (i + 1), { { "drive", dv[i] } }, 0.25f);
        }
        {
            const float cv[4] { 10.0f, 40.0f, 70.0f, 100.0f };
            for (int i = 0; i < 4; ++i)
                pf ("PFC" + std::to_string (i + 1), { { "crush", cv[i] }, { "crush_prob", 0.0f } }, 0.25f);
            for (int i = 0; i < 3; ++i)
                pf ("PFC" + std::to_string (i + 5), { { "crush", cv[i] }, { "crush_prob", 100.0f } }, 0.25f);
            // PD5: at 0.25 crush 100 % is silent on both sides (scale 2, |2x| <= 0.5 → 0);
            // 0.4 peak + stream level −6 dB (post-FX) keeps the parent below 0.45.
            pf ("PFC8", { { "crush", 100.0f }, { "crush_prob", 100.0f }, { "level", -6.0f } }, 0.4f);
        }
        {
            const float rf[3] { 20.0f, 200.0f, 5000.0f };
            const float rm[2] { 25.0f, 100.0f };
            int k = 1;
            for (int f = 0; f < 3; ++f)
                for (int m = 0; m < 2; ++m)
                    pf ("PFR" + std::to_string (k++), { { "ring_freq", rf[f] }, { "ring_mix", rm[m] } }, 0.25f);
        }
        for (int t = 0; t < 3; ++t)
            pf ("PFX" + std::to_string (t + 1), { { "filter_type", (float) (t + 1) }, { "filter_cutoff", 2000.0f }, { "filter_res", 80.0f },
                                                 { "drive", 50.0f }, { "crush", 40.0f }, { "crush_prob", 100.0f },
                                                 { "ring_freq", 200.0f }, { "ring_mix", 50.0f } }, 0.25f);
        // controls (ours only; the parent render is reused)
        {
            auto byId = [&r] (const std::string& id) -> Row { for (const auto& x : r) if (x.id == id) return x; return Row {}; };
            const float semi = 1.0594630943592953f;   // 2^(1/12)
            Row c1 = byId ("PFB5"); c1.id = "PFc1"; c1.parentOf = "PFB5"; c1.oursOnly = { { "filter_cutoff", 2000.0f * semi } }; r.push_back (c1);
            Row c2 = byId ("PFD3"); c2.id = "PFc2"; c2.parentOf = "PFD3"; c2.oursOnly = { { "drive", 51.0f } }; r.push_back (c2);
            Row c3 = byId ("PFC6"); c3.id = "PFc3"; c3.parentOf = "PFC6"; c3.oursOnly = { { "crush", 41.0f } }; r.push_back (c3);
            Row c4 = byId ("PFR4"); c4.id = "PFc4"; c4.parentOf = "PFR4"; c4.oursOnly = { { "ring_freq", 200.0f * semi } }; r.push_back (c4);
            Row c5 = byId ("PFX1"); c5.id = "PFc5"; c5.parentOf = "PFX1"; c5.oursHooks = { "fxOrderSwap" }; r.push_back (c5);
            Row c6 = byId ("PFL6"); c6.id = "PFc6"; c6.parentOf = "PFL6"; c6.oursHooks = { "legacySvfMake" }; r.push_back (c6);
        }

        // ── 2.3 statistics (the ps() geometry, fxStats) ──
        auto pfs = [&r] (const char* id, std::vector<KV> o)
        {
            Row x; x.id = id; x.kind = Kind::fxStats; x.streams = 4;
            x.sp = over (over (over (nullBase(), fxBase()), { { "density", 200.0f }, { "size", 20.0f }, { "position", 50.0f } }), o);
            x.seconds = 5.0;
            r.push_back (x);
        };
        pfs ("PS5", { { "filter_type", 1.0f }, { "filter_cutoff", 1000.0f }, { "filter_rand", 70.0f } });
        pfs ("PS6", { { "ring_freq", 200.0f }, { "ring_mix", 50.0f }, { "ring_rand", 70.0f } });
        pfs ("PS7", { { "crush", 50.0f }, { "crush_prob", 30.0f } });

        // ── 2.4 PB: FeedbackChain leaf nulls (PLAN-2.4 PD12) ──────────────────
        //   PN01 geometry, 1 voice, s1, matPeak 0.25, frozen parent (no capture
        //   push: its chain runs on the live tap, output unchanged); 32-sample
        //   blocks so the parent return is read once per cell.
        auto pb = [&r] (const std::string& id, std::vector<KV> g)
        {
            Row x; x.id = id; x.kind = Kind::fbNull; x.stream = 1;
            x.sp = over (nullBase(), fxBase()); x.seconds = 1.0; x.matPeak = 0.25f; x.blockSize = 32;
            x.gl = over (fbRevBase(), g);
            x.oursHooks = { "chainKnobParity" };
            r.push_back (x);
        };
        {
            const float lowHigh[3][2] { { 20.0f, 20000.0f }, { 80.0f, 12000.0f }, { 2000.0f, 1000.0f } };
            const float fb[2] { 25.0f, 100.0f };
            int k = 1;
            for (int f = 0; f < 2; ++f)
                for (int c = 0; c < 3; ++c)
                    pb ("PB" + std::to_string (k++), { { "feedback", fb[f] }, { "fb_low_cut", lowHigh[c][0] }, { "fb_high_cut", lowHigh[c][1] } });
            // PB5 = f 100 at the default cutoffs; PB7–9 sat {0, 50, 100}; PB10–12 diffusion {25, 50, 100}
            const float sat[3] { 0.0f, 50.0f, 100.0f };
            for (int i = 0; i < 3; ++i)
                pb ("PB" + std::to_string (k++), { { "feedback", 100.0f }, { "fb_saturation", sat[i] } });
            const float dif[3] { 25.0f, 50.0f, 100.0f };
            for (int i = 0; i < 3; ++i)
                pb ("PB" + std::to_string (k++), { { "feedback", 100.0f }, { "fb_diffusion", dif[i] } });
        }
        {
            auto byId = [&r] (const std::string& id) -> Row { for (const auto& x : r) if (x.id == id) return x; return Row {}; };
            const float semi = 1.0594630943592953f;   // 2^(1/12)
            Row p0 = byId ("PB5"); p0.id = "PB0"; p0.parentOf = "PB5"; r.push_back (p0);   // tap precondition + g_PB
            Row pd = byId ("PB5"); pd.id = "PBd"; pd.parentOf = "PB5"; r.push_back (pd);   // delay row
            Row c1 = byId ("PB5"); c1.id = "PBc1"; c1.parentOf = "PB5"; c1.oursGl = { { "fb_low_cut", 80.0f * semi } }; r.push_back (c1);
            Row c2 = byId ("PB1"); c2.id = "PBc2"; c2.parentOf = "PB1"; c2.oursHooks = {}; r.push_back (c2);   // unity knob
            Row c3 = byId ("PB11"); c3.id = "PBc3"; c3.parentOf = "PB11"; c3.oursGl = { { "fb_diffusion", 51.0f } }; r.push_back (c3);
            Row c4 = byId ("PB9"); c4.id = "PBc4"; c4.parentOf = "PB9"; c4.oursHooks = { "chainKnobParity", "testAgcLegacy" }; r.push_back (c4);
            Row c5 = byId ("PB5"); c5.id = "PBc5"; c5.parentOf = "PB5"; c5.oursHooks = { "chainKnobParity", "retLag31" }; r.push_back (c5);
        }
        // ── 2.4 PR: reverb + knee (PLAN-2.4 PD12) ────────────────────────────
        auto pr = [&r] (const std::string& id, std::vector<KV> g, float peak)
        {
            Row x; x.id = id; x.kind = Kind::revNull; x.stream = 1;
            x.sp = over (nullBase(), fxBase()); x.seconds = 1.0; x.matPeak = peak;
            x.gl = over (fbRevBase(), g);
            r.push_back (x);
        };
        {
            const float mix[2] { 25.0f, 100.0f }, size[3] { 0.0f, 50.0f, 100.0f }, damp[2] { 0.0f, 100.0f };
            int k = 1;
            for (int m = 0; m < 2; ++m)
                for (int z = 0; z < 3; ++z)
                    for (int d = 0; d < 2; ++d)
                        pr ("PR" + std::to_string (k++), { { "reverb_mix", mix[m] }, { "reverb_size", size[z] }, { "reverb_damp", damp[d] } }, 0.25f);
            pr ("PRk", { { "reverb_mix", 100.0f }, { "reverb_size", 100.0f }, { "reverb_damp", 50.0f } }, 0.4f);
            auto byId = [&r] (const std::string& id) -> Row { for (const auto& x : r) if (x.id == id) return x; return Row {}; };
            Row c1 = byId ("PR9"); c1.id = "PRc1"; c1.parentOf = "PR9"; c1.oursGl = { { "reverb_size", 51.0f } }; r.push_back (c1);
            Row c2 = byId ("PRk"); c2.id = "PRc2"; c2.parentOf = "PRk"; c2.oursHooks = { "legacyWetTanh" }; r.push_back (c2);
        }

        // ── 2.5a PL / PM / PE: matrix nulls (PLAN-2.5a PD13) ─────────────────
        //   PN01 geometry, s1 level −12 dB, peak 0.1, every 2.4 global off.
        //   LFO rates differ per LFO; every source busy in PL0.
        auto pl = [&r] (const std::string& id, std::vector<Route> routes, std::vector<KV> g = {})
        {
            Row x; x.id = id; x.kind = Kind::modNull; x.stream = 1;
            x.sp = over (over (nullBase(), fxBase()), { { "level", -12.0f } });
            x.seconds = 1.0; x.matPeak = 0.1f; x.blockSize = 512;
            x.gl = over (over (fbRevBase(), { { "lfo1_rate", 3.7f }, { "lfo2_rate", 2.3f }, { "lfo3_rate", 5.1f }, { "lfo4_rate", 7.9f },
                                               { "lfo1_shape", 0.0f }, { "lfo2_shape", 0.0f }, { "lfo3_shape", 2.0f }, { "lfo4_shape", 4.0f },
                                               { "lfo1_phase", 0.0f }, { "lfo2_phase", 0.0f }, { "lfo3_phase", 0.0f }, { "lfo4_phase", 0.0f },
                                               { "sync_mode", 0.0f }, { "macro1", 0.0f }, { "macro2", 0.0f }, { "macro3", 0.0f }, { "macro4", 0.0f } }), g);
            x.routes = routes;
            r.push_back (x);
            return r.size() - 1;
        };
        const std::string L1 = "S1 \xC2\xB7 Level";
        {
            // PL0: no route, every source busy (macros stepping, LFOs running).
            const auto i0 = pl ("PL0", {});
            r[i0].events = { { atBlock (10), "macro1", 70.0f }, { atBlock (30), "macro2", 40.0f }, { atBlock (50), "macro3", 90.0f },
                             { atBlock (70), "macro4", 20.0f } };
            for (int k = 0; k < 6; ++k)
                pl ("PL" + std::to_string (k + 1), { { 1, "LFO 1", L1, 50.0f } }, { { "lfo1_shape", static_cast<float> (k) } });
            pl ("PL7", { { 1, "LFO 2", L1, -50.0f } });
            pl ("PL8", { { 1, "LFO 3", L1, -50.0f } });
            pl ("PL9", { { 1, "LFO 4", L1, -50.0f } });
            const auto i10 = pl ("PL10", { { 1, "LFO 1", L1, 50.0f } });
            r[i10].events = { { atBlock (20), "lfo1_phase", 90.0f }, { atBlock (45), "lfo1_phase", 359.0f }, { atBlock (70), "lfo1_phase", 1.0f } };
            // Tempo (sync_mode Tempo puts the parent streams on the R41 grid too: deferred to T12)
            const auto i11 = pl ("PL11", { { 1, "LFO 1", L1, 50.0f } }, { { "sync_mode", 1.0f }, { "lfo1_rate_sync", 13.0f } });
            r[i11].tp.on = true; r[i11].tp.bpm = r[i11].tp.bpm2 = 120.0; r[i11].tp.ppq0 = 0.0; r[i11].deferred = true;
            const auto i12 = pl ("PL12", { { 1, "LFO 1", L1, 50.0f } }, { { "sync_mode", 1.0f }, { "lfo1_rate_sync", 9.0f }, { "lfo1_shape", 4.0f } });
            r[i12].tp.on = true; r[i12].tp.bpm = r[i12].tp.bpm2 = 133.0; r[i12].tp.ppq0 = 0.5; r[i12].deferred = true;
            const auto i13 = pl ("PL13", { { 1, "LFO 1", L1, 50.0f } }, { { "sync_mode", 1.0f }, { "lfo1_rate_sync", 18.0f }, { "lfo1_shape", 2.0f } });
            r[i13].tp.on = true; r[i13].tp.bpm = 97.3; r[i13].tp.ppq0 = 0.0; r[i13].tp.stepAt = atBlock (30); r[i13].tp.bpm2 = 128.0;
            r[i13].tp.loopAt = atBlock (60); r[i13].tp.loopBack = 2.0; r[i13].deferred = true;
            pl ("PL14", { { 1, "LFO 2", "LFO 1 \xC2\xB7 Rate", 30.0f }, { 2, "LFO 1", L1, 50.0f } });
            pl ("PL15", { { 1, "LFO 1", "Output Gain", 30.0f } });
            pl ("PLs1", { { 1, "LFO 1", "S1 \xC2\xB7 Size", 10.0f } });
            for (int m = 1; m <= 4; ++m)
            {
                const auto i = pl ("PM" + std::to_string (m), { { 1, "Macro " + std::to_string (m), L1, 50.0f } });
                const std::string id = "macro" + std::to_string (m);
                r[i].events = { { atBlock (15), id, 70.0f }, { atBlock (55), id, 20.0f } };
            }
            const auto e1 = pl ("PE1", { { 1, "LFO 1", L1, 60.0f } });
            r[e1].edits = { { atBlock (40), 1, "", "", true, -60.0f } };
            const auto e2 = pl ("PE2", { { 1, "LFO 1", L1, 0.0f } });
            r[e2].edits = { { atBlock (40), 1, "", "", true, 60.0f } };
            const auto e3 = pl ("PE3", { { 1, "LFO 1", L1, 50.0f } });
            r[e3].edits = { { atBlock (40), 1, "LFO 2", "", false, 0.0f } };
            const auto e4 = pl ("PE4", { { 1, "LFO 1", L1, 50.0f } });
            r[e4].edits = { { atBlock (40), 1, "", "Output Gain", false, 0.0f } };
            const auto e5 = pl ("PE5", { { 1, "LFO 1", L1, 50.0f } });
            r[e5].edits = { { atBlock (40), 1, "None", "", false, 0.0f } };
            pl ("PE6", { { 1, "Macro 1", L1, 100.0f }, { 2, "Macro 2", L1, 100.0f }, { 3, "LFO 1", L1, 60.0f }, { 4, "Macro 3", L1, -40.0f } },
                { { "macro1", 40.0f }, { "macro2", 50.0f }, { "macro3", 30.0f } });

            // controls (ours only; the parent render is reused)
            auto byId = [&r] (const std::string& id) -> Row { for (const auto& x : r) if (x.id == id) return x; return Row {}; };
            Row c1 = byId ("PL1"); c1.id = "PLc1"; c1.parentOf = "PL1"; c1.oursGl = { { "lfo1_phase", 1.0f } }; r.push_back (c1);
            Row c2 = byId ("PL5"); c2.id = "PLc2"; c2.parentOf = "PL5"; c2.seed = c2.seed + 1; r.push_back (c2);
            Row c3 = byId ("PL11"); c3.id = "PLc3"; c3.parentOf = "PL11"; c3.tp.bpmAdd = 0.1; r.push_back (c3);
            Row m1 = byId ("PM1"); m1.id = "PMc1"; m1.parentOf = "PM1"; m1.oursHooks = { "macroRampZero" }; r.push_back (m1);
            Row q1 = byId ("PE1"); q1.id = "PEc1"; q1.parentOf = "PE1"; q1.oursHooks = { "noDepthRamp" }; r.push_back (q1);
            Row q2 = byId ("PE3"); q2.id = "PEc2"; q2.parentOf = "PE3"; q2.oursHooks = { "noResidual" }; r.push_back (q2);
            Row q3 = byId ("PE1"); q3.id = "PEc3"; q3.parentOf = "PE1"; q3.edits.clear(); q3.oursEdits = { { atBlock (40), 1, "", "", true, -59.0f } }; r.push_back (q3);
            // PL0t: the per-tick s1_level trace on the PL1 stimulus (the phase precondition).
            Row t0 = byId ("PL1"); t0.id = "PL0t"; t0.traceDest = L1; r.push_back (t0);
        }

        // D16 gain: parent live (not frozen), defaults, mix 100 %; 4 seeds
        for (int s = 1; s <= 4; ++s)
        {
            Row g; g.id = "GV" + std::to_string (s); g.kind = Kind::gv; g.seconds = kGvSeconds;
            g.seed = 0x6A1D0000ull + (uint64_t) s;
            r.push_back (g);
        }
        return r;
    }

    // ── solver (R§2.3) ──────────────────────────────────────────────────────
    struct Geometry
    {
        bool    ok = false;
        std::string why;
        int     level = 0;
        double  rate = 1.0, len = 0.0;
        int64_t p0 = 0, A = 0, delta = 0;
        int     dir = 1;
    };

    /** Closed form for a null row. `sizeRaw` / `densityRaw` are the LANDED raw
        values (the skewed ranges may not land exactly on the row value). */
    inline Geometry solve (const Row& row, float sizeRaw)
    {
        Geometry g;
        const double st  = findV (row.sp, "pitch", 0.0f);
        const double pos = findV (row.sp, "position", 0.0f) / 100.0;
        g.dir   = findV (row.sp, "reverse_prob", 0.0f) >= 100.0f ? -1 : 1;
        g.rate  = std::exp2 (st / 12.0);
        g.level = levelForRate (g.rate);
        g.len   = static_cast<double> (sizeRaw) * 0.001 * kFs;
        const double xF = pos * static_cast<double> (kF);
        const double xS = pos * static_cast<double> (kS);
        if (xF != std::floor (xF) || xS != std::floor (xS)) { g.why = "x·F or pos·S not an integer"; return g; }
        g.p0 = static_cast<int64_t> (xF);
        g.A  = kW0 - margin (g.level) - static_cast<int64_t> (xS);
        g.delta = g.A - g.p0 + row.deltaAdj;
        const double reach = g.rate * g.len;
        const int64_t step = int64_t (1) << g.level;
        if (g.delta < 0)                                   { g.why = "delta < 0"; return g; }
        if (row.deltaAdj == 0 && g.delta % step != 0)      { g.why = "delta not a multiple of 2^l"; return g; }
        if (g.delta + kF > kHeld)                          { g.why = "material not captured before W0 - 223"; return g; }
        if (g.dir > 0 && static_cast<double> (g.p0) + reach + 64.0 > static_cast<double> (kF)) { g.why = "forward span leaves the material"; return g; }
        if (g.dir < 0 && static_cast<double> (g.p0) - reach - 64.0 < 0.0)                      { g.why = "reverse span leaves the material"; return g; }
        if (g.dir > 0 && xS < reach + 1.0)                 { g.why = "parent forward floor binds"; return g; }
        if (g.dir < 0 && static_cast<double> (margin (g.level)) + xS > static_cast<double> (kHeld) - reach) { g.why = "parent reverse cap binds"; return g; }
        g.ok = true;
        return g;
    }

    /** Parent input at absolute sample k for a null / stats row. */
    inline float parentInput (const Geometry& g, int64_t k, int ch, float peak = kMatPeak)
    {
        const int64_t n = k - g.delta;
        return (n >= 0 && n < kF) ? material (n, ch, peak) : 0.0f;
    }

    // ── SC grid (T9 SC1 / SC3) ──────────────────────────────────────────────
    inline const std::vector<float>& scSpans()   { static const std::vector<float> v { 50.0f, 700.0f, 2400.0f }; return v; }
    inline const std::vector<double>& scPitches() { static const std::vector<double> v { -12.0, 0.0, 7.0 }; return v; }
    constexpr uint64_t kScFixedMask = 0xA5C3F00F5AA5C33Cull;   // canonicalised per collection
    constexpr uint64_t kScBit63     = 0x8000000000000000ull;

    /** Custom-scale texts for SC1 (choice 31) and SC3 (parse). */
    inline const std::vector<std::string>& scCustomTexts()
    {
        static const std::vector<std::string> t {
            "edo 19",
            "edo 64",
            "0 100 250.5 386.3137 498.045 700 884.359 1088.269 1200",
            "1/1 9/8 5/4 4/3 3/2 5/3 15/8 2/1",
            "0 150 300 x 450",          // bad token
        };
        return t;
    }
} // namespace parity
