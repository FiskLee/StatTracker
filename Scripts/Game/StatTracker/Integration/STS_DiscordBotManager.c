// STS_DiscordBotManager.c
// Discord bot integration component for remote administration and notifications

class STS_DiscordCommand
{
    string m_sCommand;           // Command name
    string m_sDescription;       // Command description
    array<string> m_aParameters; // Parameters for this command
    
    void STS_DiscordCommand(string command, string description, array<string> parameters = null)
    {
        m_sCommand = command;
        m_sDescription = description;
        
        if (parameters)
            m_aParameters = parameters;
        else
            m_aParameters = new array<string>();
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"command\":\"" + m_sCommand + "\",";
        json += "\"description\":\"" + m_sDescription + "\",";
        json += "\"parameters\":[";
        
        for (int i = 0; i < m_aParameters.Count(); i++)
        {
            json += "\"" + m_aParameters[i] + "\"";
            if (i < m_aParameters.Count() - 1) json += ",";
        }
        
        json += "]}";
        return json;
    }
}

class STS_DiscordEvent
{
    string m_sType;            // Event type (e.g., "player_joined", "player_killed", "base_captured")
    string m_sMessage;         // Human-readable message
    map<string, string> m_Data; // Additional event data
    float m_fTimestamp;        // When the event occurred
    
    void STS_DiscordEvent(string type, string message, map<string, string> data = null)
    {
        m_sType = type;
        m_sMessage = message;
        m_fTimestamp = System.GetTickCount() / 1000.0;
        
        if (data)
            m_Data = data;
        else
            m_Data = new map<string, string>();
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"type\":\"" + m_sType + "\",";
        json += "\"message\":\"" + m_sMessage + "\",";
        json += "\"timestamp\":" + m_fTimestamp.ToString() + ",";
        json += "\"data\":{";
        
        int idx = 0;
        foreach (string key, string value : m_Data)
        {
            if (idx > 0) json += ",";
            json += "\"" + key + "\":\"" + value + "\"";
            idx++;
        }
        
        json += "}}";
        return json;
    }
}

class STS_DiscordWebhook
{
    string m_sName;        // Name of this webhook
    string m_sURL;         // Discord webhook URL
    array<string> m_aEventTypes; // Event types to forward to this webhook
    
    void STS_DiscordWebhook(string name, string url, array<string> eventTypes = null)
    {
        m_sName = name;
        m_sURL = url;
        
        if (eventTypes)
            m_aEventTypes = eventTypes;
        else
            m_aEventTypes = new array<string>();
    }
    
    bool ShouldHandleEvent(string eventType)
    {
        // Special case: if no event types are specified, handle all events
        if (m_aEventTypes.Count() == 0)
            return true;
            
        return m_aEventTypes.Contains(eventType);
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"name\":\"" + m_sName + "\",";
        json += "\"url\":\"" + m_sURL + "\",";
        json += "\"eventTypes\":[";
        
        for (int i = 0; i < m_aEventTypes.Count(); i++)
        {
            json += "\"" + m_aEventTypes[i] + "\"";
            if (i < m_aEventTypes.Count() - 1) json += ",";
        }
        
        json += "]}";
        return json;
    }
    
    static STS_DiscordWebhook FromJSON(string json)
    {
        JsonSerializer js = new JsonSerializer();
        string error;
        STS_DiscordWebhook webhook = new STS_DiscordWebhook("", "");
        
        bool success = js.ReadFromString(webhook, json, error);
        if (!success)
        {
            Print("[StatTracker] Error parsing Discord webhook JSON: " + error, LogLevel.ERROR);
            return null;
        }
        
        return webhook;
    }
}

class STS_DiscordBotManager
{
    // Singleton instance
    private static ref STS_DiscordBotManager s_Instance;
    
    // Logger reference
    protected STS_LoggingSystem m_Logger;
    
    // Config reference
    protected STS_Config m_Config;
    
    // Event queue
    protected ref array<ref STS_DiscordEvent> m_aEventQueue = new array<ref STS_DiscordEvent>();
    
    // Command queue (in the full implementation, this would be populated from Discord API)
    protected ref array<ref map<string, string>> m_aCommandQueue = new array<ref map<string, string>>();
    
    // Command handlers
    protected ref map<string, ref array<ref Param3<int, int, string>>> m_mCommandHandlers = new map<string, ref array<ref Param3<int, int, string>>>();
    
    // Available commands
    protected ref array<ref STS_DiscordCommand> m_aAvailableCommands = new array<ref STS_DiscordCommand>();
    
    // Registered webhooks
    protected ref array<ref STS_DiscordWebhook> m_aWebhooks = new array<ref STS_DiscordWebhook>();
    
    // Constants
    protected const string WEBHOOKS_CONFIG_PATH = "$profile:StatTracker/Discord/webhooks.json";
    protected const string COMMANDS_CONFIG_PATH = "$profile:StatTracker/Discord/commands.json";
    protected const float PROCESS_INTERVAL = 5.0; // 5 seconds
    
    // Last processing time
    protected float m_fLastProcessTime = 0;
    
    // Bot status
    protected bool m_bEnabled = false;
    protected bool m_bConnected = false;
    protected string m_sBotToken = "";
    protected string m_sGuildID = "";
    protected string m_sCommandPrefix = "!";
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_DiscordBotManager()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Config = STS_Config.GetInstance();
        
        if (!m_Logger || !m_Config)
        {
            Print("[StatTracker] Failed to get required systems for DiscordBotManager", LogLevel.ERROR);
            return;
        }
        
        m_Logger.LogInfo("Initializing Discord Bot Manager");
        
        // Create data directory if it doesn't exist
        string dir = "$profile:StatTracker/Discord";
        FileIO.MakeDirectory(dir);
        
        // Load configuration
        LoadConfiguration();
        
        // Load registered webhooks
        LoadWebhooks();
        
        // Register available commands
        RegisterAvailableCommands();
        
        // Start processing timer
        GetGame().GetCallqueue().CallLater(Update, 5000, true); // Process every 5 seconds
        
        m_Logger.LogInfo("Discord Bot Manager initialized" + (m_bEnabled ? " and enabled" : " but disabled"));
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_DiscordBotManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_DiscordBotManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Update function called periodically
    void Update()
    {
        if (!m_bEnabled)
            return;
            
        float currentTime = System.GetTickCount() / 1000.0;
        
        // Process queues at interval
        if (currentTime - m_fLastProcessTime >= PROCESS_INTERVAL)
        {
            // Process outgoing events
            ProcessEventQueue();
            
            // Process incoming commands
            ProcessCommandQueue();
            
            m_fLastProcessTime = currentTime;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Enable or disable the Discord bot
    void SetEnabled(bool enabled)
    {
        m_bEnabled = enabled;
        
        // Save configuration
        m_Config.SetSetting("Discord", "Enabled", enabled.ToString());
        m_Config.SaveConfig();
        
        m_Logger.LogInfo("Discord bot " + (enabled ? "enabled" : "disabled"));
    }
    
    //------------------------------------------------------------------------------------------------
    // Set Discord bot token
    void SetBotToken(string token)
    {
        m_sBotToken = token;
        
        // Save configuration
        m_Config.SetSetting("Discord", "BotToken", token);
        m_Config.SaveConfig();
        
        m_Logger.LogInfo("Discord bot token updated");
    }
    
    //------------------------------------------------------------------------------------------------
    // Set Discord guild ID
    void SetGuildID(string guildID)
    {
        m_sGuildID = guildID;
        
        // Save configuration
        m_Config.SetSetting("Discord", "GuildID", guildID);
        m_Config.SaveConfig();
        
        m_Logger.LogInfo("Discord guild ID updated to " + guildID);
    }
    
    //------------------------------------------------------------------------------------------------
    // Set command prefix
    void SetCommandPrefix(string prefix)
    {
        m_sCommandPrefix = prefix;
        
        // Save configuration
        m_Config.SetSetting("Discord", "CommandPrefix", prefix);
        m_Config.SaveConfig();
        
        m_Logger.LogInfo("Discord command prefix updated to " + prefix);
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a webhook
    void AddWebhook(string name, string url, array<string> eventTypes = null)
    {
        // Check if webhook already exists
        for (int i = 0; i < m_aWebhooks.Count(); i++)
        {
            if (m_aWebhooks[i].m_sName == name)
            {
                // Update existing webhook
                m_aWebhooks[i].m_sURL = url;
                
                if (eventTypes)
                    m_aWebhooks[i].m_aEventTypes = eventTypes;
                    
                m_Logger.LogInfo("Updated Discord webhook: " + name);
                SaveWebhooks();
                return;
            }
        }
        
        // Add new webhook
        STS_DiscordWebhook webhook = new STS_DiscordWebhook(name, url, eventTypes);
        m_aWebhooks.Insert(webhook);
        
        m_Logger.LogInfo("Added new Discord webhook: " + name);
        SaveWebhooks();
    }
    
    //------------------------------------------------------------------------------------------------
    // Remove a webhook
    void RemoveWebhook(string name)
    {
        for (int i = 0; i < m_aWebhooks.Count(); i++)
        {
            if (m_aWebhooks[i].m_sName == name)
            {
                m_aWebhooks.Remove(i);
                m_Logger.LogInfo("Removed Discord webhook: " + name);
                SaveWebhooks();
                return;
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Queue an event for Discord
    void QueueEvent(string type, string message, map<string, string> data = null)
    {
        if (!m_bEnabled)
            return;
            
        STS_DiscordEvent event = new STS_DiscordEvent(type, message, data);
        m_aEventQueue.Insert(event);
        
        // Process immediately if there are no other events in the queue
        if (m_aEventQueue.Count() == 1)
        {
            GetGame().GetCallqueue().CallLater(ProcessEventQueue, 100, false);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Process the event queue
    protected void ProcessEventQueue()
    {
        m_Logger.LogDebug("Processing Discord event queue: " + m_aEventQueue.Count() + " events");
        
        // Process a limited number of events at once to avoid delays
        int processCount = Math.Min(10, m_aEventQueue.Count());
        
        for (int i = 0; i < processCount; i++)
        {
            if (m_aEventQueue.Count() == 0)
                break;
                
            STS_DiscordEvent event = m_aEventQueue[0];
            m_aEventQueue.Remove(0);
            
            // Process event
            SendEventToDiscord(event);
        }
        
        // If there are still events in the queue, schedule another processing
        if (m_aEventQueue.Count() > 0)
        {
            GetGame().GetCallqueue().CallLater(ProcessEventQueue, 1000, false);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Send an event to Discord
    protected void SendEventToDiscord(STS_DiscordEvent event)
    {
        m_Logger.LogDebug("Sending Discord event: " + event.m_sType + " - " + event.m_sMessage);
        
        // Check which webhooks should receive this event
        foreach (STS_DiscordWebhook webhook : m_aWebhooks)
        {
            if (webhook.ShouldHandleEvent(event.m_sType))
            {
                SendWebhookMessage(webhook, event);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a message via Discord webhook
    protected void SendWebhookMessage(STS_DiscordWebhook webhook, STS_DiscordEvent event)
    {
        // In a real implementation, this would make an HTTP POST request to the webhook URL
        // with properly formatted Discord webhook payload
        // For now, we'll log that we would send this message
        
        m_Logger.LogInfo(string.Format("Would send Discord webhook message to '%1': [%2] %3", 
            webhook.m_sName, event.m_sType, event.m_sMessage));
            
        // Simulate success/failure
        bool simulatedSuccess = Math.RandomInt(0, 10) < 9; // 90% chance of success
        
        if (!simulatedSuccess)
        {
            m_Logger.LogWarning("Failed to send Discord webhook message to " + webhook.m_sName);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Process the command queue
    protected void ProcessCommandQueue()
    {
        // In a real implementation, this would poll the Discord API
        // or use a websocket connection to receive commands
        // For now, we'll simulate receiving commands
        
        SimulateReceivingCommands();
        
        m_Logger.LogDebug("Processing Discord command queue: " + m_aCommandQueue.Count() + " commands");
        
        // Process all commands in queue
        while (m_aCommandQueue.Count() > 0)
        {
            map<string, string> commandData = m_aCommandQueue[0];
            m_aCommandQueue.Remove(0);
            
            // Process command
            ProcessCommand(commandData);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Process a command
    protected void ProcessCommand(map<string, string> commandData)
    {
        if (!commandData.Contains("command") || !commandData.Contains("user"))
        {
            m_Logger.LogWarning("Invalid Discord command data received");
            return;
        }
        
        string command = commandData.Get("command");
        string user = commandData.Get("user");
        string channel = commandData.Contains("channel") ? commandData.Get("channel") : "";
        string args = commandData.Contains("args") ? commandData.Get("args") : "";
        
        m_Logger.LogInfo(string.Format("Processing Discord command from %1: %2 %3", 
            user, command, args));
            
        // Check if we have handlers for this command
        if (!m_mCommandHandlers.Contains(command))
        {
            // Send response back to Discord
            SendResponseToDiscord(channel, "Unknown command: " + command + ". Type !help for available commands.");
            return;
        }
        
        // Get handler for this command
        array<ref Param3<int, int, string>> handlers = m_mCommandHandlers.Get(command);
        
        if (handlers.Count() == 0)
        {
            SendResponseToDiscord(channel, "No handlers registered for command: " + command);
            return;
        }
        
        // Execute all handlers for this command
        foreach (Param3<int, int, string> handler : handlers)
        {
            // In a real implementation, each handler would be a method reference
            // For now, we'll simulate handlers with simple message responses
            
            string response = string.Format("Executed handler %1 for command %2 with args: %3", 
                handler.param1, command, args);
                
            SendResponseToDiscord(channel, response);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Send a response back to Discord
    protected void SendResponseToDiscord(string channel, string message)
    {
        // In a real implementation, this would send a message to a Discord channel
        // using the bot API
        // For now, we'll log that we would send this message
        
        m_Logger.LogInfo(string.Format("Would send Discord response to channel '%1': %2", 
            channel, message));
    }
    
    //------------------------------------------------------------------------------------------------
    // Simulate receiving commands from Discord
    protected void SimulateReceivingCommands()
    {
        // Only simulate commands occasionally
        if (Math.RandomInt(0, 10) < 9) // 90% chance of not receiving a command
            return;
            
        // Create a simulated command
        string[] possibleCommands = {"help", "status", "players", "kick", "ban", "restart"};
        string[] possibleUsers = {"Admin#1234", "Moderator#5678", "Player#9012"};
        
        int commandIndex = Math.RandomInt(0, possibleCommands.Length - 1);
        int userIndex = Math.RandomInt(0, possibleUsers.Length - 1);
        
        map<string, string> commandData = new map<string, string>();
        commandData.Set("command", possibleCommands[commandIndex]);
        commandData.Set("user", possibleUsers[userIndex]);
        commandData.Set("channel", "admin-commands");
        
        // Add args for certain commands
        if (commandData.Get("command") == "kick" || commandData.Get("command") == "ban")
        {
            commandData.Set("args", "Player123 Breaking server rules");
        }
        
        // Add to queue
        m_aCommandQueue.Insert(commandData);
        
        m_Logger.LogDebug("Simulated receiving Discord command: " + commandData.Get("command"));
    }
    
    //------------------------------------------------------------------------------------------------
    // Register a command handler
    void RegisterCommandHandler(string command, int handlerID, int priority, string description)
    {
        if (!m_mCommandHandlers.Contains(command))
        {
            m_mCommandHandlers.Set(command, new array<ref Param3<int, int, string>>());
        }
        
        array<ref Param3<int, int, string>> handlers = m_mCommandHandlers.Get(command);
        
        // Check if handler already exists
        foreach (Param3<int, int, string> handler : handlers)
        {
            if (handler.param1 == handlerID)
            {
                m_Logger.LogWarning("Handler " + handlerID.ToString() + " already registered for command " + command);
                return;
            }
        }
        
        // Add handler
        handlers.Insert(new Param3<int, int, string>(handlerID, priority, description));
        
        // Sort by priority (higher priority first)
        handlers.Sort(HandlerPriorityComparer);
        
        m_Logger.LogInfo(string.Format("Registered handler %1 for Discord command %2 with priority %3", 
            handlerID, command, priority));
    }
    
    //------------------------------------------------------------------------------------------------
    // Handler priority comparer
    static int HandlerPriorityComparer(Param3<int, int, string> a, Param3<int, int, string> b)
    {
        if (a.param2 > b.param2) return -1; // Higher priority first
        if (a.param2 < b.param2) return 1;
        return 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load configuration
    protected void LoadConfiguration()
    {
        // Load Discord bot settings from config
        m_bEnabled = m_Config.GetSetting("Discord", "Enabled", "false") == "true";
        m_sBotToken = m_Config.GetSetting("Discord", "BotToken", "");
        m_sGuildID = m_Config.GetSetting("Discord", "GuildID", "");
        m_sCommandPrefix = m_Config.GetSetting("Discord", "CommandPrefix", "!");
        
        m_Logger.LogInfo(string.Format("Loaded Discord configuration: Enabled: %1, Token: %2, Guild: %3, Prefix: %4", 
            m_bEnabled, m_sBotToken.IsEmpty() ? "Not set" : "Set", m_sGuildID, m_sCommandPrefix));
    }
    
    //------------------------------------------------------------------------------------------------
    // Save webhooks configuration
    protected void SaveWebhooks()
    {
        string json = "[";
        
        for (int i = 0; i < m_aWebhooks.Count(); i++)
        {
            json += m_aWebhooks[i].ToJSON();
            
            if (i < m_aWebhooks.Count() - 1)
                json += ",";
        }
        
        json += "]";
        
        // Write to file
        FileHandle file = FileIO.OpenFile(WEBHOOKS_CONFIG_PATH, FileMode.WRITE);
        if (file != 0)
        {
            FileIO.WriteFile(file, json);
            FileIO.CloseFile(file);
            m_Logger.LogDebug("Saved Discord webhooks configuration");
        }
        else
        {
            m_Logger.LogError("Failed to save Discord webhooks configuration");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load webhooks configuration
    protected void LoadWebhooks()
    {
        if (!FileIO.FileExists(WEBHOOKS_CONFIG_PATH))
        {
            m_Logger.LogInfo("No Discord webhooks configuration found");
            return;
        }
        
        string json = ReadFileContent(WEBHOOKS_CONFIG_PATH);
        if (json.IsEmpty())
        {
            m_Logger.LogError("Failed to read Discord webhooks configuration");
            return;
        }
        
        try
        {
            JsonSerializer js = new JsonSerializer();
            string error;
            
            // Parse the JSON data
            ref array<string> webhookJsons = new array<string>();
            
            bool success = js.ReadFromString(webhookJsons, json, error);
            if (!success)
            {
                m_Logger.LogError("Failed to parse Discord webhooks configuration: " + error);
                return;
            }
            
            m_aWebhooks.Clear();
            
            foreach (string webhookJson : webhookJsons)
            {
                STS_DiscordWebhook webhook = STS_DiscordWebhook.FromJSON(webhookJson);
                if (webhook)
                {
                    m_aWebhooks.Insert(webhook);
                }
            }
            
            m_Logger.LogInfo("Loaded " + m_aWebhooks.Count() + " Discord webhooks");
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception loading Discord webhooks configuration: " + e.ToString());
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Register available commands
    protected void RegisterAvailableCommands()
    {
        // Clear existing commands
        m_aAvailableCommands.Clear();
        
        // Help command
        array<string> helpParams = new array<string>();
        helpParams.Insert("command");
        m_aAvailableCommands.Insert(new STS_DiscordCommand("help", "Show available commands", helpParams));
        
        // Status command
        m_aAvailableCommands.Insert(new STS_DiscordCommand("status", "Show server status"));
        
        // Players command
        m_aAvailableCommands.Insert(new STS_DiscordCommand("players", "List online players"));
        
        // Kick command
        array<string> kickParams = new array<string>();
        kickParams.Insert("player");
        kickParams.Insert("reason");
        m_aAvailableCommands.Insert(new STS_DiscordCommand("kick", "Kick a player from the server", kickParams));
        
        // Ban command
        array<string> banParams = new array<string>();
        banParams.Insert("player");
        banParams.Insert("duration");
        banParams.Insert("reason");
        m_aAvailableCommands.Insert(new STS_DiscordCommand("ban", "Ban a player from the server", banParams));
        
        // Restart command
        array<string> restartParams = new array<string>();
        restartParams.Insert("delay");
        m_aAvailableCommands.Insert(new STS_DiscordCommand("restart", "Restart the server", restartParams));
        
        // Save commands configuration
        SaveCommands();
        
        m_Logger.LogInfo("Registered " + m_aAvailableCommands.Count() + " Discord commands");
    }
    
    //------------------------------------------------------------------------------------------------
    // Save commands configuration
    protected void SaveCommands()
    {
        string json = "[";
        
        for (int i = 0; i < m_aAvailableCommands.Count(); i++)
        {
            json += m_aAvailableCommands[i].ToJSON();
            
            if (i < m_aAvailableCommands.Count() - 1)
                json += ",";
        }
        
        json += "]";
        
        // Write to file
        FileHandle file = FileIO.OpenFile(COMMANDS_CONFIG_PATH, FileMode.WRITE);
        if (file != 0)
        {
            FileIO.WriteFile(file, json);
            FileIO.CloseFile(file);
            m_Logger.LogDebug("Saved Discord commands configuration");
        }
        else
        {
            m_Logger.LogError("Failed to save Discord commands configuration");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper method to read file content
    protected string ReadFileContent(string filePath)
    {
        string content = "";
        
        FileHandle file = FileIO.OpenFile(filePath, FileMode.READ);
        if (file != 0)
        {
            content = FileIO.ReadFile(file);
            FileIO.CloseFile(file);
        }
        
        return content;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get Discord bot status
    string GetBotStatus()
    {
        string status = "";
        
        status += "Discord Bot Status:\n";
        status += "----------------------------------\n";
        status += "Enabled: " + (m_bEnabled ? "Yes" : "No") + "\n";
        status += "Connected: " + (m_bConnected ? "Yes" : "No") + "\n";
        status += "Bot Token: " + (m_sBotToken.IsEmpty() ? "Not set" : "Set") + "\n";
        status += "Guild ID: " + (m_sGuildID.IsEmpty() ? "Not set" : m_sGuildID) + "\n";
        status += "Command Prefix: " + m_sCommandPrefix + "\n";
        status += "Commands: " + m_aAvailableCommands.Count() + "\n";
        status += "Webhooks: " + m_aWebhooks.Count() + "\n";
        status += "----------------------------------\n";
        
        // List webhooks
        if (m_aWebhooks.Count() > 0)
        {
            status += "\nRegistered Webhooks:\n";
            
            foreach (STS_DiscordWebhook webhook : m_aWebhooks)
            {
                status += "- " + webhook.m_sName + ": ";
                
                if (webhook.m_aEventTypes.Count() == 0)
                {
                    status += "All events\n";
                }
                else
                {
                    status += "Events: ";
                    for (int i = 0; i < webhook.m_aEventTypes.Count(); i++)
                    {
                        status += webhook.m_aEventTypes[i];
                        if (i < webhook.m_aEventTypes.Count() - 1)
                            status += ", ";
                    }
                    status += "\n";
                }
            }
        }
        
        return status;
    }
} 