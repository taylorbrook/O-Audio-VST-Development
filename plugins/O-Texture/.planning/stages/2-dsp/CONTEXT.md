# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-02-14
**Participants:** User, Claude

## Requirements Confirmed

- Full ANIRA inference pipeline with real trained ONNX model (Rain texture)
- PyTorch training pipeline included in this milestone (Rain VAE + Rain prior)
- Local NVIDIA GPU available for training (~6-12 hours per texture)
- Generative mode ONLY (prior -> decoder -> audio); Transform mode deferred
- O-Texture (synth variant) only; O-Texture FX (effect variant) deferred to future milestone
- Stereo decorrelation via dual-decode latent offset from the start (2x decoder calls)
- 1D Perlin noise for smooth Evolve random walk
- 48kHz only (no resampler needed)
- Overlap-add crossfading: 50% overlap, 4096-sample blocks, 2048-sample hop, Hann window
- Post-processing: tilt EQ (BRIGHTNESS parameter), no dry/wet mix (Generate-only, MIX irrelevant)
- Stage 2 verification requires real model output (not placeholder noise)

## Constraints Identified

- ANIRA model loading may require file paths (not memory buffers) -- research phase must determine if ONNX Runtime supports in-memory loading, otherwise extract binary data to temp files at runtime
- IS_SYNTH TRUE means no audio input bus in most DAWs -- Transform mode deferred entirely
- Placeholder models (Stage 1) are tiny/wrong architecture -- real Rain model needed for meaningful testing
- Prior model requires latent sequences extracted from trained VAE -- prior training follows VAE training
- Dimension mapping (which latent dims are "active") requires post-training latent space analysis
- ANIRA is a relatively new library (2024) -- may encounter undocumented behavior or API gaps

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Training scope | Rain only (1 texture) | Validate full pipeline end-to-end before training 5 more |
| Plugin variant | Generate-only synth | Simplifies bus config, defers Transform/FX to future |
| Stereo approach | Dual-decode latent offset | Organic stereo, test full CPU cost from day one |
| Evolve noise | 1D Perlin noise | Smooth organic evolution, matches architecture spec |
| Sample rate | 48kHz only | Models trained at 48kHz, no resampler complexity |
| Model loading | Research in-memory first | Best UX (embedded binary data), fallback to temp file extraction |
| Transform mode | Deferred entirely | IS_SYNTH limits audio input; dual plugin strategy chosen for future |
| Dry/wet mix | Deferred | No dry signal in Generate mode, MIX parameter non-functional until Transform mode |
| Remaining textures | Separate milestone | Rain-only validates architecture before committing to 5 more |

## Stage 2 Sub-Tasks (High Level)

### Phase 0: PyTorch Training (Rain)
1. **Training infrastructure** -- PyTorch VAE classes, dataset loader, training loop, loss functions
2. **Dataset curation** -- 30 min of diverse rain recordings, 48kHz mono, preprocessed
3. **VAE training** -- Train encoder + decoder (~100k-200k steps, ~6-12 hours on GPU)
4. **Quality validation** -- Reconstruction test, generation test, latent space analysis
5. **Prior training** -- Train GRU prior on latent sequences from trained VAE
6. **ONNX export** -- Export encoder, decoder, prior to ONNX format
7. **Dimension mapping** -- Analyze latent space, create dim_map_rain.json

### JUCE DSP Implementation
1. **ANIRA inference pipeline** -- Configure InferenceHandler, thread pool, request/response queues
2. **Model loading** -- Load Rain ONNX models (from binary data or temp file extraction)
3. **Latent space control** -- construct_latent_vector() with X/Y/Char/Evolve mapping
4. **Perlin noise evolve** -- 1D Perlin noise implementation for smooth random walk
5. **Generative mode** -- Prior model inference, latent history buffer, autoregressive generation
6. **Overlap-add crossfade** -- Hann window, two-buffer system, 50% overlap
7. **Stereo decorrelation** -- Dual-decode with latent offset (L/R channels)
8. **Post-processing EQ** -- Tilt filter (BRIGHTNESS parameter), juce::dsp::IIR::Filter
9. **Freeze control** -- Halt evolve, allow manual X/Y/Char changes

## Open Questions

- Can ANIRA/ONNX Runtime load models from memory buffers, or does it require file paths? (Research phase)
- What is the actual ANIRA API for submitting inference requests and retrieving results? (Research phase)
- How does ANIRA handle stereo (two simultaneous decoder requests)? Parallel or sequential? (Research phase)
- What dataset sources are available for Rain texture curation? (Research phase)
- Is the prior model's input format compatible with ANIRA's InferenceHandler? (variable-length sequence input) (Research phase)

## Key Architecture Reference

From ARCHITECTURE.md:
- Block size: 4096 samples (85.3ms at 48kHz)
- Hop size: 2048 samples (50% overlap)
- Latent dimensions: 32
- Active dimensions: ~8-12 (mapped to X/Y/Char A/B + evolve)
- Inactive dimensions: ~20-24 (sampled from N(0,1))
- Latency: ~6144 samples (~128ms at 48kHz), reported via setLatencySamples()
- Thread safety: Audio thread never blocks on inference (ANIRA lock-free queues)

## Existing Stage 1 Foundation

From VERIFICATION.md (all verified):
- CMakeLists.txt with ANIRA v2.0.3 FetchContent + ONNX Runtime 1.19.2
- 3 placeholder ONNX models embedded as binary data (ModelData namespace)
- 10 APVTS parameters with cached atomic pointers
- TextureProcessor: IS_SYNTH TRUE, stereo out + disabled sidechain, processBlock clears buffer
- TextureEditor: WebView with resource provider, 800x600, dark theme
- ANIRA + ONNX Runtime shared libs embedded in plugin bundles (Frameworks/)
- Plugin builds, links, registers as AU instrument (aumu OuTx OuDv)

## Next Phase

Ready for: **research** phase
- Investigate ANIRA API (InferenceHandler, request submission, result retrieval)
- Investigate ONNX Runtime memory loading (avoid temp file extraction)
- Research PyTorch 1D CNN VAE training best practices for audio texture synthesis
- Research Rain texture dataset sources and preprocessing
