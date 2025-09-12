const fs = require('fs');
const path = require('path');
const { promisify } = require('util');

const writeFileAsync = promisify(fs.writeFile);
const readFileAsync = promisify(fs.readFile);
const unlinkAsync = promisify(fs.unlink);
const existsAsync = promisify(fs.exists);

class FileUtils {
  constructor() {
    this.uploadPath = process.env.UPLOAD_PATH || './data/uploads';
    this.tempPath = process.env.TEMP_PATH || './data/temp';
  }

  // Generate a unique filename
  generateUniqueFilename(originalName) {
    const timestamp = Date.now();
    const randomStr = Math.random().toString(36).substring(2, 10);
    const extension = path.extname(originalName);
    const baseName = path.basename(originalName, extension);
    
    return `${baseName}_${timestamp}_${randomStr}${extension}`;
  }

  // Save file to uploads directory
  async saveFile(filename, data, isTemp = false) {
    const dir = isTemp ? this.tempPath : this.uploadPath;
    const filePath = path.join(dir, filename);
    
    try {
      await writeFileAsync(filePath, data);
      return filePath;
    } catch (error) {
      throw new Error(`Failed to save file: ${error.message}`);
    }
  }

  // Read file from uploads directory
  async readFile(filename, isTemp = false) {
    const dir = isTemp ? this.tempPath : this.uploadPath;
    const filePath = path.join(dir, filename);
    
    try {
      const data = await readFileAsync(filePath);
      return data;
    } catch (error) {
      throw new Error(`Failed to read file: ${error.message}`);
    }
  }

  // Delete file
  async deleteFile(filename, isTemp = false) {
    const dir = isTemp ? this.tempPath : this.uploadPath;
    const filePath = path.join(dir, filename);
    
    try {
      if (await existsAsync(filePath)) {
        await unlinkAsync(filePath);
        return true;
      }
      return false;
    } catch (error) {
      console.error(`Failed to delete file: ${error.message}`);
      return false;
    }
  }

  // Clean up temporary files older than specified hours
  async cleanupTempFiles(hours = 24) {
    try {
      const files = await fs.promises.readdir(this.tempPath);
      const now = Date.now();
      const expirationTime = hours * 60 * 60 * 1000;
      
      for (const file of files) {
        const filePath = path.join(this.tempPath, file);
        const stats = await fs.promises.stat(filePath);
        
        if (now - stats.mtimeMs > expirationTime) {
          await this.deleteFile(file, true);
        }
      }
    } catch (error) {
      console.error('Error cleaning up temp files:', error);
    }
  }

  // Get file information
  async getFileInfo(filename, isTemp = false) {
    const dir = isTemp ? this.tempPath : this.uploadPath;
    const filePath = path.join(dir, filename);
    
    try {
      const stats = await fs.promises.stat(filePath);
      return {
        size: stats.size,
        createdAt: stats.birthtime,
        modifiedAt: stats.mtime
      };
    } catch (error) {
      throw new Error(`Failed to get file info: ${error.message}`);
    }
  }
}

module.exports = FileUtils;