// STS_DiscordIntegration.c
// Provides Discord bot integration for remote administration and statistics

class STS_DiscordIntegrationConfig
{
    bool m_bEnabled = true;
    string m_sBotToken = ""; // Discord bot token
    string m_sWebhookURL = ""; // Discord webhook URL for sending messages
    string m_sCommandsChannelId = ""; // Channel ID where commands are accepted
    string m_sAdminChannelId = ""; // Channel ID for admin notifications
    string m_sStatsChannelId = ""; // Channel ID for stats updates
    string m_sLogsChannelId = ""; // Channel ID for log messages
    string m_sAlertsChannelId = ""; // Channel ID for important alerts
    array<string> m_aAdminRoleIds = {}; // Array of Discord role IDs with admin permissions
    array<string> m_aModeratorRoleIds = {}; // Array of Discord role IDs with moderator permissions
    int m_iStatsUpdateInterval = 3600; // Post server stats every hour (in seconds)
    int m_iMaxMessagesPerMinute = 10; // Rate limit for messages sent to Discord
    bool m_bLogChatToDiscord = true; // Send in-game chat to Discord
    bool m_bLogAdminActionsToDiscord = true; // Send admin actions to Discord
    bool m_bLogPlayerJoinLeaveToDiscord = true; // Send player join/leave events to Discord
    bool m_bAllowDiscordCommandExecution = true; // Allow commands from Discord
    bool m_bPostScreenshotsToDiscord = true; // Post admin screenshots to Discord
    bool m_bEnableRichPresence = true; // Enable Discord Rich Presence integration
}

class STS_DiscordIntegration
{
    // Singleton instance
    private static ref STS_DiscordIntegration s_Instance;
    
    // Configuration
    protected ref STS_DiscordIntegrationConfig m_Config;
    
    // References to other systems
    protected ref STS_LoggingSystem m_Logger;
    protected ref STS_Config m_MainConfig;
    protected ref STS_WebhookManager m_WebhookManager;
    
    // Server status info
    protected int m_iActivePlayerCount = 0;
    protected int m_iTotalNetworkPlayerCount = 0;
    protected int m_iPeakPlayerCount = 0;
    protected float m_fServerUptime = 0;
    protected float m_fPerformanceScore = 0;
    protected int m_iRestartCountdown = -1;
    
    // Message queue and rate limiting
    protected ref array<ref STS_DiscordMessage> m_aMessageQueue = new array<ref STS_DiscordMessage>();
    protected int m_iMessagesSentThisMinute = 0;
    protected float m_fLastMessageRateReset = 0;
    
    // Command handlers
    protected ref map<string, ref STS_DiscordCommandHandler> m_mCommandHandlers = new map<string, ref STS_DiscordCommandHandler>();
    
    // HTTP worker for async communication
    protected ref STS_HTTPWorker m_HTTPWorker;
    
    // API client for Discord
    protected ref STS_DiscordAPIClient m_APIClient;
    
    //------------------------------------------------------------------------------------------------
    protected void STS_DiscordIntegration()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_MainConfig = STS_Config.GetInstance();
        m_WebhookManager = STS_WebhookManager.GetInstance();
        
        // Initialize configuration
        m_Config = new STS_DiscordIntegrationConfig();
        LoadConfiguration();
        
        // Initialize API client
        m_APIClient = new STS_DiscordAPIClient(m_Config.m_sBotToken, m_Config.m_sWebhookURL);
        
        // Register command handlers
        RegisterCommandHandlers();
        
        // Initialize HTTP worker
        m_HTTPWorker = new STS_HTTPWorker();
        
        // Start message processing
        GetGame().GetCallqueue().CallLater(ProcessMessageQueue, 1000, true); // Check queue every second
        
        // Start periodic stats updates
        if (m_Config.m_bEnabled && m_Config.m_iStatsUpdateInterval > 0)
        {
            GetGame().GetCallqueue().CallLater(SendServerStats, m_Config.m_iStatsUpdateInterval * 1000, true);
        }
        
        m_Logger.LogInfo("Discord integration initialized");
        
        // Send startup message
        if (m_Config.m_bEnabled)
        {
            SendAdminMessage("Server started", "The server has been started and is now ready for players.");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    static STS_DiscordIntegration GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_DiscordIntegration();
        }
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load configuration
    protected void LoadConfiguration()
    {
        if (!m_MainConfig)
            return;
            
        // Load Discord bot configuration from main config
        m_Config.m_bEnabled = m_MainConfig.GetBoolValue("discord_enabled", m_Config.m_bEnabled);
        m_Config.m_sBotToken = m_MainConfig.GetStringValue("discord_bot_token", m_Config.m_sBotToken);
        m_Config.m_sWebhookURL = m_MainConfig.GetStringValue("discord_webhook_url", m_Config.m_sWebhookURL);
        m_Config.m_sCommandsChannelId = m_MainConfig.GetStringValue("discord_commands_channel", m_Config.m_sCommandsChannelId);
        m_Config.m_sAdminChannelId = m_MainConfig.GetStringValue("discord_admin_channel", m_Config.m_sAdminChannelId);
        m_Config.m_sStatsChannelId = m_MainConfig.GetStringValue("discord_stats_channel", m_Config.m_sStatsChannelId);
        m_Config.m_sLogsChannelId = m_MainConfig.GetStringValue("discord_logs_channel", m_Config.m_sLogsChannelId);
        m_Config.m_sAlertsChannelId = m_MainConfig.GetStringValue("discord_alerts_channel", m_Config.m_sAlertsChannelId);
        m_Config.m_iStatsUpdateInterval = m_MainConfig.GetIntValue("discord_stats_interval", m_Config.m_iStatsUpdateInterval);
        m_Config.m_iMaxMessagesPerMinute = m_MainConfig.GetIntValue("discord_rate_limit", m_Config.m_iMaxMessagesPerMinute);
        m_Config.m_bLogChatToDiscord = m_MainConfig.GetBoolValue("discord_log_chat", m_Config.m_bLogChatToDiscord);
        m_Config.m_bLogAdminActionsToDiscord = m_MainConfig.GetBoolValue("discord_log_admin_actions", m_Config.m_bLogAdminActionsToDiscord);
        m_Config.m_bLogPlayerJoinLeaveToDiscord = m_MainConfig.GetBoolValue("discord_log_player_join_leave", m_Config.m_bLogPlayerJoinLeaveToDiscord);
        m_Config.m_bAllowDiscordCommandExecution = m_MainConfig.GetBoolValue("discord_allow_commands", m_Config.m_bAllowDiscordCommandExecution);
        m_Config.m_bPostScreenshotsToDiscord = m_MainConfig.GetBoolValue("discord_post_screenshots", m_Config.m_bPostScreenshotsToDiscord);
        m_Config.m_bEnableRichPresence = m_MainConfig.GetBoolValue("discord_rich_presence", m_Config.m_bEnableRichPresence);
        
        // Load admin and moderator role IDs
        string adminRoles = m_MainConfig.GetStringValue("discord_admin_roles", "");
        string modRoles = m_MainConfig.GetStringValue("discord_moderator_roles", "");
        
        // Parse admin roles
        m_Config.m_aAdminRoleIds.Clear();
        array<string> adminRolesSplit = new array<string>();
        adminRoles.Split(",", adminRolesSplit);
        
        foreach (string role : adminRolesSplit)
        {
            string trimmed = role.Trim();
            if (!trimmed.IsEmpty())
            {
                m_Config.m_aAdminRoleIds.Insert(trimmed);
            }
        }
        
        // Parse moderator roles
        m_Config.m_aModeratorRoleIds.Clear();
        array<string> modRolesSplit = new array<string>();
        modRoles.Split(",", modRolesSplit);
        
        foreach (string role : modRolesSplit)
        {
            string trimmed = role.Trim();
            if (!trimmed.IsEmpty())
            {
                m_Config.m_aModeratorRoleIds.Insert(trimmed);
            }
        }
        
        m_Logger.LogInfo(string.Format("Loaded Discord integration configuration. Enabled: %1", m_Config.m_bEnabled));
    }
    
    //------------------------------------------------------------------------------------------------
    // Register all command handlers
    protected void RegisterCommandHandlers()
    {
        // Server management commands
        RegisterCommandHandler("status", new STS_DiscordCommandHandler_Status());
        RegisterCommandHandler("players", new STS_DiscordCommandHandler_Players());
        RegisterCommandHandler("restart", new STS_DiscordCommandHandler_Restart());
        RegisterCommandHandler("broadcast", new STS_DiscordCommandHandler_Broadcast());
        RegisterCommandHandler("kill", new STS_DiscordCommandHandler_KillServer());
        
        // Player management commands
        RegisterCommandHandler("kick", new STS_DiscordCommandHandler_Kick());
        RegisterCommandHandler("ban", new STS_DiscordCommandHandler_Ban());
        RegisterCommandHandler("unban", new STS_DiscordCommandHandler_Unban());
        RegisterCommandHandler("whitelist", new STS_DiscordCommandHandler_Whitelist());
        
        // Information commands
        RegisterCommandHandler("info", new STS_DiscordCommandHandler_Info());
        RegisterCommandHandler("help", new STS_DiscordCommandHandler_Help());
        RegisterCommandHandler("logs", new STS_DiscordCommandHandler_Logs());
        RegisterCommandHandler("performance", new STS_DiscordCommandHandler_Performance());
        
        // Stats commands
        RegisterCommandHandler("playerstats", new STS_DiscordCommandHandler_PlayerStats());
        RegisterCommandHandler("serverstats", new STS_DiscordCommandHandler_ServerStats());
        RegisterCommandHandler("heatmap", new STS_DiscordCommandHandler_Heatmap());
        RegisterCommandHandler("peak", new STS_DiscordCommandHandler_PeakTime());
        
        m_Logger.LogInfo(string.Format("Registered %1 Discord command handlers", m_mCommandHandlers.Count()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Register a command handler
    void RegisterCommandHandler(string command, STS_DiscordCommandHandler handler)
    {
        m_mCommandHandlers.Set(command.ToLower(), handler);
    }
    
    //------------------------------------------------------------------------------------------------
    // Process messages in the queue
    protected void ProcessMessageQueue()
    {
        if (!m_Config.m_bEnabled)
            return;
            
        // Check rate limiting
        float currentTime = GetGame().GetTickTime();
        if (currentTime - m_fLastMessageRateReset >= 60.0)
        {
            // Reset rate limiting after 1 minute
            m_iMessagesSentThisMinute = 0;
            m_fLastMessageRateReset = currentTime;
        }
        
        // Check if we can send more messages
        if (m_iMessagesSentThisMinute >= m_Config.m_iMaxMessagesPerMinute)
            return;
            
        // Process a message from the queue
        if (m_aMessageQueue.Count() > 0)
        {
            // Get the oldest message
            STS_DiscordMessage message = m_aMessageQueue[0];
            m_aMessageQueue.RemoveOrdered(0);
            
            // Send the message
            SendMessage(message);
            
            // Increment counter
            m_iMessagesSentThisMinute++;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a message to Discord
    protected void SendMessage(STS_DiscordMessage message)
    {
        // Implement API call to send message to Discord
        if (m_APIClient)
        {
            m_APIClient.SendMessage(message);
        }
        else
        {
            m_Logger.LogError("Failed to send Discord message: API client not initialized");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Queue a message to be sent to Discord
    protected void QueueMessage(STS_DiscordMessage message)
    {
        if (!m_Config.m_bEnabled)
            return;
            
        // Add the message to the queue
        m_aMessageQueue.Insert(message);
        
        m_Logger.LogDebug(string.Format("Queued Discord message to %1: %2", 
            message.m_sChannelId, message.m_sContent));
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle a command from Discord
    void HandleCommand(string channelId, string authorId, string authorName, string content)
    {
        if (!m_Config.m_bEnabled || !m_Config.m_bAllowDiscordCommandExecution)
            return;
            
        try
        {
            // Check if this is a command channel
            if (m_Config.m_sCommandsChannelId != channelId && m_Config.m_sAdminChannelId != channelId)
            {
                m_Logger.LogWarning(string.Format("Ignoring command from Discord channel %1: not a command channel", channelId));
                return;
            }
            
            // Parse the command
            if (!content.StartsWith("!"))
                return;
                
            content = content.Substring(1); // Remove the '!' prefix
            
            // Split into command and arguments
            array<string> parts = new array<string>();
            content.Split(" ", parts);
            
            if (parts.Count() == 0)
                return;
                
            string command = parts[0].ToLower();
            parts.RemoveOrdered(0);
            string arguments = "";
            
            // Reconstruct arguments string
            foreach (string part : parts)
            {
                if (!arguments.IsEmpty())
                    arguments += " ";
                    
                arguments += part;
            }
            
            // Look up command handler
            STS_DiscordCommandHandler handler = GetCommandHandler(command);
            
            if (!handler)
            {
                // Unknown command
                SendCommandChannelResponse(string.Format("Unknown command: %1", command));
                return;
            }
            
            // Check permissions
            if (!handler.CheckPermissions(this, authorId))
            {
                SendCommandChannelResponse(string.Format("You don't have permission to use the command: %1", command));
                return;
            }
            
            // Execute the command
            m_Logger.LogInfo(string.Format("Executing Discord command from %1: %2 %3", 
                authorName, command, arguments));
                
            string response = handler.Execute(this, arguments);
            
            if (!response.IsEmpty())
            {
                SendCommandChannelResponse(response);
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception handling Discord command: %1", e.ToString()));
            SendCommandChannelResponse(string.Format("Error executing command: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a command handler by name
    protected STS_DiscordCommandHandler GetCommandHandler(string command)
    {
        if (m_mCommandHandlers.Contains(command))
        {
            return m_mCommandHandlers.Get(command);
        }
        
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if a user has admin permissions
    bool HasAdminPermissions(string userId, array<string> userRoles)
    {
        // Check admin roles
        foreach (string roleId : m_Config.m_aAdminRoleIds)
        {
            if (userRoles.Contains(roleId))
                return true;
        }
        
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if a user has moderator permissions
    bool HasModeratorPermissions(string userId, array<string> userRoles)
    {
        // Check admin and moderator roles
        if (HasAdminPermissions(userId, userRoles))
            return true;
            
        foreach (string roleId : m_Config.m_aModeratorRoleIds)
        {
            if (userRoles.Contains(roleId))
                return true;
        }
        
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a message to the command channel
    void SendCommandChannelResponse(string message)
    {
        if (m_Config.m_sCommandsChannelId.IsEmpty())
            return;
            
        STS_DiscordMessage discordMessage = new STS_DiscordMessage();
        discordMessage.m_sChannelId = m_Config.m_sCommandsChannelId;
        discordMessage.m_sContent = message;
        
        QueueMessage(discordMessage);
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a message to the admin channel
    void SendAdminMessage(string title, string message)
    {
        if (m_Config.m_sAdminChannelId.IsEmpty())
            return;
            
        STS_DiscordMessage discordMessage = new STS_DiscordMessage();
        discordMessage.m_sChannelId = m_Config.m_sAdminChannelId;
        discordMessage.m_sContent = "";
        
        // Create embed
        map<string, string> embed = new map<string, string>();
        embed.Set("title", title);
        embed.Set("description", message);
        embed.Set("color", "16711680"); // Red color
        
        discordMessage.m_aEmbeds.Insert(embed);
        
        QueueMessage(discordMessage);
    }
    
    //------------------------------------------------------------------------------------------------
    // Log an in-game chat message to Discord
    void LogChatMessage(string playerName, string message)
    {
        if (!m_Config.m_bEnabled || !m_Config.m_bLogChatToDiscord || m_Config.m_sLogsChannelId.IsEmpty())
            return;
            
        STS_DiscordMessage discordMessage = new STS_DiscordMessage();
        discordMessage.m_sChannelId = m_Config.m_sLogsChannelId;
        discordMessage.m_sContent = string.Format("[CHAT] %1: %2", playerName, message);
        
        QueueMessage(discordMessage);
    }
    
    //------------------------------------------------------------------------------------------------
    // Log an admin action to Discord
    void LogAdminAction(string adminName, string action, string target, string details)
    {
        if (!m_Config.m_bEnabled || !m_Config.m_bLogAdminActionsToDiscord || m_Config.m_sAdminChannelId.IsEmpty())
            return;
            
        STS_DiscordMessage discordMessage = new STS_DiscordMessage();
        discordMessage.m_sChannelId = m_Config.m_sAdminChannelId;
        discordMessage.m_sContent = "";
        
        // Create embed
        map<string, string> embed = new map<string, string>();
        embed.Set("title", "Admin Action");
        
        string description = string.Format("**Admin:** %1\n**Action:** %2\n**Target:** %3", 
            adminName, action, target);
            
        if (!details.IsEmpty())
        {
            description += string.Format("\n**Details:** %1", details);
        }
        
        embed.Set("description", description);
        embed.Set("color", "3447003"); // Blue color
        
        discordMessage.m_aEmbeds.Insert(embed);
        
        QueueMessage(discordMessage);
    }
    
    //------------------------------------------------------------------------------------------------
    // Log a player join/leave event to Discord
    void LogPlayerJoinLeave(string playerName, string playerId, bool isJoining)
    {
        if (!m_Config.m_bEnabled || !m_Config.m_bLogPlayerJoinLeaveToDiscord || m_Config.m_sLogsChannelId.IsEmpty())
            return;
            
        STS_DiscordMessage discordMessage = new STS_DiscordMessage();
        discordMessage.m_sChannelId = m_Config.m_sLogsChannelId;
        
        if (isJoining)
        {
            discordMessage.m_sContent = string.Format("🟢 **%1** (%2) joined the server", playerName, playerId);
        }
        else
        {
            discordMessage.m_sContent = string.Format("🔴 **%1** (%2) left the server", playerName, playerId);
        }
        
        QueueMessage(discordMessage);
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a screenshot to Discord
    void SendScreenshotToDiscord(string playerName, string screenshotPath)
    {
        if (!m_Config.m_bEnabled || !m_Config.m_bPostScreenshotsToDiscord || m_Config.m_sAdminChannelId.IsEmpty())
            return;
            
        // In a real implementation, this would upload the screenshot file to Discord
        // For this example, we'll just log that it would happen
        
        m_Logger.LogInfo(string.Format("Would upload screenshot from %1 to Discord: %2", 
            playerName, screenshotPath));
            
        STS_DiscordMessage discordMessage = new STS_DiscordMessage();
        discordMessage.m_sChannelId = m_Config.m_sAdminChannelId;
        discordMessage.m_sContent = string.Format("Admin screenshot from %1", playerName);
        
        // In a real implementation, we would attach the file here
        
        QueueMessage(discordMessage);
    }
    
    //------------------------------------------------------------------------------------------------
    // Send server statistics to Discord
    protected void SendServerStats()
    {
        if (!m_Config.m_bEnabled || m_Config.m_sStatsChannelId.IsEmpty())
            return;
            
        try
        {
            // Update stats
            UpdateServerStats();
            
            // Prepare the message
            STS_DiscordMessage discordMessage = new STS_DiscordMessage();
            discordMessage.m_sChannelId = m_Config.m_sStatsChannelId;
            discordMessage.m_sContent = "";
            
            // Create embed
            map<string, string> embed = new map<string, string>();
            embed.Set("title", "Server Statistics");
            
            string description = string.Format(
                "**Players Online:** %1\n" +
                "**Network Players:** %2\n" +
                "**Peak Players:** %3\n" +
                "**Server Uptime:** %4\n" +
                "**Performance Score:** %5/10\n",
                m_iActivePlayerCount,
                m_iTotalNetworkPlayerCount,
                m_iPeakPlayerCount,
                FormatUptime(m_fServerUptime),
                Math.Round(m_fPerformanceScore)
            );
            
            if (m_iRestartCountdown > 0)
            {
                description += string.Format("\n**Next Restart:** %1 minutes", m_iRestartCountdown);
            }
            
            embed.Set("description", description);
            embed.Set("color", "5763719"); // Green color
            
            discordMessage.m_aEmbeds.Insert(embed);
            
            QueueMessage(discordMessage);
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception sending server stats to Discord: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update server statistics
    protected void UpdateServerStats()
    {
        // Update player counts
        m_iActivePlayerCount = GetGame().GetPlayerCount();
        
        // Get total network player count if available
        STS_MultiServerIntegration multiServer = STS_MultiServerIntegration.GetInstance();
        if (multiServer)
        {
            m_iTotalNetworkPlayerCount = multiServer.GetTotalNetworkPlayerCount();
        }
        else
        {
            m_iTotalNetworkPlayerCount = m_iActivePlayerCount;
        }
        
        // Update peak player count
        if (m_iActivePlayerCount > m_iPeakPlayerCount)
        {
            m_iPeakPlayerCount = m_iActivePlayerCount;
        }
        
        // Update server uptime
        m_fServerUptime = GetGame().GetTickTime();
        
        // Update performance score
        STS_PerformanceMonitor perfMonitor = STS_PerformanceMonitor.GetInstance();
        if (perfMonitor)
        {
            m_fPerformanceScore = perfMonitor.GetPerformanceScore();
        }
        else
        {
            m_fPerformanceScore = 10.0; // Default to 10 if no monitor
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Format uptime as a human-readable string
    protected string FormatUptime(float seconds)
    {
        int totalSeconds = Math.Round(seconds);
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        
        return string.Format("%1h %2m", hours, minutes);
    }
}

//------------------------------------------------------------------------------------------------
// Discord message class
class STS_DiscordMessage
{
    string m_sChannelId;
    string m_sContent;
    array<ref map<string, string>> m_aEmbeds = new array<ref map<string, string>>();
    array<string> m_aFileAttachments = new array<string>();
    
    void STS_DiscordMessage()
    {
        m_sChannelId = "";
        m_sContent = "";
    }
}

//------------------------------------------------------------------------------------------------
// HTTP Worker class for async requests
class STS_HTTPWorker
{
    void STS_HTTPWorker()
    {
        // In a real implementation, this would initialize an HTTP client
    }
    
    void SendRequest(string url, string method, string data, func<string> callback)
    {
        // In a real implementation, this would send an HTTP request
        // For this example, we'll just log it
        
        Print("[StatTracker] Would send HTTP request: " + method + " " + url);
        
        // Simulate a callback
        GetGame().GetCallqueue().CallLater(SimulateCallback, 100, false, callback);
    }
    
    protected void SimulateCallback(func<string> callback)
    {
        if (callback)
        {
            callback("{\"success\":true}");
        }
    }
}

//------------------------------------------------------------------------------------------------
// Discord API Client
class STS_DiscordAPIClient
{
    protected string m_sBotToken;
    protected string m_sWebhookURL;
    protected ref STS_LoggingSystem m_Logger;
    protected ref STS_HTTPWorker m_HTTPWorker;
    
    void STS_DiscordAPIClient(string botToken, string webhookURL)
    {
        m_sBotToken = botToken;
        m_sWebhookURL = webhookURL;
        m_Logger = STS_LoggingSystem.GetInstance();
        m_HTTPWorker = new STS_HTTPWorker();
    }
    
    void SendMessage(STS_DiscordMessage message)
    {
        if (m_sWebhookURL.IsEmpty())
        {
            m_Logger.LogWarning("Discord webhook URL is empty - cannot send message");
            return;
        }
        
        // In a real implementation, this would construct a JSON payload and send it to Discord
        // For this example, we'll just log it
        
        m_Logger.LogDebug(string.Format("Would send Discord message to channel %1: %2", 
            message.m_sChannelId, message.m_sContent));
    }
}

//------------------------------------------------------------------------------------------------
// Base command handler class
class STS_DiscordCommandHandler
{
    bool CheckPermissions(STS_DiscordIntegration discord, string userId)
    {
        // Base implementation - requires moderator permissions by default
        array<string> emptyRoles = new array<string>();
        return discord.HasModeratorPermissions(userId, emptyRoles);
    }
    
    string Execute(STS_DiscordIntegration discord, string arguments)
    {
        // Base implementation - should be overridden by subclasses
        return "Command not implemented";
    }
}

//------------------------------------------------------------------------------------------------
// Command handler for !status command
class STS_DiscordCommandHandler_Status : STS_DiscordCommandHandler
{
    override bool CheckPermissions(STS_DiscordIntegration discord, string userId)
    {
        // Anyone can use this command
        return true;
    }
    
    override string Execute(STS_DiscordIntegration discord, string arguments)
    {
        int playerCount = GetGame().GetPlayerCount();
        float uptime = GetGame().GetTickTime();
        
        // Format uptime as hours and minutes
        int totalSeconds = Math.Round(uptime);
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        
        return string.Format("Server Status:\nPlayers Online: %1\nUptime: %2h %3m", 
            playerCount, hours, minutes);
    }
}

//------------------------------------------------------------------------------------------------
// Command handler for !players command
class STS_DiscordCommandHandler_Players : STS_DiscordCommandHandler
{
    override bool CheckPermissions(STS_DiscordIntegration discord, string userId)
    {
        // Anyone can use this command
        return true;
    }
    
    override string Execute(STS_DiscordIntegration discord, string arguments)
    {
        array<PlayerController> players = GetGame().GetPlayerManager().GetPlayers();
        int playerCount = players.Count();
        
        if (playerCount == 0)
        {
            return "No players online";
        }
        
        string response = string.Format("Players Online (%1):\n", playerCount);
        
        foreach (PlayerController player : players)
        {
            response += string.Format("- %1 (ID: %2)\n", 
                player.GetPlayerName(), player.GetUID());
        }
        
        return response;
    }
}

//------------------------------------------------------------------------------------------------
// Command handler for !restart command
class STS_DiscordCommandHandler_Restart : STS_DiscordCommandHandler
{
    override bool CheckPermissions(STS_DiscordIntegration discord, string userId)
    {
        // Only admins can use this command
        array<string> emptyRoles = new array<string>();
        return discord.HasAdminPermissions(userId, emptyRoles);
    }
    
    override string Execute(STS_DiscordIntegration discord, string arguments)
    {
        // Parse delay argument
        int delay = 0;
        
        if (!arguments.IsEmpty())
        {
            delay = arguments.ToInt();
        }
        
        if (delay <= 0)
        {
            delay = 60; // Default to 60 seconds
        }
        
        // Schedule shutdown
        ScheduleServerShutdown(delay);
        
        return string.Format("Server restart scheduled in %1 seconds", delay);
    }
    
    protected void ScheduleServerShutdown(int seconds)
    {
        // In a real implementation, this would schedule a server shutdown
        // For this example, we'll just log it
        
        Print(string.Format("[StatTracker] Would schedule server restart in %1 seconds", seconds));
    }
}

//------------------------------------------------------------------------------------------------
// Command handler for !broadcast command
class STS_DiscordCommandHandler_Broadcast : STS_DiscordCommandHandler
{
    override bool CheckPermissions(STS_DiscordIntegration discord, string userId)
    {
        // Only moderators can use this command
        array<string> emptyRoles = new array<string>();
        return discord.HasModeratorPermissions(userId, emptyRoles);
    }
    
    override string Execute(STS_DiscordIntegration discord, string arguments)
    {
        if (arguments.IsEmpty())
        {
            return "Usage: !broadcast <message>";
        }
        
        // Broadcast message to all players
        BroadcastMessage(arguments);
        
        return string.Format("Broadcast sent: \"%1\"", arguments);
    }
    
    protected void BroadcastMessage(string message)
    {
        // In a real implementation, this would broadcast a message to all players
        // For this example, we'll just log it
        
        Print(string.Format("[StatTracker] Would broadcast message to all players: %1", message));
    }
}

//------------------------------------------------------------------------------------------------
// Command handler for !kick command
class STS_DiscordCommandHandler_Kick : STS_DiscordCommandHandler
{
    override bool CheckPermissions(STS_DiscordIntegration discord, string userId)
    {
        // Only moderators can use this command
        array<string> emptyRoles = new array<string>();
        return discord.HasModeratorPermissions(userId, emptyRoles);
    }
    
    override string Execute(STS_DiscordIntegration discord, string arguments)
    {
        // Parse arguments
        array<string> args = new array<string>();
        arguments.Split(" ", args);
        
        if (args.Count() < 1)
        {
            return "Usage: !kick <player> [reason]";
        }
        
        string playerName = args[0];
        string reason = "No reason provided";
        
        if (args.Count() > 1)
        {
            // Reconstruct reason from remaining arguments
            reason = "";
            for (int i = 1; i < args.Count(); i++)
            {
                if (!reason.IsEmpty())
                    reason += " ";
                    
                reason += args[i];
            }
        }
        
        // Find the player
        PlayerController targetPlayer = FindPlayerByName(playerName);
        
        if (!targetPlayer)
        {
            return string.Format("Player not found: %1", playerName);
        }
        
        // Kick the player
        KickPlayer(targetPlayer, reason);
        
        // Log the admin action
        discord.LogAdminAction("Discord", "kick", targetPlayer.GetPlayerName(), reason);
        
        return string.Format("Kicked %1. Reason: %2", targetPlayer.GetPlayerName(), reason);
    }
    
    protected PlayerController FindPlayerByName(string name)
    {
        // In a real implementation, this would find a player by name
        // For this example, we'll just return null
        
        return null;
    }
    
    protected void KickPlayer(PlayerController player, string reason)
    {
        // In a real implementation, this would kick the player
        // For this example, we'll just log it
        
        Print(string.Format("[StatTracker] Would kick player %1: %2", 
            player.GetPlayerName(), reason));
    }
}

//------------------------------------------------------------------------------------------------
// Command handler for !help command
class STS_DiscordCommandHandler_Help : STS_DiscordCommandHandler
{
    override bool CheckPermissions(STS_DiscordIntegration discord, string userId)
    {
        // Anyone can use this command
        return true;
    }
    
    override string Execute(STS_DiscordIntegration discord, string arguments)
    {
        string response = "Available Commands:\n";
        response += "!status - Show server status\n";
        response += "!players - List online players\n";
        response += "!help - Show this help message\n";
        
        // Add moderator commands
        array<string> emptyRoles = new array<string>();
        if (discord.HasModeratorPermissions("", emptyRoles))
        {
            response += "\nModerator Commands:\n";
            response += "!kick <player> [reason] - Kick a player\n";
            response += "!ban <player> <duration> [reason] - Ban a player\n";
            response += "!unban <player> - Unban a player\n";
            response += "!broadcast <message> - Broadcast a message\n";
        }
        
        // Add admin commands
        if (discord.HasAdminPermissions("", emptyRoles))
        {
            response += "\nAdmin Commands:\n";
            response += "!restart [seconds] - Restart the server\n";
            response += "!kill - Shut down the server\n";
            response += "!whitelist <add|remove> <player> - Modify whitelist\n";
        }
        
        return response;
    }
} 