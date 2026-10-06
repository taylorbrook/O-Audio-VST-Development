import numpy as np, wave, struct
N=524288; fs=48000; rng=np.random.default_rng(1)
t=np.arange(N)/fs
def w(name,x):
    x=x/np.max(np.abs(x))*0.98
    q=np.round(x*32767).astype('<i2')
    with wave.open(name,'wb') as f:
        f.setnchannels(1); f.setsampwidth(2); f.setframerate(fs); f.writeframes(q.tobytes())
# a) instrument-like: 220 Hz harmonic tone w/ vibrato, decaying upper partials, light noise
f0=220*(1+0.004*np.sin(2*np.pi*5.5*t)); ph=2*np.pi*np.cumsum(f0)/fs
a=sum((1/k)*np.sin(k*ph)*np.exp(-k*0.05) for k in range(1,40)) + 0.01*rng.standard_normal(N)
w('a_instrument.wav',a)
# b) white noise (worst case)
w('b_noise.wav',rng.standard_normal(N))
# c) smooth wavetable sweep: 256 frames of 2048, frame k = saw with k harmonics (Sine->Saw like)
fr=[]; n=np.arange(2048)
for k in range(256):
    H=1+int(k*31/255)
    fr.append(sum(np.sin(2*np.pi*h*n/2048)/h for h in range(1,H+1)))
w('c_wavetable.wav',np.concatenate(fr))
# d) speech-ish: noisy formant bursts
env=(np.sin(2*np.pi*3*t)>0).astype(float)*0.8+0.2
d=np.convolve(rng.standard_normal(N),np.hanning(64),'same')*env + 0.5*np.sin(2*np.pi*140*t)*env
w('d_speechish.wav',d)
