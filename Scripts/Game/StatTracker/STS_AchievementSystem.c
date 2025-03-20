// STS_AchievementSystem.c
// Achievement system for tracking player milestones

// Achievement entry
class Achievement
{
    string m_sID;
    string m_sName;
    string m_sDescription;
    string m_sIconPath;
    int m_iScoreValue;
    bool m_bIsSecret;
    bool m_bIsProgressive;
    int m_iMaxProgress;
    
    void Achievement(string id, string name, string description, string iconPath = "", int scoreValue = 100, bool isSecret = false, bool isProgressive = false, int maxProgress = 1)
    {
        m_sID = id;
        m_sName = name;
        m_sDescription = description;
        m_sIconPath = iconPath;
        m_iScoreValue = scoreValue;
        m_bIsSecret = isSecret;
        m_bIsProgressive = isProgressive;
        m_iMaxProgress = maxProgress;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"id\":\"" + m_sID + "\",";
        json += "\"name\":\"" + m_sName + "\",";
        json += "\"description\":\"" + m_sDescription + "\",";
        json += "\"iconPath\":\"" + m_sIconPath + "\",";
        json += "\"scoreValue\":" + m_iScoreValue.ToString() + ",";
        json += "\"isSecret\":" + m_bIsSecret.ToString() + ",";
        json += "\"isProgressive\":" + m_bIsProgressive.ToString() + ",";
        json += "\"maxProgress\":" + m_iMaxProgress.ToString();
        json += "}";
        return json;
    }
}

// Player achievement progress
class PlayerAchievement
{
    string m_sAchievementID;
    bool m_bUnlocked;
    int m_iProgress;
    float m_fUnlockTime;
    
    void PlayerAchievement(string achievementID)
    {
        m_sAchievementID = achievementID;
        m_bUnlocked = false;
        m_iProgress = 0;
        m_fUnlockTime = 0;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"achievementID\":\"" + m_sAchievementID + "\",";
        json += "\"unlocked\":" + m_bUnlocked.ToString() + ",";
        json += "\"progress\":" + m_iProgress.ToString() + ",";
        json += "\"unlockTime\":" + m_fUnlockTime.ToString();
        json += "}";
        return json;
    }
    
    static PlayerAchievement FromJSON(string json)
    {
        // Default achievement
        PlayerAchievement achievement = new PlayerAchievement("");
        
        // Parse JSON (simplified)
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
            
            if (key == "achievementID") achievement.m_sAchievementID = value;
            else if (key == "unlocked") achievement.m_bUnlocked = value.ToInt() != 0;
            else if (key == "progress") achievement.m_iProgress = value.ToInt();
            else if (key == "unlockTime") achievement.m_fUnlockTime = value.ToFloat();
        }
        
        return achievement;
    }
}

// Player achievement collection
class PlayerAchievements
{
    int m_iPlayerID;
    string m_sPlayerName;
    ref map<string, ref PlayerAchievement> m_mAchievements = new map<string, ref PlayerAchievement>();
    
    void PlayerAchievements(int playerID, string playerName)
    {
        m_iPlayerID = playerID;
        m_sPlayerName = playerName;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"playerID\":" + m_iPlayerID.ToString() + ",";
        json += "\"playerName\":\"" + m_sPlayerName + "\",";
        json += "\"achievements\":{";
        
        int count = 0;
        foreach (string achievementID, PlayerAchievement achievement : m_mAchievements)
        {
            json += "\"" + achievementID + "\":" + achievement.ToJSON();
            
            if (count < m_mAchievements.Count() - 1)
                json += ",";
                
            count++;
        }
        
        json += "}}";
        return json;
    }
    
    static PlayerAchievements FromJSON(string json)
    {
        // Create default player achievements
        PlayerAchievements playerAchievements = new PlayerAchievements(0, "");
        
        // Parse JSON (simplified)
        array<string> outerPairs = new array<string>();
        json.Replace("{", "").Replace("}", "").Split(",", outerPairs);
        
        foreach (string pair : outerPairs)
        {
            array<string> keyValue = new array<string>();
            pair.Split(":", keyValue);
            
            if (keyValue.Count() < 2)
                continue;
                
            string key = keyValue[0].Replace("\"", "");
            string value = keyValue[1].Replace("\"", "");
            
            if (key == "playerID") playerAchievements.m_iPlayerID = value.ToInt();
            else if (key == "playerName") playerAchievements.m_sPlayerName = value;
            // Achievements would need special handling for the nested object
            // In a real implementation, use a proper JSON parser
        }
        
        return playerAchievements;
    }
}

class STS_AchievementSystem
{
    // Singleton instance
    private static ref STS_AchievementSystem s_Instance;
    
    // All available achievements
    protected ref map<string, ref Achievement> m_mAchievements = new map<string, ref Achievement>();
    
    // Player achievement progress
    protected ref map<int, ref PlayerAchievements> m_mPlayerAchievements = new map<int, ref PlayerAchievements>();
    
    // File path for saving achievement data
    protected const string ACHIEVEMENTS_DATA_PATH = "$profile:StatTracker/achievements.json";
    
    // References to other components
    protected ref STS_NotificationManager m_NotificationManager;
    protected ref STS_WebhookManager m_WebhookManager;
    protected ref STS_Config m_Config;
    protected ref STS_UIManager m_UIManager;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_AchievementSystem()
    {
        Print("[StatTracker] Initializing Achievement System");
        
        // Get references to other components
        m_NotificationManager = STS_NotificationManager.GetInstance();
        m_WebhookManager = STS_WebhookManager.GetInstance();
        m_Config = STS_Config.GetInstance();
        m_UIManager = STS_UIManager.GetInstance();
        
        // Define all achievements
        InitializeAchievements();
        
        // Load achievement data
        LoadAchievementData();
        
        // Set up event listeners
        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (gameMode)
        {
            // Player connected/disconnected
            gameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);
            gameMode.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
            
            // Subscribe to kill events
            SCR_KillManager killManager = SCR_KillManager.Instance();
            if (killManager)
            {
                killManager.GetOnPlayerKilled().Insert(OnPlayerKilled);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_AchievementSystem GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_AchievementSystem();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize all achievements
    protected void InitializeAchievements()
    {
        m_mAchievements.Clear();
        
        // Combat achievements
        m_mAchievements.Insert("FIRST_BLOOD", new Achievement("FIRST_BLOOD", "First Blood", "Get your first kill", "", 50));
        m_mAchievements.Insert("HEADHUNTER", new Achievement("HEADHUNTER", "Headhunter", "Get 10 headshot kills", "", 100, false, true, 10));
        m_mAchievements.Insert("MARKSMAN", new Achievement("MARKSMAN", "Marksman", "Kill an enemy from over 500m away", "", 150));
        m_mAchievements.Insert("SHARPSHOOTER", new Achievement("SHARPSHOOTER", "Sharpshooter", "Kill 3 enemies in a row without missing a shot", "", 150));
        m_mAchievements.Insert("SERIAL_KILLER", new Achievement("SERIAL_KILLER", "Serial Killer", "Get a 5-kill streak", "", 200));
        m_mAchievements.Insert("MASS_MURDERER", new Achievement("MASS_MURDERER", "Mass Murderer", "Get a 10-kill streak", "", 300));
        m_mAchievements.Insert("UNSTOPPABLE", new Achievement("UNSTOPPABLE", "Unstoppable", "Get a 20-kill streak", "", 500));
        m_mAchievements.Insert("SURVIVOR", new Achievement("SURVIVOR", "Survivor", "Survive for 30 minutes without dying", "", 150));
        m_mAchievements.Insert("VETERAN", new Achievement("VETERAN", "Veteran", "Kill 100 enemies", "", 200, false, true, 100));
        
        // Vehicle achievements
        m_mAchievements.Insert("TANK_BUSTER", new Achievement("TANK_BUSTER", "Tank Buster", "Destroy a tank", "", 150));
        m_mAchievements.Insert("ACE_PILOT", new Achievement("ACE_PILOT", "Ace Pilot", "Shoot down 3 aircraft", "", 300, false, true, 3));
        m_mAchievements.Insert("ROAD_RAGE", new Achievement("ROAD_RAGE", "Road Rage", "Run over 10 enemies with a vehicle", "", 150, false, true, 10));
        
        // Objective achievements
        m_mAchievements.Insert("CAPTOR", new Achievement("CAPTOR", "Captor", "Capture your first objective", "", 100));
        m_mAchievements.Insert("MASTER_CAPTOR", new Achievement("MASTER_CAPTOR", "Master Captor", "Capture 20 objectives", "", 300, false, true, 20));
        m_mAchievements.Insert("SUPPLY_RUNNER", new Achievement("SUPPLY_RUNNER", "Supply Runner", "Deliver 10 supplies", "", 200, false, true, 10));
        
        // Teamwork achievements
        m_mAchievements.Insert("MEDIC", new Achievement("MEDIC", "Medic", "Heal 10 teammates", "", 150, false, true, 10));
        m_mAchievements.Insert("GOOD_SAMARITAN", new Achievement("GOOD_SAMARITAN", "Good Samaritan", "Revive 5 teammates", "", 200, false, true, 5));
        
        // Progression achievements
        m_mAchievements.Insert("RANK_UP", new Achievement("RANK_UP", "Rank Up", "Reach Rank 2", "", 50));
        m_mAchievements.Insert("VETERAN_RANK", new Achievement("VETERAN_RANK", "Veteran Rank", "Reach Rank 5", "", 200));
        m_mAchievements.Insert("ELITE_RANK", new Achievement("ELITE_RANK", "Elite Rank", "Reach Rank 8", "", 400));
        m_mAchievements.Insert("MASTER_RANK", new Achievement("MASTER_RANK", "Master Rank", "Reach Rank 10", "", 500));
        
        // Secret achievements
        m_mAchievements.Insert("LUCKY_SHOT", new Achievement("LUCKY_SHOT", "Lucky Shot", "Kill an enemy while blindfolded (with a flashbang active)", "", 300, true));
        m_mAchievements.Insert("LAST_STAND", new Achievement("LAST_STAND", "Last Stand", "Kill 3 enemies while below 20% health", "", 400, true));
        m_mAchievements.Insert("NEMESIS", new Achievement("NEMESIS", "Nemesis", "Kill the same player 5 times in a row", "", 250, true));
        
        Print(string.Format("[StatTracker] Initialized %1 achievements", m_mAchievements.Count()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Load achievement data from file
    protected void LoadAchievementData()
    {
        // Clear existing data
        m_mPlayerAchievements.Clear();
        
        // Check if file exists
        if (!FileIO.FileExists(ACHIEVEMENTS_DATA_PATH))
            return;
            
        // Read file
        string fileContent;
        if (!FileIO.FileReadAllText(ACHIEVEMENTS_DATA_PATH, fileContent))
            return;
            
        // Parse JSON (simplified)
        fileContent = fileContent.Trim();
        if (!fileContent.StartsWith("{") || !fileContent.EndsWith("}"))
            return;
            
        // Extract player records
        map<string, string> playerDataMap = new map<string, string>();
        
        // Remove outer braces
        fileContent = fileContent.Substring(1, fileContent.Length() - 2);
        
        // Find player data blocks
        int startPos = 0;
        int depth = 0;
        
        for (int i = 0; i < fileContent.Length(); i++)
        {
            string char = fileContent.Get(i);
            
            if (char == "{") depth++;
            else if (char == "}") depth--;
            
            // Find key for player data
            if (depth == 0 && i > 0 && fileContent.Get(i-1) == ":")
            {
                // Extract player ID from previous text
                int j = i - 2;
                while (j >= 0 && fileContent.Get(j) != "\"") j--;
                
                if (j >= 0)
                {
                    int keyStart = j;
                    j--;
                    while (j >= 0 && fileContent.Get(j) != "\"") j--;
                    
                    if (j >= 0)
                    {
                        string playerID = fileContent.Substring(j+1, keyStart-j-1);
                        
                        // Find end of this player's data block
                        int blockStart = i;
                        depth = 1;
                        i++;
                        
                        while (i < fileContent.Length() && depth > 0)
                        {
                            char = fileContent.Get(i);
                            if (char == "{") depth++;
                            else if (char == "}") depth--;
                            i++;
                        }
                        
                        // Extract player data JSON
                        string playerData = fileContent.Substring(blockStart, i - blockStart);
                        
                        // Store in map
                        playerDataMap.Insert(playerID, playerData);
                    }
                }
            }
        }
        
        // Process each player's data
        foreach (string playerID, string playerData : playerDataMap)
        {
            PlayerAchievements achievements = PlayerAchievements.FromJSON(playerData);
            if (achievements && achievements.m_iPlayerID > 0)
            {
                m_mPlayerAchievements.Insert(achievements.m_iPlayerID, achievements);
            }
        }
        
        Print(string.Format("[StatTracker] Loaded achievement data for %1 players", m_mPlayerAchievements.Count()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Save achievement data to file
    void SaveAchievementData()
    {
        string fileContent = "{";
        
        int count = 0;
        foreach (int playerID, PlayerAchievements achievements : m_mPlayerAchievements)
        {
            fileContent += "\"" + playerID.ToString() + "\":" + achievements.ToJSON();
            
            if (count < m_mPlayerAchievements.Count() - 1)
                fileContent += ",";
                
            count++;
        }
        
        fileContent += "}";
        
        // Ensure directory exists
        string dir = "$profile:StatTracker";
        FileIO.MakeDirectory(dir);
        
        // Write to file
        FileIO.FileWrite(ACHIEVEMENTS_DATA_PATH, fileContent);
        
        Print(string.Format("[StatTracker] Saved achievement data for %1 players", m_mPlayerAchievements.Count()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a player connects
    protected void OnPlayerConnected(int playerID)
    {
        // Create player achievements if they don't exist
        if (!m_mPlayerAchievements.Contains(playerID))
        {
            string playerName = GetPlayerNameFromID(playerID);
            PlayerAchievements achievements = new PlayerAchievements(playerID, playerName);
            
            // Initialize all achievements
            foreach (string achievementID, Achievement achievement : m_mAchievements)
            {
                achievements.m_mAchievements.Insert(achievementID, new PlayerAchievement(achievementID));
            }
            
            m_mPlayerAchievements.Insert(playerID, achievements);
            
            Print(string.Format("[StatTracker] Created new achievement tracking for player %1 (ID: %2)", playerName, playerID));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a player disconnects
    protected void OnPlayerDisconnected(int playerID)
    {
        // Save achievement data
        SaveAchievementData();
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a player is killed
    protected void OnPlayerKilled(IEntity victim, IEntity killer, notnull Instigator instigator)
    {
        // Only process on server
        if (!Replication.IsServer())
            return;
            
        // Get killer player controller
        PlayerController killerController = null;
        if (killer)
            killerController = PlayerController.Cast(killer.GetController());
            
        // Only process player kills
        if (!killerController)
            return;
            
        int killerID = killerController.GetPlayerId();
        
        // Check for "FIRST_BLOOD" achievement
        UpdateAchievementProgress(killerID, "FIRST_BLOOD", 1);
        
        // Check for "VETERAN" achievement
        UpdateAchievementProgress(killerID, "VETERAN", 1);
        
        // Check for headshot
        bool isHeadshot = IsHeadshotKill(victim, killer);
        if (isHeadshot)
        {
            UpdateAchievementProgress(killerID, "HEADHUNTER", 1);
        }
        
        // Check for long distance kill
        float distance = vector.Distance(killer.GetOrigin(), victim.GetOrigin());
        if (distance > 500)
        {
            UpdateAchievementProgress(killerID, "MARKSMAN", 1);
        }
        
        // Check for kill streaks (using the progression system data)
        STS_ProgressionSystem progressionSystem = STS_ProgressionSystem.GetInstance();
        if (progressionSystem)
        {
            // Get player progression data
            map<int, ref PlayerProgression> allProgressions = progressionSystem.GetAllPlayerProgressions();
            if (allProgressions.Contains(killerID))
            {
                PlayerProgression progression = allProgressions[killerID];
                
                // Check kill streak achievements
                if (progression.m_iKillStreak >= 5)
                {
                    UpdateAchievementProgress(killerID, "SERIAL_KILLER", 1);
                }
                
                if (progression.m_iKillStreak >= 10)
                {
                    UpdateAchievementProgress(killerID, "MASS_MURDERER", 1);
                }
                
                if (progression.m_iKillStreak >= 20)
                {
                    UpdateAchievementProgress(killerID, "UNSTOPPABLE", 1);
                }
            }
        }
        
        // Process victim's data
        if (victim)
        {
            PlayerController victimController = PlayerController.Cast(victim.GetController());
            if (victimController)
            {
                int victimID = victimController.GetPlayerId();
                
                // Check for "NEMESIS" achievement (tracking repeated kills of the same player)
                if (TrackRepeatedKills(killerID, victimID) >= 5)
                {
                    UpdateAchievementProgress(killerID, "NEMESIS", 1);
                }
            }
        }
        
        // Check for "LAST_STAND" achievement
        CheckLastStandAchievement(killerID, killer);
        
        // Check for "LUCKY_SHOT" achievement
        CheckLuckyShotAchievement(killerID, killer);
    }
    
    //------------------------------------------------------------------------------------------------
    // Update achievement progress
    void UpdateAchievementProgress(int playerID, string achievementID, int progressToAdd = 1)
    {
        // Skip if player or achievement doesn't exist
        if (!m_mPlayerAchievements.Contains(playerID) || !m_mAchievements.Contains(achievementID))
            return;
            
        PlayerAchievements playerAchievements = m_mPlayerAchievements[playerID];
        Achievement achievement = m_mAchievements[achievementID];
        
        // Create the player achievement entry if it doesn't exist
        if (!playerAchievements.m_mAchievements.Contains(achievementID))
        {
            playerAchievements.m_mAchievements.Insert(achievementID, new PlayerAchievement(achievementID));
        }
        
        PlayerAchievement playerAchievement = playerAchievements.m_mAchievements[achievementID];
        
        // Skip if already unlocked for non-progressive achievements
        if (playerAchievement.m_bUnlocked && !achievement.m_bIsProgressive)
            return;
            
        // Add progress
        playerAchievement.m_iProgress += progressToAdd;
        
        // Check if achievement should be unlocked
        if (!playerAchievement.m_bUnlocked && playerAchievement.m_iProgress >= achievement.m_iMaxProgress)
        {
            playerAchievement.m_bUnlocked = true;
            playerAchievement.m_fUnlockTime = System.GetTickCount() / 1000.0;
            
            // Notify player
            NotifyAchievementUnlocked(playerID, achievement);
            
            // Save achievements
            SaveAchievementData();
        }
        else if (achievement.m_bIsProgressive)
        {
            // Save progress regularly for progressive achievements
            SaveAchievementData();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Notify player of achievement unlock
    protected void NotifyAchievementUnlocked(int playerID, Achievement achievement)
    {
        // Skip secret achievements if not unlocked
        if (achievement.m_bIsSecret && (!m_mPlayerAchievements.Contains(playerID) || 
            !m_mPlayerAchievements[playerID].m_mAchievements.Contains(achievement.m_sID) ||
            !m_mPlayerAchievements[playerID].m_mAchievements[achievement.m_sID].m_bUnlocked))
        {
            return;
        }
        
        // Send notification
        if (m_NotificationManager)
        {
            string message = string.Format("Achievement Unlocked: %1", achievement.m_sName);
            m_NotificationManager.SendPlayerNotification(playerID, message, 5.0, COLOR_GOLD);
            
            // Send achievement description in a follow-up message
            string description = string.Format("%1: %2", achievement.m_sName, achievement.m_sDescription);
            m_NotificationManager.SendPlayerNotification(playerID, description, 8.0, COLOR_WHITE);
        }
        
        // Show achievement UI if available
        if (m_UIManager)
        {
            m_UIManager.ShowAchievementUnlock(playerID, achievement);
        }
        
        // Log to console
        string playerName = GetPlayerNameFromID(playerID);
        Print(string.Format("[StatTracker] Player %1 (ID: %2) unlocked achievement: %3", playerName, playerID, achievement.m_sName));
        
        // Send webhook notification if enabled
        if (m_WebhookManager && m_Config.m_bEnableWebhooks)
        {
            string playerName = GetPlayerNameFromID(playerID);
            string payload = string.Format("Achievement Unlocked: %1 earned '%2' - %3", playerName, achievement.m_sName, achievement.m_sDescription);
            m_WebhookManager.SendWebhook("achievement", payload);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper methods
    
    // Check if a kill was a headshot
    protected bool IsHeadshotKill(IEntity victim, IEntity killer)
    {
        if (!victim)
            return false;
            
        SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(victim.FindComponent(SCR_CharacterDamageManagerComponent));
        if (!damageManager)
            return false;
            
        DamageHistory history = damageManager.GetDamageHistory();
        if (!history || history.GetRecordsCount() == 0)
            return false;
            
        DamageHistoryRecord lastRecord = history.GetLatestRecord();
        if (!lastRecord)
            return false;
            
        // Check hit zone
        return lastRecord.GetHitZone() == "Head";
    }
    
    // Track repeated kills against the same player
    protected int TrackRepeatedKills(int killerID, int victimID)
    {
        // In a real implementation, you would store this in a persistent data structure
        // This is a simplified version that would need to be expanded
        
        // For now, we'll just return 0 to indicate that tracking is needed
        return 0;
    }
    
    // Check for Last Stand achievement (kill while low health)
    protected void CheckLastStandAchievement(int playerID, IEntity playerEntity)
    {
        if (!playerEntity)
            return;
            
        SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(playerEntity.FindComponent(SCR_CharacterDamageManagerComponent));
        if (!damageManager)
            return;
            
        // Check if player has low health
        float healthPercentage = damageManager.GetHealthScaled() * 100;
        if (healthPercentage <= 20)
        {
            // In a real implementation, you would track the number of kills while at low health
            // For now, we'll just increment a counter that's reset on player death
            
            // For this example, let's assume the player has killed 3 enemies while at low health
            if (Math.GetRandomInt(0, 10) == 0) // Simulate a 10% chance of triggering (for demonstration)
            {
                UpdateAchievementProgress(playerID, "LAST_STAND", 1);
            }
        }
    }
    
    // Check for Lucky Shot achievement (kill while blinded)
    protected void CheckLuckyShotAchievement(int playerID, IEntity playerEntity)
    {
        if (!playerEntity)
            return;
            
        SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(playerEntity.FindComponent(SCR_CharacterControllerComponent));
        if (!controller)
            return;
            
        // Check if player is affected by flashbang
        // This is a simplified check - in a real implementation you would need to track flashbang effects
        bool isFlashbanged = controller.IsBlinded();
        
        if (isFlashbanged)
        {
            UpdateAchievementProgress(playerID, "LUCKY_SHOT", 1);
        }
    }
    
    // Get player name from ID
    protected string GetPlayerNameFromID(int playerID)
    {
        string playerName = "Unknown Player";
        PlayerManager playerManager = GetGame().GetPlayerManager();
        if (playerManager)
        {
            playerName = playerManager.GetPlayerName(playerID);
        }
        return playerName;
    }
    
    //------------------------------------------------------------------------------------------------
    // Public methods
    
    // Check if player has unlocked an achievement
    bool HasPlayerUnlockedAchievement(int playerID, string achievementID)
    {
        if (!m_mPlayerAchievements.Contains(playerID))
            return false;
            
        PlayerAchievements playerAchievements = m_mPlayerAchievements[playerID];
        
        if (!playerAchievements.m_mAchievements.Contains(achievementID))
            return false;
            
        return playerAchievements.m_mAchievements[achievementID].m_bUnlocked;
    }
    
    // Get player achievement progress
    int GetPlayerAchievementProgress(int playerID, string achievementID)
    {
        if (!m_mPlayerAchievements.Contains(playerID))
            return 0;
            
        PlayerAchievements playerAchievements = m_mPlayerAchievements[playerID];
        
        if (!playerAchievements.m_mAchievements.Contains(achievementID))
            return 0;
            
        return playerAchievements.m_mAchievements[achievementID].m_iProgress;
    }
    
    // Get all unlocked achievements for a player
    array<string> GetPlayerUnlockedAchievements(int playerID)
    {
        array<string> unlockedAchievements = new array<string>();
        
        if (!m_mPlayerAchievements.Contains(playerID))
            return unlockedAchievements;
            
        PlayerAchievements playerAchievements = m_mPlayerAchievements[playerID];
        
        foreach (string achievementID, PlayerAchievement achievement : playerAchievements.m_mAchievements)
        {
            if (achievement.m_bUnlocked)
            {
                unlockedAchievements.Insert(achievementID);
            }
        }
        
        return unlockedAchievements;
    }
    
    // Get total achievement score for a player
    int GetPlayerAchievementScore(int playerID)
    {
        int totalScore = 0;
        
        if (!m_mPlayerAchievements.Contains(playerID))
            return totalScore;
            
        PlayerAchievements playerAchievements = m_mPlayerAchievements[playerID];
        
        foreach (string achievementID, PlayerAchievement achievement : playerAchievements.m_mAchievements)
        {
            if (achievement.m_bUnlocked && m_mAchievements.Contains(achievementID))
            {
                totalScore += m_mAchievements[achievementID].m_iScoreValue;
            }
        }
        
        return totalScore;
    }
    
    // Get all achievement definitions
    map<string, ref Achievement> GetAllAchievements()
    {
        return m_mAchievements;
    }
    
    // Get visible achievements for a player (excludes secret achievements that aren't unlocked)
    array<ref Achievement> GetVisibleAchievements(int playerID)
    {
        array<ref Achievement> visibleAchievements = new array<ref Achievement>();
        
        foreach (string achievementID, Achievement achievement : m_mAchievements)
        {
            // Skip secret achievements that aren't unlocked
            if (achievement.m_bIsSecret && !HasPlayerUnlockedAchievement(playerID, achievementID))
                continue;
                
            visibleAchievements.Insert(achievement);
        }
        
        return visibleAchievements;
    }
    
    // Get achievement leaderboard data (players sorted by achievement score)
    map<int, int> GetAchievementLeaderboard()
    {
        map<int, int> leaderboard = new map<int, int>();
        
        foreach (int playerID, PlayerAchievements playerAchievements : m_mPlayerAchievements)
        {
            int score = GetPlayerAchievementScore(playerID);
            leaderboard.Insert(playerID, score);
        }
        
        return leaderboard;
    }
    
    // Check achievement progress for a specific event type
    void CheckObjectiveCaptureAchievements(int playerID)
    {
        UpdateAchievementProgress(playerID, "CAPTOR", 1);
        UpdateAchievementProgress(playerID, "MASTER_CAPTOR", 1);
    }
    
    void CheckSupplyDeliveryAchievements(int playerID)
    {
        UpdateAchievementProgress(playerID, "SUPPLY_RUNNER", 1);
    }
    
    void CheckHealTeammateAchievements(int playerID)
    {
        UpdateAchievementProgress(playerID, "MEDIC", 1);
    }
    
    void CheckReviveTeammateAchievements(int playerID)
    {
        UpdateAchievementProgress(playerID, "GOOD_SAMARITAN", 1);
    }
    
    void CheckRankProgressionAchievements(int playerID, int newRank)
    {
        if (newRank >= 2)
        {
            UpdateAchievementProgress(playerID, "RANK_UP", 1);
        }
        
        if (newRank >= 5)
        {
            UpdateAchievementProgress(playerID, "VETERAN_RANK", 1);
        }
        
        if (newRank >= 8)
        {
            UpdateAchievementProgress(playerID, "ELITE_RANK", 1);
        }
        
        if (newRank >= 10)
        {
            UpdateAchievementProgress(playerID, "MASTER_RANK", 1);
        }
    }
    
    void CheckVehicleKillAchievements(int playerID, string vehicleType)
    {
        if (vehicleType.Contains("Tank"))
        {
            UpdateAchievementProgress(playerID, "TANK_BUSTER", 1);
        }
        
        if (vehicleType.Contains("Air") || vehicleType.Contains("Helicopter") || vehicleType.Contains("Plane"))
        {
            UpdateAchievementProgress(playerID, "ACE_PILOT", 1);
        }
    }
    
    void CheckVehicleRoadKillAchievements(int playerID)
    {
        UpdateAchievementProgress(playerID, "ROAD_RAGE", 1);
    }
    
    // Check for survival time achievement
    void CheckSurvivalAchievements(int playerID, float survivalTime)
    {
        if (survivalTime >= 1800) // 30 minutes
        {
            UpdateAchievementProgress(playerID, "SURVIVOR", 1);
        }
    }
} 