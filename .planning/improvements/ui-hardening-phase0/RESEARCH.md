# RESEARCH — ui-hardening-phase0

Research phase satisfied by `/research` Level 3 on 2026-10-26. Full reports copied into
`research/` (read-only evidence; line refs were valid at HEAD `ea44af28`):

| File | Scope | Phase-0 relevant sections |
|---|---|---|
| `research/B-reliability.md` | 34 UI bug classes, 20 open defects, upstream JUCE issues | open-defect list; WebView2 folder; `%f` JSON; dead native fns; CI gate red; generator templates |
| `research/A-duplication-census.md` | duplication/drift census, canon gates, module distribution | CI root cause (configure-time copies), candidate #0, O-Strata storage key |
| `research/C-external.md` | external architecture, migration strategy | `%TEMP%` list (16), Safari-15 floor, WebView2 shortcuts |
| `research/D-visual.md` + `contact-by-tier.jpg` | 46-UI screenshot audit, house tokens, tiers | Phase 4 only |

Reference implementation for the WebView2 folder + availability notice:
`git show 15968d17 -- plugins/O-simpleSubtractive/Source/PluginEditor.cpp`.

Known caveats: findings are from code reading + JUCE source + headless capture; none
reproduced in a DAW or on Windows. Re-verify each line ref at plan time (trees have moved).
