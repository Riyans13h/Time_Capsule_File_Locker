const CapsuleModel = require('../db/capsuleModel');
const FileUtils = require('../utils/fileUtils');
const { v4: uuidv4 } = require('uuid');

class LockController {
  async createCapsule(req, res) {
    try {
      const {
        encryptedData,
        originalFilename,
        unlockTime,
        fileHash,
        encryptedHash,
        metadataHash,
        originalSize,
        compressedSize,
        encryptedSize
      } = req.body;

      // Validate required fields
      if (!encryptedData || !originalFilename || !unlockTime) {
        return res.status(400).json({ error: 'Missing required fields' });
      }

      // Generate UNIQUE Capsule ID on SERVER
      const capsuleId = this.generateCapsuleId();
      
      // Generate encrypted filename using Capsule ID
      const encryptedFilename = `capsule_${capsuleId}.tcf`;
      
      // Convert base64 data to buffer
      const dataBuffer = Buffer.from(encryptedData, 'base64');
      
      // Save encrypted file with server-generated filename
      await fileUtils.saveFile(encryptedFilename, dataBuffer);
      
      // Create database record with SERVER-GENERATED ID
      const capsuleData = {
        capsuleId, // Server-generated ID
        originalFilename,
        encryptedFilename,
        unlockTime: new Date(unlockTime).toISOString(),
        fileHash,
        encryptedHash,
        metadataHash,
        originalSize,
        compressedSize,
        encryptedSize
      };

      // Save to database
      capsuleModel.createCapsule(capsuleData, (err, result) => {
        if (err) {
          console.error('Database error:', err);
          fileUtils.deleteFile(encryptedFilename);
          return res.status(500).json({ error: 'Failed to create time capsule' });
        }

        // Log the creation
        const ip = req.ip || req.connection.remoteAddress;
        capsuleModel.logAccess(capsuleId, ip, req.get('User-Agent'), 'CREATE', 'SUCCESS');

        res.status(201).json({
          message: 'Time capsule created successfully',
          capsuleId: capsuleId, // SERVER-GENERATED ID
          unlockTime: unlockTime,
          filename: originalFilename
        });
      });

    } catch (error) {
      console.error('Error creating time capsule:', error);
      res.status(500).json({ error: 'Failed to create time capsule' });
    }
  }

  // Generate secure server-side Capsule ID
  generateCapsuleId() {
    // Format: tc_TIMESTAMP_RANDOM
    const timestamp = Date.now().toString(36);
    const random = Math.random().toString(36).substring(2, 10);
    return `tc_${timestamp}_${random}`;
  }
}