// STS_NotificationManager.c
// Handles notifications and messages for the StatTracker system

class STS_NotificationManager
{
    // Singleton instance
    private static ref STS_NotificationManager s_Instance;
    
    // UI references
    protected ref Widget m_NotificationRoot;
    protected ref TextWidget m_NotificationText;
    protected ref Widget m_AnnouncementRoot;
    protected ref TextWidget m_AnnouncementText;
    
    // Message queue
    protected ref array<ref STS_Message> m_MessageQueue;
    
    // Current message being displayed
    protected ref STS_Message m_CurrentMessage;
    
    // Announcement timer
    protected ref Timer m_AnnouncementTimer;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_NotificationManager()
    {
        m_MessageQueue = new array<ref STS_Message>();
        
        // Initialize UI when game is loaded
        GetGame().GetCallqueue().CallLater(InitializeUI, 1000, false);
        
        // Register RPC handlers
        GetGame().GetRPCManager().AddRPC("STS_NotificationManager", "OnAdminMessage", this, SingleplayerExecutionType.Client);
        GetGame().GetRPCManager().AddRPC("STS_NotificationManager", "OnAdminAnnouncement", this, SingleplayerExecutionType.Client);
        
        Print("[StatTracker] Notification Manager initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_NotificationManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_NotificationManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize UI elements
    protected void InitializeUI()
    {
        // Create notification widget
        if (!m_NotificationRoot)
        {
            m_NotificationRoot = GetGame().GetWorkspace().CreateWidgets("StatTracker/GUI/layouts/admin_message.layout");
            m_NotificationText = TextWidget.Cast(m_NotificationRoot.FindAnyWidget("NotificationText"));
            m_NotificationRoot.Show(false);
        }
        
        // Create announcement widget
        if (!m_AnnouncementRoot)
        {
            m_AnnouncementRoot = GetGame().GetWorkspace().CreateWidgets("StatTracker/GUI/layouts/admin_announcement.layout");
            m_AnnouncementText = TextWidget.Cast(m_AnnouncementRoot.FindAnyWidget("AnnouncementText"));
            m_AnnouncementRoot.Show(false);
        }
        
        // Start message processor
        GetGame().GetCallqueue().CallLater(ProcessMessageQueue, 500, true);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle admin message RPC
    void OnAdminMessage(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;
        
        // Read message
        Param1<string> data;
        if (!ctx.Read(data))
            return;
        
        string message = data.param1;
        
        // Queue the message
        STS_Message msg = new STS_Message(message, false);
        QueueMessage(msg);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle admin announcement RPC
    void OnAdminAnnouncement(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;
        
        // Read message
        Param1<string> data;
        if (!ctx.Read(data))
            return;
        
        string message = data.param1;
        
        // Show announcement immediately (no queue)
        ShowAnnouncement(message);
    }
    
    //------------------------------------------------------------------------------------------------
    // Queue a message for display
    void QueueMessage(STS_Message message)
    {
        m_MessageQueue.Insert(message);
    }
    
    //------------------------------------------------------------------------------------------------
    // Process the message queue
    protected void ProcessMessageQueue()
    {
        // If we're currently displaying a message, don't do anything
        if (m_CurrentMessage && m_NotificationRoot && m_NotificationRoot.IsVisible())
            return;
        
        // If there are no messages in the queue, we're done
        if (m_MessageQueue.Count() == 0)
            return;
        
        // Get the next message
        m_CurrentMessage = m_MessageQueue[0];
        m_MessageQueue.RemoveOrdered(0);
        
        // Display the message
        if (m_CurrentMessage.m_bIsAnnouncement)
            ShowAnnouncement(m_CurrentMessage.m_sMessage);
        else
            ShowNotification(m_CurrentMessage.m_sMessage);
    }
    
    //------------------------------------------------------------------------------------------------
    // Show a notification message
    protected void ShowNotification(string message)
    {
        if (!m_NotificationRoot || !m_NotificationText)
            return;
        
        // Set message text
        m_NotificationText.SetText(message);
        
        // Show notification
        m_NotificationRoot.Show(true);
        
        // Set timer to hide notification
        GetGame().GetCallqueue().CallLater(HideNotification, 5000, false);
    }
    
    //------------------------------------------------------------------------------------------------
    // Hide the notification message
    protected void HideNotification()
    {
        if (m_NotificationRoot)
            m_NotificationRoot.Show(false);
        
        m_CurrentMessage = null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Show an announcement popup
    protected void ShowAnnouncement(string message)
    {
        if (!m_AnnouncementRoot || !m_AnnouncementText)
            return;
        
        // Set message text
        m_AnnouncementText.SetText(message);
        
        // Show announcement
        m_AnnouncementRoot.Show(true);
        
        // Set timer to hide announcement
        if (m_AnnouncementTimer)
        {
            m_AnnouncementTimer.Stop();
        }
        else
        {
            m_AnnouncementTimer = new Timer();
        }
        
        m_AnnouncementTimer.Run(10000, this, "HideAnnouncement");
    }
    
    //------------------------------------------------------------------------------------------------
    // Hide the announcement popup
    protected void HideAnnouncement()
    {
        if (m_AnnouncementRoot)
            m_AnnouncementRoot.Show(false);
    }
}

//------------------------------------------------------------------------------------------------
// Message class for queuing notifications
class STS_Message
{
    string m_sMessage;
    bool m_bIsAnnouncement;
    
    void STS_Message(string message, bool isAnnouncement)
    {
        m_sMessage = message;
        m_bIsAnnouncement = isAnnouncement;
    }
}