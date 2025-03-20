// STS_MultiServerManager.c
// Multi-server integration manager that shares statistics across multiple servers

class STS_ServerInfo
{
    string m_sServerID;        // Unique server identifier
    string m_sServerName;      // Friendly server name
    string m_sServerAddress;   // Server address (IP:Port)
    float m_fLastSync;         // Last synchronization timestamp
    bool m_bActive;            // Whether this server is active
    string m_sApiKey;          // API key for secure communication
    
    void STS_ServerInfo(string id, string name, string address, string apiKey)
    {
        m_sServerID = id;
        m_sServerName = name;
        m_sServerAddress = address;
        m_fLastSync = 0;
        m_bActive = true;
        m_sApiKey = apiKey;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"serverID\":\"" + m_sServerID + "\",";
        json += "\"serverName\":\"" + m_sServerName + "\",";
        json += "\"serverAddress\":\"" + m_sServerAddress + "\",";
        json += "\"lastSync\":" + m_fLastSync.ToString() + ",";
        json += "\"active\":" + m_bActive.ToString() + ",";
        json += "\"apiKey\":\"" + m_sApiKey + "\"";
        json += "}";
        return json;
    }
    
    static STS_ServerInfo FromJSON(string json)
    {
        JsonSerializer js = new JsonSerializer();
        string error;
        STS_ServerInfo serverInfo = new STS_ServerInfo("", "", "", "");
        
        bool success = js.ReadFromString(serverInfo, json, error);
        if (!success)
        {
            Print("[StatTracker] Error parsing server info JSON: " + error, LogLevel.ERROR);
            return null;
        }
        
        return serverInfo;
    }
}

class STS_SyncResult
{
    bool m_bSuccess;
    string m_sMessage;
    int m_iRecordsProcessed;
    int m_iRecordsAdded;
    int m_iRecordsUpdated;
    int m_iRecordsFailed;
    
    void STS_SyncResult()
    {
        m_bSuccess = false;
        m_sMessage = "";
        m_iRecordsProcessed = 0;
        m_iRecordsAdded = 0;
        m_iRecordsUpdated = 0;
        m_iRecordsFailed = 0;
    }
}

class STS_SyncData
{
    string m_sServerID;
    int m_iTimestamp;
    ref array<ref map<string, string>> m_Data;
    
    void STS_SyncData(string serverID)
    {
        m_sServerID = serverID;
        m_iTimestamp = System.GetUnixTime();
        m_Data = new array<ref map<string, string>>();
    }
    
    void AddRecord(map<string, string> record)
    {
        m_Data.Insert(record);
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"serverID\":\"" + m_sServerID + "\",";
        json += "\"timestamp\":" + m_iTimestamp.ToString() + ",";
        
        json += "\"data\":[";
        for (int i = 0; i < m_Data.Count(); i++)
        {
            if (i > 0)
                json += ",";
                
            json += "{";
            int idx = 0;
            foreach (string key, string value : m_Data[i])
            {
                if (idx > 0)
                    json += ",";
                    
                json += "\"" + key + "\":\"" + value + "\"";
                idx++;
            }
            json += "}";
        }
        json += "]";
        
        json += "}";
        return json;
    }
}

class STS_MultiServerManager
{
    // Singleton instance
    private static ref STS_MultiServerManager s_Instance;
    
    // Logger reference
    protected STS_LoggingSystem m_Logger;
    
    // Configuration reference
    protected STS_Config m_Config;
    
    // API server reference
    protected STS_APIServer m_APIServer;
    
    // Personal stats portal reference
    protected STS_PersonalStatsPortal m_StatsPortal;
    
    // Current server information
    protected ref STS_ServerInfo m_ThisServer;
    
    // Network of servers
    protected ref array<ref STS_ServerInfo> m_NetworkServers = new array<ref STS_ServerInfo>();
    
    // Synchronized player stats
    protected ref map<string, ref map<string, float>> m_SyncedPlayerStats = new map<string, ref map<string, float>>();
    
    // Player cross-server tracking (playerID -> array of serverIDs)
    protected ref map<string, ref array<string>> m_PlayerServers = new map<string, ref array<string>>();
    
    // Constants
    protected const string SERVER_CONFIG_PATH = "$profile:StatTracker/MultiServer/server_network.json";
    protected const string SYNCED_STATS_PATH = "$profile:StatTracker/MultiServer/synced_stats.json";
    protected const string PLAYER_SERVERS_PATH = "$profile:StatTracker/MultiServer/player_servers.json";
    protected const float SYNC_INTERVAL = 300.0; // 5 minutes
    protected const float HEALTH_CHECK_INTERVAL = 60.0; // 1 minute
    
    // Last sync time
    protected float m_fLastSyncTime = 0;
    
    // Last health check time
    protected float m_fLastHealthCheckTime = 0;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_MultiServerManager()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Config = STS_Config.GetInstance();
        
        if (!m_Logger || !m_Config)
        {
            Print("[StatTracker] Failed to get required systems for MultiServerManager", LogLevel.ERROR);
            return;
        }
        
        m_Logger.LogInfo("Initializing Multi-Server Manager");
        
        // Create data directory if it doesn't exist
        string dir = "$profile:StatTracker/MultiServer";
        FileIO.MakeDirectory(dir);
        
        // Load server network configuration
        LoadNetworkConfiguration();
        
        // Load synced player stats
        LoadSyncedPlayerStats();
        
        // Load player server tracking
        LoadPlayerServers();
        
        // Get API server reference
        m_APIServer = STS_APIServer.GetInstance();
        if (!m_APIServer)
        {
            m_Logger.LogError("Failed to get API server reference - multi-server integration will be limited");
        }
        
        // Get personal stats portal reference
        m_StatsPortal = STS_PersonalStatsPortal.GetInstance();
        if (!m_StatsPortal)
        {
            m_Logger.LogError("Failed to get personal stats portal reference - player data synchronization will be limited");
        }
        
        // Start periodic sync and health checks
        GetGame().GetCallqueue().CallLater(Update, 30000, true); // Update every 30 seconds
        
        m_Logger.LogInfo("Multi-Server Manager initialized successfully");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_MultiServerManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_MultiServerManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Update function called periodically
    void Update()
    {
        float currentTime = System.GetTickCount() / 1000.0;
        
        // Perform health check on server network
        if (currentTime - m_fLastHealthCheckTime >= HEALTH_CHECK_INTERVAL)
        {
            PerformHealthCheck();
            m_fLastHealthCheckTime = currentTime;
        }
        
        // Synchronize data with other servers
        if (currentTime - m_fLastSyncTime >= SYNC_INTERVAL)
        {
            SynchronizeWithNetwork();
            m_fLastSyncTime = currentTime;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Set this server's information
    void SetThisServerInfo(string serverID, string serverName, string serverAddress, string apiKey)
    {
        m_ThisServer = new STS_ServerInfo(serverID, serverName, serverAddress, apiKey);
        
        // Save network configuration
        SaveNetworkConfiguration();
        
        m_Logger.LogInfo("This server configured with ID: " + serverID);
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a server to the network
    void AddNetworkServer(string serverID, string serverName, string serverAddress, string apiKey)
    {
        // Check if server already exists
        foreach (STS_ServerInfo server : m_NetworkServers)
        {
            if (server.m_sServerID == serverID)
            {
                // Update existing server info
                server.m_sServerName = serverName;
                server.m_sServerAddress = serverAddress;
                server.m_sApiKey = apiKey;
                server.m_bActive = true;
                
                m_Logger.LogInfo("Updated network server: " + serverID);
                SaveNetworkConfiguration();
                return;
            }
        }
        
        // Add new server
        STS_ServerInfo newServer = new STS_ServerInfo(serverID, serverName, serverAddress, apiKey);
        m_NetworkServers.Insert(newServer);
        
        m_Logger.LogInfo("Added new network server: " + serverID);
        SaveNetworkConfiguration();
    }
    
    //------------------------------------------------------------------------------------------------
    // Remove a server from the network
    void RemoveNetworkServer(string serverID)
    {
        for (int i = 0; i < m_NetworkServers.Count(); i++)
        {
            if (m_NetworkServers[i].m_sServerID == serverID)
            {
                m_NetworkServers.Remove(i);
                m_Logger.LogInfo("Removed network server: " + serverID);
                SaveNetworkConfiguration();
                return;
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Perform health check on server network
    protected void PerformHealthCheck()
    {
        m_Logger.LogDebug("Performing network health check");
        
        foreach (STS_ServerInfo server : m_NetworkServers)
        {
            // Skip inactive servers
            if (!server.m_bActive)
                continue;
                
            // Try to connect to server and check health
            bool isAlive = CheckServerHealth(server);
            
            if (!isAlive)
            {
                m_Logger.LogWarning("Server appears to be offline: " + server.m_sServerID);
                server.m_bActive = false;
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if a server is responsive
    protected bool CheckServerHealth(STS_ServerInfo server)
    {
        // In a real implementation, this would make an HTTP request to the server's health endpoint
        // For now, we'll simulate this with a simple check
        bool simulatedResult = Math.RandomInt(0, 10) < 9; // 90% chance of success
        
        return simulatedResult;
    }
    
    //------------------------------------------------------------------------------------------------
    // Synchronize data with other servers in the network
    protected void SynchronizeWithNetwork()
    {
        m_Logger.LogInfo("Synchronizing data with server network");
        
        // Skip if no other active servers
        bool hasActiveServers = false;
        foreach (STS_ServerInfo server : m_NetworkServers)
        {
            if (server.m_bActive)
            {
                hasActiveServers = true;
                break;
            }
        }
        
        if (!hasActiveServers)
        {
            m_Logger.LogInfo("No active servers in network, skipping synchronization");
            return;
        }
        
        // Push local data to other servers
        PushLocalDataToNetwork();
        
        // Pull data from other servers
        PullDataFromNetwork();
        
        // Update last sync time
        float currentTime = System.GetTickCount() / 1000.0;
        if (m_ThisServer)
        {
            m_ThisServer.m_fLastSync = currentTime;
            SaveNetworkConfiguration();
        }
        
        m_Logger.LogInfo("Network synchronization complete");
    }
    
    //------------------------------------------------------------------------------------------------
    // Push local data to other servers in the network
    protected void PushLocalDataToNetwork()
    {
        m_Logger.LogDebug("Pushing local data to network servers");
        
        // In a real implementation, this would make HTTP POST requests to each server's API
        // For now, we'll simulate this with a placeholder
        
        foreach (STS_ServerInfo server : m_NetworkServers)
        {
            if (!server.m_bActive)
                continue;
                
            m_Logger.LogDebug("Simulated data push to server: " + server.m_sServerID);
            
            // Simulate success/failure
            bool simulatedSuccess = Math.RandomInt(0, 10) < 8; // 80% chance of success
            
            if (!simulatedSuccess)
            {
                m_Logger.LogWarning("Failed to push data to server: " + server.m_sServerID);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Pull data from other servers in the network
    protected void PullDataFromNetwork()
    {
        m_Logger.LogDebug("Pulling data from network servers");
        
        // In a real implementation, this would make HTTP GET requests to each server's API
        // For now, we'll simulate this with some sample data
        
        foreach (STS_ServerInfo server : m_NetworkServers)
        {
            if (!server.m_bActive)
                continue;
                
            m_Logger.LogDebug("Simulated data pull from server: " + server.m_sServerID);
            
            // Simulate success/failure
            bool simulatedSuccess = Math.RandomInt(0, 10) < 8; // 80% chance of success
            
            if (!simulatedSuccess)
            {
                m_Logger.LogWarning("Failed to pull data from server: " + server.m_sServerID);
                continue;
            }
            
            // Simulate receiving player data
            SimulateDataPull(server);
        }
        
        // Save the updated synced stats
        SaveSyncedPlayerStats();
        SavePlayerServers();
    }
    
    //------------------------------------------------------------------------------------------------
    // Simulate receiving data from another server
    protected void SimulateDataPull(STS_ServerInfo server)
    {
        // This is just a placeholder to simulate receiving data
        // In a real implementation, this would process actual data received from the server
        
        // Simulate a few random player IDs
        for (int i = 0; i < 3; i++)
        {
            string playerID = (10000 + Math.RandomInt(0, 1000)).ToString();
            
            // Create or update stats for this player
            if (!m_SyncedPlayerStats.Contains(playerID))
            {
                m_SyncedPlayerStats.Set(playerID, new map<string, float>());
            }
            
            map<string, float> playerStats = m_SyncedPlayerStats.Get(playerID);
            
            // Add some random stats
            playerStats.Set("kills", Math.RandomFloat(10, 100));
            playerStats.Set("deaths", Math.RandomFloat(5, 50));
            playerStats.Set("playtime", Math.RandomFloat(60, 600));
            
            // Track that this player has been seen on this server
            TrackPlayerOnServer(playerID, server.m_sServerID);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Track that a player has been seen on a server
    protected void TrackPlayerOnServer(string playerID, string serverID)
    {
        // Create array if it doesn't exist
        if (!m_PlayerServers.Contains(playerID))
        {
            m_PlayerServers.Set(playerID, new array<string>());
        }
        
        array<string> serverList = m_PlayerServers.Get(playerID);
        
        // Add server ID if not already in the list
        if (!serverList.Contains(serverID))
        {
            serverList.Insert(serverID);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load network configuration
    protected void LoadNetworkConfiguration()
    {
        if (!FileIO.FileExists(SERVER_CONFIG_PATH))
        {
            m_Logger.LogInfo("No server network configuration found - initializing new configuration");
            
            // Generate a default server ID and API key for this server
            string defaultServerID = "server_" + Math.RandomInt(10000, 99999).ToString();
            string defaultApiKey = GenerateAPIKey();
            
            // Create default configuration for this server
            m_ThisServer = new STS_ServerInfo(defaultServerID, 
                "Default Server", 
                "localhost:8080", 
                defaultApiKey);
                
            SaveNetworkConfiguration();
            return;
        }
        
        string json = ReadFileContent(SERVER_CONFIG_PATH);
        if (json.IsEmpty())
        {
            m_Logger.LogError("Failed to read server network configuration");
            return;
        }
        
        try
        {
            JsonSerializer js = new JsonSerializer();
            string error;
            
            // Parse the JSON data
            ref map<string, Variant> data = new map<string, Variant>();
            
            bool success = js.ReadFromString(data, json, error);
            if (!success)
            {
                m_Logger.LogError("Failed to parse server network configuration: " + error);
                return;
            }
            
            // Get this server's info
            if (data.Contains("thisServer"))
            {
                string thisServerJson = data.Get("thisServer").AsString();
                m_ThisServer = STS_ServerInfo.FromJSON(thisServerJson);
                
                if (!m_ThisServer)
                {
                    m_Logger.LogError("Failed to parse this server's configuration");
                    m_ThisServer = new STS_ServerInfo("server_default", "Default Server", "localhost:8080", GenerateAPIKey());
                }
            }
            
            // Get network servers
            if (data.Contains("networkServers") && data.Get("networkServers").IsArray())
            {
                array<string> serverJsons = data.Get("networkServers").AsArray();
                
                foreach (string serverJson : serverJsons)
                {
                    STS_ServerInfo serverInfo = STS_ServerInfo.FromJSON(serverJson);
                    if (serverInfo)
                    {
                        m_NetworkServers.Insert(serverInfo);
                    }
                }
            }
            
            m_Logger.LogInfo(string.Format("Loaded server network configuration: This server: %1, Network servers: %2", 
                m_ThisServer ? m_ThisServer.m_sServerID : "None", 
                m_NetworkServers.Count()));
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception loading server network configuration: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save network configuration
    protected void SaveNetworkConfiguration()
    {
        try
        {
            string json = "{";
            
            // Add this server's info
            if (m_ThisServer)
            {
                json += "\"thisServer\":" + m_ThisServer.ToJSON() + ",";
            }
            else
            {
                json += "\"thisServer\":null,";
            }
            
            // Add network servers
            json += "\"networkServers\":[";
            
            for (int i = 0; i < m_NetworkServers.Count(); i++)
            {
                json += m_NetworkServers[i].ToJSON();
                
                if (i < m_NetworkServers.Count() - 1)
                {
                    json += ",";
                }
            }
            
            json += "]";
            json += "}";
            
            // Write to file
            FileHandle file = FileIO.OpenFile(SERVER_CONFIG_PATH, FileMode.WRITE);
            if (file != 0)
            {
                FileIO.WriteFile(file, json);
                FileIO.CloseFile(file);
                m_Logger.LogDebug("Saved server network configuration");
            }
            else
            {
                m_Logger.LogError("Failed to save server network configuration");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception saving server network configuration: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load synced player stats
    protected void LoadSyncedPlayerStats()
    {
        if (!FileIO.FileExists(SYNCED_STATS_PATH))
        {
            m_Logger.LogInfo("No synced player stats found");
            return;
        }
        
        string json = ReadFileContent(SYNCED_STATS_PATH);
        if (json.IsEmpty())
        {
            m_Logger.LogError("Failed to read synced player stats");
            return;
        }
        
        try
        {
            JsonSerializer js = new JsonSerializer();
            string error;
            
            // Parse the JSON data
            bool success = js.ReadFromString(m_SyncedPlayerStats, json, error);
            if (!success)
            {
                m_Logger.LogError("Failed to parse synced player stats: " + error);
                return;
            }
            
            m_Logger.LogInfo(string.Format("Loaded synced stats for %1 players", m_SyncedPlayerStats.Count()));
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception loading synced player stats: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save synced player stats
    protected void SaveSyncedPlayerStats()
    {
        try
        {
            JsonSerializer js = new JsonSerializer();
            string error;
            
            // Convert to JSON
            string json;
            bool success = js.WriteToString(m_SyncedPlayerStats, false, json, error);
            
            if (!success)
            {
                m_Logger.LogError("Failed to serialize synced player stats: " + error);
                return;
            }
            
            // Write to file
            FileHandle file = FileIO.OpenFile(SYNCED_STATS_PATH, FileMode.WRITE);
            if (file != 0)
            {
                FileIO.WriteFile(file, json);
                FileIO.CloseFile(file);
                m_Logger.LogDebug("Saved synced player stats");
            }
            else
            {
                m_Logger.LogError("Failed to save synced player stats");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception saving synced player stats: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load player servers tracking
    protected void LoadPlayerServers()
    {
        if (!FileIO.FileExists(PLAYER_SERVERS_PATH))
        {
            m_Logger.LogInfo("No player-server tracking data found");
            return;
        }
        
        string json = ReadFileContent(PLAYER_SERVERS_PATH);
        if (json.IsEmpty())
        {
            m_Logger.LogError("Failed to read player-server tracking data");
            return;
        }
        
        try
        {
            JsonSerializer js = new JsonSerializer();
            string error;
            
            // Parse the JSON data
            bool success = js.ReadFromString(m_PlayerServers, json, error);
            if (!success)
            {
                m_Logger.LogError("Failed to parse player-server tracking data: " + error);
                return;
            }
            
            m_Logger.LogInfo(string.Format("Loaded server tracking for %1 players", m_PlayerServers.Count()));
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception loading player-server tracking data: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save player servers tracking
    protected void SavePlayerServers()
    {
        try
        {
            JsonSerializer js = new JsonSerializer();
            string error;
            
            // Convert to JSON
            string json;
            bool success = js.WriteToString(m_PlayerServers, false, json, error);
            
            if (!success)
            {
                m_Logger.LogError("Failed to serialize player-server tracking data: " + error);
                return;
            }
            
            // Write to file
            FileHandle file = FileIO.OpenFile(PLAYER_SERVERS_PATH, FileMode.WRITE);
            if (file != 0)
            {
                FileIO.WriteFile(file, json);
                FileIO.CloseFile(file);
                m_Logger.LogDebug("Saved player-server tracking data");
            }
            else
            {
                m_Logger.LogError("Failed to save player-server tracking data");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception saving player-server tracking data: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper method to read file content
    protected string ReadFileContent(string filePath)
    {
        string content = "";
        
        FileHandle file = FileIO.OpenFile(filePath, FileMode.READ);
        if (file != 0)
        {
            content = FileIO.ReadFile(file);
            FileIO.CloseFile(file);
        }
        
        return content;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a random API key
    protected string GenerateAPIKey()
    {
        string key = "";
        string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        
        for (int i = 0; i < 32; i++)
        {
            int index = Math.RandomInt(0, chars.Length() - 1);
            key += chars[index].ToString();
        }
        
        return key;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get player data synchronization status
    string GetSynchronizationStatus()
    {
        string status = "";
        
        status += "Multi-Server Integration Status:\n";
        status += "----------------------------------\n";
        
        // This server info
        if (m_ThisServer)
        {
            status += "This Server: " + m_ThisServer.m_sServerName + " (ID: " + m_ThisServer.m_sServerID + ")\n";
            status += "Last Sync: " + (m_ThisServer.m_fLastSync > 0 ? 
                TimeToString(m_ThisServer.m_fLastSync) : "Never") + "\n\n";
        }
        else
        {
            status += "This Server: Not configured\n\n";
        }
        
        // Network servers
        status += "Network Servers: " + m_NetworkServers.Count() + "\n";
        
        if (m_NetworkServers.Count() > 0)
        {
            status += "----------------------------------\n";
            foreach (STS_ServerInfo server : m_NetworkServers)
            {
                status += server.m_sServerName + " (ID: " + server.m_sServerID + ")\n";
                status += "  Status: " + (server.m_bActive ? "Active" : "Inactive") + "\n";
                status += "  Address: " + server.m_sServerAddress + "\n";
                status += "  Last Sync: " + (server.m_fLastSync > 0 ? 
                    TimeToString(server.m_fLastSync) : "Never") + "\n";
                    
                status += "----------------------------------\n";
            }
        }
        
        // Synced player data
        status += "\nSynced Player Data: " + m_SyncedPlayerStats.Count() + " players\n";
        status += "Cross-Server Player Tracking: " + m_PlayerServers.Count() + " players\n";
        
        return status;
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper method to format a timestamp
    protected string TimeToString(float timestamp)
    {
        int unixTime = timestamp;
        int year, month, day, hour, minute, second;
        
        System.GetYearMonthDayUTC(year, month, day, unixTime);
        System.GetHourMinuteSecondUTC(hour, minute, second, unixTime);
        
        return string.Format("%1-%2-%3 %4:%5:%6", 
            year, 
            month < 10 ? "0" + month.ToString() : month.ToString(), 
            day < 10 ? "0" + day.ToString() : day.ToString(),
            hour < 10 ? "0" + hour.ToString() : hour.ToString(),
            minute < 10 ? "0" + minute.ToString() : minute.ToString(),
            second < 10 ? "0" + second.ToString() : second.ToString());
    }
    
    //------------------------------------------------------------------------------------------------
    // Get cross-server stats for a player
    map<string, map<string, float>> GetCrossServerStats(string playerID)
    {
        map<string, map<string, float>> result = new map<string, map<string, float>>();
        
        // Add synced stats
        if (m_SyncedPlayerStats.Contains(playerID))
        {
            map<string, float> playerStats = m_SyncedPlayerStats.Get(playerID);
            result.Set("synced", playerStats);
        }
        
        // Get server list for this player
        if (m_PlayerServers.Contains(playerID))
        {
            array<string> serverList = m_PlayerServers.Get(playerID);
            
            // Create a map of server names
            map<string, string> serverNames = new map<string, string>();
            
            // Add server list
            foreach (string serverID : serverList)
            {
                bool foundServer = false;
                
                // Check if it's this server
                if (m_ThisServer && m_ThisServer.m_sServerID == serverID)
                {
                    serverNames.Set(serverID, m_ThisServer.m_sServerName);
                    foundServer = true;
                }
                
                // Check network servers
                if (!foundServer)
                {
                    foreach (STS_ServerInfo server : m_NetworkServers)
                    {
                        if (server.m_sServerID == serverID)
                        {
                            serverNames.Set(serverID, server.m_sServerName);
                            foundServer = true;
                            break;
                        }
                    }
                }
                
                // If server not found, use the ID as name
                if (!foundServer)
                {
                    serverNames.Set(serverID, serverID);
                }
            }
            
            // Convert to stats format
            map<string, float> serverStats = new map<string, float>();
            foreach (string serverID, string serverName : serverNames)
            {
                // Use server ID as key, set value to 1 (presence indicator)
                serverStats.Set(serverID, 1);
            }
            
            result.Set("servers", serverStats);
        }
        
        return result;
    }
} 