const express = require('express');
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');

const router = express.Router();

const upload = multer({
    dest: 'data/uploads/',
    limits: { fileSize: 50 * 1024 * 1024 }
});

router.post('/', upload.single('file'), (req, res) => {
    try {
        // Validate inputs
        if (!req.file) {
            return res.status(400).json({ message: 'No file uploaded' });
        }

        if (!req.body.password || req.body.password.length < 8) {
            return res.status(400).json({ message: 'Password must be at least 8 characters' });
        }

        const inputPath = req.file.path;
        const password = req.body.password;
        const originalFilename = req.file.originalname;

        // Find the correct metadata file
        const possibleMetaPaths = [
            path.join(__dirname, '../../data/capsules', `${originalFilename}.meta`), // For files locked as-is
            path.join(__dirname, '../../data/capsules', `${path.parse(originalFilename).name}_*.tcf.meta`) // For renamed locked files
        ];

        // Find the first existing metadata file
        let capsuleMetaPath;
        for (const possiblePath of possibleMetaPaths) {
            if (fs.existsSync(possiblePath)) {
                capsuleMetaPath = possiblePath;
                break;
            }
        }

        if (!capsuleMetaPath) {
            // Fallback: search for any .meta file matching the original name
            const capsuleDir = path.join(__dirname, '../../data/capsules');
            const files = fs.readdirSync(capsuleDir);
            const matchingMeta = files.find(f => 
                f.startsWith(path.parse(originalFilename).name) && 
                f.endsWith('.tcf.meta')
            );
            
            if (matchingMeta) {
                capsuleMetaPath = path.join(capsuleDir, matchingMeta);
            } else {
                return res.status(400).json({ 
                    message: 'Metadata file not found. Was this file properly locked?' 
                });
            }
        }

        // Create temp directory if needed
        const tempDir = path.join(__dirname, '../../data/temp');
        if (!fs.existsSync(tempDir)) {
            fs.mkdirSync(tempDir, { recursive: true });
        }

        // Copy metadata to temp location for verification
        const tempMetaPath = path.join(tempDir, `${req.file.filename}.meta`);
        fs.copyFileSync(capsuleMetaPath, tempMetaPath);

        // 1. Verify time lock using metadata
        exec(`../backend/time_lock "${tempMetaPath}"`, (timeErr, stdout, stderr) => {
            // Cleanup temp metadata file
            if (fs.existsSync(tempMetaPath)) fs.unlinkSync(tempMetaPath);

            if (timeErr) {
                // Extract unlock date from stderr
                const unlockMatch = /UNLOCK=(\d+)/.exec(stderr);
                const unlockDate = unlockMatch 
                    ? new Date(parseInt(unlockMatch[1]) * 1000).toLocaleString()
                    : 'the specified date';
                
                return res.status(403).json({ 
                    message: `File is time-locked until ${unlockDate}` 
                });
            }

            // 2. Proceed with decryption if time lock is valid
            const tempPath = path.join(tempDir, `${req.file.filename}.huff`);
            const outputFilename = `unlocked_${Date.now()}_${path.parse(originalFilename).name}`;
            const outputPath = path.join(tempDir, outputFilename);

            exec(
                `../backend/decrypt "${inputPath}" "${tempPath}" "${password}" && ` +
                `../backend/decompress "${tempPath}" "${outputPath}"`,
                (processErr, stdout, stderr) => {
                    // Cleanup temp files
                    [inputPath, tempPath].forEach(file => {
                        if (file && fs.existsSync(file)) fs.unlinkSync(file);
                    });

                    if (processErr) {
                        console.error('Unlock error:', stderr);
                        return res.status(400).json({ 
                            message: 'Invalid password or corrupted file' 
                        });
                    }

                    // Verify output file exists
                    if (!fs.existsSync(outputPath)) {
                        return res.status(500).json({ 
                            message: 'Processing failed - no output file' 
                        });
                    }

                    // Send the unlocked file
                    res.download(outputPath, outputFilename, (err) => {
                        // Cleanup after download completes
                        if (fs.existsSync(outputPath)) fs.unlinkSync(outputPath);
                        if (err) console.error('Download error:', err);
                    });
                }
            );
        });

    } catch (error) {
        console.error('Server error:', error);
        res.status(500).json({ message: 'Internal server error' });
    }
});

module.exports = router;