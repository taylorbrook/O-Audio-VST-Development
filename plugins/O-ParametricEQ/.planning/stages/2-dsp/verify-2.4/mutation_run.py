#!/usr/bin/env python3
"""Mutation run for O-ParametricEQ Phase 2.4 (Fade.h, SlotBand.h, the processor's bypass / fade path).
Nothing in the repo or build/ is written: each mutant is a copy of Source/ with one edit, compiled into
this directory against the unmodified harness sources (and the unmodified JUCE module objects in build/),
then run through the designer harness (d) and the render harness (r).

Optional third checker (p): --probe [build.sh path] runs `SRC_DIR=<mutant Source> OUT_DIR=<mutant dir>
bash build.sh`; the probe binary is <mutant dir>/probe, or else the last stdout line of build.sh if it is an
executable path. Its [FAIL] rows are parsed like the harnesses'.

usage: mutation_run.py [-jN] [--probe [path/to/build.sh]] [--no-d] [--no-r] [ID ...]
"""
import os, re, shutil, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

REPO = "/Users/taylorbrook/Dev/VST-development"
P = REPO + "/plugins/O-ParametricEQ"
MUT = os.path.dirname(os.path.abspath(__file__))
DEFAULT_PROBE_SH = os.path.dirname(MUT) + "/probe/build.sh"
OBJ = "plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-render-harness.dir/"
PROC_O = OBJ + "Source/PluginProcessor.cpp.o"
MAIN_O = OBJ + "tests/render-harness/main.cpp.o"
OUT = "plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness"
TIMEOUT = 3600

cc_proc = open(MUT + "/cc_proc.txt").read().strip()
cc_main = open(MUT + "/cc_main.txt").read().strip()
link = open(MUT + "/link.txt").read().strip()

FD, SB, PC = "dsp/Fade.h", "dsp/SlotBand.h", "PluginProcessor.cpp"

MIX_BLOCK = """            eq::BypassFade::End ended = eq::BypassFade::End::none;

            if (bypassFading)
            {
                for (int lane = 0; lane < 2; ++lane)
                    if (io[lane] != nullptr)
                        bypassFade.mix (work[lane] + pos, dry[lane], m, fadeTable);

                ended = bypassFade.advance (m, fadeTable);
            }
"""
BANDS_LOOP = """            for (int a = 0; a < numListed; ++a)
                bands[static_cast<std::size_t> (listed[static_cast<std::size_t> (a)])]
                    .process (work[0] + pos, work[1] + pos, m, stereoLayout, fadeTable, designer);
"""
WAKE = """                const int to = 1 - cur;
                configure (to, zero, next.placement, designer);
                slot[to].setTarget (next.settings);

                ++wakeCount;
                startFade();"""

# id, what, [(file, old, new, nth)]   nth: 0 = all occurrences, k = k-th only (the pattern must exist)
MUTANTS = [
    ("M0", "unmodified control", []),
    # --- Fade.h: the table
    ("F1", "linear weights instead of raised cosine",
        [(FD, "        return 0.5 - halfCos;", "        (void) halfCos; return static_cast<double> (index + 1) / static_cast<double> (length + 1);", 1)]),
    ("F2", "fade length 10 ms", [(FD, "fadeSeconds = 0.020;", "fadeSeconds = 0.010;", 1)]),
    ("F3", "fade length 40 ms", [(FD, "fadeSeconds = 0.020;", "fadeSeconds = 0.040;", 1)]),
    ("F4", "weight index n instead of n + 1 (first weight 0)",
        [(FD, "pi * static_cast<double> (index + 1);", "pi * static_cast<double> (index);", 1)]),
    ("F5", "weight denominator N instead of N + 1 (last weight 1)",
        [(FD, "numerator / static_cast<double> (length + 1);", "numerator / static_cast<double> (length);", 1)]),
    ("F6", "pre-roll 2 ms instead of 0", [(FD, "fadePrerollSeconds = 0.0;", "fadePrerollSeconds = 0.002;", 1)]),
    ("F7", "last stored weight forced to 1.0 (fade N - 1 samples)",
        [(FD, "            weights[static_cast<std::size_t> (i)] = weightOf (i, count);\n",
              "            weights[static_cast<std::size_t> (i)] = weightOf (i, count);\n        weights[static_cast<std::size_t> (count - 1)] = 1.0;\n", 1)]),
    ("F8", "weight past the table end is the last stored weight, not 1",
        [(FD, "        if (index >= fadeLength)\n            return 1.0;", "        if (index >= fadeLength)\n            return weights[static_cast<std::size_t> (fadeLength - 1)];", 1)]),
    ("F9", "sleep after 25 ms", [(FD, "sleepSeconds = 0.050;", "sleepSeconds = 0.025;", 1)]),
    # --- Fade.h: BypassFade
    ("BF1", "bypass fade reverses mid-fade (position kept)",
        [(FD, "        wanted = wantBypass;\n        return startWantedFade();",
              "        wanted = wantBypass;\n"
              "        if (state == State::toBypass && ! wantBypass) { state = State::toProcessed; return false; }\n"
              "        if (state == State::toProcessed && wantBypass) { state = State::toBypass; return false; }\n"
              "        return startWantedFade();", 1)]),
    ("BF2", "latched opposite fade dropped at the fade end (starts at the next block instead)",
        [(FD, "        position = 0;\n        startWantedFade();\n", "        position = 0;\n", 1)]),
    ("BF3", "jump does nothing", [(FD, "        start (wantBypass);\n    }", "        (void) wantBypass;\n    }", 1)]),
    ("BF4", "end of toBypass reaches processed, not bypassed (idle never reached; loops)",
        [(FD, "state = endedToBypass ? State::bypassed : State::processed;", "state = State::processed;", 1)]),
    ("BF5", "bypass mix in complementary form (1 - w) a + w b",
        [(FD, "samples[i] = a + w * (b - a);", "samples[i] = (1.0 - w) * a + w * b;", 0)]),
    # --- SlotBand.h
    ("SB1", "configure does not reset the slot (incoming slot warm start; retire still resets)",
        [(SB, "        slot[index].snapTo (settings, designer);\n        slot[index].reset();", "        slot[index].snapTo (settings, designer);", 1)]),
    ("SB1b", "warm start: neither configure nor retire resets the slot",
        [(SB, "        slot[index].snapTo (settings, designer);\n        slot[index].reset();", "        slot[index].snapTo (settings, designer);", 1),
         (SB, "        slot[index].reset();\n        slot[index].snapToTarget (designer);\n        off[index] = true;", "        slot[index].snapToTarget (designer);\n        off[index] = true;", 1)]),
    ("SB2", "outgoing slot state cleared at a structural fade start",
        [(SB, "        startFade();\n        in = next;\n        markAsleep (nextSleeps);", "        slot[cur].reset();\n        startFade();\n        in = next;\n        markAsleep (nextSleeps);", 1)]),
    ("SB3", "slot mix b + (1 - w)(a - b) (same weights, other rounding)",
        [(SB, "io[l][i] = a + w * (b - a);", "io[l][i] = b + (1.0 - w) * (a - b);", 1)]),
    ("SB3b", "slot mix complementary (1 - w) a + w b", [(SB, "io[l][i] = a + w * (b - a);", "io[l][i] = (1.0 - w) * a + w * b;", 1)]),
    ("SB3c", "slot mix b + w (a - b) (direction reversed)", [(SB, "io[l][i] = a + w * (b - a);", "io[l][i] = b + w * (a - b);", 1)]),
    ("SB4", "latch keeps the first request during a fade, not the latest",
        [(SB, "        desired = input;\n\n        if (fading)",
              "        if (! fading || classifyBandChange (in, desired) == BandChange::identical)\n            desired = input;\n\n        if (fading)", 1)]),
    ("SB5", "latch never fires at the fade end",
        [(SB, "        if (classifyBandChange (in, desired) != BandChange::identical)\n        {\n            ++latchedStarted;",
              "        if (false && classifyBandChange (in, desired) != BandChange::identical)\n        {\n            ++latchedStarted;", 1)]),
    ("SB6", "fade end inside a call deferred to the next call's start",
        [(SB, "            if (pos >= length)\n                endFade (designer);\n        }", "        }", 1)]),
    ("SB6b", "after a fade ends inside a call the rest of the call is left dry",
        [(SB, "            if (pos >= length)\n                endFade (designer);\n        }", "            if (pos >= length)\n            {\n                endFade (designer);\n                return;\n            }\n        }", 1)]),
    ("SB7", "a request identical to the realised input starts a fade",
        [(SB, "        if (change == BandChange::identical)\n            return;\n", "", 1)]),
    ("SB8", "sleep / zero test |gain| < 1e-3 instead of exactly 0",
        [(SB, "sameValue (input.settings.gainDbEff, 0.0);", "std::abs (input.settings.gainDbEff) < 1.0e-3;", 1)]),
    ("SB9", "sleep ignores whether the ramp has arrived", [(SB, "                         && ! slot[cur].isMoving()\n", "", 1)]),
    ("SB10", "sleep never", [(SB, "        if (zeroTicks >= sleepTicks)", "        if (false && zeroTicks >= sleepTicks)", 1)]),
    ("SB11", "wake snaps: current slot configured at the full target, no fade",
        [(SB, WAKE, "                (void) zero;\n                configure (cur, next.settings, next.placement, designer);\n\n                ++wakeCount;", 1)]),
    ("SB11b", "wake fades in the full-gain design (no 0 dB start, no ramp)",
        [(SB, "                configure (to, zero, next.placement, designer);", "                (void) zero;\n                configure (to, next.settings, next.placement, designer);", 1)]),
    ("SB12", "touchesLane: current slot only (misses the incoming slot's placement)",
        [(SB, "        if (slotTouches (0, lane, stereo) || slotTouches (1, lane, stereo))\n            return true;",
              "        if (slotTouches (cur, lane, stereo))\n            return true;", 1)]),
    ("SB12b", "touchesLane ignores a latched request's placement",
        [(SB, "        return hasLatchedRequest()\n            && desired.active", "        return false && hasLatchedRequest()\n            && desired.active", 1)]),
    ("SB13", "goIdle does not retire the current slot",
        [(SB, "        if (! off[cur])\n            retire (cur, designer);\n\n        in.active = false;", "        in.active = false;", 1)]),
    ("SB14", "continuous changes on a fading band are latched (not sent to the incoming slot)",
        [(SB, "            if (classifyBandChange (in, input) == BandChange::continuous\n                && input.active",
              "            if (false && classifyBandChange (in, input) == BandChange::continuous\n                && input.active", 1)]),
    ("SB15", "continuous change during a fade goes to the outgoing slot", [(SB, "                slot[to].setTarget (input.settings);", "                slot[cur].setTarget (input.settings);", 1)]),
    ("SB16", "startSettled does not drop a fade in flight", [(SB, "        stopFade();\n        retire (0, designer);", "        retire (0, designer);", 1)]),
    ("SB17", "only the current slot gets control ticks (incoming slot never ticks)",
        [(SB, "        if (! off[0])\n            slot[0].controlTick (designer);\n\n        if (! off[1])\n            slot[1].controlTick (designer);",
              "        if (! off[cur])\n            slot[cur].controlTick (designer);", 1)]),
    ("SB18", "clearState does not clear the filter state", [(SB, "            slot[i].reset();\n            slot[i].snapToTarget (designer);\n        }\n    }\n\n    //====", "            slot[i].snapToTarget (designer);\n        }\n    }\n\n    //====", 1)]),
    # --- the processor
    ("PX1", "processBlockBypassed takes a hard path (buffer untouched, clock advanced, no fade)",
        [(PC, "    processAudio (buffer, true);\n", "    juce::ignoreUnused (advanceTickClock (buffer.getNumSamples()));\n", 1)]),
    ("PX2", "un-bypass restarts on the pre-bypass request (changes while bypassed dropped)",
        [(PC, "            bands[ub].startSettled (makeInput (now), designer);", "            bands[ub].startSettled (slot.valid ? makeInput (slot.snapshot) : makeInput (now), designer);", 1),
         (PC, "        for (auto& slot : slots)\n            slot.valid = false;\n\n        return;", "        return;", 1)]),
    ("PX2b", "bypassed blocks do not mark the snapshots stale", [(PC, "        for (auto& slot : slots)\n            slot.valid = false;\n\n        return;", "        return;", 1)]),
    ("PX3", "un-bypass does not restart settled (bands re-request instead)",
        [(PC, "        restart = (bypassFade.getState() == eq::BypassFade::State::toProcessed);", "        restart = false;", 1)]),
    ("PX4", "running list not rebuilt after a latched un-bypass mid-block", [(PC, "                    numListed = collectRunningBands (listed);\n", "", 1)]),
    ("PX5", "chunk with the idle point not written back at all",
        [(PC, "            if (io[lane] == nullptr)\n                continue;\n\n            const double* const in = work[lane];\n            float* const out",
              "            if (io[lane] == nullptr || idle)\n                continue;\n\n            const double* const in = work[lane];\n            float* const out", 1)]),
    ("PX5b", "chunk with the idle point written back only up to the idle point",
        [(PC, "        for (int pos = 0; pos < n && ! idle;)", "        int writeEnd = n;\n\n        for (int pos = 0; pos < n && ! idle;)", 1),
         (PC, "                    idle = true;", "                    idle = true;\n                    writeEnd = pos;", 1),
         (PC, "            for (int i = 0; i < n; ++i)\n                out[i] = static_cast<float> (in[i]);\n        }\n    }\n}", "            for (int i = 0; i < writeEnd; ++i)\n                out[i] = static_cast<float> (in[i]);\n        }\n    }\n}", 1)]),
    ("PX6", "bypass mix before the bands (not the last stage)",
        [(PC, MIX_BLOCK, "", 1),
         (PC, BANDS_LOOP, MIX_BLOCK + "\n" + BANDS_LOOP, 1)]),
    ("PX6b", "bypass dry copy taken after the bands run",
        [(PC, "            double dry[2][kDryPieceSize];\n\n            if (bypassFading)\n            {",
              "            double dry[2][kDryPieceSize];\n\n" + BANDS_LOOP + "\n            if (bypassFading)\n            {", 1),
         (PC, BANDS_LOOP + "\n            // The bypass mix", "\n            // The bypass mix", 1)]),
    ("PX7", "prepareToPlay does not reset the bypass / start state",
        [(PC, "    started = false;\n\n    // The engine adds no delay.", "    // The engine adds no delay.", 1)]),
    ("PX8", "fully bypassed blocks do not advance the clock",
        [(PC, "            slot.valid = false;\n\n        return;", "            slot.valid = false;\n\n        tickClock = blockStart;\n        return;", 1)]),
    ("PX8b", "no-running-band blocks do not advance the clock",
        [(PC, "        bypassFade.jump (wantBypass);\n        return;", "        bypassFade.jump (wantBypass);\n        tickClock = blockStart;\n        return;", 1)]),
    ("PX9", "segment not cut at the bypass fade's end",
        [(PC, "juce::jmin (m, bypassFade.samplesLeft (fadeTable), kDryPieceSize)", "juce::jmin (m, kDryPieceSize)", 1)]),
    ("PX10", "processBlockBypassed does not want bypass", [(PC, "const bool wantBypass = viaBypassed || ", "const bool wantBypass = ", 1)]),
    ("PX11", "no-running-band blocks keep a bypass fade in flight (no jump)",
        [(PC, "        bypassFade.jump (wantBypass);\n        return;", "        return;", 1)]),
    ("PX12", "end of toBypass does not send the bands idle", [(PC, "                for (auto& band : bands)\n                    band.goIdle (designer);\n", "", 1)]),
    ("PX13", "non-finite chunk does not clear the band state", [(PC, "        if (! finite)\n            for (auto& band : bands)\n                band.clearState (designer);", "        juce::ignoreUnused (finite);", 1)]),
    ("PX14", "latched un-bypass mid-block restarts bands on their realised input, not their latest request",
        [(PC, "const eq::BandInput latest = band.getDesired();", "const eq::BandInput latest = band.getRealised();", 1)]),
]


def replace_nth(text, old, new, nth):
    n = text.count(old)
    if n == 0:
        raise RuntimeError("pattern not found: " + old[:70].replace("\n", "\\n"))
    if nth == 0:
        return text.replace(old, new)
    if n != 1 and nth == 1:
        raise RuntimeError(f"pattern occurs {n} times (expected once): " + old[:70].replace("\n", "\\n"))
    if nth > n:
        raise RuntimeError("nth > count")
    idx = -1
    for _ in range(nth):
        idx = text.index(old, idx + 1)
    return text[:idx] + new + text[idx + len(old):]


def sh(cmd, cwd, log, timeout=TIMEOUT, env=None):
    with open(log, "w") as f:
        try:
            return subprocess.run(["/bin/bash", "-c", cmd], cwd=cwd, stdout=f, stderr=subprocess.STDOUT, timeout=timeout,
                                  env=env).returncode
        except subprocess.TimeoutExpired:
            f.write("\nTIMEOUT\n")
            return -99


def fails(log):
    out = []
    for line in open(log, errors="replace"):
        if "[FAIL]" in line:
            m = re.search(r"\[FAIL\]\s+(\S+(?: \([iv]+\))?)", line)
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
    M = f"{MUT}/{mid}"
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
    res = {}
    inc_old = f"-I{P}/Source "
    inc_new = f"-I{M}/Source "
    if opts["d"]:
        cmd = (f'/usr/bin/c++ -DGOLDEN_DIR=\\"{P}/tests/golden\\" {inc_new}-O3 -DNDEBUG -std=c++17 -arch arm64 '
               f'-mmacosx-version-min=10.13 -O3 -w {P}/tests/designer-harness/main.cpp -o {M}/designer')
        rc = sh(cmd, M, M + "/d-build.log")
        if rc != 0:
            res["d"] = f"BUILD rc {rc}"
        else:
            rc = sh(f"{M}/designer", M, M + "/d-run.log")
            res["d"] = result(M + "/d-run.log", rc)
    if opts["r"]:
        ok = True
        c = cc_proc.replace(inc_old, inc_new).replace(PROC_O, M + "/proc.o") \
                   .replace(f"-c {P}/Source/PluginProcessor.cpp", f"-c {M}/Source/PluginProcessor.cpp") + " -w"
        assert inc_new in c and M + "/Source/PluginProcessor.cpp" in c and PROC_O not in c
        if sh(c, REPO + "/build", M + "/r-build-proc.log") != 0:
            ok = False
        main_o = MAIN_O
        # main.cpp includes PluginProcessor.h and the dsp headers: recompile it whenever a header changed
        if ok and (files - {PC}):
            c = cc_main.replace(inc_old, inc_new).replace(MAIN_O, M + "/main.o") + " -w"
            assert MAIN_O not in c
            if sh(c, REPO + "/build", M + "/r-build-main.log") != 0:
                ok = False
            main_o = M + "/main.o"
        if ok:
            c = link.replace(PROC_O, M + "/proc.o").replace(MAIN_O, main_o).replace("-o " + OUT, "-o " + M + "/render")
            assert "-o " + M + "/render" in c
            if sh(c, REPO + "/build", M + "/r-link.log") != 0:
                ok = False
        if not ok:
            res["r"] = "BUILD failed"
        else:
            rc = sh(f"{M}/render", M, M + "/r-run.log")
            res["r"] = result(M + "/r-run.log", rc)
    if opts["p"]:
        env = dict(os.environ, SRC_DIR=M + "/Source", OUT_DIR=M)
        rc = sh(f"bash {opts['probe_sh']}", os.path.dirname(opts["probe_sh"]), M + "/p-build.log", env=env)
        binary = M + "/probe"
        if not os.access(binary, os.X_OK):
            lines = [l.strip() for l in open(M + "/p-build.log", errors="replace") if l.strip()]
            if lines and os.access(lines[-1], os.X_OK):
                binary = lines[-1]
        if rc != 0 or not os.access(binary, os.X_OK):
            res["p"] = f"BUILD rc {rc}"
        else:
            rc = sh(binary, M, M + "/p-run.log")
            res["p"] = result(M + "/p-run.log", rc)
    line = f"{mid}  {what}\n   designer: {res.get('d', '-')}\n   render:   {res.get('r', '-')}"
    if opts["p"]:
        line += f"\n   probe:    {res.get('p', '-')}"
    print(line, flush=True)
    return line


if __name__ == "__main__":
    args = sys.argv[1:]
    opts = {"d": "--no-d" not in args, "r": "--no-r" not in args, "p": False, "probe_sh": DEFAULT_PROBE_SH}
    sel, jobs, i = [], 7, 0
    while i < len(args):
        a = args[i]
        if a.startswith("-j"):
            jobs = int(a[2:])
        elif a == "--probe":
            opts["p"] = True
            if i + 1 < len(args) and args[i + 1].endswith(".sh"):
                opts["probe_sh"] = os.path.abspath(args[i + 1]); i += 1
        elif a in ("--no-d", "--no-r"):
            pass
        elif a == "--check":
            # only apply the edits, report any that do not apply exactly
            for mid, what, edits in MUTANTS:
                for f, old, new, nth in edits:
                    try:
                        replace_nth(open(f"{P}/Source/{f}").read(), old, new, nth)
                    except RuntimeError as e:
                        print(mid, "EDIT FAILED:", e)
            print(len(MUTANTS), "mutants checked"); sys.exit(0)
        else:
            sel.append(a)
        i += 1
    todo = [m for m in MUTANTS if not sel or m[0] in sel]
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        out = list(ex.map(lambda m: run(m, opts), todo))
    name = "all" if not sel else "_".join(sel)
    open(MUT + "/results-" + name + ".txt", "w").write("\n".join(out) + "\n")
