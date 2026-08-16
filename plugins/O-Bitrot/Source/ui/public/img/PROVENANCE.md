# Image Asset Provenance — O-Bitrot

## paper.jpg
- Copied from `plugins/O-Tremolo/Source/ui/public/img/paper.jpg` (clean suite texture).
- md5 `40c5f97e25bd2492a6c8fe2ef0882541` — verified NOT the watermarked Adobe Stock
  texture (`b7c865c45f2fb95a7a8651071da186e6`) shipped by O-Lyrica/O-Gain.

## specimen.webp
- **Work:** James Sowerby, *Coloured Figures of English Fungi or Mushrooms*,
  London: J. Davis, 1797–1809.
- **Plate:** t. 189 — *Coprinus comatus* (shaggy ink cap; drawn as *Agaricus
  cylindricus* Sow.), engraved 1798. Top specimen shown deliquescing (auto-digesting
  into ink) — the "decomposing specimen" motif.
- **Source scan:** Wikimedia Commons,
  `File:Coloured Figures of English Fungi or Mushrooms - t. 189.jpg`
  (https://commons.wikimedia.org/wiki/Special:FilePath/Coloured_Figures_of_English_Fungi_or_Mushrooms_-_t._189.jpg),
  from the Biodiversity Heritage Library digitization of the New York Botanical
  Garden (LuEsther T. Mertz Library) copy, BHL bibliography 6342,
  DOI 10.5962/bhl.title.6342.
- **Copyright:** Public domain (published 1798; author died 1822; BHL status
  NOT_IN_COPYRIGHT). Faithful reproduction of a 2-D public-domain work.
- **Processing (2026-08-15):** border-connected flood-fill background removal
  (paper tone ≈ rgb(238,223,184), tol 26), feathered alpha, content crop,
  Lanczos downscale to 876×1400, WebP q90. Script: session scratchpad
  `process_specimen.py` (throwaway).
