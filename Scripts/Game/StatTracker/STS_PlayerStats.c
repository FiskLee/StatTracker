// STS_PlayerStats.c
// Player statistics data class

class STS_PlayerStats
{
    // Basic statistics
    int m_iKills;
    int m_iDeaths;
    int m_iBasesLost;
    int m_iBasesCaptured;
    int m_iTotalXP;
    int m_iRank;
    int m_iSuppliesDelivered;
    int m_iSupplyDeliveryCount;
    int m_iAIKills;
    int m_iVehicleKills;
    int m_iAirKills;
    
    // Player information
    string m_sPlayerName;
    int m_iPlayerID;
    int m_iTeamID = -1;
    
    // Connection info
    string m_sIPAddress;
    float m_fConnectionTime;
    float m_fLastSessionDuration;
    float m_fTotalPlaytime;
    
    // Tracking who killed this player and with what
    ref array<string> m_aKilledBy = new array<string>();
    ref array<string> m_aKilledByWeapon = new array<string>();
    ref array<int> m_aKilledByTeam = new array<int>();
    
    // Reference to logging system
    protected static ref STS_LoggingSystem m_Logger;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_PlayerStats()
    {
        // Get logger instance
        if (!m_Logger)
        {
            m_Logger = STS_LoggingSystem.GetInstance();
        }
        
        // Initialize all stats to 0
        m_iKills = 0;
        m_iDeaths = 0;
        m_iBasesLost = 0;
        m_iBasesCaptured = 0;
        m_iTotalXP = 0;
        m_iRank = 0;
        m_iSuppliesDelivered = 0;
        m_iSupplyDeliveryCount = 0;
        m_iAIKills = 0;
        m_iVehicleKills = 0;
        m_iAirKills = 0;
        
        // Initialize player info
        m_sPlayerName = "";
        m_iPlayerID = -1;
        m_iTeamID = -1;
        
        // Initialize connection info
        m_sIPAddress = "";
        m_fConnectionTime = 0;
        m_fLastSessionDuration = 0;
        m_fTotalPlaytime = 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Calculate the total score based on weighted values
    int GetScore()
    {
        return m_iKills * 10 + 
               m_iBasesCaptured * 50 + 
               m_iSuppliesDelivered + 
               m_iAIKills * 5 + 
               m_iVehicleKills * 20 + 
               m_iAirKills * 30;
    }
    
    //------------------------------------------------------------------------------------------------
    // Update session duration
    void UpdateSessionDuration()
    {
        try
        {
            if (m_fConnectionTime > 0)
            {
                m_fLastSessionDuration = System.GetTickCount() / 1000.0 - m_fConnectionTime;
                m_fTotalPlaytime += m_fLastSessionDuration;
                
                if (m_Logger)
                {
                    m_Logger.LogDebug(string.Format("Updated session duration for player %1 (ID: %2): Session=%3s, Total=%4s", 
                        m_sPlayerName, m_iPlayerID, m_fLastSessionDuration.ToString(1), m_fTotalPlaytime.ToString(1)), 
                        "STS_PlayerStats", "UpdateSessionDuration");
                }
            }
        }
        catch (Exception e)
        {
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception updating session duration for player %1 (ID: %2): %3", 
                    m_sPlayerName, m_iPlayerID, e.ToString()), "STS_PlayerStats", "UpdateSessionDuration");
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Convert to JSON string representation with error handling
    string ToJSON()
    {
        try
        {
            JsonSerializer serializer = new JsonSerializer();
            string json;
            if (!serializer.WriteToString(this, false, json))
            {
                Print("[StatTracker] ERROR: Failed to serialize player stats to JSON", LogLevel.ERROR);
                return GenerateFallbackJSON();
            }
            return json;
        }
        catch (Exception e)
        {
            Print(string.Format("[StatTracker] ERROR: Exception in ToJSON: %1", e.ToString()), LogLevel.ERROR);
            return GenerateFallbackJSON();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a basic JSON representation as fallback
    protected string GenerateFallbackJSON()
    {
        try
        {
            // Create a minimal JSON representation manually
            string json = "{";
            json += "\"kills\":" + m_iKills.ToString() + ",";
            json += "\"deaths\":" + m_iDeaths.ToString() + ",";
            json += "\"basesLost\":" + m_iBasesLost.ToString() + ",";
            json += "\"basesCaptured\":" + m_iBasesCaptured.ToString() + ",";
            json += "\"totalXP\":" + m_iTotalXP.ToString() + ",";
            json += "\"rank\":" + m_iRank.ToString() + ",";
            json += "\"suppliesDelivered\":" + m_iSuppliesDelivered.ToString() + ",";
            json += "\"supplyDeliveryCount\":" + m_iSupplyDeliveryCount.ToString() + ",";
            json += "\"aiKills\":" + m_iAIKills.ToString() + ",";
            json += "\"vehicleKills\":" + m_iVehicleKills.ToString() + ",";
            json += "\"airKills\":" + m_iAirKills.ToString() + ",";
            json += "\"ipAddress\":\"" + (m_sIPAddress ? m_sIPAddress : "") + "\",";
            json += "\"connectionTime\":" + m_fConnectionTime.ToString() + ",";
            json += "\"lastSessionDuration\":" + m_fLastSessionDuration.ToString() + ",";
            json += "\"totalPlaytime\":" + m_fTotalPlaytime.ToString();
            json += "}";
            return json;
        }
        catch (Exception e)
        {
            Print(string.Format("[StatTracker] CRITICAL ERROR in GenerateFallbackJSON: %1", e.ToString()), LogLevel.ERROR);
            return "{}"; // Return empty JSON as last resort
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load stats from JSON string with error handling
    bool FromJSON(string json)
    {
        if (json.IsEmpty())
        {
            Print("[StatTracker] WARNING: Attempted to parse empty JSON string", LogLevel.WARNING);
            return false;
        }
        
        try
        {
            // Use JSON serializer to parse the string into this object
            JsonSerializer serializer = new JsonSerializer();
            if (!serializer.ReadFromString(this, json))
            {
                Print("[StatTracker] ERROR: Failed to deserialize player stats from JSON", LogLevel.ERROR);
                return false;
            }
            
            // Validate and fix any invalid/corrupted values
            ValidateStats();
            
            return true;
        }
        catch (Exception e)
        {
            Print(string.Format("[StatTracker] ERROR: Exception in FromJSON: %1", e.ToString()), LogLevel.ERROR);
            
            // Attempt fallback manual parsing for critical fields
            return FromJSONFallback(json);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Manual fallback JSON parsing for critical stats when serializer fails
    bool FromJSONFallback(string json)
    {
        try
        {
            Print("[StatTracker] Attempting fallback JSON parsing", LogLevel.WARNING);
            
            // Extract values using simple string parsing
            // This is a simplified approach - a real implementation would use a more robust parser
            
            // Extract kills
            TryExtractIntValue(json, "\"kills\":", m_iKills);
            
            // Extract deaths
            TryExtractIntValue(json, "\"deaths\":", m_iDeaths);
            
            // Extract bases captured
            TryExtractIntValue(json, "\"basesCaptured\":", m_iBasesCaptured);
            
            // Extract total XP
            TryExtractIntValue(json, "\"totalXP\":", m_iTotalXP);
            
            // Validate and fix any invalid values
            ValidateStats();
            
            Print("[StatTracker] Fallback JSON parsing partially succeeded", LogLevel.WARNING);
            return true;
        }
        catch (Exception e)
        {
            Print(string.Format("[StatTracker] CRITICAL ERROR in FromJSONFallback: %1", e.ToString()), LogLevel.ERROR);
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Utility method to extract an integer value from JSON string
    protected bool TryExtractIntValue(string json, string key, out int value)
    {
        value = 0; // Default value
        
        int keyIndex = json.IndexOf(key);
        if (keyIndex == -1)
            return false;
        
        // Find value after key
        int valueStart = keyIndex + key.Length();
        int valueEnd = json.IndexOfFrom(",", valueStart);
        if (valueEnd == -1)
            valueEnd = json.IndexOfFrom("}", valueStart);
        
        if (valueEnd == -1)
            return false;
        
        // Extract the value string
        string valueStr = json.Substring(valueStart, valueEnd - valueStart);
        valueStr.Trim();
        
        // Parse integer
        value = valueStr.ToInt();
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Validate and fix any invalid stats values
    void ValidateStats()
    {
        // Ensure all numeric values are non-negative
        m_iKills = Math.Max(0, m_iKills);
        m_iDeaths = Math.Max(0, m_iDeaths);
        m_iBasesLost = Math.Max(0, m_iBasesLost);
        m_iBasesCaptured = Math.Max(0, m_iBasesCaptured);
        m_iTotalXP = Math.Max(0, m_iTotalXP);
        m_iRank = Math.Max(0, m_iRank);
        m_iSuppliesDelivered = Math.Max(0, m_iSuppliesDelivered);
        m_iSupplyDeliveryCount = Math.Max(0, m_iSupplyDeliveryCount);
        m_iAIKills = Math.Max(0, m_iAIKills);
        m_iVehicleKills = Math.Max(0, m_iVehicleKills);
        m_iAirKills = Math.Max(0, m_iAirKills);
        
        // Ensure time values are reasonable
        m_fTotalPlaytime = Math.Max(0, m_fTotalPlaytime);
        m_fLastSessionDuration = Math.Max(0, m_fLastSessionDuration);
        
        // Cap values at reasonable maximums to prevent overflow/corruption
        m_iTotalXP = Math.Min(m_iTotalXP, 1000000000); // Cap at 1 billion
        m_iKills = Math.Min(m_iKills, 1000000); // Cap at 1 million
        m_fTotalPlaytime = Math.Min(m_fTotalPlaytime, 3600 * 24 * 365 * 10); // Cap at 10 years (in seconds)
        
        // Initialize arrays if null
        if (!m_aKilledBy)
            m_aKilledBy = new array<string>();
        
        if (!m_aKilledByWeapon)
            m_aKilledByWeapon = new array<string>();
        
        if (!m_aKilledByTeam)
            m_aKilledByTeam = new array<int>();
        
        // Ensure arrays have same length
        int minCount = Math.Min(m_aKilledBy.Count(), m_aKilledByWeapon.Count());
        minCount = Math.Min(minCount, m_aKilledByTeam.Count());
        
        if (m_aKilledBy.Count() > minCount)
            m_aKilledBy.Resize(minCount);
        
        if (m_aKilledByWeapon.Count() > minCount)
            m_aKilledByWeapon.Resize(minCount);
        
        if (m_aKilledByTeam.Count() > minCount)
            m_aKilledByTeam.Resize(minCount);
        
        // Update rank based on XP
        UpdateRank();
    }
    
    //------------------------------------------------------------------------------------------------
    // Track who killed this player and with what weapon
    void AddKillInfo(string killerName, string weaponName, int teamID)
    {
        try
        {
            m_aKilledBy.Insert(killerName);
            m_aKilledByWeapon.Insert(weaponName);
            m_aKilledByTeam.Insert(teamID);
            
            if (m_Logger)
            {
                m_Logger.LogDebug(string.Format("Added kill info for player %1 (ID: %2): killed by %3 with %4 (Team: %5)", 
                    m_sPlayerName, m_iPlayerID, killerName, weaponName, teamID), 
                    "STS_PlayerStats", "AddKillInfo");
            }
        }
        catch (Exception e)
        {
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception adding kill info for player %1 (ID: %2): %3", 
                    m_sPlayerName, m_iPlayerID, e.ToString()), "STS_PlayerStats", "AddKillInfo");
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Getters and setters
    
    int GetKills() { return m_iKills; }
    void SetKills(int value) { m_iKills = value; }
    
    int GetDeaths() { return m_iDeaths; }
    void SetDeaths(int value) { m_iDeaths = value; }
    
    int GetBasesCaptured() { return m_iBasesCaptured; }
    void SetBasesCaptured(int value) { m_iBasesCaptured = value; }
    
    int GetBasesLost() { return m_iBasesLost; }
    void SetBasesLost(int value) { m_iBasesLost = value; }
    
    int GetTotalXP() { return m_iTotalXP; }
    void SetTotalXP(int value) { m_iTotalXP = value; }
    
    int GetRank() { return m_iRank; }
    void SetRank(int value) { m_iRank = value; }
    
    int GetAIKills() { return m_iAIKills; }
    void SetAIKills(int value) { m_iAIKills = value; }
    
    int GetVehicleKills() { return m_iVehicleKills; }
    void SetVehicleKills(int value) { m_iVehicleKills = value; }
    
    int GetAirKills() { return m_iAirKills; }
    void SetAirKills(int value) { m_iAirKills = value; }
    
    string GetPlayerName() { return m_sPlayerName; }
    void SetPlayerName(string value) { m_sPlayerName = value; }
    
    int GetPlayerID() { return m_iPlayerID; }
    void SetPlayerID(int value) { m_iPlayerID = value; }
    
    int GetTeamId() { return m_iTeamID; }
    void SetTeamId(int value) { m_iTeamID = value; }
    
    float GetTotalPlaytime() { return m_fTotalPlaytime; }
    float GetSessionDuration() { return m_fLastSessionDuration; }
    
    string GetIPAddress() { return m_sIPAddress; }
    void SetIPAddress(string value) { m_sIPAddress = value; }
} 