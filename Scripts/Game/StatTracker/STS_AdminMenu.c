// STS_AdminMenu.c
// In-game admin menu for the StatTracker system

class STS_AdminMenu
{
    // Singleton instance
    private static ref STS_AdminMenu s_Instance;
    
    // Config reference
    protected STS_Config m_Config;
    
    // Persistence manager reference
    protected STS_PersistenceManager m_PersistenceManager;
    
    // UI references
    protected ref Widget m_MenuRoot;
    protected ref Widget m_StatsPanel;
    protected ref TextListboxWidget m_PlayerList;
    protected ref TextListboxWidget m_StatsList;
    protected ref ButtonWidget m_KickButton;
    protected ref ButtonWidget m_BanButton;
    protected ref ButtonWidget m_MessageButton;
    protected ref ButtonWidget m_AnnounceButton;
    protected ref EditBoxWidget m_MessageInput;
    
    // Selected player
    protected string m_SelectedPlayerId;
    protected string m_SelectedPlayerName;
    
    // Admin list (loaded from config)
    protected ref array<string> m_AdminList;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_AdminMenu()
    {
        m_Config = STS_Config.GetInstance();
        m_PersistenceManager = STS_PersistenceManager.GetInstance();
        
        // Register keybind for opening menu
        GetGame().GetInput().RegisterAction("OpenAdminMenu", "Open Admin Menu", "UAOpenAdminMenu");
        
        Print("[StatTracker] Admin Menu initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_AdminMenu GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_AdminMenu();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if a player is an admin
    bool IsAdmin(PlayerBase player)
    {
        if (!player)
            return false;
            
        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return false;
            
        string playerId = identity.GetId();
        
        // Load admin list if not loaded
        if (!m_AdminList)
        {
            m_AdminList = LoadAdminList();
        }
        
        // Check if player ID is in admin list
        return m_AdminList.Find(playerId) != -1;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load admin list from config file
    protected array<string> LoadAdminList()
    {
        array<string> admins = new array<string>();
        
        // This is a placeholder. In a real implementation, you would load the admin list
        // from a configuration file or database.
        
        // For testing, add a default admin
        admins.Insert("123456789"); // Example admin ID
        
        return admins;
    }
    
    //------------------------------------------------------------------------------------------------
    // Toggle the admin menu
    void ToggleMenu(PlayerBase player)
    {
        // Check if player is an admin
        if (!IsAdmin(player))
        {
            // Not an admin, don't show menu
            return;
        }
        
        // If menu is open, close it
        if (m_MenuRoot && m_MenuRoot.IsVisible())
        {
            CloseMenu();
            return;
        }
        
        // Open the menu
        OpenMenu();
    }
    
    //------------------------------------------------------------------------------------------------
    // Open the admin menu
    protected void OpenMenu()
    {
        // Create menu if it doesn't exist
        if (!m_MenuRoot)
        {
            m_MenuRoot = GetGame().GetWorkspace().CreateWidgets("StatTracker/GUI/layouts/admin_menu.layout");
            
            // Get UI elements
            m_PlayerList = TextListboxWidget.Cast(m_MenuRoot.FindAnyWidget("PlayerList"));
            m_StatsList = TextListboxWidget.Cast(m_MenuRoot.FindAnyWidget("StatsList"));
            m_StatsPanel = m_MenuRoot.FindAnyWidget("StatsPanel");
            m_KickButton = ButtonWidget.Cast(m_MenuRoot.FindAnyWidget("KickButton"));
            m_BanButton = ButtonWidget.Cast(m_MenuRoot.FindAnyWidget("BanButton"));
            m_MessageButton = ButtonWidget.Cast(m_MenuRoot.FindAnyWidget("MessageButton"));
            m_AnnounceButton = ButtonWidget.Cast(m_MenuRoot.FindAnyWidget("AnnounceButton"));
            m_MessageInput = EditBoxWidget.Cast(m_MenuRoot.FindAnyWidget("MessageInput"));
            
            // Set event handlers
            m_MenuRoot.SetHandler(this);
        }
        
        // Show the menu
        m_MenuRoot.Show(true);
        
        // Update player list
        UpdatePlayerList();
        
        // Hide stats panel until a player is selected
        m_StatsPanel.Show(false);
    }
    
    //------------------------------------------------------------------------------------------------
    // Close the admin menu
    protected void CloseMenu()
    {
        if (m_MenuRoot)
        {
            m_MenuRoot.Show(false);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update the player list
    protected void UpdatePlayerList()
    {
        if (!m_PlayerList)
            return;
            
        // Clear the list
        m_PlayerList.ClearItems();
        
        // Get all online players
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // Add each player to the list
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (!player)
                continue;
                
            PlayerIdentity identity = player.GetIdentity();
            if (!identity)
                continue;
                
            string playerId = identity.GetId();
            string playerName = identity.GetName();
            
            // Add to list
            m_PlayerList.AddItem(playerName, NULL, 0);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update player stats display
    protected void UpdatePlayerStats(string playerId)
    {
        if (!m_StatsList)
            return;
            
        // Clear the list
        m_StatsList.ClearItems();
        
        // Load player stats
        STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
        if (!stats)
        {
            m_StatsList.AddItem("No stats found for this player", NULL, 0);
            return;
        }
        
        // Store selected player
        m_SelectedPlayerId = playerId;
        m_SelectedPlayerName = stats.m_sPlayerName;
        
        // Add basic stats
        m_StatsList.AddItem("Player Name: " + stats.m_sPlayerName, NULL, 0);
        m_StatsList.AddItem("Player ID: " + playerId, NULL, 0);
        m_StatsList.AddItem("", NULL, 0); // Spacer
        
        // Combat stats
        m_StatsList.AddItem("-- Combat Statistics --", NULL, 0);
        m_StatsList.AddItem(string.Format("Kills: %1", stats.m_iKills), NULL, 0);
        m_StatsList.AddItem(string.Format("Deaths: %1", stats.m_iDeaths), NULL, 0);
        m_StatsList.AddItem(string.Format("K/D Ratio: %.2f", stats.m_iDeaths > 0 ? (float)stats.m_iKills / stats.m_iDeaths : stats.m_iKills), NULL, 0);
        m_StatsList.AddItem(string.Format("Headshots: %1 (%.1f%%)", 
                           stats.m_iHeadshotKills, 
                           stats.m_iKills > 0 ? (float)stats.m_iHeadshotKills / stats.m_iKills * 100 : 0), NULL, 0);
        m_StatsList.AddItem(string.Format("Longest Kill: %.1f m", stats.m_fLongestKill), NULL, 0);
        m_StatsList.AddItem(string.Format("Damage Dealt: %.0f", stats.m_fDamageDealt), NULL, 0);
        m_StatsList.AddItem(string.Format("Damage Taken: %.0f", stats.m_fDamageTaken), NULL, 0);
        m_StatsList.AddItem("", NULL, 0); // Spacer
        
        // Playtime stats
        m_StatsList.AddItem("-- Playtime Statistics --", NULL, 0);
        m_StatsList.AddItem(string.Format("Total Playtime: %1", FormatPlaytime(stats.m_iTotalPlaytimeSeconds)), NULL, 0);
        m_StatsList.AddItem(string.Format("First Login: %1", FormatTimestamp(stats.m_iFirstLogin)), NULL, 0);
        m_StatsList.AddItem(string.Format("Last Login: %1", FormatTimestamp(stats.m_iLastLogin)), NULL, 0);
        m_StatsList.AddItem(string.Format("Total Sessions: %1", stats.m_iTotalSessions), NULL, 0);
        
        // Show the stats panel
        m_StatsPanel.Show(true);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle player list selection
    void OnPlayerSelected(int index)
    {
        // Get the selected player
        string playerName = m_PlayerList.GetItemText(index);
        
        // Find player ID
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (!player)
                continue;
                
            PlayerIdentity identity = player.GetIdentity();
            if (!identity)
                continue;
                
            if (identity.GetName() == playerName)
            {
                // Update stats display
                UpdatePlayerStats(identity.GetId());
                return;
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle kick button click
    void OnKickClicked()
    {
        if (m_SelectedPlayerId.Length() == 0)
            return;
            
        // Prompt for reason
        string reason = "Kicked by admin";
        
        // Send kick command
        SendAdminCommand(STS_RCONCommands.CMD_KICK, m_SelectedPlayerId, reason);
        
        // Close menu
        CloseMenu();
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle ban button click
    void OnBanClicked()
    {
        if (m_SelectedPlayerId.Length() == 0)
            return;
            
        // Prompt for duration and reason
        int duration = 0; // Permanent
        string reason = "Banned by admin";
        
        // Send ban command
        SendAdminCommand(STS_RCONCommands.CMD_BAN, m_SelectedPlayerId, duration.ToString(), reason);
        
        // Close menu
        CloseMenu();
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle message button click
    void OnMessageClicked()
    {
        if (m_SelectedPlayerId.Length() == 0 || !m_MessageInput)
            return;
            
        string message = m_MessageInput.GetText();
        if (message.Length() == 0)
            return;
            
        // Send message command
        SendAdminCommand(STS_RCONCommands.CMD_MSG, m_SelectedPlayerId, message);
        
        // Clear message input
        m_MessageInput.SetText("");
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle announce button click
    void OnAnnounceClicked()
    {
        if (!m_MessageInput)
            return;
            
        string message = m_MessageInput.GetText();
        if (message.Length() == 0)
            return;
            
        // Send announcement command
        SendAdminCommand(STS_RCONCommands.CMD_ANNOUNCE, "all", message);
        
        // Clear message input
        m_MessageInput.SetText("");
    }
    
    //------------------------------------------------------------------------------------------------
    // Send an admin command
    protected void SendAdminCommand(string command, string param1, string param2 = "", string param3 = "")
    {
        // Create parameter array
        array<string> params = new array<string>();
        params.Insert(param1);
        
        if (param2.Length() > 0)
            params.Insert(param2);
            
        if (param3.Length() > 0)
            params.Insert(param3);
            
        // Create command data
        Param2<string, array<string>> data = new Param2<string, array<string>>(command, params);
        
        // Send command to server
        GetGame().GetRPCManager().SendRPC("STS_RCONCommands", "OnRconCommand", data);
    }
    
    //------------------------------------------------------------------------------------------------
    // Format a playtime in seconds to a readable string
    protected string FormatPlaytime(int seconds)
    {
        if (seconds < 60)
            return seconds + " seconds";
            
        if (seconds < 3600)
            return (seconds / 60) + " minutes, " + (seconds % 60) + " seconds";
            
        if (seconds < 86400)
            return (seconds / 3600) + " hours, " + ((seconds % 3600) / 60) + " minutes";
            
        return (seconds / 86400) + " days, " + ((seconds % 86400) / 3600) + " hours";
    }
    
    //------------------------------------------------------------------------------------------------
    // Format a timestamp to a readable date/time
    protected string FormatTimestamp(int timestamp)
    {
        int year, month, day, hour, minute, second;
        GetTimeFromTimestamp(timestamp, year, month, day, hour, minute, second);
        
        return string.Format("%1-%2-%3 %4:%5:%6", 
                            year, 
                            month.ToString().PadLeft(2, "0"), 
                            day.ToString().PadLeft(2, "0"),
                            hour.ToString().PadLeft(2, "0"),
                            minute.ToString().PadLeft(2, "0"),
                            second.ToString().PadLeft(2, "0"));
    }
    
    //------------------------------------------------------------------------------------------------
    // Get date and time from timestamp
    protected void GetTimeFromTimestamp(int timestamp, out int year, out int month, out int day, out int hour, out int minute, out int second)
    {
        System.GetYearMonthDayUTC(timestamp, year, month, day);
        System.GetHourMinuteSecondUTC(timestamp, hour, minute, second);
    }
} 