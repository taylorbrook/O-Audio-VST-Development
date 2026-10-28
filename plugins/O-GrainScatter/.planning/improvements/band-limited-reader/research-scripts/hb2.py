import numpy as np
from scipy import signal
def hb_kaiser(N,beta):
    n=np.arange(N)-(N-1)/2
    h=0.5*np.sinc(n/2)*np.kaiser(N,beta); h/=h.sum(); return h
def hb_remez(N,delta):
    # exact half-band via the Vaidyanathan-Nguyen trick: design g of length (N+1)/2 for [0,2fp]
    M=(N+1)//2
    g=signal.remez(M,[0,2*(0.25-delta),0.5,0.5],[1,0],weight=[1,1e-9],fs=1) if False else signal.remez(M,[0,2*(0.25-delta),0.4999,0.5],[1,0],fs=1)
    h=np.zeros(N); h[0::2]=g/2; h[(N-1)//2]=0.5; return h
for fs in [44.1,48]:
  delta=0.5*(0.5-20/fs)
  for N in [79,103,127,151]:
    best=None
    for beta in np.arange(4,14,0.25):
      h=hb_kaiser(N,beta); w,H=signal.freqz(h,worN=1<<15,fs=1)
      sb=20*np.log10(np.abs(H[w>=0.25+delta]).max())
      if best is None or sb<best[0]: best=(sb,beta)
    print(f"fs{fs} delta {delta:.4f} Kaiser half-band N={N} (nonzero {(N+1)//2+1}): best stopband {best[0]:6.1f} dB at beta {best[1]}, delay {(N-1)//2}")
