// STS_Main.c
// Main entry point for the StatTracker system

// Include integration version of webhook manager
#include "$CurrentDir:Integration/STS_WebhookManager.c"
// Include backup and advanced backup systems
#include "$CurrentDir:STS_BackupManager.c"
#include "$CurrentDir:Integration/STS_AdvancedBackupSystem.c"

class STS_Main
{
    // Singleton instance
    private static ref STS_Main s_Instance;
    
    // Component references
    protected ref STS_Config m_Config;
    protected ref STS_PersistenceManager m_PersistenceManager;
    protected ref STS_RCONCommands m_RCONCommands;
    protected ref STS_AdminDashboard m_AdminDashboard;
    protected ref STS_NotificationManager m_NotificationManager;
    protected ref STS_AdminMenu m_AdminMenu;
    protected ref STS_WebhookManager m_WebhookManager;
    protected ref STS_APIServer m_APIServer;
    protected ref STS_HeatmapManager m_HeatmapManager;
    protected ref STS_DataExportManager m_DataExportManager;
    protected ref STS_DataCompression m_DataCompression;
    protected ref STS_TeamKillTracker m_TeamKillTracker;
    protected ref STS_StatTrackingManagerComponent m_StatTrackingManager;
    // Add backup system references
    protected ref STS_BackupManager m_BackupManager;
    protected ref STS_AdvancedBackupSystem m_AdvancedBackupSystem;
    
    // New systems
    protected ref STS_LoggingSystem m_LoggingSystem;
    protected ref STS_VoteKickSystem m_VoteKickSystem;
    protected ref STS_ChatLogger m_ChatLogger;
    
    // Initialization state
    protected bool m_bInitialized = false;
    
    // Callqueue reference for safety
    protected ref ScriptCallQueue m_CallQueue;
    
    // Error recovery
    protected bool m_bInRecoveryMode = false;
    protected int m_iErrorCount = 0;
    protected const int ERROR_THRESHOLD = 10;
    protected float m_fLastErrorTime = 0;
    protected float m_fErrorCooldownPeriod = 60.0; // 1 minute cooldown between errors
    protected ref array<string> m_aDisabledComponents = new array<string>();
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_Main()
    {
        Print("[StatTracker] Initializing StatTracker System");
        
        try
        {
            // Initialize logging system first
            m_LoggingSystem = STS_LoggingSystem.GetInstance();
            if (!m_LoggingSystem)
            {
                Print("[StatTracker] CRITICAL ERROR: Failed to initialize logging system", LogLevel.ERROR);
                return;
            }
            
            m_LoggingSystem.LogInfo("StatTracker initialization started");
            
            // Store callqueue reference for safety
            m_CallQueue = GetGame().GetCallqueue();
            if (!m_CallQueue)
            {
                m_LoggingSystem.LogError("Failed to get callqueue - delayed operations will not function");
            }
            
            // Initialize components in order
            m_Config = STS_Config.GetInstance();
            if (!m_Config)
            {
                m_LoggingSystem.LogError("Failed to initialize configuration - using defaults");
                // Create default config to prevent null reference
                m_Config = new STS_Config();
            }
            
            // Only continue if the system is enabled
            if (!m_Config.m_bEnabled)
            {
                m_LoggingSystem.LogWarning("System is disabled in configuration - initialization aborted");
                return;
            }
            
            // Initialize components with error handling
            InitializeComponents();
            
            // Start processing webhooks if component was successfully initialized
            if (m_WebhookManager && m_CallQueue && m_Config.m_bEnableWebhooks)
            {
                m_CallQueue.CallLater(ProcessWebhooks, 5000, true);
                m_LoggingSystem.LogInfo("Webhook processing scheduled");
            }
            
            // Set up error monitoring
            if (m_CallQueue)
            {
                m_CallQueue.CallLater(MonitorHealth, 30000, true);
                m_LoggingSystem.LogInfo("System health monitoring scheduled");
            }
            
            m_bInitialized = true;
            m_LoggingSystem.LogInfo("StatTracker System initialized successfully");
        }
        catch (Exception e)
        {
            if (m_LoggingSystem)
                m_LoggingSystem.LogCritical("Exception during STS_Main constructor: " + e.ToString() + "\nStacktrace: " + e.GetStackTrace());
            else
                Print("[StatTracker] CRITICAL ERROR during initialization: " + e.ToString(), LogLevel.ERROR);
                
            // Even with an error, we'll attempt to continue with limited functionality
            m_bInRecoveryMode = true;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize all components with error handling
    protected void InitializeComponents()
    {
        m_LoggingSystem.LogDebug("Initializing core components...");
        
        try {
            m_DataCompression = STS_DataCompression.GetInstance();
            if (!m_DataCompression)
                m_LoggingSystem.LogWarning("Failed to initialize data compression - data compression will not be available");
                
            m_PersistenceManager = STS_PersistenceManager.GetInstance();
            if (!m_PersistenceManager)
                m_LoggingSystem.LogError("Failed to initialize persistence manager - data will not be saved or loaded");
                
            m_WebhookManager = STS_WebhookManager.GetInstance();
            if (!m_WebhookManager && m_Config.m_bEnableWebhooks)
                m_LoggingSystem.LogWarning("Failed to initialize webhook manager - webhooks will not be sent");

            // Initialize backup systems
            m_BackupManager = STS_BackupManager.GetInstance();
            if (!m_BackupManager)
                m_LoggingSystem.LogWarning("Failed to initialize backup manager - automatic backups will not be available");
                
            m_AdvancedBackupSystem = STS_AdvancedBackupSystem.GetInstance();
            if (!m_AdvancedBackupSystem)
                m_LoggingSystem.LogWarning("Failed to initialize advanced backup system - multi-server backup coordination will not be available");
        }
        catch (Exception e) {
            m_LoggingSystem.LogError("Exception initializing core components: " + e.ToString());
            RecordComponentFailure("Core");
        }
        
        m_LoggingSystem.LogDebug("Initializing tracking systems...");
        
        try {
            m_TeamKillTracker = STS_TeamKillTracker.GetInstance();
            if (!m_TeamKillTracker)
                m_LoggingSystem.LogWarning("Failed to initialize team kill tracker - team kill tracking will not be available");
                
            m_VoteKickSystem = STS_VoteKickSystem.GetInstance();
            if (!m_VoteKickSystem)
                m_LoggingSystem.LogWarning("Failed to initialize vote kick system - vote kick tracking will not be available");
                
            m_ChatLogger = STS_ChatLogger.GetInstance();
            if (!m_ChatLogger)
                m_LoggingSystem.LogWarning("Failed to initialize chat logger - chat logging will not be available");
                
            m_StatTrackingManager = STS_StatTrackingManagerComponent.GetInstance();
            if (!m_StatTrackingManager)
                m_LoggingSystem.LogError("Failed to initialize stat tracking manager - player stats will not be tracked");
        }
        catch (Exception e) {
            m_LoggingSystem.LogError("Exception initializing tracking systems: " + e.ToString());
            RecordComponentFailure("Tracking");
        }
        
        m_LoggingSystem.LogDebug("Initializing admin/RCON components...");
        
        try {
            m_RCONCommands = STS_RCONCommands.GetInstance();
            if (!m_RCONCommands)
                m_LoggingSystem.LogWarning("Failed to initialize RCON commands - RCON functionality will not be available");
                
            m_NotificationManager = STS_NotificationManager.GetInstance();
            if (!m_NotificationManager)
                m_LoggingSystem.LogWarning("Failed to initialize notification manager - notifications will not be displayed");
                
            m_AdminMenu = STS_AdminMenu.GetInstance();
            if (!m_AdminMenu)
                m_LoggingSystem.LogWarning("Failed to initialize admin menu - admin UI will not be available");
        }
        catch (Exception e) {
            m_LoggingSystem.LogError("Exception initializing admin components: " + e.ToString());
            RecordComponentFailure("Admin");
        }
        
        m_LoggingSystem.LogDebug("Initializing web/API components...");
        
        // Initialize web/API components if enabled
        if (m_Config.m_bEnableStatsAPI)
        {
            try {
                m_APIServer = STS_APIServer.GetInstance();
                if (!m_APIServer)
                    m_LoggingSystem.LogWarning("Failed to initialize API server - external API will not be available");
                    
                m_AdminDashboard = STS_AdminDashboard.GetInstance();
                if (!m_AdminDashboard)
                    m_LoggingSystem.LogWarning("Failed to initialize admin dashboard - web dashboard will not be available");
            }
            catch (Exception e) {
                m_LoggingSystem.LogError("Exception initializing web/API components: " + e.ToString());
                RecordComponentFailure("WebAPI");
            }
        }
        
        // Initialize heatmap system if enabled
        if (m_Config.m_bEnableHeatmaps)
        {
            try {
                m_HeatmapManager = STS_HeatmapManager.GetInstance();
                if (!m_HeatmapManager)
                    m_LoggingSystem.LogWarning("Failed to initialize heatmap manager - heatmaps will not be available");
            }
            catch (Exception e) {
                m_LoggingSystem.LogError("Exception initializing heatmap components: " + e.ToString());
                RecordComponentFailure("Heatmap");
            }
        }
        
        // Initialize data export system if enabled
        if (m_Config.m_bEnableJsonExport || m_Config.m_bEnableImageExport)
        {
            try {
                m_DataExportManager = STS_DataExportManager.GetInstance();
                if (!m_DataExportManager)
                    m_LoggingSystem.LogWarning("Failed to initialize data export manager - data export will not be available");
            }
            catch (Exception e) {
                m_LoggingSystem.LogError("Exception initializing data export components: " + e.ToString());
                RecordComponentFailure("DataExport");
            }
        }
        
        // Log all disabled components
        if (m_aDisabledComponents.Count() > 0)
        {
            string disabledList = "";
            foreach (string component : m_aDisabledComponents)
            {
                disabledList += component + ", ";
            }
            if (disabledList.Length() > 2)
                disabledList = disabledList.Substring(0, disabledList.Length() - 2);
                
            m_LoggingSystem.LogWarning("The following components are disabled due to initialization errors: " + disabledList);
        }
        
        m_LoggingSystem.LogInfo("Component initialization complete");
    }
    
    //------------------------------------------------------------------------------------------------
    // Record a component initialization failure
    protected void RecordComponentFailure(string componentName)
    {
        if (!m_aDisabledComponents.Contains(componentName))
        {
            m_aDisabledComponents.Insert(componentName);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Process webhooks periodically
    protected void ProcessWebhooks()
    {
        try
        {
            if (!m_WebhookManager || !m_Config || !m_Config.m_bEnableWebhooks)
                return;
                
            m_WebhookManager.ProcessQueue();
        }
        catch (Exception e)
        {
            if (m_LoggingSystem)
            {
                m_LoggingSystem.LogError("Exception in ProcessWebhooks: " + e.ToString());
                RecordError("ProcessWebhooks", e.ToString());
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Health monitoring
    protected void MonitorHealth()
    {
        try
        {
            // Skip if not initialized
            if (!m_bInitialized || !m_LoggingSystem)
                return;
                
            m_LoggingSystem.LogDebug("Performing system health check");
            
            // Check if we have too many errors
            if (m_iErrorCount > ERROR_THRESHOLD)
            {
                m_LoggingSystem.LogWarning("Error threshold exceeded. Putting system into recovery mode.");
                EnterRecoveryMode();
            }
            
            // Check if components are responsive
            if (m_PersistenceManager && !m_PersistenceManager.IsHealthy())
            {
                m_LoggingSystem.LogWarning("Persistence manager is unhealthy. Attempting recovery.");
                AttemptComponentRecovery("PersistenceManager");
            }
            
            if (m_StatTrackingManager && !m_StatTrackingManager.IsResponding())
            {
                m_LoggingSystem.LogWarning("Stat tracking manager is unresponsive. Attempting recovery.");
                AttemptComponentRecovery("StatTrackingManager");
            }
            
            // Log memory usage
            float memoryUsage = GetEstimatedMemoryUsage();
            m_LoggingSystem.LogDebug(string.Format("Current memory usage: %.2f MB", memoryUsage));
            
            // Reset error count periodically
            float currentTime = System.GetTickCount() / 1000.0;
            if (currentTime - m_fLastErrorTime > m_fErrorCooldownPeriod * 5)
            {
                m_iErrorCount = Math.Max(0, m_iErrorCount - 2); // Decay error count over time
            }
        }
        catch (Exception e)
        {
            m_LoggingSystem.LogError("Exception in health monitoring: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Record an error, with rate limiting
    protected void RecordError(string source, string error)
    {
        float currentTime = System.GetTickCount() / 1000.0;
        
        // Rate limit error recording
        if (currentTime - m_fLastErrorTime < m_fErrorCooldownPeriod)
            return;
            
        m_fLastErrorTime = currentTime;
        m_iErrorCount++;
        
        m_LoggingSystem.LogError("Error in " + source + ": " + error);
    }
    
    //------------------------------------------------------------------------------------------------
    // Enter recovery mode
    protected void EnterRecoveryMode()
    {
        // Already in recovery mode
        if (m_bInRecoveryMode)
            return;
            
        m_bInRecoveryMode = true;
        m_LoggingSystem.LogWarning("System entering recovery mode. Disabling non-essential components.");
        
        try
        {
            // Disable non-essential components to reduce load
            DisableNonEssentialComponents();
            
            // Force garbage collection
            m_LoggingSystem.LogInfo("Requesting garbage collection");
            // Add game-specific GC request here if available
            
            // Log recovery mode entry
            string recoveryLogEntry = "RECOVERY MODE ENABLED due to error threshold exceeded. Error count: " + m_iErrorCount;
            m_LoggingSystem.LogCritical(recoveryLogEntry);
            
            // Schedule exit from recovery mode after a cooling-off period
            if (m_CallQueue)
            {
                m_CallQueue.CallLater(ExitRecoveryMode, 300000, false); // 5 minutes
            }
        }
        catch (Exception e)
        {
            m_LoggingSystem.LogCritical("Exception during recovery mode entry: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Exit recovery mode
    protected void ExitRecoveryMode()
    {
        if (!m_bInRecoveryMode)
            return;
            
        try
        {
            // Check if errors are still occurring
            if (m_iErrorCount > ERROR_THRESHOLD / 2)
            {
                m_LoggingSystem.LogWarning("Still too many errors to exit recovery mode. Extending recovery period.");
                if (m_CallQueue)
                {
                    m_CallQueue.CallLater(ExitRecoveryMode, 300000, false); // Try again in 5 minutes
                }
                return;
            }
            
            m_bInRecoveryMode = false;
            m_iErrorCount = 0;
            m_LoggingSystem.LogInfo("Exiting recovery mode. Attempting to restart components.");
            
            // Try to restart components
            RestartComponents();
            
            m_LoggingSystem.LogInfo("Recovery mode exited successfully.");
        }
        catch (Exception e)
        {
            m_LoggingSystem.LogError("Exception during recovery mode exit: " + e.ToString());
            
            // If error during exit, stay in recovery mode
            m_bInRecoveryMode = true;
            
            // Try again later
            if (m_CallQueue)
            {
                m_CallQueue.CallLater(ExitRecoveryMode, 600000, false); // Try again in 10 minutes
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Disable non-essential components
    protected void DisableNonEssentialComponents()
    {
        try
        {
            // Disable web API
            if (m_APIServer)
            {
                m_APIServer.Shutdown();
                m_LoggingSystem.LogInfo("API server disabled for recovery");
            }
            
            // Disable admin dashboard
            if (m_AdminDashboard)
            {
                m_AdminDashboard.Shutdown();
                m_LoggingSystem.LogInfo("Admin dashboard disabled for recovery");
            }
            
            // Disable heatmaps
            if (m_HeatmapManager)
            {
                m_HeatmapManager.Shutdown();
                m_LoggingSystem.LogInfo("Heatmap manager disabled for recovery");
            }
            
            // Disable webhooks
            if (m_WebhookManager)
            {
                m_WebhookManager.SetEnabled(false);
                m_LoggingSystem.LogInfo("Webhooks disabled for recovery");
            }
            
            // Keep essential components running: logging, stat tracking, persistence
        }
        catch (Exception e)
        {
            m_LoggingSystem.LogError("Exception disabling components: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Restart components after recovery
    protected void RestartComponents()
    {
        try
        {
            // Only restart components that were enabled in config
            
            // Restart web API
            if (m_APIServer && m_Config.m_bEnableStatsAPI)
            {
                m_APIServer.Initialize();
                m_LoggingSystem.LogInfo("API server restarted after recovery");
            }
            
            // Restart admin dashboard
            if (m_AdminDashboard && m_Config.m_bEnableStatsAPI)
            {
                m_AdminDashboard.Initialize();
                m_LoggingSystem.LogInfo("Admin dashboard restarted after recovery");
            }
            
            // Restart heatmaps
            if (m_HeatmapManager && m_Config.m_bEnableHeatmaps)
            {
                m_HeatmapManager.Initialize();
                m_LoggingSystem.LogInfo("Heatmap manager restarted after recovery");
            }
            
            // Restart webhooks
            if (m_WebhookManager && m_Config.m_bEnableWebhooks)
            {
                m_WebhookManager.SetEnabled(true);
                m_LoggingSystem.LogInfo("Webhooks re-enabled after recovery");
            }
        }
        catch (Exception e)
        {
            m_LoggingSystem.LogError("Exception restarting components: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Attempt recovery of a specific component
    protected void AttemptComponentRecovery(string componentName)
    {
        try
        {
            m_LoggingSystem.LogInfo("Attempting to recover component: " + componentName);
            
            switch (componentName)
            {
                case "PersistenceManager":
                    if (m_PersistenceManager)
                    {
                        m_PersistenceManager.Reset();
                        m_LoggingSystem.LogInfo("PersistenceManager reset attempted");
                    }
                    break;
                    
                case "StatTrackingManager":
                    if (m_StatTrackingManager)
                    {
                        // Force refresh stats
                        m_StatTrackingManager.ForceRefresh();
                        m_LoggingSystem.LogInfo("StatTrackingManager refresh attempted");
                    }
                    break;
                    
                default:
                    m_LoggingSystem.LogWarning("No recovery procedure for component: " + componentName);
            }
        }
        catch (Exception e)
        {
            m_LoggingSystem.LogError("Exception recovering component " + componentName + ": " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get estimated memory usage
    protected float GetEstimatedMemoryUsage()
    {
        // This is a placeholder - actual implementation would depend on the game engine's API
        // Return a random value for demonstration
        return Math.RandomFloat(100, 1000);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_Main GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_Main();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if the system is in recovery mode
    bool IsInRecoveryMode()
    {
        return m_bInRecoveryMode;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current system health status
    string GetHealthStatus()
    {
        if (!m_bInitialized)
            return "Not Initialized";
            
        if (m_bInRecoveryMode)
            return "Recovery Mode";
            
        if (m_iErrorCount > ERROR_THRESHOLD / 2)
            return "Degraded";
            
        return "Healthy";
    }

    override void OnGameModeLoad()
    {
        super.OnGameModeLoad();
        
        try
        {
            // Initialize logging first
            m_LoggingSystem = STS_LoggingSystem.GetInstance();
            if (!m_LoggingSystem)
            {
                Print("[StatTracker] CRITICAL ERROR: Failed to initialize logging system", LogLevel.ERROR);
                return;
            }
            
            m_LoggingSystem.LogInfo("OnGameModeLoad called, initializing systems");
            
            // Rest of initialization
            // ...
        }
        catch (Exception e)
        {
            if (m_LoggingSystem)
                m_LoggingSystem.LogCritical("Exception in OnGameModeLoad: " + e.ToString());
            else
                Print("[StatTracker] CRITICAL ERROR in OnGameModeLoad: " + e.ToString(), LogLevel.ERROR);
        }
    }
}

//------------------------------------------------------------------------------------------------
// Auto-initialize the system when the game starts
void STS_Init()
{
    // Wait a bit to ensure all systems are ready
    GetGame().GetCallqueue().CallLater(STS_Main.GetInstance, 2000, false);
} 