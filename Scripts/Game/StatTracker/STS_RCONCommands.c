// STS_RCONCommands.c
// Handles custom RCON commands for the StatTracker system

class STS_RCONCommands
{
    // Singleton instance
    private static ref STS_RCONCommands s_Instance;
    
    // Config reference
    protected STS_Config m_Config;
    
    // Persistence manager reference
    protected STS_PersistenceManager m_PersistenceManager;
    
    // RCON command types
    static const string CMD_MSG = "sts_msg";             // Send message to players
    static const string CMD_ANNOUNCE = "sts_announce";   // Send announcement popup
    static const string CMD_STATS = "sts_stats";         // Get player stats
    static const string CMD_LEADERBOARD = "sts_top";     // Get leaderboard
    static const string CMD_KICK = "sts_kick";           // Kick player
    static const string CMD_BAN = "sts_ban";             // Ban player
    static const string CMD_MONITOR = "sts_monitor";     // Get live server stats
    static const string CMD_PLAYERS = "sts_players";     // List online players
    static const string CMD_CONFIG_GET = "sts_config_get";    // Get config value
    static const string CMD_CONFIG_SET = "sts_config_set";    // Set config value
    static const string CMD_CONFIG_LIST = "sts_config_list";  // List all config values
    static const string CMD_CONFIG_RELOAD = "sts_config_reload"; // Reload config from file
    static const string CMD_CONFIG_SAVE = "sts_config_save";  // Save current config to file
    
    // Last time monitoring data was sent
    protected int m_iLastMonitorTime = 0;
    
    // Logging
    protected STS_LoggingSystem m_Logger;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_RCONCommands()
    {
        m_Config = STS_Config.GetInstance();
        m_PersistenceManager = STS_PersistenceManager.GetInstance();
        m_Logger = STS_LoggingSystem.GetInstance();
        
        // Register RCON commands
        RegisterCommands();
        
        Print("[StatTracker] RCON Commands initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_RCONCommands GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_RCONCommands();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Register RCON commands with the game
    protected void RegisterCommands()
    {
        // Register commands with the game's RCON system
        GetGame().GetRPCManager().AddRPC("STS_RCONCommands", "OnRconCommand", this, SingleplayerExecutionType.Both);
        
        // Log registration
        Print("[StatTracker] Registered RCON commands: " + 
              CMD_MSG + ", " + 
              CMD_ANNOUNCE + ", " + 
              CMD_STATS + ", " + 
              CMD_LEADERBOARD + ", " + 
              CMD_KICK + ", " + 
              CMD_BAN + ", " + 
              CMD_MONITOR + ", " + 
              CMD_PLAYERS + ", " + 
              CMD_CONFIG_GET + ", " + 
              CMD_CONFIG_SET + ", " + 
              CMD_CONFIG_LIST + ", " + 
              CMD_CONFIG_RELOAD + ", " + 
              CMD_CONFIG_SAVE);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle RCON command
    void OnRconCommand(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        
        // Read command and parameters
        Param2<string, array<string>> data;
        if (!ctx.Read(data))
            return;
        
        string command = data.param1;
        array<string> params = data.param2;
        
        // Log command received
        if (m_Config.m_bDebugMode)
        {
            Print("[StatTracker] RCON command received: " + command);
            for (int i = 0; i < params.Count(); i++)
            {
                Print("  Param " + i + ": " + params[i]);
            }
        }
        
        // Handle command
        string response = "Unknown command";
        
        switch (command)
        {
            case CMD_MSG:
                response = HandleMessageCommand(params);
                break;
                
            case CMD_ANNOUNCE:
                response = HandleAnnounceCommand(params);
                break;
                
            case CMD_STATS:
                response = HandleStatsCommand(params);
                break;
                
            case CMD_LEADERBOARD:
                response = HandleLeaderboardCommand(params);
                break;
                
            case CMD_KICK:
                response = HandleKickCommand(params);
                break;
                
            case CMD_BAN:
                response = HandleBanCommand(params);
                break;
                
            case CMD_MONITOR:
                response = HandleMonitorCommand(params);
                break;
                
            case CMD_PLAYERS:
                response = HandlePlayersCommand(params);
                break;

            case CMD_CONFIG_GET:
                response = HandleConfigGetCommand(params);
                break;
                
            case CMD_CONFIG_SET:
                response = HandleConfigSetCommand(params);
                break;
                
            case CMD_CONFIG_LIST:
                response = HandleConfigListCommand(params);
                break;
                
            case CMD_CONFIG_RELOAD:
                response = HandleConfigReloadCommand(params);
                break;
                
            case CMD_CONFIG_SAVE:
                response = HandleConfigSaveCommand(params);
                break;
                
            default:
                response = "Unknown command: " + command;
                break;
        }
        
        // Send response back to RCON
        SendRconResponse(response);
        
        // If this is a monitoring command and Discord webhooks are enabled, send to Discord
        if (command == CMD_MONITOR && m_Config.m_bEnableWebhooks && m_Config.m_sWebhookUrl.Length() > 0)
        {
            int currentTime = GetTime();
            if (currentTime - m_iLastMonitorTime > 60) // Once per minute max
            {
                SendMonitoringDataToDiscord();
                m_iLastMonitorTime = currentTime;
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle message command
    // Format: sts_msg <target|all> <message>
    protected string HandleMessageCommand(array<string> params)
    {
        if (params.Count() < 2)
            return "Usage: " + CMD_MSG + " <target|all> <message>";
        
        string target = params[0];
        string message = "";
        
        // Combine remaining parameters into message
        for (int i = 1; i < params.Count(); i++)
        {
            if (i > 1) message += " ";
            message += params[i];
        }
        
        // Send message
        if (target.ToLower() == "all")
        {
            // Send to all players
            SendMessageToAll(message);
            return "Message sent to all players: " + message;
        }
        else
        {
            // Send to specific player
            PlayerBase player = FindPlayer(target);
            if (player)
            {
                SendMessageToPlayer(player, message);
                return "Message sent to " + target + ": " + message;
            }
            else
            {
                return "Player not found: " + target;
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle announce command - creates a popup announcement
    // Format: sts_announce <target|all> <message>
    protected string HandleAnnounceCommand(array<string> params)
    {
        if (params.Count() < 2)
            return "Usage: " + CMD_ANNOUNCE + " <target|all> <message>";
        
        string target = params[0];
        string message = "";
        
        // Combine remaining parameters into message
        for (int i = 1; i < params.Count(); i++)
        {
            if (i > 1) message += " ";
            message += params[i];
        }
        
        // Send announcement
        if (target.ToLower() == "all")
        {
            // Send to all players
            SendAnnouncementToAll(message);
            return "Announcement sent to all players: " + message;
        }
        else
        {
            // Send to specific player
            PlayerBase player = FindPlayer(target);
            if (player)
            {
                SendAnnouncementToPlayer(player, message);
                return "Announcement sent to " + target + ": " + message;
            }
            else
            {
                return "Player not found: " + target;
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle stats command
    // Format: sts_stats <playerId|playerName>
    protected string HandleStatsCommand(array<string> params)
    {
        if (params.Count() < 1)
            return "Usage: " + CMD_STATS + " <playerId|playerName>";
        
        string playerIdentifier = params[0];
        
        // Try to find player by name first
        string playerId = FindPlayerIdByName(playerIdentifier);
        
        // If not found by name, use the identifier as ID
        if (playerId == "")
            playerId = playerIdentifier;
        
        // Load player stats
        STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
        if (!stats)
            return "Stats not found for player: " + playerIdentifier;
        
        // Format stats output
        string response = "Stats for " + (stats.m_sPlayerName != "" ? stats.m_sPlayerName : playerId) + ":\n";
        response += "-------------------------------------------\n";
        response += string.Format("Kills: %1\n", stats.m_iKills);
        response += string.Format("Deaths: %1\n", stats.m_iDeaths);
        response += string.Format("K/D Ratio: %.2f\n", stats.m_iDeaths > 0 ? (float)stats.m_iKills / stats.m_iDeaths : stats.m_iKills);
        response += string.Format("Headshots: %1 (%.1f%%)\n", stats.m_iHeadshotKills, stats.m_iKills > 0 ? (float)stats.m_iHeadshotKills / stats.m_iKills * 100 : 0);
        response += string.Format("Longest Kill: %.1f m\n", stats.m_fLongestKill);
        response += string.Format("Damage Dealt: %.0f\n", stats.m_fDamageDealt);
        response += string.Format("Damage Taken: %.0f\n", stats.m_fDamageTaken);
        response += string.Format("Playtime: %1\n", FormatPlaytime(stats.m_iTotalPlaytimeSeconds));
        response += string.Format("First Seen: %1\n", FormatTimestamp(stats.m_iFirstLogin));
        response += string.Format("Last Seen: %1\n", FormatTimestamp(stats.m_iLastLogin));
        
        return response;
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle leaderboard command
    // Format: sts_top <statName> [count]
    protected string HandleLeaderboardCommand(array<string> params)
    {
        if (params.Count() < 1)
            return "Usage: " + CMD_LEADERBOARD + " <statName> [count]";
        
        string statName = params[0];
        int count = 10;
        
        if (params.Count() > 1)
            count = params[1].ToInt();
        
        // Get leaderboard entries
        array<ref STS_LeaderboardEntry> leaderboard = m_PersistenceManager.GetTopPlayers(statName, count);
        if (!leaderboard || leaderboard.Count() == 0)
            return "No leaderboard data found for: " + statName;
        
        // Format leaderboard output
        string response = "Top " + count + " players by " + statName + ":\n";
        response += "-------------------------------------------\n";
        
        for (int i = 0; i < leaderboard.Count(); i++)
        {
            STS_LeaderboardEntry entry = leaderboard[i];
            response += string.Format("%1. %2: %.0f\n", i + 1, entry.m_sPlayerName, entry.m_fValue);
        }
        
        return response;
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle kick command
    // Format: sts_kick <playerId|playerName> [reason]
    protected string HandleKickCommand(array<string> params)
    {
        if (params.Count() < 1)
            return "Usage: " + CMD_KICK + " <playerId|playerName> [reason]";
        
        string playerIdentifier = params[0];
        string reason = "Kicked by admin";
        
        if (params.Count() > 1)
        {
            reason = "";
            // Combine remaining parameters into reason
            for (int i = 1; i < params.Count(); i++)
            {
                if (i > 1) reason += " ";
                reason += params[i];
            }
        }
        
        // Find player
        PlayerBase player = FindPlayer(playerIdentifier);
        if (!player)
            return "Player not found: " + playerIdentifier;
        
        // Get player identity
        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return "Could not get player identity";
        
        // Log the kick action
        string playerName = identity.GetName();
        string playerId = identity.GetId();
        Print("[StatTracker] Admin kicked player: " + playerName + " (" + playerId + ") - Reason: " + reason);
        
        // Send webhook notification if enabled
        if (m_Config.m_bEnableWebhooks)
        {
            array<string> fields = new array<string>();
            fields.Insert("Player");
            fields.Insert(playerName);
            fields.Insert("Reason");
            fields.Insert(reason);
            
            STS_WebhookManager.GetInstance().QueueServerNotification(
                "Player Kicked",
                "An admin has kicked a player from the server.",
                fields
            );
        }
        
        // Kick player
        GetGame().DisconnectPlayer(identity, reason);
        
        return "Kicked player: " + playerName + " - Reason: " + reason;
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle ban command
    // Format: sts_ban <playerId|playerName> <duration> [reason]
    // Duration in minutes, 0 = permanent
    protected string HandleBanCommand(array<string> params)
    {
        if (params.Count() < 2)
            return "Usage: " + CMD_BAN + " <playerId|playerName> <duration> [reason]";
        
        string playerIdentifier = params[0];
        int duration = params[1].ToInt();
        string reason = "Banned by admin";
        
        if (params.Count() > 2)
        {
            reason = "";
            // Combine remaining parameters into reason
            for (int i = 2; i < params.Count(); i++)
            {
                if (i > 2) reason += " ";
                reason += params[i];
            }
        }
        
        // Find player
        PlayerBase player = FindPlayer(playerIdentifier);
        if (!player)
            return "Player not found: " + playerIdentifier;
        
        // Get player identity
        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return "Could not get player identity";
        
        string playerName = identity.GetName();
        string playerId = identity.GetId();
        
        // Ban player
        BanPlayer(playerId, duration, reason);
        
        // Log the ban action
        Print("[StatTracker] Admin banned player: " + playerName + " (" + playerId + ") - Duration: " + 
              (duration == 0 ? "Permanent" : duration + " minutes") + " - Reason: " + reason);
        
        // Send webhook notification if enabled
        if (m_Config.m_bEnableWebhooks)
        {
            array<string> fields = new array<string>();
            fields.Insert("Player");
            fields.Insert(playerName);
            fields.Insert("Duration");
            fields.Insert(duration == 0 ? "Permanent" : duration + " minutes");
            fields.Insert("Reason");
            fields.Insert(reason);
            
            STS_WebhookManager.GetInstance().QueueServerNotification(
                "Player Banned",
                "An admin has banned a player from the server.",
                fields
            );
        }
        
        // Kick player
        GetGame().DisconnectPlayer(identity, "Banned: " + reason);
        
        return "Banned player: " + playerName + " - Duration: " + (duration == 0 ? "Permanent" : duration + " minutes") + " - Reason: " + reason;
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle monitor command
    // Format: sts_monitor
    protected string HandleMonitorCommand(array<string> params)
    {
        // Get all online players
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // Construct monitoring response
        string response = "Server Monitoring Data:\n";
        response += "-------------------------------------------\n";
        response += "Players Online: " + players.Count() + "\n";
        response += "Server Time: " + FormatTimestamp(System.GetUnixTime()) + "\n";
        response += "Server Uptime: " + FormatPlaytime(GetGame().GetTickTime() / 1000) + "\n";
        response += "\nOnline Players:\n";
        
        // List all online players with basic stats
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (!player)
                continue;
                
            PlayerIdentity identity = player.GetIdentity();
            if (!identity)
                continue;
                
            string playerId = identity.GetId();
            string playerName = identity.GetName();
            
            // Load player stats
            STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
            if (!stats)
                continue;
            
            // Get session playtime
            int sessionTime = 0;
            if (player.m_SessionStartTime > 0)
                sessionTime = System.GetUnixTime() - player.m_SessionStartTime;
            
            // Add player info to response
            response += string.Format("\n%1 (%2)\n", playerName, playerId);
            response += string.Format("  K/D: %1/%2 (%.2f)\n", 
                stats.m_iKills, 
                stats.m_iDeaths, 
                stats.m_iDeaths > 0 ? (float)stats.m_iKills / stats.m_iDeaths : stats.m_iKills);
            response += string.Format("  Health: %.0f%%\n", player.GetHealth("", "") * 100);
            response += string.Format("  Position: %1\n", player.GetPosition().ToString());
            response += string.Format("  Session Time: %1\n", FormatPlaytime(sessionTime));
        }
        
        return response;
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle players command
    // Format: sts_players
    protected string HandlePlayersCommand(array<string> params)
    {
        // Get all online players
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // Construct response
        string response = "Online Players (" + players.Count() + "):\n";
        response += "-------------------------------------------\n";
        
        // List all online players
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (!player)
                continue;
                
            PlayerIdentity identity = player.GetIdentity();
            if (!identity)
                continue;
                
            string playerId = identity.GetId();
            string playerName = identity.GetName();
            
            // Add player info to response
            response += string.Format("%1 (%2)\n", playerName, playerId);
        }
        
        return response;
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a message to all players
    protected void SendMessageToAll(string message)
    {
        // Get all players
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // Send message to each player
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (player)
            {
                SendMessageToPlayer(player, message);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a message to a specific player
    protected void SendMessageToPlayer(PlayerBase player, string message)
    {
        if (!player)
            return;
            
        // Use the game's notification system to send the message
        Param1<string> msgParam = new Param1<string>(message);
        GetGame().RPCSingleParam(player, STS_RPC.ADMIN_MESSAGE, msgParam, true, player.GetIdentity());
    }
    
    //------------------------------------------------------------------------------------------------
    // Send an announcement to all players (popup)
    protected void SendAnnouncementToAll(string message)
    {
        // Get all players
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // Send announcement to each player
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (player)
            {
                SendAnnouncementToPlayer(player, message);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Send an announcement to a specific player (popup)
    protected void SendAnnouncementToPlayer(PlayerBase player, string message)
    {
        if (!player)
            return;
            
        // Use the game's notification system to send the announcement
        Param1<string> msgParam = new Param1<string>(message);
        GetGame().RPCSingleParam(player, STS_RPC.ADMIN_ANNOUNCEMENT, msgParam, true, player.GetIdentity());
    }
    
    //------------------------------------------------------------------------------------------------
    // Find a player by name or ID
    protected PlayerBase FindPlayer(string playerIdentifier)
    {
        // Get all players
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // Check each player
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (!player)
                continue;
                
            PlayerIdentity identity = player.GetIdentity();
            if (!identity)
                continue;
                
            // Check if ID matches
            if (identity.GetId() == playerIdentifier)
                return player;
                
            // Check if name matches (case insensitive)
            if (identity.GetName().ToLower() == playerIdentifier.ToLower())
                return player;
        }
        
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Find player ID by name
    protected string FindPlayerIdByName(string playerName)
    {
        // Get all players
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // Check each player
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (!player)
                continue;
                
            PlayerIdentity identity = player.GetIdentity();
            if (!identity)
                continue;
                
            // Check if name matches (case insensitive)
            if (identity.GetName().ToLower() == playerName.ToLower())
                return identity.GetId();
        }
        
        return "";
    }
    
    //------------------------------------------------------------------------------------------------
    // Ban a player
    protected void BanPlayer(string playerId, int duration, string reason)
    {
        // This is a placeholder that would need to be implemented based on the specific
        // banning system used by the server/game
        
        // For DayZ, this would typically use the BanList.xml file or a database
        
        // Example:
        // GetGame().GetAdminEngine().Ban(playerId, duration, reason);
    }
    
    //------------------------------------------------------------------------------------------------
    // Format a playtime in seconds to a readable string
    protected string FormatPlaytime(int seconds)
    {
        if (seconds < 60)
            return seconds + " seconds";
            
        if (seconds < 3600)
            return (seconds / 60) + " minutes, " + (seconds % 60) + " seconds";
            
        if (seconds < 86400)
            return (seconds / 3600) + " hours, " + ((seconds % 3600) / 60) + " minutes";
            
        return (seconds / 86400) + " days, " + ((seconds % 86400) / 3600) + " hours";
    }
    
    //------------------------------------------------------------------------------------------------
    // Format a timestamp to a readable date/time
    protected string FormatTimestamp(int timestamp)
    {
        int year, month, day, hour, minute, second;
        GetTimeFromTimestamp(timestamp, year, month, day, hour, minute, second);
        
        return string.Format("%1-%2-%3 %4:%5:%6", 
                            year, 
                            month.ToString().PadLeft(2, "0"), 
                            day.ToString().PadLeft(2, "0"),
                            hour.ToString().PadLeft(2, "0"),
                            minute.ToString().PadLeft(2, "0"),
                            second.ToString().PadLeft(2, "0"));
    }
    
    //------------------------------------------------------------------------------------------------
    // Get date and time from timestamp
    protected void GetTimeFromTimestamp(int timestamp, out int year, out int month, out int day, out int hour, out int minute, out int second)
    {
        System.GetYearMonthDayUTC(timestamp, year, month, day);
        System.GetHourMinuteSecondUTC(timestamp, hour, minute, second);
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a response back to RCON
    protected void SendRconResponse(string response)
    {
        // This is a placeholder function that would need to be implemented
        // based on the specific RCON system used by the server/game
        
        // For DayZ, this might use something like:
        // GetGame().GetRPCManager().SendRPC("STS_RCONCommands", "RCONResponse", new Param1<string>(response));
    }
    
    //------------------------------------------------------------------------------------------------
    // Send monitoring data to Discord webhook
    protected void SendMonitoringDataToDiscord()
    {
        if (!m_Config.m_bEnableWebhooks)
            return;
            
        // Get all online players
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // Create webhook message
        string title = "Server Status Update";
        string description = string.Format("Players Online: %1\nUptime: %2", 
                                          players.Count(), 
                                          FormatPlaytime(GetGame().GetTickTime() / 1000));
        
        array<string> fields = new array<string>();
        
        // Add server info fields
        fields.Insert("Server Time");
        fields.Insert(FormatTimestamp(System.GetUnixTime()));
        
        fields.Insert("Players Online");
        fields.Insert(players.Count().ToString());
        
        // Add top player stats
        STS_EnhancedPlayerStats topPlayer = GetTopKillPlayer();
        if (topPlayer)
        {
            fields.Insert("Top Player");
            fields.Insert(string.Format("%1 (%2 kills)", topPlayer.m_sPlayerName, topPlayer.m_iKills));
        }
        
        // Send the webhook notification
        STS_WebhookManager.GetInstance().QueueServerNotification(title, description, fields);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the player with the most kills
    protected STS_EnhancedPlayerStats GetTopKillPlayer()
    {
        // Get top players by kills
        array<ref STS_LeaderboardEntry> leaderboard = m_PersistenceManager.GetTopPlayers("kills", 1);
        if (!leaderboard || leaderboard.Count() == 0)
            return null;
            
        // Get the player ID from the leaderboard entry
        string playerId = leaderboard[0].m_sPlayerId;
        
        // Load the full player stats
        return m_PersistenceManager.LoadPlayerStats(playerId);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current timestamp
    protected int GetTime()
    {
        return System.GetUnixTime();
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle config get command
    // Format: sts_config_get <configName>
    protected string HandleConfigGetCommand(array<string> params)
    {
        if (params.Count() < 1)
            return "Usage: " + CMD_CONFIG_GET + " <configName>";
        
        string configName = params[0];
        string configValue = m_Config.GetConfigString(configName);
        
        return string.Format("Config %1: %2", configName, configValue);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle config set command
    // Format: sts_config_set <configName> <value>
    protected string HandleConfigSetCommand(array<string> params)
    {
        if (params.Count() < 2)
            return "Usage: " + CMD_CONFIG_SET + " <configName> <value>";
        
        string configName = params[0];
        string value = params[1];
        
        // Log the request
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Config change request via RCON: %1 = %2", configName, value), "STS_RCONCommands", "HandleConfigSetCommand");
        
        // Try to set the config value
        bool success = m_Config.SetConfigValue(configName, value);
        
        if (success)
        {
            return string.Format("Config %1 set to %2", configName, value);
        }
        else
        {
            return string.Format("Failed to set config %1 to %2", configName, value);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle config list command
    // Format: sts_config_list [filter]
    protected string HandleConfigListCommand(array<string> params)
    {
        string filter = "";
        if (params.Count() > 0)
            filter = params[0].ToLower();
        
        // Get all config values
        map<string, string> configValues = new map<string, string>();
        m_Config.GetConfigValues(configValues);
        
        // Sort keys
        array<string> keys = new array<string>();
        configValues.GetKeyArray(keys);
        keys.Sort();
        
        // Construct response
        string response = "Configuration values:\n";
        response += "-------------------------------------------\n";
        
        int matchCount = 0;
        
        // List all config values
        foreach (string key : keys)
        {
            // Apply filter if specified
            if (filter.Length() > 0 && key.ToLower().IndexOf(filter) == -1)
                continue;
                
            string value = configValues.Get(key);
            response += string.Format("%1: %2\n", key, value);
            matchCount++;
        }
        
        if (matchCount == 0 && filter.Length() > 0)
            response += "No settings matching filter: " + filter + "\n";
            
        response += "-------------------------------------------\n";
        response += string.Format("Total: %1 settings", matchCount);
        
        return response;
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle config reload command
    // Format: sts_config_reload
    protected string HandleConfigReloadCommand(array<string> params)
    {
        // Log the request
        if (m_Logger)
            m_Logger.LogInfo("Config reload request via RCON", "STS_RCONCommands", "HandleConfigReloadCommand");
        
        // Reload the config
        m_Config.LoadConfig();
        
        return "Configuration reloaded from file";
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle config save command
    // Format: sts_config_save
    protected string HandleConfigSaveCommand(array<string> params)
    {
        // Log the request
        if (m_Logger)
            m_Logger.LogInfo("Config save request via RCON", "STS_RCONCommands", "HandleConfigSaveCommand");
        
        // Save the config
        m_Config.SaveConfig();
        
        return "Configuration saved to file";
    }
} 