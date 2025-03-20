// STS_AdvancedBackupSystem.c
// Advanced backup system that integrates with multi-server functionality

class STS_AdvancedBackupSystem
{
    // Singleton instance
    private static ref STS_AdvancedBackupSystem s_Instance;
    
    // References
    protected ref STS_LoggingSystem m_Logger;
    protected ref STS_BackupManager m_BackupManager;
    protected ref STS_MultiServerIntegration m_MultiServerIntegration;
    protected ref STS_Config m_Config;
    
    // Configuration
    protected bool m_bEnabled = true;
    protected bool m_bAutoBackupOnConfigChange = true;
    protected bool m_bAutoBackupBeforeServerShutdown = true;
    protected bool m_bAutoBackupAfterMajorPlayerCountChange = true;
    protected bool m_bUseIncrementalBackups = true;
    protected bool m_bReplicateBackupsAcrossServers = true;
    protected int m_iBackupCompressionLevel = 9; // 0-9 (0 = no compression, 9 = max compression)
    protected string m_sPrimaryBackupServer = ""; // Server ID that should store primary backups
    
    // Backup coordination
    protected bool m_bIsCoordinatorServer = false;
    protected float m_fLastClusterBackupTime = 0;
    protected int m_iClusterBackupIntervalHours = 24;
    protected ref array<string> m_aPreBackupCommands = new array<string>();
    protected ref array<string> m_aPostBackupCommands = new array<string>();
    
    // Backup stats
    protected int m_iTotalBackupsCreated = 0;
    protected int m_iTotalBackupsRestored = 0;
    protected float m_fTotalBackupSize = 0;
    protected float m_fLastBackupSize = 0;
    protected float m_fLastBackupDuration = 0;
    protected int m_iSuccessfulBackups = 0;
    protected int m_iFailedBackups = 0;
    
    // Backup notifications
    protected bool m_bNotifyAdminsOnBackupFailure = true;
    protected bool m_bNotifyAdminsOnSuccessfulRestore = true;
    protected bool m_bNotifyPlayersBeforeBackup = false;
    
    //------------------------------------------------------------------------------------------------
    protected void STS_AdvancedBackupSystem()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_BackupManager = STS_BackupManager.GetInstance();
        m_MultiServerIntegration = STS_MultiServerIntegration.GetInstance();
        m_Config = STS_Config.GetInstance();
        
        // Load configuration
        LoadConfiguration();
        
        if (m_bEnabled)
        {
            // Setup event handlers
            if (m_Config)
            {
                m_Config.RegisterForConfigChange(OnConfigChanged);
            }
            
            // Register for server events
            GetGame().GetCallqueue().CallLater(CheckClusterBackupSchedule, 300000, true); // Check every 5 minutes
            
            // Setup shutdown handler
            GetGame().GetCallqueue().CallLater(RegisterShutdownHandler, 1000, false);
            
            m_Logger.LogInfo("Advanced Backup System initialized", "STS_AdvancedBackupSystem", "Constructor");
        }
        else
        {
            m_Logger.LogInfo("Advanced Backup System is disabled in configuration", "STS_AdvancedBackupSystem", "Constructor");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_AdvancedBackupSystem GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_AdvancedBackupSystem();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load configuration from main config
    protected void LoadConfiguration()
    {
        if (!m_Config)
            return;
            
        m_bEnabled = m_Config.GetConfigValueBool("advanced_backup_enabled", m_bEnabled);
        m_bAutoBackupOnConfigChange = m_Config.GetConfigValueBool("auto_backup_on_config_change", m_bAutoBackupOnConfigChange);
        m_bAutoBackupBeforeServerShutdown = m_Config.GetConfigValueBool("auto_backup_before_shutdown", m_bAutoBackupBeforeServerShutdown);
        m_bAutoBackupAfterMajorPlayerCountChange = m_Config.GetConfigValueBool("auto_backup_after_player_change", m_bAutoBackupAfterMajorPlayerCountChange);
        m_bUseIncrementalBackups = m_Config.GetConfigValueBool("use_incremental_backups", m_bUseIncrementalBackups);
        m_bReplicateBackupsAcrossServers = m_Config.GetConfigValueBool("replicate_backups_across_servers", m_bReplicateBackupsAcrossServers);
        m_iBackupCompressionLevel = m_Config.GetConfigValueInt("backup_compression_level", m_iBackupCompressionLevel);
        m_sPrimaryBackupServer = m_Config.GetConfigValueString("primary_backup_server", m_sPrimaryBackupServer);
        m_bIsCoordinatorServer = m_Config.GetConfigValueBool("is_backup_coordinator", m_bIsCoordinatorServer);
        m_iClusterBackupIntervalHours = m_Config.GetConfigValueInt("cluster_backup_interval_hours", m_iClusterBackupIntervalHours);
        m_bNotifyAdminsOnBackupFailure = m_Config.GetConfigValueBool("notify_admins_on_backup_failure", m_bNotifyAdminsOnBackupFailure);
        m_bNotifyAdminsOnSuccessfulRestore = m_Config.GetConfigValueBool("notify_admins_on_successful_restore", m_bNotifyAdminsOnSuccessfulRestore);
        m_bNotifyPlayersBeforeBackup = m_Config.GetConfigValueBool("notify_players_before_backup", m_bNotifyPlayersBeforeBackup);
        
        // Load pre/post backup commands
        string preCommandsString = m_Config.GetConfigValueString("pre_backup_commands", "");
        string postCommandsString = m_Config.GetConfigValueString("post_backup_commands", "");
        
        m_aPreBackupCommands.Clear();
        m_aPostBackupCommands.Clear();
        
        if (preCommandsString != "")
        {
            array<string> commands = new array<string>();
            preCommandsString.Split(",", commands);
            foreach (string cmd : commands)
            {
                m_aPreBackupCommands.Insert(cmd.Trim());
            }
        }
        
        if (postCommandsString != "")
        {
            array<string> commands = new array<string>();
            postCommandsString.Split(",", commands);
            foreach (string cmd : commands)
            {
                m_aPostBackupCommands.Insert(cmd.Trim());
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle configuration changes
    void OnConfigChanged(map<string, string> changedValues)
    {
        // Update configuration values
        if (changedValues.Contains("advanced_backup_enabled"))
            m_bEnabled = changedValues.Get("advanced_backup_enabled").ToBool();
            
        if (changedValues.Contains("auto_backup_on_config_change"))
            m_bAutoBackupOnConfigChange = changedValues.Get("auto_backup_on_config_change").ToBool();
            
        if (changedValues.Contains("auto_backup_before_shutdown"))
            m_bAutoBackupBeforeServerShutdown = changedValues.Get("auto_backup_before_shutdown").ToBool();
            
        if (changedValues.Contains("auto_backup_after_player_change"))
            m_bAutoBackupAfterMajorPlayerCountChange = changedValues.Get("auto_backup_after_player_change").ToBool();
            
        if (changedValues.Contains("use_incremental_backups"))
            m_bUseIncrementalBackups = changedValues.Get("use_incremental_backups").ToBool();
            
        if (changedValues.Contains("replicate_backups_across_servers"))
            m_bReplicateBackupsAcrossServers = changedValues.Get("replicate_backups_across_servers").ToBool();
            
        if (changedValues.Contains("backup_compression_level"))
            m_iBackupCompressionLevel = changedValues.Get("backup_compression_level").ToInt();
            
        if (changedValues.Contains("primary_backup_server"))
            m_sPrimaryBackupServer = changedValues.Get("primary_backup_server");
            
        if (changedValues.Contains("is_backup_coordinator"))
            m_bIsCoordinatorServer = changedValues.Get("is_backup_coordinator").ToBool();
            
        if (changedValues.Contains("cluster_backup_interval_hours"))
            m_iClusterBackupIntervalHours = changedValues.Get("cluster_backup_interval_hours").ToInt();
        
        // Create a backup if auto backup on config change is enabled
        if (m_bEnabled && m_bAutoBackupOnConfigChange)
        {
            CreateBackup(true, "Config changed");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Register shutdown handler
    protected void RegisterShutdownHandler()
    {
        // In a real implementation, you would hook into the server shutdown event
        // For now, we'll just log it
        m_Logger.LogInfo("Registered server shutdown handler for auto-backup", 
            "STS_AdvancedBackupSystem", "RegisterShutdownHandler");
    }
    
    //------------------------------------------------------------------------------------------------
    // Create a new backup with advanced options
    bool CreateBackup(bool forceFull = false, string reason = "Manual")
    {
        if (!m_bEnabled || !m_BackupManager)
            return false;
            
        m_Logger.LogInfo("Creating advanced backup. Reason: " + reason, 
            "STS_AdvancedBackupSystem", "CreateBackup");
            
        // Notify players if configured
        if (m_bNotifyPlayersBeforeBackup)
        {
            NotifyPlayersOfBackup();
        }
        
        // Run pre-backup commands
        ExecutePreBackupCommands();
        
        // Start timing
        float startTime = GetGame().GetTickTime();
        
        // Create the backup
        bool success = m_BackupManager.CreateBackup(forceFull);
        
        // Record stats
        m_fLastBackupDuration = GetGame().GetTickTime() - startTime;
        
        if (success)
        {
            m_iSuccessfulBackups++;
            m_iTotalBackupsCreated++;
            
            // Get backup size
            if (m_BackupManager.m_sLastBackupFile != "")
            {
                m_fLastBackupSize = FileIO.GetFileSize(m_BackupManager.m_sLastBackupFile);
                m_fTotalBackupSize += m_fLastBackupSize;
            }
            
            // Run post-backup commands
            ExecutePostBackupCommands();
            
            // Replicate to other servers if configured
            if (m_bReplicateBackupsAcrossServers && m_MultiServerIntegration)
            {
                ReplicateBackupToOtherServers(m_BackupManager.m_sLastBackupFile);
            }
            
            m_Logger.LogInfo(string.Format("Advanced backup completed successfully in %.2f seconds, size: %.2f MB", 
                m_fLastBackupDuration, m_fLastBackupSize / (1024 * 1024)), 
                "STS_AdvancedBackupSystem", "CreateBackup");
        }
        else
        {
            m_iFailedBackups++;
            
            m_Logger.LogError("Advanced backup failed", "STS_AdvancedBackupSystem", "CreateBackup");
            
            // Notify admins of failure if configured
            if (m_bNotifyAdminsOnBackupFailure)
            {
                NotifyAdminsOfBackupFailure();
            }
        }
        
        return success;
    }
    
    //------------------------------------------------------------------------------------------------
    // Notify players that a backup is about to start
    protected void NotifyPlayersOfBackup()
    {
        // In a real implementation, this would send a message to all players
        m_Logger.LogInfo("Would notify players about upcoming backup", 
            "STS_AdvancedBackupSystem", "NotifyPlayersOfBackup");
    }
    
    //------------------------------------------------------------------------------------------------
    // Execute pre-backup commands
    protected void ExecutePreBackupCommands()
    {
        foreach (string cmd : m_aPreBackupCommands)
        {
            // In a real implementation, this would execute server commands
            m_Logger.LogDebug("Would execute pre-backup command: " + cmd, 
                "STS_AdvancedBackupSystem", "ExecutePreBackupCommands");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Execute post-backup commands
    protected void ExecutePostBackupCommands()
    {
        foreach (string cmd : m_aPostBackupCommands)
        {
            // In a real implementation, this would execute server commands
            m_Logger.LogDebug("Would execute post-backup command: " + cmd, 
                "STS_AdvancedBackupSystem", "ExecutePostBackupCommands");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Notify admins of backup failure
    protected void NotifyAdminsOfBackupFailure()
    {
        // In a real implementation, this would send notifications to admins
        m_Logger.LogInfo("Would notify admins about backup failure", 
            "STS_AdvancedBackupSystem", "NotifyAdminsOfBackupFailure");
    }
    
    //------------------------------------------------------------------------------------------------
    // Replicate a backup to other servers in the cluster
    protected void ReplicateBackupToOtherServers(string backupFile)
    {
        if (!m_MultiServerIntegration || !FileIO.FileExists(backupFile))
            return;
            
        m_Logger.LogInfo("Replicating backup to other servers: " + backupFile, 
            "STS_AdvancedBackupSystem", "ReplicateBackupToOtherServers");
            
        // In a real implementation, this would use the multi-server integration to send 
        // the backup to other servers in the cluster
        // For now, we'll just log it
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if a cluster-wide coordinated backup should be performed
    protected void CheckClusterBackupSchedule()
    {
        if (!m_bEnabled || !m_bIsCoordinatorServer)
            return;
            
        float currentTime = GetGame().GetTickTime();
        float hoursSinceLastClusterBackup = (currentTime - m_fLastClusterBackupTime) / 3600;
        
        if (hoursSinceLastClusterBackup >= m_iClusterBackupIntervalHours)
        {
            CoordinateClusterBackup();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Coordinate a backup across all servers in the cluster
    protected void CoordinateClusterBackup()
    {
        if (!m_MultiServerIntegration)
            return;
            
        m_Logger.LogInfo("Coordinating cluster-wide backup", 
            "STS_AdvancedBackupSystem", "CoordinateClusterBackup");
            
        // First back up this server
        CreateBackup(true, "Cluster backup");
        
        // Then send backup command to all other servers
        if (m_MultiServerIntegration)
        {
            map<string, string> parameters = new map<string, string>();
            parameters.Set("reason", "Cluster backup");
            
            // In a real implementation, this would send a command to all servers
            // to perform a backup
            // m_MultiServerIntegration.ExecuteNetworkCommand("create_backup", parameters);
        }
        
        m_fLastClusterBackupTime = GetGame().GetTickTime();
    }
    
    //------------------------------------------------------------------------------------------------
    // Restore from a backup file across all servers
    bool CoordinatedRestore(string backupFile, bool allServers = false)
    {
        if (!m_bEnabled || !m_BackupManager)
            return false;
            
        if (!FileIO.FileExists(backupFile))
        {
            m_Logger.LogError("Cannot restore - backup file does not exist: " + backupFile, 
                "STS_AdvancedBackupSystem", "CoordinatedRestore");
            return false;
        }
        
        // First schedule the restore on this server
        bool success = m_BackupManager.ScheduleRestore(backupFile);
        
        if (success)
        {
            m_iTotalBackupsRestored++;
            
            // If this is a multi-server restore, coordinate with other servers
            if (allServers && m_MultiServerIntegration)
            {
                map<string, string> parameters = new map<string, string>();
                parameters.Set("backup_file", backupFile);
                
                // In a real implementation, this would send a command to all servers
                // to restore from the same backup
                // m_MultiServerIntegration.ExecuteNetworkCommand("restore_backup", parameters);
            }
            
            // Notify admins if configured
            if (m_bNotifyAdminsOnSuccessfulRestore)
            {
                NotifyAdminsOfRestoreScheduled(backupFile, allServers);
            }
            
            m_Logger.LogInfo(string.Format("Coordinated restore scheduled from backup: %1, All servers: %2", 
                backupFile, allServers ? "Yes" : "No"), "STS_AdvancedBackupSystem", "CoordinatedRestore");
        }
        
        return success;
    }
    
    //------------------------------------------------------------------------------------------------
    // Notify admins that a restore has been scheduled
    protected void NotifyAdminsOfRestoreScheduled(string backupFile, bool allServers)
    {
        // In a real implementation, this would send notifications to admins
        m_Logger.LogInfo(string.Format("Would notify admins about scheduled restore from %1 (All servers: %2)", 
            backupFile, allServers ? "Yes" : "No"), "STS_AdvancedBackupSystem", "NotifyAdminsOfRestoreScheduled");
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle a player count change (e.g., for auto-backup after major player count changes)
    void OnPlayerCountChanged(int oldCount, int newCount)
    {
        if (!m_bEnabled || !m_bAutoBackupAfterMajorPlayerCountChange)
            return;
            
        // Define what a "major" change is - for example, going from some players to zero
        if (oldCount > 0 && newCount == 0)
        {
            m_Logger.LogInfo("Creating backup due to server emptying", 
                "STS_AdvancedBackupSystem", "OnPlayerCountChanged");
            CreateBackup(false, "Server emptied");
        }
        // Or a large influx of players
        else if (newCount > oldCount * 2 && newCount > 10)
        {
            m_Logger.LogInfo("Creating backup due to major player influx", 
                "STS_AdvancedBackupSystem", "OnPlayerCountChanged");
            CreateBackup(false, "Major player influx");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle server shutdown (creates backup before shutdown if configured)
    void OnServerShutdown()
    {
        if (!m_bEnabled || !m_bAutoBackupBeforeServerShutdown)
            return;
            
        m_Logger.LogInfo("Creating backup before server shutdown", 
            "STS_AdvancedBackupSystem", "OnServerShutdown");
        CreateBackup(true, "Server shutdown");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get backup statistics as a formatted string
    string GetBackupStats()
    {
        string stats = "=== Advanced Backup System Statistics ===\n";
        stats += string.Format("Total backups created: %1\n", m_iTotalBackupsCreated);
        stats += string.Format("Successful backups: %1\n", m_iSuccessfulBackups);
        stats += string.Format("Failed backups: %1\n", m_iFailedBackups);
        stats += string.Format("Total backups restored: %1\n", m_iTotalBackupsRestored);
        stats += string.Format("Total backup data: %.2f MB\n", m_fTotalBackupSize / (1024 * 1024));
        
        if (m_iTotalBackupsCreated > 0)
        {
            stats += string.Format("Average backup size: %.2f MB\n", (m_fTotalBackupSize / m_iTotalBackupsCreated) / (1024 * 1024));
        }
        
        if (m_fLastBackupDuration > 0)
        {
            stats += string.Format("Last backup duration: %.2f seconds\n", m_fLastBackupDuration);
        }
        
        if (m_fLastBackupSize > 0)
        {
            stats += string.Format("Last backup size: %.2f MB\n", m_fLastBackupSize / (1024 * 1024));
        }
        
        return stats;
    }
} 