// STS_DataExportManager.c
// Manages exporting player statistics as images and JSON data

class STS_DataExportManager
{
    // Singleton instance
    private static ref STS_DataExportManager s_Instance;
    
    // Config reference
    protected STS_Config m_Config;
    
    // Persistence manager reference
    protected STS_PersistenceManager m_PersistenceManager;
    
    // UI manager reference
    protected STS_UIManager m_UIManager;
    
    // Export file paths
    protected const string EXPORT_JSON_PATH = "$profile:StatTracker/Exports/JSON/";
    protected const string EXPORT_IMAGE_PATH = "$profile:StatTracker/Exports/Images/";
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_DataExportManager()
    {
        m_Config = STS_Config.GetInstance();
        m_PersistenceManager = STS_PersistenceManager.GetInstance();
        m_UIManager = STS_UIManager.GetInstance();
        
        // Ensure export directories exist
        EnsureDirectoriesExist();
        
        Print("[StatTracker] Data Export Manager initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_DataExportManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_DataExportManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Ensure export directories exist
    protected void EnsureDirectoriesExist()
    {
        if (!FileExist(EXPORT_JSON_PATH))
        {
            MakeDirectory(EXPORT_JSON_PATH);
        }
        
        if (!FileExist(EXPORT_IMAGE_PATH))
        {
            MakeDirectory(EXPORT_IMAGE_PATH);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Export player stats as JSON
    string ExportPlayerStatsAsJson(string playerId)
    {
        if (!m_Config.m_bEnableExport)
            return "";
            
        // Load player stats
        STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
        if (!stats)
        {
            Print("[StatTracker] ERROR: Could not load stats for player: " + playerId);
            return "";
        }
        
        // Get player name
        string playerName = stats.m_sPlayerName;
        if (playerName == "")
            playerName = playerId;
            
        // Convert to JSON
        string json = stats.ToJson();
        if (json == "")
        {
            Print("[StatTracker] ERROR: Failed to convert player stats to JSON");
            return "";
        }
        
        // Generate filename
        string timestamp = GetTimestampString();
        string filename = string.Format("%1_%2_%3.json", playerId, playerName.Replace(" ", "_"), timestamp);
        string filepath = EXPORT_JSON_PATH + filename;
        
        // Save to file
        FileHandle file = OpenFile(filepath, FileMode.WRITE);
        if (!file)
        {
            Print("[StatTracker] ERROR: Could not open file for writing: " + filepath);
            return "";
        }
        
        FPrint(file, json);
        CloseFile(file);
        
        Print("[StatTracker] Exported player stats as JSON: " + filepath);
        return filepath;
    }
    
    //------------------------------------------------------------------------------------------------
    // Export player stats as image (placeholder implementation)
    string ExportPlayerStatsAsImage(string playerId)
    {
        if (!m_Config.m_bEnableExport)
            return "";
            
        // Load player stats
        STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
        if (!stats)
        {
            Print("[StatTracker] ERROR: Could not load stats for player: " + playerId);
            return "";
        }
        
        // Get player name
        string playerName = stats.m_sPlayerName;
        if (playerName == "")
            playerName = playerId;
            
        // Generate filename
        string timestamp = GetTimestampString();
        string filename = string.Format("%1_%2_%3.png", playerId, playerName.Replace(" ", "_"), timestamp);
        string filepath = EXPORT_IMAGE_PATH + filename;
        
        // This is a placeholder. In a real implementation, we would:
        // 1. Create an off-screen rendering context
        // 2. Render the player stats UI to it
        // 3. Save the result as an image file
        
        // For now, we'll just create a dummy file
        FileHandle file = OpenFile(filepath, FileMode.WRITE);
        if (!file)
        {
            Print("[StatTracker] ERROR: Could not open file for writing: " + filepath);
            return "";
        }
        
        FPrint(file, "PNG image data would go here");
        CloseFile(file);
        
        Print("[StatTracker] Exported player stats as image: " + filepath);
        return filepath;
    }
    
    //------------------------------------------------------------------------------------------------
    // Export leaderboard as JSON
    string ExportLeaderboardAsJson(string leaderboardName, int count = 10)
    {
        if (!m_Config.m_bEnableExport)
            return "";
            
        // Get leaderboard entries
        array<ref STS_LeaderboardEntry> entries = m_PersistenceManager.GetTopPlayers(leaderboardName, count);
        if (!entries)
        {
            Print("[StatTracker] ERROR: Could not get leaderboard entries for: " + leaderboardName);
            return "";
        }
        
        // Convert to JSON
        JsonSerializer serializer = new JsonSerializer();
        string json;
        
        if (!serializer.WriteToString(entries, false, json))
        {
            Print("[StatTracker] ERROR: Failed to convert leaderboard to JSON");
            return "";
        }
        
        // Generate filename
        string timestamp = GetTimestampString();
        string filename = string.Format("leaderboard_%1_%2.json", leaderboardName, timestamp);
        string filepath = EXPORT_JSON_PATH + filename;
        
        // Save to file
        FileHandle file = OpenFile(filepath, FileMode.WRITE);
        if (!file)
        {
            Print("[StatTracker] ERROR: Could not open file for writing: " + filepath);
            return "";
        }
        
        FPrint(file, json);
        CloseFile(file);
        
        Print("[StatTracker] Exported leaderboard as JSON: " + filepath);
        return filepath;
    }
    
    //------------------------------------------------------------------------------------------------
    // Export leaderboard as image (placeholder implementation)
    string ExportLeaderboardAsImage(string leaderboardName, int count = 10)
    {
        if (!m_Config.m_bEnableExport)
            return "";
            
        // Generate filename
        string timestamp = GetTimestampString();
        string filename = string.Format("leaderboard_%1_%2.png", leaderboardName, timestamp);
        string filepath = EXPORT_IMAGE_PATH + filename;
        
        // This is a placeholder. In a real implementation, we would:
        // 1. Create an off-screen rendering context
        // 2. Render the leaderboard UI to it
        // 3. Save the result as an image file
        
        // For now, we'll just create a dummy file
        FileHandle file = OpenFile(filepath, FileMode.WRITE);
        if (!file)
        {
            Print("[StatTracker] ERROR: Could not open file for writing: " + filepath);
            return "";
        }
        
        FPrint(file, "PNG image data would go here");
        CloseFile(file);
        
        Print("[StatTracker] Exported leaderboard as image: " + filepath);
        return filepath;
    }
    
    //------------------------------------------------------------------------------------------------
    // Export heatmap as image (placeholder implementation)
    string ExportHeatmapAsImage(string heatmapType, int width = 512, int height = 512)
    {
        if (!m_Config.m_bEnableExport || !m_Config.m_bEnableHeatmaps)
            return "";
            
        // Get heatmap manager
        STS_HeatmapManager heatmapManager = STS_HeatmapManager.GetInstance();
        
        // Check if heatmap exists
        STS_Heatmap heatmap = heatmapManager.GetHeatmap(heatmapType);
        if (!heatmap)
        {
            Print("[StatTracker] ERROR: Heatmap not found: " + heatmapType);
            return "";
        }
        
        // Generate filename
        string timestamp = GetTimestampString();
        string filename = string.Format("heatmap_%1_%2.png", heatmapType, timestamp);
        string filepath = EXPORT_IMAGE_PATH + filename;
        
        // This is a placeholder. In a real implementation, we would:
        // 1. Generate a visualization of the heatmap
        // 2. Save it as an image file
        
        // For now, we'll just create a dummy file
        FileHandle file = OpenFile(filepath, FileMode.WRITE);
        if (!file)
        {
            Print("[StatTracker] ERROR: Could not open file for writing: " + filepath);
            return "";
        }
        
        FPrint(file, "PNG image data would go here");
        CloseFile(file);
        
        Print("[StatTracker] Exported heatmap as image: " + filepath);
        return filepath;
    }
    
    //------------------------------------------------------------------------------------------------
    // Export server statistics summary as JSON
    string ExportServerStatsAsJson()
    {
        if (!m_Config.m_bEnableExport)
            return "";
            
        // Get all player IDs
        array<string> playerIds = m_PersistenceManager.GetAllPlayerIds();
        
        // Aggregate stats
        int totalPlayers = playerIds.Count();
        int totalKills = 0;
        int totalDeaths = 0;
        int totalHeadshots = 0;
        int totalPlaytime = 0;
        
        foreach (string playerId : playerIds)
        {
            STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
            if (!stats)
                continue;
                
            totalKills += stats.m_iKills;
            totalDeaths += stats.m_iDeaths;
            totalHeadshots += stats.m_iHeadshotKills;
            totalPlaytime += stats.m_iTotalPlaytimeSeconds;
        }
        
        // Create summary object
        map<string, float> summary = new map<string, float>();
        summary.Insert("timestamp", System.GetUnixTime());
        summary.Insert("players", totalPlayers);
        summary.Insert("kills", totalKills);
        summary.Insert("deaths", totalDeaths);
        summary.Insert("headshots", totalHeadshots);
        summary.Insert("playtime", totalPlaytime);
        summary.Insert("kd_ratio", totalDeaths > 0 ? totalKills / totalDeaths : totalKills);
        summary.Insert("headshot_ratio", totalKills > 0 ? (totalHeadshots / totalKills) * 100 : 0);
        
        // Convert to JSON
        JsonSerializer serializer = new JsonSerializer();
        string json;
        
        if (!serializer.WriteToString(summary, false, json))
        {
            Print("[StatTracker] ERROR: Failed to convert server stats to JSON");
            return "";
        }
        
        // Generate filename
        string timestamp = GetTimestampString();
        string filename = string.Format("server_stats_%1.json", timestamp);
        string filepath = EXPORT_JSON_PATH + filename;
        
        // Save to file
        FileHandle file = OpenFile(filepath, FileMode.WRITE);
        if (!file)
        {
            Print("[StatTracker] ERROR: Could not open file for writing: " + filepath);
            return "";
        }
        
        FPrint(file, json);
        CloseFile(file);
        
        Print("[StatTracker] Exported server stats as JSON: " + filepath);
        return filepath;
    }
    
    //------------------------------------------------------------------------------------------------
    // Export player's timed statistics (daily, weekly, monthly) as JSON
    string ExportPlayerTimedStatsAsJson(string playerId)
    {
        if (!m_Config.m_bEnableExport || !m_Config.m_bEnableTimedStats)
            return "";
            
        // Load player stats
        STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
        if (!stats)
        {
            Print("[StatTracker] ERROR: Could not load stats for player: " + playerId);
            return "";
        }
        
        // Make sure player has timed stats
        if (!stats.m_TimedStats)
        {
            Print("[StatTracker] ERROR: Player has no timed stats: " + playerId);
            return "";
        }
        
        // Create timed stats container
        map<string, Managed> timedStats = new map<string, Managed>();
        
        // Add daily stats
        array<ref STS_StatSnapshot> dailyStats = stats.m_TimedStats.GetAllDailyStats();
        if (dailyStats)
            timedStats.Insert("daily", dailyStats);
            
        // Add weekly stats
        array<ref STS_StatSnapshot> weeklyStats = stats.m_TimedStats.GetAllWeeklyStats();
        if (weeklyStats)
            timedStats.Insert("weekly", weeklyStats);
            
        // Add monthly stats
        array<ref STS_StatSnapshot> monthlyStats = stats.m_TimedStats.GetAllMonthlyStats();
        if (monthlyStats)
            timedStats.Insert("monthly", monthlyStats);
            
        // Convert to JSON
        JsonSerializer serializer = new JsonSerializer();
        string json;
        
        if (!serializer.WriteToString(timedStats, false, json))
        {
            Print("[StatTracker] ERROR: Failed to convert timed stats to JSON");
            return "";
        }
        
        // Get player name
        string playerName = stats.m_sPlayerName;
        if (playerName == "")
            playerName = playerId;
            
        // Generate filename
        string timestamp = GetTimestampString();
        string filename = string.Format("%1_%2_timed_stats_%3.json", playerId, playerName.Replace(" ", "_"), timestamp);
        string filepath = EXPORT_JSON_PATH + filename;
        
        // Save to file
        FileHandle file = OpenFile(filepath, FileMode.WRITE);
        if (!file)
        {
            Print("[StatTracker] ERROR: Could not open file for writing: " + filepath);
            return "";
        }
        
        FPrint(file, json);
        CloseFile(file);
        
        Print("[StatTracker] Exported player timed stats as JSON: " + filepath);
        return filepath;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a timestamp string for filenames
    protected string GetTimestampString()
    {
        TimeAndDate date = System.GetTimeAndDate();
        return string.Format("%1-%2-%3_%4-%5-%6", 
            date.Year, 
            date.Month.ToString().PadLeft(2, "0"), 
            date.Day.ToString().PadLeft(2, "0"),
            date.Hour.ToString().PadLeft(2, "0"),
            date.Minute.ToString().PadLeft(2, "0"),
            date.Second.ToString().PadLeft(2, "0")
        );
    }
} 