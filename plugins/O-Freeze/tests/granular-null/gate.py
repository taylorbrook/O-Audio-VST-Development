# O-Freeze v2.9.0 Granular null gate (regression wall for the Spectral engine).
#
# Renders a v2.8.0 build (out of tree from `git archive 26bbaeb6 -- CMakeLists.txt
# modules plugins/O-Freeze`; there is no v2.8.0 tag) and the current build from the
# SAME template states. The templates are made on the v2.8.0 plugin, so they are
# v2.8.0 states: loading one into the new build also checks old-session migration
# (ENGINE = Granular, DIFFUSION 0 %, AVG_FRAMES 8). Each template carries its own
# random rngSeed, so both builds render the same grains. Every blocksize-null config,
# every block size: the two renders must be bit-identical (exact equality).
#
# Controls:
#   - non-vacuous: the frozen section must carry signal (> -40 dBFS RMS)
#   - negative control (--expect-fail): run against a deliberately perturbed build
#     (e.g. densityGain * 1.0000001f); the gate must then FAIL.
#
# usage: uv run --no-project --python 3.12 --with pedalboard --with numpy \
#            python -I gate.py <v2.8.0 O-Freeze-dev.vst3> [new.vst3] [--expect-fail]
import os
import sys

import numpy as np
import pedalboard

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "blocksize-null"))

args = [a for a in sys.argv[1:] if not a.startswith("--")]
EXPECT_FAIL = "--expect-fail" in sys.argv
REF_PLUGIN = args[0]
NEW_PLUGIN = args[1] if len(args) > 1 else \
    "/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-Freeze-dev.vst3"
sys.argv = [sys.argv[0], REF_PLUGIN]   # blocksize-null reads its plugin from argv[1]
import gate as bsn  # noqa: E402  (CONFIGS, stimulus, render, make_template)

LIMIT_DB = -140.0   # reported only; the verdict is EXACT equality (see below)
N_TEMPLATES = 2


def render_with(plugin_path, cfg, state, block):
    bsn.PLUGIN = plugin_path
    return bsn.render(cfg, state, block)


def main():
    print(f"v2.8.0 ref: {REF_PLUGIN}\nnew build : {NEW_PLUGIN}")
    fails = 0
    worst = -np.inf
    for cfg in bsn.GRANULAR_CONFIGS:   # the v2.8.0 configs (v2.8.0 has no ENGINE)
        for k in range(N_TEMPLATES):
            bsn.PLUGIN = REF_PLUGIN
            state = bsn.make_template(cfg)
            # old-state migration check
            p = pedalboard.load_plugin(NEW_PLUGIN)
            p.raw_state = state
            eng, dif, avg = str(p.engine), float(p.diffusion), float(p.avg_frames)
            mig_ok = eng == "Granular" and abs(dif) < 1e-6 and abs(avg - 8.0) < 1e-6
            fails += not mig_ok
            print(f"{'PASS' if mig_ok else 'FAIL'} {cfg} seed#{k}: v2.8.0 state loads as "
                  f"engine={eng} diffusion={dif:g} avg_frames={avg:g}")
            for bs in [bsn.REF_BLOCK] + bsn.BLOCKS:
                ref, (fa, fb) = render_with(REF_PLUGIN, cfg, state, bs)
                new, _ = render_with(NEW_PLUGIN, cfg, state, bs)
                rms = bsn.db(np.sqrt(np.mean(ref[:, fa:fb] ** 2)))
                if rms < -40.0:
                    print(f"FAIL {cfg} seed#{k}: frozen section {rms:.1f} dBFS (vacuous)")
                    fails += 1
                d = float(np.max(np.abs(new - ref)))
                worst = max(worst, bsn.db(d))
                # Bit-exact required: a one-ulp densityGain change lands at -138.5 to
                # -144.5 dBFS, so a -140 dBFS floor would let part of it through.
                ok = d == 0.0
                fails += not ok
                tag = "bit-exact" if d == 0 else f"{bsn.db(d):.1f} dBFS"
                print(f"{'PASS' if ok else 'FAIL'} {cfg} seed#{k} block {bs:>3}: v2.9.0 vs v2.8.0 "
                      f"{tag}   (frozen RMS {rms:.1f} dBFS)")
    print("worst difference: 0 (all bit-exact)" if worst <= -599.0 else f"worst difference: {worst:.1f} dBFS")
    if EXPECT_FAIL:
        print("NEGATIVE CONTROL", "PASS (gate failed as it must)" if fails else "FAIL (gate did not see the change)")
        return 0 if fails else 1
    print("GATE", "PASS" if fails == 0 else f"FAIL ({fails})")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
