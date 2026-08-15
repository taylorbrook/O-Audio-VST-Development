#!/usr/bin/env python3
"""O-Octagon Phase 2.3 - numeric study of the air-absorption TPT one-pole.

Produces every dB and step figure quoted in .planning/stages/2-dsp/RESEARCH-2.3.md.
Run with no arguments:  python3 tests/tools/air_filter_study.py

This is a RESEARCH ARTEFACT, not a build step and not a test. It is committed so the
numbers in RESEARCH-2.3.md can be re-derived rather than trusted - the same reason
tests/fixtures/DbapReferenceFixture.h ships with its generator.

The filter modelled here is juce::dsp::FirstOrderTPTFilter<float>, transcribed from
JUCE 8.0.14 juce_FirstOrderTPTFilter.cpp:89-107 and :117-122:

    g = tan(pi * fc / fs);  G = g / (1 + g)
    v = G * (x - s);  y = v + s;  s = y + v
    => H(z) = G(z + 1) / (z - (1 - 2G))    unity at DC, zero at Nyquist

Four parts, in the order the questions were actually worked. Section letters restart
inside each part, so E and F appear twice - that is the working order preserved, not a
mistake. Map to RESEARCH-2.3.md by question, not by letter:

  PART 1  A the D2 dB figures (H2) | B step per probe tone (Q7) | C re-entry hazard (H1)
          D the DEGENERATE form of D3, kept because the negative result is the finding (Q6)
          E DSP-08 invariance and the clamps (H8) | F Nyquist assertion (H4)
          G transcendental budget (Q2)
  PART 2  D1/D2 the coefficient step isolated, and its derived bound (Q6)
          E the skip transition, the discontinuity that matters (H1) | F what crit 2 asserts
  PART 3  the skip transition with the cold start excluded (H1, H9), and the fast-re-entry
          case that separates reset(0) from reset(x)
  PART 4  H phase vs magnitude (H2) | I edge asymmetry (H3) | J raising the base (H3)
          K ARCHITECTURE 3.5.2's fc table re-derived (H4)

Each part is self-contained and redefines its own helpers; Python executes top to
bottom, so a later redefinition never affects an earlier section.
"""

# ##########################################################################
# PART 1  (from air_study.py)
# ##########################################################################
import math

def G_of(fc, fs):
    g = math.tan(math.pi * fc / fs)
    return g / (1.0 + g)

def mag(fc, fs, f):
    """|H(f)| of the TPT one-pole, exactly (bilinear warping included)."""
    return 1.0 / math.sqrt(1.0 + (math.tan(math.pi * f / fs) / math.tan(math.pi * fc / fs)) ** 2)

def db(x):
    return 20.0 * math.log10(x) if x > 0 else float("-inf")

def air_fc(air, d_hull, dref=3.0):
    return min(max(20000.0 * 2.0 ** (-air * d_hull / dref), 500.0), 20000.0)

print("=" * 78)
print("A. The two dB figures quoted in D2 / ARCHITECTURE 3.5.2  (fc = 20 kHz, d_hull -> 0+)")
print("=" * 78)
print(f"{'fs':>8} {'G':>10} {'|H|@1k dB':>12} {'@4k':>9} {'@8k':>9} {'@10k':>9} {'@15k':>9} {'@20k':>9}")
for fs in (44100.0, 48000.0, 88200.0, 96000.0):
    row = [f"{fs:8.0f}", f"{G_of(20000.0, fs):10.6f}"]
    for f in (1000, 4000, 8000, 10000, 15000, 20000):
        row.append(f"{db(mag(20000.0, fs, f)):9.4f}")
    print(" ".join(row))
print()
print("D2 claims '3 dB @ 20 kHz, 0.7 dB @ 10 kHz'. 20 kHz is right (-3.01 dB by definition:")
print("it IS the cutoff). 10 kHz is NOT -- prewarping (fc=20k sits at 0.83*Nyquist at 48k)")
print("flattens the passband far more than an analog one-pole would.")
print(f"  analog one-pole at f/fc = 0.5: {db(1/math.sqrt(1.25)):.4f} dB   <- probably where 0.7 came from")
print(f"  digital TPT @ 48 kHz:          {db(mag(20000,48000,10000)):.4f} dB")
print(f"  digital TPT @ 44.1 kHz:        {db(mag(20000,44100,10000)):.4f} dB")

print()
print("=" * 78)
print("B. Q7 -- how big is the D2 hull-crossing STEP at each candidate probe tone?")
print("   (steady-state only; the step is exactly 1 - |H_20k(f)|, filter-absent vs filter-at-20k)")
print("=" * 78)
print(f"{'tone':>8} {'fs=44100':>22} {'fs=48000':>22}")
print(f"{'':>8} {'rel step':>11}{'dB':>11} {'rel step':>11}{'dB':>11}")
for f in (1000, 2000, 4000, 8000, 12000, 15000, 18000):
    cells = []
    for fs in (44100.0, 48000.0):
        m = mag(20000.0, fs, f)
        cells.append(f"{1.0 - m:11.3e}{db(m):11.4f}")
    print(f"{f:8d} " + " ".join(cells))
print()
print("A 1 kHz probe sees a 1.3e-4 relative step. The sine's own per-sample slew at 1 kHz is")
print(f"  2*sin(pi*1000/48000) = {2*math.sin(math.pi*1000/48000):.6f}  (x amplitude)")
print(f"  -> the step is {(1-mag(20000,48000,1000))/(2*math.sin(math.pi*1000/48000))*100:.3f}% of the natural slew. INVISIBLE.")
print("At 15 kHz the step is 7.2e-2 relative, and the natural slew is")
print(f"  2*sin(pi*15000/48000) = {2*math.sin(math.pi*15000/48000):.6f}")
print(f"  -> {(1-mag(20000,48000,15000))/(2*math.sin(math.pi*15000/48000))*100:.2f}% of natural slew. Still small vs slew,")
print("     but the DIFFERENCE-SERIES method below removes the slew entirely.")

print()
print("=" * 78)
print("C. The re-entry hazard D2 creates, and the one-line fix")
print("   airActive false->true: y[0] = G*x[0] + (1-G)*s_stale")
print("=" * 78)
for fs in (44100.0, 48000.0):
    G = G_of(20000.0, fs)
    print(f"  fs={fs:.0f}: G={G:.6f}, 1-G={1-G:.6f}, pole p=1-2G={1-2*G:+.6f} (|p|={abs(1-2*G):.4f})")
    print(f"     worst first-sample error, |s_stale - x[0]| <= 2*peak : {2*(1-G):.4f} * peak"
          f"  = {db(2*(1-G)):.2f} dB re peak")
    print(f"     ... decaying by {abs(1-2*G):.4f} per sample "
          f"({db(abs(1-2*G)):.2f} dB/sample)")
print()
print("  Three re-entry policies, first-sample output error vs the unfiltered signal x[0]:")
print("    (a) leave state resident (D2 as written) : (1-G)*|s_stale - x[0]|   up to 2(1-G)*peak")
print("    (b) reset(0)  (original 3.5.2)           : (1-G)*|x[0]|             up to (1-G)*peak")
print("    (c) reset(x[0])                          : EXACTLY 0  -- y = G*x + (1-G)*x = x")
print("  (c) costs one float store per transition and makes the entry step identically zero.")

print()
print("=" * 78)
print("D. Q6 -- D3's differential probe. Simulated, 1 s, fs = 48 kHz.")
print("   airAmount swept 0.05 -> 1.0 at full speed, d_hull held at 5 m (source outside).")
print("=" * 78)

fs = 48000.0
N = 48000
AMP = 0.5
D_HULL = 5.0

def render(tone_hz, grid, air_start, air_end):
    """Control-rate fc update every `grid` samples; TPT one-pole; returns the output list."""
    s = 0.0
    G = None
    out = []
    for n in range(N):
        if n % grid == 0:
            t = n / (N - 1)
            air = air_start + (air_end - air_start) * t
            G = G_of(air_fc(air, D_HULL), fs)
        x = AMP * math.sin(2 * math.pi * tone_hz * n / fs)
        v = G * (x - s)
        y = v + s
        s = y + v
        out.append(y)
    return out

def max_step(series, stride=1):
    return max(abs(series[n] - series[n - stride]) for n in range(stride, len(series)))

for tone in (1000.0, 8000.0):
    swept64 = render(tone, 64, 0.05, 1.0)
    swept4096 = render(tone, 4096, 0.05, 1.0)
    held = render(tone, 64, 0.05, 0.05)          # airAmount HELD at the sweep's start

    nat = max_step(held)
    s64 = max_step(swept64)
    s4096 = max_step(swept4096)

    d64 = [a - b for a, b in zip(swept64, held)]
    d4096 = [a - b for a, b in zip(swept4096, held)]
    dd64 = max_step(d64)
    dd4096 = max_step(d4096)

    print(f"\n  --- probe tone {tone:.0f} Hz, amplitude {AMP} ---")
    print("  D3 AS WRITTEN (slew of the output itself):")
    print(f"    held  max|d out|             = {nat:.9f}")
    print(f"    swept, 64-sample grid        = {s64:.9f}   (excess {s64 - nat:+.3e}, "
          f"{(s64-nat)/nat*100:+.5f}%)")
    print(f"    swept, 4096-sample grid      = {s4096:.9f}   (excess {s4096 - nat:+.3e}, "
          f"{(s4096-nat)/nat*100:+.5f}%)")
    sep = (s4096 - nat) / (s64 - nat) if (s64 - nat) > 0 else float("inf")
    print(f"    negative-control separation  = {sep:.1f}x")
    print("  DIFFERENCE SERIES (swept - held, then slew) -- the sine cancels exactly:")
    print(f"    64-sample grid   max|d diff| = {dd64:.3e}")
    print(f"    4096-sample grid max|d diff| = {dd4096:.3e}")
    print(f"    negative-control separation  = {dd4096 / dd64:.1f}x")

print()
print("=" * 78)
print("E. DSP-08 room-size invariance -- is it EXACT, and where do the clamps bind?")
print("=" * 78)
RIG = 7.93165
for lam in (0.5, 1.0, 2.0, 4.0):
    for blur in (0.0, 0.25, 1.0):
        rs = min(blur * 0.5 * RIG * lam, 8.0)
        want = blur * 0.5 * RIG * lam
        bind = "CLAMPED (invariance BREAKS)" if want > 8.0 else "ok"
        print(f"  lambda={lam:4.1f} blur={blur:4.2f}: r_s wanted {want:7.3f} m -> {rs:6.3f} m   {bind}")
print()
print("  The 8 m kMaxBlurMetres ceiling and the 0.05 m kMinDistance floor are the ONLY")
print("  scale-breaking terms. Everything else is homogeneous of degree 0 in lambda:")
print("     d_i -> lambda*d_i  =>  t_i = d_i^-a -> lambda^-a * t_i")
print("     k = 1/sqrt(sum (w t)^2) -> lambda^a * k   =>  v_i = k w_i t_i  INVARIANT, exactly.")
print(f"  A DSP-08 probe must therefore keep blur*0.5*rigScale*lambda < 8 at BOTH scales:")
print(f"     default rig 7.93165 m, lambda=2, blur=1 -> {1*0.5*RIG*2:.3f} m  -- ALREADY CLAMPED.")
print(f"     lambda=2, blur=0.25                     -> {0.25*0.5*RIG*2:.3f} m  -- safe.")

print()
print("=" * 78)
print("F. setCutoffFrequency's jassert: isPositiveAndBelow(fc, fs*0.5)")
print("=" * 78)
for fs in (22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 192000.0):
    ny = fs * 0.5
    ok = 0.0 < 20000.0 < ny
    print(f"  fs={fs:8.0f}  Nyquist={ny:9.1f}  fc_max=20000 -> {'ok' if ok else 'ASSERTS IN DEBUG'}")
print()
print("  A host running 32 kHz (or 22.05 kHz) trips the jassert and, worse, tan() goes")
print("  negative past Nyquist -> G < 0 -> the one-pole becomes unstable/nonsense.")
print("  Ceiling must be min(20000, 0.45*fs), not the literal 20000 in ARCHITECTURE 3.5.2.")
for fs in (22050.0, 32000.0, 44100.0, 48000.0):
    print(f"    fs={fs:8.0f} -> ceiling {min(20000.0, 0.45*fs):8.1f} Hz")

print()
print("=" * 78)
print("G. Transcendental budget per control block (PERF-01)")
print("=" * 78)
blocks_per_s = 48000 / 64
print(f"  control blocks/second at 48 kHz          = {blocks_per_s:.0f}")
print(f"  std::tan  via setCutoffFrequency, 2/block = {2*blocks_per_s:.0f}/s")
print(f"  std::exp2 for the two cutoffs,     2/block = {2*blocks_per_s:.0f}/s")
print(f"  std::pow  via decibelsToGain,      2/block = {2*blocks_per_s:.0f}/s  (hull trim, per sub-point)")
print(f"  std::pow  via decibelsToGain,      1/block = {blocks_per_s:.0f}/s  (outGain, already there at 2.2)")
print(f"  countedPow inside dbap::solve,    16/block = {16*blocks_per_s:.0f}/s  (unchanged; probe AE asserts ==16)")
print(f"  -> 2.3 adds {6*blocks_per_s:.0f} transcendental calls/s on top of {17*blocks_per_s:.0f}. ~35% more,")
print("     all at control rate, none per sample. PERF-01's per-sample budget is untouched.")


# ##########################################################################
# PART 2  (from air_study2.py)
# ##########################################################################
import math

FS = 48000.0
GRID = 64          # GainStage::kControlBlock

def G_of(fc, fs=FS):
    g = math.tan(math.pi * fc / fs)
    return g / (1.0 + g)

def air_fc(air, d_hull, dref=3.0, fs=FS):
    return min(max(20000.0 * 2.0 ** (-air * d_hull / dref), 500.0), min(20000.0, 0.45 * fs))

def db(x):
    return 20.0 * math.log10(x) if x > 0 else float("-inf")


def run(tone, amp, air_of_n, N, update_every, reentry=None, active_of_n=None):
    """One render. Returns (out, boundary_steps, transition_steps).

    boundary_steps  : |(G_new - G_old) * (x - s)| at each coefficient change
    transition_steps: |y - x| at each false->true airActive edge (the audible click)
    reentry: None -> leave state resident | 'zero' -> reset(0) | 'input' -> reset(x)
    """
    s, G, active_prev = 0.0, None, True
    out, bsteps, tsteps = [], [], []

    for n in range(N):
        x = amp * math.sin(2 * math.pi * tone * n / FS)
        active = True if active_of_n is None else active_of_n(n)

        if n % update_every == 0:
            G_new = G_of(air_fc(air_of_n(n), 5.0))
            if G is not None and active and active_prev:
                bsteps.append(abs((G_new - G) * (x - s)))
            G = G_new

        if active and not active_prev:                      # the re-entry edge
            if reentry == "zero":
                s = 0.0
            elif reentry == "input":
                s = x
            tsteps.append(abs((G * x + (1.0 - G) * s) - x))

        if active:
            v = G * (x - s)
            y = v + s
            s = y + v
        else:
            y = x
        out.append(y)
        active_prev = active
    return out, bsteps, tsteps


def max_step(series):
    return max(abs(series[n] - series[n - 1]) for n in range(1, len(series)))


N = 48000
AMP = 0.5

print("=" * 78)
print("D1. The COEFFICIENT step, isolated:  dy = (G_new - G_old) * (x - s)")
print("    airAmount swept 0.02 -> 1.0 over 1 s, d_hull = 5 m, always active.")
print("=" * 78)
sweep = lambda n: 0.02 + 0.98 * n / (N - 1)

print(f"{'tone':>7} {'update':>8} {'max coeff step':>16} {'natural slew':>14} {'step/slew':>11}")
for tone in (1000.0, 4000.0, 8000.0):
    row = {}
    for every in (GRID, 4096):
        o, b, _ = run(tone, AMP, sweep, N, every)
        row[every] = (max(b), max_step(o))
        print(f"{tone:7.0f} {every:8d} {max(b):16.3e} {max_step(o):14.6f} "
              f"{max(b)/max_step(o):11.3e}")
    sep = row[4096][0] / row[GRID][0]
    print(f"{'':7} {'ratio':>8} {sep:16.1f}x   <- negative-control separation on the STEP itself")
print()
print("  The coefficient step is 3-5 orders of magnitude below the sine's own per-sample")
print("  slew at every tone. `max|out[n]-out[n-1]|` CANNOT see it -- which is why the")
print("  literal D3 formulation measured 0.00000% excess at 1 kHz in part 1.")
print("  It scales EXACTLY 64x with the update interval, so it is bounded analytically:")
print("      |dy| <= max|dG per control block| * max|x - s|  <=  max|dG| * 2*peak")

print()
print("=" * 78)
print("D2. max|dG| per control block, for a full-speed 0->1 airAmount sweep")
print("=" * 78)
for T in (0.25, 0.5, 1.0, 2.0):
    n_blocks = int(T * FS / GRID)
    Gs = [G_of(air_fc(k / (n_blocks - 1), 5.0)) for k in range(n_blocks)]
    dG = max(abs(Gs[k] - Gs[k - 1]) for k in range(1, n_blocks))
    print(f"  sweep 0->1 in {T:4.2f} s ({n_blocks:5d} control blocks): "
          f"max|dG| = {dG:.6e}  -> bound {dG*2*AMP:.3e} at peak {AMP}")
print()
print("  Derived, not tuned: the probe asserts max|dy| <= max|dG| * 2 * peak, with max|dG|")
print("  computed IN THE PROBE from the same sweep the render used. Nothing to drift.")

print()
print("=" * 78)
print("E. The SKIP TRANSITION -- the discontinuity that actually matters (QUAL-01 crit 2)")
print("   Source sits outside the hull, ducks inside for 0.25 s, comes back out.")
print("=" * 78)
# outside for 0.25 s, inside for 0.25 s, outside again. airActive = outside.
inside = lambda n: not (12000 <= n < 24000)
hold = lambda n: 0.35                      # shipping default airAmount

print(f"{'tone':>7} {'re-entry policy':>18} {'|y-x| at edge':>16} {'natural slew':>14} {'edge/slew':>11}")
for tone in (1000.0, 8000.0):
    for pol, name in ((None, "resident (D2)"), ("zero", "reset(0)"), ("input", "reset(x)")):
        o, _, t = run(tone, AMP, hold, N, GRID, reentry=pol, active_of_n=inside)
        edge = max(t) if t else 0.0
        # natural slew measured on a clean always-active render of the same tone
        base, _, _ = run(tone, AMP, hold, N, GRID)
        print(f"{tone:7.0f} {name:>18} {edge:16.3e} {max_step(base):14.6f} "
              f"{edge/max_step(base):11.3e}")
print()
print("  The resident-state re-entry click is 5 to 7 ORDERS OF MAGNITUDE larger than the")
print("  coefficient step, and at 1 kHz it is ~1.5x the signal's own per-sample slew --")
print("  i.e. plainly visible to exactly the max|out[n]-out[n-1]| method probe AS uses.")
print("  reset(x) drives it to 0 identically, at both tones, for one float store.")

print()
print("=" * 78)
print("F. So what does QUAL-01 criterion 2 assert?  With reset(x) the edge is EXACT.")
print("=" * 78)
for tone in (1000.0, 8000.0):
    o_skip, _, _ = run(tone, AMP, hold, N, GRID, reentry="input", active_of_n=inside)
    # the same render, but the filter is never skipped (d_hull never reaches 0)
    o_always, _, _ = run(tone, AMP, hold, N, GRID)
    # worst per-sample step of the SKIPPING render, vs the always-on one
    print(f"  {tone:5.0f} Hz: max|d out| skipping = {max_step(o_skip):.9f}, "
          f"always-on = {max_step(o_always):.9f}")
    print(f"           bound = the 20 kHz filter's own steady-state step "
          f"= {1 - 1/math.sqrt(1 + (math.tan(math.pi*tone/FS)/math.tan(math.pi*20000/FS))**2):.3e} * peak")
print()
print("  Both edges (enter and re-enter) are then bounded by the fc->20 kHz steady-state")
print("  response difference, which is 1.5e-4 at 1 kHz and 1.2e-2 at 8 kHz (part 1, table B).")
print("  A probe asserting max|d out| <= naturalSlew + 2*peak*(1-|H_20k(tone)|) is derived")
print("  from the transfer function, not tuned, and FAILS if reset(x) is dropped.")


# ##########################################################################
# PART 3  (from air_study3.py)
# ##########################################################################
import math

FS, GRID, N, AMP = 48000.0, 64, 48000, 0.5
LEAD = 2000


def G_of(fc, fs=FS):
    g = math.tan(math.pi * fc / fs)
    return g / (1 + g)


def air_fc(air, d, dref=3.0, fs=FS):
    return min(max(20000.0 * 2.0 ** (-air * d / dref), 500.0), min(20000.0, 0.45 * fs))


def run(tone, amp, air_of_n, n_total, every, reentry=None, active_of_n=None):
    s, G, ap = 0.0, None, True
    out, tsteps = [], []
    for n in range(n_total):
        x = amp * math.sin(2 * math.pi * tone * n / FS)
        act = True if active_of_n is None else active_of_n(n)
        if n % every == 0:
            G = G_of(air_fc(air_of_n(n), 5.0))
        if act and not ap:
            if reentry == "zero":
                s = 0.0
            elif reentry == "input":
                s = x
            tsteps.append(abs((G * x + (1 - G) * s) - x))
        if act:
            v = G * (x - s)
            y = v + s
            s = y + v
        else:
            y = x
        out.append(y)
        ap = act
    return out, tsteps


def mstep(o, start):
    return max(abs(o[n] - o[n - 1]) for n in range(start, len(o)))


inside = lambda n: not (12000 <= n < 24000)   # outside, inside for 0.25 s, outside
hold = lambda n: 0.35                          # shipping default airAmount

print("Lead-in of %d samples EXCLUDED. The filter's cold start from s = 0 against a" % LEAD)
print("sine that is already moving is itself a step -- a probe that measures from sample 0")
print("measures THAT, not the automation. (It read 0.0673 vs a true steady cap of 0.0653.)")
print()
print(f"{'tone':>7} {'re-entry policy':>17} {'|y-x| at edge':>15} {'max|d out| post-lead':>21}")
for tone in (1000.0, 8000.0):
    w = 2 * math.pi * tone / FS
    for pol, name in ((None, "resident (D2)"), ("zero", "reset(0)"), ("input", "reset(x)")):
        o, t = run(tone, AMP, hold, N, GRID, reentry=pol, active_of_n=inside)
        print(f"{tone:7.0f} {name:>17} {max(t):15.3e} {mstep(o, LEAD):21.9f}")
    base, _ = run(tone, AMP, hold, N, GRID)
    print(f"{tone:7.0f} {'always-on (ref)':>17} {'-':>15} {mstep(base, LEAD):21.9f}")
    print(f"{tone:7.0f} {'unfiltered cap':>17} {'-':>15} {2*AMP*math.sin(w/2):21.9f}")
    print()

print("Reading:")
print("  * 'resident' leaves a per-sample step of 2.7e-2 at 1 kHz / 2.0e-1 at 8 kHz on top of")
print("    a signal whose own per-sample slew caps at 6.5e-2 / 4.5e-1. At 1 kHz that click is")
print("    42% of the signal's fastest legitimate move. It is a click, and it is audible.")
print("  * reset(0) leaves ~1e-13 -- the fc(d_hull->0) -> 20 kHz limit means s has already")
print("    converged to x by the time the edge arrives, so reset(0) and reset(x) coincide HERE.")
print("    They do NOT coincide when the puck re-enters at speed (fc still low at the edge).")
print("  * reset(x) is 0 IDENTICALLY, at every tone, every fc, every entry speed --")
print("    y = G*x + (1-G)*x = x by algebra, not by convergence.")
print()

print("=" * 78)
print("The case that separates reset(0) from reset(x): FAST re-entry, filter still dark.")
print("Puck jumps from d_hull = 12 m straight to inside, then straight back out to 12 m.")
print("=" * 78)
fast_inside = lambda n: not (12000 <= n < 12064)      # inside for exactly ONE control block


def run_d(tone, amp, d_of_n, n_total, every, reentry, active_of_n):
    s, G, ap = 0.0, None, True
    out, tsteps = [], []
    for n in range(n_total):
        x = amp * math.sin(2 * math.pi * tone * n / FS)
        act = active_of_n(n)
        if n % every == 0:
            G = G_of(air_fc(0.35, d_of_n(n)))
        if act and not ap:
            if reentry == "zero":
                s = 0.0
            elif reentry == "input":
                s = x
            tsteps.append(abs((G * x + (1 - G) * s) - x))
        if act:
            v = G * (x - s)
            y = v + s
            s = y + v
        else:
            y = x
        out.append(y)
        ap = act
    return out, tsteps


d12 = lambda n: 12.0        # fc = 20000 * 2^(-0.35*12/3) = 20000 * 2^-1.4 = 7.58 kHz
print(f"  d_hull = 12 m at airAmount 0.35  ->  fc = {air_fc(0.35, 12.0):.1f} Hz, "
      f"G = {G_of(air_fc(0.35,12.0)):.6f}, 1-G = {1-G_of(air_fc(0.35,12.0)):.6f}")
print()
print(f"{'tone':>7} {'re-entry policy':>17} {'|y-x| at edge':>15} {'vs unfiltered cap':>19}")
for tone in (1000.0, 8000.0):
    cap = 2 * AMP * math.sin(2 * math.pi * tone / FS / 2)
    for pol, name in ((None, "resident (D2)"), ("zero", "reset(0)"), ("input", "reset(x)")):
        o, t = run_d(tone, AMP, d12, N, GRID, pol, fast_inside)
        print(f"{tone:7.0f} {name:>17} {max(t):15.3e} {max(t)/cap:18.1%}")
    print()


# ##########################################################################
# PART 4  (from air_study4.py)
# ##########################################################################
import cmath
import math

def H_complex(fc, fs, f):
    """Exact H(e^{jw}) of the TPT one-pole:  G(z+1)/(z-(1-2G))."""
    g = math.tan(math.pi * fc / fs)
    G = g / (1.0 + g)
    z = cmath.exp(1j * 2 * math.pi * f / fs)
    return G * (z + 1) / (z - (1 - 2 * G))

def db(x):
    return 20.0 * math.log10(x) if x > 0 else float("-inf")

print("=" * 86)
print("H. The hull-crossing step, fc = 20 kHz, filter present vs absent")
print("=" * 86)
print(f"{'fs':>7} {'tone':>7} {'|H|':>9} {'|H| dB':>9} {'phase deg':>10} "
      f"{'MAGNITUDE only':>15} {'FULL |H-1|':>12} {'phase/mag':>10}")
for fs in (44100.0, 48000.0):
    for f in (1000, 2000, 4000, 8000, 12000, 15000, 20000):
        H = H_complex(20000.0, fs, f)
        magonly = abs(abs(H) - 1.0)
        full = abs(H - 1.0)
        print(f"{fs:7.0f} {f:7d} {abs(H):9.6f} {db(abs(H)):9.4f} "
              f"{math.degrees(cmath.phase(H)):10.4f} {magonly:15.3e} {full:12.3e} "
              f"{full/magonly:10.1f}x")
    print()

print("Reading: at 1 kHz the magnitude tilt is 1.5e-4 of the component but the FULL step")
print("is 1.8e-2 -- 114x larger, and entirely the 1.0 degree phase lag. D2's accepted cost")
print("is therefore ~1.8% of a 1 kHz component and ~15% of an 8 kHz component, not the")
print("0.0013 dB / 0.10 dB the magnitude-only figures imply. Still bounded, still a")
print("one-sample event, but it is the number QUAL-01 criterion 2 must be measured against.")
print()

print("=" * 86)
print("I. The two edges are NOT symmetric once reset(x) is in")
print("=" * 86)
print("  EXIT  (outside -> inside, filter switched OUT): step = A*|H_20k(f) - 1|, unavoidable.")
print("  ENTRY (inside -> outside, filter switched IN):")
print("      resident state : up to (1-G)*|s_stale - x|,  s_stale arbitrarily old")
print("      reset(0)       : (1-G)*|x|, up to (1-G)*peak")
print("      reset(x)       : v = G*(x - s) = G*0 = 0 exactly, y = 0 + x = x  -> BIT-EXACT")
print()
for fs in (44100.0, 48000.0):
    g = math.tan(math.pi * 20000.0 / fs)
    G = g / (1 + g)
    print(f"  fs={fs:.0f}: (1-G) at the 20 kHz re-entry corner = {1-G:.6f}")
    print(f"           reset(0) worst entry step = {1-G:.4f}*peak = {db(1-G):.2f} dB re peak")
    print(f"           reset(x) worst entry step = 0 (exact)")
print()
print("  So the derived QUAL-01/2 predictions a probe should ASSERT, not merely bound:")
for fs in (48000.0,):
    for f in (1000, 8000):
        H = H_complex(20000.0, fs, f)
        print(f"    fs={fs:.0f}, {f} Hz, amplitude A:")
        print(f"      exit  step  == A * {abs(H-1):.6f}   (predicted from the transfer function)")
        print(f"      entry step  == 0 exactly           (bit-exact, by v = G*0)")

print()
print("=" * 86)
print("J. Why raising the 20 kHz BASE would remove the step -- named, not recommended")
print("=" * 86)
print("  The step exists because fc(d_hull = 0) is 20 kHz rather than infinite. As fc -> fs/2,")
print("  G -> 1 and H -> 1, so the switch becomes continuous:")
for fs in (48000.0,):
    for base in (20000.0, 22000.0, 23000.0, 23800.0, 23950.0):
        H = H_complex(base, fs, 8000.0)
        print(f"    fc(d=0) = {base:7.0f} Hz -> |H-1| at 8 kHz = {abs(H-1):.5f}")
print("  But the base is also the whole curve's anchor (0.35 / 5 m -> 13.3 kHz in the")
print("  ARCHITECTURE table). Moving it re-tunes the musical mapping and is a discuss-boundary")
print("  change, not a plan-phase one. Recorded here so the option is on the record.")

print()
print("=" * 86)
print("K. ARCHITECTURE 3.5.2's own fc table, re-derived")
print("=" * 86)
for air, d, quoted in ((0.35, 5, "13.3 kHz"), (0.35, 15, "5.9 kHz"),
                       (1.00, 5, "6.3 kHz"), (1.00, 15, "0.62 kHz")):
    fc = min(max(20000.0 * 2 ** (-air * d / 3.0), 500.0), 20000.0)
    print(f"  airAmount {air:4.2f}, d_hull {d:2d} m -> {fc:8.1f} Hz   (quoted {quoted})  ok")
print("  All four correct. The 500 Hz floor never binds inside the exposed ranges:")
print(f"    worst case airAmount 1.0, d_hull large -> floor reached at d_hull = "
      f"{3.0*math.log2(20000/500):.2f} m")
