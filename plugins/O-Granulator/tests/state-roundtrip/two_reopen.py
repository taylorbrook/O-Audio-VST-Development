"""
O-Granulator 1.0.0 — ST4 (PLAN-4 P159, COMPAT-02): state round-trip across TWO
reopens on the INSTALLED VST3, through the host's own state path (pedalboard
raw_state; the JUCE-base64 IComponent framing — memory
pattern_pedalboard_vst3_raw_state_framing). Adapted from
plugins/O-Lyrica/tests/state-roundtrip/two_reopen.py.

  1. A fresh instance's state is edited: skewed params (s1_size, s1_density,
     lfo1_rate, fb_low_cut, env_release), choices (s1_scale, lfo1_shape,
     mod1_source / mod1_dest, sync_mode), a bool (s2_enable), a depth, the
     seed (> INT64_MAX as uint64), a 19-EDO custom scale, two degree masks and
     uiLanguage zh-Hans. It is loaded into instance B (reopen #1).
  2. B's saved state carries exactly ONE Engine / CustomScale / DegreeMasks
     child and the edited DECODED values (PARAM value attributes are the
     denormalised values; compared within 1e-6 relative — memory
     pattern_vst3_raw_value…: never compare raw_value).
  3. B's state is changed (22-EDO, a new seed, s1_size) and loaded into C
     (reopen #2). C holds the change and still one child of each kind.

Run (installed VST3 only):
  uv run --no-project --python 3.12 --with pedalboard \
      python plugins/O-Granulator/tests/state-roundtrip/two_reopen.py
"""
import os
import struct
import sys
import xml.etree.ElementTree as ET

import pedalboard

PLUGIN = os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-Granulator-dev.vst3")
if not os.path.exists(PLUGIN):
    PLUGIN = os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-Granulator.vst3")

MAGIC = b"VC2!"
B64 = ".ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+"


def juce_b64_decode(s: str) -> bytes:
    n, chars = s.split(".", 1)
    n = int(n)
    v = 0
    for i, ch in enumerate(chars):
        v |= B64.index(ch) << (6 * i)
    return v.to_bytes(n, "little") if n else b""


def juce_b64_encode(data: bytes) -> str:
    v = int.from_bytes(data, "little")
    groups = (len(data) * 8 + 5) // 6
    return f"{len(data)}." + "".join(B64[(v >> (6 * i)) & 63] for i in range(groups))


def unwrap_vc2(blob: bytes) -> bytes:
    assert blob[:4] == MAGIC, "no VC2! header"
    size = struct.unpack_from("<I", blob, 4)[0]
    return blob[8 : 8 + size]


def wrap_vc2(body: bytes) -> bytes:
    return MAGIC + struct.pack("<I", len(body)) + body


def split_state(raw: bytes):
    outer = ET.fromstring(unwrap_vc2(raw).rstrip(b"\0").decode("utf-8"))
    inner = unwrap_vc2(juce_b64_decode(outer.find("IComponent").text.strip()))
    return outer, inner.rstrip(b"\0").decode("utf-8")


def join_state(outer, xml_text: str) -> bytes:
    inner = wrap_vc2(xml_text.encode("utf-8") + b"\0")
    outer.find("IComponent").text = juce_b64_encode(inner)
    text = '<?xml version="1.0" encoding="UTF-8"?>\n' + ET.tostring(outer, encoding="unicode")
    return wrap_vc2(text.encode("utf-8") + b"\0")


SEED1 = 0xF00DFACECAFEBEEF            # > INT64_MAX as uint64 -> stored as a negative int64
SEED2 = (1 << 53) + 1
EDITS1 = {
    "s1_size": 600.0, "s1_density": 37.5, "lfo1_rate": 0.07, "fb_low_cut": 300.0, "env_release": 850.0,
    "s1_scale": 31.0, "lfo1_shape": 5.0, "mod1_source": 2.0, "mod1_dest": 7.0, "mod1_depth": -42.0,
    "sync_mode": 1.0, "s2_enable": 1.0, "feedback": 72.5, "fb_saturation": 35.0,
}


def i64(u):
    return u - (1 << 64) if u >= (1 << 63) else u


def edit(xml_text, params, seed, text, masks, lang=None):
    root = ET.fromstring(xml_text)
    for p in root.findall("PARAM"):
        if p.get("id") in params:
            p.set("value", repr(float(params[p.get("id")])))
    for tag in ("Engine", "CustomScale", "DegreeMasks"):
        for kid in root.findall(tag):
            if tag == "Engine":
                kid.set("seed", str(i64(seed)))
            elif tag == "CustomScale":
                kid.set("text", text)
                kid.set("name", text)
            else:
                for m in list(kid):
                    kid.remove(m)
                for (s, c, bits) in masks:
                    ET.SubElement(kid, "Mask", {"stream": str(s), "collection": str(c), "bits": f"{bits:016x}"})
    if lang is not None:
        root.set("uiLanguage", lang)
    return ET.tostring(root, encoding="unicode")


def read(plugin):
    _, text = split_state(plugin.raw_state)
    return ET.fromstring(text)


def params_of(root):
    return {p.get("id"): float(p.get("value")) for p in root.findall("PARAM")}


fails = []


def check(cond, label):
    print(("PASS " if cond else "FAIL ") + label)
    if not cond:
        fails.append(label)


def close(a, b):
    return abs(a - b) <= 1e-6 * max(1.0, abs(a))


def one_each(root, label):
    counts = [len(root.findall(t)) for t in ("Engine", "CustomScale", "DegreeMasks")]
    check(counts == [1, 1, 1], f"{label}: exactly one Engine / CustomScale / DegreeMasks child (got {counts})")
    check(len(root.findall("PARAM")) == 220, f"{label}: 220 PARAM children (got {len(root.findall('PARAM'))})")


print(f"plugin: {PLUGIN}")
fresh = pedalboard.load_plugin(PLUGIN)
outer, text = split_state(fresh.raw_state)

# Reopen #1
masks1 = [(1, 31, 0x24949), (3, 2, 0x55)]
b = pedalboard.load_plugin(PLUGIN)
b.raw_state = join_state(outer, edit(text, EDITS1, SEED1, "edo 19", masks1, lang="zh-Hans"))
root_b = read(b)
one_each(root_b, "reopen #1")
pb = params_of(root_b)
bad = [k for k, v in EDITS1.items() if not close(pb.get(k, float("nan")), v)]
check(not bad, f"reopen #1: {len(EDITS1)} edited params (skewed / choice / bool / depth) restored, decoded" + (f" BAD {bad}" if bad else ""))
eng = root_b.find("Engine")
check(eng is not None and int(eng.get("seed")) == i64(SEED1), "reopen #1: seed > INT64_MAX restored (int64 string)")
cs = root_b.find("CustomScale")
check(cs is not None and cs.get("text") == "edo 19", "reopen #1: custom scale edo 19 restored")
got = sorted((int(m.get("stream")), int(m.get("collection"))) for m in root_b.find("DegreeMasks"))
check(got == [(1, 31), (3, 2)], f"reopen #1: 2 non-default masks restored ({got})")
check(root_b.get("uiLanguage") == "zh-Hans", "reopen #1: uiLanguage zh-Hans restored")

# Change between reopens, then reopen #2
o2, t2 = split_state(b.raw_state)
edits2 = dict(EDITS1, s1_size=1234.0)
c = pedalboard.load_plugin(PLUGIN)
c.raw_state = join_state(o2, edit(t2, edits2, SEED2, "edo 22", masks1))
root_c = read(c)
one_each(root_c, "reopen #2")
pc = params_of(root_c)
check(close(pc.get("s1_size", 0.0), 1234.0), f"reopen #2: s1_size change survived ({pc.get('s1_size')})")
check(int(root_c.find("Engine").get("seed")) == i64(SEED2), "reopen #2: the changed seed survived (2^53 + 1)")
check(root_c.find("CustomScale").get("text") == "edo 22", "reopen #2: the changed scale survived (edo 22)")

print(f"\n{len(fails)} failure(s)")
sys.exit(1 if fails else 0)
