document.addEventListener('DOMContentLoaded', () => {
    // Initialize UI components
    initTabs();
    initDatePicker();
    initFileInputs();
});

function initTabs() {
    const tabs = document.querySelectorAll('.tab');
    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            tabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');
            
            document.querySelectorAll('.tab-content').forEach(content => {
                content.classList.remove('active');
            });
            document.getElementById(`${tab.dataset.tab}-tab`).classList.add('active');
        });
    });
}

function initDatePicker() {
    const dateInput = document.getElementById('unlock-date');
    const tomorrow = new Date();
    tomorrow.setDate(tomorrow.getDate() + 1);
    dateInput.min = tomorrow.toISOString().split('T')[0];
    dateInput.value = dateInput.min; // Set default to tomorrow
}

function initFileInputs() {
    setupFileInput('file-input', 'file-name');
    setupFileInput('tcf-input', 'tcf-name');
    
    document.getElementById('lock-btn').addEventListener('click', lockFile);
    document.getElementById('unlock-btn').addEventListener('click', unlockFile);
}

function setupFileInput(inputId, displayId) {
    const input = document.getElementById(inputId);
    const display = document.getElementById(displayId);
    
    input.addEventListener('change', () => {
        display.textContent = input.files.length > 0 
            ? input.files[0].name 
            : 'No file selected';
    });
}

async function lockFile() {
    const fileInput = document.getElementById('file-input');
    const dateInput = document.getElementById('unlock-date');
    const passwordInput = document.getElementById('password');
    const statusDiv = document.getElementById('lock-status');
    
    resetStatus(statusDiv);

    try {
        validateLockInputs(fileInput, dateInput, passwordInput, statusDiv);
        
        statusDiv.textContent = 'Locking file...';
        const formData = createLockFormData(fileInput, dateInput, passwordInput);
        
        const response = await fetchWithTimeout('http://localhost:3000/api/lock', {
            method: 'POST',
            body: formData
        });

        await handleLockResponse(response, statusDiv);

    } catch (error) {
        handleLockError(error, statusDiv);
    }
}

async function unlockFile() {
    const fileInput = document.getElementById('tcf-input');
    const passwordInput = document.getElementById('unlock-password');
    const statusDiv = document.getElementById('unlock-status');
    
    resetStatus(statusDiv);

    try {
        validateUnlockInputs(fileInput, passwordInput, statusDiv);
        
        statusDiv.textContent = 'Unlocking file...';
        const formData = createUnlockFormData(fileInput, passwordInput);
        
        const response = await fetchWithTimeout('http://localhost:3000/api/unlock', {
            method: 'POST',
            body: formData
        });

        await handleUnlockResponse(response, fileInput, statusDiv);

    } catch (error) {
        handleUnlockError(error, statusDiv);
    }
}

// Helper Functions
function resetStatus(element) {
    element.textContent = '';
    element.className = 'status';
}

function validateLockInputs(fileInput, dateInput, passwordInput, statusDiv) {
    if (fileInput.files.length === 0) throw new Error('Please select a file');
    if (!dateInput.value) throw new Error('Please select an unlock date');
    if (passwordInput.value.length < 8) throw new Error('Password must be at least 8 characters');
}

function validateUnlockInputs(fileInput, passwordInput, statusDiv) {
    if (fileInput.files.length === 0) throw new Error('Please select a .tcf file');
    if (!passwordInput.value) throw new Error('Please enter the password');
}

function createLockFormData(fileInput, dateInput, passwordInput) {
    const formData = new FormData();
    formData.append('file', fileInput.files[0]);
    formData.append('unlockDate', dateInput.value);
    formData.append('password', passwordInput.value);
    return formData;
}

function createUnlockFormData(fileInput, passwordInput) {
    const formData = new FormData();
    formData.append('file', fileInput.files[0]);
    formData.append('password', passwordInput.value);
    return formData;
}

async function fetchWithTimeout(url, options, timeout = 10000) {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), timeout);
    
    try {
        const response = await fetch(url, {
            ...options,
            signal: controller.signal,
            headers: {
                'Accept': 'application/json',
                ...options.headers
            }
        });
        clearTimeout(timeoutId);
        return response;
    } catch (error) {
        clearTimeout(timeoutId);
        throw error;
    }
}

async function handleLockResponse(response, statusDiv) {
    if (!response.ok) {
        const errorData = await response.json().catch(() => ({}));
        throw new Error(errorData.error || `Server error: ${response.status}`);
    }

    const result = await response.json();
    
    showSuccess(statusDiv, 'File locked successfully! Download link: ');
    const downloadLink = document.createElement('a');
    downloadLink.href = `http://localhost:3000/download/${result.filename}`;
    downloadLink.textContent = result.filename;
    downloadLink.className = 'download-link';
    downloadLink.download = result.filename;
    statusDiv.appendChild(downloadLink);
}

async function handleUnlockResponse(response, fileInput, statusDiv) {
    if (!response.ok) {
        const errorData = await response.json().catch(() => ({}));
        throw new Error(errorData.message || `Server error: ${response.status}`);
    }

    const blob = await response.blob();
    
    if (blob.size === 0) {
        throw new Error('Received empty file - possibly incorrect password');
    }

    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `unlocked_${fileInput.files[0].name.replace('.tcf', '')}`;
    document.body.appendChild(a);
    a.click();

    setTimeout(() => {
        window.URL.revokeObjectURL(url);
        document.body.removeChild(a);
        showSuccess(statusDiv, 'File unlocked successfully!');
    }, 100);
}

function handleLockError(error, statusDiv) {
    const errorMessage = error.name === 'AbortError' 
        ? 'Request timed out. Please try again.' 
        : error.message || 'Network error. Please try again.';
    
    showError(statusDiv, errorMessage);
    console.error('Lock error:', error);
}

function handleUnlockError(error, statusDiv) {
    const errorMessage = error.name === 'AbortError'
        ? 'Request timed out. Please try again.'
        : error.message.includes('password')
            ? 'Incorrect password or corrupted file'
            : error.message || 'Network error. Please try again.';
    
    showError(statusDiv, errorMessage);
    console.error('Unlock error:', error);
}

function showError(element, message) {
    element.textContent = message;
    element.classList.add('error');
}

function showSuccess(element, message) {
    element.textContent = message;
    element.classList.add('success');
}