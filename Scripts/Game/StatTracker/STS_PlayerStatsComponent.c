// STS_PlayerStatsComponent.c
// Component that handles player statistics tracking and persistence

class STS_PlayerStatsComponent : ScriptComponent
{
    // Reference to the player's stats
    protected ref STS_EnhancedPlayerStats m_PlayerStats;
    
    // Reference to the persistence manager
    protected STS_PersistenceManager m_PersistenceManager;
    
    // Tracking variables
    protected vector m_LastPosition;
    protected float m_LastMovementCheck;
    protected float m_MovementCheckInterval = 5.0; // Check movement every 5 seconds
    protected float m_AutosaveInterval = 300.0; // Autosave every 5 minutes
    protected float m_LastAutosave;
    protected IEntity m_Owner;
    
    //------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        m_Owner = owner;
        
        // Initialize the player stats
        m_PlayerStats = new STS_EnhancedPlayerStats();
        
        // Get persistence manager singleton
        m_PersistenceManager = STS_PersistenceManager.GetInstance();
        
        // Initialize tracking variables
        m_LastPosition = owner.GetOrigin();
        m_LastMovementCheck = System.GetTickCount() / 1000.0;
        m_LastAutosave = m_LastMovementCheck;
        
        // Register event handlers
        GetGame().GetCallqueue().CallLater(LoadPlayerStats, 1000, false); // Delay loading to ensure player is fully initialized
        GetGame().GetCallqueue().CallLater(StartTracking, 2000, false);   // Start tracking after stats are loaded
    }
    
    //------------------------------------------------------------------------------------------------
    // Load player stats from persistence
    protected void LoadPlayerStats()
    {
        // Get player identity
        PlayerIdentity identity = PlayerIdentity.Cast(m_Owner.GetIdentity());
        if (!identity)
            return;
            
        string playerId = identity.GetPlainId();
        string steamId = identity.GetPlainId(); // In actual implementation, get real Steam ID if available
        
        // Load player stats from persistence
        m_PlayerStats = m_PersistenceManager.LoadPlayerStats(playerId);
        
        // If new player, initialize stats
        if (!m_PlayerStats)
        {
            m_PlayerStats = new STS_EnhancedPlayerStats();
        }
        
        // Record login
        m_PlayerStats.RecordLogin(playerId, steamId);
        
        // Set network information
        if (identity.GetAddress())
            m_PlayerStats.m_sIPAddress = identity.GetAddress();
            
        m_PlayerStats.m_fConnectionTime = System.GetTickCount() / 1000.0;
        
        Print("[StatTracker] Loaded stats for player: " + playerId);
    }
    
    //------------------------------------------------------------------------------------------------
    // Start tracking player statistics
    protected void StartTracking()
    {
        // Register for tick updates
        GetGame().GetCallqueue().CallLater(OnTrackerTick, 1000, true); // Check every second
    }
    
    //------------------------------------------------------------------------------------------------
    // Stop tracking and save player statistics
    protected void StopTracking()
    {
        // Unregister tick updates
        GetGame().GetCallqueue().Remove(OnTrackerTick);
        
        // Record logout and save stats
        if (m_PlayerStats)
        {
            m_PlayerStats.RecordLogout();
            SavePlayerStats();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Tracker tick function - called every second
    protected void OnTrackerTick()
    {
        if (!m_Owner || !m_PlayerStats)
            return;
            
        float currentTime = System.GetTickCount() / 1000.0;
        
        // Check for movement updates
        if (currentTime - m_LastMovementCheck >= m_MovementCheckInterval)
        {
            UpdateMovementStats();
            m_LastMovementCheck = currentTime;
        }
        
        // Check for autosave
        if (currentTime - m_LastAutosave >= m_AutosaveInterval)
        {
            SavePlayerStats();
            m_LastAutosave = currentTime;
        }
        
        // Other periodic stats checks could go here
    }
    
    //------------------------------------------------------------------------------------------------
    // Update movement-related statistics
    protected void UpdateMovementStats()
    {
        if (!m_Owner || !m_PlayerStats)
            return;
            
        vector currentPos = m_Owner.GetOrigin();
        float distance = vector.Distance(m_LastPosition, currentPos);
        
        if (distance > 1.0) // Only count meaningful movement (> 1m)
        {
            // Check if player is in vehicle
            bool inVehicle = false;
            IEntity vehicle = GetGame().GetVehicleManager().GetPlayerVehicle(m_Owner);
            if (vehicle)
                inVehicle = true;
                
            // Record movement
            m_PlayerStats.RecordMovement(distance, inVehicle);
            
            // Update last position
            m_LastPosition = currentPos;
            
            // Check if player has entered a new location
            string locationName = GetLocationName(currentPos);
            if (locationName != "")
            {
                m_PlayerStats.RecordLocationVisit(currentPos, locationName);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the name of the location at the given position
    protected string GetLocationName(vector position)
    {
        // In a real implementation, this would query the game's location system
        // Placeholder implementation - replace with actual game API calls
        return "";
    }
    
    //------------------------------------------------------------------------------------------------
    // Record a kill made by this player
    void RecordKill(IEntity victim, string weaponName, float distance, bool isHeadshot = false)
    {
        if (!m_PlayerStats)
            return;
            
        // Record the kill
        m_PlayerStats.RecordKill(weaponName, distance, isHeadshot);
        
        // Check if victim is AI or player
        PlayerIdentity victimIdentity = PlayerIdentity.Cast(victim.GetIdentity());
        if (!victimIdentity)
        {
            // It's an AI kill
            m_PlayerStats.m_iAIKills++;
        }
        
        // Save stats after a kill
        SavePlayerStats();
    }
    
    //------------------------------------------------------------------------------------------------
    // Record a death of this player
    void RecordDeath(IEntity killer = null)
    {
        if (!m_PlayerStats)
            return;
            
        m_PlayerStats.m_iDeaths++;
        
        // Save stats after death
        SavePlayerStats();
    }
    
    //------------------------------------------------------------------------------------------------
    // Record damage dealt by this player
    void RecordDamageDealt(float amount, EDamageType damageType, int hitZone, IEntity victim)
    {
        if (!m_PlayerStats)
            return;
            
        m_PlayerStats.RecordDamageDealt(amount, damageType, hitZone);
    }
    
    //------------------------------------------------------------------------------------------------
    // Record damage taken by this player
    void RecordDamageTaken(float amount, EDamageType damageType, int hitZone, IEntity attacker)
    {
        if (!m_PlayerStats)
            return;
            
        m_PlayerStats.RecordDamageTaken(amount, damageType, hitZone);
    }
    
    //------------------------------------------------------------------------------------------------
    // Record player entered unconscious state
    void RecordUnconsciousness()
    {
        if (!m_PlayerStats)
            return;
            
        m_PlayerStats.RecordUnconsciousness();
    }
    
    //------------------------------------------------------------------------------------------------
    // Record base capture
    void RecordBaseCaptured(string baseName)
    {
        if (!m_PlayerStats)
            return;
            
        m_PlayerStats.m_iBasesCaptured++;
        SavePlayerStats();
    }
    
    //------------------------------------------------------------------------------------------------
    // Record base lost
    void RecordBaseLost(string baseName)
    {
        if (!m_PlayerStats)
            return;
            
        m_PlayerStats.m_iBasesLost++;
        SavePlayerStats();
    }
    
    //------------------------------------------------------------------------------------------------
    // Record supply delivery
    void RecordSupplyDelivery(int amount)
    {
        if (!m_PlayerStats)
            return;
            
        m_PlayerStats.m_iSuppliesDelivered += amount;
        m_PlayerStats.m_iSupplyDeliveryCount++;
        SavePlayerStats();
    }
    
    //------------------------------------------------------------------------------------------------
    // Record vehicle kill
    void RecordVehicleKill(IEntity vehicle)
    {
        if (!m_PlayerStats)
            return;
            
        // Check if it's an air vehicle
        bool isAir = false; // In real implementation, check vehicle type
        
        if (isAir)
            m_PlayerStats.m_iAirKills++;
        else
            m_PlayerStats.m_iVehicleKills++;
            
        SavePlayerStats();
    }
    
    //------------------------------------------------------------------------------------------------
    // Record economic activity - item purchase
    void RecordItemPurchase(string itemName, int count, int price)
    {
        if (!m_PlayerStats)
            return;
            
        m_PlayerStats.RecordItemPurchase(itemName, count, price);
    }
    
    //------------------------------------------------------------------------------------------------
    // Record economic activity - item sale
    void RecordItemSale(string itemName, int count, int price)
    {
        if (!m_PlayerStats)
            return;
            
        m_PlayerStats.RecordItemSale(itemName, count, price);
    }
    
    //------------------------------------------------------------------------------------------------
    // Award XP to the player
    void AwardXP(int amount, string reason = "")
    {
        if (!m_PlayerStats)
            return;
            
        m_PlayerStats.m_iTotalXP += amount;
        
        // Check for rank upgrades
        UpdateRank();
        
        SavePlayerStats();
    }
    
    //------------------------------------------------------------------------------------------------
    // Update player rank based on XP
    protected void UpdateRank()
    {
        if (!m_PlayerStats)
            return;
            
        // Simple rank calculation based on XP thresholds
        int xp = m_PlayerStats.m_iTotalXP;
        int newRank = 0;
        
        if (xp >= 100000) newRank = 10;
        else if (xp >= 50000) newRank = 9;
        else if (xp >= 25000) newRank = 8;
        else if (xp >= 15000) newRank = 7;
        else if (xp >= 10000) newRank = 6;
        else if (xp >= 5000) newRank = 5;
        else if (xp >= 2500) newRank = 4;
        else if (xp >= 1000) newRank = 3;
        else if (xp >= 500) newRank = 2;
        else if (xp >= 100) newRank = 1;
        
        // Update rank if changed
        if (newRank > m_PlayerStats.m_iRank)
        {
            m_PlayerStats.m_iRank = newRank;
            // Notify player of rank up
            // In actual implementation, send UI notification
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get player's current stats
    STS_EnhancedPlayerStats GetPlayerStats()
    {
        return m_PlayerStats;
    }
    
    //------------------------------------------------------------------------------------------------
    // Save player stats to persistence
    protected void SavePlayerStats()
    {
        if (!m_PlayerStats || !m_PersistenceManager)
            return;
            
        PlayerIdentity identity = PlayerIdentity.Cast(m_Owner.GetIdentity());
        if (!identity)
            return;
            
        string playerId = identity.GetPlainId();
        
        // Save to persistence
        m_PersistenceManager.SavePlayerStats(playerId, m_PlayerStats);
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when component is destroyed
    override void OnDelete(IEntity owner)
    {
        // Stop tracking and save final stats
        StopTracking();
        
        super.OnDelete(owner);
    }
} 