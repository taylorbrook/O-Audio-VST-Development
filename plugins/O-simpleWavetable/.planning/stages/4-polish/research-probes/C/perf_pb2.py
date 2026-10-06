import numpy as np, time
from perf_pb import load, NOTES
from mido import Message
def chunked(fs, toggle, setsame=False, bs=512, dur=2.0, worst=True):
    p=load(worst); on=[Message('note_on',note=n,velocity=100,time=0.0) for n in NOTES]
    p(on, duration=0.5, sample_rate=fs, num_channels=2, buffer_size=bs, reset=True)
    nblk=int(dur*fs/bs)
    t0=time.perf_counter()
    for b in range(nblk):
        if toggle: p.parameters['band_limiting'].raw_value = float(b & 1)
        elif setsame: p.parameters['band_limiting'].raw_value = 1.0
        y=p([], duration=bs/fs, sample_rate=fs, num_channels=2, buffer_size=bs, reset=False)
    return (time.perf_counter()-t0)/dur, float(np.sqrt(np.mean(y**2)))
for fs in (44100.0, 96000.0):
    a=chunked(fs, False); b=chunked(fs, False, True); c=chunked(fs, True)
    print(f"fs {fs:.0f}: per-call only {100*a[0]:.2f}%  | +set same value {100*b[0]:.2f}% | +toggle (xfade every block) {100*c[0]:.2f}%  rms {a[1]:.3f}/{c[1]:.3f}")
# single note vs 16 notes rms (all voices sounding?)
import pedalboard
p=load(True); y1=p([Message('note_on',note=84,velocity=100,time=0.0)],duration=1.0,sample_rate=48000.0,num_channels=2,buffer_size=512,reset=True)
p=load(True); y16=p([Message('note_on',note=n,velocity=100,time=0.0) for n in NOTES],duration=1.0,sample_rate=48000.0,num_channels=2,buffer_size=512,reset=True)
print("rms 1 note %.4f, 16 notes %.4f (ratio %.2f, sqrt(16)=4 if uncorrelated)"%(np.sqrt(np.mean(y1[:,24000:]**2)),np.sqrt(np.mean(y16[:,24000:]**2)),np.sqrt(np.mean(y16[:,24000:]**2))/np.sqrt(np.mean(y1[:,24000:]**2))))
