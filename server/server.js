require('dotenv').config();
const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const path = require('path');
const fs = require('fs');

// Import routes
const lockRoutes = require('./routes/lock');
const unlockRoutes = require('./routes/unlock');

const app = express();
const PORT = process.env.PORT || 3000;

// Ensure directories exist
const directories = [
  process.env.UPLOAD_PATH,
  process.env.TEMP_PATH,
  path.dirname(process.env.DB_PATH)
];

for (const dir of directories) {
  if (!fs.existsSync(dir)) {
    fs.mkdirSync(dir, { recursive: true });
  }
}

// Middleware
app.use(cors());
app.use(bodyParser.json({ limit: '50mb' }));
app.use(bodyParser.urlencoded({ extended: true, limit: '50mb' }));

// Static files
app.use('/downloads', express.static(process.env.UPLOAD_PATH));

// Routes
app.use('/api/lock', lockRoutes);
app.use('/api/unlock', unlockRoutes);

// Health check endpoint
app.get('/api/health', (req, res) => {
  res.status(200).json({ 
    status: 'OK', 
    timestamp: new Date().toISOString(),
    service: 'Time Capsule File Locker API'
  });
});

// Error handling middleware
app.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).json({ 
    error: 'Something went wrong!',
    message: process.env.NODE_ENV === 'development' ? err.message : 'Internal server error'
  });
});

// 404 handler
app.use('*', (req, res) => {
  res.status(404).json({ error: 'Endpoint not found' });
});

app.listen(PORT, () => {
  console.log(`Time Capsule Server running on port ${PORT}`);
  console.log(`Environment: ${process.env.NODE_ENV}`);
});

module.exports = app;