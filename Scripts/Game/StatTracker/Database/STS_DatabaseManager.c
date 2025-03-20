// STS_DatabaseManager.c
// Central manager for database operations using Enfusion Database Framework

enum STS_DatabaseType
{
    JSON_FILE,      // Simple JSON files for development or small servers
    BINARY_FILE,    // Binary files for better performance and size
    MONGODB,        // MongoDB document database for larger servers
    MYSQL,          // MySQL relational database
    POSTGRESQL      // PostgreSQL relational database
}

enum STS_DatabaseError
{
    NONE,                   // No error
    CONNECTION_FAILED,      // Failed to connect to database
    INITIALIZATION_FAILED,  // General initialization failure
    INVALID_CONFIG,         // Invalid configuration
    QUERY_FAILED,           // Query execution failed
    TIMEOUT,                // Operation timed out
    PERMISSION_DENIED,      // Permission denied to perform operation
    DATABASE_CORRUPTED,     // Database appears to be corrupted
    DISK_FULL,              // Disk is full
    INVALID_OPERATION,      // Invalid operation requested
    CONNECTION_LOST,        // Connection lost during operation
    RECOVERY_FAILED,        // Recovery attempt failed
    BACKUP_FAILED,          // Failed to create backup
    SCHEMA_MISMATCH,        // Database schema version mismatch
    DATA_VALIDATION_FAILED, // Data validation failed
    RATE_LIMIT_EXCEEDED,    // Too many operations in short time
    TRANSACTION_FAILED,     // Transaction failed to commit
    DEADLOCK_DETECTED,      // Deadlock detected in database operations
    FILE_LOCK_ERROR,        // File locked by another process
    BACKUP_RESTORE_FAILED,  // Failed to restore from backup
    NETWORK_UNREACHABLE,    // Network connectivity issues
    UNKNOWN                 // Unknown error
}

// Structure to represent a pending database operation for retry
class STS_PendingDatabaseOperation
{
    string m_sOperationType;                    // Type of operation (e.g., "SavePlayerStats", "DeletePlayerStats")
    ref map<string, string> m_mParameters;      // Operation parameters
    int m_iPriority;                           // Operation priority (higher values = higher priority)
    int m_iAttempts;                           // Number of attempts made
    float m_fLastAttemptTime;                  // Time of last attempt
    string m_sError;                           // Last error message

    void STS_PendingDatabaseOperation(string operationType, map<string, string> parameters, int priority = 1)
    {
        m_sOperationType = operationType;
        m_mParameters = parameters;
        m_iPriority = priority;
        m_iAttempts = 0;
        m_fLastAttemptTime = 0;
        m_sError = "";
    }
}

class STS_DatabaseManager
{
    // Singleton instance
    protected static ref STS_DatabaseManager s_Instance;
    
    // Database context
    protected ref EDF_DbContext m_DbContext;
    
    // Repositories
    protected ref STS_PlayerStatsRepository m_PlayerStatsRepository;
    protected ref STS_TeamKillRepository m_TeamKillRepository;
    protected ref STS_VoteKickRepository m_VoteKickRepository;
    
    // Configuration
    protected STS_DatabaseType m_eDatabaseType;
    protected string m_sDatabaseName;
    protected string m_sConnectionString;
    protected int m_iConnectionTimeoutMs = 30000;  // 30 seconds default timeout
    protected int m_iQueryTimeoutMs = 10000;       // 10 seconds default query timeout
    protected int m_iMaxRetryAttempts = 3;         // Max retry attempts for failed operations
    protected int m_iRetryDelayMs = 1000;          // Delay between retries in milliseconds
    protected int m_iBackupIntervalMinutes = 60;   // Backup every hour (for file-based DBs)
    protected string m_sBackupDirectory = "$profile:StatTracker/Backups/";
    protected int m_iMaxBackups = 5;               // Maximum number of backup files to keep
    protected int m_iMaxPendingOperations = 1000;  // Maximum pending operations to queue
    protected int m_iHealthCheckIntervalMs = 300000; // Health check interval (5 minutes)
    protected int m_iAutoRecoveryThreshold = 5;    // Auto-recovery after N consecutive failures
    protected float m_fOperationRateLimit = 100;   // Max operations per second
    protected bool m_bAutoReconnect = true;        // Attempt auto-reconnect on connection loss
    
    // Status tracking
    protected bool m_bInitialized = false;
    protected STS_DatabaseError m_eLastError = STS_DatabaseError.NONE;
    protected string m_sLastErrorMessage = "";
    protected string m_sLastErrorStackTrace = "";
    protected int m_iConnectAttempts = 0;
    protected int m_iFailedOperations = 0;
    protected int m_iSuccessfulOperations = 0;
    protected float m_fLastSuccessfulOperation = 0;
    protected bool m_bMaintenanceMode = false;
    protected int m_iLastBackupTime = 0;
    protected bool m_bHealthCheckInProgress = false;
    protected int m_iConsecutiveFailures = 0;
    protected bool m_bDataCorruptionDetected = false;
    protected float m_fLastOperationTime = 0;      // Time of last operation
    protected int m_iOperationsInLastSecond = 0;   // Operations in the last second (rate limiting)
    protected float m_fOperationRateLimitStartTime = 0; // Start time for rate limit window
    protected bool m_bReconnecting = false;        // Flag for reconnection in progress
    protected int m_iLastOperationTime = 0;        // Time of last operation (for timeout detection)
    protected int m_iSchemaVersion = 1;            // Current schema version
    
    // Logging
    protected STS_LoggingSystem m_Logger;
    
    // Critical operation queue for retrying important operations
    protected ref array<ref STS_PendingDatabaseOperation> m_aPendingOperations = new array<ref STS_PendingDatabaseOperation>();
    
    // Enhanced error tracking
    protected ref map<STS_DatabaseError, int> m_mErrorCounts = new map<STS_DatabaseError, int>();
    protected ref map<STS_DatabaseError, ref array<string>> m_mErrorContexts = new map<STS_DatabaseError, ref array<string>>();
    protected const int MAX_ERROR_CONTEXTS = 10;
    
    // Transaction management
    protected bool m_bInTransaction = false;
    protected ref array<ref STS_PendingDatabaseOperation> m_aTransactionOperations = new array<ref STS_PendingDatabaseOperation>();
    protected const int MAX_TRANSACTION_OPERATIONS = 100;
    
    // Recovery settings
    protected const float RECOVERY_CHECK_INTERVAL = 300; // 5 minutes
    protected const int MAX_RECOVERY_ATTEMPTS = 3;
    protected int m_iRecoveryAttempts = 0;
    protected float m_fLastRecoveryAttempt = 0;
    
    //------------------------------------------------------------------------------------------------
    protected void STS_DatabaseManager()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        
        if (!m_Logger)
        {
            Print("[StatTracker] WARNING: Logger not initialized in database manager. Using direct prints.");
            Print("[StatTracker] Database Manager initializing");
        }
        else
        {
            m_Logger.LogInfo("Database Manager initializing", "STS_DatabaseManager", "Constructor");
        }
        
        // Create backup directory with robust error handling
        if (!FileIO.FileExists(m_sBackupDirectory))
        {
            if (!FileIO.MakeDirectory(m_sBackupDirectory))
            {
                LogWarning(string.Format("Failed to create backup directory at %1. Backups will be disabled.", m_sBackupDirectory), "Constructor");
                
                // Try creating parent directories recursively
                array<string> pathParts = new array<string>();
                m_sBackupDirectory.Split("/", pathParts);
                
                string currentPath = "";
                for (int i = 0; i < pathParts.Count() - 1; i++)
                {
                    currentPath += pathParts[i] + "/";
                    if (!FileIO.FileExists(currentPath))
                    {
                        FileIO.MakeDirectory(currentPath);
                    }
                }
                
                // Try again after creating parent directories
                if (!FileIO.MakeDirectory(m_sBackupDirectory))
                {
                    LogError("Failed to create backup directory after recursive attempt. Backups will be disabled.", 
                        "Constructor", "");
                }
            }
        }
        
        // Set up health check timer
        GetGame().GetCallqueue().CallLater(PerformHealthCheck, m_iHealthCheckIntervalMs, true);
        
        // Initialize rate limiting
        m_fOperationRateLimitStartTime = GetCurrentTimeMs();
        m_iOperationsInLastSecond = 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_DatabaseManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_DatabaseManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize the database system with comprehensive validation and error handling
    bool Initialize(STS_DatabaseType databaseType, string databaseName, string connectionString = "")
    {
        try
        {
            // Reset error state
            ResetErrorState();
            
            // Already initialized?
            if (m_bInitialized && m_DbContext)
            {
                LogWarning("Initialize called but database is already initialized. Call Shutdown first if you want to reinitialize.",
                    "Initialize");
                return true;
            }
            
            // Input validation
            if (databaseName.IsEmpty())
            {
                SetError(STS_DatabaseError.INVALID_CONFIG, "Database name cannot be empty");
                LogError("Database name cannot be empty", "Initialize");
                return false;
            }
            
            if (!IsValidDatabaseType(databaseType))
            {
                SetError(STS_DatabaseError.INVALID_CONFIG, string.Format("Invalid database type: %1", databaseType));
                LogError(string.Format("Invalid database type: %1", databaseType), "Initialize");
                return false;
            }
            
            if (!IsValidDatabaseName(databaseName))
            {
                SetError(STS_DatabaseError.INVALID_CONFIG, string.Format("Invalid database name: %1", databaseName));
                LogError(string.Format("Invalid database name: %1. Must contain only alphanumeric characters, underscores, and hyphens.", 
                        databaseName), "Initialize");
                return false;
            }
            
            // Ensure the database name is sanitized
            string sanitizedName = SanitizeDatabaseName(databaseName);
            if (sanitizedName != databaseName)
            {
                LogWarning(string.Format("Database name '%1' contains invalid characters. Using sanitized name '%2' instead.", 
                        databaseName, sanitizedName), "Initialize");
                databaseName = sanitizedName;
            }
            
            // For remote databases, ensure connection string is provided
            if (IsRemoteDatabaseType(databaseType) && connectionString.IsEmpty())
            {
                SetError(STS_DatabaseError.INVALID_CONFIG, "Connection string is required for remote database types");
                LogError("Connection string is required for remote database types", "Initialize");
                return false;
            }
            
            // For file-based databases, ensure we have sufficient disk space
            if (IsFileDatabaseType(databaseType))
            {
                string dbPath = "$profile:StatTracker/Databases/";
                
                // Make sure directory exists
                if (!EnsureDirectoryExists(dbPath))
                {
                    SetError(STS_DatabaseError.PERMISSION_DENIED, string.Format("Failed to create database directory: %1", dbPath));
                    LogError(string.Format("Failed to create database directory: %1", dbPath), "Initialize");
                    return false;
                }
                
                // Check disk space
                if (!HasSufficientDiskSpace(dbPath))
                {
                    SetError(STS_DatabaseError.DISK_FULL, "Insufficient disk space for database operations");
                    LogError("Insufficient disk space for database operations", "Initialize");
                    return false;
                }
            }
            
            // Set database configuration
            m_eDatabaseType = databaseType;
            m_sDatabaseName = databaseName;
            m_sConnectionString = connectionString;
            
            // Display database initialization details
            LogInfo(string.Format("Initializing %1 database '%2'", 
                GetDatabaseTypeString(databaseType), databaseName), "Initialize");
            
            // Create connection info
            EDF_ConnectionInfo connectionInfo = CreateConnectionInfo();
            if (!connectionInfo)
            {
                SetError(STS_DatabaseError.INVALID_CONFIG, "Failed to create connection info");
                LogError("Failed to create connection info", "Initialize");
                return false;
            }
            
            // Create database context with timeout
            float initStartTime = GetCurrentTimeMs();
            m_DbContext = EDF_DbContextManager.Get().CreateDbContext(connectionInfo);
            
            if (!m_DbContext)
            {
                SetError(STS_DatabaseError.CONNECTION_FAILED, "Failed to create database context");
                LogError("Failed to create database context", "Initialize");
                return false;
            }
            
            // Check connection - with timeout protection
            float connectionStartTime = GetCurrentTimeMs();
            bool connectionVerified = false;
            int connectionAttempts = 0;
            bool timedOut = false;
            
            while (!connectionVerified && connectionAttempts < m_iMaxRetryAttempts && !timedOut)
            {
                connectionAttempts++;
                m_iConnectAttempts++;
                
                try
                {
                    connectionVerified = VerifyDatabaseConnection();
                    
                    if (!connectionVerified)
                    {
                        LogWarning(string.Format("Connection verification failed, attempt %1 of %2", 
                            connectionAttempts, m_iMaxRetryAttempts), "Initialize");
                        
                        // Sleep before retry
                        if (connectionAttempts < m_iMaxRetryAttempts)
                        {
                            int delayTime = m_iRetryDelayMs * connectionAttempts; // Increasing backoff
                            Sleep(delayTime);
                        }
                    }
                }
                catch (Exception e)
                {
                    LogError(string.Format("Exception during connection verification: %1", e.ToString()), 
                        "Initialize", e.GetStackTrace());
                }
                
                // Check for timeout
                if (GetCurrentTimeMs() - connectionStartTime > m_iConnectionTimeoutMs)
                {
                    timedOut = true;
                    SetError(STS_DatabaseError.TIMEOUT, "Connection verification timed out");
                    LogError("Connection verification timed out", "Initialize");
                }
            }
            
            if (!connectionVerified)
            {
                if (!timedOut)
                {
                    SetError(STS_DatabaseError.CONNECTION_FAILED, 
                        string.Format("Failed to verify database connection after %1 attempts", connectionAttempts));
                        
                    LogError(string.Format("Failed to verify database connection after %1 attempts", connectionAttempts), 
                        "Initialize");
                }
                
                // Clean up resources
                m_DbContext = null;
                
                return false;
            }
            
            // Verify/create database schema
            if (!VerifyOrCreateSchema())
            {
                SetError(STS_DatabaseError.INITIALIZATION_FAILED, "Failed to verify or create database schema");
                LogError("Failed to verify or create database schema", "Initialize");
                
                // Clean up resources
                m_DbContext = null;
                
                return false;
            }
            
            // Initialize repositories
            if (!InitializeRepositories())
            {
                SetError(STS_DatabaseError.INITIALIZATION_FAILED, "Failed to initialize repositories");
                LogError("Failed to initialize repositories", "Initialize");
                
                // Clean up resources
                m_DbContext = null;
                
                return false;
            }
            
            // Set as initialized
            m_bInitialized = true;
            m_iConsecutiveFailures = 0;
            m_fLastSuccessfulOperation = GetCurrentTimeMs();
            
            float initTime = (GetCurrentTimeMs() - initStartTime) / 1000.0;
            LogInfo(string.Format("Database initialization completed successfully in %.2f seconds", initTime), "Initialize");
            
            // Set up pending operations processing
            GetGame().GetCallqueue().CallLater(ProcessPendingOperations, 5000, true);
            
            // Set up recovery monitoring
            GetGame().GetCallqueue().CallLater(CheckRecovery, RECOVERY_CHECK_INTERVAL, true);
            
            // Initialize error tracking
            InitializeErrorTracking();
            
            // Enhanced logging of initialization
            LogInfo("Database system initialized", "Initialize", {
                "database_type": databaseType,
                "database_name": databaseName,
                "connection_string": connectionString,
                "backup_directory": m_sBackupDirectory,
                "max_retry_attempts": m_iMaxRetryAttempts
            });
            
            return true;
        }
        catch (Exception e)
        {
            HandleInitializationError(e);
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Enhanced error handling for initialization
    protected void HandleInitializationError(Exception e)
    {
        string errorContext = string.Format("Database initialization failed: %1\nStack trace: %2", 
            e.ToString(), e.StackTrace);
            
        SetError(STS_DatabaseError.INITIALIZATION_FAILED, errorContext);
        
        if (m_Logger)
        {
            m_Logger.LogError(errorContext, "STS_DatabaseManager", "HandleInitializationError", {
                "error_type": e.Type().ToString(),
                "database_type": m_eDatabaseType,
                "database_name": m_sDatabaseName
            });
        }
        else
        {
            Print("[StatTracker] CRITICAL ERROR: " + errorContext);
        }
        
        // Set up recovery attempt
        m_bIsRecovering = true;
        m_fLastRecoveryAttempt = GetCurrentTimeMs();
    }
    
    //------------------------------------------------------------------------------------------------
    // Verify or create the database schema
    protected bool VerifyOrCreateSchema()
    {
        try
        {
            if (!m_DbContext)
                return false;
            
            // Get current database schema version
            int dbVersion = GetDatabaseSchemaVersion();
            
            // If database is new (version 0), initialize it
            if (dbVersion == 0)
            {
                LogInfo("Creating new database schema", "VerifyOrCreateSchema");
                return CreateDatabaseSchema();
            }
            
            // Check for version mismatch
            if (dbVersion < m_iSchemaVersion)
            {
                LogInfo(string.Format("Database schema upgrade needed: %1 -> %2", dbVersion, m_iSchemaVersion), 
                    "VerifyOrCreateSchema");
                return UpgradeDatabaseSchema(dbVersion);
            }
            else if (dbVersion > m_iSchemaVersion)
            {
                // Database is from a newer version - could cause compatibility issues
                LogWarning(string.Format("Database schema is newer than expected: %1 > %2. This may cause compatibility issues.", 
                    dbVersion, m_iSchemaVersion), "VerifyOrCreateSchema");
                return true; // Continue anyway, but with warning
            }
            
            return true;
        }
        catch (Exception e)
        {
            SetError(STS_DatabaseError.SCHEMA_MISMATCH, 
                string.Format("Exception verifying database schema: %1", e.ToString()), e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the database schema version
    protected int GetDatabaseSchemaVersion()
    {
        try
        {
            // Check if version table exists
            string query = "SELECT Version FROM SchemaInfo LIMIT 1";
            EDF_DbQueryResult result = m_DbContext.ExecuteQuery(query);
            
            if (result && result.GetRowCount() > 0)
            {
                // Get version from table
                return result.GetValue(0, 0).ToInt();
            }
            
            return 0; // New database
        }
        catch (Exception e)
        {
            // Table likely doesn't exist yet
            return 0;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Create initial database schema
    protected bool CreateDatabaseSchema()
    {
        try
        {
            // Create schema version table
            string createVersionTable = "CREATE TABLE IF NOT EXISTS SchemaInfo (Version INT NOT NULL)";
            m_DbContext.ExecuteQuery(createVersionTable);
            
            // Insert initial version
            string insertVersion = string.Format("INSERT INTO SchemaInfo (Version) VALUES (%1)", m_iSchemaVersion);
            m_DbContext.ExecuteQuery(insertVersion);
            
            return true;
        }
        catch (Exception e)
        {
            SetError(STS_DatabaseError.INITIALIZATION_FAILED, 
                string.Format("Failed to create database schema: %1", e.ToString()), e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Upgrade database schema from older version
    protected bool UpgradeDatabaseSchema(int fromVersion)
    {
        try
        {
            LogInfo(string.Format("Upgrading database schema from v%1 to v%2", fromVersion, m_iSchemaVersion), 
                "UpgradeDatabaseSchema");
                
            // For now just update the version number
            // In a real implementation, you would add migration code here
            
            // Update schema version
            string updateVersion = string.Format("UPDATE SchemaInfo SET Version = %1", m_iSchemaVersion);
            m_DbContext.ExecuteQuery(updateVersion);
            
            return true;
        }
        catch (Exception e)
        {
            SetError(STS_DatabaseError.SCHEMA_MISMATCH, 
                string.Format("Failed to upgrade database schema: %1", e.ToString()), e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Ensure a directory exists, creating it if necessary
    protected bool EnsureDirectoryExists(string path)
    {
        if (FileIO.FileExists(path))
            return true;
            
        return FileIO.MakeDirectory(path);
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if there is sufficient disk space for database operations
    protected bool HasSufficientDiskSpace(string path)
    {
        // This is a simplified check, in a real implementation you would
        // use a system call to check actual free disk space
        
        // Try to write a small test file
        string testFile = path + "/.space_check";
        string testContent = "disk space check";
        
        bool success = FileIO.WriteFile(testFile, testContent);
        
        // Clean up test file
        if (success)
        {
            FileIO.DeleteFile(testFile);
        }
        
        return success;
    }
    
    //------------------------------------------------------------------------------------------------
    // Validate database name for invalid characters
    protected bool IsValidDatabaseName(string name)
    {
        // Disallow characters that might cause issues in file paths or SQL
        array<string> invalidChars = {"/", "\\", ":", "*", "?", "\"", "<", ">", "|", ";", "'", "`"};
        
        foreach (string invalidChar : invalidChars)
        {
            if (name.Contains(invalidChar))
                return false;
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Verify database connection with a simple query
    protected bool VerifyDatabaseConnection()
    {
        if (!m_DbContext)
            return false;
        
        try
        {
            LogDebug("Verifying database connection with test query", "VerifyDatabaseConnection");
            
            // Perform a simple query to verify connection
            string query = "SELECT 1";
            EDF_DbQueryResult result = m_DbContext.ExecuteQuery(query);
            
            if (!result)
            {
                LogError("Database test query failed", "VerifyDatabaseConnection", "");
                return false;
            }
            
            // Verify query returned expected result
            if (result.GetRowCount() < 1 || result.GetValue(0, 0).ToInt() != 1)
            {
                LogError("Database test query returned unexpected result", "VerifyDatabaseConnection", "");
                return false;
            }
            
            LogDebug("Database connection verified successfully", "VerifyDatabaseConnection");
            return true;
        }
        catch (Exception e)
        {
            LogError(string.Format("Exception during database connection verification: %1", e.ToString()),
                "VerifyDatabaseConnection", e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if database type is valid
    protected bool IsValidDatabaseType(STS_DatabaseType type)
    {
        return type >= STS_DatabaseType.JSON_FILE && type <= STS_DatabaseType.POSTGRESQL;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if database type is remote (needs connection string)
    protected bool IsRemoteDatabaseType(STS_DatabaseType type)
    {
        return type == STS_DatabaseType.MONGODB || 
               type == STS_DatabaseType.MYSQL || 
               type == STS_DatabaseType.POSTGRESQL;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if database type is file-based
    protected bool IsFileDatabaseType(STS_DatabaseType type)
    {
        return type == STS_DatabaseType.JSON_FILE || 
               type == STS_DatabaseType.BINARY_FILE;
    }
    
    //------------------------------------------------------------------------------------------------
    // Sanitize database name to prevent path traversal or injection
    protected string SanitizeDatabaseName(string name)
    {
        if (name.IsEmpty())
            return "StatTracker";
            
        // Replace any potentially dangerous characters
        name.Replace("../", "");
        name.Replace("..\\", "");
        name.Replace("/", "_");
        name.Replace("\\", "_");
        name.Replace(":", "_");
        name.Replace("*", "_");
        name.Replace("?", "_");
        name.Replace("\"", "_");
        name.Replace("<", "_");
        name.Replace(">", "_");
        name.Replace("|", "_");
        
        return name;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize with best database type based on server size
    bool InitializeWithBestSettings()
    {
        try
        {
            // Get player count or estimate
            int playerCount = GetRecommendedPlayerCount();
            
            // Set database type based on player count
            STS_DatabaseType recommendedType = DetermineBestDatabaseType(playerCount);
            
            LogInfo(string.Format("Auto-detecting database type for approx. %1 players: %2", 
                playerCount, GetDatabaseTypeString(recommendedType)), "InitializeWithBestSettings");
            
            // Use default database name if none set previously
            if (m_sDatabaseName.IsEmpty())
            {
                m_sDatabaseName = "StatTracker";
            }
            
            // Initialize with recommended type
            return Initialize(recommendedType, m_sDatabaseName, m_sConnectionString);
        }
        catch (Exception e)
        {
            string errorMsg = string.Format("Exception in auto-initialization: %1", e.ToString());
            SetError(STS_DatabaseError.INITIALIZATION_FAILED, errorMsg, e.GetStackTrace());
            
            // Fall back to JSON file storage on error
            LogWarning("Auto-detection failed. Falling back to JSON file storage.", "InitializeWithBestSettings");
            return Initialize(STS_DatabaseType.JSON_FILE, "StatTracker");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Create connection info based on database type
    protected EDF_ConnectionInfo CreateConnectionInfo()
    {
        try
        {
            EDF_ConnectionInfo connectionInfo;
            
            switch (m_eDatabaseType)
            {
                case STS_DatabaseType.JSON_FILE:
                    EDF_JsonFileConnectionInfo jsonInfo = new EDF_JsonFileConnectionInfo();
                    jsonInfo.SetDatabasePath(GetFileDatabasePath());
                    connectionInfo = jsonInfo;
                    break;
                    
                case STS_DatabaseType.BINARY_FILE:
                    EDF_BinaryFileConnectionInfo binaryInfo = new EDF_BinaryFileConnectionInfo();
                    binaryInfo.SetDatabasePath(GetFileDatabasePath());
                    connectionInfo = binaryInfo;
                    break;
                    
                case STS_DatabaseType.MONGODB:
                    EDF_MongoDBConnectionInfo mongoInfo = new EDF_MongoDBConnectionInfo();
                    if (!m_sConnectionString.IsEmpty())
                    {
                        mongoInfo.SetConnectionString(m_sConnectionString);
                    }
                    else
                    {
                        // Default MongoDB connection (localhost)
                        mongoInfo.SetHost("localhost");
                        mongoInfo.SetPort(27017);
                    }
                    mongoInfo.SetDatabaseName(m_sDatabaseName);
                    connectionInfo = mongoInfo;
                    break;
                    
                case STS_DatabaseType.MYSQL:
                    EDF_MySQLConnectionInfo mysqlInfo = new EDF_MySQLConnectionInfo();
                    if (!m_sConnectionString.IsEmpty())
                    {
                        mysqlInfo.SetConnectionString(m_sConnectionString);
                    }
                    else
                    {
                        // Default MySQL connection (localhost)
                        mysqlInfo.SetHost("localhost");
                        mysqlInfo.SetPort(3306);
                        mysqlInfo.SetUsername("stattracker");
                        mysqlInfo.SetPassword("stattracker");
                    }
                    mysqlInfo.SetDatabaseName(m_sDatabaseName);
                    connectionInfo = mysqlInfo;
                    break;
                    
                case STS_DatabaseType.POSTGRESQL:
                    EDF_PostgreSQLConnectionInfo pgInfo = new EDF_PostgreSQLConnectionInfo();
                    if (!m_sConnectionString.IsEmpty())
                    {
                        pgInfo.SetConnectionString(m_sConnectionString);
                    }
                    else
                    {
                        // Default PostgreSQL connection (localhost)
                        pgInfo.SetHost("localhost");
                        pgInfo.SetPort(5432);
                        pgInfo.SetUsername("stattracker");
                        pgInfo.SetPassword("stattracker");
                    }
                    pgInfo.SetDatabaseName(m_sDatabaseName);
                    connectionInfo = pgInfo;
                    break;
                    
                default:
                    LogError(string.Format("Unsupported database type: %1", m_eDatabaseType), "CreateConnectionInfo");
                    return null;
            }
            
            return connectionInfo;
        }
        catch (Exception e)
        {
            LogError(string.Format("Exception creating connection info: %1", e.ToString()), "CreateConnectionInfo");
            return null;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get path for file-based databases
    protected string GetFileDatabasePath()
    {
        return "$profile:StatTracker/Database/" + m_sDatabaseName;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize all repositories
    protected bool InitializeRepositories()
    {
        try
        {
            if (!m_DbContext)
            {
                SetError(STS_DatabaseError.INITIALIZATION_FAILED, "Cannot initialize repositories - database context is null");
                return false;
            }
            
            LogDebug("Initializing repositories...", "InitializeRepositories");
            
            // Initialize player stats repository
            m_PlayerStatsRepository = new STS_PlayerStatsRepository(m_DbContext);
            if (!m_PlayerStatsRepository)
            {
                SetError(STS_DatabaseError.INITIALIZATION_FAILED, "Failed to create player stats repository");
                return false;
            }
            
            // Initialize team kill repository
            m_TeamKillRepository = new STS_TeamKillRepository(m_DbContext);
            if (!m_TeamKillRepository)
            {
                SetError(STS_DatabaseError.INITIALIZATION_FAILED, "Failed to create team kill repository");
                return false;
            }
            
            // Initialize vote kick repository
            m_VoteKickRepository = new STS_VoteKickRepository(m_DbContext);
            if (!m_VoteKickRepository)
            {
                SetError(STS_DatabaseError.INITIALIZATION_FAILED, "Failed to create vote kick repository");
                return false;
            }
            
            LogDebug("Repository initialization successful", "InitializeRepositories");
            return true;
        }
        catch (Exception e)
        {
            string errorMsg = string.Format("Exception initializing repositories: %1", e.ToString());
            SetError(STS_DatabaseError.INITIALIZATION_FAILED, errorMsg, e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Determine the best database type based on server size
    protected STS_DatabaseType DetermineBestDatabaseType(int playerCount)
    {
        // Small servers (< 10 players): JSON files
        if (playerCount < 10)
            return STS_DatabaseType.JSON_FILE;
            
        // Medium servers (10-50 players): Binary files
        if (playerCount < 50)
            return STS_DatabaseType.BINARY_FILE;
            
        // Large servers (50+ players): MongoDB if connection string available
        if (!m_sConnectionString.IsEmpty())
            return STS_DatabaseType.MONGODB;
            
        // Fall back to binary files if no MongoDB connection
        return STS_DatabaseType.BINARY_FILE;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get approximate player count (or expected player count)
    protected int GetRecommendedPlayerCount()
    {
        // Try to get actual player count from server
        int actualCount = GetActualPlayerCount();
        if (actualCount >= 0)
            return actualCount;
            
        // Fall back to server max players if available
        int maxPlayers = GetServerMaxPlayers();
        if (maxPlayers > 0)
            return maxPlayers;
            
        // Default to medium size if we can't determine
        return 25;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get actual player count from server
    protected int GetActualPlayerCount()
    {
        try
        {
            GameMode gameMode = GetGame().GetGameMode();
            if (!gameMode)
                return -1;
                
            array<int> players = new array<int>();
            gameMode.GetPlayers(players);
            return players.Count();
        }
        catch (Exception e)
        {
            LogWarning(string.Format("Exception getting actual player count: %1", e.ToString()), "GetActualPlayerCount");
            return -1;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get server max players
    protected int GetServerMaxPlayers()
    {
        try
        {
            // Simplified implementation - in a real scenario, this would get the server config value
            return 50; // Default assumption
        }
        catch (Exception e)
        {
            LogWarning(string.Format("Exception getting server max players: %1", e.ToString()), "GetServerMaxPlayers");
            return -1;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Shutdown the database system
    bool Shutdown()
    {
        try
        {
            if (!m_bInitialized)
            {
                LogWarning("Shutdown called but database is not initialized", "Shutdown");
                return true;
            }
            
            LogInfo("Shutting down database", "Shutdown");
            
            // Clean up repositories
            m_PlayerStatsRepository = null;
            m_TeamKillRepository = null;
            m_VoteKickRepository = null;
            
            // Clean up database context
            if (m_DbContext)
            {
                delete m_DbContext;
                m_DbContext = null;
            }
            
            m_bInitialized = false;
            LogInfo("Database shutdown complete", "Shutdown");
            
            return true;
        }
        catch (Exception e)
        {
            string errorMsg = string.Format("Exception during database shutdown: %1", e.ToString());
            SetError(STS_DatabaseError.UNKNOWN, errorMsg, e.GetStackTrace());
            
            // Force cleanup despite error
            m_PlayerStatsRepository = null;
            m_TeamKillRepository = null;
            m_VoteKickRepository = null;
            m_DbContext = null;
            m_bInitialized = false;
            
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Run maintenance tasks
    void RunMaintenanceTasks()
    {
        if (!m_bInitialized || !m_DbContext)
        {
            LogWarning("RunMaintenanceTasks called but database is not initialized", "RunMaintenanceTasks");
            return;
        }
        
        try
        {
            m_bMaintenanceMode = true;
            LogInfo("Starting database maintenance", "RunMaintenanceTasks");
            
            // Perform maintenance tasks based on database type
            switch (m_eDatabaseType)
            {
                case STS_DatabaseType.JSON_FILE:
                case STS_DatabaseType.BINARY_FILE:
                    // For file-based databases, compact/optimize files
                    if (m_DbContext.OptimizeStorage())
                    {
                        LogInfo("File-based database optimized successfully", "RunMaintenanceTasks");
                    }
                    else
                    {
                        LogWarning("File-based database optimization failed", "RunMaintenanceTasks");
                    }
                    break;
                    
                case STS_DatabaseType.MONGODB:
                case STS_DatabaseType.MYSQL:
                case STS_DatabaseType.POSTGRESQL:
                    // For SQL-based databases, run vacuum/optimize commands
                    if (m_DbContext.OptimizeStorage())
                    {
                        LogInfo("Database optimized successfully", "RunMaintenanceTasks");
                    }
                    else
                    {
                        LogWarning("Database optimization failed", "RunMaintenanceTasks");
                    }
                    break;
            }
            
            m_bMaintenanceMode = false;
            LogInfo("Database maintenance completed", "RunMaintenanceTasks");
        }
        catch (Exception e)
        {
            m_bMaintenanceMode = false;
            string errorMsg = string.Format("Exception during database maintenance: %1", e.ToString());
            SetError(STS_DatabaseError.UNKNOWN, errorMsg, e.GetStackTrace());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get player stats repository
    STS_PlayerStatsRepository GetPlayerStatsRepository()
    {
        if (!m_bInitialized)
        {
            LogWarning("GetPlayerStatsRepository called but database is not initialized", "GetPlayerStatsRepository");
            return null;
        }
        
        return m_PlayerStatsRepository;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get team kill repository
    STS_TeamKillRepository GetTeamKillRepository()
    {
        if (!m_bInitialized)
        {
            LogWarning("GetTeamKillRepository called but database is not initialized", "GetTeamKillRepository");
            return null;
        }
        
        return m_TeamKillRepository;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get vote kick repository
    STS_VoteKickRepository GetVoteKickRepository()
    {
        if (!m_bInitialized)
        {
            LogWarning("GetVoteKickRepository called but database is not initialized", "GetVoteKickRepository");
            return null;
        }
        
        return m_VoteKickRepository;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get database status information
    void GetDatabaseStatus(out bool initialized, out string databaseType, out string databaseName, 
                           out int successCount, out int failCount, out STS_DatabaseError lastError,
                           out string lastErrorMessage)
    {
        initialized = m_bInitialized;
        databaseType = GetDatabaseTypeString(m_eDatabaseType);
        databaseName = m_sDatabaseName;
        successCount = m_iSuccessfulOperations;
        failCount = m_iFailedOperations;
        lastError = m_eLastError;
        lastErrorMessage = m_sLastErrorMessage;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if connection is healthy by performing a simple query
    bool IsConnectionHealthy()
    {
        if (!m_bInitialized || !m_DbContext)
            return false;
            
        try
        {
            // Try a simple database operation to verify connection
            bool result = m_DbContext.IsConnected();
            if (result)
            {
                m_fLastSuccessfulOperation = GetCurrentTimeMs();
            }
            return result;
        }
        catch (Exception e)
        {
            string errorMsg = string.Format("Exception checking connection health: %1", e.ToString());
            SetError(STS_DatabaseError.CONNECTION_FAILED, errorMsg);
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Attempt to recover from connection problems
    bool AttemptConnectionRecovery()
    {
        LogWarning("Attempting to recover database connection", "AttemptConnectionRecovery");
        
        try
        {
            // Store current configuration
            STS_DatabaseType currentType = m_eDatabaseType;
            string currentName = m_sDatabaseName;
            string currentConnection = m_sConnectionString;
            
            // Shutdown current connection
            Shutdown();
            
            // Wait briefly
            float startTime = GetCurrentTimeMs();
            while (GetCurrentTimeMs() - startTime < 1000)
            {
                // Artificial delay
            }
            
            // Try to reinitialize
            bool result = Initialize(currentType, currentName, currentConnection);
            
            if (result)
            {
                LogInfo("Database connection recovery successful", "AttemptConnectionRecovery");
            }
            else
            {
                LogError("Database connection recovery failed", "AttemptConnectionRecovery");
            }
            
            return result;
        }
        catch (Exception e)
        {
            string errorMsg = string.Format("Exception during connection recovery: %1", e.ToString());
            SetError(STS_DatabaseError.CONNECTION_FAILED, errorMsg, e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Set error state
    protected void SetError(STS_DatabaseError error, string message, string stackTrace = "")
    {
        m_eLastError = error;
        m_sLastErrorMessage = message;
        m_sLastErrorStackTrace = stackTrace;
        
        // Only increment if error is not NONE
        if (error != STS_DatabaseError.NONE)
        {
            m_iFailedOperations++;
            
            // Log with appropriate level based on error type
            switch (error)
            {
                case STS_DatabaseError.CONNECTION_FAILED:
                case STS_DatabaseError.INITIALIZATION_FAILED:
                case STS_DatabaseError.DATABASE_CORRUPTED:
                    LogError(message, "SetError", stackTrace);
                    break;
                    
                case STS_DatabaseError.INVALID_CONFIG:
                case STS_DatabaseError.TIMEOUT:
                case STS_DatabaseError.QUERY_FAILED:
                case STS_DatabaseError.DISK_FULL:
                    LogWarning(message, "SetError");
                    break;
                    
                default:
                    LogInfo(message, "SetError");
                    break;
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset error state
    protected void ResetErrorState()
    {
        m_eLastError = STS_DatabaseError.NONE;
        m_sLastErrorMessage = "";
        m_sLastErrorStackTrace = "";
    }
    
    //------------------------------------------------------------------------------------------------
    // Get database type as string
    protected string GetDatabaseTypeString(STS_DatabaseType type)
    {
        switch (type)
        {
            case STS_DatabaseType.JSON_FILE:
                return "JSON File";
                
            case STS_DatabaseType.BINARY_FILE:
                return "Binary File";
                
            case STS_DatabaseType.MONGODB:
                return "MongoDB";
                
            case STS_DatabaseType.MYSQL:
                return "MySQL";
                
            case STS_DatabaseType.POSTGRESQL:
                return "PostgreSQL";
                
            default:
                return "Unknown";
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current time in milliseconds
    protected float GetCurrentTimeMs()
    {
        return System.GetTickCount();
    }
    
    //------------------------------------------------------------------------------------------------
    // Logging helpers
    protected void LogDebug(string message, string methodName)
    {
        if (m_Logger)
            m_Logger.LogDebug(message, "STS_DatabaseManager", methodName);
        else
            Print(string.Format("[StatTracker][Database] DEBUG: %1", message));
    }
    
    protected void LogInfo(string message, string methodName)
    {
        if (m_Logger)
            m_Logger.LogInfo(message, "STS_DatabaseManager", methodName);
        else
            Print(string.Format("[StatTracker][Database] INFO: %1", message));
    }
    
    protected void LogWarning(string message, string methodName)
    {
        if (m_Logger)
            m_Logger.LogWarning(message, "STS_DatabaseManager", methodName);
        else
            Print(string.Format("[StatTracker][Database] WARNING: %1", message), LogLevel.WARNING);
    }
    
    protected void LogError(string message, string methodName, string stackTrace = "")
    {
        if (m_Logger)
            m_Logger.LogError(message, "STS_DatabaseManager", methodName, stackTrace);
        else
            Print(string.Format("[StatTracker][Database] ERROR: %1", message), LogLevel.ERROR);
    }
    
    //------------------------------------------------------------------------------------------------
    // Periodic health check to ensure database is functioning correctly
    protected void PerformHealthCheck()
    {
        if (!m_bInitialized || !m_DbContext || m_bHealthCheckInProgress)
            return;
            
        m_bHealthCheckInProgress = true;
        
        try
        {
            LogDebug("Performing database health check", "PerformHealthCheck");
            
            // Check if we can still query the database
            bool isHealthy = VerifyDatabaseConnection();
            
            if (!isHealthy)
            {
                LogWarning("Database health check failed - connection issues detected", "PerformHealthCheck");
                
                // Increment consecutive failures
                m_iConsecutiveFailures++;
                
                if (m_iConsecutiveFailures >= 3)
                {
                    LogError(string.Format("Database has failed %1 consecutive health checks - attempting recovery", 
                        m_iConsecutiveFailures), "PerformHealthCheck");
                    
                    // Attempt recovery
                    AttemptConnectionRecovery();
                }
            }
            else
            {
                // Reset failure counter on success
                if (m_iConsecutiveFailures > 0)
                {
                    LogInfo(string.Format("Database health check passed after %1 previous failures", 
                        m_iConsecutiveFailures), "PerformHealthCheck");
                }
                
                m_iConsecutiveFailures = 0;
                m_fLastSuccessfulOperation = GetCurrentTimeMs();
                
                // Perform maintenance if needed
                if (IsMaintenanceRequired())
                {
                    RunMaintenanceTasks();
                }
                
                // Check if it's time for a backup
                if (IsFileDatabaseType(m_eDatabaseType) && IsBackupRequired())
                {
                    CreateBackup();
                }
            }
        }
        catch (Exception e)
        {
            LogError(string.Format("Exception during database health check: %1", e.ToString()), 
                "PerformHealthCheck", e.GetStackTrace());
        }
        finally
        {
            m_bHealthCheckInProgress = false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if it's time for a backup
    protected bool IsBackupRequired()
    {
        // Get current time in seconds
        int currentTime = System.GetTickCount() / 1000;
        
        // Check if enough time has passed since the last backup
        return (currentTime - m_iLastBackupTime) > (m_iBackupIntervalMinutes * 60);
    }
    
    //------------------------------------------------------------------------------------------------
    // Create a backup of the database
    protected bool CreateBackup()
    {
        if (!IsFileDatabaseType(m_eDatabaseType))
            return false;
            
        try
        {
            LogInfo("Creating database backup", "CreateBackup");
            
            string sourcePath = GetFileDatabasePath();
            
            if (!FileIO.FileExists(sourcePath))
            {
                LogWarning(string.Format("Database path does not exist: %1", sourcePath), "CreateBackup");
                return false;
            }
            
            // Generate timestamp for backup
            int timestamp = System.GetTickCount() / 1000;
            string backupName = string.Format("%1_%2", m_sDatabaseName, timestamp);
            string backupPath = m_sBackupDirectory + backupName;
            
            // Create backup directory if it doesn't exist
            if (!FileIO.FileExists(m_sBackupDirectory))
            {
                if (!FileIO.MakeDirectory(m_sBackupDirectory))
                {
                    LogError(string.Format("Failed to create backup directory: %1", m_sBackupDirectory), "CreateBackup");
                    return false;
                }
            }
            
            // Copy database files to backup location
            // In a real implementation, we'd use proper file operations for directory copying
            // This is simplified for demonstration
            bool success = CopyDirectory(sourcePath, backupPath);
            
            if (success)
            {
                m_iLastBackupTime = System.GetTickCount() / 1000;
                LogInfo(string.Format("Database backup created successfully at %1", backupPath), "CreateBackup");
                
                // Cleanup old backups
                CleanupOldBackups();
                
                return true;
            }
            else
            {
                LogError(string.Format("Failed to create database backup at %1", backupPath), "CreateBackup");
                return false;
            }
        }
        catch (Exception e)
        {
            LogError(string.Format("Exception during database backup: %1", e.ToString()), 
                "CreateBackup", e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Remove old backup files to save disk space
    protected void CleanupOldBackups()
    {
        try
        {
            // Get all backup directories
            TStringArray backupDirs = new TStringArray();
            FindDirContent(m_sBackupDirectory, backupDirs);
            
            // Sort by timestamp (assuming naming convention includes timestamp)
            backupDirs.Sort(false); // Sort descending to have newest first
            
            // Keep only the most recent backups
            for (int i = m_iMaxBackups; i < backupDirs.Count(); i++)
            {
                string oldBackupPath = m_sBackupDirectory + backupDirs[i];
                DeleteDirectory(oldBackupPath);
                LogInfo(string.Format("Removed old database backup: %1", oldBackupPath), "CleanupOldBackups");
            }
        }
        catch (Exception e)
        {
            LogWarning(string.Format("Error during backup cleanup: %1", e.ToString()), "CleanupOldBackups");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a pending operation to the queue
    bool AddPendingOperation(string operationType, map<string, string> parameters, int priority = 1)
    {
        try
        {
            // Validate input
            if (operationType.IsEmpty())
            {
                LogError("Cannot add pending operation with empty type", "AddPendingOperation");
                return false;
            }
            
            // Create the operation
            STS_PendingDatabaseOperation operation = new STS_PendingDatabaseOperation();
            operation.m_sOperationType = operationType;
            operation.m_mParameters = parameters;
            operation.m_iPriority = priority;
            operation.m_iTimestamp = System.GetTickCount() / 1000;
            
            // Add to queue
            m_aPendingOperations.Insert(operation);
            
            LogInfo(string.Format("Added pending operation: %1 (Priority: %2, Params: %3)", 
                operationType, priority, parameters.Count()), "AddPendingOperation");
            
            return true;
        }
        catch (Exception e)
        {
            LogError(string.Format("Exception adding pending operation: %1", e.ToString()), 
                "AddPendingOperation", e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Process pending operations
    protected void ProcessPendingOperations()
    {
        if (!m_bInitialized || !m_DbContext)
            return;
            
        try
        {
            if (m_aPendingOperations.Count() == 0)
                return;
                
            LogInfo(string.Format("Processing %1 pending database operations", m_aPendingOperations.Count()), 
                "ProcessPendingOperations");
                
            // Sort operations by priority
            m_aPendingOperations.Sort();
            
            // Process high priority operations first
            foreach (STS_PendingDatabaseOperation operation : m_aPendingOperations)
            {
                if (operation.m_iPriority >= 3) // High priority
                {
                    if (!ExecuteOperation(operation))
                    {
                        LogWarning("Failed to process high priority operation", "ProcessPendingOperations", {
                            "operation_type": operation.m_sOperationType,
                            "attempts": operation.m_iAttempts
                        });
                    }
                }
            }
            
            // Process remaining operations
            foreach (STS_PendingDatabaseOperation operation : m_aPendingOperations)
            {
                if (operation.m_iPriority < 3)
                {
                    if (!ExecuteOperation(operation))
                    {
                        LogWarning("Failed to process operation", "ProcessPendingOperations", {
                            "operation_type": operation.m_sOperationType,
                            "attempts": operation.m_iAttempts
                        });
                    }
                }
            }
        }
        catch (Exception e)
        {
            LogError("Exception processing pending operations", "ProcessPendingOperations", {
                "error": e.ToString(),
                "pending_operations": m_aPendingOperations.Count()
            });
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Execute a specific pending operation
    protected bool ExecuteOperation(STS_PendingDatabaseOperation operation)
    {
        if (!operation)
            return false;
            
        try
        {
            LogDebug(string.Format("Executing pending operation: %1", operation.m_sOperationType), 
                "ExecuteOperation");
                
            // Handle different operation types
            switch (operation.m_sOperationType)
            {
                case "SavePlayerStats":
                    return ExecuteSavePlayerStats(operation);
                    
                case "DeletePlayerStats":
                    return ExecuteDeletePlayerStats(operation);
                    
                // Add more operation types as needed
                    
                default:
                    LogWarning(string.Format("Unknown pending operation type: %1", operation.m_sOperationType), 
                        "ExecuteOperation");
                    return false;
            }
        }
        catch (Exception e)
        {
            LogError(string.Format("Exception executing pending operation %1: %2", 
                operation.m_sOperationType, e.ToString()), "ExecuteOperation", e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Execute SavePlayerStats operation
    protected bool ExecuteSavePlayerStats(STS_PendingDatabaseOperation operation)
    {
        if (!m_PlayerStatsRepository)
            return false;
            
        try
        {
            string playerUID = operation.m_mParameters.Get("playerUID");
            string playerName = operation.m_mParameters.Get("playerName");
            string statsJson = operation.m_mParameters.Get("statsJson");
            
            if (playerUID.IsEmpty() || statsJson.IsEmpty())
            {
                LogError("Missing required parameters for SavePlayerStats operation", "ExecuteSavePlayerStats");
                return false;
            }
            
            // Create stats object from JSON
            STS_PlayerStats stats = new STS_PlayerStats();
            if (!stats.FromJSON(statsJson))
            {
                LogError(string.Format("Failed to parse stats JSON for player %1", playerUID), "ExecuteSavePlayerStats");
                return false;
            }
            
            // Save the stats
            return m_PlayerStatsRepository.SavePlayerStats(playerUID, playerName, stats);
        }
        catch (Exception e)
        {
            LogError(string.Format("Exception in ExecuteSavePlayerStats: %1", e.ToString()), 
                "ExecuteSavePlayerStats", e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Execute DeletePlayerStats operation
    protected bool ExecuteDeletePlayerStats(STS_PendingDatabaseOperation operation)
    {
        if (!m_PlayerStatsRepository)
            return false;
            
        try
        {
            string playerUID = operation.m_mParameters.Get("playerUID");
            
            if (playerUID.IsEmpty())
            {
                LogError("Missing playerUID parameter for DeletePlayerStats operation", "ExecuteDeletePlayerStats");
                return false;
            }
            
            // Delete the stats
            return m_PlayerStatsRepository.DeletePlayerStats(playerUID);
        }
        catch (Exception e)
        {
            LogError(string.Format("Exception in ExecuteDeletePlayerStats: %1", e.ToString()), 
                "ExecuteDeletePlayerStats", e.GetStackTrace());
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Enhanced transaction management
    bool BeginTransaction()
    {
        try
        {
            if (m_bInTransaction)
            {
                LogWarning("Transaction already in progress", "BeginTransaction");
                return false;
            }
            
            if (m_aTransactionOperations.Count() >= MAX_TRANSACTION_OPERATIONS)
            {
                LogError("Transaction operation limit reached", "BeginTransaction");
                return false;
            }
            
            m_bInTransaction = true;
            m_aTransactionOperations.Clear();
            
            LogDebug("Transaction started", "BeginTransaction");
            return true;
        }
        catch (Exception e)
        {
            HandleTransactionError(e, "BeginTransaction");
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    bool CommitTransaction()
    {
        try
        {
            if (!m_bInTransaction)
            {
                LogWarning("No transaction in progress", "CommitTransaction");
                return false;
            }
            
            bool success = true;
            foreach (STS_PendingDatabaseOperation operation : m_aTransactionOperations)
            {
                if (!ExecuteOperation(operation))
                {
                    success = false;
                    break;
                }
            }
            
            if (success)
            {
                LogInfo("Transaction committed successfully", "CommitTransaction", {
                    "operations": m_aTransactionOperations.Count()
                });
            }
            else
            {
                LogError("Transaction commit failed", "CommitTransaction");
            }
            
            m_bInTransaction = false;
            m_aTransactionOperations.Clear();
            return success;
        }
        catch (Exception e)
        {
            HandleTransactionError(e, "CommitTransaction");
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    void RollbackTransaction()
    {
        try
        {
            if (!m_bInTransaction)
            {
                LogWarning("No transaction in progress", "RollbackTransaction");
                return;
            }
            
            LogInfo("Rolling back transaction", "RollbackTransaction", {
                "operations": m_aTransactionOperations.Count()
            });
            
            m_bInTransaction = false;
            m_aTransactionOperations.Clear();
        }
        catch (Exception e)
        {
            HandleTransactionError(e, "RollbackTransaction");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void HandleTransactionError(Exception e, string operation)
    {
        string errorContext = string.Format("Transaction error in %1: %2\nStack trace: %3", 
            operation, e.ToString(), e.StackTrace);
            
        SetError(STS_DatabaseError.TRANSACTION_FAILED, errorContext);
        
        if (m_Logger)
        {
            m_Logger.LogError(errorContext, "STS_DatabaseManager", "HandleTransactionError", {
                "operation": operation,
                "error_type": e.Type().ToString(),
                "in_transaction": m_bInTransaction
            });
        }
        
        // Rollback on transaction error
        RollbackTransaction();
    }
    
    //------------------------------------------------------------------------------------------------
    // Enhanced recovery mechanism
    protected void CheckRecovery()
    {
        if (!m_bIsRecovering || !m_bInitialized) return;
        
        try
        {
            float currentTime = GetCurrentTimeMs();
            if (currentTime - m_fLastRecoveryAttempt >= RECOVERY_CHECK_INTERVAL)
            {
                if (m_iRecoveryAttempts >= MAX_RECOVERY_ATTEMPTS)
                {
                    LogCritical("Maximum recovery attempts reached", "CheckRecovery", {
                        "attempts": m_iRecoveryAttempts,
                        "last_error": m_sLastErrorMessage
                    });
                    m_bIsRecovering = false;
                    return;
                }
                
                m_iRecoveryAttempts++;
                m_fLastRecoveryAttempt = currentTime;
                
                // Attempt recovery
                if (AttemptRecovery())
                {
                    LogInfo("Database system recovered successfully", "CheckRecovery", {
                        "attempts": m_iRecoveryAttempts
                    });
                    m_bIsRecovering = false;
                    m_iRecoveryAttempts = 0;
                }
                else
                {
                    LogWarning("Recovery attempt failed", "CheckRecovery", {
                        "attempt": m_iRecoveryAttempts
                    });
                }
            }
        }
        catch (Exception e)
        {
            LogError("Exception during recovery check", "CheckRecovery", {
                "error": e.ToString(),
                "attempts": m_iRecoveryAttempts
            });
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool AttemptRecovery()
    {
        try
        {
            // Close existing connection if any
            if (m_DbContext)
            {
                m_DbContext.Close();
                m_DbContext = null;
            }
            
            // Clear error state
            ResetErrorState();
            
            // Re-initialize database connection
            if (!Initialize(m_eDatabaseType, m_sDatabaseName, m_sConnectionString))
            {
                return false;
            }
            
            // Verify database integrity
            if (!VerifyDatabaseIntegrity())
            {
                return false;
            }
            
            // Process any pending operations
            ProcessPendingOperations();
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Exception during recovery attempt", "AttemptRecovery", {
                "error": e.ToString()
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool VerifyDatabaseIntegrity()
    {
        try
        {
            // Check database schema version
            if (!VerifySchemaVersion())
            {
                LogError("Schema version mismatch detected", "VerifyDatabaseIntegrity");
                return false;
            }
            
            // Check for data corruption
            if (m_bDataCorruptionDetected)
            {
                LogError("Data corruption detected", "VerifyDatabaseIntegrity");
                return false;
            }
            
            // Perform basic connectivity test
            if (!TestDatabaseConnection())
            {
                LogError("Database connectivity test failed", "VerifyDatabaseIntegrity");
                return false;
            }
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Exception during database integrity check", "VerifyDatabaseIntegrity", {
                "error": e.ToString()
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool TestDatabaseConnection()
    {
        try
        {
            // Simple query to test connection
            string testQuery = "SELECT 1";
            if (!ExecuteQuery(testQuery))
            {
                return false;
            }
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Connection test failed", "TestDatabaseConnection", {
                "error": e.ToString()
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize error tracking
    protected void InitializeErrorTracking()
    {
        m_mErrorCounts.Clear();
        m_mErrorContexts.Clear();
    }
} 