# ML/AI Plugin & Application Briefs

Based on deep research into neural audio synthesis, ML inference frameworks, and market opportunities (Feb 2026).

---

## Brief 1: O-Morph — Neural Timbre Transfer Effect

**Concept**: Feed any audio in, get it resynthesized in a completely different timbre. Drums become cello. Guitar becomes choir. Voice becomes saxophone. Studio-grade timbre transfer as a simple effect plugin.

**Technical Approach**: RAVE v2 models with ONNX Runtime inference via ANIRA thread pool. Ship 4-6 pre-trained timbres (strings, brass, woodwinds, percussion, vocal, texture). Users select target timbre and blend with dry signal.

**Architecture**:
- RAVE encoder runs on input audio -> latent space
- Decoder trained on target timbre reconstructs from latents
- Wet/dry blend + post-EQ for integration
- Latent space parameters exposed as "character" knobs

**Key Parameters**:
- Timbre (model selector: Strings / Brass / Woodwind / Percussion / Vocal / Texture)
- Morph (wet/dry blend, 0-100%)
- Character X / Character Y (2D latent space navigation)
- Brightness (post-processing tilt EQ)
- Dynamics (how much of the input dynamics transfer)

**Latency**: 200-500ms (studio use, host-compensated). Clearly communicated in UI.

**Inference**: ONNX Runtime via ANIRA. Background thread. ~50MB model per timbre.

**Differentiation**: Scyclone exists but is experimental/niche. O-Morph would be polished, preset-rich, and approachable. No competitors have a "just pick a timbre" UX.

**Difficulty**: High (RAVE training, ANIRA integration, model management)
**Timeline**: 3-4 months
**Training Cost**: ~$200-400 for 6 models

---

## Brief 2: O-NeuralAmp — Neural Amp/Pedal Modeler

**Concept**: Load community-trained neural amp captures (NAM .nam files) inside an Ouaricon-branded plugin with a polished WebView UI, built-in cabinet IR loader, and signal chain (gate, EQ, cab, reverb).

**Technical Approach**: Port NeuralAmpModelerCore (C++ library) into JUCE. RTNeural handles inference directly on the audio thread. Zero added latency.

**Architecture**:
- Input -> Noise Gate -> Neural Amp Model -> Cabinet IR -> Post-EQ -> Reverb -> Output
- NAM Core for WaveNet/LSTM inference (Eigen backend)
- FFTConvolver for cabinet impulse responses
- Compatible with thousands of existing .nam community captures

**Key Parameters**:
- Model browser (load .nam files)
- Input Gain / Output Volume
- Cabinet IR loader (with bypass)
- Noise Gate (threshold, release)
- 3-band post-EQ
- Room reverb (size, mix)
- Blend (wet/dry for parallel processing)

**Latency**: <1ms (RTNeural, audio thread)

**Differentiation**: NAM's official plugin uses iPlug2 with a basic UI. nam-juce exists but is bare-bones. O-NeuralAmp would be the first polished, full-featured NAM-compatible plugin with WebView UI, signal chain, and Ouaricon design language.

**Difficulty**: Medium (NeuralAmpModelerCore is well-documented, proven C++ library)
**Timeline**: 6-8 weeks
**Training Cost**: $0 (uses community models)

---

## Brief 3: O-Texture — Neural Texture Synthesizer

**Concept**: A generative texture/drone instrument. Feed it a short audio sample (rain, metal scraping, crowd noise, synth pad), and it generates infinite evolving variations. Not a loop — a neural network continuously invents new texture based on what it learned.

**Technical Approach**: RAVE with prior model for unconditional generation. Train on texture/ambient/drone datasets. The prior model generates plausible latent sequences that the decoder turns into audio.

**Architecture**:
- Mode A (Generative): Prior model generates latent codes -> RAVE decoder -> audio
- Mode B (Transform): Audio input -> RAVE encoder -> latent manipulation -> decoder -> audio
- Latent space exposed as XY pad for navigation
- Macro controls mapped to latent dimensions via SVD analysis

**Key Parameters**:
- Source (select pre-trained model: Rain / Metal / Wind / Crowd / Synth / Organic)
- Seed (starting point in latent space)
- Evolve Rate (how fast the texture changes)
- XY Pad (navigate latent space in 2D)
- Density (sparse vs dense texture)
- Brightness / Warmth (post-processing)
- Freeze (hold current moment)

**Latency**: 200-500ms (acceptable for ambient/texture generation)

**Differentiation**: No plugin does this. Closest is granular synthesis (which loops samples) or Neutone (which requires technical setup). O-Texture makes neural generation accessible as a creative instrument.

**Difficulty**: High (RAVE + prior model training, generative UX design)
**Timeline**: 3-4 months
**Training Cost**: ~$300-500 for 6 texture models

---

## Brief 4: O-Resynth — DDSP Neural Instrument

**Concept**: A neural resynthesizer that takes monophonic audio or MIDI and resynthesizes it as a different instrument using DDSP. Hum into your mic, get violin out. Play MIDI keyboard, get flute. Tiny model, low CPU, low latency.

**Technical Approach**: DDSP (Differentiable DSP) — neural network predicts parameters for harmonic additive synth + filtered noise. Tiny CREPE for pitch detection on audio input. MIDI input mode bypasses pitch detection.

**Architecture**:
- Input: Audio (with pitch detection) OR MIDI
- Pitch (f0) + Loudness -> GRU decoder -> harmonic amplitudes + noise filter coefficients
- Harmonic additive synth (100 partials) + filtered noise synth
- Built-in reverb

**Key Parameters**:
- Instrument (model selector: Violin / Flute / Trumpet / Cello / Clarinet / Voice)
- Input Mode (Audio / MIDI)
- Pitch Tracking Sensitivity
- Harmonics (blend between harmonic and noise content)
- Vibrato (modulation of f0)
- Expression (dynamics response curve)
- Reverb (size, mix)

**Latency**: ~20ms (acceptable for keyboard/studio playing)

**Model Size**: ~1-5MB per instrument. All 6 fit in <30MB total.

**CPU**: Very low (~15 MFLOPS). Can run 10+ instances.

**Differentiation**: DDSP-VST (Google) is archived, bare-bones, and hard to use. O-Resynth packages it as a polished instrument plugin with preset instruments and intuitive controls. The "hum to violin" use case is immediately compelling.

**Difficulty**: Medium (DDSP is well-documented, small models, proven approach)
**Timeline**: 6-10 weeks
**Training Cost**: Free (Colab), 10-15 min of audio per instrument

---

## Brief 5: O-Capture — Neural Hardware Capture Tool

**Concept**: "ToneX for studio gear." Capture the sound of any analog hardware (compressor, preamp, tape machine, EQ) as a neural model, then use that model as a plugin. Not just for guitar amps — for mixing hardware.

**Technical Approach**: Train LSTM/GRU models on paired input/output recordings through target hardware. RTNeural inference on audio thread. Companion desktop app for capture + training workflow.

**Architecture**:
- **Capture App** (standalone): Records test signals through hardware, trains model, exports .ocap file
- **Plugin**: Loads .ocap files, runs RTNeural inference
- LSTM with 32-64 hidden units (proven for analog modeling)
- Oversampled processing for aliasing-free nonlinear behavior

**Capture Workflow**:
1. Connect hardware insert (send/return)
2. App sends 3-5 min of test signals through hardware at various settings
3. App trains model locally or via cloud (~30 min)
4. Export .ocap model file
5. Load in O-Capture plugin

**Key Parameters**:
- Model browser (load .ocap files)
- Drive / Input Level
- Mix (wet/dry)
- Oversampling (1x/2x/4x)
- Output Trim

**Latency**: <1ms (RTNeural, audio thread)

**Differentiation**: ToneX and NAM focus on guitar amps. Nobody has built a polished capture-and-use workflow for studio hardware (compressors, preamps, tape machines). Huge market of producers who want "that Neve sound" or "that LA-2A character."

**Difficulty**: Very High (training pipeline, capture app, model validation)
**Timeline**: 4-6 months
**Training Cost**: $0 (local training on Apple Silicon)

---

## Brief 6: O-VoiceMorph — AI Voice Transformer

**Concept**: Real-time voice transformation plugin. Change gender, age, character of a vocal recording. Not pitch shifting — neural voice conversion that preserves lyrics and expression while changing vocal identity.

**Technical Approach**: Encoder-decoder architecture (VAE or diffusion-based). Encode vocal features (pitch, phoneme, rhythm), decode with target voice identity. Could leverage SoundID VoiceAI-style approach or open-source voice conversion models (RVC, so-vits-svc).

**Architecture**:
- Audio input -> Feature extraction (f0, PPG, loudness)
- Neural encoder -> voice identity separated from content
- Target voice embedding applied
- Neural decoder -> transformed audio
- ANIRA for real-time safe inference

**Key Parameters**:
- Voice Preset (select target voice character)
- Gender (continuous male-female spectrum)
- Age (young-old spectrum)
- Breathiness
- Formant Shift
- Preserve Vibrato (amount of original expression to keep)
- Mix (wet/dry)

**Latency**: 50-200ms (acceptable for studio vocal production)

**Differentiation**: SoundID VoiceAI ($99) exists but has limited quality and control. Baby Audio Humanoid is more creative/extreme. O-VoiceMorph would target natural-sounding voice transformation for production (backing vocal doubling, character creation, demo production).

**Difficulty**: Very High (voice conversion is cutting-edge research, model quality is critical)
**Timeline**: 4-6 months
**Training Cost**: $100-300 for voice models

---

## Brief 7: O-SmartMix — Genre-Aware Mixing Assistant

**Concept**: AI-powered channel strip that analyzes your audio and suggests EQ, compression, and saturation settings optimized for a specific genre. Not "auto-mix" — it suggests, you refine.

**Technical Approach**: CNN classifier identifies instrument type and genre context. Lookup table of genre-specific processing targets (trained on reference mixes). Plugin suggests parameter values for its built-in EQ, compressor, and saturator. User can accept, modify, or ignore suggestions.

**Architecture**:
- Audio analysis CNN (runs once on playback start, lightweight)
- Genre/instrument classifier -> processing target lookup
- Built-in: 4-band parametric EQ + compressor + soft saturator
- "Suggest" button triggers analysis and sets initial parameters
- All parameters remain fully manual

**Key Parameters**:
- Genre (Auto-detect / Rock / Electronic / Jazz / Hip-Hop / Acoustic / Classical)
- Instrument Hint (Auto / Vocal / Guitar / Bass / Drums / Keys / Synth)
- Intensity (how aggressive the suggestions are, 0-100%)
- Standard channel strip controls (EQ bands, comp threshold/ratio/attack/release, saturation drive)
- A/B (compare suggested vs current settings)

**Latency**: Zero (analysis is offline, processing is traditional DSP)

**Model Size**: ~5-10MB (small CNN classifier)

**Differentiation**: Neutron/Sonible are genre-agnostic. A plugin that understands "this is a metal kick drum" vs "this is a jazz kick drum" and adjusts accordingly would be genuinely useful. The suggestion-based UX avoids the "black box" problem.

**Difficulty**: Medium-High (training data collection is the bottleneck)
**Timeline**: 2-3 months
**Training Cost**: $50-100 (classifier training)

---

## Comparison Matrix

| Brief | Plugin | Difficulty | Timeline | Novelty | Market Gap | CPU Cost | Latency |
|-------|--------|-----------|----------|---------|------------|----------|---------|
| 1 | O-Morph | High | 3-4 mo | High | Large | Moderate | 200-500ms |
| 2 | O-NeuralAmp | Medium | 6-8 wk | Low | Medium | Low | <1ms |
| 3 | O-Texture | High | 3-4 mo | Very High | Very Large | Moderate | 200-500ms |
| 4 | O-Resynth | Medium | 6-10 wk | High | Large | Very Low | ~20ms |
| 5 | O-Capture | Very High | 4-6 mo | High | Very Large | Low | <1ms |
| 6 | O-VoiceMorph | Very High | 4-6 mo | Medium | Medium | High | 50-200ms |
| 7 | O-SmartMix | Med-High | 2-3 mo | Medium | Medium | Very Low | 0ms |

## Recommended Starting Points

**If you want a quick win**: O-NeuralAmp (Brief 2) — proven technology, huge community model library, medium difficulty, shortest timeline.

**If you want maximum novelty**: O-Texture (Brief 3) — nothing like it exists. Generative neural textures as an instrument is a genuinely new category.

**If you want the best effort-to-impact ratio**: O-Resynth (Brief 4) — DDSP is easy to train (free, 10 min of audio), tiny models, low CPU, and "hum to violin" is an immediately compelling demo.

**If you want the biggest market opportunity**: O-Capture (Brief 5) — "ToneX for studio gear" addresses a massive unserved market, but it's the hardest to execute.
