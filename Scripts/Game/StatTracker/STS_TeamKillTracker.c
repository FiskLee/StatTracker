// STS_TeamKillTracker.c
// System for tracking team kills and managing penalties

enum ETeamKillSeverity
{
    Low,     // Single unintentional TK
    Medium,  // Multiple TKs in a short period
    High,    // Repeated offenses or clear pattern
    Critical // Systematic team killing
}

class STS_TeamKillEvent
{
    string m_sKillerID;        // Unique player ID
    string m_sKillerName;      // Player name
    string m_sVictimID;        // Unique victim ID
    string m_sVictimName;      // Victim name
    float m_fTimestamp;        // When the TK occurred
    vector m_vPosition;        // World position
    string m_sWeapon;          // Weapon/vehicle used
    int m_iFactionKiller;      // Killer's faction
    int m_iFactionVictim;      // Victim's faction
    bool m_bReviewed;          // Whether an admin has reviewed this
    string m_sAdminNotes;      // Admin notes after review
    string m_sSessionId;       // Game session ID
    int m_iTeamId;             // Team ID (for backward compatibility)
    
    void STS_TeamKillEvent(string killerID, string killerName, string victimID, string victimName, float timestamp, vector position, string weapon, int factionKiller = 0, int factionVictim = 0, int teamId = 0)
    {
        m_sKillerID = killerID;
        m_sKillerName = killerName;
        m_sVictimID = victimID;
        m_sVictimName = victimName;
        m_fTimestamp = timestamp;
        m_vPosition = position;
        m_sWeapon = weapon;
        m_iFactionKiller = factionKiller;
        m_iFactionVictim = factionVictim;
        m_iTeamId = teamId;
        m_bReviewed = false;
        m_sAdminNotes = "";
        m_sSessionId = STS_PersistenceManager.GetInstance().GetCurrentSessionId();
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"killerID\":\"" + m_sKillerID + "\",";
        json += "\"killerName\":\"" + m_sKillerName + "\",";
        json += "\"victimID\":\"" + m_sVictimID + "\",";
        json += "\"victimName\":\"" + m_sVictimName + "\",";
        json += "\"timestamp\":" + m_fTimestamp.ToString() + ",";
        json += "\"position\":[" + m_vPosition[0].ToString() + "," + m_vPosition[1].ToString() + "," + m_vPosition[2].ToString() + "],";
        json += "\"weapon\":\"" + m_sWeapon + "\",";
        json += "\"factionKiller\":" + m_iFactionKiller.ToString() + ",";
        json += "\"factionVictim\":" + m_iFactionVictim.ToString() + ",";
        json += "\"teamId\":" + m_iTeamId.ToString() + ",";
        json += "\"reviewed\":" + m_bReviewed.ToString() + ",";
        json += "\"adminNotes\":\"" + m_sAdminNotes + "\",";
        json += "\"sessionId\":\"" + m_sSessionId + "\"";
        json += "}";
        return json;
    }
    
    static STS_TeamKillEvent FromJSON(string json)
    {
        JsonSerializer js = new JsonSerializer();
        string error;
        STS_TeamKillEvent event = new STS_TeamKillEvent("", "", "", "", 0, vector.Zero, "");
        
        bool success = js.ReadFromString(event, json, error);
        if (!success)
        {
            Print("[StatTracker] Error parsing team kill event JSON: " + error);
            return null;
        }
        
        return event;
    }
    
    // Convert from old format TeamKillRecord
    static STS_TeamKillEvent FromLegacyRecord(int killerID, string killerName, int victimID, string victimName, vector location, string weaponUsed)
    {
        return new STS_TeamKillEvent(killerID.ToString(), killerName, victimID.ToString(), victimName, System.GetTickCount() / 1000.0, location, weaponUsed);
    }
    
    // Convert from old format TeamKillEntry
    static STS_TeamKillEvent FromLegacyEntry(int killerID, string killerName, int victimID, string victimName, string weaponUsed, int teamID)
    {
        return new STS_TeamKillEvent(killerID.ToString(), killerName, victimID.ToString(), victimName, System.GetTickCount() / 1000.0, vector.Zero, weaponUsed, 0, 0, teamID);
    }
}

class STS_PlayerTKRecord
{
    string m_sPlayerID;        // Unique player ID
    string m_sPlayerName;      // Player name
    array<ref STS_TeamKillEvent> m_aTeamKills; // All team kills committed
    array<ref STS_TeamKillEvent> m_aTeamDeaths; // All team deaths (killed by team)
    int m_iTKPoints;           // Accumulated TK points (higher = worse)
    int m_iWarnings;           // Number of warnings issued
    int m_iKicks;              // Number of times kicked for TK
    int m_iBans;               // Number of times banned for TK
    bool m_bIsBanned;          // Currently banned for TK
    float m_fLastTKTime;       // Last time player committed a TK
    
    void STS_PlayerTKRecord(string playerID, string playerName)
    {
        m_sPlayerID = playerID;
        m_sPlayerName = playerName;
        m_aTeamKills = new array<ref STS_TeamKillEvent>();
        m_aTeamDeaths = new array<ref STS_TeamKillEvent>();
        m_iTKPoints = 0;
        m_iWarnings = 0;
        m_iKicks = 0;
        m_iBans = 0;
        m_bIsBanned = false;
        m_fLastTKTime = 0;
    }
    
    void AddTeamKill(STS_TeamKillEvent event)
    {
        if (!event)
            return;
            
        m_aTeamKills.Insert(event);
        m_fLastTKTime = System.GetTickCount() / 1000.0;
        
        // Add TK points based on recency
        // Recent TKs count more heavily
        int tkPointsToAdd = 10;
        
        // If multiple TKs in short period, add more points
        if (m_aTeamKills.Count() > 1)
        {
            float timeSinceLastTK = m_fLastTKTime - (m_aTeamKills.Count() > 1 ? m_aTeamKills[m_aTeamKills.Count() - 2].m_fTimestamp : 0);
            if (timeSinceLastTK < 300) // 5 minutes
            {
                tkPointsToAdd += 15; // Significant penalty for rapid TKs
            }
        }
        
        m_iTKPoints += tkPointsToAdd;
    }
    
    void AddTeamDeath(STS_TeamKillEvent event)
    {
        if (!event)
            return;
            
        m_aTeamDeaths.Insert(event);
    }
    
    // Decay TK points over time (call periodically)
    void DecayTKPoints(float currentTime)
    {
        // If no TKs, nothing to decay
        if (m_aTeamKills.Count() == 0)
            return;
            
        // Calculate time since last TK
        float timeSinceLastTK = currentTime - m_fLastTKTime;
        
        // Apply decay based on time elapsed (decay more over longer periods)
        // 1 point per hour, capped at current TK points
        int decayAmount = Math.Floor(timeSinceLastTK / 3600.0);
        
        if (decayAmount > 0)
        {
            m_iTKPoints = Math.Max(0, m_iTKPoints - decayAmount);
        }
    }
    
    // Get the severity level based on TK points and history
    ETeamKillSeverity GetSeverityLevel()
    {
        if (m_iTKPoints < 20)
            return ETeamKillSeverity.Low;
        else if (m_iTKPoints < 50)
            return ETeamKillSeverity.Medium;
        else if (m_iTKPoints < 100)
            return ETeamKillSeverity.High;
        else
            return ETeamKillSeverity.Critical;
    }
    
    string ToJSON()
    {
        // Serialize team kills
        string teamKillsJson = "[";
        for (int i = 0; i < m_aTeamKills.Count(); i++)
        {
            teamKillsJson += m_aTeamKills[i].ToJSON();
            if (i < m_aTeamKills.Count() - 1)
                teamKillsJson += ",";
        }
        teamKillsJson += "]";
        
        // Serialize team deaths
        string teamDeathsJson = "[";
        for (int i = 0; i < m_aTeamDeaths.Count(); i++)
        {
            teamDeathsJson += m_aTeamDeaths[i].ToJSON();
            if (i < m_aTeamDeaths.Count() - 1)
                teamDeathsJson += ",";
        }
        teamDeathsJson += "]";
        
        string json = "{";
        json += "\"playerID\":\"" + m_sPlayerID + "\",";
        json += "\"playerName\":\"" + m_sPlayerName + "\",";
        json += "\"teamKills\":" + teamKillsJson + ",";
        json += "\"teamDeaths\":" + teamDeathsJson + ",";
        json += "\"tkPoints\":" + m_iTKPoints.ToString() + ",";
        json += "\"warnings\":" + m_iWarnings.ToString() + ",";
        json += "\"kicks\":" + m_iKicks.ToString() + ",";
        json += "\"bans\":" + m_iBans.ToString() + ",";
        json += "\"isBanned\":" + m_bIsBanned.ToString() + ",";
        json += "\"lastTKTime\":" + m_fLastTKTime.ToString();
        json += "}";
        return json;
    }
}

class TeamKillRecord
{
    int m_iKillerID;
    string m_sKillerName;
    int m_iVictimID;
    string m_sVictimName;
    float m_fTimestamp;
    vector m_vLocation;
    string m_sWeaponUsed;
    
    void TeamKillRecord(int killerID, string killerName, int victimID, string victimName, vector location, string weaponUsed)
    {
        m_iKillerID = killerID;
        m_sKillerName = killerName;
        m_iVictimID = victimID;
        m_sVictimName = victimName;
        m_fTimestamp = System.GetTickCount() / 1000.0;
        m_vLocation = location;
        m_sWeaponUsed = weaponUsed;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"killerID\":" + m_iKillerID.ToString() + ",";
        json += "\"killerName\":\"" + m_sKillerName + "\",";
        json += "\"victimID\":" + m_iVictimID.ToString() + ",";
        json += "\"victimName\":\"" + m_sVictimName + "\",";
        json += "\"timestamp\":" + m_fTimestamp.ToString() + ",";
        json += "\"location\":[" + m_vLocation[0].ToString() + "," + m_vLocation[1].ToString() + "," + m_vLocation[2].ToString() + "],";
        json += "\"weaponUsed\":\"" + m_sWeaponUsed + "\"";
        json += "}";
        return json;
    }
    
    static TeamKillRecord FromJSON(string json)
    {
        // Create a default record
        TeamKillRecord record = new TeamKillRecord(0, "", 0, "", "0 0 0".ToVector(), "");
        
        // Parse JSON (simplified parser)
        array<string> pairs = new array<string>();
        json.Replace("{", "").Replace("}", "").Split(",", pairs);
        
        foreach (string pair : pairs)
        {
            array<string> keyValue = new array<string>();
            pair.Split(":", keyValue);
            
            if (keyValue.Count() < 2)
                continue;
                
            string key = keyValue[0].Replace("\"", "");
            string value = keyValue[1].Replace("\"", "");
            
            if (key == "killerID") record.m_iKillerID = value.ToInt();
            else if (key == "killerName") record.m_sKillerName = value;
            else if (key == "victimID") record.m_iVictimID = value.ToInt();
            else if (key == "victimName") record.m_sVictimName = value;
            else if (key == "timestamp") record.m_fTimestamp = value.ToFloat();
            else if (key == "weaponUsed") record.m_sWeaponUsed = value;
            // Location needs special handling for the array
            // In a real implementation, use a proper JSON parser
        }
        
        return record;
    }
}

class TeamKillEntry
{
    int m_iKillerID;
    string m_sKillerName;
    int m_iVictimID;
    string m_sVictimName;
    string m_sWeaponUsed;
    int m_iTeamID;
    float m_fTimestamp;
    
    void TeamKillEntry(int killerID, string killerName, int victimID, string victimName, string weaponUsed, int teamID)
    {
        m_iKillerID = killerID;
        m_sKillerName = killerName;
        m_iVictimID = victimID;
        m_sVictimName = victimName;
        m_sWeaponUsed = weaponUsed;
        m_iTeamID = teamID;
        m_fTimestamp = System.GetTickCount() / 1000.0;
    }
    
    // Convert to JSON
    string ToJSON()
    {
        string json = "{";
        json += "\"killerID\":" + m_iKillerID.ToString() + ",";
        json += "\"killerName\":\"" + m_sKillerName + "\",";
        json += "\"victimID\":" + m_iVictimID.ToString() + ",";
        json += "\"victimName\":\"" + m_sVictimName + "\",";
        json += "\"weaponUsed\":\"" + m_sWeaponUsed + "\",";
        json += "\"teamID\":" + m_iTeamID.ToString() + ",";
        json += "\"timestamp\":" + m_fTimestamp.ToString();
        json += "}";
        return json;
    }
}

class STS_TeamKillTracker
{
    // Singleton instance
    private static ref STS_TeamKillTracker s_Instance;
    
    // Configuration
    protected int m_iMaxTeamKillsBeforeBan = 3;
    protected float m_fTeamKillHistoryDuration = 86400.0; // 24 hours
    protected float m_fAutoKickThreshold = 3; // 3 TKs in a session
    protected bool m_bEnableAutoBan = true;
    
    // Records of team kills
    protected ref array<ref TeamKillRecord> m_aTeamKillRecords = new array<ref TeamKillRecord>();
    
    // Cache of team kills per player
    protected ref map<int, int> m_mPlayerTeamKillCount = new map<int, int>();
    
    // Path for saving team kill data
    protected const string TEAMKILL_LOG_PATH = "$profile:StatTracker/teamkills.json";
    
    // References to other components
    protected ref STS_NotificationManager m_NotificationManager;
    protected ref STS_WebhookManager m_WebhookManager;
    protected ref STS_Config m_Config;
    
    // Team kill entries
    protected ref array<ref TeamKillEntry> m_aTeamKillEntries = new array<ref TeamKillEntry>();
    
    // Player team kill counts
    protected ref map<int, int> m_mPlayerTeamKillCounts = new map<int, int>();
    
    // File path for saving team kill data
    protected const string TEAMKILL_DATA_PATH = "$profile:StatTracker/teamkills.json";
    
    // Configuration
    protected int m_iWarnThreshold = 2; // Warn after this many team kills
    protected int m_iKickThreshold = 3; // Kick after this many team kills
    protected int m_iBanThreshold = 5;  // Ban after this many team kills
    protected float m_fForgetTime = 3600.0; // Time in seconds after which team kills are "forgotten" (1 hour)
    
    // Reference to logging system
    protected ref STS_LoggingSystem m_Logger;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_TeamKillTracker()
    {
        // Get logger instance
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Logger.LogInfo("Initializing Team Kill Tracker");
        
        try
        {
            // Get references to other systems
            m_NotificationManager = STS_NotificationManager.GetInstance();
            m_WebhookManager = STS_WebhookManager.GetInstance();
            m_Config = STS_Config.GetInstance();
            
            // Load settings from config
            if (m_Config)
            {
                m_iMaxTeamKillsBeforeBan = m_Config.m_iMaxTeamKillsBeforeBan;
                m_fTeamKillHistoryDuration = m_Config.m_fTeamKillHistoryDuration;
                m_fAutoKickThreshold = m_Config.m_fAutoKickThreshold;
                m_bEnableAutoBan = m_Config.m_bEnableAutoBan;
            }
            
            // Load team kill history from file
            LoadTeamKillHistory();
            
            // Setup event listeners
            SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
            if (gameMode)
            {
                // Subscribe to player killed event
                SCR_KillManager killManager = SCR_KillManager.Instance();
                if (killManager)
                {
                    killManager.GetOnPlayerKilled().Insert(OnPlayerKilled);
                }
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception during TeamKillTracker initialization: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_TeamKillTracker GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_TeamKillTracker();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load team kill history from file
    protected void LoadTeamKillHistory()
    {
        // Clear existing records
        m_aTeamKillRecords.Clear();
        m_mPlayerTeamKillCount.Clear();
        
        // Check if file exists
        if (!FileIO.FileExists(TEAMKILL_LOG_PATH))
            return;
            
        // Read file
        string fileContent;
        if (!FileIO.FileReadAllText(TEAMKILL_LOG_PATH, fileContent))
            return;
            
        // Parse JSON array (simplified)
        fileContent = fileContent.Trim();
        if (!fileContent.StartsWith("[") || !fileContent.EndsWith("]"))
            return;
            
        // Remove brackets
        fileContent = fileContent.Substring(1, fileContent.Length() - 2);
        
        // Split by record separators
        array<string> recordStrings = new array<string>();
        int depth = 0;
        int startPos = 0;
        
        for (int i = 0; i < fileContent.Length(); i++)
        {
            string char = fileContent.Get(i);
            if (char == "{") depth++;
            else if (char == "}") depth--;
            
            // Record separator found
            if (depth == 0 && char == "}" && i < fileContent.Length() - 1)
            {
                recordStrings.Insert(fileContent.Substring(startPos, i - startPos + 1));
                startPos = i + 2; // Skip comma
            }
        }
        
        // Add the last record if there is one
        if (startPos < fileContent.Length())
        {
            recordStrings.Insert(fileContent.Substring(startPos));
        }
        
        // Current time to filter old records
        float currentTime = System.GetTickCount() / 1000.0;
        
        // Process each record
        foreach (string recordString : recordStrings)
        {
            TeamKillRecord record = TeamKillRecord.FromJSON(recordString);
            
            // Skip old records
            if (currentTime - record.m_fTimestamp > m_fTeamKillHistoryDuration)
                continue;
                
            // Add to records array
            m_aTeamKillRecords.Insert(record);
            
            // Update player count
            if (!m_mPlayerTeamKillCount.Contains(record.m_iKillerID))
                m_mPlayerTeamKillCount.Insert(record.m_iKillerID, 0);
                
            m_mPlayerTeamKillCount[record.m_iKillerID] = m_mPlayerTeamKillCount[record.m_iKillerID] + 1;
        }
        
        Print(string.Format("[StatTracker] Loaded %1 team kill records", m_aTeamKillRecords.Count()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Save team kill history to file
    void SaveTeamKillHistory()
    {
        string fileContent = "[";
        
        for (int i = 0; i < m_aTeamKillRecords.Count(); i++)
        {
            fileContent += m_aTeamKillRecords[i].ToJSON();
            
            if (i < m_aTeamKillRecords.Count() - 1)
                fileContent += ",";
        }
        
        fileContent += "]";
        
        // Ensure directory exists
        string dir = "$profile:StatTracker";
        FileIO.MakeDirectory(dir);
        
        // Write to file
        FileIO.FileWrite(TEAMKILL_LOG_PATH, fileContent);
        
        Print(string.Format("[StatTracker] Saved %1 team kill records", m_aTeamKillRecords.Count()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a player is killed
    protected void OnPlayerKilled(IEntity victim, IEntity killer, notnull Instigator instigator)
    {
        // Only process on server
        if (!Replication.IsServer())
            return;
            
        // Check if killer is valid
        if (!killer || !victim)
            return;
            
        // Get player controllers
        PlayerController killerController = PlayerController.Cast(killer.GetController());
        PlayerController victimController = PlayerController.Cast(victim.GetController());
        
        // Check if both are players
        if (!killerController || !victimController)
            return;
            
        // Check if they are on the same team
        int killerFactionId = GetPlayerFaction(killer);
        int victimFactionId = GetPlayerFaction(victim);
        
        if (killerFactionId != victimFactionId)
            return; // Not a team kill
            
        // Get player details
        int killerID = killerController.GetPlayerId();
        string killerName = killerController.GetPlayerName();
        int victimID = victimController.GetPlayerId();
        string victimName = victimController.GetPlayerName();
        
        // Get weapon used
        string weaponUsed = "Unknown";
        SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(victim.FindComponent(SCR_CharacterDamageManagerComponent));
        if (damageManager)
        {
            DamageHistory history = damageManager.GetDamageHistory();
            if (history && history.GetRecordsCount() > 0)
            {
                DamageHistoryRecord lastRecord = history.GetLatestRecord();
                if (lastRecord)
                {
                    weaponUsed = lastRecord.GetSource().ToString();
                }
            }
        }
        
        // Create team kill record
        TeamKillRecord record = new TeamKillRecord(killerID, killerName, victimID, victimName, victim.GetOrigin(), weaponUsed);
        m_aTeamKillRecords.Insert(record);
        
        // Update player team kill count
        if (!m_mPlayerTeamKillCount.Contains(killerID))
            m_mPlayerTeamKillCount.Insert(killerID, 0);
            
        m_mPlayerTeamKillCount[killerID] = m_mPlayerTeamKillCount[killerID] + 1;
        
        // Save team kill history
        SaveTeamKillHistory();
        
        // Send notification to all players
        if (m_NotificationManager)
        {
            string message = string.Format("%1 team killed %2", killerName, victimName);
            m_NotificationManager.BroadcastNotification(message, 5.0, COLOR_RED);
            m_NotificationManager.SendAdminNotification("Team Kill Alert: " + message);
        }
        
        // Send webhook notification
        if (m_WebhookManager && m_Config.m_bEnableWebhooks)
        {
            string payload = string.Format("Team Kill Detected: %1 killed %2 using %3", killerName, victimName, weaponUsed);
            m_WebhookManager.SendWebhook("teamkill", payload);
        }
        
        // Check if auto-kick/ban should be applied
        if (m_bEnableAutoBan && m_mPlayerTeamKillCount[killerID] >= m_iMaxTeamKillsBeforeBan)
        {
            // Ban player
            GetGame().GetBackendApi().BanPlayer(killerID, "Excessive team killing", 86400); // 24-hour ban
            Print(string.Format("[StatTracker] Player %1 (ID: %2) banned for excessive team killing", killerName, killerID));
            
            if (m_NotificationManager)
            {
                string banMessage = string.Format("%1 has been banned for excessive team killing", killerName);
                m_NotificationManager.BroadcastNotification(banMessage, 10.0, COLOR_RED);
            }
        }
        else if (m_mPlayerTeamKillCount[killerID] >= m_fAutoKickThreshold)
        {
            // Kick player
            GetGame().GetBackendApi().KickPlayer(killerID, "Excessive team killing");
            Print(string.Format("[StatTracker] Player %1 (ID: %2) kicked for excessive team killing", killerName, killerID));
            
            if (m_NotificationManager)
            {
                string kickMessage = string.Format("%1 has been kicked for excessive team killing", killerName);
                m_NotificationManager.BroadcastNotification(kickMessage, 10.0, COLOR_RED);
            }
        }
        else
        {
            // Warn player
            if (m_NotificationManager)
            {
                string warningMessage = string.Format("Warning: You have %1 team kills. %2 will result in a kick/ban.", m_mPlayerTeamKillCount[killerID], m_fAutoKickThreshold);
                m_NotificationManager.SendPlayerNotification(killerID, warningMessage, 15.0, COLOR_RED);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get player faction ID
    protected int GetPlayerFaction(IEntity player)
    {
        if (!player)
            return -1;
            
        FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(player.FindComponent(FactionAffiliationComponent));
        if (!factionComponent)
            return -1;
            
        Faction faction = factionComponent.GetAffiliatedFaction();
        if (!faction)
            return -1;
            
        return faction.GetFactionKey().GetID();
    }
    
    //------------------------------------------------------------------------------------------------
    // Get team kill count for a player
    int GetPlayerTeamKillCount(int playerID)
    {
        if (!m_mPlayerTeamKillCount.Contains(playerID))
            return 0;
            
        return m_mPlayerTeamKillCount[playerID];
    }
    
    //------------------------------------------------------------------------------------------------
    // Get team kill records for admin review
    array<ref TeamKillRecord> GetTeamKillRecords()
    {
        return m_aTeamKillRecords;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get recent team kills for a specific player
    array<ref TeamKillRecord> GetPlayerTeamKillRecords(int playerID)
    {
        array<ref TeamKillRecord> playerRecords = new array<ref TeamKillRecord>();
        
        foreach (TeamKillRecord record : m_aTeamKillRecords)
        {
            if (record.m_iKillerID == playerID)
            {
                playerRecords.Insert(record);
            }
        }
        
        return playerRecords;
    }
    
    //------------------------------------------------------------------------------------------------
    // Clear a player's team kill history (admin function)
    void ClearPlayerTeamKillHistory(int playerID)
    {
        // Remove all records for this player
        for (int i = m_aTeamKillRecords.Count() - 1; i >= 0; i--)
        {
            if (m_aTeamKillRecords[i].m_iKillerID == playerID)
            {
                m_aTeamKillRecords.Remove(i);
            }
        }
        
        // Reset player's team kill count
        if (m_mPlayerTeamKillCount.Contains(playerID))
        {
            m_mPlayerTeamKillCount[playerID] = 0;
        }
        
        // Save changes
        SaveTeamKillHistory();
    }

    //------------------------------------------------------------------------------------------------
    // New method: Report a team kill with comprehensive error handling
    void ReportTeamKill(int killerID, string killerName, int victimID, string victimName, vector position, string weapon, int killerFaction, int victimFaction)
    {
        STS_LoggingSystem logger = STS_LoggingSystem.GetInstance();
        
        try
        {
            // Validate input parameters
            if (killerID < 0 || victimID < 0)
            {
                logger.LogWarning(string.Format("Invalid player IDs in team kill report: killer=%1, victim=%2", 
                    killerID, victimID), "STS_TeamKillTracker", "ReportTeamKill");
                return;
            }
            
            if (killerName.IsEmpty())
            {
                logger.LogWarning("Empty killer name in team kill report - using 'Unknown'", 
                    "STS_TeamKillTracker", "ReportTeamKill");
                killerName = "Unknown";
            }
            
            if (victimName.IsEmpty())
            {
                logger.LogWarning("Empty victim name in team kill report - using 'Unknown'", 
                    "STS_TeamKillTracker", "ReportTeamKill");
                victimName = "Unknown";
            }
            
            if (weapon.IsEmpty())
            {
                logger.LogWarning("Empty weapon name in team kill report - using 'Unknown Weapon'", 
                    "STS_TeamKillTracker", "ReportTeamKill");
                weapon = "Unknown Weapon";
            }
            
            // Convert player IDs to unique identifier strings
            string killerUID = GetPlayerUID(killerID);
            string victimUID = GetPlayerUID(victimID);
            
            if (killerUID.IsEmpty())
            {
                logger.LogWarning(string.Format("Could not get UID for killer (ID: %1) - using fallback ID", killerID), 
                    "STS_TeamKillTracker", "ReportTeamKill");
                killerUID = "player_" + killerID.ToString();
            }
            
            if (victimUID.IsEmpty())
            {
                logger.LogWarning(string.Format("Could not get UID for victim (ID: %1) - using fallback ID", victimID), 
                    "STS_TeamKillTracker", "ReportTeamKill");
                victimUID = "player_" + victimID.ToString();
            }
            
            // Get current timestamp
            float timestamp = System.GetTickCount() / 1000.0;
            
            // Create team kill event
            STS_TeamKillEvent teamKillEvent = new STS_TeamKillEvent(killerUID, killerName, victimUID, victimName, 
                timestamp, position, weapon, killerFaction, victimFaction);
            
            // Log the event
            logger.LogInfo(string.Format("Team Kill: %1 (ID: %2, Team: %3) killed %4 (ID: %5, Team: %6) with %7", 
                killerName, killerID, killerFaction, victimName, victimID, victimFaction, weapon), 
                "STS_TeamKillTracker", "ReportTeamKill");
            
            // Get or create the team kill record for the killer
            STS_PlayerTKRecord killerRecord = GetPlayerTKRecord(killerUID, killerName);
            
            if (!killerRecord)
            {
                logger.LogError(string.Format("Failed to create TK record for player %1 (UID: %2)", 
                    killerName, killerUID), "STS_TeamKillTracker", "ReportTeamKill");
                return;
            }
            
            // Add the team kill to the killer's record
            killerRecord.AddTeamKill(teamKillEvent);
            
            // Get or create the team kill record for the victim
            STS_PlayerTKRecord victimRecord = GetPlayerTKRecord(victimUID, victimName);
            
            if (victimRecord)
            {
                // Add the team death to the victim's record
                victimRecord.AddTeamDeath(teamKillEvent);
            }
            
            // Check if we need to take action against the killer
            if (m_Config && m_Config.m_bEnableAutoPunishment)
            {
                ETeamKillSeverity severity = killerRecord.GetSeverityLevel();
                
                // Apply punishment based on severity
                switch (severity)
                {
                    case ETeamKillSeverity.Low:
                        // Just a warning for now
                        if (killerRecord.m_iTKPoints >= m_Config.m_iWarnTKPoints && killerRecord.m_iWarnings == 0)
                        {
                            IssueTKWarning(killerID, killerName, victimName);
                            killerRecord.m_iWarnings++;
                            
                            logger.LogInfo(string.Format("Issued TK warning to %1 (first offense)", killerName), 
                                "STS_TeamKillTracker", "ReportTeamKill");
                        }
                        break;
                        
                    case ETeamKillSeverity.Medium:
                        // Issue another warning or kick based on config
                        if (killerRecord.m_iWarnings < m_Config.m_iMaxWarnings)
                        {
                            IssueTKWarning(killerID, killerName, victimName);
                            killerRecord.m_iWarnings++;
                            
                            logger.LogInfo(string.Format("Issued TK warning to %1 (warning #%2)", 
                                killerName, killerRecord.m_iWarnings), "STS_TeamKillTracker", "ReportTeamKill");
                        }
                        else if (killerRecord.m_iKicks < m_Config.m_iMaxKicks)
                        {
                            KickPlayerForTK(killerID, killerName);
                            killerRecord.m_iKicks++;
                            
                            logger.LogWarning(string.Format("Kicked %1 for team killing (kick #%2)", 
                                killerName, killerRecord.m_iKicks), "STS_TeamKillTracker", "ReportTeamKill");
                        }
                        break;
                        
                    case ETeamKillSeverity.High:
                    case ETeamKillSeverity.Critical:
                        // Kick or ban based on config
                        if (killerRecord.m_iKicks < m_Config.m_iMaxKicks)
                        {
                            KickPlayerForTK(killerID, killerName);
                            killerRecord.m_iKicks++;
                            
                            logger.LogWarning(string.Format("Kicked %1 for excessive team killing (kick #%2)", 
                                killerName, killerRecord.m_iKicks), "STS_TeamKillTracker", "ReportTeamKill");
                        }
                        else if (!killerRecord.m_bIsBanned && severity == ETeamKillSeverity.Critical)
                        {
                            BanPlayerForTK(killerID, killerName);
                            killerRecord.m_iBans++;
                            killerRecord.m_bIsBanned = true;
                            
                            logger.LogWarning(string.Format("Banned %1 for repeated team killing", killerName), 
                                "STS_TeamKillTracker", "ReportTeamKill");
                        }
                        break;
                }
            }
            
            // Send webhooks if enabled
            if (m_Config && m_Config.m_bEnableWebhooks)
            {
                SendTeamKillWebhook(teamKillEvent);
            }
            
            // Save updated team kill data
            SaveTeamKills();
            
            // Notify all players if enabled
            if (m_Config && m_Config.m_bAnnounceTeamKills)
            {
                BroadcastTeamKillMessage(killerName, victimName, weapon);
            }
        }
        catch (Exception e)
        {
            if (logger)
            {
                logger.LogError(string.Format("Exception in ReportTeamKill: %1\nStackTrace: %2", 
                    e.ToString(), e.GetStackTrace()), "STS_TeamKillTracker", "ReportTeamKill");
            }
            else
            {
                Print(string.Format("[StatTracker] CRITICAL ERROR in ReportTeamKill: %1", e.ToString()), LogLevel.ERROR);
            }
        }
    }

    //------------------------------------------------------------------------------------------------
    // Get or create a player TK record
    protected STS_PlayerTKRecord GetPlayerTKRecord(string playerUID, string playerName)
    {
        try
        {
            // Check if we already have a record for this player
            if (m_mPlayerTKRecords.Contains(playerUID))
            {
                STS_PlayerTKRecord record = m_mPlayerTKRecords.Get(playerUID);
                
                // Update player name in case it changed
                if (record.m_sPlayerName != playerName && !playerName.IsEmpty())
                {
                    record.m_sPlayerName = playerName;
                }
                
                return record;
            }
            
            // Create a new record
            STS_PlayerTKRecord newRecord = new STS_PlayerTKRecord(playerUID, playerName);
            m_mPlayerTKRecords.Set(playerUID, newRecord);
            
            STS_LoggingSystem.GetInstance().LogDebug(string.Format("Created new TK record for player %1 (UID: %2)", 
                playerName, playerUID), "STS_TeamKillTracker", "GetPlayerTKRecord");
                
            return newRecord;
        }
        catch (Exception e)
        {
            STS_LoggingSystem.GetInstance().LogError(string.Format("Exception in GetPlayerTKRecord: %1", e.ToString()), 
                "STS_TeamKillTracker", "GetPlayerTKRecord");
            return null;
        }
    }
} 
} 