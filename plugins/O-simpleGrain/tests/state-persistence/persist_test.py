"""O-simpleGrain v1.5.6 source-persistence round trip, against the INSTALLED VST3.

Each case builds a session by editing the plugin's own state XML, loads it into a
FRESH instance (= a project reopened after a DAW restart), plays a note, and
identifies the source by its spectral peak. Sine sources at distinct frequencies
make the verdict unambiguous; the built-in Fire has no stable tonal peak.
"""
import base64, os, re, struct, sys, tempfile, io
import numpy as np
import soundfile as sf
import pedalboard
from pedalboard import load_plugin
from mido import Message

PLUGIN = os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3")
if not os.path.exists(PLUGIN):
    PLUGIN = os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-simpleGrain.vst3")
SR = 48000
fails = []

def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond: fails.append(msg)

def sine_wav(freq, secs=2.0, sr=44100, subtype="PCM_24", amp=0.5):
    t = np.arange(int(secs * sr)) / sr
    x = (amp * np.sin(2 * np.pi * freq * t)).astype(np.float32)
    b = io.BytesIO(); sf.write(b, np.stack([x, x], 1), sr, subtype=subtype, format="WAV")
    return b.getvalue()

# --- JUCE VST3 framing: outer copyXmlToBinary of <VST3PluginState><IComponent>SIZE.b64</IComponent>,
# where b64 is JUCE's MemoryBlock::toBase64Encoding (own alphabet, LSB-first bit packing), and the
# decoded component chunk is the plugin's own copyXmlToBinary ("VC2!" + u32 size + xml\0) + wrapper extras.
JT = ".ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+"
def juce_b64_decode(s):
    size, enc = s.split(".", 1)
    size = int(size); bits = 0; nb = 0; out = bytearray()
    for c in enc:
        bits |= JT.index(c) << nb; nb += 6
        while nb >= 8: out.append(bits & 0xFF); bits >>= 8; nb -= 8
    return bytes(out[:size]).ljust(size, b"\0")
def juce_b64_encode(b):
    n = (len(b) * 8 + 5) // 6; out = []; bits = 0; nb = 0
    for byte in b:
        bits |= byte << nb; nb += 8
        while nb >= 6: out.append(JT[bits & 63]); bits >>= 6; nb -= 6
    if len(out) < n: out.append(JT[bits & 63])
    return f"{len(b)}." + "".join(out)

def vc2_unpack(raw):
    assert raw[:4] == b"VC2!", raw[:8]
    size = struct.unpack("<I", raw[4:8])[0]
    return raw[8:8+size].decode("utf-8"), raw[8+size+1:]   # size excludes the trailing NUL
def vc2_pack(xml, tail=b""):
    body = xml.encode("utf-8")
    return b"VC2!" + struct.pack("<I", len(body)) + body + b"\0" + tail

def split_state(raw):
    outer, _ = vc2_unpack(raw)
    m = re.search(r"<IComponent>([^<]*)</IComponent>", outer)
    comp = juce_b64_decode(m.group(1))
    xml, tail = vc2_unpack(comp)
    return (outer, tail), xml, None

def join_state(pre, xml, post):
    outer, tail = pre
    comp = vc2_pack(xml, tail)
    return vc2_pack(re.sub(r"<IComponent>[^<]*</IComponent>",
                           lambda _: f"<IComponent>{juce_b64_encode(comp)}</IComponent>", outer))

def set_source(xml, identity, data_b64=None):
    xml = re.sub(r"<SOURCE[^>]*/>", "", xml)
    attrs = f'identity="{identity}"' + (f' data="{data_b64}"' if data_b64 else "")
    return xml.replace("</", f"<SOURCE {attrs}/></", 1) if False else re.sub(r"(</[^>]+>\s*)$", f"<SOURCE {attrs}/>\\1", xml)

def source_attrs(raw):
    _, xml, _ = split_state(raw)
    m = re.search(r"<SOURCE([^>]*)/>", xml)
    if not m: return {}
    return dict(re.findall(r'(\w+)="([^"]*)"', m.group(1)))

def fresh():
    p = load_plugin(PLUGIN)
    return p

def render(p):
    msgs = [Message("note_on", note=60, velocity=100, time=0.0),
            Message("note_off", note=60, velocity=0, time=1.5)]
    p.process([], 0.5, SR, num_channels=2, buffer_size=512, reset=True)
    y = p.process(msgs, 2.0, SR, num_channels=2, buffer_size=512, reset=False)
    return y

def peak_hz(y):
    m = y.mean(0) if y.ndim == 2 else y
    m = m[int(0.2*SR):int(1.4*SR)]
    if np.max(np.abs(m)) < 1e-5: return None
    spec = np.abs(np.fft.rfft(m * np.hanning(len(m))))
    f = np.fft.rfftfreq(len(m), 1/SR)
    k = np.argmax(spec[f > 50]) + np.searchsorted(f, 50, side="right")
    return f[k], spec[k] / (np.median(spec) + 1e-12)

def is_tone(y, hz, tol=0.05):   # grain AM sidebands sit ~20 Hz off the carrier
    r = peak_hz(y)
    if r is None: return False, "silent"
    f, prom = r
    return (abs(f - hz) / hz < tol and prom > 50), f"peak {f:.1f} Hz prominence {prom:.0f}"

base = fresh()
assert np.isfinite(render(base)).all()
pre, xml0, post = split_state(base.raw_state)
print("plugin:", PLUGIN, "| default SOURCE:", source_attrs(base.raw_state).get("identity"))

# Baseline: Fire is not a 440/660 tone (negative control for the detector).
y = render(fresh())
ok, info = is_tone(y, 440); check(not ok, f"negative control: default Fire is not a 440 Hz tone ({info})")

# 1. Dropped file, embedded copy (what a v1.5.6 session will hold after a drop).
wav440 = sine_wav(440)
st = join_state(pre, set_source(xml0, "dropped:sine440.wav", base64.b64encode(wav440).decode()), post)
p = fresh(); p.raw_state = st; y = render(p)
check(np.isfinite(y).all(), "case 1 output finite")
ok, info = is_tone(y, 440); check(ok, f"case 1: dropped + embedded copy restores the dropped sound ({info})")
a = source_attrs(p.raw_state)
check(a.get("identity") == "dropped:sine440.wav", f"case 1: identity survives ({a.get('identity')})")
blob = base64.b64decode(a.get("data", ""))
check(len(blob) > 0, f"case 1: re-saved state re-embeds the copy ({blob[:4]!r}, {len(blob)} bytes)")
# 1b. Second generation: the plugin's own re-saved state into another fresh instance.
p2 = fresh(); p2.raw_state = p.raw_state; y2 = render(p2)
ok, info = is_tone(y2, 440); check(ok, f"case 1b: plugin's own saved state restores in a fresh instance ({info})")
# 1c. FLAC copy is lossless vs the 24-bit source: renders match the first generation.
check(np.allclose(y, y2, atol=1e-6), f"case 1c: generation-2 render == generation-1 (max diff {np.max(np.abs(y-y2)):.2e})")

# 2. Long drop (30 s) -> stored copy is capped (~10 s), and still the right sound.
wavlong = sine_wav(660, secs=30.0)
st = join_state(pre, set_source(xml0, "dropped:long660.wav", base64.b64encode(wavlong).decode()), post)
p = fresh(); p.raw_state = st; y = render(p)
ok, info = is_tone(y, 660); check(ok, f"case 2: long drop restores ({info})")
lp = os.path.join(tempfile.mkdtemp(), "long660.wav"); open(lp, "wb").write(wavlong)
p = fresh(); p.raw_state = join_state(pre, set_source(xml0, lp), post)
ok, info = is_tone(render(p), 660); check(ok, f"case 2: long picker file loads ({info})")
blob = base64.b64decode(source_attrs(p.raw_state).get("data", ""))
check(blob[:4] == b"fLaC", f"case 2: freshly loaded 24-bit source is stored as FLAC ({blob[:4]!r})")
check(0 < len(blob) < len(wavlong) / 2, f"case 2: stored copy {len(blob)/1e6:.2f} MB < half of the 30 s file {len(wavlong)/1e6:.2f} MB")

# 3. Float WAV with peaks over 0 dBFS -> stored as float WAV, not clipped FLAC.
wavf = sine_wav(440, subtype="FLOAT", amp=1.5)
fp = os.path.join(tempfile.mkdtemp(), "hot.wav"); open(fp, "wb").write(wavf)
p = fresh(); p.raw_state = join_state(pre, set_source(xml0, fp), post)
blob = base64.b64decode(source_attrs(p.raw_state).get("data", ""))
check(blob[:4] == b"RIFF", f"case 3: float source stored as WAV ({blob[:4]!r})")
dec, _ = sf.read(io.BytesIO(blob), dtype="float32")
check(dec.max() > 1.4, f"case 3: stored float copy keeps the >0 dBFS peak ({dec.max():.3f})")

# 4. Legacy (v1.5.5) session: dropped, name only -> falls back to Fire, no crash.
st = join_state(pre, set_source(xml0, "dropped:gone.wav"), post)
p = fresh(); p.raw_state = st; y = render(p)
check(np.isfinite(y).all() and np.max(np.abs(y)) > 1e-4, "case 4: legacy name-only session plays (fallback)")
check(source_attrs(p.raw_state).get("identity") == "embedded:fire", f"case 4: falls back to embedded:fire ({source_attrs(p.raw_state).get('identity')})")
check("data" not in source_attrs(p.raw_state), "case 4: no stale data embedded after fallback")

# 5. Picker file: path present -> loads + embeds; file deleted -> embedded copy restores.
d = tempfile.mkdtemp()
path = os.path.join(d, "picked330.wav")
open(path, "wb").write(sine_wav(330))
st = join_state(pre, set_source(xml0, path), post)
p = fresh(); p.raw_state = st; y = render(p)
ok, info = is_tone(y, 330); check(ok, f"case 5a: legacy picker path (no copy) still loads ({info})")
saved = p.raw_state
check(base64.b64decode(source_attrs(saved).get("data", ""))[:4] == b"fLaC", "case 5a: re-saved picker session embeds a FLAC copy")
os.remove(path)
p = fresh(); p.raw_state = saved; y = render(p)
ok, info = is_tone(y, 330); check(ok, f"case 5b: picker file deleted -> embedded copy restores ({info})")
check(source_attrs(p.raw_state).get("identity") == path, "case 5b: identity keeps the original path")

# 5c. Path wins over copy when the file exists (file content changed to 550 Hz).
open(path, "wb").write(sine_wav(550))
p = fresh(); p.raw_state = saved; y = render(p)
ok, info = is_tone(y, 550); check(ok, f"case 5c: existing picker file takes precedence over the copy ({info})")

# 6. Back to a built-in: state carries no data.
st = join_state(pre, set_source(xml0, "embedded:water"), post)
p = fresh(); p.raw_state = st
a = source_attrs(p.raw_state)
check(a.get("identity") == "embedded:water" and "data" not in a, f"case 6: built-in session embeds nothing ({a})")

# 7. Corrupt base64 -> no crash, falls back.
st = join_state(pre, set_source(xml0, "dropped:bad.wav", "!!!notbase64@@@"), post)
p = fresh(); p.raw_state = st; y = render(p)
check(np.isfinite(y).all(), "case 7: corrupt data -> finite output")
check(source_attrs(p.raw_state).get("identity") == "embedded:fire", "case 7: corrupt data -> embedded:fire")

print(f"\n{len(fails)} FAIL(s)")
sys.exit(1 if fails else 0)
