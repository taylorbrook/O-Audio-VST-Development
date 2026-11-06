#!/usr/bin/env python3
# ==============================================================================
#   O-Granulator — HA host-automation cache gate (PLAN-2.5a P57, T13; 2.5b P78).
#
#   usage (from the repo root, on the INSTALLED VST3):
#     uv run --no-project --python 3.12 --with pedalboard --with numpy \
#         python plugins/O-Granulator/tests/hostcache/run.py [path/to/O-Granulator*.vst3]
#
#   Sets every modM_source / dest / depth, lfoN_shape / rate / rate_sync / phase,
#   sync_mode and macroN — and (2.5b, P78) scale_root, onset_threshold,
#   sN_onset_trigger ×4 and pitch_follow_sN ×4 — through the host's normalised
#   raw_value, renders 2 s,
#   pumps the CFRunLoop (JUCE's async parameter / state machinery), renders
#   again, then checks:
#     HA1  the host cache (decoded values) equals what the host set, read right
#          after the set (memory: raw_value is the cached UN-snapped host float —
#          compare DECODED values, source-after-set vs later);
#     HA2  every OTHER parameter's host value is unchanged (no nested writes —
#          memory: parameterChanged writing another param never reaches the host);
#     HA3  the plugin's own state XML (raw_state → VC2! → <IComponent> JUCE base64
#          → plugin VC2!) holds the set values (choices exact, floats ≤ 1e-3 of span,
#          the skewed LFO rate ≤ 1e-3 relative).
#   Exit 0 = all three PASS. Nothing in 2.5a / 2.5b writes a parameter (the
#   engine never writes APVTS), so this should pass trivially; it is still run.
# ==============================================================================
import ctypes, glob, os, re, struct, sys
import numpy as np
from pedalboard import load_plugin

# ── CFRunLoop pump (ctypes, no PyObjC) ───────────────────────────────────────
_cf = ctypes.CDLL("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")
_cf.CFRunLoopRunInMode.argtypes = [ctypes.c_void_p, ctypes.c_double, ctypes.c_bool]
_cf.CFRunLoopRunInMode.restype = ctypes.c_int32
_mode = ctypes.c_void_p.in_dll(_cf, "kCFRunLoopDefaultMode")
def pump(seconds=0.3):
    _cf.CFRunLoopRunInMode(_mode, seconds, False)

# ── JUCE VST3 state framing (O-simpleGrain persist_test.py codec) ───────────
JT = ".ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+"
def juce_b64_decode(s):
    size, enc = s.split(".", 1)
    size = int(size); bits = 0; nb = 0; out = bytearray()
    for c in enc:
        bits |= JT.index(c) << nb; nb += 6
        while nb >= 8: out.append(bits & 0xFF); bits >>= 8; nb -= 8
    return bytes(out[:size]).ljust(size, b"\0")
def vc2_unpack(raw):
    assert raw[:4] == b"VC2!", raw[:8]
    size = struct.unpack("<I", raw[4:8])[0]
    return raw[8:8 + size].decode("utf-8"), raw[8 + size + 1:]
def plugin_xml(raw):
    outer, _ = vc2_unpack(raw)
    m = re.search(r"<IComponent>([^<]*)</IComponent>", outer)
    xml, _ = vc2_unpack(juce_b64_decode(m.group(1)))
    return xml
def xml_params(xml):
    return {m.group(1): float(m.group(2)) for m in re.finditer(r'<PARAM id="([^"]+)" value="([^"]+)"', xml)}

# ── the parameter set under test (ids + host names, GranulatorParamIds.h) ───
LFO_SHAPES = 6; MOD_SOURCES = 12; MOD_DESTS = 113; RATE_SYNCS = 22
def targets():
    t = [("sync_mode", "Sync Mode", "choice", 2, 1)]
    for n in range(1, 5):
        t += [(f"lfo{n}_shape", f"LFO {n} · Shape", "choice", LFO_SHAPES, (n + 2) % LFO_SHAPES),
              (f"lfo{n}_rate", f"LFO {n} · Rate", "skewed", (0.01, 20.0), [0.07, 1.3, 4.2, 15.0][n - 1]),
              (f"lfo{n}_rate_sync", f"LFO {n} · Rate Sync", "choice", RATE_SYNCS, [3, 9, 15, 21][n - 1]),
              (f"lfo{n}_phase", f"LFO {n} · Phase", "linear", (0.0, 360.0), 45.0 * n),
              (f"macro{n}", f"Macro {n}", "linear", (0.0, 100.0), 20.0 * n + 3.0)]
    for m in range(1, 17):
        t += [(f"mod{m}_source", f"Mod {m} · Source", "choice", MOD_SOURCES, m % MOD_SOURCES),
              (f"mod{m}_dest", f"Mod {m} · Dest", "choice", MOD_DESTS, (7 * m) % MOD_DESTS),
              (f"mod{m}_depth", f"Mod {m} · Depth", "linear", (-100.0, 100.0), -90.0 + 11.0 * m)]
    # 2.5b (P78): +10 → HA1 79, HA2 142 untouched.
    t += [("scale_root", "Scale Root", "choice", 12, 7),
          ("onset_threshold", "Onset Threshold", "linear", (-60.0, 0.0), -24.0)]
    for n in range(1, 5):
        t += [(f"s{n}_onset_trigger", f"S{n} · Onset Trigger", "choice", 2, n % 2),
              (f"pitch_follow_s{n}", f"Pitch Follow S{n}", "choice", 2, (n + 1) % 2)]
    return t

def main():
    path = sys.argv[1] if len(sys.argv) > 1 else None
    if path is None:
        found = sorted(glob.glob(os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-Granulator*.vst3")))
        if not found:
            print("HA: no installed O-Granulator VST3"); return 2
        path = found[0]
    print(f"HA host-cache gate on {path}")
    plug = load_plugin(path)
    params = plug.parameters
    by_name = {p.name: k for k, p in params.items()}
    sr = 48000.0
    silence = np.zeros((2, int(sr)), dtype=np.float32)
    noise = (np.random.default_rng(7).uniform(-0.3, 0.3, (2, int(2 * sr)))).astype(np.float32)

    plug.process(silence, sr, reset=True)
    tg = targets()
    keys = {}
    for pid, name, kind, rng, val in tg:
        k = by_name.get(name)
        if k is None:
            print(f"  [FAIL] HA0 host name '{name}' ({pid}) not found"); return 1
        keys[pid] = k
    touched = set(keys.values())
    before_other = {k: p.raw_value for k, p in params.items() if k not in touched}

    set_decoded = {}
    for pid, name, kind, rng, val in tg:
        p = params[keys[pid]]
        if kind == "choice":
            p.raw_value = val / (rng - 1)
        elif kind == "linear":
            p.raw_value = (val - rng[0]) / (rng[1] - rng[0])
        else:   # skewed: bisect raw_value on the decoded read-back
            lo, hi = 0.0, 1.0
            for _ in range(60):
                mid = 0.5 * (lo + hi); p.raw_value = mid
                if float(getattr(plug, keys[pid])) < val: lo = mid
                else: hi = mid
            p.raw_value = hi
        set_decoded[pid] = getattr(plug, keys[pid])

    plug.process(noise, sr, reset=False)
    pump()
    plug.process(noise, sr, reset=False)
    pump()

    bad1 = [(pid, set_decoded[pid], getattr(plug, keys[pid])) for pid, *_ in tg if getattr(plug, keys[pid]) != set_decoded[pid]]
    print(f"  [{'PASS' if not bad1 else 'FAIL'}] HA1 host cache == the set values after render + CFRunLoop pump ({len(tg) - len(bad1)}/{len(tg)})"
          + (f"  e.g. {bad1[:3]}" if bad1 else ""))
    bad2 = [k for k, v in before_other.items() if params[k].raw_value != v]
    print(f"  [{'PASS' if not bad2 else 'FAIL'}] HA2 no nested writes: the {len(before_other)} untouched parameters keep their host values"
          + (f"  e.g. {bad2[:3]}" if bad2 else ""))
    xp = xml_params(plugin_xml(plug.raw_state))
    bad3 = []
    for pid, name, kind, rng, val in tg:
        got = xp.get(pid)
        if got is None: bad3.append((pid, "absent")); continue
        if kind == "choice": ok = int(round(got)) == val
        elif kind == "linear": ok = abs(got - val) <= 1e-3 * (rng[1] - rng[0])
        else: ok = abs(got - val) <= 1e-3 * val
        if not ok: bad3.append((pid, got, val))
    print(f"  [{'PASS' if not bad3 else 'FAIL'}] HA3 plugin state XML holds the set values ({len(tg) - len(bad3)}/{len(tg)}, {len(xp)} PARAM)"
          + (f"  e.g. {bad3[:3]}" if bad3 else ""))
    return 0 if not (bad1 or bad2 or bad3) else 1

if __name__ == "__main__":
    sys.exit(main())
