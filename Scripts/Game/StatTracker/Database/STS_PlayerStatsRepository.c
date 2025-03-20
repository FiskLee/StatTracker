// STS_PlayerStatsRepository.c
// Repository for player statistics database operations

class STS_PlayerStatsRepository
{
    // Database context
    protected EDF_DbContext m_DbContext;
    
    // Repository for player stats entities
    protected ref EDF_DbRepository<STS_PlayerStatsEntity> m_Repository;
    
    // Logger for diagnostics
    protected STS_LoggingSystem m_Logger;
    
    //------------------------------------------------------------------------------------------------
    void STS_PlayerStatsRepository(EDF_DbContext dbContext)
    {
        m_DbContext = dbContext;
        m_Repository = EDF_DbEntityHelper<STS_PlayerStatsEntity>.GetRepository(m_DbContext);
        m_Logger = STS_LoggingSystem.GetInstance();
        
        if (!m_Repository)
        {
            m_Logger.LogError("Failed to create player stats repository", "STS_PlayerStatsRepository", "Constructor");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save or update player statistics with full error handling, validation and transactional support
    bool SavePlayerStats(string playerUID, string playerName, STS_PlayerStats stats)
    {
        // Start measuring operation time for performance monitoring
        float startTime = System.GetTickCount() / 1000.0;
        
        // Input validation
        if (!m_Repository)
        {
            if (m_Logger)
                m_Logger.LogError("Repository is null - database connection may be broken", 
                    "STS_PlayerStatsRepository", "SavePlayerStats");
            return false;
        }
        
        if (playerUID.IsEmpty())
        {
            if (m_Logger)
                m_Logger.LogError("Cannot save player stats with empty UID", 
                    "STS_PlayerStatsRepository", "SavePlayerStats");
            return false;
        }
        
        if (!stats)
        {
            if (m_Logger)
                m_Logger.LogError(string.Format("Cannot save null stats for player %1 (UID: %2)", 
                    playerName, playerUID), "STS_PlayerStatsRepository", "SavePlayerStats");
            return false;
        }
        
        // Sanitize player name if empty
        if (playerName.IsEmpty())
        {
            playerName = "Unknown Player";
            if (m_Logger)
                m_Logger.LogWarning(string.Format("Empty player name for UID %1 - using '%2'", 
                    playerUID, playerName), "STS_PlayerStatsRepository", "SavePlayerStats");
        }
        
        try
        {
            // Validate stats data before saving
            if (!ValidatePlayerStats(stats))
            {
                if (m_Logger)
                    m_Logger.LogError(string.Format("Invalid player stats data for %1 (UID: %2) - aborting save", 
                        playerName, playerUID), "STS_PlayerStatsRepository", "SavePlayerStats");
                return false;
            }
            
            // Start a transaction for data integrity
            EDF_DbTransaction transaction = m_DbContext.BeginTransaction();
            if (!transaction)
            {
                if (m_Logger)
                    m_Logger.LogError(string.Format("Failed to create transaction for saving %1 (UID: %2)", 
                        playerName, playerUID), "STS_PlayerStatsRepository", "SavePlayerStats");
                return false;
            }
            
            bool success = false;
            
            try
            {
                // Check if player already exists in the database with retry logic
                STS_PlayerStatsEntity entity = null;
                EDF_DbFindCondition condition = EDF_DbFind.Field("m_sPlayerUID").Equals(playerUID);
                
                // Retry query up to 3 times in case of temporary failures
                for (int attempt = 1; attempt <= 3; attempt++)
                {
                    try
                    {
                        entity = m_Repository.FindFirst(condition);
                        break; // Success, exit retry loop
                    }
                    catch (Exception queryEx)
                    {
                        if (attempt == 3)
                        {
                            // Rethrow on final attempt
                            throw queryEx;
                        }
                        
                        if (m_Logger)
                            m_Logger.LogWarning(string.Format("Query attempt %1 failed for UID %2: %3", 
                                attempt, playerUID, queryEx.ToString()), 
                                "STS_PlayerStatsRepository", "SavePlayerStats");
                        
                        // Sleep before retry with increasing delay
                        int delayMs = 50 * attempt;
                        Sleep(delayMs);
                    }
                }
                
                // Always log operation details at debug level
                if (m_Logger)
                    m_Logger.LogDebug(string.Format("Saving stats for player %1 (UID: %2) - exists: %3", 
                        playerName, playerUID, entity != null), "STS_PlayerStatsRepository", "SavePlayerStats");
                
                EDF_EDbOperationStatusCode statusCode;
                
                if (entity)
                {
                    // Create a backup of existing entity values for logging significant changes
                    STS_PlayerStats oldStats = entity.ToPlayerStats();
                    
                    // Update existing entity
                    entity.UpdateFromStats(playerName, stats);
                    
                    // Record update timestamp
                    entity.m_fLastUpdateTime = System.GetTickCount() / 1000.0;
                    
                    // Update the entity
                    statusCode = m_Repository.AddOrUpdate(entity);
                    
                    // Log significant changes for debugging and auditing
                    if (m_Logger && statusCode == EDF_EDbOperationStatusCode.SUCCESS)
                    {
                        // Log significant stat changes
                        LogSignificantChanges(playerUID, playerName, oldStats, stats);
                    }
                }
                else
                {
                    // Create new entity
                    entity = STS_PlayerStatsEntity.FromPlayerStats(playerUID, playerName, stats);
                    
                    // Set creation and update timestamps
                    float currentTime = System.GetTickCount() / 1000.0;
                    entity.m_fCreationTime = currentTime;
                    entity.m_fLastUpdateTime = currentTime;
                    
                    // Insert the new entity
                    statusCode = m_Repository.AddOrUpdate(entity);
                    
                    // Log creation
                    if (m_Logger && statusCode == EDF_EDbOperationStatusCode.SUCCESS)
                    {
                        m_Logger.LogInfo(string.Format("Created new player stats record for %1 (UID: %2)", 
                            playerName, playerUID), "STS_PlayerStatsRepository", "SavePlayerStats");
                    }
                }
                
                success = statusCode == EDF_EDbOperationStatusCode.SUCCESS;
                
                if (!success)
                {
                    // Log detailed information about the failure
                    if (m_Logger)
                    {
                        m_Logger.LogError(string.Format("Database operation failed for %1 (UID: %2) - Status: %3", 
                            playerName, playerUID, statusCode), "STS_PlayerStatsRepository", "SavePlayerStats");
                    }
                    
                    // Rollback transaction on failure
                    transaction.Rollback();
                }
                else
                {
                    // Commit transaction on success
                    EDF_EDbOperationStatusCode commitStatus = transaction.Commit();
                    if (commitStatus != EDF_EDbOperationStatusCode.SUCCESS)
                    {
                        success = false;
                        if (m_Logger)
                        {
                            m_Logger.LogError(string.Format("Failed to commit transaction for %1 (UID: %2) - Status: %3", 
                                playerName, playerUID, commitStatus), "STS_PlayerStatsRepository", "SavePlayerStats");
                        }
                    }
                }
            }
            catch (Exception innerEx)
            {
                // Make sure transaction is rolled back on exception
                transaction.Rollback();
                
                // Re-throw the exception to be caught by outer catch block
                throw innerEx;
            }
            
            // Log operation duration for performance monitoring
            float duration = (System.GetTickCount() / 1000.0) - startTime;
            if (m_Logger)
            {
                if (duration > 0.5) // Log warning if operation took more than 500ms
                {
                    m_Logger.LogWarning(string.Format("SavePlayerStats for %1 (UID: %2) took %.2f seconds", 
                        playerName, playerUID, duration), "STS_PlayerStatsRepository", "SavePlayerStats");
                }
                else 
                {
                    m_Logger.LogDebug(string.Format("SavePlayerStats completed in %.2f seconds", duration), 
                        "STS_PlayerStatsRepository", "SavePlayerStats");
                }
            }
            
            return success;
        }
        catch (Exception e)
        {
            // Log detailed exception information with stack trace
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception in SavePlayerStats for %1 (UID: %2): %3", 
                    playerName, playerUID, e.ToString()), 
                    "STS_PlayerStatsRepository", "SavePlayerStats", e.GetStackTrace());
            }
            
            // Log failure duration for performance monitoring
            float duration = (System.GetTickCount() / 1000.0) - startTime;
            if (m_Logger)
            {
                m_Logger.LogWarning(string.Format("Failed SavePlayerStats operation took %.2f seconds", duration), 
                    "STS_PlayerStatsRepository", "SavePlayerStats");
            }
            
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Validate player statistics before saving
    bool ValidatePlayerStats(STS_PlayerStats stats)
    {
        if (!stats)
            return false;
        
        // Check for impossible values that would indicate corruption or exploitation
        if (stats.m_iKills < 0 || stats.m_iDeaths < 0 || stats.m_iBasesLost < 0 || 
            stats.m_iBasesCaptured < 0 || stats.m_iTotalXP < 0 || stats.m_iRank < 0 ||
            stats.m_iSuppliesDelivered < 0 || stats.m_iSupplyDeliveryCount < 0 || 
            stats.m_iAIKills < 0 || stats.m_iVehicleKills < 0 || stats.m_iAirKills < 0)
        {
            return false;
        }
        
        // Check for unreasonably high values that would indicate cheating or corruption
        const int MAX_REASONABLE_KILLS = 10000;
        const int MAX_REASONABLE_DEATHS = 10000;
        const int MAX_REASONABLE_BASECAPTURES = 5000;
        const int MAX_REASONABLE_XP = 1000000;
        
        if (stats.m_iKills > MAX_REASONABLE_KILLS || stats.m_iDeaths > MAX_REASONABLE_DEATHS || 
            stats.m_iBasesCaptured > MAX_REASONABLE_BASECAPTURES || stats.m_iTotalXP > MAX_REASONABLE_XP)
        {
            return false;
        }
        
        // Check for valid timestamps
        if (stats.m_fConnectionTime < 0 || stats.m_fLastSessionDuration < 0 || stats.m_fTotalPlaytime < 0)
        {
            return false;
        }
        
        // Check for reasonable session duration (less than 30 days)
        const float MAX_SESSION_DURATION = 30 * 24 * 60 * 60; // 30 days in seconds
        if (stats.m_fLastSessionDuration > MAX_SESSION_DURATION)
        {
            return false;
        }
        
        // Validate arrays
        if (!stats.m_aKilledBy || !stats.m_aKilledByWeapon || !stats.m_aKilledByTeam)
        {
            return false;
        }
        
        // Check array consistency
        if (stats.m_aKilledBy.Count() != stats.m_aKilledByWeapon.Count() || 
            stats.m_aKilledBy.Count() != stats.m_aKilledByTeam.Count())
        {
            return false;
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Log significant changes in player stats for auditing and debugging
    protected void LogSignificantChanges(string playerUID, string playerName, STS_PlayerStats oldStats, STS_PlayerStats newStats)
    {
        if (!m_Logger || !oldStats || !newStats)
            return;
        
        const int SIGNIFICANT_KILL_CHANGE = 10;
        const int SIGNIFICANT_SCORE_CHANGE = 500;
        
        // Calculate differences
        int killDiff = newStats.m_iKills - oldStats.m_iKills;
        int deathDiff = newStats.m_iDeaths - oldStats.m_iDeaths;
        int rankDiff = newStats.m_iRank - oldStats.m_iRank;
        int scoreDiff = newStats.CalculateTotalScore() - oldStats.CalculateTotalScore();
        
        // Log significant kill increase
        if (killDiff >= SIGNIFICANT_KILL_CHANGE)
        {
            m_Logger.LogInfo(string.Format("Player %1 (UID: %2) had significant kill increase: +%3 (now %4)", 
                playerName, playerUID, killDiff, newStats.m_iKills), 
                "STS_PlayerStatsRepository", "LogSignificantChanges");
        }
        
        // Log significant score change
        if (Math.Abs(scoreDiff) >= SIGNIFICANT_SCORE_CHANGE)
        {
            m_Logger.LogInfo(string.Format("Player %1 (UID: %2) had significant score change: %3%4 (now %5)", 
                playerName, playerUID, scoreDiff > 0 ? "+" : "", scoreDiff, newStats.CalculateTotalScore()), 
                "STS_PlayerStatsRepository", "LogSignificantChanges");
        }
        
        // Log rank changes
        if (rankDiff > 0)
        {
            m_Logger.LogInfo(string.Format("Player %1 (UID: %2) ranked up from %3 to %4", 
                playerName, playerUID, oldStats.m_iRank, newStats.m_iRank), 
                "STS_PlayerStatsRepository", "LogSignificantChanges");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load player statistics with robust error handling, retry logic, and validation
    STS_PlayerStats LoadPlayerStats(string playerUID)
    {
        // Start measuring operation time for performance monitoring
        float startTime = System.GetTickCount() / 1000.0;
        
        // Input validation
        if (!m_Repository)
        {
            if (m_Logger)
                m_Logger.LogError("Repository is null - database connection may be broken", 
                    "STS_PlayerStatsRepository", "LoadPlayerStats");
            return null;
        }
        
        if (playerUID.IsEmpty())
        {
            if (m_Logger)
                m_Logger.LogError("Cannot load player stats with empty UID", 
                    "STS_PlayerStatsRepository", "LoadPlayerStats");
            return null;
        }
        
        try
        {
            if (m_Logger)
                m_Logger.LogDebug(string.Format("Loading stats for player UID: %1", playerUID), 
                    "STS_PlayerStatsRepository", "LoadPlayerStats");
            
            // Find player in the database using retry logic
            STS_PlayerStatsEntity entity = null;
            Exception lastException = null;
            EDF_DbFindCondition condition = EDF_DbFind.Field("m_sPlayerUID").Equals(playerUID);
            
            // Retry query up to 3 times in case of temporary failures
            for (int attempt = 1; attempt <= 3; attempt++)
            {
                try
                {
                    entity = m_Repository.FindFirst(condition);
                    lastException = null; // Clear exception if query succeeds
                    break; // Success, exit retry loop
                }
                catch (Exception queryEx)
                {
                    lastException = queryEx;
                    
                    if (m_Logger)
                        m_Logger.LogWarning(string.Format("Query attempt %1 failed for UID %2: %3", 
                            attempt, playerUID, queryEx.ToString()), 
                            "STS_PlayerStatsRepository", "LoadPlayerStats");
                    
                    if (attempt < 3)
                    {
                        // Sleep before retry with increasing delay
                        int delayMs = 50 * attempt;
                        Sleep(delayMs);
                    }
                }
            }
            
            // If we have a persistent exception after retries, log and rethrow
            if (lastException)
            {
                if (m_Logger)
                    m_Logger.LogError(string.Format("All query attempts failed for UID %1: %2", 
                        playerUID, lastException.ToString()), 
                        "STS_PlayerStatsRepository", "LoadPlayerStats", lastException.GetStackTrace());
                
                throw lastException;
            }
            
            STS_PlayerStats result = null;
            
            if (entity)
            {
                try
                {
                    // Convert entity to stats
                    result = entity.ToPlayerStats();
                    
                    // Validate the loaded stats
                    if (!ValidatePlayerStats(result))
                    {
                        if (m_Logger)
                            m_Logger.LogWarning(string.Format("Loaded player stats failed validation for UID: %1 - data may be corrupted", 
                                playerUID), "STS_PlayerStatsRepository", "LoadPlayerStats");
                        
                        // Option 1: Return null to indicate error
                        // return null;
                        
                        // Option 2: Create a clean stats object instead
                        result = new STS_PlayerStats();
                        
                        // Log data repair attempt
                        if (m_Logger)
                            m_Logger.LogInfo(string.Format("Created fresh stats for UID %1 due to validation failure", 
                                playerUID), "STS_PlayerStatsRepository", "LoadPlayerStats");
                    }
                }
                catch (Exception conversionEx)
                {
                    // Handle conversion errors (could be due to schema changes or data corruption)
                    if (m_Logger)
                        m_Logger.LogError(string.Format("Failed to convert entity to stats for UID %1: %2", 
                            playerUID, conversionEx.ToString()), 
                            "STS_PlayerStatsRepository", "LoadPlayerStats", conversionEx.GetStackTrace());
                    
                    // Create a clean stats object instead
                    result = new STS_PlayerStats();
                    
                    // Log data repair attempt
                    if (m_Logger)
                        m_Logger.LogInfo(string.Format("Created fresh stats for UID %1 due to conversion error", 
                            playerUID), "STS_PlayerStatsRepository", "LoadPlayerStats");
                }
            }
            else
            {
                if (m_Logger)
                    m_Logger.LogInfo(string.Format("No player stats found for UID: %1 - will create new record when saved", 
                        playerUID), "STS_PlayerStatsRepository", "LoadPlayerStats");
                
                // Return fresh stats for new players
                result = new STS_PlayerStats();
            }
            
            // Log operation duration for performance monitoring
            float duration = (System.GetTickCount() / 1000.0) - startTime;
            if (m_Logger)
            {
                if (duration > 0.1) // Log warning if loading took more than 100ms
                {
                    m_Logger.LogWarning(string.Format("LoadPlayerStats for UID %1 took %.2f seconds", 
                        playerUID, duration), "STS_PlayerStatsRepository", "LoadPlayerStats");
                }
                else
                {
                    m_Logger.LogDebug(string.Format("LoadPlayerStats completed in %.2f seconds", duration), 
                        "STS_PlayerStatsRepository", "LoadPlayerStats");
                }
            }
            
            return result;
        }
        catch (Exception e)
        {
            // Log detailed exception information with stack trace
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception in LoadPlayerStats for UID %1: %2", 
                    playerUID, e.ToString()), 
                    "STS_PlayerStatsRepository", "LoadPlayerStats", e.GetStackTrace());
            }
            
            // Log failure duration for performance monitoring
            float duration = (System.GetTickCount() / 1000.0) - startTime;
            if (m_Logger)
            {
                m_Logger.LogWarning(string.Format("Failed LoadPlayerStats operation took %.2f seconds", duration), 
                    "STS_PlayerStatsRepository", "LoadPlayerStats");
            }
            
            return null;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load player statistics asynchronously with validation and error handling
    void LoadPlayerStatsAsync(string playerUID, func<STS_PlayerStats> callback)
    {
        // Measure start time for performance tracking
        float startTime = System.GetTickCount() / 1000.0;
        
        // Input validation
        if (!m_Repository)
        {
            if (m_Logger)
                m_Logger.LogError("Repository is null - database connection may be broken", 
                    "STS_PlayerStatsRepository", "LoadPlayerStatsAsync");
                
            if (callback)
                callback(null);
            return;
        }
        
        if (playerUID.IsEmpty())
        {
            if (m_Logger)
                m_Logger.LogError("Cannot load player stats with empty UID", 
                    "STS_PlayerStatsRepository", "LoadPlayerStatsAsync");
                
            if (callback)
                callback(null);
            return;
        }
        
        if (!callback)
        {
            if (m_Logger)
                m_Logger.LogWarning(string.Format("LoadPlayerStatsAsync called with null callback for UID: %1", 
                    playerUID), "STS_PlayerStatsRepository", "LoadPlayerStatsAsync");
            return;
        }
        
        try
        {
            if (m_Logger)
                m_Logger.LogDebug(string.Format("Starting async load of stats for player UID: %1", playerUID), 
                    "STS_PlayerStatsRepository", "LoadPlayerStatsAsync");
            
            // Find player in the database
            EDF_DbFindCondition condition = EDF_DbFind.Field("m_sPlayerUID").Equals(playerUID);
            
            // Create a callback handler with error handling and validation
            STS_LoadPlayerStatsCallback callbackHandler = new STS_LoadPlayerStatsCallback(callback, playerUID, m_Logger, startTime);
            
            // Execute the async find
            m_Repository.FindFirstAsync(condition, callbackHandler);
        }
        catch (Exception e)
        {
            // Log error with stack trace
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception initiating async load for UID %1: %2", 
                    playerUID, e.ToString()), 
                    "STS_PlayerStatsRepository", "LoadPlayerStatsAsync", e.GetStackTrace());
            }
            
            // Ensure callback is called even on error
            if (callback)
                callback(null);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all player statistics for scoreboard
    array<ref STS_PlayerStats> GetAllPlayerStats()
    {
        array<ref STS_PlayerStats> result = new array<ref STS_PlayerStats>();
        
        if (!m_Repository)
            return result;
        
        try
        {
            // Find all players
            array<ref STS_PlayerStatsEntity> entities = m_Repository.FindAll();
            
            // Convert entities to stats
            foreach (STS_PlayerStatsEntity entity : entities)
            {
                STS_PlayerStats stats = entity.ToPlayerStats();
                result.Insert(stats);
            }
            
            return result;
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in GetAllPlayerStats: %1", e.ToString()), 
                "STS_PlayerStatsRepository", "GetAllPlayerStats");
            return result;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get top players by score (for leaderboards)
    array<ref STS_PlayerStats> GetTopPlayers(int limit = 10)
    {
        array<ref STS_PlayerStats> result = new array<ref STS_PlayerStats>();
        
        if (!m_Repository)
            return result;
        
        try
        {
            // Create sorting configuration
            EDF_DbFind.SortConfiguration sortConfig();
            sortConfig.AddField("m_iTotalXP", false); // Sort by XP descending
            
            // Find top players
            array<ref STS_PlayerStatsEntity> entities = m_Repository.FindAll(null, sortConfig, limit);
            
            // Convert entities to stats
            foreach (STS_PlayerStatsEntity entity : entities)
            {
                STS_PlayerStats stats = entity.ToPlayerStats();
                result.Insert(stats);
            }
            
            return result;
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in GetTopPlayers: %1", e.ToString()), 
                "STS_PlayerStatsRepository", "GetTopPlayers");
            return result;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Delete player statistics with comprehensive error handling, transaction support, and retry logic
    bool DeletePlayerStats(string playerUID)
    {
        // Start measuring operation time for performance monitoring
        float startTime = System.GetTickCount() / 1000.0;
        
        // Input validation
        if (!m_Repository)
        {
            if (m_Logger)
                m_Logger.LogError("Repository is null - database connection may be broken", 
                    "STS_PlayerStatsRepository", "DeletePlayerStats");
            return false;
        }
        
        if (playerUID.IsEmpty())
        {
            if (m_Logger)
                m_Logger.LogError("Cannot delete player stats with empty UID", 
                    "STS_PlayerStatsRepository", "DeletePlayerStats");
            return false;
        }
        
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Attempting to delete player stats for UID: %1", playerUID), 
                "STS_PlayerStatsRepository", "DeletePlayerStats");
        
        try
        {
            // Find player in the database with retry logic
            STS_PlayerStatsEntity entity = null;
            Exception lastException = null;
            EDF_DbFindCondition condition = EDF_DbFind.Field("m_sPlayerUID").Equals(playerUID);
            
            // Retry query up to 3 times in case of temporary failures
            for (int attempt = 1; attempt <= 3; attempt++)
            {
                try
                {
                    entity = m_Repository.FindFirst(condition);
                    lastException = null; // Clear exception if query succeeds
                    break; // Success, exit retry loop
                }
                catch (Exception queryEx)
                {
                    lastException = queryEx;
                    
                    if (m_Logger)
                        m_Logger.LogWarning(string.Format("Query attempt %1 failed for UID %2: %3", 
                            attempt, playerUID, queryEx.ToString()), 
                            "STS_PlayerStatsRepository", "DeletePlayerStats");
                    
                    if (attempt < 3)
                    {
                        // Sleep before retry with increasing delay
                        int delayMs = 50 * attempt;
                        Sleep(delayMs);
                    }
                }
            }
            
            // If we have a persistent exception after retries, log and rethrow
            if (lastException)
            {
                if (m_Logger)
                    m_Logger.LogError(string.Format("All query attempts failed for UID %1: %2", 
                        playerUID, lastException.ToString()), 
                        "STS_PlayerStatsRepository", "DeletePlayerStats", lastException.GetStackTrace());
                
                throw lastException;
            }
            
            if (entity)
            {
                // Start a transaction for data integrity
                EDF_DbTransaction transaction = m_DbContext.BeginTransaction();
                if (!transaction)
                {
                    if (m_Logger)
                        m_Logger.LogError(string.Format("Failed to create transaction for deleting UID: %1", 
                            playerUID), "STS_PlayerStatsRepository", "DeletePlayerStats");
                    return false;
                }
                
                bool success = false;
                
                try
                {
                    // Create backup copy of stats before deletion for potential recovery or audit
                    STS_PlayerStats backupStats = entity.ToPlayerStats();
                    
                    if (m_Logger)
                        m_Logger.LogDebug(string.Format("Found player stats to delete - UID: %1, Name: %2", 
                            playerUID, entity.m_sPlayerName), "STS_PlayerStatsRepository", "DeletePlayerStats");
                    
                    // Delete with retry logic
                    EDF_EDbOperationStatusCode statusCode = EDF_EDbOperationStatusCode.ERROR;
                    
                    for (int attempt = 1; attempt <= 3; attempt++)
                    {
                        try
                        {
                            statusCode = m_Repository.Remove(entity);
                            break; // Success or definitive failure, exit retry loop
                        }
                        catch (Exception deleteEx)
                        {
                            if (attempt == 3)
                            {
                                // Rethrow on final attempt
                                throw deleteEx;
                            }
                            
                            if (m_Logger)
                                m_Logger.LogWarning(string.Format("Delete attempt %1 failed for UID %2: %3", 
                                    attempt, playerUID, deleteEx.ToString()), 
                                    "STS_PlayerStatsRepository", "DeletePlayerStats");
                            
                            // Sleep before retry with increasing delay
                            int delayMs = 50 * attempt;
                            Sleep(delayMs);
                        }
                    }
                    
                    success = statusCode == EDF_EDbOperationStatusCode.SUCCESS;
                    
                    if (!success)
                    {
                        // Log detailed information about the failure
                        if (m_Logger)
                        {
                            m_Logger.LogError(string.Format("Database delete operation failed for UID: %1 - Status: %2", 
                                playerUID, statusCode), "STS_PlayerStatsRepository", "DeletePlayerStats");
                        }
                        
                        // Rollback transaction on failure
                        transaction.Rollback();
                    }
                    else
                    {
                        // Commit transaction on success
                        EDF_EDbOperationStatusCode commitStatus = transaction.Commit();
                        if (commitStatus != EDF_EDbOperationStatusCode.SUCCESS)
                        {
                            success = false;
                            if (m_Logger)
                            {
                                m_Logger.LogError(string.Format("Failed to commit delete transaction for UID: %1 - Status: %2", 
                                    playerUID, commitStatus), "STS_PlayerStatsRepository", "DeletePlayerStats");
                            }
                        }
                        else
                        {
                            // Log successful deletion with some details for auditing
                            if (m_Logger)
                            {
                                m_Logger.LogInfo(string.Format("Successfully deleted player stats for '%1' (UID: %2) - Kills: %3, XP: %4", 
                                    entity.m_sPlayerName, playerUID, backupStats.m_iKills, backupStats.m_iTotalXP),
                                    "STS_PlayerStatsRepository", "DeletePlayerStats");
                            }
                        }
                    }
                }
                catch (Exception innerEx)
                {
                    // Make sure transaction is rolled back on exception
                    transaction.Rollback();
                    
                    // Re-throw the exception to be caught by outer catch block
                    throw innerEx;
                }
                
                // Log operation duration for performance monitoring
                float duration = (System.GetTickCount() / 1000.0) - startTime;
                if (m_Logger)
                {
                    if (duration > 0.5) // Log warning if operation took more than 500ms
                    {
                        m_Logger.LogWarning(string.Format("DeletePlayerStats for UID %1 took %.2f seconds", 
                            playerUID, duration), "STS_PlayerStatsRepository", "DeletePlayerStats");
                    }
                    else 
                    {
                        m_Logger.LogDebug(string.Format("DeletePlayerStats completed in %.2f seconds", duration), 
                            "STS_PlayerStatsRepository", "DeletePlayerStats");
                    }
                }
                
                return success;
            }
            else
            {
                // No stats found for this player
                if (m_Logger)
                    m_Logger.LogWarning(string.Format("No player stats found for deletion - UID: %1", playerUID), 
                        "STS_PlayerStatsRepository", "DeletePlayerStats");
                
                // Log operation duration for performance monitoring
                float duration = (System.GetTickCount() / 1000.0) - startTime;
                if (m_Logger)
                {
                    m_Logger.LogDebug(string.Format("DeletePlayerStats (not found) completed in %.2f seconds", duration), 
                        "STS_PlayerStatsRepository", "DeletePlayerStats");
                }
                
                // Could return true since the end state is as requested (stats don't exist),
                // but returning false to indicate that no action was taken
                return false;
            }
        }
        catch (Exception e)
        {
            // Log detailed exception information with stack trace
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception in DeletePlayerStats for UID %1: %2", 
                    playerUID, e.ToString()), 
                    "STS_PlayerStatsRepository", "DeletePlayerStats", e.GetStackTrace());
            }
            
            // Log failure duration for performance monitoring
            float duration = (System.GetTickCount() / 1000.0) - startTime;
            if (m_Logger)
            {
                m_Logger.LogWarning(string.Format("Failed DeletePlayerStats operation took %.2f seconds", duration), 
                    "STS_PlayerStatsRepository", "DeletePlayerStats");
            }
            
            return false;
        }
    }
}

//------------------------------------------------------------------------------------------------
// Enhanced callback handler for async player stats loading with error handling
class STS_LoadPlayerStatsCallback : EDF_DbFindCallbackSingle<STS_PlayerStatsEntity>
{
    protected func<STS_PlayerStats> m_Callback;
    protected string m_sPlayerUID;
    protected STS_LoggingSystem m_Logger;
    protected float m_fStartTime;
    
    //------------------------------------------------------------------------------------------------
    void STS_LoadPlayerStatsCallback(func<STS_PlayerStats> callback, string playerUID, STS_LoggingSystem logger, float startTime)
    {
        m_Callback = callback;
        m_sPlayerUID = playerUID;
        m_Logger = logger;
        m_fStartTime = startTime;
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnSuccess(STS_PlayerStatsEntity result)
    {
        try
        {
            STS_PlayerStats stats = null;
            
            if (result)
            {
                // Convert entity to stats
                stats = result.ToPlayerStats();
                
                // Validate stats data
                if (!ValidatePlayerStats(stats))
                {
                    if (m_Logger)
                        m_Logger.LogWarning(string.Format("Loaded player stats failed validation for UID: %1 - creating fresh stats", 
                            m_sPlayerUID), "STS_LoadPlayerStatsCallback", "OnSuccess");
                    
                    // Create fresh stats on validation failure
                    stats = new STS_PlayerStats();
                }
                
                if (m_Logger)
                    m_Logger.LogDebug(string.Format("Successfully loaded stats for UID: %1", m_sPlayerUID), 
                        "STS_LoadPlayerStatsCallback", "OnSuccess");
            }
            else
            {
                if (m_Logger)
                    m_Logger.LogInfo(string.Format("No player stats found for UID: %1", m_sPlayerUID), 
                        "STS_LoadPlayerStatsCallback", "OnSuccess");
                
                // Return empty stats for new players
                stats = new STS_PlayerStats();
            }
            
            // Log operation duration for performance monitoring
            float duration = (System.GetTickCount() / 1000.0) - m_fStartTime;
            if (m_Logger)
            {
                if (duration > 0.1) // Log warning if loading took more than 100ms
                {
                    m_Logger.LogWarning(string.Format("Async load for UID %1 took %.2f seconds", 
                        m_sPlayerUID, duration), "STS_LoadPlayerStatsCallback", "OnSuccess");
                }
                else
                {
                    m_Logger.LogDebug(string.Format("Async load completed in %.2f seconds", duration), 
                        "STS_LoadPlayerStatsCallback", "OnSuccess");
                }
            }
            
            // Invoke user callback with result
            if (m_Callback)
                m_Callback(stats);
        }
        catch (Exception e)
        {
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception in async callback for UID %1: %2", 
                    m_sPlayerUID, e.ToString()), 
                    "STS_LoadPlayerStatsCallback", "OnSuccess", e.GetStackTrace());
            }
            
            // Ensure callback is called even on error
            if (m_Callback)
                m_Callback(new STS_PlayerStats());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnFailure(EDF_EDbOperationStatusCode statusCode)
    {
        if (m_Logger)
        {
            m_Logger.LogError(string.Format("Async load operation failed for UID %1 with status code: %2", 
                m_sPlayerUID, statusCode), "STS_LoadPlayerStatsCallback", "OnFailure");
        }
        
        // Log operation duration for performance monitoring
        float duration = (System.GetTickCount() / 1000.0) - m_fStartTime;
        if (m_Logger)
        {
            m_Logger.LogWarning(string.Format("Failed async load operation took %.2f seconds", duration), 
                "STS_LoadPlayerStatsCallback", "OnFailure");
        }
        
        // Always return an empty stats object on failure
        if (m_Callback)
            m_Callback(new STS_PlayerStats());
    }
    
    //------------------------------------------------------------------------------------------------
    // Validate player stats - same as main repository validation
    protected bool ValidatePlayerStats(STS_PlayerStats stats)
    {
        if (!stats)
            return false;
        
        // Check for impossible values that would indicate corruption or exploitation
        if (stats.m_iKills < 0 || stats.m_iDeaths < 0 || stats.m_iBasesLost < 0 || 
            stats.m_iBasesCaptured < 0 || stats.m_iTotalXP < 0 || stats.m_iRank < 0 ||
            stats.m_iSuppliesDelivered < 0 || stats.m_iSupplyDeliveryCount < 0 || 
            stats.m_iAIKills < 0 || stats.m_iVehicleKills < 0 || stats.m_iAirKills < 0)
        {
            return false;
        }
        
        // Check for unreasonably high values that would indicate cheating or corruption
        const int MAX_REASONABLE_KILLS = 10000;
        const int MAX_REASONABLE_DEATHS = 10000;
        const int MAX_REASONABLE_BASECAPTURES = 5000;
        const int MAX_REASONABLE_XP = 1000000;
        
        if (stats.m_iKills > MAX_REASONABLE_KILLS || stats.m_iDeaths > MAX_REASONABLE_DEATHS || 
            stats.m_iBasesCaptured > MAX_REASONABLE_BASECAPTURES || stats.m_iTotalXP > MAX_REASONABLE_XP)
        {
            return false;
        }
        
        return true;
    }
} 