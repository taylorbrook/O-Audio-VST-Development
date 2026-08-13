# Stage 4 — Polish · Phase 4.2 (host-and-ear) — Research

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation
**Phase:** 4.2 of 2 — Logic Pro, bounce path, `COMPAT-02`, Gate 13's interactive half, Q5, the audible clause
**GSD phase:** research
**Date:** 2026-08-13
**Branch:** `feat/o-octagon` @ `4952a8ca`
**Inputs:** `CONTEXT-4.2.md` (D11–D21, Q1–Q6), the frozen 4.1 binary `fba35081`

> **Numbering.** The N-series restarts each stage; **4.1 ran N1–N6, so 4.2's findings are N7–N15.**
> The P-series and probe letters do not restart — 4.1 closed at P100 / probe CQ, so plan decisions
> begin at P101 and new probes at CR.

---

## Entry Check — contracts, and the frozen binary

Per the standing rule (VERIFICATION-4.1 Issue 2, carried by CONTEXT-4.2): **compare against
`STATUS.md`'s live `contract_checksums` block — the ledger — never against a value quoted in a prior
artifact's prose.** Measured here against `STATUS.md:1136-1139`:

| Contract | `shasum -a 256` at arrival | `STATUS.md` ledger | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32890d7420…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ twelve consecutive phases unmoved |
| `parameter-spec.md` | `b45f88dc5017ec2c…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ 17 parameters, unmoved since Stage 1 |
| `research/ARCHITECTURE.md` | `2806c788092d9ec9…57bceb17` | `2806c788…57bceb17` | ✅ |
| `ROADMAP.md` | `ea50d991d1a6b158…1063d424` | `ea50d991…1063d424` | ✅ the 4.2-discuss re-pin (D11/D15/D20) |

**No drift.** No contract is amended at this boundary; this phase's research changes *how* things are
tested, and the two places it touches a contract's wording are raised as plan decisions in §9, not
executed here.

### The binary — re-measured, not inherited

CONTEXT-4.2 measured these six hours ago. They are measured again because the shared tree carries
three worktrees and a sibling session is live on `improve/o-spectralshaper-tooltips` right now — the
exact condition that invalidated "it was installed then" during 4.1 verify.

| Bundle | Installed on disk now | Freeze record (VERIFICATION-4.1:326-327) | Result |
|---|---|---|---|
| VST3 | `c0fdd8f217f37e51…bce29844a` | `c0fdd8f2…bce29844a` | ✅ |
| AU | `1e04f0a8928ac4e5…b7fda007` | `1e04f0a8…b7fda007` | ✅ |

Only `-dev` variants are on disk — no alternate-variant bundle to shadow the AU registry slot
(`critical_dev_release_variant_shadowing`). **The frozen 4.1 binary is what Logic will load.**

---

## Executive summary — what this research changes

Nine findings. **Four of them change a decision, and two of those retire a test that cannot do
what it was written to do.**

| # | Finding | Consequence |
|---|---|---|
| **N7** | **Q1's real gate is not *whether* Logic offers 7.1 — it is *which* 7.1.** JUCE documents `create7point1()` as Logic's **"7.1 (3/4.1)"** and `create7point1SDDS()` as Logic's **"7.1 (SDDS)"** (`juce_AudioChannelSet.h:190-207`). The shipped default label map uses `Lss/Rss/Lrs/Rrs`, and **all four are absent from SDDS** — the plugin's own probe E says so (`tests/unit/main.cpp:69`) | **D11 does not collapse, but it acquires a one-line pre-flight.** Pick the wrong menu entry and `buildSpeakerToBuffer` fails `labelNotInSet`, the banner raises, and every test in the phase is invalid. The observable is `getStatus.mapInvalid == false`, and it must be checked **before** the first bounce, not read back off one |
| **N8** | **Gate 13's Q5 item is VACUOUS.** The completion gate is `Component::isVisible()` (`juce_WebBrowserComponent.cpp:607-611`) — the component's **own** flag, set by `addAndMakeVisible` (`PluginEditor.cpp:1139`) and never cleared anywhere in O-Octagon. Minimise, ⌘H, occlusion and a hidden ancestor all leave it **true**. JUCE's own hidden-page path uses a *different* predicate, `isShowing()`, and `withKeepPageLoadedWhenBrowserIsHidden()` makes that branch a no-op | **"Hide the editor for 10 s and confirm the meters resume" cannot drop a completion.** It passes identically on a build with a latching guard — `pattern_zipper_sweep_probe_needs_liveness_gate`. Worse, what it *does* observe is WebKit background-timer throttling, which looks the same from outside |
| **N9** | **`diagnostics().dropped` is unreadable from the frozen binary, twice over.** `developerExtrasEnabled` is `#if JUCE_DEBUG` (`juce_WebBrowserComponent_mac.mm:881-883`) → no Web Inspector on a Release bundle; and `meters` is a module-scope binding never attached to `window` (`app.js:639`) → no handle to call it on | **D18's premise needs correcting.** "Confirmed present in shipped source" is true and insufficient — present in source ≠ reachable at runtime. Q5 closes in the **ui-stub harness**, where NC5 already lives, not in Logic |
| **N10** | **The verify-ping cannot be bounced.** `prepareToPlay` calls `verifyPing.prepare()` (`PluginProcessor.cpp:258`), which sets `phase = idle`, `command = kCmdNone`, `activeFlag = false` (`VerifyPing.cpp:79-86`). Logic calls `prepareToPlay` at the start of every offline bounce | **`COMPAT-02` criterion 2 needs a REALTIME capture, not a bounce** — and `ffmpeg`/`sox` are not installed. Rig in §4: BlackHole 64ch as Logic's device for **both** in and out (64 in / 64 out, confirmed), 8 mono tracks armed on inputs 1–8. No new tooling |
| **N11** | **The eight distinct tones must come from one-hot `w1..w8`, and `airAmount` must be 0.** `v_i` is exactly `0.0f` when `w_i == 0` (DSP-05/1), so isolation is exact. But `airCutoffHz = clamp(20000·2^(−airAmount·dHull/3), …)` (`HullProcessor.h:123`) puts a low-pass in the path whose corner depends on source position | **D15's per-band test has a confound the plugin ships itself.** Two slots fed from instances at *different* positions differ in `fc` — a false per-band delta that reads exactly like bass management and triggers D16's re-freeze on nothing |
| **N12** | **Bounce 24-bit integer PCM, not 32-bit float.** Python 3.14's `wave` accepts only `WAVE_FORMAT_PCM` and `WAVE_FORMAT_EXTENSIBLE` (`wave.py:386-387`); float is `0x0003` → `Error: unknown format: 3`. All five existing `tests/tools/*.py` are stdlib-only, and `audioop` was removed in 3.13 | Fixes the tool contract before it is written (Q6). 8-channel WAV *is* EXTENSIBLE and reads fine. **Prefer a multitone to a log sweep** — per-band then reduces to a Goertzel per partial with closed-form expectations, no FFT windowing choices |
| **N13** | **The CR-b trap reappears one level up, inside the tool.** A `--expect` permutation that is not asserted non-identity lets a CR-b run with the wrong `.venue` loaded report a CR-a pass | The anti-vacuity D20 wrote into the *test* has to be written into the *analyser* too, or it is bypassed by the one mistake it exists to catch |
| **N14** | **Q3 answered: CR-b is a committed `.venue` fixture, not a UI gesture.** `btn-venue-load` → `loadVenue` → `venuefile::load` into a **fresh** model, then `applyVenueEditChecked()` (`VenueFile.h:53-62`). Labels are `@label` abbreviations on the 8 `SPEAKER` nodes | A fixture is reviewable and re-derivable; eight typed labels are neither. It also sidesteps the UI's hold-and-mark label column, which exists because every route from (L,R) to (R,L) passes through a duplicate (`venue.js:44-49`) |
| **N15** | **Evidence named `*.log` is gitignored.** `.gitignore:217`. Three Stage-3.3 evidence logs exist today only as ignored files, in the *other* worktree | D14's evidence standard is "committed script + artifact". `.wav`, `.py`, `.txt`, `.venue` are all clean — **4.2's evidence must not use `.log`** |

**Q1 does not collapse, and D11 stands.** The gating question turned out to have a sharper edge than
discuss expected, but it is a pre-flight check rather than a rig decision.

**Two of 4.2's six questions were answered by falsifying their premise** (N8, N10). Both were
premises inherited across four or more phases without ever being run — which is exactly what
CONTEXT-4.2 said was true of Q5, and it turned out to be true of the ping too.

---

## 1. Q1 — Logic, 7.1, and the format that actually matters

### 1.1 The finding

JUCE names Logic's menu strings in its own header, and the two 8-channel 7.1 variants are
different sets:

| JUCE factory | Composition | Logic's menu string (`juce_AudioChannelSet.h`) |
|---|---|---|
| `create7point1()` | L, R, C, LFE, **Lss, Rss, Lrs, Rrs** | **"7.1 (3/4.1)"** (`:194`) |
| `create7point1SDDS()` | L, R, C, LFE, Ls, Rs, Lc, Rc | **"7.1 (SDDS)"** (`:204`) |

`RigPolicy.h:73-75` admits three containers — `create7point1()`, `create7point1SDDS()` and
`create5point1point2()`. The shipped default label map (`VenueModel.cpp:89-98`) is:

```
1→left(1)  2→right(2)  3→centre(3)  4→LFE(4)
5→leftSurroundSide(10)  6→rightSurroundSide(11)
7→leftSurroundRear(20)  8→rightSurroundRear(21)
```

`buildSpeakerToBuffer` resolves each label with `outSet.getChannelIndexForType()`
(`ChannelMap.cpp:73`) and fails `labelNotInSet` on any `-1` (`:75-76`). **Under SDDS, four of those
eight types are not members of the set.** The plugin's own probe E states it verbatim:

> `tests/unit/main.cpp:69` — *"E  Cross-container — 7.1 labels against 7.1-SDDS: 4 of 8 types absent"*

`create5point1point2()` is the same story from a different direction: it carries
`leftSurround(5)`/`rightSurround(6)` and two top channels, so `Lss`, `Rss`, `Lrs` and `Rrs` are again
absent.

### 1.2 What this means for the session

If Logic negotiates SDDS or 5.1.2, then at `prepareToPlay` → `rebuildChannelMap()`:

- `buildSpeakerToBuffer` returns false, **leaving `speakerToBuffer` untouched** (`ChannelMap.cpp:97-101`);
- `mapInvalid` goes true (`PluginProcessor.cpp:324`);
- the banner raises with the row and the reason (`PluginEditor.cpp:482-484`).

So the failure is **loud, not silent** — which is the design working. But a phase that ran CR-a
against it would be measuring a retained stale map, and the bounce would still produce eight
channels of *something*. **The pre-flight is not optional.**

### 1.3 The pre-flight, stated as an observable

Before any bounce, with the plugin instantiated on the surround track, `getStatus` must report:

| Property | Required value | Why |
|---|---|---|
| `numOutputChannels` | `8` | the bus negotiated at all |
| `mapInvalid` | `false` | **the label map resolved** — this is the whole check |
| `safeMode` | `false` | `isRealRig()` true, so not the mono/stereo fold (`RigPolicy.h:73-75`) |
| `outputSetName` | the 7.1 description | records *which* container, for the artifact |

All four are already on the status poll at 2 Hz and all four are rendered in the UI. **No code change
and no new native function.** Capture the banner state as a screenshot into the evidence directory.

> **This is Q1's answer and it is stronger than the question.** Discuss asked whether Logic would
> offer 7.1 on a virtual device. The format list is fixed at ten and is not a function of the device
> (`critical_logic_only_named_surround_formats`); what the device gates is the **I/O Assignment**
> page having eight outputs to assign, and BlackHole 64ch has sixty-four. The question that can
> actually fail is the one above, and it has a one-line answer visible in the plugin's own UI.

### 1.4 A prose correction, logged not fixed

`VenueModel.cpp:87-89` says the default is *"the identity under all three accepted 8-channel
containers"*. It is the identity under `create7point1()`; under the other two it does not resolve at
all. The *operative* claim in that comment — that a map test driven by the default alone is vacuous
— is correct and is the reason D20 exists. **Recommendation: a v1.1 doc row, not a re-freeze.** The
behaviour is right, tested and banner'd; only the sentence overstates. Raised as **P-decision
candidate** in §9.

---

## 2. Q2 — Surround Bounce on BlackHole

**This cannot be settled from source, and should not be assumed.** It is a host behaviour, and §6a's
bounce material is drawn from user reports about 5.1 rather than Apple documentation — the research
doc rates the whole area MEDIUM and says *"TEST BEFORE RELYING ON IT"* (`:195`).

What *is* established from §6a and stands as constraint rather than question:

- Interleaved surround bounce produces **one N-channel file**; split produces N mono files.
- **WAV, AIFF, CAF only** — compressed surround is refused with a dialog.
- Bounces are unencoded PCM.
- Route: File → Bounce → Project or Section, tick **Surround Bounce**.

**Recommendation: make Q2 the first two minutes of the session, not an inference.** A 4-second bounce
of a 7.1 project, with the file's channel count read off by `analyse_bounce.py --probe`, settles it
before any test material is built. If it fails, D13 fails and the phase stops there having spent two
minutes — which is the point of putting it first.

---

## 3. Q4 and Q5 — the hidden WKWebView, and why the test cannot work

This is the finding that changes the most, so the chain is given in full.

### 3.1 The gate is `isVisible()`, and `isVisible()` is not `isShowing()`

```cpp
// juce_WebBrowserComponent.cpp:607-611
void WebBrowserComponent::emitEventIfBrowserIsVisible (const Identifier& eventId, const var& object)
{
    if (isVisible())
        impl->emitEvent (eventId, object);
}
```

```cpp
// juce_Component.h:131
bool isVisible() const noexcept   { return flags.visibleFlag; }
```

`visibleFlag` is the component's **own** flag. It is set by `setVisible()` and by nothing else. It
does **not** track the peer, the window state, or any ancestor: `isShowing()` is the predicate that
walks the parent chain, and it is a different function.

In O-Octagon the flag is set once —

```cpp
// PluginEditor.cpp:1139
addAndMakeVisible (*webView);
```

— and **`setVisible(false)` is never called on the web view anywhere in `Source/`.**

### 3.2 JUCE's own hidden-page handling uses the other predicate, and O-Octagon disables it

```cpp
// juce_WebBrowserComponent_mac.mm:966-990
void checkWindowAssociation() override
{
    auto& browser = owner.owner;
    if (browser.isShowing())          // <-- isSHOWING, not isVisible
    { browser.reloadLastURL(); handleAsyncUpdate(); }
    else
    {
        if (browser.unloadPageWhenHidden && ! browser.blankPageShown && …)
        { browser.blankPageShown = true; goToURL (blankPageUrl, …); }
    }
}
```

`unloadPageWhenHidden` is `! options.keepsPageLoadedWhenBrowserIsHidden()`
(`juce_WebBrowserComponent.cpp:587`), and `PluginEditor.cpp:266` sets
`.withKeepPageLoadedWhenBrowserIsHidden()`. **So the `else` branch does nothing at all.**

### 3.3 Therefore

| Gesture | `isShowing()` | `isVisible()` | Completions | Page |
|---|---|---|---|---|
| Minimise the Standalone window | false | **true** | **delivered** | stays loaded |
| ⌘H / hide the app | false | **true** | **delivered** | stays loaded |
| Another window occludes it | true | true | delivered | loaded |
| Switch Spaces | false | **true** | **delivered** | stays loaded |
| `setVisible(false)` on the web view | — | false | **DROPPED** | loaded |

Only the last row reaches the hazard, and nothing in O-Octagon performs it.

**PLAN-3.3.md:821's Q5 item — *"hide the editor for 10 s with the meter poll live, re-show, and
confirm the meters RESUME"* — therefore cannot drop a completion.** It would pass identically on a
build whose guard latched permanently. That is `pattern_zipper_sweep_probe_needs_liveness_gate`: a
probe whose stimulus never reaches the mechanism.

**And it is worse than merely vacuous — it is confusable.** What minimising *does* cause is WebKit
background-timer throttling: the 33 ms `setInterval` is clamped in a hidden page and recovers on
re-show. The meters freeze and then resume. An observer running the test as written sees exactly
the behaviour it asks for, produced by a mechanism it is not testing, and ticks it.

One further point closing the Debug-build escape route: `jassert (owner.isVisible())` sits
immediately above the emit (`juce_WebBrowserComponent.cpp:344`). In a Debug build the drop path
**asserts** rather than drops, so a Debug build is not a vehicle for observing it either.

### 3.4 Q4 answered: the readout does not exist on the frozen binary

Two independent blockers, either sufficient:

1. **No inspector.** `[preferences setValue:@(true) forKey:@"developerExtrasEnabled"]` is wrapped in
   `#if JUCE_DEBUG` (`juce_WebBrowserComponent_mac.mm:881-883`). The freeze is a Release build.
2. **No handle.** `meters` is a module-scope `let` in `app.js`, assigned at `:639`. Nothing attaches
   it to `window` or `globalThis` — a grep for `diagnostics` across `Source/ui/public/js/` finds
   exactly two definition sites (`meters.js:227`, `field.js:209`) and **zero call sites**.

CONTEXT-4.2 D18 records `diagnostics` as *"confirmed present in shipped source"*. That is true. It is
also not the property the test needs: **present in source ≠ reachable at runtime.**

### 3.5 Recommended disposition

**Split Q5 from Gate 13 and close each where it can actually be closed.**

| Half | Where | Method | Evidence |
|---|---|---|---|
| **The mechanism** — does the deadline guard release when a completion never settles? | **`tests/ui-stub/`**, on the desk | Exactly NC5's construction (`PLAN-3.3.md:595`), pointed at the meter poll: serve the page with the real stub, make `getMeters` return a never-settling promise for one tick, assert `dropped` increments and the poll continues. This is the method that measured N9 | scripted, re-derivable, committed |
| **The host behaviour** — does the page survive being hidden in Logic for 10 s? | Logic, in session | Minimise / ⌘H for 10 s, re-show, confirm the meters follow the source again | screenshot + a line in VERIFICATION-4.2 |

The second is worth running and worth keeping. **It must be relabelled**: it is a
*throttling-recovery smoke check*, not a completion-drop test, and VERIFICATION-4.2 must say so or
the next reader inherits the same false premise a fifth time.

**Recommendation against a source change.** Exposing `meters.diagnostics` on `window` would make the
counter readable, and it could ride D19's already-scheduled pre-session re-freeze. **Do not.** The
mechanism closes in the stub without it, and widening a re-freeze that currently touches one
doc-comment into one that touches shipped page behaviour trades a real risk for a convenience.

---

## 4. `COMPAT-02` criterion 2 — the verify-ping cannot be bounced

### 4.1 The finding

```cpp
// PluginProcessor.cpp:258
verifyPing.prepare (sampleRate);
```

```cpp
// VerifyPing.cpp:79-86
b0 = b1 = b2 = 0.0f;
phase        = Phase::idle;
phaseCounter = 0;
runCounter   = 0;
speaker      = 0;
mode         = 0;
command.store   (kCmdNone, std::memory_order_release);
activeFlag.store (false,   std::memory_order_release);
```

`prepare()` unconditionally returns the ping to idle. Logic calls `prepareToPlay` at the start of
every offline bounce. **A ping started before the bounce is dead by sample 0 of it**, and there is no
way to press the UI button "during" an offline render.

`PluginProcessor.cpp:255-257` describes this as intended — *"A prepare() that arrives mid-ping
restarts the ping's own state"* — so this is a correct behaviour with an inconvenient consequence,
not a defect.

### 4.2 The rig this forces

Criterion 2 needs a **realtime** per-channel capture, and neither `ffmpeg` nor `sox` is installed.
The rig that needs no new tooling:

- BlackHole 64ch is Logic's device for **both input and output** — it is 64-in / 64-out
  (confirmed via `system_profiler SPAudioDataType`), so its outputs appear as its inputs.
- Eight mono audio tracks, inputs 1–8, record-armed.
- Roll the transport, start the ping in **auto-cycle**, let it run.

The ping's timing is fixed and derivable, which is what makes the capture assertable rather than
observed: 1.2 s on / 0.4 s gap, and *"8 × (on + gap) is exactly 12.8 s regardless of the fade
length"* (`VerifyPing.h:55-57`). Signal is band-limited pink noise — 200 Hz HP, 8 kHz LP, RMS
normalised, measured crest 4.21 so the steady peak lands near −7.5 dBFS (`VerifyPing.h:96-102`).

**The assertion:** in each of the eight 1.6 s windows, exactly one of the eight captured channels
carries energy above a stated floor, and the sequence of energised channels is `1..8` in order. That
is per-channel sample data, not eight moving meters — the "strictly stronger evidence" D11 claims,
now with a mechanism behind it.

> **Recording arms a loopback.** Logic will be recording the device it is playing to. This is fine
> for the assertion (which is about *which channel carries energy in which window*, not about
> latency), but the recorded files will carry the round-trip delay. The analyser must not assume
> sample-zero alignment — window boundaries are found from the energy envelope, not from the file
> start.

---

## 5. CR-a / CR-b — where the eight tones come from

### 5.1 One-hot weights, because the ping is unavailable

The parameter set (`GainStage.h:59-66`) is:

```
srcX, srcY, srcZ, width, rolloff, blur, w1..w8, hullAtten, airAmount, outputGain   // 17
```

Isolation comes from the weights and it is exact, not approximate: `v_i` is **exactly `0.0f`**
whenever `w_i == 0` (DSP-05/1, restated at `PluginProcessor.cpp:357-359`). A one-hot weight vector
routes the input to precisely one speaker slot with no leakage into the other seven.

Placing the source *at* a speaker position is **not** equivalent and must not be substituted: DBAP's
`1/d^a` falloff with the blur radius `r_s` leaves every other speaker non-zero.

### 5.2 `airAmount` must be 0, and this is N11's real weight

```
// HullProcessor.h:123, 139-148
fc = clamp (20000 * 2^(-airAmount * dHull / 3),  500,  min (20000, 0.45 * fs))
```

The air filter is applied **to the source**, before the gain vector (`GainStage.cpp:226-227`), so it
is common-mode across all eight outputs and cancels in any slot-vs-slot differential — **provided
both slots are fed from the same instance at the same source position.**

They are not, in the natural construction. Eight instances at eight different positions have eight
different `dHull` values and therefore eight different corner frequencies.

**For CR-a/CR-b** that muddies tone amplitudes without breaking identification (the analyser takes an
argmax, not a level).

**For D15's LFE test it is a live trap.** The whole test is "compare LFE slot against a reference
slot per band, and read HF attenuation as bass management". A per-band HF delta produced by two
different `fc` values is indistinguishable from the thing being looked for — and D16's disposition
for a positive result is *fix, re-freeze, re-run 4.1's 18 gates*. **A confound here costs hours and
lands a compensating trim the plugin does not need.**

**Rule for every bounce test in this phase: `airAmount = 0`.** It pins `fc` at the ceiling
independently of position and takes the filter out of the path entirely. Belt and braces: feed the
LFE slot and its reference from the same source position as well.

Obviously the **audible-clause** test does the opposite — the air filter is the artifact under test
there. Stated so the rule is not applied where it would erase the measurement.

### 5.3 The expected channel table, derived rather than transcribed

From the enum (`juce_AudioChannelSet.h:407-430`) and the bitset ordering rule
(`critical_audiochannelset_is_a_bitset_not_an_order` — buffer order is **ascending enum-bit order**,
which `verifyEnumBitOrder` asserts at `ChannelMap.cpp:105-151`):

| Speaker | Default label | Abbrev | Enum value | Buffer index | Bounce channel (CR-a) |
|---|---|---|---|---|---|
| 1 | `left` | `L` | 1 | 0 | 1 |
| 2 | `right` | `R` | 2 | 1 | 2 |
| 3 | `centre` | `C` | 3 | 2 | 3 |
| 4 | `LFE` | `Lfe` | 4 | 3 | 4 |
| 5 | `leftSurroundSide` | `Lss` | 10 | 4 | 5 |
| 6 | `rightSurroundSide` | `Rss` | 11 | 5 | 6 |
| 7 | `leftSurroundRear` | `Lrs` | 20 | 6 | 7 |
| 8 | `rightSurroundRear` | `Rrs` | 21 | 7 | 8 |

Identity confirmed. Abbreviations are JUCE's own
(`juce_AudioChannelSet.cpp:168-197` write, `:291-310` parse) and are what `@label` stores.

### 5.4 CR-b's fixture (Q3 answered)

`VenueModel.h:66` gives the tree shape — `SPEAKER × 8 { @index, @x, @y, @z, @trimDb, @label }` — and
`.venue` is that subtree as XML (`VenueFile.h:83-89`). The load path is
`btn-venue-load` → `loadVenue` (`venue.js:406-408`, `PluginEditor.cpp:627`) → `venuefile::load` into
a **fresh** model → `applyVenueEditChecked()`.

**Recommendation: a committed fixture, `tests/fixtures/cr-b-permuted.venue`**, carrying a
non-identity label permutation and otherwise identical to the shipped default venue.

Three reasons it beats typing eight labels in the UI:

1. **Re-derivable.** D14's standard. A fixture is diffable; eight gestures are not.
2. **It sidesteps the label column's hold-and-mark behaviour**, which exists precisely because
   *"EVERY ROUTE FROM (L, R) TO (R, L) PASSES THROUGH A DUPLICATE"* (`venue.js:44-49`). Typing a
   permutation means passing through rejected intermediate states; loading one does not.
3. **It is the same object for all eight instances**, so CR-b's setup is one repeated action rather
   than sixty-four typed cells.

**Choose a derangement, not a transposition.** A two-element swap leaves six speakers on the
identity, so six of the eight columns prove nothing — the same vacuity D20 caught, at one-quarter
scale. A full 8-cycle (`1→2→3→…→8→1` in label terms) moves every speaker.

### 5.5 Project construction

**Recommended: eight surround tracks, eight instances, one bounce.**

| | |
|---|---|
| Each track | one distinct tone → one O-Octagon instance → one-hot `w`, `airAmount = 0`, identical `srcX/Y/Z` |
| Track *k* | tone *k*, `w_k = 1`, all other `w = 0` |
| Bounce | interleaved, Surround Bounce, **24-bit PCM WAV** (N12) |
| CR-a | shipped default venue |
| CR-b | `cr-b-permuted.venue` loaded into all eight |

A single artifact per run, and the analysis is a plain 8 × 8 Goertzel matrix with no segmentation
logic — which keeps the committed tool small enough to review.

If one instance fails to take the CR-b fixture, the result is a **mixed** matrix, which the analyser
reports as a mismatch. The failure is detected, not silent. That is worth stating explicitly because
it is the reason eight loads is an acceptable amount of manual work.

*Alternative considered:* one instance, one bounce, `w1..w8` automated one-hot across eight
2-second segments with the eight tones sequenced on the input track. Fewer instances, but it moves
complexity into Logic automation and into per-segment logic in the tool. **Not recommended** — the
tool is the evidence, and it should be the simplest thing that discriminates.

### 5.6 Tone selection

Bin-exact Goertzel at `N = fs` gives 1 Hz bins, so any integer frequency is exact and no windowing
choice enters the measurement. Distinct primes, comfortably inside the band and clear of the ping's
own 200 Hz / 8 kHz corners:

```
997, 1499, 2003, 2503, 3001, 3499, 4001, 4507   Hz
```

The path is linear gain (plus a first-order filter when `airAmount > 0`), so harmonic generation is
not a concern; primes are cheap insurance against an arithmetic slip in the expectation table rather
than a physical requirement.

---

## 6. D15 — the LFE test, multitone rather than sweep

**Recommendation: a multitone, not a log sweep.**

A sweep's per-band decomposition needs an FFT, a window and a band-edge convention — three constants
that have to be right, in a project that has already shipped three metrics whose own constants were
wrong (`pattern_metric_window_vs_modulation_period`). A multitone reduces "per band" to "per
partial", each one a Goertzel with a closed-form expected value.

**Partials** — bin-exact at `N = fs`, log-spaced, bracketing any plausible bass-management crossover
(typically 80–120 Hz) and reaching high enough to see a full-range difference:

```
31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000   Hz
```

**Level:** §6a specifies −20 dBFS. With ten partials the composite crest matters — use Schroeder
phases and target a composite peak ≤ −12 dBFS, with the generator recording the exact per-partial
level into the artifact so the expected deltas are derivable rather than remembered.

**Construction:** two tracks, both `airAmount = 0`, both at the same `srcX/Y/Z`, one-hot on speaker 4
(the LFE slot under the identity map) and on speaker 1 (reference). One bounce.

**Verdicts:**

| Observation | Reading |
|---|---|
| Broadband delta ≈ 0 **and** every partial delta ≈ 0 | `VenueModel.cpp:84` **confirmed**, and can cite a measurement instead of an assumption |
| Broadband delta ≈ +10 dB, flat across partials | Logic applies the LFE gain offset → **D16** |
| Delta ≈ 0 at 31–125 Hz, increasingly negative above | Bass-management low-pass → **D16** |
| Deltas that do not fit any of the above | Check `airAmount` is actually 0 and both sources are co-located **before** invoking D16 (N11) |

That last row is the finding earning its place: it is the check that stops a confound from costing a
re-freeze and eighteen re-run gates.

---

## 7. Q6 — the `analyse_bounce.py` contract

### 7.1 Dependencies: stdlib only

All five existing tools in `tests/tools/` are stdlib-only — a grep for `numpy`/`scipy`/`soundfile`
across them returns nothing. numpy 2.4.0 and scipy 1.17.0 *are* installed locally, but adding them
here would make this the only tool in the repo that cannot run on a bare interpreter. Nothing in this
analysis needs them: Goertzel is six lines.

### 7.2 File format constraints, measured

```python
# wave.py:85-86, 386-387  (Python 3.14)
WAVE_FORMAT_PCM        = 0x0001
WAVE_FORMAT_EXTENSIBLE = 0xFFFE
...
if wFormatTag != WAVE_FORMAT_PCM and wFormatTag != WAVE_FORMAT_EXTENSIBLE:
    raise Error('unknown format: %r' % (wFormatTag,))
```

| Point | Consequence |
|---|---|
| 8-channel WAV is written as `WAVE_FORMAT_EXTENSIBLE` | **reads fine** |
| 32-bit float is `WAVE_FORMAT_IEEE_FLOAT` (`0x0003`) | **rejected** → `Error: unknown format: 3` |
| `audioop` removed in Python 3.13 (PEP 594) | 24-bit needs a manual 3-byte little-endian sign-extending unpack (~8 lines) |

**→ Bounce 24-bit integer PCM.** At −20 dBFS the 24-bit floor is ~124 dB down; nothing in this
phase is anywhere near it.

### 7.3 Interface

```
analyse_bounce.py --input <bounce.wav> --mode {order,lfe,probe} [--expect <perm>]
                  [--tones <csv>] [--partials <csv>] [--check]
```

| Mode | Does |
|---|---|
| `probe` | prints channel count, sample rate, bit depth, duration. **Q2's two-minute pre-flight** |
| `order` | 8 × 8 Goertzel matrix; per channel, argmax tone and isolation in dB vs the runner-up |
| `lfe` | per-partial magnitude for two named channels; prints the per-partial delta table and the broadband delta |

### 7.4 Anti-vacuity clauses — the part that is load-bearing

Mirroring `gen_dbap_reference.py`'s stated discipline (*"Exits non-zero on any failure and NEVER
emits a fixture with zero cases: a vacuous oracle is worse than no oracle, because it reports
green"*, `:56-57`):

1. **`--expect` is mandatory in `order` mode.** No default. A default would be the identity, and the
   identity is exactly what D20 forbids driving a map test with.
2. **For CR-b, `--expect` must be asserted non-identity** — the tool refuses to report a pass when
   `--expect` is `1,2,3,4,5,6,7,8` and the invocation is labelled CR-b. **This is N13**: without it,
   a CR-b run against a venue that silently failed to load reports a clean CR-a pass, and the check
   is bypassed by the one mistake it exists to catch.
3. **Every expected tone must be found somewhere.** If a tone is absent from all eight channels the
   run is a **failure**, not a partial pass — a missing tone means a track was muted or an instance
   was not one-hot, and a 7-of-8 match must not read as green.
4. **Isolation floor.** Each channel's argmax must exceed its runner-up by a stated margin (≥ 40 dB
   is generous given exact-zero weights). Prints the measured margin so the number is in the record
   rather than the threshold alone.
5. **Non-zero exit on any failure**, and no "OK" line on zero cases analysed.
6. **`--check`** re-runs the committed expectation table against the committed WAV, so the pair stays
   verifiable after the session — the `gen_dbap_reference.py --check` precedent, which is Gate 15.

### 7.5 Window discovery

Per §4.2's loopback note: window boundaries come from the energy envelope, not from sample zero.
For `order` mode on a bounce this is moot (the tones are steady for the whole file), but the same
tool serves the realtime ping capture, where it is not.

---

## 8. Q5 (material) — the audible clause's source

The repo ships **no audio assets**; `tests/fixtures/` holds one generated header. So "HF-rich" has to
be turned into a named file by this phase, per CONTEXT-4.2's own framing that it *"needs to be a
named file, not an adjective"*.

**Recommendation — two sources, and they answer different questions:**

| Source | Role | Why |
|---|---|---|
| **Committed generator** — `tests/tools/gen_audible_source.py`, emitting a deterministic bright signal (sustained 6–16 kHz content plus transients) | **primary** | D14's standard: re-derivable by anyone, byte-identical on re-run, and the difference-signal half of D12 needs a source whose null render is exactly reproducible |
| **One named commercial/Apple Loop**, bright percussion | **ecological cross-check** | D12's full-bounce half asks whether it ticks *in context*. Synthetic material is the wrong judge of that. Named in VERIFICATION-4.2, not committed (licence) |

**Both halves of D12 need the same gesture**, and the difference signal requires two renders that
differ *only* in the gesture — same source file, same start time, same everything else. That is
straightforward given 4.1 proved bit-reproducibility, but it does mean the null render must be a
genuinely static source position, not "roughly the same place".

**D12's stated residual stands and must be discharged in the artifact: name the headphones.** A
one-sample step of ~15 % of an 8 kHz component is exactly what a cheap transducer hides, and an
unnamed monitoring path makes the whole clause unreproducible.

---

## 9. Handoff to plan

### 9.1 Session order — derived, not arbitrary

1. **Pre-session, at the desk:** D21 (commit the 4.1 verify artifacts), D19 (Gate 16b re-spell) and
   its re-freeze, then re-run 4.1's gates against the new freeze. Write `analyse_bounce.py`,
   `gen_audible_source.py` and `cr-b-permuted.venue`. Run Q5's stub half (§3.5).
2. **Session, first two minutes:** Q2's bounce pre-flight (§2). **If it fails, stop.**
3. **Session:** the `getStatus` pre-flight (§1.3) — `mapInvalid == false`. **If it fails, fix the
   format selection before anything else.**
4. `COMPAT-02` criteria 1 and 3 (instantiation, session recall, automation lanes).
5. Criterion 2 — realtime ping capture (§4.2).
6. CR-a, then CR-b (§5.5).
7. The LFE multitone bounce (§6).
8. The audible clause: gesture render + null render (§8).
9. Gate 13's interactive half, including the relabelled throttling-recovery check (§3.5).

Steps 2 and 3 are cheap and gate everything after them. Step 1's re-freeze must complete before the
session or the phase runs against a binary that is about to be replaced.

### 9.2 Plan decisions this research asks for

| # | Decision needed | Recommendation |
|---|---|---|
| **P101** | Gate 13's Q5 item is vacuous (N8) | Split it: mechanism → ui-stub (NC5's construction); host half → relabel as a throttling-recovery smoke check. **Do not** add a `window` handle to read `diagnostics` |
| **P102** | Criterion 2's rig, given N10 | Realtime loopback capture on BlackHole 64ch in/out, 8 armed mono tracks. No new tooling, no `ffmpeg` install |
| **P103** | `airAmount = 0` on every bounce test except the audible clause (N11) | Adopt as a stated rule, and add the "check this before invoking D16" row to the LFE verdict table |
| **P104** | Bounce format | 24-bit integer PCM WAV, interleaved (N12) |
| **P105** | `analyse_bounce.py`'s six anti-vacuity clauses (§7.4), especially the non-identity assertion (N13) | Adopt verbatim; they are the tool's contract |
| **P106** | CR-b fixture shape | A committed 8-cycle derangement, not a transposition (§5.4) |
| **P107** | `VenueModel.cpp:87-89`'s "all three containers" prose (N7 §1.4) | v1.1 doc row. **Not** a re-freeze — behaviour is correct and banner'd |
| **P108** | Evidence naming (N15) | No `*.log`. `.txt`/`.wav`/`.py`/`.venue` under `stages/4-polish/evidence/` and `tests/` |

### 9.3 Named deferrals — unchanged by this research

Every item CONTEXT-4.2 listed under *"What 4.2 will NOT close"* is untouched: the hardware-driver
half of `COMPAT-02`/2, Windows UI correctness, RT-safety beyond allocation, the two JS gates in CI,
spatial coherence in a hall, and `ARCHITECTURE.md`'s three unreconstructible intermediate checksums.
Owners remain as recorded. **This research adds no new deferral.**

### 9.4 What this research did not establish

- **Q2 is not answered**, only made cheap to answer and placed first (§2). Bounce behaviour on a
  virtual device is a host property and no amount of source reading settles it.
- **Logic's canonical 7.1 bounce order remains the MEDIUM-confidence claim** it was; CR-a is what
  moves it, and CR-a has not been run.
- **No claim is made about `create7point1SDDS()` or `create5point1point2()` in Logic** beyond what
  the type composition forces. Whether Logic ever negotiates them for this plugin is unobserved —
  §1.3's pre-flight exists because it is unobserved.

---

## Next Phase

**Ready for:** plan phase — P101–P108, against the frozen binary and the session order in §9.1.

The gating question changed shape rather than failing: **D11 stands, and Q1 reduces to a one-line
pre-flight** (`mapInvalid == false`) that must be read before the first bounce rather than off one.
The two premises that did fail — Q5's hidden-editor test and the assumption that the verify-ping
could be bounced — both failed the same way, by having been carried across several phases without
ever being run.
