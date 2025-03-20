// STS_VoteKickSystem.c
// System to track vote kicks and prevent abuse

class VoteKickEntry
{
    int m_iInitiatorID;
    string m_sInitiatorName;
    int m_iTargetID;
    string m_sTargetName;
    string m_sReason;
    float m_fStartTime;
    float m_fEndTime;
    bool m_bApproved;
    int m_iVotesFor;
    int m_iVotesAgainst;
    ref array<int> m_aVotersFor = new array<int>();
    ref array<int> m_aVotersAgainst = new array<int>();
    
    void VoteKickEntry(int initiatorID, string initiatorName, int targetID, string targetName, string reason)
    {
        m_iInitiatorID = initiatorID;
        m_sInitiatorName = initiatorName;
        m_iTargetID = targetID;
        m_sTargetName = targetName;
        m_sReason = reason;
        m_fStartTime = System.GetTickCount() / 1000.0;
        m_fEndTime = 0;
        m_bApproved = false;
        m_iVotesFor = 0;
        m_iVotesAgainst = 0;
    }
    
    // Convert to JSON
    string ToJSON()
    {
        string json = "{";
        json += "\"initiatorID\":" + m_iInitiatorID.ToString() + ",";
        json += "\"initiatorName\":\"" + m_sInitiatorName + "\",";
        json += "\"targetID\":" + m_iTargetID.ToString() + ",";
        json += "\"targetName\":\"" + m_sTargetName + "\",";
        json += "\"reason\":\"" + m_sReason + "\",";
        json += "\"startTime\":" + m_fStartTime.ToString() + ",";
        json += "\"endTime\":" + m_fEndTime.ToString() + ",";
        json += "\"approved\":" + m_bApproved.ToString() + ",";
        json += "\"votesFor\":" + m_iVotesFor.ToString() + ",";
        json += "\"votesAgainst\":" + m_iVotesAgainst.ToString() + ",";
        
        // Voters for
        json += "\"votersFor\":[";
        for (int i = 0; i < m_aVotersFor.Count(); i++)
        {
            json += m_aVotersFor[i].ToString();
            if (i < m_aVotersFor.Count() - 1)
                json += ",";
        }
        json += "],";
        
        // Voters against
        json += "\"votersAgainst\":[";
        for (int i = 0; i < m_aVotersAgainst.Count(); i++)
        {
            json += m_aVotersAgainst[i].ToString();
            if (i < m_aVotersAgainst.Count() - 1)
                json += ",";
        }
        json += "]";
        
        json += "}";
        return json;
    }
}

class STS_VoteKickSystem
{
    // Singleton instance
    private static ref STS_VoteKickSystem s_Instance;
    
    // Active vote kicks
    protected ref array<ref VoteKickEntry> m_aActiveVoteKicks = new array<ref VoteKickEntry>();
    
    // Historical vote kicks
    protected ref array<ref VoteKickEntry> m_aHistoricalVoteKicks = new array<ref VoteKickEntry>();
    
    // File path for saving vote kick history
    protected const string VOTEKICK_HISTORY_PATH = "$profile:StatTracker/vote_kick_history.json";
    
    // Anti-abuse parameters
    protected const int MAX_VOTEKICKS_PER_PLAYER = 3; // Per session
    protected const float VOTEKICK_COOLDOWN = 300.0; // 5 minutes between vote kicks
    protected map<int, int> m_mPlayerVoteKickCount = new map<int, int>(); // Track how many vote kicks each player has initiated
    protected map<int, float> m_mPlayerLastVoteKick = new map<int, float>(); // Track when each player last initiated a vote kick
    
    // Reference to logging system
    protected ref STS_LoggingSystem m_Logger;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_VoteKickSystem()
    {
        Print("[StatTracker] Initializing Vote Kick System");
        
        // Get logger instance
        m_Logger = STS_LoggingSystem.GetInstance();
        
        // Load vote kick history
        LoadVoteKickHistory();
        
        // Set up event handlers
        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (gameMode)
        {
            // Player connected event
            gameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);
            
            // Vote kick events
            SCR_VotingManagerComponent votingManager = SCR_VotingManagerComponent.Cast(GetGame().GetVotingManager());
            if (!votingManager)
            {
                // Fallback: try to find it as a world component
                votingManager = SCR_VotingManagerComponent.Cast(GetGame().GetWorld().FindComponent(SCR_VotingManagerComponent));
            }
            
            if (votingManager)
            {
                votingManager.GetOnVoteStartedInvoker().Insert(OnVoteStarted);
                votingManager.GetOnVoteEndedInvoker().Insert(OnVoteEnded);
                votingManager.GetOnVoteCastInvoker().Insert(OnVoteCast);
                m_Logger.LogInfo("Successfully hooked into voting system");
            }
            else
            {
                m_Logger.LogError("Failed to find voting manager component");
            }
        }
        
        m_Logger.LogInfo("Vote Kick System initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_VoteKickSystem GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_VoteKickSystem();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a player connects
    protected void OnPlayerConnected(int playerID)
    {
        // Reset vote kick count for new players
        if (!m_mPlayerVoteKickCount.Contains(playerID))
            m_mPlayerVoteKickCount.Insert(playerID, 0);
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a vote is started
    protected void OnVoteStarted(SCR_VotingManagerComponent manager, int voteID, int initiatorID, int targetID, string reason)
    {
        // Only handle vote kicks
        if (manager.GetVoteType(voteID) != EVoteType.KICK)
            return;
            
        // Get player names
        string initiatorName = GetPlayerName(initiatorID);
        string targetName = GetPlayerName(targetID);
        
        // Check if the player is on cooldown
        if (m_mPlayerLastVoteKick.Contains(initiatorID))
        {
            float currentTime = System.GetTickCount() / 1000.0;
            float lastVoteKickTime = m_mPlayerLastVoteKick[initiatorID];
            
            if (currentTime - lastVoteKickTime < VOTEKICK_COOLDOWN)
            {
                float remainingCooldown = VOTEKICK_COOLDOWN - (currentTime - lastVoteKickTime);
                m_Logger.LogWarning(string.Format("Player %1 (ID: %2) tried to initiate a vote kick but is on cooldown for %3 more seconds", 
                    initiatorName, initiatorID, remainingCooldown.ToString()));
                return;
            }
        }
        
        // Check if the player has reached the limit
        if (m_mPlayerVoteKickCount.Contains(initiatorID) && m_mPlayerVoteKickCount[initiatorID] >= MAX_VOTEKICKS_PER_PLAYER)
        {
            m_Logger.LogWarning(string.Format("Player %1 (ID: %2) tried to initiate a vote kick but has reached the limit of %3 vote kicks per session", 
                initiatorName, initiatorID, MAX_VOTEKICKS_PER_PLAYER));
            return;
        }
        
        // Record the vote kick
        VoteKickEntry entry = new VoteKickEntry(initiatorID, initiatorName, targetID, targetName, reason);
        m_aActiveVoteKicks.Insert(entry);
        
        // Update player's vote kick count and time
        if (m_mPlayerVoteKickCount.Contains(initiatorID))
            m_mPlayerVoteKickCount[initiatorID]++;
        else
            m_mPlayerVoteKickCount.Insert(initiatorID, 1);
            
        m_mPlayerLastVoteKick.Set(initiatorID, System.GetTickCount() / 1000.0);
        
        // Log the vote kick
        m_Logger.LogVoteKick(initiatorName, initiatorID, targetName, targetID, reason);
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a vote is cast
    protected void OnVoteCast(SCR_VotingManagerComponent manager, int voteID, int voterID, bool voteInFavor)
    {
        // Only handle vote kicks
        if (manager.GetVoteType(voteID) != EVoteType.KICK)
            return;
            
        // Find the active vote kick
        int targetID = manager.GetVoteTarget(voteID);
        VoteKickEntry entry = FindActiveVoteKick(targetID);
        if (!entry)
            return;
            
        // Record the vote
        if (voteInFavor)
        {
            entry.m_iVotesFor++;
            entry.m_aVotersFor.Insert(voterID);
        }
        else
        {
            entry.m_iVotesAgainst++;
            entry.m_aVotersAgainst.Insert(voterID);
        }
        
        // Log the vote
        string voterName = GetPlayerName(voterID);
        m_Logger.LogVoteKickVote(voterName, voterID, entry.m_sTargetName, entry.m_iTargetID, voteInFavor);
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a vote ends
    protected void OnVoteEnded(SCR_VotingManagerComponent manager, int voteID, EVoteResult result)
    {
        // Only handle vote kicks
        if (manager.GetVoteType(voteID) != EVoteType.KICK)
            return;
            
        // Find the active vote kick
        int targetID = manager.GetVoteTarget(voteID);
        VoteKickEntry entry = FindActiveVoteKick(targetID);
        if (!entry)
            return;
            
        // Update the entry
        entry.m_fEndTime = System.GetTickCount() / 1000.0;
        entry.m_bApproved = (result == EVoteResult.ACCEPTED);
        
        // Move to historical records
        m_aHistoricalVoteKicks.Insert(entry);
        m_aActiveVoteKicks.RemoveItem(entry);
        
        // Log the result
        m_Logger.LogVoteKickResult(entry.m_sTargetName, entry.m_iTargetID, entry.m_bApproved, 
            entry.m_iVotesFor, entry.m_iVotesAgainst);
        
        // Save vote kick history
        SaveVoteKickHistory();
    }
    
    //------------------------------------------------------------------------------------------------
    // Find an active vote kick for a target
    protected VoteKickEntry FindActiveVoteKick(int targetID)
    {
        foreach (VoteKickEntry entry : m_aActiveVoteKicks)
        {
            if (entry.m_iTargetID == targetID)
                return entry;
        }
        
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a player's name from their ID
    protected string GetPlayerName(int playerID)
    {
        PlayerManager playerManager = GetGame().GetPlayerManager();
        if (!playerManager)
            return "Unknown";
            
        IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerID);
        if (!playerEntity)
            return "Unknown";
            
        return playerEntity.GetName();
    }
    
    //------------------------------------------------------------------------------------------------
    // Save vote kick history to JSON
    protected void SaveVoteKickHistory()
    {
        string json = "[";
        
        // Add all historical vote kicks
        for (int i = 0; i < m_aHistoricalVoteKicks.Count(); i++)
        {
            json += m_aHistoricalVoteKicks[i].ToJSON();
            
            if (i < m_aHistoricalVoteKicks.Count() - 1)
                json += ",";
        }
        
        json += "]";
        
        // Create directory if it doesn't exist
        string directory = "$profile:StatTracker";
        if (!FileIO.FileExists(directory))
        {
            if (!FileIO.MakeDirectory(directory))
            {
                m_Logger.LogError("Failed to create directory for vote kick history: " + directory);
                return;
            }
        }
        
        // Write to file
        FileHandle file = FileIO.OpenFile(VOTEKICK_HISTORY_PATH, FileMode.WRITE);
        if (file)
        {
            if (file.WriteLine(json))
            {
                file.Close();
                m_Logger.LogDebug("Vote kick history saved to " + VOTEKICK_HISTORY_PATH);
            }
            else
            {
                m_Logger.LogError("Failed to write to vote kick history file: " + VOTEKICK_HISTORY_PATH);
                file.Close();
            }
        }
        else
        {
            m_Logger.LogError("Failed to open vote kick history file for writing: " + VOTEKICK_HISTORY_PATH);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load vote kick history from JSON
    protected void LoadVoteKickHistory()
    {
        // Check if file exists
        if (!FileIO.FileExists(VOTEKICK_HISTORY_PATH))
        {
            m_Logger.LogInfo("No vote kick history file found at " + VOTEKICK_HISTORY_PATH);
            return;
        }
        
        // Read file content
        FileHandle file = FileIO.OpenFile(VOTEKICK_HISTORY_PATH, FileMode.READ);
        if (!file)
        {
            m_Logger.LogError("Failed to open vote kick history file at " + VOTEKICK_HISTORY_PATH);
            return;
        }
        
        string json = "";
        string line;
        while (file.ReadLine(line) >= 0)
        {
            json += line;
        }
        file.Close();
        
        // Parse JSON (simplified version, a proper implementation would use a JSON parser)
        m_Logger.LogInfo("Vote kick history loaded from " + VOTEKICK_HISTORY_PATH);
        
        // Note: A real implementation would parse the JSON and populate m_aHistoricalVoteKicks
    }
    
    //------------------------------------------------------------------------------------------------
    // Get vote kick statistics for a player
    void GetPlayerVoteKickStats(int playerID, out int initiatedCount, out int targetedCount, out int approvedCount)
    {
        initiatedCount = 0;
        targetedCount = 0;
        approvedCount = 0;
        
        foreach (VoteKickEntry entry : m_aHistoricalVoteKicks)
        {
            if (entry.m_iInitiatorID == playerID)
                initiatedCount++;
                
            if (entry.m_iTargetID == playerID)
            {
                targetedCount++;
                if (entry.m_bApproved)
                    approvedCount++;
            }
        }
    }
} 