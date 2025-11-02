const express = require('express');
const router = express.Router();
const multer = require('multer');
const fs = require('fs');
const path = require('path');
const { v4: uuidv4 } = require('uuid');
const db = require('../db');

// Configure multer for file uploads
const fileStorage = multer.diskStorage({
    destination: function (req, file, cb) {
        const filesDir = path.join(__dirname, '../storage/files');
        if (!fs.existsSync(filesDir)) {
            fs.mkdirSync(filesDir, { recursive: true });
        }
        cb(null, filesDir);
    },
    filename: function (req, file, cb) {
        const timestamp = Date.now();
        const randomString = Math.random().toString(36).substring(2, 15);
        const extension = path.extname(file.originalname);
        const filename = `encrypted_${timestamp}_${randomString}${extension}`;
        cb(null, filename);
    }
});

const upload = multer({ 
    storage: fileStorage,
    limits: {
        fileSize: 100 * 1024 * 1024, // 100MB max file size
    }
});

// Upload encrypted file and key package
router.post('/', upload.fields([
    { name: 'encrypted_file', maxCount: 1 },
    { name: 'encrypted_key_package', maxCount: 1 }
]), (req, res) => {
    try {
        const {
            receiver_id,
            sender_info,
            original_filename,
            release_time,
            sha256_hash,
            file_size
        } = req.body;

        // Validate required fields
        if (!receiver_id || !release_time || !original_filename) {
            return res.status(400).json({
                error: 'Missing required fields',
                details: 'receiver_id, release_time, and original_filename are required'
            });
        }

        // Check if files were uploaded
        if (!req.files || !req.files['encrypted_file'] || !req.files['encrypted_key_package']) {
            return res.status(400).json({
                error: 'Missing files',
                details: 'Both encrypted_file and encrypted_key_package are required'
            });
        }

        // Verify receiver exists
        const receiverStmt = db.prepare('SELECT receiver_id FROM receivers WHERE receiver_id = ?');
        const receiver = receiverStmt.get(receiver_id);
        
        if (!receiver) {
            // Clean up uploaded files
            if (req.files['encrypted_file']) {
                fs.unlinkSync(req.files['encrypted_file'][0].path);
            }
            if (req.files['encrypted_key_package']) {
                fs.unlinkSync(req.files['encrypted_key_package'][0].path);
            }
            
            return res.status(404).json({
                error: 'Receiver not found',
                details: `No receiver registered with ID: ${receiver_id}`
            });
        }

        // Validate release time is in the future
        const releaseDate = new Date(release_time);
        const now = new Date();
        
        if (releaseDate <= now) {
            return res.status(400).json({
                error: 'Invalid release time',
                details: 'Release time must be in the future'
            });
        }

        // Generate unique capsule ID
        const capsule_id = uuidv4();
        const created_at = new Date().toISOString();

        // Get file paths
        const encrypted_file_path = req.files['encrypted_file'][0].path;
        const encrypted_key_path = req.files['encrypted_key_package'][0].path;

        // Insert capsule into database
        const stmt = db.prepare(`
            INSERT INTO capsules (
                capsule_id, sender_info, receiver_id, original_filename,
                encrypted_file_path, encrypted_key_path, file_size,
                sha256_hash, release_time, created_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        `);

        stmt.run(
            capsule_id,
            sender_info || 'anonymous',
            receiver_id,
            original_filename,
            encrypted_file_path,
            encrypted_key_path,
            file_size || 0,
            sha256_hash || '',
            release_time,
            created_at
        );

        console.log(`New capsule created: ${capsule_id} for receiver: ${receiver_id}`);

        res.json({
            status: 'success',
            message: 'Time capsule created successfully',
            capsule_id: capsule_id,
            receiver_id: receiver_id,
            release_time: release_time,
            created_at: created_at
        });

    } catch (error) {
        console.error('File upload error:', error);
        
        // Clean up any uploaded files on error
        if (req.files) {
            Object.values(req.files).forEach(fileArray => {
                fileArray.forEach(file => {
                    try {
                        if (fs.existsSync(file.path)) {
                            fs.unlinkSync(file.path);
                        }
                    } catch (cleanupError) {
                        console.error('Error cleaning up file:', cleanupError);
                    }
                });
            });
        }

        res.status(500).json({
            error: 'Internal server error',
            details: error.message
        });
    }
});

// Get upload status
router.get('/status/:capsule_id', (req, res) => {
    try {
        const { capsule_id } = req.params;
        
        const stmt = db.prepare(`
            SELECT 
                capsule_id, sender_info, receiver_id, original_filename,
                file_size, sha256_hash, release_time, status,
                created_at, delivered_at
            FROM capsules 
            WHERE capsule_id = ?
        `);
        
        const capsule = stmt.get(capsule_id);
        
        if (!capsule) {
            return res.status(404).json({
                error: 'Capsule not found',
                details: `No capsule found with ID: ${capsule_id}`
            });
        }

        res.json({
            status: 'success',
            capsule: capsule
        });

    } catch (error) {
        console.error('Status check error:', error);
        res.status(500).json({
            error: 'Internal server error',
            details: error.message
        });
    }
});

module.exports = router;