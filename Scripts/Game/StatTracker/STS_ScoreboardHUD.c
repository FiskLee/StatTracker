// STS_ScoreboardHUD.c
// HUD for displaying player statistics

class STS_ScoreboardHUD : SCR_InfoDisplayExtended
{
    // HUD layout references
    protected Widget m_wRoot;
    protected Widget m_wScoreboardPanel;
    protected Widget m_wScoreboardPanelContent;
    protected Widget m_wMiniScorePanel;
    protected TextWidget m_wPlayerRankText;
    protected TextWidget m_wPlayerXPText;
    protected TextWidget m_wPlayerKillsText;
    protected TextWidget m_wPlayerDeathsText;
    
    // Store player stats locally
    protected ref array<int> m_aPlayerIDs = new array<int>();
    protected ref array<ref STS_PlayerStats> m_aPlayerStats = new array<ref STS_PlayerStats>();
    protected ref array<string> m_aPlayerNames = new array<string>();
    
    // Flag to indicate if scoreboard is visible
    protected bool m_bScoreboardVisible = false;
    
    // Input action to toggle scoreboard visibility
    protected static const string s_sToggleScoreboardAction = "STS_ToggleScoreboard";
    
    // Logging system
    protected STS_LoggingSystem m_Logger;
    
    // Widget manager
    protected SCR_Workspace m_WidgetManager;
    
    // Root widget
    protected Widget m_RootWidget;
    
    // Header widget
    protected VerticalLayoutWidget m_HeaderWidget;
    
    // Players widget
    protected GridLayoutWidget m_PlayersWidget;
    
    // Last update time
    protected float m_LastUpdateTime = 0;
    
    // Update interval
    protected const float UPDATE_INTERVAL = 1.0;
    
    //------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        
        try 
        {
            // Get logging system
            m_Logger = STS_LoggingSystem.GetInstance();
            if (!m_Logger)
            {
                Print("STS_ScoreboardHUD: Failed to get logging system", LogLevel.ERROR);
                return;
            }
            
            m_Logger.LogDebug("STS_ScoreboardHUD: Initializing...");
            
            // Initialize scoreboard properties
            m_bScoreboardVisible = false;
            m_LastUpdateTime = 0;
            
            // Get widget manager and layout
            m_WidgetManager = GetGame().GetWorkspace();
            if (!m_WidgetManager)
            {
                m_Logger.LogError("Failed to get widget manager - scoreboard will not function!");
                return;
            }
            
            // Create scoreboard layout
            m_RootWidget = GetGame().GetWorkspace().CreateWidgets("StatTracker/GUI/Layouts/Scoreboard.layout");
            if (!m_RootWidget)
            {
                m_Logger.LogError("Failed to create scoreboard widget from layout - check file path!");
                return;
            }
            
            // Hide scoreboard by default
            m_RootWidget.SetVisible(false);
            
            // Find and store child widgets
            m_HeaderWidget = VerticalLayoutWidget.Cast(m_RootWidget.FindAnyWidget("HeaderLayout"));
            m_PlayersWidget = GridLayoutWidget.Cast(m_RootWidget.FindAnyWidget("PlayersGrid"));
            
            if (!m_HeaderWidget || !m_PlayersWidget)
            {
                m_Logger.LogError("Failed to find required scoreboard widgets - check layout file!");
                return;
            }
            
            // Set up input handling
            GetGame().GetInputManager().AddActionListener("Scoreboard", EActionTrigger.DOWN, ShowScoreboard);
            GetGame().GetInputManager().AddActionListener("Scoreboard", EActionTrigger.UP, HideScoreboard);
            
            // Get stat tracking manager
            m_StatTrackingManager = STS_StatTrackingManagerComponent.GetInstance();
            if (!m_StatTrackingManager)
            {
                m_Logger.LogError("Failed to find stat tracking manager - scoreboard will not show stats!");
            }
            
            m_Logger.LogInfo("STS_ScoreboardHUD: Initialized successfully");
        }
        catch (Exception e)
        {
            Print("STS_ScoreboardHUD: Exception during initialization: " + e.ToString(), LogLevel.ERROR);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnDeinit()
    {
        // Unregister input handler
        GetGame().GetInputManager().RemoveActionListener(s_sToggleScoreboardAction, EActionTrigger.DOWN, ToggleScoreboard);
        
        super.OnDeinit();
    }
    
    //------------------------------------------------------------------------------------------------
    override void Update(float timeSlice)
    {
        super.Update(timeSlice);
        
        // Update the mini score panel if it's visible
        if (m_wMiniScorePanel.IsVisible())
        {
            UpdateMiniScorePanel();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Toggle scoreboard visibility
    protected bool ToggleScoreboard(float value)
    {
        // Toggle visibility
        m_bScoreboardVisible = !m_bScoreboardVisible;
        
        // Update UI
        m_wScoreboardPanel.SetVisible(m_bScoreboardVisible);
        
        // If scoreboard is now visible, update it
        if (m_bScoreboardVisible)
        {
            UpdateFullScoreboard();
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Update the scoreboard with latest stats
    void UpdateScoreboard(array<int> playerIDs, array<ref STS_PlayerStats> playerStats, array<string> playerNames)
    {
        // Store the data locally
        m_aPlayerIDs.Copy(playerIDs);
        m_aPlayerNames.Copy(playerNames);
        
        // Deep copy of player stats
        m_aPlayerStats.Clear();
        foreach (STS_PlayerStats stats : playerStats)
        {
            m_aPlayerStats.Insert(stats);
        }
        
        // Update UI if scoreboard is visible
        if (m_bScoreboardVisible)
        {
            UpdateFullScoreboard();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update the mini score panel with local player stats
    protected void UpdateMiniScorePanel()
    {
        // Get local player ID
        int localPlayerId = GetGame().GetPlayerController().GetPlayerId();
        
        // Find local player stats
        STS_PlayerStats localStats = null;
        for (int i = 0; i < m_aPlayerIDs.Count(); i++)
        {
            if (m_aPlayerIDs[i] == localPlayerId)
            {
                localStats = m_aPlayerStats[i];
                break;
            }
        }
        
        // Update UI with local player stats
        if (localStats)
        {
            m_wPlayerRankText.SetText("Rank: " + localStats.m_iRank.ToString());
            m_wPlayerXPText.SetText("XP: " + localStats.m_iTotalXP.ToString());
            m_wPlayerKillsText.SetText("Kills: " + localStats.m_iKills.ToString());
            m_wPlayerDeathsText.SetText("Deaths: " + localStats.m_iDeaths.ToString());
        }
        else
        {
            // No stats found for local player
            m_wPlayerRankText.SetText("Rank: 0");
            m_wPlayerXPText.SetText("XP: 0");
            m_wPlayerKillsText.SetText("Kills: 0");
            m_wPlayerDeathsText.SetText("Deaths: 0");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update the full scoreboard with all player stats
    protected void UpdateFullScoreboard()
    {
        if (!m_StatTrackingManager || !m_PlayersWidget || !m_HeaderWidget)
        {
            m_Logger.LogWarning("Cannot update scoreboard - missing required components");
            return;
        }
        
        try
        {
            float currentTime = System.GetTickCount() / 1000.0;
            
            // Only update at limited rate to avoid performance issues
            if (currentTime - m_LastUpdateTime < UPDATE_INTERVAL)
                return;
            
            m_LastUpdateTime = currentTime;
            
            m_Logger.LogDebug("Updating scoreboard display");
            
            // Clear existing player entries
            m_PlayersWidget.RemoveAllChildren();
            
            // Get player data from stat manager
            array<ref STS_PlayerStats> playerStats = m_StatTrackingManager.GetAllPlayerStats();
            if (!playerStats || playerStats.Count() == 0)
            {
                m_Logger.LogDebug("No player stats available to display");
                
                // Create a message widget to show when no players have stats
                TextWidget noPlayersText = TextWidget.Cast(GetGame().GetWorkspace().CreateWidgets("StatTracker/GUI/Layouts/MessageText.layout"));
                if (noPlayersText)
                {
                    noPlayersText.SetText("No player statistics available");
                    m_PlayersWidget.AddChild(noPlayersText);
                }
                return;
            }
            
            // Sort player stats by score
            playerStats.Sort(SortPlayersByScore);
            
            // Add each player to the scoreboard
            foreach (STS_PlayerStats stats : playerStats)
            {
                if (!stats) 
                {
                    m_Logger.LogWarning("Null player stats encountered - skipping");
                    continue;
                }
                
                Widget playerRow = GetGame().GetWorkspace().CreateWidgets("StatTracker/GUI/Layouts/PlayerRow.layout");
                if (!playerRow)
                {
                    m_Logger.LogError("Failed to create player row widget - check layout file!");
                    continue;
                }
                
                TextWidget nameText = TextWidget.Cast(playerRow.FindAnyWidget("PlayerName"));
                TextWidget killsText = TextWidget.Cast(playerRow.FindAnyWidget("PlayerKills"));
                TextWidget deathsText = TextWidget.Cast(playerRow.FindAnyWidget("PlayerDeaths"));
                TextWidget scoreText = TextWidget.Cast(playerRow.FindAnyWidget("PlayerScore"));
                
                if (!nameText || !killsText || !deathsText || !scoreText)
                {
                    m_Logger.LogError("Player row widget is missing required text fields!");
                    continue;
                }
                
                nameText.SetText(stats.GetPlayerName());
                killsText.SetText(stats.GetKills().ToString());
                deathsText.SetText(stats.GetDeaths().ToString());
                scoreText.SetText(stats.GetScore().ToString());
                
                ImageWidget factionIcon = ImageWidget.Cast(playerRow.FindAnyWidget("FactionIcon"));
                if (factionIcon)
                {
                    // Set faction icon based on player's team
                    int teamId = stats.GetTeamId();
                    string iconPath = GetFactionIconPath(teamId);
                    if (iconPath != "")
                    {
                        factionIcon.LoadImageTexture(0, iconPath);
                    }
                    else
                    {
                        factionIcon.SetVisible(false);
                    }
                }
                
                m_PlayersWidget.AddChild(playerRow);
            }
            
            m_Logger.LogDebug(string.Format("Scoreboard updated with %1 players", playerStats.Count()));
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception while updating scoreboard: " + e.ToString());
        }
    }
    
    // Get faction icon path based on team ID
    protected string GetFactionIconPath(int teamId)
    {
        try
        {
            // Default path for unknown team
            string iconPath = "StatTracker/GUI/Textures/faction_unknown.edds";
            
            // Get faction manager
            SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
            if (!factionManager)
            {
                m_Logger.LogWarning("Failed to get faction manager - using default faction icon");
                return iconPath;
            }
            
            // Get faction by ID
            Faction faction = factionManager.GetFactionByIndex(teamId);
            if (!faction)
            {
                m_Logger.LogWarning(string.Format("Unknown faction ID %1 - using default faction icon", teamId));
                return iconPath;
            }
            
            // Get faction resources
            ResourceName factionResourceName = faction.GetFactionResourceName();
            if (factionResourceName.IsEmpty())
            {
                return iconPath;
            }
            
            Resource factionResource = Resource.Load(factionResourceName);
            if (!factionResource)
            {
                return iconPath;
            }
            
            // Extract faction icon path
            BaseResourceObject factionObject = factionResource.GetResource();
            if (!factionObject)
            {
                return iconPath;
            }
            
            SCR_Faction scrFaction = SCR_Faction.Cast(factionObject);
            if (scrFaction)
            {
                ResourceName iconResourceName = scrFaction.GetFactionIcon();
                if (!iconResourceName.IsEmpty())
                {
                    return iconResourceName;
                }
            }
            
            return iconPath;
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception in GetFactionIconPath: " + e.ToString());
            return "";
        }
    }
    
    // Helper method to sort players by score
    static int SortPlayersByScore(STS_PlayerStats a, STS_PlayerStats b)
    {
        if (!a && !b) return 0;
        if (!a) return 1;
        if (!b) return -1;
        
        // Sort by score (descending)
        if (a.GetScore() > b.GetScore()) return -1;
        if (a.GetScore() < b.GetScore()) return 1;
        
        // If scores are equal, sort by kills
        if (a.GetKills() > b.GetKills()) return -1;
        if (a.GetKills() < b.GetKills()) return 1;
        
        // If kills are equal, sort by deaths (ascending)
        if (a.GetDeaths() < b.GetDeaths()) return -1;
        if (a.GetDeaths() > b.GetDeaths()) return 1;
        
        return 0;
    }
    
    void ShowScoreboard()
    {
        try
        {
            if (!m_RootWidget)
            {
                m_Logger.LogWarning("Cannot show scoreboard - root widget is null");
                return;
            }
            
            m_bScoreboardVisible = true;
            m_RootWidget.SetVisible(true);
            
            // Update scoreboard immediately when shown
            UpdateScoreboard();
            
            m_Logger.LogDebug("Scoreboard displayed");
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception in ShowScoreboard: " + e.ToString());
        }
    }
    
    void HideScoreboard()
    {
        try
        {
            if (!m_RootWidget)
            {
                m_Logger.LogWarning("Cannot hide scoreboard - root widget is null");
                return;
            }
            
            m_bScoreboardVisible = false;
            m_RootWidget.SetVisible(false);
            
            m_Logger.LogDebug("Scoreboard hidden");
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception in HideScoreboard: " + e.ToString());
        }
    }
} 