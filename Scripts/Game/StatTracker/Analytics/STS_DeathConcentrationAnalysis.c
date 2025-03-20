// STS_DeathConcentrationAnalysis.c
// Component that analyzes player death locations to identify potential camping spots and high-activity areas

class STS_DeathConcentrationConfig
{
    float m_fClusterRadius = 10.0; // Radius in meters to consider deaths as part of the same cluster
    int m_iMinDeathsForHotspot = 5; // Minimum deaths required to consider an area a hotspot
    float m_fTimeWindowForCamping = 300.0; // Time window in seconds to consider for camping detection (5 minutes)
    int m_iKillsInWindowForCamping = 3; // Minimum kills in the time window to flag as camping
    float m_fMaxCamperMovementRadius = 15.0; // Maximum movement radius for a player to be considered camping
    int m_iHeatmapResolution = 64; // Resolution of the heatmap grid
    int m_iMaxClusters = 50; // Maximum number of clusters to track
    int m_iMaxHeatmapEntries = 10000; // Maximum number of death locations to store for heatmap
    float m_fHeatDecayRate = 0.05; // Heat decay rate per minute for old death locations
}

class STS_DeathLocation
{
    vector m_vPosition; // World position
    int m_iTimestamp; // Unix timestamp when death occurred
    string m_sKillerPlayerId; // Player ID of killer (if any)
    string m_sVictimPlayerId; // Player ID of victim
    string m_sWeaponName; // Weapon used
    float m_fKillDistance; // Distance between killer and victim
    int m_iTeamId; // Team ID of the killer
}

class STS_Cluster
{
    vector m_vCenterPosition; // Center position of the cluster
    array<ref STS_DeathLocation> m_aDeathLocations = new array<ref STS_DeathLocation>(); // Death locations in this cluster
    int m_iLastUpdateTime = 0; // Last time this cluster was updated
    float m_fRadius = 0; // Radius of the cluster
    map<string, int> m_mKillerCounts = new map<string, int>(); // Count of kills by each player in this cluster
    map<string, int> m_mWeaponCounts = new map<string, int>(); // Count of kills by each weapon type in this cluster
    bool m_bIsCampingSpot = false; // Whether this cluster is identified as a camping spot
    float m_fHeatValue = 0; // Heat value for visualization
    
    void AddDeathLocation(STS_DeathLocation deathLocation)
    {
        m_aDeathLocations.Insert(deathLocation);
        
        // Update the center position with weighted average
        vector newCenter = vector.Zero;
        foreach (STS_DeathLocation loc : m_aDeathLocations)
        {
            newCenter += loc.m_vPosition;
        }
        m_vCenterPosition = newCenter / m_aDeathLocations.Count();
        
        // Update radius to encompass all points
        float maxDist = 0;
        foreach (STS_DeathLocation loc : m_aDeathLocations)
        {
            float dist = vector.Distance(m_vCenterPosition, loc.m_vPosition);
            if (dist > maxDist)
                maxDist = dist;
        }
        m_fRadius = maxDist;
        
        // Update killer and weapon counts
        if (!m_mKillerCounts.Contains(deathLocation.m_sKillerPlayerId))
            m_mKillerCounts.Insert(deathLocation.m_sKillerPlayerId, 0);
        m_mKillerCounts.Set(deathLocation.m_sKillerPlayerId, m_mKillerCounts.Get(deathLocation.m_sKillerPlayerId) + 1);
        
        if (!m_mWeaponCounts.Contains(deathLocation.m_sWeaponName))
            m_mWeaponCounts.Insert(deathLocation.m_sWeaponName, 0);
        m_mWeaponCounts.Set(deathLocation.m_sWeaponName, m_mWeaponCounts.Get(deathLocation.m_sWeaponName) + 1);
        
        // Update timestamp
        m_iLastUpdateTime = System.GetUnixTime();
        
        // Update heat value
        m_fHeatValue = m_aDeathLocations.Count();
    }
    
    string GetMostCommonWeapon()
    {
        string mostCommonWeapon = "Unknown";
        int maxCount = 0;
        
        foreach (string weapon, int count : m_mWeaponCounts)
        {
            if (count > maxCount)
            {
                maxCount = count;
                mostCommonWeapon = weapon;
            }
        }
        
        return mostCommonWeapon;
    }
    
    string GetMostActiveKiller()
    {
        string mostActiveKiller = "Unknown";
        int maxCount = 0;
        
        foreach (string killer, int count : m_mKillerCounts)
        {
            if (count > maxCount && killer != "")
            {
                maxCount = count;
                mostActiveKiller = killer;
            }
        }
        
        return mostActiveKiller;
    }
    
    int GetDeathCount()
    {
        return m_aDeathLocations.Count();
    }
    
    // Check if this cluster shows camping behavior
    bool AnalyzeCampingBehavior(float timeWindow, int minKills, float maxMovementRadius)
    {
        int currentTime = System.GetUnixTime();
        
        // Skip if not enough deaths
        if (m_aDeathLocations.Count() < minKills)
            return false;
            
        // Group deaths by killer
        map<string, ref array<ref STS_DeathLocation>> killerDeaths = new map<string, ref array<ref STS_DeathLocation>>();
        
        foreach (STS_DeathLocation death : m_aDeathLocations)
        {
            string killerId = death.m_sKillerPlayerId;
            if (killerId == "")
                continue;
                
            if (!killerDeaths.Contains(killerId))
                killerDeaths.Insert(killerId, new array<ref STS_DeathLocation>());
                
            killerDeaths.Get(killerId).Insert(death);
        }
        
        // Check each killer for camping behavior
        foreach (string killerId, array<ref STS_DeathLocation> deaths : killerDeaths)
        {
            // Skip if not enough kills
            if (deaths.Count() < minKills)
                continue;
                
            // Sort deaths by timestamp
            deaths.Sort(SortDeathsByTimestamp);
            
            // Check if kills are within the time window
            for (int i = 0; i <= deaths.Count() - minKills; i++)
            {
                int startTime = deaths[i].m_iTimestamp;
                int endTime = deaths[i + minKills - 1].m_iTimestamp;
                
                if ((endTime - startTime) <= timeWindow)
                {
                    // Check if killer stayed within the movement radius
                    vector minPos = deaths[i].m_vPosition;
                    vector maxPos = deaths[i].m_vPosition;
                    
                    for (int j = i; j < i + minKills; j++)
                    {
                        vector pos = deaths[j].m_vPosition;
                        if (vector.Distance(pos, minPos) > maxMovementRadius)
                        {
                            maxPos = pos;
                            if (vector.Distance(minPos, maxPos) > maxMovementRadius)
                                break;
                        }
                    }
                    
                    if (vector.Distance(minPos, maxPos) <= maxMovementRadius)
                    {
                        // Camping detected
                        m_bIsCampingSpot = true;
                        return true;
                    }
                }
            }
        }
        
        return false;
    }
    
    static int SortDeathsByTimestamp(STS_DeathLocation a, STS_DeathLocation b)
    {
        if (a.m_iTimestamp < b.m_iTimestamp) return -1;
        if (a.m_iTimestamp > b.m_iTimestamp) return 1;
        return 0;
    }
}

class STS_DeathConcentrationAnalysis
{
    // Singleton instance
    private static ref STS_DeathConcentrationAnalysis s_Instance;
    
    // Configuration
    protected ref STS_DeathConcentrationConfig m_Config;
    
    // References to other components
    protected ref STS_LoggingSystem m_Logger;
    protected ref STS_DatabaseManager m_DatabaseManager;
    
    // Clusters of death locations
    protected ref array<ref STS_Cluster> m_aClusters = new array<ref STS_Cluster>();
    
    // Raw death locations for heatmap generation
    protected ref array<ref STS_DeathLocation> m_aAllDeathLocations = new array<ref STS_DeathLocation>();
    
    // Map boundaries for normalization
    protected vector m_vMapMin = vector.Zero;
    protected vector m_vMapMax = vector.Zero;
    
    // Heatmap grid
    protected ref array<array<float>> m_aHeatmapGrid;
    
    // Last analysis time
    protected int m_iLastAnalysisTime = 0;
    
    // Analysis results
    protected ref array<ref STS_Cluster> m_aHotspots = new array<ref STS_Cluster>();
    protected ref array<ref STS_Cluster> m_aCampingSpots = new array<ref STS_Cluster>();
    
    //------------------------------------------------------------------------------------------------
    protected void STS_DeathConcentrationAnalysis()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Logger.LogInfo("Initializing Death Concentration Analysis System");
        
        // Initialize configuration
        m_Config = new STS_DeathConcentrationConfig();
        
        // Initialize heatmap grid
        InitializeHeatmapGrid();
        
        // Get map boundaries
        GetMapBoundaries();
        
        // Connect to the database
        m_DatabaseManager = STS_DatabaseManager.GetInstance();
        if (m_DatabaseManager)
        {
            LoadHistoricalData();
        }
        else
        {
            m_Logger.LogWarning("Database manager not available - historical death data won't be loaded");
        }
        
        // Register to run analysis periodically
        GetGame().GetCallqueue().CallLater(PeriodicAnalysis, 300000, true); // 300000ms = 5 minutes
    }
    
    //------------------------------------------------------------------------------------------------
    static STS_DeathConcentrationAnalysis GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_DeathConcentrationAnalysis();
        }
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize the heatmap grid
    protected void InitializeHeatmapGrid()
    {
        m_aHeatmapGrid = new array<array<float>>();
        for (int i = 0; i < m_Config.m_iHeatmapResolution; i++)
        {
            array<float> row = new array<float>();
            for (int j = 0; j < m_Config.m_iHeatmapResolution; j++)
            {
                row.Insert(0);
            }
            m_aHeatmapGrid.Insert(row);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the map boundaries from the world
    protected void GetMapBoundaries()
    {
        // Get map boundaries from the world
        // For Arma Reforger, we can use the World class to get the map size
        // This is a simplified version for demonstration
        float worldSize = 12800; // Example size, would be retrieved from World class
        
        m_vMapMin = vector.Zero;
        m_vMapMax = Vector(worldSize, 1000, worldSize); // Assuming square map with reasonable height
        
        m_Logger.LogInfo(string.Format("Map boundaries set to min %1, max %2", m_vMapMin.ToString(), m_vMapMax.ToString()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Load historical death data from database
    protected void LoadHistoricalData()
    {
        if (!m_DatabaseManager)
            return;
            
        try 
        {
            // Get death location data from database
            array<ref STS_DeathLocation> records = m_DatabaseManager.GetDeathRepository().GetRecentDeathLocations(1000); // Get the 1000 most recent deaths
            
            if (records && records.Count() > 0)
            {
                // Add each death location to our records
                foreach (STS_DeathLocation deathLocation : records)
                {
                    AddDeathLocation(deathLocation);
                }
                
                m_Logger.LogInfo(string.Format("Loaded %1 historical death location records", records.Count()));
                
                // Run initial analysis
                AnalyzeDeathClusters();
            }
            else
            {
                m_Logger.LogWarning("No historical death location data found");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception loading historical death location data: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a new death location
    void AddDeathLocation(STS_DeathLocation deathLocation)
    {
        // Add to raw death locations list for heatmap
        m_aAllDeathLocations.Insert(deathLocation);
        
        // Trim if we exceed the maximum
        if (m_aAllDeathLocations.Count() > m_Config.m_iMaxHeatmapEntries)
        {
            m_aAllDeathLocations.Remove(0);
        }
        
        // Find the nearest cluster or create a new one
        bool addedToCluster = false;
        foreach (STS_Cluster cluster : m_aClusters)
        {
            if (vector.Distance(cluster.m_vCenterPosition, deathLocation.m_vPosition) <= m_Config.m_fClusterRadius)
            {
                cluster.AddDeathLocation(deathLocation);
                addedToCluster = true;
                break;
            }
        }
        
        if (!addedToCluster)
        {
            // Create a new cluster
            STS_Cluster newCluster = new STS_Cluster();
            newCluster.m_vCenterPosition = deathLocation.m_vPosition;
            newCluster.AddDeathLocation(deathLocation);
            m_aClusters.Insert(newCluster);
            
            // Limit the number of clusters
            if (m_aClusters.Count() > m_Config.m_iMaxClusters)
            {
                // Find the smallest/oldest cluster to remove
                int oldestIndex = 0;
                int oldestTime = m_aClusters[0].m_iLastUpdateTime;
                
                for (int i = 1; i < m_aClusters.Count(); i++)
                {
                    if (m_aClusters[i].m_iLastUpdateTime < oldestTime)
                    {
                        oldestTime = m_aClusters[i].m_iLastUpdateTime;
                        oldestIndex = i;
                    }
                }
                
                m_aClusters.RemoveOrdered(oldestIndex);
            }
        }
        
        // Update the heatmap
        UpdateHeatmap(deathLocation);
        
        // Save to database if available
        if (m_DatabaseManager)
        {
            m_DatabaseManager.GetDeathRepository().SaveDeathLocation(deathLocation);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update the heatmap with a new death location
    protected void UpdateHeatmap(STS_DeathLocation deathLocation)
    {
        // Normalize the position to grid coordinates
        vector normalizedPos = NormalizePosition(deathLocation.m_vPosition);
        
        // Calculate grid indices
        int gridX = Math.Clamp(Math.Round(normalizedPos[0] * (m_Config.m_iHeatmapResolution - 1)), 0, m_Config.m_iHeatmapResolution - 1);
        int gridZ = Math.Clamp(Math.Round(normalizedPos[2] * (m_Config.m_iHeatmapResolution - 1)), 0, m_Config.m_iHeatmapResolution - 1);
        
        // Update heat value
        m_aHeatmapGrid[gridX][gridZ] += 1.0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Normalize a world position to 0-1 range
    protected vector NormalizePosition(vector worldPos)
    {
        vector size = m_vMapMax - m_vMapMin;
        vector normalized;
        
        normalized[0] = (worldPos[0] - m_vMapMin[0]) / size[0];
        normalized[1] = (worldPos[1] - m_vMapMin[1]) / size[1];
        normalized[2] = (worldPos[2] - m_vMapMin[2]) / size[2];
        
        return normalized;
    }
    
    //------------------------------------------------------------------------------------------------
    // Analyze death clusters to identify hotspots and camping spots
    protected void AnalyzeDeathClusters()
    {
        m_aHotspots.Clear();
        m_aCampingSpots.Clear();
        
        // Apply time decay to heat values
        ApplyHeatDecay();
        
        // Identify hotspots and camping spots
        foreach (STS_Cluster cluster : m_aClusters)
        {
            // Check if this is a hotspot
            if (cluster.GetDeathCount() >= m_Config.m_iMinDeathsForHotspot)
            {
                m_aHotspots.Insert(cluster);
            }
            
            // Check if this is a camping spot
            if (cluster.AnalyzeCampingBehavior(
                m_Config.m_fTimeWindowForCamping,
                m_Config.m_iKillsInWindowForCamping,
                m_Config.m_fMaxCamperMovementRadius))
            {
                m_aCampingSpots.Insert(cluster);
            }
        }
        
        // Sort hotspots by death count (highest first)
        m_aHotspots.Sort(SortClustersByDeathCount);
        
        // Update last analysis time
        m_iLastAnalysisTime = System.GetUnixTime();
        
        m_Logger.LogInfo(string.Format("Death cluster analysis complete. Found %1 hotspots and %2 camping spots.", 
            m_aHotspots.Count(), m_aCampingSpots.Count()));
    }
    
    //------------------------------------------------------------------------------------------------
    // Apply time-based decay to heat values
    protected void ApplyHeatDecay()
    {
        int currentTime = System.GetUnixTime();
        float minutesSinceLastAnalysis = (m_iLastAnalysisTime > 0) ? (currentTime - m_iLastAnalysisTime) / 60.0 : 0;
        
        if (minutesSinceLastAnalysis > 0)
        {
            float decayFactor = 1.0 - Math.Min(0.95, m_Config.m_fHeatDecayRate * minutesSinceLastAnalysis);
            
            // Apply decay to the heatmap grid
            for (int x = 0; x < m_Config.m_iHeatmapResolution; x++)
            {
                for (int z = 0; z < m_Config.m_iHeatmapResolution; z++)
                {
                    m_aHeatmapGrid[x][z] *= decayFactor;
                }
            }
            
            // Apply decay to clusters
            foreach (STS_Cluster cluster : m_aClusters)
            {
                cluster.m_fHeatValue *= decayFactor;
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Sort clusters by death count in descending order
    static int SortClustersByDeathCount(STS_Cluster a, STS_Cluster b)
    {
        if (a.GetDeathCount() > b.GetDeathCount()) return -1;
        if (a.GetDeathCount() < b.GetDeathCount()) return 1;
        return 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Run periodic analysis of death patterns
    protected void PeriodicAnalysis()
    {
        try
        {
            m_Logger.LogDebug("Running periodic death concentration analysis");
            AnalyzeDeathClusters();
            
            // Generate report if there are significant findings
            if (m_aHotspots.Count() > 0 || m_aCampingSpots.Count() > 0)
            {
                GenerateDeathAnalysisReport();
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception in periodic death analysis: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a report of the analysis results
    protected void GenerateDeathAnalysisReport()
    {
        string report = "=== Death Concentration Analysis Report ===\n\n";
        
        // Add hotspots section
        report += "Top " + Math.Min(5, m_aHotspots.Count()) + " Death Hotspots:\n";
        int count = 0;
        foreach (STS_Cluster hotspot : m_aHotspots)
        {
            if (count >= 5) break;
            
            report += string.Format("%1. Position: %2 - Deaths: %3 - Most common weapon: %4\n",
                count + 1,
                hotspot.m_vCenterPosition.ToString(),
                hotspot.GetDeathCount(),
                hotspot.GetMostCommonWeapon());
            count++;
        }
        
        // Add camping spots section
        if (m_aCampingSpots.Count() > 0)
        {
            report += "\nDetected Camping Spots:\n";
            count = 0;
            foreach (STS_Cluster campSpot : m_aCampingSpots)
            {
                report += string.Format("%1. Position: %2 - Most active camper: %3 - Weapon: %4\n",
                    count + 1,
                    campSpot.m_vCenterPosition.ToString(),
                    campSpot.GetMostActiveKiller(),
                    campSpot.GetMostCommonWeapon());
                count++;
            }
        }
        
        // Add timestamp
        report += "\nGenerated at: " + System.GetUnixTime() + "\n";
        
        // Log the report
        m_Logger.LogInfo(report);
        
        // Save report to file
        string reportPath = "$profile:StatTracker/Reports/DeathAnalysis_" + System.GetUnixTime() + ".txt";
        FileIO.MakeDirectory("$profile:StatTracker/Reports");
        FileHandle file = FileIO.OpenFile(reportPath, FileMode.WRITE);
        if (file)
        {
            file.WriteLine(report);
            file.Close();
        }
        
        // Notify admins if camping is detected
        if (m_aCampingSpots.Count() > 0)
        {
            STS_NotificationManager notificationManager = STS_NotificationManager.GetInstance();
            if (notificationManager)
            {
                notificationManager.NotifyAdmins("Camping Detected", 
                    string.Format("%1 potential camping spot(s) detected. Check admin panel for details.", m_aCampingSpots.Count()));
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get top death hotspots
    array<ref STS_Cluster> GetHotspots(int maxCount = 10)
    {
        array<ref STS_Cluster> result = new array<ref STS_Cluster>();
        
        // Return top N hotspots
        int count = Math.Min(maxCount, m_aHotspots.Count());
        for (int i = 0; i < count; i++)
        {
            result.Insert(m_aHotspots[i]);
        }
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get camping spots
    array<ref STS_Cluster> GetCampingSpots()
    {
        return m_aCampingSpots;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get heatmap data
    array<array<float>> GetHeatmapData()
    {
        return m_aHeatmapGrid;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate JSON data for the heatmap
    string GetHeatmapJSON()
    {
        string json = "{\"heatmap\":[";
        
        for (int x = 0; x < m_Config.m_iHeatmapResolution; x++)
        {
            json += "[";
            for (int z = 0; z < m_Config.m_iHeatmapResolution; z++)
            {
                json += m_aHeatmapGrid[x][z].ToString();
                if (z < m_Config.m_iHeatmapResolution - 1)
                    json += ",";
            }
            json += "]";
            if (x < m_Config.m_iHeatmapResolution - 1)
                json += ",";
        }
        
        json += "]}";
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Register a player death
    void RegisterPlayerDeath(int victimId, int killerId, vector deathPosition, vector killerPosition, string weaponName, int teamId)
    {
        try
        {
            // Create death location record
            STS_DeathLocation deathLocation = new STS_DeathLocation();
            deathLocation.m_vPosition = deathPosition;
            deathLocation.m_iTimestamp = System.GetUnixTime();
            deathLocation.m_sKillerPlayerId = killerId.ToString();
            deathLocation.m_sVictimPlayerId = victimId.ToString();
            deathLocation.m_sWeaponName = weaponName;
            deathLocation.m_iTeamId = teamId;
            
            // Calculate kill distance
            if (killerPosition != vector.Zero)
            {
                deathLocation.m_fKillDistance = vector.Distance(deathPosition, killerPosition);
            }
            
            // Add to our analysis
            AddDeathLocation(deathLocation);
            
            // Run analysis if we have accumulated enough data since last analysis
            int timeSinceLastAnalysis = System.GetUnixTime() - m_iLastAnalysisTime;
            if (timeSinceLastAnalysis > 300) // 5 minutes
            {
                AnalyzeDeathClusters();
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception registering player death: %1", e.ToString()));
        }
    }
}
