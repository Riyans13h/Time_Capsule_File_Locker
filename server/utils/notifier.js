const nodemailer = require('nodemailer');

class Notifier {
    constructor() {
        this.transporter = null;
        this.init();
    }

    init() {
        // Create transporter if email configuration is available
        if (process.env.SMTP_HOST && process.env.SMTP_USER && process.env.SMTP_PASS) {
            this.transporter = nodemailer.createTransporter({
                host: process.env.SMTP_HOST,
                port: process.env.SMTP_PORT || 587,
                secure: false,
                auth: {
                    user: process.env.SMTP_USER,
                    pass: process.env.SMTP_PASS
                }
            });
            console.log('Email notifier initialized');
        } else {
            console.log('Email configuration not found - email notifications disabled');
        }
    }

    async notifyReceiver(email, capsuleId, filename) {
        if (!this.transporter) {
            console.log(`Email notification skipped for capsule ${capsuleId} - no email config`);
            return;
        }

        try {
            const mailOptions = {
                from: process.env.SMTP_USER,
                to: email,
                subject: 'Time Capsule File Available',
                html: `
                    <h2>Time Capsule File Ready</h2>
                    <p>Your time capsule file is now available for download.</p>
                    <ul>
                        <li><strong>Capsule ID:</strong> ${capsuleId}</li>
                        <li><strong>Filename:</strong> ${filename}</li>
                        <li><strong>Released:</strong> ${new Date().toLocaleString()}</li>
                    </ul>
                    <p>Please visit the receiver portal to download and decrypt your file.</p>
                    <hr>
                    <p><em>This is an automated message from the Time Capsule File Locker system.</em></p>
                `
            };

            await this.transporter.sendMail(mailOptions);
            console.log(`Notification email sent to receiver: ${email} for capsule: ${capsuleId}`);

        } catch (error) {
            console.error('Failed to send receiver notification:', error);
        }
    }

    async notifySender(callbackUrl, capsuleId, receiverId) {
        try {
            // For now, we'll just log the callback
            // In production, you might want to use axios or similar to make the HTTP call
            console.log(`Sender notification - Callback URL: ${callbackUrl}`);
            console.log(`Capsule ${capsuleId} delivered to receiver ${receiverId}`);
            
            // Example implementation with axios (commented out):
            /*
            const axios = require('axios');
            await axios.post(callbackUrl, {
                capsule_id: capsuleId,
                receiver_id: receiverId,
                status: 'delivered',
                delivered_at: new Date().toISOString()
            }, { timeout: 5000 });
            */

        } catch (error) {
            console.error('Failed to send sender notification:', error);
        }
    }

    // Test email configuration
    async testEmail() {
        if (!this.transporter) {
            return { success: false, error: 'Email transporter not configured' };
        }

        try {
            await this.transporter.verify();
            return { success: true, message: 'Email configuration is valid' };
        } catch (error) {
            return { success: false, error: error.message };
        }
    }
}

module.exports = new Notifier();