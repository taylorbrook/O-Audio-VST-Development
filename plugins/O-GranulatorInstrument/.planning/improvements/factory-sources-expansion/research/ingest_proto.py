import sys, os, glob, numpy as np, soundfile as sf
from scipy.signal import resample_poly
P="/Users/taylorbrook/Projects/"; SP=P+"gameAudio/SP - Sound Effects/"; OUT=sys.argv[1]; SR=48000
def load(path,a=None,b=None):
    i=sf.info(path); x,sr=sf.read(path,start=int((a or 0)*i.samplerate),stop=None if b is None else int(b*i.samplerate),dtype='float64',always_2d=True)
    if sr!=SR:
        from math import gcd
        g=gcd(SR,sr); x=resample_poly(x,SR//g,sr//g,axis=0)
    return x
def fade(x,ms=10):
    n=int(SR*ms/1000); r=np.linspace(0,1,n)[:,None]; x=x.copy(); x[:n]*=r; x[-n:]*=r[::-1]; return x
def frames_db(m,hop):
    n=len(m)//hop; return 10*np.log10(np.mean(m[:n*hop].reshape(n,hop)**2,axis=1)+1e-12)
def trim(x,thr=-45):
    m=x.mean(axis=1); hop=240; d=frames_db(m,hop); ok=np.where(d>d.max()+thr)[0]
    return x[max(0,(ok[0]-1)*hop):min(len(x),(ok[-1]+3)*hop)]
def seam(x,F=0.5):
    n=int(F*SR); t=(np.arange(n)+0.5)/n; fi=np.sin(0.5*np.pi*t)[:,None]; fo=np.cos(0.5*np.pi*t)[:,None]
    y=x[:len(x)-n].copy(); y[:n]=x[:n]*fi+x[len(x)-n:]*fo; return y
def coll(pat,gap=0.18):
    parts=[]
    for f in sorted(g for g in glob.glob(SP+"**/"+pat,recursive=True) if not os.path.basename(g).startswith("._")):
        x=trim(load(f),-60); x=x.mean(axis=1,keepdims=True); pk=np.abs(x).max()
        x=fade(x,3)*(0.6/pk)**0.5; parts+=[x,np.zeros((int(gap*SR),1))]
    return np.concatenate(parts)
T=P+"TensedComputerImproviser/corpus/"; A=P+"Soundscaper/git/soundscaper/audio/"
src=[ # id, loader, seam, bits
 ("guitar_overtones",lambda:load(T+"Dovertones.wav",6.3,16.2),0),
 ("guitar_drift",lambda:load(T+"DguitarCorpus.wav",316.9,326.6),0),
 ("stream",lambda:load(A+"stream.wav",11.1,21.9),1),
 ("park",lambda:load(A+"park.wav",4.7,15.1),1),
 ("waves",lambda:load(A+"waves.wav",27.1,38.0),1),
 ("woods",lambda:load(A+"woods.wav",187.1,197.8),1),
 ("garden",lambda:load(SP+"ambiances/ambiance_garden.wav",4.0,14.3),1),
 ("crystal",lambda:trim(load(P+"Nox/NOX - Performance Package/max/max/06_crystaDb4+16.wav")[:12*SR]),0),
 ("fire",lambda:trim(load(SP+"fire/sfx_fire_crackling_loop1.wav")),1),
 ("bubbles",lambda:trim(load(SP+"water/water_bubbles_airytexture_1.wav")),1),
 ("metal_grate",lambda:trim(load(SP+"Metals/granular/MetalFiles_grate_2.wav")),0),
 ("stick_snaps",lambda:coll("Sticks_snap_*.wav"),0),
 ("scissors",lambda:coll("Scissors_snip*.wav"),0),
 ("contrabassoon",lambda:load(P+"five weather reports/Solo Version/samples/contrabassoon/G#0-31.wav"),0),
]
rng=np.random.default_rng(1)
tot={16:0,24:0}
print(f"{'id':17s} ch   len   crest  gain   peak    rms   p90f | flac16    flac24  | seam: lvl dB  step/p999")
for id_,ld,sm in src:
    x=ld(); 
    if sm: x=seam(x)
    else: x=fade(x)
    m=x.mean(axis=1); h=int(.4*SR); fr=frames_db(m,h); p90=np.percentile(fr,90)
    pk=20*np.log10(np.abs(x).max())
    g=min(-20.0-p90, -1.0-pk)       # p90 400 ms frame -> -20 dBFS, peak cap -1
    y=x*10**(g/20)
    rms=20*np.log10(np.sqrt((y**2).mean()))
    sizes={}
    for bits in (16,24):
        q=2**(bits-1); d=(rng.random(y.shape)-rng.random(y.shape)) if bits==16 else 0
        z=np.clip(np.round(y*q+d),-q,q-1)/q
        fn=f"{OUT}/{id_}.{bits}.flac"; sf.write(fn,z,SR,subtype='PCM_24' if bits==24 else 'PCM_16',format='FLAC'); sizes[bits]=os.path.getsize(fn); tot[bits]+=sizes[bits]
    s=""
    if sm:
        # level across wrap: 100 ms either side of the wrap + over the crossfaded head, re the body median (100 ms frames)
        mm=y.mean(axis=1); f=frames_db(mm,4800); body=np.median(f)
        wrap=np.concatenate([mm[-4800:],mm[:4800]]); head=frames_db(mm[:24000],4800)
        step=abs(y[0]-y[-1]).max(); dd=np.abs(np.diff(y,axis=0)); 
        s=f"wrap {10*np.log10((wrap**2).mean()+1e-12)-body:+.1f} head {head.min()-body:+.1f}..{head.max()-body:+.1f} (body p10..p90 {np.percentile(f,10)-body:+.1f}..{np.percentile(f,90)-body:+.1f})  {step/np.percentile(dd,99.9):.2f}"
    print(f"{id_:17s} {x.shape[1]} {len(x)/SR:6.2f} {pk-20*np.log10(np.sqrt((x**2).mean())):6.1f} {g:+6.1f} {pk+g:6.1f} {rms:6.1f} {p90+g:6.1f} | {sizes[16]:8d} {sizes[24]:8d} | {s}")
print("total16",tot[16],"total24",tot[24])
