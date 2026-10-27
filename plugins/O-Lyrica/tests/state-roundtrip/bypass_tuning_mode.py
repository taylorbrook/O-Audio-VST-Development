"""
O-Lyrica v2.6.5 — tuning mode survives a save made while the engine is stale.

tuningMode reaches TuningEngine only in processBlock. Up to v2.6.4 the saved
<CustomState> took the mode from the engine and the restore forced the param
from it, so a mode changed while bypassed (processBlockBypassed runs instead)
or before the first block came back as 12-TET.

Hosts the INSTALLED VST3 in pedalboard. No audio is processed, so the engine
never sees the change — the same stale state bypass produces.

  1. Change tuningMode with no block run; the save must carry the new mode in
     both the PARAM and the CustomState, and a reopen must restore it.
  2. A <= v2.6.4 session saved stale (PARAM = Custom, CustomState = 12-TET)
     reopens as Custom — the param outranks the record.
  3. A session with no tuningMode PARAM still takes the CustomState mode.

Run:
  uv run --no-project --python 3.12 --with pedalboard \
      python plugins/O-Lyrica/tests/state-roundtrip/bypass_tuning_mode.py
"""
import json
import os
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


fails = []


def check(cond, label):
    print(("PASS " if cond else "FAIL ") + label)
    if not cond:
        fails.append(label)


def mode_of(root):
    param = next((p for p in root.findall("PARAM") if p.get("id") == "tuningMode"), None)
    cs = root.find("CustomState")
    cs_mode = json.loads(cs.get("data")).get("tuningMode") if cs is not None else None
    return (round(float(param.get("value"))) if param is not None else None), cs_mode


def read(plugin):
    return ET.fromstring(split_state(plugin.raw_state)[1])


def tuning_mode_param(plugin):
    for p in plugin.parameters.values():
        if p.name.lower().replace(" ", "") == "tuningmode":
            return p
    raise SystemExit("tuningMode parameter not found")


# 1. Change with no block processed (engine stale), save, reopen
a = pedalboard.load_plugin(PLUGIN)
tuning_mode_param(a).raw_value = 1.0  # choice of 3: 1.0 = index 2 (Custom)
root_a = read(a)
check(mode_of(root_a) == (2, 2), f"stale-engine save carries mode 2 in PARAM and CustomState (got {mode_of(root_a)})")

b = pedalboard.load_plugin(PLUGIN)
b.raw_state = a.raw_state
check(mode_of(read(b)) == (2, 2), f"reopen restores mode 2 (got {mode_of(read(b))})")

# 2. Legacy stale session: PARAM says 2, CustomState says 0
outer, text = split_state(a.raw_state)
root = ET.fromstring(text)
cs = root.find("CustomState")
data = json.loads(cs.get("data"))
data["tuningMode"] = 0
cs.set("data", json.dumps(data))
c = pedalboard.load_plugin(PLUGIN)
c.raw_state = join_state(outer, ET.tostring(root, encoding="unicode"))
check(mode_of(read(c)) == (2, 2), f"legacy stale session reopens as mode 2 (got {mode_of(read(c))})")

# 3. No tuningMode PARAM: CustomState is the fallback
root = ET.fromstring(text)
for p in [p for p in root.findall("PARAM") if p.get("id") == "tuningMode"]:
    root.remove(p)
cs = root.find("CustomState")
data = json.loads(cs.get("data"))
data["tuningMode"] = 1
cs.set("data", json.dumps(data))
d = pedalboard.load_plugin(PLUGIN)
d.raw_state = join_state(outer, ET.tostring(root, encoding="unicode"))
check(mode_of(read(d)) == (1, 1), f"param-less session takes CustomState mode 1 (got {mode_of(read(d))})")

print(f"\n{len(fails)} failure(s)")
sys.exit(1 if fails else 0)
