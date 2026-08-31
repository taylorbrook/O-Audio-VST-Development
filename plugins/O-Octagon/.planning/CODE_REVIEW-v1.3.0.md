---
plugin: O-Octagon
version_reviewed: 1.3.0
reviewed: 2026-08-25
verified: 2026-08-26
verification: >
  /improve-verify, v1.3.2 (Rounds 1-2: CR-01 + WR-01..WR-05). All six findings CLOSED with evidence.
  Survived-to-disk confirmed in the shipped bundle (installed AU/VST3 carry the fixed CSS/JS; zero
  occurrences of the pre-fix `transform-origin: center` remain in the binary). Version coherent
  end-to-end: CMakeLists 1.3.2 / AU + VST3 CFBundleShortVersionString 1.3.2 / CHANGELOG / PLUGINS.md.
  Gates re-run independently: render-harness 51/51, geometry 46/46, ui_layout_check 31/31,
  ui_frontend_check 42/42, DBAP fixture --check OK (102 cases), auval -v aufx OuOc OuDv SUCCEEDED.
  GOLDEN NEUTRALITY MEASURED, not assumed: the v1.3.2 harness binary was rebuilt against the v1.3.0
  Source tree and diffed probe-line by probe-line. 48 of 51 render probes and 45 of 46 geometry
  probes are BYTE-IDENTICAL across the resolution. The three that move are accounted for: CR and P2
  are the fix probes (they reproduce NC6 and NC4 exactly), BL moves only its publish-GENERATION
  counter (3->4 -> 2->3, which IS WR-01; its measured geometry is unchanged, and venueGen is only
  ever change-detected, never magnitude-compared), and BR is a noise-seeded ping whose RMS scatters
  -19.78..-20.17 dBFS across repeat runs of one binary, inside its own stated +/-1.5 dB tolerance.
  No audio-path golden moved. Adversarial pass over the resolution diff: no regressions survived.
  Deferred to human DAW test: WR-01 under a live host preset switch, WR-02 across a real
  Retina/non-Retina display move, and a WKWebView eyeball of CR-01/WR-04/WR-05.
  ── RE-VERIFIED 2026-08-26 THROUGH v1.3.5 (span v1.3.3, v1.3.4, v1.3.5) ──
  The CR/WR closure above still holds: nothing in the span touches those fixes, and every gate
  they rest on re-ran green against the v1.3.5 tree. Resolution under test was SIMPLIFICATION-
  AUDIT MEDIUM-03 (getFieldGrid readParam fallbacks derived, not transcribed) — CLOSED: all
  three literals gone, fallback now getParameter(id)->convertFrom0to1(getDefaultValue()), the
  same derivation getParameterDefaults uses. Bit-exactness of that derivation was COMPUTED, not
  assumed: all three ranges are linearRange (interval 0, skew 1) and the float32 round-trip is
  bit-identical for rolloff 4.0f and hullAtten 1.0f, so only blur moves (0.10 -> 0.03), which is
  the correction. Survived-to-disk confirmed across the WHOLE span, not just the top entry:
  v1.3.3 hullPoints/placeGlyph exported from roomplan.js and imported by venue.js (which no
  longer calls metresToPx), v1.3.4 LOW-01..05/07 markers all present and the LOW-06 revert
  intact. Version coherent end-to-end: CMakeLists 1.3.5 / AU + VST3 CFBundleShortVersionString
  1.3.5 / CHANGELOG / PLUGINS.md / NOTES.md. Gates re-run independently: render 51/0, geometry
  46/0, ui_layout_check 31, ui_frontend_check 42, DBAP fixture --check OK (102 cases, fixture
  unmodified), auval -v aufx OuOc OuDv SUCCEEDED. Zero-warning gate re-proved by FORCING a
  recompile of PluginEditor.cpp rather than trusting an up-to-date object: 0 warnings, 0 errors.
  Adversarial pass over the v1.3.5 diff: no regressions survived. One observation, refuted as a
  finding — on the param==nullptr branch the helper now returns 0.0f where it used to return the
  transcribed literal, but getParameter and getRawParameterValue resolve through the same APVTS
  adapter map and are null together, all three ids are registered in the layout, and even if it
  did fire rolloffToAlpha(0) = 0 yields t = 1 for every speaker (a flat field, finite, no NaN).
  NOT runtime-exercised, and cannot be: PluginEditor.cpp is excluded from the render harness by
  design, and the fallback branch is unreachable without a host writing NaN.
depth: deep
method: >
  Six parallel subsystem reviewers (DSP core, processor/params/state, editor-WebView bridge,
  venue/scene data model, UI app wiring, UI rendering/canvas) produced 48 raw findings. Every
  Critical and Warning was then re-read from disk by a second agent instructed to REFUTE it.
  3 findings were disproved, 2 Criticals were downgraded, and 4 Warnings were demoted to Info.
findings:
  critical: 1
  warning: 5
  info: 34
  total: 40
raw_findings_before_verification: 48
adversarially_verified: 14
refuted_in_verification: 3
status: issues_found
resolved: >
  CR-01 (fixed in v1.3.1, 2026-08-25 — transform-origin: 0 0 on both rules; ui_layout_check.js
  section 23 rewritten to measure rendered geometry and negative-controlled against the reverted
  CSS).
  WR-01..WR-05 (fixed in v1.3.2, 2026-08-25). WR-01 took the publish-suppression one-liner, NOT
  the seqlock: verification established that setStateInformation is the only reachable
  double-publish path and that the reviewer's "dragged rake control at mouse-move rate" trigger
  describes code that does not exist, so readVenueFromState(publish=false) closes it and the
  seqlock is recorded as optional hardening in NOTES.md Known Issues. New probes, each
  negative-controlled: render-harness CR setstate-publishes-once, geometry P2
  venue-nonnumeric-attrs, ui_layout_check sections 29 (DPR), 30 (metres echo), 31 (rejected
  commit). ui_frontend_check section 25's "no state in a completion" rule narrowed to distinguish
  ESTABLISHING state from RE-RENDERING it. Two existing gates were found to be measuring the bug
  rather than the behaviour and were repaired: section 14's metres clause, and the first draft of
  section 31.
  Remaining: IN-01..IN-34 (Info tier, opt-in via /improve-review-info).
---

# O-Octagon v1.3.0: Code Review Report

**Reviewed:** 2026-08-25 · **Depth:** deep · **Files reviewed:** 20

Propose-only review — **no source files were changed**. Resolve with `/improve-review O-Octagon`
(Critical + Warning) and `/improve-review-info O-Octagon` (the Info tail).

## Summary

O-Octagon is in good shape, and the review says so with evidence: no allocation, lock, or logging on
the audio thread; user-supplied geometry sanitised at the boundary before it reaches the DSP;
block-size invariance held; DBAP maths that matches an independent oracle. The **Handled correctly**
section below lists 99 commonly-broken things this codebase gets right.

**One finding is worth acting on before anything else.** `CR-01` breaks all eight level meters —
**on every platform, right now, including the shipped macOS build**. `.meter-arc` and `.meter-peak`
set `transform-origin: center` without `transform-box`, and for SVG elements both Chromium and WebKit
resolve that against the **viewBox**, not the glyph. Measured on the real page via the repo's own
ui-stub: every arc renders **507 px** from its speaker dot, and the peak tick orbits 509–729 px away.
The plugin's own docs call this meter row "a safety feature wearing a visualisation costume" — the
second human line of defence on the channel map, on a plugin whose top registered risk (`R1`) is that
a wrong channel map fails *silently*. That defence is currently 100% non-functional. The fix is two
CSS lines.

The reviewer originally filed this as Windows-only, reasoning that WebKit's non-spec (fill-box-like)
behaviour spared macOS. The verifier ran the identical page in WebKit 26.5 and got byte-identical
displacement — so the finding was **understated**, not overstated.

**It shipped because its own test gate is decoration.** `tests/ui_layout_check.js:1245` is titled
"eight meter arcs, at their glyph positions" but never measures a rendered position — it asserts DOM
parentage, the `r` attributes, and the `stroke-dasharray` attribute, with no `getBoundingClientRect`,
`getBBox` or `getScreenCTM` anywhere. It passes identically with the arcs 507 px off-glyph. This is
the repo's own recorded *"a probe that passes BOTH ways is decoration"* pattern. **Any fix must add a
real rendered-geometry assertion, or the gate will keep certifying the bug.**

The remaining real defects are a genuine (if bounded) **audio-thread data race** in the venue
snapshot publisher, **input-validation gaps** on hand-editable `.venue` files, and a few **UI
staleness** bugs.

### What verification changed

The adversarial pass materially improved this report, and the pattern is worth recording:

- **Three findings were disproved outright**, all resting on the same false premise — that a JUCE
  WebView drops native-fn completions when the editor is hidden. It does not. The gate is
  `Component::isVisible()`, which is the web view's **own** `visibleFlag`, and `setVisible` does not
  propagate to children; O-Octagon never hides the view (`grep -rn setVisible Source/` returns
  nothing). I re-checked this against JUCE 8.0.14 myself before accepting it. The repo memory note
  that seeded this error has been corrected.
- **Both "critical" snapshot findings were downgraded to warning** — real UB, but no crash, OOB, NaN
  or silence.
- **Four warnings were demoted to info** as theoretical-but-unreachable.

One Warning fell outside the 14-finding verification budget and is flagged inline as unconfirmed. Five
Info findings that rest on the same disproved hidden-editor premise are annotated rather than deleted,
since the "wasted work" half of each still stands.


## CRITICAL

### CR-01 — Meter arcs and peak ticks render displaced on Chromium/WebView2 (transform-origin:center with default transform-box:view-box)

**File:** `plugins/O-Octagon/Source/ui/public/css/styles.css:416`

> **Read the verification note below before the description.** The original finding called this
> Windows-only and said macOS was spared. Measurement in both engines proved that wrong — macOS is
> equally broken. The description is left as filed; the correction is in *Adversarially verified*.

`.meter-arc` (styles.css:410-418) carries `transform: rotate(-90deg); transform-origin: center;` and `.meter-peak` (styles.css:427-432) carries `transform-origin: center;` while roomplan.js:391 drives it via the SVG `transform` attribute (`rotate(peak*360)`). On SVG elements the default `transform-box` is `view-box`, so Chromium resolves `transform-origin: center` to the centre of the plan viewBox measured in the element's LOCAL (glyph-translated) space — the rotation origin lands (planW/2, planH/2) away from the glyph. Every meter arc is rotated about that far-off point and lands completely off its glyph, and the peak tick orbits off-plan. I reproduced the exact markup in Chromium 151: a glyph at (300,100) in a 400x400 viewBox rendered its `.meter-arc` at (300,500) and its attribute-rotated `.meter-peak` at (715,100) — hundreds of px from the glyph. On Windows (WebView2 = Chromium, built by this repo's CI) UI-03 — which the module's own docs call "a safety feature wearing a visualisation costume" (second human line of defence on the channel map) — is completely broken. macOS WKWebView currently renders as intended only because WebKit resolves SVG transform-origin non-spec (fill-box-like), so the shipped macOS UI is one WebKit spec-alignment away from the same breakage.

**Evidence:** styles.css 410-418: `.meter-arc { ... transform: rotate(-90deg); transform-origin: center; stroke-dasharray: 0 94.2478; }`; styles.css 427-432: `.meter-peak { ... transform-origin: center; opacity: 0; }`; roomplan.js 390-391: `tick.setAttribute("transform", `rotate(${peak * 360})`)`. Playwright/Chromium 151 measurement of this exact structure: control circle (no transform) centre (300,100); `.meter-arc` centre (300,500); `.meter-peak` centre (715,100); computed `transform-box: "view-box"`, computed `transform-origin: "200px 200px"` (viewBox centre). With the suggested fix applied, re-measured: arc (300,100), 90°-rotated peak (315,100) — both correct.

**Fix:** In `.meter-arc` add `transform-box: fill-box;` (the circle's fill-box centre IS the glyph centre, so `transform-origin: center` becomes correct in both engines). In `.meter-peak` DELETE `transform-origin: center;` entirely — for SVG elements the initial used transform-origin is 0 0 (the glyph-local origin), which is exactly the point the tick must sweep around; the attribute `rotate()` then behaves identically in WebKit and Chromium (verified in Chromium). Do not use fill-box on the peak tick — its own fill-box centre is (0,-15), which would make the tick spin in place instead of sweeping.

**Adversarially verified — CONFIRMED.** Severity adjusted to **critical**. CONFIRMED — and the finding is UNDERSTATED on platform scope.

Code re-read from disk; all cited sites are accurate:
- plugins/O-Octagon/Source/ui/public/css/styles.css:410-418 — `.meter-arc { ... transform: rotate(-90deg); transform-origin: center; stroke-dasharray: 0 94.2478; }` (no transform-box)
- styles.css:427-432 — `.meter-peak { ... transform-origin: center; opacity: 0; }`
- js/roomplan.js:391 — `tick.setAttribute("transform", `rotate(${peak * 360})`)`
- js/roomplan.js:345 — `g.setAttribute("transform", `translate(${p.x} ${p.y})`)` is what translates the glyph's local space; roomplan.js:254 sets `viewBox = 0 0 w h`. index.html:164-171 confirms arc/peak are children of that <g> with NO cx/cy, so their local origin IS the glyph centre. `grep -rn transform-box` over the whole public/ tree returns nothing — no override exists.

I recomputed the math myself before measuring: rotating local (0,0) by -90deg about (200,200) gives local (0,400) -> viewport (300,500); rotating (0,-15) by +90deg about (200,200) gives local (415,0) -> viewport (715,100). Both match the reviewer's numbers exactly.

Measured on the REAL page (served via the repo's own tests/ui-stub/serve-stub.sh, byte-identical to production), viewBox `0 0 448 560`:
- computed `transform-box: view-box`, `transform-origin: 224px 280px` (= viewBox centre) on both .meter-arc and .meter-peak
- ALL EIGHT meter arcs render 507 px from their glyph dot (spk1 dot=(80,138), arc=(24,642))
- peak tick at rotate(90/180/270) lands 522 / 729 / 509 px away; correct offset is 15 px
- with the proposed fix injected live: arc offset 0 px, tick offset 15 px — exactly correct

TWO CORRECTIONS TO THE FINDING:

1. The claim that macOS renders correctly is FALSE. I ran the identical page in Playwright WebKit 26.5 (AppleWebKit/605.1.15, the WKWebView engine) and got byte-identical numbers to Chromium 151 — same 507 px arc displacement, same 522/729/509 px tick orbit, same computed `view-box` / `224px 280px`. This is not a latent Windows-only risk one spec-alignment away: UI-03 is completely broken RIGHT NOW on the shipped macOS build too. WebKit is spec-aligned here, not fill-box-like. Severity stays critical (arguably rises) — the module's own docs call UI-03 the second human line of defence on the channel map, and it is 100% non-functional on every platform.

2. Why it shipped: the plugin's own Gate 23 in plugins/O-Octagon/tests/ui_layout_check.js:1245 is titled "eight meter arcs, at their glyph positions" but never measures a rendered position. It reads only `arc.closest('#glyph-n')` (DOM parentage), the `r` ATTRIBUTES of dot/arc, and the `stroke-dasharray` ATTRIBUTE (lines 1247-1310). No getBoundingClientRect / getBBox / getScreenCTM anywhere. It passes identically with the arcs 507 px off-glyph — the repo's own recorded "a probe that passes BOTH ways is decoration" pattern. Any fix must add a real rendered-geometry assertion or the gate will keep certifying this.

The proposed fix is correct and I verified it in both engines. `transform-box: fill-box` on .meter-arc computes origin `15px 15px` in fill-box space = local (0,0). Deleting `transform-origin` from .meter-peak does give computed `0px 0px` in BOTH Chromium and WebKit (the SVG-specific initial used value), and the warning against fill-box on the tick is right — its fill-box centre is (0,-15), which would spin it in place.

Minor suggestions: (a) prefer writing `transform-origin: 0 0` explicitly on .meter-peak rather than relying on an initial-value subtlety a later reader will "helpfully" re-add `center` to; (b) `transform-origin: 0 0` also works on .meter-arc (the circle is centred at local 0,0, so rotating about it is exactly the intended dash-start rotation) — `0 0` on both is the simpler uniform fix and needs no transform-box at all; (c) Windows/WebView2 is a genuine first-class release target — .github/workflows/build-and-release.yml:504-534 builds windows-latest with the WebView2 NuGet package.

## WARNING

### WR-01 — Venue snapshot publisher races the audio thread: a second publish inside one block rewrites the slot processBlock is holding

**File:** `plugins/O-Octagon/Source/PluginProcessor.cpp:555`

processBlock acquires `const auto& snapshot = venuePublisher.read();` and holds that reference for the entire callback (GainStage render plus the meter loop at lines 593-605). VenueSnapshotPublisher is a 2-slot double buffer whose publish() always writes the slot `1 - activeSlot`. If the message thread publishes TWICE while the audio thread is inside one block, the second publish writes into the very slot the audio thread is reading — a concurrent non-atomic write/read of ~250 bytes of floats (formal UB, practically a torn geometry: speaker positions half old/half new, trims mismatched with positions, speakerToBuffer mid-overwrite). The publisher's own comment argues 'ABA is not reachable at venue-edit rates ... a slot is only overwritten after a full release/acquire cycle has published the other one' — but this code defeats that assumption with a back-to-back double publish on one message-thread call: setStateInformation() calls readVenueFromState() (→ publishSnapshot, publish #1, line 853) and then rebuildChannelMap() (→ publishSnapshot, publish #2, line 883) microseconds apart. A session restore or host preset switch during playback is exactly this path. Consequence is bounded — speakerToBuffer entries are individually valid ints (no OOB/crash), so the damage is one control block of gains solved against inconsistent geometry (a transient level/position glitch) — but it is a real data race on the audio thread, reachable in normal operation.

**Evidence:** PluginProcessor.cpp:554-555: "// Acquired ONCE per block and held." / "const auto& snapshot = venuePublisher.read();" — held through gainStage.process (563) and the meter loop (595 reads snapshot.speakerToBuffer). VenueSnapshot.h:113-124 (read to verify this claim): "const int target = 1 - activeSlot.load (std::memory_order_relaxed); slots[(size_t) target] = newSnapshot; ... activeSlot.store (target, std::memory_order_release);" — nothing tracks which slot a reader currently holds. PluginProcessor.cpp:853 "readVenueFromState();" (whose body at 344-353 ends in publishSnapshot()) followed at 883 by "rebuildChannelMap();" (whose body at 310-342 ends in publishSnapshot()) — two publishes in one setStateInformation call. Trace: audio reads slot A → publish #1 writes slot B, flips active to B → publish #2 (same setStateInformation call, same audio block still in flight) writes slot A while the audio thread's held reference points into it.

**Fix:** Two layers. (1) Immediate mitigation: make setStateInformation publish once — give readVenueFromState() a publish=false variant when a rebuildChannelMap() follows (rebuildChannelMap already publishes unconditionally). Same for prepareToPlay for symmetry (harmless there, audio is suspended). (2) Real fix, because any two publishes straddling one block still race (e.g. a dragged rake control committing venue edits at mouse-move rate): protect the slot write with a seqlock — writer bumps an odd/even std::atomic<uint32_t> version around the slot copy; the audio thread copies the snapshot into a local (it is static_assert-ed trivially copyable, ~250 B memcpy) and retries if the version changed or is odd. Bounded retries, no lock on the audio thread, no allocation. Alternatively a 3-slot buffer where the audio thread publishes its claimed slot index (relaxed store per block) and the writer picks a slot that is neither active nor claimed.

**Corroboration:** Found independently by two reviewers from opposite ends — the publisher (`Source/Data/VenueSnapshot.h:115`) and the consumer (`Source/PluginProcessor.cpp:555`). Both verifiers confirmed it and both downgraded critical→warning for the same reason: real UB, but no crash, OOB, NaN or silence, and the dominant outcome self-heals within a block or two.

**Adversarially verified — CONFIRMED.** Severity adjusted to **warning**. VERIFIED REAL. Every mechanical claim checks out against disk.

CONFIRMED:
1. VenueSnapshot.h:129-132 read() returns `slots[(size_t) activeSlot.load(acquire)]` BY REFERENCE. PluginProcessor.cpp:555 binds it as `const auto& snapshot` and holds it through gainStage.process (563) and the meter loop (593-605, reading snapshot.speakerToBuffer at 595).
2. VenueSnapshot.h:111-125 publish() writes `slots[1 - activeSlot]` via non-atomic ~276-byte struct assignment, then release-stores the index. Nothing tracks which slot a reader holds. Trace confirmed: audio holds slot 0 -> publish #1 writes slot 1, active=1 -> publish #2 computes target = 1-1 = 0 and writes the held slot.
3. Double-publish in setStateInformation confirmed: line 853 readVenueFromState() -> publishSnapshot() (line 352); line 883 rebuildChannelMap() -> publishSnapshot() (line 341). Between them only venue.writeToState / sceneStore read+write -- microseconds, vs a ~10.7 ms block at 48k/512. If a block is in flight both publishes land inside the held window with near certainty; probability per state restore is roughly the processBlock duty cycle.
4. NO GUARD anywhere. grep over Source/ finds no getCallbackLock, no suspendProcessing, no ScopedLock. JUCE's wrappers add none either: juce_audio_plugin_client_VST3.cpp:2779-2818 and AU_1.mm:923 call setStateInformation bare. Concurrency with processBlock on a preset/setting switch during playback is real.

FULL SITE INVENTORY (reviewer cited only one):
- setStateInformation 853+883: 2 publishes, RACY.
- prepareToPlay 240-241: same pair, host-guaranteed non-concurrent -- harmless, as the reviewer noted.
- Constructor line 182: 1 publish.
- applyVenueEdit 438-450: exactly 1 publish (`if (preparedYet) rebuildChannelMap(); else publishSnapshot();`) -- never both.

TWO CORRECTIONS:

(a) REVIEWER OVERSTATES REACHABILITY. The fix section's "dragged rake control committing venue edits at mouse-move rate" is wrong. Source/ui/public/js/venue.js has exactly ONE setVenue call site, reached from commit() bound via bindNumeric (blur/Enter), not on drag. Every C++ UI entry point (setVenue, assignSpeakerOutput, applyOutputOrderPreset, loadVenue) routes through applyVenueEditChecked -> applyVenueEdit -> ONE publish. setStateInformation is the ONLY reachable double-publish-during-playback path today.

(b) REVIEWER UNDERSTATES CONSEQUENCE. GainStage.cpp:236-242 latches `lastSolvedGeneration = snapshot.generation` and early-returns when it matches with unchanged params. publishSnapshot() constructs a default-initialised VenueSnapshot (generation 0), so `slots[target] = newSnapshot` writes generation=0 and a SEPARATE store (VenueSnapshot.h:120) then writes N+2. If the audio thread observes that generation store ahead of the geometry stores -- permitted on arm64, no barrier in this non-atomic race -- it latches N+2 while solving against half-old geometry. The next block reads slot 0, sees generation N+2 == lastSolvedGeneration with identical params, and RETURNS WITHOUT RE-SOLVING. The wrong gains persist until any of the 17 params moves or another publish lands -- not "one control block". This is exactly the H1 permanent-stale class VenueSnapshot.h:66-78 claims is structurally unreachable, reached through a different door. Narrow window, secondary to the dominant transient, but it raises the worst case from "glitch" to "silently wrong gains on a restored session".

"NO CRASH" IS CORRECT: ChannelMap.cpp:30-38 validates speakerToBuffer as a 0..7 permutation and buildSpeakerToBuffer commits only on full success; the default is {0..7}. Torn entries are per-entry old-or-new, always in range. The meter loop's `ch >= numOut` guard suffices; `ch < 0` is unreachable. No OOB, no NaN (both snapshots are sanitised by publishSnapshot).

SEVERITY -> WARNING, not critical. It is a genuine data race / formal UB on the audio thread, reachable in normal operation (preset or setting recall while the transport rolls), and the fix is a one-liner. But there is no crash, no OOB, no NaN, no silence; the dominant outcome self-heals within a block or two, and the reviewer's own consequence paragraph ("bounded... a transient level/position glitch") argues against its critical label. The persistent-latch amplification pushes it up but occupies a narrow window.

CAVEAT ON THE PROPOSED FIX: rebuildChannelMap() at line 883 is gated on `if (preparedYet)`. A naive readVenueFromState(publish=false) would leave the snapshot unpublished when the host calls setStateInformation before prepareToPlay. It happens to be rescued by prepareToPlay:240-241, but that is coincidence -- the suppression must itself be conditional on preparedYet. The seqlock (or 3-slot claimed-index) proposal is sound and is the durable fix; suppressing the first publish closes the only reachable path today.

**Second verifier:** MECHANISM CONFIRMED. VenueSnapshot.h:111-125 publish() selects `target = 1 - activeSlot.load(relaxed)` with zero knowledge of the reader, and read() (129-132) returns a reference the audio thread holds for the whole block (PluginProcessor.cpp:555; used per-sample in GainStage.cpp:392 and in the meter loop at PluginProcessor.cpp:595). A second publish inside one block therefore does overwrite the slot being read — a genuine C++ data race. The ABA comment (lines 101-103) argues memory ORDERING, which is indeed a different property from reader completion, so the reviewer's critique of that comment is fair. The dirty check really is `snapshot.generation == lastSolvedGeneration` (GainStage.cpp:236-243), so the generation-tear amplification is structurally coherent.

REVIEWER'S REACHABILITY ARGUMENT IS FABRICATED. There is no speaker drag in the Room plan. roomplan.js:627-658 drags the SOURCE PUCK, writing srcX/srcY via SliderState.setNormalisedValue() — APVTS parameters, never the venue, never publishSnapshot(). roomplan.js's only venue write is deps.assignOutput from a double-click popover (one click, one publish). Venue coordinates are typed into a table and committed on blur/Enter (venue.js:324-356; the single setVenue call site is venue.js:309). "Every Room-plan speaker drag event goes applyVenueEdit -> publishSnapshot at 60-120 Hz" describes code that does not exist. applyVenueEdit itself publishes exactly once (PluginProcessor.cpp:438-450 — rebuildChannelMap publishes, else publishSnapshot).

THE RACE IS STILL REACHABLE, BY PATHS THE REVIEWER NEVER FOUND: prepareToPlay() publishes twice back-to-back (readVenueFromState at :240 -> :352, then rebuildChannelMap at :241 -> :341), and setStateInformation() does the same (:853 then :883). setStateInformation can run while processBlock is active (host preset/session load with transport rolling), so two publishes microseconds apart inside one read window are genuinely reachable. A blur-commit immediately followed by a button click (setVenue then applyOutputOrderPreset) also queues two native calls sub-millisecond apart.

BUT THE CRITICAL CONSEQUENCE DOES NOT FOLLOW. Both reachable double-publish paths build both snapshots from the SAME venue and hull, so spk, hullPts, hullCount, hullEpsCross, centroid, rigScale, bbox, rakes and trimLin are byte-identical across the two publishes — only speakerToBuffer and generation differ (publishSnapshot, PluginProcessor.cpp:397-435). A tear there cannot latch stale GEOMETRY into the solver, and speakerToBuffer is re-read from the live reference every block, so it self-heals on the next block. The claimed permanent-stale outcome further requires observing the NEW generation with OLD geometry; updateControl loads generation BEFORE the geometry in program order and publish() stores generation LAST (line 120, after the struct memcpy), so x86 TSO forecloses it and only weakly-ordered ARM leaves a window. Realistic worst case: one block rendered against a half-permuted output map during a state load — a transient blip, not the permanent wrong gain vector claimed.

Verdict: real defect, correct mechanism, wrong trigger, overstated impact. Worth fixing (seqlock-style copy-and-validate in read(), or simply coalescing the two back-to-back publishes in prepareToPlay/setStateInformation into one), but not critical.

### WR-02 — DPR change is watched via the window resize event, which never fires in a fixed-size plugin WebView

**File:** `plugins/O-Octagon/Source/ui/public/js/roomplan.js:675`

The DPR rebuild path (roomplan.js:668-675) is `window.addEventListener("resize", dprWatch)`. The editor is a fixed 1100x720 surface (PluginEditor.cpp:1348 `setSize(1100, 720)` is the single call; the page is not resizable), so the layout viewport never changes size — and a devicePixelRatio change from dragging the plugin window between a Retina and non-Retina display changes NO CSS size, which is precisely the case where no `resize` event is dispatched. The comment above the listener states the requirement correctly ("changes DPR without changing any CSS size, so the backing store has to be rebuilt") but hooks it to an event that will not fire for that case: the canvas keeps the old backing-store scale and renders blurry (Retina→non-Retina leaves a 2x store downsampled; non-Retina→Retina leaves 1x upscaled) until something else triggers relayout.

**Evidence:** roomplan.js 668-675: `const dprWatch = () => { const next = window.devicePixelRatio || 1; if (next === dpr) return; ... }; window.addEventListener("resize", dprWatch);` — the only site that updates `dpr` after construction. app.js 871-875 adds another `resize` listener for relayout, so no other path re-reads devicePixelRatio either. The `resize` event fires on layout-viewport size changes; a monitor-DPR change with an unchanged 1100x720 viewport produces none.

**Fix:** Watch DPR with the standard media-query hook and re-arm it on every fire: `function armDprWatch() { const mq = window.matchMedia(`(resolution: ${window.devicePixelRatio}dppx)`); mq.addEventListener("change", () => { dprWatch(); armDprWatch(); }, { once: true }); }` — call once at construction, keep the existing resize listener as a belt-and-braces path.

**Adversarially verified — CONFIRMED.** Severity adjusted to **warning**. CONFIRMED against disk. roomplan.js:211 `let dpr = window.devicePixelRatio || 1` is the construction read; lines 666-675 match the quoted evidence verbatim (comment names the Retina<->non-Retina drag, listener is `window.addEventListener("resize", dprWatch)`). A grep for devicePixelRatio across Source/ui/public/js/ returns exactly three sites — roomplan.js:211 (init), roomplan.js:669 (inside dprWatch), app.js:871 (a second resize listener that only calls relayout()) — so dprWatch is the sole post-construction writer of `dpr` and it sits behind `resize`. resizeCanvas() (~line 265) computes `canvas.width = Math.round(rect.width * dpr)` + `ctx.scale(dpr, dpr)` from that module-level value, so the other relayout triggers (setGeometry at app.js:478, screen switches at app.js:338-340) all re-run with the stale value and none of them heals it. PluginEditor.cpp:1348 `setSize (1100, 720)` is the only sizing call, its comment says "Fixed 1100 x 720, non-resizable (CONTEXT-3.1 D7)", and grep for setResizable/constrainer over Source/ returns nothing — the CSS viewport genuinely never changes. A device-scale-factor change with unchanged CSS viewport does not dispatch resize (WebKit keys resize dispatch on CSS viewport size and zoom factor, not backing scale factor), so on the macOS WKWebView editor the listener will not fire for the case its own comment describes; the proposed re-arming matchMedia("(resolution: Xdppx)") hook is the standard remedy. Also confirmed the existing gate cannot catch this: tests/ui_layout_check.js:242/407 builds two SEPARATE browser contexts at deviceScaleFactor 1 and 2, i.e. construction-time DPR only, never a live transition. OVERSTATED in two respects. (1) Only one direction is blurry — non-Retina->Retina leaves a 1x store upscaled 2x (genuinely blurry), while Retina->non-Retina leaves a 2x store downsampled into a 1x box, which looks fine (just wasteful); the finding claims both. (2) Impact is cosmetic only: canvas.width and ctx.scale() use the SAME stale dpr, so CSS-pixel drawing coordinates stay self-consistent — geometry, the metres readout, and puck hit-testing (which read getBoundingClientRect on the CSS box) are unaffected. Scope is the single #plan-backdrop canvas (elevation/venue are SVG; field.js blits through the same ctx), it requires a mixed-DPI multi-monitor setup, and it self-heals when the editor is closed and reopened. Real defect — a documented requirement wired to an event that cannot deliver it, making the watcher inert — but warning at the low end, not a functional break.

### WR-03 — readFloat() returns 0.0 (not the documented fallback) for present-but-non-numeric attributes, and accepts literal NaN/Inf strings unchecked

**File:** `plugins/O-Octagon/Source/Data/VenueModel.cpp:119`

readFloat only falls back when the property is ABSENT. When it is present but garbage, the var string->double conversion goes through String::getDoubleValue, which returns 0.0 for non-numeric text — so '<VENUE rakeFront="tall">' loads rakeFront = 0.0 instead of the 1.10 default, directly contradicting the header contract 'A missing OR PARTIAL node yields the §OQ4 defaults PER ATTRIBUTE — never zeros' (VenueModel.h:97). Worse, JUCE's parser explicitly recognises 'nan'/'NaN' and 'inf' strings (juce_CharacterFunctions.h:256-273 returns quiet_NaN()/infinity()), so a hand-edited .venue or corrupted session with x="nan" loads a real NaN into the model with no isfinite and no clamp anywhere in this file. The audio thread is protected downstream (publishSnapshot() sanitises every field — PluginProcessor.cpp:355-435, and it documents this gap: 'readFloat() has no clamp, no jlimit and no isfinite anywhere in VenueModel.cpp'), but the message-thread model itself carries the NaN: earHeight/normToMetres/centroid/rigScale go NaN for UI readouts and the Room plan, and writeToState/toValueTree persist the NaN back into the session and into re-saved .venue files, so the corruption is sticky across saves.

**Evidence:** 'if (! tree.isValid() || ! tree.hasProperty (prop)) return fallback; return static_cast<float> (static_cast<double> (tree.getProperty (prop)));' (lines 116-119) — the fallback branch tests presence only; the conversion path has no numeric-validity or finiteness check. Contrast SceneStore::readFromState, which sanitises the same class of input at ingestion: 'slot.w[...] = std::isfinite (raw) ? juce::jlimit (0.0f, 1.0f, raw) : 0.0f;' (SceneModel.cpp:236-237).

**Fix:** In readFloat, convert explicitly and validate: parse tree.getProperty(prop).toString() with getDoubleValue(), return `fallback` when the result is !std::isfinite() (and optionally when the trimmed text is non-numeric, to fix the garbage->0.0 case), plus a plausibility clamp for geometry (e.g. |coord| <= 10 km, |trimDb| <= 60) mirroring the kVenueTrimClampDb clamp publishSnapshot applies.

**Adversarially verified — CONFIRMED.** Severity adjusted to **warning**. VERIFIED AGAINST DISK, not the reviewer's quotes.

Code: VenueModel.cpp:114-120 is exactly as quoted — the guard is `!tree.isValid() || !tree.hasProperty(prop)`, i.e. presence only, then an unchecked var->double->float. I read the entire file: no isfinite, no jlimit, no clamp anywhere in VenueModel.cpp. Confirmed.

JUCE mechanism re-derived from source, not assumed: juce_Variant.cpp:276 stringToDouble == getString(data)->getDoubleValue(); juce_CharacterFunctions.h returns quiet_NaN() at line 260 for 'nan'/'NaN' and +/-inf at line 273 for 'inf' (the cited 256-273 span is exact), and 0.0 for any other non-numeric leading character. Both sub-claims (garbage->0.0, 'nan'->real NaN) are true.

Reachability confirmed and it is the NORMAL path, not exotic: getStateInformation serialises via state.createXml() (PluginProcessor.cpp:816-827), so on restore every ValueTree property is a STRING var — getDoubleValue is always the conversion used. Two doors carry user-controlled text into readFloat: setStateInformation->readVenueFromState, and venuefile::load (VenueFile.cpp:115), whose only validation is structural (malformedRoot = wrong root tag or <8 SPEAKER children) with no numeric check. Hand-edited .venue files are an anticipated case here (VenueModel.cpp:218-220 explicitly supports reordered children in a hand-edited file), so the precondition is not purely theoretical.

Stickiness confirmed: toValueTree writes the float back unchecked; String(double,int) -> NumberToStringConverters::StackArrayStream::writeDouble -> ostream << NaN -> "nan", which getDoubleValue re-reads as NaN. The corruption loop closes across saves.

The strongest evidence the reviewer did NOT cite: PluginEditor.cpp:112-127 finiteOr() carries the comment "publishSnapshot()'s sanitiser is downstream of the venue model, and a NaN that reached VenueModel would be stored before it was ever clamped." The project already identified this exact hazard and closed the UI write door with std::isfinite, closed the scenes door (SceneModel.cpp:236-237, quoted accurately) and closed the audio door — but left the file/session door open. That asymmetry is the real defect.

Where the finding OVERSTATES (why not critical):
1. Audio is genuinely safe. publishSnapshot (PluginProcessor.cpp:385-435) sanitises every snapshot field — spk, trimLin (isfinite then jlimit +/-kVenueTrimClampDb before decibelsToGain), hullPts, hullEpsCross, centroid, rigScale, all four bbox rails, rakeFront, rakeRear. Nothing non-finite reaches the DSP or a SmoothedValue.
2. UI impact is bounded. juce_JSON.cpp:500-508 writes `null` for non-finite doubles, and the WebView bridge serialises through JSON::toString (juce_WebBrowserComponent.cpp:423). The page receives VALID JSON with nulls — blank/zero readouts and a mis-drawn Room plan, not a parse failure or crash.
3. The "directly contradicting the header contract" claim is a stretch. VenueModel.h:97 scopes "missing OR PARTIAL node ... never zeros" to ABSENT attributes; readFloat's own doc-comment says "falling back when the property is absent", which is accurate. A present-but-garbage attribute is outside that contract's scope rather than a violation of it.
4. Trigger requires a hand-edited or externally corrupted file/session — the plugin never writes non-numeric text itself (except in the NaN feedback loop, which needs a NaN in first).

Net: a real, correctly-located defense-in-depth gap at the one un-sanitised ingestion site, with genuine persist-across-saves stickiness and a documented sibling guard idiom it fails to apply. No audio-path consequence and a corrupt-input precondition keep it at warning, not critical. The proposed fix (isfinite check returning fallback, plus a plausibility clamp) is the right shape and matches the existing finiteOr/SceneStore idiom; note the garbage->0.0 half additionally needs an explicit numeric-text check, since getDoubleValue cannot distinguish "0" from "tall".

### WR-04 — Footer metres readout goes stale when srcX/srcY change by any path except a puck drag

**File:** `plugins/O-Octagon/Source/ui/public/js/app.js:746`

renderMetres() (the footer 'Source … m' readout, UI-02 criterion 5) is invoked from exactly three places: end of init (877), refreshGeometry (492), and roomPlan's onSourceMoved callback (746). roomplan.js calls deps.onSourceMoved() ONLY inside the puck's pointermove handler (roomplan.js:654); the parameter-echo path adds only renderPuck as a listener (roomplan.js:663-664 — comment: 'Host automation and the srcX / srcY sliders in the controls column both arrive here'). Consequence: dragging the ctl-srcX/ctl-srcY range sliders, host automation, a preset load (which re-centres srcX/srcY via loadPreserving), or dblclick-reset moves the puck and updates val-srcX — but the footer metres readout keeps the old position until the next venue change. This is the exact staleness class the file otherwise closes (pattern_webview_one_shot_state_push_stale_on_preset_load applied to a derived readout).

**Evidence:** app.js:746 `onSourceMoved: renderMetres,` is the only live-update wiring; roomplan.js:654 `if (typeof deps.onSourceMoved === "function") deps.onSourceMoved();` sits inside `puck.addEventListener("pointermove", ...)`; roomplan.js:663-664 `srcX.state.valueChangedEvent.addListener(renderPuck);` — renderPuck never calls onSourceMoved. renderMetres reads the slider states directly (app.js:364-368), so it would be correct if only it were called.

**Fix:** In init(), after bindSlider runs, subscribe the readout to the echo: `for (const id of ["srcX","srcY"]) sliders.get(id).state.valueChangedEvent.addListener(renderMetres);` — same pattern already used for FIELD_INPUT_IDS at app.js:839-842. (Marking dirty and rendering on the 2 Hz tick would also do.)

**Adversarially verified — CONFIRMED.** Severity adjusted to **warning**. CONFIRMED, with one incorrect supporting example.

Verified against disk:
- app.js:355-370 renderMetres() derives its text from sliders.get("srcX"/"srcY").state.getNormalisedValue() via normToMetres(), so it is a function of srcX/srcY and must be re-run when they move.
- renderMetres has exactly three call sites (definition 355; calls at 492, 746, 877). Nothing else in the plugin references #readout-metres.
- app.js:492 sits inside refreshGeometry(), which applyStatus() calls ONLY when status.venueGen != cachedVenueGen (app.js:530-531). pollStatus does nothing else with the metres, so there is no 2 Hz self-heal.
- roomplan.js:654 `if (typeof deps.onSourceMoved === "function") deps.onSourceMoved();` is the only onSourceMoved call in the module, and it is inside puck.addEventListener("pointermove", ...). The puck's full listener set is pointerdown/pointermove/pointerup/pointercancel/lostpointercapture (627-659) — no dblclick path either.
- roomplan.js:663-664 attaches renderPuck to srcX/srcY valueChangedEvent; renderPuck (445-456) only repositions the puck element.
- app.js bindSlider (241-313) attaches a render() that writes val-<id> only, plus a dblclick reset that writes the parameter — neither calls renderMetres.
- ctl-srcX and ctl-srcY are real, enabled range inputs (index.html:253, 258), so the "slider in the controls column" path is live, as is host automation and keyboard stepping on those sliders.

Two independent pieces of in-repo evidence make the defect unambiguous:
1. elevation.js:343 does exactly the fix the reviewer proposes — `srcY.state.valueChangedEvent.addListener(() => drawMarker())` — so the elevation strip stays live on the echo while the footer does not. The footer is out of step with its own sibling module.
2. index.html:250, the Source X tooltip, literally reads "the metres readout below is live." Dragging that exact slider is the case where it is not.

Where the finding OVERSTATES: the "preset load (which re-centres srcX/srcY via loadPreserving)" example is backwards. Source/Data/PresetPolicy.h:68-70 lists srcX, srcY, srcZ and w1..w8 in kPreserved, and PluginEditor.cpp:851-863 documents that loadPreserving exists precisely to STOP the re-centring that raw applyPresetJson caused. A preset load therefore holds the source still and is not a staleness path. Scene apply is also not one — PluginEditor.cpp:1076-1114 shows applyScene writes only the eight weights. Dropping those two leaves the real paths: ctl-srcX/ctl-srcY pointer drag, keyboard step, their dblclick reset, and host automation of srcX/srcY.

Severity: warning is right. It is display-only (no audio/DSP effect), and it self-heals on the next venue change or editor reopen (init awaits refreshGeometry before its renderMetres at 877, so geometry is non-null there). But it shows a plausible WRONG source position in metres rather than a blank, on a plugin whose footer is the only metres readout, and it breaks a stated acceptance criterion (UI-02 criterion 5) plus the tooltip's own promise. Not critical.

Proposed fix is sound and matches the file's existing idiom at app.js:839-842 (the FIELD_INPUT_IDS echo subscription).

### WR-05 — venue.js commit(): pending.clear() after a C++-rejected setVenue leaves the inputs displaying text the model has silently reverted

**File:** `plugins/O-Octagon/Source/ui/public/js/venue.js:319`

commit() fires setVenue fire-and-forget and synchronously clears `pending`. If C++'s applyVenueEditChecked backstop rejects (result.ok === false — reachable when the page's committed-set permutation check passes but the negotiated set changed underneath it, e.g. a host renegotiation or session restore racing the edit), the JS adds an is-colliding mark, but: the typed text is still displayed in the input, `pending` no longer holds it, so the model's value for that field is the OLD committed one. A rejected setVenue does not bump venueGen, so no geometry refresh arrives and paintFields() never repaints — the desync between what the operator sees and what the next commit will send persists indefinitely. The next commit (triggered by blurring any other field) silently sends the committed value while the screen shows the rejected text.

**Evidence:** venue.js:309-320: `nativeFn("setVenue")(payload)\n  .then((result) => { if (... result.ok === false) { ... rows[marked].fields.label.classList.add("is-colliding"); } })\n  ...\npending.clear();` — pending is cleared unconditionally, and paintFields() runs only from setGeometry (line 585), which only runs when venueGen moves.

**Fix:** In the ok:false branch, repaint the table from committed so display matches model: call paintFields() after removing the affected pending entries (or simply call paintFields() unconditionally in that branch — it skips focused/pending fields by design). Alternatively keep the rejected field's text in `pending` (don't clear it before the advisory resolves) so the hold-and-mark semantics of the label column stay truthful.

**Adversarially verified — CONFIRMED.** Severity adjusted to **warning**. CONFIRMED against source. venue.js:309-319: setVenue is fire-and-forget and pending.clear() runs unconditionally outside the .then; the ok:false branch only adds "is-colliding" to a label element and never restores any input.value. paintFields() (venue.js:213) has exactly one caller, setGeometry (venue.js:585), which has exactly one caller, refreshGeometry() in app.js:479, gated on status.venueGen !== cachedVenueGen (app.js:530-531). relayout() (venue.js:590) only calls drawMini(), so a tab switch does not repaint the table either. On the C++ side applyVenueEditChecked (PluginProcessor.cpp:452-484) returns false BEFORE applyVenueEdit(), so publishSnapshot() never runs; venueGeneration is venuePublisher.getGeneration() (PluginProcessor.h:141) and publishSnapshot()'s only callers are the ctor, readVenueFromState(), rebuildChannelMap() and applyVenueEdit(). Nothing on the parameter path bumps it, so a rejected commit produces no refresh and the desync does persist. buildPayload() (venue.js:171-174) falls back to committed base.x/y/z/trimDb for any field not in pending, so the next commit does send the old value while the input shows the rejected text.

Reachability is BROADER than the reviewer claimed. isBusesLayoutSupported (PluginProcessor.cpp:186-221) explicitly admits mono/stereo output (SAFE mode, load-bearing for the AU (n,1)/(n,2) configs), where getMainOutputChannelSet().size() != 8 makes buildSpeakerToBuffer fail with notEightChannels on EVERY commit. Nothing in index.html or app.js disables the venue inputs when safeMode/mapInvalid is set. And in that case speakerIndex is -1, so Number.isFinite(marked) && marked >= 0 is false and not even the is-colliding mark appears -- zero feedback. The session-restore route is also real: readVenueFromState() (PluginProcessor.cpp:344-353) publishes without validating, and labelType() (VenueModel.cpp:349-361) resolves strings through getChannelTypeFromAbbreviation, so committed labels can be unresolvable while JS's string-level collidingRows() still passes.

Nuances: the JS-detected collision path is unaffected -- commit() returns at line 302 before pending.clear(), so the documented hold-and-mark swap reachability (P53) is intact there; the bug is confined to the C++-reject path, where it inverts that same intent (label text held on screen, silently dropped from the model). It self-heals on the next successful venue change since any venueGen bump repaints from committed, but in SAFE mode / mapInvalid no commit can ever succeed, so the window is unbounded. The proposed fix is sound: paintFields() in the ok:false branch skips pending (now empty) and document.activeElement by design.

Severity stays warning, not critical: nothing invalid reaches the audio path (the guard applies nothing), no crash -- but the operator is shown room measurements the plugin does not hold, in a live-hall calibration tool, with no feedback at all in the most reachable case.

## INFO

Opt-in tier — low-risk cleanup, documentation drift, and robustness parity. Not adversarially verified unless noted. Swept by `/improve-review-info O-Octagon`.

### IN-01 — Air-filter EXIT (active→bypass) is an unsmoothed binary switch — click risk on jump automation

**File:** `plugins/O-Octagon/Source/DSP/GainStage.cpp:446`

The air filter's ENTRY edge got elaborate bit-exact treatment (seed s=x so v=G*(x-s)=0, GainStage.cpp:317/402-412), but the EXIT edge (airActive true→false) simply flips the per-sample ternary from the filtered signal to the dry one at the next control boundary. For CONTINUOUS puck motion this is nearly transparent because d_hull→0 drives the cutoff to the ceiling before the flip. But for a JUMP — scene recall, stepped automation, or a preset change moving the source from deep outside the hull (cutoff can be at kAirFloorHz=500 Hz) to inside — the output steps from LP500(x) to x in one sample. The discontinuity equals the full removed HF content of the signal, exactly the class of click P27's measurements (430%/521% slew) were run to eliminate on the entry side. Every other element of a position jump (all 16 speaker gains, the trims, the z-cue) is smoothed by the 5 ms SmoothedValue ramps; this bypass flip is the only unsmoothed element on the signal path.

**Fix:** On the true→false edge (was-active && !now-active && airAmount still > 0), keep the filter running for one more control block with the cutoff already set to the ceiling (updateControl already sets it unconditionally, line 295), then bypass — the residual step shrinks to (1−G)·(s−x) ≈ −17 dB of the HF delta at 44.1 kHz. Alternatively crossfade fL between filtered and dry over the 64-sample chunk on that edge. Either keeps QUAL-03 intact because the edge is keyed to the absolute control grid.

### IN-02 — v1.3.0 range/scale reworks shift pre-existing host AUTOMATION lanes — the preset migration hook cannot reach them

**File:** `plugins/O-Octagon/Source/PluginProcessor.cpp:101`

rolloff widened 3–6 → 3–12 dB/2x and width 0–6 → 0–12 m; blur's kBlurScale tripled (0.5 → 1.5) with the default rescaled to compensate. Sessions are safe (APVTS state stores denormalised values) and user/factory presets are re-mapped by the editor's preset-manager v1.0.6 migration hook — but host automation lanes store NORMALISED 0..1 values and no code path touches them. A pre-1.3.0 project with a written rolloff lane at 0.333 (= 4.0 dB/2x on the old range) now plays back at 3 + 0.333·9 = 6.0 dB/2x; a width lane at 0.5 (3 m) now means 6 m; every blur lane now drives triple the radius even though the range digits are unchanged. All ParameterID version hints remain { id, 1 } (line 58), and the repo's own rule is that each param range move needs its own migration version gate (pattern_preset_migration_per_param_version_gate) — presets got one, automation did not and structurally cannot through the same hook.

**Fix:** There is no clean in-code fix for normalised automation data already written by hosts; pick one deliberately: (a) accept and document — add a v1.3.0 release-note/UI-facing caveat that existing rolloff/width/blur automation plays back hotter/wider (this matches how the CHANGELOG already documents the preset side); or (b) if pre-1.3.0 projects in active use matter, introduce new parameter IDs (rolloff2/width2) so old lanes orphan visibly instead of silently re-scaling, keeping the old IDs as hidden compat parameters. Given v1.3.0 has shipped, (a) is the pragmatic choice — but decide it explicitly rather than by omission.

### IN-03 — VenueFile::load() validates SPEAKER child COUNT but not index coverage — duplicate/shifted @index files load partly-default with LoadResult::ok

**File:** `plugins/O-Octagon/Source/Data/VenueFile.cpp:101`

Rule 3 of this API ('fewer than 8 SPEAKER children is rejected OUTRIGHT') is implemented as a bare child count. VenueModel::readFromState locates speakers by @index (1..8), so a file with 8 SPEAKER children whose indices are duplicated (two @index="5", no @index="4") or off-by-one (2..9) passes the count gate, yet the unclaimed indices silently keep their §OQ4 defaults — and the positional fallback is disabled for them because those children DO carry an index property (VenueModel.cpp:227 requires '! positional.hasProperty (propIndex)'). Whitespace/zero-padded hand edits ('@index="01"', '" 1"') fail too: the var comparison for a string property against int coerces via stringEquals — intToString(1)=="1" matches, but "01" != "1". Result: LoadResult::ok, banner silent, and a room that is partly measured and partly placeholder — exactly the outcome VenueFile.h:48-51 says is 'unrecoverable' in a hall and that this TU exists to prevent. The defaults being 'legible' §OQ4 values softens this only if the operator notices them.

**Fix:** After the count check, verify coverage: for i in 1..8, require exactly one SPEAKER child whose @index string, trimmed, parses to i (getChildWithProperty plus a duplicate scan); return LoadResult::malformedRoot (or a new badIndices result surfaced by describe()) on any gap or duplicate.

### IN-04 — SIDES degenerate-axis guard uses `hx > 0.0f`, not the kMinSpan guard the comment claims it shares

**File:** `plugins/O-Octagon/Source/Data/SceneModel.cpp:141`

scenes::resolve guards the half-span normalisation with a strict positivity test, while the comment two lines above claims it is 'the same guard VenueModel applies to the audience plane and the bbox denormalisation, arriving a third time (QUAL-02)'. It is not the same guard: plane::earHeight/normToMetres treat any span below kMinSpan = 1e-6f as degenerate (VenueGeometry.h:47, 64, 84-85), but resolve divides by any hx > 0. For a rig whose x-coordinates differ by, say, 1e-8 m (float noise after an import or a nudge), normToMetres pins the axis as degenerate while resolve computes nx = |p.x - cx| / 5e-9 ~ 1e8 and decides SIDES membership from rounding noise — two 'identical' guards giving contradictory answers on the same venue. Not on the audio path (membership feeds user-initiated weight writes), but it is an operator-facing wrong scene on a degenerate rig and a documented-invariant violation.

**Fix:** Use the shared constant: 'constexpr float minHalf = plane::kMinSpan * 0.5f;' and test 'hx > minHalf' / 'hy > minHalf' (SceneModel.h already includes Vec.h; add VenueGeometry.h or hoist kMinSpan), so a degenerate axis contributes 0 under exactly the same threshold as the rest of the geometry.

### IN-05 — v1.3.0 z-cue std::pow bypasses countedPow — pow-budget instrumentation under-counts by 2 per control block

**File:** `plugins/O-Octagon/Source/DSP/GainStage.cpp:69`

zCueGain() calls std::pow directly rather than dbap::countedPow, so under OOCTAGON_INSTRUMENT the powCalls counter — whose doc comment (DbapSolver.h:60) reads 'std::pow calls. Expected EXACTLY 32 per control block' — misses the two z-cue exponentiations. The real per-control-block std::pow count on the audio thread is 34, and the CHANGELOG claim 'the pow budget is now exactly 32 per solve pair' is understated. HullProcessor's exp2 has an explicit comment justifying its exclusion from the counter (HullProcessor.h:135-137); the z-cue pow has no such acknowledgement, so the exclusion reads as an oversight rather than a decision. This is the test-fixture-drifts-silently class: the executable form of the PERF budget no longer covers all pow calls the feature added.

**Fix:** Either route the z-cue exponent through dbap::countedPow and update probes BJ/BL from 32 to 34, or add a one-line comment at zCueGain mirroring HullProcessor.h's exp2 note, stating that this pow is deliberately outside the counted §3.3.5 budget and why.

### IN-06 — Stale pre-v1.3.0 comment: 'blur = 1 → half the RMS rig radius' contradicts kBlurScale = 1.5

**File:** `plugins/O-Octagon/Source/DSP/DbapSolver.cpp:36`

v1.3.0 changed kBlurScale from 0.5 to 1.5 (blur = 1 now reaches 1.5× the RMS rig radius — the header comment at DbapSolver.h:169 and the v1.3.0 rationale block at DbapSolver.h:172-177 are correct), but this .cpp comment still describes the old 0.5 mapping. Given this repo's regression sensitivity, a future reader reconciling the two comments could 'restore' 0.5 and silently undo the v1.3.0 audibility fix — the exact hazard the header's own migration note warns about.

**Fix:** Change the parenthetical to '(blur = 1 → 1.5× the RMS rig radius)' to match kBlurScale and the header.

### IN-07 — Stale pre-v1.3.0 comment: rolloffToAlpha 'exposed extremes' still quotes the old 3–6 dB range

**File:** `plugins/O-Octagon/Source/DSP/DbapSolver.cpp:43`

v1.3.0 widened rolloff to 3–12 dB per doubling (PluginProcessor.cpp:101 `linearRange (3.0f, 12.0f)`), but the comment above rolloffToAlpha still names R = 6 → a = 0.99658 as the exposed maximum. The actual exposed maximum is R = 12 → a ≈ 1.99316. The arithmetic itself is correct (a = R / (20·log10 2)); only the range documentation is stale.

**Fix:** Update to 'R = 3 → a = 0.49829, R = 12 → a = 1.99316'.

### IN-08 — fadeRatio division executes before its degenerate-rig guard

**File:** `plugins/O-Octagon/Source/DSP/SourceShaper.cpp:52`

`bLen / rFade` is evaluated unconditionally; when rFade == 0 (rigScale 0, all speakers coincident) this computes inf, or NaN when bLen is also 0, and the ternary on the next line then discards the value via its `: 0.0f` branch. Correct today under IEEE semantics (the repo builds without -ffast-math), and the guard itself is right — but the computed-then-discarded inf/NaN is refactor bait: hoisting, an fp-model change, or a UBSan float-divide-by-zero check would each surface it, and the pattern invites someone to 'simplify' the ternary in a way that consumes the poisoned value.

**Fix:** Move the division inside the guarded branch: `const float fade = rFade > kBearingEpsilon ? juce::jmin (bLen / rFade, 1.0f) : 0.0f;` (or the branchless std::min equivalent to keep the no-JUCE constraint of this TU).

### IN-09 — NaN-guard recovery point is chunk-cadenced, so post-poisoning output varies with host block size

**File:** `plugins/O-Octagon/Source/DSP/GainStage.cpp:476`

The guard runs at the end of every renderChunk, and chunk length is min(host-block remainder, control-grid remainder) — a 32-sample host block yields 32-sample chunks while a 512-sample block yields 64-sample chunks. After a non-finite input poisons the filter, the airL.reset() therefore lands at a different absolute sample depending on how the host chopped the buffer, and up to 63 non-finite samples ship to the host before recovery. Strict block-size invariance (QUAL-03) is broken in this pathological case only; normal operation is unaffected, and the design notes acknowledge the once-per-block granularity as accepted risk R6. Recorded so the QUAL-03 claim is not read as unconditional.

**Fix:** No change required. If ever tightened, run the finiteness check against the grid (accumulate per-control-block rather than per-chunk) so the reset lands at an absolute-sample-keyed position; document that R6 excludes non-finite input from the QUAL-03 invariance claim.

### IN-10 — SAFE mode duplicates the R feed on every channel above 1 in wide-buffer (F3) configurations

**File:** `plugins/O-Octagon/Source/DSP/GainStage.cpp:555`

In SAFE mode with numOut between 3 and 7 (the F3 case: valid map, buffer narrower than 8), channels 1 through numWrite−1 all receive sR — a 6-channel bus carries the right input on five lanes at unity. The block comment documents 'the dry input at unity' and the outGain inertness at length but never states this fan-out. Probably intended (the alternative — silence on channels ≥ 2 — is equally defensible), but it is the one behavior in this exhaustively-annotated function that carries no rationale, and at unity gain on five speakers it is a loud default.

**Fix:** Either write silence to channels ≥ 2 (`ch == 0 ? sL : ch == 1 ? sR : 0.0f`) or add a comment stating the fan-out is deliberate and why, so the next reader does not have to reverse-engineer whether it was considered.

### IN-11 — kDenomEpsilon guard creates a full-scale→silence cliff as total weighted energy crosses 1e-20

**File:** `plugins/O-Octagon/Source/DSP/DbapSolver.cpp:83`

Because of the Σv²=1 normalisation, a single active speaker holds EXACTLY unity gain (v = k·w·t = 1) no matter how small its weight — until w²t² drops below kDenomEpsilon, at which point the early return snaps all gains to hard zero. Concretely: one speaker at d≈10 m, a=0.66 (default rolloff) gives t≈0.21, so w=2e-9 still yields full-scale output while w≈4e-10 yields silence — a 0 dB → −∞ step across one control block, masked only by the 5 ms SmoothedValue ramp. This is inherent to DBAP weight semantics (weights choose distribution, not level — documented as DSP-05) and a host automation sweep crosses the window in well under a millisecond, so it is not a practical defect; recorded because the threshold placement (float 1e-20, above the FTZ denormal floor) is load-bearing and must not be casually changed.

**Fix:** No change required. If a fader-like weight behavior is ever wanted, it is a design change (scale output by a function of Σw rather than pure normalisation), not an epsilon adjustment — note this beside kDenomEpsilon so the constant is not tuned to 'fix' the cliff.

### IN-12 — numOut = buffer.getNumChannels() overcounts outputs in the (2-in, 1-out) SAFE config — meter 2 lights from a channel that reaches no output

**File:** `plugins/O-Octagon/Source/PluginProcessor.cpp:530`

Bounding the block by buffer.getNumChannels() instead of getTotalNumOutputChannels() is the right call for the F3 hazard (the comment is correct: the accessor lies on 3–7-channel devices, the buffer does not). The cost is the opposite corner: JUCE's processBlock buffer carries max(inputs, outputs) channels, so in the negotiated stereo-in/mono-out SAFE config — which isBusesLayoutSupported() admits and which JUCE bakes into AUChannelInfo as (2,1) — the buffer has 2 channels and numOut reads 2 while the host consumes only channel 0. GainStage is handed numOut=2 and writes channel 1 (harmless; the host ignores it), and the unmapped meter loop (`ch = i`, guarded only by `ch >= numOut`) then meters channel 1: the speaker-2 meter displays signal that reaches no physical output. Cosmetic-only, and only in mono SAFE with a stereo input, but the meters are explicitly positioned as "a second human line of defence" so a lane that lights without a corresponding output slightly undermines that.

**Fix:** In the meter loop only, additionally clamp against the real output-bus width for the unmapped arm: `const int metered = juce::jmin (numOut, getTotalNumOutputChannels());` and use it as the guard bound when !mapped (when mapped, numOut==8 and the map is a validated permutation, so nothing changes). getTotalNumOutputChannels() is safe to trust in the SAFE-fold direction — its F3 lie is only ever an overcount on 8-channel negotiation.

### IN-13 — VerifyPing::getState() assembles its snapshot from five independent atomics — the tuple can tear across a phase step

**File:** `plugins/O-Octagon/Source/DSP/VerifyPing.cpp:126`

getState() performs five separate acquire loads while the audio thread's publish() performs five separate release stores per chunk. A poll landing between stores can pair, e.g., the NEW speaker number with the PREVIOUS step's elapsedMs/remainingMs, or active=true with speaker already zeroed for the gap. For a 100 ms UI poll on a 1.2 s/0.4 s cycle this is a one-frame cosmetic inconsistency at worst, and the design (POD ints, no juce::String across threads) is otherwise exactly right — noting it so a future consumer doesn't treat the State struct as an atomic unit (e.g., asserting elapsedMs monotonicity per speaker in a probe would flake).

**Fix:** Leave as is for the current UI (document the tear in the State struct's comment), or if a consumer ever needs coherence, pack the poll surface into one std::atomic<uint64_t> (mode:2 | speaker:4 | elapsedMs:29 | remainingMs:29 fits) or version it with an even/odd counter the reader re-checks.

### IN-14 — prepareToPlay touches the ValueTree and plain flags that the message thread also uses — safe only while hosts call it on (or synchronised with) the message thread

**File:** `plugins/O-Octagon/Source/PluginProcessor.cpp:240`

prepareToPlay() calls readVenueFromState() — which reads apvts.state (a juce::ValueTree, not thread-safe) — and writes the plain `preparedYet` bool that setStateInformation() and applyVenueEdit() read on the message thread with no synchronisation. The constructor comment (lines 166-170) addresses only the MUTATION half ('readVenueFromState() and writeToState() are never the ones mutating the ValueTree from whatever thread a host chooses to call prepareToPlay() on') — but a concurrent read against a message-thread applyVenueEdit()/writeToState() is still a race if a host calls prepareToPlay off the main thread while the editor commits a venue edit. In practice mainstream hosts either call prepareToPlay on the main thread or fence it against UI activity, and every JUCE plugin in this repo makes the same assumption — recording it here because this plugin uniquely does heavy tree work (venue read, hull rebuild, map build, two snapshot publishes) inside prepareToPlay rather than the usual buffer sizing.

**Fix:** No change needed today. If ever hardening: make preparedYet a std::atomic<bool>, and have prepareToPlay defer the venue/tree work to the message thread via a flag consumed there when the calling thread is not the message thread (juce::MessageManager::existsAndIsCurrentThread()).

### IN-15 — meterPeak is never cleared in prepareToPlay — a pre-reconfiguration peak can survive into the new configuration's first UI poll

**File:** `plugins/O-Octagon/Source/PluginProcessor.cpp:243`

prepareToPlay() re-derives the map, safeMode, ping and gain stage but leaves the eight meterPeak atomics holding whatever the last block before reconfiguration wrote. After a sample-rate change, layout renegotiation, or transport-stop/prepare cycle with the editor closed, the first readAndZeroMeters() reports peaks measured under the OLD map/layout attributed to the new one. Self-healing within one 33 ms meter frame and read-and-zero by design, so purely cosmetic — but for a plugin whose meters are explicitly a channel-map verification aid, a one-frame wrong-lane light after a layout change is the exact artefact an operator might act on.

**Fix:** Add to prepareToPlay: `for (auto& m : meterPeak) m.store (0.0f, std::memory_order_relaxed);` — message/prepare thread, audio suspended, so no ordering subtleties.

### IN-16 — Pending-promise map grows without bound while the editor stays hidden with the page kept loaded

**File:** `plugins/O-Octagon/Source/ui/public/js/juce/index.js:45`

PromiseHandler deletes a promises-map entry only when the completion event arrives. While the editor is hidden (page kept loaded), the 2 Hz status poll keeps firing (pollStatus is deliberately guard-free per app.js:456-460) and the meters guard re-issues every 5 ticks; every completion is dropped, so each call leaks one {resolve, reject} entry plus the never-settling promise, forever. The in-file comment calls the leak 'bounded and acceptable' for a dropped tick, but under continuous hiding it accrues at roughly 2/s (status) + ~6/s (meters deadline re-issues), i.e. tens of thousands of entries per hour hidden. Reclaimed only when the editor (and page) is destroyed.

**Fix:** Low priority. If addressed: pause both intervals on document.visibilitychange (hidden -> clearInterval, visible -> restart + immediate pollStatus), which also removes most of the dropped-completion traffic that the deadline guards exist to survive.

⚠️ **Premise partly disproved.** This finding assumes a hidden editor silently drops native-fn completions. Verification established that it does not in this plugin — the gate is the web view's own `isVisible()` flag, which O-Octagon never clears (see *Refuted in verification*). Whatever part of this finding depends on a never-settling completion cannot fire today; any part that is simply wasted work still stands.

### IN-17 — Typing in a venue label field before the first geometry payload arrives throws TypeError on null `committed`

**File:** `plugins/O-Octagon/Source/ui/public/js/venue.js:365`

bindLabel's input listener calls applyLabelMarks(collidingRows()) on every keystroke, and collidingRows() dereferences `committed.speakers` (line 194) with no null guard. `committed` stays null until the first setGeometry() call, which happens only after init()'s awaited getParameterDefaults round trip plus the first refreshGeometry completes — a window of a few hundred ms (longer if the first getVenueGeometry completion is dropped, up to the 3 s guard deadline). A keystroke in that window throws inside the listener; the exception kills only that handler invocation, but the label field then behaves dead until geometry arrives. commit() and the Escape handler DO guard committed === null; the input handler is the one path that does not.

**Fix:** First line of collidingRows(): `if (committed === null) return new Set();` (labelOf() is then also safe since it is only reached through collidingRows/buildPayload, and buildPayload is behind commit()'s guard).

### IN-18 — Stale native-function-count documentation: header says 13, cpp says 3, app.js says 18 — the surface is 22

**File:** `plugins/O-Octagon/Source/PluginEditor.h:37`

Three load-bearing-sounding comments state historical surface counts: PluginEditor.h:37 'NATIVE-FUNCTION SURFACE IS EXACTLY THIRTEEN (PLAN-3.2 P65)' and its 13-name list omits the 9 later functions; PluginEditor.cpp:25 '17 WebSliderRelay bindings, THREE native functions'; app.js:47 'SURFACE IS EXACTLY EIGHTEEN (PLAN-3.3 P81)'. The real registered/called set is 22 (18 + assignSpeakerOutput/applyOutputOrderPreset at v1.1.0 + set/getTooltipsEnabled at v1.2.0 — CHANGELOG confirms 'bridge surface 20 -> 22'). The grep-diff gate was updated (ui_frontend_check §3 passes at 42/42), so this is comment drift only — but these comments explicitly advertise themselves as THE count ('THE COUNT IS LOAD-BEARING'), so a future reader auditing the bridge against the header's list would wrongly conclude 9 functions are unregistered.

**Fix:** Update the three comment blocks to 22 (or reword to 'see the registration list; the count is asserted by ui_frontend_check §3' so the prose can never drift again).

### IN-19 — saveVenue/loadVenue silently destroy an already-open FileChooser if the second button is reachable while the first dialog is up

**File:** `plugins/O-Octagon/Source/PluginEditor.cpp:719`

Both handlers do `venueChooser = std::make_unique<juce::FileChooser>(...)` unconditionally. The header comment (PluginEditor.h:159-162) asserts 'they cannot be open at once', but launchAsync on macOS presents a non-app-modal panel in some hosts, leaving the WebView clickable; a second click then destroys the live FileChooser mid-dialog. This is SAFE (verified in JUCE source: FileChooser::~FileChooser() just does `asyncCallback = nullptr`, so the old completion lambda — and the captured native-fn `complete` — is discarded, never invoked, matching this page's advisory-completion design and causing no UAF), but the first dialog is dismissed out from under the user and its JS promise never settles. Robustness nit, not a crash.

**Fix:** Guard re-entry: `if (venueChooser != nullptr) { complete(makeResult(false, "busy", -1)); return; }` at the top of both handlers, and reset `venueChooser` (via MessageManager-safe reset) at the end of each completion lambda.

### IN-20 — PresetPolicy doc table still shows the pre-v1.3.0 blur column — a reconciliation edit would silently triple every factory blur

**File:** `plugins/O-Octagon/Source/Data/PresetPolicy.h:87`

The authoritative-looking table in the factoryDefs doc comment lists blur = 0.00 / 0.10 / 0.18 / 0.35 / 0.55 / 0.80, but the shipped rows (lines 140-148) carry the v1.3.0 ÷3 values 0.00 / 0.03 / 0.06 / 0.12 / 0.18 / 0.27 (kBlurScale 0.5 -> 1.5, radii preserved per the row comment at 135-139). The table and the code now disagree in the highest-regression-risk area of v1.3.0; a future editor 'fixing' the rows to match the table would silently triple every factory preset's blur radius and break Concert Default's exact-default identity (blur default is 0.03, PluginProcessor.cpp:106).

**Fix:** Update the table's blur column to the post-1.3.0 values (0.00 / 0.03 / 0.06 / 0.12 / 0.18 / 0.27) and note the ÷3 rescale beside it, or delete the blur column and point at the rows as the single source.

### IN-21 — SceneStore::writeToState lacks the invalid-parent guard VenueModel::writeToState has

**File:** `plugins/O-Octagon/Source/Data/SceneModel.cpp:244`

VenueModel::writeToState early-returns on an invalid parent (VenueModel.cpp:276-277); SceneStore::writeToState does not. On an invalid parentState, getOrCreateChildWithName returns an invalid tree (juce_ValueTree.cpp:905 'object != nullptr ? ... : ValueTree()') and the subsequent setProperty hits 'jassert (object != nullptr)' (juce_ValueTree.cpp:777) — an assertion storm in Debug, a silent dropped write in Release. Today's call sites pass apvts.state, which is always valid, so this is robustness parity rather than a live bug.

**Fix:** Add 'if (! parentState.isValid()) return;' at the top of SceneStore::writeToState, matching the VENUE writer.

### IN-22 — setSpeakerLabel skips recomputeDerived() while setSpeakerTrimDb calls it purely for future-proof uniformity

**File:** `plugins/O-Octagon/Source/Data/VenueModel.cpp:308`

setSpeakerTrimDb calls recomputeDerived() with the stated rationale 'keeping the call uniform means a future derived value that DOES depend on trim cannot be missed here' (lines 303-305). setSpeakerLabel, the very next mutator, omits the call — so the uniformity invariant the comment sells is already broken one function down: a future derived value depending on labels (e.g. a cached labelTypes array) would be silently stale after a label edit. Harmless today (labels are not geometric).

**Fix:** Either call recomputeDerived() in setSpeakerLabel too (it is cheap and message-thread-only), or narrow the comment in setSpeakerTrimDb so it no longer claims a uniformity that does not hold.

### IN-23 — Lenient schemaVersion parse: a garbage stamp reads as version 0 and reports LoadResult::ok

**File:** `plugins/O-Octagon/Source/Data/VenueFile.cpp:105`

The version is read with var->int coercion: schemaVersion="abc" parses to 0 via String::getIntValue, 0 > kSchemaVersion is false, and the load returns ok with *fileVersion = 0 — a corrupted or hand-mangled stamp is indistinguishable from a healthy v1 file, and describe(ok) returns an empty banner. Low impact (the payload still validates structurally), but the version channel is the one signal a FUTURE migration will key off, and it currently accepts nonsense silently.

**Fix:** Treat version < 1 as suspicious: either return malformedRoot, or clamp to kSchemaVersion and have describe() surface 'unrecognised format stamp' so the operator sees it.

### IN-24 — Venue field handlers dereference `committed` without a null guard — typing before the first geometry arrives throws inside the listener

**File:** `plugins/O-Octagon/Source/ui/public/js/venue.js:161`

labelOf() reads `committed.speakers[n - 1].label` and the bindNumeric revertText closures read `committed.speakers[...]` / `committed.rake.front` with no committed===null guard. bindLabel's input handler calls applyLabelMarks(collidingRows()) on every keystroke, and collidingRows() starts with `committed.speakers.map(...)`. If the operator reaches the Venue table before setGeometry has ever run (initial getVenueGeometry failed or — per the critical finding — its completion was dropped), every keystroke/blur in the table throws a TypeError inside its listener. Each throw is contained to that handler, so the page survives, but the table's commit/revert machinery is dead with console noise only. commit() itself guards (`if (committed === null) return;` line 298); the input-path helpers do not.

**Fix:** Add `if (committed === null) return new Set();` at the top of collidingRows(), and make revertText closures return "" (or skip the revert write) when committed is null — mirroring the guard commit() already has and the one bindLabel's Escape branch uses (line 380).

### IN-25 — Dropped native completions leak PromiseHandler map entries indefinitely while the editor is hidden with polls running

**File:** `plugins/O-Octagon/Source/ui/public/js/app.js:856`

PromiseHandler (juce/index.js:42-59) deletes a promise from its map only when the __juce__complete event arrives. With withKeepPageLoadedWhenBrowserIsHidden() set, a hidden editor keeps the page — and its timers — alive while every completion is dropped: pollStatus at 2 Hz (deliberately unguarded, per the P71 comment at app.js:456-460 which calls the leak 'bounded and acceptable') plus getMeters at ~30 Hz each strand one unresolved Promise + one map entry per tick. Over a long hidden stretch (editor window closed but instance alive in a host that hides rather than destroys — the configuration N9 measured) this is ~115k entries/hour, unbounded in time rather than 'bounded'. Memory-only and slow, hence info — but it is the one place the 'bounded' claim in the comments is optimistic.

**Fix:** Gate the polls on document visibility in app.js: on `visibilitychange`, clear statusTimer and meters.stop() when document.hidden, and restart both (plus one immediate pollStatus/refreshGeometry) when visible again. This also removes the wasted hidden-state bridge traffic. (Do not modify the vendored juce/index.js.)

⚠️ **Premise partly disproved.** This finding assumes a hidden editor silently drops native-fn completions. Verification established that it does not in this plugin — the gate is the web view's own `isVisible()` flag, which O-Octagon never clears (see *Refuted in verification*). Whatever part of this finding depends on a never-settling completion cannot fire today; any part that is simply wasted work still stands.

### IN-26 — Tooltip tipSuppressed can latch true when pointerup lands outside the WebView

**File:** `plugins/O-Octagon/Source/ui/public/js/app.js:712`

The capture-phase document pointerdown listener sets tipSuppressed = true; only a document pointerup clears it. For gestures on elements that take pointer capture (puck, sliders) the pointerup is delivered through the document even off-window — fine. But a press on a non-capturing surface (e.g. mousedown on a label or empty frame area) followed by dragging out of the plugin window and releasing there never delivers pointerup to the document: tipSuppressed stays true and every tooltip — including the data-tip-always help toggle's — is dead until the next in-view press-release cycle. Self-healing on the next click, hence info.

**Fix:** Also clear the flag on `pointercancel` (capture) and on `window.addEventListener("blur", ...)` — or clear it in the mouseover handler when no button is held (`if (e.buttons === 0) tipSuppressed = false;`).

### IN-27 — Deadline-released refreshGeometry can let a stale in-flight completion overwrite a newer geometry for one poll tick

**File:** `plugins/O-Octagon/Source/ui/public/js/app.js:475`

When the deadline (3 s) releases the guard while request A is still pending, request B starts; if A's completion then arrives after B's (out-of-order resolution is explicitly possible per the file's own header, lines 80-82), A's older payload overwrites `geometry` and rolls cachedVenueGen back to the older generation. Also, A's `finally` clears geometryFetchInFlight while B is still in flight, un-guarding a third call. Both are self-healing — the next 2 Hz tick sees venueGen !== cachedVenueGen and refetches — so the stale window is at most ~500 ms plus one round trip. Listed because a one-line generation check closes it entirely.

**Fix:** Guard the apply: `const g = Number(payload.generation); if (Number.isFinite(g) && g < cachedVenueGen) return;` before assigning geometry, and tag the guard release to the request (`const mine = geometryFetchSince; ... finally { if (geometryFetchSince === mine) geometryFetchInFlight = false; }`).

⚠️ **Premise partly disproved.** This finding assumes a hidden editor silently drops native-fn completions. Verification established that it does not in this plugin — the gate is the web view's own `isVisible()` flag, which O-Octagon never clears (see *Refuted in verification*). Whatever part of this finding depends on a never-settling completion cannot fire today; any part that is simply wasted work still stands.

### IN-28 — Channel-label text fields carry inputmode=decimal

**File:** `plugins/O-Octagon/Source/ui/public/index.html:480`

All eight `vf-label-N` fields (index.html:480, 489, 498, 507, 516, 525, 534, 543) are `<input type="text" inputmode="decimal" class="vfield vfield-label">` — but they hold channel LABELS ("L", "R", "Ls", ...), not numbers; the attribute was evidently copied from the numeric X/Y/Z/Trim cells. Harmless with a physical keyboard, but any touch/on-screen keyboard offers a number pad for a field that needs letters.

**Fix:** Remove `inputmode="decimal"` from the eight `vfield-label` inputs (or set `inputmode="text"`).

### IN-29 — Meter attack/decay are per-rAF-frame coefficients — ballistics run 2x fast on 120 Hz displays

**File:** `plugins/O-Octagon/Source/ui/public/js/meters.js:191`

The peak HOLD and RELEASE are correctly wall-clock (lines 196-201), but the level smoothing applies `ATTACK_PER_FRAME = 0.5` / `DECAY_PER_FRAME = 0.12` once per requestAnimationFrame frame (lines 191-192: `s.cur += (target - s.cur) * k`). On a 120 Hz ProMotion display the arcs attack and decay twice as fast as on 60 Hz; in a throttled/30 Hz context, half as fast. The module's own header (lines 37-44) cites pattern_block_rate_envelope_breaks_blocksize_invariance as the trap being avoided, yet the smoothing clock is the frame rate — the criterion's "per FRAME" wording bakes in a 60 fps assumption. Visual-only; noted because the file's stated discipline is exactly this invariance.

**Fix:** Make the coefficient dt-aware, anchored to the spec'd 60 fps feel: `const k60 = target > s.cur ? ATTACK_PER_FRAME : DECAY_PER_FRAME; const k = 1 - Math.pow(1 - k60, dtSec * 60); s.cur += (target - s.cur) * k;` (dtSec already exists at line 177).

### IN-30 — Deadline-released polls can apply a superseded payload out of order and momentarily defeat the in-flight guard

**File:** `plugins/O-Octagon/Source/ui/public/js/field.js:181`

In both field.js (refresh, lines 165-183) and meters.js (tick, lines 147-170), when the deadline expires the module issues a replacement call while the old promise is still pending. If the old completion then arrives late (slow, not dropped), two things happen: its `.then` runs `decode(payload)`/`applyPeaks(...)` and can overwrite data from the NEWER call that settled first (stale field grid / stale peaks applied out of order), and its `.finally` sets `inFlight = false` while the replacement is genuinely still in flight, so one extra overlapping call can be issued on the next tick. Bounded (self-corrects on the following tick for meters; next dirty tick for the field) and only reachable after a >3 s / >165 ms straggler, hence info.

**Fix:** Tag each request with a generation: `const gen = ++requestGen;` then in `.then`/`.finally` do nothing unless `gen === requestGen`. One integer per module closes both the out-of-order apply and the premature guard release.

⚠️ **Premise partly disproved.** This finding assumes a hidden editor silently drops native-fn completions. Verification established that it does not in this plugin — the gate is the web view's own `isVisible()` flag, which O-Octagon never clears (see *Refuted in verification*). Whatever part of this finding depends on a never-settling completion cannot fire today; any part that is simply wasted work still stands.

### IN-31 — Elevation speaker-label de-clash handles pairs only — three or more coincident speakers stack labels

**File:** `plugins/O-Octagon/Source/ui/public/js/elevation.js:277`

The side elevation collapses the lateral axis, so all speakers sharing (y, z) land on one point. The de-clash logic only distinguishes `clash === 0` (offset -5) from `clash >= 1` (offset +5): with three or more coincident speakers — e.g. a front L/C/R row at one depth and height, a realistic measured rig — labels 2, 3, ... all render at exactly `p.x + 5, p.y - 7`, illegibly stacked. The dots staying coincident is correct by the module's own rule; the labels are the part meant to step aside and they don't beyond the first pair.

**Fix:** Use the clash count as an index, e.g. `x: p.x + (clash === 0 ? -5 : 5 + 8 * (clash - 1))` or alternate sides/stagger vertically (`y: p.y - 7 - 8 * Math.floor(clash / 2)`), so each successive coincident label gets a distinct slot.

### IN-32 — Dead CSS: --frame-w / --frame-h are declared as 'the frame contract' but never consumed

**File:** `plugins/O-Octagon/Source/ui/public/css/styles.css:71`

styles.css:70-72 declares `--frame-w: 1100px; --frame-h: 720px;` under the comment "Frame contract. Mirrored by PluginEditor.cpp setSize(1100, 720)." — but nothing references them: html/body (lines 105-106) and .frame (117-118) use the literals `1100px`/`720px` directly, and no JS reads the variables. The other frame tokens (--header-h, --footer-h, --plan-col-w, --venue-rail-w) are all consumed. The two dead variables invite a future edit that changes the token, sees no effect, and leaves the contract comment pointing at inert declarations.

**Fix:** Either consume them (`width: var(--frame-w); height: var(--frame-h);` on html/body and .frame — keeps the setSize mirror in one place, which is the comment's stated intent) or delete the two declarations.

### IN-33 — Puck is role=slider with tabindex=0 but has no keyboard handling and no aria-value attributes

**File:** `plugins/O-Octagon/Source/ui/public/index.html:183`

The puck advertises `role="slider" tabindex="0"` (index.html:183-185) but roomplan.js registers only pointer listeners (pointerdown/move/up/cancel, lines 627-659) — no keydown for arrow keys, and none of aria-valuemin/valuemax/valuenow, so the ARIA slider contract is half-implemented: it is focusable and announced as a slider yet inert to the keyboard. The 16 native range inputs are fully keyboard-driveable, so this is the only control on the Room screen with the gap.

**Fix:** Add a keydown handler on the puck mapping Arrow keys to ±one step of srcX/srcY (open/close the gesture brackets around each keypress, reusing openPuckGesture/closePuckGesture), and keep aria-valuenow/valuetext updated in renderPuck (e.g. the metres readout string); or drop role=slider/tabindex if keyboard support is explicitly out of scope.

### IN-34 — Meter poll runs at 30 Hz forever — never-settling native calls accumulate while the editor is hidden, and full rAF work continues while the Venue tab hides the meters

**File:** `plugins/O-Octagon/Source/ui/public/js/meters.js:161`

meters.start() is called once at init (app.js:864) and stop() only on `pagehide` (app.js:866-869). Two consequences. (1) While the editor view is hidden (plugin window closed-but-cached, host UI hidden), setInterval keeps firing at 33 ms; by this repo's own measured research (module header, lines 45-66: completions are DROPPED while hidden, neither catch nor finally runs), each tick's `deps.nativeFn("getMeters")()` leaves a promise that never settles, and after the 165 ms deadline the guard deliberately issues the NEXT call anyway (lines 147-170). That is ~30 never-settled bridge calls per second, each holding a resolver entry in the JUCE frontend's completion registry — unbounded growth for as long as the editor stays hidden, plus a 30 Hz message-thread `readAndZeroMeters()` in C++ doing work no one can see. (2) With the Venue tab active, the Room screen is display:none but the rAF loop (line 175) still runs at full frame rate and setMeters still writes dasharray/transform/class attributes into the hidden SVG every frame.

**Fix:** Gate the poll on visibility: in tick(), `if (document.visibilityState === "hidden") return;` (the deadline guard already recovers the one in-flight call from the moment of hiding), and/or wire `document.addEventListener("visibilitychange", ...)` to meters.stop()/start(). Optionally pause the rAF-driven setMeters DOM writes (not the state update) while the Room screen tab is inactive — app.js already knows the active screen in switchScreen().

⚠️ **Premise partly disproved.** This finding assumes a hidden editor silently drops native-fn completions. Verification established that it does not in this plugin — the gate is the web view's own `isVisible()` flag, which O-Octagon never clears (see *Refuted in verification*). Whatever part of this finding depends on a never-settling completion cannot fire today; any part that is simply wasted work still stands.

## Refuted in verification

Raised by a reviewer, then disproved against the code by a second agent. Recorded in full so a future review does not re-raise them — each is a plausible-looking reading that the code defeats.

### ~~init() awaits four unguarded native completions before installing the status poll — one dropped completion permanently kills the UI~~

**File:** `plugins/O-Octagon/Source/ui/public/js/app.js:851` · **Claimed severity:** critical

**Claim:** init() serially awaits getParameterDefaults (line 845), refreshGeometry() (851), pollStatus() (852) and scenes.refreshSlots() (854) BEFORE window.setInterval(pollStatus, ...) is installed at line 856 and meters.start() at 864. A native completion is DROPPED (not rejected) while the WebView is hidden — the file's own header (lines 151-168) documents this and mandates that 'EVERY IN-FLIGHT GUARD ON THIS PAGE RELEASES ON A DEADLINE (P71/N9)'. The init await chain violates that doctrine: catch never fires on a drop, the await never resumes, and everything after it never runs. Because PluginEditor.cpp:270 sets withKeepPageLoadedWhenBrowserIsHidden(), the page survives in this half-initialised sta…

**Why it does not hold:** CODE READING IS CORRECT; THE TRIGGER IS NOT REACHABLE.

Verified verbatim from disk: app.js:845 `paramDefaults = await nativeFn("getParameterDefaults")();` (inside try/catch), 851 `await refreshGeometry();`, 852 `await pollStatus();`, 854 `if (scenes !== null) await scenes.refreshSlots();`, 856 `statusTimer = window.setInterval(pollStatus, STATUS_POLL_MS);`, 864 `meters.start()`. The consequence chain is also correct IF an await hangs: refreshGeometry has exactly two callers (app.js:531 in applyStatus, app.js:851 in init), so GEOMETRY_GUARD_DEADLINE_MS (169) cannot rescue init; the only other intervals are meters.js:213 (armed by meters.start() at 864) and venue.js:507 (ping, user-driven); PluginEditor.cpp:270 does carry .withKeepPageLoadedWhenBrowserIsHidden(). The reviewer's IF-THEN is sound.

WHAT FAILS IS THE "IF". The drop gate is WebBrowserComponent::emitEventIfBrowserIsVisible (JUCE 8.0.14, juce_WebBrowserComponent.cpp:607-611): `if (isVisible()) impl->emitEvent(...)`. Component::isVisible() (juce_Component.h:131) returns `flags.visibleFlag` — that component's OWN flag, not isShowing(). Component::setVisible (juce_Component.cpp:543-590) mutates only `this`; internalHierarchyChanged (1627-1655) notifies children via parentHierarchyChanged but never touches their flags; JUCE's own hide handling (juce_WebBrowserComponent_mac.mm:966 checkWindowAssociation) uses the RECURSIVE isShowing(), not isVisible(). So hiding the editor, hiding/minimising the plugin window, switching tracks, or a host calling editor->setVisible(false) (the only such calls in the tree are juce_audio_plugin_client_AUv3.mm:1916/1981, LV2:1567, VST2:969 — all on the editor/wrapper, never on a child) does NOT clear the webview's flag.

O-Octagon's webview is addAndMakeVisible'd once at PluginEditor.cpp:1331 and never hidden — `webView` appears only at 1315 (construct), 1331, 1332 (goToURL), 1372-1373 (setBounds). Nothing in the editor or any host path can make owner.isVisible() false while the page lives. If the editor IS destroyed, the page dies with it and reopening reloads it fresh. The completion-drop path this finding depends on cannot fire for this plugin in any format.

The second never-settling path — unregistered function name, `jassertfalse; return;` with no completion (juce_WebBrowserComponent.cpp:303-311) — also does not apply here: all four init calls are registered and each calls complete() exactly once on an unconditional path with no early return: getParameterDefaults (PluginEditor.cpp:284 -> complete 297), getVenueGeometry (325 -> 456), getStatus (470 -> 506), getScenes (1027 -> 1058).

The project doctrine the reviewer leans on is itself only half-verified: RESEARCH-3.3 N9 measured the JS half in a harness by making the transport return a never-settling promise, and the doc states explicitly "What was NOT run, and is not claimed: Q5's WKWebView half... Confirming that a real WKWebView drops a 30 Hz completion when hidden requires a running plugin... It is specified below as a named execute-phase item, not answered here." The memory note critical_webview_completion_gated_on_isvisible.md infers "the first time the editor is hidden (collapsed inspector, switched track, minimised window)" — that inference conflates isVisible() with isShowing() and does not hold against this JUCE source. Also, even granting the reviewer's model wholesale, the exposure window is only the few ms between page load and the fourth completion at editor open, not an ongoing hazard.

WHY 'info' RATHER THAN 'not-a-bug': the shape is genuinely fragile and contradicts the file's own rule at app.js:151-168 ("no UI state may depend solely on a promise resolving") — every live subsystem on the page hangs off four one-shot promises with no deadline and no recovery path. If anyone later adds a native fn to the init chain and forgets to register it in PluginEditor.cpp, the Release jassertfalse-no-completion path reproduces exactly the permanent kill described (that path IS reachable; only the ui_frontend_check.js section-3 grep-diff guards it). The proposed fix (install setInterval + meters.start() + the listeners before the awaits, make getParameterDefaults a .then, drop the redundant await on scenes.refreshSlots since cachedScenesGen starts at -1) is cheap, behaviour-preserving and strictly safer — worth doing as hardening, not as a critical bug fix.

### ~~fieldDirty is consumed even when field.refresh() refuses under its in-flight deadline guard — lost recompute, stale DBAP backdrop with no retry~~

**File:** `plugins/O-Octagon/Source/ui/public/js/app.js:548` · **Claimed severity:** warning

**Claim:** applyStatus() clears fieldDirty unconditionally before calling field.refresh(), but refresh() can silently refuse to issue a request: if a previous getFieldGrid completion was dropped (editor hidden, per the repo's own N4 mechanism) and less than GUARD_DEADLINE_MS (3000 ms) has elapsed, refresh() early-returns without fetching. The dirty flag is already gone, and nothing re-marks it until the NEXT weight/rolloff/blur/hullAtten echo or venue change — so the field gradient freezes on pre-change data indefinitely. This is asymmetric with the venueGen path, which deliberately self-heals: refreshGeometry() refusal leaves cachedVenueGen unchanged so the next poll retries. The field path has no equ…

**Why it does not hold:** CODE SHAPE: read correctly. /Users/taylorbrook/Dev/VST-development/plugins/O-Octagon/Source/ui/public/js/app.js:548-551 is verbatim `if (fieldDirty && field !== null) { fieldDirty = false; field.refresh(); }`, and field.js:162-184 does early-return void at `if (inFlight) { if (now - inFlightSince < GUARD_DEADLINE_MS) return; ++dropped; }`. The venueGen self-heal contrast is also accurate (app.js:465-476 — cachedVenueGen is only written inside the try after a successful await, so a refusal retries next tick). fieldDirty is set in exactly three places: the initial `let fieldDirty = true` (app.js:200), refreshGeometry success (app.js:488), and the 11 FIELD_INPUT_IDS echo listeners (app.js:841). So there is genuinely no retry path.

WHY THE FAILURE CANNOT OCCUR:

(1) The cited trigger is refuted by this plugin's own measured research. field.js:76-81's "drops are real" comment is a Phase 3.3 artifact that Phase 4.2 superseded. /Users/taylorbrook/Dev/VST-development/plugins/O-Octagon/.planning/stages/4-polish/RESEARCH-4.2.md N8 / §3.1-3.3 traces the gate to `emitEventIfBrowserIsVisible` (juce_WebBrowserComponent.cpp:607-611), which tests `Component::isVisible()` — the component's OWN flag, not `isShowing()`. Minimise, ⌘H, occlusion, Spaces and a hidden ancestor all leave it true; only an explicit `setVisible(false)` on the web view drops a completion. I confirmed independently rather than trusting the doc: `grep -rn setVisible` over /Users/taylorbrook/Dev/VST-development/plugins/O-Octagon/Source/ returns ZERO hits, and PluginEditor.cpp:1331-1332 does `addAndMakeVisible(*webView)` BEFORE `goToURL`, so the flag is already true before any page JS can invoke. The getFieldGrid handler (PluginEditor.cpp:1149-1194) has no early return — every path calls `complete()` exactly once — so there is no missing-completion source there either. Nothing in O-Octagon can latch inFlight.

(2) Without a latch, a refusal is benign, because the backend→JS channel is FIFO and native calls are serviced synchronously in arrival order. juce_WebBrowserComponent.cpp:297-346: `handleNativeFunctionCall` runs the (synchronous) lambda and `completeNativeFunctionCall` emits immediately on the message thread; :414-429 shows every completion AND every control-relay event going through the same `emitEvent` → one `evaluateJavaScript` per WKWebView, in order. Consequences: (a) getStatus for tick N+1 is necessarily posted after the getFieldGrid issued from tick N's applyStatus, so its completion is delivered after — inFlight is already false when the next applyStatus runs; (b) fieldDirty can only be raised by backend traffic — Source/ui/public/js/juce/index.js:178-212 shows `valueChangedEvent.callListeners()` fires ONLY inside `handleEvent` (a backend-delivered event), never from the local `setNormalisedValue`, matching app.js:836-838's "mark on the ECHO" comment. So if a request was serviced before the parameter change, its completion was emitted before the echo and therefore clears inFlight before fieldDirty is set; if it was serviced after the change, the in-flight result already carries that change and refusing costs nothing. Each evaluateJavaScript drains its microtasks (including the `.finally`) before the next one runs, so there is no JS-level reordering either. The reviewer's implicit "burst/stall" variant fails for the same ordering reason.

NET: the refusal branch is effectively unreachable in this plugin, and where reachable it is harmless. The described symptom (backdrop frozen on pre-change data with no retry) requires a never-settling completion that O-Octagon has no mechanism to produce. Even hypothetically the blast radius is a cosmetic gradient plus legend that self-heals on the next of 11 param echoes or any venue change — no audio, no state loss.

The proposed one-liner (`if (fieldDirty && field !== null && field.refresh()) fieldDirty = false;`) is harmless defence-in-depth, consistent with the file's own "every guard releases on a deadline" doctrine, and would not disturb tests/ui_layout_check.js §27 which counts getFieldGrid invocations. But it fixes nothing that can happen today.

### ~~Parameter values have no resync path after the hidden-editor event drop — sliders, readouts, puck and elevation marker can show stale values on re-show~~

**File:** `plugins/O-Octagon/Source/ui/public/js/app.js:263` · **Claimed severity:** warning

**Claim:** The page opts into withKeepPageLoadedWhenBrowserIsHidden (PluginEditor.cpp:270), and the codebase's own research (N4/N9, documented at PluginEditor.h:68-75 and venue.js:80-82: 'emitEvent IS emitEventIfBrowserIsVisible') establishes that ALL C++->JS events are dropped while the component is hidden — including WebSliderRelay valueChanged echoes. Every stale-prone surface was given a poll to self-heal (getStatus for venue/scenes generations, getPingState, getMeters, getTooltipsEnabled pull) EXCEPT parameter values: slider positions, value readouts, the puck, and the elevation marker are painted only from valueChangedEvent listeners. Host automation or a host-side preset change while the editor …

**Why it does not hold:** The finding's DESCRIPTION of the code is accurate in every particular, but its PRECONDITION is unreachable in this plugin, so the failure it predicts cannot occur.

What checks out (all re-read from disk):
- PluginEditor.cpp:268 does declare `.withKeepPageLoadedWhenBrowserIsHidden()`.
- app.js:263-264 (`state.valueChangedEvent.addListener(render)` / `propertiesChangedEvent`) really is the only path that repaints a slider or readout from C++ state; roomplan.js:663-664 paints the puck and elevation.js:343-344 the elevation marker from the same event, nothing else.
- juce/index.js SliderState (lines 135-167) caches `scaledValue` and emits `requestInitialUpdate` exactly once, in its constructor.
- getStatus (PluginEditor.cpp:470-507) carries safeMode/outputSetName/numOutputChannels/mapInvalid*/venueGen/scenesGen and no parameter revision.
- No `visibilitychange`, `pageshow`, `document.hidden` or `visibilityState` anywhere under Source/ui or tests/ (grep returns nothing); only two `pagehide` handlers plus `resize`.
- juce_WebControlRelays.cpp:84 confirms `WebSliderRelay::emitEvent` routes through `browser->emitEventIfBrowserIsVisible`, so the echo genuinely shares the drop gate with native-fn completions.

Why it still cannot fire:
- `WebBrowserComponent::emitEventIfBrowserIsVisible` (juce_WebBrowserComponent.cpp:607-611) gates on `isVisible()`, which is `Component::flags.visibleFlag` — the component's OWN flag (juce_Component.h:131). `Component::setVisible` (juce_Component.cpp:543-588) mutates only `this->flags.visibleFlag` and calls `sendVisibilityChangeMessage()` on `this` alone; it does NOT propagate to children. So a hidden editor, a hidden host window, a collapsed inspector, a switched track or a hidden NSView — all of which change `isShowing()` — leave the WebBrowserComponent's own `isVisible()` true.
- O-Octagon calls `addAndMakeVisible (*webView)` at PluginEditor.cpp:1331 and `grep -rn "setVisible" Source/ tests/` returns ZERO hits — the webView is never hidden by this plugin. The JUCE VST3/AU wrappers only ever call `setVisible(true)` (juce_audio_plugin_client_VST3.cpp:2003, juce_audio_plugin_client_AU_1.mm:1750); the `setVisible(false)` calls in the plugin client are AUv3/LV2/VST2 only (not built here) and target the editor holder, not the webView child.
- With keepPageLoaded set, `checkWindowAssociation` (juce_WebBrowserComponent_mac.mm:734-755 / 966-990) does nothing at all on hide, and `reloadLastURL()` clears `lastURL` after the first navigation, so there is no reload/re-show cycle to reason about either.
- JUCE itself asserts `jassert (owner.isVisible())` on the completion path (juce_WebBrowserComponent.cpp:341) — it treats an emit-while-hidden as a programming error, not a normal state.

The one real drop window is the editor constructor, between `webView = make_unique<WebBrowserComponent>` (PluginEditor.cpp:1315) and `addAndMakeVisible` (:1331): the 17 `WebSliderParameterAttachment` ctors run in that gap and their `sendInitialUpdate()` is silently discarded. That is already fully covered — the JS SliderState ctor emits `requestInitialUpdate`, which reaches `WebSliderParameterAttachment::initialUpdateRequested` (juce_ParameterAttachments.h:301) -> `sendInitialUpdate()` -> `attachment.sendInitialUpdate()`, and `ParameterAttachment::sendInitialUpdate` reads the LIVE `parameter.getValue()`, not a cached one. So the page always starts from current values, and every later host-automation or preset change reaches it via `ParameterAttachment`'s AsyncUpdater -> `WebSliderRelay::setValue` -> an emit that is never gated off.

Also worth recording against the reviewer's cited evidence: the project's own N9 measurement (RESEARCH-3.3.md:87-94) was produced by making the `getVenueGeometry` transport return a never-settling promise in the browser stub, NOT by hiding a real editor, and the same document states at lines 53-56 that the WKWebView half "was NOT run, and is not claimed". So the hidden-drop precondition has never been observed in a running host here; it is a source-read inference that the `isVisible()`-vs-`isShowing()` distinction defeats.

Residual note (not a defect today): if a future change ever adds a `webView->setVisible(false)` — e.g. overlaying a native component or a modal — the gap the finding describes becomes real, and its proposed paramsGen fix would then be the right shape. As shipped, adding a 2 Hz re-render of all 17 sliders would be dead code that also risks fighting an open drag gesture.

## Handled correctly

Commonly-broken things this codebase gets right — recorded so a later reviewer does not re-litigate them.

- Division-by-zero at speaker positions: the kMinDistance floor is applied unconditionally on every path AFTER the blur term (DbapSolver.cpp:68-69, d = max(sqrt(dist²+rs²), 0.05)), with a static_assert (DbapSolver.h:179) making the strictly-positive floor an enforced invariant — sitting the source exactly on a speaker at blur 0 cannot produce inf.
- All-zero-weight degenerate rig: denom < 1e-20 writes eight EXACT zeros plus a 0.0f field value (DbapSolver.cpp:83-94), preventing the k=inf → v=inf·0=NaN chain that would latch permanently in the SmoothedValue targets; the epsilon itself is static_asserted strictly positive.
- DBAP math is correct against the revised equations: a = R/(20·log10 2) (rolloffToAlpha), blur folded as d = sqrt(dist²+rs²), v_i = k·w_i·d_i^-a with k = 1/sqrt(Σ(w·t)²) giving Σv²=1 exactly, and w_i=0 yields bit-exact 0.0f at that speaker.
- pow() budget: exactly 8 std::pow per solve via the t = pow(d,−a) reuse (t and t², DbapSolver.cpp:74-77), and ALL pow/exp2/tan work happens at the 64-sample control rate only — the per-sample inner loop is pure multiply-add.
- Block-size invariance: the control grid is keyed to an absolute uint64 sample counter with a power-of-two mask (static_asserted, GainStage.h:129), chunks split at grid boundaries (process():202-217), and all 17 smoothers advance exactly once per sample UNCONDITIONALLY in both REAL and SAFE modes (renderChunk:452-456, 543-551) — including the deliberately-discarded getNextValue() calls in SAFE mode so an F3 mode flip mid-stream resumes from live smoother state.
- Smoother reset-order trap avoided: prepare() does reset(sr, 0.005) → one forced updateControl() → setCurrentAndTargetValue(getTargetValue()) (GainStage.cpp:150-187), so sample 0 is already at the solved gains instead of 5 ms into a fade-in from zero; reset is called with the real sample rate before any advance.
- Zipper noise: 5 ms linear ramps on all 16 speaker gains + outGain; both v1.3.0 additions (z-cue trim, widened rolloff) fold into the smoothed targets rather than the sample path.
- RT safety: no heap allocation, locks, or logging anywhere on the audio path; instrumentation atomics are compiled OUT of the shipping binary (not merely unread, DbapSolver.h:59-103); FirstOrderTPTFilter prepared with numChannels=1 so reset/prepare do not allocate; ScopedNoDenormals is in processBlock (PluginProcessor.cpp:520); parameters are read through cached atomic pointers (paramPtr[k]->load), not per-block APVTS string lookups.
- NaN hygiene end-to-end: the 17-float snapshot is isfinite-sanitised with per-param defaults (PluginProcessor.cpp:296); the dirty check is memcmp specifically so a NaN cannot make it report 'changed' forever (GainStage.cpp:225-238); zCueGain guards NaN/zero/negative invK via !(x > 0.0f) which is true for NaN (GainStage.cpp:66); and the TPT-filter NaN guard checks the LAST output rather than a running jmax — correctly reasoned, since `worst < NaN` is false and a max would silently discard the NaN (GainStage.cpp:419-433). The stickiness argument (s = y + v re-derives state from the poisoned value) is mathematically sound for this filter topology.
- Air filter: Nyquist-margin clamp at 0.45·fs (static_asserted < 0.5) prevents the setCutoffFrequency assert and negative-tan nonsense at 22.05/32 kHz; the floor takes the ceiling as its own upper bound so the clamp cannot invert; TWO mono instances because G is per-filter, not per-channel; bit-exact ENTRY via s=x seeding with the seed expression exactly matching the sL/sR computation; seed flags deliberately not consumed in SAFE mode (F3 flip window); reset only on the airAmount→0 TRANSITION, never on d_hull==0 crossings (hull-edge oscillation preserved).
- Input aliasing (out[0] aliases in[0]): sL/sR are read at the top of each sample iteration before any output write for that sample; write pointers are hoisted, read hoisting is avoided, and the ping overwrite goes through the same mapped out[] pointers after the loop.
- Convex hull edge cases: degenerate counts 0/1/2 are routed explicitly in both isInside and project (a 2-point 'polygon' never reaches the zero-area loop); duplicate speakers are deduped before the chain with lowest-index representative; zero-length segments guarded by max(ab2, EPS_LEN2); winding is MEASURED via signed area and reversed if CW rather than trusted; collinear vertices are popped with an area-scaled epsilon that correctly degrades to exact comparison on a degenerate span; the chain array is sized 2n and hullCount is clamped; lowerEnd = k+1 matches the textbook monotone-chain upper-hull sentinel.
- hullTrimGain unity at the origin is genuinely bit-exact: −0.0f compares equal to 0.0f so decibelsToGain takes its pow branch, and pow(10, ±0) == 1 is IEEE-mandated — the reasoning in the comment (HullProcessor.h:96-109) checks out, including the no-ffast-math premise.
- v1.3.0 z-cue transparency at default: the reference solve shares the identical (possibly hull-projected) x/y, weights, a and rs, differing only by the stripped srcZ — at srcZ=0 the inputs are bit-identical, invK ratio is exactly 1, pow(1, 2.5)=1, and x·1.0f preserves pre-1.3.0 output bit-for-bit; the ±6 dB clamp bounds pathological rigs, and inf/NaN ratios fall into the clamp or the 1.0f guard.
- FieldSampler: message-thread only, sample() allocation-free, quantise's divide-by-zero on a uniform field guarded (encodes 128), silent field short-circuits before normalisation, cell CENTRES avoid the kMinDistance floor flattening the grid edge, and the field runs the full shipping chain (hull project + solve + trim) rather than a mirrored reimplementation.
- Repo landmine patterns checked and absent in this file set: no param-ID named 'end'/'begin'; no AudioParameterChoice at all (all floats); no MessageManager::callAsync, AsyncUpdater, or IIR Coefficients::makeXXX on the audio path; no per-block getRawParameterValue string lookups; Vec2/Vec3 are static_asserted trivially copyable for the snapshot memcpy.
- APVTS caching: all 17 getRawParameterValue pointers are cached once in the constructor into paramPtr[] (PluginProcessor.cpp:156) and the audio thread only does relaxed atomic loads through them — no per-block string lookups (the exact landmine avoided).
- Parameter NaN latch: snapshotParameters() (cpp:287-300) substitutes each parameter's DECLARED default for any non-finite read, and paramDefaults is derived from the parameter objects at construction (cpp:163), not hand-transcribed — closes the SmoothedValue setTargetValue(NaN) permanent-silence path and avoids pattern_test_fixture_mirrors_drift_silently.
- Venue NaN/Inf funnel: publishSnapshot() (cpp:355-436) is the single sanitisation site for all 42 venue values — isfinite fallbacks read from a default-constructed VenueModel, and trimDb is jlimit-ed to ±kVenueTrimClampDb BEFORE decibelsToGain so the 0·inf=NaN door into the smoothers cannot be constructed.
- ValueTree string-var trap: tooltipsEnabled rides get/setStateInformation as a root XML ATTRIBUTE with getBoolAttribute (cpp:824, 841-843), deliberately not a ValueTree property — critical_valuetree_xml_roundtrip_loses_type sidestepped by design, with the reasoning written at both sites.
- AsyncUpdater guard-flag trap: there is no AsyncUpdater anywhere; the setState-before-prepare window is handled by the plain preparedYet flag with the rebuild deferred to prepareToPlay (cpp:882-883), so no cancelPendingUpdate() obligation exists — and the header documents why (h:397-401).
- Denormalised-vs-normalised split: sessions survive the v1.3.0 range widenings because APVTS state stores denormalised values; presets (normalised fractions) get an explicit per-version migration hook (preset-manager v1.0.6) — the pattern's session half is fully correct (automation lanes are the residual, reported as a warning).
- AudioChannelSet-is-a-bitset discipline: the map is keyed on ChannelType and resolved through getChannelIndexForType() as a genuine lookup (ChannelMap.cpp:73); buildSpeakerToBuffer writes the output array ONLY on full success (cpp:100), the caller retains the last valid map on failure and raises mapInvalid (PluginProcessor.cpp:334-337); isPermutationOf0to7 catches duplicates and absent labels (ChannelMap.cpp:26-42); verifyEnumBitOrder's 256-bit scan plus the size assertion closes the silent-truncation hole (ChannelMap.cpp:118-135). OutputOrder.h stays a pure label table — no buffer index anywhere.
- F3 narrow-buffer hazard: processBlock bounds everything by buffer.getNumChannels(), never getTotalNumOutputChannels() (cpp:530), and mappedOutputAvailable() requires BOTH an 8-channel buffer AND a valid map (cpp:303-307) — with the audio thread aborting a running ping the moment mapped goes false (cpp:550-551), backstopping the message-thread precondition that can lie.
- VerifyPing RT-safety: cross-thread surface is atomics-only (no juce::String crosses a thread), command consumption is a single exchange (VerifyPing.cpp:191), decibelsToGain is hoisted out of the sample loop (cpp:226), both clocks are sample-counted for offline measurability, the TPT filters are prepare()d (avoiding the s1={2} unprepared-decay trap, cpp:67), and the RNG is member-owned — pattern_rng_stream_interleave_blocksize avoided by construction.
- Meter path RT-safety: lock-free std::atomic<float> max-hold with is_always_lock_free enforced by static_assert (h:364-367), exchange(0) read so dropped WebView frames widen the window instead of losing peaks, the benign load/compare/store race documented rather than 'fixed' with an audio-thread CAS loop, and attribution through the SAME snapshot the block was rendered against (cpp:595) so a venue edit cannot mis-light a lane for one block.
- Snapshot generation tearing (the H1 bug): the generation counter is carried INSIDE the trivially-copyable payload and stamped before the single release store (VenueSnapshot.h:80, 120-124), so data and generation cross the acquire edge together — the two-atomic stale-generation permanent-miss is structurally unreachable (independent of the double-publish tear reported above).
- Gesture brackets: applySceneWeights (cpp:649-688) wraps every setValueNotifyingHost in beginChangeGesture/endChangeGesture, closed on all paths — the Logic Latch/Touch moves-but-never-records omission that no build gate can see.
- prepareToPlay ordering and reset discipline: venue → map → safeMode → ping → gainStage.prepare LAST against a published snapshot and built map (cpp:275); releaseResources deliberately does NOT reset the smoothers (a second reset site would be invisible to block-size-invariance probes); setLatencySamples correctly absent (latency is 0; getLatencySamples is non-virtual in JUCE 8).
- processBlockBypassed override: aborts the ping (atomics-only, RT-safe) before delegating to the base passthrough (cpp:506-516), so bypass actually silences the diagnostic and stops the 120 s latch instead of leaving it ticking.
- CMakeLists: VERSION (not the silently-ignored PLUGIN_VERSION keyword), no PLUGIN_CHANNEL_CONFIGURATIONS (layout comes from isBusesLayoutSupported), juce_add_binary_data has an explicit NAMESPACE and hyphen-free filenames, JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 is set, and check_native_interop.js is embedded — every build-note landmine in the repo memory is individually addressed.
- None of the remaining checklist items apply and none are violated: no AudioParameterChoice at all (so no single-choice 0/0 NaN), no param IDs named end/begin, no IIR Coefficients::makeXXX on the audio thread (the only filters are TPT, prepared off-thread), no MessageManager::callAsync from processBlock, ParameterID version hints present, and String usage is ASCII-safe throughout the reviewed files.
- Block-size invariance is designed-in rather than asserted: GainStage's control grid keys off an absolute sample counter (verified in GainStage.h while confirming the params enum), the ping's clocks are sample-counted, and every probe-visible constant is a named symbol rather than a transcribed literal.
- Bridge symmetry is complete in BOTH directions: all 22 native functions registered in PluginEditor.cpp have JS call sites and every Juce.getNativeFunction name in JS (via the single nativeFn() cache, app.js:208-211) has a C++ registration — no silently-dead control, no never-settling promise from an unregistered name.
- Member destruction order is correct and documented: relays declared first, WebView second, attachments last (PluginEditor.h:172-196), so attachments die while the WebView is alive — the classic reload crash is structurally absent.
- FileChooser lifetime: the chooser is a member (outlives launchAsync), the SafePointer is hoisted to a local (MSVC init-capture trap avoided), and on a dead pointer the completion returns BARE — never complete(false) into a freed editor (PluginEditor.cpp:717-745, 759-809). Cross-checked in JUCE source that ~FileChooser() nulls asyncCallback, so editor teardown with an open dialog cannot invoke the stale lambda.
- Dropped-completion (N4) defence is systematic: every in-flight guard carries a timestamp deadline (geometry app.js:465-469, meters meters.js:147-151, field field.js:165-171); pollStatus is deliberately guard-free so it self-heals; the ping poll starts BEFORE its start promise is awaited (venue.js:505-513); no poll is a poll().then(poll) recursion; getMeters read-and-zero lives in C++ so a dropped completion widens the measurement window instead of losing the peak.
- Knob readouts use SliderState.getScaledValue() only; the FORMAT table carries units and decimals with no range constants (app.js:126-144); dblclick reset converts defaults through the live properties with pow((def-start)/span, skew), which matches NormalisableRange::convertTo0to1 — and no JS default table exists to drift.
- v1.3.0 preset migration is exact: all parameter ranges are two-arg (skew-1 linear) NormalisableRanges (PluginProcessor.cpp:38-43), so the stored-fraction rescales (rolloff /3 for 3-6->3-12, width /2 for 0-6->0-12, blur /3 for the kBlurScale x3) are algebraically correct; the version gate parses major.minor with unparseable-treated-as-pre-1.3, correct for every preset that exists; APVTS sessions (denormalised) are correctly left alone.
- loadPreset opens and closes change gestures on all 17 parameters, closed on BOTH success and failure paths (PluginEditor.cpp:869-888); the puck opens/closes gestures on both srcX and srcY including pointercancel and lostpointercapture (roomplan.js:600-659); slider bindings close on pointercancel/lostpointercapture/blur (app.js:284-290).
- Canvas is handled as a replaced element: explicit width/height via calc(var(--plan-w)*1px) in CSS, backing store set to round(rect*dpr) with ctx.setTransform+scale (roomplan.js:264-274), matching the o-textureforge lesson.
- SVG display-only children (glyph numeral, output badge, meter track/arc/peak) carry pointer-events:none (styles.css:365-369), so the double-clickable glyph receives its events; the controls layer is pointer-events:none with children opting back in (styles.css:453-456).
- TDZ discipline: every module-level binding is a declaration, init() is the file's only top-level call and its last statement (app.js:104-201, 880-883); each page-module constructor is wrapped in its own try/catch so one failing module cannot kill the 17 bindings.
- No shared JS state updater touches an HTML-authored label anywhere: scenes write disabled/data-*/aria only, roomplan writes its own badge/value nodes, tooltips build their own child nodes with textContent, venue tab labels move class/aria only.
- Resource provider matches bare paths by equality (no scheme stripping), serves all 11 embedded files including js/juce/check_native_interop.js, all with charset=utf-8; every filename is authored hyphen-free so the juce_add_binary_data hyphen-stripping trap cannot fire.
- SceneStore bounds-checks every slot index (isOccupied/weights/capture all guard slot<0||>=kNumSlots, SceneModel.cpp:265-296), and storeScene/applyScene validate ranges editor-side too, so a malformed wire id cannot cause an OOB read.
- setVenue NaN-guards every float via finiteOr(), validates through applyVenueEditChecked() only (no raw applyVenueEdit call site), starts from the live venue so the name survives, and the page blocks commit entirely while the label set is not a permutation — with the label column deliberately hold-not-revert so L<->R swaps stay reachable.
- tooltipsEnabled persistence avoids the ValueTree XML string-var trap by using a root XML attribute with getBoolAttribute (PluginProcessor.cpp:824, 841-843), and the page PULLS it at init instead of relying on a one-shot C++ push.
- Meter ballistics separate the two clocks correctly: attack/decay per rAF frame, peak hold and release on wall-clock timestamps (meters.js:174-206), so a throttled tab changes neither the hold time nor the release rate; toDb floors at -60 so -Infinity can never poison the smoother state.
- Preset dropdown is flat (single-level options, venue.js:417-437) and there are no prev/next walkers to desync; selection is preserved across refresh with a sensible fallback to the current preset.
- The venue table repaint never stamps on a pending edit or the focused field (venue.js:226-239), and Escape/blur semantics differ correctly between numeric (revert) and label (hold) columns.
- Editor destructor stops the verify ping so a latched 120 s ping cannot outlive its only stop control (PluginEditor.cpp:1359-1362); startPing refuses on an invalid map, mirroring the audible-failure defence-in-depth.
- Windows WebView2 options set withUserDataFolder (temp dir), status bar and built-in error page disabled (PluginEditor.cpp:1222-1234) — the blank-page-in-DAW-hosts gotcha is covered.
- getFieldGrid samples on the message thread through the stateless free-function solver, reads raw parameter atomics with relaxed loads and NaN fallbacks, and ships the grid as 8-bit base64 (1.7 kB) rather than 61 kB of JSON — with the JS decoding via atob and never re-deriving the dB span from the bytes.
- NaN/Inf funnel to the audio thread: publishSnapshot() (PluginProcessor.cpp:355-435) sanitises every venue field per-value with fallbacks read from a default-constructed VenueModel (not transcribed literals), and clamps trimDb BEFORE the dB->linear conversion so +inf can never be constructed — the SmoothedValue permanent-silence latch (RESEARCH-2.2 H2) is closed at a single unbypassable site.
- SceneStore sanitises weights at BOTH ingestion sites — readFromState (SceneModel.cpp:229-237) and capture (288-293) apply isfinite + jlimit(0,1) — so a NaN from a hand-edited session can never reach setValueNotifyingHost.
- The @occupied bool survives the ValueTree->XML->string round-trip because it is read via var->bool coercion (static_cast<bool>, SceneModel.cpp:225) which routes through stringToBool ('1'/'true'/'yes' all parse) — the critical_valuetree_xml_roundtrip_loses_type landmine (isBool() guards never firing) is avoided.
- Speaker @index lookup works after XML round-trip despite string-vs-int var types: juce var equality coerces (stringEquals compares intToString(1)=="1", verified in juce_Variant.cpp:293-296), and a positional fallback (VenueModel.cpp:223-229) covers legacy children without an index — many codebases break exactly here.
- The snapshot generation counter rides INSIDE the POD payload and crosses on the single release/acquire pair (VenueSnapshot.h:111-131), structurally closing the two-atomic new-geometry/old-generation permanent-stale bug for non-torn reads; VenueSnapshot is static_asserted trivially copyable, and the publisher deliberately avoids atomic<shared_ptr> so no refcount free can occur on the audio thread.
- The v1.3.0 ÷-migrations are mathematically correct: rolloff 3-6 -> 3-12 shares its minimum so f_new = f_old·(3/9) = ÷3; width 0-6 -> 0-12 is ÷2; blur's 0-1 range is unchanged with kBlurScale tripled so ÷3 preserves the radius. The hook (PluginEditor.cpp:1272-1295) gates on major.minor >= 1.3, treats unparseable stamps as pre-1.3 (correct for every preset that exists), clamps results to 0..1, and is idempotent because migrated presets re-save stamped >= 1.3. Sessions are correctly untouched (APVTS stores denormalised values).
- Factory preset rows are ENGINEERING units converted through the LIVE NormalisableRange (PresetPolicy.h:152-155) — never hand-baked fractions — so the v1.3.0 range widenings moved the presets automatically (pattern_factory_preset_normalized_ignores_skew avoided). Concert Default's width/rolloff/blur match createParameterLayout's shipped defaults (0.0 / 4.0 / 0.03, verified against PluginProcessor.cpp:89-106).
- kPreserved + kAuthored are static_asserted to PARTITION oo::params::kCount (PresetPolicy.h:76-80), so a new parameter cannot silently join the WR-01 reset-to-default set; loadPreserving restores the 11 held params on BOTH success and failure paths, normalised-in/normalised-out bit-exact, with null-guarded parameter lookups.
- .venue loading goes into a FRESH model, rejects a wrong root type or missing speakers BEFORE touching `out`, and loads-then-SURFACES a forward schemaVersion instead of silently best-efforting (VenueFile.cpp:98-122) — the session-vs-file trust distinction is designed, documented, and mostly enforced (see the index-coverage finding for the residual gap).
- juce::String traps: the non-ASCII default venue name is built through CharPointer_UTF8 (VenueModel.cpp:185), describe() builds its message with << onto a NAMED lvalue (VenueFile.cpp:139-142), and scene wire-ids are deliberately pure ASCII (SceneModel.h:75-81) — both critical_juce_string_char_ctor_is_ascii_only and the operator<< rvalue trap are avoided and documented in place.
- Degenerate geometry: earHeight and normToMetres carry INDEPENDENT per-axis zero-span guards with a single kMinSpan definition (VenueModel::kMinSpan aliases plane::kMinSpan, VenueGeometry.h:47) — no second constant to drift, and the audio thread runs the identical free functions against the snapshot.
- Out-of-range indices are ignored, not asserted, in every mutator and accessor (isValidSpeaker gates, slot range checks, propWeight index clamp) — a UI click or hostile bridge payload cannot index out of bounds anywhere in this file set.
- RigPolicy states the safe-mode partition as the complement of the three real 8-ch containers, so an unknown fourth 8-ch layout raises the SAFE banner instead of silently passing as a rig; the literal appears exactly once in Source/.
- Speaker labels resolve through JUCE's own getChannelTypeFromAbbreviation rather than a mirrored local table, and both failure shapes (unknown string, numeric string -> unranged discreteChannel) are rejected downstream by the channel-map permutation check.
- Bridge surface closure: every getNativeFunction name used in the reviewed JS (getParameterDefaults, getVenueGeometry, getStatus, getFieldGrid, setVenue, saveVenue, loadVenue, savePreset, loadPreset, getPresetList, getCurrentPreset, startPing, stopPing, getPingState, getScenes, applyScene, storeScene, assignSpeakerOutput, applyOutputOrderPreset, setTooltipsEnabled, getTooltipsEnabled, getMeters) has a matching withNativeFunction registration in PluginEditor.cpp — no silent bridge gaps, including the v1.1.0/v1.2.0 additions.
- TDZ discipline holds: app.js declares every module-level binding in one top block, defines only functions below it, and init() is the sole top-level call as the last statement; venue.js and scenes.js are pure factories invoked inside init()'s per-module try/catch, so a constructor throw cannot take the 17 bindings down.
- Readouts use SliderState.getScaledValue() exclusively; the FORMAT table carries units/decimals only (no range or skew constants); the dblclick reset converts the engineering-unit default through live properties with Math.pow((def-start)/span, skew), which exactly matches getNormalisedValue()'s formula in juce/index.js — no JS min/max map anywhere.
- No authored label is ever written from JS: values go to dedicated .cell-value/.w-value/.vcell-value nodes, scene/tab state goes to data-*/aria attributes only, and the tooltip writes textContent into its own created child nodes.
- The N9 latch is genuinely repaired for the poll path: refreshGeometry's in-flight guard carries a timestamp and a deadline release, with finally as the fast path; pollStatus is deliberately left unguarded so the interval self-heals a dropped tick.
- Preset/scene/venue staleness converges on generation counters polled at 2 Hz (venueGen and scenesGen) rather than one-shot C++→JS pushes; scene membership rides the getVenueGeometry payload so it cannot go stale independently of the venue; the tooltip preference is PULLED at init, never pushed.
- Gesture brackets are complete on the JS side: openGesture/closeGesture on pointerdown/up/cancel/lostpointercapture plus keydown/keyup/blur, the dblclick reset is bracketed, and C++ loadPreset brackets all 17 parameters on both success and failure paths.
- setVenue is a single write path carrying all 42 values, blocked client-side while the label set is not a permutation of the committed set (checked against committed labels, not a transcribed channel list); the label column holds-and-marks while numeric columns revert — the L↔R swap stays reachable per P53.
- The ping poll is started before the startPing promise is awaited (a dropped completion cannot leave a sounding ping with a dead indicator), self-terminates on inactive state or error, and is cleared on pagehide; the lit speaker is always getPingState().speaker, never a JS timer.
- Scene wire formats match C++ exactly: storeScene takes the 1-based slot number (C++ validates 1..kNumSlots), applyScene accepts both named ids and 'slot1'..'slot4', slot payload order aligns with slots[n-1] indexing, and the armed STORE path auto-disarms whether the capture succeeded or not.
- The preset dropdown is built with createElement/appendChild (no innerHTML), preserves the current selection across refreshes, and there are no prev/next buttons to desync from a C++ flat list.
- Event listeners are bound once for page-lifetime elements; renderPresets replaces only listener-free option nodes; no rebinding loops exist, so there are no listener leaks in the reviewed files.
- Screen switching re-lays-out the plan, elevation strip, and mini-plan on return to their tab (hidden sections measure zero), and drawMini early-returns on a zero rect instead of drawing garbage.
- Payload key usage matches the C++ producers field-for-field: envelope/bbox/centroid/speakers[].{x,y,z,label,class,trimDb,output}/rake.{front,rear}/hull/scenes[].{id,indices,empty}/generation on getVenueGeometry, and safeMode/outputSetName/mapInvalid{,Reason,Speaker}/venueGen/scenesGen on getStatus.
- C++<->JS bridge surface is closed: all 22 native-fn names called from JS (getVenueGeometry, getStatus, setVenue, assignSpeakerOutput, applyOutputOrderPreset, saveVenue, loadVenue, savePreset, loadPreset, getPresetList, getCurrentPreset, startPing, stopPing, getPingState, getMeters, getScenes, applyScene, storeScene, getFieldGrid, setTooltipsEnabled, getTooltipsEnabled, getParameterDefaults) have matching withNativeFunction registrations in PluginEditor.cpp — verified 1:1, no gaps either direction.
- Canvas replaced-element landmine handled: #plan-backdrop gets explicit CSS width/height via calc(var(--plan-w) * 1px) (styles.css:319-326), and roomplan.js resizeCanvas() sets the backing store to round(rect * dpr) with ctx.setTransform + ctx.scale(dpr, dpr) (roomplan.js:264-274).
- Single coordinate projection: metresToPx() is the one mapping for canvas, SVG and controls layers; the elevation strip and mini-plan reuse it as second VIEWS (makeView), and earHeight is computed THROUGH it rather than beside it — no duplicate (v-min)/span arithmetic anywhere in the file set.
- Puck drag math is right: relative-delta (no jump-to-cursor on off-centre grabs), accumulator clamped at the accumulator (no sticky-edge unwind), pixel span derived from the BBOX not the envelope so pointer and puck track 1:1, degenerate axes taken from C++ payload flags rather than a transcribed kMinSpan threshold, and metres are computed from cached geometry — no native round trip in pointermove.
- Gesture brackets correct for the only two-parameter gesture: sliderDragStarted/Ended on BOTH srcX and srcY, closed on pointerup AND pointercancel AND lostpointercapture (roomplan.js:600-659); renders optimistically during drag and from the parameter echo otherwise, so host automation moves the puck.
- bbox vs envelope discipline held throughout: the plan box fits the ENVELOPE, the puck/footer resolve through the BBOX via normToMetres — the two boxes are never swapped.
- Hidden-completion latch (the repo's measured N9 failure) is defended everywhere: meters.js and field.js in-flight guards carry timestamps and release on a deadline, polls are fixed setInterval (never poll().then(poll)), dropped counts are exposed for the gates, and app.js's geometry fetch has the same shape.
- No label-erasure: every JS write in the file set targets a dedicated value node (gout-N badges, outpop-num, w-value, elev-ear/src, field-legend); HTML-authored numerals and labels are never touched; meters write only dasharray/rotate/class.
- SVG hit-testing correct: display-only glyph children (numeral, badge, meter track/arc/peak) are pointer-events:none (styles.css:366-369) so dblclick lands on the glyph; #plan-controls is pointer-events:none with children opting back in — the v1.1.0 popover flow (open on dblclick, outside-pointerdown + Escape to close, converge on venueGen poll) is sound.
- Field visualisation cannot drift from the DSP: nothing is derived in JS — the C++ FieldSampler runs the shipping shaper→hull-project→dbap::solve→hullTrim chain through the SAME oo::dbap::rolloffToAlpha/blurToRadius the audio path uses, so the v1.3.0 rolloff/blur range rework tracks the display automatically; JS only decodes base64 and blits; legend prints the returned span, never re-derived from the bytes; recompute is coalesced to the 2 Hz status tick with the dirty flag set on the parameter ECHO (catches host automation and preset loads), and the input list (w1-w8, rolloff, blur, hullAtten) matches the sampler's actual inputs.
- Elevation strip: srcZ read via getScaledValue() (skew-safe — the knob-readout landmine avoided), srcY through the shared normToMetres; rake line spans bbox only with the extrapolation on a separate dashed element; the marker clamps while both readouts stay exact; venue-derived quantised axis; and the chevron's [hidden] attribute — which the UA sheet does NOT hide on SVG elements — has an explicit `.elev-marker-chevron[hidden] { display:none }` rule (styles.css:848).
- elev-stage measured via clientWidth/clientHeight rather than getBoundingClientRect because the stage carries a 1px border — the 2px-overfit trap is explicitly avoided and documented.
- Hidden-screen resize behaviour: relayout early-returns on zero-size rects and is re-fired on tab switch (app.js switchScreen) and window resize, so a venue change while the Room screen is hidden re-fits correctly on return.
- No grouped-preset desync: the preset <select> is flat (options via createElement, no optgroup) and there are no prev/next walkers.
- CSS is essentially all live: every class rule in styles.css was cross-checked against JS/HTML writers (data-placement via dataset.placement, --arrow-x, is-invalid/is-colliding/is-occupied/is-pinging/is-current/is-hot/is-metered/is-preview, tooltip-title/body all written) — the only dead items found are the two unused --frame-w/--frame-h custom properties reported above.
- No TDZ/module-extraction hazards: all cross-referenced roomplan.js exports are hoisted function declarations, app.js's only top-level call is the final init(), and every Phase-3.3 module constructor is individually try/caught so one failure cannot take the 17 bindings down.
- Tooltip surface (v1.2.0) is a sibling of .frame (keeps #group-elevation the column's last child), width measure-then-pin with max-width load-bearing, content via textContent, and the state is PULLED at init via getTooltipsEnabled rather than pushed (one-shot-push staleness pattern avoided).

## Files reviewed

- `plugins/O-Octagon/Source/DSP/DbapSolver.cpp`
- `plugins/O-Octagon/Source/DSP/GainStage.cpp`
- `plugins/O-Octagon/Source/DSP/SourceShaper.cpp`
- `plugins/O-Octagon/Source/DSP/VerifyPing.cpp`
- `plugins/O-Octagon/Source/Data/PresetPolicy.h`
- `plugins/O-Octagon/Source/Data/SceneModel.cpp`
- `plugins/O-Octagon/Source/Data/VenueFile.cpp`
- `plugins/O-Octagon/Source/Data/VenueModel.cpp`
- `plugins/O-Octagon/Source/PluginEditor.cpp`
- `plugins/O-Octagon/Source/PluginEditor.h`
- `plugins/O-Octagon/Source/PluginProcessor.cpp`
- `plugins/O-Octagon/Source/ui/public/css/styles.css`
- `plugins/O-Octagon/Source/ui/public/index.html`
- `plugins/O-Octagon/Source/ui/public/js/app.js`
- `plugins/O-Octagon/Source/ui/public/js/elevation.js`
- `plugins/O-Octagon/Source/ui/public/js/field.js`
- `plugins/O-Octagon/Source/ui/public/js/juce/index.js`
- `plugins/O-Octagon/Source/ui/public/js/meters.js`
- `plugins/O-Octagon/Source/ui/public/js/roomplan.js`
- `plugins/O-Octagon/Source/ui/public/js/venue.js`
