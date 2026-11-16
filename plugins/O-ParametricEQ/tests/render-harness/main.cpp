/*
   This file is part of the Ouaricon Audio plugin suite.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-ParametricEQ - render harness (Stage 1 gate G11; Stage 2 extends it)

      O-ParametricEQ-render-harness                     run every row
      O-ParametricEQ-render-harness --golden-dir <dir>  compare L1 against <dir>/params.tsv
      O-ParametricEQ-render-harness --dump-params       print the layout golden, exit 0

    Exit code = number of FAIL rows (capped at 255). Last line: "N PASS, M FAIL".

    The golden (tests/golden/params.tsv) is this program's own --dump-params
    output, so on its own it only proves the layout has not moved. It is tied
    to parameter-spec.md by tests/golden/check_spec.py, independently of this
    file.

      L1   196 parameters; dump == <golden-dir>/params.tsv byte for byte
      L2   version hint 1; automatable; 50 Bool / 72 Choice / 74 Float; 25 groups in order
      L3   none of the 72 reserved dynamic-EQ IDs is registered
      L4   bypass parameter; latency 0 at 44.1 k and 96 k; tail 2.0 (amended in Phase 2.2)
      L5   float defaults: normalised exactly 0.5; linear raw exact; freq / q raw within 2e-4
      B1   bus layouts: (1,1) and (2,2) accepted; (2,1), (1,2), (0,2), (2,0), 5.1 refused
      P1   pass-through, memcmp-equal: mono / stereo x blocks 1, 64, 512, 4096 x
           bypass Off / On / every parameter random, and through processBlockBypassed.
           Amended in Phase 2.2: the processBlock "random" variant switches every
           bNN_enabled off and bypass off after the draw (the engine now acts on
           an enabled band); the processBlockBypassed rows keep the full draw
      P1n  vacuity guard for P1: one flipped bit fails the comparator; input RMS > -20 dBFS
      S1   state round trip: 196 getValue() bit-equal, UI members, byte-equal re-save
      S2   XML shape
      S3   two reopens with a UI change between them
      S4   unknown parameter loads and is saved back
      S5   missing parameter loads at its default (with a pre-set guard)
      S6   stateVersion="2" loads; the next save writes "1"
      S7   wrong root tag / truncated / empty blob: nothing changes
      S8   UI values validated by type

    S1 is exact by decision (plan P13). If it misses by one ULP on a skewed
    float, the row prints every ID with both values: do not loosen it.

    Stage 2, Phase 2.2 (PLAN-2.2 T7): the engine through the plugin's float
    output. Every closed-form reference is designed from the raw parameter
    values (rawOf) through eq::makeBandInput and eq::Designer::design. There is
    no audio golden: a bit-identity row compares two renders of this binary.
    A row named "...n" is a negative control and PASSES when the control fires.
    [INFO] lines are printed and never counted.

      E1   FUNC-01: 24 bells at 48 kHz vs the 24 closed forms; each band disabled in turn
      E1n  the full render against each 23-band closed form must fail
      E2   FUNC-02: eight types at 44.1 and 96 kHz, mono and stereo, <= 0.01 dB above -80 dB
      E2n  the bell against the same bell 1 % higher must fail
      N1   DSP-06: eight types at 25 and 30 kHz, 44.1 kHz: finite, closed form
      N2   24 mixed bands at 11 - 30 kHz, 22.05 and 44.1 kHz: finite, peak <= +40 dB
      T1a  G4 (a): no band enabled, noise + denormal tail: memcmp-equal, 7 block patterns
      T1b  G4 (b): Left bands leave the right channel memcmp-equal; and the converse
      T1bn the same bands on Stereo must fail
      T1c  G4 (c): Mid bands leave the side, Side bands the mid, <= -130 dB re the larger peak
      T1cn the same bands on Left must fail
      T1g  G4 (g): block sizes 32, 64, 512, 2048, 1 and a random sequence: bit-identical
      T1m  mono layout: every placement value equals the Stereo-placement render
      Z1   G8: latency 0, nothing before the impulse, predicted first-sample gain
      A1   G6 static: no allocation inside processBlock (malloc_logger, this thread)
      A1n  a planted allocation inside the window is counted
      K1   host blocks of 1024 on a 256 prepare == blocks of 256
      H1   hard bypass: buffer untouched; state cleared
      U1   a gain edit reaches the new closed form
      U2   re-enabling a band starts from cleared state
      U3   inert edits (slope on Bell / Notch / Band Pass, Q at 6 dB/oct, placement on mono)
      V1   DSP-07 engine half: the dynamic offset reads exactly 0
      X-cpu  settled CPU readings, INFO

    Stage 2, Phase 2.3 (PLAN-2.3 T7): continuous modulation through the
    plugin's float output.

      O-ParametricEQ-render-harness --long              S5 at 600 s per rate (default 60 s)

    The G3 verdict is a ratio to a same-motion reference that lives in this
    file (refRender: the control trajectory re-stated here, then a design on
    every sample and the settled recurrences; PLAN-2.3 P11). The no-smoothing
    control is hardRender (the 2.2 rule). Both are fed the plugin's own float
    input, widened, and compared in double. Mid-render parameter edits of
    these rows are stores to the raw parameter values the processor reads
    (parameters.getRawParameterValue), which a worker thread may do; rows
    with many renders run them on worker threads, one plugin instance per
    job, constructed on the main thread, and print in order afterwards.
    The IDs B1, S5 and S6 below are PLAN-2.3's; they are not the Stage 1 rows
    of the same name above (bus layouts, state codec).

      M1   G3: 13 motions x 50 Hz / 1 kHz x blocks 32 / 512, local kappa <= 4.0x refRender
      M1h  hardRender at block 512 reads >= 10x refRender (24 readings; 2 void, printed)
      M1k  motion 8 at 96 kHz (K = 32) and 192 kHz (K = 64) <= 4.0x
      M2   gain_scale 100 -> 25 on three Stereo bells: steady-state gains; kappa of the move
      M2c  the same edit on a Low Cut + Notch: bit-identical to no edit
      M3   bNN_bypass on, then off, on the only active band
      M4   re-prepare at another rate == a fresh instance at that rate
      M5   Mid / Side bell: gain of each component at the bell centre
      M6   a gain edit on a ringing bell keeps the state (<= 10x the ring's own curvature)
      M6n  retired in Phase 2.4 (a type change is a crossfade): see XR-rest, XR-g3h
      B1   events on multiples of 2048 (f, gain, Q, gain_scale): blocks 32 / 64 / 512 / 2048
           bit-identical at four rates; differs from the render without events
      TK   the tick clock runs through bypass, no-active-band and processBlockBypassed blocks
      S5   G5: seeded random stress at 8 rates: stack guard, finite, peak printed, bands off
      S6   a NaN / +inf input sample does not stick
      A2   G6 under stress: no allocation inside processBlock (malloc_logger, this thread)
      A2n  a planted allocation inside the window is counted
      N7   G7 float: noise floor; silence costs no more than signal
      X-cpu2  sweeping CPU readings, INFO

    Stage 2, Phase 2.4 (PLAN-2.4 T7): structural crossfades, the neutral
    sleep and the soft bypass through the plugin's float output.

      O-ParametricEQ-render-harness --long              S5 at 600 s per rate (default 60 s);
                                                        its every-block run is 10 s in both

    A step row is scored against the two held renders of the same instance
    (PLAN-2.4 P13): (a) the largest second difference, (b) local kappa at
    1 kHz, (c) the hard control. Two harness renders carry the rest:
    fadeRefRender (PLAN-2.4 P12: two static chains by Designer::design ->
    toRuntime -> svfTick / onePoleTick, the placement algebra and the weight
    formula re-stated in this file; none of the two-slot band, the fade
    table, the bypass fade, eq::Band or eq::Router) and hardStepRender
    (PLAN-2.4 P10: the Phase 2.3 rule on plain eq::Band + eq::Router). Both
    are fed the plugin's own float input, widened, and compared in double.
    Rows with many renders run on worker threads, one plugin instance per
    thread, constructed on the main thread; every job rewrites all the
    parameters it uses. N below is the fade length, round (0.020 fs).

      XR-g3   G3 step rows: 242 + 15 band-bypass rows x 50 Hz / 1 kHz x blocks 32 / 512:
              largest second difference <= 2.0x the louder held render
      XR-g3k  the same renders at 1 kHz: local kappa <= 2.5x held; 50 Hz printed
      XR-g3h  hardStepRender at block 512 reads >= 10x held; three void readings printed
      XR-g3b  the Global bypass group through processBlockBypassed
      XR-ref  30 first fades at 48 and 96 kHz against fadeRefRender, <= -120 dB re the peak
      XR-rest after a type change on the ringing bell the output from the fade's end equals
              a fresh instance started at the fade's first sample
      XR-g4d  G4 (d): 24 bells at 0 dB; 24 bells 6 dB -> 0, 200 ms later: memcmp-equal
      XR-g4e  G4 (e): gain_scale -> 0 %, 200 ms later: memcmp-equal
      XR-g4f  G4 (f): bypass on, by the parameter and by processBlockBypassed: equal from
              exactly N samples after the block that sees it
      XR-g4n  (d), (e) with a 0.001 dB gain; (f) read 5 ms after the toggle: must differ
      XR-g2   G2 live: each of the 8 slopes of both cuts reached from its neighbour
      XR-byp  a parameter changed while bypassed is in effect, settled, after un-bypass
      XR-bypn the same compare against the parameters from before the bypass must differ
      XR-big  host blocks of 4096 on a prepared size of 512 == prepared 4096
      XR-prep re-prepare with a fade and a ramp in flight, then moves in blocks of 500
      XR-blk  structural, continuous, gain_scale and bypass events on multiples of 2048:
              blocks 32 / 64 / 512 / 2048 bit-identical at four rates
      A3      G6 with fades: no allocation inside processBlock / processBlockBypassed
      A3n     a planted allocation inside the window is counted
      X-cpu3  CPU with fades, INFO

    Existing rows amended in Phase 2.4, by name (PLAN-2.4 P18; each carries a
    comment at its code): H1 (ii), M3 and U2 compare from the toggle + N;
    A1 checks the bypassed block after the fade has ended; S5 runs the P14
    draw (structure with probability 1/50 per band and block, global bypass
    1/200, plus a run that redraws the structure at every block); M6n is
    retired. TK, by Taylor's call in the execute session: the on-tick case
    (edit at 1024) first differs at 1026 (the 0 dB bell sleeps; its wake
    fade's first two weights round away in float).

    Step-row parameters: every step render runs gain_scale 200 % with the raw
    gain at half the bench's effective gain, so the effective gains are the
    bench's exactly (Tilt +-36 dB lies outside the raw +-30 dB range).

    Stage 2, Phase 2.5 (PLAN-2.5 T7): output gain and auto gain (eq::OutputStage,
    eq::AutoGain) through the plugin's float output.

      O-ParametricEQ-render-harness --long              S5 at 600 s per rate (default 60 s);
                                                        its every-block run is 10 s in both

    Three harness references (PLAN-2.5 P8), none of which uses GainSmoother,
    OutputStage or AutoGain: gainRefRender (the four-pole recurrence, the
    tick / refresh schedule and the seeding of P2, P3, P5 re-stated per
    sample, times the plugin's own float output rendered with the gains
    neutral, widened, compared in double), hardGainRender (the same output,
    the gain jumping to its target at the sample the stage would start
    moving) and agRef (the estimator of P4 re-stated: 512 cell midpoints,
    Designer::design, sectionMag2 products, lanes, mean, guards, clamp).
    Gain-row parameters are raw stores, as in 2.3 / 2.4.

      XR-g12o   G12 output gain -36 ... +36 dB, 1 kHz: RMS ratio within 1e-4 dB (limit 0.05 dB)
      XR-g12s   gain_scale 200 % on +-6 dB bells: +-12 dB within the G1 tolerance
      XR-g12a   auto gain, +12 dB Low Shelf 200 Hz, band-limited pink: out / in RMS within
                +-0.5 dB; agRef -6.167 dB +- 0.001
      XR-g12d   two renders of XR-g12a bit-identical
      XR-g12n   XR-g12a with auto_gain off reads outside +-0.5 dB (control fires)
      XR-gstep  G3 step rows: the 20 output-gain pairs and auto_gain toggles (estimates
                -18 / -6 / +6 / +12 dB), a Bell running, 50 Hz / 1 kHz, blocks 32 / 512:
                (a) <= 2.0x the louder held render; 1 kHz local kappa <= 2.5x
      XR-gsteph hardGainRender at block 512 reads >= 10x held (control fires)
      XR-gret   output-gain reversals (4 cases x 25 / 50 / 75 / 90 %, 8 phases), blocks 32 / 512
                cut at the edits: <= 2.0x held
      XR-gsweep output gain -30 -> +30 -> -30 dB in 20 ms against the per-sample automation;
                auto gain following a bell sweep against gainRefRender on agRef targets: <= 4.0x
      XR-ref    every XR-gstep / XR-gret render against gainRefRender: <= -120 dB re the peak
      XR-g4a    G4 (a) with the stage: no band, output 0 dB, auto_gain off and on: memcmp-equal
      XR-g4an   output +0.001 dB; auto_gain on with one Left +1 dB bell: each differs (control)
      XR-idle   no band, output 0 -> +6 -> 0 dB: memcmp-equal from 60 ms after the second edit
      XR-seed   (i) opened at +12 dB, auto on, +12 dB shelf: the settled gain from the first
                sample; (ii) edits while bypassed in effect from un-bypass + N, both routes
      XR-seedn  (ii) against the parameters from before the bypass differs (control fires)
      XR-gbyp   no band, output +12 dB: a bypass toggle fades over exactly N samples, both ways
      XR-unb    VERIFICATION-2.4 Issue 2: un-bypass latched during the fade to bypass (PX14, PX9)
      XR-unbn   the pre-edit parameters; the second fade one sample late: each differs (control)
      XR-blk5   4 bands, band / output_gain / auto_gain events on multiples of 2048: blocks
                32 / 64 / 512 / 2048 bit-identical at four rates; evaluation counts printed
      XR-meter  readAndResetOutputPeak: a one-sample +6 dBFS spike between two reads, every path
      A4        G6 with the stage: no allocation, auto gain refreshing with 24 bands moving
      A4n       a planted allocation inside the window is counted
      X-cpu4    CPU with auto gain, INFO, beside X-cpu3

    Existing rows amended in Phase 2.5, by name (PLAN-2.5 P12; a comment at
    each one's code): S5 also draws output_gain and auto_gain per block, its
    stack guard is stack + the drawn output gain + 12 dB (execute-session
    call: no +12 dB peak margin; up to 256 seeds), and after it the output
    returns to the input within 300 ms. P1's processBlock "all random"
    variant also sets output_gain to exactly 0 dB and auto_gain off
    (execute-session call).

    Stage 2, Phase 2.6 (PLAN-2.6 T9): the display model, the analyzer taps
    and band listen through the plugin.

      O-ParametricEQ-render-harness --lid-dir <dir>     XR-lid's 0.5.0 session dump (else $OPEQ_LID_DIR)
      O-ParametricEQ-render-harness --dump-sessions <dir>
                                                        write the 16 XR-lid sessions (PLAN-2.6 T1, P13)
      O-ParametricEQ-render-harness --dump-g9 <dir>     write the 260 XR-g9 draws for tests/g9b (PLAN-3.2 P4)
      O-ParametricEQ-render-harness --rows33            only the Phase 3.3 rows (PLAN-3.3 T11 mutant runs)

    Harness references (PLAN-2.6 P12), none of which calls eq::Listen,
    OutputStage::process / nextGains or a DisplayModel design call:
    listenRefRender (P4 - P6 re-stated per sample: the D11 mapping, the solo
    router, two static RefChain voices, the 30 ms input ramp and the 20 ms
    fade from the weight formula, the tick schedule, the latch, the gains as
    constants; fed the plugin's dry input and its EQ path), listenHardRender
    (R = 0, N = 1; or N = 20 ms, the fade from rest), and the G9 closed form
    (composeFromPackets: the display model's packets parsed back from their
    var, eq::bandResponseAtZ, the 2x2 placement matrices in index order;
    dampedResponse: the weighted direct sum of the plugin's float impulse
    responses; slowestPoleRadius from the packets). Rows with many renders
    run on worker threads, one instance per worker (makePool / poolFor).

      XR-g9     G9 (a) (C1): 50 random 1 - 24 band configurations x 44.1 / 48 / 88.2 / 96 /
                192 kHz stereo + 10 mono; four impulses, 2^17 samples, a^N = 1e-9; the closed
                form from the processor's own DisplayModel packets: LL / RR / MM / SS <= 0.05 dB
                within 80 dB of the largest maximum among the four and >= -120 dB re the impulse
                (Taylor's call 2026-11-15; configurations with no such point: void, listed)
      XR-g9u    the unit circle on draws whose slowest pole falls 200 dB within 6 s, rendered
                1.25x that time, >= 25 per rate: <= 0.05 dB
      XR-g9n    controls on the closed form (k x 1.01, one-pole dropped, an L band and an M band
                in the wrong order, Mid for Side): each > 0.05 dB
      XR-g9id   packets (parsed var and the model's BandDesign) == getBandRealisedDesign field
                by field, bit for bit; disabled / bypassed / 0 dB Bell / gain scale 0 %: inactive
      XR-g9j    JSON round trip: <= 1e-7 dB from the unserialised composition
      XR-dm     DisplayModel::poll: no edit, one band, a preset load of 5 bands, a new rate,
                before any prepare (rateKnown false, 48 kHz)
      XR-an     -12 dBFS 1 kHz, five rates, 8 phases, pre and post: -12 +- 0.5 dB
      XR-answ   50 log tones 20 Hz - 20 kHz, five rates: -12 +- 0.25 dB
      XR-ann    zero-pad 1x re-stated in the harness at 96 kHz: below -12.5 dB (control)
      XR-anpp   +12 dB Bell 1 kHz: post - pre = +12 +- 0.5 dB
      XR-anrel  no processBlock: the display falls 20 dB/s (+-1 dB over 2 s); instant attack
      XR-gate   consumer and analyzerMode gate the tap writes on every path; unprepared: none
      XR-ovf    no drain for 2 s at blocks 32 / 4096 / 65 536: every block returns, exact drops,
                the oldest fitting run (a 65 536 block: its last 32 768), the reading within 3
                process calls (P2; Taylor's call 2026-11-15)
      XR-gen    prepareToPlay at a new rate between drains (one thread; and a second-thread
                reader): no old-rate sample reaches a transform; re-prepared at the new size
      XR-lon    listen on / off, the 20 XL-on bands, 50 Hz / 1 kHz, 4 phases, blocks 32 / 512,
                48 / 96 kHz: (a) <= 2.0x; 1 kHz kappa <= 2.5x (kappa-void when the two held
                levels at the tone are > 40 dB apart: Taylor's call 2026-11-15)
      XR-lonh   listenHardRender on the same: >= 10x on every non-void reading (block 512)
      XR-lchg   64 sampled ordered band changes, 48 kHz, blocks 32 / 512: (a) <= 2.0x, (b) <= 2.5x
      XR-lref   every XR-lon / XR-lchg render against listenRefRender: <= -120 dB re peak
      XR-lid    the 16 T1 sessions == the 0.5.0 dump (memcmp), also with the analyzer drained
                by a second thread
      XR-lidn   one listenBand write must differ (control)
      XR-loff   listen on -> off, no band: memcmp-equal to the input from the release end (the
                denormal tail: from the first block after it)
      XR-lroute the solo router per placement, stereo and mono, settled; R2 Left / Right engage
                at block 2048: <= -140 dB re peak of listenRefRender
      XR-lseed  listen at session open, changed while bypassed, with a latched un-bypass, both
                routes: the settled listen from the first processed sample, <= -140 dB
      XR-lseedn the same against an unseeded engage must differ (control)
      XR-lag    C3: listen level independent of auto gain (1e-6 dB); output gain +6 -> +6 dB
      XR-blk6   XR-blk5 + listenBand events off the grid + analyzer on: blocks 1 / 32 / 64 /
                512 / 2048 / offline bit-identical at 44.1 / 96 / 192 kHz
      XR-lbyp   bypass during listening and each move kind, both routes: (a) <= 2.0x, the fade
                mixes the listen output with the dry input (<= -120 dB)
      XR-px17   VERIFICATION-2.5 PX17: latched un-bypass, no band, output +12 dB: no fade-in
      XR-px11   PX11: output-gain edits off the tick grid at random offsets: bit-identical
      XR-px4    PX4: an auto-gain toggle in an otherwise idle block lands on the refresh tick
      XR-ag3    AG3: auto gain at 22.05 / 32 kHz against the pink truth with the 0.45 fs top
      A5        G6 with the rings saturated, listen switching, the A4 load: 0 allocations
      A5n       a planted allocation inside the window is counted
      X-cpu5    CPU with the taps and listen (one / two voices), INFO, beside X-cpu4

    Stage 3, Phase 3.1 (CONTEXT U8): the rows VERIFICATION-2.6 Issue 2 owes,
    ported from stages/2-dsp/verify-2.6/probe.cpp, plus three real-by-read
    mutants that no row gated.

      XR-ls10   LS10 / PX27 (probe E1): an f / Q edit inside the incoming voice's ramp is
                heard from move sample R (within one grid); a settled listen's edit moves it
      XR-ls11   LS11 / LS12 (probe T1): the listen state at block 1 = the re-stated P6
                machine (latches; settled f edit ramps, no move; type family -> change)
      XR-ls15   LS15 / PX26 (probe N1): a NaN input, listen settled / off: finite from the
                next block
      XR-px38   PX38 (probe R1): listenBand 24, 1000, -2, INT_MIN, INT_MAX read as off
      XR-px39   PX39 (probe X1): a latched un-bypass with listen engaged and no band fades
      XR-px22   PX22: mono, a placement edit of the listened band starts nothing (memcmp);
                control: stereo starts moves
      XR-dm2    DM2: a layout change at the same rate re-designs all 24 (effective placement)
      XR-an1    AN1: no analyzer frame straddles a drop (a 2 s stall)

    Stage 3, Phase 3.1 (PLAN-3.1 P6, call C2): the processor's preset manager,
    in a per-run temp home (main() re-execs once with CFFIXED_USER_HOME; the
    rows verify the presets directory is under it before the first write).

      XR-pm     savePreset / loadPreset round trip of all 196 (seeded): every value but
                output_gain / bypass bit-equal; those two, the six UI atomics and
                listenBand untouched; current name. Control: no exclusions -> output_gain moves
      XR-pmd    a preset carrying only b03_gain: every other non-excluded value at its
                default (WR-01 reset), b03_gain applied, output_gain / bypass untouched
      XR-psg    Factory/Locked.json: isFactoryPreset, PresetSaveGuard refuses the name and
                the file, not a user name. Control: file absent -> the name is not refused

    Stage 3, Phase 3.3 (PLAN-3.3 T3): the listen NaN fix (P1), analyzer Off
    gating and the meter feed's hold rule (P4; the shipping editor/PeakHold.h).

      N1e       VERIFICATION-2.6 Issue 1 (probe33 n): a NaN inside listen's input ramp is
                caught at its own block: 16 cases (12 / 16 before the fix)
      N1e-u     the voice flag is taken every chunk, not inside `! finite || ...`: NaN on
                the voice's channel vs the other channel, a lane non-finite in both: the
                renders are bit-equal from the next block
      AO1       analyzer mode 0 drains 0 / 0; mode 2 drains post only
      AO2       the analyzer's Off rule: a tap switched back on after 3 s Off draws no
                pre-gap audio (>= 55 dB under the noise's top) (control: the mutant "Off rule removed")
      M1        PeakHold over the editor's 60 Hz reads: -6 dBFS within 0.5 dB at 44.1 / 48 /
                192 kHz, blocks 1024 - 4096; H = 1 control shows empty reads

    Existing rows amended in Phase 2.6, by name (PLAN-2.6 P16; a comment at
    its code): S5 also redraws listenBand (p 1/200 per block) and the
    analyzer mode (p 1/500) with the consumer registered and no reader, and
    sets listen -1 before its end clause. Every other existing row runs with
    listen -1 and no consumer. A4's and A3's stress loop passes no listen
    draw (stressLoopCore's new argument is nullptr there: the 2.5 loop bit
    for bit).

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "dsp/EqTypes.h"
#include "dsp/BandParams.h"
#include "dsp/Designer.h"
#include "dsp/ResponseEval.h"
#include "dsp/Band.h"
#include "dsp/Router.h"
#include "dsp/Fade.h"
#include "dsp/OutputStage.h"
#include "dsp/AutoGain.h"
#include "dsp/Listen.h"
#include "dsp/SpectrumTap.h"
#include "analysis/SpectrumAnalyzer.h"
#include "analysis/DisplayModel.h"
#include "PresetSaveGuard.h"
#include "editor/PeakHold.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined (__APPLE__)
 #include <pthread.h>
 #include <pwd.h>
 #include <unistd.h>
 #include <mach-o/dyld.h>   // _NSGetExecutablePath (fake-home re-exec, PLAN-3.1 P6)

 // libmalloc's stack-logging hook (what MallocStackLogging / Instruments attach
 // to). Same declaration as the designer harness (rows A1 / A1n).
 extern "C"
 {
     typedef void (malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                     uintptr_t result, uint32_t numHotFramesToSkip);
     extern malloc_logger_t* malloc_logger;
 }
#endif

#if defined (__clang__)
 #pragma clang diagnostic ignored "-Wfloat-equal"
#endif

// juce_audio_plugin_client (which normally declares it) is not linked into a
// console target.
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

#ifndef GOLDEN_DIR
 #error "GOLDEN_DIR must point at tests/golden"
#endif

namespace
{
    using Processor = OuariconParametricEQAudioProcessor;

    constexpr int kExpectedParams = 196;
    constexpr int kExpectedBools = 50, kExpectedChoices = 72, kExpectedFloats = 74;
    constexpr int kExpectedGroups = 25;

    // "The stimulus was applied" guard for the seeded-random rows. A uniform
    // random normalised value leaves about 163 of the 196 away from default
    // (half of the Bools and a share of the Choices land on it).
    constexpr int kMinNonDefault = 100;

    //==========================================================================
    int failures = 0, passes = 0;

    void row (bool ok, const juce::String& name, const juce::String& detail = {})
    {
        std::printf ("  [%s] %-70s %s\n", ok ? "PASS" : "FAIL", name.toRawUTF8(), detail.toRawUTF8());
        std::fflush (stdout);

        if (ok) ++passes; else ++failures;
    }

    void section (const char* title)
    {
        std::printf ("\n-- %s\n", title);
    }

    // Collects the sub-failures of a many-item row; keeps the first few as notes.
    struct Tally
    {
        int total = 0, bad = 0;
        juce::StringArray notes;

        void check (bool ok, const juce::String& what)
        {
            ++total;

            if (! ok)
            {
                ++bad;

                if (notes.size() < 4)
                    notes.add (what);
            }
        }

        juce::String detail() const
        {
            juce::String text = juce::String (total - bad) + "/" + juce::String (total);

            if (bad > 0)
                text += "  e.g. " + notes.joinIntoString ("; ");

            return text;
        }
    };

    std::string fmt (double value)
    {
        char text[64];
        std::snprintf (text, sizeof (text), "%.9g", value);
        return text;
    }

    juce::String fmtJ (double value)
    {
        return juce::String (fmt (value));
    }

    bool sameBits (float a, float b)
    {
        return std::memcmp (&a, &b, sizeof (float)) == 0;
    }

    //==========================================================================
    std::unique_ptr<Processor> make()
    {
        std::unique_ptr<juce::AudioProcessor> base (createPluginFilter());
        auto* typed = dynamic_cast<Processor*> (base.get());

        if (typed == nullptr)
        {
            std::printf ("FATAL: createPluginFilter() did not return the O-ParametricEQ processor\n");
            std::exit (255);
        }

        base.release();
        return std::unique_ptr<Processor> (typed);
    }

    std::vector<juce::RangedAudioParameter*> rangedParams (juce::AudioProcessor& p)
    {
        std::vector<juce::RangedAudioParameter*> list;

        for (auto* parameter : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                list.push_back (r);

        return list;
    }

    juce::RangedAudioParameter* findParam (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* r : rangedParams (p))
            if (r->getParameterID() == id)
                return r;

        return nullptr;
    }

    // The host side: a normalised value, through the host-notifying path.
    void setNormalised (juce::AudioProcessor& p, const juce::String& id, float normalised)
    {
        if (auto* r = findParam (p, id))
            r->setValueNotifyingHost (normalised);
        else
            row (false, "harness: no parameter '" + id + "'");
    }

    // An engineering value, converted and written the same way.
    void setRaw (juce::AudioProcessor& p, const juce::String& id, float raw)
    {
        if (auto* r = findParam (p, id))
            r->setValueNotifyingHost (r->convertTo0to1 (raw));
        else
            row (false, "harness: no parameter '" + id + "'");
    }

    void setAllRandom (juce::AudioProcessor& p, juce::int64 seed)
    {
        juce::Random random (seed);

        for (auto* r : rangedParams (p))
        {
            const auto x = random.nextFloat();

            // Discrete parameters get a LEGAL normalised value (one a host or the
            // parameter's own UI can produce). juce::AudioParameterBool stores an
            // off-grid value such as 0.37 verbatim in getValue() while the state
            // tree holds the thresholded 0 / 1, so an off-grid stimulus can never
            // round-trip bit-equal. Continuous parameters keep the raw draw.
            r->setValueNotifyingHost (r->isDiscrete() ? r->convertTo0to1 (r->convertFrom0to1 (x)) : x);
        }
    }

    // "b07_freq" from (7, "freq").
    juce::String bandId (int band, const char* suffix)
    {
        return "b" + juce::String (band).paddedLeft ('0', 2) + "_" + suffix;
    }

    void disableEveryBand (juce::AudioProcessor& p)
    {
        for (int band = 1; band <= Processor::kNumBands; ++band)
            setNormalised (p, bandId (band, "enabled"), 0.0f);
    }

    std::vector<float> valuesOf (juce::AudioProcessor& p)
    {
        std::vector<float> values;

        for (auto* r : rangedParams (p))
            values.push_back (r->getValue());

        return values;
    }

    int countNonDefault (juce::AudioProcessor& p)
    {
        int n = 0;

        for (auto* r : rangedParams (p))
            if (! sameBits (r->getValue(), r->getDefaultValue()))
                ++n;

        return n;
    }

    float rawOf (Processor& p, const juce::String& id)
    {
        if (auto* raw = p.parameters.getRawParameterValue (id))
            return raw->load();

        row (false, "harness: no raw value for '" + id + "'");
        return std::numeric_limits<float>::quiet_NaN();
    }

    //==========================================================================
    // UI state (the six non-parameter members)
    //==========================================================================
    struct UiState
    {
        int language = 0, analyzer = 0, range = 0, band = 0, width = 0, height = 0;

        bool operator== (const UiState& other) const
        {
            return language == other.language && analyzer == other.analyzer && range == other.range
                && band == other.band && width == other.width && height == other.height;
        }

        juce::String describe() const
        {
            return "lang " + juce::String (language) + " mode " + juce::String (analyzer)
                 + " range " + juce::String (range) + " band " + juce::String (band)
                 + " size " + juce::String (width) + "x" + juce::String (height);
        }
    };

    UiState uiOf (const Processor& p)
    {
        UiState s;
        s.language = p.uiLanguage.load();
        s.analyzer = p.analyzerMode.load();
        s.range    = p.uiDisplayRange.load();
        s.band     = p.uiSelectedBand.load();
        s.width    = p.uiEditorWidth.load();
        s.height   = p.uiEditorHeight.load();
        return s;
    }

    void setUi (Processor& p, const UiState& s)
    {
        p.uiLanguage.store (s.language);
        p.analyzerMode.store (s.analyzer);
        p.uiDisplayRange.store (s.range);
        p.uiSelectedBand.store (s.band);
        p.uiEditorWidth.store (s.width);
        p.uiEditorHeight.store (s.height);
    }

    UiState makeUi (int language, int analyzer, int range, int band, int width, int height)
    {
        UiState s;
        s.language = language; s.analyzer = analyzer; s.range = range;
        s.band = band; s.width = width; s.height = height;
        return s;
    }

    //==========================================================================
    // State blobs
    //==========================================================================
    juce::MemoryBlock stateOf (juce::AudioProcessor& p)
    {
        juce::MemoryBlock block;
        p.getStateInformation (block);
        return block;
    }

    void loadState (juce::AudioProcessor& p, const juce::MemoryBlock& block)
    {
        p.setStateInformation (block.getData(), (int) block.getSize());
    }

    std::unique_ptr<juce::XmlElement> xmlOf (const juce::MemoryBlock& block)
    {
        return juce::AudioProcessor::getXmlFromBinary (block.getData(), (int) block.getSize());
    }

    juce::MemoryBlock blobOf (const juce::XmlElement& xml)
    {
        juce::MemoryBlock block;
        juce::AudioProcessor::copyXmlToBinary (xml, block);
        return block;
    }

    int countChildren (const juce::XmlElement& xml, const char* tag)
    {
        int n = 0;

        for (auto* child : xml.getChildIterator())
            if (child->hasTagName (tag))
                ++n;

        return n;
    }

    juce::XmlElement* paramChild (juce::XmlElement& xml, const juce::String& id)
    {
        for (auto* child : xml.getChildIterator())
            if (child->hasTagName ("PARAM") && child->getStringAttribute ("id") == id)
                return child;

        return nullptr;
    }

    //==========================================================================
    // Layout dump (the golden)
    //==========================================================================
    using GroupMap = std::map<const juce::AudioProcessorParameter*, juce::String>;

    void walkGroups (const juce::AudioProcessorParameterGroup& group, const juce::String& groupId,
                     GroupMap& groupOf, juce::StringArray& order)
    {
        for (const auto* node : group)
        {
            if (auto* sub = node->getGroup())
            {
                order.add (sub->getID());
                walkGroups (*sub, sub->getID(), groupOf, order);
            }
            else if (auto* parameter = node->getParameter())
            {
                groupOf[parameter] = groupId;
            }
        }
    }

    const char* typeOf (juce::AudioProcessorParameter* parameter)
    {
        if (dynamic_cast<juce::AudioParameterFloat*> (parameter) != nullptr)   return "Float";
        if (dynamic_cast<juce::AudioParameterChoice*> (parameter) != nullptr)  return "Choice";
        if (dynamic_cast<juce::AudioParameterBool*> (parameter) != nullptr)    return "Bool";
        return "Other";
    }

    // A cell can hold neither a tab nor a line break.
    std::string cell (const juce::String& text)
    {
        if (text.isEmpty())
            return "-";

        return text.replace ("\t", " ").replace ("\r", " ").replace ("\n", " ").toStdString();
    }

    /** Columns: index, id, name, group, type, versionHint, start, end, interval,
        skew, defaultNorm, defaultRaw, numSteps, choices, automatable.
        Floats %.9g; choices joined with '|'; empty cells '-'; "\n" endings.
        The range columns are filled for Float rows only: a Bool or Choice
        range is JUCE's own, not something the layout chooses. */
    std::string buildDump (juce::AudioProcessor& p)
    {
        GroupMap groupOf;
        juce::StringArray groupOrder;
        walkGroups (p.getParameterTree(), {}, groupOf, groupOrder);

        std::string out;
        out += "# GENERATED by O-ParametricEQ-render-harness --dump-params; do not edit\n";
        out += "index\tid\tname\tgroup\ttype\tversionHint\tstart\tend\tinterval\tskew\t"
               "defaultNorm\tdefaultRaw\tnumSteps\tchoices\tautomatable\n";

        int index = 0;

        for (auto* parameter : p.getParameters())
        {
            auto* r = dynamic_cast<juce::RangedAudioParameter*> (parameter);
            const auto found = groupOf.find (parameter);

            out += std::to_string (index++);
            out += '\t';
            out += cell (r != nullptr ? r->getParameterID() : juce::String ("<no-id>"));
            out += '\t';
            out += cell (parameter->getName (128));
            out += '\t';
            out += cell (found != groupOf.end() ? found->second : juce::String());
            out += '\t';
            out += typeOf (parameter);
            out += '\t';
            out += std::to_string (parameter->getVersionHint());
            out += '\t';

            if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (parameter))
            {
                const auto& range = f->getNormalisableRange();
                out += fmt ((double) range.start);    out += '\t';
                out += fmt ((double) range.end);      out += '\t';
                out += fmt ((double) range.interval); out += '\t';
                out += fmt ((double) range.skew);     out += '\t';
            }
            else
            {
                out += "-\t-\t-\t-\t";
            }

            const float defaultNorm = parameter->getDefaultValue();
            out += fmt ((double) defaultNorm);
            out += '\t';
            out += r != nullptr ? fmt ((double) r->convertFrom0to1 (defaultNorm)) : std::string ("-");
            out += '\t';
            out += std::to_string (parameter->getNumSteps());
            out += '\t';

            if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (parameter))
                out += cell (c->choices.joinIntoString ("|"));
            else
                out += "-";

            out += '\t';
            out += parameter->isAutomatable() ? "1" : "0";
            out += '\n';
        }

        return out;
    }

    //==========================================================================
    // L. layout
    //==========================================================================
    void layoutRows (const juce::File& goldenDir)
    {
        section ("L. parameter layout");

        auto p = make();
        const auto& params = p->getParameters();
        const auto ranged = rangedParams (*p);

        // L1
        {
            const auto dump = buildDump (*p);
            const auto goldenFile = goldenDir.getChildFile ("params.tsv");
            juce::MemoryBlock golden;
            const bool readOk = goldenFile.existsAsFile() && goldenFile.loadFileAsData (golden);
            const bool same = readOk && golden.getSize() == dump.size()
                           && std::memcmp (golden.getData(), dump.data(), dump.size()) == 0;

            juce::String detail = juce::String (params.size()) + " params";

            if (! readOk)
            {
                detail += "; cannot read " + goldenFile.getFullPathName();
            }
            else if (! same)
            {
                // Name the first differing line.
                const std::string goldenText (static_cast<const char*> (golden.getData()), golden.getSize());
                size_t a = 0, b = 0;
                int line = 1;

                while (a < dump.size() && b < goldenText.size())
                {
                    const auto ea = dump.find ('\n', a);
                    const auto eb = goldenText.find ('\n', b);
                    const auto la = dump.substr (a, ea == std::string::npos ? std::string::npos : ea - a);
                    const auto lb = goldenText.substr (b, eb == std::string::npos ? std::string::npos : eb - b);

                    if (la != lb || ea == std::string::npos || eb == std::string::npos)
                        break;

                    a = ea + 1;
                    b = eb + 1;
                    ++line;
                }

                detail += "; differs from " + goldenFile.getFullPathName() + " at line " + juce::String (line)
                        + " (plugin " + juce::String ((int) dump.size()) + " bytes, golden "
                        + juce::String ((int) golden.getSize()) + " bytes)";
            }

            row (params.size() == kExpectedParams && (int) ranged.size() == kExpectedParams && same,
                 "L1 196 parameters; dump == golden params.tsv byte for byte", detail);
        }

        // L2
        {
            Tally hints, automatable;
            int numFloat = 0, numChoice = 0, numBool = 0;

            for (auto* parameter : params)
            {
                const juce::String name = parameter->getName (64);
                hints.check (parameter->getVersionHint() == 1, name + " hint " + juce::String (parameter->getVersionHint()));
                automatable.check (parameter->isAutomatable(), name);

                const juce::String type (typeOf (parameter));
                numFloat  += type == "Float"  ? 1 : 0;
                numChoice += type == "Choice" ? 1 : 0;
                numBool   += type == "Bool"   ? 1 : 0;
            }

            row (hints.bad == 0 && hints.total == kExpectedParams, "L2 every version hint is 1", hints.detail());
            row (automatable.bad == 0 && automatable.total == kExpectedParams, "L2 every parameter automatable", automatable.detail());
            row (numBool == kExpectedBools && numChoice == kExpectedChoices && numFloat == kExpectedFloats,
                 "L2 types 50 Bool / 72 Choice / 74 Float",
                 juce::String (numBool) + " / " + juce::String (numChoice) + " / " + juce::String (numFloat));

            GroupMap groupOf;
            juce::StringArray order;
            walkGroups (p->getParameterTree(), {}, groupOf, order);

            juce::StringArray expected { "global" };

            for (int n = 1; n <= Processor::kNumBands; ++n)
                expected.add ("band" + juce::String (n).paddedLeft ('0', 2));

            Tally grouped;

            for (auto* parameter : params)
            {
                const auto found = groupOf.find (parameter);
                grouped.check (found != groupOf.end() && found->second.isNotEmpty(), parameter->getName (64));
            }

            row (order == expected && order.size() == kExpectedGroups && grouped.bad == 0,
                 "L2 25 groups: global, band01 ... band24, in order; every parameter in one",
                 juce::String (order.size()) + " groups, grouped " + grouped.detail());
        }

        // L3
        {
            Tally reserved;

            for (int n = 1; n <= Processor::kNumBands; ++n)
                for (const char* suffix : { "dyn_range", "dyn_threshold", "dyn_auto" })
                {
                    const juce::String id = "b" + juce::String (n).paddedLeft ('0', 2) + "_" + suffix;
                    reserved.check (p->parameters.getParameter (id) == nullptr, id);
                }

            // Guard: the lookup does find a registered ID.
            const bool lookupWorks = p->parameters.getParameter ("b01_freq") != nullptr
                                  && p->parameters.getParameter ("b24_placement") != nullptr;

            row (reserved.bad == 0 && reserved.total == 72 && lookupWorks,
                 "L3 none of the 72 reserved dynamic-EQ IDs is registered", reserved.detail());
        }

        // L4
        {
            auto* bypass = findParam (*p, "bypass");
            row (bypass != nullptr && p->getBypassParameter() == bypass,
                 "L4 getBypassParameter() is the parameter with ID 'bypass'");

            p->prepareToPlay (44100.0, 512);
            const int latency44 = p->getLatencySamples();
            p->releaseResources();
            p->prepareToPlay (96000.0, 512);
            const int latency96 = p->getLatencySamples();
            p->releaseResources();

            row (latency44 == 0 && latency96 == 0, "L4 latency 0 after prepareToPlay at 44.1 k and 96 k",
                 juce::String (latency44) + " / " + juce::String (latency96));
            // Amended in Phase 2.2 (PLAN-2.2 P9, P10): the engine rings.
            row (p->getTailLengthSeconds() == 2.0, "L4 tail 2.0", fmtJ (p->getTailLengthSeconds()));
        }

        // L5 (a fresh instance: nothing has been written)
        {
            auto q = make();
            Tally norm, linear, skewed;
            juce::String actual;

            for (auto* r : rangedParams (*q))
            {
                if (dynamic_cast<juce::AudioParameterFloat*> (r) == nullptr)
                    continue;

                const auto id = r->getParameterID();
                const float defaultNorm = r->getDefaultValue();
                const float converted = r->convertFrom0to1 (defaultNorm);
                const float live = rawOf (*q, id);

                norm.check (sameBits (defaultNorm, 0.5f), id + " norm " + fmtJ ((double) defaultNorm));

                const bool isFreq = id.endsWith ("_freq");
                const bool isQ    = id.endsWith ("_q");

                if (isFreq || isQ)
                {
                    const double target = isFreq ? 1000.0 : 1.0;
                    const bool ok = std::abs ((double) converted - target) / target <= 2e-4
                                 && std::abs ((double) live - target) / target <= 2e-4;
                    skewed.check (ok, id + " raw " + fmtJ ((double) live));

                    if (id == "b01_freq" || id == "b01_q")
                        actual += (actual.isEmpty() ? "" : ", ") + id + " raw " + fmtJ ((double) live)
                                + " (convertFrom0to1 " + fmtJ ((double) converted) + ")";
                }
                else
                {
                    const float target = id == "gain_scale" ? 100.0f : 0.0f;
                    linear.check (sameBits (converted, target) && sameBits (live, target),
                                  id + " raw " + fmtJ ((double) live));
                }
            }

            row (norm.bad == 0 && norm.total == kExpectedFloats, "L5 normalised default of every float exactly 0.5", norm.detail());
            row (linear.bad == 0 && linear.total == 2 + Processor::kNumBands,
                 "L5 linear raw defaults exact (output_gain 0, gain 0, gain_scale 100)", linear.detail());
            row (skewed.bad == 0 && skewed.total == 2 * Processor::kNumBands,
                 "L5 raw freq / q defaults within 2e-4 relative of 1000 / 1.0", skewed.detail() + "  " + actual);
        }
    }

    //==========================================================================
    // B. buses
    //==========================================================================
    juce::AudioProcessor::BusesLayout busLayout (const juce::AudioChannelSet& in, const juce::AudioChannelSet& out)
    {
        juce::AudioProcessor::BusesLayout layout;
        layout.inputBuses.add (in);
        layout.outputBuses.add (out);
        return layout;
    }

    void busRows()
    {
        section ("B. bus layouts");
        using CS = juce::AudioChannelSet;

        auto p = make();

        const bool accepted = p->checkBusesLayoutSupported (busLayout (CS::mono(), CS::mono()))
                           && p->checkBusesLayoutSupported (busLayout (CS::stereo(), CS::stereo()));

        Tally refused;
        refused.check (! p->checkBusesLayoutSupported (busLayout (CS::stereo(), CS::mono())), "(2,1)");
        refused.check (! p->checkBusesLayoutSupported (busLayout (CS::mono(), CS::stereo())), "(1,2)");
        refused.check (! p->checkBusesLayoutSupported (busLayout (CS::disabled(), CS::stereo())), "(0,2)");
        refused.check (! p->checkBusesLayoutSupported (busLayout (CS::stereo(), CS::disabled())), "(2,0)");
        refused.check (! p->checkBusesLayoutSupported (busLayout (CS::create5point1(), CS::create5point1())), "5.1 -> 5.1");

        row (accepted, "B1 accepts (1,1) and (2,2)");
        row (refused.bad == 0, "B1 refuses (2,1), (1,2), (0,2), (2,0), 5.1 -> 5.1", refused.detail());
        row (p->getMainBusNumInputChannels() == 2 && p->getMainBusNumOutputChannels() == 2,
             "B1 default layout is (2,2)");
    }

    //==========================================================================
    // P. pass-through
    //==========================================================================
    constexpr int kRenderLength = 9001;   // not a multiple of any block size used
    constexpr double kRenderRate = 48000.0;

    juce::AudioBuffer<float> makeNoise (int numChannels, juce::int64 seed)
    {
        juce::AudioBuffer<float> buffer (numChannels, kRenderLength);
        juce::Random random (seed);

        for (int channel = 0; channel < numChannels; ++channel)
            for (int i = 0; i < kRenderLength; ++i)
                buffer.setSample (channel, i, (random.nextFloat() * 2.0f - 1.0f) * 0.5f);

        return buffer;
    }

    // The P1 comparator: channel count, length and every byte.
    bool sameAudio (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return false;

        const auto bytes = sizeof (float) * (size_t) a.getNumSamples();

        for (int channel = 0; channel < a.getNumChannels(); ++channel)
            if (std::memcmp (a.getReadPointer (channel), b.getReadPointer (channel), bytes) != 0)
                return false;

        return true;
    }

    // What the per-block hook of render() sees, and may change.
    struct BlockInfo
    {
        int index = 0;              // block number, from 0
        int position = 0;           // index of the block's first sample
        bool viaBypassed = false;   // entry point for this block; starts at render()'s argument
    };

    using BlockHook = std::function<void (BlockInfo&)>;

    /** The four trailing arguments are Phase 2.2 additions. Their defaults are
        the Stage 1 behaviour, bit for bit: 48 kHz, no hook, every block
        `blockSize` long, prepared at `blockSize`.

        sampleRate         the rate handed to prepareToPlay
        beforeBlock        called before each block is filled and processed
                           (mid-render parameter edits; it may switch the
                           block's entry point)
        blockSequence      when not empty: the block lengths, cycled, each
                           limited to 1 ... blockSize
        preparedBlockSize  when > 0: the size handed to prepareToPlay instead
                           of blockSize (a host block larger than prepared) */
    juce::AudioBuffer<float> render (juce::AudioProcessor& p, const juce::AudioBuffer<float>& input,
                                     int blockSize, bool viaBypassed,
                                     double sampleRate = kRenderRate,
                                     const BlockHook& beforeBlock = {},
                                     const std::vector<int>& blockSequence = {},
                                     int preparedBlockSize = 0)
    {
        const int numChannels = input.getNumChannels();
        const int length = input.getNumSamples();

        juce::AudioBuffer<float> output (numChannels, length);
        output.clear();
        juce::AudioBuffer<float> block (numChannels, blockSize);
        juce::MidiBuffer midi;

        p.prepareToPlay (sampleRate, preparedBlockSize > 0 ? preparedBlockSize : blockSize);

        int index = 0;

        for (int position = 0; position < length; ++index)
        {
            const int wanted = blockSequence.empty()
                                   ? blockSize
                                   : juce::jlimit (1, blockSize, blockSequence[(size_t) index % blockSequence.size()]);
            const int n = juce::jmin (wanted, length - position);

            BlockInfo current;
            current.index = index;
            current.position = position;
            current.viaBypassed = viaBypassed;

            if (beforeBlock)
                beforeBlock (current);

            block.setSize (numChannels, n, false, false, true);

            for (int channel = 0; channel < numChannels; ++channel)
                block.copyFrom (channel, 0, input, channel, position, n);

            if (current.viaBypassed)
                p.processBlockBypassed (block, midi);
            else
                p.processBlock (block, midi);

            for (int channel = 0; channel < numChannels; ++channel)
                output.copyFrom (channel, position, block, channel, 0, n);

            position += n;
        }

        p.releaseResources();
        return output;
    }

    void passThroughRows()
    {
        section ("P. pass-through");
        using CS = juce::AudioChannelSet;

        struct Layout { const char* name; int channels; };
        const Layout layouts[] = { { "mono", 1 }, { "stereo", 2 } };
        const int blockSizes[] = { 1, 64, 512, 4096 };
        const char* const variants[] = { "bypass Off", "bypass On", "all random" };

        // Amended in Phase 2.2 (PLAN-2.2 P10): through processBlock the engine
        // acts on an enabled band and on the bypass switch, so "random settings
        // pass through" is no longer a property of the plugin. The variant keeps
        // the full draw (output_gain and auto_gain included) and then switches
        // every bNN_enabled off and bypass off. The stimulus guard is unchanged.
        // The processBlockBypassed rows keep the full draw.
        //
        // Extended in Phase 2.5 (PLAN-2.5 P12; execute-session call, Taylor):
        // output_gain and auto_gain now act on the audio (the output stage),
        // so after the draw the processBlock variant also sets output_gain to
        // exactly 0 dB (a raw-value store) and auto_gain off. The stimulus
        // guard is unchanged; the processBlockBypassed rows keep the full draw.
        const char* const amendedRandom = "all random, every bNN_enabled off, bypass off, output_gain 0 dB, auto_gain off";

        for (const auto& layout : layouts)
        {
            const auto input = makeNoise (layout.channels, 0x0E0A11CE + layout.channels);
            const auto set = layout.channels == 1 ? CS::mono() : CS::stereo();

            for (const bool viaBypassed : { false, true })
            {
                Tally tally;

                for (int variant = 0; variant < 3; ++variant)
                    for (const int blockSize : blockSizes)
                    {
                        auto p = make();
                        const bool layoutOk = p->setBusesLayout (busLayout (set, set));

                        if (variant == 1)
                        {
                            setNormalised (*p, "bypass", 1.0f);
                        }
                        else if (variant == 2)
                        {
                            setAllRandom (*p, 0x5EED0000 + blockSize);

                            if (! viaBypassed)
                            {
                                disableEveryBand (*p);
                                setNormalised (*p, "bypass", 0.0f);
                                p->parameters.getRawParameterValue ("output_gain")->store (0.0f);   // PLAN-2.5 P12
                                setNormalised (*p, "auto_gain", 0.0f);                               // PLAN-2.5 P12
                            }
                        }

                        const bool stimulusOk = variant == 0 ? countNonDefault (*p) == 0
                                              : variant == 1 ? findParam (*p, "bypass")->getValue() == 1.0f
                                                             : countNonDefault (*p) > kMinNonDefault
                                                                   && (viaBypassed || (sameBits (rawOf (*p, "output_gain"), 0.0f)
                                                                                       && rawOf (*p, "auto_gain") < 0.5f));

                        const auto output = render (*p, input, blockSize, viaBypassed);

                        tally.check (layoutOk && stimulusOk && sameAudio (output, input),
                                     juce::String (variant == 2 && ! viaBypassed ? amendedRandom : variants[variant])
                                         + " block " + juce::String (blockSize)
                                         + (layoutOk ? "" : " (layout refused)")
                                         + (stimulusOk ? "" : " (stimulus not applied)"));
                    }

                row (tally.bad == 0 && tally.total == 12,
                     juce::String ("P1 ") + layout.name + (viaBypassed ? " processBlockBypassed" : " processBlock")
                         + (viaBypassed ? ": out memcmp-equal to in (blocks 1/64/512/4096 x Off/On/random)"
                                        : ": out memcmp-equal to in (blocks 1/64/512/4096 x Off/On/all random, every bNN_enabled off, bypass off, output_gain 0 dB, auto_gain off)"),
                     tally.detail());
            }
        }

        // P1n: the comparator and the stimulus are not vacuous.
        {
            const auto input = makeNoise (2, 0x0E0A11CE + 2);

            auto flipped = input;
            std::uint32_t bits = 0;
            float* sample = flipped.getWritePointer (1) + 4321;
            std::memcpy (&bits, sample, sizeof (bits));
            bits ^= 1u;                                     // one least-significant bit
            std::memcpy (sample, &bits, sizeof (bits));

            juce::AudioBuffer<float> shorter (2, kRenderLength - 1);
            shorter.clear();

            row (sameAudio (input, input) && ! sameAudio (flipped, input) && ! sameAudio (shorter, input),
                 "P1n comparator FAILS on one flipped bit (and on a length mismatch)");

            const double rmsDb = (double) juce::Decibels::gainToDecibels (input.getRMSLevel (0, 0, kRenderLength), -200.0f);
            const double peak = (double) input.getMagnitude (0, kRenderLength);
            row (rmsDb > -20.0 && peak <= 0.5 && peak > 0.45, "P1n input RMS > -20 dBFS, peak 0.5",
                 fmtJ (rmsDb) + " dBFS, peak " + fmtJ (peak));
        }
    }

    //==========================================================================
    // S. state codec
    //==========================================================================
    // Bit-exact comparison of all 196 normalised values. Prints EVERY mismatch
    // (plan P13 wants the IDs and the deltas, not a sample of them).
    bool sameValuesExact (juce::AudioProcessor& a, juce::AudioProcessor& b, const char* rowId, juce::String& detail)
    {
        const auto pa = rangedParams (a);
        const auto pb = rangedParams (b);

        if (pa.size() != pb.size())
        {
            detail = "parameter counts differ";
            return false;
        }

        int bad = 0;

        for (size_t i = 0; i < pa.size(); ++i)
        {
            const float va = pa[i]->getValue();
            const float vb = pb[i]->getValue();

            if (! sameBits (va, vb))
            {
                ++bad;
                std::printf ("      %s mismatch %-16s expected %s  got %s  delta %s\n", rowId,
                             pa[i]->getParameterID().toRawUTF8(), fmt ((double) va).c_str(), fmt ((double) vb).c_str(),
                             fmt ((double) vb - (double) va).c_str());
            }
        }

        detail = juce::String ((int) pa.size() - bad) + "/" + juce::String ((int) pa.size()) + " bit-equal";
        return bad == 0;
    }

    // "Known values applied": S4 and S6 are about their own subject, so they
    // use a 1e-6 window on the normalised value. Exactness is S1's job alone.
    bool sameValuesClose (juce::AudioProcessor& a, juce::AudioProcessor& b, juce::String& detail)
    {
        const auto pa = rangedParams (a);
        const auto pb = rangedParams (b);
        Tally tally;

        for (size_t i = 0; i < pa.size() && i < pb.size(); ++i)
            tally.check (std::abs (pa[i]->getValue() - pb[i]->getValue()) <= 1.0e-6f, pa[i]->getParameterID());

        detail = tally.detail();
        return pa.size() == pb.size() && tally.bad == 0 && tally.total == kExpectedParams;
    }

    bool sameValuesAsSnapshot (juce::AudioProcessor& p, const std::vector<float>& snapshot)
    {
        const auto now = valuesOf (p);

        if (now.size() != snapshot.size())
            return false;

        for (size_t i = 0; i < now.size(); ++i)
            if (! sameBits (now[i], snapshot[i]))
                return false;

        return true;
    }

    void stateRows()
    {
        section ("S. state codec");

        // S1 + S2 share one saved state.
        {
            auto a = make();
            setAllRandom (*a, 0x51A7E001);
            const auto uiA = makeUi (2, 3, 30, 17, 1234, 777);
            setUi (*a, uiA);
            const int nonDefault = countNonDefault (*a);

            const auto blobA = stateOf (*a);

            auto b = make();
            loadState (*b, blobA);

            juce::String detail;
            const bool exact = sameValuesExact (*a, *b, "S1", detail);
            row (exact && nonDefault > kMinNonDefault,"S1 round trip: 196 getValue() bit-equal in a fresh instance",
                 detail + "  (" + juce::String (nonDefault) + " non-default in the source)");

            const auto uiB = uiOf (*b);
            row (uiB == uiA, "S1 UI members restored (mode 3, range 30, band 17, 1234 x 777, zh-Hans)", uiB.describe());

            const auto blobB = stateOf (*b);
            const bool sameBlob = blobA == blobB;

            if (! sameBlob)
            {
                auto xa = xmlOf (blobA);
                auto xb = xmlOf (blobB);

                if (xa != nullptr && xb != nullptr)
                    for (auto* child : xa->getChildIterator())
                        if (child->hasTagName ("PARAM"))
                        {
                            const auto id = child->getStringAttribute ("id");
                            auto* other = paramChild (*xb, id);
                            const auto va = child->getStringAttribute ("value");
                            const auto vb = other != nullptr ? other->getStringAttribute ("value") : juce::String ("<missing>");

                            if (va != vb)
                                std::printf ("      S1 re-save differs %-16s A %s  A->B %s\n",
                                             id.toRawUTF8(), va.toRawUTF8(), vb.toRawUTF8());
                        }
            }

            row (sameBlob, "S1 'A -> save' and 'A -> B -> save' are byte-equal",
                 juce::String ((int) blobA.getSize()) + " / " + juce::String ((int) blobB.getSize()) + " bytes");

            // S2
            const auto xml = xmlOf (blobA);
            const bool parsed = xml != nullptr;
            row (parsed && xml->hasTagName ("OParametricEQ"), "S2 root tag OParametricEQ",
                 parsed ? xml->getTagName() : juce::String ("no XML"));

            if (parsed)
            {
                row (xml->getStringAttribute ("stateVersion") == "1"
                         && xml->getStringAttribute ("pluginVersion").isNotEmpty()
                         && xml->getStringAttribute ("uiLanguage") == "zh-Hans",
                     "S2 stateVersion=\"1\", pluginVersion, uiLanguage=\"zh-Hans\"",
                     "stateVersion " + xml->getStringAttribute ("stateVersion")
                         + ", pluginVersion " + xml->getStringAttribute ("pluginVersion")
                         + ", uiLanguage " + xml->getStringAttribute ("uiLanguage"));

                const int numParam = countChildren (*xml, "PARAM");
                const int numUi = countChildren (*xml, "UI");
                auto* ui = xml->getChildByName ("UI");

                row (numParam == kExpectedParams && numUi == 1 && xml->getNumChildElements() == kExpectedParams + 1,
                     "S2 exactly 196 PARAM and exactly one UI child",
                     juce::String (numParam) + " PARAM, " + juce::String (numUi) + " UI, "
                         + juce::String (xml->getNumChildElements()) + " children");

                row (ui != nullptr && ! ui->hasAttribute ("id")
                         && ui->getIntAttribute ("analyzerMode", -1) == 3
                         && ui->getIntAttribute ("displayRange", -1) == 30
                         && ui->getIntAttribute ("selectedBand", -1) == 17
                         && ui->getIntAttribute ("editorWidth", -1) == 1234
                         && ui->getIntAttribute ("editorHeight", -1) == 777,
                     "S2 UI child: no 'id' attribute; five values present");
            }
            else
            {
                row (false, "S2 attributes / children (no XML to inspect)");
            }
        }

        // S3: two reopens with a UI change between them.
        {
            auto a = make();
            setUi (*a, makeUi (1, 1, 6, 5, 1000, 700));

            auto b = make();
            loadState (*b, stateOf (*a));                    // reopen 1
            const bool first = uiOf (*b) == makeUi (1, 1, 6, 5, 1000, 700);

            const auto changed = makeUi (0, 3, 30, 9, 1500, 900);
            setUi (*b, changed);                             // the change
            const auto blob2 = stateOf (*b);

            auto c = make();
            loadState (*c, blob2);                           // reopen 2, fresh instance
            const bool second = uiOf (*c) == changed;

            setUi (*b, makeUi (2, 0, 12, 1, 801, 501));
            loadState (*b, blob2);                           // reopen 2, same instance
            const bool secondSame = uiOf (*b) == changed;

            const auto xmlB = xmlOf (blob2);
            const auto xmlC = xmlOf (stateOf (*c));
            const auto xmlB2 = xmlOf (stateOf (*b));
            const bool oneUi = xmlB != nullptr && xmlC != nullptr && xmlB2 != nullptr
                            && countChildren (*xmlB, "UI") == 1 && countChildren (*xmlC, "UI") == 1
                            && countChildren (*xmlB2, "UI") == 1
                            && countChildren (*xmlC, "PARAM") == kExpectedParams;

            row (first && second && secondSame, "S3 second reopen shows the changed UI values",
                 "fresh: " + uiOf (*c).describe());
            row (oneUi, "S3 still exactly one UI child after each reopen");
        }

        // S4: unknown parameter.
        {
            auto a = make();
            setAllRandom (*a, 0x51A7E004);
            auto xml = xmlOf (stateOf (*a));
            bool ok = xml != nullptr;
            juce::String detail = "no XML";

            if (ok)
            {
                auto* extra = xml->createNewChildElement ("PARAM");
                extra->setAttribute ("id", "b01_dyn_range");
                extra->setAttribute ("value", "6");

                auto b = make();
                loadState (*b, blobOf (*xml));

                const bool applied = sameValuesClose (*a, *b, detail);
                const auto saved = xmlOf (stateOf (*b));
                auto* kept = saved != nullptr ? paramChild (*saved, "b01_dyn_range") : nullptr;

                ok = applied && countNonDefault (*b) > kMinNonDefault
                  && b->parameters.getParameter ("b01_dyn_range") == nullptr
                  && kept != nullptr && kept->getDoubleAttribute ("value", -1.0) == 6.0
                  && countChildren (*saved, "PARAM") == kExpectedParams + 1
                  && countChildren (*saved, "UI") == 1;

                detail += kept != nullptr ? "  unknown child kept" : "  unknown child LOST";
            }

            row (ok, "S4 unknown parameter: loads, known values applied, child present in the next save", detail);
        }

        // S5: missing parameter.
        {
            auto source = make();
            setRaw (*source, "b07_q", 4.0f);                 // a known value that must still arrive
            setRaw (*source, "b03_gain", -7.5f);
            auto xml = xmlOf (stateOf (*source));
            int removed = 0;

            if (xml != nullptr)
                for (const char* id : { "b07_gain", "b07_freq" })
                    if (auto* child = paramChild (*xml, id))
                    {
                        xml->removeChildElement (child, true);
                        ++removed;
                    }

            auto target = make();
            setRaw (*target, "b07_gain", 12.0f);
            setRaw (*target, "b07_freq", 5000.0f);

            auto* gain = findParam (*target, "b07_gain");
            auto* freq = findParam (*target, "b07_freq");
            auto* q    = findParam (*target, "b07_q");

            // Guard: without the pre-set, "equals the default afterwards" proves nothing.
            const bool preset = gain != nullptr && freq != nullptr && q != nullptr
                             && ! sameBits (gain->getValue(), gain->getDefaultValue())
                             && ! sameBits (freq->getValue(), freq->getDefaultValue())
                             && std::abs (rawOf (*target, "b07_gain") - 12.0f) < 1.0e-3f
                             && std::abs (rawOf (*target, "b07_freq") - 5000.0f) < 1.0f;

            bool ok = false;
            juce::String detail = "blob / pre-set guard failed (removed " + juce::String (removed) + ")";

            if (xml != nullptr && removed == 2 && preset)
            {
                loadState (*target, blobOf (*xml));

                auto fresh = make();
                const float gainRaw = rawOf (*target, "b07_gain");
                const float freqRaw = rawOf (*target, "b07_freq");
                const float freshFreqRaw = rawOf (*fresh, "b07_freq");
                const bool freqBitEqual = sameBits (freqRaw, freshFreqRaw);

                const bool gainOk = sameBits (gainRaw, 0.0f) && sameBits (gain->getValue(), 0.5f);
                const bool freqOk = std::abs ((double) freqRaw - (double) freshFreqRaw) / (double) freshFreqRaw <= 1.0e-6;
                const bool knownOk = std::abs (rawOf (*target, "b07_q") - 4.0f) < 1.0e-3f
                                  && std::abs (rawOf (*target, "b03_gain") + 7.5f) < 1.0e-3f;

                ok = gainOk && freqOk && knownOk;
                detail = "b07_gain raw " + fmtJ ((double) gainRaw) + " norm " + fmtJ ((double) gain->getValue())
                       + "; b07_freq raw " + fmtJ ((double) freqRaw) + " vs fresh " + fmtJ ((double) freshFreqRaw)
                       + (freqBitEqual ? " (bit-equal)" : " (NOT bit-equal)")
                       + " norm " + fmtJ ((double) freq->getValue())
                       + (knownOk ? "" : "; known values NOT applied");
            }

            row (ok, "S5 missing parameter loads at its default (pre-set non-default first)", detail);
        }

        // S6: a later state version.
        {
            auto a = make();
            setAllRandom (*a, 0x51A7E006);
            auto xml = xmlOf (stateOf (*a));
            bool ok = xml != nullptr;
            juce::String detail = "no XML";

            if (ok)
            {
                xml->setAttribute ("stateVersion", "2");

                auto b = make();
                loadState (*b, blobOf (*xml));

                const bool applied = sameValuesClose (*a, *b, detail);
                const auto saved = xmlOf (stateOf (*b));
                const auto version = saved != nullptr ? saved->getStringAttribute ("stateVersion") : juce::String ("no XML");

                ok = applied && countNonDefault (*b) > kMinNonDefault && version == "1";
                detail += "  next save stateVersion " + version;
            }

            row (ok, "S6 stateVersion=\"2\": loads, values applied, next save writes \"1\"", detail);
        }

        // S7: blobs that must change nothing.
        {
            auto source = make();
            setAllRandom (*source, 0x51A7E0A7);              // different from the target on purpose
            setUi (*source, makeUi (2, 3, 30, 24, 1600, 1000));
            const auto valid = stateOf (*source);

            auto target = make();
            setAllRandom (*target, 0x51A7E007);
            const auto uiBefore = makeUi (1, 1, 6, 11, 900, 550);
            setUi (*target, uiBefore);
            const auto snapshot = valuesOf (*target);
            const bool startsNonDefault = countNonDefault (*target) > kMinNonDefault &&! sameValuesAsSnapshot (*source, snapshot);

            Tally tally;

            // Wrong root tag (otherwise a complete, valid state).
            if (auto xml = xmlOf (valid))
            {
                xml->setTagName ("NotOParametricEQ");
                loadState (*target, blobOf (*xml));
                tally.check (sameValuesAsSnapshot (*target, snapshot) && uiOf (*target) == uiBefore, "wrong root tag");
            }
            else
            {
                tally.check (false, "no XML for the wrong-root case");
            }

            // Truncated blob (half of a valid one).
            target->setStateInformation (valid.getData(), (int) (valid.getSize() / 2));
            tally.check (sameValuesAsSnapshot (*target, snapshot) && uiOf (*target) == uiBefore, "truncated");

            // Truncated inside the 8-byte header.
            target->setStateInformation (valid.getData(), 5);
            tally.check (sameValuesAsSnapshot (*target, snapshot) && uiOf (*target) == uiBefore, "5-byte blob");

            // Empty blob.
            const char nothing = 0;
            target->setStateInformation (&nothing, 0);
            tally.check (sameValuesAsSnapshot (*target, snapshot) && uiOf (*target) == uiBefore, "empty");

            // Control: the valid blob DOES change the target (the checks above are not vacuous).
            loadState (*target, valid);
            const bool control = ! sameValuesAsSnapshot (*target, snapshot) && ! (uiOf (*target) == uiBefore);

            row (tally.bad == 0 && startsNonDefault && control,
                 "S7 wrong root / truncated / empty blob: 196 values, UI and language unchanged",
                 tally.detail() + (control ? "" : "  (control: the valid blob changed nothing)")
                     + (startsNonDefault ? "" : "  (starting state not distinct)"));
        }

        // S8: UI validation.
        {
            auto source = make();
            auto xml = xmlOf (stateOf (*source));
            auto* ui = xml != nullptr ? xml->getChildByName ("UI") : nullptr;
            bool invalidOk = false, absentOk = false;
            juce::String detail = "no XML / UI child";

            if (ui != nullptr)
            {
                ui->setAttribute ("analyzerMode", "9");
                ui->setAttribute ("displayRange", "7");
                ui->setAttribute ("selectedBand", "99");
                ui->setAttribute ("editorWidth", "99999");
                ui->setAttribute ("editorHeight", "1");
                xml->setAttribute ("uiLanguage", "xx");

                auto target = make();
                setUi (*target, makeUi (1, 1, 6, 3, 900, 550));
                loadState (*target, blobOf (*xml));

                // Invalid values leave their members alone; the size clamps;
                // an unknown language code is English.
                const auto after = uiOf (*target);
                invalidOk = after == makeUi (0, 1, 6, 3, 1600, 500);
                detail = "invalid: " + after.describe();

                // No UI child and no uiLanguage attribute: nothing moves.
                xml->removeChildElement (ui, true);
                xml->removeAttribute ("uiLanguage");

                auto target2 = make();
                const auto kept = makeUi (2, 3, 30, 21, 1111, 888);
                setUi (*target2, kept);
                loadState (*target2, blobOf (*xml));

                absentOk = uiOf (*target2) == kept && countChildren (*xml, "UI") == 0;
                detail += "; absent: " + uiOf (*target2).describe();
            }

            row (invalidOk, "S8 invalid mode / range / band ignored; width clamps to 1600; 'xx' -> en", detail);
            row (absentOk, "S8 no UI child and no uiLanguage: every UI member unchanged");
        }
    }

    //==========================================================================
    //==========================================================================
    // Stage 2, Phase 2.2: the engine through the plugin (PLAN-2.2 T7)
    //==========================================================================
    //==========================================================================

    // Printed, never counted: the last line stays "N PASS, M FAIL".
    void info (const juce::String& text)
    {
        std::printf ("  [INFO] %s\n", text.toRawUTF8());
        std::fflush (stdout);
    }

    juce::String fmtG (double value, int digits = 3)
    {
        char text[64];
        std::snprintf (text, sizeof (text), "%.*g", digits, value);
        return juce::String (text);
    }

    // Choice indices (parameter-spec; eq::Type and eq::Placement carry the same values).
    constexpr int kBell = 0, kLowShelf = 1, kHighShelf = 2, kLowCut = 3, kHighCut = 4,
                  kNotch = 5, kBandPass = 6, kTilt = 7;
    constexpr int kStereo = 0, kLeft = 1, kRight = 2, kMid = 3, kSide = 4;

    const char* const kTypeNames[8] = { "Bell", "Low Shelf", "High Shelf", "Low Cut",
                                        "High Cut", "Notch", "Band Pass", "Tilt" };
    const char* const kPlacementNames[5] = { "Stereo", "Left", "Right", "Mid", "Side" };

    constexpr float kNoise12 = 0.25f;       // white noise peak, -12 dBFS
    constexpr int kNoiseLength = 16384;     // the noise part of "noise+tail"
    constexpr int kTailLength = 2048;       // the denormal part
    constexpr int kSequenceMax = 1024;      // largest block of the random block sequence

    //==========================================================================
    // Band settings
    //==========================================================================
    struct BandSpec
    {
        int type = 0;
        float freq = 1000.0f;
        float gain = 0.0f;
        float q = 1.0f;
        int slope = 1;
        int placement = 0;
    };

    BandSpec makeSpec (int type, float freq, float gain, float q, int slope, int placement)
    {
        BandSpec s;
        s.type = type;
        s.freq = freq;
        s.gain = gain;
        s.q = q;
        s.slope = slope;
        s.placement = placement;
        return s;
    }

    // Writes the band's six settings, clears its bypass and enables it.
    void setBand (juce::AudioProcessor& p, int band, const BandSpec& s)
    {
        setRaw (p, bandId (band, "type"), (float) s.type);
        setRaw (p, bandId (band, "freq"), s.freq);
        setRaw (p, bandId (band, "gain"), s.gain);
        setRaw (p, bandId (band, "q"), s.q);
        setRaw (p, bandId (band, "slope"), (float) s.slope);
        setRaw (p, bandId (band, "placement"), (float) s.placement);
        setRaw (p, bandId (band, "bypass"), 0.0f);
        setRaw (p, bandId (band, "enabled"), 1.0f);
    }

    bool setLayout (juce::AudioProcessor& p, int channels)
    {
        const auto set = channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
        return p.setBusesLayout (busLayout (set, set));
    }

    // A fresh instance on a mono or stereo layout with bands 1 ... numBands set
    // from specOf (0 ... numBands - 1).
    template <typename SpecFn>
    std::unique_ptr<Processor> makeBands (int channels, int numBands, SpecFn specOf)
    {
        auto p = make();

        if (! setLayout (*p, channels))
            row (false, "harness: layout refused (" + juce::String (channels) + " channels)");

        for (int index = 0; index < numBands; ++index)
            setBand (*p, index + 1, specOf (index));

        return p;
    }

    // The 24-band mixed setting of T1g, K1, H1 and V1: every type, every slope,
    // every placement, gains +-6 dB (PLAN-2.2 P12: bounded). Cuts sit at the
    // band edges and the band passes are wide, so the output keeps its level.
    BandSpec mixedSpec (int index)
    {
        static const float qs[4] = { 0.7f, 1.0f, 1.4f, 2.0f };

        BandSpec s;
        s.type = index % 8;
        s.slope = (index * 3 + 1) % 8;
        s.placement = index % 5;
        s.freq = 40.0f * std::pow (400.0f, (float) ((index * 7) % 24) / 23.0f);
        s.gain = ((index / 2 + index / 8) % 2 == 0) ? 6.0f : -6.0f;
        s.q = qs[index % 4];

        if (s.type == kLowCut)
            s.freq = 30.0f + 15.0f * (float) (index / 8);

        if (s.type == kHighCut)
            s.freq = 12000.0f + 3000.0f * (float) (index / 8);

        if (s.type == kBandPass)
            s.q = 0.25f;

        return s;
    }

    // The six bands of T1b / T1bn, on one placement.
    BandSpec channelBandSpec (int index, int placement)
    {
        const BandSpec table[6] =
        {
            makeSpec (kBell,      500.0f,   6.0f, 1.5f, 1, kStereo),
            makeSpec (kLowShelf,  150.0f,   4.0f, 1.0f, 3, kStereo),
            makeSpec (kHighShelf, 6000.0f, -5.0f, 1.0f, 1, kStereo),
            makeSpec (kLowCut,    60.0f,    0.0f, 1.0f, 5, kStereo),
            makeSpec (kNotch,     2000.0f,  0.0f, 4.0f, 1, kStereo),
            makeSpec (kTilt,      1000.0f,  3.0f, 1.0f, 1, kStereo)
        };

        BandSpec s = table[index % 6];
        s.placement = placement;
        return s;
    }

    // The bells of T1c / T1cn: Q 1, 40 Hz - 16 kHz log-spaced (one band: 800 Hz).
    BandSpec msBellSpec (int index, int numBands, float gainDb, int placement)
    {
        const float freq = numBands > 1 ? 40.0f * std::pow (400.0f, (float) index / (float) (numBands - 1)) : 800.0f;
        return makeSpec (kBell, freq, gainDb, 1.0f, 1, placement);
    }

    //==========================================================================
    // References: raw values -> makeBandInput -> Designer::design
    //==========================================================================
    eq::RawBand rawBandOf (Processor& p, int band)
    {
        eq::RawBand raw;
        raw.enabled   = rawOf (p, bandId (band, "enabled"));
        raw.bypass    = rawOf (p, bandId (band, "bypass"));
        raw.type      = rawOf (p, bandId (band, "type"));
        raw.freq      = rawOf (p, bandId (band, "freq"));
        raw.gain      = rawOf (p, bandId (band, "gain"));
        raw.q         = rawOf (p, bandId (band, "q"));
        raw.slope     = rawOf (p, bandId (band, "slope"));
        raw.placement = rawOf (p, bandId (band, "placement"));
        return raw;
    }

    // band is 1 ... 24. freqFactor scales the raw frequency (E2n only).
    // getBandDynOffsetDb is read with a 0-based index.
    eq::BandInput inputOf (Processor& p, int band, float freqFactor = 1.0f)
    {
        eq::RawBand raw = rawBandOf (p, band);
        raw.freq *= freqFactor;
        return eq::makeBandInput (raw, rawOf (p, "gain_scale"), p.getBandDynOffsetDb (band - 1));
    }

    int activeBands (Processor& p)
    {
        int n = 0;

        for (int band = 1; band <= Processor::kNumBands; ++band)
            if (inputOf (p, band).active)
                ++n;

        return n;
    }

    int enabledBands (Processor& p)
    {
        int n = 0;

        for (int band = 1; band <= Processor::kNumBands; ++band)
            if (rawOf (p, bandId (band, "enabled")) >= 0.5f)
                ++n;

        return n;
    }

    // Designer::prepare before the first design at every rate.
    std::unique_ptr<eq::Designer> designerFor (double rate)
    {
        auto designer = std::make_unique<eq::Designer>();
        designer->prepare (rate);
        return designer;
    }

    // PLAN-2.2 P11: 160 log-spaced points, 20 Hz to min (20 kHz, 0.499 fs).
    std::vector<double> responseGrid (double rate)
    {
        constexpr int numPoints = 160;
        const double top = juce::jmin (20000.0, 0.499 * rate);
        std::vector<double> hz;

        for (int j = 0; j < numPoints; ++j)
            hz.push_back (20.0 * std::pow (top / 20.0, (double) j / (double) (numPoints - 1)));

        return hz;
    }

    // Sum in dB of the closed forms of the active bands (all on one lane: the
    // rows that use this keep every band on Stereo). excludeBand (1 ... 24)
    // leaves one band out; 0 leaves none out.
    std::vector<double> chainRefDb (Processor& p, const eq::Designer& designer, double rate,
                                    const std::vector<double>& hz, int excludeBand = 0, float freqFactor = 1.0f)
    {
        std::vector<double> db (hz.size(), 0.0);

        for (int band = 1; band <= Processor::kNumBands; ++band)
        {
            if (band == excludeBand)
                continue;

            const auto in = inputOf (p, band, freqFactor);

            if (! in.active)
                continue;

            eq::BandDesign design {};
            designer.design (in.settings, design);

            for (size_t j = 0; j < hz.size(); ++j)
                db[j] += eq::bandMagDb (design, 2.0 * eq::pi * hz[j] / rate);
        }

        return db;
    }

    //==========================================================================
    // Impulse response and single-bin DFT (ported from research-2.2 engine_bench modeIr)
    //==========================================================================
    struct Impulse
    {
        std::vector<std::vector<float>> channels;
        bool tailReached = false;
        bool finite = true;
        double seconds = 0.0;

        bool ok() const     { return tailReached && finite; }
    };

    /** PLAN-2.2 P11: an impulse on every channel (amplitudes[channel]) at
        sample 0, rendered in 1 s steps until the last 10 ms is at least
        tailDb below the peak. Cap 20 s: tailReached stays false there, and
        the calling row fails. */
    Impulse renderImpulse (juce::AudioProcessor& p, int numChannels, double rate,
                           const float* amplitudes, double tailDb = 140.0)
    {
        constexpr int blockSize = 512;
        constexpr int capSeconds = 20;
        const int step = (int) std::lround (rate);
        const int tailLength = (int) std::lround (rate * 0.010);
        const double tailRatio = std::pow (10.0, -tailDb / 20.0);

        Impulse result;
        result.channels.resize ((size_t) numChannels);

        juce::AudioBuffer<float> block (numChannels, blockSize);
        juce::MidiBuffer midi;

        p.prepareToPlay (rate, blockSize);

        int position = 0;

        for (int second = 1; second <= capSeconds; ++second)
        {
            const int end = second * step;

            while (position < end)
            {
                const int n = juce::jmin (blockSize, end - position);
                block.setSize (numChannels, n, false, false, true);
                block.clear();

                if (position == 0)
                    for (int channel = 0; channel < numChannels; ++channel)
                        block.setSample (channel, 0, amplitudes[channel]);

                p.processBlock (block, midi);

                for (int channel = 0; channel < numChannels; ++channel)
                {
                    const float* data = block.getReadPointer (channel);
                    auto& store = result.channels[(size_t) channel];
                    store.insert (store.end(), data, data + n);
                }

                position += n;
            }

            double peak = 0.0, tail = 0.0;

            for (const auto& channel : result.channels)
            {
                const int total = (int) channel.size();

                for (int i = 0; i < total; ++i)
                {
                    const double v = std::abs ((double) channel[(size_t) i]);

                    if (! std::isfinite (v))
                        result.finite = false;

                    peak = juce::jmax (peak, v);

                    if (i >= total - tailLength)
                        tail = juce::jmax (tail, v);
                }
            }

            result.seconds = (double) position / rate;

            if (! result.finite)
                break;

            if (peak > 0.0 && tail <= peak * tailRatio)
            {
                result.tailReached = true;
                break;
            }
        }

        p.releaseResources();
        return result;
    }

    juce::String describe (const Impulse& ir)
    {
        return fmtG (ir.seconds) + " s rendered" + (ir.tailReached ? "" : ", TAIL NOT REACHED")
             + (ir.finite ? "" : ", NOT FINITE");
    }

    bool sameImpulse (const Impulse& a, const Impulse& b)
    {
        if (a.channels.size() != b.channels.size())
            return false;

        for (size_t channel = 0; channel < a.channels.size(); ++channel)
        {
            const auto& x = a.channels[channel];
            const auto& y = b.channels[channel];

            if (x.size() != y.size() || std::memcmp (x.data(), y.data(), sizeof (float) * x.size()) != 0)
                return false;
        }

        return true;
    }

    // Magnitude in dB at each frequency: a running phasor, renormalised every
    // 4096 samples. `amplitude` is the impulse's.
    std::vector<double> dftDb (const std::vector<float>& h, double amplitude, double rate, const std::vector<double>& hz)
    {
        std::vector<double> db;

        for (const double f : hz)
        {
            const double w = 2.0 * eq::pi * f / rate;
            const double cr = std::cos (w), ci = -std::sin (w);
            double pr = 1.0, pim = 0.0, ar = 0.0, ai = 0.0;

            for (size_t n = 0; n < h.size(); ++n)
            {
                const double v = (double) h[n];
                ar += v * pr;
                ai += v * pim;

                const double t = pr * cr - pim * ci;
                pim = pr * ci + pim * cr;
                pr = t;

                if ((n & 4095u) == 4095u)
                {
                    const double m = std::sqrt (pr * pr + pim * pim);
                    pr /= m;
                    pim /= m;
                }
            }

            db.push_back (eq::powerToDb ((ar * ar + ai * ai) / (amplitude * amplitude)));
        }

        return db;
    }

    std::vector<std::vector<double>> measureImpulse (const Impulse& ir, const float* amplitudes, double rate,
                                                     const std::vector<double>& hz)
    {
        std::vector<std::vector<double>> measured;

        for (size_t channel = 0; channel < ir.channels.size(); ++channel)
            measured.push_back (dftDb (ir.channels[channel], (double) amplitudes[channel], rate, hz));

        return measured;
    }

    // Worst |measured - reference| over every channel, where the reference is
    // above -80 dB.
    struct Score
    {
        double worst = 0.0;
        int scored = 0;
        bool invalid = false;

        bool within (double limitDb) const   { return ! invalid && scored > 0 && worst <= limitDb; }

        // For the "...n" rows: a real miss, not an empty or broken comparison.
        bool misses (double limitDb) const   { return ! invalid && scored > 0 && worst > limitDb; }
    };

    Score scoreChannels (const std::vector<std::vector<double>>& measured, const std::vector<double>& reference)
    {
        Score s;

        if (measured.empty())
            s.invalid = true;

        for (const auto& channel : measured)
        {
            if (channel.size() != reference.size())
            {
                s.invalid = true;
                continue;
            }

            for (size_t j = 0; j < reference.size(); ++j)
                if (reference[j] > -80.0)
                {
                    const double d = std::abs (channel[j] - reference[j]);
                    ++s.scored;

                    if (! std::isfinite (d))
                        s.invalid = true;
                    else
                        s.worst = juce::jmax (s.worst, d);
                }
        }

        return s;
    }

    //==========================================================================
    // Stimuli and comparators
    //==========================================================================
    juce::AudioBuffer<float> makeNoiseAt (int numChannels, int length, juce::int64 seed, float amplitude)
    {
        juce::AudioBuffer<float> buffer (numChannels, length);
        juce::Random random (seed);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* data = buffer.getWritePointer (channel);

            for (int i = 0; i < length; ++i)
                data[i] = (random.nextFloat() * 2.0f - 1.0f) * amplitude;
        }

        return buffer;
    }

    /** "noise+tail": white noise at -12 dBFS, then 2048 samples with magnitudes
        in 1e-41 ... 1e-39, random sign. The tail is written by bit pattern
        (a denormal float is bits x 1.4013e-45), so it does not depend on this
        thread's floating-point mode. */
    juce::AudioBuffer<float> makeNoiseTail (int numChannels, juce::int64 seed)
    {
        constexpr int bitsMin = 7137;       // 1.0001e-41
        constexpr int bitsMax = 713600;     // 9.9997e-40

        auto buffer = makeNoiseAt (numChannels, kNoiseLength + kTailLength, seed, kNoise12);
        juce::Random random (seed + 0x7A11);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* data = buffer.getWritePointer (channel) + kNoiseLength;

            for (int i = 0; i < kTailLength; ++i)
            {
                auto bits = (std::uint32_t) (bitsMin + random.nextInt (bitsMax - bitsMin + 1));

                if (random.nextBool())
                    bits |= 0x80000000u;

                std::memcpy (data + i, &bits, sizeof (bits));
            }
        }

        return buffer;
    }

    // False when this thread flushes denormal results to zero: the stimulus
    // and the comparisons need it not to.
    bool keepsDenormals()
    {
        volatile float a = 1.0e-30f;
        volatile float b = 1.0e-10f;
        const float product = a * b;
        return product != 0.0f && std::fpclassify (product) == FP_SUBNORMAL;
    }

    // Every sample of the last 2048 on every channel is a non-zero denormal
    // float in 1e-41 ... 1e-39.
    bool hasDenormalTail (const juce::AudioBuffer<float>& buffer)
    {
        if (! keepsDenormals() || buffer.getNumSamples() < kTailLength || buffer.getNumChannels() < 1)
            return false;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const float* data = buffer.getReadPointer (channel, buffer.getNumSamples() - kTailLength);

            for (int i = 0; i < kTailLength; ++i)
            {
                const double magnitude = std::abs ((double) data[i]);

                if (std::fpclassify (data[i]) != FP_SUBNORMAL || magnitude < 1.0e-41 || magnitude > 1.0e-39)
                    return false;
            }
        }

        return true;
    }

    bool allFinite (const juce::AudioBuffer<float>& buffer)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const float* data = buffer.getReadPointer (channel);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                if (! std::isfinite (data[i]))
                    return false;
        }

        return true;
    }

    bool sameChannel (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, int channel)
    {
        return a.getNumSamples() == b.getNumSamples()
            && channel < a.getNumChannels() && channel < b.getNumChannels()
            && std::memcmp (a.getReadPointer (channel), b.getReadPointer (channel),
                            sizeof (float) * (size_t) a.getNumSamples()) == 0;
    }

    // n samples of every channel: a from startA against b from startB.
    bool sameFrom (const juce::AudioBuffer<float>& a, int startA, const juce::AudioBuffer<float>& b, int startB, int n)
    {
        if (a.getNumChannels() != b.getNumChannels() || n <= 0 || startA < 0 || startB < 0
            || startA + n > a.getNumSamples() || startB + n > b.getNumSamples())
            return false;

        for (int channel = 0; channel < a.getNumChannels(); ++channel)
            if (std::memcmp (a.getReadPointer (channel, startA), b.getReadPointer (channel, startB),
                             sizeof (float) * (size_t) n) != 0)
                return false;

        return true;
    }

    juce::AudioBuffer<float> sliceOf (const juce::AudioBuffer<float>& source, int start, int length)
    {
        juce::AudioBuffer<float> out (source.getNumChannels(), length);

        for (int channel = 0; channel < source.getNumChannels(); ++channel)
            out.copyFrom (channel, 0, source, channel, start, length);

        return out;
    }

    // Seeded block lengths in 1 ... maxSize.
    std::vector<int> randomBlockSequence (juce::int64 seed, int maxSize)
    {
        juce::Random random (seed);
        std::vector<int> sizes;

        for (int i = 0; i < 97; ++i)
            sizes.push_back (1 + random.nextInt (maxSize));

        return sizes;
    }

    // pattern > 0: that block size throughout. pattern == 0: the seeded random
    // sequence (prepared at its largest block).
    juce::AudioBuffer<float> renderPattern (juce::AudioProcessor& p, const juce::AudioBuffer<float>& input, int pattern)
    {
        if (pattern > 0)
            return render (p, input, pattern, false);

        return render (p, input, kSequenceMax, false, kRenderRate, {}, randomBlockSequence (0x5E0BE9, kSequenceMax));
    }

    juce::String patternName (int pattern)
    {
        return pattern > 0 ? "block " + juce::String (pattern) : juce::String ("random sequence");
    }

    //==========================================================================
    // E. response (FUNC-01, FUNC-02)
    //==========================================================================
    void responseRows()
    {
        section ("E. engine response through the plugin (Phase 2.2: FUNC-01, FUNC-02)");

        constexpr double limitDb = 0.01;
        const float amplitudes[2] = { 1.0f, 0.5f };     // distinct, so a channel mix-up shows

        // E1, E1n
        {
            const double rate = 48000.0;
            const auto designer = designerFor (rate);
            const auto hz = responseGrid (rate);

            auto p = makeBands (2, 24, [] (int index)
            {
                return makeSpec (kBell, 40.0f * std::pow (400.0f, (float) index / 23.0f),
                                 index % 2 == 0 ? 3.0f : -3.0f, 2.0f, 1, kStereo);
            });

            const auto full = renderImpulse (*p, 2, rate, amplitudes);
            const auto fullMeasured = measureImpulse (full, amplitudes, rate, hz);
            const auto fullScore = scoreChannels (fullMeasured, chainRefDb (*p, *designer, rate, hz));

            row (full.ok() && activeBands (*p) == 24 && fullScore.within (limitDb),
                 "E1 (i) 24 bells +-3 dB, Q 2, 48 kHz stereo: response vs the 24 closed forms <= 0.01 dB above -80 dB",
                 "worst " + fmtG (fullScore.worst) + " dB over " + juce::String (fullScore.scored) + " points; " + describe (full));

            // E1n: the comparison can see one missing band.
            Tally control;
            double smallestMiss = std::numeric_limits<double>::infinity();

            for (int band = 1; band <= Processor::kNumBands; ++band)
            {
                const auto s = scoreChannels (fullMeasured, chainRefDb (*p, *designer, rate, hz, band));
                control.check (s.misses (limitDb), "without band " + juce::String (band) + ": " + fmtG (s.worst) + " dB");
                smallestMiss = juce::jmin (smallestMiss, s.worst);
            }

            // E1 (ii): each band disabled in turn.
            Tally independent;
            double worstPartial = 0.0;

            for (int band = 1; band <= Processor::kNumBands; ++band)
            {
                setRaw (*p, bandId (band, "enabled"), 0.0f);

                const auto partial = renderImpulse (*p, 2, rate, amplitudes);
                const auto s = scoreChannels (measureImpulse (partial, amplitudes, rate, hz),
                                              chainRefDb (*p, *designer, rate, hz));
                const bool differs = ! sameImpulse (partial, full);

                independent.check (partial.ok() && activeBands (*p) == 23 && differs && s.within (limitDb),
                                   "band " + juce::String (band) + (differs ? "" : " (render unchanged)")
                                       + ": " + fmtG (s.worst) + " dB, " + describe (partial));
                worstPartial = juce::jmax (worstPartial, s.worst);

                setRaw (*p, bandId (band, "enabled"), 1.0f);
            }

            row (independent.bad == 0 && independent.total == 24,
                 "E1 (ii) each band disabled in turn: render differs from the full one, matches its 23-band closed form <= 0.01 dB",
                 independent.detail() + "  worst " + fmtG (worstPartial) + " dB");

            row (control.bad == 0 && control.total == 24,
                 "E1n the full render against each 23-band closed form FAILS the 0.01 dB test (control fires)",
                 control.detail() + "  smallest miss " + fmtG (smallestMiss) + " dB");
        }

        // E2, E2n (PLAN-2.2 P11)
        {
            struct Case { const char* name; BandSpec spec; };

            const Case cases[] =
            {
                { "Bell 1 kHz +12 dB Q 1",              makeSpec (kBell,      1000.0f,  12.0f, 1.0f,  1, kStereo) },
                { "Low Shelf 200 Hz +9 dB 24 dB/oct",   makeSpec (kLowShelf,  200.0f,    9.0f, 1.0f,  3, kStereo) },
                { "High Shelf 8 kHz -12 dB 12 dB/oct",  makeSpec (kHighShelf, 8000.0f, -12.0f, 1.0f,  1, kStereo) },
                { "Low Cut 80 Hz 48 dB/oct",            makeSpec (kLowCut,    80.0f,     0.0f, 1.0f,  5, kStereo) },
                { "High Cut 6 kHz 96 dB/oct",           makeSpec (kHighCut,   6000.0f,   0.0f, 1.0f,  7, kStereo) },
                { "Notch 1 kHz Q 10",                   makeSpec (kNotch,     1000.0f,   0.0f, 10.0f, 1, kStereo) },
                { "Band Pass 500 Hz Q 2",               makeSpec (kBandPass,  500.0f,    0.0f, 2.0f,  1, kStereo) },
                { "Tilt 1 kHz +6 dB 12 dB/oct",         makeSpec (kTilt,      1000.0f,   6.0f, 1.0f,  1, kStereo) }
            };

            Tally higher;

            for (const double rate : { 44100.0, 96000.0 })
            {
                const auto designer = designerFor (rate);
                const auto hz = responseGrid (rate);

                for (const int channels : { 1, 2 })
                {
                    Tally tally;
                    double worst = 0.0;
                    const juce::String where = fmtG (rate / 1000.0, 4) + " kHz " + (channels == 1 ? "mono" : "stereo");

                    for (const auto& c : cases)
                    {
                        auto p = makeBands (channels, 1, [&c] (int) { return c.spec; });

                        const auto ir = renderImpulse (*p, channels, rate, amplitudes);
                        const auto measured = measureImpulse (ir, amplitudes, rate, hz);
                        const auto s = scoreChannels (measured, chainRefDb (*p, *designer, rate, hz));

                        tally.check (ir.ok() && activeBands (*p) == 1 && s.within (limitDb),
                                     juce::String (c.name) + ": " + fmtG (s.worst) + " dB, " + describe (ir));
                        worst = juce::jmax (worst, s.worst);

                        // The figures the summary needs: worst error per type, rate and layout.
                        std::printf ("      E2 %-16s %-34s worst %-9s dB  (%d points above -80 dB, %s)\n",
                                     where.toRawUTF8(), c.name, fmtG (s.worst).toRawUTF8(), s.scored,
                                     describe (ir).toRawUTF8());

                        // E2n: the bell against the same bell 1 % higher in frequency.
                        if (&c == &cases[0])
                        {
                            const auto h = scoreChannels (measured, chainRefDb (*p, *designer, rate, hz, 0, 1.01f));
                            higher.check (h.misses (limitDb), where + ": " + fmtG (h.worst) + " dB");
                        }
                    }

                    row (tally.bad == 0 && tally.total == 8,
                         "E2 " + where + ": 8 types vs the closed form <= 0.01 dB above -80 dB; tail >= 140 dB down",
                         tally.detail() + "  worst " + fmtG (worst) + " dB");
                }
            }

            row (higher.bad == 0 && higher.total == 4,
                 "E2n the bell against the same bell 1 % higher in frequency FAILS the 0.01 dB test (control fires)",
                 higher.detail());
        }
    }

    //==========================================================================
    // N. above Nyquist (DSP-06)
    //==========================================================================
    void nyquistRows()
    {
        section ("N. above Nyquist (Phase 2.2: DSP-06)");

        // N1
        {
            const double rate = 44100.0;
            const auto designer = designerFor (rate);
            const auto hz = responseGrid (rate);
            const float amplitude[1] = { 1.0f };

            Tally tally;
            double worst = 0.0;
            int fewestPoints = std::numeric_limits<int>::max();

            for (int type = 0; type < 8; ++type)
                for (const float freq : { 25000.0f, 30000.0f })
                    for (const float gain : { 12.0f, -12.0f })
                    {
                        const float q = (type == kNotch || type == kBandPass) ? 0.5f : 1.0f;
                        auto p = makeBands (1, 1, [&] (int) { return makeSpec (type, freq, gain, q, 1, kStereo); });

                        const auto ir = renderImpulse (*p, 1, rate, amplitude);
                        const auto s = scoreChannels (measureImpulse (ir, amplitude, rate, hz),
                                                      chainRefDb (*p, *designer, rate, hz));

                        tally.check (ir.ok() && activeBands (*p) == 1 && s.within (0.01),
                                     juce::String (kTypeNames[type]) + " " + fmtG ((double) freq, 5) + " Hz "
                                         + fmtG ((double) gain) + " dB: " + fmtG (s.worst) + " dB, "
                                         + juce::String (s.scored) + " points, " + describe (ir));
                        worst = juce::jmax (worst, s.worst);
                        fewestPoints = juce::jmin (fewestPoints, s.scored);
                    }

            row (tally.bad == 0 && tally.total == 32,
                 "N1 44.1 kHz mono: 8 types at 25 kHz and 30 kHz, +-12 dB, 12 dB/oct: finite; closed form <= 0.01 dB above -80 dB",
                 tally.detail() + "  worst " + fmtG (worst) + " dB; fewest points scored " + juce::String (fewestPoints));
        }

        // N2
        {
            Tally tally;
            juce::String figures;

            for (const double rate : { 22050.0, 44100.0 })
            {
                const int length = (int) std::lround (rate * 5.0);
                auto input = makeNoiseAt (2, length, 0x0E2A0002, kNoise12);

                for (int channel = 0; channel < 2; ++channel)
                    input.clear (channel, length / 2, length - length / 2);     // noise, then silence

                auto p = makeBands (2, 24, [] (int index)
                {
                    return makeSpec (index % 8, 11000.0f * std::pow (30000.0f / 11000.0f, (float) index / 23.0f),
                                     ((index + index / 8) % 2 == 0) ? 6.0f : -6.0f, 1.0f, (index * 3) % 8, kStereo);
                });

                const auto output = render (*p, input, 512, false, rate);

                const bool finite = allFinite (output);
                const double inPeak = (double) input.getMagnitude (0, length);
                const double outPeak = finite ? (double) output.getMagnitude (0, length) : 0.0;
                const double peakDb = 20.0 * std::log10 (juce::jmax (outPeak, 1.0e-300) / inPeak);
                const bool changed = ! sameAudio (output, input);

                tally.check (finite && changed && activeBands (*p) == 24 && peakDb <= 40.0,
                             fmtG (rate / 1000.0, 5) + " kHz" + (finite ? "" : " NOT FINITE")
                                 + (changed ? "" : " (output equals input)") + " peak " + fmtG (peakDb, 4) + " dB");

                figures += (figures.isEmpty() ? "" : "; ") + fmtG (rate / 1000.0, 5) + " kHz peak "
                         + fmtG (peakDb, 4) + " dB re input";
            }

            row (tally.bad == 0 && tally.total == 2,
                 "N2 24 mixed bands at 11 - 30 kHz, +-6 dB, 22.05 and 44.1 kHz, 5 s noise then silence: finite; peak <= +40 dB re input",
                 tally.detail() + "  " + figures);
        }
    }

    //==========================================================================
    // T. transparency (G4 a, b, c, g; mono placement)
    //==========================================================================
    struct MsFigures
    {
        double untouchedDb = 0.0;   // the component the bands must leave, re the larger peak
        double touchedDb = 0.0;     // the component they act on, re the larger peak
        double outPeakDb = 0.0;     // output peak re input peak
    };

    // sideBands: the bands act on the side, so the mid is the untouched one.
    MsFigures msFigures (const juce::AudioBuffer<float>& in, const juce::AudioBuffer<float>& out, bool sideBands)
    {
        double inPeak = 0.0, outPeak = 0.0, midError = 0.0, sideError = 0.0;

        for (int i = 0; i < in.getNumSamples(); ++i)
        {
            const double inL = (double) in.getSample (0, i),   inR = (double) in.getSample (1, i);
            const double outL = (double) out.getSample (0, i), outR = (double) out.getSample (1, i);

            inPeak = juce::jmax (inPeak, std::abs (inL), std::abs (inR));
            outPeak = juce::jmax (outPeak, std::abs (outL), std::abs (outR));

            const double mid = std::abs (0.5 * (outL + outR) - 0.5 * (inL + inR));
            const double side = std::abs (0.5 * (outL - outR) - 0.5 * (inL - inR));

            // A NaN must not be lost in a max.
            if (! (mid <= midError))
                midError = mid;

            if (! (side <= sideError))
                sideError = side;
        }

        const double reference = juce::jmax (inPeak, outPeak);
        const double midDb = 20.0 * std::log10 (juce::jmax (midError, 1.0e-300) / reference);
        const double sideDb = 20.0 * std::log10 (juce::jmax (sideError, 1.0e-300) / reference);

        MsFigures f;
        f.untouchedDb = std::isnan (midError) || std::isnan (sideError) ? std::numeric_limits<double>::quiet_NaN()
                                                                        : (sideBands ? midDb : sideDb);
        f.touchedDb = sideBands ? sideDb : midDb;
        f.outPeakDb = 20.0 * std::log10 (juce::jmax (outPeak, 1.0e-300) / inPeak);
        return f;
    }

    void transparencyRows()
    {
        section ("T. transparency (Phase 2.2: G4 a, b, c, g; mono placement)");

        // Block sizes 1, 16, 64, 512, 1024, 4096 and the seeded random sequence (0).
        const int t1Patterns[] = { 1, 16, 64, 512, 1024, 4096, 0 };

        // T1a
        for (const int channels : { 1, 2 })
        {
            const auto input = makeNoiseTail (channels, 0x71A00000 + channels);

            Tally tally;
            tally.check (hasDenormalTail (input), "stimulus: the tail is not denormal non-zero, or this thread flushes to zero");

            for (const int pattern : t1Patterns)
            {
                auto p = make();
                const bool layoutOk = setLayout (*p, channels);

                setAllRandom (*p, 0x71A05EED + pattern);
                disableEveryBand (*p);
                setNormalised (*p, "bypass", 0.0f);         // the engine path, not the hard bypass
                setRaw (*p, "output_gain", 0.0f);

                const bool stimulusOk = countNonDefault (*p) > kMinNonDefault && enabledBands (*p) == 0
                                     && sameBits (rawOf (*p, "output_gain"), 0.0f) && rawOf (*p, "bypass") < 0.5f;

                const auto output = renderPattern (*p, input, pattern);

                tally.check (layoutOk && stimulusOk && sameAudio (output, input),
                             patternName (pattern) + (layoutOk ? "" : " (layout refused)")
                                 + (stimulusOk ? "" : " (stimulus not applied)"));
            }

            row (tally.bad == 0 && tally.total == 8,
                 juce::String ("T1a ") + (channels == 1 ? "mono" : "stereo")
                     + ": no band enabled, the rest random, output 0 dB, noise + denormal tail: out memcmp-equal to in (7 block patterns)",
                 tally.detail());
        }

        // T1b, T1bn
        {
            const auto input = makeNoiseTail (2, 0x71B00002);

            Tally tally, control;
            tally.check (hasDenormalTail (input), "stimulus: the tail is not denormal non-zero, or this thread flushes to zero");

            for (const int placement : { kLeft, kRight })
            {
                const int touched = placement == kLeft ? 0 : 1;
                const juce::String side = kPlacementNames[placement];

                for (const int pattern : t1Patterns)
                {
                    auto p = makeBands (2, 6, [placement] (int index) { return channelBandSpec (index, placement); });
                    const auto output = renderPattern (*p, input, pattern);

                    const bool untouchedSame = sameChannel (output, input, 1 - touched);
                    const bool touchedDiffers = ! sameChannel (output, input, touched);

                    tally.check (untouchedSame && touchedDiffers,
                                 side + " bands, " + patternName (pattern)
                                     + (untouchedSame ? "" : ": the other channel changed")
                                     + (touchedDiffers ? "" : ": the band's channel did not change"));

                    // T1bn: the same six bands on Stereo.
                    auto q = makeBands (2, 6, [] (int index) { return channelBandSpec (index, kStereo); });
                    const auto both = renderPattern (*q, input, pattern);

                    control.check (! sameChannel (both, input, 1 - touched),
                                   "Stereo bands, " + patternName (pattern) + ": channel "
                                       + juce::String (1 - touched) + " still memcmp-equal");
                }
            }

            row (tally.bad == 0 && tally.total == 15,
                 "T1b 6 Left bands, noise + denormal tail: right channel memcmp-equal, left differs; and the converse (7 block patterns)",
                 tally.detail());

            row (control.bad == 0 && control.total == 14,
                 "T1bn the same bands on Stereo FAIL the untouched-channel comparison (control fires)",
                 control.detail());
        }

        // T1c, T1cn
        {
            constexpr int length = 131072;
            const auto input = makeNoiseAt (2, length, 0x71C00002, kNoise12);

            struct Config { const char* name; int numBands; float gainDb; int placement; };

            const Config configs[] =
            {
                { "24 Mid bells +12 dB",  24, 12.0f, kMid },
                { "1 Mid bell +30 dB",    1,  30.0f, kMid },
                { "24 Side bells +12 dB", 24, 12.0f, kSide },
                { "1 Side bell +30 dB",   1,  30.0f, kSide }
            };

            Tally tally, control;

            for (const auto& c : configs)
            {
                const bool sideBands = c.placement == kSide;

                auto p = makeBands (2, c.numBands, [&c] (int index) { return msBellSpec (index, c.numBands, c.gainDb, c.placement); });
                const auto f = msFigures (input, render (*p, input, 512, false), sideBands);

                tally.check (f.untouchedDb <= -130.0 && f.touchedDb > -40.0,
                             juce::String (c.name) + ": untouched " + fmtG (f.untouchedDb, 4) + " dB, touched "
                                 + fmtG (f.touchedDb, 4) + " dB");

                info (juce::String ("T1c ") + c.name + ": " + (sideBands ? "mid" : "side") + " error "
                          + fmtG (f.untouchedDb, 4) + " dB re the larger peak (limit -130); "
                          + (sideBands ? "side" : "mid") + " change " + fmtG (f.touchedDb, 4)
                          + " dB (must exceed -40); output peak " + fmtG (f.outPeakDb, 4) + " dB re input");

                // T1cn: the same bells on Left.
                auto q = makeBands (2, c.numBands, [&c] (int index) { return msBellSpec (index, c.numBands, c.gainDb, kLeft); });
                const auto g = msFigures (input, render (*q, input, 512, false), sideBands);

                control.check (g.untouchedDb > -130.0,
                               juce::String (c.name) + " moved to Left: " + fmtG (g.untouchedDb, 4) + " dB");

                info (juce::String ("T1cn ") + c.name + " moved to Left: " + (sideBands ? "mid" : "side") + " error "
                          + fmtG (g.untouchedDb, 4) + " dB re the larger peak");
            }

            row (tally.bad == 0 && tally.total == 4,
                 "T1c Mid bells leave the side, Side bells leave the mid: <= -130 dB re max (input peak, output peak); touched > -40 dB",
                 tally.detail());

            row (control.bad == 0 && control.total == 4,
                 "T1cn the same bells on Left FAIL the untouched-component test (control fires)",
                 control.detail());
        }

        // T1g
        {
            const int patterns[] = { 32, 64, 512, 2048, 1, 0 };
            const auto input = makeNoiseAt (2, kNoiseLength + 777, 0x71600002, kNoise12);

            juce::AudioBuffer<float> reference;
            Tally tally;
            double peakDb = 0.0;

            for (const int pattern : patterns)
            {
                auto p = makeBands (2, 24, mixedSpec);
                const auto output = renderPattern (*p, input, pattern);

                if (reference.getNumSamples() == 0)
                {
                    reference = output;

                    const double outPeak = (double) output.getMagnitude (0, output.getNumSamples());
                    peakDb = 20.0 * std::log10 (juce::jmax (outPeak, 1.0e-300) / (double) kNoise12);

                    tally.check (allFinite (output) && outPeak > 0.0 && ! sameAudio (output, input) && activeBands (*p) == 24,
                                 patternName (pattern) + ": the setting does not act on the audio");
                }
                else
                {
                    tally.check (sameAudio (output, reference), patternName (pattern) + " differs from block 32");
                }
            }

            row (tally.bad == 0 && tally.total == 6,
                 "T1g 24-band mixed setting (all types, slopes, placements; +-6 dB): blocks 32 / 64 / 512 / 2048 / 1 / random sequence bit-identical",
                 tally.detail() + "  output peak " + fmtG (peakDb, 4) + " dB re the noise peak");
        }

        // T1m
        {
            const auto input = makeNoiseAt (1, kNoiseLength, 0x71D00001, kNoise12);

            juce::AudioBuffer<float> stereoPlacement;
            Tally tally;

            for (int placement = 0; placement < 5; ++placement)
            {
                auto p = makeBands (1, 1, [placement] (int) { return makeSpec (kBell, 1000.0f, 6.0f, 1.0f, 1, placement); });
                const auto output = render (*p, input, 512, false);

                if (placement == kStereo)
                    stereoPlacement = output;

                const bool placementSet = sameBits (rawOf (*p, bandId (1, "placement")), (float) placement);
                const bool differs = ! sameAudio (output, input);
                const bool sameAsStereo = sameAudio (output, stereoPlacement);

                tally.check (placementSet && differs && sameAsStereo,
                             juce::String (kPlacementNames[placement]) + (placementSet ? "" : " (placement not applied)")
                                 + (differs ? "" : " (output equals input)")
                                 + (sameAsStereo ? "" : " (differs from the Stereo-placement render)"));
            }

            row (tally.bad == 0 && tally.total == 5,
                 "T1m mono layout, one band per placement value: output differs from input, memcmp-equal to the Stereo-placement render",
                 tally.detail());
        }
    }

    //==========================================================================
    // Z. latency and the first sample (G8)
    //==========================================================================
    struct Coverage
    {
        unsigned types = 0, slopes = 0, placements = 0;

        bool complete() const   { return types == 0xFFu && slopes == 0xFFu && placements == 0x1Fu; }
    };

    // 1 ... 24 bands on a seeded random subset: every type, slope and placement;
    // frequency and Q over their full ranges; gains +-12 dB (PLAN-2.2 P12).
    void applyRandomBands (juce::AudioProcessor& p, juce::int64 seed, Coverage& coverage)
    {
        juce::Random random (seed);
        int order[Processor::kNumBands];

        for (int i = 0; i < Processor::kNumBands; ++i)
            order[i] = i + 1;

        for (int i = Processor::kNumBands - 1; i > 0; --i)
            std::swap (order[i], order[random.nextInt (i + 1)]);

        const int numBands = 1 + random.nextInt (Processor::kNumBands);

        for (int k = 0; k < numBands; ++k)
        {
            BandSpec s;
            s.type = random.nextInt (8);
            s.slope = random.nextInt (8);
            s.placement = random.nextInt (5);
            s.freq = 10.0f * std::pow (3000.0f, random.nextFloat());
            s.q = 0.025f * std::pow (1600.0f, random.nextFloat());
            s.gain = -12.0f + 24.0f * random.nextFloat();
            setBand (p, order[k], s);

            coverage.types |= 1u << s.type;
            coverage.slopes |= 1u << s.slope;
            coverage.placements |= 1u << s.placement;
        }
    }

    constexpr int kImpulseIndex = 37;

    struct FirstSample
    {
        int firstNonZero[2] = { -1, -1 };
        double predicted[2] = { 0.0, 0.0 };
    };

    /** One Z1 case: an impulse at index 37 (1.0 on L, 0.5 on R), blocks of 16
        so the impulse is not in the first block. Checks latency 0, exact
        silence before the impulse, and the output at the impulse index
        against the predicted first-sample gain: per-band
        Band::firstSampleGain() scalars with Router::applyFirstSample applied
        in band index order over the active bands, within 1e-6 relative where
        the prediction is at least 1e-30 in magnitude. */
    FirstSample checkFirstSample (Processor& p, const eq::Designer& designer, double rate, int channels, int length,
                                  const juce::String& label, Tally& tally, int& compared)
    {
        const float amplitudes[2] = { 1.0f, 0.5f };

        juce::AudioBuffer<float> input (channels, length);
        input.clear();

        for (int channel = 0; channel < channels; ++channel)
            input.setSample (channel, kImpulseIndex, amplitudes[channel]);

        const auto output = render (p, input, 16, false, rate);

        double l = (double) amplitudes[0];
        double r = (double) amplitudes[1];

        for (int band = 1; band <= Processor::kNumBands; ++band)
        {
            const auto in = inputOf (p, band);

            if (! in.active)
                continue;

            eq::BandDesign design {};
            designer.design (in.settings, design);

            eq::Band engineBand;
            engineBand.setDesign (design);
            eq::Router::applyFirstSample (engineBand.firstSampleGain(), in.placement, channels == 2, l, r);
        }

        FirstSample result;
        result.predicted[0] = l;
        result.predicted[1] = r;

        bool silentBefore = true, gainOk = true;
        juce::String note;

        for (int channel = 0; channel < channels; ++channel)
        {
            const float* y = output.getReadPointer (channel);

            for (int i = 0; i < kImpulseIndex; ++i)
                if (y[i] != 0.0f)
                    silentBefore = false;

            for (int i = 0; i < length; ++i)
                if (y[i] != 0.0f)
                {
                    result.firstNonZero[channel] = i;
                    break;
                }

            const double predicted = result.predicted[channel];

            if (std::abs (predicted) >= 1.0e-30)
            {
                ++compared;
                const double got = (double) y[kImpulseIndex];

                if (! (std::abs (got - predicted) <= 1.0e-6 * std::abs (predicted)))
                {
                    gainOk = false;
                    note += " ch " + juce::String (channel) + " predicted " + fmtJ (predicted) + " got " + fmtJ (got);
                }
            }
        }

        const int latency = p.getLatencySamples();

        tally.check (latency == 0 && silentBefore && gainOk,
                     label + (latency == 0 ? juce::String() : " latency " + juce::String (latency))
                         + (silentBefore ? "" : " output before the impulse") + note);

        return result;
    }

    void latencyRows()
    {
        section ("Z. latency and first sample (Phase 2.2: G8)");

        Tally tally;
        Coverage coverage;
        int compared = 0;

        for (const double rate : { 44100.0, 96000.0, 192000.0 })
        {
            const auto designer = designerFor (rate);

            for (int config = 0; config < 20; ++config)
                for (const int channels : { 1, 2 })
                {
                    auto p = makeBands (channels, 0, mixedSpec);
                    applyRandomBands (*p, 0x21000000 + config, coverage);

                    checkFirstSample (*p, *designer, rate, channels, 256,
                                      "config " + juce::String (config) + " " + fmtG (rate / 1000.0, 4) + " kHz "
                                          + (channels == 1 ? "mono" : "stereo") + " (" + juce::String (activeBands (*p)) + " bands)",
                                      tally, compared);
                }
        }

        // The three forced cases of RESEARCH-2.2 Q3: the first-sample gain is
        // below the smallest float (2.0e-47, 1.2e-57) or a float denormal
        // (1.2e-41), so the float output starts late. Latency and the silence
        // before the impulse are still checked; the index is INFO.
        struct Forced { const char* name; float freq; double rate; };

        const Forced forced[] =
        {
            { "High Cut 10 Hz 96 dB/oct Q 0.025, 44.1 kHz", 10.0f,  44100.0 },
            { "High Cut 10 Hz 96 dB/oct Q 0.025, 192 kHz",  10.0f,  192000.0 },
            { "High Cut 100 Hz 96 dB/oct Q 0.025, 192 kHz", 100.0f, 192000.0 }
        };

        juce::StringArray forcedLines;

        for (const auto& f : forced)
        {
            const auto designer = designerFor (f.rate);
            juce::String line = juce::String ("Z1 forced, ") + f.name + " (impulse at index "
                              + juce::String (kImpulseIndex) + "): first non-zero index";

            for (const int channels : { 1, 2 })
            {
                auto p = makeBands (channels, 1, [&f] (int) { return makeSpec (kHighCut, f.freq, 0.0f, 0.025f, 7, kStereo); });

                const auto first = checkFirstSample (*p, *designer, f.rate, channels, kImpulseIndex + 1024,
                                                     juce::String (f.name) + (channels == 1 ? " mono" : " stereo"),
                                                     tally, compared);

                if (channels == 1)
                    line += " mono " + juce::String (first.firstNonZero[0]) + " (predicted first-sample gain "
                          + fmtG (first.predicted[0]) + ");";
                else
                    line += " stereo L " + juce::String (first.firstNonZero[0]) + ", R " + juce::String (first.firstNonZero[1]);
            }

            forcedLines.add (line);
        }

        row (tally.bad == 0 && tally.total == 126 && compared > 0 && coverage.complete(),
             "Z1 20 random configurations x 44.1 / 96 / 192 kHz x mono / stereo + 3 forced High Cuts: latency 0, silent before index 37, first-sample gain within 1e-6",
             tally.detail() + "  " + juce::String (compared) + " gains compared (predicted >= 1e-30)"
                 + (coverage.complete() ? "" : "  (the draw missed a type, slope or placement)"));

        for (const auto& line : forcedLines)
            info (line + "  (-1 = no non-zero sample in the render)");
    }

    //==========================================================================
    // A. allocation (G6, static parameters)
    //==========================================================================
   #if defined (__APPLE__)
    // volatile: the compiler knows the allocator as a builtin that cannot touch
    // globals and would otherwise fold "armed = true; ...; armed = false".
    volatile bool allocArmed = false;
    volatile int allocCount = 0;
    volatile int freeCount = 0;
    pthread_t allocThread;

    void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
    {
        if (! allocArmed || pthread_equal (pthread_self(), allocThread) == 0)
            return;

        if ((type & 2u) != 0)       // MALLOC_LOG_TYPE_ALLOCATE
            allocCount = allocCount + 1;

        if ((type & 4u) != 0)       // MALLOC_LOG_TYPE_DEALLOCATE
            freeCount = freeCount + 1;
    }

    // 24 bands at 96 dB/oct: the five slope types, every placement, gains +-3 dB.
    BandSpec allocSpec (int index)
    {
        static const int types[5] = { kLowShelf, kHighShelf, kLowCut, kHighCut, kTilt };

        BandSpec s;
        s.type = types[index % 5];
        s.slope = 7;
        s.placement = (index + index / 5) % 5;
        s.freq = 60.0f * std::pow (200.0f, (float) ((index * 7) % 24) / 23.0f);
        s.gain = (index % 2 == 0) ? 3.0f : -3.0f;
        s.q = 1.0f;

        if (s.type == kLowCut)
            s.freq = 30.0f + 10.0f * (float) (index / 5);

        if (s.type == kHighCut)
            s.freq = 12000.0f + 1500.0f * (float) (index / 5);

        return s;
    }
   #endif

    struct AllocResult
    {
        bool available = false;
        int allocs = 0, frees = 0;
        int probeAllocs = 0, probeFrees = 0;
        int blocks = 0;
        bool audioOk = false;
    };

    /** The logger counts on this thread (the one that calls processBlock) and
        only while armed; it is armed around processBlock alone. Every
        parameter write happens between windows: a write notifies listeners
        and may allocate legitimately. A separate window with a deliberate
        allocation proves the hook live. `plant` puts one deliberate
        allocation inside a processBlock window (A1n). */
    AllocResult runAllocWindow (bool plant)
    {
        AllocResult result;

       #if defined (__APPLE__)
        constexpr int blockSize = 512;

        auto p = makeBands (2, 24, allocSpec);
        const auto noise = makeNoiseAt (2, blockSize, 0x0A110C8, kNoise12);
        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;

        p->prepareToPlay (kRenderRate, blockSize);

        malloc_logger_t* const previous = malloc_logger;
        allocThread = pthread_self();
        allocCount = 0;
        freeCount = 0;
        malloc_logger = countAllocation;

        // Liveness: the hook must see a deliberate allocation and its release.
        allocArmed = true;
        {
            void* volatile probe = std::malloc (64);
            std::free (probe);
        }
        allocArmed = false;
        result.probeAllocs = allocCount;
        result.probeFrees = freeCount;
        allocCount = 0;
        freeCount = 0;

        bool plantNext = plant;

        const auto armedBlock = [&]()
        {
            for (int channel = 0; channel < 2; ++channel)
                block.copyFrom (channel, 0, noise, channel, 0, blockSize);      // outside the window

            allocArmed = true;
            p->processBlock (block, midi);

            if (plantNext)
            {
                void* volatile planted = std::malloc (64);
                std::free (planted);
            }

            allocArmed = false;
            plantNext = false;
            ++result.blocks;
        };

        // The write is outside the window; the block that acts on it is inside.
        const auto change = [&] (const juce::String& id, float raw)
        {
            setRaw (*p, id, raw);
            armedBlock();
        };

        armedBlock();                                   // the first block after prepareToPlay (24 designs)
        const bool firstActs = ! sameAudio (block, noise);

        for (int i = 0; i < 200; ++i)                   // settled
            armedBlock();

        change (bandId (1, "gain"), -2.0f);             // continuous
        change (bandId (1, "freq"), 180.0f);
        change (bandId (1, "q"), 2.5f);
        change ("gain_scale", 80.0f);                   // continuous, every band redesigned
        change (bandId (2, "type"), (float) kBell);     // structural: type
        change (bandId (6, "slope"), 3.0f);             // structural: slope
        change (bandId (7, "placement"), (float) kStereo);  // structural: placement
        change (bandId (3, "enabled"), 0.0f);           // enable off / on
        change (bandId (3, "enabled"), 1.0f);
        change (bandId (4, "bypass"), 1.0f);            // band bypass on / off
        change (bandId (4, "bypass"), 0.0f);
        change ("bypass", 1.0f);                        // global bypass on / off

        // Amended in Phase 2.4 (PLAN-2.4 P18): global bypass is an N-sample
        // fade to the dry input, so "the bypassed block equals the input" is
        // checked on the first block after the fade has ended. The blocks of
        // the fade (and of the fade back) stay inside the allocation window.
        const int fadeBlocks = (eq::fadeLengthSamples (kRenderRate) + blockSize - 1) / blockSize;   // the toggle block included

        for (int i = 1; i < fadeBlocks; ++i)
            armedBlock();

        armedBlock();                                   // the first block after the fade has ended
        const bool bypassPasses = sameAudio (block, noise);
        change ("bypass", 0.0f);
        const bool resumed = ! sameAudio (block, noise);

        for (int i = 1; i < fadeBlocks; ++i)
            armedBlock();

        malloc_logger = previous;
        p->releaseResources();

        result.available = true;
        result.allocs = allocCount;
        result.frees = freeCount;
        result.audioOk = firstActs && bypassPasses && resumed;
       #else
        juce::ignoreUnused (plant);
       #endif

        return result;
    }

    void allocationRows()
    {
        section ("A. allocation (Phase 2.2: G6, static parameters)");

        const AllocResult clean = runAllocWindow (false);

        if (! clean.available)
        {
            info ("A1 allocation gate SKIPPED: no malloc_logger on this platform");
            info ("A1n allocation control SKIPPED: no malloc_logger on this platform");
            return;
        }

        const bool hookLive = clean.probeAllocs >= 1 && clean.probeFrees >= 1;

        row (hookLive && clean.audioOk && clean.allocs == 0 && clean.frees == 0,
             "A1 processBlock, 24 bands x 96 dB/oct: first block, 200 settled, the block after each kind of change: 0 allocations, 0 frees",
             juce::String (clean.allocs) + " allocations, " + juce::String (clean.frees) + " frees in "
                 + juce::String (clean.blocks) + " windows (malloc_logger, this thread); hook "
                 + (hookLive ? "live" : "DEAD") + " (probe: " + juce::String (clean.probeAllocs) + " / "
                 + juce::String (clean.probeFrees) + ")" + (clean.audioOk ? "" : "; the engine did not act as expected"));

        const AllocResult planted = runAllocWindow (true);

        row (planted.allocs >= 1 && planted.frees >= 1,
             "A1n a planted allocation inside the processBlock window is counted (control fires)",
             juce::String (planted.allocs) + " allocations, " + juce::String (planted.frees) + " frees");
    }

    //==========================================================================
    // K, H, U, V. host block size and the 2.2 state rules
    //==========================================================================
    // Gain in dB of a steady tone over [start, start + length): the output's
    // single-bin projection over the input's. length must hold whole cycles.
    double toneGainDb (const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output,
                       int start, int length, double hz, double rate)
    {
        double inRe = 0.0, inIm = 0.0, outRe = 0.0, outIm = 0.0;

        for (int i = 0; i < length; ++i)
        {
            const double phase = 2.0 * eq::pi * hz * (double) (start + i) / rate;
            const double c = std::cos (phase), s = std::sin (phase);
            const double x = (double) input.getSample (0, start + i);
            const double y = (double) output.getSample (0, start + i);

            inRe += x * c;
            inIm -= x * s;
            outRe += y * c;
            outIm -= y * s;
        }

        return eq::powerToDb ((outRe * outRe + outIm * outIm) / (inRe * inRe + inIm * inIm));
    }

    struct EditResult
    {
        bool identical = false;     // the render with the edit equals the render without
        bool applied = false;       // the raw value did change
        bool audible = false;       // the band acts on the audio at all
    };

    // One band; the same input rendered without and with one parameter edit
    // before block 10 (blocks of 512).
    EditResult renderWithEdit (int channels, const BandSpec& spec, const char* suffix, float newRaw)
    {
        const auto input = makeNoiseAt (channels, 12000, 0x0E3A0000 + channels, kNoise12);
        const auto id = bandId (1, suffix);

        auto a = makeBands (channels, 1, [&spec] (int) { return spec; });
        auto b = makeBands (channels, 1, [&spec] (int) { return spec; });

        const auto plain = render (*a, input, 512, false);
        const auto edited = render (*b, input, 512, false, kRenderRate, [&] (BlockInfo& current)
        {
            if (current.index == 10)
                setRaw (*b, id, newRaw);
        });

        EditResult result;
        result.identical = sameAudio (plain, edited);
        result.applied = ! sameBits (rawOf (*a, id), rawOf (*b, id));
        result.audible = ! sameAudio (plain, input);
        return result;
    }

    void blockAndStateRows()
    {
        section ("K / H / U / V. host block size and the 2.2 state rules");

        // K1
        {
            const auto input = makeNoiseAt (2, 17000, 0x4B100002, kNoise12);

            auto a = makeBands (2, 24, mixedSpec);
            auto b = makeBands (2, 24, mixedSpec);

            const auto oversized = render (*a, input, 1024, false, kRenderRate, {}, {}, 256);
            const auto prepared = render (*b, input, 256, false);
            const bool acts = ! sameAudio (prepared, input);

            row (acts && sameAudio (oversized, prepared),
                 "K1 prepare 256, feed 1024-sample blocks, 24-band setting: bit-identical to the same input in 256-sample blocks",
                 acts ? juce::String() : juce::String ("the setting does not act on the audio"));
        }

        // H1
        {
            constexpr int blockSize = 512, firstBypassed = 10, firstResumed = 20;
            const auto input = makeNoiseTail (2, 0x48100002);       // 36 blocks of 512
            const int total = input.getNumSamples();
            const int bypassAt = firstBypassed * blockSize;
            const int resumeAt = firstResumed * blockSize;

            auto steady = makeBands (2, 24, mixedSpec);
            const auto steadyOut = render (*steady, input, blockSize, false);

            // (i) the buffer is untouched
            Tally untouched;
            untouched.check (hasDenormalTail (input), "stimulus: the tail is not denormal non-zero, or this thread flushes to zero");
            untouched.check (! sameAudio (steadyOut, input) && activeBands (*steady) == 24, "control: with bypass off the 24 bands do not act");

            {
                auto p = makeBands (2, 24, mixedSpec);
                setNormalised (*p, "bypass", 1.0f);
                untouched.check (sameAudio (render (*p, input, blockSize, false), input), "bypass on, processBlock");

                auto q = makeBands (2, 24, mixedSpec);
                untouched.check (sameAudio (render (*q, input, blockSize, true), input), "processBlockBypassed");
            }

            row (untouched.bad == 0 && untouched.total == 4,
                 "H1 (i) 24 bands enabled, noise + denormal tail: bypass on via processBlock, and processBlockBypassed: out memcmp-equal to in",
                 untouched.detail());

            // (ii) the state was cleared
            // Amended in Phase 2.4 (PLAN-2.4 P18): global bypass is an N-sample
            // fade to the dry input and un-bypass an N-sample fade back onto
            // bands started from rest. The bypassed span is the input from
            // the toggle + N, and the output equals a fresh instance started
            // at the un-bypass sample from un-bypass + N, bit for bit, by
            // both routes. N = eq::fadeLengthSamples (fs).
            const int fadeN = eq::fadeLengthSamples (kRenderRate);

            auto fresh = makeBands (2, 24, mixedSpec);
            const auto freshOut = render (*fresh, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

            Tally resumed;
            resumed.check (! sameFrom (steadyOut, resumeAt, freshOut, 0, total - resumeAt),
                           "control: an uninterrupted render equals a fresh one (state does not matter)");

            for (const bool viaParameter : { true, false })
            {
                auto p = makeBands (2, 24, mixedSpec);

                const auto output = render (*p, input, blockSize, false, kRenderRate, [&] (BlockInfo& current)
                {
                    if (viaParameter)
                    {
                        if (current.index == firstBypassed)
                            setNormalised (*p, "bypass", 1.0f);

                        if (current.index == firstResumed)
                            setNormalised (*p, "bypass", 0.0f);
                    }
                    else
                    {
                        current.viaBypassed = current.index >= firstBypassed && current.index < firstResumed;
                    }
                });

                const juce::String route = viaParameter ? "bypass parameter" : "processBlockBypassed";

                resumed.check (sameFrom (output, 0, steadyOut, 0, bypassAt), route + ": output before the bypass differs from the steady render");
                resumed.check (sameFrom (output, bypassAt + fadeN, input, bypassAt + fadeN, resumeAt - bypassAt - fadeN),
                               route + ": the bypassed span differs from the input from the toggle + N");
                resumed.check (sameFrom (output, resumeAt + fadeN, freshOut, fadeN, total - resumeAt - fadeN),
                               route + ": output from un-bypass + N differs from a fresh instance started at the un-bypass sample");
            }

            row (resumed.bad == 0 && resumed.total == 7,
                 "H1 (ii) bypassed: memcmp-equal to the input from the toggle + N; after bypass off: from un-bypass + N memcmp-equal to a fresh instance started at the un-bypass sample (both routes)",
                 resumed.detail() + "  N = " + juce::String (fadeN));
        }

        // U1
        {
            constexpr int blockSize = 512, editBlock = 47, window = 4800;   // 100 cycles of 1 kHz at 48 kHz
            const double rate = kRenderRate;
            const int editAt = editBlock * blockSize;
            const int oneSecond = (int) std::lround (rate);
            const int total = editAt + oneSecond + window;

            juce::AudioBuffer<float> input (1, total);

            for (int i = 0; i < total; ++i)
                input.setSample (0, i, kNoise12 * (float) std::sin (2.0 * eq::pi * 1000.0 * (double) i / rate));

            const auto designer = designerFor (rate);
            const std::vector<double> at { 1000.0 };

            auto p = makeBands (1, 1, [] (int) { return makeSpec (kBell, 1000.0f, 6.0f, 1.0f, 1, kStereo); });
            const double oldRef = chainRefDb (*p, *designer, rate, at)[0];

            const auto output = render (*p, input, blockSize, false, rate, [&] (BlockInfo& current)
            {
                if (current.index == editBlock)
                    setRaw (*p, bandId (1, "gain"), -9.0f);
            });

            const double newRef = chainRefDb (*p, *designer, rate, at)[0];
            const double before = toneGainDb (input, output, editAt - window, window, 1000.0, rate);
            const double after = toneGainDb (input, output, editAt + oneSecond, window, 1000.0, rate);

            row (std::abs (after - newRef) <= 0.01 && std::abs (before - oldRef) <= 0.01 && std::abs (newRef - oldRef) > 1.0,
                 "U1 gain edit between blocks on a 1 kHz bell, 1 kHz sine: steady-state gain 1 s later equals the new closed form within 0.01 dB",
                 "after " + fmtG (after, 6) + " dB (closed form " + fmtG (newRef, 6) + "); before " + fmtG (before, 6)
                     + " dB (closed form " + fmtG (oldRef, 6) + ")");
        }

        // U2
        // Amended in Phase 2.4 (PLAN-2.4 P18): a disable and an enable are
        // N-sample crossfades. The band stays disabled for at least N samples
        // (two blocks of 512 at 48 kHz); the disabled span is the input from
        // the disable + N; from the re-enable + N the output equals a fresh
        // instance started at the re-enable. N = eq::fadeLengthSamples (fs).
        {
            constexpr int blockSize = 512, offBlock = 10, offBlocks = 2;
            const int fadeN = eq::fadeLengthSamples (kRenderRate);
            const auto input = makeNoiseAt (2, 36 * blockSize, 0x0E2B0002, kNoise12);
            const int total = input.getNumSamples();
            const int offAt = offBlock * blockSize;
            const int resumeAt = offAt + offBlocks * blockSize;

            const auto spec = makeSpec (kLowShelf, 300.0f, 9.0f, 1.0f, 3, kStereo);
            const auto one = [&spec] (int) { return spec; };

            auto p = makeBands (2, 1, one);

            const auto output = render (*p, input, blockSize, false, kRenderRate, [&] (BlockInfo& current)
            {
                if (current.index == offBlock)
                    setRaw (*p, bandId (1, "enabled"), 0.0f);

                if (current.index == offBlock + offBlocks)
                    setRaw (*p, bandId (1, "enabled"), 1.0f);
            });

            auto steady = makeBands (2, 1, one);
            const auto steadyOut = render (*steady, input, blockSize, false);

            auto fresh = makeBands (2, 1, one);
            const auto freshOut = render (*fresh, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

            const bool longEnough = resumeAt - offAt >= fadeN;
            const bool before = sameFrom (output, 0, steadyOut, 0, offAt);
            const bool during = longEnough && sameFrom (output, offAt + fadeN, input, offAt + fadeN, resumeAt - offAt - fadeN);
            const bool after = sameFrom (output, resumeAt + fadeN, freshOut, fadeN, total - resumeAt - fadeN);
            const bool stateMatters = ! sameFrom (steadyOut, resumeAt, freshOut, 0, total - resumeAt);

            row (before && during && after && stateMatters,
                 "U2 band disabled for at least N samples, then enabled: disabled span memcmp-equal to the input from the disable + N; from the re-enable + N memcmp-equal to a fresh instance started at the re-enable",
                 juce::String (before ? "" : "output before the edit differs from the steady render; ")
                     + (longEnough ? "" : "the band is disabled for fewer than N samples; ")
                     + (during ? "" : "the disabled span is not the input from the disable + N; ")
                     + (after ? "" : "output from the re-enable + N differs from a fresh instance; ")
                     + (stateMatters ? "" : "control: the steady render equals the fresh one; ")
                     + "N = " + juce::String (fadeN));
        }

        // U3
        {
            struct Edit { const char* name; int channels; BandSpec spec; const char* suffix; float newRaw; };

            const Edit inert[] =
            {
                { "slope on a Bell",           2, makeSpec (kBell,     1000.0f, 6.0f, 1.0f, 1, kStereo), "slope",     5.0f },
                { "slope on a Notch",          2, makeSpec (kNotch,    1000.0f, 0.0f, 4.0f, 1, kStereo), "slope",     6.0f },
                { "slope on a Band Pass",      2, makeSpec (kBandPass, 500.0f,  0.0f, 2.0f, 1, kStereo), "slope",     3.0f },
                { "Q on a 6 dB/oct Low Cut",   2, makeSpec (kLowCut,   200.0f,  0.0f, 1.0f, 0, kStereo), "q",         4.0f },
                { "placement on mono",         1, makeSpec (kBell,     1000.0f, 6.0f, 1.0f, 1, kStereo), "placement", (float) kMid }
            };

            Tally tally;

            for (const auto& e : inert)
            {
                const auto r = renderWithEdit (e.channels, e.spec, e.suffix, e.newRaw);
                tally.check (r.identical && r.applied && r.audible,
                             juce::String (e.name) + (r.identical ? "" : " changed the output")
                                 + (r.applied ? "" : " (edit not applied)") + (r.audible ? "" : " (band does not act)"));
            }

            // Control: the same edit where it is not inert.
            const auto live = renderWithEdit (2, makeSpec (kLowCut, 200.0f, 0.0f, 1.0f, 1, kStereo), "slope", 5.0f);
            tally.check (! live.identical && live.applied && live.audible, "control: a slope edit on a Low Cut left the output unchanged");

            row (tally.bad == 0 && tally.total == 6,
                 "U3 inert edits mid-render (slope on Bell / Notch / Band Pass, Q at 6 dB/oct, placement on mono): bit-identical; a Low Cut slope edit changes the output",
                 tally.detail());
        }

        // V1
        {
            Tally tally;

            auto fresh = make();

            for (int band = 0; band < Processor::kNumBands; ++band)
                tally.check (fresh->getBandDynOffsetDb (band) == 0.0, "fresh instance, band " + juce::String (band + 1));

            auto p = makeBands (2, 24, mixedSpec);
            const auto input = makeNoiseAt (2, (int) std::lround (kRenderRate), 0x56100002, kNoise12);
            const auto output = render (*p, input, 512, false);

            for (int band = 0; band < Processor::kNumBands; ++band)
                tally.check (p->getBandDynOffsetDb (band) == 0.0, "after a 1 s render, band " + juce::String (band + 1));

            const bool acts = ! sameAudio (output, input);

            row (tally.bad == 0 && tally.total == 48 && acts,
                 "V1 getBandDynOffsetDb reads exactly 0.0 for all 24 bands, fresh and after a 1 s render (arithmetic: designer-harness EN-dyn)",
                 tally.detail() + (acts ? "" : "  (the render did not act on the audio)"));
        }
    }

    //==========================================================================
    // X. CPU, settled (INFO; PLAN-2.2 P15)
    //==========================================================================
    void cpuInfo()
    {
        section ("X. CPU, settled (INFO, not gated)");

        constexpr int blockSize = 512;
        const double rate = 48000.0;
        const int numBlocks = (int) (20.0 * rate / (double) blockSize);
        const double audioSeconds = (double) numBlocks * (double) blockSize / rate;
        const auto noise = makeNoiseAt (2, blockSize, 0x0C9000002, kNoise12);

        struct Config { const char* name; int numBands; int type; int slope; float gain; const char* beside; };

        // Settings as research-2.2 engine_bench modeCpu.
        const Config configs[] =
        {
            { "0 bands",                0,  kBell,      1, 0.0f, "no bench figure" },
            { "24 cuts x 96 dB/oct",    24, kHighCut,   7, 0.0f, "bench 2.4 sample-major; budget 10" },
            { "24 shelves x 96 dB/oct", 24, kHighShelf, 7, 6.0f, "bench 4.7 sample-major; budget 14" },
            { "24 bells",               24, kBell,      1, 6.0f, "bench 0.70 sample-major" },
            { "1 band",                 1,  kBell,      1, 6.0f, "no bench figure" }
        };

        for (const auto& c : configs)
        {
            auto p = makeBands (2, c.numBands, [&c] (int index)
            {
                const float freq = c.type == kHighCut ? 20000.0f - 300.0f * (float) index
                                 : c.numBands > 1     ? 30.0f * std::pow (500.0f, (float) index / 23.0f)
                                                      : 1000.0f;
                return makeSpec (c.type, freq, c.gain, 1.0f, c.slope, kStereo);
            });

            juce::AudioBuffer<float> block (2, blockSize);
            juce::MidiBuffer midi;
            double seconds = 0.0;

            p->prepareToPlay (rate, blockSize);

            for (int k = 0; k < numBlocks; ++k)
            {
                for (int channel = 0; channel < 2; ++channel)
                    block.copyFrom (channel, 0, noise, channel, 0, blockSize);

                const auto t0 = std::chrono::steady_clock::now();
                p->processBlock (block, midi);
                const auto t1 = std::chrono::steady_clock::now();

                seconds += std::chrono::duration<double> (t1 - t0).count();
            }

            p->releaseResources();

            info (juce::String ("X-cpu 48 kHz stereo, 512-sample blocks, 20 s, ") + c.name + ": "
                      + fmtG (100.0 * seconds / audioSeconds, 3) + " % of one core (" + c.beside + ")"
                      + (allFinite (block) ? "" : "  OUTPUT NOT FINITE"));
        }
    }

    //==========================================================================
    //==========================================================================
    // Stage 2, Phase 2.3: continuous modulation through the plugin (PLAN-2.3 T7)
    //==========================================================================
    //==========================================================================

    // --long: S5 runs the contract's 600 s per rate instead of 60 s (PLAN-2.3 P13).
    bool longRunRequested = false;

    //==========================================================================
    // Raw parameter values, as the processor reads them
    //==========================================================================
    struct BandHandles
    {
        std::atomic<float>* enabled = nullptr;
        std::atomic<float>* bypass = nullptr;
        std::atomic<float>* type = nullptr;
        std::atomic<float>* freq = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* q = nullptr;
        std::atomic<float>* slope = nullptr;
        std::atomic<float>* placement = nullptr;
    };

    /** The raw values of one instance, looked up once on the main thread. A
        mid-render edit of the 2.3 rows is a store to one of these: it is what
        the processor reads at the next block start, it allocates nothing and
        a worker thread may do it. Values stored are legal ones (in range;
        choices and toggles as whole numbers). */
    struct Handles
    {
        std::atomic<float>* bypass = nullptr;
        std::atomic<float>* gainScale = nullptr;
        std::atomic<float>* outputGain = nullptr;      // Phase 2.5 rows (and the amended S5)
        std::atomic<float>* autoGain = nullptr;        // Phase 2.5 rows (and the amended S5)
        std::array<BandHandles, (size_t) Processor::kNumBands> band;
    };

    Handles handlesOf (Processor& p)
    {
        Handles h;
        bool complete = true;

        const auto fetch = [&p, &complete] (const juce::String& id)
        {
            std::atomic<float>* const value = p.parameters.getRawParameterValue (id);
            complete = complete && value != nullptr;
            return value;
        };

        h.bypass = fetch ("bypass");
        h.gainScale = fetch ("gain_scale");
        h.outputGain = fetch ("output_gain");
        h.autoGain = fetch ("auto_gain");

        for (int b = 0; b < Processor::kNumBands; ++b)
        {
            BandHandles& d = h.band[(size_t) b];
            d.enabled   = fetch (bandId (b + 1, "enabled"));
            d.bypass    = fetch (bandId (b + 1, "bypass"));
            d.type      = fetch (bandId (b + 1, "type"));
            d.freq      = fetch (bandId (b + 1, "freq"));
            d.gain      = fetch (bandId (b + 1, "gain"));
            d.q         = fetch (bandId (b + 1, "q"));
            d.slope     = fetch (bandId (b + 1, "slope"));
            d.placement = fetch (bandId (b + 1, "placement"));
        }

        if (! complete)
        {
            std::printf ("FATAL: a raw parameter value is missing\n");
            std::exit (255);
        }

        return h;
    }

    eq::RawBand rawOfHandles (const BandHandles& h)
    {
        eq::RawBand raw;
        raw.enabled   = h.enabled->load();
        raw.bypass    = h.bypass->load();
        raw.type      = h.type->load();
        raw.freq      = h.freq->load();
        raw.gain      = h.gain->load();
        raw.q         = h.q->load();
        raw.slope     = h.slope->load();
        raw.placement = h.placement->load();
        return raw;
    }

    //==========================================================================
    // Workers
    //==========================================================================
    /** Runs job (0) ... job (count - 1) on up to 12 threads, this one
        included. A job owns everything it writes: its own plugin instance
        (constructed beforehand, on the main thread) and its own result slot.
        Jobs call prepareToPlay / processBlock, store raw parameter values and
        run harness code. They print nothing and touch no tally. */
    template <typename JobFn>
    void parallelFor (int count, JobFn&& job)
    {
        if (count <= 0)
            return;

        const int hardware = (int) std::thread::hardware_concurrency();
        const int workers = juce::jlimit (1, 12, juce::jmin (count, hardware > 0 ? hardware : 1));
        std::atomic<int> next { 0 };

        const auto work = [&next, &job, count]()
        {
            for (;;)
            {
                const int mine = next.fetch_add (1);

                if (mine >= count)
                    return;

                job (mine);
            }
        };

        std::vector<std::thread> pool;

        for (int w = 1; w < workers; ++w)
            pool.emplace_back (work);

        work();

        for (auto& thread : pool)
            thread.join();
    }

    //==========================================================================
    // Rendering for the 2.3 rows
    //==========================================================================
    /** render() with the block length from sizeAt (index, position) and a
        hook before each block (raw parameter stores; the entry point). The
        instance is prepared here at `prepared`, which is also the longest
        block. No std::function, so a worker thread may run it. It does not
        release the instance: M4 prepares again on top of a render. */
    template <typename SizeFn, typename HookFn>
    juce::AudioBuffer<float> renderWith (juce::AudioProcessor& p, const juce::AudioBuffer<float>& input, double rate,
                                         int prepared, SizeFn&& sizeAt, HookFn&& hook)
    {
        const int numChannels = input.getNumChannels();
        const int length = input.getNumSamples();

        juce::AudioBuffer<float> output (numChannels, length);
        output.clear();
        juce::AudioBuffer<float> block (numChannels, prepared);
        juce::MidiBuffer midi;

        p.prepareToPlay (rate, prepared);

        int index = 0;

        for (int position = 0; position < length; ++index)
        {
            const int n = juce::jlimit (1, juce::jmin (prepared, length - position), (int) sizeAt (index, position));

            BlockInfo current;
            current.index = index;
            current.position = position;
            current.viaBypassed = false;

            hook (current);

            block.setSize (numChannels, n, false, false, true);

            for (int channel = 0; channel < numChannels; ++channel)
                block.copyFrom (channel, 0, input, channel, position, n);

            if (current.viaBypassed)
                p.processBlockBypassed (block, midi);
            else
                p.processBlock (block, midi);

            for (int channel = 0; channel < numChannels; ++channel)
                output.copyFrom (channel, position, block, channel, 0, n);

            position += n;
        }

        return output;
    }

    std::vector<float> channelOf (const juce::AudioBuffer<float>& buffer, int channel)
    {
        const float* data = buffer.getReadPointer (channel);
        return std::vector<float> (data, data + buffer.getNumSamples());
    }

    std::vector<double> widened (const juce::AudioBuffer<float>& buffer, int channel)
    {
        const float* data = buffer.getReadPointer (channel);
        std::vector<double> out ((size_t) buffer.getNumSamples());

        for (size_t i = 0; i < out.size(); ++i)
            out[i] = (double) data[i];

        return out;
    }

    // First sample index at which two renders differ on any channel, by bit
    // pattern; -1 when they are the same. A shape mismatch reads 0.
    int firstDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return 0;

        for (int i = 0; i < a.getNumSamples(); ++i)
            for (int channel = 0; channel < a.getNumChannels(); ++channel)
                if (! sameBits (a.getReadPointer (channel)[i], b.getReadPointer (channel)[i]))
                    return i;

        return -1;
    }

    int firstDifference (const std::vector<double>& a, const std::vector<double>& b)
    {
        if (a.size() != b.size())
            return 0;

        for (size_t i = 0; i < a.size(); ++i)
            if (std::memcmp (&a[i], &b[i], sizeof (double)) != 0)
                return (int) i;

        return -1;
    }

    // Power of one channel at hz over [start, start + length): single bin.
    double binPower (const juce::AudioBuffer<float>& buffer, int channel, int start, int length, double hz, double rate)
    {
        const float* data = buffer.getReadPointer (channel);
        double re = 0.0, im = 0.0;

        for (int i = 0; i < length; ++i)
        {
            const double phase = 2.0 * eq::pi * hz * (double) (start + i) / rate;
            const double v = (double) data[start + i];
            re += v * std::cos (phase);
            im -= v * std::sin (phase);
        }

        return re * re + im * im;
    }

    // The same for the component 0.5 (L + sign R) of a stereo buffer
    // (sign +1: mid; -1: side).
    double componentPower (const juce::AudioBuffer<float>& buffer, double sign, int start, int length, double hz, double rate)
    {
        const float* left = buffer.getReadPointer (0);
        const float* right = buffer.getReadPointer (1);
        double re = 0.0, im = 0.0;

        for (int i = 0; i < length; ++i)
        {
            const double phase = 2.0 * eq::pi * hz * (double) (start + i) / rate;
            const double v = 0.5 * ((double) left[start + i] + sign * (double) right[start + i]);
            re += v * std::cos (phase);
            im -= v * std::sin (phase);
        }

        return re * re + im * im;
    }

    double ratioOf (double value, double reference)
    {
        return (reference > 0.0 && std::isfinite (value) && std::isfinite (reference))
                   ? value / reference
                   : std::numeric_limits<double>::infinity();
    }

    //==========================================================================
    // Local kappa (gate G3; ported from research-2.3 ramp_bench localKappa):
    // the largest, over 5 ms frames, of max |second difference| / frame RMS.
    // Frames below -80 dBFS RMS are skipped.
    //==========================================================================
    double localKappa (const std::vector<double>& y, double rate, size_t from, size_t to)
    {
        const auto frame = (size_t) std::lround (0.005 * rate);

        if (y.size() < 3 || frame == 0)
            return 0.0;

        const size_t end = juce::jmin (to, y.size() - 1);
        double worst = 0.0;

        for (size_t a = juce::jmax (from, (size_t) 1); a + frame < end; a += frame)
        {
            double energy = 0.0, d2 = 0.0;

            for (size_t i = a; i < a + frame; ++i)
            {
                energy += y[i] * y[i];
                d2 = juce::jmax (d2, std::abs (y[i + 1] - 2.0 * y[i] + y[i - 1]));
            }

            const double rms = std::sqrt (energy / (double) frame);

            if (rms < 1.0e-4)
                continue;

            worst = juce::jmax (worst, d2 / rms);
        }

        return worst;
    }

    //==========================================================================
    // The control grid, re-stated in harness code (PLAN-2.3 P1, P11). The
    // reference below takes these two numbers as arguments; M1 checks that
    // they are the engine's at all eight rates.
    //==========================================================================
    int harnessGrid (double rate)
    {
        return rate <= 48000.0 ? 16 : (rate <= 96000.0 ? 32 : 64);
    }

    int harnessRampTicks (double rate)
    {
        return juce::jmax (1, (int) std::lround (0.020 * rate / (double) harnessGrid (rate)));
    }

    //==========================================================================
    // References (PLAN-2.3 P11). They share no code with the engine's ramp.
    //==========================================================================
    /** What a host block start reads, block by block: the settings of each
        band in series (band 0 first), on one lane. blockStart[0] is 0. */
    struct RefScript
    {
        std::vector<int> blockStart;
        std::vector<std::vector<eq::BandSettings>> settings;    // [block][band]
    };

    bool sameSettings (const eq::BandSettings& a, const eq::BandSettings& b)
    {
        return a.type == b.type && a.slopeIndex == b.slopeIndex
            && eq::sameValue (a.freqHz, b.freqHz)
            && eq::sameValue (a.gainDbEff, b.gainDbEff)
            && eq::sameValue (a.q, b.q);
    }

    double clampRange (double v, double lo, double hi)
    {
        return (v >= lo) ? ((v <= hi) ? v : hi) : lo;
    }

    /** The same-motion reference: the specified control trajectory, then a
        design on EVERY sample and the settled recurrences.

        The trajectory, as PLAN-2.3 P2, P3 and P8 state it:
          - three values per band: ln f, effective gain in dB, ln Q;
          - settings seen at a block start that differ from the ones the ramp
            is heading for wait for the next grid tick (a tick is a sample
            where the clock is a multiple of `grid`; a block that starts on a
            tick is served by that tick);
          - at that tick the ramp restarts from its current value: per-tick
            steps (goal - current) / rampTicks, rampTicks ticks to go. Each
            tick while running moves the value by one step; the last one
            assigns the goal itself;
          - a tick's value is the END of the grid interval that starts there
            (the one-interval lag): inside the interval the control values
            run linearly from the previous tick's value, sample j of the
            interval (0 ... grid - 1) at (j + 1) / grid of the way;
          - while moving, the design settings are exp (ln f), the gain and
            exp (ln Q), each clamped to its range; the sample that ends the
            last interval, and every sample after it, is designed from the
            goal's own settings.

        The clock counts from clockAtStart at input[0]. One lane, bands in
        series, double throughout. */
    std::vector<double> refRender (const std::vector<float>& input, double rate, int grid, int rampTicks,
                                   const RefScript& script, std::uint64_t clockAtStart = 0)
    {
        struct Track
        {
            eq::BandSettings goal {};
            eq::BandSettings waiting {};
            bool hasWaiting = false;

            double lnF = 0.0, gainDb = 0.0, lnQ = 0.0;                  // at the latest tick
            double prevLnF = 0.0, prevGainDb = 0.0, prevLnQ = 0.0;      // at the tick before it
            double goalLnF = 0.0, goalGainDb = 0.0, goalLnQ = 0.0;
            double stepLnF = 0.0, stepGainDb = 0.0, stepLnQ = 0.0;      // per tick
            int ticksLeft = 0;
            bool moving = false;        // this interval runs between two tick values
            bool endsOnGoal = false;    // and its end is the goal itself

            double ic1[eq::maxSections] = {};
            double ic2[eq::maxSections] = {};
            double s[eq::maxSections] = {};
        };

        if (grid < 1 || rampTicks < 1 || script.blockStart.empty()
            || script.blockStart.size() != script.settings.size() || script.blockStart[0] != 0)
            return {};

        const size_t numBands = script.settings[0].size();

        for (const auto& perBlock : script.settings)
            if (perBlock.size() != numBands)
                return {};

        const auto designer = designerFor (rate);
        std::vector<Track> tracks (numBands);
        std::vector<double> out (input.size(), 0.0);

        const auto gridLength = (std::uint64_t) grid;
        const double perSample = 1.0 / (double) grid;
        const double ticks = (double) rampTicks;
        size_t block = 0;

        for (size_t n = 0; n < input.size(); ++n)
        {
            // A host block starts here: the parameters are read.
            while (block < script.blockStart.size() && (size_t) script.blockStart[block] == n)
            {
                for (size_t b = 0; b < numBands; ++b)
                {
                    Track& t = tracks[b];
                    const eq::BandSettings& seen = script.settings[block][b];

                    if (block == 0)
                    {
                        t.goal = seen;
                        t.goalLnF = std::log (clampRange (seen.freqHz, eq::rawFreqMin, eq::rawFreqMax));
                        t.goalGainDb = clampRange (seen.gainDbEff, eq::gainEffMin, eq::gainEffMax);
                        t.goalLnQ = std::log (clampRange (seen.q, eq::rawQMin, eq::rawQMax));
                        t.lnF = t.goalLnF;
                        t.gainDb = t.goalGainDb;
                        t.lnQ = t.goalLnQ;
                        t.prevLnF = t.lnF;
                        t.prevGainDb = t.gainDb;
                        t.prevLnQ = t.lnQ;
                    }
                    else if (sameSettings (seen, t.goal))
                    {
                        t.hasWaiting = false;
                    }
                    else
                    {
                        t.waiting = seen;
                        t.hasWaiting = true;
                    }
                }

                ++block;
            }

            const int phase = (int) ((clockAtStart + (std::uint64_t) n) % gridLength);

            // A grid tick.
            if (phase == 0)
            {
                for (auto& t : tracks)
                {
                    t.prevLnF = t.lnF;
                    t.prevGainDb = t.gainDb;
                    t.prevLnQ = t.lnQ;

                    if (t.hasWaiting)
                    {
                        t.goal = t.waiting;
                        t.hasWaiting = false;

                        t.goalLnF = std::log (clampRange (t.goal.freqHz, eq::rawFreqMin, eq::rawFreqMax));
                        t.goalGainDb = clampRange (t.goal.gainDbEff, eq::gainEffMin, eq::gainEffMax);
                        t.goalLnQ = std::log (clampRange (t.goal.q, eq::rawQMin, eq::rawQMax));

                        t.stepLnF = (t.goalLnF - t.lnF) / ticks;
                        t.stepGainDb = (t.goalGainDb - t.gainDb) / ticks;
                        t.stepLnQ = (t.goalLnQ - t.lnQ) / ticks;
                        t.ticksLeft = rampTicks;
                    }

                    if (t.ticksLeft > 1)
                    {
                        t.lnF += t.stepLnF;
                        t.gainDb += t.stepGainDb;
                        t.lnQ += t.stepLnQ;
                        --t.ticksLeft;
                        t.moving = true;
                        t.endsOnGoal = false;
                    }
                    else if (t.ticksLeft == 1)
                    {
                        t.lnF = t.goalLnF;
                        t.gainDb = t.goalGainDb;
                        t.lnQ = t.goalLnQ;
                        t.ticksLeft = 0;
                        t.moving = true;
                        t.endsOnGoal = true;
                    }
                    else
                    {
                        t.moving = false;
                        t.endsOnGoal = false;
                    }
                }
            }

            double v = (double) input[n];
            const bool lastOfInterval = phase == grid - 1;

            for (auto& t : tracks)
            {
                eq::BandSettings now = t.goal;

                if (t.moving && ! (t.endsOnGoal && lastOfInterval))
                {
                    double lnF = t.lnF, gainDb = t.gainDb, lnQ = t.lnQ;

                    if (! lastOfInterval)
                    {
                        const double u = (double) (phase + 1) * perSample;
                        lnF = t.prevLnF + u * (t.lnF - t.prevLnF);
                        gainDb = t.prevGainDb + u * (t.gainDb - t.prevGainDb);
                        lnQ = t.prevLnQ + u * (t.lnQ - t.prevLnQ);
                    }

                    now.freqHz = clampRange (std::exp (lnF), eq::rawFreqMin, eq::rawFreqMax);
                    now.gainDbEff = clampRange (gainDb, eq::gainEffMin, eq::gainEffMax);
                    now.q = clampRange (std::exp (lnQ), eq::rawQMin, eq::rawQMax);
                }

                eq::BandDesign design {};
                designer->design (now, design);

                const int count = juce::jlimit (0, (int) eq::maxSections, design.numSections);

                for (int i = 0; i < count; ++i)
                {
                    const eq::SectionRuntime runtime = eq::toRuntime (design.params[(size_t) i]);

                    if (runtime.hasSvf)
                        v = eq::svfTick (runtime.svf, t.ic1[i], t.ic2[i], v);

                    if (runtime.hasOnePole)
                        v = eq::onePoleTick (runtime.onePole, t.s[i], v);
                }
            }

            out[n] = v;
        }

        return out;
    }

    /** The no-smoothing control: the Phase 2.2 rule. The settings a block
        start reads are designed there and held for the block; the filter
        state is kept. One lane, bands in series, double throughout. */
    std::vector<double> hardRender (const std::vector<float>& input, double rate, const RefScript& script)
    {
        struct Stage
        {
            eq::BandSettings held {};
            bool designed = false;
            int numSections = 0;
            eq::SectionRuntime runtime[eq::maxSections];

            double ic1[eq::maxSections] = {};
            double ic2[eq::maxSections] = {};
            double s[eq::maxSections] = {};
        };

        if (script.blockStart.empty() || script.blockStart.size() != script.settings.size()
            || script.blockStart[0] != 0)
            return {};

        const size_t numBands = script.settings[0].size();

        for (const auto& perBlock : script.settings)
            if (perBlock.size() != numBands)
                return {};

        const auto designer = designerFor (rate);
        std::vector<Stage> stages (numBands);
        std::vector<double> out (input.size(), 0.0);
        size_t block = 0;

        for (size_t n = 0; n < input.size(); ++n)
        {
            while (block < script.blockStart.size() && (size_t) script.blockStart[block] == n)
            {
                for (size_t b = 0; b < numBands; ++b)
                {
                    Stage& st = stages[b];
                    const eq::BandSettings& seen = script.settings[block][b];

                    if (st.designed && sameSettings (seen, st.held))
                        continue;

                    eq::BandDesign design {};
                    designer->design (seen, design);

                    st.numSections = juce::jlimit (0, (int) eq::maxSections, design.numSections);

                    for (int i = 0; i < st.numSections; ++i)
                        st.runtime[i] = eq::toRuntime (design.params[(size_t) i]);

                    st.held = seen;
                    st.designed = true;
                }

                ++block;
            }

            double v = (double) input[n];

            for (auto& st : stages)
                for (int i = 0; i < st.numSections; ++i)
                {
                    if (st.runtime[i].hasSvf)
                        v = eq::svfTick (st.runtime[i].svf, st.ic1[i], st.ic2[i], v);

                    if (st.runtime[i].hasOnePole)
                        v = eq::onePoleTick (st.runtime[i].onePole, st.s[i], v);
                }

            out[n] = v;
        }

        return out;
    }

    //==========================================================================
    // M1, M1h, M1k. G3 sweep rows (PLAN-2.3 P12; ported from ramp_bench g3)
    //==========================================================================
    struct Motion
    {
        const char* name;
        int number;             // 1 ... 13, as PLAN-2.3 P12 numbers them
        int type, slope;
        double fA, fB, gA, gB, qA, qB;
        double sweepSeconds;
        int pattern;            // 0: A | sweep | hold | back | hold.  1: there and back with no hold, twice
    };

    std::vector<Motion> motionsFor (double tone)
    {
        return {
            { "Bell +18 dB Q 1, 10 Hz <-> 30 kHz in 50 ms, p0",             1,  kBell,      1, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 0 },
            { "Bell +18 dB Q 1, 10 Hz <-> 30 kHz in 50 ms, p1",             2,  kBell,      1, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 1 },
            { "Bell at the tone Q 1, -30 <-> +30 dB in 20 ms",              3,  kBell,      1, tone, tone, -30.0, 30.0, 1.0, 1.0, 0.020, 0 },
            { "Bell +18 dB at 2x the tone, Q 0.025 <-> 40 in 20 ms",        4,  kBell,      1, 2.0 * tone, 2.0 * tone, 18.0, 18.0, 0.025, 40.0, 0.020, 0 },
            { "Low Cut 96 dB/oct Q 1, 10 Hz <-> 30 kHz in 50 ms, p0",       5,  kLowCut,    7, 10.0, 30000.0, 0.0, 0.0, 1.0, 1.0, 0.050, 0 },
            { "Low Cut 96 dB/oct Q 1, 10 Hz <-> 30 kHz in 50 ms, p1",       6,  kLowCut,    7, 10.0, 30000.0, 0.0, 0.0, 1.0, 1.0, 0.050, 1 },
            { "Low Cut 96 dB/oct at 30 Hz, Q 0.025 <-> 40 in 20 ms",        7,  kLowCut,    7, 30.0, 30.0, 0.0, 0.0, 0.025, 40.0, 0.020, 0 },
            { "High Shelf 12 dB/oct +18 dB, 10 Hz <-> 30 kHz in 50 ms, p0", 8,  kHighShelf, 1, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 0 },
            { "High Shelf 12 dB/oct +18 dB, 10 Hz <-> 30 kHz in 50 ms, p1", 9,  kHighShelf, 1, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 1 },
            { "High Shelf 12 dB/oct at tone / 2, -30 <-> +30 dB in 20 ms",  10, kHighShelf, 1, 0.5 * tone, 0.5 * tone, -30.0, 30.0, 1.0, 1.0, 0.020, 0 },
            { "High Shelf 12 dB/oct +18 dB at the tone, Q 0.025 <-> 40",    11, kHighShelf, 1, tone, tone, 18.0, 18.0, 0.025, 40.0, 0.020, 0 },
            { "High Shelf 96 dB/oct +18 dB, 10 Hz <-> 30 kHz in 50 ms, p0", 12, kHighShelf, 7, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 0 },
            { "High Shelf 96 dB/oct +18 dB, 10 Hz <-> 30 kHz in 50 ms, p1", 13, kHighShelf, 7, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 1 }
        };
    }

    // Where the parameter stands at `seconds`: 0 at A, 1 at B.
    double motionPhase (const Motion& m, double seconds)
    {
        const double lead = 0.5;
        const double sweep = m.sweepSeconds;

        if (seconds < lead)
            return 0.0;

        double t = seconds - lead;

        if (m.pattern == 0)
        {
            if (t < sweep)
                return t / sweep;

            t -= sweep;

            if (t < 0.4)
                return 1.0;

            t -= 0.4;

            if (t < sweep)
                return 1.0 - t / sweep;

            return 0.0;
        }

        t = std::fmod (t, sweep + 0.5);

        if (t < 0.5 * sweep)
            return t / (0.5 * sweep);

        if (t < sweep)
            return 2.0 - t / (0.5 * sweep);

        return 0.0;
    }

    double motionSeconds (const Motion& m)
    {
        return m.pattern == 0 ? 0.5 + m.sweepSeconds + 0.4 + m.sweepSeconds + 0.4
                              : 0.5 + 2.0 * (m.sweepSeconds + 0.5);
    }

    struct SweepJob
    {
        Motion motion;
        double tone;
        int blockSize;
        double rate;
        bool withHard;
    };

    struct SweepReading
    {
        bool valid = false;         // renders complete and finite, reference kappa above 0
        double pluginKappa = 0.0;
        double refKappa = 0.0;
        double hardKappa = 0.0;
        bool hasHard = false;
    };

    /** One reading: a sine at -6 dBFS through one band of a mono instance,
        the moved parameter written at every block start (read once per block,
        as the processor does); the first 0.3 s is not scored. The reference
        and the control get the same float input and the settings
        eq::makeBandInput makes of the same raw values. */
    SweepReading runSweep (Processor& p, const Handles& h, const SweepJob& job)
    {
        const Motion& m = job.motion;
        const double rate = job.rate;
        const int blockSize = job.blockSize;
        const int length = (int) std::lround (motionSeconds (m) * rate);
        const int numBlocks = (length + blockSize - 1) / blockSize;
        const BandHandles& hb = h.band[0];

        juce::AudioBuffer<float> input (1, length);

        {
            float* x = input.getWritePointer (0);

            for (int i = 0; i < length; ++i)
                x[i] = (float) (0.5 * std::sin (2.0 * eq::pi * job.tone * (double) i / rate));
        }

        std::vector<float> freqs ((size_t) numBlocks), gains ((size_t) numBlocks), qs ((size_t) numBlocks);
        RefScript script;

        const float scale = h.gainScale->load();
        eq::RawBand raw = rawOfHandles (hb);

        const double lnFA = std::log (m.fA), lnFB = std::log (m.fB);
        const double lnQA = std::log (m.qA), lnQB = std::log (m.qB);

        for (int k = 0; k < numBlocks; ++k)
        {
            const auto at = (size_t) k;
            const double u = motionPhase (m, (double) (k * blockSize) / rate);

            freqs[at] = juce::jlimit (10.0f, 30000.0f, (float) std::exp (lnFA + u * (lnFB - lnFA)));
            gains[at] = juce::jlimit (-30.0f, 30.0f, (float) (m.gA + u * (m.gB - m.gA)));
            qs[at] = juce::jlimit (0.025f, 40.0f, (float) std::exp (lnQA + u * (lnQB - lnQA)));

            raw.freq = freqs[at];
            raw.gain = gains[at];
            raw.q = qs[at];

            script.blockStart.push_back (k * blockSize);
            script.settings.push_back ({ eq::makeBandInput (raw, scale, 0.0).settings });
        }

        const auto output = renderWith (p, input, rate, blockSize,
                                        [blockSize] (int, int) { return blockSize; },
                                        [&hb, &freqs, &gains, &qs] (BlockInfo& current)
                                        {
                                            const auto slot = (size_t) current.index;

                                            if (slot < freqs.size())
                                            {
                                                hb.freq->store (freqs[slot]);
                                                hb.gain->store (gains[slot]);
                                                hb.q->store (qs[slot]);
                                            }
                                        });

        const auto from = (size_t) std::lround (0.3 * rate);
        const auto source = channelOf (input, 0);
        const auto yPlugin = widened (output, 0);
        const auto yRef = refRender (source, rate, harnessGrid (rate), harnessRampTicks (rate), script);

        SweepReading r;
        r.pluginKappa = localKappa (yPlugin, rate, from, yPlugin.size());
        r.refKappa = localKappa (yRef, rate, from, yRef.size());

        bool hardOk = true;

        if (job.withHard)
        {
            const auto yHard = hardRender (source, rate, script);
            r.hardKappa = localKappa (yHard, rate, from, yHard.size());
            r.hasHard = true;
            hardOk = yHard.size() == yPlugin.size() && std::isfinite (r.hardKappa);
        }

        r.valid = allFinite (output) && yRef.size() == yPlugin.size() && hardOk
               && std::isfinite (r.pluginKappa) && std::isfinite (r.refKappa) && r.refKappa > 0.0;
        return r;
    }

    juce::String sweepLabel (const SweepJob& job)
    {
        return "#" + juce::String (job.motion.number) + " " + job.motion.name + ", " + fmtG (job.tone, 4)
             + " Hz, block " + juce::String (job.blockSize);
    }

    // The tick-clock script shared by TK (the plugin) and M1's guard (the
    // reference): idle to sample 1000, a 0 dB bell from there, one more block
    // boundary, and a 0 -> +24 dB gain edit seen at the block that starts on
    // it. Expected first sample that differs from the unedited render, at
    // 48 kHz (K = 16), counted from prepareToPlay.
    struct TickCase
    {
        int boundary;
        int expected;
    };

    constexpr TickCase kTickCases[3] = { { 1036, 1040 }, { 1041, 1056 }, { 1024, 1024 } };

    // TK's expected first differing sample through the plugin, per kTickCases
    // entry. Amended in Phase 2.4 by Taylor's call (execute session
    // 2026-11-14): the 0 dB bell sleeps; the wake fade starts at 1024 (gated
    // exactly in double by XF-wake) but w(0), w(1) times one ramp step are
    // below float resolution, so the float output first differs at 1026.
    // Kept apart from kTickCases, whose `expected` M1's reference guard also
    // reads (the reference has no sleep and still acts at 1024).
    constexpr int kTickExpectedPlugin[3] = { 1040, 1056, 1026 };
    constexpr int kTickActiveFrom = 1000;
    constexpr int kTickLength = 4096;

    // refRender on that script (the part from sample 1000 on): the index,
    // counted from prepareToPlay, of the first sample the edit changes.
    int referenceStartIndex (int boundary)
    {
        const double rate = 48000.0;
        const int length = kTickLength - kTickActiveFrom;
        const auto noise = makeNoiseAt (1, length, 0x7C4000 + boundary, kNoise12);

        eq::RawBand raw;
        raw.enabled = 1.0f;
        raw.bypass = 0.0f;
        raw.type = (float) kBell;
        raw.freq = 1000.0f;
        raw.gain = 0.0f;
        raw.q = 1.0f;
        raw.slope = 1.0f;
        raw.placement = (float) kStereo;

        const eq::BandSettings flat = eq::makeBandInput (raw, 100.0f, 0.0).settings;
        raw.gain = 24.0f;
        const eq::BandSettings boosted = eq::makeBandInput (raw, 100.0f, 0.0).settings;

        RefScript plain, edited;
        int position = 0;

        for (int index = 0; position < length; ++index)
        {
            plain.blockStart.push_back (position);
            plain.settings.push_back ({ flat });
            edited.blockStart.push_back (position);
            edited.settings.push_back ({ index == 0 ? flat : boosted });

            position += index == 0 ? boundary - kTickActiveFrom : 64;
        }

        const auto source = channelOf (noise, 0);
        const auto grid = harnessGrid (rate);
        const auto ticks = harnessRampTicks (rate);
        const auto a = refRender (source, rate, grid, ticks, plain, (std::uint64_t) kTickActiveFrom);
        const auto b = refRender (source, rate, grid, ticks, edited, (std::uint64_t) kTickActiveFrom);

        const int first = (a.size() == source.size() && ! a.empty()) ? firstDifference (a, b) : -1;
        return first < 0 ? -1 : first + kTickActiveFrom;
    }

    void sweepRows()
    {
        section ("M1. G3 sweep rows through processBlock (Phase 2.3: DSP-04; PLAN-2.3 P11, P12)");

        // Guards on the reference itself (PLAN-2.3 R3): its grid and ramp
        // length are the engine's at every rate, and its "a target seen at a
        // block start acts at the next tick" rule lands on the plan's sample
        // (TK asserts the same three indices on the plugin).
        Tally guard;

        for (const double fs : { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
            guard.check (harnessGrid (fs) == eq::controlGridSize (fs) && harnessRampTicks (fs) == eq::rampLengthTicks (fs),
                         "harness grid / ramp length differ from the engine's at " + fmtG (fs, 6) + " Hz");

        juce::String starts;

        for (const auto& c : kTickCases)
        {
            const int at = referenceStartIndex (c.boundary);
            guard.check (at == c.expected, "reference: edit seen at " + juce::String (c.boundary) + " acts at "
                                               + juce::String (at) + ", expected " + juce::String (c.expected));
            starts += (starts.isEmpty() ? "" : " / ") + juce::String (at);
        }

        // 52 readings at 48 kHz, then motion 8 at 50 Hz, block 32, on the two other grids.
        const double tones[2] = { 50.0, 1000.0 };
        std::vector<SweepJob> jobs;

        for (const double tone : tones)
            for (const auto& m : motionsFor (tone))
                for (const int blockSize : { 32, 512 })
                    jobs.push_back ({ m, tone, blockSize, 48000.0, blockSize == 512 });

        const size_t numMain = jobs.size();

        for (const double fs : { 96000.0, 192000.0 })
            jobs.push_back ({ motionsFor (50.0)[7], 50.0, 32, fs, false });

        // Instances and settings on this thread; renders on the workers.
        std::vector<std::unique_ptr<Processor>> instances;
        std::vector<Handles> handles;

        for (const auto& job : jobs)
        {
            const Motion& m = job.motion;

            instances.push_back (makeBands (1, 1, [&m] (int)
            {
                return makeSpec (m.type, (float) m.fA, (float) m.gA, (float) m.qA, m.slope, kStereo);
            }));

            handles.push_back (handlesOf (*instances.back()));
        }

        std::vector<SweepReading> readings (jobs.size());

        parallelFor ((int) jobs.size(), [&instances, &handles, &jobs, &readings] (int j)
        {
            const auto slot = (size_t) j;
            readings[slot] = runSweep (*instances[slot], handles[slot], jobs[slot]);
        });

        // M1
        Tally sweeps;
        double worstSweep = 0.0;
        juce::String worstSweepAt;

        for (size_t j = 0; j < numMain; ++j)
        {
            const SweepReading& r = readings[j];
            const double ratio = ratioOf (r.pluginKappa, r.refKappa);
            const juce::String label = sweepLabel (jobs[j]);

            std::printf ("      M1  %-88s plugin %.3e  reference %.3e  plugin / reference %.3f%s\n",
                         label.toRawUTF8(), r.pluginKappa, r.refKappa, ratio, r.valid ? "" : "  INVALID");

            sweeps.check (r.valid && ratio <= 4.0, label + ": " + fmtG (ratio, 4) + "x");

            if (! (ratio <= worstSweep))
            {
                worstSweep = ratio;
                worstSweepAt = label;
            }
        }

        row (sweeps.bad == 0 && sweeps.total == 52 && guard.bad == 0 && guard.total == 11,
             "M1 13 motions x 50 Hz / 1 kHz x blocks 32 / 512, 48 kHz: local kappa of the float output <= 4.0x the same-motion per-sample reference",
             sweeps.detail() + "  worst " + fmtG (worstSweep, 4) + "x (" + worstSweepAt + "); reference guards " + guard.detail()
                 + " (reference first acts at " + starts + ", expected 1040 / 1056 / 1024)");

        // M1h: the no-smoothing control, at block 512 (at block 32 the 2.2
        // staircase is already a fine grid; RESEARCH-2.3 Q1).
        Tally hard;
        int voids = 0;
        double weakestHard = std::numeric_limits<double>::infinity();
        juce::String voidFigures;

        for (size_t j = 0; j < numMain; ++j)
        {
            const SweepReading& r = readings[j];

            if (! r.hasHard)
                continue;

            const SweepJob& job = jobs[j];
            const double ratio = ratioOf (r.hardKappa, r.refKappa);
            const juce::String label = sweepLabel (job);

            // Void by construction (ARCHITECTURE G3, PLAN-2.3 P12): printed, not counted.
            const bool lowTone = job.tone < 100.0;
            const bool isVoid = (job.motion.number == 5 && lowTone) || (job.motion.number == 7 && ! lowTone);

            std::printf ("      M1h %-88s hard %.3e  reference %.3e  hard / reference %.2f%s%s\n",
                         label.toRawUTF8(), r.hardKappa, r.refKappa, ratio, r.valid ? "" : "  INVALID",
                         isVoid ? (lowTone ? "  VOID, not counted (the tone dies during the sweep)"
                                           : "  VOID, not counted (the Q of a 30 Hz cut barely moves a 1 kHz tone)")
                                : "");

            if (isVoid)
            {
                ++voids;
                voidFigures += (voidFigures.isEmpty() ? "" : "; ") + juce::String ("motion ") + juce::String (job.motion.number)
                             + " at " + fmtG (job.tone, 4) + " Hz reads " + fmtG (ratio, 3) + "x";
                continue;
            }

            hard.check (r.valid && ratio >= 10.0, label + ": " + fmtG (ratio, 4) + "x");
            weakestHard = juce::jmin (weakestHard, ratio);
        }

        row (hard.bad == 0 && hard.total == 24 && voids == 2,
             "M1h no smoothing (hardRender, block 512, the plugin's input and settings) reads >= 10x the same-motion reference (control fires)",
             hard.detail() + "  weakest " + fmtG (weakestHard, 4) + "x; void, printed and not counted: " + voidFigures);

        // M1k
        Tally grids;
        juce::String gridFigures;

        for (size_t j = numMain; j < jobs.size(); ++j)
        {
            const SweepReading& r = readings[j];
            const double ratio = ratioOf (r.pluginKappa, r.refKappa);
            const juce::String label = sweepLabel (jobs[j]) + ", " + fmtG (jobs[j].rate / 1000.0, 4) + " kHz (K = "
                                     + juce::String (harnessGrid (jobs[j].rate)) + ")";

            std::printf ("      M1k %-88s plugin %.3e  reference %.3e  plugin / reference %.3f%s\n",
                         label.toRawUTF8(), r.pluginKappa, r.refKappa, ratio, r.valid ? "" : "  INVALID");

            grids.check (r.valid && ratio <= 4.0, label + ": " + fmtG (ratio, 4) + "x");
            gridFigures += (gridFigures.isEmpty() ? "" : "; ") + fmtG (jobs[j].rate / 1000.0, 4) + " kHz " + fmtG (ratio, 4) + "x";
        }

        row (grids.bad == 0 && grids.total == 2,
             "M1k motion 8 at 50 Hz, block 32, at 96 kHz (K = 32) and 192 kHz (K = 64): <= 4.0x the same-motion reference",
             grids.detail() + "  " + gridFigures);
    }

    //==========================================================================
    // M2 - M6. The gaps carried from VERIFICATION-2.2 (issues 1 - 4)
    //==========================================================================
    struct KeepFigures
    {
        double ownD2 = 0.0;     // the ring's largest second difference in the 50 ms before the edit
        double after = 0.0;     // largest |y[n] - (2 y[n-1] - y[n-2])| over the 20 ms from the edit
        double ringPeak = 0.0;  // the ring's peak in the 50 ms before the edit

        double ratio() const    { return ratioOf (after, ownD2); }
    };

    KeepFigures keepFigures (const juce::AudioBuffer<float>& out, int editAt, double rate)
    {
        KeepFigures f;
        const float* y = out.getReadPointer (0);
        const int span = (int) std::lround (0.020 * rate);
        const int lookBack = (int) std::lround (0.050 * rate);

        const auto secondDifference = [y] (int i)
        {
            return std::abs ((double) y[i + 1] - 2.0 * (double) y[i] + (double) y[i - 1]);
        };

        for (int i = editAt - lookBack; i + 1 < editAt; ++i)
        {
            f.ownD2 = juce::jmax (f.ownD2, secondDifference (i));
            f.ringPeak = juce::jmax (f.ringPeak, std::abs ((double) y[i]));
        }

        for (int n = editAt; n < editAt + span; ++n)
            f.after = juce::jmax (f.after, secondDifference (n - 1));

        return f;
    }

    // RM-keep's stimulus through the plugin: a bell at 40 Hz, Q 20, +24 dB,
    // mono, rung by a 10 ms burst; blocks of 480, so sample 24000 starts a
    // block (and is a tick). edit 0: none. 1: gain -> +18 dB. 2: Bell -> Low Shelf.
    constexpr int kKeepBlock = 480;
    constexpr int kKeepEditBlock = 50;

    juce::AudioBuffer<float> keepRender (const juce::AudioBuffer<float>& burst, double rate, int edit)
    {
        auto p = makeBands (1, 1, [] (int) { return makeSpec (kBell, 40.0f, 24.0f, 20.0f, 1, kStereo); });
        const Handles h = handlesOf (*p);
        const BandHandles& hb = h.band[0];

        return renderWith (*p, burst, rate, kKeepBlock,
                           [] (int, int) { return kKeepBlock; },
                           [&hb, edit] (BlockInfo& current)
                           {
                               if (current.index != kKeepEditBlock)
                                   return;

                               if (edit == 1)
                                   hb.gain->store (18.0f);

                               if (edit == 2)
                                   hb.type->store ((float) kLowShelf);
                           });
    }

    void carriedGapRows()
    {
        section ("M2 - M6. gain scale, band bypass, re-prepare, Mid / Side response, kept state (Phase 2.3; VERIFICATION-2.2 issues 1 - 4)");

        // M2: gain_scale 100 -> 25 on three Stereo bells, three sines at their centres.
        {
            constexpr int blockSize = 512, editBlock = 235, window = 4800;
            const double rate = 48000.0;
            const int editAt = editBlock * blockSize;               // 120320: a block start and a tick
            const int oneSecond = (int) std::lround (rate);
            const int total = editAt + oneSecond + window;
            const int numBlocks = (total + blockSize - 1) / blockSize;

            const double tones[3] = { 50.0, 500.0, 5000.0 };
            const float bellGains[3] = { 24.0f, 6.0f, 0.0f };
            const double nominalAfter[3] = { 6.0, 1.5, 0.0 };

            juce::AudioBuffer<float> input (2, total);

            for (int i = 0; i < total; ++i)
            {
                double v = 0.0;

                for (const double tone : tones)
                    v += 0.1 * std::sin (2.0 * eq::pi * tone * (double) i / rate);

                input.setSample (0, i, (float) v);
                input.setSample (1, i, (float) v);
            }

            auto p = makeBands (2, 3, [&tones, &bellGains] (int index)
            {
                return makeSpec (kBell, (float) tones[index], bellGains[index], 8.0f, 1, kStereo);
            });

            const Handles h = handlesOf (*p);
            const auto designer = designerFor (rate);
            const std::vector<double> at (std::begin (tones), std::end (tones));
            const auto closedBefore = chainRefDb (*p, *designer, rate, at);

            RefScript script;

            for (int k = 0; k < numBlocks; ++k)
            {
                const float scale = k < editBlock ? 100.0f : 25.0f;
                std::vector<eq::BandSettings> perBand;

                for (size_t b = 0; b < 3; ++b)
                    perBand.push_back (eq::makeBandInput (rawOfHandles (h.band[b]), scale, 0.0).settings);

                script.blockStart.push_back (k * blockSize);
                script.settings.push_back (perBand);
            }

            const auto output = renderWith (*p, input, rate, blockSize,
                                            [] (int, int) { return 512; },
                                            [&h] (BlockInfo& current)
                                            {
                                                if (current.index == 235)
                                                    h.gainScale->store (25.0f);
                                            });

            const auto closedAfter = chainRefDb (*p, *designer, rate, at);
            const bool editApplied = sameBits (rawOf (*p, "gain_scale"), 25.0f);

            Tally steady;
            juce::String figures;

            for (size_t i = 0; i < 3; ++i)
            {
                const double before = toneGainDb (input, output, editAt - window, window, tones[i], rate);
                const double after = toneGainDb (input, output, editAt + oneSecond, window, tones[i], rate);

                steady.check (std::abs (after - closedAfter[i]) <= 0.01 && std::abs (closedAfter[i] - nominalAfter[i]) <= 0.01,
                              fmtG (tones[i], 4) + " Hz after: " + fmtG (after, 6) + " dB, closed form " + fmtG (closedAfter[i], 6)
                                  + ", nominal " + fmtG (nominalAfter[i], 3));
                steady.check (std::abs (before - closedBefore[i]) <= 0.01,
                              fmtG (tones[i], 4) + " Hz before: " + fmtG (before, 6) + " dB, closed form " + fmtG (closedBefore[i], 6));

                figures += (figures.isEmpty() ? "" : "; ") + fmtG (tones[i], 4) + " Hz " + fmtG (before, 6) + " -> "
                         + fmtG (after, 6) + " dB (closed form " + fmtG (closedAfter[i], 6) + ")";
            }

            const bool scaleBites = std::abs (closedBefore[0] - closedAfter[0]) > 1.0 && std::abs (closedBefore[1] - closedAfter[1]) > 1.0;

            row (steady.bad == 0 && steady.total == 6 && editApplied && scaleBites && allFinite (output) && activeBands (*p) == 3,
                 "M2 (i) three Stereo bells +24 / +6 / 0 dB, gain_scale 100 -> 25 mid-render: steady-state gains 1 s later are +6 / +1.5 / 0 dB within 0.01 dB of the closed forms",
                 steady.detail() + "  " + figures + (editApplied ? "" : "  (edit not applied)"));

            // (ii) the move itself
            const auto from = (size_t) std::lround (1.0 * rate);
            const auto source = channelOf (input, 0);
            const auto yPlugin = widened (output, 0);
            const auto yRef = refRender (source, rate, harnessGrid (rate), harnessRampTicks (rate), script);
            const auto yHard = hardRender (source, rate, script);

            const double pluginKappa = localKappa (yPlugin, rate, from, yPlugin.size());
            const double refKappa = localKappa (yRef, rate, from, yRef.size());
            const double hardKappa = localKappa (yHard, rate, from, yHard.size());
            const double ratio = ratioOf (pluginKappa, refKappa);

            // L = R in, one Stereo band chain: the two lanes must stay equal while the bands ramp (PLAN-2.3 P4).
            const bool lanesEqual = std::memcmp (output.getReadPointer (0), output.getReadPointer (1),
                                                 sizeof (float) * (size_t) total) == 0;

            row (yRef.size() == yPlugin.size() && refKappa > 0.0 && ratio <= 4.0 && lanesEqual && allFinite (output),
                 "M2 (ii) the same move: local kappa of the float output <= 4.0x the same-motion reference (refRender); L and R stay memcmp-equal on L = R input",
                 "plugin " + fmtG (pluginKappa, 4) + ", reference " + fmtG (refKappa, 4) + ", plugin / reference "
                     + fmtG (ratio, 4) + "x; no smoothing (hardRender) reads " + fmtG (ratioOf (hardKappa, refKappa), 4) + "x"
                     + (lanesEqual ? "" : "; L AND R DIFFER"));
        }

        // M2c: the same edit where gain is inert (PLAN-2.3 P6).
        {
            constexpr int blockSize = 512, editBlock = 10;
            const auto input = makeNoiseAt (2, 36 * blockSize, 0x3B2C0002, kNoise12);

            // Gain +12 dB on both, so the effective gain would move from 12 to 3 dB were it read.
            const auto specOf = [] (int index, bool bellInstead)
            {
                if (index == 0)
                    return makeSpec (kLowCut, 80.0f, 12.0f, 1.0f, 3, kStereo);

                return makeSpec (bellInstead ? kBell : kNotch, 1000.0f, 12.0f, 4.0f, 1, kStereo);
            };

            const auto renderPair = [&input, &specOf] (bool bellInstead, bool withEdit, bool& applied)
            {
                auto q = makeBands (2, 2, [&specOf, bellInstead] (int index) { return specOf (index, bellInstead); });
                const Handles hq = handlesOf (*q);

                auto rendered = renderWith (*q, input, kRenderRate, 512,
                                            [] (int, int) { return 512; },
                                            [&hq, withEdit] (BlockInfo& current)
                                            {
                                                if (withEdit && current.index == 10)
                                                    hq.gainScale->store (25.0f);
                                            });

                applied = sameBits (hq.gainScale->load(), 25.0f);
                return rendered;
            };

            bool plainApplied = false, editApplied = false, bellPlainApplied = false, bellEditApplied = false;
            const auto plain = renderPair (false, false, plainApplied);
            const auto edited = renderPair (false, true, editApplied);
            const auto bellPlain = renderPair (true, false, bellPlainApplied);
            const auto bellEdited = renderPair (true, true, bellEditApplied);

            const bool identical = sameAudio (plain, edited);
            const bool acts = ! sameAudio (plain, input);
            const int bellFirst = firstDifference (bellPlain, bellEdited);
            const bool controlFires = bellFirst >= editBlock * blockSize;

            row (identical && acts && editApplied && ! plainApplied && bellEditApplied && ! bellPlainApplied && controlFires,
                 "M2c gain_scale 100 -> 25 with a Low Cut and a Notch as the only bands: output bit-identical to the render without the edit; with a Bell for the Notch it differs",
                 juce::String (identical ? "" : "the edit changed the output; ") + (acts ? "" : "the bands do not act; ")
                     + (editApplied ? "" : "edit not applied; ")
                     + "control: Bell in place of the Notch first differs at sample " + juce::String (bellFirst)
                     + " (edit seen at " + juce::String (editBlock * blockSize) + ")");
        }

        // M3: band bypass on, then off, on the only active band.
        {
            constexpr int blockSize = 512, onBlock = 10, offBlock = 20;
            const auto input = makeNoiseAt (2, 36 * blockSize, 0x3B300002, kNoise12);
            const int total = input.getNumSamples();
            const int onAt = onBlock * blockSize;
            const int offAt = offBlock * blockSize;

            const auto spec = makeSpec (kLowShelf, 300.0f, 9.0f, 1.0f, 3, kStereo);
            const auto one = [&spec] (int) { return spec; };

            auto p = makeBands (2, 1, one);
            const Handles h = handlesOf (*p);

            const auto output = renderWith (*p, input, kRenderRate, 512,
                                            [] (int, int) { return 512; },
                                            [&h] (BlockInfo& current)
                                            {
                                                if (current.index == 10)
                                                    h.band[0].bypass->store (1.0f);

                                                if (current.index == 20)
                                                    h.band[0].bypass->store (0.0f);
                                            });

            auto steady = makeBands (2, 1, one);
            const auto steadyOut = render (*steady, input, blockSize, false);

            auto fresh = makeBands (2, 1, one);
            const auto freshOut = render (*fresh, sliceOf (input, offAt, total - offAt), blockSize, false);

            // Amended in Phase 2.4 (PLAN-2.4 P18): band bypass on and off are
            // N-sample crossfades. On: the output is the input from the toggle
            // + N; off: from the toggle + N it equals a fresh instance started
            // at the toggle. N = eq::fadeLengthSamples (fs).
            const int fadeN = eq::fadeLengthSamples (kRenderRate);

            Tally tally;
            tally.check (sameFrom (output, 0, steadyOut, 0, onAt), "output before the bypass differs from the steady render");
            tally.check (sameFrom (output, onAt + fadeN, input, onAt + fadeN, offAt - onAt - fadeN), "bypass on: the output is not the input from the toggle + N");
            tally.check (sameFrom (output, offAt + fadeN, freshOut, fadeN, total - offAt - fadeN), "bypass off: from the toggle + N the output differs from a fresh instance started at the toggle");
            tally.check (! sameFrom (steadyOut, onAt, input, onAt, offAt - onAt), "control: the band does not act on the audio");
            tally.check (! sameFrom (steadyOut, offAt, freshOut, 0, total - offAt), "control: an uninterrupted render equals a fresh one (state does not matter)");
            tally.check (activeBands (*p) == 1 && activeBands (*steady) == 1, "the band is not the only active one at the end");

            row (tally.bad == 0 && tally.total == 6,
                 "M3 bNN_bypass on the only active band, on at block 10, off at block 20: on, out memcmp-equal to in from the toggle + N; off, from the toggle + N memcmp-equal to a fresh instance started at the toggle",
                 tally.detail() + "  N = " + juce::String (fadeN));
        }

        // M4: prepare at 44.1 kHz, render, prepare at 96 kHz, render.
        {
            const auto in44 = makeNoiseAt (2, 44100, 0x3B440002, kNoise12);
            const auto in96 = makeNoiseAt (2, 96000, 0x3B960002, kNoise12);
            const auto six = [] (int index) { return channelBandSpec (index, kStereo); };
            const auto blocks512 = [] (int, int) { return 512; };
            const auto noHook = [] (BlockInfo&) {};

            auto reused = makeBands (2, 6, six);
            const auto first = renderWith (*reused, in44, 44100.0, 512, blocks512, noHook);
            const auto second = renderWith (*reused, in96, 96000.0, 512, blocks512, noHook);

            auto fresh = makeBands (2, 6, six);
            const auto freshOut = renderWith (*fresh, in96, 96000.0, 512, blocks512, noHook);

            // Control: the same input through designs made for 44.1 kHz is another signal.
            auto stale = makeBands (2, 6, six);
            const auto staleOut = renderWith (*stale, in96, 44100.0, 512, blocks512, noHook);

            const bool same = sameAudio (second, freshOut);
            const bool acts = ! sameAudio (first, in44) && ! sameAudio (second, in96) && activeBands (*reused) == 6;
            const bool rateMatters = ! sameAudio (freshOut, staleOut);

            row (same && acts && rateMatters,
                 "M4 one instance: prepare 44.1 kHz, render 1 s with 6 bands, prepare 96 kHz, render: memcmp-equal to a fresh instance prepared at 96 kHz",
                 (same ? juce::String()
                       : "differs from the fresh instance (first at sample " + juce::String (firstDifference (second, freshOut)) + "); ")
                     + (acts ? "" : "the bands do not act; ")
                     + (rateMatters ? "control: a 44.1 kHz preparation renders the same input differently"
                                    : "control: the rate does not change the render"));
        }

        // M5: the response of a Mid band and of a Side band, through the plugin.
        {
            constexpr int window = 4800;                    // 80 cycles of 800 Hz at 48 kHz
            const double rate = kRenderRate;
            const double tone = 800.0;
            const int settle = (int) std::lround (rate);
            const int total = settle + window;

            // L and R differ in level and phase, so the mid and the side both carry the tone.
            juce::AudioBuffer<float> input (2, total);

            for (int i = 0; i < total; ++i)
            {
                const double phase = 2.0 * eq::pi * tone * (double) i / rate;
                input.setSample (0, i, (float) (0.1 * std::sin (phase)));
                input.setSample (1, i, (float) (0.05 * std::cos (phase)));
            }

            const auto designer = designerFor (rate);
            const std::vector<double> at { tone };

            Tally tally;
            juce::String figures;

            for (const int placement : { kMid, kSide })
            {
                auto p = makeBands (2, 1, [placement] (int index) { return msBellSpec (index, 1, 30.0f, placement); });
                const auto output = render (*p, input, 512, false);

                const double bellGain = (double) rawOf (*p, bandId (1, "gain"));
                const double closedForm = chainRefDb (*p, *designer, rate, at)[0];

                const double midDb = eq::powerToDb (componentPower (output, 1.0, settle, window, tone, rate)
                                                        / componentPower (input, 1.0, settle, window, tone, rate));
                const double sideDb = eq::powerToDb (componentPower (output, -1.0, settle, window, tone, rate)
                                                         / componentPower (input, -1.0, settle, window, tone, rate));

                const double own = placement == kMid ? midDb : sideDb;
                const double other = placement == kMid ? sideDb : midDb;
                const juce::String name = kPlacementNames[placement];

                tally.check (std::abs (own - bellGain) <= 0.01,
                             name + " bell: its own component reads " + fmtG (own, 7) + " dB, bell gain " + fmtG (bellGain, 4));
                tally.check (std::abs (other) <= 0.01,
                             name + " bell: the other component reads " + fmtG (other, 4) + " dB");

                figures += (figures.isEmpty() ? "" : "; ") + name + " bell: own component " + fmtG (own, 7)
                         + " dB (bell gain " + fmtG (bellGain, 4) + ", closed form at 800 Hz " + fmtG (closedForm, 7)
                         + "), other component " + fmtG (other, 3) + " dB";
            }

            row (tally.bad == 0 && tally.total == 4,
                 "M5 T1c's single bell (800 Hz, +30 dB, Q 1) on Mid, then on Side: the gain of its own component at the centre equals the bell gain, the other component's is 0 dB, both within 0.01 dB",
                 tally.detail() + "  " + figures);
        }

        // M6, M6n: a continuous change keeps the state, a structural one does not.
        {
            const double rate = 48000.0;
            const int total = (int) std::lround (rate);
            const int editAt = kKeepEditBlock * kKeepBlock;         // 24000

            juce::AudioBuffer<float> burst (1, total);
            burst.clear();

            for (int i = 0; i < 480; ++i)
                burst.setSample (0, i, (float) (0.5 * std::sin (2.0 * eq::pi * 40.0 * (double) i / rate)));

            const auto unedited = keepRender (burst, rate, 0);
            const auto gainEdit = keepRender (burst, rate, 1);

            const KeepFigures kept = keepFigures (gainEdit, editAt, rate);

            const int gainFirst = firstDifference (unedited, gainEdit);

            row (allFinite (gainEdit) && kept.ownD2 > 0.0 && gainFirst == editAt && kept.ratio() <= 10.0,
                 "M6 bell 40 Hz Q 20 +24 dB rung by a burst, gain -> +18 dB during the ring: |y[n] - (2 y[n-1] - y[n-2])| over the 20 ms after the edit <= 10x the ring's own largest second difference",
                 fmtG (kept.ratio(), 4) + "x (" + fmtG (kept.after, 4) + " against " + fmtG (kept.ownD2, 4) + "; ring peak "
                     + fmtG (kept.ringPeak, 4) + "); the edit first changes sample " + juce::String (gainFirst)
                     + " (seen at " + juce::String (editAt) + ")");

            // M6n retired in Phase 2.4 (PLAN-2.4 P18): a type change is now a
            // 20 ms crossfade, so it no longer reads >= 100x. Replaced by
            // XR-rest (the incoming slot starts from rest at the fade's first
            // sample, on this same stimulus) and by the hard control XR-g3h.
        }
    }

    //==========================================================================
    // B1, TK. Block identity with moving parameters; the tick clock (PLAN-2.3 P7, P10)
    //==========================================================================
    struct BlockEvent
    {
        float freq[4];
        float gain[4];
        float q[4];
        float scale;
    };

    struct Xorshift
    {
        std::uint64_t state = 0x9E3779B97F4A7C15ull;

        double uniform()        // [0, 1)
        {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            return (double) (state >> 11) * (1.0 / 9007199254740992.0);
        }
    };

    // RM-blk's script (ramp_bench blk): frequency, gain and Q of four bands
    // redrawn over their full ranges at each of 96 events; plus gain_scale.
    std::vector<BlockEvent> makeBlockEvents()
    {
        Xorshift draw;
        draw.state = 777;

        std::vector<BlockEvent> events (96);

        for (auto& e : events)
        {
            for (int b = 0; b < 4; ++b)
            {
                e.freq[b] = juce::jlimit (10.0f, 30000.0f, (float) std::exp (std::log (10.0) + draw.uniform() * std::log (3000.0)));
                e.gain[b] = juce::jlimit (-30.0f, 30.0f, (float) (-30.0 + 60.0 * draw.uniform()));
                e.q[b] = juce::jlimit (0.025f, 40.0f, (float) std::exp (std::log (0.025) + draw.uniform() * std::log (1600.0)));
            }

            e.scale = juce::jlimit (0.0f, 200.0f, (float) (200.0 * draw.uniform()));
        }

        return events;
    }

    BandSpec blockIdentitySpec (int index)
    {
        const BandSpec table[4] =
        {
            makeSpec (kBell,      200.0f,   6.0f, 1.0f, 1, kStereo),
            makeSpec (kHighShelf, 4000.0f, -9.0f, 1.0f, 7, kStereo),    // 96 dB/oct
            makeSpec (kLowCut,    60.0f,    0.0f, 2.0f, 5, kStereo),    // 48 dB/oct
            makeSpec (kTilt,      900.0f,   4.0f, 1.0f, 3, kStereo)     // 24 dB/oct
        };

        return table[index % 4];
    }

    // events == nullptr: the same render with no event.
    juce::AudioBuffer<float> blockIdentityRender (Processor& p, const Handles& h, const juce::AudioBuffer<float>& input,
                                                  double rate, int blockSize, const std::vector<BlockEvent>* events)
    {
        return renderWith (p, input, rate, blockSize,
                           [blockSize] (int, int) { return blockSize; },
                           [&h, events] (BlockInfo& current)
                           {
                               // An event is seen by the block that starts on it.
                               if (events == nullptr || current.position % 2048 != 0)
                                   return;

                               const auto slot = (size_t) (current.position / 2048);

                               if (slot >= events->size())
                                   return;

                               const BlockEvent& e = (*events)[slot];
                               h.gainScale->store (e.scale);

                               for (size_t b = 0; b < 4; ++b)
                               {
                                   h.band[b].freq->store (e.freq[b]);
                                   h.band[b].gain->store (e.gain[b]);
                                   h.band[b].q->store (e.q[b]);
                               }
                           });
    }

    juce::AudioBuffer<float> tickRender (const juce::AudioBuffer<float>& input, int idleMode, int boundary, bool withEdit)
    {
        // A 0 dB bell, enabled. idleMode 0: global bypass on to sample 1000.
        // 1: the band disabled to sample 1000. 2: processBlockBypassed to sample 1000.
        auto p = makeBands (2, 1, [] (int) { return makeSpec (kBell, 1000.0f, 0.0f, 1.0f, 1, kStereo); });
        const Handles h = handlesOf (*p);

        if (idleMode == 0)
            h.bypass->store (1.0f);

        if (idleMode == 1)
            h.band[0].enabled->store (0.0f);

        return renderWith (*p, input, 48000.0, 1024,
                           [boundary] (int index, int)
                           {
                               return index == 0 ? kTickActiveFrom : (index == 1 ? boundary - kTickActiveFrom : 64);
                           },
                           [&h, idleMode, withEdit] (BlockInfo& current)
                           {
                               if (current.index == 0 && idleMode == 2)
                                   current.viaBypassed = true;

                               if (current.index == 1)
                               {
                                   if (idleMode == 0)
                                       h.bypass->store (0.0f);

                                   if (idleMode == 1)
                                       h.band[0].enabled->store (1.0f);
                               }

                               if (current.index == 2 && withEdit)
                                   h.band[0].gain->store (24.0f);
                           });
    }

    void blockAndTickRows()
    {
        section ("B1 / TK. block identity with moving parameters; the tick clock (Phase 2.3: PLAN-2.3 P7, P10)");

        // B1 (PLAN-2.3; not the bus-layout rows of Stage 1)
        {
            constexpr int length = 2048 * 96;
            const double rates[4] = { 44100.0, 48000.0, 96000.0, 192000.0 };
            const int sizes[4] = { 32, 64, 512, 2048 };
            const auto input = makeNoiseAt (2, length, 0x0B1E0002, kNoise12);
            const auto events = makeBlockEvents();

            struct Job { double rate; int blockSize; bool withEvents; };
            std::vector<Job> jobs;

            for (const double rate : rates)
            {
                for (const int blockSize : sizes)
                    jobs.push_back ({ rate, blockSize, true });

                jobs.push_back ({ rate, 512, false });
            }

            std::vector<std::unique_ptr<Processor>> instances;
            std::vector<Handles> handles;

            for (size_t j = 0; j < jobs.size(); ++j)
            {
                instances.push_back (makeBands (2, 4, blockIdentitySpec));
                handles.push_back (handlesOf (*instances.back()));
            }

            std::vector<juce::AudioBuffer<float>> outputs (jobs.size());

            parallelFor ((int) jobs.size(), [&instances, &handles, &jobs, &outputs, &input, &events] (int j)
            {
                const auto slot = (size_t) j;
                outputs[slot] = blockIdentityRender (*instances[slot], handles[slot], input, jobs[slot].rate,
                                                     jobs[slot].blockSize, jobs[slot].withEvents ? &events : nullptr);
            });

            Tally tally;

            for (size_t r = 0; r < 4; ++r)
            {
                const size_t base = r * 5;
                const juce::String where = fmtG (rates[r] / 1000.0, 4) + " kHz";

                tally.check (allFinite (outputs[base]) && ! sameAudio (outputs[base], input), where + ": not finite, or the bands do not act");

                for (size_t k = 1; k < 4; ++k)
                    tally.check (sameAudio (outputs[base + k], outputs[base]),
                                 where + ": block " + juce::String (sizes[k]) + " differs from block 32, first at sample "
                                     + juce::String (firstDifference (outputs[base + k], outputs[base])));

                // The render without events is compared at its own block size (512 is index 2).
                tally.check (! sameAudio (outputs[base + 4], outputs[base + 2]), where + ": the render without events is the same (the events do not bite)");
            }

            row (tally.bad == 0 && tally.total == 20,
                 "B1 four bands, 96 events on multiples of 2048 (f, gain, Q, gain_scale redrawn): blocks 32 / 64 / 512 / 2048 bit-identical at 44.1 / 48 / 96 / 192 kHz; differs from the render without events",
                 tally.detail());
        }

        // TK (PLAN-2.3 R4: the only row that sees a clock that stops on an early return)
        {
            const auto input = makeNoiseAt (2, kTickLength, 0x71C40002, kNoise12);
            const char* const idleNames[3] = { "(a) bypass on", "(b) no active band", "(c) processBlockBypassed" };

            Tally tally;
            juce::String figures;

            for (size_t tc = 0; tc < 3; ++tc)
            {
                // Amended in Phase 2.4 by Taylor's call (see kTickExpectedPlugin).
                const TickCase c { kTickCases[tc].boundary, kTickExpectedPlugin[tc] };

                figures += (figures.isEmpty() ? "" : "; ") + juce::String ("edit at ") + juce::String (c.boundary) + ":";

                for (int idleMode = 0; idleMode < 3; ++idleMode)
                {
                    const auto plain = tickRender (input, idleMode, c.boundary, false);
                    const auto edited = tickRender (input, idleMode, c.boundary, true);

                    const bool idleUntouched = sameFrom (plain, 0, input, 0, kTickActiveFrom)
                                            && sameFrom (edited, 0, input, 0, kTickActiveFrom);
                    const int first = firstDifference (plain, edited);

                    tally.check (idleUntouched && allFinite (edited) && first == c.expected,
                                 juce::String (idleNames[idleMode]) + ", boundary and edit at " + juce::String (c.boundary)
                                     + ": first differing sample " + juce::String (first) + ", expected " + juce::String (c.expected)
                                     + (idleUntouched ? "" : " (the idle block is not the input)"));

                    figures += " " + juce::String (first);
                }

                figures += " (expected " + juce::String (c.expected) + ")";
            }

            row (tally.bad == 0 && tally.total == 9,
                 "TK 1000 samples idle by (a) bypass on, (b) no active band, (c) processBlockBypassed, then a bell and a 0 -> +24 dB edit, 48 kHz: first sample that differs from the unedited render is 1040 (edit at 1036), 1056 (at 1041), 1026 (at 1024; the 0 dB bell sleeps and its wake fade's first two weights round away in float)",
                 tally.detail() + "  " + figures);
        }
    }

    //==========================================================================
    // S5, S6, A2. Random stress (G5, PLAN-2.3 P13; ported from ramp_bench g5),
    // non-finite input (P9), allocation under stress (G6)
    //==========================================================================
    constexpr int kStressPrepared = 4096;
    constexpr double kStressStackLimitDb = 600.0;

    struct StressBand
    {
        float type = 0.0f;
        float slope = 1.0f;
        float placement = 0.0f;
        float enabled = 1.0f;
        float bypass = 0.0f;
        float freq = 1000.0f;
        float gain = 0.0f;
        float q = 1.0f;
    };

    /** The seeded draw. Every host block: gain_scale, and the frequency, gain
        and Q of all 24 bands, 10 % of the draws on each range end. Every 50th
        block (the first included): type, slope, placement, enable and band
        bypass as well. The audio has its own stream, so the parameter draw
        can be replayed without it (the stack guard does). */
    struct StressDraw
    {
        Xorshift random;
        float scale = 100.0f;
        StressBand bands[Processor::kNumBands];

        void nextBlock (bool restructure)
        {
            const double su = random.uniform();
            scale = (float) (su < 0.1 ? 0.0 : (su > 0.9 ? 200.0 : 200.0 * random.uniform()));

            for (auto& s : bands)
            {
                if (restructure)
                {
                    s.type = (float) juce::jmin (7, (int) (random.uniform() * 8.0));
                    s.slope = (float) juce::jmin (7, (int) (random.uniform() * 8.0));
                }

                const double u = random.uniform();
                const double f = u < 0.1 ? 10.0
                                         : (u > 0.9 ? 30000.0 : std::exp (std::log (10.0) + random.uniform() * std::log (3000.0)));
                const double w = random.uniform();
                const double g = w < 0.1 ? -30.0 : (w > 0.9 ? 30.0 : -30.0 + 60.0 * random.uniform());
                const double z = random.uniform();
                const double sharpness = z < 0.1 ? 0.025
                                                 : (z > 0.9 ? 40.0 : std::exp (std::log (0.025) + random.uniform() * std::log (1600.0)));

                s.freq = juce::jlimit (10.0f, 30000.0f, (float) f);
                s.gain = juce::jlimit (-30.0f, 30.0f, (float) g);
                s.q = juce::jlimit (0.025f, 40.0f, (float) sharpness);

                if (restructure)
                {
                    s.enabled = random.uniform() > 0.15 ? 1.0f : 0.0f;
                    s.placement = (float) juce::jmin (4, (int) (random.uniform() * 5.0));
                    s.bypass = random.uniform() < 0.10 ? 1.0f : 0.0f;
                }
            }
        }

        eq::RawBand rawBand (int b) const
        {
            const StressBand& s = bands[b];

            eq::RawBand raw;
            raw.enabled = s.enabled;
            raw.bypass = s.bypass;
            raw.type = s.type;
            raw.freq = s.freq;
            raw.gain = s.gain;
            raw.q = s.q;
            raw.slope = s.slope;
            raw.placement = s.placement;
            return raw;
        }
    };

    // Block sizes 4096, 512, 64, 7, 1, cycling every 2 s.
    int stressBlockSize (std::int64_t position, double rate)
    {
        static const int sizes[5] = { 4096, 512, 64, 7, 1 };
        return sizes[(size_t) ((double) position / rate / 2.0) % 5];
    }

    // The static stack of the current draw: the sum over active bands of the
    // peak of eq::bandMagDb of the band's target design, floor 0 dB per band
    // (64 log-spaced points from 5 Hz to 0.4999 fs, plus the band's own frequency).
    double stressStackDb (const StressDraw& draw, const eq::Designer& designer, double rate)
    {
        const double top = 0.4999 * rate;
        double stack = 0.0;

        for (int b = 0; b < Processor::kNumBands; ++b)
        {
            const eq::BandInput in = eq::makeBandInput (draw.rawBand (b), draw.scale, 0.0);

            if (! in.active)
                continue;

            eq::BandDesign design {};
            designer.design (in.settings, design);

            double peak = 0.0;

            for (int i = 0; i < 64; ++i)
            {
                const double hz = 5.0 * std::pow (top / 5.0, (double) i / 63.0);
                peak = juce::jmax (peak, eq::bandMagDb (design, 2.0 * eq::pi * hz / rate));
            }

            peak = juce::jmax (peak, eq::bandMagDb (design, 2.0 * eq::pi * juce::jmin (in.settings.freqHz, top) / rate));
            stack += peak;
        }

        return stack;
    }

    // The largest static stack of the whole draw, taken at the blocks of 512
    // and 4096 samples: the parameter stream replayed without audio.
    double stressLargestStackDb (double rate, double seconds, std::uint64_t seed)
    {
        const auto designer = designerFor (rate);
        const auto total = (std::int64_t) std::llround (seconds * rate);

        StressDraw draw;
        draw.random.state = seed;

        double largest = 0.0;
        std::int64_t position = 0, blocks = 0;

        while (position < total)
        {
            const int size = stressBlockSize (position, rate);
            const int m = (int) juce::jmin ((std::int64_t) size, total - position);

            draw.nextBlock ((blocks % 50) == 0);

            if (size >= 512)
                largest = juce::jmax (largest, stressStackDb (draw, *designer, rate));

            position += m;
            ++blocks;
        }

        return largest;
    }

    struct StressSeed
    {
        bool ok = false;                // a draw below the limit was found
        int attempt = 0;                // 0 is the first seed; above 0 the seed was changed
        std::uint64_t seed = 0;
        double largestStackDb = 0.0;    // of the draw in use
        double rejectedStackDb = 0.0;   // the largest stack among the rejected draws
        double plainStackDb = 0.0;      // Phase 2.5 gain draws: the stack alone (largestStackDb is then the guard)
    };

    /** PLAN-2.3 P13: the float output overflows near +770 dB, so the largest
        static stack of the whole draw must stay below +600 dB. A draw that
        does not is a stimulus failure: the seed is changed (attempt + 1),
        and the change is recorded in the row's output. Nothing else is
        changed for it. */
    StressSeed chooseStressSeed (double rate, double seconds)
    {
        StressSeed chosen;

        for (int attempt = 0; attempt < 32; ++attempt)
        {
            std::uint64_t seed = (0x9E3779B97F4A7C15ull ^ (std::uint64_t) rate)
                               + (std::uint64_t) attempt * 0xD1B54A32D192ED03ull;

            if (seed == 0)
                seed = 1;

            const double stack = stressLargestStackDb (rate, seconds, seed);

            chosen.attempt = attempt;
            chosen.seed = seed;
            chosen.largestStackDb = stack;

            if (stack < kStressStackLimitDb)
            {
                chosen.ok = true;
                return chosen;
            }

            chosen.rejectedStackDb = juce::jmax (chosen.rejectedStackDb, stack);
        }

        return chosen;
    }

    void writeStress (const Handles& h, const StressDraw& draw, bool structureToo)
    {
        h.gainScale->store (draw.scale);

        for (size_t b = 0; b < h.band.size(); ++b)
        {
            const StressBand& s = draw.bands[b];
            const BandHandles& hb = h.band[b];

            hb.freq->store (s.freq);
            hb.gain->store (s.gain);
            hb.q->store (s.q);

            if (structureToo)
            {
                hb.type->store (s.type);
                hb.slope->store (s.slope);
                hb.placement->store (s.placement);
                hb.enabled->store (s.enabled);
                hb.bypass->store (s.bypass);
            }
        }
    }

    struct StressOutcome
    {
        std::int64_t nonFinite = 0;     // output samples
        std::int64_t blocks = 0;
        std::int64_t actingBlocks = 0;  // blocks whose output differs from their input
        double peak = 0.0;              // of the finite output samples
        double inputPeak = 0.0;
    };

    /** The stress render: stereo, prepared at 4096, noise at -12 dBFS with one
        second of silence in five. Parameters are stored before each block;
        `process (buffer, midi)` runs the block (A2 arms its window there). */
    template <typename ProcessFn>
    StressOutcome stressLoop (Processor& p, const Handles& h, double rate, double seconds, std::uint64_t seed,
                              ProcessFn&& process)
    {
        StressOutcome out;

        StressDraw draw;
        draw.random.state = seed;

        Xorshift audio;
        audio.state = seed ^ 0xA5A5A5A55A5A5A5Aull;

        if (audio.state == 0)
            audio.state = 1;

        const auto total = (std::int64_t) std::llround (seconds * rate);

        juce::AudioBuffer<float> block (2, kStressPrepared);
        std::vector<float> keptLeft ((size_t) kStressPrepared), keptRight ((size_t) kStressPrepared);
        juce::MidiBuffer midi;

        h.bypass->store (0.0f);
        p.prepareToPlay (rate, kStressPrepared);

        std::int64_t position = 0;

        while (position < total)
        {
            const int size = stressBlockSize (position, rate);
            const int m = (int) juce::jmin ((std::int64_t) size, total - position);
            const bool silent = std::fmod ((double) position / rate, 5.0) > 4.0;
            const bool restructure = (out.blocks % 50) == 0;

            draw.nextBlock (restructure);
            writeStress (h, draw, restructure);

            block.setSize (2, m, false, false, true);
            float* const left = block.getWritePointer (0);
            float* const right = block.getWritePointer (1);

            for (int i = 0; i < m; ++i)
            {
                const float l = silent ? 0.0f : (float) ((audio.uniform() * 2.0 - 1.0) * 0.25);
                const float r = silent ? 0.0f : (float) ((audio.uniform() * 2.0 - 1.0) * 0.25);

                left[i] = l;
                right[i] = r;
                keptLeft[(size_t) i] = l;
                keptRight[(size_t) i] = r;
                out.inputPeak = juce::jmax (out.inputPeak, (double) std::abs (l), (double) std::abs (r));
            }

            process (block, midi);

            const float* const outLeft = block.getReadPointer (0);
            const float* const outRight = block.getReadPointer (1);
            bool acts = false;

            for (int i = 0; i < m; ++i)
            {
                const float l = outLeft[i];
                const float r = outRight[i];

                acts = acts || ! sameBits (l, keptLeft[(size_t) i]) || ! sameBits (r, keptRight[(size_t) i]);

                if (! std::isfinite (l) || ! std::isfinite (r))
                {
                    ++out.nonFinite;
                    continue;
                }

                out.peak = juce::jmax (out.peak, (double) std::abs (l), (double) std::abs (r));
            }

            if (acts)
                ++out.actingBlocks;

            position += m;
            ++out.blocks;
        }

        return out;
    }

    /** Phase 2.5 (PLAN-2.5 P12, the S5 amendment; T7 A4): output_gain and
        auto_gain per block, from their own stream, so the P14 band draw is
        the 2.4 one unchanged. */
    struct StressGainDraw
    {
        Xorshift random;
        int mode = 0;               // 0: no gain draw (2.4); 1: S5 (2.5); 2: A4
        float outputDb = 0.0f;
        bool autoOn = false;
        int autoToggles = 0;
        float lowestDb = 0.0f, highestDb = 0.0f;

        void begin (int gainMode, std::uint64_t seed)
        {
            mode = gainMode;
            random.state = seed ^ 0x6A09E667F3BCC909ull;

            if (random.state == 0)
                random.state = 1;

            outputDb = 0.0f;
            autoOn = false;
            autoToggles = 0;
            lowestDb = highestDb = 0.0f;
        }

        // output_gain uniform -36 ... +36 dB, each range end with p 0.05;
        // auto_gain toggled with p 1/100 (mode 1) or on (mode 2).
        void nextBlock()
        {
            if (mode == 0)
                return;

            const double u = random.uniform();
            outputDb = (float) (u < 0.05 ? -36.0 : (u > 0.95 ? 36.0 : -36.0 + 72.0 * random.uniform()));
            lowestDb = juce::jmin (lowestDb, outputDb);
            highestDb = juce::jmax (highestDb, outputDb);

            if (mode == 2)
            {
                autoOn = true;
            }
            else if (random.uniform() < 0.01)
            {
                autoOn = ! autoOn;
                ++autoToggles;
            }
        }
    };

    /** Phase 2.6 (PLAN-2.6 P16, the S5 amendment; A5): listenBand and
        analyzerMode per block, from their own stream, so the band and gain
        draws are unchanged. mode 1 (S5): listenBand redrawn with p 1/200 per
        block (-1 with p 0.5, else a uniform band), analyzerMode redrawn with
        p 1/500 (uniform 0 ... 3; 3 at the start). mode 2 (A5): listenBand
        switched every 100 ms of audio (bands 1, 8, 15, 22, 5 in turn, then
        off, cycling), analyzerMode 3. The caller registers the analyzer
        consumer; nothing drains the rings. */
    struct StressListenDraw
    {
        Xorshift random;
        int mode = 0;               // 0: none (2.5 and earlier); 1: S5; 2: A5
        int band = -1;
        int analyzer = 3;
        int listenChanges = 0, modeChanges = 0, step = 0;
        std::int64_t nextSwitch = 0;

        void begin (int listenMode, std::uint64_t seed)
        {
            mode = listenMode;
            random.state = seed ^ 0x4C495354454E3236ull;

            if (random.state == 0)
                random.state = 1;

            band = -1;
            analyzer = 3;
            listenChanges = modeChanges = step = 0;
            nextSwitch = 0;
        }

        void nextBlock (std::int64_t position, double rate)
        {
            if (mode == 1)
            {
                if (random.uniform() < 1.0 / 200.0)
                {
                    band = random.uniform() < 0.5 ? -1 : juce::jmin (Processor::kNumBands - 1, (int) (random.uniform() * (double) Processor::kNumBands));
                    ++listenChanges;
                }

                if (random.uniform() < 1.0 / 500.0)
                {
                    analyzer = juce::jmin (3, (int) (random.uniform() * 4.0));
                    ++modeChanges;
                }
            }
            else if (mode == 2)
            {
                while (position >= nextSwitch)
                {
                    band = (step % 6 == 5) ? -1 : (step * 7) % Processor::kNumBands;
                    ++step;
                    ++listenChanges;
                    nextSwitch += std::llround (0.100 * rate);
                }

                analyzer = 3;
            }
        }
    };

    struct StressResult
    {
        StressSeed chosen;
        StressOutcome outcome;
        bool ran = false;
        bool offFinite = true;
        double offEqualFromMs = 0.0;    // after "all bands off": the last block that still differed from its input ends here
        StressGainDraw gains;           // Phase 2.5: the gain draw's final state
        StressListenDraw listen;        // Phase 2.6: the listen / analyzer draw's final state
        std::int64_t listeningBlocks = 0, moveBlocks = 0;
        std::uint32_t dropsPre = 0, dropsPost = 0;
    };

    // (Phase 2.4: the 2.3 S5 driver runStress was replaced by runStress24
    // below, PLAN-2.4 P18; the 2.3 draw and loop above stay for A2.)

    //==========================================================================
    // The P14 draw (PLAN-2.4 P14, C10; ported from fade_bench g5): S5's
    // generator, plus per band type / slope / placement / enable / band bypass
    // redrawn with probability 1 / structEvery per block (1: every block), and
    // the global bypass toggled with probability 1/200 per block. Used by the
    // amended S5 and by A3. The 2.3 draw above stays as it was (A2 runs it).
    //==========================================================================
    struct StressDraw24
    {
        Xorshift random;
        float scale = 100.0f;
        bool bypass = false;
        int toggles = 0;
        StressBand bands[Processor::kNumBands];

        void nextBlock (int structEvery)
        {
            const double su = random.uniform();
            scale = (float) (su < 0.1 ? 0.0 : (su > 0.9 ? 200.0 : 200.0 * random.uniform()));

            if (random.uniform() < 1.0 / 200.0)
            {
                bypass = ! bypass;
                ++toggles;
            }

            const double structProbability = 1.0 / (double) juce::jmax (1, structEvery);

            for (auto& s : bands)
            {
                const bool restructure = structEvery <= 1 || random.uniform() < structProbability;

                if (restructure)
                {
                    s.type = (float) juce::jmin (7, (int) (random.uniform() * 8.0));
                    s.slope = (float) juce::jmin (7, (int) (random.uniform() * 8.0));
                }

                const double u = random.uniform();
                const double f = u < 0.1 ? 10.0
                                         : (u > 0.9 ? 30000.0 : std::exp (std::log (10.0) + random.uniform() * std::log (3000.0)));
                const double w = random.uniform();
                const double g = w < 0.1 ? -30.0 : (w > 0.9 ? 30.0 : -30.0 + 60.0 * random.uniform());
                const double z = random.uniform();
                const double sharpness = z < 0.1 ? 0.025
                                                 : (z > 0.9 ? 40.0 : std::exp (std::log (0.025) + random.uniform() * std::log (1600.0)));

                s.freq = juce::jlimit (10.0f, 30000.0f, (float) f);
                s.gain = juce::jlimit (-30.0f, 30.0f, (float) g);
                s.q = juce::jlimit (0.025f, 40.0f, (float) sharpness);

                if (restructure)
                {
                    s.enabled = random.uniform() > 0.15 ? 1.0f : 0.0f;
                    s.placement = (float) juce::jmin (4, (int) (random.uniform() * 5.0));
                    s.bypass = random.uniform() < 0.10 ? 1.0f : 0.0f;
                }
            }
        }

        eq::RawBand rawBand (int b) const
        {
            const StressBand& s = bands[b];

            eq::RawBand raw;
            raw.enabled = s.enabled;
            raw.bypass = s.bypass;
            raw.type = s.type;
            raw.freq = s.freq;
            raw.gain = s.gain;
            raw.q = s.q;
            raw.slope = s.slope;
            raw.placement = s.placement;
            return raw;
        }
    };

    // The peak of one band's target design (floor 0 dB; 0 for an inactive
    // band): the per-band term of stressStackDb, on one input.
    double stressBandPeakDb (const eq::BandInput& in, const eq::Designer& designer, double rate)
    {
        if (! in.active)
            return 0.0;

        const double top = 0.4999 * rate;
        eq::BandDesign design {};
        designer.design (in.settings, design);

        double peak = 0.0;

        for (int i = 0; i < 64; ++i)
        {
            const double hz = 5.0 * std::pow (top / 5.0, (double) i / 63.0);
            peak = juce::jmax (peak, eq::bandMagDb (design, 2.0 * eq::pi * hz / rate));
        }

        return juce::jmax (peak, eq::bandMagDb (design, 2.0 * eq::pi * juce::jmin (in.settings.freqHz, top) / rate));
    }

    // True when a band's structural fields (type, slope, placement, enable,
    // band bypass) differ between two draws: the change that starts a fade.
    bool stressStructureChanged (const StressBand& a, const StressBand& b)
    {
        return a.type != b.type || a.slope != b.slope || a.placement != b.placement
            || a.enabled != b.enabled || a.bypass != b.bypass;
    }

    // PLAN-2.4 P14: the largest static stack of the whole P14 draw, at the
    // blocks of 512 and 4096 samples, with the stack taking per FADING band the
    // larger peak of its two slots (RESEARCH-2.4 C10, which P14 adopts). Only
    // a structural change starts a fade; a continuous redraw ramps on one
    // slot, as in 2.3, and counts its latest request only. Execute-session
    // deviation (2026-11-14): the first T7 form took the larger of the two
    // latest requests on every band at every block, which charged a second
    // slot to bands that have none and left no seed under +600 dB at
    // 176.4 kHz in 32 attempts. Here each band follows the engine's fade
    // schedule (PLAN-2.4 P4): a structural change with no fade in flight
    // starts an N-sample fade whose outgoing slot is the request just before
    // the change; a change during a fade is latched and starts one more fade
    // at that fade's end, its outgoing slot the request then in force. While
    // a fade is in flight the band's term is the larger of the latest request
    // and the outgoing slot.
    //
    // Phase 2.5 (PLAN-2.5 P12, the S5 amendment; A4): gainMode 0 is the 2.4
    // guard above, unchanged (A3). gainMode 1 / 2 also replays the gain draw
    // (StressGainDraw) and the guard at a checked block becomes
    //   stack + the drawn output gain + 12 dB auto-gain allowance,
    // the output gain being the largest drawn over the last 100 ms (the
    // four-pole smoother is exact about 50 ms after a step; the value in
    // force at the window's start included). *plainStackOut, when given,
    // receives the largest stack alone. (StressGainDraw is defined above
    // StressResult.) Execute-session call (Taylor, PLAN-2.5 P12): the first
    // form also added the +12 dB G5 peak margin, which the render guard never
    // carried in 2.4; at 600 s it left no seed in 32 attempts at 176.4 and
    // 192 kHz. The margin is dropped, and S5's gain mode (1) tries up to 256
    // seeds; every other caller keeps 32.
    constexpr double kStressAutoGainAllowanceDb = 12.0;    // the +12 dB clamp of auto gain

    double stressLargestStackDb24 (double rate, double seconds, std::uint64_t seed, int structEvery,
                                   int gainMode = 0, double* plainStackOut = nullptr)
    {
        const auto designer = designerFor (rate);
        const auto total = (std::int64_t) std::llround (seconds * rate);
        const auto fadeN = (std::int64_t) eq::fadeLengthSamples (rate);

        StressDraw24 draw;
        draw.random.state = seed;

        StressGainDraw gains;
        gains.begin (gainMode, seed);
        std::deque<std::pair<std::int64_t, float>> recentGain;     // (block start, output gain dB)
        const auto gainWindow = (std::int64_t) std::llround (0.100 * rate);
        double largestPlain = 0.0;

        double largest = 0.0;
        std::int64_t position = 0;

        double outgoingPeak[Processor::kNumBands] = {};
        std::int64_t fadeEnd[Processor::kNumBands] = {};
        bool latched[Processor::kNumBands] = {};

        while (position < total)
        {
            const int size = stressBlockSize (position, rate);
            const int m = (int) juce::jmin ((std::int64_t) size, total - position);

            const StressDraw24 previous = draw;
            draw.nextBlock (structEvery);

            if (gainMode != 0)
            {
                gains.nextBlock();
                recentGain.emplace_back (position, gains.outputDb);

                while (recentGain.size() >= 2 && recentGain[1].first <= position - gainWindow)
                    recentGain.pop_front();
            }

            for (int b = 0; b < Processor::kNumBands; ++b)
            {
                // A latched change whose fade has started since the last
                // block (at the previous fade's end, inside the last block):
                // its outgoing slot ran the request of that block.
                if (latched[b] && position >= fadeEnd[b])
                {
                    latched[b] = false;
                    outgoingPeak[b] = stressBandPeakDb (eq::makeBandInput (previous.rawBand (b), previous.scale, 0.0), *designer, rate);
                    fadeEnd[b] += fadeN;
                }

                if (! stressStructureChanged (previous.bands[b], draw.bands[b]))
                    continue;

                if (position >= fadeEnd[b])
                {
                    outgoingPeak[b] = stressBandPeakDb (eq::makeBandInput (previous.rawBand (b), previous.scale, 0.0), *designer, rate);
                    fadeEnd[b] = position + fadeN;
                }
                else
                {
                    latched[b] = true;
                }
            }

            if (size >= 512)
            {
                double stack = 0.0;

                for (int b = 0; b < Processor::kNumBands; ++b)
                {
                    const double now = stressBandPeakDb (eq::makeBandInput (draw.rawBand (b), draw.scale, 0.0), *designer, rate);
                    stack += position < fadeEnd[b] ? juce::jmax (now, outgoingPeak[b]) : now;
                }

                largestPlain = juce::jmax (largestPlain, stack);

                if (gainMode != 0)
                {
                    double outputDb = -std::numeric_limits<double>::infinity();

                    for (const auto& entry : recentGain)
                        outputDb = juce::jmax (outputDb, (double) entry.second);

                    stack += outputDb + kStressAutoGainAllowanceDb;
                }

                largest = juce::jmax (largest, stack);
            }

            position += m;
        }

        if (plainStackOut != nullptr)
            *plainStackOut = largestPlain;

        return largest;
    }

    // As chooseStressSeed, on the P14 draw. The every-block run (structEvery
    // 1) takes its own seeds. gainMode (Phase 2.5): see stressLargestStackDb24.
    StressSeed chooseStressSeed24 (double rate, double seconds, int structEvery, int gainMode = 0)
    {
        StressSeed chosen;

        // PLAN-2.5 P12, execute-session call: 256 attempts for S5's gain mode only.
        const int attempts = gainMode == 1 ? 256 : 32;

        for (int attempt = 0; attempt < attempts; ++attempt)
        {
            std::uint64_t seed = ((0x9E3779B97F4A7C15ull ^ (std::uint64_t) rate)
                                  + (std::uint64_t) attempt * 0xD1B54A32D192ED03ull)
                               ^ (structEvery <= 1 ? 0x2B992DDFA23249D6ull : 0x0ull);

            if (seed == 0)
                seed = 1;

            double plain = 0.0;
            const double stack = stressLargestStackDb24 (rate, seconds, seed, structEvery, gainMode, &plain);

            chosen.attempt = attempt;
            chosen.seed = seed;
            chosen.largestStackDb = stack;
            chosen.plainStackDb = plain;

            if (stack < kStressStackLimitDb)
            {
                chosen.ok = true;
                return chosen;
            }

            chosen.rejectedStackDb = juce::jmax (chosen.rejectedStackDb, stack);
        }

        return chosen;
    }

    /** The P14 stress render: as stressLoop, on the P14 draw, every value
        stored before every block. alternateRoute: every second bypassed span
        arrives through processBlockBypassed (the bypass parameter then stays
        off) instead of through the parameter. `process (buffer, midi,
        viaBypassed)` runs the block (A3 arms its window there). */
    /** stressLoop24 with the Phase 2.5 gain draw (gainMode 1: S5, 2: A4;
        0: none, the 2.4 loop bit for bit). The gain values are stored with
        the band values before each block; *gainsOut receives the draw's
        final state (toggle count, range). Phase 2.6 (PLAN-2.6 P16; A5):
        listenDraw, when given and its mode is not 0, stores listenBand and
        analyzerMode after the gains before each block and keeps its counts
        (nullptr: the 2.5 loop bit for bit). */
    template <typename ProcessFn>
    StressOutcome stressLoopCore (Processor& p, const Handles& h, double rate, double seconds, std::uint64_t seed,
                                  int structEvery, bool alternateRoute, int gainMode, StressGainDraw* gainsOut,
                                  StressListenDraw* listenDraw, ProcessFn&& process);

    template <typename ProcessFn>
    StressOutcome stressLoop24 (Processor& p, const Handles& h, double rate, double seconds, std::uint64_t seed,
                                int structEvery, bool alternateRoute, ProcessFn&& process)
    {
        return stressLoopCore (p, h, rate, seconds, seed, structEvery, alternateRoute, 0, nullptr, nullptr, process);
    }

    template <typename ProcessFn>
    StressOutcome stressLoopCore (Processor& p, const Handles& h, double rate, double seconds, std::uint64_t seed,
                                  int structEvery, bool alternateRoute, int gainMode, StressGainDraw* gainsOut,
                                  StressListenDraw* listenDraw, ProcessFn&& process)
    {
        StressOutcome out;

        StressDraw24 draw;
        draw.random.state = seed;

        StressGainDraw gains;
        gains.begin (gainMode, seed);

        Xorshift audio;
        audio.state = seed ^ 0xA5A5A5A55A5A5A5Aull;

        if (audio.state == 0)
            audio.state = 1;

        const auto total = (std::int64_t) std::llround (seconds * rate);

        juce::AudioBuffer<float> block (2, kStressPrepared);
        std::vector<float> keptLeft ((size_t) kStressPrepared), keptRight ((size_t) kStressPrepared);
        juce::MidiBuffer midi;

        h.bypass->store (0.0f);
        p.prepareToPlay (rate, kStressPrepared);

        std::int64_t position = 0;

        while (position < total)
        {
            const int size = stressBlockSize (position, rate);
            const int m = (int) juce::jmin ((std::int64_t) size, total - position);
            const bool silent = std::fmod ((double) position / rate, 5.0) > 4.0;

            draw.nextBlock (structEvery);

            const bool viaBypassed = alternateRoute && draw.bypass && (draw.toggles % 4) == 1;

            h.gainScale->store (draw.scale);
            h.bypass->store (draw.bypass && ! viaBypassed ? 1.0f : 0.0f);

            for (size_t b = 0; b < h.band.size(); ++b)
            {
                const StressBand& s = draw.bands[b];
                const BandHandles& hb = h.band[b];

                hb.freq->store (s.freq);
                hb.gain->store (s.gain);
                hb.q->store (s.q);
                hb.type->store (s.type);
                hb.slope->store (s.slope);
                hb.placement->store (s.placement);
                hb.enabled->store (s.enabled);
                hb.bypass->store (s.bypass);
            }

            if (gainMode != 0)
            {
                gains.nextBlock();
                h.outputGain->store (gains.outputDb);
                h.autoGain->store (gains.autoOn ? 1.0f : 0.0f);
            }

            if (listenDraw != nullptr && listenDraw->mode != 0)
            {
                listenDraw->nextBlock (position, rate);
                p.listenBand.store (listenDraw->band);
                p.analyzerMode.store (listenDraw->analyzer);
            }

            block.setSize (2, m, false, false, true);
            float* const left = block.getWritePointer (0);
            float* const right = block.getWritePointer (1);

            for (int i = 0; i < m; ++i)
            {
                const float l = silent ? 0.0f : (float) ((audio.uniform() * 2.0 - 1.0) * 0.25);
                const float r = silent ? 0.0f : (float) ((audio.uniform() * 2.0 - 1.0) * 0.25);

                left[i] = l;
                right[i] = r;
                keptLeft[(size_t) i] = l;
                keptRight[(size_t) i] = r;
                out.inputPeak = juce::jmax (out.inputPeak, (double) std::abs (l), (double) std::abs (r));
            }

            process (block, midi, viaBypassed);

            const float* const outLeft = block.getReadPointer (0);
            const float* const outRight = block.getReadPointer (1);
            bool acts = false;

            for (int i = 0; i < m; ++i)
            {
                const float l = outLeft[i];
                const float r = outRight[i];

                acts = acts || ! sameBits (l, keptLeft[(size_t) i]) || ! sameBits (r, keptRight[(size_t) i]);

                if (! std::isfinite (l) || ! std::isfinite (r))
                {
                    ++out.nonFinite;
                    continue;
                }

                out.peak = juce::jmax (out.peak, (double) std::abs (l), (double) std::abs (r));
            }

            if (acts)
                ++out.actingBlocks;

            position += m;
            ++out.blocks;
        }

        if (gainsOut != nullptr)
            *gainsOut = gains;

        return out;
    }

    /** runStress on the P14 draw: the stress, then every band off and the
        global bypass off, 0.5 s in blocks of 512.

        Amended in Phase 2.5 (PLAN-2.5 P12): the P14 draw plus output_gain
        (uniform -36 ... +36 dB, each range end with p 0.05) and auto_gain
        (toggled with p 1/100) per block, from their own stream; the stack
        guard is stack + the drawn output gain + 12 dB auto-gain allowance
        (execute-session call: no +12 dB peak margin; up to 256 seeds) (chooseStressSeed24, gain mode 1); afterwards every band
        off, output gain 0 dB and auto gain off, and the output must be the
        input again within 300 ms (the auto-gain smoother snaps at about
        232 ms after its refresh tick).

        Amended in Phase 2.6 (PLAN-2.6 P16): listenBand redrawn with p 1/200
        per block (-1 with p 0.5, else a uniform band) and the analyzer
        (consumer registered, analyzerMode redrawn with p 1/500, never
        drained, so the rings overflow), from their own stream
        (StressListenDraw mode 1); the end clause sets listen -1 first. */
    StressResult runStress24 (Processor& p, const Handles& h, double rate, double seconds, int structEvery)
    {
        StressResult result;
        result.chosen = chooseStressSeed24 (rate, seconds, structEvery, 1);

        if (! result.chosen.ok)
            return result;

        result.listen.begin (1, result.chosen.seed);
        p.setAnalyzerConsumer (true);

        result.outcome = stressLoopCore (p, h, rate, seconds, result.chosen.seed, structEvery, false, 1, &result.gains, &result.listen,
                                         [&p, &result] (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, bool viaBypassed)
                                         {
                                             if (viaBypassed)
                                                 p.processBlockBypassed (buffer, midiMessages);
                                             else
                                                 p.processBlock (buffer, midiMessages);

                                             const eq::ListenState state = p.getListenState();
                                             result.listeningBlocks += state.target >= 0 ? 1 : 0;
                                             result.moveBlocks += state.move != eq::ListenMove::none ? 1 : 0;
                                         });
        result.ran = true;
        result.dropsPre = p.getPreTap().getDropCount();
        result.dropsPost = p.getPostTap().getDropCount();

        // PLAN-2.6 P16: listen off first.
        p.listenBand.store (-1);

        // Every band off, the bypass off, output gain 0 dB and auto gain off
        // (PLAN-2.5 P12): the output must be the input again within 300 ms.
        h.bypass->store (0.0f);
        h.outputGain->store (0.0f);
        h.autoGain->store (0.0f);

        for (const auto& hb : h.band)
            hb.enabled->store (0.0f);

        constexpr int offBlockSize = 512;
        const int numBlocks = (int) std::ceil (0.5 * rate / (double) offBlockSize);

        Xorshift audio;
        audio.state = result.chosen.seed ^ 0x0FF0FF0FF0FF0FF0ull;

        if (audio.state == 0)
            audio.state = 1;

        juce::AudioBuffer<float> block (2, offBlockSize), kept (2, offBlockSize);
        juce::MidiBuffer midi;
        int lastUnequalEnd = 0;

        for (int k = 0; k < numBlocks; ++k)
        {
            for (int channel = 0; channel < 2; ++channel)
            {
                float* const data = block.getWritePointer (channel);

                for (int i = 0; i < offBlockSize; ++i)
                    data[i] = (float) ((audio.uniform() * 2.0 - 1.0) * 0.25);

                kept.copyFrom (channel, 0, block, channel, 0, offBlockSize);
            }

            p.processBlock (block, midi);

            if (! allFinite (block))
                result.offFinite = false;

            if (! sameAudio (block, kept))
                lastUnequalEnd = (k + 1) * offBlockSize;
        }

        result.offEqualFromMs = 1000.0 * (double) lastUnequalEnd / rate;
        p.setAnalyzerConsumer (false);
        p.releaseResources();
        return result;
    }

    void stressRows()
    {
        section ("S5 / S6. random stress and non-finite input through processBlock (Phase 2.3: G5; PLAN-2.3 P9, P13)");

        // S5 (PLAN-2.3; not the state-codec row of Stage 1)
        // Amended in Phase 2.4 (PLAN-2.4 P18, P14): the P14 draw (structure
        // per band with probability 1/50 per block, global bypass toggled with
        // probability 1/200 per block), 60 s per rate (--long 600 s), plus the
        // every-block run (structure redrawn at every block), 10 s per rate in
        // both; the stack guard takes per band the larger peak of its two
        // latest requests. Verdict text unchanged; the tally holds 16 runs.
        //
        // Amended in Phase 2.5 (PLAN-2.5 P12): the same, plus output_gain
        // (uniform -36 ... +36 dB, range ends with p 0.05) and auto_gain
        // (toggled with p 1/100) per block in both runs; the stack guard is
        // stack + the drawn output gain + 12 dB auto-gain allowance,
        // below +600 dB; "all bands off -> memcmp equal within 100 ms" becomes
        // "all bands off, output gain 0 dB, auto gain off -> memcmp equal
        // within 300 ms" (the auto-gain smoother snaps at about 232 ms).
        // Execute-session call (Taylor, PLAN-2.5 P12): the first form added
        // the +12 dB G5 peak margin to the guard as well (never part of the
        // render guard in 2.4); at 600 s no seed in 32 attempts held it at
        // 176.4 and 192 kHz. The margin is dropped and this gain mode tries up
        // to 256 seeds (A3 and every other caller keep 32).
        // See runStress24 and chooseStressSeed24 (gain mode 1).
        //
        // Amended in Phase 2.6 (PLAN-2.6 P16): the same, plus listenBand
        // (redrawn with p 1/200 per block: -1 with p 0.5, else a uniform
        // band) and the analyzer (consumer registered, analyzerMode redrawn
        // with p 1/500, never drained, so the rings overflow) in both runs,
        // from their own stream (StressListenDraw), so the band and gain draws
        // and the stack guard are unchanged; the end clause "all bands off,
        // output 0 dB, auto off -> memcmp equal within 300 ms" sets listen -1
        // first. The tally also asks that listen moved and was heard and that
        // both rings overflowed.
        {
            const double seconds = longRunRequested ? 600.0 : 60.0;
            const double everyBlockSeconds = 10.0;
            const double offLimitMs = 300.0;
            const double rates[8] = { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 };

            std::vector<std::unique_ptr<Processor>> instances;
            std::vector<Handles> handles;

            for (int i = 0; i < 16; ++i)
            {
                instances.push_back (makeBands (2, 0, mixedSpec));
                handles.push_back (handlesOf (*instances.back()));
            }

            std::vector<StressResult> results (16);

            parallelFor (16, [&instances, &handles, &results, &rates, seconds, everyBlockSeconds] (int j)
            {
                const auto slot = (size_t) j;
                const bool everyBlock = j >= 8;
                results[slot] = runStress24 (*instances[slot], handles[slot], rates[slot % 8],
                                             everyBlock ? everyBlockSeconds : seconds, everyBlock ? 1 : 50);
            });

            Tally tally;
            int reseeded = 0;
            double largestPeakDb = -std::numeric_limits<double>::infinity();
            double largestStackDb = 0.0, largestGuardDb = 0.0, slowestOffMs = 0.0;

            for (size_t r = 0; r < 16; ++r)
            {
                const StressResult& s = results[r];
                const bool everyBlock = r >= 8;
                const double runSeconds = everyBlock ? everyBlockSeconds : seconds;
                const juce::String where = fmtG (rates[r % 8] / 1000.0, 5) + " kHz" + (everyBlock ? " every-block" : "");

                if (s.chosen.attempt > 0 || ! s.chosen.ok)
                {
                    ++reseeded;
                    info ("S5 STIMULUS at " + where + ": the draw of an earlier seed reached a guard (stack + output gain + 12) of "
                              + fmtG (s.chosen.rejectedStackDb, 5) + " dB (limit +600); "
                              + (s.chosen.ok ? "seed changed to attempt " + juce::String (s.chosen.attempt) + " (0x"
                                                   + juce::String::toHexString ((juce::int64) s.chosen.seed) + "), nothing else changed"
                                             : juce::String ("no seed in 256 attempts stays below it")));
                }

                if (! s.chosen.ok || ! s.ran)
                {
                    tally.check (false, where + ": STIMULUS FAILURE, the stack guard does not hold (largest guard "
                                            + fmtG (s.chosen.largestStackDb, 5) + " dB)");
                    continue;
                }

                const double peakDb = 20.0 * std::log10 (juce::jmax (s.outcome.peak, 1.0e-300) / juce::jmax (s.outcome.inputPeak, 1.0e-300));
                largestPeakDb = juce::jmax (largestPeakDb, peakDb);
                largestStackDb = juce::jmax (largestStackDb, s.chosen.plainStackDb);
                largestGuardDb = juce::jmax (largestGuardDb, s.chosen.largestStackDb);
                slowestOffMs = juce::jmax (slowestOffMs, s.offEqualFromMs);

                std::printf ("      S5 %-9s %g s, seed attempt %d: largest static stack %.1f dB, guard (stack + output gain + 12) %.1f dB (below +600); output gain drawn %+.1f ... %+.1f dB, auto gain toggled %d times; listenBand redrawn %d times (%lld blocks listening, %lld ending in a move), analyzerMode %d times, tap drops %u / %u; %lld non-finite samples in %lld blocks (%lld acting); peak %+.1f dB re input peak; listen off, all bands off, output 0 dB, auto off: memcmp-equal to input from %.1f ms\n",
                             where.toRawUTF8(), runSeconds, s.chosen.attempt, s.chosen.plainStackDb, s.chosen.largestStackDb,
                             (double) s.gains.lowestDb, (double) s.gains.highestDb, s.gains.autoToggles,
                             s.listen.listenChanges, (long long) s.listeningBlocks, (long long) s.moveBlocks, s.listen.modeChanges,
                             (unsigned) s.dropsPre, (unsigned) s.dropsPost,
                             (long long) s.outcome.nonFinite, (long long) s.outcome.blocks, (long long) s.outcome.actingBlocks,
                             peakDb, s.offEqualFromMs);

                const bool listened = s.listen.listenChanges > 0 && s.listeningBlocks > 0 && s.moveBlocks > 0;
                const bool overflowed = s.dropsPre > 0 && s.dropsPost > 0;

                tally.check (s.outcome.nonFinite == 0 && s.offFinite && s.outcome.actingBlocks > 0 && s.offEqualFromMs <= offLimitMs
                                 && s.gains.autoToggles > 0 && listened && overflowed,
                             where + ": " + juce::String ((juce::int64) s.outcome.nonFinite) + " non-finite samples"
                                 + (s.outcome.actingBlocks > 0 ? "" : "; the bands never act")
                                 + (s.gains.autoToggles > 0 ? "" : "; auto gain never toggled")
                                 + (listened ? "" : "; listen never engaged")
                                 + (overflowed ? "" : "; a ring never overflowed")
                                 + "; bands off, equal to input from " + fmtG (s.offEqualFromMs, 4) + " ms");
            }

            row (tally.bad == 0 && tally.total == 16,
                 "S5 G5 through processBlock: 24 bands, every parameter redrawn (P13), output_gain and auto_gain per block (PLAN-2.5 P12), listenBand and the analyzer (mode redrawn, never drained) per block (PLAN-2.6 P16), blocks 4096 / 512 / 64 / 7 / 1, 8 rates: stack guard (stack + output gain + 12 dB) holds; every sample finite; listen -1, all bands off, output 0 dB, auto off -> memcmp-equal to input within 300 ms",
                 tally.detail() + "  P14 draw (PLAN-2.4) + gains (PLAN-2.5) + listen and taps (PLAN-2.6): " + fmtG (seconds, 4) + " s per rate" + (longRunRequested ? " (--long)" : "")
                     + " + every-block run " + fmtG (everyBlockSeconds, 3) + " s per rate"
                     + "; largest peak " + fmtG (largestPeakDb, 4) + " dB re input peak; largest static stack (fading bands: larger of their two slots) "
                     + fmtG (largestStackDb, 4) + " dB, largest guard " + fmtG (largestGuardDb, 4) + " dB; slowest return to the input "
                     + fmtG (slowestOffMs, 4) + " ms; seed changed at " + juce::String (reseeded) + " of 16 runs");
        }

        // S6 (PLAN-2.3; not the state-codec row of Stage 1)
        {
            constexpr int blockSize = 512, badBlock = 10, window = 48000;
            const double rate = kRenderRate;
            const int total = 3 * 48000;
            const int badAt = badBlock * blockSize + 100;
            const int nextBlock = (badBlock + 1) * blockSize;
            const int steadyFrom = 2 * 48000;
            const double bins[3] = { 100.0, 1000.0, 5000.0 };

            const auto clean = makeNoiseAt (2, total, 0x56A00002, kNoise12);

            auto fresh = makeBands (2, 6, mixedSpec);
            const auto freshOut = render (*fresh, clean, blockSize, false);

            Tally tally;
            juce::String figures;

            struct Bad { const char* name; float value; };
            const Bad bads[2] = { { "NaN", std::numeric_limits<float>::quiet_NaN() },
                                  { "+inf", std::numeric_limits<float>::infinity() } };

            for (const auto& bad : bads)
            {
                juce::AudioBuffer<float> poisoned (clean);
                poisoned.setSample (0, badAt, bad.value);

                auto p = makeBands (2, 6, mixedSpec);
                const auto output = render (*p, poisoned, blockSize, false);

                // The stimulus reached the engine: nothing is replaced, so its block is not finite.
                const bool reached = ! allFinite (sliceOf (output, badBlock * blockSize, blockSize));
                const bool finiteAfter = allFinite (sliceOf (output, nextBlock, total - nextBlock));

                double worstDb = 0.0;
                bool binsFinite = true;

                for (int channel = 0; channel < 2; ++channel)
                    for (const double hz : bins)
                    {
                        const double db = eq::powerToDb (binPower (output, channel, steadyFrom, window, hz, rate)
                                                             / binPower (freshOut, channel, steadyFrom, window, hz, rate));

                        // A NaN must not be lost in a max.
                        if (! std::isfinite (db))
                            binsFinite = false;
                        else
                            worstDb = juce::jmax (worstDb, std::abs (db));
                    }

                tally.check (reached, juce::String (bad.name) + ": the block holding it is finite (the stimulus did not reach the engine)");
                tally.check (finiteAfter, juce::String (bad.name) + ": output not finite from the next block on");
                tally.check (binsFinite && worstDb <= 0.01,
                             juce::String (bad.name) + ": from 2 s on " + fmtG (worstDb, 3) + " dB from a fresh instance"
                                 + (binsFinite ? "" : " (a bin is not finite)"));

                figures += (figures.isEmpty() ? "" : "; ") + juce::String (bad.name) + ": from 2 s on within "
                         + fmtG (worstDb, 3) + " dB of a fresh instance (6 bins)";
            }

            tally.check (activeBands (*fresh) == 6 && ! sameAudio (freshOut, clean), "the 6 bands do not act");

            row (tally.bad == 0 && tally.total == 7,
                 "S6 one block holding a NaN sample, then noise; the same with +inf; 6 mixed bands: output finite from the next block on; from 2 s on within 0.01 dB (single bin) of a fresh instance",
                 tally.detail() + "  " + figures);
        }
    }

    AllocResult runStressAllocWindow (bool plant)
    {
        AllocResult result;

       #if defined (__APPLE__)
        const double rate = 48000.0;
        const double seconds = 20.0;

        const StressSeed chosen = chooseStressSeed (rate, seconds);
        auto p = makeBands (2, 0, mixedSpec);
        const Handles h = handlesOf (*p);

        malloc_logger_t* const previous = malloc_logger;
        allocThread = pthread_self();
        allocCount = 0;
        freeCount = 0;
        malloc_logger = countAllocation;

        // Liveness: the hook must see a deliberate allocation and its release.
        allocArmed = true;
        {
            void* volatile probe = std::malloc (64);
            std::free (probe);
        }
        allocArmed = false;
        result.probeAllocs = allocCount;
        result.probeFrees = freeCount;
        allocCount = 0;
        freeCount = 0;

        bool plantNext = plant;

        // The draw, the parameter stores, the fill and the scan are outside
        // the window (stressLoop); the window is processBlock alone, the
        // first block after prepareToPlay included.
        const StressOutcome outcome = stressLoop (*p, h, rate, seconds, chosen.seed,
                                                  [&p, &plantNext] (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
                                                  {
                                                      allocArmed = true;
                                                      p->processBlock (buffer, midiMessages);

                                                      if (plantNext)
                                                      {
                                                          void* volatile planted = std::malloc (64);
                                                          std::free (planted);
                                                      }

                                                      allocArmed = false;
                                                      plantNext = false;
                                                  });

        malloc_logger = previous;
        p->releaseResources();

        result.available = true;
        result.allocs = allocCount;
        result.frees = freeCount;
        result.blocks = (int) outcome.blocks;
        result.audioOk = chosen.ok && outcome.nonFinite == 0 && outcome.actingBlocks > 0;
       #else
        juce::ignoreUnused (plant);
       #endif

        return result;
    }

    void stressAllocationRows()
    {
        section ("A2. allocation under stress (Phase 2.3: G6, PERF-01)");

        const AllocResult clean = runStressAllocWindow (false);

        if (! clean.available)
        {
            info ("A2 allocation gate SKIPPED: no malloc_logger on this platform");
            info ("A2n allocation control SKIPPED: no malloc_logger on this platform");
            return;
        }

        const bool hookLive = clean.probeAllocs >= 1 && clean.probeFrees >= 1;

        row (hookLive && clean.audioOk && clean.allocs == 0 && clean.frees == 0,
             "A2 processBlock under stress, 20 s of the S5 draw at 48 kHz, first block included, parameter writes outside the window: 0 allocations, 0 frees",
             juce::String (clean.allocs) + " allocations, " + juce::String (clean.frees) + " frees in "
                 + juce::String (clean.blocks) + " windows (malloc_logger, this thread); hook "
                 + (hookLive ? "live" : "DEAD") + " (probe: " + juce::String (clean.probeAllocs) + " / "
                 + juce::String (clean.probeFrees) + ")"
                 + (clean.audioOk ? "" : "; the stress did not run as expected (stack guard, non-finite output, or no acting block)"));

        const AllocResult planted = runStressAllocWindow (true);

        row (planted.allocs >= 1 && planted.frees >= 1,
             "A2n a planted allocation inside the processBlock window is counted (control fires)",
             juce::String (planted.allocs) + " allocations, " + juce::String (planted.frees) + " frees");
    }

    //==========================================================================
    // N7. Noise floor and silence (G7 float, PLAN-2.3 P14; ported from ramp_bench g7)
    //==========================================================================
    struct ToneResidual
    {
        double residualDb = 0.0;    // RMS of what is left after the tone is removed, dBFS
        double amplitude = 0.0;     // of the fitted tone
    };

    // Least-squares removal of the hz component over [start, end).
    ToneResidual toneResidual (const float* y, int start, int end, double hz, double rate)
    {
        long double ss = 0.0L, cc = 0.0L, sc = 0.0L, ys = 0.0L, yc = 0.0L;

        for (int i = start; i < end; ++i)
        {
            const double phase = 2.0 * eq::pi * hz * (double) i / rate;
            const double sn = std::sin (phase), cs = std::cos (phase);
            const double v = (double) y[i];

            ss += (long double) (sn * sn);
            cc += (long double) (cs * cs);
            sc += (long double) (sn * cs);
            ys += (long double) (v * sn);
            yc += (long double) (v * cs);
        }

        const long double det = ss * cc - sc * sc;
        const long double a = (ys * cc - yc * sc) / det;
        const long double b = (yc * ss - ys * sc) / det;
        long double r2 = 0.0L;

        for (int i = start; i < end; ++i)
        {
            const double phase = 2.0 * eq::pi * hz * (double) i / rate;
            const long double e = (long double) y[i] - a * (long double) std::sin (phase) - b * (long double) std::cos (phase);
            r2 += e * e;
        }

        ToneResidual result;
        result.residualDb = 10.0 * std::log10 ((double) (r2 / (long double) (end - start)) + 1.0e-300);
        result.amplitude = std::sqrt ((double) (a * a + b * b));
        return result;
    }

    double medianOf (std::vector<double>& values)
    {
        if (values.empty())
            return 0.0;

        const auto middle = values.begin() + (std::ptrdiff_t) (values.size() / 2);
        std::nth_element (values.begin(), middle, values.end());
        return *middle;
    }

    struct SilenceCost
    {
        double noiseMedian = 0.0;       // seconds per block
        double silenceMedian = 0.0;
        bool finite = true;

        double ratio() const    { return ratioOf (silenceMedian, noiseMedian); }
    };

    // 24 bells at 10 - 33 Hz, Q 40, +12 dB, 192 kHz stereo, blocks of 512:
    // 10 s of noise (median block time over its last 5 s), then 60 s of
    // silence (median over its last 5 s). Only processBlock is timed.
    SilenceCost silenceCostTrial()
    {
        constexpr int blockSize = 512;
        const double rate = 192000.0;
        const int blocksPerSecond = (int) (rate / (double) blockSize);     // 375
        const int noiseBlocks = 10 * blocksPerSecond;
        const int silenceBlocks = 60 * blocksPerSecond;
        const int scored = 5 * blocksPerSecond;

        auto p = makeBands (2, 24, [] (int index) { return makeSpec (kBell, 10.0f + (float) index, 12.0f, 40.0f, 1, kStereo); });
        const auto noise = makeNoiseAt (2, blockSize, 0x0E7000002, kNoise12);
        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;

        std::vector<double> noiseTimes, silenceTimes;
        noiseTimes.reserve ((size_t) scored);
        silenceTimes.reserve ((size_t) scored);

        SilenceCost cost;
        p->prepareToPlay (rate, blockSize);

        for (int k = 0; k < noiseBlocks + silenceBlocks; ++k)
        {
            const bool withNoise = k < noiseBlocks;

            if (withNoise)
            {
                for (int channel = 0; channel < 2; ++channel)
                    block.copyFrom (channel, 0, noise, channel, 0, blockSize);
            }
            else
            {
                block.clear();
            }

            const auto t0 = std::chrono::steady_clock::now();
            p->processBlock (block, midi);
            const auto t1 = std::chrono::steady_clock::now();
            const double elapsed = std::chrono::duration<double> (t1 - t0).count();

            if (withNoise && k >= noiseBlocks - scored)
                noiseTimes.push_back (elapsed);

            if (! withNoise && k >= noiseBlocks + silenceBlocks - scored)
                silenceTimes.push_back (elapsed);

            if (k == noiseBlocks - 1 || k == noiseBlocks + silenceBlocks - 1)
                cost.finite = cost.finite && allFinite (block);
        }

        p->releaseResources();

        cost.noiseMedian = medianOf (noiseTimes);
        cost.silenceMedian = medianOf (silenceTimes);
        return cost;
    }

    void noiseFloorRows()
    {
        section ("N7. noise floor and silence (Phase 2.3: G7 float, DSP-05)");

        // (i) 192 kHz, one bell at 10 Hz, Q 40, +12 dB, static; a 1 kHz sine at
        // -6 dBFS with a 2 s raised-cosine fade-in, 7 s; the 1 kHz component
        // removed from the last 3 s by least squares.
        {
            const double rate = 192000.0;
            const int length = (int) std::lround (7.0 * rate);
            const int start = (int) std::lround (4.0 * rate);

            juce::AudioBuffer<float> input (1, length);

            {
                float* x = input.getWritePointer (0);

                for (int i = 0; i < length; ++i)
                {
                    const double t = (double) i / rate;
                    const double fade = t < 2.0 ? 0.5 - 0.5 * std::cos (eq::pi * t / 2.0) : 1.0;
                    x[i] = (float) (0.5 * fade * std::sin (2.0 * eq::pi * 1000.0 * t));
                }
            }

            auto p = makeBands (1, 1, [] (int) { return makeSpec (kBell, 10.0f, 12.0f, 40.0f, 1, kStereo); });
            const auto output = render (*p, input, 512, false, rate);

            const ToneResidual out = toneResidual (output.getReadPointer (0), start, length, 1000.0, rate);
            const ToneResidual in = toneResidual (input.getReadPointer (0), start, length, 1000.0, rate);

            const bool stimulusOk = activeBands (*p) == 1 && allFinite (output) && ! sameAudio (output, input)
                                 && std::abs (out.amplitude - 0.5) < 0.005;

            row (stimulusOk && out.residualDb <= -140.0,
                 "N7 (i) bell 10 Hz Q 40 +12 dB at 192 kHz, 1 kHz sine at -6 dBFS: residual of the float output after removing 1 kHz, last 3 s of 7, <= -140 dBFS RMS",
                 fmtG (out.residualDb, 5) + " dBFS RMS (fitted amplitude " + fmtG (out.amplitude, 6) + "); the float input's own residual is "
                     + fmtG (in.residualDb, 5) + " dBFS" + (stimulusOk ? "" : "; stimulus guard failed"));
        }

        // (ii) wall clock (PLAN-2.3 R6): medians, three trials, the best ratio.
        {
            double best = std::numeric_limits<double>::infinity();
            bool finite = true;
            juce::String trials;

            for (int trial = 0; trial < 3; ++trial)
            {
                const SilenceCost cost = silenceCostTrial();
                best = juce::jmin (best, cost.ratio());
                finite = finite && cost.finite;

                trials += (trials.isEmpty() ? "" : "; ") + fmtG (cost.ratio(), 4) + "x (silence "
                        + fmtG (cost.silenceMedian * 1.0e6, 4) + " us, noise " + fmtG (cost.noiseMedian * 1.0e6, 4) + " us per block)";
            }

            row (finite && best <= 1.2,
                 "N7 (ii) 24 bells 10 - 33 Hz Q 40, 192 kHz stereo: median block time over the last 5 s of a 60 s silence <= 1.2x the median with noise (best of three)",
                 "best " + fmtG (best, 4) + "x; trials: " + trials + (finite ? "" : "; OUTPUT NOT FINITE"));
        }
    }

    //==========================================================================
    // X2. CPU, sweeping (INFO; PLAN-2.3 P17; ported from ramp_bench cpu)
    //==========================================================================
    void cpuInfoSweeping()
    {
        section ("X2. CPU, sweeping (INFO, not gated; G10 binds in Stage 4)");

        constexpr int blockSize = 512;
        const double rate = 48000.0;
        const int numBlocks = (int) (20.0 * rate / (double) blockSize);
        const double audioSeconds = (double) numBlocks * (double) blockSize / rate;
        const auto noise = makeNoiseAt (2, blockSize, 0x0C9300002, kNoise12);

        struct Config { const char* name; int type; int slope; float gain; const char* benchSweeping; const char* benchSettled; };

        // Settings and motion as research-2.3 ramp_bench modeCpu (RESEARCH-2.3 Q5).
        const Config configs[] =
        {
            { "24 cuts x 96 dB/oct (PERF-02)", kHighCut,   7, 0.0f, "4.35", "2.48" },
            { "24 shelves x 96 dB/oct",        kHighShelf, 7, 9.0f, "23.0", "5.10" },
            { "24 bells",                      kBell,      1, 9.0f, "4.46", "0.71" }
        };

        for (const auto& c : configs)
        {
            double percent[2] = { 0.0, 0.0 };       // sweeping, settled
            bool finite = true;

            for (int mode = 0; mode < 2; ++mode)
            {
                const bool sweeping = mode == 0;

                auto p = makeBands (2, 24, [&c] (int index)
                {
                    return makeSpec (c.type, 40.0f * std::pow (300.0f, (float) index / 23.0f), c.gain, 1.0f, c.slope, kStereo);
                });

                const Handles h = handlesOf (*p);
                juce::AudioBuffer<float> block (2, blockSize);
                juce::MidiBuffer midi;
                double seconds = 0.0;

                p->prepareToPlay (rate, blockSize);

                for (int k = 0; k < numBlocks; ++k)
                {
                    if (sweeping)
                    {
                        // A 2 s triangle in log frequency, 40 Hz - 12 kHz, one phase per
                        // band; gain-bearing types move their gain with it. Written
                        // before every block, so every band redesigns at every tick.
                        const double position = (double) (k * blockSize) / rate;

                        for (int b = 0; b < Processor::kNumBands; ++b)
                        {
                            const double phase = std::fmod (position / 2.0 + (double) b / 24.0, 1.0);
                            const double triangle = phase < 0.5 ? 2.0 * phase : 2.0 - 2.0 * phase;
                            const BandHandles& hb = h.band[(size_t) b];

                            hb.freq->store ((float) (40.0 * std::pow (300.0, triangle)));

                            if (c.gain != 0.0f)
                                hb.gain->store ((float) ((double) c.gain * (0.5 + triangle)));
                        }
                    }

                    for (int channel = 0; channel < 2; ++channel)
                        block.copyFrom (channel, 0, noise, channel, 0, blockSize);

                    const auto t0 = std::chrono::steady_clock::now();
                    p->processBlock (block, midi);
                    const auto t1 = std::chrono::steady_clock::now();

                    seconds += std::chrono::duration<double> (t1 - t0).count();
                }

                p->releaseResources();

                percent[mode] = 100.0 * seconds / audioSeconds;
                finite = finite && allFinite (block);
            }

            info (juce::String ("X-cpu2 48 kHz stereo, 512-sample blocks, 20 s, ") + c.name + ": sweeping "
                      + fmtG (percent[0], 3) + " % of one core (bench " + c.benchSweeping + "; line 35), settled "
                      + fmtG (percent[1], 3) + " % (bench " + c.benchSettled + "; compare 2.2's X-cpu)"
                      + (finite ? "" : "  OUTPUT NOT FINITE"));
        }
    }

    //==========================================================================
    //==========================================================================
    // Stage 2, Phase 2.4: structural crossfades and bypass (PLAN-2.4 T7)
    //==========================================================================
    //==========================================================================

    //==========================================================================
    // Workers with an index
    //==========================================================================
    /** How many workers (and so plugin instances) a pool of `count` jobs uses. */
    int workerCount (int count)
    {
        const int hardware = (int) std::thread::hardware_concurrency();
        return juce::jlimit (1, 12, juce::jmin (juce::jmax (1, count), hardware > 0 ? hardware : 1));
    }

    /** parallelFor with the worker's index: job (index, worker), worker in
        0 ... workers - 1, one job at a time per worker. The caller constructs
        one plugin instance per worker on the main thread; a job rewrites every
        parameter it uses before it renders. Jobs print nothing. */
    template <typename JobFn>
    void parallelForWorkers (int count, int workers, JobFn&& job)
    {
        if (count <= 0 || workers <= 0)
            return;

        std::atomic<int> next { 0 };

        const auto work = [&next, &job, count] (int worker)
        {
            for (;;)
            {
                const int mine = next.fetch_add (1);

                if (mine >= count)
                    return;

                job (mine, worker);
            }
        };

        std::vector<std::thread> pool;

        for (int w = 1; w < workers; ++w)
            pool.emplace_back (work, w);

        work (0);

        for (auto& thread : pool)
            thread.join();
    }

    //==========================================================================
    // Readings (fade_bench maxD2 and kappa2)
    //==========================================================================
    /** The largest |y[n + 1] - 2 y[n] + y[n - 1]| from `from` on. */
    double maxD2Of (const std::vector<double>& y, size_t from)
    {
        double d = 0.0;

        for (size_t i = juce::jmax (from, (size_t) 1); i + 1 < y.size(); ++i)
        {
            const double v = std::abs (y[i + 1] - 2.0 * y[i] + y[i - 1]);

            if (! (v <= d))
                d = v;
        }

        return d;
    }

    struct StereoReading
    {
        double d2 = 0.0;        // largest second difference, worse channel
        double kappa = 0.0;     // local kappa, worse channel
    };

    StereoReading readStereo (const std::vector<double>& left, const std::vector<double>& right, double rate, size_t from)
    {
        StereoReading r;
        r.d2 = juce::jmax (maxD2Of (left, from), maxD2Of (right, from));
        r.kappa = juce::jmax (localKappa (left, rate, from, left.size()), localKappa (right, rate, from, right.size()));
        return r;
    }

    StereoReading readStereo (const juce::AudioBuffer<float>& y, double rate, size_t from)
    {
        return readStereo (widened (y, 0), widened (y, 1), rate, from);
    }

    //==========================================================================
    // The step rows (PLAN-2.4 P11; fade_bench stepRows, deep, stimulus)
    //==========================================================================
    struct StepCfg
    {
        eq::BandSettings s { eq::Type::bell, 1000.0, 0.0, 1.0, 0 };
        eq::Placement p = eq::Placement::stereo;
        bool active = true;
        bool bypass = false;        // global
    };

    struct StepRow
    {
        std::string name;
        int group = 0;
        StepCfg a, b;
        int copies = 1;
        bool viaBandBypass = false;     // an inactive band is bNN_bypass on (else bNN_enabled off)
        bool voidAt1k = false;          // P13 (c): void at 1 kHz (the 10 Hz, Q 40 bell)
    };

    constexpr int kGroupType = 0, kGroupSlope = 1, kGroupPlace = 2, kGroupEnable = 3, kGroupBypass = 4,
                  kGroupEnable24 = 5, kGroupBandBypass = 6, kNumStepGroups = 7;

    const char* const kStepGroupNames[kNumStepGroups] = { "Type pairs", "Slope pairs", "Placement pairs", "Enable",
                                                          "Global bypass", "Enable, 24 bells", "Band bypass" };

    constexpr double kStepRate = 48000.0;
    constexpr double kStepLead = 0.5, kStepSeconds = 0.100;
    constexpr int kNumSteps = 8;

    // Every step render runs gain_scale 200 % with the raw gain at half the
    // bench's effective gain: gEff = raw * 200 / 100 is then the bench's value
    // exactly, also where it lies outside the raw +-30 dB (Tilt +36 / -36).
    constexpr float kStepGainScale = 200.0f;

    StepCfg deepCfg (eq::Type t, double tone, double gain = 18.0)
    {
        StepCfg c;
        c.s.type = t;
        c.s.gainDbEff = gain;
        c.s.q = 1.0;
        c.s.slopeIndex = eq::usesSlope (t) ? 1 : 0;

        switch (t)
        {
            case eq::Type::bell:       c.s.freqHz = tone;  break;
            case eq::Type::lowShelf:   c.s.freqHz = 2.0 * tone;  break;
            case eq::Type::highShelf:  c.s.freqHz = 0.5 * tone;  break;
            case eq::Type::lowCut:     c.s.freqHz = tone;  c.s.slopeIndex = 3;  break;
            case eq::Type::highCut:    c.s.freqHz = tone;  c.s.slopeIndex = 3;  break;
            case eq::Type::notch:      c.s.freqHz = 1.15 * tone;  break;
            case eq::Type::bandPass:   c.s.freqHz = 3.0 * tone;  break;
            case eq::Type::tilt:       c.s.freqHz = 0.25 * tone;  c.s.gainDbEff = 2.0 * gain > 60.0 ? 60.0 : 2.0 * gain;  break;
        }

        return c;
    }

    std::string typeNameOf (eq::Type t)            { return kTypeNames[(int) t]; }
    std::string placementNameOf (eq::Placement p)  { return kPlacementNames[(int) p]; }

    /** The 15 subjects of the Enable, Global bypass and band-bypass groups. */
    std::vector<std::pair<std::string, StepCfg>> stepSubjects (double tone)
    {
        std::vector<std::pair<std::string, StepCfg>> subjects;
        subjects.push_back ({ "Bell +18", deepCfg (eq::Type::bell, tone) });
        subjects.push_back ({ "Bell -18", deepCfg (eq::Type::bell, tone, -18.0) });
        { StepCfg c = deepCfg (eq::Type::bell, tone);  c.s.q = 40.0;  subjects.push_back ({ "Bell +18 Q 40", c }); }
        { StepCfg c = deepCfg (eq::Type::bell, tone);  c.s.freqHz = 10.0;  c.s.q = 40.0;  c.s.gainDbEff = 12.0;  subjects.push_back ({ "Bell 10 Hz Q 40 +12", c }); }
        { StepCfg c = deepCfg (eq::Type::lowCut, tone);  c.s.freqHz = 30.0;  c.s.slopeIndex = 7;  subjects.push_back ({ "LowCut 96 at 30 Hz", c }); }
        { StepCfg c = deepCfg (eq::Type::lowCut, tone);  c.s.slopeIndex = 7;  subjects.push_back ({ "LowCut 96 at tone", c }); }
        { StepCfg c = deepCfg (eq::Type::lowCut, tone);  c.s.slopeIndex = 7;  c.s.freqHz = 0.8 * tone;  c.s.q = 4.0;  subjects.push_back ({ "LowCut 96 Q 4 below tone", c }); }
        { StepCfg c = deepCfg (eq::Type::highCut, tone);  c.s.slopeIndex = 7;  subjects.push_back ({ "HighCut 96 at tone", c }); }
        { StepCfg c = deepCfg (eq::Type::highCut, tone);  c.s.slopeIndex = 0;  subjects.push_back ({ "HighCut 6 at tone", c }); }
        subjects.push_back ({ "HighShelf 12 +18", deepCfg (eq::Type::highShelf, tone) });
        { StepCfg c = deepCfg (eq::Type::highShelf, tone);  c.s.slopeIndex = 7;  subjects.push_back ({ "HighShelf 96 +18", c }); }
        subjects.push_back ({ "LowShelf 12 -18", deepCfg (eq::Type::lowShelf, tone, -18.0) });
        subjects.push_back ({ "Notch near tone", deepCfg (eq::Type::notch, tone) });
        subjects.push_back ({ "BandPass at 3x tone", deepCfg (eq::Type::bandPass, tone) });
        subjects.push_back ({ "Tilt 12 +36", deepCfg (eq::Type::tilt, tone) });
        return subjects;
    }

    const char* const kVoidSubject = "Bell 10 Hz Q 40 +12";

    /** fade_bench stepRows ("all"), in its order, plus the band-bypass group:
        242 + 15 rows. */
    std::vector<StepRow> stepRowsFor (double tone)
    {
        std::vector<StepRow> rows;
        const eq::Type types[8] = { eq::Type::bell, eq::Type::lowShelf, eq::Type::highShelf, eq::Type::lowCut,
                                    eq::Type::highCut, eq::Type::notch, eq::Type::bandPass, eq::Type::tilt };

        const auto add = [&rows] (const std::string& name, int group, const StepCfg& a, const StepCfg& b,
                                  int copies, bool viaBandBypass, bool voidAt1k)
        {
            StepRow r;
            r.name = name;
            r.group = group;
            r.a = a;
            r.b = b;
            r.copies = copies;
            r.viaBandBypass = viaBandBypass;
            r.voidAt1k = voidAt1k;
            rows.push_back (r);
        };

        for (int i = 0; i < 8; ++i)
            for (int j = i + 1; j < 8; ++j)
                add ("type " + typeNameOf (types[i]) + " <> " + typeNameOf (types[j]), kGroupType,
                     deepCfg (types[i], tone), deepCfg (types[j], tone), 1, false, false);

        for (int i = 0; i < 8; ++i)
            for (int j = i + 1; j < 8; ++j)
                if (eq::isGainType (types[i]) || eq::isGainType (types[j]))
                    add ("type -18 " + typeNameOf (types[i]) + " <> " + typeNameOf (types[j]), kGroupType,
                         deepCfg (types[i], tone, -18.0), deepCfg (types[j], tone, -18.0), 1, false, false);

        for (const eq::Type t : { eq::Type::lowCut, eq::Type::highCut, eq::Type::lowShelf, eq::Type::highShelf, eq::Type::tilt })
            for (int i = 0; i < 8; ++i)
                for (int j = i + 1; j < 8; ++j)
                {
                    StepCfg a = deepCfg (t, tone), b = a;
                    a.s.slopeIndex = i;
                    b.s.slopeIndex = j;
                    add ("slope " + typeNameOf (t) + " " + std::to_string (6 * eq::slopeOrder[i]) + " <> "
                             + std::to_string (6 * eq::slopeOrder[j]), kGroupSlope, a, b, 1, false, false);
                }

        for (int v = 0; v < 2; ++v)
            for (int i = 0; i < 5; ++i)
                for (int j = i + 1; j < 5; ++j)
                {
                    StepCfg a = v == 0 ? deepCfg (eq::Type::bell, tone) : deepCfg (eq::Type::lowCut, tone);

                    if (v == 1)
                        a.s.slopeIndex = 7;

                    StepCfg b = a;
                    a.p = (eq::Placement) i;
                    b.p = (eq::Placement) j;
                    add (std::string ("place ") + (v == 0 ? "Bell +18 " : "LowCut 96 ") + placementNameOf (a.p) + " <> "
                             + placementNameOf (b.p), kGroupPlace, a, b, 1, false, false);
                }

        const auto subjects = stepSubjects (tone);

        for (const auto& sub : subjects)
        {
            StepCfg off = sub.second;
            off.active = false;
            add ("enable " + sub.first, kGroupEnable, off, sub.second, 1, false, sub.first == kVoidSubject);
        }

        for (const auto& sub : subjects)
        {
            StepCfg byp = sub.second;
            byp.bypass = true;
            add ("global bypass, " + sub.first, kGroupBypass, sub.second, byp, 1, false, sub.first == kVoidSubject);
        }

        {
            StepCfg c = deepCfg (eq::Type::bell, tone, 1.0);
            c.s.q = 4.0;
            StepCfg byp = c;
            byp.bypass = true;
            add ("global bypass, 24 x Bell +1 Q 4", kGroupBypass, c, byp, 24, false, false);

            StepCfg off = c;
            off.active = false;
            add ("enable 24 x Bell +1 Q 4", kGroupEnable24, off, c, 24, false, false);
        }

        // The render harness's own group: the same 15 subjects through bNN_bypass.
        for (const auto& sub : subjects)
        {
            StepCfg off = sub.second;
            off.active = false;
            add ("band bypass " + sub.first, kGroupBandBypass, off, sub.second, 1, true, sub.first == kVoidSubject);
        }

        return rows;
    }

    /** 0: A, 1: B. motion 0: A, then B / A alternating every 100 ms from
        0.5 s, 8 steps (fade_bench renderSteps, its float arithmetic included);
        1: held at A; 2: held at B. */
    int stepState (int motion, int position, double rate)
    {
        if (motion == 2)
            return 1;

        if (motion == 1)
            return 0;

        const double t = (double) position / rate;

        if (t < kStepLead)
            return 0;

        const int k = (int) ((t - kStepLead) / kStepSeconds);
        return k < kNumSteps ? ((k + 1) & 1) : (kNumSteps & 1);
    }

    int stepLength (double rate)
    {
        return (int) ((kStepLead + (double) kNumSteps * kStepSeconds + 0.3) * rate);
    }

    /** L: the tone at -6 dBFS; R: the same tone at -10.5 dBFS, 1.1 rad later. */
    juce::AudioBuffer<float> stepStimulus (double tone, double rate, int length)
    {
        juce::AudioBuffer<float> buffer (2, length);
        float* const left = buffer.getWritePointer (0);
        float* const right = buffer.getWritePointer (1);

        for (int i = 0; i < length; ++i)
        {
            const double t = (double) i / rate;
            left[i] = (float) (0.5 * std::sin (2.0 * eq::pi * tone * t));
            right[i] = (float) (0.3 * std::sin (2.0 * eq::pi * tone * t + 1.1));
        }

        return buffer;
    }

    /** The raw values of band `band` (0-based) for configuration c of row r. */
    eq::RawBand stepRawOf (const StepRow& r, const StepCfg& c, int band)
    {
        eq::RawBand raw;
        raw.type = (float) (int) c.s.type;
        raw.freq = (float) c.s.freqHz;
        raw.gain = (float) (c.s.gainDbEff / 2.0);
        raw.q = (float) c.s.q;
        raw.slope = (float) c.s.slopeIndex;
        raw.placement = (float) (int) c.p;

        if (band >= r.copies)
        {
            raw.enabled = 0.0f;
            raw.bypass = 0.0f;
        }
        else if (r.viaBandBypass)
        {
            raw.enabled = 1.0f;
            raw.bypass = c.active ? 0.0f : 1.0f;
        }
        else
        {
            raw.enabled = c.active ? 1.0f : 0.0f;
            raw.bypass = 0.0f;
        }

        return raw;
    }

    void storeRaw (const BandHandles& hb, const eq::RawBand& raw)
    {
        hb.enabled->store (raw.enabled);
        hb.bypass->store (raw.bypass);
        hb.type->store (raw.type);
        hb.freq->store (raw.freq);
        hb.gain->store (raw.gain);
        hb.q->store (raw.q);
        hb.slope->store (raw.slope);
        hb.placement->store (raw.placement);
    }

    /** Every parameter the step rows use. bypassViaEntry: the global bypass
        arrives through processBlockBypassed, the parameter stays off. */
    void storeStepCfg (const Handles& h, const StepRow& r, const StepCfg& c, bool bypassViaEntry)
    {
        h.gainScale->store (kStepGainScale);
        h.bypass->store (c.bypass && ! bypassViaEntry ? 1.0f : 0.0f);

        for (int b = 0; b < Processor::kNumBands; ++b)
            storeRaw (h.band[(size_t) b], stepRawOf (r, c, b));
    }

    eq::BandInput stepInputOf (const StepRow& r, const StepCfg& c, int band)
    {
        eq::BandInput in = eq::makeBandInput (stepRawOf (r, c, band), kStepGainScale, 0.0);
        in.placement = eq::effectivePlacement (in.placement, true);
        return in;
    }

    juce::AudioBuffer<float> renderStepMotion (Processor& p, const Handles& h, const juce::AudioBuffer<float>& input,
                                               double rate, const StepRow& r, int motion, int blockSize, bool bypassViaEntry)
    {
        storeStepCfg (h, r, r.a, bypassViaEntry);

        return renderWith (p, input, rate, blockSize,
                           [blockSize] (int, int) { return blockSize; },
                           [&h, &r, motion, rate, bypassViaEntry] (BlockInfo& current)
                           {
                               const StepCfg& c = stepState (motion, current.position, rate) != 0 ? r.b : r.a;
                               storeStepCfg (h, r, c, bypassViaEntry);
                               current.viaBypassed = bypassViaEntry && c.bypass;
                           });
    }

    //==========================================================================
    // hardStepRender (PLAN-2.4 P10): the Phase 2.3 rule on plain eq::Band +
    // eq::Router, double. Per host block start, per band: structural ->
    // snapTo + reset (inactive: state cleared, not processed); continuous ->
    // setTarget, ramped on the tick grid; global bypass a hard switch (the
    // block untouched, every band's state cleared and its ramp snapped).
    //==========================================================================
    struct HardStepScript
    {
        std::vector<int> blockStart;
        std::vector<std::vector<eq::BandInput>> bands;      // [block][band]
        std::vector<char> bypass;                           // [block]
    };

    HardStepScript stepScriptOf (const StepRow& r, double rate, int length, int blockSize)
    {
        HardStepScript script;

        for (int position = 0; position < length; position += blockSize)
        {
            const StepCfg& c = stepState (0, position, rate) != 0 ? r.b : r.a;
            std::vector<eq::BandInput> perBand;

            for (int b = 0; b < r.copies; ++b)
                perBand.push_back (stepInputOf (r, c, b));

            script.blockStart.push_back (position);
            script.bands.push_back (perBand);
            script.bypass.push_back ((char) (c.bypass ? 1 : 0));
        }

        return script;
    }

    std::array<std::vector<double>, 2> hardStepRender (const std::vector<double>& inL, const std::vector<double>& inR,
                                                       bool stereo, double rate, const HardStepScript& script)
    {
        if (script.blockStart.empty() || script.blockStart[0] != 0
            || script.bands.size() != script.blockStart.size() || script.bypass.size() != script.blockStart.size())
            return {};

        std::array<std::vector<double>, 2> out { inL, inR };

        const size_t numBands = script.bands[0].size();
        eq::Designer designer;
        designer.prepare (rate);

        const int grid = eq::controlGridSize (rate);
        const int rampTicks = eq::rampLengthTicks (rate);

        std::vector<eq::Band> bands (numBands);
        std::vector<eq::BandInput> held (numBands);
        std::vector<char> designed (numBands, 0);

        for (auto& band : bands)
        {
            band.setControlGrid (grid, rampTicks);
            band.reset();
        }

        const size_t length = inL.size();

        for (size_t k = 0; k < script.blockStart.size(); ++k)
        {
            const auto start = (size_t) script.blockStart[k];
            const size_t end = k + 1 < script.blockStart.size() ? juce::jmin (length, (size_t) script.blockStart[k + 1]) : length;

            if (start >= length || script.bands[k].size() != numBands)
                break;

            for (size_t b = 0; b < numBands; ++b)
            {
                const eq::BandInput& next = script.bands[k][b];
                eq::Band& band = bands[b];

                if (designed[b] == 0)
                {
                    if (next.active)
                    {
                        band.snapTo (next.settings, designer);
                        band.reset();
                    }

                    held[b] = next;
                    designed[b] = 1;
                    continue;
                }

                const eq::BandChange change = eq::classifyBandChange (held[b], next);

                if (change == eq::BandChange::structural)
                {
                    if (next.active)
                        band.snapTo (next.settings, designer);
                    else
                        band.snapToTarget (designer);

                    band.reset();
                }
                else if (change == eq::BandChange::continuous && next.active)
                {
                    band.setTarget (next.settings);
                }

                held[b] = next;
            }

            if (script.bypass[k] != 0)
            {
                for (auto& band : bands)
                {
                    band.reset();
                    band.snapToTarget (designer);
                }

                continue;
            }

            size_t position = start;

            while (position < end)
            {
                const auto phase = (int) (position % (size_t) grid);

                if (phase == 0)
                    for (size_t b = 0; b < numBands; ++b)
                        if (held[b].active)
                            bands[b].controlTick (designer);

                const size_t m = juce::jmin (end - position, (size_t) (grid - phase));

                for (size_t b = 0; b < numBands; ++b)
                    if (held[b].active)
                        eq::Router::process (bands[b], held[b].placement, out[0].data() + position,
                                             stereo ? out[1].data() + position : nullptr, (int) m, stereo);

                position += m;
            }
        }

        return out;
    }

    //==========================================================================
    // fadeRefRender (PLAN-2.4 P12). Two static chains, each Designer::design
    // -> toRuntime -> svfTick / onePoleTick; the placement algebra of the
    // ARCHITECTURE "Processing Chain" table and the weight formula of P2,
    // re-stated here. The outgoing chain runs from sample 0, the incoming one
    // from rest at the fade's first sample; an inactive side is the dry input.
    // No two-slot band, fade table, bypass fade, eq::Band or eq::Router.
    //==========================================================================
    /** N re-stated: 20 ms rounded to nearest, at least 1. */
    int refFadeLength (double rate)
    {
        return juce::jmax (1, (int) std::lround (0.020 * rate));
    }

    /** The weight of the incoming side at fade sample n of N, re-stated. */
    double refFadeWeight (int n, int fadeLength)
    {
        const double angle = eq::pi * (double) (n + 1) / (double) (fadeLength + 1);
        return 0.5 - 0.5 * std::cos (angle);
    }

    struct RefChain
    {
        int numSections = 0;
        eq::SectionRuntime runtime[eq::maxSections];
        double ic1[2][eq::maxSections] = {};
        double ic2[2][eq::maxSections] = {};
        double s[2][eq::maxSections] = {};

        double tick (int lane, double x)
        {
            double v = x;

            for (int i = 0; i < numSections; ++i)
            {
                if (runtime[i].hasSvf)
                    v = eq::svfTick (runtime[i].svf, ic1[lane][i], ic2[lane][i], v);

                if (runtime[i].hasOnePole)
                    v = eq::onePoleTick (runtime[i].onePole, s[lane][i], v);
            }

            return v;
        }
    };

    void prepareRefChain (RefChain& chain, const eq::Designer& designer, const eq::BandInput& in)
    {
        chain.numSections = 0;

        if (! in.active)
            return;

        eq::BandDesign design {};
        designer.design (in.settings, design);
        chain.numSections = juce::jlimit (0, (int) eq::maxSections, design.numSections);

        for (int i = 0; i < chain.numSections; ++i)
            chain.runtime[i] = eq::toRuntime (design.params[(size_t) i]);
    }

    /** One sample of one side: Stereo L <- F(L), R <- F(R); Left / Right one
        lane; Mid m = (L + R) / 2, d = F(m) - m, L += d, R += d; Side
        s = (L - R) / 2, d = F(s) - s, L += d, R -= d. Mono: lane 0. */
    void refApply (RefChain& chain, const eq::BandInput& in, bool stereo, double l, double r, double& yl, double& yr)
    {
        yl = l;
        yr = r;

        if (! in.active)
            return;

        if (! stereo)
        {
            yl = chain.tick (0, l);
            return;
        }

        switch (in.placement)
        {
            case eq::Placement::left:
                yl = chain.tick (0, l);
                return;

            case eq::Placement::right:
                yr = chain.tick (1, r);
                return;

            case eq::Placement::mid:
            {
                const double m = 0.5 * (l + r);
                const double d = chain.tick (0, m) - m;
                yl = l + d;
                yr = r + d;
                return;
            }

            case eq::Placement::side:
            {
                const double sv = 0.5 * (l - r);
                const double d = chain.tick (0, sv) - sv;
                yl = l + d;
                yr = r - d;
                return;
            }

            case eq::Placement::stereo:
                break;
        }

        yl = chain.tick (0, l);
        yr = chain.tick (1, r);
    }

    std::array<std::vector<double>, 2> fadeRefRender (const std::vector<double>& inL, const std::vector<double>& inR,
                                                      double rate, const eq::BandInput& from, const eq::BandInput& to,
                                                      size_t fadeStart, int fadeLength)
    {
        if (inL.size() != inR.size() || fadeLength < 1)
            return {};

        const auto designer = designerFor (rate);
        auto outgoing = std::make_unique<RefChain>();
        auto incoming = std::make_unique<RefChain>();
        prepareRefChain (*outgoing, *designer, from);
        prepareRefChain (*incoming, *designer, to);

        std::array<std::vector<double>, 2> out { std::vector<double> (inL.size(), 0.0), std::vector<double> (inL.size(), 0.0) };

        for (size_t i = 0; i < inL.size(); ++i)
        {
            double al = 0.0, ar = 0.0;
            refApply (*outgoing, from, true, inL[i], inR[i], al, ar);

            if (i < fadeStart)
            {
                out[0][i] = al;
                out[1][i] = ar;
                continue;
            }

            double bl = 0.0, br = 0.0;
            refApply (*incoming, to, true, inL[i], inR[i], bl, br);

            const size_t into = i - fadeStart;

            if (into < (size_t) fadeLength)
            {
                const double w = refFadeWeight ((int) into, fadeLength);
                out[0][i] = al + w * (bl - al);
                out[1][i] = ar + w * (br - ar);
            }
            else
            {
                out[0][i] = bl;
                out[1][i] = br;
            }
        }

        return out;
    }

    //==========================================================================
    // XR-g3, XR-g3k, XR-g3h, XR-g3b. G3 step rows (PLAN-2.4 P11, P13)
    //==========================================================================
    struct StepJob
    {
        int tone = 0;               // 0: 50 Hz, 1: 1 kHz
        int rowIndex = 0;
        int blockSize = 32;
        bool viaEntry = false;      // XR-g3b: the bypassed blocks arrive through processBlockBypassed
        bool withHard = false;      // hardStepRender (block 512)
    };

    struct StepReading
    {
        bool valid = false;
        double dHeld = 0.0, dEng = 0.0, dHard = 0.0;
        double kHeld = 0.0, kEng = 0.0;
        bool hasHard = false;
    };

    /** One reading: held A, held B and the step render of one instance, the
        first 0.3 s not scored; the hard control on the same float input. */
    StepReading runStepJob (Processor& p, const Handles& h, const StepRow& r, const juce::AudioBuffer<float>& input,
                            const StepJob& job)
    {
        const double rate = kStepRate;
        const auto from = (size_t) std::lround (0.3 * rate);

        const auto heldA = renderStepMotion (p, h, input, rate, r, 1, job.blockSize, job.viaEntry);
        const auto heldB = renderStepMotion (p, h, input, rate, r, 2, job.blockSize, job.viaEntry);
        const auto engine = renderStepMotion (p, h, input, rate, r, 0, job.blockSize, job.viaEntry);

        const StereoReading a = readStereo (heldA, rate, from);
        const StereoReading b = readStereo (heldB, rate, from);
        const StereoReading e = readStereo (engine, rate, from);

        StepReading q;
        q.dHeld = juce::jmax (a.d2, b.d2);
        q.kHeld = juce::jmax (a.kappa, b.kappa);
        q.dEng = e.d2;
        q.kEng = e.kappa;

        bool hardOk = true;

        if (job.withHard)
        {
            const auto script = stepScriptOf (r, rate, input.getNumSamples(), job.blockSize);
            const auto hard = hardStepRender (widened (input, 0), widened (input, 1), true, rate, script);

            hardOk = hard[0].size() == (size_t) input.getNumSamples() && hard[1].size() == hard[0].size();

            if (hardOk)
                q.dHard = juce::jmax (maxD2Of (hard[0], from), maxD2Of (hard[1], from));

            hardOk = hardOk && std::isfinite (q.dHard);
            q.hasHard = true;
        }

        q.valid = allFinite (heldA) && allFinite (heldB) && allFinite (engine) && hardOk
               && q.dHeld > 0.0 && q.kHeld > 0.0 && std::isfinite (q.dEng) && std::isfinite (q.kEng);
        return q;
    }

    void stepRowsXr()
    {
        section ("XR-g3. G3 step rows through the plugin (Phase 2.4: DSP-04, FUNC-01, FUNC-04; PLAN-2.4 P11, P13)");

        const double tones[2] = { 50.0, 1000.0 };
        const int length = stepLength (kStepRate);
        const juce::AudioBuffer<float> inputs[2] = { stepStimulus (tones[0], kStepRate, length),
                                                     stepStimulus (tones[1], kStepRate, length) };
        const std::vector<StepRow> rows[2] = { stepRowsFor (tones[0]), stepRowsFor (tones[1]) };

        std::vector<StepJob> jobs;

        for (int t = 0; t < 2; ++t)
            for (size_t i = 0; i < rows[t].size(); ++i)
                for (const int blockSize : { 32, 512 })
                {
                    StepJob job;
                    job.tone = t;
                    job.rowIndex = (int) i;
                    job.blockSize = blockSize;
                    job.withHard = blockSize == 512;
                    jobs.push_back (job);
                }

        const size_t numMain = jobs.size();

        for (int t = 0; t < 2; ++t)
            for (size_t i = 0; i < rows[t].size(); ++i)
                if (rows[t][i].group == kGroupBypass)
                    for (const int blockSize : { 32, 512 })
                    {
                        StepJob job;
                        job.tone = t;
                        job.rowIndex = (int) i;
                        job.blockSize = blockSize;
                        job.viaEntry = true;
                        jobs.push_back (job);
                    }

        // One instance per worker, constructed here; the renders on the workers.
        const int workers = workerCount ((int) jobs.size());
        std::vector<std::unique_ptr<Processor>> instances;
        std::vector<Handles> handles;

        for (int w = 0; w < workers; ++w)
        {
            instances.push_back (makeBands (2, 0, mixedSpec));
            handles.push_back (handlesOf (*instances.back()));
        }

        std::vector<StepReading> readings (jobs.size());

        parallelForWorkers ((int) jobs.size(), workers, [&jobs, &instances, &handles, &readings, &rows, &inputs] (int j, int w)
        {
            const StepJob& job = jobs[(size_t) j];
            readings[(size_t) j] = runStepJob (*instances[(size_t) w], handles[(size_t) w],
                                               rows[job.tone][(size_t) job.rowIndex], inputs[job.tone], job);
        });

        struct Agg
        {
            int n = 0;
            double worstD2 = 0.0;
            juce::String worstD2At;
            double worstK = 0.0;
            int kOver2 = 0;
            double minHard = std::numeric_limits<double>::infinity();
            int voids = 0;
        };

        Agg agg[2][kNumStepGroups];
        Tally g3, g3k, g3h;
        int voidCount = 0;
        juce::String voidFigures;
        double worstK50 = 0.0, worstD2All = 0.0, worstK1k = 0.0, weakestHard = std::numeric_limits<double>::infinity();
        int k50Over2 = 0;

        for (size_t j = 0; j < numMain; ++j)
        {
            const StepJob& job = jobs[j];
            const StepRow& r = rows[job.tone][(size_t) job.rowIndex];
            const StepReading& q = readings[j];
            const juce::String label = fmtG (tones[job.tone], 4) + " Hz, block " + juce::String (job.blockSize) + ", " + juce::String (r.name);

            const double dRatio = ratioOf (q.dEng, q.dHeld);
            const double kRatio = ratioOf (q.kEng, q.kHeld);

            g3.check (q.valid && dRatio <= 2.0, label + ": " + fmtG (dRatio, 4) + "x" + (q.valid ? "" : " INVALID"));

            if (! (dRatio <= worstD2All))
                worstD2All = dRatio;

            if (job.tone == 1)
            {
                g3k.check (q.valid && kRatio <= 2.5, label + ": " + fmtG (kRatio, 4) + "x" + (q.valid ? "" : " INVALID"));

                if (! (kRatio <= worstK1k))
                    worstK1k = kRatio;
            }
            else
            {
                if (! (kRatio <= worstK50))
                    worstK50 = kRatio;

                if (! (kRatio <= 2.0))
                    ++k50Over2;
            }

            Agg& a = agg[job.tone][r.group];
            ++a.n;

            if (! (dRatio <= a.worstD2))
            {
                a.worstD2 = dRatio;
                a.worstD2At = juce::String (r.name) + " b" + juce::String (job.blockSize);
            }

            if (! (kRatio <= a.worstK))
                a.worstK = kRatio;

            if (! (kRatio <= 2.0))
                ++a.kOver2;

            if (q.hasHard)
            {
                const double hRatio = ratioOf (q.dHard, q.dHeld);
                const bool isVoid = r.voidAt1k && job.tone == 1;

                if (isVoid)
                {
                    ++voidCount;
                    ++a.voids;
                    voidFigures += (voidFigures.isEmpty() ? "" : "; ") + juce::String (r.name) + " reads " + fmtG (hRatio, 3) + "x";
                    std::printf ("      XR-g3h %-80s hard / held %.2f  VOID, not counted (the 10 Hz, Q 40 bell does nothing to a 1 kHz tone)\n",
                                 label.toRawUTF8(), hRatio);
                    continue;
                }

                g3h.check (q.valid && hRatio >= 10.0, label + ": " + fmtG (hRatio, 4) + "x");
                a.minHard = juce::jmin (a.minHard, hRatio);
                weakestHard = juce::jmin (weakestHard, hRatio);
            }
        }

        // The bench's table, per group, float beside double (RESEARCH-2.4 Q1).
        for (int t = 0; t < 2; ++t)
            for (int g = 0; g < kNumStepGroups; ++g)
            {
                const Agg& a = agg[t][g];
                std::printf ("      XR-g3 %6.0f Hz  %-17s n %3d | d2 eng / held worst %6.3f (%s) | kappa eng / held worst %6.2f, above 2.0: %3d | hard / held lowest at block 512 %9.1f | void %d\n",
                             tones[t], kStepGroupNames[g], a.n, a.worstD2, a.worstD2At.toRawUTF8(), a.worstK, a.kOver2,
                             a.minHard, a.voids);
            }

        row (g3.bad == 0 && g3.total == 1028,
             "XR-g3 G3 step rows through processBlock: 242 rows + 15 band-bypass rows x 50 Hz / 1 kHz x blocks 32 / 512: largest second difference <= 2.0x the louder held render",
             g3.detail() + "  worst " + fmtG (worstD2All, 4) + "x (bench 1.53x at 50 Hz, 1.02x at 1 kHz)");

        row (g3k.bad == 0 && g3k.total == 514,
             "XR-g3k the same renders, 1 kHz: local kappa <= 2.5x the larger held reference",
             g3k.detail() + "  worst " + fmtG (worstK1k, 4) + "x (bench 2.20x)");

        info ("XR-g3k 50 Hz (printed, not gated, PLAN-2.4 P13 (b)): local kappa engine / held worst " + fmtG (worstK50, 4)
                  + "x; " + juce::String (k50Over2) + " of 514 readings above 2.0x (bench, 484 readings: worst 20.5x, 202 above 2.0x)");

        row (g3h.bad == 0 && g3h.total == 511 && voidCount == 3,
             "XR-g3h hardStepRender (the 2.3 rule, the plugin's float input) at block 512 reads >= 10x the louder held render (control fires); void readings printed, not counted",
             g3h.detail() + "  weakest " + fmtG (weakestHard, 4) + "x (bench 274x / 17.9x); void: " + voidFigures);

        // XR-g3b
        Tally g3b, g3bk;
        double worstB = 0.0, worstBk = 0.0, worstBk50 = 0.0;

        for (size_t j = numMain; j < jobs.size(); ++j)
        {
            const StepJob& job = jobs[j];
            const StepRow& r = rows[job.tone][(size_t) job.rowIndex];
            const StepReading& q = readings[j];
            const juce::String label = fmtG (tones[job.tone], 4) + " Hz, block " + juce::String (job.blockSize) + ", " + juce::String (r.name);

            const double dRatio = ratioOf (q.dEng, q.dHeld);
            const double kRatio = ratioOf (q.kEng, q.kHeld);

            g3b.check (q.valid && dRatio <= 2.0, label + ": " + fmtG (dRatio, 4) + "x" + (q.valid ? "" : " INVALID"));
            worstB = juce::jmax (worstB, dRatio);

            if (job.tone == 1)
            {
                g3bk.check (q.valid && kRatio <= 2.5, label + ": kappa " + fmtG (kRatio, 4) + "x");
                worstBk = juce::jmax (worstBk, kRatio);
            }
            else
            {
                worstBk50 = juce::jmax (worstBk50, kRatio);
            }
        }

        row (g3b.bad == 0 && g3b.total == 64 && g3bk.bad == 0 && g3bk.total == 32,
             "XR-g3b the Global bypass group through processBlockBypassed: (a) second difference <= 2.0x held, both tones; (b) 1 kHz local kappa <= 2.5x held",
             "(a) " + g3b.detail() + " worst " + fmtG (worstB, 4) + "x; (b) " + g3bk.detail() + " worst " + fmtG (worstBk, 4)
                 + "x; 50 Hz kappa worst " + fmtG (worstBk50, 4) + "x (printed)");
    }

    //==========================================================================
    // XR-ref, XR-rest. The engine does the specified fade (PLAN-2.4 P12)
    //==========================================================================
    /** XF-ref's 30 rows: 6 type pairs, 12 slope pairs (6 and 18 dB/oct among
        them), 6 placement pairs, 3 enables, 3 disables; the deep settings at
        1 kHz. */
    std::vector<StepRow> refRowsFor()
    {
        using T = eq::Type;
        const double tone = 1000.0;
        std::vector<StepRow> rows;

        const auto add = [&rows] (const std::string& name, const StepCfg& a, const StepCfg& b)
        {
            StepRow r;
            r.name = name;
            r.a = a;
            r.b = b;
            rows.push_back (r);
        };

        const std::pair<T, T> typePairs[6] = { { T::bell, T::lowShelf }, { T::highShelf, T::lowCut }, { T::highCut, T::notch },
                                               { T::bandPass, T::tilt }, { T::bell, T::highCut }, { T::lowCut, T::tilt } };

        for (const auto& tp : typePairs)
            add ("type " + typeNameOf (tp.first) + " -> " + typeNameOf (tp.second), deepCfg (tp.first, tone), deepCfg (tp.second, tone));

        struct SlopePair { T type; int from, to; };

        const SlopePair slopePairs[12] = { { T::lowCut, 0, 2 }, { T::lowCut, 2, 7 }, { T::lowCut, 5, 0 },
                                           { T::highCut, 0, 3 }, { T::highCut, 2, 5 }, { T::highCut, 7, 2 },
                                           { T::lowShelf, 0, 1 }, { T::lowShelf, 2, 6 }, { T::highShelf, 1, 2 },
                                           { T::highShelf, 0, 7 }, { T::tilt, 2, 4 }, { T::tilt, 0, 3 } };

        for (const auto& sp : slopePairs)
        {
            StepCfg a = deepCfg (sp.type, tone), b = a;
            a.s.slopeIndex = sp.from;
            b.s.slopeIndex = sp.to;
            add ("slope " + typeNameOf (sp.type) + " " + std::to_string (6 * eq::slopeOrder[sp.from]) + " -> "
                     + std::to_string (6 * eq::slopeOrder[sp.to]), a, b);
        }

        struct PlacePair { bool cut; int from, to; };

        const PlacePair placePairs[6] = { { false, 0, 1 }, { false, 1, 2 }, { false, 2, 3 }, { false, 3, 4 }, { false, 4, 0 }, { true, 0, 3 } };

        for (const auto& pp : placePairs)
        {
            StepCfg a = pp.cut ? deepCfg (T::lowCut, tone) : deepCfg (T::bell, tone);

            if (pp.cut)
                a.s.slopeIndex = 7;

            StepCfg b = a;
            a.p = (eq::Placement) pp.from;
            b.p = (eq::Placement) pp.to;
            add (std::string ("place ") + (pp.cut ? "LowCut 96 " : "Bell +18 ") + placementNameOf (a.p) + " -> " + placementNameOf (b.p), a, b);
        }

        {
            StepCfg shelf96 = deepCfg (T::highShelf, tone);
            shelf96.s.slopeIndex = 7;
            const std::pair<std::string, StepCfg> enables[3] = { { "Bell +18", deepCfg (T::bell, tone) }, { "HighShelf 96 +18", shelf96 },
                                                                 { "Tilt 12 +36", deepCfg (T::tilt, tone) } };

            for (const auto& e : enables)
            {
                StepCfg off = e.second;
                off.active = false;
                add ("enable " + e.first, off, e.second);
            }
        }

        {
            StepCfg cut96 = deepCfg (T::lowCut, tone);
            cut96.s.slopeIndex = 7;
            const std::pair<std::string, StepCfg> disables[3] = { { "LowCut 96 at tone", cut96 }, { "Notch near tone", deepCfg (T::notch, tone) },
                                                                  { "LowShelf 12 -18", deepCfg (T::lowShelf, tone, -18.0) } };

            for (const auto& e : disables)
            {
                StepCfg off = e.second;
                off.active = false;
                add ("disable " + e.first, e.second, off);
            }
        }

        return rows;
    }

    struct RefReading
    {
        bool valid = false;
        double errorDb = 0.0;       // largest |plugin - reference| re the output peak
        int changeAt = 0;
    };

    /** Stereo noise, blocks of 64; the change at sample 4096 (48 kHz) or 8192
        (96 kHz): a block start and a tick. Scored from the render start to
        50 ms after the fade. */
    RefReading runRefJob (Processor& p, const Handles& h, const StepRow& r, double rate)
    {
        RefReading q;
        q.changeAt = rate > 48000.0 ? 8192 : 4096;

        const int changeAt = q.changeAt;
        const int fadeN = refFadeLength (rate);
        const int length = changeAt + fadeN + (int) std::lround (0.050 * rate);
        const auto input = makeNoiseAt (2, length, 0x7E5F0002, kNoise12);

        storeStepCfg (h, r, r.a, false);

        const auto output = renderWith (p, input, rate, 64,
                                        [] (int, int) { return 64; },
                                        [&h, &r, changeAt] (BlockInfo& current)
                                        {
                                            storeStepCfg (h, r, current.position < changeAt ? r.a : r.b, false);
                                        });

        const auto reference = fadeRefRender (widened (input, 0), widened (input, 1), rate,
                                              stepInputOf (r, r.a, 0), stepInputOf (r, r.b, 0), (size_t) changeAt, fadeN);

        if (reference[0].size() != (size_t) length || reference[1].size() != (size_t) length || ! allFinite (output))
            return q;

        double error = 0.0, peak = 0.0;

        for (int channel = 0; channel < 2; ++channel)
        {
            const float* y = output.getReadPointer (channel);
            const auto& ref = reference[(size_t) channel];

            for (int i = 0; i < length; ++i)
            {
                const double v = (double) y[i];
                const double d = std::abs (v - ref[(size_t) i]);

                if (! (d <= error))
                    error = d;

                peak = juce::jmax (peak, std::abs (v));
            }
        }

        q.valid = peak > 0.0 && std::isfinite (error);
        q.errorDb = 20.0 * std::log10 (juce::jmax (error, 1.0e-300) / juce::jmax (peak, 1.0e-300));
        return q;
    }

    void fadeReferenceRowsXr()
    {
        section ("XR-ref / XR-rest. the specified fade through the plugin (Phase 2.4: PLAN-2.4 P12; VERIFICATION-2.3 issue 4)");

        // XR-ref
        {
            const auto rows = refRowsFor();
            const double rates[2] = { 48000.0, 96000.0 };

            Tally guard;

            for (const double fs : { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
                guard.check (refFadeLength (fs) == eq::fadeLengthSamples (fs),
                             "harness N differs from the engine's at " + fmtG (fs, 6) + " Hz");

            struct Job { size_t rowIndex; double rate; };
            std::vector<Job> jobs;

            for (const double rate : rates)
                for (size_t i = 0; i < rows.size(); ++i)
                    jobs.push_back ({ i, rate });

            const int workers = workerCount ((int) jobs.size());
            std::vector<std::unique_ptr<Processor>> instances;
            std::vector<Handles> handles;

            for (int w = 0; w < workers; ++w)
            {
                instances.push_back (makeBands (2, 0, mixedSpec));
                handles.push_back (handlesOf (*instances.back()));
            }

            std::vector<RefReading> readings (jobs.size());

            parallelForWorkers ((int) jobs.size(), workers, [&jobs, &instances, &handles, &readings, &rows] (int j, int w)
            {
                const Job& job = jobs[(size_t) j];
                readings[(size_t) j] = runRefJob (*instances[(size_t) w], handles[(size_t) w], rows[job.rowIndex], job.rate);
            });

            Tally tally;
            double worst = -std::numeric_limits<double>::infinity();
            juce::String worstAt;

            for (size_t j = 0; j < jobs.size(); ++j)
            {
                const RefReading& q = readings[j];
                const juce::String label = fmtG (jobs[j].rate / 1000.0, 4) + " kHz " + juce::String (rows[jobs[j].rowIndex].name);

                tally.check (q.valid && q.errorDb <= -120.0, label + ": " + fmtG (q.errorDb, 4) + " dB" + (q.valid ? "" : " INVALID"));

                if (! (q.errorDb <= worst))
                {
                    worst = q.errorDb;
                    worstAt = label;
                }
            }

            row (tally.bad == 0 && tally.total == 60 && guard.bad == 0 && guard.total == 8,
                 "XR-ref 30 first fades (6 type, 12 slope, 6 placement, 3 enable, 3 disable) at 48 and 96 kHz, noise: float output against fadeRefRender <= -120 dB re the output peak, render start to 50 ms after the fade",
                 tally.detail() + "  worst " + fmtG (worst, 4) + " dB (" + worstAt + "); harness N guard " + guard.detail());
        }

        // XR-rest (VERIFICATION-2.3 issue 4): RM-keep's stimulus, the type
        // change of keepRender (Bell -> Low Shelf at block 50, sample 24000).
        {
            const double rate = 48000.0;
            const int total = (int) std::lround (rate);
            const int editAt = kKeepEditBlock * kKeepBlock;
            const int fadeN = eq::fadeLengthSamples (rate);

            juce::AudioBuffer<float> burst (1, total);
            burst.clear();

            for (int i = 0; i < 480; ++i)
                burst.setSample (0, i, (float) (0.5 * std::sin (2.0 * eq::pi * 40.0 * (double) i / rate)));

            const auto unedited = keepRender (burst, rate, 0);
            const auto typeEdit = keepRender (burst, rate, 2);

            auto fresh = makeBands (1, 1, [] (int) { return makeSpec (kLowShelf, 40.0f, 24.0f, 20.0f, 1, kStereo); });
            const auto freshOut = renderWith (*fresh, sliceOf (burst, editAt, total - editAt), rate, kKeepBlock,
                                              [] (int, int) { return kKeepBlock; },
                                              [] (BlockInfo&) {});

            const bool after = sameFrom (typeEdit, editAt + fadeN, freshOut, fadeN, total - editAt - fadeN);
            const bool before = sameFrom (typeEdit, 0, unedited, 0, editAt);
            const bool differsFromOld = ! sameFrom (typeEdit, editAt, unedited, editAt, fadeN);
            const bool differsFromNew = ! sameFrom (typeEdit, editAt, freshOut, 0, fadeN);
            const bool ringing = ! sameFrom (unedited, editAt, freshOut, 0, total - editAt);

            row (after && before && differsFromOld && differsFromNew && ringing && allFinite (typeEdit),
                 "XR-rest bell 40 Hz Q 20 +24 dB ringing, type -> Low Shelf at sample 24000: from the fade's end memcmp-equal to a fresh instance on the new parameters started at the fade's first sample",
                 juce::String (after ? "" : "differs from the fresh instance after the fade (first at sample "
                                                 + juce::String (firstDifference (sliceOf (typeEdit, editAt, total - editAt), freshOut)) + " after the edit); ")
                     + (before ? "" : "differs before the edit; ")
                     + (differsFromOld ? "" : "control: the fade equals the outgoing render; ")
                     + (differsFromNew ? "" : "control: the fade equals the fresh instance; ")
                     + (ringing ? "" : "control: the ring does not matter; ")
                     + "N = " + juce::String (fadeN));
        }
    }

    //==========================================================================
    // XR-g4d, XR-g4e, XR-g4f, XR-g4n. G4 (d), (e), (f) on the float output
    //==========================================================================
    constexpr int kG4Length = 36000;            // 0.75 s at 48 kHz
    constexpr int kG4EventAt = 14400;           // 0.3 s: the event is seen by the first block starting there or later

    // 24 bells as fade_bench g4: 40 Hz x 1.3^b, Q 0.5 + 0.2 b.
    BandSpec g4BellSpec (int index, float gainDb)
    {
        return makeSpec (kBell, 40.0f * std::pow (1.3f, (float) index), gainDb, 0.5f + 0.2f * (float) index, 1, kStereo);
    }

    // 24 gain-bearing bands at +-12 dB: bells, shelves, tilt, every slope.
    BandSpec g4GainSpec (int index)
    {
        static const int types[4] = { kBell, kLowShelf, kHighShelf, kTilt };
        return makeSpec (types[index % 4], 40.0f * std::pow (1.3f, (float) index), (index % 2 == 1) ? 12.0f : -12.0f,
                         0.5f + 0.2f * (float) index, index % 8, kStereo);
    }

    struct EventRender
    {
        juce::AudioBuffer<float> output;
        int firedAt = -1;           // the first sample of the block that saw the event
    };

    /** A render of one pattern (a block size, or 0: the seeded random
        sequence) with one event, made by `edit` before the first block that
        starts at or after eventAt. viaBypassedAfter: that block and every
        later one arrive through processBlockBypassed. Main thread. */
    template <typename EditFn>
    EventRender renderEvent (Processor& p, const juce::AudioBuffer<float>& input, int pattern, int eventAt,
                             bool viaBypassedAfter, EditFn&& edit)
    {
        EventRender result;
        int fired = -1;

        const BlockHook hook = [&fired, &edit, eventAt, viaBypassedAfter] (BlockInfo& current)
        {
            if (fired < 0 && current.position >= eventAt)
            {
                fired = current.position;
                edit();
            }

            if (viaBypassedAfter && fired >= 0)
                current.viaBypassed = true;
        };

        result.output = pattern > 0 ? render (p, input, pattern, false, kRenderRate, hook)
                                    : render (p, input, kSequenceMax, false, kRenderRate, hook, randomBlockSequence (0x5E0BE9, kSequenceMax));
        result.firedAt = fired;
        return result;
    }

    /** Output memcmp-equal to the input from `from` to the end (false for a
        `from` outside the render). */
    bool equalFrom (const juce::AudioBuffer<float>& output, const juce::AudioBuffer<float>& input, int from)
    {
        return from >= 0 && from < input.getNumSamples() && sameFrom (output, from, input, from, input.getNumSamples() - from);
    }

    /** The last sample index at which any channel differs by bit pattern; -1 when none does. */
    int lastDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return a.getNumSamples();

        for (int i = a.getNumSamples(); --i >= 0;)
            for (int channel = 0; channel < a.getNumChannels(); ++channel)
                if (! sameBits (a.getReadPointer (channel)[i], b.getReadPointer (channel)[i]))
                    return i;

        return -1;
    }

    void transparencyRowsXr()
    {
        section ("XR-g4. G4 (d), (e), (f) on the float output (Phase 2.4: QUAL-01, FUNC-04, FUNC-05)");

        const int patterns[7] = { 1, 16, 64, 512, 1024, 4096, 0 };
        const int after200 = (int) std::lround (0.200 * kRenderRate);
        const int after100 = (int) std::lround (0.100 * kRenderRate);
        const int after5 = (int) std::lround (0.005 * kRenderRate);
        const int fadeN = eq::fadeLengthSamples (kRenderRate);

        // XR-g4d
        {
            Tally tally;
            juce::String figures;

            for (const int channels : { 1, 2 })
            {
                const auto input = makeNoiseAt (channels, kG4Length, 0x64D00000 + channels, kNoise12);

                for (const int pattern : patterns)
                {
                    const juce::String where = (channels == 1 ? juce::String ("mono ") : juce::String ("stereo ")) + patternName (pattern);

                    // (d0) enabled at exactly 0 dB from the first block.
                    {
                        auto p = makeBands (channels, 24, [] (int index) { return g4BellSpec (index, 0.0f); });
                        const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [] {});
                        tally.check (sameAudio (r.output, input) && activeBands (*p) == 24, where + ": 24 bells on at 0 dB from the start differ from the input");
                    }

                    // (d0) disabled, then enabled at exactly 0 dB at the event.
                    {
                        auto p = makeBands (channels, 24, [] (int index) { return g4BellSpec (index, 0.0f); });
                        const Handles h = handlesOf (*p);

                        for (const auto& hb : h.band)
                            hb.enabled->store (0.0f);

                        const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [&h]
                        {
                            for (const auto& hb : h.band)
                                hb.enabled->store (1.0f);
                        });

                        tally.check (sameAudio (r.output, input) && r.firedAt >= 0 && activeBands (*p) == 24,
                                     where + ": 24 bells enabled at 0 dB mid-render differ from the input");
                    }

                    // (d1) 24 bells 6 dB -> 0.
                    {
                        auto p = makeBands (channels, 24, [] (int index) { return g4BellSpec (index, 6.0f); });
                        const Handles h = handlesOf (*p);

                        const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [&h]
                        {
                            for (const auto& hb : h.band)
                                hb.gain->store (0.0f);
                        });

                        const bool acted = r.firedAt > 0 && ! sameFrom (r.output, 0, input, 0, r.firedAt);
                        const bool equal = r.firedAt >= 0 && equalFrom (r.output, input, r.firedAt + after200);
                        const int last = lastDifference (r.output, input);

                        tally.check (acted && equal, where + ": 24 bells 6 -> 0 dB"
                                                         + (acted ? "" : " (the bells did not act before the edit)")
                                                         + (equal ? "" : " differ 200 ms after the edit"));

                        if (channels == 2)
                            figures += (figures.isEmpty() ? "" : ", ") + patternName (pattern) + " "
                                     + fmtG (1000.0 * (double) (last + 1 - r.firedAt) / kRenderRate, 4) + " ms";
                    }
                }
            }

            row (tally.bad == 0 && tally.total == 42,
                 "XR-g4d G4 (d): 24 bells on at exactly 0 dB (from the start, and enabled mid-render) memcmp-equal to the input throughout; 24 bells 6 -> 0 dB memcmp-equal 200 ms after the edit (blocks 1 / 16 / 64 / 512 / 1024 / 4096 / random, mono and stereo)",
                 tally.detail() + "  stereo 6 -> 0 dB: equal from " + figures + " after the edit (bench 85.7 - 88.3)");
        }

        // XR-g4e
        {
            Tally tally;
            juce::String figures;
            const auto input = makeNoiseAt (2, kG4Length, 0x64E00002, kNoise12);

            for (const int pattern : patterns)
            {
                auto p = makeBands (2, 24, g4GainSpec);
                const Handles h = handlesOf (*p);

                const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [&h] { h.gainScale->store (0.0f); });

                const bool acted = r.firedAt > 0 && ! sameFrom (r.output, 0, input, 0, r.firedAt);
                const bool equal = r.firedAt >= 0 && equalFrom (r.output, input, r.firedAt + after200);
                const int last = lastDifference (r.output, input);

                tally.check (acted && equal, patternName (pattern) + (acted ? "" : " (the bands did not act before the edit)")
                                                 + (equal ? "" : ": differs 200 ms after gain_scale -> 0 %"));

                figures += (figures.isEmpty() ? "" : ", ") + patternName (pattern) + " "
                         + fmtG (1000.0 * (double) (last + 1 - r.firedAt) / kRenderRate, 4) + " ms";
            }

            row (tally.bad == 0 && tally.total == 7,
                 "XR-g4e G4 (e): 24 gain-bearing bands at +-12 dB (bells, shelves, tilt), gain_scale -> 0 %: memcmp-equal to the input 200 ms later (7 block patterns, stereo)",
                 tally.detail() + "  equal from " + figures + " after the edit (bench 87.0 - 89.1)");
        }

        // XR-g4f
        {
            Tally tally;
            const auto input = makeNoiseAt (2, kG4Length, 0x64F00002, kNoise12);

            for (const bool viaEntry : { false, true })
                for (const int pattern : patterns)
                {
                    auto p = makeBands (2, 24, g4GainSpec);
                    const Handles h = handlesOf (*p);

                    const auto r = renderEvent (*p, input, pattern, kG4EventAt, viaEntry, [&h, viaEntry]
                    {
                        if (! viaEntry)
                            h.bypass->store (1.0f);
                    });

                    const bool equal = r.firedAt >= 0 && equalFrom (r.output, input, r.firedAt + after100);
                    const int last = lastDifference (r.output, input);
                    const bool exact = r.firedAt >= 0 && last + 1 == r.firedAt + fadeN;

                    tally.check (equal && exact, juce::String (viaEntry ? "processBlockBypassed, " : "parameter, ") + patternName (pattern)
                                                     + (equal ? "" : ": differs 100 ms after the toggle")
                                                     + (exact ? "" : ": equal from " + juce::String (last + 1 - r.firedAt)
                                                                         + " samples after the toggle, not N = " + juce::String (fadeN)));
                }

            row (tally.bad == 0 && tally.total == 14,
                 "XR-g4f G4 (f): 24 bands at +-12 dB, bypass on by the parameter and by processBlockBypassed: memcmp-equal 100 ms later, and equal from exactly N samples after the block that sees it (7 block patterns)",
                 tally.detail() + "  N = " + juce::String (fadeN));
        }

        // XR-g4n
        {
            Tally tally;
            const auto input = makeNoiseAt (2, kG4Length, 0x64A00002, kNoise12);

            for (const int pattern : patterns)
            {
                // (d) with one band at 0.001 dB instead of 0
                {
                    auto p = makeBands (2, 24, [] (int index) { return g4BellSpec (index, 6.0f); });
                    const Handles h = handlesOf (*p);

                    const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [&h]
                    {
                        for (size_t b = 0; b < h.band.size(); ++b)
                            h.band[b].gain->store (b == 0 ? 0.001f : 0.0f);
                    });

                    tally.check (r.firedAt >= 0 && ! equalFrom (r.output, input, r.firedAt + after200),
                                 "(d) one bell at 0.001 dB, " + patternName (pattern) + ": still memcmp-equal 200 ms later");
                }

                // (e) gain_scale to 0.001 / 12 of 100 %: the 12 dB bands at 0.001 dB
                {
                    auto p = makeBands (2, 24, g4GainSpec);
                    const Handles h = handlesOf (*p);
                    const auto scale = (float) (100.0 * 0.001 / 12.0);

                    const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [&h, scale] { h.gainScale->store (scale); });

                    tally.check (r.firedAt >= 0 && ! equalFrom (r.output, input, r.firedAt + after200),
                                 "(e) gain_scale -> " + fmtG ((double) scale, 3) + " %, " + patternName (pattern) + ": still memcmp-equal 200 ms later");
                }

                // (f) read 5 ms after the toggle, both routes
                for (const bool viaEntry : { false, true })
                {
                    auto p = makeBands (2, 24, g4GainSpec);
                    const Handles h = handlesOf (*p);

                    const auto r = renderEvent (*p, input, pattern, kG4EventAt, viaEntry, [&h, viaEntry]
                    {
                        if (! viaEntry)
                            h.bypass->store (1.0f);
                    });

                    tally.check (r.firedAt >= 0 && ! equalFrom (r.output, input, r.firedAt + after5),
                                 juce::String ("(f) ") + (viaEntry ? "processBlockBypassed, " : "parameter, ") + patternName (pattern)
                                     + ": already memcmp-equal 5 ms after the toggle");
                }
            }

            row (tally.bad == 0 && tally.total == 28,
                 "XR-g4n (d) with one bell at 0.001 dB, (e) with gain_scale leaving the 12 dB bands at 0.001 dB, (f) read 5 ms after the toggle: each differs from the input (control fires)",
                 tally.detail());
        }
    }

    //==========================================================================
    // XR-g2. G2 live: slope changes on a running band (DSP-03)
    //==========================================================================
    /** The magnitude in dB of an impulse response (amplitude 1) at hz. */
    double irMagDb (const std::vector<float>& h, double rate, double hz)
    {
        return dftDb (h, 1.0, rate, { hz })[0];
    }

    /** G2s's crossing, on a measured response: the frequency in [fA, fB]
        where the magnitude passes targetDb, by bisection in log f. */
    bool irCrossing (const std::vector<float>& h, double rate, double fA, double fB, double targetDb, double& fOut)
    {
        double la = std::log (fA), lb = std::log (fB);
        double ea = irMagDb (h, rate, fA) - targetDb;
        const double eb = irMagDb (h, rate, fB) - targetDb;

        if (! std::isfinite (ea) || ! std::isfinite (eb) || (ea > 0.0) == (eb > 0.0))
            return false;

        for (int i = 0; i < 50; ++i)
        {
            const double lm = 0.5 * (la + lb);
            const double em = irMagDb (h, rate, std::exp (lm)) - targetDb;

            if ((em > 0.0) == (ea > 0.0))
            {
                la = lm;
                ea = em;
            }
            else
            {
                lb = lm;
            }
        }

        fOut = std::exp (0.5 * (la + lb));
        return true;
    }

    struct SlopeReading
    {
        bool ok = false;
        bool shortSpan = false;     // measured against the prototype (G2s-short)
        double cornerDb = 0.0;
        double slopeError = 0.0;    // dB/oct
        juce::String note;
    };

    /** One band at 500 Hz, Q 1, at slope `fromSlope`, mono, 48 kHz, blocks of
        512, noise; the slope set to `toSlope` at block 30; the noise stops at
        the fade's end; an impulse 100 ms later, its response read over 1 s. */
    SlopeReading runSlopeChange (int type, int fromSlope, int toSlope)
    {
        constexpr int blockSize = 512, changeBlock = 30;
        const double rate = kRenderRate;
        const double fc = 500.0;
        const int changeAt = changeBlock * blockSize;
        const int fadeEnd = changeAt + eq::fadeLengthSamples (rate);
        const int impulseAt = fadeEnd + (int) std::lround (0.100 * rate);
        const int irLength = (int) std::lround (rate);
        const int total = impulseAt + irLength;

        auto input = makeNoiseAt (1, total, 0x62200000 + type * 16 + toSlope, kNoise12);
        input.clear (0, fadeEnd, total - fadeEnd);
        input.setSample (0, impulseAt, 1.0f);

        auto p = makeBands (1, 1, [type, fromSlope, fc] (int) { return makeSpec (type, (float) fc, 0.0f, 1.0f, fromSlope, kStereo); });
        const Handles h = handlesOf (*p);

        const auto output = renderWith (*p, input, rate, blockSize,
                                        [] (int, int) { return blockSize; },
                                        [&h, toSlope] (BlockInfo& current)
                                        {
                                            if (current.index == changeBlock)
                                                h.band[0].slope->store ((float) toSlope);
                                        });

        SlopeReading r;

        if (! allFinite (output))
        {
            r.note = "not finite";
            return r;
        }

        const float* y = output.getReadPointer (0);
        const std::vector<float> ir (y + impulseAt, y + impulseAt + irLength);

        // G2s: -3.0103 dB at the corner (Q 1), the slope between the -20 and
        // -50 dB points against 6.02 N (harness-local table, not eq::slopeOrder).
        static const double nominalDbPerOct[8] = { 6.0, 12.0, 18.0, 24.0, 36.0, 48.0, 72.0, 96.0 };
        const double cornerTarget = 10.0 * std::log10 (0.5);
        const double nominal = 20.0 * std::log10 (2.0) * (nominalDbPerOct[toSlope] / 6.0);

        r.cornerDb = irMagDb (ir, rate, fc);
        const bool cornerOk = std::abs (r.cornerDb - cornerTarget) <= 0.10;

        double f20 = 0.0, f50 = 0.0;
        const bool lowCut = type == kLowCut;
        const bool measurable = lowCut ? (irCrossing (ir, rate, fc * 1.0e-3, fc, -20.0, f20) && irCrossing (ir, rate, fc * 1.0e-3, fc, -50.0, f50))
                                       : (irCrossing (ir, rate, fc, 20000.0, -20.0, f20) && irCrossing (ir, rate, fc, 20000.0, -50.0, f50));

        bool slopeOk = false;

        if (measurable)
        {
            const double slope = 30.0 / std::abs (std::log2 (f50 / f20));
            r.slopeError = std::abs (slope - nominal);
            slopeOk = r.slopeError <= 1.0;
        }
        else if (! lowCut)
        {
            // G2s-short: from the -20 dB point to 20 kHz, against the prototype.
            r.shortSpan = true;
            const double fEnd = 20000.0;
            double fStart = 0.0;

            if (irCrossing (ir, rate, fc, fEnd, -20.0, fStart) && std::log2 (fEnd / fStart) >= 1.0)
            {
                eq::RawBand raw = rawOfHandles (h.band[0]);
                const eq::BandInput in = eq::makeBandInput (raw, h.gainScale->load(), 0.0);
                const auto designer = designerFor (rate);
                eq::BandDesign design {};
                designer->design (in.settings, design);

                const double octaves = std::log2 (fEnd / fStart);
                const double wA = 2.0 * eq::pi * fStart / rate;
                const double wB = 2.0 * eq::pi * fEnd / rate;
                const double measuredSlope = (irMagDb (ir, rate, fStart) - irMagDb (ir, rate, fEnd)) / octaves;
                const double analogSlope = (eq::analogBandMagDb (design, wA) - eq::analogBandMagDb (design, wB)) / octaves;

                r.slopeError = std::abs (measuredSlope - analogSlope);
                slopeOk = r.slopeError <= 1.0;
            }
            else
            {
                r.note = "unmeasurable (span under one octave)";
            }
        }
        else
        {
            r.note = "a low cut without its -20 / -50 dB points";
        }

        r.ok = cornerOk && slopeOk;
        return r;
    }

    void liveSlopeRowsXr()
    {
        section ("XR-g2. G2 live: slope changes on a running band (Phase 2.4: DSP-03)");

        Tally tally;
        double worstCorner = 0.0, worstSlope = 0.0;
        int shortCount = 0;

        for (const int type : { kLowCut, kHighCut })
            for (int slope = 0; slope < 8; ++slope)
            {
                const int neighbour = slope > 0 ? slope - 1 : 1;
                const SlopeReading r = runSlopeChange (type, neighbour, slope);
                const double cornerError = std::abs (r.cornerDb - 10.0 * std::log10 (0.5));

                tally.check (r.ok, juce::String (kTypeNames[type]) + " " + juce::String (6 * eq::slopeOrder[slope]) + " dB/oct from "
                                       + juce::String (6 * eq::slopeOrder[neighbour]) + ": corner " + fmtG (r.cornerDb, 5) + " dB, slope error "
                                       + fmtG (r.slopeError, 3) + " dB/oct" + (r.shortSpan ? " (G2s-short)" : "")
                                       + (r.note.isEmpty() ? juce::String() : " " + r.note));

                worstCorner = juce::jmax (worstCorner, cornerError);
                worstSlope = juce::jmax (worstSlope, r.slopeError);
                shortCount += r.shortSpan ? 1 : 0;
            }

        row (tally.bad == 0 && tally.total == 16,
             "XR-g2 Low Cut and High Cut at 500 Hz, each of the 8 slopes reached from its neighbour on a running band; impulse 100 ms after the fade: corner -3.01 +- 0.10 dB; slope within 1.0 dB/oct (-20 / -50 dB points, else G2s-short against the prototype)",
             tally.detail() + "  worst corner error " + fmtG (worstCorner, 3) + " dB, worst slope error " + fmtG (worstSlope, 3)
                 + " dB/oct; " + juce::String (shortCount) + " cells measured G2s-short");
    }

    //==========================================================================
    // XR-byp, XR-bypn, XR-big, XR-prep. Changed while bypassed; VERIFICATION-2.3 issue 3
    //==========================================================================
    constexpr int kBypBands = 7;        // six mixed bands, and band 7 disabled until the edit

    /** The edits of one XR-byp variant. 0, 1: type, slope, placement, gain,
        frequency, Q and enable across the bands; 2: frequency, gain and Q
        only (SUMMARY-2.3 deviation 4); 3: a gain move on band 1 (made before
        the bypass, in the middle of whose ramp the bypass engages). */
    void applyBypEdits (const Handles& h, int variant)
    {
        if (variant == 3)
        {
            h.band[0].gain->store (-3.0f);
            return;
        }

        if (variant != 2)
        {
            h.band[0].type->store ((float) kHighShelf);     // Bell -> High Shelf
            h.band[1].slope->store (6.0f);                  // Low Shelf 36 -> 72 dB/oct
            h.band[2].placement->store ((float) kMid);      // High Shelf Right -> Mid
            h.band[6].enabled->store (1.0f);                // Band Pass enabled
        }

        h.band[2].gain->store (4.5f);                       // High Shelf -6 -> +4.5 dB
        h.band[3].freq->store (45.0f);                      // Low Cut 30 -> 45 Hz
        h.band[4].q->store (2.0f);                          // High Cut Q 0.7 -> 2
    }

    std::unique_ptr<Processor> makeBypBands()
    {
        auto p = makeBands (2, kBypBands, mixedSpec);
        setRaw (*p, bandId (kBypBands, "enabled"), 0.0f);
        return p;
    }

    void bypassRowsXr()
    {
        section ("XR-byp / XR-big / XR-prep. changed while bypassed; oversize host blocks; re-prepare (Phase 2.4; VERIFICATION-2.3 issue 3)");

        // XR-byp, XR-bypn
        {
            constexpr int blockSize = 512, rampBlock = 9, bypassBlock = 10, editBlock = 14, resumeBlock = 20;
            const int fadeN = eq::fadeLengthSamples (kRenderRate);
            const auto input = makeNoiseAt (2, 36 * blockSize, 0x6B790002, kNoise12);
            const int total = input.getNumSamples();
            const int bypassAt = bypassBlock * blockSize;
            const int resumeAt = resumeBlock * blockSize;
            const char* const names[4] = { "bypass parameter, structural + continuous edits", "processBlockBypassed, structural + continuous edits",
                                           "bypass parameter, continuous edits only", "bypass parameter, engaged in the middle of a gain ramp" };

            Tally tally, control;

            for (int variant = 0; variant < 4; ++variant)
            {
                const bool viaEntry = variant == 1;
                auto p = makeBypBands();
                const Handles h = handlesOf (*p);

                const auto output = render (*p, input, blockSize, false, kRenderRate, [&h, variant, viaEntry] (BlockInfo& current)
                {
                    if (variant == 3 && current.index == rampBlock)
                        applyBypEdits (h, variant);

                    if (current.index == bypassBlock && ! viaEntry)
                        h.bypass->store (1.0f);

                    if (current.index == editBlock && variant != 3)
                        applyBypEdits (h, variant);

                    if (current.index == resumeBlock && ! viaEntry)
                        h.bypass->store (0.0f);

                    if (viaEntry)
                        current.viaBypassed = current.index >= bypassBlock && current.index < resumeBlock;
                });

                // Fresh instances started at the un-bypass sample: on the edited
                // parameters (XR-byp) and on those from before (XR-bypn).
                auto edited = makeBypBands();
                applyBypEdits (handlesOf (*edited), variant);
                const auto editedOut = render (*edited, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

                auto before = makeBypBands();
                const auto beforeOut = render (*before, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

                const bool bypassed = sameFrom (output, bypassAt + fadeN, input, bypassAt + fadeN, resumeAt - bypassAt - fadeN);
                const bool resumed = sameFrom (output, resumeAt + fadeN, editedOut, fadeN, total - resumeAt - fadeN);
                const bool stale = sameFrom (output, resumeAt + fadeN, beforeOut, fadeN, total - resumeAt - fadeN);

                tally.check (bypassed && resumed,
                             juce::String (names[variant]) + (bypassed ? "" : ": the bypassed span is not the input from the toggle + N")
                                 + (resumed ? "" : ": from un-bypass + N differs from a fresh instance on the edited parameters"));

                control.check (! stale, juce::String (names[variant]) + ": equals a fresh instance on the parameters from before the "
                                            + (variant == 3 ? "gain move" : "bypass"));
            }

            row (tally.bad == 0 && tally.total == 4,
                 "XR-byp six mixed bands, noise: edited while fully bypassed (type, slope, placement, gain, frequency, Q, enable; or continuous only; or bypass engaged mid-ramp), both routes: from un-bypass + N memcmp-equal to a fresh instance on the edited parameters started at the un-bypass sample",
                 tally.detail() + "  N = " + juce::String (fadeN));

            row (control.bad == 0 && control.total == 4,
                 "XR-bypn the same compare against a fresh instance on the parameters from before the bypass (variant 4: before the gain move) differs (control fires)",
                 control.detail());
        }

        // XR-big (VERIFICATION-2.3 issue 3: the tick phase across chunks)
        {
            constexpr int hostBlock = 4096;
            const auto input = makeNoiseAt (2, 14 * hostBlock, 0x6B160002, kNoise12);

            const auto script = [] (Processor& target)
            {
                return [&target] (BlockInfo& current)
                {
                    switch (current.index)
                    {
                        case 2:  setRaw (target, bandId (1, "gain"), -3.0f);
                                 setRaw (target, bandId (2, "type"), (float) kBell);  break;
                        case 4:  setRaw (target, bandId (3, "slope"), 7.0f);
                                 setRaw (target, bandId (4, "placement"), (float) kSide);
                                 setRaw (target, bandId (5, "freq"), 15000.0f);  break;
                        case 6:  setRaw (target, bandId (6, "enabled"), 0.0f);
                                 setRaw (target, "bypass", 1.0f);  break;
                        case 8:  setRaw (target, "bypass", 0.0f);
                                 setRaw (target, bandId (6, "enabled"), 1.0f);  break;
                        case 10: setRaw (target, bandId (1, "freq"), 700.0f);
                                 setRaw (target, bandId (2, "q"), 3.0f);  break;
                        default: break;
                    }
                };
            };

            auto a = makeBands (2, 6, mixedSpec);
            auto b = makeBands (2, 6, mixedSpec);

            const auto oversized = render (*a, input, hostBlock, false, kRenderRate, script (*a), {}, 512);
            const auto prepared = render (*b, input, hostBlock, false, kRenderRate, script (*b));
            const bool acts = ! sameAudio (prepared, input);

            row (acts && sameAudio (oversized, prepared) && allFinite (prepared),
                 "XR-big host blocks of 4096 on a prepared size of 512, 6 bands with moves, structural events and a bypass toggle: bit-identical to the same script prepared at 4096",
                 sameAudio (oversized, prepared) ? juce::String (acts ? "" : "the setting does not act on the audio")
                                                 : "first differs at sample " + juce::String (firstDifference (oversized, prepared)));
        }

        // XR-prep (VERIFICATION-2.3 issue 3: the clock and the bands after a re-prepare)
        {
            constexpr int blockSize = 500;
            const double rate = kRenderRate;
            const auto first = makeNoiseAt (2, (int) std::lround (rate), 0x6B700002, kNoise12);
            const auto second = makeNoiseAt (2, (int) std::lround (0.5 * rate), 0x6B710002, kNoise12);
            const auto blocks500 = [] (int, int) { return blockSize; };

            // In flight at the end of the first render: a type fade and a gain ramp.
            const auto inFlight = [] (const Handles& target)
            {
                target.band[0].type->store ((float) kHighShelf);
                target.band[1].gain->store (3.0f);
            };

            const auto moves = [] (const Handles& target, int index)
            {
                if (index == 3)
                    target.band[2].gain->store (2.0f);

                if (index == 7)
                    target.band[3].freq->store (50.0f);

                if (index == 11)
                    target.band[4].type->store ((float) kLowShelf);
            };

            auto reused = makeBands (2, 6, mixedSpec);
            const Handles hr = handlesOf (*reused);

            const auto firstOut = renderWith (*reused, first, rate, blockSize, blocks500, [&hr, &inFlight] (BlockInfo& current)
            {
                if (current.index == 95)
                    inFlight (hr);
            });

            const auto secondOut = renderWith (*reused, second, rate, blockSize, blocks500, [&hr, &moves] (BlockInfo& current)
            {
                moves (hr, current.index);
            });

            auto fresh = makeBands (2, 6, mixedSpec);
            const Handles hf = handlesOf (*fresh);
            inFlight (hf);

            const auto freshOut = renderWith (*fresh, second, rate, blockSize, blocks500, [&hf, &moves] (BlockInfo& current)
            {
                moves (hf, current.index);
            });

            reused->releaseResources();
            fresh->releaseResources();

            const bool same = sameAudio (secondOut, freshOut);
            const bool acts = ! sameAudio (secondOut, second) && allFinite (firstOut);

            row (same && acts,
                 "XR-prep re-prepare at the same rate after 1 s (a type fade and a gain ramp in flight), then two moves and one type change in blocks of 500: memcmp-equal to a fresh instance",
                 (same ? juce::String() : "first differs at sample " + juce::String (firstDifference (secondOut, freshOut)) + "; ")
                     + (acts ? "" : "the bands do not act"));
        }
    }

    //==========================================================================
    // XR-blk. Block identity with structural events (PLAN-2.4 P9; fade_bench blk)
    //==========================================================================
    struct BlkBand
    {
        float type = 0.0f, slope = 1.0f, placement = 0.0f, enabled = 1.0f;
        float freq = 1000.0f, gain = 0.0f, q = 1.0f;
    };

    struct BlkEvent
    {
        BlkBand bands[6];
        float scale = 100.0f;
        float bypass = 0.0f;
    };

    /** XF-blk's script: 160 events; at each, every band redraws type and
        slope (p 0.5), placement (0.3), enable (0.3), frequency, gain (exactly
        0 with p 0.25) and Q; the global bypass toggles (p 0.1); and here also
        gain_scale (p 0.3, 0 - 200 %). The state carries from event to event. */
    std::vector<BlkEvent> makeStructuralEvents()
    {
        Xorshift draw;
        draw.state = 777;

        BlkEvent current;
        std::vector<BlkEvent> events;

        for (int e = 0; e < 160; ++e)
        {
            for (auto& c : current.bands)
            {
                if (draw.uniform() < 0.5)
                {
                    c.type = (float) juce::jmin (7, (int) (draw.uniform() * 8.0));
                    c.slope = (float) juce::jmin (7, (int) (draw.uniform() * 8.0));
                }

                if (draw.uniform() < 0.3)
                    c.placement = (float) juce::jmin (4, (int) (draw.uniform() * 5.0));

                if (draw.uniform() < 0.3)
                    c.enabled = draw.uniform() > 0.3 ? 1.0f : 0.0f;

                c.freq = juce::jlimit (10.0f, 30000.0f, (float) std::exp (std::log (10.0) + draw.uniform() * std::log (3000.0)));
                c.gain = draw.uniform() < 0.25 ? 0.0f : juce::jlimit (-30.0f, 30.0f, (float) (-30.0 + 60.0 * draw.uniform()));
                c.q = juce::jlimit (0.025f, 40.0f, (float) std::exp (std::log (0.025) + draw.uniform() * std::log (1600.0)));
            }

            if (draw.uniform() < 0.1)
                current.bypass = current.bypass >= 0.5f ? 0.0f : 1.0f;

            if (draw.uniform() < 0.3)
                current.scale = juce::jlimit (0.0f, 200.0f, (float) (200.0 * draw.uniform()));

            events.push_back (current);
        }

        return events;
    }

    void storeBlkEvent (const Handles& h, const BlkEvent& e)
    {
        h.gainScale->store (e.scale);
        h.bypass->store (e.bypass);

        for (size_t b = 0; b < 6; ++b)
        {
            const BlkBand& c = e.bands[b];
            const BandHandles& hb = h.band[b];
            hb.type->store (c.type);
            hb.slope->store (c.slope);
            hb.placement->store (c.placement);
            hb.enabled->store (c.enabled);
            hb.bypass->store (0.0f);
            hb.freq->store (c.freq);
            hb.gain->store (c.gain);
            hb.q->store (c.q);
        }
    }

    // withEvents false: event 0 at sample 0 and nothing after it.
    juce::AudioBuffer<float> structuralBlockRender (Processor& p, const Handles& h, const juce::AudioBuffer<float>& input,
                                                    double rate, int blockSize, const std::vector<BlkEvent>& events, bool withEvents)
    {
        storeBlkEvent (h, events[0]);

        return renderWith (p, input, rate, blockSize,
                           [blockSize] (int, int) { return blockSize; },
                           [&h, &events, withEvents] (BlockInfo& current)
                           {
                               if (current.position % 2048 != 0)
                                   return;

                               const auto slot = (size_t) (current.position / 2048);

                               if (slot >= events.size() || (! withEvents && slot > 0))
                                   return;

                               storeBlkEvent (h, events[slot]);
                           });
    }

    void structuralBlockRowsXr()
    {
        section ("XR-blk. block identity with structural events (Phase 2.4: PLAN-2.4 P9)");

        constexpr int length = 2048 * 160;
        const double rates[4] = { 44100.0, 48000.0, 96000.0, 192000.0 };
        const int sizes[4] = { 32, 64, 512, 2048 };
        const auto input = makeNoiseAt (2, length, 0x6B1C0002, kNoise12);
        const auto events = makeStructuralEvents();

        struct Job { double rate; int blockSize; bool withEvents; };
        std::vector<Job> jobs;

        for (const double rate : rates)
        {
            for (const int blockSize : sizes)
                jobs.push_back ({ rate, blockSize, true });

            jobs.push_back ({ rate, 512, false });
        }

        std::vector<std::unique_ptr<Processor>> instances;
        std::vector<Handles> handles;

        for (size_t j = 0; j < jobs.size(); ++j)
        {
            instances.push_back (makeBands (2, 6, mixedSpec));
            handles.push_back (handlesOf (*instances.back()));
        }

        std::vector<juce::AudioBuffer<float>> outputs (jobs.size());

        parallelFor ((int) jobs.size(), [&instances, &handles, &jobs, &outputs, &input, &events] (int j)
        {
            const auto slot = (size_t) j;
            outputs[slot] = structuralBlockRender (*instances[slot], handles[slot], input, jobs[slot].rate,
                                                   jobs[slot].blockSize, events, jobs[slot].withEvents);
        });

        Tally tally;

        for (size_t r = 0; r < 4; ++r)
        {
            const size_t base = r * 5;
            const juce::String where = fmtG (rates[r] / 1000.0, 4) + " kHz";

            tally.check (allFinite (outputs[base]) && ! sameAudio (outputs[base], input), where + ": not finite, or the bands do not act");

            for (size_t k = 1; k < 4; ++k)
                tally.check (sameAudio (outputs[base + k], outputs[base]),
                             where + ": block " + juce::String (sizes[k]) + " differs from block 32, first at sample "
                                 + juce::String (firstDifference (outputs[base + k], outputs[base])));

            tally.check (! sameAudio (outputs[base + 4], outputs[base + 2]), where + ": the render without events is the same (the events do not bite)");
        }

        row (tally.bad == 0 && tally.total == 20,
             "XR-blk six bands, 160 events on multiples of 2048 (type, slope, placement, enable, f, gain incl. exactly 0, Q, gain_scale, bypass): blocks 32 / 64 / 512 / 2048 bit-identical at 44.1 / 48 / 96 / 192 kHz; differs from the render without events",
             tally.detail());
    }

    //==========================================================================
    // A3, A3n. G6 with fades (PERF-01)
    //==========================================================================
    struct FadeAllocResult
    {
        AllocResult base;
        int viaEntryBlocks = 0;
    };

    /** The window is processBlock or processBlockBypassed alone, the first
        block after prepareToPlay included; the draw, the parameter stores, the
        fill and the scan stay outside it (stressLoop24). 20 s of the P14
        every-block draw at 48 kHz, every second bypassed span through
        processBlockBypassed. `plant`: one allocation inside the first window. */
    FadeAllocResult runFadeAllocWindow (bool plant)
    {
        FadeAllocResult result;

       #if defined (__APPLE__)
        const double rate = 48000.0;
        const double seconds = 20.0;

        const StressSeed chosen = chooseStressSeed24 (rate, seconds, 1);
        auto p = makeBands (2, 0, mixedSpec);
        const Handles h = handlesOf (*p);

        malloc_logger_t* const previous = malloc_logger;
        allocThread = pthread_self();
        allocCount = 0;
        freeCount = 0;
        malloc_logger = countAllocation;

        allocArmed = true;
        {
            void* volatile probe = std::malloc (64);
            std::free (probe);
        }
        allocArmed = false;
        result.base.probeAllocs = allocCount;
        result.base.probeFrees = freeCount;
        allocCount = 0;
        freeCount = 0;

        bool plantNext = plant;
        int viaEntry = 0;

        const StressOutcome outcome = stressLoop24 (*p, h, rate, seconds, chosen.seed, 1, true,
                                                    [&p, &plantNext, &viaEntry] (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, bool viaBypassed)
                                                    {
                                                        allocArmed = true;

                                                        if (viaBypassed)
                                                            p->processBlockBypassed (buffer, midiMessages);
                                                        else
                                                            p->processBlock (buffer, midiMessages);

                                                        if (plantNext)
                                                        {
                                                            void* volatile planted = std::malloc (64);
                                                            std::free (planted);
                                                        }

                                                        allocArmed = false;
                                                        plantNext = false;

                                                        if (viaBypassed)
                                                            ++viaEntry;
                                                    });

        malloc_logger = previous;
        p->releaseResources();

        result.base.available = true;
        result.base.allocs = allocCount;
        result.base.frees = freeCount;
        result.base.blocks = (int) outcome.blocks;
        result.base.audioOk = chosen.ok && outcome.nonFinite == 0 && outcome.actingBlocks > 0;
        result.viaEntryBlocks = viaEntry;
       #else
        juce::ignoreUnused (plant);
       #endif

        return result;
    }

    void fadeAllocationRowsXr()
    {
        section ("A3. allocation with fades and bypass (Phase 2.4: G6, PERF-01)");

        const FadeAllocResult clean = runFadeAllocWindow (false);

        if (! clean.base.available)
        {
            info ("A3 allocation gate SKIPPED: no malloc_logger on this platform");
            info ("A3n allocation control SKIPPED: no malloc_logger on this platform");
            return;
        }

        const bool hookLive = clean.base.probeAllocs >= 1 && clean.base.probeFrees >= 1;

        row (hookLive && clean.base.audioOk && clean.viaEntryBlocks > 0 && clean.base.allocs == 0 && clean.base.frees == 0,
             "A3 processBlock and processBlockBypassed, 20 s of the P14 every-block draw at 48 kHz (structure at every block, bypass toggles by both routes), first block included: 0 allocations, 0 frees",
             juce::String (clean.base.allocs) + " allocations, " + juce::String (clean.base.frees) + " frees in "
                 + juce::String (clean.base.blocks) + " windows (" + juce::String (clean.viaEntryBlocks)
                 + " through processBlockBypassed; malloc_logger, this thread); hook " + (hookLive ? "live" : "DEAD")
                 + " (probe: " + juce::String (clean.base.probeAllocs) + " / " + juce::String (clean.base.probeFrees) + ")"
                 + (clean.base.audioOk ? "" : "; the stress did not run as expected (stack guard, non-finite output, or no acting block)"));

        const FadeAllocResult planted = runFadeAllocWindow (true);

        row (planted.base.allocs >= 1 && planted.base.frees >= 1,
             "A3n a planted allocation inside the processBlock / processBlockBypassed window is counted (control fires)",
             juce::String (planted.base.allocs) + " allocations, " + juce::String (planted.base.frees) + " frees");
    }

    //==========================================================================
    // X-cpu3. CPU with fades (INFO; PLAN-2.4 P17; fade_bench cpu, RESEARCH-2.4 Q8)
    //==========================================================================
    // X-cpu3's figures, kept for X-cpu4 (Phase 2.5): [config][mode].
    double xcpu3Percent[4][4] = {};

    void cpuInfoFading()
    {
        section ("X3. CPU with fades (INFO, not gated; G10 binds in Stage 4)");

        constexpr int blockSize = 512;
        const double rate = 48000.0;
        const int numBlocks = (int) (20.0 * rate / (double) blockSize);
        const double audioSeconds = (double) numBlocks * (double) blockSize / rate;
        const auto noise = makeNoiseAt (2, blockSize, 0x0C9400002, kNoise12);

        struct Config { const char* name; int type; int slope; int type2; int slope2; const char* bench; };

        const Config configs[] =
        {
            { "24 cuts x 96 dB/oct (Low Cut <> High Cut)",       kLowCut,    7, kHighCut,   7, "2.65 / 5.17 / 2.71 / -" },
            { "24 shelves x 96 dB/oct (High <> Low)",            kHighShelf, 7, kLowShelf,  7, "4.93 / 9.88 / 5.06 / 0.04" },
            { "24 bells <> 12 dB/oct shelves",                   kBell,      0, kHighShelf, 1, "0.67 / 1.74 / 0.87 / 0.04" },
            { "24 shelves 96 <> 72 dB/oct",                      kHighShelf, 7, kHighShelf, 6, "4.97 / 8.39 / 4.26 / 0.04" }
        };

        const char* const modeNames[4] = { "settled", "always fading", "one fade per second", "24 asleep" };
        int configIndex = 0;        // Phase 2.5: the figures are kept for X-cpu4

        for (const auto& c : configs)
        {
            double percent[4] = { 0.0, 0.0, 0.0, 0.0 };
            bool finite = true;

            for (int mode = 0; mode < 4; ++mode)
            {
                auto p = makeBands (2, 24, [&c, mode] (int index)
                {
                    return makeSpec (c.type, 40.0f * std::pow (1.28f, (float) index), mode == 3 ? 0.0f : 6.0f, 1.0f, c.slope, kStereo);
                });

                const Handles h = handlesOf (*p);
                juce::AudioBuffer<float> block (2, blockSize);
                juce::MidiBuffer midi;
                double seconds = 0.0;

                p->prepareToPlay (rate, blockSize);

                for (int k = 0; k < numBlocks; ++k)
                {
                    const double t = (double) (k * blockSize) / rate;
                    bool second = false;

                    if (mode == 1)
                        second = (((long) (t / 0.0215)) & 1) != 0;       // a new request as soon as each fade has ended

                    if (mode == 2)
                        second = (((long) t) & 1) != 0;

                    for (const auto& hb : h.band)
                    {
                        hb.type->store ((float) (second ? c.type2 : c.type));
                        hb.slope->store ((float) (second ? c.slope2 : c.slope));
                    }

                    for (int channel = 0; channel < 2; ++channel)
                        block.copyFrom (channel, 0, noise, channel, 0, blockSize);

                    const auto t0 = std::chrono::steady_clock::now();
                    p->processBlock (block, midi);
                    const auto t1 = std::chrono::steady_clock::now();

                    seconds += std::chrono::duration<double> (t1 - t0).count();
                }

                p->releaseResources();

                percent[mode] = 100.0 * seconds / audioSeconds;
                finite = finite && allFinite (block);
            }

            juce::String line = juce::String ("X-cpu3 48 kHz stereo, 512-sample blocks, 20 s, ") + c.name + ":";

            for (int mode = 0; mode < 4; ++mode)
                line += juce::String (" ") + modeNames[mode] + " " + fmtG (percent[mode], 3) + " %" + (mode < 3 ? ";" : "");

            info (line + " of one core (bench settled / always fading / one per second / asleep: " + c.bench
                      + " %; compare X-cpu2 settled above)" + (finite ? "" : "  OUTPUT NOT FINITE"));

            for (int mode = 0; mode < 4; ++mode)
                xcpu3Percent[configIndex][mode] = percent[mode];

            ++configIndex;
        }
    }

    //==========================================================================
    //==========================================================================
    // Stage 2, Phase 2.5: output stage and auto gain (PLAN-2.5 T7)
    //==========================================================================
    //==========================================================================

    //==========================================================================
    // The stage re-stated (PLAN-2.5 P2, P3, P5, P8). Harness code: none of
    // GainSmoother, OutputStage or AutoGain is used by gainTrajectory,
    // gainRefRender, hardGainRender or agRef. The plan's numbers are typed
    // here again.
    //==========================================================================
    constexpr double kRefPoleX = 13.0622;           // a four-pole step is within 1e-3 of its size at N
    constexpr double kRefPoleSnap = 1.0e-9;         // all four stages this close: set to the target
    constexpr double kRefOutputSeconds = 0.020;
    constexpr double kRefAutoSeconds = 0.100;

    /** N = round (t fs), at least 1. */
    int refPoleLength (double seconds, double rate)
    {
        const double n = std::round (seconds * rate);
        return n < 1.0 ? 1 : (int) n;
    }

    /** output_gain raw (dB) to the linear target: non-finite reads 0 dB, the
        value is clamped to +-36 dB, 10^(dB / 20). */
    double refOutputTarget (double rawDb)
    {
        const double db = std::isfinite (rawDb) ? juce::jlimit (-36.0, 36.0, rawDb) : 0.0;
        return std::pow (10.0, db / 20.0);
    }

    /** Ticks between auto-gain refreshes: max (1, round (1024 / 48000 fs / K)). */
    int refRefreshTicks (double rate)
    {
        const double r = std::round (1024.0 / 48000.0 * rate / (double) harnessGrid (rate));
        return r < 1.0 ? 1 : (int) r;
    }

    /** One four-pole cascade in amplitude: y1 += a (T - y1); y2 += a (y1 - y2);
        y3 += a (y2 - y3); y4 += a (y3 - y4); a = 1 - exp (-X / N); all four
        within 1e-9 of T: all set to T. A cascade sitting on its target stays
        there exactly (each step adds a * 0). */
    struct RefPole4
    {
        double y[4] = { 1.0, 1.0, 1.0, 1.0 };
        double target = 1.0;
        double a = 1.0;

        void prepare (int length)       { a = 1.0 - std::exp (-kRefPoleX / (double) juce::jmax (1, length)); }
        void snap (double t)            { target = t; y[0] = y[1] = y[2] = y[3] = t; }
        void retarget (double t)        { target = t; }

        double next()
        {
            y[0] += a * (target - y[0]);
            y[1] += a * (y[0] - y[1]);
            y[2] += a * (y[1] - y[2]);
            y[3] += a * (y[2] - y[3]);

            if (std::abs (y[0] - target) <= kRefPoleSnap && std::abs (y[1] - target) <= kRefPoleSnap
                && std::abs (y[2] - target) <= kRefPoleSnap && std::abs (y[3] - target) <= kRefPoleSnap)
                y[0] = y[1] = y[2] = y[3] = target;

            return y[3];
        }
    };

    /** What the stage reads at each host block start: the sanitised output
        target (linear), whether auto_gain is wanted, and the estimate of the
        bands' latest requests (the auto-gain target while on). */
    struct GainScript
    {
        std::vector<int> blockStart;
        std::vector<double> outputTarget;
        std::vector<char> autoWanted;
        std::vector<double> autoEstimate;

        void add (int start, double output, bool wanted, double estimate)
        {
            blockStart.push_back (start);
            outputTarget.push_back (output);
            autoWanted.push_back ((char) (wanted ? 1 : 0));
            autoEstimate.push_back (estimate);
        }
    };

    /** The per-sample gain of the stage (P3, P5, P6) for a render with no
        bypass: seeded at sample 0 (both cascades on their targets); at every
        tick (clock a multiple of K) the output target read at the last block
        start goes to its cascade; on a refresh tick (tick index a multiple of
        R) auto gain takes its wanted state and its target, the estimate or
        1.0. g = auto x output per sample. hard (hardGainRender): each cascade
        jumps to its target there instead of moving. */
    std::vector<double> gainTrajectory (const GainScript& script, int length, double rate, bool hard)
    {
        const size_t blocks = script.blockStart.size();

        if (blocks == 0 || script.blockStart[0] != 0 || script.outputTarget.size() != blocks
            || script.autoWanted.size() != blocks || script.autoEstimate.size() != blocks || length <= 0)
            return {};

        const int grid = harnessGrid (rate);
        const int refresh = refRefreshTicks (rate);

        RefPole4 og, ag;
        og.prepare (refPoleLength (kRefOutputSeconds, rate));
        ag.prepare (refPoleLength (kRefAutoSeconds, rate));

        double pending = script.outputTarget[0];
        bool wanted = script.autoWanted[0] != 0;
        double estimate = script.autoEstimate[0];

        og.snap (pending);
        ag.snap (wanted ? estimate : 1.0);

        std::vector<double> g ((size_t) length, 1.0);
        size_t k = 0;

        for (int n = 0; n < length; ++n)
        {
            while (k + 1 < blocks && script.blockStart[k + 1] <= n)
            {
                ++k;
                pending = script.outputTarget[k];
                wanted = script.autoWanted[k] != 0;
                estimate = script.autoEstimate[k];
            }

            if (n % grid == 0)
            {
                if (hard)
                    og.snap (pending);
                else
                    og.retarget (pending);

                if ((n / grid) % refresh == 0)
                {
                    const double autoTarget = wanted ? estimate : 1.0;

                    if (hard)
                        ag.snap (autoTarget);
                    else
                        ag.retarget (autoTarget);
                }
            }

            const double gA = ag.next();
            g[(size_t) n] = gA * og.next();
        }

        return g;
    }

    /** gainRefRender (hard false) and hardGainRender (hard true): the plugin's
        float output rendered with the gains neutral (output 0 dB, auto gain
        off), widened, times the re-stated gain, in double. */
    std::array<std::vector<double>, 2> gainRefRender (const juce::AudioBuffer<float>& neutral, const GainScript& script,
                                                      double rate, bool hard)
    {
        std::array<std::vector<double>, 2> out;
        const int length = neutral.getNumSamples();
        const auto g = gainTrajectory (script, length, rate, hard);

        if (g.size() != (size_t) length)
            return out;

        for (int channel = 0; channel < juce::jmin (2, neutral.getNumChannels()); ++channel)
        {
            auto y = widened (neutral, channel);

            for (size_t i = 0; i < y.size(); ++i)
                y[i] *= g[i];

            out[(size_t) channel] = std::move (y);
        }

        return out;
    }

    /** max |y - ref| over [from, to) re the largest |y| there, in dB. A
        missing reference or a non-finite sample reads +inf / NaN (fails any
        "<=" test). */
    double errorReDb (const juce::AudioBuffer<float>& y, const std::array<std::vector<double>, 2>& ref, int from, int to)
    {
        double error = 0.0, peak = 0.0;

        for (int channel = 0; channel < juce::jmin (2, y.getNumChannels()); ++channel)
        {
            const auto& r = ref[(size_t) channel];

            if (r.size() != (size_t) y.getNumSamples())
                return std::numeric_limits<double>::infinity();

            const float* data = y.getReadPointer (channel);

            for (int i = juce::jmax (0, from); i < juce::jmin (to, y.getNumSamples()); ++i)
            {
                const double v = (double) data[i];
                const double d = std::abs (v - r[(size_t) i]);

                if (! (d <= error))
                    error = d;

                peak = juce::jmax (peak, std::abs (v));
            }
        }

        if (! (peak > 0.0))
            return std::numeric_limits<double>::infinity();

        return 20.0 * std::log10 (juce::jmax (error, 1.0e-300) / peak);
    }

    /** The largest |y[n + 1] - 2 y[n] + y[n - 1]| for n in [from, to) (gain_bench maxD2). */
    double maxD2Between (const std::vector<double>& y, size_t from, size_t to)
    {
        double d = 0.0;
        const size_t end = juce::jmin (to, y.size());

        for (size_t i = juce::jmax (from, (size_t) 1); i + 1 < end; ++i)
        {
            const double v = std::abs (y[i + 1] - 2.0 * y[i] + y[i - 1]);

            if (! (v <= d))
                d = v;
        }

        return d;
    }

    double maxD2Stereo (const juce::AudioBuffer<float>& y, size_t from, size_t to)
    {
        return juce::jmax (maxD2Between (widened (y, 0), from, to), maxD2Between (widened (y, 1), from, to));
    }

    double maxD2Stereo (const std::array<std::vector<double>, 2>& y, size_t from, size_t to)
    {
        return juce::jmax (maxD2Between (y[0], from, to), maxD2Between (y[1], from, to));
    }

    //==========================================================================
    // agRef: the auto-gain estimate re-stated (PLAN-2.5 P4; ARCHITECTURE
    // "Auto gain" as amended by C1). Harness code, no AutoGain.
    //==========================================================================
    struct AgRef
    {
        double linear = 1.0;
        double db = 0.0;
        int contributing = 0;
    };

    /** 512 cell midpoints f_i = exp (ln 40 + (ln fTop - ln 40)(i + 0.5) / 512),
        fTop = min (16 kHz, 0.45 fs), t2 = tan^2 (pi f / fs). A band
        contributes when active, not a gain type at exactly 0 dB, and (stereo)
        not on Side. Its |H|^2 is the product of sectionMag2 over the sections
        of Designer::design of its settings, multiplied into L (Stereo, Mid,
        Left) and R (Stereo, Mid, Right); mono: both. mean = sum of
        0.5 (L + R) / 512; NaN 0 dB, +inf -18 dB, <= 1e-30 +12 dB, else
        -10 log10 (mean) clamped to [-18, +12]. None contributing: exactly 1. */
    AgRef agRef (const std::vector<eq::BandInput>& inputs, bool stereo, double rate, const eq::Designer& designer)
    {
        constexpr int points = 512;
        const double low = 40.0;
        const double top = juce::jmin (16000.0, 0.45 * rate);

        std::vector<double> t2 ((size_t) points), laneL ((size_t) points, 1.0), laneR ((size_t) points, 1.0);

        for (int i = 0; i < points; ++i)
        {
            const double u = ((double) i + 0.5) / (double) points;
            const double f = std::exp (std::log (low) + (std::log (top) - std::log (low)) * u);
            const double t = std::tan (eq::pi * f / rate);
            t2[(size_t) i] = t * t;
        }

        AgRef result;

        for (const auto& in : inputs)
        {
            if (! in.active)
                continue;

            if (eq::isGainType (in.settings.type) && in.settings.gainDbEff == 0.0)
                continue;

            if (stereo && in.placement == eq::Placement::side)
                continue;

            const bool intoL = ! stereo || in.placement == eq::Placement::stereo || in.placement == eq::Placement::mid
                            || in.placement == eq::Placement::left;
            const bool intoR = ! stereo || in.placement == eq::Placement::stereo || in.placement == eq::Placement::mid
                            || in.placement == eq::Placement::right;

            eq::BandDesign design {};
            designer.design (in.settings, design);
            ++result.contributing;

            for (int i = 0; i < points; ++i)
            {
                double power = 1.0;

                for (int s = 0; s < design.numSections; ++s)
                    power *= eq::sectionMag2 (t2[(size_t) i], design.params[(size_t) s]);

                if (intoL)
                    laneL[(size_t) i] *= power;

                if (intoR)
                    laneR[(size_t) i] *= power;
            }
        }

        if (result.contributing == 0)
            return result;

        double sum = 0.0;

        for (int i = 0; i < points; ++i)
            sum += 0.5 * (laneL[(size_t) i] + laneR[(size_t) i]);

        const double mean = sum / (double) points;

        if (std::isnan (mean))
            return result;

        double db = 0.0;

        if (std::isinf (mean))
            db = -18.0;
        else if (mean <= 1.0e-30)
            db = 12.0;
        else
            db = juce::jlimit (-18.0, 12.0, -10.0 * std::log10 (mean));

        result.db = db;
        result.linear = std::pow (10.0, db / 20.0);
        return result;
    }

    /** The raw values of a BandSpec (enabled, band bypass off). */
    eq::RawBand rawBandSpec (const BandSpec& s, bool enabled = true)
    {
        eq::RawBand raw;
        raw.enabled = enabled ? 1.0f : 0.0f;
        raw.bypass = 0.0f;
        raw.type = (float) s.type;
        raw.freq = s.freq;
        raw.gain = s.gain;
        raw.q = s.q;
        raw.slope = (float) s.slope;
        raw.placement = (float) s.placement;
        return raw;
    }

    /** A disabled band at the layout's defaults. */
    eq::RawBand rawBandOff()
    {
        return rawBandSpec (makeSpec (kBell, 1000.0f, 0.0f, 1.0f, 1, kStereo), false);
    }

    /** What the processor hands its band for these raw values (makeInput). */
    eq::BandInput rawInput (const eq::RawBand& raw, float gainScale, bool stereo)
    {
        eq::BandInput in = eq::makeBandInput (raw, gainScale, 0.0);
        in.placement = eq::effectivePlacement (in.placement, stereo);
        return in;
    }

    //==========================================================================
    // Band-limited pink noise, FFT-shaped (XR-g12a)
    //==========================================================================
    void fftRadix2 (std::vector<std::complex<double>>& a, bool inverse)
    {
        const size_t n = a.size();

        for (size_t i = 1, j = 0; i < n; ++i)
        {
            size_t bit = n >> 1;

            for (; (j & bit) != 0; bit >>= 1)
                j ^= bit;

            j ^= bit;

            if (i < j)
                std::swap (a[i], a[j]);
        }

        for (size_t len = 2; len <= n; len <<= 1)
        {
            const double step = (inverse ? 2.0 : -2.0) * eq::pi / (double) len;

            for (size_t k = 0; k < len / 2; ++k)
            {
                const std::complex<double> w (std::cos (step * (double) k), std::sin (step * (double) k));

                for (size_t i = 0; i < n; i += len)
                {
                    const std::complex<double> u = a[i + k];
                    const std::complex<double> v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                }
            }
        }
    }

    /** Per channel: a spectrum of 2^m >= length bins with magnitude 1 / sqrt (f)
        from 40 Hz to 16 kHz and 0 elsewhere, seeded random phases, inverse
        FFT; the first `length` samples, scaled to `rms`. */
    juce::AudioBuffer<float> pinkNoise (int numChannels, int length, double rate, std::uint64_t seed, double rms)
    {
        size_t n = 1;

        while (n < (size_t) length)
            n <<= 1;

        juce::AudioBuffer<float> out (numChannels, length);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            Xorshift random;
            random.state = seed + 0x9E3779B97F4A7C15ull * (std::uint64_t) (channel + 1);

            std::vector<std::complex<double>> spectrum (n);

            for (size_t k = 1; k < n / 2; ++k)
            {
                const double f = (double) k * rate / (double) n;

                if (f < 40.0 || f > 16000.0)
                    continue;

                const double phase = 2.0 * eq::pi * random.uniform();
                spectrum[k] = std::polar (1.0 / std::sqrt (f), phase);
                spectrum[n - k] = std::conj (spectrum[k]);
            }

            fftRadix2 (spectrum, true);

            double energy = 0.0;

            for (int i = 0; i < length; ++i)
                energy += spectrum[(size_t) i].real() * spectrum[(size_t) i].real();

            const double scale = rms / std::sqrt (energy / (double) length);
            float* data = out.getWritePointer (channel);

            for (int i = 0; i < length; ++i)
                data[i] = (float) (spectrum[(size_t) i].real() * scale);
        }

        return out;
    }

    /** RMS of y over RMS of x, both channels, over [from, to), in dB. */
    double rmsRatioDb (const juce::AudioBuffer<float>& x, const juce::AudioBuffer<float>& y, int from, int to)
    {
        double sx = 0.0, sy = 0.0;

        for (int channel = 0; channel < x.getNumChannels(); ++channel)
        {
            const float* a = x.getReadPointer (channel);
            const float* b = y.getReadPointer (channel);

            for (int i = from; i < to; ++i)
            {
                sx += (double) a[i] * (double) a[i];
                sy += (double) b[i] * (double) b[i];
            }
        }

        return 10.0 * std::log10 (sy / sx);
    }

    //==========================================================================
    // XR-g12o, XR-g12s, XR-g12a, XR-g12d, XR-g12n. G12 (FUNC-05, FUNC-06)
    //==========================================================================
    void gainG12RowsXr()
    {
        section ("XR-g12. G12 through the plugin (Phase 2.5: FUNC-05, FUNC-06; PLAN-2.5 T7)");

        const double rate = 48000.0;
        const auto blocks512 = [] (int, int) { return 512; };

        // XR-g12o: the edit is seen by the first block starting at 100 ms or
        // later; the RMS ratio is read from 100 ms after it, whole periods.
        {
            const int length = (int) rate;
            constexpr int editAt = 4800;
            const auto input = stepStimulus (1000.0, rate, length);

            Tally tally;
            double worst = 0.0;

            for (const double db : { -36.0, -12.0, 0.0, 12.0, 36.0 })
            {
                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                int fired = -1;

                const auto output = renderWith (*p, input, rate, 512, blocks512, [&h, &fired, db] (BlockInfo& current)
                {
                    if (fired < 0 && current.position >= editAt)
                    {
                        fired = current.position;
                        h.outputGain->store ((float) db);
                    }
                });

                p->releaseResources();

                const int from = fired + 4800;
                const int to = from + ((length - from) / 48) * 48;

                for (int channel = 0; channel < 2; ++channel)
                {
                    double sx = 0.0, sy = 0.0;

                    for (int i = from; i < to; ++i)
                    {
                        sx += (double) input.getSample (channel, i) * (double) input.getSample (channel, i);
                        sy += (double) output.getSample (channel, i) * (double) output.getSample (channel, i);
                    }

                    const double error = 10.0 * std::log10 (sy / sx) - db;

                    if (! (std::abs (error) <= worst))
                        worst = std::abs (error);

                    tally.check (fired >= 0 && std::abs (error) <= 1.0e-4 && std::abs (error) <= 0.05,
                                 fmtG (db, 3) + " dB, channel " + juce::String (channel) + ": " + fmtG (error, 3) + " dB off");
                }
            }

            row (tally.bad == 0 && tally.total == 10,
                 "XR-g12o G12 output gain -36 / -12 / 0 / +12 / +36 dB, 1 kHz sine, 48 kHz, read from 100 ms after the edit: RMS ratio within 1e-4 dB of the target, both channels (limit +-0.05 dB)",
                 tally.detail() + "  worst " + fmtG (worst, 3) + " dB");
        }

        // XR-g12s: the measured gain at the bell's centre against +-12 dB
        // within the G1 tolerance (Phase 2.1b bell row at 48 kHz:
        // 0.05 + 0.012 |g|), and the render bit-identical to the raw +-12 dB
        // bell at 100 %.
        {
            const int length = (int) (0.75 * rate);
            const auto input = stepStimulus (1000.0, rate, length);
            const double tolerance = 0.05 + 0.012 * 12.0;

            Tally tally;
            juce::String figures;

            for (const float sign : { 1.0f, -1.0f })
            {
                // Raw stores: a normalised write would round the raw gain (6 -> 6.0000005).
                auto scaled = makeBands (2, 1, mixedSpec);
                const Handles hs = handlesOf (*scaled);
                storeRaw (hs.band[0], rawBandSpec (makeSpec (kBell, 1000.0f, 6.0f * sign, 1.0f, 1, kStereo)));
                hs.gainScale->store (200.0f);

                auto plain = makeBands (2, 1, mixedSpec);
                storeRaw (handlesOf (*plain).band[0], rawBandSpec (makeSpec (kBell, 1000.0f, 12.0f * sign, 1.0f, 1, kStereo)));

                const auto outScaled = render (*scaled, input, 512, false);
                const auto outPlain = render (*plain, input, 512, false);

                const double measured = toneGainDb (input, outScaled, 9600, 24000, 1000.0, rate);
                const double target = 12.0 * (double) sign;
                const bool same = sameAudio (outScaled, outPlain);

                tally.check (std::abs (measured - target) <= tolerance && same,
                             fmtG (6.0 * sign, 2) + " dB at 200 %: " + fmtG (measured, 6) + " dB" + (same ? "" : ", differs from the raw +-12 dB bell"));

                figures += (figures.isEmpty() ? "" : ", ") + fmtG (6.0 * sign, 2) + " dB -> " + fmtG (measured, 6) + " dB";
            }

            row (tally.bad == 0 && tally.total == 2,
                 "XR-g12s G12 gain scale 200 % on +-6 dB bells (1 kHz, Q 1): the centre gain is +-12 dB within the G1 tolerance (0.05 + 0.012 |g| = 0.194 dB), and the render is memcmp-equal to the raw +-12 dB bell at 100 %",
                 tally.detail() + "  " + figures);
        }

        // XR-g12a, XR-g12d, XR-g12n
        {
            const int length = (int) (12.0 * rate);
            const int from = (int) rate;
            const auto input = pinkNoise (2, length, rate, 0x9125EED5ull, 0.1);
            const BandSpec shelf = makeSpec (kLowShelf, 200.0f, 12.0f, 1.0f, 1, kStereo);

            std::vector<std::unique_ptr<Processor>> instances;

            for (int k = 0; k < 3; ++k)
            {
                instances.push_back (makeBands (2, 1, [&shelf] (int) { return shelf; }));
                handlesOf (*instances.back()).autoGain->store (k < 2 ? 1.0f : 0.0f);
            }

            std::vector<juce::AudioBuffer<float>> outputs (3);

            parallelFor (3, [&instances, &outputs, &input, rate, blocks512] (int j)
            {
                outputs[(size_t) j] = renderWith (*instances[(size_t) j], input, rate, 512, blocks512, [] (BlockInfo&) {});
                instances[(size_t) j]->releaseResources();
            });

            const auto designer = designerFor (rate);
            const std::vector<eq::BandInput> bands { rawInput (rawBandSpec (shelf), 100.0f, true) };
            const AgRef estimate = agRef (bands, true, rate, *designer);

            // Printed for information only: the plugin's own estimator on the same input.
            auto ag = std::make_unique<eq::AutoGain>();
            ag->prepare (rate);
            std::vector<eq::BandInput> all ((size_t) Processor::kNumBands, rawInput (rawBandOff(), 100.0f, true));
            all[0] = bands[0];
            std::vector<const eq::BandInput*> desired;

            for (const auto& in : all)
                desired.push_back (&in);

            const double pluginDb = 20.0 * std::log10 (ag->evaluateAll (desired.data(), true, *designer));

            const double onDb = rmsRatioDb (input, outputs[0], from, length);
            const double offDb = rmsRatioDb (input, outputs[2], from, length);
            const bool finite = allFinite (outputs[0]) && allFinite (outputs[2]);
            const bool estimateOk = std::abs (estimate.db - (-6.167)) <= 0.001;

            row (finite && std::abs (onDb) <= 0.5 && estimateOk,
                 "XR-g12a G12 auto gain: +12 dB Low Shelf 200 Hz 12 dB/oct, auto_gain on, band-limited pink 40 Hz - 16 kHz (FFT-shaped, 12 s, seeded), RMS from 1 s on: output / input within +-0.5 dB; agRef estimate -6.167 dB +- 0.001",
                 "output / input " + fmtG (onDb, 4) + " dB; agRef " + fmtG (estimate.db, 6) + " dB (eq::AutoGain, printed: "
                     + fmtG (pluginDb, 6) + " dB)");

            row (sameAudio (outputs[0], outputs[1]) && ! sameAudio (outputs[0], input),
                 "XR-g12d two renders of XR-g12a (two instances) are bit-identical",
                 sameAudio (outputs[0], outputs[1]) ? juce::String() : "first differs at sample " + juce::String (firstDifference (outputs[0], outputs[1])));

            row (finite && std::abs (offDb) > 0.5,
                 "XR-g12n XR-g12a with auto_gain off reads outside +-0.5 dB (control fires)",
                 "output / input " + fmtG (offDb, 4) + " dB (expected about +6.2)");
        }
    }

    //==========================================================================
    // XR-gstep, XR-gsteph, XR-gret, XR-ref. G3 step rows for the gains
    //==========================================================================
    constexpr double kGainRate = 48000.0;

    // The band of every gain row, running so the band path is live: Bell
    // 3 kHz +6 dB Q 1, Stereo (band 1).
    BandSpec gainBellSpec()
    {
        return makeSpec (kBell, 3000.0f, 6.0f, 1.0f, 1, kStereo);
    }

    // The auto-gain rows' second band (band 2): a High Shelf at 20 Hz,
    // 12 dB/oct, Q 1, a near-broadband gain; its raw gain is picked so the
    // estimate of the two bands is the row's target.
    BandSpec gainShelfSpec (float gainDb)
    {
        return makeSpec (kHighShelf, 20.0f, gainDb, 1.0f, 1, kStereo);
    }

    struct GainBandSet
    {
        double targetDb = 0.0;
        float shelfGain = 0.0f;
        double estimateDb = 0.0;
        double estimateLinear = 1.0;
    };

    std::vector<eq::BandInput> gainSetInputs (const GainBandSet* set)
    {
        std::vector<eq::BandInput> in;
        in.push_back (rawInput (rawBandSpec (gainBellSpec()), 100.0f, true));

        if (set != nullptr)
            in.push_back (rawInput (rawBandSpec (gainShelfSpec (set->shelfGain)), 100.0f, true));

        return in;
    }

    /** Bisection on the shelf's raw gain in [-30, +30] dB (the estimate falls
        as it rises), then the estimate of the float value picked. */
    GainBandSet pickGainSet (double targetDb, const eq::Designer& designer)
    {
        GainBandSet set;
        set.targetDb = targetDb;
        double lo = -30.0, hi = 30.0;

        for (int i = 0; i < 60; ++i)
        {
            const double mid = 0.5 * (lo + hi);
            set.shelfGain = (float) mid;

            if (agRef (gainSetInputs (&set), true, kGainRate, designer).db > targetDb)
                lo = mid;
            else
                hi = mid;
        }

        set.shelfGain = (float) (0.5 * (lo + hi));
        const AgRef r = agRef (gainSetInputs (&set), true, kGainRate, designer);
        set.estimateDb = r.db;
        set.estimateLinear = r.linear;
        return set;
    }

    struct GainStepRow
    {
        std::string name;
        int set = -1;                   // -1: an output-gain row; else the auto-gain band set
        double ogA = 0.0, ogB = 0.0;    // dB
        bool agA = false, agB = false;
    };

    /** The 20 ordered output-gain pairs of -36 / -12 / 0 / +12 / +36 dB, and
        auto_gain off -> on and on -> off for each of the four band sets. */
    std::vector<GainStepRow> gainStepRows (const std::vector<GainBandSet>& sets)
    {
        std::vector<GainStepRow> rows;
        const double levels[5] = { -36.0, -12.0, 0.0, 12.0, 36.0 };

        for (const double a : levels)
            for (const double b : levels)
                if (a != b)
                {
                    GainStepRow r;
                    r.name = "output gain " + fmtG (a, 3).toStdString() + " -> " + fmtG (b, 3).toStdString() + " dB";
                    r.ogA = a;
                    r.ogB = b;
                    rows.push_back (r);
                }

        for (size_t s = 0; s < sets.size(); ++s)
            for (const bool toOn : { true, false })
            {
                GainStepRow r;
                r.name = std::string ("auto gain ") + (toOn ? "off -> on" : "on -> off") + ", estimate "
                       + fmtG (sets[s].estimateDb, 4).toStdString() + " dB";
                r.set = (int) s;
                r.agA = ! toOn;
                r.agB = toOn;
                rows.push_back (r);
            }

        return rows;
    }

    /** 0: A, 1: B. motion 0: A, then B / A alternating from 0.5 s, every 100 ms
        for 8 steps (output gain) or every 400 ms for 4 steps (auto gain: the
        100 ms cascade is exact about 230 ms after its refresh tick); 1: held
        at A; 2: held at B. */
    int gainStepState (const GainStepRow& r, int motion, int position, double rate)
    {
        if (motion == 2)
            return 1;

        if (motion == 1)
            return 0;

        const double stepSeconds = r.set < 0 ? 0.100 : 0.400;
        const int numSteps = r.set < 0 ? 8 : 4;
        const double t = (double) position / rate;

        if (t < 0.5)
            return 0;

        const int k = (int) ((t - 0.5) / stepSeconds);
        return k < numSteps ? ((k + 1) & 1) : (numSteps & 1);
    }

    int gainStepLength (bool autoRow, double rate)
    {
        return (int) ((autoRow ? 0.5 + 4 * 0.4 + 0.4 : 0.5 + 8 * 0.1 + 0.3) * rate);
    }

    /** Every parameter a gain row uses. neutral: output 0 dB, auto gain off,
        the same bands. */
    void storeGainStep (const Handles& h, const GainStepRow& r, const std::vector<GainBandSet>& sets, int state, bool neutral)
    {
        const double og = (neutral || r.set >= 0) ? 0.0 : (state != 0 ? r.ogB : r.ogA);
        const bool ag = ! neutral && r.set >= 0 && (state != 0 ? r.agB : r.agA);

        h.gainScale->store (100.0f);
        h.bypass->store (0.0f);
        h.outputGain->store ((float) og);
        h.autoGain->store (ag ? 1.0f : 0.0f);

        storeRaw (h.band[0], rawBandSpec (gainBellSpec()));
        storeRaw (h.band[1], r.set >= 0 ? rawBandSpec (gainShelfSpec (sets[(size_t) r.set].shelfGain)) : rawBandOff());

        for (size_t b = 2; b < h.band.size(); ++b)
            storeRaw (h.band[b], rawBandOff());
    }

    juce::AudioBuffer<float> renderGainStep (Processor& p, const Handles& h, const juce::AudioBuffer<float>& input, double rate,
                                             const GainStepRow& r, const std::vector<GainBandSet>& sets, int motion, int blockSize,
                                             bool neutral, std::vector<int>* starts)
    {
        storeGainStep (h, r, sets, gainStepState (r, motion, 0, rate), neutral);

        return renderWith (p, input, rate, blockSize,
                           [blockSize] (int, int) { return blockSize; },
                           [&h, &r, &sets, motion, rate, neutral, starts] (BlockInfo& current)
                           {
                               storeGainStep (h, r, sets, gainStepState (r, motion, current.position, rate), neutral);

                               if (starts != nullptr)
                                   starts->push_back (current.position);
                           });
    }

    /** What the stage reads at the recorded block starts of a gain row render. */
    GainScript gainStepScript (const GainStepRow& r, const std::vector<GainBandSet>& sets, int motion,
                               const std::vector<int>& starts, double rate)
    {
        GainScript script;

        for (const int position : starts)
        {
            const int state = gainStepState (r, motion, position, rate);
            const double og = r.set >= 0 ? 0.0 : (state != 0 ? r.ogB : r.ogA);
            const bool ag = r.set >= 0 && (state != 0 ? r.agB : r.agA);

            script.add (position, refOutputTarget ((double) (float) og), ag,
                        r.set >= 0 ? sets[(size_t) r.set].estimateLinear : 1.0);
        }

        return script;
    }

    struct GainStepReading
    {
        bool valid = false;
        double dHeld = 0.0, dEng = 0.0, dHard = 0.0;
        double kHeld = 0.0, kEng = 0.0;         // frames from 0.3 s (the 2.4 step-row reading; printed)
        double kRatio = 0.0;                    // gated: per change, frames anchored where the stage starts moving
        double refErrorDb = 0.0;        // the worst of held A, held B and the step render against gainRefRender
        int refRenders = 0;
    };

    GainStepReading runGainStepJob (Processor& p, const Handles& h, const GainStepRow& r, const std::vector<GainBandSet>& sets,
                                    const juce::AudioBuffer<float>& input, int blockSize, bool withHard)
    {
        const double rate = kGainRate;
        const auto from = (size_t) std::lround (0.3 * rate);
        const int length = input.getNumSamples();

        std::vector<int> startsA, startsB, startsE;
        const auto heldA = renderGainStep (p, h, input, rate, r, sets, 1, blockSize, false, &startsA);
        const auto heldB = renderGainStep (p, h, input, rate, r, sets, 2, blockSize, false, &startsB);
        const auto engine = renderGainStep (p, h, input, rate, r, sets, 0, blockSize, false, &startsE);
        const auto neutral = renderGainStep (p, h, input, rate, r, sets, 0, blockSize, true, nullptr);

        const StereoReading a = readStereo (heldA, rate, from);
        const StereoReading b = readStereo (heldB, rate, from);
        const StereoReading e = readStereo (engine, rate, from);

        GainStepReading q;
        q.dHeld = juce::jmax (a.d2, b.d2);
        q.kHeld = juce::jmax (a.kappa, b.kappa);
        q.dEng = e.d2;
        q.kEng = e.kappa;

        // Local kappa per change, scored as gain_bench steps scores it: the
        // 5 ms frames start 50 ms before the sample where the stage starts
        // moving (the block start that sees an output-gain change, a tick;
        // the first refresh tick at or after the block start that sees an
        // auto_gain toggle) and end 50 ms after the cascade's length N; the
        // same span of both held renders. The ratio is the worst change.
        {
            const int grid = harnessGrid (rate);
            const int refreshSamples = refRefreshTicks (rate) * grid;
            const int n = refPoleLength (r.set >= 0 ? kRefAutoSeconds : kRefOutputSeconds, rate);
            const int lead = (int) std::lround (0.050 * rate);
            int previous = gainStepState (r, 0, 0, rate);

            for (const int position : startsE)
            {
                const int state = gainStepState (r, 0, position, rate);

                if (state == previous)
                    continue;

                previous = state;
                const int moves = r.set >= 0 ? ((position + refreshSamples - 1) / refreshSamples) * refreshSamples : position;
                const auto spanFrom = (size_t) juce::jmax (1, moves - lead);
                const auto spanTo = (size_t) juce::jmin (length, moves + n + lead);

                double kE = 0.0, kH = 0.0;

                for (int channel = 0; channel < 2; ++channel)
                {
                    kE = juce::jmax (kE, localKappa (widened (engine, channel), rate, spanFrom, spanTo));
                    kH = juce::jmax (kH, localKappa (widened (heldA, channel), rate, spanFrom, spanTo),
                                     localKappa (widened (heldB, channel), rate, spanFrom, spanTo));
                }

                const double ratio = ratioOf (kE, kH);

                if (! (ratio <= q.kRatio))
                    q.kRatio = ratio;
            }
        }

        bool hardOk = true;

        if (withHard)
        {
            const auto hard = gainRefRender (neutral, gainStepScript (r, sets, 0, startsE, rate), rate, true);
            hardOk = hard[0].size() == (size_t) length && hard[1].size() == (size_t) length;

            if (hardOk)
                q.dHard = juce::jmax (maxD2Of (hard[0], from), maxD2Of (hard[1], from));

            hardOk = hardOk && std::isfinite (q.dHard);
        }

        const double errA = errorReDb (heldA, gainRefRender (neutral, gainStepScript (r, sets, 1, startsA, rate), rate, false), 0, length);
        const double errB = errorReDb (heldB, gainRefRender (neutral, gainStepScript (r, sets, 2, startsB, rate), rate, false), 0, length);
        const double errE = errorReDb (engine, gainRefRender (neutral, gainStepScript (r, sets, 0, startsE, rate), rate, false), 0, length);

        q.refErrorDb = errA;

        for (const double err : { errB, errE })
            if (! (err <= q.refErrorDb))
                q.refErrorDb = err;

        q.refRenders = 3;
        q.valid = allFinite (heldA) && allFinite (heldB) && allFinite (engine) && allFinite (neutral) && hardOk
               && q.dHeld > 0.0 && q.kHeld > 0.0 && std::isfinite (q.dEng) && std::isfinite (q.kEng);
        return q;
    }

    // The reversal cases of RESEARCH-2.5 Q1 (output gain): a -> b at e1, then
    // -> c at e1 + 16 round (f N / 16), f = 25 / 50 / 75 / 90 %.
    struct RetCase { double a, b, c; };

    constexpr RetCase kRetCases[4] = { { 0.0, 36.0, -36.0 }, { 36.0, -36.0, 36.0 }, { 36.0, -36.0, 0.0 }, { 0.0, 12.0, 18.0 } };
    constexpr double kRetFractions[4] = { 0.25, 0.50, 0.75, 0.90 };
    constexpr double kRetLevels[5] = { -36.0, 0.0, 12.0, 18.0, 36.0 };
    constexpr int kRetLength = 41000;
    constexpr int kRetPhases = 8;

    int retLevelIndex (double db)
    {
        for (int i = 0; i < 5; ++i)
            if (kRetLevels[i] == db)
                return i;

        return 0;
    }

    void storeRetLevel (const Handles& h, double og)
    {
        h.gainScale->store (100.0f);
        h.bypass->store (0.0f);
        h.outputGain->store ((float) og);
        h.autoGain->store (0.0f);
        storeRaw (h.band[0], rawBandSpec (gainBellSpec()));

        for (size_t b = 1; b < h.band.size(); ++b)
            storeRaw (h.band[b], rawBandOff());
    }

    struct RetJob { int tone, caseIndex, fraction, blockSize, phase; };

    struct RetReading
    {
        bool valid = false;
        double ratio = 0.0;
        double refErrorDb = 0.0;
    };

    /** One reversal render. The host blocks are at most blockSize long and are
        cut at the two edit samples, so each edit is the start of a block (the
        edits sit on the 16-sample grid, as in the bench). */
    RetReading runRetJob (Processor& p, const Handles& h, const RetJob& job, const juce::AudioBuffer<float>& input,
                          const juce::AudioBuffer<float>& neutral, const std::array<juce::AudioBuffer<float>, 5>& held)
    {
        const double rate = kGainRate;
        const int fadeN = refPoleLength (kRefOutputSeconds, rate);
        const int e1 = 24000 + 16 * job.phase * 37;
        const int e2 = e1 + 16 * (int) std::lround (kRetFractions[job.fraction] * (double) fadeN / 16.0);
        const RetCase& c = kRetCases[job.caseIndex];
        const int blockSize = job.blockSize;

        std::vector<int> starts;
        storeRetLevel (h, c.a);

        const auto output = renderWith (p, input, rate, blockSize,
                                        [blockSize, e1, e2] (int, int position)
                                        {
                                            if (position >= e2)
                                                return blockSize;

                                            return juce::jmin (blockSize, (position < e1 ? e1 : e2) - position);
                                        },
                                        [&h, &starts, &c, e1, e2] (BlockInfo& current)
                                        {
                                            storeRetLevel (h, current.position >= e2 ? c.c : (current.position >= e1 ? c.b : c.a));
                                            starts.push_back (current.position);
                                        });

        const auto a = (size_t) (e1 - 2400);
        const auto b = (size_t) (e1 + 2 * fadeN + 2400);

        double heldD2 = 0.0;

        for (const double level : { c.a, c.b, c.c })
            heldD2 = juce::jmax (heldD2, maxD2Stereo (held[(size_t) retLevelIndex (level)], a, b));

        GainScript script;

        for (const int position : starts)
        {
            const double og = position >= e2 ? c.c : (position >= e1 ? c.b : c.a);
            script.add (position, refOutputTarget ((double) (float) og), false, 1.0);
        }

        RetReading q;
        q.ratio = ratioOf (maxD2Stereo (output, a, b), heldD2);
        q.refErrorDb = errorReDb (output, gainRefRender (neutral, script, rate, false), 0, output.getNumSamples());
        q.valid = allFinite (output) && heldD2 > 0.0;
        return q;
    }

    void gainStepRowsXr()
    {
        section ("XR-gstep / XR-gret / XR-ref. G3 step rows for the gains through the plugin (Phase 2.5: G12 as amended, G3; PLAN-2.5 P8, T7)");

        const auto designer = designerFor (kGainRate);
        std::vector<GainBandSet> sets;

        for (const double target : { -18.0, -6.0, 6.0, 12.0 })
        {
            sets.push_back (pickGainSet (target, *designer));
            std::printf ("      XR-gstep auto-gain band set %+5.0f dB: Bell 3 kHz +6 dB Q 1 + High Shelf 20 Hz 12 dB/oct Q 1 at %+.4f dB (raw); agRef estimate %+.4f dB\n",
                         target, (double) sets.back().shelfGain, sets.back().estimateDb);
        }

        const auto rows = gainStepRows (sets);
        const double tones[2] = { 50.0, 1000.0 };

        // [tone][0: output-gain length, 1: auto-gain length]
        const juce::AudioBuffer<float> inputs[2][2] = { { stepStimulus (tones[0], kGainRate, gainStepLength (false, kGainRate)),
                                                          stepStimulus (tones[0], kGainRate, gainStepLength (true, kGainRate)) },
                                                        { stepStimulus (tones[1], kGainRate, gainStepLength (false, kGainRate)),
                                                          stepStimulus (tones[1], kGainRate, gainStepLength (true, kGainRate)) } };

        struct Job { int tone; size_t rowIndex; int blockSize; };
        std::vector<Job> jobs;

        for (int t = 0; t < 2; ++t)
            for (size_t i = 0; i < rows.size(); ++i)
                for (const int blockSize : { 32, 512 })
                    jobs.push_back ({ t, i, blockSize });

        const int workers = workerCount ((int) jobs.size());
        std::vector<std::unique_ptr<Processor>> instances;
        std::vector<Handles> handles;

        for (int w = 0; w < workers; ++w)
        {
            instances.push_back (makeBands (2, 0, mixedSpec));
            handles.push_back (handlesOf (*instances.back()));
        }

        std::vector<GainStepReading> readings (jobs.size());

        parallelForWorkers ((int) jobs.size(), workers, [&jobs, &instances, &handles, &readings, &rows, &sets, &inputs] (int j, int w)
        {
            const Job& job = jobs[(size_t) j];
            const GainStepRow& r = rows[job.rowIndex];
            readings[(size_t) j] = runGainStepJob (*instances[(size_t) w], handles[(size_t) w], r, sets,
                                                   inputs[job.tone][r.set >= 0 ? 1 : 0], job.blockSize, job.blockSize == 512);
        });

        Tally step, kappa, hard, refTally;
        double worstD2[2][2] = { { 0.0, 0.0 }, { 0.0, 0.0 } };     // [output / auto][tone]
        juce::String worstD2At[2][2];
        double worstK[2] = { 0.0, 0.0 };                           // 1 kHz, [output / auto]
        double worstK300[2] = { 0.0, 0.0 };                        // the same, frames from 0.3 s (printed)
        juce::String worstK300At[2];
        double weakestHard[2][2] = { { std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() },
                                     { std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() } };
        double worstRef = -std::numeric_limits<double>::infinity();
        juce::String worstRefAt;

        for (size_t j = 0; j < jobs.size(); ++j)
        {
            const Job& job = jobs[j];
            const GainStepRow& r = rows[job.rowIndex];
            const GainStepReading& q = readings[j];
            const int kind = r.set >= 0 ? 1 : 0;
            const juce::String label = fmtG (tones[job.tone], 4) + " Hz, block " + juce::String (job.blockSize) + ", " + juce::String (r.name);

            const double dRatio = ratioOf (q.dEng, q.dHeld);
            const double kRatio = q.kRatio;
            const double kRatioFrom300 = ratioOf (q.kEng, q.kHeld);

            step.check (q.valid && dRatio <= 2.0, label + ": " + fmtG (dRatio, 4) + "x" + (q.valid ? "" : " INVALID"));

            if (! (dRatio <= worstD2[kind][job.tone]))
            {
                worstD2[kind][job.tone] = dRatio;
                worstD2At[kind][job.tone] = juce::String (r.name) + " b" + juce::String (job.blockSize);
            }

            if (job.tone == 1)
            {
                kappa.check (q.valid && kRatio <= 2.5, label + ": kappa " + fmtG (kRatio, 4) + "x");

                if (! (kRatio <= worstK[kind]))
                    worstK[kind] = kRatio;

                if (! (kRatioFrom300 <= worstK300[kind]))
                {
                    worstK300[kind] = kRatioFrom300;
                    worstK300At[kind] = label;
                }
            }

            if (job.blockSize == 512)
            {
                const double hRatio = ratioOf (q.dHard, q.dHeld);
                hard.check (q.valid && hRatio >= 10.0, label + ": hard " + fmtG (hRatio, 4) + "x");
                weakestHard[kind][job.tone] = juce::jmin (weakestHard[kind][job.tone], hRatio);
            }

            refTally.check (q.valid && q.refErrorDb <= -120.0, "XR-gstep " + label + ": " + fmtG (q.refErrorDb, 4) + " dB");

            if (! (q.refErrorDb <= worstRef))
            {
                worstRef = q.refErrorDb;
                worstRefAt = "XR-gstep " + label;
            }
        }

        const char* const kindNames[2] = { "output gain (20 pairs)", "auto gain toggles (8)" };

        for (int kind = 0; kind < 2; ++kind)
            for (int t = 0; t < 2; ++t)
                std::printf ("      XR-gstep %-23s %6.0f Hz | d2 step / louder held worst %6.3f (%s) | hard / held lowest at block 512 %10.1f\n",
                             kindNames[kind], tones[t], worstD2[kind][t], worstD2At[kind][t].toRawUTF8(), weakestHard[kind][t]);

        info ("XR-gstep 1 kHz local kappa with the frames on the 2.4 step-row grid (from 0.3 s, not anchored at the change; printed, not gated): worst output "
                  + fmtG (worstK300[0], 4) + "x (" + worstK300At[0] + "), auto " + fmtG (worstK300[1], 4) + "x (" + worstK300At[1] + ")");

        row (step.bad == 0 && step.total == 112 && kappa.bad == 0 && kappa.total == 56,
             "XR-gstep G3 step rows for the gains through processBlock (P11 stimulus, a Bell +6 dB running): 20 output-gain pairs and 8 auto_gain toggles (estimates -18 / -6 / +6 / +12 dB) x 50 Hz / 1 kHz x blocks 32 / 512: (a) largest second difference <= 2.0x the louder held render; (b) 1 kHz local kappa <= 2.5x (per change, frames from 50 ms before the stage starts moving, as gain_bench)",
             "(a) " + step.detail() + " worst output " + fmtG (juce::jmax (worstD2[0][0], worstD2[0][1]), 4) + "x, auto "
                 + fmtG (juce::jmax (worstD2[1][0], worstD2[1][1]), 4) + "x (bench P4 1.56x / 1.00x, AG 1.01x); (b) " + kappa.detail()
                 + " worst output " + fmtG (worstK[0], 4) + "x, auto " + fmtG (worstK[1], 4) + "x (bench 2.22x)");

        row (hard.bad == 0 && hard.total == 56,
             "XR-gsteph hardGainRender (the gain-neutral float output, the gain jumping at the sample the stage starts moving) at block 512 reads >= 10x the louder held render on every row (control fires)",
             hard.detail() + "  weakest output " + fmtG (juce::jmin (weakestHard[0][0], weakestHard[0][1]), 4) + "x, auto "
                 + fmtG (juce::jmin (weakestHard[1][0], weakestHard[1][1]), 4) + "x (bench 59x at 1 kHz, 23 000x at 50 Hz)");

        // XR-gret
        {
            const int retWorkers = workers;
            const juce::AudioBuffer<float> retInputs[2] = { stepStimulus (tones[0], kGainRate, kRetLength),
                                                            stepStimulus (tones[1], kGainRate, kRetLength) };

            // The held renders (five levels) and the gain-neutral render, per tone and block size.
            struct HeldJob { int tone, blockSize, level; };     // level -1: neutral
            std::vector<HeldJob> heldJobs;

            for (int t = 0; t < 2; ++t)
                for (const int blockSize : { 32, 512 })
                    for (int level = -1; level < 5; ++level)
                        heldJobs.push_back ({ t, blockSize, level });

            std::vector<juce::AudioBuffer<float>> heldOut (heldJobs.size());

            parallelForWorkers ((int) heldJobs.size(), retWorkers, [&heldJobs, &heldOut, &instances, &handles, &retInputs] (int j, int w)
            {
                const HeldJob& hj = heldJobs[(size_t) j];
                const Handles& hw = handles[(size_t) w];
                const double og = hj.level < 0 ? 0.0 : kRetLevels[hj.level];
                storeRetLevel (hw, og);
                heldOut[(size_t) j] = renderWith (*instances[(size_t) w], retInputs[hj.tone], kGainRate, hj.blockSize,
                                                  [&hj] (int, int) { return hj.blockSize; },
                                                  [&hw, og] (BlockInfo&) { storeRetLevel (hw, og); });
            });

            const auto heldIndex = [] (int tone, int blockSize, int level)
            {
                return (size_t) ((tone * 2 + (blockSize == 512 ? 1 : 0)) * 6 + level + 1);
            };

            std::array<std::array<std::array<juce::AudioBuffer<float>, 5>, 2>, 2> held;     // [tone][block][level]

            for (int t = 0; t < 2; ++t)
                for (int bi = 0; bi < 2; ++bi)
                    for (int level = 0; level < 5; ++level)
                        held[(size_t) t][(size_t) bi][(size_t) level] = heldOut[heldIndex (t, bi == 1 ? 512 : 32, level)];

            // The held renders against gainRefRender (a constant script).
            for (int t = 0; t < 2; ++t)
                for (int bi = 0; bi < 2; ++bi)
                    for (int level = 0; level < 5; ++level)
                    {
                        GainScript constant;
                        constant.add (0, refOutputTarget ((double) (float) kRetLevels[level]), false, 1.0);
                        const auto& neutral = heldOut[heldIndex (t, bi == 1 ? 512 : 32, -1)];
                        const auto& y = held[(size_t) t][(size_t) bi][(size_t) level];
                        const double err = errorReDb (y, gainRefRender (neutral, constant, kGainRate, false), 0, y.getNumSamples());
                        const juce::String label = "XR-gret held " + fmtG (kRetLevels[level], 3) + " dB, " + fmtG (tones[t], 4) + " Hz, block "
                                                 + juce::String (bi == 1 ? 512 : 32);

                        refTally.check (allFinite (y) && err <= -120.0, label + ": " + fmtG (err, 4) + " dB");

                        if (! (err <= worstRef))
                        {
                            worstRef = err;
                            worstRefAt = label;
                        }
                    }

            std::vector<RetJob> retJobs;

            for (int t = 0; t < 2; ++t)
                for (int c = 0; c < 4; ++c)
                    for (int f = 0; f < 4; ++f)
                        for (const int blockSize : { 32, 512 })
                            for (int ph = 0; ph < kRetPhases; ++ph)
                                retJobs.push_back ({ t, c, f, blockSize, ph });

            std::vector<RetReading> retReadings (retJobs.size());

            parallelForWorkers ((int) retJobs.size(), retWorkers,
                                [&retJobs, &retReadings, &instances, &handles, &retInputs, &heldOut, &held, &heldIndex] (int j, int w)
            {
                const RetJob& job = retJobs[(size_t) j];
                const auto& neutral = heldOut[heldIndex (job.tone, job.blockSize, -1)];
                retReadings[(size_t) j] = runRetJob (*instances[(size_t) w], handles[(size_t) w], job, retInputs[job.tone], neutral,
                                                     held[(size_t) job.tone][(size_t) (job.blockSize == 512 ? 1 : 0)]);
            });

            Tally ret;
            double worstRet[2] = { 0.0, 0.0 };
            juce::String worstRetAt[2];
            std::map<std::string, double> perCase;

            for (int t = 0; t < 2; ++t)
                for (int c = 0; c < 4; ++c)
                    for (int f = 0; f < 4; ++f)
                        for (const int blockSize : { 32, 512 })
                        {
                            double worst = 0.0;
                            bool valid = true;

                            for (size_t j = 0; j < retJobs.size(); ++j)
                            {
                                const RetJob& job = retJobs[j];

                                if (job.tone != t || job.caseIndex != c || job.fraction != f || job.blockSize != blockSize)
                                    continue;

                                const RetReading& q = retReadings[j];
                                valid = valid && q.valid;

                                if (! (q.ratio <= worst))
                                    worst = q.ratio;

                                const juce::String label = "XR-gret " + fmtG (tones[t], 4) + " Hz, block " + juce::String (blockSize) + ", "
                                                         + fmtG (kRetCases[c].a, 3) + " > " + fmtG (kRetCases[c].b, 3) + " > " + fmtG (kRetCases[c].c, 3)
                                                         + " at " + fmtG (100.0 * kRetFractions[f], 3) + " %, phase " + juce::String (job.phase);

                                refTally.check (q.valid && q.refErrorDb <= -120.0, label + ": " + fmtG (q.refErrorDb, 4) + " dB");

                                if (! (q.refErrorDb <= worstRef))
                                {
                                    worstRef = q.refErrorDb;
                                    worstRefAt = label;
                                }
                            }

                            const juce::String label = fmtG (tones[t], 4) + " Hz, block " + juce::String (blockSize) + ", "
                                                     + fmtG (kRetCases[c].a, 3) + " > " + fmtG (kRetCases[c].b, 3) + " > " + fmtG (kRetCases[c].c, 3)
                                                     + " at " + fmtG (100.0 * kRetFractions[f], 3) + " %";

                            ret.check (valid && worst <= 2.0, label + ": " + fmtG (worst, 4) + "x" + (valid ? "" : " INVALID"));

                            if (! (worst <= worstRet[t]))
                            {
                                worstRet[t] = worst;
                                worstRetAt[t] = label;
                            }

                            const std::string key = (fmtG (kRetCases[c].a, 3) + " > " + fmtG (kRetCases[c].b, 3) + " > "
                                                     + fmtG (kRetCases[c].c, 3) + ", " + fmtG (tones[t], 4) + " Hz").toStdString();
                            perCase[key] = juce::jmax (perCase[key], worst);
                        }

            for (const auto& entry : perCase)
                std::printf ("      XR-gret %-28s worst over 4 fractions x 2 block sizes x %d phases: %.3fx\n",
                             entry.first.c_str(), kRetPhases, entry.second);

            row (ret.bad == 0 && ret.total == 64,
                 "XR-gret output-gain reversals through processBlock (0 > +36 > -36, +36 > -36 > +36, +36 > -36 > 0, 0 > +12 > +18; second change at 25 / 50 / 75 / 90 % of the 20 ms; 8 phases) x 50 Hz / 1 kHz x blocks 32 / 512 (blocks cut at the edit samples): largest second difference <= 2.0x the louder of the three held renders",
                 ret.detail() + "  worst 50 Hz " + fmtG (worstRet[0], 4) + "x (" + worstRetAt[0] + "), 1 kHz " + fmtG (worstRet[1], 4)
                     + "x (bench P4 1.58x)");
        }

        row (refTally.bad == 0 && refTally.total == 112 + 20 + 512,
             "XR-ref every XR-gstep render (112 jobs x held A, held B, step) and every XR-gret render (20 held, 512 reversals) against gainRefRender (the four-pole recurrence, tick / refresh schedule and seeding re-stated per sample) x the gain-neutral float output: <= -120 dB re the output peak",
             refTally.detail() + "  worst " + fmtG (worstRef, 4) + " dB (" + worstRefAt + ")");
    }

    //==========================================================================
    // XR-gsweep. G3 sweep form for the gains (RESEARCH-2.5 Q1 "Automation")
    //==========================================================================
    // The sweep of gain_bench sweep: low until s1, to high over 20 ms, held
    // 100 ms, back to low over 20 ms.
    constexpr int kSweepStart = 24000, kSweepLength = 960, kSweepHold = 4800;
    constexpr int kSweepBack = kSweepStart + kSweepLength + kSweepHold;
    constexpr int kSweepEnd = kSweepBack + kSweepLength;
    constexpr int kSweepTotal = kSweepEnd + 19200;

    double sweepDb (int n, double low, double high)
    {
        if (n < kSweepStart)
            return low;

        if (n < kSweepStart + kSweepLength)
            return low + (high - low) * (double) (n - kSweepStart) / (double) kSweepLength;

        if (n < kSweepBack)
            return high;

        if (n < kSweepEnd)
            return high - (high - low) * (double) (n - kSweepBack) / (double) kSweepLength;

        return low;
    }

    /** kind 0: output gain -30 -> +30 -> -30 dB, the Bell 3 kHz +6 dB running;
        kind 1: a Bell 1 kHz Q 1 swept -12 -> +12 -> -12 dB with auto gain on.
        neutral: output 0 dB, auto gain off, the same band automation. */
    void storeSweep (const Handles& h, int kind, int position, bool neutral)
    {
        h.gainScale->store (100.0f);
        h.bypass->store (0.0f);

        if (kind == 0)
        {
            h.outputGain->store (neutral ? 0.0f : (float) sweepDb (position, -30.0, 30.0));
            h.autoGain->store (0.0f);
            storeRaw (h.band[0], rawBandSpec (gainBellSpec()));
        }
        else
        {
            h.outputGain->store (0.0f);
            h.autoGain->store (neutral ? 0.0f : 1.0f);
            storeRaw (h.band[0], rawBandSpec (makeSpec (kBell, 1000.0f, (float) sweepDb (position, -12.0, 12.0), 1.0f, 1, kStereo)));
        }

        for (size_t b = 1; b < h.band.size(); ++b)
            storeRaw (h.band[b], rawBandOff());
    }

    struct SweepGainReading
    {
        bool valid = false;
        double ratio = 0.0;
        double errorDb = 0.0;       // kind 1: the plugin against its reference (printed)
    };

    SweepGainReading runSweepGainJob (Processor& p, const Handles& h, int kind, const juce::AudioBuffer<float>& input, int blockSize)
    {
        const double rate = kGainRate;
        std::vector<int> starts;

        storeSweep (h, kind, 0, false);
        const auto output = renderWith (p, input, rate, blockSize, [blockSize] (int, int) { return blockSize; },
                                        [&h, &starts, kind] (BlockInfo& current)
                                        {
                                            storeSweep (h, kind, current.position, false);
                                            starts.push_back (current.position);
                                        });

        storeSweep (h, kind, 0, true);
        const auto neutral = renderWith (p, input, rate, blockSize, [blockSize] (int, int) { return blockSize; },
                                         [&h, kind] (BlockInfo& current) { storeSweep (h, kind, current.position, true); });

        std::array<std::vector<double>, 2> reference;

        if (kind == 0)
        {
            // The per-sample automation (gain_bench sweep's reference).
            for (int channel = 0; channel < 2; ++channel)
            {
                auto y = widened (neutral, channel);

                for (size_t i = 0; i < y.size(); ++i)
                    y[i] *= std::pow (10.0, sweepDb ((int) i, -30.0, 30.0) / 20.0);

                reference[(size_t) channel] = std::move (y);
            }
        }
        else
        {
            // The same render with the auto-gain trajectory replaced by
            // gainRefRender on agRef targets: the band's latest request at
            // each block start, refreshed on the same ticks.
            const auto designer = designerFor (rate);
            GainScript script;

            for (const int position : starts)
            {
                const auto raw = rawBandSpec (makeSpec (kBell, 1000.0f, (float) sweepDb (position, -12.0, 12.0), 1.0f, 1, kStereo));
                script.add (position, 1.0, true, agRef ({ rawInput (raw, 100.0f, true) }, true, rate, *designer).linear);
            }

            reference = gainRefRender (neutral, script, rate, false);
        }

        const auto from = (size_t) (kSweepStart - 2400);
        const auto to = (size_t) (kSweepEnd + 9600);

        SweepGainReading q;
        q.ratio = ratioOf (maxD2Stereo (output, from, to), maxD2Stereo (reference, from, to));
        q.errorDb = errorReDb (output, reference, 0, output.getNumSamples());
        q.valid = allFinite (output) && allFinite (neutral) && reference[0].size() == (size_t) output.getNumSamples();
        return q;
    }

    void gainSweepRowsXr()
    {
        section ("XR-gsweep. G3 sweep form for the gains through the plugin (Phase 2.5: G12 as amended; RESEARCH-2.5 Q1)");

        const double tones[2] = { 50.0, 1000.0 };
        const juce::AudioBuffer<float> inputs[2] = { stepStimulus (tones[0], kGainRate, kSweepTotal),
                                                     stepStimulus (tones[1], kGainRate, kSweepTotal) };

        struct Job { int kind, tone, blockSize; };
        std::vector<Job> jobs;

        for (int kind = 0; kind < 2; ++kind)
            for (int t = 0; t < 2; ++t)
                for (const int blockSize : { 32, 512 })
                    jobs.push_back ({ kind, t, blockSize });

        std::vector<std::unique_ptr<Processor>> instances;
        std::vector<Handles> handles;

        for (size_t j = 0; j < jobs.size(); ++j)
        {
            instances.push_back (makeBands (2, 0, mixedSpec));
            handles.push_back (handlesOf (*instances.back()));
        }

        std::vector<SweepGainReading> readings (jobs.size());

        parallelFor ((int) jobs.size(), [&jobs, &instances, &handles, &readings, &inputs] (int j)
        {
            const auto slot = (size_t) j;
            readings[slot] = runSweepGainJob (*instances[slot], handles[slot], jobs[slot].kind, inputs[jobs[slot].tone], jobs[slot].blockSize);
        });

        Tally tally;
        juce::String figures[2];
        double worst[2] = { 0.0, 0.0 }, worstError = -std::numeric_limits<double>::infinity();

        for (size_t j = 0; j < jobs.size(); ++j)
        {
            const Job& job = jobs[j];
            const SweepGainReading& q = readings[j];
            const juce::String label = juce::String (job.kind == 0 ? "output gain" : "auto gain") + ", " + fmtG (tones[job.tone], 4)
                                     + " Hz, block " + juce::String (job.blockSize);

            tally.check (q.valid && q.ratio <= 4.0, label + ": " + fmtG (q.ratio, 4) + "x" + (q.valid ? "" : " INVALID"));
            worst[job.kind] = juce::jmax (worst[job.kind], q.ratio);
            figures[job.kind] += (figures[job.kind].isEmpty() ? "" : ", ") + fmtG (tones[job.tone], 4) + " Hz / " + juce::String (job.blockSize)
                               + " " + fmtG (q.ratio, 3) + "x";

            if (job.kind == 1 && ! (q.errorDb <= worstError))
                worstError = q.errorDb;
        }

        row (tally.bad == 0 && tally.total == 8,
             "XR-gsweep output gain -30 -> +30 -> -30 dB in 20 ms each way (automation at block starts) against the per-sample automation of the gain-neutral output; auto gain on while a Bell 1 kHz Q 1 sweeps -12 -> +12 -> -12 dB in 20 ms against the same render with the auto-gain trajectory from gainRefRender on agRef targets; 50 Hz / 1 kHz, blocks 32 / 512: largest second difference <= 4.0x",
             tally.detail() + "  output gain: " + figures[0] + " (bench 0.03x); auto gain: " + figures[1]
                 + " (no bench figure); auto-gain render against its reference worst " + fmtG (worstError, 4) + " dB re peak (printed)");
    }

    //==========================================================================
    // XR-g4a, XR-g4an, XR-idle. G4 (a) with the stage (QUAL-01; PLAN-2.5 R1)
    //==========================================================================
    void stageTransparencyRowsXr()
    {
        section ("XR-g4a / XR-idle. G4 (a) with the output stage on the float output (Phase 2.5: QUAL-01; PLAN-2.5 P6, R1)");

        const int patterns[7] = { 1, 16, 64, 512, 1024, 4096, 0 };

        // XR-g4a
        {
            Tally tally;

            for (const int channels : { 1, 2 })
            {
                const auto input = makeNoiseTail (channels, 0x64A50000 + channels);
                tally.check (hasDenormalTail (input), "stimulus: the tail is not denormal non-zero, or this thread flushes to zero");

                for (const bool autoOn : { false, true })
                    for (const int pattern : patterns)
                    {
                        auto p = make();
                        const bool layoutOk = setLayout (*p, channels);
                        const Handles h = handlesOf (*p);
                        h.outputGain->store (0.0f);
                        h.autoGain->store (autoOn ? 1.0f : 0.0f);

                        const auto output = renderPattern (*p, input, pattern);

                        tally.check (layoutOk && enabledBands (*p) == 0 && sameAudio (output, input),
                                     juce::String (channels == 1 ? "mono" : "stereo") + ", auto_gain " + (autoOn ? "on" : "off") + ", "
                                         + patternName (pattern) + " differs from the input");
                    }
            }

            row (tally.bad == 0 && tally.total == 30,
                 "XR-g4a G4 (a) with the stage: no band, output gain 0 dB, auto_gain off and on, noise + denormal tail: out memcmp-equal to in (blocks 1 / 16 / 64 / 512 / 1024 / 4096 / random, mono and stereo)",
                 tally.detail());
        }

        // XR-g4an
        {
            Tally tally;

            for (const int channels : { 1, 2 })
            {
                const auto input = makeNoiseTail (channels, 0x64A60000 + channels);

                for (const int pattern : patterns)
                {
                    auto p = make();
                    setLayout (*p, channels);
                    handlesOf (*p).outputGain->store (0.001f);

                    const auto output = renderPattern (*p, input, pattern);
                    tally.check (! sameAudio (output, input), juce::String (channels == 1 ? "mono" : "stereo") + ", output +0.001 dB, "
                                                                  + patternName (pattern) + ": still memcmp-equal");
                }
            }

            {
                const auto input = makeNoiseTail (2, 0x64A70002);

                for (const int pattern : patterns)
                {
                    auto p = makeBands (2, 1, [] (int) { return makeSpec (kBell, 1000.0f, 1.0f, 1.0f, 1, kLeft); });
                    handlesOf (*p).autoGain->store (1.0f);

                    const auto output = renderPattern (*p, input, pattern);

                    // The right lane: no band touches it, the auto gain does.
                    tally.check (! sameChannel (output, input, 1), "auto_gain on, one Left +1 dB bell, " + patternName (pattern)
                                                                       + ": the right channel is still memcmp-equal");
                }
            }

            row (tally.bad == 0 && tally.total == 21,
                 "XR-g4an output gain +0.001 dB (mono, stereo), and auto_gain on with one Left-only +1 dB bell (the right channel): each differs from the input (control fires; 7 block patterns)",
                 tally.detail());
        }

        // XR-idle: +6 dB at the first block start at or after sample 2048,
        // back to 0 dB at the first at or after 8192.
        {
            Tally tally;
            juce::String figures;
            const int after60 = (int) std::lround (0.060 * kRenderRate);

            for (const int channels : { 1, 2 })
            {
                const auto input = makeNoiseTail (channels, 0x1D1E0000 + channels);

                for (const int pattern : patterns)
                {
                    auto p = make();
                    setLayout (*p, channels);
                    const Handles h = handlesOf (*p);
                    int up = -1, down = -1;

                    const BlockHook hook = [&h, &up, &down] (BlockInfo& current)
                    {
                        if (up < 0 && current.position >= 2048)
                        {
                            up = current.position;
                            h.outputGain->store (6.0f);
                        }

                        if (down < 0 && current.position >= 8192)
                        {
                            down = current.position;
                            h.outputGain->store (0.0f);
                        }
                    };

                    const auto output = pattern > 0 ? render (*p, input, pattern, false, kRenderRate, hook)
                                                    : render (*p, input, kSequenceMax, false, kRenderRate, hook,
                                                              randomBlockSequence (0x5E0BE9, kSequenceMax));

                    const bool acted = up >= 0 && down > up && ! sameFrom (output, up, input, up, down - up);
                    const bool before = up > 0 && sameFrom (output, 0, input, 0, up);
                    const bool equal = down >= 0 && equalFrom (output, input, down + after60);
                    const int last = lastDifference (output, input);

                    tally.check (acted && before && equal && hasDenormalTail (input),
                                 juce::String (channels == 1 ? "mono " : "stereo ") + patternName (pattern)
                                     + (acted ? "" : ": the +6 dB did not act") + (before ? "" : ": differs before the edit")
                                     + (equal ? "" : ": differs 60 ms after the return to 0 dB"));

                    if (channels == 2)
                        figures += (figures.isEmpty() ? "" : ", ") + patternName (pattern) + " "
                                 + fmtG (1000.0 * (double) (last + 1 - down) / kRenderRate, 4) + " ms";
                }
            }

            row (tally.bad == 0 && tally.total == 14,
                 "XR-idle no band, output gain 0 -> +6 -> 0 dB: memcmp-equal to the input from 60 ms after the second edit, denormal tail included (7 block patterns, mono and stereo)",
                 tally.detail() + "  stereo equal from " + figures + " after the second edit (bench snap 45 - 52 ms)");
        }
    }

    //==========================================================================
    // XR-seed, XR-seedn, XR-gbyp, XR-unb, XR-unbn. Seeding and bypass with
    // the stage (PLAN-2.5 P6, R3; VERIFICATION-2.4 Issue 2)
    //==========================================================================
    /** render() with host blocks of at most blockSize cut at each sample of
        `cuts` (each then starts a block). Main thread. */
    juce::AudioBuffer<float> renderCut (juce::AudioProcessor& p, const juce::AudioBuffer<float>& input, int blockSize,
                                        const std::vector<int>& cuts, const BlockHook& hook)
    {
        const int numChannels = input.getNumChannels();
        const int length = input.getNumSamples();

        juce::AudioBuffer<float> output (numChannels, length);
        output.clear();
        juce::AudioBuffer<float> block (numChannels, blockSize);
        juce::MidiBuffer midi;

        p.prepareToPlay (kRenderRate, blockSize);

        int index = 0;

        for (int position = 0; position < length; ++index)
        {
            int n = juce::jmin (blockSize, length - position);

            for (const int cut : cuts)
                if (cut > position && cut - position < n)
                    n = cut - position;

            BlockInfo current;
            current.index = index;
            current.position = position;
            current.viaBypassed = false;

            if (hook)
                hook (current);

            block.setSize (numChannels, n, false, false, true);

            for (int channel = 0; channel < numChannels; ++channel)
                block.copyFrom (channel, 0, input, channel, position, n);

            if (current.viaBypassed)
                p.processBlockBypassed (block, midi);
            else
                p.processBlock (block, midi);

            for (int channel = 0; channel < numChannels; ++channel)
                output.copyFrom (channel, position, block, channel, 0, n);

            position += n;
        }

        p.releaseResources();
        return output;
    }

    void stageSeedRowsXr()
    {
        section ("XR-seed / XR-gbyp / XR-unb. seeding, bypass with the gain, the latched un-bypass (Phase 2.5: PLAN-2.5 P6, R3; VERIFICATION-2.4 Issue 2)");

        const int fadeN = refFadeLength (kRenderRate);

        // XR-seed (i): the instance opens at +12 dB output, auto gain on, one
        // +12 dB Low Shelf; from the first sample the output is the render at
        // 0 dB / auto off times the constant 10^(12/20) agRef.
        bool seedOk = true;
        juce::String seedFigures;
        {
            const auto input = makeNoiseAt (2, 24000, 0x5EED0002, kNoise12);
            const BandSpec shelf = makeSpec (kLowShelf, 200.0f, 12.0f, 1.0f, 1, kStereo);
            const auto designer = designerFor (kRenderRate);
            const AgRef estimate = agRef ({ rawInput (rawBandSpec (shelf), 100.0f, true) }, true, kRenderRate, *designer);
            const double g = std::pow (10.0, 12.0 / 20.0) * estimate.linear;

            for (const int blockSize : { 64, 512 })
            {
                auto a = makeBands (2, 1, [&shelf] (int) { return shelf; });
                const Handles ha = handlesOf (*a);
                ha.outputGain->store (12.0f);
                ha.autoGain->store (1.0f);

                auto b = makeBands (2, 1, [&shelf] (int) { return shelf; });

                const auto outA = render (*a, input, blockSize, false);
                const auto outB = render (*b, input, blockSize, false);

                std::array<std::vector<double>, 2> reference { widened (outB, 0), widened (outB, 1) };

                for (auto& lane : reference)
                    for (auto& v : lane)
                        v *= g;

                const double err = errorReDb (outA, reference, 0, outA.getNumSamples());
                const bool ok = err <= -120.0 && ! sameAudio (outA, outB) && allFinite (outA);
                seedOk = seedOk && ok;
                seedFigures += (seedFigures.isEmpty() ? "" : ", ") + juce::String ("block ") + juce::String (blockSize) + " " + fmtG (err, 4) + " dB";
            }

            seedFigures += " (constant " + fmtG (20.0 * std::log10 (g), 6) + " dB: +12 and agRef " + fmtG (estimate.db, 6) + ")";
        }

        // XR-seed (ii), XR-seedn: one band; bypass on; after the fade the
        // output gain, auto gain and the band are changed; un-bypass by the
        // parameter and by processBlockBypassed.
        Tally seedTally, seedControl;
        {
            constexpr int blockSize = 512, bypassBlock = 10, ogBlock = 14, agBlock = 15, bandBlock = 16, resumeBlock = 20;
            const auto input = makeNoiseAt (2, 36 * blockSize, 0x5EED0012, kNoise12);
            const int total = input.getNumSamples();
            const int bypassAt = bypassBlock * blockSize;
            const int resumeAt = resumeBlock * blockSize;
            const BandSpec before = makeSpec (kBell, 1000.0f, 6.0f, 1.0f, 1, kStereo);
            const BandSpec after = makeSpec (kHighShelf, 4000.0f, 8.0f, 1.0f, 1, kStereo);

            for (const bool viaEntry : { false, true })
            {
                // Every band value is a raw store (exact), here and in the
                // fresh instances: a normalised write rounds the raw value.
                auto p = makeBands (2, 1, mixedSpec);
                const Handles h = handlesOf (*p);
                storeRaw (h.band[0], rawBandSpec (before));

                const auto output = render (*p, input, blockSize, false, kRenderRate, [&h, &after, viaEntry] (BlockInfo& current)
                {
                    if (current.index == bypassBlock && ! viaEntry)
                        h.bypass->store (1.0f);

                    if (current.index == ogBlock)
                        h.outputGain->store (9.0f);

                    if (current.index == agBlock)
                        h.autoGain->store (1.0f);

                    if (current.index == bandBlock)
                        storeRaw (h.band[0], rawBandSpec (after));

                    if (current.index == resumeBlock && ! viaEntry)
                        h.bypass->store (0.0f);

                    if (viaEntry)
                        current.viaBypassed = current.index >= bypassBlock && current.index < resumeBlock;
                });

                auto edited = makeBands (2, 1, mixedSpec);
                const Handles he = handlesOf (*edited);
                storeRaw (he.band[0], rawBandSpec (after));
                he.outputGain->store (9.0f);
                he.autoGain->store (1.0f);
                const auto editedOut = render (*edited, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

                auto stale = makeBands (2, 1, mixedSpec);
                storeRaw (handlesOf (*stale).band[0], rawBandSpec (before));
                const auto staleOut = render (*stale, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

                const bool bypassed = sameFrom (output, bypassAt + fadeN, input, bypassAt + fadeN, resumeAt - bypassAt - fadeN);
                const bool resumed = sameFrom (output, resumeAt + fadeN, editedOut, fadeN, total - resumeAt - fadeN);
                const bool asBefore = sameFrom (output, resumeAt + fadeN, staleOut, fadeN, total - resumeAt - fadeN);
                const juce::String route = viaEntry ? "processBlockBypassed" : "bypass parameter";

                seedTally.check (bypassed && resumed,
                                 route + (bypassed ? "" : ": the bypassed span is not the input")
                                     + (resumed ? "" : ": from un-bypass + N differs from a fresh instance on the edited parameters (first at "
                                                       + juce::String (firstDifference (sliceOf (output, resumeAt + fadeN, total - resumeAt - fadeN),
                                                                                        sliceOf (editedOut, fadeN, total - resumeAt - fadeN)))
                                                       + " after un-bypass + N)"));

                seedControl.check (! asBefore, route + ": equals a fresh instance on the parameters from before the bypass");
            }
        }

        row (seedOk && seedTally.bad == 0 && seedTally.total == 2,
             "XR-seed (i) opened at output +12 dB, auto_gain on, one +12 dB Low Shelf 200 Hz: from the first sample the output is the 0 dB / auto-off render x 10^(12/20) agRef, <= -120 dB re the peak (blocks 64 / 512); (ii) one band, edits while bypassed (output gain, auto_gain, the band), un-bypass by both routes: from un-bypass + N memcmp-equal to a fresh instance on the edited parameters",
             "(i) " + seedFigures + "; (ii) " + seedTally.detail() + "  N = " + juce::String (fadeN));

        row (seedControl.bad == 0 && seedControl.total == 2,
             "XR-seedn (ii) against a fresh instance on the parameters from before the bypass differs (control fires)",
             seedControl.detail());

        // XR-gbyp: no band, output +12 dB; bypass on at the first block at or
        // after 9600, off at the first at or after 24000.
        {
            const auto input = makeNoiseAt (2, kG4Length, 0x6B9B0002, kNoise12);
            const int total = input.getNumSamples();
            const double g = std::pow (10.0, 12.0 / 20.0);

            Tally tally;
            double worstMix = -std::numeric_limits<double>::infinity();

            for (const bool viaEntry : { false, true })
                for (const int blockSize : { 64, 512 })
                {
                    auto p = make();
                    setLayout (*p, 2);
                    const Handles h = handlesOf (*p);
                    h.outputGain->store (12.0f);

                    int on = -1, off = -1;

                    const auto output = render (*p, input, blockSize, false, kRenderRate, [&h, &on, &off, viaEntry] (BlockInfo& current)
                    {
                        if (on < 0 && current.position >= 9600)
                        {
                            on = current.position;

                            if (! viaEntry)
                                h.bypass->store (1.0f);
                        }

                        if (off < 0 && current.position >= 24000)
                        {
                            off = current.position;

                            if (! viaEntry)
                                h.bypass->store (0.0f);
                        }

                        if (viaEntry)
                            current.viaBypassed = on >= 0 && off < 0;
                    });

                    // The harness mix with the re-stated weights: proc = x g
                    // (in double), toward bypass proc + w (x - proc), back
                    // x + w (proc - x).
                    std::array<std::vector<double>, 2> reference;
                    bool processed = true, bypassed = true, edges = true;

                    for (int channel = 0; channel < 2; ++channel)
                    {
                        const float* x = input.getReadPointer (channel);
                        const float* y = output.getReadPointer (channel);
                        auto& r = reference[(size_t) channel];
                        r.assign ((size_t) total, 0.0);

                        for (int i = 0; i < total; ++i)
                        {
                            const double dry = (double) x[i];
                            const double proc = dry * g;
                            const auto procFloat = (float) proc;

                            if (i < on || i >= off + fadeN)
                            {
                                r[(size_t) i] = proc;
                                processed = processed && sameBits (y[i], procFloat);
                            }
                            else if (i < on + fadeN)
                            {
                                r[(size_t) i] = proc + refFadeWeight (i - on, fadeN) * (dry - proc);
                            }
                            else if (i < off)
                            {
                                r[(size_t) i] = dry;
                                bypassed = bypassed && sameBits (y[i], x[i]);
                            }
                            else
                            {
                                r[(size_t) i] = dry + refFadeWeight (i - off, fadeN) * (proc - dry);
                            }
                        }
                    }

                    // Exactly N each way: the last mixed sample is neither end.
                    edges = on >= 0 && off > on + fadeN
                         && (! sameBits (output.getSample (0, on + fadeN - 1), input.getSample (0, on + fadeN - 1))
                             || ! sameBits (output.getSample (1, on + fadeN - 1), input.getSample (1, on + fadeN - 1)))
                         && (! sameBits (output.getSample (0, off + fadeN - 1), (float) ((double) input.getSample (0, off + fadeN - 1) * g))
                             || ! sameBits (output.getSample (1, off + fadeN - 1), (float) ((double) input.getSample (1, off + fadeN - 1) * g)));

                    const double errOn = errorReDb (output, reference, on, on + fadeN);
                    const double errOff = errorReDb (output, reference, off, off + fadeN);
                    const double err = juce::jmax (errOn, errOff);
                    worstMix = juce::jmax (worstMix, err);

                    tally.check (processed && bypassed && edges && err <= -120.0,
                                 juce::String (viaEntry ? "processBlockBypassed" : "parameter") + ", block " + juce::String (blockSize)
                                     + (processed ? "" : ": the processed spans are not x g") + (bypassed ? "" : ": the bypassed span is not the input")
                                     + (edges ? "" : ": the mixing is not exactly N samples")
                                     + (err <= -120.0 ? "" : ": the mix reads " + fmtG (err, 4) + " dB"));
                }

            row (tally.bad == 0 && tally.total == 4,
                 "XR-gbyp no band, output +12 dB: bypass on and off by the parameter and by processBlockBypassed (blocks 64 / 512): the toggle fades, exactly N samples of mixing each way equal to the harness mix of x g and x with the re-stated weights (<= -120 dB re peak); memcmp-equal to the input while bypassed and to x g outside",
                 tally.detail() + "  worst mix " + fmtG (worstMix, 4) + " dB; N = " + juce::String (fadeN));
        }

        // XR-unb, XR-unbn (VERIFICATION-2.4 Issue 2; P13: one band). Output
        // +6 dB and auto gain on throughout, so the stage is seeded by the
        // step-7 restart too (PLAN-2.5 R3). Bypass on at e; a type change at
        // e + 64 (starts the band's own fade) and a gain change at e + 128
        // (latched behind it); un-bypass at e + 240 (5 ms into the fade,
        // latched behind it). Host blocks are cut at the event samples.
        {
            constexpr int e = 10240, eType = e + 64, eGain = e + 128, eUn = e + 240;
            constexpr int length = 24000;
            const auto input = makeNoiseAt (2, length, 0x0B0B0002, kNoise12);
            const std::vector<int> cuts { e, eType, eGain, eUn };

            const BandSpec first = makeSpec (kBell, 1000.0f, 9.0f, 1.0f, 1, kStereo);
            const BandSpec typed = makeSpec (kHighShelf, 2000.0f, 9.0f, 1.0f, 1, kStereo);
            const BandSpec last = makeSpec (kHighShelf, 2000.0f, -6.0f, 1.0f, 1, kStereo);

            const auto withStage = [] (const BandSpec& spec)
            {
                auto p = makeBands (2, 1, mixedSpec);
                const Handles h = handlesOf (*p);
                storeRaw (h.band[0], rawBandSpec (spec));      // exact raw values, as the edits are
                h.outputGain->store (6.0f);
                h.autoGain->store (1.0f);
                return p;
            };

            Tally same, mix, controlSame, controlMix;
            double worstMix = -std::numeric_limits<double>::infinity(), weakestLate = std::numeric_limits<double>::infinity();

            for (const int blockSize : { 32, 512 })
                for (const bool viaEntry : { false, true })
                {
                    const juce::String where = juce::String (viaEntry ? "processBlockBypassed" : "parameter") + ", block " + juce::String (blockSize);

                    auto u = withStage (first);
                    const Handles hu = handlesOf (*u);

                    const auto output = renderCut (*u, input, blockSize, cuts, [&hu, &typed, &last, viaEntry] (BlockInfo& current)
                    {
                        if (current.position == e && ! viaEntry)
                            hu.bypass->store (1.0f);

                        if (current.position == eType)
                            storeRaw (hu.band[0], rawBandSpec (typed));

                        if (current.position == eGain)
                            storeRaw (hu.band[0], rawBandSpec (last));

                        if (current.position == eUn && ! viaEntry)
                            hu.bypass->store (0.0f);

                        if (viaEntry)
                            current.viaBypassed = current.position >= e && current.position < eUn;
                    });

                    // The held processed render: the same edits, no bypass.
                    auto pp = withStage (first);
                    const Handles hp = handlesOf (*pp);

                    const auto processed = renderCut (*pp, input, blockSize, cuts, [&hp, &typed, &last] (BlockInfo& current)
                    {
                        if (current.position == eType)
                            storeRaw (hp.band[0], rawBandSpec (typed));

                        if (current.position == eGain)
                            storeRaw (hp.band[0], rawBandSpec (last));
                    });

                    // Fresh instances started at e + N: on the edited parameters, and (control) on the first ones.
                    const auto tail = sliceOf (input, e + fadeN, length - e - fadeN);
                    auto q = withStage (last);
                    const auto fresh = renderCut (*q, tail, blockSize, {}, {});
                    auto q0 = withStage (first);
                    const auto freshOld = renderCut (*q0, tail, blockSize, {}, {});

                    // (i) PX14
                    const int span = length - e - 2 * fadeN;
                    const bool equal = sameFrom (output, e + 2 * fadeN, fresh, fadeN, span);
                    same.check (equal, where + ": from e + 2N differs from a fresh instance on the edited parameters started at e + N");
                    controlSame.check (! sameFrom (output, e + 2 * fadeN, freshOld, fadeN, span),
                                       where + ": equals a fresh instance on the pre-edit parameters");

                    // (ii) PX9: the harness mix over [0, e + 2N].
                    const auto mixReference = [&] (int late)
                    {
                        std::array<std::vector<double>, 2> r;

                        for (int channel = 0; channel < 2; ++channel)
                        {
                            const float* x = input.getReadPointer (channel);
                            const float* pr = processed.getReadPointer (channel);
                            const float* fr = fresh.getReadPointer (channel);
                            auto& lane = r[(size_t) channel];
                            lane.assign ((size_t) length, 0.0);

                            for (int i = 0; i <= e + 2 * fadeN; ++i)
                            {
                                const double dry = (double) x[i];

                                if (i < e)
                                {
                                    lane[(size_t) i] = (double) pr[i];
                                }
                                else if (i < e + fadeN)
                                {
                                    const double proc = (double) pr[i];
                                    lane[(size_t) i] = proc + refFadeWeight (i - e, fadeN) * (dry - proc);
                                }
                                else
                                {
                                    const int k = i - e - fadeN - late;
                                    const double inc = (double) fr[i - e - fadeN];

                                    if (k < 0)
                                        lane[(size_t) i] = dry;
                                    else if (k < fadeN)
                                        lane[(size_t) i] = dry + refFadeWeight (k, fadeN) * (inc - dry);
                                    else
                                        lane[(size_t) i] = inc;
                                }
                            }
                        }

                        return r;
                    };

                    const double err = errorReDb (output, mixReference (0), 0, e + 2 * fadeN + 1);
                    const double errLate = errorReDb (output, mixReference (1), 0, e + 2 * fadeN + 1);
                    worstMix = juce::jmax (worstMix, err);
                    weakestLate = juce::jmin (weakestLate, errLate);

                    mix.check (err <= -120.0 && allFinite (output), where + ": the mix reads " + fmtG (err, 4) + " dB");
                    controlMix.check (errLate > -120.0, where + ": one sample late reads " + fmtG (errLate, 4) + " dB");
                }

            row (same.bad == 0 && same.total == 4 && mix.bad == 0 && mix.total == 4,
                 "XR-unb VERIFICATION-2.4 Issue 2, one band, output +6 dB and auto_gain on: bypass on, a type edit and a gain edit during the fade, un-bypass 5 ms into it; parameter and processBlockBypassed, blocks 32 / 512: (i) from e + 2N memcmp-equal to a fresh instance on the edited parameters started at e + N (PX14); (ii) over [start, e + 2N] the harness mix of the held processed render and the dry input, the second fade from e + N exactly, <= -120 dB re peak (PX9)",
                 "(i) " + same.detail() + "; (ii) " + mix.detail() + " worst " + fmtG (worstMix, 4) + " dB; N = " + juce::String (fadeN));

            row (controlSame.bad == 0 && controlSame.total == 4 && controlMix.bad == 0 && controlMix.total == 4,
                 "XR-unbn (i) against a fresh instance on the pre-edit parameters, (ii) with the second fade starting one sample late: each differs (control fires)",
                 "(i) " + controlSame.detail() + "; (ii) " + controlMix.detail() + " closest " + fmtG (weakestLate, 4) + " dB");
        }
    }

    //==========================================================================
    // XR-blk5. Block identity with gain events (PLAN-2.5 P4 invariant, R2)
    //==========================================================================
    struct Blk5Event
    {
        BlkBand bands[4];
        float outputDb = 0.0f;
        float autoOn = 1.0f;
    };

    /** 160 events: per band as XR-blk (type and slope p 0.5, placement 0.3,
        enable 0.3, frequency, gain exactly 0 with p 0.25, Q); output_gain
        redrawn over -36 ... +36 dB with p 0.3; auto_gain toggled with p 0.1
        (on at the start). */
    std::vector<Blk5Event> makeGainEvents()
    {
        Xorshift draw;
        draw.state = 2525;

        Blk5Event current;
        std::vector<Blk5Event> events;

        for (int e = 0; e < 160; ++e)
        {
            for (auto& c : current.bands)
            {
                if (draw.uniform() < 0.5)
                {
                    c.type = (float) juce::jmin (7, (int) (draw.uniform() * 8.0));
                    c.slope = (float) juce::jmin (7, (int) (draw.uniform() * 8.0));
                }

                if (draw.uniform() < 0.3)
                    c.placement = (float) juce::jmin (4, (int) (draw.uniform() * 5.0));

                if (draw.uniform() < 0.3)
                    c.enabled = draw.uniform() > 0.3 ? 1.0f : 0.0f;

                c.freq = juce::jlimit (10.0f, 30000.0f, (float) std::exp (std::log (10.0) + draw.uniform() * std::log (3000.0)));
                c.gain = draw.uniform() < 0.25 ? 0.0f : juce::jlimit (-30.0f, 30.0f, (float) (-30.0 + 60.0 * draw.uniform()));
                c.q = juce::jlimit (0.025f, 40.0f, (float) std::exp (std::log (0.025) + draw.uniform() * std::log (1600.0)));
            }

            if (draw.uniform() < 0.3)
                current.outputDb = (float) (-36.0 + 72.0 * draw.uniform());

            if (draw.uniform() < 0.1)
                current.autoOn = current.autoOn >= 0.5f ? 0.0f : 1.0f;

            events.push_back (current);
        }

        return events;
    }

    void storeGainEvent (const Handles& h, const Blk5Event& e)
    {
        h.gainScale->store (100.0f);
        h.bypass->store (0.0f);
        h.outputGain->store (e.outputDb);
        h.autoGain->store (e.autoOn);

        for (size_t b = 0; b < 4; ++b)
        {
            const BlkBand& c = e.bands[b];
            const BandHandles& hb = h.band[b];
            hb.type->store (c.type);
            hb.slope->store (c.slope);
            hb.placement->store (c.placement);
            hb.enabled->store (c.enabled);
            hb.bypass->store (0.0f);
            hb.freq->store (c.freq);
            hb.gain->store (c.gain);
            hb.q->store (c.q);
        }
    }

    void gainBlockRowsXr()
    {
        section ("XR-blk5. block identity with output_gain and auto_gain events (Phase 2.5: PLAN-2.5 P4, P5, R2)");

        constexpr int length = 2048 * 160;
        const double rates[4] = { 44100.0, 48000.0, 96000.0, 192000.0 };
        const int sizes[4] = { 32, 64, 512, 2048 };
        const auto input = makeNoiseAt (2, length, 0x6B150002, kNoise12);
        const auto events = makeGainEvents();

        struct Job { double rate; int blockSize; bool withEvents; };
        std::vector<Job> jobs;

        for (const double rate : rates)
        {
            for (const int blockSize : sizes)
                jobs.push_back ({ rate, blockSize, true });

            jobs.push_back ({ rate, 512, false });
        }

        std::vector<std::unique_ptr<Processor>> instances;
        std::vector<Handles> handles;

        for (size_t j = 0; j < jobs.size(); ++j)
        {
            instances.push_back (makeBands (2, 4, mixedSpec));
            handles.push_back (handlesOf (*instances.back()));
        }

        std::vector<juce::AudioBuffer<float>> outputs (jobs.size());
        std::vector<unsigned long> evaluations (jobs.size()), refreshes (jobs.size());

        parallelFor ((int) jobs.size(), [&instances, &handles, &jobs, &outputs, &evaluations, &refreshes, &input, &events] (int j)
        {
            const auto slot = (size_t) j;
            const Handles& h = handles[slot];
            const bool withEvents = jobs[slot].withEvents;
            const int blockSize = jobs[slot].blockSize;

            storeGainEvent (h, events[0]);

            outputs[slot] = renderWith (*instances[slot], input, jobs[slot].rate, blockSize,
                                        [blockSize] (int, int) { return blockSize; },
                                        [&h, &events, withEvents] (BlockInfo& current)
                                        {
                                            if (current.position % 2048 != 0)
                                                return;

                                            const auto index = (size_t) (current.position / 2048);

                                            if (index >= events.size() || (! withEvents && index > 0))
                                                return;

                                            storeGainEvent (h, events[index]);
                                        });

            evaluations[slot] = instances[slot]->getAutoGainEvaluationCount();
            refreshes[slot] = instances[slot]->getAutoGainRefreshCount();
            instances[slot]->releaseResources();
        });

        Tally tally;
        int outputEvents = 0, autoEvents = 0;

        for (size_t k = 1; k < events.size(); ++k)
        {
            outputEvents += events[k].outputDb != events[k - 1].outputDb ? 1 : 0;
            autoEvents += events[k].autoOn != events[k - 1].autoOn ? 1 : 0;
        }

        for (size_t r = 0; r < 4; ++r)
        {
            const size_t base = r * 5;
            const juce::String where = fmtG (rates[r] / 1000.0, 4) + " kHz";

            tally.check (allFinite (outputs[base]) && ! sameAudio (outputs[base], input), where + ": not finite, or the bands do not act");

            for (size_t k = 1; k < 4; ++k)
                tally.check (sameAudio (outputs[base + k], outputs[base]),
                             where + ": block " + juce::String (sizes[k]) + " differs from block 32, first at sample "
                                 + juce::String (firstDifference (outputs[base + k], outputs[base])));

            tally.check (! sameAudio (outputs[base + 4], outputs[base + 2]), where + ": the render without events is the same (the events do not bite)");

            std::printf ("      XR-blk5 %-10s auto-gain band evaluations / refreshes at blocks 32 / 64 / 512 / 2048: %lu / %lu, %lu / %lu, %lu / %lu, %lu / %lu (without events, block 512: %lu / %lu)\n",
                         where.toRawUTF8(), evaluations[base], refreshes[base], evaluations[base + 1], refreshes[base + 1],
                         evaluations[base + 2], refreshes[base + 2], evaluations[base + 3], refreshes[base + 3],
                         evaluations[base + 4], refreshes[base + 4]);

            tally.check (evaluations[base] > 0 && refreshes[base] > 0, where + ": the estimator never ran");
        }

        row (tally.bad == 0 && tally.total == 24,
             "XR-blk5 four bands, 160 events on multiples of 2048 (type, slope, placement, enable, f, gain incl. exactly 0, Q, output_gain p 0.3, auto_gain p 0.1), auto-gain refreshes in flight: blocks 32 / 64 / 512 / 2048 bit-identical at 44.1 / 48 / 96 / 192 kHz; differs from the render without events",
             tally.detail() + "  " + juce::String (outputEvents) + " output-gain and " + juce::String (autoEvents) + " auto-gain changes in the script");
    }

    //==========================================================================
    // XR-meter. The max-hold output peaks (PLAN-2.5 P7)
    //==========================================================================
    void meterRowsXr()
    {
        section ("XR-meter. the max-hold output peak (Phase 2.5: PLAN-2.5 P7)");

        constexpr int blockSize = 32, numBlocks = 50, leadBlocks = 40;
        const float spike = 2.0f;           // +6 dBFS
        const float ulp = std::nextafter (spike, 3.0f) - spike;

        struct Path { const char* name; int channels; int kind; };   // kind 0 untouched, 1 bypass parameter, 2 processBlockBypassed, 3 processed (+6 dB), 4 not prepared

        const Path paths[] = { { "stereo, untouched (no band)", 2, 0 }, { "stereo, bypassed by the parameter", 2, 1 },
                               { "stereo, processBlockBypassed", 2, 2 }, { "stereo, processed (output +6 dB)", 2, 3 },
                               { "stereo, not prepared", 2, 4 }, { "mono, untouched (no band)", 1, 0 } };

        Tally tally;
        juce::String figures;

        for (const auto& path : paths)
        {
            auto p = make();
            setLayout (*p, path.channels);
            const Handles h = handlesOf (*p);

            if (path.kind == 3)
                h.outputGain->store (6.0f);

            if (path.kind == 1)
                h.bypass->store (1.0f);

            if (path.kind != 4)
                p->prepareToPlay (kRenderRate, blockSize);

            auto input = makeNoiseAt (path.channels, (leadBlocks + numBlocks) * blockSize, 0x3E7E0000 + path.channels, 0.1f);
            input.setSample (0, (leadBlocks + 23) * blockSize + 7, spike);

            juce::AudioBuffer<float> block (path.channels, blockSize);
            juce::MidiBuffer midi;
            float harnessPeak[2] = { 0.0f, 0.0f };

            for (int k = 0; k < leadBlocks + numBlocks; ++k)
            {
                if (k == leadBlocks)
                {
                    p->readAndResetOutputPeak (0);
                    p->readAndResetOutputPeak (1);
                }

                for (int channel = 0; channel < path.channels; ++channel)
                    block.copyFrom (channel, 0, input, channel, k * blockSize, blockSize);

                if (path.kind == 2)
                    p->processBlockBypassed (block, midi);
                else
                    p->processBlock (block, midi);

                if (k >= leadBlocks)
                    for (int channel = 0; channel < path.channels; ++channel)
                        harnessPeak[channel] = juce::jmax (harnessPeak[channel], block.getMagnitude (channel, 0, blockSize));
            }

            const float read0 = p->readAndResetOutputPeak (0);
            const float read1 = p->readAndResetOutputPeak (1);
            const float again0 = p->readAndResetOutputPeak (0);
            const float again1 = p->readAndResetOutputPeak (1);

            if (path.kind != 4)
                p->releaseResources();

            const bool matches0 = sameBits (read0, harnessPeak[0]);
            const bool matches1 = path.channels == 2 ? sameBits (read1, harnessPeak[1]) : read1 == 0.0f;
            const bool spikeOk = path.kind == 3 ? read0 > spike : std::abs (read0 - spike) <= ulp;
            const bool reset = again0 == 0.0f && again1 == 0.0f;

            tally.check (matches0 && matches1 && spikeOk && reset,
                         juce::String (path.name) + ": read " + fmtJ (read0) + " / " + fmtJ (read1) + ", harness max " + fmtJ (harnessPeak[0])
                             + " / " + fmtJ (harnessPeak[1]) + ", after the read " + fmtJ (again0) + " / " + fmtJ (again1));

            figures += (figures.isEmpty() ? "" : "; ") + juce::String (path.name) + " " + fmtJ (read0) + " / " + fmtJ (read1);
        }

        row (tally.bad == 0 && tally.total == 6,
             "XR-meter readAndResetOutputPeak: a one-sample +6 dBFS spike in one 32-sample block among 50 between two reads is reported (2.0 within 1 ulp; equal to the largest |output| sample) on the untouched, bypassed (both routes), processed and not-prepared paths; the read resets to 0; mono has channel 0 only",
             tally.detail() + "  " + figures);
    }

    //==========================================================================
    // A4, A4n. G6 with the stage (PERF-01)
    //==========================================================================
    struct GainAllocResult
    {
        FadeAllocResult fade;
        unsigned long evaluations = 0, refreshes = 0;
    };

    /** As A3 (runFadeAllocWindow): the window is processBlock or
        processBlockBypassed alone, the first block after prepareToPlay
        included; 20 s at 48 kHz of the P14 every-block draw (24 bands moving
        at every block, so every refresh is dirty; the bypass toggled by both
        routes), auto_gain on and output_gain redrawn at every block (gain
        mode 2). */
    GainAllocResult runGainAllocWindow (bool plant)
    {
        GainAllocResult result;

       #if defined (__APPLE__)
        const double rate = 48000.0;
        const double seconds = 20.0;

        const StressSeed chosen = chooseStressSeed24 (rate, seconds, 1, 2);
        auto p = makeBands (2, 0, mixedSpec);
        const Handles h = handlesOf (*p);

        malloc_logger_t* const previous = malloc_logger;
        allocThread = pthread_self();
        allocCount = 0;
        freeCount = 0;
        malloc_logger = countAllocation;

        allocArmed = true;
        {
            void* volatile probe = std::malloc (64);
            std::free (probe);
        }
        allocArmed = false;
        result.fade.base.probeAllocs = allocCount;
        result.fade.base.probeFrees = freeCount;
        allocCount = 0;
        freeCount = 0;

        bool plantNext = plant;
        int viaEntry = 0;

        const StressOutcome outcome = stressLoopCore (*p, h, rate, seconds, chosen.seed, 1, true, 2, nullptr, nullptr,
                                                      [&p, &plantNext, &viaEntry] (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, bool viaBypassed)
                                                      {
                                                          allocArmed = true;

                                                          if (viaBypassed)
                                                              p->processBlockBypassed (buffer, midiMessages);
                                                          else
                                                              p->processBlock (buffer, midiMessages);

                                                          if (plantNext)
                                                          {
                                                              void* volatile planted = std::malloc (64);
                                                              std::free (planted);
                                                          }

                                                          allocArmed = false;
                                                          plantNext = false;

                                                          if (viaBypassed)
                                                              ++viaEntry;
                                                      });

        malloc_logger = previous;
        result.evaluations = p->getAutoGainEvaluationCount();
        result.refreshes = p->getAutoGainRefreshCount();
        p->releaseResources();

        result.fade.base.available = true;
        result.fade.base.allocs = allocCount;
        result.fade.base.frees = freeCount;
        result.fade.base.blocks = (int) outcome.blocks;
        result.fade.base.audioOk = chosen.ok && outcome.nonFinite == 0 && outcome.actingBlocks > 0;
        result.fade.viaEntryBlocks = viaEntry;
       #else
        juce::ignoreUnused (plant);
       #endif

        return result;
    }

    void gainAllocationRowsXr()
    {
        section ("A4. allocation with the output stage and auto gain (Phase 2.5: G6, PERF-01)");

        const GainAllocResult clean = runGainAllocWindow (false);

        if (! clean.fade.base.available)
        {
            info ("A4 allocation gate SKIPPED: no malloc_logger on this platform");
            info ("A4n allocation control SKIPPED: no malloc_logger on this platform");
            return;
        }

        const AllocResult& base = clean.fade.base;
        const bool hookLive = base.probeAllocs >= 1 && base.probeFrees >= 1;
        const bool refreshing = clean.refreshes > 0 && clean.evaluations > clean.refreshes;

        row (hookLive && base.audioOk && clean.fade.viaEntryBlocks > 0 && refreshing && base.allocs == 0 && base.frees == 0,
             "A4 processBlock and processBlockBypassed, 20 s at 48 kHz: the P14 every-block draw (24 bands moving at every block, bypass toggled by both routes), auto_gain on, output_gain redrawn per block, first block included: 0 allocations, 0 frees",
             juce::String (base.allocs) + " allocations, " + juce::String (base.frees) + " frees in " + juce::String (base.blocks) + " windows ("
                 + juce::String (clean.fade.viaEntryBlocks) + " through processBlockBypassed; " + juce::String ((juce::int64) clean.evaluations)
                 + " band evaluations in " + juce::String ((juce::int64) clean.refreshes) + " refreshes; malloc_logger, this thread); hook "
                 + (hookLive ? "live" : "DEAD") + " (probe: " + juce::String (base.probeAllocs) + " / " + juce::String (base.probeFrees) + ")"
                 + (base.audioOk ? "" : "; the stress did not run as expected (stack guard, non-finite output, or no acting block)"));

        const GainAllocResult planted = runGainAllocWindow (true);

        row (planted.fade.base.allocs >= 1 && planted.fade.base.frees >= 1,
             "A4n a planted allocation inside the processBlock / processBlockBypassed window is counted (control fires)",
             juce::String (planted.fade.base.allocs) + " allocations, " + juce::String (planted.fade.base.frees) + " frees");
    }

    //==========================================================================
    // X-cpu4. CPU with auto gain (INFO; PLAN-2.5 P11)
    //==========================================================================
    // Kept for X-cpu5 (Phase 2.6), which prints beside it.
    double xcpu4Percent[4][4] = {};

    void cpuInfoGains()
    {
        section ("X4. CPU with the output stage and auto gain (INFO, not gated; G10 binds in Stage 4)");

        constexpr int blockSize = 512;
        const double rate = 48000.0;
        const int numBlocks = (int) (20.0 * rate / (double) blockSize);
        const double audioSeconds = (double) numBlocks * (double) blockSize / rate;
        const auto noise = makeNoiseAt (2, blockSize, 0x0C9500002, kNoise12);

        struct Config { const char* name; int type; int slope; int type2; int slope2; };

        const Config configs[] =
        {
            { "24 cuts x 96 dB/oct (Low Cut <> High Cut)",       kLowCut,    7, kHighCut,   7 },
            { "24 shelves x 96 dB/oct (High <> Low)",            kHighShelf, 7, kLowShelf,  7 },
            { "24 bells <> 12 dB/oct shelves",                   kBell,      0, kHighShelf, 1 },
            { "24 shelves 96 <> 72 dB/oct",                      kHighShelf, 7, kHighShelf, 6 }
        };

        const char* const modeNames[4] = { "settled", "always fading", "one fade per second", "24 asleep" };

        // mode 4: the 24 shelves of config 1 settled in type, their gain swept at every block.
        const auto timeRun = [&] (const Config& c, int mode, bool autoOn, bool& finite)
        {
            auto p = makeBands (2, 24, [&c, mode] (int index)
            {
                return makeSpec (c.type, 40.0f * std::pow (1.28f, (float) index), mode == 3 ? 0.0f : 6.0f, 1.0f, c.slope, kStereo);
            });

            const Handles h = handlesOf (*p);
            h.autoGain->store (autoOn ? 1.0f : 0.0f);

            juce::AudioBuffer<float> block (2, blockSize);
            juce::MidiBuffer midi;
            double seconds = 0.0;

            p->prepareToPlay (rate, blockSize);

            for (int k = 0; k < numBlocks; ++k)
            {
                const double t = (double) (k * blockSize) / rate;
                bool second = false;

                if (mode == 1)
                    second = (((long) (t / 0.0215)) & 1) != 0;

                if (mode == 2)
                    second = (((long) t) & 1) != 0;

                for (size_t b = 0; b < h.band.size(); ++b)
                {
                    const BandHandles& hb = h.band[b];
                    hb.type->store ((float) (second ? c.type2 : c.type));
                    hb.slope->store ((float) (second ? c.slope2 : c.slope));

                    if (mode == 4)
                        hb.gain->store ((float) (6.0 * std::sin (2.0 * eq::pi * 0.5 * t + (double) b)));
                }

                for (int channel = 0; channel < 2; ++channel)
                    block.copyFrom (channel, 0, noise, channel, 0, blockSize);

                const auto t0 = std::chrono::steady_clock::now();
                p->processBlock (block, midi);
                const auto t1 = std::chrono::steady_clock::now();

                seconds += std::chrono::duration<double> (t1 - t0).count();
            }

            p->releaseResources();
            finite = finite && allFinite (block);
            return 100.0 * seconds / audioSeconds;
        };

        for (size_t ci = 0; ci < 4; ++ci)
        {
            const Config& c = configs[ci];
            bool finite = true;
            juce::String line = juce::String ("X-cpu4 48 kHz stereo, 512-sample blocks, 20 s, auto_gain on, ") + c.name + ":";

            for (int mode = 0; mode < 4; ++mode)
            {
                const double percent = timeRun (c, mode, true, finite);
                xcpu4Percent[ci][mode] = percent;
                line += juce::String (" ") + modeNames[mode] + " " + fmtG (percent, 3) + " % (X-cpu3 " + fmtG (xcpu3Percent[ci][mode], 3) + " %)"
                      + (mode < 3 ? ";" : "");
            }

            info (line + " of one core (bench: auto gain adds about +1.4 % in the worst case)" + (finite ? "" : "  OUTPUT NOT FINITE"));
        }

        {
            bool finite = true;
            const double off = timeRun (configs[1], 4, false, finite);
            const double on = timeRun (configs[1], 4, true, finite);

            info ("X-cpu4 48 kHz stereo, 512-sample blocks, 20 s, 24 High Shelves x 96 dB/oct, gain swept at every block (every refresh redesigns 24 bands): auto_gain off "
                  + fmtG (off, 3) + " %, on " + fmtG (on, 3) + " % of one core (difference " + fmtG (on - off, 3) + " %; bench +1.4 %)"
                  + (finite ? "" : "  OUTPUT NOT FINITE"));
        }
    }

    //==========================================================================
    //==========================================================================
    // Stage 2, Phase 2.6: listen-off identity sessions (PLAN-2.6 T1, P13)
    //
    // `--dump-sessions <dir>` renders the 16 sessions of lidSessions() through
    // a fresh processor each and writes <dir>/session-NN.f32 (NN = 00 ... 15).
    // Row XR-lid (2.6 T9) re-renders the same sessions through renderLidSession
    // on the changed processor and memcmps against the files written by the
    // unchanged 0.5.0 processor (R7: same compiler, flags and machine).
    //
    // File format: raw 32-bit IEEE float, host byte order (little-endian on
    // every target here), no header, CHANNEL-PLANAR: all samples of channel 0,
    // then all samples of channel 1 (stereo sessions only). Length per channel
    // is llround (4.0 * rate): 176400 at 44.1 kHz, 384000 at 96 kHz.
    //
    // A session is a pure function of its index: a seeded Xorshift stream per
    // concern (band draw, block sizes, gains, bypass), juce::Random for the
    // input; nothing time-dependent. Parameters are raw-value stores before
    // each block, as S5 does.
    //   - bands: the S5 / P14 draw (StressDraw24, structEvery 50), all 24
    //     bands every block; its own global-bypass bit is ignored (no listen:
    //     listen does not exist in 0.5.0)
    //   - block sizes: log-uniform 1 ... 4096 (prepared at 4096)
    //   - output_gain: redrawn with p 0.1 per block (uniform -36 ... +36 dB,
    //     each range end with p 0.05); auto_gain toggled with p 1/40
    //   - global bypass toggled with p 1/40 per block; successive bypassed
    //     spans alternate between the routes: the bypass parameter
    //     (processBlock) and processBlockBypassed (parameter left off)
    //   - input: white noise at -12 dBFS for the first total - 2048 samples,
    //     then makeNoiseTail's 2048-sample denormal tail
    //   - layouts x rates: index bit 0 = rate (44.1 / 96 kHz), bit 1 = layout
    //     (mono / stereo): all four combinations, four sessions each
    //==========================================================================
    //==========================================================================
    constexpr int kLidSessionCount = 16;
    constexpr int kLidPrepared = 4096;
    constexpr double kLidSeconds = 4.0;

    struct LidSession
    {
        int index = 0;
        double rate = 44100.0;
        int channels = 2;
        std::uint64_t seed = 1;
    };

    std::vector<LidSession> lidSessions()
    {
        std::vector<LidSession> sessions;

        for (int i = 0; i < kLidSessionCount; ++i)
        {
            LidSession s;
            s.index = i;
            s.rate = (i & 1) != 0 ? 96000.0 : 44100.0;
            s.channels = (i & 2) != 0 ? 2 : 1;
            s.seed = 0x4C49442D53455353ull ^ ((std::uint64_t) (i + 1) * 0x9E3779B97F4A7C15ull);

            if (s.seed == 0)
                s.seed = 1;

            sessions.push_back (s);
        }

        return sessions;
    }

    struct LidStats
    {
        int blocks = 0;
        int bypassToggles = 0;          // on and off, both routes
        int bypassOnViaParameter = 0;   // bypassed spans entered through the bypass parameter
        int bypassOnViaBypassed = 0;    // bypassed spans entered through processBlockBypassed
        int outputGainEdits = 0;
        int autoGainToggles = 0;
        int minBlock = 0, maxBlock = 0;
        std::int64_t nonFinite = 0;     // output samples
        double peak = 0.0;              // of the finite output samples
    };

    // A Xorshift stream for one concern of a session, never state 0.
    Xorshift lidStream (std::uint64_t seed, std::uint64_t salt)
    {
        Xorshift x;
        x.state = seed ^ salt;

        if (x.state == 0)
            x.state = 1;

        return x;
    }

    /** Renders session `idx` of lidSessions() through a fresh processor and
        returns its output (channels x llround (4 s x rate)).
          afterPrepare (Processor&): once, after prepareToPlay, before block 0
            (XR-lid registers its analyzer consumer here).
          afterBlock (Processor&, const juce::AudioBuffer<float>& out, int blockIndex):
            after every processBlock / processBlockBypassed, with that block's
            output (the hook must not write to the processor's parameters).
        *stats, when given, receives the session's counts. */
    template <typename PreparedFn, typename BlockFn>
    juce::AudioBuffer<float> renderLidSession (int idx, PreparedFn&& afterPrepare, BlockFn&& afterBlock,
                                               LidStats* stats = nullptr)
    {
        const LidSession session = lidSessions()[(size_t) juce::jlimit (0, kLidSessionCount - 1, idx)];
        const int channels = session.channels;
        const double rate = session.rate;
        const auto total = (int) std::llround (kLidSeconds * rate);

        // Input: noise at -12 dBFS, the last kTailLength samples the denormal tail.
        auto input = makeNoiseAt (channels, total, (juce::int64) (session.seed & 0x7FFFFFFFFFFFull), kNoise12);
        {
            const auto tail = makeNoiseTail (channels, (juce::int64) ((session.seed >> 17) & 0x7FFFFFFFFFFFull));

            for (int channel = 0; channel < channels; ++channel)
                input.copyFrom (channel, total - kTailLength, tail, channel, kNoiseLength, kTailLength);
        }

        auto p = makeBands (channels, 0, mixedSpec);
        const Handles h = handlesOf (*p);

        StressDraw24 draw;
        draw.random.state = session.seed;
        Xorshift sizes = lidStream (session.seed, 0x5A17E5B10C55EEDull);
        Xorshift gains = lidStream (session.seed, 0x6A09E667F3BCC909ull);
        Xorshift bypass = lidStream (session.seed, 0xB7E151628AED2A6Bull);

        LidStats st;
        float outputDb = 0.0f;
        bool autoOn = false, bypassOn = false, viaBypassed = false;

        h.bypass->store (0.0f);
        h.outputGain->store (0.0f);
        h.autoGain->store (0.0f);

        juce::AudioBuffer<float> output (channels, total);
        output.clear();
        juce::AudioBuffer<float> block (channels, kLidPrepared);
        juce::MidiBuffer midi;

        p->prepareToPlay (rate, kLidPrepared);
        afterPrepare (*p);

        st.minBlock = kLidPrepared;

        for (int position = 0; position < total; ++st.blocks)
        {
            // Block size: log-uniform 1 ... 4096.
            const int drawn = juce::jlimit (1, kLidPrepared, (int) std::exp (sizes.uniform() * std::log ((double) kLidPrepared + 1.0)));
            const int n = juce::jmin (drawn, total - position);
            st.minBlock = juce::jmin (st.minBlock, n);
            st.maxBlock = juce::jmax (st.maxBlock, n);

            // Bands: the S5 draw, every value stored before every block.
            draw.nextBlock (50);
            h.gainScale->store (draw.scale);

            for (size_t b = 0; b < h.band.size(); ++b)
            {
                const StressBand& s = draw.bands[b];
                const BandHandles& hb = h.band[b];

                hb.freq->store (s.freq);
                hb.gain->store (s.gain);
                hb.q->store (s.q);
                hb.type->store (s.type);
                hb.slope->store (s.slope);
                hb.placement->store (s.placement);
                hb.enabled->store (s.enabled);
                hb.bypass->store (s.bypass);
            }

            // Output gain and auto gain.
            if (gains.uniform() < 0.1)
            {
                const double u = gains.uniform();
                outputDb = (float) (u < 0.05 ? -36.0 : (u > 0.95 ? 36.0 : -36.0 + 72.0 * gains.uniform()));
                ++st.outputGainEdits;
            }

            if (gains.uniform() < 1.0 / 40.0)
            {
                autoOn = ! autoOn;
                ++st.autoGainToggles;
            }

            h.outputGain->store (outputDb);
            h.autoGain->store (autoOn ? 1.0f : 0.0f);

            // Global bypass, alternating routes between bypassed spans.
            if (bypass.uniform() < 1.0 / 40.0)
            {
                bypassOn = ! bypassOn;
                ++st.bypassToggles;

                if (bypassOn)
                {
                    viaBypassed = ((st.bypassOnViaParameter + st.bypassOnViaBypassed) % 2) == 1;
                    ++(viaBypassed ? st.bypassOnViaBypassed : st.bypassOnViaParameter);
                }
            }

            const bool callBypassed = bypassOn && viaBypassed;
            h.bypass->store (bypassOn && ! viaBypassed ? 1.0f : 0.0f);

            block.setSize (channels, n, false, false, true);

            for (int channel = 0; channel < channels; ++channel)
                block.copyFrom (channel, 0, input, channel, position, n);

            if (callBypassed)
                p->processBlockBypassed (block, midi);
            else
                p->processBlock (block, midi);

            afterBlock (*p, static_cast<const juce::AudioBuffer<float>&> (block), st.blocks);

            for (int channel = 0; channel < channels; ++channel)
            {
                output.copyFrom (channel, position, block, channel, 0, n);
                const float* const data = block.getReadPointer (channel);

                for (int i = 0; i < n; ++i)
                {
                    if (! std::isfinite (data[i]))
                        ++st.nonFinite;
                    else
                        st.peak = juce::jmax (st.peak, (double) std::abs (data[i]));
                }
            }

            position += n;
        }

        p->releaseResources();

        if (stats != nullptr)
            *stats = st;

        return output;
    }

    // renderLidSession with no hooks.
    juce::AudioBuffer<float> renderLidSession (int idx, LidStats* stats = nullptr)
    {
        return renderLidSession (idx, [] (Processor&) {},
                                 [] (Processor&, const juce::AudioBuffer<float>&, int) {}, stats);
    }

    // --dump-sessions <dir>: the 16 sessions as raw channel-planar float files.
    // Returns 0 when every file was written and every session's output is finite.
    int dumpLidSessions (const juce::File& dir)
    {
        if (! keepsDenormals())
        {
            std::printf ("FATAL: this thread flushes denormals to zero; the session input needs them\n");
            return 255;
        }

        if (! dir.createDirectory())
        {
            std::printf ("FATAL: cannot create %s\n", dir.getFullPathName().toRawUTF8());
            return 255;
        }

        int bad = 0;

        for (const auto& session : lidSessions())
        {
            LidStats st;
            const auto out = renderLidSession (session.index, &st);
            const auto file = dir.getChildFile (juce::String::formatted ("session-%02d.f32", session.index));

            bool written = false;

            if (FILE* f = std::fopen (file.getFullPathName().toRawUTF8(), "wb"))
            {
                written = true;

                for (int channel = 0; channel < out.getNumChannels(); ++channel)
                    written = written && std::fwrite (out.getReadPointer (channel), sizeof (float), (size_t) out.getNumSamples(), f)
                                             == (size_t) out.getNumSamples();

                written = (std::fclose (f) == 0) && written;
            }

            if (! written || st.nonFinite != 0)
                ++bad;

            std::printf ("session-%02d  %5.1f kHz  %-6s  %4d blocks (%d ... %d)  bypass toggles %2d (on via parameter %d, via processBlockBypassed %d)  "
                         "output_gain edits %3d  auto_gain toggles %2d  peak %.6g (%+.1f dBFS)  non-finite %lld%s\n",
                         session.index, session.rate / 1000.0, session.channels == 1 ? "mono" : "stereo",
                         st.blocks, st.minBlock, st.maxBlock, st.bypassToggles, st.bypassOnViaParameter, st.bypassOnViaBypassed,
                         st.outputGainEdits, st.autoGainToggles, st.peak, 20.0 * std::log10 (juce::jmax (st.peak, 1.0e-300)),
                         (long long) st.nonFinite, written ? "" : "  WRITE FAILED");
        }

        std::fflush (stdout);
        return bad == 0 ? 0 : 1;
    }

    //==========================================================================
    //==========================================================================
    // Stage 2, Phase 2.6: analyzer taps, display model, listen (PLAN-2.6 T9)
    //
    // Harness references (PLAN-2.6 P12). None of them calls eq::Listen,
    // OutputStage::process / nextGains or a DisplayModel design call:
    //   - listenRefRender: P4 - P6 re-stated per sample. The D11 mapping
    //     (lrMap), the solo router, two static voices (RefChain: Designer::
    //     design -> toRuntime -> svfTick / onePoleTick), the 30 ms input ramp
    //     and the 20 ms fade from the weight formula, the tick schedule and
    //     the latch. It is fed the plugin's dry input and its EQ path (the
    //     float output of the same instance with listen off and the gains
    //     neutral, widened), and the gains are re-stated as constants (the
    //     rows that use it hold them settled). listenHardRender: the same with
    //     R = 0 and N = 1 (the hard control), or R = 0 and N = 20 ms (the fade
    //     from rest of the bench, printed).
    //   - G9: composeFromPackets (the display model's packets parsed back
    //     from their var, eq::bandResponseAtZ per band, the 2x2 placement
    //     matrices in index order), dampedResponse (the weighted direct sum
    //     of the plugin's own float impulse responses, a recursive phasor
    //     re-set every 4096 samples), slowestPoleRadius (from the packets).
    //
    // Rows with many renders run on worker threads: one instance per worker,
    // constructed on the main thread (makePool), reused across that worker's
    // jobs; every job first writes every raw value back to its default
    // (resetAllRaw), then the values it uses.
    //==========================================================================
    //==========================================================================
    using Cd = std::complex<double>;

    /** XR-lid's 0.5.0 session dump (PLAN-2.6 T1, P13; not committed):
        --lid-dir <dir>, else the environment variable OPEQ_LID_DIR. */
    juce::File lidDumpDir;

    /** --dump-g9 <dir> (PLAN-3.2 P4): the G9 (b) gate's input. When set, the
        XR-g9 jobs (250 stereo + 10 mono draws, unchanged) each write
        { rate, stereo, header, packets, grid, cpp } to <dir>/sNNN.json or
        mNNN.json, and the harness exits 0 before any XR-g9 row is scored.
        grid = the 192 G9 log points plus each active band's frequency below
        Nyquist; cpp = composeFromPackets on the unit circle at each grid point,
        LL / RR / MM / SS as re / im pairs. tests/g9b/g9b.mjs scores the page's
        own evaluator against it. Without the flag nothing below changes. */
    juce::File g9DumpDir;

    /** Every raw value back to the layout's defaults, listen off, analyzer
        mode 2 (the default), no consumer. */
    void resetAllRaw (Processor& p, const Handles& h)
    {
        h.bypass->store (0.0f);
        h.gainScale->store (100.0f);
        h.outputGain->store (0.0f);
        h.autoGain->store (0.0f);

        for (const auto& hb : h.band)
            storeRaw (hb, rawBandOff());

        p.listenBand.store (-1);
        p.analyzerMode.store (2);
        p.setAnalyzerConsumer (false);
    }

    /** Up to 12 instances on one layout, constructed here (main thread). */
    std::vector<std::unique_ptr<Processor>> makePool (int channels, int jobs)
    {
        const int hardware = (int) std::thread::hardware_concurrency();
        const int count = juce::jlimit (1, 12, juce::jmin (jobs, hardware > 0 ? hardware : 1));
        std::vector<std::unique_ptr<Processor>> pool;

        for (int i = 0; i < count; ++i)
            pool.push_back (makeBands (channels, 0, mixedSpec));

        return pool;
    }

    /** job (j, instance, handles) for j = 0 ... count - 1, one worker thread
        per instance of `pool` (this thread included). Jobs print nothing and
        touch no tally. */
    template <typename JobFn>
    void poolFor (int count, std::vector<std::unique_ptr<Processor>>& pool, JobFn&& job)
    {
        if (count <= 0 || pool.empty())
            return;

        std::vector<Handles> handles;

        for (auto& instance : pool)
            handles.push_back (handlesOf (*instance));

        std::atomic<int> next { 0 };

        const auto work = [&next, &job, &pool, &handles, count] (size_t w)
        {
            for (;;)
            {
                const int mine = next.fetch_add (1);

                if (mine >= count)
                    return;

                job (mine, *pool[w], handles[w]);
            }
        };

        std::vector<std::thread> threads;

        for (size_t w = 1; w < pool.size(); ++w)
            threads.emplace_back (work, w);

        work (0);

        for (auto& thread : threads)
            thread.join();
    }

    /** A render at `rate`, prepared at `prepared`: block n's length from
        sizeAt (index, position), a block starting at each sample of `cuts`,
        hook (BlockInfo&) before each block (it may switch the entry point).
        Worker-safe (no std::function); it does not release the instance. */
    template <typename SizeFn, typename HookFn>
    juce::AudioBuffer<float> renderSized (Processor& p, const juce::AudioBuffer<float>& input, double rate, int prepared,
                                          SizeFn&& sizeAt, std::vector<int> cuts, HookFn&& hook)
    {
        std::sort (cuts.begin(), cuts.end());

        const int numChannels = input.getNumChannels();
        const int length = input.getNumSamples();

        juce::AudioBuffer<float> output (numChannels, length);
        output.clear();
        juce::AudioBuffer<float> block (numChannels, juce::jmax (1, prepared));
        juce::MidiBuffer midi;

        p.prepareToPlay (rate, juce::jmax (1, prepared));

        size_t nextCut = 0;
        int index = 0;

        for (int position = 0; position < length; ++index)
        {
            int n = juce::jlimit (1, length - position, (int) sizeAt (index, position));

            while (nextCut < cuts.size() && cuts[nextCut] <= position)
                ++nextCut;

            if (nextCut < cuts.size() && cuts[nextCut] - position < n)
                n = cuts[nextCut] - position;

            BlockInfo current;
            current.index = index;
            current.position = position;
            current.viaBypassed = false;

            hook (current);

            block.setSize (numChannels, n, false, false, true);

            for (int channel = 0; channel < numChannels; ++channel)
                block.copyFrom (channel, 0, input, channel, position, n);

            if (current.viaBypassed)
                p.processBlockBypassed (block, midi);
            else
                p.processBlock (block, midi);

            for (int channel = 0; channel < numChannels; ++channel)
                output.copyFrom (channel, position, block, channel, 0, n);

            position += n;
        }

        return output;
    }

    /** renderSized with every block blockSize long (prepared: blockSize). */
    template <typename HookFn>
    juce::AudioBuffer<float> renderAt (Processor& p, const juce::AudioBuffer<float>& input, double rate, int blockSize,
                                       const std::vector<int>& cuts, HookFn&& hook, int prepared = 0)
    {
        return renderSized (p, input, rate, prepared > 0 ? prepared : blockSize,
                            [blockSize] (int, int) { return blockSize; }, cuts, hook);
    }

    const auto noHook = [] (BlockInfo&) {};

    /** max |y - ref| over [from, to) re the largest |y| there, one lane, dB. */
    double laneErrorReDb (const juce::AudioBuffer<float>& y, int channel, const std::vector<double>& ref, int from, int to)
    {
        if (ref.size() != (size_t) y.getNumSamples())
            return std::numeric_limits<double>::infinity();

        const float* data = y.getReadPointer (channel);
        double error = 0.0, peak = 0.0;

        for (int i = juce::jmax (0, from); i < juce::jmin (to, y.getNumSamples()); ++i)
        {
            const double d = std::abs ((double) data[i] - ref[(size_t) i]);

            if (! (d <= error))
                error = d;

            peak = juce::jmax (peak, std::abs ((double) data[i]));
        }

        if (! (peak > 0.0))
            return std::numeric_limits<double>::infinity();

        return 20.0 * std::log10 (juce::jmax (error, 1.0e-300) / peak);
    }

    double dbOf (double magnitude)
    {
        return 20.0 * std::log10 (juce::jmax (magnitude, 1.0e-300));
    }

    //==========================================================================
    // G9 (a): the display's closed form against the measured response
    // (PLAN-2.6 C1, T9; RESEARCH-2.6 Q1; tap_bench damped, g9unit, json)
    //==========================================================================
    constexpr int kG9Length = 1 << 17;
    constexpr int kG9Points = 192;
    constexpr int kG9StereoPerRate = 50;
    constexpr int kG9MonoPerRate = 2;               // 10 mono over the five rates
    constexpr int kG9UnitKeep = 25, kG9UnitMaxDraws = 200;
    constexpr double kG9Limit = 0.05, kG9WithinDb = 80.0, kG9JsonLimit = 1.0e-7;
    constexpr double kG9AbsoluteFloorDb = -120.0;     // PLAN-2.6 execute, Taylor's call 2026-11-15
    const double kG9Rates[5] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };

    struct G9Config
    {
        int numBands = 1;
        std::array<eq::RawBand, (size_t) Processor::kNumBands> raw {};
    };

    /** A full-range draw (G5 / tap_bench randomConfig): 1 - 24 enabled bands;
        every band type 0 - 7, frequency log 10 Hz - 30 kHz, gain +-30 dB, Q
        log 0.025 - 40, slope 0 - 7, placement 0 - 4. */
    G9Config g9Draw (Xorshift& random)
    {
        G9Config c;
        c.numBands = 1 + juce::jmin (23, (int) (random.uniform() * 24.0));

        for (int b = 0; b < Processor::kNumBands; ++b)
        {
            eq::RawBand r;
            r.enabled = b < c.numBands ? 1.0f : 0.0f;
            r.bypass = 0.0f;
            r.type = (float) juce::jmin (7, (int) (random.uniform() * 8.0));
            r.freq = juce::jlimit (10.0f, 30000.0f, (float) std::exp (std::log (10.0) + random.uniform() * std::log (3000.0)));
            r.gain = juce::jlimit (-30.0f, 30.0f, (float) (-30.0 + 60.0 * random.uniform()));
            r.q = juce::jlimit (0.025f, 40.0f, (float) std::exp (std::log (0.025) + random.uniform() * std::log (1600.0)));
            r.slope = (float) juce::jmin (7, (int) (random.uniform() * 8.0));
            r.placement = (float) juce::jmin (4, (int) (random.uniform() * 5.0));
            c.raw[(size_t) b] = r;
        }

        return c;
    }

    /** XR-g9n: the L / M order control needs a draw holding an L band and an
        M band that do not commute (PLAN-2.6 XG-ctl wording): bands 1 and 2. */
    void g9ForcePair (G9Config& c)
    {
        c.numBands = juce::jmax (c.numBands, 2);
        c.raw[0] = rawBandSpec (makeSpec (kBell, 1000.0f, 12.0f, 1.0f, 1, kLeft));
        c.raw[1] = rawBandSpec (makeSpec (kBell, 1000.0f, -9.0f, 2.0f, 1, kMid));
    }

    void storeG9Config (Processor& p, const Handles& h, const G9Config& c)
    {
        resetAllRaw (p, h);

        for (int b = 0; b < Processor::kNumBands; ++b)
            storeRaw (h.band[(size_t) b], c.raw[(size_t) b]);
    }

    /** 192 log points, 10 Hz - min (30 kHz, 0.49 fs). */
    std::vector<double> g9Grid (double rate)
    {
        const double top = juce::jmin (30000.0, 0.49 * rate);
        std::vector<double> hz;

        for (int i = 0; i < kG9Points; ++i)
            hz.push_back (std::exp (std::log (10.0) + (std::log (top) - std::log (10.0)) * (double) i / (double) (kG9Points - 1)));

        return hz;
    }

    /** One band of the display, parsed back from its packet. */
    struct G9Band
    {
        int band = -1, type = -1, placement = 0;
        bool active = false;
        eq::BandDesign design {};
    };

    bool g9Numbers (const juce::var& v, double* out, int count, bool& finite)
    {
        const juce::Array<juce::var>* a = v.getArray();

        if (a == nullptr || a->size() != count)
            return false;

        for (int i = 0; i < count; ++i)
        {
            const juce::var& e = a->getReference (i);

            if (! (e.isDouble() || e.isInt() || e.isInt64()))
                return false;

            out[i] = (double) e;
            finite = finite && std::isfinite (out[i]);
        }

        return true;
    }

    /** The packet's var back to a design. The packet carries no half flags:
        a half sent as its neutral values (all ones, DisplayModel.h) is read
        as absent, which is what the designer writes for an absent half. */
    bool g9Parse (const juce::var& v, G9Band& out, bool& finite)
    {
        if (! v.isObject())
            return false;

        const juce::var sections = v.getProperty ("sections", juce::var());
        const juce::var onePoles = v.getProperty ("onePoles", juce::var());
        const juce::Array<juce::var>* s = sections.getArray();
        const juce::Array<juce::var>* o = onePoles.getArray();

        if (s == nullptr || o == nullptr || s->size() != o->size() || s->size() > (int) eq::maxSections)
            return false;

        out.band = (int) v.getProperty ("band", -1);
        out.type = (int) v.getProperty ("type", -1);
        out.placement = (int) v.getProperty ("placement", 0);
        out.active = (bool) v.getProperty ("active", false);
        out.design = eq::BandDesign {};
        out.design.numSections = s->size();

        for (int i = 0; i < s->size(); ++i)
        {
            double svf[5] = {}, onePole[3] = {};

            if (! g9Numbers (s->getReference (i), svf, 5, finite) || ! g9Numbers (o->getReference (i), onePole, 3, finite))
                return false;

            eq::SectionParams& p = out.design.params[(size_t) i];
            p.g = svf[0];
            p.k = svf[1];
            p.mL = svf[2];
            p.mB = svf[3];
            p.mH = svf[4];
            p.G = onePole[0];
            p.cL = onePole[1];
            p.cH = onePole[2];
            p.hasSvf = ! (svf[0] == 1.0 && svf[1] == 1.0 && svf[2] == 1.0 && svf[3] == 1.0 && svf[4] == 1.0);
            p.hasOnePole = ! (onePole[0] == 1.0 && onePole[1] == 1.0 && onePole[2] == 1.0);
        }

        return true;
    }

    bool g9SameBits (double a, double b)
    {
        return std::memcmp (&a, &b, sizeof (double)) == 0;
    }

    /** Field by field (wave-1 notes 13: SectionParams has padding, so no memcmp of the struct). */
    bool g9SameSection (const eq::SectionParams& a, const eq::SectionParams& b)
    {
        return a.hasSvf == b.hasSvf && a.hasOnePole == b.hasOnePole
            && g9SameBits (a.g, b.g) && g9SameBits (a.k, b.k) && g9SameBits (a.mL, b.mL) && g9SameBits (a.mB, b.mB)
            && g9SameBits (a.mH, b.mH) && g9SameBits (a.G, b.G) && g9SameBits (a.cL, b.cL) && g9SameBits (a.cH, b.cH);
    }

    bool g9SameSections (const eq::BandDesign& a, const eq::BandDesign& b)
    {
        if (a.numSections != b.numSections || a.numSections < 0 || a.numSections > (int) eq::maxSections)
            return false;

        for (int i = 0; i < a.numSections; ++i)
            if (! g9SameSection (a.params[(size_t) i], b.params[(size_t) i]))
                return false;

        return true;
    }

    struct M2 { Cd a, b, c, d; };   // [[a b] [c d]]

    M2 m2Mul (const M2& x, const M2& y)
    {
        return { x.a * y.a + x.b * y.c, x.a * y.b + x.b * y.d,
                 x.c * y.a + x.d * y.c, x.c * y.b + x.d * y.d };
    }

    /** ARCHITECTURE "True-response display": the placement's 2x2 in L / R. */
    M2 g9Matrix (int placement, Cd F)
    {
        const Cd one (1.0, 0.0), zero (0.0, 0.0);
        const Cd h = 0.5 * (F - one);

        switch (placement)
        {
            case kLeft:  return { F, zero, zero, one };
            case kRight: return { one, zero, zero, F };
            case kMid:   return { one + h, h, h, one + h };
            case kSide:  return { one + h, -h, -h, one + h };
            default:     break;
        }

        return { F, zero, zero, F };
    }

    /** LL, RR, MM = 1/2 sum M_ij, SS = 1/2 (M00 - M01 - M10 + M11). */
    std::array<Cd, 4> g9Drawn (const M2& m)
    {
        return { m.a, m.d, 0.5 * (m.a + m.b + m.c + m.d), 0.5 * (m.a - m.b - m.c + m.d) };
    }

    /** The four XR-g9n controls, applied to the closed form only (P12). */
    enum class G9Control { none, kScaled, onePoleDropped, swappedLM, midForSide };

    /** The display's closed form at z: the active packets in index order,
        each band's response eq::bandResponseAtZ, its placement matrix
        multiplied on the left. */
    std::array<Cd, 4> composeFromPackets (const std::vector<G9Band>& bands, Cd z, G9Control control = G9Control::none)
    {
        std::vector<int> order;

        for (int i = 0; i < (int) bands.size(); ++i)
            if (bands[(size_t) i].active)
                order.push_back (i);

        if (control == G9Control::swappedLM)
        {
            int l = -1, m = -1;

            for (int k = 0; k < (int) order.size(); ++k)
            {
                const int pl = bands[(size_t) order[(size_t) k]].placement;

                if (pl == kLeft && l < 0)
                    l = k;

                if (pl == kMid && m < 0)
                    m = k;
            }

            if (l >= 0 && m >= 0)
                std::swap (order[(size_t) l], order[(size_t) m]);
        }

        M2 m { Cd (1.0, 0.0), Cd (0.0, 0.0), Cd (0.0, 0.0), Cd (1.0, 0.0) };

        for (const int i : order)
        {
            const G9Band& b = bands[(size_t) i];
            Cd F (1.0, 0.0);

            if (control == G9Control::kScaled || control == G9Control::onePoleDropped)
            {
                for (int s = 0; s < b.design.numSections; ++s)
                {
                    eq::SectionParams p = b.design.params[(size_t) s];

                    if (control == G9Control::kScaled && p.hasSvf)
                        p.k *= 1.01;

                    if (control == G9Control::onePoleDropped)
                        p.hasOnePole = false;

                    F *= eq::sectionResponseAtZ (z, p);
                }
            }
            else
            {
                F = eq::bandResponseAtZ (b.design, z);
            }

            const int placement = (control == G9Control::midForSide && b.placement == kSide) ? kMid : b.placement;
            m = m2Mul (g9Matrix (placement, F), m);
        }

        return g9Drawn (m);
    }

    /** Whether a control changes this draw's closed form at all. */
    bool g9ControlApplies (const std::vector<G9Band>& bands, G9Control control)
    {
        bool left = false, mid = false;

        for (const auto& b : bands)
        {
            if (! b.active)
                continue;

            left = left || b.placement == kLeft;
            mid = mid || b.placement == kMid;

            for (int s = 0; s < b.design.numSections; ++s)
            {
                const auto& p = b.design.params[(size_t) s];

                if (control == G9Control::kScaled && p.hasSvf)
                    return true;

                if (control == G9Control::onePoleDropped && p.hasOnePole)
                    return true;
            }

            if (control == G9Control::midForSide && b.placement == kSide)
                return true;
        }

        return control == G9Control::swappedLM && left && mid;
    }

    /** The largest pole radius of the active packets' sections: SVF poles
        x = (-k +- sqrt (k^2 - 4)) / 2, s = g x; one-pole s = -G; z = (1 + s) / (1 - s). */
    double slowestPoleRadius (const std::vector<G9Band>& bands)
    {
        double radius = 0.0;

        for (const auto& b : bands)
        {
            if (! b.active)
                continue;

            for (int s = 0; s < b.design.numSections; ++s)
            {
                const auto& p = b.design.params[(size_t) s];

                if (p.hasSvf)
                {
                    const Cd disc = std::sqrt (Cd (p.k * p.k - 4.0, 0.0));

                    for (const Cd x : { (-p.k + disc) / 2.0, (-p.k - disc) / 2.0 })
                    {
                        const Cd sv = p.g * x;
                        radius = juce::jmax (radius, std::abs ((1.0 + sv) / (1.0 - sv)));
                    }
                }

                if (p.hasOnePole)
                    radius = juce::jmax (radius, std::abs ((1.0 - p.G) / (1.0 + p.G)));
            }
        }

        return radius;
    }

    /** sum_n v[n] a^n e^{-j w n} at each w: a direct sum with a recursive
        phasor, re-set to its exact value a^(n+1) e^{-j w (n+1)} every 4096
        samples (a = 1: the unit circle; a^N = 1e-9: the damped circle). */
    std::vector<Cd> dampedResponse (const std::vector<double>& v, double a, const std::vector<double>& w)
    {
        std::vector<Cd> out (w.size());

        for (size_t k = 0; k < w.size(); ++k)
        {
            const double cr = a * std::cos (w[k]), ci = -a * std::sin (w[k]);
            double pr = 1.0, pm = 0.0, ar = 0.0, ai = 0.0;

            for (size_t n = 0; n < v.size(); ++n)
            {
                ar += v[n] * pr;
                ai += v[n] * pm;

                const double t = pr * cr - pm * ci;
                pm = pr * ci + pm * cr;
                pr = t;

                if ((n & 4095u) == 4095u)
                {
                    const double magnitude = std::pow (a, (double) (n + 1));
                    const double angle = -w[k] * (double) (n + 1);
                    pr = magnitude * std::cos (angle);
                    pm = magnitude * std::sin (angle);
                }
            }

            out[k] = Cd (ar, ai);
        }

        return out;
    }

    /** The response q of the impulse set: LL (impulse L, lane L), RR
        (impulse R, lane R), MM (impulse (1, 1), 1/2 (L + R)), SS (impulse
        (1, -1), 1/2 (L - R)); mono: lane 0 of the one impulse. */
    std::vector<double> g9ResponseOf (const std::vector<std::array<std::vector<float>, 2>>& irs, int q)
    {
        const auto& set = irs[(size_t) (irs.size() == 1 ? 0 : q)];
        std::vector<double> v (set[0].size());

        for (size_t n = 0; n < v.size(); ++n)
        {
            switch (irs.size() == 1 ? 0 : q)
            {
                case 0:  v[n] = (double) set[0][n]; break;
                case 1:  v[n] = (double) set[1][n]; break;
                case 2:  v[n] = 0.5 * ((double) set[0][n] + (double) set[1][n]); break;
                default: v[n] = 0.5 * ((double) set[0][n] - (double) set[1][n]); break;
            }
        }

        return v;
    }

    /** The largest closed-form value (dB) over the grid of one response. */
    double g9TopOf (const std::vector<Cd>& closed)
    {
        double top = -std::numeric_limits<double>::infinity();

        for (const auto& c : closed)
            top = juce::jmax (top, dbOf (std::abs (c)));

        return top;
    }

    /** Worst |dB measured - dB closed| over the points where the closed form
        is within 80 dB of floorTop. PLAN-2.6 execute, Taylor's call
        2026-11-15 (the G9 floor): floorTop is the LARGEST maximum among the
        configuration's LL / RR / MM / SS, not each response's own (C1 as
        first written scored a response cancelled to -345 dB down to -425 dB,
        below the float output's resolution of the louder lanes). *topOut
        receives this response's own maximum (the INFO diagnostic). */
    double g9Worst (const std::vector<Cd>& closed, const std::vector<Cd>& measured, int& scored, double floorTop, double* topOut = nullptr)
    {
        if (topOut != nullptr)
            *topOut = g9TopOf (closed);

        double worst = 0.0;

        for (size_t k = 0; k < closed.size(); ++k)
        {
            const double ref = dbOf (std::abs (closed[k]));

            // PLAN-2.6 execute, Taylor's call 2026-11-15: and an absolute
            // floor, -120 dB re the unit impulse.
            if (ref < floorTop - kG9WithinDb || ref < kG9AbsoluteFloorDb)
                continue;

            const double e = std::abs (dbOf (std::abs (measured[k])) - ref);
            ++scored;

            if (! (e <= worst))
                worst = e;
        }

        return worst;
    }

    struct G9Out
    {
        bool rendered = false, polled = false, parsedOk = true, finiteDesigns = true, finiteOutput = true;
        bool idOk = true, jsonOk = true;
        int numActive = 0, idCompared = 0, scored = 0;
        std::string idNote;
        double worst = 0.0;
        std::array<double, 4> control {};       // k x 1.01, one-pole dropped, L / M swapped, Mid for Side
        std::array<bool, 4> controlApplies {};
        double jsonDb = 0.0;
        double radius = 0.0;
        std::string g9bDump;   // --dump-g9 only (PLAN-3.2 P4)

        // Diagnostics (printed, not gated): the worst error per response,
        // by that response's own maximum over the grid: >= -20 dB,
        // -20 ... -80 dB, below -80 dB (re the unit impulse).
        std::array<double, 3> worstByTop {}, jsonByTop {};
        std::array<int, 3> responsesByTop {}, failingByTop {};
    };

    int g9TopBucket (double top)
    {
        return top >= -20.0 ? 0 : (top >= -80.0 ? 1 : 2);
    }

    /** One configuration: four impulses (stereo; mono: one), each from a
        fresh prepare (the first block starts every band settled, so no ramp),
        blocks of 4096, `length` samples. After the first block of the first
        impulse: DisplayModel::poll, every packet parsed and compared with
        getBandRealisedDesign (XR-g9id), and through JSON (XR-g9j). Then the
        closed form from the packets against the measured response on the
        damped circle (damped) or the unit circle. */
    G9Out g9RunJob (Processor& p, const Handles& h, const G9Config& c, double rate, bool stereo, int length, bool damped, bool controls)
    {
        G9Out out;
        storeG9Config (p, h, c);

        auto model = std::make_unique<DisplayModel> (p);
        std::vector<G9Band> bands ((size_t) Processor::kNumBands), jsonBands ((size_t) Processor::kNumBands);

        const auto afterFirst = [&] ()
        {
            out.polled = true;
            model->poll();

            for (int b = 0; b < Processor::kNumBands; ++b)
            {
                const juce::var packet = model->packetFor (b);
                G9Band& pb = bands[(size_t) b];
                bool finite = true;

                if (! g9Parse (packet, pb, finite) || pb.band != b)
                {
                    out.parsedOk = false;
                    continue;
                }

                out.finiteDesigns = out.finiteDesigns && finite;

                bool jsonFinite = true;

                if (! g9Parse (juce::JSON::parse (juce::JSON::toString (packet)), jsonBands[(size_t) b], jsonFinite))
                    out.jsonOk = false;

                eq::BandDesign realised {};
                const bool has = p.getBandRealisedDesign (b, realised);
                ++out.idCompared;

                bool ok = ! has;

                if (pb.active)
                {
                    ++out.numActive;
                    ok = has && g9SameSections (pb.design, realised) && g9SameSections (model->getBandDesign (b), realised);
                }

                if (! ok)
                {
                    out.idOk = false;

                    if (out.idNote.empty())
                        out.idNote = "band " + std::to_string (b + 1)
                                   + (pb.active ? (has ? ": the packet differs from the realised design" : ": an active packet, no realised design")
                                                : ": an inactive packet, but a realised design");
                }
            }
        };

        const int numImpulses = stereo ? 4 : 1;
        const float impulses[4][2] = { { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f } };
        std::vector<std::array<std::vector<float>, 2>> irs ((size_t) numImpulses);

        for (int s = 0; s < numImpulses; ++s)
        {
            juce::AudioBuffer<float> input (stereo ? 2 : 1, length);
            input.clear();
            input.setSample (0, 0, impulses[s][0]);

            if (stereo)
                input.setSample (1, 0, impulses[s][1]);

            bool first = (s == 0);

            const auto y = renderAt (p, input, rate, 4096, {}, [&first, &afterFirst] (BlockInfo& current)
            {
                if (first && current.index == 1)
                {
                    first = false;
                    afterFirst();
                }
            });

            if (first)
                afterFirst();

            out.finiteOutput = out.finiteOutput && allFinite (y);

            for (int channel = 0; channel < y.getNumChannels(); ++channel)
                irs[(size_t) s][(size_t) channel] = channelOf (y, channel);
        }

        p.releaseResources();
        out.rendered = true;
        out.radius = slowestPoleRadius (bands);

        if (! out.parsedOk)
            return out;

        const auto hz = g9Grid (rate);
        std::vector<double> w;

        for (const double f : hz)
            w.push_back (2.0 * eq::pi * f / rate);

        const double a = damped ? std::pow (1.0e-9, 1.0 / (double) length) : 1.0;
        const int numResponses = stereo ? 4 : 1;

        const G9Control kinds[4] = { G9Control::kScaled, G9Control::onePoleDropped, G9Control::swappedLM, G9Control::midForSide };
        std::array<std::vector<std::array<Cd, 4>>, 5> closed;      // [0] the closed form, [1 ... 4] the controls

        for (size_t k = 0; k < w.size(); ++k)
        {
            const Cd z = std::polar (1.0 / a, w[k]);
            closed[0].push_back (composeFromPackets (bands, z));

            if (controls)
                for (int ci = 0; ci < 4; ++ci)
                    closed[(size_t) ci + 1].push_back (composeFromPackets (bands, z, kinds[ci]));
        }

        if (controls)
            for (int ci = 0; ci < 4; ++ci)
                out.controlApplies[(size_t) ci] = g9ControlApplies (bands, kinds[ci]);

        // The scoring floor: 80 dB below the largest maximum among the
        // configuration's responses (PLAN-2.6 execute, Taylor's call
        // 2026-11-15), per closed form (the controls use their own).
        std::array<double, 5> floorTop;

        for (size_t f = 0; f < 5; ++f)
        {
            floorTop[f] = -std::numeric_limits<double>::infinity();

            if (f > 0 && ! controls)
                continue;

            for (int q = 0; q < numResponses; ++q)
            {
                std::vector<Cd> one;

                for (const auto& c4 : closed[f])
                    one.push_back (c4[(size_t) q]);

                floorTop[f] = juce::jmax (floorTop[f], g9TopOf (one));
            }
        }

        for (int q = 0; q < numResponses; ++q)
        {
            const auto measured = dampedResponse (g9ResponseOf (irs, q), a, w);
            std::vector<Cd> reference;

            for (const auto& c4 : closed[0])
                reference.push_back (c4[(size_t) q]);

            double top = 0.0;
            const double e = g9Worst (reference, measured, out.scored, floorTop[0], &top);
            const int bucket = g9TopBucket (top);
            out.worst = juce::jmax (out.worst, e);
            out.worstByTop[(size_t) bucket] = juce::jmax (out.worstByTop[(size_t) bucket], e);
            ++out.responsesByTop[(size_t) bucket];
            out.failingByTop[(size_t) bucket] += (e <= kG9Limit) ? 0 : 1;

            if (! std::isfinite (out.worst))
                out.worst = std::numeric_limits<double>::infinity();

            if (controls)
            {
                for (int ci = 0; ci < 4; ++ci)
                {
                    std::vector<Cd> perturbed;

                    for (const auto& c4 : closed[(size_t) ci + 1])
                        perturbed.push_back (c4[(size_t) q]);

                    int ignored = 0;
                    out.control[(size_t) ci] = juce::jmax (out.control[(size_t) ci], g9Worst (perturbed, measured, ignored, floorTop[(size_t) ci + 1]));
                }
            }
        }

        // --dump-g9 (PLAN-3.2 P4): this draw's packets, header, grid and C++
        // closed form, for tests/g9b/g9b.mjs. Nothing scored here changes.
        if (g9DumpDir != juce::File())
        {
            std::vector<double> grid (hz.begin(), hz.end());

            for (int b = 0; b < Processor::kNumBands; ++b)
                if (bands[(size_t) b].active)
                {
                    const double f = model->getBandInput (b).settings.freqHz;
                    if (f >= 10.0 && f <= 30000.0 && f < 0.5 * rate)
                        grid.push_back (f);
                }

            std::string s = "{\"rate\":" + std::to_string (rate) + ",\"stereo\":" + (stereo ? "true" : "false");
            s += ",\"header\":" + juce::JSON::toString (model->headerVar(), true).toStdString();
            s += ",\"packets\":" + juce::JSON::toString (model->allPackets(), true).toStdString();
            s += ",\"grid\":[";
            char buf[64];

            for (size_t k = 0; k < grid.size(); ++k)
            {
                std::snprintf (buf, sizeof (buf), "%s%.17g", k ? "," : "", grid[k]);
                s += buf;
            }

            s += "],\"cpp\":[";

            for (size_t k = 0; k < grid.size(); ++k)
            {
                const auto c4 = composeFromPackets (bands, std::polar (1.0, 2.0 * eq::pi * grid[k] / rate));
                s += k ? ",[" : "[";

                for (int q = 0; q < 4; ++q)
                {
                    std::snprintf (buf, sizeof (buf), "%s%.17g,%.17g", q ? "," : "", c4[(size_t) q].real(), c4[(size_t) q].imag());
                    s += buf;
                }

                s += "]";
            }

            s += "]}";
            out.g9bDump = std::move (s);
        }

        // XR-g9j: the JSON round trip, composed again on the unit circle,
        // against the unserialised packets, within 80 dB of the largest
        // response maximum (PLAN-2.6 execute, Taylor's call 2026-11-15).
        if (out.jsonOk)
        {
            std::array<std::vector<Cd>, 4> plain, serial;

            for (size_t k = 0; k < w.size(); ++k)
            {
                const Cd z = std::polar (1.0, w[k]);
                const auto a4 = composeFromPackets (bands, z);
                const auto b4 = composeFromPackets (jsonBands, z);

                for (int q = 0; q < 4; ++q)
                {
                    plain[(size_t) q].push_back (a4[(size_t) q]);
                    serial[(size_t) q].push_back (b4[(size_t) q]);
                }
            }

            double jsonFloor = -std::numeric_limits<double>::infinity();

            for (int q = 0; q < numResponses; ++q)
                jsonFloor = juce::jmax (jsonFloor, g9TopOf (plain[(size_t) q]));

            for (int q = 0; q < numResponses; ++q)
            {
                int ignored = 0;
                double top = 0.0;
                const double e = g9Worst (plain[(size_t) q], serial[(size_t) q], ignored, jsonFloor, &top);
                out.jsonDb = juce::jmax (out.jsonDb, e);
                const auto bucket = (size_t) g9TopBucket (top);
                out.jsonByTop[bucket] = juce::jmax (out.jsonByTop[bucket], e);
            }
        }

        return out;
    }

    /** XR-g9id's named cases: one instance, 48 kHz stereo, one block of
        noise, then the packets against the realised designs. Returns the
        number of bands that disagree; `note` names the first. */
    int g9IdCase (Processor& p, const Handles& h, const std::vector<std::pair<int, eq::RawBand>>& bandsSet, float gainScale,
                  const std::vector<int>& expectInactive, const std::vector<int>& expectActive, std::string& note)
    {
        resetAllRaw (p, h);
        h.gainScale->store (gainScale);

        for (const auto& entry : bandsSet)
            storeRaw (h.band[(size_t) entry.first], entry.second);

        auto model = std::make_unique<DisplayModel> (p);
        const auto input = makeNoiseAt (2, 512, 0x6739D000, kNoise12);
        int bad = 0;

        renderAt (p, input, 48000.0, 512, {}, noHook);
        model->poll();

        const auto check = [&] (int b, bool wantActive)
        {
            G9Band pb;
            bool finite = true;
            eq::BandDesign realised {};
            const bool parsed = g9Parse (model->packetFor (b), pb, finite);
            const bool has = p.getBandRealisedDesign (b, realised);
            const bool ok = parsed && pb.active == wantActive && has == wantActive
                            && (! wantActive || (g9SameSections (pb.design, realised) && g9SameSections (model->getBandDesign (b), realised)));

            if (! ok)
            {
                ++bad;

                if (note.empty())
                    note = "band " + std::to_string (b + 1) + ": packet active " + (pb.active ? "true" : "false") + ", realised design "
                         + (has ? "present" : "absent") + " (wanted " + (wantActive ? "active" : "inactive") + ")";
            }
        };

        for (const int b : expectInactive)
            check (b, false);

        for (const int b : expectActive)
            check (b, true);

        p.releaseResources();
        return bad;
    }

    /** The diagnostic line of a set of G9 runs (not gated). */
    juce::String g9BucketText (const std::vector<G9Out>& outs, bool json)
    {
        std::array<double, 3> worst {};
        std::array<int, 3> responses {}, failing {};

        for (const auto& o : outs)
            for (size_t b = 0; b < 3; ++b)
            {
                worst[b] = juce::jmax (worst[b], json ? o.jsonByTop[b] : o.worstByTop[b]);
                responses[b] += o.responsesByTop[b];
                failing[b] += o.failingByTop[b];
            }

        const char* const names[3] = { "response maximum >= -20 dB", "-20 ... -80 dB", "below -80 dB" };
        juce::String text;

        for (size_t b = 0; b < 3; ++b)
            text += (b == 0 ? "" : "; ") + juce::String (names[b]) + ": worst " + fmtG (worst[b], 3) + " dB"
                  + (json ? juce::String() : " (" + juce::String (failing[b]) + " of " + juce::String (responses[b]) + " responses above 0.05)");

        return text;
    }

    void displayRowsXr()
    {
        section ("XR-g9 / XR-dm. the display model and G9 (a) (Phase 2.6: UI-02 C++ half; C1; PLAN-2.6 P8, T9)");

        struct Job
        {
            int rate = 0;
            bool stereo = true;
            G9Config config;
        };

        std::vector<Job> stereoJobs, monoJobs;

        for (int r = 0; r < 5; ++r)
        {
            Xorshift draw;
            draw.state = 0x4739410000000000ull ^ (std::uint64_t) kG9Rates[r];

            for (int d = 0; d < kG9StereoPerRate; ++d)
            {
                Job j;
                j.rate = r;
                j.config = g9Draw (draw);

                if (d == 0)
                    g9ForcePair (j.config);

                stereoJobs.push_back (j);
            }

            Xorshift drawMono;
            drawMono.state = 0x47394D4F4E4F0000ull ^ (std::uint64_t) kG9Rates[r];

            for (int d = 0; d < kG9MonoPerRate; ++d)
            {
                Job j;
                j.rate = r;
                j.stereo = false;
                j.config = g9Draw (drawMono);
                monoJobs.push_back (j);
            }
        }

        std::vector<G9Out> stereoOut (stereoJobs.size()), monoOut (monoJobs.size());

        {
            auto pool = makePool (2, (int) stereoJobs.size());
            poolFor ((int) stereoJobs.size(), pool, [&stereoJobs, &stereoOut] (int j, Processor& p, const Handles& h)
            {
                const Job& job = stereoJobs[(size_t) j];
                stereoOut[(size_t) j] = g9RunJob (p, h, job.config, kG9Rates[job.rate], true, kG9Length, true, true);
            });
        }

        {
            auto pool = makePool (1, (int) monoJobs.size());
            poolFor ((int) monoJobs.size(), pool, [&monoJobs, &monoOut] (int j, Processor& p, const Handles& h)
            {
                const Job& job = monoJobs[(size_t) j];
                monoOut[(size_t) j] = g9RunJob (p, h, job.config, kG9Rates[job.rate], false, kG9Length, true, false);
            });
        }

        // --dump-g9 (PLAN-3.2 P4): write the draws and stop. Exit 0 only when
        // every draw produced its file.
        if (g9DumpDir != juce::File())
        {
            int written = 0, expected = 0;
            const auto put = [&written, &expected] (const std::vector<G9Out>& outs, const char* prefix)
            {
                for (size_t j = 0; j < outs.size(); ++j)
                {
                    ++expected;
                    const auto file = g9DumpDir.getChildFile (juce::String (prefix) + juce::String ((int) j).paddedLeft ('0', 3) + ".json");
                    if (! outs[j].g9bDump.empty() && file.replaceWithText (outs[j].g9bDump, false, false, "\n"))
                        ++written;
                }
            };

            put (stereoOut, "s");
            put (monoOut, "m");
            std::printf ("G9B dump: %d of %d files in %s\n", written, expected, g9DumpDir.getFullPathName().toRawUTF8());
            std::fflush (stdout);
            std::_Exit (written == expected && expected > 0 ? 0 : 1);
        }

        //======================================================================
        // XR-g9
        //======================================================================
        {
            Tally tally;
            double worstStereo[5] = {}, worstMono = 0.0;
            int scored = 0, activeBands = 0, voids[5] = {};
            juce::String voidNames;

            for (size_t j = 0; j < stereoJobs.size(); ++j)
            {
                const G9Out& o = stereoOut[j];
                const juce::String where = fmtG (kG9Rates[stereoJobs[j].rate] / 1000.0, 4) + " kHz draw " + juce::String ((int) j % kG9StereoPerRate);
                worstStereo[stereoJobs[j].rate] = juce::jmax (worstStereo[stereoJobs[j].rate], o.worst);
                scored += o.scored;
                activeBands += o.numActive;
                if (o.rendered && o.parsedOk && o.scored == 0)
                {
                    ++voids[stereoJobs[j].rate];
                    voidNames += (voidNames.isEmpty() ? "" : ", ") + where;
                    continue;
                }

                tally.check (o.rendered && o.parsedOk && o.finiteOutput && o.worst <= kG9Limit,
                             where + ": " + fmtG (o.worst, 3) + " dB" + (o.finiteOutput ? "" : ", output not finite") + (o.parsedOk ? "" : ", a packet did not parse"));
            }

            for (size_t j = 0; j < monoJobs.size(); ++j)
            {
                const G9Out& o = monoOut[j];
                worstMono = juce::jmax (worstMono, o.worst);
                scored += o.scored;
                activeBands += o.numActive;

                if (o.rendered && o.parsedOk && o.scored == 0)
                {
                    ++voids[monoJobs[j].rate];
                    voidNames += (voidNames.isEmpty() ? "" : ", ") + juce::String ("mono ") + fmtG (kG9Rates[monoJobs[j].rate] / 1000.0, 4) + " kHz draw "
                               + juce::String ((int) j % kG9MonoPerRate);
                    continue;
                }

                tally.check (o.rendered && o.parsedOk && o.finiteOutput && o.worst <= kG9Limit,
                             "mono " + fmtG (kG9Rates[monoJobs[j].rate] / 1000.0, 4) + " kHz: " + fmtG (o.worst, 3) + " dB");
            }

            juce::String figures;

            int voidTotal = 0;

            for (int r = 0; r < 5; ++r)
            {
                figures += (r == 0 ? "" : ", ") + fmtG (kG9Rates[r] / 1000.0, 4) + " kHz " + fmtG (worstStereo[r], 3) + " (" + juce::String (voids[r]) + " void)";
                voidTotal += voids[r];
            }

            figures += "; VOID (no point at or above -120 dB re the impulse): " + juce::String (voidTotal) + (voidNames.isEmpty() ? juce::String() : " (" + voidNames + ")");

            row (tally.bad == 0 && tally.total + voidTotal == (int) (stereoJobs.size() + monoJobs.size()),
                 "XR-g9 G9 (a) as amended (C1): 50 random full-range 1 - 24 band configurations at each of 44.1 / 48 / 88.2 / 96 / 192 kHz, stereo, plus 10 mono; four impulses (L, R, (1, 1), (1, -1)), 2^17 samples, a^N = 1e-9; the closed form from the processor's own DisplayModel packets: LL / RR / MM / SS <= 0.05 dB where the closed form is within 80 dB of the largest maximum among the four responses and at or above -120 dB re the impulse (192 log points; floors: PLAN-2.6 execute, Taylor's call 2026-11-15; a configuration with no such point is void, listed)",
                 tally.detail() + "  worst dB per rate (stereo): " + figures + "; mono " + fmtG (worstMono, 3) + " (bench <= 1.2e-3); "
                     + juce::String (scored) + " points scored, " + juce::String (activeBands) + " active bands");

            std::vector<G9Out> all (stereoOut);
            all.insert (all.end(), monoOut.begin(), monoOut.end());
            info ("XR-g9 by each response's own maximum (re the unit impulse; diagnostic, not gated): " + g9BucketText (all, false));
        }

        //======================================================================
        // XR-g9u: the unit circle on draws whose slowest pole decays
        //======================================================================
        {
            struct UnitJob
            {
                int rate = 0;
                G9Config config;
                int length = 4096;
            };

            std::vector<UnitJob> unitJobs;
            int drawn[5] = {}, kept[5] = {};

            {
                auto selector = makeBands (2, 0, mixedSpec);
                const Handles hs = handlesOf (*selector);
                auto model = std::make_unique<DisplayModel> (*selector);

                for (int r = 0; r < 5; ++r)
                {
                    const double rate = kG9Rates[r];
                    Xorshift draw;
                    draw.state = 0x4739554E49540000ull ^ (std::uint64_t) rate;

                    while (kept[r] < kG9UnitKeep && drawn[r] < kG9UnitMaxDraws)
                    {
                        ++drawn[r];
                        const G9Config c = g9Draw (draw);
                        storeG9Config (*selector, hs, c);
                        selector->prepareToPlay (rate, 512);
                        model->poll();

                        std::vector<G9Band> bands ((size_t) Processor::kNumBands);
                        bool parsed = true, finite = true;

                        for (int b = 0; b < Processor::kNumBands; ++b)
                            parsed = g9Parse (model->packetFor (b), bands[(size_t) b], finite) && parsed;

                        const double radius = slowestPoleRadius (bands);

                        if (! parsed || ! (radius > 0.0 && radius < 1.0))
                            continue;

                        const double seconds = 200.0 / (-20.0 * std::log10 (radius) * rate);

                        if (! (seconds <= 6.0))
                            continue;

                        ++kept[r];
                        UnitJob j;
                        j.rate = r;
                        j.config = c;
                        j.length = juce::jmax (4096, (int) std::llround (1.25 * seconds * rate));
                        unitJobs.push_back (j);
                    }
                }

                selector->releaseResources();
            }

            std::vector<G9Out> unitOut (unitJobs.size());

            {
                auto pool = makePool (2, (int) unitJobs.size());
                poolFor ((int) unitJobs.size(), pool, [&unitJobs, &unitOut] (int j, Processor& p, const Handles& h)
                {
                    const UnitJob& job = unitJobs[(size_t) j];
                    unitOut[(size_t) j] = g9RunJob (p, h, job.config, kG9Rates[job.rate], true, job.length, false, false);
                });
            }

            Tally tally;
            double worst[5] = {};
            int longest[5] = {}, unitVoids = 0;
            bool unitIdOk = true;
            juce::String unitVoidNames;

            for (size_t j = 0; j < unitJobs.size(); ++j)
            {
                const G9Out& o = unitOut[j];
                const int r = unitJobs[j].rate;
                worst[r] = juce::jmax (worst[r], o.worst);
                longest[r] = juce::jmax (longest[r], unitJobs[j].length);
                unitIdOk = unitIdOk && o.idOk;

                if (o.rendered && o.parsedOk && o.scored == 0)
                {
                    ++unitVoids;
                    unitVoidNames += (unitVoidNames.isEmpty() ? "" : ", ") + fmtG (kG9Rates[r] / 1000.0, 4) + " kHz kept draw " + juce::String ((int) j);
                    continue;
                }

                tally.check (o.rendered && o.parsedOk && o.finiteOutput && o.worst <= kG9Limit,
                             fmtG (kG9Rates[r] / 1000.0, 4) + " kHz: " + fmtG (o.worst, 3) + " dB");
            }

            for (int r = 0; r < 5; ++r)
                tally.check (kept[r] >= kG9UnitKeep, fmtG (kG9Rates[r] / 1000.0, 4) + " kHz: only " + juce::String (kept[r]) + " decaying draws in "
                                                         + juce::String (drawn[r]));

            juce::String figures;

            for (int r = 0; r < 5; ++r)
                figures += (r == 0 ? "" : "; ") + fmtG (kG9Rates[r] / 1000.0, 4) + " kHz " + juce::String (kept[r]) + " of " + juce::String (drawn[r])
                         + " drawn, worst " + fmtG (worst[r], 3) + " dB, longest " + fmtG ((double) longest[r] / kG9Rates[r], 3) + " s";

            row (tally.bad == 0,
                 "XR-g9u confirming unit-circle row: draws whose slowest pole (from the packets) falls 200 dB within 6 s, rendered 1.25x that time (at least 25 per rate, drawn until kept, max 200): LL / RR / MM / SS <= 0.05 dB within 80 dB of the largest maximum among the four responses and at or above -120 dB re the impulse (void configurations listed)",
                 tally.detail() + "  " + figures + " (bench <= 3.8e-2 at 48 kHz, 1 - 8 bands); VOID " + juce::String (unitVoids)
                     + (unitVoidNames.isEmpty() ? juce::String() : " (" + unitVoidNames + ")") + (unitIdOk ? "" : "; PACKET IDENTITY FAILED in a unit-circle render"));

            info ("XR-g9u by each response's own maximum (re the unit impulse; diagnostic, not gated): " + g9BucketText (unitOut, false));
        }

        //======================================================================
        // XR-g9n: the four controls on the XR-g9 draws
        //======================================================================
        {
            const char* const names[4] = { "k x 1.01", "one-pole dropped", "L band and M band in the wrong order", "Mid matrix for Side" };
            double largest[4] = {};
            int applied[4] = {}, fired[4] = {};

            for (const auto& o : stereoOut)
                for (int c = 0; c < 4; ++c)
                {
                    if (! o.controlApplies[(size_t) c])
                        continue;

                    ++applied[c];
                    largest[c] = juce::jmax (largest[c], o.control[(size_t) c]);

                    if (o.control[(size_t) c] > kG9Limit)
                        ++fired[c];
                }

            juce::String figures;
            bool all = true;

            for (int c = 0; c < 4; ++c)
            {
                all = all && largest[c] > kG9Limit;
                figures += (c == 0 ? "" : "; ") + juce::String (names[c]) + " " + fmtG (largest[c], 3) + " dB (above 0.05 on " + juce::String (fired[c])
                         + " of " + juce::String (applied[c]) + " draws it changes)";
            }

            row (all, "XR-g9n the four controls on the XR-g9 draws (the harness closed form perturbed, the plugin untouched): each exceeds 0.05 dB",
                 figures);
        }

        //======================================================================
        // XR-g9id
        //======================================================================
        {
            Tally tally;
            int compared = 0, active = 0;

            for (const auto* outs : { &stereoOut, &monoOut })
                for (const auto& o : *outs)
                {
                    compared += o.idCompared;
                    active += o.numActive;
                    tally.check (o.polled && o.parsedOk && o.idOk && o.idCompared == Processor::kNumBands, o.idNote.empty() ? juce::String ("not polled") : juce::String (o.idNote));
                }

            auto p = makeBands (2, 0, mixedSpec);
            const Handles h = handlesOf (*p);
            std::string note;

            const auto bell = [] (float freq, float gain) { return rawBandSpec (makeSpec (kBell, freq, gain, 1.0f, 1, kStereo)); };
            eq::RawBand disabled = bell (1500.0f, 6.0f);
            disabled.enabled = 0.0f;
            eq::RawBand bypassed = bell (2000.0f, 6.0f);
            bypassed.bypass = 1.0f;

            const int badNamed = g9IdCase (*p, h, { { 0, bell (1000.0f, 6.0f) }, { 1, disabled }, { 2, bypassed }, { 3, bell (3000.0f, 0.0f) } },
                                           100.0f, { 1, 2, 3 }, { 0 }, note);
            const int badScale = g9IdCase (*p, h, { { 0, bell (1000.0f, 6.0f) },
                                                    { 1, rawBandSpec (makeSpec (kLowShelf, 200.0f, 6.0f, 1.0f, 1, kStereo)) },
                                                    { 2, rawBandSpec (makeSpec (kLowCut, 100.0f, 0.0f, 1.0f, 3, kStereo)) } },
                                           0.0f, { 0, 1 }, { 2 }, note);

            tally.check (badNamed == 0, "disabled / bypassed / Bell at 0 dB: " + juce::String (note));
            tally.check (badScale == 0, "gain scale 0 %: " + juce::String (note));

            row (tally.bad == 0,
                 "XR-g9id packet identity: after the first block, every band's packet sections (parsed back from the var, and the model's BandDesign) equal getBandRealisedDesign field by field, bit for bit; inactive and sleeping bands send active: false and realise no design (disabled, bypassed, Bell at exactly 0 dB, gain scale 0 %)",
                 tally.detail() + "  " + juce::String (compared) + " bands compared over " + juce::String ((int) (stereoOut.size() + monoOut.size()))
                     + " configurations (" + juce::String (active) + " active), plus the named cases");
        }

        //======================================================================
        // XR-g9j
        //======================================================================
        {
            double worst = 0.0;
            bool ok = true, finite = true;

            for (const auto* outs : { &stereoOut, &monoOut })
                for (const auto& o : *outs)
                {
                    worst = juce::jmax (worst, o.jsonDb);
                    ok = ok && o.jsonOk && o.parsedOk;
                    finite = finite && o.finiteDesigns;
                }

            row (ok && finite && worst <= kG9JsonLimit,
                 "XR-g9j every XR-g9 packet through juce::JSON::toString and juce::JSON::parse, composed again on the unit circle: <= 1e-7 dB from the unserialised composition (within 80 dB of the largest response maximum and at or above -120 dB re the impulse); every packet value finite",
                 "worst " + fmtG (worst, 3) + " dB (bench 1.5e-8)" + (finite ? "" : "; A NON-FINITE PACKET VALUE") + (ok ? "" : "; a round trip did not parse"));

            std::vector<G9Out> all (stereoOut);
            all.insert (all.end(), monoOut.begin(), monoOut.end());
            info ("XR-g9j by each response's own maximum (re the unit impulse; diagnostic, not gated): " + g9BucketText (all, true));
        }

        //======================================================================
        // XR-dm: polling
        //======================================================================
        {
            Tally tally;
            auto p = make();
            setLayout (*p, 2);
            auto model = std::make_unique<DisplayModel> (*p);

            const auto list = [] (const juce::Array<int>& a)
            {
                juce::String s;

                for (const int v : a)
                    s += (s.isEmpty() ? "" : ",") + juce::String (v);

                return "[" + s + "]";
            };

            const auto header = [&model] (double rate, bool known)
            {
                const juce::var head = model->headerVar();
                return (double) head.getProperty ("sampleRate", 0.0) == rate && (bool) head.getProperty ("rateKnown", ! known) == known
                    && (bool) head.getProperty ("stereo", false);
            };

            const auto all24 = [] (const juce::Array<int>& a)
            {
                if (a.size() != Processor::kNumBands)
                    return false;

                for (int b = 0; b < Processor::kNumBands; ++b)
                    if (a[b] != b)
                        return false;

                return true;
            };

            auto changed = model->poll();
            tally.check (all24 (changed) && header (48000.0, false), "before any prepare: " + list (changed) + ", header not 48 kHz / rateKnown false");

            p->prepareToPlay (44100.0, 512);
            changed = model->poll();
            tally.check (all24 (changed) && header (44100.0, true), "first prepare at 44.1 kHz: " + list (changed));

            changed = model->poll();
            tally.check (changed.isEmpty(), "no edit: " + list (changed));

            setRaw (*p, bandId (7, "gain"), 5.0f);
            changed = model->poll();
            tally.check (changed.size() == 1 && changed[0] == 6, "one band (7) edited: " + list (changed));

            changed = model->poll();
            tally.check (changed.isEmpty(), "no edit after it: " + list (changed));

            {
                auto q = make();
                setLayout (*q, 2);
                loadState (*q, stateOf (*p));
                const int edited[5] = { 1, 4, 9, 15, 22 };

                for (const int b : edited)
                    setRaw (*q, bandId (b + 1, "freq"), 400.0f + 100.0f * (float) b);

                loadState (*p, stateOf (*q));
                changed = model->poll();

                bool exact = changed.size() == 5;

                for (int i = 0; i < 5 && exact; ++i)
                    exact = changed[i] == edited[i];

                tally.check (exact, "a setStateInformation changing bands 2, 5, 10, 16, 23: " + list (changed));
            }

            p->prepareToPlay (96000.0, 512);
            changed = model->poll();
            tally.check (all24 (changed) && header (96000.0, true), "prepareToPlay at 96 kHz: " + list (changed));

            changed = model->poll();
            tally.check (changed.isEmpty(), "no edit at 96 kHz: " + list (changed));

            p->releaseResources();

            row (tally.bad == 0 && tally.total == 8,
                 "XR-dm polling: before any prepare all 24 and rateKnown false at 48 kHz; the first prepare all 24; no edit -> empty; one band edited -> exactly that band; a setStateInformation (preset load) changing 5 bands -> those 5; prepareToPlay at a new rate -> all 24 and the header's rate",
                 tally.detail());
        }
    }

    //==========================================================================
    // Analyzer rows (PLAN-2.6 P2, P3, P11; RESEARCH-2.6 Q2; tap_bench analyzer)
    //==========================================================================
    const double kAnRates[5] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };

    float maxPoint (const float* points)
    {
        float m = SpectrumAnalyzer::floorDb;

        for (int i = 0; i < SpectrumAnalyzer::numPoints; ++i)
            m = juce::jmax (m, points[i]);

        return m;
    }

    struct AnRead
    {
        double pre = -1000.0, post = -1000.0;
        std::uint64_t framesPre = 0, framesPost = 0;
        std::array<float, SpectrumAnalyzer::numPoints> prePoints {}, postPoints {};
        bool finite = true;
    };

    /** The instance (stereo) prepared at `rate`, analyzerMode 3, a
        SpectrumAnalyzer registered; sampleAt (n) on both channels for
        `seconds`, blocks of 512; analyzer.process (dt) whenever 33 ms of
        audio have passed (dt = those samples / rate). The reading: the
        largest display value of each tap after the last call. */
    template <typename SampleFn>
    AnRead anRun (Processor& p, double rate, double seconds, SampleFn&& sampleAt)
    {
        AnRead r;
        p.analyzerMode.store (3);
        p.prepareToPlay (rate, 512);
        auto analyzer = std::make_unique<SpectrumAnalyzer> (p);

        const int total = (int) std::llround (seconds * rate);
        const int cadence = juce::jmax (1, (int) std::lround (0.033 * rate));
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;
        int since = 0;

        for (int position = 0; position < total; position += 512)
        {
            const int m = juce::jmin (512, total - position);
            block.setSize (2, m, false, false, true);

            for (int i = 0; i < m; ++i)
            {
                const float v = sampleAt (position + i);
                block.setSample (0, i, v);
                block.setSample (1, i, v);
            }

            p.processBlock (block, midi);
            r.finite = r.finite && allFinite (block);
            since += m;

            if (since >= cadence)
            {
                analyzer->process ((double) since / rate);
                since = 0;
            }
        }

        r.pre = maxPoint (analyzer->getPre());
        r.post = maxPoint (analyzer->getPost());
        r.framesPre = analyzer->getFramesComputed (0);
        r.framesPost = analyzer->getFramesComputed (1);
        std::copy (analyzer->getPre(), analyzer->getPre() + SpectrumAnalyzer::numPoints, r.prePoints.begin());
        std::copy (analyzer->getPost(), analyzer->getPost() + SpectrumAnalyzer::numPoints, r.postPoints.begin());

        analyzer.reset();
        p.releaseResources();
        return r;
    }

    AnRead anSine (Processor& p, double rate, double hz, double amplitude, double phase, double seconds = 0.3)
    {
        return anRun (p, rate, seconds, [rate, hz, amplitude, phase] (int n)
        {
            return (float) (amplitude * std::sin (2.0 * eq::pi * hz * (double) n / rate + phase));
        });
    }

    /** The display point whose span [f_p, f_{p+1}) holds hz. */
    int anPointOf (double hz)
    {
        for (int p = 0; p < SpectrumAnalyzer::numPoints; ++p)
            if (SpectrumAnalyzer::pointFrequency (p) <= hz && hz < SpectrumAnalyzer::pointFrequency (p + 1))
                return p;

        return 0;
    }

    /** P3 steps 2 and 4 re-stated (harness code; the plugin's analyzer is not
        used): the newest n samples x the periodic Hann window, zero-padded to
        pad x n, a double FFT (fftRadix2), 20 log10 (|X| 2 / sum (w)) floor
        -200; 256 points over 10 Hz - 30 kHz, f_p = 10 x 3000^(p / 256): the
        largest bin in [f_p, f_{p+1}), else the dB interpolation of the two
        bins around sqrt (f_p f_{p+1}); a span from fs / 2 on reads the floor.
        Returns the largest display value. */
    double anRestatedReading (const std::vector<float>& x, double rate, int n, int pad)
    {
        if ((int) x.size() < n)
            return -1000.0;

        const size_t size = (size_t) (n * pad);
        std::vector<std::complex<double>> buffer (size);
        const size_t offset = x.size() - (size_t) n;
        double windowSum = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (2.0 * eq::pi * (double) i / (double) n);
            windowSum += w;
            buffer[(size_t) i] = (double) x[offset + (size_t) i] * w;
        }

        fftRadix2 (buffer, false);

        const int bins = (int) size / 2 + 1;
        const double binHz = rate / (double) size;
        std::vector<double> db ((size_t) bins);

        for (int k = 0; k < bins; ++k)
            db[(size_t) k] = juce::jmax (-200.0, 20.0 * std::log10 (juce::jmax (std::abs (buffer[(size_t) k]) * 2.0 / windowSum, 1.0e-300)));

        double best = -200.0;

        for (int p = 0; p < 256; ++p)
        {
            const double f0 = 10.0 * std::pow (3000.0, (double) p / 256.0);
            const double f1 = 10.0 * std::pow (3000.0, (double) (p + 1) / 256.0);
            double value = -200.0;

            if (f0 < 0.5 * rate)
            {
                const int k0 = (int) std::ceil (f0 / binHz);
                const int k1 = juce::jmin ((int) std::ceil (f1 / binHz) - 1, bins - 1);

                if (k0 <= k1)
                {
                    value = db[(size_t) k0];

                    for (int k = k0 + 1; k <= k1; ++k)
                        value = juce::jmax (value, db[(size_t) k]);
                }
                else
                {
                    const double c = std::sqrt (f0 * f1) / binHz;
                    const int k = (int) std::floor (c);

                    if (k + 1 < bins)
                        value = (1.0 - (c - (double) k)) * db[(size_t) k] + (c - (double) k) * db[(size_t) k + 1];
                    else
                        value = db[(size_t) bins - 1];
                }
            }

            best = juce::jmax (best, value);
        }

        return best;
    }

    /** Drains everything the tap holds (a consumer of its own, no analyzer). */
    std::vector<float> drainAll (eq::SpectrumTap& tap)
    {
        std::vector<float> out ((size_t) eq::SpectrumTap::capacity);
        std::uint32_t gen = 0, drops = 0;
        const int count = tap.drain (out.data(), eq::SpectrumTap::capacity, gen, drops);
        out.resize ((size_t) juce::jmax (0, count));
        return out;
    }

    /** The held level (dB) of the analyzer's bins within +-3 bins of `cyclesPerSample` (any rate label). */
    double anHeldAt (const SpectrumAnalyzer& analyzer, int tap, double cyclesPerSample)
    {
        const auto& held = analyzer.getHeldBins (tap);
        const int n = analyzer.getFftSize();

        if (held.empty() || n <= 0)
            return -1000.0;

        const int k = (int) std::lround (cyclesPerSample * (double) (n * SpectrumAnalyzer::zeroPadFactor));
        double m = -1000.0;

        for (int i = juce::jmax (0, k - 3); i <= juce::jmin ((int) held.size() - 1, k + 3); ++i)
            m = juce::jmax (m, (double) held[(size_t) i]);

        return m;
    }

    void analyzerRowsXr()
    {
        section ("XR-an / XR-gate / XR-ovf / XR-gen. the analyzer taps and the message-side analyzer (Phase 2.6: UI-03 engine half; PLAN-2.6 P2, P3, P11)");

        const double amplitude = std::pow (10.0, -12.0 / 20.0);

        //======================================================================
        // XR-an, XR-answ
        //======================================================================
        {
            struct Job { int rate = 0; double hz = 1000.0; double phase = 0.0; bool sweep = false; };
            std::vector<Job> jobs;

            for (int r = 0; r < 5; ++r)
                for (int ph = 0; ph < 8; ++ph)
                    jobs.push_back ({ r, 1000.0, (double) ph * eq::pi / 4.0, false });

            for (int r = 0; r < 5; ++r)
                for (int t = 0; t < 50; ++t)
                    jobs.push_back ({ r, std::exp (std::log (20.0) + (std::log (20000.0) - std::log (20.0)) * (double) t / 49.0), 0.0, true });

            std::vector<AnRead> reads (jobs.size());
            auto pool = makePool (2, (int) jobs.size());

            poolFor ((int) jobs.size(), pool, [&jobs, &reads, amplitude] (int j, Processor& p, const Handles& h)
            {
                resetAllRaw (p, h);
                const Job& job = jobs[(size_t) j];
                reads[(size_t) j] = anSine (p, kAnRates[job.rate], job.hz, amplitude, job.phase);
            });

            Tally an, sweep;
            double lowAn[5], highAn[5], lowSweep = 1000.0, highSweep = -1000.0;
            juce::String sweepWhere;

            for (int r = 0; r < 5; ++r)
            {
                lowAn[r] = 1000.0;
                highAn[r] = -1000.0;
            }

            for (size_t j = 0; j < jobs.size(); ++j)
            {
                const AnRead& q = reads[j];
                const Job& job = jobs[j];
                const bool framed = q.framesPre > 0 && q.framesPost > 0 && q.finite;
                const juce::String where = fmtG (kAnRates[job.rate] / 1000.0, 4) + " kHz " + fmtG (job.hz, 5) + " Hz";

                if (! job.sweep)
                {
                    lowAn[job.rate] = juce::jmin (lowAn[job.rate], q.pre, q.post);
                    highAn[job.rate] = juce::jmax (highAn[job.rate], q.pre, q.post);
                    an.check (framed && std::abs (q.pre + 12.0) <= 0.5 && std::abs (q.post + 12.0) <= 0.5,
                              where + ": pre " + fmtG (q.pre, 5) + ", post " + fmtG (q.post, 5) + " dB" + (framed ? "" : " (no frame)"));
                }
                else
                {
                    if (! (juce::jmin (q.pre, q.post) >= lowSweep))
                        sweepWhere = where;

                    lowSweep = juce::jmin (lowSweep, q.pre, q.post);
                    highSweep = juce::jmax (highSweep, q.pre, q.post);
                    sweep.check (framed && std::abs (q.pre + 12.0) <= 0.25 && std::abs (q.post + 12.0) <= 0.25,
                                 where + ": pre " + fmtG (q.pre, 5) + ", post " + fmtG (q.post, 5) + " dB");
                }
            }

            juce::String figures;

            for (int r = 0; r < 5; ++r)
                figures += (r == 0 ? "" : ", ") + fmtG (kAnRates[r] / 1000.0, 4) + " kHz " + fmtG (lowAn[r], 5) + " .. " + fmtG (highAn[r], 5);

            row (an.bad == 0 && an.total == 40,
                 "XR-an -12 dBFS 1 kHz sine on both channels, 44.1 / 48 / 88.2 / 96 / 192 kHz, 8 phases, pre and post (no band), drained once per 33 ms of audio: the largest display value -12 +- 0.5 dB",
                 an.detail() + "  worst over phases and taps: " + figures + " dB (bench -12.08 / -12.04 / -12.04 at 44.1 / 96 / 192)");

            row (sweep.bad == 0 && sweep.total == 250,
                 "XR-answ 50 log tones 20 Hz - 20 kHz at -12 dBFS, the five rates, pre and post: within +-0.25 dB of -12",
                 sweep.detail() + "  " + fmtG (lowSweep, 5) + " ... " + fmtG (highSweep, 5) + " dB (lowest at " + sweepWhere + "; bench -12.11 ... -11.99)");
        }

        //======================================================================
        // XR-ann: zero-pad 1x, re-stated in the harness, at 96 kHz
        //======================================================================
        {
            const double rate = 96000.0;
            const int n = SpectrumAnalyzer::fftSizeForRate (rate);
            double worst1 = 1000.0, worst4 = 1000.0;
            bool drained = true;

            auto p = makeBands (2, 0, mixedSpec);
            const Handles h = handlesOf (*p);

            for (int ph = 0; ph < 8; ++ph)
            {
                resetAllRaw (*p, h);
                p->setAnalyzerConsumer (true);
                p->analyzerMode.store (3);

                const int length = 3 * n;
                juce::AudioBuffer<float> input (2, length);

                for (int i = 0; i < length; ++i)
                {
                    const auto v = (float) (amplitude * std::sin (2.0 * eq::pi * 1000.0 * (double) i / rate + (double) ph * eq::pi / 4.0));
                    input.setSample (0, i, v);
                    input.setSample (1, i, v);
                }

                renderAt (*p, input, rate, 512, {}, noHook);
                const auto x = drainAll (p->getPostTap());
                drained = drained && (int) x.size() == length;

                worst1 = juce::jmin (worst1, anRestatedReading (x, rate, n, 1));
                worst4 = juce::jmin (worst4, anRestatedReading (x, rate, n, 4));
                p->setAnalyzerConsumer (false);
                p->releaseResources();
            }

            row (drained && worst1 < -12.5,
                 "XR-ann the XR-an stimulus at 96 kHz through P3 step 2 re-stated in the harness with zero-pad 1x (the plugin's analyzer untouched): the reading falls below -12.5 dB (control fires)",
                 "1x " + fmtG (worst1, 5) + " dB (bench -12.63); the same restatement at 4x " + fmtG (worst4, 5) + " dB (bench -12.04)"
                     + (drained ? "" : "; THE POST TAP DID NOT HOLD THE RENDER"));
        }

        //======================================================================
        // XR-anpp, XR-anrel
        //======================================================================
        {
            auto p = makeBands (2, 0, mixedSpec);
            const Handles h = handlesOf (*p);
            const double rate = 48000.0;

            resetAllRaw (*p, h);
            storeRaw (h.band[0], rawBandSpec (makeSpec (kBell, 1000.0f, 12.0f, 1.0f, 1, kStereo)));
            const AnRead q = anSine (*p, rate, 1000.0, std::pow (10.0, -24.0 / 20.0), 0.3);
            const int point = anPointOf (1000.0);
            const double diff = (double) q.postPoints[(size_t) point] - (double) q.prePoints[(size_t) point];

            row (q.framesPre > 0 && q.framesPost > 0 && std::abs (diff - 12.0) <= 0.5,
                 "XR-anpp +12 dB Bell 1 kHz Q 1, a -24 dBFS 1 kHz sine, 48 kHz: post - pre at the display point holding 1 kHz is +12 +- 0.5 dB",
                 "post " + fmtG (q.postPoints[(size_t) point], 5) + ", pre " + fmtG (q.prePoints[(size_t) point], 5) + " dB: " + fmtG (diff, 5)
                     + " dB (point " + juce::String (point) + ", " + fmtG (SpectrumAnalyzer::pointFrequency (point), 5) + " - "
                     + fmtG (SpectrumAnalyzer::pointFrequency (point + 1), 5) + " Hz)");

            // XR-anrel: 0.5 s of the sine, drained every 2048 samples; the
            // host stops right after a drain: 61 calls of process (0.033)
            // with no new data; then the sine continues (phase-continuous)
            // and the next process call follows 2048 samples later.
            resetAllRaw (*p, h);
            p->analyzerMode.store (3);
            p->prepareToPlay (rate, 512);
            auto analyzer = std::make_unique<SpectrumAnalyzer> (*p);

            juce::AudioBuffer<float> block (2, 512);
            juce::MidiBuffer midi;
            int clock = 0;

            const auto play = [&] (int samples)
            {
                for (int done = 0; done < samples; done += 512)
                {
                    for (int i = 0; i < 512; ++i)
                    {
                        const auto v = (float) (amplitude * std::sin (2.0 * eq::pi * 1000.0 * (double) (clock + i) / rate));
                        block.setSample (0, i, v);
                        block.setSample (1, i, v);
                    }

                    p->processBlock (block, midi);
                    clock += 512;
                }
            };

            for (int k = 0; k < 12; ++k)
            {
                play (2048);
                analyzer->process (2048.0 / rate);
            }

            const double start = juce::jmax ((double) maxPoint (analyzer->getPre()), (double) maxPoint (analyzer->getPost()));
            const std::uint64_t framesBefore = analyzer->getFramesComputed (1);
            double worstFall = 0.0;

            for (int k = 1; k <= 61; ++k)
            {
                analyzer->process (0.033);

                for (int tap = 0; tap < 2; ++tap)
                {
                    const double reading = (double) maxPoint (tap == 0 ? analyzer->getPre() : analyzer->getPost());
                    worstFall = juce::jmax (worstFall, std::abs (reading - (start - 20.0 * 0.033 * (double) k)));
                }
            }

            const double fallen = (double) maxPoint (analyzer->getPost());
            const bool noFrame = analyzer->getFramesComputed (1) == framesBefore;

            play (2048);
            analyzer->process (2048.0 / rate);
            const double back = juce::jmin ((double) maxPoint (analyzer->getPre()), (double) maxPoint (analyzer->getPost()));

            analyzer.reset();
            p->releaseResources();

            row (std::abs (start + 12.0) <= 0.5 && noFrame && worstFall <= 1.0 && std::abs (back - start) <= 1.0,
                 "XR-anrel the host stops calling processBlock: process (0.033) keeps falling at 20 dB/s (within +-1 dB over 2 s, both taps); instant attack on re-entry (the next frame reads the held value again, +-1 dB)",
                 "held " + fmtG (start, 5) + " dB; after 61 calls (2.01 s) " + fmtG (fallen, 5) + " dB (expected " + fmtG (start - 20.0 * 0.033 * 61.0, 5)
                     + "), worst deviation " + fmtG (worstFall, 3) + " dB; re-entry " + fmtG (back, 5) + " dB" + (noFrame ? "" : "; A FRAME WAS COMPUTED WITH NO DATA"));
        }

        //======================================================================
        // XR-gate
        //======================================================================
        {
            Tally tally;
            const double rate = 48000.0;
            const auto noise = makeNoiseAt (2, 512, 0x6A7E0002, kNoise12);

            const auto blockDelta = [&noise] (Processor& p, bool viaBypassed, int& pre, int& post)
            {
                juce::AudioBuffer<float> block (noise);
                juce::MidiBuffer midi;
                const std::uint32_t a = p.getPreTap().getWriteIndex(), b = p.getPostTap().getWriteIndex();

                if (viaBypassed)
                    p.processBlockBypassed (block, midi);
                else
                    p.processBlock (block, midi);

                pre = (int) (p.getPreTap().getWriteIndex() - a);
                post = (int) (p.getPostTap().getWriteIndex() - b);
            };

            struct Case { const char* name; bool consumer; int mode; int path; int wantPre; int wantPost; };   // path 0 processed, 1 bypass parameter, 2 processBlockBypassed, 3 untouched (no band)
            const Case cases[] = {
                { "no consumer, mode 3", false, 3, 0, 0, 0 },     { "no consumer, mode 2 (default)", false, 2, 0, 0, 0 },
                { "consumer, mode 0 (Off)", true, 0, 0, 0, 0 },   { "consumer, mode 1 (Pre)", true, 1, 0, 512, 0 },
                { "consumer, mode 2 (Post)", true, 2, 0, 0, 512 }, { "consumer, mode 3 (both)", true, 3, 0, 512, 512 },
                { "consumer, mode 3, bypass parameter", true, 3, 1, 512, 512 }, { "consumer, mode 3, processBlockBypassed", true, 3, 2, 512, 512 },
                { "consumer, mode 3, no band (untouched)", true, 3, 3, 512, 512 }
            };

            auto p = makeBands (2, 0, mixedSpec);
            const Handles h = handlesOf (*p);
            juce::String figures;

            for (const auto& c : cases)
            {
                resetAllRaw (*p, h);

                if (c.path != 3)
                    storeRaw (h.band[0], rawBandSpec (makeSpec (kBell, 1000.0f, 6.0f, 1.0f, 1, kStereo)));

                if (c.path == 1)
                    h.bypass->store (1.0f);

                p->setAnalyzerConsumer (c.consumer);
                p->analyzerMode.store (c.mode);
                p->prepareToPlay (rate, 512);

                int pre = -1, post = -1, pre2 = -1, post2 = -1;
                blockDelta (*p, c.path == 2, pre, post);
                blockDelta (*p, c.path == 2, pre2, post2);       // a second block (after a bypass fade has ended too)

                tally.check (pre == c.wantPre && post == c.wantPost && pre2 == c.wantPre && post2 == c.wantPost,
                             juce::String (c.name) + ": pre / post moved " + juce::String (pre) + " / " + juce::String (post) + ", then "
                                 + juce::String (pre2) + " / " + juce::String (post2));
                p->releaseResources();
            }

            // Unprepared: a fresh instance, consumer and mode 3, never prepared.
            {
                auto fresh = makeBands (2, 0, mixedSpec);
                fresh->setAnalyzerConsumer (true);
                fresh->analyzerMode.store (3);
                int pre = -1, post = -1;
                blockDelta (*fresh, false, pre, post);
                tally.check (pre == 0 && post == 0, "unprepared: pre / post moved " + juce::String (pre) + " / " + juce::String (post));
            }

            // R9: a restored analyzerMode (the UI child of a saved state) is the gate.
            {
                resetAllRaw (*p, h);
                auto xml = xmlOf (stateOf (*p));
                bool restored = false;

                if (xml != nullptr)
                    if (auto* ui = xml->getChildByName ("UI"))
                    {
                        ui->setAttribute ("analyzerMode", 1);
                        loadState (*p, blobOf (*xml));
                        restored = p->analyzerMode.load() == 1;
                    }

                p->setAnalyzerConsumer (true);
                p->prepareToPlay (rate, 512);
                int pre = -1, post = -1;
                blockDelta (*p, false, pre, post);
                tally.check (restored && pre == 512 && post == 0,
                             "analyzerMode 1 restored from a state: " + juce::String (restored ? "restored" : "NOT RESTORED") + ", pre / post moved "
                                 + juce::String (pre) + " / " + juce::String (post));
                p->releaseResources();
            }

            row (tally.bad == 0 && tally.total == 11,
                 "XR-gate the tap writes: mode 0, or no consumer -> neither tap's write index moves; mode 1 / 2 move only their tap; mode 3 both, on the processed, bypassed (both routes) and untouched paths; unprepared -> nothing; a restored analyzerMode gates the same way (R9)",
                 tally.detail());
        }

        //======================================================================
        // XR-ovf (PLAN-2.6 execute, Taylor's call 2026-11-15: the row reads
        // P2, the ring as built; the processor offers each block's last
        // min (n, capacity) samples only)
        //======================================================================
        {
            Tally tally;
            const double rate = 48000.0;
            const int total = (int) (2.0 * rate);
            const int fftN = SpectrumAnalyzer::fftSizeForRate (rate);
            juce::String figures;

            // Noise, L and R different, so the mono sum is a real sum.
            const auto noise = makeNoiseAt (2, total, 0x0FF10002, kNoise12);
            std::vector<float> mono ((size_t) total);

            for (int i = 0; i < total; ++i)
                mono[(size_t) i] = 0.5f * (noise.getSample (0, i) + noise.getSample (1, i));

            for (const int blockSize : { 32, 4096, 65536 })
            {
                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                resetAllRaw (*p, h);
                p->setAnalyzerConsumer (true);
                p->analyzerMode.store (3);
                p->prepareToPlay (rate, juce::jmin (blockSize, 4096));

                // The harness's model of P2 plus the push rule: each block offers its last
                // min (n, C) samples, the ring keeps the oldest run that fits since the last drain.
                std::vector<float> expected;
                std::uint64_t offered = 0;
                int blocks = 0, returned = 0;
                double longest = 0.0;
                juce::AudioBuffer<float> block (2, blockSize);
                juce::MidiBuffer midi;
                const std::uint32_t dropsBefore = p->getPostTap().getDropCount();

                for (int position = 0; position < total; position += blockSize)
                {
                    const int n = juce::jmin (blockSize, total - position);
                    block.setSize (2, n, false, false, true);

                    for (int channel = 0; channel < 2; ++channel)
                        block.copyFrom (channel, 0, noise, channel, position, n);

                    const int from = juce::jmax (0, n - eq::SpectrumTap::capacity);
                    offered += (std::uint64_t) (n - from);

                    for (int i = from; i < n && (int) expected.size() < eq::SpectrumTap::capacity; ++i)
                        expected.push_back (mono[(size_t) (position + i)]);

                    ++blocks;
                    const auto t0 = std::chrono::steady_clock::now();
                    p->processBlock (block, midi);
                    const auto t1 = std::chrono::steady_clock::now();
                    ++returned;
                    longest = juce::jmax (longest, std::chrono::duration<double, std::milli> (t1 - t0).count());
                }

                const int keptPre = p->getPreTap().getNumAvailable(), keptPost = p->getPostTap().getNumAvailable();
                const std::uint32_t dropsPre = p->getPreTap().getDropCount(), dropsPost = p->getPostTap().getDropCount() - dropsBefore;
                const bool dropsExact = (std::uint64_t) dropsPre == offered - (std::uint64_t) keptPre && (std::uint64_t) dropsPost == offered - (std::uint64_t) keptPost;

                const auto held = drainAll (p->getPostTap());
                bool content = held.size() == expected.size() && ! held.empty();

                for (size_t i = 0; i < held.size() && content; ++i)
                    content = sameBits (held[i], expected[i]);

                // A single 65 536 block into an empty ring: exactly its last 32 768, the input tail's mono sum.
                bool tail = true;

                if (blockSize == 65536)
                {
                    tail = held.size() == (size_t) eq::SpectrumTap::capacity;

                    for (size_t i = 0; i < held.size() && tail; ++i)
                        tail = sameBits (held[i], mono[(size_t) (65536 - eq::SpectrumTap::capacity) + i]);
                }

                tally.check (returned == blocks && dropsExact && content && tail && dropsPre > 0,
                             "block " + juce::String (blockSize) + ": drops " + juce::String ((juce::int64) dropsPre) + " / " + juce::String ((juce::int64) dropsPost)
                                 + ", offered " + juce::String ((juce::int64) offered) + ", kept " + juce::String (keptPre) + " / " + juce::String (keptPost)
                                 + (content ? "" : ", the ring is not the oldest fitting run") + (tail ? "" : ", not the 65 536 block's last 32 768"));

                figures += (figures.isEmpty() ? "" : "; ") + juce::String ("block ") + juce::String (blockSize) + ": offered " + juce::String ((juce::int64) offered)
                         + ", kept " + juce::String (keptPre) + ", drops " + juce::String ((juce::int64) dropsPre) + " (exact), ring = oldest fitting run"
                         + (blockSize == 65536 ? " = the block's last 32 768" : "") + ", longest call " + fmtG (longest, 3) + " ms";
                p->releaseResources();
            }

            // The reading: an analyzer registered through the fill, called
            // once before it and not during it (a stalled timer); then up to
            // three process calls, each after fftN fresh samples of the sine.
            for (const int blockSize : { 32, 4096, 65536 })
            {
                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                resetAllRaw (*p, h);
                p->analyzerMode.store (3);
                p->prepareToPlay (rate, juce::jmin (blockSize, 4096));
                auto analyzer = std::make_unique<SpectrumAnalyzer> (*p);
                analyzer->process (0.033);

                juce::AudioBuffer<float> block (2, blockSize);
                juce::MidiBuffer midi;
                int position = 0;

                const auto play = [&] (int samples, int size)
                {
                    for (int done = 0; done < samples;)
                    {
                        const int n = juce::jmin (size, samples - done);
                        block.setSize (2, n, false, false, true);

                        for (int i = 0; i < n; ++i)
                        {
                            const auto v = (float) (amplitude * std::sin (2.0 * eq::pi * 1000.0 * (double) (position + i) / rate));
                            block.setSample (0, i, v);
                            block.setSample (1, i, v);
                        }

                        p->processBlock (block, midi);
                        position += n;
                        done += n;
                    }
                };

                play (total, blockSize);

                int readAt = 0;
                double reading = -1000.0;

                for (int call = 1; call <= 3 && readAt == 0; ++call)
                {
                    play (fftN, juce::jmin (blockSize, fftN));
                    analyzer->process ((double) fftN / rate);
                    const double r = juce::jmax ((double) maxPoint (analyzer->getPre()), (double) maxPoint (analyzer->getPost()));

                    if (analyzer->getFramesComputed (0) > 0 && analyzer->getFramesComputed (1) > 0)
                    {
                        readAt = call;
                        reading = r;
                    }
                }

                tally.check (readAt > 0 && std::abs (reading + 12.0) <= 0.5,
                             "block " + juce::String (blockSize) + ": " + (readAt > 0 ? "process #" + juce::String (readAt) + " reads " + fmtG (reading, 5) + " dB"
                                                                                    : juce::String ("no frame in 3 process calls")));

                figures += "; block " + juce::String (blockSize) + ": the reading at process #" + juce::String (readAt) + ", " + fmtG (reading, 5) + " dB";
                analyzer.reset();
                p->releaseResources();
            }

            row (tally.bad == 0 && tally.total == 6,
                 "XR-ovf (P2; Taylor's call 2026-11-15) no drain for 2 s at blocks 32 / 4096 / 65 536 (stereo, 48 kHz; the 65 536 block through processBlock on a 4096 prepare): every block returns; the drop count is exact (offered - kept); the ring holds the oldest fitting run since the last drain, and for the single 65 536 block into an empty ring its last 32 768 (bit-exact vs the input's mono sum); after the stale run is discarded the analyzer reads -12 +- 0.5 dB within 3 process calls (N fresh samples before each)",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-gen
        //======================================================================
        {
            Tally tally;
            const double oldCycles = 1000.0 / 48000.0;      // the old stream's tone, in cycles per sample
            juce::String figures;

            // (i) one thread: 48 kHz 1 kHz, drained; more 48 kHz left in the
            // ring; prepareToPlay at 96 kHz; 96 kHz 5 kHz, drained.
            {
                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                resetAllRaw (*p, h);
                p->analyzerMode.store (3);
                p->prepareToPlay (48000.0, 512);
                auto analyzer = std::make_unique<SpectrumAnalyzer> (*p);

                juce::AudioBuffer<float> block (2, 512);
                juce::MidiBuffer midi;
                int oldFramesWithTone = 0, newFrames = 0, newFramesWithOld = 0;
                double worstNewOld = -1000.0;

                const auto play = [&] (double rate, double hz, int samples, bool drain, bool afterReset)
                {
                    int since = 0;

                    for (int position = 0; position < samples; position += 512)
                    {
                        for (int i = 0; i < 512; ++i)
                        {
                            const auto v = (float) (amplitude * std::sin (2.0 * eq::pi * hz * (double) (position + i) / rate));
                            block.setSample (0, i, v);
                            block.setSample (1, i, v);
                        }

                        p->processBlock (block, midi);
                        since += 512;

                        if (drain && since >= (int) std::lround (0.033 * rate))
                        {
                            const std::uint64_t before = analyzer->getFramesComputed (0) + analyzer->getFramesComputed (1);
                            analyzer->process (10.0);
                            since = 0;

                            if (analyzer->getFramesComputed (0) + analyzer->getFramesComputed (1) == before)
                                continue;

                            const double level = juce::jmax (anHeldAt (*analyzer, 0, oldCycles), anHeldAt (*analyzer, 1, oldCycles));

                            if (! afterReset)
                            {
                                oldFramesWithTone += level > -100.0 ? 1 : 0;
                            }
                            else
                            {
                                ++newFrames;
                                worstNewOld = juce::jmax (worstNewOld, level);
                                newFramesWithOld += level > -100.0 ? 1 : 0;
                            }
                        }
                    }
                };

                play (48000.0, 1000.0, 14848, true, false);
                const int preparesBefore = analyzer->getPrepareCount();
                play (48000.0, 1000.0, 4608, false, false);         // left in the ring
                p->prepareToPlay (96000.0, 512);
                play (96000.0, 5000.0, 28672, true, true);

                const double reading = juce::jmax ((double) maxPoint (analyzer->getPre()), (double) maxPoint (analyzer->getPost()));
                const bool resized = analyzer->getFftSize() == 8192 && analyzer->getPreparedRate() == 96000.0
                                     && analyzer->getPrepareCount() == preparesBefore + 1;

                tally.check (oldFramesWithTone > 0 && newFrames > 0 && newFramesWithOld == 0 && resized && std::abs (reading + 12.0) <= 0.5,
                             "one thread: " + juce::String (newFramesWithOld) + " of " + juce::String (newFrames) + " frames after the prepare hold the old tone");

                figures += "one thread: old-rate frames holding the 1 kHz tone " + juce::String (oldFramesWithTone) + " (the check sees it); after the prepare "
                         + juce::String (newFrames) + " frames, the old tone's bins at most " + fmtG (worstNewOld, 4) + " dB; re-prepared at N = "
                         + juce::String (analyzer->getFftSize()) + ", " + fmtG (analyzer->getPreparedRate(), 6) + " Hz (" + juce::String (analyzer->getPrepareCount())
                         + " prepares); 5 kHz reads " + fmtG (reading, 5) + " dB";

                analyzer.reset();
                p->releaseResources();
            }

            // (ii) the analyzer drained by a second thread throughout; the
            // prepare lands between two of its drains.
            {
                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                resetAllRaw (*p, h);
                p->analyzerMode.store (3);
                p->prepareToPlay (48000.0, 512);
                auto analyzer = std::make_unique<SpectrumAnalyzer> (*p);
                const std::uint32_t genOld = p->getPreTap().getGeneration();

                std::atomic<bool> stop { false };
                int framesOld = 0, framesOldWithTone = 0, framesNew = 0, framesNewWithOld = 0, calls = 0;
                double worstNew = -1000.0;

                std::thread reader ([&] ()
                {
                    std::uint64_t seen = 0;

                    while (! stop.load (std::memory_order_acquire))
                    {
                        analyzer->process (10.0);
                        ++calls;
                        const std::uint64_t frames = analyzer->getFramesComputed (0);

                        if (frames == seen)
                            continue;

                        seen = frames;
                        const double level = anHeldAt (*analyzer, 0, oldCycles);

                        if (analyzer->getGenerationSeen (0) == genOld)
                        {
                            ++framesOld;
                            framesOldWithTone += level > -100.0 ? 1 : 0;
                        }
                        else
                        {
                            ++framesNew;
                            worstNew = juce::jmax (worstNew, level);
                            framesNewWithOld += level > -100.0 ? 1 : 0;
                        }
                    }
                });

                juce::AudioBuffer<float> block (2, 512);
                juce::MidiBuffer midi;

                const auto play = [&] (double rate, double hz, int samples)
                {
                    for (int position = 0; position < samples; position += 512)
                    {
                        for (int i = 0; i < 512; ++i)
                        {
                            const auto v = (float) (amplitude * std::sin (2.0 * eq::pi * hz * (double) (position + i) / rate));
                            block.setSample (0, i, v);
                            block.setSample (1, i, v);
                        }

                        p->processBlock (block, midi);
                        std::this_thread::sleep_for (std::chrono::microseconds (500));
                    }
                };

                play (48000.0, 1000.0, 48128);
                p->prepareToPlay (96000.0, 512);
                play (96000.0, 5000.0, 96256);

                for (int wait = 0; wait < 2000 && (p->getPreTap().getNumAvailable() > 0 || p->getPostTap().getNumAvailable() > 0); ++wait)
                    std::this_thread::sleep_for (std::chrono::milliseconds (1));

                std::this_thread::sleep_for (std::chrono::milliseconds (20));
                stop.store (true, std::memory_order_release);
                reader.join();

                tally.check (framesOld > 0 && framesOldWithTone > 0 && framesNew > 0 && framesNewWithOld == 0
                                 && analyzer->getFftSize() == 8192 && analyzer->getPreparedRate() == 96000.0,
                             "second thread: " + juce::String (framesNewWithOld) + " of " + juce::String (framesNew) + " frames after the prepare hold the old tone");

                figures += "; second thread (" + juce::String (calls) + " process calls): old-generation frames " + juce::String (framesOld) + " ("
                         + juce::String (framesOldWithTone) + " with the 1 kHz tone), new-generation frames " + juce::String (framesNew)
                         + ", the old tone's bins at most " + fmtG (worstNew, 4) + " dB; N = " + juce::String (analyzer->getFftSize());

                analyzer.reset();
                p->releaseResources();
            }

            row (tally.bad == 0 && tally.total == 2,
                 "XR-gen prepareToPlay at a new rate between two drains (one thread; and drained concurrently by a second thread): no sample of the old rate reaches a transform (no frame after the generation moved holds the old 1 kHz tone above -100 dB); the analyzer re-prepares at the new size",
                 tally.detail() + "  " + figures);
        }
    }

    //==========================================================================
    // Listen (PLAN-2.6 P4 - P6, P10, P12, P13; RESEARCH-2.6 Q3; tap_bench
    // listen, listen3)
    //==========================================================================
    struct LrCase
    {
        const char* name;
        int type;
        float freq, gain, q;
        int slope;
    };

    // tap_bench listen2 / listen3 (the designer harness's XL-on set): the 20 listened bands.
    constexpr int kLrCaseCount = 20;
    const LrCase kLrCases[kLrCaseCount] =
    {
        { "Bell 1 kHz +12 Q 2",      kBell,      1000.0f,  12.0f,  2.0f, 0 },
        { "Bell 50 Hz -18 Q 4",      kBell,        50.0f, -18.0f,  4.0f, 0 },
        { "Bell 5 kHz +6 Q 40",      kBell,      5000.0f,   6.0f, 40.0f, 0 },
        { "Bell 10 Hz +12 Q 40",     kBell,        10.0f,  12.0f, 40.0f, 0 },
        { "Low Shelf 200 Hz +12",    kLowShelf,   200.0f,  12.0f,  1.0f, 2 },
        { "High Shelf 8 kHz -12",    kHighShelf, 8000.0f, -12.0f,  1.0f, 1 },
        { "Low Cut 100 Hz 48 dB",    kLowCut,     100.0f,   0.0f,  1.0f, 4 },
        { "High Cut 2 kHz 96 dB",    kHighCut,   2000.0f,   0.0f,  1.0f, 7 },
        { "Notch 50 Hz Q 10",        kNotch,       50.0f,   0.0f, 10.0f, 0 },
        { "Band Pass 1 kHz Q 10",    kBandPass,  1000.0f,   0.0f, 10.0f, 0 },
        { "Band Pass 8 kHz Q 40",    kBandPass,  8000.0f,   0.0f, 40.0f, 0 },
        { "Band Pass 200 Hz Q 0.3",  kBandPass,   200.0f,   0.0f,  0.3f, 0 },
        { "Tilt 1 kHz +12",          kTilt,      1000.0f,  12.0f,  1.0f, 1 },
        { "Bell 2 kHz +12 Q 10",     kBell,      2000.0f,  12.0f, 10.0f, 0 },
        { "Band Pass 300 Hz Q 40",   kBandPass,   300.0f,   0.0f, 40.0f, 0 },
        { "Band Pass 150 Hz Q 20",   kBandPass,   150.0f,   0.0f, 20.0f, 0 },
        { "Bell 400 Hz +12 Q 40",    kBell,       400.0f,  12.0f, 40.0f, 0 },
        { "Band Pass 3 kHz Q 40",    kBandPass,  3000.0f,   0.0f, 40.0f, 0 },
        { "Low Shelf 20 Hz +12",     kLowShelf,    20.0f,  12.0f,  1.0f, 1 },
        { "High Cut 120 Hz 12 dB",   kHighCut,    120.0f,   0.0f,  1.0f, 1 }
    };

    const double kLrTones[2] = { 50.0, 1000.0 };
    constexpr int kLrPhases = 4;

    eq::RawBand lrRaw (const LrCase& c, int placement = kStereo)
    {
        return rawBandSpec (makeSpec (c.type, c.freq, c.gain, c.q, c.slope, placement));
    }

    /** D11 re-stated (harness code; not eq::listenInputFor): Bell / Notch /
        Band Pass -> Band Pass (f, Q); Low Shelf / Low Cut -> High Cut,
        slope index 1, Q 1; High Shelf / High Cut -> Low Cut, slope index 1,
        Q 1; Tilt -> Band Pass, Q 0.7. Gain 0, active, the band's (effective)
        placement. */
    eq::BandInput lrMap (const eq::BandInput& band)
    {
        eq::BandInput out {};
        out.placement = band.placement;
        out.active = true;
        out.settings.freqHz = band.settings.freqHz;
        out.settings.gainDbEff = 0.0;
        out.settings.type = eq::Type::bandPass;
        out.settings.q = band.settings.q;
        out.settings.slopeIndex = 0;

        switch (band.settings.type)
        {
            case eq::Type::lowShelf:
            case eq::Type::lowCut:
                out.settings.type = eq::Type::highCut;
                out.settings.q = 1.0;
                out.settings.slopeIndex = 1;
                break;

            case eq::Type::highShelf:
            case eq::Type::highCut:
                out.settings.type = eq::Type::lowCut;
                out.settings.q = 1.0;
                out.settings.slopeIndex = 1;
                break;

            case eq::Type::tilt:
                out.settings.q = 0.7;
                break;

            case eq::Type::bell:
            case eq::Type::notch:
            case eq::Type::bandPass:
                break;
        }

        return out;
    }

    /** tap_bench listen / listen3 timeline, at the rate. */
    struct LrTimes
    {
        int total = 0, tOn = 0, tOff = 0, pre = 0, postOn = 0, postOff = 0, ramp = 1, fade = 1;
    };

    LrTimes lrTimes (double rate)
    {
        LrTimes t;
        t.total = (int) (1.5 * rate);
        t.tOn = (int) (0.5 * rate) + 7;
        t.tOff = (int) (1.0 * rate) + 11;
        t.pre = (int) (0.050 * rate);
        t.ramp = juce::jmax (1, (int) std::lround (0.030 * rate));
        t.fade = refFadeLength (rate);
        t.postOn = (int) (0.070 * rate) + t.ramp;
        t.postOff = (int) (0.070 * rate);
        return t;
    }

    /** The tone at -6 dBFS on both channels (tap_bench: 0.5 sin (2 pi f n / fs + phase pi / 4)). */
    juce::AudioBuffer<float> lrTone (double rate, double hz, int phase, int length)
    {
        juce::AudioBuffer<float> b (2, length);

        for (int i = 0; i < length; ++i)
        {
            const auto v = (float) (0.5 * std::sin (2.0 * eq::pi * hz * (double) i / rate + (double) phase * eq::pi / 4.0));
            b.setSample (0, i, v);
            b.setSample (1, i, v);
        }

        return b;
    }

    /** The listenBand value read at the host block starting at `at`, and its mapped input. */
    struct LrWish
    {
        int at = 0;
        int index = -1;
        eq::BandInput mapped {};
    };

    struct LrOptions
    {
        int ramp = -1;              // R; -1: max (1, round (0.030 fs))
        int fade = -1;              // N; -1: round (0.020 fs)
        int weightLead = 0;         // the weight starts this many samples before R (a control)
        double ag = 1.0, og = 1.0;  // the gains, re-stated as constants
    };

    /** PLAN-2.6 P12: listen per P5 / P6 re-stated per sample, both lanes.
        The wish read at each block start (`wishes`), taken at the next tick
        (the clock is the sample index from 0, the grid re-stated); engage:
        the voice from rest, its input x the ramp weight for R samples, then
        e + w (l - e) over N; change: B from rest with its ramp, A
        meanwhile, then a + w (b - a); release: a + w (e - a) over N; a
        request during a move waits for its end and starts the next move on
        the next sample. e = ag x the EQ path; the output is og x the result.
        A seeded index starts listening at sample 0, its ramp complete.
        Voices are static (RefChain), so a continuous (f / Q) change is not
        re-stated: *unsupported is set when one is met. */
    std::array<std::vector<double>, 2> listenRefRender (double rate, bool stereo,
                                                        const std::vector<double>& dryL, const std::vector<double>& dryR,
                                                        const std::vector<double>& eqL, const std::vector<double>& eqR,
                                                        int seedIndex, const eq::BandInput& seedInput,
                                                        const std::vector<LrWish>& wishes, const LrOptions& options,
                                                        bool* unsupported = nullptr)
    {
        const size_t length = dryL.size();
        const size_t grid = (size_t) harnessGrid (rate);
        const int rampLength = options.ramp >= 0 ? options.ramp : juce::jmax (1, (int) std::lround (0.030 * rate));
        const int fadeLength = options.fade > 0 ? options.fade : refFadeLength (rate);
        const int weightStart = rampLength - options.weightLead;
        const auto designer = designerFor (rate);

        struct Voice
        {
            std::unique_ptr<RefChain> chain = std::make_unique<RefChain>();
            eq::BandInput input {};
            int rampPos = 0;
            bool live = false;
        };

        enum class Move { none, engage, change, release };

        std::array<Voice, 2> voice;
        int active = 0, target = -1, movePos = 0, moveLength = 0;
        Move move = Move::none;
        int wishIndex = -1, desiredIndex = -1;
        eq::BandInput wishInput {}, desiredInput {};
        size_t nextWish = 0;
        bool notRestated = false;

        const auto configure = [&voice, &designer] (int v, const eq::BandInput& in)
        {
            Voice& vc = voice[(size_t) v];
            *vc.chain = RefChain();
            prepareRefChain (*vc.chain, *designer, in);
            vc.input = in;
            vc.rampPos = 0;
            vc.live = true;
        };

        const auto retire = [&voice] (int v)
        {
            Voice& vc = voice[(size_t) v];
            *vc.chain = RefChain();
            vc.rampPos = 0;
            vc.live = false;
        };

        const auto startMove = [&move, &movePos, &moveLength, rampLength, fadeLength] (Move kind)
        {
            move = kind;
            movePos = 0;
            moveLength = (kind == Move::release) ? fadeLength : rampLength + fadeLength;
        };

        const auto settle = [&] ()
        {
            if (desiredIndex < 0)
            {
                if (target >= 0)
                {
                    target = -1;
                    startMove (Move::release);
                }

                return;
            }

            if (target < 0)
            {
                configure (active, desiredInput);
                target = desiredIndex;
                startMove (Move::engage);
                return;
            }

            if (desiredIndex != target)
            {
                configure (1 - active, desiredInput);
                target = desiredIndex;
                startMove (Move::change);
                return;
            }

            const eq::BandChange change = eq::classifyBandChange (voice[(size_t) active].input, desiredInput);

            if (change == eq::BandChange::structural)
            {
                configure (1 - active, desiredInput);
                startMove (Move::change);
            }
            else if (change == eq::BandChange::continuous)
            {
                notRestated = true;
            }
        };

        // One sample of voice v through the solo router (P5), its input x the ramp weight.
        const auto voiceOut = [&voice, rampLength, stereo] (int v, double dl, double dr, double& yl, double& yr)
        {
            Voice& vc = voice[(size_t) v];
            const bool ramping = vc.rampPos < rampLength;
            const double gain = ramping ? refFadeWeight (vc.rampPos, rampLength) : 1.0;

            if (ramping)
                ++vc.rampPos;

            const auto in = [ramping, gain] (double x) { return ramping ? x * gain : x; };
            RefChain& f = *vc.chain;

            if (! stereo)
            {
                yl = f.tick (0, in (dl));
                yr = 0.0;
                return;
            }

            switch (vc.input.placement)
            {
                case eq::Placement::left:
                    yl = f.tick (0, in (dl));
                    yr = 0.0;
                    return;

                case eq::Placement::right:
                    yl = 0.0;
                    yr = f.tick (1, in (dr));
                    return;

                case eq::Placement::mid:
                    yl = f.tick (0, in (0.5 * (dl + dr)));
                    yr = yl;
                    return;

                case eq::Placement::side:
                    yl = f.tick (0, in (0.5 * (dl - dr)));
                    yr = -yl;
                    return;

                case eq::Placement::stereo:
                    break;
            }

            yl = f.tick (0, in (dl));
            yr = f.tick (1, in (dr));
        };

        if (seedIndex >= 0)
        {
            configure (active, seedInput);
            voice[(size_t) active].rampPos = rampLength;
            target = seedIndex;
            wishIndex = desiredIndex = seedIndex;
            wishInput = desiredInput = seedInput;
        }

        std::array<std::vector<double>, 2> out { std::vector<double> (length, 0.0), std::vector<double> (length, 0.0) };

        for (size_t n = 0; n < length; ++n)
        {
            for (; nextWish < wishes.size() && (size_t) wishes[nextWish].at <= n; ++nextWish)
            {
                wishIndex = wishes[nextWish].index;
                wishInput = wishes[nextWish].mapped;
            }

            if (n % grid == 0)
            {
                desiredIndex = wishIndex;
                desiredInput = wishInput;

                if (move == Move::none)
                {
                    settle();
                }
                else if ((move == Move::engage || move == Move::change) && desiredIndex >= 0 && desiredIndex == target
                         && eq::classifyBandChange (voice[(size_t) (move == Move::change ? 1 - active : active)].input, desiredInput)
                                == eq::BandChange::continuous)
                {
                    notRestated = true;
                }
            }

            const double dl = dryL[n], dr = stereo ? dryR[n] : 0.0;
            const double el = options.ag * eqL[n], er = stereo ? options.ag * eqR[n] : 0.0;
            double yl = el, yr = er;

            switch (move)
            {
                case Move::none:
                    if (target >= 0)
                        voiceOut (active, dl, dr, yl, yr);

                    break;

                case Move::engage:
                {
                    double ll = 0.0, lr = 0.0;
                    voiceOut (active, dl, dr, ll, lr);

                    if (movePos >= weightStart)
                    {
                        const double w = refFadeWeight (movePos - weightStart, fadeLength);
                        yl = el + w * (ll - el);
                        yr = er + w * (lr - er);
                    }

                    break;
                }

                case Move::change:
                {
                    double al = 0.0, ar = 0.0, bl = 0.0, br = 0.0;
                    voiceOut (active, dl, dr, al, ar);
                    voiceOut (1 - active, dl, dr, bl, br);
                    yl = al;
                    yr = ar;

                    if (movePos >= weightStart)
                    {
                        const double w = refFadeWeight (movePos - weightStart, fadeLength);
                        yl = al + w * (bl - al);
                        yr = ar + w * (br - ar);
                    }

                    break;
                }

                case Move::release:
                {
                    double al = 0.0, ar = 0.0;
                    voiceOut (active, dl, dr, al, ar);
                    const double w = refFadeWeight (movePos, fadeLength);
                    yl = al + w * (el - al);
                    yr = ar + w * (er - ar);
                    break;
                }
            }

            out[0][n] = options.og * yl;
            out[1][n] = options.og * yr;

            if (move != Move::none && ++movePos >= moveLength)
            {
                if (move == Move::change)
                {
                    retire (active);
                    active = 1 - active;
                }
                else if (move == Move::release)
                {
                    retire (0);
                    retire (1);
                }

                move = Move::none;
                movePos = 0;

                const bool differs = desiredIndex != target
                                     || (target >= 0 && eq::classifyBandChange (voice[(size_t) active].input, desiredInput) != eq::BandChange::identical);

                if (differs)
                    settle();
            }
        }

        if (unsupported != nullptr)
            *unsupported = notRestated;

        return out;
    }

    /** P12 hard control: R = 0 (no input ramp, the weight from the move's
        first sample) and N = fadeLength (1: the hard switch; 20 ms: the fade
        from rest of the bench). */
    std::array<std::vector<double>, 2> listenHardRender (double rate, bool stereo,
                                                         const std::vector<double>& dryL, const std::vector<double>& dryR,
                                                         const std::vector<double>& eqL, const std::vector<double>& eqR,
                                                         int seedIndex, const eq::BandInput& seedInput,
                                                         const std::vector<LrWish>& wishes, int fadeLength)
    {
        LrOptions options;
        options.ramp = 0;
        options.fade = fadeLength;
        return listenRefRender (rate, stereo, dryL, dryR, eqL, eqR, seedIndex, seedInput, wishes, options);
    }

    std::array<std::vector<double>, 2> lanesOf (const juce::AudioBuffer<float>& b)
    {
        return { widened (b, 0), b.getNumChannels() > 1 ? widened (b, 1) : widened (b, 0) };
    }

    double kappaStereo (const juce::AudioBuffer<float>& y, double rate, int from, int to)
    {
        double k = 0.0;

        for (int channel = 0; channel < y.getNumChannels(); ++channel)
            k = juce::jmax (k, localKappa (widened (y, channel), rate, (size_t) from, (size_t) to));

        return k;
    }

    /** kappa / the larger held kappa; -1 when no render has a frame above -80 dB. */
    double kappaRatio (double k, double heldA, double heldB)
    {
        const double held = juce::jmax (heldA, heldB);

        if (! (held > 0.0) && ! (k > 0.0))
            return -1.0;

        return held > 0.0 ? k / held : std::numeric_limits<double>::infinity();
    }

    //==========================================================================
    // XR-lon, XR-lonh, XR-lchg, XR-lref
    //==========================================================================
    struct LonReading
    {
        double a[2][2] = { { 0.0, 0.0 }, { 0.0, 0.0 } };        // [block 32, 512][on, off]
        double k[2][2] = { { -1.0, -1.0 }, { -1.0, -1.0 } };
        double hard = 0.0, hardOff = 0.0, fromRest = 0.0;
        double refErr[2] = { 0.0, 0.0 };
        double heldD2On = 0.0, heldKOn = 0.0;                   // the held listen render over the on window (XR-lchg)
        double stopDb = 0.0;
        double toneDbEq = 0.0, toneDbListen = 0.0;              // the held renders' level at the tone (0.3 - 0.5 s, left lane)
        bool finite = true, restated = true;
    };

    /** PLAN-2.6 execute, Taylor's call 2026-11-15 (kappa void into
        silence): the 1 kHz kappa of a listen step is scored only when the
        destination's held level at the tone is within 40 dB of the
        source's; outside, the reading is kappa-void (listed by name), (a)
        still binds it. */
    constexpr double kKappaVoidDb = 40.0;

    /** Level (dB) of the tone in lane 0 over [0.3 s, 0.5 s): whole periods of both tones. */
    double toneLevelDb (const juce::AudioBuffer<float>& y, double hz, double rate)
    {
        const int start = (int) (0.3 * rate), length = (int) (0.2 * rate);
        const double power = binPower (y, 0, start, length, hz, rate);
        return 10.0 * std::log10 (juce::jmax (power, 1.0e-300));
    }

    LonReading lonJob (Processor& p, const Handles& h, double rate, int c, int t, int phase)
    {
        LonReading q;
        const LrTimes tm = lrTimes (rate);
        const auto x = lrTone (rate, kLrTones[t], phase, tm.total);

        resetAllRaw (p, h);
        const eq::RawBand raw = lrRaw (kLrCases[c]);
        storeRaw (h.band[0], raw);
        const eq::BandInput mapped = lrMap (rawInput (raw, 100.0f, true));

        const auto eqOut = renderAt (p, x, rate, 512, {}, noHook);
        p.listenBand.store (0);
        const auto liOut = renderAt (p, x, rate, 512, {}, noHook);
        p.listenBand.store (-1);

        const int sizes[2] = { 32, 512 };
        std::array<juce::AudioBuffer<float>, 2> y;

        for (int b = 0; b < 2; ++b)
        {
            y[(size_t) b] = renderAt (p, x, rate, sizes[b], { tm.tOn, tm.tOff }, [&p, &tm] (BlockInfo& current)
            {
                if (current.position == tm.tOn)
                    p.listenBand.store (0);

                if (current.position == tm.tOff)
                    p.listenBand.store (-1);
            });

            p.listenBand.store (-1);
        }

        p.releaseResources();

        const auto dry = lanesOf (x);
        const auto eqLanes = lanesOf (eqOut);
        const std::vector<LrWish> wishes { { tm.tOn, 0, mapped }, { tm.tOff, -1, {} } };
        bool unsupported = false;
        const auto ref = listenRefRender (rate, true, dry[0], dry[1], eqLanes[0], eqLanes[1], -1, {}, wishes, LrOptions(), &unsupported);
        q.restated = ! unsupported;

        const int win[2][2] = { { tm.tOn - tm.pre, tm.tOn + tm.postOn }, { tm.tOff - tm.pre, tm.tOff + tm.postOff } };
        const bool at1k = kLrTones[t] > 500.0;

        for (int ev = 0; ev < 2; ++ev)
        {
            const auto from = (size_t) win[ev][0], to = (size_t) win[ev][1];
            const double held = juce::jmax (maxD2Stereo (eqOut, from, to), maxD2Stereo (liOut, from, to));

            for (int b = 0; b < 2; ++b)
            {
                q.a[b][ev] = ratioOf (maxD2Stereo (y[(size_t) b], from, to), held);

                if (at1k)
                    q.k[b][ev] = kappaRatio (kappaStereo (y[(size_t) b], rate, win[ev][0], win[ev][1]),
                                             kappaStereo (eqOut, rate, win[ev][0], win[ev][1]), kappaStereo (liOut, rate, win[ev][0], win[ev][1]));
            }
        }

        for (int b = 0; b < 2; ++b)
        {
            q.refErr[b] = errorReDb (y[(size_t) b], ref, 0, tm.total);
            q.finite = q.finite && allFinite (y[(size_t) b]);
        }

        {
            const auto from = (size_t) win[0][0], to = (size_t) win[0][1];
            const double heldOn = juce::jmax (maxD2Stereo (eqOut, from, to), maxD2Stereo (liOut, from, to));
            const auto hard = listenHardRender (rate, true, dry[0], dry[1], eqLanes[0], eqLanes[1], -1, {}, wishes, 1);
            const auto rest = listenHardRender (rate, true, dry[0], dry[1], eqLanes[0], eqLanes[1], -1, {}, wishes, tm.fade);
            q.hard = ratioOf (maxD2Stereo (hard, from, to), heldOn);
            q.fromRest = ratioOf (maxD2Stereo (rest, from, to), heldOn);

            const auto offFrom = (size_t) win[1][0], offTo = (size_t) win[1][1];
            q.hardOff = ratioOf (maxD2Stereo (hard, offFrom, offTo),
                                 juce::jmax (maxD2Stereo (eqOut, offFrom, offTo), maxD2Stereo (liOut, offFrom, offTo)));

            q.heldD2On = maxD2Stereo (liOut, from, to);
            q.heldKOn = kappaStereo (liOut, rate, win[0][0], win[0][1]);
        }

        q.finite = q.finite && allFinite (eqOut) && allFinite (liOut);
        q.toneDbEq = toneLevelDb (eqOut, kLrTones[t], rate);
        q.toneDbListen = toneLevelDb (liOut, kLrTones[t], rate);

        eq::BandDesign design {};
        designerFor (rate)->design (mapped.settings, design);
        q.stopDb = eq::bandMagDb (design, 2.0 * eq::pi * kLrTones[t] / rate);
        return q;
    }

    struct LchgReading
    {
        double a[2] = { 0.0, 0.0 }, k[2] = { -1.0, -1.0 }, refErr[2] = { 0.0, 0.0 };
        bool finite = true, restated = true;
    };

    LchgReading lchgJob (Processor& p, const Handles& h, double rate, int c0, int c1, int t, int phase, const LonReading& held0,
                         const LonReading& held1)
    {
        LchgReading q;
        const LrTimes tm = lrTimes (rate);
        const auto x = lrTone (rate, kLrTones[t], phase, tm.total);

        resetAllRaw (p, h);
        const eq::RawBand raw0 = lrRaw (kLrCases[c0]), raw1 = lrRaw (kLrCases[c1]);
        storeRaw (h.band[0], raw0);
        storeRaw (h.band[1], raw1);
        const eq::BandInput m0 = lrMap (rawInput (raw0, 100.0f, true)), m1 = lrMap (rawInput (raw1, 100.0f, true));

        const int sizes[2] = { 32, 512 };
        const auto dry = lanesOf (x);
        const std::vector<LrWish> wishes { { tm.tOn, 1, m1 } };
        bool unsupported = false;
        const auto ref = listenRefRender (rate, true, dry[0], dry[1], dry[0], dry[1], 0, m0, wishes, LrOptions(), &unsupported);
        q.restated = ! unsupported;

        const auto from = (size_t) (tm.tOn - tm.pre), to = (size_t) (tm.tOn + tm.postOn);

        for (int b = 0; b < 2; ++b)
        {
            p.listenBand.store (0);

            const auto y = renderAt (p, x, rate, sizes[b], { tm.tOn }, [&p, &tm] (BlockInfo& current)
            {
                if (current.position == tm.tOn)
                    p.listenBand.store (1);
            });

            p.listenBand.store (-1);

            q.a[b] = ratioOf (maxD2Stereo (y, from, to), juce::jmax (held0.heldD2On, held1.heldD2On));

            if (kLrTones[t] > 500.0)
                q.k[b] = kappaRatio (kappaStereo (y, rate, (int) from, (int) to), held0.heldKOn, held1.heldKOn);

            q.refErr[b] = errorReDb (y, ref, 0, tm.total);
            q.finite = q.finite && allFinite (y);
        }

        p.releaseResources();
        return q;
    }

    void listenStepRowsXr()
    {
        section ("XR-lon / XR-lchg / XR-lref. listen on / off and band changes through processBlock (Phase 2.6: FUNC-08 engine half, G3; C2; PLAN-2.6 P6, P12)");

        const double rates[2] = { 48000.0, 96000.0 };
        const int onTasks = 2 * kLrCaseCount * 2 * kLrPhases;
        std::vector<LonReading> on ((size_t) onTasks);

        {
            auto pool = makePool (2, onTasks);
            poolFor (onTasks, pool, [&on, &rates] (int task, Processor& p, const Handles& h)
            {
                const int phase = task % kLrPhases;
                const int t = (task / kLrPhases) % 2;
                const int c = (task / (2 * kLrPhases)) % kLrCaseCount;
                const int r = task / (kLrCaseCount * 2 * kLrPhases);
                on[(size_t) task] = lonJob (p, h, rates[r], c, t, phase);
            });
        }

        const auto onAt = [&on] (int r, int c, int t, int phase) -> const LonReading&
        {
            return on[(size_t) (((r * kLrCaseCount + c) * 2 + t) * kLrPhases + phase)];
        };

        double refWorst = -std::numeric_limits<double>::infinity();
        juce::String refWhere;
        int refRenders = 0, refBad = 0, notRestated = 0;

        const auto tallyRef = [&] (double err, bool restated, const juce::String& where)
        {
            ++refRenders;

            if (! (err <= refWorst))
            {
                refWorst = err;
                refWhere = where;
            }

            if (! (err <= -120.0) || ! restated)
                ++refBad;

            if (! restated)
                ++notRestated;
        };

        //======================================================================
        // XR-lon
        //======================================================================
        {
            int aAbove = 0, kAbove = 0, kUnscored = 0, kVoid = 0, readings = 0, nonFinite = 0;
            double aWorst[2][2] = {}, kWorst[2][2] = {};
            juce::String aWhere[2][2], kWhere[2][2], voidText;

            for (int r = 0; r < 2; ++r)
                for (int c = 0; c < kLrCaseCount; ++c)
                    for (int t = 0; t < 2; ++t)
                    {
                        for (int ph = 0; ph < kLrPhases; ++ph)
                        {
                            const LonReading& q = onAt (r, c, t, ph);
                            nonFinite += q.finite ? 0 : 1;

                            for (int b = 0; b < 2; ++b)
                                tallyRef (q.refErr[b], q.restated, juce::String ("XR-lon ") + kLrCases[c].name + ", " + fmtG (kLrTones[t], 4) + " Hz, "
                                                                       + fmtG (rates[r] / 1000.0, 3) + " kHz, block " + (b == 0 ? "32" : "512"));
                        }

                        for (int b = 0; b < 2; ++b)
                            for (int ev = 0; ev < 2; ++ev)
                            {
                                double a = 0.0, k = -1.0;

                                for (int ph = 0; ph < kLrPhases; ++ph)
                                {
                                    const LonReading& q = onAt (r, c, t, ph);
                                    a = juce::jmax (a, q.a[b][ev]);
                                    k = juce::jmax (k, q.k[b][ev]);
                                }

                                ++readings;
                                aAbove += (a <= 2.0) ? 0 : 1;

                                if (! (a <= aWorst[r][ev]))
                                {
                                    aWorst[r][ev] = a;
                                    aWhere[r][ev] = juce::String (kLrCases[c].name) + ", " + fmtG (kLrTones[t], 4) + " Hz, block " + (b == 0 ? "32" : "512");
                                }

                                if (t == 1)
                                {
                                    const LonReading& q0 = onAt (r, c, t, 0);
                                    const double gap = std::abs (q0.toneDbListen - q0.toneDbEq);

                                    if (! (gap <= kKappaVoidDb))
                                    {
                                        ++kVoid;

                                        if (b == 0 && ev == 0)
                                            voidText += (voidText.isEmpty() ? "" : "; ") + juce::String (kLrCases[c].name) + " " + fmtG (rates[r] / 1000.0, 3)
                                                      + " kHz (" + fmtG (gap, 3) + " dB, kappa " + fmtG (k, 4) + "x)";
                                    }
                                    else if (k < 0.0)
                                    {
                                        ++kUnscored;
                                    }
                                    else
                                    {
                                        kAbove += (k <= 2.5) ? 0 : 1;

                                        if (! (k <= kWorst[r][ev]))
                                        {
                                            kWorst[r][ev] = k;
                                            kWhere[r][ev] = juce::String (kLrCases[c].name) + ", block " + (b == 0 ? "32" : "512");
                                        }
                                    }
                                }
                            }
                    }

            row (aAbove == 0 && kAbove == 0 && nonFinite == 0 && readings == 2 * kLrCaseCount * 2 * 2 * 2,
                 "XR-lon listen on / off through processBlock: the 20 XL-on bands, 50 Hz / 1 kHz at -6 dBFS, 4 phases, blocks 32 / 512, 48 / 96 kHz, listenBand written at block starts off the grid (0.5 s + 7, 1.0 s + 11): (a) <= 2.0x the louder held render; 1 kHz kappa <= 2.5x where the listen-held and EQ-held levels at the tone are within 40 dB (else kappa-void)",
                 juce::String (aAbove) + " of " + juce::String (readings) + " readings above 2.0x; " + juce::String (kAbove) + " of "
                     + juce::String (readings / 2 - kUnscored - kVoid) + " scored 1 kHz kappa readings above 2.5x (" + juce::String (kVoid)
                     + " kappa-void: listen-held and EQ-held more than 40 dB apart at the tone (Taylor's call 2026-11-15); " + juce::String (kUnscored)
                     + " with no frame above -80 dB); " + juce::String (nonFinite) + " non-finite");

            info ("XR-lon kappa-void (destination held level at 1 kHz more than 40 dB from the source's; (a) still binds; per band and rate, gap and the unscored kappa): "
                  + (voidText.isEmpty() ? juce::String ("none") : voidText));

            for (int r = 0; r < 2; ++r)
                info ("XR-lon " + fmtG (rates[r] / 1000.0, 3) + " kHz: on (a) " + fmtG (aWorst[r][0], 4) + "x (" + aWhere[r][0] + "; bench 1.21x), off (a) "
                      + fmtG (aWorst[r][1], 4) + "x (" + aWhere[r][1] + "; bench 1.12x); 1 kHz kappa on " + fmtG (kWorst[r][0], 4) + "x (" + kWhere[r][0]
                      + "; bench 2.36x), off " + fmtG (kWorst[r][1], 4) + "x (" + kWhere[r][1] + "; bench 2.25x)" + (r == 0 ? "" : " (no bench at 96 kHz)"));
        }

        //======================================================================
        // XR-lonh (block 512)
        //======================================================================
        {
            int counted = 0, below = 0, voids = 0;
            double lowest[2] = { std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() };
            double restWorst = 0.0, offLowest = std::numeric_limits<double>::infinity();
            juce::String lowestWhere[2], restWhere, voidText;

            for (int r = 0; r < 2; ++r)
                for (int c = 0; c < kLrCaseCount; ++c)
                    for (int t = 0; t < 2; ++t)
                    {
                        double hard = 0.0, rest = 0.0;

                        // A reading is the largest over the four phases, as for (a) (XL-onh).
                        for (int ph = 0; ph < kLrPhases; ++ph)
                        {
                            const LonReading& q = onAt (r, c, t, ph);
                            hard = juce::jmax (hard, q.hard);
                            rest = juce::jmax (rest, q.fromRest);
                            offLowest = juce::jmin (offLowest, q.hardOff);
                        }

                        const juce::String where = juce::String (kLrCases[c].name) + " @ " + fmtG (kLrTones[t], 4) + " Hz, " + fmtG (rates[r] / 1000.0, 3) + " kHz";

                        if (! (rest <= restWorst))
                        {
                            restWorst = rest;
                            restWhere = where;
                        }

                        const double stopDb = onAt (r, c, t, 0).stopDb;

                        if (! (stopDb <= -20.0))
                        {
                            ++voids;

                            if (r == 0)
                                voidText += (voidText.isEmpty() ? "" : "; ") + juce::String (kLrCases[c].name) + " @ " + fmtG (kLrTones[t], 4) + " Hz ("
                                          + fmtG (stopDb, 3) + " dB, " + fmtG (hard, 3) + "x)";

                            continue;
                        }

                        ++counted;
                        below += (std::isfinite (hard) && hard >= 10.0) ? 0 : 1;

                        if (! (hard >= lowest[t]))
                        {
                            lowest[t] = hard;
                            lowestWhere[t] = where;
                        }
                    }

            row (below == 0 && counted > 0 && counted + voids == 2 * kLrCaseCount * 2,
                 "XR-lonh the XR-lon engage with the input ramp and fade replaced by listenHardRender (R = 0, N = 1) on the gain-neutral EQ output and the dry input: (a) >= 10x the held renders on every reading whose tone lies in the listen filter's stop band (<= -20 dB at the tone; block 512)",
                 juce::String (counted - below) + " of " + juce::String (counted) + " counted readings >= 10x (a reading: the largest of the four phases); lowest 50 Hz "
                     + fmtG (lowest[0], 4) + "x (" + lowestWhere[0] + "), 1 kHz " + fmtG (lowest[1], 4) + "x (" + lowestWhere[1]
                     + "); R = 0 / N = 20 ms (the fade from rest) worst " + fmtG (restWorst, 5) + "x (" + restWhere + "; bench 29 672x)");

            info ("XR-lonh VOID (tone in the listen pass band, printed at 48 kHz, not counted; " + juce::String (voids) + " readings over both rates): " + voidText);
            info ("XR-lonh hard switch back to the EQ path (not gated: the EQ path is warm): lowest " + fmtG (offLowest, 4) + "x");
        }

        //======================================================================
        // XR-lchg: 64 sampled ordered pairs at 48 kHz, blocks 32 / 512
        //======================================================================
        {
            std::vector<std::pair<int, int>> pairs;
            std::array<bool, kLrCaseCount * kLrCaseCount> taken {};
            Xorshift pick;
            pick.state = 0x4C4348474C434847ull;

            while (pairs.size() < 64)
            {
                const int a = juce::jmin (kLrCaseCount - 1, (int) (pick.uniform() * kLrCaseCount));
                const int b = juce::jmin (kLrCaseCount - 1, (int) (pick.uniform() * kLrCaseCount));

                if (a == b || taken[(size_t) (a * kLrCaseCount + b)])
                    continue;

                taken[(size_t) (a * kLrCaseCount + b)] = true;
                pairs.emplace_back (a, b);
            }

            const int tasks = (int) pairs.size() * 2 * kLrPhases;
            std::vector<LchgReading> chg ((size_t) tasks);

            {
                auto pool = makePool (2, tasks);
                poolFor (tasks, pool, [&chg, &pairs, &onAt] (int task, Processor& p, const Handles& h)
                {
                    const int phase = task % kLrPhases;
                    const int t = (task / kLrPhases) % 2;
                    const auto& pr = pairs[(size_t) (task / (2 * kLrPhases))];
                    chg[(size_t) task] = lchgJob (p, h, 48000.0, pr.first, pr.second, t, phase, onAt (0, pr.first, t, phase), onAt (0, pr.second, t, phase));
                });
            }

            int aAbove = 0, kAbove = 0, kUnscored = 0, kVoid = 0, readings = 0, over2 = 0, phaseReadings = 0, nonFinite = 0;
            double aWorst[2] = {}, kWorst = 0.0;
            juce::String aWhere[2], kWhere, voidText;

            for (size_t s = 0; s < pairs.size(); ++s)
                for (int t = 0; t < 2; ++t)
                {
                    const juce::String where = juce::String (kLrCases[pairs[s].first].name) + " -> " + kLrCases[pairs[s].second].name;

                    for (int b = 0; b < 2; ++b)
                    {
                        double a = 0.0, k = -1.0;

                        for (int ph = 0; ph < kLrPhases; ++ph)
                        {
                            const LchgReading& q = chg[(s * 2 + (size_t) t) * (size_t) kLrPhases + (size_t) ph];
                            a = juce::jmax (a, q.a[b]);
                            k = juce::jmax (k, q.k[b]);
                            ++phaseReadings;
                            over2 += q.a[b] > 2.0 ? 1 : 0;
                            nonFinite += (q.finite || b == 1) ? 0 : 1;

                            tallyRef (q.refErr[b], q.restated, "XR-lchg " + where + ", " + fmtG (kLrTones[t], 4) + " Hz, block " + (b == 0 ? "32" : "512"));
                        }

                        ++readings;
                        aAbove += (a <= 2.0) ? 0 : 1;

                        if (! (a <= aWorst[t]))
                        {
                            aWorst[t] = a;
                            aWhere[t] = where + ", block " + (b == 0 ? "32" : "512");
                        }

                        if (t == 1)
                        {
                            const double gap = std::abs (onAt (0, pairs[s].second, 1, 0).toneDbListen - onAt (0, pairs[s].first, 1, 0).toneDbListen);

                            if (! (gap <= kKappaVoidDb))
                            {
                                ++kVoid;

                                if (b == 0)
                                    voidText += (voidText.isEmpty() ? "" : "; ") + where + " (" + fmtG (gap, 3) + " dB, kappa " + fmtG (k, 4) + "x)";
                            }
                            else if (k < 0.0)
                            {
                                ++kUnscored;
                            }
                            else
                            {
                                kAbove += (k <= 2.5) ? 0 : 1;

                                if (! (k <= kWorst))
                                {
                                    kWorst = k;
                                    kWhere = where;
                                }
                            }
                        }
                    }
                }

            row (aAbove == 0 && kAbove == 0 && nonFinite == 0 && readings == (int) pairs.size() * 2 * 2,
                 "XR-lchg listen band changes through processBlock: 64 sampled ordered pairs of the 20 bands, 48 kHz, 50 Hz / 1 kHz, 4 phases, blocks 32 / 512: (a) <= 2.0x the louder held listen render, (b) 1 kHz kappa <= 2.5x where the B-held and A-held levels at 1 kHz are within 40 dB (else kappa-void)",
                 juce::String (aAbove) + " of " + juce::String (readings) + " above 2.0x; (a) worst 50 Hz " + fmtG (aWorst[0], 4) + "x (" + aWhere[0] + "), 1 kHz "
                     + fmtG (aWorst[1], 4) + "x (" + aWhere[1] + "; bench 1.35x); (b) " + juce::String (kAbove) + " above 2.5x, worst " + fmtG (kWorst, 4)
                     + "x (" + kWhere + "; bench 2.41x; " + juce::String (kUnscored) + " with no frame, " + juce::String (kVoid)
                     + " kappa-void: B-held and A-held more than 40 dB apart at 1 kHz); single phases over 2.0x: " + juce::String (over2)
                     + " / " + juce::String (phaseReadings) + " (bench 0 / 3040)");

            info ("XR-lchg kappa-void (Taylor's call 2026-11-15; (a) still binds; pair, gap, unscored kappa): " + (voidText.isEmpty() ? juce::String ("none") : voidText));
        }

        row (refBad == 0 && refRenders == 2 * kLrCaseCount * 2 * kLrPhases * 2 + 64 * 2 * kLrPhases * 2,
             "XR-lref every XR-lon / XR-lchg render against listenRefRender (P12: the mapping, solo router, voices, ramp, fade, latch and gains re-stated) fed the plugin's dry input and its EQ path: <= -120 dB re the output peak",
             juce::String (refRenders - refBad) + "/" + juce::String (refRenders) + " renders; worst " + fmtG (refWorst, 4) + " dB (" + refWhere + ")"
                 + (notRestated == 0 ? "" : "; " + juce::String (notRestated) + " renders met a continuous change the reference does not re-state"));
    }

    //==========================================================================
    // XR-lroute, XR-lseed, XR-lseedn, XR-lag, XR-loff, XR-lbyp
    //==========================================================================
    /** The band 1 input the listen rows' one band (P17) maps from. */
    eq::RawBand lrBell (int placement)
    {
        return rawBandSpec (makeSpec (kBell, 1000.0f, 6.0f, 2.0f, 1, placement));
    }

    void listenPathRowsXr()
    {
        section ("XR-lroute / XR-lseed / XR-lag / XR-loff / XR-lbyp. listen routing, seeding, C3, return to idle, bypass (Phase 2.6: D11; C3; PLAN-2.6 P5, P10, P13)");

        const double rate = 48000.0;
        const int fadeN = refFadeLength (rate);

        //======================================================================
        // XR-lroute
        //======================================================================
        {
            Tally tally;
            double worst = -std::numeric_limits<double>::infinity();

            for (const int channels : { 2, 1 })
                for (int placement = 0; placement < 5; ++placement)
                {
                    auto p = makeBands (channels, 0, mixedSpec);
                    const Handles h = handlesOf (*p);
                    resetAllRaw (*p, h);
                    const eq::RawBand raw = lrBell (placement);
                    storeRaw (h.band[0], raw);
                    p->listenBand.store (0);

                    const auto input = makeNoiseAt (channels, 24000, 0x1A0E0000 + channels * 8 + placement, kNoise12);
                    const auto y = renderAt (*p, input, rate, 512, {}, noHook);
                    p->releaseResources();

                    const auto dry = lanesOf (input);
                    const auto ref = listenRefRender (rate, channels == 2, dry[0], dry[1], dry[0], dry[1], 0, lrMap (rawInput (raw, 100.0f, channels == 2)), {},
                                                      LrOptions());
                    const double err = errorReDb (y, ref, 0, y.getNumSamples());
                    worst = juce::jmax (worst, err);
                    tally.check (err <= -140.0 && allFinite (y),
                                 juce::String (channels == 2 ? "stereo " : "mono ") + kPlacementNames[placement] + ": " + fmtG (err, 4) + " dB");
                }

            // PLAN-2.6 R2: Left / Right listen engaged mid-session on a band that is not running (disabled, so
            // the EQ path is the dry input exactly), block 2048: the other lane must be converted and written.
            for (const int placement : { kLeft, kRight })
            {
                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                resetAllRaw (*p, h);
                eq::RawBand raw = lrBell (placement);
                raw.enabled = 0.0f;
                storeRaw (h.band[0], raw);

                const auto input = makeNoiseAt (2, 8 * 2048, 0x1A0E0100 + placement, kNoise12);
                const auto y = renderAt (*p, input, rate, 2048, {}, [&p] (BlockInfo& current)
                {
                    if (current.position == 4 * 2048)
                        p->listenBand.store (0);
                });
                p->releaseResources();

                const auto dry = lanesOf (input);
                const auto ref = listenRefRender (rate, true, dry[0], dry[1], dry[0], dry[1], -1, {},
                                                  { { 4 * 2048, 0, lrMap (rawInput (raw, 100.0f, true)) } }, LrOptions());
                const double err = errorReDb (y, ref, 0, y.getNumSamples());
                worst = juce::jmax (worst, err);
                tally.check (err <= -140.0 && allFinite (y), juce::String ("R2, ") + kPlacementNames[placement] + " engaged at block 2048 on a disabled band: "
                                                                 + fmtG (err, 4) + " dB");
            }

            row (tally.bad == 0 && tally.total == 12,
                 "XR-lroute solo routing through the plugin: one Bell (1 kHz, Q 2) on each placement, stereo and mono layouts, listen settled from the first block: within float rounding (<= -140 dB re peak) of listenRefRender; and (R2) a Left / Right listen engaged mid-session at block 2048",
                 tally.detail() + "  worst " + fmtG (worst, 4) + " dB");
        }

        //======================================================================
        // XR-lseed, XR-lseedn
        //======================================================================
        {
            Tally seed, control;
            double worst = -std::numeric_limits<double>::infinity(), closest = std::numeric_limits<double>::infinity();
            const eq::RawBand raw = lrBell (kStereo);
            const eq::BandInput mapped = lrMap (rawInput (raw, 100.0f, true));

            // (i) listen on at session open.
            for (const int blockSize : { 64, 512 })
            {
                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                resetAllRaw (*p, h);
                storeRaw (h.band[0], raw);
                p->listenBand.store (0);

                const auto input = makeNoiseAt (2, 24000, 0x5EEDA000 + blockSize, kNoise12);
                const auto y = renderAt (*p, input, rate, blockSize, {}, noHook);
                p->releaseResources();

                const auto dry = lanesOf (input);
                const auto seeded = listenRefRender (rate, true, dry[0], dry[1], dry[0], dry[1], 0, mapped, {}, LrOptions());
                const auto engaged = listenRefRender (rate, true, dry[0], dry[1], dry[0], dry[1], -1, {}, { { 0, 0, mapped } }, LrOptions());

                const double err = errorReDb (y, seeded, 0, y.getNumSamples());
                const double errEngaged = errorReDb (y, engaged, 0, y.getNumSamples());
                worst = juce::jmax (worst, err);
                closest = juce::jmin (closest, errEngaged);
                seed.check (err <= -140.0, "session open, block " + juce::String (blockSize) + ": " + fmtG (err, 4) + " dB");
                control.check (errEngaged > -140.0, "session open, block " + juce::String (blockSize) + ": the unseeded engage reads " + fmtG (errEngaged, 4) + " dB");
            }

            // (ii) changed while fully bypassed, un-bypassed by both routes;
            // (iii) changed with an un-bypass latched during the fade to bypass (the step-7 restart).
            constexpr int blockSize = 512, e = 8 * blockSize, length = 40 * blockSize;
            const int resume = 20 * blockSize, latched = e + 240;
            const auto input = makeNoiseAt (2, length, 0x5EEDB002, kNoise12);
            const auto dry = lanesOf (input);

            for (const bool latch : { false, true })
                for (const bool viaEntry : { false, true })
                {
                    auto p = makeBands (2, 0, mixedSpec);
                    const Handles h = handlesOf (*p);
                    resetAllRaw (*p, h);
                    storeRaw (h.band[0], raw);

                    const int un = latch ? latched : resume;
                    const int listenAt = latch ? latched : resume - 6 * blockSize;     // with the latched un-bypass (iii), or while bypassed (ii)
                    const int restart = latch ? e + fadeN : resume;      // the first processed sample of the fade back

                    // Every event sample starts a block.
                    const auto y = renderAt (*p, input, rate, blockSize, { e, listenAt, un }, [&p, &h, viaEntry, un, listenAt] (BlockInfo& current)
                    {
                        if (current.position == e && ! viaEntry)
                            h.bypass->store (1.0f);

                        if (current.position == listenAt)
                            p->listenBand.store (0);

                        if (current.position == un && ! viaEntry)
                            h.bypass->store (0.0f);

                        if (viaEntry)
                            current.viaBypassed = current.position >= e && current.position < un;
                    });
                    p->releaseResources();

                    // The reference from `restart`: dry + w (k) (L - dry), L the listen started settled there.
                    const int span = length - restart;
                    std::vector<double> sl (dry[0].begin() + restart, dry[0].end()), sr (dry[1].begin() + restart, dry[1].end());
                    const auto settled = listenRefRender (rate, true, sl, sr, sl, sr, 0, mapped, {}, LrOptions());
                    const auto engaged = listenRefRender (rate, true, sl, sr, sl, sr, -1, {}, { { 0, 0, mapped } }, LrOptions());

                    const auto mixOf = [&] (const std::array<std::vector<double>, 2>& l)
                    {
                        std::array<std::vector<double>, 2> r { std::vector<double> ((size_t) length, 0.0), std::vector<double> ((size_t) length, 0.0) };

                        for (int channel = 0; channel < 2; ++channel)
                            for (int k = 0; k < span; ++k)
                            {
                                const double d = dry[(size_t) channel][(size_t) (restart + k)];
                                const double v = l[(size_t) channel][(size_t) k];
                                r[(size_t) channel][(size_t) (restart + k)] = k < fadeN ? d + refFadeWeight (k, fadeN) * (v - d) : v;
                            }

                        return r;
                    };

                    const double err = errorReDb (y, mixOf (settled), restart, length);
                    const double errEngaged = errorReDb (y, mixOf (engaged), restart, length);

                    const bool bypassedSpan = latch || sameFrom (y, e + fadeN, input, e + fadeN, resume - e - fadeN);
                    worst = juce::jmax (worst, err);
                    closest = juce::jmin (closest, errEngaged);

                    const juce::String where = juce::String (latch ? "latched un-bypass (step 7)" : "changed while bypassed") + ", "
                                             + (viaEntry ? "processBlockBypassed" : "bypass parameter");
                    seed.check (err <= -140.0 && bypassedSpan && allFinite (y), where + ": " + fmtG (err, 4) + " dB" + (bypassedSpan ? "" : ", the bypassed span is not the input"));
                    control.check (errEngaged > -140.0, where + ": the unseeded engage reads " + fmtG (errEngaged, 4) + " dB");
                }

            row (seed.bad == 0 && seed.total == 6,
                 "XR-lseed listen on at session open (blocks 64 / 512); listen changed while bypassed, then un-bypassed, and with an un-bypass latched in the fade to bypass, both routes: from the first processed sample the output is the bypass fade back to a listen started settled (one band), <= -140 dB re peak; no ramp",
                 seed.detail() + "  worst " + fmtG (worst, 4) + " dB; N = " + juce::String (fadeN));

            row (control.bad == 0 && control.total == 6,
                 "XR-lseedn the same renders against an unseeded engage (the 30 ms ramp and the fade from the first sample) differ (control fires)",
                 control.detail() + "  closest " + fmtG (closest, 4) + " dB");
        }

        //======================================================================
        // XR-lag (C3)
        //======================================================================
        {
            const auto input = makeNoiseAt (2, 48000, 0x1A6A0002, kNoise12);
            const eq::RawBand shelf = rawBandSpec (makeSpec (kLowShelf, 200.0f, 12.0f, 1.0f, 1, kStereo));

            const auto renderWithGains = [&input, &shelf, rate] (bool listen, bool autoOn, float outputDb)
            {
                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                resetAllRaw (*p, h);
                storeRaw (h.band[0], shelf);
                h.autoGain->store (autoOn ? 1.0f : 0.0f);
                h.outputGain->store (outputDb);
                p->listenBand.store (listen ? 0 : -1);
                auto y = renderAt (*p, input, rate, 512, {}, noHook);
                p->releaseResources();
                return y;
            };

            const auto off = renderWithGains (true, false, 0.0f);
            const auto on = renderWithGains (true, true, 0.0f);
            const auto plus6 = renderWithGains (true, true, 6.0f);
            const auto eqOff = renderWithGains (false, false, 0.0f);
            const auto eqOn = renderWithGains (false, true, 0.0f);

            const double autoDb = rmsRatioDb (off, on, 0, off.getNumSamples());
            const double gainDb = rmsRatioDb (off, plus6, 0, off.getNumSamples());
            const double eqAutoDb = rmsRatioDb (eqOff, eqOn, 0, eqOff.getNumSamples());

            row (std::abs (autoDb) <= 1.0e-6 && std::abs (gainDb - 6.0) <= 1.0e-4 && eqAutoDb < -5.0,
                 "XR-lag C3 through the plugin: auto gain on with a +12 dB Low Shelf, listen on that band: the listen level equals the auto-gain-off listen level within 1e-6 dB (float output); output gain +6 dB moves it +6 +- 1e-4 dB",
                 "listen, auto on vs off " + fmtG (autoDb, 4) + " dB" + (sameAudio (off, on) ? " (memcmp-equal)" : "") + "; output +6 dB: " + fmtG (gainDb, 8)
                     + " dB; the EQ path with listen off, auto on vs off: " + fmtG (eqAutoDb, 6) + " dB (the estimate acts there)");
        }

        //======================================================================
        // XR-loff
        //======================================================================
        {
            Tally tally;
            juce::String figures;
            constexpr int blockSize = 512, tListen = 16 * blockSize, tOff = 48 * blockSize;
            const int grid = harnessGrid (rate);
            const int releaseEnd = ((tOff + grid - 1) / grid) * grid + fadeN;
            const int firstBlockAfter = ((releaseEnd + blockSize - 1) / blockSize) * blockSize;

            for (const bool tail : { false, true })
            {
                if (tail && ! keepsDenormals())
                {
                    tally.check (false, "the denormal variant: this thread flushes denormals");
                    continue;
                }

                const int length = tail ? releaseEnd + 2 * 2048 : 64 * blockSize;
                auto input = makeNoiseAt (2, length, 0x10FF0000 + (tail ? 1 : 0), kNoise12);
                const int tailFrom = releaseEnd - 200;

                if (tail)
                {
                    juce::Random random (0x10FF7A11);

                    for (int channel = 0; channel < 2; ++channel)
                    {
                        float* data = input.getWritePointer (channel);

                        for (int i = tailFrom; i < length; ++i)
                        {
                            auto bits = (std::uint32_t) (7137 + random.nextInt (713600 - 7137 + 1));

                            if (random.nextBool())
                                bits |= 0x80000000u;

                            std::memcpy (data + i, &bits, sizeof (bits));
                        }
                    }
                }

                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                resetAllRaw (*p, h);

                const auto y = renderAt (*p, input, rate, blockSize, {}, [&p] (BlockInfo& current)
                {
                    if (current.position == tListen)
                        p->listenBand.store (0);

                    if (current.position == tOff)
                        p->listenBand.store (-1);
                });

                const auto state = p->getListenState();
                p->releaseResources();

                const int from = tail ? firstBlockAfter : releaseEnd;
                const bool equal = sameFrom (y, from, input, from, length - from);
                const bool acted = ! sameFrom (y, tOff, input, tOff, releaseEnd - tOff);
                const bool flushed = ! tail || ! sameFrom (y, releaseEnd, input, releaseEnd, firstBlockAfter - releaseEnd);
                const bool idle = state.target < 0 && state.move == eq::ListenMove::none;

                tally.check (equal && acted && flushed && idle,
                             juce::String (tail ? "with the denormal tail" : "noise") + ": memcmp-equal from " + juce::String (from) + ": " + (equal ? "yes" : "NO")
                                 + (acted ? "" : "; the release did not act") + (flushed ? "" : "; the block holding the release end kept its denormals (it was not converted?)")
                                 + (idle ? "" : "; listen is not idle at the end"));

                figures += (figures.isEmpty() ? "" : "; ") + juce::String (tail ? "denormal tail: equal from the first block start after the release end, " : "noise: equal from the release end, ")
                         + juce::String (from) + " (release end " + juce::String (releaseEnd) + ")";
            }

            row (tally.bad == 0 && tally.total == 2,
                 "XR-loff listen on -> off with no band, output 0 dB, auto off, blocks 512: white noise memcmp-equal to the input from the end of the release fade; with the denormal tail (from 200 samples before that end), from the first block that starts after it",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-lbyp
        //======================================================================
        {
            Tally tally;
            double worstA = 0.0, worstMix = -std::numeric_limits<double>::infinity();
            juce::String worstAWhere;
            constexpr int length = 48000, tL = 12003;
            const int grid = harnessGrid (rate);
            const int tick = ((tL + grid - 1) / grid) * grid;
            const int ramp = juce::jmax (1, (int) std::lround (0.030 * rate));
            const int pre = (int) (0.050 * rate), post = (int) (0.070 * rate);

            struct Move { const char* name; int seedBand; int request; int bypassAt; };
            const Move moves[] = {
                { "listening, settled",           0,  0, 24007 },
                { "engage, in the input ramp",   -1,  0, tick + ramp / 2 },
                { "engage, in the fade",         -1,  0, tick + ramp + fadeN / 2 },
                { "change, in the input ramp",    0,  1, tick + ramp / 2 },
                { "change, in the fade",          0,  1, tick + ramp + fadeN / 2 },
                { "release",                      0, -1, tick + fadeN / 2 }
            };

            for (int t = 0; t < 2; ++t)
            {
                const auto input = lrTone (rate, kLrTones[t], 0, length);
                const auto dry = lanesOf (input);

                for (const auto& mv : moves)
                {
                    const auto renderMove = [&] (bool bypass, bool viaEntry)
                    {
                        auto p = makeBands (2, 0, mixedSpec);
                        const Handles h = handlesOf (*p);
                        resetAllRaw (*p, h);
                        storeRaw (h.band[0], rawBandSpec (makeSpec (kBell, 1000.0f, 12.0f, 2.0f, 0, kStereo)));
                        storeRaw (h.band[1], rawBandSpec (makeSpec (kBandPass, 1000.0f, 0.0f, 10.0f, 0, kStereo)));
                        p->listenBand.store (mv.seedBand);

                        auto y = renderAt (*p, input, rate, 512, { tL, mv.bypassAt }, [&p, &h, &mv, bypass, viaEntry] (BlockInfo& current)
                        {
                            if (current.position == tL && mv.request != mv.seedBand)
                                p->listenBand.store (mv.request);

                            if (bypass && current.position == mv.bypassAt && ! viaEntry)
                                h.bypass->store (1.0f);

                            if (bypass && viaEntry)
                                current.viaBypassed = current.position >= mv.bypassAt;
                        });

                        p->releaseResources();
                        return y;
                    };

                    const auto held = renderMove (false, false);
                    const auto heldLanes = lanesOf (held);

                    for (const bool viaEntry : { false, true })
                    {
                        const auto y = renderMove (true, viaEntry);
                        const int e = mv.bypassAt;

                        std::array<std::vector<double>, 2> ref;

                        for (int channel = 0; channel < 2; ++channel)
                        {
                            auto& r = ref[(size_t) channel];
                            r.assign ((size_t) length, 0.0);

                            for (int i = 0; i < length; ++i)
                            {
                                const double hv = heldLanes[(size_t) channel][(size_t) i], d = dry[(size_t) channel][(size_t) i];
                                r[(size_t) i] = i < e ? hv : (i < e + fadeN ? hv + refFadeWeight (i - e, fadeN) * (d - hv) : d);
                            }
                        }

                        const double mixErr = errorReDb (y, ref, e - pre, e + fadeN + 1);
                        const double heldD2 = juce::jmax (maxD2Stereo (held, (size_t) (e - pre), (size_t) (e + post)),
                                                          maxD2Stereo (input, (size_t) (e - pre), (size_t) (e + post)));
                        const double a = ratioOf (maxD2Stereo (y, (size_t) (e - pre), (size_t) (e + post)), heldD2);
                        const bool bypassed = sameFrom (y, e + fadeN, input, e + fadeN, length - e - fadeN);
                        const juce::String where = juce::String (mv.name) + ", " + fmtG (kLrTones[t], 4) + " Hz, " + (viaEntry ? "processBlockBypassed" : "bypass parameter");

                        if (! (a <= worstA))
                        {
                            worstA = a;
                            worstAWhere = where;
                        }

                        worstMix = juce::jmax (worstMix, mixErr);
                        tally.check (allFinite (y) && a <= 2.0 && mixErr <= -120.0 && bypassed,
                                     where + ": (a) " + fmtG (a, 4) + "x, mix " + fmtG (mixErr, 4) + " dB" + (bypassed ? "" : ", not the input after the fade"));
                    }
                }
            }

            row (tally.bad == 0 && tally.total == 24,
                 "XR-lbyp bypass toggled while listening and during each move kind (engage and change in the ramp and in the fade, release), both routes, 50 Hz / 1 kHz: finite; (a) <= 2.0x the held render and the dry input; the bypass fade mixes the listen output with the dry input (<= -120 dB vs the re-stated mix); the input after it",
                 tally.detail() + "  worst (a) " + fmtG (worstA, 4) + "x (" + worstAWhere + "), worst mix " + fmtG (worstMix, 4) + " dB");
        }
    }

    //==========================================================================
    // XR-blk6. Block identity with listen (PLAN-2.6 R1)
    //==========================================================================
    void listenBlockRowsXr()
    {
        section ("XR-blk6. block identity with listen events off the grid and the analyzer on (Phase 2.6: PLAN-2.6 P10, R1)");

        constexpr int numEvents = 160, spacing = 2048;
        constexpr int length = spacing * numEvents;
        const double rates[3] = { 44100.0, 96000.0, 192000.0 };
        const int sizes[6] = { 1, 32, 64, 512, 2048, length };      // the last: offline, one block per event span
        const auto input = makeNoiseAt (2, length, 0x6B160002, kNoise12);
        const auto events = makeGainEvents();

        // listenBand events: at each event span with p 0.2, at 2048 k + d,
        // d in 1 ... 2047 and never a multiple of 16 (off the grid at every
        // rate): -1 with p 0.5, else one of the four bands.
        std::vector<std::pair<int, int>> listenEvents;
        {
            Xorshift draw;
            draw.state = 0x424C4B364C495354ull;

            for (int k = 0; k < numEvents; ++k)
            {
                if (draw.uniform() >= 0.2)
                    continue;

                int d = 1 + juce::jmin (2046, (int) (draw.uniform() * 2047.0));

                if (d % 16 == 0)
                    ++d;

                const int band = draw.uniform() < 0.5 ? -1 : juce::jmin (3, (int) (draw.uniform() * 4.0));
                listenEvents.emplace_back (k * spacing + d, band);
            }
        }

        std::vector<int> cuts;

        for (int k = 1; k < numEvents; ++k)
            cuts.push_back (k * spacing);

        for (const auto& le : listenEvents)
            cuts.push_back (le.first);

        struct Job { int rate = 0; int size = 0; bool listen = true; };
        std::vector<Job> jobs;

        for (int r = 0; r < 3; ++r)
        {
            for (int s = 0; s < 6; ++s)
                jobs.push_back ({ r, s, true });

            jobs.push_back ({ r, 3, false });
        }

        std::vector<juce::AudioBuffer<float>> outputs (jobs.size());
        std::vector<std::uint32_t> drops (jobs.size());
        auto pool = makePool (2, (int) jobs.size());

        poolFor ((int) jobs.size(), pool, [&] (int j, Processor& p, const Handles& h)
        {
            const Job& job = jobs[(size_t) j];
            resetAllRaw (p, h);
            p.setAnalyzerConsumer (true);
            p.analyzerMode.store (3);
            storeGainEvent (h, events[0]);

            size_t nextListen = 0;

            outputs[(size_t) j] = renderAt (p, input, rates[job.rate], sizes[job.size], cuts, [&] (BlockInfo& current)
            {
                if (current.position % spacing == 0)
                {
                    const auto index = (size_t) (current.position / spacing);

                    if (index < events.size())
                        storeGainEvent (h, events[index]);
                }

                while (nextListen < listenEvents.size() && listenEvents[nextListen].first <= current.position)
                {
                    if (job.listen && listenEvents[nextListen].first == current.position)
                        p.listenBand.store (listenEvents[nextListen].second);

                    ++nextListen;
                }
            }, sizes[job.size]);

            drops[(size_t) j] = p.getPostTap().getDropCount();
            p.releaseResources();
            p.setAnalyzerConsumer (false);
            p.listenBand.store (-1);
        });

        Tally tally;
        int listening = 0;

        for (const auto& le : listenEvents)
            listening += le.second >= 0 ? 1 : 0;

        for (int r = 0; r < 3; ++r)
        {
            const size_t base = (size_t) r * 7;
            const juce::String where = fmtG (rates[r] / 1000.0, 4) + " kHz";

            tally.check (allFinite (outputs[base]) && ! sameAudio (outputs[base], input), where + ": not finite, or nothing acts");

            for (size_t s = 1; s < 6; ++s)
                tally.check (sameAudio (outputs[base + s], outputs[base]),
                             where + ": " + (s == 5 ? juce::String ("offline") : "block " + juce::String (sizes[s])) + " differs from block 1, first at sample "
                                 + juce::String (firstDifference (outputs[base + s], outputs[base])));

            tally.check (! sameAudio (outputs[base + 6], outputs[base + 3]), where + ": the render without listen events is the same (they do not bite)");
            tally.check (drops[base] > 0, where + ": the rings never overflowed (the analyzer taps were not on)");
        }

        row (tally.bad == 0 && tally.total == 24,
             "XR-blk6 XR-blk5's four bands and 160 events (f / Q of the listened band edited at every event, ramps in flight) plus listenBand events off the tick grid (p 0.2 per span), the analyzer on (consumer, mode 3): blocks 1, 32, 64, 512, 2048 and offline (one block per event span) bit-identical at 44.1 / 96 / 192 kHz; differs without the listen events",
             tally.detail() + "  " + juce::String ((int) listenEvents.size()) + " listenBand events (" + juce::String (listening) + " to a band)");
    }

    //==========================================================================
    // XR-lid, XR-lidn. Listen-off identity against the 0.5.0 processor (P13)
    //==========================================================================
    /** session-NN.f32 of the 0.5.0 dump (channel-planar raw floats) into `out`. */
    bool loadLidFile (int index, int channels, int samples, juce::AudioBuffer<float>& out)
    {
        const auto file = lidDumpDir.getChildFile (juce::String::formatted ("session-%02d.f32", index));
        juce::MemoryBlock data;

        if (! file.existsAsFile() || ! file.loadFileAsData (data)
            || data.getSize() != (size_t) channels * (size_t) samples * sizeof (float))
            return false;

        out.setSize (channels, samples);

        for (int channel = 0; channel < channels; ++channel)
            std::memcpy (out.getWritePointer (channel), static_cast<const char*> (data.getData()) + (size_t) channel * (size_t) samples * sizeof (float),
                         (size_t) samples * sizeof (float));

        return true;
    }

    void lidRowsXr()
    {
        section ("XR-lid. listen-off identity against the 0.5.0 processor (Phase 2.6: PLAN-2.6 P13, R7)");

        if (lidDumpDir == juce::File())
        {
            row (false, "XR-lid the 16 sessions memcmp-equal to the 0.5.0 dump", "no dump given: --lid-dir <dir> or OPEQ_LID_DIR (PLAN-2.6 T1: --dump-sessions on a 0.5.0 build)");
            row (false, "XR-lidn one listenBand write differs from the 0.5.0 dump", "no dump given");
            return;
        }

        if (! keepsDenormals())
        {
            row (false, "XR-lid the 16 sessions memcmp-equal to the 0.5.0 dump", "this thread flushes denormals; the session input needs them");
            row (false, "XR-lidn one listenBand write differs from the 0.5.0 dump", "this thread flushes denormals");
            return;
        }

        Tally plain, analyzed;
        std::uint64_t drainedTotal = 0, framesTotal = 0;
        int sessionsDrained = 0;

        for (const auto& session : lidSessions())
        {
            const auto samples = (int) std::llround (kLidSeconds * session.rate);
            juce::AudioBuffer<float> golden;
            const bool loaded = loadLidFile (session.index, session.channels, samples, golden);
            const juce::String where = juce::String::formatted ("session-%02d", session.index);

            if (! loaded)
            {
                plain.check (false, where + ": the 0.5.0 file is missing or has the wrong size");
                analyzed.check (false, where + ": the 0.5.0 file is missing or has the wrong size");
                continue;
            }

            const auto out = renderLidSession (session.index);
            plain.check (sameAudio (out, golden), where + " differs from 0.5.0 at sample " + juce::String (firstDifference (out, golden)));

            // Again with the analyzer consumer registered, mode 3, drained by a second thread.
            std::unique_ptr<SpectrumAnalyzer> analyzer;
            std::thread reader;
            std::atomic<bool> stop { false };
            int done = 0;
            std::uint64_t drained = 0, frames = 0;

            const auto finish = [&] ()
            {
                if (reader.joinable())
                {
                    stop.store (true, std::memory_order_release);
                    reader.join();
                }

                if (analyzer != nullptr)
                {
                    drained = analyzer->getSamplesDrained (0) + analyzer->getSamplesDrained (1);
                    frames = analyzer->getFramesComputed (0) + analyzer->getFramesComputed (1);
                    analyzer.reset();       // unregisters, before the processor goes
                }
            };

            const auto outAnalyzed = renderLidSession (session.index,
                [&] (Processor& p)
                {
                    p.analyzerMode.store (3);
                    analyzer = std::make_unique<SpectrumAnalyzer> (p);
                    reader = std::thread ([&stop, &analyzer] ()
                    {
                        while (! stop.load (std::memory_order_acquire))
                        {
                            analyzer->process (0.033);
                            std::this_thread::yield();
                        }
                    });
                },
                [&] (Processor&, const juce::AudioBuffer<float>& block, int)
                {
                    done += block.getNumSamples();

                    if (done >= samples)
                        finish();
                });

            finish();
            drainedTotal += drained;
            framesTotal += frames;
            sessionsDrained += drained > 0 ? 1 : 0;

            analyzed.check (sameAudio (outAnalyzed, golden) && drained > 0,
                            where + (drained > 0 ? "" : ": the reader drained nothing") + " differs from 0.5.0 at sample "
                                + juce::String (firstDifference (outAnalyzed, golden)));
        }

        row (plain.bad == 0 && plain.total == kLidSessionCount && analyzed.bad == 0 && analyzed.total == kLidSessionCount,
             "XR-lid listen-off identity (P13): the 16 sessions of T1 (random band automation, gains, bypass by both routes, mono / stereo, 44.1 / 96 kHz, blocks 1 - 4096, noise + denormal tail) rendered by this processor are memcmp-equal to the 0.5.0 dump; and again with the analyzer consumer registered, mode 3, drained by a second thread",
             "plain " + plain.detail() + "; analyzer on " + analyzed.detail() + "  (" + juce::String (sessionsDrained) + " sessions drained, "
                 + juce::String ((juce::int64) drainedTotal) + " samples, " + juce::String ((juce::int64) framesTotal) + " frames; dump "
                 + lidDumpDir.getFullPathName() + ")");

        // XR-lidn: session 2 with listenBand = 0 for block 1 only.
        {
            const auto session = lidSessions()[2];
            const auto samples = (int) std::llround (kLidSeconds * session.rate);
            juce::AudioBuffer<float> golden;
            const bool loaded = loadLidFile (2, session.channels, samples, golden);
            bool engaged = false;

            const auto out = renderLidSession (2, [] (Processor&) {}, [&engaged] (Processor& p, const juce::AudioBuffer<float>&, int blockIndex)
            {
                if (blockIndex == 0)
                    p.listenBand.store (0);

                if (blockIndex == 1)
                {
                    p.listenBand.store (-1);
                    engaged = p.getListenState().target >= 0 || p.getListenState().move != eq::ListenMove::none;
                }
            });

            row (loaded && ! sameAudio (out, golden),
                 "XR-lidn session 2 with one listenBand write (band 1 for one block) must differ from the 0.5.0 dump (control fires)",
                 loaded ? "first difference at sample " + juce::String (firstDifference (out, golden)) + (engaged ? " (listen engaged)" : " (listen did not engage)")
                        : juce::String ("the 0.5.0 file is missing"));
        }
    }

    //==========================================================================
    // VERIFICATION-2.5 Issue 2: XR-px17, XR-px11, XR-px4, XR-ag3
    //==========================================================================
    /** agRef with the grid top and the number of cells as arguments (agRef:
        top = min (16 kHz, 0.45 fs), 512 cells). Unclamped dB when `clamp` is
        false. */
    double agEstimateDb (const std::vector<eq::BandInput>& inputs, bool stereo, double rate, const eq::Designer& designer, double top, int points,
                         bool clamp)
    {
        const double low = 40.0;
        std::vector<double> t2 ((size_t) points), laneL ((size_t) points, 1.0), laneR ((size_t) points, 1.0);

        for (int i = 0; i < points; ++i)
        {
            const double u = ((double) i + 0.5) / (double) points;
            const double f = std::exp (std::log (low) + (std::log (top) - std::log (low)) * u);
            const double t = std::tan (eq::pi * f / rate);
            t2[(size_t) i] = t * t;
        }

        int contributing = 0;

        for (const auto& in : inputs)
        {
            if (! in.active || (eq::isGainType (in.settings.type) && in.settings.gainDbEff == 0.0) || (stereo && in.placement == eq::Placement::side))
                continue;

            const bool intoL = ! stereo || in.placement == eq::Placement::stereo || in.placement == eq::Placement::mid || in.placement == eq::Placement::left;
            const bool intoR = ! stereo || in.placement == eq::Placement::stereo || in.placement == eq::Placement::mid || in.placement == eq::Placement::right;

            eq::BandDesign design {};
            designer.design (in.settings, design);
            ++contributing;

            for (int i = 0; i < points; ++i)
            {
                double power = 1.0;

                for (int s = 0; s < design.numSections; ++s)
                    power *= eq::sectionMag2 (t2[(size_t) i], design.params[(size_t) s]);

                if (intoL)
                    laneL[(size_t) i] *= power;

                if (intoR)
                    laneR[(size_t) i] *= power;
            }
        }

        if (contributing == 0)
            return 0.0;

        double sum = 0.0;

        for (int i = 0; i < points; ++i)
            sum += 0.5 * (laneL[(size_t) i] + laneR[(size_t) i]);

        const double db = -10.0 * std::log10 (sum / (double) points);
        return clamp ? juce::jlimit (-18.0, 12.0, db) : db;
    }

    void owedRowsXr()
    {
        section ("XR-px17 / XR-px11 / XR-px4 / XR-ag3. the rows VERIFICATION-2.5 Issue 2 owes (Phase 2.6: PLAN-2.6 T9)");

        const double rate = 48000.0;
        const int fadeN = refFadeLength (rate);

        //======================================================================
        // XR-px17
        //======================================================================
        {
            Tally main, control;
            double worst = -std::numeric_limits<double>::infinity(), closest = std::numeric_limits<double>::infinity();
            constexpr int e = 10240, eUn = e + 240, length = 24000;
            const auto input = makeNoiseAt (2, length, 0x0B170002, kNoise12);
            const double g = refOutputTarget (12.0);
            const int grid = harnessGrid (rate);

            for (const int blockSize : { 32, 512 })
                for (const bool viaEntry : { false, true })
                {
                    auto p = makeBands (2, 0, mixedSpec);
                    const Handles h = handlesOf (*p);
                    resetAllRaw (*p, h);
                    h.outputGain->store (12.0f);

                    const auto y = renderAt (*p, input, rate, blockSize, { e, eUn }, [&h, viaEntry] (BlockInfo& current)
                    {
                        if (current.position == e && ! viaEntry)
                            h.bypass->store (1.0f);

                        if (current.position == eUn && ! viaEntry)
                            h.bypass->store (0.0f);

                        if (viaEntry)
                            current.viaBypassed = current.position >= e && current.position < eUn;
                    });
                    p->releaseResources();

                    // The re-stated output: x g, the fade to bypass over [e, e + N), the fade back
                    // from e + N with the gain in effect from its first sample; the control: the
                    // stage restarting at unity there (a four-pole ramp to g from the next tick).
                    const auto reference = [&] (bool seeded)
                    {
                        std::array<std::vector<double>, 2> r;
                        RefPole4 pole;
                        pole.prepare (refPoleLength (kRefOutputSeconds, rate));
                        pole.snap (1.0);

                        std::vector<double> gain ((size_t) length, g);

                        if (! seeded)
                        {
                            for (int i = e + fadeN; i < length; ++i)
                            {
                                if (i % grid == 0)
                                    pole.retarget (g);

                                gain[(size_t) i] = pole.next();
                            }
                        }

                        for (int channel = 0; channel < 2; ++channel)
                        {
                            const float* x = input.getReadPointer (channel);
                            auto& lane = r[(size_t) channel];
                            lane.assign ((size_t) length, 0.0);

                            for (int i = 0; i < length; ++i)
                            {
                                const double dry = (double) x[i];
                                const double proc = dry * gain[(size_t) i];

                                if (i < e)
                                    lane[(size_t) i] = proc;
                                else if (i < e + fadeN)
                                    lane[(size_t) i] = proc + refFadeWeight (i - e, fadeN) * (dry - proc);
                                else if (i < e + 2 * fadeN)
                                    lane[(size_t) i] = dry + refFadeWeight (i - e - fadeN, fadeN) * (proc - dry);
                                else
                                    lane[(size_t) i] = proc;
                            }
                        }

                        return r;
                    };

                    const double err = errorReDb (y, reference (true), e + fadeN, length);
                    const double errUnseeded = errorReDb (y, reference (false), e + fadeN, length);
                    const double errBefore = errorReDb (y, reference (true), 0, e + fadeN);
                    worst = juce::jmax (worst, err, errBefore);
                    closest = juce::jmin (closest, errUnseeded);

                    const juce::String where = juce::String (viaEntry ? "processBlockBypassed" : "parameter") + ", block " + juce::String (blockSize);
                    main.check (err <= -140.0 && errBefore <= -140.0 && allFinite (y),
                                where + ": from e + N " + fmtG (err, 4) + " dB, before it " + fmtG (errBefore, 4) + " dB");
                    control.check (errUnseeded > -140.0, where + ": the unseeded stage reads " + fmtG (errUnseeded, 4) + " dB");
                }

            row (main.bad == 0 && main.total == 4 && control.bad == 0 && control.total == 4,
                 "XR-px17 VERIFICATION-2.5 PX17: no band, output gain +12 dB; bypass on, un-bypass latched 5 ms into the fade, by both routes, blocks 32 / 512: the gain is in effect from the first processed sample after the restart (no fade-in), <= -140 dB re peak against the constant-gain render; control: an unseeded stage (unity at the restart, ramping to +12 dB) differs",
                 main.detail() + " worst " + fmtG (worst, 4) + " dB; control " + control.detail() + " closest " + fmtG (closest, 4) + " dB");
        }

        //======================================================================
        // XR-px11
        //======================================================================
        {
            Tally tally;
            int movedCuts = 0;
            constexpr int numEvents = 80, spacing = 2048;
            constexpr int length = numEvents * spacing;
            const double rates[2] = { 48000.0, 96000.0 };
            const char* const names[6] = { "random 1 - 2048", "1", "7", "64", "512", "2048" };
            const int fixedSizes[6] = { 0, 1, 7, 64, 512, 2048 };

            for (const double fs : rates)
            {
                const int grid = harnessGrid (fs);
                const auto input = makeNoiseAt (2, length, 0x0B110002 + (int) (fs / 1000.0), kNoise12);
                std::vector<int> positions, ticks;
                std::vector<float> gains;
                Xorshift draw;
                draw.state = 0x5058313150583131ull ^ (std::uint64_t) fs;

                for (int k = 0; k < numEvents; ++k)
                {
                    int off = 1 + juce::jmin (spacing - 2, (int) (draw.uniform() * (double) (spacing - 1)));

                    if (off % grid == 0)
                        ++off;

                    const int position = k * spacing + off;
                    positions.push_back (position);
                    ticks.push_back (((position + grid - 1) / grid) * grid);
                    gains.push_back ((float) (-36.0 + 72.0 * draw.uniform()));
                }

                const auto random = randomBlockSequence (0x0B11B10C + (int) fs, 2048);
                std::vector<juce::AudioBuffer<float>> outs;
                std::vector<std::vector<int>> cutSets;

                for (int r = 0; r < 7; ++r)
                {
                    // Render r cuts event k at c in [P_k, T_k], a different c per render: the
                    // output target goes to its smoother at the next tick, T_k, wherever c lies.
                    std::vector<int> cuts;

                    for (int k = 0; k < numEvents; ++k)
                    {
                        const int span = ticks[(size_t) k] - positions[(size_t) k] + 1;
                        const auto mix = (std::uint64_t) (k + 1) * 0x9E3779B97F4A7C15ull + (std::uint64_t) (r + 1) * 0xD1B54A32D192ED03ull;
                        cuts.push_back (positions[(size_t) k] + (int) ((mix >> 33) % (std::uint64_t) span));
                    }

                    cutSets.push_back (cuts);

                    auto p = makeBands (2, 0, mixedSpec);
                    const Handles h = handlesOf (*p);
                    resetAllRaw (*p, h);
                    h.outputGain->store (-6.0f);

                    size_t next = 0;
                    const bool withEdits = r < 6;
                    const int fixed = fixedSizes[juce::jmin (r, 5)];

                    outs.push_back (renderSized (*p, input, fs, 2048,
                        [&random, fixed] (int index, int) { return fixed > 0 ? fixed : random[(size_t) index % random.size()]; },
                        cuts,
                        [&] (BlockInfo& current)
                        {
                            while (next < cuts.size() && cuts[next] <= current.position)
                            {
                                if (withEdits && cuts[next] == current.position)
                                    h.outputGain->store (gains[next]);

                                ++next;
                            }
                        }));

                    p->releaseResources();
                }

                for (int k = 0; k < numEvents; ++k)
                {
                    bool moved = false;

                    for (size_t r = 1; r < 6; ++r)
                        moved = moved || cutSets[r][(size_t) k] != cutSets[0][(size_t) k];

                    movedCuts += moved ? 1 : 0;
                }

                for (size_t r = 1; r < 6; ++r)
                    tally.check (sameAudio (outs[r], outs[0]), fmtG (fs / 1000.0, 3) + " kHz: block " + names[r] + " differs from the random sequence, first at "
                                                                   + juce::String (firstDifference (outs[r], outs[0])));

                tally.check (allFinite (outs[0]) && ! sameAudio (outs[0], outs[6]), fmtG (fs / 1000.0, 3) + " kHz: the edits do not bite");
            }

            row (tally.bad == 0 && tally.total == 12,
                 "XR-px11 VERIFICATION-2.5 PX11: 80 output-gain edits off the tick boundary (random offsets), each seen at a block start that differs between renders but lies before the next tick; blocks 1 - 2048 random and fixed 1 / 7 / 64 / 512 / 2048, 48 and 96 kHz: bit-identical",
                 tally.detail() + "  " + juce::String (movedCuts) + " of " + juce::String (2 * numEvents) + " edits seen at different samples across the renders");
        }

        //======================================================================
        // XR-px4
        //======================================================================
        {
            Tally tally, control;
            juce::String figures;
            const eq::RawBand bell = rawBandSpec (makeSpec (kBell, 1000.0f, 1.0f, 1.0f, 1, kLeft));

            for (const double fs : { 48000.0, 96000.0 })
            {
                const int grid = harnessGrid (fs);
                const int refresh = refRefreshTicks (fs) * grid;
                const int length = (int) (0.6 * fs);
                const auto input = makeNoiseAt (2, length, 0x0B040002 + (int) (fs / 1000.0), kNoise12);
                const auto designer = designerFor (fs);
                const double estimate = agEstimateDb ({ rawInput (bell, 100.0f, true) }, true, fs, *designer, juce::jmin (16000.0, 0.45 * fs), 512, true);
                const double target = std::pow (10.0, estimate / 20.0);
                const int toggleAt = 3 * 2048 + 5 * grid + 3;
                const int bellAt = toggleAt + (int) (0.1 * fs) + 7;

                for (const bool sameBlock : { true, false })
                {
                    const int seenAt = sameBlock ? toggleAt : bellAt;
                    const int refreshTick = ((seenAt + refresh - 1) / refresh) * refresh;
                    std::vector<juce::AudioBuffer<float>> outs;

                    for (const int blockSize : { 32, 512, 2048 })
                    {
                        auto p = makeBands (2, 0, mixedSpec);
                        const Handles h = handlesOf (*p);
                        resetAllRaw (*p, h);

                        outs.push_back (renderAt (*p, input, fs, blockSize, { toggleAt, bellAt }, [&h, &bell, toggleAt, bellAt, sameBlock] (BlockInfo& current)
                        {
                            if (current.position == toggleAt)
                            {
                                h.autoGain->store (1.0f);

                                if (sameBlock)
                                    storeRaw (h.band[0], bell);
                            }

                            if (current.position == bellAt && ! sameBlock)
                                storeRaw (h.band[0], bell);
                        }));

                        p->releaseResources();
                    }

                    // The R lane re-stated: x_R g (n), g the auto-gain cascade retargeted on the
                    // first refresh tick after the bell is seen (gainTrajectory); the control one refresh later.
                    const auto trajectoryR = [&] (int estimateFrom)
                    {
                        GainScript script;
                        script.add (0, 1.0, false, 1.0);
                        script.add (toggleAt, 1.0, true, estimateFrom == toggleAt ? target : 1.0);

                        if (estimateFrom != toggleAt)
                            script.add (estimateFrom, 1.0, true, target);

                        const auto gain = gainTrajectory (script, length, fs, false);
                        std::vector<double> r ((size_t) length, 0.0);
                        const float* x = input.getReadPointer (1);

                        for (int i = 0; i < length && gain.size() == (size_t) length; ++i)
                            r[(size_t) i] = (double) x[i] * gain[(size_t) i];

                        return r;
                    };

                    const auto ref = trajectoryR (seenAt);
                    const auto late = trajectoryR (refreshTick + 1);
                    const float* x = input.getReadPointer (1);
                    const juce::String where = fmtG (fs / 1000.0, 3) + " kHz, " + (sameBlock ? "bell with the toggle" : "toggle alone, bell 100 ms later");

                    for (size_t b = 0; b < outs.size(); ++b)
                    {
                        const auto& y = outs[b];
                        int onset = -1;

                        for (int i = 0; i < length && onset < 0; ++i)
                            if (! sameBits (y.getSample (1, i), x[i]))
                                onset = i;

                        int refOnset = -1;

                        for (int i = 0; i < length && refOnset < 0; ++i)
                            if (! sameBits ((float) ref[(size_t) i], x[i]))
                                refOnset = i;

                        const double err = laneErrorReDb (y, 1, ref, 0, length);
                        const double errLate = laneErrorReDb (y, 1, late, 0, length);
                        tally.check (onset >= refreshTick && onset == refOnset && err <= -140.0 && (b == 0 || sameAudio (y, outs[0])),
                                     where + ", block " + juce::String (b == 0 ? 32 : (b == 1 ? 512 : 2048)) + ": onset " + juce::String (onset) + " (re-stated "
                                         + juce::String (refOnset) + ", refresh tick " + juce::String (refreshTick) + "), " + fmtG (err, 4) + " dB"
                                         + (b == 0 || sameAudio (y, outs[0]) ? "" : ", differs from block 32"));
                        control.check (errLate > -140.0, where + ": one refresh late reads " + fmtG (errLate, 4) + " dB");

                        if (b == 0)
                            figures += (figures.isEmpty() ? "" : "; ") + where + ": refresh tick " + juce::String (refreshTick) + ", onset " + juce::String (onset)
                                     + ", " + fmtG (err, 4) + " dB (one refresh late " + fmtG (errLate, 4) + " dB)";
                    }
                }
            }

            row (tally.bad == 0 && tally.total == 12 && control.bad == 0,
                 "XR-px4 VERIFICATION-2.5 PX4: an auto-gain toggle in an otherwise idle block (no band, stage at unity) with a Left +1 dB bell set in the same block, and with the bell 100 ms later: the right lane follows the gain from the refresh tick (equal to the input before it; <= -140 dB re peak of the re-stated trajectory; its first changed sample is the re-stated one) at blocks 32 / 512 / 2048, bit-identical across them; one refresh late differs",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-ag3
        //======================================================================
        {
            Tally tally, control;
            juce::String figures;

            struct Case { const char* name; int type; float freq, gain, q; int slope; bool g12; };
            const Case cases[] = {
                { "G12 Low Shelf 200 Hz +12 12 dB/oct", kLowShelf, 200.0f, 12.0f, 1.0f, 1, true },
                { "Low Shelf 200 Hz +12 24 dB/oct",     kLowShelf, 200.0f, 12.0f, 1.0f, 3, false },
                { "High Shelf 6 kHz +12",               kHighShelf, 6000.0f, 12.0f, 1.0f, 1, false },
                { "High Shelf 8 kHz +12",               kHighShelf, 8000.0f, 12.0f, 1.0f, 1, false },
                { "Bell 1 kHz +12 Q 2",                 kBell, 1000.0f, 12.0f, 2.0f, 0, false },
                { "Low Cut 100 Hz 24 dB/oct",           kLowCut, 100.0f, 0.0f, 1.0f, 3, false },
                { "High Cut 5 kHz 48 dB/oct",           kHighCut, 5000.0f, 0.0f, 1.0f, 5, false },
                { "Tilt 1 kHz +12",                     kTilt, 1000.0f, 12.0f, 1.0f, 1, false }
            };

            double worst = 0.0;
            juce::String worstWhere;

            for (const double fs : { 22050.0, 32000.0 })
            {
                const auto designer = designerFor (fs);
                const auto input = makeNoiseAt (2, (int) (0.5 * fs), (juce::int64) 0xA6300002 + (juce::int64) fs, kNoise12);
                const double top45 = juce::jmin (16000.0, 0.45 * fs), top50 = juce::jmin (16000.0, 0.5 * fs);

                for (const auto& c : cases)
                {
                    const eq::RawBand raw = rawBandSpec (makeSpec (c.type, c.freq, c.gain, c.q, c.slope, kStereo));
                    const std::vector<eq::BandInput> bands { rawInput (raw, 100.0f, true) };

                    const auto renderAuto = [&input, &raw, fs] (bool on)
                    {
                        auto p = makeBands (2, 0, mixedSpec);
                        const Handles h = handlesOf (*p);
                        resetAllRaw (*p, h);
                        storeRaw (h.band[0], raw);
                        h.autoGain->store (on ? 1.0f : 0.0f);
                        auto y = renderAt (*p, input, fs, 512, {}, noHook);
                        p->releaseResources();
                        return y;
                    };

                    const auto on = renderAuto (true);
                    const auto off = renderAuto (false);
                    double num = 0.0, den = 0.0;

                    for (int channel = 0; channel < 2; ++channel)
                        for (int i = 0; i < on.getNumSamples(); ++i)
                        {
                            num += (double) on.getSample (channel, i) * (double) off.getSample (channel, i);
                            den += (double) off.getSample (channel, i) * (double) off.getSample (channel, i);
                        }

                    const double appliedDb = 20.0 * std::log10 (num / den);
                    const double truthDb = agEstimateDb (bands, true, fs, *designer, top45, 40000, true);
                    const double top50Db = agEstimateDb (bands, true, fs, *designer, top50, 512, true);
                    const double error = std::abs (appliedDb - truthDb);
                    const juce::String where = juce::String (c.name) + ", " + fmtG (fs / 1000.0, 4) + " kHz";

                    if (! (error <= worst))
                    {
                        worst = error;
                        worstWhere = where;
                    }

                    tally.check (allFinite (on) && error <= 0.06, where + ": plugin " + fmtG (appliedDb, 6) + " dB, truth " + fmtG (truthDb, 6) + " dB");

                    if (c.g12)
                    {
                        control.check (std::abs (top50Db - appliedDb) > 0.001, where + ": a 0.5 fs top reads " + fmtG (top50Db, 6) + " dB");
                        figures += (figures.isEmpty() ? "" : "; ") + where + ": plugin " + fmtG (appliedDb, 6) + " dB, truth (0.45 fs top) " + fmtG (truthDb, 6)
                                 + " dB, 0.5 fs top " + fmtG (top50Db, 6) + " dB";
                    }
                }
            }

            row (tally.bad == 0 && tally.total == 16 && control.bad == 0 && control.total == 2,
                 "XR-ag3 VERIFICATION-2.5 AG3: G12 auto gain at 22.05 and 32 kHz (the 0.45 fs grid top active), the plugin's applied gain (auto on / off, least squares) against the 40 000-point pink truth with the 0.45 fs top: within 0.06 dB (8 bands incl. the G12 shelf); control: the 512-point estimate with a 0.5 fs top reads outside 0.001 dB of the plugin's",
                 tally.detail() + " worst " + fmtG (worst, 3) + " dB (" + worstWhere + "); control " + control.detail() + "  " + figures);
        }
    }

    //==========================================================================
    //==========================================================================
    // Stage 3, Phase 3.1: the rows VERIFICATION-2.6 Issue 2 owes (CONTEXT U8).
    // Ported from stages/2-dsp/verify-2.6/probe.cpp (E1, T1, N1, R1, X1: the
    // probe-only kills of LS10, LS11, LS12, LS15 / PX26, PX27, PX38, PX39),
    // plus PX22, DM2 and AN1 (real by read, ungated in 2.6). The renders
    // follow the probe's: a fresh instance prepared, then set up; a block is
    // cut at every event, so an event lands on a block start.
    //==========================================================================
    //==========================================================================
    using OwedFn = std::function<void (Processor&)>;

    struct OwedEvent
    {
        int at;
        OwedFn apply;
    };

    struct OwedOptions
    {
        int block = 64;
        bool randomBlocks = false;      // 1 ... maxBlock, seeded
        juce::int64 seed = 1;
        int maxBlock = 2048;
        std::function<bool (int)> viaBypassed;                  // per block start
        std::function<void (Processor&, int, int)> after;       // after each block (start, length)
    };

    // Band indices here are 0-based, as listenBand's (bandId is 1-based).
    OwedFn owedBand (int index, int type, float freq, float gain, float q = 1.0f, int slope = 1, int placement = kStereo)
    {
        return [=] (Processor& p) { setBand (p, index + 1, makeSpec (type, freq, gain, q, slope, placement)); };
    }

    OwedFn owedSet (const juce::String& id, float raw)
    {
        return [=] (Processor& p) { setRaw (p, id, raw); };
    }

    OwedFn owedListen (int band)
    {
        return [=] (Processor& p) { p.listenBand.store (band); };
    }

    OwedFn owedAll (std::vector<OwedFn> fns)
    {
        return [=] (Processor& p) { for (const auto& f : fns) if (f) f (p); };
    }

    int owedLength (double seconds, double rate)  { return juce::jmax (1, (int) std::lround (seconds * rate)); }
    int owedCeil (int sample, int grid)           { return ((sample + grid - 1) / grid) * grid; }

    juce::AudioBuffer<float> owedRender (double rate, const juce::AudioBuffer<float>& input, const OwedFn& setup,
                                         const std::vector<OwedEvent>& events, const OwedOptions& o = {})
    {
        const int length = input.getNumSamples(), numChannels = input.getNumChannels();
        const int prepared = o.randomBlocks ? o.maxBlock : o.block;
        auto p = make();

        if (! setLayout (*p, numChannels))
            row (false, "harness: layout refused (" + juce::String (numChannels) + " channels)");

        p->setRateAndBufferSizeDetails (rate, prepared);
        p->prepareToPlay (rate, prepared);

        if (setup)
            setup (*p);

        juce::AudioBuffer<float> buffer;
        buffer.makeCopyOf (input);
        juce::MidiBuffer midi;
        juce::Random random (o.seed);

        for (int start = 0; start < length;)
        {
            int n = juce::jmin (o.randomBlocks ? 1 + random.nextInt (o.maxBlock) : o.block, length - start);

            for (const auto& e : events)
                if (e.at > start && e.at < start + n)
                    n = e.at - start;

            for (const auto& e : events)
                if (e.at == start)
                    e.apply (*p);

            juce::AudioBuffer<float> view (buffer.getArrayOfWritePointers(), numChannels, start, n);

            if (o.viaBypassed && o.viaBypassed (start))
                p->processBlockBypassed (view, midi);
            else
                p->processBlock (view, midi);

            if (o.after)
                o.after (*p, start, n);

            start += n;
        }

        p->releaseResources();
        return buffer;
    }

    bool owedSame (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        return a.getNumChannels() == b.getNumChannels() && a.getNumSamples() == b.getNumSamples()
            && sameFrom (a, 0, b, 0, a.getNumSamples());
    }

    /** First sample index at which a and b differ (any channel), or -1. */
    int owedFirstDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        int first = -1;

        for (int channel = 0; channel < a.getNumChannels(); ++channel)
            for (int i = 0; i < a.getNumSamples(); ++i)
                if (! sameBits (a.getSample (channel, i), b.getSample (channel, i)))
                {
                    if (first < 0 || i < first)
                        first = i;

                    break;
                }

        return first;
    }

    void owed26RowsXr()
    {
        section ("XR-ls10 / XR-ls11 / XR-ls15 / XR-px38 / XR-px39 / XR-px22 / XR-dm2 / XR-an1. the rows VERIFICATION-2.6 Issue 2 owes (Phase 3.1: CONTEXT U8; probe E1, T1, N1, R1, X1)");

        //======================================================================
        // XR-ls10 (probe E1; kills LS10, PX27): an f / Q edit of the listened
        // band inside the incoming voice's input ramp reaches that voice on its
        // tick (PLAN-2.6 P6, applyPending), so the output differs from the
        // unedited render from move sample R on (the first sample the voice is
        // heard), within one grid; and an edit of a settled listen moves the
        // listen output (the voices' control tick runs).
        //======================================================================
        {
            Tally tally;
            juce::String figures;

            for (const double rate : { 48000.0, 96000.0 })
                for (const int kind : { 0, 1 })         // 0 engage band 3; 1 change 3 -> 7, the edit on 7
                    for (const int what : { 0, 1 })     // 0 freq, 1 Q
                    {
                        const int grid = harnessGrid (rate), ramp = owedLength (0.030, rate), fade = owedLength (0.020, rate), move = ramp + fade;
                        const int s1 = 1001, t1 = owedCeil (s1, grid);
                        const int s2 = t1 + move + 500, t2 = owedCeil (s2, grid);
                        const int moveStart = kind == 0 ? t1 : t2, band = kind == 0 ? 3 : 7;
                        const int edit = moveStart + ramp / 3;
                        const int length = moveStart + move + 4000;
                        const auto input = makeNoiseAt (2, length, 0x31000095, 0.5f);
                        const auto setup = owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.0f), owedSet (bandId (4, "enabled"), 0.0f),
                                                      owedBand (7, kBandPass, 3000.0f, 0.0f, 4.0f), owedSet (bandId (8, "enabled"), 0.0f) });
                        std::vector<OwedEvent> base = { { s1, owedListen (3) } };

                        if (kind == 1)
                            base.push_back ({ s2, owedListen (7) });

                        auto edited = base;
                        edited.push_back ({ edit, what == 0 ? owedSet (bandId (band + 1, "freq"), band == 3 ? 1500.0f : 4500.0f)
                                                            : owedSet (bandId (band + 1, "q"), 0.5f) });
                        OwedOptions o;
                        o.block = 64;
                        const int first = owedFirstDifference (owedRender (rate, input, setup, base, o), owedRender (rate, input, setup, edited, o));
                        const bool ok = first >= moveStart + ramp && first < moveStart + ramp + grid;
                        const juce::String where = fmtG (rate / 1000.0, 3) + "k " + (kind ? "change" : "engage") + (what ? " Q" : " f");
                        tally.check (ok, where + " first difference " + juce::String (first) + " (want " + juce::String (moveStart + ramp) + " + < " + juce::String (grid) + ")");
                        figures += (figures.isEmpty() ? "" : ", ") + where + " " + juce::String (first) + " / " + juce::String (moveStart + ramp);
                    }

            // A settled listen (from the first block) on band 3: an f / Q edit at mid render moves the output.
            for (const int what : { 0, 1 })
            {
                const double rate = 48000.0;
                const int length = owedLength (0.25, rate);
                const auto input = makeNoiseAt (2, length, 0x31000096, 0.5f);
                const auto setup = owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.5f), owedListen (3) });
                OwedOptions o;
                o.block = 128;
                const auto base = owedRender (rate, input, setup, {}, o);
                const auto edited = owedRender (rate, input, setup, { { length / 2, what == 0 ? owedSet (bandId (4, "freq"), 1300.0f) : owedSet (bandId (4, "q"), 0.7f) } }, o);
                const int first = owedFirstDifference (base, edited);
                tally.check (first >= length / 2, juce::String ("settled listen, ") + (what ? "Q" : "f") + " edit at " + juce::String (length / 2) + ": first difference " + juce::String (first));
                figures += juce::String (", settled ") + (what ? "Q" : "f") + " edit " + juce::String (first) + " / " + juce::String (length / 2);
            }

            row (tally.bad == 0 && tally.total == 10,
                 "XR-ls10 VERIFICATION-2.6 LS10 / PX27 (probe E1): an f / Q edit of the listened band inside the incoming voice's input ramp (engage, and change 3 -> 7), 48 / 96 kHz, block 64: the output first differs from the unedited render at move sample R (within one grid); a settled listen's f / Q edit moves the output",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-ls11 (probe T1; kills LS11, LS12): the listen timeline at block 1
        // against the re-stated P6 state machine. A latched change, release
        // and engage; a settled frequency edit of the listened band ramps (no
        // move); a gain edit does nothing; a type-family edit runs a change
        // move to the same band; a latched change after it.
        //======================================================================
        {
            struct Segment { int start; eq::ListenMove kind; int length; int target; bool latched; };
            struct Cfg { double rate; int channels; };
            Tally tally;
            juce::String figures;

            for (const Cfg c : { Cfg { 48000.0, 2 }, Cfg { 96000.0, 2 }, Cfg { 44100.0, 1 }, Cfg { 192000.0, 2 } })
            {
                const int grid = harnessGrid (c.rate), ramp = owedLength (0.030, c.rate), fade = owedLength (0.020, c.rate), move = ramp + fade;
                const int s1 = 1001;            const int t1 = owedCeil (s1, grid);
                const int s2 = t1 + 500;        const int e1 = t1 + move;       // latched change to 7
                const int s3 = e1 + 700;        const int e2 = e1 + move;       // latched release
                const int s4 = e2 + 100;        const int e3 = e2 + fade;       // latched engage to 5
                const int s5 = e3 + move + 301;                                  // band 5 freq edit: a ramp, no move
                const int s6 = s5 + 403;                                         // band 5 gain edit: nothing
                const int s7 = s6 + 405;        const int t7 = owedCeil (s7, grid);   // band 5 type family: a change, target 5
                const int s8 = t7 + 37;         const int e7 = t7 + move;       // latched change to 9
                const int length = e7 + move + 500;

                const std::vector<Segment> segments = {
                    { t1, eq::ListenMove::engage,  move, 3,  false },
                    { e1, eq::ListenMove::change,  move, 7,  true  },
                    { e2, eq::ListenMove::release, fade, -1, true  },
                    { e3, eq::ListenMove::engage,  move, 5,  true  },
                    { t7, eq::ListenMove::change,  move, 5,  false },
                    { e7, eq::ListenMove::change,  move, 9,  true  } };

                const auto targetWithNoMove = [&] (int i)
                {
                    if (i < t1)
                        return -1;

                    for (int k = (int) segments.size() - 1; k >= 0; --k)
                        if (i >= segments[(size_t) k].start)
                            return segments[(size_t) k].target;

                    return -1;
                };

                const auto expect = [&] (int i, eq::ListenMove& kind, int& position, int& target)
                {
                    for (size_t k = 0; k < segments.size(); ++k)
                    {
                        const Segment& s = segments[k];

                        if (i >= s.start && i < s.start + s.length)
                        {
                            if (i < s.start + s.length - 1)
                            {
                                kind = s.kind; position = i - s.start + 1; target = s.target;
                                return;
                            }

                            if (k + 1 < segments.size() && segments[k + 1].latched && segments[k + 1].start == i + 1)
                            {
                                kind = segments[k + 1].kind; position = 0; target = segments[k + 1].target;
                                return;
                            }

                            kind = eq::ListenMove::none; position = 0; target = s.target;
                            return;
                        }
                    }

                    kind = eq::ListenMove::none; position = 0; target = targetWithNoMove (i);
                };

                int mismatched = 0, firstBad = -1, rampSeen = 0, fadeSeen = 0;
                OwedOptions o;
                o.block = 1;
                o.after = [&] (Processor& p, int start, int)
                {
                    const eq::ListenState state = p.getListenState();
                    rampSeen = state.rampLength;
                    fadeSeen = state.fadeLength;
                    eq::ListenMove kind;
                    int position = 0, target = 0;
                    expect (start, kind, position, target);

                    if (state.move != kind || state.movePosition != position || state.target != target)
                    {
                        if (firstBad < 0)
                            firstBad = start;

                        ++mismatched;
                    }
                };

                const auto input = makeNoiseAt (c.channels, length, 0x31000021, 0.5f);
                const auto setup = owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.0f), owedBand (7, kLowShelf, 200.0f, -4.0f),
                                              owedBand (5, kBell, 3000.0f, 3.0f, 4.0f), owedBand (9, kHighCut, 6000.0f, 0.0f, 1.0f, 3) });
                owedRender (c.rate, input, setup,
                            { { s1, owedListen (3) }, { s2, owedListen (7) }, { s3, owedListen (-1) }, { s4, owedListen (5) },
                              { s5, owedSet (bandId (6, "freq"), 2500.0f) }, { s6, owedSet (bandId (6, "gain"), -9.0f) },
                              { s7, owedSet (bandId (6, "type"), (float) kHighShelf) }, { s8, owedListen (9) } }, o);

                const juce::String where = fmtG (c.rate / 1000.0, 4) + "k/" + juce::String (c.channels) + "ch";
                tally.check (mismatched == 0 && rampSeen == ramp && fadeSeen == fade,
                             where + ": " + juce::String (mismatched) + " samples off the state machine" + (firstBad >= 0 ? " (first " + juce::String (firstBad) + ")" : juce::String())
                                 + ", R " + juce::String (rampSeen) + " / " + juce::String (ramp) + ", N " + juce::String (fadeSeen) + " / " + juce::String (fade));
                figures += (figures.isEmpty() ? "" : ", ") + where + " " + juce::String (length) + " samples, " + juce::String (mismatched) + " off";
            }

            row (tally.bad == 0 && tally.total == 4,
                 "XR-ls11 VERIFICATION-2.6 LS11 / LS12 (probe T1): the listen state (move, move position, target) after every sample at block 1, 48 / 96 / 44.1 (mono) / 192 kHz, equals the re-stated P6 machine: latched change, release and engage; a settled f edit of the listened band ramps with no move; a gain edit does nothing; a type-family edit runs a change move to the same band; R and N as prepared",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-ls15 (probe N1; kills LS15, PX26): a non-finite input sample,
        // listen settled on band 3 and no listen: the output is finite from
        // the first block that starts after it (PLAN-2.6 P10 step 8 clears the
        // listen voices).
        //======================================================================
        {
            Tally tally;

            for (const int channels : { 1, 2 })
                for (const bool listening : { true, false })
                    for (const int block : { 64, 512 })
                    {
                        const double rate = 48000.0;
                        const int length = 48000, nanAt = 20000 + 37;
                        auto input = makeNoiseAt (channels, length, 0x31000090 + channels, 0.5f);

                        for (int channel = 0; channel < channels; ++channel)
                            input.setSample (channel, nanAt, std::numeric_limits<float>::quiet_NaN());

                        OwedOptions o;
                        o.block = block;
                        const auto y = owedRender (rate, input, owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.0f), owedListen (listening ? 3 : -1) }), {}, o);
                        const int from = (nanAt / block + 1) * block;
                        int nonFinite = 0;

                        for (int channel = 0; channel < channels; ++channel)
                            for (int i = from; i < length; ++i)
                                if (! std::isfinite (y.getSample (channel, i)))
                                    ++nonFinite;

                        tally.check (nonFinite == 0, juce::String (channels) + "ch " + (listening ? "listen" : "no listen") + " block " + juce::String (block) + ": "
                                                         + juce::String (nonFinite) + " non-finite samples from " + juce::String (from));
                    }

            row (tally.bad == 0 && tally.total == 8,
                 "XR-ls15 VERIFICATION-2.6 LS15 / PX26 (probe N1): a NaN input sample at 20 037, listen settled on band 3 and no listen, mono / stereo, blocks 64 / 512, 48 kHz: the output is finite from the first block that starts after it",
                 tally.detail());
        }

        //======================================================================
        // XR-px38 (probe R1): an out-of-range listenBand reads as off
        //======================================================================
        {
            Tally tally;

            for (const int channels : { 1, 2 })
            {
                const double rate = 48000.0;
                const int length = owedLength (0.4, rate);
                const auto input = makeNoiseAt (channels, length, 0x31000060, 0.5f);
                const auto setup = owedAll ({ owedBand (0, kBell, 500.0f, 6.0f), owedBand (11, kLowCut, 60.0f, 0.0f, 1.0f, 5), owedBand (23, kHighShelf, 5000.0f, -4.0f) });
                OwedOptions o;
                o.randomBlocks = true;
                o.maxBlock = 1000;
                o.seed = 3;
                const auto base = owedRender (rate, input, setup, {}, o);
                const auto y = owedRender (rate, input, setup,
                                           { { 777, owedListen (Processor::kNumBands) }, { 5000, owedListen (1000) }, { 9001, owedListen (-2) },
                                             { 13000, owedListen (std::numeric_limits<int>::min()) }, { 15555, owedListen (std::numeric_limits<int>::max()) },
                                             { 17000, owedListen (-1) } }, o);
                const int first = owedFirstDifference (y, base);
                tally.check (first < 0, juce::String (channels) + "ch: first difference " + juce::String (first));
            }

            row (tally.bad == 0 && tally.total == 2,
                 "XR-px38 VERIFICATION-2.6 PX38 (probe R1): listenBand 24, 1000, -2, INT_MIN, INT_MAX, then -1, three bands on, random blocks 1 - 1000, mono / stereo, 48 kHz: memcmp-equal to the render with no listen",
                 tally.detail());
        }

        //======================================================================
        // XR-px39 (probe X1): a latched un-bypass with listen engaged, no band
        // running and the stage at unity fades back in (P10 step 7 jumps only
        // with listen idle). Bypass on at b1, off again 128 samples later, by
        // the parameter and by processBlockBypassed; the fade back starts at
        // r = b1 + N from the dry signal.
        //======================================================================
        {
            Tally tally;
            juce::String figures;

            for (const double rate : { 48000.0, 96000.0 })
                for (const int channels : { 1, 2 })
                    for (const bool viaEntry : { false, true })
                    {
                        const int fade = owedLength (0.020, rate), b1 = 6400, b2 = b1 + 128, length = b1 + 6 * fade;
                        const auto input = makeNoiseAt (channels, length, 0x31000101, 0.5f);
                        OwedOptions o;
                        o.block = 64;
                        std::vector<OwedEvent> events = { { 0, owedListen (3) } };

                        if (viaEntry)
                        {
                            o.viaBypassed = [b1, b2] (int start) { return start >= b1 && start < b2; };
                        }
                        else
                        {
                            events.push_back ({ b1, owedSet ("bypass", 1.0f) });
                            events.push_back ({ b2, owedSet ("bypass", 0.0f) });
                        }

                        const auto y = owedRender (rate, input, owedAll ({ owedBand (3, kBandPass, 1000.0f, 0.0f, 2.0f), owedSet (bandId (4, "enabled"), 0.0f) }), events, o);
                        const int r = b1 + fade;
                        const auto meanError = [&] (int from, int to)
                        {
                            double sum = 0.0;
                            int count = 0;

                            for (int channel = 0; channel < channels; ++channel)
                                for (int i = from; i < to; ++i, ++count)
                                    sum += std::abs ((double) y.getSample (channel, i) - (double) input.getSample (channel, i));

                            return sum / juce::jmax (1, count);
                        };

                        const double ratio = meanError (r - 4, r + 12) / meanError (r + 2 * fade, r + 2 * fade + 512);
                        const juce::String where = fmtG (rate / 1000.0, 3) + "k/" + juce::String (channels) + "ch/" + (viaEntry ? "bypassed" : "param");
                        tally.check (ratio < 0.05, where + " " + fmtG (ratio, 3));
                        figures += (figures.isEmpty() ? "" : ", ") + where + " " + fmtG (ratio, 2);
                    }

            row (tally.bad == 0 && tally.total == 8,
                 "XR-px39 VERIFICATION-2.6 PX39 (probe X1): listen on band 3 (disabled, no band running, unity stage), bypass on at 6400 and off 128 samples later, by the parameter and by processBlockBypassed, 48 / 96 kHz, mono / stereo: the fade back from the dry signal, mean |y - x| over [r - 4, r + 12) under 0.05 x the settled one (r = 6400 + N; a jump reads ~1)",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-px22: on a mono layout every placement is effectively Stereo, so
        // a placement edit of the listened band while listening changes
        // nothing: no move starts and the output is memcmp-equal to the
        // unedited render (makeInput applies effectiveBandInput). Control: the
        // same edits on a stereo layout start change moves and move the output.
        //======================================================================
        {
            Tally tally;
            juce::String figures;

            for (const int channels : { 1, 2 })
            {
                const double rate = 48000.0;
                const int length = owedLength (0.5, rate);
                const auto input = makeNoiseAt (channels, length, 0x31000022, 0.5f);
                const auto setup = owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.0f, 1, kLeft), owedListen (3) });
                const std::vector<OwedEvent> edits = { { 4001, owedSet (bandId (4, "placement"), (float) kRight) },
                                                       { 9007, owedSet (bandId (4, "placement"), (float) kMid) },
                                                       { 14011, owedSet (bandId (4, "placement"), (float) kSide) },
                                                       { 19013, owedSet (bandId (4, "placement"), (float) kStereo) } };
                int moveBlocks = 0, offTarget = 0;
                OwedOptions o;
                o.block = 64;
                const auto base = owedRender (rate, input, setup, {}, o);
                o.after = [&] (Processor& p, int, int)
                {
                    const eq::ListenState state = p.getListenState();
                    moveBlocks += state.move != eq::ListenMove::none ? 1 : 0;
                    offTarget += state.target != 3 ? 1 : 0;
                };
                const auto y = owedRender (rate, input, setup, edits, o);
                const int first = owedFirstDifference (y, base);

                if (channels == 1)
                    tally.check (moveBlocks == 0 && offTarget == 0 && first < 0,
                                 "mono: " + juce::String (moveBlocks) + " blocks in a move, " + juce::String (offTarget) + " off target 3, first difference " + juce::String (first));
                else
                    tally.check (moveBlocks > 0 && first >= 4001,
                                 "control, stereo: " + juce::String (moveBlocks) + " blocks in a move, first difference " + juce::String (first));

                figures += (figures.isEmpty() ? "" : ", ") + juce::String (channels == 1 ? "mono" : "stereo control") + ": " + juce::String (moveBlocks)
                         + " move blocks, first difference " + juce::String (first);
            }

            row (tally.bad == 0 && tally.total == 2,
                 "XR-px22 VERIFICATION-2.6 PX22: mono layout, listen settled on a Left bell, placement edits Left -> Right -> Mid -> Side -> Stereo, block 64, 48 kHz: no listen move starts and the output is memcmp-equal to the unedited render; control: the same edits on stereo start moves and move the output",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-dm2: DisplayModel::poll re-designs every band on a layout change
        // at the same rate (a host renegotiating mono / stereo): all 24 are
        // reported, the header's stereo flag follows, and the packets carry
        // the effective placement (Stereo on mono, the parameter's on stereo).
        //======================================================================
        {
            Tally tally;
            auto p = make();
            setLayout (*p, 2);
            const int bands[4] = { 2, 4, 6, 8 };
            const int placements[4] = { kLeft, kMid, kSide, kRight };
            setBand (*p, bands[0] + 1, makeSpec (kBell, 500.0f, 6.0f, 2.0f, 1, placements[0]));
            setBand (*p, bands[1] + 1, makeSpec (kHighShelf, 4000.0f, -3.0f, 1.0f, 1, placements[1]));
            setBand (*p, bands[2] + 1, makeSpec (kBell, 2000.0f, 4.0f, 1.0f, 1, placements[2]));
            setBand (*p, bands[3] + 1, makeSpec (kLowCut, 80.0f, 0.0f, 1.0f, 3, placements[3]));
            auto model = std::make_unique<DisplayModel> (*p);

            const auto all24 = [] (const juce::Array<int>& a)
            {
                if (a.size() != Processor::kNumBands)
                    return false;

                for (int b = 0; b < Processor::kNumBands; ++b)
                    if (a[b] != b)
                        return false;

                return true;
            };

            const auto header = [&model] (double rate, bool stereo)
            {
                const juce::var head = model->headerVar();
                return (double) head.getProperty ("sampleRate", 0.0) == rate && (bool) head.getProperty ("rateKnown", false)
                    && (bool) head.getProperty ("stereo", ! stereo) == stereo;
            };

            const auto placementsAre = [&] (bool stereo)
            {
                for (int i = 0; i < 4; ++i)
                    if ((int) model->packetFor (bands[i]).getProperty ("placement", -1) != (stereo ? placements[i] : kStereo))
                        return false;

                return true;
            };

            const auto relayout = [&p] (int channels, double rate)
            {
                p->releaseResources();
                const bool ok = setLayout (*p, channels);
                p->prepareToPlay (rate, 512);
                return ok;
            };

            for (const double rate : { 48000.0, 96000.0 })
            {
                const juce::String r = fmtG (rate / 1000.0, 3) + "k";
                bool ok = relayout (2, rate);
                auto changed = model->poll();
                tally.check (ok && all24 (changed) && header (rate, true) && placementsAre (true), r + " stereo prepare: " + juce::String (changed.size()) + " bands");

                changed = model->poll();
                tally.check (changed.isEmpty(), r + " no edit: " + juce::String (changed.size()));

                ok = relayout (1, rate);
                changed = model->poll();
                tally.check (ok && all24 (changed) && header (rate, false) && placementsAre (false),
                             r + " stereo -> mono at the same rate: " + juce::String (changed.size()) + " bands, header stereo "
                                 + (model->headerVar().getProperty ("stereo", true) ? "true" : "false") + ", placements " + (placementsAre (false) ? "Stereo" : "NOT EFFECTIVE"));

                changed = model->poll();
                tally.check (changed.isEmpty(), r + " no edit on mono: " + juce::String (changed.size()));

                ok = relayout (2, rate);
                changed = model->poll();
                tally.check (ok && all24 (changed) && header (rate, true) && placementsAre (true),
                             r + " mono -> stereo at the same rate: " + juce::String (changed.size()) + " bands, placements " + (placementsAre (true) ? "the parameters'" : "NOT RESTORED"));
            }

            model.reset();
            p->releaseResources();

            row (tally.bad == 0 && tally.total == 10,
                 "XR-dm2 VERIFICATION-2.6 DM2: DisplayModel::poll on a layout change at the same rate (stereo -> mono -> stereo at 48 and 96 kHz, re-prepared): all 24 bands re-designed, the header's stereo flag follows, Left / Mid / Side / Right bands carry Stereo on mono and their own placement on stereo; no edit -> empty",
                 tally.detail());
        }

        //======================================================================
        // XR-an1: no analyzer frame straddles a drop. The harness re-states
        // which input samples each drain holds (the processor offers each
        // block's last min (n, C) samples; a push that does not fit keeps its
        // newest `space` samples; the analyzer drains everything per call),
        // checks that against the analyzer's drained count, and requires that
        // whenever a frame is computed the newest N drained samples are
        // consecutive input samples. A 2 s stall (no process call) overflows
        // both rings; frames must resume after it (vacuity guard).
        //======================================================================
        {
            Tally tally;
            juce::String figures;
            const double rate = 48000.0;
            const int fftN = SpectrumAnalyzer::fftSizeForRate (rate);
            constexpr int blockSize = 512;
            const int stall = (int) (2.0 * rate);

            for (const int cadence : { 512, 2048 })
            {
                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                resetAllRaw (*p, h);
                p->analyzerMode.store (3);
                p->prepareToPlay (rate, blockSize);
                auto analyzer = std::make_unique<SpectrumAnalyzer> (*p);

                std::vector<int> pending;           // input indices in the rings since the last drain (both taps alike)
                std::vector<int> history;           // every drained index, in order
                juce::AudioBuffer<float> block (2, blockSize);
                juce::MidiBuffer midi;
                int position = 0, straddles = 0, modelOff = 0, framesBefore = 0, framesAfter = 0, callsToResume = -1, callsAfter = 0;
                bool afterStall = false;
                std::uint64_t frames[2] = { 0, 0 }, drained[2] = { 0, 0 };

                const auto play = [&] (int samples)
                {
                    for (int done = 0; done < samples;)
                    {
                        const int n = juce::jmin (blockSize, samples - done);
                        block.setSize (2, n, false, false, true);

                        for (int i = 0; i < n; ++i)
                        {
                            const auto v = (float) (0.25 * std::sin (2.0 * eq::pi * 1000.0 * (double) (position + i) / rate));
                            block.setSample (0, i, v);
                            block.setSample (1, i, v);
                        }

                        p->processBlock (block, midi);

                        const int offered = juce::jmin (n, eq::SpectrumTap::capacity);
                        const int space = eq::SpectrumTap::capacity - (int) pending.size();
                        const int kept = juce::jmin (offered, space);

                        for (int i = n - kept; i < n; ++i)
                            pending.push_back (position + i);

                        position += n;
                        done += n;
                    }
                };

                const auto call = [&]
                {
                    analyzer->process ((double) cadence / rate);
                    history.insert (history.end(), pending.begin(), pending.end());

                    bool framed = false;

                    for (int tap = 0; tap < 2; ++tap)
                    {
                        const auto d = analyzer->getSamplesDrained (tap) - drained[tap];
                        drained[tap] = analyzer->getSamplesDrained (tap);

                        if (d != (std::uint64_t) pending.size())
                            ++modelOff;

                        if (analyzer->getFramesComputed (tap) > frames[tap])
                        {
                            framed = true;
                            frames[tap] = analyzer->getFramesComputed (tap);

                            bool consecutive = (int) history.size() >= fftN;

                            for (size_t k = history.size() - (size_t) juce::jmin ((int) history.size(), fftN) + 1; k < history.size() && consecutive; ++k)
                                consecutive = history[k] == history[k - 1] + 1;

                            if (! consecutive)
                                ++straddles;
                        }
                    }

                    pending.clear();

                    if (afterStall)
                    {
                        ++callsAfter;

                        if (framed)
                        {
                            ++framesAfter;

                            if (callsToResume < 0)
                                callsToResume = callsAfter;
                        }
                    }
                    else if (framed)
                    {
                        ++framesBefore;
                    }
                };

                for (int k = 0; k < 16; ++k)
                {
                    play (cadence);
                    call();
                }

                const std::uint32_t dropsBefore = p->getPreTap().getDropCount() + p->getPostTap().getDropCount();
                play (stall);
                const std::uint32_t dropsStall = p->getPreTap().getDropCount() + p->getPostTap().getDropCount() - dropsBefore;
                afterStall = true;
                call();                                             // the stale, overflowed run

                for (int k = 0; k < 16; ++k)
                {
                    play (cadence);
                    call();
                }

                analyzer.reset();
                p->releaseResources();

                const bool resumed = callsToResume > 0 && framesAfter > 0;
                tally.check (straddles == 0 && modelOff == 0 && framesBefore > 0 && dropsStall > 0 && resumed,
                             "cadence " + juce::String (cadence) + ": " + juce::String (straddles) + " frames straddle a gap, model off " + juce::String (modelOff)
                                 + ", frames before / after " + juce::String (framesBefore) + " / " + juce::String (framesAfter) + ", drops " + juce::String ((juce::int64) dropsStall)
                                 + ", first frame at call " + juce::String (callsToResume) + " after the stall");
                figures += (figures.isEmpty() ? "" : ", ") + juce::String ("cadence ") + juce::String (cadence) + ": first frame at call " + juce::String (callsToResume)
                         + " after the stall, drops " + juce::String ((juce::int64) dropsStall);
            }

            row (tally.bad == 0 && tally.total == 2,
                 "XR-an1 VERIFICATION-2.6 AN1: analyzer with both taps, 48 kHz, blocks 512, a process call every 512 / 2048 samples, a 2 s stall (both rings overflow): every frame's newest N drained samples are consecutive input samples (the harness's re-stated drain contents, checked against the analyzer's drained count); frames resume after the stall",
                 tally.detail() + "  " + figures);
        }
    }

    //==========================================================================
    // A5, A5n. G6 with the analyzer taps saturated and listen switching
    //==========================================================================
    struct ListenAllocResult
    {
        GainAllocResult gain;
        std::int64_t listeningBlocks = 0, moveBlocks = 0;
        int switches = 0;
        std::uint32_t dropsPre = 0, dropsPost = 0;
    };

    /** As A4 (runGainAllocWindow): the window is processBlock or
        processBlockBypassed alone, the first block after prepareToPlay
        included; 20 s at 48 kHz of the P14 every-block draw (24 bands moving,
        bypass toggled by both routes), auto_gain on, output_gain redrawn per
        block; plus the analyzer consumer registered with mode 3 and no reader
        (the rings fill in about 0.7 s, then every block overflows) and
        listenBand switched every 100 ms (StressListenDraw mode 2). */
    ListenAllocResult runListenAllocWindow (bool plant)
    {
        ListenAllocResult result;

       #if defined (__APPLE__)
        const double rate = 48000.0;
        const double seconds = 20.0;

        const StressSeed chosen = chooseStressSeed24 (rate, seconds, 1, 2);
        auto p = makeBands (2, 0, mixedSpec);
        const Handles h = handlesOf (*p);
        p->setAnalyzerConsumer (true);
        p->analyzerMode.store (3);

        StressListenDraw listen;
        listen.begin (2, chosen.seed);

        malloc_logger_t* const previous = malloc_logger;
        allocThread = pthread_self();
        allocCount = 0;
        freeCount = 0;
        malloc_logger = countAllocation;

        allocArmed = true;
        {
            void* volatile probe = std::malloc (64);
            std::free (probe);
        }
        allocArmed = false;
        result.gain.fade.base.probeAllocs = allocCount;
        result.gain.fade.base.probeFrees = freeCount;
        allocCount = 0;
        freeCount = 0;

        bool plantNext = plant;
        int viaEntry = 0;

        const StressOutcome outcome = stressLoopCore (*p, h, rate, seconds, chosen.seed, 1, true, 2, nullptr, &listen,
                                                      [&p, &plantNext, &viaEntry, &result] (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, bool viaBypassed)
                                                      {
                                                          allocArmed = true;

                                                          if (viaBypassed)
                                                              p->processBlockBypassed (buffer, midiMessages);
                                                          else
                                                              p->processBlock (buffer, midiMessages);

                                                          if (plantNext)
                                                          {
                                                              void* volatile planted = std::malloc (64);
                                                              std::free (planted);
                                                          }

                                                          allocArmed = false;
                                                          plantNext = false;

                                                          if (viaBypassed)
                                                              ++viaEntry;

                                                          const eq::ListenState state = p->getListenState();
                                                          result.listeningBlocks += state.target >= 0 ? 1 : 0;
                                                          result.moveBlocks += state.move != eq::ListenMove::none ? 1 : 0;
                                                      });

        malloc_logger = previous;
        result.gain.evaluations = p->getAutoGainEvaluationCount();
        result.gain.refreshes = p->getAutoGainRefreshCount();
        result.dropsPre = p->getPreTap().getDropCount();
        result.dropsPost = p->getPostTap().getDropCount();
        result.switches = listen.listenChanges;
        p->releaseResources();

        result.gain.fade.base.available = true;
        result.gain.fade.base.allocs = allocCount;
        result.gain.fade.base.frees = freeCount;
        result.gain.fade.base.blocks = (int) outcome.blocks;
        result.gain.fade.base.audioOk = chosen.ok && outcome.nonFinite == 0 && outcome.actingBlocks > 0;
        result.gain.fade.viaEntryBlocks = viaEntry;
       #else
        juce::ignoreUnused (plant);
       #endif

        return result;
    }

    void listenAllocationRowsXr()
    {
        section ("A5. allocation with the analyzer taps saturated and listen switching (Phase 2.6: G6 as amended, PERF-01)");

        const ListenAllocResult clean = runListenAllocWindow (false);
        const AllocResult& base = clean.gain.fade.base;

        if (! base.available)
        {
            info ("A5 allocation gate SKIPPED: no malloc_logger on this platform");
            info ("A5n allocation control SKIPPED: no malloc_logger on this platform");
            return;
        }

        const bool hookLive = base.probeAllocs >= 1 && base.probeFrees >= 1;
        const bool refreshing = clean.gain.refreshes > 0 && clean.gain.evaluations > clean.gain.refreshes;
        const bool saturated = clean.dropsPre > 0 && clean.dropsPost > 0;
        const bool listening = clean.listeningBlocks > 0 && clean.moveBlocks > 0 && clean.switches >= 190;

        row (hookLive && base.audioOk && clean.gain.fade.viaEntryBlocks > 0 && refreshing && saturated && listening && base.allocs == 0 && base.frees == 0,
             "A5 G6: processBlock and processBlockBypassed, 20 s at 48 kHz, first block included: the A4 load (24 bands moving at every block, bypass toggled by both routes, auto gain refreshing, output gain per block) plus the analyzer consumer registered, mode 3, no reader (the rings saturate), listen switching every 100 ms: 0 allocations, 0 frees",
             juce::String (base.allocs) + " allocations, " + juce::String (base.frees) + " frees in " + juce::String (base.blocks) + " windows ("
                 + juce::String (clean.gain.fade.viaEntryBlocks) + " through processBlockBypassed; " + juce::String ((juce::int64) clean.gain.evaluations)
                 + " band evaluations in " + juce::String ((juce::int64) clean.gain.refreshes) + " refreshes; drops pre / post "
                 + juce::String ((juce::int64) clean.dropsPre) + " / " + juce::String ((juce::int64) clean.dropsPost) + "; " + juce::String (clean.switches)
                 + " listenBand switches, " + juce::String ((juce::int64) clean.listeningBlocks) + " blocks listening, "
                 + juce::String ((juce::int64) clean.moveBlocks) + " ending inside a move); hook " + (hookLive ? "live" : "DEAD")
                 + " (probe: " + juce::String (base.probeAllocs) + " / " + juce::String (base.probeFrees) + ")"
                 + (base.audioOk ? "" : "; the stress did not run as expected (stack guard, non-finite output, or no acting block)"));

        const ListenAllocResult planted = runListenAllocWindow (true);

        row (planted.gain.fade.base.allocs >= 1 && planted.gain.fade.base.frees >= 1,
             "A5n a planted allocation inside the A5 window is counted (control fires)",
             juce::String (planted.gain.fade.base.allocs) + " allocations, " + juce::String (planted.gain.fade.base.frees) + " frees");
    }

    //==========================================================================
    // X-cpu5. CPU with the taps and listen (INFO; PLAN-2.6 P15)
    //==========================================================================
    void cpuInfoListen()
    {
        section ("X5. CPU with the analyzer taps and listen (INFO, not gated; G10 binds in Stage 4)");

        constexpr int blockSize = 512;
        const double rate = 48000.0;
        const int numBlocks = (int) (20.0 * rate / (double) blockSize);
        const double audioSeconds = (double) numBlocks * (double) blockSize / rate;
        const auto noise = makeNoiseAt (2, blockSize, 0x0C9500002, kNoise12);
        const int switchEvery = (int) std::lround (0.050 * rate);     // R + N: a change move in flight nearly all the time

        struct Config { const char* name; int type; int slope; int type2; int slope2; };

        const Config configs[] =
        {
            { "24 cuts x 96 dB/oct (Low Cut <> High Cut)",       kLowCut,    7, kHighCut,   7 },
            { "24 shelves x 96 dB/oct (High <> Low)",            kHighShelf, 7, kLowShelf,  7 },
            { "24 bells <> 12 dB/oct shelves",                   kBell,      0, kHighShelf, 1 },
            { "24 shelves 96 <> 72 dB/oct",                      kHighShelf, 7, kHighShelf, 6 }
        };

        const char* const modeNames[4] = { "settled", "always fading", "one fade per second", "24 asleep" };

        // voices 1: listen settled on band 1 from the first block; 2: listenBand
        // alternating between bands 1 and 2 every 50 ms (two voices in a change).
        const auto timeRun = [&] (const Config& c, int mode, int voices, bool& finite)
        {
            auto p = makeBands (2, 24, [&c, mode] (int index)
            {
                return makeSpec (c.type, 40.0f * std::pow (1.28f, (float) index), mode == 3 ? 0.0f : 6.0f, 1.0f, c.slope, kStereo);
            });

            const Handles h = handlesOf (*p);
            h.autoGain->store (1.0f);
            p->setAnalyzerConsumer (true);
            p->analyzerMode.store (3);
            p->listenBand.store (0);

            juce::AudioBuffer<float> block (2, blockSize);
            juce::MidiBuffer midi;
            double seconds = 0.0;

            p->prepareToPlay (rate, blockSize);

            for (int k = 0; k < numBlocks; ++k)
            {
                const double t = (double) (k * blockSize) / rate;
                bool second = false;

                if (mode == 1)
                    second = (((long) (t / 0.0215)) & 1) != 0;

                if (mode == 2)
                    second = (((long) t) & 1) != 0;

                for (size_t b = 0; b < h.band.size(); ++b)
                {
                    const BandHandles& hb = h.band[b];
                    hb.type->store ((float) (second ? c.type2 : c.type));
                    hb.slope->store ((float) (second ? c.slope2 : c.slope));
                }

                if (voices == 2)
                    p->listenBand.store (((k * blockSize) / switchEvery) % 2);

                for (int channel = 0; channel < 2; ++channel)
                    block.copyFrom (channel, 0, noise, channel, 0, blockSize);

                const auto t0 = std::chrono::steady_clock::now();
                p->processBlock (block, midi);
                const auto t1 = std::chrono::steady_clock::now();

                seconds += std::chrono::duration<double> (t1 - t0).count();
            }

            p->releaseResources();
            finite = finite && allFinite (block);
            return 100.0 * seconds / audioSeconds;
        };

        for (size_t ci = 0; ci < 4; ++ci)
        {
            const Config& c = configs[ci];
            bool finite = true;
            juce::String line = juce::String ("X-cpu5 48 kHz stereo, 512-sample blocks, 20 s, auto_gain on, taps on (mode 3), ") + c.name + ":";

            for (int mode = 0; mode < 4; ++mode)
            {
                const double one = timeRun (c, mode, 1, finite);
                const double two = timeRun (c, mode, 2, finite);
                line += juce::String (" ") + modeNames[mode] + " listen 1 voice " + fmtG (one, 3) + " %, 2 voices " + fmtG (two, 3) + " % (X-cpu4 "
                      + fmtG (xcpu4Percent[ci][mode], 3) + " %)" + (mode < 3 ? ";" : "");
            }

            info (line + " of one core" + (finite ? "" : "  OUTPUT NOT FINITE"));
        }
    }

    //==========================================================================
    // Stage 3, Phase 3.3 (PLAN-3.3 T3): the listen NaN fix (P1), the analyzer
    // Off gating and the meter feed's hold rule (P4). N1e and AO1 are the
    // RESEARCH-3.3 probe rows (research-3.3/probe/probe33.cpp) as written.
    //==========================================================================
    std::unique_ptr<Processor> p33Make (double rate, int channels, int block)
    {
        auto p = make();

        if (! setLayout (*p, channels))
            row (false, "harness: layout refused (" + juce::String (channels) + " channels)");

        p->setRateAndBufferSizeDetails (rate, block);
        p->prepareToPlay (rate, block);
        return p;
    }

    void phase33RowsXr()
    {
        section ("N1e / N1e-u / AO1 / AO2 / M1. the listen NaN fix, analyzer Off and its Off rule, the meter hold (Phase 3.3: PLAN-3.3 P1, P2, P4)");

        //======================================================================
        // N1e (VERIFICATION-2.6 Issue 1; probe33 n): a NaN at t1 + 700, inside
        // the 30 ms input ramp of an engage (from off) or a change (5 -> 3) at
        // 1001, 48 kHz; band 3 on / off, band 5 off. PASS when no non-finite
        // sample follows the NaN's own host block. Before P1, an engage with
        // no band running stayed non-finite until the ramp opened (12 / 16).
        //======================================================================
        {
            Tally tally;

            for (const int channels : { 1, 2 })
                for (const int block : { 64, 512 })
                    for (const int band3 : { 0, 1 })
                        for (const int kind : { 0, 1 })     // 0 engage from off, 1 change 5 -> 3
                        {
                            const double rate = 48000.0;
                            const int length = 9000, grid = harnessGrid (rate);
                            const int t1 = owedCeil (1001, grid), nanAt = t1 + 700;
                            auto input = makeNoiseAt (channels, length, 91, 0.5f);

                            for (int channel = 0; channel < channels; ++channel)
                                input.setSample (channel, nanAt, std::numeric_limits<float>::quiet_NaN());

                            OwedOptions o;
                            o.block = block;
                            const auto y = owedRender (rate, input,
                                                       owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.0f), owedBand (5, kBell, 4000.0f, 3.0f, 1.0f),
                                                                  owedSet (bandId (6, "enabled"), 0.0f), owedSet (bandId (4, "enabled"), band3 ? 1.0f : 0.0f),
                                                                  owedListen (kind ? 5 : -1) }),
                                                       { { 1001, owedListen (3) } }, o);
                            const int chunkEnd = 1001 + ((nanAt - 1001) / block + 1) * block;
                            int lo = -1, hi = -1;

                            for (int channel = 0; channel < channels; ++channel)
                                for (int i = chunkEnd; i < length; ++i)
                                    if (! std::isfinite (y.getSample (channel, i)))
                                    {
                                        if (lo < 0 || i < lo) lo = i;
                                        hi = juce::jmax (hi, i);
                                    }

                            const juce::String where = juce::String (channels) + "ch blk " + juce::String (block) + (band3 ? " on" : " off") + (kind ? " change" : " engage");
                            tally.check (lo < 0, where + ": non-finite [" + juce::String (lo) + " .. " + juce::String (hi) + "] after " + juce::String (chunkEnd));
                        }

            row (tally.bad == 0 && tally.total == 16,
                 "N1e    VERIFICATION-2.6 Issue 1 (probe33 n): a NaN at t1 + 700 inside listen's 30 ms input ramp (engage from off / change 5 -> 3 at 1001), band 3 on / off, mono / stereo, blocks 64 / 512, 48 kHz: no non-finite sample after the NaN's own block",
                 tally.detail());
        }

        //======================================================================
        // N1e-u (PLAN-3.3 P1: the flag is taken every chunk, never inside
        // `! finite || ...`). Stereo; listen engages band 3 (Left; itself
        // running in the EQ path or not); band 7 running on Stereo so the
        // NaN's lane goes non-finite in the same chunk. (Engage only: during
        // an established Left listen the right lane carries no EQ path, so a
        // NaN on R never reaches a lane.) The NaN sits inside the incoming
        // voice's ramp, on the Left channel (the voice and a lane both
        // non-finite) or on the Right channel (a lane only: a Left voice
        // renders 0 on the right). Both renders clear every state at the
        // NaN's chunk, so they are bit-equal from the next block on. With the
        // flag taken inside the `||`, the Left render keeps the flag and
        // clears again at the next clean chunk: the renders then differ.
        //======================================================================
        {
            Tally tally;
            juce::String figures;

            for (const int block : { 64, 512 })
                for (const int band3 : { 0, 1 })
                {
                    const double rate = 48000.0;
                    const int length = 12000, grid = harnessGrid (rate);
                    const int t1 = owedCeil (1001, grid), nanAt = t1 + 700;
                    const auto clean = makeNoiseAt (2, length, 0x33000001, 0.5f);
                    auto onLeft = clean, onRight = clean;
                    onLeft.setSample (0, nanAt, std::numeric_limits<float>::quiet_NaN());
                    onRight.setSample (1, nanAt, std::numeric_limits<float>::quiet_NaN());

                    const auto setup = owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.0f, 1, kLeft), owedSet (bandId (4, "enabled"), band3 ? 1.0f : 0.0f),
                                                  owedBand (7, kBell, 2500.0f, 4.0f, 1.5f), owedListen (-1) });
                    OwedOptions o;
                    o.block = block;
                    const std::vector<OwedEvent> events { { 1001, owedListen (3) } };
                    const auto yl = owedRender (rate, onLeft, setup, events, o);
                    const auto yr = owedRender (rate, onRight, setup, events, o);
                    const int blockEnd = 1001 + ((nanAt - 1001) / block + 1) * block;

                    // Preconditions: each render is non-finite inside the NaN's block (a lane took it).
                    int nonFiniteL = 0, nonFiniteR = 0;
                    for (int channel = 0; channel < 2; ++channel)
                        for (int i = nanAt; i < blockEnd; ++i)
                        {
                            nonFiniteL += std::isfinite (yl.getSample (channel, i)) ? 0 : 1;
                            nonFiniteR += std::isfinite (yr.getSample (channel, i)) ? 0 : 1;
                        }

                    int firstDiff = -1;
                    for (int channel = 0; channel < 2; ++channel)
                        for (int i = blockEnd; i < length; ++i)
                            if (! sameBits (yl.getSample (channel, i), yr.getSample (channel, i)))
                            {
                                if (firstDiff < 0 || i < firstDiff) firstDiff = i;
                                break;
                            }

                    const juce::String where = "blk " + juce::String (block) + (band3 ? " band 3 on" : " band 3 off");
                    tally.check (nonFiniteL > 0 && nonFiniteR > 0 && firstDiff < 0,
                                 where + ": non-finite in the NaN's block L / R " + juce::String (nonFiniteL) + " / " + juce::String (nonFiniteR)
                                     + ", first difference from " + juce::String (blockEnd) + ": " + juce::String (firstDiff));
                    figures += (figures.isEmpty() ? "" : ", ") + where + " equal from " + juce::String (blockEnd);
                }

            row (tally.bad == 0 && tally.total == 4,
                 "N1e-u  PLAN-3.3 P1: NaN inside listen's engage ramp with a lane also non-finite in that chunk (Left voice; NaN on L: voice + lane, on R: lane only), stereo, blocks 64 / 512, band 3 on / off, 48 kHz: the two renders are bit-equal from the block after the NaN (the voice flag is taken, not left set)",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // AO1 (probe33 a): the consumer registered, analyzerMode 0 for 1 s of
        // noise -> 0 / 0 samples drained; mode 2 the next second -> pre 0,
        // post = the second's sample count.
        //======================================================================
        {
            const double rate = 48000.0;
            const int second = 48000, block = 512;
            auto p = p33Make (rate, 2, block);
            setBand (*p, 1, makeSpec (kBell, 800.0f, -4.0f, 2.0f, 1, kStereo));
            p->analyzerMode.store (0);
            SpectrumAnalyzer analyzer (*p);
            auto input = makeNoiseAt (2, second, 3, 0.5f);
            juce::MidiBuffer midi;

            const auto runSecond = [&]
            {
                auto buffer = input;

                for (int s = 0; s < second; s += block)
                {
                    juce::AudioBuffer<float> view (buffer.getArrayOfWritePointers(), 2, s, juce::jmin (block, second - s));
                    p->processBlock (view, midi);

                    if (s % 4096 == 0)
                        analyzer.process (0.016);
                }

                analyzer.process (0.016);
            };

            runSecond();
            const auto off0 = analyzer.getSamplesDrained (0), off1 = analyzer.getSamplesDrained (1);
            p->analyzerMode.store (2);
            runSecond();
            const auto on0 = analyzer.getSamplesDrained (0), on1 = analyzer.getSamplesDrained (1);

            row (off0 == 0 && off1 == 0 && on0 == 0 && on1 == (std::uint64_t) second,
                 "AO1    analyzer Off stops the audio-thread writes (ROADMAP 3.3 criterion 1): consumer registered, mode 0 for 1 s of noise -> 0 / 0 drained; mode 2 the next second -> pre 0, post 48 000 (48 kHz, blocks 512)",
                 "off drained " + juce::String ((juce::int64) off0) + " / " + juce::String ((juce::int64) off1) + "; Post: pre " + juce::String ((juce::int64) on0)
                     + ", post " + juce::String ((juce::int64) on1));
        }

        //======================================================================
        // AO2 (PLAN-3.3 execute: the analyzer's Off rule). Post with noise for
        // 1 s, mode 0 for 3 s of silence (process every 1 / 60 s throughout),
        // then mode 2 with silence for 0.5 s: no Post frame may show pre-gap
        // audio, so every display point after the switch stays at least 55 dB
        // below the noise's highest point (the release alone takes 60 dB off
        // in the 3 s; a frame of pre-gap audio puts it back near the top).
        // Stimulus guard: during the noise second the display reaches above
        // -60 dB. (The rule's control is the mutant "Off rule removed".)
        //======================================================================
        {
            const double rate = 48000.0;
            const int block = 512;
            auto p = p33Make (rate, 2, block);
            p->analyzerMode.store (2);
            SpectrumAnalyzer analyzer (*p);
            juce::MidiBuffer midi;
            auto noise = makeNoiseAt (2, 48000, 5, 0.5f);
            juce::AudioBuffer<float> silence (2, block);
            const double tick = rate / 60.0;
            double clock = 0.0, nextTick = tick;
            int pos = 0;

            const auto runFor = [&] (double seconds, bool withNoise) -> float
            {
                const double end = clock + seconds * rate;
                float highest = -999.0f;

                while (clock < end)
                {
                    if (withNoise)
                    {
                        juce::AudioBuffer<float> view (noise.getArrayOfWritePointers(), 2, pos, block);
                        p->processBlock (view, midi);
                        pos = (pos + block) % (48000 - block);
                    }
                    else
                    {
                        silence.clear();
                        p->processBlock (silence, midi);
                    }

                    clock += block;

                    while (nextTick <= clock)
                    {
                        analyzer.process (1.0 / 60.0);
                        nextTick += tick;

                        for (int i = 0; i < SpectrumAnalyzer::numPoints; ++i)
                            highest = juce::jmax (highest, analyzer.getPost()[i]);
                    }
                }

                return highest;
            };

            const float during = runFor (1.0, true);
            p->analyzerMode.store (0);
            runFor (3.0, false);
            p->analyzerMode.store (2);
            const float after = runFor (0.5, false);

            row (during > -60.0f && after <= during - 55.0f,
                 "AO2    analyzer Off rule (PLAN-3.3 execute): Post with noise 1 s, Off 3 s of silence, then Post with silence: every Post point after the switch >= 55 dB below the noise's highest (no frame of pre-gap audio; the release takes 60 dB); the noise reached > -60 dB (stimulus guard)",
                 "highest Post point: during the noise " + fmtG (during, 4) + " dB, after the switch " + fmtG (after, 4) + " dB");
        }

        //======================================================================
        // M1 (PLAN-3.3 P4; RESEARCH-3.3 Q2): the shipping opeq::PeakHold and
        // gainToTenthsDb on the editor's 60 Hz reads, blocks completing in
        // real time (block b ends at (b + 1) B / fs), a -6 dBFS 1 kHz sine,
        // stereo, 3 s. Every tick after the first H reads shows -6 within
        // 0.5 dB on both channels. Vacuity guard: the same run with H = 1 at
        // 1024 / 44.1 kHz must show empty ticks (the floor, -60 dB).
        //======================================================================
        {
            Tally tally;
            juce::String figures;
            int emptyTicksDepth1 = 0;

            struct Case { double rate; int block; };

            for (const auto c : { Case { 44100.0, 1024 }, Case { 44100.0, 2048 }, Case { 44100.0, 4096 },
                                  Case { 48000.0, 1024 }, Case { 48000.0, 4096 }, Case { 192000.0, 2048 } })
                for (const bool depthOne : { false, true })
                {
                    if (depthOne && ! (c.rate == 44100.0 && c.block == 1024))
                        continue;

                    auto p = p33Make (c.rate, 2, c.block);
                    const int length = owedLength (3.0, c.rate);
                    const float amplitude = (float) std::pow (10.0, -6.0 / 20.0);
                    juce::AudioBuffer<float> input (2, length);

                    for (int i = 0; i < length; ++i)
                    {
                        const auto v = (float) (amplitude * std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * i / c.rate));
                        input.setSample (0, i, v);
                        input.setSample (1, i, v);
                    }

                    const int depth = depthOne ? 1 : opeq::PeakHold::depthFor (p->getBlockSize(), p->getSampleRate());
                    std::array<opeq::PeakHold, 2> hold;
                    for (auto& h : hold) h.setDepth (depth);

                    juce::MidiBuffer midi;
                    int done = 0, worstTenths = 0, worstDev = -1, empty = 0, ticks = 0;

                    for (int j = 1; j / 60.0 < 3.0; ++j)
                    {
                        const double t = j / 60.0;

                        while (done < length / c.block && (done + 1) * (double) c.block / c.rate <= t)
                        {
                            juce::AudioBuffer<float> view (input.getArrayOfWritePointers(), 2, done * c.block, c.block);
                            p->processBlock (view, midi);
                            ++done;
                        }

                        for (int ch = 0; ch < 2; ++ch)
                            hold[(size_t) ch].push (p->readAndResetOutputPeak (ch));

                        if (j <= depth)
                            continue;

                        ++ticks;

                        for (int ch = 0; ch < 2; ++ch)
                        {
                            const int tenths = opeq::gainToTenthsDb (hold[(size_t) ch].max(), -600, 120);

                            if (tenths <= -600)
                                ++empty;

                            if (worstDev < 0 || std::abs (tenths + 60) > worstDev)
                            {
                                worstDev = std::abs (tenths + 60);
                                worstTenths = tenths;
                            }
                        }
                    }

                    const juce::String where = fmtG (c.rate / 1000.0, 4) + "k / " + juce::String (c.block);

                    if (depthOne)
                    {
                        emptyTicksDepth1 = empty;
                        figures += "; H = 1 control at " + where + ": " + juce::String (empty) + " empty reads";
                        continue;
                    }

                    tally.check (worstDev >= 0 && worstDev <= 5 && ticks > 100,
                                 where + " H " + juce::String (depth) + ": worst " + fmtG (worstTenths / 10.0, 4) + " dB over " + juce::String (ticks) + " ticks");
                    figures += (figures.isEmpty() ? "" : ", ") + where + " H " + juce::String (depth) + " worst " + fmtG (worstTenths / 10.0, 4) + " dB";
                }

            row (tally.bad == 0 && tally.total == 6 && emptyTicksDepth1 >= 1,
                 "M1     UI-06 meter feed (PLAN-3.3 P4): opeq::PeakHold (H = ceil (B / fs x 60) + 1) on 60 Hz reads of readAndResetOutputPeak, blocks in real time, -6 dBFS 1 kHz sine, 3 s, 44.1 k / 1024, 2048, 4096; 48 k / 1024, 4096; 192 k / 2048: every tick after the first H within 0.5 dB of -6 on both channels; H = 1 at 1024 / 44.1 k shows empty reads (control)",
                 tally.detail() + "  " + figures);
        }
    }

    //==========================================================================
    // Stage 3, Phase 3.1 (PLAN-3.1 P6, call C2): the preset manager
    //==========================================================================

    /** The per-run temp home main() re-execs into (empty if none). */
    juce::File harnessHome()
    {
        const char* h = std::getenv ("OPEQ_HARNESS_HOME");
        return h != nullptr && *h != 0 ? juce::File (juce::String::fromUTF8 (h)) : juce::File();
    }

    /** R5: the presets directory must resolve under the temp home before
        ANY preset file is written. Anything else stops the run here. */
    void requirePresetsUnderHarnessHome (const Processor& p)
    {
        const auto home    = harnessHome();
        const auto presets = p.presetManager.getPresetsDirectory();
        const auto resolved = juce::File::getSpecialLocation (juce::File::userHomeDirectory);

        if (home == juce::File() || resolved != home || ! presets.isAChildOf (home))
        {
            std::printf ("FATAL: the presets directory %s is not under the harness home %s (resolved home %s): "
                         "refusing to write presets -- STOP\n",
                         presets.getFullPathName().toRawUTF8(), home.getFullPathName().toRawUTF8(),
                         resolved.getFullPathName().toRawUTF8());
            std::fflush (stdout);
            std::exit (3);
        }
    }

    void presetRowsXr()
    {
        section ("XR-pm / XR-pmd / XR-psg. the preset manager (Phase 3.1: PLAN-3.1 P6, call C2; temp home)");

        //======================================================================
        // XR-pm: round trip through the real save / load path. Everything but
        // the two excluded parameters comes back bit-equal (normalised); the
        // excluded two keep the values set after the save; the UI atomics and
        // listenBand are not touched by a preset. Control: the same load on a
        // processor with no exclusions moves output_gain (the reset pass sets
        // the default, the file has no key for it), so it is the exclusion
        // that holds it.
        //======================================================================
        {
            auto p = make();
            requirePresetsUnderHarnessHome (*p);

            const auto ui = makeUi (1, 3, 30, 7, 1111, 777);
            setUi (*p, ui);
            p->listenBand.store (5);

            setAllRandom (*p, 0x3150a1);
            const int appliedFirst = countNonDefault (*p);
            const auto saved = valuesOf (*p);
            const bool savedOk = p->presetManager.savePreset ("t31");
            const bool fileOk  = p->presetManager.getUserPresetsDirectory().getChildFile ("t31.json").existsAsFile();

            setAllRandom (*p, 0x3150b2);
            setRaw (*p, "output_gain", 7.0f);
            setNormalised (*p, "bypass", 1.0f);
            // What the host path stored (+7 dB round-trips through the
            // normalised value: 6.99999905), the reference for "held".
            const float gainSet = rawOf (*p, "output_gain");
            const auto beforeLoad = valuesOf (*p);
            const bool loadedOk = p->presetManager.loadPreset ("t31");

            const auto params = rangedParams (*p);
            const auto now = valuesOf (*p);
            Tally tally;
            int changedBySecondDraw = 0;

            for (size_t i = 0; i < params.size() && i < saved.size() && i < now.size(); ++i)
            {
                const auto id = params[i]->getParameterID();

                if (id == "output_gain" || id == "bypass")
                    continue;

                if (! sameBits (beforeLoad[i], saved[i]))
                    ++changedBySecondDraw;

                tally.check (sameBits (now[i], saved[i]),
                             id + " saved " + juce::String (fmt ((double) saved[i])) + " got " + juce::String (fmt ((double) now[i])));
            }

            const float gainAfter = rawOf (*p, "output_gain");
            const float bypassAfter = rawOf (*p, "bypass");
            const bool excludedHeld = sameBits (gainAfter, gainSet) && std::abs (gainSet - 7.0f) < 1.0e-3f
                                   && sameBits (bypassAfter, 1.0f);
            const bool uiHeld = uiOf (*p) == ui && p->listenBand.load() == 5;
            const bool nameOk = p->presetManager.getCurrentPresetName() == "t31";
            const bool vacuityOk = appliedFirst >= kMinNonDefault && changedBySecondDraw >= kMinNonDefault;

            // Control: no exclusions on a second processor, same file.
            auto q = make();
            requirePresetsUnderHarnessHome (*q);
            q->presetManager.setExcludedParameters ({});
            q->presetManager.setValidatedApply (true);
            setRaw (*q, "output_gain", 7.0f);
            const float controlSet = rawOf (*q, "output_gain");
            const bool controlLoaded = q->presetManager.loadPreset ("t31");
            const float controlGain = rawOf (*q, "output_gain");
            const bool controlMoved = controlLoaded && ! sameBits (controlGain, controlSet);

            row (savedOk && fileOk && loadedOk && tally.bad == 0 && tally.total == kExpectedParams - 2
                     && excludedHeld && uiHeld && nameOk && vacuityOk && controlMoved,
                 "XR-pm   preset round trip: 194 bit-equal, output_gain / bypass / UI / listen held",
                 tally.detail() + "; output_gain " + juce::String (fmt ((double) gainAfter)) + " dB, bypass "
                     + juce::String (fmt ((double) bypassAfter)) + "; ui " + uiOf (*p).describe()
                     + ", listen " + juce::String (p->listenBand.load()) + "; name '" + p->presetManager.getCurrentPresetName()
                     + "'; second draw moved " + juce::String (changedBySecondDraw)
                     + "; control (no exclusions) output_gain " + juce::String (fmt ((double) controlSet)) + " -> "
                     + juce::String (fmt ((double) controlGain)) + " dB"
                     + (savedOk && fileOk && loadedOk ? "" : "; save / load failed"));
        }

        //======================================================================
        // XR-pmd (WR-01): a hand-written preset with one key. Every other
        // non-excluded parameter is reset to its default (within 1e-6
        // normalised: the reset goes through setValueNotifyingHost), the one
        // key is applied, the excluded two keep their values.
        //======================================================================
        {
            auto p = make();
            requirePresetsUnderHarnessHome (*p);

            const auto file = p->presetManager.getUserPresetsDirectory().getChildFile ("pmd.json");
            file.getParentDirectory().createDirectory();
            const bool written = file.replaceWithText ("{\n  \"parameters\": { \"b03_gain\": 0.8125 },\n"
                                                       "  \"version\": \"0.7.0\",\n  \"plugin\": \"O-ParametricEQ\"\n}\n",
                                                       false, false, "\n");

            setAllRandom (*p, 0x3150c3);
            const int appliedFirst = countNonDefault (*p);
            const float gainBefore = rawOf (*p, "output_gain");
            const float bypassBefore = rawOf (*p, "bypass");
            const bool loadedOk = p->presetManager.loadPreset ("pmd");

            Tally tally;

            for (auto* r : rangedParams (*p))
            {
                const auto id = r->getParameterID();

                if (id == "output_gain" || id == "bypass")
                    continue;

                const float want = id == "b03_gain" ? 0.8125f : r->getDefaultValue();
                tally.check (std::abs (r->getValue() - want) <= 1.0e-6f,
                             id + " want " + juce::String (fmt ((double) want)) + " got " + juce::String (fmt ((double) r->getValue())));
            }

            const bool excludedHeld = sameBits (rawOf (*p, "output_gain"), gainBefore) && sameBits (rawOf (*p, "bypass"), bypassBefore);

            row (written && loadedOk && tally.bad == 0 && tally.total == kExpectedParams - 2 && excludedHeld
                     && appliedFirst >= kMinNonDefault,
                 "XR-pmd  a one-key preset: the rest at default, output_gain / bypass held",
                 tally.detail() + "; non-default before the load " + juce::String (appliedFirst)
                     + (excludedHeld ? "" : "; an excluded parameter moved") + (written && loadedOk ? "" : "; write / load failed"));
        }

        //======================================================================
        // XR-psg: PresetSaveGuard with a factory preset on disk. The name and
        // the Factory/ file are refused, a user name is not. Control: with the
        // file absent the name is not refused (the Factory/ LOCATION is
        // refused whatever is in it, by design: not part of the control).
        //======================================================================
        {
            auto p = make();
            requirePresetsUnderHarnessHome (*p);
            auto& pm = p->presetManager;

            const auto factoryFile = pm.getFactoryPresetsDirectory().getChildFile ("Locked.json");
            factoryFile.getParentDirectory().createDirectory();
            const bool placed = factoryFile.replaceWithText ("{ \"parameters\": {}, \"plugin\": \"O-ParametricEQ\" }\n",
                                                             false, false, "\n");

            const auto userLocked = pm.getUserPresetsDirectory().getChildFile ("Locked.json");
            const auto userLockedNoExt = pm.getUserPresetsDirectory().getChildFile ("Locked");
            const auto userMine = pm.getUserPresetsDirectory().getChildFile ("Mine.json");

            const bool isFactory   = pm.isFactoryPreset ("Locked");
            const bool nameRefused = opeq::presetSaveRefusal (userLocked, pm).isNotEmpty()
                                  && opeq::presetSaveRefusal (userLockedNoExt, pm).isNotEmpty();
            const bool fileRefused = opeq::presetSaveRefusal (factoryFile, pm).isNotEmpty();
            const bool userAllowed = opeq::presetSaveRefusal (userMine, pm).isEmpty();
            const bool moduleRefuses = ! pm.savePreset ("Locked") && ! userLocked.existsAsFile();

            const bool removed = factoryFile.deleteFile();
            const bool controlNotFactory = ! pm.isFactoryPreset ("Locked");
            const bool controlAllowed = opeq::presetSaveRefusal (userLocked, pm).isEmpty();

            row (placed && isFactory && nameRefused && fileRefused && userAllowed && moduleRefuses
                     && removed && controlNotFactory && controlAllowed,
                 "XR-psg  PresetSaveGuard: factory name and Factory/ file refused, user name allowed",
                 juce::String ("isFactory ") + (isFactory ? "1" : "0") + ", name " + (nameRefused ? "refused" : "ALLOWED")
                     + ", file " + (fileRefused ? "refused" : "ALLOWED") + ", Mine " + (userAllowed ? "allowed" : "REFUSED")
                     + ", savePreset (Locked) " + (moduleRefuses ? "refused" : "WROTE")
                     + "; control (file absent): isFactory " + (controlNotFactory ? "0" : "1")
                     + ", name " + (controlAllowed ? "allowed" : "REFUSED"));
        }
    }
}

//==============================================================================
int main (int argc, char** argv)
{
   #if defined (__APPLE__)
    // PLAN-3.1 P6 / R5: the preset rows write files, so every run re-execs
    // once with a per-run temp home (the O-Granulator pattern). JUCE resolves
    // home through NSHomeDirectory(), which honours CFFIXED_USER_HOME only
    // when it is set at launch; HOME alone is ignored. presetRowsXr() checks
    // the resolved presets directory is under it before the first write.
    if (std::getenv ("OPEQ_HARNESS_HOME") == nullptr)
    {
        const char* td = std::getenv ("TMPDIR");
        std::string tmpl = std::string (td != nullptr ? td : "/tmp/") + "/opeq-harness-home-XXXXXX";

        if (mkdtemp (tmpl.data()) == nullptr) { std::printf ("FATAL: mkdtemp failed\n"); return 3; }

        char exe[4096];
        std::uint32_t exeLen = sizeof (exe);

        if (_NSGetExecutablePath (exe, &exeLen) != 0) { std::printf ("FATAL: executable path\n"); return 3; }

        setenv ("OPEQ_HARNESS_HOME", tmpl.c_str(), 1);
        setenv ("CFFIXED_USER_HOME", tmpl.c_str(), 1);
        setenv ("HOME", tmpl.c_str(), 1);
        std::fflush (stdout);
        execv (exe, argv);
        std::printf ("FATAL: re-exec failed\n");
        return 3;
    }
   #endif

    // The temp home (if any) is removed at exit.
    struct HomeCleanup
    {
        juce::File dir;
        ~HomeCleanup() { if (dir != juce::File() && dir.getFileName().startsWith ("opeq-harness-home-")) dir.deleteRecursively(); }
    } homeCleanup { harnessHome() };

    bool dumpParams = false;
    bool rows33Only = false;
    juce::File dumpSessionsDir;
    juce::File goldenDir (juce::String::fromUTF8 (GOLDEN_DIR));

    for (int a = 1; a < argc; ++a)
    {
        const auto arg = juce::String::fromUTF8 (argv[a]);

        if (arg == "--dump-params")
        {
            dumpParams = true;
        }
        else if (arg == "--golden-dir" && a + 1 < argc)
        {
            goldenDir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
        }
        else if (arg == "--long")
        {
            longRunRequested = true;
        }
        else if (arg == "--dump-sessions" && a + 1 < argc)
        {
            dumpSessionsDir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
        }
        else if (arg == "--lid-dir" && a + 1 < argc)
        {
            lidDumpDir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
        }
        else if (arg == "--rows33")
        {
            rows33Only = true;   // PLAN-3.3 T11 mutant runs: only phase33RowsXr
        }
        else if (arg == "--dump-g9" && a + 1 < argc)
        {
            g9DumpDir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
        }
        else
        {
            std::fprintf (stderr, "usage: %s [--dump-params] [--golden-dir <dir>] [--long] [--dump-sessions <dir>] [--lid-dir <dir>] [--dump-g9 <dir>] [--rows33]\n", argv[0]);
            return 255;
        }
    }

    if (lidDumpDir == juce::File())
        if (const char* fromEnvironment = std::getenv ("OPEQ_LID_DIR"))
            if (*fromEnvironment != 0)
                lidDumpDir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (fromEnvironment));

    juce::ScopedJuceInitialiser_GUI juceInit;

    if (dumpSessionsDir != juce::File())
        return dumpLidSessions (dumpSessionsDir);   // PLAN-2.6 T1, P13

    if (g9DumpDir != juce::File())
    {
        // PLAN-3.2 P4: only displayRowsXr's G9 jobs; it exits from there.
        if (! g9DumpDir.createDirectory())
        {
            std::printf ("FATAL: cannot create %s\n", g9DumpDir.getFullPathName().toRawUTF8());
            return 255;
        }

        displayRowsXr();
        std::printf ("FATAL: --dump-g9 reached the end of displayRowsXr\n");
        return 1;
    }

    if (dumpParams)
    {
        // stdout only, "\n" endings only: the caller redirects this into
        // tests/golden/params.tsv.
        auto p = make();
        std::cout << buildDump (*p);
        std::cout.flush();
        return 0;
    }

    std::printf ("O-ParametricEQ render harness (Stage 1)  golden dir: %s\n", goldenDir.getFullPathName().toRawUTF8());

    if (rows33Only)
    {
        phase33RowsXr();
        std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
        std::fflush (stdout);
        return juce::jmin (failures, 255);
    }

    layoutRows (goldenDir);
    busRows();
    passThroughRows();
    stateRows();

    // Stage 2, Phase 2.2 (PLAN-2.2 T7)
    responseRows();
    nyquistRows();
    transparencyRows();
    latencyRows();
    allocationRows();
    blockAndStateRows();
    cpuInfo();

    // Stage 2, Phase 2.3 (PLAN-2.3 T7)
    sweepRows();
    carriedGapRows();
    blockAndTickRows();
    stressRows();
    stressAllocationRows();
    noiseFloorRows();
    cpuInfoSweeping();

    // Stage 2, Phase 2.4 (PLAN-2.4 T7)
    stepRowsXr();
    fadeReferenceRowsXr();
    transparencyRowsXr();
    liveSlopeRowsXr();
    bypassRowsXr();
    structuralBlockRowsXr();
    fadeAllocationRowsXr();
    cpuInfoFading();

    // Stage 2, Phase 2.5 (PLAN-2.5 T7)
    gainG12RowsXr();
    gainStepRowsXr();
    gainSweepRowsXr();
    stageTransparencyRowsXr();
    stageSeedRowsXr();
    gainBlockRowsXr();
    meterRowsXr();
    gainAllocationRowsXr();
    cpuInfoGains();

    // Stage 2, Phase 2.6 (PLAN-2.6 T9)
    displayRowsXr();
    analyzerRowsXr();
    listenStepRowsXr();
    listenPathRowsXr();
    listenBlockRowsXr();
    lidRowsXr();
    owedRowsXr();

    // Stage 3, Phase 3.1 (CONTEXT U8: VERIFICATION-2.6 Issue 2)
    owed26RowsXr();

    // Stage 3, Phase 3.1 (PLAN-3.1 P6, call C2)
    presetRowsXr();

    // Stage 3, Phase 3.3 (PLAN-3.3 T3)
    phase33RowsXr();

    listenAllocationRowsXr();
    cpuInfoListen();

    std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
    std::fflush (stdout);
    return juce::jmin (failures, 255);
}
