// STS_UIManager.c
// Manages UI components and interactions for the StatTracker system

class STS_UIManager
{
    // Singleton instance
    private static ref STS_UIManager s_Instance;
    
    // Configuration
    protected bool m_bUIEnabled = true;                 // Whether UI components are enabled
    protected float m_fUpdateInterval = 1.0;            // How often to update UI elements (in seconds)
    protected float m_fLastUpdateTime = 0;              // Last time UI was updated
    
    // UI components and layouts
    protected ref array<ref STS_UIComponent> m_aUIComponents = new array<ref STS_UIComponent>();
    protected ref array<Widget> m_aActiveWidgets = new array<Widget>();
    
    // UI state
    protected bool m_bScoreboardVisible = false;
    protected bool m_bStatsMenuVisible = false;
    protected int m_iSelectedPlayerIndex = -1;
    protected bool m_bAdminMode = false;
    
    // Dependencies
    protected STS_Config m_Config;
    protected STS_LoggingSystem m_Logger;
    protected STS_LocalizationManager m_Localization;
    protected STS_PerformanceMonitor m_PerformanceMonitor;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_UIManager()
    {
        m_Config = STS_Config.GetInstance();
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Localization = STS_LocalizationManager.GetInstance();
        m_PerformanceMonitor = STS_PerformanceMonitor.GetInstance();
        
        // Register for config changes
        if (m_Config)
            m_Config.RegisterConfigChangeCallback(OnConfigChanged);
            
        // Set up UI update timer
        GetGame().GetCallqueue().CallLater(UpdateUI, m_fUpdateInterval * 1000, true);
        
        if (m_Logger)
            m_Logger.LogInfo("UI Manager initialized", "STS_UIManager", "Constructor");
        else
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
    // Initialize UI system
    void Initialize()
    {
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_UIManager", "Initialize");
            
        // Register UI components
        RegisterUIComponents();
        
        // Initialize UI components
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            component.Initialize();
        }
        
        if (m_Logger)
            m_Logger.LogInfo("UI system initialized", "STS_UIManager", "Initialize");
            
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_UIManager", "Initialize", startTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // Register UI components
    protected void RegisterUIComponents()
    {
        // Register all UI components here
        // Example:
        // m_aUIComponents.Insert(new STS_ScoreboardComponent());
        // m_aUIComponents.Insert(new STS_StatsMenuComponent());
        // etc.
    }
    
    //------------------------------------------------------------------------------------------------
    // Update UI elements periodically
    protected void UpdateUI()
    {
        if (!m_bUIEnabled)
            return;
            
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_UIManager", "UpdateUI");
            
        m_fLastUpdateTime = GetGame().GetTime();
        
        // Update each UI component
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            if (component.IsVisible())
                component.Update();
        }
        
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_UIManager", "UpdateUI", startTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle input events
    bool HandleInput(UAInput input, int type)
    {
        if (!m_bUIEnabled)
            return false;
            
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_UIManager", "HandleInput");
            
        bool handled = false;
        
        // Allow components to handle input
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            if (component.IsVisible() && component.HandleInput(input, type))
            {
                handled = true;
                break;
            }
        }
        
        // Toggle scoreboard visibility on key press
        if (type == UIEvent.KEY_DOWN)
        {
            UAInputName inputName = input.InputName();
            if (inputName == "STS_ToggleScoreboard")
            {
                ToggleScoreboard();
                handled = true;
            }
            else if (inputName == "STS_ToggleStatsMenu")
            {
                ToggleStatsMenu();
                handled = true;
            }
        }
        
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_UIManager", "HandleInput", startTime);
            
        return handled;
    }
    
    //------------------------------------------------------------------------------------------------
    // Show the scoreboard
    void ShowScoreboard()
    {
        if (m_bScoreboardVisible)
            return;
            
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_UIManager", "ShowScoreboard");
            
        m_bScoreboardVisible = true;
        
        // Find scoreboard component and show it
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            if (component.GetName() == "Scoreboard")
            {
                component.Show();
                break;
            }
        }
        
        if (m_Logger)
            m_Logger.LogDebug("Scoreboard shown", "STS_UIManager", "ShowScoreboard");
            
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_UIManager", "ShowScoreboard", startTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // Hide the scoreboard
    void HideScoreboard()
    {
        if (!m_bScoreboardVisible)
            return;
            
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_UIManager", "HideScoreboard");
            
        m_bScoreboardVisible = false;
        
        // Find scoreboard component and hide it
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            if (component.GetName() == "Scoreboard")
            {
                component.Hide();
                break;
            }
        }
        
        if (m_Logger)
            m_Logger.LogDebug("Scoreboard hidden", "STS_UIManager", "HideScoreboard");
            
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_UIManager", "HideScoreboard", startTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // Toggle scoreboard visibility
    void ToggleScoreboard()
    {
        if (m_bScoreboardVisible)
            HideScoreboard();
        else
            ShowScoreboard();
    }
    
    //------------------------------------------------------------------------------------------------
    // Show the stats menu
    void ShowStatsMenu()
    {
        if (m_bStatsMenuVisible)
            return;
            
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_UIManager", "ShowStatsMenu");
            
        m_bStatsMenuVisible = true;
        
        // Find stats menu component and show it
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            if (component.GetName() == "StatsMenu")
            {
                component.Show();
                break;
            }
        }
        
        if (m_Logger)
            m_Logger.LogDebug("Stats menu shown", "STS_UIManager", "ShowStatsMenu");
            
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_UIManager", "ShowStatsMenu", startTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // Hide the stats menu
    void HideStatsMenu()
    {
        if (!m_bStatsMenuVisible)
            return;
            
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_UIManager", "HideStatsMenu");
            
        m_bStatsMenuVisible = false;
        
        // Find stats menu component and hide it
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            if (component.GetName() == "StatsMenu")
            {
                component.Hide();
                break;
            }
        }
        
        if (m_Logger)
            m_Logger.LogDebug("Stats menu hidden", "STS_UIManager", "HideStatsMenu");
            
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_UIManager", "HideStatsMenu", startTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // Toggle stats menu visibility
    void ToggleStatsMenu()
    {
        if (m_bStatsMenuVisible)
            HideStatsMenu();
        else
            ShowStatsMenu();
    }
    
    //------------------------------------------------------------------------------------------------
    // Get localized text for UI elements
    string GetLocalizedText(string key, array<string> params = null)
    {
        if (m_Localization)
            return m_Localization.GetLocalizedString(key, params);
            
        return key; // Fallback if localization system not available
    }
    
    //------------------------------------------------------------------------------------------------
    // Show notification to player
    void ShowNotification(string message, float duration = 5.0, int notificationType = 0)
    {
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_UIManager", "ShowNotification");
            
        // Find notification component and show message
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            if (component.GetName() == "Notifications")
            {
                STS_NotificationComponent notificationComponent = STS_NotificationComponent.Cast(component);
                if (notificationComponent)
                    notificationComponent.AddNotification(message, duration, notificationType);
                break;
            }
        }
        
        if (m_Logger)
            m_Logger.LogDebug("Notification shown: " + message, "STS_UIManager", "ShowNotification");
            
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_UIManager", "ShowNotification", startTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // Show a player achievement notification
    void ShowAchievementNotification(string achievementName, string description)
    {
        // Create a formatted message using localization
        array<string> params = new array<string>();
        params.Insert(achievementName);
        params.Insert(description);
        
        string message = GetLocalizedText("STS_TEXT_ACHIEVEMENT_UNLOCKED", params);
        
        // Show the notification with achievement type (1)
        ShowNotification(message, 10.0, 1);
    }
    
    //------------------------------------------------------------------------------------------------
    // Update player stats display for a specific player
    void UpdatePlayerStatsDisplay(string playerID)
    {
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_UIManager", "UpdatePlayerStatsDisplay");
            
        // Find stats components and update them
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            if (component.GetName() == "PlayerStats" || component.GetName() == "Scoreboard")
            {
                STS_StatsDisplayComponent statsComponent = STS_StatsDisplayComponent.Cast(component);
                if (statsComponent)
                    statsComponent.RefreshPlayerData(playerID);
            }
        }
        
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_UIManager", "UpdatePlayerStatsDisplay", startTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // Set admin mode for UI
    void SetAdminMode(bool enabled)
    {
        m_bAdminMode = enabled;
        
        // Update all components with admin mode status
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            component.SetAdminMode(enabled);
        }
        
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Admin mode %1", enabled ? "enabled" : "disabled"), "STS_UIManager", "SetAdminMode");
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle config changes
    protected void OnConfigChanged(map<string, string> changedValues)
    {
        // Process any config changes relevant to UI
        if (changedValues.Contains("UIEnabled"))
        {
            m_bUIEnabled = changedValues.Get("UIEnabled").ToInt() != 0;
        }
        
        if (changedValues.Contains("UIUpdateInterval"))
        {
            m_fUpdateInterval = changedValues.Get("UIUpdateInterval").ToFloat();
            
            // Update the timer
            GetGame().GetCallqueue().Remove(UpdateUI);
            GetGame().GetCallqueue().CallLater(UpdateUI, m_fUpdateInterval * 1000, true);
        }
        
        // Notify components of config changes
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            component.OnConfigChanged(changedValues);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up UI resources
    void CleanUp()
    {
        // Measure performance
        float startTime = 0;
        if (m_PerformanceMonitor)
            startTime = m_PerformanceMonitor.StartMeasurement("STS_UIManager", "CleanUp");
            
        // Clean up all active widgets
        foreach (Widget widget : m_aActiveWidgets)
        {
            if (widget)
                widget.ClearFlags(WidgetFlags.VISIBLE);
        }
        
        // Clean up components
        foreach (STS_UIComponent component : m_aUIComponents)
        {
            component.CleanUp();
        }
        
        m_aActiveWidgets.Clear();
        
        if (m_Logger)
            m_Logger.LogInfo("UI resources cleaned up", "STS_UIManager", "CleanUp");
            
        // End performance measurement
        if (m_PerformanceMonitor)
            m_PerformanceMonitor.EndMeasurement("STS_UIManager", "CleanUp", startTime);
    }
}

//------------------------------------------------------------------------------------------------
// Base class for UI components
class STS_UIComponent
{
    // Component name
    protected string m_sName;
    
    // Visibility
    protected bool m_bVisible = false;
    
    // Admin mode
    protected bool m_bAdminMode = false;
    
    // Dependencies
    protected STS_UIManager m_UIManager;
    protected STS_Config m_Config;
    protected STS_LoggingSystem m_Logger;
    protected STS_LocalizationManager m_Localization;
    protected STS_PerformanceMonitor m_PerformanceMonitor;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_UIComponent()
    {
        m_UIManager = STS_UIManager.GetInstance();
        m_Config = STS_Config.GetInstance();
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Localization = STS_LocalizationManager.GetInstance();
        m_PerformanceMonitor = STS_PerformanceMonitor.GetInstance();
        
        m_sName = "BaseComponent";
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize the component
    void Initialize()
    {
        // Override in derived classes
    }
    
    //------------------------------------------------------------------------------------------------
    // Update the component
    void Update()
    {
        // Override in derived classes
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle input events
    bool HandleInput(UAInput input, int type)
    {
        // Override in derived classes
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Show the component
    void Show()
    {
        m_bVisible = true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Hide the component
    void Hide()
    {
        m_bVisible = false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if component is visible
    bool IsVisible()
    {
        return m_bVisible;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get component name
    string GetName()
    {
        return m_sName;
    }
    
    //------------------------------------------------------------------------------------------------
    // Set admin mode
    void SetAdminMode(bool enabled)
    {
        m_bAdminMode = enabled;
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle config changes
    void OnConfigChanged(map<string, string> changedValues)
    {
        // Override in derived classes
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up resources
    void CleanUp()
    {
        // Override in derived classes
    }
    
    //------------------------------------------------------------------------------------------------
    // Get localized text
    string GetLocalizedText(string key, array<string> params = null)
    {
        if (m_Localization)
            return m_Localization.GetLocalizedString(key, params);
            
        return key;
    }
}

//------------------------------------------------------------------------------------------------
// Stats display component base class
class STS_StatsDisplayComponent : STS_UIComponent
{
    // Override in derived classes to implement stats display
    
    //------------------------------------------------------------------------------------------------
    // Refresh player data
    void RefreshPlayerData(string playerID)
    {
        // Override in derived classes
    }
}

//------------------------------------------------------------------------------------------------
// Notification component
class STS_NotificationComponent : STS_UIComponent
{
    // Notification queue
    protected ref array<ref STS_Notification> m_aNotifications = new array<ref STS_Notification>();
    
    // Maximum number of visible notifications
    protected int m_iMaxVisibleNotifications = 5;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_NotificationComponent()
    {
        m_sName = "Notifications";
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a notification
    void AddNotification(string message, float duration = 5.0, int type = 0)
    {
        // Create new notification
        STS_Notification notification = new STS_Notification(message, duration, type);
        m_aNotifications.Insert(notification);
        
        // Display notification
        DisplayNotifications();
    }
    
    //------------------------------------------------------------------------------------------------
    // Display notifications
    protected void DisplayNotifications()
    {
        // Override in derived classes to implement UI display
    }
    
    //------------------------------------------------------------------------------------------------
    // Update notifications
    override void Update()
    {
        float currentTime = GetGame().GetTime() / 1000.0;
        
        // Check for expired notifications
        for (int i = m_aNotifications.Count() - 1; i >= 0; i--)
        {
            STS_Notification notification = m_aNotifications[i];
            if (notification.IsExpired(currentTime))
            {
                m_aNotifications.RemoveOrdered(i);
            }
        }
        
        // Update display
        DisplayNotifications();
    }
}

//------------------------------------------------------------------------------------------------
// Notification data class
class STS_Notification
{
    protected string m_sMessage;          // Notification message
    protected float m_fDuration;          // Duration in seconds
    protected float m_fStartTime;         // Start time
    protected int m_iType;                // Notification type (0=info, 1=achievement, 2=warning, 3=error)
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_Notification(string message, float duration = 5.0, int type = 0)
    {
        m_sMessage = message;
        m_fDuration = duration;
        m_fStartTime = GetGame().GetTime() / 1000.0;
        m_iType = type;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if notification has expired
    bool IsExpired(float currentTime)
    {
        return (currentTime - m_fStartTime) >= m_fDuration;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get remaining time as percentage (for animation)
    float GetRemainingTimePercent(float currentTime)
    {
        float elapsed = currentTime - m_fStartTime;
        float remaining = Math.Clamp(1.0 - (elapsed / m_fDuration), 0.0, 1.0);
        return remaining;
    }
    
    //------------------------------------------------------------------------------------------------
    // Getters
    string GetMessage() { return m_sMessage; }
    int GetType() { return m_iType; }
} 