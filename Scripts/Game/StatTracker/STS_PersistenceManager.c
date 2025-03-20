// STS_PersistenceManager.c
// Manager for persistence operations using the Enfusion Database Framework

class STS_PersistenceManager
{
    // Singleton instance
    private static ref STS_PersistenceManager s_Instance;
    
    // Database manager
    protected ref STS_DatabaseManager m_DatabaseManager;
    
    // Logging system
    protected ref STS_LoggingSystem m_Logger;
    
    // Configuration
    protected string m_sCurrentSessionId;
    protected bool m_bAutosaveEnabled = true;
    protected float m_fLastAutosaveTime = 0;
    protected const float AUTOSAVE_INTERVAL = 300.0; // 5 minutes
    
    // Error handling
    protected bool m_bHealthy = true;
    protected int m_iConsecutiveErrors = 0;
    protected const int ERROR_THRESHOLD = 5;
    protected ref array<string> m_aFailedOperations = new array<string>();
    protected bool m_bUsingFallbackStorage = false;
    protected bool m_bDataCorruptionDetected = false;
    
    // Fallback mechanism for when database is down
    protected ref map<string, string> m_mMemoryBackup = new map<string, string>();
    protected bool m_bMemoryBackupDirty = false;
    protected float m_fLastBackupDumpTime = 0;
    protected const float BACKUP_DUMP_INTERVAL = 300.0; // 5 minutes
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    protected void STS_PersistenceManager()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        
        // Generate a unique session ID
        m_sCurrentSessionId = GenerateSessionId();
        
        m_Logger.LogInfo(string.Format("Persistence Manager initialized. Session ID: %1", m_sCurrentSessionId), 
            "STS_PersistenceManager", "Constructor");
        
        // Initialize the database manager
        m_DatabaseManager = STS_DatabaseManager.GetInstance();
        
        // Auto-initialize with best settings
        if (!m_DatabaseManager.InitializeWithBestSettings())
        {
            m_Logger.LogError("Failed to initialize database with best settings - persistence will not function",
                "STS_PersistenceManager", "Constructor");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_PersistenceManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_PersistenceManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current session ID
    string GetCurrentSessionId()
    {
        return m_sCurrentSessionId;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a unique session ID
    protected string GenerateSessionId()
    {
        // Get current time
        int timestamp = System.GetTickCount();
        
        // Generate a random part
        int random = Math.RandomInt(1000, 9999);
        
        // Combine to create a unique ID
        return string.Format("Session_%1_%2", timestamp, random);
    }
    
    //------------------------------------------------------------------------------------------------
    // Load player statistics from database
    STS_PlayerStats LoadPlayerStats(string playerUID)
    {
        if (!m_DatabaseManager)
            return null;
            
        try
        {
            STS_PlayerStatsRepository repository = m_DatabaseManager.GetPlayerStatsRepository();
            if (!repository)
            {
                m_Logger.LogError("Failed to get player stats repository", 
                    "STS_PersistenceManager", "LoadPlayerStats");
                return null;
            }
            
            // Attempt to load stats
            STS_PlayerStats stats = repository.LoadPlayerStats(playerUID);
            
            if (stats)
            {
                m_Logger.LogInfo(string.Format("Successfully loaded stats for player UID: %1", playerUID), 
                    "STS_PersistenceManager", "LoadPlayerStats");
            }
            else
            {
                m_Logger.LogInfo(string.Format("No existing stats found for player UID: %1", playerUID), 
                    "STS_PersistenceManager", "LoadPlayerStats");
                stats = new STS_PlayerStats(); // Create a fresh stats object
            }
            
            return stats;
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception loading player stats for UID %1: %2", 
                playerUID, e.ToString()), "STS_PersistenceManager", "LoadPlayerStats");
            return new STS_PlayerStats(); // Return empty stats on error
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load player statistics asynchronously
    void LoadPlayerStatsAsync(string playerUID, func<STS_PlayerStats> callback)
    {
        if (!m_DatabaseManager)
        {
            if (callback)
                callback(new STS_PlayerStats());
            return;
        }
            
        try
        {
            STS_PlayerStatsRepository repository = m_DatabaseManager.GetPlayerStatsRepository();
            if (!repository)
            {
                m_Logger.LogError("Failed to get player stats repository", 
                    "STS_PersistenceManager", "LoadPlayerStatsAsync");
                
                if (callback)
                    callback(new STS_PlayerStats());
                return;
            }
            
            // Create a wrapped callback that handles null results
            func<STS_PlayerStats> wrappedCallback = func<STS_PlayerStats>(STS_PlayerStats loadedStats)
            {
                if (!loadedStats)
                    loadedStats = new STS_PlayerStats();
                    
                if (callback)
                    callback(loadedStats);
            };
            
            // Load stats asynchronously
            repository.LoadPlayerStatsAsync(playerUID, wrappedCallback);
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in LoadPlayerStatsAsync for UID %1: %2", 
                playerUID, e.ToString()), "STS_PersistenceManager", "LoadPlayerStatsAsync");
                
            if (callback)
                callback(new STS_PlayerStats());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save player statistics to database
    bool SavePlayerStats(string playerUID, string playerName, STS_PlayerStats stats)
    {
        if (!m_DatabaseManager)
            return false;
            
        try
        {
            STS_PlayerStatsRepository repository = m_DatabaseManager.GetPlayerStatsRepository();
            if (!repository)
            {
                m_Logger.LogError("Failed to get player stats repository", 
                    "STS_PersistenceManager", "SavePlayerStats");
                return false;
            }
            
            // Save stats
            bool success = repository.SavePlayerStats(playerUID, playerName, stats);
            
            if (success)
            {
                m_Logger.LogDebug(string.Format("Successfully saved stats for player %1 (UID: %2)", 
                    playerName, playerUID), "STS_PersistenceManager", "SavePlayerStats");
            }
            else
            {
                m_Logger.LogError(string.Format("Failed to save stats for player %1 (UID: %2)", 
                    playerName, playerUID), "STS_PersistenceManager", "SavePlayerStats");
            }
            
            return success;
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception saving player stats for %1 (UID: %2): %3", 
                playerName, playerUID, e.ToString()), "STS_PersistenceManager", "SavePlayerStats");
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all player statistics (for scoreboard, etc.)
    array<ref STS_PlayerStats> GetAllPlayerStats()
    {
        array<ref STS_PlayerStats> result = new array<ref STS_PlayerStats>();
        
        if (!m_DatabaseManager)
            return result;
            
        try
        {
            STS_PlayerStatsRepository repository = m_DatabaseManager.GetPlayerStatsRepository();
            if (!repository)
            {
                m_Logger.LogError("Failed to get player stats repository", 
                    "STS_PersistenceManager", "GetAllPlayerStats");
                return result;
            }
            
            // Get all player stats
            result = repository.GetAllPlayerStats();
            
            m_Logger.LogDebug(string.Format("Retrieved %1 player stats records", result.Count()), 
                "STS_PersistenceManager", "GetAllPlayerStats");
                
            return result;
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in GetAllPlayerStats: %1", e.ToString()),
                "STS_PersistenceManager", "GetAllPlayerStats");
            return result;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Delete player statistics
    bool DeletePlayerStats(string playerUID)
    {
        if (!m_DatabaseManager)
            return false;
            
        try
        {
            STS_PlayerStatsRepository repository = m_DatabaseManager.GetPlayerStatsRepository();
            if (!repository)
            {
                m_Logger.LogError("Failed to get player stats repository", 
                    "STS_PersistenceManager", "DeletePlayerStats");
                return false;
            }
            
            // Delete player stats
            bool success = repository.DeletePlayerStats(playerUID);
            
            if (success)
            {
                m_Logger.LogInfo(string.Format("Successfully deleted stats for player UID: %1", 
                    playerUID), "STS_PersistenceManager", "DeletePlayerStats");
            }
            else
            {
                m_Logger.LogWarning(string.Format("Failed to delete stats for player UID: %1", 
                    playerUID), "STS_PersistenceManager", "DeletePlayerStats");
            }
            
            return success;
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception deleting player stats for UID %1: %2", 
                playerUID, e.ToString()), "STS_PersistenceManager", "DeletePlayerStats");
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Process scheduled operations (like autosave)
    void ProcessScheduledOperations()
    {
        // Check if it's time for an autosave
        if (m_bAutosaveEnabled)
        {
            float currentTime = System.GetTickCount() / 1000.0;
            if (currentTime - m_fLastAutosaveTime > AUTOSAVE_INTERVAL)
            {
                m_fLastAutosaveTime = currentTime;
                
                m_Logger.LogInfo("Performing scheduled autosave", 
                    "STS_PersistenceManager", "ProcessScheduledOperations");
                
                // Trigger save on manager
                STS_StatTrackingManagerComponent manager = STS_StatTrackingManagerComponent.GetInstance();
                if (manager)
                {
                    manager.SaveAllPlayerStats();
                }
                else
                {
                    m_Logger.LogWarning("Could not perform autosave - stat tracking manager not found", 
                        "STS_PersistenceManager", "ProcessScheduledOperations");
                }
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Enable or disable autosave
    void SetAutosaveEnabled(bool enabled)
    {
        m_bAutosaveEnabled = enabled;
        m_Logger.LogInfo(string.Format("Autosave %1", enabled ? "enabled" : "disabled"), 
            "STS_PersistenceManager", "SetAutosaveEnabled");
    }
    
    //------------------------------------------------------------------------------------------------
    // Shutdown the persistence system
    void Shutdown()
    {
        m_Logger.LogInfo("Persistence Manager shutting down", 
            "STS_PersistenceManager", "Shutdown");
        
        // Save all stats before shutdown
        STS_StatTrackingManagerComponent manager = STS_StatTrackingManagerComponent.GetInstance();
        if (manager)
        {
            manager.SaveAllPlayerStats();
        }
        
        // Shutdown database
        if (m_DatabaseManager)
        {
            m_DatabaseManager.Shutdown();
        }
        
        m_Logger.LogInfo("Persistence Manager shutdown complete", 
            "STS_PersistenceManager", "Shutdown");
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if the persistence manager is healthy
    bool IsHealthy()
    {
        return m_bHealthy;
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset the persistence manager state after errors
    void Reset()
    {
        m_Logger.LogInfo("Resetting persistence manager");
        m_bHealthy = true;
        m_iConsecutiveErrors = 0;
        m_bUsingFallbackStorage = false;
        
        // Try to reconnect to database if needed
        if (m_DatabaseManager)
        {
            bool success = m_DatabaseManager.Reconnect();
            m_Logger.LogInfo("Database reconnection attempt: " + (success ? "successful" : "failed"));
            
            if (success && m_bMemoryBackupDirty)
            {
                // Try to write memory backup to database
                ReplayBackupOperations();
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle persistence error
    protected void HandlePersistenceError(string errorMessage, string operation)
    {
        m_Logger.LogError("Persistence error in " + operation + ": " + errorMessage);
        m_iConsecutiveErrors++;
        
        // Check if we need to switch to fallback storage
        if (m_iConsecutiveErrors >= ERROR_THRESHOLD && !m_bUsingFallbackStorage)
        {
            m_Logger.LogWarning("Too many persistence errors (" + m_iConsecutiveErrors + 
                "), switching to fallback storage");
            m_bUsingFallbackStorage = true;
            m_bHealthy = false;
            
            // Dump memory backup to emergency file
            DumpMemoryBackupToFile();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset error state after successful operation
    protected void ResetErrorState()
    {
        if (m_iConsecutiveErrors > 0)
        {
            m_iConsecutiveErrors = 0;
            m_Logger.LogInfo("Persistence operations working again");
        }
        
        // If we were using fallback storage but database is now working
        if (m_bUsingFallbackStorage && m_iConsecutiveErrors == 0)
        {
            // Try to switch back after some successful operations
            bool canSwitchBack = TryReconnectToDatabase();
            if (canSwitchBack)
            {
                m_bUsingFallbackStorage = false;
                m_bHealthy = true;
                m_Logger.LogInfo("Switching back to database storage");
                
                // Try to replay failed operations
                ReplayBackupOperations();
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Try to reconnect to database
    protected bool TryReconnectToDatabase()
    {
        try
        {
            if (!m_DatabaseManager)
                return false;
                
            bool reconnected = m_DatabaseManager.Reconnect();
            if (reconnected)
            {
                m_Logger.LogInfo("Successfully reconnected to database");
                return true;
            }
            else
            {
                m_Logger.LogWarning("Failed to reconnect to database");
                return false;
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception during database reconnection attempt: " + e.ToString());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Replay backup operations once database is available again
    protected void ReplayBackupOperations()
    {
        if (!m_DatabaseManager || !m_DatabaseManager.IsInitialized())
            return;
            
        m_Logger.LogInfo("Replaying " + m_aFailedOperations.Count() + " backed up operations");
        
        array<string> completedOps = new array<string>();
        
        foreach (string operation : m_aFailedOperations)
        {
            array<string> parts = new array<string>();
            operation.Split(":", parts);
            
            if (parts.Count() < 2)
                continue;
                
            string opType = parts[0];
            string playerId = parts[1];
            
            if (opType == "save")
            {
                string json = m_mMemoryBackup.Get(playerId);
                if (json && !json.IsEmpty())
                {
                    bool success = m_DatabaseManager.SavePlayerData(playerId, json);
                    if (success)
                    {
                        completedOps.Insert(operation);
                        m_Logger.LogDebug("Successfully replayed save operation for player ID: " + playerId);
                    }
                }
            }
        }
        
        // Remove completed operations
        foreach (string completedOp : completedOps)
        {
            m_aFailedOperations.RemoveItem(completedOp);
        }
        
        m_Logger.LogInfo("Replay complete. " + completedOps.Count() + " operations succeeded, " + 
            m_aFailedOperations.Count() + " operations still pending");
            
        // Mark memory backup as clean if all operations are complete
        if (m_aFailedOperations.Count() == 0)
        {
            m_bMemoryBackupDirty = false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Dump memory backup to file in case of emergency
    protected void DumpMemoryBackupToFile()
    {
        try
        {
            // Only dump if dirty and if enough time has passed since last dump
            float currentTime = System.GetTickCount() / 1000.0;
            if (!m_bMemoryBackupDirty || currentTime - m_fLastBackupDumpTime < BACKUP_DUMP_INTERVAL)
                return;
                
            m_fLastBackupDumpTime = currentTime;
            
            // Create backup directory
            string backupDir = "$profile:StatTracker/EmergencyBackups/";
            if (!FileIO.FileExists(backupDir))
            {
                FileIO.MakeDirectory(backupDir);
            }
            
            // Create timestamped backup file
            string timestamp = GetTimestamp();
            string backupFile = backupDir + "backup_" + timestamp + ".json";
            
            // Build JSON array of all player data
            string json = "[";
            int count = 0;
            
            foreach (string playerId, string playerData : m_mMemoryBackup)
            {
                if (count > 0)
                    json += ",";
                    
                json += "{\"id\":\"" + playerId + "\",\"data\":" + playerData + "}";
                count++;
            }
            
            json += "]";
            
            // Write to file
            FileHandle file = FileIO.OpenFile(backupFile, FileMode.WRITE);
            if (file)
            {
                FileIO.FPrintln(file, json);
                FileIO.CloseFile(file);
                m_Logger.LogInfo("Emergency backup created with " + count + " player records: " + backupFile);
            }
            else
            {
                m_Logger.LogError("Failed to create emergency backup file: " + backupFile);
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception creating emergency backup: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Validate JSON data
    protected bool IsValidJson(string json)
    {
        if (json.IsEmpty())
            return false;
            
        // Basic validation - check for opening/closing braces
        if (json.IndexOf("{") != 0 || json.LastIndexOf("}") != json.Length() - 1)
            return false;
            
        // TODO: Add more comprehensive JSON validation if needed
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get timestamp for backup files
    protected string GetTimestamp()
    {
        int year, month, day, hour, minute, second;
        GetHourMinuteSecond(hour, minute, second);
        GetYearMonthDay(year, month, day);
        
        return string.Format("%1-%2-%3_%4-%5-%6", 
            year.ToString(), 
            month.ToString().PadLeft(2, "0"), 
            day.ToString().PadLeft(2, "0"),
            hour.ToString().PadLeft(2, "0"), 
            minute.ToString().PadLeft(2, "0"), 
            second.ToString().PadLeft(2, "0"));
    }
} 