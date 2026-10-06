import numpy as np
from wt_model import *
lv = build_levels(*pulse_frame_spectrum(32))
for L in (0, 5, 8, 10):
    t = lv[L][:N]
    X32 = np.abs(np.fft.rfft(t.astype(np.float32)))  # numpy>=2: float32 path
    X64 = np.abs(np.fft.rfft(t.astype(np.float64)))
    print(L, X32.dtype, "f32 above Kmax %.1f dB" % (20*np.log10(X32[KMAX[L]+1:].max()/X32.max())) if KMAX[L]<1024 else "", " f64 %.1f" % (20*np.log10(X64[KMAX[L]+1:].max()/X64.max()+1e-300)) if KMAX[L]<1024 else "")
print(np.__version__)
