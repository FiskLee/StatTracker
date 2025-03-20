// STS_StatTrackingComponent.c
// Component attached to each player to track their stats

class STS_PlayerStats
{
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
    
    // Adding connection info
    string m_sIPAddress;
    float m_fConnectionTime;
    float m_fLastSessionDuration;
    float m_fTotalPlaytime;
    
    // Tracking who killed this player and with what
    ref array<string> m_aKilledBy = new array<string>();
    ref array<string> m_aKilledByWeapon = new array<string>();
    ref array<int> m_aKilledByTeam = new array<int>();
    
    void STS_PlayerStats()
    {
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
        
        // Initialize connection info
        m_sIPAddress = "";
        m_fConnectionTime = 0;
        m_fLastSessionDuration = 0;
        m_fTotalPlaytime = 0;
    }
    
    // Calculate the total score based on weighted values
    int CalculateTotalScore()
    {
        return m_iKills * 10 + 
               m_iBasesCaptured * 50 + 
               m_iSuppliesDelivered + 
               m_iAIKills * 5 + 
               m_iVehicleKills * 20 + 
               m_iAirKills * 30;
    }
    
    // Update session duration
    void UpdateSessionDuration()
    {
        if (m_fConnectionTime > 0)
        {
            m_fLastSessionDuration = System.GetTickCount() / 1000.0 - m_fConnectionTime;
            m_fTotalPlaytime += m_fLastSessionDuration;
        }
    }
    
    // Convert to JSON string representation
    string ToJSON()
    {
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
        
        // Add kill tracking info
        json += "\"killedBy\":[";
        for (int i = 0; i < m_aKilledBy.Count(); i++) {
            json += "\"" + m_aKilledBy[i] + "\"";
            if (i < m_aKilledBy.Count() - 1) json += ",";
        }
        json += "],";
        
        json += "\"killedByWeapon\":[";
        for (int i = 0; i < m_aKilledByWeapon.Count(); i++) {
            json += "\"" + m_aKilledByWeapon[i] + "\"";
            if (i < m_aKilledByWeapon.Count() - 1) json += ",";
        }
        json += "],";
        
        json += "\"killedByTeam\":[";
        for (int i = 0; i < m_aKilledByTeam.Count(); i++) {
            json += m_aKilledByTeam[i].ToString();
            if (i < m_aKilledByTeam.Count() - 1) json += ",";
        }
        json += "]";
        
        json += "}";
        return json;
    }
    
    // Load stats from JSON string
    void FromJSON(string json)
    {
        // Simple JSON parsing (in a real implementation, use a proper JSON parser)
        // This is a simplified version for demonstration purposes
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
            
            if (key == "kills") m_iKills = value.ToInt();
            else if (key == "deaths") m_iDeaths = value.ToInt();
            else if (key == "basesLost") m_iBasesLost = value.ToInt();
            else if (key == "basesCaptured") m_iBasesCaptured = value.ToInt();
            else if (key == "totalXP") m_iTotalXP = value.ToInt();
            else if (key == "rank") m_iRank = value.ToInt();
            else if (key == "suppliesDelivered") m_iSuppliesDelivered = value.ToInt();
            else if (key == "supplyDeliveryCount") m_iSupplyDeliveryCount = value.ToInt();
            else if (key == "aiKills") m_iAIKills = value.ToInt();
            else if (key == "vehicleKills") m_iVehicleKills = value.ToInt();
            else if (key == "airKills") m_iAirKills = value.ToInt();
            else if (key == "ipAddress") m_sIPAddress = value;
            else if (key == "connectionTime") m_fConnectionTime = value.ToFloat();
            else if (key == "lastSessionDuration") m_fLastSessionDuration = value.ToFloat();
            else if (key == "totalPlaytime") m_fTotalPlaytime = value.ToFloat();
            // Note: Complex arrays like killedBy would need special parsing logic
        }
    }
    
    // Track who killed this player and with what weapon
    void AddKillInfo(string killerName, string weaponName, int teamID)
    {
        m_aKilledBy.Insert(killerName);
        m_aKilledByWeapon.Insert(weaponName);
        m_aKilledByTeam.Insert(teamID);
    }
}

//! Component that tracks stats for a player
class STS_StatTrackingComponent : ScriptComponent
{
    // Stats for this player
    protected ref STS_PlayerStats m_Stats;
    
    // Reference to the manager component
    protected STS_StatTrackingManagerComponent m_Manager;
    
    // Player ID
    protected int m_iPlayerID;
    
    // Player name
    protected string m_sPlayerName;
    
    // Flag to indicate if this is an AI
    protected bool m_bIsAI;
    
    // Reference to logging system
    protected static ref STS_LoggingSystem m_Logger;
    
    // Enhanced error tracking
    protected ref map<string, int> m_mErrorCounts = new map<string, int>();
    protected ref map<string, ref array<string>> m_mErrorContexts = new map<string, ref array<string>>();
    protected const int MAX_ERROR_CONTEXTS = 10;
    
    // Recovery settings
    protected const float RECOVERY_CHECK_INTERVAL = 60; // 1 minute
    protected const int MAX_RECOVERY_ATTEMPTS = 3;
    protected int m_iRecoveryAttempts = 0;
    
    // State tracking
    protected bool m_bIsInitialized = false;
    protected bool m_bIsRecovering = false;
    protected float m_fLastRecoveryAttempt = 0;
    
    //------------------------------------------------------------------------------------------------
    void STS_StatTrackingComponent()
    {
        if (!m_Logger)
        {
            m_Logger = STS_LoggingSystem.GetInstance();
        }
        
        m_Stats = new STS_PlayerStats();
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        
        try
        {
            // Initialize logging with context
            if (!m_Logger)
            {
                m_Logger = STS_LoggingSystem.GetInstance();
                if (!m_Logger)
                {
                    Print("[StatTracker] CRITICAL ERROR: Failed to initialize logging system");
                    return;
                }
            }
            
            // Set up recovery monitoring
            GetGame().GetCallqueue().CallLater(CheckRecovery, RECOVERY_CHECK_INTERVAL, true);
            
            // Enhanced initialization with better error handling
            InitializeComponent(owner);
            
            m_bIsInitialized = true;
            m_Logger.LogInfo("Player component initialized successfully", 
                "STS_StatTrackingComponent", "OnPostInit", {
                    "entity_id": owner.GetID().ToString(),
                    "player_name": m_sPlayerName,
                    "is_ai": m_bIsAI
                });
        }
        catch (Exception e)
        {
            HandleInitializationError(e, owner);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void InitializeComponent(IEntity owner)
    {
        try
        {
            // Register with the manager with retry logic
            if (!RegisterWithManager(owner))
            {
                throw new Exception("Failed to register with StatTrackingManagerComponent");
            }
            
            // Subscribe to events with validation
            if (!SubscribeToEvents(owner))
            {
                throw new Exception("Failed to subscribe to required events");
            }
            
            // Initialize player data
            InitializePlayerData(owner);
        }
        catch (Exception e)
        {
            throw new Exception(string.Format("Component initialization failed: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool RegisterWithManager(IEntity owner)
    {
        try
        {
            STS_StatTrackingManagerComponent manager = GetGame().GetWorld().FindComponent(STS_StatTrackingManagerComponent);
            if (!manager)
            {
                m_Logger.LogError("StatTrackingManagerComponent not found", 
                    "STS_StatTrackingComponent", "RegisterWithManager", {
                        "entity_id": owner.GetID().ToString()
                    });
                return false;
            }
            
            manager.RegisterPlayer(this);
            return true;
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception during manager registration", 
                "STS_StatTrackingComponent", "RegisterWithManager", {
                    "error": e.ToString(),
                    "entity_id": owner.GetID().ToString()
                });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool SubscribeToEvents(IEntity owner)
    {
        bool success = true;
        
        try
        {
            // Subscribe to damage events with validation
            ScriptInvokerInt<IEntity, IEntity, float, int, int> damageInvoker = GetOnDamageDealtInvoker(owner);
            if (!damageInvoker)
            {
                m_Logger.LogWarning("Damage invoker not available", 
                    "STS_StatTrackingComponent", "SubscribeToEvents", {
                        "entity_id": owner.GetID().ToString()
                    });
                success = false;
            }
            else
            {
                damageInvoker.Insert(OnDamageDealt);
            }
            
            // Subscribe to death events with validation
            ScriptInvokerInt<EDamageState, EDamageState> damageStateInvoker = GetOnDamageStateChangedInvoker(owner);
            if (!damageStateInvoker)
            {
                m_Logger.LogWarning("Damage state invoker not available", 
                    "STS_StatTrackingComponent", "SubscribeToEvents", {
                        "entity_id": owner.GetID().ToString()
                    });
                success = false;
            }
            else
            {
                damageStateInvoker.Insert(OnDamageStateChanged);
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception during event subscription", 
                "STS_StatTrackingComponent", "SubscribeToEvents", {
                    "error": e.ToString(),
                    "entity_id": owner.GetID().ToString()
                });
            success = false;
        }
        
        return success;
    }
    
    //------------------------------------------------------------------------------------------------
    protected void HandleInitializationError(Exception e, IEntity owner)
    {
        string errorContext = string.Format("Initialization failed for entity %1: %2\nStack trace: %3", 
            owner, e.ToString(), e.StackTrace);
            
        if (m_Logger)
        {
            m_Logger.LogError(errorContext, "STS_StatTrackingComponent", "HandleInitializationError", {
                "entity_id": owner.GetID().ToString(),
                "error_type": e.Type().ToString()
            });
        }
        else
        {
            Print("[StatTracker] CRITICAL ERROR: " + errorContext);
        }
        
        // Set up recovery attempt
        m_bIsRecovering = true;
        m_fLastRecoveryAttempt = System.GetTickCount() / 1000.0;
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CheckRecovery()
    {
        if (!m_bIsRecovering || !m_bIsInitialized) return;
        
        try
        {
            float currentTime = System.GetTickCount() / 1000.0;
            if (currentTime - m_fLastRecoveryAttempt >= RECOVERY_CHECK_INTERVAL)
            {
                if (m_iRecoveryAttempts >= MAX_RECOVERY_ATTEMPTS)
                {
                    m_Logger.LogCritical("Maximum recovery attempts reached", 
                        "STS_StatTrackingComponent", "CheckRecovery", {
                            "entity_id": GetOwner().GetID().ToString(),
                            "attempts": m_iRecoveryAttempts
                        });
                    m_bIsRecovering = false;
                    return;
                }
                
                m_iRecoveryAttempts++;
                m_fLastRecoveryAttempt = currentTime;
                
                // Attempt recovery
                if (AttemptRecovery())
                {
                    m_Logger.LogInfo("Component recovered successfully", 
                        "STS_StatTrackingComponent", "CheckRecovery", {
                            "entity_id": GetOwner().GetID().ToString(),
                            "attempts": m_iRecoveryAttempts
                        });
                    m_bIsRecovering = false;
                    m_iRecoveryAttempts = 0;
                }
                else
                {
                    m_Logger.LogWarning("Recovery attempt failed", 
                        "STS_StatTrackingComponent", "CheckRecovery", {
                            "entity_id": GetOwner().GetID().ToString(),
                            "attempt": m_iRecoveryAttempts
                        });
                }
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception during recovery check", 
                "STS_StatTrackingComponent", "CheckRecovery", {
                    "error": e.ToString(),
                    "entity_id": GetOwner().GetID().ToString()
                });
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool AttemptRecovery()
    {
        try
        {
            IEntity owner = GetOwner();
            if (!owner) return false;
            
            // Re-register with manager
            if (!RegisterWithManager(owner)) return false;
            
            // Re-subscribe to events
            if (!SubscribeToEvents(owner)) return false;
            
            // Re-initialize player data
            InitializePlayerData(owner);
            
            return true;
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception during recovery attempt", 
                "STS_StatTrackingComponent", "AttemptRecovery", {
                    "error": e.ToString(),
                    "entity_id": GetOwner().GetID().ToString()
                });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnDamageDealt(IEntity victim, IEntity attacker, float damage, int teamID, int weaponID)
    {
        try
        {
            if (!m_bIsInitialized || m_bIsRecovering)
            {
                m_Logger.LogWarning("Ignoring damage event - component not ready", 
                    "STS_StatTrackingComponent", "OnDamageDealt", {
                        "entity_id": GetOwner().GetID().ToString(),
                        "victim_id": victim.GetID().ToString(),
                        "attacker_id": attacker.GetID().ToString()
                    });
                return;
            }
            
            // Validate entities
            if (!victim || !attacker)
            {
                m_Logger.LogWarning("Invalid entities in damage event", 
                    "STS_StatTrackingComponent", "OnDamageDealt", {
                        "victim_valid": victim != null,
                        "attacker_valid": attacker != null
                    });
                return;
            }
            
            // Process damage event
            ProcessDamageEvent(victim, attacker, damage, teamID, weaponID);
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception in damage event handler", 
                "STS_StatTrackingComponent", "OnDamageDealt", {
                    "error": e.ToString(),
                    "entity_id": GetOwner().GetID().ToString()
                });
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void ProcessDamageEvent(IEntity victim, IEntity attacker, float damage, int teamID, int weaponID)
    {
        try
        {
            // Get weapon name with validation
            string weaponName = GetWeaponName(weaponID);
            if (weaponName == "")
            {
                m_Logger.LogWarning("Unknown weapon ID", 
                    "STS_StatTrackingComponent", "ProcessDamageEvent", {
                        "weapon_id": weaponID
                    });
            }
            
            // Update stats based on damage
            if (damage >= 100) // Assuming 100 damage is lethal
            {
                UpdateKillStats(attacker, victim, weaponName, teamID);
            }
            
            // Log event with context
            m_Logger.LogDebug("Damage event processed", 
                "STS_StatTrackingComponent", "ProcessDamageEvent", {
                    "victim_id": victim.GetID().ToString(),
                    "attacker_id": attacker.GetID().ToString(),
                    "damage": damage,
                    "weapon": weaponName,
                    "team_id": teamID
                });
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception processing damage event", 
                "STS_StatTrackingComponent", "ProcessDamageEvent", {
                    "error": e.ToString(),
                    "entity_id": GetOwner().GetID().ToString()
                });
        }
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnDelete(IEntity owner)
    {
        try
        {
            // Update session duration before deleting
            if (m_Stats)
                m_Stats.UpdateSessionDuration();
            
            // Unregister from the manager
            if (GetGame() && GetGame().GetWorld())
            {
                STS_StatTrackingManagerComponent manager = GetGame().GetWorld().FindComponent(STS_StatTrackingManagerComponent);
                if (manager)
                    manager.UnregisterPlayer(this);
            }
            
            // Unsubscribe from damage events
            ScriptInvokerInt<IEntity, IEntity, float, int, int> damageInvoker = GetOnDamageDealtInvoker(owner);
            if (damageInvoker)
                damageInvoker.Remove(OnDamageDealt);
                
            // Unsubscribe from death events
            ScriptInvokerInt<EDamageState, EDamageState> damageStateInvoker = GetOnDamageStateChangedInvoker(owner);
            if (damageStateInvoker)
                damageStateInvoker.Remove(OnDamageStateChanged);
        }
        catch (Exception e)
        {
            if (m_Logger)
                m_Logger.LogError(string.Format("Exception in OnDelete for entity %1: %2", owner, e.ToString()),
                    "STS_StatTrackingComponent", "OnDelete");
        }
        
        super.OnDelete(owner);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the damage invoker for an entity
    protected ScriptInvokerInt<IEntity, IEntity, float, int, int> GetOnDamageDealtInvoker(IEntity entity)
    {
        if (!entity)
            return null;
            
        SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));
        if (!damageManager)
            return null;
            
        return damageManager.GetOnDamageDealtInvoker();
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the damage state changed invoker for an entity
    protected ScriptInvokerInt<EDamageState, EDamageState> GetOnDamageStateChangedInvoker(IEntity entity)
    {
        if (!entity)
            return null;
            
        SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));
        if (!damageManager)
            return null;
            
        return damageManager.GetOnDamageStateChangedInvoker();
    }
    
    //------------------------------------------------------------------------------------------------
    // Process damage events to track kills
    protected int OnDamageDealt(IEntity victim, IEntity instigator, float damage, int damageType, int hitZone)
    {
        try
        {
            // Validate inputs
            if (!victim)
            {
                m_Logger.LogWarning("OnDamageDealt called with null victim", 
                    "STS_StatTrackingComponent", "OnDamageDealt");
                return -1;
            }
            
            if (!instigator)
            {
                m_Logger.LogWarning("OnDamageDealt called with null instigator", 
                    "STS_StatTrackingComponent", "OnDamageDealt");
                return -1;
            }
            
            if (damage <= 0)
            {
                // Skip processing for zero or negative damage
                return -1;
            }
            
            // Get victim's damage manager to check if this was a killing blow
            BaseDamageManager victimDamageManager = BaseDamageManager.Cast(victim.FindComponent(BaseDamageManager));
            if (!victimDamageManager)
            {
                m_Logger.LogWarning(string.Format("Victim %1 has no damage manager", victim), 
                    "STS_StatTrackingComponent", "OnDamageDealt");
                return -1;
            }
            
            // Skip if victim isn't dead
            EDamageState victimState = victimDamageManager.GetState();
            if (victimState != EDamageState.DESTROYED)
            {
                return -1;
            }
            
            // Verify this entity caused the damage
            IEntity attackerEntity = GetOwner();
            if (!attackerEntity || attackerEntity != instigator)
            {
                m_Logger.LogDebug("OnDamageDealt called for non-matching instigator", 
                    "STS_StatTrackingComponent", "OnDamageDealt");
                return -1;
            }
            
            // Check if victim is a vehicle
            bool isVehicle = false;
            bool isAircraft = false;
            VehicleComponent vehicleComponent = VehicleComponent.Cast(victim.FindComponent(VehicleComponent));
            if (vehicleComponent)
            {
                isVehicle = true;
                
                // Check if it's an aircraft
                if (vehicleComponent.IsHelicopter() || vehicleComponent.IsPlane())
                {
                    isAircraft = true;
                }
            }
            
            // Check if victim is an AI or player
            STS_StatTrackingComponent victimComponent = STS_StatTrackingComponent.Cast(victim.FindComponent(STS_StatTrackingComponent));
            if (victimComponent)
            {
                // Get faction info for team kill detection
                int attackerFaction = GetFactionId();
                int victimFaction = victimComponent.GetFactionId();
                
                // Get weapon used
                string weaponName = "Unknown";
                WeaponEntity weapon = GetActiveWeapon();
                if (weapon)
                {
                    weaponName = weapon.GetName();
                }
                else
                {
                    // Try to get vehicle name if attacking from vehicle
                    VehicleComponent attackerVehicle = GetCurrentVehicle();
                    if (attackerVehicle)
                    {
                        weaponName = "Vehicle: " + attackerVehicle.GetOwner().GetName();
                    }
                }
                
                // Handle player kill
                if (!victimComponent.IsAI())
                {
                    // Check for team kill
                    if (attackerFaction == victimFaction && attackerFaction != 0)
                    {
                        // Log team kill
                        m_Logger.LogInfo(string.Format("Team kill detected: %1 killed %2 with %3", 
                            m_sPlayerName, victimComponent.GetPlayerName(), weaponName),
                            "STS_StatTrackingComponent", "OnDamageDealt");
                        
                        // Report to team kill tracker
                        STS_TeamKillTracker teamKillTracker = STS_TeamKillTracker.GetInstance();
                        if (teamKillTracker)
                        {
                            vector position = attackerEntity.GetOrigin();
                            teamKillTracker.ReportTeamKill(m_iPlayerID, m_sPlayerName, 
                                                         victimComponent.GetPlayerID(), victimComponent.GetPlayerName(),
                                                         position, weaponName, attackerFaction, victimFaction);
                        }
                        else
                        {
                            m_Logger.LogWarning("Team kill tracker not available - could not report team kill",
                                "STS_StatTrackingComponent", "OnDamageDealt");
                        }
                    }
                    else
                    {
                        // Normal player kill
                        m_Stats.m_iKills++;
                        m_Logger.LogInfo(string.Format("%1 killed player %2 with %3", 
                            m_sPlayerName, victimComponent.GetPlayerName(), weaponName),
                            "STS_StatTrackingComponent", "OnDamageDealt");
                    }
                    
                    // Update both stats
                    NotifyStatsChanged();
                    
                    // Update victim's death stats if available
                    if (victimComponent)
                    {
                        victimComponent.RecordDeath(m_iPlayerID, m_sPlayerName, weaponName, attackerFaction);
                    }
                }
                else
                {
                    // AI kill
                    m_Stats.m_iAIKills++;
                    m_Logger.LogDebug(string.Format("%1 killed AI %2 with %3", 
                        m_sPlayerName, victimComponent.GetPlayerName(), weaponName),
                        "STS_StatTrackingComponent", "OnDamageDealt");
                    NotifyStatsChanged();
                }
            }
            else if (isVehicle)
            {
                // Update vehicle/aircraft kill stats
                if (isAircraft)
                {
                    m_Stats.m_iAirKills++;
                    m_Logger.LogInfo(string.Format("%1 destroyed aircraft %2", 
                        m_sPlayerName, victim.GetName()),
                        "STS_StatTrackingComponent", "OnDamageDealt");
                }
                else
                {
                    m_Stats.m_iVehicleKills++;
                    m_Logger.LogInfo(string.Format("%1 destroyed vehicle %2", 
                        m_sPlayerName, victim.GetName()),
                        "STS_StatTrackingComponent", "OnDamageDealt");
                }
                NotifyStatsChanged();
            }
            
            return -1;
        }
        catch (Exception e)
        {
            if (m_Logger)
            {
                m_Logger.LogError(string.Format("Exception in OnDamageDealt: %1\nStacktrace: %2", 
                    e.ToString(), e.GetStackTrace()),
                    "STS_StatTrackingComponent", "OnDamageDealt");
            }
            else
            {
                Print(string.Format("[StatTracker] CRITICAL ERROR in OnDamageDealt: %1", e.ToString()), LogLevel.ERROR);
            }
            return -1;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when damage state changes
    protected int OnDamageStateChanged(EDamageState oldState, EDamageState newState)
    {
        try
        {
            // Check if player died
            if (newState == EDamageState.DESTROYED && oldState != EDamageState.DESTROYED)
            {
                if (m_Stats)
                {
                    m_Stats.m_iDeaths++;
                    
                    m_Logger.LogDebug(string.Format("Death recorded for player %1 (ID: %2)", m_sPlayerName, m_iPlayerID), 
                        "STS_StatTrackingComponent", "OnDamageStateChanged");
                    
                    // Update manager
                    if (m_Manager)
                        m_Manager.OnStatsChanged(this);
                    else
                        m_Logger.LogWarning(string.Format("Manager reference is null for player %1 - death update not synchronized", m_sPlayerName), 
                            "STS_StatTrackingComponent", "OnDamageStateChanged");
                }
                else
                {
                    m_Logger.LogError(string.Format("Stats object is null for player %1 - death not recorded", m_sPlayerName), 
                        "STS_StatTrackingComponent", "OnDamageStateChanged");
                }
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in OnDamageStateChanged: %1", e.ToString()), 
                "STS_StatTrackingComponent", "OnDamageStateChanged");
        }
        
        return 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a base capture
    void AddBaseCaptured()
    {
        try
        {
            if (!m_Stats) 
            {
                m_Logger.LogError(string.Format("Stats object is null for player %1 - base capture not recorded", m_sPlayerName), 
                    "STS_StatTrackingComponent", "AddBaseCaptured");
                return;
            }
            
            m_Stats.m_iBasesCaptured++;
            AddXP(50);
            
            m_Logger.LogDebug(string.Format("Base capture recorded for player %1 (ID: %2)", m_sPlayerName, m_iPlayerID), 
                "STS_StatTrackingComponent", "AddBaseCaptured");
            
            // Update manager
            if (m_Manager)
                m_Manager.OnStatsChanged(this);
            else
                m_Logger.LogWarning(string.Format("Manager reference is null for player %1 - stats update not synchronized", m_sPlayerName), 
                    "STS_StatTrackingComponent", "AddBaseCaptured");
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in AddBaseCaptured: %1", e.ToString()), 
                "STS_StatTrackingComponent", "AddBaseCaptured");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a base lost
    void AddBaseLost()
    {
        try
        {
            if (!m_Stats) 
            {
                m_Logger.LogError(string.Format("Stats object is null for player %1 - base loss not recorded", m_sPlayerName), 
                    "STS_StatTrackingComponent", "AddBaseLost");
                return;
            }
            
            m_Stats.m_iBasesLost++;
            
            m_Logger.LogDebug(string.Format("Base loss recorded for player %1 (ID: %2)", m_sPlayerName, m_iPlayerID), 
                "STS_StatTrackingComponent", "AddBaseLost");
            
            // Update manager
            if (m_Manager)
                m_Manager.OnStatsChanged(this);
            else
                m_Logger.LogWarning(string.Format("Manager reference is null for player %1 - stats update not synchronized", m_sPlayerName), 
                    "STS_StatTrackingComponent", "AddBaseLost");
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in AddBaseLost: %1", e.ToString()), 
                "STS_StatTrackingComponent", "AddBaseLost");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Add supplies delivered
    void AddSuppliesDelivered(int amount)
    {
        try
        {
            if (!m_Stats) 
            {
                m_Logger.LogError(string.Format("Stats object is null for player %1 - supplies delivery not recorded", m_sPlayerName), 
                    "STS_StatTrackingComponent", "AddSuppliesDelivered");
                return;
            }
            
            m_Stats.m_iSuppliesDelivered += amount;
            m_Stats.m_iSupplyDeliveryCount++;
            AddXP(amount);
            
            m_Logger.LogDebug(string.Format("Supplies delivery recorded for player %1 (ID: %2): %3 units", 
                m_sPlayerName, m_iPlayerID, amount), 
                "STS_StatTrackingComponent", "AddSuppliesDelivered");
            
            // Update manager
            if (m_Manager)
                m_Manager.OnStatsChanged(this);
            else
                m_Logger.LogWarning(string.Format("Manager reference is null for player %1 - stats update not synchronized", m_sPlayerName), 
                    "STS_StatTrackingComponent", "AddSuppliesDelivered");
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in AddSuppliesDelivered: %1", e.ToString()), 
                "STS_StatTrackingComponent", "AddSuppliesDelivered");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Add XP to the player
    void AddXP(int amount)
    {
        try
        {
            if (!m_Stats) 
            {
                m_Logger.LogError(string.Format("Stats object is null for player %1 - XP not added", m_sPlayerName), 
                    "STS_StatTrackingComponent", "AddXP");
                return;
            }
            
            if (amount <= 0)
            {
                m_Logger.LogWarning(string.Format("Attempted to add invalid XP amount (%1) to player %2", amount, m_sPlayerName), 
                    "STS_StatTrackingComponent", "AddXP");
                return;
            }
            
            int oldXP = m_Stats.m_iTotalXP;
            m_Stats.m_iTotalXP += amount;
            
            m_Logger.LogDebug(string.Format("Added %1 XP to player %2 (now %3, was %4)", 
                amount, m_sPlayerName, m_Stats.m_iTotalXP, oldXP), 
                "STS_StatTrackingComponent", "AddXP");
            
            // Update manager
            if (m_Manager)
                m_Manager.OnStatsChanged(this);
            else
                m_Logger.LogWarning(string.Format("Manager reference is null for player %1 - XP update not synchronized", m_sPlayerName), 
                    "STS_StatTrackingComponent", "AddXP");
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in AddXP: %1", e.ToString()), 
                "STS_StatTrackingComponent", "AddXP");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Set connection info
    void SetConnectionInfo(string ipAddress)
    {
        try
        {
            if (!m_Stats) 
            {
                m_Logger.LogError(string.Format("Stats object is null for player %1 - connection info not set", m_sPlayerName), 
                    "STS_StatTrackingComponent", "SetConnectionInfo");
                return;
            }
            
            m_Stats.m_sIPAddress = ipAddress;
            m_Stats.m_fConnectionTime = System.GetTickCount() / 1000.0; // Current time in seconds
            
            m_Logger.LogDebug(string.Format("Connection info set for player %1 (ID: %2): IP=%3", 
                m_sPlayerName, m_iPlayerID, ipAddress), 
                "STS_StatTrackingComponent", "SetConnectionInfo");
            
            // Update manager
            if (m_Manager)
                m_Manager.OnStatsChanged(this);
            else
                m_Logger.LogWarning(string.Format("Manager reference is null for player %1 - connection info update not synchronized", m_sPlayerName), 
                    "STS_StatTrackingComponent", "SetConnectionInfo");
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in SetConnectionInfo: %1", e.ToString()), 
                "STS_StatTrackingComponent", "SetConnectionInfo");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get session duration in seconds
    float GetSessionDuration()
    {
        try
        {
            if (!m_Stats)
                return 0;
                
            return m_Stats.m_fLastSessionDuration;
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in GetSessionDuration: %1", e.ToString()), 
                "STS_StatTrackingComponent", "GetSessionDuration");
            return 0;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Getters and setters
    //------------------------------------------------------------------------------------------------
    
    STS_PlayerStats GetStats()
    {
        return m_Stats;
    }
    
    int GetPlayerID()
    {
        return m_iPlayerID;
    }
    
    void SetPlayerID(int playerID)
    {
        try
        {
            m_iPlayerID = playerID;
            
            if (m_Stats)
                m_Stats.SetPlayerID(playerID);
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in SetPlayerID: %1", e.ToString()), 
                "STS_StatTrackingComponent", "SetPlayerID");
        }
    }
    
    string GetPlayerName()
    {
        return m_sPlayerName;
    }
    
    void SetPlayerName(string name)
    {
        try
        {
            m_sPlayerName = name;
            
            if (m_Stats)
                m_Stats.SetPlayerName(name);
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in SetPlayerName: %1", e.ToString()), 
                "STS_StatTrackingComponent", "SetPlayerName");
        }
    }
    
    bool IsAI()
    {
        return m_bIsAI;
    }
    
    void SetIsAI(bool isAI)
    {
        m_bIsAI = isAI;
    }
    
    void SetManager(STS_StatTrackingManagerComponent manager)
    {
        m_Manager = manager;
    }
    
    string GetIPAddress()
    {
        if (!m_Stats)
            return "";
            
        return m_Stats.m_sIPAddress;
    }
    
    float GetConnectionTime()
    {
        if (!m_Stats)
            return 0;
            
        return m_Stats.m_fConnectionTime;
    }
    
    // Get the name of the currently used weapon
    protected string GetUsedWeaponName()
    {
        try
        {
            IEntity owner = GetOwner();
            if (!owner)
            {
                m_Logger.LogWarning("GetUsedWeaponName called with null owner", 
                    "STS_StatTrackingComponent", "GetUsedWeaponName");
                return "Unknown Weapon";
            }
            
            SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));
            if (!characterController)
            {
                m_Logger.LogDebug(string.Format("Entity %1 has no character controller component", owner.GetName()), 
                    "STS_StatTrackingComponent", "GetUsedWeaponName");
                return "Unknown Weapon";
            }
            
            WeaponManagerComponent weaponManager = WeaponManagerComponent.Cast(owner.FindComponent(WeaponManagerComponent));
            if (!weaponManager)
            {
                m_Logger.LogDebug(string.Format("Entity %1 has no weapon manager component", owner.GetName()), 
                    "STS_StatTrackingComponent", "GetUsedWeaponName");
                return "Unknown Weapon";
            }
            
            IEntity activeWeapon = weaponManager.GetCurrentWeapon();
            if (!activeWeapon)
            {
                m_Logger.LogDebug(string.Format("Entity %1 has no active weapon", owner.GetName()), 
                    "STS_StatTrackingComponent", "GetUsedWeaponName");
                return "No Weapon";
            }
            
            string weaponName = activeWeapon.GetName();
            m_Logger.LogDebug(string.Format("Weapon used: %1", weaponName), 
                "STS_StatTrackingComponent", "GetUsedWeaponName");
            return weaponName;
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in GetUsedWeaponName: %1", e.ToString()), 
                "STS_StatTrackingComponent", "GetUsedWeaponName");
            return "Error Determining Weapon";
        }
    }
    
    // Get player's team ID
    protected int GetPlayerTeam()
    {
        try
        {
            IEntity owner = GetOwner();
            if (!owner)
                return -1;
            
            FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
            if (!factionComponent)
                return -1;
            
            Faction faction = factionComponent.GetAffiliatedFaction();
            if (!faction)
                return -1;
            
            return faction.GetFactionIndex();
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in GetPlayerTeam: %1", e.ToString()), 
                "STS_StatTrackingComponent", "GetPlayerTeam");
            return -1;
        }
    }
} 
} 