const sqlite3 = require('sqlite3').verbose();
const path = require('path');
const fs = require('fs');

const dbPath = process.env.DB_PATH || './db/timecapsules.db';

// Ensure the directory exists
const dbDir = path.dirname(dbPath);
if (!fs.existsSync(dbDir)) {
  fs.mkdirSync(dbDir, { recursive: true });
}

// Initialize database
const db = new sqlite3.Database(dbPath, (err) => {
  if (err) {
    console.error('Error opening database:', err.message);
    process.exit(1);
  }
  console.log('Connected to SQLite database.');
});

// Read and execute schema
const schemaPath = path.join(__dirname, 'schema.sql');
fs.readFile(schemaPath, 'utf8', (err, data) => {
  if (err) {
    console.error('Error reading schema file:', err);
    process.exit(1);
  }

  db.exec(data, (err) => {
    if (err) {
      console.error('Error executing schema:', err);
      process.exit(1);
    }
    console.log('Database schema initialized successfully.');
    db.close();
  });
});