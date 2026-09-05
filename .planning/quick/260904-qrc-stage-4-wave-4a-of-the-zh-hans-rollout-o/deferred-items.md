# Deferred items — out of scope for wave 4a

## boot-all-uis --strict-tips: 19 LATE tip bindings across 2 plugins

`O-Bells` (2) and `O-IntonationPad` (17) report `late-tips` — TIP_BINDINGS rows
whose selector misses the first sweep and resolves later, so they need a
re-sweep to carry a tip. **DEAD bindings: 0. failed: 0. warn: 0.**

Neither plugin is in wave 4a (O-IntonationPad is wave 4b). Both readings predate
this work — the wave-4a plugins contribute 0 late and 0 dead. Recorded rather
than fixed, per the executor scope boundary: only issues caused by this wave's
own changes are auto-fixed here.
