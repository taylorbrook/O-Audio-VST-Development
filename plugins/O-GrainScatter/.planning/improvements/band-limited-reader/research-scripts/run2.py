from fast import *
import sys
tl,hl=lagrange4()
for fs in [44.1,48]:
    a=20/fs
    print(f"fs {fs} Lagrange-4 alias fmax=a:", " ".join(f"{r}:{worst_alias(tl,hl,r,a,a):.0f}" for r in [1.06,1.5,2,4,8]), f" -1dB pb {pb(tl,hl,1)*fs:.1f}k")
for fs in [44.1,48]:
  a=20/fs
  for T in [16,24,32]:
    for fmaxname in ['a']:
      row=[]
      for r in [1.0,1.1,1.25,1.5,1.75,1.99,2.5,3.0,3.99]:
        best=None
        for beta in np.arange(3,11,0.5):
          for fcn in np.linspace(0.3,1.1,41):
            fc=0.5*fcn/max(r,1); t,h=kaiser_sinc(T,fc,beta)
            fmax=a if r<=1.0001 else min(0.5, a/1.0)  # content: level0 audible input; see notes
            fmax=0.5
            if worst_alias(t,h,r,a,fmax)<=-60:
              p=pb(t,h,r)
              if best is None or p>best[0]: best=(p,beta,fcn)
        row.append(f"{r}:"+("FAIL" if best is None else f"{best[0]*fs:4.1f}k(b{best[1]},c{best[2]:.2f})"))
      print(f"fs{fs} T{T}: "+" ".join(row)); sys.stdout.flush()
