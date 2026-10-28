import numpy as np
OS=64; NFFT=1<<18
def kaiser_sinc(T,fc,beta):
    t=np.arange(-T/2*OS,T/2*OS+1)/OS
    w=np.i0(beta*np.sqrt(np.clip(1-(2*t/T)**2,0,1)))/np.i0(beta)
    return t,2*fc*np.sinc(2*fc*t)*w
def lagrange4():
    t=np.arange(-2*OS,2*OS+1)/OS; x=np.abs(t)
    h=np.where(x<1,(x-1)*(x+1)*(x-2)/2,np.where(x<2,-(x-1)*(x-2)*(x-3)/6,0)); return t,h
def resp(t,h):
    X=np.abs(np.fft.rfft(h,NFFT)); f=np.arange(len(X))*OS/NFFT; return f,X/X[0]
def worst_alias(t,h,r,a,fmax):
    f,X=resp(t,h); f0=np.linspace(0.002,fmax,300); worst=-300
    for m in range(-9,10):
        ff=np.abs(f0+m); g=np.interp(ff,f,X)
        out=np.abs(((ff*r+0.5)%1)-0.5)
        mask=(out<a)&~((m==0)&(f0*r<=0.5))
        if m==0: pass
        if mask.any(): worst=max(worst,20*np.log10(g[mask].max()+1e-30))
    return worst
def pb(t,h,r,drop=1):
    f,X=resp(t,h); i=np.argmax(20*np.log10(X+1e-30)<-drop); return f[i]*max(r,1)
