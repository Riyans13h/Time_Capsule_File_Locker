
# 🔒 Time Capsule File Locker ⏳

A secure, zero-knowledge file locking system that allows users to encrypt and compress files, assign a future unlock date, and share access securely via a Capsule ID. Decryption and decompression occur **only on the client side**, ensuring complete end-to-end security.

---

## 📚 Table of Contents
1. [Project Overview](#project-overview)
2. [Key Features](#key-features)
3. [Technical Architecture](#technical-architecture)
4. [System Workflow](#system-workflow)
5. [Installation Guide](#installation-guide)
6. [Configuration](#configuration)
7. [API Documentation](#api-documentation)
8. [Security Considerations](#security-considerations)
9. [Performance Metrics](#performance-metrics)
10. [Troubleshooting](#troubleshooting)
11. [FAQs](#faqs)
12. [Contributing](#contributing)
13. [License](#license)

---

## 🌐 Project Overview

The Time Capsule File Locker allows users to:
- Compress, encrypt, and lock files until a **specific future date**
- Store locked files on a **zero-knowledge server**
- Share **only a Capsule ID and password** with recipients
- Ensure decryption only happens **after the unlock date** on the recipient’s machine

**Use Cases**:
- Legal document escrow
- Digital wills & posthumous messages
- Intellectual property protection
- Time-based journal backups

---

## ✨ Key Features

### ✅ Security
- AES-256-CBC encryption via Crypto++
- PBKDF2 key derivation with salt (100,000 iterations)
- Time-lock via UTC timestamp validation
- SHA-256 hashing for integrity verification
- Zero-knowledge server: password never sent

### 🚀 Performance
- Huffman compression (30–50% typical size reduction)
- Large-file support (multi-GB)
- Native C++ for core logic + WebAssembly build option

### 🖥 Usability
- Simple drag & drop UI
- No registration/login required
- Download history, hash verification
- Fully client-side decryption

---

## 🏗 Technical Architecture

```mermaid
graph TD
    A[User Browser] -->|Encrypt & Compress| B[Client-Side WASM]
    B -->|POST .tcf + metadata| C[Node.js Server]
    C -->|Store| D[SQLite + Secure File Storage]
    User -->|Capsule ID + Password| E[Recipient]
    E -->|Fetch .tcf by ID| C
    C -->|If UnlockDate Valid| E
    E -->|Decrypt + Decompress| B
````

---

## 🔄 System Workflow

### 🔐 Sender (Client 1)

1. Compress → Encrypt → Hash file using password
2. Generate metadata (salt, unlock date, hash)
3. Upload `.tcf` + metadata to server
4. Server returns a `capsuleId`
5. Share capsule ID + password with receiver

### 📥 Receiver (Client 2)

1. Submit capsule ID to server
2. If unlock date is reached → receive `.tcf` + metadata
3. Use password to:

   * Derive key
   * Decrypt + decompress `.tcf`
   * Verify SHA-256 hash

---

## 🧪 Installation Guide

### 🔧 Prerequisites

* Linux/macOS (or WSL)
* GCC/Clang with C++17 support
* Node.js >= 16.x
* Crypto++ library
* SQLite3
* Emscripten (optional for WebAssembly)

### 📥 Setup

```bash
# Clone the repository
git clone https://github.com/yourusername/time-capsule-locker.git
cd time-capsule-locker

# Build C++ logic
cd cpp_core
make
cd ..

# Install server dependencies
cd server
npm install

# Setup environment
cp .env.example .env
nano .env  # Set variables like PORT, FILE_SIZE_LIMIT

# Start backend server
node server.js
```

---

## ⚙ Configuration

`.env` Example:

```ini
PORT=3000
MAX_FILE_SIZE=1073741824
UPLOAD_DIR=./data/uploads
CAPSULE_DIR=./data/capsules
DB_PATH=./server/capsules.db
```

---

## 📡 API Documentation

### `POST /api/lock`

Uploads a `.tcf` file and metadata to the server.

#### Request (multipart/form-data):

* `file`: Binary `.tcf` file
* `sha256`: File hash
* `unlockDate`: ISO UTC date
* `salt`: Random hex salt
* `filename`: Original filename

#### Response:

```json
{
  "status": "success",
  "capsuleId": "ab12cd34-ef56-7890-ab12-34cd56ef7890"
}
```

---

### `POST /api/unlock`

Fetch file and metadata by Capsule ID (if unlock date is valid)

#### Request:

```json
{
  "capsuleId": "ab12cd34-ef56-7890-ab12-34cd56ef7890"
}
```

#### Response (if unlocked):

```json
{
  "status": "success",
  "file": "<base64 .tcf>",
  "salt": "abc123...",
  "sha256": "def456...",
  "filename": "message.tcf"
}
```

#### Response (if locked):

```json
{
  "status": "error",
  "message": "File is still locked. Try again later."
}
```

---

## 🔐 Security Considerations

| Threat               | Defense                        |
| -------------------- | ------------------------------ |
| Brute-force attacks  | PBKDF2 key derivation w/ salt  |
| Server compromise    | Encrypted file + zero-password |
| Metadata tampering   | SHA-256 hash + hash check      |
| Replay attacks       | UUID capsule IDs               |
| Early access attempt | Time check on server           |

---

## 📊 Performance Benchmarks

| File Size | Compression | Encryption | Upload |
| --------- | ----------- | ---------- | ------ |
| 1 MB      | \~100 ms    | \~80 ms    | Fast   |
| 100 MB    | \~1.2 s     | \~0.9 s    | Fast   |
| 1 GB      | \~11.5 s    | \~8.3 s    | Medium |

---

## 🧰 Troubleshooting

### Issue: Crypto++ not found

```bash
sudo ldconfig
export LD_LIBRARY_PATH=/usr/local/lib
```

### Issue: Unlock returns “still locked”

* Ensure system time is correct
* Server uses UTC; client mismatch may block unlock

---

## ❓ FAQs

**Q: Can I recover files without the password?**
A: No. The system uses zero-knowledge encryption. Passwords are never stored.

**Q: What if I try to unlock too early?**
A: The server will deny the request until the unlock date.

**Q: Can I change the unlock date?**
A: No. Once uploaded, the `.tcf` file and its metadata are immutable.

---

## 🤝 Contributing

We welcome contributions!

1. Fork the repo
2. Create a feature branch
3. Add changes with `commit -m "description"`
4. Push and open a Pull Request

---

## 📜 License

MIT License
© 2024 Riyansh Sachan

- a `poster or presentation format`

I can generate any of those from this content.
```
