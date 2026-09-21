---
schema_version: 1
open_count: 1
waived_count: 0
fixed_count: 1
total_count: 2
last_updated: 2026-09-21T20:04:31.400Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | quick-260904-q4j | deviation | .planning/quick/260904-q4j-fix-i18n-zh-lint-before-stage-4-masklati/260904-q4j-PLAN.md |  | Baseline miscount: plan recorded 8 plugins carrying .scl/.kbm; live count is 7 (O-Lyrica has none). Count is definition-sensitive: 7 file-grep / 3 any-rendering / 1 zh-Hans rendering. | open |  | 2026-09-05T02:10:09.317Z |  |
| 2 | quick-260921-hno | deviation | scripts/gen-juce-overrides.sh |  | patch -t reverse-applies an already-applied patch; -N required (found by NC6, fixed in e49ab789) | fixed |  | 2026-09-21T20:04:07.722Z | 2026-09-21T20:04:31.400Z |

````json
[
  {
    "id": 1,
    "kind": "deviation",
    "phase": "quick-260904-q4j",
    "file": ".planning/quick/260904-q4j-fix-i18n-zh-lint-before-stage-4-masklati/260904-q4j-PLAN.md",
    "line": null,
    "description": "Baseline miscount: plan recorded 8 plugins carrying .scl/.kbm; live count is 7 (O-Lyrica has none). Count is definition-sensitive: 7 file-grep / 3 any-rendering / 1 zh-Hans rendering.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-05T02:10:09.317Z",
    "resolved_at": null
  },
  {
    "id": 2,
    "kind": "deviation",
    "phase": "quick-260921-hno",
    "file": "scripts/gen-juce-overrides.sh",
    "line": null,
    "description": "patch -t reverse-applies an already-applied patch; -N required (found by NC6, fixed in e49ab789)",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-09-21T20:04:07.722Z",
    "resolved_at": "2026-09-21T20:04:31.400Z"
  }
]
````
