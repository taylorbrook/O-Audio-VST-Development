# Image Asset Provenance — O-Emulator

## paper.jpg
- Copied from `plugins/O-Bitrot/Source/ui/public/img/paper.jpg` (itself from
  `plugins/O-Tremolo/Source/ui/public/img/paper.jpg`, the clean suite texture).
- md5 `40c5f97e25bd2492a6c8fe2ef0882541` — verified NOT the watermarked Adobe
  Stock texture (`b7c865c45f2fb95a7a8651071da186e6`) shipped by O-Lyrica/O-Gain.

## specimen.webp
- **Work:** Othniel Charles Marsh, *The Dinocerata: a monograph of an extinct
  order of gigantic mammals*, Monographs of the United States Geological
  Survey, Vol. X, Washington: Government Printing Office, 1886.
- **Plate:** full skeleton restoration of *Dinoceras mirabile* (Uintatherium),
  from the Internet Archive scan `dinoceratamonogr00mars`, page image 0487.
- **Copyright:** Public domain (US Government publication, 1886; author died
  1899). Faithful reproduction of a 2-D public-domain work.
- **Source file:** `~/Dev/Ouaricon Audio Images/skeletons/`
  `fulldino_dinoceratamonogr00mars_0487.png` (1007×665 RGBA, pre-existing
  clean alpha cut-out — 79.2% fully transparent, feathered edges; no
  background removal needed).
- **Processing (2026-08-21):** content-bbox crop (42,85)→(971,560) with 10px
  margin → 949×495, PIL crop; `cwebp -q 90 -metadata none` → 116 KB WebP.
  Filename hyphen-free (juce_add_binary_data strips hyphens).
