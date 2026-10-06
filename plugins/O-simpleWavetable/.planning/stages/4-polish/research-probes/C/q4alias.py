import numpy as np
from wt_model import *
def metrics(y, f, fs, band_hi=None):
    M=len(y); w=np.kaiser(M,38.0); Y=np.abs(np.fft.rfft(y.astype(np.float64)*w))
    fr=np.arange(len(Y))*fs/M; g=14*fs/M
    harm=fr<=g
    for k in range(1,int((fs/2)//f)+1): harm|=np.abs(fr-k*f)<=g
    peak=Y[harm].max()
    inh=~harm
    full=20*np.log10(Y[inh].max()/peak)
    sel = inh & (fr <= band_hi) if band_hi else inh
    band=20*np.log10(Y[sel].max()/peak) if sel.any() else -999
    # alias-to-signal power ratio (all inharmonic bins vs harmonic bins)
    asr=10*np.log10((Y[inh]**2).sum()/(Y[harm]**2).sum())
    asr_b=10*np.log10((Y[sel]**2).sum()/(Y[harm]**2).sum())
    wf=fr[inh][np.argmax(Y[inh])]
    return full, wf, band, asr, asr_b
d32 = build_levels(*drive_frame_spectrum(32))
s32 = build_levels(*sine_phase_spectrum(saw_frame(32)))
for name,tb in (("Drive32",d32),("Saw32(Stage3 recipe)",s32)):
  print("==",name, "  [full-band max inh rel peak harm @Hz | max inh <= 8 kHz | ASR all | ASR <= 8k]")
  for fs in (44100.0,48000.0,88200.0,96000.0):
    for note in (84,96,108):
        f=midi_hz(note)
        yo,_=render(tb,f,fs,1<<17,bandlimit=False); yn,L=render(tb,f,fs,1<<17,bandlimit=True)
        mo=metrics(yo,f,fs,8000.0); mn=metrics(yn,f,fs,8000.0)
        print(f"fs {fs:6.0f} MIDI {note}: Off {mo[0]:6.1f} dB @{mo[1]:7.0f} | <=8k {mo[2]:6.1f} | ASR {mo[3]:6.1f} | ASR8k {mo[4]:6.1f}   On(L{L}) {mn[0]:6.1f} | {mn[2]:6.1f} | ASR {mn[3]:6.1f}   contrast {mo[0]-mn[0]:5.1f} dB")
