// O-ParametricEQ, Stage 2 research for Phase 2.4 (structural crossfades and bypass).
// A scratch bench over the real Source/dsp headers: a prototype of the two-slot
// band (two eq::Band objects, a raised-cosine crossfade, a latched request, the
// neutral sleep) and of the global soft bypass, in double. Not plugin code and
// not a gate.
//
//   clang++ -O3 -std=c++17 -I ../../../../Source fade_bench.cpp -o <scratch>/fade_bench
//   fade_bench <mode> [args]
//
// Modes:
//   ident     a gain type at exactly 0 dB: is the design the identity, and how far is the output from the input
//   g3        gate G3 step rows: local kappa against the held references, a warm two-filter crossfade, and the hard control
//   g3var     the same rows for other fade lengths and weight shapes (worst ratios)
//   sleep     neutral sleep and wake: click reading with and without the fades, against an engine that never sleeps
//   g4        time to bit transparency: (d) bells at 0 dB, (e) gain scale 0 %, (f) global bypass
//   mix       structural changes that meet a continuous move, a latched request, block-rate toggling
//   blk       bit identity over host block sizes with structural events on multiples of 2048
//   g5 [s]    random stress with structural parameters and bypass randomised, 8 rates (default 20 s per rate)
//   cpu       cost of a fade
//   dip [ms]  level dip after a step: what the incoming slot's start from rest costs, against a warm crossfade;
//             the argument is a pre-roll of the incoming slot in ms (default 0). DIP_ALL=1 lists every row
//
// Environment: PREROLL_MS=<ms> gives the g3 mode the same pre-roll.

#include "dsp/Designer.h"
#include "dsp/ResponseEval.h"
#include "dsp/BandParams.h"
#include "dsp/Band.h"
#include "dsp/Router.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

using namespace eq;

//==============================================================================
static uint64_t rngState = 0x9E3779B97F4A7C15ull;
static double urand()
{
    rngState ^= rngState << 13;  rngState ^= rngState >> 7;  rngState ^= rngState << 17;
    return (double) (rngState >> 11) * (1.0 / 9007199254740992.0);
}

using Clock = std::chrono::steady_clock;
static double secs (Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double> (b - a).count(); }

static void setFlushToZero()
{
   #if defined (__aarch64__)
    uint64_t fpcr;
    __asm__ __volatile__ ("mrs %0, fpcr" : "=r" (fpcr));
    fpcr |= (1ull << 24);
    __asm__ __volatile__ ("msr fpcr, %0" : : "r" (fpcr));
   #endif
}

static const char* typeName (Type t)
{
    static const char* n[] = { "Bell", "LowShelf", "HighShelf", "LowCut", "HighCut", "Notch", "BandPass", "Tilt" };
    return n[(int) t];
}

static const char* placeName (Placement p)
{
    static const char* n[] = { "Stereo", "Left", "Right", "Mid", "Side" };
    return n[(int) p];
}

//==============================================================================
// Prototype: the two-slot band
//==============================================================================
enum Shape { shapeCos = 0, shapeLinear = 1, shapePower = 2 };

struct Opt
{
    double fadeMs = 20.0;
    int shape = shapeCos;
    bool fades = true;          // false: the Phase 2.3 behaviour (design, state cleared, no fade)
    bool sleep = true;
    bool sleepFade = true;      // false: a sleeping band is dropped without a fade
    bool wakeFade = true;       // false: a waking band starts without a fade
    double sleepMs = 50.0;
    double prerollMs = 0.0;     // the incoming slot runs this long at weight 0 before the fade starts
    bool bypassFade = true;
    bool bypassLatch = true;    // a bypass change during the bypass fade waits for the fade to end (false: the fade reverses)
};

static long fadesStarted = 0, latched = 0, sleeps = 0, wakes = 0;

struct FBand
{
    Band slot[2];
    Placement place[2] = { Placement::stereo, Placement::stereo };
    bool off[2] = { true, true };       // an off slot contributes the dry band input
    int cur = 0;                        // the slot that runs when no fade is in flight; during a fade, the outgoing one
    bool fading = false;
    int pos = 0;

    BandInput in {};                    // what the incoming slot (or the current one) realises
    bool asleep = false;                // in.active, gain type, gain exactly 0, and the slot is off
    BandInput desired {};               // the latest request
    int zeroTicks = 0;
};

static bool gainIsZero (const BandInput& b)
{
    return isGainType (b.settings.type) && sameValue (b.settings.gainDbEff, 0.0);
}

struct FEngine
{
    double fs = 48000;
    int K = 16, rampTicks = 60, N = 960, sleepTicks = 150, NB = 960;
    bool stereo = true;
    int phase = 0;
    Opt opt;
    Designer des;
    std::vector<FBand> bands;
    std::vector<double> wtab;           // weight of the incoming slot at fade sample n

    // global bypass: bpos 0 = fully processed, NB = fully bypassed
    bool bypTarget = false, bypWanted = false;
    int bpos = 0;
    std::vector<double> btab;           // dry weight at position p (1 ... NB - 1)

    static double weight (double x, int shape)
    {
        if (shape == shapeLinear) return x;
        if (shape == shapePower)  return std::sin (0.5 * pi * x);
        return 0.5 - 0.5 * std::cos (pi * x);
    }

    void prepare (double rate, int numBands, bool isStereo = true)
    {
        fs = rate;  stereo = isStereo;
        K = controlGridSize (rate);
        rampTicks = rampLengthTicks (rate);
        const int fadeLen = std::max (1, (int) std::lround (opt.fadeMs * 0.001 * rate));
        const int pre = std::max (0, (int) std::lround (opt.prerollMs * 0.001 * rate));
        N = pre + fadeLen;
        NB = fadeLen;
        sleepTicks = std::max (1, (int) std::lround (opt.sleepMs * 0.001 * rate / K));
        des.prepare (rate);
        bands.assign ((size_t) numBands, FBand());
        for (auto& b : bands)
        {
            for (auto& s : b.slot) { s.setControlGrid (K, rampTicks);  s.reset(); }
            b.in.active = false;  b.in.placement = Placement::stereo;
            b.in.settings = BandSettings { Type::bell, 1000.0, 0.0, 1.0, 0 };
            b.desired = b.in;
        }
        wtab.resize ((size_t) N);
        for (int n = 0; n < N; ++n) wtab[(size_t) n] = n < pre ? 0.0 : weight ((double) (n - pre + 1) / (double) (fadeLen + 1), opt.shape);
        btab.resize ((size_t) NB + 1);
        for (int p = 0; p <= NB; ++p) btab[(size_t) p] = weight ((double) p / (double) NB, shapeCos);
        phase = 0;  bypTarget = bypWanted = false;  bpos = 0;
    }

    //==========================================================================
    void configureSlot (FBand& b, int index, const BandSettings& s, Placement p)
    {
        b.slot[index].snapTo (s, des);
        b.slot[index].reset();
        b.place[index] = p;
        b.off[index] = false;
    }

    void retire (FBand& b, int index)
    {
        b.slot[index].reset();
        b.slot[index].snapToTarget (des);
        b.off[index] = true;
    }

    void startFade (FBand& b)
    {
        b.fading = true;  b.pos = 0;  b.zeroTicks = 0;
        ++fadesStarted;
    }

    // The band is not fading. Brings it to `next`.
    void settle (FBand& b, const BandInput& next)
    {
        const BandChange change = classifyBandChange (b.in, next);

        if (change == BandChange::identical)
            return;

        const bool nextSleeps = opt.sleep && next.active && gainIsZero (next);

        if (change == BandChange::continuous)
        {
            if (! next.active)                  { b.in = next;  return; }

            if (b.asleep)
            {
                if (nextSleeps)                 { b.in = next;  return; }

                // wake: the slot starts at the band's frequency and Q at 0 dB, then ramps like any gain move
                BandSettings zero = next.settings;
                zero.gainDbEff = 0.0;
                ++wakes;

                if (opt.wakeFade && opt.fades)
                {
                    const int to = 1 - b.cur;
                    configureSlot (b, to, zero, next.placement);
                    b.slot[to].setTarget (next.settings);
                    startFade (b);
                }
                else
                {
                    configureSlot (b, b.cur, zero, next.placement);
                    b.slot[b.cur].setTarget (next.settings);
                }

                b.asleep = false;  b.in = next;
                return;
            }

            b.slot[b.cur].setTarget (next.settings);
            b.in = next;
            return;
        }

        // structural
        const bool toOff = ! next.active || nextSleeps;

        if (toOff && b.off[b.cur])
        {
            b.in = next;  b.asleep = nextSleeps;
            return;
        }

        if (! opt.fades)
        {
            if (toOff) retire (b, b.cur);
            else       configureSlot (b, b.cur, next.settings, next.placement);
        }
        else
        {
            const int to = 1 - b.cur;
            if (toOff) b.off[to] = true;
            else       configureSlot (b, to, next.settings, next.placement);
            startFade (b);
        }

        b.in = next;  b.asleep = nextSleeps;  b.zeroTicks = 0;
    }

    void endFade (FBand& b)
    {
        retire (b, b.cur);
        b.cur = 1 - b.cur;
        b.fading = false;  b.pos = 0;

        if (classifyBandChange (b.in, b.desired) != BandChange::identical)
        {
            ++latched;
            settle (b, b.desired);
        }
    }

    // Called at a host block start with the canonical input of the band.
    void setInput (int index, const BandInput& next)
    {
        FBand& b = bands[(size_t) index];
        b.desired = next;

        if (bypTarget && bpos >= NB)    // idle: the request is kept and applied at un-bypass
            return;

        if (b.fading)
        {
            // A continuous change reaches the incoming slot at once; anything else waits for the fade to end.
            if (classifyBandChange (b.in, next) == BandChange::continuous && next.active && ! b.asleep
                && ! (opt.sleep && gainIsZero (b.in) && b.off[1 - b.cur]))
            {
                b.slot[1 - b.cur].setTarget (next.settings);
                b.in = next;
            }
            return;
        }

        settle (b, next);
    }

    void setInput (int index, const BandSettings& s, Placement p = Placement::stereo, bool active = true)
    {
        BandInput in;
        in.settings = s;  in.placement = effectivePlacement (p, stereo);  in.active = active;
        if (! usesSlope (s.type)) in.settings.slopeIndex = 0;
        else if (in.settings.slopeIndex == 0) in.settings.q = 1.0;
        if (! isGainType (s.type)) in.settings.gainDbEff = 0.0;
        setInput (index, in);
    }

    void setBypass (bool on)
    {
        bypWanted = on;
        const bool inFade = bpos > 0 && bpos < NB;
        if (inFade && opt.bypassLatch && opt.bypassFade) return;
        applyBypass (on);
    }

    void applyBypass (bool on)
    {
        if (on == bypTarget) return;
        bypTarget = on;

        if (! on && bpos >= NB)
        {
            // Un-bypass from idle: every band starts from rest on its latest request, settled, with no band fade.
            for (auto& b : bands)
            {
                b.in.active = false;  b.asleep = false;  b.fading = false;  b.off[0] = b.off[1] = true;
                const bool f = opt.fades;  opt.fades = false;
                settle (b, b.desired);
                opt.fades = f;
            }
            if (! opt.bypassFade) bpos = 0;
        }
        else if (on && ! opt.bypassFade)
            enterIdle();
    }

    void enterIdle()
    {
        bpos = NB;
        for (auto& b : bands)
        {
            if (b.fading) { retire (b, b.cur);  b.cur = 1 - b.cur;  b.fading = false; }
            if (! b.off[b.cur]) retire (b, b.cur);
            b.in.active = false;  b.asleep = false;  b.zeroTicks = 0;
        }
    }

    //==========================================================================
    void tick()
    {
        for (auto& b : bands)
        {
            if (! b.off[0]) b.slot[0].controlTick (des);
            if (! b.off[1]) b.slot[1].controlTick (des);

            if (opt.sleep && ! b.fading && ! b.off[b.cur] && b.in.active && gainIsZero (b.in) && ! b.slot[b.cur].isMoving())
            {
                if (++b.zeroTicks >= sleepTicks)
                {
                    ++sleeps;
                    b.asleep = true;
                    if (opt.sleepFade && opt.fades) { b.off[1 - b.cur] = true;  startFade (b); }
                    else                             retire (b, b.cur);
                    b.zeroTicks = 0;
                }
            }
            else
                b.zeroTicks = 0;
        }
    }

    // m <= K samples of one band.
    void runBand (FBand& b, double* L, double* R, int m)
    {
        while (m > 0)
        {
            if (! b.fading)
            {
                if (! b.off[b.cur])
                    Router::process (b.slot[b.cur], b.place[b.cur], L, R, m, stereo);
                return;
            }

            const int n = std::min (m, N - b.pos);
            const int from = b.cur, to = 1 - b.cur;
            double a[2][64], c[2][64];
            const int nl = stereo ? 2 : 1;
            double* io[2] = { L, R };

            for (int l = 0; l < nl; ++l)
                for (int i = 0; i < n; ++i) { a[l][i] = io[l][i];  c[l][i] = io[l][i]; }

            if (! b.off[from]) Router::process (b.slot[from], b.place[from], a[0], a[1], n, stereo);
            if (! b.off[to])   Router::process (b.slot[to],   b.place[to],   c[0], c[1], n, stereo);

            if (opt.shape == shapePower)
            {
                for (int l = 0; l < nl; ++l)
                    for (int i = 0; i < n; ++i)
                    {
                        const double x = (double) (b.pos + i + 1) / (double) (N + 1);
                        io[l][i] = std::cos (0.5 * pi * x) * a[l][i] + std::sin (0.5 * pi * x) * c[l][i];
                    }
            }
            else
            {
                for (int l = 0; l < nl; ++l)
                    for (int i = 0; i < n; ++i)
                        io[l][i] = a[l][i] + wtab[(size_t) (b.pos + i)] * (c[l][i] - a[l][i]);
            }

            b.pos += n;  L += n;  if (R != nullptr) R += n;  m -= n;

            if (b.pos >= N)
                endFade (b);
        }
    }

    // One host block. Returns true when the buffer was not touched at all.
    void process (double* L, double* R, int n)
    {
        int i = 0;
        while (i < n)
        {
            if (bypTarget && bpos >= NB)                    // idle: the clock runs, nothing else
            {
                phase = (phase + (n - i)) % K;
                return;
            }

            if (phase == 0) tick();
            int m = std::min (n - i, K - phase);

            const bool bfade = (bypTarget && bpos < NB) || (! bypTarget && bpos > 0);
            if (bfade)
                m = std::min (m, bypTarget ? NB - bpos : bpos);

            double dry[2][64];
            if (bfade)
            {
                for (int j = 0; j < m; ++j) { dry[0][j] = L[i + j];  if (R != nullptr) dry[1][j] = R[i + j]; }
            }

            for (auto& b : bands)
                runBand (b, L + i, R != nullptr ? R + i : nullptr, m);

            if (bfade)
            {
                for (int j = 0; j < m; ++j)
                {
                    bpos += bypTarget ? 1 : -1;
                    const double wd = btab[(size_t) bpos];
                    L[i + j] = L[i + j] + wd * (dry[0][j] - L[i + j]);
                    if (R != nullptr) R[i + j] = R[i + j] + wd * (dry[1][j] - R[i + j]);
                }
                if (bypTarget && bpos >= NB)
                    enterIdle();
                if ((bpos >= NB || bpos <= 0) && bypWanted != bypTarget)
                    applyBypass (bypWanted);
            }

            phase = (phase + m) % K;
            i += m;
        }
    }

    bool anyFading() const { for (auto& b : bands) if (b.fading) return true;  return false; }
    int numRunning() const { int c = 0; for (auto& b : bands) if (b.fading || ! b.off[b.cur]) ++c;  return c; }
};

//==============================================================================
// Local kappa (gate G3): max over 5 ms frames of max |d2 y| / frame RMS,
// frames below -80 dBFS skipped.
//==============================================================================
static double localKappa (const std::vector<double>& y, double fs, size_t from, size_t to)
{
    const size_t frame = (size_t) std::lround (0.005 * fs);
    double worst = 0.0;

    for (size_t a = std::max<size_t> (from, 1); a + frame < std::min (to, y.size() - 1); a += frame)
    {
        double e = 0.0, d2 = 0.0;
        for (size_t i = a; i < a + frame; ++i)
        {
            e += y[i] * y[i];
            d2 = std::max (d2, std::fabs (y[i + 1] - 2.0 * y[i] + y[i - 1]));
        }
        const double rms = std::sqrt (e / (double) frame);
        if (rms < 1.0e-4) continue;
        worst = std::max (worst, d2 / rms);
    }
    return worst;
}

struct Stereo { std::vector<double> L, R; };

// The largest second difference of either channel: an absolute reading that a level dip cannot inflate.
static double maxD2 (const Stereo& y, size_t from)
{
    double d = 0.0;
    for (size_t i = std::max<size_t> (from, 1); i + 1 < y.L.size(); ++i)
        d = std::max ({ d, std::fabs (y.L[i + 1] - 2.0 * y.L[i] + y.L[i - 1]), std::fabs (y.R[i + 1] - 2.0 * y.R[i] + y.R[i - 1]) });
    return d;
}

static double kappa2 (const Stereo& y, double fs, size_t from)
{
    return std::max (localKappa (y.L, fs, from, y.L.size()), localKappa (y.R, fs, from, y.R.size()));
}

// L and R carry the tone at different levels and phases, so every placement does something different.
static Stereo stimulus (double tone, double fs, size_t n)
{
    Stereo s;  s.L.resize (n);  s.R.resize (n);
    for (size_t i = 0; i < n; ++i)
    {
        const double t = (double) i / fs;
        s.L[i] = 0.5 * std::sin (2.0 * pi * tone * t);
        s.R[i] = 0.3 * std::sin (2.0 * pi * tone * t + 1.1);
    }
    return s;
}

//==============================================================================
// A configuration of one band (or of the whole engine) and the step rows
//==============================================================================
struct Cfg
{
    BandSettings s { Type::bell, 1000.0, 0.0, 1.0, 0 };
    Placement p = Placement::stereo;
    bool active = true;
    bool bypass = false;        // global
};

struct Row { std::string name;  Cfg a, b;  int copies = 1; };

static const double stepSec = 0.100, leadSec = 0.5;
static const int numSteps = 8;

// motion 0: A, then B / A alternating every 100 ms; 1: held at A; 2: held at B
static Stereo renderSteps (const Row& r, double tone, int block, const Opt& opt, int motion, double fs,
                           std::vector<size_t>* fadeStarts = nullptr, double stepSeconds = stepSec)
{
    FEngine e;  e.opt = opt;
    e.prepare (fs, r.copies);

    const size_t n = (size_t) ((leadSec + numSteps * stepSeconds + 0.3) * fs);
    Stereo y = stimulus (tone, fs, n);
    int lastState = -1;

    for (size_t pos = 0; pos < n; pos += (size_t) block)
    {
        const double t = (double) pos / fs;
        int state = 0;
        if (motion == 2) state = 1;
        else if (motion == 0 && t >= leadSec)
        {
            const int k = (int) ((t - leadSec) / stepSeconds);
            state = k < numSteps ? ((k + 1) & 1) : (numSteps & 1);
        }
        const Cfg& c = state ? r.b : r.a;

        if (state != lastState && lastState >= 0 && fadeStarts != nullptr)
            fadeStarts->push_back (pos);
        lastState = state;

        e.setBypass (c.bypass);
        for (int k = 0; k < r.copies; ++k)
            e.setInput (k, c.s, c.p, c.active);

        const int m = (int) std::min<size_t> ((size_t) block, n - pos);
        e.process (y.L.data() + pos, y.R.data() + pos, m);
    }
    return y;
}

// The warm reference: both configurations run from the start, each held, and are mixed with the fade's weights.
// Shares the engine only for the two held renders (no fade, no slot change in either).
static Stereo warmReference (const Stereo& yA, const Stereo& yB, const std::vector<size_t>& starts, double fs, const Opt& opt)
{
    const int N = std::max (1, (int) std::lround (opt.fadeMs * 0.001 * fs));
    const size_t pre = (size_t) std::max (0L, std::lround (opt.prerollMs * 0.001 * fs));
    Stereo y = yA;
    size_t k = 0;
    int state = 0;      // 0 = A
    size_t fadeFrom = (size_t) -1;

    for (size_t i = 0; i < y.L.size(); ++i)
    {
        if (k < starts.size() && i == starts[k] + pre) { fadeFrom = i;  state = 1 - state;  ++k; }

        double w = (double) state;      // weight of B
        if (fadeFrom != (size_t) -1 && i < fadeFrom + (size_t) N)
        {
            const double x = (double) (i - fadeFrom + 1) / (double) (N + 1);
            const double win = 0.5 - 0.5 * std::cos (pi * x);
            w = state ? win : 1.0 - win;
        }
        y.L[i] = yA.L[i] + w * (yB.L[i] - yA.L[i]);
        y.R[i] = yA.R[i] + w * (yB.R[i] - yA.R[i]);
    }
    return y;
}

static Cfg deep (Type t, double tone, double gain = 18.0)
{
    Cfg c;
    c.s.type = t;  c.s.gainDbEff = gain;  c.s.q = 1.0;  c.s.slopeIndex = usesSlope (t) ? 1 : 0;
    switch (t)
    {
        case Type::bell:       c.s.freqHz = tone;  break;
        case Type::lowShelf:   c.s.freqHz = 2.0 * tone;  break;
        case Type::highShelf:  c.s.freqHz = 0.5 * tone;  break;
        case Type::lowCut:     c.s.freqHz = tone;  c.s.slopeIndex = 3;  break;      // corner at the tone, 24 dB/oct
        case Type::highCut:    c.s.freqHz = tone;  c.s.slopeIndex = 3;  break;
        case Type::notch:      c.s.freqHz = 1.15 * tone;  break;
        case Type::bandPass:   c.s.freqHz = 3.0 * tone;  break;
        case Type::tilt:       c.s.freqHz = 0.25 * tone;  c.s.gainDbEff = 2.0 * gain > 60.0 ? 60.0 : 2.0 * gain;  break;
    }
    return c;
}

static std::vector<Row> stepRows (double tone, const std::string& group)
{
    std::vector<Row> rows;
    const Type types[] = { Type::bell, Type::lowShelf, Type::highShelf, Type::lowCut, Type::highCut, Type::notch, Type::bandPass, Type::tilt };

    if (group == "type" || group == "all")
        for (int i = 0; i < 8; ++i)
            for (int j = i + 1; j < 8; ++j)
                rows.push_back ({ std::string ("type ") + typeName (types[i]) + " <> " + typeName (types[j]),
                                  deep (types[i], tone), deep (types[j], tone) });

    if (group == "typecut" || group == "all")
        for (int i = 0; i < 8; ++i)
            for (int j = i + 1; j < 8; ++j)
                if (isGainType (types[i]) || isGainType (types[j]))
                    rows.push_back ({ std::string ("type -18 ") + typeName (types[i]) + " <> " + typeName (types[j]),
                                      deep (types[i], tone, -18.0), deep (types[j], tone, -18.0) });

    if (group == "slope" || group == "all")
        for (Type t : { Type::lowCut, Type::highCut, Type::lowShelf, Type::highShelf, Type::tilt })
            for (int i = 0; i < 8; ++i)
                for (int j = i + 1; j < 8; ++j)
                {
                    Cfg a = deep (t, tone), b = a;
                    a.s.slopeIndex = i;  b.s.slopeIndex = j;
                    rows.push_back ({ std::string ("slope ") + typeName (t) + " " + std::to_string (6 * slopeOrder[i]) + " <> "
                                      + std::to_string (6 * slopeOrder[j]), a, b });
                }

    if (group == "place" || group == "all")
        for (int v = 0; v < 2; ++v)
            for (int i = 0; i < 5; ++i)
                for (int j = i + 1; j < 5; ++j)
                {
                    Cfg a = v == 0 ? deep (Type::bell, tone) : deep (Type::lowCut, tone), b;
                    if (v == 1) a.s.slopeIndex = 7;
                    b = a;
                    a.p = (Placement) i;  b.p = (Placement) j;
                    rows.push_back ({ std::string ("place ") + (v == 0 ? "Bell +18 " : "LowCut 96 ") + placeName (a.p) + " <> " + placeName (b.p), a, b });
                }

    if (group == "enable" || group == "all")
    {
        std::vector<std::pair<std::string, Cfg>> subjects;
        subjects.push_back ({ "Bell +18", deep (Type::bell, tone) });
        subjects.push_back ({ "Bell -18", deep (Type::bell, tone, -18.0) });
        { Cfg c = deep (Type::bell, tone);  c.s.q = 40.0;  subjects.push_back ({ "Bell +18 Q 40", c }); }
        { Cfg c = deep (Type::bell, tone);  c.s.freqHz = 10.0;  c.s.q = 40.0;  c.s.gainDbEff = 12.0;  subjects.push_back ({ "Bell 10 Hz Q 40 +12", c }); }
        { Cfg c = deep (Type::lowCut, tone);  c.s.freqHz = 30.0;  c.s.slopeIndex = 7;  subjects.push_back ({ "LowCut 96 at 30 Hz", c }); }
        { Cfg c = deep (Type::lowCut, tone);  c.s.slopeIndex = 7;  subjects.push_back ({ "LowCut 96 at tone", c }); }
        { Cfg c = deep (Type::lowCut, tone);  c.s.slopeIndex = 7;  c.s.freqHz = 0.8 * tone;  c.s.q = 4.0;  subjects.push_back ({ "LowCut 96 Q 4 below tone", c }); }
        { Cfg c = deep (Type::highCut, tone);  c.s.slopeIndex = 7;  subjects.push_back ({ "HighCut 96 at tone", c }); }
        { Cfg c = deep (Type::highCut, tone);  c.s.slopeIndex = 0;  subjects.push_back ({ "HighCut 6 at tone", c }); }
        subjects.push_back ({ "HighShelf 12 +18", deep (Type::highShelf, tone) });
        { Cfg c = deep (Type::highShelf, tone);  c.s.slopeIndex = 7;  subjects.push_back ({ "HighShelf 96 +18", c }); }
        subjects.push_back ({ "LowShelf 12 -18", deep (Type::lowShelf, tone, -18.0) });
        subjects.push_back ({ "Notch near tone", deep (Type::notch, tone) });
        subjects.push_back ({ "BandPass at 3x tone", deep (Type::bandPass, tone) });
        subjects.push_back ({ "Tilt 12 +36", deep (Type::tilt, tone) });

        for (auto& sub : subjects)
        {
            Cfg off = sub.second;  off.active = false;
            rows.push_back ({ "enable " + sub.first, off, sub.second });
        }
        for (auto& sub : subjects)
        {
            Cfg byp = sub.second;  byp.bypass = true;
            rows.push_back ({ "global bypass, " + sub.first, sub.second, byp });
        }
        {
            Cfg c = deep (Type::bell, tone, 1.0);  c.s.q = 4.0;
            Cfg byp = c;  byp.bypass = true;
            rows.push_back ({ "global bypass, 24 x Bell +1 Q 4", c, byp, 24 });
            Cfg off = c;  off.active = false;
            rows.push_back ({ "enable 24 x Bell +1 Q 4", off, c, 24 });
        }
    }
    return rows;
}

struct Reading { double held, eng, warm, hard, kA, kB, dHeld, dEng, dWarm, dHard; };

static Reading readRow (const Row& r, double tone, int block, double fs, const Opt& opt)
{
    const size_t from = (size_t) (0.3 * fs);
    Opt hard = opt;  hard.fades = false;  hard.bypassFade = false;

    std::vector<size_t> starts;
    const Stereo yA = renderSteps (r, tone, block, opt, 1, fs);
    const Stereo yB = renderSteps (r, tone, block, opt, 2, fs);
    const Stereo yE = renderSteps (r, tone, block, opt, 0, fs, &starts);
    const Stereo yX = renderSteps (r, tone, block, hard, 0, fs);
    const Stereo yW = warmReference (yA, yB, starts, fs, opt);

    Reading q;
    q.kA = kappa2 (yA, fs, from);  q.kB = kappa2 (yB, fs, from);
    q.held = std::max (q.kA, q.kB);
    q.eng = kappa2 (yE, fs, from);  q.warm = kappa2 (yW, fs, from);  q.hard = kappa2 (yX, fs, from);
    q.dHeld = std::max (maxD2 (yA, from), maxD2 (yB, from));
    q.dEng = maxD2 (yE, from);  q.dWarm = maxD2 (yW, from);  q.dHard = maxD2 (yX, from);
    return q;
}

static void modeG3 (const std::string& group, bool verbose)
{
    const double fs = 48000.0;
    Opt opt;
    if (const char* pr = std::getenv ("PREROLL_MS")) opt.prerollMs = std::atof (pr);

    for (double tone : { 50.0, 1000.0 })
    {
        const auto rows = stepRows (tone, group);
        std::printf ("\n== tone %.0f Hz, fs 48 k, steps every 100 ms, fade %.0f ms. kappa of a held sine = %.3e\n", tone, opt.fadeMs,
                     std::sqrt (2.0) * std::pow (2.0 * pi * tone / fs, 2.0));
        if (verbose)
            std::printf ("%-44s %5s | %9s | %8s %8s %8s | %8s %8s | abs d2: %7s %7s %9s\n", "row", "block", "held ref", "eng/held", "warm/held", "hard/held", "eng/warm", "hard/warm", "eng", "warm", "hard");

        struct Agg { double worstAbs = 0, worstAbsWarm = 0, minAbsHard = 1e300;  int absOver = 0, absHardUnder = 0;  std::string aname;  int n = 0, over2 = 0, over2warm = 0, warmOver2 = 0, hardUnder10 = 0, hardUnder10w = 0;  double worst = 0, worstW = 0, worstEW = 0, minHard = 1e300, minHardW = 1e300;  std::string wname, ewname; };
        std::vector<std::pair<std::string, Agg>> groups;

        for (const auto& r : rows)
        {
            const std::string g = r.name.substr (0, r.name.find (' '));
            if (groups.empty() || groups.back().first != g) groups.push_back ({ g, Agg() });
            Agg& a = groups.back().second;

            for (int block : { 32, 512 })
            {
                const Reading q = readRow (r, tone, block, fs, opt);
                const double eh = q.eng / q.held, wh = q.warm / q.held, xh = q.hard / q.held, ew = q.eng / q.warm, xw = q.hard / q.warm;
                if (verbose)
                    std::printf ("%-44s %5d | %9.3e | %8.2f %8.2f %8.2f | %8.2f %8.2f | %15.3f %7.3f %9.2f\n", r.name.c_str(), block, q.held, eh, wh, xh, ew, xw,
                                 q.dEng / q.dHeld, q.dWarm / q.dHeld, q.dHard / q.dHeld);
                ++a.n;
                {
                    const double ea = q.dEng / q.dHeld;
                    if (ea > a.worstAbs) { a.worstAbs = ea;  a.aname = r.name + " b" + std::to_string (block); }
                    a.worstAbsWarm = std::max (a.worstAbsWarm, q.dWarm / q.dHeld);
                    if (ea > 1.5) ++a.absOver;
                    if (block == 512) { a.minAbsHard = std::min (a.minAbsHard, q.dHard / q.dHeld);  if (q.dHard / q.dHeld < 10.0) ++a.absHardUnder; }
                }
                if (eh > 2.0) ++a.over2;
                if (wh > 2.0) ++a.warmOver2;
                if (ew > 2.0) ++a.over2warm;
                if (eh > a.worst) { a.worst = eh;  a.wname = r.name + " b" + std::to_string (block); }
                a.worstW = std::max (a.worstW, wh);
                if (ew > a.worstEW) { a.worstEW = ew;  a.ewname = r.name + " b" + std::to_string (block); }
                if (block == 512)
                {
                    if (xh < 10.0) ++a.hardUnder10;
                    if (xw < 10.0) ++a.hardUnder10w;
                    a.minHard = std::min (a.minHard, xh);  a.minHardW = std::min (a.minHardW, xw);
                }
            }
        }

        std::printf ("%-8s %5s | %10s %9s %10s | %10s %9s | %12s %12s | worst rows\n", "group", "n", "eng/held>2", "worst", "warm/held>2",
                     "eng/warm>2", "worst", "hard/held<10", "hard/warm<10");
        for (auto& g : groups)
        {
            const Agg& a = g.second;
            std::printf ("%-8s %5d | %10d %9.2f %10d | %10d %9.2f | %5d (min %5.1f) %5d (min %5.1f) | %s ; %s\n", g.first.c_str(), a.n, a.over2, a.worst, a.warmOver2,
                         a.over2warm, a.worstEW, a.hardUnder10, a.minHard, a.hardUnder10w, a.minHardW, a.wname.c_str(), a.ewname.c_str());
        }
        std::printf ("absolute second difference re the louder held render:\n");
        for (auto& g : groups)
        {
            const Agg& a = g.second;
            std::printf ("%-8s %5d | eng > 1.5: %3d, worst eng %6.3f (warm %6.3f) | hard < 10 at block 512: %3d, min hard %8.2f | %s\n", g.first.c_str(), a.n, a.absOver, a.worstAbs,
                         a.worstAbsWarm, a.absHardUnder, a.minAbsHard, a.aname.c_str());
        }
    }
    std::printf ("\nfades started %ld, latched %ld, sleeps %ld, wakes %ld\n", fadesStarted, latched, sleeps, wakes);
}

static void modeG3Var()
{
    const double fs = 48000.0;
    struct V { const char* name; double ms; int shape; };
    const V vs[] = { { "cos 20 ms", 20, shapeCos }, { "cos 10 ms", 10, shapeCos }, { "cos 5 ms", 5, shapeCos }, { "cos 40 ms", 40, shapeCos },
                     { "linear 20 ms", 20, shapeLinear }, { "equal-power 20 ms", 20, shapePower } };

    for (double tone : { 50.0, 1000.0 })
    {
        std::printf ("\n== tone %.0f Hz: per variant, rows with eng/held > 2 and the worst eng/held; the same against the 20 ms warm reference of each variant\n", tone);
        for (const auto& v : vs)
        {
            Opt opt;  opt.fadeMs = v.ms;  opt.shape = v.shape;
            std::printf ("%-18s |", v.name);
            for (const char* g : { "type", "slope", "place", "enable" })
            {
                const auto rows = stepRows (tone, g);
                int over = 0, n = 0;  double worst = 0, worstEW = 0;
                for (const auto& r : rows)
                    for (int block : { 32, 512 })
                    {
                        const size_t from = (size_t) (0.3 * fs);
                        std::vector<size_t> starts;
                        const Stereo yA = renderSteps (r, tone, block, opt, 1, fs);
                        const Stereo yB = renderSteps (r, tone, block, opt, 2, fs);
                        const Stereo yE = renderSteps (r, tone, block, opt, 0, fs, &starts);
                        Opt wopt = opt;  wopt.shape = shapeCos;
                        const Stereo yW = warmReference (yA, yB, starts, fs, wopt);
                        const double held = std::max (kappa2 (yA, fs, from), kappa2 (yB, fs, from));
                        const double eh = kappa2 (yE, fs, from) / held;
                        ++n;  if (eh > 2.0) ++over;
                        worst = std::max (worst, eh);
                        worstEW = std::max (worstEW, kappa2 (yE, fs, from) / kappa2 (yW, fs, from));
                    }
                std::printf (" %s %3d/%3d worst %7.2f e/w %6.2f |", g, over, n, worst, worstEW);
            }
            std::printf ("\n");
        }
    }
}

//==============================================================================
// ident: gain types at exactly 0 dB
//==============================================================================
static void modeIdent()
{
    setFlushToZero();
    for (double fs : { 22050.0, 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        Designer des;  des.prepare (fs);
        long designs = 0, exact = 0;
        double worstParam = 0.0, worstMag = 0.0, worstOut = 0.0, worstOutCold = 0.0;
        rngState = 4242 ^ (uint64_t) fs;

        for (int trial = 0; trial < 20000; ++trial)
        {
            BandSettings s;
            const Type gt[] = { Type::bell, Type::lowShelf, Type::highShelf, Type::tilt };
            s.type = gt[(int) (urand() * 4)];
            s.slopeIndex = usesSlope (s.type) ? (int) (urand() * 8) : 0;
            const double u = urand();
            s.freqHz = u < 0.05 ? 10.0 : (u > 0.95 ? 30000.0 : std::exp (std::log (10.0) + urand() * std::log (3000.0)));
            const double z = urand();
            s.q = z < 0.05 ? 0.025 : (z > 0.95 ? 40.0 : std::exp (std::log (0.025) + urand() * std::log (1600.0)));
            if (usesSlope (s.type) && s.slopeIndex == 0) s.q = 1.0;
            s.gainDbEff = 0.0;

            BandDesign d;
            des.design (s, d);
            ++designs;

            bool ex = true;
            for (int i = 0; i < d.numSections; ++i)
            {
                const auto& p = d.params[(size_t) i];
                if (p.hasSvf)
                {
                    const double dev = std::max ({ std::fabs (p.mL - 1.0), std::fabs (p.mB / p.k - 1.0), std::fabs (p.mH - 1.0) });
                    worstParam = std::max (worstParam, dev);
                    if (dev != 0.0) ex = false;
                }
                if (p.hasOnePole)
                {
                    const double dev = std::max (std::fabs (p.cL - 1.0), std::fabs (p.cH - 1.0));
                    worstParam = std::max (worstParam, dev);
                    if (dev != 0.0) ex = false;
                }
            }
            if (ex) ++exact;

            for (int i = 0; i < 24; ++i)
            {
                const double f = 10.0 * std::pow (0.499 * fs / 10.0, i / 23.0);
                worstMag = std::max (worstMag, std::fabs (bandMagDb (d, 2.0 * pi * f / fs)));
            }

            if (trial < 2000)
            {
                Band b;  b.setControlGrid (controlGridSize (fs), rampLengthTicks (fs));
                b.snapTo (s, des);  b.reset();
                double x[512], y[512];
                double peak = 0;
                for (int blk = 0; blk < 8; ++blk)
                {
                    for (int i = 0; i < 512; ++i) { x[i] = (urand() * 2.0 - 1.0) * 0.25;  y[i] = x[i]; }
                    b.processLane (0, y, 512);
                    for (int i = 0; i < 512; ++i) peak = std::max (peak, std::fabs (y[i] - x[i]));
                    if (blk == 0) worstOutCold = std::max (worstOutCold, peak);
                }
                worstOut = std::max (worstOut, peak);
            }
        }
        std::printf ("fs %6.0f: %ld designs at 0 dB (bell, shelves, tilt; all slopes; f and Q incl. range ends): mix exactly (1, k, 1) and (1, 1) in %ld; worst mix deviation %.2e; worst |H| %.2e dB; "
                     "noise at 0.25 peak through the band from rest, max |y - x| %.2e (first block %.2e) = %.1f dB re input peak\n",
                     fs, designs, exact, worstParam, worstMag, worstOut, worstOutCold, 20.0 * std::log10 (std::max (worstOut, 1e-300) / 0.25));
    }
}

//==============================================================================
// sleep: neutral sleep and wake
//==============================================================================
static void modeSleep()
{
    const double fs = 48000.0;
    struct Case { const char* name; Cfg c; };

    for (double tone : { 50.0, 1000.0 })
    {
        std::vector<std::pair<std::string, Cfg>> subjects;
        subjects.push_back ({ "Bell at tone Q 1", deep (Type::bell, tone) });
        { Cfg c = deep (Type::bell, tone);  c.s.q = 40.0;  subjects.push_back ({ "Bell at tone Q 40", c }); }
        { Cfg c = deep (Type::bell, tone);  c.s.freqHz = 10.0;  c.s.q = 40.0;  subjects.push_back ({ "Bell 10 Hz Q 40", c }); }
        subjects.push_back ({ "HighShelf 12", deep (Type::highShelf, tone) });
        { Cfg c = deep (Type::highShelf, tone);  c.s.slopeIndex = 7;  subjects.push_back ({ "HighShelf 96", c }); }
        { Cfg c = deep (Type::lowShelf, tone);  c.s.slopeIndex = 7;  c.s.q = 4.0;  subjects.push_back ({ "LowShelf 96 Q 4", c }); }
        subjects.push_back ({ "Tilt 12", deep (Type::tilt, tone) });

        std::printf ("\n== tone %.0f Hz. gain 0 <> G every 150 ms (the band sleeps 70 ms after each return to 0). kappa as a ratio to an engine that never sleeps\n", tone);
        std::printf ("%-22s %5s %5s | %9s | %10s %10s %10s | %9s\n", "band", "gain", "block", "no sleep", "sleep+fades", "no fades", "wake fade", "ns/held");

        for (auto& sub : subjects)
            for (double gain : { 18.0, -18.0 })
                for (int block : { 32, 512 })
                {
                    Row r;  r.name = sub.first;
                    r.b = sub.second;  r.b.s.gainDbEff = sub.second.s.type == Type::tilt ? 2.0 * gain : gain;
                    r.a = r.b;  r.a.s.gainDbEff = 0.0;
                    const size_t from = (size_t) (0.3 * fs);

                    Opt never;  never.sleep = false;
                    Opt full;
                    Opt none;  none.sleepFade = false;  none.wakeFade = false;
                    Opt wakeOnly;  wakeOnly.sleepFade = false;

                    const double kN = kappa2 (renderSteps (r, tone, block, never, 0, fs, nullptr, 0.150), fs, from);
                    const double kF = kappa2 (renderSteps (r, tone, block, full, 0, fs, nullptr, 0.150), fs, from);
                    const double k0 = kappa2 (renderSteps (r, tone, block, none, 0, fs, nullptr, 0.150), fs, from);
                    const double kW = kappa2 (renderSteps (r, tone, block, wakeOnly, 0, fs, nullptr, 0.150), fs, from);
                    const double held = std::max (kappa2 (renderSteps (r, tone, block, never, 1, fs), fs, from),
                                                  kappa2 (renderSteps (r, tone, block, never, 2, fs), fs, from));
                    std::printf ("%-22s %+5.0f %5d | %9.3e | %10.3f %10.3f %10.3f | %9.2f\n", sub.first.c_str(), gain, block, kN, kF / kN, k0 / kN, kW / kN, kN / held);
                }
    }

    // What the cold start costs: the output of a waking band against one that never slept, after the wake.
    std::printf ("\n== largest difference between a band that slept and one that never did, after the wake (re the never-slept output's peak in the same window)\n");
    for (double tone : { 50.0, 1000.0 })
    {
        struct S { const char* name; Cfg c; };
        Cfg a = deep (Type::bell, tone);  a.s.q = 40.0;
        Cfg b = deep (Type::bell, tone);  b.s.freqHz = 10.0;  b.s.q = 40.0;
        Cfg c = deep (Type::lowShelf, tone);  c.s.slopeIndex = 7;
        const S subjects[] = { { "Bell at tone Q 1", deep (Type::bell, tone) }, { "Bell at tone Q 40", a }, { "Bell 10 Hz Q 40", b }, { "LowShelf 96", c } };

        for (auto& sub : subjects)
        {
            Row r;  r.b = sub.c;  r.a = r.b;  r.a.s.gainDbEff = 0.0;
            Opt never;  never.sleep = false;
            Opt full;
            const Stereo yN = renderSteps (r, tone, 64, never, 0, fs, nullptr, 0.150);
            const Stereo yS = renderSteps (r, tone, 64, full, 0, fs, nullptr, 0.150);
            // the second wake is at lead + 2 * 0.150
            const size_t w0 = (size_t) ((leadSec + 0.300) * fs);
            double d20 = 0, d150 = 0, pk = 0;
            for (size_t i = w0; i < w0 + (size_t) (0.150 * fs); ++i)
            {
                const double d = std::fabs (yN.L[i] - yS.L[i]);
                if (i < w0 + (size_t) (0.020 * fs)) d20 = std::max (d20, d);
                d150 = std::max (d150, d);
                pk = std::max (pk, std::fabs (yN.L[i]));
            }
            std::printf ("tone %5.0f  %-20s first 20 ms %7.1f dB, whole 150 ms %7.1f dB\n", tone, sub.name, 20.0 * std::log10 (d20 / pk + 1e-300), 20.0 * std::log10 (d150 / pk + 1e-300));
        }
    }
}

//==============================================================================
// g4: time to bit transparency
//==============================================================================
static void modeG4()
{
    setFlushToZero();
    std::printf ("Sample time after the event from which the output equals the input bit for bit (double engine; noise at 0.25 peak; -1 = never within 400 ms)\n");
    std::printf ("%8s %6s | %12s %12s %12s %12s | %12s %12s\n", "fs", "block", "(d) on at 0", "(d) 6 -> 0", "(e) scale 0", "(e) no fade", "(f) bypass", "un-bypass");

    for (double fs : { 22050.0, 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int block : { 1, 16, 64, 512, 1024, 4096 })
        {
            auto run = [&] (int scenario) -> double
            {
                Opt opt;
                if (scenario == 3) opt.sleepFade = false;
                FEngine e;  e.opt = opt;  e.prepare (fs, 24);
                rngState = 99;
                const size_t pre = (size_t) (0.3 * fs) / (size_t) block * (size_t) block + (size_t) block;
                const size_t n = pre + (size_t) (0.45 * fs);
                std::vector<double> L (n), R (n), xL (n), xR (n);
                for (size_t i = 0; i < n; ++i) { xL[i] = L[i] = (urand() * 2.0 - 1.0) * 0.25;  xR[i] = R[i] = (urand() * 2.0 - 1.0) * 0.25; }

                for (size_t pos = 0; pos < n; pos += (size_t) block)
                {
                    const bool after = pos >= pre;
                    for (int b = 0; b < 24; ++b)
                    {
                        Cfg c;
                        const Type gt[] = { Type::bell, Type::lowShelf, Type::highShelf, Type::tilt };
                        c.s.type = (scenario == 0 || scenario == 1) ? Type::bell : gt[b % 4];
                        c.s.slopeIndex = usesSlope (c.s.type) ? b % 8 : 0;
                        c.s.freqHz = 40.0 * std::pow (1.3, b);
                        c.s.q = 0.5 + 0.2 * b;
                        const double g = (b & 1) ? 12.0 : -12.0;
                        switch (scenario)
                        {
                            case 0:  c.s.gainDbEff = 0.0;  c.active = after;  break;                 // enabled at 0 dB
                            case 1:  c.s.gainDbEff = after ? 0.0 : 6.0;  break;                      // gain moved to 0
                            case 2:
                            case 3:  c.s.gainDbEff = after ? 0.0 : g;  break;                        // gain scale 0 %
                            case 4:  c.s.gainDbEff = g;  break;                                      // bypass
                            default: c.s.gainDbEff = g;  break;
                        }
                        e.setInput (b, c.s, c.p, c.active);
                    }
                    if (scenario == 4) e.setBypass (after);
                    if (scenario == 5) e.setBypass (! after);
                    const int m = (int) std::min<size_t> ((size_t) block, n - pos);
                    e.process (L.data() + pos, R.data() + pos, m);
                }

                if (scenario == 5)
                {
                    // un-bypass: time until every band runs (no fade in flight)
                    return 0.0;
                }
                // last differing sample
                size_t last = 0;  bool any = false;
                for (size_t i = pre; i < n; ++i)
                    if (std::memcmp (&L[i], &xL[i], sizeof (double)) != 0 || std::memcmp (&R[i], &xR[i], sizeof (double)) != 0) { last = i;  any = true; }
                if (! any) return 0.0;
                if (last >= n - 2) return -1.0;
                return 1000.0 * (double) (last + 1 - pre) / fs;
            };

            std::printf ("%8.0f %6d | %9.2f ms %9.2f ms %9.2f ms %9.2f ms | %9.2f ms\n", fs, block, run (0), run (1), run (2), run (3), run (4));
        }
}

//==============================================================================
// mix: structural changes that meet other motion
//==============================================================================
static void modeMix()
{
    const double fs = 48000.0;
    const size_t from = (size_t) (0.3 * fs);

    for (double tone : { 50.0, 1000.0 })
    {
        std::printf ("\n== tone %.0f Hz\n", tone);

        // 1. A structural change every block: the latch turns it into back-to-back 20 ms fades.
        for (int block : { 32, 512 })
        {
            auto render = [&] (bool fades, int motion)
            {
                Opt opt;  opt.fades = fades;
                FEngine e;  e.opt = opt;  e.prepare (fs, 1);
                const size_t n = (size_t) (1.6 * fs);
                Stereo y = stimulus (tone, fs, n);
                long k = 0;
                for (size_t pos = 0; pos < n; pos += (size_t) block, ++k)
                {
                    const double t = (double) pos / fs;
                    Cfg a = deep (Type::bell, tone), b = deep (Type::highShelf, tone);
                    const bool useB = motion == 2 || (motion == 0 && t > 0.5 && t < 1.2 && (k & 1));
                    const Cfg& c = useB ? b : a;
                    e.setInput (0, c.s, c.p, c.active);
                    e.process (y.L.data() + pos, y.R.data() + pos, (int) std::min<size_t> ((size_t) block, n - pos));
                }
                return y;
            };
            const Stereo hA = render (true, 1), hB = render (true, 2);
            const double held = std::max (kappa2 (hA, fs, from), kappa2 (hB, fs, from)), dHeld = std::max (maxD2 (hA, from), maxD2 (hB, from));
            const long f0 = fadesStarted;
            const Stereo yE = render (true, 0);
            const long nf = fadesStarted - f0;
            const Stereo yX = render (false, 0);
            std::printf ("%-48s block %4d: kappa eng/held %6.2f, hard/held %9.2f | abs d2 eng %6.3f, hard %9.2f  (fades started %ld)\n", "type toggled at every block for 0.7 s", block,
                         kappa2 (yE, fs, from) / held, kappa2 (yX, fs, from) / held, maxD2 (yE, from) / dHeld, maxD2 (yX, from) / dHeld, nf);
        }

        // 2. Type change and a continuous change in the same block (a preset load), and 5 ms into a ramp.
        struct M { const char* name; int kind; };
        for (const M& mm : { M { "type + freq + gain in one block (preset load)", 0 }, M { "type change 5 ms into a gain ramp", 1 },
                             M { "gain change 5 ms into a type fade", 2 }, M { "enable + gain 0 -> +18 in one block", 3 },
                             M { "type change 5 ms into a type fade (latched)", 4 }, M { "bypass 5 ms into a type fade", 5 },
                             M { "un-bypass 5 ms into the bypass fade", 6 }, M { "the same, the fade reverses instead of latching", 7 },
                             M { "bypass on for one block only", 8 }, M { "gain 0 -> +18 and back to 0 within 5 ms (wake)", 9 } })
            for (int block : { 32, 512 })
            {
                auto render = [&] (bool fades, int motion)
                {
                    // motion 3: the same continuous moves with the structural part taken out (kinds 1, 2), or with the sleep off (kind 9)
                    const bool rampOnly = motion == 3;
                    if (rampOnly) motion = 0;
                    Opt opt;  opt.fades = fades;  opt.bypassFade = fades;  opt.bypassLatch = mm.kind != 7;
                    if (rampOnly) opt.sleep = false;
                    FEngine e;  e.opt = opt;  e.prepare (fs, 1);
                    const size_t n = (size_t) (1.5 * fs);
                    bool firstOfEvent = false;  int lastK = -1;
                    Stereo y = stimulus (tone, fs, n);
                    for (size_t pos = 0; pos < n; pos += (size_t) block)
                    {
                        const double t = (double) pos / fs;
                        // events every 200 ms from 0.5 s: state toggles A <-> B; a second event 5 ms later for kinds 1, 2, 4, 5, 6
                        const int k = t < 0.5 ? -1 : (int) ((t - 0.5) / 0.2);
                        const double into = t < 0.5 ? 0.0 : (t - 0.5) - 0.2 * k;
                        const bool odd = motion == 2 || (motion == 0 && k >= 0 && k < 4 && ! (k & 1));
                        const bool late = into >= 0.005;
                        firstOfEvent = k != lastK;  lastK = k;
                        Cfg a = deep (Type::bell, tone), b = deep (Type::highShelf, tone), c;
                        bool byp = false;

                        switch (mm.kind)
                        {
                            case 0:  b.s.freqHz = 4.0 * tone;  b.s.gainDbEff = -12.0;  c = odd ? b : a;  break;
                            case 1:  c = odd ? b : a;  if (motion == 0 && k >= 0 && k < 4 && ! late) { c = odd ? a : b;  c.s.gainDbEff = 6.0; }  break;
                            case 2:  c = odd ? b : a;  if (motion == 0 && k >= 0 && k < 4 && late) c.s.gainDbEff = 6.0;  if (motion != 0) c.s.gainDbEff = 6.0;  break;
                            case 3:  a.active = false;  a.s.gainDbEff = 0.0;  c = odd ? deep (Type::bell, tone) : a;  break;
                            case 4:  c = odd ? b : a;  if (motion == 0 && k >= 0 && k < 4 && ! late) c = deep (Type::lowCut, tone);  break;
                            case 5:  c = odd ? b : a;  byp = motion == 0 && k >= 0 && k < 4 && late && odd;  break;
                            case 6:
                            case 7:  c = a;  byp = motion == 0 && k >= 0 && k < 4 && odd && ! late;  if (motion == 2) byp = true;  break;
                            case 8:  c = a;  byp = motion == 0 && k >= 0 && k < 4 && firstOfEvent;  if (motion == 2) byp = true;  break;
                            case 9:  c = a;  c.s.gainDbEff = (motion == 0 && k >= 0 && k < 4 && ! late) || motion == 2 ? 18.0 : 0.0;  break;
                        }
                        if (rampOnly && (mm.kind == 1 || mm.kind == 2))
                        {
                            const double g = c.s.gainDbEff;
                            c = a;  c.s.gainDbEff = g;
                        }
                        e.setBypass (byp);
                        e.setInput (0, c.s, c.p, c.active);
                        e.process (y.L.data() + pos, y.R.data() + pos, (int) std::min<size_t> ((size_t) block, n - pos));
                    }
                    return y;
                };
                const Stereo hA = render (true, 1), hB = render (true, 2), yE = render (true, 0), yX = render (false, 0);
                const double held = std::max (kappa2 (hA, fs, from), kappa2 (hB, fs, from)), dHeld = std::max (maxD2 (hA, from), maxD2 (hB, from));
                std::printf ("%-48s block %4d: kappa eng/held %6.2f, hard/held %9.2f | abs d2 eng %6.3f, hard %9.2f", mm.name, block,
                             kappa2 (yE, fs, from) / held, kappa2 (yX, fs, from) / held, maxD2 (yE, from) / dHeld, maxD2 (yX, from) / dHeld);
                if (mm.kind == 1 || mm.kind == 2 || mm.kind == 9)
                {
                    const Stereo yR = render (true, 3);
                    std::printf (" | the moves alone: kappa %6.2f, abs d2 %6.3f", kappa2 (yR, fs, from) / held, maxD2 (yR, from) / dHeld);
                }
                std::printf ("\n");
            }
    }
}

//==============================================================================
// blk: structural events on multiples of 2048
//==============================================================================
static void modeBlk()
{
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        std::vector<std::vector<double>> outs;
        const int sizes[] = { 32, 64, 512, 2048, 1, 7, 4096 };
        for (int bs : sizes)
        {
            rngState = 12345;
            FEngine e;  e.prepare (fs, 6);
            const size_t n = 2048 * 160;
            std::vector<double> L (n), R (n);
            for (size_t i = 0; i < n; ++i) { L[i] = (urand() - 0.5) * 0.5;  R[i] = (urand() - 0.5) * 0.5; }
            uint64_t evRng = 777;
            auto ev = [&] { evRng ^= evRng << 13; evRng ^= evRng >> 7; evRng ^= evRng << 17; return (double) (evRng >> 11) / 9007199254740992.0; };
            Cfg cur[6];
            bool byp = false;
            size_t nextEvent = 0;
            for (size_t pos = 0; pos < n; pos += (size_t) bs)
            {
                while (nextEvent <= pos)
                {
                    for (auto& c : cur)
                    {
                        if (ev() < 0.5) { c.s.type = (Type) (int) (ev() * 8);  c.s.slopeIndex = (int) (ev() * 8); }
                        if (ev() < 0.3) c.p = (Placement) (int) (ev() * 5);
                        if (ev() < 0.3) c.active = ev() > 0.3;
                        c.s.freqHz = std::exp (std::log (10.0) + ev() * std::log (3000.0));
                        c.s.gainDbEff = ev() < 0.25 ? 0.0 : -30.0 + 60.0 * ev();
                        c.s.q = std::exp (std::log (0.025) + ev() * std::log (1600.0));
                    }
                    if (ev() < 0.1) byp = ! byp;
                    nextEvent += 2048;
                }
                e.setBypass (byp);
                for (int b = 0; b < 6; ++b) e.setInput (b, cur[b].s, cur[b].p, cur[b].active);
                const int m = (int) std::min<size_t> ((size_t) bs, n - pos);
                e.process (L.data() + pos, R.data() + pos, m);
            }
            L.insert (L.end(), R.begin(), R.end());
            outs.push_back (L);
        }
        std::printf ("fs %6.0f, structural + continuous + bypass events on multiples of 2048:", fs);
        for (size_t k = 1; k < outs.size(); ++k)
            std::printf (" block %d %s |", sizes[k], std::memcmp (outs[0].data(), outs[k].data(), outs[0].size() * sizeof (double)) == 0 ? "identical" : "DIFFERS");
        std::printf ("  (against block 32)\n");
    }
    std::printf ("fades started %ld, latched %ld, sleeps %ld, wakes %ld\n", fadesStarted, latched, sleeps, wakes);
}

//==============================================================================
// g5: random stress with structural parameters
//==============================================================================
static double bandPeakDb (const Designer& des, const BandSettings& s, double fs)
{
    BandDesign d;
    des.design (s, d);
    double worst = 0.0;
    const double top = 0.4999 * fs;
    for (int i = 0; i < 48; ++i)
    {
        const double f = 5.0 * std::pow (top / 5.0, i / 47.0);
        worst = std::max (worst, bandMagDb (d, 2.0 * pi * f / fs));
    }
    worst = std::max (worst, bandMagDb (d, 2.0 * pi * std::min (s.freqHz, top) / fs));
    return worst;
}

static void modeG5 (double seconds, int structEvery)
{
    setFlushToZero();
    std::printf ("G5-style stress with structure, double engine, 24 bands stereo, %.0f s per rate; block sizes 4096, 512, 64, 7, 1 cycling every 2 s\n", seconds);
    std::printf ("frequency, gain, Q and gain scale redrawn every block; per band, type / slope / placement / enable / band bypass redrawn with probability 1/%d per block; "
                 "global bypass toggled with probability 1/200 per block\n", structEvery);
    std::printf ("%8s | %9s %13s %15s %15s | %9s %9s %8s %8s | %10s | %s\n", "fs", "nonfinite", "peak dB re in", "peak-run stack", "peak-2 s stack",
                 "fades/s", "latched/s", "sleeps", "wakes", "x realtime", "all off -> bit-equal after");

    const int blocks[] = { 4096, 512, 64, 7, 1 };

    for (double fs : { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
    {
        rngState = 0x9E3779B97F4A7C15ull ^ (uint64_t) fs;
        FEngine e;  e.prepare (fs, 24);
        fadesStarted = latched = sleeps = wakes = 0;

        std::vector<Cfg> cur (24);
        std::vector<double> rawGain (24, 0.0);
        bool byp = false;
        double runStack = 0.0;
        std::deque<std::pair<size_t, double>> window;       // (sample position, stack) over the trailing 2 s

        const size_t total = (size_t) (seconds * fs);
        std::vector<double> L (4096), R (4096);
        long nonFinite = 0;
        double peak = 0.0, inPeak = 0.0, worstRun = -1e9, worstWin = -1e9;
        size_t pos = 0;
        const auto t0 = Clock::now();

        while (pos < total)
        {
            const int bs = blocks[(size_t) ((double) pos / fs / 2.0) % 5];
            const int m = (int) std::min<size_t> ((size_t) bs, total - pos);
            const bool silent = std::fmod ((double) pos / fs, 5.0) > 4.0;

            const double su = urand();
            const double scale = su < 0.1 ? 0.0 : (su > 0.9 ? 200.0 : 200.0 * urand());
            if (urand() < 1.0 / 200.0) byp = ! byp;

            const bool evalStack = bs >= 512;
            double stackNow = 0.0;

            for (int b = 0; b < 24; ++b)
            {
                Cfg& c = cur[(size_t) b];
                if (urand() < 1.0 / structEvery)
                {
                    c.s.type = (Type) (int) (urand() * 8);
                    c.s.slopeIndex = (int) (urand() * 8);
                    c.p = (Placement) (int) (urand() * 5);
                    c.active = urand() > 0.2;
                }
                const double u = urand();
                c.s.freqHz = u < 0.1 ? 10.0 : (u > 0.9 ? 30000.0 : std::exp (std::log (10.0) + urand() * std::log (3000.0)));
                const double w = urand();
                rawGain[(size_t) b] = w < 0.1 ? -30.0 : (w > 0.9 ? 30.0 : -30.0 + 60.0 * urand());
                const double z = urand();
                c.s.q = z < 0.1 ? 0.025 : (z > 0.9 ? 40.0 : std::exp (std::log (0.025) + urand() * std::log (1600.0)));
                c.s.gainDbEff = std::max (-60.0, std::min (60.0, rawGain[(size_t) b] * scale / 100.0));
                e.setInput (b, c.s, c.p, c.active);

                if (evalStack && c.active)
                {
                    BandSettings s = c.s;
                    if (! usesSlope (s.type)) s.slopeIndex = 0;
                    if (! isGainType (s.type)) s.gainDbEff = 0.0;
                    stackNow += bandPeakDb (e.des, s, fs);
                }
            }
            e.setBypass (byp);

            if (evalStack)
            {
                runStack = std::max (runStack, stackNow);
                window.push_back ({ pos, stackNow });
            }
            while (! window.empty() && window.front().first + (size_t) (2.0 * fs) < pos) window.pop_front();
            double winStack = 0.0;
            for (auto& w : window) winStack = std::max (winStack, w.second);

            for (int i = 0; i < m; ++i)
            {
                L[(size_t) i] = silent ? 0.0 : (urand() * 2.0 - 1.0) * 0.25;
                R[(size_t) i] = silent ? 0.0 : (urand() * 2.0 - 1.0) * 0.25;
                inPeak = std::max ({ inPeak, std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i]) });
            }

            e.process (L.data(), R.data(), m);

            double blockPeak = 0.0;
            for (int i = 0; i < m; ++i)
            {
                const double a = std::fabs (L[(size_t) i]), c = std::fabs (R[(size_t) i]);
                if (! std::isfinite (a) || ! std::isfinite (c)) { ++nonFinite; continue; }
                blockPeak = std::max ({ blockPeak, a, c });
            }
            peak = std::max (peak, blockPeak);
            if (blockPeak > 0.0 && pos > (size_t) fs)
            {
                const double db = 20.0 * std::log10 (blockPeak / 0.25);
                worstRun = std::max (worstRun, db - runStack);
                if (bs >= 512 && window.size() > 4) worstWin = std::max (worstWin, db - winStack);
            }
            pos += (size_t) m;
        }
        const double wall = secs (t0, Clock::now());
        const long nf = fadesStarted, nl = latched, ns = sleeps, nw = wakes;

        // Afterwards: all bands off, bypass off. From which sample is the output the input, bit for bit?
        e.setBypass (false);
        const size_t tail = (size_t) (0.3 * fs);
        std::vector<double> tl (tail), tr (tail), xl (tail), xr (tail);
        for (size_t i = 0; i < tail; ++i) { xl[i] = tl[i] = (urand() * 2.0 - 1.0) * 0.25;  xr[i] = tr[i] = (urand() * 2.0 - 1.0) * 0.25; }
        for (size_t p = 0; p < tail; p += 64)
        {
            for (int b = 0; b < 24; ++b) e.setInput (b, cur[(size_t) b].s, cur[(size_t) b].p, false);
            e.process (tl.data() + p, tr.data() + p, (int) std::min<size_t> (64, tail - p));
        }
        size_t last = 0;  bool any = false;
        for (size_t i = 0; i < tail; ++i)
            if (std::memcmp (&tl[i], &xl[i], 8) != 0 || std::memcmp (&tr[i], &xr[i], 8) != 0) { last = i;  any = true; }

        std::printf ("%8.0f | %9ld %13.1f %15.1f %15.1f | %9.1f %9.1f %8ld %8ld | %10.1f | %.2f ms\n", fs, nonFinite, 20.0 * std::log10 (peak / inPeak),
                     worstRun, worstWin, (double) nf / seconds, (double) nl / seconds, ns, nw, seconds / wall, any ? 1000.0 * (double) (last + 1) / fs : 0.0);
    }
}

//==============================================================================
// cpu
//==============================================================================
static void modeCpu()
{
    setFlushToZero();
    const double fs = 48000.0;
    std::printf ("48 kHz stereo, 512-sample blocks, 20 s of noise, 24 bands. Share of one core.\n");
    std::printf ("%-28s | %8s | %14s %14s | %12s\n", "case", "settled", "always fading", "fade every 1 s", "24 asleep");

    struct C { const char* name; Type t; int slope; Type t2; int slope2; };
    const C cases[] = { { "24 cuts x 96 dB/oct", Type::lowCut, 7, Type::highCut, 7 }, { "24 shelves x 96 dB/oct", Type::highShelf, 7, Type::lowShelf, 7 },
                        { "24 bells <> shelves 12", Type::bell, 0, Type::highShelf, 1 }, { "24 shelves 96, slope 96 <> 72", Type::highShelf, 7, Type::highShelf, 6 } };

    for (const auto& c : cases)
    {
        auto run = [&] (int mode) -> double
        {
            FEngine e;  e.prepare (fs, 24);
            rngState = 5;
            std::vector<double> L (512), R (512);
            const size_t total = (size_t) (20.0 * fs);
            double noiseOnly = 0;
            {
                const auto t0 = Clock::now();
                for (size_t pos = 0; pos < total; pos += 512)
                    for (int i = 0; i < 512; ++i) { L[(size_t) i] = (urand() - 0.5) * 0.5;  R[(size_t) i] = (urand() - 0.5) * 0.5; }
                noiseOnly = secs (t0, Clock::now());
            }
            const auto t0 = Clock::now();
            for (size_t pos = 0; pos < total; pos += 512)
            {
                const double t = (double) pos / fs;
                bool second = false;
                if (mode == 1) second = ((long) (t / 0.0215)) & 1;       // a new request as soon as each fade has ended
                if (mode == 2) second = ((long) t) & 1;
                for (int b = 0; b < 24; ++b)
                {
                    BandSettings s { second ? c.t2 : c.t, 40.0 * std::pow (1.28, b), mode == 3 ? 0.0 : 6.0, 1.0, second ? c.slope2 : c.slope };
                    e.setInput (b, s);
                }
                for (int i = 0; i < 512; ++i) { L[(size_t) i] = (urand() - 0.5) * 0.5;  R[(size_t) i] = (urand() - 0.5) * 0.5; }
                e.process (L.data(), R.data(), 512);
            }
            return 100.0 * (secs (t0, Clock::now()) - noiseOnly) / 20.0;
        };
        std::printf ("%-28s | %7.2f%% | %13.2f%% %13.2f%% | %11.3f%%\n", c.name, run (0), run (1), run (2), run (3));
    }
}

//==============================================================================
// dip: what the incoming slot's start from rest costs. Lowest 5 ms frame level of the left channel
// in the 100 ms after a step, re the quieter of the two held levels; engine and warm reference.
//==============================================================================
static double lowestFrameDb (const Stereo& y, double fs, const std::vector<size_t>& starts, double floorRms, double* msBelow3)
{
    const size_t frame = (size_t) std::lround (0.005 * fs);
    double lowest = 1e300;  size_t below = 0;
    for (size_t st : starts)
        for (size_t a = st; a + frame <= st + (size_t) (0.100 * fs) && a + frame < y.L.size(); a += frame / 5)
        {
            double e = 0;
            for (size_t i = a; i < a + frame; ++i) e += y.L[i] * y.L[i];
            const double r = std::sqrt (e / (double) frame);
            lowest = std::min (lowest, r);
            if (r < floorRms * 0.7079) ++below;
        }
    if (msBelow3) *msBelow3 = 1000.0 * (double) below * (double) (frame / 5) / fs / (double) std::max<size_t> (1, starts.size());
    return 20.0 * std::log10 (std::max (lowest, 1e-300) / floorRms);
}

static void modeDip (double prerollMs)
{
    const double fs = 48000.0;
    Opt opt;  opt.prerollMs = prerollMs;
    std::printf ("pre-roll %.0f ms\n", prerollMs);
    for (double tone : { 50.0, 1000.0 })
    {
        std::printf ("\n== tone %.0f Hz: lowest 5 ms level after a step re the quieter held level (dB), and the time spent more than 3 dB below it per step (ms)\n", tone);
        std::printf ("%-44s | %9s %9s | %9s %9s\n", "row", "engine", "ms < -3", "warm", "ms < -3");
        for (const char* g : { "enable", "slope", "type", "place" })
        {
            struct Worst { double db = 0, ms = 0, wdb = 0, wms = 0;  std::string name; };
            std::vector<Worst> all;
            for (const auto& r : stepRows (tone, g))
            {
                std::vector<size_t> starts;
                const Stereo yA = renderSteps (r, tone, 64, opt, 1, fs);
                const Stereo yB = renderSteps (r, tone, 64, opt, 2, fs);
                const Stereo yE = renderSteps (r, tone, 64, opt, 0, fs, &starts);
                const Stereo yW = warmReference (yA, yB, starts, fs, opt);
                auto rmsOf = [&] (const Stereo& y) { double e = 0; size_t n = 0; for (size_t i = (size_t) (0.3 * fs); i < (size_t) (0.5 * fs); ++i, ++n) e += y.L[i] * y.L[i];  return std::sqrt (e / (double) n); };
                const double floorRms = std::min (rmsOf (yA), rmsOf (yB));
                if (floorRms < 1e-4) continue;
                Worst w;  w.name = r.name;
                w.db = lowestFrameDb (yE, fs, starts, floorRms, &w.ms);
                w.wdb = lowestFrameDb (yW, fs, starts, floorRms, &w.wms);
                all.push_back (w);
            }
            if (std::getenv ("DIP_ALL") != nullptr)
                for (auto& w : all)
                    std::printf ("%-44s | %9.1f %9.1f | %9.1f %9.1f\n", w.name.c_str(), w.db, w.ms, w.wdb, w.wms);
            std::sort (all.begin(), all.end(), [] (const Worst& a, const Worst& b) { return a.db < b.db; });
            int deeper6 = 0, warmDeeper6 = 0;
            for (auto& w : all) { if (w.db < -6.0) ++deeper6;  if (w.wdb < -6.0) ++warmDeeper6; }
            std::printf ("-- %s: %zu rows; engine dips more than 6 dB in %d, the warm reference in %d. Deepest four:\n", g, all.size(), deeper6, warmDeeper6);
            for (size_t i = 0; i < std::min<size_t> (4, all.size()); ++i)
                std::printf ("%-44s | %9.1f %9.1f | %9.1f %9.1f\n", all[i].name.c_str(), all[i].db, all[i].ms, all[i].wdb, all[i].wms);
        }
    }
}

//==============================================================================
int main (int argc, char** argv)
{
    const std::string mode = argc > 1 ? argv[1] : "";

    if (mode == "ident")        modeIdent();
    else if (mode == "g3")      modeG3 (argc > 2 ? argv[2] : "all", argc > 3);
    else if (mode == "g3var")   modeG3Var();
    else if (mode == "sleep")   modeSleep();
    else if (mode == "g4")      modeG4();
    else if (mode == "mix")     modeMix();
    else if (mode == "blk")     modeBlk();
    else if (mode == "g5")      modeG5 (argc > 2 ? std::atof (argv[2]) : 20.0, argc > 3 ? std::atoi (argv[3]) : 50);
    else if (mode == "cpu")     modeCpu();
    else if (mode == "dip")     modeDip (argc > 2 ? std::atof (argv[2]) : 0.0);
    else
    {
        std::printf ("modes: ident g3 [group [v]] g3var sleep g4 mix blk g5 [seconds [structEvery]] cpu dip\n");
        return 1;
    }
    return 0;
}
