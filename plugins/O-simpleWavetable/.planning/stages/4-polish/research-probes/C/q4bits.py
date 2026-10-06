import numpy as np
from wt_model import *
def quant(x, bits):
    M=2**(bits-1); y=np.abs(x)*np.float32(M); k=np.where(y<M-1, np.floor(y), M-1)
    q=((k+0.5)/M).astype(np.float32); return np.where(x<0,-q,q).astype(np.float32)
sq=[build_levels(*sine_phase_spectrum(square_frame(k))) for k in range(1,33)]
saw=[build_levels(*sine_phase_spectrum(saw_frame(k))) for k in range(1,33)]
fs=48000.0
for name,tb,note in (("Sine->Square f19 A3",sq[19-1],57),("Sine->Saw f32 A4",saw[31],69),("Sine->Square f1 (sine) A4",sq[0],69)):
    x,_=render(tb,midi_hz(note),fs,48000)
    ps=np.mean(x.astype(np.float64)**2); prev=None; row=[]
    for b in range(16,2,-1):
        e=quant(x,b).astype(np.float64)-x; snr=10*np.log10(ps/np.mean(e**2))
        row.append(snr)
    d=np.diff(row)
    print(f"{name}: peak {np.abs(x).max():.3f} rms {np.sqrt(ps):.3f}")
    print("  SNR 16..3:", " ".join(f"{v:5.1f}" for v in row))
    print("  step dB  :", " ".join(f"{-v:5.2f}" for v in d), " min step %.2f max %.2f"%(-d.max(), -d.min()))
    print("  theory 6.02b+10log10(3*ps):", " ".join(f"{6.0206*b+10*np.log10(3*ps):5.1f}" for b in range(16,2,-1)))
