/**
 * Spectrogram - WebGL Real-Time Spectrogram Renderer
 *
 * Phase 3.3: WebGL spectrogram with transient heat overlay
 * - Circular buffer texture (512 columns × 257 bins)
 * - Inferno colormap for spectrogram
 * - Heat overlay for transient activity
 * - Logarithmic frequency axis
 */

export class Spectrogram {
    constructor(canvasId, options = {}) {
        this.canvas = document.getElementById(canvasId);
        if (!this.canvas) {
            console.error(`Canvas not found: ${canvasId}`);
            return;
        }

        this.options = {
            width: options.width || 512,
            height: options.height || 257,
            heatIntensity: options.heatIntensity || 0.5,
            ...options
        };

        this.writeOffset = 0;
        this.isReady = false;

        // Size the canvas to fit its container
        this.resizeCanvas();

        // Initialize WebGL after sizing
        requestAnimationFrame(() => {
            this.resizeCanvas();
            this.initWebGL();
        });

        // Handle window resize
        window.addEventListener('resize', () => {
            this.resizeCanvas();
            if (this.gl) {
                this.gl.viewport(0, 0, this.canvas.width, this.canvas.height);
            }
        });
    }

    resizeCanvas() {
        const rect = this.canvas.getBoundingClientRect();
        const width = rect.width > 0 ? rect.width : 400;
        const height = rect.height > 0 ? rect.height : 150;

        const dpr = window.devicePixelRatio || 1;
        this.canvas.width = width * dpr;
        this.canvas.height = height * dpr;

        console.log(`Spectrogram canvas sized: ${width}x${height} (buffer: ${this.canvas.width}x${this.canvas.height})`);
    }

    // ============================================================================
    // WEBGL INITIALIZATION
    // ============================================================================

    initWebGL() {
        // Try WebGL2 first, fallback to WebGL1
        this.gl = this.canvas.getContext('webgl2') || this.canvas.getContext('webgl');

        if (!this.gl) {
            console.error('WebGL not supported');
            this.fallbackToCanvas2D();
            return;
        }

        console.log('WebGL initialized:', this.gl instanceof WebGL2RenderingContext ? 'WebGL2' : 'WebGL1');

        // Setup context loss handling
        this.canvas.addEventListener('webglcontextlost', this.handleContextLost.bind(this), false);
        this.canvas.addEventListener('webglcontextrestored', this.handleContextRestored.bind(this), false);

        // Create shaders and program
        this.createShaderProgram();

        // Create textures
        this.createTextures();

        // Create geometry (fullscreen quad)
        this.createGeometry();

        // Setup uniforms
        this.setupUniforms();

        this.isReady = true;
    }

    // ============================================================================
    // SHADER COMPILATION
    // ============================================================================

    createShaderProgram() {
        const gl = this.gl;

        // Vertex shader (fullscreen quad)
        const vertexShaderSource = `
            attribute vec2 a_position;
            varying vec2 v_texCoord;

            void main() {
                gl_Position = vec4(a_position, 0.0, 1.0);
                v_texCoord = a_position * 0.5 + 0.5;
            }
        `;

        // Fragment shader (inferno colormap + heat overlay)
        const fragmentShaderSource = `
            precision highp float;

            varying vec2 v_texCoord;

            uniform sampler2D u_fftTexture;
            uniform sampler2D u_transientTexture;
            uniform float u_writeOffset;
            uniform float u_heatIntensity;

            // Inferno colormap (simplified perceptually uniform)
            vec3 inferno(float t) {
                t = clamp(t, 0.0, 1.0);
                vec3 c0 = vec3(0.0, 0.0, 0.0);
                vec3 c1 = vec3(0.3, 0.0, 0.5);
                vec3 c2 = vec3(0.9, 0.3, 0.0);
                vec3 c3 = vec3(1.0, 0.8, 0.2);

                if (t < 0.33) {
                    return mix(c0, c1, t / 0.33);
                } else if (t < 0.67) {
                    return mix(c1, c2, (t - 0.33) / 0.34);
                } else {
                    return mix(c2, c3, (t - 0.67) / 0.33);
                }
            }

            // Heat colormap (black → red → orange → yellow)
            vec3 heat(float t) {
                t = clamp(t, 0.0, 1.0);
                vec3 black = vec3(0.0, 0.0, 0.0);
                vec3 red = vec3(1.0, 0.0, 0.0);
                vec3 orange = vec3(1.0, 0.5, 0.0);
                vec3 yellow = vec3(1.0, 1.0, 0.5);

                if (t < 0.33) {
                    return mix(black, red, t / 0.33);
                } else if (t < 0.67) {
                    return mix(red, orange, (t - 0.33) / 0.34);
                } else {
                    return mix(orange, yellow, (t - 0.67) / 0.33);
                }
            }

            void main() {
                // Circular buffer lookup (wrap X coordinate)
                float x = mod(v_texCoord.x + u_writeOffset, 1.0);

                // Logarithmic Y mapping (flip so low frequencies at bottom)
                float y = 1.0 - v_texCoord.y;

                // Sample FFT magnitude
                float magnitude = texture2D(u_fftTexture, vec2(x, y)).r;

                // Convert to dB scale (0-60dB range)
                float db = 20.0 * log(max(magnitude, 0.0001)) / log(10.0);
                float normalized = clamp((db + 60.0) / 60.0, 0.0, 1.0);

                // Apply inferno colormap
                vec3 color = inferno(normalized);

                // Sample transient activity (interpolate 32 bands to 257 bins)
                float transient = texture2D(u_transientTexture, vec2(x, y)).r;

                // Apply heat overlay (additive blend)
                vec3 heatColor = heat(transient) * u_heatIntensity;
                color += heatColor;

                gl_FragColor = vec4(color, 1.0);
            }
        `;

        const vertexShader = this.compileShader(gl.VERTEX_SHADER, vertexShaderSource);
        const fragmentShader = this.compileShader(gl.FRAGMENT_SHADER, fragmentShaderSource);

        this.program = gl.createProgram();
        gl.attachShader(this.program, vertexShader);
        gl.attachShader(this.program, fragmentShader);
        gl.linkProgram(this.program);

        if (!gl.getProgramParameter(this.program, gl.LINK_STATUS)) {
            console.error('Shader program link failed:', gl.getProgramInfoLog(this.program));
            return;
        }

        gl.useProgram(this.program);
    }

    compileShader(type, source) {
        const gl = this.gl;
        const shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);

        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            console.error('Shader compile failed:', gl.getShaderInfoLog(shader));
            gl.deleteShader(shader);
            return null;
        }

        return shader;
    }

    // ============================================================================
    // TEXTURE CREATION
    // ============================================================================

    createTextures() {
        const gl = this.gl;
        const width = this.options.width;
        const height = this.options.height;

        // FFT magnitude texture (512×257, R32F or RGBA8)
        this.fftTexture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.fftTexture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

        // Use RGBA8 for compatibility (pack float into RGBA)
        const emptyData = new Uint8Array(width * height * 4);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, emptyData);

        // Transient activity texture (512×257, interpolated from 32 bands)
        this.transientTexture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.transientTexture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, emptyData);
    }

    // ============================================================================
    // GEOMETRY SETUP
    // ============================================================================

    createGeometry() {
        const gl = this.gl;

        // Fullscreen quad
        const vertices = new Float32Array([
            -1, -1,
             1, -1,
            -1,  1,
             1,  1
        ]);

        this.vertexBuffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);

        const positionLocation = gl.getAttribLocation(this.program, 'a_position');
        gl.enableVertexAttribArray(positionLocation);
        gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, 0);
    }

    // ============================================================================
    // UNIFORM SETUP
    // ============================================================================

    setupUniforms() {
        const gl = this.gl;

        this.uniforms = {
            fftTexture: gl.getUniformLocation(this.program, 'u_fftTexture'),
            transientTexture: gl.getUniformLocation(this.program, 'u_transientTexture'),
            writeOffset: gl.getUniformLocation(this.program, 'u_writeOffset'),
            heatIntensity: gl.getUniformLocation(this.program, 'u_heatIntensity')
        };

        gl.uniform1i(this.uniforms.fftTexture, 0);
        gl.uniform1i(this.uniforms.transientTexture, 1);
        gl.uniform1f(this.uniforms.heatIntensity, this.options.heatIntensity);
    }

    // ============================================================================
    // FRAME UPDATE
    // ============================================================================

    addFrame(fftData, transientData) {
        if (!this.isReady || !this.gl) return;

        const gl = this.gl;
        const width = this.options.width;
        const height = this.options.height;

        // Update single column in circular buffer
        const columnIndex = this.writeOffset;

        // Convert float arrays to RGBA8 (pack float into 4 bytes)
        const fftColumn = new Uint8Array(height * 4);
        for (let i = 0; i < height; i++) {
            const value = Math.min(Math.max(fftData[i] || 0, 0), 1);
            const packed = Math.floor(value * 255);
            fftColumn[i * 4 + 0] = packed;
            fftColumn[i * 4 + 1] = packed;
            fftColumn[i * 4 + 2] = packed;
            fftColumn[i * 4 + 3] = 255;
        }

        // Update FFT texture column
        gl.bindTexture(gl.TEXTURE_2D, this.fftTexture);
        gl.texSubImage2D(gl.TEXTURE_2D, 0, columnIndex, 0, 1, height, gl.RGBA, gl.UNSIGNED_BYTE, fftColumn);

        // Interpolate 32-band transient data to 257 bins
        const transientColumn = new Uint8Array(height * 4);
        for (let i = 0; i < height; i++) {
            // Map bin index to band (logarithmic spacing)
            const bandIndex = Math.floor((i / height) * 32);
            const value = Math.min(Math.max(transientData[bandIndex] || 0, 0), 1);
            const packed = Math.floor(value * 255);
            transientColumn[i * 4 + 0] = packed;
            transientColumn[i * 4 + 1] = packed;
            transientColumn[i * 4 + 2] = packed;
            transientColumn[i * 4 + 3] = 255;
        }

        // Update transient texture column
        gl.bindTexture(gl.TEXTURE_2D, this.transientTexture);
        gl.texSubImage2D(gl.TEXTURE_2D, 0, columnIndex, 0, 1, height, gl.RGBA, gl.UNSIGNED_BYTE, transientColumn);

        // Advance write offset (circular buffer)
        this.writeOffset = (this.writeOffset + 1) % width;
    }

    // ============================================================================
    // RENDER
    // ============================================================================

    draw() {
        if (!this.isReady || !this.gl) return;

        const gl = this.gl;

        // Set viewport
        gl.viewport(0, 0, this.canvas.width, this.canvas.height);

        // Clear
        gl.clearColor(0, 0, 0, 1);
        gl.clear(gl.COLOR_BUFFER_BIT);

        // Bind textures
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, this.fftTexture);

        gl.activeTexture(gl.TEXTURE1);
        gl.bindTexture(gl.TEXTURE_2D, this.transientTexture);

        // Update writeOffset uniform (for circular buffer wrapping)
        gl.uniform1f(this.uniforms.writeOffset, this.writeOffset / this.options.width);

        // Draw fullscreen quad
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    }

    // ============================================================================
    // CONTEXT LOSS HANDLING
    // ============================================================================

    handleContextLost(event) {
        event.preventDefault();
        console.warn('WebGL context lost');
        this.isReady = false;
    }

    handleContextRestored() {
        console.log('WebGL context restored');
        this.initWebGL();
    }

    // ============================================================================
    // FALLBACK (Canvas 2D)
    // ============================================================================

    fallbackToCanvas2D() {
        console.warn('Falling back to Canvas 2D (WebGL not supported)');
        this.ctx = this.canvas.getContext('2d');
        this.isReady = false;

        // TODO: Implement Canvas 2D fallback if needed
        // For now, just display a message
        this.ctx.fillStyle = '#1A1A1A';
        this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
        this.ctx.fillStyle = '#E8E0D4';
        this.ctx.font = '14px Georgia';
        this.ctx.textAlign = 'center';
        this.ctx.fillText('WebGL not supported', this.canvas.width / 2, this.canvas.height / 2);
    }

    // ============================================================================
    // CLEANUP
    // ============================================================================

    destroy() {
        if (this.gl) {
            const gl = this.gl;
            gl.deleteTexture(this.fftTexture);
            gl.deleteTexture(this.transientTexture);
            gl.deleteBuffer(this.vertexBuffer);
            gl.deleteProgram(this.program);
        }

        this.canvas.removeEventListener('webglcontextlost', this.handleContextLost);
        this.canvas.removeEventListener('webglcontextrestored', this.handleContextRestored);

        this.isReady = false;
    }
}
