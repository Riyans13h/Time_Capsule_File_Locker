// sender/ui/app.js
class SenderApp {
    constructor() {
        this.serverUrl = 'http://localhost:3000';
        this.currentCapsuleId = null;
        this.init();
    }

    init() {
        this.bindEvents();
        this.loadReceivers();
    }

    bindEvents() {
        // Form submission
        document.getElementById('uploadForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleFileUpload();
        });

        // File input change
        document.getElementById('fileInput').addEventListener('change', (e) => {
            this.updateFileInfo(e.target.files[0]);
        });

        // Clear form
        document.getElementById('clearBtn').addEventListener('click', () => {
            this.clearForm();
        });

        // New capsule button
        document.getElementById('newCapsuleBtn').addEventListener('click', () => {
            this.showUploadForm();
        });

        // Copy capsule ID
        document.getElementById('copyIdBtn').addEventListener('click', () => {
            this.copyCapsuleId();
        });
    }

    async loadReceivers() {
        try {
            const response = await fetch(`${this.serverUrl}/api/publickey`);
            if (response.ok) {
                const data = await response.json();
                this.populateReceivers(data.receivers);
            }
        } catch (error) {
            console.error('Failed to load receivers:', error);
        }
    }

    populateReceivers(receivers) {
        const receiverSelect = document.getElementById('receiverId');
        receiverSelect.innerHTML = '<option value="">Select a receiver...</option>';
        
        receivers.forEach(receiver => {
            const option = document.createElement('option');
            option.value = receiver.receiver_id;
            option.textContent = `${receiver.receiver_id} ${receiver.contact_email ? `(${receiver.contact_email})` : ''}`;
            receiverSelect.appendChild(option);
        });
    }

    updateFileInfo(file) {
        const fileInfo = document.getElementById('fileInfo');
        if (file) {
            const size = this.formatFileSize(file.size);
            fileInfo.innerHTML = `
                <div class="file-details">
                    <strong>Name:</strong> ${file.name}<br>
                    <strong>Size:</strong> ${size}<br>
                    <strong>Type:</strong> ${file.type || 'Unknown'}
                </div>
            `;
        } else {
            fileInfo.innerHTML = '';
        }
    }

    async handleFileUpload() {
        const formData = new FormData();
        const form = document.getElementById('uploadForm');
        
        // Get form values
        const receiverId = form.receiverId.value;
        const fileInput = document.getElementById('fileInput');
        const releaseTime = form.releaseTime.value;
        const password = form.password.value;
        const senderInfo = form.senderInfo.value;

        // Validation
        if (!receiverId || !fileInput.files[0] || !releaseTime) {
            this.showError('Please fill in all required fields');
            return;
        }

        const releaseDate = new Date(releaseTime);
        if (releaseDate <= new Date()) {
            this.showError('Release time must be in the future');
            return;
        }

        try {
            this.showProgress();
            this.updateProgress('Starting encryption process...', 10);

            // Step 1: Get receiver's public key
            this.updateProgress('Fetching receiver public key...', 20);
            const publicKey = await this.getReceiverPublicKey(receiverId);
            
            if (!publicKey) {
                throw new Error('Failed to get receiver public key');
            }

            // Step 2: Prepare files for C++ encryptor
            this.updateProgress('Preparing files for encryption...', 30);
            const file = fileInput.files[0];
            const tempFiles = await this.prepareFilesForEncryption(file, publicKey);

            // Step 3: Run C++ encryptor
            this.updateProgress('Encrypting file...', 50);
            const encryptionResult = await this.runEncryptor({
                receiver_id: receiverId,
                public_key_path: tempFiles.publicKeyPath,
                input_file: tempFiles.inputPath,
                release_time: releaseTime,
                password: password,
                sender_info: senderInfo
            });

            if (!encryptionResult.success) {
                throw new Error(encryptionResult.error || 'Encryption failed');
            }

            // Step 4: Upload to server
            this.updateProgress('Uploading to server...', 80);
            const uploadResult = await this.uploadToServer(encryptionResult, file.name);

            // Step 5: Show success
            this.updateProgress('Complete!', 100);
            setTimeout(() => {
                this.showSuccess(uploadResult);
            }, 1000);

        } catch (error) {
            this.showError(error.message);
            this.hideProgress();
        }
    }

    async getReceiverPublicKey(receiverId) {
        try {
            const response = await fetch(`${this.serverUrl}/api/publickey/${encodeURIComponent(receiverId)}`);
            if (!response.ok) {
                throw new Error('Receiver not found');
            }
            const data = await response.json();
            return data.public_key_pem;
        } catch (error) {
            throw new Error(`Failed to get public key: ${error.message}`);
        }
    }

    async prepareFilesForEncryption(file, publicKey) {
        // In a real implementation, this would:
        // 1. Save the uploaded file to a temporary location
        // 2. Save the public key to a temporary file
        // 3. Return paths for the C++ encryptor
        
        return {
            inputPath: URL.createObjectURL(file),
            publicKeyPath: this.createTempKeyFile(publicKey)
        };
    }

    createTempKeyFile(publicKey) {
        // Create a temporary file with the public key
        const blob = new Blob([publicKey], { type: 'application/x-pem-file' });
        return URL.createObjectURL(blob);
    }

    async runEncryptor(config) {
        // This would interface with the C++ encryptor executable
        // For the web demo, we'll simulate this process
        
        return new Promise((resolve) => {
            setTimeout(() => {
                // Simulate successful encryption
                resolve({
                    success: true,
                    encrypted_file: 'simulated_encrypted_file.bin',
                    key_package: 'simulated_key_package.bin',
                    sha256_hash: 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855'
                });
            }, 2000);
        });
    }

    async uploadToServer(encryptionResult, originalFilename) {
        const formData = new FormData();
        
        // Add encrypted files (in real implementation, these would be actual files)
        formData.append('encrypted_file', new Blob(['encrypted_data']), 'encrypted_file.bin');
        formData.append('encrypted_key_package', new Blob(['key_package']), 'encrypted_key.bin');
        
        // Add form data
        const form = document.getElementById('uploadForm');
        formData.append('receiver_id', form.receiverId.value);
        formData.append('sender_info', form.senderInfo.value || 'Web UI Sender');
        formData.append('original_filename', originalFilename);
        formData.append('release_time', form.releaseTime.value);
        formData.append('sha256_hash', encryptionResult.sha256_hash);
        formData.append('file_size', '1024'); // Simulated size

        const response = await fetch(`${this.serverUrl}/api/upload`, {
            method: 'POST',
            body: formData
        });

        if (!response.ok) {
            const error = await response.json();
            throw new Error(error.details || 'Upload failed');
        }

        return await response.json();
    }

    showProgress() {
        document.getElementById('uploadForm').style.display = 'none';
        document.getElementById('progressSection').style.display = 'block';
        document.getElementById('resultSection').style.display = 'none';
    }

    hideProgress() {
        document.getElementById('progressSection').style.display = 'none';
    }

    updateProgress(message, percent) {
        const progressFill = document.getElementById('progressFill');
        const progressLog = document.getElementById('progressLog');
        
        progressFill.style.width = `${percent}%`;
        progressLog.innerHTML += `<div>${new Date().toLocaleTimeString()}: ${message}</div>`;
        progressLog.scrollTop = progressLog.scrollHeight;
    }

    showSuccess(result) {
        this.currentCapsuleId = result.capsule_id;
        
        document.getElementById('capsuleId').textContent = result.capsule_id;
        document.getElementById('receiverInfo').textContent = result.receiver_id;
        document.getElementById('unlockTime').textContent = result.release_info.scheduled_time;
        document.getElementById('fileName').textContent = result.file_info.original_name;
        document.getElementById('createdTime').textContent = new Date().toLocaleString();
        
        document.getElementById('progressSection').style.display = 'none';
        document.getElementById('resultSection').style.display = 'block';
    }

    showError(message) {
        alert(`Error: ${message}`);
    }

    showUploadForm() {
        document.getElementById('resultSection').style.display = 'none';
        document.getElementById('uploadForm').style.display = 'block';
        this.clearForm();
    }

    clearForm() {
        document.getElementById('uploadForm').reset();
        document.getElementById('fileInfo').innerHTML = '';
    }

    copyCapsuleId() {
        if (this.currentCapsuleId) {
            navigator.clipboard.writeText(this.currentCapsuleId)
                .then(() => {
                    const btn = document.getElementById('copyIdBtn');
                    const originalText = btn.textContent;
                    btn.textContent = '✓ Copied!';
                    setTimeout(() => {
                        btn.textContent = originalText;
                    }, 2000);
                })
                .catch(err => {
                    console.error('Failed to copy:', err);
                });
        }
    }

    formatFileSize(bytes) {
        if (bytes === 0) return '0 Bytes';
        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }
}

// Initialize the application when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    new SenderApp();
});