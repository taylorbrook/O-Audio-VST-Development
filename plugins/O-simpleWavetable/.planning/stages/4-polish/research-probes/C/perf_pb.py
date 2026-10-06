import pedalboard, numpy as np, time, sys, os
from mido import Message
PATH="/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3"
def load(worst=True):
    p = pedalboard.load_plugin(PATH)
    P = p.parameters
    def s(k,v): P[k].raw_value = v
    if worst:
        s('interpolation',1.0); s('band_limiting',1.0); s('bit_depth',1.0)   # 3 bits (idx 14/14)
        s('lfo_shape',0.5); s('lfo_depth',1.0); s('lfo_rate_hz',0.8)
        s('env_amount',1.0); s('mod_env_sustain',0.5)
        s('amp_sustain',1.0); s('amp_attack_s',0.0); s('position',0.5)
    s('voice_mode',0.0)
    return p
NOTES=list(range(84,100))   # 16 high notes C6..D#7
def run(fs, dur=4.0, bs=512, worst=True, storm=False, reps=5):
    p=load(worst)
    on=[Message('note_on',note=n,velocity=100,time=0.0) for n in NOTES]
    # warm-up 0.5 s, notes start; keep held (reset=False afterwards)
    p(on, duration=0.5, sample_rate=fs, num_channels=2, buffer_size=bs, reset=True)
    res=[]
    for r in range(reps):
        if not storm:
            c0=time.process_time(); t0=time.perf_counter()
            y=p([], duration=dur, sample_rate=fs, num_channels=2, buffer_size=bs, reset=False)
            t1=time.perf_counter(); c1=time.process_time()
        else:
            nblk=int(dur*fs/bs); y=None
            c0=time.process_time(); t0=time.perf_counter()
            for b in range(nblk):
                p.parameters['band_limiting'].raw_value = float(b & 1)   # crossfade trigger every block
                y=p([], duration=bs/fs, sample_rate=fs, num_channels=2, buffer_size=bs, reset=False)
            t1=time.perf_counter(); c1=time.process_time()
        assert np.isfinite(y).all()
        res.append(((t1-t0)/dur, (c1-c0)/max(t1-t0,1e-9), float(np.abs(y).max())))
    fr=np.array([r[0] for r in res])
    return np.median(fr), fr.min(), fr.max(), np.median([r[1] for r in res]), res[-1][2]
if __name__=="__main__":
    print("host:", os.uname().machine)
    for storm in (False, True):
        for fs in (44100.0,48000.0,88200.0,96000.0):
            med,mn,mx,duty,pk=run(fs, storm=storm)
            print(f"{'storm' if storm else 'steady'} fs {fs:6.0f} bs 512: RT fraction median {100*med:5.2f}% (min {100*mn:5.2f}, max {100*mx:5.2f}); duty {100*duty:4.0f}%; peak {pk:.3f}")
    med,mn,mx,duty,pk=run(96000.0, worst=False)
    print(f"default-patch 16 voices fs 96000: median {100*med:5.2f}%  duty {100*duty:.0f}%")
    for bs in (64,128,1024):
        med,mn,mx,duty,pk=run(96000.0, bs=bs)
        print(f"steady fs 96000 bs {bs}: median {100*med:5.2f}% (min {100*mn:.2f})")
