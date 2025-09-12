const CapsuleModel = require('../db/capsuleModel');
const FileUtils = require('../utils/fileUtils');

const capsuleModel = new CapsuleModel();
const fileUtils = new FileUtils();

class UnlockController {
  // Check if a capsule can be unlocked
  async checkUnlockStatus(req, res) {
    try {
      const { capsuleId } = req.params;
      
      capsuleModel.isUnlockTimeReached(capsuleId, (err, result) => {
        if (err) {
          console.error('Database error:', err);
          return res.status(500).json({ error: 'Database error' });
        }
        
        res.json({
          capsuleId,
          isUnlocked: result.isUnlocked,
          unlockTime: result.unlockTime,
          currentTime: new Date().toISOString()
        });
      });
    } catch (error) {
      console.error('Error checking unlock status:', error);
      res.status(500).json({ error: 'Failed to check unlock status' });
    }
  }

  // Unlock a time capsule
  async unlockCapsule(req, res) {
    try {
      const { capsuleId } = req.params;
      const { password } = req.body;
      
      if (!password) {
        return res.status(400).json({ error: 'Password is required' });
      }
      
      // First check if the capsule exists and can be unlocked
      capsuleModel.isUnlockTimeReached(capsuleId, async (err, result) => {
        if (err) {
          console.error('Database error:', err);
          return res.status(500).json({ error: 'Database error' });
        }
        
        if (!result) {
          return res.status(404).json({ error: 'Time capsule not found' });
        }
        
        if (!result.isUnlocked) {
          return res.status(403).json({
            error: 'Time capsule is still locked',
            unlockTime: result.unlockTime
          });
        }
        
        // Get capsule details
        capsuleModel.getCapsule(capsuleId, async (err, capsule) => {
          if (err) {
            console.error('Database error:', err);
            return res.status(500).json({ error: 'Database error' });
          }
          
          try {
            // Read the encrypted file
            const encryptedData = await fileUtils.readFile(capsule.encrypted_filename);
            
            
            const response = {
              capsuleId: capsule.capsule_id,
              originalFilename: capsule.original_filename,
              encryptedData: encryptedData.toString('base64'),
              fileHash: capsule.file_hash,
              encryptedHash: capsule.encrypted_hash,
              metadataHash: capsule.metadata_hash,
              originalSize: capsule.original_size,
              compressedSize: capsule.compressed_size
            };
            
            // Record the access
            capsuleModel.recordAccess(capsuleId, (err) => {
              if (err) {
                console.error('Failed to record access:', err);
              }
            });
            
            // Log the access
            const ip = req.ip || req.connection.remoteAddress;
            capsuleModel.logAccess(
              capsuleId, 
              ip, 
              req.get('User-Agent'), 
              'UNLOCK', 
              'SUCCESS'
            );
            
            res.json(response);
          } catch (error) {
            console.error('Error reading encrypted file:', error);
            
            // Log the failed access
            const ip = req.ip || req.connection.remoteAddress;
            capsuleModel.logAccess(
              capsuleId, 
              ip, 
              req.get('User-Agent'), 
              'UNLOCK', 
              'FAILED'
            );
            
            res.status(500).json({ error: 'Failed to read encrypted file' });
          }
        });
      });
    } catch (error) {
      console.error('Error unlocking capsule:', error);
      res.status(500).json({ error: 'Failed to unlock time capsule' });
    }
  }
}

module.exports = new UnlockController();