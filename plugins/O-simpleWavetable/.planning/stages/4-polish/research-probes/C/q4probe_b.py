import numpy as np
from wt_model import *
from q4probe import render_scan
saw = [build_levels(*sine_phase_spectrum(saw_frame(k))) for k in range(1,33)]
def metric(y, P, fs, secs, lo, hi):
    ns=len(y); C = y[:(ns//P)*P].reshape(-1,P).astype(np.float64)
    d = np.sum(np.diff(C,axis=0)**2,axis=1); t=(np.arange(len(d))+1)*P/fs
    m=(t>lo*secs)&(t<hi*secs); return d[m]
fs=56320.0; P=128
# (1) render 5 s so the saw LFO resets at 4 s: include the reset
for interp in (False,True):
    y,_=render_scan(saw,440.0,fs,int(5*fs),interp)
    globals()['w%d'%interp]=metric(y,P,fs,5.0,0.0,1.0)
print("incl. LFO saw reset: ratio Off/On = %.2f (On max step %.3f)"%(w0.sum()/w1.sum(), np.sqrt(w1.max())))
# (2) negative control Off vs Off
y0,_=render_scan(saw,440.0,fs,int(4*fs),False); a=metric(y0,P,fs,4.0,0.05,0.95)
print("NC Off/Off = %.3f"%(a.sum()/a.sum()))
# (3) lower pitch: A3 220 Hz period 256 at 56320
for interp in (False,True):
    y,_=render_scan(saw,220.0,fs,int(4*fs),interp)
    globals()['l%d'%interp]=metric(y,256,fs,4.0,0.05,0.95)
print("A3: ratio Off/On = %.1f"%(l0.sum()/l1.sum()))
# (4) vacuous stimulus: LFO depth 0 -> both static
