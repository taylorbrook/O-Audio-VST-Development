import numpy as np
from wt_model import *
from q4probe import render_scan
saw=[build_levels(*sine_phase_spectrum(saw_frame(k))) for k in range(1,33)]
fs=56320.0;P=128;ns=int(4*fs)
def env(n):  # JUCE-ish linear ADSR A 5ms D 300ms S 0.8
    t=np.arange(n)/fs; a=np.minimum(t/0.005,1.0); d=np.clip((t-0.005)/0.3,0,1); return np.where(t<0.005,a,1-0.2*d)
out={}
for interp in (False,True):
    y,_=render_scan(saw,440.0,fs,ns,interp)
    for useenv in (False,True):
        z=(y*env(ns)) if useenv else y
        C=z[:(ns//P)*P].reshape(-1,P).astype(np.float64); d=np.sum(np.diff(C,axis=0)**2,axis=1); t=(np.arange(len(d))+1)*P/fs
        for lo,hi in ((0.2,3.8),(1.0,3.8)):
            m=(t>lo)&(t<hi); out[(interp,useenv,lo)]=(d[m].sum(), int((d[m]>1e-6*d[m].max()).sum()) if not interp else int(m.sum()))
for useenv in (False,True):
    for lo in (0.2,1.0):
        a=out[(False,useenv,lo)]; b=out[(True,useenv,lo)]
        print(f"env={useenv} window [{lo},3.8]s: Off steps {a[1]}, ratio Off/On {a[0]/b[0]:.1f}")
