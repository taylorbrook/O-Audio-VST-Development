# Stage 3 — GUI · Phase 3.2 (Venue screen, venue store, verify-ping) — Research

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI · **Phase 3.2 of 3**
**GSD phase:** research
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 / 3.1 work uncommitted)
**Answers:** the eleven questions in `CONTEXT-3.2.md`
**Sources:** JUCE 8.0.14 in-tree, `modules/persistence/preset-manager` v1.0.5, O-Octagon `Source/`,
one **measured** Chromium render at 1100 × 720

---

## Entry Check — the four contracts

Re-run at this boundary, before anything else
(`pattern_promotion_checksum_pins_replaced_file`):

| Contract | SHA-256 measured now | STATUS frontmatter | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `a8a358f4…9b6d4408` | `a8a358f4…9b6d4408` | ✅ |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | `aec7d0ce…0137ee29` | ✅ |

**All four byte-exact. No contract is amended and no pin moves at this boundary.** The three §8
re-pins remain scheduled for 3.3 discuss, untouched.

---

## Executive summary — what this research changes

Five findings live in JUCE's or the module's own source and **could not have been known at discuss**.
Two of them change a decision; one of them is a defect class that is live in shipped code today.

| # | Finding | Consequence |
|---|---|---|
| **N4** | A native function's `complete()` is gated on `isVisible()`. `emitCompletionEvent` does `jassert (owner.isVisible())` then calls `emitEventIfBrowserIsVisible`, which **drops the event** when the component is hidden (`juce_WebBrowserComponent.cpp:336-344, 607-611`) | **Q4's hole is real and BROADER than "the editor died".** With `withKeepPageLoadedWhenBrowserIsHidden()` the page stays alive while hidden, so the promise never settles **and the page is still there to have awaited it**. Every `await nativeFn(...)` in 3.2 needs a settle-or-abandon rule |
| **N5** | `applyPresetJson` calls `setValueNotifyingHost` **directly on the parameter**, not through `ParameterAttachment` (`OuariconPresetManager.h:325-341`) | **F3's "an unchanged write is skipped" does NOT apply**, and neither do gesture brackets. A preset load emits **up to 34 unbracketed host writes** (17 reset + up to 17 apply). Q1 answered: yes, the writes happen; they are bare `kAudioUnitEvent_ParameterValueChange` / VST3 `performEdit` with no Begin/End |
| **N8** | **A `mapInvalid` venue does not play through the retained map — it falls to the SAFE fold.** `mappedOutputAvailable()` is false, so `GainStage` takes the `else` arm and writes `out[ch][n] = ch == 0 ? sL : sR` (`GainStage.cpp:408, 461`; `PluginProcessor.cpp:264-268, 430`) | **Q7's label-swap transient is AUDIBLE and wrong**, not silent-and-retained: speaker 1 gets L and **speakers 2–8 all get R at unity**. Probe F's "retains the last valid map" is a claim about the *data*, not about what comes out of the PA. This changes the answer to Q7 |
| **N6** | `PresetManager`'s JS constructor wires **TEN** native functions, and `createPresetBar()` does `container.innerHTML = …` (`preset-manager.js:107-119, 411-434`) | Adopting the module's **JS** costs +10 bridge entries and re-imports `pattern_js_state_updater_overwrites_html_labels`. **Adopt the C++ header, write O-Octagon's own JS.** The FUNC-05 guarantee lives entirely in the header |
| **N7** | `buildSpeakerToBuffer` already separates three failure modes and throws the distinction away in a `bool` (`ChannelMap.cpp:45-75`); the same file already has the `juce::String* whyNot` out-param precedent (`:78-86`) | Q9 answered **yes**, and it is nearly free — no new logic, an in-file precedent, and **no new atomic** (P43) because both writer and reader are the message thread |

**Measured, not estimated (Q11):** the 42-field table is **not** the constraint — it renders at
**752 × 277 px** inside 592 px of available height, using 47 % of it. **The rail overflows by 9 px,
and the mini-plan is why**: width-bound at the portrait aspect 0.800 it demands 348 px of height.
Height-binding it — exactly how `roomplan.js` fits the big plan into the stage — gives **270 × 337 px**
and the rail fits at **592 == 592**. **D9 survives and D7 survives.**

**Two gates enumerate JS files by name and will silently under-cover 3.2's new files.**
`ui_frontend_check.js` §3 scans `[S.appJs, S.roomJs]` and §19 tests `S.appJs` alone. A new
`venue.js` that is not added to both arrays passes both gates **by not being looked at**. This is the
single most important line item to hand to plan.

---

## 1. Findings verified in JUCE 8.0.14 and module source

### N4 — a native-function completion is dropped when the browser is hidden

This is the mechanism behind Q4, and the pattern in the knowledge base does not cover it.

```cpp
// juce_WebBrowserComponent.cpp:336-344
void emitCompletionEvent (int64 resultId, const var& object)
{
    DynamicObject::Ptr eventObject { new DynamicObject() };
    eventObject->setProperty ("promiseId", resultId);
    eventObject->setProperty ("result", object);

    jassert (owner.isVisible());
    owner.emitEventIfBrowserIsVisible (NativeEvents::Invoke::completeId, eventObject.get());
}

// juce_WebBrowserComponent.cpp:607-611
void WebBrowserComponent::emitEventIfBrowserIsVisible (const Identifier& eventId, const var& object)
{
    if (isVisible())
        impl->emitEvent (eventId, object);
}
```

**Every** `complete(...)` in `PluginEditor.cpp` travels this path — the three that exist today and
every one 3.2 adds. In a Debug build a hidden component trips the assert; **in Release the completion
is discarded with no error, no rejection and no log line**, and the page's `await` never settles.

Three things follow that the discuss-phase framing did not anticipate:

1. **The hazard is not confined to teardown.** `OctagonEditor` passes
   `.withKeepPageLoadedWhenBrowserIsHidden()` (`PluginEditor.cpp:159-162`), which sets
   `unloadPageWhenHidden = false` (`juce_WebBrowserComponent.cpp:587`). The page therefore survives
   being hidden, so a promise abandoned during a hidden interval is abandoned **inside a page that
   is still running** and will still be running when the component is shown again.
2. **It applies to the existing 2 Hz poll too.** `pollStatus()` (`app.js:355-359`) awaits
   `getStatus`. It already has a `try/catch`, but a dropped completion is not a rejection — the
   `await` simply never returns, so **that iteration of the poll leaks a pending promise**. It is
   harmless today because `setInterval` fires the next one regardless (`app.js:394`), which is worth
   recording as the reason 3.1 never saw this.
3. **`FileChooser` is the sharp case** because it is the one call whose completion can arrive
   *seconds* after the invocation, with a native modal in between. A user who closes the plugin
   window while the Save dialog is open produces exactly the abandoned-promise state.

**Recommended rule for plan — "no bare `await` on a native function whose completion can outlive the
gesture":** any UI state that a native call is going to change must be driven by the **next poll or
the next explicit refresh**, not solely by the resolution of the promise. Concretely, the venue
save/load path should refresh from `getVenueGeometry` on the `venueGen` change the poll already
watches, so the UI converges even when the completion is lost. The chooser's own promise may be left
pending — it is not the thing the UI depends on.

**This is additive to the SafePointer rule, not a replacement.** Constraint 7 still holds exactly as
written (`return` bare on a dead pointer, never `complete(false)` —
`pattern_webview_launchasync_safepointer_no_complete`). N4 says that even on a *live* pointer the
completion may not arrive.

**A second, related trap in the same file.** An invocation naming a function that C++ never
registered hits `jassertfalse; return;` (`juce_WebBrowserComponent.cpp:306-312`) — again **no
completion is emitted**, so the promise never settles. The ui-stub does **not** model this: it
returns `Promise.reject(new Error("Unregistered native function: …"))`
(`tests/ui-stub/juce-stub.js:281-283`). **The stub fails loudly where the plugin hangs silently.**
That asymmetry is safe only because §3's three-way grep-diff catches the gap statically — which is
precisely why §3's file enumeration must be extended (§4 below).

### N5 — a preset load emits up to 34 unbracketed host writes (Q1)

The reset loop and the apply loop both call `setValueNotifyingHost` **directly on the parameter
object**:

```cpp
// OuariconPresetManager.h:325-341
for (int pass = 0; pass < 2; ++pass) {
    const bool metaPass = (pass == 0);
    for (auto* param : parameters.processor.getParameters())
        if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (rangedParam->isMetaParameter() == metaPass)
                rangedParam->setValueNotifyingHost (rangedParam->getDefaultValue());
}
// … then the same two-pass shape applying the preset's own values
```

**All 17 of O-Octagon's parameters are `AudioParameterFloat` and none overrides
`isMetaParameter()`** (it returns `false` in the base class,
`juce_AudioProcessorParameter.cpp:125`), so pass 0 matches nothing and pass 1 matches all 17. **The
meta machinery is a no-op here; the count is exactly 17 reset writes**, plus one per key present in
the preset.

Two consequences, both verified in the chain:

1. **`ParameterAttachment`'s unchanged-value skip does not protect this path.**
   `callIfParameterValueChanged` (`juce_ParameterAttachments.cpp:88-95`) is a member of the
   *attachment*, and the preset manager never goes through one. So a preset load from an untouched
   default patch still emits 17 writes of the values the parameters already hold — F3's saving grace
   at 3.1 is absent here.
2. **There are no gesture brackets.** `setValueNotifyingHost` is
   `setValue` + `sendValueChangedMessageToListeners`
   (`juce_AudioProcessorParameter.cpp:59-63, 111-121`) and nothing else; `beginChangeGesture` /
   `endChangeGesture` are separate functions (`:65-109`) that this path never calls. The wrappers
   turn that into:
   - **AU** — `sendAUEvent (kAudioUnitEvent_ParameterValueChange, index)` with **no**
     `kAudioUnitEvent_BeginParameterChangeGesture` / `End`
     (`juce_audio_plugin_client_AU_1.mm:1341-1360`);
   - **VST3** — `paramChanged (…)` with no `beginGesture`/`endGesture`, which are only reached via
     `parameterGestureChanged` (`juce_audio_plugin_client_VST3.cpp:1498-1501, 1652-1663`).

**So the answer to Q1 is: yes, the writes are emitted, and they are bare.** Whether Logic records
them into an armed Latch/Touch lane is host behaviour this research cannot settle from source — but
the shape is exactly the one N1 flagged at 3.1, one layer up, and the repo's standing rule is that a
host-visible write without a bracket is a defect waiting for a host that disagrees.

**FUNC-05 criterion 1 is not threatened.** The writes are confined to the 17 APVTS parameters;
they cannot reach `VENUE`, which is the criterion. What is threatened is the *musical* experience of
loading a preset while automation is armed.

**Recommended disposition for plan — bracket the load, do not fork the module.** Wrap the
`loadPreset` call site in O-Octagon's own code with `beginChangeGesture()` on all 17 before and
`endChangeGesture()` on all 17 after. That converts up to 34 loose writes into 17 well-formed
gestures, is ~6 lines in `PluginEditor.cpp`, changes no shared module, and generalises P39's rule
("open a gesture on every parameter you will write, and close all of them") from the puck to the
preset store. **Do not edit `OuariconPresetManager.h`** — four other plugins depend on it and this
boundary is not the place to move a shared module.

### N8 — `mapInvalid` is audible, and it is not the retained map (Q7)

This is the finding that changes a decision, and it is live in shipped Stage-2 code.

`rebuildChannelMap()` retains the last valid map on rejection — `buildSpeakerToBuffer` writes `out`
only on the success path (`ChannelMap.cpp:70-74`) and the snapshot is published either way
(`PluginProcessor.cpp:293-297`). That is probe F's guarantee and it holds exactly as written.

**But the audio thread is then stopped from using it.**

```cpp
// PluginProcessor.cpp:264-268
bool OOctagonProcessor::mappedOutputAvailable (int numOutputChannels) const noexcept
{
    return numOutputChannels == ochan::kNumSpeakers
        && ! mapInvalid.load (std::memory_order_acquire);
}
// PluginProcessor.cpp:430
const bool mapped = mappedOutputAvailable (numOut);
```

With `mapInvalid == true` on an 8-channel bus, `mapped` is false, `GainStage::renderChunk` takes the
`else` arm (`GainStage.cpp:408`) and writes:

```cpp
// GainStage.cpp:461
out[ch][n] = ch == 0 ? sL : sR;
```

with `numWrite = jmin (numOut, 8) = 8`. **Buffer channel 0 receives the left input at unity and
buffer channels 1 through 7 all receive the right input at unity.** In a hall that is seven speakers
playing the same signal, full level, immediately on commit — not a retained-map fallback, and not
silence.

**Consequences for Q7 and D13:**

- **The label-swap transient is not benign.** Committing `speaker 1 = R` while speaker 2 is still
  `R` produces a duplicate, `isPermutationOf0to7` rejects (`ChannelMap.cpp:67-68`), `mapInvalid`
  goes true, and the rig collapses to the fold above until the second commit repairs it. Under D8's
  commit-on-blur that window is *however long the user takes to type the second label*.
- **D13's banner is necessary but not sufficient.** It makes the state visible; it does not stop the
  sound.

**Recommended answer to Q7 — validate the label set at the commit surface, before
`applyVenueEdit()`.** D8 already sends all 42 values in one call, so the check is a single pass over
the eight labels inside the `setVenue` native function: if they are not a permutation of the
negotiated set, **reject the commit, return the reason, and do not call `applyVenueEdit()` at all**.
This:

- preserves the property that **3.2 adds no second venue-apply path** — it adds a *guard in front of*
  the single existing one;
- leaves `buildSpeakerToBuffer`'s rejection exactly where it is, as the backstop it already is
  (a session restored from a foreign venue must still be caught);
- removes the audible transient entirely rather than annotating it.

**And it forces one UI rule that must be stated at plan, because it contradicts D12 for one column:**
a **label** field that would create a duplicate must be **held and marked, not reverted**. D12's
"reject means revert" is right for a numeric field — reverting `abc` to the last good metre value
loses nothing. Reverting a label makes L↔R **impossible**, because every route from `(L,R)` to
`(R,L)` passes through a duplicate. So: numeric fields revert on invalid; the label column holds the
pending edit, marks both colliding rows, and **blocks the commit until the set is a permutation
again**. That is one extra state, and it is the difference between a swap being awkward and a swap
being unreachable.

### N6 — adopt the preset manager's C++, write O-Octagon's own JS (Q10)

The module is header-only C++ (663 lines) plus a 447-line JS side. **The FUNC-05 guarantee lives
entirely in the header** — `applyPresetJson` is parameter-scoped and cannot reach `apvts.state`'s
children. The JS side is a convenience, and it carries two costs O-Octagon should not pay:

1. **Ten native functions, wired unconditionally in the constructor**
   (`preset-manager.js:107-119`): `savePreset`, `savePresetWithDialog`, `loadPreset`,
   `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`,
   `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`. Any one of those that C++ does not
   register becomes an N4-class never-settling promise the moment it is called.
2. **`createPresetBar()` writes `container.innerHTML = …`** and then queries its own injected markup
   (`preset-manager.js:411-434`). That is
   `pattern_js_state_updater_overwrites_html_labels` by construction, and it is incompatible with
   D3's authored chrome. The class *can* be used without it — the documented form takes
   caller-supplied elements (`preset-manager.js:43-48`) — but at that point O-Octagon is supplying
   the DOM, the styling and the layout, and the class is contributing a ten-function bridge surface
   and a navigation model 3.2 does not need.

**Recommendation:** `#include` the header, construct an `OuariconPresetManager`, and expose the
**four** functions the Venue rail actually needs — save, load, list, current. Do not vendor
`preset-manager.js`.

**On §16's four-way closure:** adopting the header disturbs nothing. The closure is
spec == `createParameterLayout()` == `params::id()` == DOM `ctl-<id>`, and none of the preset
functions is a parameter or uses a `ctl-` id. `parameter-spec.md` stays pinned and 3.2 adds no
parameter (constraint 8).

### N7 — `mapInvalid` should carry a reason, and it needs no new atomic (Q9)

`buildSpeakerToBuffer` already distinguishes three cases and discards the distinction:

| Line | Condition | What it means to an operator |
|---|---|---|
| `ChannelMap.cpp:49-50` | `outSet.size() != kNumSpeakers` | Not an 8-channel bus — this is the SAFE case, not a label error |
| `ChannelMap.cpp:61-62` | `getChannelIndexForType (labels[n]) < 0` | **Speaker `n`'s label is not a member of the negotiated set** — the row is known |
| `ChannelMap.cpp:67-68` | `! isPermutationOf0to7 (next)` | **Two speakers resolved to the same output index** — a duplicate |

The same file already carries the idiom for surfacing this: `verifyEnumBitOrder (set, juce::String* whyNot)`
(`ChannelMap.cpp:78-86`). Extending `buildSpeakerToBuffer` with an optional out-param in that shape
is no new logic and one in-file precedent.

**And it does not need an atomic.** This is P43's rule reused rather than re-argued: a cross-thread
`juce::String` is a race, which is why `outputSetName` is resolved inside `getStatus` on the message
thread rather than stored. The reason code has the same property — `rebuildChannelMap()` runs on the
message thread and `getStatus` runs on the message thread, so a plain member suffices. **`mapInvalid`
stays exactly the `std::atomic<bool>` it is** (`PluginProcessor.cpp:293`), because that one *is* read
by the audio thread via `mappedOutputAvailable()`.

**Recommendation: yes to the reason code**, carried as `{ enum reason, int speakerIndex }` and
surfaced through `getStatus` alongside the boolean. In a hall, *which row* is the actionable half —
and after N8, the banner is the only thing telling an operator why seven speakers just went mono.

---

## 2. The remaining questions

### Q2 — is `customState` the right home for `SCENES` at 3.3?

**Yes, and D10's "none at 3.2" makes it safe.** `setCustomStateCallbacks` is the only path by which
a preset reaches non-parameter state (`OuariconPresetManager.h:100-105`, written at `:287-289`, read
at `:346-349`). Because **O-Octagon does not route session state through the module** (N2), the
`setStateFromXml` half of the callback (`:591-603`) is never invoked — so at 3.3 the callback is
reachable **only** from a preset load, which is exactly the scope D6 wants for scenes.

The 3.3 obligation is therefore a single bounded one, and FUNC-06/5's re-measurement has a defined
target: **`customLoad` must write to the `SCENES` node and to nothing else.** A greppable assertion
of the same kind that discharges FUNC-05 at 3.2 — the difference being that at 3.2 the assertion is
"no callback exists at all", and at 3.3 it becomes "exactly one callback exists and its body touches
only `SCENES`".

### Q3 — ping indicator transport

**Recommendation: a second, faster poll that runs only while pinging. Do not add a push path at 3.2.**

The sampling argument is the decisive one and it is arithmetic, not taste. §OQ2 fixes the auto-cycle
at **1.2 s on / 0.4 s gap**, so the shortest state the indicator must resolve is 400 ms. The existing
poll is `STATUS_POLL_MS = 500` (`app.js:120`) — **longer than the gap**, so it can miss a gap
entirely and can lag a speaker change by up to half a period. A ping-only poll at **100 ms** resolves
the 400 ms gap with margin and costs nothing when the ping is off, which is almost always.

**Why not push, given the machinery exists.** `emitEventIfBrowserIsVisible` is public and the JUCE JS
side exposes `addEventListener` (`js/juce/index.js:42`, and the internal relays use it at `:160`,
`:321`, `:409`). Three costs, and the third is disqualifying at this boundary:

1. It would be **the first non-pull surface in this UI**, and 3.1 chose pull deliberately.
2. **The ui-stub does not model `backend.addEventListener`** — `tests/ui-stub/juce-stub.js:33`
   records the absence as a design property. D4 made the stub mandatory, so a push path must be
   modelled there before it can be rendered stub-first, and UI-02 criterion 6's ordering discipline
   applies to every new page module (constraint on the stub in CONTEXT §Confirmed available).
3. **N4 applies to `emitEvent` even harder than to completions** — it *is* the same
   `emitEventIfBrowserIsVisible` call. A pushed indicator update during a hidden interval is dropped
   with no retry, whereas a poll self-heals on its next tick. **For an indicator whose entire job is
   to be trustworthy during a speaker-identification procedure, a transport that can silently skip a
   step is the wrong transport** — that is D14's own argument applied to the mechanism rather than to
   the derivation.

The C++ side is unchanged either way: D14 stands, the authority is a value read from the audio-side
ping state, and JS never re-derives the step.

### Q5 — where the ping injects, and whether it needs a `reset()`

**§7.2 already fixes the position and it is unambiguous:** *"Verify-ping override **after** the
write, **before** metering — so the meters confirm the ping reached the intended channel."*
Combined with §OQ2's *"injected at the channel map, all other channels hard-zeroed"*, the ping is a
**post-write overwrite** of the eight mapped output pointers, not a term folded into the gain path.

**No `reset()` is needed, and none should be added.** This preserves P23/P30's *one reset site,
ever*:

- The smoothers are untouched by the ping. `GainStage`'s per-sample loop advances all 17
  unconditionally (§3.6.4, and `GainStage.cpp:451-460` shows the SAFE arm advancing them precisely
  so a mode flip cannot resume from a stale `currentValue`). A post-write overwrite leaves that
  invariant alone: on ping stop, the DBAP signal resumes from smoothers that never froze, so there
  is nothing to teleport.
- The discontinuity at start and stop is already owned by §OQ2's **20 ms raised-cosine envelope**.
  This is 2.3's H1 argument in a different place — an envelope that reaches zero makes a state reset
  unnecessary, and a reset would be the thing that *introduces* a click by discarding a partly-faded
  tail.

**One thing §OQ2 does not say, and plan must decide: the ping has no meaning in SAFE mode.** When
`mapped` is false there is no map to test, and after N8 the SAFE arm is writing dry input across the
bus. Pinging "speaker 5" on a stereo fold would name a speaker that does not exist during the one
procedure whose purpose is confirming speaker N is speaker N — R1 reproduced inside its own
diagnostic tool, which is D14's stated failure mode arriving by a different route.
**Recommendation: the ping refuses to start when `mappedOutputAvailable()` is false, and the UI says
why.** That is a fifth stop condition in spirit, though not in D11's sense — D11's four stops are
about a *running* ping; this is a start precondition.

### Q6 — `.venue` schema-version mismatch policy

**Current behaviour, verified:** `@schemaVersion` is written (`VenueModel.cpp:253`) and **read but
never branched on** — the code says so explicitly at `VenueModel.cpp:212-214`. Every attribute is
read through a fallback that keeps the *existing* value when the attribute is absent
(`:208-210` and the per-speaker loop at `:216-226`).

For **session** state that is the right design: the tree came from this plugin. For a **file**,
it is not, and the failure mode is the one §R8 exists to prevent. A `.venue` written by a future
build that renames or adds an attribute loads as **a mixture of the file and whatever the model
already held** — a room that is partly the measured room and partly the placeholder, with no
indication which parts are which. That is precisely CONTEXT's *"a venue that half-applied"*, and it
is unrecoverable in a hall because nothing on screen distinguishes it from a correct load.

**Recommendation for plan — three rules, all cheap:**

1. **Load into a fresh `VenueModel`, never into the live one.** Then a missing attribute falls back
   to a *known* default rather than to a neighbouring venue's value, and the failure is at worst
   legible.
2. **`schemaVersion > kSchemaVersion` → load, but surface it.** Refusing a forward file is worse
   than loading it — the operator has the numbers in front of them either way — but it must be
   stated, in the same banner class D13 establishes. Silent best-effort is the one option to rule
   out.
3. **A file whose root is not `VENUE`, or which yields fewer than 8 speaker children, is rejected
   outright** and the live venue is not touched. FUNC-02/2's *"reproduces all 42 values exactly"* is
   a claim about a file this build wrote; it says nothing about a foreign one, so plan owns this.

### Q8 — does the mini-plan break §19's "exactly one `metresToPx`"?

**No, and no weakening is required — the function is already parameterised for it.**

```js
// roomplan.js:62-67
export function metresToPx(mx, my, view) {
  return {
    x: ((mx - view.minX) / view.spanX) * view.w,
    y: ((my - view.minY) / view.spanY) * view.h,
  };
}
```

The box size arrives in `view` (`{ minX, minY, spanX, spanY, w, h }`, built at `roomplan.js:157`).
A mini-plan is **a second `view`, not a second projection** — same function, smaller `w`/`h`. §19's
three assertions all continue to hold as written, and P46 is honoured rather than argued around.

**But §19 has an enumeration hole, and 3.2 is what opens it.** Its second assertion is

```js
check(!/metresToPx\s*\(/.test(S.appJs.replace(/\/\/[^\n]*/g, '')),
    'app.js does no projection of its own — it delegates to roomplan.js');
```

— **`S.appJs` by name.** A new `venue.js` computing its own `(v - min) / span` is invisible to it.
The third assertion has the same shape: it scans `S.roomJs` only. **The fix is to widen the
enumeration to every page module, which strengthens the gate; the fix is never to relax the
assertion.**

### Q11 — does the 42-field table fit at 1100 × 720? **Measured.**

Rendered in Chromium at 1100 × 720, DPR 1, against **the real `styles.css`** — its tokens, its type
scale (11 px mono `tabular-nums` fields, 9 px column heads), its `--header-h: 56px` /
`--footer-h: 40px` frame contract, and `.screen { padding: 16px }`. Row arithmetic is not the gate
here for the same reason it is not the gate in `ui_layout_check.js`
(`pattern_flex1_container_slack_invisible_to_row_sum`).

| Measurement | Value |
|---|---|
| Screen area inside the frame | **1100 × 624** (720 − 56 − 40) |
| Content area after `.screen` padding | **1068 × 592** |
| **The 8-row table** (`#`, label, x, y, z, trim, class) | **752 × 277 px** — 8 rows at **32.5 px**, header included |
| Table's share of available height | **47 %** |
| Field height / font | 24 px / **11 px mono**, `font-variant-numeric: tabular-nums` confirmed computed |
| Rail | 300 px wide |
| `document.scrollWidth` / `scrollHeight` | **1100 / 720** — no page overflow |

**The table is comfortably inside budget. 3.1's Q7 is answered and D7 survives — the 42 values were
never the problem.**

**The rail, however, overflowed — by 9 px — and the mini-plan was the cause.** Width-bound in a
300 px rail, the portrait envelope aspect **0.800** demands `278 / 0.8 = 348 px` of height, and the
rail stack (set name · mini-plan · save/load · ping card) totalled **601 px** against 592 available:
`railScrollHeight 601 > railClientHeight 592`.

**Height-binding the mini-plan fixes it, and it is the same rule the Room plan already follows.**
Re-measured with the plan fitted to the rail's residual height instead of its width:

| | Width-bound (overflows) | **Height-bound (fits)** |
|---|---|---|
| Mini-plan box | 278 × 348 | **270 × 337** |
| Rendered aspect | 0.7989 | **0.8012** |
| `railScrollHeight` vs `railClientHeight` | **601 > 592** ✗ | **592 == 592** ✓ |

**This is D7's portrait consequence appearing a second time, one layer down.** The Room plan is
height-bound at 448 × 560 for exactly this reason; the mini-plan is height-bound at 270 × 337 for
exactly the same reason. Plan should state the rule once — **a plan box is fitted to the smaller of
its two bounds, and the aspect always comes from the returned envelope** — and gate it, because a
width-bound mini-plan is a nine-pixel overflow that no `document.scrollHeight` assertion catches:
**the page did not overflow, only the rail did.** `ui_layout_check.js` §8 measures the document, so
as written it would have passed.

Two incidental observations from the render, recorded so plan does not rediscover them:

- **The venue-name field truncates at 220 px** — `Default (placeholder — NOT meas`. The left column
  has ~300 px of unused width on that row; the field simply needs to be wider.
- **~240 px of vertical slack sits under the table in the left column.** Moving the mini-plan there
  does *not* help: height-bound in that space it would be ~240 px wide, **narrower than the rail's
  270**. **The rail is the better home, and the measurement confirms D9 rather than overturning it.**

---

## 3. Native-function inventory for Phase 3.2

3.1's surface was **exactly three**, grep-diffed both directions at verify. Below is the proposed
3.2 surface. **The count literal in `ui_frontend_check.js:178` (`registered.size === 3`) must move,
and it will fail loudly until it does — which is correct.**

| # | Function | Direction | Notes |
|---|---|---|---|
| 1–3 | `getParameterDefaults` · `getVenueGeometry` · `getStatus` | read | Unchanged from 3.1. **`getVenueGeometry` must gain the 8 trims and the 2 rake heights** — the only venue values not yet on the wire (CONTEXT §Confirmed available). One call, still: P38's torn-read argument is unchanged |
| 4 | `setVenue` | **write** | D8 — all 42 values, one call, atomic. **Validates the label set before `applyVenueEdit()`** (N8) and returns a result the page can render |
| 5 | `saveVenue` | write | `FileChooser::launchAsync`; completion calls the `juce::File`-taking function UI-01/3's probe (a) also calls |
| 6 | `loadVenue` | write | Same; parse → fresh `VenueModel` → `applyVenueEdit()`. **No second venue-apply path** |
| 7–10 | `savePreset` · `loadPreset` · `getPresetList` · `getCurrentPreset` | write/read | The four the rail needs. **Not the module's ten** (N6). The load site brackets all 17 gestures (N5) |
| 11 | `startPing` | write | Speaker index, or auto-cycle. **Refuses when `mappedOutputAvailable()` is false** (Q5) |
| 12 | `stopPing` | write | D11's explicit Stop |
| 13 | `getPingState` | read | D14's authority. Polled at **100 ms while pinging only** (Q3) |

**Thirteen.** Every one must appear in three places or §3 fails: the C++ registration, the JS call
site **in a file §3 enumerates**, and the ui-stub whitelist.

---

## 4. The two gate-enumeration holes — the sharpest item for plan

Both new gate files were written at 3.1 against a two-module page (`app.js`, `roomplan.js`). 3.2 adds
at least one more module, and **both gates address their inputs by name**:

| Gate | Line | Enumeration | What 3.2 breaks |
|---|---|---|---|
| `ui_frontend_check.js` §3 | `:165, :167` | `for (const src of [S.appJs, S.roomJs])` | Native-function calls in `venue.js` are **not counted**, so a called-but-unregistered name passes the both-directions diff |
| `ui_frontend_check.js` §19 | `:649` | `S.appJs` | A second projection in `venue.js` is **not detected**, so P46's one-projection rule is asserted against a file that no longer contains the risk |

**Neither gate fails when this happens. Both pass, by not looking.** That is the vacuity class this
project has now caught six times, and it is the reason `RESEARCH` says it here rather than leaving it
to verify:

> **Every new page module must be added to `S.*` and to both enumerations in the same task that
> creates it**, and the negative control for that task is *put a `metresToPx` and an unregistered
> `nativeFn` in the new file and watch both gates fire.*

The stub whitelist has the same property but is safe by construction: §3 asserts
`stubbed == registered` as a set, so a missing stub entry fails rather than passes.

---

## 5. Risks and pitfalls new at this boundary

| # | Risk | Mitigation |
|---|---|---|
| 1 | **A dropped completion leaves the UI stale forever** (N4) | No UI state depends solely on a promise resolving; converge on the `venueGen` poll |
| 2 | **A label-swap collapses 7 speakers to mono at unity** (N8) | Validate the label set in `setVenue` before `applyVenueEdit()`; label fields hold-and-mark rather than revert |
| 3 | **A preset load sweeps 17 lanes in an armed host** (N5) | Bracket the load with 17 gestures at O-Octagon's call site; do not edit the shared module |
| 4 | **A new page module silently reduces two gates' coverage** (§4) | Widen both enumerations in the task that adds the file; negative-control it |
| 5 | **A width-bound mini-plan overflows the rail invisibly** (Q11) | Height-bind; assert `railScrollHeight <= railClientHeight`, not just document scroll |
| 6 | **A forward-version `.venue` half-applies** (Q6) | Fresh model, surface the version, reject a malformed root |
| 7 | **MSVC**: 3.2 writes the chooser completions, which are nested lambdas capturing `this` | `critical_msvc_safepointer_init_capture_nested_lambda` — hoist the `SafePointer` to a local. `critical_msvc_constexpr_lambda_capture` — no non-static `constexpr` in a lambda (constraint 10) |
| 8 | **Non-ASCII in new C++ strings** — the reason codes and the version warning are new user-facing text | `juce::String(const char*)` is ASCII-only; §20 already gates `PluginEditor.cpp` and every new string inherits it (constraint 6, and D-2 was a real defect at 3.1) |

---

## 6. Contract dispositions

**No contract is amended and no checksum moves.** Every 3.2 question resolved against a contract
resolved *inside* it:

| Question | Contract clause consulted | Disposition |
|---|---|---|
| Q5 ping placement | §7.2 (*after the write, before metering*) + §OQ2 (*at the channel map, others hard-zeroed*) | **Already specified.** No gap |
| Q3 transport | §OQ2's 1.2 s / 0.4 s | **Already specified.** The transport is an implementation choice below the contract |
| Q6 schema policy | §4.1 fixes the `.venue` format; it does not fix a *foreign-version* policy | **An absence, not an error** — the same class as 3.1's two gaps, and it is resolved by a plan gate, not a re-pin. Recorded here so 3.3 does not read it as settled contract |
| Q2 `SCENES` home | §4.1's tree, §6.3 | Unchanged; the three §8 re-pins stay scheduled for **3.3 discuss** |

**One thing 3.3 inherits, stated now:** N8 means FUNC-06's scene work must not assume that an invalid
map merely "retains" — the SAFE fold is what plays. Any 3.3 probe that asserts retention must assert
it against the **snapshot**, not against the output buffer.

---

## 7. Handed to plan

1. **`setVenue` validates the label set before `applyVenueEdit()`** (N8) — and the label column
   holds-and-marks where D12's numeric columns revert.
2. **Bracket the preset load with 17 gestures** at O-Octagon's call site (N5). Do not edit
   `OuariconPresetManager.h`.
3. **Adopt the preset manager's C++ header only**; four native functions, not ten; no
   `createPresetBar` (N6).
4. **Widen §3's and §19's file enumerations in the same task that adds a page module**, with a
   negative control that fires (§4).
5. **The mini-plan is height-bound**, and the rail gets its own overflow assertion —
   `railScrollHeight <= railClientHeight`. The document-level check would not have caught this
   (Q11).
6. **Ping indicator: a 100 ms poll while pinging only.** No push path at 3.2; no `reset()`; refuse to
   start when `mappedOutputAvailable()` is false (Q3, Q5).
7. **`mapInvalid` carries a reason and a speaker index**, surfaced through `getStatus`, as a plain
   member — no new atomic (N7, P43).
8. **`.venue` load: fresh model, surface a forward version, reject a malformed root** (Q6).
9. **No UI state depends solely on a native-function promise resolving** (N4).
10. **`getVenueGeometry` gains the 8 trims and 2 rake heights** — still one call.
11. **The §3 count literal moves from 3 to 13** and is expected to fail until updated.

**Predicted outcome unchanged from discuss: FUNC-02 ✅ · FUNC-04 ✅ · FUNC-05 ✅ · UI-01 ✅ — all four
`must`, zero partials declared in advance.** Nothing in this research contradicts that. The seven
ROADMAP orphans and UI-02/5's inherited end-to-end gate remain 3.2's to close.

---

## 8. References

**JUCE 8.0.14, in-tree** — `juce_WebBrowserComponent.cpp:306-312, 336-344, 587, 607-611`;
`juce_WebBrowserComponent.h:328-333, 592`; `juce_AudioProcessorParameter.cpp:59-63, 65-109, 111-125`;
`juce_ParameterAttachments.cpp:78-95`; `juce_audio_plugin_client_AU_1.mm:1341-1360`;
`juce_audio_plugin_client_VST3.cpp:1498-1501, 1652-1663`; `js/juce/index.js:37-91, 570-577`.

**Module** — `OuariconPresetManager.h:100-105, 287-289, 299-352, 346-349, 580-607`;
`preset-manager.js:43-48, 107-119, 411-434`.

**O-Octagon** — `PluginEditor.cpp:143-372`; `PluginProcessor.cpp:264-268, 271-297, 394-406, 430`;
`GainStage.cpp:290-321, 408-465`; `ChannelMap.cpp:45-86`; `VenueModel.cpp:208-226, 249-267`;
`VenueModel.h:51-52, 104-120`; `app.js:120, 334-394`; `roomplan.js:62-67, 157`;
`ui_frontend_check.js:158-190, 642-667`; `ui-stub/juce-stub.js:33, 267-283`;
`ARCHITECTURE.md` §7.2, §7.4, §OQ2.

**Knowledge base** — `pattern_webview_launchasync_safepointer_no_complete` ·
`pattern_webview_native_fn_bridge_gap` · `pattern_js_state_updater_overwrites_html_labels` ·
`pattern_flex1_container_slack_invisible_to_row_sum` · `pattern_webview_knob_readout_scaled_value` ·
`critical_msvc_safepointer_init_capture_nested_lambda` · `critical_msvc_constexpr_lambda_capture` ·
`critical_juce_string_char_ctor_is_ascii_only` · `pattern_promotion_checksum_pins_replaced_file`.

**Measurement** — Chromium via Playwright at 1100 × 720, DPR 1, against the real `styles.css`.
Harness retained as **`tests/tools/venue_layout_study.js`** (alongside `air_filter_study.py` and
`gen_dbap_reference.py`, the same class of study script), so the Q11 numbers are **re-runnable at
plan and at verify** rather than a claim that expires with this document. It is a study script, not
a gate: it is not wired into any test target, and the rail-overflow assertion it motivates belongs in
`ui_layout_check.js` (§7 item 5).

---

## Next Phase

Ready for: **plan** — `/plugin-plan O-Octagon 3-gui`

Eleven questions answered, **five from source that discuss could not have reached**. The two that
change a decision are **N8** (a label-swap transient is audible, so `setVenue` must validate before
applying) and **Q11's measurement** (the table was never the constraint; the mini-plan must be
height-bound). The one to carry loudest into plan is **§4** — two gates enumerate their inputs by
name, and a new page module reduces their coverage without failing either.
