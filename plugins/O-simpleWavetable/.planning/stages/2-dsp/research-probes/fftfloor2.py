import numpy as np
from wt_model import *
def metric(y, f, fs, dt):
    M=len(y); w=np.kaiser(M,38.0).astype(dt); Y=np.abs(np.fft.rfft((y.astype(dt)*w)))
    print("  dtype", Y.dtype, end="")
    fr=np.arange(len(Y))*fs/M; g=14*fs/M; h=fr<=g
    for k in range(1,int((fs/2)//f)+1): h|=np.abs(fr-k*f)<=g
    return 20*np.log10(Y[~h].max()/Y[h].max())
tb = build_levels(*drive_frame_spectrum(32))
for fs in (44100.0, 96000.0):
    y,L = render(tb, midi_hz(108), fs, 1<<17)
    print(fs, "f64 %.1f" % metric(y, midi_hz(108), fs, np.float64), " f32 %.1f" % metric(y, midi_hz(108), fs, np.float32))
