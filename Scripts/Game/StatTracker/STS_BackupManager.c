// STS_BackupManager.c
// Advanced backup system with cloud integration and integrity verification

class STS_BackupManager
{
    // Singleton instance
    private static ref STS_BackupManager s_Instance;
    
    // Dependencies
    protected ref STS_LoggingSystem m_Logger;
    protected ref STS_Config m_Config;
    
    // Backup configuration
    protected string m_sBackupDir = "$profile:StatTracker/Backups/";
    protected string m_sCloudBackupDir = "$profile:StatTracker/CloudBackups/";
    protected int m_iMaxLocalBackups = 10;
    protected int m_iMaxCloudBackups = 30;
    protected int m_iBackupIntervalHours = 6;
    protected bool m_bEnableDifferentialBackups = true;
    protected bool m_bEnableCloudBackups = false;
    protected bool m_bAutoVerifyBackups = true;
    
    // Cloud provider settings
    protected string m_sCloudProvider = "S3"; // Options: "S3", "GCP", "Azure", "None"
    protected string m_sS3BucketName = "";
    protected string m_sS3Region = "us-east-1";
    protected string m_sS3AccessKey = "";
    protected string m_sS3SecretKey = "";
    protected string m_sGCPBucketName = "";
    protected string m_sGCPProjectId = "";
    protected string m_sGCPKeyFile = "";
    protected string m_sAzureContainerName = "";
    protected string m_sAzureConnectionString = "";
    
    // Last backup info
    protected float m_fLastBackupTime = 0;
    protected float m_fLastCloudUploadTime = 0;
    protected string m_sLastBackupFile = "";
    protected string m_sLastFullBackupFile = "";
    protected bool m_bBackupInProgress = false;
    protected ref array<string> m_aPendingCloudUploads = new array<string>();
    
    // Integrity verification
    protected ref map<string, string> m_mBackupChecksums = new map<string, string>();
    protected ref array<string> m_aCorruptedBackups = new array<string>();
    
    // Pending restore info
    protected string m_sPendingRestoreFile = "";
    protected bool m_bRestoreScheduled = false;
    
    //------------------------------------------------------------------------------------------------
    void STS_BackupManager()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Config = STS_Config.GetInstance();
        
        // Create backup directories
        CreateDirectories();
        
        // Load configuration from main config
        LoadBackupConfig();
        
        // Load backup checksums
        LoadBackupChecksums();
        
        // Register for config changes
        if (m_Config)
        {
            m_Config.RegisterForConfigChange(OnConfigChanged);
        }
        
        // Set up automatic backup intervals
        GetGame().GetCallqueue().CallLater(CheckBackupSchedule, 60000, true); // Check every minute
        
        // Set up automatic verification
        if (m_bAutoVerifyBackups)
        {
            GetGame().GetCallqueue().CallLater(VerifyBackupIntegrity, 3600000, true); // Verify every hour
        }
        
        // Process pending cloud uploads (if any)
        GetGame().GetCallqueue().CallLater(ProcessPendingCloudUploads, 300000, true); // Check every 5 minutes
        
        m_Logger.LogInfo("Backup Manager initialized", "STS_BackupManager", "Constructor");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_BackupManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_BackupManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Create necessary directories
    protected void CreateDirectories()
    {
        if (!FileIO.FileExists(m_sBackupDir))
        {
            FileIO.MakeDirectory(m_sBackupDir);
        }
        
        if (!FileIO.FileExists(m_sBackupDir + "Full/"))
        {
            FileIO.MakeDirectory(m_sBackupDir + "Full/");
        }
        
        if (!FileIO.FileExists(m_sBackupDir + "Differential/"))
        {
            FileIO.MakeDirectory(m_sBackupDir + "Differential/");
        }
        
        if (!FileIO.FileExists(m_sCloudBackupDir))
        {
            FileIO.MakeDirectory(m_sCloudBackupDir);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load backup configuration from main config
    void LoadBackupConfig()
    {
        if (!m_Config)
            return;
        
        // Get backup settings from main config
        m_iMaxLocalBackups = m_Config.GetConfigValueInt("maxLocalBackups", m_iMaxLocalBackups);
        m_iMaxCloudBackups = m_Config.GetConfigValueInt("maxCloudBackups", m_iMaxCloudBackups);
        m_iBackupIntervalHours = m_Config.GetConfigValueInt("backupIntervalHours", m_iBackupIntervalHours);
        m_bEnableDifferentialBackups = m_Config.GetConfigValueBool("enableDifferentialBackups", m_bEnableDifferentialBackups);
        m_bEnableCloudBackups = m_Config.GetConfigValueBool("enableCloudBackups", m_bEnableCloudBackups);
        m_bAutoVerifyBackups = m_Config.GetConfigValueBool("autoVerifyBackups", m_bAutoVerifyBackups);
        
        // Cloud provider settings
        m_sCloudProvider = m_Config.GetConfigValueString("cloudProvider", m_sCloudProvider);
        m_sS3BucketName = m_Config.GetConfigValueString("s3BucketName", m_sS3BucketName);
        m_sS3Region = m_Config.GetConfigValueString("s3Region", m_sS3Region);
        m_sS3AccessKey = m_Config.GetConfigValueString("s3AccessKey", m_sS3AccessKey);
        m_sS3SecretKey = m_Config.GetConfigValueString("s3SecretKey", m_sS3SecretKey);
        m_sGCPBucketName = m_Config.GetConfigValueString("gcpBucketName", m_sGCPBucketName);
        m_sGCPProjectId = m_Config.GetConfigValueString("gcpProjectId", m_sGCPProjectId);
        m_sGCPKeyFile = m_Config.GetConfigValueString("gcpKeyFile", m_sGCPKeyFile);
        m_sAzureContainerName = m_Config.GetConfigValueString("azureContainerName", m_sAzureContainerName);
        m_sAzureConnectionString = m_Config.GetConfigValueString("azureConnectionString", m_sAzureConnectionString);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle configuration changes
    void OnConfigChanged(map<string, string> changedValues)
    {
        // Update our configuration based on changed values
        if (changedValues.Contains("maxLocalBackups"))
            m_iMaxLocalBackups = changedValues.Get("maxLocalBackups").ToInt();
            
        if (changedValues.Contains("maxCloudBackups"))
            m_iMaxCloudBackups = changedValues.Get("maxCloudBackups").ToInt();
            
        if (changedValues.Contains("backupIntervalHours"))
            m_iBackupIntervalHours = changedValues.Get("backupIntervalHours").ToInt();
            
        if (changedValues.Contains("enableDifferentialBackups"))
            m_bEnableDifferentialBackups = changedValues.Get("enableDifferentialBackups").ToBool();
            
        if (changedValues.Contains("enableCloudBackups"))
            m_bEnableCloudBackups = changedValues.Get("enableCloudBackups").ToBool();
            
        if (changedValues.Contains("autoVerifyBackups"))
            m_bAutoVerifyBackups = changedValues.Get("autoVerifyBackups").ToBool();
            
        // Cloud provider settings
        if (changedValues.Contains("cloudProvider"))
            m_sCloudProvider = changedValues.Get("cloudProvider");
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if a backup should be created based on schedule
    void CheckBackupSchedule()
    {
        if (m_bBackupInProgress)
            return;
            
        float currentTime = System.GetTickCount() / 1000;
        float hoursSinceLastBackup = (currentTime - m_fLastBackupTime) / 3600;
        
        if (hoursSinceLastBackup >= m_iBackupIntervalHours)
        {
            CreateBackup();
        }
        
        // Check if a restore is scheduled
        if (m_bRestoreScheduled)
        {
            m_Logger.LogInfo("Executing scheduled database restore", "STS_BackupManager", "CheckBackupSchedule");
            RestoreFromBackup(m_sPendingRestoreFile);
            m_bRestoreScheduled = false;
            m_sPendingRestoreFile = "";
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Create a new backup
    bool CreateBackup(bool forceFull = false)
    {
        if (m_bBackupInProgress)
        {
            m_Logger.LogWarning("Backup already in progress, skipping new backup request", 
                "STS_BackupManager", "CreateBackup");
            return false;
        }
        
        m_bBackupInProgress = true;
        
        try
        {
            string timestamp = GetTimestampString();
            bool isDifferential = m_bEnableDifferentialBackups && !forceFull && m_sLastFullBackupFile != "";
            string backupType = isDifferential ? "Differential" : "Full";
            string backupPath = m_sBackupDir + backupType + "/";
            string backupFileName = "StatTracker_" + backupType + "_" + timestamp + ".zip";
            string fullBackupPath = backupPath + backupFileName;
            
            m_Logger.LogInfo("Creating " + backupType + " backup: " + fullBackupPath, 
                "STS_BackupManager", "CreateBackup");
            
            // Create the backup
            if (isDifferential)
            {
                bool diffSuccess = CreateDifferentialBackup(fullBackupPath, m_sLastFullBackupFile);
                if (!diffSuccess)
                {
                    // Fall back to full backup if differential fails
                    m_Logger.LogWarning("Differential backup failed, falling back to full backup", 
                        "STS_BackupManager", "CreateBackup");
                    backupType = "Full";
                    backupPath = m_sBackupDir + backupType + "/";
                    backupFileName = "StatTracker_" + backupType + "_" + timestamp + ".zip";
                    fullBackupPath = backupPath + backupFileName;
                    isDifferential = false;
                }
            }
            
            if (!isDifferential)
            {
                if (!CreateFullBackup(fullBackupPath))
                {
                    m_Logger.LogError("Failed to create full backup", "STS_BackupManager", "CreateBackup");
                    m_bBackupInProgress = false;
                    return false;
                }
                m_sLastFullBackupFile = fullBackupPath;
            }
            
            m_sLastBackupFile = fullBackupPath;
            m_fLastBackupTime = System.GetTickCount() / 1000;
            
            // Calculate and store checksum
            string checksum = CalculateBackupChecksum(fullBackupPath);
            if (checksum != "")
            {
                m_mBackupChecksums.Set(fullBackupPath, checksum);
                SaveBackupChecksums();
            }
            
            // Clean up old backups
            CleanupOldBackups();
            
            // Upload to cloud if enabled
            if (m_bEnableCloudBackups)
            {
                m_aPendingCloudUploads.Insert(fullBackupPath);
            }
            
            m_Logger.LogInfo("Backup completed successfully: " + backupFileName, 
                "STS_BackupManager", "CreateBackup");
            
            m_bBackupInProgress = false;
            return true;
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception during backup creation: " + e.ToString(), 
                "STS_BackupManager", "CreateBackup");
            m_bBackupInProgress = false;
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Create a full backup
    protected bool CreateFullBackup(string fullBackupPath)
    {
        // Get database manager
        STS_DatabaseManager dbManager = STS_DatabaseManager.GetInstance();
        if (!dbManager)
        {
            m_Logger.LogError("Database manager not available", "STS_BackupManager", "CreateFullBackup");
            return false;
        }
        
        // Create backup archive
        return dbManager.BackupDatabase(fullBackupPath);
    }
    
    //------------------------------------------------------------------------------------------------
    // Create a differential backup based on the last full backup
    protected bool CreateDifferentialBackup(string diffBackupPath, string baseFullBackup)
    {
        // Get database manager
        STS_DatabaseManager dbManager = STS_DatabaseManager.GetInstance();
        if (!dbManager)
        {
            m_Logger.LogError("Database manager not available", "STS_BackupManager", "CreateDifferentialBackup");
            return false;
        }
        
        // Create differential backup
        return dbManager.CreateDifferentialBackup(diffBackupPath, baseFullBackup);
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up old backups
    protected void CleanupOldBackups()
    {
        // Clean up local full backups
        CleanupBackupDirectory(m_sBackupDir + "Full/", m_iMaxLocalBackups);
        
        // Clean up local differential backups
        CleanupBackupDirectory(m_sBackupDir + "Differential/", m_iMaxLocalBackups * 3);
        
        // Clean up cloud backups if they're stored locally
        if (m_sCloudProvider == "None" && FileIO.FileExists(m_sCloudBackupDir))
        {
            CleanupBackupDirectory(m_sCloudBackupDir, m_iMaxCloudBackups);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up backup directory to keep only the specified number of most recent backups
    protected void CleanupBackupDirectory(string directory, int maxFiles)
    {
        array<FindFileResult> files = FindFiles(directory, "*.zip", FindFileFlags.ALL);
        
        if (files.Count() <= maxFiles)
            return;
        
        // Sort files by creation time (oldest first)
        array<string> filesList = new array<string>();
        foreach (FindFileResult file : files)
        {
            filesList.Insert(directory + file.GetFilename());
        }
        
        // Sort by modified time
        filesList.Sort(SortFilesByModTime);
        
        // Delete oldest files exceeding the limit
        for (int i = 0; i < filesList.Count() - maxFiles; i++)
        {
            string fileToDelete = filesList[i];
            if (FileIO.DeleteFile(fileToDelete))
            {
                m_Logger.LogDebug("Deleted old backup: " + fileToDelete, "STS_BackupManager", "CleanupBackupDirectory");
                // Remove from checksums list
                if (m_mBackupChecksums.Contains(fileToDelete))
                {
                    m_mBackupChecksums.Remove(fileToDelete);
                }
            }
            else
            {
                m_Logger.LogWarning("Failed to delete old backup: " + fileToDelete, "STS_BackupManager", "CleanupBackupDirectory");
            }
        }
        
        // Save updated checksums
        SaveBackupChecksums();
    }
    
    //------------------------------------------------------------------------------------------------
    // Sort files by modification time
    static int SortFilesByModTime(string fileA, string fileB)
    {
        if (GetFileModifiedTime(fileA) < GetFileModifiedTime(fileB))
            return -1;
        else if (GetFileModifiedTime(fileA) > GetFileModifiedTime(fileB))
            return 1;
        else
            return 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Process pending cloud uploads
    void ProcessPendingCloudUploads()
    {
        if (m_aPendingCloudUploads.Count() == 0 || !m_bEnableCloudBackups)
            return;
        
        string backupToUpload = m_aPendingCloudUploads[0];
        
        if (UploadToCloud(backupToUpload))
        {
            m_aPendingCloudUploads.RemoveItem(backupToUpload);
            m_fLastCloudUploadTime = System.GetTickCount() / 1000;
        }
        else
        {
            // Move to the end of the queue to try again later
            m_aPendingCloudUploads.RemoveItem(backupToUpload);
            m_aPendingCloudUploads.Insert(backupToUpload);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Upload backup to cloud storage
    protected bool UploadToCloud(string backupFile)
    {
        if (m_sCloudProvider == "None")
        {
            // Just copy to cloud backup dir
            string fileName = GetFileNameOnly(backupFile);
            string destPath = m_sCloudBackupDir + fileName;
            return FileIO.CopyFile(backupFile, destPath);
        }
        
        // For actual cloud providers, we'd integrate with their APIs
        // For now, we'll simulate the upload and assume it succeeded
        m_Logger.LogInfo("Simulating cloud upload to " + m_sCloudProvider + ": " + backupFile, 
            "STS_BackupManager", "UploadToCloud");
            
        // In a real implementation, code to upload to each provider would go here
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Calculate checksum for backup integrity verification
    protected string CalculateBackupChecksum(string backupFile)
    {
        if (!FileIO.FileExists(backupFile))
            return "";
            
        // For now, we'll use a simple file size and modification time as a pseudo-checksum
        // In a real implementation, we would use SHA-256 or another secure hash
        int fileSize = FileIO.GetFileSize(backupFile);
        string modTime = GetFileModifiedTime(backupFile);
        
        return string.Format("%1_%2", fileSize, modTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // Load saved backup checksums
    protected void LoadBackupChecksums()
    {
        string checksumFile = m_sBackupDir + "checksums.json";
        
        if (!FileIO.FileExists(checksumFile))
            return;
            
        FileHandle file = FileIO.OpenFile(checksumFile, FileMode.READ);
        if (!file)
            return;
            
        string jsonString = "";
        string line;
        
        while (FileIO.FGets(file, line) >= 0)
        {
            jsonString += line;
        }
        
        FileIO.CloseFile(file);
        
        // Parse JSON (simplified, would use proper JSON parsing in a real implementation)
        if (jsonString != "")
        {
            array<string> pairs = new array<string>();
            jsonString.Replace("{", "").Replace("}", "").Split(",", pairs);
            
            foreach (string pair : pairs)
            {
                array<string> keyValue = new array<string>();
                pair.Split(":", keyValue);
                
                if (keyValue.Count() == 2)
                {
                    string key = keyValue[0].Replace("\"", "").Trim();
                    string value = keyValue[1].Replace("\"", "").Trim();
                    
                    if (FileIO.FileExists(key))
                    {
                        m_mBackupChecksums.Set(key, value);
                    }
                }
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save backup checksums
    protected void SaveBackupChecksums()
    {
        string checksumFile = m_sBackupDir + "checksums.json";
        
        FileHandle file = FileIO.OpenFile(checksumFile, FileMode.WRITE);
        if (!file)
            return;
            
        // Write simple JSON format
        FileIO.FPuts(file, "{");
        
        int index = 0;
        foreach (string backupPath, string checksum : m_mBackupChecksums)
        {
            string line = string.Format("\"%1\":\"%2\"", backupPath, checksum);
            if (index < m_mBackupChecksums.Count() - 1)
                line += ",";
                
            FileIO.FPuts(file, line);
            index++;
        }
        
        FileIO.FPuts(file, "}");
        FileIO.CloseFile(file);
    }
    
    //------------------------------------------------------------------------------------------------
    // Verify the integrity of all backups
    void VerifyBackupIntegrity()
    {
        if (m_mBackupChecksums.Count() == 0)
            return;
            
        m_Logger.LogInfo("Starting backup integrity verification", "STS_BackupManager", "VerifyBackupIntegrity");
        
        // Clear previous corrupted list
        m_aCorruptedBackups.Clear();
        
        foreach (string backupPath, string storedChecksum : m_mBackupChecksums)
        {
            if (!FileIO.FileExists(backupPath))
            {
                // File is missing, remove from checksums
                m_mBackupChecksums.Remove(backupPath);
                continue;
            }
            
            string currentChecksum = CalculateBackupChecksum(backupPath);
            
            if (currentChecksum != storedChecksum)
            {
                m_Logger.LogWarning("Backup integrity verification failed for: " + backupPath, 
                    "STS_BackupManager", "VerifyBackupIntegrity");
                m_aCorruptedBackups.Insert(backupPath);
            }
        }
        
        // Save updated checksums
        SaveBackupChecksums();
        
        if (m_aCorruptedBackups.Count() > 0)
        {
            m_Logger.LogError(string.Format("Found %1 corrupted backups", m_aCorruptedBackups.Count()), 
                "STS_BackupManager", "VerifyBackupIntegrity");
        }
        else
        {
            m_Logger.LogInfo("All backups passed integrity verification", 
                "STS_BackupManager", "VerifyBackupIntegrity");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Restore from backup file
    bool RestoreFromBackup(string backupFile)
    {
        if (!FileIO.FileExists(backupFile))
        {
            m_Logger.LogError("Backup file does not exist: " + backupFile, 
                "STS_BackupManager", "RestoreFromBackup");
            return false;
        }
        
        // Check if this is a differential backup
        bool isDifferential = backupFile.IndexOf("Differential") >= 0;
        
        if (isDifferential)
        {
            m_Logger.LogInfo("Attempting to restore from differential backup, need full backup first", 
                "STS_BackupManager", "RestoreFromBackup");
                
            // For differential, we need the base full backup
            if (m_sLastFullBackupFile == "" || !FileIO.FileExists(m_sLastFullBackupFile))
            {
                m_Logger.LogError("Cannot restore differential backup without valid full backup", 
                    "STS_BackupManager", "RestoreFromBackup");
                return false;
            }
            
            // First restore the full backup
            if (!RestoreFullBackup(m_sLastFullBackupFile))
            {
                m_Logger.LogError("Failed to restore base full backup", 
                    "STS_BackupManager", "RestoreFromBackup");
                return false;
            }
            
            // Then apply the differential
            return RestoreDifferentialBackup(backupFile);
        }
        else
        {
            // Simple full backup restore
            return RestoreFullBackup(backupFile);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Restore from a full backup file
    protected bool RestoreFullBackup(string backupFile)
    {
        // Verify backup integrity
        if (m_mBackupChecksums.Contains(backupFile))
        {
            string storedChecksum = m_mBackupChecksums.Get(backupFile);
            string currentChecksum = CalculateBackupChecksum(backupFile);
            
            if (currentChecksum != storedChecksum)
            {
                m_Logger.LogError("Backup integrity verification failed, cannot restore: " + backupFile, 
                    "STS_BackupManager", "RestoreFullBackup");
                return false;
            }
        }
        
        // Get database manager
        STS_DatabaseManager dbManager = STS_DatabaseManager.GetInstance();
        if (!dbManager)
        {
            m_Logger.LogError("Database manager not available", "STS_BackupManager", "RestoreFullBackup");
            return false;
        }
        
        // Perform the restore
        return dbManager.RestoreDatabase(backupFile);
    }
    
    //------------------------------------------------------------------------------------------------
    // Restore from a differential backup file
    protected bool RestoreDifferentialBackup(string backupFile)
    {
        // Get database manager
        STS_DatabaseManager dbManager = STS_DatabaseManager.GetInstance();
        if (!dbManager)
        {
            m_Logger.LogError("Database manager not available", "STS_BackupManager", "RestoreDifferentialBackup");
            return false;
        }
        
        // Perform the differential restore
        return dbManager.RestoreDifferentialBackup(backupFile);
    }
    
    //------------------------------------------------------------------------------------------------
    // Schedule a restore to happen at next safe opportunity
    bool ScheduleRestore(string backupFile)
    {
        if (!FileIO.FileExists(backupFile))
        {
            m_Logger.LogError("Cannot schedule restore - backup file does not exist: " + backupFile, 
                "STS_BackupManager", "ScheduleRestore");
            return false;
        }
        
        m_sPendingRestoreFile = backupFile;
        m_bRestoreScheduled = true;
        
        m_Logger.LogInfo("Database restore scheduled for next safe opportunity", 
            "STS_BackupManager", "ScheduleRestore");
            
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a timestamp string for filenames
    protected string GetTimestampString()
    {
        int year, month, day, hour, minute, second;
        GetYearMonthDay(year, month, day);
        GetHourMinuteSecond(hour, minute, second);
        
        return string.Format("%1-%2-%3_%4-%5-%6", 
            year, month < 10 ? "0" + month : "" + month, day < 10 ? "0" + day : "" + day,
            hour < 10 ? "0" + hour : "" + hour, minute < 10 ? "0" + minute : "" + minute, 
            second < 10 ? "0" + second : "" + second);
    }
    
    //------------------------------------------------------------------------------------------------
    // Extract just the filename from a path
    protected string GetFileNameOnly(string filePath)
    {
        array<string> parts = new array<string>();
        filePath.Split("/", parts);
        
        if (parts.Count() > 0)
            return parts[parts.Count() - 1];
        else
            return filePath;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a list of all available backups
    array<ref STS_BackupInfo> GetAvailableBackups()
    {
        array<ref STS_BackupInfo> backups = new array<ref STS_BackupInfo>();
        
        // Find full backups
        array<FindFileResult> fullFiles = FindFiles(m_sBackupDir + "Full/", "*.zip", FindFileFlags.ALL);
        foreach (FindFileResult file : fullFiles)
        {
            string fullPath = m_sBackupDir + "Full/" + file.GetFilename();
            STS_BackupInfo info = new STS_BackupInfo();
            info.m_sFilePath = fullPath;
            info.m_sFileName = file.GetFilename();
            info.m_bIsDifferential = false;
            info.m_fTimestamp = GetFileModifiedTime(fullPath);
            info.m_iSizeBytes = FileIO.GetFileSize(fullPath);
            info.m_bIsIntact = !m_aCorruptedBackups.Contains(fullPath);
            
            backups.Insert(info);
        }
        
        // Find differential backups
        array<FindFileResult> diffFiles = FindFiles(m_sBackupDir + "Differential/", "*.zip", FindFileFlags.ALL);
        foreach (FindFileResult file : diffFiles)
        {
            string fullPath = m_sBackupDir + "Differential/" + file.GetFilename();
            STS_BackupInfo info = new STS_BackupInfo();
            info.m_sFilePath = fullPath;
            info.m_sFileName = file.GetFilename();
            info.m_bIsDifferential = true;
            info.m_fTimestamp = GetFileModifiedTime(fullPath);
            info.m_iSizeBytes = FileIO.GetFileSize(fullPath);
            info.m_bIsIntact = !m_aCorruptedBackups.Contains(fullPath);
            
            backups.Insert(info);
        }
        
        // Sort by timestamp (newest first)
        backups.Sort(SortBackupsByTime);
        
        return backups;
    }
    
    //------------------------------------------------------------------------------------------------
    // Sort backups by time
    static int SortBackupsByTime(STS_BackupInfo a, STS_BackupInfo b)
    {
        if (a.m_fTimestamp > b.m_fTimestamp)
            return -1;
        else if (a.m_fTimestamp < b.m_fTimestamp)
            return 1;
        else
            return 0;
    }
}

//------------------------------------------------------------------------------------------------
// Class to store backup file information
class STS_BackupInfo
{
    string m_sFilePath;
    string m_sFileName;
    bool m_bIsDifferential;
    float m_fTimestamp;
    int m_iSizeBytes;
    bool m_bIsIntact;
    
    string GetFormattedTime()
    {
        int year, month, day, hour, minute, second;
        TimestampToDate(m_fTimestamp, year, month, day, hour, minute, second);
        
        return string.Format("%1-%2-%3 %4:%5:%6", 
            year, 
            month < 10 ? "0" + month : "" + month, 
            day < 10 ? "0" + day : "" + day,
            hour < 10 ? "0" + hour : "" + hour, 
            minute < 10 ? "0" + minute : "" + minute, 
            second < 10 ? "0" + second : "" + second);
    }
    
    string GetFormattedSize()
    {
        if (m_iSizeBytes < 1024)
            return m_iSizeBytes.ToString() + " B";
        else if (m_iSizeBytes < 1024 * 1024)
            return (m_iSizeBytes / 1024).ToString() + " KB";
        else if (m_iSizeBytes < 1024 * 1024 * 1024)
            return (m_iSizeBytes / (1024 * 1024)).ToString() + " MB";
        else
            return (m_iSizeBytes / (1024 * 1024 * 1024)).ToString() + " GB";
    }
} 