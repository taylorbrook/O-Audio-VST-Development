# reverb-unit

Compact FDN plate reverb for embedding in Ouaricon VST instruments, sized to sit
under `compressor-unit` on an Effects tab.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `rev_enabled` | on/off | OFF | Master bypass (clickable title) |
| `rev_size` | 0–1 | 0.5 | Tank size; RT60 = 0.2 s + size² × 11.8 s |
| `rev_damp` | 0–1 | 0.5 | HF damping in the feedback loop |
| `rev_predelay` | 0–200 ms | 20 ms | Pre-delay |
| `rev_mix` | 0–1 | 0.25 | Linear dry/wet |
| `rev_mod` | 0–1 | 0.2 | Tank delay modulation depth |
| `rev_shimmer` | 0–1 | 0 | Octave-up feedback |

OFF by default, so adopting the unit does not change how existing presets or
sessions sound.

## C++

```cpp
#include "ReverbUnit.h"   // add modules/effects/reverb-unit/cpp to the include path

ReverbUnit reverbUnit;

// createParameterLayout():
ReverbUnit::addParameters(layout, "fx_rev_");

// constructor, after the APVTS exists:
reverbUnit.bind(parameters, "fx_rev_");

// prepareToPlay():
reverbUnit.prepare(sampleRate, samplesPerBlock);

// processBlock(), in place, mono or stereo:
reverbUnit.process(buffer);

// getTailLengthSeconds():
return reverbUnit.getTailLengthSeconds();
```

Turning the unit off (or Mix to 0) ramps the wet path down over 50 ms. The tank
idles only once the fade has landed, and turning it back on from idle starts from
a cleared tank.

## JavaScript

```js
import { ReverbUnit } from '../modules/reverb-unit.js';

const reverbUI = new ReverbUnit({
  container: document.getElementById('reverb-panel'),
  paramPrefix: 'fx_rev_',
  getSliderState: Juce.getSliderState,
  getToggleState: Juce.getToggleState
});
reverbUI.initialize();
```

Relays: one `WebToggleButtonRelay` (`<prefix>enabled`) and six
`WebSliderRelay`s. Serve the file at `/modules/reverb-unit.js` from
`getResource()`.

**Localization.** The captions are stable hooks: the title is
`.rev-bypass-toggle`, and each knob caption is
`[data-param="<name>"] .rev-label`. A host keys them with literal
`el.dataset.i18n = '…'` assignments, which is the shape `check-i18n` [15] reads.
For an example, see `plugins/O-Marimba/Source/ui/public/js/app.js`.

**Strip contract** (shared with `analog-eq-unit` / `compressor-unit` 1.3.0):
- Type is `var(--font-serif)` with an EB Garamond fallback; no text is smaller than 9px.
- The strip is 505 × 71px with 21px side padding. The bypass button is a fixed 76px, so
  nothing reflows when the language changes.
- Knob centres sit on a 64px pitch (48px groups, 16px gap, 6px lead-in), which puts each
  column on the EQ band centres above it.
- When bypassed, only the knob graphics fade; captions and readouts stay AA.
