import numpy as np, cand as C, eqmodel as m
PI=np.pi; fs=44100.
def sec_err(fc,Qu,g,N,high,**kw):
    wc=2*PI*fc/fs; s=m.shelf_sections(wc,Qu,g,N,high)[0]   # the resonant (highest-Q) section only
    proto=np.broadcast_arrays(*s[1:]); phi0=np.log(np.stack(np.broadcast_arrays(*m.mlr(*proto)),-1))
    return C.err(fs,phi0,proto,700), C.err(fs,C.polish(fs,phi0,proto,**{"Kmax":None,**kw}),proto,700)
fcs=np.geomspace(20,20000,40)
sets=[("sh12 Q<=1.41 all fc",np.geomspace(0.025,1.41,9),2,(0,20000)),("sh12 Q2-10 fc<=10k",np.array([2.,4.,10.]),2,(0,10000)),("sh12 Q2-10 fc>10k",np.array([2.,4.,10.]),2,(10000,20000)),
      ("sh12 Q20-40 fc>10k",np.array([20.,40.]),2,(10000,20000)),("sh96 top section Q1 fc>10k",np.array([1.0]),16,(10000,20000)),("sh48 Q4 top sec fc>10k",np.array([4.]),8,(10000,20000))]
print("highest-Q section only, 44.1k, low+high shelf: seed -> polish max err at 6/12/30 dB  [worse-than-seed]")
for name,kw in [("K2 mu.1",dict(K=2,mu=.1)),("K3 mu.1",dict(K=3,mu=.1)),("K4 mu.1",dict(K=4,mu=.1)),("K6 mu.1",dict(K=6,mu=.1)),("K4 mu.01",dict(K=4,mu=.01)),("K6 mu.01",dict(K=6,mu=.01)),("K10 mu.01",dict(K=10,mu=.01)),("K6 mu.1 lam.1",dict(K=6,mu=.1,lam=.1)),("K10 mu.1 lam.3",dict(K=10,mu=.1,lam=.3))]:
    print(name)
    for label,Qs,N,(lo,hi) in sets:
        FC,QQ=np.meshgrid(fcs[(fcs>lo)&(fcs<=hi)],Qs,indexing='ij'); fc=FC.ravel(); Q=QQ.ravel(); out=[]
        for g in (6.,12.,30.):
            r=[sec_err(fc,Q,np.full_like(fc,g),N,h,**kw) for h in (False,True)]
            e0=np.maximum(r[0][0],r[1][0]); e1=np.maximum(r[0][1],r[1][1]); out.append(f"{e0.max():.2f}->{e1.max():.2f}[{(e1>e0+1e-3).sum()}]")
        print(f"   {label:28s} "+"  ".join(out))
