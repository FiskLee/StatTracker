// STS_ScoreboardComponent.c
// Manages the scoreboard display in the StatTracker UI

class STS_ScoreboardComponent : STS_StatsDisplayComponent
{
    // Layout references
    protected Widget m_wRoot;                      // Root widget
    protected Widget m_wScoreboardPanel;           // Main scoreboard panel
    protected Widget m_wHeaderRow;                 // Header row
    protected Widget m_wPlayerList;                // Player list container
    protected Widget m_wFooter;                    // Footer container
    
    // Player list
    protected ref array<ref STS_ScoreboardPlayerEntry> m_aPlayerEntries = new array<ref STS_ScoreboardPlayerEntry>();
    
    // Sort settings
    protected string m_sSortColumn = "Score";      // Current sort column
    protected bool m_bSortAscending = false;       // Sort direction (false = descending, true = ascending)
    
    // Filter settings
    protected string m_sFilter = "";               // Text filter for player names
    protected int m_iTeamFilter = -1;              // Team filter (-1 = all teams)
    
    // Layout configuration
    protected ref array<ref STS_ScoreboardColumn> m_aColumns = new array<ref STS_ScoreboardColumn>();
    protected float m_fRowHeight = 30;             // Height of each player row
    protected int m_iMaxVisibleRows = 20;          // Maximum number of visible rows
    protected int m_iCurrentPage = 0;              // Current page for pagination
    
    // Update settings
    protected bool m_bNeedsRefresh = true;         // Whether the scoreboard needs a full refresh
    protected float m_fLastRefreshTime = 0;        // Last time the scoreboard was refreshed
    protected float m_fRefreshInterval = 2.0;      // How often to refresh data (in seconds)
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_ScoreboardComponent()
    {
        m_sName = "Scoreboard";
        
        // Define columns
        DefineColumns();
    }
    
    //------------------------------------------------------------------------------------------------
    // Define scoreboard columns
    protected void DefineColumns()
    {
        // Clear existing columns
        m_aColumns.Clear();
        
        // Add columns with localized headers
        m_aColumns.Insert(new STS_ScoreboardColumn("Rank", "STS_TEXT_RANK", 50, true));
        m_aColumns.Insert(new STS_ScoreboardColumn("PlayerName", "STS_TEXT_PLAYER_NAME", 200, true));
        m_aColumns.Insert(new STS_ScoreboardColumn("Kills", "STS_TEXT_KILLS", 80, true));
        m_aColumns.Insert(new STS_ScoreboardColumn("Deaths", "STS_TEXT_DEATHS", 80, true));
        m_aColumns.Insert(new STS_ScoreboardColumn("KDRatio", "STS_TEXT_KD_RATIO", 80, true));
        m_aColumns.Insert(new STS_ScoreboardColumn("Score", "STS_TEXT_SCORE", 80, true));
        m_aColumns.Insert(new STS_ScoreboardColumn("Headshots", "STS_TEXT_HEADSHOTS", 80, true));
        m_aColumns.Insert(new STS_ScoreboardColumn("Playtime", "STS_TEXT_PLAYTIME", 100, false));
        
        // Admin-only columns (only visible in admin mode)
        STS_ScoreboardColumn ipColumn = new STS_ScoreboardColumn("IP", "IP", 120, false);
        ipColumn.SetAdminOnly(true);
        m_aColumns.Insert(ipColumn);
        
        STS_ScoreboardColumn uidColumn = new STS_ScoreboardColumn("UID", "UID", 120, false);
        uidColumn.SetAdminOnly(true);
        m_aColumns.Insert(uidColumn);
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize the component
    override void Initialize()
    {
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_ScoreboardComponent", "Initialize");
            
        // Create or get layout
        CreateLayout();
        
        // Initially hide the scoreboard
        Hide();
        
        if (m_Logger)
            m_Logger.LogInfo("Scoreboard component initialized", "STS_ScoreboardComponent", "Initialize");
            
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_ScoreboardComponent", "Initialize", startTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // Create scoreboard layout
    protected void CreateLayout()
    {
        // Get game UI manager
        Widget root = GetGame().GetWorkspace().CreateWidgets("StatTracker/GUI/layouts/scoreboard.layout");
        if (!root)
        {
            if (m_Logger)
                m_Logger.LogError("Failed to create scoreboard layout", "STS_ScoreboardComponent", "CreateLayout");
            return;
        }
        
        m_wRoot = root;
        
        // Get widget references
        m_wScoreboardPanel = m_wRoot.FindAnyWidget("ScoreboardPanel");
        m_wHeaderRow = m_wRoot.FindAnyWidget("HeaderRow");
        m_wPlayerList = m_wRoot.FindAnyWidget("PlayerList");
        m_wFooter = m_wRoot.FindAnyWidget("Footer");
        
        // Set up header row with columns
        CreateHeaderRow();
        
        // Set up footer
        SetupFooter();
        
        // Add to active widgets
        m_UIManager.m_aActiveWidgets.Insert(m_wRoot);
    }
    
    //------------------------------------------------------------------------------------------------
    // Create header row with columns
    protected void CreateHeaderRow()
    {
        // Clear existing content
        m_wHeaderRow.ClearChildren();
        
        // Create header cells
        float xPos = 0;
        
        foreach (STS_ScoreboardColumn column : m_aColumns)
        {
            // Skip admin-only columns if not in admin mode
            if (column.IsAdminOnly() && !m_bAdminMode)
                continue;
                
            // Create header cell
            Widget headerCell = GetGame().GetWorkspace().CreateWidgets("StatTracker/GUI/layouts/scoreboard_header_cell.layout", m_wHeaderRow);
            
            // Position and size
            headerCell.SetPos(xPos, 0);
            headerCell.SetSize(column.GetWidth(), m_fRowHeight);
            
            // Set localized text
            TextWidget headerText = TextWidget.Cast(headerCell.FindAnyWidget("HeaderText"));
            if (headerText)
                headerText.SetText(GetLocalizedText(column.GetTextKey()));
                
            // Add sort indicator
            ImageWidget sortIcon = ImageWidget.Cast(headerCell.FindAnyWidget("SortIcon"));
            if (sortIcon)
            {
                if (m_sSortColumn == column.GetId())
                {
                    sortIcon.SetVisible(true);
                    sortIcon.LoadImageFile(m_bSortAscending ? "StatTracker/GUI/images/sort_up.edds" : "StatTracker/GUI/images/sort_down.edds");
                }
                else
                {
                    sortIcon.SetVisible(false);
                }
            }
            
            // Add click handler for sorting
            if (column.IsSortable())
            {
                ButtonWidget sortButton = ButtonWidget.Cast(headerCell.FindAnyWidget("SortButton"));
                if (sortButton)
                {
                    string columnId = column.GetId();
                    sortButton.SetData(columnId); // Store column ID
                    // sort handler will be handled in the input handler
                }
            }
            
            xPos += column.GetWidth();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Set up footer
    protected void SetupFooter()
    {
        // Set up pagination controls
        ButtonWidget prevButton = ButtonWidget.Cast(m_wFooter.FindAnyWidget("PrevButton"));
        if (prevButton)
        {
            prevButton.SetText(GetLocalizedText("STS_TEXT_PREVIOUS"));
            // handler will be in input handler
        }
        
        ButtonWidget nextButton = ButtonWidget.Cast(m_wFooter.FindAnyWidget("NextButton"));
        if (nextButton)
        {
            nextButton.SetText(GetLocalizedText("STS_TEXT_NEXT"));
            // handler will be in input handler
        }
        
        // Set up filter controls
        EditBoxWidget filterBox = EditBoxWidget.Cast(m_wFooter.FindAnyWidget("FilterBox"));
        if (filterBox)
        {
            filterBox.SetText(m_sFilter);
            // handler will be in input handler
        }
        
        // Set up team filter
        ButtonWidget teamFilterButton = ButtonWidget.Cast(m_wFooter.FindAnyWidget("TeamFilterButton"));
        if (teamFilterButton)
        {
            string text;
            if (m_iTeamFilter == -1)
                text = GetLocalizedText("STS_TEXT_ALL_TEAMS");
            else
                text = string.Format("%1: %2", GetLocalizedText("STS_TEXT_TEAM"), m_iTeamFilter.ToString());
                
            teamFilterButton.SetText(text);
            // handler will be in input handler
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Refresh player data
    override void RefreshPlayerData(string playerID = "")
    {
        // Mark for refresh
        m_bNeedsRefresh = true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Update the scoreboard
    override void Update()
    {
        // Skip if not visible
        if (!m_bVisible)
            return;
            
        // Check if we need to refresh
        float currentTime = GetGame().GetTime() / 1000;
        if (m_bNeedsRefresh || (currentTime - m_fLastRefreshTime) >= m_fRefreshInterval)
        {
            // Measure performance
            float startTime = 0;
            if (m_PerformanceMonitor)
                startTime = m_PerformanceMonitor.StartMeasurement("STS_ScoreboardComponent", "UpdateScoreboard");
                
            // Refresh data
            RefreshPlayerList();
            
            // Update timestamps
            m_fLastRefreshTime = currentTime;
            m_bNeedsRefresh = false;
            
            // End performance measurement
            if (m_PerformanceMonitor)
                m_PerformanceMonitor.EndMeasurement("STS_ScoreboardComponent", "UpdateScoreboard", startTime);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Refresh player list
    protected void RefreshPlayerList()
    {
        // Clear the player list
        m_wPlayerList.ClearChildren();
        
        // Get all player stats
        array<ref STS_PlayerStats> allStats = GetAllPlayerStats();
        
        // Sort player stats
        SortPlayerStats(allStats);
        
        // Filter player stats
        array<ref STS_PlayerStats> filteredStats = new array<ref STS_PlayerStats>();
        foreach (STS_PlayerStats stats : allStats)
        {
            if (ShouldIncludePlayer(stats))
                filteredStats.Insert(stats);
        }
        
        // Calculate pagination
        int totalPlayers = filteredStats.Count();
        int totalPages = Math.Ceil(totalPlayers / m_iMaxVisibleRows);
        if (m_iCurrentPage >= totalPages)
            m_iCurrentPage = Math.Max(0, totalPages - 1);
            
        // Update page count display
        TextWidget pageCountText = TextWidget.Cast(m_wFooter.FindAnyWidget("PageCount"));
        if (pageCountText)
            pageCountText.SetText(string.Format("%1 / %2", m_iCurrentPage + 1, Math.Max(1, totalPages)));
            
        // Calculate range to display
        int startIndex = m_iCurrentPage * m_iMaxVisibleRows;
        int endIndex = Math.Min(startIndex + m_iMaxVisibleRows, totalPlayers);
        
        // Create player rows
        for (int i = startIndex; i < endIndex; i++)
        {
            STS_PlayerStats stats = filteredStats[i];
            CreatePlayerRow(stats, i - startIndex, i + 1); // i+1 for rank number (1-based)
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Create a player row
    protected void CreatePlayerRow(STS_PlayerStats stats, int rowIndex, int rank)
    {
        // Create row widget
        Widget rowWidget = GetGame().GetWorkspace().CreateWidgets("StatTracker/GUI/layouts/scoreboard_player_row.layout", m_wPlayerList);
        
        // Position row
        rowWidget.SetPos(0, rowIndex * m_fRowHeight);
        
        // Set background color (alternate rows)
        Widget background = Widget.Cast(rowWidget.FindAnyWidget("Background"));
        if (background)
        {
            if (rowIndex % 2 == 0)
                background.SetColor(ARGB(200, 30, 30, 30)); // Dark gray
            else
                background.SetColor(ARGB(200, 40, 40, 40)); // Slightly lighter gray
        }
        
        // Highlight current player
        IEntity localPlayer = GetGame().GetPlayerController();
        if (localPlayer && stats.m_sPlayerID == localPlayer.GetID())
        {
            if (background)
                background.SetColor(ARGB(200, 60, 60, 80)); // Highlight color
        }
        
        // Fill in cells
        float xPos = 0;
        
        foreach (STS_ScoreboardColumn column : m_aColumns)
        {
            // Skip admin-only columns if not in admin mode
            if (column.IsAdminOnly() && !m_bAdminMode)
                continue;
                
            // Create cell widget
            Widget cellWidget = GetGame().GetWorkspace().CreateWidgets("StatTracker/GUI/layouts/scoreboard_cell.layout", rowWidget);
            
            // Position and size
            cellWidget.SetPos(xPos, 0);
            cellWidget.SetSize(column.GetWidth(), m_fRowHeight);
            
            // Set cell value
            TextWidget cellText = TextWidget.Cast(cellWidget.FindAnyWidget("CellText"));
            if (cellText)
                cellText.SetText(GetCellValue(stats, column.GetId(), rank));
                
            xPos += column.GetWidth();
        }
        
        // Store row reference for later
        //rowWidget.SetUserData(stats.m_sPlayerID); // Store player ID for selection
    }
    
    //------------------------------------------------------------------------------------------------
    // Get cell value based on column ID
    protected string GetCellValue(STS_PlayerStats stats, string columnId, int rank)
    {
        switch (columnId)
        {
            case "Rank":
                return rank.ToString();
                
            case "PlayerName":
                return stats.m_sPlayerName;
                
            case "Kills":
                return stats.m_iKills.ToString();
                
            case "Deaths":
                return stats.m_iDeaths.ToString();
                
            case "KDRatio":
                if (stats.m_iDeaths == 0)
                    return stats.m_iKills.ToString() + ".0";
                else
                    return (stats.m_iKills / (float)stats.m_iDeaths).ToFixed(2);
                
            case "Score":
                return stats.m_iScore.ToString();
                
            case "Headshots":
                return stats.m_iHeadshots.ToString();
                
            case "Playtime":
                return FormatPlaytime(stats.m_fPlaytime);
                
            case "IP":
                return stats.m_sIPAddress;
                
            case "UID":
                return stats.m_sPlayerUID;
        }
        
        return "";
    }
    
    //------------------------------------------------------------------------------------------------
    // Format playtime in seconds to a readable format
    protected string FormatPlaytime(float seconds)
    {
        int hours = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        
        if (hours > 0)
            return string.Format("%1h %2m", hours, minutes);
        else
            return string.Format("%1m", minutes);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all player stats
    protected array<ref STS_PlayerStats> GetAllPlayerStats()
    {
        // Create a placeholder array for testing
        array<ref STS_PlayerStats> result = new array<ref STS_PlayerStats>();
        
        // In a real implementation, this would fetch data from the stats manager
        // For now, we'll create dummy data
        
        /*
        // Example:
        STS_StatTrackingManagerComponent statsManager = GetStatTrackingManager();
        if (statsManager)
        {
            return statsManager.GetAllPlayerStats();
        }
        */
        
        // Dummy data for testing
        for (int i = 0; i < 30; i++)
        {
            STS_PlayerStats stats = new STS_PlayerStats();
            stats.m_sPlayerID = "ID" + i.ToString();
            stats.m_sPlayerUID = "UID" + i.ToString();
            stats.m_sPlayerName = "Player" + i.ToString();
            stats.m_sIPAddress = "192.168.1." + i.ToString();
            stats.m_iKills = Math.RandomInt(0, 50);
            stats.m_iDeaths = Math.RandomInt(1, 30);
            stats.m_iHeadshots = Math.RandomInt(0, stats.m_iKills);
            stats.m_iScore = stats.m_iKills * 100 - stats.m_iDeaths * 50 + Math.RandomInt(-100, 100);
            stats.m_fPlaytime = Math.RandomFloat(600, 7200);
            stats.m_iTeam = Math.RandomInt(1, 3);
            
            result.Insert(stats);
        }
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Sort player stats based on current sort settings
    protected void SortPlayerStats(array<ref STS_PlayerStats> stats)
    {
        // Define sorter
        STS_PlayerStatsSorter sorter = new STS_PlayerStatsSorter(m_sSortColumn, m_bSortAscending);
        
        // Sort the array
        stats.Sort(sorter);
    }
    
    //------------------------------------------------------------------------------------------------
    // Filter player stats based on current filter settings
    protected bool ShouldIncludePlayer(STS_PlayerStats stats)
    {
        // Apply team filter
        if (m_iTeamFilter != -1 && stats.m_iTeam != m_iTeamFilter)
            return false;
            
        // Apply text filter
        if (m_sFilter != "" && stats.m_sPlayerName.IndexOf(m_sFilter, 0, true) == -1)
            return false;
            
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle input events
    override bool HandleInput(UAInput input, int type)
    {
        // Skip if not visible
        if (!m_bVisible)
            return false;
            
        // Handle button clicks
        if (type == UIEvent.BUTTON_CLICK)
        {
            // Get the button
            ButtonWidget button = ButtonWidget.Cast(input.GetData());
            if (!button)
                return false;
                
            // Get parent widget
            Widget parent = button.GetParent();
            if (!parent)
                return false;
                
            // Handle sort buttons
            if (parent.GetName() == "HeaderCell")
            {
                string columnId = button.GetData();
                if (columnId != "")
                {
                    // Toggle sort direction if already sorting by this column
                    if (m_sSortColumn == columnId)
                        m_bSortAscending = !m_bSortAscending;
                    else
                    {
                        m_sSortColumn = columnId;
                        m_bSortAscending = false; // Default to descending
                    }
                    
                    // Refresh header row
                    CreateHeaderRow();
                    
                    // Mark for refresh
                    m_bNeedsRefresh = true;
                    
                    return true;
                }
            }
            
            // Handle pagination buttons
            if (parent.GetName() == "Footer")
            {
                if (button.GetName() == "PrevButton")
                {
                    m_iCurrentPage = Math.Max(0, m_iCurrentPage - 1);
                    m_bNeedsRefresh = true;
                    return true;
                }
                else if (button.GetName() == "NextButton")
                {
                    m_iCurrentPage++;
                    m_bNeedsRefresh = true;
                    return true;
                }
                else if (button.GetName() == "TeamFilterButton")
                {
                    // Cycle through team filters
                    m_iTeamFilter++;
                    if (m_iTeamFilter > 2) // Assume max 2 teams
                        m_iTeamFilter = -1;
                        
                    // Update button text
                    string text;
                    if (m_iTeamFilter == -1)
                        text = GetLocalizedText("STS_TEXT_ALL_TEAMS");
                    else
                        text = string.Format("%1: %2", GetLocalizedText("STS_TEXT_TEAM"), m_iTeamFilter.ToString());
                        
                    button.SetText(text);
                    
                    m_bNeedsRefresh = true;
                    return true;
                }
            }
        }
        
        // Handle filter box text changes
        if (type == UIEvent.CHANGE)
        {
            EditBoxWidget editBox = EditBoxWidget.Cast(input.GetData());
            if (editBox && editBox.GetName() == "FilterBox")
            {
                m_sFilter = editBox.GetText();
                m_bNeedsRefresh = true;
                return true;
            }
        }
        
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Show the scoreboard
    override void Show()
    {
        if (m_bVisible)
            return;
            
        super.Show();
        
        if (m_wRoot)
            m_wRoot.SetVisibility(true);
            
        // Force refresh
        m_bNeedsRefresh = true;
        Update();
    }
    
    //------------------------------------------------------------------------------------------------
    // Hide the scoreboard
    override void Hide()
    {
        if (!m_bVisible)
            return;
            
        super.Hide();
        
        if (m_wRoot)
            m_wRoot.SetVisibility(false);
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up resources
    override void CleanUp()
    {
        if (m_wRoot)
        {
            m_wRoot.ClearFlags(WidgetFlags.VISIBLE);
            m_wRoot = null;
        }
        
        m_wScoreboardPanel = null;
        m_wHeaderRow = null;
        m_wPlayerList = null;
        m_wFooter = null;
    }
}

//------------------------------------------------------------------------------------------------
// Scoreboard column definition
class STS_ScoreboardColumn
{
    protected string m_sId;           // Column ID (for data lookup)
    protected string m_sTextKey;      // Localization key for header
    protected float m_fWidth;         // Column width
    protected bool m_bSortable;       // Whether column can be sorted
    protected bool m_bAdminOnly;      // Whether column is only visible to admins
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_ScoreboardColumn(string id, string textKey, float width, bool sortable)
    {
        m_sId = id;
        m_sTextKey = textKey;
        m_fWidth = width;
        m_bSortable = sortable;
        m_bAdminOnly = false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Getters
    string GetId() { return m_sId; }
    string GetTextKey() { return m_sTextKey; }
    float GetWidth() { return m_fWidth; }
    bool IsSortable() { return m_bSortable; }
    bool IsAdminOnly() { return m_bAdminOnly; }
    
    //------------------------------------------------------------------------------------------------
    // Setters
    void SetAdminOnly(bool adminOnly) { m_bAdminOnly = adminOnly; }
}

//------------------------------------------------------------------------------------------------
// Player entry for scoreboard
class STS_ScoreboardPlayerEntry
{
    string m_sPlayerID;
    string m_sPlayerName;
    int m_iKills;
    int m_iDeaths;
    int m_iScore;
    float m_fKDRatio;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_ScoreboardPlayerEntry(string id, string name, int kills, int deaths, int score)
    {
        m_sPlayerID = id;
        m_sPlayerName = name;
        m_iKills = kills;
        m_iDeaths = deaths;
        m_iScore = score;
        
        if (deaths > 0)
            m_fKDRatio = kills / (float)deaths;
        else
            m_fKDRatio = kills;
    }
}

//------------------------------------------------------------------------------------------------
// Sorter for player stats
class STS_PlayerStatsSorter
{
    protected string m_sSortColumn;
    protected bool m_bAscending;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_PlayerStatsSorter(string sortColumn, bool ascending)
    {
        m_sSortColumn = sortColumn;
        m_bAscending = ascending;
    }
    
    //------------------------------------------------------------------------------------------------
    // Compare function for sorting
    int Compare(STS_PlayerStats x, STS_PlayerStats y)
    {
        int result = 0;
        
        // Compare based on column
        switch (m_sSortColumn)
        {
            case "PlayerName":
                result = x.m_sPlayerName.Compare(y.m_sPlayerName);
                break;
                
            case "Kills":
                result = x.m_iKills - y.m_iKills;
                break;
                
            case "Deaths":
                result = x.m_iDeaths - y.m_iDeaths;
                break;
                
            case "KDRatio":
                float xRatio = (x.m_iDeaths > 0) ? x.m_iKills / (float)x.m_iDeaths : x.m_iKills;
                float yRatio = (y.m_iDeaths > 0) ? y.m_iKills / (float)y.m_iDeaths : y.m_iKills;
                
                if (xRatio < yRatio) result = -1;
                else if (xRatio > yRatio) result = 1;
                else result = 0;
                break;
                
            case "Score":
                result = x.m_iScore - y.m_iScore;
                break;
                
            case "Headshots":
                result = x.m_iHeadshots - y.m_iHeadshots;
                break;
                
            case "Playtime":
                if (x.m_fPlaytime < y.m_fPlaytime) result = -1;
                else if (x.m_fPlaytime > y.m_fPlaytime) result = 1;
                else result = 0;
                break;
                
            default:
                // Default to score
                result = x.m_iScore - y.m_iScore;
                break;
        }
        
        // Apply sort direction
        if (!m_bAscending)
            result = -result;
            
        return result;
    }
}

//------------------------------------------------------------------------------------------------
// Placeholder player stats class (would normally be defined elsewhere)
class STS_PlayerStats
{
    string m_sPlayerID;
    string m_sPlayerUID;
    string m_sPlayerName;
    string m_sIPAddress;
    int m_iKills;
    int m_iDeaths;
    int m_iHeadshots;
    int m_iScore;
    float m_fPlaytime;
    int m_iTeam;
    
    void STS_PlayerStats()
    {
        m_sPlayerID = "";
        m_sPlayerUID = "";
        m_sPlayerName = "";
        m_sIPAddress = "";
        m_iKills = 0;
        m_iDeaths = 0;
        m_iHeadshots = 0;
        m_iScore = 0;
        m_fPlaytime = 0;
        m_iTeam = 0;
    }
} 