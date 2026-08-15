---
plugin: O-Octagon
stage: 4
stage_name: polish
stage_phase: 2                             # DECIDED AT THE 4.1 BOUNDARY (D1). Stage 4 is TWO phases,
stage_phase_name: host-and-ear             # not the "single pass" ROADMAP said: 4.1 = machine
stage_phase_total: 2                       # (CI, Windows, pluginval/auval, presets, COMPAT-04,
artifact_suffix: "-4.2"                    # docs), 4.2 = host-and-ear against a FROZEN 4.1 binary
phase: verify                              # STAGE-4 ROLL-UP RE-VERIFY LANDED 2026-08-14 (evening).
status: stage_4_complete                   # VERIFICATION.md REWRITTEN — supersedes the morning
                                           # PARTIAL roll-up (preserved at commit b628f4b5).
                                           # VERDICT: ✅ VERIFIED. STAGE 4 COMPLETE — ALL FOUR
                                           # STAGES COMPLETE. Both morning blockers discharged by
                                           # measurement, and every machine-checkable figure was
                                           # RE-MEASURED at the boundary, not inherited: four
                                           # contract checksums exact; Source/ unmoved since the
                                           # freeze; installed VST3 928cd447… / AU cc54db02… EXACT;
                                           # 95 probes 0 failures; 70 JS sections; Gate 9 as its
                                           # literal string (102 cases); selftest 24 cases; Gate 25
                                           # --check 3 runs re-derived AND its negative control
                                           # exit 1; auval PASS re-run at the boundary; CR-a
                                           # RE-DERIVED from its checksum-exact staged WAV both
                                           # ways (as spelled: FAILED exit 1 with the identical
                                           # mismatch; against the measured order: OK, 1,2,3,4,
                                           # 7,8,5,6 @ 158.3 dB) — every transcribed figure
                                           # reproduces including the failure; NC4 re-derived at
                                           # BOTH operating points, exact; all 7 session-WAV
                                           # sha256s + both Gate-13 PNGs exact; ledger recounted
                                           # 30/30 from the status column, and it now AGREES with
                                           # the completion signal. Residuals carried, none a
                                           # blocker: Gate 20 loopback delta (owner operator),
                                           # Gate 22 revealing-monitoring audition (operator, low),
                                           # D11 physical half (owner none), the 4-item v1.1 tool
                                           # register, session WAVs outside the repo unpinned
                                           # (checksums re-verified exact here).
                                           # NEXT: /install-plugin O-Octagon.
                                           #
                                           # superseded note kept for history:
                                           # BLOCK C PART 2 COMPLETE 2026-08-14 — gates 17-25 ALL
                                           # RAN (evidence/session-gates-4.2.txt, close block at
                                           # the end of the file). ALL 14 SESSION GATES RECORDED.
                                           #
                                           # THE STAGE GOAL'S SECOND CLAUSE IS MET: Logic's
                                           # canonical interleaved 7.1 BOUNCE order = 1,2,3,4,7,8,5,6
                                           # (Gate 17 / CR-a, 158.3 dB min isolation), CONFIRMED by
                                           # Gate 18 / CR-b returning the before-the-bounce
                                           # prediction 2,3,4,7,8,5,6,1 EXACTLY. NOT the device
                                           # order (CT: 1,2,5,6,7,8,3,4) — three measured orders now
                                           # coexist (buffer/device/bounce); never conflate them.
                                           #
                                           # QUAL-01's audible clause CONCLUDED (Gate 22): machine
                                           # null-scan at sample resolution found no step, gesture
                                           # proven non-vacuous (tilt onset 12.0 s vs 12.1 s
                                           # predicted); operator PASS; monitoring MacBook Pro
                                           # speakers, recorded as weak — machine half carries it.
                                           #
                                           # THE RUNBOOK WAS WRONG THREE TIMES, EACH CAUGHT BEFORE
                                           # IT COST A RESULT: Gate 18's --expect was stale
                                           # (derived from the identity CR-a falsified); NC4 as
                                           # spelled CANNOT fire (missing dHull>0 precondition —
                                           # sources at centre have the air filter structurally
                                           # inert); the manifest can hold only ONE lfe run and a
                                           # second SILENTLY EVICTS the first (would have destroyed
                                           # Gate 20's CS entry). Four tool defects consolidated in
                                           # the close block + NOTES.md v1.1 register; NO tool
                                           # assertion edited after grading (refused twice already).
                                           #
                                           # Gate 20: bounce path PASS (LFE byte-identical to an
                                           # ordinary speaker); loopback delta NOT MEASURED —
                                           # devices 1-2 destroyed by the documented Software-
                                           # Monitoring feedback loop; operator-accepted, residual
                                           # owner: operator. D16 never invoked. NC4 validated the
                                           # null at TWO operating points (air 1.00 worst 0.00 dB,
                                           # air 0.35 worst 0.03 dB vs the TPT model, fc derived
                                           # forward from geometry both times).
                                           #
                                           # OWED: the stage-4 roll-up RE-VERIFY (the standing
                                           # VERIFICATION.md verdict PARTIAL predates Block C part
                                           # 2 and is now stale). THEN /install-plugin.
                                           #
                                           # superseded note kept for history:
                                           # STAGE-4 ROLL-UP VERIFY LANDED 2026-08-14 —
                                           # VERIFICATION.md. VERDICT: PARTIAL. STAGE 4 IS NOT
                                           # COMPLETE and the plugin is NOT ready to ship.
                                           # (next_action is set below, at its canonical key — a
                                           # second one here would be a DUPLICATE YAML KEY, the
                                           # same trap caught and reverted at the 4.2 discuss
                                           # boundary.)
                                           #
                                           # RE-MEASURED AT THAT BOUNDARY (not inherited): four
                                           # contract checksums exact; Source/ unmoved since the
                                           # freeze; both installed binaries EXACT vs the freeze
                                           # record; 95 probes 0 failures; 70 JS gate sections;
                                           # Gate 9 run as its LITERAL recorded string (exit 0,
                                           # 102 cases — the 4.2 Issue-1 spelling defect is
                                           # genuinely closed); selftest 24 cases; Gate 25 --check
                                           # exit 0 WITH its negative control exiting 1; Gate 12's
                                           # probe re-derived; and GATE 16 / CT FULLY RE-DERIVED
                                           # from the capture WAVs — every isolation figure, the
                                           # period, the length and the sequence 1,2,5,6,7,8,3,4
                                           # reproduce EXACTLY, including the tool's FAILED verdict.
                                           # Gate 16's routing argument was checked against JUCE
                                           # source itself (create7point1 order + enum bits
                                           # 10/11/20/21) and holds: the permutation is the host's,
                                           # not a plugin defect.
                                           #
                                           # THE STAGE GOAL'S SECOND CLAUSE IS UNMET: "with the
                                           # bounce path confirmed" is Gate 17 (CR-a) and it did
                                           # NOT run. CT measured the DEVICE order; the BOUNCE
                                           # order is a separate measurement and is unmeasured.
                                           #
                                           # THE 30/0/0 LEDGER IS NOT THE COMPLETION SIGNAL.
                                           # QUAL-01 criterion 2's audible clause is UNCONCLUDED
                                           # and rides Gate 22. Do NOT run /install-plugin.
                                           #
                                           # OWED AT THE BLOCK C CLOSE: (1) FINDING 1 IS CLOSED
                                           # 2026-08-14 — Gate 13's two banner screenshots are
                                           # now COMMITTED as evidence/gate13-banner-room.png and
                                           # evidence/gate13-banner-venue.png, taken in LOGIC PRO
                                           # on the 7.1 surround track (host confirmed by the
                                           # operator, ASKED not inferred — a Standalone capture
                                           # on BlackHole would read "7.1 Surround" too and would
                                           # evidence nothing about what Logic negotiated). Both
                                           # show NO SAFE and NO MAP banner + "Set 7.1 Surround";
                                           # stages/3-gui/evidence/standalone-verify-3.3.png is
                                           # the pre-existing negative control showing BOTH
                                           # banners present, so banner-absence discriminates.
                                           # Gate 14 half 1 no longer inherits an unevidenced
                                           # observation. STILL OWED: (2) the ping-mode --expect
                                           # decision (Finding 3); (3) Gate 23's throttling-
                                           # recovery relabelling, at its third boundary
                                           # (Finding 4). Both require gates that have NOT run,
                                           # so writing either now would assert an unrun gate.
                                           #
                                           # superseded note kept for history:
                                           # BLOCK C PART 1 — gates 12-16 of 25 are DONE and
                                           # committed; gates 17-25 have NOT run. See
                                           # evidence/session-gates-4.2.txt for the gate-by-gate
                                           # record and evidence/BLOCK-C-RUNBOOK.md for the
                                           # remaining procedure.
                                           #
                                           # *** COMPAT-02 IS CLOSED, 3 of 3, 2026-08-14. ***
                                           # The ledger moved for the first time in 4.2: 30/0/0,
                                           # openRows empty. Gate 13 (getStatus: safeMode false,
                                           # mapInvalid false, 8 ch, "7.1 Surround"), Gate 12
                                           # (probe: 8 ch / 48 kHz / 24-bit), Gate 14 (recall
                                           # survives save-quit-reopen), Gate 15 (all 11 lanes
                                           # written AND read back), Gate 16 / CT (8 distinct
                                           # channels, one per window, isolation 219.9 dB vs a
                                           # 40 dB floor).
                                           #
                                           # STAGE 4 IS STILL NOT COMPLETE. Do NOT run
                                           # /install-plugin. Nine gates remain and QUAL-01
                                           # criterion 2's audible clause is unconcluded.
                                           #
                                           # NOTHING FROM THIS SESSION HAS BEEN RE-RUN AT A VERIFY
                                           # BOUNDARY. Every figure above is an EXECUTE-side
                                           # measurement and is owed a from-scratch re-run.
                                           #
                                           # superseded note kept for history:
                                           # 4.2 VERIFIED 2026-08-13 — VERIFICATION-4.2.md.
                                           # VERDICT: PARTIAL. Blocks A+B verified (all 11 desk
                                           # gates RE-RUN FROM SCRATCH at verify, all green, incl.
                                           # NC1 executed mutation-and-revert and a THIRD full
                                           # rebuild reproducing both bundle checksums). Block C —
                                           # the Logic session, gates 12-25 — HAS NOT RUN. COMPAT-02
                                           # is still 0 of 3 and the ledger did NOT move: 29/0/1.
                                           # STAGE 4 IS NOT COMPLETE. Do NOT run /install-plugin.
                                           # NOTE: this field said `phase: plan / phase_complete`
                                           # until now — neither execute commit (378fb4cd, 300d8cf0)
                                           # touched STATUS.md, so a resume would have re-run execute
                                           # from Task 1 and re-cut the freeze. Recorded as Issue 2.
last_updated: 2026-08-14
branch: feat/o-octagon
complexity_tier: 6
complexity_score: 5.0
research_depth: DEEP
staged_implementation: true
orchestration_mode: true
next_action: install_plugin
                                         # ── WHAT IS ACTUALLY NEXT (2026-08-14, stage 4 verified) ──
                                         # /install-plugin O-Octagon. The stage-4 roll-up re-verify
                                         # is DONE (VERIFICATION.md, verdict ✅ VERIFIED — all four
                                         # stages complete, no blockers). NOTE FOR INSTALL: the
                                         # frozen -dev bundles are ALREADY installed and measured
                                         # byte-identical to the freeze (VST3 928cd447… /
                                         # AU cc54db02…); every Block C gate ran against them.
                                         # /install-plugin formalises the lifecycle/registry step
                                         # and must not rebuild past freeze 378fb4cd without cause.
                                         #
                                         # superseded note kept for history:
                                         # run_stage_4_rollup_reverify —
                                         # ── (2026-08-14, Block C closed) ──
                                         # /plugin-verify O-Octagon 4-polish — re-roll the stage
                                         # verify. The standing VERIFICATION.md verdict (PARTIAL)
                                         # predates Block C part 2: its two blockers — gates 17-25
                                         # unrun, bounce clause unmet — are both discharged in
                                         # evidence/session-gates-4.2.txt. After a clean verify:
                                         # /install-plugin O-Octagon.
                                         #
                                         # superseded note kept for history:
                                         # RESUME AT GATE 17 (CR-a). Gates 12-16 are DONE.
                                         # Nine gates remain, est. 45-60 min:
                                         #   17 CR-a  bounce order, shipped default venue
                                         #   18 CR-b  same under cr-b-permuted.venue, 8-cycle
                                         #   19 NC2+NC3  the controls that prove 17/18 can fail
                                         #   20 CS    LFE on BOTH bounce and realtime loopback
                                         #   21 NC4   airAmount confound control, BEFORE any D16
                                         #   22 CU    the audible clause — NAME the headphones
                                         #   23 interactive drive + throttling-recovery relabel
                                         #   24 User/ presets byte-identical
                                         #   25 analyse_bounce.py --check
                                         #
                                         # ── DESK BOUNDARY 2026-08-14, between session parts ──
                                         # NO GATE RAN. Gates 17-25 are still NOT RUN. Two defects
                                         # were fixed that would have corrupted the session:
                                         #
                                         # 1. GATE 25 WAS UNPASSABLE BY CONSTRUCTION. --emit-json
                                         #    recorded bare basenames and --check resolved them
                                         #    relative to the MANIFEST (inside the repo), while the
                                         #    runbook mandates every WAV live OUTSIDE it. Proven,
                                         #    not inferred: bare --check exited 1 on the one
                                         #    recorded run. Fixed BEFORE the session on purpose —
                                         #    left to Gate 25, all eight session runs would have
                                         #    baked in unresolvable paths. Runs now record
                                         #    input_dir; --session-root DIR overrides. ONLY path
                                         #    resolution changed; no assertion was loosened.
                                         #    selftest 24/24, Gate 12 probe re-derived exactly,
                                         #    bare --check exit 0, and the anti-vacuity control
                                         #    (--session-root /tmp/nonexistent) still exits 1.
                                         #    analyse_bounce.py is a TEST TOOL — it cannot enter
                                         #    the binary, so the freeze is intact (re-verified:
                                         #    Source/ unmoved, both installed binaries still match
                                         #    the freeze checksums exactly).
                                         #
                                         # 2. session-gates-4.2.txt CONTRADICTED ITSELF — its
                                         #    closing summary still asserted COMPAT-02 0 of 3 and a
                                         #    29/0/1 ledger, stale since gates 12-16 ran. A verify
                                         #    taking the pessimistic side would have REOPENED a
                                         #    correctly closed row. Corrected, original preserved.
                                         #
                                         # GATE 20's SPELLING IS NOW CORRECTED IN THE RUNBOOK
                                         # (loopback = --channels 6,1 from CT's device order; the
                                         # bounce path left as a placeholder to be re-derived from
                                         # CR-a). PLAN-4.2.md deliberately NOT edited (P109).
                                         #
                                         # *** READ BEFORE GATE 20 — CS's SPELLING IS WRONG ***
                                         # *** (corrected in BLOCK-C-RUNBOOK.md at the 08-14 desk ***
                                         # *** boundary; the original analysis is kept below)    ***
                                         # PLAN-4.2 spells CS as `--mode lfe --channels 4,1`. CT
                                         # measured the device order as L R Lrs Rrs C Lfe Lss Rss,
                                         # so in a DEVICE-order capture channel 4 is Rrs and the
                                         # LFE is at channel 6. Run as spelled it would measure a
                                         # rear surround against L, find no bass-management
                                         # signature, and report a CONFIDENT WRONG ANSWER. NC4 does
                                         # not cover this door. --channels MUST be re-derived per
                                         # path: from CR-a's result for the bounce, from CT's for
                                         # the realtime loopback. A Logic bounce writes channels per
                                         # the file's layout tag and NEED NOT match the device
                                         # order — that is exactly why CR-a is a separate gate.
                                         #
                                         # CR-a's own `--expect 1,2,3,4,5,6,7,8` may likewise not
                                         # hold. Run it AS SPELLED first and record what comes back;
                                         # do NOT pre-load the CT permutation, which would fit the
                                         # expectation to data from a different path.
                                         #
                                         # RIG FACTS ESTABLISHED THIS SESSION — carry them:
                                         #  - Instantiate ONLY via the insert slot's `Stereo -> 7.1`
                                         #    entry. Clicking the plugin NAME gives MULTI-MONO:
                                         #    eight mono instances, each raising both banners
                                         #    correctly. A surround-INPUT track can never work —
                                         #    isBusesLayoutSupported rejects input wider than stereo.
                                         #  - Logic's global "Software Monitoring" MUST be OFF, or
                                         #    BlackHole out->in->out feeds back and pins channels
                                         #    1-2 to infinity. Per-track I buttons undo themselves
                                         #    on each arm; disable it globally.
                                         #  - Project sample rate 48 kHz; the tone set is 48 kHz.
                                         #  - Session sources are staged OUTSIDE the repo at
                                         #    ~/Dev/octagon-4.2-session/sources (8 tones, the LFE
                                         #    multitone + sidecar, the CU audible probe). Keep every
                                         #    WAV there: the gitignore rule is
                                         #    plugins/*/.planning/evidence/**/*.wav and does NOT
                                         #    match this phase's stages/4-polish/evidence/ path.
                                         #  - Manifest: evidence/bounce-manifest.json (the tool's
                                         #    own DEFAULT_MANIFEST, so gate 25's bare --check finds
                                         #    it). 1 run recorded so far (Gate 12's probe).
                                         #
                                         # There is NO 4.3 (D18): Block C completes 4.2, which
                                         # completes Stage 4. A D16 finding re-enters Block B with a
                                         # SECOND freeze rather than opening a phase.
                                         # RUNS AGAINST THE FREEZE, NOT AGAINST WHATEVER IS ON DISK:
                                         #   commit 378fb4cdc70ef7e7b4523771dd4f014f189246ec
                                         #   VST3   928cd447c57435c93554fbb90fd14ec035cd39e8a8db54a5aba37a1597e0bb42
                                         #   AU     cc54db026875173e47daf691228c4c80c52da4c9050880aea0976bc16fe1fc99
                                         # Verified at 4.2 verify: Source/ has NOT moved since the
                                         # freeze, and the binary rebuilds byte-identically from
                                         # source on a fresh rm -rf build. A mismatch in the session
                                         # is a REAL SIGNAL, not build nondeterminism.
                                         # FIVE THINGS TO RE-READ BEFORE STARTING:
                                         #  1. airAmount = 0 on CR-a/CR-b/CT/CS, NEVER on CU. An
                                         #     airAmount HF delta reads exactly like bass management
                                         #     and would trigger D16's re-freeze on nothing.
                                         #  2. CS runs under the CR-a IDENTITY venue — under CR-b,
                                         #     speaker 4 is not the LFE slot.
                                         #  3. NC4 runs BEFORE any D16 disposition.
                                         #  4. Gates 12 and 13 STOP the phase; they cost 2 minutes.
                                         #  5. No Source/ edit — anything needing one re-enters
                                         #     Block B with a SECOND freeze.
                                         # TWO THINGS OWED AT BLOCK C's CLOSE (4.2 verify issues):
                                         #  - Gate 9's spelling in desk-gates-4.2.txt: recorded as
                                         #    `gen_dbap_reference.py --check`, which EXITS 2. --output
                                         #    is required=True. Same defect class as 4.1's Issue 1.
                                         #  - Gate 23's relabelling sentence (hidden-editor check is
                                         #    THROTTLING-RECOVERY, and cannot drop a completion) is
                                         #    not yet carried by any artifact.
                                         # ── superseded plan-boundary note below, kept for history ──
                                         # 4.2 PLAN COMPLETE 2026-08-13 — PLAN-4.2.md, P101-P112,
                                         # 15 tasks in THREE BLOCKS (desk / re-freeze / session), 25
                                         # gates, NC1-NC4. Entry check green and NO CONTRACT AMENDED
                                         # — the first Stage-4 plan boundary where that is true.
                                         # THE PHASE IS NO LONGER ONE SESSION (D18 corrected): N8 and
                                         # N10 moved work to the desk, and D19 edits Source/, so P110
                                         # schedules EXACTLY ONE RE-FREEZE at the end of block B and
                                         # forbids any Source/ edit after it. A D16 finding re-enters
                                         # block B rather than opening a 4.3. JS gate sections move
                                         # 69 -> 70 (P101's new layout section), STATED HERE so a
                                         # re-run against 69 is not misread as a failure.
                                         # FOUR DECISIONS THE RESEARCH DID NOT ASK FOR:
                                         #  - P109: the re-spelled Gate 16b would COUNT ITS OWN
                                         #    DOC-COMMENT. Measured at this boundary — the receiver-
                                         #    agnostic regex returns TWO hits, one being
                                         #    PresetPolicy.h:202's description of the gate. De-match
                                         #    the comment, THEN assert exactly 1; otherwise the gate
                                         #    is off by one on day one and the next reader loosens it
                                         #    to 2, making a real second call site invisible
                                         #  - P111: a bounce measures the RENDER path only. The :84
                                         #    claim is about what reaches an output, so CS runs TWICE
                                         #    — bounced AND through the realtime loopback CT already
                                         #    builds. "Bounce flat, realtime not" is a real third
                                         #    outcome that would otherwise ship overclaimed
                                         #  - P101 mechanics: diagnostics() is unreachable on the
                                         #    app's live instance (N9) AND from page.evaluate, so the
                                         #    new layout section DYNAMIC-IMPORTS js/meters.js and
                                         #    builds its OWN createMeters instance with a controlled
                                         #    nativeFn. No source change, no window handle. Deadline
                                         #    measured: METER_POLL_MS 33 x GUARD_DEADLINE_TICKS 5 =
                                         #    165 ms. NC1 (remove the release, section must FAIL) is
                                         #    what makes it a gate rather than a claim
                                         #  - P104: CT records EIGHT MONO FILES while CR/CS produce
                                         #    ONE 8-channel file. analyse_bounce.py --input takes one
                                         #    or many, removing an export step that could itself
                                         #    re-order channels — the exact class of thing the phase
                                         #    exists to measure
                                         # AND ONE TASK NOT NEEDED: N15's three "gitignored evidence"
                                         # files are untracked debris left in the SHARED checkout by
                                         # a sibling session. On feat/o-octagon the 3.3 evidence dir
                                         # holds .txt/.png only, all committed. The RULE stands
                                         # (P108, no *.log); the remediation it implies does not.
                                         # ── 4.2 RESEARCH (history) ─────────────────────────────
                                         # RESEARCH-4.2.md, N7-N15,
                                         # asks for P101-P108. Entry check green: all four contracts
                                         # match the LEDGER BLOCK BELOW (not a prior artifact's prose)
                                         # and BOTH FROZEN BUNDLES RE-MEASURED on disk — c0fdd8f2… /
                                         # 1e04f0a8… — because a sibling session is live on
                                         # improve/o-spectralshaper-tooltips in the shared tree.
                                         # Q1 DID NOT COLLAPSE: D11 STANDS. The gate is not WHETHER
                                         # Logic offers 7.1 but WHICH — JUCE names create7point1() as
                                         # Logic's "7.1 (3/4.1)" and create7point1SDDS() as "7.1
                                         # (SDDS)", and the shipped Lss/Rss/Lrs/Rrs labels are ABSENT
                                         # from SDDS and from 5.1.2 (probe E already says so). So Q1
                                         # reduces to a ONE-LINE PRE-FLIGHT: getStatus.mapInvalid ==
                                         # false, read BEFORE the first bounce, not off one.
                                         # TWO PREMISES FALSIFIED, BOTH CARRIED UNRUN FOR 4+ PHASES:
                                         #  - N8: GATE 13's Q5 ITEM IS VACUOUS. The completion gate
                                         #    is Component::isVisible() — the component's OWN flag,
                                         #    set by addAndMakeVisible and never cleared in
                                         #    Source/. Minimise/⌘H/occlusion/Spaces all leave it
                                         #    TRUE; JUCE's own hidden-page path uses isSHOWING and
                                         #    withKeepPageLoadedWhenBrowserIsHidden() makes it a
                                         #    no-op. "Hide 10 s, re-show, meters resume" CANNOT drop
                                         #    a completion — and what it DOES observe is WebKit
                                         #    timer throttling, which looks identical. Worse than
                                         #    vacuous: confusable
                                         #  - N10: THE VERIFY-PING CANNOT BE BOUNCED. prepareToPlay
                                         #    -> verifyPing.prepare() sets phase=idle, cmd=kCmdNone,
                                         #    activeFlag=false, and Logic prepares at the start of
                                         #    every offline bounce. COMPAT-02/2 needs a REALTIME
                                         #    capture; ffmpeg/sox are NOT installed
                                         # N9: diagnostics().dropped is UNREADABLE on the freeze —
                                         # developerExtrasEnabled is #if JUCE_DEBUG, AND `meters` is
                                         # a module-scope let never put on window. D18's "confirmed
                                         # present in shipped source" is true and INSUFFICIENT.
                                         # N11: airAmount MUST be 0 on every bounce test — the air
                                         # filter's fc is position-dependent, so two instances at
                                         # different positions give a FALSE per-band delta that
                                         # reads exactly like bass management and triggers D16's
                                         # re-freeze on nothing.
                                         # PRIOR: 4.2 DISCUSS COMPLETE 2026-08-13 — CONTEXT-4.2.md, D11-D21.
                                         # Entry check applied 4.1-Issue-2's rule (compare against
                                         # STATUS.md's LIVE contract_checksums block, never a prior
                                         # artifact's prose): all four exact. The FROZEN 4.1 BINARY
                                         # IS ON DISK AND MEASURED — VST3 c0fdd8f2… / AU 1e04f0a8…
                                         # both match the freeze record, so 4.2 runs against
                                         # fba35081 as instructed. Research owes Q1-Q6; Q1 GATES
                                         # THE PHASE. 4.1 VERIFIED 2026-08-13: all 18 gates re-run
                                         # from scratch, not read out of SUMMARY-4.1.md. 95/0, six
                                         # pluginval runs, auval, CI run 31708358940 with headSha =
                                         # the freeze commit, and all five negative controls
                                         # reproduced. See VERIFICATION-4.1.md
stage_4_2_rig: blackhole-64ch            # D11. NO PHYSICAL 8-OUT INTERFACE IS ATTACHED — the
                                         # devices present are BlackHole 2ch, BlackHole 64ch, MBP
                                         # Speakers (2), MBP Mic, Teams, Zoom; NO aggregate. This
                                         # AMENDS the no_hall_this_milestone premise below, which
                                         # assumed "an 8-channel interface at the desk" and leaned
                                         # on it twice: verify-ping's "8 PHYSICAL OUTPUTS" and the
                                         # audible clause's "any-monitoring" judgement. Neither
                                         # holds as written. Disposition, per D11/D12:
                                         #  - verify-ping closes against the COREAUDIO DEVICE
                                         #    BOUNDARY via PER-CHANNEL CAPTURE, which is STRONGER
                                         #    than 8 moving meters. Residual = one specific
                                         #    hardware driver, OWNER: NONE (ungeneralisable across
                                         #    interfaces even if one were present). The criterion
                                         #    KEEPS THE WORD "PHYSICAL" — it is not edited to fit
                                         #    what the rig can prove
                                         #  - the audible clause runs off an OFFLINE BOUNCE on
                                         #    headphones, valid because QUAL-03 proved block-size
                                         #    invariance and 4.1 proved bit-reproducibility. Two
                                         #    halves, NOT interchangeable: the soloed difference
                                         #    signal is a LOCATOR only (soloing removes masking);
                                         #    the full bounce in context is THE REQUIREMENT
                                         #  - bounce-order and LFE-gain need NO device to RUN — the
                                         #    bounce is offline. BlackHole 64ch is needed only to
                                         #    CONFIGURE a 7.1 output at all (D13), which is Q1
stage_4_2_bounce_order_is_a_pair: true   # D20. THE §6a BOUNCE-ORDER TEST PASSES VACUOUSLY IF RUN
                                         # ONLY ON THE SHIPPED DEFAULT. The plugin says so in its
                                         # own source — VenueModel.cpp:87-89: "because this default
                                         # is the identity … a channel-map test driven by it alone
                                         # is VACUOUS — a hardcoded 0..7 map would pass it". So the
                                         # test is CR-a (identity → proves Logic's canonical bounce
                                         # order, the §6 MEDIUM-confidence claim) PLUS CR-b
                                         # (NON-IDENTITY permutation → proves the LABEL MAP is what
                                         # determines bounce order). Source material must be EIGHT
                                         # DISTINCT TONES; eight copies of one tone makes CR-b
                                         # unreadable
stage_4_2_lfe_test_widened: true         # D15. Source/Data/VenueModel.cpp:84 ASSERTS AS FACT a
                                         # claim the locked research doc rates MEDIUM-LOW: "Logic
                                         # applies no automatic bass management or LFE low-pass".
                                         # §6 is explicit — "absence of evidence, not proof". The
                                         # §6a test as written (ONE −20 dBFS tone) catches a GAIN
                                         # OFFSET and is BLIND TO A LOW-PASS, closing half the
                                         # claim while reading as if it closed all of it. Widened
                                         # to a multi-tone/log sweep compared PER BAND
stage_4_2_failure_dispositions: |
  Both decided AT THE DISCUSS BOUNDARY, not in the session — this project puts dispositions here
  precisely so a curve does not get re-tuned to satisfy one tired judgement at the end of a run.

  LFE TEST FAILS (D16) → FIX, RE-FREEZE, RE-RUN 4.1's 18 GATES. A 10 dB hot speaker is a broken
  default, not an artifact. Four edit sites: (1) speaker-4 compensating default trim — FUNC-07's
  venue-scoped post-solve trim path already exists, no new mechanism; (2) VenueModel.cpp:84, which
  must then state the MEASUREMENT not the assumption; (3) research/logic-pro-multichannel-
  octaphonic-dbap.md §6's confidence row; (4) re-cut freeze + re-run 4.2 against it. The identity
  label map is NOT the lever — moving speaker 4 off the LFE slot breaks channel N = speaker N,
  which §6a calls the entire reason for the default.

  AUDIBLE CLAUSE TICKS (D17) → LOG IT, SHIP v1.0, open a v1.1 row. The lever (RESEARCH-2.3 H3,
  raising fc(d_hull = 0) toward Nyquist) re-tunes the WHOLE musical air curve and is therefore a
  DISCUSS-BOUNDARY CHANGE, NOT A FIX — the ROADMAP already says so at D2.
stage_4_2_pre_session_code_edits: |
  TWO code changes are owed BEFORE the host session, because each re-cuts the freeze and a freeze
  re-cut after the session invalidates the session.

  D19 — Gate 16b, carried from 4.1 verify Issue 1. The gate is spelled `presetManager.loadPreset (`
  and counts ZERO call sites: the parameter is named `manager`, and the only hit without the space
  is PresetPolicy.h:202, the doc-comment DESCRIBING the gate. Re-spell receiver-agnostically —
  grep -rnE '\.loadPreset[[:space:]]*\(' Source/ must return exactly one NON-COMMENT hit. Three
  sites: PLAN-4.1 Gate 16, the PresetPolicy.h:202 comment, and any future stage's gate list.

  D21 — THE 4.1 VERIFY ARTIFACTS ARE UNCOMMITTED. VERIFICATION-4.1.md is untracked;
  REQUIREMENTS.md and STATUS.md are modified. Commit before the host session
  (pattern_uncommitted_improve_versions_lost).
stage_4_cycle_structure: "4.1 machine · 4.2 host-and-ear"  # D1. The split is a HARD DEPENDENCY, not
                                           # a preference: 4.2 should run against a binary 4.1 has
                                           # frozen. A single pass plans the human session before
                                           # the machine gates have frozen what it would test
no_hall_this_milestone: true               # D2. Rig is LOGIC + AN 8-CHANNEL INTERFACE AT THE DESK.
                                           # AND THE COST IS SMALLER THAN THE ROADMAP GOAL LINE
                                           # IMPLIED — every Stage 4 criterion closes at the desk.
                                           # Bounce-order and LFE-gain are FILE-BASED (read the
                                           # interleaved bounce, don't listen to it); verify-ping
                                           # needs 8 PHYSICAL OUTPUTS, which the interface is. D5's
                                           # ONLY unique coverage is QUAL-01/2's audible clause, and
                                           # its open question — does ~15% of an 8 kHz component, as
                                           # a ONE-SAMPLE step on a SINGLE hull-crossing gesture,
                                           # tick on HF-rich material — is a one-gesture,
                                           # any-monitoring judgement that needs no room. What a
                                           # hall would add is SPATIAL-COHERENCE judgement, which
                                           # NO REQUIREMENT ROW ASKS FOR. Stated so it is a decision
                                           # and not an omission. If it ticks, the lever is
                                           # RESEARCH-2.3 H3 — a DISCUSS-BOUNDARY change opening
                                           # v1.0.1, not a fix inside 4.2
r2_prediction_was_live_in_three_contracts: true # D7. STAGE 3'S PARTING RULE FIRED ON ITS FIRST
                                           # APPLICATION. Logic negotiated plain create7point1() at
                                           # the 2.1 manual gate, contradicting the Stage-0 R2
                                           # prediction of 7.1-SDDS. THE RETIREMENT WAS WRITTEN INTO
                                           # REQUIREMENTS.md COMPAT-02 AND NOWHERE ELSE. Three
                                           # further phases ran with the prediction still stated as
                                           # LIVE in ARCHITECTURE §3.2.2, ARCHITECTURE §R2 (HIGH),
                                           # ROADMAP:401-402 and ROADMAP:472-474. ROADMAP:401 IS NOT
                                           # PROSE — IT IS A STAGE 4 ACCEPTANCE CRITERION, and as
                                           # written 4.2 would have set out to SETTLE a fact settled
                                           # nine days earlier. Carried forward soundly: the
                                           # extracted isBusesLayoutSupported() body is BYTE-
                                           # IDENTICAL between a47cef88 (the observation's commit)
                                           # and the working tree, 31 lines. Amended at this
                                           # boundary; the three-container MITIGATION STAYS SHIPPED,
                                           # because it is what makes one observation safe to rely
                                           # on. 4.2 CONFIRMS, and gains one genuinely unobserved
                                           # thing in place of the settled one: STABILITY ACROSS
                                           # SESSION RECALL, which 2.1 never looked at
ledger_corrected_to_28_of_30: true         # NOT 29. COMPAT-04 was recorded complete from its
                                           # summary row. Reading the stage-1 evidence: criteria 1
                                           # and 2 WERE genuinely measured (auval exercised all four
                                           # configs, Standalone ran on a 2-ch device) — the tick
                                           # was UNDER-DOCUMENTED, NOT UNEARNED. But criterion 3
                                           # covers safeMode, WHICH LANDED AT PHASE 3.1 (P43), TWO
                                           # STAGES AFTER THE ROW CLOSED. A COMPLETED ROW WHOSE
                                           # SUBJECT MATTER LATER GREW — a distinct shape from the
                                           # five vacuity defects Stage 3 catalogued: nothing went
                                           # vacuous and nothing regressed, the requirement simply
                                           # acquired new surface after sign-off and nothing in the
                                           # process looks back. O-Octagon has now shipped FOUR
                                           # behaviours on the stereo/degenerate path — safeMode,
                                           # the SAFE banner, the negotiated-set readout and the
                                           # mapInvalid fold — THREE OF THEM AFTER COMPAT-04 CLOSED
the_residual_nobody_owned: true            # THIS STAGE'S DEFINING HAZARD, and it is NOT Stage 3's.
                                           # Stage 2's hazard was the silent channel map; Stage 3's
                                           # was the vacuous assertion. Stage 4's is A RESIDUAL
                                           # CARRIED IN PROSE AND NEVER PROMOTED TO A CRITERION. The
                                           # CI gap is the specimen: stated at EVERY verify boundary
                                           # since 2.1 — five times, each noting it had WIDENED —
                                           # and present in NO ACCEPTANCE LIST ANYWHERE, including
                                           # ROADMAP §Stage 4, the document that decides what Stage
                                           # 4 IS. A residual restated five times and owned by no
                                           # criterion is functionally a residual that was DROPPED;
                                           # it survived only because the same person kept re-typing
                                           # it. FOUR items promoted to checkboxes at this boundary
                                           # (CI wiring, COMPAT-04's criteria, Gate 13 + Q5, D5's
                                           # audible clause). THE RULE 4.1'S PLAN MUST ENFORCE:
                                           # every carried-forward item is either a ROADMAP §Stage 4
                                           # checkbox or an EXPLICITLY RECORDED deferral with a
                                           # named owner phase. PROSE IS NOT A THIRD OPTION
stage_4_phase_4_1_researched: true         # RESEARCH-4.1.md — Q1-Q6 answered, N1-N7. All four
                                           # checksums byte-exact ON ARRIVAL at their NEW discuss
                                           # pins (2806c788 / dbb0dd57), which is the check those
                                           # amendments landed. NO contract amended here; THREE are
                                           # REQUIRED AT THE PLAN BOUNDARY (A1-A4), flagged not
                                           # taken, exactly as 3.3 research scheduled its ROADMAP
                                           # amendment.
                                           #
                                           # Q2 MEASURED, and it KILLS its own contingency: clean
                                           # CI-shaped configure 7.8s, clean build of both test
                                           # targets 27.6s wall / 167s CPU (arm64, 16 cores, ZERO
                                           # warnings), and all 92 probes RUN IN 0.39s. The render
                                           # harness was the suspected cost and it is 0.37s. No
                                           # separate job, no larger runner; ~3-4 min on a 3-core
                                           # macos-14. VERIFIED and it halves the number: the
                                           # tests-only build does NOT produce the VST3/AU/Standalone
                                           # bundles — those artefact dirs come out EMPTY, only
                                           # libO-Octagon-dev_SharedCode.a plus two console apps.
                                           #
                                           # N3 — THE STAGE'S OWN HAZARD FIRED AGAIN, ONE DAY OLD.
                                           # ROADMAP §4.1 bullet 1 (written yesterday, D6) says the
                                           # probes run in build-and-release.yml. CMakeLists:172-176
                                           # (PLAN-2.1 P13) says DO NOT ADD THEM THERE. It is the
                                           # repo's ONLY workflow. Two live contracts contradict;
                                           # the plan must RESOLVE it, not inherit it. Recommended:
                                           # a NEW SECRETLESS ci-tests.yml on push/PR — honours P13
                                           # literally AND delivers D6's actual intent, because the
                                           # stated failure mode is "a JUCE bump ships silently" and
                                           # a JUCE bump is a COMMIT, not a tag. And the JUCE pin
                                           # must be DERIVED, not mirrored: a second literal drifts
                                           # SILENTLY IN THE ONE DIRECTION THAT MATTERS (bump one
                                           # file, keep proving the old JUCE green).
                                           #
                                           # N2 — the real gate is the COMPILE. Probe C's Layer-2
                                           # comparison is a static_assert (unit/main.cpp:123-124)
                                           # over a header GENERATED FROM PARSED JUCE SOURCE, so a
                                           # JUCE bump that reorders AudioChannelSet FAILS TO
                                           # COMPILE. The CI step must BUILD, not run a cached
                                           # binary. vendored/JUCE-overrides touches neither
                                           # AudioChannelSet file, so CI's golden should agree.
                                           #
                                           # N4 — Q3 ANSWERED: NEITHER MSVC pattern fires. Lexical
                                           # scan of every lambda body in Source/**: ZERO non-static
                                           # constexpr. SafePointer appears NOWHERE in Source/ —
                                           # zero call sites, not "authored against". Portability
                                           # scan clean too. THE PLAN MUST STOP CALLING THE WINDOWS
                                           # RISK "the two known patterns" — it is that no MSVC has
                                           # parsed the code. Q4: WebView2 fully authored
                                           # (NEEDS_WEBVIEW2, static-linking define,
                                           # withUserDataFolder, bare-path resource provider) AND IT
                                           # IS THE HOUSE PATTERN — all 40 plugins set both. The
                                           # Windows dispatch path already exists (O-Contrabass's
                                           # validate_only), BUT build-macos has NO validate_only
                                           # guard, so dispatching SIGNS AND NOTARISES — which D4
                                           # excludes. Put the Windows job in ci-tests.yml instead.
                                           #
                                           # N5 — THE FINDING THAT CHANGES D5. applyPresetJson
                                           # RESETS ALL 17 PARAMETERS to default before applying
                                           # (WR-01, module v1.0.3, :315-331). So a six-key
                                           # "room-character only" preset does NOT leave position
                                           # and weights alone — it resets srcX/srcY to 0.5, srcZ to
                                           # 0, and w1..w8 to 1.0. LOADING ANY FACTORY PRESET UNDOES
                                           # WHATEVER FUNC-06 SCENE IS APPLIED. That is the exact
                                           # collision D5 was written to prevent, arriving through
                                           # the MODULE'S DEFENSIVE BEHAVIOUR rather than the
                                           # preset's content. The mechanism was known in source
                                           # (PluginEditor.cpp:697-707 documents the 17 reset
                                           # writes) but it reasons about a DEFAULT patch, where
                                           # they are no-ops; nothing looked back at D5. Fix is
                                           # snapshot-and-restore AT O-OCTAGON'S CALL SITE, inside
                                           # the bracket already open on all 17 — which is what
                                           # PluginEditor.cpp:711 already rules. The probe MUST be a
                                           # negative control: asserting only "the six moved" PASSES
                                           # WITH THE BUG PRESENT.
                                           #
                                           # N6 — COMPAT-04 CRITERION 3 ASSERTS THE FAILING
                                           # BEHAVIOUR, in TWO contracts. It asks for a fourth
                                           # 8-channel set confirming the banner "stays down". Under
                                           # the shipped COMPLEMENT form the banner goes UP; "stays
                                           # down" is what the == mono || == stereo spelling D8
                                           # REJECTS would do. Verified as written it passes ONLY IF
                                           # THE CODE HAS THE WRONG SPELLING. Same shape as D7, one
                                           # boundary later, in a criterion authored specifically to
                                           # be un-vacuous. AND THE MIRROR OF THE STAGE'S HAZARD:
                                           # two thirds of criterion 3 is ALREADY MET by probe BM
                                           # (render-harness:3807-3850, Phase 3.1, all five layouts)
                                           # — EVIDENCE NOBODY CLAIMED, invisible because the row
                                           # had no criteria section until yesterday. Remaining work
                                           # is ONE probe (CO), via a header-only oo::rig::isRealRig
                                           # extraction so the complement property is asserted
                                           # DIRECTLY in the fast target. CO proves the FORM, BM
                                           # proves the WIRING; neither alone does.
                                           #
                                           # N7 — the .factory-version sentinel is an AUTHORING AND
                                           # A VERIFICATION trap at a pinned 1.0.0: the second and
                                           # every later edit to the definitions WRITES NOTHING, and
                                           # a probe reading the on-disk JSON can be reading an
                                           # earlier iteration AND PASS.
                                           #
                                           # Q5 premise was STALE: the preset-manager is ALREADY
                                           # integrated (include path, editor member, four native
                                           # fns, setCustomStateCallbacks). 4.1 authors DEFINITIONS,
                                           # not plumbing. Manager is EDITOR-owned while all ~20
                                           # precedents call initializeFactoryPresets from the
                                           # PROCESSOR ctor — no precedent to copy, and the
                                           # departure is STRICTLY BETTER for auval.
                                           #
                                           # Q6: values derived from SHIPPED CONSTANTS, not taste —
                                           # rigScale of the §OQ4 default venue is 7.9317 m. Five
                                           # presets + an optional sixth, and TWO OF THEM SIT ON A
                                           # DEFEAT PATH DELIBERATELY (airAmount=0 bit-transparent,
                                           # hullAtten=0 bit-exact unity), so the factory set
                                           # doubles as a reachability check on both exact-no-op
                                           # branches.
                                           #
                                           # Gate state on arrival, ALL FOUR GREEN: 44 + 48 C++
                                           # probes and 42 + 27 JS sections, 0 failures. Baseline
                                           # 4.1 freezes for 4.2. Next probe CO, first plan decision
                                           # P86.
                                           #
                                           # NOT RUN AND NOT CLAIMED: no Windows compiler was
                                           # invoked. N4 is a STATIC SCAN, and a static scan is not
                                           # a compile — which is the whole reason D3 exists.
stage_4_phase_4_1_discussed: true          # CONTEXT-4.1.md — D1-D10. TWO CONTRACTS AMENDED AND
                                           # RE-PINNED (ROADMAP aec7d0ce -> dbb0dd57, ARCHITECTURE
                                           # a8a358f4 -> 2806c788); BRIEF and parameter-spec UNMOVED
                                           # — parameter-spec now unmoved across SEVEN consecutive
                                           # phases, 17 parameters, and Stage 4 adds none. All four
                                           # byte-exact ON ARRIVAL before any amendment
stage_3_verified: true                     # STAGE 3 CLOSES. VERIFICATION.md — NINE rows, 40 criteria,
                                           # ZERO partials, across 3.1 / 3.2 / 3.3. 92 C++ probes
                                           # (dedup 2026-08-13 at the 4.2 discuss boundary: this key
                                           # appeared TWICE, an orphaned two-line paste artifact
                                           # duplicating the head of this block. Both read `true`,
                                           # so nothing was lost — but it is the same silent-shadow
                                           # hazard as roadmap_checksum_superseded below, found by
                                           # the same duplicate-key scan and worth keeping in the
                                           # boundary checklist)
                                           # (44 unit + 48 harness) and 69 JS sections (42 + 27), 0
                                           # failures, re-run from a FORCED FULL RECOMPILE at each of
                                           # three verify boundaries. 8 negative controls run as NEW
                                           # work across the three verifies, all 8 fired, tree proved
                                           # byte-identical each time. 29 of the project's 30
                                           # requirement rows are now complete; COMPAT-04 is the only
                                           # row still without a derived criteria section
stage_3_phase_3_3_verified: true           # VERIFICATION-3.3.md — FUNC-06 / UI-03 / UI-04 / UI-05 ALL
                                           # COMPLETE, 18 criteria, zero partials. All 16 gates re-run
                                           # from scratch on a forced full recompile (143 steps, 0
                                           # diagnostics) — and the WHOLE SET re-run a SECOND time
                                           # after the one fix this verify landed. All four checksums
                                           # byte-exact incl. ROADMAP at its NEW pin; referent checked,
                                           # not just the match
ao_residual_resolved_it_was_the_probe: true # THE FINDING OF THIS VERIFY, and the summary was right to
                                           # refuse to round it down. Execute carried AO's "1 alloc
                                           # once in 35 runs" as unreproduced and unattributed. Verify
                                           # reproduced it 4 TIMES IN 40 RUNS UNDER 8-WAY LOAD and
                                           # attributed ALL FOUR to a thread other than the one calling
                                           # processBlock. rtcheck::armed was a PROCESS-WIDE flag, so
                                           # the replaced operator new counted ANY thread's allocation
                                           # — message thread, macOS runtime threads. Contention does
                                           # not make processBlock allocate; IT WIDENS THE WINDOW IN
                                           # WHICH ANOTHER THREAD'S ALLOCATION LANDS INSIDE IT. Counter
                                           # now scoped to the arming thread, all arm sites through one
                                           # rtcheck::arm(), foreign tally STILL TAKEN AND STILL
                                           # REPORTED (a filter that hid it would trade a flaky probe
                                           # for a silent one). Before 4/40 failed, after 0/40.
                                           # PERF-01 NEVER REGRESSED — processBlock read 0 at every
                                           # load level. THE ONLY tree change made at verify, confined
                                           # to tests/render-harness/main.cpp
gate_4_corroborated_against_the_filesystem: true # D27's one line paid off at the FIRST boundary that
                                           # could test it. §0's machine stamp 2026-08-12T22:47:48.514Z
                                           # (= 15:47:48 PDT) was checked against the MTIMES of the C++
                                           # the claim is about: DbapSolver.h 15:48:28 (+40 s),
                                           # SceneModel.h 15:50:14, FieldSampler.h 15:51:49,
                                           # FieldSampler.cpp 15:52:54, SceneModel.cpp 16:36:06. EVERY
                                           # 3.3 C++ FILE IS NEWER. An INDEPENDENT artifact agrees with
                                           # the gate; the three-boundary transcription chain is closed
nc_b_is_d25_reproduced_in_its_strongest_form: true # Verify built the control with a ZERO-HEIGHT node so
                                           # nothing could overflow, and got the exact predicted split:
                                           # §21 [coarse] PASSED 592<=592 at BOTH DPRs and §21 [guard]
                                           # PASSED 123<=123, while §22 FIRED on the ordering. Every
                                           # measurement of SIZE reported green while the ORDERING FACT
                                           # they depend on was broken. N11's correction and D25's
                                           # surviving conclusion both confirmed by measurement
verify_3_3_issues_are_documents_only: true # Besides the probe fix: (1) SUMMARY quotes 141 build steps,
                                           # verify measured 143 twice — two steps, no diagnostic, but
                                           # a future phase comparing to 141 would see a drift that is
                                           # not one; (2) layout §27's counter BASELINE is run-dependent
                                           # (verify read 11->11 / control 11->12 vs the summary's
                                           # 12->12 / 12->13) — the assertion is the DELTA, 0 and 1, and
                                           # both held exactly; (3) GATE 11'S THREE-MODULE PROSE HAS NOW
                                           # SURVIVED THREE BOUNDARIES. juce_events is transitive via
                                           # juce_data_structures. The asserted invariant is the ABSENCE
                                           # of juce_dsp / juce_gui_extra / juce_audio_processors, never
                                           # the presence of exactly three
gate_13_static_half_further_discharged_3_3: true # ALL FOUR 3.3 COMPONENTS CONFIRMED IN WKWEBVIEW at
                                           # verify, window 1102x778: SCENES with U1-U4 STRUCK THROUGH
                                           # AND DISABLED (D20's refusal, rendered) and STORE in the
                                           # title row (Q9's V3); the ELEVATION strip with the rake
                                           # solid and BOTH extrapolations dashed, the 0-7 quantised
                                           # axis, pairs reading 1 2 / 3 8 / 4 7 / 5 6, and both
                                           # readouts EQUAL at 2.15 m because srcZ=0 rides the rake;
                                           # the FIELD gradient with hull and glyphs still legible and
                                           # the legend "0.0 - 2.7 dB"; the eight meter arcs at rest.
                                           # THE INTERACTIVE HALF DID NOT RUN — synthetic clicks return
                                           # -25208 in this environment, exactly as at 3.2
q5_now_unrun_by_four_phases: true          # STATED, not implied. A 30 Hz poll against a HIDDEN
                                           # WKWebView. Attempted at verify and ABANDONED for a stated
                                           # reason: with the Standalone's input muted the meters read
                                           # zero either way, so "resume" has no observable at rest, and
                                           # meters.js's `dropped` counter needs devtools INTO the
                                           # WKWebView. It needs a human with a signal running
stage_3_phase_3_3_executed: true         # SUMMARY-3.3.md — 25 tasks, ALL FOUR ROWS CLOSED, 18
                                           # criteria, ZERO partials. 92 probes / 0 failures
                                           # (44 unit + 48 harness). 69 JS sections (42 + 27), both
                                           # green, layout did NOT skip. Native surface 13 -> 18.
                                           # EIGHT negative controls fired, tree byte-identical.
                                           # 15 of 16 gates PASSED; GATE 13 (human Standalone,
                                           # incl. Q5's hidden-WKWebView test) HAS NOT RUN
gate_4_is_evidence_for_the_first_time: true # THE ORDERING CLAIM IS NOW SELF-EVIDENCING. Tasks 6-15
                                           # (JS) were run BEFORE Tasks 1-5 (C++), inverting the
                                           # plan's own numbering, because the plan numbered the
                                           # C++ first while Gate 4 required the stub render to
                                           # precede ANY 3.3 C++ — both could not hold and the gate
                                           # won. Stamp 2026-08-12T22:47:48.514Z, MACHINE-EMITTED by
                                           # ui_layout_check §0 (P84), recorded with FieldSampler
                                           # and SceneModel absent from disk and outInvK absent from
                                           # DbapSolver.h. Verified afterwards: the pre- and
                                           # post-integration runs agree on ALL 148 assertions
ao_residual_unattributed: true             # THE ONE THING NOT TO ROUND DOWN. Probe AO reported
                                           # 1 allocation ONCE IN 35 RUNS, on the first execution
                                           # after a full clean rebuild. 34 others read 0 — 20
                                           # quiet, 12 under 8-way CPU load, 2 after a SECOND clean
                                           # rebuild reproducing the original condition. DID NOT
                                           # REPRODUCE. Attribution to 3.3 is neither established
                                           # nor ruled out: getMagnitude resolves to
                                           # FloatVectorOperations on a raw pointer and allocates
                                           # nothing, and probe CN — the same measurement with
                                           # metering explicitly live plus a non-vacuity clause —
                                           # read 0 in that same run. Verify should re-derive it
cb_failed_on_its_own_non_vacuity_clause: true # NOT on its arithmetic, which agreed to 4e-8. Zero of
                                           # 20 STRIDED cells landed outside the §OQ4 hexagonal
                                           # hull, so hullTrimGain multiplied by bit-exact unity at
                                           # every sampled point and half the chain was untested.
                                           # Now 14 strided + 6 drawn from the 66 outside-hull
                                           # cells. THE CLAUSE WORKED AS DESIGNED
frontend_18_had_a_latent_unanchored_regex: true # FOUND BY 3.3's CSS, FIXED IN 3.3. §18's per-class
                                           # scan was unanchored, so `.elev-readouts .cell-value`
                                           # was harvested INSTEAD OF `.cell-value`'s own rule and
                                           # the gate reported the mono stack and tabular-nums
                                           # missing from a stylesheet that has both. The selector
                                           # is now anchored at a rule boundary. This would have
                                           # bitten any future phase adding a scoped override
p78_deviation_apply_moved_to_processor: true # applySceneWeights lives on the PROCESSOR, not at the
                                           # editor's call site. P78's argument is C++-vs-JS and is
                                           # fully honoured by one C++ function with one call site;
                                           # what the move buys is that PLAN-3.3's OWN PROBE TABLE
                                           # makes CI a HARNESS probe, and the harness never
                                           # compiles PluginEditor.cpp. On the editor the eight
                                           # gesture brackets could only ever have been GREPPED;
                                           # they are now MEASURED through real parameter listeners
two_visual_defects_found_by_looking: true  # NEITHER WAS CAUGHT BY A GATE. The field gradient at
                                           # alpha 30..180 washed out the hull polygon and the rig
                                           # extent (now 0..96); the elevation strip's four mirrored
                                           # speaker pairs stacked their numerals illegibly — the
                                           # DOTS stay exact, because moving one would make the
                                           # strip lie about a depth, and only the LABEL steps aside
stage_3_phase_3_3_planned: true            # PLAN-3.3.md — 25 tasks, P69-P85, probes CA-CN (78 -> 92),
                                           # JS gates 49 -> 69 sections (frontend 31 -> 42, layout
                                           # 18 -> 27), EIGHT negative controls declared AT PLAN,
                                           # 16 gates. All four checksums byte-exact ON ARRIVAL;
                                           # ROADMAP.md then AMENDED AND RE-PINNED here (P70) — see
                                           # roadmap_checksum_superseded. The other three unmoved.
                                           # THE TWO PROBES THAT CARRY THE PHASE: CG (permutation —
                                           # the only probe a fixed-index scene impl fails) and CM
                                           # (post-map meters on a NON-IDENTITY map — the only probe
                                           # a v_i meter fails). Every other scene/meter probe passes
                                           # under BOTH defects, which is what makes these two the
                                           # non-vacuity guards rather than additional coverage
p70_amendment_found_the_same_error_twice: true # THE RULE THIS BOUNDARY LEAVES BEHIND: when an
                                           # amendment corrects a claim, GREP THE OTHER CONTRACTS FOR
                                           # THE SAME CLAIM before closing. §4.3's "on a Timer" was
                                           # corrected at 3.3 discuss; nobody checked whether the
                                           # SECOND document said it too. ROADMAP's meter bullet did,
                                           # AND SO DID THE ACCEPTANCE CRITERION DERIVED FROM IT
                                           # (REQUIREMENTS UI-03/4). Honouring it literally would
                                           # have undone 3.1's Timer-free editor — which is what
                                           # lets tests/ui-stub render the whole UI and makes the
                                           # pre-integration half of every layout gate possible.
                                           # REQUIREMENTS.md is NOT a pinned contract and is NOT
                                           # re-pinned; the fix is a dated in-place note and NO
                                           # criterion's testable content changed — only a mechanism
                                           # phrase, and it moved TOWARD the pinned contract
n9_repair_is_3_2_debt_discharged_at_3_3: true # P71. refreshGeometry is REPAIRED at 3.3. Not caused by
                                           # 3.3, repaired by 3.3, because 3.3 is what makes the same
                                           # shape load-bearing at 30 Hz. EVERY in-flight guard on
                                           # this page releases on a DEADLINE, never on settlement.
                                           # DO NOT add a guard to pollStatus — it has none, which is
                                           # precisely why it is the one poll already safe
d25_conclusion_survives_justification_changed: true # P75. N11 corrected D25's PREMISE. The guard is
                                           # still required, but because the column assertion is
                                           # non-vacuous ONLY WHILE #group-elevation REMAINS THE LAST
                                           # CHILD — insert anything after it at any future phase and
                                           # it silently goes vacuous. So layout §21 asserts the
                                           # fitted box AND §22 asserts the ORDERING FACT it depends
                                           # on. The NC asymmetry is [§8 passes] while [§21 fires],
                                           # one level up from where 3.2 found it. NC8 is the control
stage_3_phase_3_3_researched: true         # RESEARCH-3.3.md — all 11 questions answered. All four
                                           # checksums byte-exact on arrival INCLUDING the new
                                           # architecture pin 32a85018 (which is the check that the
                                           # 3.3-discuss amendment actually landed). NO pin moved
                                           # here. N-series continues N9-N13. Evidence tool:
                                           # tests/tools/room_layout_study.js
n9_dropped_completion_latches_the_guard: true # THE finding, and it is a LIVE DEFECT IN SHIPPED 3.2
                                           # CODE, not a 3.3 design hazard. refreshGeometry clears
                                           # geometryFetchInFlight in a `finally` (app.js:379-399)
                                           # and a promise that NEVER SETTLES never runs its finally
                                           # — so neither catch nor finally fires and the flag stays
                                           # true for the life of the page. MEASURED on the shipping
                                           # page: after ONE dropped getVenueGeometry the envelope
                                           # readout stays 15.60x19.50 m while the venue is
                                           # 39.00x52.00 m, and does NOT recover through five poll
                                           # ticks with the transport restored. Control run (no
                                           # drop) updates within one tick. D20's "fixed interval +
                                           # in-flight guard" is NECESSARY BUT NOT SUFFICIENT: every
                                           # guard on this page must release on a DEADLINE, never on
                                           # settlement. The 2 Hz getStatus poll is safe only
                                           # because it has NO guard at all — do not "fix" it
n10_max_v_squared_is_degenerate: true      # ROADMAP's UI-04 formula is DISQUALIFIED, measured against
                                           # the shipping solver over the default envelope:
                                           # max_i v_i^2 is IDENTICALLY 1.0000 EVERYWHERE when one
                                           # weight is non-zero (DBAP normalises to sum v^2 = 1, so
                                           # it measures only CONCENTRATION), and 3.2-5.4 dB
                                           # otherwise. The picture goes blank exactly when the
                                           # spatial situation is most extreme. The UN-NORMALISED
                                           # field 1/k = sqrt(denom) gives 1.3-10.4 dB with correct
                                           # radial structure and never degenerates — and
                                           # dbap::solve ALREADY COMPUTES IT as `denom`. Reach it
                                           # with a DEFAULTED out-param (float* outInvK = nullptr),
                                           # the P54 MapDiagnosis precedent: no new pow, powCalls==16
                                           # untouched, every call site compiles unchanged.
                                           # REQUIREMENTS' four UI-04 criteria never name a formula;
                                           # only ROADMAP's component bullet does — so this is a
                                           # ROADMAP AMENDMENT + RE-PIN AT THE PLAN BOUNDARY
field_is_flat_over_a_real_audience_plane: true # BOTH formulas share a cause: every grid point is at
                                           # z=0 while speakers are 4.50-5.40 m up, so the minimum
                                           # 3-D distance is >= 4.5 m in a 12x15 m hall. The DBAP
                                           # field over a raked audience plane GENUINELY IS FLAT.
                                           # Any absolute 0..1 colour map renders a uniform wash —
                                           # normalise to the per-recompute observed min/max and
                                           # print the dB span in a legend, or the picture carries
                                           # no information WHILE LOOKING AS THOUGH IT DOES
n11_d25_premise_is_over_attributed: true   # CONTEXT-3.3 read controls.scrollHeight===clientHeight
                                           # ===592 on an UNMODIFIED tree as proof the vacuous shape
                                           # is present. An unmodified tree has NOTHING TO OVERFLOW,
                                           # so both read equal either way. MEASURED with a real
                                           # 120 px overflow the column-level coarse assertion FIRES
                                           # (699<=592) in ALL THREE candidate stage constructions
                                           # (plain flex:1 / .plan-stage clone / absolute z-stack).
                                           # The genuinely vacuous assertion here is the
                                           # DOCUMENT-LEVEL §8, which passed at 720<=720 in every
                                           # run at both DPRs. RULE: a flex container's scrollHeight
                                           # grows only for overflow past its LAST child — .miniplan
                                           # is child 2 of 5 in the rail (measured miniIsLastChild
                                           # false), #group-elevation IS the column's last child.
                                           # D25's CONCLUSION survives; the NC asymmetry must be
                                           # built against §8, and the gate must assert the ORDERING
                                           # FACT it depends on or a later insert re-vacuums it
nc3_reproduced_as_a_method_control: true   # Part D of room_layout_study.js replays 3.2's NC3 on the
                                           # venue rail through the SAME code path BEFORE the N11
                                           # conclusion is drawn, and reproduces VERIFICATION-3.2
                                           # exactly: rail coarse PASSES 592<=592 while the guard
                                           # FIRES 375<=213. Without this control the N11 correction
                                           # would be indistinguishable from a measurement artifact
n12_the_field_has_five_inputs: true        # UI-04/2 names geometry and weights. updateControl shows
                                           # rolloff (-> a), blur (-> r_s) AND hullAtten (-> hull
                                           # trim) also move the field, and all three are
                                           # AUTOMATABLE AT AUDIO RATE — so a literal "recompute on
                                           # change" makes a blur ramp recompute every block.
                                           # Coalesce to one recompute per poll tick. The criterion's
                                           # TESTABLE claim is the puck one and it is exactly right:
                                           # srcX/srcY/srcZ/width are genuinely NOT inputs
n13_scenes_rides_copystate_but_needs_normalisation: true # SCENES as a SIBLING of VENUE is persisted
                                           # and restored by apvts.copyState() with NO new code, so
                                           # FUNC-06/4's session round-trip is STRUCTURAL. What is
                                           # NOT free: the writeToState NORMALISATION that VENUE
                                           # already gets in setStateInformation — without it every
                                           # session written before 3.3 restores with no SCENES node
                                           # and the four slots read as ABSENT rather than EMPTY.
                                           # Preset path verified in module source: applyPresetJson
                                           # calls customLoad ONLY when "customState" exists
                                           # (:346-349), so a scene-less preset leaves slots
                                           # untouched. TRAP: setStateFromXml (:592-604) calls
                                           # customLoad on a DIFFERENT condition and does
                                           # replaceState() — O-Octagon does not use that path and
                                           # must not start; worth a one-line gate
room_3_3_layout_measured: "552x125"        # Q8/Q9 MEASURED on the rendered page at 1100x720 against
                                           # the ui-stub. Baseline reproduced: column 582x592, four
                                           # groups end at y=386, slack 278. Q8's PREMISE WAS WRONG:
                                           # the strip's real box is 552x125, not 582x~160 — 582 is
                                           # the COLUMN and the group padding takes 30 px. Variants:
                                           # V1 two rows -> strip 552x92; V2 one row of 11 -> strip
                                           # 552x130 but buttons are 42.9 px with 32.9 px of content
                                           # against 33.1 px of ink, so FRONT/RIGHT/SIDES/STORE CLIP
                                           # BY 0.2 px; V3 one row of 10 with STORE IN THE TITLE ROW
                                           # -> scenes 82, strip 552x125, 48 px buttons, 4.9 px to
                                           # spare. V3 RECOMMENDED. STORE in the title row costs 5 px
                                           # not 30 because the title row already exists
elevation_needs_no_exaggeration: true      # Depth 28.31 px/m (envelope y-span 19.50 m over 552 px)
                                           # vs height 19.23 px/m (0..6.5 m over 125 px), ratio 0.68
                                           # — compressed, NOT faked; two scales is ordinary for a
                                           # section drawing and needs LABELLING, not an exaggeration
                                           # factor. rakeRear sensitivity 19.23 px/m, so a 0.5 m edit
                                           # moves the rear 9.6 px. The OQ4 grading 4.50->5.40 spans
                                           # 17.3 px. THREE CONSTRUCTION RULES: (1) draw the rake
                                           # line ONLY bbMinY->bbMaxY — earHeight EXTRAPOLATES, so a
                                           # line across the whole envelope moves BOTH ends when
                                           # rakeRear moves and breaks UI-05/1's negative half;
                                           # (2) derive the height axis from the venue and QUANTISE
                                           # it, and let the criterion's own "front endpoint
                                           # unchanged" half be the guard against a rescaling axis;
                                           # (3) srcZ spans -2.0..8.0 m so absolute height reaches
                                           # ~11.5 m — the MARKER clamps with a chevron, the NUMBERS
                                           # never do
field_grid_cost_measured: "32x40 = 183us"  # Q2 benchmarked against the shipping DbapSolver.cpp.
                                           # pow is NOT the constraint — even 112x140 (125,440 pow)
                                           # is 660 us on the message thread. THE PAYLOAD IS: 61 kB
                                           # of JSON per recompute through a bridge that serialises
                                           # every value. 32x40 quantised to 8 bits and base64'd is
                                           # 1.7 kB, decoded with atob -> putImageData -> drawImage,
                                           # which satisfies UI-04/3's "offscreen canvas and blitted"
                                           # DIRECTLY. 8-bit quantisation does NOT weaken UI-04/1:
                                           # the 1e-3 comparison is a C++ UNIT PROBE on the sampler's
                                           # FLOAT output, strictly upstream of transport
dbap_solve_is_message_thread_safe: true    # Q1. Free function, no instance, no state, noexcept, no
                                           # allocation, no JUCE. NO second instance needed. TWO
                                           # qualifications: (1) under OOCTAGON_INSTRUMENT a field
                                           # sample pollutes powCalls, and probe AE asserts ==16
                                           # EXACTLY — call instr::resetCounters() between them, as
                                           # the harness already does at 11 sites; solveRuns is NOT
                                           # at risk (countSolveRun is in updateControl:287, not in
                                           # solve). (2) the recompute counter must be C++-side.
                                           # FieldSampler.{h,cpp} depends only on DbapSolver.h/Vec.h/
                                           # VenueSnapshot.h so it lands in the FAST unit target —
                                           # the same move P56 made for VenueFile.cpp
meters_site_is_processblock_after_gainstage: true # Q4. LAST statement in processBlock, so it is
                                           # genuinely what leaves the plugin AND it is AFTER the
                                           # ping's post-write overwrite — which is what makes
                                           # UI-03/2's ping cross-check possible at all. GainStage
                                           # untouched, so P24 survives. Index through
                                           # SNAPSHOT.speakerToBuffer, never the processor member.
                                           # Identity attribution when unmapped is CORRECT (that is
                                           # N8's fold being visible). getMagnitude resolves to
                                           # FloatVectorOperations on a raw pointer — no allocation.
                                           # The load/compare/store vs exchange(0) race can only
                                           # RE-PUBLISH a reported peak, never lose one. TWO polls,
                                           # not one: getStatus builds a juce::String from
                                           # getCurrentLayout().getDescription() on EVERY call, so
                                           # folding meters in would cost 30 String constructions/s
scene_write_path_is_cpp: true              # Q6. applyScene in C++, one native fn, eight brackets —
                                           # the loadPreset shape at PluginEditor.cpp:635-659
                                           # already does this for 17 params INCLUDING closed-on-
                                           # both-paths. A JS-side write through eight SliderStates
                                           # scatters D18 across 24 messages where no single grep
                                           # confirms it. The parameter echo still repaints the page
                                           # (WebSliderParameterAttachment listens to the param).
                                           # D20's refusal lives in C++ TOO, not only in the disabled
                                           # control — the startPing/mapInvalid defence-in-depth
func_06_6_fade_probe_shape: true           # Q6. Apply A -> read 8 host values (a); apply B -> read 8
                                           # (b); write w_i = 0.5(a_i+b_i) DIRECTLY, bypassing the
                                           # scene path; render and assert the 8 gains equal a direct
                                           # solve for the blended vector AND that no subsequent
                                           # block re-asserts a or b. An implementation storing
                                           # "current scene" and re-applying it in updateControl
                                           # FAILS; one that only writes parameters passes by
                                           # construction. Runs entirely in the render harness
scene_sets_default_venue: "SIDES {3,4,7,8}" # D16's predicate evaluated exactly, centroid
                                           # (6.5000, 12.4625), hx 6.000 hy 7.500. ALL {1..8} /
                                           # FRONT {1,2,3,8} / REAR {4,5,6,7} / LEFT {1,6,7,8} /
                                           # RIGHT {2,3,4,5} / SIDES {3,4,7,8}. Speakers 1 and 2 miss
                                           # by 6.2 % (1.0617 vs 1.0000) — CONTEXT's figure
                                           # CONFIRMED. 3 and 8 are ON_EDGE, which is why the
                                           # predicate must read != INTERIOR and NOT == VERTEX
q7_fixtures_designed: true                 # PERMUTATION: same 8 positions, indices rotated by k —
                                           # FRONT must return the indices that NOW hold y < cy; a
                                           # fixed-index impl returns {1,2,3,8} and FAILS.
                                           # EMPTY-SET: a PROSCENIUM rig — 4 corners + 2 points on
                                           # each of the front and rear edges, no side fills.
                                           # Corners give 1.000 vs 1.000 and edge points 0.333 vs
                                           # 1.000, so SIDES = {} while every speaker is
                                           # non-INTERIOR and the venue is non-degenerate. A
                                           # PHYSICALLY PLAUSIBLE rig, not a contrived one, which is
                                           # what makes it a fair test of D20
q10_membership_rides_getvenuegeometry: true # Named-scene membership is a PURE FUNCTION OF THE VENUE,
                                           # so it belongs in the payload that already refreshes on
                                           # venueGen — removes a whole staleness class and saves a
                                           # native fn. User slots are NOT a venue function and need
                                           # their own read + a scenesGen on getStatus, mirroring
                                           # venueGen. UI-05 needs NO new fn (3.2's P55 confirmed)
native_fn_surface_3_3_proposed: 18         # 13 + getMeters + getScenes + applyScene + storeScene +
                                           # getFieldGrid. ui_frontend_check.js:211's literal
                                           # `registered.size === 13` must move to 18 and WILL FAIL
                                           # LOUDLY until all eighteen exist in the C++
                                           # registrations, the derived call sites AND the stub
                                           # whitelist — exactly as the 3 -> 13 move did at 3.2
enumeration_hole_already_closed_at_3_3: true # P51 derives PAGE_MODULES from Source/ui/public/js/*.js
                                           # (ui_frontend_check.js:122) and §21 asserts set-equality
                                           # against the CMake SOURCES, so 3.3's new modules land
                                           # AUTOMATICALLY. Seventh time this class would have bitten
                                           # and the first time it costs nothing. ONE GAP REMAINS:
                                           # UI-04/2's puck-drag assertion needs the Playwright side
                                           # to COUNT getFieldGrid invocations, and the stub records
                                           # WRITES and GESTURES but has no call counter
q5_wkwebview_half_not_run: true            # STATED AS NOT RUN rather than implied. The JS half of N4
                                           # is now MEASURED (N9) and the JUCE drop is READ FROM
                                           # SOURCE (3.2 N4, re-verified). A real 30 Hz poll against
                                           # a HIDDEN WKWebView has been executed by NO ONE — it
                                           # needs a running plugin with the meter poll in it, which
                                           # does not exist until execute. Becomes a named Gate 13
                                           # item, not a summary line that reads as though it ran
stage_3_phase_3_3_discussed: true          # CONTEXT-3.3.md — FUNC-06 + UI-03/04/05, four rows /
                                           # 18 criteria, ZERO partials declared. D15-D28. The three
                                           # scheduled §8 re-pins TAKEN plus a FOURTH found here.
                                           # P-series continues at P69; probe letters at CA
ui_04_ui_05_descope_decision: SHIP_BOTH    # D15, the decision ROADMAP explicitly routed to 3.3
                                           # discuss. Neither is dropped to v1.1. The gradient costs
                                           # ZERO layout (#plan-backdrop already exists as a separate
                                           # canvas layer, index.html:119) and the elevation strip
                                           # needs NO new native fn (getVenueGeometry already carries
                                           # per-speaker z, rake.front/rear, bbox and centroid —
                                           # 3.2's P55). §R7's descope path stays live at plan
room_controls_slack_px: "582x278"          # MEASURED on the rendered page at 1100x720 against the
                                           # ui-stub, not computed. .controls-column is 582x592,
                                           # four groups end at y=386. THIS is 3.3's layout budget
verification_3_2_residual_premise_wrong: true # VERIFICATION-3.2 routed D-1/D-2 to 3.3 saying "3.3 adds
                                           # meters and an elevation strip to that same rail". It does
                                           # NOT. Meters go on the ROOM plan (UI-03/1), the strip goes
                                           # in the ROOM controls column. 3.3 does not touch the venue
                                           # rail at all, and the 251 px carried in from 3.2 is the
                                           # VENUE main column's slack, not this one. Correcting the
                                           # premise is what makes D24 a decision, not a deferral
controls_column_scrollheight_is_vacuous_too: true # D25. The SAME discuss measurement returned
                                           # controls.scrollHeight === clientHeight === 592 — D-2's
                                           # exact signature, confirmed PRESENT on 3.3's own target
                                           # column TODAY. An assertion of that form would report
                                           # green over an arbitrary overflow. The 3.3 guard is
                                           # fitted-box-against-its-stage, with an NC3-style
                                           # asymmetry proving it fires while the coarse one passes
arch_43_timer_was_never_the_mechanism: true # AMENDMENT 2, found at this boundary and on NO ONE'S
                                           # list. §4.3 said the UI reads and zeroes "at ~30 Hz on a
                                           # Timer". The RATE is right; the MECHANISM is not what this
                                           # plugin uses. Honouring it literally at plan would have
                                           # undone 3.1's deliberate choice to keep PluginEditor
                                           # Timer-free so tests/ui-stub can render the whole UI —
                                           # which is what makes the pre-integration half of every
                                           # layout gate possible. Re-pinned to the fixed-interval JS
                                           # pull + N4's never-settling promise + in-flight guard +
                                           # C++-side exchange(0) + the FRAME BASIS of 0.5/0.12
meter_poll_must_not_be_a_promise_chain: true # D20 / constraint 7. N4: completion is gated on
                                           # owner.isVisible() and DROPPED when hidden — in Release no
                                           # error, no rejection, the promise never settles. So
                                           # poll().then(poll) stops PERMANENTLY the first time the
                                           # editor is hidden and the meters never return. Fixed
                                           # interval + in-flight guard degrades to dropped frames.
                                           # The finding is READ FROM JUCE SOURCE — nothing has run
                                           # it at 30 Hz, so Q5 verifies it empirically at research
sides_predicate: "nonInterior AND |dx|/hx > |dy|/hy"  # D16 / §6.3 amendment 3. -> {3,4,7,8} on the
                                           # default venue. Speakers 1 and 2 MISS BY 6 % (dy/hy
                                           # 1.0617) — a property of the hall, not a defect. That 6 %
                                           # margin is exactly what FUNC-06/3's show-before-commit
                                           # exists to make visible instead of silent
scene_membership_derived_in_cpp: true      # D19. The page performs NO speaker arithmetic — direct
                                           # inheritance of D14's ping-readout rule. A JS
                                           # re-derivation is a mirrored fixture over R1. It is also
                                           # what makes FUNC-06/2's permutation probe meaningful:
                                           # a fixed-index implementation MUST FAIL it
empty_scene_is_not_writable: true          # D20. All-zero weights are DSP-05's SILENCE path and a
                                           # degenerate venue can legitimately empty a named scene.
                                           # Reaching silence by a mis-derived scene click
                                           # mid-concert is unrecoverable, so the control disables
scene_preview_is_hover_focus: true         # D21. Hover OR KEYBOARD FOCUS lights the resolved set;
                                           # click commits. Focus parity matters — a preview only a
                                           # pointer can trigger does not satisfy FUNC-06/3
user_slot_store_is_armed_two_step: true    # D22, deliberately ASYMMETRIC with D21: recalling a scene
                                           # is reversible, OVERWRITING a slot is not. STORE arms,
                                           # next slot click captures, auto-disarms. A judgement call
                                           # made rather than asked — plan may overturn it cheaply
meter_form_is_concentric_arc: true         # D23. The glyph's OWN stroke already renders hull
                                           # classification (3 and 8 dashed today), so the meter
                                           # cannot reuse it without the two readings colliding
n8_meters_show_the_fold_and_that_is_correct: true # Constraint 2. mapInvalid sends GainStage to its
                                           # else arm (speaker 1 = L, speakers 2-8 = R at unity), so
                                           # the meters will show SEVEN channels lit at once in that
                                           # state. That is the fold being VISIBLE, which is the whole
                                           # point of metering the output. DO NOT "fix" it
stage_3_phase_3_2_verified: true           # VERIFICATION-3.2.md — FUNC-02/04/05 + UI-01 ALL COMPLETE,
                                           # 4/4 must rows, 3/3 criteria each, ZERO partials. All 15
                                           # gates RE-RUN at verify on a forced full recompile
                                           # (134 steps, 0 diagnostics). 78 probes / 49 sections, 0
                                           # failures. 3 negative controls run as NEW work, ONE PER
                                           # GATE FAMILY, all three fired; tree byte-identical after.
                                           # No pin moved. TWO issues, both in DOCUMENTS not code
nc3_reproduced_d2_at_verify: true          # THE confirmation this verify existed to make. Re-running
                                           # NC3 as new work reproduced ALL THREE halves at once:
                                           # §8 PASSED (720<=720) and §11 [coarse] PASSED (592<=592)
                                           # while §11 [guard] FIRED (375<=213, a 162 px overflow).
                                           # D-2's attribution correction is INDEPENDENTLY CONFIRMED
br_figures_are_not_reproducible: true      # ISSUE 1, FIXED in REQUIREMENTS.md. VerifyPing's RNG is
                                           # correctly MEMBER-OWNED (kills stream interleaving) but
                                           # juce::Random's DEFAULT ctor calls setSeedRandomly()
                                           # (juce_Random.cpp:42-45), so the pink stream differs every
                                           # run. SUMMARY quoted RMS -20.07 / peak -8.99 as a
                                           # measurement; verify measured -20.14..-19.97 / -9.21..-8.47
                                           # over five runs, PASS every time. FUNC-04/3 holds BY
                                           # CONSTRUCTION: VerifyPing.cpp:258 hard-clamps with
                                           # jlimit(-ceiling,ceiling,s) at kPeakCeilDb, and the ping is
                                           # a post-write OVERWRITE so neither gain is in its path.
                                           # 3.3: seed explicitly (juce::Random rng { 1 };) IF a probe
                                           # ever needs the stream reproducible — a decision, not a default
layout_gate_section_0_has_no_timestamp: true # ISSUE 2, RECORDED not fixed. §0 prints only "NOTE: [0]
                                           # PluginEditor.cpp absent" — NO timestamp. A repo-wide search
                                           # for 14:22:05 hits ONLY planning prose, never a
                                           # machine-produced artifact. So VERIFICATION-3.1's "§0's
                                           # recorded run at 2026-08-12T14:22:05Z" OVER-ATTRIBUTES to the
                                           # gate: the stamp was transcribed from console output.
                                           # 3.3 must print a stamp from INSIDE the gate — one line
gate_11_three_module_prose_recurs: true     # ISSUE 3. VERIFICATION-3.1 issue 2 already recorded that the
                                           # "audio_basics + core + data_structures only" wording omits
                                           # juce_events; SUMMARY-3.2 repeated it verbatim. Substantive
                                           # claim HOLDS (no juce_dsp, no juce_gui_extra, no
                                           # juce_audio_processors). Has now survived TWO boundaries
gate_13_partially_discharged_3_2: true     # ROOM screen + shell CONFIRMED LIVE in WKWebView at verify:
                                           # window 1102x778, plan drew, 3 and 8 dashed, all 8 weights
                                           # 1.00, defaults per parameter-spec, em-dash correct. NEW at
                                           # 3.2: BOTH frame-level banners side by side — SAFE *and*
                                           # "MAP output set is not 8 channels", which is D13's
                                           # mapInvalid banner WITH ITS REASON in WKWebView.
                                           # The VENUE tab needs a synthetic click and this environment
                                           # lacks the accessibility permission (-25208), so the Venue
                                           # render + UI-01/3(c)'s native SAVE/LOAD modal REMAIN THE
                                           # ~8 MIN HUMAN ITEM. NOT a blocker: UI-01/3 closes on
                                           # (a) probe BN + (b) §29, both fully verified. STILL NOT D5
stage_3_phase_3_2_executed: true           # SUMMARY-3.2.md — 20/20 tasks, 78 probes / 49 sections /
                                           # 6 negative controls, 14 of 15 gates green. GATE 13
                                           # (Standalone launch-and-look) is the ONLY open item
miniplan_px_measured: "170x213"            # DEVIATION D-1, NOT Q11's predicted 270x337: Q11's mock
                                           # had FOUR rail items and no preset bar; the shipped rail
                                           # has five and the preset group alone is 116 px. The TABLE
                                           # reproduced Q11 EXACTLY at 752x277, rows 32.5 px
q11_main_column_comparison_inverts: true   # 3.3 DISCUSS. Q11 rejected the main column as a home for
                                           # the mini-plan because it would be ~240 px wide vs the
                                           # rail's 270. The rail's is 170 and the main column's
                                           # measured slack is 251 — the comparison points the other
                                           # way now. NOT acted on: D9/P63 fixes the layout
rail_scrollheight_assertion_is_vacuous: true # DEVIATION D-2. NC3 width-bound the mini-plan to
                                           # 300x375 inside a 300x213 stage — a 162 px overflow — and
                                           # railScrollHeight <= railClientHeight STILL PASSED at
                                           # 592<=592. .miniplan is a flex:1 STAGE, so the svg
                                           # overflows the STAGE and Chromium does not propagate that
                                           # into the rail. The GUARD is the fitted-box-vs-stage
                                           # assertion. Both kept, now labelled [coarse] / [guard]
gate_4_timestamp_not_captured: true        # The ORDERING held (18/18 vs the stub before VenueFile.cpp
                                           # existed) but no wall-clock stamp was taken, and mtimes
                                           # cannot corroborate it after the NC restores. An
                                           # execute-phase RECORD, not a measurement. A future phase
                                           # wanting it measured must print a stamp from IN the gate
stage_3_phase_3_2_planned: true            # PLAN-3.2.md — P51–P68, 20 tasks. All four checksums
                                           # re-measured byte-exact at the plan boundary, no pin
                                           # moved, REQUIREMENTS.md not edited (no debt dated here)
plan_3_2_probe_target: 78                  # 65 → 78. BN/BO/BV unit (VenueFile + ChannelMap reasons,
                                           # Gate 11's narrow link line SURVIVES — verified against
                                           # tests/unit/CMakeLists.txt:91-99 at plan), BP–BZ harness
plan_3_2_js_sections_target: 49            # ui_frontend_check 20 → 31 (§21–§31),
                                           # ui_layout_check 10 → 18 (§11–§18)
playwright_resolvable_at_3_2_plan: true    # NO Task 0 this phase, unlike 3.1. Found at
                                           # ~/.npm/_npx/6f4879659183bc49/node_modules/playwright,
                                           # browsers in ~/Library/Caches/ms-playwright. VERIFIED BY
                                           # RUNNING THE GATE'S OWN RESOLVER — ~/.cache/ms-playwright
                                           # does not exist on this machine and a naive check would
                                           # have reported a false blocker
p51_registry_is_derived_not_widened: true  # THE fix for the sixth instance of the enumeration-hole
                                           # class. RESEARCH said "widen both arrays in the task that
                                           # adds the file"; the plan derives PAGE_MODULES from
                                           # Source/ui/public/js/*.js instead, in TASK 1, BEFORE
                                           # venue.js exists. §1/§3/§6/§12/§14/§19 all iterate it;
                                           # new §21 asserts length>=3, ⊆ CMake SOURCES, and set
                                           # equality. Same principle as P37's kSliderIds
p52_guard_reuses_the_backstop_predicate: true # applyVenueEditChecked builds into a SCRATCH array via
                                           # ochan::buildSpeakerToBuffer ITSELF — not a second
                                           # implementation of "is this label set valid", so guard
                                           # and backstop cannot drift. applyVenueEdit stays public
                                           # (probe BL needs it); §22 asserts PluginEditor.cpp
                                           # contains no `applyVenueEdit (` call site
p54_mapfailure_out_param_is_defaulted: true # MapDiagnosis* whyNot = nullptr, so every existing
                                           # buildSpeakerToBuffer call site compiles unchanged.
                                           # isPermutationOf0to7 NOT touched — probed by name at 2.1
p56_venuefile_is_its_own_tu: true          # Source/Data/VenueFile.{h,cpp}, namespace oo::venuefile,
                                           # NO processor ref. juce::File/ValueTree/createXml all sit
                                           # inside the unit target's existing three-module link
                                           # line, so probe BN (UI-01/3a) lands in the FAST target
p60_ping_clocks_are_sample_counted: true   # 1.2s/0.4s/120s all in SAMPLES from the prepared rate —
                                           # deterministic, block-size invariant, and the ONLY form
                                           # probes BS/BT can measure offline. A juce::Timer is
                                           # unmeasurable in a render harness. RMS scalar is
                                           # CALIBRATED at execute and landed as a named constant;
                                           # probe BR asserts the result, not the constant
p60_bypass_needs_processblockbypassed: true # D11's bypass stop is an override, not free. JUCE's
                                           # default passthrough silences the ping incidentally but
                                           # leaves its STATE running and the 120 s clock frozen
negative_controls_planned_3_2: 6           # NC1 second projection in venue.js →§19 · NC2 unregistered
                                           # nativeFn →§3 · NC3 width-bound mini-plan →§11 FIRES WHILE
                                           # §8 PASSES · NC4 label reverts →§15 · NC5 drop 17 preset
                                           # gestures →§28 · NC6 remove the setVenue guard →probe BP
nc3_asymmetry_is_the_evidence: true        # A negative control that makes the NEW section fire while
                                           # the OLD one stays green is the strongest available proof
                                           # the new section was not redundant. Must be recorded with
                                           # BOTH halves in SUMMARY-3.2
gate_13_grows_to_8_min_at_3_2: true        # UI-01/3(c) is now part of it — SAVE must open a native
                                           # modal and LOAD must read the file back. STILL NOT D5
stage_3_phase_3_2_researched: true         # RESEARCH-3.2.md — all 11 questions answered, FIVE from
                                           # JUCE/module source. All four checksums byte-exact on
                                           # arrival, no pin moved
n4_completion_gated_on_isvisible: true     # juce_WebBrowserComponent.cpp:336-344 emitCompletionEvent
                                           # does jassert(owner.isVisible()) then calls
                                           # emitEventIfBrowserIsVisible (:607-611), which DROPS the
                                           # event when hidden. Release: no error, no rejection, the
                                           # JS promise NEVER settles. Broader than "the editor died"
                                           # — withKeepPageLoadedWhenBrowserIsHidden() (:587) keeps
                                           # the page ALIVE while hidden. Also hits an unregistered
                                           # name (:306-312, jassertfalse; return — no completion).
                                           # Rule: no UI state depends solely on a promise resolving
n5_preset_load_emits_unbracketed_writes: true  # applyPresetJson calls setValueNotifyingHost DIRECTLY
                                           # on the parameter (OuariconPresetManager.h:325-341), NOT
                                           # through ParameterAttachment — so F3's unchanged-write
                                           # skip does NOT apply. Up to 34 writes (17 reset + 17
                                           # apply), ALL unbracketed: AU sends bare
                                           # kAudioUnitEvent_ParameterValueChange (AU_1.mm:1341-1360),
                                           # VST3 bare paramChanged (VST3.cpp:1498-1501). All 17
                                           # params are non-meta so the meta pass is a no-op.
                                           # FIX AT O-OCTAGON'S CALL SITE (17 gestures), never in the
                                           # shared module — 4 other plugins depend on it
n8_mapinvalid_is_audible_safe_fold: true   # THE FINDING THAT CHANGES A DECISION, and it is live in
                                           # shipped Stage-2 code. mapInvalid => mappedOutputAvailable
                                           # false (PluginProcessor.cpp:264-268, 430) => GainStage
                                           # takes the else arm (:408) and writes
                                           # out[ch][n] = ch == 0 ? sL : sR (:461) with numWrite 8.
                                           # Speaker 1 gets L; speakers 2-8 ALL get R at unity.
                                           # Probe F's "retains the last valid map" is a claim about
                                           # the SNAPSHOT, not about what leaves the PA
label_swap_needs_precommit_validation: true # Consequence of N8: Q7's transient is NOT benign.
                                           # setVenue must validate the label set BEFORE
                                           # applyVenueEdit() — a guard in FRONT of the single
                                           # existing apply path, so "3.2 adds no second venue-apply
                                           # path" still holds. And the LABEL column holds-and-marks
                                           # where D12's numeric columns REVERT: reverting a label
                                           # makes L<->R unreachable, since every route from (L,R) to
                                           # (R,L) passes through a duplicate
n6_adopt_preset_manager_cpp_only: true     # preset-manager.js wires TEN native fns in its ctor
                                           # (:107-119) and createPresetBar writes container.innerHTML
                                           # (:411-434) = pattern_js_state_updater_overwrites_html_labels.
                                           # The FUNC-05 guarantee lives ENTIRELY in the header.
                                           # Adopt the header, write O-Octagon's own JS, expose FOUR
                                           # fns. Section 16's four-way closure is undisturbed
n7_mapinvalid_reason_needs_no_atomic: true # buildSpeakerToBuffer already separates 3 failure modes
                                           # (ChannelMap.cpp:49-50 size, :61-62 unresolvable label
                                           # with the row known, :67-68 duplicate) and discards them
                                           # in a bool. verifyEnumBitOrder's juce::String* whyNot
                                           # (:78-86) is the in-file precedent. NO new atomic — P43
                                           # reused: rebuildChannelMap and getStatus are BOTH message
                                           # thread. mapInvalid itself STAYS atomic (audio thread
                                           # reads it via mappedOutputAvailable)
gate_file_enumeration_hole: true           # SHARPEST ITEM FOR PLAN. ui_frontend_check.js section 3
                                           # scans [S.appJs, S.roomJs] (:165, :167) and section 19
                                           # tests S.appJs alone (:649). A new venue.js is invisible
                                           # to BOTH — they PASS by not looking. Widen both
                                           # enumerations in the SAME task that creates the file, with
                                           # a negative control that fires. Sixth time this vacuity
                                           # class has been caught
q11_measured_table_fits_rail_did_not: true # MEASURED in Chromium at 1100x720 against the REAL
                                           # styles.css. Table 752x277 px inside 592 available — 47%,
                                           # 8 rows at 32.5 px, 11 px mono tabular. 3.1's Q7 answered,
                                           # D7 AND D9 both survive. But the RAIL overflowed 601 > 592:
                                           # a WIDTH-bound mini-plan at portrait aspect 0.800 demands
                                           # 348 px of height. HEIGHT-bound gives 270x337 and 592==592.
                                           # D7's portrait consequence appearing a second time, one
                                           # layer down. Document scrollHeight was 720 the whole time —
                                           # section 8 would have PASSED
native_fn_surface_3_2_proposed: 13         # 3 carried + setVenue + saveVenue + loadVenue + 4 preset
                                           # + startPing/stopPing/getPingState. The section 3 count
                                           # literal (ui_frontend_check.js:178, `=== 3`) must move and
                                           # will FAIL loudly until it does, which is correct
ping_transport_poll_not_push: true         # Q3: a 100 ms poll while pinging ONLY. STATUS_POLL_MS is
                                           # 500 (app.js:120) — LONGER than the 400 ms auto-cycle gap,
                                           # so it can miss a gap entirely. Push rejected on three
                                           # counts, the third disqualifying: emitEvent IS
                                           # emitEventIfBrowserIsVisible, so N4 applies to it HARDER —
                                           # a dropped push never retries, a poll self-heals
ping_needs_no_reset_site: true             # Q5: SS7.2 already fixes it — override AFTER the write,
                                           # BEFORE metering. A post-write overwrite leaves all 17
                                           # smoothers advancing (GainStage.cpp:451-460), so P23/P30's
                                           # ONE RESET SITE EVER is preserved. SSOQ2's 20 ms
                                           # raised-cosine owns the discontinuity (2.3's H1 argument).
                                           # NEW: the ping must REFUSE to start when
                                           # mappedOutputAvailable() is false — pinging "speaker 5" on
                                           # a stereo fold is R1 inside its own diagnostic tool
venue_file_foreign_version_is_an_absence: true # Q6: @schemaVersion is read but NEVER branched on
                                           # (VenueModel.cpp:212-214) and every attribute falls back to
                                           # the EXISTING value. Right for session state, wrong for a
                                           # FILE: a forward .venue half-applies into the live model.
                                           # Load into a FRESH model, surface a forward version, reject
                                           # a malformed root. An ABSENCE in SS4.1, not an error — a
                                           # plan gate, NOT a re-pin
stage_3_phase_3_1_verified: true    # VERIFICATION-3.1.md — UI-02 ✅ 7/7, all 13 gates RE-RUN at
                                    # verify on a forced full recompile (128 steps, 0 diagnostics).
                                    # 2 negative controls run as NEW work, both fired; tree proved
                                    # byte-identical afterwards. No pin moved.
ui_02_criterion_6_is_historical: true  # "rendered against the stub BEFORE C++" rests on
                                    # ui_layout_check §0's recorded 2026-08-12T14:22:05Z run.
                                    # PluginEditor.cpp exists now, so verify CANNOT re-create the
                                    # pre-integration state without deleting delivered work.
                                    # Stated as an execute-phase record, not a verify measurement
probe_count: 92          # 44 unit + 48 render-harness (series A–CN, no gaps), 0 failures.
                         # CA–CH unit + CI–CN harness added at 3.3; ALL 92 re-run at verify on a
                         # FORCED FULL RECOMPILE (143 steps, 0 diagnostics), and again after the
                         # verify-phase harness fix. None regressed
js_gate_sections: 69     # ui_frontend_check 42 static + ui_layout_check 27 Playwright. A SEPARATE
                         # family from the C++ letter series, counted as SECTIONS so "92 probes,
                         # 0 failures" cannot come to mean two things in two documents
negative_controls_3_1: 8 # 4 static + 4 Playwright, ALL EIGHT FIRED; unmodified tree passes both
negative_controls: 27    # 1 at 2.1, 4 at 2.2, 8 at 2.3, 6 at 3.2, 8 at 3.3 — PLUS 8 more run as NEW
                         # WORK at the three Stage-3 verify boundaries (2 at 3.1, 3 at 3.2, 3 at 3.3),
                         # every one of which fired with the tree proved byte-identical afterwards.
                         # SIX have now CORRECTED an attribution — NC3 at 3.2 proved the assertion P62
                         # NAMED is itself VACUOUS, and NC-B at 3.3 verify proved the SAME SHAPE one
                         # level up with a ZERO-HEIGHT node: §21 [coarse] passed 592<=592 at both DPRs
                         # while §22 fired on the ordering
stage_2_phase_2_3_markers_retired: true   # PHASE-2.3-WIDTH / -AIR / -TRIM all ZERO in Source/ AND tests/
stage_2_verified: true                    # 18/18 requirement rows complete, 0 partial, 0 failed
open_manual_gate_d5: OPEN_STAGE_4          # 2.2 Task 12 (D4) + 2.3 Task 10 (D5), folded into one
                                           # ~15 min session. FOLDED INTO THE STAGE 4 HALL SESSION
                                           # by Stage 3 discuss D2 (2026-08-12). Its only unique
                                           # coverage is QUAL-01/2's *audible* clause, so QUAL-01
                                           # carries an unverified clause through ALL of Stage 3.
                                           # Risk accepted and named: if the crossing ticks audibly,
                                           # RESEARCH-2.3 H3's lever (fc(d_hull=0) toward Nyquist)
                                           # re-tunes the Stage-2 curve UNDER a finished Stage-3 UI.
stage_3_criteria_debt: CLEARED             # FUNC-06 + UI-02..05 written at Stage 3 discuss 2026-08-12
                                           # (6/7/4/4/4 criteria). 30 rows, 29 sections, 0 duplicates,
                                           # 0 sections without criteria. COMPAT-04 is the ONLY row
                                           # still without a section — known Stage-4 retroactive debt.
next_stage: 4
d_series_restarts_at_stage_3: true         # D1–D7 are Stage 3's. P-series CONTINUES from P36 → P37
d_series_continues_within_stage_3: true    # 3.2 discuss is D8–D14, NOT a restart at D1. Unlike Stage 2,
                                           # Stage 3's D1–D7 are STAGE-level and are referenced by those
                                           # numbers across STATUS / SUMMARY-3.1 / VERIFICATION-3.1;
                                           # restarting would make "D2" mean two things in one stage.
                                           # 3.2 plan starts at P51
requirements_edited_at_3_2_discuss: false  # NO criteria debt was dated to this boundary — all four
                                           # 3.2 rows (FUNC-02/04/05 + UI-01, ALL `must`) have carried
                                           # 3 criteria each since Stage 0. Re-checked programmatically:
                                           # 30 rows / 29 sections / 0 dups, COMPAT-04 still the only
                                           # row without a section. 3.1's edit discharged a debt DATED
                                           # to 3.1; none is dated here
roadmap_orphans_3_2: CLOSED                # ALL SEVEN closed, each mapped to its gate in SUMMARY-3.2.
                                           # Originally: SEVEN Phase-3.2 criteria carried by NO
                                           # requirement-row line. Each MUST become a named PLAN-3.2
                                           # gate. Sharpest two: "duplicate/missing label SURFACES the
                                           # warning" and "label-row change confirmed BY PING" attach to
                                           # FUNC-03, which is already ✅ COMPLETE — so dropping them
                                           # turns nothing yellow. Also: 120 s self-stop (no FUNC-04
                                           # line at all), ping ceiling at trim +6 dB, 12.8 s cycle,
                                           # negotiated set name, per-speaker hull class readout
ui_01_3_needs_three_part_probe: true       # DECLARED AT DISCUSS, not left for verify. Venue save/load
                                           # runs through FileChooser::launchAsync — a NATIVE MODAL.
                                           # Playwright cannot drive it; the render harness has no
                                           # editor. Closes on the P45 shape: (a) C++ probe through the
                                           # same juce::File-taking functions, (b) static assert the
                                           # completion calls exactly those, (c) Gate-13-style
                                           # Standalone launch-and-look
func_05_reduces_to_one_grep: true          # VERIFIED IN MODULE SOURCE at 3.2 discuss:
                                           # applyPresetJson iterates processor.getParameters() /
                                           # parameters.getParameter(id) ONLY — it never walks
                                           # apvts.state's children (OuariconPresetManager.h:298-350).
                                           # FUNC-05/1 holds BY CONSTRUCTION. The ONE hole is
                                           # setCustomStateCallbacks: customSave/customLoad is the only
                                           # path a preset can reach non-parameter state. D10 registers
                                           # NONE at 3.2; 3.3 registers one for SCENES, which is exactly
                                           # why FUNC-06/5 re-runs the bit-compare
apply_venue_edit_writes_state: true        # PluginProcessor.cpp:394-406 — applyVenueEdit() already does
                                           # venue.writeToState(apvts.state) → hull.build() →
                                           # rebuildChannelMap()/publishSnapshot(). A .venue LOAD reuses
                                           # it. 3.2 adds NO second venue-apply path, which is what keeps
                                           # §4.1's ordering hazard from reappearing
venue_payload_missing_at_3_1: "trims+rake" # getVenueGeometry returns x/y/z/label/class per speaker but
                                           # NOT the 8 trims and NOT the 2 rake heights — the only venue
                                           # values not yet on the wire. getStatus already carries
                                           # outputSetName / mapInvalid / numOutputChannels / venueGen
native_fn_surface_grows_at_3_2: true       # 3.1's grep-diff ran against THREE. 3.2 adds venue write,
                                           # file store, preset store and ping. This is what D1's
                                           # three-cycle structure was bought for — the gate catches a
                                           # gap against 3.2's surface, not against 3.3's combined one
scenes_tree_node: SCENES                   # NEW third child of apvts.state, SIBLING of VENUE (D6)
stage_3_native_fn_surface_3_2: 13          # VERIFIED at execute, THREE ways: the C++ registrations,
                                           # the DERIVED page-module call sites, and the ui-stub
                                           # whitelist. §3's literal moved 3 -> 13 and FAILED LOUDLY
                                           # until all thirteen existed in all three places
stage_3_native_fn_surface_3_1: 3           # VERIFIED, not predicted. getParameterDefaults /
                                           # getVenueGeometry / getStatus.
                                           # grep-diffed BOTH directions at 3.1 verify (UI-02/7)
room_plan_px: "448x560"                    # derived envelope aspect 0.800 (15.60 m x 19.50 m);
                                           # HEIGHT-bound inside 1100x720 → layout is plan-left /
                                           # controls-right. Widening the window does NOT enlarge it
gesture_bracket_obligation: CLOSED_3_1_PUCK__CLOSED_3_2_PRESET__OPEN_3_3_SCENES    # N1: WebSliderParameterAttachment routes JS writes through
                                           # setValueAsPartOfGesture — NO begin/endChangeGesture.
                                           # The puck MUST bracket srcX AND srcY; scenes MUST bracket
                                           # each of w1..w8 at 3.3 (ARCHITECTURE §6.3 is incomplete).
                                           # PLAN-3.1 P39 + ui_frontend_check §12 discharge the 3.1
                                           # half; the 3.3 half stays open
playwright_required: true                  # PLAN-3.1 P49: ui_layout_check.js FAILS rather than SKIPs
                                           # when Playwright is missing — it is the SOLE evidence for
                                           # UI-02 criteria 1/3/4/5. `npx playwright install chromium`
                                           # is Task 0 and is NOT installed as of 2026-08-12
ui_gate_files: 2                           # tests/ui_frontend_check.js (20 static sections) +
                                           # tests/ui_layout_check.js (10 Playwright sections @1100x720)
ui_02_5_e2e_gate: CLOSED_3_2           # layout §14: a coordinate typed on the VENUE screen moved the
                                       # ROOM envelope readout 15.60x19.50 -> 21.45x19.50 m, and the
                                       # metres readout with it. The 3.1 half was:
                                       # P45: (a) stub-mutation + (b) probe BL + (c) no-bbox-
                                           # literals close UI-02/5 at 3.1. The end-to-end half —
                                           # edit a coordinate on the Venue screen, watch the Room
                                           # readout move — is a 3.2 gate, declared AT PLAN
preset_manager_session_state: DO_NOT_ADOPT # N2: OuariconPresetManager::setStateFromXml calls
                                           # replaceState() and nothing else — it would bypass the
                                           # §4.1 readVenueFromState() → rebuildChannelMap() ordering.
                                           # Adopt the module for PRESETS only (3.2)
stage_3_repins_scheduled: TAKEN_3_3_DISCUSS # THREE scheduled, all scene-related: §6.3 SIDES
                                           # predicate, §4.1 three-node tree, §6.3 gesture brackets.
                                           # ALL THREE TAKEN at 3.3 discuss, plus a FOURTH found
                                           # there (§4.3's "on a Timer"). Architecture re-pinned —
                                           # the THIRD time (2.2 D2, 2.3 D2, 3.3 discuss)
ui_02_status: COMPLETE_3_1               # 7/7 criteria, each with measured evidence in REQUIREMENTS.md
gate_13_standalone: DISCHARGED_3_1       # real WKWebView: window 1102x778, plan drew, SAFE banner live.
                                         # NOT D5 — D5 remains folded into the Stage 4 hall session
venue_geometry_payload_has_bbox: true    # DEVIATION D-1: envelope and speaker bbox are DIFFERENT boxes.
                                         # normToMetres denormalises srcX/srcY against the BBOX; sending
                                         # only the envelope would force JS to invert the 15% margin rule
venue_name_utf8_fixed: true              # DEVIATION D-2: VenueModel.cpp:173 built its name through
                                         # String(const char*) = CharPointer_ASCII and mangled the em-dash
                                         # into apvts.state. Silent since Stage 2; 3.1 is the first thing
                                         # to RENDER it. ChannelMap.cpp's two `+` forms were never affected
plan_box_px_measured: "448.0x560.0"      # MEASURED on the rendered page, matching RESEARCH §3.1 exactly
ready_for_implementation: true
stage_1_open_manual_gate: CLOSED
stage_1_gate_closed_at: stage_2_phase_2.1_verify
logic_negotiated_container: create7point1
build_target: OuariconOctagon
plugin_code: OuOc
musical_parameter_count: 17
venue_value_count: 42
contract_checksums:                        # RE-MEASURED AND CORRECTED AT THE 4.1 PLAN BOUNDARY.
                                           # Both moving pins were STALE HERE: this block still held
                                           # the Stage-3.3 values while CONTEXT-4.1 and RESEARCH-4.1
                                           # had re-pinned twice. Every value below is `shasum -a
                                           # 256` on the file as it stands after the A1/A3/A4 edits
  brief: sha256:697a4f32890d7420cdef85bafbf8fe45775bf805cf1ff7b449ed2c14f6b9fbd6
  parameter_spec: sha256:b45f88dc5017ec2c1a9da49ba35242d01903000a4ff199d16758e1b6cbb9e02f
  architecture: sha256:2806c788092d9ec9c54c04bfbd3c227ce900644dd9973f66178bc0fa57bceb17
  roadmap: sha256:ea50d991d1a6b158c8ba12b57e9a0881e25308e19dc1236ca6af96d91063d424
                                           # RE-PINNED AT THE 4.2 DISCUSS BOUNDARY (2026-08-13).
                                           # Was 90c65131…fa562c92 (the 4.1-plan pin), which was
                                           # MEASURED EXACT ON ARRIVAL here before the amendment —
                                           # this is an authored change, not drift. ROADMAP §Stage 4
                                           # amended in three places by D11/D15/D20; see
                                           # roadmap_checksum_superseded (the list further down this
                                           # file — NOT a second copy; see the note there)
checksum_referent_discrepancy_4_1_discuss: |
  DISCHARGED WITH RESIDUAL at 4.1 VERIFY (2026-08-13). The finding below is CONFIRMED, not
  dismissed; what changes is that it is now closed rather than open, and the residual is PERMANENT
  rather than pending. Git holds ARCHITECTURE.md at exactly two points — 12ae50dd (Stage 0) blob
  bff8a83b, and fba35081 (the 4.1 freeze) blob 2806c788. P100 anchors BOTH ENDS of the chain but
  lands AFTER every intermediate re-pin, so cd881a10, a8a358f4 and 32a85018 are permanently
  unreconstructible. That is a fact to record once, not a gap to close later. What IS settled: all
  four contracts were re-measured at the 4.1 verify boundary and match this block exactly, and from
  fba35081 forward every boundary is independently checkable from git for the first time in this
  project. Recorded so no future reader mistakes the 3.3-era arrival tick for something that was
  ever independently confirmed — it was not, and it now cannot be.
  STANDING RULE THIS YIELDS: an arrival check compares against THIS BLOCK — the ledger — never
  against a value quoted in a prior artifact's prose. The 4.1-discuss check failed by doing exactly
  the latter. Carried to 4.2 and every boundary after it.
  ── The original finding, verbatim ────────────────────────────────────────────────────────────
  NOT a wrong contract — a wrong COMPARISON, which is the same family as
  D7 and A3 and the THIRD instance of it at this stage boundary.
  CONTEXT-4.1.md's Entry Check reports ARCHITECTURE.md "on arrival" as a8a358f4…9b6d4408 and ticks
  it ✅ "matches the Stage 3.3-discuss re-pin". But a8a358f4 is listed in this file's OWN
  architecture_checksum_superseded list as the value SUPERSEDED at 3.3 discuss, and PLAN-3.3.md's
  entry table records the live pin at 3.3-plan as 32a85018…81d85273, agreeing with this block.
  So the arrival check compared against a value already recorded as retired and reported green —
  pattern_promotion_checksum_pins_replaced_file, committed by the check that exists to prevent it.
  UNRESOLVABLE FROM GIT: the only commit touching either contract is 12ae50dd (Stage 0), whose
  blobs hash bff8a83b (ARCHITECTURE) and aec7d0ce (ROADMAP) — matching the OLDEST entry of each
  superseded list. Every re-pin since lives only in the working tree, so no arrival value between
  Stage 0 and today can be independently reconstructed. That is the actual lesson and it has a
  standing fix, not an investigation: COMMIT (4.1 Task 15, P100). The OPERATIVE pins are not in
  doubt — the files measure 2806c788 and 90c65131 today, and that is what the block above now says.
gate_16b_literal_does_not_match_the_code: |
  OPEN, owner 4.2. Found at 4.1 VERIFY by re-running the gate rather than reading SUMMARY-4.1.md.
  PLAN-4.1 Task 6 and Gate 16 specify: "presetManager.loadPreset (" appears EXACTLY ONCE in Source/.
  Measured at 4952a8ca: that literal WITH the space returns ZERO hits; without the space it returns
  ONE — and that one is PresetPolicy.h:202, the DOC-COMMENT DESCRIBING THE GATE, not a call. The
  parameter is named `manager`, not `presetManager`, so the real call is manager.loadPreset
  (presetName) at :222. The gate as spelled counts ZERO call sites and reports a plausible "1" only
  by matching its own description.
  SUBSTANCE HOLDS AND WAS VERIFIED DIRECTLY: exactly one loadPreset call site in Source/, inside
  loadPreserving. P93's "one implementation, two consumers" discipline is intact; no shipped
  behaviour is affected. WHAT DOES NOT HOLD: a future call site written presets.loadPreset(...) or
  mgr.loadPreset(...) is invisible to a grep anchored on `presetManager` — i.e. the gate cannot catch
  the likely bypass, which is the whole reason it exists.
  Same family this project keeps catching, one level up — a check that is not looking at what it
  claims to look at (pattern_criterion_discriminator_states_outcome_backwards,
  pattern_zipper_sweep_probe_needs_liveness_gate) — and this time the defect is IN A GATE.
  FIX (one line, three sites, in ONE edit per the standing parting rule): re-spell
  receiver-agnostically as  grep -rnE '\.loadPreset[[:space:]]*\(' Source/  requiring exactly one
  NON-COMMENT hit; update PLAN-4.1.md Gate 16, the PresetPolicy.h:202 doc-comment, and any later
  stage's gate list.
roadmap_checksum_superseded:               # ⚠ THE ONLY COPY OF THIS KEY. A duplicate block was
                                           # briefly added near contract_checksums at the 4.2 discuss
                                           # boundary and REMOVED on discovery: YAML resolves
                                           # duplicate keys silently (last wins), so a second list
                                           # would have DISCARDED this history without any error —
                                           # a ledger that loses entries while still parsing clean.
                                           # Keep all entries here. The same edit also nearly
                                           # recorded the 3.3 pin as "unreconstructible"; it is
                                           # recorded IN FULL at the bottom of this list
  - sha256:90c651318ac7a1cc5ec7416076b18b854308e4f072f92d39d10cb230fa562c92  # superseded at 4.2 DISCUSS (D11/D15/D20 — §Stage 4's rig premise
                                                                            # corrected: D2 assumed "an 8-channel interface at the desk" and NONE
                                                                            # IS ATTACHED; plus the bounce-order bullet split into the CR-a/CR-b
                                                                            # pair and the LFE bullet widened to gain AND low-pass). FULL value,
                                                                            # measured at the 4.2 arrival check BEFORE the amendment — this pin
                                                                            # is checkable, unlike the two elided ones below
  - sha256:643471baa689b63436a6a78573df1f1a0fa2a7f4322fb6bb833d82b43b4383d8  # superseded at 4.1 DISCUSS (D1/D2/D5/D6/D7/D9 — §Stage 4 split into
                                                                            # 4.1/4.2, "in the hall" corrected, and FOUR carried-prose residuals
                                                                            # promoted to checkboxes). Recorded HERE at the 4.1 plan boundary:
                                                                            # the discuss boundary re-pinned the artifact and not this file
  - sha256:dbb0dd5796b07ba2…18fa6da9  # PARTIAL — HEAD AND TAIL ONLY, AND DELIBERATELY SO. Superseded at 4.1 PLAN
                                      # (A1/A3/A4 — P86 CI destination, P87 the derived JUCE pin, P88 Windows,
                                      # P90 "stays down" → "is raised", P92 the preset scope's mechanism). The SECOND
                                      # ROADMAP re-pin in the project. The full 64 hex digits of the pre-edit file were
                                      # never recorded anywhere: RESEARCH-4.1 and CONTEXT-4.1 both abbreviate, the file
                                      # has since been edited, and it is uncommitted, so the middle CANNOT be recovered.
                                      # Written elided rather than reconstructed — a plausible-looking full hash here
                                      # would be exactly the wrong-referent failure this stage is about, one step worse
                                      # because it would look verifiable. Head and tail are measured
  - sha256:aec7d0ce0db9ad6c78cb1c9e9574a0a2f8ddb1cf258e6e4b701f2e2e0137ee29  # re-pinned at 3.3 PLAN (P70, TWO amendments to Phase 3.3's
                                                                            # component bullets): (1) the gradient formula max_i v_i^2 ->
                                                                            # 1/k = sqrt(denom), N10's MEASURED degeneracy — max_i v_i^2 reads
                                                                            # identically 1.0000 EVERYWHERE with one active weight because DBAP
                                                                            # normalises to sum v^2 = 1, so it measures CONCENTRATION not level;
                                                                            # (2) FOUND WHILE TAKING (1) — the meter bullet's "~30 Hz Timer read"
                                                                            # is the SAME error ARCHITECTURE §4.3 carried and that 3.3-discuss
                                                                            # amendment 2 corrected, sitting undetected in the SECOND document.
                                                                            # THE FIRST ROADMAP.md RE-PIN IN THE PROJECT — the other three all
                                                                            # moved ARCHITECTURE.md, so Gate 12 now checks four pins against TWO
                                                                            # different expected values, which is exactly the state in which a
                                                                            # checksum gate silently checks the wrong referent
architecture_checksum_superseded:
  - sha256:bff8a83b379113ac8b1e2a8915d6f1edc7183558b992bdc3808877f86c406cfe  # re-pinned at 2.2 discuss (D2, rigScale 7.95 → 7.93165)
  - sha256:cd881a10e16fc5600845bdc9569cfdca21003bfe7202823162b0d8084b10861b  # re-pinned at 2.3 discuss (D2, §3.5.2 air skip → airAmount·d_hull == 0)
  - sha256:a8a358f4be0ea1834da1540c01550c438d4c445dfbfe3d79478df0429b6d4408  # re-pinned at 3.3 discuss (FOUR amendments: §4.1 SCENES sibling + the
                                                                            # first setCustomStateCallbacks · §4.3 the pull model, NOT a juce::Timer ·
                                                                            # §6.3 the SIDES predicate · §6.3 the w1..w8 gesture brackets)
                                                                            # ⚠️ THIS is the value CONTEXT-4.1's Entry Check ticked as "on arrival" —
                                                                            # see checksum_referent_discrepancy_4_1_discuss above
  - sha256:32a850181b8ca95eeb9fea87c4a47eff7aba5b3efa0ef0befc69797881d85273  # superseded at 4.1 DISCUSS (D7 — the retired R2 prediction, still stated
                                                                            # as LIVE in §3.2.2 and §R2 three phases after Phase 2.1's manual gate
                                                                            # settled it by observation). Recorded HERE at the 4.1 plan boundary:
                                                                            # the discuss boundary re-pinned the artifact and not this file
---

# O-Octagon Status

## Current Position

Stage: **4 of 4 (Polish) — phase 4.1 of 2, ✅ VERIFIED AND CLOSED.** Stages 1, 2 and 3 all ✅
VERIFIED AND CLOSED.
Status: **`stages/4-polish/VERIFICATION-4.1.md` written.** All 18 gates **re-run from scratch**,
none read out of `SUMMARY-4.1.md`. **95 probes / 0 failures**, six pluginval s10 runs, `auval`
SUCCEEDED, CI run 31708358940 green on both jobs with `headSha` = the freeze commit, and all five
negative controls reproduced as declared with the tree byte-identical afterwards.
**Ledger: 29 complete · 0 partial · 1 pending of 30.** `COMPAT-02` is the only open row.
Next: **discuss phase for 4.2** — host-and-ear against `fba35081`.

---

## Stage 4 Phase 4.1 Verify Results (2026-08-13) — `stages/4-polish/VERIFICATION-4.1.md`

**Verdict: ✅ VERIFIED. No blockers.** `COMPAT-04` closed 3 of 3; `COMPAT-01` re-confirmed on the
final binary; `COMPAT-02` correctly untouched and owned by 4.2.

**The freeze turns out to be bit-reproducible, which is stronger than P100 asked for.** A forced
full recompile from the committed tree reproduced **both** bundle binaries byte-for-byte —
`c0fdd8f2…` (VST3) and `1e04f0a8…` (AU), matching the freeze record and the installed bundles on all
three sides. P100 asked for the freeze to be *recorded*; it is *checkable*. This changes what a 4.2
finding can conclude: a checksum mismatch at 4.2 is now a real signal, not a build-nondeterminism
excuse.

**All five negative controls reproduced**, including the sixth observation (a data-only golden
perturbation compiles clean and is caught at runtime by probe B, `slot 6 runtime 20 != golden 21`).
NC3's byproduct is the phase's sharpest single piece of evidence: under the stubbed apply, the six
read `0.00 / 4.00 / 0.10 / 1.00 / 0.35 / 0.00` — *exactly Concert Default's authored values*, so
P94's "never use Concert Default" rule is demonstrated by the failing output printing the passing
preset's value set.

### The one defect found — `gate_16b_literal_does_not_match_the_code`

**OPEN, owner 4.2.** Low in effect, notable in kind: the gate's substance holds, its spelling cannot
detect the drift it exists to prevent.

`PLAN-4.1.md` Task 6 and Gate 16 specify `presetManager.loadPreset (` appears **exactly once** in
`Source/`. Measured against `4952a8ca`:

| Search | Hits |
|---|---|
| `presetManager.loadPreset (` — the gate **exactly as written** | **0** |
| `presetManager.loadPreset(` — no space | **1**, and it is `PresetPolicy.h:202` — **the doc-comment describing the gate**, not a call |
| `\.loadPreset[[:space:]]*\(` — receiver-agnostic | **2**: that comment, and the real call `manager.loadPreset (presetName)` at `:222` |

The parameter is named `manager`, not `presetManager`, so the gate as spelled **counts zero call
sites** and reports a plausible "1" only by matching its own description. `SUMMARY-4.1.md` records it
as `.loadPreset (` once — true of the substance, silently re-worded from the plan's literal, which is
why reading the summary would not have surfaced it.

**Substance verified directly and holds:** exactly one `loadPreset` call site in `Source/`, inside
`loadPreserving`. P93's "one implementation, two consumers" discipline is intact and no shipped
behaviour is affected. **What does not hold:** a future call site written `presets.loadPreset(...)`
or `mgr.loadPreset(...)` is invisible to a grep anchored on `presetManager`.

**Same family this project keeps catching, one level up** — a check that is not looking at what it
claims to look at (`pattern_criterion_discriminator_states_outcome_backwards`,
`pattern_zipper_sweep_probe_needs_liveness_gate`), and this time the defect is *in a gate*.

**Fix, carried to 4.2 — one line, three sites:** re-spell receiver-agnostically as
`grep -rnE '\.loadPreset[[:space:]]*\(' Source/` → exactly one non-comment hit; update `PLAN-4.1.md`
Gate 16, the `PresetPolicy.h:202` doc-comment, and any later stage's gate list **in the same edit**,
per the standing parting rule.

### Gate results — every one re-run

| # | Gate | Result |
|---|---|---|
| 1 | Forced full recompile, 3 formats + both test targets | ✅ exit 0, **zero** `warning:`/`error:`/`FAILED` |
| 2 | Both C++ test targets | ✅ **95 / 0** — unit 45, harness 50 |
| 3 | `ui_frontend_check.js` | ✅ **42 sections** |
| 4 | `ui_layout_check.js` | ✅ **27 sections**, zero SKIP |
| 5 | `auval -v aufx OuOc OuDv` | ✅ SUCCEEDED, `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]` |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | ✅ six exit 0, zero `FAILED` |
| 7 | CI macOS job | ✅ run 31708358940, `headSha` = `fba35081` |
| 8 | CI Windows job | ✅ **0** MSVC warnings; Editor Automation **11.18 s**; 1-in/8-out |
| 9 | JUCE pin derived | ✅ exactly one `8.0.14` in `.github/` |
| 10 | 17 params, three sides | ✅ 17/17, ranges and defaults row-for-row |
| 11 | Unit-target link line | ✅ narrow — `RigPolicy.h` did not widen it |
| 12 | `createEditor` guard; TU absent from harness | ✅ both |
| 13 | Contract checksums + `STATUS.md` agrees | ✅ all four exact |
| 14 | Five negative controls | ✅ all five + the sixth observation; tree byte-identical |
| 15 | `gen_dbap_reference.py --check` | ✅ 102 cases |
| 16 | Static gates | ⚠️ 16a ✅, 16c ✅, **16b — see above** |
| 17 | `User/` byte-identical | ✅ **still never created**; Factory's seven files byte-identical across the run |
| 18 | Install + dual-variant sweep | ✅ `⚠ Sweeping ALTERNATE-variant` **absent**, recorded |

### Verification environment — stated so the result is re-derivable

This verify ran with two sibling Claude sessions live in the same repository. Partway through, one of
them checked the shared working tree out to `improve/o-spectralshaper-tooltips`, removing
`plugins/O-Octagon/` from disk. **No result is affected** — every gate had already been measured
against `feat/o-octagon` @ `4952a8ca`, and the remaining static measurement (Gate 16b) was taken
read-only from the commit via `git grep`. Artifacts were written from a dedicated worktree at
`../VST-development-octagon` rather than by switching the shared checkout back, so the sibling
session was not disturbed. Recorded because it is this stage's own hazard class: **a result whose
provenance is not stated is a result nobody can re-derive.**

---

## Stage 4 Phase 4.1 Discuss Results (2026-08-12) — `stages/4-polish/CONTEXT-4.1.md`

Contract checksums re-verified at the boundary — **all four byte-exact on arrival.** `ROADMAP.md` and
`research/ARCHITECTURE.md` are then **both amended and re-pinned** (D7, D1/D2/D5/D6/D9);
`BRIEF.md` and `parameter-spec.md` are untouched.

| Contract | Was | Now |
|---|---|---|
| `BRIEF.md` | `697a4f32…` | **unmoved** — seven consecutive phases |
| `parameter-spec.md` | `b45f88dc…` | **unmoved** — 17 parameters, Stage 4 adds none |
| `ROADMAP.md` | `aec7d0ce…` | **`dbb0dd57…`** |
| `research/ARCHITECTURE.md` | `a8a358f4…` | **`2806c788…`** |

### Decisions D1–D10

| # | Decision | Choice |
|---|---|---|
| D1 | Phase structure | **Two phases: 4.1 machine, 4.2 host-and-ear.** ROADMAP said "single pass"; the split is a hard dependency — 4.2 runs against a binary 4.1 froze |
| D2 | Hall access | **Logic + an 8-channel interface at the desk. No hall** — and the cost is smaller than the goal line implied |
| D3 | Windows | **In scope at 4.1.** No MSVC compiler has ever seen this code |
| D4 | Ship target | **Verified + installed locally.** No packaging, no signing, no public release |
| D5 | Factory presets | **5–6, room-character axis only** (`width`, `rolloff`, `blur`, `hullAtten`, `airAmount`, `outputGain`) — position is per-cue automation, the 8 weights are already FUNC-06's scenes |
| D6 | CI scope | **92 C++ probes on macOS + the Windows VST3/pluginval job.** The two JS gates stay local-only *by decision* — DPR/viewport-sensitive and CI-flaky in this repo |
| D7 | The retired R2 prediction, still live in three contracts | **Amend `ARCHITECTURE.md` §3.2.2 + §R2 and two `ROADMAP.md` bullets now** |
| D8 | `COMPAT-04` criteria | **Derived at this boundary from shipped source** — the last of 30 rows to get a section |
| D9 | Gate 13 interactive + Q5 | **Both fold into 4.2's single session** |
| D10 | Locks / file I/O in `processBlock` | **Stays grep + inspection. Not closed, not claimed closed** — `-fsanitize=realtime` unsupported by Apple clang 17.0.0 |

### Numbering

The D-series **restarts each stage** (Stage 2: D1–D5; Stage 3: D1–D28). **Stage 4 restarts at D1;
4.1 holds D1–D10.** The P-series and probe letters **do not restart** — Stage 3 closed at **P85** and
probe **CN**, so 4.1 begins at **P86** and **CO**.

### Open questions carried to 4.1 research

**Q1** CI invocation shape for the 92 probes *(the repo has never run a test target for any plugin —
no in-repo precedent)* · **Q2** render-harness CI wall-clock · **Q3** does the Windows VST3 build
compile, and which of the two known MSVC patterns fire · **Q4** does the WebView UI have a Windows
story at all *(WebView2 static-linking define + `withUserDataFolder()`; missing either gives a
**silently blank** WebView that passes pluginval)* · **Q5** where factory presets live — shared
`preset-manager` module or plugin-local *(this is the first time `FUNC-05`'s store separation is
tested by a **second real store** rather than by a bit-compare)* · **Q6** the presets' actual values
in engineering units · ~~**Q7**~~ **answered at the boundary — `CMakeLists.txt:15` reads
`VERSION 1.0.0` correctly.** Two facts confirmed while checking: the CMake target is
**`OuariconOctagon`**, not `O-Octagon` — this plugin **is** one of the 11-of-37 that differ, so every
CI step, install path and artefact path written at 4.1 must resolve the target, not the folder.

### Residuals

**Open:** D10's lock/file-I/O gap (blocked on toolchain, not effort) · everything in 4.2.
**Closed as residuals by promotion to criteria:** the CI gap · `COMPAT-04`'s criteria · Gate 13's
interactive half · Q5's hidden-WKWebView poll · D5's audible clause.

---

Stage: **3 of 4 (GUI) ✅ VERIFIED AND CLOSED — all three phases.** Stage 2 closed ✅ VERIFIED,
18/18 rows.
Status: **`stages/3-gui/VERIFICATION.md` written — nine requirement rows, 40 criteria, ZERO
partials.** `UI-02` (3.1) · `FUNC-02` / `FUNC-04` / `FUNC-05` / `UI-01` (3.2) ·
`FUNC-06` / `UI-03` / `UI-04` / `UI-05` (3.3). **92 C++ probes (44 unit + 48 harness) and 69 JS gate
sections (42 + 27), 0 failures**, re-run from a **forced full recompile** at each of three verify
boundaries. **29 of the project's 30 requirement rows are complete.**
Progress: `[####################]` Stage 3 of 4 · **VERIFIED** · **Stage 4 discuss next**

> 🔬 **The AO residual is RESOLVED, and it was a defect in the PROBE.** `SUMMARY-3.3.md` refused to
> round down *"1 allocation once in 35 runs, unreproduced and unattributed"* and asked verify to
> re-derive it. Verify reproduced it **4 times in 40 runs under 8-way CPU load** and attributed
> **all four** to a thread other than the one calling `processBlock`. `rtcheck::armed` was a
> **process-wide** flag, so the replaced `operator new` counted *any* thread's allocation.
> Contention does not make `processBlock` allocate — **it widens the window in which another
> thread's allocation lands inside it.** Counter now scoped to the arming thread; foreign tally
> still taken and still **reported**. Before **4/40 failed**, after **0/40**. **`PERF-01` never
> regressed.** The only tree change made at verify, confined to `tests/render-harness/main.cpp`.

> ✅ **Gate 4's ordering claim is now corroborated against the FILESYSTEM.** D27's one line landed at
> 3.3, and verify checked §0's machine stamp `2026-08-12T22:47:48.514Z` against the mtimes of the
> C++ the claim is about: **every 3.3 C++ file is newer** — `DbapSolver.h` by 40 seconds,
> `FieldSampler.cpp` by five minutes. Three boundaries from finding to closing.

> ✅ **NC-B reproduced D25's asymmetry in its strongest form** — a **zero-height** node after
> `#group-elevation`, so nothing could overflow. **§21 `[coarse]` passed `592 <= 592` at BOTH DPRs
> and §21 `[guard]` passed `123 <= 123`, while §22 FIRED on the ordering.** Every measurement of
> *size* reported green while the ordering fact they depend on was broken.

> ⏳ **STILL OPEN: Gate 13's interactive half, ~15 min human.** The **static** half went further than
> at 3.2 — **all four 3.3 components confirmed live in WKWebView** at 1102 × 778: `SCENES` with
> `U1`–`U4` **struck through and disabled** and `STORE` in the title row; the `ELEVATION` strip with
> the rake solid, both extrapolations dashed, pairs reading **1 2 / 3 8 / 4 7 / 5 6** and both
> readouts equal at **2.15 m** because `srcZ = 0` rides the rake; the **field gradient** with hull
> and glyphs still legible and the legend **`0.0 – 2.7 dB`**; the eight meter arcs at rest. Every
> *interactive* item needs a synthetic click and this environment returns **`-25208`**, exactly as
> at 3.2. **It is still not D5.**
> `build/plugins/O-Octagon/OuariconOctagon_artefacts/Release/Standalone/O-Octagon-dev.app`

> ⏳ **Q5 has now gone unrun by FOUR consecutive phases** — a 30 Hz meter poll against a **hidden**
> WKWebView. Attempted at verify and abandoned for a stated reason: with the Standalone's input
> muted the meters read zero either way, so *"resume"* has no observable at rest, and
> `js/meters.js`'s `dropped` counter needs devtools **into** the WKWebView. It needs a human with a
> signal running.

## Stage 3 Phase 3.3 Verify Results (2026-08-12) — `stages/3-gui/VERIFICATION-3.3.md`

**Four rows close — `FUNC-06`, `UI-03`, `UI-04`, `UI-05` — 18 criteria, zero partials. All 16 gates
re-run from scratch on a forced full recompile (143 steps, 0 diagnostics), then the whole set re-run
a SECOND time after the one fix this verify landed.**

| # | Item | Disposition |
|---|---|---|
| **1** | **The AO residual — reproduced, attributed, FIXED.** 4 of 40 loaded runs fired; `DIAG foreign-thread 1` on **all four**, `foreign 0` on the other 36. The allocation counter was process-wide | **FIXED** in `tests/render-harness/main.cpp` — the only tree change at verify. `PERF-01` never regressed; `processBlock` measured **0** at every load level. Locks and file I/O remain grep + inspection (`-fsanitize=realtime` unsupported by Apple clang 17.0.0) |
| **2** | **`SUMMARY-3.3.md` reports 141 build steps; verify measured 143, twice** | **RECORDED.** Two steps, no diagnostic, no claim affected — noted so a future phase comparing against 141 does not read a drift that is not one |
| **3** | **Layout §27's counter baseline is run-dependent.** Summary quotes `12 → 12` / control `12 → 13`; verify measured `11 → 11` / `11 → 12` | **RECORDED, not a defect.** The baseline is however many 2 Hz ticks elapsed before the drag; **the assertion is the DELTA — 0 and 1 — and both held exactly** |
| **4** | **Gate 11's three-module prose has now survived THREE boundaries** (3.1, 3.2, 3.3) | Substantive claim **holds** — re-verified from `ninja -t commands` with both new TUs joined. `juce_events` is transitive via `juce_data_structures`; the asserted invariant is the **absence** of `juce_dsp` / `juce_gui_extra` / `juce_audio_processors`, never the presence of exactly three |

**Three negative controls run as NEW work, one per gate family, all three fired; tree proved
byte-identical afterwards (49 files, zero drift).** NC-A — meters indexed by identity → **CM fired
with `wrongIndex 8` while CN still passed**. NC-B — zero-height node after `#group-elevation` →
**§22 fired while §21 passed at both DPRs**. NC-C — `app.js`'s guard released only on settlement →
**§33 fired**.

## Stage 3 Phase 3.2 Verify Results (2026-08-12) — `stages/3-gui/VERIFICATION-3.2.md`

**Two issues found, both in DOCUMENTS rather than in delivered code. Nothing shipped was wrong.**

| # | Issue | Disposition |
|---|---|---|
| **1** | **`FUNC-04/3`'s evidence quoted a non-reproducible figure as a measurement.** SUMMARY recorded probe BR at `RMS −20.07 / peak −8.99 dBFS`; five verify runs gave **−20.14 … −19.97** and **−9.21 … −8.47**, PASS every time. Cause: `VerifyPing`'s `juce::Random` is correctly **member-owned** (which does kill stream interleaving) but the **default ctor calls `setSeedRandomly()`** (`juce_Random.cpp:42-45`), so the pink stream differs every run | **FIXED** in `REQUIREMENTS.md`. FUNC-04/3 holds **by construction** — `VerifyPing.cpp:258` hard-clamps with `jlimit` at `kPeakCeilDb`, and the ping is a post-write **overwrite** so neither `outputGain` nor the trims are in its path. **3.3:** seed explicitly only if a probe ever needs reproducibility — a decision, not a default |
| **2** | **`ui_layout_check.js` §0 emits no timestamp.** It prints only `NOTE: [0] PluginEditor.cpp absent`. A repo-wide search for `14:22:05` hits **only planning prose** — never a machine-produced artifact — so VERIFICATION-3.1's *"§0's recorded run at 2026-08-12T14:22:05Z"* **over-attributes to the gate**; the stamp was transcribed from console output | **RECORDED, not fixed** — changing §0 now cannot retroactively create 3.2's record. **3.3 lands one line** printing a stamp from inside the gate, after which the claim is self-evidencing |
| **3** | **Gate 11's three-module prose enumeration recurs.** VERIFICATION-3.1 issue 2 already flagged that the list omits `juce_events`; SUMMARY-3.2 repeated it verbatim. Has now survived **two** boundaries | Substantive claim **holds exactly** — no `juce_dsp`, no `juce_gui_extra`, no `juce_audio_processors`. Prose only |

**Also confirmed at verify:** **D-1** — §12 measured the shipped mini-plan at **170.0 × 213.0 px**
(aspect 0.7981 vs the returned 0.8000) while gate 15's study still reproduces Q11's own mock at
270 × 337; the two figures describe **different pages**, exactly as SUMMARY-3.2 states. Gate 9 was
re-derived by an **independent** four-way parse (spec table ↔ `makeFloat` call sites ↔ `params::id()`
table ↔ relay construction): **17/17**, no `kSliderIds` list.

## Stage 3 Phase 3.2 Execute Results (2026-08-12) — `stages/3-gui/SUMMARY-3.2.md`

**Every plan decision landed. Three findings are worth carrying forward on their own.**

| | Finding |
|---|---|
| **NC3 corrected an attribution** | The assertion **P62 named** — `railScrollHeight <= railClientHeight` — **is vacuous in this layout.** Width-binding the mini-plan to 300×375 inside a 300×213 stage, a **162 px overflow**, still reported `592 <= 592` and PASSED. `.miniplan` is a `flex:1` STAGE, so the `<svg>` overflows the *stage* and Chromium does not propagate that into the rail. **The guard is the fitted-box-vs-stage assertion**, which is what fired. Both kept, now labelled `[coarse]` / `[guard]`. Fifth negative control in this project to correct an attribution rather than confirm one |
| **NC3's asymmetry, both halves** | **§8 PASSED** (`scrollHeight 720 <= 720`, "the Venue screen also fits") **while §11 FIRED** (`the fitted plan is inside its stage — 375 <= 213`). That is the evidence §11 was not redundant |
| **Q11's other conclusion inverts** | The mini-plan lands at **170×213**, not the predicted 270×337 — Q11's mock had four rail items and no preset bar (116 px). The **table** reproduced Q11 exactly at **752×277, rows 32.5 px**. But Q11 rejected the main column as a home for the plan because it "would be ~240 px wide vs the rail's 270" — the rail's is **170** and the main column's slack is **251**. **3.3 discuss**, not acted on here: D9/P63 fixes the layout |

Also found: **four static sections were scanning RAW source for tokens their own comments contain**
(§20/§31 SafePointer, §27 `setCustomStateCallbacks`, §30 `juce::Timer`/`getSystemRandom`) — the rule
`ui_frontend_check.js` states at the top of itself, and §20 was the one place not following it. 3.2
is the phase that wrote the comments which exposed it. All now scan comment-stripped code.

## Stage 3 Phase 3.2 Plan Results (2026-08-12) — `stages/3-gui/PLAN-3.2.md`

**Two orderings are load-bearing, and the first is new to this phase.** Task 1 — the derived
page-module registry — runs **before `venue.js` exists**, not in the same task that creates it.
Tasks 2–8 complete and Task 8 **passes** before Task 9 writes a line of C++; D4 removed the
browser-iteration safety net for the whole stage, not just for 3.1.

| P | Decision |
|---|---|
| **P51** | **The page-module enumeration is DERIVED FROM DISK, not widened by hand.** RESEARCH's sharpest item was that §3 scans `[S.appJs, S.roomJs]` and §19 tests `S.appJs` alone, so a new `venue.js` makes both **pass by not looking** — the sixth instance of this vacuity class. The plan does **not** add `S.venueJs` to two arrays; it reads `Source/ui/public/js/*.js` and has §1/§3/§6/§12/§14/§19 iterate it, with a new **§21** asserting `length >= 3`, membership in the CMake SOURCES, and set equality. Same principle as P37's `kSliderIds` |
| **P52** | **`setVenue` validates BEFORE it applies, and the predicate is `buildSpeakerToBuffer` ITSELF.** `applyVenueEditChecked` builds into a scratch array — not a second implementation of "is this label set valid", so guard and backstop cannot drift. `applyVenueEdit` stays public (probe BL needs it); **§22 asserts the editor never calls it** |
| P53 | **The label column holds-and-marks; every numeric column reverts.** A reachability argument, not taste: every route from `(L,R)` to `(R,L)` passes through a duplicate, so a reverting label makes L↔R **unreachable**. While the set is not a permutation the page does not call `setVenue` at all |
| P54 | `mapInvalid` carries `{ reason, speakerIndex }` through a **defaulted** out-param, so every existing call site compiles unchanged. **No new atomic** — P43 reused, both ends are the message thread. `mapInvalid` itself stays the atomic it is |
| P55 | `getVenueGeometry` gains **8 trims + 2 rake heights** — `trimDb` **inside** each speaker object so the two cannot be indexed out of step. Still **one** call: P38's torn-read argument is untouched |
| P56 | **`Source/Data/VenueFile.{h,cpp}`, no processor reference.** Fresh model, forward version surfaced, malformed root rejected without touching the live venue. It joins the **unit** target and **Gate 11's narrow link line survives** — verified against `tests/unit/CMakeLists.txt:91-99` at plan, which is what puts UI-01/3's probe in the fast target |
| P57 | UI-01/3 closes on **(a)** probe BN through the same `juce::File`-taking functions, **(b)** §29's static proof the completion calls exactly those with no parallel serialisation path, **(c)** Gate 13's native modal. (b) is what makes (a) non-vacuous |
| P58 | Adopt the preset manager's **header only** — four native functions, not the JS side's ten, and **no `createPresetBar`**. `setCustomStateCallbacks` is registered **nowhere**, which is FUNC-05's strongest form and one grep |
| P59 | **Bracket the preset load with 17 gestures at O-Octagon's call site.** N5: `applyPresetJson` writes directly on the parameter, so F3's unchanged-write skip does not apply and up to 34 writes go out bare. **Never edit the shared module** — four plugins depend on it |
| P60 | `VerifyPing` is a **post-write overwrite**, **no `reset()`** (P23/P30's one-reset-site-ever holds; §OQ2's 20 ms raised cosine owns the discontinuity), and **both clocks are counted in SAMPLES** — the only form probes BS/BT can measure offline. Bypass needs an actual `processBlockBypassed` override; the RMS scalar is **calibrated at execute** and probe BR asserts the result rather than the constant |
| P61 | **100 ms poll while pinging only.** `STATUS_POLL_MS` is 500 — *longer* than the 400 ms gap. Push rejected on three counts, the third disqualifying: `emitEvent` **is** `emitEventIfBrowserIsVisible`, so N4 hits it harder — a dropped push never retries, a poll self-heals |
| P62 | **A plan box is fitted to the smaller of its two bounds**, stated once and exported once (`fitBox` in `roomplan.js`, used by both plans). §11 asserts `railScrollHeight <= railClientHeight` — **§8 measures the document and would have passed the 9 px rail overflow** |
| P63 | Table-left / rail-right, at Q11's measured figures. The `mapInvalid` banner is **frame-level, beside SAFE, on both screens** (D13) |
| P64 | **No UI state depends solely on a promise resolving** (N4). Authoritative state converges on the `venueGen` poll; the chooser's promise may stay pending. Additive to the SafePointer rule, not a replacement |
| P65 | Native surface is exactly **THIRTEEN**; `ui_frontend_check.js:178`'s `=== 3` **must move and will fail loudly until it does** |
| P66 | **BN–BZ → 78 probes** (36 unit + 42 harness). JS gates **30 → 49 sections**, counted as a separate family so "78 probes" cannot come to mean two things |
| **P67** | **Six negative controls declared AT PLAN**, each naming the gate it must make fire. **NC3 is the sharpest: width-bind the mini-plan and §11 fires WHILE §8 PASSES** — that asymmetry is the evidence §11 was not redundant |
| P68 | `REQUIREMENTS.md` is **not** edited — no criteria debt is dated to this boundary. The seven ROADMAP orphans are a **traceability** gap, carried as named gates |

### Declared at plan, not left for execute

- **Task 0 is discharged.** Playwright **is** resolvable — verified by running the gate's own
  resolver, not by checking `~/.cache/ms-playwright`, which does not exist on this machine and would
  have reported a false blocker. Browsers live in `~/Library/Caches/ms-playwright`.
- **Gate 13 grows from ~5 to ~8 min** because UI-01/3(c) is now part of it: SAVE must open a native
  modal and LOAD must read the file back. **It is still not D5.**
- **Three things must reach `SUMMARY-3.2.md` or 3.3 rediscovers them:** N8's inheritance (a 3.3 probe
  asserting map retention must assert against the **snapshot**, not the output buffer) · the still-open
  3.3 half of the gesture obligation (`w1..w8` per scene) · FUNC-05's changed shape at 3.3 ("exactly
  one callback, touching only `SCENES`" replaces "no callback at all").

### Predicted outcome — unchanged from discuss and research

**FUNC-02 ✅ · FUNC-04 ✅ · FUNC-05 ✅ · UI-01 ✅ — all four `must`, zero partials declared in
advance**, the fifth consecutive phase under that discipline. Also closing: **UI-02/5's inherited
end-to-end gate** (layout §14) and **all seven ROADMAP orphans**. Residuals unchanged: D5/QUAL-01's
audible clause · the CI gap, which 3.2 widens again · `COMPAT-02` · `COMPAT-04` · UI-04/05 descope
(3.3) · D7's legibility cost (3.3) · the three §8 re-pins (3.3).

## Stage 3 Phase 3.2 Research Results (2026-08-12) — `stages/3-gui/RESEARCH-3.2.md`

**Two findings change a decision, and one of them is live in shipped Stage-2 code.**

| # | Finding | Consequence |
|---|---|---|
| **N4** | A native function's `complete()` is gated on `isVisible()` — `emitCompletionEvent` does `jassert (owner.isVisible())` then `emitEventIfBrowserIsVisible`, which **drops the event** when hidden (`juce_WebBrowserComponent.cpp:336-344, 607-611`) | **Q4's hole is real and BROADER than "the editor died"**: `withKeepPageLoadedWhenBrowserIsHidden()` keeps the page **alive** while hidden, so the promise is abandoned inside a page that is still running. In Release: no error, no rejection, no log. **No UI state may depend solely on a promise resolving** |
| **N5** | `applyPresetJson` calls `setValueNotifyingHost` **directly on the parameter** (`OuariconPresetManager.h:325-341`), not through `ParameterAttachment` | **F3's unchanged-write skip does NOT apply.** Up to **34 unbracketed** host writes per preset load — bare `kAudioUnitEvent_ParameterValueChange` (AU) / bare `paramChanged` (VST3). Q1 answered **yes**. Fix at O-Octagon's call site with 17 gestures; **never edit the shared module** |
| **N8** | **`mapInvalid` does not play through the retained map — it falls to the SAFE fold.** `mappedOutputAvailable()` false → `GainStage` `else` arm → `out[ch][n] = ch == 0 ? sL : sR` with `numWrite 8` (`GainStage.cpp:408, 461`) | **Speaker 1 gets L; speakers 2–8 ALL get R at unity.** Probe F's "retains the last valid map" is a claim about the **snapshot**, not about what leaves the PA. **Q7's label-swap transient is audible and wrong** |
| **N6** | `preset-manager.js` wires **TEN** native fns in its constructor and `createPresetBar` writes `container.innerHTML` (`:107-119, 411-434`) | The FUNC-05 guarantee lives **entirely in the header**. **Adopt the C++ only**, expose four functions, write O-Octagon's own JS. §16's four-way closure undisturbed |
| **N7** | `buildSpeakerToBuffer` already separates three failure modes and discards them in a `bool`; `verifyEnumBitOrder`'s `String* whyNot` is the in-file precedent (`ChannelMap.cpp:49-86`) | **Q9 = yes, and it needs NO new atomic** — P43 reused: writer and reader are both the message thread. `mapInvalid` itself stays atomic |

### N8's consequence — `setVenue` validates BEFORE it applies

A label swap must pass through a duplicate, which the map rejects — and after N8 the rejection is
**seven speakers playing the same signal at unity** for as long as the user takes to type the second
label. So `setVenue` validates the label set **before** `applyVenueEdit()`: a guard *in front of* the
single existing apply path, so **"3.2 adds no second venue-apply path" still holds.**

**And one UI rule that contradicts D12 for exactly one column:** numeric fields revert on invalid;
**the label column holds-and-marks.** Reverting a label makes L↔R *unreachable*, because every route
from `(L,R)` to `(R,L)` passes through a duplicate.

### Q11 — measured, and it inverts the expectation

Rendered in Chromium at 1100 × 720 against the **real `styles.css`**, its tokens and its type scale.

**The 42-field table was never the constraint** — **752 × 277 px** inside 592 available, **47 %**,
8 rows at 32.5 px, 11 px mono `tabular-nums`. **3.1's Q7 is answered; D7 and D9 both survive.**

**The rail overflowed — 601 > 592 — and the mini-plan was why.** Width-bound at the portrait
envelope aspect **0.800** it demands 348 px of height. **Height-bound it measures 270 × 337 and the
rail fits at 592 == 592.** This is D7's portrait consequence appearing a second time, one layer down
— the Room plan is height-bound at 448 × 560 for exactly the same reason.

> ⚠️ **`document.scrollHeight` was 720 the whole time.** `ui_layout_check.js` §8 measures the
> document, so **as written it would have passed.** The rail needs its own assertion:
> `railScrollHeight <= railClientHeight`.

### The sharpest item for plan — two gates enumerate their inputs by NAME

`ui_frontend_check.js` §3 scans `[S.appJs, S.roomJs]` (`:165, :167`); §19 tests `S.appJs` alone
(`:649`). **A new `venue.js` is invisible to both — they PASS by not looking.** Sixth time this
vacuity class has been caught. **Widen both enumerations in the same task that creates the file**,
with a negative control that fires. The stub whitelist is safe by construction (§3 asserts set
equality, so a missing entry fails).

### The other five questions

**Q2** `customState` is the right `SCENES` home at 3.3 — and because N2 keeps session state out of
the module, `setStateFromXml`'s half is never invoked, so the callback is reachable **only** from a
preset load · **Q3** a **100 ms poll while pinging only**; `STATUS_POLL_MS` is 500, *longer* than the
400 ms gap. Push rejected on three counts, the third disqualifying: `emitEvent` **is**
`emitEventIfBrowserIsVisible`, so N4 hits it harder — a dropped push never retries, a poll self-heals
· **Q5** §7.2 already fixes placement (after the write, before metering); a post-write overwrite
leaves all 17 smoothers advancing, so **P23/P30's one-reset-site-ever is preserved**, and §OQ2's
20 ms raised-cosine owns the discontinuity. **New: the ping must refuse to start when
`mappedOutputAvailable()` is false** — pinging "speaker 5" on a stereo fold is R1 inside its own
diagnostic tool · **Q6** `@schemaVersion` is read but **never branched on** and every attribute falls
back to the *existing* value — right for session state, wrong for a file. Fresh model, surface a
forward version, reject a malformed root. **An absence in §4.1, so a plan gate and NOT a re-pin** ·
**Q8** `metresToPx(mx, my, view)` is **already parameterised** — a mini-plan is a second `view`, not a
second projection. **§19 is not weakened; it is widened.**

### Native-function surface: 3 → **13**

`getParameterDefaults` · `getVenueGeometry` (**gains the 8 trims + 2 rake heights**, still one call) ·
`getStatus` · `setVenue` · `saveVenue` · `loadVenue` · `savePreset` · `loadPreset` · `getPresetList` ·
`getCurrentPreset` · `startPing` · `stopPing` · `getPingState`. **The `registered.size === 3` literal
at `ui_frontend_check.js:178` must move and will fail loudly until it does — which is correct.**

### Predicted outcome — unchanged

**FUNC-02 ✅ · FUNC-04 ✅ · FUNC-05 ✅ · UI-01 ✅, all four `must`, zero partials declared in advance.**
Nothing in this research contradicts it. **No contract amended, no checksum moved.** Residuals
unchanged: D5/QUAL-01's audible clause · the CI gap · `COMPAT-02` · `COMPAT-04` · UI-04/05 descope
(3.3) · D7's legibility cost (3.3) · the three §8 re-pins (3.3).

---

## Stage 3 Phase 3.2 Discuss Results (2026-08-12) — `stages/3-gui/CONTEXT-3.2.md`

3.2 verifies **FUNC-02, FUNC-04, FUNC-05, UI-01 — four rows, ALL `must`**, the heaviest requirement
load of any Stage-3 phase. **No criteria debt at this boundary** (all four rows have carried 3
criteria each since Stage 0; `REQUIREMENTS.md` is NOT edited here). **Seven ROADMAP criteria are
carried by no requirement row** and must each become a named plan gate — two of them attach to
`FUNC-03`, which is already ✅ complete, so dropping them turns nothing yellow.
Branch: `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 / 3.1 work uncommitted)

**3.2 is where the plugin stops being a renderer of state and becomes an editor of it.** Every
requirement it carries is a write path: 42 typed venue values, a `.venue` file, a musical preset, and
an audio generator commanded from a button. 3.1's hazards were silent *rendering* bugs; 3.2's are
silent *state* bugs, and two are unrecoverable in a hall — a venue that half-applied, and a preset
that quietly rewrote the room.

| # | Decision | Choice |
|---|----------|--------|
| D8 | Venue field commit | **Blur / Enter commits; Escape reverts. ONE `setVenue` call carrying all 42 values.** Live-per-keystroke applies transiently DEGENERATE venues (`12.5` passes through `1` and `12`, each getting a real `hull.build()`); a per-field surface is **P38's torn-read hazard on the write side** |
| D9 | Venue screen layout | **Table-left / rail-right, with a LIVE MINI-PLAN in the rail.** Resolves 3.1's open Q7: the 42 values are ONE 8-row table + 2 rake fields, not a 42-field grid — **D7's 1100 × 720 survives.** The mini-plan is the feedback D8 removes, in a better form: a mis-typed `12` for `1.2` shows as a rig that CHANGED SHAPE |
| **D10** | Musical preset store | **Adopt `OuariconPresetManager` for PRESETS ONLY, and register NO custom-state callback at 3.2.** N2 stands: session state keeps O-Octagon's own get/`setStateInformation` |
| D11 | Verify-ping lifetime | **Four stops: editor close · bypass · 120 s latch · explicit Stop. A tab switch is NOT one** — an operator walking the hall needs the ping alive while they look at the Room plan |
| D12 | Numeric input element | **`type="text"` + `inputmode="decimal"` with an explicit parse — NOT `type="number"`.** `type=number` makes FUNC-02/1 UNTESTABLE: on invalid content `.value` is `""` and `.valueAsNumber` is `NaN`, so "typed `abc`" and "cleared the field" are indistinguishable. **Reject means revert, not hold** |
| D13 | `mapInvalid` surfacing | **A frame-level banner beside the SAFE banner, visible on BOTH screens.** R1's defining property is silence; putting its only warning behind a tab reproduces the failure it exists to break. `getStatus` already returns `mapInvalid` — a rendering decision, not new plumbing |
| D14 | Ping active-speaker indicator | **C++ is the authority; never a JS `setInterval` re-deriving the step.** A drifted JS cycle names speaker 5 while 6 sounds, during the one procedure whose purpose is confirming speaker N is speaker N — **R1 reproduced inside its own diagnostic tool** |

### Two source findings that SHRINK 3.2 — verified, not assumed

1. **`applyVenueEdit()` already writes back to `apvts.state`** (`PluginProcessor.cpp:394-406`).
   A `.venue` load is *parse → build a `VenueModel` → `applyVenueEdit()`*, reusing the single
   construction site. **3.2 adds no second venue-apply path**, which is what keeps §4.1's ordering
   hazard from reappearing.
2. **`applyPresetJson` cannot reach the VENUE node** — it iterates
   `parameters.processor.getParameters()` and resolves via `parameters.getParameter (id)`
   (`OuariconPresetManager.h:298-350`), never walking `apvts.state`'s children. **FUNC-05/1 holds by
   CONSTRUCTION.** The one hole: `setCustomStateCallbacks` is the only path by which a preset can
   reach non-parameter state, so **FUNC-05 reduces to a single greppable assertion.** D10 registers
   none at 3.2; 3.3 will register one for `SCENES` — which is exactly why FUNC-06/5 re-runs
   FUNC-05's bit-compare rather than inheriting it. **The two decisions were made a phase apart and
   they line up.**

### UI-01/3 cannot be closed by ANY existing gate class — declared at discuss

*"Venue save/load round-trips through a named file"* runs through `FileChooser::launchAsync`, a
**native modal**: Playwright cannot drive it and the render harness has no editor. Left undeclared it
becomes a verify-time surprise on a **`must`** row. Closes on the P45 three-part shape — (a) a C++
probe through the same `juce::File`-taking functions the completion calls, (b) a static assertion
that the completion calls exactly those and no parallel path exists, (c) a Gate-13-style Standalone
launch-and-look for the modal itself.

### Seven ROADMAP criteria carried by no requirement row — each must become a plan gate

120 s ping self-stop (**no FUNC-04 line at all**) · ping ceiling at `trim = +6 dB` (FUNC-04/3 says
`outputGain` only) · auto-cycle **12.8 s** · duplicate/missing label **surfaces** the warning ·
label-row change **confirmed by ping** · the **negotiated set name** on screen (Stage 4's R2 reads it
off this screen) · per-speaker **hull classification** readout. **Items 4 and 5 attach to `FUNC-03`,
already ✅ complete** — nothing in `REQUIREMENTS.md` goes yellow if they are dropped.

### Predicted outcomes, declared at discuss

**FUNC-02 ✅ · FUNC-04 ✅ · FUNC-05 ✅ · UI-01 ✅ — all four `must`. Zero partials declared in
advance**, the fourth consecutive phase under that discipline (three Stage-2 phases + 3.1, zero
verify-time surprises). Also closing: **UI-02/5's inherited end-to-end gate** (P45) and the seven
orphans. **No contract gaps found** — unlike 3.1, everything 3.2 needs is specified (§4.1, §4.4,
§OQ2, §6.3, §F9). **Residuals unchanged:** D5/QUAL-01's audible clause · the CI gap, which 3.2
widens again · `COMPAT-02` · `COMPAT-04` · UI-04/05 descope (3.3) · D7's legibility cost (3.3) · the
three §8 re-pins (3.3).

### Eleven open questions handed to research

**Q1 — does `applyPresetJson`'s reset loop emit 17 spurious host automation writes?** It calls
`setValueNotifyingHost(getDefaultValue())` on every parameter before applying; in Logic with a lane
in Latch/Touch that could record a full default sweep, and FUNC-05/1 rests on it · **Q4 — a
`FileChooser` launched from inside a native function leaves the JS promise unresolved** if the editor
dies (SafePointer says `return` bare, but `await nativeFn("loadVenue")` then never settles — the
pattern does not cover this context) · **Q3 — ping indicator transport**: `getStatus` polls at 2 Hz
but the auto-cycle gap is 0.4 s, and a push path would be the first non-pull-only surface in this UI
(the ui-stub does not model `backend.addEventListener`) · whether `customState` is `SCENES`' home at
3.3 · where the ping injects relative to `GainStage`'s smoothers, and whether start/stop needs a
`reset()` · `.venue` schema-version mismatch policy · the **label-swap transient** (L↔R must pass
through a duplicate, which the map rejects) · whether the mini-plan breaks §19's *exactly one
`metresToPx` definition* — **the gate must not be weakened to accommodate the feature** · whether
`mapInvalid` needs a reason code rather than a boolean · what the preset-manager's 447-line JS side
assumes about the DOM and what it costs the bridge grep-diff · **whether the 42-field table actually
fits at 1100 × 720 at a legible type size** (3.1's Q7, now answerable — and if it does not, D7 is
revisited HERE, not at 3.3).

---

## Stage 3 Phase 3.1 — VERIFIED

Stage: **3 of 4 (GUI) — Phase 3.1 of 3 ✅ VERIFIED.** Stage 2 closed ✅ VERIFIED, 18/18 rows.
Status: **`stages/3-gui/VERIFICATION-3.1.md` written — `UI-02` ✅ COMPLETE, 7/7 criteria, each
RE-MEASURED at the verify boundary rather than read out of SUMMARY.** All 13 gates re-run on a
forced full recompile (**128 steps, zero `warning:` / `error:` / `FAILED`**). **65 C++ probes
(33 unit + 32 harness), 0 failures. 30 JS gate sections, 0 failures.** `auval` SUCCEEDED; pluginval
s10 **6/6**; DBAP fixture **102 cases**; 17/17 params re-derived by an **independent** spec↔C++ parse;
bridge closure **3 == 3** re-derived by hand. All four contract checksums byte-exact, **no pin moved.**
**Two negative controls run as NEW work at verify — both fired**, tree proved byte-identical after.
**Gate 13 re-discharged with a fresh WKWebView screenshot** — window 1102 × 778.
Progress: `[###############.....]` Stage 3 phase 3.1 of 3 **complete** · **3.2 discuss next**
Branch: `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 / 3.1 work uncommitted)

## Stage 3 Phase 3.1 Verify Results (2026-08-12) — `stages/3-gui/VERIFICATION-3.1.md`

**Verdict: ✅ VERIFIED. Ready for Phase 3.2: yes, no blockers.** UI-02 closes; the other eight
stage-3 rows remain `pending` by design. Zero partials declared, zero found.

| Gate | Verify-phase result |
|---|---|
| Clean 3-format build + both test targets, **forced full recompile** | ✓ **128 steps, 0 `warning:` / 0 `error:` / 0 `FAILED`** |
| `ui_frontend_check.js` / `ui_layout_check.js` | ✓ exit 0 / exit 0 — 20 + 10 sections, **`skip` count 0** |
| Both C++ test targets | ✓ **33 + 32 = 65 probes, 0 failures** (BK `1:V 2:V 3:E 4:V 5:V 6:V 7:V 8:E`; BL gen 3→4; BM `mono:SAFE stereo:SAFE 7.1:REAL 7.1-SDDS:REAL 5.1.2:REAL`) |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED** |
| pluginval s10 — VST3 ×3, AU ×3 | ✓ **6/6 exit 0**, zero `FAILED` |
| `gen_dbap_reference.py --check` | ✓ **102 cases OK** |
| 17 params vs `parameter-spec.md` | ✓ **17/17** — re-derived by an INDEPENDENT parse of both documents |
| Bridge grep-diff, both directions | ✓ **3 == 3**, re-derived by hand: `nativeFn("…")` sites == `withNativeFunction` registrations |
| Unit-target link line | ✓ `juce_audio_basics` / `juce_core` / `juce_data_structures` / `juce_events` — **no `juce_dsp`, no `juce_gui_extra`** |
| Contract checksums | ✓ all four byte-exact, **no pin moved** |
| **Gate 13 — Standalone WKWebView** | ✓ **re-discharged**: window **1102 × 778**, plan drew, 3/8 dashed, 8 weights `1.00`, 9 controls at spec defaults, footer `SOURCE 6.50 × 12.00 m · ENVELOPE 15.60 × 19.50 m`, SAFE banner live, **em-dash correct (D-2 confirmed)** |

**Two negative controls as NEW work at verify — both fired.** NC2 (a literal `-1.30f` envelope
bound) → §13 `found 1.30f`, exit 1. NC8 (hardcoded `aspect = 0.800`) → §2 fails **twice**, exit 2.
**Tree proved byte-identical afterwards** (32 files, `shasum -a 256`) and both gates returned exit 0
on the restored tree. **Neither new gate file is vacuous.**

### Three issues found at verify — one fixed, none a defect in delivered code

1. **`REQUIREMENTS.md`: UI-02 criterion 7's evidence line was orphaned into the UI-03 section —
   FIXED at verify.** The criterion was ticked with **no evidence line**; its evidence had been
   written after the `### UI-03` heading and read as the lead line of UI-03's list. Read as written,
   UI-02 had six substantiated criteria and one bare tick, and a `pending` 3.3 row appeared to carry
   stray measured evidence — the exact shape of a partial being read as complete at the next
   boundary. Moved under its own criterion and the independent verify grep-diff appended.
   Re-checked programmatically: **UI-02 7 criteria / 7 evidence arrows; UI-03 0 arrows, 4 unchecked.**
2. **`SUMMARY-3.1.md` Gate 11's module enumeration is incomplete** — the unit link line is the three
   named modules **plus `juce_events`** (transitive via `juce_data_structures`). The substantive
   claim holds exactly (no `juce_dsp`, no `juce_gui_extra`); recorded so 3.2 does not read the
   three-module list as the asserted invariant.
3. **Cold-configure warnings unchanged** — 0 compiler diagnostics across 128 steps; the repo-wide
   `JUCE_BUNDLE_ID` CMake messages are not O-Octagon's. Re-recorded so it is not read as new.

### Method note — one criterion verify could NOT re-measure

**UI-02 criterion 6 is a historical claim.** "Rendered against the stub *before* C++ integration"
rests on `ui_layout_check.js` §0's recorded run at `2026-08-12T14:22:05Z`, taken when
`Source/PluginEditor.cpp` did not exist. That file exists now, so verify **cannot re-create the
pre-integration state without deleting delivered work**. Verify re-ran the gate post-integration
(10/10) and confirmed §0 still cross-checks `setSize`. Stated as an execute-phase record rather than
presented as a verify-phase re-measurement.

## Stage 3 Phase 3.1 Execute Results (2026-08-12) — `stages/3-gui/SUMMARY-3.1.md`

**Task ordering held, and it is the load-bearing part.** `ui_layout_check.js` passed **10/10 at
`2026-08-12T14:22:05Z`, against the ui-stub, while `Source/PluginEditor.cpp` did not exist** — the
gate recorded the pre-integration run itself. That ordering *is* UI-02 criterion 6, and it was the
only protection D4 left standing against a top-level TDZ throw first surfacing inside a plugin.

| Gate | Result |
|---|---|
| Clean 3-format build + both test targets, forced full recompile | **zero `warning:` / `error:` / `FAILED`** |
| `ui_frontend_check.js` / `ui_layout_check.js` | 20 + 10 sections, exit 0; the Playwright gate **did not skip** |
| `auval -v aufx OuOc OuDv` | **AU VALIDATION SUCCEEDED** |
| pluginval s10 — VST3 ×3, AU ×3 | **6/6 exit 0**, zero `FAILED` |
| C++ probes | **65, 0 failures** (BK hull classes `1:V 2:V 3:E 4:V 5:V 6:V 7:V 8:E`; BL bbox moved + gen 3→4; BM `mono:SAFE stereo:SAFE 7.1:REAL 7.1-SDDS:REAL 5.1.2:REAL`) |
| `gen_dbap_reference.py --check` | 102 cases OK |
| 17 params vs `parameter-spec.md` | **17/17**, three sides, none hand-transcribed |
| Contract checksums | all four byte-exact, **no pin moved** |
| **Gate 13 — Standalone, macOS** | **DISCHARGED with real WKWebView evidence**: window **1102 × 778**, plan drew, hull hexagon with 3/8 dashed, 8 weights at **1.00**, footer `SOURCE 6.50 × 12.00 m · ENVELOPE 15.60 × 19.50 m`, **SAFE banner live**. NOT D5 |

**Measured, not asserted:** the plan box rendered at **448.0 × 560.0 px** — RESEARCH §3.1's predicted
figure to the pixel, and a *consequence* of a measured stage rect rather than an input.
`scrollWidth 1100 ≤ 1100`, `scrollHeight 720 ≤ 720`, on **both** screens.

**Two declarations 3.2 must inherit** (or it rediscovers them):

1. **UI-02/5's end-to-end half is a 3.2 gate** — type a coordinate on the Venue screen, watch the
   Room readout move. 3.1 closed the criterion on P45's (a)+(b)+(c).
2. **N2 — never route session state through `OuariconPresetManager::setStateFromXml`.** It calls
   `replaceState()` and nothing else, which would bypass §4.1's `readVenueFromState()` →
   `rebuildChannelMap()` ordering and leave geometry, hull and map describing the **previous** venue.
   Silent, and it passes every existing probe. **Adopt the module for presets only.**

**Eight negative controls, eight fired** — both new gate files are demonstrably non-vacuous:
dropping `srcY`'s bracket, a literal bbox bound, a drifted stub default, a missing `pointercancel`
close, absolute cursor tracking, a surplus-carrying accumulator, a dropped DPR backing store, and a
hardcoded plan aspect. Every UI-02 criterion resting on the Playwright gate has a demonstrated
failure mode.

**Residuals unchanged:** D5/QUAL-01's audible clause · the CI gap, which 3.1 **widens** ·
`COMPAT-02` · `COMPAT-04` · UI-04/UI-05's descope decision (3.3) · D7's legibility cost, now
**measured** at 448 px wide and routed to 3.3 discuss.

## Stage 3 Phase 3.1 Plan Results (2026-08-12) — `stages/3-gui/PLAN-3.1.md`

**Task ordering is the load-bearing part: Tasks 1–5 complete, and Task 5 PASSES, before Task 6
writes a single line of C++.** That ordering *is* UI-02 criterion 6, and it is the only protection
D4 left standing against a top-level TDZ throw first surfacing inside a plugin.

| P | Decision |
|---|---|
| P37 | `Source/PluginEditor.{h,cpp}`, class `OctagonEditor`, **ONE** relay list — all 17 params are `AudioParameterFloat`, so O-ReverseDelay's three-list type-split failure class is structurally absent. `kSliderIds` built from `oo::params::id(i)`, never hand-written |
| P38 | Native surface is **exactly THREE** — `getParameterDefaults` / `getVenueGeometry` / `getStatus` (2 Hz). `getVenueGeometry` is **one call** because three admit a torn read — the §7.2 hazard P16 fixed on the audio thread. Payload shape fixed at plan; `degenerateX/Y` computed in C++ against `plane::kMinSpan` so JS branches on a flag, not a transcribed threshold |
| **P39** | **N1 — the puck brackets BOTH `srcX` and `srcY`.** Rule generalised so 3.3 inherits it: every pointer interaction opens `sliderDragStarted()` on every parameter it will write and closes all of them — **including on `pointercancel` / `lostpointercapture`**. Invisible to build/auval/pluginval; §12 asserts the pairing statically |
| P40 | Puck renders **optimistically during a drag**, from `getScaledValue()` otherwise, re-syncs on `sliderDragEnded`. **Render on echo; never write on echo** (`ignoreCallbacks` + `jassertfalse` on that path) |
| P41 | The relative-delta accumulator is clamped **at the accumulator**, not only at the write. Clamping only the written value IS the N5 sticky-edge bug. Probe: drag past an edge, reverse one step, assert immediate response |
| P42 | Venue envelope **cached, invalidated by `venueGen`** on the existing poll. No `getNativeFunction` in a `pointermove` — it is an async round trip whose promises resolve out of order, so a naive readout shows a **stale** metre while the puck is current |
| P43 | `std::atomic<bool> safeMode` written in `prepareToPlay()`, adjacent to the `isBusesLayoutSupported()` rule it mirrors. **The negotiated-set NAME is never an atomic** — a cross-thread `juce::String` is a race; resolved inside the native fn on the message thread |
| P44 | The stub's 17 ranges are **ASSERTED against `createParameterLayout()`** parsed from source — the precedent recorded its own table drifting five times. Two neutral-default traps pinned: **`w1..w8` default 1.0**, `blur` 0.10, `airAmount` 0.35 |
| P45 | UI-02/5 closes on **(a)** stub-mutation + **(b)** probe BL + **(c)** a no-bbox-literals static scan. **The end-to-end half is declared NOW as a 3.2 gate** rather than left for 3.2 to discover |
| P46 | Three layers — `<canvas>` backdrop / `<svg>` geometry / DOM controls — **all driven by ONE `metresToPx()`**. An all-canvas plan makes UI-02 criteria 1/2/3 unmeasurable without a debug back-channel: no boxes to measure, no vertices to count |
| P47 | Layout is **plan-left (448 × 560) / controls-right (~580)**. The 1100 × 720 gate **MEASURES** `scrollWidth`/`scrollHeight` on the rendered page — row arithmetic is not the gate (`pattern_flex1_container_slack_invisible_to_row_sum`: 93.5 px of phantom slack through five releases). `tabular-nums` is a gate, not taste |
| P48 | `createEditor`'s two arms **diverge**; the generic editor is not deleted but **demoted to the `#else` arm**, where 29 harness probes need it. The Stage-1 comment is corrected accordingly |
| **P49** | **The Playwright gate FAILS when Playwright is missing — it must NOT SKIP.** Deviation from the precedent, with the reason: there it was supplementary, here it is the **sole** evidence for UI-02 criteria 1/3/4/5. A SKIP that exits 0 is the exact vacuity class this project has caught five times. **Verified at plan: Playwright is NOT installed** — Task 0 |
| P50 | Three new C++ probes **BK / BL / BM**, all render-harness → **65** (33 unit + 32 harness). JS gates are a **separate family counted as sections**, so "62 probes, 0 failures" cannot come to mean two things in two documents |

### New at plan — predicted, not to be discovered at execute

- **Gate 13 is a Standalone launch-and-look, and it is NOT D5.** No Logic, no automation, no
  measurement — ~5 min. It exists because **no automated gate in this repo can prove a WebView loads
  at all**: the stub renders in Chromium, not WKWebView, and the blank-WebView class ships green
  through build, `auval` **and** `pluginval`. D4 removed the browser-iteration safety net; this is
  the cheapest replacement. D5 remains folded into the Stage 4 hall session, untouched.
- **Two ROADMAP phase criteria are carried by no UI-02 line**, and the plan says so rather than
  letting verify find the gap: the SAFE banner (probe BM + layout §9) and *"every control moves its
  parameter; host automation moves every control"* (layout §10's drive-and-echo + static §16's
  four-way closure, with **the in-host confirmation folded into the Stage 4 session**).
- **`REQUIREMENTS.md` is NOT edited at this boundary** — unlike 2.3 Task 1. UI-02/5 is satisfied as
  *written* by P45's three-part probe; the criterion needed a probe design, not a rewording.
- **Two things must reach `SUMMARY-3.1.md` or 3.2 rediscovers them:** UI-02/5's end-to-end 3.2 gate
  (P45), and **N2** — never route session state through `OuariconPresetManager::setStateFromXml`,
  which calls `replaceState()` and nothing else and would leave geometry, hull and map describing the
  **previous** venue. Silent, and it passes every existing probe.

### Predicted outcome, unchanged from discuss and research

**UI-02 ✅ closes; nothing else. Zero partials declared in advance.** Five residuals stay open:
D5/QUAL-01's audible clause · the CI gap, which 3.1 **widens** (the two new JS gates are also
local-only) · `COMPAT-04` · `COMPAT-02` · UI-04/UI-05's descope decision, which belongs at 3.3.

## Stage 3 Phase 3.1 Research Results (2026-08-12) — `stages/3-gui/RESEARCH-3.1.md`

Three of the seven findings live in JUCE's own source and could not have been known at discuss.

| # | Finding | Consequence |
|---|---|---|
| **N1** | `WebSliderParameterAttachment::sliderValueChanged` calls `setValueAsPartOfGesture` — **`setValueNotifyingHost` with NO gesture brackets** (`juce_ParameterAttachments.cpp:324, 76`) | **A 3.1 change, not a 3.3 one.** The puck drag must call `sliderDragStarted`/`Ended` on **both** `srcX` and `srcY`; without it Logic's Touch/Latch may move the sound and not record it. Scenes need the same at 3.3, so **§6.3's stated mechanism is incomplete** |
| **F3** | `ParameterAttachment::callIfParameterValueChanged` **skips a write whose value is unchanged** | From defaults (`w1..w8 = 1.0`), clicking `ALL` emits **zero** host events. A "count 8 events" probe fails on correct code — FUNC-06/1's *"read back all 8 values"* wording is now load-bearing. **No REQUIREMENTS edit needed** |
| **§3.1** | Derived envelope is **15.60 × 19.50 m, aspect 0.800 — portrait**. Inside 1100 × 720 the plan is **~448 × 560 px, height-bound** | Layout is **plan-left / controls-right**, not a wide plan with controls beneath. D7 survives, but its stated legibility cost is sharper: the plan is 448 px wide, and widening the window would not help |
| **Q4** | `getNativeFunction` is an **async promise round trip** whose responses can resolve out of order | Do **not** call it per `pointermove` — the readout can show a stale metre while the puck is current. Pull the envelope once, cache it, refresh on `venueGen`. Still honours `pattern_webview_knob_readout_scaled_value`: the numbers are the plugin's, fetched from the plugin |
| **N2** | `OuariconPresetManager::setStateFromXml` calls `replaceState()` **and nothing else** | Would bypass §4.1's `readVenueFromState()` → `rebuildChannelMap()` ordering, leaving geometry, hull and map describing the **previous** venue — silent, and it passes every existing probe. **3.2: adopt the module for presets only** |
| **N3** | D5's `SIDES` — *"hull speakers off both axes"* — **is not derivable as written** | Predicate proposed and measured: normalised lateral dominance ∧ `classify != INTERIOR` → `{3,4,7,8}` on the default venue. **Speakers 1 and 2 miss it by 6 %** — concrete evidence that FUNC-06/3's show-before-commit earns its cost. 3.3 work |
| **F1** | All 17 parameters are `AudioParameterFloat` — **one relay list, no Choice, no Bool** | The relay-type split that bit O-ReverseDelay three times (`grainShape`, `freeze`, `sourceMode`) is **structurally absent here**. Plan should not budget for it |

**Answered with existing machinery, no new state:** `getAPVTS()` serves the relays as-is (Q6);
`SCENES` needs **no** `AsyncUpdater` and **no** `cancelPendingUpdate()` — the processor's plain
`preparedYet` flag means there is no queued apply to cancel, and adding one would *create* the
obligation the design has avoided (Q1); the venue-generation counter the solver's dirty check already
reads is exactly the UI's change signal (Q4/Q9). Only two additions: `std::atomic<bool> safeMode`
written in `prepareToPlay()`, and a `getVenueGeneration()` accessor.

**Native-function surface at 3.1 is exactly THREE** — `getParameterDefaults`, `getVenueGeometry`
(one call, so the plan cannot composite an envelope from venue A with glyphs from venue B — the same
torn-read hazard §7.2 guards on the audio thread), `getStatus` (polled ~2 Hz).

**UI-02/5 cannot be tested as literally written at 3.1** — it implies a venue-coordinate edit, and the
Venue screen is 3.2. Closed by a three-part probe (stub-mutation in Playwright + an `applyVenueEdit`
probe in the render harness + a no-bbox-literals static scan); **the end-to-end half is declared now
as a 3.2 gate** rather than left for 3.2 to discover.

**ui-stub verdict: the O-ReverseDelay precedent is sufficient, and O-Octagon's stub is SMALLER** — 17
ranges vs 20, 3 native fns vs 13, zero combos, zero toggles, no `backend.addEventListener` (the design
is pull-only). Two neutral-default traps to respect: **`w1..w8` default to 1.0, not the range
minimum**, and `blur` 0.10 / `airAmount` 0.35.

## Stage 3 Phase 3.1 Discuss Results (2026-08-12) — `stages/3-gui/CONTEXT-3.1.md`

Stage 3 verifies **FUNC-02, FUNC-04, FUNC-05, FUNC-06, UI-01..05** across three phases:
**3.1** → UI-02 · **3.2** → FUNC-02/04/05 + UI-01 · **3.3** → FUNC-06 + UI-03/04/05.

Nothing in Stage 3 is arithmetically hard; **all of it is silent when it breaks.** §R7 names it the
largest UI in the repo. The repo's UI scar tissue — TDZ throws, bridge gaps, the canvas replaced-element
trap, state updaters erasing labels — is now written as acceptance criteria rather than hoped for.

| # | Decision | Choice |
|---|----------|--------|
| D1 | Stage 3 cycle structure | **Three full discuss→research→plan→execute→verify cycles.** Stage 2's per-phase cycle earned itself back repeatedly (2.3 alone: 8 negative controls, **4 corrected an attribution**). Concrete win: 3.1's grep-diff bridge gate runs against **17** bindings, not 70 |
| D2 | D5 manual Logic gate | **Folded into the Stage 4 hall session** — see the risk callout below |
| D3 | Visual aesthetic | **Ouaricon Naturalist, darkened — split: brand owns the chrome, function owns the data.** Serif + botanical watermark in the frame; **mono tabular numerals** for every metre value and level. Tabular figures are load-bearing, not taste: a proportional serif cannot column-align UI-01's 24-field coordinate grid, and a mis-scanned metre propagates silently into the solve. The level ramp is the interface's only hue |
| D4 | Design workflow | **Designed inside 3.1 execute** by `gui-agent`; no separate `/ui-mockup` pass. **Consequence: the ui-stub becomes mandatory, not advisory** — D4 removes the browser-reload safety net, so a top-level TDZ throw would first surface inside a plugin |
| D5 | FUNC-06 named-scene semantics | **Derived from measured geometry, and shown on the plan before commit.** FRONT/REAR split on centroid y, LEFT/RIGHT on x, SIDES = hull speakers off both axes |
| D6 | FUNC-06 user-slot persistence | **A `SCENES` child of `apvts.state`, SIBLING of `VENUE`**, carried by musical presets — scenes are pure weight data and venue-portable |
| D7 | Editor sizing | **Fixed 1100 × 720, non-resizable.** Cheapest answer to §R7 — reflow is where a UI this size breaks |

### D2 — what folding D5 to Stage 4 actually costs, named rather than hidden

D5's **only unique coverage is QUAL-01 criterion 2's *audible* clause.** QUAL-01 is already ✅ complete:
measurement bounded the D2 step to **~15 % of an 8 kHz component as a one-sample step** and matched its
own prediction to 0.000 % — but measurement cannot conclude audibility. **So QUAL-01 carries an
unverified clause through the whole of Stage 3.**

> ⚠️ **The accepted risk, stated once so Stage 4 does not rediscover it:** if the hull crossing ticks
> audibly with HF-rich material, the fix is **RESEARCH-2.3's H3 lever — `fc(d_hull = 0)` toward
> Nyquist** — which re-tunes the Stage-2 musical curve **after Stage 3 is built and verified.** It is
> bounded (it touches `hullproc::airCutoffHz()` and DSP-07's curve criteria, not the UI), and Stage 4
> is where the material and the room are real rather than simulated at a desk. **No Stage-3
> verification should be read as having settled it.**

### Two contract GAPS found — absences, not errors, so neither triggers a re-pin

1. **FUNC-06's named-scene semantics were undefined.** `BRIEF.md` and `ARCHITECTURE.md` §6.3 both list
   the six names and specify only the *write mechanism*. **Neither says what `FRONT` resolves to.**
   Resolved by D5. The alternative — fixed `FRONT = {1,2}` — is trivially testable and **silently
   wrong the moment a hall is measured with different numbering**: the R1 failure class
   (`critical_audiochannelset_is_a_bitset_not_an_order`) reproduced one layer up, in a plugin whose
   whole premise is that the room is measured rather than assumed.
2. **The 4 user slots had no storage location** — §4.1's tree diagram has exactly two children.
   Resolved by D6.

Research decides whether §6.3 / §4.1 must *state* these; that would be a re-pin **at a boundary**.

### Five of the new criteria are non-obvious and deliberate

1. **FUNC-06/2 requires a PERMUTED-NUMBERING probe.** Against the traced layout speakers 1,2 *happen*
   to be front, so derived **and** hardcoded both pass — the probe re-measures with permuted numbering
   and **a fixed-index implementation must FAIL**. Probe-O discipline from 2.1.
2. **FUNC-06/5 re-runs FUNC-05's bit-compare after `SCENES` exists.** §4.1's "a preset physically
   cannot reach VENUE" was argued against a **two**-node tree; a third node does not inherit it.
3. **UI-02/5 is a non-vacuity gate.** "Metres via `getNativeFunction`" is unfalsifiable — a JS min/max
   map produces metres too. The criterion: **puck stationary, edit a venue coordinate, readout must
   change** (`pattern_webview_knob_readout_scaled_value`).
4. **UI-03/2 cross-checks against verify-ping, never `v_i`.** Metering the solve lights the correct
   speaker **even under a bypassed channel map** — literally 2.2's NC3. §4.3 meters the written buffer
   for exactly this reason.
5. **UI-04/2 asserts a recompute COUNTER**, not smoothness: the field depends on speakers and weights,
   never source position, so N frames of puck drag must leave the count unchanged.

### Predicted outcomes, declared at discuss

**3.1 closes UI-02 ✅ and nothing else. Zero partials declared in advance** — if a UI-02 criterion
cannot be met it will be declared at the 3.1 verify boundary **with a named destination**, the
discipline that produced zero verify-time surprises across all three Stage-2 phases.

**Five residuals Stage 3 does not close:** D5/QUAL-01's audible clause (Stage 4) · the CI gap, which
Stage 3 will *widen* since UI probes are also local-only (Stage 4) · `COMPAT-04` · `COMPAT-02` ·
**UI-04/UI-05 descope to v1.1 — deliberately NOT decided here**, it belongs at 3.3 discuss when real
schedule information exists. **UI-03 is `should` and is NOT descopable** — §R7 names it a defence on R1.

### Ten open questions handed to research

Whether an 8-parameter scene write **coalesces** in Logic (FUNC-06/1 and /6 both rest on it; 2.2's Q1
proved the call synchronous but said nothing about host-side recording) · where reading `SCENES` lands
in §4.1's `replaceState` → `readVenueFromState` → `rebuildChannelMap` order, and whether it needs
`cancelPendingUpdate()` · geometry-derived scene sets on 2.1's degenerate venues, and whether "empty"
is the right answer · `getNativeFunction` cost per mousemove for UI-02/5's live metres resolve ·
whether the UI thread's meter zeroing races the audio thread's max-store · whether the unplanned
`getAPVTS()` accessor serves the WebView relays · **whether D7's fixed 1100 × 720 actually fits the
42-field table** at the type size legibility demands · whether `O-ReverseDelay/tests/ui-stub/` is rich
enough for a canvas-heavy two-screen UI · which atomic reports SAFE mode · whether the darkened-
Naturalist split should be saved back as a **new aesthetic template** for future performance plugins.

---

## Stage 2 — CLOSED

Stage: **2 of 4 (DSP) — ✅ COMPLETE.** All three phases closed ✅ VERIFIED.
Status: **Stage 2 closes.** `stages/2-dsp/VERIFICATION.md` (stage-level) + `VERIFICATION-2.3.md`.
**The chain closes: all seven of §5's steps are live.** `width` reaches the shaper, `solveSubPoint`
returns the `d_hull` it used to discard, the hull trim and air LPF consume it, and the eight venue
trims carried since 2.1 are multiplied in. **All eleven gates RE-RUN at verify against a forced full
recompile — all pass. 62 probes, 0 failures** (33 unit + 29 harness); clean 3-format build, **zero
compiler diagnostics**; `auval` SUCCEEDED; pluginval s10 ×3 VST3 + ×3 AU all exit 0. All three
`PHASE-2.3-*` markers read **ZERO in `Source/` AND `tests/`**. **Eight negative controls run as new
work at verify** — every load-bearing claim fails loudly when broken; tree proved byte-identical
afterwards and the restored render output is byte-identical to the pre-NC run.
**All 18 Stage-2 requirement rows close ✅ complete. Zero partial, zero failed.** Every partial in
this stage was **declared at a discuss boundary and closed at its named destination** — none was
discovered at verify. **No contract pin moved at 2.3** (P36).
Progress: `[####################]` 100% of Stage 2 — **closed 2026-08-11**
Branch: `feat/o-octagon` (cut from `docs/logic-multichannel-dbap-research` @ 12ae50dd)

> ⚠️ **`contract_checksums.architecture` has now been re-pinned TWICE, once per phase boundary.**
> 2.2 discuss (D2 — `rigScale` 7.95 → 7.93165 m) gave `cd881a10…`; **2.3 discuss (D2 — §3.5.2's air
> skip condition → `airAmount · d_hull == 0`) gives `a8a358f4…`**. Both superseded values are in
> frontmatter. Artifacts dated 2.1 verify against `bff8a83b…`, 2.2 against `cd881a10…`, 2.3 onward
> against `a8a358f4…` — each was correct when written. `BRIEF.md`, `parameter-spec.md` and
> `ROADMAP.md` are **unmodified across all three phases** — their pins have never changed.

## Stage 2 Cycle Structure (decided at 2.1 discuss)

**One full discuss→research→plan→execute→verify cycle per phase**, not one pass over the stage. The
channel map is R1 (CRITICAL, silent failure, audible only in the hall) and gets its own verify
before any gain math exists to confuse a diagnosis.

| Phase | Name | Verifies | Status |
|-------|------|----------|--------|
| 2.1 | Geometry Core — `VenueModel`, `ConvexHull2D`, `ChannelMap`, `VenueSnapshot`, tests | COMPAT-03 ✅, DSP-03 ✅, DSP-04 ⚠️, FUNC-03 ⚠️ | **✅ VERIFIED** |
| 2.2 | DBAP Solve + Gain Application | FUNC-01 ✅, DSP-01 ✅, DSP-02 ✅, DSP-05 ✅, PERF-01 ✅, PERF-02 ✅, QUAL-02 ✅, QUAL-03 ✅, QUAL-04 ⚠️, DSP-04 ✅, FUNC-03 ✅ | **✅ VERIFIED** |
| 2.3 | Source Shaping + Outside-Hull | FUNC-07 ✅, DSP-06 ✅, DSP-07 ✅, DSP-08 ✅, QUAL-01 ✅, QUAL-04/3 ✅ | **✅ VERIFIED** |

Artifacts live in `stages/2-dsp/` with a phase suffix — `CONTEXT-2.1.md`, `RESEARCH-2.1.md`,
`PLAN-2.1.md`, `SUMMARY-2.1.md`, `VERIFICATION-2.1.md` — so the stage-level commands still resolve
`2-dsp`. A stage-level `VERIFICATION.md` is written only at the close of 2.3.

## Stage 2 Phase 2.3 Discuss Results (2026-08-11) — `stages/2-dsp/CONTEXT-2.3.md`

Contract checksums re-verified at the boundary — **all four byte-exact on arrival**, then
`ARCHITECTURE.md` deliberately amended and re-pinned by D2 (see the callout above).

**2.3 is small in code and sharp in measurement.** 2.2 shipped the general path in its degenerate
configuration, so the functional diff is three marked lines plus a filter pair:
`GainStage.cpp:147` (`PHASE-2.3-WIDTH` → read `p[params::width]`), `:174` (`PHASE-2.3-AIR` → consume
the `d_hull` that `solveSubPoint` already computes and discards), `:182` (`PHASE-2.3-TRIM` →
`* snapshot.trimLin[i]`). All three must read **zero** at 2.3. `shaper::shape()` is already complete
and already exercised at `width > 0` by probe AH.

| # | Decision | Choice |
|---|----------|--------|
| D1 | `HullProcessor` file shape | **JUCE-free free functions `hullproc::hullTrimGain()` / `airCutoffHz()`; the two `FirstOrderTPTFilter`s live in `GainStage`.** The `hull::`/`dbap::`/`shaper::` precedent — keeps DSP-07's curve gate in the **fast** unit target instead of dragging `juce_dsp` across the narrow link line |
| D2 | `ARCHITECTURE §3.5.2` air-filter skip | **Amend to `airAmount · d_hull == 0` and re-pin.** §3.5.2 already argues 20 kHz is not a transparent bypass — it applied that argument to only one of the two axes that zero the effect. `d_hull = 0` inside the hull is the common case AND the default patch |
| D3 | How QUAL-01 measures `airAmount` | **Differential 1 kHz sine probe vs. a held-parameter reference, with a negative control.** QUAL-04's DC method is *exactly blind*: a one-pole passes DC unchanged, so an air sweep moves a DC output by literally zero |
| D4 | QUAL-01 probe scope | **17 musical params + hull crossing + a FUNC-07 trim step + one live venue edit during playback.** Only APVTS params ride an automation lane; venue edits all land on the same smoothed targets |
| D5 | Manual Logic gate | **One combined ~15 min session at 2.3 verify** — 2.2's carried Task 12 (a, b) plus 2.3's width / air / trim observations |

### D2 — what was traded for what, named rather than hidden

**Gained:** the shipping default patch (`airAmount = 0.35`, source inside the hull) becomes
bit-transparent. Under the original wording the plugin was transparent only at `airAmount = 0` —
never on the default — because the TPT one-pole's zero at Nyquist puts ≈ −3 dB @ 20 kHz and
≈ −0.7 dB @ 10 kHz there **with no distance to justify it**. The stage is also free inside the hull.

**Paid:** crossing the hull boundary now **steps** the transfer function instead of sliding it —
bounded at 3 dB @ 20 kHz, 0.7 dB @ 10 kHz, and **0 dB at DC**. That last figure is the load-bearing
one: the single discontinuity this amendment introduces is **invisible to the DC method 2.2
established**, so QUAL-01 criterion 2 is measured on both excitations. `reset()` fires only on the
`airAmount → 0` transition, never on a `d_hull == 0` block — resetting on every skip would let a puck
oscillating across the hull edge re-zero the filter repeatedly.

### The criteria debt owed at this boundary is CLEARED

STATUS carried *"Criteria owed at 2.3 discuss, before 2.3 plan — FUNC-07, DSP-06, DSP-07, DSP-08."*
All four had a summary row and **no criteria at all**. Written now, before plan: FUNC-07 (4),
DSP-06 (5), DSP-07 (8, under §3.5.2 as amended), DSP-08 (4), plus a scope-and-method note under
QUAL-01 recording D3 and D4. Re-checked programmatically: **30 summary rows, 24 sections, zero
sections without a row, zero duplicate headings.** The six rows still lacking criteria are exactly the
known dated debt — `FUNC-06` + `UI-02..05` (Stage 3 discuss) and `COMPAT-04` (retroactive, Stage 4).
No new gap; the `### DSP-04` duplicate repaired at 2.2 stays repaired.

**Two criteria are non-obvious and deliberate:** DSP-08's room-size independence is asserted as a
**scaling invariant, not a constant** (a bare `rigScale ≈ 7.93` probe is a mirrored fixture — and this
is the exact number the architecture got wrong twice); DSP-06 criterion 1 is a **regression** gate —
`width = 0` must stay bit-identical to 2.2's shipping behaviour once `width` goes live.

### Predicted outcomes, declared at discuss

FUNC-07 ✅ · DSP-06 ✅ · DSP-07 ✅ *(under §3.5.2 as amended)* · DSP-08 ✅ *(implemented at 2.2, ticks
here)* · QUAL-01 ✅ *(under the D3/D4 scope)* · **QUAL-04 ✅ completes**, clearing 2.2's partial.
Stage 2 then closes with a stage-level `VERIFICATION.md`. Three residuals do not close at 2.3: the CI
gap, `COMPAT-04`'s retroactive criteria, and `FUNC-06`/`UI-02..05`'s criteria.

### Ten open questions handed to research

`FirstOrderTPTFilter` state inspection when `s1` is private (is an output check equivalent?);
`setCutoffFrequency`'s `std::tan` cost and whether it needs a counter; whether it perturbs state —
which the D2 "skip without reset" design depends on; filter `prepare()` vs. P23's *one reset site,
ever*; whether anything on the path breaks the bit-transparency claim, and whether
`dbToGain(-0.0f) == 1.0f` exactly on this toolchain; a **derived, not tuned** tolerance for the
differential sine probe, tight enough that its negative control fails; whether 1 kHz is even
sensitive enough to *see* the D2 step or criterion 2 needs 8 kHz; whether `trimLin` is populated today
or FUNC-07 needs plumbing as well as a multiply; the straddling case where one sub-point is inside and
one outside, so one filter is active and one skipped; and whether probe AQ's live-edit rig can carry
a continuity measurement.

## Stage 2 Phase 2.3 Research Results (2026-08-11) — `stages/2-dsp/RESEARCH-2.3.md`

Contract checksums re-verified at the boundary — **all four byte-exact**, including the `a8a358f4…`
re-pin issued at 2.3 discuss. Marker state verified: each `PHASE-2.3-*` token appears **exactly
once** in `Source/`. Probe inventory verified: **46 distinct probes, `A`–`AT`, no gaps.**

**All ten questions answered.** The numeric work is committed and re-runnable —
`tests/tools/air_filter_study.py`, no arguments, prints every table in the research doc. It is a
research artefact, not a build step and not a test.

### Two HIGH findings — neither was among the ten questions

- **H1 — D2's "leave the filter state resident" is the *worst* of the three re-entry policies.** A
  source that ducks inside the hull for one control block and comes back out produces a first-sample
  click of **520% of the signal's own maximum per-sample slew** at 1 kHz. `reset(0)` (the original
  §3.5.2) gives 430%. **Seeding `s = x` on the false→true edge gives 0, bit-exactly** — the code
  computes `v = G·(x − s) = G·0.0f = 0.0f`, so `y = 0.0f + x` on every toolchain, at every cutoff, at
  every entry speed. One float store. It satisfies **D2's own rationale better than D2's own choice
  does**: D2 objected to *re-zeroing*, and re-seeding discards nothing. It also closes DSP-07's
  skipped-filter NaN hole for free.
- **H5 — FUNC-07's multiply arms the H2 permanent NaN latch.** The 17 musical parameters are
  `isfinite`-sanitised at ingestion; **the 42 venue values are not** — `readFloat` has no clamp, no
  `jlimit`, no `isfinite` anywhere in `VenueModel.cpp`. Today `trimLin` is carried and never used, so
  it is latent. Once `* snapshot.trimLin[i]` lands: `trimDb = 1e30` → `trimLin = inf`, and
  `v_i = 0.0f` (exactly, whenever `w_i == 0` per DSP-05/1) → `0.0f * inf = NaN` →
  `setTargetValue(NaN)` → permanent silence. Sanitise at `publishSnapshot()`, the single funnel —
  the exact analogue of `snapshotParameters()`.

### D2's accepted cost was mis-stated, in the conservative direction (H2)

The quoted "**0.7 dB @ 10 kHz**" is the **analog** one-pole at `f/fc = 0.5`. The digital TPT filter
at `fc = 20 kHz` is far flatter — **−0.18 dB @ 48 kHz, −0.07 dB @ 44.1 kHz** — because 20 kHz sits at
0.83 × Nyquist and bilinear prewarping compresses the passband. The 3 dB @ 20 kHz figure is right by
definition.

**But the magnitude figures are the wrong quantity.** The crossing step is the complex `H(f) − 1`,
and the **phase lag dominates everywhere below ~15 kHz** — by **114×** at 1 kHz (48 kHz) and **190×**
at 44.1 kHz. The real accepted cost is **1.8% of a 1 kHz component and 15% of an 8 kHz component**,
as a one-sample step. Bounded, one sample, only on a deliberate gesture — but it is the number
QUAL-01 criterion 2 must be measured against, and **"0 dB at DC" remains true**, so D3's core insight
survives untouched. Recommended as an **erratum in `SUMMARY-2.3.md`, not a mid-phase re-pin**.

### D3's method as written is numerically degenerate (Q6/Q7)

Simulated: `max |out[n] − out[n−1]|` under a full-speed `airAmount` sweep vs. held reads **+0.00000%
excess at 1 kHz** — the coefficient step is 3–4 orders below the sine's own slew at every tone. The
measurable quantity is `Δy = (G_new − G_old)·(x − s)`, whose bound the probe **derives from the sweep
it already drives** (nothing tuned, nothing mirrored) and whose 4096-sample negative control
separates by **~48×**. Q7 flips to "1 kHz is fine, but not for D3's reason" — the phase term, not the
magnitude tilt, is what makes it visible.

### Answered, no work needed

`juce_dsp` **is already linked** in both the plugin and render-harness targets — **`CONTEXT-2.3.md`
constraint 9 is wrong**; what is missing is the `#include` in `GainStage.h`, and that is safe because
`tests/unit/main.cpp` does not include `GainStage.h` (verified). **`trimLin` plumbing is complete** —
FUNC-07 really is one multiply. **Straddling sub-points are strictly less severe** than a coincident
crossing (half the step), and the `wEff` collapse cannot reach the hull edge on any non-collinear rig
(the floor-projected centroid is a convex combination, so it is interior by definition).
**Live-edit effects are always control-grid-aligned** — the measurement window just has to allow the
≤63-sample lag. **§3.5.2's four-row `fc` table re-derives exactly.**

### Three mandatory fixes if the JUCE filter class is kept

**H4** — the literal `20000` ceiling trips `jassert(isPositiveAndBelow(fc, fs*0.5))` at 32 kHz and
below, and past Nyquist `tan()` goes negative so `G < 0` and the one-pole is not a lowpass at all.
Ceiling must be **`min(20000, 0.45·fs)`**. **H6** — `std::vector<SampleType> s1 { 2 }` is the
*initializer-list* constructor: the default state is **one element holding 2.0f**, so `prepare()` is
mandatory and `processSample(1, …)` is OOB without it; and `G` is **per-filter, not per-channel**, so
one 2-channel instance cannot carry two cutoffs — **two mono instances is required, not stylistic**.
**H9** — every filter-path probe must discard a **≥2000-sample lead-in**, or it measures the filter's
cold start rather than the automation (this produced a 3% over-reading during the research itself).

### Handed to plan as decisions, each with a recommendation

Adopt `reset(x)` (yes) · sanitise venue values (yes for `trimDb`, in scope; yes-with-deviation for
positions, pre-existing) · Nyquist-safe ceiling (yes) · H2 as erratum vs. re-pin (erratum) ·
**`hullproc::OnePoleTPT` vs. `juce::dsp::FirstOrderTPTFilter` — no recommendation, both defensible.**
A five-line own-struct would be JUCE-free, allocation-free, **state-inspectable** (closing Q1
directly rather than by equivalence), free of H4's `jassert` and H6's quirk, and would move every
DSP-07 filter criterion into the **fast** unit target — against which it deviates from a named
architecture choice. **DSP-08's probe must use `blur = 0.25`**, not 1.0: at λ = 2 the default rig
lands at 7.932 m against the 8 m clamp — passing today **by 0.9%**, and `rigScale` has already moved
twice.

## Stage 2 Phase 2.3 Plan Results (2026-08-11) — `stages/2-dsp/PLAN-2.3.md`

Contract checksums re-computed at this boundary — **all four byte-exact, and no pin moves at 2.3**
(P36). Markers verified: each `PHASE-2.3-*` token appears **exactly once** in `Source/`;
`PHASE-2.2-REPLACE` is 0. Probe inventory reconciled: **46 (A–AT)**, 29 unit + 17 harness.

**11 tasks, 12 plan decisions (P25–P36), 16 new probes (AU–BJ) → 62 total.**

| # | Decision | Choice |
|---|----------|--------|
| P25 | `hullproc` file shape | **Header-only `Source/DSP/HullProcessor.h`**, `inline` free functions — the `VenueGeometry.h` (P14) precedent; **zero CMake churn**. D1's real constraint is "no `juce_dsp`/`juce_audio_processors`", which holds: `juce::Decibels` comes from `juce_audio_basics`, which the fast target already links, and is the *same* function `VenueModel::trimLin` uses. Deviation from ROADMAP's `.{h,cpp}` |
| **P26** | **`OnePoleTPT` vs. the JUCE class — the open call** | **Keep `juce::dsp::FirstOrderTPTFilter<float>`.** Its rival's one unique advantage (state inspection) is **already neutralised by Q1**, which proved the output check equivalent *and immediate*; the "move DSP-07 to the fast target" win is small because criteria 5–8 are about the filter **as `GainStage` drives it**, and `GainStage.cpp` is harness-only by construction. §3.5.2 names the class. **Consequence: H4 and H6 become mandatory, not optional** |
| P27 | H1's `reset(x)` at the air edge | **Adopt.** 520.9 % → **0.0 %** of the signal's own max slew, bit-exact by the arithmetic (`v = G·(x−x) = 0`). Satisfies D2's own rationale better than D2's choice, and closes the skipped-filter NaN hole free. Seed **hoisted** out of the per-sample loop; flag consumed in REAL mode only, left pending in SAFE (H10) |
| P28 | Cutoff bounds + `ProcessSpec` | `ceiling = min(20000, 0.45·fs)`, `floor = min(500, ceiling)`. `GainStage` stores `sampleRate`; **`prepareToPlay` re-instates its `samplesPerBlock` parameter** and forwards a real spec rather than fabricating one |
| P29 | H5 sanitisation site | **`publishSnapshot()`** — the single funnel, the message-thread analogue of P17. `trimDb` clamped ±24 dB (in scope: the multiply arms it); every other float `isfinite`-guarded to its §OQ4 default (**recorded deviation** — pre-existing 2.2 hazard, same line) |
| P30 | Filter `prepare()` placement | **Step 2** of `GainStage::prepare`, extending P23 to "step 2 is the only place any DSP state is initialised, ever". **Two mono instances is mandatory** — `G` is per-filter, not per-channel (H6) |
| P31 | DSP-07 NaN guard | Against the **output**, once per block, `reset()` on failure. Comment must record that `pattern_biquad_nan_guard_sticky_silence` does **not** apply verbatim (`G` is recomputed every control block), or someone adds coefficient preservation that does nothing |
| P32 | Two counters | `airCutoffUpdates` (`== solveRuns·2` — asserts **placement**, not cost) and `airSamplesFiltered` (makes P33 measurable). **Neither routes through `countedPow`**; `powCalls == 16` must stay exact |
| P33 | DSP-07/2 and /5 method | **Structural**, not against a fictitious "filter absent" build — a counted never-taken branch and a multiply asserted `bitExact(…, 1.0f)`. Each half carries a **non-vacuity control** proving the stage changes the output when enabled. Denormal precondition stated in the probe comment |
| P34 | QUAL-01's bounds | `airAmount`: **two-render differential** (64-grid vs 4096-grid), bound `max\|ΔG\|·2·peak` computed **in-probe**, negative control separating ~48×. Criterion 2: bound `A·\|H_20k(f) − 1\|` **predicted and asserted** (1.756e-2 @ 1 kHz, 1.529e-1 @ 8 kHz), entry edge **bit-exact**. Every filter-path probe discards ≥2000 samples |
| P35 | DSP-08 probe | λ ∈ {0.5, 2.0} at **`blur = 0.25`** (4× margin), scaling the **source** as well as the speakers, plus a **positive control** at `blur = 1, λ = 2.1` proving the probe can see the clamp |
| P36 | H2's figure correction | **Erratum, not a third re-pin.** `ARCHITECTURE.md` is untouched. `REQUIREMENTS.md` is corrected **at plan** (Task 1) because its QUAL-01 note prescribes a method Q6 proved vacuous — correcting a *method* is not ticking a *verdict* |

### New at plan — predicted, not to be discovered at execute

- **Three recorded deviations** are already named: P27 (`reset(x)`, beyond §3.5.2's two policies),
  P29's position guard (a 2.2 hazard folded into a 2.3 site), P25 (header-only vs. ROADMAP's `.cpp`).
- **`CONTEXT-2.3.md` constraint 9 is wrong and the plan says so** — `juce_dsp` is already linked in
  all three places that need it; what Task 5 adds is the `#include` in `GainStage.h`. **Gate 11**
  re-verifies the fast target's link line has not acquired `juce_dsp` as a side effect.
- **QUAL-01/1's 17-parameter coverage is arithmetic, not assertion:** AS (11) + AZ (5) + BC (1) = 17,
  stated in AZ's detail string.
- **Q9 is answered by argument, not by a probe** — straddling is provably half the coincident step and
  the `wEff` collapse disc is interior to the hull. Recorded in BB's comment so verify does not
  rediscover it as a gap.
- **DSP-07/2 and /5 will NOT read "bit-transparency ✓" in SUMMARY** — P33's method is reported as a
  method, in the PERF-01/H8 tradition from 2.2.
- **D5's session gains an H2 item:** cross the hull with **HF-rich** material. The accepted cost of D2
  is ~15 % of an 8 kHz component as a one-sample step; measurement bounds it, only listening settles
  it. If it ticks audibly that is a **Stage-3 discuss** finding (H3's lever re-tunes the whole musical
  curve), not a plan-phase change.

## Stage 2 Phase 2.3 Verify Results (2026-08-11) — `VERIFICATION-2.3.md` + `VERIFICATION.md`

**Verdict: ✅ VERIFIED. Stage 2 closes. Ready for Stage 3: yes, no blockers.**

Every gate **re-run from scratch at verify** on a forced full recompile
(`rm -rf build/plugins/O-Octagon`, 119 steps), not read out of `SUMMARY-2.3.md`. All eleven passed;
62 probes / 0 failures; none of A–AT regressed. Contract checksums recomputed — **all four
byte-exact and no pin moved** (P36 honoured).

| Gate | Verify-phase result |
|---|---|
| Clean 3-format build + both test targets, forced TU recompile | ✓ exit 0, **zero `warning:` / `error:` / `FAILED`** |
| `PHASE-2.3-WIDTH` / `-AIR` / `-TRIM` / `PHASE-2.2-REPLACE` | ✓ **0 / 0 / 0 / 0** in `Source/` **and** `tests/`, occurrences not lines |
| Hardcoded output indices outside `ChannelMap` | ✓ 2 hits, both INPUT reads bounded by `numIn` |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED** |
| pluginval s10, VST3 ×3 / AU ×3 | ✓ all six exit 0, zero `FAILED` |
| Both test targets | ✓ **33 + 29 = 62 probes, 0 failures**, exit 0 / exit 0 |
| `gen_dbap_reference.py --check` | ✓ exit 0 — 102 cases |
| 17 params vs `parameter-spec.md` | ✓ **17/17 across three sides**, none hand-transcribed |
| `setLatencySamples` / `switch` on ChannelType / `createEditor` guard | ✓ absent / absent / present |
| `OOCTAGON_INSTRUMENT` scoping | ✓ 0 in the plugin target; **six** counters |
| Unit-target link line has no `juce_dsp` | ✓ |

### Eight negative controls — new work at verify, and four of them corrected an attribution

| # | Broken | Result |
|---|---|---|
| NC1 | P27's `reset(x)` entry seed | BB FAIL (`P27's SEED IS GONE`, DC step 0.1367 = 33× bound) — **and AS + AZ FAIL**: 2.2's QUAL-04 probes now depend on a 2.3 mechanism |
| NC2 | P29's `trimDb` sanitisation | BH FAIL, `NON-FINITE`, `clamp: xinf — NOT CLAMPED`. **Only the `+inf` path kills it**; NaN and −1e30 are benign unguarded |
| NC3 | FUNC-07's `* trimLin[i]` multiply | BF FAIL on all four criteria at once |
| NC4 | the `d_hull > 0` half of D2 | BD FAIL (`FILTER RAN INSIDE THE HULL`) — **and Q′ FAIL at 0.319**: D2 is load-bearing for FUNC-01, not only DSP-07/6 |
| NC5 | H4's Nyquist-safe ceiling | AU FAIL (`ceilings WRONG; PAST NYQUIST`) |
| NC6 | `width` back to the 2.2 literal | AY FAIL (`WIDTH IS NOT REACHING THE SHAPER`) — **AZ still PASSES** |
| NC7 | P31's per-block NaN guard | BE FAIL half 1 (`active-filter NaN LATCHED`) |
| NC8 | P31 **and** P27 together | BE half 2 `RE-ENTERED POISONED` — the two mechanisms independently close the same hole |

**Tree proved byte-identical to baseline afterwards** (26 files, `shasum -a 256`), and the restored
build's render-harness output is **byte-identical to the pre-NC run**.

### Requirement outcomes

✅ **Complete (6):** FUNC-07, DSP-06, DSP-07, DSP-08, QUAL-01, **QUAL-04 — 2.2's PARTIAL cleared** ·
⚠️ Partial: **0** · ❌ Failed: **0**.
**Stage 2 total: 18/18 rows complete.** The 10 still `pending` in `REQUIREMENTS.md` are all stage-3
or stage-4.

### Four issues found at verify — none a defect in delivered code

1. **QUAL-04/3's attribution corrected (NC6).** `SUMMARY-2.3` proposed it on "AZ's `width` sweep";
   AZ passes with `width` wired to nothing, because a bit-identical render measures a step of zero
   and zero is under any bound. **QUAL-04/3 = AY (the control is live) + AZ (and it does not
   zipper).** Same class as 2.2's NC3 (AI is not AJ) — the third instance in this stage.
2. **A green probe BE is over-determined (NC7/NC8).** Half 2 poisons the filter while it is *active*,
   so P31's guard cleans the state before the source ever moves inside. DSP-07/8's real coverage is
   **BE half 1 + NC7**; P27's third payoff is evidenced by **NC8**, not by a green run.
3. **Probe AS's position figure moved, and it is a real 2.3 behavioural change.** 0.0008846 →
   **0.0008203** (control 0.0564730 → 0.0524102); the weights half is unchanged to the last digit.
   `hullAtten` defaults to 1.0 dB/m, so a full-range position sweep now crosses the hull. Still met,
   at 20 % of the bound.
4. **Gate 1's cold-configure warning count is 40 again** — 39 repo-wide `JUCE_BUNDLE_ID` messages
   (none O-Octagon's) + 1 concurrentqueue deprecation, **0 compiler diagnostics**. Unchanged from 2.2
   issue 1; re-recorded so it is not read as new.

### Residual — open beyond Stage 2

1. **D5 manual Logic gate — OPEN, carried to STAGE 3 DISCUSS** (decision at this boundary). ~15 min,
   folding in 2.2's Task 12. Fresh VST3 + AU installed at verify, `auval`-clean. Its only unique
   coverage is **QUAL-01/2's *audible* clause** — measurement bounds the D2 step to ~15 % of an 8 kHz
   component as one sample and matches its prediction to 0.000 %, but cannot conclude audibility.
2. **CI gap** — unchanged since 2.1. All 62 probes fire only under `-DOUARICON_BUILD_TESTS=ON`
   locally. Stage 4.
3. **`COMPAT-04`** — retroactive acceptance criteria owed at Stage 4.
4. **`FUNC-06` and `UI-02..05`** — summary rows with no acceptance criteria, owed at **Stage 3
   discuss, before Stage 3 plan.** This stage repaired the same defect three times; carry the habit.

---

## Stage 2 Phase 2.2 Discuss Results (2026-08-11) — `stages/2-dsp/CONTEXT-2.2.md`

Contract checksums re-verified at the boundary — **all four byte-exact on arrival**, then
`ARCHITECTURE.md` deliberately amended and re-pinned by D2 (see the callout above).

Four decisions settled. The architecture was not re-opened — §3.3 fixes every constant, §3.6 fixes
the control grid and inner loop line by line, §5 fixes the processing order.

| # | Decision | Choice |
|---|----------|--------|
| D1 | Gain-stage shape at 2.2 | **Two sub-point slots from day one, `wEff` forced to 0.** The full 17-smoother §3.6.4 inner loop is written verbatim now; `width` is not read. Per §3.4.3 this is not a stub — it is the general path in its degenerate configuration, bit-identical to `width = 0`. 2.3 only makes `wEff` live |
| D2 | `ARCHITECTURE §OQ4` `rigScale ≈ 7.95 m` (wrong since 2.1) | **Correct now and re-pin.** 2.2 is the first phase to consume `rigScale`, via the blur mapping. A phase boundary is exactly where 2.1's "don't edit a checksummed contract mid-phase" objection does not apply |
| D3 | Geometry for the D4 Python DBAP reference | **Self-contained fixture** — it carries the speaker array and every solver input it used; the C++ test builds a `VenueModel` *from* the fixture. No mirrored coordinate table exists anywhere, so none can drift |
| D4 | Human gate before 2.2 verify | **Yes — ~10 min in Logic.** (a) automate `srcX` and confirm the 8 lanes **no longer move in lockstep** (the direct contrast with 2.1); (b) `w3 = 0` → that lane silent, others compensate |

### Declared partial AT DISCUSS — the 2.1 discipline, applied earlier

**QUAL-04 will close ⚠️ partial at 2.2**: criteria 1–2 (position, weights) close here; criterion 3
(**`width`**) moves to 2.3, because under D1 `width` is wired to nothing and a width sweep would pass
vacuously. Named now, with its destination, so verify discovers nothing the plan did not predict.

### Two contract defects found and repaired at this boundary

1. **`ARCHITECTURE §OQ4` `rigScale`** — 7.95 → **7.93165 m**, plus §3.3.2's blur table (`1.99` →
   `1.98` at `blur = 0.50`; `3.97` was already right *by accident* — against 7.95 it should have read
   3.98). Recomputed independently at discuss, reproducing the figure and the centroid to all printed
   digits — the **third** independent derivation, and the first time the contract agrees. Checksum
   re-pinned; the superseded hash is in frontmatter and in an inline note in §OQ4.
2. **`REQUIREMENTS.md` — `PERF-02` and `QUAL-04` had summary-table rows but NO acceptance criteria
   at all.** Both sit on 2.2's traceability line: they would have been "verified" against nothing.
   Criteria are now written, derived from `ROADMAP` 2.2 and `ARCHITECTURE` §3.6.2/§3.6.4/§3.6.5/
   §3.3.5. Separately, **`### DSP-04` appeared twice** — one copy carrying the 2.1 results, one the
   stale original with all three criteria unticked; a verify pass could land on either. Duplicate
   deleted, heading uniqueness now asserted.

> **Owed at 2.3 discuss:** FUNC-07, DSP-06, DSP-07, DSP-08 are in the same state — summary row, no
> criteria. They must be written **before 2.3 plan**. UI-02..05 and COMPAT-04 likewise, for Stage 3/4.

### Eight open questions handed to research

The APVTS write path that actually updates the atomics synchronously in a console app (or QUAL-03's
`memcmp` gate goes flaky rather than failing honestly); `earHeight()` on the audio thread without a
second derivation of a message-thread quantity; `absoluteSampleCounter` lifecycle and reset point;
the dirty check's comparison under NaN/denormal; whether the solve runs in SAFE mode (`auval`
exercises that path at four configs, so it is load-bearing for COMPAT-01); `pow` instrumentation that
compiles to nothing in Release; FFT tone set and bin-centring for Layer 3; fixture format for D3.

### Confirmed available from 2.1 — do not rebuild

`hull::isInside()` and `hull::project()` are **free functions over raw storage**, callable straight
against `snapshot.hullPts` / `hullCount` / `hullEpsCross` from the audio thread — §5 step 4 needs
exactly this, and 2.1's `hullEpsCross` deviation exists to make it possible. The 17 parameter atomics
are already cached in `PluginProcessor.h` as the control-grid snapshot source.

## Stage 2 Phase 2.2 Research Results (2026-08-11) — `stages/2-dsp/RESEARCH-2.2.md`

Contract checksums re-computed at this boundary — **all four byte-exact**, including the architecture
pin re-issued at discuss (`cd881a10…`). All eight open questions answered against JUCE 8.0.14 source
with file:line references; toolchain claims verified by running the toolchain.

**Answers, in one line each**

| Q | Answer |
|---|---|
| 1 | `getParameter(id)->setValueNotifyingHost(convertTo0to1(real))` is **fully synchronous** — no timer, no message loop. `setValue()` alone does **not** notify and leaves the cached atomic stale (QUAL-03 would pass vacuously). The `ValueTree` mirror **is** timer-driven, so never assert against `apvts.state` in a console app |
| 2 | Free functions over the four published values, `VenueModel` delegating — the 2.1 `hull::` precedent again. **Q2 understated it: `normToMetres` needs the same treatment and carries its own separate `kMinSpan` guard** |
| 3 | `uint64_t`, plugin-local, monotonic, reset **only** in `prepareToPlay()`. Never from the playhead. 64 divides 2⁶⁴, so even the wrap preserves grid phase |
| 4 | `memcmp`, **because** of NaN, not despite it — `x != x` would re-solve every block forever. A host really can write NaN (`jlimit` passes it; the `jassert` is Debug-only) |
| 5 | Steps 1–7 run in **both** modes, and all 17 `getNextValue()` advance in both — §3.6.4's invariant says *unconditionally*, and the F3 hazard can flip modes between blocks |
| 6 | `inline` counter + forwarding `countedPow` behind `OOCTAGON_INSTRUMENT`, defined only by the test targets. Three counters: `powCalls`, `solveRuns`, `hullProjections` |
| 7 | `juce_dsp` already linked. Bin-centred tones (N = 4096, k = 64…512) + **rectangular** window make "dominant bin" exact. Gain is exactly 1.0 at `w = δ_ij`, so level is assertable too |
| 8 | **Committed** generated C++ header, not build-time — Layer 2 tracks a moving target (JUCE), the DBAP reference does not. Assert at 1e-5 and print the measured value (2.1 F2 precedent) |

**Ten findings beyond the questions — H1 and H2 are HIGH and both are new**

- **H1 — the venue generation counter and the snapshot are two separate atomics**
  (`VenueSnapshot.h:92-115`), so a publish landing between the audio thread's two acquires makes the
  dirty check skip **permanently** against the new geometry. Fix: stamp the generation *inside*
  `VenueSnapshot`. Same class of deviation as 2.1's `hullEpsCross`. Invisible to any probe that edits
  the venue while audio is stopped
- **H2 — QUAL-02's sticky NaN has a second latch site the architecture does not name.** §3.5.2 calls
  the TPT filter *"the only recursive element"*; the 17 `SmoothedValue`s are recursive too, and
  §3.3.4's guard misses NaN (`NaN < eps` is false). Reachable from a **parameter**, before 2.3 exists.
  Mitigation: sanitise the 17-float snapshot at ingestion
- **H3** — §5 step 2 (bbox denormalisation) is assigned to 2.3 but 2.2 cannot solve without it →
  create `SourceShaper` at 2.2 in D1-degenerate form
- **H4** — the §OQ4 venue is exactly mirror-symmetric about x = 6.5 m, so at the default `srcX = 0.5`
  four speaker pairs receive **identical** gains. A naive FUNC-01/3 "all 8 differ" probe fails on a
  correct implementation; the symmetry itself is a strong non-vacuous probe
- **H5** — `earHeight(bbMinY) = rakeFront` for any `rakeRear`, so DSP-04/3 needs its negative half
- **H6** — QUAL-03's protocol is only executable with events at multiples of the **larger** block
  size; a ragged block-size sequence is a strictly stronger second probe and nearly free
- **H7** — input aliasing is sharper at 2.2 than 2.1: per-sample read-before-write, no hoisted read
  pointer, no scratch buffer available
- **H8** — `-fsanitize=realtime` is **unsupported** by Apple clang 17.0.0 (verified by running it).
  PERF-01's no-allocation criterion is measurable via replaced global `operator new`; locks and file
  I/O stay grep + inspection, and SUMMARY must say so
- **H9** — `SmoothedValue::reset()` is a *state* reset (teleports all 17). One call site only
- **H10** — the 5 ms ramp never completes during motion; no probe may assert rendered == solved gain
  while a parameter is moving

**No new dependency, no new module, no change to `build-and-release.yml`.** Eleven items handed to
plan, each with a recommendation.

## Stage 2 Phase 2.2 Plan Results (2026-08-11) — `stages/2-dsp/PLAN-2.2.md`

Contract checksums re-computed at this boundary — **all four byte-exact**, including the D2
architecture re-pin. **13 tasks.** RESEARCH-2.2's eleven open items resolve as **P14–P24**, continuing
the P-series (Stage 1 P1–P4, Phase 2.1 P5–P13).

**Requirement staging declared here, nothing to be discovered at verify:** FUNC-01, FUNC-03, DSP-01,
DSP-02, DSP-04, DSP-05, PERF-01, PERF-02, QUAL-02, QUAL-03 all close ✅; **QUAL-04 ⚠️ partial**
(criterion 3, `width`, → 2.3, declared at *discuss*). **DSP-08 is implemented at 2.2 but does not
close here** — the traceability table assigns it to 2.3; probe AG is supporting evidence only.

| P | Decision |
|---|---|
| P14 | `Source/Data/VenueGeometry.h` — free `earHeight`/`normToMetres` + the shared `kMinSpan`, `VenueModel` delegating. **Both zero-span guards stay independent**; probes V/W assert member == free function |
| P15 | **`SourceShaper` created at 2.2**, §3.4.1 written in full; degeneracy comes from the *caller* passing a literal `0.0f` at a greppable `PHASE-2.3-WIDTH` marker |
| P16 | **Stamp the generation inside `VenueSnapshot`** (H1) — in scope, edits a 2.1 file, deviation from §3.6.6 recorded. Two acquires collapse to one |
| P17 | Sanitise the 17-float snapshot at ingestion; fallback is the parameter's **declared default, read from the parameter object**, never a hand-written table |
| P18 | SAFE mode runs steps 1–7 and advances all 17; `outGain` **not** applied, per §5 as written. Confirmed, not escalated |
| P19 | `DbapSolver` takes **raw inputs** (`Vec3 spk[8]`, `w[8]`, …) so the DBAP probes live in the fast `tests/unit/` target and the narrow link line survives |
| P20 | `OOCTAGON_INSTRUMENT=1` on both test targets; **four** counters — `powCalls`, `solveRuns`, `hullProjections`, **`sampleAdvances`** (added: PERF-02/4 is otherwise unmeasurable) |
| P21 | Block-size matrix: 512-vs-4096 at 4096-multiple offsets (mandated) **plus** a ragged `1,7,64,333,4096` sequence (the real gate) |
| P22 | DBAP oracle is a **committed generated header** `tests/fixtures/DbapReferenceFixture.h`; generator ships beside it with `--check`; hard gate 1e-5 with the measured deviation printed |
| P23 | `prepareToPlay`: counter reset → `reset(sr,0.005)` ×17 → one `updateControl()` → `setCurrentAndTargetValue()` ×17. **One reset site, ever** |
| P24 | `GainStage` owns the grid and the smoothers; the processor evaluates `mappedOutputAvailable()` and passes G1 **in**. Never re-derived, never `getTotalNumOutputChannels()` |

### New at plan — predicted, not to be discovered at execute

- **Probe Q will FAIL as written.** It asserts unity through all 8 outputs (`max |out − in| = 0` at
  2.1). Once DBAP is live each lane carries `v_i · in` and `Σ v_i² = 1`, so no lane is at unity. Q is
  **re-specified** to the stronger statement — with `w = δ_ij`, `v_j = 1` analytically, so lane
  `speakerToBuffer[j]` reproduces the input. Asserted at **1e-6 relative, not bit-exact**: the
  `k = 1/√(t²)` round-trip is not guaranteed exact in single precision and a bit-exact gate would be
  flaky rather than strict.
- **QUAL-04 is only testable against a DC input** — the rendered sample must *be* the gain, or the
  signal's own per-sample steps swamp the measurement. Bound: `|Δout| ≤ 1/240 + 1e-6` (`v_i ∈ [0,1]`
  from `Σ v² = 1`; 240 steps at 48 kHz). It carries a **negative control** — the same sweep with the
  vector held per control block must *exceed* the bound.
- **`powCalls == 16` exactly**, not merely ≤ 32: the bound alone passes if the `t = pow(d,−a)` reuse
  is dropped.
- **Three `PHASE-2.3-*` markers** (`-WIDTH`, `-AIR`, `-TRIM`), each asserted exactly once at 2.2 and
  zero at 2.3 — the Stage-1 `PHASE-2.2-REPLACE` mechanism, which worked.
- **PERF-01's method is stated, not just its verdict** (H8): allocation **measured** via a replaced
  `operator new` family (every variant, warmed up first); locks and file I/O **grep + inspection**;
  RTSan unavailable on Apple clang 17.0.0.

**Probe letters:** unit target gains **V–AH**, render harness gains **Q′ and AI–AT**. A–U (2.1) must
all still pass.

## Stage 2 Phase 2.2 Execute Results (2026-08-11) — `stages/2-dsp/SUMMARY-2.2.md`

Contract checksums re-computed at **both** boundaries — all four byte-exact on arrival and on
departure; no contract was edited during execute. **12 of 13 tasks complete**; Task 12 (manual Logic
gate, D4) is HUMAN and open.

**46 probes across two test targets, 0 failures.** Unit target 29 (A–P + **V–AH**), render harness 17
(Q′, R–U + **AI–AT**). All of A–U still pass. Every gate re-run here, none read out of a prior doc.

| Gate | Result |
|---|---|
| Clean 3-format build + both test targets, forced TU recompile | ✓ **0 warning/error matches in the entire log** |
| Hardcoded output channel indices outside `ChannelMap` | ✓ **zero** — the 2 grep hits are INPUT reads (§3.4.3); the only 2 output writes go via `speakerToBuffer[i]` and a bounded loop var |
| `PHASE-2.3-WIDTH` / `-AIR` / `-TRIM` | ✓ **1 / 1 / 1**; `PHASE-2.2-REPLACE` still **0** |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED** |
| pluginval s10, VST3 ×3 / AU ×3 | ✓ SUCCESS on all six, exit 0, **zero `FAILED`** |
| Both test targets | ✓ exit 0 / exit 0 |
| `gen_dbap_reference.py --check` | ✓ exit 0 — 102 cases |
| 17 params vs `parameter-spec.md` | ✓ **17/17**, compared programmatically; 2.2 adds none |
| `setLatencySamples` / `switch` on ChannelType / `createEditor` guard | ✓ absent / absent / present |
| `OOCTAGON_INSTRUMENT` in the plugin target | ✓ **0** — only the two test targets define it |

### Headline measurements

- **Probe Y — DSP-01's independence gate: max |impl − oracle| = 1.0236e-7 over 102 committed
  fixture cases. DSP-01's 1e-6 IS MET** (hard gate set at 1e-5 per 2.1 F2 precedent).
- **AA:** max |Σv²−1| = **3.259e-7** over **7686** solves across the rolloff × blur product.
- **AE:** **16** `pow` per solve pair — asserted `== 16`, not merely `≤ 32`.
- **AO:** **0 allocations** across 66 `processBlock` calls incl. an 8192-sample over-size block.
- **AP:** idle solves **0** (pow 0); param-change solves **1** (pow 16); inside proj **0**, outside
  proj **2**; venue-edit solves **1**.
- **AS:** per-sample position 0.00088 / weights 0.00165 against a 0.00417 bound, and the **negative
  control fires at 0.056 / 0.106** — the probe can fail.
- **AL / AM / AN:** bit-identical by `memcmp`, never a tolerance.

### PERF-01 criterion 1 — say the method (H8)

Allocation is **MEASURED** (replaced global `operator new` family, every variant incl. both
`align_val_t` overloads, warmed up first). **Locks and file I/O are grep + inspection only.**
`-fsanitize=realtime` is **unavailable** on Apple clang 17.0.0. Do not restate this as
"RT-safety harness pass".

### Three deviations

1. **P16 (planned)** — `VenueSnapshot::generation` moved inside the payload, beyond §3.6.6's field
   list. Same class as 2.1's `hullEpsCross`. Probe AQ measures it with a venue edit published
   *between* two `processBlock` calls.
2. **P15 (planned)** — `SourceShaper` created a phase early, §3.4.1 written in full; degeneracy is in
   the caller's literal `0.0f` at the `PHASE-2.3-WIDTH` marker.
3. **`getAPVTS()` added (NOT in the plan)** — the harness needs the synchronous
   `setValueNotifyingHost` path (Q1) and `apvts` was private. Not a test-only hook: it is the accessor
   name used by 12+ sibling plugins and Stage 3.1's WebView relays need it. Recorded because it
   widens the public API.

### Findings worth carrying

- **`juce::String (const char*)` is ASCII-only** (`juce_String.h:88-94`); `operator+=` / `operator<<`
  append through `CharPointer_UTF8` (`juce_String.cpp:773`). `detail = "… — …"` renders as `â`;
  `detail << "… — …"` is correct. **No compiler warning.** Matters for Stage 3 UI strings.
- **The §OQ4 rig is not exactly mirror-symmetric in float32.** `0.5f`/`12.5f` are exact so pairs
  (1,2), (3,8), (4,7) are bit-identical; `9.8f`/`3.2f` are not, so **(5,6) is not**. The asymmetry is
  real in double (visible in the fixture's 9th significant figure) but currently below one float ULP.
  Probe AF asserts bit-identity only for the three genuinely exact pairs.
- **The puck can never leave the speaker bbox**, and `srcX=0, srcY=0` is exactly speaker 1 — a hull
  **vertex**, which `isInside` (an inside-*or-on* test) accepts, so no projection fires. AP now
  *searches* for an outside position and asserts it found one.
- **A helper that leaves only the last block in its destination must return the absolute offset** —
  Q′ initially failed at 0.96 while routing was provably perfect, purely from a window mismatch
  against a position-deterministic excitation.
- **`posix_memalign`, not `std::aligned_alloc`** — libc++ gates the latter behind a feature macro, and
  a probe that fails to *build* is a probe that silently stops running.

### Predicted at plan, confirmed at execute

- **Probe Q failed as written** (`max |out − in| = 0.344283581`) on the first post-solver run, exactly
  as predicted. Re-specified to Q′: with `w = δ_ij`, `v_j = 1` analytically → measured **6.0e-8** at a
  1e-6 relative gate.
- The three `PHASE-2.3-*` markers needed **two rounds**: the first pass had each appearing twice
  because prose comments quoted the literal token, which would survive the real marker's retirement
  and keep 2.3's zero-occurrence gate non-zero.

### Requirement outcomes expected at verify

FUNC-01 ✅ · FUNC-03 ✅ · DSP-01 ✅ · DSP-02 ✅ · DSP-04 ✅ · DSP-05 ✅ · PERF-01 ✅ · PERF-02 ✅ ·
QUAL-02 ✅ · QUAL-03 ✅ · **QUAL-04 ⚠️ partial (criterion 3, `width`, → 2.3)**.
**DSP-08 implemented, NOT ticked** — probe AG is supporting evidence only.

## Stage 2 Phase 2.2 Verify Results (2026-08-11) — `stages/2-dsp/VERIFICATION-2.2.md`

**Verdict: ✅ VERIFIED. Ready for Phase 2.3: yes, no blockers.**

Every gate **re-run from scratch at verify** on a forced full recompile (`rm -rf build/plugins/O-Octagon`,
119 steps), not read out of `SUMMARY-2.2.md`. All 10 passed; 46 probes / 0 failures; none of A–U
regressed. Contract checksums recomputed — all four byte-exact including the D2 re-pin (C6).

| Gate | Verify-phase result |
|---|---|
| Clean 3-format build + both test targets, forced TU recompile | ✓ exit 0, **zero compiler diagnostics** |
| Hardcoded output indices outside `ChannelMap` | ✓ 2 hits, both INPUT reads bounded by `numIn`; one output write, `speakerToBuffer[i]` |
| `PHASE-2.3-WIDTH` / `-AIR` / `-TRIM` / `PHASE-2.2-REPLACE` | ✓ **1 / 1 / 1 / 0** |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED**, 17 × `-parameter PASS` |
| pluginval s10, VST3 ×3 / AU ×3 | ✓ SUCCESS on all six, exit 0, zero `FAILED` |
| Both test targets | ✓ 29 + 17 probes, **0 failures**, exit 0 / exit 0 |
| `gen_dbap_reference.py --check` | ✓ exit 0 — 102 cases |
| 17 params vs `parameter-spec.md` | ✓ **17/17 across three sides** — spec parsed, source parsed, `auval` runtime dump; no side hand-transcribed |
| `setLatencySamples` / `switch` on ChannelType / `createEditor` guard | ✓ absent / absent / present |
| `OOCTAGON_INSTRUMENT` in the plugin target | ✓ **0**; 1 each in the two test targets |

### Four negative controls — new work at verify

Execute *asserted* its load-bearing gates work. Verify **measured** it: each claim that carries a
requirement alone was broken deliberately, then reverted and the tree proved byte-identical.

- **NC1 — drop the `(z_i − z_s)` term.** Probe Y 1.0236e-7 → **0.39153065** FAIL; probe Z 8/8 → **0/8
  lanes** FAIL. **But AA still PASSES** (3.5154e-7) — `Σ v² = 1` is a *normalisation* invariant, so
  **DSP-02 is structurally incapable of catching a DSP-01 defect.** DSP-01 rests on Y and Z alone.
- **NC2 — perturb one committed fixture gain by 1e-4.** `--check` → exit 1; probe **Y fails at
  exactly 1.0000e-4**. Y genuinely reads the fixture rather than recomputing, and the 1e-5 gate is
  tight. Both drift directions covered.
- **NC3 — bypass the channel map** (`speakerToBuffer[i]` → `i`), i.e. R1 itself. AJ **FAIL**, all 8
  misrouted, every dominant bin off by one; Q′ **FAIL** at 0.499. **AI PASSES bit-identically** —
  **AI is invariant under a map permutation**, so it evidences FUNC-01/3 (independence) and is *no
  evidence at all* for FUNC-03/3 (routing). SUMMARY's attribution was already correct; recorded
  because the two probes look interchangeable in a results table.
- **NC4 — remove the generation term from the dirty check** (the H1 mode P16 exists to prevent).
  AQ **FAIL**, `THE DIRTY CHECK IS STALE (H1 has regressed)`; AP venue-edit solves **1 → 0**. The one
  deviation justified by a hazard argument now has a measured failure path.

### Requirement outcomes

✅ **Complete (10):** FUNC-01, FUNC-03, DSP-01, DSP-02, DSP-04, DSP-05, PERF-01, PERF-02, QUAL-02,
QUAL-03 · ⚠️ **Partial (1):** QUAL-04 (2/3; criterion 3 `width` → 2.3) · ❌ **Failed: 0**

**QUAL-04's partial verified as real, not accepted on assertion:** `GainStage.cpp:147` reads
`const float widthMetres = 0.0f;` — `p[params::width]` is not read in the solve path, so a width
sweep at 2.2 would pass vacuously. **DSP-08 remains `pending`** — probe AG is supporting evidence
only; the traceability table assigns it to 2.3.

### Three issues found at verify — none a defect in delivered code

1. **`SUMMARY-2.2.md` Gate 1 overstates its scope.** "0 warning/error matches in the entire log" holds
   for compile and link, but a **cold-configure** log carries **40** CMake configure-stage matches —
   39 × `JUCE_BUNDLE_ID … contains spaces` emitted once per plugin target **repo-wide** (none
   O-Octagon's) + 1 concurrentqueue deprecation. Filtering to `warning:` / `error:` / `FAILED` gives
   **zero**. Recorded so a rerun on a cold build dir is not read as a regression.
2. **AI and AJ are not interchangeable** (NC3).
3. **DSP-02 cannot backstop DSP-01** (NC1).

### New at verify — a fourth criteria gap, already closed green

A criteria audit found **nine** requirements with a summary row and no `###` section. Eight are
`pending` (FUNC-06, FUNC-07, DSP-06, DSP-07, DSP-08, UI-02..05) — the known debt. **The ninth is
`COMPAT-04`, ticked `complete` at stage-1.** That is the PERF-02/QUAL-04 defect one step further
along: it did not merely risk verifying against nothing, it *did*. Not re-opened at 2.2; criteria owed
retroactively at Stage 4. `### DSP-04` remains unique — the duplicate-heading defect stays repaired.

### Residual — open beyond 2.2, none blocking 2.3

1. **Task 12, the manual Logic gate (D4) — still OPEN, carried to 2.3 verify.** Fresh VST3 + AU
   installed and `auval`-clean. (a) automate `srcX`, confirm the 8 lanes no longer move in lockstep;
   (b) `w3 = 0` → that lane silent, others compensate.
2. **QUAL-04 criterion 3** (`width`) → 2.3.
3. **CI gap** — unchanged from 2.1; all 46 probes fire only under `-DOUARICON_BUILD_TESTS=ON`.
   Stage 4.
4. **Criteria owed at 2.3 discuss, before 2.3 plan** — FUNC-07, DSP-06, DSP-07, DSP-08.
5. **`COMPAT-04` criteria owed retroactively at Stage 4.**

## Stage 2 Phase 2.1 Discuss Results (2026-08-11) — `stages/2-dsp/CONTEXT-2.1.md`

**Contract checksums re-verified at the stage boundary — all four byte-exact.** This was the
explicit carry-forward from Stage 1 issue 7 (`pattern_promotion_checksum_pins_replaced_file`).
No drift; 2.1 plans against these exact documents.

Four decisions settled; the architecture itself was not re-opened (ARCHITECTURE.md already resolves
OQ1–OQ5, both design defects, and every numeric default).

| # | Decision | Choice |
|---|----------|--------|
| D1 | Cycle granularity | One full GSD cycle per phase (2.1 → 2.2 → 2.3) |
| D2 | Task 13 (Logic 8-ch negotiation) | Run **before 2.1 execute** — discuss/research/plan do not depend on it |
| D3 | 2.1 test vehicle | Stand up **both** the `tests/` unit target **and** `tests/render-harness/` now (harness pulled forward from 2.2) |
| D4 | DBAP reference for 2.2's DSP-01 gate | **Python** script in `tests/tools/` emitting a committed fixture — a C++ reference beside the implementation mirrors the same misreading (`pattern_test_fixture_mirrors_drift_silently`) |

**D3 closes two Stage 1 issues a phase early:** issue 3 (unity gain confirmed by inspection only)
and issue 4 (the F3 3–7-channel hazard reasoned, not measured — the harness constructs those bus
layouts programmatically, no hardware needed).

**Five open questions handed to research:** what replaces the `PHASE-2.2-REPLACE` block at 2.1 with
no solver yet; how `gen_juce_channel_order.py` locates JUCE portably (CI does not use the local
tree); whether the unit target runs under CI; where hull classification surfaces before the Venue
screen exists; which unit-test framework.

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-08-11 |
| research | ✓ | 2026-08-11 |
| plan | ✓ | 2026-08-11 |
| execute | ✓ | 2026-08-11 |
| verify | ⚠️ PARTIAL | 2026-08-11 |

### Stage 2: DSP — Phase 2.1 (Geometry Core)
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-08-11 |
| research | ✓ | 2026-08-11 |
| plan | ✓ | 2026-08-11 |
| execute | ✓ | 2026-08-11 |
| verify | ✅ VERIFIED | 2026-08-11 |

### Stage 2: DSP — Phase 2.2 (DBAP Solve + Gain Application)
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-08-11 |
| research | ✓ | 2026-08-11 |
| plan | ✓ | 2026-08-11 |
| execute | ✓ | 2026-08-11 |
| verify | ✅ VERIFIED | 2026-08-11 |

### Stage 2: DSP — Phase 2.3 (Source Shaping + Outside-Hull)
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-08-11 |
| research | ✓ | 2026-08-11 |
| plan | ✓ | 2026-08-11 |
| execute | ✓ | 2026-08-11 |
| verify | ✅ VERIFIED | 2026-08-11 |

### Stage 2: DSP — STAGE CLOSE
| | |
|---|---|
| Stage-level report | `stages/2-dsp/VERIFICATION.md` |
| Verdict | **✅ VERIFIED** — ready for Stage 3, no blockers |
| Requirements | **18/18 rows complete**, 0 partial, 0 failed |
| Probes | 62 (33 unit + 29 render harness), 0 failures |
| Negative controls | 13 across the stage (1 + 4 + 8) |

### Stage 3: GUI
| Phase | Status | Date |
|-------|--------|------|
| discuss | — | next |

### Stage 2: DSP — Phase 2.3 (Source Shaping + Outside-Hull)
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-08-11 |
| research | ✓ | 2026-08-11 |
| plan | ✓ | 2026-08-11 |
| execute | ○ | — |
| verify | ○ | — |

## Stage 2 Phase 2.1 Verify Results (2026-08-11) — `stages/2-dsp/VERIFICATION-2.1.md`

**Verdict: ✅ VERIFIED. Ready for Phase 2.2: yes, no blockers.**

Every automated gate was **re-run from scratch at verify**, not read out of SUMMARY-2.1.md. All 12
passed; none regressed. Contract checksums recomputed — all four byte-exact (C6).

| Gate | Verify-phase result |
|---|---|
| Clean 3-format build + both test targets, forced TU recompile | ✓ exit 0, **zero warning/error matches in the whole log** |
| Hardcoded output indices outside `ChannelMap` | ✓ 3 hits, all doc prose; `processBlock` read line-by-line |
| `PHASE-2.2-REPLACE` | ✓ **0** |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED**, 17 × `-parameter PASS` |
| pluginval s10, VST3 ×3 / AU ×3 | ✓ SUCCESS on all six, exit 0 each |
| Both test targets | ✓ 16 probes / 5 probes, **0 failures**, exit 0 / exit 0 |
| 17 params vs `parameter-spec.md` | ✓ **17/17** name, range, default AND group — compared programmatically, neither side hand-transcribed |
| `setLatencySamples` / `switch` on ChannelType / non-goal symbols | ✓ all absent |
| `createEditor` `#if JUCE_WEB_BROWSER` guard (G8) | ✓ present, provably inert |
| Test dirs gated behind `OUARICON_BUILD_TESTS` (default OFF) | ✓ |

### Negative controls — new work at verify, not run at execute

Execute *asserted* the Layer-2 gate works. Verify **measured** it. A gate never observed to fail is
a gate whose failure path is untested.

- **NC1 — the golden genuinely tracks parsed JUCE source.** Mutating `leftSurroundRear = 20 → 90` in
  a copied JUCE tree moved the generated SHA `5cd774cb…` → `39298098…`. The baseline is byte-identical
  to the committed constant. **Not a mirrored fixture.**
- **NC2 — divergence FAILS THE BUILD.** Substituting the mutated SHA produced
  `error: static assertion failed`, `BUILD_EXIT=1`. ROADMAP:131 requires the *build* to fail, not a
  test run. Reverted and rebuilt green before proceeding.
- **NC3 — the generator refuses a vacuous golden.** Bad `--juce-modules` → exit 1, no header; a JUCE
  tree parsing to zero enum entries → exit 1, no header; two runs byte-identical.
- **NC4 — `rigScale` recomputed independently** from §OQ4's own table: **7.93165 m**; centroid
  `(6.5000, 12.4625, 4.9250)` and bbox exact. F1 confirmed — the spec prose is wrong, not the code.

### New at verify

- **V1 — Logic negotiated `7.1` (`create7point1()`), NOT 7.1-SDDS. The Stage-0 R2 prediction is
  contradicted and is now retired.** R2 reasoned from `kAudioChannelLayoutTag_Emagic_Default_7_1`.
  Impact nil — all three containers are accepted, the map is keyed on `ChannelType` not on container,
  and nothing branches on the choice. But the prediction was being carried forward as fact into
  COMPAT-02 at Stage 4. Bonus: the container Logic actually uses is the **primary** one, not the
  least-exercised `create5point1point2()` VST3 path (F5).
- **V2 (benign)** — `outputGain` reads `Default = 0.0000007` in the auval dump. AU
  normalise→denormalise float round-trip of `0.0 dB` over `−24…+12`; within the 1e-6 tolerance.
  Recorded so nobody chases it.

### Human verification — Task 1 CLOSED, all four observations positive

- ✅ **All 8 surround-meter lanes moved** — proves negotiation and writability. **Not** independence:
  all 8 lanes carry identical signal at this phase by design. Independence is FUNC-01/3 at 2.2.
- ✅ **Container: `7.1`** (see V1)
- ✅ **`outputGain` survived save → close → reopen** — FUNC-05 slice confirmed in a real host
- ✅ **17 parameters under 5 groups** in the automation menu, matching the auval clump dump

### Requirements

| Req | Status |
|---|---|
| COMPAT-03 | ✅ **complete** — 3/3 criteria |
| DSP-03 | ✅ **complete** — 4/4 criteria |
| DSP-04 | ⚠️ **partial** — 2/3; criterion 3 (`rakeRear` changes the **gain vector**) → 2.2 |
| FUNC-03 | ⚠️ **partial** — 2/3; criterion 3 (a row edit moves audio) → 2.2 |

### Why VERIFIED and not PARTIAL

Stage 1 closed PARTIAL because a mis-staged requirement was discovered **at verify**. 2.1 is the
direct correction: both partials were declared in `PLAN-2.1.md` **before execute began**, with reason
and destination phase named. Nothing was discovered at verify the plan did not predict, no gate
failed, and the one genuinely open item — the Logic manual gate — is closed by this verify.

### Residual — open beyond 2.1, none blocking 2.2

1. **CI gap.** No test target in this repo has ever run in CI, so the Layer-2 build-failing checksum
   only fires for whoever configures with `-DOUARICON_BUILD_TESTS=ON`. **A JUCE bump performed
   without building the test target ships silently.** Logged at
   `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md`; belongs to Stage 4.
2. **`ARCHITECTURE §OQ4` `rigScale ≈ 7.95`** → correct to `7.93` at the next contract revision. Not
   edited mid-phase: it is a checksummed contract and editing it invalidates the pins 2.2 re-verifies.
3. **DSP-04/3 and FUNC-03/3** staged to 2.2.

## Stage 2 Phase 2.1 Execute Results (2026-08-11) — `stages/2-dsp/SUMMARY-2.1.md`

Contract checksums re-verified at the boundary — all four byte-exact. **13 of 14 tasks complete;
Task 1 (manual Logic check) reported done, its four observations still to be recorded.**

**21 probes across two test targets, 0 failures. Every gate re-run, none read out of a prior document.**

| Gate | Result |
|---|---|
| Clean 3-format build + both test targets, forced TU recompile | ✓ **0 warnings, 0 errors** in the entire log |
| Hardcoded output channel indices outside `ChannelMap` | ✓ **zero** (4 remaining `8` hits are doc prose) |
| `PHASE-2.2-REPLACE` | ✓ **0** — retired, not grandfathered |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED**, 17 × `parameter PASS` |
| pluginval s10, VST3 ×3 / AU ×3 | ✓ SUCCESS on all six |
| Both test targets | ✓ exit 0 / exit 0 |
| 17 params vs `parameter-spec.md` | ✓ **17/17** on name, range, default **and group** |
| `setLatencySamples` | ✓ absent |

**Two Stage-1 carry-overs closed a phase early, as D3 predicted.** Issue 3 (unity gain confirmed by
inspection only) → probe Q **measures** it, `max |out − in| = 0.000000000` across all 8 lanes.
Issue 4 (the F3 3–7-channel hazard reasoned, not measured) → probe S constructs those layouts
programmatically, no hardware, and asserts *both* that the hazard state is genuinely reproduced
(`getTotalNumOutputChannels() == 8` while the buffer is narrower) **and** that nothing writes OOB.

### Findings worth carrying

- **`ARCHITECTURE §OQ4`'s `rigScale ≈ 7.95 m` is wrong — the true value is 7.9317 m.** Recomputing
  by hand from §OQ4's own coordinate table gives 7.93165, so this is a hand-calc slip in the spec,
  not an implementation error; centroid and bbox in the same section match exactly. Impact nil today
  (`rigScale` is consumed by the blur mapping at 2.2; the error is 0.23%). **Not corrected in
  ARCHITECTURE** — it is a checksummed contract and editing it mid-phase would invalidate the pins
  2.2 re-verifies against. Correct it at the next contract revision.
- **This is exactly why probe O asserts the scaling INVARIANT, not the constant.** A bare `≈ 7.95`
  assertion would have been "fixed" by tuning the code until it produced 7.95
  (`pattern_test_fixture_mirrors_drift_silently`).
- **Probe H does not test what it looks like it tests** — confirmed empirically. The §OQ4 walls are
  dead straight, so speakers 3 and 8 have *exactly* zero cross product and pop for any non-negative
  epsilon; H would pass with `EPS_CROSS = 0`. Probe I supplies the coverage: 1 µm popped, 1 mm kept,
  against a measured `EPS_CROSS` of 1.0e-4.
- **G4 confirmed:** a label of `"7"` resolves to type **134** (`discreteChannel0 + 6`) — a plausible
  discrete channel, **not** `unknown` — and is rejected by the *permutation check*, not by a parse
  error. The mechanism keeping this safe is not the one a reader would assume.
- **Probe K's oracle is genuinely independent:** ternary search on a convex 1-D function in double
  precision, not the closed-form dot-product projection the implementation uses. Measured worst
  deviation **8.67e-7 m**, which meets DSP-03 criterion 3's 1e-6; the asserted bound is set at 1e-4
  because one float ULP at the 27 m sampling extent is already ~2e-6.
- **`juce::var` double serialisation uses `serialiseDouble()` (15–20 dp), not `ostream`'s 6-sig-fig
  default** — so float → XML → float is exact and FUNC-02's exactness is not at risk. Probe T
  asserts all 42 values bit-exactly via `memcmp`.
- **Generator failure paths tested, not asserted in prose:** bad `--juce-modules` → exit 1; a JUCE
  tree parsing to zero enum entries → exit 1 **and no header written**; two runs byte-identical.

### Deviations from PLAN-2.1 (both deliberate)

1. `VenueSnapshot` carries one field beyond §3.6.6's list — `hullEpsCross`. `hull::isInside()` needs
   the room-scaled tolerance, and recomputing it on the audio thread would be a second derivation of
   a message-thread quantity, which is the drift the snapshot exists to prevent.
2. `ConvexHull2D::classify()` also returns `VERTEX` by coordinate, not only by index. §3.1.3's
   index-only definition leaves the STEP-0 dedup case ambiguous; a duplicate sitting exactly on a
   corner is a corner.

### Requirement outcomes (planned at plan, not discovered at verify)

COMPAT-03 ✅ complete · DSP-03 ✅ complete · DSP-04 ⚠️ partial (criterion 3 → 2.2) ·
FUNC-03 ⚠️ partial (criterion 3 → 2.2). `REQUIREMENTS.md` is updated at **verify**.

## Stage 2 Phase 2.1 Plan Results (2026-08-11) — `stages/2-dsp/PLAN-2.1.md`

Contract checksums re-verified — all four byte-exact. **14 tasks**, the first of which is the manual
Logic gate (Task 13 from Stage 1) and blocks every other.

**Requirement staging corrected at plan, not at verify** (the direct lesson of Stage 1's FUNC-01
mis-staging). Two of the four requirements this phase "verifies" cannot fully close here:

| Req | Outcome expected at 2.1 verify |
|---|---|
| COMPAT-03 | ✅ complete — all 3 criteria |
| DSP-03 | ✅ complete — all 4 criteria |
| DSP-04 | ⚠️ **partial** — criterion 3 ("changing `rakeRear` alone changes the **gain vector**") needs `DbapSolver` → 2.2 |
| FUNC-03 | ⚠️ **partial** — criterion 3 ("changing a row moves audio to that output") is unobservable while all 8 lanes carry identical signal → 2.2 Layer 3 |

The five RESEARCH open items are resolved as P5–P13:

- **P5** — targets `O-Octagon-geometry-test` / `O-Octagon-render-test`; both reference the *plugin
  target* `OuariconOctagon`, never the folder name
- **P6** — the SAFE/REAL branch is a **named helper** `mappedOutputAvailable(int)`, so G1 is stated
  once and 2.2's `GainStage` inherits it rather than re-deriving it
- **P7** — `ChannelMap` is **free functions in `ochan`** with no processor reference (so the unit
  target can link it without `PluginProcessor.cpp`); `rebuildChannelMap()` remains the single
  construction site
- **P8** — Layer 1 is **one function with two callers**: a Debug `jassert` in `rebuildChannelMap()`
  and a unit probe. G2's two fixes fold in — scan bound 256 **and** `expected.size() == set.size()`
- **P9** — 200-point hull fixture **generated in-test from a pinned seed**; oracle in the same TU
- **P10** — Layer 2's checksum gate is a **`static_assert`** (ROADMAP says *fails the build*), and the
  SHA covers **parsed data**, not emitted file text
- **P11** — hull classification is a first-class `ConvexHull2D` return value (Q4)
- **P12** — two supporting headers: `Source/DSP/Vec.h`, `Source/Data/VenueSnapshot.h`
- **P13** — the CI gap goes to `.planning/todos/pending/` as a **repo-level** todo;
  `build-and-release.yml` is not touched

**Probe I exists because probe H does not test what it looks like it tests:** the §OQ4 walls are dead
straight, so speakers 3 and 8 have *exactly* zero cross product and are popped regardless of
`EPS_CROSS`. A separate near-collinear case exercises the epsilon.

**Probe O asserts a scaling invariant, not just `rigScale ≈ 7.95`** — a bare constant is a mirrored
fixture (`pattern_test_fixture_mirrors_drift_silently`).

## Stage 2 Phase 2.1 Research Results (2026-08-11) — `stages/2-dsp/RESEARCH-2.1.md`

Contract checksums re-verified — all four byte-exact. Q1–Q5 answered; 9 findings (G1–G9), three of
which are defects in the *specification* found by checking it against JUCE 8.0.14 source.

| Q | Answer |
|---|---|
| Q1 — replaces `PHASE-2.2-REPLACE` | Route the mono sum through `speakerToBuffer` — **but bound the indexing by `buffer.getNumChannels()` independently of `mapInvalid`** (see G1) |
| Q2 — JUCE path for the generator | `JUCE_MODULES_DIR` (`JUCE/CMakeLists.txt:41`, `CACHE INTERNAL`), passed as a script argument. Portable: CI sets `JUCE_DIR` at `build-and-release.yml:131` and the root resolves it identically. `FATAL_ERROR` if empty |
| Q3 — CI | **Local-only at 2.1.** CI is tag-triggered release-only, secrets-bearing, and has *never* set `OUARICON_BUILD_TESTS` — no harness in the repo has ever run in CI. Residual gap logged for Stage 4, not hidden |
| Q4 — hull classification sink | A first-class `ConvexHull2D` return value, not a processor accessor. The 3.2 Venue screen reads the same call — drift becomes structurally impossible |
| Q5 — test framework | **None.** Zero Catch2/GTest/doctest/`juce::UnitTest` anywhere in plugin source or CMake. Use `juce_add_console_app` + `check()`/exit-code, matching all 12 existing harnesses. The Catch2 references in `docs/codebase/TESTING.md` describe an intent never implemented — do not follow them |

### Findings worth carrying

- **G1 (HIGH)** — a valid channel map is **not** evidence of an 8-channel buffer. Under F3 the layout
  reports 7.1 while the buffer holds `n < 8`, so `mapInvalid` stays false and `speakerToBuffer` holds
  indices up to 7. The map is derived from the accessor that lies. Carries into 2.2's `GainStage`.
- **G2 (HIGH)** — ARCHITECTURE §3.2.5 Layer 1's `bit < 64` scan is too small: named types run to 99,
  `discreteChannel0 = 128`, `channels` is a `BigInteger`. Worse, it fails *silently* — a truncated
  list is still strictly increasing. Assert `expected.size() == set.size()`.
- **G3 (HIGH)** — the enum's declaration order ≠ value order (`topSideLeft/Right = 28/29` are declared
  before `ambisonicACN0..3 = 24..27`) and it contains an alias (`surround = centreSurround`). The
  Layer 2 parser must read `= <int>` pairs and assert every referenced name resolved.
- **G5 (HIGH)** — the shipped default label map is the identity map, so a test using it is vacuous
  (C1). Two free non-vacuous cases: a permuted label set under 7.1, and 7.1-types-vs-SDDS (4 of 8
  types absent → the ROADMAP:131 missing-label test from real JUCE sets).
- **G4** — `getChannelTypeFromAbbreviation` already exists (`.h:553`); use it instead of a hand-rolled
  table. Sharp edge: an unvalidated numeric branch (`.cpp:283-285`) turns a label of `"7"` into a
  plausible-looking discrete type rather than `unknown`. Fails safe via the permutation check — test it.
- **G7/G8** — derive `JucePlugin_VersionString` from the target's `JUCE_VERSION`; guard `createEditor`
  with `#if JUCE_WEB_BROWSER` now, while it is inert.
- **D3 resolves to two console apps, not two frameworks:** `tests/unit/` links only the three geometry
  TUs (no `PluginProcessor.cpp`, so no `JucePlugin_*` block and immune to the Stage-3 WebView swap);
  `tests/render-harness/` links the processor.
- **`stages/2-dsp/` is untracked — do NOT execute 2.1 in an isolated worktree**
  (`pattern_worktree_isolation_wrong_for_untracked_scope`).

## Stage 1 Verify Results (2026-08-11) — `stages/1-foundation/VERIFICATION.md`

**Verdict: ⚠️ PARTIAL. Ready for Stage 2: yes, with one caveat.**

Every automated gate below was **re-run from scratch at verify**, not read out of SUMMARY.md.
All passed; none regressed.

| Gate | Verify-phase result |
|---|---|
| Clean rebuild, 3 formats (forced TU recompile) | ✓ **0 warnings, 0 errors** in the entire log |
| `auval -a` | ✓ `aufx OuOc OuDv` |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED** |
| AU channel config set | ✓ `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]` — exactly RESEARCH F2 |
| pluginval s10 VST3 / AU | ✓ SUCCESS ×3 each |
| State round-trip | ✓ pluginval *Plugin state* + *Plugin state restoration*, ×3 |
| 17 params vs `parameter-spec.md` | ✓ 17/17 on name, range, default **and** group |
| Standalone on a 2-ch device (COMPAT-04) | ✓ launched, stayed running |
| `PHASE-2.2-REPLACE` uniqueness | ✓ exactly 1 occurrence |
| Forbidden CMake keywords, `setLatencySamples`, non-goals | ✓ all clean |

### Requirements

| Req | Status |
|---|---|
| COMPAT-01 (pluginval VST3+AU s10) | ✅ **complete** |
| COMPAT-04 (defined behaviour on stereo) | ✅ **complete** |
| FUNC-01 (8 discrete feeds) | ⚠️ **partial → re-mapped to stage-2** |

**Verify finding — FUNC-01 was mis-staged.** Its third acceptance criterion (*"all 8 output channels
carry independent, non-duplicated signal"*) cannot be met by a shell whose D1 placeholder writes the
same mono sum to all 8 **by design**. Independence requires the DBAP solve (DSP-01/DSP-05). The first
two criteria were met at Stage 1 and are recorded as such in REQUIREMENTS.md so Stage 2 does not
re-derive them. `verifiedAt` moved `stage-1` → `stage-2`.

Corollary: Task 13's "all 8 lanes move" proves **negotiation and writability**, not independence —
with the placeholder in place all 8 lanes carry identical signal. Do not over-read it.

### Why PARTIAL, and why Stage 2 is not blocked

Nothing in Stage 2 depends on the Logic result: Phase 2.1 builds `ChannelMap` and the `VENUE` tree,
and the plugin accepts all three 8-channel containers regardless of which one Logic picks. Running
Task 13 first is nonetheless the cheaper order — if Logic fails to negotiate 8 channels the fault is
in the bus predicate, and unpicking that after a DBAP solver exists costs materially more.

## Stage 1 Execute Results (2026-08-11)

**Files created:** `CMakeLists.txt`, `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`,
`.planning/parameter-spec.md` (promoted; draft banner-marked superseded).

| Gate | Result |
|---|---|
| Build — VST3 + AU + Standalone | ✓ clean, **zero warnings** from O-Octagon's own TU |
| `auval -a` | ✓ `aufx OuOc OuDv — Ouaricon Audio Development: O-Octagon-dev` |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED**, 17 × `-parameter PASS` |
| AU channel configs | ✓ `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]` — **exactly RESEARCH F2's prediction** |
| pluginval strictness 10, VST3 | ✓ SUCCESS × 3 runs |
| pluginval strictness 10, AU | ✓ SUCCESS × 3 runs |
| 17 parameters vs `parameter-spec.md` | ✓ verified **programmatically** from the auval dump — all 17 match name, range, default *and* group |
| Standalone on 2-ch device (COMPAT-04) | ✓ opens, no error, generic editor renders 5 groups + units (`m`, `dB/2x`) |

**F2 confirmed empirically.** The AU config set is derived from `isBusesLayoutSupported()` exactly
as researched — this is the first hard evidence that the predicate is the sole authority.

### Outstanding at Stage 1 — carry into verify

1. **Task 13 — Logic 8-channel negotiation is NOT yet done.** Requires Logic and a surround track;
   cannot be automated. It is the strongest available evidence for **FUNC-01 / COMPAT-01**.
   Must confirm: plugin instantiates on a surround track; **all 8 surround-meter lanes move**;
   `outputGain` survives save/close/reopen (FUNC-05 slice); automation menu lists 17 under 5 groups.
   **Record which container Logic actually negotiated** — R2 predicts 7.1-SDDS. Observation, not a
   gate (all three are accepted); it feeds COMPAT-02 at Stage 4.
2. **Task 12 item 3** — audio reaching outputs at unity in Standalone was not confirmed: JUCE's
   Standalone mutes input by default ("Audio input is muted to avoid feedback loop"). Needs a
   manual unmute, or is subsumed by Task 13's meter check.
3. **Gate bypass on record.** The `0-ideation → 1-foundation` gate was run with `--force`: its build
   check is unconditional on stage and cannot pass before `CMakeLists.txt` exists. Logged to
   `.planning/gate-bypasses.log`.
4. **Benign:** pluginval AU emits `WARNING: Current program is -1` — JUCE AU-wrapper reporting,
   present across the repo, not a failure (run still returns SUCCESS).

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (Logic-native 8-channel DBAP spatializer for an irregular, non-flat concert array)
- Architecture inherited as locked constraints from `research/logic-pro-multichannel-octaphonic-dbap.md`
- **17** musical parameters + 42 venue values specified *(corrected from 18 at Stage 0 — see ARCHITECTURE §11)*
- Preset strategy settled (two separate stores)
- Signal flow, UI concept, use cases captured
- 30 requirements extracted with acceptance criteria
- v1.1+ deferrals recorded explicitly

**Stage 0:** ✓ Complete
- Complexity tier 6 (DEEP research); complexity score **5.0** (capped; raw 13.0)
- 9 features researched: bus transport, channel map, venue model, convex hull, DBAP solver,
  source shaping, outside-hull processing, gain stage, verify-ping
- All JUCE APIs verified **directly against the local JUCE 8.0.14 source tree** with file:line
  references (Context7 doc-fetch was unavailable; local source is the stronger authority)
- 8 core DSP components specified with full algorithms
- 3-layer channel-map test strategy designed (runtime invariant → source-parsed golden with a
  committed SHA → offline tone-per-speaker render)
- All 5 open questions resolved with concrete defaults
- Parameter-count discrepancy resolved: **17**, arithmetic slip demonstrated
- 8 risks registered; 2 design defects found and fixed before code exists
- Strategy: **staged implementation** — Stage 2 in 3 phases, Stage 3 in 3 phases
- ARCHITECTURE.md and ROADMAP.md documented

## Stage 0 Findings Worth Carrying

1. **`kAudioChannelLayoutTag_Emagic_Default_7_1`** (`juce_CoreAudioLayouts_mac.h:117`) shows Logic's
   native 7.1 ordering corresponds to JUCE's `create7point1SDDS()` membership, not `create7point1()`.
   `isBusesLayoutSupported()` therefore accepts all three 8-channel containers, and the label map is
   keyed on `ChannelType`.
   > ⚠️ **The R2 corollary drawn from this — "Logic will negotiate 7.1-SDDS" — is CONTRADICTED by
   > observation (2.1 verify, V1). Logic negotiated `create7point1()`.** The layout-tag reading
   > itself still stands; the prediction built on it does not. Impact nil (all three are accepted,
   > nothing branches on the choice), but do not carry the SDDS prediction forward as fact.
   > COMPAT-02 at Stage 4 tests against **7.1** as observed, with SDDS and 5.1.2 as alternates.
2. **For 7.1 the enum-bit order coincidentally equals the initializer-list order** — a hardcoded
   0..7 map would appear correct today. The locked constraint is *more* important because of this,
   not less. **Amplified at Stage 1 research (F1): this holds for all THREE accepted 8-channel
   containers**, so no container choice can discriminate a hardcoded map. Only a non-identity
   `map1..map8` label assignment permutes the buffer — the Phase 2.1 test must drive those.
3. **PERF-02 and QUAL-03 are incompatible under a per-block solve.** Resolved with a fixed
   64-sample absolute-sample-aligned control grid.
4. **Centre-crossing L/R flip** in the stereo sub-point geometry found at design time; fixed with an
   `rFade` spread collapse.

## Stage 1 Research Findings Worth Carrying

- **F1** — all three accepted 8-channel containers have initializer order == enum-bit order (see above)
- **F2** — AU channel configs are *derived* from `isBusesLayoutSupported()`:
  `AUChannelInfo = {(1,1),(1,2),(1,8),(2,1),(2,2),(2,8)}`. `auval` exercises all six, so the Stage 1
  placeholder must be correct at 1 and 2 output channels, not only 8. SAFE mode is load-bearing for
  AU, not just Standalone. Confirms `PLUGIN_CHANNEL_CONFIGURATIONS` is redundant as well as harmful.
- **F3 (hazard)** — Standalone on a 3–7 output device: `canonicalChannelSet(n)` yields LCR/quad/5.0/
  5.1/7.0, all rejected; Debug asserts, **Release keeps the 7.1 layout while the buffer has n
  channels**. Bound every output loop by `buffer.getNumChannels()` — never by `8`, never by
  `getTotalNumOutputChannels()`, which is the accessor that lies in exactly this state.
- **F4** — `canonicalChannelSet(8) == create7point1()`, so Standalone on an 8-out interface
  negotiates REAL mode with no host. Free Stage 2 listening rig.
- **F5** — `create5point1point2()` has no VST3 layout-table entry; it resolves via the generic
  bit-order fallback. Works, least-exercised of the three; keep 7.1 primary.
- **F6** — JUCE hoists `Fx` to index 0 of `VST3_CATEGORIES`; the emitted string is always
  `Fx|Spatial`. Declare `"Fx" "Spatial"` so the source matches what ships.
- **F7** — `AU_MAIN_TYPE kAudioUnitType_Effect` is already JUCE's default. Both keywords are new to
  this repo — no sibling CMakeLists sets either.
- **State root is `OOctagon`** (not the sibling's `OOrbitParams` idiom) and must never change —
  Phase 2.1 attaches `VENUE` to that node. Stage 1 sessions carry **no** `VENUE` child, so
  `readVenueFromState()` must treat a missing/partial node as "use defaults".
- **`-Wswitch-enum` bans switching on `AudioChannelSet::ChannelType`** (~60 enumerators; warns even
  with a `default:`). The Phase 2.1 label map must be a table or `if`-chain.

## Stage 1 Plan Decisions (plan phase)

The four open items RESEARCH §9 handed to the plan phase are resolved in PLAN.md:

- **P1** — five parameter groups: Position / Solve / Weights / Space / Output. The headline
  gesture is automating eight weights; a flat 17-entry menu buries them. Reversible.
- **P2** — the venue-store member slot is claimed **above `apvts`** in `PluginProcessor.h` as a
  comment marker. Declaration order is fixed at Stage 1 and annoying to change once 2.1 depends
  on it. No member exists yet.
- **P3** — the D1 placeholder loop carries the greppable token `// PHASE-2.2-REPLACE:` so Phase
  2.1's "zero hardcoded output indices" gate retires it rather than grandfathering it.
- **P4** — `parameter-spec.md` is promoted from the draft as Task 1. The executor reads the
  promoted file, **never** `parameter-spec-draft.md` (which still marks OQ3/4/5 and the
  17-vs-18 count as open, all four resolved at Stage 0).

Also settled at plan: `srcX`/`srcY` display **normalised** in the host lane (metres are a Stage
3.1 UI-side conversion — a value→text lambda is captured at construction and cannot read a live
venue); **no `PluginEditor.{h,cpp}`** at Stage 1 (`GenericAudioProcessorEditor` is five lines and
is itself the 17-parameter exit criterion).

## Next Steps

1. **Phase 2.2 verify** — `/plugin-verify O-Octagon 2-dsp`. Writes `VERIFICATION-2.2.md`. Re-run
   every gate from scratch rather than reading SUMMARY-2.2 (C6), and re-compute the four contract
   checksums at the boundary. What verify must handle:
   - **Task 12 — the manual Logic gate (D4) is OPEN.** ~10 min. (a) automate `srcX` and confirm the 8
     surround-meter lanes **no longer move in lockstep** — the direct contrast with 2.1; (b) `w3 = 0`
     → that lane silent, others compensate. Record both verbatim, positive or negative. It is
     corroboration; the gate for FUNC-03/3 is probe AJ, which passed.
   - **Three deviations to confirm**, two planned (P16, P15) and one not (`getAPVTS()` — see above).
   - **QUAL-04 closes ⚠️ partial**, criterion 3 → 2.3. Declared at *discuss*; do not treat as a
     discovery.
   - **DSP-08 must NOT be ticked** — implemented at 2.2, assigned to 2.3 by the traceability table.
   - Consider a **negative control** on the new gates, as 2.1 verify did (NC1–NC4): the DBAP fixture's
     `--check` failure path and the generator's zero-case refusal are the obvious candidates, and the
     `--check` mutation path was exercised once at execute.
2. UI mockup — two screens, Room + Venue. Due before Stage 3.1; not a Stage 2 blocker.
3. Measure Roy Barnett Recital Hall — 8 × (x, y, z) metres + front/rear ear heights
4. **Stage 4** — pick up the CI-gap todo and the `ARCHITECTURE §OQ4` `rigScale` correction.

## Context to Preserve

**Build constraints for Stage 1:**
- Target `OuariconOctagon`, folder `plugins/O-Octagon`, `PRODUCT_NAME "O-Octagon${OUARICON_DEV_SUFFIX}"`
- `PLUGIN_CODE OuOc` (verified unused across all 39 existing plugins)
- **`VERSION 1.0.0`**, never `PLUGIN_VERSION`
- **No `PLUGIN_CHANNEL_CONFIGURATIONS`**
- `juce::juce_dsp` linked; `BusesProperties` in the constructor init list
- **Must not link SAF** (unlike sibling O-Orbit)

**Locked by prior research — do NOT re-litigate:**
- mono/stereo in → `AudioChannelSet::create7point1()` out; 7.1 is only an 8-channel carrier
- Never `octagonal()` or `discreteChannels(8)` — Logic ignores both
- Not multi-output (`aumu` only; `aufx` gets one bus)
- Speaker→buffer map built ONCE in `prepareToPlay()` via `getChannelIndexForType()` — a wrong map
  is SILENT and passes every automated gate
- DBAP per the **2011-04-14 revised** equations

**Deferred to v1.1+ — do not plan work for these:**
VBAP A/B mode; binaural/stereo fold-down; quadraphonic variant; internal diffuse reverb; motion
engine; multiple simultaneous sources.

**Highest risk:** the speaker→buffer channel map (R1, CRITICAL, silent failure).

## Files Created

- `plugins/O-Octagon/.planning/BRIEF.md` *(Ideation; parameter count corrected at Stage 0)*
- `plugins/O-Octagon/.planning/REQUIREMENTS.md` *(Ideation)*
- `plugins/O-Octagon/.planning/parameter-spec-draft.md` *(Ideation)*
- `plugins/O-Octagon/.planning/research/ARCHITECTURE.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/ROADMAP.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/stages/0-ideation/CONTEXT.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/stages/1-foundation/CONTEXT.md` *(Stage 1 discuss)*
- `plugins/O-Octagon/.planning/stages/1-foundation/RESEARCH.md` *(Stage 1 research)*
- `plugins/O-Octagon/.planning/stages/1-foundation/PLAN.md` *(Stage 1 plan — 14 tasks)*
- `plugins/O-Octagon/.planning/stages/1-foundation/SUMMARY.md` *(Stage 1 execute)*
- `plugins/O-Octagon/.planning/parameter-spec.md` *(Stage 1 execute — promoted, supersedes the draft)*
- `plugins/O-Octagon/.planning/stages/2-dsp/CONTEXT-2.1.md` *(Stage 2 Phase 2.1 discuss)*
- `plugins/O-Octagon/.planning/stages/2-dsp/RESEARCH-2.1.md` *(Stage 2 Phase 2.1 research)*
- `plugins/O-Octagon/.planning/stages/2-dsp/PLAN-2.1.md` *(Stage 2 Phase 2.1 plan — 14 tasks)*
- `plugins/O-Octagon/.planning/stages/2-dsp/SUMMARY-2.1.md` *(Stage 2 Phase 2.1 execute)*
- `plugins/O-Octagon/.planning/stages/2-dsp/VERIFICATION-2.1.md` *(Stage 2 Phase 2.1 verify)*
- `plugins/O-Octagon/.planning/stages/2-dsp/CONTEXT-2.2.md` *(Stage 2 Phase 2.2 discuss)*
- `plugins/O-Octagon/.planning/stages/2-dsp/RESEARCH-2.2.md` *(Stage 2 Phase 2.2 research)*
- `plugins/O-Octagon/.planning/stages/2-dsp/PLAN-2.2.md` *(Stage 2 Phase 2.2 plan — 13 tasks)*
- `plugins/O-Octagon/.planning/stages/2-dsp/SUMMARY-2.2.md` *(Stage 2 Phase 2.2 execute)*
- `plugins/O-Octagon/Source/Data/VenueGeometry.h` *(2.2 — `oo::plane`, header-only)*
- `plugins/O-Octagon/Source/DSP/DbapSolver.{h,cpp}` *(2.2 — §3.3, + 4 instrumentation counters)*
- `plugins/O-Octagon/Source/DSP/SourceShaper.{h,cpp}` *(2.2 — §3.4.1 in full, inert caller)*
- `plugins/O-Octagon/Source/DSP/GainStage.{h,cpp}` *(2.2 — control grid, 17 smoothers, inner loop)*
- `plugins/O-Octagon/tests/tools/gen_dbap_reference.py` *(2.2 — the DSP-01 oracle, with `--check`)*
- `plugins/O-Octagon/tests/fixtures/DbapReferenceFixture.h` *(2.2 — COMMITTED, 102 cases)*
- `plugins/O-Octagon/CMakeLists.txt` *(Stage 1 execute; 2.1 adds 3 TUs + the test-target option)*
- `plugins/O-Octagon/Source/PluginProcessor.h` *(Stage 1 execute; rewritten at 2.1)*
- `plugins/O-Octagon/Source/PluginProcessor.cpp` *(Stage 1 execute; rewritten at 2.1)*
- `plugins/O-Octagon/Source/DSP/Vec.h` *(2.1)*
- `plugins/O-Octagon/Source/DSP/ConvexHull2D.{h,cpp}` *(2.1)*
- `plugins/O-Octagon/Source/DSP/ChannelMap.{h,cpp}` *(2.1)*
- `plugins/O-Octagon/Source/Data/VenueModel.{h,cpp}` *(2.1)*
- `plugins/O-Octagon/Source/Data/VenueSnapshot.h` *(2.1)*
- `plugins/O-Octagon/tests/tools/gen_juce_channel_order.py` *(2.1 — Layer 2 generator)*
- `plugins/O-Octagon/tests/unit/{CMakeLists.txt,main.cpp}` *(2.1 — 16 probes A–P)*
- `plugins/O-Octagon/tests/render-harness/{CMakeLists.txt,main.cpp}` *(2.1 — 5 probes Q–U)*
- `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md` *(2.1 — repo-level, P13)*
- `plugins/O-Octagon/.planning/STATUS.md`
