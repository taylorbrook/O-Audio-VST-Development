import numpy as np, eqmodel as m, polish as P, polish2 as P2, t5_variants as T
PI=np.pi; FREE6=[0,1,3,4,5,7]
def run(fs,fc,Q,g,M,sc,K,mu,clamp,ne=400,chunk=15000,lam=1e-3):
    fmax=min(20000,0.499*fs); fe=np.geomspace(20,fmax,ne); out=[[],[],[],[]]
    for a in range(0,len(fc),chunk):
        s=slice(a,a+chunk); f_,Q_,g_=fc[s],Q[s],g[s]; wc=2*PI*f_/fs; B=len(f_)
        proto=m.bell_proto(wc,Q_,g_); p0,_=m.bell_design(wc,Q_,g_); phi0=np.log(np.stack(p0,-1))
        fa=np.concatenate([np.broadcast_to(np.geomspace(20,fmax,M),(B,M)),T.tied(f_,Q_,g_,sc,fmax)],-1); wa=2*PI*fa/fs
        with np.errstate(all='ignore'):
            ph,S0,S1=P2.fixed2(phi0,FREE6,np.tan(wa/2)**2,T.an(wa,proto),K,mu,lam,clamp)
            fd=np.concatenate([np.broadcast_to(fe,(B,ne)),T.tied(f_,Q_,g_,(0.1,0.25,0.5,0.75,1.,1.5,2.,3.),fmax)],-1); wd=2*PI*fd/fs; td=np.tan(wd/2)**2; ad=T.an(wd,proto)
            out[0].append(np.abs(P.resp_jac(phi0,td,False)[0]-ad).max(-1)); out[1].append(np.abs(P.resp_jac(ph,td,False)[0]-ad).max(-1)); out[2].append(S0); out[3].append(S1)
    return [np.concatenate(x) for x in out]
if __name__=="__main__":
    fcs=np.geomspace(20,20000,40); Qs=np.geomspace(0.025,40,17); FC,QQ=np.meshgrid(fcs,Qs,indexing='ij'); gfc=FC.ravel(); gQ=QQ.ravel()
    rng=np.random.default_rng(1); n=30000
    rf=np.exp(rng.uniform(np.log(10),np.log(30000),n)); rQ=np.exp(rng.uniform(np.log(0.025),np.log(40),n)); rg=rng.uniform(0,60,n)
    print("variant | G1 grid 44.1k max err 6/12/30 dB | random full-range sweep per rate: worse-than-seed count (by >1e-3 dB), worst excess dB, guard misses (S1<S0 yet worse by >0.05 dB), max err |g|<=30 seed->polish")
    for name,K,mu,clamp in [("K2 mu0",2,0,None),("K2 mu.01",2,.01,None),("K2 mu.1",2,.1,None),("K2 mu1",2,1.,None),("K2 mu10",2,10.,None),("K3 mu1",3,1.,None),("K2 mu.1 clamp1",2,.1,1.0),("K2 mu1 clamp1",2,1.,1.0),("K1 mu1",1,1.,None)]:
        r=[]
        for g in (6.,12.,30.):
            e0,e1,_,_=run(44100.,gfc,gQ,np.full_like(gfc,g),16,(.5,1.5),K,mu,clamp,ne=1000); r.append(f"{np.nanmax(e1):.3f}")
        line=f"{name:15s}| "+"/".join(r)+" |"
        for fs in (22050.,44100.,48000.,96000.):
            e0,e1,S0,S1=run(fs,rf,rQ,rg,16,(.5,1.5),K,mu,clamp)
            e1=np.where(np.isfinite(e1),e1,1e9); bad=e1>e0+1e-3; miss=(e1>e0+0.05)&(S1<S0); lo=rg<=30
            line+=f" {fs/1000:.0f}k: {bad.sum()} (+{(e1-e0).max():.2g}) miss {miss.sum()} {e0[lo].max():.2f}->{e1[lo].max():.2f};"
        print(line)
