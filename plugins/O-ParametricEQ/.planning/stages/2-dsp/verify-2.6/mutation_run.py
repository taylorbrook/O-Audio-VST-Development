#!/usr/bin/env python3
"""Mutation run for O-ParametricEQ Phase 2.6 verify: Listen.h, SpectrumTap.h, the processor's listen / tap
glue and pathIdleForBlock, plus a few message-thread mutants (DisplayModel, SpectrumAnalyzer).

Nothing in the repo or build/ is written. Each mutant is a copy of Source/ with one edit, compiled into
$WORK/<ID>/ with the compile and link commands ninja prints for the harness targets (read only), against
the unmodified harness sources and JUCE objects. Checkers:
  d  designer harness (only for edits to a header the designer includes: dsp/*.h)
  r  render harness (main + PluginProcessor + analysis TUs recompiled; --lid-dir $LID)
  p  verify-2.6 probe (build.sh with SRC_DIR / OUT_DIR)

usage: WORK=<dir> LID=<0.5.0 dump dir> mutation_run.py [-jN] [--no-d] [--no-r] [--no-p] [--check] [ID ...]
"""
import os, re, shutil, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

REPO = "/Users/taylorbrook/Dev/VST-development"
BUILD = REPO + "/build"
P = REPO + "/plugins/O-ParametricEQ"
HERE = os.path.dirname(os.path.abspath(__file__))
WORK = os.environ["WORK"]
LID = os.environ["LID"]
TIMEOUT = 3600

LS, ST, PC = "dsp/Listen.h", "dsp/SpectrumTap.h", "PluginProcessor.cpp"
DM, AN = "analysis/DisplayModel.cpp", "analysis/SpectrumAnalyzer.cpp"

MUTANTS = [
    ("M0", "unmodified control", []),
    # ---------------------------------------------------------------- Listen.h
    ("LS1", "input ramp 30 -> 20 ms", [(LS, "listenRampSeconds = 0.030;", "listenRampSeconds = 0.020;", 1)]),
    ("LS2", "engage: weight starts at the move start (inside the ramp)", [(LS, "                            if (p < rampLength)\n                                continue;\n\n                            const double w = table.weight (p - rampLength);", "                            const double w = table.weight (p < table.getLength() ? p : table.getLength() - 1);", 1)]),
    ("LS3", "engage: weight one sample late", [(LS, "                            const double w = table.weight (p - rampLength);\n                            const double x = io[l][i];", "                            const double w = (p == rampLength) ? 0.0 : table.weight (p - rampLength - 1);\n                            const double x = io[l][i];", 1)]),
    ("LS4", "change: incoming voice heard during its ramp", [(LS, "                                io[l][i] = x;\n                                continue;", "                                io[l][i] = b[l][i];\n                                continue;", 1)]),
    ("LS5", "release: weights reversed", [(LS, "                            io[l][i] = x + w * (io[l][i] - x);", "                            io[l][i] = io[l][i] + w * (x - io[l][i]);", 1)]),
    ("LS6", "input ramp not applied (filter from rest at full input)", [(LS, "            g = gain;\n", "            g = nullptr;\n", 1)]),
    ("LS7", "ramp table built for 48 kHz at every rate", [(LS, "buildRamp (listenRampLengthSamples (sampleRateHz));", "buildRamp (listenRampLengthSamples (48000.0));", 1)]),
    ("LS8", "latch: a request made during a move is never taken", [(LS, "        if (requestDiffers())\n", "        if (false)\n", 1)]),
    ("LS9", "no latch: a request during a move restarts at once", [(LS, "        if (moveKind != ListenMove::none)\n        {\n            if ((moveKind", "        if (false)\n        {\n            if ((moveKind", 1)]),
    ("LS10", "f / Q edit during a move not passed to the incoming voice", [(LS, "                if (classifyBandChange (in.input, desiredInput) == BandChange::continuous)", "                if (false)", 1)]),
    ("LS11", "settled f / Q edit runs a change move instead of a ramp", [(LS, "            case BandChange::continuous:\n                cur.band.setTarget (desiredInput.settings);\n                cur.input = desiredInput;\n                return;", "            case BandChange::continuous:\n                configure (1 - active, desiredInput, designer);\n                startMove (ListenMove::change);\n                return;", 1)]),
    ("LS12", "structural edit of the listened band ignored", [(LS, "            case BandChange::structural:\n                configure (1 - active, desiredInput, designer);\n                startMove (ListenMove::change);\n                return;", "            case BandChange::structural:\n                return;", 1)]),
    ("LS13", "goIdle drops the target", [(LS, "    void goIdle() noexcept\n    {\n        moveKind = ListenMove::none;", "    void goIdle() noexcept\n    {\n        target = -1;\n        moveKind = ListenMove::none;", 1)]),
    ("LS14", "startSettled leaves the input ramp to run", [(LS, "            voice[active].rampPos = rampLength;\n            target = desiredIndex;", "            voice[active].rampPos = 0;\n            target = desiredIndex;", 1)]),
    ("LS15", "clearState does nothing", [(LS, "            voice[v].band.reset();\n            voice[v].band.snapToTarget (designer);", "            (void) designer;", 1)]),
    ("LS16", "isIdle ignores a pending wish", [(LS, "return ! isEngaged() && ! hasPending(); }", "return ! isEngaged(); }", 1)]),
    ("LS17", "hasPending ignores an input change of the same band", [(LS, "        return pendingIndex >= 0 && ! sameBandInput (pendingInput, desiredInput);", "        return false;", 1)]),
    ("LS18", "isEngaged ignores a move in flight (release)", [(LS, "return target >= 0 || moveKind != ListenMove::none; }", "return target >= 0; }", 1)]),
    ("LS19", "map: shelves / cuts listen at Q 0.7", [(LS, "            out.settings.type = Type::highCut;\n            out.settings.q = 1.0;", "            out.settings.type = Type::highCut;\n            out.settings.q = 0.7;", 1)]),
    ("LS20", "map: tilt band pass at Q 1", [(LS, "            out.settings.q = 0.7;", "            out.settings.q = 1.0;", 1)]),
    ("LS21", "map: bell family at a fixed Q 1", [(LS, "    out.settings.q = band.settings.q;", "    out.settings.q = 1.0;", 1)]),
    ("LS22", "map: low shelf / cut -> low cut (wrong side)", [(LS, "            out.settings.type = Type::highCut;", "            out.settings.type = Type::lowCut;", 1)]),
    ("LS23", "map: shelf listen slope 12 -> 24 dB/oct", [(LS, "            out.settings.slopeIndex = 1;", "            out.settings.slopeIndex = 3;", 1)]),
    ("LS24", "router: Side R = +F(s)", [(LS, "                    outR[i] = -outL[i];", "                    outR[i] = outL[i];", 1)]),
    ("LS25", "router: Mid from L only", [(LS, "gained (0.5 * (dryL[i] + dryR[i]), inputGain, i);", "gained (dryL[i], inputGain, i);", 1)]),
    ("LS26", "router: Left listen passes R dry", [(LS, "                    outL[i] = gained (dryL[i], inputGain, i);\n                    outR[i] = 0.0;", "                    outL[i] = gained (dryL[i], inputGain, i);\n                    outR[i] = dryR[i];", 1)]),
    ("LS27", "router: Side without the 0.5", [(LS, "gained (0.5 * (dryL[i] - dryR[i]), inputGain, i);", "gained ((dryL[i] - dryR[i]), inputGain, i);", 1)]),
    ("LS28", "engage / change last N only (no ramp time)", [(LS, "            case ListenMove::change:    return rampLength + fadeTotal;", "            case ListenMove::change:    return fadeTotal;", 1)]),
    ("LS29", "release R + N long", [(LS, "            case ListenMove::release:   return fadeTotal;", "            case ListenMove::release:   return rampLength + fadeTotal;", 1)]),
    ("LS31", "voice ramp position never advances", [(LS, "        vc.rampPos = (next < rampLength) ? next : rampLength;", "        (void) next;", 1)]),
    ("LS32", "configure does not reset the voice", [(LS, "        voice[v].band.snapTo (input.settings, designer);\n        voice[v].band.reset();", "        voice[v].band.snapTo (input.settings, designer);", 1)]),
    ("LS33", "ramp linear instead of raised cosine", [(LS, "            ramp[i] = FadeTable::weightOf (i, count);", "            ramp[i] = (i + 1.0) / (count + 1.0);", 1)]),
    ("LS34", "applyPending: a settled listen ignores the request", [(LS, "            return;\n        }\n\n        settle (designer);\n    }", "            return;\n        }\n\n        if (target < 0) settle (designer);\n    }", 1)]),
    # ----------------------------------------------------------- SpectrumTap.h
    ("ST1", "overflow keeps the oldest of a push", [(ST, "            src += lost;\n", "", 1)]),
    ("ST2", "drops not counted", [(ST, "            drops.fetch_add (lost, std::memory_order_relaxed);", "            (void) lost;", 1)]),
    ("ST3", "free space ignores the reader (overwrites unread)", [(ST, "        const std::uint32_t space = (used < ucapacity) ? ucapacity - used : 0u;", "        const std::uint32_t space = ucapacity; (void) used;", 1)]),
    ("ST4", "copyIn wrap writes past the start", [(ST, "            ring[i - first] = src[i];", "            ring[(start + i) & mask] = src[i - first];", 1)]),
    ("ST5", "drain does not advance the read index", [(ST, "        const std::uint64_t next = packState (gen, r + count);", "        const std::uint64_t next = packState (gen, r);", 1)]),
    ("ST6", "reset does not bump the generation", [(ST, "readState.load (std::memory_order_acquire)) + 1u;", "readState.load (std::memory_order_acquire)) + 0u;", 1)]),
    ("ST7", "reset does not zero writeIndex", [(ST, "        writeIndex.store (0u, std::memory_order_release);\n        readState.store", "        readState.store", 1)]),
    # ---------------------------------------------------------- PluginProcessor
    ("PX21", "pathIdleForBlock ignores listen", [(PC, "    return stageIdleForBlock() && listen.isIdle();", "    return stageIdleForBlock();", 1)]),
    ("PX22", "listen input without the effective placement", [(PC, "            listenInput = eq::listenInputFor (makeInput (now));", "            listenInput = eq::listenInputFor (eq::makeBandInput (now.raw, now.gainScale, 0.0));", 1)]),
    ("PX23", "first block: listen not seeded", [(PC, "        seedOutputStage();\n        seedListen();\n    }", "        seedOutputStage();\n    }", 1)]),
    ("PX24", "latched un-bypass: listen not seeded", [(PC, "                    seedOutputStage();\n                    seedListen();", "                    seedOutputStage();", 1)]),
    ("PX25", "fade-to-bypass end: listen not idled", [(PC, "                listen.goIdle();\n", "", 1)]),
    ("PX26", "non-finite: listen state not cleared", [(PC, "            listen.clearState (designer);\n", "", 1)]),
    ("PX27", "listen voices get no control tick", [(PC, "                listen.controlTick (designer);\n", "", 1)]),
    ("PX28", "segment not ended at a listen move end", [(PC, "                if (moveLeft > 0)\n                    m = juce::jmin (m, moveLeft);", "                juce::ignoreUnused (moveLeft);", 1)]),
    ("PX29", "dry copy only while a bypass fade runs", [(PC, "            if (bypassFading || listenActive)\n", "            if (bypassFading)\n", 1)]),
    ("PX30", "gain form chosen by isIdle, not isEngaged", [(PC, "            const bool listenEngaged = listen.isEngaged();", "            const bool listenEngaged = ! listen.isIdle();", 1)]),
    ("PX31", "listen before auto gain (C3 broken)", [(PC, "                        x[i] *= ag[i];", "                        x[i] *= 1.0;", 1), (PC, "                        x[i] *= og[i];", "                        x[i] *= og[i] * ag[i];", 1)]),
    ("PX32", "taps ignore the mode bits", [(PC, "    return analyzerMode.load (std::memory_order_relaxed) & 3;", "    return 3;", 1)]),
    ("PX33", "taps ignore the consumer flag", [(PC, "    if (! analyzerConsumer.load (std::memory_order_acquire))\n        return 0;", "", 1)]),
    ("PX34", "tap offers the whole block, not its tail", [(PC, "    const int first = juce::jmax (0, numSamples - eq::SpectrumTap::capacity);", "    const int first = 0;", 1)]),
    ("PX35", "mono sum without 0.5", [(PC, "            piece[i] = 0.5f * (left[start + i] + right[start + i]);", "            piece[i] = (left[start + i] + right[start + i]);", 1)]),
    ("PX36", "post tap before processAudio (processBlock)", [(PC, "    processAudio (buffer, false);\n\n    if ((taps & 2) != 0)\n        pushTap (postTap, buffer);", "    if ((taps & 2) != 0)\n        pushTap (postTap, buffer);\n\n    processAudio (buffer, false);", 1)]),
    ("PX37", "prepare: pre tap not reset", [(PC, "    preTap.reset();\n", "", 1)]),
    ("PX38", "listenBand range off by one (24 accepted)", [(PC, "    if (listenWanted < 0 || listenWanted >= kNumBands)", "    if (listenWanted < 0 || listenWanted > kNumBands)", 1)]),
    ("PX39", "latched un-bypass jumps although listen is engaged", [(PC, "    if (numListed == 0 && outputStage.isUnity() && listen.isIdle())", "    if (numListed == 0 && outputStage.isUnity())", 1)]),
    ("PX40", "listen off never handed to listen (release never starts)", [(PC, "    listen.setPending (listenWanted, listenInput);", "    if (listenWanted >= 0) listen.setPending (listenWanted, listenInput);", 1)]),
    ("PX41", "segment rule uses isEngaged instead of ! isIdle", [(PC, "            const bool listenActive = ! listen.isIdle();", "            const bool listenActive = listen.isEngaged();", 1)]),
    ("PX42", "rate published before the layout", [(PC, "    preparedStereo.store (stereoLayout, std::memory_order_release);\n    preparedRate.store (sampleRate, std::memory_order_release);", "    preparedRate.store (sampleRate, std::memory_order_release);\n    preparedStereo.store (stereoLayout, std::memory_order_release);", 1)]),
    # ------------------------------------------------- message thread (render only)
    ("DM1", "display: a sleeping band sent active", [(DM, "        e.active = e.input.active && ! sleeps (e.input);", "        e.active = e.input.active;", 1)]),
    ("DM2", "display: layout change does not re-design", [(DM, " || st != stereo", "", 1)]),
    ("DM3", "display: absent one-pole half sent as zeros", [(DM, "            onePole = { 1.0, 1.0, 1.0 };", "            onePole = { 0.0, 0.0, 0.0 };", 1)]),
    ("AN1", "analyzer: drop counter ignored", [(AN, "    if (drops != t.drops)", "    if (false)", 1)]),
    ("AN2", "analyzer: the drain after a drop not skipped", [(AN, "    if (t.skipNext)\n", "    if (false)\n", 1)]),
    ("AN3", "analyzer: scale 1 / sum w (6 dB low)", [(AN, "    const double scale = 2.0 / windowSum;", "    const double scale = 1.0 / windowSum;", 1)]),
    ("AN4", "analyzer: no release with no new frame", [(AN, "                held[k] = static_cast<float> (std::max (static_cast<double> (held[k]) - fall, static_cast<double> (floorDb)));", "                (void) fall;", 1)]),
]

DESIGNER_HEADERS = {LS, ST}


def replace_nth(text, old, new, nth):
    idx, start = -1, 0
    for _ in range(nth):
        idx = text.find(old, start)
        if idx < 0:
            raise RuntimeError(f"pattern not found (occurrence {nth}): {old[:60]!r}")
        start = idx + 1
    return text[:idx] + new + text[idx + len(old):]


def sh(cmd, cwd, log, env=None):
    with open(log, "w") as f:
        try:
            return subprocess.run(cmd, shell=True, cwd=cwd, stdout=f, stderr=subprocess.STDOUT, timeout=TIMEOUT, env=env).returncode
        except subprocess.TimeoutExpired:
            f.write("\nTIMEOUT\n")
            return -99


def commands(target):
    return subprocess.run(f"ninja -t commands {target}", shell=True, cwd=BUILD, capture_output=True, text=True).stdout.splitlines()


RCMDS = commands("O-ParametricEQ-render-harness")
DCMDS = commands("O-ParametricEQ-designer-harness")
RD = "plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-render-harness.dir/"
DD = "plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-designer-harness.dir/"
RTU = {"main": RD + "tests/render-harness/main.cpp.o", "PluginProcessor": RD + "Source/PluginProcessor.cpp.o",
       "SpectrumAnalyzer": RD + "Source/analysis/SpectrumAnalyzer.cpp.o", "DisplayModel": RD + "Source/analysis/DisplayModel.cpp.o"}
RSRC = {"main": P + "/tests/render-harness/main.cpp", "PluginProcessor": P + "/Source/PluginProcessor.cpp",
        "SpectrumAnalyzer": P + "/Source/analysis/SpectrumAnalyzer.cpp", "DisplayModel": P + "/Source/analysis/DisplayModel.cpp"}
ROUT = "plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness"
DOUT = "plugins/O-ParametricEQ/O-ParametricEQ-designer-harness"


def cc_for(cmds, obj):
    c = [l for l in cmds if f"-o {obj} -c " in l]
    assert len(c) == 1, obj
    return re.sub(r" -MD -MT \S+ -MF \S+", "", c[0])


def link_for(cmds, out):
    c = [l for l in cmds if f"-o {out} " in l and l.startswith(": && ")]
    assert len(c) == 1, out
    return c[0][len(": && "):].replace(" && :", "")


def fails(log):
    out = []
    for line in open(log, errors="replace"):
        if "[FAIL]" in line:
            m = re.search(r"\[FAIL\]\s+(\S+)", line)
            out.append(m.group(1) if m else line.strip()[:40])
    return out


def tally(log):
    last = ""
    for line in open(log, errors="replace"):
        if re.search(r"\d+ PASS, \d+ FAIL", line):
            last = line.strip()
    return last or "no summary line"


def result(log, rc):
    f = fails(log)
    t = "TIMEOUT" if rc == -99 else f"rc {rc}"
    if rc < 0 and rc != -99:
        t += f" (signal {-rc})"
    return f"{t} | {tally(log)} | FAIL: {', '.join(f) or 'none'}"


def run(mut, opts):
    mid, what, edits = mut
    M = f"{WORK}/{mid}"
    shutil.rmtree(M, ignore_errors=True)
    os.makedirs(M)
    shutil.copytree(P + "/Source", M + "/Source")
    files = set()
    try:
        for f, old, new, nth in edits:
            path = f"{M}/Source/{f}"
            t = open(path).read()
            open(path, "w").write(replace_nth(t, old, new, nth))
            files.add(f)
    except RuntimeError as e:
        line = f"{mid}  {what}\n   EDIT FAILED: {e}"
        print(line, flush=True)
        return line
    inc_old, inc_new = f"-I{P}/Source ", f"-I{M}/Source "
    res = {}
    if opts["d"] and (not edits or (files & DESIGNER_HEADERS)):
        c = cc_for(DCMDS, DD + "tests/designer-harness/main.cpp.o").replace(inc_old, inc_new) \
            .replace(DD + "tests/designer-harness/main.cpp.o", M + "/d-main.o") + " -w"
        l = link_for(DCMDS, DOUT).replace(DD + "tests/designer-harness/main.cpp.o", M + "/d-main.o").replace("-o " + DOUT, "-o " + M + "/designer")
        if sh(c, BUILD, M + "/d-build.log") != 0 or sh(l, BUILD, M + "/d-link.log") != 0:
            res["d"] = "BUILD failed"
        else:
            rc = sh(f"{M}/designer", M, M + "/d-run.log")
            res["d"] = result(M + "/d-run.log", rc)
    if opts["r"]:
        ok, objs = True, dict(RTU)
        header_edit = bool(files - {PC, DM, AN})
        for tu, obj in RTU.items():
            need = (not edits) or header_edit or (tu == "PluginProcessor" and PC in files) \
                or (tu == "DisplayModel" and DM in files) or (tu == "SpectrumAnalyzer" and AN in files)
            if not need:
                continue
            src_new = RSRC[tu] if tu == "main" else RSRC[tu].replace(P + "/Source", M + "/Source")
            c = cc_for(RCMDS, obj).replace(inc_old, inc_new).replace(f"-c {RSRC[tu]}", f"-c {src_new}") \
                .replace(f"-o {obj} ", f"-o {M}/{tu}.o ") + " -w"
            assert f"-o {M}/{tu}.o " in c and f"-c {src_new}" in c
            if sh(c, BUILD, f"{M}/r-build-{tu}.log") != 0:
                ok = False
                break
            objs[tu] = f"{M}/{tu}.o"
        if ok:
            l = link_for(RCMDS, ROUT)
            for tu, obj in RTU.items():
                l = l.replace(f" {obj} ", f" {objs[tu]} ")
            l = l.replace("-o " + ROUT, "-o " + M + "/render")
            ok = sh(l, BUILD, M + "/r-link.log") == 0
        if not ok:
            res["r"] = "BUILD failed"
        else:
            rc = sh(f"{M}/render --lid-dir {LID}", M, M + "/r-run.log")
            res["r"] = result(M + "/r-run.log", rc)
    if opts["p"]:
        env = dict(os.environ, SRC_DIR=M + "/Source", OUT_DIR=M + "/probe-out")
        rc = sh(f"bash {HERE}/build.sh", HERE, M + "/p-build.log", env=env)
        binary = M + "/probe-out/probe"
        if rc != 0 or not os.access(binary, os.X_OK):
            res["p"] = f"BUILD rc {rc}"
        else:
            rc = sh(binary, M, M + "/p-run.log")
            res["p"] = result(M + "/p-run.log", rc)
    line = f"{mid}  {what}\n   designer: {res.get('d', '-')}\n   render:   {res.get('r', '-')}\n   probe:    {res.get('p', '-')}"
    print(line, flush=True)
    # keep the logs, drop the objects and binaries
    for root, _, fs in os.walk(M):
        for f in fs:
            if f.endswith(".o") or f in ("designer", "render", "probe"):
                os.remove(os.path.join(root, f))
    shutil.rmtree(M + "/Source", ignore_errors=True)
    return line


if __name__ == "__main__":
    args = sys.argv[1:]
    opts = {"d": "--no-d" not in args, "r": "--no-r" not in args, "p": "--no-p" not in args}
    sel, jobs = [], 3
    for a in args:
        if a.startswith("-j"):
            jobs = int(a[2:])
        elif a == "--check":
            bad = 0
            for mid, what, edits in MUTANTS:
                for f, old, new, nth in edits:
                    try:
                        replace_nth(open(f"{P}/Source/{f}").read(), old, new, nth)
                    except RuntimeError as e:
                        bad += 1
                        print(mid, "EDIT FAILED:", e)
            print(len(MUTANTS), "mutants checked,", bad, "failed")
            sys.exit(1 if bad else 0)
        elif not a.startswith("--"):
            sel.append(a)
    if sel == ["ALL"]:          # every mutant but the control
        todo, sel = [m for m in MUTANTS if m[0] != "M0"], []
    else:
        todo = [m for m in MUTANTS if not sel or m[0] in sel]
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        out = list(ex.map(lambda m: run(m, opts), todo))
    name = "all" if not sel else "_".join(sel)
    open(WORK + "/results-" + name + ".txt", "w").write("\n".join(out) + "\n")
