// STS_DatabaseInit.c
// Initializes the database system on mod loading

[RegisterModule(EModuleInitOrder.Early)]
class STS_DatabaseInitModule: SCR_ModuleBase
{
    // Logger for diagnostics
    protected STS_LoggingSystem m_Logger;
    
    // Default database configuration
    [Attribute("StatTracker", UIWidgets.EditBox, "Database name", "")]
    protected string m_sDatabaseName;
    
    [Attribute("0", UIWidgets.ComboBox, "Database type to use", "", ParamEnumArray.FromEnum(STS_DatabaseType))]
    protected STS_DatabaseType m_eDatabaseType;
    
    [Attribute("", UIWidgets.EditBox, "Connection string for remote databases", "")]
    protected string m_sConnectionString;
    
    [Attribute("1", UIWidgets.CheckBox, "Use auto-detection for best database type", "")]
    protected bool m_bUseAutoDetection;
    
    //------------------------------------------------------------------------------------------------
    override void OnInit(IEntity owner)
    {
        super.OnInit(owner);
        
        // Get logger
        m_Logger = STS_LoggingSystem.GetInstance();
        
        if (!m_Logger)
        {
            Print("[StatTracker] WARNING: Could not get logger - database initialization will have limited logging");
        }
        else
        {
            m_Logger.LogInfo("Database initialization module starting", "STS_DatabaseInitModule", "OnInit");
        }
        
        // Initialize the database system
        InitializeDatabase();
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize the database system
    protected void InitializeDatabase()
    {
        try
        {
            // Get database manager
            STS_DatabaseManager dbManager = STS_DatabaseManager.GetInstance();
            
            // Use auto-detection if enabled
            if (m_bUseAutoDetection)
            {
                if (m_Logger)
                {
                    m_Logger.LogInfo("Using auto-detection for best database type", 
                        "STS_DatabaseInitModule", "InitializeDatabase");
                }
                
                if (!dbManager.InitializeWithBestSettings())
                {
                    if (m_Logger)
                    {
                        m_Logger.LogError("Failed to initialize database with auto-detected settings", 
                            "STS_DatabaseInitModule", "InitializeDatabase");
                    }
                    else
                    {
                        Print("[StatTracker] ERROR: Failed to initialize database with auto-detected settings");
                    }
                }
                else
                {
                    if (m_Logger)
                    {
                        m_Logger.LogInfo("Successfully initialized database with auto-detected settings", 
                            "STS_DatabaseInitModule", "InitializeDatabase");
                    }
                    else
                    {
                        Print("[StatTracker] Database initialized with auto-detected settings");
                    }
                }
            }
            else
            {
                // Initialize with explicit settings
                if (m_Logger)
                {
                    m_Logger.LogInfo(string.Format("Initializing database with explicit settings - Type: %1, Name: %2", 
                        m_eDatabaseType, m_sDatabaseName), "STS_DatabaseInitModule", "InitializeDatabase");
                }
                
                if (!dbManager.Initialize(m_eDatabaseType, m_sDatabaseName, m_sConnectionString))
                {
                    if (m_Logger)
                    {
                        m_Logger.LogError(string.Format("Failed to initialize database with explicit settings - Type: %1, Name: %2", 
                            m_eDatabaseType, m_sDatabaseName), "STS_DatabaseInitModule", "InitializeDatabase");
                    }
                    else
                    {
                        Print(string.Format("[StatTracker] ERROR: Failed to initialize database - Type: %1, Name: %2", 
                            m_eDatabaseType, m_sDatabaseName));
                    }
                }
                else
                {
                    if (m_Logger)
                    {
                        m_Logger.LogInfo(string.Format("Successfully initialized database - Type: %1, Name: %2", 
                            m_eDatabaseType, m_sDatabaseName), "STS_DatabaseInitModule", "InitializeDatabase");
                    }
                    else
                    {
                        Print(string.Format("[StatTracker] Database initialized - Type: %1, Name: %2", 
                            m_eDatabaseType, m_sDatabaseName));
                    }
                }
            }
        }
        catch (Exception e)
        {
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception during database initialization: %1", e.ToString()), 
                    "STS_DatabaseInitModule", "InitializeDatabase");
            }
            else
            {
                Print(string.Format("[StatTracker] CRITICAL ERROR during database initialization: %1", e.ToString()));
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnGameStart()
    {
        super.OnGameStart();
        
        // Setup a periodic task to check for scheduled operations
        GetGame().GetCallqueue().CallLater(ProcessScheduledOperations, 60000, true); // Check every minute
    }
    
    //------------------------------------------------------------------------------------------------
    // Process scheduled database operations
    protected void ProcessScheduledOperations()
    {
        try
        {
            // Get persistence manager to process operations
            STS_PersistenceManager persistenceManager = STS_PersistenceManager.GetInstance();
            if (persistenceManager)
            {
                persistenceManager.ProcessScheduledOperations();
            }
        }
        catch (Exception e)
        {
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception in ProcessScheduledOperations: %1", e.ToString()), 
                    "STS_DatabaseInitModule", "ProcessScheduledOperations");
            }
            else
            {
                Print(string.Format("[StatTracker] ERROR in ProcessScheduledOperations: %1", e.ToString()));
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnGameEnd()
    {
        super.OnGameEnd();
        
        // Shutdown the database properly
        try
        {
            // Get persistence manager to handle shutdown
            STS_PersistenceManager persistenceManager = STS_PersistenceManager.GetInstance();
            if (persistenceManager)
            {
                if (m_Logger)
                {
                    m_Logger.LogInfo("Shutting down persistence manager on game end", 
                        "STS_DatabaseInitModule", "OnGameEnd");
                }
                
                persistenceManager.Shutdown();
            }
        }
        catch (Exception e)
        {
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception during persistence shutdown: %1", e.ToString()), 
                    "STS_DatabaseInitModule", "OnGameEnd");
            }
            else
            {
                Print(string.Format("[StatTracker] ERROR during persistence shutdown: %1", e.ToString()));
            }
        }
    }
} 