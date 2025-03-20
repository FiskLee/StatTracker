// STS_PlayerStatsEntity.c
// Entity class for player statistics with validation and sanitization

[EDF_DbEntryName("PlayerStats")]
class STS_PlayerStatsEntity : EDF_DbEntity
{
    // Player identity
    [EDF_DbProperty("PlayerUID")]
    string m_sPlayerUID;

    [EDF_DbProperty("PlayerName")]
    string m_sPlayerName;
    
    [EDF_DbProperty("IPAddress")]
    string m_sIPAddress;

    // Core stats
    [EDF_DbProperty("Kills")]
    int m_iKills;

    [EDF_DbProperty("Deaths")]
    int m_iDeaths;

    [EDF_DbProperty("ObjectivesCaptured")]
    int m_iObjectivesCaptured;

    [EDF_DbProperty("ObjectivesLost")]
    int m_iObjectivesLost;

    [EDF_DbProperty("TotalScore")]
    int m_iTotalScore;

    [EDF_DbProperty("SuppliesDelivered")]
    int m_iSuppliesDelivered;

    [EDF_DbProperty("AIKills")]
    int m_iAIKills;

    [EDF_DbProperty("VehicleKills")]
    int m_iVehicleKills;

    // Connection stats
    [EDF_DbProperty("FirstJoined")]
    int m_iFirstJoined;

    [EDF_DbProperty("LastSeen")]
    int m_iLastSeen;

    [EDF_DbProperty("TotalPlaytime")]
    int m_iTotalPlaytime;

    // Complex data (stored as JSON)
    [EDF_DbProperty("WeaponKills")]
    string m_sWeaponKillsJson;

    [EDF_DbProperty("KilledBy")]
    string m_sKilledByJson;

    [EDF_DbProperty("ShotsFired")]
    int m_iShotsFired;

    [EDF_DbProperty("ShotsHit")]
    int m_iShotsHit;

    [EDF_DbProperty("TeamKillCount")]
    int m_iTeamKillCount;

    [EDF_DbProperty("DistanceTraveled")]
    float m_fDistanceTraveled;

    [EDF_DbProperty("HeadshotCount")]
    int m_iHeadshotCount;

    [EDF_DbProperty("LongestKill")]
    float m_fLongestKill;

    [EDF_DbProperty("HighestKillstreak")]
    int m_iHighestKillstreak;

    [EDF_DbProperty("CurrentKillstreak")]
    int m_iCurrentKillstreak;

    [EDF_DbProperty("KillHistory")]
    string m_sKillHistoryJson;
    
    // Validation and sanitization
    protected ref STS_LoggingSystem m_Logger;
    static const int MAX_NAME_LENGTH = 64;
    static const int MAX_JSON_LENGTH = 10000;
    static const int MAX_IP_LENGTH = 45; // IPv6 length
    
    //------------------------------------------------------------------------------------------------
    void STS_PlayerStatsEntity()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
    }
    
    //------------------------------------------------------------------------------------------------
    // Set player information with validation
    void SetPlayerInfo(string playerUID, string playerName, string ipAddress)
    {
        // Validate player UID
        if (!ValidatePlayerUID(playerUID))
        {
            if (m_Logger) 
                m_Logger.LogWarning(string.Format("Invalid player UID: '%1', using sanitized version", playerUID), "STS_PlayerStatsEntity", "SetPlayerInfo");
            
            m_sPlayerUID = SanitizePlayerUID(playerUID);
        }
        else
        {
            m_sPlayerUID = playerUID;
        }
        
        // Validate player name
        if (!ValidatePlayerName(playerName))
        {
            if (m_Logger)
                m_Logger.LogWarning(string.Format("Invalid player name: '%1', using sanitized version", playerName), "STS_PlayerStatsEntity", "SetPlayerInfo");
            
            m_sPlayerName = SanitizePlayerName(playerName);
        }
        else
        {
            m_sPlayerName = playerName;
        }
        
        // Validate IP address
        if (!ValidateIPAddress(ipAddress))
        {
            if (m_Logger)
                m_Logger.LogWarning(string.Format("Invalid IP address: '%1', using sanitized version", ipAddress), "STS_PlayerStatsEntity", "SetPlayerInfo");
            
            m_sIPAddress = SanitizeIPAddress(ipAddress);
        }
        else
        {
            m_sIPAddress = ipAddress;
        }
        
        // If we had to perform any sanitization and have no logger, print a warning
        if (!m_Logger && (m_sPlayerUID != playerUID || m_sPlayerName != playerName || m_sIPAddress != ipAddress))
        {
            Print(string.Format("[StatTracker] Warning: One or more player identifiers required sanitization for player: %1", m_sPlayerName));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Set weapon kills with validation
    void SetWeaponKills(map<string, int> weaponKills)
    {
        // Sanitize weapon names
        map<string, int> sanitizedMap = new map<string, int>();
        
        array<string> keys = new array<string>();
        weaponKills.GetKeyArray(keys);
        
        foreach (string key : keys)
        {
            string sanitizedKey = SanitizeString(key);
            sanitizedMap.Insert(sanitizedKey, weaponKills.Get(key));
        }
        
        // Convert to JSON
        string json = SerializeMapToJson(sanitizedMap);
        
        // Check JSON length
        if (json.Length() > MAX_JSON_LENGTH)
        {
            // Truncate by removing entries until under limit
            while (json.Length() > MAX_JSON_LENGTH && sanitizedMap.Count() > 0)
            {
                // Find the smallest value to remove
                string keyToRemove = "";
                int minValue = 999999;
                
                array<string> remainingKeys = new array<string>();
                sanitizedMap.GetKeyArray(remainingKeys);
                
                foreach (string k : remainingKeys)
                {
                    int val = sanitizedMap.Get(k);
                    if (val < minValue)
                    {
                        minValue = val;
                        keyToRemove = k;
                    }
                }
                
                if (keyToRemove != "")
                {
                    sanitizedMap.Remove(keyToRemove);
                    json = SerializeMapToJson(sanitizedMap);
                }
                else
                {
                    break;
                }
            }
            
            if (m_Logger)
                m_Logger.LogWarning(string.Format("Weapon kills JSON was too large (%1 bytes), truncated to %2 entries", json.Length(), sanitizedMap.Count()), "STS_PlayerStatsEntity", "SetWeaponKills");
        }
        
        m_sWeaponKillsJson = json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Set killed by with validation
    void SetKilledBy(map<string, int> killedBy)
    {
        // Sanitize player names
        map<string, int> sanitizedMap = new map<string, int>();
        
        array<string> keys = new array<string>();
        killedBy.GetKeyArray(keys);
        
        foreach (string key : keys)
        {
            string sanitizedKey = SanitizePlayerName(key);
            sanitizedMap.Insert(sanitizedKey, killedBy.Get(key));
        }
        
        // Convert to JSON
        string json = SerializeMapToJson(sanitizedMap);
        
        // Check JSON length
        if (json.Length() > MAX_JSON_LENGTH)
        {
            // Truncate by removing entries until under limit
            while (json.Length() > MAX_JSON_LENGTH && sanitizedMap.Count() > 0)
            {
                // Find the smallest value to remove
                string keyToRemove = "";
                int minValue = 999999;
                
                array<string> remainingKeys = new array<string>();
                sanitizedMap.GetKeyArray(remainingKeys);
                
                foreach (string k : remainingKeys)
                {
                    int val = sanitizedMap.Get(k);
                    if (val < minValue)
                    {
                        minValue = val;
                        keyToRemove = k;
                    }
                }
                
                if (keyToRemove != "")
                {
                    sanitizedMap.Remove(keyToRemove);
                    json = SerializeMapToJson(sanitizedMap);
                }
                else
                {
                    break;
                }
            }
            
            if (m_Logger)
                m_Logger.LogWarning(string.Format("Killed by JSON was too large (%1 bytes), truncated to %2 entries", json.Length(), sanitizedMap.Count()), "STS_PlayerStatsEntity", "SetKilledBy");
        }
        
        m_sKilledByJson = json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Set kill history with validation
    void SetKillHistory(array<ref STS_KillRecord> killHistory)
    {
        // Sanitize kill history
        array<ref STS_KillRecord> sanitizedArray = new array<ref STS_KillRecord>();
        
        foreach (STS_KillRecord record : killHistory)
        {
            // Create sanitized copy of record
            STS_KillRecord sanitizedRecord = new STS_KillRecord();
            sanitizedRecord.m_sKillerName = SanitizePlayerName(record.m_sKillerName);
            sanitizedRecord.m_sVictimName = SanitizePlayerName(record.m_sVictimName);
            sanitizedRecord.m_sWeapon = SanitizeString(record.m_sWeapon);
            sanitizedRecord.m_iTimestamp = record.m_iTimestamp;
            sanitizedRecord.m_fDistance = record.m_fDistance;
            sanitizedRecord.m_bHeadshot = record.m_bHeadshot;
            
            sanitizedArray.Insert(sanitizedRecord);
        }
        
        // Convert to JSON
        string json = "";
        
        // Serialize array
        json = "{\"records\":[";
        for (int i = 0; i < sanitizedArray.Count(); i++)
        {
            json += sanitizedArray[i].ToJSON();
            if (i < sanitizedArray.Count() - 1)
                json += ",";
        }
        json += "]}";
        
        // Check JSON length
        if (json.Length() > MAX_JSON_LENGTH)
        {
            // Truncate by taking only the most recent entries
            while (json.Length() > MAX_JSON_LENGTH && sanitizedArray.Count() > 0)
            {
                // Remove oldest record
                sanitizedArray.RemoveOrdered(0);
                
                // Regenerate JSON
                json = "{\"records\":[";
                for (int i = 0; i < sanitizedArray.Count(); i++)
                {
                    json += sanitizedArray[i].ToJSON();
                    if (i < sanitizedArray.Count() - 1)
                        json += ",";
                }
                json += "]}";
            }
            
            if (m_Logger)
                m_Logger.LogWarning(string.Format("Kill history JSON was too large (%1 bytes), truncated to %2 entries", json.Length(), sanitizedArray.Count()), "STS_PlayerStatsEntity", "SetKillHistory");
        }
        
        m_sKillHistoryJson = json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get weapon kills as map
    map<string, int> GetWeaponKills()
    {
        map<string, int> result = new map<string, int>();
        
        if (m_sWeaponKillsJson.Length() > 0)
        {
            DeserializeJsonToMap(m_sWeaponKillsJson, result);
        }
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get killed by as map
    map<string, int> GetKilledBy()
    {
        map<string, int> result = new map<string, int>();
        
        if (m_sKilledByJson.Length() > 0)
        {
            DeserializeJsonToMap(m_sKilledByJson, result);
        }
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get kill history as array
    array<ref STS_KillRecord> GetKillHistory()
    {
        array<ref STS_KillRecord> results = new array<ref STS_KillRecord>();
        
        if (m_sKillHistoryJson.Length() == 0)
            return results;
            
        // Basic JSON parsing (in a real implementation, use a proper JSON parser)
        // Looking for pattern: {"records":[{...},{...}]}
        string recordsJson = m_sKillHistoryJson;
        
        // Find the records array
        int recordsStart = recordsJson.IndexOf("[");
        int recordsEnd = recordsJson.LastIndexOf("]");
        
        if (recordsStart == -1 || recordsEnd == -1)
            return results;
            
        // Extract the records array content
        string recordsArray = recordsJson.Substring(recordsStart + 1, recordsEnd - recordsStart - 1);
        
        // Split by commas that are followed by opening braces
        array<string> recordJsons = new array<string>();
        int startPos = 0;
        int braceLevel = 0;
        
        for (int i = 0; i < recordsArray.Length(); i++)
        {
            string c = recordsArray.Get(i);
            
            if (c == "{")
                braceLevel++;
            else if (c == "}")
            {
                braceLevel--;
                if (braceLevel == 0 && i < recordsArray.Length() - 1)
                {
                    // Extract this record
                    string recordJson = recordsArray.Substring(startPos, i - startPos + 1);
                    recordJsons.Insert(recordJson);
                    
                    // Skip the comma
                    i += 1;
                    startPos = i + 1;
                }
                else if (braceLevel == 0 && i == recordsArray.Length() - 1)
                {
                    // Last record
                    string recordJson = recordsArray.Substring(startPos, i - startPos + 1);
                    recordJsons.Insert(recordJson);
                }
            }
        }
        
        // Parse each record
        foreach (string recordJson : recordJsons)
        {
            STS_KillRecord record = STS_KillRecord.FromJSON(recordJson);
            if (record)
                results.Insert(record);
        }
        
        return results;
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper methods for validation and sanitization
    
    //------------------------------------------------------------------------------------------------
    // Validate player UID
    protected bool ValidatePlayerUID(string uid)
    {
        if (!uid || uid.Length() == 0)
            return false;
            
        // Check for invalid characters
        for (int i = 0; i < uid.Length(); i++)
        {
            int charCode = uid.Get(i);
            // Allow only alphanumeric, underscore, or dash
            if (!(
                (charCode >= 48 && charCode <= 57) || // 0-9
                (charCode >= 65 && charCode <= 90) || // A-Z
                (charCode >= 97 && charCode <= 122) || // a-z
                charCode == 95 || // _
                charCode == 45 // -
            ))
            {
                return false;
            }
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Validate player name
    protected bool ValidatePlayerName(string name)
    {
        if (!name)
            return false;
            
        if (name.Length() == 0 || name.Length() > MAX_NAME_LENGTH)
            return false;
        
        // Check for potentially dangerous characters
        for (int i = 0; i < name.Length(); i++)
        {
            int charCode = name.Get(i);
            // Disallow control characters and certain special characters
            if (charCode < 32 || charCode == 127 || 
                charCode == 60 || charCode == 62 || // < >
                charCode == 34 || charCode == 39 || // " '
                charCode == 92 || charCode == 96) // \ `
            {
                return false;
            }
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Validate IP address
    protected bool ValidateIPAddress(string ip)
    {
        if (!ip || ip.Length() == 0 || ip.Length() > MAX_IP_LENGTH)
            return false;
            
        // Very basic validation for both IPv4 and IPv6
        // For IPv4, we'd check for 4 octets of numbers 0-255 separated by dots
        // For IPv6, we'd check for 8 groups of hex digits separated by colons
        // This is a simplified version
        
        // Check for invalid characters
        for (int i = 0; i < ip.Length(); i++)
        {
            int charCode = ip.Get(i);
            // Allow only digits, a-f, A-F, dots, colons, and percent sign (for IPv6 scope)
            if (!(
                (charCode >= 48 && charCode <= 57) || // 0-9
                (charCode >= 65 && charCode <= 70) || // A-F
                (charCode >= 97 && charCode <= 102) || // a-f
                charCode == 46 || // .
                charCode == 58 || // :
                charCode == 37 // %
            ))
            {
                return false;
            }
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Sanitize player UID
    protected string SanitizePlayerUID(string uid)
    {
        if (!uid || uid.Length() == 0)
            return "unknown";
            
        string result = "";
        
        // Keep only valid characters
        for (int i = 0; i < uid.Length(); i++)
        {
            int charCode = uid.Get(i);
            // Allow only alphanumeric, underscore, or dash
            if (
                (charCode >= 48 && charCode <= 57) || // 0-9
                (charCode >= 65 && charCode <= 90) || // A-Z
                (charCode >= 97 && charCode <= 122) || // a-z
                charCode == 95 || // _
                charCode == 45 // -
            )
            {
                result += uid.Get(i);
            }
        }
        
        if (result.Length() == 0)
            return "unknown";
            
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Sanitize player name
    protected string SanitizePlayerName(string name)
    {
        if (!name || name.Length() == 0)
            return "Unknown";
            
        string result = "";
        
        // Keep only valid characters
        for (int i = 0; i < name.Length(); i++)
        {
            int charCode = name.Get(i);
            // Allow printable characters except for certain special ones
            if (charCode >= 32 && charCode != 127 && 
                charCode != 60 && charCode != 62 && // < >
                charCode != 34 && charCode != 39 && // " '
                charCode != 92 && charCode != 96) // \ `
            {
                result += name.Get(i);
            }
            else
            {
                // Replace with underscore
                result += "_";
            }
        }
        
        if (result.Length() == 0)
            return "Unknown";
            
        // Truncate if too long
        if (result.Length() > MAX_NAME_LENGTH)
            result = result.Substring(0, MAX_NAME_LENGTH);
            
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Sanitize IP address
    protected string SanitizeIPAddress(string ip)
    {
        if (!ip || ip.Length() == 0)
            return "0.0.0.0";
            
        string result = "";
        
        // Keep only valid characters
        for (int i = 0; i < ip.Length(); i++)
        {
            int charCode = ip.Get(i);
            // Allow only digits, a-f, A-F, dots, colons, and percent sign (for IPv6 scope)
            if (
                (charCode >= 48 && charCode <= 57) || // 0-9
                (charCode >= 65 && charCode <= 70) || // A-F
                (charCode >= 97 && charCode <= 102) || // a-f
                charCode == 46 || // .
                charCode == 58 || // :
                charCode == 37 // %
            )
            {
                result += ip.Get(i);
            }
        }
        
        if (result.Length() == 0 || result.Length() > MAX_IP_LENGTH)
            return "0.0.0.0";
            
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Sanitize general string
    protected string SanitizeString(string str)
    {
        if (!str || str.Length() == 0)
            return "";
            
        string result = "";
        
        // Keep only valid characters
        for (int i = 0; i < str.Length(); i++)
        {
            int charCode = str.Get(i);
            // Allow printable characters except for certain special ones
            if (charCode >= 32 && charCode != 127 && 
                charCode != 60 && charCode != 62 && // < >
                charCode != 34 && charCode != 39 && // " '
                charCode != 92 && charCode != 96) // \ `
            {
                result += str.Get(i);
            }
            else
            {
                // Replace with underscore
                result += "_";
            }
        }
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Serialize map to JSON
    protected string SerializeMapToJson(map<string, int> dataMap)
    {
        if (!dataMap || dataMap.Count() == 0)
            return "{}";
            
        string json = "{";
        
        array<string> keys = new array<string>();
        dataMap.GetKeyArray(keys);
        
        for (int i = 0; i < keys.Count(); i++)
        {
            string key = keys[i];
            int value = dataMap.Get(key);
            
            // Escape quotes in key
            key = key.Replace("\"", "\\\"");
            
            json += "\"" + key + "\":" + value;
            
            if (i < keys.Count() - 1)
                json += ",";
        }
        
        json += "}";
        
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Deserialize JSON to map
    protected void DeserializeJsonToMap(string json, out map<string, int> result)
    {
        // This is a very simplified JSON parser for the format we use
        // In a real implementation, use a proper JSON parser
        // Expected format: {"key1":value1,"key2":value2,...}
        
        if (!result)
            result = new map<string, int>();
            
        if (json.Length() < 2 || json[0] != '{' || json[json.Length() - 1] != '}')
            return;
            
        // Remove the outer braces
        string content = json.Substring(1, json.Length() - 2);
        
        // Process one key-value pair at a time
        int pos = 0;
        while (pos < content.Length())
        {
            // Find the key (should start with a quote)
            int keyStart = content.IndexOfFrom("\"", pos);
            if (keyStart == -1)
                break;
                
            int keyEnd = content.IndexOfFrom("\"", keyStart + 1);
            if (keyEnd == -1)
                break;
                
            // Extract the key
            string key = content.Substring(keyStart + 1, keyEnd - keyStart - 1);
            
            // Find the value (should start after a colon)
            int colonPos = content.IndexOfFrom(":", keyEnd);
            if (colonPos == -1)
                break;
                
            // Find the end of the value (comma or end of string)
            int valueEnd = content.IndexOfFrom(",", colonPos);
            if (valueEnd == -1)
                valueEnd = content.Length();
                
            // Extract the value
            string valueStr = content.Substring(colonPos + 1, valueEnd - colonPos - 1);
            
            // Parse the value as an integer
            int value = valueStr.ToInt();
            
            // Add to the map
            result.Insert(key, value);
            
            // Move to the next pair
            pos = valueEnd + 1;
        }
    }
}

// Kill record data structure
class STS_KillRecord
{
    string m_sKillerName;
    string m_sVictimName;
    string m_sWeapon;
    int m_iTimestamp;
    float m_fDistance;
    bool m_bHeadshot;
    
    //------------------------------------------------------------------------------------------------
    // Serialize to JSON
    string ToJSON()
    {
        string json = "{";
        
        json += "\"killer\":\"" + m_sKillerName.Replace("\"", "\\\"") + "\",";
        json += "\"victim\":\"" + m_sVictimName.Replace("\"", "\\\"") + "\",";
        json += "\"weapon\":\"" + m_sWeapon.Replace("\"", "\\\"") + "\",";
        json += "\"timestamp\":" + m_iTimestamp.ToString() + ",";
        json += "\"distance\":" + m_fDistance.ToString() + ",";
        json += "\"headshot\":" + m_bHeadshot.ToString();
        
        json += "}";
        
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Deserialize from JSON
    static STS_KillRecord FromJSON(string json)
    {
        // Create new record
        STS_KillRecord record = new STS_KillRecord();
        
        // This is a very simplified JSON parser
        // In a real implementation, use a proper JSON parser
        
        // Parse killer
        int killerStart = json.IndexOf("\"killer\":\"");
        if (killerStart != -1)
        {
            killerStart += 10; // Length of "killer":\"
            int killerEnd = json.IndexOfFrom("\"", killerStart);
            if (killerEnd != -1)
            {
                record.m_sKillerName = json.Substring(killerStart, killerEnd - killerStart);
            }
        }
        
        // Parse victim
        int victimStart = json.IndexOf("\"victim\":\"");
        if (victimStart != -1)
        {
            victimStart += 10; // Length of "victim":\"
            int victimEnd = json.IndexOfFrom("\"", victimStart);
            if (victimEnd != -1)
            {
                record.m_sVictimName = json.Substring(victimStart, victimEnd - victimStart);
            }
        }
        
        // Parse weapon
        int weaponStart = json.IndexOf("\"weapon\":\"");
        if (weaponStart != -1)
        {
            weaponStart += 10; // Length of "weapon":\"
            int weaponEnd = json.IndexOfFrom("\"", weaponStart);
            if (weaponEnd != -1)
            {
                record.m_sWeapon = json.Substring(weaponStart, weaponEnd - weaponStart);
            }
        }
        
        // Parse timestamp
        int timestampStart = json.IndexOf("\"timestamp\":");
        if (timestampStart != -1)
        {
            timestampStart += 12; // Length of "timestamp":
            int timestampEnd = json.IndexOfFrom(",", timestampStart);
            if (timestampEnd != -1)
            {
                string timestampStr = json.Substring(timestampStart, timestampEnd - timestampStart);
                record.m_iTimestamp = timestampStr.ToInt();
            }
        }
        
        // Parse distance
        int distanceStart = json.IndexOf("\"distance\":");
        if (distanceStart != -1)
        {
            distanceStart += 11; // Length of "distance":
            int distanceEnd = json.IndexOfFrom(",", distanceStart);
            if (distanceEnd != -1)
            {
                string distanceStr = json.Substring(distanceStart, distanceEnd - distanceStart);
                record.m_fDistance = distanceStr.ToFloat();
            }
        }
        
        // Parse headshot
        int headshotStart = json.IndexOf("\"headshot\":");
        if (headshotStart != -1)
        {
            headshotStart += 11; // Length of "headshot":
            string headshotStr = json.Substring(headshotStart, 5); // true or false
            record.m_bHeadshot = headshotStr.IndexOf("true") != -1;
        }
        
        return record;
    }
} 