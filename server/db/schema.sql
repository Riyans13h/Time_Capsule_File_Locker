-- Time Capsules table
CREATE TABLE IF NOT EXISTS time_capsules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    capsule_id TEXT UNIQUE NOT NULL,
    original_filename TEXT NOT NULL,
    encrypted_filename TEXT NOT NULL,
    unlock_time DATETIME NOT NULL,
    file_hash TEXT NOT NULL,
    encrypted_hash TEXT NOT NULL,
    metadata_hash TEXT NOT NULL,
    original_size INTEGER NOT NULL,
    compressed_size INTEGER NOT NULL,
    encrypted_size INTEGER NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    accessed_at DATETIME,
    access_count INTEGER DEFAULT 0
);

-- Access logs table
CREATE TABLE IF NOT EXISTS access_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    capsule_id TEXT NOT NULL,
    ip_address TEXT,
    user_agent TEXT,
    action TEXT NOT NULL,
    status TEXT NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (capsule_id) REFERENCES time_capsules (capsule_id)
);

-- Create indexes for better performance
CREATE INDEX IF NOT EXISTS idx_capsule_id ON time_capsules (capsule_id);
CREATE INDEX IF NOT EXISTS idx_unlock_time ON time_capsules (unlock_time);
CREATE INDEX IF NOT EXISTS idx_access_logs ON access_logs (capsule_id, timestamp);