// STS_HeatmapManager.c
// Manager for generating and displaying heatmaps of player activity

class STS_HeatmapPoint
{
    vector m_vPosition;
    int m_iType; // 0 = Kill, 1 = Death, 2 = Base Capture, 3 = Supply Delivery, etc.
    int m_iPlayerID;
    string m_sPlayerName;
    float m_fTimestamp;
    int m_iWeaponType; // 0 = Rifle, 1 = Pistol, 2 = Launcher, 3 = Vehicle, etc.
    float m_fDistance; // Distance if it was a kill
    
    void STS_HeatmapPoint(vector position, int type, int playerID, string playerName, float timestamp = 0, int weaponType = -1, float distance = 0)
    {
        m_vPosition = position;
        m_iType = type;
        m_iPlayerID = playerID;
        m_sPlayerName = playerName;
        m_fTimestamp = timestamp > 0 ? timestamp : System.GetTickCount() / 1000.0;
        m_iWeaponType = weaponType;
        m_fDistance = distance;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"x\":" + m_vPosition[0].ToString() + ",";
        json += "\"y\":" + m_vPosition[1].ToString() + ",";
        json += "\"z\":" + m_vPosition[2].ToString() + ",";
        json += "\"type\":" + m_iType.ToString() + ",";
        json += "\"playerID\":" + m_iPlayerID.ToString() + ",";
        json += "\"playerName\":\"" + m_sPlayerName + "\",";
        json += "\"timestamp\":" + m_fTimestamp.ToString() + ",";
        json += "\"weaponType\":" + m_iWeaponType.ToString() + ",";
        json += "\"distance\":" + m_fDistance.ToString();
        json += "}";
        return json;
    }
}

class STS_HeatmapCell
{
    int m_iGridX;
    int m_iGridY;
    int m_iCount;
    int m_iType; // Combined heatmap type
    
    void STS_HeatmapCell(int gridX, int gridY, int type)
    {
        m_iGridX = gridX;
        m_iGridY = gridY;
        m_iCount = 1;
        m_iType = type;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"x\":" + m_iGridX.ToString() + ",";
        json += "\"y\":" + m_iGridY.ToString() + ",";
        json += "\"count\":" + m_iCount.ToString() + ",";
        json += "\"type\":" + m_iType.ToString();
        json += "}";
        return json;
    }
}

class STS_HeatmapHotspot
{
    vector m_vPosition;
    float m_fRadius;
    int m_iType;
    int m_iIntensity;
    string m_sLabel;
    
    void STS_HeatmapHotspot(vector position, float radius, int type, int intensity, string label)
    {
        m_vPosition = position;
        m_fRadius = radius;
        m_iType = type;
        m_iIntensity = intensity;
        m_sLabel = label;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"x\":" + m_vPosition[0].ToString() + ",";
        json += "\"y\":" + m_vPosition[1].ToString() + ",";
        json += "\"z\":" + m_vPosition[2].ToString() + ",";
        json += "\"radius\":" + m_fRadius.ToString() + ",";
        json += "\"type\":" + m_iType.ToString() + ",";
        json += "\"intensity\":" + m_iIntensity.ToString() + ",";
        json += "\"label\":\"" + m_sLabel + "\"";
        json += "}";
        return json;
    }
}

class STS_HeatmapManager
{
    // Singleton instance
    private static ref STS_HeatmapManager s_Instance;
    
    // Config reference
    protected ref STS_Config m_Config;
    
    // Heat data storage by type
    protected ref map<string, ref array<ref STS_HeatmapPoint>> m_HeatData;
    
    // Cache of generated heatmaps
    protected ref map<string, ref STS_HeatmapCache> m_HeatmapCache;
    
    // Heatmap types
    static const string HEATMAP_KILLS = "kills";
    static const string HEATMAP_DEATHS = "deaths";
    static const string HEATMAP_ACTIVITY = "activity";
    static const string HEATMAP_COMBAT = "combat";
    static const string HEATMAP_VEHICLES = "vehicles";
    static const string HEATMAP_BASECAPTURE = "basecapture";
    static const string HEATMAP_SUPPLY = "supply";
    
    // API server reference
    protected ref STS_APIServer m_APIServer;
    
    // Data visualization reference
    protected ref STS_DataVisualization m_DataVis;
    
    // Dimensions for heatmap
    protected vector m_vWorldSize;
    protected int m_iMapResolution;
    protected float m_fDecayRate;
    protected float m_fPointRadius;
    
    // Statistics for hotspot analysis
    protected ref array<ref STS_HeatmapHotspot> m_Hotspots;
    protected int m_iMaxHotspots = 10;
    protected float m_fHotspotThreshold = 0.75; // Top 25% of heat intensity
    protected float m_fLastHotspotUpdate = 0;
    protected const float HOTSPOT_UPDATE_INTERVAL = 300; // Update hotspots every 5 minutes
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_HeatmapManager()
    {
        Print("[StatTracker] Initializing Heatmap Manager");
        
        m_Config = STS_Config.GetInstance();
        m_APIServer = STS_APIServer.GetInstance();
        m_DataVis = STS_DataVisualization.GetInstance();
        
        // Initialize heat data storage
        m_HeatData = new map<string, ref array<ref STS_HeatmapPoint>>();
        m_HeatmapCache = new map<string, ref STS_HeatmapCache>();
        m_Hotspots = new array<ref STS_HeatmapHotspot>();
        
        // Initialize heat data arrays for each type
        m_HeatData.Set(HEATMAP_KILLS, new array<ref STS_HeatmapPoint>());
        m_HeatData.Set(HEATMAP_DEATHS, new array<ref STS_HeatmapPoint>());
        m_HeatData.Set(HEATMAP_ACTIVITY, new array<ref STS_HeatmapPoint>());
        m_HeatData.Set(HEATMAP_COMBAT, new array<ref STS_HeatmapPoint>());
        m_HeatData.Set(HEATMAP_VEHICLES, new array<ref STS_HeatmapPoint>());
        m_HeatData.Set(HEATMAP_BASECAPTURE, new array<ref STS_HeatmapPoint>());
        m_HeatData.Set(HEATMAP_SUPPLY, new array<ref STS_HeatmapPoint>());
        
        // Get world size
        ChimeraWorld world = GetGame().GetWorld();
        if (world)
        {
            m_vWorldSize = world.GetWorldSize();
        }
        else
        {
            // Default size if world not available
            m_vWorldSize = Vector(8192, 0, 8192);
        }
        
        // Set configuration values
        m_iMapResolution = 256; // Default resolution
        m_fDecayRate = 0.05; // 5% decay per hour
        m_fPointRadius = 100; // 100m radius per point
        
        // Load configuration from STS_Config if available
        if (m_Config)
        {
            if (m_Config.m_iHeatmapResolution > 0)
                m_iMapResolution = m_Config.m_iHeatmapResolution;
                
            if (m_Config.m_fHeatmapDecayRate > 0)
                m_fDecayRate = m_Config.m_fHeatmapDecayRate;
                
            if (m_Config.m_fHeatmapPointRadius > 0)
                m_fPointRadius = m_Config.m_fHeatmapPointRadius;
                
            if (m_Config.m_iMaxHotspots > 0)
                m_iMaxHotspots = m_Config.m_iMaxHotspots;
                
            if (m_Config.m_fHotspotThreshold > 0)
                m_fHotspotThreshold = m_Config.m_fHotspotThreshold;
        }
        
        // Register API endpoints if API server is available
        if (m_APIServer)
        {
            RegisterAPIEndpoints();
        }
        
        // Load existing heat data
        LoadHeatData();
        
        // Start decay process
        GetGame().GetCallqueue().CallLater(DecayHeatpoints, 3600000, true); // Run every hour
        
        // Start regular hotspot analysis
        GetGame().GetCallqueue().CallLater(UpdateHotspots, HOTSPOT_UPDATE_INTERVAL * 1000, true);
        
        Print("[StatTracker] Heatmap Manager initialized successfully");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_HeatmapManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_HeatmapManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Register API endpoints
    protected void RegisterAPIEndpoints()
    {
        if (!m_APIServer)
            return;
            
        // Register endpoint for getting heatmap data
        m_APIServer.RegisterEndpoint("GET", "/api/heatmap", GetHeatmapData);
        
        // Register endpoint for getting hotspots
        m_APIServer.RegisterEndpoint("GET", "/api/hotspots", GetHotspots);
        
        // Register endpoint for getting activity analytics
        m_APIServer.RegisterEndpoint("GET", "/api/activity", GetActivityAnalytics);
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a heatpoint to the specified heatmap
    void AddHeatPoint(string type, vector position, float intensity = 1.0, string metadata = "")
    {
        if (!m_HeatData.Contains(type))
            return;
            
        // Create new heat point
        STS_HeatmapPoint point = new STS_HeatmapPoint(position, 0, 0, "", 0, -1, 0);
        point.m_fIntensity = intensity;
        point.m_sMetadata = metadata;
        
        // Add to the appropriate heat map
        m_HeatData.Get(type).Insert(point);
        
        // Invalidate cache for this type
        InvalidateCache(type);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get heatmap data (for API endpoint)
    string GetHeatmapData(map<string, string> parameters)
    {
        string type = "kills"; // Default type
        if (parameters && parameters.Contains("type"))
            type = parameters.Get("type");
            
        // Check if valid type
        if (!m_HeatData.Contains(type))
            return "{\"error\": \"Invalid heatmap type\"}";
            
        // Get resolution if specified
        int resolution = m_iMapResolution;
        if (parameters && parameters.Contains("resolution"))
            resolution = parameters.Get("resolution").ToInt();
            
        // Check if we have a valid cached heatmap
        if (m_HeatmapCache.Contains(type))
        {
            STS_HeatmapCache cache = m_HeatmapCache.Get(type);
            if (cache && cache.m_iResolution == resolution && System.GetUnixTime() - cache.m_iTimestamp < 300) // Cache valid for 5 minutes
            {
                return cache.m_sData;
            }
        }
        
        // Generate new heatmap
        return GenerateHeatmapJSON(type, resolution);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get hotspots (for API endpoint)
    string GetHotspots(map<string, string> parameters)
    {
        // Check if hotspots need updating
        float currentTime = System.GetTickCount() / 1000.0;
        if (currentTime - m_fLastHotspotUpdate > HOTSPOT_UPDATE_INTERVAL)
        {
            UpdateHotspots();
        }
        
        // Serialize hotspots to JSON
        string json = "[";
        for (int i = 0; i < m_Hotspots.Count(); i++)
        {
            json += m_Hotspots[i].ToJSON();
            if (i < m_Hotspots.Count() - 1)
                json += ",";
        }
        json += "]";
        
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get activity analytics (for API endpoint)
    string GetActivityAnalytics(map<string, string> parameters)
    {
        // Time period (default to last 24 hours)
        int timeFrom = System.GetUnixTime() - 86400;
        if (parameters && parameters.Contains("from"))
            timeFrom = parameters.Get("from").ToInt();
            
        // Generate activity data
        return GenerateActivityAnalyticsJSON(timeFrom);
    }
    
    //------------------------------------------------------------------------------------------------
    // Update hotspots analysis
    void UpdateHotspots()
    {
        Print("[StatTracker] Updating hotspot analysis");
        
        m_fLastHotspotUpdate = System.GetTickCount() / 1000.0;
        
        // Clear existing hotspots
        m_Hotspots.Clear();
        
        // Process each heatmap type
        foreach (string type, array<ref STS_HeatmapPoint> points : m_HeatData)
        {
            // Skip types with too few points
            if (points.Count() < 10)
                continue;
                
            // Find clusters using density-based clustering
            array<ref array<ref STS_HeatmapPoint>> clusters = FindClusters(points, m_fPointRadius * 2, 5);
            
            // Process each cluster
            foreach (array<ref STS_HeatmapPoint> cluster : clusters)
            {
                // Skip small clusters
                if (cluster.Count() < 5)
                    continue;
                    
                // Calculate cluster center and total intensity
                vector center = Vector(0, 0, 0);
                float totalIntensity = 0;
                int startTime = 2147483647; // INT_MAX
                int endTime = 0;
                
                foreach (STS_HeatmapPoint point : cluster)
                {
                    center += point.m_vPosition;
                    totalIntensity += point.m_fIntensity;
                    
                    if (point.m_fTimestamp < startTime)
                        startTime = point.m_fTimestamp;
                        
                    if (point.m_fTimestamp > endTime)
                        endTime = point.m_fTimestamp;
                }
                
                center = center / cluster.Count();
                
                // Create hotspot
                STS_HeatmapHotspot hotspot = new STS_HeatmapHotspot(center, 0, 0, 0, "");
                hotspot.m_fRadius = CalculateClusterRadius(cluster, center);
                hotspot.m_fIntensity = totalIntensity;
                hotspot.m_iType = type;
                hotspot.m_iPointCount = cluster.Count();
                hotspot.m_iStartTime = startTime;
                hotspot.m_iEndTime = endTime;
                
                // Calculate name based on nearest location
                hotspot.m_sLabel = GetNearestLocationName(center);
                
                // Add to hotspots array
                m_Hotspots.Insert(hotspot);
            }
        }
        
        // Sort hotspots by intensity (descending)
        SortHotspotsByIntensity();
        
        // Limit to max hotspots
        if (m_Hotspots.Count() > m_iMaxHotspots)
        {
            m_Hotspots.Resize(m_iMaxHotspots);
        }
        
        Print(string.Format("[StatTracker] Identified %1 hotspots", m_Hotspots.Count()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Find clusters using density-based clustering (simplified DBSCAN)
    protected array<ref array<ref STS_HeatmapPoint>> FindClusters(array<ref STS_HeatmapPoint> points, float eps, int minPoints)
    {
        array<ref array<ref STS_HeatmapPoint>> clusters = new array<ref array<ref STS_HeatmapPoint>>();
        array<bool> visited = new array<bool>();
        
        // Initialize visited array
        for (int i = 0; i < points.Count(); i++)
        {
            visited.Insert(false);
        }
        
        // Process each point
        for (int i = 0; i < points.Count(); i++)
        {
            // Skip if already visited
            if (visited[i])
                continue;
                
            // Mark as visited
            visited[i] = true;
            
            // Find neighbors
            array<int> neighbors = GetNeighbors(points, i, eps);
            
            // Check if this is a core point
            if (neighbors.Count() >= minPoints)
            {
                // Start a new cluster
                ref array<ref STS_HeatmapPoint> cluster = new array<ref STS_HeatmapPoint>();
                cluster.Insert(points[i]);
                
                // Expand cluster
                ExpandCluster(points, visited, neighbors, cluster, eps, minPoints);
                
                // Add cluster to results
                clusters.Insert(cluster);
            }
        }
        
        return clusters;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get neighbors within eps distance
    protected array<int> GetNeighbors(array<ref STS_HeatmapPoint> points, int pointIndex, float eps)
    {
        array<int> neighbors = new array<int>();
        
        for (int i = 0; i < points.Count(); i++)
        {
            if (i == pointIndex)
                continue;
                
            float distance = vector.Distance(points[pointIndex].m_vPosition, points[i].m_vPosition);
            if (distance <= eps)
            {
                neighbors.Insert(i);
            }
        }
        
        return neighbors;
    }
    
    //------------------------------------------------------------------------------------------------
    // Expand cluster by adding neighbors
    protected void ExpandCluster(array<ref STS_HeatmapPoint> points, array<bool> visited, array<int> neighbors, array<ref STS_HeatmapPoint> cluster, float eps, int minPoints)
    {
        // Process each neighbor
        for (int i = 0; i < neighbors.Count(); i++)
        {
            int index = neighbors[i];
            
            // Skip if already processed
            if (visited[index])
                continue;
                
            // Mark as visited
            visited[index] = true;
            
            // Add to cluster
            cluster.Insert(points[index]);
            
            // Get neighbors of this point
            array<int> newNeighbors = GetNeighbors(points, index, eps);
            
            // If this is a core point, add its neighbors to be processed
            if (newNeighbors.Count() >= minPoints)
            {
                for (int j = 0; j < newNeighbors.Count(); j++)
                {
                    if (neighbors.Find(newNeighbors[j]) < 0)
                    {
                        neighbors.Insert(newNeighbors[j]);
                    }
                }
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Calculate the radius of a cluster
    protected float CalculateClusterRadius(array<ref STS_HeatmapPoint> cluster, vector center)
    {
        float maxDistance = 0;
        
        foreach (STS_HeatmapPoint point : cluster)
        {
            float distance = vector.Distance(center, point.m_vPosition);
            if (distance > maxDistance)
            {
                maxDistance = distance;
            }
        }
        
        return maxDistance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the name of the nearest location
    protected string GetNearestLocationName(vector position)
    {
        // This would use the game's location system or a custom location database
        // For now, use a generic name based on map coordinates
        return string.Format("Area %1,%2", Math.Round(position[0] / 100), Math.Round(position[2] / 100));
    }
    
    //------------------------------------------------------------------------------------------------
    // Sort hotspots by intensity (descending)
    protected void SortHotspotsByIntensity()
    {
        // Simple bubble sort
        for (int i = 0; i < m_Hotspots.Count(); i++)
        {
            for (int j = i + 1; j < m_Hotspots.Count(); j++)
            {
                if (m_Hotspots[i].m_fIntensity < m_Hotspots[j].m_fIntensity)
                {
                    ref STS_HeatmapHotspot temp = m_Hotspots[i];
                    m_Hotspots[i] = m_Hotspots[j];
                    m_Hotspots[j] = temp;
                }
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate heatmap JSON data
    protected string GenerateHeatmapJSON(string type, int resolution)
    {
        if (!m_HeatData.Contains(type))
            return "{}";
            
        array<ref STS_HeatmapPoint> points = m_HeatData.Get(type);
        
        // Generate grid
        array<array<float>> grid = new array<array<float>>();
        for (int i = 0; i < resolution; i++)
        {
            array<float> row = new array<float>();
            for (int j = 0; j < resolution; j++)
            {
                row.Insert(0);
            }
            grid.Insert(row);
        }
        
        // Calculate cell size
        float cellSizeX = m_vWorldSize[0] / resolution;
        float cellSizeZ = m_vWorldSize[2] / resolution;
        
        // Add heat from each point
        foreach (STS_HeatmapPoint point : points)
        {
            // Convert world position to grid position
            int gridX = Math.Clamp(Math.Round(point.m_vPosition[0] / cellSizeX), 0, resolution - 1);
            int gridZ = Math.Clamp(Math.Round(point.m_vPosition[2] / cellSizeZ), 0, resolution - 1);
            
            // Calculate radius in grid cells
            int radiusCells = Math.Clamp(Math.Round(m_fPointRadius / cellSizeX), 1, resolution / 4);
            
            // Add heat to surrounding cells based on distance
            for (int x = Math.Max(0, gridX - radiusCells); x <= Math.Min(resolution - 1, gridX + radiusCells); x++)
            {
                for (int z = Math.Max(0, gridZ - radiusCells); z <= Math.Min(resolution - 1, gridZ + radiusCells); z++)
                {
                    float distance = Math.Sqrt(Math.Pow(x - gridX, 2) + Math.Pow(z - gridZ, 2));
                    if (distance <= radiusCells)
                    {
                        // Gaussian falloff
                        float intensity = point.m_fIntensity * Math.Exp(-(distance * distance) / (2 * radiusCells * radiusCells));
                        grid[x][z] += intensity;
                    }
                }
            }
        }
        
        // Normalize grid values
        float maxValue = 0;
        for (int i = 0; i < resolution; i++)
        {
            for (int j = 0; j < resolution; j++)
            {
                if (grid[i][j] > maxValue)
                    maxValue = grid[i][j];
            }
        }
        
        if (maxValue > 0)
        {
            for (int i = 0; i < resolution; i++)
            {
                for (int j = 0; j < resolution; j++)
                {
                    grid[i][j] /= maxValue;
                }
            }
        }
        
        // Generate JSON
        string json = "{\"type\":\"" + type + "\",\"resolution\":" + resolution + ",\"data\":[";
        for (int i = 0; i < resolution; i++)
        {
            json += "[";
            for (int j = 0; j < resolution; j++)
            {
                json += grid[i][j].ToString();
                if (j < resolution - 1)
                    json += ",";
            }
            json += "]";
            if (i < resolution - 1)
                json += ",";
        }
        json += "]}";
        
        // Cache the result
        STS_HeatmapCache cache = new STS_HeatmapCache();
        cache.m_iResolution = resolution;
        cache.m_iTimestamp = System.GetUnixTime();
        cache.m_sData = json;
        
        m_HeatmapCache.Set(type, cache);
        
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate activity analytics JSON
    protected string GenerateActivityAnalyticsJSON(int timeFrom)
    {
        // Collect activity data for each hour
        map<int, int> hourlyActivity = new map<int, int>();
        map<string, int> activityByType = new map<string, int>();
        
        // Initialize hourly data (24 hours)
        int currentHour = Math.Floor(System.GetUnixTime() / 3600);
        for (int i = 0; i < 24; i++)
        {
            hourlyActivity.Set(currentHour - i, 0);
        }
        
        // Initialize type data
        activityByType.Set(HEATMAP_KILLS, 0);
        activityByType.Set(HEATMAP_DEATHS, 0);
        activityByType.Set(HEATMAP_ACTIVITY, 0);
        activityByType.Set(HEATMAP_COMBAT, 0);
        activityByType.Set(HEATMAP_VEHICLES, 0);
        activityByType.Set(HEATMAP_BASECAPTURE, 0);
        activityByType.Set(HEATMAP_SUPPLY, 0);
        
        // Count activity data
        foreach (string type, array<ref STS_HeatmapPoint> points : m_HeatData)
        {
            int typeCount = 0;
            
            foreach (STS_HeatmapPoint point : points)
            {
                if (point.m_fTimestamp < timeFrom)
                    continue;
                    
                // Count by type
                typeCount++;
                
                // Count by hour
                int hour = Math.Floor(point.m_fTimestamp / 3600);
                if (hourlyActivity.Contains(hour))
                {
                    hourlyActivity.Set(hour, hourlyActivity.Get(hour) + 1);
                }
            }
            
            activityByType.Set(type, typeCount);
        }
        
        // Generate JSON
        string json = "{";
        
        // Add hourly data
        json += "\"hourly\":{";
        bool first = true;
        foreach (int hour, int count : hourlyActivity)
        {
            if (!first) json += ",";
            json += "\"" + hour + "\":" + count;
            first = false;
        }
        json += "},";
        
        // Add type data
        json += "\"byType\":{";
        first = true;
        foreach (string type, int count : activityByType)
        {
            if (!first) json += ",";
            json += "\"" + type + "\":" + count;
            first = false;
        }
        json += "}";
        
        json += "}";
        
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Decay heatpoints over time
    void DecayHeatpoints()
    {
        Print("[StatTracker] Applying heatmap decay");
        
        foreach (string type, array<ref STS_HeatmapPoint> points : m_HeatData)
        {
            for (int i = points.Count() - 1; i >= 0; i--)
            {
                // Calculate age in hours
                float ageHours = (System.GetUnixTime() - points[i].m_fTimestamp) / 3600.0;
                
                // Apply decay
                float newIntensity = points[i].m_fIntensity * Math.Pow(1 - m_fDecayRate, ageHours);
                
                // Remove if too small, otherwise update
                if (newIntensity < 0.05)
                {
                    points.Remove(i);
                }
                else
                {
                    points[i].m_fIntensity = newIntensity;
                }
            }
            
            // Invalidate cache
            InvalidateCache(type);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Invalidate cache for a specific type
    protected void InvalidateCache(string type)
    {
        if (m_HeatmapCache.Contains(type))
        {
            m_HeatmapCache.Remove(type);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save heat data to disk
    void SaveHeatData()
    {
        Print("[StatTracker] Saving heatmap data");
        
        string filePath = "$profile:StatTracker/heatmap_data.json";
        
        // Build JSON structure
        string json = "{";
        
        bool firstType = true;
        foreach (string type, array<ref STS_HeatmapPoint> points : m_HeatData)
        {
            if (!firstType) json += ",";
            
            json += "\"" + type + "\":[";
            
            bool firstPoint = true;
            foreach (STS_HeatmapPoint point : points)
            {
                if (!firstPoint) json += ",";
                
                json += "{";
                json += "\"x\":" + point.m_vPosition[0].ToString() + ",";
                json += "\"y\":" + point.m_vPosition[1].ToString() + ",";
                json += "\"z\":" + point.m_vPosition[2].ToString() + ",";
                json += "\"i\":" + point.m_fIntensity.ToString() + ",";
                json += "\"t\":" + point.m_fTimestamp.ToString() + ",";
                json += "\"m\":\"" + point.m_sMetadata + "\"";
                json += "}";
                
                firstPoint = false;
            }
            
            json += "]";
            firstType = false;
        }
        
        json += "}";
        
        // Save to file (compressed if available)
        if (m_Config && m_Config.m_bCompressHeatmapData && STS_DataCompression.GetInstance())
        {
            STS_DataCompression.GetInstance().SaveCompressedData(filePath, json);
        }
        else
        {
            FileIO.WriteFile(filePath, json);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load heat data from disk
    void LoadHeatData()
    {
        Print("[StatTracker] Loading heatmap data");
        
        string filePath = "$profile:StatTracker/heatmap_data.json";
        
        if (!FileIO.FileExists(filePath))
        {
            Print("[StatTracker] No heatmap data file found");
            return;
        }
        
        string json;
        
        // Load from file (decompressed if needed)
        if (m_Config && m_Config.m_bCompressHeatmapData && STS_DataCompression.GetInstance())
        {
            json = STS_DataCompression.GetInstance().LoadCompressedData(filePath);
        }
        else
        {
            FileIO.ReadFile(filePath, json);
        }
        
        if (json.Length() < 10)
        {
            Print("[StatTracker] Empty or invalid heatmap data file");
            return;
        }
        
        // Parse JSON (simple parser)
        // In a real implementation, use a proper JSON parser
        // This is a simplified version for demonstration purposes
        
        // Clear existing data
        foreach (string type, array<ref STS_HeatmapPoint> points : m_HeatData)
        {
            points.Clear();
        }
        
        // TODO: Implement proper JSON parsing here
        // For a real implementation, you would need a proper JSON parser
        
        Print("[StatTracker] Heatmap data loaded successfully");
    }
}

//------------------------------------------------------------------------------------------------
// Heat point class
class STS_HeatmapPoint
{
    vector m_vPosition;      // World position
    float m_fIntensity;      // Heat intensity
    int m_iTimestamp;        // Unix timestamp
    string m_sMetadata;      // Additional data (JSON string)
    
    void STS_HeatmapPoint()
    {
        m_vPosition = Vector(0, 0, 0);
        m_fIntensity = 1.0;
        m_iTimestamp = System.GetUnixTime();
        m_sMetadata = "";
    }
}

//------------------------------------------------------------------------------------------------
// Heatmap cache class
class STS_HeatmapCache
{
    int m_iResolution;     // Resolution of the cached heatmap
    int m_iTimestamp;      // Time when cache was created
    string m_sData;        // JSON data
}

//------------------------------------------------------------------------------------------------
// Heatmap hotspot class
class STS_HeatmapHotspot
{
    vector m_vPosition;      // Center position
    float m_fRadius;         // Radius in meters
    int m_iType;             // Heatmap type
    int m_iIntensity;        // Total intensity
    string m_sLabel;          // Name (nearest location)
    int m_iPointCount;       // Number of points in hotspot
    int m_iStartTime;        // First activity time
    int m_iEndTime;          // Last activity time
    
    void STS_HeatmapHotspot(vector position, float radius, int type, int intensity, string label)
    {
        m_vPosition = position;
        m_fRadius = radius;
        m_iType = type;
        m_iIntensity = intensity;
        m_sLabel = label;
        m_iPointCount = 0;
        m_iStartTime = 0;
        m_iEndTime = 0;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"x\":" + m_vPosition[0].ToString() + ",";
        json += "\"y\":" + m_vPosition[1].ToString() + ",";
        json += "\"z\":" + m_vPosition[2].ToString() + ",";
        json += "\"radius\":" + m_fRadius.ToString() + ",";
        json += "\"intensity\":" + m_iIntensity.ToString() + ",";
        json += "\"type\":\"" + m_iType.ToString() + "\",";
        json += "\"points\":" + m_iPointCount.ToString() + ",";
        json += "\"name\":\"" + m_sLabel + "\",";
        json += "\"startTime\":" + m_iStartTime.ToString() + ",";
        json += "\"endTime\":" + m_iEndTime.ToString() + ",";
        json += "\"duration\":" + (m_iEndTime - m_iStartTime).ToString();
        json += "}";
        return json;
    }
} 