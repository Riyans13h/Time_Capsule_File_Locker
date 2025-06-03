const express = require('express');
const cors = require('cors'); // Add this
const path = require('path');

const app = express();

// Enable CORS for all routes
app.use(cors());

// Add these middleware
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// API Routes
const lockRouter = require('./api/lock');
const unlockRouter = require('./api/unlock');
app.use('/api/lock', lockRouter);
app.use('/api/unlock', unlockRouter);

// Serve static files from web directory
app.use(express.static(path.join(__dirname, '../web')));

// Start server
const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});