// STS_EnhancedPlayerStats.c
// Enhanced player statistics class with detailed metrics

class STS_EnhancedPlayerStats : STS_PlayerStats
{
    // Additional player identification
    string m_sUID;           // Unique identifier
    string m_sSteamID;       // Steam ID (if available)
    
    // Additional session stats
    float m_fFirstLogin;     // First time player logged in (timestamp)
    float m_fLastLogin;      // Last time player logged in (timestamp)
    float m_fLastLogout;     // Last time player logged out (timestamp)
    int m_iTotalSessions;    // Number of sessions player has played
    
    // Combat stats - extended
    int m_iHeadshotKills;    // Headshot kills
    int m_iLongestKill;      // Longest kill distance in meters
    ref map<string, int> m_mKillsByWeapon; // Kills by weapon type
    
    // Movement and travel stats
    float m_fDistanceTraveled;       // Total distance traveled in meters
    float m_fDistanceOnFoot;         // Distance traveled on foot
    float m_fDistanceInVehicles;     // Distance traveled in vehicles
    array<vector> m_aVisitedLocations; // Array of visited location positions
    
    // Damage stats
    float m_fDamageDealt;            // Total damage dealt to enemies
    float m_fDamageTaken;            // Total damage taken
    int m_iTimesBled;                // Number of times player has bled
    int m_iTimesUnconcious;          // Number of times player was unconscious
    
    // Economy stats (for game modes with economy)
    int m_iMoneyEarned;              // Total money earned
    int m_iMoneySpent;               // Total money spent
    ref map<string, int> m_mItemsBought; // Items bought with counts
    ref map<string, int> m_mItemsSold;   // Items sold with counts
    
    // Achievement-like stats
    ref array<string> m_aAchievements;   // List of achievements earned
    ref map<string, int> m_mChallenges;  // Progress on different challenges
    
    // Leaderboard position tracking
    int m_iLastLeaderboardRank;      // Last known position on leaderboard
    int m_iBestLeaderboardRank;      // Best position on leaderboard
    
    //------------------------------------------------------------------------------------------------
    void STS_EnhancedPlayerStats()
    {
        super.STS_PlayerStats();
        
        // Initialize additional fields
        m_sUID = "";
        m_sSteamID = "";
        
        m_fFirstLogin = 0;
        m_fLastLogin = 0;
        m_fLastLogout = 0;
        m_iTotalSessions = 0;
        
        m_iHeadshotKills = 0;
        m_iLongestKill = 0;
        m_mKillsByWeapon = new map<string, int>();
        
        m_fDistanceTraveled = 0;
        m_fDistanceOnFoot = 0;
        m_fDistanceInVehicles = 0;
        m_aVisitedLocations = new array<vector>();
        
        m_fDamageDealt = 0;
        m_fDamageTaken = 0;
        m_iTimesBled = 0;
        m_iTimesUnconcious = 0;
        
        m_iMoneyEarned = 0;
        m_iMoneySpent = 0;
        m_mItemsBought = new map<string, int>();
        m_mItemsSold = new map<string, int>();
        
        m_aAchievements = new array<string>();
        m_mChallenges = new map<string, int>();
        
        m_iLastLeaderboardRank = 0;
        m_iBestLeaderboardRank = 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Record a player kill with detailed information
    void RecordKill(string weaponName, float distance, bool isHeadshot = false)
    {
        // Update basic kill count through parent class
        m_iKills++;
        
        // Record weapon stats
        if (!m_mKillsByWeapon.Contains(weaponName))
            m_mKillsByWeapon.Insert(weaponName, 0);
            
        m_mKillsByWeapon[weaponName]++;
        
        // Record headshot
        if (isHeadshot)
            m_iHeadshotKills++;
        
        // Update longest kill if applicable
        if (distance > m_iLongestKill)
            m_iLongestKill = distance;
            
        // Check for achievements
        CheckKillAchievements();
    }
    
    //------------------------------------------------------------------------------------------------
    // Record player login
    void RecordLogin(string uid, string steamId)
    {
        float currentTime = System.GetTickCount() / 1000.0;
        
        m_sUID = uid;
        m_sSteamID = steamId;
        
        // Set first login time if this is the first time
        if (m_fFirstLogin == 0)
            m_fFirstLogin = currentTime;
            
        m_fLastLogin = currentTime;
        m_iTotalSessions++;
    }
    
    //------------------------------------------------------------------------------------------------
    // Record player logout
    void RecordLogout()
    {
        float currentTime = System.GetTickCount() / 1000.0;
        m_fLastLogout = currentTime;
        
        // Update total playtime in the parent class
        float sessionDuration = m_fLastLogout - m_fLastLogin;
        if (sessionDuration > 0)
            m_fTotalPlaytime += sessionDuration;
    }
    
    //------------------------------------------------------------------------------------------------
    // Record damage dealt to an enemy
    void RecordDamageDealt(float amount, EDamageType damageType, int hitZone)
    {
        m_fDamageDealt += amount;
        
        // Check for specific damage achievements
        if (amount > 50)
            IncrementChallenge("HighDamageHits");
    }
    
    //------------------------------------------------------------------------------------------------
    // Record damage taken by player
    void RecordDamageTaken(float amount, EDamageType damageType, int hitZone)
    {
        m_fDamageTaken += amount;
        
        // Check if this damage caused bleeding
        if (damageType == EDamageType.BLEEDING)
            m_iTimesBled++;
    }
    
    //------------------------------------------------------------------------------------------------
    // Record player movement
    void RecordMovement(float distance, bool inVehicle = false)
    {
        m_fDistanceTraveled += distance;
        
        if (inVehicle)
            m_fDistanceInVehicles += distance;
        else
            m_fDistanceOnFoot += distance;
            
        // Check for movement-based achievements
        if (m_fDistanceTraveled > 10000)
            AddAchievement("Traveler");
            
        if (m_fDistanceTraveled > 100000)
            AddAchievement("Explorer");
    }
    
    //------------------------------------------------------------------------------------------------
    // Record player visiting a new location
    void RecordLocationVisit(vector position, string locationName)
    {
        // Check if we've already visited a location near this one
        bool alreadyVisited = false;
        foreach (vector loc : m_aVisitedLocations)
        {
            if (vector.Distance(loc, position) < 100)
            {
                alreadyVisited = true;
                break;
            }
        }
        
        if (!alreadyVisited)
        {
            m_aVisitedLocations.Insert(position);
            
            // Update visited locations count in challenges
            IncrementChallenge("LocationsVisited");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Record economic activity - buying items
    void RecordItemPurchase(string itemName, int count, int price)
    {
        if (!m_mItemsBought.Contains(itemName))
            m_mItemsBought.Insert(itemName, 0);
            
        m_mItemsBought[itemName] += count;
        m_iMoneySpent += price;
    }
    
    //------------------------------------------------------------------------------------------------
    // Record economic activity - selling items
    void RecordItemSale(string itemName, int count, int price)
    {
        if (!m_mItemsSold.Contains(itemName))
            m_mItemsSold.Insert(itemName, 0);
            
        m_mItemsSold[itemName] += count;
        m_iMoneyEarned += price;
    }
    
    //------------------------------------------------------------------------------------------------
    // Record player becoming unconscious
    void RecordUnconsciousness()
    {
        m_iTimesUnconcious++;
    }
    
    //------------------------------------------------------------------------------------------------
    // Update player's rank on leaderboard
    void UpdateLeaderboardRank(int newRank)
    {
        m_iLastLeaderboardRank = newRank;
        
        if (m_iBestLeaderboardRank == 0 || newRank < m_iBestLeaderboardRank)
            m_iBestLeaderboardRank = newRank;
    }
    
    //------------------------------------------------------------------------------------------------
    // Add an achievement
    void AddAchievement(string achievementName)
    {
        // Check if already has this achievement
        foreach (string ach : m_aAchievements)
        {
            if (ach == achievementName)
                return;
        }
        
        m_aAchievements.Insert(achievementName);
    }
    
    //------------------------------------------------------------------------------------------------
    // Increment a challenge counter
    void IncrementChallenge(string challengeName, int amount = 1)
    {
        if (!m_mChallenges.Contains(challengeName))
            m_mChallenges.Insert(challengeName, 0);
            
        m_mChallenges[challengeName] += amount;
        
        // Check if the challenge is completed based on thresholds
        CheckChallengeCompletion(challengeName);
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if a challenge is completed and award achievements
    void CheckChallengeCompletion(string challengeName)
    {
        if (!m_mChallenges.Contains(challengeName))
            return;
            
        int progress = m_mChallenges[challengeName];
        
        // Different thresholds for different challenges
        if (challengeName == "LocationsVisited")
        {
            if (progress >= 10)
                AddAchievement("Sightseer");
                
            if (progress >= 50)
                AddAchievement("Globetrotter");
        }
        else if (challengeName == "HighDamageHits")
        {
            if (progress >= 50)
                AddAchievement("HeavyHitter");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Check for kill-based achievements
    void CheckKillAchievements()
    {
        if (m_iKills >= 10)
            AddAchievement("Hunter");
            
        if (m_iKills >= 50)
            AddAchievement("Warrior");
            
        if (m_iKills >= 100)
            AddAchievement("Veteran");
            
        if (m_iHeadshotKills >= 25)
            AddAchievement("Marksman");
    }
    
    //------------------------------------------------------------------------------------------------
    // Override ToJSON to include all the enhanced stats
    override string ToJSON()
    {
        // Start with base stats from parent class
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
        json += "\"ipAddress\":\"" + m_sIPAddress + "\",";
        json += "\"connectionTime\":" + m_fConnectionTime.ToString() + ",";
        json += "\"lastSessionDuration\":" + m_fLastSessionDuration.ToString() + ",";
        json += "\"totalPlaytime\":" + m_fTotalPlaytime.ToString() + ",";
        
        // Enhanced stats
        json += "\"uid\":\"" + m_sUID + "\",";
        json += "\"steamId\":\"" + m_sSteamID + "\",";
        json += "\"firstLogin\":" + m_fFirstLogin.ToString() + ",";
        json += "\"lastLogin\":" + m_fLastLogin.ToString() + ",";
        json += "\"lastLogout\":" + m_fLastLogout.ToString() + ",";
        json += "\"totalSessions\":" + m_iTotalSessions.ToString() + ",";
        json += "\"headshotKills\":" + m_iHeadshotKills.ToString() + ",";
        json += "\"longestKill\":" + m_iLongestKill.ToString() + ",";
        
        // Weapons stats
        json += "\"killsByWeapon\":{";
        bool firstWeapon = true;
        foreach (string weapon, int kills : m_mKillsByWeapon)
        {
            if (!firstWeapon)
                json += ",";
                
            json += "\"" + weapon + "\":" + kills.ToString();
            firstWeapon = false;
        }
        json += "},";
        
        // Movement stats
        json += "\"distanceTraveled\":" + m_fDistanceTraveled.ToString() + ",";
        json += "\"distanceOnFoot\":" + m_fDistanceOnFoot.ToString() + ",";
        json += "\"distanceInVehicles\":" + m_fDistanceInVehicles.ToString() + ",";
        
        // Damage stats
        json += "\"damageDealt\":" + m_fDamageDealt.ToString() + ",";
        json += "\"damageTaken\":" + m_fDamageTaken.ToString() + ",";
        json += "\"timesBled\":" + m_iTimesBled.ToString() + ",";
        json += "\"timesUnconcious\":" + m_iTimesUnconcious.ToString() + ",";
        
        // Economy stats
        json += "\"moneyEarned\":" + m_iMoneyEarned.ToString() + ",";
        json += "\"moneySpent\":" + m_iMoneySpent.ToString() + ",";
        
        // Achievements
        json += "\"achievements\":[";
        for (int i = 0; i < m_aAchievements.Count(); i++)
        {
            if (i > 0) json += ",";
            json += "\"" + m_aAchievements[i] + "\"";
        }
        json += "],";
        
        // Challenges
        json += "\"challenges\":{";
        bool firstChallenge = true;
        foreach (string challenge, int progress : m_mChallenges)
        {
            if (!firstChallenge)
                json += ",";
                
            json += "\"" + challenge + "\":" + progress.ToString();
            firstChallenge = false;
        }
        json += "},";
        
        // Leaderboard
        json += "\"lastLeaderboardRank\":" + m_iLastLeaderboardRank.ToString() + ",";
        json += "\"bestLeaderboardRank\":" + m_iBestLeaderboardRank.ToString();
        
        json += "}";
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Override FromJSON to load all the enhanced stats
    override void FromJSON(string json)
    {
        // First call the parent implementation to load basic stats
        super.FromJSON(json);
        
        // Now parse additional fields
        ExtractStringValue(json, "uid", m_sUID);
        ExtractStringValue(json, "steamId", m_sSteamID);
        
        m_fFirstLogin = ExtractFloatValue(json, "firstLogin");
        m_fLastLogin = ExtractFloatValue(json, "lastLogin");
        m_fLastLogout = ExtractFloatValue(json, "lastLogout");
        m_iTotalSessions = ExtractIntValue(json, "totalSessions");
        
        m_iHeadshotKills = ExtractIntValue(json, "headshotKills");
        m_iLongestKill = ExtractIntValue(json, "longestKill");
        
        // Extract killsByWeapon map
        string weaponsJson = ExtractObjectValue(json, "killsByWeapon");
        ParseKillsByWeaponMap(weaponsJson);
        
        m_fDistanceTraveled = ExtractFloatValue(json, "distanceTraveled");
        m_fDistanceOnFoot = ExtractFloatValue(json, "distanceOnFoot");
        m_fDistanceInVehicles = ExtractFloatValue(json, "distanceInVehicles");
        
        m_fDamageDealt = ExtractFloatValue(json, "damageDealt");
        m_fDamageTaken = ExtractFloatValue(json, "damageTaken");
        m_iTimesBled = ExtractIntValue(json, "timesBled");
        m_iTimesUnconcious = ExtractIntValue(json, "timesUnconcious");
        
        m_iMoneyEarned = ExtractIntValue(json, "moneyEarned");
        m_iMoneySpent = ExtractIntValue(json, "moneySpent");
        
        // Extract achievements array
        string achievementsJson = ExtractArrayValue(json, "achievements");
        ParseAchievementsArray(achievementsJson);
        
        // Extract challenges map
        string challengesJson = ExtractObjectValue(json, "challenges");
        ParseChallengesMap(challengesJson);
        
        m_iLastLeaderboardRank = ExtractIntValue(json, "lastLeaderboardRank");
        m_iBestLeaderboardRank = ExtractIntValue(json, "bestLeaderboardRank");
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper for extracting string values from JSON
    private void ExtractStringValue(string json, string key, out string value)
    {
        string searchKey = "\"" + key + "\":\"";
        int startPos = json.IndexOf(searchKey);
        
        if (startPos == -1)
            return;
            
        startPos += searchKey.Length();
        int endPos = json.IndexOf("\"", startPos);
        
        if (endPos == -1)
            return;
            
        value = json.Substring(startPos, endPos - startPos);
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper for extracting integer values from JSON
    private int ExtractIntValue(string json, string key)
    {
        string searchKey = "\"" + key + "\":";
        int startPos = json.IndexOf(searchKey);
        
        if (startPos == -1)
            return 0;
            
        startPos += searchKey.Length();
        int endPos = startPos;
        
        while (endPos < json.Length() && "0123456789".Contains(json[endPos].ToString()))
            endPos++;
            
        if (startPos == endPos)
            return 0;
            
        string valueStr = json.Substring(startPos, endPos - startPos);
        return valueStr.ToInt();
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper for extracting float values from JSON
    private float ExtractFloatValue(string json, string key)
    {
        string searchKey = "\"" + key + "\":";
        int startPos = json.IndexOf(searchKey);
        
        if (startPos == -1)
            return 0.0;
            
        startPos += searchKey.Length();
        int endPos = startPos;
        
        while (endPos < json.Length() && "0123456789.".Contains(json[endPos].ToString()))
            endPos++;
            
        if (startPos == endPos)
            return 0.0;
            
        string valueStr = json.Substring(startPos, endPos - startPos);
        return valueStr.ToFloat();
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper for extracting object values from JSON
    private string ExtractObjectValue(string json, string key)
    {
        string searchKey = "\"" + key + "\":{";
        int startPos = json.IndexOf(searchKey);
        
        if (startPos == -1)
            return "{}";
            
        startPos += searchKey.Length() - 1; // Include the opening brace
        
        // Find the matching closing brace
        int braceCount = 1;
        int endPos = startPos + 1;
        
        while (braceCount > 0 && endPos < json.Length())
        {
            if (json[endPos] == '{')
                braceCount++;
            else if (json[endPos] == '}')
                braceCount--;
                
            endPos++;
        }
        
        if (braceCount != 0)
            return "{}";
            
        return json.Substring(startPos, endPos - startPos);
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper for extracting array values from JSON
    private string ExtractArrayValue(string json, string key)
    {
        string searchKey = "\"" + key + "\":[";
        int startPos = json.IndexOf(searchKey);
        
        if (startPos == -1)
            return "[]";
            
        startPos += searchKey.Length() - 1; // Include the opening bracket
        
        // Find the matching closing bracket
        int bracketCount = 1;
        int endPos = startPos + 1;
        
        while (bracketCount > 0 && endPos < json.Length())
        {
            if (json[endPos] == '[')
                bracketCount++;
            else if (json[endPos] == ']')
                bracketCount--;
                
            endPos++;
        }
        
        if (bracketCount != 0)
            return "[]";
            
        return json.Substring(startPos, endPos - startPos);
    }
    
    //------------------------------------------------------------------------------------------------
    // Parse kills by weapon map from JSON
    private void ParseKillsByWeaponMap(string json)
    {
        m_mKillsByWeapon.Clear();
        
        // Simple parser for the format {"weapon1":count1,"weapon2":count2,...}
        int pos = 1; // Skip opening brace
        int len = json.Length();
        
        while (pos < len - 1)
        {
            // Find the weapon name (should be in quotes)
            if (json[pos] != '"')
            {
                pos++;
                continue;
            }
            
            int weaponStart = pos + 1;
            pos++;
            
            while (pos < len && json[pos] != '"')
                pos++;
                
            if (pos >= len)
                break;
                
            string weaponName = json.Substring(weaponStart, pos - weaponStart);
            pos++;
            
            // Find the colon
            while (pos < len && json[pos] != ':')
                pos++;
                
            if (pos >= len)
                break;
                
            pos++; // Skip the colon
            
            // Find the count
            int countStart = pos;
            
            while (pos < len && "0123456789".Contains(json[pos].ToString()))
                pos++;
                
            if (countStart == pos)
                continue;
                
            string countStr = json.Substring(countStart, pos - countStart);
            int count = countStr.ToInt();
            
            // Add to the map
            m_mKillsByWeapon.Insert(weaponName, count);
            
            // Skip to the next pair
            while (pos < len && json[pos] != ',')
                pos++;
                
            if (pos >= len)
                break;
                
            pos++; // Skip the comma
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Parse achievements array from JSON
    private void ParseAchievementsArray(string json)
    {
        m_aAchievements.Clear();
        
        // Simple parser for the format ["achievement1","achievement2",...]
        int pos = 1; // Skip opening bracket
        int len = json.Length();
        
        while (pos < len - 1)
        {
            // Find the achievement name (should be in quotes)
            if (json[pos] != '"')
            {
                pos++;
                continue;
            }
            
            int achievementStart = pos + 1;
            pos++;
            
            while (pos < len && json[pos] != '"')
                pos++;
                
            if (pos >= len)
                break;
                
            string achievement = json.Substring(achievementStart, pos - achievementStart);
            m_aAchievements.Insert(achievement);
            
            pos++; // Skip closing quote
            
            // Skip to the next item
            while (pos < len && json[pos] != ',')
                pos++;
                
            if (pos >= len)
                break;
                
            pos++; // Skip the comma
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Parse challenges map from JSON
    private void ParseChallengesMap(string json)
    {
        m_mChallenges.Clear();
        
        // Simple parser for the format {"challenge1":progress1,"challenge2":progress2,...}
        int pos = 1; // Skip opening brace
        int len = json.Length();
        
        while (pos < len - 1)
        {
            // Find the challenge name (should be in quotes)
            if (json[pos] != '"')
            {
                pos++;
                continue;
            }
            
            int challengeStart = pos + 1;
            pos++;
            
            while (pos < len && json[pos] != '"')
                pos++;
                
            if (pos >= len)
                break;
                
            string challengeName = json.Substring(challengeStart, pos - challengeStart);
            pos++;
            
            // Find the colon
            while (pos < len && json[pos] != ':')
                pos++;
                
            if (pos >= len)
                break;
                
            pos++; // Skip the colon
            
            // Find the progress
            int progressStart = pos;
            
            while (pos < len && "0123456789".Contains(json[pos].ToString()))
                pos++;
                
            if (progressStart == pos)
                continue;
                
            string progressStr = json.Substring(progressStart, pos - progressStart);
            int progress = progressStr.ToInt();
            
            // Add to the map
            m_mChallenges.Insert(challengeName, progress);
            
            // Skip to the next pair
            while (pos < len && json[pos] != ',')
                pos++;
                
            if (pos >= len)
                break;
                
            pos++; // Skip the comma
        }
    }
} 