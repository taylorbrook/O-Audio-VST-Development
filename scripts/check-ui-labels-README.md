# `scripts/check-ui-labels.js` — the both-language label render gate

This is what retires **D-04**: "No French label clips, ellipsizes, or moves a
neighbouring element at any plugin's shipping frame size."

```bash
node scripts/check-ui-labels.js --plugin O-Tapestop
node scripts/check-ui-labels.js --plugin O-Tapestop --verbose   # keeps the served tree
node scripts/check-ui-labels.js --plugin O-Fixture --root /tmp/fix
```

| Exit | Meaning |
|---|---|
| `0` | Every assertion passed. |
| `n > 0` | `n` assertions failed. |
| `77` | Playwright unresolvable — **nothing was verified**. Never a pass. |
| `1` + "nothing to measure" | The plugin has **zero** `[data-i18n]` elements. Deliberately non-zero: a gate that passes because there is nothing to check is the vacuous pass this whole design exists to prevent. |

## One file, repo-level — the structural decision to overrule or confirm

The three tooltip clamp gates live in `plugins/<Name>/tests/`. This one does
not.

**For:** the no-shared-module rule in CONTEXT.md governs shipped UI *runtime*
code, not test tooling; `scripts/check-i18n.js` is already the precedent for a
repo-level per-plugin gate; and 43 hand-copies of a ~600-line Playwright file is
precisely the drift this repo has paid for twice.

**Against:** it does not run from a plugin's own `tests/` directory the way every
other gate there does, so a plugin-scoped CI job has to know about it.

Per-plugin knowledge lives in an optional
`plugins/<Name>/tests/i18n-states.json`:

```json
[
  { "name": "free mode",       "click": "#seg-sync-free" },
  { "name": "settings popover","click": "#settings-gear" }
]
```

Each entry re-runs the whole both-language sweep in that state. **When the file
is absent the gate reports how many `[data-i18n]` elements were never visible**
— a coverage hole is stated, not silently skipped. It is reported rather than
asserted because an unmeasured label is a coverage hole, not a geometry failure,
and conflating the two would make the gate lie in both directions.

## The geometry diff is the primary detector, not the clip check

Repo-wide there are only ~33 `text-overflow: ellipsis` declarations and ~110
`white-space: nowrap`. **Nine plugins have neither.** On those, French does not
clip a label — it wraps, the row grows taller, and the element below is pushed
down. A clip check is blind to that, and would certify exactly the plugins most
at risk.

So assertion **7** is load-bearing: at a fixed frame, any element that is
neither a label nor inside one must occupy the identical rectangle in both
languages, within 0.5px. Anything that moved was pushed by a French string, and
it is named with its delta.

It uses `getBoundingClientRect` only. `scrollHeight` does not cross a `flex: 1`
stage and is clamped on a grid container, so it lies about any *container*
(`pattern_scrollheight_does_not_cross_a_flex_stage`,
`pattern_scrollheight_clamped_on_grid_container`). That is why the clip check is
restricted to **leaf** label elements, and why the diff is immune.

The 0.5px tolerance is chosen between two real numbers: measured sub-pixel text
jitter is a few hundredths of a pixel, and the smallest real shift is one
line-height. Zero would drown the signal in noise; a loose value would hide a
wrap.

## The assertions

| # | Language-labelled | What |
|---|---|---|
| 1 | yes | Every visible `[data-i18n]` renders non-empty text, and no `{token}` placeholder survives. |
| 2 | — | **Vacuity guard.** At least 25% of labels must *differ* between the two passes, and keyed attributes must change too. |
| 3 | yes | `dataset.label === textContent` for every `[data-i18n]` — after the switch **and after a state-update pass**. |
| 4 | yes | No **leaf** label is clipped by its own `overflow`. |
| 5 | yes | Every label stays inside its `offsetParent`'s padding box. |
| 6 | yes | No label rect crosses the frame, and the document's own scroll extent does not either. |
| 7 | — | **The geometry diff**, plus: the visible element *set* is identical in both languages. |
| 8 | — | Two labels disjoint in English do not intersect in French. |

Every failure is labelled with its language. An unlabelled French-only failure
reads as a mysterious regression in a file that never mentions French.

**Assertion 2 is not optional.** A run in which `__setLanguage` silently did
nothing measures English twice and reports a confident, worthless pass. Stage D
proved this catches a real no-op on all three plugins it was tried against.

**Assertion 3's state pass is real, not nominal.** Where the generic stub is in
use, the gate drives every slider / toggle / combo state through
`window.__stubStates`, which fires the page's own `valueChangedEvent`
listeners. Without that the invariant would only be checked at the moment
nothing has happened — and the failure it guards
(`pattern_js_state_updater_overwrites_html_labels`) happens precisely when an
updater has run. The mechanism actually used is printed
(`stub slider states` or `events-only`), so a weaker run cannot be mistaken for
a strong one.

## The viewport is parsed, never mirrored

`setSize(W, H)` is read from that plugin's `PluginEditor.cpp`. A fixture
constant that mirrors a plugin constant has drifted silently in this repo twice
(`pattern_test_fixture_mirrors_drift_silently`). Cross-checked against the three
committed clamp gates' own `SHIP_W`/`SHIP_H`: O-Tapestop 860x580, O-Bitrot
900x740, O-ReverseDelay 940x768 — all three agree.

Playwright is given `viewport`, **not** `viewportSize`: the latter is the
getter's name, is silently ignored as a context option, and leaves Chromium's
1280x720 default. Measuring French overflow at a viewport wider than the plugin
ships is the exact failure this file exists to catch.

## Negative controls — run, and what they proved

All three were applied to a byte-exact backup of a canon-v2 fixture plugin and
restored **from that backup**, never `git checkout --`, which would have wiped
the uncommitted tooling alongside the mutation.

| Control | Fires |
|---|---|
| `window.__setLanguage` stubbed to a no-op | **[2]** — `0/6 labels (0%) differ`, and `0/2` attributes |
| A fixed-width `nowrap` French label widened ~200px | **[4]** — `knob-MIX 381>120` |
| A free-flow French label widened ~200px | **[7]** — the row grows `dh=16.9`, and `#filter` below it is named with `dy=16.9` |

The last two are the point. **The same mutation produces a different detector
depending on the CSS**, and on the free-flow shape — which nine plugins have —
the clip check stays silent while the geometry diff names the neighbour that
moved. That is the plan's fact 5, reproduced rather than asserted.

## Known limitation, carried openly

**The tool has not been cross-checked against a real plugin's committed clamp
gate**, because no real plugin is on canon v2 yet: run against O-Tapestop today
it correctly reports "nothing to measure" and exits 1. The agreement check
against `ui_tooltip_clamp_check.js` — same anchor counts, same geometry for the
elements both measure — is a **Stage F** step, and if the two disagree there the
tool is wrong, not the plugin.
