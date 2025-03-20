// STS_WebhookManager.c
// Provides webhook integration with external services

class STS_Webhook
{
    string m_sId;                  // Unique identifier for this webhook
    string m_sName;                // Human-readable name for this webhook
    string m_sUrl;                 // URL to send webhook data to
    string m_sSecret;              // Secret key for authentication
    string m_sContentType;         // Content type for the webhook (e.g., "application/json")
    bool m_bEnabled;               // Whether this webhook is enabled
    string m_sEventTypes;          // Comma-separated list of event types this webhook handles
    map<string, string> m_mHeaders; // Custom headers to include in requests
    int m_iMaxRetries;             // Maximum number of retries for failed webhook calls
    float m_fLastCallTime;         // Time of last webhook call
    int m_iRateLimitPerMinute;     // Maximum number of calls per minute (rate limiting)
    int m_iCallsThisMinute;        // Counter for rate limiting
    float m_fRateLimitResetTime;   // Time when rate limit counter resets
    
    //------------------------------------------------------------------------------------------------
    void STS_Webhook()
    {
        m_sId = "";
        m_sName = "";
        m_sUrl = "";
        m_sSecret = "";
        m_sContentType = "application/json";
        m_bEnabled = true;
        m_sEventTypes = "*";  // Default to all event types
        m_mHeaders = new map<string, string>();
        m_iMaxRetries = 3;
        m_fLastCallTime = 0;
        m_iRateLimitPerMinute = 60;
        m_iCallsThisMinute = 0;
        m_fRateLimitResetTime = 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if this webhook is interested in a specific event type
    bool HandlesEventType(string eventType)
    {
        if (m_sEventTypes == "*")
            return true; // Handles all events
            
        array<string> eventTypes = new array<string>();
        m_sEventTypes.Split(",", eventTypes);
        
        foreach (string type : eventTypes)
        {
            if (type.Trim() == eventType)
                return true;
        }
        
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if this webhook can be called (respecting rate limits)
    bool CanCall(float currentTime)
    {
        if (!m_bEnabled)
            return false;
            
        // Check rate limiting
        if (currentTime - m_fRateLimitResetTime >= 60.0)
        {
            // Reset rate limiting after 1 minute
            m_iCallsThisMinute = 0;
            m_fRateLimitResetTime = currentTime;
        }
        
        return m_iCallsThisMinute < m_iRateLimitPerMinute;
    }
    
    //------------------------------------------------------------------------------------------------
    // Mark a call as made (for rate limiting)
    void RecordCall(float currentTime)
    {
        m_fLastCallTime = currentTime;
        m_iCallsThisMinute++;
    }
}

class STS_IncomingWebhook
{
    string m_sId;                  // Unique identifier for this webhook
    string m_sName;                // Human-readable name for this webhook
    string m_sEndpoint;            // Endpoint path to receive webhook data
    string m_sSecret;              // Secret key for authentication
    bool m_bEnabled;               // Whether this webhook is enabled
    array<string> m_aAllowedIPs;   // List of allowed IP addresses (empty = all allowed)
    map<string, string> m_mRequiredHeaders; // Headers that must be present and match
    string m_sEventTypes;          // Comma-separated list of event types this webhook handles
    
    //------------------------------------------------------------------------------------------------
    void STS_IncomingWebhook()
    {
        m_sId = "";
        m_sName = "";
        m_sEndpoint = "";
        m_sSecret = "";
        m_bEnabled = true;
        m_aAllowedIPs = new array<string>();
        m_mRequiredHeaders = new map<string, string>();
        m_sEventTypes = "*";  // Default to all event types
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if this webhook is interested in a specific event type
    bool HandlesEventType(string eventType)
    {
        if (m_sEventTypes == "*")
            return true; // Handles all events
            
        array<string> eventTypes = new array<string>();
        m_sEventTypes.Split(",", eventTypes);
        
        foreach (string type : eventTypes)
        {
            if (type.Trim() == eventType)
                return true;
        }
        
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if request is authorized based on IP and headers
    bool IsAuthorized(string ipAddress, map<string, string> headers)
    {
        if (!m_bEnabled)
            return false;
            
        // Check IP whitelist if specified
        if (m_aAllowedIPs.Count() > 0)
        {
            bool ipAllowed = false;
            
            foreach (string allowedIP : m_aAllowedIPs)
            {
                if (ipAddress == allowedIP || allowedIP == "*")
                {
                    ipAllowed = true;
                    break;
                }
            }
            
            if (!ipAllowed)
                return false;
        }
        
        // Check required headers
        foreach (string headerKey, string headerValue : m_mRequiredHeaders)
        {
            if (!headers.Contains(headerKey) || headers.Get(headerKey) != headerValue)
                return false;
        }
        
        // Check secret if provided
        if (!m_sSecret.IsEmpty())
        {
            if (!headers.Contains("X-Webhook-Secret") || headers.Get("X-Webhook-Secret") != m_sSecret)
                return false;
        }
        
        return true;
    }
}

class STS_WebhookEvent
{
    string m_sEventType;           // Type of event
    map<string, string> m_mData;   // Event data
    float m_fTimestamp;            // Event timestamp
    bool m_bDelayed;               // Whether delivery of this event should be delayed
    float m_fDeliverAt;            // Time when this event should be delivered
    
    //------------------------------------------------------------------------------------------------
    void STS_WebhookEvent()
    {
        m_sEventType = "";
        m_mData = new map<string, string>();
        m_fTimestamp = GetGame().GetTickTime();
        m_bDelayed = false;
        m_fDeliverAt = 0;
    }
}

class STS_WebhookManagerConfig
{
    bool m_bEnabled = true;                         // Whether webhook integration is enabled
    bool m_bOutgoingEnabled = true;                 // Whether outgoing webhooks are enabled
    bool m_bIncomingEnabled = false;                // Whether incoming webhooks are enabled
    int m_iIncomingPort = 8080;                     // Port for incoming webhooks server
    int m_iQueueProcessIntervalMs = 1000;           // Interval to process queued webhook events in milliseconds
    int m_iMaxQueueSize = 1000;                     // Maximum number of events to queue
    array<ref STS_Webhook> m_aOutgoingWebhooks;     // List of outgoing webhooks
    array<ref STS_IncomingWebhook> m_aIncomingWebhooks; // List of incoming webhooks
    string m_sWebhooksConfigPath = "$profile:StatTracker/webhooks.json"; // Path to webhooks config file
    bool m_bLogWebhookCalls = true;                 // Whether to log webhook calls
    string m_sUserAgent = "StatTracker Webhook Client"; // User agent for outgoing webhook calls
    
    //------------------------------------------------------------------------------------------------
    void STS_WebhookManagerConfig()
    {
        m_aOutgoingWebhooks = new array<ref STS_Webhook>();
        m_aIncomingWebhooks = new array<ref STS_IncomingWebhook>();
        
        // Add example outgoing webhooks
        STS_Webhook discordWebhook = new STS_Webhook();
        discordWebhook.m_sId = "discord_notifications";
        discordWebhook.m_sName = "Discord Notifications";
        discordWebhook.m_sUrl = "https://discord.com/api/webhooks/example";
        discordWebhook.m_sContentType = "application/json";
        discordWebhook.m_sEventTypes = "player.join,player.leave,admin.action,server.restart";
        discordWebhook.m_mHeaders.Set("User-Agent", m_sUserAgent);
        
        m_aOutgoingWebhooks.Insert(discordWebhook);
        
        // Add example incoming webhooks
        STS_IncomingWebhook adminWebhook = new STS_IncomingWebhook();
        adminWebhook.m_sId = "admin_webhook";
        adminWebhook.m_sName = "Admin Control Webhook";
        adminWebhook.m_sEndpoint = "/api/admin";
        adminWebhook.m_sEventTypes = "admin.command";
        adminWebhook.m_aAllowedIPs.Insert("127.0.0.1"); // Only allow local requests
        
        m_aIncomingWebhooks.Insert(adminWebhook);
    }
}

class STS_WebhookManager
{
    // Singleton instance
    private static ref STS_WebhookManager s_Instance;
    
    // Configuration
    protected ref STS_WebhookManagerConfig m_Config;
    
    // References to other systems
    protected ref STS_LoggingSystem m_Logger;
    protected ref STS_Config m_MainConfig;
    protected ref STS_HTTPWorker m_HTTPWorker;
    
    // Webhook queue
    protected ref array<ref STS_WebhookEvent> m_aEventQueue;
    protected ref map<string, ref array<ref STS_Webhook>> m_mEventTypeHandlers;
    
    // Internal HTTP server (for incoming webhooks)
    // In a real implementation, this would be a proper HTTP server
    // For this example, it's just a placeholder
    protected bool m_bServerRunning = false;
    
    // Enhanced error tracking
    protected ref map<string, int> m_mErrorCounts = new map<string, int>();
    protected ref map<string, ref array<string>> m_mErrorContexts = new map<string, ref array<string>>();
    protected const int MAX_ERROR_CONTEXTS = 10;
    
    // Retry settings
    protected const float RETRY_CHECK_INTERVAL = 60; // 1 minute
    protected const int MAX_RETRY_ATTEMPTS = 3;
    protected const float RETRY_DELAY = 5.0; // 5 seconds between retries
    protected int m_iRetryAttempts = 0;
    protected float m_fLastRetryAttempt = 0;
    
    // Security settings
    protected const int MAX_REQUESTS_PER_MINUTE = 100;
    protected const int MAX_REQUESTS_PER_HOUR = 1000;
    protected ref map<string, ref array<float>> m_mRequestTimestamps = new map<string, ref array<float>>();
    protected const float REQUEST_WINDOW_MINUTES = 60;
    protected const float REQUEST_WINDOW_HOURS = 3600;
    
    // Health monitoring
    protected bool m_bIsHealthy = true;
    protected float m_fLastHealthCheck = 0;
    protected const float HEALTH_CHECK_INTERVAL = 300; // 5 minutes
    
    //------------------------------------------------------------------------------------------------
    protected void STS_WebhookManager()
    {
        try
        {
            // Initialize components
            InitializeComponents();
            
            // Set up monitoring
            GetGame().GetCallqueue().CallLater(CheckHealth, HEALTH_CHECK_INTERVAL, true);
            GetGame().GetCallqueue().CallLater(CheckRetries, RETRY_CHECK_INTERVAL, true);
            
            // Enhanced logging of initialization
            LogInfo("Webhook manager initialized", "Constructor", {
                "outgoing_webhooks": m_Config.m_aOutgoingWebhooks.Count(),
                "incoming_webhooks": m_Config.m_aIncomingWebhooks.Count(),
                "queue_size": m_aEventQueue.Count()
            });
        }
        catch (Exception e)
        {
            HandleInitializationError(e);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    static STS_WebhookManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_WebhookManager();
        }
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    protected void InitializeComponents()
    {
        try
        {
            // Initialize logging
            m_Logger = STS_LoggingSystem.GetInstance();
            if (!m_Logger)
            {
                throw new Exception("Failed to initialize logging system");
            }
            
            // Initialize main config
            m_MainConfig = STS_Config.GetInstance();
            if (!m_MainConfig)
            {
                throw new Exception("Failed to initialize main configuration");
            }
            
            // Initialize HTTP worker
            m_HTTPWorker = new STS_HTTPWorker();
            if (!m_HTTPWorker)
            {
                throw new Exception("Failed to initialize HTTP worker");
            }
            
            // Initialize event queue and handlers
            m_aEventQueue = new array<ref STS_WebhookEvent>();
            m_mEventTypeHandlers = new map<string, ref array<ref STS_Webhook>>();
            
            // Load webhook configuration
            if (!LoadWebhookConfig())
            {
                throw new Exception("Failed to load webhook configuration");
            }
        }
        catch (Exception e)
        {
            throw new Exception(string.Format("Component initialization failed: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void HandleInitializationError(Exception e)
    {
        string errorContext = string.Format("Webhook manager initialization failed: %1\nStack trace: %2", 
            e.ToString(), e.StackTrace);
            
        if (m_Logger)
        {
            m_Logger.LogError(errorContext, "STS_WebhookManager", "HandleInitializationError", {
                "error_type": e.Type().ToString()
            });
        }
        else
        {
            Print("[StatTracker] CRITICAL ERROR: " + errorContext);
        }
        
        // Set up recovery attempt
        m_bIsHealthy = false;
        m_fLastRetryAttempt = GetGame().GetTickTime();
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CheckHealth()
    {
        try
        {
            float currentTime = GetGame().GetTickTime();
            
            // Check HTTP worker health
            if (!m_HTTPWorker.IsHealthy())
            {
                LogWarning("HTTP worker health check failed", "CheckHealth");
                m_bIsHealthy = false;
            }
            
            // Check event queue health
            if (m_aEventQueue.Count() >= m_Config.m_iMaxQueueSize)
            {
                LogWarning("Event queue size limit reached", "CheckHealth", {
                    "queue_size": m_aEventQueue.Count(),
                    "max_size": m_Config.m_iMaxQueueSize
                });
                m_bIsHealthy = false;
            }
            
            // Check webhook configurations
            foreach (STS_Webhook webhook : m_Config.m_aOutgoingWebhooks)
            {
                if (!ValidateWebhookConfig(webhook))
                {
                    LogWarning("Invalid webhook configuration detected", "CheckHealth", {
                        "webhook_id": webhook.m_sId,
                        "webhook_name": webhook.m_sName
                    });
                    m_bIsHealthy = false;
                }
            }
            
            // Update health status
            if (m_bIsHealthy)
            {
                LogDebug("Health check passed", "CheckHealth");
            }
            else
            {
                LogWarning("Health check failed", "CheckHealth");
            }
            
            m_fLastHealthCheck = currentTime;
        }
        catch (Exception e)
        {
            LogError("Exception during health check", "CheckHealth", {
                "error": e.ToString()
            });
            m_bIsHealthy = false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CheckRetries()
    {
        if (!m_bIsHealthy) return;
        
        try
        {
            float currentTime = GetGame().GetTickTime();
            if (currentTime - m_fLastRetryAttempt >= RETRY_CHECK_INTERVAL)
            {
                if (m_iRetryAttempts >= MAX_RETRY_ATTEMPTS)
                {
                    LogCritical("Maximum retry attempts reached", "CheckRetries", {
                        "attempts": m_iRetryAttempts
                    });
                    return;
                }
                
                m_iRetryAttempts++;
                m_fLastRetryAttempt = currentTime;
                
                // Attempt recovery
                if (AttemptRecovery())
                {
                    LogInfo("Webhook manager recovered successfully", "CheckRetries", {
                        "attempts": m_iRetryAttempts
                    });
                    m_bIsHealthy = true;
                    m_iRetryAttempts = 0;
                }
                else
                {
                    LogWarning("Recovery attempt failed", "CheckRetries", {
                        "attempt": m_iRetryAttempts
                    });
                }
            }
        }
        catch (Exception e)
        {
            LogError("Exception during retry check", "CheckRetries", {
                "error": e.ToString(),
                "attempts": m_iRetryAttempts
            });
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool AttemptRecovery()
    {
        try
        {
            // Re-initialize components
            InitializeComponents();
            
            // Verify webhook configurations
            if (!VerifyWebhookConfigs())
            {
                return false;
            }
            
            // Test HTTP connectivity
            if (!TestHTTPConnectivity())
            {
                return false;
            }
            
            // Process any pending events
            ProcessEventQueue();
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Exception during recovery attempt", "AttemptRecovery", {
                "error": e.ToString()
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool ValidateWebhookConfig(STS_Webhook webhook)
    {
        try
        {
            // Validate required fields
            if (webhook.m_sId.IsEmpty() || webhook.m_sName.IsEmpty() || webhook.m_sUrl.IsEmpty())
            {
                return false;
            }
            
            // Validate URL format
            if (!IsValidURL(webhook.m_sUrl))
            {
                return false;
            }
            
            // Validate rate limits
            if (webhook.m_iRateLimitPerMinute <= 0)
            {
                return false;
            }
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Exception validating webhook config", "ValidateWebhookConfig", {
                "error": e.ToString(),
                "webhook_id": webhook.m_sId
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool IsValidURL(string url)
    {
        try
        {
            // Basic URL validation
            if (url.IndexOf("http://") != 0 && url.IndexOf("https://") != 0)
            {
                return false;
            }
            
            // Check for valid characters
            for (int i = 0; i < url.Length(); i++)
            {
                if (!IsValidURLCharacter(url[i]))
                {
                    return false;
                }
            }
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Exception validating URL", "IsValidURL", {
                "error": e.ToString(),
                "url": url
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool IsValidURLCharacter(int character)
    {
        // Allow alphanumeric characters, hyphens, dots, and common URL characters
        return (character >= 'a' && character <= 'z') ||
               (character >= 'A' && character <= 'Z') ||
               (character >= '0' && character <= '9') ||
               character == '-' || character == '.' || character == '_' ||
               character == ':' || character == '/' || character == '?' ||
               character == '=' || character == '&';
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool TestHTTPConnectivity()
    {
        try
        {
            // Test connection to a reliable endpoint
            string testUrl = "https://api.github.com";
            if (!m_HTTPWorker.TestConnection(testUrl))
            {
                LogError("HTTP connectivity test failed", "TestHTTPConnectivity");
                return false;
            }
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Exception during HTTP connectivity test", "TestHTTPConnectivity", {
                "error": e.ToString()
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool VerifyWebhookConfigs()
    {
        try
        {
            // Verify outgoing webhooks
            foreach (STS_Webhook webhook : m_Config.m_aOutgoingWebhooks)
            {
                if (!ValidateWebhookConfig(webhook))
                {
                    return false;
                }
            }
            
            // Verify incoming webhooks
            foreach (STS_IncomingWebhook webhook : m_Config.m_aIncomingWebhooks)
            {
                if (!ValidateIncomingWebhookConfig(webhook))
                {
                    return false;
                }
            }
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Exception verifying webhook configs", "VerifyWebhookConfigs", {
                "error": e.ToString()
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool ValidateIncomingWebhookConfig(STS_IncomingWebhook webhook)
    {
        try
        {
            // Validate required fields
            if (webhook.m_sId.IsEmpty() || webhook.m_sName.IsEmpty() || webhook.m_sEndpoint.IsEmpty())
            {
                return false;
            }
            
            // Validate endpoint format
            if (!IsValidEndpoint(webhook.m_sEndpoint))
            {
                return false;
            }
            
            // Validate IP whitelist if specified
            if (webhook.m_aAllowedIPs.Count() > 0)
            {
                foreach (string ip : webhook.m_aAllowedIPs)
                {
                    if (!IsValidIP(ip))
                    {
                        return false;
                    }
                }
            }
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Exception validating incoming webhook config", "ValidateIncomingWebhookConfig", {
                "error": e.ToString(),
                "webhook_id": webhook.m_sId
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool IsValidEndpoint(string endpoint)
    {
        try
        {
            // Basic endpoint validation
            if (endpoint[0] != '/')
            {
                return false;
            }
            
            // Check for valid characters
            for (int i = 0; i < endpoint.Length(); i++)
            {
                if (!IsValidEndpointCharacter(endpoint[i]))
                {
                    return false;
                }
            }
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Exception validating endpoint", "IsValidEndpoint", {
                "error": e.ToString(),
                "endpoint": endpoint
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool IsValidEndpointCharacter(int character)
    {
        // Allow alphanumeric characters, hyphens, and forward slashes
        return (character >= 'a' && character <= 'z') ||
               (character >= 'A' && character <= 'Z') ||
               (character >= '0' && character <= '9') ||
               character == '-' || character == '/';
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool IsValidIP(string ip)
    {
        try
        {
            // Allow wildcard
            if (ip == "*")
            {
                return true;
            }
            
            // Split IP into octets
            array<string> octets = new array<string>();
            ip.Split(".", octets);
            
            if (octets.Count() != 4)
            {
                return false;
            }
            
            // Validate each octet
            foreach (string octet : octets)
            {
                int value = octet.ToInt();
                if (value < 0 || value > 255)
                {
                    return false;
                }
            }
            
            return true;
        }
        catch (Exception e)
        {
            LogError("Exception validating IP", "IsValidIP", {
                "error": e.ToString(),
                "ip": ip
            });
            return false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load configuration
    protected void LoadConfiguration()
    {
        if (!m_MainConfig)
            return;
            
        // Load general configuration
        m_Config.m_bEnabled = m_MainConfig.GetBoolValue("webhooks_enabled", m_Config.m_bEnabled);
        m_Config.m_bOutgoingEnabled = m_MainConfig.GetBoolValue("outgoing_webhooks_enabled", m_Config.m_bOutgoingEnabled);
        m_Config.m_bIncomingEnabled = m_MainConfig.GetBoolValue("incoming_webhooks_enabled", m_Config.m_bIncomingEnabled);
        m_Config.m_iIncomingPort = m_MainConfig.GetIntValue("incoming_webhooks_port", m_Config.m_iIncomingPort);
        m_Config.m_iQueueProcessIntervalMs = m_MainConfig.GetIntValue("webhook_queue_process_interval", m_Config.m_iQueueProcessIntervalMs);
        m_Config.m_iMaxQueueSize = m_MainConfig.GetIntValue("webhook_max_queue_size", m_Config.m_iMaxQueueSize);
        m_Config.m_sWebhooksConfigPath = m_MainConfig.GetStringValue("webhooks_config_path", m_Config.m_sWebhooksConfigPath);
        m_Config.m_bLogWebhookCalls = m_MainConfig.GetBoolValue("log_webhook_calls", m_Config.m_bLogWebhookCalls);
        m_Config.m_sUserAgent = m_MainConfig.GetStringValue("webhook_user_agent", m_Config.m_sUserAgent);
        
        // Load webhooks from config file
        LoadWebhooksFromConfig();
        
        m_Logger.LogInfo(string.Format("Loaded webhook manager configuration. Enabled: %1", m_Config.m_bEnabled));
    }
    
    //------------------------------------------------------------------------------------------------
    // Load webhooks from config file
    protected void LoadWebhooksFromConfig()
    {
        // In a real implementation, this would load webhooks from a JSON file
        // For this example, we'll just use the default webhooks from the config constructor
        
        m_Logger.LogInfo("Would load webhooks from config file: " + m_Config.m_sWebhooksConfigPath);
    }
    
    //------------------------------------------------------------------------------------------------
    // Save webhooks to config file
    protected void SaveWebhooksToConfig()
    {
        // In a real implementation, this would save webhooks to a JSON file
        // For this example, we'll just log it
        
        m_Logger.LogInfo("Would save webhooks to config file: " + m_Config.m_sWebhooksConfigPath);
    }
    
    //------------------------------------------------------------------------------------------------
    // Build a map of event types to handlers for faster lookup
    protected void BuildEventTypeHandlersMap()
    {
        m_mEventTypeHandlers.Clear();
        
        foreach (STS_Webhook webhook : m_Config.m_aOutgoingWebhooks)
        {
            if (!webhook.m_bEnabled)
                continue;
                
            if (webhook.m_sEventTypes == "*")
            {
                // This webhook handles all event types
                // We'll add it to a special key
                if (!m_mEventTypeHandlers.Contains("*"))
                {
                    m_mEventTypeHandlers.Set("*", new array<ref STS_Webhook>());
                }
                
                m_mEventTypeHandlers.Get("*").Insert(webhook);
                continue;
            }
            
            // Parse event types
            array<string> eventTypes = new array<string>();
            webhook.m_sEventTypes.Split(",", eventTypes);
            
            foreach (string eventType : eventTypes)
            {
                string trimmedType = eventType.Trim();
                
                if (trimmedType.IsEmpty())
                    continue;
                    
                if (!m_mEventTypeHandlers.Contains(trimmedType))
                {
                    m_mEventTypeHandlers.Set(trimmedType, new array<ref STS_Webhook>());
                }
                
                m_mEventTypeHandlers.Get(trimmedType).Insert(webhook);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Start the incoming webhook server
    protected void StartIncomingWebhookServer()
    {
        // In a real implementation, this would start an HTTP server to receive webhooks
        // For this example, we'll just log it
        
        m_Logger.LogInfo(string.Format("Would start incoming webhook server on port %1", m_Config.m_iIncomingPort));
        m_bServerRunning = true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Stop the incoming webhook server
    protected void StopIncomingWebhookServer()
    {
        // In a real implementation, this would stop the HTTP server
        // For this example, we'll just log it
        
        if (m_bServerRunning)
        {
            m_Logger.LogInfo("Would stop incoming webhook server");
            m_bServerRunning = false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Process the event queue
    protected void ProcessEventQueue()
    {
        if (!m_Config.m_bEnabled || !m_Config.m_bOutgoingEnabled || m_aEventQueue.Count() == 0)
            return;
            
        float currentTime = GetGame().GetTickTime();
        
        // Process up to 10 events per call to avoid blocking
        int processCount = Math.Min(10, m_aEventQueue.Count());
        
        for (int i = 0; i < processCount; i++)
        {
            if (m_aEventQueue.Count() == 0)
                break;
                
            STS_WebhookEvent event = m_aEventQueue[0];
            
            // Check if this is a delayed event and it's not time to process it yet
            if (event.m_bDelayed && currentTime < event.m_fDeliverAt)
            {
                // Skip this event and try the next one
                continue;
            }
            
            // Remove the event from the queue
            m_aEventQueue.RemoveOrdered(0);
            
            // Process the event
            ProcessEvent(event, currentTime);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Process a single event
    protected void ProcessEvent(STS_WebhookEvent event, float currentTime)
    {
        // Get webhooks that handle this event type
        array<ref STS_Webhook> handlers = new array<ref STS_Webhook>();
        
        // Add specific handlers for this event type
        if (m_mEventTypeHandlers.Contains(event.m_sEventType))
        {
            foreach (STS_Webhook webhook : m_mEventTypeHandlers.Get(event.m_sEventType))
            {
                handlers.Insert(webhook);
            }
        }
        
        // Add handlers that handle all event types
        if (m_mEventTypeHandlers.Contains("*"))
        {
            foreach (STS_Webhook webhook : m_mEventTypeHandlers.Get("*"))
            {
                handlers.Insert(webhook);
            }
        }
        
        // Send the event to each handler
        foreach (STS_Webhook webhook : handlers)
        {
            if (webhook.CanCall(currentTime))
            {
                SendWebhook(webhook, event);
                webhook.RecordCall(currentTime);
            }
            else
            {
                m_Logger.LogWarning(string.Format("Webhook %1 (%2) is rate limited, skipping event %3", 
                    webhook.m_sName, webhook.m_sId, event.m_sEventType));
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a webhook to a specific endpoint
    protected void SendWebhook(STS_Webhook webhook, STS_WebhookEvent event)
    {
        if (m_Config.m_bLogWebhookCalls)
        {
            m_Logger.LogInfo(string.Format("Sending webhook %1 (%2) for event %3", 
                webhook.m_sName, webhook.m_sId, event.m_sEventType));
        }
        
        // Create the webhook payload
        string payload = CreateWebhookPayload(webhook, event);
        
        // Create headers
        map<string, string> headers = new map<string, string>();
        
        // Add content type
        headers.Set("Content-Type", webhook.m_sContentType);
        
        // Add custom headers
        foreach (string headerKey, string headerValue : webhook.m_mHeaders)
        {
            headers.Set(headerKey, headerValue);
        }
        
        // Add signature if a secret is provided
        if (!webhook.m_sSecret.IsEmpty())
        {
            string signature = GenerateSignature(payload, webhook.m_sSecret);
            headers.Set("X-Webhook-Signature", signature);
        }
        
        // In a real implementation, this would send an HTTP request
        // For this example, we'll just log it
        
        if (m_Config.m_bLogWebhookCalls)
        {
            m_Logger.LogDebug(string.Format("Would send webhook to %1: %2", webhook.m_sUrl, payload));
        }
        
        // Simulate sending the webhook
        SimulateSendWebhook(webhook, payload, headers);
    }
    
    //------------------------------------------------------------------------------------------------
    // Create a webhook payload for an event
    protected string CreateWebhookPayload(STS_Webhook webhook, STS_WebhookEvent event)
    {
        // In a real implementation, this would create a proper JSON payload
        // For this example, we'll just create a simple JSON string
        
        string json = "{\n";
        json += "  \"event_type\": \"" + event.m_sEventType + "\",\n";
        json += "  \"timestamp\": " + event.m_fTimestamp + ",\n";
        json += "  \"data\": {\n";
        
        bool firstItem = true;
        foreach (string key, string value : event.m_mData)
        {
            if (!firstItem)
                json += ",\n";
                
            json += "    \"" + key + "\": \"" + value + "\"";
            firstItem = false;
        }
        
        json += "\n  }\n";
        json += "}";
        
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a signature for webhook authentication
    protected string GenerateSignature(string payload, string secret)
    {
        // In a real implementation, this would generate an HMAC signature
        // For this example, we'll just return a placeholder
        
        return "simulated-signature";
    }
    
    //------------------------------------------------------------------------------------------------
    // Simulate sending a webhook
    protected void SimulateSendWebhook(STS_Webhook webhook, string payload, map<string, string> headers)
    {
        // In a real implementation, this would send an HTTP request using m_HTTPWorker
        // For this example, we'll just log success after a delay
        
        GetGame().GetCallqueue().CallLater(SimulateWebhookResponse, 100, false, webhook);
    }
    
    //------------------------------------------------------------------------------------------------
    // Simulate a webhook response
    protected void SimulateWebhookResponse(STS_Webhook webhook)
    {
        if (m_Config.m_bLogWebhookCalls)
        {
            m_Logger.LogInfo(string.Format("Webhook %1 (%2) sent successfully", webhook.m_sName, webhook.m_sId));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle an incoming webhook request
    protected void HandleIncomingWebhook(string endpoint, string ipAddress, map<string, string> headers, string body)
    {
        if (!m_Config.m_bEnabled || !m_Config.m_bIncomingEnabled)
            return;
            
        try
        {
            // Find the webhook handler for this endpoint
            STS_IncomingWebhook handler = null;
            
            foreach (STS_IncomingWebhook webhook : m_Config.m_aIncomingWebhooks)
            {
                if (webhook.m_sEndpoint == endpoint)
                {
                    handler = webhook;
                    break;
                }
            }
            
            if (!handler)
            {
                m_Logger.LogWarning(string.Format("No handler found for incoming webhook endpoint: %1", endpoint));
                return;
            }
            
            // Check authorization
            if (!handler.IsAuthorized(ipAddress, headers))
            {
                m_Logger.LogWarning(string.Format("Unauthorized incoming webhook request to %1 from %2", endpoint, ipAddress));
                return;
            }
            
            // Parse the event type from the body or headers
            string eventType = "";
            
            if (headers.Contains("X-Webhook-Event"))
            {
                eventType = headers.Get("X-Webhook-Event");
            }
            else
            {
                // Try to extract event type from JSON body
                // This is a simplified example; in a real implementation,
                // you would use a proper JSON parser
                int eventTypeStart = body.IndexOf("\"event_type\":\"") + 14;
                int eventTypeEnd = body.IndexOf("\"", eventTypeStart);
                
                if (eventTypeStart > 14 && eventTypeEnd > eventTypeStart)
                {
                    eventType = body.Substring(eventTypeStart, eventTypeEnd - eventTypeStart);
                }
            }
            
            if (eventType.IsEmpty())
            {
                m_Logger.LogWarning("No event type found in incoming webhook request");
                return;
            }
            
            // Check if this webhook handles this event type
            if (!handler.HandlesEventType(eventType))
            {
                m_Logger.LogWarning(string.Format("Webhook %1 does not handle event type: %2", handler.m_sName, eventType));
                return;
            }
            
            // Process the webhook
            if (m_Config.m_bLogWebhookCalls)
            {
                m_Logger.LogInfo(string.Format("Processing incoming webhook %1 for event %2", handler.m_sName, eventType));
            }
            
            // In a real implementation, this would parse the webhook payload and take appropriate action
            // For this example, we'll just log it
            
            m_Logger.LogInfo(string.Format("Would process incoming webhook: %1", body));
            
            // Note: In a real implementation, you would execute code based on the webhook content
            // For example, if it's an admin command, you would call the appropriate admin function
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception handling incoming webhook: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Queue an event to be sent to webhooks
    void QueueEvent(string eventType, map<string, string> data, bool immediate = true)
    {
        if (!m_Config.m_bEnabled || !m_Config.m_bOutgoingEnabled)
            return;
            
        // Check if we've reached the maximum queue size
        if (m_aEventQueue.Count() >= m_Config.m_iMaxQueueSize)
        {
            m_Logger.LogWarning(string.Format("Webhook event queue is full, dropping event: %1", eventType));
            return;
        }
        
        // Create the event
        STS_WebhookEvent event = new STS_WebhookEvent();
        event.m_sEventType = eventType;
        event.m_mData = data;
        event.m_fTimestamp = GetGame().GetTickTime();
        
        if (!immediate)
        {
            // Delay the event for a small amount of time (e.g., 5 seconds)
            event.m_bDelayed = true;
            event.m_fDeliverAt = event.m_fTimestamp + 5.0;
        }
        
        // Add to queue
        m_aEventQueue.Insert(event);
        
        if (m_Config.m_bLogWebhookCalls)
        {
            m_Logger.LogDebug(string.Format("Queued webhook event: %1", eventType));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a new outgoing webhook
    STS_Webhook AddOutgoingWebhook(string id, string name, string url, string eventTypes)
    {
        // Check if a webhook with this ID already exists
        foreach (STS_Webhook webhook : m_Config.m_aOutgoingWebhooks)
        {
            if (webhook.m_sId == id)
            {
                m_Logger.LogWarning(string.Format("Webhook with ID %1 already exists", id));
                return webhook;
            }
        }
        
        // Create the webhook
        STS_Webhook newWebhook = new STS_Webhook();
        newWebhook.m_sId = id;
        newWebhook.m_sName = name;
        newWebhook.m_sUrl = url;
        newWebhook.m_sEventTypes = eventTypes;
        newWebhook.m_mHeaders.Set("User-Agent", m_Config.m_sUserAgent);
        
        // Add to the list
        m_Config.m_aOutgoingWebhooks.Insert(newWebhook);
        
        // Rebuild event type handlers map
        BuildEventTypeHandlersMap();
        
        // Save webhooks to config
        SaveWebhooksToConfig();
        
        return newWebhook;
    }
    
    //------------------------------------------------------------------------------------------------
    // Remove an outgoing webhook
    bool RemoveOutgoingWebhook(string id)
    {
        for (int i = 0; i < m_Config.m_aOutgoingWebhooks.Count(); i++)
        {
            if (m_Config.m_aOutgoingWebhooks[i].m_sId == id)
            {
                m_Config.m_aOutgoingWebhooks.RemoveOrdered(i);
                
                // Rebuild event type handlers map
                BuildEventTypeHandlersMap();
                
                // Save webhooks to config
                SaveWebhooksToConfig();
                
                return true;
            }
        }
        
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a new incoming webhook
    STS_IncomingWebhook AddIncomingWebhook(string id, string name, string endpoint, string eventTypes)
    {
        // Check if a webhook with this ID already exists
        foreach (STS_IncomingWebhook webhook : m_Config.m_aIncomingWebhooks)
        {
            if (webhook.m_sId == id)
            {
                m_Logger.LogWarning(string.Format("Incoming webhook with ID %1 already exists", id));
                return webhook;
            }
        }
        
        // Create the webhook
        STS_IncomingWebhook newWebhook = new STS_IncomingWebhook();
        newWebhook.m_sId = id;
        newWebhook.m_sName = name;
        newWebhook.m_sEndpoint = endpoint;
        newWebhook.m_sEventTypes = eventTypes;
        
        // Add to the list
        m_Config.m_aIncomingWebhooks.Insert(newWebhook);
        
        // Save webhooks to config
        SaveWebhooksToConfig();
        
        return newWebhook;
    }
    
    //------------------------------------------------------------------------------------------------
    // Remove an incoming webhook
    bool RemoveIncomingWebhook(string id)
    {
        for (int i = 0; i < m_Config.m_aIncomingWebhooks.Count(); i++)
        {
            if (m_Config.m_aIncomingWebhooks[i].m_sId == id)
            {
                m_Config.m_aIncomingWebhooks.RemoveOrdered(i);
                
                // Save webhooks to config
                SaveWebhooksToConfig();
                
                return true;
            }
        }
        
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get an outgoing webhook by ID
    STS_Webhook GetOutgoingWebhook(string id)
    {
        foreach (STS_Webhook webhook : m_Config.m_aOutgoingWebhooks)
        {
            if (webhook.m_sId == id)
            {
                return webhook;
            }
        }
        
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get an incoming webhook by ID
    STS_IncomingWebhook GetIncomingWebhook(string id)
    {
        foreach (STS_IncomingWebhook webhook : m_Config.m_aIncomingWebhooks)
        {
            if (webhook.m_sId == id)
            {
                return webhook;
            }
        }
        
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all outgoing webhooks
    array<ref STS_Webhook> GetOutgoingWebhooks()
    {
        return m_Config.m_aOutgoingWebhooks;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all incoming webhooks
    array<ref STS_IncomingWebhook> GetIncomingWebhooks()
    {
        return m_Config.m_aIncomingWebhooks;
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper functions for common events
    
    //------------------------------------------------------------------------------------------------
    // Send a player joined event
    void SendPlayerJoinedEvent(string playerId, string playerName)
    {
        map<string, string> data = new map<string, string>();
        data.Set("player_id", playerId);
        data.Set("player_name", playerName);
        data.Set("server_id", GetServerId());
        data.Set("server_name", GetServerName());
        
        QueueEvent("player.join", data);
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a player left event
    void SendPlayerLeftEvent(string playerId, string playerName)
    {
        map<string, string> data = new map<string, string>();
        data.Set("player_id", playerId);
        data.Set("player_name", playerName);
        data.Set("server_id", GetServerId());
        data.Set("server_name", GetServerName());
        
        QueueEvent("player.leave", data);
    }
    
    //------------------------------------------------------------------------------------------------
    // Send an admin action event
    void SendAdminActionEvent(string adminId, string adminName, string action, string targetId, string details)
    {
        map<string, string> data = new map<string, string>();
        data.Set("admin_id", adminId);
        data.Set("admin_name", adminName);
        data.Set("action", action);
        data.Set("target_id", targetId);
        data.Set("details", details);
        data.Set("server_id", GetServerId());
        data.Set("server_name", GetServerName());
        
        QueueEvent("admin.action", data);
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a server restart event
    void SendServerRestartEvent(int countdownSeconds, string reason)
    {
        map<string, string> data = new map<string, string>();
        data.Set("countdown_seconds", countdownSeconds.ToString());
        data.Set("reason", reason);
        data.Set("server_id", GetServerId());
        data.Set("server_name", GetServerName());
        
        QueueEvent("server.restart", data);
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper function to get the server ID
    protected string GetServerId()
    {
        STS_MultiServerIntegration multiServer = STS_MultiServerIntegration.GetInstance();
        if (multiServer && multiServer.m_Config)
        {
            return multiServer.m_Config.m_sCurrentServerId;
        }
        
        return "unknown";
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper function to get the server name
    protected string GetServerName()
    {
        STS_MultiServerIntegration multiServer = STS_MultiServerIntegration.GetInstance();
        if (multiServer && multiServer.m_Config)
        {
            return multiServer.m_Config.m_sCurrentServerName;
        }
        
        return "Unknown Server";
    }
} 