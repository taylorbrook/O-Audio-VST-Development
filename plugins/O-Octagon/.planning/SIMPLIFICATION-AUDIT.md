---
plugin: O-Octagon
version_audited: 1.3.0
audited: 2026-08-25
scope: code simplification (propose-only)
files_audited: Source/*.{h,cpp}, Source/DSP/, Source/Data/, Source/ui/public/ (js/juce/ vendored, excluded)
candidates:
  high: 1
  medium: 7
  low: 8
  total: 16
phase_routing:
  phase1_improve: [HIGH-01]   # APPLIED in v1.3.3 (2026-08-26)
  phase3_applied:  [LOW-01, LOW-02, LOW-03, LOW-04, LOW-05, LOW-07]   # v1.3.4 (2026-08-26)
  phase3_false_positive: [LOW-06]        # gate-coupled; see Phase 3 notes
  phase3_skipped:  [MEDIUM-01, MEDIUM-02, MEDIUM-03, MEDIUM-04, MEDIUM-05]
  phase3_deferred: [MEDIUM-06, MEDIUM-07, LOW-08]
  phase2: []
  phase3_original: [MEDIUM-01, MEDIUM-02, MEDIUM-03, MEDIUM-04, MEDIUM-05, MEDIUM-06, MEDIUM-07, LOW-01, LOW-02, LOW-03, LOW-04, LOW-05, LOW-06, LOW-07, LOW-08]   # superseded by the four keys above
status: phase3_partial
---

# O-Octagon v1.3.0 — Simplification Audit

**Audited:** 2026-08-25 · **Method:** two parallel auditors (C++ side, WebView UI side) plus a feasibility
pass over the HIGH-severity set that re-read the code and checked for hidden dependencies (test-gate greps,
render-harness references, JS `classList` uses of "dead" CSS).

Every candidate below is **behaviour-preserving by construction**. Nothing here changes DSP arithmetic or
float evaluation order — the plugin has byte-identical render goldens and no candidate was allowed to put
them at risk. `Severity` is payoff; `Risk` is the chance of behaviour change.

## Routing

| Phase | Command | Candidates |
|---|---|---|
| Phase 1 | `/improve O-Octagon` (apply the LOW-risk HIGH item) | HIGH-01 — ✅ **APPLIED in v1.3.3** |
| Phase 2 | `/simplify-phase2 O-Octagon` | — (none) |
| Phase 3 | `/simplify-phase3 O-Octagon` | ✅ **6 applied in v1.3.4** (LOW-01…05, LOW-07) · ❌ LOW-06 false positive · ⏭ 5 skipped (MEDIUM-01…05) · ⏸ 3 deferred (MEDIUM-06, MEDIUM-07, LOW-08) |

## The one thing that isn't cosmetic

**MEDIUM-04** is not really a simplification — it is a live drift bug found while auditing. `getFieldGrid`'s
`readParam` fallback for `blur` still reads `0.1f`, the pre-v1.3.0 default; the live default moved to `0.03f`
when `kBlurScale` tripled. It sits on an unreachable-in-practice path (fires only if the host writes NaN), so
it is filed here rather than in the code review, but deriving the fallback from the live parameter removes the
whole drift class. This is the repo's own `pattern_test_fixture_mirrors_drift_silently` firing in production
code.

---

## HIGH severity

Biggest maintenance payoff.

### [HIGH-01] drawMini() re-implements roomplan's hull-points and glyph-classification rendering verbatim

> ✅ **APPLIED — O-Octagon v1.3.3 (2026-08-26).** Extracted as `hullPoints(hull, view)` and
> `placeGlyph(g, s, view)` in `roomplan.js`; both `drawGeometryLayer()` and `drawMini()` call them.
> The transform was folded into `placeGlyph` alongside the toggles rather than left duplicated.
> The v1.1.0 output badge stayed in `roomplan.js` as the feasibility check required, and the
> optional `fitToStage` prologue extraction was NOT taken — out of the applied scope.
> `venue.js` no longer imports `metresToPx` at all. Rendered DOM byte-identical (sha256
> `fd73cf31…649b8b` before and after, four venue states); a 297-comparison old-vs-new probe
> covering `INTERIOR` and the on→off toggle direction is identical and negative-controls to 48
> mismatches; both JS gates green (42 / 31 sections). See CHANGELOG v1.3.3.

**File:** `plugins/O-Octagon/Source/ui/public/js/venue.js:270` · **Type:** duplication · **Risk:** LOW · **Side:** ui

**Current:**
```
// venue.js drawMini(), lines 270-290 — identical to roomplan.js drawGeometryLayer() 330-349:
miniHull.setAttribute("points", geometry.hull.map((p)=>{ const q=metresToPx(p.x,p.y,view); return `${q.x},${q.y}`; }).join(" "));
geometry.speakers.forEach((s,i)=>{ ... g.setAttribute("transform", `translate(${p.x} ${p.y})`);
  g.classList.toggle("is-vertex", s.class==="VERTEX");
  g.classList.toggle("is-onedge", s.class==="ON_EDGE");
  g.classList.toggle("is-interior", s.class==="INTERIOR"); });
// plus the identical measure→span-guard→fitBox→viewBox prologue (venue.js 253-266 vs roomplan.js 238-254)
```

**Proposed:**
```
// export beside metresToPx/fitBox/makeView in roomplan.js:
export function hullPoints(hull, view) {
  return hull.map((p)=>{ const q=metresToPx(p.x,p.y,view); return `${q.x},${q.y}`; }).join(" ");
}
export function applyClassification(g, cls) {
  g.classList.toggle("is-vertex", cls==="VERTEX");
  g.classList.toggle("is-onedge", cls==="ON_EDGE");
  g.classList.toggle("is-interior", cls==="INTERIOR");
}
// both drawGeometryLayer() and drawMini() call them; optionally also a shared fitToStage(rectW,rectH,envelope) for the duplicated prologue
```

**Rationale:** The file's own charter is 'a second view, never a second projection' (Q8) — but only metresToPx/fitBox/makeView were shared; the points-string construction and the three class toggles were copy-pasted. A future fourth classification class or points format change must now be made twice. Extraction extends the existing sharing pattern, still routes every coordinate through the one metresToPx (section 19 of the static gate stays true — it is widened, not weakened).

**Test impact:** No render-golden exposure. DOM output is byte-identical (same points string, same class toggles). ui_frontend_check §19 asserts no second (v-min)/span form — extraction preserves that. Round-trip the stub page once to confirm both plans still draw.

**Feasibility check:** Confirmed: hull points-string (venue.js:270-278 vs roomplan.js:330-338) and glyph transform + three class toggles (venue.js:280-290 vs roomplan.js:340-349) are verbatim copies. Risk LOW is honest — pure code motion, all coordinates still route through the one metresToPx, no float reassociation. Two caveats for the implementer: (1) roomplan's glyph loop additionally renders the v1.1.0 output badge (outBadges, roomplan.js:351-362) which the mini has no DOM nodes for, so the extraction must scope to hull+transform+toggles and leave badge rendering in roomplan (the candidate's own scoping already says this); (2) the shared helper must live in roomplan.js — ui_frontend_check.js section 32 bans the VERTEX/ON_EDGE/INTERIOR vocabulary in js/scenes.js and section 19 requires the single-projection module; extraction into roomplan.js passes sections 19, 22, 26, and 32, and layout gate sections 3/8 measure rendered DOM so they verify rather than block the change.

## Phase 3 Applied (v1.3.4, 2026-08-26)

Batch A of the tiered approval — the LOW-severity, LOW-risk tier. Six of seven applied.

| ID | Outcome |
|---|---|
| LOW-01 | ✅ Applied as `OOctagonProcessor::commitScenes()`. **Scoped to the three paired sites.** The audit listed three; there is a **fourth** `sceneStore.writeToState` at `PluginProcessor.cpp:178` (constructor) with no generation bump, deliberately — it seeds the `SCENES` node at birth (N13), has no cache to invalidate, and `scenesGeneration` starts at 1. Folding it in would have been a behaviour change. |
| LOW-02 | ✅ Applied. Measured, not argued: computed `text-transform` on all eight `#vf-label-N` inputs is `none` before and after. |
| LOW-03 | ✅ Applied as the `{ state }` variant. Verified first that all four consumer files read only `.state`. |
| LOW-04 | ✅ Applied. `WEIGHT_IDS` is declared at `app.js:122`, `FIELD_INPUT_IDS` at `:181` — no TDZ hazard. |
| LOW-05 | ✅ Applied. `clamp01`'s shipped declaration asserted character-identical to the deleted inline expression. |
| LOW-06 | ❌ **REVERTED — FALSE POSITIVE.** See below. |
| LOW-07 | ✅ Applied at all three sites (`venue.js` preset list, `elevation.js` axis + speaker groups). |

### LOW-06 is a false positive, not pending work

The candidate reads *"The aliases add a line per site and no meaning"* / *"Test impact: None —
purely mechanical."* Both are false. The seven `const value = pingStateNode;` aliases are what makes
`venue.js` satisfy **`ui_frontend_check.js` section 6**, which guards
`pattern_js_state_updater_overwrites_html_labels` by whitelisting the *identifier names* that appear
on the left of a `.textContent =` write, each paired with a companion assertion about its binding.
Deleting the aliases moved the receivers to `classNode`, `venueNameNode`, `presetCurrentNode`,
`pingStateNode` and `ooStateNode` and **failed section 6**.

Passing again would mean loosening a deliberately short whitelist, or writing five new companion
assertions, for seven lines of nit-tier cosmetics. **Do not re-attempt without also deciding what
section 6's contract should be.** This is the audit's own blind spot: it screened MEDIUM-06 and
MEDIUM-07 for gate coupling and cleared every LOW item without doing so.

### Verification

Rendered-DOM + computed-style snapshot across five page states, sha256 `69227ed4…d0ae47` before and
after, 0 console errors — shown deterministic across two baseline runs first, and negative-controlled
(80 computed-style lines change, `#elev-axis` children accumulate). LOW-05's clamp separately proven
over 126 pathological inputs, since the stub's meters are all-zero without a ping.
`ui_frontend_check` 42 · `ui_layout_check` 31 · render 51/0 · geometry 46/0 · `auval` PASS.

**Coverage gap surfaced, not introduced:** removing `++scenesGeneration` from `commitScenes()`
outright leaves all 51 render probes green — nothing observes `getScenesGeneration()`. LOW-01 is
verified by the call-site diff and the compiler, not by a test.

## Phase 3 Skipped (still open)

MEDIUM-01…MEDIUM-05 were skipped by choice at the Batch B gate and remain below **verbatim**,
including **MEDIUM-03**, the live `blur` fallback drift (`0.1f` vs the live `0.03f` default) — the
only item in the whole audit that corrects behaviour.

> **Note — the audit mislabels its own item.** The preamble section *"The one thing that isn't
> cosmetic"* calls the `getFieldGrid` drift bug **MEDIUM-04**. It is **MEDIUM-03**; MEDIUM-04 is the
> CSS banner dedup.

## Phase 3 Deferred

Batch C — every one needs a test gate edited in the same commit, so none is a drive-by.

| ID | What it needs |
|---|---|
| MEDIUM-06 | `ui_frontend_check.js:368,387` extract served paths by regexing `url == "…"`; against a table form both report 0 comparisons and pass vacuously. The regex must become a table reader in the same commit. |
| MEDIUM-07 | §33 statically asserts the guard shape at all three sites and NC5 removes a deadline to prove the latch reproduces. The audit's own verdict: *"only worth doing with the gate maintained alongside; otherwise defer."* |
| LOW-08 | Tagged `Risk: MEDIUM` despite LOW severity. §21 measures the fitted strip and NC1 oversizes it — confirm the gate reads the rendered rect, not the attributes, then re-run both. |

## MEDIUM severity

Structural dedups and consolidations.

### [MEDIUM-01] VerifyPing::State -> DynamicObject serialized twice (startPing, getPingState)

**File:** `plugins/O-Octagon/Source/PluginEditor.cpp:937` · **Type:** duplication · **Risk:** LOW · **Side:** cpp

**Current:**
```
// in startPing (lines 937-944) AND again in getPingState (lines 970-975):
auto* obj = new juce::DynamicObject();
obj->setProperty ("active",      state.active);
obj->setProperty ("mode",        state.mode);
obj->setProperty ("speaker",     state.speaker);
obj->setProperty ("elapsedMs",   state.elapsedMs);
obj->setProperty ("remainingMs", state.remainingMs);
```

**Proposed:**
```
// beside makeResult() in the anonymous namespace:
juce::DynamicObject* pingStateToObject (const oo::VerifyPing::State& s)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("active", s.active);       obj->setProperty ("mode", s.mode);
    obj->setProperty ("speaker", s.speaker);     obj->setProperty ("elapsedMs", s.elapsedMs);
    obj->setProperty ("remainingMs", s.remainingMs);
    return obj;
}
// startPing: auto* obj = pingStateToObject (state); obj->setProperty ("ok", ok); obj->setProperty ("reason", ...);
```

**Rationale:** The five-field ping-state wire shape is written out twice; a field added to State (or renamed) must currently be edited in two lambdas or the two payloads silently diverge — exactly the drift class this codebase documents elsewhere. The helper preserves the exact property names, order, and types, so the JS consumer sees byte-identical JSON.

**Test impact:** None on goldens (editor TU never compiled by the render harness). ui_frontend_check.js section 3 diffs native-function REGISTRATION names only, not lambda bodies. Proof: build + open the ping panel once, or diff the getPingState JSON before/after in the ui-stub.

### [MEDIUM-02] Validate-and-complete tail triplicated across setVenue / assignSpeakerOutput / applyOutputOrderPreset

**File:** `plugins/O-Octagon/Source/PluginEditor.cpp:575` · **Type:** duplication · **Risk:** LOW · **Side:** cpp

**Current:**
```
// identical trailing block in three native functions (lines 575-578, 655-658, 693-696):
ochan::MapDiagnosis whyNot {};
const bool ok = processorRef.applyVenueEditChecked (edited, &whyNot);
complete (makeResult (ok, mapFailureName (whyNot.reason), whyNot.speakerIndex));
```

**Proposed:**
```
// local helper in the constructor (or a private member):
const auto applyEditedAndComplete = [this] (const oo::VenueModel& edited, auto& complete)
{
    ochan::MapDiagnosis whyNot {};
    const bool ok = processorRef.applyVenueEditChecked (edited, &whyNot);
    complete (makeResult (ok, mapFailureName (whyNot.reason), whyNot.speakerIndex));
};
// each fn ends: applyEditedAndComplete (edited, complete);
```

**Rationale:** Three exact copies of the guarded-apply epilogue. A future change to the result shape (or a fourth label-editing surface, which v1.1.0 already added two of) must currently be replicated three times. The helper is same-TU, same call order, same wire shape — mechanically identical.

**Test impact:** ui_frontend_check.js section 22 asserts this file contains no bare `applyVenueEdit (` call site — `applyVenueEditChecked (` does not match that pattern, and the helper keeps calling the checked form, so the gate stays green. Goldens untouched (editor excluded from harness). Verify with ui_frontend_check.js plus one manual venue-table commit in Standalone.

### [MEDIUM-03] getFieldGrid readParam fallbacks are transcribed defaults — and blur's has ALREADY drifted (0.1f vs live 0.03f)

**File:** `plugins/O-Octagon/Source/PluginEditor.cpp:1176` · **Type:** verbose-pattern · **Risk:** LOW · **Side:** cpp

**Current:**
```
const float rolloff   = readParam (oo::params::id (oo::params::rolloff), 4.0f);
const float blur      = readParam (oo::params::id (oo::params::blur), 0.1f);   // stale: default moved to 0.03f in v1.3.0
const float hullAtten = readParam (oo::params::id (oo::params::hullAtten), 1.0f);
```

**Proposed:**
```
// derive the fallback from the parameter object, as getParameterDefaults already does:
const auto readParam = [this] (int index)
{
    const auto* id = oo::params::id (index);
    auto* p   = processorRef.getAPVTS().getParameter (id);
    auto* raw = processorRef.getAPVTS().getRawParameterValue (id);
    const float def = p != nullptr ? p->convertFrom0to1 (p->getDefaultValue()) : 0.0f;
    if (raw == nullptr) return def;
    const float v = raw->load (std::memory_order_relaxed);
    return std::isfinite (v) ? v : def;
};
```

**Rationale:** This is the codebase's own top-documented failure (pattern_test_fixture_mirrors_drift_silently) and it has already fired: blur's default moved 0.10 -> 0.03 in v1.3.0 but the transcribed fallback did not. Deriving the fallback from the live parameter removes all three literals and the drift class with them. The fallback fires only when the atomic is null (impossible for valid ids) or non-finite (host wrote NaN), so no reachable behaviour changes.

**Test impact:** Render goldens untouched — this TU is excluded from the harness, and the changed value is on an unreachable-in-practice path. No gate parses these literals. Proof: build + open the field backdrop once; optionally poke a NaN into blur via a debugger to see the field fall back to 0.03 instead of 0.10 (a behaviour CORRECTION on that dead path, worth stating in the commit).

### [MEDIUM-04] SAFE and MAP banner rule blocks are byte-for-byte duplicates

**File:** `plugins/O-Octagon/Source/ui/public/css/styles.css:234` · **Type:** duplication · **Risk:** LOW · **Side:** ui

**Current:**
```
.safe-banner { align-self:center; display:flex; align-items:center; gap:8px; padding:4px 10px; border:1px solid var(--alert); border-radius:2px; background:rgba(194,121,58,0.12); }
/* ...then .map-banner repeats all 8 declarations, .map-tag repeats .safe-tag's 4 declarations, and both [hidden]{display:none} rules repeat */
```

**Proposed:**
```
.safe-banner,
.map-banner { align-self:center; display:flex; align-items:center; gap:8px; padding:4px 10px; border:1px solid var(--alert); border-radius:2px; background:rgba(194,121,58,0.12); }
.safe-banner[hidden], .map-banner[hidden] { display:none; }
.safe-tag, .map-tag { font-family:var(--mono); font-size:10px; letter-spacing:0.16em; color:var(--alert); }
/* keep .safe-copy and .map-copy separate — they genuinely differ (serif vs mono/tabular/nowrap) */
```

**Rationale:** Three rule blocks (~20 declarations) are exact copies; the D13 comment explains the two banners are siblings by design, so grouped selectors state that design instead of copying it. Only .safe-copy vs .map-copy differ and they stay separate.

**Test impact:** None on render goldens (DSP-side, offline harness — CSS never reaches it). Computed styles are identical, so ui_layout_check measurements are unchanged. Verify ui_frontend_check has no per-selector grep on .map-banner's own block before merging.

### [MEDIUM-05] Six parallel per-speaker element arrays built by six copies of the same map

**File:** `plugins/O-Octagon/Source/ui/public/js/roomplan.js:171` · **Type:** consolidation · **Risk:** LOW · **Side:** ui

**Current:**
```
const glyphs      = deps.weightIds.map((_, i) => document.getElementById(`glyph-${i + 1}`));
const wCells      = deps.weightIds.map((_, i) => document.getElementById(`wcell-${i + 1}`));
const meterArcs   = deps.weightIds.map((_, i) => document.getElementById(`meter-${i + 1}`));
const meterPeaks  = deps.weightIds.map((_, i) => document.getElementById(`mpeak-${i + 1}`));
const outBadges   = deps.weightIds.map((_, i) => document.getElementById(`gout-${i + 1}`));
// + outPopButtons at line 194, same shape
```

**Proposed:**
```
const byIndex = (prefix) => deps.weightIds.map((_, i) => document.getElementById(`${prefix}-${i + 1}`));
const glyphs = byIndex("glyph");
const wCells = byIndex("wcell");
const meterArcs = byIndex("meter");
const meterPeaks = byIndex("mpeak");
const outBadges = byIndex("gout");
const outPopButtons = byIndex("out-btn");
```

**Rationale:** Six copies of one lookup expression differing only in the id prefix. A one-line helper removes the repetition without changing the parallel-array consumers (setMeters, drawGeometryLayer, etc. index them independently, so a full struct-of-arrays rework is not required to get the win).

**Test impact:** None — identical elements resolved for identical ids; DOM ids in index.html untouched, so every gate that reads glyph-N/meter-N by id is unaffected.

### [MEDIUM-06] getResource: 12 copy-pasted if-return blocks -> static path/data/mime table

**File:** `plugins/O-Octagon/Source/PluginEditor.cpp:187` · **Type:** consolidation · **Risk:** MEDIUM · **Side:** cpp

**Current:**
```
if (url == "/css/styles.css")
    return makeBinaryResource (UIBinaryData::styles_css, UIBinaryData::styles_cssSize,
                               "text/css; charset=utf-8");

if (url == "/js/app.js")
    return makeBinaryResource (UIBinaryData::app_js, UIBinaryData::app_jsSize,
                               "application/javascript; charset=utf-8");
// ... 10 more identical blocks
```

**Proposed:**
```
struct Entry { const char* path; const char* data; int size; const char* mime; };
static const Entry kResources[] = {
    { "/index.html",     UIBinaryData::index_html, UIBinaryData::index_htmlSize, "text/html; charset=utf-8" },
    { "/css/styles.css", UIBinaryData::styles_css, UIBinaryData::styles_cssSize, "text/css; charset=utf-8" },
    /* ... */ };
for (const auto& e : kResources)
    if (url == e.path || (url == "/" && e == kResources[0]))
        return makeBinaryResource (e.data, e.size, e.mime);
return std::nullopt;
```

**Rationale:** Twelve structurally identical blocks (~70 lines) that grow by several entries per phase; a table makes add/remove a one-line edit and makes the served set greppable in one place. Behaviour is identical: same paths, same data symbols, same mime strings, same fallthrough to nullopt.

**Test impact:** GATE COUPLING, not golden coupling: tests/ui_frontend_check.js sections 8 and 9 extract served paths by regexing `url == "..."` inside OctagonEditor::getResource and will report 0 comparisons against a loop form — the gate's extraction regex must be updated in the same commit (e.g. match the table's `{ "/...",` entries). Render goldens are untouched: the harness compiles this TU never (PluginEditor.cpp is excluded from tests/render-harness by name). Verify with a full ui_frontend_check.js run plus a Standalone visual open.

### [MEDIUM-07] Deadline-released in-flight guard implemented three times (app.js, meters.js, field.js)

**File:** `plugins/O-Octagon/Source/ui/public/js/app.js:465` · **Type:** duplication · **Risk:** MEDIUM · **Side:** ui

**Current:**
```
// app.js refreshGeometry():
if (geometryFetchInFlight && now - geometryFetchSince < GEOMETRY_GUARD_DEADLINE_MS) return;
geometryFetchInFlight = true; geometryFetchSince = now;
... finally { geometryFetchInFlight = false; }
// meters.js tick() lines 147-170 and field.js refresh() lines 163-183 repeat the same
// flag+timestamp+deadline+finally machinery (meters/field also count `dropped`)
```

**Proposed:**
```
// shared, e.g. in a tiny guard.js:
export function makeDeadlineGuard(deadlineMs) {
  let inFlight = false, since = 0, dropped = 0;
  return {
    tryAcquire(now) { if (inFlight) { if (now - since < deadlineMs) return false; ++dropped; } inFlight = true; since = now; return true; },
    release() { inFlight = false; },
    dropped: () => dropped,
  };
}
```

**Rationale:** The P71/N9 guard pattern — boolean + timestamp, released on deadline OR settlement — is the most safety-critical idiom on the page and exists in three hand-rolled copies with slightly different spellings (ticks vs ms, with/without a dropped counter). One audited implementation is easier to keep correct than three; the shipped-3.2 latch bug is exactly what a fourth hand copy risks reintroducing.

**Test impact:** Behaviour-preserving if each site keeps its own deadline constant. BUT ui_frontend_check §33 statically asserts the guard SHAPE at every site and NC5 removes the deadline to prove the latch reproduces — an extraction almost certainly breaks the gate's grep patterns and needs the gate updated in the same change, plus a re-run of NC5. Only worth doing with the gate maintained alongside; otherwise defer.

## LOW severity

Nit tier — mechanical, bulk-approvable.

### [LOW-01] sceneStore.writeToState + ++scenesGeneration commit pair repeated at three sites

**File:** `plugins/O-Octagon/Source/PluginProcessor.cpp:643` · **Type:** duplication · **Risk:** LOW · **Side:** cpp

**Current:**
```
// captureScene (643-645), scenesFromVar (765-766), setStateInformation (871-873):
sceneStore.writeToState (apvts.state);
++scenesGeneration;
```

**Proposed:**
```
// private helper:
void OOctagonProcessor::commitScenes()
{
    sceneStore.writeToState (apvts.state);
    ++scenesGeneration;
}
// call sites: sceneStore.capture (slot, w); commitScenes();
```

**Rationale:** The write-back-and-bump pair is the invariant that keeps the page's scenesGen cache coherent; today it is three hand-kept copies, and a fourth scene-mutating path would have to remember both lines. One helper makes the invariant structural. Identical call order at every site.

**Test impact:** None. Probes CK/CI/CL drive these paths through the public API and observe identical sequences; goldens (audio) never touch scene persistence. A render-harness re-run plus one save/load round-trip suffices.

### [LOW-02] .vfield-label { text-transform: none; } is a no-op

**File:** `plugins/O-Octagon/Source/ui/public/css/styles.css:1014` · **Type:** dead-code · **Risk:** LOW · **Side:** ui

**Current:**
```
.vfield-label { text-transform: none; }
```

**Proposed:**
```
/* delete the rule */
```

**Rationale:** text-transform inherits, but no ancestor of the label inputs (td.vcell → tr → table.vtable → .venue-main → #screen-venue → body) sets any transform — the uppercase rules on this screen target .vcol-head, .group-title and .vbtn only. 'none' is already the computed value, so the rule guards against nothing.

**Test impact:** None — computed style identical. One Venue-screen screenshot diff (or the layout gate run) confirms label casing unchanged.

### [LOW-03] sliders map stores input and value nodes nothing ever reads

**File:** `plugins/O-Octagon/Source/ui/public/js/app.js:311` · **Type:** dead-code · **Risk:** LOW · **Side:** ui

**Current:**
```
sliders.set(id, { state, input, value });
// every consumer — renderMetres, FIELD_INPUT_IDS loop, roomplan.js, elevation.js —
// only ever reads .state; input/value live on in the bindSlider closure anyway
```

**Proposed:**
```
sliders.set(id, { state });
// or sliders.set(id, state) with .state accessors flattened — the { state } shape is the
// smaller diff since 4 files destructure .state
```

**Rationale:** input and value are captured by bindSlider's closures for rendering; the copies stored in the module-level map are dead payload that suggests a wider contract than exists. Dropping them documents that the map's contract is 'paramId -> SliderState'.

**Test impact:** None — the map is module-local and unexported; grep confirms no .input/.value access on map entries anywhere in ui/public/js. The { state } variant keeps all four consumer files unchanged.

### [LOW-04] FIELD_INPUT_IDS re-lists the eight weight ids WEIGHT_IDS already declares

**File:** `plugins/O-Octagon/Source/ui/public/js/app.js:181` · **Type:** verbose-pattern · **Risk:** LOW · **Side:** ui

**Current:**
```
const FIELD_INPUT_IDS = [
  "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8",
  "rolloff", "blur", "hullAtten",
];
```

**Proposed:**
```
const FIELD_INPUT_IDS = [...WEIGHT_IDS, "rolloff", "blur", "hullAtten"];
```

**Rationale:** WEIGHT_IDS is declared 60 lines above; spreading it makes 'the field depends on all eight weights plus three solve params' a stated fact instead of a transcription that can drift if a ninth weight ever appears.

**Test impact:** Identical array contents. Verify ui_frontend_check does not grep the literal list (its §4 check targets the FORMAT table, not this one) before applying.

### [LOW-05] setMeters() inlines the clamp that clamp01 in the same file already provides

**File:** `plugins/O-Octagon/Source/ui/public/js/roomplan.js:382` · **Type:** duplication · **Risk:** LOW · **Side:** ui

**Current:**
```
const level = Math.min(1, Math.max(0, Number(levels?.[i]) || 0));
const peak  = Math.min(1, Math.max(0, Number(peaks?.[i]) || 0));
// clamp01 is defined at line 157 of this same file; meters.js line 93 carries a third copy
```

**Proposed:**
```
const level = clamp01(Number(levels?.[i]) || 0);
const peak  = clamp01(Number(peaks?.[i]) || 0);
```

**Rationale:** clamp01 exists 200 lines above for exactly this expression; the inline copies are the drift the file's own P46 commentary warns about. (meters.js keeping its own local clamp01 is acceptable — importing from roomplan.js would couple the DOM-free ballistics module to the plan module.)

**Test impact:** None — Math.min/Math.max composition is identical, bit-for-bit, no float reassociation.

### [LOW-06] Single-use alias variables before textContent writes (6 sites in venue.js)

**File:** `plugins/O-Octagon/Source/ui/public/js/venue.js:242` · **Type:** verbose-pattern · **Risk:** LOW · **Side:** ui

**Current:**
```
const el = venueNameNode;
el.textContent = String(geometry === null ? "" : geometry.venueName ?? "");
// same shape at lines 233-234 (r.classNode), 435-436, 444-445 (presetCurrentNode),
// 480-481 (pingStateNode), 521-522, 555-556 (ooStateNode)
```

**Proposed:**
```
venueNameNode.textContent = String(geometry === null ? "" : geometry.venueName ?? "");
```

**Rationale:** The aliases add a line per site and no meaning — the target node is already a well-named const captured at construction. Six sites collapse to direct assignments.

**Test impact:** None — identical writes to identical nodes. Purely mechanical.

### [LOW-07] while(firstChild) removeChild loops instead of replaceChildren()

**File:** `plugins/O-Octagon/Source/ui/public/js/venue.js:423` · **Type:** verbose-pattern · **Risk:** LOW · **Side:** ui

**Current:**
```
while (presetSelect.firstChild !== null) presetSelect.removeChild(presetSelect.firstChild);
// same loop in elevation.js drawAxis() line 201 and drawSpeakers() line 246
```

**Proposed:**
```
presetSelect.replaceChildren();
// elevation.js: axisGroup.replaceChildren(); speakerGroup.replaceChildren();
```

**Rationale:** replaceChildren() with no arguments is the idiomatic one-call equivalent, supported in every WebView2/WKWebView this plugin ships into (Chromium 86+/Safari 14+), and works on SVG elements.

**Test impact:** None — same end state (empty child list). One stub-page render to confirm axis/speaker groups still rebuild.

### [LOW-08] svg width/height attributes are dead — the CSS custom properties they duplicate always win

**File:** `plugins/O-Octagon/Source/ui/public/js/elevation.js:175` · **Type:** dead-code · **Risk:** MEDIUM · **Side:** ui

**Current:**
```
svg.style.setProperty("--elev-w", String(box.w));
svg.style.setProperty("--elev-h", String(box.h));
svg.setAttribute("viewBox", `0 0 ${box.w} ${box.h}`);
svg.setAttribute("width", String(box.w));
svg.setAttribute("height", String(box.h));
```

**Proposed:**
```
svg.style.setProperty("--elev-w", String(box.w));
svg.style.setProperty("--elev-h", String(box.h));
svg.setAttribute("viewBox", `0 0 ${box.w} ${box.h}`);
```

**Rationale:** #elev-strip's CSS rule sizes it from calc(var(--elev-w) * 1px); author-stylesheet declarations always override SVG presentation attributes, so the width/height attributes never take effect. The other two fitted boxes (#plan-geometry, #mini-geometry) already ship without them.

**Test impact:** Rendered size provably unchanged (CSS wins today), but MEDIUM risk because ui_layout_check §21 measures the fitted strip and NC1 oversizes it — confirm the gate reads the rendered rect, not the attributes, and re-run §21 + NC1 after removal.
