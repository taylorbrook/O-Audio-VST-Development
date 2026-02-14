# O-Texture - Implementation Plan

**Date:** 2026-02-14
**Complexity Score:** 5.0 (MAXIMUM - Capped)
**Strategy:** Phased implementation with ML training prerequisite

---

## Complexity Factors

**Calculation breakdown:**

- **Parameters:** 10 parameters (10/5 = 2.0 points, capped at 2.0) = **2.0**
- **Algorithms:** 9 DSP/ML components = **9.0**
  - Custom 1D CNN VAE (encoder, decoder, prior) - counts as 3
  - ANIRA inference engine
  - Latent space control system
  - Overlap-add crossfade
  - Generative prior model
  - Transform mode encoder
  - Post-processing EQ
  - Stereo decorrelation
- **Features:** 0 points (no complexity features like FFT/multiband - those are inside neural network)
  - Neural network IS the feature (counted in algorithms)
- **Total:** 2.0 + 9.0 + 0 = **11.0 (capped at 5.0)**

**Classification:** COMPLEX (exceeds maximum score)

**Special notes:**
- This plugin exceeds normal complexity metrics
- Custom neural network training is not captured by standard algorithm count
- Actual complexity: UNPRECEDENTED in codebase (first ML plugin)
- Recommended timeline: 9-14 weeks (vs 2-4 weeks for typical complex plugin)

---

## Unique Stage Structure

**CRITICAL: This plugin has a NON-STANDARD stage structure.**

Traditional plugins: Stage 0 (Research) → Stage 1 (Foundation) → Stage 2 (Shell) → Stage 3 (DSP) → Stage 4 (GUI) → Stage 5 (Validation)

**O-Texture requires:**
- **Phase 0: PyTorch Training (3-6 weeks)** - EXTERNAL to JUCE, must complete before JUCE implementation
- Stage 1: Foundation (CMake + ANIRA integration)
- Stage 2: Shell (Parameters)
- Stage 3: DSP (ML inference + audio pipeline)
- Stage 4: GUI (WebView XY pad)
- Stage 5: Validation (Testing + polish)

**Why different:**
- Plugin requires pre-trained ONNX models to function
- Cannot implement JUCE plugin without trained models (no way to test)
- PyTorch training is separate codebase (Python, not C++)
- Training is longest phase (~50% of project time)

---

## Phase 0: PyTorch Model Training (PREREQUISITE)

**Duration:** 3-6 weeks
**Location:** Separate Python/PyTorch project (NOT in JUCE codebase)
**Status:** NOT STARTED (must complete before JUCE implementation)

### Phase 0.1: Training Infrastructure Setup (3-5 days)

**Goal:** Set up PyTorch training environment and validate basic VAE architecture

**Tasks:**
- [ ] Install PyTorch 2.x, ONNX, ONNX Runtime Python bindings
- [ ] Implement `TextureEncoder` class (6-layer 1D CNN)
- [ ] Implement `TextureDecoder` class (4-layer transposed 1D CNN)
- [ ] Implement `TextureVAE` wrapper (encoder + decoder + reparameterization)
- [ ] Implement `TextureVAELoss` (multi-scale spectral + waveform L1 + KL divergence)
- [ ] Write dataset loader (`TextureDataset` class - 4096-sample random crops)
- [ ] Write training loop with KL warmup (0 → 0.001 over first 10-20k steps)

**Test Criteria:**
- [ ] Model compiles and runs forward pass (no errors)
- [ ] Loss calculation works (spectral + L1 + KL)
- [ ] Dataset loader produces 4096-sample crops
- [ ] Training loop runs for 100 steps without crashing

**Output:** `train_texture_vae.py` script + model architecture classes

---

### Phase 0.2: Train First Model (Rain) (5-7 days)

**Goal:** Train VAE on one texture category to validate architecture and quality

**Tasks:**
- [ ] Curate Rain texture dataset (30 min of diverse rain recordings)
  - Various intensities (light drizzle, heavy downpour)
  - Various surfaces (roof, pavement, leaves, water)
  - Clean recordings (no music, no speech)
- [ ] Preprocess audio (48kHz mono, normalize, remove silence)
- [ ] Train VAE for 100k-200k steps (~6-12 hours on RTX 3080/4090)
- [ ] Monitor losses (spectral, L1, KL) - ensure convergence
- [ ] Test reconstruction quality:
  - Encode real rain audio → decode → compare (should sound like input)
  - Generate from random z → listen (should sound like rain, not noise)
- [ ] Analyze latent space (run `analyze_latent_space()` - identify active dimensions)
- [ ] Export to ONNX (encoder.onnx, decoder.onnx)

**Test Criteria:**
- [ ] Reconstruction quality: Decoded audio sounds like input (not muffled or distorted)
- [ ] Generation quality: Random z → decoder produces rain-like texture (not white noise)
- [ ] Latent space: At least 8-12 active dimensions (variance ratio > 0.5)
- [ ] ONNX export: Models load in ONNX Runtime Python without errors

**Output:** `rain_vae_final.pt`, `rain/encoder.onnx`, `rain/decoder.onnx`, `rain_dim_map.json`

**Decision point:**
- If quality GOOD → Proceed to Phase 0.3 (train remaining textures)
- If quality MEDIOCRE → Add adversarial fine-tuning (Phase 0.2.5 - see below)
- If quality POOR → Revisit architecture or pivot to RAVE (see fallback plan)

---

### Phase 0.2.5: Adversarial Fine-Tuning (Optional - 3-5 days)

**Goal:** Improve perceptual quality with multi-scale discriminator (if VAE quality insufficient)

**Tasks:**
- [ ] Implement `MultiScaleDiscriminator` (3 scales: full res, 2x downsample, 4x downsample)
- [ ] Freeze encoder, fine-tune decoder against discriminator
- [ ] Train for 50k-100k steps (~3-6 hours)
- [ ] Compare quality before/after fine-tuning (informal listening test)

**Test Criteria:**
- [ ] Quality improvement: Less blurry/muffled, more realistic texture
- [ ] No training collapse: Discriminator doesn't dominate (generator still converges)

**Output:** `rain_vae_adversarial.pt`, updated ONNX exports

**Note:** Only run if Phase 0.2 quality is insufficient. Skip if VAE already sounds good.

---

### Phase 0.3: Train Remaining Textures (1-2 weeks)

**Goal:** Train VAEs for all 6 texture categories

**Tasks:**
- [ ] Curate datasets for 5 remaining categories (30 min each):
  - **Metal:** Industrial recordings, scrapes, drones, resonances
  - **Wind:** Field recordings, various speeds/surfaces
  - **Crowd:** Ambience, indoor/outdoor, various sizes
  - **Synth:** Synthesizer pads/drones, analog and digital
  - **Organic:** Nature recordings (fire, water, forest, insects)
- [ ] Train VAE for each category (100k-200k steps each = ~36-72 hours total)
- [ ] Test reconstruction and generation quality for each
- [ ] Analyze latent spaces (dimension activity per texture)
- [ ] Export to ONNX (18 files total: 6 textures × 3 models)
- [ ] Create dimension mapping files (which dims → X/Y/Char per texture)

**Test Criteria:**
- [ ] All 6 textures produce realistic audio (not muffled or distorted)
- [ ] Latent spaces have 8-12 active dimensions each
- [ ] Dimension mappings vary per texture (X/Y don't always map to same semantic meaning)

**Output:** 6 × `{texture}_vae_final.pt`, 18 × `{texture}/{encoder,decoder,prior}.onnx`, 6 × `{texture}_dim_map.json`

---

### Phase 0.4: Train Prior Models (3-5 days)

**Goal:** Train autoregressive GRU-based priors for Generative mode

**Tasks:**
- [ ] Implement `TexturePrior` class (2-layer GRU, 128 hidden dim)
- [ ] Write prior training script (`train_prior.py`)
- [ ] For each texture:
  - [ ] Extract latent sequences from VAE encoder (consecutive blocks from training audio)
  - [ ] Train prior on sequences (100 epochs, ~2-4 hours per texture)
  - [ ] Test generation: Does prior → decoder produce coherent, evolving texture?
  - [ ] Tune temperature parameter (avoid repetition, ensure variation)
- [ ] Export priors to ONNX (6 files: `{texture}/prior.onnx`)

**Test Criteria:**
- [ ] Autoregressive generation: Prior predicts plausible next latent vectors (not random noise)
- [ ] Temporal coherence: Generated sequences sound like evolving texture (not disjointed blocks)
- [ ] No repetition: Temperature tuning prevents obvious loops or patterns

**Output:** 6 × `{texture}_prior_final.pt`, 6 × `{texture}/prior.onnx`

---

### Phase 0.5: Model Validation & Packaging (2-3 days)

**Goal:** Final quality checks and prepare models for JUCE plugin integration

**Tasks:**
- [ ] Run inference tests in Python:
  - [ ] Load all 18 ONNX models in ONNX Runtime Python
  - [ ] Test Generative mode: Prior → Decoder → 4096-sample audio block
  - [ ] Test Transform mode: Real audio → Encoder → Decoder → reconstructed audio
  - [ ] Verify dimension mappings: X/Y/Char controls produce expected effects
- [ ] Benchmark performance:
  - [ ] Decoder inference time (~1-3ms per block on CPU)
  - [ ] Encoder inference time (Transform mode)
  - [ ] Prior inference time (Generative mode)
- [ ] Package models for plugin:
  - [ ] Create `models/` directory structure: `{rain,metal,wind,crowd,synth,organic}/{encoder,decoder,prior}.onnx`
  - [ ] Copy dimension mapping JSON files
  - [ ] Total size check (~31MB for all 6 textures)
- [ ] Document training process (notebook or README)

**Test Criteria:**
- [ ] All ONNX models load without errors
- [ ] Inference times acceptable (<5ms per block for decoder)
- [ ] Total model size <50MB (fits plugin size budget)

**Output:** `models/` directory ready for JUCE plugin, training documentation

---

## Stage 1: Foundation (JUCE Plugin Shell)

**Duration:** 2-3 weeks
**Prerequisites:** Phase 0 complete (trained ONNX models available)

### Stage 1.1: CMake + ANIRA Integration (3-5 days)

**Goal:** Set up JUCE project with ANIRA and ONNX Runtime dependencies

**Tasks:**
- [ ] Create `O-Texture/CMakeLists.txt` based on existing plugin template
- [ ] Add ANIRA as CMake subdirectory or FetchContent:
  ```cmake
  FetchContent_Declare(anira
    GIT_REPOSITORY https://github.com/anira-project/anira.git
    GIT_TAG main  # or specific commit/tag
  )
  FetchContent_MakeAvailable(anira)
  ```
- [ ] Link ONNX Runtime static library (via ANIRA's ONNX backend)
- [ ] Add model files to binary resources:
  ```cmake
  juce_add_binary_data(O-Texture_Models
    SOURCES
      models/rain/encoder.onnx
      models/rain/decoder.onnx
      models/rain/prior.onnx
      # ... repeat for all 6 textures
  )
  ```
- [ ] Configure plugin properties:
  ```cmake
  juce_add_plugin(O-Texture
    COMPANY_NAME "Ouaricon"
    PLUGIN_MANUFACTURER_CODE Ouar
    PLUGIN_CODE OTex
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Texture"
    IS_SYNTH FALSE  # Effect, not synth (Transform mode processes input)
    NEEDS_MIDI_INPUT FALSE  # No MIDI control (future version)
    NEEDS_WEB_BROWSER TRUE  # WebView UI
  )
  ```
- [ ] Create PluginProcessor skeleton (empty processBlock for now)
- [ ] Create PluginEditor skeleton (empty GUI for now)

**Test Criteria:**
- [ ] Plugin compiles without errors (CMake + Ninja)
- [ ] Plugin loads in DAW (shows in plugin list)
- [ ] ANIRA and ONNX Runtime link successfully (no missing symbols)
- [ ] Model files embedded in binary resources (check binary size ~80-100MB)

---

### Stage 1.2: Model Loading Infrastructure (3-5 days)

**Goal:** Load ONNX models at runtime and manage model switching

**Tasks:**
- [ ] Implement `ModelManager` class:
  - [ ] Load ONNX models from binary resources (`BinaryData::rain_encoder_onnx`, etc.)
  - [ ] Create ANIRA InferenceHandler sessions (encoder, decoder, prior)
  - [ ] Handle model switching when SOURCE parameter changes
  - [ ] Lazy loading (load on demand, not all 6 at startup)
- [ ] Implement model loading on message thread (not audio thread)
- [ ] Add "Loading..." state (show indicator during model swap, 1-2 seconds)
- [ ] Atomic pointer swap when new model ready (audio thread queries loaded model)
- [ ] Error handling:
  - [ ] Missing model file → Fall back to default texture (Rain), log error
  - [ ] Invalid ONNX file → Disable plugin (critical failure)

**Test Criteria:**
- [ ] Plugin loads default model (Rain) on startup
- [ ] Switching SOURCE parameter triggers model reload (1-2 second pause)
- [ ] Audio thread never blocks on model loading (async on message thread)
- [ ] No crashes when switching models during playback

---

### Stage 1.3: APVTS Parameter Structure (1-2 days)

**Goal:** Define all 10 parameters in AudioProcessorValueTreeState

**Tasks:**
- [ ] Create APVTS with 10 parameters:
  - [ ] SOURCE (Choice: 0-5, default 0) - Rain/Metal/Wind/Crowd/Synth/Organic
  - [ ] MODE (Choice: 0-1, default 0) - Generate/Transform
  - [ ] X (Float: 0.0-1.0, default 0.5) - Latent space X position
  - [ ] Y (Float: 0.0-1.0, default 0.5) - Latent space Y position
  - [ ] CHARACTER_A (Float: 0.0-1.0, default 0.5) - Third latent dimension
  - [ ] CHARACTER_B (Float: 0.0-1.0, default 0.5) - Fourth latent dimension
  - [ ] EVOLVE (Float: 0.0-1.0, default 0.3) - Random walk rate
  - [ ] FREEZE (Bool: 0/1, default 0) - Halt evolve
  - [ ] BRIGHTNESS (Float: -1.0-1.0, default 0.0) - Tilt EQ
  - [ ] MIX (Float: 0.0-1.0, default 1.0) - Dry/wet blend
- [ ] Add parameter listeners in PluginProcessor (for model switching on SOURCE change)
- [ ] Verify APVTS persistence (save/restore in DAW)

**Test Criteria:**
- [ ] All parameters visible in DAW automation list
- [ ] Parameter changes reflected in `getRawParameterValue()->load()`
- [ ] Presets save/restore parameter values correctly

---

## Stage 2: DSP Implementation (ML Inference Pipeline)

**Duration:** 3-4 weeks
**Prerequisites:** Stage 1 complete (CMake, ANIRA, parameters, models loaded)

### Stage 2.1: ANIRA Inference Pipeline (5-7 days)

**Goal:** Set up real-time safe neural network inference via ANIRA

**Tasks:**
- [ ] Configure ANIRA InferenceHandler:
  - [ ] Thread pool: 2 background threads (high priority)
  - [ ] Buffer sizes: 4096-sample blocks (85.3ms @ 48kHz)
  - [ ] Request queue depth: 4-8 slots (enough for smooth streaming)
- [ ] Implement inference request submission (audio thread):
  ```cpp
  // In processBlock(), every 2048 samples:
  std::vector<float> latent_z = construct_latent_vector();  // From user controls
  anira_decoder->submit_request(latent_z);
  ```
- [ ] Implement completed block retrieval (audio thread):
  ```cpp
  if (anira_decoder->has_completed_block()) {
    auto audio_block = anira_decoder->get_completed_block();
    crossfade_buffer.push(audio_block);
  }
  ```
- [ ] Handle underruns (if ANIRA can't keep up):
  - [ ] Repeat last block (audible but non-fatal)
  - [ ] Log warning (for debugging)

**Test Criteria:**
- [ ] ANIRA submits inference requests without blocking audio thread
- [ ] Decoded audio blocks returned to audio thread (lock-free queue)
- [ ] No audio dropouts or glitches under normal load
- [ ] Latency ~85-128ms (4096-sample block + overhead)

---

### Stage 2.2: Latent Space Control System (4-6 days)

**Goal:** Map user controls (X/Y/Char/Evolve) to 32-dimensional latent vectors

**Tasks:**
- [ ] Load dimension mapping JSON files (per texture):
  ```json
  {
    "texture": "rain",
    "active_dims": [0, 1, 2, 3, 4, 5, 7, 9],
    "inactive_dims": [6, 8, 10, 11, ...],
    "x_axis": 0,
    "y_axis": 1,
    "char_a": 2,
    "char_b": 3
  }
  ```
- [ ] Implement `construct_latent_vector()`:
  ```cpp
  std::vector<float> construct_latent_vector() {
    std::vector<float> z(32, 0.0f);

    // User controls → Active dimensions
    z[dim_map.x_axis] = X_param->load();
    z[dim_map.y_axis] = Y_param->load();
    z[dim_map.char_a] = CHARACTER_A_param->load();
    z[dim_map.char_b] = CHARACTER_B_param->load();

    // Evolve → Remaining active dimensions
    for (int i : remaining_active_dims) {
      z[i] = evolve_state[i];
    }

    // Inactive dimensions → Sample from N(0,1)
    for (int i : inactive_dims) {
      z[i] = random_normal(0.0f, 1.0f);
    }

    return z;
  }
  ```
- [ ] Implement evolve random walk:
  - [ ] Use Perlin noise for smooth evolution (or Brownian motion fallback)
  - [ ] Update every 2048 samples (hop size)
  - [ ] Rate controlled by EVOLVE parameter (0.0-1.0 → 0.0-0.01 step size)
  - [ ] Clamp positions to [-3.0, 3.0] (prevent drift)
- [ ] Implement FREEZE control (halt evolve, but X/Y/Char still work)

**Test Criteria:**
- [ ] X/Y parameters change decoder output (audible texture shift)
- [ ] Character A/B parameters affect timbre (secondary variation)
- [ ] Evolve causes slow texture evolution (organic drift)
- [ ] Freeze stops evolution (texture locks in place)
- [ ] Switching SOURCE reloads dimension mapping (X/Y map to different semantics)

---

### Stage 2.3: Generative Mode (Prior Model) (3-4 days)

**Goal:** Implement autoregressive latent generation for Generative mode

**Tasks:**
- [ ] Load prior ONNX model (via ANIRA, same as decoder)
- [ ] Implement latent history buffer (FIFO, last 16 vectors):
  ```cpp
  std::deque<std::vector<float>> latent_history;  // Max 16 x 32 floats
  ```
- [ ] Implement prior inference (every 2048 samples):
  ```cpp
  if (MODE_param->load() == 0) {  // Generative mode
    auto [mu, logvar] = anira_prior->infer(latent_history);
    auto z_next = reparameterize(mu, logvar);
    z_next = apply_user_controls(z_next);  // X/Y/Char/Evolve
    latent_history.push_back(z_next);
    if (latent_history.size() > 16) latent_history.pop_front();

    auto audio_block = anira_decoder->infer(z_next);
    crossfade_buffer.push(audio_block);
  }
  ```
- [ ] Handle mode switch (Generate ↔ Transform):
  - [ ] Clear latent history when switching to Generate
  - [ ] Seed with random z or load from preset

**Test Criteria:**
- [ ] Generative mode produces evolving texture (not static noise)
- [ ] Texture sounds coherent (not disjointed random blocks)
- [ ] User controls (X/Y/Char/Evolve) affect generated texture
- [ ] Mode switch doesn't crash (brief audio gap acceptable)

---

### Stage 2.4: Transform Mode (Encoder) (3-4 days)

**Goal:** Implement audio input encoding for Transform mode

**Tasks:**
- [ ] Configure input bus (stereo input for Transform mode):
  ```cpp
  AudioProcessor(BusesProperties()
    .withInput("Input", AudioChannelSet::stereo(), true)
    .withOutput("Output", AudioChannelSet::stereo(), true))
  ```
- [ ] Implement encoder inference (every 2048 samples):
  ```cpp
  if (MODE_param->load() == 1) {  // Transform mode
    auto input_block = audio_input_buffer.read(4096);  // Lookahead
    auto [mu, logvar] = anira_encoder->infer(input_block);
    auto z = reparameterize(mu, logvar);
    z = apply_user_controls(z);  // Overwrite X/Y/Char, add Evolve
    auto audio_block = anira_decoder->infer(z);
    crossfade_buffer.push(audio_block);
  }
  ```
- [ ] Handle input lookahead (4096-sample block for encoder)
- [ ] DryWetMixer setup (capture dry at input, blend at output)

**Test Criteria:**
- [ ] Transform mode processes input audio (not silent)
- [ ] Output sounds like input re-synthesized with target texture
- [ ] User controls impose target texture character (X/Y/Char override encoder values)
- [ ] Latency ~85-128ms (host-compensated)

---

### Stage 2.5: Overlap-Add Crossfading (2-3 days)

**Goal:** Seamlessly blend consecutive decoder blocks to avoid clicking

**Tasks:**
- [ ] Pre-compute Hann window (4096 samples):
  ```cpp
  std::vector<float> hann_window(4096);
  for (int i = 0; i < 4096; ++i) {
    hann_window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / 4096.0f));
  }
  ```
- [ ] Implement two-buffer crossfade:
  ```cpp
  std::vector<float> old_block(4096);
  std::vector<float> current_block(4096);

  // Every 2048 samples (hop size):
  old_block = current_block;
  current_block = anira_decoder->get_completed_block();

  // Output loop (per-sample in processBlock):
  for (int i = 0; i < buffer_size; ++i) {
    int block_pos = samples_since_hop + i;
    if (block_pos < 2048) {  // Overlap region
      float fade_out = hann_window[2048 + block_pos];
      float fade_in = hann_window[block_pos];
      output[i] = old_block[2048 + block_pos] * fade_out
                + current_block[block_pos] * fade_in;
    } else {  // No overlap (only new block)
      output[i] = current_block[block_pos];
    }
  }
  ```
- [ ] Handle startup (first block fades in from silence)
- [ ] Handle underrun (repeat last block if ANIRA can't keep up)

**Test Criteria:**
- [ ] No clicks or pops at block boundaries
- [ ] Smooth transitions between consecutive blocks
- [ ] First block fades in cleanly (no abrupt start)

---

### Stage 2.6: Post-Processing (EQ + Stereo) (2-3 days)

**Goal:** Add brightness control and stereo decorrelation

**Tasks:**
- [ ] Implement post-processing tilt EQ:
  ```cpp
  juce::dsp::IIR::Filter<float> tilt_filter;

  // In prepareToPlay:
  auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
    sampleRate, 2000.0, 0.707, juce::Decibels::decibelsToGain(BRIGHTNESS_param->load() * 6.0));
  tilt_filter.coefficients = coeffs;

  // In processBlock (after crossfade):
  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
  tilt_filter.process(context);
  ```
- [ ] Implement stereo decorrelation (latent offset):
  ```cpp
  // Generate two latent vectors (L/R)
  auto z_center = construct_latent_vector();
  auto z_left = z_center;
  auto z_right = z_center;
  for (int i = 0; i < 32; ++i) {
    float offset = uniform_random(-0.1f, 0.1f);
    z_left[i] += offset;
    z_right[i] -= offset;
  }

  // Decode both (two ANIRA requests)
  auto audio_left = anira_decoder->infer(z_left);
  auto audio_right = anira_decoder->infer(z_right);
  crossfade_buffer.push_stereo(audio_left, audio_right);
  ```
- [ ] Add DryWetMixer (Transform mode only):
  ```cpp
  juce::dsp::DryWetMixer<float> dry_wet_mixer;

  // In processBlock (Transform mode):
  dry_wet_mixer.setWetMixProportion(MIX_param->load());
  dry_wet_mixer.pushDrySamples(input_buffer);  // Capture dry at start
  // ... process through encoder → decoder → crossfade → EQ ...
  dry_wet_mixer.mixWetSamples(buffer);  // Blend dry with wet at end
  ```

**Test Criteria:**
- [ ] BRIGHTNESS parameter changes tonal balance (dark ↔ bright)
- [ ] Stereo output has natural width (not mono or exaggerated)
- [ ] MIX parameter blends dry/wet in Transform mode (0% = dry input, 100% = re-synthesized)
- [ ] MIX has no effect in Generate mode (no dry signal)

---

## Stage 3: GUI Implementation (WebView XY Pad)

**Duration:** 2-3 weeks
**Prerequisites:** Stage 2 complete (DSP working, audio output functional)

### Stage 3.1: WebView Shell + XY Pad Layout (3-5 days)

**Goal:** Create WebView UI with dominant XY pad interface

**Tasks:**
- [ ] Create `Source/ui/public/index.html`:
  - [ ] Header: O-TEXTURE logo, MODE toggle (Generate / Transform)
  - [ ] XY Pad: Large interactive area (~50% of UI, 400x400px)
  - [ ] Model selector: 6 buttons (Rain, Metal, Wind, Crowd, Synth, Organic)
  - [ ] Secondary controls: Character A/B knobs, Evolve knob, Brightness knob, Mix knob
  - [ ] Freeze button (toggle state, visual indication)
- [ ] Copy JUCE WebView assets (`js/juce/index.js`, `check_native_interop.js`)
- [ ] Configure CMakeLists.txt:
  ```cmake
  juce_add_binary_data(O-Texture_UIResources
    SOURCES
      Source/ui/public/index.html
      Source/ui/public/js/juce/index.js
      Source/ui/public/js/juce/check_native_interop.js
  )
  ```
- [ ] Implement PluginEditor WebView setup:
  ```cpp
  webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
      .withNativeIntegrationEnabled()
      .withResourceProvider([this](auto& url) { return getResource(url); })
  );
  addAndMakeVisible(*webView);
  webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
  ```

**Test Criteria:**
- [ ] WebView window opens with correct size (800x600px)
- [ ] All UI elements visible and styled correctly
- [ ] XY pad renders as large interactive area
- [ ] Layout matches mockup design (if mockup exists)

---

### Stage 3.2: Parameter Binding (X/Y/Char/Evolve/Brightness/Mix) (3-5 days)

**Goal:** Two-way parameter communication (UI ↔ DSP)

**Tasks:**
- [ ] Create WebSliderRelay for continuous parameters:
  ```cpp
  x_relay = std::make_unique<juce::WebSliderRelay>("X");
  y_relay = std::make_unique<juce::WebSliderRelay>("Y");
  char_a_relay = std::make_unique<juce::WebSliderRelay>("CHARACTER_A");
  char_b_relay = std::make_unique<juce::WebSliderRelay>("CHARACTER_B");
  evolve_relay = std::make_unique<juce::WebSliderRelay>("EVOLVE");
  brightness_relay = std::make_unique<juce::WebSliderRelay>("BRIGHTNESS");
  mix_relay = std::make_unique<juce::WebSliderRelay>("MIX");
  ```
- [ ] Create WebSliderParameterAttachment (JUCE 8 requires 3 params):
  ```cpp
  x_attachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *processorRef.parameters.getParameter("X"), *x_relay, nullptr);
  ```
- [ ] Implement JavaScript XY pad interaction:
  ```javascript
  import { getSliderState } from './js/juce/index.js';

  const xState = getSliderState("X");
  const yState = getSliderState("Y");

  xyPad.addEventListener('mousedown', (e) => {
    isDragging = true;
  });

  xyPad.addEventListener('mousemove', (e) => {
    if (!isDragging) return;
    const rect = xyPad.getBoundingClientRect();
    const x = (e.clientX - rect.left) / rect.width;  // 0.0-1.0
    const y = 1.0 - ((e.clientY - rect.top) / rect.height);  // Invert Y
    xState.setValue(x);
    yState.setValue(y);
  });

  // Update cursor position when parameters change (host automation)
  xState.valueChangedEvent.addListener(() => {
    updateXYCursor(xState.getNormalisedValue(), yState.getNormalisedValue());
  });
  yState.valueChangedEvent.addListener(() => {
    updateXYCursor(xState.getNormalisedValue(), yState.getNormalisedValue());
  });
  ```
- [ ] Implement rotary knobs (Character A/B, Evolve, Brightness, Mix):
  ```javascript
  // Relative drag pattern (frame-delta, not absolute)
  let lastY = 0;
  knob.addEventListener('mousedown', (e) => {
    isDragging = true;
    lastY = e.clientY;
  });
  document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;
    const deltaY = lastY - e.clientY;
    rotation += deltaY * 0.5;  // Sensitivity factor
    rotation = Math.max(-135, Math.min(135, rotation));
    knob.style.transform = `rotate(${rotation}deg)`;
    paramState.setValue(mapRotationToValue(rotation));
    lastY = e.clientY;  // Update for next frame
  });
  ```

**Test Criteria:**
- [ ] XY pad drag changes X and Y parameters
- [ ] Knob drag changes parameters (relative drag, not absolute)
- [ ] Host automation updates UI cursor/knob positions
- [ ] Preset changes update all UI elements
- [ ] No lag or visual glitches during interaction

---

### Stage 3.3: Model Selector + Mode Toggle (2-3 days)

**Goal:** UI controls for SOURCE and MODE parameters

**Tasks:**
- [ ] Implement model selector (6 buttons):
  ```javascript
  const sourceState = getSliderState("SOURCE");  // Choice parameter (0-5)

  rainButton.addEventListener('click', () => sourceState.setValue(0));
  metalButton.addEventListener('click', () => sourceState.setValue(1));
  windButton.addEventListener('click', () => sourceState.setValue(2));
  crowdButton.addEventListener('click', () => sourceState.setValue(3));
  synthButton.addEventListener('click', () => sourceState.setValue(4));
  organicButton.addEventListener('click', () => sourceState.setValue(5));

  // Highlight active model
  sourceState.valueChangedEvent.addListener(() => {
    const activeIndex = sourceState.getNormalisedValue() * 5;  // Map 0-1 to 0-5
    updateActiveButton(Math.round(activeIndex));
  });
  ```
- [ ] Implement mode toggle (Generate / Transform):
  ```javascript
  const modeState = getToggleState("MODE");  // Bool parameter (0/1)

  modeToggle.addEventListener('click', () => {
    modeState.setValue(!modeState.getValue());
  });

  modeState.valueChangedEvent.addListener(() => {
    updateModeUI(modeState.getValue());  // Show "Generate" or "Transform"
    // Gray out MIX in Generate mode (not functional)
    mixKnob.style.opacity = modeState.getValue() ? "1.0" : "0.5";
  });
  ```
- [ ] Show "Loading..." indicator during model swap (1-2 seconds)

**Test Criteria:**
- [ ] Clicking model buttons switches texture (audio changes)
- [ ] Mode toggle switches Generate/Transform (audio input enabled in Transform)
- [ ] MIX knob grayed out in Generate mode (visual feedback)
- [ ] Loading indicator appears during model swap (SOURCE change)

---

### Stage 3.4: Freeze Button (1-2 days)

**Goal:** UI control for FREEZE parameter (halt evolve)

**Tasks:**
- [ ] Implement freeze button:
  ```javascript
  const freezeState = getToggleState("FREEZE");

  freezeButton.addEventListener('click', () => {
    freezeState.setValue(!freezeState.getValue());
  });

  freezeState.valueChangedEvent.addListener(() => {
    updateFreezeVisual(freezeState.getValue());  // Border glow or frost effect
  });
  ```
- [ ] Add visual indication (border glow, icon change, color shift)

**Test Criteria:**
- [ ] Clicking freeze button stops texture evolution (audio locks in place)
- [ ] Freeze state visually indicated (border glow or icon change)
- [ ] X/Y/Char still work when frozen (manual control, not random walk)
- [ ] Unfreezing resumes evolution from current position (no jump)

---

### Stage 3.5: Particle Visualization (Optional - 2-3 days)

**Goal:** Animated particle visualization in XY pad showing latent space activity

**Tasks:**
- [ ] Implement Canvas-based particle system:
  ```javascript
  const canvas = document.getElementById('xy-canvas');
  const ctx = canvas.getContext('2d');

  // Particles represent latent dimensions
  const particles = [];
  for (let i = 0; i < remaining_active_dims.length; ++i) {
    particles.push({x: random(), y: random(), vx: 0, vy: 0});
  }

  // Animation loop
  function animate() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Update particle positions (simulate evolve random walk)
    for (let p of particles) {
      p.vx += (Math.random() - 0.5) * evolve_rate;
      p.vy += (Math.random() - 0.5) * evolve_rate;
      p.x += p.vx;
      p.y += p.vy;
      // Wrap around edges
      if (p.x < 0) p.x = canvas.width;
      if (p.x > canvas.width) p.x = 0;
      if (p.y < 0) p.y = canvas.height;
      if (p.y > canvas.height) p.y = 0;
    }

    // Draw particles
    ctx.fillStyle = 'rgba(212, 165, 116, 0.8)';
    for (let p of particles) {
      ctx.beginPath();
      ctx.arc(p.x, p.y, 3, 0, 2 * Math.PI);
      ctx.fill();
    }

    // Draw XY cursor
    const cursorX = xState.getNormalisedValue() * canvas.width;
    const cursorY = (1.0 - yState.getNormalisedValue()) * canvas.height;
    ctx.strokeStyle = '#d4a574';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.arc(cursorX, cursorY, 8, 0, 2 * Math.PI);
    ctx.stroke();

    requestAnimationFrame(animate);
  }
  animate();
  ```

**Test Criteria:**
- [ ] Particles animate smoothly (~60fps)
- [ ] Particle motion reflects evolve rate (faster when Evolve high)
- [ ] Particles freeze when FREEZE enabled (motion stops)
- [ ] XY cursor tracks user input (drag on XY pad)
- [ ] Visual doesn't impact CPU (Canvas rendering is efficient)

**Note:** This is optional polish. Skip if time is limited. Core functionality works without visualization.

---

## Stage 4: Validation & Testing

**Duration:** 1-2 weeks
**Prerequisites:** Stage 3 complete (GUI working, all parameters functional)

### Stage 4.1: Perceptual Quality Testing (3-5 days)

**Goal:** Validate neural network output quality across all textures

**Tasks:**
- [ ] Informal listening tests:
  - [ ] All 6 textures sound realistic (not muffled, distorted, or synthetic)
  - [ ] Generative mode produces evolving, non-repeating audio
  - [ ] Transform mode re-synthesizes input with target texture character
  - [ ] X/Y controls produce intuitive perceptual changes (not random)
  - [ ] Evolve creates organic evolution (not jittery or robotic)
  - [ ] Freeze locks texture without artifacts
- [ ] Compare to reference plugins (RAVE, Output Portal, granular synths)
- [ ] Test edge cases:
  - [ ] High Evolve rate (0.8-1.0) - should evolve smoothly, not chaotically
  - [ ] Extreme X/Y positions (corners of XY pad) - should produce valid textures
  - [ ] Rapid model switching - should not crash or glitch
  - [ ] Long playback (10+ minutes) - no quality degradation or drift

**Test Criteria:**
- [ ] Perceptual quality acceptable (not muffled or distorted)
- [ ] Latent space controls intuitive (X/Y have predictable effects)
- [ ] No artifacts (clicks, pops, dropouts, distortion)

---

### Stage 4.2: Performance Benchmarking (2-3 days)

**Goal:** Ensure CPU usage <20% single core, latency <150ms

**Tasks:**
- [ ] Profile CPU usage (DAW's performance meter or Activity Monitor):
  - [ ] Generative mode: Decoder + Prior inference (~10-15% target)
  - [ ] Transform mode: Encoder + Decoder inference (~15-20% target)
  - [ ] With all 6 textures loaded (lazy loading should prevent this)
- [ ] Measure latency:
  - [ ] getLatencySamples() reports ~4096-6144 samples (~85-128ms @ 48kHz)
  - [ ] Host compensates latency (no perceived delay in Transform mode)
- [ ] Test on lower-end CPU (e.g., Intel Core i5 8th gen, Apple M1):
  - [ ] If CPU >30% → Consider INT8 quantization or smaller models
- [ ] Test buffer size sensitivity (64, 128, 256, 512 samples):
  - [ ] No dropouts or glitches at any buffer size
  - [ ] ANIRA handles varying buffer sizes gracefully

**Test Criteria:**
- [ ] CPU usage <20% single core on modern CPU (M1, Ryzen 5000, Intel 12th gen)
- [ ] No audio dropouts or glitches under normal load
- [ ] Latency acceptable (<150ms in Transform mode, host-compensated)

---

### Stage 4.3: Preset Creation (2-3 days)

**Goal:** Create 10-15 presets showcasing textures and controls

**Tasks:**
- [ ] Create presets for each texture category:
  - [ ] **Rain - Light Drizzle:** X=0.3, Y=0.4, Evolve=0.2 (subtle, slow evolution)
  - [ ] **Rain - Heavy Downpour:** X=0.7, Y=0.6, Evolve=0.5 (intense, fast evolution)
  - [ ] **Metal - Industrial Drone:** X=0.5, Y=0.8, Char A=0.6, Evolve=0.1 (dark, dense)
  - [ ] **Wind - Gentle Breeze:** X=0.2, Y=0.3, Evolve=0.3 (airy, slow motion)
  - [ ] **Crowd - Distant Ambience:** X=0.4, Y=0.5, Char B=0.7, Evolve=0.4 (diffuse, evolving)
  - [ ] **Synth - Analog Pad:** X=0.6, Y=0.5, Char A=0.5, Evolve=0.2 (warm, stable)
  - [ ] **Organic - Forest Texture:** X=0.5, Y=0.6, Char B=0.6, Evolve=0.6 (lively, complex)
- [ ] Create creative presets (cross-texture exploration):
  - [ ] **Frozen Crystal:** Any texture, Freeze=On, Brightness=+0.5 (bright, static)
  - [ ] **Morphing Landscape:** Any texture, Evolve=0.8, X/Y animated (chaotic evolution)
  - [ ] **Subliminal Whisper:** Any texture, Mix=0.2 (subtle background layer)
- [ ] Test preset recall (all parameters update correctly)

**Test Criteria:**
- [ ] Presets load without errors
- [ ] Presets produce expected sounds (match preset names)
- [ ] Preset browser navigation works (DAW preset list)

---

### Stage 4.4: Pluginval + Cross-DAW Testing (2-3 days)

**Goal:** Validate plugin passes industry-standard tests and works in all DAWs

**Tasks:**
- [ ] Run pluginval (JUCE's plugin validator):
  ```bash
  pluginval --validate-in-process --strictness-level 10 \
    ~/Library/Audio/Plug-Ins/VST3/O-Texture.vst3
  ```
  - [ ] Fix any errors or warnings (critical failures)
  - [ ] Document known issues (if any non-critical warnings remain)
- [ ] Test in multiple DAWs:
  - [ ] **Ableton Live:** Load in instrument/effect rack, test automation
  - [ ] **Logic Pro:** Test AU format, automation, preset browser
  - [ ] **FL Studio:** Test VST3, routing, automation
  - [ ] **Reaper:** Test VST3/AU, advanced routing
  - [ ] **Standalone:** Test without DAW (audio device selection, MIDI input)
- [ ] Test host features:
  - [ ] Automation recording (X/Y/Evolve)
  - [ ] Preset save/recall
  - [ ] Multi-instance (3-4 instances simultaneously)
  - [ ] Latency compensation (Transform mode)

**Test Criteria:**
- [ ] Pluginval passes with no critical errors
- [ ] Plugin loads and works in all tested DAWs
- [ ] Host automation records and plays back correctly
- [ ] No crashes or hangs under normal use

---

### Stage 4.5: Documentation & Release Prep (1-2 days)

**Goal:** Prepare plugin for release (changelog, user guide, build artifacts)

**Tasks:**
- [ ] Write CHANGELOG.md entry for v1.0.0:
  - [ ] List all features (Generative/Transform modes, 6 textures, XY pad, Evolve, etc.)
  - [ ] Note system requirements (macOS 10.15+, Windows 10+, 8GB RAM, AVX2 CPU)
  - [ ] List known limitations (48kHz only, no user training, ~100MB binary size)
- [ ] Update README.md (if public release):
  - [ ] Description of plugin concept
  - [ ] Installation instructions
  - [ ] Quick start guide (load preset, adjust X/Y, explore textures)
  - [ ] Technical details (neural network architecture, latency, CPU usage)
- [ ] Build release artifacts:
  - [ ] macOS: Universal binary (ARM64 + x86_64) for VST3 + AU
  - [ ] Windows: x64 binary for VST3
  - [ ] Test installers (DMG for macOS, installer for Windows)
- [ ] Code sign binaries (macOS):
  ```bash
  codesign --force --sign - ~/Library/Audio/Plug-Ins/VST3/O-Texture.vst3
  codesign --force --sign - ~/Library/Audio/Plug-Ins/Components/O-Texture.component
  ```

**Test Criteria:**
- [ ] CHANGELOG.md up to date
- [ ] Release builds install and run on clean systems (no dev dependencies)
- [ ] macOS binaries code-signed and notarized (if public release)

---

## Implementation Notes

### Non-Standard Architecture

**O-Texture is NOT a traditional DSP plugin:**
- Core "DSP" is a neural network (encoder, decoder, prior)
- No JUCE DSP classes for primary audio synthesis (only post-processing)
- Audio generation happens in ANIRA background threads (not audio thread)
- Real-time safety via lock-free queues (not standard JUCE audio processing)

**Implications:**
- Cannot use existing plugins as reference (no similar plugin in codebase)
- JUCE DSP patterns (ProcessSpec, AudioBlock, etc.) only apply to post-processing
- Thread safety is critical (audio thread never blocks on inference)
- Latency is structural (4096-sample blocks, cannot reduce without retraining models)

---

### PyTorch Training (Phase 0) is Separate Project

**Phase 0 is NOT part of JUCE codebase:**
- Python/PyTorch environment (not C++)
- Training scripts live in separate repo or folder (e.g., `ml_training/`)
- Only ONNX export files are embedded in JUCE plugin
- JUCE plugin is inference-only (no training at runtime)

**Development workflow:**
1. Train models in Python (Phase 0.1-0.5)
2. Export to ONNX (Phase 0.5)
3. Copy ONNX files to JUCE project (`models/` directory)
4. Build JUCE plugin (Stage 1-4)
5. Test plugin with trained models

**If models need retraining:**
- Go back to Python environment
- Retrain with adjusted architecture or data
- Export new ONNX files
- Rebuild JUCE plugin (no code changes needed)

---

### Thread Safety (CRITICAL)

**Audio thread responsibilities (real-time safe):**
- Read parameters (atomic)
- Construct latent vectors (pure computation, no allocations)
- Submit ANIRA inference requests (non-blocking push to lock-free queue)
- Pop completed audio blocks (non-blocking pop from lock-free queue)
- Crossfade audio blocks (windowing + summing)
- Post-processing (IIR filter, dry/wet mix)

**Audio thread NEVER does:**
- ONNX Runtime inference (runs on ANIRA background threads)
- Model loading (runs on message thread)
- Memory allocation (all buffers pre-allocated in prepareToPlay)
- Mutex locks (all communication via atomics or lock-free queues)

**ANIRA background threads (high priority, not real-time):**
- ONNX Runtime sessions (inference)
- Audio block generation (decoder output)
- Completed blocks pushed to lock-free queue

**Message thread (non-real-time):**
- Model loading (ONNX file I/O, session creation) when SOURCE changes
- Parameter updates from UI (APVTS writes)
- Preset save/restore

**Communication mechanisms:**
- APVTS parameters: Atomic reads (audio thread) / atomic writes (message thread)
- ANIRA queues: SPSC lock-free queues (audio thread ↔ background threads)
- Model swap: Atomic pointer swap when new model ready

---

### Performance Targets

**CPU Usage:**
- Target: <20% single core on modern CPU (M1, Ryzen 5000, Intel 12th gen)
- Generative mode: ~10-15% (Prior + Decoder inference)
- Transform mode: ~15-20% (Encoder + Decoder inference)
- If exceeded: Consider INT8 quantization (reduces inference time ~2-3x)

**Latency:**
- Target: <150ms in Transform mode (host-compensated, acceptable for post-processing)
- Actual: ~85-128ms (4096-sample block + overlap + inference overhead)
- Generate mode: Latency irrelevant (no input to delay)

**Binary Size:**
- Target: <150MB (acceptable for modern plugin distribution)
- Actual: ~80-100MB (6 textures × ~5MB ONNX + ~50MB ONNX Runtime static lib)
- If exceeded: INT8 quantization reduces model size ~75% (~20MB total models)

**Memory:**
- Target: <200MB RAM (plugin + models + buffers)
- Actual: ~100-150MB (ONNX Runtime + models + audio buffers)

---

### Known Challenges

**Challenge 1: VAE Training Quality**
- Risk: VAE may produce muffled/blurry audio (common VAE failure mode)
- Mitigation: Multi-scale spectral loss + adversarial fine-tuning (Phase 0.2.5)
- Fallback: Use pre-trained RAVE models if custom VAE fails

**Challenge 2: ANIRA Integration Complexity**
- Risk: ANIRA is new library (2024), may have bugs or breaking changes
- Mitigation: Test early (Stage 1.2), monitor GitHub for updates
- Fallback: RTNeural (simpler, but no thread pool) if ANIRA fails

**Challenge 3: Latency Perception (Transform Mode)**
- Risk: ~85-128ms latency may feel sluggish for real-time tracking
- Mitigation: Host compensation, target use case is post-processing (not live tracking)
- Fallback: Reduce block size to 2048 samples (halves latency, doubles inference rate)

**Challenge 4: Binary Size**
- Risk: ~80-100MB plugin size may exceed distribution limits
- Mitigation: INT8 quantization reduces size ~75% (~30MB total)
- Fallback: Ship fewer textures (3 instead of 6) if size critical

**Challenge 5: Sample Rate Support (48kHz Only)**
- Risk: Users may request 44.1kHz or 96kHz support
- Mitigation: Add resampler (juce::Resampler) if needed
- Fallback: Train models at multiple sample rates (increases project scope significantly)

---

## References

**Contract files:**
- Creative brief: `plugins/O-Texture/.planning/BRIEF.md`
- Parameter spec: `plugins/O-Texture/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Texture/.planning/research/ARCHITECTURE.md`

**JUCE patterns:**
- Critical patterns: `troubleshooting/patterns/juce8-critical-patterns.md`
- WebView integration: Pattern 8 (resource provider), Pattern 11 (member initialization)
- Parameter binding: Pattern 12 (three-parameter attachment), Pattern 15 (no callback params)

**External resources:**
- ANIRA: https://github.com/anira-project/anira
- ONNX Runtime: https://onnxruntime.ai/docs/get-started/with-cpp.html
- RAVE: https://github.com/acids-ircam/RAVE (reference architecture)

**Similar plugins (NOT in codebase, external research):**
- Neutone (RAVE-based timbre transfer) - XY pad latent control reference
- Output Portal (granular synthesis) - UI/UX reference for texture navigation
- Valhalla Shimmer - Not comparable (reverb-based), but control simplicity reference

---

## Timeline Summary

**Total estimated time:** 9-14 weeks

**Breakdown:**
- **Phase 0: PyTorch Training (3-6 weeks)** - Train VAE + Prior models for 6 textures
- **Stage 1: Foundation (2-3 weeks)** - CMake, ANIRA, parameters, model loading
- **Stage 2: DSP (3-4 weeks)** - Inference pipeline, latent control, crossfading, post-processing
- **Stage 3: GUI (2-3 weeks)** - WebView XY pad, parameter binding, visualization
- **Stage 4: Validation (1-2 weeks)** - Testing, presets, documentation

**Critical path:** Phase 0 (PyTorch training) is longest and must complete before JUCE implementation

**Parallelization opportunities:**
- Train multiple textures in parallel (if multiple GPUs available)
- GUI implementation (Stage 3) can start before Stage 2 complete (basic WebView shell)
- Preset creation (Stage 4.3) can overlap with testing (Stage 4.1-4.2)

**Risk buffer:** Add 2-3 weeks for iteration, troubleshooting, quality tuning (total 11-17 weeks realistic)
