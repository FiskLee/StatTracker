// STS_UIManager.c
// Manages UI elements for displaying player statistics

class STS_UIManager
{
    // Singleton instance
    private static ref STS_UIManager s_Instance;
    
    // Reference to the persistence manager
    protected STS_PersistenceManager m_PersistenceManager;
    
    // UI layouts
    protected ResourceName m_StatsMenuLayout = "{E7B5081F0BCAF3BF}UI/Layouts/StatsMenu.layout";
    protected ResourceName m_LeaderboardLayout = "{D3461C40AB6B291E}UI/Layouts/Leaderboard.layout";
    
    // Active UI elements
    protected Widget m_StatsMenu;
    protected Widget m_LeaderboardMenu;
    
    // Currently displayed player ID
    protected string m_CurrentPlayerID;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_UIManager()
    {
        // Get persistence manager
        m_PersistenceManager = STS_PersistenceManager.GetInstance();
        
        Print("[StatTracker] UI Manager initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_UIManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_UIManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Show statistics menu for a player
    void ShowPlayerStats(string playerId, string playerName = "")
    {
        // Store current player ID
        m_CurrentPlayerID = playerId;
        
        // Load player stats
        STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
        if (!stats)
        {
            Print("[StatTracker] ERROR: Could not load stats for player: " + playerId);
            return;
        }
        
        // Create menu if it doesn't exist
        if (!m_StatsMenu)
        {
            m_StatsMenu = GetGame().GetWorkspace().CreateWidgets(m_StatsMenuLayout);
            if (!m_StatsMenu)
            {
                Print("[StatTracker] ERROR: Could not create stats menu");
                return;
            }
        }
        
        // Update UI with player stats
        UpdateStatsMenuUI(stats, playerName);
        
        // Show the menu
        m_StatsMenu.SetVisible(true);
    }
    
    //------------------------------------------------------------------------------------------------
    // Hide statistics menu
    void HidePlayerStats()
    {
        if (m_StatsMenu)
        {
            m_StatsMenu.SetVisible(false);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update stats menu UI with player data
    protected void UpdateStatsMenuUI(STS_EnhancedPlayerStats stats, string playerName = "")
    {
        if (!m_StatsMenu || !stats)
            return;
            
        // Set player name
        TextWidget nameText = TextWidget.Cast(m_StatsMenu.FindAnyWidget("PlayerNameText"));
        if (nameText)
        {
            if (playerName != "")
                nameText.SetText(playerName);
            else if (stats.m_sPlayerName != "")
                nameText.SetText(stats.m_sPlayerName);
            else
                nameText.SetText("Unknown Player");
        }
        
        // Set player rank
        TextWidget rankText = TextWidget.Cast(m_StatsMenu.FindAnyWidget("RankText"));
        if (rankText)
        {
            rankText.SetText("Rank: " + stats.m_iRank.ToString());
        }
        
        // Set general stats
        SetTextValue("PlaytimeText", FormatPlaytime(stats.m_iTotalPlaytimeSeconds));
        SetTextValue("SessionsText", "Sessions: " + stats.m_iSessionCount.ToString());
        SetTextValue("FirstLoginText", "First Login: " + FormatTimestamp(stats.m_iFirstLoginTime));
        SetTextValue("LastLoginText", "Last Login: " + FormatTimestamp(stats.m_iLastLoginTime));
        
        // Set combat stats
        SetTextValue("KillsText", "Kills: " + stats.m_iKills.ToString());
        SetTextValue("DeathsText", "Deaths: " + stats.m_iDeaths.ToString());
        SetTextValue("KDRatioText", "K/D: " + FormatRatio(stats.m_iKills, stats.m_iDeaths));
        SetTextValue("HeadshotsText", "Headshots: " + stats.m_iHeadshotKills.ToString());
        SetTextValue("HeadshotRatioText", "HS%: " + FormatPercentage(stats.m_iHeadshotKills, stats.m_iKills));
        SetTextValue("LongestKillText", "Longest Kill: " + stats.m_fLongestKillDistance.ToString() + "m");
        SetTextValue("KillstreakText", "Best Killstreak: " + stats.m_iLongestKillstreak.ToString());
        
        // Set damage stats
        SetTextValue("DamageDealtText", "Damage Dealt: " + stats.m_fTotalDamageDealt.ToString("0"));
        SetTextValue("DamageTakenText", "Damage Taken: " + stats.m_fTotalDamageTaken.ToString("0"));
        SetTextValue("UnconsciousText", "Knocked Out: " + stats.m_iUnconsciousCount.ToString() + " times");
        
        // Set vehicle stats
        SetTextValue("VehicleKillsText", "Vehicle Kills: " + stats.m_iVehicleKills.ToString());
        SetTextValue("AirKillsText", "Aircraft Kills: " + stats.m_iAirKills.ToString());
        
        // Set movement stats
        SetTextValue("DistanceTraveledText", "Distance: " + FormatDistance(stats.m_fTotalDistanceTraveled));
        SetTextValue("VehicleDistanceText", "Vehicle Distance: " + FormatDistance(stats.m_fVehicleDistanceTraveled));
        SetTextValue("FootDistanceText", "On Foot: " + FormatDistance(stats.m_fFootDistanceTraveled));
        
        // Set economic stats
        SetTextValue("MoneyEarnedText", "Money Earned: $" + stats.m_iTotalMoneyEarned.ToString());
        SetTextValue("MoneySpentText", "Money Spent: $" + stats.m_iTotalMoneySpent.ToString());
        
        // Set objective stats
        SetTextValue("BasesCapturedText", "Bases Captured: " + stats.m_iBasesCaptured.ToString());
        SetTextValue("BasesLostText", "Bases Lost: " + stats.m_iBasesLost.ToString());
        SetTextValue("SuppliesDeliveredText", "Supplies Delivered: " + stats.m_iSuppliesDelivered.ToString());
        
        // Set achievement stats
        SetTextValue("AchievementsText", "Achievements: " + stats.m_iAchievementsCompleted.ToString() + "/" + stats.m_iAchievementsTotal.ToString());
        SetTextValue("ChallengesText", "Challenges: " + stats.m_iChallengesCompleted.ToString() + "/" + stats.m_iChallengesTotal.ToString());
        
        // Set favorite weapon
        SetTextValue("FavoriteWeaponText", "Favorite Weapon: " + GetFavoriteWeapon(stats));
        
        // Set XP related stats
        SetTextValue("XPText", "XP: " + stats.m_iTotalXP.ToString());
        SetTextValue("NextRankText", "Next Rank: " + GetXPForNextRank(stats.m_iRank, stats.m_iTotalXP));
        
        // Set leaderboard positions
        UpdateLeaderboardPositions(stats);
    }
    
    //------------------------------------------------------------------------------------------------
    // Update leaderboard positions in UI
    protected void UpdateLeaderboardPositions(STS_EnhancedPlayerStats stats)
    {
        // Get positions from persistence manager
        int killsRank = m_PersistenceManager.GetPlayerLeaderboardPosition(m_CurrentPlayerID, "kills");
        int killstreakRank = m_PersistenceManager.GetPlayerLeaderboardPosition(m_CurrentPlayerID, "killstreak");
        int xpRank = m_PersistenceManager.GetPlayerLeaderboardPosition(m_CurrentPlayerID, "xp");
        
        // Update UI
        SetTextValue("KillsRankText", "Rank: #" + (killsRank > 0 ? killsRank.ToString() : "N/A"));
        SetTextValue("KillstreakRankText", "Rank: #" + (killstreakRank > 0 ? killstreakRank.ToString() : "N/A"));
        SetTextValue("XPRankText", "Rank: #" + (xpRank > 0 ? xpRank.ToString() : "N/A"));
    }
    
    //------------------------------------------------------------------------------------------------
    // Show leaderboard UI
    void ShowLeaderboard(string category = "kills", int count = 10)
    {
        // Create menu if it doesn't exist
        if (!m_LeaderboardMenu)
        {
            m_LeaderboardMenu = GetGame().GetWorkspace().CreateWidgets(m_LeaderboardLayout);
            if (!m_LeaderboardMenu)
            {
                Print("[StatTracker] ERROR: Could not create leaderboard menu");
                return;
            }
        }
        
        // Get top players
        array<ref STS_LeaderboardEntry> topPlayers = m_PersistenceManager.GetTopPlayers(category, count);
        
        // Update UI
        UpdateLeaderboardUI(topPlayers, category);
        
        // Show the menu
        m_LeaderboardMenu.SetVisible(true);
    }
    
    //------------------------------------------------------------------------------------------------
    // Hide leaderboard UI
    void HideLeaderboard()
    {
        if (m_LeaderboardMenu)
        {
            m_LeaderboardMenu.SetVisible(false);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update leaderboard UI
    protected void UpdateLeaderboardUI(array<ref STS_LeaderboardEntry> entries, string category)
    {
        if (!m_LeaderboardMenu || !entries)
            return;
            
        // Set category title
        TextWidget titleText = TextWidget.Cast(m_LeaderboardMenu.FindAnyWidget("CategoryTitleText"));
        if (titleText)
        {
            string title = "Top Players - ";
            
            // Format category name for display
            switch (category)
            {
                case "kills": title += "Kills"; break;
                case "killstreak": title += "Killstreaks"; break;
                case "deaths": title += "Deaths"; break;
                case "playtime": title += "Playtime"; break;
                case "damage_dealt": title += "Damage Dealt"; break;
                case "headshots": title += "Headshots"; break;
                case "distance_traveled": title += "Distance Traveled"; break;
                case "money_earned": title += "Money Earned"; break;
                case "xp": title += "Experience"; break;
                default: title += category; break;
            }
            
            titleText.SetText(title);
        }
        
        // Clear existing entries
        Widget entriesContainer = m_LeaderboardMenu.FindAnyWidget("EntriesContainer");
        if (entriesContainer)
        {
            entriesContainer.ClearItems();
        }
        else
        {
            return;
        }
        
        // Add entries
        for (int i = 0; i < entries.Count(); i++)
        {
            // Create entry widget
            Widget entryWidget = GetGame().GetWorkspace().CreateWidgets("{F234A07B7E2C5D1A}UI/Layouts/LeaderboardEntry.layout", entriesContainer);
            if (!entryWidget)
                continue;
                
            // Set rank
            TextWidget rankText = TextWidget.Cast(entryWidget.FindAnyWidget("RankText"));
            if (rankText)
            {
                rankText.SetText("#" + entries[i].m_iPosition.ToString());
            }
            
            // Set name
            TextWidget nameText = TextWidget.Cast(entryWidget.FindAnyWidget("NameText"));
            if (nameText)
            {
                nameText.SetText(entries[i].m_sPlayerName);
            }
            
            // Format value based on category
            string valueStr = FormatLeaderboardValue(entries[i].m_fValue, category);
            
            // Set value
            TextWidget valueText = TextWidget.Cast(entryWidget.FindAnyWidget("ValueText"));
            if (valueText)
            {
                valueText.SetText(valueStr);
            }
            
            // Highlight current player
            if (entries[i].m_sPlayerId == m_CurrentPlayerID)
            {
                entryWidget.SetColor(Color.Yellow);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Format leaderboard value based on category
    protected string FormatLeaderboardValue(float value, string category)
    {
        switch (category)
        {
            case "playtime":
                return FormatPlaytime(value);
                
            case "distance_traveled":
                return FormatDistance(value);
                
            case "money_earned":
                return "$" + value.ToString("0");
                
            case "damage_dealt":
                return value.ToString("0");
                
            default:
                return value.ToString("0");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Set text value for a UI element
    protected void SetTextValue(string widgetName, string value)
    {
        TextWidget widget = TextWidget.Cast(m_StatsMenu.FindAnyWidget(widgetName));
        if (widget)
        {
            widget.SetText(value);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Format playtime (seconds -> hours, minutes, seconds)
    protected string FormatPlaytime(float seconds)
    {
        int totalSeconds = seconds;
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int secs = totalSeconds % 60;
        
        if (hours > 0)
            return string.Format("%1h %2m %3s", hours, minutes, secs);
        else if (minutes > 0)
            return string.Format("%1m %2s", minutes, secs);
        else
            return string.Format("%1s", secs);
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Format ratio (kills/deaths)
    protected string FormatRatio(int numerator, int denominator)
    {
        if (denominator == 0)
            return numerator.ToString();
            
        float ratio = numerator / denominator;
        return ratio.ToString("0.00");
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Format percentage
    protected string FormatPercentage(int part, int total)
    {
        if (total == 0)
            return "0%";
            
        int percentage = Math.Round((part / total) * 100);
        return percentage.ToString() + "%";
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Format distance (meters -> km or m)
    protected string FormatDistance(float meters)
    {
        if (meters >= 1000)
        {
            float km = meters / 1000;
            return km.ToString("0.0") + " km";
        }
        else
        {
            return meters.ToString("0") + " m";
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Format timestamp (unix timestamp -> date string)
    protected string FormatTimestamp(int timestamp)
    {
        if (timestamp == 0)
            return "Never";
            
        TimeAndDate date = System.GetTimeAndDateFromUnixTime(timestamp);
        return string.Format("%1/%2/%3", date.Month, date.Day, date.Year);
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Get favorite weapon from stats
    protected string GetFavoriteWeapon(STS_EnhancedPlayerStats stats)
    {
        if (!stats || !stats.m_mWeaponKills || stats.m_mWeaponKills.Count() == 0)
            return "None";
            
        string favoriteWeapon = "";
        int mostKills = 0;
        
        foreach (string weapon, int kills : stats.m_mWeaponKills)
        {
            if (kills > mostKills)
            {
                favoriteWeapon = weapon;
                mostKills = kills;
            }
        }
        
        return favoriteWeapon;
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Get XP needed for next rank
    protected string GetXPForNextRank(int currentRank, int currentXP)
    {
        int nextRankXP = 0;
        
        switch (currentRank)
        {
            case 0: nextRankXP = 100; break;
            case 1: nextRankXP = 500; break;
            case 2: nextRankXP = 1000; break;
            case 3: nextRankXP = 2500; break;
            case 4: nextRankXP = 5000; break;
            case 5: nextRankXP = 10000; break;
            case 6: nextRankXP = 15000; break;
            case 7: nextRankXP = 25000; break;
            case 8: nextRankXP = 50000; break;
            case 9: nextRankXP = 100000; break;
            default: return "Max Rank";
        }
        
        int xpNeeded = nextRankXP - currentXP;
        if (xpNeeded <= 0)
            return "Ready for promotion!";
            
        return xpNeeded.ToString() + " XP needed";
    }
} 