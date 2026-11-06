#!/usr/bin/env python3
# ==============================================================================
#   O-Granulator — session-codec byte dump (PLAN-3.1 P98 / risk "codec refactor").
#
#   usage (from the repo root, on the INSTALLED VST3):
#     uv run --no-project --python 3.12 --with pedalboard --with numpy \
#         python plugins/O-Granulator/tests/hostcache/codec_bytes.py OUT_DIR [path/to/O-Granulator*.vst3]
#
#   Injects fixed session states (non-default params, seed > INT64_MAX, custom
#   scales, high-bit masks, uiLanguage; plus a stripped state with no custom
#   children) into FRESH instances through raw_state (setStateInformation), reads
#   the plugin's own state back (getStateInformation), and writes the inner plugin
#   XML of every fixture to OUT_DIR/<name>.xml. Two-reopen fixtures chain the
#   read-back into a second fresh instance.
#
#   Run it on the installed plugin before and after a codec change and
#   `diff -r` the two OUT_DIRs: empty = the session path is byte-for-byte
#   unchanged. Nothing random is in the output (every fixture sets the seed,
#   except "stripped", which keeps the injecting instance's seed out of the
#   dump by masking the seed attribute).
# ==============================================================================
import ctypes, glob, os, re, struct, sys
import numpy as np
from pedalboard import load_plugin

_cf = ctypes.CDLL("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")
_cf.CFRunLoopRunInMode.argtypes = [ctypes.c_void_p, ctypes.c_double, ctypes.c_bool]
_cf.CFRunLoopRunInMode.restype = ctypes.c_int32
_mode = ctypes.c_void_p.in_dll(_cf, "kCFRunLoopDefaultMode")
def pump(seconds=0.2):
    _cf.CFRunLoopRunInMode(_mode, seconds, False)

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
    return raw[8:8 + size].decode("utf-8"), raw[8 + size + 1:]   # size excludes the NUL
def vc2_pack(xml, tail=b""):
    body = xml.encode("utf-8")
    return b"VC2!" + struct.pack("<I", len(body)) + body + b"\0" + tail
def split_state(raw):
    outer, _ = vc2_unpack(raw)
    m = re.search(r"<IComponent>([^<]*)</IComponent>", outer)
    xml, tail = vc2_unpack(juce_b64_decode(m.group(1)))
    return (outer, tail), xml
def join_state(pre, xml):
    outer, tail = pre
    comp = vc2_pack(xml, tail)
    return vc2_pack(re.sub(r"<IComponent>[^<]*</IComponent>",
                           lambda _: f"<IComponent>{juce_b64_encode(comp)}</IComponent>", outer))

def strip_custom(xml):
    xml = re.sub(r"<Engine[^>]*/>", "", xml)
    xml = re.sub(r"<CustomScale[^>]*/>", "", xml)
    xml = re.sub(r"<DegreeMasks\s*/>", "", xml)
    xml = re.sub(r"<DegreeMasks>.*?</DegreeMasks>", "", xml, flags=re.S)
    return re.sub(r'\s+uiLanguage="[^"]*"', "", xml)

def set_param(xml, pid, value):
    return re.sub(rf'(<PARAM id="{re.escape(pid)}" value=")[^"]*(")', rf"\g<1>{value}\g<2>", xml)

def add_custom(xml, seed, lang, name, text, masks):
    xml = strip_custom(xml)
    ms = "".join(f'<Mask stream="{s}" collection="{c}" bits="{b}"/>' for s, c, b in masks)
    kids = (f'<Engine seed="{seed}" stateVersion="1" modDestTableVersion="1"/>'
            f'<CustomScale name="{name}" text="{text}" period="1200.0" version="7"/>'
            + (f"<DegreeMasks>{ms}</DegreeMasks>" if masks else "<DegreeMasks/>"))
    xml = re.sub(r"<(OGranulator)\b", rf'<\1 uiLanguage="{lang}"', xml, count=1)
    return re.sub(r"(</OGranulator>\s*)$", kids + r"\1", xml)

def main():
    if len(sys.argv) < 2:
        print(__doc__); return 2
    out = sys.argv[1]
    path = sys.argv[2] if len(sys.argv) > 2 else sorted(glob.glob(os.path.expanduser(
        "~/Library/Audio/Plug-Ins/VST3/O-Granulator*.vst3")))[0]
    os.makedirs(out, exist_ok=True)
    sr = 48000.0
    silence = np.zeros((2, 4800), dtype=np.float32)

    def fresh():
        p = load_plugin(path)
        p.process(silence, sr, reset=True)
        return p
    def inject(xml):
        p = fresh()
        pre, _ = split_state(p.raw_state)
        p.raw_state = join_state(pre, xml)
        pump()
        p.process(silence, sr, reset=False)
        pump()
        return p

    base = fresh()
    _, xml0 = split_state(base.raw_state)
    xml0 = set_param(xml0, "s3_pitch", "7.5")
    xml0 = set_param(xml0, "mod2_dest", "17")
    xml0 = set_param(xml0, "feedback", "33.0")

    fixtures = {
        "f1-edo19-highmask-zh": add_custom(xml0, "-1147221449397142545", "zh-Hans", "Nineteen", "edo 19",
                                           [(2, 23, "8000000000000001"), (4, 1, "0000000000000aaa")]),
        "f2-edo64-fr": add_custom(xml0, "42", "fr", "", "edo 64", [(1, 31, "8000000000000001")]),
        "f3-ratios-intlang": add_custom(xml0, "-1", "2", "Pelog-ish", "120 270 540 670 785 950 1200",
                                        [(1, 19, "0000000000000007"), (3, 5, "FFFFFFFFFFFFFFF3")]),
        "f4-badscale-badmask": add_custom(xml0, "notanumber", "xx", "Broken", "1/0 garbage",
                                          [(9, 1, "1"), (1, 40, "0000000000000001"), (2, 2, "zz")]),
    }
    for name, xml in fixtures.items():
        p = inject(xml)
        _, got = split_state(p.raw_state)
        if name == "f4-badscale-badmask":   # bad seed keeps the instance's random seed
            got = re.sub(r'seed="[^"]*"', 'seed="<instance>"', got)
        open(os.path.join(out, name + ".xml"), "w", newline="\n").write(got)
        # two reopens: read-back -> fresh instance -> read-back
        q = inject(got if name != "f4-badscale-badmask" else split_state(p.raw_state)[1])
        _, got2 = split_state(q.raw_state)
        if name == "f4-badscale-badmask":
            got2 = re.sub(r'seed="[^"]*"', 'seed="<instance>"', got2)
        open(os.path.join(out, name + "-reopen2.xml"), "w", newline="\n").write(got2)
        print(f"  {name}: {len(got)} chars, reopen2 {'==' if got2 == got else '!='} first")

    p = inject(strip_custom(xml0))
    _, got = split_state(p.raw_state)
    got = re.sub(r'seed="[^"]*"', 'seed="<instance>"', got)
    open(os.path.join(out, "f5-stripped.xml"), "w", newline="\n").write(got)
    print(f"  f5-stripped: {len(got)} chars")
    print(f"wrote {len(fixtures) * 2 + 1} fixtures to {out}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
