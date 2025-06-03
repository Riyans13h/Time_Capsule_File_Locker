const express = require('express');
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');

const router = express.Router();
const upload = multer({ dest: 'data/uploads/' });

router.post('/', upload.single('file'), (req, res) => {
    const { unlockDate, password } = req.body;
    const inputPath = req.file.path;
    const originalName = req.file.originalname;
    const outputName = `${path.parse(originalName).name}_${Date.now()}.tcf`;
    const outputPath = path.join(__dirname, '../../data/capsules', outputName);
    const metaPath = `${outputPath}.meta`;
    const tempPath = path.join(__dirname, '../../data/temp', `${req.file.filename}.huff`);

    // Create directories if they don't exist
    if (!fs.existsSync(path.join(__dirname, '../../data/capsules'))) {
        fs.mkdirSync(path.join(__dirname, '../../data/capsules'), { recursive: true });
    }
    if (!fs.existsSync(path.join(__dirname, '../../data/temp'))) {
        fs.mkdirSync(path.join(__dirname, '../../data/temp'), { recursive: true });
    }

    // Step 1: Generate metadata first
    const metaCommand = `../backend/metadata "${inputPath}" "${unlockDate}" "${metaPath}"`;
    
    // Step 2: Compress -> Encrypt
    const processCommand = `../backend/compress "${inputPath}" "${tempPath}" && ` +
                          `../backend/encrypt "${tempPath}" "${outputPath}" "${password}"`;

    // Execute metadata generation first
    exec(metaCommand, (metaErr) => {
        if (metaErr) {
            cleanup(inputPath, tempPath);
            return res.status(500).json({ error: 'Metadata generation failed' });
        }

        // Then process the file
        exec(processCommand, (processErr) => {
            cleanup(inputPath, tempPath);
            
            if (processErr) {
                // Clean up metadata if processing failed
                if (fs.existsSync(metaPath)) fs.unlinkSync(metaPath);
                return res.status(500).json({ error: 'File processing failed' });
            }

            res.json({ 
                success: true,
                filename: outputName 
            });
        });
    });
});

function cleanup(...paths) {
    paths.forEach(p => {
        try { if (p && fs.existsSync(p)) fs.unlinkSync(p); } 
        catch (err) { console.error('Cleanup error:', err); }
    });
}

module.exports = router;