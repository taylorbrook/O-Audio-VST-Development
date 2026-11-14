import numpy as np, soundfile as sf
P="/Users/taylorbrook/Projects/"
def ct(f,ref): return 1200*np.log2(f/ref)
def peaks(x,sr,n=8,fmax=4000):
    N=1<<int(np.ceil(np.log2(len(x)))+1); X=abs(np.fft.rfft(x*np.hanning(len(x)),N)); X[:int(30*N/sr)]=0; X[int(fmax*N/sr):]=0; out=[]
    Y=X.copy()
    for _ in range(n):
        j=int(np.argmax(Y)); y0,y1,y2=np.log(X[j-1:j+2]+1e-20); d=0.5*(y0-y2)/(y0-2*y1+y2); f=(j+d)*sr/N
        out.append((f,20*np.log10(X[j]/X.max()))); w=int(15*N/sr); Y[max(0,j-w):j+w]=0
    return sorted(out)
def seg(path,a,b):
    i=sf.info(path); x,sr=sf.read(path,start=int(a*i.samplerate),stop=int(b*i.samplerate),always_2d=True); return x.mean(axis=1),sr
D2=73.4162; 
print("crystal 1-6 s peaks (Db4 = 277.18, Db5 = 554.37):")
x,sr=seg(P+"Nox/NOX - Performance Package/max/max/06_crystaDb4+16.wav",1,6)
for f,db in peaks(x,sr,8,6000): print(f"   {f:9.2f} Hz {db:6.1f} dB   re Db4 {ct(f,277.183):+8.1f} ct")
print("guitar_drift, 1 s segments: strongest peaks, f/k re D2")
for t in range(0,9):
    x,sr=seg(P+"TensedComputerImproviser/corpus/DguitarCorpus.wav",316.9+t,317.9+t)
    pk=[(f,db) for f,db in peaks(x,sr,6,1500) if db>-25]
    print(f"  t={t}s "+"  ".join(f"{f:.1f}Hz(h{round(f/D2)} {ct(f/round(f/D2),D2):+.0f}ct,{db:.0f}dB)" for f,db in pk))
print("guitar_overtones, 1 s segments")
for t in range(0,10):
    x,sr=seg(P+"TensedComputerImproviser/corpus/Dovertones.wav",6.3+t,7.2+t)
    pk=[(f,db) for f,db in peaks(x,sr,6,3000) if db>-25]
    print(f"  t={t}s "+"  ".join(f"{f:.1f}Hz(h{round(f/D2)} {ct(f/round(f/D2),D2):+.0f}ct,{db:.0f}dB)" for f,db in pk))
