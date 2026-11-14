import numpy as np, soundfile as sf
P="/Users/taylorbrook/Projects/"
def note(f):
    m=69+12*np.log2(f/440); n=int(round(m)); return f"{['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'][n%12]}{n//12-1} ({n}) {100*(m-n):+.1f} ct"
def acf_f0(x,sr,lo,hi,win):
    """per-frame normalised autocorrelation, parabolic peak; returns list of f0"""
    out=[]; hop=win//2
    for s in range(0,len(x)-win,hop):
        w=x[s:s+win]-x[s:s+win].mean()
        if np.sqrt((w**2).mean())<1e-3: out.append(np.nan); continue
        F=np.fft.rfft(w*np.hanning(win),2*win); r=np.fft.irfft(abs(F)**2); r/=r[0]
        a,b=int(sr/hi),int(sr/lo); k=a+int(np.argmax(r[a:b]))
        # prefer the shortest lag within 0.9 of the max (octave guard)
        thr=0.9*r[k]
        for j in range(a+1,b-1):
            if r[j]>thr and r[j]>=r[j-1] and r[j]>=r[j+1]: k=j; break
        y0,y1,y2=r[k-1],r[k],r[k+1]; d=0.5*(y0-y2)/(y0-2*y1+y2)
        out.append(sr/(k+d))
    return np.array(out)
def harm_f0(x,sr,f0,K=12):
    """long-window FFT: refine each harmonic peak, weighted mean of f_k/k"""
    n=len(x); N=1<<int(np.ceil(np.log2(n))+2); X=abs(np.fft.rfft(x*np.hanning(n),N)); fs=[];ws=[]; rows=[]
    for k in range(1,K+1):
        c=k*f0*N/sr; a=int(c*(1-0.02/1)); b=int(c*1.02)+2
        a=int((k*f0-0.4*f0)*N/sr); b=int((k*f0+0.4*f0)*N/sr)
        j=a+int(np.argmax(X[a:b])); y0,y1,y2=np.log(X[j-1:j+2]); d=0.5*(y0-y2)/(y0-2*y1+y2)
        f=(j+d)*sr/N; fs.append(f/k); ws.append(X[j]); rows.append((k,f,f/k,20*np.log10(X[j]/X.max())))
    fs=np.array(fs); ws=np.array(ws); return float((fs*ws).sum()/ws.sum()), rows
def run(name,path,a,b,lo,hi,win_s):
    i=sf.info(path); x,sr=sf.read(path,start=int(a*i.samplerate),stop=int(b*i.samplerate) if b else None,always_2d=True); x=x.mean(axis=1)
    f=acf_f0(x,sr,lo,hi,int(win_s*sr)); g=f[~np.isnan(f)]
    med=np.median(g); print(f"\n{name}: {len(g)} frames, ACF median {med:.3f} Hz = {note(med)}; p10..p90 {np.percentile(g,10):.3f}..{np.percentile(g,90):.3f}")
    print("  per-frame cents re median:", " ".join(f"{1200*np.log2(v/med):+.0f}" for v in g[:40]))
    return x,sr,med
x,sr,med=run("contrabassoon",P+"five weather reports/Solo Version/samples/contrabassoon/G#0-31.wav",0,None,20,40,0.6)
for (a,b) in [(0.5,7.5),(0.5,4.0),(4.0,7.5)]:
    f0,rows=harm_f0(x[int(a*sr):int(b*sr)],sr,med,16)
    print(f"  harmonic fit {a}-{b}s: {f0:.4f} Hz = {note(f0)}")
    if a==0.5 and b==7.5:
        for k,f,fk,db in rows: print(f"    h{k:2d} {f:8.3f} Hz  /k {fk:.4f} = {1200*np.log2(fk/25.9565):+.1f} ct re G#0  {db:6.1f} dB")
x,sr,med=run("guitar_drift",P+"TensedComputerImproviser/corpus/DguitarCorpus.wav",316.9,326.6,55,200,0.2)
x,sr,med=run("guitar_overtones",P+"TensedComputerImproviser/corpus/Dovertones.wav",6.3,16.2,55,1200,0.2)
x,sr,med=run("crystal",P+"Nox/NOX - Performance Package/max/max/06_crystaDb4+16.wav",0.3,11,150,1200,0.1)
f0,rows=harm_f0(x[int(.5*sr):int(8*sr)],sr,med,4); print("  crystal harmonic fit",f0,note(f0)); [print("   ",r) for r in rows]
