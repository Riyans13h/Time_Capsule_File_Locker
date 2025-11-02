const cron = require('node-cron');
const db = require('../db');
const notifier = require('./notifier');

class Scheduler {
    constructor() {
        this.isRunning = false;
        this.init();
    }

    init() {
        // Run every minute to check for pending releases
        cron.schedule('* * * * *', () => {
            this.checkPendingReleases();
        });

        console.log('Scheduler initialized - checking for releases every minute');
    }

    async checkPendingReleases() {
        if (this.isRunning) {
            console.log('Scheduler already running, skipping...');
            return;
        }

        this.isRunning = true;
        
        try {
            const now = new Date().toISOString();
            
            // Find capsules that are due for release
            const dueCapsulesStmt = db.prepare(`
                SELECT 
                    capsule_id, receiver_id, sender_info, 
                    original_filename, release_time, contact_email
                FROM capsules c
                JOIN receivers r ON c.receiver_id = r.receiver_id
                WHERE c.status = 'pending' AND c.release_time <= ?
            `);
            
            const dueCapsules = dueCapsulesStmt.all(now);
            
            if (dueCapsules.length > 0) {
                console.log(`Found ${dueCapsules.length} capsules due for release`);
                
                for (const capsule of dueCapsules) {
                    await this.releaseCapsule(capsule);
                }
            }

        } catch (error) {
            console.error('Scheduler error:', error);
        } finally {
            this.isRunning = false;
        }
    }

    async releaseCapsule(capsule) {
        const transaction = db.transaction(() => {
            try {
                // Update capsule status to delivered
                const updateStmt = db.prepare(`
                    UPDATE capsules 
                    SET status = 'delivered', delivered_at = ?
                    WHERE capsule_id = ?
                `);
                
                updateStmt.run(new Date().toISOString(), capsule.capsule_id);

                console.log(`Capsule released: ${capsule.capsule_id} for receiver: ${capsule.receiver_id}`);

                // Notify receiver
                if (capsule.contact_email) {
                    notifier.notifyReceiver(
                        capsule.contact_email,
                        capsule.capsule_id,
                        capsule.original_filename
                    );
                }

                // Notify sender if callback URL provided
                if (capsule.sender_info && capsule.sender_info.startsWith('http')) {
                    notifier.notifySender(
                        capsule.sender_info,
                        capsule.capsule_id,
                        capsule.receiver_id
                    );
                }

                return true;
                
            } catch (error) {
                console.error(`Error releasing capsule ${capsule.capsule_id}:`, error);
                throw error;
            }
        });

        try {
            transaction();
        } catch (error) {
            console.error(`Transaction failed for capsule ${capsule.capsule_id}:`, error);
        }
    }

    // Manual release for testing/admin purposes
    async manualRelease(capsule_id) {
        try {
            const stmt = db.prepare(`
                SELECT 
                    capsule_id, receiver_id, sender_info, 
                    original_filename, contact_email
                FROM capsules c
                JOIN receivers r ON c.receiver_id = r.receiver_id
                WHERE c.capsule_id = ?
            `);
            
            const capsule = stmt.get(capsule_id);
            
            if (!capsule) {
                throw new Error(`Capsule not found: ${capsule_id}`);
            }

            await this.releaseCapsule(capsule);
            return { success: true, message: `Capsule ${capsule_id} released manually` };
            
        } catch (error) {
            console.error('Manual release error:', error);
            return { success: false, error: error.message };
        }
    }

    // Get scheduler status
    getStatus() {
        return {
            isRunning: this.isRunning,
            lastRun: new Date().toISOString()
        };
    }
}

module.exports = new Scheduler();