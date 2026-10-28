from fast import *
import sys
# fmax = a : source content inside the audible band (level>=1 is HB-limited to exactly a in level units)
def outpb(t,h,r,a):
    f,X=resp(t,h); i=np.argmax(20*np.log10(X+1e-30)<-1); g=min(f[i],a if r<1 else 9)
    return g*r
for fs in [44.1,48]:
  a=20/fs
  for T in [16,24,32]:
    row=[]
    for r in [0.5,0.707,0.85,1.0,1.19,1.414,1.68,1.99,2.83,3.99]:
      best=None
      for beta in np.arange(4,9.5,0.5):
        for fcn in np.linspace(0.4,1.1,36):
          fc=0.5*fcn/max(r,1); t,h=kaiser_sinc(T,fc,beta)
          if worst_alias(t,h,r,a,a)<=-60:
            p=outpb(t,h,r,a)
            if best is None or p>best: best=p
      row.append(f"{r}:"+("FAIL" if best is None else f"{best*fs:4.1f}"))
    print(f"fs{fs} T{T} out -1dB kHz: "+" ".join(row)); sys.stdout.flush()
