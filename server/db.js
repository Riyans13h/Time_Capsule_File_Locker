const Database = require('better-sqlite3');
const path = require('path');
const fs = require('fs');

// Ensure db directory exists
const dbDir = path.join(__dirname, 'db');
if (!fs.existsSync(dbDir)) {
    fs.mkdirSync(dbDir, { recursive: true });
}

const dbPath = process.env.DB_PATH || path.join(dbDir, 'capsules.sqlite');
const db = new Database(dbPath);

// Enable foreign keys and WAL mode for better performance
db.pragma('foreign_keys = ON');
db.pragma('journal_mode = WAL');

// Initialize tables
function initializeDatabase() {
    // Receivers table - stores receiver public keys
    db.prepare(`
        CREATE TABLE IF NOT EXISTS receivers (
            receiver_id TEXT PRIMARY KEY,
            public_key_pem TEXT NOT NULL,
            contact_email TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    `).run();

    // Capsules table - stores file metadata and encryption info
    db.prepare(`
        CREATE TABLE IF NOT EXISTS capsules (
            capsule_id TEXT PRIMARY KEY,
            sender_info TEXT,
            receiver_id TEXT NOT NULL,
            original_filename TEXT NOT NULL,
            encrypted_file_path TEXT NOT NULL,
            encrypted_key_path TEXT NOT NULL,
            file_size INTEGER,
            sha256_hash TEXT,
            release_time DATETIME NOT NULL,
            status TEXT DEFAULT 'pending',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            delivered_at DATETIME,
            ack_sent INTEGER DEFAULT 0,
            FOREIGN KEY (receiver_id) REFERENCES receivers (receiver_id)
        )
    `).run();

    // Create indexes for better performance
    db.prepare(`
        CREATE INDEX IF NOT EXISTS idx_capsules_release_time 
        ON capsules(release_time) WHERE status = 'pending'
    `).run();

    db.prepare(`
        CREATE INDEX IF NOT EXISTS idx_capsules_receiver_id 
        ON capsules(receiver_id)
    `).run();

    db.prepare(`
        CREATE INDEX IF NOT EXISTS idx_capsules_status 
        ON capsules(status)
    `).run();

    console.log('Database initialized successfully');
}

// Initialize the database
initializeDatabase();

module.exports = db;