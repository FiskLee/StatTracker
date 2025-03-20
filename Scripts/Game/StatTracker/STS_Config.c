// STS_Config.c
// Configuration manager for the StatTracker system

class STS_Config
{
    // Singleton instance
    private static ref STS_Config s_Instance;
    
    // General settings
    bool m_bEnabled = true;                   // Master switch for the entire system
    bool m_bDebugMode = false;                // Enable debug logging
    
    // Language setting
    string m_sLanguage = "en";                // UI language (en, fr, de, es, ru)
    
    // Data storage settings
    string m_sDataStoragePath = "$profile:StatTracker/Data/";  // Path to save data
    int m_iSaveInterval = 300;                // How often to save data (in seconds)
    int m_iMaxPlayersPerFile = 1000;          // Maximum number of players to store in a single file
    bool m_bCompressData = true;              // Whether to compress data to save space
    
    // Feature toggles
    bool m_bTrackKills = true;                // Track player kills
    bool m_bTrackDeaths = true;               // Track player deaths
    bool m_bTrackDamage = true;               // Track damage dealt/received
    bool m_bTrackHeadshots = true;            // Track headshot statistics
    bool m_bTrackPlaytime = true;             // Track player session time
    bool m_bTrackWeapons = true;              // Track weapon usage
    bool m_bTrackItems = true;                // Track item usage/collection
    bool m_bTrackMovement = true;             // Track player movement
    bool m_bTrackLocations = true;            // Track player locations
    bool m_bTrackEconomy = true;              // Track player economy (money earned/spent)
    bool m_bTrackAchievements = true;         // Track player achievements
    bool m_bEnableLeaderboards = true;        // Enable leaderboard functionality
    bool m_bEnableTimedStats = true;          // Enable time-based stat tracking (daily/weekly/monthly)
    bool m_bEnableVisualization = true;       // Enable data visualization
    bool m_bEnableWebhooks = true;            // Enable webhook notifications
    bool m_bEnableHeatmaps = true;            // Enable heatmap generation
    bool m_bEnableStatsAPI = true;            // Enable Stats API
    
    // Leaderboard settings
    int m_iLeaderboardRefreshInterval = 900;  // How often to refresh leaderboards (in seconds)
    int m_iLeaderboardDisplayCount = 10;      // How many players to display on leaderboards
    
    // Webhook settings
    string m_sWebhookUrl = "";                // URL for webhook notifications
    int m_iWebhookRateLimit = 10;             // Maximum webhook calls per minute
    bool m_bWebhookNotifyKills = true;        // Send webhook for kills
    bool m_bWebhookNotifyAchievements = true; // Send webhook for achievements
    bool m_bWebhookNotifyRecords = true;      // Send webhook for new records
    
    // Timed stats settings
    bool m_bResetDailyStats = true;           // Whether to reset daily stats
    bool m_bResetWeeklyStats = true;          // Whether to reset weekly stats
    bool m_bResetMonthlyStats = true;         // Whether to reset monthly stats
    int m_iDailyResetHour = 0;                // Hour of day to reset daily stats (0-23)
    int m_iWeeklyResetDay = 1;                // Day of week to reset weekly stats (1-7, Monday=1)
    int m_iMonthlyResetDay = 1;               // Day of month to reset monthly stats (1-31)
    
    // Data export settings
    bool m_bEnableJsonExport = true;          // Enable JSON export
    bool m_bEnableImageExport = true;         // Enable image export
    string m_sExportPath = "$profile:StatTracker/Exports/"; // Path for exports
    
    // Player stats snapshot settings
    int m_iMaxSnapshotsPerPlayer = 30;        // Maximum number of snapshots to keep per player
    
    // Heatmap settings
    int m_iHeatmapResolution = 512;           // Resolution of generated heatmaps
    float m_fHeatmapOpacity = 0.7;            // Opacity of heatmap overlays (0.0-1.0)
    
    // API settings
    int m_iApiPort = 8080;                    // Port for Stats API server
    bool m_bApiRequireAuth = true;            // Require authentication for API requests
    string m_sApiAuthToken = "";              // Authorization token for API access
    int m_iApiRateLimit = 60;                 // Maximum API requests per minute
    
    // Hot-reloading settings
    bool m_bEnableHotReload = true;           // Enable hot-reloading of configuration
    int m_iConfigCheckInterval = 60;          // How often to check for config changes (in seconds)
    float m_fLastConfigLoadTime = 0;          // Last time config was loaded (internal use)
    string m_sConfigModifyTime = "";          // Last config file modification time

    // Event notification system for config changes
    protected ref array<func> m_aConfigChangeCallbacks = new array<func>();

    // Logger
    protected STS_LoggingSystem m_Logger;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_Config()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        
        // Load config from file, or use defaults if file doesn't exist
        LoadConfig();
        
        // Set up hot-reloading timer if enabled
        if (m_bEnableHotReload && IsMissionHost())
        {
            GetGame().GetCallqueue().CallLater(CheckConfigUpdates, m_iConfigCheckInterval * 1000, true);
        }
        
        Print("[StatTracker] Config initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_Config GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_Config();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load configuration from file
    void LoadConfig()
    {
        string configPath = "$profile:StatTracker/config.json";
        
        // Check if config file exists
        if (!FileExist(configPath))
        {
            if (m_Logger)
                m_Logger.LogWarning("Config file not found, using defaults", "STS_Config", "LoadConfig");
            else
                Print("[StatTracker] Config file not found, using defaults");
                
            SaveConfig(); // Create default config file
            return;
        }
        
        // Store last modified time
        m_sConfigModifyTime = GetFileModifiedTime(configPath);
        m_fLastConfigLoadTime = GetGame().GetTime();
        
        // Open the file
        FileHandle file = OpenFile(configPath, FileMode.READ);
        if (!file)
        {
            if (m_Logger)
                m_Logger.LogError("Error opening config file, using defaults", "STS_Config", "LoadConfig");
            else
                Print("[StatTracker] Error opening config file, using defaults");
            return;
        }
        
        string jsonStr;
        string line;
        
        // Read entire file into string
        while (FGets(file, line) >= 0)
        {
            jsonStr += line;
        }
        
        CloseFile(file);
        
        // Store original values before parsing to detect changes
        map<string, string> originalValues = new map<string, string>();
        GetConfigValues(originalValues);
        
        // Parse JSON
        JsonSerializer js = new JsonSerializer();
        
        string errorMsg;
        bool parseSuccess = js.ReadFromString(this, jsonStr, errorMsg);
        
        if (!parseSuccess)
        {
            if (m_Logger)
                m_Logger.LogError("Error parsing config: " + errorMsg, "STS_Config", "LoadConfig");
            else
                Print("[StatTracker] Error parsing config: " + errorMsg);
            return;
        }
        
        // Check which values changed and notify subscribers
        map<string, string> newValues = new map<string, string>();
        GetConfigValues(newValues);
        map<string, string> changedValues = new map<string, string>();
        
        array<string> keys = new array<string>();
        newValues.GetKeyArray(keys);
        
        foreach (string key : keys)
        {
            if (!originalValues.Contains(key) || originalValues[key] != newValues[key])
            {
                changedValues.Insert(key, newValues[key]);
            }
        }
        
        // Notify subscribers of changes
        if (changedValues.Count() > 0)
        {
            NotifyConfigChanged(changedValues);
        }
        
        if (m_Logger)
            m_Logger.LogInfo("Config loaded successfully", "STS_Config", "LoadConfig");
        else
            Print("[StatTracker] Config loaded successfully");
    }
    
    //------------------------------------------------------------------------------------------------
    // Save configuration to file
    void SaveConfig()
    {
        string configDir = "$profile:StatTracker/";
        string configPath = configDir + "config.json";
        
        // Make sure directory exists
        if (!FileExist(configDir))
        {
            MakeDirectory(configDir);
        }
        
        // Open the file
        FileHandle file = OpenFile(configPath, FileMode.WRITE);
        if (!file)
        {
            if (m_Logger)
                m_Logger.LogError("Error opening config file for writing", "STS_Config", "SaveConfig");
            else
                Print("[StatTracker] Error opening config file for writing");
            return;
        }
        
        // Serialize to JSON
        JsonSerializer js = new JsonSerializer();
        string jsonStr;
        
        bool serializeSuccess = js.WriteToString(this, false, jsonStr);
        
        if (!serializeSuccess)
        {
            if (m_Logger)
                m_Logger.LogError("Error serializing config to JSON", "STS_Config", "SaveConfig");
            else
                Print("[StatTracker] Error serializing config to JSON");
            CloseFile(file);
            return;
        }
        
        // Write to file
        FPrint(file, jsonStr);
        CloseFile(file);
        
        // Update modified time
        m_sConfigModifyTime = GetFileModifiedTime(configPath);
        
        if (m_Logger)
            m_Logger.LogInfo("Config saved successfully", "STS_Config", "SaveConfig");
        else
            Print("[StatTracker] Config saved successfully");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get config value as string for display
    string GetConfigString(string configName)
    {
        // General settings
        if (configName == "Enabled") return m_bEnabled.ToString();
        if (configName == "DebugMode") return m_bDebugMode.ToString();
        if (configName == "Language") return m_sLanguage;
        
        // Data storage settings
        if (configName == "DataStoragePath") return m_sDataStoragePath;
        if (configName == "SaveInterval") return m_iSaveInterval.ToString() + " seconds";
        if (configName == "MaxPlayersPerFile") return m_iMaxPlayersPerFile.ToString();
        if (configName == "CompressData") return m_bCompressData.ToString();
        
        // Feature toggles
        if (configName == "TrackKills") return m_bTrackKills.ToString();
        if (configName == "TrackDeaths") return m_bTrackDeaths.ToString();
        if (configName == "TrackDamage") return m_bTrackDamage.ToString();
        if (configName == "TrackHeadshots") return m_bTrackHeadshots.ToString();
        if (configName == "TrackPlaytime") return m_bTrackPlaytime.ToString();
        if (configName == "TrackWeapons") return m_bTrackWeapons.ToString();
        if (configName == "TrackItems") return m_bTrackItems.ToString();
        if (configName == "TrackMovement") return m_bTrackMovement.ToString();
        if (configName == "TrackLocations") return m_bTrackLocations.ToString();
        if (configName == "TrackEconomy") return m_bTrackEconomy.ToString();
        if (configName == "TrackAchievements") return m_bTrackAchievements.ToString();
        if (configName == "EnableLeaderboards") return m_bEnableLeaderboards.ToString();
        if (configName == "EnableTimedStats") return m_bEnableTimedStats.ToString();
        if (configName == "EnableVisualization") return m_bEnableVisualization.ToString();
        if (configName == "EnableWebhooks") return m_bEnableWebhooks.ToString();
        if (configName == "EnableHeatmaps") return m_bEnableHeatmaps.ToString();
        if (configName == "EnableStatsAPI") return m_bEnableStatsAPI.ToString();
        
        // Leaderboard settings
        if (configName == "LeaderboardRefreshInterval") return m_iLeaderboardRefreshInterval.ToString() + " seconds";
        if (configName == "LeaderboardDisplayCount") return m_iLeaderboardDisplayCount.ToString();
        
        // Webhook settings
        if (configName == "WebhookUrl") return m_sWebhookUrl;
        if (configName == "WebhookRateLimit") return m_iWebhookRateLimit.ToString() + " per minute";
        if (configName == "WebhookNotifyKills") return m_bWebhookNotifyKills.ToString();
        if (configName == "WebhookNotifyAchievements") return m_bWebhookNotifyAchievements.ToString();
        if (configName == "WebhookNotifyRecords") return m_bWebhookNotifyRecords.ToString();
        
        // Timed stats settings
        if (configName == "ResetDailyStats") return m_bResetDailyStats.ToString();
        if (configName == "ResetWeeklyStats") return m_bResetWeeklyStats.ToString();
        if (configName == "ResetMonthlyStats") return m_bResetMonthlyStats.ToString();
        if (configName == "DailyResetHour") return m_iDailyResetHour.ToString() + ":00";
        if (configName == "WeeklyResetDay") return GetDayName(m_iWeeklyResetDay);
        if (configName == "MonthlyResetDay") return m_iMonthlyResetDay.ToString();
        
        // Data export settings
        if (configName == "EnableJsonExport") return m_bEnableJsonExport.ToString();
        if (configName == "EnableImageExport") return m_bEnableImageExport.ToString();
        if (configName == "ExportPath") return m_sExportPath;
        
        // Player stats snapshot settings
        if (configName == "MaxSnapshotsPerPlayer") return m_iMaxSnapshotsPerPlayer.ToString();
        
        // Heatmap settings
        if (configName == "HeatmapResolution") return m_iHeatmapResolution.ToString() + "x" + m_iHeatmapResolution.ToString();
        if (configName == "HeatmapOpacity") return (m_fHeatmapOpacity * 100).ToString() + "%";
        
        // API settings
        if (configName == "ApiPort") return m_iApiPort.ToString();
        if (configName == "ApiRequireAuth") return m_bApiRequireAuth.ToString();
        if (configName == "ApiRateLimit") return m_iApiRateLimit.ToString() + " per minute";
        
        // Hot-reloading settings
        if (configName == "EnableHotReload") return m_bEnableHotReload.ToString();
        if (configName == "ConfigCheckInterval") return m_iConfigCheckInterval.ToString() + " seconds";
        
        return "Unknown setting: " + configName;
    }
    
    //------------------------------------------------------------------------------------------------
    // Set config value from string (for admin commands)
    bool SetConfigValue(string configName, string value)
    {
        // General settings
        if (configName == "Enabled") m_bEnabled = value.ToInt() != 0;
        else if (configName == "DebugMode") m_bDebugMode = value.ToInt() != 0;
        else if (configName == "Language") m_sLanguage = value;
        
        // Data storage settings
        else if (configName == "DataStoragePath") m_sDataStoragePath = value;
        else if (configName == "SaveInterval") m_iSaveInterval = value.ToInt();
        else if (configName == "MaxPlayersPerFile") m_iMaxPlayersPerFile = value.ToInt();
        else if (configName == "CompressData") m_bCompressData = value.ToInt() != 0;
        
        // Feature toggles
        else if (configName == "TrackKills") m_bTrackKills = value.ToInt() != 0;
        else if (configName == "TrackDeaths") m_bTrackDeaths = value.ToInt() != 0;
        else if (configName == "TrackDamage") m_bTrackDamage = value.ToInt() != 0;
        else if (configName == "TrackHeadshots") m_bTrackHeadshots = value.ToInt() != 0;
        else if (configName == "TrackPlaytime") m_bTrackPlaytime = value.ToInt() != 0;
        else if (configName == "TrackWeapons") m_bTrackWeapons = value.ToInt() != 0;
        else if (configName == "TrackItems") m_bTrackItems = value.ToInt() != 0;
        else if (configName == "TrackMovement") m_bTrackMovement = value.ToInt() != 0;
        else if (configName == "TrackLocations") m_bTrackLocations = value.ToInt() != 0;
        else if (configName == "TrackEconomy") m_bTrackEconomy = value.ToInt() != 0;
        else if (configName == "TrackAchievements") m_bTrackAchievements = value.ToInt() != 0;
        else if (configName == "EnableLeaderboards") m_bEnableLeaderboards = value.ToInt() != 0;
        else if (configName == "EnableTimedStats") m_bEnableTimedStats = value.ToInt() != 0;
        else if (configName == "EnableVisualization") m_bEnableVisualization = value.ToInt() != 0;
        else if (configName == "EnableWebhooks") m_bEnableWebhooks = value.ToInt() != 0;
        else if (configName == "EnableHeatmaps") m_bEnableHeatmaps = value.ToInt() != 0;
        else if (configName == "EnableStatsAPI") m_bEnableStatsAPI = value.ToInt() != 0;
        
        // Leaderboard settings
        else if (configName == "LeaderboardRefreshInterval") m_iLeaderboardRefreshInterval = value.ToInt();
        else if (configName == "LeaderboardDisplayCount") m_iLeaderboardDisplayCount = value.ToInt();
        
        // Webhook settings
        else if (configName == "WebhookUrl") m_sWebhookUrl = value;
        else if (configName == "WebhookRateLimit") m_iWebhookRateLimit = value.ToInt();
        else if (configName == "WebhookNotifyKills") m_bWebhookNotifyKills = value.ToInt() != 0;
        else if (configName == "WebhookNotifyAchievements") m_bWebhookNotifyAchievements = value.ToInt() != 0;
        else if (configName == "WebhookNotifyRecords") m_bWebhookNotifyRecords = value.ToInt() != 0;
        
        // Timed stats settings
        else if (configName == "ResetDailyStats") m_bResetDailyStats = value.ToInt() != 0;
        else if (configName == "ResetWeeklyStats") m_bResetWeeklyStats = value.ToInt() != 0;
        else if (configName == "ResetMonthlyStats") m_bResetMonthlyStats = value.ToInt() != 0;
        else if (configName == "DailyResetHour") m_iDailyResetHour = value.ToInt();
        else if (configName == "WeeklyResetDay") m_iWeeklyResetDay = ParseDayName(value);
        else if (configName == "MonthlyResetDay") m_iMonthlyResetDay = value.ToInt();
        
        // Data export settings
        else if (configName == "EnableJsonExport") m_bEnableJsonExport = value.ToInt() != 0;
        else if (configName == "EnableImageExport") m_bEnableImageExport = value.ToInt() != 0;
        else if (configName == "ExportPath") m_sExportPath = value;
        
        // Player stats snapshot settings
        else if (configName == "MaxSnapshotsPerPlayer") m_iMaxSnapshotsPerPlayer = value.ToInt();
        
        // Heatmap settings
        else if (configName == "HeatmapResolution") m_iHeatmapResolution = value.ToInt();
        else if (configName == "HeatmapOpacity") m_fHeatmapOpacity = value.ToFloat();
        
        // API settings
        else if (configName == "ApiPort") m_iApiPort = value.ToInt();
        else if (configName == "ApiRequireAuth") m_bApiRequireAuth = value.ToInt() != 0;
        else if (configName == "ApiRateLimit") m_iApiRateLimit = value.ToInt();
        
        // Hot-reloading settings
        else if (configName == "EnableHotReload") m_bEnableHotReload = value.ToInt() != 0;
        else if (configName == "ConfigCheckInterval") m_iConfigCheckInterval = value.ToInt();
        
        else
            return false;
            
        // Save updated config
        SaveConfig();
        
        // Notify single setting change
        map<string, string> changedValues = new map<string, string>();
        changedValues.Insert(configName, value);
        NotifyConfigChanged(changedValues);
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Register for config change notifications
    void RegisterConfigChangeCallback(func callback)
    {
        if (!m_aConfigChangeCallbacks.Contains(callback))
        {
            m_aConfigChangeCallbacks.Insert(callback);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Unregister from config change notifications
    void UnregisterConfigChangeCallback(func callback)
    {
        int index = m_aConfigChangeCallbacks.Find(callback);
        if (index >= 0)
        {
            m_aConfigChangeCallbacks.Remove(index);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Notify subscribers of config changes
    protected void NotifyConfigChanged(map<string, string> changedValues)
    {
        // Skip if no subscribers
        if (m_aConfigChangeCallbacks.Count() == 0)
            return;
            
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Notifying %1 subscribers of %2 config changes", m_aConfigChangeCallbacks.Count(), changedValues.Count()), "STS_Config", "NotifyConfigChanged");
            
        // Call each subscriber
        foreach (func callback : m_aConfigChangeCallbacks)
        {
            callback.Invoke(changedValues);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Check for config file changes
    protected void CheckConfigUpdates()
    {
        if (!m_bEnableHotReload || !IsMissionHost())
            return;
            
        string configPath = "$profile:StatTracker/config.json";
        
        // Check if config file exists
        if (!FileExist(configPath))
            return;
            
        // Get last modified time
        string currentModifyTime = GetFileModifiedTime(configPath);
        
        // Check if file has been modified since last load
        if (currentModifyTime != m_sConfigModifyTime)
        {
            if (m_Logger)
                m_Logger.LogInfo("Config file has changed, reloading", "STS_Config", "CheckConfigUpdates");
            else
                Print("[StatTracker] Config file has changed, reloading");
                
            // Reload config
            LoadConfig();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Get all config values as key-value pairs
    protected void GetConfigValues(out map<string, string> values)
    {
        values.Insert("Enabled", m_bEnabled.ToString());
        values.Insert("DebugMode", m_bDebugMode.ToString());
        values.Insert("Language", m_sLanguage);
        values.Insert("DataStoragePath", m_sDataStoragePath);
        values.Insert("SaveInterval", m_iSaveInterval.ToString());
        values.Insert("MaxPlayersPerFile", m_iMaxPlayersPerFile.ToString());
        values.Insert("CompressData", m_bCompressData.ToString());
        values.Insert("TrackKills", m_bTrackKills.ToString());
        values.Insert("TrackDeaths", m_bTrackDeaths.ToString());
        values.Insert("TrackDamage", m_bTrackDamage.ToString());
        values.Insert("TrackHeadshots", m_bTrackHeadshots.ToString());
        values.Insert("TrackPlaytime", m_bTrackPlaytime.ToString());
        values.Insert("TrackWeapons", m_bTrackWeapons.ToString());
        values.Insert("TrackItems", m_bTrackItems.ToString());
        values.Insert("TrackMovement", m_bTrackMovement.ToString());
        values.Insert("TrackLocations", m_bTrackLocations.ToString());
        values.Insert("TrackEconomy", m_bTrackEconomy.ToString());
        values.Insert("TrackAchievements", m_bTrackAchievements.ToString());
        values.Insert("EnableLeaderboards", m_bEnableLeaderboards.ToString());
        values.Insert("EnableTimedStats", m_bEnableTimedStats.ToString());
        values.Insert("EnableVisualization", m_bEnableVisualization.ToString());
        values.Insert("EnableWebhooks", m_bEnableWebhooks.ToString());
        values.Insert("EnableHeatmaps", m_bEnableHeatmaps.ToString());
        values.Insert("EnableStatsAPI", m_bEnableStatsAPI.ToString());
        values.Insert("LeaderboardRefreshInterval", m_iLeaderboardRefreshInterval.ToString());
        values.Insert("LeaderboardDisplayCount", m_iLeaderboardDisplayCount.ToString());
        values.Insert("WebhookUrl", m_sWebhookUrl);
        values.Insert("WebhookRateLimit", m_iWebhookRateLimit.ToString());
        values.Insert("WebhookNotifyKills", m_bWebhookNotifyKills.ToString());
        values.Insert("WebhookNotifyAchievements", m_bWebhookNotifyAchievements.ToString());
        values.Insert("WebhookNotifyRecords", m_bWebhookNotifyRecords.ToString());
        values.Insert("ResetDailyStats", m_bResetDailyStats.ToString());
        values.Insert("ResetWeeklyStats", m_bResetWeeklyStats.ToString());
        values.Insert("ResetMonthlyStats", m_bResetMonthlyStats.ToString());
        values.Insert("DailyResetHour", m_iDailyResetHour.ToString());
        values.Insert("WeeklyResetDay", m_iWeeklyResetDay.ToString());
        values.Insert("MonthlyResetDay", m_iMonthlyResetDay.ToString());
        values.Insert("EnableJsonExport", m_bEnableJsonExport.ToString());
        values.Insert("EnableImageExport", m_bEnableImageExport.ToString());
        values.Insert("ExportPath", m_sExportPath);
        values.Insert("MaxSnapshotsPerPlayer", m_iMaxSnapshotsPerPlayer.ToString());
        values.Insert("HeatmapResolution", m_iHeatmapResolution.ToString());
        values.Insert("HeatmapOpacity", m_fHeatmapOpacity.ToString());
        values.Insert("ApiPort", m_iApiPort.ToString());
        values.Insert("ApiRequireAuth", m_bApiRequireAuth.ToString());
        values.Insert("ApiRateLimit", m_iApiRateLimit.ToString());
        values.Insert("EnableHotReload", m_bEnableHotReload.ToString());
        values.Insert("ConfigCheckInterval", m_iConfigCheckInterval.ToString());
    }
    
    //------------------------------------------------------------------------------------------------
    // Get file modified time as string
    protected string GetFileModifiedTime(string filePath)
    {
        FileAttribute attributes;
        GetFileAttributes(filePath, attributes);
        return attributes.m_Timestamp.ToString();
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Check if we're the server or server host
    protected bool IsMissionHost()
    {
        return GetGame().IsMissionHost();
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Get day name from day number
    protected string GetDayName(int dayNumber)
    {
        switch (dayNumber)
        {
            case 1: return "Monday";
            case 2: return "Tuesday";
            case 3: return "Wednesday";
            case 4: return "Thursday";
            case 5: return "Friday";
            case 6: return "Saturday";
            case 7: return "Sunday";
            default: return "Monday";
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Parse day name to day number
    protected int ParseDayName(string dayName)
    {
        dayName = dayName.ToLower();
        
        if (dayName == "monday" || dayName == "mon" || dayName == "1") return 1;
        if (dayName == "tuesday" || dayName == "tue" || dayName == "2") return 2;
        if (dayName == "wednesday" || dayName == "wed" || dayName == "3") return 3;
        if (dayName == "thursday" || dayName == "thu" || dayName == "4") return 4;
        if (dayName == "friday" || dayName == "fri" || dayName == "5") return 5;
        if (dayName == "saturday" || dayName == "sat" || dayName == "6") return 6;
        if (dayName == "sunday" || dayName == "sun" || dayName == "7") return 7;
        
        return 1; // Default to Monday
    }
}

//------------------------------------------------------------------------------------------------
// Webhook configuration class
class STS_WebhookConfig
{
    string m_sName;
    string m_sURL;
    string m_sType;
    bool m_bEnabled;
    ref array<string> m_aEvents = new array<string>();
    
    void STS_WebhookConfig()
    {
        m_sName = "Webhook";
        m_sURL = "";
        m_sType = "discord";
        m_bEnabled = false;
        
        // Default events to send
        m_aEvents.Insert("achievement_earned");
        m_aEvents.Insert("leaderboard_position_changed");
        m_aEvents.Insert("killstreak_significant");
    }
}

//------------------------------------------------------------------------------------------------
// Achievement configuration class
class STS_AchievementConfig
{
    string m_sID;
    string m_sName;
    string m_sDescription;
    string m_sStatType;
    float m_fThreshold;
    int m_iXPReward;
    
    void STS_AchievementConfig()
    {
        m_sID = "";
        m_sName = "";
        m_sDescription = "";
        m_sStatType = "";
        m_fThreshold = 0;
        m_iXPReward = 0;
    }
} 