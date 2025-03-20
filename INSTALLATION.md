# StatTracker Installation Guide

This guide provides detailed instructions for both publishing StatTracker to the Workshop and installing it on your Arma Reforger server.

## Publishing to Workshop

### Prerequisites
- Arma Reforger Tools installed
- Basic understanding of the Workbench
- Arma Reforger game ownership

### Step-by-Step Publishing Process

1. **Open Arma Reforger Tools**
   - Launch Arma Reforger Tools from your Steam library

2. **Open the StatTracker Project**
   - Navigate to File → Open
   - Select the `StatTracker/addon.gproj` file
   - The project should load with all components

3. **Verify Project Content**
   - Ensure all scripts are correctly listed in the project
   - Check for any errors in the console window
   - Verify the preview image exists at `StatTracker/UI/Images/Preview.edds`
     - If missing, add your own 512x512 preview image in .edds format

4. **Build the Mod**
   - Select Build → Build Solution 
   - Wait for the build process to complete
   - Check for any build errors

5. **Configure Workshop Publishing**
   - Select Tools → Arma Reforger → Addon Publishing
   - In the Publishing window:
     - Title: StatTracker
     - Description: Copy the description from the mod.meta file
     - Version: 3.3.0
     - Visibility: Public (or as desired)
     - Tags: Statistics, Server, UI, Utility, Admin

6. **Upload to Workshop**
   - Click the "Publish" button
   - Sign in to your Steam account if prompted
   - Wait for the upload to complete

7. **Verify Workshop Listing**
   - Check that your mod appears on the Workshop
   - Verify all information is displayed correctly
   - Ensure the preview image is visible

## Server Installation

### Prerequisites
- Access to your Arma Reforger server files
- FTP or direct file access to the server
- Server administrator privileges

### Step-by-Step Server Installation

1. **Subscribe to the Mod**
   - Subscribe to StatTracker via the Steam Workshop
   - Note the Workshop ID (you'll need this later)

2. **Update Server Configuration**
   - Locate your server configuration file (typically `ArmaReforgerServer.json`)
   - Add StatTracker to the mods section:
   ```json
   "mods": [
     {
       "modId": "5c2c3d1a-b4d7-4e8f-b2c6-a9d6f4a27e1b", // StatTracker mod ID
       "name": "StatTracker",
       "version": "3.3.0",
       "settings": {}
     }
   ]
   ```

3. **Configure StatTracker Settings**
   - Customize settings within the "settings" object:
   ```json
   "settings": {
     "enableTeamKillTracking": true,
     "enableChatLogging": true,
     "scoreboardUpdateInterval": 5,
     "databaseType": "FILE",
     "defaultLanguage": "en",
     "enableRecoveryMode": true
   }
   ```

4. **Configure Server Launch Parameters**
   - Add the Workshop mod ID to your server launch parameters
   - Example: `-mod=1234567890` (replace with actual Workshop ID)

5. **Create Required Directories**
   - SSH or FTP into your server
   - Create the following directories:
     - `$profile:StatTracker/`
     - `$profile:StatTracker/Logs/`
     - `$profile:StatTracker/Backups/`
     - `$profile:StatTracker/Databases/`

6. **Restart Your Server**
   - Stop your Arma Reforger server
   - Start your server with the updated configuration

7. **Verify Installation**
   - Connect to your server as a client
   - Press TAB to check if the scoreboard appears
   - Type `/stats` in chat to see your statistics

8. **Monitor Server Logs**
   - Check the server logs for any StatTracker-related messages
   - Look for "StatTracker initialized successfully" message
   - Verify logs are being created in the `$profile:StatTracker/Logs/` directory

## Troubleshooting

### Common Issues

**Mod Not Loading**
- Verify the mod ID in your server configuration
- Check server logs for loading errors
- Ensure you're using the correct version number

**Scoreboard Not Appearing**
- Check that the Input.conf file is correctly referenced in mod.meta
- Verify key bindings in Arma Reforger settings
- Check console for script errors

**Database Errors**
- Ensure the StatTracker/Databases directory exists and is writable
- Check server logs for database connection errors
- Try changing the database type to "FILE" if using other types

**Performance Issues**
- Lower the update frequencies in the configuration
- Disable memory-intensive features like heatmap generation
- Enable memory optimization in settings

## Support

If you encounter any issues not covered by this guide, please:

1. Check the `$profile:StatTracker/Logs/` directory for error logs
2. Post on our support forum (link)
3. Include your server configuration and relevant log files 