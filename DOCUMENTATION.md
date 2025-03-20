# StatTracker Documentation

## Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Core Features](#core-features)
   - [Statistics Tracking](#statistics-tracking)
   - [Real-time Display](#real-time-display)
   - [Database System](#database-system)
   - [Team Kill Tracking](#team-kill-tracking)
   - [Vote Kick Anti-Abuse](#vote-kick-anti-abuse)
   - [Chat Logging](#chat-logging)
   - [Analytics Dashboard](#analytics-dashboard)
   - [Localization System](#localization-system)
   - [Performance Optimization](#performance-optimization)
   - [Session-based Logging](#session-based-logging)
   - [Error Recovery System](#error-recovery-system)
4. [User Guide](#user-guide)
   - [Player HUD](#player-hud)
   - [Scoreboard](#scoreboard)
   - [Commands](#commands)
   - [Language Settings](#language-settings)
5. [Admin Guide](#admin-guide)
   - [RCON Commands](#rcon-commands)
   - [Web Dashboard](#web-dashboard)
   - [Log Files](#log-files)
   - [Configuration](#configuration)
   - [Performance Monitoring](#performance-monitoring)
   - [Language Management](#language-management)
6. [Troubleshooting](#troubleshooting)
7. [FAQ](#faq)
8. [Support](#support)

## Introduction

StatTracker is a comprehensive player statistics tracking system for Arma Reforger. It records detailed player performance metrics, logs game events, and provides both in-game and external tools for viewing and analyzing this data. The mod is designed to work alongside Arma Reforger's built-in systems without interference.

## Installation

### Player Installation

1. Open the Arma Reforger Workshop
2. Search for "StatTracker"
3. Subscribe to the mod
4. Launch Arma Reforger
5. In the launcher, ensure StatTracker is enabled in the mod list
6. Start the game and join any server that has the mod enabled

### Server Installation

1. Subscribe to the mod in the Arma Reforger Workshop
2. Configure your server to load the StatTracker mod
3. (Optional) Edit the server configuration to customize StatTracker settings:
   ```
   "mods": [
     {
       "modId": "5c2c3d1a-b4d7-4e8f-b2c6-a9d6f4a27e1b", // StatTracker mod ID
       "name": "StatTracker",
       "version": "3.1.0",
       "settings": {
         "enableWebDashboard": true,
         "dashboardPort": 8080,
         "enableDetailedLogging": true,
         "maxVoteKicksPerPlayer": 3,
         "voteKickCooldown": 300,
         "language": "en",
         "enablePerformanceMonitoring": true,
         "performanceLogInterval": 300,
         "maxCachedPlayers": 500
       }
     }
   ]
   ```
4. Restart your server to apply changes

## Core Features

### Statistics Tracking

StatTracker records a wide range of player statistics during gameplay:

- **Kills**: Player kills with details about weapon used
- **Deaths**: Including who killed you and with what weapon
- **Team Information**: Tracking which team each player belongs to
- **Objectives**: Bases captured and lost
- **AI Interactions**: AI kills
- **Vehicle Usage**: Vehicle and aircraft kills
- **Logistics**: Supplies delivered
- **Session Data**: Connection times, IP addresses, and playtime

All statistics are tracked in real-time and are viewable through the in-game interface.

### Real-time Display

StatTracker provides two primary display options:

1. **Mini-HUD**: A small, unobtrusive display in the top-right corner showing basic personal stats:
   - Current rank
   - Total XP
   - Kills
   - Deaths

2. **Full Scoreboard**: A comprehensive scoreboard showing all players and their stats:
   - Toggled with the TAB key
   - Sortable by score
   - Shows detailed stats for all players
   - Highlights the local player for easy identification

### Database System

StatTracker uses a robust database system to store and manage player statistics:

- **Data Storage**: Persistent storage of player stats and session data
- **Query Performance**: Optimized for fast data retrieval and analysis
- **Data Integrity**: Ensures data consistency and accuracy
- **Backup and Restore**: Automated backup and restore capabilities

### Team Kill Tracking

The system monitors and records all team kills:

- Records who killed whom and with what weapon
- Logs team killing incidents with timestamps
- Can be configured to automatically warn, kick, or ban excessive team killers
- Admin review system for team kill history
- Team kill notifications to all players or just to admins

### Vote Kick Anti-Abuse

Prevents abuse of the vote kick system:

- Logs who initiated each vote kick and their reason
- Records all votes cast (for and against) with player IDs
- Limits the number of vote kicks a player can initiate per session (default: 3)
- Enforces a cooldown period between vote kicks (default: 5 minutes)
- Provides admins with complete vote kick history
- Helps identify patterns of targeted harassment

### Chat Logging

Comprehensive chat logging system:

- Logs all in-game chat messages to dedicated files
- Records chat channel (ALL, TEAM, GROUP, VEHICLE, DIRECT)
- Timestamps and player identification (name and ID)
- Searchable by player or content
- Available to admins for moderation purposes
- In-memory storage for quick access to recent messages

### Analytics Dashboard

Advanced data visualization and analytics:

- Interactive heatmaps showing player activity locations
- Player retention metrics and trends
- Performance statistics over time
- Custom filtering and data export options
- Real-time server monitoring dashboard
- Secure web interface for admin access

### Localization System

StatTracker features a comprehensive localization system that supports multiple languages:

- **Supported Languages**: 
  - English (en) - Default
  - French (fr)
  - German (de)
  - Spanish (es)
  - Russian (ru)

- **Dynamic Text Formatting**: 
  - Uses placeholder system for dynamic content
  - Automatic formatting of numbers, dates, and times

- **Fallback Mechanism**:
  - Automatically falls back to English for missing translations
  - Prevents UI issues with incomplete translations

- **JSON-based Translation Files**:
  - Stored in `$profile:StatTracker/Localization/`
  - Easy to edit or add new languages

- **Integration**:
  - All UI components use the localization system
  - Error messages are localized
  - Admin notifications are localized

### Performance Optimization

StatTracker includes extensive performance optimization systems:

- **Memory Management**:
  - Automatic cleanup of unused objects
  - Configurable cache sizes to prevent memory bloat
  - Periodic garbage collection on long-running servers

- **Performance Monitoring**:
  - Real-time tracking of component performance
  - Detailed metrics for operations (min/avg/max execution times)
  - Identification of performance bottlenecks

- **Resource Usage Reports**:
  - Periodic logging of memory usage
  - CPU utilization tracking
  - Data transfer metrics

- **Hot-reloadable Configuration**:
  - Change settings without server restart
  - Dynamic adjustment based on server load
  - RCON commands for real-time configuration

### Session-based Logging

StatTracker implements a comprehensive session-based logging system:

- **Session Organization**:
  - Each game session creates a unique directory with timestamp and ID
  - Example: `$profile:StatTracker/Logs/Session_2024-09-20_12-34-56_1234/`
  - Clear separation between different server runs

- **Severity-based Log Files**:
  - `Debug_Server_timestamp.log` - Detailed development information
  - `Info_Server_timestamp.log` - General operation information
  - `Warning_Server_timestamp.log` - Potential issues requiring attention
  - `Error_Server_timestamp.log` - Error conditions requiring investigation
  - `Critical_Server_timestamp.log` - Severe errors affecting system stability

- **Specialized Log Files**:
  - `Chat_Server_timestamp.log` - All in-game chat communications
  - `VoteKick_Server_timestamp.log` - Vote kick activity and results
  - `Performance_Server_timestamp.log` - Performance metrics in CSV format

- **Log Management**:
  - Automatic log rotation when files exceed configured size limit
  - Retention policy to avoid excessive disk usage
  - Old log cleanup based on maximum count settings

- **Diagnostic Context**:
  - Detailed timestamps on all log entries
  - Component and method information for better context
  - Stack traces for error conditions
  - Structured format for easier parsing and analysis

### Error Recovery System

StatTracker includes a robust error recovery system to ensure reliable operation even during unexpected conditions:

### Comprehensive Error Handling

All components implement a consistent error handling approach:

- **Rich Error Context**: Each error includes detailed contextual information to aid troubleshooting
- **Error Categorization**: Errors are categorized for better tracking and analysis
- **Error Statistics**: System tracks error frequency by category to detect patterns
- **Detailed Logging**: Comprehensive logging of all errors with stack traces when enabled
- **Error Recovery**: Automatic recovery from transient failures

Example contextual error information:
```
[ERROR] [DATABASE] Failed to execute query
Context:
  operation_type: SavePlayerStats
  player_id: 12345
  attempts: 2
  last_error: CONNECTION_LOST
Stack trace:
  at STS_DatabaseManager::ExecuteQuery (line 312)
  at STS_DatabaseManager::SavePlayerStats (line 450)
  at STS_PersistenceManager::SavePlayer (line 178)
```

### Recovery Mechanisms

The system employs several levels of recovery:

1. **Component-level Recovery**: Each component implements self-healing capabilities
   - Configurable retry attempts with exponential backoff
   - State tracking to ensure consistent recovery
   - Automatic resource cleanup after failures

2. **System-level Recovery**: Coordination between components during failures
   - Component health monitoring
   - Dependency management during recovery
   - Cascading recovery for interdependent components

3. **Database Recovery**: Specialized handling for database failures
   - Transaction support for atomic operations
   - Memory-based failover during database outages
   - Operation queuing and replay after reconnection
   - Integrity validation after recovery

4. **Server Recovery**: Protection against server-level issues
   - Emergency state saving during crashes
   - Disk space monitoring and management
   - Resource usage monitoring and optimization

### Graceful Degradation

When full recovery isn't possible, the system gracefully degrades functionality:

- **Feature Disabling**: Non-critical features are temporarily disabled
- **Read-only Mode**: Falls back to read-only operation when writing fails
- **Caching**: Uses memory caching when database access is limited
- **Minimal Operation**: Maintains core tracking even when other systems fail
- **User Notification**: Clear messaging when features are temporarily unavailable

### Health Monitoring

Continuous health monitoring to detect and prevent issues:

- **Component Status Tracking**: Real-time status of all system components
- **Automatic Health Checks**: Periodic validation of system integrity
- **Performance Monitoring**: Detection of performance degradation
- **Resource Usage**: Tracking of memory, file handles, and other resources
- **Predictive Recovery**: Proactive intervention before critical failures

### Error Configuration

Administrators can customize error handling behavior:

```json
{
  "errorHandling": {
    "enableDetailedErrorContext": true,
    "maxErrorsPerCategory": 1000,
    "errorContextRetention": 10,
    "logStackTraces": true,
    "enableRecoveryMode": true,
    "maxRecoveryAttempts": 3,
    "recoveryAttemptDelay": 60,
    "failureThresholdPerMinute": 5,
    "minTimeBetweenRecoveries": 300,
    "enableMemoryBackup": true,
    "fallbackModeEnabled": true,
    "notifyAdminOnCritical": true
  }
}
```

### Admin Tools

Administrators have access to specialized tools for managing system health:

- **Health Dashboard**: Real-time view of all component statuses
- **Error Statistics**: Detailed breakdown of error frequency by category and time
- **Manual Recovery**: Commands to trigger recovery procedures
- **Integrity Validation**: Tools to verify and repair data integrity
- **Log Analysis**: Advanced filtering and searching of error logs

Commands:
- `/system_health` - View overall system health status
- `/component_health` - View individual component health
- `/error_stats` - View error statistics by category
- `/recovery_mode` - Manually toggle recovery mode
- `/validate_integrity` - Check and repair data integrity

### Enhanced Logging for Troubleshooting

The session-based logging system provides comprehensive information for troubleshooting:

- **Context-Rich Entries**: All log entries include relevant context information
- **Session Organization**: Logs are organized by gaming session for easier correlation
- **Severity Separation**: Different log files for each severity level
- **Specialized Logs**: Dedicated logs for chat, vote kicks, and performance
- **Automatic Rotation**: Size-based log rotation to prevent excessive disk usage
- **Retention Management**: Automatic cleanup of old log files based on policy

Example log entry with rich context:
```
[2024-10-15 14:23:45] [INFO] [DATABASE] [STS_DatabaseManager::SavePlayerStats] Player stats saved successfully
Context:
  player_id: 12345
  player_name: JohnDoe
  operation_duration_ms: 45
  retry_count: 0
  data_size_bytes: 2048
```

## User Guide

### Player HUD

The mini-HUD is visible in the top-right corner of the screen during gameplay. It shows:

- **Rank**: Your current rank in the server
- **XP**: Total experience points earned
- **Kills**: Number of player kills
- **Deaths**: Number of times you've died

The HUD is designed to be informative without being intrusive to gameplay.

### Scoreboard

To view the full scoreboard:

1. Press the TAB key during gameplay
2. The scoreboard will appear, showing all players and their stats
3. Your row will be highlighted in yellow for easy identification
4. Press TAB again to hide the scoreboard

The scoreboard displays:
- Player Name
- Rank
- Score (calculated from various actions)
- Kills (players)
- Deaths
- Bases Captured
- Bases Lost
- AI Kills
- Vehicle Kills
- Supplies Delivered

### Commands

Players can use the following in-game chat commands:

- `/stats` - Show your personal stats in chat
- `/stats [playername]` - Show stats for another player
- `/top10` - Show the top 10 players by score
- `/playtime` - Show your total playtime on the server

### Language Settings

Players can change their preferred language:

1. Open the options menu (ESC key)
2. Navigate to the StatTracker settings tab
3. Select your preferred language from the dropdown menu
4. Changes take effect immediately without restart

### Log Files

StatTracker generates several log files to help with troubleshooting:

- **Session Directory Structure**:
  ```
  $profile:StatTracker/
  └── Logs/
      ├── Session_2024-09-20_12-34-56_1234/
      │   ├── Debug_Server_20240920_123456.log
      │   ├── Info_Server_20240920_123456.log
      │   ├── Warning_Server_20240920_123456.log
      │   ├── Error_Server_20240920_123456.log
      │   ├── Critical_Server_20240920_123456.log
      │   ├── Chat_Server_20240920_123456.log
      │   ├── VoteKick_Server_20240920_123456.log
      │   └── Performance_Server_20240920_123456.log
      └── Session_2024-09-21_10-11-12_5678/
          └── ...
  ```

- **Log Levels**:
  - **DEBUG**: Detailed information for development and troubleshooting
  - **INFO**: General operational messages and status updates
  - **WARNING**: Potential issues that don't affect core functionality
  - **ERROR**: Problems requiring admin attention
  - **CRITICAL**: Severe issues that may affect system stability

- **Log Rotation**:
  When a log file reaches the configured size limit (default: 10MB), it's automatically rotated:
  ```
  Debug_Server_20240920_123456.log → Debug_Server_20240920_123456.log.20240920_130000
  ```

- **Emergency Backups**:
  During database outages, the system creates emergency backups:
  ```
  $profile:StatTracker/EmergencyBackups/backup_2024-09-20_13-00-00.json
  ```

- **Log Format**:
  Example from Info log:
  ```
  [2024-09-20 12:34:56] [INFO] STS_Main::Initialize() - StatTracker system initializing
  ```
  
  Example from Error log:
  ```
  [2024-09-20 12:35:10] [ERROR] STS_DatabaseManager::Connect() - Failed to connect to database: Connection timeout
    StackTrace: at STS_DatabaseManager::Connect line 235
  ```

## Admin Guide

### RCON Commands

Administrators can use these RCON commands to manage the system:

#### Stats Management

- `stats_show <playerID>` - Show detailed stats for a specific player
- `stats_reset <playerID>` - Reset a player's stats to zero
- `stats_export` - Export all player stats to CSV
- `stats_load` - Force reload stats from disk

#### Team Kill Management

- `tk_check <playerID>` - Check team kill count for a player
- `tk_clear <playerID>` - Clear team kill history for a player
- `tk_list` - List recent team kills
- `tk_config_set <warnCount> <kickCount> <banCount>` - Configure auto-punishments

#### Vote Kick Management

- `votekick_history <playerID>` - Show vote kick history for a player
- `votekick_stats` - Show overall vote kick statistics
- `votekick_config_set <maxPerPlayer> <cooldownSeconds>` - Configure limits

#### Chat Management

- `chat_history <playerID>` - Show chat history for a player
- `chat_search <term>` - Search chat logs for a specific term
- `chat_broadcast <message>` - Send a message to all players

#### System Management

- `stats_system_info` - Show system status and memory usage
- `stats_debugging <level>` - Set debugging level (0-4)
- `stats_restart` - Restart the StatTracker system

### Web Dashboard

Access the web dashboard at `http://your-server-ip:8080/admin`:

1. Login using your admin credentials
2. Navigate the dashboard sections:
   - **Overview**: Server health, player count, and system status
   - **Players**: Detailed player statistics with filtering and sorting
   - **Heatmaps**: Activity visualizations across the map
   - **Team Kills**: Team kill incidents with filtering options
   - **Vote Kicks**: Vote kick history and analysis
   - **Chat Logs**: Searchable chat history
   - **Reports**: Generate custom reports and export data
   - **Settings**: Configure system parameters

### Log Files

The mod generates detailed logs in the `$profile:StatTracker/Logs/` directory:

- `StatTracker_[timestamp].log` - Main system log with all events
- `ChatLog_[timestamp].log` - Log of all in-game chat messages
- `VoteKicks_[timestamp].log` - Log of all vote kick activities

Log levels:
- DEBUG: Detailed information for debugging purposes
- INFO: General information about system operation
- WARNING: Warning conditions that should be addressed
- ERROR: Error conditions that prevent features from working
- CRITICAL: Critical conditions that may cause system failure

### Configuration

Server admins can configure StatTracker through the `config.json` file:

```json
{
  "general": {
    "enabled": true,
    "debugLevel": 1
  },
  "storage": {
    "autoSaveInterval": 60,
    "exportEnabled": true
  },
  "teamKills": {
    "warnThreshold": 2,
    "kickThreshold": 3,
    "banThreshold": 5,
    "forgiveTimeMinutes": 60
  },
  "voteKick": {
    "maxPerPlayer": 3,
    "cooldownSeconds": 300,
    "loggingEnabled": true
  },
  "webDashboard": {
    "enabled": true,
    "port": 8080,
    "requireAuth": true
  }
}
```

### Performance Monitoring

Server administrators can monitor and optimize performance:

- **Access Performance Logs**:
  - Located in `$profile:StatTracker/Logs/Performance/`
  - Contains detailed metrics on component performance
  - Updated according to the configured interval (default: 5 minutes)

- **RCON Commands for Performance**:
  - `perf_stats` - Shows current performance statistics
  - `mem_stats` - Shows current memory usage statistics
  - `mem_cleanup` - Forces an immediate memory cleanup

- **Performance Dashboard**:
  - Access via the web dashboard at `/performance`
  - Real-time charts and graphs
  - Historical performance data

- **Optimization Suggestions**:
  - The system will provide recommendations based on performance data
  - Suggestions for configuration changes appear in the logs
  - Critical performance issues trigger admin notifications

### Language Management

Server administrators can manage languages:

- **Set Default Language**:
  - Configure default language in server settings
  - All players will use this language by default

- **Add Custom Translations**:
  - Create or edit JSON files in `$profile:StatTracker/Localization/`
  - Follow the format of existing language files
  - New languages are automatically detected on restart

- **Translation Validation**:
  - Missing translations are logged
  - Automatic validation of translation files
  - Compatibility with special characters and different alphabets

## Troubleshooting

### Common Issues

**Q: The scoreboard doesn't appear when I press TAB**
A: Check your key bindings to ensure the TAB key is bound to the "STS_ToggleScoreboard" action. You can rebind this in the game's control settings.

**Q: My stats aren't being saved between sessions**
A: Ensure you have sufficient disk space and the StatTracker mod is correctly loaded. Check the logs for any file I/O errors.

**Q: The web dashboard isn't accessible**
A: Verify that the dashboard is enabled in your configuration and the specified port is open in your firewall/network.

**Q: Some players' names don't show up correctly in the scoreboard**
A: This can happen with special characters in names. Update to the latest version of the mod which includes improved character handling.

### Log Locations

- Main system logs: `$profile:StatTracker/Logs/StatTracker_[timestamp].log`
- Chat logs: `$profile:StatTracker/Logs/ChatLog_[timestamp].log`
- Vote kick logs: `$profile:StatTracker/Logs/VoteKicks_[timestamp].log`

### Reporting Bugs

If you encounter a bug, please:

1. Check the logs for error messages
2. Take a screenshot if applicable
3. Note what steps led to the issue
4. Report the issue on our GitHub repository with all relevant information

## FAQ

**Q: Does StatTracker affect game performance?**
A: The mod is optimized for minimal performance impact. Most players won't notice any difference in FPS or gameplay.

**Q: Can I use StatTracker alongside other mods?**
A: Yes, StatTracker is designed to be compatible with most other mods. It doesn't modify core game files.

**Q: Will my stats transfer between different servers?**
A: No, stats are server-specific and are stored in that server's profile directory.

**Q: Does StatTracker interfere with the game's built-in progression system?**
A: No, StatTracker is designed to operate alongside the game's built-in systems without interference. It tracks statistics separately.

**Q: Can I customize what statistics are tracked?**
A: Server administrators can configure which statistics are tracked and displayed through the configuration file.

**Q: Is StatTracker compatible with all game modes?**
A: Yes, StatTracker works with all standard game modes and most custom game modes.

## Support

For technical support:

- GitHub Issues: [link to be added]
- Discord: [link to be added]
- Email: [email to be added]

We aim to respond to all support requests within 48 hours. 