// receiver/ui/app.js
class ReceiverApp {
    constructor() {
        this.serverUrl = 'http://localhost:3000';
        this.currentReceiverId = null;
        this.init();
    }

    init() {
        this.bindEvents();
        this.loadStoredReceiverId();
    }

    bindEvents() {
        // Public key registration form
        document.getElementById('registerForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleKeyRegistration();
        });

        // Check capsule status form
        document.getElementById('checkForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.checkCapsuleStatus();
        });

        // Download capsule form
        document.getElementById('downloadForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleDownload();
        });

        // Refresh available capsules
        document.getElementById('refreshBtn').addEventListener('click', () => {
            this.loadAvailableCapsules();
        });

        // Private key file selection
        document.getElementById('privateKeyFile').addEventListener('change', (e) => {
            this.validatePrivateKey(e.target.files[0]);
        });
    }

    loadStoredReceiverId() {
        const storedId = localStorage.getItem('timecapsule_receiver_id');
        if (storedId) {
            this.currentReceiverId = storedId;
            document.getElementById('currentReceiverId').textContent = storedId;
            document.getElementById('receiverDashboard').style.display = 'block';
            this.loadAvailableCapsules();
        }
    }

    async handleKeyRegistration() {
        const form = document.getElementById('registerForm');
        const receiverId = form.receiverId.value;
        const publicKeyFile = document.getElementById('pubkey').files[0];
        const contactEmail = form.contact_email.value;

        if (!receiverId || !publicKeyFile) {
            this.showOutput('Error: Please provide receiver ID and public key file', 'error');
            return;
        }

        try {
            this.showOutput('Registering public key...', 'info');

            const formData = new FormData();
            formData.append('receiver_id', receiverId);
            formData.append('public_key', publicKeyFile);
            if (contactEmail) {
                formData.append('contact_email', contactEmail);
            }

            const response = await fetch(`${this.serverUrl}/api/publickey/register`, {
                method: 'POST',
                body: formData
            });

            const result = await response.json();

            if (response.ok) {
                this.showOutput('✅ Public key registered successfully!', 'success');
                this.currentReceiverId = receiverId;
                localStorage.setItem('timecapsule_receiver_id', receiverId);
                
                // Show receiver dashboard
                document.getElementById('currentReceiverId').textContent = receiverId;
                document.getElementById('receiverDashboard').style.display = 'block';
                document.getElementById('keyRegistration').style.display = 'none';
                
                this.loadAvailableCapsules();
            } else {
                this.showOutput(`Error: ${result.error} - ${result.details}`, 'error');
            }

        } catch (error) {
            this.showOutput(`Error: ${error.message}`, 'error');
        }
    }

    async checkCapsuleStatus() {
        const capsuleId = document.getElementById('capsuleId').value.trim();
        
        if (!capsuleId) {
            this.showOutput('Please enter a capsule ID', 'error');
            return;
        }

        try {
            this.showOutput('Checking capsule status...', 'info');

            const response = await fetch(`${this.serverUrl}/api/capsule/status/${capsuleId}`);
            const result = await response.json();

            if (response.ok) {
                this.displayCapsuleInfo(result.capsule);
            } else {
                this.showOutput(`Error: ${result.error} - ${result.details}`, 'error');
            }

        } catch (error) {
            this.showOutput(`Error: ${error.message}`, 'error');
        }
    }

    displayCapsuleInfo(capsule) {
        const output = document.getElementById('out');
        const statusClass = capsule.status === 'delivered' ? 'status-delivered' : 'status-pending';
        
        output.innerHTML = `
            <div class="capsule-info">
                <h3>Capsule Information</h3>
                <div class="info-grid">
                    <div class="info-item">
                        <label>Capsule ID:</label>
                        <span>${capsule.capsule_id}</span>
                    </div>
                    <div class="info-item">
                        <label>Status:</label>
                        <span class="status ${statusClass}">${capsule.status}</span>
                    </div>
                    <div class="info-item">
                        <label>Sender:</label>
                        <span>${capsule.sender_info || 'Unknown'}</span>
                    </div>
                    <div class="info-item">
                        <label>Original File:</label>
                        <span>${capsule.original_filename}</span>
                    </div>
                    <div class="info-item">
                        <label>File Size:</label>
                        <span>${this.formatFileSize(capsule.file_size)}</span>
                    </div>
                    <div class="info-item">
                        <label>Release Time:</label>
                        <span>${new Date(capsule.release_time).toLocaleString()}</span>
                    </div>
                    <div class="info-item">
                        <label>Created:</label>
                        <span>${new Date(capsule.created_at).toLocaleString()}</span>
                    </div>
                    ${capsule.delivered_at ? `
                    <div class="info-item">
                        <label>Delivered:</label>
                        <span>${new Date(capsule.delivered_at).toLocaleString()}</span>
                    </div>
                    ` : ''}
                </div>
                
                ${capsule.status === 'delivered' ? `
                <div class="action-buttons">
                    <button onclick="receiverApp.prepareDownload('${capsule.capsule_id}')" 
                            class="btn-download">
                        📥 Download & Decrypt
                    </button>
                </div>
                ` : `
                <div class="waiting-message">
                    ⏳ This capsule will be available on ${new Date(capsule.release_time).toLocaleString()}
                </div>
                `}
            </div>
        `;
    }

    async loadAvailableCapsules() {
        if (!this.currentReceiverId) return;

        try {
            const response = await fetch(`${this.serverUrl}/api/release/available/${this.currentReceiverId}`);
            const result = await response.json();

            if (response.ok) {
                this.displayAvailableCapsules(result.capsules);
            } else {
                this.showOutput(`Error loading capsules: ${result.error}`, 'error');
            }

        } catch (error) {
            this.showOutput(`Error: ${error.message}`, 'error');
        }
    }

    displayAvailableCapsules(capsules) {
        const container = document.getElementById('availableCapsules');
        
        if (capsules.length === 0) {
            container.innerHTML = '<div class="no-capsules">No capsules available for download</div>';
            return;
        }

        container.innerHTML = `
            <h3>Available Capsules (${capsules.length})</h3>
            <div class="capsules-list">
                ${capsules.map(capsule => `
                    <div class="capsule-item">
                        <div class="capsule-header">
                            <strong>${capsule.original_filename}</strong>
                            <span class="file-size">${this.formatFileSize(capsule.file_size)}</span>
                        </div>
                        <div class="capsule-details">
                            <div>From: ${capsule.sender_info || 'Unknown'}</div>
                            <div>Released: ${new Date(capsule.delivered_at).toLocaleString()}</div>
                        </div>
                        <div class="capsule-actions">
                            <button onclick="receiverApp.prepareDownload('${capsule.capsule_id}')" 
                                    class="btn-download-small">
                                Download & Decrypt
                            </button>
                        </div>
                    </div>
                `).join('')}
            </div>
        `;
    }

    prepareDownload(capsuleId) {
        document.getElementById('downloadCapsuleId').value = capsuleId;
        document.getElementById('downloadSection').style.display = 'block';
        
        // Scroll to download section
        document.getElementById('downloadSection').scrollIntoView({ behavior: 'smooth' });
    }

    async handleDownload() {
        const capsuleId = document.getElementById('downloadCapsuleId').value;
        const privateKeyFile = document.getElementById('privateKeyFile').files[0];
        const password = document.getElementById('decryptPassword').value;

        if (!capsuleId || !privateKeyFile) {
            this.showOutput('Error: Please select capsule and private key file', 'error');
            return;
        }

        try {
            this.showOutput('Starting download and decryption process...', 'info');

            // Step 1: Download encrypted files
            this.showOutput('Downloading encrypted files...', 'info');
            const downloadedFiles = await this.downloadCapsuleFiles(capsuleId);

            // Step 2: Run C++ decryptor
            this.showOutput('Decrypting files...', 'info');
            const decryptionResult = await this.runDecryptor({
                capsule_id: capsuleId,
                private_key_file: privateKeyFile,
                encrypted_file: downloadedFiles.encryptedFile,
                encrypted_key: downloadedFiles.encryptedKey,
                password: password
            });

            if (decryptionResult.success) {
                this.showOutput('✅ File decrypted successfully!', 'success');
                this.offerFileDownload(decryptionResult.decrypted_file, decryptionResult.filename);
            } else {
                this.showOutput(`Decryption failed: ${decryptionResult.error}`, 'error');
            }

        } catch (error) {
            this.showOutput(`Error: ${error.message}`, 'error');
        }
    }

    async downloadCapsuleFiles(capsuleId) {
        // In a real implementation, this would download the actual files
        // For the web demo, we'll simulate this process
        
        return new Promise((resolve) => {
            setTimeout(() => {
                resolve({
                    encryptedFile: 'simulated_encrypted_file.bin',
                    encryptedKey: 'simulated_encrypted_key.bin'
                });
            }, 1000);
        });
    }

    async runDecryptor(config) {
        // This would interface with the C++ decryptor executable
        // For the web demo, we'll simulate this process
        
        return new Promise((resolve) => {
            setTimeout(() => {
                // Simulate successful decryption
                resolve({
                    success: true,
                    decrypted_file: new Blob(['decrypted file content'], { type: 'application/octet-stream' }),
                    filename: 'decrypted_document.pdf'
                });
            }, 2000);
        });
    }

    offerFileDownload(blob, filename) {
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();
        URL.revokeObjectURL(url);
    }

    async validatePrivateKey(file) {
        if (!file) return;

        // Basic validation - check if it looks like a private key
        const text = await file.text();
        if (text.includes('-----BEGIN PRIVATE KEY-----') && 
            text.includes('-----END PRIVATE KEY-----')) {
            document.getElementById('keyValidation').textContent = '✅ Valid private key format';
            document.getElementById('keyValidation').className = 'validation valid';
        } else {
            document.getElementById('keyValidation').textContent = '❌ Invalid private key format';
            document.getElementById('keyValidation').className = 'validation invalid';
        }
    }

    showOutput(message, type = 'info') {
        const output = document.getElementById('out');
        const messageClass = `message-${type}`;
        
        output.innerHTML += `<div class="message ${messageClass}">${message}</div>`;
        output.scrollTop = output.scrollHeight;
    }

    formatFileSize(bytes) {
        if (bytes === 0) return '0 Bytes';
        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }
}

// Initialize the application
const receiverApp = new ReceiverApp();