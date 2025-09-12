// Time Capsule File Locker Web Application
class TimeCapsuleApp {
    constructor() {
        this.wasmModule = null;
        this.currentCapsule = null;
        this.currentFile = null;
        
        this.initializeApp();
    }

    // Initialize the application
    async initializeApp() {
        try {
            // Initialize WebAssembly module
            this.wasmModule = await window.wasmLoader.initialize();
            
            // Set up event listeners
            this.setupEventListeners();
            
            console.log('Time Capsule App initialized successfully');
        } catch (error) {
            console.error('Failed to initialize app:', error);
            this.showError('Failed to initialize application. Please refresh the page and try again.');
        }
    }

    // Set up event listeners
    setupEventListeners() {
        // Tab switching
        document.querySelectorAll('.tab-button').forEach(button => {
            button.addEventListener('click', (e) => {
                const tab = e.target.dataset.tab;
                this.switchTab(tab);
            });
        });

        // Lock form submission
        document.getElementById('lockForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleLock();
        });

        // Unlock form submission
        document.getElementById('unlockForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleUnlock();
        });

        // Download buttons
        document.getElementById('downloadCapsule').addEventListener('click', () => {
            this.downloadCapsule();
        });

        document.getElementById('downloadOriginal').addEventListener('click', () => {
            this.downloadOriginalFile();
        });

        // Password confirmation validation
        document.getElementById('confirmPassword').addEventListener('input', (e) => {
            this.validatePasswordConfirmation();
        });
    }

    // Switch between tabs
    switchTab(tabName) {
        // Update tab buttons
        document.querySelectorAll('.tab-button').forEach(button => {
            button.classList.toggle('active', button.dataset.tab === tabName);
        });

        // Update tab content
        document.querySelectorAll('.tab-content').forEach(content => {
            content.classList.toggle('active', content.id === `${tabName}-tab`);
        });

        // Clear previous results
        this.clearResults();
    }

    // Handle file locking
    async handleLock() {
        try {
            // Validate form
            if (!this.validateLockForm()) {
                return;
            }

            // Get form data
            const fileInput = document.getElementById('fileInput');
            const password = document.getElementById('password').value;
            const unlockDate = document.getElementById('unlockDate').value;
            const unlockTime = document.getElementById('unlockTime').value;
            
            // Combine date and time
            const unlockDateTime = `${unlockDate}T${unlockTime}:00Z`;
            
            // Show progress
            this.showProgress('lock', true);
            
            // Read the file
            const file = fileInput.files[0];
            const fileData = await this.readFileAsArrayBuffer(file);
            
            // Create time capsule
            const result = await this.createTimeCapsule(fileData, file.name, password, unlockDateTime);
            
            // Store the result
            this.currentCapsule = result;
            
            // Show success
            this.showLockResult(result);
            
        } catch (error) {
            console.error('Error creating time capsule:', error);
            this.showError('Failed to create time capsule: ' + error.message);
            this.showProgress('lock', false);
        }
    }

    // Handle file unlocking
    async handleUnlock() {
        try {
            // Validate form
            if (!this.validateUnlockForm()) {
                return;
            }

            // Get form data
            const fileInput = document.getElementById('capsuleFile');
            const password = document.getElementById('unlockPassword').value;
            
            // Show progress
            this.showProgress('unlock', true);
            
            // Read the capsule file
            const file = fileInput.files[0];
            const capsuleData = await this.readFileAsArrayBuffer(file);
            
            // Extract time capsule
            const result = await this.extractTimeCapsule(capsuleData, password);
            
            // Store the result
            this.currentFile = result;
            
            // Show success
            this.showUnlockResult(result);
            
        } catch (error) {
            console.error('Error unlocking time capsule:', error);
            this.showUnlockError(error.message);
            this.showProgress('unlock', false);
        }
    }

    // Validate lock form
    validateLockForm() {
        const fileInput = document.getElementById('fileInput');
        const password = document.getElementById('password').value;
        const confirmPassword = document.getElementById('confirmPassword').value;
        const unlockDate = document.getElementById('unlockDate').value;
        const unlockTime = document.getElementById('unlockTime').value;

        // Check if file is selected
        if (!fileInput.files || fileInput.files.length === 0) {
            this.showError('Please select a file to encrypt');
            return false;
        }

        // Check password strength
        if (password.length < 8) {
            this.showError('Password must be at least 8 characters long');
            return false;
        }

        // Check password confirmation
        if (password !== confirmPassword) {
            this.showError('Passwords do not match');
            return false;
        }

        // Check unlock date/time
        if (!unlockDate || !unlockTime) {
            this.showError('Please select an unlock date and time');
            return false;
        }

        // Check if unlock time is in the future
        const unlockDateTime = new Date(`${unlockDate}T${unlockTime}`);
        if (unlockDateTime <= new Date()) {
            this.showError('Unlock time must be in the future');
            return false;
        }

        return true;
    }

    // Validate unlock form
    validateUnlockForm() {
        const fileInput = document.getElementById('capsuleFile');
        const password = document.getElementById('unlockPassword').value;

        // Check if file is selected
        if (!fileInput.files || fileInput.files.length === 0) {
            this.showError('Please select a time capsule file');
            return false;
        }

        // Check password
        if (!password) {
            this.showError('Please enter the password');
            return false;
        }

        return true;
    }

    // Validate password confirmation
    validatePasswordConfirmation() {
        const password = document.getElementById('password').value;
        const confirmPassword = document.getElementById('confirmPassword').value;
        const confirmField = document.getElementById('confirmPassword');

        if (password && confirmPassword) {
            if (password === confirmPassword) {
                confirmField.style.borderColor = '#2ecc71';
            } else {
                confirmField.style.borderColor = '#e74c3c';
            }
        } else {
            confirmField.style.borderColor = '#ddd';
        }
    }

    // Read file as ArrayBuffer
    readFileAsArrayBuffer(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = (e) => resolve(new Uint8Array(e.target.result));
            reader.onerror = (e) => reject(new Error('Failed to read file'));
            reader.readAsArrayBuffer(file);
        });
    }

    // Create time capsule using WebAssembly
    async createTimeCapsule(fileData, filename, password, unlockTime) {
        // Convert data to pointers for WASM
        const dataPtr = window.wasmLoader.arrayToPointer(fileData);
        const filenamePtr = window.wasmLoader.stringToCString(filename);
        const passwordPtr = window.wasmLoader.stringToCString(password);
        const unlockTimePtr = window.wasmLoader.stringToCString(unlockTime);
        
        // Prepare result pointers
        const resultPtrPtr = this.wasmModule.instance.exports._malloc(4);
        const resultSizePtr = this.wasmModule.instance.exports._malloc(4);
        
        try {
            // Call WASM function
            const success = this.wasmModule.instance.exports._wasm_create_time_capsule(
                dataPtr, fileData.length, passwordPtr, unlockTimePtr, 
                filenamePtr, resultPtrPtr, resultSizePtr
            );
            
            if (!success) {
                throw new Error('Failed to create time capsule');
            }
            
            // Get result data
            const resultPtr = new Uint32Array(
                this.wasmModule.instance.exports.memory.buffer,
                resultPtrPtr,
                1
            )[0];
            
            const resultSize = new Uint32Array(
                this.wasmModule.instance.exports.memory.buffer,
                resultSizePtr,
                1
            )[0];
            
            const resultData = window.wasmLoader.pointerToArray(resultPtr, resultSize);
            
            // Convert to base64 for storage
            const base64Data = this.arrayToBase64(resultData);
            
            return {
                data: base64Data,
                filename: filename,
                unlockTime: unlockTime,
                size: resultSize
            };
            
        } finally {
            // Clean up memory
            window.wasmLoader.freeMemory(dataPtr);
            window.wasmLoader.freeMemory(filenamePtr);
            window.wasmLoader.freeMemory(passwordPtr);
            window.wasmLoader.freeMemory(unlockTimePtr);
            window.wasmLoader.freeMemory(resultPtrPtr);
            window.wasmLoader.freeMemory(resultSizePtr);
        }
    }

    // Extract time capsule using WebAssembly
    async extractTimeCapsule(capsuleData, password) {
        // Convert data to pointers for WASM
        const dataPtr = window.wasmLoader.arrayToPointer(capsuleData);
        const passwordPtr = window.wasmLoader.stringToCString(password);
        
        // Prepare result pointers
        const resultPtrPtr = this.wasmModule.instance.exports._malloc(4);
        const resultSizePtr = this.wasmModule.instance.exports._malloc(4);
        const metadataPtrPtr = this.wasmModule.instance.exports._malloc(4);
        
        try {
            // Call WASM function
            const success = this.wasmModule.instance.exports._wasm_extract_time_capsule(
                dataPtr, capsuleData.length, passwordPtr, 
                resultPtrPtr, resultSizePtr, metadataPtrPtr
            );
            
            if (!success) {
                throw new Error('Failed to extract time capsule. Invalid password or corrupted file.');
            }
            
            // Get result data
            const resultPtr = new Uint32Array(
                this.wasmModule.instance.exports.memory.buffer,
                resultPtrPtr,
                1
            )[0];
            
            const resultSize = new Uint32Array(
                this.wasmModule.instance.exports.memory.buffer,
                resultSizePtr,
                1
            )[0];
            
            const resultData = window.wasmLoader.pointerToArray(resultPtr, resultSize);
            
            // Get metadata
            const metadataPtr = new Uint32Array(
                this.wasmModule.instance.exports.memory.buffer,
                metadataPtrPtr,
                1
            )[0];
            
            const metadataJson = window.wasmLoader.cStringToString(metadataPtr);
            const metadata = JSON.parse(metadataJson);
            
            return {
                data: resultData,
                metadata: metadata
            };
            
        } finally {
            // Clean up memory
            window.wasmLoader.freeMemory(dataPtr);
            window.wasmLoader.freeMemory(passwordPtr);
            window.wasmLoader.freeMemory(resultPtrPtr);
            window.wasmLoader.freeMemory(resultSizePtr);
            window.wasmLoader.freeMemory(metadataPtrPtr);
        }
    }

    // Show progress indicator
    showProgress(type, show) {
        const progress = document.getElementById(`${type}Progress`);
        const button = document.getElementById(`${type}Button`);
        
        if (show) {
            progress.classList.remove('hidden');
            button.disabled = true;
            
            // Animate progress bar
            const progressBar = progress.querySelector('.progress');
            let width = 0;
            const interval = setInterval(() => {
                if (width >= 100) {
                    clearInterval(interval);
                } else {
                    width += 5;
                    progressBar.style.width = width + '%';
                }
            }, 100);
        } else {
            progress.classList.add('hidden');
            button.disabled = false;
        }
    }

    // Show lock result
    showLockResult(result) {
        document.getElementById('capsuleId').textContent = this.generateCapsuleId();
        document.getElementById('capsuleUnlockTime').textContent = new Date(result.unlockTime).toLocaleString();
        document.getElementById('lockResult').classList.remove('hidden');
    }

    // Show unlock result
    showUnlockResult(result) {
        document.getElementById('originalFileName').textContent = result.metadata.originalFilename;
        document.getElementById('originalFileSize').textContent = this.formatFileSize(result.metadata.originalSize);
        document.getElementById('unlockResult').classList.remove('hidden');
    }

    // Show unlock error
    showUnlockError(message) {
        document.getElementById('errorMessage').textContent = message;
        document.getElementById('unlockError').classList.remove('hidden');
    }

    // Show general error
    showError(message) {
        alert('Error: ' + message);
    }

    // Clear results
    clearResults() {
        document.getElementById('lockResult').classList.add('hidden');
        document.getElementById('unlockResult').classList.add('hidden');
        document.getElementById('unlockError').classList.add('hidden');
        this.currentCapsule = null;
        this.currentFile = null;
    }

    // Download time capsule file
    downloadCapsule() {
        if (!this.currentCapsule) {
            this.showError('No time capsule to download');
            return;
        }
        
        const blob = this.base64ToBlob(this.currentCapsule.data, 'application/octet-stream');
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        
        a.href = url;
        a.download = `${this.currentCapsule.filename}.tcf`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    // Download original file
    downloadOriginalFile() {
        if (!this.currentFile) {
            this.showError('No file to download');
            return;
        }
        
        const blob = new Blob([this.currentFile.data], { type: 'application/octet-stream' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        
        a.href = url;
        a.download = this.currentFile.metadata.originalFilename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    // Generate a random capsule ID
    generateCapsuleId() {
        return Math.random().toString(36).substring(2, 10) + 
               Math.random().toString(36).substring(2, 10);
    }

    // Format file size
    formatFileSize(bytes) {
        if (bytes < 1024) return bytes + ' bytes';
        if (bytes < 1048576) return (bytes / 1024).toFixed(2) + ' KB';
        return (bytes / 1048576).toFixed(2) + ' MB';
    }

    // Convert ArrayBuffer to base64
    arrayToBase64(buffer) {
        let binary = '';
        const bytes = new Uint8Array(buffer);
        const len = bytes.byteLength;
        
        for (let i = 0; i < len; i++) {
            binary += String.fromCharCode(bytes[i]);
        }
        
        return window.btoa(binary);
    }

    // Convert base64 to Blob
    base64ToBlob(base64, type) {
        const binary = window.atob(base64);
        const array = new Uint8Array(binary.length);
        
        for (let i = 0; i < binary.length; i++) {
            array[i] = binary.charCodeAt(i);
        }
        
        return new Blob([array], { type: type });
    }
}

// Initialize the application when the page loads
document.addEventListener('DOMContentLoaded', () => {
    window.timeCapsuleApp = new TimeCapsuleApp();
});