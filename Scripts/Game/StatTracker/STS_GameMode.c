// STS_GameMode.c
// Game mode class that integrates all stat tracking components

class STS_GameMode : GameMode
{
    // Reference to managers
    protected STS_PersistenceManager m_PersistenceManager;
    protected STS_UIManager m_UIManager;
    
    // Command prefix
    protected const string COMMAND_PREFIX = "!stats";
    
    // Player component map
    protected ref map<string, ref STS_PlayerStatsComponent> m_PlayerComponents;
    
    //------------------------------------------------------------------------------------------------
    override void OnGameModeStart()
    {
        super.OnGameModeStart();
        
        // Initialize managers
        m_PersistenceManager = STS_PersistenceManager.GetInstance();
        m_UIManager = STS_UIManager.GetInstance();
        
        // Initialize player component map
        m_PlayerComponents = new map<string, ref STS_PlayerStatsComponent>();
        
        // Register RPCs and event handlers
        GetGame().GetCallqueue().CallLater(RegisterCallbacks, 500, false);
        
        Print("[StatTracker] Game mode initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Register event callbacks
    protected void RegisterCallbacks()
    {
        // Register for player connected/disconnected events
        GetGame().GetCallqueue().CallLater(CheckForNewPlayers, 5000, true);
        
        // Register for chat commands
        GetGame().GetInputManager().AddActionListener("ChatMessageAction", EActionTrigger.DOWN, OnChatMessage);
        
        // Register for player kill events (example using hypothetical event system)
        ScriptInvoker.Get(EGameEvents.PlayerKilled).Insert(OnPlayerKilled);
        
        // Register for damage events
        ScriptInvoker.Get(EGameEvents.EntityDamaged).Insert(OnEntityDamaged);
        
        // Register for vehicle destroyed events
        ScriptInvoker.Get(EGameEvents.VehicleDestroyed).Insert(OnVehicleDestroyed);
        
        // Register for base capture events
        ScriptInvoker.Get(EGameEvents.BaseCaptured).Insert(OnBaseCaptured);
        
        // Register for supply delivery events
        ScriptInvoker.Get(EGameEvents.SupplyDelivered).Insert(OnSupplyDelivered);
        
        // Register for economic transaction events
        ScriptInvoker.Get(EGameEvents.ItemPurchased).Insert(OnItemPurchased);
        ScriptInvoker.Get(EGameEvents.ItemSold).Insert(OnItemSold);
    }
    
    //------------------------------------------------------------------------------------------------
    // Periodically check for new players to attach components to
    protected void CheckForNewPlayers()
    {
        array<IEntity> players = new array<IEntity>();
        GetGame().GetPlayerManager().GetPlayers(players);
        
        foreach (IEntity player : players)
        {
            PlayerIdentity identity = PlayerIdentity.Cast(player.GetIdentity());
            if (!identity)
                continue;
                
            string playerId = identity.GetPlainId();
            
            // If player doesn't have a component yet, add one
            if (!m_PlayerComponents.Contains(playerId))
            {
                AddStatsComponentToPlayer(player, playerId);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Add stats component to a player
    protected void AddStatsComponentToPlayer(IEntity player, string playerId)
    {
        // Create component
        STS_PlayerStatsComponent component = STS_PlayerStatsComponent.Cast(player.FindComponent(STS_PlayerStatsComponent));
        
        // If component doesn't exist, add it
        if (!component)
        {
            component = STS_PlayerStatsComponent.Cast(player.AddComponent(STS_PlayerStatsComponent));
        }
        
        // Store in map
        m_PlayerComponents.Insert(playerId, component);
        
        Print("[StatTracker] Added stats component to player: " + playerId);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle chat messages for commands
    protected void OnChatMessage(float value, EActionTrigger trigger, IEntity entity)
    {
        if (!entity || trigger != EActionTrigger.DOWN)
            return;
            
        // Get the chat message (this is hypothetical - actual implementation will depend on game's chat system)
        string message = GetLastChatMessage();
        
        // Check if it's a stats command
        if (message.StartsWith(COMMAND_PREFIX))
        {
            // Process command
            ProcessStatsCommand(message, entity);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Process stats commands
    protected void ProcessStatsCommand(string message, IEntity player)
    {
        // Split command into parts
        array<string> parts = new array<string>();
        message.Split(" ", parts);
        
        // Must have at least one part (the command itself)
        if (parts.Count() < 1)
            return;
            
        // Get player ID
        PlayerIdentity identity = PlayerIdentity.Cast(player.GetIdentity());
        if (!identity)
            return;
            
        string playerId = identity.GetPlainId();
        
        // Process command
        if (parts.Count() == 1)
        {
            // Just !stats - show player's own stats
            m_UIManager.ShowPlayerStats(playerId);
        }
        else if (parts[1] == "leaderboard" || parts[1] == "top")
        {
            // !stats leaderboard [category] [count]
            string category = "kills";
            int count = 10;
            
            if (parts.Count() >= 3)
                category = parts[2];
                
            if (parts.Count() >= 4)
                count = parts[3].ToInt();
                
            m_UIManager.ShowLeaderboard(category, count);
        }
        else if (parts[1] == "hide")
        {
            // !stats hide - hide any open stats windows
            m_UIManager.HidePlayerStats();
            m_UIManager.HideLeaderboard();
        }
        else if (parts[1] == "help")
        {
            // !stats help - show command help
            SendChatMessage(player, "Stats Commands: !stats, !stats leaderboard [category] [count], !stats hide, !stats help");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a chat message to a player
    protected void SendChatMessage(IEntity player, string message)
    {
        // Actual implementation will depend on game's chat system
        // This is just a placeholder
        Print("[StatTracker] Sending chat message to player: " + message);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the last chat message
    protected string GetLastChatMessage()
    {
        // Actual implementation will depend on game's chat system
        // This is just a placeholder
        return "";
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle player killed event
    protected void OnPlayerKilled(IEntity victim, IEntity killer, string weaponName, float distance, bool isHeadshot)
    {
        // Get victim ID
        PlayerIdentity victimIdentity = PlayerIdentity.Cast(victim.GetIdentity());
        if (!victimIdentity)
            return;
            
        string victimId = victimIdentity.GetPlainId();
        
        // Record death for victim
        STS_PlayerStatsComponent victimComponent = m_PlayerComponents.Get(victimId);
        if (victimComponent)
        {
            victimComponent.RecordDeath(killer);
        }
        
        // If killer is a player, record kill
        if (killer)
        {
            PlayerIdentity killerIdentity = PlayerIdentity.Cast(killer.GetIdentity());
            if (killerIdentity)
            {
                string killerId = killerIdentity.GetPlainId();
                
                STS_PlayerStatsComponent killerComponent = m_PlayerComponents.Get(killerId);
                if (killerComponent)
                {
                    killerComponent.RecordKill(victim, weaponName, distance, isHeadshot);
                }
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle entity damaged event
    protected void OnEntityDamaged(IEntity victim, IEntity attacker, float damage, EDamageType damageType, int hitZone)
    {
        // If victim is not a player, ignore
        PlayerIdentity victimIdentity = PlayerIdentity.Cast(victim.GetIdentity());
        if (!victimIdentity)
            return;
            
        string victimId = victimIdentity.GetPlainId();
        
        // Record damage taken
        STS_PlayerStatsComponent victimComponent = m_PlayerComponents.Get(victimId);
        if (victimComponent)
        {
            victimComponent.RecordDamageTaken(damage, damageType, hitZone, attacker);
        }
        
        // If attacker is a player, record damage dealt
        if (attacker)
        {
            PlayerIdentity attackerIdentity = PlayerIdentity.Cast(attacker.GetIdentity());
            if (attackerIdentity)
            {
                string attackerId = attackerIdentity.GetPlainId();
                
                STS_PlayerStatsComponent attackerComponent = m_PlayerComponents.Get(attackerId);
                if (attackerComponent)
                {
                    attackerComponent.RecordDamageDealt(damage, damageType, hitZone, victim);
                }
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle vehicle destroyed event
    protected void OnVehicleDestroyed(IEntity vehicle, IEntity destroyer)
    {
        // If destroyer is not a player, ignore
        if (!destroyer)
            return;
            
        PlayerIdentity destroyerIdentity = PlayerIdentity.Cast(destroyer.GetIdentity());
        if (!destroyerIdentity)
            return;
            
        string destroyerId = destroyerIdentity.GetPlainId();
        
        // Record vehicle kill
        STS_PlayerStatsComponent destroyerComponent = m_PlayerComponents.Get(destroyerId);
        if (destroyerComponent)
        {
            destroyerComponent.RecordVehicleKill(vehicle);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle base captured event
    protected void OnBaseCaptured(string baseName, IEntity capturingPlayer)
    {
        // If capturingPlayer is not a player, ignore
        if (!capturingPlayer)
            return;
            
        PlayerIdentity playerIdentity = PlayerIdentity.Cast(capturingPlayer.GetIdentity());
        if (!playerIdentity)
            return;
            
        string playerId = playerIdentity.GetPlainId();
        
        // Record base capture
        STS_PlayerStatsComponent component = m_PlayerComponents.Get(playerId);
        if (component)
        {
            component.RecordBaseCaptured(baseName);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle supply delivered event
    protected void OnSupplyDelivered(int amount, IEntity deliveringPlayer)
    {
        // If deliveringPlayer is not a player, ignore
        if (!deliveringPlayer)
            return;
            
        PlayerIdentity playerIdentity = PlayerIdentity.Cast(deliveringPlayer.GetIdentity());
        if (!playerIdentity)
            return;
            
        string playerId = playerIdentity.GetPlainId();
        
        // Record supply delivery
        STS_PlayerStatsComponent component = m_PlayerComponents.Get(playerId);
        if (component)
        {
            component.RecordSupplyDelivery(amount);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle item purchased event
    protected void OnItemPurchased(string itemName, int count, int price, IEntity buyer)
    {
        // If buyer is not a player, ignore
        if (!buyer)
            return;
            
        PlayerIdentity playerIdentity = PlayerIdentity.Cast(buyer.GetIdentity());
        if (!playerIdentity)
            return;
            
        string playerId = playerIdentity.GetPlainId();
        
        // Record purchase
        STS_PlayerStatsComponent component = m_PlayerComponents.Get(playerId);
        if (component)
        {
            component.RecordItemPurchase(itemName, count, price);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle item sold event
    protected void OnItemSold(string itemName, int count, int price, IEntity seller)
    {
        // If seller is not a player, ignore
        if (!seller)
            return;
            
        PlayerIdentity playerIdentity = PlayerIdentity.Cast(seller.GetIdentity());
        if (!playerIdentity)
            return;
            
        string playerId = playerIdentity.GetPlainId();
        
        // Record sale
        STS_PlayerStatsComponent component = m_PlayerComponents.Get(playerId);
        if (component)
        {
            component.RecordItemSale(itemName, count, price);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Award XP to a player
    void AwardXP(IEntity player, int amount, string reason = "")
    {
        if (!player)
            return;
            
        PlayerIdentity playerIdentity = PlayerIdentity.Cast(player.GetIdentity());
        if (!playerIdentity)
            return;
            
        string playerId = playerIdentity.GetPlainId();
        
        // Award XP
        STS_PlayerStatsComponent component = m_PlayerComponents.Get(playerId);
        if (component)
        {
            component.AwardXP(amount, reason);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnGameModeEnd()
    {
        // Clean up
        GetGame().GetCallqueue().Remove(CheckForNewPlayers);
        
        super.OnGameModeEnd();
    }
} 