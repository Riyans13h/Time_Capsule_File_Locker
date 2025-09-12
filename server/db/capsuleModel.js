const sqlite3 = require('sqlite3').verbose();
const path = require('path');

const dbPath = process.env.DB_PATH || './db/timecapsules.db';

class CapsuleModel {
  constructor() {
    this.db = new sqlite3.Database(dbPath);
  }

  // Create a new time capsule with SERVER-GENERATED ID
  createCapsule(capsuleData, callback) {
    const {
      capsuleId,
      originalFilename,
      encryptedFilename,
      unlockTime,
      fileHash,
      encryptedHash,
      metadataHash,
      originalSize,
      compressedSize,
      encryptedSize
    } = capsuleData;

    const query = `
      INSERT INTO time_capsules (
        capsule_id, original_filename, encrypted_filename, unlock_time,
        file_hash, encrypted_hash, metadata_hash,
        original_size, compressed_size, encrypted_size
      ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    `;

    const params = [
      capsuleId,
      originalFilename,
      encryptedFilename,
      unlockTime,
      fileHash,
      encryptedHash,
      metadataHash,
      originalSize,
      compressedSize,
      encryptedSize
    ];

    this.db.run(query, params, function(err) {
      if (err) {
        callback(err, null);
      } else {
        callback(null, { 
          capsuleId: capsuleId,
          originalFilename: originalFilename,
          unlockTime: unlockTime
        });
      }
    });
  }

  // Get capsule by ID
  getCapsule(capsuleId, callback) {
    const query = 'SELECT * FROM time_capsules WHERE capsule_id = ?';
    this.db.get(query, [capsuleId], (err, row) => {
      if (err) {
        callback(err, null);
      } else {
        callback(null, row);
      }
    });
  }

  // Check if unlock time has been reached
  isUnlockTimeReached(capsuleId, callback) {
    const query = `
      SELECT capsule_id, unlock_time, 
             datetime('now') >= unlock_time as is_unlocked
      FROM time_capsules 
      WHERE capsule_id = ?
    `;
    
    this.db.get(query, [capsuleId], (err, row) => {
      if (err) {
        callback(err, null);
      } else if (!row) {
        callback(new Error('Capsule not found'), null);
      } else {
        callback(null, {
          isUnlocked: Boolean(row.is_unlocked),
          unlockTime: row.unlock_time
        });
      }
    });
  }

  // Increment access count and update accessed_at
  recordAccess(capsuleId, callback) {
    const query = `
      UPDATE time_capsules 
      SET access_count = access_count + 1, accessed_at = datetime('now')
      WHERE capsule_id = ?
    `;
    
    this.db.run(query, [capsuleId], function(err) {
      if (err) {
        callback(err);
      } else {
        callback(null);
      }
    });
  }

  // Log access attempt
  logAccess(capsuleId, ip, userAgent, action, status) {
    const query = `
      INSERT INTO access_logs (capsule_id, ip_address, user_agent, action, status)
      VALUES (?, ?, ?, ?, ?)
    `;
    
    this.db.run(query, [capsuleId, ip, userAgent, action, status], (err) => {
      if (err) {
        console.error('Failed to log access:', err);
      }
    });
  }

  // Clean up old capsules
  cleanupExpiredCapsules(daysToKeep, callback) {
    const query = `
      DELETE FROM time_capsules 
      WHERE datetime(accessed_at) < datetime('now', ?)
      AND access_count > 0
    `;
    
    this.db.run(query, [`-${daysToKeep} days`], function(err) {
      if (err) {
        callback(err);
      } else {
        callback(null, this.changes);
      }
    });
  }

  // Close database connection
  close() {
    this.db.close();
  }
}

module.exports = CapsuleModel;