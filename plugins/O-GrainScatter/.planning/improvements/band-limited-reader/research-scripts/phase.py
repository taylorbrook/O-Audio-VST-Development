import numpy as np
T=16;beta=6.0;fc=0.5*0.84
def k(fr):  # taps for read at i0+fr, taps i0-7..i0+8
    t=np.arange(-7,9)-fr
    w=np.i0(beta*np.sqrt(np.clip(1-(2*t/T)**2,0,1)))/np.i0(beta)
    h=2*fc*np.sinc(2*fc*t)*w; return h/h.sum()
for P in [64,128,256,512]:
    tab=np.array([k(p/P) for p in range(P+1)])
    err=0
    for fr in np.linspace(0,1,2001):
        x=fr*P; i=min(int(x),P-1); u=x-i
        e=np.abs((1-u)*tab[i]+u*tab[i+1]-k(fr)).sum(); err=max(err,e)
    print(f"P={P}: worst L1 tap error {20*np.log10(err):.1f} dB (bound on output error, full-scale), table {P+1}x16 floats/set = {(P+1)*16*4/1024:.0f} KB/set")
