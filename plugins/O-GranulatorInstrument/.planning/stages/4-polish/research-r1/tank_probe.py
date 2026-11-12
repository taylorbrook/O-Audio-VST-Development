# Port of modules/effects/reverb-unit/cpp/ReverbUnit.h tank (mix 1, predelay 20 ms, mod 0.2, shimmer 0), 48 k.
# Measures wet-only (T) RMS / peak vs the mono input. Smoothers omitted (static params).
import math, random, sys
FS=48000.0
BASE=[809,877,937,1049,1151,1249,1373,1499]; DD=[142,107,79,53]; G=0.625
POL=[[1,-1,1,1,-1,1,-1,-1],[1,1,-1,1,-1,-1,1,-1],[-1,1,1,-1,1,-1,-1,1],[1,-1,-1,1,1,-1,1,-1]]
LFO=[0.15,0.33,0.57,0.97]
def run(size,damp,secs,kind,amp,seed=1):
    rnd=random.Random(seed)
    scale=(0.5+size); sd=[b*scale for b in BASE]; avg=sum(sd)/8
    rt=0.2+size*size*11.8; fbg=10**(-3*avg/(FS*rt)); a=(damp*0.7)
    N=8192; M=N-1
    tank=[[0.0]*N for _ in range(8)]; wp=0
    dl=[[[0.0]*256 for _ in range(8)] for _ in range(4)]; dwp=[0]*4; st=[[0.0]*8 for _ in range(4)]
    z=[0.0]*8; ph=[0.0]*4
    pre=[0.0]*960; pp=0
    n=int(secs*FS); e_in=0.0; e_out=0.0; pk_in=0.0; pk_out=0.0; clips=0
    half=n//2; res=[]
    win=int(0.5*FS); acc_i=acc_o=0.0
    for i in range(n):
        if kind=='noise': x=amp*(2*rnd.random()-1)
        else: x=amp*math.sin(2*math.pi*440*i/FS)
        d=pre[pp]; pre[pp]=x; pp=(pp+1)%960; mono=d
        ch=[mono]*8
        for s in range(4):
            L=dl[s]; w=dwp[s]; r=(w-DD[s])&255
            for c in range(8):
                delayed=L[c][r]; v=ch[c]-G*st[s][c]; out=st[s][c]+G*v; st[s][c]=delayed; L[c][w]=v; ch[c]=out*POL[s][c]
            dwp[s]=(w+1)&255
            sm=sum(ch)*0.25
            ch=[c_-sm for c_ in ch]
        to=[0.0]*8
        for c in range(8):
            dly=sd[c]
            if c&1==0:
                k=c>>1; ph[k]+=LFO[k]/FS
                if ph[k]>=1: ph[k]-=1
                dly+=math.sin(ph[k]*2*math.pi)*0.2*16.0
            rp=wp-dly; ip=math.floor(rp); fr=rp-ip
            to[c]=tank[c][ip&M]*(1-fr)+tank[c][(ip+1)&M]*fr
        sm=sum(to)*0.25
        to=[t-sm for t in to]
        for c in range(8):
            xx=to[c]*fbg; z[c]=xx+a*(z[c]-xx)
            v=ch[c]+z[c]
            if v>2: v=2.0; clips+=1
            elif v<-2: v=-2.0; clips+=1
            tank[c][wp]=v
        wp=(wp+1)&M
        oL=(to[0]+to[2]+to[4]+to[6])*0.5
        acc_i+=x*x; acc_o+=oL*oL
        if abs(oL)>pk_out: pk_out=abs(oL)
        if (i+1)%win==0:
            res.append(10*math.log10(max(acc_o,1e-30)/max(acc_i,1e-30))); acc_i=acc_o=0.0
    print(f"size {size} damp {damp} {kind} amp {amp}: wet/dry RMS dB per 0.5 s: "+" ".join(f"{r:+.1f}" for r in res)+f" | wet peak {pk_out:.3f} (in peak {amp}) clips {clips} fbg {fbg:.4f}",flush=True)
cfg=[(0.5,0.5,5,'noise',0.1),(1.0,0.5,10,'noise',0.1),(1.0,0.0,10,'noise',0.1),(0.0,0.5,3,'noise',0.1),(1.0,0.5,10,'sine',0.1),(1.0,0.0,8,'noise',0.7)]
for c in cfg: run(*c)
