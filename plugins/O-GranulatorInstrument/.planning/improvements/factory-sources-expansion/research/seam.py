import numpy as np, sys
sys.argv=['x',sys.argv[1]]; exec(open(sys.argv[1]+'/../ingest_proto.py').read().split("rng=np.random")[0])
n=int(.5*SR); t=(np.arange(n)+.5)/n
def worst(x,fi,fo,hop=2400):
    head=x[:n].mean(axis=1); tail=x[len(x)-n:].mean(axis=1); mix=head*fi+tail*fo; out=[]
    for s in range(0,n,hop):
        e=(fi[s:s+hop]**2*head[s:s+hop]**2+fo[s:s+hop]**2*tail[s:s+hop]**2).mean()   # uncorrelated expectation
        ref=0.5*((head[s:s+hop]**2).mean()+(tail[s:s+hop]**2).mean())
        out.append((10*np.log10((mix[s:s+hop]**2).mean()/e), 10*np.log10((mix[s:s+hop]**2).mean()/ref)))
    o=np.array(out); return o
for id_,ld,sm in src:
    if not sm: continue
    x=ld(); a=worst(x,np.sin(.5*np.pi*t),np.cos(.5*np.pi*t)); b=worst(x,t,1-t)
    print(f"{id_:8s} equal-power: re expectation {a[:,0].min():+.1f}..{a[:,0].max():+.1f} dB, re mean(head,tail) power, centre 100 ms {a[4:6,1].mean():+.1f} | linear ctrl centre {b[4:6,1].mean():+.1f} dB")
