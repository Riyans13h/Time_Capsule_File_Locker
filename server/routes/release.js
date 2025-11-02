const express = require('express');
const router = express.Router();
const fs = require('fs');
const path = require('path');
const db = require('../db');

// Get list of available capsules for a receiver
router.get('/available/:receiver_id', (req, res) => {
    try {
        const { receiver_id } = req.params;
        
        const stmt = db.prepare(`
            SELECT 
                capsule_id, sender_info, original_filename,
                file_size, release_time, created_at, delivered_at
            FROM capsules 
            WHERE receiver_id = ? AND status = 'delivered'
            ORDER BY delivered_at DESC
        `);
        
        const capsules = stmt.all(receiver_id);
        
        res.json({
            status: 'success',
            count: capsules.length,
            capsules: capsules
        });

    } catch (error) {
        console.error('Available capsules error:', error);
        res.status(500).json({
            error: 'Internal server error',
            details: error.message
        });
    }
});

// Download encrypted file
router.get('/download/file/:capsule_id', (req, res) => {
    try {
        const { capsule_id } = req.params;
        
        const stmt = db.prepare(`
            SELECT 
                capsule_id, original_filename, encrypted_file_path,
                receiver_id, status
            FROM capsules 
            WHERE capsule_id = ?
        `);
        
        const capsule = stmt.get(capsule_id);
        
        if (!capsule) {
            return res.status(404).json({
                error: 'Capsule not found'
            });
        }

        if (capsule.status !== 'delivered') {
            return res.status(403).json({
                error: 'Capsule not available',
                details: 'This capsule has not been released yet'
            });
        }

        if (!fs.existsSync(capsule.encrypted_file_path)) {
            return res.status(404).json({
                error: 'File not found',
                details: 'Encrypted file missing from storage'
            });
        }

        // Set appropriate headers for file download
        res.setHeader('Content-Type', 'application/octet-stream');
        res.setHeader('Content-Disposition', 
            `attachment; filename="${capsule.original_filename}.encrypted"`);
        
        // Stream the file to the client
        const fileStream = fs.createReadStream(capsule.encrypted_file_path);
        fileStream.pipe(res);

        console.log(`File downloaded: ${capsule.encrypted_file_path} for capsule: ${capsule_id}`);

    } catch (error) {
        console.error('File download error:', error);
        res.status(500).json({
            error: 'Internal server error',
            details: error.message
        });
    }
});

// Download encrypted key package
router.get('/download/key/:capsule_id', (req, res) => {
    try {
        const { capsule_id } = req.params;
        
        const stmt = db.prepare(`
            SELECT 
                capsule_id, encrypted_key_path, receiver_id, status
            FROM capsules 
            WHERE capsule_id = ?
        `);
        
        const capsule = stmt.get(capsule_id);
        
        if (!capsule) {
            return res.status(404).json({
                error: 'Capsule not found'
            });
        }

        if (capsule.status !== 'delivered') {
            return res.status(403).json({
                error: 'Capsule not available',
                details: 'This capsule has not been released yet'
            });
        }

        if (!fs.existsSync(capsule.encrypted_key_path)) {
            return res.status(404).json({
                error: 'Key package not found',
                details: 'Encrypted key package missing from storage'
            });
        }

        // Set appropriate headers for file download
        res.setHeader('Content-Type', 'application/octet-stream');
        res.setHeader('Content-Disposition', 
            'attachment; filename="encrypted_key_package.bin"');
        
        // Stream the key package to the client
        const fileStream = fs.createReadStream(capsule.encrypted_key_path);
        fileStream.pipe(res);

        console.log(`Key package downloaded: ${capsule.encrypted_key_path} for capsule: ${capsule_id}`);

    } catch (error) {
        console.error('Key package download error:', error);
        res.status(500).json({
            error: 'Internal server error',
            details: error.message
        });
    }
});

// Get capsule metadata
router.get('/metadata/:capsule_id', (req, res) => {
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
                error: 'Capsule not found'
            });
        }

        res.json({
            status: 'success',
            capsule: capsule
        });

    } catch (error) {
        console.error('Metadata retrieval error:', error);
        res.status(500).json({
            error: 'Internal server error',
            details: error.message
        });
    }
});

module.exports = router;