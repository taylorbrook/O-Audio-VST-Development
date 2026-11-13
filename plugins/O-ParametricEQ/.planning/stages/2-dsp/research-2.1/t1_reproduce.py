import numpy as np, eqmodel as m
PI=np.pi
def bell_err(fs, fcs, Qs, gdb, n=3000):
    f, w = m.fgrid(fs, n)
    wc = 2*PI*fcs[:,None]/fs; Q = Qs[None,:]
    p,_ = m.bell_design(wc, Q, gdb)
    d = m.sec_db(w, p); a = m.an2_db(w, *m.bell_proto(wc, Q, gdb))
    return np.abs(d-a).max(-1)
Qs = np.geomspace(0.025, 40, 33)
print("Bell (MLR) max err, Q 0.025-40")
for fs, lo, hi in [(44100,10,16000),(44100,16000,20000),(48000,10,20000),(96000,10,20000),(44100,20000,30000)]:
    fcs = np.geomspace(max(lo,10), hi, 60)
    if lo>10: fcs = fcs[1:] if False else np.geomspace(lo*1.0001, hi, 40)
    row = [bell_err(fs, fcs, Qs, g).max() for g in (6,12,30)]
    rowc = [bell_err(fs, fcs, Qs, -g).max() for g in (6,12,30)]
    print(f"  {fs} fc {lo}-{hi}: boost {row[0]:.2f} {row[1]:.2f} {row[2]:.2f}   cut {rowc[0]:.2f} {rowc[1]:.2f} {rowc[2]:.2f}")
def shelf_err(fs, fcs, Qs, gdb, N, high, n=3000):
    f, w = m.fgrid(fs, n)
    wc = 2*PI*fcs[:,None]/fs; Q = Qs[None,:]
    d,a = m.shelf_db(w, w, wc, Q, gdb, N, high)
    return np.abs(d-a).max(-1)
print("Shelves (max over low/high shelf)")
for label, fs, lo, hi, Qs_, N in [
  ("12 Q1", 44100, 10, 10000, np.array([1.0]), 2), ("12 Q1", 44100, 10000, 20000, np.array([1.0]), 2),
  ("12 Q.25-1.41", 44100, 10000, 20000, np.geomspace(0.25,1.41,12), 2), ("12 Q1", 96000, 10, 20000, np.array([1.0]),2),
  ("6", 44100, 10, 30000, np.array([1.0]), 1), ("6", 96000, 10, 30000, np.array([1.0]), 1),
  ("48 Q1", 44100, 10, 10000, np.array([1.0]), 8), ("48 Q1", 44100, 10000, 20000, np.array([1.0]), 8),
  ("12 Q1 20-30k", 44100, 20000, 30000, np.array([1.0]), 2)]:
    fcs = np.geomspace(lo if lo==10 else lo*1.0001, hi, 60)
    row = [max(shelf_err(fs,fcs,Qs_,s*g,N,h).max() for s in (1,-1) for h in (False,True)) for g in (6,12,30)]
    print(f"  {label:14s} {fs} fc {lo}-{hi}: {row[0]:.2f} {row[1]:.2f} {row[2]:.2f}")
# reciprocity
f,w = m.fgrid(44100, 2000)
wc = 2*PI*np.geomspace(10,30000,50)[:,None]/44100
pb,_ = m.bell_design(wc, Qs[None,:], 30.0); pc,_ = m.bell_design(wc, Qs[None,:], -30.0)
print("bell reciprocity max |dB|:", np.abs(m.sec_db(w,pb)+m.sec_db(w,pc)).max())
# 10 Hz Q40 +30 at 192k
fs=192000; f = np.geomspace(1,1000,4000); w=2*PI*f/fs
p,_ = m.bell_design(2*PI*10/fs, 40.0, 30.0)
print("10Hz Q40 +30 @192k:", np.abs(m.sec_db(w,p)-m.an2_db(w,*m.bell_proto(2*PI*10/fs,40.0,30.0))).max())
