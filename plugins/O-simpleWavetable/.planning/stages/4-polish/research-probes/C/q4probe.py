import numpy as np, time
from wt_model import *

def latch(p, nF): return int(np.clip(np.round(np.clip(p,0,1)*(nF-1)), 0, nF-1))

def render_scan(frames, f, fs, nsamp, interp, lfo_hz=0.25, L=None, tau=0.002, shape="saw"):
    """Processor-ish model: raw = clamp(0.5 + 0.5*lfo), Saw LFO from phase 0; Interp On: 2 ms one-pole
    smoother + frame lerp; Off: latched at note-on + each wrap. Single voice, no env."""
    nF = len(frames)
    if L is None:
        x = f*N/fs; L = int(np.clip(np.ceil(np.log2(x)), 1, LEVELS-1))
    tabs = np.stack([fr[L] for fr in frames]).astype(np.float32)   # [nF][2049]
    inc = f/fs; ph = 0.0
    lp = 0.0
    a = 1.0 - np.exp(-1.0/(tau*fs))
    y = np.zeros(nsamp, np.float32)
    raw_arr = np.zeros(nsamp)
    for n in range(nsamp):
        lph = (lp % 1.0)
        if shape == "saw": lv = 2*lph - 1
        else: lv = (4*lph if lph<0.25 else (2-4*lph if lph<0.75 else 4*lph-4))
        lp += lfo_hz/fs
        raw = min(1.0, max(0.0, 0.5 + 0.5*lv)); raw_arr[n]=raw
        if n == 0:
            eff = raw; lf = latch(raw, nF)
        if interp:
            eff = eff + a*(raw-eff)
        idx = ph*N; i0 = min(int(idx), N-1); t = np.float32(idx - i0)
        if interp:
            fp = np.float32(eff*(nF-1)); f0 = min(int(fp), nF-1); f1 = min(f0+1, nF-1)
            s0 = tabs[f0,i0] + t*(tabs[f0,i0+1]-tabs[f0,i0]); s1 = tabs[f1,i0] + t*(tabs[f1,i0+1]-tabs[f1,i0])
            s = s0 + (fp-f0)*(s1-s0)
        else:
            s = tabs[lf,i0] + t*(tabs[lf,i0+1]-tabs[lf,i0])
        y[n] = s
        ph += inc
        if ph >= 1.0:
            ph -= 1.0
            if not interp: lf = latch(raw, nF)
    return y, raw_arr

if __name__ == "__main__":
    t0=time.time()
    saw = [build_levels(*sine_phase_spectrum(saw_frame(k))) for k in range(1,33)]
    fs = 56320.0; f = 440.0; P = 128
    secs = 4.0; ns = int(secs*fs)      # one full LFO ramp (0.25 Hz) from phase 0
    res = {}
    for interp in (False, True):
        y, raw = render_scan(saw, f, fs, ns, interp)
        C = y[: (ns//P)*P].reshape(-1, P).astype(np.float64)
        d = np.sum(np.diff(C, axis=0)**2, axis=1)       # per-boundary step energy
        # interior of ramp only: LFO phase 0.05..0.95
        k = np.arange(len(d)); tsec = (k+1)*P/fs
        m = (tsec > 0.05*secs) & (tsec < 0.95*secs)
        res[interp] = (d[m], np.mean(C**2))
        nz = int(np.sum(d[m] > 1e-12))
        print(f"interp={interp}: cycles {len(d)}, interior {m.sum()}, nonzero steps {nz}, sum E {d[m].sum():.4e}, max step {np.sqrt(d[m].max()):.4e}, cycle power {np.mean(C**2):.4f}")
    Eoff, Eon = res[False][0].sum(), res[True][0].sum()
    print(f"G-Q4-STEP energy ratio Off/On = {Eoff/Eon:.1f}  ({10*np.log10(Eoff/Eon):.1f} dB); peak ratio = {np.sqrt(res[False][0].max()/res[True][0].max()):.1f}")
    # normalised: per-step energy relative to cycle energy
    print(f"  Off sum step energy / cycle energy*P = {Eoff/(res[False][1]*P):.3f}")
    # whole-render (incl. saw LFO reset) to show the pitfall
    print("time %.1fs" % (time.time()-t0))
