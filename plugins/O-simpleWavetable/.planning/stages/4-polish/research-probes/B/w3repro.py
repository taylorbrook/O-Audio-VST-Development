# W3 repro on the INSTALLED VST3: Poly wheel moves, then Poly->Mono, then a Mono note.
import sys, numpy as np, mido, pedalboard
P = sys.argv[1]
SR = 48000
p = pedalboard.load_plugin(P)
keys = sorted(p.parameters.keys())
base = {k: p.parameters[k].raw_value for k in keys}

def f0(y, t0, t1):
    x = y[0, int(t0*SR):int(t1*SR)].astype(np.float64)
    s = np.signbit(x); idx = np.where(s[:-1] & ~s[1:])[0]       # rising zero crossings
    fr = idx + x[idx] / (x[idx] - x[idx+1])
    return (len(fr) - 1) / ((fr[-1] - fr[0]) / SR)

def case(name, pre_msgs, expect_hz, seed_wheel_note=True):
    for k in keys: p.parameters[k].raw_value = base[k]
    p.parameters['voice_mode'].raw_value = 0.0                  # Poly
    p.parameters['position'].raw_value = 0.0                    # Sine -> Saw pos 0 = sine
    p.reset()
    p(pre_msgs, duration=0.6, sample_rate=SR, num_channels=2, reset=False)
    p.parameters['voice_mode'].raw_value = 1.0                  # Poly -> Mono (next block)
    p([], duration=0.1, sample_rate=SR, num_channels=2, reset=False)
    y = p([mido.Message('note_on', note=69, velocity=100, time=0.0)], duration=1.0, sample_rate=SR, num_channels=2, reset=False)
    f = f0(y, 0.2, 0.9)
    c = 1200*np.log2(f/expect_hz)
    print(f"{name}: measured {f:.3f} Hz, expected {expect_hz:.3f} Hz, error {c:+.2f} c")

# A: critic scenario - wheel up while voice 0 plays, release, wheel back to centre while idle
case('A wheel up -> note off -> centre (idle) -> Mono', [
    mido.Message('note_on', note=69, velocity=100, time=0.0),
    mido.Message('pitchwheel', pitch=8191, time=0.1),
    mido.Message('note_off', note=69, time=0.15),
    mido.Message('pitchwheel', pitch=0, time=0.55)], 440.0)
# B: wheel moved with no note at all (voice 0 never saw it), left at -1 st
case('B wheel -4096 idle -> Mono', [mido.Message('pitchwheel', pitch=-4096, time=0.1)], 440.0*2**(-1/12))
# C control: no wheel activity
case('C control (no wheel)', [], 440.0)
