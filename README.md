# StatTracker for Arma Reforger

[![Version](https://img.shields.io/badge/Version-3.3.0-blue.svg)](https://github.com/yourusername/stattracker)
[![License](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](http://creativecommons.org/licenses/by-nc-sa/4.0/)

A comprehensive statistics tracking system for Arma Reforger servers.

## Features

- **Player Statistics Tracking**: Track kills, deaths, captures, and more
- **Persistent Storage**: Player stats are saved between sessions
- **Real-time Display**: View stats in-game through customizable UI elements
- **Admin Dashboard**: Advanced administration tools and reports
- **RCON Integration**: Control and query the system via RCON commands
- **Team Kill Monitoring**: Track and manage team kill incidents
- **Vote Kick Tracking**: Monitor vote kick usage to prevent abuse
- **Chat Logging**: Record and search through in-game chat
- **Multi-language Support**: Full localization with support for English, French, German, Spanish, and Russian
- **Memory Management**: Optimized memory usage for long server sessions
- **Performance Monitoring**: Real-time performance statistics and optimization
- **Hot-reloadable Configuration**: Modify settings on the fly without server restarts
- **Session-specific Logging**: Separate log files for each gaming session with severity-based organization
- **Enhanced Error Recovery**: Automatic recovery from errors with graceful component degradation
- **Database Failover System**: Memory backup and emergency storage during database outages
- **Comprehensive Error Handling**: Rich error context, automatically retries, and graceful degradation
- **Transaction Support**: Database operations are executed within transactions for data integrity
- **Webhook Integration**: Real-time notifications to external services with validation and security
- **Multi-server Synchronization**: Statistics sharing between server instances

## Screenshots

![Scoreboard](https://example.com/scoreboard.jpg)
![Admin Dashboard](https://example.com/dashboard.jpg)
![Stats Display](https://example.com/stats.jpg)
![Performance Monitor](https://example.com/performance.jpg)

## Requirements

- Arma Reforger (Latest Version)
- Minimum 2GB additional RAM for servers with many players
- Optional: MongoDB for advanced database features

## Installation

### Client Installation

1. Subscribe to the mod on the Arma Reforger Workshop
2. Enable the mod in the launcher
3. Join a server that has the mod installed

### Server Installation

1. Subscribe to the mod on the Arma Reforger Workshop
2. Add the mod to your server's startup parameters
3. Configure the settings in `$profile:StatTracker/config.json`
4. Select your preferred database mode:
   - `FILE`: Simple file-based storage (default)
   - `BINARY`: Optimized binary storage
   - `MONGODB`: MongoDB integration (requires MongoDB)
   - `AUTO`: Automatically select the best option based on player count

## Configuration

### Basic Configuration

Edit `$profile:StatTracker/config.json` to customize:

```json
{
  "enableTeamKillTracking": true,
  "enableChatLogging": true,
  "scoreboardUpdateInterval": 5,
  "notifyOnKillstreak": true,
  "maxKillstreakNotification": 10,
  "persistenceInterval": 300,
  "databaseType": "FILE",
  "databaseName": "StatTracker",
  "enablePerformanceMonitoring": true,
  "performanceLoggingInterval": 300,
  "enableMemoryOptimization": true,
  "memoryCleanupInterval": 600,
  "enableHotReload": true,
  "configCheckInterval": 60,
  "defaultLanguage": "en"
}
```

### Error Handling Configuration

Configure comprehensive error handling behavior:

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
    "fallbackModeEnabled": true
  }
}
```

### Language Configuration

Set your preferred language in the configuration:

```json
{
  "defaultLanguage": "en",  // Options: "en", "fr", "de", "es", "ru"
  "fallbackLanguage": "en",
  "enableClientLanguageSelection": true
}
```

### Performance Settings

Optimize for your server:

```json
{
  "enablePerformanceMonitoring": true,
  "performanceLoggingInterval": 300,
  "highPlayerCountThreshold": 40,
  "reducedUpdateFrequencyForHighPlayerCount": true,
  "enableMemoryOptimization": true,
  "memoryCleanupInterval": 600,
  "maxKillHistoryPerPlayer": 50,
  "maxTeamKillRecords": 1000,
  "enableCaching": true
}
```

### Error Recovery Settings

Configure the system's error handling behavior:

```json
{
  "enableRecoveryMode": true,
  "errorThreshold": 10,
  "errorCooldownPeriod": 60,
  "recoveryModeDuration": 300,
  "enableMemoryBackup": true,
  "memoryBackupDumpInterval": 300,
  "maxDatabaseRetryAttempts": 5,
  "gracefulDegradationEnabled": true
}
```

## Usage

### Player Commands

- `/stats` - Show your current statistics
- `/stats <playername>` - Show another player's statistics
- `/rank` - Show your current ranking on the server
- `/top` - Show the top 10 players
- `/language <code>` - Set your preferred language (if allowed by server)

### Admin Commands

- `/admin` - Open the admin menu (admin permission required)
- `/announce <message>` - Broadcast an announcement
- `/kick <player> <reason>` - Kick a player
- `/ban <player> <duration> <reason>` - Ban a player
- `/stats_reset <player>` - Reset a player's statistics
- `/export_stats` - Export all statistics to CSV
- `/import_stats <file>` - Import statistics from CSV
- `/votekick_history <player>` - Show vote kick history for a player
- `/chat_history <playerID>` - Show chat history for a player
- `/chat_search <term>` - Search chat logs for a specific term
- `/config_get <key>` - Get a configuration value
- `/config_set <key> <value>` - Set a configuration value
- `/config_list` - List all configuration values
- `/config_reload` - Reload configuration from file
- `/config_save` - Save current configuration to file
- `/perf_stats` - Show performance statistics
- `/mem_stats` - Show memory usage statistics
- `/mem_cleanup` - Force memory cleanup
- `/system_health` - Display overall system health status
- `/recovery_mode` - Toggle recovery mode manually
- `/db_reconnect` - Force database reconnection attempt
- `/error_stats` - View error statistics by category
- `/component_health` - Check health status of all components
- `/validate_integrity` - Validate database integrity

### Web Dashboard

Access the admin dashboard at http://your-server-ip:8080/admin (requires authentication)

## Logs

The mod generates detailed logs organized by session and severity level:

### Session-based Organization
Each gaming session creates a unique folder: `$profile:StatTracker/Logs/Session_YYYY-MM-DD_HH-MM-SS_XXXX/`

### Log Types
Each session contains multiple log files:
- `Debug_[type]_[timestamp].log` - Detailed debug information
- `Info_[type]_[timestamp].log` - General information messages
- `Warning_[type]_[timestamp].log` - Warning messages
- `Error_[type]_[timestamp].log` - Error messages requiring attention
- `Critical_[type]_[timestamp].log` - Critical errors that may affect functionality
- `Chat_[type]_[timestamp].log` - Log of all in-game chat messages
- `VoteKick_[type]_[timestamp].log` - Log of all vote kick activities
- `Performance_[type]_[timestamp].log` - Performance monitoring logs

### Log Features
- Rich context logging with detailed information for troubleshooting
- Session-specific organization for easy correlation
- Severity-based separation for quick problem identification
- Automatic log rotation and retention management
- Comprehensive error tracking with detailed context

### Log Management
- Logs automatically rotate when they exceed the configured size limit
- Old log files are automatically cleaned up based on retention settings
- Emergency backups are created during database outages in `$profile:StatTracker/EmergencyBackups/`

## Error Handling

StatTracker implements comprehensive error handling to ensure stability:

### Key Features
- Rich error context with detailed information
- Component state tracking for recovery
- Automatic retry mechanisms for transient failures
- Graceful degradation when components fail
- Session-based error tracking and diagnostics
- Transaction support for database operations
- Health monitoring with self-healing capabilities

### Recovery Mechanisms
- Components automatically attempt recovery after failures
- Configurable retry intervals and maximum attempts
- Fallback mechanisms for critical components
- Memory backups during database failures
- Transaction rollback on partial failures

## Documentation

For more detailed information, please check the following documentation:

- [DOCUMENTATION.md](DOCUMENTATION.md) - Comprehensive user and admin guide
- [DEVELOPER.md](DEVELOPER.md) - Technical documentation for developers
- [CONTRIBUTING.md](CONTRIBUTING.md) - Guide for contributing to the project
- [LICENSE.md](LICENSE.md) - Full license text

## Known Issues

- None currently known. Please report any issues on our GitHub repository.

## Credits

Created by: Your Name

## License

This mod is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License. 