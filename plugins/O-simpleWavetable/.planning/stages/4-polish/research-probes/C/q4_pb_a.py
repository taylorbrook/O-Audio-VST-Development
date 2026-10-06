import numpy as np
from q4_pb import plug
from mido import Message
fs=56320.0; P=128; res={}
for it in (0.0,1.0):
    p=plug(bank=0.0, position=0.5, interpolation=it, lfo_shape=0.5, lfo_rate_hz=((0.25-0.01)/19.99)**0.3, lfo_depth=1.0, lfo_sync=0.0)
    p([], duration=0.5, sample_rate=fs, num_channels=2, buffer_size=128, reset=True)   # LFO runs 0.5 s before the note -> reset at note+3.5 s
    y=p([Message('note_on',note=69,velocity=100,time=0.0)], duration=4.0, sample_rate=fs, num_channels=2, buffer_size=128, reset=False)[0].astype(np.float64)
    C=y[:(len(y)//P)*P].reshape(-1,P); d=np.sum(np.diff(C,axis=0)**2,axis=1); t=(np.arange(len(d))+1)*P/fs
    res[it]=(d,t)
for lo,hi in ((1.0,3.4),(1.0,3.8),(0.2,3.4)):
    e={}
    for it in (0.0,1.0):
        d,t=res[it]; m=(t>lo)&(t<hi); e[it]=(d[m].sum(), int((d[m]>1e-3*d[m].max()).sum()), np.sqrt(d[m].max()))
    print(f"window [{lo},{hi}]: Off steps {e[0.0][1]}, ratio Off/On {e[0.0][0]/e[1.0][0]:.1f}, peak ratio {e[0.0][2]/e[1.0][2]:.1f}")
