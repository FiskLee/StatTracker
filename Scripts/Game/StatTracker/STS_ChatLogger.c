// STS_ChatLogger.c
// System to log all in-game chat messages

class STS_ChatLogger
{
    // Singleton instance
    private static ref STS_ChatLogger s_Instance;
    
    // Reference to logging system
    protected ref STS_LoggingSystem m_Logger;
    
    // Chat storage for current session
    protected ref array<string> m_aChatLog = new array<string>();
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_ChatLogger()
    {
        Print("[StatTracker] Initializing Chat Logger");
        
        // Get logger instance
        m_Logger = STS_LoggingSystem.GetInstance();
        
        // Hook into chat events
        SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetChatComponent());
        if (!chatComponent)
        {
            // Fallback: try to find it as a world component
            chatComponent = SCR_ChatComponent.Cast(GetGame().GetWorld().FindComponent(SCR_ChatComponent));
        }
        
        if (chatComponent)
        {
            chatComponent.GetOnMessageReceivedInvoker().Insert(OnChatMessageReceived);
            m_Logger.LogInfo("Successfully hooked into chat component");
        }
        else
        {
            m_Logger.LogError("Failed to find chat component - chat logging will not work");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_ChatLogger GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_ChatLogger();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a chat message is received
    protected void OnChatMessageReceived(ChatMessageInfo messageInfo)
    {
        // Ignore empty or system messages
        if (!messageInfo || !messageInfo.m_Sender || messageInfo.m_sContent.IsEmpty())
            return;
            
        // Get player information
        string playerName = messageInfo.m_Sender.GetName();
        int playerID = messageInfo.m_Sender.GetPlayerId();
        string message = messageInfo.m_sContent;
        
        // Determine message type
        string messageType = "ALL";
        if (messageInfo.m_eChannelType == ChatChannelType.FACTION)
            messageType = "TEAM";
        else if (messageInfo.m_eChannelType == ChatChannelType.GROUP)
            messageType = "GROUP";
        else if (messageInfo.m_eChannelType == ChatChannelType.VEHICLE)
            messageType = "VEHICLE";
        else if (messageInfo.m_eChannelType == ChatChannelType.DIRECT)
            messageType = "DIRECT";
            
        // Format full message for storage
        string formattedMessage = string.Format("[%1] [%2] %3 (ID: %4): %5",
            GetTimestamp(),
            messageType,
            playerName,
            playerID,
            message);
            
        // Add to in-memory log
        m_aChatLog.Insert(formattedMessage);
        
        // Log the chat message
        m_Logger.LogChat(playerName, playerID, "[" + messageType + "] " + message);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current timestamp
    protected string GetTimestamp()
    {
        int year, month, day, hour, minute, second;
        System.GetYearMonthDay(year, month, day);
        System.GetHourMinuteSecond(hour, minute, second);
        
        return string.Format("%1-%2-%3 %4:%5:%6", 
            year, month.ToString(2, "0"), day.ToString(2, "0"), 
            hour.ToString(2, "0"), minute.ToString(2, "0"), second.ToString(2, "0"));
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the last N chat messages
    array<string> GetRecentChatMessages(int count = 10)
    {
        array<string> recentMessages = new array<string>();
        
        int startIndex = Math.Max(0, m_aChatLog.Count() - count);
        for (int i = startIndex; i < m_aChatLog.Count(); i++)
        {
            recentMessages.Insert(m_aChatLog[i]);
        }
        
        return recentMessages;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all chat messages from a specific player
    array<string> GetPlayerChatMessages(int playerID)
    {
        array<string> playerMessages = new array<string>();
        
        foreach (string message : m_aChatLog)
        {
            // Very simple parsing - a proper implementation would use regex or better parsing
            if (message.Contains("ID: " + playerID.ToString()))
            {
                playerMessages.Insert(message);
            }
        }
        
        return playerMessages;
    }
} 