# O-simpleReverb — v3.0.0 family retrofit: v2.0.0 sessions load in v3 with identical values.
#
# Run (one process per VST3: both builds share the OuSr class ID):
#   uv run --no-project --python 3.12 --with pedalboard python \
#       plugins/O-simpleReverb/tests/pedalboard/verify_3_state.py dump  <v2.0.0.vst3> <state.json>
#   uv run ... verify_3_state.py check <v3.vst3> <state.json>
"""Independent measurer for the v3.0.0 state path, through a real host wrapper.

The retrofit keeps the parameter IDs, ranges, the OuSr identity and the preset-manager
XML, and adds ONE property (editorScale). So a session saved by v2.0.0 must restore in
v3 with every value identical, and the two non-parameter properties must round-trip.

Pedalboard exposes no property API, so this file decodes the VST3 state itself:
raw_state = <VST3PluginState><IComponent>SIZE.CHARS</IComponent>... where SIZE.CHARS is
JUCE's MemoryBlock::toBase64Encoding (decimal byte count, '.', then 6-bit groups
LSB-first over ".ABC...XYZabc...xyz0123456789+"); the decoded block is copyXmlToBinary's
"VC2!" magic + uint32 size + the XML text.

dump (v2.0.0 binary): set all 8 parameters to non-default values by raw_value (matched by
NAME), patch uiLanguage "en" -> "fr" into the XML, assign it back, and write the v2
raw_state plus the v2-decoded values to JSON.

check (v3 binary):
  S0  the committed v1.14.0 blob (tests/fixtures/state-v1.14.0.bin) wrapped into a v3
      raw_state: the re-saved PARAM values equal the fixture's, uiLanguage "fr" survives
  S1  the v2.0.0 raw_state assigned to v3: the 8 values pedalboard decodes equal v2's (by
      name), and the re-saved PARAM values equal v2's XML exactly
  S2  uiLanguage "fr" restored and re-saved
  S3  no editorScale in the re-save (v2 never wrote one, no editor was opened)
  S4  editorScale="1.250" patched in -> re-saved as "1.250"; "2.000" (out of range) and
      "abc" -> removed (fit-to-display), never re-saved
  S5  the 8 values unchanged through S2-S4

Nothing here shares code with the project harness or render-check.
"""
import json, os, re, struct, sys

ALPHA = ".ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+"
MAGIC = 0x21324356  # "VC2!"
HERE = os.path.dirname(os.path.abspath(__file__))
FIXTURE = os.path.join(HERE, "..", "fixtures", "state-v1.14.0.bin")

# APVTS ID -> pedalboard key (snake_case of the host NAME; pedalboard appends unit labels)
NAME_KEYS = {
    "TYPE": "type", "CHARACTER": "character", "WET": "wet", "DRY": "dry",
    "DECAY": "decay", "SIZE": "size", "LPFREQ": "low_cut_freq", "LPON": "low_cut_on",
}
# Non-default normalised values (none is a default; TYPE 0.8 = Plate, LPON on).
DUMP_NORM = {"TYPE": 0.8, "CHARACTER": 0.3125, "WET": 0.612, "DRY": 0.734,
             "DECAY": 0.82, "SIZE": 0.22, "LPFREQ": 0.307895, "LPON": 1.0}

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

def component_blob(raw):
    m = COMPONENT.search(raw)
    assert m, "no <IComponent> in raw_state"
    return b64_decode(m.group(1).decode("ascii"))

def blob_xml(blob):
    magic, size = struct.unpack("<II", blob[:8])
    assert magic == MAGIC, hex(magic)
    return blob[8:8 + size].decode("utf-8").rstrip("\0")

def component_xml(raw):
    return blob_xml(component_blob(raw))

def with_blob(raw, blob):
    m = COMPONENT.search(raw)
    return raw[:m.start(1)] + b64_encode(blob).encode("ascii") + raw[m.end(1):]

def with_component_xml(raw, xml):
    x = xml.encode("utf-8") + b"\0"
    return with_blob(raw, struct.pack("<II", MAGIC, len(x)) + x)

def xml_params(xml):
    return {m.group(1): m.group(2) for m in re.finditer(r'<PARAM id="([^"]+)" value="([^"]*)"', xml)}

def attr(xml, name):
    m = re.search(rf'\b{name}="([^"]*)"', xml)
    return m.group(1) if m else None

def resolve_keys(p):
    keys = {}
    for pid, base in NAME_KEYS.items():
        hits = [k for k in p.parameters.keys() if k == base or k.startswith(base + "_")]
        assert len(hits) == 1, f"{pid}: {hits} in {sorted(p.parameters.keys())}"
        keys[pid] = hits[0]
    return keys

def decoded(p, keys):
    return {pid: str(getattr(p, k)) for pid, k in keys.items()}

def dump(bundle, out_path):
    from pedalboard import load_plugin
    p = load_plugin(bundle)
    keys = resolve_keys(p)
    for pid, v in DUMP_NORM.items():
        p.parameters[keys[pid]].raw_value = float(v)
    xml = component_xml(p.raw_state)
    assert 'uiLanguage="en"' in xml, xml[:200]
    q = load_plugin(bundle)
    q.raw_state = with_component_xml(p.raw_state, xml.replace('uiLanguage="en"', 'uiLanguage="fr"', 1))
    raw = q.raw_state
    out = {"bundle": bundle, "raw_state": raw.decode("latin-1"), "xml": component_xml(raw), "decoded": decoded(q, resolve_keys(q))}
    json.dump(out, open(out_path, "w"), indent=1)
    print(f"v2 state: {out['decoded']}")
    print(f"v2 XML:   {out['xml'][:240]}...")
    print(f"wrote {out_path}")

def check_all(bundle, in_path):
    from pedalboard import load_plugin
    v2 = json.load(open(in_path))
    raw2 = v2["raw_state"].encode("latin-1")
    base = load_plugin(bundle)          # a v3 raw_state to wrap blobs into
    keys = resolve_keys(base)

    # ---------- S0: the committed v1.14.0 blob ----------
    fx = open(FIXTURE, "rb").read()
    fxml = blob_xml(fx)
    a = load_plugin(bundle)
    a.raw_state = with_blob(base.raw_state, fx)
    axml = component_xml(a.raw_state)
    check("S0 v1.14.0 blob -> v3: re-saved PARAM values equal the fixture's, uiLanguage \"fr\"",
          xml_params(axml) == xml_params(fxml) and attr(axml, "uiLanguage") == "fr",
          f"fixture {xml_params(fxml)} · v3 {xml_params(axml)} · uiLanguage {attr(axml, 'uiLanguage')!r}")

    # ---------- S1 ----------
    p = load_plugin(bundle)
    p.raw_state = raw2
    xml = component_xml(p.raw_state)
    got = decoded(p, keys)
    diff = {k: (v2["decoded"][k], got[k]) for k in got if got[k] != v2["decoded"][k]}
    check("S1a the 8 values decoded by name equal v2.0.0's", len(got) == 8 and not diff,
          f"{got}" if not diff else f"differences {diff}")
    check("S1b the re-saved PARAM values equal v2.0.0's XML exactly",
          xml_params(xml) == xml_params(v2["xml"]) and len(xml_params(xml)) == 8,
          f"{xml_params(xml)}" if xml_params(xml) == xml_params(v2["xml"]) else f"v2 {xml_params(v2['xml'])} vs v3 {xml_params(xml)}")

    # ---------- S2 / S3 ----------
    check("S2 uiLanguage \"fr\" restored by v3 and re-saved", attr(xml, "uiLanguage") == "fr",
          f"uiLanguage={attr(xml, 'uiLanguage')!r}")
    check("S3 no editorScale in the re-save (v2 wrote none, no editor opened)", attr(xml, "editorScale") is None,
          f"editorScale={attr(xml, 'editorScale')!r}")

    # ---------- S4 ----------
    def with_scale(v):
        return re.sub(r'<Parameters\b', f'<Parameters editorScale="{v}"', xml, count=1)
    r = load_plugin(bundle); r.raw_state = with_component_xml(p.raw_state, with_scale("1.250"))
    s_ok = attr(component_xml(r.raw_state), "editorScale")
    t = load_plugin(bundle); t.raw_state = with_component_xml(p.raw_state, with_scale("2.000"))
    s_hi = attr(component_xml(t.raw_state), "editorScale")
    u = load_plugin(bundle); u.raw_state = with_component_xml(p.raw_state, with_scale("abc"))
    s_bad = attr(component_xml(u.raw_state), "editorScale")
    check("S4 editorScale 1.250 round-trips; 2.000 and \"abc\" are dropped",
          s_ok == "1.250" and s_hi is None and s_bad is None,
          f"1.250 -> {s_ok!r}, 2.000 -> {s_hi!r}, abc -> {s_bad!r}")

    # ---------- S5 ----------
    d = {}
    for name, inst in (("S4 1.250", r), ("S4 2.000", t), ("S4 abc", u)):
        for k, v in decoded(inst, keys).items():
            if v != v2["decoded"][k]: d[f"{name} {k}"] = (v2["decoded"][k], v)
    check("S5 the 8 values unchanged through S2-S4", not d, f"differences: {d or 'none'}")

    print()
    print("ALL PASS" if not fails else f"{len(fails)} FAILED: {fails}")
    return 1 if fails else 0

if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "dump":
        dump(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 4 and sys.argv[1] == "check":
        sys.exit(check_all(sys.argv[2], sys.argv[3]))
    else:
        sys.exit(f"usage: {sys.argv[0]} dump <v2.vst3> <state.json> | check <v3.vst3> <state.json>")
