import numpy as np, eqmodel as m, polish as P, time
PI=np.pi; FREE6=[0,1,3,4,5,7]
def tied(fc,Q,g,scales,fmax):
    A=10**(np.abs(g)/40); pts=[fc]
    for q in (Q/A,Q,Q*A):
        for s in scales:
            h=s/(2*q); pts += [fc*(np.sqrt(1+h*h)+h), fc*(np.sqrt(1+h*h)-h)]
    return np.clip(np.stack(pts,-1),20,fmax)
def an(wa,proto):
    pz=[np.asarray(x)[...,None]*np.ones_like(wa) for x in proto]
    return 10*np.log10(((pz[0]**2-wa**2)**2+(pz[0]*wa/pz[1])**2)/((pz[2]**2-wa**2)**2+(pz[2]*wa/pz[3])**2))
def run(fs,fc,Q,g,M,scales,K,lam=1e-3,free=FREE6,ne=1200,wtpow=0.0,chunk=20000):
    """g>=0 arrays. returns e_seed, e_polish (dense + tied eval, 20 Hz - min(20k, Nyq))"""
    fmax=min(20000,0.499*fs); fe=np.geomspace(20,fmax,ne); E0=[];E1=[]
    for a in range(0,len(fc),chunk):
        s=slice(a,a+chunk); f_,Q_,g_=fc[s],Q[s],g[s]; wc=2*PI*f_/fs; B=len(f_)
        proto=m.bell_proto(wc,Q_,g_)
        p0,_=m.bell_design(wc,Q_,g_); phi0=np.log(np.stack(p0,-1))
        fa=np.concatenate([np.broadcast_to(np.geomspace(20,fmax,M),(B,M)),tied(f_,Q_,g_,scales,fmax)],-1); wa=2*PI*fa/fs
        ph=P.fixed(phi0,free,np.tan(wa/2)**2,an(wa,proto),np.ones_like(wa),K,lam)
        fd=np.concatenate([np.broadcast_to(fe,(B,ne)),tied(f_,Q_,g_,(0.1,0.25,0.5,0.75,1.,1.5,2.,3.),fmax)],-1); wd=2*PI*fd/fs; td=np.tan(wd/2)**2; ad=an(wd,proto)
        E0.append(np.abs(P.resp_jac(phi0,td,False)[0]-ad).max(-1)); E1.append(np.abs(P.resp_jac(ph,td,False)[0]-ad).max(-1))
    return np.concatenate(E0),np.concatenate(E1)
if __name__=="__main__":
    fcs=np.geomspace(20,20000,40); Qs=np.geomspace(0.025,40,17); FC,QQ=np.meshgrid(fcs,Qs,indexing='ij'); fc=FC.ravel(); Q=QQ.ravel()
    print("A. G1 grid, 44.1 kHz: max err at 3/6/12/18/30 dB [worse-than-seed count]")
    for name,M,sc,K in [("M16+t19 K1",16,(.5,1.,2.),1),("M16+t19 K2",16,(.5,1.,2.),2),("M16+t13 K1",16,(.5,1.5),1),("M16+t13 K2",16,(.5,1.5),2),("M16+t7 K2",16,(1.,),2),
                        ("M8+t13 K2",8,(.5,1.5),2),("M12+t13 K2",12,(.5,1.5),2),("M24+t13 K2",24,(.5,1.5),2),("M32+t13 K2",32,(.5,1.5),2),("M24+t19 K1",24,(.5,1.,2.),1)]:
        r=[]
        for g in (3.,6.,12.,18.,30.):
            e0,e1=run(44100.,fc,Q,np.full_like(fc,g),M,sc,K); r.append(f"{np.nanmax(e1):.3f}[{((e1>e0+1e-6)|~np.isfinite(e1)).sum()}]")
        print(f"  {name:12s}", " ".join(r))
    print("B. other rates, M16+t13 K2 and K1: seed -> polish max err at 6/12/30 dB")
    for fs in (48000.,88200.,96000.,192000.):
        for K in (1,2):
            r=[]
            for g in (6.,12.,30.):
                e0,e1=run(fs,fc,Q,np.full_like(fc,g),16,(.5,1.5),K); r.append(f"{e0.max():.3f}->{np.nanmax(e1):.3f}[{((e1>e0+1e-6)|~np.isfinite(e1)).sum()}]")
            print(f"  {fs:.0f} K{K}", " ".join(r))
    print("C. random sweep, full ranges: fc 10 Hz-30 kHz (log), Q 0.025-40 (log), gain 0-60 dB; 60k settings per rate")
    rng=np.random.default_rng(1)
    for fs in (22050.,32000.,44100.,48000.,96000.,192000.):
        n=60000; f_=np.exp(rng.uniform(np.log(10),np.log(30000),n)); Q_=np.exp(rng.uniform(np.log(0.025),np.log(40),n)); g_=rng.uniform(0,60,n)
        for K in (1,2):
            e0,e1=run(fs,f_,Q_,g_,16,(.5,1.5),K,ne=500)
            bad=(e1>e0+1e-3)|~np.isfinite(e1); lo=g_<=30
            i=np.nanargmax(np.where(bad,e1-e0,-1))
            print(f"  {fs:.0f} K{K}: nonfinite {(~np.isfinite(e1)).sum()}  worse-than-seed {bad.sum()} (worst +{(e1-e0)[i]:.3f} dB at fc {f_[i]:.0f} Q {Q_[i]:.3f} g {g_[i]:.1f}); max seed/polish |g|<=30: {e0[lo].max():.2f}/{np.nanmax(e1[lo]):.2f}; all: {e0.max():.2f}/{np.nanmax(e1):.2f}")
