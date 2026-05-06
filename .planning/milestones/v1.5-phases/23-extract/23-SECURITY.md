---
phase: 23
slug: extract
status: verified
threats_open: 0
asvs_level: 1
created: 2026-04-26
---

# Phase 23 — Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| Patched JUCE wrapper → Plugin | `Vst3RawEvent` structs produced in-process by patched JUCE; consumed via `VST3ClientExtensions::onVst3RawEvent` | NoteOn/NoteOff/NE values (validated by host before reaching plugin) |
| Developer machine → JUCE fork | `apply-juce-patches.sh` writes into `/Users/taylorbrook/JUCE/` (developer-local fork) | Patch hunks (committed in repo, applied via `patch -p1`) |
| SharedCode TU ↔ VST3-only TU | Two-TU split with dispatch-slot indirection (`std::atomic<NEUpdateFn>`) keeps Steinberg symbols out of SharedCode | Function pointer registered by VST3 TU static-init |
| `auval` ↔ installed AU bundle | AU may link cleanly but fail load-time validation (code signing, plist, IID registration) | Bundle metadata + IIDs |

---

## Threat Register

| Threat ID | Category | Component | Disposition | Mitigation | Status |
|-----------|----------|-----------|-------------|------------|--------|
| T-23-01 | Tampering | `applyPendingTuning` (stale data on retrigger) | mitigate | `exchange(0.0, std::memory_order_acq_rel)` — `cpp/NoteExpression.h:74` | closed |
| T-23-02 | Denial | `onVst3RawEvent` (audio-thread alloc) | mitigate | `blockEvents.reserve(64); rawEventScratch.reserve(64);` — `cpp/NoteExpression.cpp:130-131` | closed |
| T-23-03 | Information Disclosure | Module header (debug artifacts) | mitigate | grep audit: 0 hits for neTrace/iidToHex/`<fstream>`/`<mutex>`/`OLyrica::detail` in `modules/tuning/note-expression/` | closed |
| T-23-04 | Tampering | `apply-juce-patches.sh` (partial write) | mitigate | Preflight `[[ ! -d "$JUCE_DIR" ]] exit 1` + idempotency `FOUND -ge 2 → exit 0` — `scripts/apply-juce-patches.sh:26-51` | closed |
| T-23-05 | Denial | `module.cmake` marker check (silent regression after JUCE upgrade) | mitigate | Two `FATAL_ERROR` calls referencing `scripts/apply-juce-patches.sh` — `module.cmake:27-29, 34-37` | closed |
| T-23-06 | Elevation | `module.cmake` (untrusted JUCE path) | accept | `JUCE_DIR` from env or platform default (`/Users/taylorbrook/JUCE`, `C:/JUCE`); developer-controlled, no user-supplied data — `module.cmake:13-19` | closed |
| T-23-07 | Tampering | Refactor composition order | mitigate | `getFrequency` (`HarpSynthVoice.cpp:115`) precedes `applyPendingTuning` (`:145`) — D-10 ordering | closed |
| T-23-08 | Denial | Build regression | mitigate | UAT Tests 4–5 + `23-VERIFICATION.md` truths #11–12: `OLyrica_VST3` + `OLyrica_AU` build clean | closed |
| T-23-09 | Information Disclosure | Audio-thread file I/O | mitigate | grep `#include <fstream>` across `plugins/O-Lyrica/Source/` and `modules/tuning/note-expression/` returns 0 | closed |
| T-23-10 | Denial | User skips cache clear | mitigate | CLAUDE.md cache-clear protocol codified at lines 18-25; invoked by Plan 23-04 Task 3 | closed |
| T-23-11 | Tampering | Stale bundle masks regression | mitigate | `rm -rf` old bundle before `cp -R` fresh — CLAUDE.md:22-25; `verify-au-link.sh:136` `killall -9 AudioComponentRegistrar` | closed |
| T-23-12 | Repudiation | "Works on my machine" | mitigate | Plan 23-04 Task 4 human-verify + UAT Tests 5–6 (Dorico microtonal regression, AU loads in non-Dorico DAW) | closed |
| T-23-05-01 | Tampering | `NoteExpression.h` Steinberg leak | mitigate | `grep -c "pluginterfaces" NoteExpression.h` = 0; `grep -c "JucePlugin_Build_VST3"` = 0; `Controller` forward-declared (`:87`); `std::unique_ptr<Controller, void(*)(Controller*)> nec;` (`:209`) | closed |
| T-23-05-02 | Denial | OLyrica AU/Standalone link | mitigate | Two-TU split: `cpp/NoteExpression.cpp` (Steinberg-free) + `cpp/vst3/NoteExpression_VST3.cpp` (VST3-only); per-format `target_sources` in `OuariconModules.cmake:96` | closed |
| T-23-05-02b | Denial | New ctor/dtor undef-symbol class | mitigate | `VST3Extensions::VST3Extensions()`, `~VST3Extensions() = default`, `drainAndUpdate()` SharedCode-bound — `NoteExpression.cpp:125-152` | closed |
| T-23-05-02c | Denial | `drainAndUpdate` `kTuningTypeID` re-leak | mitigate | `grep "Steinberg::" NoteExpression.cpp` returns 0 SDK symbols; `std::atomic<NEUpdateFn> g_neUpdate {nullptr}` dispatch slot — `:81` | closed |
| T-23-05-03 | Information Disclosure | Internal refactor — none applicable | accept | Internal C++ refactor; no user data, no PII, no network surface | closed |
| T-23-05-04 | Repudiation | "AU built clean" claim regression | mitigate | `scripts/verify-au-link.sh` exists, executable; invokes `auval -v "$AU_TYPE" "$PLUGIN_CODE" "$MANUF"` at `:141` | closed |
| T-23-05-05 | Spoofing | Stale on-disk AU artefact | mitigate | `verify-au-link.sh:136` `killall -9 AudioComponentRegistrar` + cache-clear sequence in CLAUDE.md (referenced by script header `:27-28`) | closed |
| T-23-05-06 | Elevation | Internal refactor — none applicable | accept | No new privileges, no new attack surface | closed |
| T-23-05-07 | Tampering | Per-format glob misroute | mitigate | `target_sources(${TARGET_NAME}_${_FMT_TARGET_SUFFIX} PRIVATE ${_FMT_SOURCES})` per-format routing — `OuariconModules.cmake:86-101` | closed |
| T-23-05-08 | Tampering | Static-init registration race | accept | `registerNEUpdate` uses `std::memory_order_release` (`:101-104`); `drainAndUpdate` paired `std::memory_order_acquire` (`:147`); rationale documented `:23-31, :75-94` | closed |

*Status: open · closed*
*Disposition: mitigate (implementation required) · accept (documented risk) · transfer (third-party)*

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| AR-23-01 | T-23-06 | `JUCE_DIR` is developer-controlled; default `/Users/taylorbrook/JUCE` or `C:/JUCE`; no user-supplied data flows into `file(READ)` | Taylor Brook | 2026-04-26 |
| AR-23-02 | T-23-05-03 | Phase 23 is an internal C++ refactor — no user data, no PII, no network surface introduced | Taylor Brook | 2026-04-26 |
| AR-23-03 | T-23-05-06 | Refactor introduces no new privileges or attack surface — module is in-process audio code | Taylor Brook | 2026-04-26 |
| AR-23-04 | T-23-05-08 | C++ static-init order guaranteed before `dlopen` returns; `std::atomic` acquire/release on dispatch slot gives well-defined memory ordering | Taylor Brook | 2026-04-26 |

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-04-26 | 22 | 22 | 0 | gsd-security-auditor (sonnet, quality profile) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-04-26
