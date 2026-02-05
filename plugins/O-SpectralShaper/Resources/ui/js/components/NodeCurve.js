/**
 * NodeCurve - Node-based editing mode with Bezier interpolation
 *
 * User double-clicks to add control points, drags to move them.
 * Bezier interpolation between nodes.
 * Delete key removes selected node.
 */

import { CurveEditor } from './CurveEditor.js';

export class NodeCurve extends CurveEditor {
    constructor(canvasId, config = {}) {
        super(canvasId, config);

        // Node-specific state
        this.nodes = []; // { x, y, freq, gain }
        this.selectedNode = null;
        this.isDragging = false;
        this.hitRadius = 10; // pixels

        // Bind event handlers
        this.onMouseDown = this.onMouseDown.bind(this);
        this.onMouseMove = this.onMouseMove.bind(this);
        this.onMouseUp = this.onMouseUp.bind(this);
        this.onDoubleClick = this.onDoubleClick.bind(this);
        this.onKeyDown = this.onKeyDown.bind(this);

        // Attach listeners
        this.canvas.addEventListener('mousedown', this.onMouseDown);
        this.canvas.addEventListener('dblclick', this.onDoubleClick);
        document.addEventListener('keydown', this.onKeyDown);

        // Initialize with default nodes (start and end at 0dB)
        this.initializeDefaultNodes();
    }

    initializeDefaultNodes() {
        const centerY = this.gainToY(0.0);
        this.nodes = [
            { x: this.freqToX(this.minFreq), y: centerY, freq: this.minFreq, gain: 0.0 },
            { x: this.freqToX(this.maxFreq), y: centerY, freq: this.maxFreq, gain: 0.0 }
        ];
        this.interpolateAndSample();
        this.render();
    }

    onDoubleClick(e) {
        e.preventDefault();
        const rect = this.canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;

        // Add new node
        const freq = this.xToFreq(x);
        const gain = this.yToGain(y);
        this.nodes.push({ x, y, freq, gain });

        // Sort by X
        this.nodes.sort((a, b) => a.x - b.x);

        // Update curve
        this.interpolateAndSample();
        this.render();
        this.notifyCurveChange();
    }

    onMouseDown(e) {
        e.preventDefault();
        const rect = this.canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;

        // Check if clicking on a node
        this.selectedNode = this.findNodeAt(x, y);
        if (this.selectedNode !== null) {
            this.isDragging = true;
            document.addEventListener('mousemove', this.onMouseMove);
            document.addEventListener('mouseup', this.onMouseUp);
        }
    }

    onMouseMove(e) {
        if (!this.isDragging || this.selectedNode === null) return;

        const rect = this.canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;

        // Update node position
        const node = this.nodes[this.selectedNode];
        node.x = Math.max(0, Math.min(this.width, x));
        node.y = Math.max(0, Math.min(this.height, y));
        node.freq = this.xToFreq(node.x);
        node.gain = this.yToGain(node.y);

        // Update curve
        this.interpolateAndSample();
        this.render();
        this.notifyCurveChange();
    }

    onMouseUp(e) {
        if (!this.isDragging) return;

        this.isDragging = false;
        document.removeEventListener('mousemove', this.onMouseMove);
        document.removeEventListener('mouseup', this.onMouseUp);
    }

    onKeyDown(e) {
        if (e.key === 'Delete' || e.key === 'Backspace') {
            if (this.selectedNode !== null && this.nodes.length > 2) {
                // Don't delete if it's the only node
                this.nodes.splice(this.selectedNode, 1);
                this.selectedNode = null;

                // Update curve
                this.interpolateAndSample();
                this.render();
                this.notifyCurveChange();
            }
        }
    }

    /**
     * Find node at mouse position
     */
    findNodeAt(x, y) {
        for (let i = 0; i < this.nodes.length; i++) {
            const node = this.nodes[i];
            const dist = Math.sqrt(
                Math.pow(node.x - x, 2) + Math.pow(node.y - y, 2)
            );
            if (dist <= this.hitRadius) {
                return i;
            }
        }
        return null;
    }

    /**
     * Bezier interpolation between nodes
     */
    interpolateAndSample() {
        if (this.nodes.length < 2) return;

        const bandFreqs = this.getBandFrequencies();

        for (let i = 0; i < this.numBands; i++) {
            const freq = bandFreqs[i];

            // Find surrounding nodes
            let leftNode = this.nodes[0];
            let rightNode = this.nodes[this.nodes.length - 1];

            for (let j = 0; j < this.nodes.length - 1; j++) {
                if (freq >= this.nodes[j].freq && freq <= this.nodes[j + 1].freq) {
                    leftNode = this.nodes[j];
                    rightNode = this.nodes[j + 1];
                    break;
                }
            }

            // Linear interpolation (simple Bezier with no control points)
            const t = (freq - leftNode.freq) / (rightNode.freq - leftNode.freq);
            const gain = leftNode.gain + t * (rightNode.gain - leftNode.gain);

            this.curveData[i] = Math.max(-1.0, Math.min(1.0, gain));
        }
    }

    /**
     * Draw the node curve
     */
    drawCurve() {
        // Draw curve line
        const bandFreqs = this.getBandFrequencies();

        this.ctx.strokeStyle = this.accentColor;
        this.ctx.lineWidth = 2;
        this.ctx.beginPath();

        bandFreqs.forEach((freq, i) => {
            const x = this.freqToX(freq);
            const y = this.gainToY(this.curveData[i]);

            if (i === 0) {
                this.ctx.moveTo(x, y);
            } else {
                this.ctx.lineTo(x, y);
            }
        });

        this.ctx.stroke();

        // Draw nodes
        this.ctx.fillStyle = this.accentColor;
        this.ctx.strokeStyle = '#E8E0D4';
        this.ctx.lineWidth = 2;

        this.nodes.forEach((node, i) => {
            this.ctx.beginPath();
            this.ctx.arc(node.x, node.y, 5, 0, Math.PI * 2);
            this.ctx.fill();

            // Highlight selected node
            if (i === this.selectedNode) {
                this.ctx.stroke();
            }
        });
    }

    /**
     * Reset curve to flat, resetting nodes to default positions
     */
    resetCurve() {
        this.selectedNode = null;
        this.initializeDefaultNodes();
        this.notifyCurveChange();
    }

    /**
     * Set curve data from C++ and rebuild nodes
     */
    setCurveData(data) {
        super.setCurveData(data);

        // Rebuild nodes from curve data (simplified: just endpoints)
        const bandFreqs = this.getBandFrequencies();
        this.nodes = [
            {
                x: this.freqToX(bandFreqs[0]),
                y: this.gainToY(data[0]),
                freq: bandFreqs[0],
                gain: data[0]
            },
            {
                x: this.freqToX(bandFreqs[this.numBands - 1]),
                y: this.gainToY(data[this.numBands - 1]),
                freq: bandFreqs[this.numBands - 1],
                gain: data[this.numBands - 1]
            }
        ];

        this.render();
    }
}
