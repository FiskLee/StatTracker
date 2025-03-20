// STS_PersonalStatsPortal.c
// Provides a web-accessible portal for player statistics

class STS_PersonalStatsPortalConfig
{
    bool m_bEnabled = true;
    string m_sPortalURLTemplate = "https://stats.example.com/player/{PLAYER_UID}"; // URL template with placeholder
    string m_sAPIEndpoint = "https://api.example.com/stats"; // API endpoint for data queries
    string m_sAPIKey = ""; // API key for authentication
    bool m_bRequireAuth = true; // Require player authentication to view their own stats
    bool m_bAllowPublicProfiles = true; // Allow players to make their profiles public
    int m_iStatsUpdateInterval = 900; // Seconds between stats updates (15 minutes)
    int m_iMaxHistoricalDataPoints = 100; // Maximum historical data points to store per stat
    int m_iMaxAchievements = 50; // Maximum achievements to track
    bool m_bEnableSocialSharing = true; // Enable social media sharing
    bool m_bEnablePlayerComparison = true; // Enable comparison between players
}

class STS_PersonalStatsPortal
{
    // Singleton instance
    private static ref STS_PersonalStatsPortal s_Instance;
    
    // Configuration
    protected ref STS_PersonalStatsPortalConfig m_Config;
    
    // References to other components
    protected ref STS_LoggingSystem m_Logger;
    protected ref STS_DatabaseManager m_DatabaseManager;
    protected ref STS_APIServer m_APIServer;
    
    // Player portal data
    protected ref map<string, ref STS_PlayerPortalData> m_mPlayerPortalData = new map<string, ref STS_PlayerPortalData>();
    
    // Last stats push time
    protected float m_fLastStatsPushTime = 0;
    
    // Stats API client
    protected ref STS_StatsAPIClient m_APIClient;
    
    //------------------------------------------------------------------------------------------------
    protected void STS_PersonalStatsPortal()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Logger.LogInfo("Initializing Personal Stats Portal");
        
        // Initialize configuration
        m_Config = new STS_PersonalStatsPortalConfig();
        
        // Get API server reference
        m_APIServer = STS_APIServer.GetInstance();
        
        // Get database manager reference
        m_DatabaseManager = STS_DatabaseManager.GetInstance();
        
        // Initialize API client
        m_APIClient = new STS_StatsAPIClient(m_Config.m_sAPIEndpoint, m_Config.m_sAPIKey);
        
        // Register API endpoints
        if (m_APIServer && m_Config.m_bEnabled)
        {
            RegisterAPIEndpoints();
        }
        
        // Schedule periodic stats push
        if (m_Config.m_bEnabled)
        {
            GetGame().GetCallqueue().CallLater(PushPlayerStats, m_Config.m_iStatsUpdateInterval * 1000, true);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    static STS_PersonalStatsPortal GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_PersonalStatsPortal();
        }
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Register API endpoints for the personal stats portal
    protected void RegisterAPIEndpoints()
    {
        if (!m_APIServer)
            return;
            
        // Register endpoints
        m_APIServer.RegisterEndpoint("/api/stats/player/{playerUID}", APIGetPlayerStats);
        m_APIServer.RegisterEndpoint("/api/stats/player/{playerUID}/history", APIGetPlayerStatsHistory);
        m_APIServer.RegisterEndpoint("/api/stats/player/{playerUID}/achievements", APIGetPlayerAchievements);
        m_APIServer.RegisterEndpoint("/api/stats/player/{playerUID}/compare/{otherPlayerUID}", APIComparePlayerStats);
        
        m_Logger.LogInfo("Registered Personal Stats Portal API endpoints");
    }
    
    //------------------------------------------------------------------------------------------------
    // API handler: Get player statistics
    protected string APIGetPlayerStats(map<string, string> parameters, string requestBody, string method)
    {
        string playerUID = parameters.Get("playerUID");
        if (playerUID.IsEmpty())
        {
            return "{\"error\":\"Missing player UID\"}";
        }
        
        // Check if player exists
        if (!m_DatabaseManager || !m_DatabaseManager.PlayerExists(playerUID))
        {
            return "{\"error\":\"Player not found\"}";
        }
        
        // Check if player profile is public or if authentication is required
        STS_PlayerPortalData portalData = GetPlayerPortalData(playerUID);
        if (m_Config.m_bRequireAuth && !portalData.m_bIsPublic)
        {
            // In a real implementation, we would check for authentication
            // For this example, we'll assume authentication is valid
        }
        
        // Get player stats
        STS_PlayerStats playerStats = m_DatabaseManager.GetPlayerStats(playerUID);
        if (!playerStats)
        {
            return "{\"error\":\"Player stats not found\"}";
        }
        
        // Convert to JSON
        return playerStats.ToJSON();
    }
    
    //------------------------------------------------------------------------------------------------
    // API handler: Get player statistics history
    protected string APIGetPlayerStatsHistory(map<string, string> parameters, string requestBody, string method)
    {
        string playerUID = parameters.Get("playerUID");
        if (playerUID.IsEmpty())
        {
            return "{\"error\":\"Missing player UID\"}";
        }
        
        // Get the player portal data, which includes history
        STS_PlayerPortalData portalData = GetPlayerPortalData(playerUID);
        if (!portalData)
        {
            return "{\"error\":\"Player data not found\"}";
        }
        
        // Check if player profile is public or if authentication is required
        if (m_Config.m_bRequireAuth && !portalData.m_bIsPublic)
        {
            // In a real implementation, we would check for authentication
            // For this example, we'll assume authentication is valid
        }
        
        // Convert history to JSON
        return portalData.GetHistoryJSON();
    }
    
    //------------------------------------------------------------------------------------------------
    // API handler: Get player achievements
    protected string APIGetPlayerAchievements(map<string, string> parameters, string requestBody, string method)
    {
        string playerUID = parameters.Get("playerUID");
        if (playerUID.IsEmpty())
        {
            return "{\"error\":\"Missing player UID\"}";
        }
        
        // Get the player portal data, which includes achievements
        STS_PlayerPortalData portalData = GetPlayerPortalData(playerUID);
        if (!portalData)
        {
            return "{\"error\":\"Player data not found\"}";
        }
        
        // Check if player profile is public or if authentication is required
        if (m_Config.m_bRequireAuth && !portalData.m_bIsPublic)
        {
            // In a real implementation, we would check for authentication
            // For this example, we'll assume authentication is valid
        }
        
        // Convert achievements to JSON
        return portalData.GetAchievementsJSON();
    }
    
    //------------------------------------------------------------------------------------------------
    // API handler: Compare player statistics
    protected string APIComparePlayerStats(map<string, string> parameters, string requestBody, string method)
    {
        if (!m_Config.m_bEnablePlayerComparison)
        {
            return "{\"error\":\"Player comparison is disabled\"}";
        }
        
        string playerUID = parameters.Get("playerUID");
        string otherPlayerUID = parameters.Get("otherPlayerUID");
        
        if (playerUID.IsEmpty() || otherPlayerUID.IsEmpty())
        {
            return "{\"error\":\"Missing player UIDs\"}";
        }
        
        // Get both player stats
        STS_PlayerStats playerStats = m_DatabaseManager.GetPlayerStats(playerUID);
        STS_PlayerStats otherPlayerStats = m_DatabaseManager.GetPlayerStats(otherPlayerUID);
        
        if (!playerStats || !otherPlayerStats)
        {
            return "{\"error\":\"One or both players not found\"}";
        }
        
        // Check if both profiles are public or if authentication is required
        STS_PlayerPortalData portalData = GetPlayerPortalData(playerUID);
        STS_PlayerPortalData otherPortalData = GetPlayerPortalData(otherPlayerUID);
        
        if (m_Config.m_bRequireAuth && (!portalData.m_bIsPublic || !otherPortalData.m_bIsPublic))
        {
            // In a real implementation, we would check for authentication
            // For this example, we'll assume authentication is valid
        }
        
        // Generate comparison JSON
        string json = "{";
        json += "\"player1\":" + playerStats.ToJSON() + ",";
        json += "\"player2\":" + otherPlayerStats.ToJSON() + ",";
        
        // Add comparison metrics
        json += "\"comparison\":{";
        json += "\"kills_diff\":" + (playerStats.m_iKills - otherPlayerStats.m_iKills).ToString() + ",";
        json += "\"deaths_diff\":" + (playerStats.m_iDeaths - otherPlayerStats.m_iDeaths).ToString() + ",";
        json += "\"kd_ratio_diff\":" + (playerStats.GetKDRatio() - otherPlayerStats.GetKDRatio()).ToString() + ",";
        json += "\"score_diff\":" + (playerStats.CalculateTotalScore() - otherPlayerStats.CalculateTotalScore()).ToString();
        json += "}";
        
        json += "}";
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get or create player portal data
    protected STS_PlayerPortalData GetPlayerPortalData(string playerUID)
    {
        if (m_mPlayerPortalData.Contains(playerUID))
        {
            return m_mPlayerPortalData.Get(playerUID);
        }
        
        // Create new portal data
        STS_PlayerPortalData portalData = new STS_PlayerPortalData();
        portalData.m_sPlayerUID = playerUID;
        portalData.m_bIsPublic = false; // Default to private
        
        // Load from database if available
        if (m_DatabaseManager)
        {
            // In a real implementation, this would load data from the database
            // For this example, we'll just create a new instance
        }
        
        // Store in cache
        m_mPlayerPortalData.Insert(playerUID, portalData);
        
        return portalData;
    }
    
    //------------------------------------------------------------------------------------------------
    // Push player statistics to the web portal
    protected void PushPlayerStats()
    {
        if (!m_Config.m_bEnabled)
            return;
            
        m_Logger.LogDebug("Pushing player statistics to web portal");
        
        try
        {
            // Get all active players
            array<PlayerController> players = GetGame().GetPlayerManager().GetPlayers();
            
            // Update each player's stats
            foreach (PlayerController player : players)
            {
                string playerUID = player.GetUID();
                string playerName = player.GetPlayerName();
                
                if (playerUID.IsEmpty())
                    continue;
                    
                // Get player stats
                STS_PlayerStats playerStats = m_DatabaseManager.GetPlayerStats(playerUID);
                if (!playerStats)
                    continue;
                    
                // Get portal data
                STS_PlayerPortalData portalData = GetPlayerPortalData(playerUID);
                
                // Add current stats to history
                portalData.AddHistoryPoint(playerStats);
                
                // Check for new achievements
                CheckAchievements(playerUID, playerStats, portalData);
                
                // Push to API
                if (m_APIClient)
                {
                    m_APIClient.PushPlayerStats(playerUID, playerName, playerStats, portalData);
                }
            }
            
            m_fLastStatsPushTime = GetGame().GetTickTime();
            m_Logger.LogInfo("Pushed player statistics to web portal");
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception pushing player stats: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Check for new achievements
    protected void CheckAchievements(string playerUID, STS_PlayerStats playerStats, STS_PlayerPortalData portalData)
    {
        // Kill achievements
        if (playerStats.m_iKills >= 100 && !portalData.HasAchievement("kills_100"))
        {
            portalData.AddAchievement("kills_100", "Century Killer", "Kill 100 players");
        }
        
        if (playerStats.m_iKills >= 500 && !portalData.HasAchievement("kills_500"))
        {
            portalData.AddAchievement("kills_500", "Legendary Killer", "Kill 500 players");
        }
        
        if (playerStats.m_iKills >= 1000 && !portalData.HasAchievement("kills_1000"))
        {
            portalData.AddAchievement("kills_1000", "Unstoppable Force", "Kill 1000 players");
        }
        
        // KD ratio achievements
        if (playerStats.GetKDRatio() >= 2.0 && !portalData.HasAchievement("kd_2"))
        {
            portalData.AddAchievement("kd_2", "Double Trouble", "Achieve a K/D ratio of 2.0 or higher");
        }
        
        if (playerStats.GetKDRatio() >= 5.0 && !portalData.HasAchievement("kd_5"))
        {
            portalData.AddAchievement("kd_5", "Dominator", "Achieve a K/D ratio of 5.0 or higher");
        }
        
        // Playtime achievements
        if (playerStats.m_fTotalPlaytime >= 3600 && !portalData.HasAchievement("playtime_1h"))
        {
            portalData.AddAchievement("playtime_1h", "Just Getting Started", "Play for 1 hour");
        }
        
        if (playerStats.m_fTotalPlaytime >= 86400 && !portalData.HasAchievement("playtime_24h"))
        {
            portalData.AddAchievement("playtime_24h", "Dedicated Soldier", "Play for 24 hours");
        }
        
        if (playerStats.m_fTotalPlaytime >= 604800 && !portalData.HasAchievement("playtime_1w"))
        {
            portalData.AddAchievement("playtime_1w", "Veteran", "Play for a week");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a unique portal URL for a player
    string GetPlayerPortalURL(string playerUID)
    {
        if (!m_Config.m_bEnabled || playerUID.IsEmpty())
            return "";
            
        string url = m_Config.m_sPortalURLTemplate;
        url.Replace("{PLAYER_UID}", playerUID);
        
        return url;
    }
    
    //------------------------------------------------------------------------------------------------
    // Set player profile visibility
    void SetPlayerProfileVisibility(string playerUID, bool isPublic)
    {
        if (!m_Config.m_bEnabled || playerUID.IsEmpty())
            return;
            
        // Get portal data
        STS_PlayerPortalData portalData = GetPlayerPortalData(playerUID);
        portalData.m_bIsPublic = isPublic && m_Config.m_bAllowPublicProfiles;
        
        // Save to database if available
        if (m_DatabaseManager)
        {
            // In a real implementation, this would save to the database
            m_Logger.LogInfo(string.Format("Set player %1 profile visibility to %2", 
                playerUID, portalData.m_bIsPublic ? "public" : "private"));
        }
    }
}

//------------------------------------------------------------------------------------------------
// Class to store player portal data including history and achievements
class STS_PlayerPortalData
{
    string m_sPlayerUID;
    bool m_bIsPublic = false;
    
    // History of player stats over time
    protected ref array<ref STS_HistoricalStatPoint> m_aHistory = new array<ref STS_HistoricalStatPoint>();
    
    // Player achievements
    protected ref array<ref STS_Achievement> m_aAchievements = new array<ref STS_Achievement>();
    
    // Settings
    int m_iMaxHistoryPoints = 100;
    int m_iMaxAchievements = 50;
    
    //------------------------------------------------------------------------------------------------
    void STS_PlayerPortalData()
    {
        m_sPlayerUID = "";
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a history point for the player's stats
    void AddHistoryPoint(STS_PlayerStats stats)
    {
        if (!stats)
            return;
            
        // Create a new history point
        STS_HistoricalStatPoint point = new STS_HistoricalStatPoint();
        point.m_iTimestamp = System.GetUnixTime();
        point.m_iKills = stats.m_iKills;
        point.m_iDeaths = stats.m_iDeaths;
        point.m_iTotalXP = stats.m_iTotalXP;
        point.m_iScore = stats.CalculateTotalScore();
        point.m_fKDRatio = stats.GetKDRatio();
        
        // Add to history
        m_aHistory.Insert(point);
        
        // Trim if too many points
        while (m_aHistory.Count() > m_iMaxHistoryPoints)
        {
            m_aHistory.Remove(0);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Add an achievement
    void AddAchievement(string id, string name, string description)
    {
        // Check if already exists
        foreach (STS_Achievement achievement : m_aAchievements)
        {
            if (achievement.m_sId == id)
                return;
        }
        
        // Create achievement
        STS_Achievement newAchievement = new STS_Achievement();
        newAchievement.m_sId = id;
        newAchievement.m_sName = name;
        newAchievement.m_sDescription = description;
        newAchievement.m_iUnlockTimestamp = System.GetUnixTime();
        
        // Add to list
        m_aAchievements.Insert(newAchievement);
        
        // Trim if too many achievements
        while (m_aAchievements.Count() > m_iMaxAchievements)
        {
            m_aAchievements.Remove(0);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if player has an achievement
    bool HasAchievement(string id)
    {
        foreach (STS_Achievement achievement : m_aAchievements)
        {
            if (achievement.m_sId == id)
                return true;
        }
        
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get history as JSON
    string GetHistoryJSON()
    {
        string json = "{\"history\":[";
        
        for (int i = 0; i < m_aHistory.Count(); i++)
        {
            STS_HistoricalStatPoint point = m_aHistory[i];
            
            json += "{";
            json += "\"timestamp\":" + point.m_iTimestamp.ToString() + ",";
            json += "\"kills\":" + point.m_iKills.ToString() + ",";
            json += "\"deaths\":" + point.m_iDeaths.ToString() + ",";
            json += "\"xp\":" + point.m_iTotalXP.ToString() + ",";
            json += "\"score\":" + point.m_iScore.ToString() + ",";
            json += "\"kd_ratio\":" + point.m_fKDRatio.ToString();
            json += "}";
            
            if (i < m_aHistory.Count() - 1)
                json += ",";
        }
        
        json += "]}";
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get achievements as JSON
    string GetAchievementsJSON()
    {
        string json = "{\"achievements\":[";
        
        for (int i = 0; i < m_aAchievements.Count(); i++)
        {
            STS_Achievement achievement = m_aAchievements[i];
            
            json += "{";
            json += "\"id\":\"" + achievement.m_sId + "\",";
            json += "\"name\":\"" + achievement.m_sName + "\",";
            json += "\"description\":\"" + achievement.m_sDescription + "\",";
            json += "\"unlock_timestamp\":" + achievement.m_iUnlockTimestamp.ToString();
            json += "}";
            
            if (i < m_aAchievements.Count() - 1)
                json += ",";
        }
        
        json += "]}";
        return json;
    }
}

//------------------------------------------------------------------------------------------------
// Class for historical stat points
class STS_HistoricalStatPoint
{
    int m_iTimestamp;
    int m_iKills;
    int m_iDeaths;
    int m_iTotalXP;
    int m_iScore;
    float m_fKDRatio;
}

//------------------------------------------------------------------------------------------------
// Class for achievements
class STS_Achievement
{
    string m_sId;
    string m_sName;
    string m_sDescription;
    int m_iUnlockTimestamp;
}

//------------------------------------------------------------------------------------------------
// API client for pushing stats to the web portal
class STS_StatsAPIClient
{
    protected string m_sEndpoint;
    protected string m_sAPIKey;
    protected ref STS_LoggingSystem m_Logger;
    
    //------------------------------------------------------------------------------------------------
    void STS_StatsAPIClient(string endpoint, string apiKey)
    {
        m_sEndpoint = endpoint;
        m_sAPIKey = apiKey;
        m_Logger = STS_LoggingSystem.GetInstance();
    }
    
    //------------------------------------------------------------------------------------------------
    // Push player stats to the API
    bool PushPlayerStats(string playerUID, string playerName, STS_PlayerStats stats, STS_PlayerPortalData portalData)
    {
        if (m_sEndpoint.IsEmpty())
            return false;
            
        // In a real implementation, this would use HTTP requests to push data to the API
        // For this example, we'll just log that it would happen
        m_Logger.LogDebug(string.Format("Would push stats for player %1 (%2) to API endpoint: %3", 
            playerName, playerUID, m_sEndpoint));
            
        return true;
    }
} 