// STS_APIServer.c
// REST API server for exposing player statistics to external applications

class STS_APIServer
{
    // Singleton instance
    private static ref STS_APIServer s_Instance;
    
    // Config reference
    protected STS_Config m_Config;
    
    // Persistence manager reference
    protected STS_PersistenceManager m_PersistenceManager;
    
    // Internal server instance (placeholder - implementation depends on game's networking)
    protected ref HTTPServer m_HTTPServer;
    
    // API endpoints
    protected const string ENDPOINT_PLAYERS = "/api/players";
    protected const string ENDPOINT_PLAYER = "/api/players/{id}";
    protected const string ENDPOINT_LEADERBOARDS = "/api/leaderboards";
    protected const string ENDPOINT_LEADERBOARD = "/api/leaderboards/{name}";
    protected const string ENDPOINT_STATS = "/api/stats";
    protected const string ENDPOINT_ACHIEVEMENTS = "/api/achievements";
    
    // Cached response timeout (in seconds)
    protected const float CACHE_TIMEOUT = 60.0;
    
    // Cached responses
    protected ref map<string, ref STS_APICache> m_ResponseCache = new map<string, ref STS_APICache>();
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_APIServer()
    {
        m_Config = STS_Config.GetInstance();
        m_PersistenceManager = STS_PersistenceManager.GetInstance();
        
        // Initialize HTTP server
        InitServer();
        
        // Start cache cleanup timer
        GetGame().GetCallqueue().CallLater(CleanupCache, 60000, true); // Check cache every minute
        
        Print("[StatTracker] API Server initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_APIServer GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_APIServer();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize HTTP server
    protected void InitServer()
    {
        if (!m_Config.m_bEnableAPI)
            return;
            
        // Create HTTP server instance (placeholder - actual implementation depends on game's networking)
        m_HTTPServer = new HTTPServer(m_Config.m_sAPIPort);
        
        // Register routes
        m_HTTPServer.RegisterRoute("GET", ENDPOINT_PLAYERS, this, "HandlePlayersRequest");
        m_HTTPServer.RegisterRoute("GET", ENDPOINT_PLAYER, this, "HandlePlayerRequest");
        m_HTTPServer.RegisterRoute("GET", ENDPOINT_LEADERBOARDS, this, "HandleLeaderboardsRequest");
        m_HTTPServer.RegisterRoute("GET", ENDPOINT_LEADERBOARD, this, "HandleLeaderboardRequest");
        m_HTTPServer.RegisterRoute("GET", ENDPOINT_STATS, this, "HandleStatsRequest");
        m_HTTPServer.RegisterRoute("GET", ENDPOINT_ACHIEVEMENTS, this, "HandleAchievementsRequest");
        
        // Start the server
        m_HTTPServer.Start();
        
        Print("[StatTracker] API Server started on port " + m_Config.m_sAPIPort);
    }
    
    //------------------------------------------------------------------------------------------------
    // Stop the HTTP server
    void Stop()
    {
        if (m_HTTPServer)
        {
            m_HTTPServer.Stop();
            m_HTTPServer = null;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle /api/players request
    void HandlePlayersRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check API key
        if (!ValidateAPIKey(request, response))
            return;
            
        // Check cache
        string cacheKey = "players";
        STS_APICache cachedResponse = GetCachedResponse(cacheKey);
        if (cachedResponse)
        {
            SendResponse(response, 200, cachedResponse.m_sData);
            return;
        }
        
        // Get all player IDs
        array<string> playerIds = m_PersistenceManager.GetAllPlayerIds();
        
        // Create response JSON
        JsonSerializer serializer = new JsonSerializer();
        string json;
        
        if (!serializer.WriteToString(playerIds, false, json))
        {
            SendErrorResponse(response, 500, "Failed to serialize player data");
            return;
        }
        
        // Cache the response
        CacheResponse(cacheKey, json);
        
        // Send response
        SendResponse(response, 200, json);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle /api/players/{id} request
    void HandlePlayerRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check API key
        if (!ValidateAPIKey(request, response))
            return;
            
        // Get player ID from path parameter
        string playerId = request.GetPathParam("id");
        if (playerId == "")
        {
            SendErrorResponse(response, 400, "Missing player ID");
            return;
        }
        
        // Check cache
        string cacheKey = "player_" + playerId;
        STS_APICache cachedResponse = GetCachedResponse(cacheKey);
        if (cachedResponse)
        {
            SendResponse(response, 200, cachedResponse.m_sData);
            return;
        }
        
        // Load player stats
        STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
        if (!stats)
        {
            SendErrorResponse(response, 404, "Player not found");
            return;
        }
        
        // Create response JSON
        string json = stats.ToJson();
        if (json == "")
        {
            SendErrorResponse(response, 500, "Failed to serialize player data");
            return;
        }
        
        // Cache the response
        CacheResponse(cacheKey, json);
        
        // Send response
        SendResponse(response, 200, json);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle /api/leaderboards request
    void HandleLeaderboardsRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check API key
        if (!ValidateAPIKey(request, response))
            return;
            
        // Check cache
        string cacheKey = "leaderboards";
        STS_APICache cachedResponse = GetCachedResponse(cacheKey);
        if (cachedResponse)
        {
            SendResponse(response, 200, cachedResponse.m_sData);
            return;
        }
        
        // Get all leaderboard categories
        array<string> categories = m_PersistenceManager.GetLeaderboardCategories();
        
        // Create response JSON
        JsonSerializer serializer = new JsonSerializer();
        string json;
        
        if (!serializer.WriteToString(categories, false, json))
        {
            SendErrorResponse(response, 500, "Failed to serialize leaderboard data");
            return;
        }
        
        // Cache the response
        CacheResponse(cacheKey, json);
        
        // Send response
        SendResponse(response, 200, json);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle /api/leaderboards/{name} request
    void HandleLeaderboardRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check API key
        if (!ValidateAPIKey(request, response))
            return;
            
        // Get leaderboard name from path parameter
        string leaderboardName = request.GetPathParam("name");
        if (leaderboardName == "")
        {
            SendErrorResponse(response, 400, "Missing leaderboard name");
            return;
        }
        
        // Get count parameter
        string countParam = request.GetQueryParam("count");
        int count = countParam != "" ? countParam.ToInt() : 10;
        
        // Check cache
        string cacheKey = "leaderboard_" + leaderboardName + "_" + count;
        STS_APICache cachedResponse = GetCachedResponse(cacheKey);
        if (cachedResponse)
        {
            SendResponse(response, 200, cachedResponse.m_sData);
            return;
        }
        
        // Get leaderboard entries
        array<ref STS_LeaderboardEntry> entries = m_PersistenceManager.GetTopPlayers(leaderboardName, count);
        
        // Create response JSON
        JsonSerializer serializer = new JsonSerializer();
        string json;
        
        if (!serializer.WriteToString(entries, false, json))
        {
            SendErrorResponse(response, 500, "Failed to serialize leaderboard data");
            return;
        }
        
        // Cache the response
        CacheResponse(cacheKey, json);
        
        // Send response
        SendResponse(response, 200, json);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle /api/stats request
    void HandleStatsRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check API key
        if (!ValidateAPIKey(request, response))
            return;
            
        // Check cache
        string cacheKey = "stats";
        STS_APICache cachedResponse = GetCachedResponse(cacheKey);
        if (cachedResponse)
        {
            SendResponse(response, 200, cachedResponse.m_sData);
            return;
        }
        
        // Create stats summary
        map<string, int> totalStats = new map<string, int>();
        
        // Get all player IDs
        array<string> playerIds = m_PersistenceManager.GetAllPlayerIds();
        
        // Aggregate stats
        int totalPlayers = playerIds.Count();
        int totalKills = 0;
        int totalDeaths = 0;
        int totalHeadshots = 0;
        int totalPlaytime = 0;
        
        foreach (string playerId : playerIds)
        {
            STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
            if (!stats)
                continue;
                
            totalKills += stats.m_iKills;
            totalDeaths += stats.m_iDeaths;
            totalHeadshots += stats.m_iHeadshotKills;
            totalPlaytime += stats.m_iTotalPlaytimeSeconds;
        }
        
        // Create summary object
        map<string, float> summary = new map<string, float>();
        summary.Insert("players", totalPlayers);
        summary.Insert("kills", totalKills);
        summary.Insert("deaths", totalDeaths);
        summary.Insert("headshots", totalHeadshots);
        summary.Insert("playtime", totalPlaytime);
        summary.Insert("kd_ratio", totalDeaths > 0 ? totalKills / totalDeaths : totalKills);
        summary.Insert("headshot_ratio", totalKills > 0 ? (totalHeadshots / totalKills) * 100 : 0);
        
        // Create response JSON
        JsonSerializer serializer = new JsonSerializer();
        string json;
        
        if (!serializer.WriteToString(summary, false, json))
        {
            SendErrorResponse(response, 500, "Failed to serialize stats data");
            return;
        }
        
        // Cache the response
        CacheResponse(cacheKey, json);
        
        // Send response
        SendResponse(response, 200, json);
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle /api/achievements request
    void HandleAchievementsRequest(HTTPRequest request, HTTPResponse response)
    {
        // Check API key
        if (!ValidateAPIKey(request, response))
            return;
            
        // Check cache
        string cacheKey = "achievements";
        STS_APICache cachedResponse = GetCachedResponse(cacheKey);
        if (cachedResponse)
        {
            SendResponse(response, 200, cachedResponse.m_sData);
            return;
        }
        
        // Get all achievements
        array<ref STS_AchievementConfig> achievements = m_Config.m_aAchievements;
        
        // Create response JSON
        JsonSerializer serializer = new JsonSerializer();
        string json;
        
        if (!serializer.WriteToString(achievements, false, json))
        {
            SendErrorResponse(response, 500, "Failed to serialize achievements data");
            return;
        }
        
        // Cache the response
        CacheResponse(cacheKey, json);
        
        // Send response
        SendResponse(response, 200, json);
    }
    
    //------------------------------------------------------------------------------------------------
    // Validate API key from request
    protected bool ValidateAPIKey(HTTPRequest request, HTTPResponse response)
    {
        // Get API key from request
        string apiKey = request.GetHeader("X-API-Key");
        
        // Check if API key is valid
        if (apiKey != m_Config.m_sAPIKey)
        {
            SendErrorResponse(response, 401, "Invalid API key");
            return false;
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get cached response if still valid
    protected STS_APICache GetCachedResponse(string key)
    {
        if (!m_ResponseCache.Contains(key))
            return null;
            
        STS_APICache cache = m_ResponseCache.Get(key);
        
        // Check if cache has expired
        if (System.GetTickCount() / 1000.0 - cache.m_fTimestamp > CACHE_TIMEOUT)
        {
            m_ResponseCache.Remove(key);
            return null;
        }
        
        return cache;
    }
    
    //------------------------------------------------------------------------------------------------
    // Cache a response
    protected void CacheResponse(string key, string data)
    {
        STS_APICache cache = new STS_APICache();
        cache.m_sData = data;
        cache.m_fTimestamp = System.GetTickCount() / 1000.0;
        
        if (m_ResponseCache.Contains(key))
            m_ResponseCache.Set(key, cache);
        else
            m_ResponseCache.Insert(key, cache);
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up expired cache entries
    protected void CleanupCache()
    {
        float currentTime = System.GetTickCount() / 1000.0;
        array<string> keysToRemove = new array<string>();
        
        // Find expired cache entries
        for (int i = 0; i < m_ResponseCache.Count(); i++)
        {
            string key = m_ResponseCache.GetKey(i);
            STS_APICache cache = m_ResponseCache.Get(key);
            
            if (currentTime - cache.m_fTimestamp > CACHE_TIMEOUT)
            {
                keysToRemove.Insert(key);
            }
        }
        
        // Remove expired entries
        foreach (string key : keysToRemove)
        {
            m_ResponseCache.Remove(key);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Send HTTP response
    protected void SendResponse(HTTPResponse response, int statusCode, string data)
    {
        response.SetStatusCode(statusCode);
        response.SetHeader("Content-Type", "application/json");
        response.SetData(data);
        response.Send();
    }
    
    //------------------------------------------------------------------------------------------------
    // Send error response
    protected void SendErrorResponse(HTTPResponse response, int statusCode, string message)
    {
        string errorJson = "{\"error\":" + statusCode + ",\"message\":\"" + message + "\"}";
        SendResponse(response, statusCode, errorJson);
    }
}

//------------------------------------------------------------------------------------------------
// API response cache class
class STS_APICache
{
    string m_sData;
    float m_fTimestamp;
    
    void STS_APICache()
    {
        m_sData = "";
        m_fTimestamp = 0;
    }
}

//------------------------------------------------------------------------------------------------
// HTTP server placeholder class (actual implementation would depend on game's networking capabilities)
class HTTPServer
{
    protected string m_sPort;
    protected ref map<string, ref STS_Route> m_Routes = new map<string, ref STS_Route>();
    
    void HTTPServer(string port)
    {
        m_sPort = port;
    }
    
    void RegisterRoute(string method, string path, Class instance, string callback)
    {
        string key = method + "_" + path;
        
        STS_Route route = new STS_Route();
        route.m_sMethod = method;
        route.m_sPath = path;
        route.m_Instance = instance;
        route.m_sCallback = callback;
        
        if (m_Routes.Contains(key))
            m_Routes.Set(key, route);
        else
            m_Routes.Insert(key, route);
    }
    
    void Start()
    {
        // Placeholder - actual implementation would start a HTTP server
        Print("[StatTracker] HTTP Server started on port " + m_sPort);
    }
    
    void Stop()
    {
        // Placeholder - actual implementation would stop the HTTP server
        Print("[StatTracker] HTTP Server stopped");
    }
}

//------------------------------------------------------------------------------------------------
// Route configuration class
class STS_Route
{
    string m_sMethod;
    string m_sPath;
    Class m_Instance;
    string m_sCallback;
}

//------------------------------------------------------------------------------------------------
// HTTP request placeholder class
class HTTPRequest
{
    protected ref map<string, string> m_Headers = new map<string, string>();
    protected ref map<string, string> m_QueryParams = new map<string, string>();
    protected ref map<string, string> m_PathParams = new map<string, string>();
    protected string m_sBody;
    
    string GetHeader(string name)
    {
        if (!m_Headers.Contains(name))
            return "";
            
        return m_Headers.Get(name);
    }
    
    string GetQueryParam(string name)
    {
        if (!m_QueryParams.Contains(name))
            return "";
            
        return m_QueryParams.Get(name);
    }
    
    string GetPathParam(string name)
    {
        if (!m_PathParams.Contains(name))
            return "";
            
        return m_PathParams.Get(name);
    }
    
    string GetBody()
    {
        return m_sBody;
    }
}

//------------------------------------------------------------------------------------------------
// HTTP response placeholder class
class HTTPResponse
{
    protected int m_iStatusCode = 200;
    protected ref map<string, string> m_Headers = new map<string, string>();
    protected string m_sData;
    
    void SetStatusCode(int code)
    {
        m_iStatusCode = code;
    }
    
    void SetHeader(string name, string value)
    {
        if (m_Headers.Contains(name))
            m_Headers.Set(name, value);
        else
            m_Headers.Insert(name, value);
    }
    
    void SetData(string data)
    {
        m_sData = data;
    }
    
    void Send()
    {
        // Placeholder - actual implementation would send the HTTP response
    }
}

//------------------------------------------------------------------------------------------------
// REST API placeholder class
class RestApi
{
    static RestApi Create()
    {
        return new RestApi();
    }
    
    RestContext GetContext(string url)
    {
        return new RestContext();
    }
}

//------------------------------------------------------------------------------------------------
// REST context placeholder class
class RestContext
{
    void SetHeader(string name, string value)
    {
        // Placeholder
    }
    
    void POST(string data, string contentType, Class instance, string callback)
    {
        // Placeholder - actual implementation would send a POST request
    }
}

//------------------------------------------------------------------------------------------------
// REST response placeholder class
class RestResponse
{
    protected int m_iCode = 200;
    protected string m_sData = "";
    
    int GetCode()
    {
        return m_iCode;
    }
    
    string GetData()
    {
        return m_sData;
    }
} 