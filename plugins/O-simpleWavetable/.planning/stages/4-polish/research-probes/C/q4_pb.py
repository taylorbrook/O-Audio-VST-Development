import pedalboard, numpy as np
from mido import Message
PATH="/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3"
def plug(**kw):
    p=pedalboard.load_plugin(PATH)
    for k,v in kw.items(): p.parameters[k].raw_value=v
    return p
def render(p, notes, fs, dur, vel=100):
    p([], duration=0.5, sample_rate=fs, num_channels=2, buffer_size=512, reset=True)  # settle
    y=p([Message('note_on',note=n,velocity=vel,time=0.0) for n in notes], duration=dur, sample_rate=fs, num_channels=2, buffer_size=512, reset=False)
    assert np.isfinite(y).all(); return y[0].astype(np.float64)
def mhz(n): return 440*2**((n-69)/12)
def alias(y,f,fs,hi=8000.0):
    M=1<<17; x=y[-M:]; w=np.kaiser(M,38.0); Y=np.abs(np.fft.rfft(x*w)); fr=np.arange(len(Y))*fs/M; g=14*fs/M
    h=fr<=g
    for k in range(1,int((fs/2)//f)+1): h|=np.abs(fr-k*f)<=g
    pk=Y[h].max(); sel=(~h)&(fr<=hi)
    return 20*np.log10(Y[~h].max()/pk), 20*np.log10(Y[sel].max()/pk)
if __name__=="__main__":
    print("== (b) alias, installed VST3 [full-band | <=8 kHz] dB rel max harmonic")
    for bankraw,name in ((0.8,"Drive32"),(0.0,"Saw32")):
      for fs in (44100.0,48000.0,88200.0,96000.0):
        row=[]
        for note in (84,96,108):
            r={}
            for bl in (0.0,1.0):
                p=plug(bank=bankraw, position=1.0, band_limiting=bl, amp_sustain=1.0)
                y=render(p,[note],fs,0.05+(1<<17)/fs+0.05)
                r[bl]=alias(y,mhz(note),fs)
            row.append(f"MIDI{note}: Off {r[0.0][0]:6.1f}|{r[0.0][1]:6.1f} On {r[1.0][0]:6.1f}|{r[1.0][1]:6.1f}")
        print(f"{name} fs {fs:6.0f}: "+"  ".join(row))
    print("== (c) SNR per bit, Sine->Saw pos 1 A4 48k, installed VST3")
    fs=48000.0
    ref=render(plug(bank=0.0,position=1.0,bit_depth=0.0,amp_sustain=1.0),[69],fs,1.0)[12000:]
    snr=[]
    for idx in range(1,15):
        y=render(plug(bank=0.0,position=1.0,bit_depth=idx/14.0,amp_sustain=1.0),[69],fs,1.0)[12000:]
        e=y-ref; snr.append(10*np.log10(np.sum(ref**2)/np.sum(e**2)) if np.any(e) else np.inf)
    print("bits 16..3:", " ".join(f"{v:5.1f}" for v in snr)); d=-np.diff(snr); print("steps:", " ".join(f"{v:4.2f}" for v in d))
    print("slope fit (dB/bit):", np.polyfit(np.arange(16,2,-1), snr, 1)[0])
    print("== (a) step energy at fs 56320 A4, LFO Saw 0.25 Hz depth 1, pos 0.5 (default amp env)")
    fs=56320.0; P=128
    E={}
    for it in (0.0,1.0):
        p=plug(bank=0.0, position=0.5, interpolation=it, lfo_shape=0.5, lfo_rate_hz=((0.25-0.01)/19.99)**0.3, lfo_depth=1.0, lfo_sync=0.0)
        p([], duration=0.5, sample_rate=fs, num_channels=2, buffer_size=128, reset=True)
        y=p([Message('note_on',note=69,velocity=100,time=0.0)], duration=4.0, sample_rate=fs, num_channels=2, buffer_size=128, reset=False)[0].astype(np.float64)
        C=y[:(len(y)//P)*P].reshape(-1,P); d=np.sum(np.diff(C,axis=0)**2,axis=1); t=(np.arange(len(d))+1)*P/fs
        m=(t>1.0)&(t<3.8); E[it]=(d[m].sum(), int((d[m]>1e-3*d[m].max()).sum()))
    print("Off steps %d, On nonzero %d, ratio Off/On %.1f"%(E[0.0][1],E[1.0][1],E[0.0][0]/E[1.0][0]))
