# O-simpleGrain v2.0.0 Pulsar mode — Wave C gate (task C1).
#
# One gate on the INSTALLED VST3 (pedalboard, via uv) that re-proves every Pulsar
# acceptance item. The render / spectrum helpers are wave_a.py's (imported, not copied).
# Every check prints its measured value and its bound, and every check is run a
# second time against a NEGATIVE CONTROL that must FAIL the same bound:
#
#   check          bound                                   control (must fail)
#   fp             MIDI 57 @ 44.1/48/96 kHz within 0.1 cent  Grain Pitch +1 cent (a clock 1 cent off)
#   formant        centroid rises x1.05 per step 2..8x,      Formant held at 4x for all five renders
#                  f_p within 0.1 cent                       (an inert Formant)
#   alias          <= -60 dB: Sine C8/C7/A6 x32; Sample      ctl-ceiling build (0.45 fs ceiling):
#                  fire/voice/piano; FM 100 % x glisson +24  Sine C8 x32, Sample piano C8 x32,
#                  sweep; Pitch Spray 12 Nyquist band        Pitch Spray band
#   bh-band        level per duty over L 0.5..1.6 ms         ctl-ceiling build (power gains forced 1)
#                  spans <= 0.5 dB
#   bh-sidelobe    0.3 ms one-cycle pulsaret @ 96 kHz: every ctl-nobh build (Hann at every length)
#                  harmonic >= 5 f_form <= -60 dB re peak
#   frac-onset     96 kHz, f_p 220 (436.36-sample period),   ctl-int build (sub-sample offset dropped)
#                  formant 5 kHz: worst off-comb <= -60 dB
#   mask           Mask 3: f_p/3 line > -30 dB (Off < -80),  ctl-absent build (Mask/Skip/FM/Glisson
#                  at f_p/3 within 0.1 cent                  code removed)
#   skip           Skip 25/50/75 %: count within 4 sigma,    ctl-absent build, Skip 50 %
#                  survivors on Skip-0 onsets
#   fm-line        ratio 0.5 / 1.5 depth 60 %: f_p/2 line    ctl-absent build
#                  > -40 dB (no FM < -80) within 0.1 cent
#   fm-inert       ratio 1 / 2 depth 60 %: spectrum = no-FM  ratio 1.01 (onsets no longer 1/f_p apart)
#                  within -100 dB
#   fm-crossing    harness P7: the FM onset solve vs           P7's in-harness pre-fix 3-step Newton
#                  bisection, dm 0.005..1, <= 1e-7 sample      (must fail the same bound)
#   glisson        half-cycle end 0.4293 L (+12) / 0.6139 L  ctl-absent build
#                  (-12), within 0.01 L
#   sine-no-src    Sine renders bit-identically over a        the same check in Sample content (must
#                  silent dropped source and over Fire,       differ: proves the source swap reaches
#                  and is audible (> -60 dBFS)               the engine); + harness P2 (NaN source)
#   mode-switch    harness P6 (see below)                    harness builds that CUT the other kind's
#                                                            grains / ramp ONE shared output gain
#                                                            grains / SNAP the output gain
#
# Mode switch: pedalboard cannot move a parameter inside one render, and a two-call
# render parked in silence would test a switch with no grain in flight. The check is
# therefore HARNESS-SIDE (tests/render-harness pulsar_checks.cpp, P6), on a processor
# built from the same tree: Cloud->Pulsar and Pulsar->Cloud switched mid-note at a
# block boundary inside a pulsaret, verdict vs the same-motion model (old-mode tail +
# new-mode note started at the switch, each on its own bus gain).
#
# usage: uv run --no-project --python 3.12 --with pedalboard --with numpy --with soundfile \
#            python -I gate.py [PLUGIN.vst3] --ctl-ceiling C.vst3 --ctl-absent A.vst3 \
#            --ctl-nobh N.vst3 --ctl-int I.vst3 --harness H \
#            --harness-snapgrain HG --harness-sharedgain HS [--only fp,alias,...]
# PLUGIN defaults to the installed ~/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3.
# A missing control is a FAIL (the gate cannot claim the check discriminates).
import os
import re
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ARGV = sys.argv[1:]


def opt(name, default=None):
    if name in ARGV:
        i = ARGV.index(name); v = ARGV[i + 1]; del ARGV[i:i + 2]; return v
    return default


CTL_CEIL = opt("--ctl-ceiling")
CTL_ABSENT = opt("--ctl-absent")
CTL_NOBH = opt("--ctl-nobh")
CTL_INT = opt("--ctl-int")
HARNESS = opt("--harness")
H_SNAPGRAIN = opt("--harness-snapgrain")
H_SHAREDGAIN = opt("--harness-sharedgain")
ONLY = opt("--only")
PLUGIN = ARGV[0] if ARGV else os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3")

# wave_a parses its own argv at import: hand it just the plugin.
_argv = sys.argv
sys.argv = [_argv[0], PLUGIN]
sys.path.insert(0, HERE)
import wave_a as W        # noqa: E402  (render / spectrum helpers)
sys.argv = _argv

fails = 0
results = []              # (check, ok)


def verdict(check, ok, msg):
    global fails
    fails += not ok
    results.append((check, ok))
    print(f"{'PASS' if ok else 'FAIL'} [{check}] {msg}", flush=True)


def control(check, path, label, fn):
    """fn(path) -> (passes_bound, value_msg). The control must NOT pass."""
    if path is None:
        verdict(check, False, f"CONTROL {label}: not supplied - the check is unproven")
        return
    passes, msg = fn(path)
    verdict(check, not passes, f"CONTROL {label}: {msg} -> {'FAILS the bound as required' if not passes else 'PASSES (control is BROKEN)'}")


def want(name):
    return ONLY is None or name in ONLY.split(",")


mono, render, db = W.mono, W.render, W.db


# ------------------------------------------------------------------------------- fp
def check_fp():
    fs = 48000
    t = np.arange(2 * fs) / fs
    est = W.cents(W.peak_near(np.sin(2 * np.pi * 220.0 * t), fs, 220.0, 5.0), 220.0)
    verdict("fp", abs(est) < 0.01, f"estimator liveness: a synthetic 220 Hz sine reads {est:+.4f} cent (bound 0.01)")

    def fp_cents(pl, fs, extra=None):
        y = mono(render(pl, dict(formant=1.0, **(extra or {})), [57], 3.0, fs))
        return W.cents(W.peak_near(y[int(1.0 * fs):], fs, 220.0, 5.0), 220.0)
    for fs in (44100, 48000, 96000):
        c = fp_cents(PLUGIN, fs)
        verdict("fp", abs(c) <= 0.1, f"MIDI 57, Formant 1x @ {fs} Hz: f_p {c:+.4f} cent re 220 Hz (bound |c| <= 0.1)")
    control("fp", PLUGIN, "Grain Pitch +1 cent (clock 1 cent off)",
            lambda pl: (abs(c := fp_cents(pl, 48000, dict(grain_pitch=0.01))) <= 0.1, f"{c:+.4f} cent"))


# --------------------------------------------------------------------------- formant
def check_formant():
    fs = 48000

    def sweep(pl, forms):
        cs, fps = [], []
        for F in forms:
            y = mono(render(pl, dict(formant=F), [57], 2.0, fs))[fs:]
            cs.append(W.centroid(y, fs)); fps.append(W.cents(W.peak_near(y, fs, 220.0, 5.0), 220.0))
        rising = all(b > a * 1.05 for a, b in zip(cs, cs[1:]))
        return rising, cs, fps
    rising, cs, fps = sweep(PLUGIN, (2.0, 3.0, 4.0, 6.0, 8.0))
    verdict("formant", rising, "centroid " + " < ".join(f"{c:.0f}" for c in cs)
            + " Hz for Formant 2/3/4/6/8x (bound: each step x1.05)")
    verdict("formant", max(abs(c) for c in fps) <= 0.1,
            "f_p stays: " + ", ".join(f"{c:+.4f}" for c in fps) + " cent (bound 0.1)")
    control("formant", PLUGIN, "Formant held at 4x for the five renders (inert Formant)",
            lambda pl: ((r := sweep(pl, (4.0,) * 5))[0], "centroid " + " / ".join(f"{c:.0f}" for c in r[1]) + " Hz"))


# ----------------------------------------------------------------------------- alias
def alias_sine(pl, note):
    fs = 44100
    return W.alias_db(mono(render(pl, dict(formant=32.0), [note], 2.0, fs))[fs:], fs, 440 * 2 ** ((note - 69) / 12))


def alias_sample(pl, src, note, F):
    fs = 44100
    y = mono(render(pl, dict(pulsaret="Sample", source=src, formant=F), [note], 2.5, fs))[int(1.5 * fs):]
    return W.alias_db(y, fs, 440 * 2 ** ((note - 69) / 12))


def nyq_band(pl, content):
    fs = 44100
    sets = dict(pulsaret=content, source="piano", formant=32.0, fm_depth=100.0, fm_ratio=1.5, pitch_spray=12.0)
    y = mono(render(pl, sets, [108], 2.5, fs))[int(1.5 * fs):]
    X, df = W.spectrum(y, fs, pad=1)
    f = np.arange(len(X)) * df
    return db(X[(f > 0.47 * fs) & (f < 0.499 * fs)].max() / X.max())


def check_alias():
    fs = 44100
    for note in (108, 96, 93):
        a, fa = alias_sine(PLUGIN, note)
        verdict("alias", a <= -60.0, f"Sine MIDI {note} x Formant 32 @ 44.1 kHz: worst off-comb {a:.1f} dB at {fa:.0f} Hz (bound -60)")
    for src in ("fire", "voice", "piano"):
        worst = max((alias_sample(PLUGIN, src, n, F) + (n, F)) for n, F in ((108, 32.0), (96, 32.0), (84, 32.0), (72, 8.0)))
        verdict("alias", worst[0] <= -60.0, f"Sample '{src}' worst over MIDI 108/96/84 x32, 72 x8: {worst[0]:.1f} dB "
                f"(MIDI {worst[2]} x{worst[3]:g}, at {worst[1]:.0f} Hz; bound -60)")
    for content in ("Sine", "Sample"):
        worst = []
        for note in (108, 100, 90):
            for gl in (24.0, 0.0):
                sets = dict(pulsaret=content, source="piano", formant=32.0, fm_depth=100.0, fm_ratio=1.5, glisson=gl)
                y = mono(render(PLUGIN, sets, [note], 2.5, fs))[int(1.5 * fs):]
                fp = min(440 * 2 ** ((note - 69) / 12), 0.1 * fs / 2 ** (gl / 12))
                a, fa = W.alias_db(y, fs, fp / 2)
                worst.append((a, note, gl, fa))
        a, note, gl, fa = max(worst)
        verdict("alias", a <= -60.0, f"sweep {content} MIDI 108/100/90 x32 x FM 100 % (ratio 1.5) x glisson 0/+24: "
                f"worst {a:.1f} dB (MIDI {note}, glisson {gl:g}, at {fa:.0f} Hz; bound -60)")
    for content in ("Sine", "Sample"):
        v = nyq_band(PLUGIN, content)
        verdict("alias", v <= -60.0, f"Pitch Spray 12 (+FM 100 % ratio 1.5, x32, C8) {content}: 0.47..0.5 fs band {v:.1f} dB re peak (bound -60)")
    control("alias", CTL_CEIL, "ceiling 0.45 fs, Sine C8 x32", lambda pl: ((a := alias_sine(pl, 108)[0]) <= -60.0, f"{a:.1f} dB"))
    control("alias", CTL_CEIL, "ceiling 0.45 fs, Sample 'piano' C8 x32",
            lambda pl: ((a := alias_sample(pl, "piano", 108, 32.0)[0]) <= -60.0, f"{a:.1f} dB"))
    control("alias", CTL_CEIL, "ceiling 0.45 fs, Pitch Spray band (Sine)", lambda pl: ((v := nyq_band(pl, "Sine")) <= -60.0, f"{v:.1f} dB"))


# --------------------------------------------------------------------------- bh-band
def band_spread(pl):
    fs, fp = 48000, 110.0
    rows = []
    for Lms in (0.5, 0.6, 0.7, 0.75, 0.8, 0.9, 1.0, 1.1, 1.2, 1.25, 1.3, 1.4, 1.6):
        F = 1000.0 / (Lms * fp)
        y = render(pl, dict(formant=F), [45], 1.5, fs)[:, int(0.5 * fs):]
        rows.append((Lms, 10 * np.log10(float(np.mean(y ** 2)) * F)))     # P / d, d = 1 / F
    e = np.array([r[1] for r in rows])
    return float(e.max() - e.min()), rows


def check_bh_band():
    s, rows = band_spread(PLUGIN)
    verdict("bh-band", s <= 0.5, f"level per duty over L 0.5..1.6 ms (Hann -> BH band 0.75..1.25) spans {s:.3f} dB "
            "(bound 0.5): " + " ".join(f"{L}:{v - rows[-1][1]:+.2f}" for L, v in rows))
    control("bh-band", CTL_CEIL, "power gains forced to 1", lambda pl: ((c := band_spread(pl)[0]) <= 0.5, f"spans {c:.2f} dB"))


# ----------------------------------------------------------------------- bh-sidelobe
def sidelobe(pl):
    """0.3 ms one-cycle Sine pulsaret at 96 kHz (f_p 220, f_form 3333 Hz): the largest
    harmonic at or above 5 f_form (the BH main lobe ends at f_form + 4/L = 5 f_form;
    Hann's -60 dB extent is 6.63 f_form) re the largest harmonic."""
    fs, fp, L = 96000, 220.0, 0.3e-3
    F = 1.0 / (L * fp)
    y = mono(render(pl, dict(formant=F), [57], 2.0, fs))[int(0.5 * fs):]
    X, df = W.spectrum(y, fs)
    ks = np.arange(1, int(0.45 * fs / fp))
    amp = np.array([X[max(0, int(round(k * fp / df)) - 3):int(round(k * fp / df)) + 4].max() for k in ks])
    fform = fp * F
    hi = ks * fp >= 5.0 * fform
    return db(amp[hi].max() / amp.max()), fform


def check_bh_sidelobe():
    v, ff = sidelobe(PLUGIN)
    verdict("bh-sidelobe", v <= -60.0, f"0.3 ms pulsaret (f_form {ff:.0f} Hz) @ 96 kHz: largest harmonic >= 5 f_form "
            f"{v:.1f} dB re peak (bound -60)")
    if CTL_NOBH is not None:
        h, _ = sidelobe(CTL_NOBH)
        print(f"INFO [bh-sidelobe] improvement vs Hann at 0.3 ms: {h - v:.1f} dB ({h:.1f} -> {v:.1f})")
    control("bh-sidelobe", CTL_NOBH, "no BH switch (Hann at 0.3 ms)", lambda pl: ((h := sidelobe(pl)[0]) <= -60.0, f"{h:.1f} dB"))


# ------------------------------------------------------------------------ frac-onset
def onset_spur(pl):
    fs = 96000
    F = 5000.0 / 220.0
    y = mono(render(pl, dict(formant=F), [57], 2.5, fs))[int(0.5 * fs):]
    return W.alias_db(y, fs, 220.0)


def check_frac_onset():
    a, fa = onset_spur(PLUGIN)
    verdict("frac-onset", a <= -60.0, f"96 kHz, f_p 220 Hz (period 436.36 samples), formant 5 kHz: worst off-comb "
            f"{a:.1f} dB at {fa:.0f} Hz (bound -60)")
    if CTL_INT is not None:
        b, fb = onset_spur(CTL_INT)
        print(f"INFO [frac-onset] spur reduction vs integer onsets: {b - a:.1f} dB ({b:.1f} at {fb:.0f} Hz -> {a:.1f})")
    control("frac-onset", CTL_INT, "integer onsets", lambda pl: ((b := onset_spur(pl)[0]) <= -60.0, f"{b:.1f} dB"))


# ------------------------------------------------------------------------------ mask
def line_db(y, f, fs=48000):
    X, df = W.spectrum(y, fs)
    i = int(round(f / df))
    return db(X[i - 3:i + 4].max() / X.max())


def mask_line(pl):
    y3 = mono(render(pl, dict(formant=4.0, pulse_mask="3"), [57], 2.5, 48000))[48000:]
    return line_db(y3, 220.0 / 3), y3


def check_mask():
    l3, y3 = mask_line(PLUGIN)
    l0 = line_db(mono(render(PLUGIN, dict(formant=4.0), [57], 2.5, 48000))[48000:], 220.0 / 3)
    f3 = W.peak_near(y3, 48000, 220.0 / 3, 2.0)
    verdict("mask", l3 > -30.0 and l0 < -80.0 and abs(W.cents(f3, 220.0 / 3)) <= 0.1,
            f"Mask 3: f_p/3 line {l3:.1f} dB re peak (bound > -30), Mask Off {l0:.1f} dB (bound < -80), "
            f"at {f3:.4f} Hz ({W.cents(f3, 220.0 / 3):+.3f} cent, bound 0.1)")
    control("mask", CTL_ABSENT, "Mask code removed, Mask 3", lambda pl: ((l := mask_line(pl)[0]) > -30.0, f"f_p/3 line {l:.1f} dB"))


# ------------------------------------------------------------------------------ skip
def skip_count(pl, p, b0):
    bp = W.bursts(mono(render(pl, dict(formant=4.0, grain_skip=100 * p), [33], 3.0, 48000)))
    on0 = {b[0] for b in b0}
    aligned = all(b[0] in on0 for b in bp)
    n, exp = len(bp), (1 - p) * len(b0)
    sd = np.sqrt(len(b0) * p * (1 - p))
    return abs(n - exp) <= 4 * sd and aligned, f"{n} of {len(b0)} pulses sound (expect {exp:.0f} +- {4 * sd:.0f}), all on Skip-0 onsets: {aligned}"


def check_skip():
    b0 = W.bursts(mono(render(PLUGIN, dict(formant=4.0), [33], 3.0, 48000)))
    for p in (0.25, 0.5, 0.75):
        ok, msg = skip_count(PLUGIN, p, b0)
        verdict("skip", ok, f"Skip {100 * p:.0f} %: {msg}")
    control("skip", CTL_ABSENT, "Skip code removed, Skip 50 %", lambda pl: skip_count(pl, 0.5, b0))


# -------------------------------------------------------------------------------- fm
def fm_line(pl, ratio):
    y = mono(render(pl, dict(formant=4.0, fm_depth=60.0, fm_ratio=ratio), [57], 2.5, 48000))[48000:]
    return line_db(y, 110.0), y


def fm_dev(pl, ratio, base):
    y = mono(render(pl, dict(formant=4.0, fm_depth=60.0, fm_ratio=ratio), [57], 2.5, 48000))[48000:]
    X, _ = W.spectrum(y, 48000); B, _ = W.spectrum(base, 48000)
    return db(np.max(np.abs(X - B)) / B.max())


def check_fm():
    base = mono(render(PLUGIN, dict(formant=4.0), [57], 2.5, 48000))[48000:]
    l0 = line_db(base, 110.0)
    for ratio in (0.5, 1.5):
        h, y = fm_line(PLUGIN, ratio)
        fq = W.peak_near(y, 48000, 110.0, 2.0)
        verdict("fm-line", h > -40.0 and l0 < -80.0 and abs(W.cents(fq, 110.0)) <= 0.1,
                f"ratio {ratio:g} depth 60 %: f_p/2 line {h:.1f} dB re peak (bound > -40; no FM {l0:.1f}, bound < -80) "
                f"at {fq:.4f} Hz ({W.cents(fq, 110.0):+.3f} cent, bound 0.1)")
    control("fm-line", CTL_ABSENT, "FM code removed, ratio 0.5", lambda pl: ((h := fm_line(pl, 0.5)[0]) > -40.0, f"f_p/2 line {h:.1f} dB"))
    for ratio in (1.0, 2.0):
        d = fm_dev(PLUGIN, ratio, base)
        verdict("fm-inert", d <= -100.0, f"ratio {ratio:g} depth 60 %: spectrum = no-FM spectrum within {d:.1f} dB "
                "(bound -100; integer ratio: onsets stay 1/f_p apart)")
    control("fm-inert", PLUGIN, "ratio 1.01 (onsets no longer 1/f_p apart)",
            lambda pl: ((d := fm_dev(pl, 1.01, base)) <= -100.0, f"{d:.1f} dB"))
    # The onset solve under FM, harness-side (P7): an integer-ratio spectrum cannot
    # see a solver error (every onset lands at the same modulator phase, so it is one
    # constant shift), so the solve is checked directly against bisection.
    if HARNESS is None:
        verdict("fm-crossing", False, "harness P7: --harness not supplied")
    else:
        rc, lines = harness(HARNESS, "P7")
        for ln in lines:
            print("     harness " + ln)
        p7 = [ln for ln in lines if " P7 fm-crossing" in ln]
        ctl = [ln for ln in lines if " P7 control" in ln]
        verdict("fm-crossing", rc == 0 and len(p7) == 1 and p7[0].startswith("[PASS]"),
                "harness P7: fmCrossingDelta vs bisection <= 1e-7 sample over dm 0.005..1")
        verdict("fm-crossing", len(ctl) == 1 and ctl[0].startswith("[PASS]"),
                "CONTROL pre-fix 3-step Newton (in harness P7) exceeds 1e-7 sample -> "
                + ("FAILS the bound as required" if ctl and ctl[0].startswith("[PASS]") else "control BROKEN"))
    yf = mono(render(PLUGIN, dict(formant=8.0, fm_depth=95.0, fm_ratio=0.37), [33], 4.0, 48000))
    yn = mono(render(PLUGIN, dict(formant=8.0), [33], 4.0, 48000))
    nf, nn = len(W.bursts(yf)), len(W.bursts(yn))
    print(f"INFO [fm] depth 95 % ratio 0.37: {nf} pulses vs {nn} without FM in 4 s (mean rate kept)")


# --------------------------------------------------------------------------- glisson
def glisson_x(pl):
    def first_zc(g):
        y = mono(render(pl, dict(formant=32.0, glisson=g, grain_pitch=-8.5), [0], 1.2, 48000))
        xs = []
        for a, b in W.bursts(y, merge=4)[1:5]:
            seg = y[a:b]
            i = int(np.where(np.diff(np.signbit(seg)))[0][0])
            xs.append((i + seg[i] / (seg[i] - seg[i + 1])) / 300.0)
        return float(np.median(xs))
    x0 = first_zc(0.0)
    xp, xm = first_zc(12.0) - x0 + 0.5, first_zc(-12.0) - x0 + 0.5
    return abs(xp - 0.4293) < 0.01 and abs(xm - 0.6139) < 0.01, \
        f"half-cycle ends at {xp:.4f} L for +12 st (0.4293), {xm:.4f} L for -12 st (0.6139); bound +-0.01 L"


def check_glisson():
    ok, msg = glisson_x(PLUGIN)
    verdict("glisson", ok, msg)
    control("glisson", CTL_ABSENT, "Glisson code removed", glisson_x)


# ----------------------------------------------------------------------- sine-no-src
def no_source(pl, content):
    st = W.load(pl, dict(formant=4.0)).raw_state                     # Fire, one RNG seed
    silent = W.G.with_source(st, "dropped:silence.wav", np.zeros(48000))
    a = render(pl, dict(pulsaret=content), [57], 1.5, 48000, state=st)
    b = render(pl, dict(pulsaret=content), [57], 1.5, 48000, state=silent)
    same = np.array_equal(a, b)
    rms = db(np.sqrt(np.mean(b ** 2)))
    return same and rms > -60.0, f"{content} over a silent dropped source vs over Fire: " \
        f"{'bit-identical' if same else f'differs ({db(np.max(np.abs(a - b))):.1f} dBFS)'}; RMS over silence {rms:.1f} dBFS"


def harness(path, which):
    r = subprocess.run([path, "--pulsar", which], capture_output=True, text=True, timeout=600)
    lines = [ln.strip() for ln in r.stdout.splitlines() if re.match(r"\s*\[(PASS|FAIL|INFO)\]", ln)]
    return r.returncode, lines


def check_no_source():
    ok, msg = no_source(PLUGIN, "Sine")
    verdict("sine-no-src", ok, msg + " (bound: bit-identical, > -60 dBFS)")
    control("sine-no-src", PLUGIN, "Sample content (reads the source)", lambda pl: no_source(pl, "Sample"))
    if HARNESS is None:
        verdict("sine-no-src", False, "harness P2 (NaN-poisoned / no source): --harness not supplied")
        return
    rc, lines = harness(HARNESS, "P2")
    for ln in lines:
        print("     harness " + ln)
    verdict("sine-no-src", rc == 0 and any("[PASS] P2" in ln for ln in lines),
            "harness P2: Sine over a NaN-poisoned source finite + audible, no source audible, Sample over it non-finite (its control)")


# ----------------------------------------------------------------------- mode-switch
def check_mode_switch():
    if HARNESS is None:
        verdict("mode-switch", False, "--harness not supplied")
        return
    rc, lines = harness(HARNESS, "P6")
    for ln in lines:
        print("     harness " + ln)
    p6 = [ln for ln in lines if " P6 " in ln and not ln.startswith("[INFO]")]
    verdict("mode-switch", rc == 0 and len(p6) == 2 and all(ln.startswith("[PASS]") for ln in p6),
            "harness P6 Cloud->Pulsar and Pulsar->Cloud: = old-mode render before, max|S - R| <= -100 dB re peak, "
            "curvature S/R <= 1.01 vs the same-motion model")
    for path, label in ((H_SNAPGRAIN, "the other kind's in-flight grains CUT at the switch"),
                        (H_SHAREDGAIN, "ONE output gain ramped across the switch (pre-fix 2.0.0)")):
        def fn(pl):
            rc2, l2 = harness(pl, "P6")
            q = [ln for ln in l2 if " P6 " in ln and not ln.startswith("[INFO]")]
            for ln in q:
                print("     control " + ln)
            return rc2 == 0 and len(q) == 2 and all(ln.startswith("[PASS]") for ln in q), \
                f"{sum(ln.startswith('[FAIL]') for ln in q)} of {len(q)} P6 directions fail"
        control("mode-switch", path, label, fn)


CHECKS = [("fp", check_fp), ("formant", check_formant), ("alias", check_alias), ("bh-band", check_bh_band),
          ("bh-sidelobe", check_bh_sidelobe), ("frac-onset", check_frac_onset), ("mask", check_mask),
          ("skip", check_skip), ("fm", check_fm), ("glisson", check_glisson), ("sine-no-src", check_no_source),
          ("mode-switch", check_mode_switch)]


def main():
    print(f"plugin  {PLUGIN}\nceiling {CTL_CEIL}\nabsent  {CTL_ABSENT}\nnobh    {CTL_NOBH}\nint     {CTL_INT}\n"
          f"harness {HARNESS}\nsnapgrain {H_SNAPGRAIN}\nsharedgain {H_SHAREDGAIN}")
    for name, fn in CHECKS:
        if want(name):
            print(f"\n== {name}", flush=True)
            fn()
    print("\nSUMMARY")
    for name in dict.fromkeys(c for c, _ in results):
        rs = [ok for c, ok in results if c == name]
        print(f"  {name:12s} {'PASS' if all(rs) else 'FAIL'} ({sum(rs)}/{len(rs)})")
    print("\nPULSAR GATE", "PASS" if fails == 0 else f"FAIL ({fails})")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
