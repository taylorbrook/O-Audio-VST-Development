import numpy as np
from wt_model import *
def i0nr(x):
    ax=abs(x)
    if ax<3.75:
        y=(x/3.75)**2; return 1+y*(3.5156229+y*(3.0899424+y*(1.2067492+y*(0.2659732+y*(0.360768e-1+y*0.45813e-2)))))
    y=3.75/ax
    return (np.exp(ax)/np.sqrt(ax))*(0.39894228+y*(0.1328592e-1+y*(0.225319e-2+y*(-0.157565e-2+y*(0.916281e-2+y*(-0.2057706e-1+y*(0.2635537e-1+y*(-0.1647633e-1+y*0.392377e-2))))))))
M=1<<17; beta=38.0
n=np.arange(M); arg=beta*np.sqrt(1-((n-0.5*(M-1))/(0.5*(M-1)))**2)
wj=np.array([i0nr(a) for a in arg])/i0nr(beta)
def met(y,f,fs,w):
    Y=np.abs(np.fft.rfft(y.astype(np.float64)*w)); fr=np.arange(len(Y))*fs/M; g=14*fs/M; h=fr<=g
    for k in range(1,int((fs/2)//f)+1): h|=np.abs(fr-k*f)<=g
    return 20*np.log10(Y[~h].max()/Y[h].max())
fs=44100.0
s=np.sin(2*np.pi*1000.3*n/fs).astype(np.float32)
print("sine floor: numpy kaiser %.1f  JUCE-NR-I0 kaiser %.1f" % (met(s,1000.3,fs,np.kaiser(M,beta)), met(s,1000.3,fs,wj)))
tb=build_levels(*drive_frame_spectrum(32)); y,L=render(tb,midi_hz(108),fs,M)
print("drive32 C8: numpy %.1f  JUCE-NR %.1f" % (met(y,midi_hz(108),fs,np.kaiser(M,beta)), met(y,midi_hz(108),fs,wj)))
