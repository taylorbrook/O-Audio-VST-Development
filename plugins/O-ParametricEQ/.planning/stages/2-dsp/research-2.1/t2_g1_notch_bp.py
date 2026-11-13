import numpy as np, eqmodel as m
PI=np.pi
Qs = np.geomspace(0.025, 40, 33); gains=[3,6,12,18,30]
print("G1 grid (fc 20 Hz-20 kHz x40, 20 Hz-20 kHz measure): worst err / tolerance, margin ratio")
for fs in (44100,48000,88200,96000,192000):
    f,w = m.fgrid(fs, 3000, 20.0); fcs=np.geomspace(20,20000,40); wc=2*PI*fcs[:,None]/fs
    out=[]
    for g in gains:
        p,_=m.bell_design(wc,Qs[None,:],float(g)); e=np.abs(m.sec_db(w,p)-m.an2_db(w,*m.bell_proto(wc,Qs[None,:],float(g)))).max()
        tol = (0.15+0.05*g) if fs<88200 else (0.10+0.015*g)
        out.append(f"{e:.3f}/{tol:.2f}")
    s12=[];s6=[]
    for g in gains:
        e=max(np.abs(np.subtract(*m.shelf_db(w,w,wc,np.geomspace(0.025,1.0,17)[None,:],s*g,2,h))).max() for s in (1,-1) for h in (0,1))
        tol=(0.15+0.06*g) if fs<88200 else (0.10+0.015*g); s12.append(f"{e:.3f}/{tol:.2f}")
        e=max(np.abs(np.subtract(*m.shelf_db(w,w,wc,np.array([[1.0]]),s*g,1,h))).max() for s in (1,-1) for h in (0,1))
        tol=(0.10+0.02*g) if fs<88200 else (0.10+0.015*g); s6.append(f"{e:.3f}/{tol:.2f}")
    print(fs,"bell ",out); print(fs,"sh12 Q<=1",s12); print(fs,"sh6  ",s6)

# where does shelf12 Q<=1 fail? breakdown at 44.1k by Q
fs=44100; f,w=m.fgrid(fs,3000,20.0); fcs=np.geomspace(20,20000,40); wc=2*PI*fcs[:,None]/fs
for Q in (0.025,0.1,0.25,0.5,0.71,1.0):
    r=[max(np.abs(np.subtract(*m.shelf_db(w,w,wc,np.array([[Q]]),s*g,2,h))).max() for s in (1,-1) for h in (0,1)) for g in gains]
    print(" sh12 44.1k Q",Q,np.round(r,3))

def notch(wc,Q): return m.mlr(wc,np.inf,wc,Q)
def bp(wc,Q):
    D=m.matched_pair(wc,Q); X0=np.tan(wc/2)**2/D['g']**2
    mH=np.sqrt(np.maximum(1-1/X0**2,0)); mB=np.sqrt(np.maximum(D['k']**2+2*(1-X0)/X0,0))
    return D['g'],D['k'],0*mB,mB,mH,1+0*mB,1+0*mB,1+0*mB
def an_notch(wa,wc,Q):
    wc,Q=[np.asarray(x,float)[...,None] for x in (wc,Q)]
    return 10*np.log10(np.maximum((wc**2-wa**2)**2,1e-300)/((wc**2-wa**2)**2+(wc*wa/Q)**2))
def an_bp(wa,wc,Q):
    wc,Q=[np.asarray(x,float)[...,None] for x in (wc,Q)]
    return 10*np.log10((wc*wa/Q)**2/((wc**2-wa**2)**2+(wc*wa/Q)**2))
Qn=np.geomspace(0.025,40,25)
for fs in (44100,48000):
  f,w=m.fgrid(fs,4000,20.0)
  print(f"--- fs {fs}: notch (err where prototype > -40 dB) and band pass (top 40 dB); max over Q 0.025-40, and Q at worst")
  for lo,hi in [(20,10000),(10000,0.95*fs/2),(0.95*fs/2,25000),(25000,30000)]:
    fcs=np.geomspace(lo*1.0001,hi,40); wt=2*PI*fcs[:,None]/fs; wd=np.minimum(wt,0.95*PI)
    a=an_notch(w,wt,Qn[None,:]); d=m.sec_db(w,notch(wd,Qn[None,:])); e=np.where(a>-40,np.abs(d-a),0).max(-1)
    i=np.unravel_index(e.argmax(),e.shape)
    # per-Q bands
    print(f"  notch fc {lo:.0f}-{hi:.0f}: max {e.max():.2f} dB (Q {Qn[i[1]]:.3f}, fc {fcs[i[0]]:.0f}); Q>=1: {e[:,Qn>=1].max():.2f}; Q<=0.3: {e[:,Qn<=0.3].max():.2f}")
  for lo,hi in [(20,10000),(10000,0.9*fs/2),(0.9*fs/2,25000),(25000,30000)]:
    fcs=np.geomspace(lo*1.0001,hi,40); wt=2*PI*fcs[:,None]/fs; wd=np.minimum(wt,0.9*PI)
    a=an_bp(w,wt,Qn[None,:]); p=bp(wd,Qn[None,:]); d=m.sec_db(w,p); e=np.where(a>-40,np.abs(d-a),0).max(-1)
    i=np.unravel_index(e.argmax(),e.shape)
    pk=np.abs(m.sec_db(wd[:,0][:,None]*np.ones((1,1)),p) if False else 0)
    print(f"  bandpass fc {lo:.0f}-{hi:.0f}: max {e.max():.2f} dB (Q {Qn[i[1]]:.3f}, fc {fcs[i[0]]:.0f}); Q>=1: {e[:,Qn>=1].max():.2f}; Q<=0.3: {e[:,Qn<=0.3].max():.2f}; finite {np.isfinite(d).all()}")
