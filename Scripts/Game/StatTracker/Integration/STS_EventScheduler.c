// STS_EventScheduler.c
// Provides scheduling of server events, restarts, game modes and announcements

class STS_ScheduledEvent
{
    string m_sEventId;               // Unique identifier for the event
    string m_sEventType;             // Type of event (restart, announcement, gamemode, etc)
    string m_sDescription;           // Human-readable description of the event
    int m_iIntervalSeconds;          // Repeat interval in seconds (0 for one-time events)
    string m_sCronExpression;        // Cron-style expression for complex scheduling (optional)
    bool m_bEnabled;                 // Whether the event is currently enabled
    string m_sTimeOfDay;             // Time of day to run the event (HH:MM format)
    array<int> m_aDaysOfWeek;        // Days of week (0-6, Sunday = 0)
    float m_fNextExecutionTime;      // Next execution time (game tick time)
    map<string, string> m_mParameters; // Event-specific parameters
    bool m_bNotifyPlayers;           // Whether to notify players before the event
    int m_iNotifyMinutesBefore;      // How many minutes before to notify players
    float m_fLastExecutionTime;      // When the event was last executed
    bool m_bExecuteOnce;             // Whether to only execute once and then disable

    //------------------------------------------------------------------------------------------------
    void STS_ScheduledEvent()
    {
        m_sEventId = "";
        m_sEventType = "";
        m_sDescription = "";
        m_iIntervalSeconds = 0;
        m_sCronExpression = "";
        m_bEnabled = true;
        m_sTimeOfDay = "";
        m_aDaysOfWeek = new array<int>();
        m_fNextExecutionTime = 0;
        m_mParameters = new map<string, string>();
        m_bNotifyPlayers = true;
        m_iNotifyMinutesBefore = 5;
        m_fLastExecutionTime = 0;
        m_bExecuteOnce = false;
    }
    
    //------------------------------------------------------------------------------------------------
    bool ShouldExecute(float currentTime)
    {
        if (!m_bEnabled)
            return false;
            
        return currentTime >= m_fNextExecutionTime;
    }
    
    //------------------------------------------------------------------------------------------------
    bool ShouldNotify(float currentTime)
    {
        if (!m_bEnabled || !m_bNotifyPlayers)
            return false;
            
        float notifyTime = m_fNextExecutionTime - (m_iNotifyMinutesBefore * 60);
        return currentTime >= notifyTime && currentTime < m_fNextExecutionTime;
    }
    
    //------------------------------------------------------------------------------------------------
    void UpdateNextExecutionTime(float currentTime)
    {
        if (m_iIntervalSeconds > 0)
        {
            // Simple interval scheduling
            m_fNextExecutionTime = currentTime + m_iIntervalSeconds;
        }
        else if (!m_sCronExpression.IsEmpty())
        {
            // Cron-style scheduling (would be implemented in full version)
            m_fNextExecutionTime = CalculateNextCronExecutionTime(currentTime);
        }
        else if (!m_sTimeOfDay.IsEmpty())
        {
            // Daily scheduling at specific time
            m_fNextExecutionTime = CalculateNextTimeOfDay(currentTime);
        }
        else
        {
            // Default to one-time event 
            m_fNextExecutionTime = currentTime + 3600; // Default to 1 hour from now
            m_bExecuteOnce = true;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected float CalculateNextTimeOfDay(float currentTime)
    {
        // This would be implemented to calculate the next occurrence of a specific time of day
        // For this example, we'll just return 24 hours from current time
        return currentTime + 86400;
    }
    
    //------------------------------------------------------------------------------------------------
    protected float CalculateNextCronExecutionTime(float currentTime)
    {
        // This would implement cron expression parsing to determine next run time
        // For this example, we'll just return a day from now
        return currentTime + 86400;
    }
    
    //------------------------------------------------------------------------------------------------
    string GetParameter(string key, string defaultValue = "")
    {
        if (m_mParameters.Contains(key))
            return m_mParameters.Get(key);
            
        return defaultValue;
    }
    
    //------------------------------------------------------------------------------------------------
    int GetIntParameter(string key, int defaultValue = 0)
    {
        string value = GetParameter(key);
        
        if (value.IsEmpty())
            return defaultValue;
            
        return value.ToInt();
    }
    
    //------------------------------------------------------------------------------------------------
    bool GetBoolParameter(string key, bool defaultValue = false)
    {
        string value = GetParameter(key).ToLower();
        
        if (value.IsEmpty())
            return defaultValue;
            
        return value == "true" || value == "1" || value == "yes";
    }
}

class STS_EventSchedulerConfig
{
    bool m_bEnabled = true;                    // Whether the scheduler is enabled
    int m_iCheckIntervalSeconds = 60;          // How often to check for events to execute (seconds)
    array<ref STS_ScheduledEvent> m_aEvents;   // List of scheduled events
    bool m_bLoadFromConfig = true;             // Whether to load events from config file
    string m_sEventsConfigPath = "$profile:StatTracker/events.json"; // Path to events config file
    bool m_bLogEvents = true;                  // Whether to log events to the server log
    bool m_bDiscordNotifications = true;       // Whether to send event notifications to Discord
    
    //------------------------------------------------------------------------------------------------
    void STS_EventSchedulerConfig()
    {
        m_aEvents = new array<ref STS_ScheduledEvent>();
        
        // Default server restart event (every 6 hours)
        STS_ScheduledEvent restartEvent = new STS_ScheduledEvent();
        restartEvent.m_sEventId = "scheduled_restart";
        restartEvent.m_sEventType = "restart";
        restartEvent.m_sDescription = "Scheduled server restart";
        restartEvent.m_iIntervalSeconds = 21600; // 6 hours
        restartEvent.m_bEnabled = true;
        restartEvent.m_bNotifyPlayers = true;
        restartEvent.m_iNotifyMinutesBefore = 15;
        restartEvent.m_mParameters.Insert("countdown_minutes", "15");
        restartEvent.m_mParameters.Insert("message", "Server restart in {time}");
        
        m_aEvents.Insert(restartEvent);
    }
}

class STS_EventScheduler
{
    // Singleton instance
    private static ref STS_EventScheduler s_Instance;
    
    // Configuration
    protected ref STS_EventSchedulerConfig m_Config;
    
    // References to other systems
    protected ref STS_LoggingSystem m_Logger;
    protected ref STS_Config m_MainConfig;
    protected ref STS_DiscordIntegration m_Discord;
    
    // Event handling
    protected float m_fLastCheckTime = 0;
    protected ref array<string> m_aNotificationsSent = new array<string>();
    
    //------------------------------------------------------------------------------------------------
    protected void STS_EventScheduler()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_MainConfig = STS_Config.GetInstance();
        
        // Initialize configuration
        m_Config = new STS_EventSchedulerConfig();
        LoadConfiguration();
        
        // Get a reference to Discord integration if available
        m_Discord = STS_DiscordIntegration.GetInstance();
        
        // Start event checking
        if (m_Config.m_bEnabled)
        {
            GetGame().GetCallqueue().CallLater(CheckEvents, m_Config.m_iCheckIntervalSeconds * 1000, true);
            m_Logger.LogInfo("Event scheduler initialized");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    static STS_EventScheduler GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_EventScheduler();
        }
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load configuration
    protected void LoadConfiguration()
    {
        if (!m_MainConfig)
            return;
            
        // Load general configuration
        m_Config.m_bEnabled = m_MainConfig.GetBoolValue("event_scheduler_enabled", m_Config.m_bEnabled);
        m_Config.m_iCheckIntervalSeconds = m_MainConfig.GetIntValue("event_scheduler_check_interval", m_Config.m_iCheckIntervalSeconds);
        m_Config.m_bLoadFromConfig = m_MainConfig.GetBoolValue("event_scheduler_load_from_config", m_Config.m_bLoadFromConfig);
        m_Config.m_sEventsConfigPath = m_MainConfig.GetStringValue("event_scheduler_config_path", m_Config.m_sEventsConfigPath);
        m_Config.m_bLogEvents = m_MainConfig.GetBoolValue("event_scheduler_log_events", m_Config.m_bLogEvents);
        m_Config.m_bDiscordNotifications = m_MainConfig.GetBoolValue("event_scheduler_discord_notifications", m_Config.m_bDiscordNotifications);
        
        // Load events from config file if enabled
        if (m_Config.m_bLoadFromConfig)
        {
            LoadEventsFromConfig();
        }
        
        // Initialize next execution times for all events
        float currentTime = GetGame().GetTickTime();
        foreach (STS_ScheduledEvent event : m_Config.m_aEvents)
        {
            event.UpdateNextExecutionTime(currentTime);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load events from config file
    protected void LoadEventsFromConfig()
    {
        // In a real implementation, this would load events from a JSON file
        // For this example, we'll just use the default events
        
        m_Logger.LogInfo("Would load events from config file: " + m_Config.m_sEventsConfigPath);
    }
    
    //------------------------------------------------------------------------------------------------
    // Save events to config file
    protected void SaveEventsToConfig()
    {
        // In a real implementation, this would save events to a JSON file
        // For this example, we'll just log it
        
        m_Logger.LogInfo("Would save events to config file: " + m_Config.m_sEventsConfigPath);
    }
    
    //------------------------------------------------------------------------------------------------
    // Check for events to execute
    protected void CheckEvents()
    {
        if (!m_Config.m_bEnabled)
            return;
            
        float currentTime = GetGame().GetTickTime();
        
        // Check each event
        foreach (STS_ScheduledEvent event : m_Config.m_aEvents)
        {
            // Check if it's time to execute the event
            if (event.ShouldExecute(currentTime))
            {
                // Execute the event
                ExecuteEvent(event);
                
                // Update next execution time
                event.m_fLastExecutionTime = currentTime;
                event.UpdateNextExecutionTime(currentTime);
                
                // If it's a one-time event, disable it
                if (event.m_bExecuteOnce)
                {
                    event.m_bEnabled = false;
                }
                
                // Clear notification status
                m_aNotificationsSent.RemoveItem(event.m_sEventId);
            }
            // Check if it's time to notify players about the event
            else if (event.ShouldNotify(currentTime) && !m_aNotificationsSent.Contains(event.m_sEventId))
            {
                // Notify players about the event
                NotifyEventUpcoming(event);
                
                // Mark notification as sent
                m_aNotificationsSent.Insert(event.m_sEventId);
            }
        }
        
        m_fLastCheckTime = currentTime;
    }
    
    //------------------------------------------------------------------------------------------------
    // Execute an event
    protected void ExecuteEvent(STS_ScheduledEvent event)
    {
        if (m_Config.m_bLogEvents)
        {
            m_Logger.LogInfo(string.Format("Executing scheduled event: %1 (%2)", 
                event.m_sDescription, event.m_sEventId));
        }
        
        // Handle different event types
        switch (event.m_sEventType)
        {
            case "restart":
                ExecuteRestartEvent(event);
                break;
                
            case "announcement":
                ExecuteAnnouncementEvent(event);
                break;
                
            case "gamemode":
                ExecuteGameModeEvent(event);
                break;
                
            case "command":
                ExecuteCommandEvent(event);
                break;
                
            default:
                m_Logger.LogWarning(string.Format("Unknown event type: %1", event.m_sEventType));
                break;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Execute a restart event
    protected void ExecuteRestartEvent(STS_ScheduledEvent event)
    {
        // Get restart parameters
        int countdownMinutes = event.GetIntParameter("countdown_minutes", 5);
        string message = event.GetParameter("message", "Server restart in {time}");
        
        // Replace placeholders in message
        message = message.Replace("{time}", string.Format("%1 minutes", countdownMinutes));
        
        // Broadcast restart message to players
        BroadcastMessage(message);
        
        // Send notification to Discord if enabled
        if (m_Config.m_bDiscordNotifications && m_Discord)
        {
            m_Discord.SendAdminMessage("Server Restart", 
                string.Format("Server restart initiated. Server will restart in %1 minutes.", countdownMinutes));
        }
        
        // Schedule actual restart
        ScheduleServerShutdown(countdownMinutes * 60);
    }
    
    //------------------------------------------------------------------------------------------------
    // Execute an announcement event
    protected void ExecuteAnnouncementEvent(STS_ScheduledEvent event)
    {
        // Get announcement parameters
        string message = event.GetParameter("message", "Server announcement");
        bool showTitle = event.GetBoolParameter("show_title", true);
        string title = event.GetParameter("title", "Announcement");
        int displayTimeSeconds = event.GetIntParameter("display_time", 10);
        
        // Broadcast announcement to players
        if (showTitle)
        {
            BroadcastTitledMessage(title, message, displayTimeSeconds);
        }
        else
        {
            BroadcastMessage(message);
        }
        
        // Send notification to Discord if enabled
        if (m_Config.m_bDiscordNotifications && m_Discord)
        {
            m_Discord.SendAdminMessage(title, message);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Execute a game mode event
    protected void ExecuteGameModeEvent(STS_ScheduledEvent event)
    {
        // Get game mode parameters
        string gameModeName = event.GetParameter("gamemode", "");
        int durationMinutes = event.GetIntParameter("duration", 60);
        
        if (gameModeName.IsEmpty())
        {
            m_Logger.LogWarning("Cannot execute game mode event: no game mode specified");
            return;
        }
        
        // In a real implementation, this would activate a specific game mode
        // For this example, we'll just log it
        
        m_Logger.LogInfo(string.Format("Would activate game mode '%1' for %2 minutes", 
            gameModeName, durationMinutes));
            
        // Broadcast game mode activation to players
        BroadcastMessage(string.Format("Special game mode activated: %1", gameModeName));
        
        // Send notification to Discord if enabled
        if (m_Config.m_bDiscordNotifications && m_Discord)
        {
            m_Discord.SendAdminMessage("Game Mode Activated", 
                string.Format("Game mode '%1' activated for %2 minutes", gameModeName, durationMinutes));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Execute a command event
    protected void ExecuteCommandEvent(STS_ScheduledEvent event)
    {
        // Get command parameters
        string command = event.GetParameter("command", "");
        
        if (command.IsEmpty())
        {
            m_Logger.LogWarning("Cannot execute command event: no command specified");
            return;
        }
        
        // In a real implementation, this would execute a server command
        // For this example, we'll just log it
        
        m_Logger.LogInfo(string.Format("Would execute server command: %1", command));
        
        // Send notification to Discord if enabled
        if (m_Config.m_bDiscordNotifications && m_Discord)
        {
            m_Discord.SendAdminMessage("Server Command", 
                string.Format("Executed scheduled command: %1", command));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Notify players about an upcoming event
    protected void NotifyEventUpcoming(STS_ScheduledEvent event)
    {
        // Calculate time until event
        float timeUntilEvent = event.m_fNextExecutionTime - GetGame().GetTickTime();
        int minutesUntilEvent = Math.Round(timeUntilEvent / 60);
        
        // Create notification message based on event type
        string message = "";
        
        switch (event.m_sEventType)
        {
            case "restart":
                message = string.Format("Server will restart in %1 minutes", minutesUntilEvent);
                break;
                
            case "announcement":
                message = string.Format("Important announcement in %1 minutes", minutesUntilEvent);
                break;
                
            case "gamemode":
                string gameModeName = event.GetParameter("gamemode", "Special event");
                message = string.Format("%1 will start in %2 minutes", gameModeName, minutesUntilEvent);
                break;
                
            default:
                message = string.Format("%1 will occur in %2 minutes", event.m_sDescription, minutesUntilEvent);
                break;
        }
        
        // Broadcast notification to players
        BroadcastMessage(message);
        
        // Send notification to Discord if enabled
        if (m_Config.m_bDiscordNotifications && m_Discord)
        {
            m_Discord.SendAdminMessage("Upcoming Event", message);
        }
        
        // Log the notification
        if (m_Config.m_bLogEvents)
        {
            m_Logger.LogInfo(string.Format("Event notification sent for %1: %2", 
                event.m_sEventId, message));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Broadcast a message to all players
    protected void BroadcastMessage(string message)
    {
        // In a real implementation, this would send a message to all players
        // For this example, we'll just log it
        
        Print(string.Format("[StatTracker] Would broadcast message to all players: %1", message));
    }
    
    //------------------------------------------------------------------------------------------------
    // Broadcast a titled message to all players
    protected void BroadcastTitledMessage(string title, string message, int displayTimeSeconds)
    {
        // In a real implementation, this would send a titled message to all players
        // For this example, we'll just log it
        
        Print(string.Format("[StatTracker] Would broadcast titled message to all players - Title: %1, Message: %2, Time: %3s", 
            title, message, displayTimeSeconds));
    }
    
    //------------------------------------------------------------------------------------------------
    // Schedule a server shutdown
    protected void ScheduleServerShutdown(int seconds)
    {
        // In a real implementation, this would schedule a server shutdown
        // For this example, we'll just log it
        
        Print(string.Format("[StatTracker] Would schedule server shutdown in %1 seconds", seconds));
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a new event
    STS_ScheduledEvent AddEvent(string eventId, string eventType, string description)
    {
        // Create new event
        STS_ScheduledEvent newEvent = new STS_ScheduledEvent();
        newEvent.m_sEventId = eventId;
        newEvent.m_sEventType = eventType;
        newEvent.m_sDescription = description;
        
        // Set initial next execution time
        newEvent.UpdateNextExecutionTime(GetGame().GetTickTime());
        
        // Add to events list
        m_Config.m_aEvents.Insert(newEvent);
        
        // Save events to config
        if (m_Config.m_bLoadFromConfig)
        {
            SaveEventsToConfig();
        }
        
        return newEvent;
    }
    
    //------------------------------------------------------------------------------------------------
    // Remove an event
    bool RemoveEvent(string eventId)
    {
        for (int i = 0; i < m_Config.m_aEvents.Count(); i++)
        {
            if (m_Config.m_aEvents[i].m_sEventId == eventId)
            {
                m_Config.m_aEvents.RemoveOrdered(i);
                
                // Save events to config
                if (m_Config.m_bLoadFromConfig)
                {
                    SaveEventsToConfig();
                }
                
                return true;
            }
        }
        
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get an event by ID
    STS_ScheduledEvent GetEvent(string eventId)
    {
        foreach (STS_ScheduledEvent event : m_Config.m_aEvents)
        {
            if (event.m_sEventId == eventId)
            {
                return event;
            }
        }
        
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Enable or disable an event
    bool SetEventEnabled(string eventId, bool enabled)
    {
        STS_ScheduledEvent event = GetEvent(eventId);
        
        if (!event)
            return false;
            
        event.m_bEnabled = enabled;
        
        // Save events to config
        if (m_Config.m_bLoadFromConfig)
        {
            SaveEventsToConfig();
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all scheduled events
    array<ref STS_ScheduledEvent> GetEvents()
    {
        return m_Config.m_aEvents;
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a daily restart event
    STS_ScheduledEvent AddDailyRestartEvent(string timeOfDay, string message = "Server restart in {time}")
    {
        string eventId = "daily_restart_" + timeOfDay.Replace(":", "");
        
        STS_ScheduledEvent event = AddEvent(eventId, "restart", "Daily server restart");
        event.m_sTimeOfDay = timeOfDay;
        event.m_iIntervalSeconds = 0; // Use time of day instead of interval
        event.m_bNotifyPlayers = true;
        event.m_iNotifyMinutesBefore = 15;
        event.m_mParameters.Insert("countdown_minutes", "15");
        event.m_mParameters.Insert("message", message);
        
        // Update next execution time
        event.UpdateNextExecutionTime(GetGame().GetTickTime());
        
        return event;
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a recurring announcement event
    STS_ScheduledEvent AddRecurringAnnouncementEvent(string message, int intervalMinutes, string title = "Announcement")
    {
        string eventId = "announcement_" + GetRandomId();
        
        STS_ScheduledEvent event = AddEvent(eventId, "announcement", "Recurring announcement");
        event.m_iIntervalSeconds = intervalMinutes * 60;
        event.m_bNotifyPlayers = false; // No need to notify for announcements
        event.m_mParameters.Insert("message", message);
        event.m_mParameters.Insert("title", title);
        event.m_mParameters.Insert("show_title", "true");
        
        // Update next execution time
        event.UpdateNextExecutionTime(GetGame().GetTickTime());
        
        return event;
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a one-time game mode event
    STS_ScheduledEvent AddGameModeEvent(string gameModeName, string timeOfDay, int durationMinutes)
    {
        string eventId = "gamemode_" + gameModeName.Replace(" ", "_").ToLower();
        
        STS_ScheduledEvent event = AddEvent(eventId, "gamemode", "Game mode: " + gameModeName);
        event.m_sTimeOfDay = timeOfDay;
        event.m_bNotifyPlayers = true;
        event.m_iNotifyMinutesBefore = 30; // Notify 30 minutes before
        event.m_mParameters.Insert("gamemode", gameModeName);
        event.m_mParameters.Insert("duration", durationMinutes.ToString());
        event.m_bExecuteOnce = true; // One-time event
        
        // Update next execution time
        event.UpdateNextExecutionTime(GetGame().GetTickTime());
        
        return event;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a random ID
    protected string GetRandomId()
    {
        return Math.RandomInt(100000, 999999).ToString();
    }
} 