// STS_AnalyticsManager.c
// Advanced analytics manager for trend analysis and statistical processing

class STS_AnalyticsTimeBucket
{
    int m_iHour;            // Hour of day (0-23)
    int m_iDayOfWeek;       // Day of week (0-6, Sunday=0)
    int m_iPlayerCount;     // Total player count for this bucket
    int m_iSampleCount;     // Number of samples for this bucket
    
    void STS_AnalyticsTimeBucket(int hour, int dayOfWeek)
    {
        m_iHour = hour;
        m_iDayOfWeek = dayOfWeek;
        m_iPlayerCount = 0;
        m_iSampleCount = 0;
    }
    
    float GetAveragePlayerCount()
    {
        if (m_iSampleCount == 0)
            return 0;
            
        return m_iPlayerCount / float(m_iSampleCount);
    }
    
    void AddSample(int playerCount)
    {
        m_iPlayerCount += playerCount;
        m_iSampleCount++;
    }
    
    void Reset()
    {
        m_iPlayerCount = 0;
        m_iSampleCount = 0;
    }
}

class STS_DeathHeatPoint
{
    vector m_vPosition;          // 3D world position
    int m_iDeathCount;           // Death count at this position
    float m_fRadius;             // Radius for clustering
    ref array<string> m_aWeapons; // Weapons used
    ref map<int, int> m_aPlayerIDs; // Player IDs and their death count
    
    void STS_DeathHeatPoint(vector position, float radius = 5.0)
    {
        m_vPosition = position;
        m_iDeathCount = 0;
        m_fRadius = radius;
        m_aWeapons = new array<string>();
        m_aPlayerIDs = new map<int, int>();
    }
    
    bool IsInRadius(vector position)
    {
        return vector.Distance(m_vPosition, position) <= m_fRadius;
    }
    
    void AddDeath(int playerID, string weapon = "")
    {
        m_iDeathCount++;
        
        // Track player ID
        if (m_aPlayerIDs.Contains(playerID))
            m_aPlayerIDs.Set(playerID, m_aPlayerIDs.Get(playerID) + 1);
        else
            m_aPlayerIDs.Set(playerID, 1);
            
        // Track weapon
        if (weapon != "" && !m_aWeapons.Contains(weapon))
            m_aWeapons.Insert(weapon);
    }
    
    // Check if this point qualifies as a camping spot
    bool IsPotentialCampingSpot()
    {
        // At least 5 deaths to be considered a camping spot
        if (m_iDeathCount < 5)
            return false;
            
        // At least 3 unique victims
        if (m_aPlayerIDs.Count() < 3)
            return false;
            
        return true;
    }
    
    // Get percentage of deaths for specific player
    float GetPlayerDeathPercentage(int playerID)
    {
        if (!m_aPlayerIDs.Contains(playerID) || m_iDeathCount == 0)
            return 0;
            
        return m_aPlayerIDs.Get(playerID) / float(m_iDeathCount);
    }
    
    string GetAnalysisString()
    {
        string result = string.Format("Position: %1, Deaths: %2, Unique victims: %3", 
            m_vPosition.ToString(), m_iDeathCount, m_aPlayerIDs.Count());
            
        if (m_aWeapons.Count() > 0)
        {
            result += ", Weapons: ";
            for (int i = 0; i < m_aWeapons.Count(); i++)
            {
                result += m_aWeapons[i];
                if (i < m_aWeapons.Count() - 1)
                    result += ", ";
            }
        }
        
        return result;
    }
    
    string ToJSON()
    {
        string json = "{";
        json += "\"position\":[" + m_vPosition[0].ToString() + "," + m_vPosition[1].ToString() + "," + m_vPosition[2].ToString() + "],";
        json += "\"deathCount\":" + m_iDeathCount.ToString() + ",";
        json += "\"radius\":" + m_fRadius.ToString() + ",";
        
        json += "\"weapons\":[";
        for (int i = 0; i < m_aWeapons.Count(); i++)
        {
            json += "\"" + m_aWeapons[i] + "\"";
            if (i < m_aWeapons.Count() - 1)
                json += ",";
        }
        json += "],";
        
        json += "\"players\":{";
        int idx = 0;
        foreach (int playerID, int count : m_aPlayerIDs)
        {
            json += "\"" + playerID.ToString() + "\":" + count.ToString();
            if (idx < m_aPlayerIDs.Count() - 1)
                json += ",";
            idx++;
        }
        json += "}";
        
        json += "}";
        return json;
    }
}

class STS_AnalyticsManager
{
    // Singleton instance
    private static ref STS_AnalyticsManager s_Instance;
    
    // Logger reference
    protected STS_LoggingSystem m_Logger;
    
    // Config reference
    protected STS_Config m_Config;
    
    // Player count data for forecasting
    protected ref map<string, ref STS_AnalyticsTimeBucket> m_PlayerCountBuckets = new map<string, ref STS_AnalyticsTimeBucket>();
    
    // Death heatmap data
    protected ref array<ref STS_DeathHeatPoint> m_DeathHeatPoints = new array<ref STS_DeathHeatPoint>();
    
    // Tracking data
    protected float m_fLastSampleTime;
    protected float m_fLastAnalysisTime;
    protected float m_fLastDataSaveTime;
    
    // Constants
    protected const float SAMPLE_INTERVAL = 600.0;      // 10 minutes
    protected const float ANALYSIS_INTERVAL = 3600.0;   // 1 hour
    protected const float SAVE_INTERVAL = 900.0;        // 15 minutes
    protected const string DATA_FILENAME = "$profile:StatTracker/Analytics/player_counts.json";
    protected const string DEATH_DATA_FILENAME = "$profile:StatTracker/Analytics/death_points.json";
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_AnalyticsManager()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Config = STS_Config.GetInstance();
        
        m_fLastSampleTime = 0;
        m_fLastAnalysisTime = 0;
        m_fLastDataSaveTime = 0;
        
        // Initialize time buckets
        InitializeTimeBuckets();
        
        // Load existing data
        LoadData();
        
        // Start periodic update
        GetGame().GetCallqueue().CallLater(Update, 60000, true); // Update every minute
        
        m_Logger.LogInfo("Analytics Manager initialized", "STS_AnalyticsManager", "Constructor");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_AnalyticsManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_AnalyticsManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize the time buckets for each hour and day of the week
    protected void InitializeTimeBuckets()
    {
        for (int day = 0; day < 7; day++)
        {
            for (int hour = 0; hour < 24; hour++)
            {
                string key = day.ToString() + ":" + hour.ToString();
                m_PlayerCountBuckets.Set(key, new STS_AnalyticsTimeBucket(hour, day));
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update function called periodically
    void Update()
    {
        float currentTime = System.GetTickCount() / 1000.0;
        
        // Take a sample of player count
        if (currentTime - m_fLastSampleTime >= SAMPLE_INTERVAL)
        {
            TakePlayerCountSample();
            m_fLastSampleTime = currentTime;
        }
        
        // Run analysis
        if (currentTime - m_fLastAnalysisTime >= ANALYSIS_INTERVAL)
        {
            AnalyzeData();
            m_fLastAnalysisTime = currentTime;
        }
        
        // Save data
        if (currentTime - m_fLastDataSaveTime >= SAVE_INTERVAL)
        {
            SaveData();
            m_fLastDataSaveTime = currentTime;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Take a sample of the current player count
    protected void TakePlayerCountSample()
    {
        try
        {
            // Get current player count
            int playerCount = GetPlayerCount();
            
            // Get current day and hour
            int day, hour;
            GetCurrentDayAndHour(day, hour);
            
            // Add to the appropriate bucket
            string key = day.ToString() + ":" + hour.ToString();
            STS_AnalyticsTimeBucket bucket = m_PlayerCountBuckets.Get(key);
            if (bucket)
            {
                bucket.AddSample(playerCount);
                m_Logger.LogDebug(string.Format("Added player count sample: day=%1, hour=%2, count=%3", 
                    day, hour, playerCount), "STS_AnalyticsManager", "TakePlayerCountSample");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception taking player count sample: " + e.ToString(),
                "STS_AnalyticsManager", "TakePlayerCountSample");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Analyze the collected data
    protected void AnalyzeData()
    {
        try
        {
            m_Logger.LogInfo("Running data analysis", "STS_AnalyticsManager", "AnalyzeData");
            
            // Analyze player count trends
            AnalyzePlayerCountTrends();
            
            // Analyze death heatmap data
            AnalyzeDeathHeatmap();
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception analyzing data: " + e.ToString(),
                "STS_AnalyticsManager", "AnalyzeData");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Analyze player count trends
    protected void AnalyzePlayerCountTrends()
    {
        try
        {
            m_Logger.LogDebug("Analyzing player count trends", "STS_AnalyticsManager", "AnalyzePlayerCountTrends");
            
            // Find peak hours for each day
            for (int day = 0; day < 7; day++)
            {
                float maxAvg = 0;
                int peakHour = -1;
                
                for (int hour = 0; hour < 24; hour++)
                {
                    string key = day.ToString() + ":" + hour.ToString();
                    STS_AnalyticsTimeBucket bucket = m_PlayerCountBuckets.Get(key);
                    if (bucket && bucket.GetAveragePlayerCount() > maxAvg)
                    {
                        maxAvg = bucket.GetAveragePlayerCount();
                        peakHour = hour;
                    }
                }
                
                if (peakHour >= 0)
                {
                    m_Logger.LogInfo(string.Format("Peak hour for day %1: %2:00 with avg %.1f players", 
                        GetDayName(day), peakHour, maxAvg), "STS_AnalyticsManager", "AnalyzePlayerCountTrends");
                }
            }
            
            // Find overall peak day
            float maxDayAvg = 0;
            int peakDay = -1;
            
            for (int day = 0; day < 7; day++)
            {
                float dayTotal = 0;
                int validHours = 0;
                
                for (int hour = 0; hour < 24; hour++)
                {
                    string key = day.ToString() + ":" + hour.ToString();
                    STS_AnalyticsTimeBucket bucket = m_PlayerCountBuckets.Get(key);
                    if (bucket && bucket.GetAveragePlayerCount() > 0)
                    {
                        dayTotal += bucket.GetAveragePlayerCount();
                        validHours++;
                    }
                }
                
                if (validHours > 0)
                {
                    float dayAvg = dayTotal / validHours;
                    if (dayAvg > maxDayAvg)
                    {
                        maxDayAvg = dayAvg;
                        peakDay = day;
                    }
                }
            }
            
            if (peakDay >= 0)
            {
                m_Logger.LogInfo(string.Format("Peak day of week: %1 with avg %.1f players", 
                    GetDayName(peakDay), maxDayAvg), "STS_AnalyticsManager", "AnalyzePlayerCountTrends");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception analyzing player count trends: " + e.ToString(),
                "STS_AnalyticsManager", "AnalyzePlayerCountTrends");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Analyze death heatmap
    protected void AnalyzeDeathHeatmap()
    {
        try
        {
            m_Logger.LogDebug("Analyzing death heat points: " + m_DeathHeatPoints.Count() + " points", 
                "STS_AnalyticsManager", "AnalyzeDeathHeatmap");
            
            // Find potential camping spots
            int campingSpots = 0;
            
            foreach (STS_DeathHeatPoint point : m_DeathHeatPoints)
            {
                if (point.IsPotentialCampingSpot())
                {
                    campingSpots++;
                    m_Logger.LogInfo("Potential camping spot identified: " + point.GetAnalysisString(),
                        "STS_AnalyticsManager", "AnalyzeDeathHeatmap");
                }
            }
            
            m_Logger.LogInfo("Death hotspot analysis complete. Found " + campingSpots + " potential camping spots",
                "STS_AnalyticsManager", "AnalyzeDeathHeatmap");
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception analyzing death heatmap: " + e.ToString(),
                "STS_AnalyticsManager", "AnalyzeDeathHeatmap");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Record a player death
    void RecordPlayerDeath(int playerID, vector position, string weapon = "")
    {
        try
        {
            // Check if the position is near an existing heat point
            bool pointFound = false;
            
            foreach (STS_DeathHeatPoint point : m_DeathHeatPoints)
            {
                if (point.IsInRadius(position))
                {
                    point.AddDeath(playerID, weapon);
                    pointFound = true;
                    break;
                }
            }
            
            // If not, create a new heat point
            if (!pointFound)
            {
                STS_DeathHeatPoint newPoint = new STS_DeathHeatPoint(position);
                newPoint.AddDeath(playerID, weapon);
                m_DeathHeatPoints.Insert(newPoint);
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception recording player death: " + e.ToString(),
                "STS_AnalyticsManager", "RecordPlayerDeath");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save the analytics data
    protected void SaveData()
    {
        try
        {
            // Save player count data
            SavePlayerCountData();
            
            // Save death heat points
            SaveDeathHeatPoints();
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception saving analytics data: " + e.ToString(),
                "STS_AnalyticsManager", "SaveData");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save player count data
    protected void SavePlayerCountData()
    {
        try
        {
            // Create directories if needed
            string dirPath = "$profile:StatTracker/Analytics/";
            if (!FileIO.FileExists(dirPath))
            {
                FileIO.MakeDirectory(dirPath);
            }
            
            // Build JSON data
            string json = "{\"playerCounts\":{";
            
            int bucketCount = 0;
            foreach (string key, STS_AnalyticsTimeBucket bucket : m_PlayerCountBuckets)
            {
                if (bucket.m_iSampleCount > 0)
                {
                    if (bucketCount > 0)
                        json += ",";
                        
                    json += "\"" + key + "\":{";
                    json += "\"day\":" + bucket.m_iDayOfWeek.ToString() + ",";
                    json += "\"hour\":" + bucket.m_iHour.ToString() + ",";
                    json += "\"playerCount\":" + bucket.m_iPlayerCount.ToString() + ",";
                    json += "\"sampleCount\":" + bucket.m_iSampleCount.ToString() + ",";
                    json += "\"average\":" + bucket.GetAveragePlayerCount().ToString();
                    json += "}";
                    
                    bucketCount++;
                }
            }
            
            json += "}}";
            
            // Write to file
            FileHandle file = FileIO.OpenFile(DATA_FILENAME, FileMode.WRITE);
            if (file)
            {
                FileIO.FPrintln(file, json);
                FileIO.CloseFile(file);
                m_Logger.LogDebug("Player count data saved to " + DATA_FILENAME,
                    "STS_AnalyticsManager", "SavePlayerCountData");
            }
            else
            {
                m_Logger.LogError("Failed to open file for writing: " + DATA_FILENAME,
                    "STS_AnalyticsManager", "SavePlayerCountData");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception saving player count data: " + e.ToString(),
                "STS_AnalyticsManager", "SavePlayerCountData");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save death heat points
    protected void SaveDeathHeatPoints()
    {
        try
        {
            // Create directories if needed
            string dirPath = "$profile:StatTracker/Analytics/";
            if (!FileIO.FileExists(dirPath))
            {
                FileIO.MakeDirectory(dirPath);
            }
            
            // Build JSON data
            string json = "{\"deathPoints\":[";
            
            for (int i = 0; i < m_DeathHeatPoints.Count(); i++)
            {
                if (i > 0)
                    json += ",";
                    
                json += m_DeathHeatPoints[i].ToJSON();
            }
            
            json += "]}";
            
            // Write to file
            FileHandle file = FileIO.OpenFile(DEATH_DATA_FILENAME, FileMode.WRITE);
            if (file)
            {
                FileIO.FPrintln(file, json);
                FileIO.CloseFile(file);
                m_Logger.LogDebug("Death heat points data saved to " + DEATH_DATA_FILENAME,
                    "STS_AnalyticsManager", "SaveDeathHeatPoints");
            }
            else
            {
                m_Logger.LogError("Failed to open file for writing: " + DEATH_DATA_FILENAME,
                    "STS_AnalyticsManager", "SaveDeathHeatPoints");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception saving death heat points: " + e.ToString(),
                "STS_AnalyticsManager", "SaveDeathHeatPoints");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load analytics data
    protected void LoadData()
    {
        try
        {
            // Load player count data
            LoadPlayerCountData();
            
            // Load death heat points
            LoadDeathHeatPoints();
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception loading analytics data: " + e.ToString(),
                "STS_AnalyticsManager", "LoadData");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load player count data
    protected void LoadPlayerCountData()
    {
        try
        {
            // Check if file exists
            if (!FileIO.FileExists(DATA_FILENAME))
            {
                m_Logger.LogInfo("No player count data file found", "STS_AnalyticsManager", "LoadPlayerCountData");
                return;
            }
            
            // Read file
            string json = ReadFileContent(DATA_FILENAME);
            if (json.IsEmpty())
            {
                m_Logger.LogError("Failed to read player count data file", "STS_AnalyticsManager", "LoadPlayerCountData");
                return;
            }
            
            // Parse JSON (simplified parser for demonstration)
            if (json.IndexOf("\"playerCounts\":{") >= 0)
            {
                // Reset all buckets
                foreach (string key, STS_AnalyticsTimeBucket bucket : m_PlayerCountBuckets)
                {
                    bucket.Reset();
                }
                
                // Extract each bucket data
                // Note: In a real implementation, use a proper JSON parser
                // This is a simplified approach for demonstration purposes
                
                int dataStartPos = json.IndexOf("\"playerCounts\":{") + 15;
                int dataEndPos = json.LastIndexOf("}}");
                if (dataStartPos > 0 && dataEndPos > dataStartPos)
                {
                    string bucketsData = json.Substring(dataStartPos, dataEndPos - dataStartPos);
                    array<string> bucketEntries = new array<string>();
                    
                    // Split by entries (this is very simplified)
                    int currentPos = 0;
                    while (currentPos < bucketsData.Length())
                    {
                        int nextKeyStart = bucketsData.IndexOfFrom("\"", currentPos);
                        if (nextKeyStart < 0) break;
                        
                        int keyEnd = bucketsData.IndexOfFrom("\":{", nextKeyStart + 1);
                        if (keyEnd < 0) break;
                        
                        int entryEnd = bucketsData.IndexOfFrom("}", keyEnd + 2);
                        if (entryEnd < 0) break;
                        
                        string key = bucketsData.Substring(nextKeyStart + 1, keyEnd - nextKeyStart - 1);
                        string entryData = bucketsData.Substring(keyEnd + 2, entryEnd - keyEnd - 1);
                        
                        // Parse the entry data
                        map<string, string> entryValues = new map<string, string>();
                        array<string> valuePairs = new array<string>();
                        entryData.Split(",", valuePairs);
                        
                        foreach (string pair : valuePairs)
                        {
                            array<string> keyValue = new array<string>();
                            pair.Split(":", keyValue);
                            if (keyValue.Count() == 2)
                            {
                                string valKey = keyValue[0].Replace("\"", "");
                                entryValues.Set(valKey, keyValue[1]);
                            }
                        }
                        
                        // Update bucket if we have the required data
                        if (entryValues.Contains("playerCount") && entryValues.Contains("sampleCount"))
                        {
                            STS_AnalyticsTimeBucket bucket = m_PlayerCountBuckets.Get(key);
                            if (bucket)
                            {
                                bucket.m_iPlayerCount = entryValues.Get("playerCount").ToInt();
                                bucket.m_iSampleCount = entryValues.Get("sampleCount").ToInt();
                            }
                        }
                        
                        currentPos = entryEnd + 1;
                    }
                }
                
                m_Logger.LogInfo("Player count data loaded successfully", "STS_AnalyticsManager", "LoadPlayerCountData");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception loading player count data: " + e.ToString(),
                "STS_AnalyticsManager", "LoadPlayerCountData");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load death heat points
    protected void LoadDeathHeatPoints()
    {
        try
        {
            // Check if file exists
            if (!FileIO.FileExists(DEATH_DATA_FILENAME))
            {
                m_Logger.LogInfo("No death heat points data file found", "STS_AnalyticsManager", "LoadDeathHeatPoints");
                return;
            }
            
            // Read file
            string json = ReadFileContent(DEATH_DATA_FILENAME);
            if (json.IsEmpty())
            {
                m_Logger.LogError("Failed to read death heat points data file", "STS_AnalyticsManager", "LoadDeathHeatPoints");
                return;
            }
            
            // Parse JSON (simplified parser for demonstration)
            if (json.IndexOf("\"deathPoints\":[") >= 0)
            {
                // Clear existing points
                m_DeathHeatPoints.Clear();
                
                // Extract death points data
                // Note: In a real implementation, use a proper JSON parser
                
                int dataStartPos = json.IndexOf("\"deathPoints\":[") + 14;
                int dataEndPos = json.LastIndexOf("]}");
                if (dataStartPos > 0 && dataEndPos > dataStartPos)
                {
                    string pointsData = json.Substring(dataStartPos, dataEndPos - dataStartPos);
                    
                    // Split by entries (this is very simplified)
                    int currentPos = 0;
                    while (currentPos < pointsData.Length())
                    {
                        int entryStart = pointsData.IndexOfFrom("{", currentPos);
                        if (entryStart < 0) break;
                        
                        int entryEnd = pointsData.IndexOfFrom("}", entryStart);
                        if (entryEnd < 0) break;
                        
                        string entryData = pointsData.Substring(entryStart, entryEnd - entryStart + 1);
                        
                        // Parse position
                        if (entryData.IndexOf("\"position\":[") >= 0)
                        {
                            int posStart = entryData.IndexOf("\"position\":[") + 12;
                            int posEnd = entryData.IndexOf("]", posStart);
                            if (posStart > 0 && posEnd > posStart)
                            {
                                string posData = entryData.Substring(posStart, posEnd - posStart);
                                array<string> coordinates = new array<string>();
                                posData.Split(",", coordinates);
                                
                                if (coordinates.Count() >= 3)
                                {
                                    vector position = Vector(coordinates[0].ToFloat(), coordinates[1].ToFloat(), coordinates[2].ToFloat());
                                    
                                    // Create a new heat point
                                    STS_DeathHeatPoint point = new STS_DeathHeatPoint(position);
                                    
                                    // Extract death count if available
                                    if (entryData.IndexOf("\"deathCount\":") >= 0)
                                    {
                                        int countStart = entryData.IndexOf("\"deathCount\":") + 13;
                                        int countEnd = entryData.IndexOf(",", countStart);
                                        if (countStart > 0 && countEnd > countStart)
                                        {
                                            string countStr = entryData.Substring(countStart, countEnd - countStart);
                                            point.m_iDeathCount = countStr.ToInt();
                                        }
                                    }
                                    
                                    // Extract radius if available
                                    if (entryData.IndexOf("\"radius\":") >= 0)
                                    {
                                        int radiusStart = entryData.IndexOf("\"radius\":") + 9;
                                        int radiusEnd = entryData.IndexOf(",", radiusStart);
                                        if (radiusStart > 0 && radiusEnd > radiusStart)
                                        {
                                            string radiusStr = entryData.Substring(radiusStart, radiusEnd - radiusStart);
                                            point.m_fRadius = radiusStr.ToFloat();
                                        }
                                    }
                                    
                                    // Add to the array
                                    m_DeathHeatPoints.Insert(point);
                                }
                            }
                        }
                        
                        currentPos = entryEnd + 1;
                    }
                }
                
                m_Logger.LogInfo("Death heat points loaded successfully: " + m_DeathHeatPoints.Count(), 
                    "STS_AnalyticsManager", "LoadDeathHeatPoints");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception loading death heat points: " + e.ToString(),
                "STS_AnalyticsManager", "LoadDeathHeatPoints");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper method to read file content
    protected string ReadFileContent(string filePath)
    {
        string content = "";
        
        FileHandle file = FileIO.OpenFile(filePath, FileMode.READ);
        if (file)
        {
            string line;
            while (FileIO.FGets(file, line) >= 0)
            {
                content += line;
            }
            
            FileIO.CloseFile(file);
        }
        
        return content;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the current player count
    protected int GetPlayerCount()
    {
        int playerCount = 0;
        
        // Get player manager
        PlayerManager playerManager = GetGame().GetPlayerManager();
        if (playerManager)
        {
            playerCount = playerManager.GetPlayerCount();
        }
        
        return playerCount;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the current day of week and hour
    protected void GetCurrentDayAndHour(out int day, out int hour)
    {
        int year, month, dayOfMonth, minute, second;
        GetYearMonthDay(year, month, dayOfMonth);
        GetHourMinuteSecond(hour, minute, second);
        
        // Calculate day of week (0=Sunday, 6=Saturday)
        // This is a simplified algorithm - in production use a more accurate method
        int a = (14 - month) / 12;
        int y = year - a;
        int m = month + 12 * a - 2;
        day = (dayOfMonth + y + y/4 - y/100 + y/400 + (31*m)/12) % 7;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get day name from day number
    protected string GetDayName(int day)
    {
        switch (day)
        {
            case 0: return "Sunday";
            case 1: return "Monday";
            case 2: return "Tuesday";
            case 3: return "Wednesday";
            case 4: return "Thursday";
            case 5: return "Friday";
            case 6: return "Saturday";
            default: return "Unknown";
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get forecasted peak time for next 7 days
    array<ref map<string, string>> GetPeakTimeForecast()
    {
        array<ref map<string, string>> forecast = new array<ref map<string, string>>();
        
        try
        {
            // Get current day of week
            int currentDay, currentHour;
            GetCurrentDayAndHour(currentDay, currentHour);
            
            // Generate forecast for next 7 days
            for (int dayOffset = 0; dayOffset < 7; dayOffset++)
            {
                int forecastDay = (currentDay + dayOffset) % 7;
                
                // Find peak hour for this day
                float maxAvg = 0;
                int peakHour = -1;
                int peakPlayers = 0;
                
                for (int hour = 0; hour < 24; hour++)
                {
                    string key = forecastDay.ToString() + ":" + hour.ToString();
                    STS_AnalyticsTimeBucket bucket = m_PlayerCountBuckets.Get(key);
                    if (bucket && bucket.GetAveragePlayerCount() > maxAvg)
                    {
                        maxAvg = bucket.GetAveragePlayerCount();
                        peakHour = hour;
                        peakPlayers = Math.Round(maxAvg);
                    }
                }
                
                // Create forecast entry
                map<string, string> entry = new map<string, string>();
                entry.Set("day", GetDayName(forecastDay));
                entry.Set("dayNumber", forecastDay.ToString());
                entry.Set("peakHour", peakHour >= 0 ? peakHour.ToString() : "Unknown");
                entry.Set("predictedPlayers", peakPlayers.ToString());
                entry.Set("confidence", GetConfidenceLevel(forecastDay, peakHour));
                
                forecast.Insert(entry);
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception getting peak time forecast: " + e.ToString(),
                "STS_AnalyticsManager", "GetPeakTimeForecast");
        }
        
        return forecast;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get confidence level for prediction
    protected string GetConfidenceLevel(int day, int hour)
    {
        string key = day.ToString() + ":" + hour.ToString();
        STS_AnalyticsTimeBucket bucket = m_PlayerCountBuckets.Get(key);
        
        if (!bucket)
            return "Low";
            
        // Calculate confidence based on sample count
        if (bucket.m_iSampleCount < 5)
            return "Low";
        else if (bucket.m_iSampleCount < 15)
            return "Medium";
        else
            return "High";
    }
    
    //------------------------------------------------------------------------------------------------
    // Get potential camping spots
    array<ref map<string, string>> GetPotentialCampingSpots()
    {
        array<ref map<string, string>> spots = new array<ref map<string, string>>();
        
        try
        {
            foreach (STS_DeathHeatPoint point : m_DeathHeatPoints)
            {
                if (point.IsPotentialCampingSpot())
                {
                    map<string, string> entry = new map<string, string>();
                    entry.Set("position", point.m_vPosition.ToString());
                    entry.Set("deathCount", point.m_iDeathCount.ToString());
                    entry.Set("uniqueVictims", point.m_aPlayerIDs.Count().ToString());
                    
                    string weapons = "";
                    for (int i = 0; i < point.m_aWeapons.Count(); i++)
                    {
                        weapons += point.m_aWeapons[i];
                        if (i < point.m_aWeapons.Count() - 1)
                            weapons += ", ";
                    }
                    entry.Set("weapons", weapons);
                    
                    spots.Insert(entry);
                }
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception getting potential camping spots: " + e.ToString(),
                "STS_AnalyticsManager", "GetPotentialCampingSpots");
        }
        
        return spots;
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset all data
    void ResetData()
    {
        try
        {
            // Reset player count buckets
            foreach (string key, STS_AnalyticsTimeBucket bucket : m_PlayerCountBuckets)
            {
                bucket.Reset();
            }
            
            // Clear death heat points
            m_DeathHeatPoints.Clear();
            
            // Save empty data
            SaveData();
            
            m_Logger.LogInfo("All analytics data has been reset", "STS_AnalyticsManager", "ResetData");
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception resetting analytics data: " + e.ToString(),
                "STS_AnalyticsManager", "ResetData");
        }
    }
} 