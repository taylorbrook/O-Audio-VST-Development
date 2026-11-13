import numpy as np, cand as C, sys
fcs=np.geomspace(20,20000,40); Qs=np.geomspace(0.025,40,17); FC,QQ=np.meshgrid(fcs,Qs,indexing='ij'); gfc=FC.ravel(); gQ=QQ.ravel()
print("BELL, candidate (M16 + 10 tied, K2, mu 0.1, guard blend). G1 grid. max err seed -> polish [-> reference optimum at 44.1/48k]; worse-than-seed count; share polished(l>.99)/blended/seed(l=0)")
for fs in (44100.,48000.,88200.,96000.,192000.):
    for g in (3.,6.,12.,18.,30.):
        phi0,proto=C.bell_seed(fs,gfc,gQ,np.full_like(gfc,g)); info={}
        ph=C.polish(fs,phi0,proto,info=info); e0=C.err(fs,phi0,proto); e1=C.err(fs,ph,proto); l=info['lam']
        ref=f" -> {C.err(fs,C.reference(fs,phi0,proto),proto).max():.3f}" if fs<50000 else ""
        # mirror: cut = invert(polish(boost)); reciprocity checked on dB sum
        d,a=C.eval_db(fs,ph,proto); di,_=C.eval_db(fs,C.invert(ph),proto)
        print(f" {fs:.0f} {g:4.0f} dB: {e0.max():.3f} -> {e1.max():.3f}{ref}   worse {(e1>e0+1e-4).sum()}   l: {(l>.99).mean():.2f}/{((l>0)&(l<=.99)).mean():.2f}/{(l==0).mean():.2f}   recip |boost+inverse| {np.abs(d+di).max():.1e} dB")
rng=np.random.default_rng(7); n=40000
rf=np.exp(rng.uniform(np.log(10),np.log(30000),n)); rQ=np.exp(rng.uniform(np.log(0.025),np.log(40),n)); rg=rng.uniform(0,60,n)
print("random full range (fc 10-30k, Q .025-40, gain 0-60 dB), 40k settings/rate: nonfinite | worse than seed by >1e-3 / >0.05 dB (worst) | max err seed->polish, |g|<=30, fc<=20k | same, fc>20k")
for fs in (22050.,32000.,44100.,48000.,88200.,96000.,176400.,192000.):
    phi0,proto=C.bell_seed(fs,rf,rQ,rg); ph=C.polish(fs,phi0,proto); e0=C.err(fs,phi0,proto,400); e1=C.err(fs,ph,proto,400)
    a=(rg<=30)&(rf<=20000); b=(rg<=30)&(rf>20000)
    print(f" {fs:.0f}: {(~np.isfinite(ph)).any(-1).sum()} | {(e1>e0+1e-3).sum()} / {(e1>e0+0.05).sum()} (+{(e1-e0).max():.3f}) | {e0[a].max():.2f}->{e1[a].max():.2f} | {e0[b].max():.2f}->{e1[b].max():.2f}")
