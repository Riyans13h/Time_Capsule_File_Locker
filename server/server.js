require('dotenv').config();
const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const path = require('path');
const fs = require('fs');

// Import routes
const publicKeyRoutes = require('./routes/publicKey');
const uploadRoutes = require('./routes/upload');
const releaseRoutes = require('./routes/release');

// Import utilities
const scheduler = require('./utils/scheduler');
const notifier = require('./utils/notifier');

// Import database (initializes on require)
const db = require('./db');

class TimeCapsuleServer {
    constructor() {
        this.app = express();
        this.port = process.env.PORT || 3000;
        this.init();
    }

    init() {
        // Create necessary directories
        this.createDirectories();
        
        // Initialize middleware
        this.setupMiddleware();
        
        // Initialize routes
        this.setupRoutes();
        
        // Initialize health check
        this.setupHealthCheck();
        
        // Initialize error handling
        this.setupErrorHandling();
    }

    createDirectories() {
        const directories = [
            path.join(__dirname, 'storage', 'files'),
            path.join(__dirname, 'storage', 'keys'),
            path.join(__dirname, 'db')
        ];

        directories.forEach(dir => {
            if (!fs.existsSync(dir)) {
                fs.mkdirSync(dir, { recursive: true });
                console.log(`Created directory: ${dir}`);
            }
        });
    }

    setupMiddleware() {
        // CORS configuration
        this.app.use(cors({
            origin: process.env.CLIENT_URL || 'http://localhost:8080',
            credentials: true,
            methods: ['GET', 'POST', 'PUT', 'DELETE'],
            allowedHeaders: ['Content-Type', 'Authorization']
        }));

        // Body parsing middleware
        this.app.use(bodyParser.json({ limit: '10mb' }));
        this.app.use(bodyParser.urlencoded({ extended: true, limit: '10mb' }));

        // Request logging middleware
        this.app.use((req, res, next) => {
            console.log(`${new Date().toISOString()} - ${req.method} ${req.path}`);
            next();
        });
    }

    setupRoutes() {
        // API routes
        this.app.use('/api/publickey', publicKeyRoutes);
        this.app.use('/api/upload', uploadRoutes);
        this.app.use('/api/release', releaseRoutes);

        // Serve static files from storage (for downloads)
        this.app.use('/storage', express.static(path.join(__dirname, 'storage')));
    }

    setupHealthCheck() {
        // Health check endpoint
        this.app.get('/health', (req, res) => {
            const health = {
                status: 'healthy',
                timestamp: new Date().toISOString(),
                uptime: process.uptime(),
                memory: process.memoryUsage(),
                database: 'connected', // Since db.js initializes on require
                scheduler: scheduler.getStatus()
            };

            res.json(health);
        });

        // Root endpoint
        this.app.get('/', (req, res) => {
            res.json({
                message: 'Time Capsule File Locker Server',
                version: '1.0.0',
                endpoints: {
                    publicKey: '/api/publickey',
                    upload: '/api/upload',
                    release: '/api/release',
                    health: '/health'
                },
                documentation: 'See README for API documentation'
            });
        });
    }

    setupErrorHandling() {
        // 404 handler
        this.app.use('*', (req, res) => {
            res.status(404).json({
                error: 'Endpoint not found',
                path: req.originalUrl,
                method: req.method
            });
        });

        // Global error handler
        this.app.use((error, req, res, next) => {
            console.error('Global error handler:', error);
            
            res.status(error.status || 500).json({
                error: 'Internal server error',
                details: process.env.NODE_ENV === 'development' ? error.message : 'Something went wrong'
            });
        });
    }

    start() {
        this.server = this.app.listen(this.port, () => {
            console.log('='.repeat(60));
            console.log('🚀 Time Capsule File Locker Server');
            console.log('='.repeat(60));
            console.log(`📍 Server running on port: ${this.port}`);
            console.log(`🌍 Environment: ${process.env.NODE_ENV || 'development'}`);
            console.log(`📊 Database: ${process.env.DB_PATH || './db/capsules.sqlite'}`);
            console.log(`📧 Email notifications: ${notifier.transporter ? 'Enabled' : 'Disabled'}`);
            console.log('='.repeat(60));
            
            // Test email configuration if available
            if (notifier.transporter) {
                notifier.testEmail().then(result => {
                    if (result.success) {
                        console.log('✅ Email configuration test: PASSED');
                    } else {
                        console.log('❌ Email configuration test: FAILED -', result.error);
                    }
                });
            }
        });

        // Graceful shutdown
        process.on('SIGTERM', this.gracefulShutdown.bind(this));
        process.on('SIGINT', this.gracefulShutdown.bind(this));
    }

    gracefulShutdown() {
        console.log('\n🛑 Received shutdown signal...');
        
        if (this.server) {
            this.server.close(() => {
                console.log('✅ HTTP server closed');
                
                // Close database connection
                if (db && db.close) {
                    db.close();
                    console.log('✅ Database connection closed');
                }
                
                console.log('👋 Time Capsule Server shutdown complete');
                process.exit(0);
            });
        }
    }
}

// Create and start the server
const server = new TimeCapsuleServer();
server.start();

module.exports = server;