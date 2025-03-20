// STS_AdminDashboard.c
// Web-based admin dashboard for the StatTracker system

class STS_AdminDashboard
{
    // Singleton instance
    private static ref STS_AdminDashboard s_Instance;
    
    // Config reference
    protected STS_Config m_Config;
    
    // API server reference
    protected STS_APIServer m_APIServer;
    
    // Dashboard routes
    static const string ROUTE_DASHBOARD = "/admin/dashboard";
    static const string ROUTE_PLAYERS = "/admin/players";
    static const string ROUTE_PLAYER = "/admin/player/:id";
    static const string ROUTE_STATS = "/admin/stats";
    static const string ROUTE_COMMAND = "/admin/command";
    static const string ROUTE_LEADERBOARD = "/admin/leaderboard/:stat";
    static const string ROUTE_LIVE = "/admin/live";
    
    // Dashboard files path
    protected const string DASHBOARD_FILES_PATH = "$profile:StatTracker/Dashboard/";
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_AdminDashboard()
    {
        m_Config = STS_Config.GetInstance();
        m_APIServer = STS_APIServer.GetInstance();
        
        // Register dashboard routes
        RegisterDashboardRoutes();
        
        // Extract dashboard files if they don't exist
        EnsureDashboardFilesExist();
        
        Print("[StatTracker] Admin Dashboard initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_AdminDashboard GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_AdminDashboard();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Register dashboard routes with the API server
    protected void RegisterDashboardRoutes()
    {
        // Main dashboard page
        m_APIServer.RegisterRoute("GET", ROUTE_DASHBOARD, this, "HandleDashboardRequest");
        
        // Player list
        m_APIServer.RegisterRoute("GET", ROUTE_PLAYERS, this, "HandlePlayersRequest");
        
        // Single player details
        m_APIServer.RegisterRoute("GET", ROUTE_PLAYER, this, "HandlePlayerRequest");
        
        // Server statistics
        m_APIServer.RegisterRoute("GET", ROUTE_STATS, this, "HandleStatsRequest");
        
        // Admin command endpoint
        m_APIServer.RegisterRoute("POST", ROUTE_COMMAND, this, "HandleCommandRequest");
        
        // Leaderboard
        m_APIServer.RegisterRoute("GET", ROUTE_LEADERBOARD, this, "HandleLeaderboardRequest");
        
        // Live data (WebSocket/SSE)
        m_APIServer.RegisterRoute("GET", ROUTE_LIVE, this, "HandleLiveDataRequest");
    }
    
    //------------------------------------------------------------------------------------------------
    // Ensure dashboard web files exist
    protected void EnsureDashboardFilesExist()
    {
        // Check if dashboard directory exists
        if (!FileExist(DASHBOARD_FILES_PATH))
        {
            // Create directory
            MakeDirectory(DASHBOARD_FILES_PATH);
            
            // Extract dashboard files
            ExtractDashboardFiles();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Extract dashboard web files (placeholder)
    protected void ExtractDashboardFiles()
    {
        // This is a placeholder. In a real implementation, you would extract
        // the dashboard web files from the mod package to the profile directory.
        
        // Create a basic HTML file
        CreateBasicDashboardFile();
    }
    
    //------------------------------------------------------------------------------------------------
    // Create a basic dashboard HTML file
    protected void CreateBasicDashboardFile()
    {
        string htmlContent = 
@"<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Stat Tracker - Admin Dashboard</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #121212;
            color: #eee;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        header {
            background-color: #1a1a1a;
            padding: 10px 20px;
            border-bottom: 1px solid #333;
        }
        h1 {
            margin: 0;
            color: #fff;
        }
        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
            gap: 20px;
            margin-top: 20px;
        }
        .card {
            background-color: #1e1e1e;
            border-radius: 5px;
            padding: 20px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }
        .card h2 {
            margin-top: 0;
            border-bottom: 1px solid #333;
            padding-bottom: 10px;
            color: #fff;
        }
        table {
            width: 100%;
            border-collapse: collapse;
        }
        th, td {
            padding: 10px;
            text-align: left;
            border-bottom: 1px solid #333;
        }
        th {
            background-color: #252525;
        }
        .button {
            background-color: #3498db;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            cursor: pointer;
        }
        .button:hover {
            background-color: #2980b9;
        }
        input, textarea {
            background-color: #252525;
            border: 1px solid #333;
            color: #eee;
            padding: 8px;
            border-radius: 4px;
            width: 100%;
            margin-bottom: 10px;
        }
    </style>
</head>
<body>
    <header>
        <h1>Stat Tracker - Admin Dashboard</h1>
    </header>
    
    <div class='container'>
        <div class='dashboard'>
            <div class='card'>
                <h2>Server Overview</h2>
                <div id='server-stats'>
                    <p>Players Online: <span id='online-players'>Loading...</span></p>
                    <p>Server Uptime: <span id='server-uptime'>Loading...</span></p>
                    <p>Last Restart: <span id='last-restart'>Loading...</span></p>
                </div>
            </div>
            
            <div class='card'>
                <h2>Online Players</h2>
                <div id='player-list'>
                    <table>
                        <thead>
                            <tr>
                                <th>Name</th>
                                <th>Playtime</th>
                                <th>K/D</th>
                                <th>Actions</th>
                            </tr>
                        </thead>
                        <tbody id='player-table-body'>
                            <tr>
                                <td colspan='4'>Loading players...</td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
            
            <div class='card'>
                <h2>Top Killers</h2>
                <div id='top-killers'>
                    <table>
                        <thead>
                            <tr>
                                <th>#</th>
                                <th>Player</th>
                                <th>Kills</th>
                            </tr>
                        </thead>
                        <tbody id='killers-table-body'>
                            <tr>
                                <td colspan='3'>Loading leaderboard...</td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
            
            <div class='card'>
                <h2>Admin Actions</h2>
                <div id='admin-actions'>
                    <textarea id='message-input' placeholder='Enter message or command'></textarea>
                    <button class='button' id='send-message'>Send Message</button>
                    <button class='button' id='send-announcement'>Send Announcement</button>
                    <button class='button' id='restart-server'>Restart Server</button>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        // Basic dashboard functionality
        document.addEventListener('DOMContentLoaded', function() {
            // Load initial data
            fetchServerData();
            fetchOnlinePlayers();
            fetchTopKillers();
            
            // Set up periodic refresh
            setInterval(fetchServerData, 30000);
            setInterval(fetchOnlinePlayers, 15000);
            
            // Set up action buttons
            document.getElementById('send-message').addEventListener('click', sendMessage);
            document.getElementById('send-announcement').addEventListener('click', sendAnnouncement);
            document.getElementById('restart-server').addEventListener('click', confirmRestart);
        });
        
        function fetchServerData() {
            fetch('/admin/stats')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('online-players').textContent = data.players_online;
                    document.getElementById('server-uptime').textContent = data.uptime;
                    document.getElementById('last-restart').textContent = data.last_restart;
                })
                .catch(error => console.error('Error fetching server data:', error));
        }
        
        function fetchOnlinePlayers() {
            fetch('/admin/players')
                .then(response => response.json())
                .then(data => {
                    const tableBody = document.getElementById('player-table-body');
                    tableBody.innerHTML = '';
                    
                    if (data.players.length === 0) {
                        const row = document.createElement('tr');
                        row.innerHTML = '<td colspan='4'>No players online</td>';
                        tableBody.appendChild(row);
                        return;
                    }
                    
                    data.players.forEach(player => {
                        const row = document.createElement('tr');
                        row.innerHTML = `
                            <td>${player.name}</td>
                            <td>${player.playtime}</td>
                            <td>${player.kills}/${player.deaths}</td>
                            <td>
                                <button class='button' onclick='viewPlayer(\"${player.id}\")'>View</button>
                                <button class='button' onclick='kickPlayer(\"${player.id}\")'>Kick</button>
                            </td>
                        `;
                        tableBody.appendChild(row);
                    });
                })
                .catch(error => console.error('Error fetching players:', error));
        }
        
        function fetchTopKillers() {
            fetch('/admin/leaderboard/kills')
                .then(response => response.json())
                .then(data => {
                    const tableBody = document.getElementById('killers-table-body');
                    tableBody.innerHTML = '';
                    
                    if (data.entries.length === 0) {
                        const row = document.createElement('tr');
                        row.innerHTML = '<td colspan='3'>No data available</td>';
                        tableBody.appendChild(row);
                        return;
                    }
                    
                    data.entries.forEach((entry, index) => {
                        const row = document.createElement('tr');
                        row.innerHTML = `
                            <td>${index + 1}</td>
                            <td>${entry.player_name}</td>
                            <td>${entry.value}</td>
                        `;
                        tableBody.appendChild(row);
                    });
                })
                .catch(error => console.error('Error fetching leaderboard:', error));
        }
        
        function viewPlayer(playerId) {
            window.location.href = `/admin/player/${playerId}`;
        }
        
        function kickPlayer(playerId) {
            const reason = prompt('Enter kick reason:');
            if (reason === null) return;
            
            sendCommand('kick', { player: playerId, reason: reason });
        }
        
        function sendMessage() {
            const message = document.getElementById('message-input').value;
            if (!message) {
                alert('Please enter a message');
                return;
            }
            
            sendCommand('message', { target: 'all', text: message });
            document.getElementById('message-input').value = '';
        }
        
        function sendAnnouncement() {
            const message = document.getElementById('message-input').value;
            if (!message) {
                alert('Please enter a message');
                return;
            }
            
            sendCommand('announce', { target: 'all', text: message });
            document.getElementById('message-input').value = '';
        }
        
        function confirmRestart() {
            if (confirm('Are you sure you want to restart the server?')) {
                sendCommand('restart', {});
            }
        }
        
        function sendCommand(command, params) {
            fetch('/admin/command', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    command: command,
                    params: params
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    alert('Command executed successfully');
                } else {
                    alert('Error: ' + data.error);
                }
            })
            .catch(error => {
                console.error('Error sending command:', error);
                alert('Error sending command');
            });
        }
    </script>
</body>
</html>";

        // Save the file
        string filePath = DASHBOARD_FILES_PATH + "index.html";
        FileHandle file = OpenFile(filePath, FileMode.WRITE);
        if (file)
        {
            FPrint(file, htmlContent);
            CloseFile(file);
            Print("[StatTracker] Created basic dashboard file at: " + filePath);
        }
        else
        {
            Print("[StatTracker] ERROR: Could not create dashboard file at: " + filePath);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle main dashboard page request
    void HandleDashboardRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check authorization
        if (!ValidateAdminAuth(request, response))
            return;
            
        // Serve the dashboard HTML file
        string filePath = DASHBOARD_FILES_PATH + "index.html";
        if (FileExist(filePath))
        {
            string html = LoadFileContent(filePath);
            
            response.SetHeader("Content-Type", "text/html");
            response.SetData(html);
            response.SetStatusCode(200);
        }
        else
        {
            response.SetHeader("Content-Type", "text/html");
            response.SetData("<html><body><h1>Dashboard not found</h1><p>The dashboard files have not been installed.</p></body></html>");
            response.SetStatusCode(404);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle player list request
    void HandlePlayersRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check authorization
        if (!ValidateAdminAuth(request, response))
            return;
            
        // Get all online players
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // Create response data
        map<string, Managed> responseData = new map<string, Managed>();
        array<map<string, Managed>> playerList = new array<map<string, Managed>>();
        
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
            
            // Get player stats
            STS_EnhancedPlayerStats stats = STS_PersistenceManager.GetInstance().LoadPlayerStats(playerId);
            if (!stats)
                continue;
                
            // Calculate session time
            int sessionTime = 0;
            if (player.m_SessionStartTime > 0)
                sessionTime = System.GetUnixTime() - player.m_SessionStartTime;
                
            // Create player data
            map<string, Managed> playerData = new map<string, Managed>();
            playerData.Insert("id", playerId);
            playerData.Insert("name", playerName);
            playerData.Insert("kills", stats.m_iKills);
            playerData.Insert("deaths", stats.m_iDeaths);
            playerData.Insert("playtime", FormatPlaytime(sessionTime));
            playerData.Insert("position", player.GetPosition().ToString());
            playerData.Insert("health", player.GetHealth("", "") * 100);
            
            // Add to list
            playerList.Insert(playerData);
        }
        
        // Add player list to response
        responseData.Insert("players", playerList);
        responseData.Insert("count", playerList.Count());
        
        // Send response
        SendJsonResponse(response, 200, responseData);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle single player request
    void HandlePlayerRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check authorization
        if (!ValidateAdminAuth(request, response))
            return;
            
        // Get player ID from path
        string playerId = request.GetPathParam("id");
        if (!playerId)
        {
            SendErrorResponse(response, 400, "Player ID is required");
            return;
        }
        
        // Load player stats
        STS_EnhancedPlayerStats stats = STS_PersistenceManager.GetInstance().LoadPlayerStats(playerId);
        if (!stats)
        {
            SendErrorResponse(response, 404, "Player not found");
            return;
        }
        
        // Create response data
        map<string, Managed> responseData = new map<string, Managed>();
        responseData.Insert("id", playerId);
        responseData.Insert("name", stats.m_sPlayerName);
        responseData.Insert("kills", stats.m_iKills);
        responseData.Insert("deaths", stats.m_iDeaths);
        responseData.Insert("headshots", stats.m_iHeadshotKills);
        responseData.Insert("kd_ratio", stats.m_iDeaths > 0 ? (float)stats.m_iKills / stats.m_iDeaths : stats.m_iKills);
        responseData.Insert("damage_dealt", stats.m_fDamageDealt);
        responseData.Insert("damage_taken", stats.m_fDamageTaken);
        responseData.Insert("longest_kill", stats.m_fLongestKill);
        responseData.Insert("total_playtime", stats.m_iTotalPlaytimeSeconds);
        responseData.Insert("first_login", stats.m_iFirstLogin);
        responseData.Insert("last_login", stats.m_iLastLogin);
        responseData.Insert("sessions", stats.m_iTotalSessions);
        
        // Check if player is online
        bool isOnline = false;
        string position = "";
        float health = 0;
        
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
                
            if (identity.GetId() == playerId)
            {
                isOnline = true;
                position = player.GetPosition().ToString();
                health = player.GetHealth("", "") * 100;
                break;
            }
        }
        
        responseData.Insert("online", isOnline);
        if (isOnline)
        {
            responseData.Insert("position", position);
            responseData.Insert("health", health);
        }
        
        // Send response
        SendJsonResponse(response, 200, responseData);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle server stats request
    void HandleStatsRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check authorization
        if (!ValidateAdminAuth(request, response))
            return;
            
        // Get all online players
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // Get server stats
        float uptime = GetGame().GetTickTime() / 1000;
        int timestamp = System.GetUnixTime();
        
        // Create response data
        map<string, Managed> responseData = new map<string, Managed>();
        responseData.Insert("players_online", players.Count());
        responseData.Insert("uptime", FormatPlaytime(uptime));
        responseData.Insert("uptime_seconds", uptime);
        responseData.Insert("server_time", timestamp);
        responseData.Insert("server_time_formatted", FormatTimestamp(timestamp));
        
        // This would need to be stored somewhere to be accurate
        responseData.Insert("last_restart", FormatTimestamp(timestamp - uptime));
        
        // Get all player IDs
        array<string> playerIds = STS_PersistenceManager.GetInstance().GetAllPlayerIds();
        
        // Total stats
        int totalPlayers = playerIds.Count();
        int totalKills = 0;
        int totalDeaths = 0;
        int totalHeadshots = 0;
        int totalPlaytime = 0;
        
        foreach (string pid : playerIds)
        {
            STS_EnhancedPlayerStats stats = STS_PersistenceManager.GetInstance().LoadPlayerStats(pid);
            if (!stats)
                continue;
                
            totalKills += stats.m_iKills;
            totalDeaths += stats.m_iDeaths;
            totalHeadshots += stats.m_iHeadshotKills;
            totalPlaytime += stats.m_iTotalPlaytimeSeconds;
        }
        
        // Add total stats to response
        responseData.Insert("total_players", totalPlayers);
        responseData.Insert("total_kills", totalKills);
        responseData.Insert("total_deaths", totalDeaths);
        responseData.Insert("total_headshots", totalHeadshots);
        responseData.Insert("total_playtime", totalPlaytime);
        responseData.Insert("total_playtime_formatted", FormatPlaytime(totalPlaytime));
        
        // Send response
        SendJsonResponse(response, 200, responseData);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle admin command request
    void HandleCommandRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check authorization
        if (!ValidateAdminAuth(request, response))
            return;
            
        // Get command data from request body
        string body = request.GetBody();
        map<string, Managed> commandData = new map<string, Managed>();
        
        JsonSerializer serializer = new JsonSerializer();
        if (!serializer.ReadFromString(commandData, body))
        {
            SendErrorResponse(response, 400, "Invalid JSON");
            return;
        }
        
        // Extract command and parameters
        string command = commandData.Get("command");
        map<string, Managed> params = commandData.Get("params");
        
        if (!command)
        {
            SendErrorResponse(response, 400, "Command is required");
            return;
        }
        
        // Handle different commands
        string result = "Unknown command";
        bool success = false;
        
        if (command == "message")
        {
            // Send a message to players
            string target = params.Get("target");
            string text = params.Get("text");
            
            if (!target || !text)
            {
                SendErrorResponse(response, 400, "Target and text are required");
                return;
            }
            
            // Process RCON command
            ProcessRconCommand(STS_RCONCommands.CMD_MSG, target, text);
            
            result = "Message sent to " + target;
            success = true;
        }
        else if (command == "announce")
        {
            // Send an announcement to players
            string target = params.Get("target");
            string text = params.Get("text");
            
            if (!target || !text)
            {
                SendErrorResponse(response, 400, "Target and text are required");
                return;
            }
            
            // Process RCON command
            ProcessRconCommand(STS_RCONCommands.CMD_ANNOUNCE, target, text);
            
            result = "Announcement sent to " + target;
            success = true;
        }
        else if (command == "kick")
        {
            // Kick a player
            string player = params.Get("player");
            string reason = params.Get("reason");
            
            if (!player)
            {
                SendErrorResponse(response, 400, "Player ID is required");
                return;
            }
            
            if (!reason)
                reason = "Kicked by admin";
                
            // Process RCON command
            ProcessRconCommand(STS_RCONCommands.CMD_KICK, player, reason);
            
            result = "Player kicked: " + player;
            success = true;
        }
        else if (command == "ban")
        {
            // Ban a player
            string player = params.Get("player");
            int duration = params.Get("duration");
            string reason = params.Get("reason");
            
            if (!player)
            {
                SendErrorResponse(response, 400, "Player ID is required");
                return;
            }
            
            if (!reason)
                reason = "Banned by admin";
                
            // Process RCON command
            ProcessRconCommand(STS_RCONCommands.CMD_BAN, player, duration.ToString(), reason);
            
            result = "Player banned: " + player;
            success = true;
        }
        else if (command == "restart")
        {
            // Restart the server
            // This would typically need server-specific implementation
            
            // Notify players first
            ProcessRconCommand(STS_RCONCommands.CMD_ANNOUNCE, "all", "Server is restarting in 2 minutes. Please log out safely.");
            
            // Schedule restart (placeholder)
            GetGame().GetCallqueue().CallLater(RestartServer, 120000, false);
            
            result = "Server restart scheduled in 2 minutes";
            success = true;
        }
        
        // Create response data
        map<string, Managed> responseData = new map<string, Managed>();
        responseData.Insert("success", success);
        responseData.Insert("result", result);
        
        // Send response
        SendJsonResponse(response, 200, responseData);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle leaderboard request
    void HandleLeaderboardRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check authorization
        if (!ValidateAdminAuth(request, response))
            return;
            
        // Get stat type from path
        string statType = request.GetPathParam("stat");
        if (!statType)
        {
            SendErrorResponse(response, 400, "Stat type is required");
            return;
        }
        
        // Get count parameter
        int count = 10;
        string countParam = request.GetQueryParam("count");
        if (countParam)
            count = countParam.ToInt();
            
        // Get leaderboard entries
        array<ref STS_LeaderboardEntry> leaderboard = STS_PersistenceManager.GetInstance().GetTopPlayers(statType, count);
        if (!leaderboard)
        {
            SendErrorResponse(response, 404, "Leaderboard not found");
            return;
        }
        
        // Create response data
        map<string, Managed> responseData = new map<string, Managed>();
        responseData.Insert("stat", statType);
        responseData.Insert("count", leaderboard.Count());
        
        // Convert leaderboard entries
        array<map<string, Managed>> entries = new array<map<string, Managed>>();
        
        foreach (STS_LeaderboardEntry entry : leaderboard)
        {
            map<string, Managed> entryData = new map<string, Managed>();
            entryData.Insert("player_id", entry.m_sPlayerId);
            entryData.Insert("player_name", entry.m_sPlayerName);
            entryData.Insert("value", entry.m_fValue);
            
            entries.Insert(entryData);
        }
        
        responseData.Insert("entries", entries);
        
        // Send response
        SendJsonResponse(response, 200, responseData);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle live data request (placeholder)
    void HandleLiveDataRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check authorization
        if (!ValidateAdminAuth(request, response))
            return;
            
        // This would typically use Server-Sent Events or WebSockets
        // As a placeholder, we'll return a message
        
        response.SetHeader("Content-Type", "text/plain");
        response.SetData("Live data streaming is not implemented in this version");
        response.SetStatusCode(501); // Not Implemented
    }
    
    //------------------------------------------------------------------------------------------------
    // Process RCON command
    protected void ProcessRconCommand(string command, string param1, string param2 = "", string param3 = "")
    {
        // Create parameter array
        array<string> params = new array<string>();
        params.Insert(param1);
        
        if (param2.Length() > 0)
            params.Insert(param2);
            
        if (param3.Length() > 0)
            params.Insert(param3);
            
        // Get RCON command handler
        STS_RCONCommands rconCommands = STS_RCONCommands.GetInstance();
        
        // Create command data
        Param2<string, array<string>> data = new Param2<string, array<string>>(command, params);
        
        // Process command directly
        rconCommands.OnRconCommand(CallType.Server, NULL, NULL, NULL);
    }
    
    //------------------------------------------------------------------------------------------------
    // Restart the server (placeholder)
    protected void RestartServer()
    {
        // This is a placeholder. In a real implementation, you would use
        // server-specific methods to restart the server.
        
        // Notify all players
        ProcessRconCommand(STS_RCONCommands.CMD_ANNOUNCE, "all", "Server is restarting NOW!");
        
        // Log the restart
        Print("[StatTracker] Server restart initiated from admin dashboard");
        
        // In most cases, you would use a server management tool or script
        // to handle the actual restart.
    }
    
    //------------------------------------------------------------------------------------------------
    // Validate admin authentication
    protected bool ValidateAdminAuth(HTTPRequest request, HTTPResponse response)
    {
        // Check if API authentication is required
        if (!m_Config.m_bApiRequireAuth)
            return true;
            
        // Get authorization token
        string authToken = request.GetHeader("Authorization");
        
        // Remove "Bearer " prefix if present
        if (authToken.IndexOf("Bearer ") == 0)
            authToken = authToken.Substring(7);
            
        // Check token
        if (authToken != m_Config.m_sApiAuthToken)
        {
            SendErrorResponse(response, 401, "Unauthorized");
            return false;
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load file content
    protected string LoadFileContent(string filePath)
    {
        if (!FileExist(filePath))
            return "";
            
        FileHandle file = OpenFile(filePath, FileMode.READ);
        if (!file)
            return "";
            
        string content = "";
        string line;
        
        while (FGets(file, line) > 0)
        {
            content += line;
        }
        
        CloseFile(file);
        return content;
    }
    
    //------------------------------------------------------------------------------------------------
    // Send JSON response
    protected void SendJsonResponse(HTTPResponse response, int statusCode, map<string, Managed> data)
    {
        // Serialize data to JSON
        JsonSerializer serializer = new JsonSerializer();
        string json;
        
        if (!serializer.WriteToString(data, false, json))
        {
            SendErrorResponse(response, 500, "Error serializing response");
            return;
        }
        
        // Send response
        response.SetHeader("Content-Type", "application/json");
        response.SetData(json);
        response.SetStatusCode(statusCode);
    }
    
    //------------------------------------------------------------------------------------------------
    // Send error response
    protected void SendErrorResponse(HTTPResponse response, int statusCode, string message)
    {
        map<string, Managed> errorData = new map<string, Managed>();
        errorData.Insert("error", message);
        
        SendJsonResponse(response, statusCode, errorData);
    }
    
    //------------------------------------------------------------------------------------------------
    // Format a playtime in seconds to a readable string
    protected string FormatPlaytime(float seconds)
    {
        if (seconds < 60)
            return Math.Round(seconds).ToString() + " seconds";
            
        if (seconds < 3600)
            return Math.Floor(seconds / 60).ToString() + " minutes, " + Math.Round(seconds % 60).ToString() + " seconds";
            
        if (seconds < 86400)
            return Math.Floor(seconds / 3600).ToString() + " hours, " + Math.Floor((seconds % 3600) / 60).ToString() + " minutes";
            
        return Math.Floor(seconds / 86400).ToString() + " days, " + Math.Floor((seconds % 86400) / 3600).ToString() + " hours";
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