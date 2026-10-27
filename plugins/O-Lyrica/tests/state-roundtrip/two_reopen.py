"""
O-Lyrica v2.6.4 — glissando state round-trip across TWO reopens.

Hosts the INSTALLED VST3 in pedalboard and drives state through raw_state:

  1. A fresh instance's state is edited to a 19-EDO custom tuning plus a custom
     glissando degree mask, then loaded into instance B (reopen #1).
  2. B's saved state must carry exactly ONE <CustomState> child (v2.6.3 left the
     restored one inside the live tree, so every save after a reopen carried two
     and the second reopen read the stale first one).
  3. B's state is edited to 31-EDO with a different mask and loaded into
     instance C (reopen #2). C must report 31-EDO and the second mask — the
     change made between the reopens survives.
  4. A session written before v2.6.4 (no glissMaskDegreeCount attribute) keeps
     its mask.

Run:
  uv run --no-project --python 3.12 --with pedalboard \
      python plugins/O-Lyrica/tests/state-roundtrip/two_reopen.py
"""
import json
import os
import re
import struct
import sys
import xml.etree.ElementTree as ET

import pedalboard

PLUGIN = os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3")
if not os.path.exists(PLUGIN):
    PLUGIN = os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-Lyrica.vst3")

MAGIC = b"VC2!"
B64 = ".ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+"


# juce::MemoryBlock::toBase64Encoding — "<numBytes>.<6-bit groups, LSB-first>"
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
    """raw_state = VC2!(<VST3PluginState><IComponent>b64(VC2!(plugin xml))</IComponent>…)
    Returns (outer_root, plugin_xml_text)."""
    outer = ET.fromstring(unwrap_vc2(raw).rstrip(b"\0").decode("utf-8"))
    inner = unwrap_vc2(juce_b64_decode(outer.find("IComponent").text.strip()))
    return outer, inner.rstrip(b"\0").decode("utf-8")


def join_state(outer, xml_text: str) -> bytes:
    inner = wrap_vc2(xml_text.encode("utf-8") + b"\0")
    outer.find("IComponent").text = juce_b64_encode(inner)
    text = '<?xml version="1.0" encoding="UTF-8"?>\n' + ET.tostring(outer, encoding="unicode")
    return wrap_vc2(text.encode("utf-8") + b"\0")


def edo(n):
    return [1200.0 * k / n for k in range(0, n + 1)]  # stored as 0 … 1200, n + 1 entries


def set_tuning(xml_text, n, mask, count_attr=True):
    root = ET.fromstring(xml_text)
    for cs in root.findall("CustomState"):
        root.remove(cs)
    cs = ET.SubElement(root, "CustomState")
    cs.set("data", json.dumps({
        "intervals": edo(n), "scaleName": f"{n}-EDO", "tonic": 0,
        "presetIndex": 0, "octaveStretch": 0.0, "tuningMode": 0,
    }))
    root.set("glissCustomDegrees", str(mask))
    if count_attr:
        root.set("glissMaskDegreeCount", str(n))
    elif "glissMaskDegreeCount" in root.attrib:
        del root.attrib["glissMaskDegreeCount"]
    return ET.tostring(root, encoding="unicode")


def read(plugin):
    _, text = split_state(plugin.raw_state)
    return ET.fromstring(text)


def custom(root):
    kids = root.findall("CustomState")
    return kids, (json.loads(kids[0].get("data")) if kids else None)


fails = []


def check(cond, label):
    print(("PASS " if cond else "FAIL ") + label)
    if not cond:
        fails.append(label)


fresh = pedalboard.load_plugin(PLUGIN)
outer, text = split_state(fresh.raw_state)

# Reopen #1 — 19-EDO, mask = degrees 0, 2, 5, 18
mask1 = (1 << 0) | (1 << 2) | (1 << 5) | (1 << 18)
b = pedalboard.load_plugin(PLUGIN)
b.raw_state = join_state(outer, set_tuning(text, 19, mask1))
root_b = read(b)
kids, data = custom(root_b)
check(len(kids) == 1, f"reopen #1 save carries one <CustomState> (got {len(kids)})")
check(data and len(data["intervals"]) == 20, "reopen #1 restored 19-EDO")
check(int(root_b.get("glissCustomDegrees")) == mask1, "reopen #1 mask restored")
check(root_b.get("glissMaskDegreeCount") == "19", "reopen #1 mask bound to 19 degrees")
check(data and int(data.get("glissCustomDegrees", "0")) == mask1, "mask rides the preset customState")

# Change between reopens: 31-EDO, mask = degrees 1, 30 — then reopen #2
mask2 = (1 << 1) | (1 << 30)
o2, t2 = split_state(b.raw_state)
c = pedalboard.load_plugin(PLUGIN)
c.raw_state = join_state(o2, set_tuning(t2, 31, mask2))
root_c = read(c)
kids, data = custom(root_c)
check(len(kids) == 1, f"reopen #2 save carries one <CustomState> (got {len(kids)})")
check(data and len(data["intervals"]) == 32, "reopen #2 restored 31-EDO (the change survived)")
check(int(root_c.get("glissCustomDegrees")) == mask2, "reopen #2 mask restored")

# Pre-2.6.4 session: no size attribute -> mask kept, bound to the restored tuning
d = pedalboard.load_plugin(PLUGIN)
d.raw_state = join_state(outer, set_tuning(text, 19, mask1, count_attr=False))
root_d = read(d)
check(int(root_d.get("glissCustomDegrees")) == mask1, "legacy session keeps its mask")
check(root_d.get("glissMaskDegreeCount") == "19", "legacy mask bound to restored size")

print(f"\n{len(fails)} failure(s)")
sys.exit(1 if fails else 0)
