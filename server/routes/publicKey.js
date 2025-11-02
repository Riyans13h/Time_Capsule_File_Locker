const express = require('express');
const router = express.Router();
const multer = require('multer');
const fs = require('fs');
const path = require('path');
const db = require('../db');

// Configure multer for public key uploads
const keyStorage = multer.diskStorage({
    destination: function (req, file, cb) {
        const keysDir = path.join(__dirname, '../storage/keys');
        if (!fs.existsSync(keysDir)) {
            fs.mkdirSync(keysDir, { recursive: true });
        }
        cb(null, keysDir);
    },
    filename: function (req, file, cb) {
        const receiverId = req.body.receiver_id;
        const filename = `public_key_${receiverId}_${Date.now()}.pem`;
        cb(null, filename);
    }
});

const upload = multer({ 
    storage: keyStorage,
    limits: {
        fileSize: 1024 * 1024, // 1MB max for public keys
    },
    fileFilter: (req, file, cb) => {
        // Check if file is a PEM file
        if (file.mimetype === 'application/x-pem-file' || 
            file.originalname.endsWith('.pem')) {
            cb(null, true);
        } else {
            cb(new Error('Only PEM files are allowed'), false);
        }
    }
});

// Register receiver and upload public key
router.post('/register', upload.single('public_key'), (req, res) => {
    try {
        const { receiver_id, contact_email } = req.body;
        
        if (!receiver_id || !req.file) {
            return res.status(400).json({
                error: 'Missing required fields',
                details: 'receiver_id and public_key are required'
            });
        }

        // Read the public key file
        const publicKeyPem = fs.readFileSync(req.file.path, 'utf8');
        
        // Validate it's a proper PEM format
        if (!publicKeyPem.includes('-----BEGIN PUBLIC KEY-----') || 
            !publicKeyPem.includes('-----END PUBLIC KEY-----')) {
            // Clean up the uploaded file
            fs.unlinkSync(req.file.path);
            return res.status(400).json({
                error: 'Invalid public key format',
                details: 'File must be in PEM format'
            });
        }

        const now = new Date().toISOString();
        
        // Insert or update receiver
        const stmt = db.prepare(`
            INSERT OR REPLACE INTO receivers 
            (receiver_id, public_key_pem, contact_email, updated_at)
            VALUES (?, ?, ?, ?)
        `);
        
        stmt.run(receiver_id, publicKeyPem, contact_email, now);

        res.json({
            status: 'success',
            message: 'Public key registered successfully',
            receiver_id: receiver_id,
            key_path: req.file.path
        });

    } catch (error) {
        console.error('Public key registration error:', error);
        res.status(500).json({
            error: 'Internal server error',
            details: error.message
        });
    }
});

// Get public key for a receiver
router.get('/:receiver_id', (req, res) => {
    try {
        const { receiver_id } = req.params;
        
        const stmt = db.prepare(`
            SELECT receiver_id, public_key_pem, contact_email, created_at
            FROM receivers 
            WHERE receiver_id = ?
        `);
        
        const receiver = stmt.get(receiver_id);
        
        if (!receiver) {
            return res.status(404).json({
                error: 'Receiver not found',
                details: `No public key found for receiver: ${receiver_id}`
            });
        }

        res.json({
            status: 'success',
            receiver_id: receiver.receiver_id,
            public_key_pem: receiver.public_key_pem,
            contact_email: receiver.contact_email,
            registered_at: receiver.created_at
        });

    } catch (error) {
        console.error('Public key retrieval error:', error);
        res.status(500).json({
            error: 'Internal server error',
            details: error.message
        });
    }
});

// List all receivers (for admin purposes)
router.get('/', (req, res) => {
    try {
        const stmt = db.prepare(`
            SELECT receiver_id, contact_email, created_at
            FROM receivers 
            ORDER BY created_at DESC
        `);
        
        const receivers = stmt.all();
        
        res.json({
            status: 'success',
            count: receivers.length,
            receivers: receivers
        });

    } catch (error) {
        console.error('Receivers list error:', error);
        res.status(500).json({
            error: 'Internal server error',
            details: error.message
        });
    }
});

module.exports = router;