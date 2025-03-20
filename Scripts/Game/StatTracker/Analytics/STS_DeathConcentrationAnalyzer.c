// STS_DeathConcentrationAnalyzer.c
// Component for analyzing where deaths are occurring to identify potential camping spots

class STS_DeathLocation
{
    vector m_vPosition;    // World position
    float m_fTimestamp;    // When the death occurred
    string m_sKillerID;    // Who caused the death
    string m_sWeapon;      // Weapon used
    float m_fDistance;     // Kill distance if available
    
    void STS_DeathLocation(vector position, float timestamp, string killerID, string weapon, float distance = 0)
    {
        m_vPosition = position;
        m_fTimestamp = timestamp;
        m_sKillerID = killerID;
        m_sWeapon = weapon;
        m_fDistance = distance;
    }
}

class STS_HeatZone
{
    vector m_vCenter;                // Center of this heat zone
    int m_iDeathCount;               // Number of deaths in this zone
    float m_fRadius;                 // Radius of the zone in meters
    float m_fLastDeathTime;          // Time of last death
    ref array<string> m_aKillerIDs;  // IDs of killers in this zone
    
    void STS_HeatZone(vector center, float radius = 10.0)
    {
        m_vCenter = center;
        m_iDeathCount = 0;
        m_fRadius = radius;
        m_fLastDeathTime = 0;
        m_aKillerIDs = new array<string>();
    }
    
    // Add a death to this zone
    void AddDeath(STS_DeathLocation deathLocation)
    {
        m_iDeathCount++;
        m_fLastDeathTime = deathLocation.m_fTimestamp;
        
        // Track unique killers
        if (!m_aKillerIDs.Contains(deathLocation.m_sKillerID))
        {
            m_aKillerIDs.Insert(deathLocation.m_sKillerID);
        }
    }
    
    // Check if a position is within this zone
    bool ContainsPosition(vector position)
    {
        return vector.Distance(m_vCenter, position) <= m_fRadius;
    }
    
    // Get number of unique killers
    int GetUniqueKillerCount()
    {
        return m_aKillerIDs.Count();
    }
    
    // Is this zone fresh (recent deaths)
    bool IsFresh(float currentTime, float maxAge = 1800) // Default 30 minutes
    {
        return (currentTime - m_fLastDeathTime) < maxAge;
    }
    
    // Get a "camping probability" score
    float GetCampingProbabilityScore()
    {
        // More deaths from few unique killers = higher camping probability
        if (GetUniqueKillerCount() == 0) return 0;
        
        return Math.Clamp(m_iDeathCount / Math.Max(1, GetUniqueKillerCount()), 0, 1) * 100;
    }
}

class STS_DeathConcentrationAnalyzer
{
    // Singleton instance
    private static ref STS_DeathConcentrationAnalyzer s_Instance;
    
    // Reference to logging system
    protected STS_LoggingSystem m_Logger;
    
    // Recent death locations
    protected ref array<ref STS_DeathLocation> m_aRecentDeaths = new array<ref STS_DeathLocation>();
    
    // Identified hot zones
    protected ref array<ref STS_HeatZone> m_aHeatZones = new array<ref STS_HeatZone>();
    
    // File paths for persistence
    protected const string DEATH_LOCATIONS_PATH = "$profile:StatTracker/Analytics/death_locations.json";
    protected const string HEAT_ZONES_PATH = "$profile:StatTracker/Analytics/heat_zones.json";
    
    // Maximum death records to keep
    protected const int MAX_DEATH_RECORDS = 1000;
    
    // Analysis settings
    protected const float ZONE_RADIUS = 10.0; // 10 meter radius for death zones
    protected const float MAX_DEATH_AGE = 43200.0; // 12 hours
    protected const int MIN_DEATHS_FOR_HOTSPOT = 5; // Minimum deaths to consider a location a hotspot
    
    // Time of last analysis
    protected float m_fLastAnalysisTime = 0;
    
    // Analysis interval (5 minutes)
    protected const float ANALYSIS_INTERVAL = 300;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_DeathConcentrationAnalyzer()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        if (!m_Logger)
        {
            Print("[StatTracker] Failed to get logging system for Death Concentration Analyzer", LogLevel.ERROR);
            return;
        }
        
        m_Logger.LogInfo("Initializing Death Concentration Analysis System");
        
        // Create directory if it doesn't exist
        string dir = "$profile:StatTracker/Analytics";
        FileIO.MakeDirectory(dir);
        
        // Load existing data if available
        LoadDeathLocations();
        LoadHeatZones();
        
        // Start analysis timer
        GetGame().GetCallqueue().CallLater(PerformPeriodicAnalysis, 300000, true); // Check every 5 minutes
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_DeathConcentrationAnalyzer GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_DeathConcentrationAnalyzer();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Record a player death
    void RecordDeath(vector position, string killerID, string weapon, float distance = 0)
    {
        float timestamp = System.GetTickCount() / 1000.0;
        
        // Create death record
        STS_DeathLocation deathLocation = new STS_DeathLocation(position, timestamp, killerID, weapon, distance);
        
        // Add to recent deaths
        m_aRecentDeaths.Insert(deathLocation);
        
        // Trim if we have too many records
        while (m_aRecentDeaths.Count() > MAX_DEATH_RECORDS)
        {
            m_aRecentDeaths.Remove(0); // Remove oldest
        }
        
        // Save after several deaths or periodically
        if (m_aRecentDeaths.Count() % 10 == 0)
        {
            SaveDeathLocations();
        }
        
        // Check if it's time to re-analyze
        float currentTime = System.GetTickCount() / 1000.0;
        if (currentTime - m_fLastAnalysisTime > ANALYSIS_INTERVAL)
        {
            AnalyzeDeathConcentrations();
            m_fLastAnalysisTime = currentTime;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Analyze death concentrations to identify hot zones
    void AnalyzeDeathConcentrations()
    {
        m_Logger.LogInfo("Analyzing death concentrations");
        
        float currentTime = System.GetTickCount() / 1000.0;
        
        // Clear old heat zones
        m_aHeatZones.Clear();
        
        // Remove deaths that are too old
        int i = 0;
        while (i < m_aRecentDeaths.Count())
        {
            if (currentTime - m_aRecentDeaths[i].m_fTimestamp > MAX_DEATH_AGE)
            {
                m_aRecentDeaths.Remove(i);
            }
            else
            {
                i++;
            }
        }
        
        // Group deaths into zones
        foreach (STS_DeathLocation death : m_aRecentDeaths)
        {
            // Check if death belongs in an existing zone
            bool addedToZone = false;
            
            foreach (STS_HeatZone zone : m_aHeatZones)
            {
                if (zone.ContainsPosition(death.m_vPosition))
                {
                    zone.AddDeath(death);
                    addedToZone = true;
                    break;
                }
            }
            
            // If not, create a new zone
            if (!addedToZone)
            {
                STS_HeatZone newZone = new STS_HeatZone(death.m_vPosition, ZONE_RADIUS);
                newZone.AddDeath(death);
                m_aHeatZones.Insert(newZone);
            }
        }
        
        // Filter out zones with too few deaths
        i = 0;
        while (i < m_aHeatZones.Count())
        {
            if (m_aHeatZones[i].m_iDeathCount < MIN_DEATHS_FOR_HOTSPOT)
            {
                m_aHeatZones.Remove(i);
            }
            else
            {
                i++;
            }
        }
        
        // Sort zones by death count (highest first)
        m_aHeatZones.Sort(ZoneComparer);
        
        // Log results
        m_Logger.LogInfo(string.Format("Found %1 significant death concentration areas", m_aHeatZones.Count()));
        
        // Save hot zones
        SaveHeatZones();
    }
    
    //------------------------------------------------------------------------------------------------
    // Perform periodic analysis
    protected void PerformPeriodicAnalysis()
    {
        float currentTime = System.GetTickCount() / 1000.0;
        
        // Only analyze at intervals
        if (currentTime - m_fLastAnalysisTime < ANALYSIS_INTERVAL)
            return;
            
        AnalyzeDeathConcentrations();
        m_fLastAnalysisTime = currentTime;
    }
    
    //------------------------------------------------------------------------------------------------
    // Comparer function for sorting heat zones
    static int ZoneComparer(STS_HeatZone a, STS_HeatZone b)
    {
        if (a.m_iDeathCount > b.m_iDeathCount) return -1;
        if (a.m_iDeathCount < b.m_iDeathCount) return 1;
        return 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Save death locations to file
    protected void SaveDeathLocations()
    {
        // Cap the number of death locations we save
        int startIndex = Math.Max(0, m_aRecentDeaths.Count() - MAX_DEATH_RECORDS);
        
        string json = "[";
        
        for (int i = startIndex; i < m_aRecentDeaths.Count(); i++)
        {
            STS_DeathLocation death = m_aRecentDeaths[i];
            
            json += "{";
            json += "\"position\":[" + death.m_vPosition[0].ToString() + "," + death.m_vPosition[1].ToString() + "," + death.m_vPosition[2].ToString() + "],";
            json += "\"timestamp\":" + death.m_fTimestamp.ToString() + ",";
            json += "\"killerID\":\"" + death.m_sKillerID + "\",";
            json += "\"weapon\":\"" + death.m_sWeapon + "\",";
            json += "\"distance\":" + death.m_fDistance.ToString();
            json += "}";
            
            if (i < m_aRecentDeaths.Count() - 1) json += ",";
        }
        
        json += "]";
        
        // Save to file
        FileHandle file = FileIO.OpenFile(DEATH_LOCATIONS_PATH, FileMode.WRITE);
        if (file != 0)
        {
            FileIO.WriteFile(file, json);
            FileIO.CloseFile(file);
            m_Logger.LogInfo("Death location data saved successfully");
        }
        else
        {
            m_Logger.LogError("Failed to save death location data");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load death locations from file
    protected void LoadDeathLocations()
    {
        if (!FileIO.FileExists(DEATH_LOCATIONS_PATH))
        {
            m_Logger.LogInfo("No previous death location data found");
            return;
        }
        
        string content;
        FileHandle file = FileIO.OpenFile(DEATH_LOCATIONS_PATH, FileMode.READ);
        if (file != 0)
        {
            content = FileIO.ReadFile(file);
            FileIO.CloseFile(file);
            
            try
            {
                JsonSerializer js = new JsonSerializer();
                string error;
                
                // Parse the array of death locations
                ref array<ref map<string, Variant>> deathData = new array<ref map<string, Variant>>();
                
                bool success = js.ReadFromString(deathData, content, error);
                if (success && deathData)
                {
                    m_aRecentDeaths.Clear();
                    
                    foreach (map<string, Variant> data : deathData)
                    {
                        // Parse position
                        vector position = vector.Zero;
                        if (data.Contains("position"))
                        {
                            array<float> posArray = data.Get("position").AsArray();
                            if (posArray && posArray.Count() >= 3)
                            {
                                position[0] = posArray[0];
                                position[1] = posArray[1];
                                position[2] = posArray[2];
                            }
                        }
                        
                        // Get other properties
                        float timestamp = data.Contains("timestamp") ? data.Get("timestamp").AsFloat() : 0;
                        string killerID = data.Contains("killerID") ? data.Get("killerID").AsString() : "";
                        string weapon = data.Contains("weapon") ? data.Get("weapon").AsString() : "";
                        float distance = data.Contains("distance") ? data.Get("distance").AsFloat() : 0;
                        
                        // Create and add death location
                        STS_DeathLocation death = new STS_DeathLocation(position, timestamp, killerID, weapon, distance);
                        m_aRecentDeaths.Insert(death);
                    }
                    
                    m_Logger.LogInfo(string.Format("Successfully loaded %1 death locations", m_aRecentDeaths.Count()));
                }
                else
                {
                    m_Logger.LogError("Failed to parse death location data: " + error);
                }
            }
            catch (Exception e)
            {
                m_Logger.LogError("Exception loading death location data: " + e.ToString());
            }
        }
        else
        {
            m_Logger.LogError("Failed to open death location file");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save heat zones to file
    protected void SaveHeatZones()
    {
        string json = "[";
        
        for (int i = 0; i < m_aHeatZones.Count(); i++)
        {
            STS_HeatZone zone = m_aHeatZones[i];
            
            json += "{";
            json += "\"center\":[" + zone.m_vCenter[0].ToString() + "," + zone.m_vCenter[1].ToString() + "," + zone.m_vCenter[2].ToString() + "],";
            json += "\"deathCount\":" + zone.m_iDeathCount.ToString() + ",";
            json += "\"radius\":" + zone.m_fRadius.ToString() + ",";
            json += "\"lastDeathTime\":" + zone.m_fLastDeathTime.ToString() + ",";
            
            // Killers array
            json += "\"killerIDs\":[";
            for (int j = 0; j < zone.m_aKillerIDs.Count(); j++)
            {
                json += "\"" + zone.m_aKillerIDs[j] + "\"";
                if (j < zone.m_aKillerIDs.Count() - 1) json += ",";
            }
            json += "],";
            
            json += "\"campingProbability\":" + zone.GetCampingProbabilityScore().ToString();
            json += "}";
            
            if (i < m_aHeatZones.Count() - 1) json += ",";
        }
        
        json += "]";
        
        // Save to file
        FileHandle file = FileIO.OpenFile(HEAT_ZONES_PATH, FileMode.WRITE);
        if (file != 0)
        {
            FileIO.WriteFile(file, json);
            FileIO.CloseFile(file);
            m_Logger.LogInfo("Heat zone data saved successfully");
        }
        else
        {
            m_Logger.LogError("Failed to save heat zone data");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load heat zones from file
    protected void LoadHeatZones()
    {
        if (!FileIO.FileExists(HEAT_ZONES_PATH))
        {
            m_Logger.LogInfo("No previous heat zone data found");
            return;
        }
        
        string content;
        FileHandle file = FileIO.OpenFile(HEAT_ZONES_PATH, FileMode.READ);
        if (file != 0)
        {
            content = FileIO.ReadFile(file);
            FileIO.CloseFile(file);
            
            try
            {
                JsonSerializer js = new JsonSerializer();
                string error;
                
                // Parse the array of heat zones
                ref array<ref map<string, Variant>> zoneData = new array<ref map<string, Variant>>();
                
                bool success = js.ReadFromString(zoneData, content, error);
                if (success && zoneData)
                {
                    m_aHeatZones.Clear();
                    
                    foreach (map<string, Variant> data : zoneData)
                    {
                        // Parse center position
                        vector center = vector.Zero;
                        if (data.Contains("center"))
                        {
                            array<float> posArray = data.Get("center").AsArray();
                            if (posArray && posArray.Count() >= 3)
                            {
                                center[0] = posArray[0];
                                center[1] = posArray[1];
                                center[2] = posArray[2];
                            }
                        }
                        
                        // Create zone
                        float radius = data.Contains("radius") ? data.Get("radius").AsFloat() : ZONE_RADIUS;
                        STS_HeatZone zone = new STS_HeatZone(center, radius);
                        
                        // Set properties
                        zone.m_iDeathCount = data.Contains("deathCount") ? data.Get("deathCount").AsInt() : 0;
                        zone.m_fLastDeathTime = data.Contains("lastDeathTime") ? data.Get("lastDeathTime").AsFloat() : 0;
                        
                        // Get killer IDs
                        if (data.Contains("killerIDs"))
                        {
                            array<string> killerIDs = data.Get("killerIDs").AsArray();
                            if (killerIDs)
                            {
                                zone.m_aKillerIDs.Clear();
                                foreach (string id : killerIDs)
                                {
                                    zone.m_aKillerIDs.Insert(id);
                                }
                            }
                        }
                        
                        m_aHeatZones.Insert(zone);
                    }
                    
                    m_Logger.LogInfo(string.Format("Successfully loaded %1 heat zones", m_aHeatZones.Count()));
                }
                else
                {
                    m_Logger.LogError("Failed to parse heat zone data: " + error);
                }
            }
            catch (Exception e)
            {
                m_Logger.LogError("Exception loading heat zone data: " + e.ToString());
            }
        }
        else
        {
            m_Logger.LogError("Failed to open heat zone file");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all heat zones
    array<ref STS_HeatZone> GetHeatZones()
    {
        return m_aHeatZones;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get active heat zones (recent deaths)
    array<ref STS_HeatZone> GetActiveHeatZones(float maxAge = 1800) // Default 30 minutes
    {
        float currentTime = System.GetTickCount() / 1000.0;
        array<ref STS_HeatZone> activeZones = new array<ref STS_HeatZone>();
        
        foreach (STS_HeatZone zone : m_aHeatZones)
        {
            if (zone.IsFresh(currentTime, maxAge))
            {
                activeZones.Insert(zone);
            }
        }
        
        return activeZones;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get potential camping spots (high probability zones)
    array<ref STS_HeatZone> GetPotentialCampingSpots(float minProbability = 70.0)
    {
        array<ref STS_HeatZone> campingSpots = new array<ref STS_HeatZone>();
        
        foreach (STS_HeatZone zone : m_aHeatZones)
        {
            if (zone.GetCampingProbabilityScore() >= minProbability)
            {
                campingSpots.Insert(zone);
            }
        }
        
        return campingSpots;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a text report of potential camping spots
    string GetCampingSpotReport()
    {
        array<ref STS_HeatZone> campingSpots = GetPotentialCampingSpots();
        
        if (campingSpots.Count() == 0)
        {
            return "No significant camping spots detected.";
        }
        
        string report = "Potential camping spots detected:\n\n";
        
        for (int i = 0; i < campingSpots.Count(); i++)
        {
            STS_HeatZone zone = campingSpots[i];
            
            report += string.Format("%1. Position: [%2, %3, %4]\n", 
                i + 1, 
                Math.Round(zone.m_vCenter[0]), 
                Math.Round(zone.m_vCenter[1]), 
                Math.Round(zone.m_vCenter[2]));
                
            report += string.Format("   Deaths: %1, Unique killers: %2\n", 
                zone.m_iDeathCount, 
                zone.GetUniqueKillerCount());
                
            report += string.Format("   Camping probability: %1%\n\n", 
                Math.Round(zone.GetCampingProbabilityScore()));
        }
        
        return report;
    }
} 