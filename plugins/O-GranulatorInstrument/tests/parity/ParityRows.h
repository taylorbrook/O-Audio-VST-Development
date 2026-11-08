#pragma once
// ============================================================================
//   O-GranulatorInstrument — DSP-01 / DSP-03 parity rows (PLAN-2.2 T3 / PD5,
//   R§1–§3; PLAN-2.3 T3: PF / PFc / PS5–7)
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

    enum class Kind { null_, stats, gv, fxNull, fxStats };   // fx*: PLAN-2.3 PD11 (pg22 never sees them)

    inline bool isNullKind (Kind k)  { return k == Kind::null_ || k == Kind::fxNull; }
    inline bool isStatsKind (Kind k) { return k == Kind::stats || k == Kind::fxStats; }

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
        std::vector<std::string> oursHooks;   // our-side test hooks: "legacySvfMake", "fxOrderSwap"
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
