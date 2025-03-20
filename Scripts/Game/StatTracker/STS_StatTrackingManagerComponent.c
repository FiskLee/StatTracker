// STS_StatTrackingManagerComponent.c
// Manager component to track all players' stats and handle global events

class STS_StatTrackingManagerComponent : ScriptComponent
{
    // List of all registered players
    protected ref array<STS_StatTrackingComponent> m_aPlayers = new array<STS_StatTrackingComponent>();
    
    // Reference to the HUD component
    protected STS_ScoreboardHUD m_ScoreboardHUD;
    
    // Timer for periodic save operations
    protected float m_fLastSaveTime = 0;
    protected const float SAVE_INTERVAL = 60.0; // Save every 60 seconds
    
    // File paths for stats
    protected string m_sStatsFilePath = "$profile:StatTracker/player_stats.json";
    protected string m_sSessionFilePath = "$profile:StatTracker/current_session.json";
    
    // Player stats cache for load/save operations (mapped by player UID)
    protected ref map<string, ref STS_PlayerStats> m_mPlayerStatsCache = new map<string, ref STS_PlayerStats>();
    
    // RPCs for client-server communication
    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_UpdateStats(array<int> playerIDs, array<ref STS_PlayerStats> playerStats, array<string> playerNames)
    {
        // Update HUD with latest stats
        if (m_ScoreboardHUD)
            m_ScoreboardHUD.UpdateScoreboard(playerIDs, playerStats, playerNames);
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        
        // Only do this on server
        if (!Replication.IsServer())
            return;
        
        // Create directory for saving stats
        string dir = "$profile:StatTracker";
        FileIO.MakeDirectory(dir);
        
        // Load existing player stats from file
        LoadPlayerStats();
        
        // Log server start
        Print("[StatTracker] Stats tracking system initialized.");
            
        // Subscribe to game events
        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (gameMode)
        {
            // Player connected/disconnected events
            gameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);
            gameMode.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
            
            // Base capture events - use the appropriate game-specific event if available
            SCR_CaptureFlagGameModeComponent captureComponent = SCR_CaptureFlagGameModeComponent.Cast(gameMode.FindComponent(SCR_CaptureFlagGameModeComponent));
            if (captureComponent)
            {
                captureComponent.GetOnFlagCapturedInvoker().Insert(OnFlagCaptured);
            }
        }
        
        // Find HUD component
        m_ScoreboardHUD = STS_ScoreboardHUD.Cast(GetGame().GetHUD().FindHandler(STS_ScoreboardHUD));
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnDelete(IEntity owner)
    {
        // Only do this on server
        if (Replication.IsServer())
        {
            // Save stats before shutting down
            SaveAllPlayerStats();
            
            // Unsubscribe from game events
            SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
            if (gameMode)
            {
                gameMode.GetOnPlayerConnected().Remove(OnPlayerConnected);
                gameMode.GetOnPlayerDisconnected().Remove(OnPlayerDisconnected);
                
                // Base capture events
                SCR_CaptureFlagGameModeComponent captureComponent = SCR_CaptureFlagGameModeComponent.Cast(gameMode.FindComponent(SCR_CaptureFlagGameModeComponent));
                if (captureComponent)
                {
                    captureComponent.GetOnFlagCapturedInvoker().Remove(OnFlagCaptured);
                }
            }
        }
        
        super.OnDelete(owner);
    }
    
    //------------------------------------------------------------------------------------------------
    override void EOnFrame(IEntity owner, float timeSlice)
    {
        super.EOnFrame(owner, timeSlice);
        
        // Only do this on server
        if (!Replication.IsServer())
            return;
            
        // Auto-save stats periodically
        float currentTime = System.GetTickCount() / 1000.0;
        if (currentTime - m_fLastSaveTime > SAVE_INTERVAL)
        {
            m_fLastSaveTime = currentTime;
            SaveAllPlayerStats();
            SaveCurrentSession();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when the statistics for a player have changed
    void OnStatsChanged(STS_StatTrackingComponent player)
    {
        // Only process on server
        if (!Replication.IsServer())
            return;
            
        // Output to console
        LogPlayerAction(player);
            
        // Broadcast updated stats to all clients
        BroadcastStats();
    }
    
    //------------------------------------------------------------------------------------------------
    // Log player action to console
    protected void LogPlayerAction(STS_StatTrackingComponent player)
    {
        if (!player || player.IsAI())
            return;
            
        STS_PlayerStats stats = player.GetStats();
        string output = string.Format("[StatTracker] Player: %1 (ID: %2, IP: %3) - K/D: %4/%5, Score: %6, Rank: %7", 
            player.GetPlayerName(), 
            player.GetPlayerID(),
            player.GetIPAddress(),
            stats.m_iKills,
            stats.m_iDeaths,
            stats.CalculateTotalScore(),
            stats.m_iRank);
            
        Print(output);
    }
    
    //------------------------------------------------------------------------------------------------
    // Broadcast all player stats to clients
    protected void BroadcastStats()
    {
        array<int> playerIDs = new array<int>();
        array<ref STS_PlayerStats> playerStats = new array<ref STS_PlayerStats>();
        array<string> playerNames = new array<string>();
        
        // Collect all player stats
        foreach (STS_StatTrackingComponent player : m_aPlayers)
        {
            playerIDs.Insert(player.GetPlayerID());
            playerStats.Insert(player.GetStats());
            playerNames.Insert(player.GetPlayerName());
        }
        
        // Send RPC to all clients
        RPC_UpdateStats(playerIDs, playerStats, playerNames);
    }
    
    //------------------------------------------------------------------------------------------------
    // Register a player with the manager
    void RegisterPlayer(STS_StatTrackingComponent player)
    {
        // Create local logger reference
        STS_LoggingSystem logger = STS_LoggingSystem.GetInstance();
        
        try
        {
            // Only process on server
            if (!Replication.IsServer())
            {
                logger.LogDebug("RegisterPlayer called on client - ignoring", "STS_StatTrackingManagerComponent", "RegisterPlayer");
                return;
            }
            
            // Validate player parameter
            if (!player)
            {
                logger.LogError("Attempted to register null player component", "STS_StatTrackingManagerComponent", "RegisterPlayer");
                return;
            }
            
            // Skip if already registered
            if (m_aPlayers.Contains(player))
            {
                logger.LogWarning(string.Format("Player component already registered: %1", player.GetPlayerName()), 
                    "STS_StatTrackingManagerComponent", "RegisterPlayer");
                return;
            }
            
            // Add player to the list
            m_aPlayers.Insert(player);
            
            // Set manager reference in player component
            player.SetManager(this);
            
            // Get player ID and name if applicable
            IEntity playerEntity = player.GetOwner();
            if (!playerEntity)
            {
                logger.LogError("Player component has null owner entity", "STS_StatTrackingManagerComponent", "RegisterPlayer");
                return;
            }
            
            PlayerController playerController = PlayerController.Cast(SCR_PlayerController.GetPlayerController(playerEntity));
            
            if (playerController)
            {
                // This is a player, not an AI
                int playerId = playerController.GetPlayerId();
                if (playerId < 0)
                {
                    logger.LogWarning(string.Format("Invalid player ID (%1) - using fallback ID", playerId), 
                        "STS_StatTrackingManagerComponent", "RegisterPlayer");
                    playerId = m_aPlayers.Count() + 1000; // Fallback ID
                }
                
                string playerName = playerController.GetPlayerName();
                if (playerName.IsEmpty())
                {
                    logger.LogWarning("Empty player name - using 'Unknown Player'", 
                        "STS_StatTrackingManagerComponent", "RegisterPlayer");
                    playerName = "Unknown Player";
                }
                
                string ipAddress = GetPlayerIP(playerId);
                if (ipAddress.IsEmpty())
                {
                    logger.LogInfo(string.Format("Could not get IP for player %1 (ID: %2) - using 'unknown'", 
                        playerName, playerId), "STS_StatTrackingManagerComponent", "RegisterPlayer");
                    ipAddress = "unknown";
                }
                
                player.SetPlayerID(playerId);
                player.SetPlayerName(playerName);
                player.SetIsAI(false);
                player.SetConnectionInfo(ipAddress);
                
                // Try to load previous stats
                string playerUID = GetPlayerUID(playerId);
                if (!playerUID.IsEmpty() && m_mPlayerStatsCache.Contains(playerUID))
                {
                    // Successfully loaded previous stats
                    if (LoadPlayerPreviousStats(player, playerUID))
                    {
                        logger.LogInfo(string.Format("Loaded previous stats for player %1 (UID: %2)", 
                            playerName, playerUID), "STS_StatTrackingManagerComponent", "RegisterPlayer");
                    }
                    else
                    {
                        logger.LogWarning(string.Format("Failed to load previous stats for player %1 (UID: %2)", 
                            playerName, playerUID), "STS_StatTrackingManagerComponent", "RegisterPlayer");
                    }
                }
                else
                {
                    // New player or stats not found
                    logger.LogInfo(string.Format("New player registered: %1 (ID: %2, IP: %3)", 
                        playerName, playerId, ipAddress), "STS_StatTrackingManagerComponent", "RegisterPlayer");
                }
            }
            else
            {
                // This is an AI
                player.SetPlayerID(-1); // Use negative IDs for AI
                player.SetPlayerName("AI " + m_aPlayers.Count().ToString());
                player.SetIsAI(true);
                
                logger.LogDebug(string.Format("Registered AI entity: %1", player.GetPlayerName()), 
                    "STS_StatTrackingManagerComponent", "RegisterPlayer");
            }
            
            // Broadcast updated stats
            BroadcastStats();
            
            logger.LogInfo(string.Format("Successfully registered player: %1", player.GetPlayerName()), 
                "STS_StatTrackingManagerComponent", "RegisterPlayer");
        }
        catch (Exception e)
        {
            if (logger)
            {
                logger.LogError(string.Format("Exception in RegisterPlayer: %1\nStacktrace: %2", 
                    e.ToString(), e.GetStackTrace()), "STS_StatTrackingManagerComponent", "RegisterPlayer");
            }
            else
            {
                Print(string.Format("[StatTracker] CRITICAL ERROR in RegisterPlayer: %1", e.ToString()), LogLevel.ERROR);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Unregister a player from the manager
    void UnregisterPlayer(STS_StatTrackingComponent player)
    {
        // Only process on server
        if (!Replication.IsServer())
            return;
            
        // Save this player's stats before unregistering
        if (!player.IsAI())
        {
            string playerUID = GetPlayerUID(player.GetPlayerID());
            if (playerUID)
            {
                STS_PlayerStats stats = player.GetStats();
                
                // Update the cache with current stats
                if (!m_mPlayerStatsCache.Contains(playerUID))
                    m_mPlayerStatsCache.Insert(playerUID, new STS_PlayerStats());
                    
                // Copy the current stats
                m_mPlayerStatsCache[playerUID] = stats;
                
                Print(string.Format("[StatTracker] Player %1 (ID: %2) disconnected. Session duration: %3 minutes", 
                    player.GetPlayerName(), 
                    player.GetPlayerID(),
                    Math.Round(player.GetSessionDuration() / 60)
                ));
            }
        }
            
        // Remove player from the list
        m_aPlayers.RemoveItem(player);
        
        // Broadcast updated stats
        BroadcastStats();
        
        // Save all stats after a player leaves
        SaveAllPlayerStats();
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a player connects
    void OnPlayerConnected(int playerId)
    {
        // Player component will register itself when initialized
        Print(string.Format("[StatTracker] Player connected with ID: %1", playerId));
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a player disconnects
    void OnPlayerDisconnected(int playerId)
    {
        // Only process on server
        if (!Replication.IsServer())
            return;
            
        // Find and remove the player
        STS_StatTrackingComponent playerComponent = null;
        foreach (STS_StatTrackingComponent player : m_aPlayers)
        {
            if (player.GetPlayerID() == playerId)
            {
                playerComponent = player;
                break;
            }
        }
        
        if (playerComponent)
            UnregisterPlayer(playerComponent);
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a flag is captured
    void OnFlagCaptured(SCR_CaptureArea area, SCR_ChimeraCharacter player, SCR_CaptureAreaOwnershipChange change)
    {
        // Only process on server
        if (!Replication.IsServer())
            return;
            
        // Skip if player is null
        if (!player)
            return;
            
        // Find the player component
        STS_StatTrackingComponent playerComponent = STS_StatTrackingComponent.Cast(player.FindComponent(STS_StatTrackingComponent));
        if (!playerComponent)
            return;
            
        // Update stats based on ownership change
        if (change == SCR_CaptureAreaOwnershipChange.FRIENDLY_CAPTURED)
        {
            // Player captured a base
            playerComponent.AddBaseCaptured();
            Print(string.Format("[StatTracker] Player %1 captured a base!", playerComponent.GetPlayerName()));
        }
        else if (change == SCR_CaptureAreaOwnershipChange.FRIENDLY_LOST)
        {
            // Player lost a base
            playerComponent.AddBaseLost();
            Print(string.Format("[StatTracker] Player %1 lost a base!", playerComponent.GetPlayerName()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Called to register a supply delivery
    void RegisterSupplyDelivery(IEntity player, int amount)
    {
        // Only process on server
        if (!Replication.IsServer())
            return;
            
        // Skip if player is null
        if (!player)
            return;
            
        // Find the player component
        STS_StatTrackingComponent playerComponent = STS_StatTrackingComponent.Cast(player.FindComponent(STS_StatTrackingComponent));
        if (!playerComponent)
            return;
            
        // Add supplies
        playerComponent.AddSuppliesDelivered(amount);
        Print(string.Format("[StatTracker] Player %1 delivered %2 supplies!", playerComponent.GetPlayerName(), amount));
    }
    
    //------------------------------------------------------------------------------------------------
    // Save all player stats to the database
    void SaveAllPlayerStats()
    {
        // Create local logger reference
        STS_LoggingSystem logger = STS_LoggingSystem.GetInstance();
        
        try
        {
            // Only process on server
            if (!Replication.IsServer())
            {
                logger.LogDebug("SaveAllPlayerStats called on client - ignoring", 
                    "STS_StatTrackingManagerComponent", "SaveAllPlayerStats");
                return;
            }
            
            // Get persistence manager
            STS_PersistenceManager persistenceManager = STS_PersistenceManager.GetInstance();
            if (!persistenceManager)
            {
                logger.LogError("Failed to get persistence manager - player stats won't be saved", 
                    "STS_StatTrackingManagerComponent", "SaveAllPlayerStats");
                return;
            }
            
            // Log start of save operation
            logger.LogInfo(string.Format("Saving stats for %1 players", m_aPlayers.Count()), 
                "STS_StatTrackingManagerComponent", "SaveAllPlayerStats");
            
            // Track success/failure counts
            int successCount = 0;
            int failureCount = 0;
            
            // Save each player's stats
            foreach (STS_StatTrackingComponent player : m_aPlayers)
            {
                // Skip AI players
                if (player.IsAI())
                    continue;
                
                // Get player info
                int playerId = player.GetPlayerID();
                string playerUID = GetPlayerUID(playerId);
                string playerName = player.GetPlayerName();
                
                // Skip if can't get player UID
                if (playerUID.IsEmpty())
                {
                    logger.LogWarning(string.Format("Cannot save stats for player %1 (ID: %2) - unable to get UID", 
                        playerName, playerId), "STS_StatTrackingManagerComponent", "SaveAllPlayerStats");
                    failureCount++;
                    continue;
                }
                
                // Get player stats
                STS_PlayerStats stats = player.GetStats();
                if (!stats)
                {
                    logger.LogWarning(string.Format("Cannot save stats for player %1 (ID: %2) - stats object is null", 
                        playerName, playerId), "STS_StatTrackingManagerComponent", "SaveAllPlayerStats");
                    failureCount++;
                    continue;
                }
                
                // Update session duration before saving
                stats.UpdateSessionDuration();
                
                // Save to database
                bool success = persistenceManager.SavePlayerStats(playerUID, playerName, stats);
                
                if (success)
                    successCount++;
                else
                    failureCount++;
            }
            
            // Log results
            logger.LogInfo(string.Format("Saved player stats: %1 successful, %2 failed", 
                successCount, failureCount), "STS_StatTrackingManagerComponent", "SaveAllPlayerStats");
        }
        catch (Exception e)
        {
            if (logger)
            {
                logger.LogError(string.Format("Exception in SaveAllPlayerStats: %1\nStacktrace: %2", 
                    e.ToString(), e.GetStackTrace()), "STS_StatTrackingManagerComponent", "SaveAllPlayerStats");
            }
            else
            {
                Print(string.Format("[StatTracker] CRITICAL ERROR in SaveAllPlayerStats: %1", e.ToString()), LogLevel.ERROR);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save current session data
    void SaveCurrentSession()
    {
        // Create local logger reference
        STS_LoggingSystem logger = STS_LoggingSystem.GetInstance();
        
        try
        {
            // Only process on server
            if (!Replication.IsServer())
                return;
            
            // Get persistence manager
            STS_PersistenceManager persistenceManager = STS_PersistenceManager.GetInstance();
            if (!persistenceManager)
            {
                logger.LogWarning("Failed to get persistence manager - current session won't be saved", 
                    "STS_StatTrackingManagerComponent", "SaveCurrentSession");
                return;
            }
            
            // Create a session snapshot
            JsonObjectRef sessionData = new JsonObjectRef();
            
            // Add session ID
            sessionData.AddString("sessionId", persistenceManager.GetCurrentSessionId());
            
            // Add timestamp
            sessionData.AddFloat("timestamp", System.GetTickCount() / 1000.0);
            
            // Add server name
            string serverName = "Unknown";
            sessionData.AddString("serverName", serverName);
            
            // Add player count
            sessionData.AddInt("playerCount", m_aPlayers.Count());
            
            // Add online players
            JsonArray playerIds = new JsonArray();
            JsonArray playerNames = new JsonArray();
            
            foreach (STS_StatTrackingComponent player : m_aPlayers)
            {
                // Skip AI
                if (player.IsAI())
                    continue;
                    
                playerIds.AddInt(player.GetPlayerID());
                playerNames.AddString(player.GetPlayerName());
            }
            
            sessionData.AddArray("playerIds", playerIds);
            sessionData.AddArray("playerNames", playerNames);
            
            // Run process to save session snapshot
            // This functionality would be implemented in a dedicated session manager
            // For now, log that we've created a session snapshot
            logger.LogInfo(string.Format("Session snapshot created - session ID: %1, players: %2", 
                persistenceManager.GetCurrentSessionId(), m_aPlayers.Count()), 
                "STS_StatTrackingManagerComponent", "SaveCurrentSession");
        }
        catch (Exception e)
        {
            if (logger)
            {
                logger.LogError(string.Format("Exception in SaveCurrentSession: %1\nStacktrace: %2", 
                    e.ToString(), e.GetStackTrace()), "STS_StatTrackingManagerComponent", "SaveCurrentSession");
            }
            else
            {
                Print(string.Format("[StatTracker] CRITICAL ERROR in SaveCurrentSession: %1", e.ToString()), LogLevel.ERROR);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load player stats from file
    protected void LoadPlayerStats()
    {
        // Only process on server
        if (!Replication.IsServer())
            return;
            
        // Clear cache
        m_mPlayerStatsCache.Clear();
        
        // Try to open file
        FileHandle file = FileIO.OpenFile(m_sStatsFilePath, FileMode.READ);
        if (!file)
        {
            Print(string.Format("[StatTracker] No player stats file found at %1, starting with empty stats", m_sStatsFilePath));
            return;
        }
        
        // Read file content
        string content;
        file.ReadToString(content);
        file.Close();
        
        // Parse JSON (simplified parsing for demonstration)
        // In a real implementation, use a proper JSON parser
        if (content.Length() == 0)
        {
            Print("[StatTracker] Stats file is empty");
            return;
        }
        
        // Check for start of players object
        int playersStart = content.IndexOf("\"players\":{");
        if (playersStart == -1)
        {
            Print("[StatTracker] Invalid stats file format");
            return;
        }
        
        // Extract players object
        string playersJson = content.Substring(playersStart + 10);
        int playersEnd = playersJson.LastIndexOf("}");
        if (playersEnd == -1)
        {
            Print("[StatTracker] Invalid stats file format (no closing })");
            return;
        }
        
        playersJson = playersJson.Substring(0, playersEnd);
        
        // Split by player UIDs
        bool parsingPlayer = false;
        string currentPlayerUID = "";
        string currentPlayerJson = "";
        int bracketCount = 0;
        
        for (int i = 0; i < playersJson.Length(); i++)
        {
            char c = playersJson[i];
            
            // Start of player UID
            if (c == '"' && !parsingPlayer && bracketCount == 0)
            {
                int uidEnd = playersJson.IndexOf("\":", i + 1);
                if (uidEnd != -1)
                {
                    currentPlayerUID = playersJson.Substring(i + 1, uidEnd - i - 1);
                    i = uidEnd + 1;
                    parsingPlayer = true;
                    currentPlayerJson = "";
                    continue;
                }
            }
            
            // Track JSON object nesting
            if (c == '{')
                bracketCount++;
            else if (c == '}')
                bracketCount--;
                
            // If parsing player and we're at the end of the player's JSON object
            if (parsingPlayer)
            {
                currentPlayerJson += c;
                
                if (bracketCount == 0)
                {
                    // Parse this player's stats
                    STS_PlayerStats stats = new STS_PlayerStats();
                    stats.FromJSON(currentPlayerJson);
                    m_mPlayerStatsCache.Insert(currentPlayerUID, stats);
                    
                    parsingPlayer = false;
                }
            }
        }
        
        Print(string.Format("[StatTracker] Loaded stats for %1 players from %2", m_mPlayerStatsCache.Count(), m_sStatsFilePath));
    }
    
    //------------------------------------------------------------------------------------------------
    // Load previous stats for a player
    protected bool LoadPlayerPreviousStats(STS_StatTrackingComponent player, string playerUID)
    {
        // Get logger
        STS_LoggingSystem logger = STS_LoggingSystem.GetInstance();
        
        try
        {
            // Validate parameters
            if (!player)
            {
                logger.LogError("LoadPlayerPreviousStats called with null player", 
                    "STS_StatTrackingManagerComponent", "LoadPlayerPreviousStats");
                return false;
            }
            
            if (playerUID.IsEmpty())
            {
                logger.LogWarning(string.Format("LoadPlayerPreviousStats called with empty playerUID for player %1", 
                    player.GetPlayerName()), "STS_StatTrackingManagerComponent", "LoadPlayerPreviousStats");
                return false;
            }
            
            // Get persistence manager
            STS_PersistenceManager persistenceManager = STS_PersistenceManager.GetInstance();
            if (!persistenceManager)
            {
                logger.LogError("Failed to get persistence manager - previous stats cannot be loaded", 
                    "STS_StatTrackingManagerComponent", "LoadPlayerPreviousStats");
                return false;
            }
            
            // Load player's previous stats from database
            STS_PlayerStats previousStats = persistenceManager.LoadPlayerStats(playerUID);
            
            // If no previous stats found, nothing more to do
            if (!previousStats)
                return false;
            
            // Get current stats
            STS_PlayerStats currentStats = player.GetStats();
            if (!currentStats)
            {
                logger.LogWarning(string.Format("Failed to get current stats for player %1 - creating new stats", 
                    player.GetPlayerName()), "STS_StatTrackingManagerComponent", "LoadPlayerPreviousStats");
                
                currentStats = new STS_PlayerStats();
                player.SetStats(currentStats);
            }
            
            // Copy data from previous stats
            currentStats.m_iKills = previousStats.m_iKills;
            currentStats.m_iDeaths = previousStats.m_iDeaths;
            currentStats.m_iBasesLost = previousStats.m_iBasesLost;
            currentStats.m_iBasesCaptured = previousStats.m_iBasesCaptured;
            currentStats.m_iTotalXP = previousStats.m_iTotalXP;
            currentStats.m_iRank = previousStats.m_iRank;
            currentStats.m_iSuppliesDelivered = previousStats.m_iSuppliesDelivered;
            currentStats.m_iSupplyDeliveryCount = previousStats.m_iSupplyDeliveryCount;
            currentStats.m_iAIKills = previousStats.m_iAIKills;
            currentStats.m_iVehicleKills = previousStats.m_iVehicleKills;
            currentStats.m_iAirKills = previousStats.m_iAirKills;
            
            // Add session time to total playtime
            currentStats.m_fTotalPlaytime = previousStats.m_fTotalPlaytime;
            currentStats.m_fLastSessionDuration = previousStats.m_fLastSessionDuration;
            
            // Copy killed by information if available
            if (previousStats.m_aKilledBy && previousStats.m_aKilledBy.Count() > 0)
            {
                // Create arrays if they don't exist
                if (!currentStats.m_aKilledBy)
                    currentStats.m_aKilledBy = new array<string>();
                    
                if (!currentStats.m_aKilledByWeapon)
                    currentStats.m_aKilledByWeapon = new array<string>();
                    
                if (!currentStats.m_aKilledByTeam)
                    currentStats.m_aKilledByTeam = new array<int>();
                
                // Clear existing arrays
                currentStats.m_aKilledBy.Clear();
                currentStats.m_aKilledByWeapon.Clear();
                currentStats.m_aKilledByTeam.Clear();
                
                // Copy data
                for (int i = 0; i < previousStats.m_aKilledBy.Count(); i++)
                {
                    currentStats.m_aKilledBy.Insert(previousStats.m_aKilledBy[i]);
                    
                    if (i < previousStats.m_aKilledByWeapon.Count())
                        currentStats.m_aKilledByWeapon.Insert(previousStats.m_aKilledByWeapon[i]);
                    else
                        currentStats.m_aKilledByWeapon.Insert("Unknown");
                    
                    if (i < previousStats.m_aKilledByTeam.Count())
                        currentStats.m_aKilledByTeam.Insert(previousStats.m_aKilledByTeam[i]);
                    else
                        currentStats.m_aKilledByTeam.Insert(-1);
                }
            }
            
            // Update rank based on loaded XP
            player.UpdateRank();
            
            logger.LogInfo(string.Format("Successfully loaded previous stats for player %1 (UID: %2) - Kills: %3, XP: %4", 
                player.GetPlayerName(), playerUID, currentStats.m_iKills, currentStats.m_iTotalXP), 
                "STS_StatTrackingManagerComponent", "LoadPlayerPreviousStats");
            
            return true;
        }
        catch (Exception e)
        {
            if (logger)
            {
                logger.LogError(string.Format("Exception in LoadPlayerPreviousStats for player %1 (UID: %2): %3\nStacktrace: %4", 
                    player.GetPlayerName(), playerUID, e.ToString(), e.GetStackTrace()), 
                    "STS_StatTrackingManagerComponent", "LoadPlayerPreviousStats");
            }
            else
            {
                Print(string.Format("[StatTracker] CRITICAL ERROR in LoadPlayerPreviousStats: %1", e.ToString()), LogLevel.ERROR);
            }
            
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get player UID from player ID - this is simplified for the example
    // In a real implementation, use a more robust method to get permanent player identifiers
    protected string GetPlayerUID(int playerId)
    {
        if (playerId < 0)
            return "";
            
        // Try to get a more permanent ID for the player
        // For this example, we'll just use the player ID as string
        return playerId.ToString();
    }
    
    //------------------------------------------------------------------------------------------------
    // Get player IP address from player ID - simplified for example
    protected string GetPlayerIP(int playerId)
    {
        // In a real implementation, you would get this from the game networking system
        // Simplified version for the example
        return "127.0.0.1"; // Placeholder
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the stats of a specific player
    STS_PlayerStats GetPlayerStats(int playerId)
    {
        foreach (STS_StatTrackingComponent player : m_aPlayers)
        {
            if (player.GetPlayerID() == playerId)
                return player.GetStats();
        }
        
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all registered players
    array<STS_StatTrackingComponent> GetAllPlayers()
    {
        return m_aPlayers;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the top N players sorted by score
    array<STS_StatTrackingComponent> GetTopPlayers(int count = 10)
    {
        array<STS_StatTrackingComponent> sortedPlayers = new array<STS_StatTrackingComponent>();
        sortedPlayers.Copy(m_aPlayers);
        
        // Sort players by score (descending)
        for (int i = 0; i < sortedPlayers.Count(); i++)
        {
            for (int j = i + 1; j < sortedPlayers.Count(); j++)
            {
                if (sortedPlayers[i].GetStats().CalculateTotalScore() < sortedPlayers[j].GetStats().CalculateTotalScore())
                {
                    STS_StatTrackingComponent temp = sortedPlayers[i];
                    sortedPlayers[i] = sortedPlayers[j];
                    sortedPlayers[j] = temp;
                }
            }
        }
        
        // Return up to 'count' players
        if (sortedPlayers.Count() > count)
            sortedPlayers.Resize(count);
            
        return sortedPlayers;
    }
} 
} 