# Phase 6: Domain Specialization - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

<domain>
## Phase Boundary

Encode professional domain expertise into agents that catch domain-specific quality issues. DSP agent enforces real-time safety, GUI agent enforces thread-safety, all agents follow JUCE 8 best practices. Professional quality standards defined. Music theory agent spec created with working prototype.

</domain>

<decisions>
## Implementation Decisions

### Real-time Safety Rules (DSP Agent)
- Zero tolerance for allocations, locks, syscalls in processBlock — reject any potential violation
- Pre-allocated memory pools allowed if allocation verified to occur in prepareToPlay
- std::function rejected in processBlock entirely (type erasure may allocate)
- Capture-less lambdas `[]` allowed (compile to function pointers)
- Lambdas with captures avoided unless proven non-allocating and not passed through std::function

### MessageManager Communication (Claude's Discretion)
- Claude determines rejection/suggestion based on what's being communicated
- Generally: reject MessageManager::callAsync from audio thread, suggest AsyncUpdater or atomic flag pattern

### Thread-safety Patterns (GUI Agent)
- APVTS best practices enforced: Attachment classes, atomic reads via getRawParameterValue, no direct setValue from audio thread
- Member declaration order strictly enforced: APVTS before Attachments (catches destruction-order crashes)
- WebView relay lifecycle strictly enforced:
  - Relays created in constructor (initializer list)
  - Connected in resized() after WebView has bounds
  - Disconnected in destructor before WebView destruction
  - Declaration order: relays after WebView
- Timer patterns enforced: stopTimer() in destructor, callbacks check validity

### Professional Quality Standards
- DSP metrics: Core mandatory (no DC offset, no clipping), recommended (THD, SNR thresholds)
- Reference comparison: Output quality should match commercial plugins (FabFilter, Soundtoys level)
- UI polish: Visual consistency required (spacing, alignment, font hierarchy), interaction quality recommended (smooth animations, responsive controls, hover/focus states)
- DAW compatibility: Must work in minimum DAW set — Logic Pro, Ableton Live, Pro Tools

### Music Theory Agent
- Scope: Both tuning/temperament AND harmonic analysis
- Role: Both design assistance (algorithm suggestions) AND validation (musical correctness)
- Focus: Generic musical theory (universal concepts), not instrument-specific
- Deliverable: Working prototype in Phase 6, not just spec

</decisions>

<specifics>
## Specific Ideas

- Real-time safety rules modeled on FabFilter/Valhalla/u-he approach (zero tolerance)
- WebView relay lifecycle follows CHOC/Blueprint patterns
- Music theory agent assists with pitch detection plugins, tuning systems (equal, just, Pythagorean, microtonal)

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 06-domain-specialization*
*Context gathered: 2026-01-31*
