---
phase: quick-7
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - PLUGINS.md
autonomous: true
requirements: [QUICK-7]
must_haves:
  truths:
    - "O-Bells appears as the first plugin in the Ouaricon Plugins table"
  artifacts:
    - path: "PLUGINS.md"
      provides: "Plugin registry with reordered Ouaricon Plugins table"
      contains: "O-Bells"
  key_links: []
---

<objective>
Move O-Bells to the first row of the Ouaricon Plugins table in PLUGINS.md.

Purpose: Prioritize O-Bells visibility by placing it at the top of the product listing.
Output: Updated PLUGINS.md with O-Bells as the first entry in the Ouaricon Plugins table.
</objective>

<execution_context>
@/Users/taylorbrook/.claude/get-shit-done/workflows/execute-plan.md
@/Users/taylorbrook/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@PLUGINS.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Move O-Bells to first row in Ouaricon Plugins table</name>
  <files>PLUGINS.md</files>
  <action>
In PLUGINS.md, find the "Ouaricon Plugins:" table (the first table under "## Plugin Registry"). The O-Bells row currently reads:

```
| O-Bells | installed | 3.2.1 | Synth (Physical Modeling Bells) | 2026-02-19 |
```

Remove this row from its current position (approximately row 15 of the table) and insert it as the FIRST data row, immediately after the table header row (`|-------------|--------|---------|------|--------------|`).

Do NOT change any other rows or their ordering. Do NOT modify any content in the O-Bells row itself.
  </action>
  <verify>Read PLUGINS.md and confirm O-Bells is the first data row in the Ouaricon Plugins table (line immediately after the header separator).</verify>
  <done>O-Bells appears as the first plugin entry in the Ouaricon Plugins table, all other rows remain in their original order, no content changes.</done>
</task>

</tasks>

<verification>
- O-Bells is the first row after the table header in the Ouaricon Plugins section
- All other plugins remain in the same relative order
- No data was modified, only row position changed
</verification>

<success_criteria>
O-Bells is the first entry in the Ouaricon Plugins table in PLUGINS.md.
</success_criteria>

<output>
After completion, create `.planning/quick/7-move-o-bells-up-to-the-top-of-the-produc/7-SUMMARY.md`
</output>
