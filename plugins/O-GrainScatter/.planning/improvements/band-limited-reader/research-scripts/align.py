import numpy as np
from scipy import signal
N=127; n=np.arange(N)-(N-1)/2
hb=0.5*np.sinc(n/2)*np.kaiser(N,9.0); hb/=hb.sum()
rng=np.random.default_rng(1)
x=signal.lfilter(*signal.butter(8,0.02),rng.standard_normal(1<<15))  # content << fs/32
# causal cascade: level l sample j = sum_k hb[k] * parent[2j+1-k]   (output on odd parent index)
levels=[x]
for l in range(3):
    p=levels[-1]; y=signal.lfilter(hb,1,p)[1::2]; levels.append(y)
def interp(v,pos,T=64):
    i0=int(np.floor(pos)); fr=pos-i0; k=np.arange(-T//2+1,T//2+1)
    w=np.sinc(k-fr)*np.kaiser(T,8,)  ; w/=w.sum(); return np.dot(v[i0+k],w)
# derived mapping: level l sample j <-> level-0 time t = (2^l)*j + c_l ; c_1 = 1-63, c_{l+1} = 2*c_l... derive: parent idx 2j+1 - 63 (parent units)
def to_level(t0,l):
    # t (level-0 units) -> level-l fractional index
    pos=t0
    for _ in range(l):
        # parent index q corresponds to child index (q - 1 + 63)/2
        pos=(pos+63-1)/2
    return pos
for l in range(1,4):
    errs=[]
    for t0 in np.linspace(9000,30000,200):
        a=interp(levels[0],t0); b=interp(levels[l],to_level(t0,l)); errs.append(abs(a-b))
    D=sum(63*2**k for k in range(l))
    print(f"level {l}: max |lvl0 - lvl{l}| = {20*np.log10(max(errs)/np.std(x)):.1f} dB rel rms; accumulated delay D_acc = {D} level-0 samples; mapping p_l = (p0 + {D} - {(2**l-1)})/{2**l}")
    # check closed form
    t0=12345.678; print("   closed form check", to_level(t0,l), (t0+D-(2**l-1))/2**l)
