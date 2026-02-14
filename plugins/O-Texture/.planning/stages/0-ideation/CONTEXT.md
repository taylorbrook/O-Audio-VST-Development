# Stage 0 Context: O-Texture (Research & Planning)

**Date:** 2026-02-14
**Agent:** research-planning-agent
**Status:** Complete

---

## Research Summary

O-Texture is the **most complex plugin ever attempted in this codebase**. It exceeds normal complexity metrics and requires a completely novel implementation approach.

### Complexity Assessment

**Calculated Score:** 11.0 (capped at 5.0 maximum)
- Parameters: 10 (2.0 points)
- Algorithms: 9 components (9.0 points)
- Features: 0 (neural network IS the feature)

**Classification:** UNPRECEDENTED
- First machine learning plugin in codebase
- No existing reference implementation
- Requires separate PyTorch training pipeline (external to JUCE)
- Custom neural network architecture (~7.8M parameters across 6 models)

### Tier 6 Complexity Indicators

This plugin exhibits ALL Tier 6 characteristics:
1. Custom neural network training (PyTorch, separate from JUCE) - 3-6 weeks
2. ONNX Runtime + ANIRA integration (novel for codebase) - 2-3 weeks
3. Real-time ML inference with thread pool management - HIGH risk
4. Block-based audio synthesis with overlap-add (not sample-by-sample DSP)
5. 18 ONNX model files (6 textures × 3 models each = ~31MB)
6. Non-standard stage structure (requires PyTorch Phase 0 BEFORE JUCE Stages 1-4)

**Estimated timeline:** 9-14 weeks (vs 2-4 weeks for typical complex plugin)

---

## Key Decisions

### Decision 1: Custom 1D CNN VAE vs. RAVE

**Chosen:** Custom 1D CNN VAE (32-dim latent space)

**Rationale:**
- Compact latent space (32 dims) easier to control than RAVE's 128 dims
- XY pad maps to top 2 active dimensions → intuitive navigation
- Tailored to texture synthesis (not general audio)
- Smaller model size (~1.2M params vs RAVE's 9M) → lower CPU, smaller binary
- No GPL license dependency (RAVE is GPL, limits commercial use)

**Tradeoffs accepted:**
- Higher development time (3-6 weeks training from scratch)
- Training risk (may produce lower quality than RAVE)
- No user-trainable models (plugin ships with 6 fixed textures)

**When to revisit:**
- If VAE training produces poor quality after spectral loss + adversarial tuning → consider RAVE
- If 32-dim latent space insufficient → increase to 64 dims
- If binary size exceeds 100MB → INT8 quantization

---

### Decision 2: ANIRA + ONNX Runtime vs. RTNeural

**Chosen:** ANIRA (Architecture for Neural network Inference in Real-time Audio applications) with ONNX Runtime backend

**Rationale:**
- Real-time safety: ANIRA decouples inference from audio thread (prevents dropouts)
- Thread pool management: Built-in high-priority threads (no manual thread management)
- Latency management: Handles buffer sizing, request queuing, underrun gracefully
- State-of-the-art: ANIRA is cutting-edge research (2024 paper)

**Tradeoffs accepted:**
- Binary size: ONNX Runtime adds ~50MB (vs ~10MB for RTNeural)
- External dependency: ANIRA is new (2024, may have bugs)
- CMake complexity: Careful configuration required

**When to revisit:**
- If ANIRA causes stability issues → fallback to RTNeural
- If binary size exceeds 150MB → INT8 quantization or RTNeural

---

### Decision 3: 50% Overlap-Add vs. 75% Overlap

**Chosen:** 50% overlap (2048-sample hop, 4096-sample blocks) with Hann windowing

**Rationale:**
- Industry standard for STFT-like processing (proven, well-understood)
- Perfect reconstruction: Hann window at 50% overlap sums to unity
- Lower CPU: Two concurrent blocks vs three for 75% overlap
- Simpler buffer management: Two-buffer system

**Tradeoffs accepted:**
- Latency: 50% overlap adds ~42ms (2048 samples @ 48kHz)
- Two-buffer memory: 32KB @ 32-bit float stereo (negligible)

**When to revisit:**
- If crossfade artifacts audible → increase to 75% overlap
- If latency critical → reduce block size to 2048 samples

---

### Decision 4: Latent Offset Stereo vs. Allpass Decorrelation

**Chosen:** Latent space offset for stereo (generate two slightly different latent vectors, decode both)

**Rationale:**
- Organic stereo: L/R channels differ in texture (not just phase)
- Semantic stereo: Offset in latent space creates timbral variation
- Controllable: Offset magnitude tuned per texture
- Consistent with VAE design: Exploits learned latent space structure

**Tradeoffs accepted:**
- Double inference cost: Two decoder calls per hop (L and R)
- Higher complexity: Manage two ANIRA inference requests

**When to revisit:**
- If CPU usage exceeds target → fallback to allpass decorrelation
- If stereo image too narrow → increase offset magnitude
- If stereo image too wide → decrease offset or use allpass

---

## Implementation Approach

### Non-Standard Stage Structure

**Traditional plugin workflow does NOT apply:**
- Standard: Stage 0 → Stage 1 (Foundation) → Stage 2 (Shell) → Stage 3 (DSP) → Stage 4 (GUI) → Stage 5 (Validation)
- O-Texture: **Phase 0 (PyTorch Training)** → Stage 1 → Stage 2 → Stage 3 → Stage 4 → Stage 5

**Phase 0 is EXTERNAL to JUCE:**
- Python/PyTorch environment (not C++)
- Training scripts separate from JUCE codebase
- Must complete before JUCE implementation (Stage 1-5)
- Longest phase: 3-6 weeks (~50% of project time)

**Why different:**
- Plugin requires pre-trained ONNX models to function
- Cannot test JUCE plugin without trained models
- PyTorch training is separate skill set (data science, not C++ audio programming)

### Phase 0: PyTorch Training (PREREQUISITE)

**Duration:** 3-6 weeks
**Location:** Separate Python project (NOT in JUCE codebase)

**Sub-phases:**
1. Training infrastructure setup (3-5 days)
2. Train first model (Rain) to validate architecture (5-7 days)
3. Optional adversarial fine-tuning if quality insufficient (3-5 days)
4. Train remaining 5 textures (1-2 weeks)
5. Train prior models for Generative mode (3-5 days)
6. Model validation & packaging (2-3 days)

**Output:** 18 ONNX files (6 textures × 3 models), 6 dimension mapping JSON files, ~31MB total

**Decision point after Phase 0.2 (first model):**
- Quality GOOD → Proceed to remaining textures
- Quality MEDIOCRE → Add adversarial fine-tuning
- Quality POOR → Pivot to RAVE or granular synthesis fallback

---

### Staged Implementation (JUCE Stages 1-5)

**Stage 1: Foundation (2-3 weeks)**
- CMake + ANIRA integration (3-5 days)
- Model loading infrastructure (3-5 days)
- APVTS parameter structure (1-2 days)

**Stage 2: DSP (3-4 weeks)**
- ANIRA inference pipeline (5-7 days)
- Latent space control system (4-6 days)
- Generative mode (prior model) (3-4 days)
- Transform mode (encoder) (3-4 days)
- Overlap-add crossfading (2-3 days)
- Post-processing (EQ + stereo) (2-3 days)

**Stage 3: GUI (2-3 weeks)**
- WebView shell + XY pad layout (3-5 days)
- Parameter binding (X/Y/Char/Evolve/Brightness/Mix) (3-5 days)
- Model selector + mode toggle (2-3 days)
- Freeze button (1-2 days)
- Optional: Particle visualization (2-3 days)

**Stage 4: Validation (1-2 weeks)**
- Perceptual quality testing (3-5 days)
- Performance benchmarking (2-3 days)
- Preset creation (2-3 days)
- Pluginval + cross-DAW testing (2-3 days)
- Documentation & release prep (1-2 days)

---

## Risk Assessment

### HIGH Risk Components

**1. Custom VAE Training (Phase 0.2-0.3)**
- Risk: VAE may produce muffled/blurry audio (common VAE failure mode)
- Probability: MEDIUM (spectral loss mitigates, but not guaranteed)
- Impact: HIGH (blocks entire project if quality unacceptable)
- Mitigation: Multi-scale spectral loss + adversarial fine-tuning
- Fallback: Use pre-trained RAVE models (GPL licensing implications)

**2. Latent Space Controllability**
- Risk: 32-dim latent space may not disentangle semantically (X/Y have unpredictable effects)
- Probability: LOW (VAE with KL regularization encourages disentanglement)
- Impact: MEDIUM (controls work but are less intuitive)
- Mitigation: Post-training dimension analysis, map most active dims to X/Y
- Fallback: Increase latent dims to 64 (more space for meaningful variation)

**3. ANIRA Integration Stability**
- Risk: ANIRA is new library (2024), may have bugs or API changes
- Probability: MEDIUM (cutting-edge research code)
- Impact: MEDIUM (blocks Stage 2 implementation)
- Mitigation: Test early (Stage 1.2), monitor GitHub for updates
- Fallback: RTNeural (simpler, but no thread pool → potential dropouts)

### MEDIUM Risk Components

**4. Overlap-Add Crossfading**
- Risk: Crossfade artifacts (clicks, pops, amplitude modulation)
- Probability: LOW (50% Hann overlap is proven technique)
- Impact: MEDIUM (audible quality degradation)
- Mitigation: Test extensively, tune window if needed
- Fallback: Increase to 75% overlap (smoother, more CPU)

**5. CPU Performance**
- Risk: Decoder inference may exceed 20% single core target
- Probability: MEDIUM (1.2M params is moderate, but 2x for stereo)
- Impact: MEDIUM (users complain about CPU usage)
- Mitigation: Benchmark early (Stage 2.1), profile ONNX Runtime
- Fallback: INT8 quantization (reduces inference time ~2-3x)

### LOW Risk Components

**6. WebView XY Pad UI**
- Risk: XY pad interaction feels sluggish or unresponsive
- Probability: LOW (standard WebView pattern, existing reference implementations)
- Impact: LOW (UI polish issue, not functional failure)
- Mitigation: Use relative drag pattern (see juce8-critical-patterns.md Pattern 16)

**7. Binary Size**
- Risk: Plugin exceeds 150MB (distribution limits)
- Probability: LOW (~80-100MB expected with 6 models + ONNX Runtime)
- Impact: LOW (users with limited disk space may complain)
- Mitigation: INT8 quantization reduces models ~75% (~20MB total)

---

## Constraints & Requirements

### Hard Requirements

1. **Sample rate: 48kHz only (v1)**
   - Models trained at 48kHz (fixed during training)
   - If host rate ≠ 48kHz → Add resampler (future feature)
   - Rationale: Simplify v1 scope, add resampling if users request

2. **Latency: ~85-128ms in Transform mode**
   - 4096-sample block size (cannot reduce without retraining models)
   - Host-compensated (DAW aligns timeline)
   - Acceptable for post-processing (not live tracking)
   - Rationale: Block size chosen for quality (larger blocks → better texture statistics)

3. **Thread safety: Audio thread NEVER blocks**
   - No ONNX inference on audio thread (runs on ANIRA background threads)
   - No model loading on audio thread (runs on message thread)
   - Communication via atomics and lock-free queues only
   - Rationale: Real-time safety is non-negotiable for plugin stability

4. **Binary size: <150MB**
   - 6 textures × ~5MB ONNX = ~30MB models
   - ONNX Runtime static lib = ~50MB
   - Total plugin binary = ~80-100MB (acceptable)
   - Rationale: Modern systems handle 100MB plugins, quality worth the size

### Soft Requirements (Nice-to-Have)

1. **User-trainable models**
   - Out of scope for v1 (complex UX, training time, GPU requirements)
   - Future feature: Load custom audio, train on device (may require cloud training)

2. **INT8 quantization**
   - Out of scope for v1 (test FP32 quality first)
   - Future optimization: Reduces model size ~75%, inference time ~2-3x faster

3. **Additional sample rates (44.1kHz, 96kHz)**
   - Out of scope for v1 (train models at 48kHz only)
   - Future feature: Add resampler or train models at multiple rates

4. **MIDI control of latent dimensions**
   - Out of scope for v1 (focus on mouse/automation control)
   - Future feature: Map MIDI CCs to X/Y/Char/Evolve for expressive performance

---

## Fallback Plans

### If VAE Training Fails (Phase 0)

**Option A: Pre-trained RAVE models**
- Download existing RAVE models from IRCAM/HuggingFace
- Deploy in plugin via ONNX export
- Tradeoff: 128-dim latent (harder to control), GPL license
- Timeline: Saves 3-6 weeks (skip custom training)

**Option B: Pivot to granular synthesis**
- Traditional sample-based texture synthesis (not neural)
- Load audio files, granular resynthesis with randomization
- Tradeoff: Not "neural network plugin", but achieves similar infinite texture goal
- Timeline: Moderate complexity (2-3 weeks for granular engine)

**Option C: Delay v1 release**
- Invest additional 2-3 weeks in adversarial training
- Iterate on architecture (try different encoder/decoder configs)
- Tradeoff: Longer timeline, but higher quality outcome

**Decision point:** After Phase 0.2 (first model trained), evaluate perceptual quality
- Quality GOOD → Proceed with custom VAE
- Quality MEDIOCRE → Add adversarial fine-tuning (Option C)
- Quality POOR → Consider Options A or B

---

### If ANIRA Integration Fails (Stage 1-2)

**Option A: RTNeural**
- Header-only ONNX inference library (simpler integration)
- Inference runs on audio thread (no thread pool)
- Tradeoff: Risk of dropouts with large models, but proven to work
- Timeline: 1-2 days to switch (RTNeural is simpler)

**Option B: Manual ONNX Runtime + custom thread pool**
- Implement own thread pool, request queue, latency management
- Full control over inference pipeline
- Tradeoff: High complexity, reinventing wheel (2-3 weeks)
- Use case: Only if ANIRA AND RTNeural both fail

**Decision point:** Stage 1.2 (ANIRA inference pipeline)
- If ANIRA stable → Continue with ANIRA
- If ANIRA buggy/complex → Switch to RTNeural (Option A)
- If both fail (unlikely) → Manual implementation (Option B)

---

## Research Resources Consulted

### ANIRA (Real-Time ML Inference)
- GitHub: https://github.com/anira-project/anira
- Paper: https://arxiv.org/pdf/2506.12665 (ANIRA: An Architecture for Neural Network Inference in Real-Time Audio Applications)
- Key insight: Thread pool decoupling essential for real-time safety with large models

### RAVE (VAE Audio Synthesis)
- GitHub: https://github.com/acids-ircam/RAVE
- Paper: https://arxiv.org/abs/2111.05011 (RAVE: A variational autoencoder for fast and high-quality neural audio synthesis)
- Key insight: Multi-band decomposition + adversarial training improves perceptual quality

### Overlap-Add Processing
- CCRMA: https://ccrma.stanford.edu/~jos/sasp/Overlap_Add_OLA_STFT_Processing.html
- Key insight: Hann window at 50% overlap provides perfect reconstruction (COLA property)

### Latent Space Control
- Frontiers: https://www.frontiersin.org/journals/computer-science/articles/10.3389/fcomp.2025.1575202/full
- Key insight: XY pad mapping to top 2 active dimensions most intuitive for users

### ONNX Runtime
- GitHub: https://github.com/microsoft/onnxruntime
- Documentation: https://onnxruntime.ai/docs/get-started/with-cpp.html
- Key insight: Static linking with pruned operators reduces binary size significantly

---

## Next Steps

**Immediate (Stage 1):**
1. Set up PyTorch training environment (Phase 0.1)
2. Implement training infrastructure (TextureVAE, TextureVAELoss, training loop)
3. Curate Rain texture dataset (30 min of diverse recordings)
4. Train first VAE model (Rain) to validate architecture (5-7 days)
5. Evaluate quality → Decide on adversarial fine-tuning or proceed

**After Phase 0 Complete:**
1. Stage 1: CMake + ANIRA integration (2-3 weeks)
2. Stage 2: ANIRA inference pipeline + latent control (3-4 weeks)
3. Stage 3: WebView XY pad UI (2-3 weeks)
4. Stage 4: Testing, presets, documentation (1-2 weeks)

**Critical path:** Phase 0 (PyTorch training) must complete before Stage 1 (JUCE implementation) can begin.

---

## Notes for Future Agents

**This plugin is UNPRECEDENTED:**
- First ML plugin in codebase (no reference implementation)
- Requires external PyTorch training (not C++ skill set)
- Thread safety via ANIRA (novel pattern for codebase)
- Block-based synthesis (not sample-by-sample DSP)

**Do NOT assume standard JUCE DSP patterns:**
- No `juce::dsp::Reverb`, `juce::dsp::Delay`, etc.
- Core "DSP" is neural network inference (not traditional audio processing)
- JUCE DSP only used for post-processing (EQ, dry/wet mix)

**Do NOT skip Phase 0:**
- Cannot implement JUCE plugin without trained ONNX models
- Training is longest phase (~50% of project time)
- Quality validation happens in Phase 0 (not after plugin is built)

**Treat as research project:**
- Expect iteration and pivots
- Keep fallback architectures ready
- Document learnings for future ML plugins in this codebase
