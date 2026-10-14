# O-simpleAmbisonics — Stage 3 verify: the two non-parameter state properties on the INSTALLED VST3.
# Run: uv run --no-project --python 3.12 --with pedalboard python plugins/O-simpleAmbisonics/tests/pedalboard/verify_3_state.py
"""Independent measurer for the Stage-3 state XML (uiLanguage / editorScale) — installed VST3 via pedalboard.

Why a NEW file: the Stage-2 round-trips (verify_2_1 V9, verify_2_2 V9, verify_2_3 V10) compare
decoded PARAMETER values only and stay Stage-2 verdict files (re-run, never edited). Pedalboard
exposes no property API, so this file decodes the VST3 state itself:

  raw_state = <VST3PluginState><IComponent>SIZE.CHARS</IComponent>...  where SIZE.CHARS is JUCE's
  MemoryBlock::toBase64Encoding — decimal byte count, '.', then 6-bit groups LSB-first over the
  alphabet ".ABC...XYZabc...xyz0123456789+". The decoded block is copyXmlToBinary's
  "VC2!" magic + uint32 size + the XML text.

  S1  a fresh instance's XML carries uiLanguage="en" and NO editorScale (no editor ever opened)
  S2  same-length patch uiLanguage="en" -> "fr", re-encode, assign raw_state, decode again -> "fr"
      (the installed binary restores the property and re-saves it)
  S3  the 12 parameter values (decoded by pedalboard) are unchanged through S2
  S4  (Stage 4) a fresh instance's XML carries currentPreset="Init"; patched to a 4-char user name
      "Test", re-encoded, assigned and decoded again -> "Test" (the installed binary restores and
      re-saves the preset name; no preset file is read — an unknown user name stays unnamed-safe)
  S5  (Stage 4) the currentPreset attribute removed -> round-trips as currentPreset="" (an older
      session is unnamed, never the ctor's Init); the 12 values unchanged through S4 / S5

Nothing here shares code with the project harness.
"""
import os, re, struct, sys
from pedalboard import load_plugin

PATH = os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-simpleAmbisonics-dev.vst3")
ALPHA = ".ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+"
MAGIC = 0x21324356  # "VC2!"

fails = []
def check(name, ok, detail=""):
    print(f"  [{'PASS' if ok else 'FAIL'}] {name} — {detail}")
    if not ok: fails.append(name)

def b64_decode(s):
    n, body = s.split(".", 1)
    n = int(n); out = bytearray(n); bit = 0
    for ch in body:
        v = ALPHA.index(ch)
        for k in range(6):
            if bit < n * 8 and (v >> k) & 1:
                out[bit >> 3] |= 1 << (bit & 7)
            bit += 1
    return bytes(out)

def b64_encode(b):
    nbits = len(b) * 8; chars = []
    for i in range((nbits + 5) // 6):
        v = 0
        for k in range(6):
            bit = i * 6 + k
            if bit < nbits and (b[bit >> 3] >> (bit & 7)) & 1:
                v |= 1 << k
        chars.append(ALPHA[v])
    return f"{len(b)}." + "".join(chars)

COMPONENT = re.compile(rb"<IComponent>([0-9]+\.[^<]*)</IComponent>")

def component_xml(raw):
    m = COMPONENT.search(raw)
    assert m, "no <IComponent> in raw_state"
    blob = b64_decode(m.group(1).decode("ascii"))
    magic, size = struct.unpack("<II", blob[:8])
    assert magic == MAGIC, hex(magic)
    return blob[8:8 + size].decode("utf-8").rstrip("\0")

def with_component_xml(raw, xml):
    x = xml.encode("utf-8") + b"\0"
    blob = struct.pack("<II", MAGIC, len(x)) + x
    m = COMPONENT.search(raw)
    return raw[:m.start(1)] + b64_encode(blob).encode("ascii") + raw[m.end(1):]

# pedalboard also exposes the host's own bypass; compare the 12 APVTS parameters only.
def params(p):
    return {k: getattr(p, k) for k in sorted(p.parameters.keys()) if "bypass" not in k.lower()}

# ---------- S1 ----------
p = load_plugin(PATH)
xml = component_xml(p.raw_state)
lang = re.search(r'uiLanguage="([^"]*)"', xml)
check("S1 fresh instance: uiLanguage=\"en\", no editorScale",
      lang is not None and lang.group(1) == "en" and "editorScale" not in xml,
      f"uiLanguage={lang.group(1) if lang else None!r}, editorScale present: {'editorScale' in xml}")

# ---------- S2 ----------
before = params(p)
patched = xml.replace('uiLanguage="en"', 'uiLanguage="fr"', 1)
q = load_plugin(PATH)
q.raw_state = with_component_xml(p.raw_state, patched)
xml2 = component_xml(q.raw_state)
lang2 = re.search(r'uiLanguage="([^"]*)"', xml2)
check("S2 uiLanguage \"fr\" restored by the installed binary and re-saved",
      lang2 is not None and lang2.group(1) == "fr",
      f"re-saved uiLanguage={lang2.group(1) if lang2 else None!r}")

# ---------- S3 ----------
after = params(q)
diff = {k: (before[k], after[k]) for k in before if str(before[k]) != str(after[k])}
check("S3 the 12 parameter values unchanged through S2", len(before) == 12 and not diff,
      f"{len(before)} parameters, differences: {diff or 'none'}")

# ---------- S4 (Stage 4) ----------
cp = re.search(r'currentPreset="([^"]*)"', xml)
check("S4a fresh instance: currentPreset=\"Init\"", cp is not None and cp.group(1) == "Init",
      f"currentPreset={cp.group(1) if cp else None!r}")
r = load_plugin(PATH)
r.raw_state = with_component_xml(p.raw_state, xml.replace('currentPreset="Init"', 'currentPreset="Test"', 1))
cp4 = re.search(r'currentPreset="([^"]*)"', component_xml(r.raw_state))
check("S4b currentPreset \"Test\" restored by the installed binary and re-saved",
      cp4 is not None and cp4.group(1) == "Test", f"re-saved currentPreset={cp4.group(1) if cp4 else None!r}")

# ---------- S5 (Stage 4) ----------
t = load_plugin(PATH)
t.raw_state = with_component_xml(p.raw_state, re.sub(r'\s*currentPreset="[^"]*"', "", xml, count=1))
cp5 = re.search(r'currentPreset="([^"]*)"', component_xml(t.raw_state))
check("S5 currentPreset absent -> round-trips as currentPreset=\"\" (unnamed)",
      cp5 is not None and cp5.group(1) == "", f"re-saved currentPreset={cp5.group(1) if cp5 else None!r}")
d45 = {k: (before[k], v) for k, v in {**params(r), **{}}.items() if str(before[k]) != str(v)}
d5 = {k: (before[k], v) for k, v in params(t).items() if str(before[k]) != str(v)}
check("S5b the 12 parameter values unchanged through S4 / S5", not d45 and not d5,
      f"differences: {d45 or d5 or 'none'}")

print()
print("ALL PASS" if not fails else f"{len(fails)} FAILED: {fails}")
sys.exit(1 if fails else 0)
