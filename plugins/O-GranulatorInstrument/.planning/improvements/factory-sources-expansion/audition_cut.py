import sys, os, glob, json, re, html
import numpy as np, soundfile as sf
from scipy.signal import resample_poly
P="/Users/taylorbrook/Projects/"; OUT=sys.argv[1]; SR=48000
SP=P+"gameAudio/SP - Sound Effects/"
items=[]  # dict(id, group, src, note, file)

def load(path, start=None, stop=None):
    info=sf.info(path)
    a=None if start is None else int(start*info.samplerate); b=None if stop is None else int(stop*info.samplerate)
    x,sr=sf.read(path, start=a or 0, stop=b, dtype='float32', always_2d=True)
    if sr!=SR: x=resample_poly(x, SR, sr, axis=0).astype('float32')
    return x
def fade(x, ms=10):
    n=min(int(SR*ms/1000), len(x)//2)
    if n>0:
        r=np.linspace(0,1,n,dtype='float32')[:,None]; x=x.copy(); x[:n]*=r; x[-n:]*=r[::-1]
    return x
def out(id_, group, x, src, note=""):
    pk=float(np.max(np.abs(x))) if len(x) else 0
    if pk<1e-5: print("SILENT", id_); return
    rms=20*np.log10(np.sqrt(np.mean(x**2))+1e-12)
    y=fade(x)*(10**(-3/20)/pk)
    fn=f"{id_}.wav"; sf.write(os.path.join(OUT,fn), y, SR, subtype='PCM_16')
    items.append(dict(id=id_, group=group, src=src.replace(P,""), note=note, file=fn, dur=round(len(x)/SR,1), ch=x.shape[1], crest=round(20*np.log10(pk)-rms,1)))
def frames_db(m, hop):
    n=len(m)//hop
    return 10*np.log10(np.mean(m[:n*hop].reshape(n,hop)**2,axis=1)+1e-12)
def trim(x, thr=-45):
    m=x.mean(axis=1); hop=240; d=frames_db(m,hop)
    if len(d)==0: return x
    ok=np.where(d>d.max()+thr)[0]
    if len(ok)==0: return x
    return x[max(0,(ok[0]-1)*hop):min(len(x),(ok[-1]+3)*hop)]

def windows(path, W, k=3):
    """pick k non-overlapping W-second windows: loud, few dropouts, spread in brightness"""
    info=sf.info(path); sr=info.samplerate
    x,_=sf.read(path, dtype='float32', always_2d=True); m=x.mean(axis=1)
    hop=sr//20; d=frames_db(m,hop); ref=np.percentile(d,90)
    nW=int(W*20); cands=[]
    # brightness per frame: zero-crossing rate proxy
    n=len(d); zc=np.abs(np.diff(np.signbit(m[:n*hop]).astype(np.int8), prepend=0)).reshape(n,hop).mean(axis=1)
    for s in range(0, n-nW, 10):
        seg=d[s:s+nW]; quiet=np.mean(seg<ref-30)
        score=np.mean(np.clip(seg-ref,-40,6)) - 30*quiet
        cands.append((score,s,float(np.mean(zc[s:s+nW]))))
    cands.sort(reverse=True); picks=[]
    for sc,s,z in cands:
        if all(abs(s-p[1])>=nW*1.5 for p in picks) and all(abs(np.log2((z+1e-4)/(p[2]+1e-4)))>0.25 for p in picks[:1] if len(picks)<2):
            picks.append((sc,s,z))
        if len(picks)==k: break
    if len(picks)<k:
        for sc,s,z in cands:
            if all(abs(s-p[1])>=nW*1.5 for p in picks): picks.append((sc,s,z))
            if len(picks)==k: break
    res=[]
    for sc,s,z in sorted(picks,key=lambda p:p[1]):
        # snap both ends to the quietest frame within +-0.75 s
        def snap(f):
            lo=max(0,f-15); hi=min(n,f+15); return lo+int(np.argmin(d[lo:hi]))
        a=snap(s); b=snap(s+nW)
        if b-a<nW//2: a,b=s,s+nW
        res.append((a/20,b/20))
    return res

def long(prefix, group, path, W, k=3, note=""):
    if not os.path.exists(path): print("MISSING",path); return
    for i,(a,b) in enumerate(windows(path,W,k)):
        out(f"{prefix}_{'abc'[i]}", group, load(path,a,b), path, f"{note} {a:.1f}–{b:.1f} s".strip())

def sp(name):
    r=[f for f in glob.glob(SP+"**/"+name, recursive=True) if not os.path.basename(f).startswith("._")]
    return sorted(r)
def whole(id_, group, name, maxlen=12, note=""):
    r=sp(name)
    if not r: print("MISSING",name); return
    info=sf.info(r[0])
    if info.duration>maxlen+0.5: long(id_, group, r[0], 10, 2, note); return
    out(id_, group, trim(load(r[0])), r[0], note)
def collection(id_, group, pattern, maxlen=10, gap=0.18, note=""):
    r=sp(pattern)
    if not r: print("MISSING",pattern); return
    parts=[]; tot=0; used=0
    for f in r:
        x=trim(load(f), -60)
        if x.shape[1]==2: x=x.mean(axis=1,keepdims=True)
        pk=np.max(np.abs(x)); 
        if pk<1e-4: continue
        x=fade(x,3)*(0.6/pk)**0.5   # half-way level matching between hits
        if tot+len(x)/SR>maxlen: break
        parts+= [x, np.zeros((int(gap*SR),1),'float32')]; tot+=len(x)/SR+gap; used+=1
    if parts: out(id_, group, np.concatenate(parts), os.path.dirname(r[0])+"/"+pattern, f"{note} {used} of {len(r)} files end to end".strip())

G="Guitar"
T=P+"TensedComputerImproviser/"; E=P+"electric guitar and improvisor/"
long("gtr_overtones", G, T+"corpus/Dovertones.wav", 9)
long("gtr_harmonics", G, T+"corpus/guitarharmonics.wav", 9)
long("gtr_in_d", G, T+"corpus/guitarin D 3.wav", 9)
long("gtr_in_d2", G, T+"corpus/guitarin D 2.wav", 9, 2)
long("gtr_corpus", G, T+"corpus/DguitarCorpus.wav", 9)
long("gtr_d_overtone_take", G, T+"corpus/guitarrecording_1#01guitarDOvertone.wav", 9)
long("gtr_drones", G, T+"jeffAudio/BrookdronesLouder.wav", 12)
long("gtr_improv", G, T+"jeffAudio/BrookImprovCorpus2.wav", 9)
long("gtr_hypha", G, E+"git/Hypha/hypha-MAX/corpus.wav", 9)
G="Field"
for n in ["stream","rainystreet","park","voices","waves","woods","traffic"]:
    long("field_"+n, G, P+f"Soundscaper/git/soundscaper/audio/{n}.wav", 10, 2)
for n,f in [("calm_sea","ambiance_calm sea.wav"),("crashing_waves","ambiance_crashing waves.wav"),("garden","ambiance_garden.wav"),("interior","ambiance_interior.wav"),("night","ambiance_night_1.wav")]:
    whole("amb_"+n, G, f, 10)
G="Crystal (Nox)"
for f in sorted(glob.glob(P+"Nox/NOX - Performance Package/max/max/06_cry*.wav")):
    nm=re.sub(r'[^a-z0-9]+','_',os.path.basename(f)[3:-4].lower().replace('+','p').replace('-','m')).strip('_')
    x=load(f); out("nox_"+nm, G, trim(x[:12*SR]), f, os.path.basename(f)[3:-4])
G="Foley — textures"
for id_,f in [("fire_crackle","sfx_fire_crackling_loop1.wav"),("fire_soft","sfx_fire_softcrackling_loop1.wav"),("fire_wind","sfx_fire_crackling_wind_loop1.wav"),("fire_growing","fire_growing.wav"),
              ("bubbles_airy","water_bubbles_airytexture_1.wav"),("bubbles","water_bubbles_texture_3.wav"),("water_pour","water_pour_on_plastic_5*.wav"),
              ("metal_rubbing","MetalFiles_rubbing_1.wav"),("metal_grate","MetalFiles_grate_2.wav"),("metal_grate_growing","MetalFiles_grate_growing_2.wav"),("metal_jangle","metal_jangling_5.wav"),("metal_screwing","metal_screwing_1.wav"),
              ("pliers_crank","Pliers_cranking_5*.wav"),("clamp","clamp_testure_6*.wav"),("door_hinge","door_hinge_1.wav"),("drill","drill_2.wav"),("drill_changing","drill_changing_2.wav"),
              ("sand_shake","sand_shaking_1.wav"),("tape_measure","tapemeasure_9.wav"),("sticks_rustle","Sticks_rustling_3.wav"),
              ("voice_fry","voice_fry_4*.wav"),("mouth_gargle","mouth_gargle.wav"),("gargle_hum","voice_gargle_hum_1.wav"),("growl_fry","voice_zombie_growl_6*.wav")]:
    whole(id_, G, f)
G="Foley — collections (one-shots end to end)"
for id_,pat in [("rock_ticks","rock_pebble_tick_*.wav"),("rock_knocks","rock_knock_*.wav"),("rock_smash","rock_smash_*.wav"),("pebbles","rock_pebbles_smush_*.wav"),
                ("glass_ticks","glass_tick_*.wav"),("metal_ticks","metal_tick_*.wav"),("metal_pipe","MetalPipe_dryhit_*.wav"),("stick_snaps","Sticks_snap_*.wav"),
                ("chest_slaps","chestslap_*.wav"),("impacts","impact_medium_*.wav"),("wet_impacts","impact_wet_*.wav"),("duct_tape","ducttape_rip_*.wav"),
                ("scissors","Scissors_snip*.wav"),("staple_gun","staplegun_click_*.wav"),("hiss","voice_hiss_*.wav"),("footsteps_carpet","footstep_carpet_*.wav"),("door_hinges","door_hinge_*.wav"),("squish","water_shirt_squish_*.wav")]:
    collection(id_, G, pat)
json.dump(items, open(os.path.join(OUT,"items.json"),"w"), indent=1)
print(len(items),"items", round(sum(i['dur'] for i in items)),"s")
