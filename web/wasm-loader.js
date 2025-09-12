// WebAssembly module loader for Time Capsule Core
class WasmLoader {
    constructor() {
        this._module = null;
        this._isInitialized = false;
    }

    // Initialize the WebAssembly module
    async initialize() {
        if (this._isInitialized) {
            return this._module;
        }

        try {
            // Check if WebAssembly is supported
            if (!('WebAssembly' in window)) {
                throw new Error('WebAssembly is not supported in this browser');
            }

            // Load the WebAssembly module
            const importObject = {
                env: {
                    emscripten_notify_memory_growth: function() {},
                    // Add any other required imports here
                }
            };

            // Load the module
            const response = await fetch('timecapsule_core.wasm');
            const buffer = await response.arrayBuffer();
            const module = await WebAssembly.compile(buffer);
            
            // Instantiate the module
            this._module = await WebAssembly.instantiate(module, importObject);
            this._isInitialized = true;

            console.log('WebAssembly module loaded successfully');
            return this._module;
        } catch (error) {
            console.error('Failed to load WebAssembly module:', error);
            throw error;
        }
    }

    // Get the module instance
    getModule() {
        if (!this._isInitialized) {
            throw new Error('WebAssembly module not initialized');
        }
        return this._module;
    }

    // Call a function from the WebAssembly module
    callFunction(name, ...args) {
        const instance = this.getModule().instance;
        const func = instance.exports[name];
        
        if (typeof func !== 'function') {
            throw new Error(`Function ${name} not found in WebAssembly module`);
        }
        
        return func(...args);
    }

    // Allocate memory in the WebAssembly module
    allocateMemory(size) {
        return this.callFunction('_malloc', size);
    }

    // Free memory in the WebAssembly module
    freeMemory(pointer) {
        this.callFunction('_free', pointer);
    }

    // Convert JavaScript string to C-style string (allocated in WASM memory)
    stringToCString(str) {
        const encoder = new TextEncoder();
        const bytes = encoder.encode(str);
        const ptr = this.allocateMemory(bytes.length + 1);
        
        const memory = new Uint8Array(this.getModule().instance.exports.memory.buffer);
        for (let i = 0; i < bytes.length; i++) {
            memory[ptr + i] = bytes[i];
        }
        memory[ptr + bytes.length] = 0; // Null terminator
        
        return ptr;
    }

    // Convert C-style string to JavaScript string
    cStringToString(ptr) {
        const memory = new Uint8Array(this.getModule().instance.exports.memory.buffer);
        let length = 0;
        
        while (memory[ptr + length] !== 0) {
            length++;
        }
        
        const bytes = new Uint8Array(memory.buffer, ptr, length);
        const decoder = new TextDecoder();
        return decoder.decode(bytes);
    }

    // Convert JavaScript Uint8Array to WASM pointer
    arrayToPointer(array) {
        const ptr = this.allocateMemory(array.length);
        const memory = new Uint8Array(this.getModule().instance.exports.memory.buffer);
        
        for (let i = 0; i < array.length; i++) {
            memory[ptr + i] = array[i];
        }
        
        return ptr;
    }

    // Convert WASM pointer to JavaScript Uint8Array
    pointerToArray(ptr, length) {
        const memory = new Uint8Array(this.getModule().instance.exports.memory.buffer);
        return new Uint8Array(memory.buffer, ptr, length);
    }
}

// Create global instance
window.wasmLoader = new WasmLoader();