# Phase 23 Deferred Items

## Plan 23-05 — discovered during execution

### AU validation: APVTS parameter Meta Param Flag setup
- **Discovered:** Plan 23-05 Task 6, sub-step 6.4 (verify-au-link.sh OLyrica)
- **Symptom:** `auval -v aumu OLyr OuDv` returns exit code 255 with:
  ```
  ParameterID=1275870432, Scope=0, Element=0: Saved Value = 0.337891, Current Value 0.000000
  ERROR: Parameter values are different since last set - probable cause: a Meta Param Flag is NOT set on a parameter that will change values of other parameters.
  Cannot perform Parameter Value check across initialization and reset
  ```
- **What works:** AU bundle loads, RENDER tests pass at every sample rate, MIDI test passes, parameter set/get works individually. The failure is solely on the cross-parameter Meta-Flag invariant check.
- **Root cause (pre-existing, NOT Plan 23-05 regression):** O-Lyrica has at least one APVTS parameter that mutates the value of other parameters when set, but does not declare `kAudioUnitParameterFlag_IsMeta`. The defect was masked in Plan 23-04 because the AU re-link itself failed (D-23-04-A) — auval was never previously invoked successfully against an OLyrica AU bundle.
- **Scope:** This is a logic change in O-Lyrica's parameter implementation, not a note-expression-module issue. Out of scope for Plan 23-05 which is strictly a defect-fix for the Steinberg-symbol AU/Standalone link failure.
- **Status:** AU bundle and Standalone now link cleanly (D-23-04-A original defect IS resolved). VST3 path is unaffected and is the canonical Dorico microtonal route. Phase 24 propagation is unblocked because the per-format module-source convention is in place — but Phase 24 verify gates that include `auval -v` will hit this same finding on every plugin that has cross-mutating APVTS parameters.
- **Recommended follow-up plan:** A small Phase 23 post-script or quick-task to audit O-Lyrica's APVTS parameter set, identify the cross-mutating parameter (ID 1275870432 — likely a "Meta" / preset-changing parameter), and add `kAudioUnitParameterFlag_IsMeta` via `juce::AudioProcessorParameter::isMetaParameter()` override. Same audit applies to all 7 Phase 24 target plugins.
- **Discovery commit context:** Plan 23-05 build log `/tmp/plan-23-05-build.log` (clean), auval log `/tmp/plan-23-05-auval.log` (Meta Param Flag error).
