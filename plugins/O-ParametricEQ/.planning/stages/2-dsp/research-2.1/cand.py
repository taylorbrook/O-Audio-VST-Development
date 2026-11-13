"""Candidate v1 polish ("anchored Gauss-Newton") for any MLR gain section (wz,Qz,wp,Qp,c).
Free: ln g, ln k, ln mB, ln mH, ln G, ln cH. Fixed: mL, cL (DC gain exact, as in the seed)."""
import numpy as np, eqmodel as m, polish as P
PI=np.pi; FREE=np.array([0,1,3,4,5,7])
def sstep(x): x=np.clip(x,0,1); return x*x*(3-2*x)
def tied_pts(fz,Qz,fp,Qp,scales,flo,fhi):
    pts=[]
    for f0,q in ((fz,Qz),(fp,Qp)):
        pts.append(f0+0*q)
        for s in scales:
            h=s/(2*q); pts += [f0*(np.sqrt(1+h*h)+h), f0*(np.sqrt(1+h*h)-h)]
    return np.clip(np.stack(pts,-1),flo,fhi)
def an_pts(wa,wz,Qz,wp,Qp,c):
    wz,Qz,wp,Qp,c=[np.asarray(x,float)[...,None] for x in np.broadcast_arrays(wz,Qz,wp,Qp,c)]
    return 10*np.log10(c**2*(wp/wz)**4*((wz**2-wa**2)**2+(wz*wa/Qz)**2)/((wp**2-wa**2)**2+(wp*wa/Qp)**2))
def polish(fs,phi0,proto,M=16,scales=(0.5,1.5),K=2,mu=0.1,lam=1e-3,guard=True,Sa=0.002,Sb=0.006,rho1=0.1,info=None,Kmax=4,Sx=0.03):
    """phi0 (B,8) seed log-params; proto=(wz,Qz,wp,Qp,c) TRUE analog section (rad/sample). Returns phi (B,8)."""
    wz,Qz,wp,Qp,c=[np.asarray(x,float) for x in np.broadcast_arrays(*proto)]; B=len(wz)
    fhi=min(20000.,0.499*fs); flo=20.
    fa=np.concatenate([np.broadcast_to(np.geomspace(flo,fhi,M),(B,M)),tied_pts(wz*fs/(2*PI),Qz,wp*fs/(2*PI),Qp,scales,flo,fhi)],-1)
    wa=2*PI*fa/fs; t=np.tan(wa/2)**2; tgt=an_pts(wa,wz,Qz,wp,Qp,c); n=fa.shape[1]
    phi=phi0.copy(); I=np.eye(len(FREE)); S0=None
    with np.errstate(all='ignore'):
        def step(phi,act=None):
            db,J=P.resp_jac(phi,t); r=db-tgt; S=(r*r).sum(-1)
            Jf=J[...,FREE]; A=np.einsum('bfi,bfj->bij',Jf,Jf); b=np.einsum('bfi,bf->bi',Jf,r)
            A=A+(lam*np.einsum('bii->bi',A)+mu)[:,:,None]*I; b=b+mu*(phi[:,FREE]-phi0[:,FREE])
            d=np.linalg.solve(np.where(np.isfinite(A),A,0)+1e-300*I,np.where(np.isfinite(b),b,0)[...,None])[...,0]
            out=phi.copy(); out[:,FREE]-=d; return out,S
        for it in range(K):
            phi,S=step(phi)
            if S0 is None: S0=S
        if Kmax and Kmax>K:
            # continuous extension: weight wx rises from 0 to 1 as the match-grid RMS error after K steps goes Sx -> 2 Sx
            r=P.resp_jac(phi,t,False)[0]-tgt; rms=np.sqrt((r*r).sum(-1)/n); wx=sstep((rms-Sx)/Sx); wx=np.where(np.isfinite(wx),wx,0)
            if info is not None: info['wx']=wx
            if (wx>0).any():
                ph2=phi.copy()
                for it in range(Kmax-K): ph2,_=step(ph2)
                phi=phi+wx[:,None]*(np.where(np.isfinite(ph2),ph2,phi)-phi)
        r=P.resp_jac(phi,t,False)[0]-tgt; S1=(r*r).sum(-1)
    if not guard: return phi
    ok=np.isfinite(phi).all(-1)&np.isfinite(S1)&np.isfinite(S0)
    l=sstep((S0/n-Sa**2)/(Sb**2-Sa**2))*sstep((1-S1/np.maximum(S0,1e-300))/rho1); l=np.where(ok,l,0.0)
    if info is not None: info['lam']=l; info['S0']=S0; info['S1']=S1
    return phi0+l[:,None]*(np.where(ok[:,None],phi,phi0)-phi0)
def invert(phi):
    """exact inverse section in log-params (needs min-phase numerator: all log-params real)"""
    g,k,mL,mB,mH,G,cL,cH=[phi[...,i] for i in range(8)]
    return np.stack([g+0.5*(mL-mH), mB-0.5*(mL+mH), -mL, k-0.5*(mL+mH), -mH, G+cL-cH, -cL, -cH],-1)
def reference(fs,phi0,proto,M=220):
    wz,Qz,wp,Qp,c=[np.asarray(x,float) for x in np.broadcast_arrays(*proto)]; B=len(wz); fhi=min(20000.,0.499*fs)
    fa=np.concatenate([np.broadcast_to(np.geomspace(20,fhi,M),(B,M)),tied_pts(wz*fs/(2*PI),Qz,wp*fs/(2*PI),Qp,(0.1,0.25,0.5,0.75,1.,1.5,2.,3.),20,fhi)],-1)
    wa=2*PI*fa/fs
    with np.errstate(all='ignore'): return P.fit(phi0,FREE,np.tan(wa/2)**2,an_pts(wa,wz,Qz,wp,Qp,c))
def eval_db(fs,phi,proto,ne=1000):
    """(digital dB, analog dB) on dense 20 Hz - min(20k,Nyq) grid plus fine tied points; shape (B,F)"""
    wz,Qz,wp,Qp,c=[np.asarray(x,float) for x in np.broadcast_arrays(*proto)]; B=len(wz); fhi=min(20000.,0.499*fs)
    fd=np.concatenate([np.broadcast_to(np.geomspace(20,fhi,ne),(B,ne)),tied_pts(wz*fs/(2*PI),Qz,wp*fs/(2*PI),Qp,(0.1,0.25,0.5,0.75,1.,1.5,2.,3.),20,fhi)],-1)
    wd=2*PI*fd/fs
    with np.errstate(all='ignore'): return P.resp_jac(phi,np.tan(wd/2)**2,False)[0], an_pts(wd,wz,Qz,wp,Qp,c)
def err(fs,phi,proto,ne=1000):
    d,a=eval_db(fs,phi,proto,ne); e=np.abs(d-a).max(-1); return np.where(np.isfinite(e),e,1e9)
def bell_seed(fs,fc,Q,g):
    wc=2*PI*fc/fs; p0,_=m.bell_design(wc,Q,g); return np.log(np.stack(np.broadcast_arrays(*p0),-1)), m.bell_proto(wc,Q,g)
