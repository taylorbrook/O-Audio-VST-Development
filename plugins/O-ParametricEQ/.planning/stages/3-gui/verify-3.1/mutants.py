"""Verify 3.1: editor / page / processor mutants run through the editor bench (scratch copy).
Each mutant: one exact-string replacement in the scratch Source copy; rebuild; run; verdict; restore."""
import subprocess, sys, os, shutil, json

S = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(S, "msrc")
APP = os.path.join(S, "bench/build/EditorBench_artefacts/Release/EditorBench.app/Contents/MacOS/EditorBench")
ED, PG, PP, EH = "PluginEditor.cpp", "ui/public/index.html", "PluginProcessor.cpp", "PluginEditor.h"

M = [
    ("E1 drop q relay registration", EH, ".withOptionsFrom (qRelay)\n", "\n"),
    ("E2 gain attachment on q relay", EH, "relays.gainRelay,      nullptr)", "relays.qRelay,      nullptr)"),
    ("E3 slope/placement combo relays swapped", EH, "relays.slopeRelay,     nullptr)", "relays.placementRelay,     nullptr)"),
    ("E4 output_gain attachment on gain_scale relay", ED, "*apvts.getParameter (\"output_gain\"), *outputGainRelay", "*apvts.getParameter (\"output_gain\"), *gainScaleRelay"),
    ("E5 no listen reset in dtor", ED, "audioProcessor.listenBand.store (-1, std::memory_order_release);\n\n    // Members", "\n    // Members"),
    ("E6 setListenBand off by one", ED, "? band - 1 : -1", "? band : -1"),
    ("E7 setListenBand upper bound 23", ED, "band >= 1 && band <= opeq::kNumBands ?", "band >= 1 && band < opeq::kNumBands ?"),
    ("E8 analyzerMode accepts 4", ED, "value >= 0 && value <= 3)", "value >= 0 && value <= 4)"),
    ("E9 selectedBand accepts -1", ED, "key == \"selectedBand\" && value >= 0", "key == \"selectedBand\" && value >= -1"),
    ("E10 resizeEditor unclamped", ED, "setSize (juce::jlimit (kMinWidth,  kMaxWidth,  (int) args[0]),\n                         juce::jlimit (kMinHeight, kMaxHeight, (int) args[1]));", "setSize ((int) args[0], (int) args[1]);"),
    ("E11 resized() stops storing size", ED, "audioProcessor.uiEditorWidth.store  (getWidth(),  std::memory_order_release);", ""),
    ("E12 woff2 wrong MIME", ED, "UIBinaryData::EBGaramondRegular_woff2Size, \"font/woff2\")", "UIBinaryData::EBGaramondRegular_woff2Size, \"application/octet-stream\")"),
    ("E13 css not served", ED, "if (url == \"/css/eb-garamond.css\")", "if (url == \"/css/eb-garamond.cssX\")"),
    ("E14 pageReady not registered", ED, "withNativeFunction (\"pageReady\"", "withNativeFunction (\"pageReadyX\""),
    ("E15 getParameterDefaults normalised", ED, "param->convertFrom0to1 (param->getDefaultValue())", "param->getDefaultValue()"),
    ("E16 savePreset guard skipped", ED, "if (opeq::presetSaveRefusal (target, pm).isNotEmpty())", "if (false && opeq::presetSaveRefusal (target, pm).isNotEmpty())"),
    ("E17 timer started in ctor (no pageReady gate)", ED, "    // The 30 Hz timer starts in the pageReady native function, not here.", "    startTimerHz (30);"),
    ("P1 notch keeps gain", PG, "const TYPE_HAS_GAIN  = [1, 1, 1, 0, 0, 0, 0, 1];", "const TYPE_HAS_GAIN  = [1, 1, 1, 0, 0, 1, 0, 1];"),
    ("P2 tilt has no slope", PG, "const TYPE_HAS_SLOPE = [0, 1, 1, 1, 1, 0, 0, 1];", "const TYPE_HAS_SLOPE = [0, 1, 1, 1, 1, 0, 0, 0];"),
    ("P3 Q inert at 12 dB/oct", PG, "const hasQ = !(hasSlope && slope === 0);", "const hasQ = !(hasSlope && slope === 1);"),
    ("P4 pageReady never called", PG, "Juce.getNativeFunction('pageReady')(performance.now())", "Promise.resolve(performance.now())"),
    ("R1 preset exclusion removed", PP, "presetManager.setExcludedParameters ({ \"output_gain\", \"bypass\" });", ""),
    ("R2 validated apply off", PP, "presetManager.setValidatedApply (true);", ""),
]

def run(name, f, old, new):
    path = os.path.join(SRC, f)
    orig = open(path).read()
    n = orig.count(old)
    if n != 1:
        return f"BADPATCH ({n} matches)"
    open(path, "w").write(orig.replace(old, new))
    try:
        b = subprocess.run(["ninja", "-C", os.path.join(S, "bench/build")], capture_output=True, text=True)
        if b.returncode:
            return "BUILD-FAIL " + b.stdout[-300:]
        out = os.path.join(S, "mut", name.split()[0] + ".json")
        if os.path.exists(out): os.remove(out)
        try:
            p = subprocess.run([APP, "--cycles", "2", "--reps", "2", "--idle", "3", "--out", out, "-NSAppSleepDisabled", "YES"],
                               capture_output=True, text=True, timeout=240)
            rc = p.returncode
            log = p.stdout + p.stderr
        except subprocess.TimeoutExpired:
            return "CAUGHT timeout"
        asserts = log.count("JUCE Assertion")
        if not os.path.exists(out):
            return f"CAUGHT no-json rc={rc}"
        v = subprocess.run([sys.executable, os.path.join(S, "verdict.py"), out], capture_output=True, text=True).stdout.strip()
        return ("CAUGHT " if v != "ALL-PASS" else "SURVIVED ") + v + f" rc={rc} asserts={asserts}"
    finally:
        open(path, "w").write(orig)

only = sys.argv[1:]
for name, f, old, new in M:
    if only and name.split()[0] not in only:
        continue
    print(f"{name:48s} {run(name, f, old, new)}", flush=True)
subprocess.run(["ninja", "-C", os.path.join(S, "bench/build")], capture_output=True)
