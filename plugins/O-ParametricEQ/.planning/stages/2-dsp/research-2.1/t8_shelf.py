import numpy as np, cand as C, eqmodel as m, sys
PI=np.pi
def shelf(fs,fc,Qu,g,N,high,mode):
    """full-cascade (digital dB, analog dB) with second-order sections seed / polished / reference. g>=0."""
    wc=2*PI*fc/fs; dig=0; ana=0; lam=[]
    for s in m.shelf_sections(wc,Qu,g,N,high):
        if s[0]=='2':
            proto=np.broadcast_arrays(*s[1:]); phi=np.log(np.stack(np.broadcast_arrays(*m.mlr(*proto)),-1))
            if mode=='polish': info={}; phi=C.polish(fs,phi,proto,info=info); lam.append(info['lam'])
            if mode=='ref': phi=C.reference(fs,phi,proto)
            d,a=C.eval_db(fs,phi,proto,700)
        else:
            fhi=min(20000.,0.499*fs); fd=np.geomspace(20,fhi,700); wd=2*PI*fd/fs
            G1,cL,cH=[np.asarray(x,float)[...,None] for x in m.vic1_shelf(s[1],s[2],s[3])]
            d=10*np.log10(m.op_mag2(wd,G1,cL,cH)); a=m.an1_shelf_db(wd,s[1],s[2],s[3])
            pad=dig.shape[1]-700 if not np.isscalar(dig) else 0   # tied points differ per section: compare on the shared dense part only
        dig=dig+d[:,:700]; ana=ana+a[:,:700]
    return np.abs(dig-ana).max(-1), lam
if __name__=="__main__":
    fs=float(sys.argv[1]) if len(sys.argv)>1 else 44100.
    fcs=np.geomspace(20,20000,40)
    def rows(label,Qs,N,zones,ref=True):
        FC,QQ=np.meshgrid(fcs,Qs,indexing='ij'); fc=FC.ravel(); Q=QQ.ravel()
        for lo,hi in zones:
            z=(fc>lo)&(fc<=hi); out=[]
            for g in (6.,12.,30.):
                e=[np.maximum(*[shelf(fs,fc[z],Q[z],np.full(z.sum(),g),N,h,mode)[0] for h in (False,True)]) for mode in (('seed','polish','ref') if ref else ('seed','polish'))]
                out.append("/".join(f"{x.max():.2f}" for x in e)+f"[{(e[1]>e[0]+1e-3).sum()}]")
            print(f"  {label:26s} fc {lo:5d}-{hi:5d}: "+"   ".join(out))
    print(f"SHELVES fs {fs:.0f}: full-cascade max err, seed/polish/reference-optimum [count polish worse than seed], at 6 / 12 / 30 dB (low and high shelf)")
    Z=[(0,10000),(10000,20000)]
    rows("12 dB/oct Q 0.025-1.0",np.geomspace(0.025,1,9),2,Z)
    rows("12 dB/oct Q 1.0-1.41",np.array([1.0,1.19,1.41]),2,Z)
    rows("12 dB/oct Q 2-10 (weak)",np.array([2.,4.,10.]),2,Z)
    rows("12 dB/oct Q 20-40 (weak)",np.array([20.,40.]),2,Z)
    rows("24 dB/oct Q 1",np.array([1.0]),4,Z)
    rows("48 dB/oct Q 1",np.array([1.0]),8,Z)
    rows("96 dB/oct Q 1",np.array([1.0]),16,Z)
    rows("48 dB/oct Q 0.25-4",np.array([0.25,0.5,2.,4.]),8,Z)
    rows("18 dB/oct Q 1 (odd)",np.array([1.0]),3,Z,ref=False)
    
