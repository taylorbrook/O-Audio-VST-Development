# Stage K — the plan's per-plugin numbers, MEASURED

Measured by the orchestrator before the first dispatch, with
`node scripts/i18n-extract.js --plugin <Name> --dry-run`, so each dispatch
starts from a corrected number instead of re-discovering the same drift.

**This does not replace step 1.** The executor still runs the extractor on its
own plugin and reviews every `UNSURE` and `READOUT` row by hand. These counts
are a budget, not a worklist.

## Frames and served roots — the plan was RIGHT about all 21

Every `setSize` matches the plan's frame, and every served root matches. The
one apparent divergence, `O-Detune setSize(0, 0)`, was an artifact of a survey
grep taking the first match across all of `Source/*.cpp`;
`PluginEditor.cpp:251` reads `setSize(600, 480)` exactly as the plan says.

`O-MicrotonalSampler` carries BOTH `Resources/ui` and `Source/ui/public`, as
the plan warns. The served root is `Resources/ui`; `Source/ui/public` is a
build-time JS-copy staging dir and is NOT embedded.

`PLUGIN_VERSION` — the silently-ignored non-keyword — appears in exactly two
Stage K plugins' `CMakeLists.txt`: **O-Reed** and **O-MicrotonalSampler**.
Both are already on the pending human-decision list. Report, do not fix.

## Text counts — the plan is wrong on 20 of 21, in BOTH directions

`LABEL` is the number that matters: rows the extractor classifies as
localizable. `READOUT`, `UNIT` and `UNSURE` are the hand-review load.

| Plugin | Plan | LABEL | READOUT | UNIT | UNSURE | attr | js-prose | js-composed |
|---|---|---|---|---|---|---|---|---|
| O-AnalogSaturation | 14 | *dispatch 1 measured it* | | | | | | |
| O-Texture | 16 | **21** | – | – | – | 6 | – | – |
| O-Emulator | 17 | **19** | – | – | 2 | 3 | 2 | – |
| O-Freeze | 23 | **18** | 4 | – | – | – | – | – |
| O-TextureForge | 25 | **21** | 2 | – | 1 | – | – | – |
| O-Bassoon | 33 | **29** | 5 | – | 2 | 3 | 1 | – |
| O-Detune | 34 | **30** | 4 | 1 | 4 | 5 | 1 | – |
| O-Chorus | 15 | **16** | 1 | – | 1 | 4 | – | – |
| O-DigiDelay | 16 | **21** | 1 | 1 | 2 | 5 | **4** | **1** |
| O-AnalogEQ | 25 | **20** | 8 | – | 2 | 5 | 1 | – |
| O-Bass | 12 | **16** | 2 | – | 2 | 7 | 2 | – |
| O-SimpleReverb | 20 | **26** | – | – | 1 | 6 | 2 | – |
| O-Comp | 19 | **22** | 5 | – | 2 | 5 | **6** | – |
| O-Tremolo | 19 | **23** | 1 | – | 2 | 7 | 1 | – |
| O-Bowed | 50 | **48** | 3 | – | 2 | 3 | 1 | – |
| O-Reed | 58 | **52** | 4 | – | 2 | – | 1 | – |
| O-GrainScatter | 73 | **70** | 2 | – | – | – | – | – |
| O-Wind | 65 | **61** | 7 | – | 3 | 3 | **4** | – |
| O-Bells | 84 | **79** | 8 | 1 | 3 | 1 | **4** | **3** |
| O-Formant | 109 | **94** | **21** | – | 3 | 5 | **4** | 1 |
| O-MicrotonalSampler | 126 | **146** | 6 | 4 | **19** | 14 | **24** | **12** |

### What this changes

- **The batch ORDER still holds.** Cheapest-first is preserved under the
  measured numbers; no plugin needs to move between batches.
- **The plan undercounted JS prose almost everywhere it counted it at all.**
  O-DigiDelay's `4 + 1 composed`, O-Comp's `6` and O-Wind's `4` are absent
  from the plan entirely. O-Bells' "1 + 2 composed" is `4 + 3`; O-Formant's
  "1 composed" is `4 + 1`; O-MicrotonalSampler's "6 + 8 composed" is
  **`24 + 12`**.
- **O-MicrotonalSampler is harder than the plan says, by a wide margin** —
  146 LABEL against a claimed 126, plus 19 `UNSURE` rows (the most in the
  stage by 4x) and 36 JS strings. The plan already calls it "the hardest
  plugin in this stage"; the measurement widens the gap rather than closing
  it. Budget for it accordingly and do it last.
- **O-Formant carries 21 `READOUT` rows** — the most in the stage. That is
  where the three-arm D-01 test earns its keep: arm 3 exempts a readout node
  regardless of the parameter type behind it.
