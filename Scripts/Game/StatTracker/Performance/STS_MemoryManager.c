// STS_MemoryManager.c
// Handles memory management and optimization for the StatTracker system

class STS_MemoryManager
{
    // Singleton instance
    private static ref STS_MemoryManager s_Instance;
    
    // Configuration
    protected int m_iCleanupInterval = 600;          // How often to clean up memory (in seconds)
    protected int m_iMaxCachedPlayers = 500;         // Maximum number of players to keep in memory
    protected int m_iMaxArraySize = 1000;            // Maximum size of arrays before trimming
    protected int m_iMaxHistoryRecords = 1000;       // Maximum number of history records to keep
    protected float m_fLastCleanupTime = 0;          // Last time cleanup was performed
    
    // Runtime statistics
    protected int m_iTotalCleanups = 0;              // Total number of cleanups performed
    protected int m_iTotalItemsRemoved = 0;          // Total number of items removed from memory
    protected float m_fMemoryUsageEstimate = 0;      // Estimated memory usage in MB
    protected float m_fPeakMemoryUsage = 0;          // Peak memory usage in MB
    
    // Logger
    protected STS_LoggingSystem m_Logger;
    
    // Reference to other systems
    protected STS_Config m_Config;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_MemoryManager()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Config = STS_Config.GetInstance();
        
        // Set up scheduled cleanup
        GetGame().GetCallqueue().CallLater(PerformScheduledCleanup, m_iCleanupInterval * 1000, true);
        
        if (m_Logger)
            m_Logger.LogInfo("Memory Manager initialized", "STS_MemoryManager", "Constructor");
        else
            Print("[StatTracker] Memory Manager initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_MemoryManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_MemoryManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Perform a scheduled cleanup
    void PerformScheduledCleanup()
    {
        if (!GetGame().IsMissionHost())
            return;
            
        m_fLastCleanupTime = GetGame().GetTime();
        
        // Log cleanup start
        if (m_Logger)
            m_Logger.LogInfo("Starting scheduled memory cleanup", "STS_MemoryManager", "PerformScheduledCleanup");
        else
            Print("[StatTracker] Starting scheduled memory cleanup");
            
        // Estimate memory usage before cleanup
        float beforeCleanup = EstimateMemoryUsage();
        int itemsRemoved = 0;
        
        // Clean up player cache
        itemsRemoved += CleanupPlayerCache();
        
        // Clean up team kill records
        itemsRemoved += CleanupTeamKillRecords();
        
        // Clean up kill history
        itemsRemoved += CleanupKillHistory();
        
        // Clean up other arrays
        itemsRemoved += CleanupArrays();
        
        // Update stats
        m_iTotalCleanups++;
        m_iTotalItemsRemoved += itemsRemoved;
        
        // Estimate memory usage after cleanup
        float afterCleanup = EstimateMemoryUsage();
        float savedMemory = beforeCleanup - afterCleanup;
        
        // Log results
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Memory cleanup completed: Removed %1 items, freed ~%.2f MB", itemsRemoved, savedMemory), "STS_MemoryManager", "PerformScheduledCleanup");
        else
            Print(string.Format("[StatTracker] Memory cleanup completed: Removed %1 items, freed ~%.2f MB", itemsRemoved, savedMemory));
            
        // Trigger garbage collection
        TriggerGarbageCollection();
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up player cache
    protected int CleanupPlayerCache()
    {
        // Get persistence manager
        STS_PersistenceManager persistenceManager = STS_PersistenceManager.GetInstance();
        if (!persistenceManager)
            return 0;
            
        // Get cached player count
        int cachedPlayers = persistenceManager.GetCachedPlayerCount();
        if (cachedPlayers <= m_iMaxCachedPlayers)
            return 0;
            
        // Calculate how many to remove
        int toRemove = cachedPlayers - m_iMaxCachedPlayers;
        
        // Remove oldest accessed players
        int removed = persistenceManager.RemoveOldestCachedPlayers(toRemove);
        
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Cleaned up player cache: Removed %1 of %2 players", removed, cachedPlayers), "STS_MemoryManager", "CleanupPlayerCache");
            
        return removed;
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up team kill records
    protected int CleanupTeamKillRecords()
    {
        // Get team kill tracker
        STS_TeamKillTracker teamKillTracker = STS_TeamKillTracker.GetInstance();
        if (!teamKillTracker)
            return 0;
            
        // Calculate old timestamp (records older than 30 days)
        int currentTime = GetCurrentTimestamp();
        int oldTimestamp = currentTime - (30 * 24 * 60 * 60); // 30 days in seconds
        
        // Remove old records
        int removed = teamKillTracker.CleanupOldRecords(oldTimestamp);
        
        if (m_Logger && removed > 0)
            m_Logger.LogInfo(string.Format("Cleaned up team kill records: Removed %1 records older than 30 days", removed), "STS_MemoryManager", "CleanupTeamKillRecords");
            
        return removed;
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up kill history
    protected int CleanupKillHistory()
    {
        // Get stat tracking manager
        STS_StatTrackingManagerComponent statManager = GetStatTrackingManager();
        if (!statManager)
            return 0;
            
        // Get kill history
        array<ref STS_KillRecord> killHistory = statManager.GetKillHistory();
        if (!killHistory || killHistory.Count() <= m_iMaxHistoryRecords)
            return 0;
            
        // Calculate how many to remove
        int toRemove = killHistory.Count() - m_iMaxHistoryRecords;
        
        // Remove oldest records
        for (int i = 0; i < toRemove; i++)
        {
            killHistory.RemoveOrdered(0); // Remove oldest (at index 0)
        }
        
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Cleaned up kill history: Removed %1 oldest records", toRemove), "STS_MemoryManager", "CleanupKillHistory");
            
        return toRemove;
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up arrays that may grow too large
    protected int CleanupArrays()
    {
        int totalRemoved = 0;
        
        // Get stat tracking manager
        STS_StatTrackingManagerComponent statManager = GetStatTrackingManager();
        if (!statManager)
            return 0;
            
        // Clean up player arrays that might grow too large
        // This is just an example - actual implementation would depend on your data structures
        /*
        array<ref YourDataType> someArray = statManager.GetSomeArray();
        if (someArray && someArray.Count() > m_iMaxArraySize)
        {
            int toRemove = someArray.Count() - m_iMaxArraySize;
            for (int i = 0; i < toRemove; i++)
            {
                someArray.RemoveOrdered(0);
            }
            totalRemoved += toRemove;
        }
        */
        
        return totalRemoved;
    }
    
    //------------------------------------------------------------------------------------------------
    // Estimate memory usage (very rough estimation)
    float EstimateMemoryUsage()
    {
        float totalMB = 0;
        
        // This is a very rough estimation
        // In a real implementation, you would need more detailed accounting
        
        // Estimate player stats cache
        STS_PersistenceManager persistenceManager = STS_PersistenceManager.GetInstance();
        if (persistenceManager)
        {
            int cachedPlayers = persistenceManager.GetCachedPlayerCount();
            // Assume average player data is around 10KB
            totalMB += (cachedPlayers * 0.01);
        }
        
        // Estimate team kill records
        STS_TeamKillTracker teamKillTracker = STS_TeamKillTracker.GetInstance();
        if (teamKillTracker)
        {
            int recordCount = teamKillTracker.GetTotalRecordCount();
            // Assume each record is around 500 bytes
            totalMB += (recordCount * 0.0005);
        }
        
        // Estimate kill history
        STS_StatTrackingManagerComponent statManager = GetStatTrackingManager();
        if (statManager)
        {
            array<ref STS_KillRecord> killHistory = statManager.GetKillHistory();
            if (killHistory)
            {
                // Assume each kill record is around 200 bytes
                totalMB += (killHistory.Count() * 0.0002);
            }
        }
        
        // Store current estimate
        m_fMemoryUsageEstimate = totalMB;
        
        // Update peak if needed
        if (totalMB > m_fPeakMemoryUsage)
            m_fPeakMemoryUsage = totalMB;
            
        return totalMB;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get memory usage statistics as a string
    string GetMemoryStats()
    {
        string stats = "Memory Statistics:\n";
        stats += string.Format("- Estimated Usage: %.2f MB\n", m_fMemoryUsageEstimate);
        stats += string.Format("- Peak Usage: %.2f MB\n", m_fPeakMemoryUsage);
        stats += string.Format("- Cleanups Performed: %1\n", m_iTotalCleanups);
        stats += string.Format("- Total Items Removed: %1\n", m_iTotalItemsRemoved);
        stats += string.Format("- Last Cleanup: %1\n", FormatTimeSince(m_fLastCleanupTime));
        
        return stats;
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset peak memory usage
    void ResetPeakMemoryUsage()
    {
        m_fPeakMemoryUsage = m_fMemoryUsageEstimate;
        
        if (m_Logger)
            m_Logger.LogInfo("Reset peak memory usage", "STS_MemoryManager", "ResetPeakMemoryUsage");
    }
    
    //------------------------------------------------------------------------------------------------
    // Trigger garbage collection
    void TriggerGarbageCollection()
    {
        // Unfortunately, Enfusion doesn't have an explicit way to trigger garbage collection
        // This function is mainly a placeholder for future implementation if such API becomes available
        
        // For now, we can use a trick to encourage garbage collection
        for (int i = 0; i < 5; i++)
        {
            // Create and immediately discard large temporary objects
            array<int> temp = new array<int>();
            for (int j = 0; j < 10000; j++)
            {
                temp.Insert(j);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the stat tracking manager component
    protected STS_StatTrackingManagerComponent GetStatTrackingManager()
    {
        // Try to get from game mode entity
        IEntity gameModeEntity = GetGame().GetGameMode();
        if (gameModeEntity)
        {
            STS_StatTrackingManagerComponent component = STS_StatTrackingManagerComponent.Cast(gameModeEntity.FindComponent(STS_StatTrackingManagerComponent));
            if (component)
                return component;
        }
        
        // Try to get from world
        BaseWorld world = GetGame().GetWorld();
        if (world)
        {
            IEntity worldEntity = world.GetWorldEntity();
            if (worldEntity)
            {
                STS_StatTrackingManagerComponent component = STS_StatTrackingManagerComponent.Cast(worldEntity.FindComponent(STS_StatTrackingManagerComponent));
                if (component)
                    return component;
            }
        }
        
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Get current Unix timestamp
    protected int GetCurrentTimestamp()
    {
        return GetGame().GetWorldTime().GetTimestamp();
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Format time since a given game time
    protected string FormatTimeSince(float gameTime)
    {
        if (gameTime == 0)
            return "Never";
            
        float currentTime = GetGame().GetTime();
        float elapsedSeconds = (currentTime - gameTime) / 1000;
        
        if (elapsedSeconds < 60)
            return string.Format("%.0f seconds ago", elapsedSeconds);
        else if (elapsedSeconds < 3600)
            return string.Format("%.0f minutes ago", elapsedSeconds / 60);
        else if (elapsedSeconds < 86400)
            return string.Format("%.1f hours ago", elapsedSeconds / 3600);
        else
            return string.Format("%.1f days ago", elapsedSeconds / 86400);
    }
} 