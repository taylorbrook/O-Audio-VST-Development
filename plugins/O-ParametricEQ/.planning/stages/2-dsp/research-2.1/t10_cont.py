import numpy as np, cand as C, eqmodel as m
PI=np.pi; fs=44100.; fd=np.geomspace(20,20000,160); td=np.tan(PI*fd/fs)**2
def design(kind,fc,Q,g,**kw):
    wc=2*PI*fc/fs
    if kind=='bell': phi0,proto=C.bell_seed(fs,fc,Q,g)
    else:
        s=m.shelf_sections(wc,Q,g,2 if kind!='hs48' else 8,kind!='ls')[0]; proto=np.broadcast_arrays(*s[1:]); phi0=np.log(np.stack(np.broadcast_arrays(*m.mlr(*proto)),-1))
    return phi0, C.polish(fs,phi0,proto,**kw)
def jumps(phi):
    db=C.P.resp_jac(phi,td,False)[0]; return np.abs(np.diff(db,axis=0)).max()
kw=dict(Kmax=4)
print("adjacent-step response jump (dB, max over 20 Hz-20 kHz) along fine sweeps at 44.1 kHz; n = 8000 and 32000 steps. A continuous map shrinks ~4x; a discontinuity does not.")
print("sweep | seed n=8k, 32k | polish n=8k, 32k")
cases=[('bell',1.0,12.),('bell',0.1,30.),('bell',10.,30.),('bell',40.,6.),('bell',0.025,18.),('ls',0.71,12.),('hs',1.0,30.),('hs',4.0,12.),('ls',10.,6.),('hs48',1.0,30.)]
for kind,Q,g in cases:
    r=[]
    for n in (8000,32000):
        fc=np.geomspace(10,30000,n); p0,p1=design(kind,fc,np.full(n,Q),np.full(n,g),**kw); r.append((jumps(p0),jumps(p1)))
    print(f" fc 10->30k  {kind:5s} Q {Q:<5} g {g:<4}: {r[0][0]:.4f} {r[1][0]:.4f} | {r[0][1]:.4f} {r[1][1]:.4f}  {'<-- JUMP' if r[1][1]>0.6*r[0][1] and r[1][1]>2*r[1][0] else ''}")
for kind,Q,fc0 in [('bell',1.0,15000.),('bell',0.2,8000.),('bell',8.,19000.),('hs',1.0,12000.),('ls',3.0,14000.),('bell',1.0,25000.)]:
    r=[]
    for n in (8000,32000):
        g=np.linspace(0.001,60,n); p0,p1=design(kind,np.full(n,fc0),np.full(n,Q),g,**kw); r.append((jumps(p0),jumps(p1)))
    print(f" gain 0->60  {kind:5s} Q {Q:<5} fc {fc0:<6.0f}: {r[0][0]:.4f} {r[1][0]:.4f} | {r[0][1]:.4f} {r[1][1]:.4f}  {'<-- JUMP' if r[1][1]>0.6*r[0][1] and r[1][1]>2*r[1][0] else ''}")
for kind,g,fc0 in [('bell',12.,15000.),('bell',30.,5000.),('hs',12.,12000.),('ls',18.,16000.),('bell',18.,22000.)]:
    r=[]
    for n in (8000,32000):
        Q=np.geomspace(0.025,40,n); p0,p1=design(kind,np.full(n,fc0),Q,np.full(n,g),**kw); r.append((jumps(p0),jumps(p1)))
    print(f" Q .025->40  {kind:5s} g {g:<4} fc {fc0:<6.0f}: {r[0][0]:.4f} {r[1][0]:.4f} | {r[0][1]:.4f} {r[1][1]:.4f}  {'<-- JUMP' if r[1][1]>0.6*r[0][1] and r[1][1]>2*r[1][0] else ''}")
