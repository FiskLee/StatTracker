// STS_ProgressionSystem.c
// Player progression system with experience, ranks, and unlockable perks

// Define player ranks/tiers
enum EPlayerRank
{
    RECRUIT,
    PRIVATE,
    CORPORAL,
    SERGEANT,
    STAFF_SERGEANT,
    LIEUTENANT,
    CAPTAIN,
    MAJOR,
    COLONEL,
    GENERAL
}

// Define unlockable perks
enum EPlayerPerk
{
    NONE,
    FAST_RELOAD,
    REDUCED_RECOIL,
    EXTRA_AMMO,
    REDUCED_SWAY,
    FASTER_STAMINA_REGEN,
    EXTRA_INVENTORY_SPACE,
    FASTER_MOVEMENT,
    THERMAL_VISION,
    VEHICLE_REPAIR_SPECIALIST,
    MEDIC_SPECIALIST
}

// Player rank definition
class RankDefinition
{
    int m_iRankLevel;
    string m_sRankName;
    int m_iXPRequired;
    ref array<EPlayerPerk> m_aAvailablePerks;
    
    void RankDefinition(int level, string name, int xpRequired, array<EPlayerPerk> perks = null)
    {
        m_iRankLevel = level;
        m_sRankName = name;
        m_iXPRequired = xpRequired;
        
        if (perks)
            m_aAvailablePerks = perks;
        else
            m_aAvailablePerks = new array<EPlayerPerk>();
    }
}

// Player progression data
class PlayerProgression
{
    int m_iPlayerID;
    string m_sPlayerName;
    int m_iTotalXP;
    int m_iCurrentRank;
    ref array<EPlayerPerk> m_aUnlockedPerks;
    
    // Session-specific XP tracking
    int m_iSessionXP;
    int m_iLastKillXP;
    int m_iLastObjectiveXP;
    int m_iLastSurvivalXP;
    
    // Stats affecting progression
    int m_iKillStreak;
    float m_fLongestSurvivalTime;
    int m_iObjectivesCompleted;
    
    void PlayerProgression(int playerID, string playerName)
    {
        m_iPlayerID = playerID;
        m_sPlayerName = playerName;
        m_iTotalXP = 0;
        m_iCurrentRank = 0;
        m_aUnlockedPerks = new array<EPlayerPerk>();
        
        m_iSessionXP = 0;
        m_iLastKillXP = 0;
        m_iLastObjectiveXP = 0;
        m_iLastSurvivalXP = 0;
        
        m_iKillStreak = 0;
        m_fLongestSurvivalTime = 0;
        m_iObjectivesCompleted = 0;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"playerID\":" + m_iPlayerID.ToString() + ",";
        json += "\"playerName\":\"" + m_sPlayerName + "\",";
        json += "\"totalXP\":" + m_iTotalXP.ToString() + ",";
        json += "\"currentRank\":" + m_iCurrentRank.ToString() + ",";
        
        // Serialize unlocked perks
        json += "\"unlockedPerks\":[";
        for (int i = 0; i < m_aUnlockedPerks.Count(); i++)
        {
            json += m_aUnlockedPerks[i].ToString();
            if (i < m_aUnlockedPerks.Count() - 1)
                json += ",";
        }
        json += "],";
        
        json += "\"killStreak\":" + m_iKillStreak.ToString() + ",";
        json += "\"longestSurvivalTime\":" + m_fLongestSurvivalTime.ToString() + ",";
        json += "\"objectivesCompleted\":" + m_iObjectivesCompleted.ToString();
        json += "}";
        
        return json;
    }
    
    static PlayerProgression FromJSON(string json)
    {
        // Create default player progression
        PlayerProgression progression = new PlayerProgression(0, "");
        
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
            
            if (key == "playerID") progression.m_iPlayerID = value.ToInt();
            else if (key == "playerName") progression.m_sPlayerName = value;
            else if (key == "totalXP") progression.m_iTotalXP = value.ToInt();
            else if (key == "currentRank") progression.m_iCurrentRank = value.ToInt();
            else if (key == "killStreak") progression.m_iKillStreak = value.ToInt();
            else if (key == "longestSurvivalTime") progression.m_fLongestSurvivalTime = value.ToFloat();
            else if (key == "objectivesCompleted") progression.m_iObjectivesCompleted = value.ToInt();
            
            // Perks need special handling for the array
            // In a real implementation, use a proper JSON parser
        }
        
        return progression;
    }
}

class STS_ProgressionSystem
{
    // Singleton instance
    private static ref STS_ProgressionSystem s_Instance;
    
    // Rank definitions
    protected ref array<ref RankDefinition> m_aRankDefinitions = new array<ref RankDefinition>();
    
    // Player progression data
    protected ref map<int, ref PlayerProgression> m_mPlayerProgression = new map<int, ref PlayerProgression>();
    
    // XP settings
    protected int m_iBaseKillXP = 100;
    protected int m_iBaseObjectiveXP = 200;
    protected int m_iBaseSurvivalXP = 10; // Per minute
    protected float m_fKillStreakMultiplier = 0.1; // +10% per kill in streak
    
    // File path for saving progression data
    protected const string PROGRESSION_DATA_PATH = "$profile:StatTracker/progression.json";
    
    // References to other components
    protected ref STS_NotificationManager m_NotificationManager;
    protected ref STS_Config m_Config;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_ProgressionSystem()
    {
        Print("[StatTracker] Initializing Progression System");
        
        // Get references to other components
        m_NotificationManager = STS_NotificationManager.GetInstance();
        m_Config = STS_Config.GetInstance();
        
        // Initialize rank definitions
        InitializeRankDefinitions();
        
        // Load progression data
        LoadProgressionData();
        
        // Set up event listeners
        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (gameMode)
        {
            // Player connected/disconnected
            gameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);
            gameMode.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
            
            // Subscribe to kills
            SCR_KillManager killManager = SCR_KillManager.Instance();
            if (killManager)
            {
                killManager.GetOnPlayerKilled().Insert(OnPlayerKilled);
            }
            
            // Subscribe to objective events (capture flag, etc.)
            SCR_CaptureFlagGameModeComponent captureComponent = SCR_CaptureFlagGameModeComponent.Cast(gameMode.FindComponent(SCR_CaptureFlagGameModeComponent));
            if (captureComponent)
            {
                captureComponent.GetOnFlagCapturedInvoker().Insert(OnFlagCaptured);
            }
        }
        
        // Start periodic XP updates for survival time
        GetGame().GetCallqueue().CallLater(UpdateSurvivalXP, 60000, true); // Every minute
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_ProgressionSystem GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_ProgressionSystem();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize rank definitions
    protected void InitializeRankDefinitions()
    {
        m_aRankDefinitions.Clear();
        
        // Define ranks with required XP and available perks
        array<EPlayerPerk> rank1Perks = new array<EPlayerPerk>();
        rank1Perks.Insert(EPlayerPerk.NONE);
        m_aRankDefinitions.Insert(new RankDefinition(0, "Recruit", 0, rank1Perks));
        
        array<EPlayerPerk> rank2Perks = new array<EPlayerPerk>();
        rank2Perks.Insert(EPlayerPerk.FAST_RELOAD);
        m_aRankDefinitions.Insert(new RankDefinition(1, "Private", 1000, rank2Perks));
        
        array<EPlayerPerk> rank3Perks = new array<EPlayerPerk>();
        rank3Perks.Insert(EPlayerPerk.REDUCED_RECOIL);
        m_aRankDefinitions.Insert(new RankDefinition(2, "Corporal", 2500, rank3Perks));
        
        array<EPlayerPerk> rank4Perks = new array<EPlayerPerk>();
        rank4Perks.Insert(EPlayerPerk.EXTRA_AMMO);
        m_aRankDefinitions.Insert(new RankDefinition(3, "Sergeant", 5000, rank4Perks));
        
        array<EPlayerPerk> rank5Perks = new array<EPlayerPerk>();
        rank5Perks.Insert(EPlayerPerk.REDUCED_SWAY);
        m_aRankDefinitions.Insert(new RankDefinition(4, "Staff Sergeant", 10000, rank5Perks));
        
        array<EPlayerPerk> rank6Perks = new array<EPlayerPerk>();
        rank6Perks.Insert(EPlayerPerk.FASTER_STAMINA_REGEN);
        m_aRankDefinitions.Insert(new RankDefinition(5, "Lieutenant", 20000, rank6Perks));
        
        array<EPlayerPerk> rank7Perks = new array<EPlayerPerk>();
        rank7Perks.Insert(EPlayerPerk.EXTRA_INVENTORY_SPACE);
        m_aRankDefinitions.Insert(new RankDefinition(6, "Captain", 35000, rank7Perks));
        
        array<EPlayerPerk> rank8Perks = new array<EPlayerPerk>();
        rank8Perks.Insert(EPlayerPerk.FASTER_MOVEMENT);
        m_aRankDefinitions.Insert(new RankDefinition(7, "Major", 60000, rank8Perks));
        
        array<EPlayerPerk> rank9Perks = new array<EPlayerPerk>();
        rank9Perks.Insert(EPlayerPerk.VEHICLE_REPAIR_SPECIALIST);
        m_aRankDefinitions.Insert(new RankDefinition(8, "Colonel", 100000, rank9Perks));
        
        array<EPlayerPerk> rank10Perks = new array<EPlayerPerk>();
        rank10Perks.Insert(EPlayerPerk.THERMAL_VISION);
        rank10Perks.Insert(EPlayerPerk.MEDIC_SPECIALIST);
        m_aRankDefinitions.Insert(new RankDefinition(9, "General", 200000, rank10Perks));
    }
    
    //------------------------------------------------------------------------------------------------
    // Load progression data from file
    protected void LoadProgressionData()
    {
        // Clear existing data
        m_mPlayerProgression.Clear();
        
        // Check if file exists
        if (!FileIO.FileExists(PROGRESSION_DATA_PATH))
            return;
            
        // Read file
        string fileContent;
        if (!FileIO.FileReadAllText(PROGRESSION_DATA_PATH, fileContent))
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
            PlayerProgression progression = PlayerProgression.FromJSON(playerData);
            if (progression && progression.m_iPlayerID > 0)
            {
                m_mPlayerProgression.Insert(progression.m_iPlayerID, progression);
            }
        }
        
        Print(string.Format("[StatTracker] Loaded progression data for %1 players", m_mPlayerProgression.Count()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Save progression data to file
    void SaveProgressionData()
    {
        string fileContent = "{";
        
        int count = 0;
        foreach (int playerID, PlayerProgression progression : m_mPlayerProgression)
        {
            fileContent += "\"" + playerID.ToString() + "\":" + progression.ToJSON();
            
            if (count < m_mPlayerProgression.Count() - 1)
                fileContent += ",";
                
            count++;
        }
        
        fileContent += "}";
        
        // Ensure directory exists
        string dir = "$profile:StatTracker";
        FileIO.MakeDirectory(dir);
        
        // Write to file
        FileIO.FileWrite(PROGRESSION_DATA_PATH, fileContent);
        
        Print(string.Format("[StatTracker] Saved progression data for %1 players", m_mPlayerProgression.Count()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Add XP for a player
    int AddXP(int playerID, int xpAmount, string source = "")
    {
        // Ignore invalid player IDs
        if (playerID <= 0)
            return 0;
            
        // Get player progression data
        PlayerProgression progression;
        if (!m_mPlayerProgression.Contains(playerID))
        {
            // Create new progression data for this player
            string playerName = GetPlayerNameFromID(playerID);
            progression = new PlayerProgression(playerID, playerName);
            m_mPlayerProgression.Insert(playerID, progression);
        }
        else
        {
            progression = m_mPlayerProgression[playerID];
        }
        
        // Store previous rank
        int previousRank = progression.m_iCurrentRank;
        
        // Add XP
        progression.m_iTotalXP += xpAmount;
        progression.m_iSessionXP += xpAmount;
        
        // Update rank if necessary
        UpdatePlayerRank(progression);
        
        // Check if rank changed
        if (progression.m_iCurrentRank > previousRank)
        {
            // Player ranked up, notify them
            if (m_NotificationManager)
            {
                string rankName = m_aRankDefinitions[progression.m_iCurrentRank].m_sRankName;
                string message = string.Format("RANK UP! You are now %1", rankName);
                m_NotificationManager.SendPlayerNotification(playerID, message, 5.0, COLOR_GREEN);
                
                // Check for unlocked perks
                array<EPlayerPerk> newPerks = m_aRankDefinitions[progression.m_iCurrentRank].m_aAvailablePerks;
                if (newPerks && newPerks.Count() > 0)
                {
                    foreach (EPlayerPerk perk : newPerks)
                    {
                        if (perk != EPlayerPerk.NONE && !progression.m_aUnlockedPerks.Contains(perk))
                        {
                            progression.m_aUnlockedPerks.Insert(perk);
                            string perkName = GetPerkName(perk);
                            string perkMessage = string.Format("New perk unlocked: %1", perkName);
                            m_NotificationManager.SendPlayerNotification(playerID, perkMessage, 5.0, COLOR_BLUE);
                        }
                    }
                }
            }
        }
        
        // Save progression data periodically
        SaveProgressionData();
        
        // Return the amount of XP added
        return xpAmount;
    }
    
    //------------------------------------------------------------------------------------------------
    // Update player rank based on XP
    protected void UpdatePlayerRank(PlayerProgression progression)
    {
        // Find the highest rank the player qualifies for
        int newRank = 0;
        
        for (int i = m_aRankDefinitions.Count() - 1; i >= 0; i--)
        {
            if (progression.m_iTotalXP >= m_aRankDefinitions[i].m_iXPRequired)
            {
                newRank = i;
                break;
            }
        }
        
        progression.m_iCurrentRank = newRank;
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a player connects
    protected void OnPlayerConnected(int playerID)
    {
        // Check if we have progression data for this player
        if (!m_mPlayerProgression.Contains(playerID))
        {
            // Create new progression data
            string playerName = GetPlayerNameFromID(playerID);
            PlayerProgression progression = new PlayerProgression(playerID, playerName);
            m_mPlayerProgression.Insert(playerID, progression);
        }
        
        // Welcome message with rank info
        if (m_NotificationManager)
        {
            PlayerProgression progression = m_mPlayerProgression[playerID];
            string rankName = m_aRankDefinitions[progression.m_iCurrentRank].m_sRankName;
            string message = string.Format("Welcome back! Your current rank is %1 with %2 XP", rankName, progression.m_iTotalXP);
            m_NotificationManager.SendPlayerNotification(playerID, message, 5.0, COLOR_GREEN);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a player disconnects
    protected void OnPlayerDisconnected(int playerID)
    {
        // Save progression data
        SaveProgressionData();
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
        
        // Get victim info to determine kill type
        bool isAIKill = !victim.GetController();
        bool isVehicleKill = false;
        bool isAirKill = false;
        
        // Check if victim is a vehicle
        if (IsVehicle(victim))
        {
            isVehicleKill = true;
            
            // Check if it's an aircraft
            if (IsAircraft(victim))
            {
                isAirKill = true;
            }
        }
        
        // Calculate XP based on kill type
        int xpEarned = m_iBaseKillXP;
        string xpSource = "Kill";
        
        if (isAIKill)
        {
            xpEarned = Math.Round(m_iBaseKillXP * 0.5); // Less XP for AI kills
            xpSource = "AI Kill";
        }
        else if (isVehicleKill)
        {
            if (isAirKill)
            {
                xpEarned = Math.Round(m_iBaseKillXP * 3.0); // More XP for aircraft kills
                xpSource = "Aircraft Kill";
            }
            else
            {
                xpEarned = Math.Round(m_iBaseKillXP * 2.0); // More XP for vehicle kills
                xpSource = "Vehicle Kill";
            }
        }
        
        // Apply kill streak bonus if applicable
        if (m_mPlayerProgression.Contains(killerID))
        {
            PlayerProgression progression = m_mPlayerProgression[killerID];
            progression.m_iKillStreak++;
            
            // Apply kill streak multiplier
            if (progression.m_iKillStreak > 1)
            {
                float multiplier = 1.0 + (progression.m_iKillStreak - 1) * m_fKillStreakMultiplier;
                xpEarned = Math.Round(xpEarned * multiplier);
                
                // Notify player of kill streak
                if (m_NotificationManager && progression.m_iKillStreak >= 3)
                {
                    string message = string.Format("%1 KILL STREAK! +%2%% XP", progression.m_iKillStreak, Math.Round((multiplier - 1.0) * 100));
                    m_NotificationManager.SendPlayerNotification(killerID, message, 3.0, COLOR_YELLOW);
                }
            }
            
            // Store last kill XP
            progression.m_iLastKillXP = xpEarned;
        }
        
        // Reset kill streak for the victim if it's a player
        if (!isAIKill && victim)
        {
            PlayerController victimController = PlayerController.Cast(victim.GetController());
            if (victimController)
            {
                int victimID = victimController.GetPlayerId();
                if (m_mPlayerProgression.Contains(victimID))
                {
                    m_mPlayerProgression[victimID].m_iKillStreak = 0;
                }
            }
        }
        
        // Add XP with notification
        AddXP(killerID, xpEarned, xpSource);
        
        // Notify player of XP earned
        if (m_NotificationManager)
        {
            string message = string.Format("+%1 XP (%2)", xpEarned, xpSource);
            m_NotificationManager.SendPlayerNotification(killerID, message, 3.0, COLOR_YELLOW);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a flag is captured
    protected void OnFlagCaptured(SCR_CaptureAreaComponent captureArea, SCR_CaptureAreaCapturingFactionSwitchedParams params)
    {
        // Only process on server
        if (!Replication.IsServer())
            return;
            
        // Get the faction that captured the flag
        Faction capturingFaction = params.m_FactionCapturedBy;
        if (!capturingFaction)
            return;
            
        // Calculate objective XP
        int objectiveXP = m_iBaseObjectiveXP;
        
        // Find players involved in the capture
        array<int> playersInvolvedIDs = FindPlayersInArea(captureArea.GetOwner().GetOrigin(), 100.0, capturingFaction);
        
        // Award XP to all players involved
        foreach (int playerID : playersInvolvedIDs)
        {
            // Add to objectives completed counter
            if (m_mPlayerProgression.Contains(playerID))
            {
                m_mPlayerProgression[playerID].m_iObjectivesCompleted++;
                m_mPlayerProgression[playerID].m_iLastObjectiveXP = objectiveXP;
            }
            
            // Add XP with notification
            AddXP(playerID, objectiveXP, "Flag Capture");
            
            // Notify player
            if (m_NotificationManager)
            {
                string message = string.Format("+%1 XP (Flag Capture)", objectiveXP);
                m_NotificationManager.SendPlayerNotification(playerID, message, 3.0, COLOR_YELLOW);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Find players in an area belonging to a specific faction
    protected array<int> FindPlayersInArea(vector position, float radius, Faction faction)
    {
        array<int> playerIDs = new array<int>();
        
        // Get all players in the game
        array<int> allPlayerIDs = GetGame().GetPlayerManager().GetAllPlayers();
        
        foreach (int playerID : allPlayerIDs)
        {
            IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
            if (!playerEntity)
                continue;
                
            // Check faction
            FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(playerEntity.FindComponent(FactionAffiliationComponent));
            if (!factionComponent || factionComponent.GetAffiliatedFaction() != faction)
                continue;
                
            // Check distance
            float distance = vector.Distance(playerEntity.GetOrigin(), position);
            if (distance <= radius)
            {
                playerIDs.Insert(playerID);
            }
        }
        
        return playerIDs;
    }
    
    //------------------------------------------------------------------------------------------------
    // Update survival XP for all players (called periodically)
    protected void UpdateSurvivalXP()
    {
        // Only process on server
        if (!Replication.IsServer())
            return;
            
        // Get all active players
        array<int> playerIDs = GetGame().GetPlayerManager().GetAllPlayers();
        
        foreach (int playerID : playerIDs)
        {
            // Skip inactive players
            IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
            if (!playerEntity)
                continue;
                
            // Check if player is alive
            SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(playerEntity.FindComponent(SCR_CharacterDamageManagerComponent));
            if (!damageManager || damageManager.GetState() == EDamageState.DESTROYED)
                continue;
                
            // Add survival XP
            if (m_mPlayerProgression.Contains(playerID))
            {
                m_mPlayerProgression[playerID].m_iLastSurvivalXP = m_iBaseSurvivalXP;
            }
            
            AddXP(playerID, m_iBaseSurvivalXP, "Survival");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper functions
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
    
    protected bool IsVehicle(IEntity entity)
    {
        if (!entity)
            return false;
            
        // Check for vehicle component
        return entity.FindComponent(VehicleControllerComponent) != null;
    }
    
    protected bool IsAircraft(IEntity entity)
    {
        if (!entity)
            return false;
            
        // Simple check based on vehicle tags or type
        // More complex implementation would check specific vehicle types
        SCR_VehicleSpawnerComponent vehicleSpawner = SCR_VehicleSpawnerComponent.Cast(entity.FindComponent(SCR_VehicleSpawnerComponent));
        if (vehicleSpawner)
        {
            string vehicleType = vehicleSpawner.GetVehicleType();
            return vehicleType.Contains("Air") || vehicleType.Contains("Helicopter") || vehicleType.Contains("Plane");
        }
        
        return false;
    }
    
    protected string GetPerkName(EPlayerPerk perk)
    {
        switch (perk)
        {
            case EPlayerPerk.FAST_RELOAD: return "Fast Reload";
            case EPlayerPerk.REDUCED_RECOIL: return "Reduced Recoil";
            case EPlayerPerk.EXTRA_AMMO: return "Extra Ammo";
            case EPlayerPerk.REDUCED_SWAY: return "Reduced Sway";
            case EPlayerPerk.FASTER_STAMINA_REGEN: return "Faster Stamina Regeneration";
            case EPlayerPerk.EXTRA_INVENTORY_SPACE: return "Extra Inventory Space";
            case EPlayerPerk.FASTER_MOVEMENT: return "Faster Movement";
            case EPlayerPerk.THERMAL_VISION: return "Thermal Vision";
            case EPlayerPerk.VEHICLE_REPAIR_SPECIALIST: return "Vehicle Repair Specialist";
            case EPlayerPerk.MEDIC_SPECIALIST: return "Medic Specialist";
            default: return "Unknown Perk";
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Public accessor methods
    
    // Get player rank name
    string GetPlayerRankName(int playerID)
    {
        if (!m_mPlayerProgression.Contains(playerID))
            return "Unranked";
            
        PlayerProgression progression = m_mPlayerProgression[playerID];
        return m_aRankDefinitions[progression.m_iCurrentRank].m_sRankName;
    }
    
    // Get player total XP
    int GetPlayerTotalXP(int playerID)
    {
        if (!m_mPlayerProgression.Contains(playerID))
            return 0;
            
        return m_mPlayerProgression[playerID].m_iTotalXP;
    }
    
    // Get player session XP
    int GetPlayerSessionXP(int playerID)
    {
        if (!m_mPlayerProgression.Contains(playerID))
            return 0;
            
        return m_mPlayerProgression[playerID].m_iSessionXP;
    }
    
    // Get player rank level
    int GetPlayerRankLevel(int playerID)
    {
        if (!m_mPlayerProgression.Contains(playerID))
            return 0;
            
        return m_mPlayerProgression[playerID].m_iCurrentRank;
    }
    
    // Get XP needed for next rank
    int GetXPForNextRank(int playerID)
    {
        if (!m_mPlayerProgression.Contains(playerID))
            return m_aRankDefinitions[1].m_iXPRequired;
            
        PlayerProgression progression = m_mPlayerProgression[playerID];
        
        // Check if player is at max rank
        if (progression.m_iCurrentRank >= m_aRankDefinitions.Count() - 1)
            return 0;
            
        return m_aRankDefinitions[progression.m_iCurrentRank + 1].m_iXPRequired - progression.m_iTotalXP;
    }
    
    // Get player unlocked perks
    array<EPlayerPerk> GetPlayerPerks(int playerID)
    {
        if (!m_mPlayerProgression.Contains(playerID))
            return new array<EPlayerPerk>();
            
        return m_mPlayerProgression[playerID].m_aUnlockedPerks;
    }
    
    // Check if player has a specific perk
    bool HasPlayerPerk(int playerID, EPlayerPerk perk)
    {
        if (!m_mPlayerProgression.Contains(playerID))
            return false;
            
        return m_mPlayerProgression[playerID].m_aUnlockedPerks.Contains(perk);
    }
    
    // Get all player progression data for admin/dashboard purposes
    map<int, ref PlayerProgression> GetAllPlayerProgressions()
    {
        return m_mPlayerProgression;
    }
    
    // Get rank definitions for UI display
    array<ref RankDefinition> GetRankDefinitions()
    {
        return m_aRankDefinitions;
    }
    
    // Admin function to adjust player XP
    void AdminAdjustPlayerXP(int playerID, int xpAmount)
    {
        if (xpAmount == 0)
            return;
            
        // Add/subtract XP
        if (xpAmount > 0)
        {
            AddXP(playerID, xpAmount, "Admin Adjustment");
        }
        else
        {
            // Handle negative XP (reduction)
            if (m_mPlayerProgression.Contains(playerID))
            {
                PlayerProgression progression = m_mPlayerProgression[playerID];
                progression.m_iTotalXP = Math.Max(0, progression.m_iTotalXP + xpAmount);
                progression.m_iSessionXP = Math.Max(0, progression.m_iSessionXP + xpAmount);
                
                // Update rank if necessary
                UpdatePlayerRank(progression);
                
                // Save changes
                SaveProgressionData();
            }
        }
        
        // Notify player
        if (m_NotificationManager)
        {
            string prefix = xpAmount > 0 ? "+" : "";
            string message = string.Format("%1%2 XP (Admin Adjustment)", prefix, xpAmount);
            m_NotificationManager.SendPlayerNotification(playerID, message, 5.0, COLOR_YELLOW);
        }
    }
} 
} 