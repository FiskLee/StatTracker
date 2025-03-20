# StatTracker Developer Documentation

This document provides technical details about the StatTracker mod for Arma Reforger, intended for developers who want to understand, modify or extend the system.

## Architecture Overview

StatTracker is built using a modular component-based architecture following Arma Reforger's entity-component system. The system is composed of several specialized components that work together to track, store, and display player statistics.

### Core Design Principles

1. **Minimal Game Impact**: The mod is designed to have minimal impact on the base game, especially avoiding interference with built-in progression systems.
2. **Persistent Storage**: All statistics are persistently stored and loaded when players reconnect.
3. **Efficient Network Usage**: Only necessary data is synchronized between server and clients.
4. **Modularity**: Each subsystem is self-contained and can be enabled/disabled independently.
5. **Extensibility**: The system is designed to be easily extended with new features.
6. **Localization**: All user-facing text is localized to support multiple languages.
7. **Performance Optimization**: Memory usage and performance are continuously monitored and optimized.

## Component Hierarchy

```
STS_Main (Main Entry Point)
├── STS_LoggingSystem (Logging Framework)
├── STS_Config (Configuration Manager)
├── STS_DatabaseManager (Database Management)
├── STS_PersistenceManager (Data Persistence)
├── STS_VoteKickSystem (Vote Kick Tracking)
├── STS_ChatLogger (Chat Logging)
├── STS_TeamKillTracker (Team Kill Monitoring)
├── STS_NotificationManager (UI Notifications)
├── STS_AdminMenu (Admin Interface)
├── STS_WebhookManager (External Notifications)
├── STS_APIServer (HTTP API Server)
├── STS_AdminDashboard (Web Dashboard)
├── STS_HeatmapManager (Activity Heatmaps)
├── STS_LocalizationManager (Multi-language Support)
├── STS_MemoryManager (Memory Optimization)
├── STS_PerformanceMonitor (Performance Tracking)
├── STS_UIManager (UI Component Management)
└── STS_DataExportManager (Data Export)
```

## Core Components

### STS_Main

The central class that initializes and coordinates all subsystems. It follows a singleton pattern and is the entry point for the mod.

```c
static STS_Main GetInstance()
{
    if (!s_Instance)
    {
        s_Instance = new STS_Main();
    }
    
    return s_Instance;
}
```

### STS_LoggingSystem

A comprehensive logging system that supports multiple log levels (DEBUG, INFO, WARNING, ERROR, CRITICAL) and outputs to both console and files.

```c
// Log at different levels
m_Logger.LogDebug("Debug message");
m_Logger.LogInfo("Info message");
m_Logger.LogWarning("Warning message");
m_Logger.LogError("Error message");
m_Logger.LogCritical("Critical message");

// Special logs for specific features
m_Logger.LogChat(playerName, playerID, message);
m_Logger.LogVoteKick(initiatorName, initiatorID, targetName, targetID, reason);
```

### STS_StatTrackingComponent

Attached to each player entity to track individual statistics:

- Kills, deaths, captures, etc.
- Connection time, IP addresses
- Team kill information
- Kill details (who killed with what weapon)

The component hooks into the game's damage system to track kills and deaths:

```c
protected int OnDamageDealt(IEntity victim, IEntity instigator, float damage, int damageType, int hitZone)
{
    // Process damage events and track kills
}

protected void OnDamageStateChanged(EDamageState oldState, EDamageState newState)
{
    // Track player deaths
}
```

### STS_StatTrackingManagerComponent

World component that manages all player tracking components:

- Registers/unregisters players
- Handles global events (base captures, etc.)
- Coordinates data saving/loading
- Broadcasts stats to clients

```c
void RegisterPlayer(STS_StatTrackingComponent player)
{
    // Add player to tracked list
    m_aPlayers.Insert(player);
    // ...
}

void BroadcastStats()
{
    // Collect and broadcast player stats to all clients
    RPC_UpdateStats(playerIDs, playerStats, playerNames);
}
```

### STS_ScoreboardHUD

Manages the user interface for displaying statistics:

- Mini-HUD in the corner
- Full scoreboard (toggled with TAB)
- Dynamic updating as stats change

```c
protected void UpdateFullScoreboard()
{
    // Sort players by score
    // Create UI elements for each player
    // Display stats
}
```

## New Systems

### STS_VoteKickSystem

Tracks and manages vote kicks to prevent abuse:

- Records who initiated vote kicks and against whom
- Tracks all votes cast
- Enforces cooldown periods and limits
- Archives vote kick history for review

```c
protected void OnVoteStarted(SCR_VotingManagerComponent manager, int voteID, int initiatorID, int targetID, string reason)
{
    // Record vote kick initiation
    // Check for abuse (cooldowns, limits)
    // Log the event
}
```

### STS_ChatLogger

Logs all in-game chat messages:

- Hooks into the game's chat system
- Records messages with player details and timestamps
- Provides searching and filtering functionality

```c
protected void OnChatMessageReceived(ChatMessageInfo messageInfo)
{
    // Extract and format chat message details
    // Log to file and in-memory storage
    // Categorize by channel type
}
```

## Data Flow

1. **Stats Collection**:
   - Game events trigger callbacks in tracking components
   - Components update internal data structures
   - Changes are logged and broadcast if needed

2. **Persistence**:
   - Stats are auto-saved at regular intervals
   - Stats are saved when players disconnect
   - Stats are loaded when players reconnect

3. **UI Updates**:
   - Server broadcasts stat updates to clients
   - Clients update HUD components when data changes
   - Scoreboard is refreshed when displayed

## Network Architecture

The mod uses Arma Reforger's replication system to synchronize data:

- Server is authoritative for all stats
- Stats are only modified on the server
- Updates are broadcast to clients via RPCs
- Clients only display data, never modify it

```c
[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
protected void RPC_UpdateStats(array<int> playerIDs, array<ref STS_PlayerStats> playerStats, array<string> playerNames)
{
    // Update client-side UI with latest stats
}
```

## File Structure

```
StatTracker/
├── Scripts/
│   └── Game/
│       └── StatTracker/
│           ├── STS_Main.c                        # Main entry point
│           ├── STS_LoggingSystem.c               # Logging framework
│           ├── STS_Config.c                      # Configuration manager
│           ├── STS_StatTrackingComponent.c       # Player stat tracking
│           ├── STS_StatTrackingManagerComponent.c # Global stat management
│           ├── STS_ScoreboardHUD.c               # HUD display
│           ├── STS_TeamKillTracker.c             # Team kill monitoring
│           ├── STS_VoteKickSystem.c              # Vote kick tracking
│           ├── STS_ChatLogger.c                  # Chat logging
│           ├── STS_PersistenceManager.c          # Data persistence
│           ├── STS_NotificationManager.c         # UI notifications
│           ├── STS_AdminMenu.c                   # Admin UI
│           ├── STS_RCONCommands.c                # RCON command handling
│           ├── STS_WebhookManager.c              # Webhook notifications
│           ├── STS_HeatmapManager.c              # Heatmap generation
│           ├── STS_DataVisualization.c           # Data visualization
│           ├── STS_DataExportManager.c           # Data export tools
│           ├── STS_APIServer.c                   # HTTP API server
│           ├── STS_AdminDashboard.c              # Web dashboard
│           └── STS_RPC.c                         # RPC definitions
├── UI/
│   └── layouts/
│       ├── Scoreboard.layout                     # Main scoreboard layout
│       └── PlayerScoreRow.layout                 # Player row template
├── GUI/
│   └── layouts/
│       ├── admin_menu.layout                     # Admin UI layout
│       ├── admin_announcement.layout             # Admin announcement layout
│       └── admin_message.layout                  # Admin message layout
├── Configs/
│   ├── Input.conf                                # Input configuration
│   ├── StatTrackerManager.et                     # Manager entity config
│   └── PlayerStatTracking.conf                   # Player tracking config
└── addon.gproj                                   # Mod project file
```

## Data Persistence

All data is stored in JSON format in the profile directory:

- `$profile:StatTracker/player_stats.json` - Persistent player statistics
- `$profile:StatTracker/current_session.json` - Current session data
- `$profile:StatTracker/vote_kick_history.json` - Vote kick history
- `$profile:StatTracker/teamkills.json` - Team kill records
- `$profile:StatTracker/Logs/` - Log files directory

## JSON Format Examples

### Player Stats

```json
{
  "kills": 10,
  "deaths": 5,
  "basesLost": 0,
  "basesCaptured": 2,
  "totalXP": 750,
  "rank": 3,
  "suppliesDelivered": 0,
  "supplyDeliveryCount": 0,
  "aiKills": 15,
  "vehicleKills": 1,
  "airKills": 0,
  "ipAddress": "192.168.1.100",
  "connectionTime": 1622547689.352,
  "lastSessionDuration": 3600.25,
  "totalPlaytime": 14400.75,
  "killedBy": ["PlayerA", "PlayerB"],
  "killedByWeapon": ["M4A1", "AK-74"],
  "killedByTeam": [1, 1]
}
```

### Vote Kick Entry

```json
{
  "initiatorID": 123,
  "initiatorName": "PlayerA",
  "targetID": 456,
  "targetName": "PlayerB",
  "reason": "Team killing",
  "startTime": 1622547689.352,
  "endTime": 1622547749.123,
  "approved": true,
  "votesFor": 5,
  "votesAgainst": 2,
  "votersFor": [123, 789, 101, 202, 303],
  "votersAgainst": [505, 606]
}
```

## Error Handling and Edge Cases

The system is designed to handle various edge cases:

1. **Player Disconnection/Reconnection**: Stats are preserved and loaded when players return
2. **Server Crashes**: Regular auto-saving minimizes data loss
3. **Network Issues**: Critical operations use reliable RPC channels
4. **Late-joining Players**: Full state synchronization for new connections
5. **Mod Compatibility**: Designed to not interfere with other mods
6. **Performance Considerations**: Optimizations for high player counts

## Extending the System

To add new features to the StatTracker:

1. **Create a new component**: Implement your functionality in a new class
2. **Register with STS_Main**: Add your component to the initialization sequence
3. **Implement data persistence**: Add your data to the persistence system if needed
4. **Add UI elements**: Extend the UI system to display your data

## Debugging

The system includes comprehensive logging:

```c
// Enable detailed logging
m_LoggingSystem.SetConsoleLogLevel(ELogLevel.DEBUG);
m_LoggingSystem.SetFileLogLevel(ELogLevel.DEBUG);

// Log events
m_LoggingSystem.LogDebug("Debug information here");
```

## Event System

The mod hooks into various game events:

1. **Damage and Death**: For tracking kills/deaths
2. **Player Connection/Disconnection**: For session tracking
3. **Base Captures**: For objective tracking
4. **Vote Kick Events**: For abuse prevention
5. **Chat Messages**: For logging communications

## Performance Considerations

- Regular profiling ensures minimal performance impact
- Optimized data structures for fast lookups
- Conditional execution based on configuration
- Throttled network traffic to minimize bandwidth usage

## Arma Reforger Workshop Publication

1. **Test thoroughly** before uploading
2. **Set appropriate tags** for discovery
3. **Include descriptive images** showing the mod in action
4. **Provide clear installation instructions** in the workshop description
5. **Respond to user feedback** and update accordingly 

## Localization System

### STS_LocalizationManager

Manages translations and localization for the StatTracker system, supporting multiple languages.

```c
// Get instance of the localization manager
STS_LocalizationManager locManager = STS_LocalizationManager.GetInstance();

// Get a localized string
string translatedText = locManager.GetLocalizedString("STS_TEXT_SCOREBOARD_TITLE");

// Format a localized string with parameters
array<string> params = new array<string>();
params.Insert(playerName);
params.Insert(weaponName);
string formattedText = locManager.GetLocalizedString("STS_TEXT_PLAYER_KILLED", params);

// Static shorthand version
string text = STS_LocalizationManager.Loc("STS_TEXT_CLOSE");

// Set the current language
locManager.SetLanguage("fr");

// Get available languages
array<string> languages = locManager.GetAvailableLanguages();
```

#### Language Files

Language files are stored as JSON in `$profile:StatTracker/Localization/` with the language code as the filename (e.g., `en.json`, `fr.json`). Each file contains key-value pairs mapping localization keys to translated strings:

```json
{
  "STS_TEXT_SCOREBOARD_TITLE": "Player Statistics",
  "STS_TEXT_CLOSE": "Close",
  "STS_TEXT_PLAYER_KILLED": "{0} was killed by {1}"
}
```

#### Localization Keys

Use the `STS_TEXT_` prefix for all localization keys to maintain consistency. Organize keys by feature area:

- `STS_TEXT_UI_*` - UI element text
- `STS_TEXT_ERROR_*` - Error messages
- `STS_TEXT_NOTIFICATION_*` - Notification messages
- `STS_TEXT_STATS_*` - Statistics labels

### Memory Management

#### STS_MemoryManager

Manages memory usage and performs periodic cleanup to prevent memory leaks and optimize performance.

```c
// Get instance of the memory manager
STS_MemoryManager memManager = STS_MemoryManager.GetInstance();

// Get memory usage statistics
string stats = memManager.GetMemoryStats();

// Force a cleanup
memManager.PerformScheduledCleanup();

// Reset peak memory usage tracking
memManager.ResetPeakMemoryUsage();
```

#### Memory Cleanup Methods

The memory manager includes several specialized cleanup methods:

- `CleanupPlayerCache()` - Removes oldest cached player stats
- `CleanupTeamKillRecords()` - Cleans up old team kill records
- `CleanupKillHistory()` - Trims the kill history to a maximum size
- `CleanupArrays()` - Generic cleanup for other large arrays

#### Memory Usage Estimation

The `EstimateMemoryUsage()` method provides an approximation of current memory usage, tracking:

- Cached player statistics
- Team kill records
- Kill history
- Other in-memory data structures

### Performance Monitoring

#### STS_PerformanceMonitor

Monitors and tracks performance metrics across the system, providing detailed statistics and reporting.

```c
// Get instance of the performance monitor
STS_PerformanceMonitor perfMonitor = STS_PerformanceMonitor.GetInstance();

// Start measuring an operation
float startTime = perfMonitor.StartMeasurement("ComponentName", "OperationName");

// ... perform operation ...

// End measurement
perfMonitor.EndMeasurement("ComponentName", "OperationName", startTime);

// Measure a complete operation with known duration
perfMonitor.MeasureOperation("ComponentName", "OperationName", elapsedTimeMs);

// Generate a performance report
string report = perfMonitor.GeneratePerformanceReport();

// Reset metrics
perfMonitor.ResetMetrics();
```

#### Performance Metrics

Performance data is organized hierarchically:

- `STS_PerformanceMetrics` - Tracks metrics for a single component
  - `STS_OperationMetrics` - Tracks metrics for a specific operation
    - Count of operations
    - Total execution time
    - Minimum execution time
    - Maximum execution time
    - Average execution time

#### Performance Reporting

Performance reports are generated periodically (configurable interval) and include:

- Overall system statistics
- Component-level metrics
- Operation-level details
- Server load information (when available)

## User Interface Architecture

The UI system is organized around a central manager and component-based architecture:

```
STS_UIManager
├── STS_ScoreboardComponent
├── STS_PlayerStatsComponent
├── STS_NotificationComponent
├── STS_AdminMenuComponent
└── STS_MinimapComponent
```

### STS_UIManager

Centrally manages all UI components, handling input events, visibility, and updates.

```c
// Get UI manager instance
STS_UIManager uiManager = STS_UIManager.GetInstance();

// Show/hide components
uiManager.ShowScoreboard();
uiManager.HideScoreboard();
uiManager.ToggleStatsMenu();

// Show notifications
uiManager.ShowNotification("Player connected", 5.0, 0);
uiManager.ShowAchievementNotification("Sharpshooter", "Get 10 headshots");

// Update specific displays
uiManager.UpdatePlayerStatsDisplay("playerID");
```

### UI Components

Each UI component inherits from `STS_UIComponent` base class, providing:

- Standard initialization and cleanup methods
- Consistent visibility handling
- Access to localization, logging, and configuration
- Performance monitoring integration

```c
// Base class for all UI components
class STS_UIComponent
{
    // Component name for identification
    protected string m_sName;
    
    // Visibility state
    protected bool m_bVisible = false;
    
    // Core dependencies
    protected STS_UIManager m_UIManager;
    protected STS_Config m_Config;
    protected STS_LoggingSystem m_Logger;
    protected STS_LocalizationManager m_Localization;
    protected STS_PerformanceMonitor m_PerformanceMonitor;
    
    // Standard methods
    void Initialize();
    void Update();
    bool HandleInput(UAInput input, int type);
    void Show();
    void Hide();
    bool IsVisible();
    string GetName();
    void CleanUp();
    
    // Localization helper
    string GetLocalizedText(string key, array<string> params = null);
}
```

## Best Practices

### Logging

- Use appropriate log levels for different types of messages
- Include component and method names for context
- Do not log sensitive information

### Performance

- Always measure performance of complex operations
- Use the performance monitoring system for all significant operations
- Keep UI updates minimal and efficient

### Memory Management

- Use proper reference management to avoid leaks
- Do not store unnecessary large data structures in memory
- Clean up after operations that create temporary objects

### Localization

- Never use hardcoded strings in user interfaces
- Use localization keys with the `STS_TEXT_` prefix
- Use parameters for dynamic content rather than string concatenation
- Test layouts with longer translations (some languages expand text length)

### Error Handling

- Always check for null references
- Use try/catch blocks around risky operations
- Log detailed error information
- Provide user-friendly error messages through the localization system

## Events

The system uses several custom events to communicate between components:

- `STS_PlayerStatsUpdatedEvent` - Triggered when player stats change
- `STS_TeamKillEvent` - Triggered when a team kill occurs
- `STS_VoteKickEvent` - Triggered during vote kick operations
- `STS_ConfigChangedEvent` - Triggered when configuration changes

Example event handling:

```c
// Register for events
GetGame().GetCallqueue().CallLater(OnPlayerStatsUpdated, 0, false, playerID, stats);

// Event handler
void OnPlayerStatsUpdated(string playerID, STS_PlayerStats stats)
{
    // Handle the event
    Print(string.Format("Player %1 stats updated: %2 kills", playerID, stats.m_iKills));
}
```

## Database System

The database system uses the Enfusion Database Framework (EDF) to provide a flexible and robust data storage solution. It supports multiple database backends:

- JSON files (small servers)
- Binary files (medium servers)
- MongoDB (large servers)
- MySQL (optional)
- PostgreSQL (optional)

### Repository Pattern

Data access is organized using the repository pattern:

```c
// Get player stats from repository
STS_PlayerStatsRepository playerRepo = m_DatabaseManager.GetPlayerStatsRepository();
STS_PlayerStats stats = playerRepo.GetByUID(playerUID);

// Save player stats
playerRepo.Save(stats);

// Query multiple records
array<ref STS_PlayerStats> topPlayers = playerRepo.GetTopByKills(10);
```

## Adding New Features

When adding new features to StatTracker:

1. Create a new component class in the appropriate directory
2. Follow the existing naming conventions and patterns
3. Add localization keys for all user-facing text
4. Include performance monitoring for significant operations
5. Add appropriate logging
6. Register the component with the main system
7. Update documentation to reflect the new feature

## Testing Guidelines

- Test with different database configurations
- Test with multiple languages
- Test with high player counts (simulate when needed)
- Test error handling by forcing edge cases
- Test memory usage over long play sessions