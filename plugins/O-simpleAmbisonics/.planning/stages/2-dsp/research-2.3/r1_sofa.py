import sofar, numpy as np, glob, os, inspect
base = "/Users/taylorbrook/Dev/VST-development/plugins/O-simpleAmbisonics/.cache/sadie/D1_HRIR_SOFA"
for f in sorted(glob.glob(base + "/*.sofa")):
    s = sofar.read_sofa(f, verbose=False)
    print("=====", os.path.basename(f), os.path.getsize(f)//1024, "KB")
    print(" conv", s.GLOBAL_SOFAConventions, s.GLOBAL_SOFAConventionsVersion, "| DataType", s.GLOBAL_DataType)
    for k in ["GLOBAL_Title","GLOBAL_DatabaseName","GLOBAL_ListenerShortName","GLOBAL_License","GLOBAL_Comment","GLOBAL_History","GLOBAL_Organization","GLOBAL_AuthorContact","GLOBAL_References","GLOBAL_RoomType","GLOBAL_Origin"]:
        if hasattr(s,k): print(" ", k, "=", repr(getattr(s,k))[:200])
    ir = s.Data_IR; print(" Data_IR", ir.shape, ir.dtype, "fs", s.Data_SamplingRate, "delay", np.unique(s.Data_Delay))
    sp = s.SourcePosition; print(" SourcePosition", sp.shape, s.SourcePosition_Type, s.SourcePosition_Units)
    print("  az range", sp[:,0].min(), sp[:,0].max(), "el range", sp[:,1].min(), sp[:,1].max(), "r", np.unique(sp[:,2]))
    print(" Listener", s.ListenerPosition, s.ListenerPosition_Type, "View", s.ListenerView, "Up", s.ListenerUp)
    print(" Receiver", s.ReceiverPosition.squeeze(), s.ReceiverPosition_Type)
    print(" Emitter", s.EmitterPosition.squeeze(), s.EmitterPosition_Type)
    # find left (az 90) & right (az 270) & front
    def nearest(az, el):
        d = np.hypot(((sp[:,0]-az+180)%360)-180, sp[:,1]-el); return int(np.argmin(d)), d.min()
    for name,(az,el) in {"front":(0,0),"left(90)":(90,0),"right(270)":(270,0),"top":(0,90),"back":(180,0)}.items():
        i, dd = nearest(az, el); h = ir[i]
        on = [int(np.argmax(np.abs(h[e]) > 0.1*np.abs(h[e]).max())) for e in range(2)]
        pk = [int(np.argmax(np.abs(h[e]))) for e in range(2)]
        rms = [float(np.sqrt(np.mean(h[e]**2))) for e in range(2)]
        print(f"  {name:10s} idx {i} ({sp[i,0]:.1f},{sp[i,1]:.1f}) err {dd:.2f} onset10% L/R {on} peak {pk} rms L/R {rms[0]:.4f}/{rms[1]:.4f} ILD {20*np.log10(rms[0]/rms[1]):+.1f} dB peakabs {np.abs(h).max():.3f}")
    # leading delay across the whole set
    thr = 0.1
    onsets = np.array([[int(np.argmax(np.abs(ir[i,e]) > thr*np.abs(ir[i,e]).max())) for e in range(2)] for i in range(ir.shape[0])])
    print("  onset(10%) min/median/max over set:", onsets.min(), int(np.median(onsets)), onsets.max())
    # energy outside first 256/ last taps
    L = ir.shape[2]; e_tot = np.sum(ir**2); 
    for n in (128, 192, 224, 256):
        if n <= L: print(f"  energy in first {n} taps: {100*np.sum(ir[:,:,:n]**2)/e_tot:.4f}%")
    # grid: unique elevations and counts
    els, cnt = np.unique(np.round(sp[:,1],1), return_counts=True)
    print("  unique elevations", len(els), "counts min/max", cnt.min(), cnt.max(), "els", els[:5], "...", els[-5:])
    print("  peak |IR| over set", np.abs(ir).max(), "mean rms", np.sqrt(np.mean(ir**2)))
import spaudiopy
from spaudiopy import io as sio, sig
print([n for n in dir(sio) if 'sofa' in n.lower() or 'hrir' in n.lower()])
print(inspect.signature(sig.HRIRs.__init__))
print(inspect.signature(sio.load_sofa_hrirs) if hasattr(sio,'load_sofa_hrirs') else 'no load_sofa_hrirs')
from spaudiopy import sph
print(inspect.signature(sph.sh_matrix)); print(inspect.getsource(sph.sh_matrix)[:1800])
