const express = require('express');
const router = express.Router();
const lockController = require('../controllers/lockController');

// Create a new time capsule
router.post('/', lockController.createCapsule);

// Get capsule metadata (without unlocking)
router.get('/:capsuleId', lockController.getCapsuleInfo);

module.exports = router;