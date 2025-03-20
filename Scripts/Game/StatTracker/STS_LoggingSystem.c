// STS_LoggingSystem.c
// Comprehensive logging system for the StatTracker mod

enum ELogLevel 
{
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
}

enum ELogCategory
{
    GENERAL,
    NETWORK,
    PERSISTENCE,
    UI,
    GAME_EVENT,
    TEAMKILL,
    VOTEKICK,
    CHAT,
    ADMIN,
    PERFORMANCE
}

class STS_LoggingSystem
{
    // Singleton instance
    private static ref STS_LoggingSystem s_Instance;
    
    // Log file paths and directories
    protected const string LOG_DIRECTORY = "$profile:StatTracker/Logs/";
    protected string m_sSessionLogDirectory;
    protected string m_sSessionID;
    
    // Level-specific log files
    protected ref FileHandle m_DebugLogFile;
    protected ref FileHandle m_InfoLogFile;
    protected ref FileHandle m_WarningLogFile;
    protected ref FileHandle m_ErrorLogFile;
    protected ref FileHandle m_CriticalLogFile;
    
    // Special log files
    protected ref FileHandle m_ChatLogFile;
    protected ref FileHandle m_VoteKickLogFile;
    protected ref FileHandle m_PerformanceLogFile;
    
    // Level-specific log file paths
    protected string m_sDebugLogPath;
    protected string m_sInfoLogPath;
    protected string m_sWarningLogPath;
    protected string m_sErrorLogPath;
    protected string m_sCriticalLogPath;
    protected string m_sChatLogPath;
    protected string m_sVoteKickLogPath;
    protected string m_sPerformanceLogPath;
    
    // In-memory log buffer for recent entries (circular buffer)
    protected const int MAX_MEMORY_LOGS = 1000;
    protected ref array<string> m_aMemoryLogs = new array<string>();
    protected int m_iMemoryLogIndex = 0;
    
    // Configuration
    protected ELogLevel m_eConsoleLogLevel = ELogLevel.INFO;
    protected ELogLevel m_eFileLogLevel = ELogLevel.DEBUG;
    protected bool m_bLogToConsole = true;
    protected bool m_bLogToFile = true;
    protected bool m_bAddStackTraceToErrors = true;
    protected bool m_bIsServer = false;
    
    // Log file management
    protected ref map<string, ref FileHandle> m_mLogFiles = new map<string, ref FileHandle>();
    protected int m_iMaxLogSizeMB = 10; // Maximum log file size in MB
    protected int m_iMaxLogFiles = 5; // Maximum number of rotated log files to keep
    protected float m_fLastRotationCheck = 0; // Last time log rotation was checked
    
    // Stats for diagnostics
    protected int m_iLoggedMessages = 0;
    protected int m_iErrorCount = 0;
    protected int m_iFileWriteErrors = 0;
    
    // Enhanced error tracking
    protected ref map<string, int> m_mErrorCounts = new map<string, int>();
    protected ref map<string, ref array<string>> m_mErrorContexts = new map<string, ref array<string>>();
    protected const int MAX_ERROR_CONTEXTS = 10;
    
    // File rotation settings
    protected const int MAX_LOG_SIZE_MB = 50;
    protected const int MAX_ROTATED_FILES = 5;
    protected const float ROTATION_CHECK_INTERVAL = 300; // 5 minutes
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_LoggingSystem()
    {
        try {
            // Determine if we're on server
            m_bIsServer = Replication.IsServer();
            
            // Generate session ID with timestamp
            m_sSessionID = GenerateSessionID();
            
            // Create session-specific log directory
            m_sSessionLogDirectory = LOG_DIRECTORY + m_sSessionID + "/";
            
            // Try to create log directory and all parent directories
            if (!CreateLogDirectories()) {
                Print("[StatTracker] ERROR: Failed to create log directories. File logging will be disabled.", LogLevel.ERROR);
                m_bLogToFile = false;
            }
            
            // Generate file paths for different log levels
            string instanceType = m_bIsServer ? "Server" : "Client";
            string timestamp = GetTimestamp(true);
            
            m_sDebugLogPath = m_sSessionLogDirectory + "Debug_" + instanceType + "_" + timestamp + ".log";
            m_sInfoLogPath = m_sSessionLogDirectory + "Info_" + instanceType + "_" + timestamp + ".log";
            m_sWarningLogPath = m_sSessionLogDirectory + "Warning_" + instanceType + "_" + timestamp + ".log";
            m_sErrorLogPath = m_sSessionLogDirectory + "Error_" + instanceType + "_" + timestamp + ".log";
            m_sCriticalLogPath = m_sSessionLogDirectory + "Critical_" + instanceType + "_" + timestamp + ".log";
            m_sChatLogPath = m_sSessionLogDirectory + "Chat_" + instanceType + "_" + timestamp + ".log";
            m_sVoteKickLogPath = m_sSessionLogDirectory + "VoteKick_" + instanceType + "_" + timestamp + ".log";
            m_sPerformanceLogPath = m_sSessionLogDirectory + "Performance_" + instanceType + "_" + timestamp + ".log";
            
            // Open log files
            if (m_bLogToFile) {
                OpenLogFiles();
            }
            
            // Initialize error tracking
            InitializeErrorTracking();
            
            // Set up file rotation monitoring
            GetGame().GetCallqueue().CallLater(CheckLogRotation, ROTATION_CHECK_INTERVAL, true);
            
            // Enhanced logging of initialization
            LogInfo("Logging system initialized", "STS_LoggingSystem", "Constructor", {
                "session_id": m_sSessionID,
                "is_server": m_bIsServer,
                "log_directory": m_sSessionLogDirectory,
                "console_level": m_eConsoleLogLevel,
                "file_level": m_eFileLogLevel
            });
        }
        catch (Exception e) {
            // Enhanced error handling with context
            string errorContext = string.Format("Failed to initialize logging system: %1\nStack trace: %2", 
                e.ToString(), e.StackTrace);
            Print("[StatTracker] CRITICAL ERROR: " + errorContext, LogLevel.ERROR);
            m_bLogToFile = false;
            
            // Attempt to log to a fallback location
            LogToFallbackLocation(errorContext);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Create log directories recursively
    protected bool CreateLogDirectories()
    {
        try
        {
            string directory = m_sSessionLogDirectory;
            
            // Remove profile: prefix for directory creation
            if (directory.IndexOf("$profile:") == 0)
            {
                directory = directory.Substring(9);
            }
            
            array<string> parts = new array<string>();
            directory.Split("/", parts);
            
            string currentPath = "$profile:";
            foreach (string part : parts)
            {
                if (part == "")
                    continue;
                    
                currentPath += "/" + part;
                if (!FileIO.FileExists(currentPath))
                {
                    if (!FileIO.MakeDirectory(currentPath))
                    {
                        Print("[StatTracker] WARNING: Failed to create directory: " + currentPath);
                        return false;
                    }
                }
            }
            
            return true;
        }
        catch (Exception e)
        {
            Print("[StatTracker] ERROR: Exception creating log directories: " + e.ToString(), LogLevel.ERROR);
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a unique session ID
    protected string GenerateSessionID()
    {
        string dateTime = GetTimestamp(true);
        int randomNum = Math.RandomInt(1000, 9999);
        return "Session_" + dateTime + "_" + randomNum;
    }
    
    //------------------------------------------------------------------------------------------------
    // Open all log files
    protected void OpenLogFiles()
    {
        try
        {
            // Open level-specific log files
            m_DebugLogFile = SafeOpenFile(m_sDebugLogPath);
            m_InfoLogFile = SafeOpenFile(m_sInfoLogPath);
            m_WarningLogFile = SafeOpenFile(m_sWarningLogPath);
            m_ErrorLogFile = SafeOpenFile(m_sErrorLogPath);
            m_CriticalLogFile = SafeOpenFile(m_sCriticalLogPath);
            
            // Open special log files
            m_ChatLogFile = SafeOpenFile(m_sChatLogPath);
            m_VoteKickLogFile = SafeOpenFile(m_sVoteKickLogPath);
            m_PerformanceLogFile = SafeOpenFile(m_sPerformanceLogPath);
            
            // Store file handles in map for easier access
            m_mLogFiles.Insert("debug", m_DebugLogFile);
            m_mLogFiles.Insert("info", m_InfoLogFile);
            m_mLogFiles.Insert("warning", m_WarningLogFile);
            m_mLogFiles.Insert("error", m_ErrorLogFile);
            m_mLogFiles.Insert("critical", m_CriticalLogFile);
            m_mLogFiles.Insert("chat", m_ChatLogFile);
            m_mLogFiles.Insert("votekick", m_VoteKickLogFile);
            m_mLogFiles.Insert("performance", m_PerformanceLogFile);
            
            // Add headers to all files
            foreach (string logType, FileHandle fileHandle : m_mLogFiles)
            {
                if (fileHandle)
                {
                    LogToFile(fileHandle, GetLogHeader("SYSTEM", "Logging started for type: " + logType));
                }
            }
            
            // Add header row for performance metrics
            if (m_PerformanceLogFile)
            {
                LogToFile(m_PerformanceLogFile, "Timestamp,FPS,MemoryUsage,NetworkBandwidth,ObjectCount,PlayerCount");
            }
        }
        catch (Exception e)
        {
            Print("[StatTracker] CRITICAL ERROR: Exception opening log files: " + e.ToString(), LogLevel.ERROR);
            m_bLogToFile = false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Safely open a file handle with error handling
    protected FileHandle SafeOpenFile(string filePath)
    {
        FileHandle fileHandle = FileIO.OpenFile(filePath, FileMode.WRITE);
        if (!fileHandle)
        {
            Print("[StatTracker] ERROR: Failed to open log file: " + filePath, LogLevel.ERROR);
        }
        return fileHandle;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if game is shutting down, and close logs if needed
    protected void CheckShutdown()
    {
        if (!GetGame())
        {
            Print("[StatTracker] Game reference lost, closing logs");
            CloseAllLogs();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Close all open log files
    protected void CloseAllLogs()
    {
        try
        {
            string shutdownMessage = GetLogHeader("SYSTEM", string.Format("Logging ended. Logged %1 messages (%2 errors)", m_iLoggedMessages, m_iErrorCount));
            
            // Close all log files in the map
            foreach (string logType, FileHandle fileHandle : m_mLogFiles)
            {
                if (fileHandle)
                {
                    try
                    {
                        LogToFile(fileHandle, shutdownMessage);
                        FileIO.CloseFile(fileHandle);
                    }
                    catch (Exception e)
                    {
                        Print("[StatTracker] ERROR: Exception closing log file " + logType + ": " + e.ToString(), LogLevel.ERROR);
                    }
                }
            }
            
            // Clear the map
            m_mLogFiles.Clear();
            
            // Set all file handles to null
            m_DebugLogFile = null;
            m_InfoLogFile = null;
            m_WarningLogFile = null;
            m_ErrorLogFile = null;
            m_CriticalLogFile = null;
            m_ChatLogFile = null;
            m_VoteKickLogFile = null;
            m_PerformanceLogFile = null;
        }
        catch (Exception e)
        {
            Print("[StatTracker] CRITICAL ERROR: Exception closing log files: " + e.ToString(), LogLevel.ERROR);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Log at DEBUG level
    void LogDebug(string message, string className = "", string methodName = "")
    {
        if (m_eFileLogLevel > ELogLevel.DEBUG)
            return;
        
        LogInternal(ELogLevel.DEBUG, FormatMessage(message, className, methodName));
    }
    
    //------------------------------------------------------------------------------------------------
    // Log at INFO level
    void LogInfo(string message, string className = "", string methodName = "")
    {
        if (m_eFileLogLevel > ELogLevel.INFO)
            return;
        
        LogInternal(ELogLevel.INFO, FormatMessage(message, className, methodName));
    }
    
    //------------------------------------------------------------------------------------------------
    // Log at WARNING level
    void LogWarning(string message, string className = "", string methodName = "")
    {
        if (m_eFileLogLevel > ELogLevel.WARNING)
            return;
        
        LogInternal(ELogLevel.WARNING, FormatMessage(message, className, methodName));
    }
    
    //------------------------------------------------------------------------------------------------
    // Log at ERROR level
    void LogError(string message, string className = "", string methodName = "", string stackTrace = "")
    {
        if (m_eFileLogLevel > ELogLevel.ERROR)
            return;
        
        m_iErrorCount++;
        LogInternal(ELogLevel.ERROR, FormatMessage(message, className, methodName), stackTrace);
    }
    
    //------------------------------------------------------------------------------------------------
    // Log at CRITICAL level
    void LogCritical(string message, string className = "", string methodName = "", string stackTrace = "")
    {
        // Critical logs are always logged regardless of log level
        LogInternal(ELogLevel.CRITICAL, FormatMessage(message, className, methodName), stackTrace);
    }
    
    //------------------------------------------------------------------------------------------------
    // Internal logging method
    protected void LogInternal(ELogLevel level, string message, string stackTrace = "")
    {
        try
        {
            // Don't log if level is below minimum console log level and we're not logging to file
            if (level < m_eConsoleLogLevel && !m_bLogToFile)
                return;
                
            // Don't log if level is below minimum file log level and console logging is disabled
            if (level < m_eFileLogLevel && !m_bLogToConsole)
                return;
            
            // Increment stats counters
            m_iLoggedMessages++;
            if (level == ELogLevel.ERROR || level == ELogLevel.CRITICAL)
                m_iErrorCount++;
            
            // Get current timestamp
            string timestamp = GetTimestamp();
            
            // Format log entry
            string logEntry = string.Format("[%1] [%2] %3", timestamp, GetLogLevelString(level), message);
            
            // Add stack trace for errors if enabled
            if ((level == ELogLevel.ERROR || level == ELogLevel.CRITICAL) && m_bAddStackTraceToErrors)
            {
                if (stackTrace != "")
                {
                    logEntry += "\n  StackTrace: " + stackTrace;
                }
            }
            
            // Add to memory buffer (circular)
            m_aMemoryLogs[m_iMemoryLogIndex] = logEntry;
            m_iMemoryLogIndex = (m_iMemoryLogIndex + 1) % MAX_MEMORY_LOGS;
            
            // Output to console if enabled and level is sufficient
            if (m_bLogToConsole && level >= m_eConsoleLogLevel)
            {
                switch (level)
                {
                    case ELogLevel.DEBUG:
                        Print(logEntry, LogLevel.NORMAL);
                        break;
                    
                    case ELogLevel.INFO:
                        Print(logEntry, LogLevel.NORMAL);
                        break;
                    
                    case ELogLevel.WARNING:
                        Print(logEntry, LogLevel.WARNING);
                        break;
                    
                    case ELogLevel.ERROR:
                    case ELogLevel.CRITICAL:
                        Print(logEntry, LogLevel.ERROR);
                        break;
                }
            }
            
            // Write to appropriate log file based on level
            if (m_bLogToFile && level >= m_eFileLogLevel)
            {
                switch (level)
                {
                    case ELogLevel.DEBUG:
                        LogToLevelFile("debug", logEntry);
                        break;
                    
                    case ELogLevel.INFO:
                        LogToLevelFile("info", logEntry);
                        break;
                    
                    case ELogLevel.WARNING:
                        LogToLevelFile("warning", logEntry);
                        break;
                    
                    case ELogLevel.ERROR:
                        LogToLevelFile("error", logEntry);
                        break;
                    
                    case ELogLevel.CRITICAL:
                        LogToLevelFile("critical", logEntry);
                        break;
                }
            }
            
            // Check if we need to rotate log files (only check periodically)
            float currentTime = System.GetTickCount() / 1000.0;
            if (currentTime - m_fLastRotationCheck > 60.0) // Check every minute
            {
                m_fLastRotationCheck = currentTime;
                CheckAllLogFilesRotation();
            }
        }
        catch (Exception e)
        {
            // Last resort error handling - don't use our own logging here to prevent recursion
            Print("[StatTracker] CRITICAL ERROR in logging system: " + e.ToString(), LogLevel.ERROR);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Format a log message with context information
    protected string FormatMessage(string message, string className, string methodName)
    {
        if (className != "" && methodName != "")
        {
            return string.Format("%1::%2() - %3", className, methodName, message);
        }
        else if (className != "")
        {
            return string.Format("%1 - %2", className, message);
        }
        else
        {
            return message;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Log to a specific level file
    protected void LogToLevelFile(string level, string message)
    {
        FileHandle fileHandle = m_mLogFiles.Get(level);
        if (fileHandle)
        {
            LogToFile(fileHandle, message);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Log message to a file
    protected void LogToFile(FileHandle fileHandle, string message)
    {
        if (!fileHandle)
            return;
            
        try
        {
            FileIO.FPrintln(fileHandle, message);
        }
        catch (Exception e)
        {
            m_iFileWriteErrors++;
            Print("[StatTracker] ERROR: Failed to write to log file: " + e.ToString(), LogLevel.ERROR);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Check all log files for rotation
    protected void CheckAllLogFilesRotation()
    {
        foreach (string logType, FileHandle fileHandle : m_mLogFiles)
        {
            if (fileHandle)
            {
                CheckLogFileRotation(logType);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if a specific log file needs rotation
    protected void CheckLogFileRotation(string logType)
    {
        try
        {
            FileHandle fileHandle = m_mLogFiles.Get(logType);
            if (!fileHandle)
                return;
                
            string filePath = GetLogFilePath(logType);
            if (filePath.IsEmpty())
                return;
                
            int fileSize = GetFileSize(filePath);
            int maxSizeBytes = m_iMaxLogSizeMB * 1024 * 1024;
            
            if (fileSize > maxSizeBytes)
            {
                Print("[StatTracker] Rotating log file: " + logType + " (size: " + fileSize + " bytes)");
                RotateLogFile(logType);
            }
        }
        catch (Exception e)
        {
            Print("[StatTracker] ERROR: Exception checking log file rotation: " + e.ToString(), LogLevel.ERROR);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the file path for a specific log type
    protected string GetLogFilePath(string logType)
    {
        switch (logType)
        {
            case "debug": return m_sDebugLogPath;
            case "info": return m_sInfoLogPath;
            case "warning": return m_sWarningLogPath;
            case "error": return m_sErrorLogPath;
            case "critical": return m_sCriticalLogPath;
            case "chat": return m_sChatLogPath;
            case "votekick": return m_sVoteKickLogPath;
            case "performance": return m_sPerformanceLogPath;
            default: return "";
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Rotate a specific log file
    protected void RotateLogFile(string logType)
    {
        try
        {
            // Close current file
            FileHandle currentFile = m_mLogFiles.Get(logType);
            if (currentFile)
            {
                FileIO.CloseFile(currentFile);
            }
            
            // Get file path
            string filePath = GetLogFilePath(logType);
            if (filePath.IsEmpty())
                return;
                
            // Generate new file name with timestamp
            string timestamp = GetTimestamp(true);
            string newFilePath = filePath + "." + timestamp;
            
            // Rename current file
            if (!FileIO.MoveFile(filePath, newFilePath))
            {
                Print("[StatTracker] ERROR: Failed to rotate log file: " + filePath, LogLevel.ERROR);
                
                // Try to reopen the original file
                m_mLogFiles.Set(logType, SafeOpenFile(filePath));
                return;
            }
            
            // Open a new file with the original name
            FileHandle newFile = SafeOpenFile(filePath);
            m_mLogFiles.Set(logType, newFile);
            
            // Log rotation event
            if (newFile)
            {
                LogToFile(newFile, GetLogHeader("SYSTEM", "Log file rotated from " + filePath + " to " + newFilePath));
            }
            
            // Clean up old rotated files if we have too many
            CleanupOldRotatedFiles(logType);
        }
        catch (Exception e)
        {
            Print("[StatTracker] ERROR: Exception rotating log file: " + e.ToString(), LogLevel.ERROR);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up old rotated log files
    protected void CleanupOldRotatedFiles(string logType)
    {
        try
        {
            string filePath = GetLogFilePath(logType);
            if (filePath.IsEmpty())
                return;
                
            // Get a list of all rotated files
            array<string> allFiles = new array<string>();
            FileIO.FindFiles(allFiles, filePath + ".*");
            
            // If we have more files than the maximum, delete the oldest ones
            if (allFiles.Count() > m_iMaxLogFiles)
            {
                // Sort files by name (which includes timestamp)
                allFiles.Sort();
                
                // Delete the oldest files (at the beginning of the sorted array)
                int filesToDelete = allFiles.Count() - m_iMaxLogFiles;
                for (int i = 0; i < filesToDelete; i++)
                {
                    if (!FileIO.DeleteFile(allFiles[i]))
                    {
                        Print("[StatTracker] WARNING: Failed to delete old log file: " + allFiles[i], LogLevel.WARNING);
                    }
                }
            }
        }
        catch (Exception e)
        {
            Print("[StatTracker] ERROR: Exception cleaning up old log files: " + e.ToString(), LogLevel.ERROR);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get file size in bytes
    protected int GetFileSize(string filePath)
    {
        try
        {
            FileHandle file = FileIO.OpenFile(filePath, FileMode.READ);
            if (!file)
                return 0;
                
            // Get file size by seeking to the end
            FileIO.FSeek(file, 0, FileSeek.END);
            int size = FileIO.FTell(file);
            
            // Close the file
            FileIO.CloseFile(file);
            
            return size;
        }
        catch (Exception e)
        {
            Print("[StatTracker] ERROR: Exception getting file size: " + e.ToString(), LogLevel.ERROR);
            return 0;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get formatted date for filename
    protected string GetDateForFilename()
    {
        return GetCurrentDateFormatted("yyyy-MM-dd");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get formatted log header
    protected string GetLogHeader(string category, string message)
    {
        string instanceType = m_bIsServer ? "SERVER" : "CLIENT";
        return string.Format("[%1] [%2] [%3] %4", GetTimestamp(), instanceType, category, message);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current timestamp
    protected string GetTimestamp(bool forFilename = false)
    {
        try
        {
            int year, month, day, hour, minute, second;
            System.GetYearMonthDay(year, month, day);
            System.GetHourMinuteSecond(hour, minute, second);
            
            if (forFilename)
            {
                return string.Format("%1-%2-%3_%4-%5-%6", 
                    year, month.ToString(2, "0"), day.ToString(2, "0"), 
                    hour.ToString(2, "0"), minute.ToString(2, "0"), second.ToString(2, "0"));
            }
            else
            {
                return string.Format("%1-%2-%3 %4:%5:%6", 
                    year, month.ToString(2, "0"), day.ToString(2, "0"), 
                    hour.ToString(2, "0"), minute.ToString(2, "0"), second.ToString(2, "0"));
            }
        }
        catch (Exception e)
        {
            Print("[StatTracker] ERROR: Exception in GetTimestamp: " + e.ToString());
            return "Error-Timestamp";
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Change log levels
    void SetConsoleLogLevel(ELogLevel level)
    {
        m_eConsoleLogLevel = level;
        LogInfo("Console log level changed to " + level, "STS_LoggingSystem", "SetConsoleLogLevel");
    }
    
    void SetFileLogLevel(ELogLevel level)
    {
        m_eFileLogLevel = level;
        LogInfo("File log level changed to " + level, "STS_LoggingSystem", "SetFileLogLevel");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get logging statistics
    void GetLoggingStats(out int loggedMessages, out int errorCount, out int fileWriteErrors)
    {
        loggedMessages = m_iLoggedMessages;
        errorCount = m_iErrorCount;
        fileWriteErrors = m_iFileWriteErrors;
    }
    
    //------------------------------------------------------------------------------------------------
    // Log a chat message
    void LogChat(string playerName, string playerID, string message)
    {
        try
        {
            if (!m_bLogToFile)
                return;
                
            string timestamp = GetTimestamp();
            string chatEntry = string.Format("[%1] [%2] [%3]: %4", timestamp, playerID, playerName, message);
            
            // Log to chat-specific file
            if (m_ChatLogFile)
            {
                LogToFile(m_ChatLogFile, chatEntry);
            }
            
            // Also log to debug file
            LogDebug(string.Format("CHAT: [%1] [%2]: %3", playerID, playerName, message));
        }
        catch (Exception e)
        {
            LogError("Exception logging chat: " + e.ToString(), "STS_LoggingSystem", "LogChat", e.GetStackTrace());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Log a vote kick
    void LogVoteKick(string initiatorName, string initiatorID, string targetName, string targetID, string reason)
    {
        try
        {
            if (!m_bLogToFile)
                return;
                
            string timestamp = GetTimestamp();
            string voteKickEntry = string.Format("[%1] VOTE KICK: Initiator [%2] %3 against Target [%4] %5, Reason: %6", 
                timestamp, initiatorID, initiatorName, targetID, targetName, reason);
            
            // Log to vote kick-specific file
            if (m_VoteKickLogFile)
            {
                LogToFile(m_VoteKickLogFile, voteKickEntry);
            }
            
            // Also log to info file
            LogInfo(string.Format("VOTE KICK: %1 (%2) initiated against %3 (%4), Reason: %5", 
                initiatorName, initiatorID, targetName, targetID, reason));
        }
        catch (Exception e)
        {
            LogError("Exception logging vote kick: " + e.ToString(), "STS_LoggingSystem", "LogVoteKick", e.GetStackTrace());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Log performance data
    void LogPerformance(float fps, float memoryUsageMB, float networkBandwidthKBps, int objectCount, int playerCount)
    {
        try
        {
            if (!m_bLogToFile)
                return;
                
            string timestamp = GetTimestamp();
            string perfEntry = string.Format("%1,%2,%.2f,%.2f,%3,%4", 
                timestamp, fps.ToString(), memoryUsageMB, networkBandwidthKBps, objectCount, playerCount);
            
            // Log to performance-specific file
            if (m_PerformanceLogFile)
            {
                LogToFile(m_PerformanceLogFile, perfEntry);
            }
            
            // Also log to debug file if performance is problematic
            if (fps < 20 || memoryUsageMB > 2000)
            {
                LogWarning(string.Format("PERFORMANCE ISSUE: FPS=%1, Memory=%.2fMB, Network=%.2fKB/s, Objects=%3, Players=%4", 
                    fps.ToString(), memoryUsageMB, networkBandwidthKBps, objectCount, playerCount));
            }
        }
        catch (Exception e)
        {
            LogError("Exception logging performance: " + e.ToString(), "STS_LoggingSystem", "LogPerformance", e.GetStackTrace());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the session ID
    string GetSessionID()
    {
        return m_sSessionID;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the session log directory
    string GetSessionLogDirectory()
    {
        return m_sSessionLogDirectory;
    }
    
    // Enhanced log method with rich context
    void Log(ELogLevel level, string message, string category, string source, string function, map<string, string> context = null)
    {
        try {
            if (!ShouldLog(level)) return;
            
            string timestamp = GetTimestamp(true);
            string logEntry = string.Format("[%1] [%2] [%3] [%4::%5] %6", 
                timestamp, GetLogLevelString(level), category, source, function, message);
            
            // Add context if provided
            if (context) {
                logEntry += "\nContext:";
                foreach (string key, string value : context) {
                    logEntry += string.Format("\n  %1: %2", key, value);
                }
            }
            
            // Add stack trace for errors and critical messages
            if (level >= ELogLevel.ERROR && m_bAddStackTraceToErrors) {
                logEntry += "\nStack trace:\n" + GetStackTrace();
            }
            
            // Log to appropriate destinations
            if (m_bLogToConsole) LogToConsole(logEntry, level);
            if (m_bLogToFile) LogToFile(logEntry, level);
            
            // Update error tracking
            if (level >= ELogLevel.ERROR) {
                UpdateErrorTracking(category, logEntry);
            }
            
            // Update memory buffer
            UpdateMemoryBuffer(logEntry);
        }
        catch (Exception e) {
            // Handle logging failures gracefully
            HandleLoggingFailure(e, level, message);
        }
    }
    
    // Enhanced file rotation with better error handling
    protected void CheckLogRotation()
    {
        try {
            if (!m_bLogToFile) return;
            
            array<string> logFiles = {
                m_sDebugLogPath, m_sInfoLogPath, m_sWarningLogPath,
                m_sErrorLogPath, m_sCriticalLogPath, m_sChatLogPath,
                m_sVoteKickLogPath, m_sPerformanceLogPath
            };
            
            foreach (string logFile : logFiles) {
                if (FileExist(logFile)) {
                    int fileSize = GetFileSize(logFile);
                    if (fileSize > MAX_LOG_SIZE_MB * 1024 * 1024) {
                        RotateLogFile(logFile);
                    }
                }
            }
        }
        catch (Exception e) {
            LogError("Failed to check log rotation", "STS_LoggingSystem", "CheckLogRotation", {
                "error": e.ToString(),
                "stack_trace": e.StackTrace
            });
        }
    }
    
    // Enhanced error tracking
    protected void UpdateErrorTracking(string category, string errorMessage)
    {
        if (!m_mErrorCounts.Contains(category)) {
            m_mErrorCounts.Insert(category, 0);
            m_mErrorContexts.Insert(category, new array<string>);
        }
        
        m_mErrorCounts[category]++;
        
        // Keep only the most recent error contexts
        array<string> contexts = m_mErrorContexts[category];
        contexts.InsertAt(errorMessage, 0);
        if (contexts.Count() > MAX_ERROR_CONTEXTS) {
            contexts.RemoveOrdered(MAX_ERROR_CONTEXTS);
        }
        
        // Log error statistics periodically
        if (m_mErrorCounts[category] % 100 == 0) {
            LogWarning(string.Format("Error statistics for category '%1': %2 errors", 
                category, m_mErrorCounts[category]), "STS_LoggingSystem", "UpdateErrorTracking");
        }
    }
    
    // Enhanced error handling for logging failures
    protected void HandleLoggingFailure(Exception e, ELogLevel level, string originalMessage)
    {
        string errorContext = string.Format("Logging failure while trying to log: [%1] %2\nError: %3\nStack trace: %4",
            GetLogLevelString(level), originalMessage, e.ToString(), e.StackTrace);
            
        // Try to log to fallback location
        LogToFallbackLocation(errorContext);
        
        // Update error tracking
        UpdateErrorTracking("LOGGING_SYSTEM", errorContext);
    }
} 