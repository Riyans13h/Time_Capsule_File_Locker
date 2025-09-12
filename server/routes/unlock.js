const express = require('express');
const router = express.Router();
const unlockController = require('../controllers/unlockController');

// Unlock a time capsule
router.post('/:capsuleId', unlockController.unlockCapsule);

// Check if capsule can be unlocked
router.get('/:capsuleId/status', unlockController.checkUnlockStatus);

module.exports = router;