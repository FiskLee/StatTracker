// STS_HeatmapGenerator.c
// Generates heatmaps to visualize player activity across the map

class STS_HeatmapGenerator
{
    // Singleton instance
    private static ref STS_HeatmapGenerator s_Instance;
    
    // Config reference
    protected STS_Config m_Config;
    
    // Persistence manager reference
    protected STS_PersistenceManager m_PersistenceManager;
    
    // Data compression reference
    protected STS_DataCompression m_DataCompression;
    
    // Heatmap types
    static const string HEATMAP_TYPE_KILLS = "kills";
    static const string HEATMAP_TYPE_DEATHS = "deaths";
    static const string HEATMAP_TYPE_ACTIVITY = "activity";
    static const string HEATMAP_TYPE_LOOTING = "looting";
    static const string HEATMAP_TYPE_DAMAGE = "damage";
    
    // Map boundaries (these values should be set based on your specific map)
    protected vector m_MinMapBounds = Vector(0, 0, 0);
    protected vector m_MaxMapBounds = Vector(15360, 0, 15360); // Example for a 15360x15360 map
    
    // Heatmap data for different types
    protected ref map<string, ref STS_HeatmapData> m_Heatmaps;
    
    // Last update timestamps
    protected int m_LastFullUpdateTimestamp;
    protected int m_LastSaveTimestamp;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_HeatmapGenerator()
    {
        m_Config = STS_Config.GetInstance();
        m_PersistenceManager = STS_PersistenceManager.GetInstance();
        m_DataCompression = STS_DataCompression.GetInstance();
        
        m_Heatmaps = new map<string, ref STS_HeatmapData>();
        
        // Initialize heatmaps
        InitializeHeatmaps();
        
        m_LastFullUpdateTimestamp = GetTime();
        m_LastSaveTimestamp = GetTime();
        
        Print("[StatTracker] HeatmapGenerator initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_HeatmapGenerator GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_HeatmapGenerator();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize heatmaps
    protected void InitializeHeatmaps()
    {
        // Create heatmap data for different types
        m_Heatmaps.Insert(HEATMAP_TYPE_KILLS, new STS_HeatmapData(HEATMAP_TYPE_KILLS, m_Config.m_iHeatmapResolution));
        m_Heatmaps.Insert(HEATMAP_TYPE_DEATHS, new STS_HeatmapData(HEATMAP_TYPE_DEATHS, m_Config.m_iHeatmapResolution));
        m_Heatmaps.Insert(HEATMAP_TYPE_ACTIVITY, new STS_HeatmapData(HEATMAP_TYPE_ACTIVITY, m_Config.m_iHeatmapResolution));
        m_Heatmaps.Insert(HEATMAP_TYPE_LOOTING, new STS_HeatmapData(HEATMAP_TYPE_LOOTING, m_Config.m_iHeatmapResolution));
        m_Heatmaps.Insert(HEATMAP_TYPE_DAMAGE, new STS_HeatmapData(HEATMAP_TYPE_DAMAGE, m_Config.m_iHeatmapResolution));
        
        // Load existing heatmap data
        LoadHeatmaps();
    }
    
    //------------------------------------------------------------------------------------------------
    // Record a kill event
    void RecordKill(vector position, string weaponName = "")
    {
        if (!m_Config.m_bEnableHeatmaps)
            return;
            
        if (!m_Heatmaps.Contains(HEATMAP_TYPE_KILLS))
            return;
            
        // Map position to heatmap grid
        STS_HeatmapData heatmap = m_Heatmaps.Get(HEATMAP_TYPE_KILLS);
        vector2 gridPos = WorldToGrid(position, heatmap.GetResolution());
        
        // Increment the value at this position
        heatmap.IncrementValue(gridPos);
    }
    
    //------------------------------------------------------------------------------------------------
    // Record a death event
    void RecordDeath(vector position)
    {
        if (!m_Config.m_bEnableHeatmaps)
            return;
            
        if (!m_Heatmaps.Contains(HEATMAP_TYPE_DEATHS))
            return;
            
        // Map position to heatmap grid
        STS_HeatmapData heatmap = m_Heatmaps.Get(HEATMAP_TYPE_DEATHS);
        vector2 gridPos = WorldToGrid(position, heatmap.GetResolution());
        
        // Increment the value at this position
        heatmap.IncrementValue(gridPos);
    }
    
    //------------------------------------------------------------------------------------------------
    // Record player activity (presence)
    void RecordActivity(vector position)
    {
        if (!m_Config.m_bEnableHeatmaps)
            return;
            
        if (!m_Heatmaps.Contains(HEATMAP_TYPE_ACTIVITY))
            return;
            
        // Map position to heatmap grid
        STS_HeatmapData heatmap = m_Heatmaps.Get(HEATMAP_TYPE_ACTIVITY);
        vector2 gridPos = WorldToGrid(position, heatmap.GetResolution());
        
        // Increment the value at this position
        heatmap.IncrementValue(gridPos);
    }
    
    //------------------------------------------------------------------------------------------------
    // Record looting activity
    void RecordLooting(vector position)
    {
        if (!m_Config.m_bEnableHeatmaps)
            return;
            
        if (!m_Heatmaps.Contains(HEATMAP_TYPE_LOOTING))
            return;
            
        // Map position to heatmap grid
        STS_HeatmapData heatmap = m_Heatmaps.Get(HEATMAP_TYPE_LOOTING);
        vector2 gridPos = WorldToGrid(position, heatmap.GetResolution());
        
        // Increment the value at this position
        heatmap.IncrementValue(gridPos);
    }
    
    //------------------------------------------------------------------------------------------------
    // Record damage event
    void RecordDamage(vector position, float amount)
    {
        if (!m_Config.m_bEnableHeatmaps)
            return;
            
        if (!m_Heatmaps.Contains(HEATMAP_TYPE_DAMAGE))
            return;
            
        // Map position to heatmap grid
        STS_HeatmapData heatmap = m_Heatmaps.Get(HEATMAP_TYPE_DAMAGE);
        vector2 gridPos = WorldToGrid(position, heatmap.GetResolution());
        
        // Increment the value at this position, scaled by damage amount
        heatmap.IncrementValue(gridPos, amount);
    }
    
    //------------------------------------------------------------------------------------------------
    // Convert world position to grid position
    protected vector2 WorldToGrid(vector worldPos, int resolution)
    {
        // Calculate relative position in the world bounds (0.0 to 1.0)
        float relX = (worldPos[0] - m_MinMapBounds[0]) / (m_MaxMapBounds[0] - m_MinMapBounds[0]);
        float relZ = (worldPos[2] - m_MinMapBounds[2]) / (m_MaxMapBounds[2] - m_MinMapBounds[2]);
        
        // Clamp to ensure within bounds
        relX = Math.Clamp(relX, 0, 1);
        relZ = Math.Clamp(relZ, 0, 1);
        
        // Convert to grid coordinates
        int gridX = Math.Floor(relX * resolution);
        int gridY = Math.Floor(relZ * resolution);
        
        // Ensure within grid bounds
        gridX = Math.Clamp(gridX, 0, resolution - 1);
        gridY = Math.Clamp(gridY, 0, resolution - 1);
        
        return Vector2(gridX, gridY);
    }
    
    //------------------------------------------------------------------------------------------------
    // Convert grid position to world position
    protected vector GridToWorld(vector2 gridPos, int resolution)
    {
        // Calculate relative position in the grid (0.0 to 1.0)
        float relX = gridPos[0] / resolution;
        float relY = gridPos[1] / resolution;
        
        // Convert to world coordinates
        float worldX = m_MinMapBounds[0] + relX * (m_MaxMapBounds[0] - m_MinMapBounds[0]);
        float worldZ = m_MinMapBounds[2] + relY * (m_MaxMapBounds[2] - m_MinMapBounds[2]);
        
        // Sample terrain for Y coordinate or use 0
        float worldY = 0; // GetTerrainHeight(worldX, worldZ);
        
        return Vector(worldX, worldY, worldZ);
    }
    
    //------------------------------------------------------------------------------------------------
    // Update heatmaps (should be called periodically)
    void Update()
    {
        if (!m_Config.m_bEnableHeatmaps)
            return;
            
        int currentTime = GetTime();
        
        // Save heatmaps periodically
        if (currentTime - m_LastSaveTimestamp >= 300) // Every 5 minutes
        {
            SaveHeatmaps();
            m_LastSaveTimestamp = currentTime;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Save heatmaps to disk
    void SaveHeatmaps()
    {
        if (!m_Config.m_bEnableHeatmaps)
            return;
            
        // Create directory if it doesn't exist
        string heatmapDir = "$profile:StatTracker/Heatmaps/";
        if (!FileExist(heatmapDir))
        {
            MakeDirectory(heatmapDir);
        }
        
        // Save each heatmap
        foreach (string type, STS_HeatmapData heatmap : m_Heatmaps)
        {
            string filePath = heatmapDir + type + ".json";
            SaveHeatmapToFile(heatmap, filePath);
        }
        
        if (m_Config.m_bDebugMode)
            Print("[StatTracker] Heatmaps saved");
    }
    
    //------------------------------------------------------------------------------------------------
    // Save heatmap to file
    protected void SaveHeatmapToFile(STS_HeatmapData heatmap, string filePath)
    {
        if (!heatmap)
            return;
            
        // Serialize heatmap to JSON
        JsonSerializer serializer = new JsonSerializer();
        string jsonStr;
        
        bool serializeSuccess = serializer.WriteToString(heatmap, false, jsonStr);
        
        if (!serializeSuccess)
        {
            Print("[StatTracker] Error serializing heatmap to JSON");
            return;
        }
        
        // Compress data if enabled
        if (m_Config.m_bCompressData)
        {
            jsonStr = m_DataCompression.CompressJsonString(jsonStr);
        }
        
        // Write to file
        FileHandle file = OpenFile(filePath, FileMode.WRITE);
        if (!file)
        {
            Print("[StatTracker] Error opening heatmap file for writing: " + filePath);
            return;
        }
        
        FPrint(file, jsonStr);
        CloseFile(file);
    }
    
    //------------------------------------------------------------------------------------------------
    // Load heatmaps from disk
    void LoadHeatmaps()
    {
        if (!m_Config.m_bEnableHeatmaps)
            return;
            
        // Check if heatmaps directory exists
        string heatmapDir = "$profile:StatTracker/Heatmaps/";
        if (!FileExist(heatmapDir))
        {
            // No heatmaps saved yet
            return;
        }
        
        // Load each heatmap
        foreach (string type, STS_HeatmapData heatmap : m_Heatmaps)
        {
            string filePath = heatmapDir + type + ".json";
            LoadHeatmapFromFile(type, filePath);
        }
        
        if (m_Config.m_bDebugMode)
            Print("[StatTracker] Heatmaps loaded");
    }
    
    //------------------------------------------------------------------------------------------------
    // Load heatmap from file
    protected void LoadHeatmapFromFile(string type, string filePath)
    {
        if (!FileExist(filePath))
            return;
            
        // Read file
        FileHandle file = OpenFile(filePath, FileMode.READ);
        if (!file)
        {
            Print("[StatTracker] Error opening heatmap file for reading: " + filePath);
            return;
        }
        
        string jsonStr;
        string line;
        
        // Read entire file into string
        while (FGets(file, line) >= 0)
        {
            jsonStr += line;
        }
        
        CloseFile(file);
        
        // Decompress data if needed
        if (jsonStr.IndexOf("\"~v~\":") == 1) // Compressed data marker
        {
            jsonStr = m_DataCompression.DecompressJsonString(jsonStr);
        }
        
        // Deserialize from JSON
        STS_HeatmapData loadedHeatmap = new STS_HeatmapData(type, m_Config.m_iHeatmapResolution);
        
        JsonSerializer serializer = new JsonSerializer();
        string errorMsg;
        
        bool parseSuccess = serializer.ReadFromString(loadedHeatmap, jsonStr, errorMsg);
        
        if (!parseSuccess)
        {
            Print("[StatTracker] Error parsing heatmap from JSON: " + errorMsg);
            return;
        }
        
        // Update heatmap in collection
        if (m_Heatmaps.Contains(type))
        {
            m_Heatmaps.Set(type, loadedHeatmap);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate heatmap image in SVG format
    string GenerateHeatmapSVG(string type, int width = 512, int height = 512)
    {
        if (!m_Config.m_bEnableHeatmaps || !m_Heatmaps.Contains(type))
            return "";
            
        STS_HeatmapData heatmap = m_Heatmaps.Get(type);
        if (!heatmap)
            return "";
            
        // Create SVG header
        string svg = string.Format("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%1\" height=\"%2\">", width, height);
        
        // Add background
        svg += string.Format("<rect width=\"%1\" height=\"%2\" fill=\"#222222\"/>", width, height);
        
        // Find max value in heatmap data for normalization
        float maxValue = 0;
        for (int y = 0; y < heatmap.GetResolution(); y++)
        {
            for (int x = 0; x < heatmap.GetResolution(); x++)
            {
                float value = heatmap.GetValue(Vector2(x, y));
                if (value > maxValue)
                    maxValue = value;
            }
        }
        
        // Avoid division by zero
        if (maxValue == 0)
            maxValue = 1;
            
        // Calculate pixel size
        float pixelWidth = width / heatmap.GetResolution();
        float pixelHeight = height / heatmap.GetResolution();
        
        // Draw heatmap cells
        for (int y = 0; y < heatmap.GetResolution(); y++)
        {
            for (int x = 0; x < heatmap.GetResolution(); x++)
            {
                float value = heatmap.GetValue(Vector2(x, y));
                
                if (value <= 0)
                    continue; // Skip empty cells
                    
                // Normalize value (0.0 to 1.0)
                float normalizedValue = value / maxValue;
                
                // Get color based on value
                string color = GetHeatmapColor(normalizedValue);
                
                // Set opacity based on value
                float opacity = m_Config.m_fHeatmapOpacity * Math.Clamp(normalizedValue * 1.5, 0, 1);
                
                // Draw rectangle for this cell
                float pixelX = x * pixelWidth;
                float pixelY = y * pixelHeight;
                
                svg += string.Format("<rect x=\"%1\" y=\"%2\" width=\"%3\" height=\"%4\" fill=\"%5\" opacity=\"%6\"/>",
                    pixelX, pixelY, pixelWidth, pixelHeight, color, opacity);
            }
        }
        
        // Add overlay elements (map landmarks, borders, etc.)
        // This would be customized based on your specific map
        
        // Close SVG
        svg += "</svg>";
        
        return svg;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get color for heatmap based on normalized value (0.0 to 1.0)
    protected string GetHeatmapColor(float value)
    {
        // Use a color gradient from blue (cold) to red (hot)
        if (value < 0.25)
        {
            // Blue to cyan
            int blue = 255;
            int green = Math.Round(255 * (value * 4));
            return string.Format("#00%1%2", 
                green.ToString(16).PadLeft(2, "0"), 
                blue.ToString(16).PadLeft(2, "0"));
        }
        else if (value < 0.5)
        {
            // Cyan to green
            int blue = Math.Round(255 * (1 - (value - 0.25) * 4));
            int green = 255;
            return string.Format("#00%1%2", 
                green.ToString(16).PadLeft(2, "0"), 
                blue.ToString(16).PadLeft(2, "0"));
        }
        else if (value < 0.75)
        {
            // Green to yellow
            int red = Math.Round(255 * ((value - 0.5) * 4));
            int green = 255;
            return string.Format("#%1%200", 
                red.ToString(16).PadLeft(2, "0"), 
                green.ToString(16).PadLeft(2, "0"));
        }
        else
        {
            // Yellow to red
            int red = 255;
            int green = Math.Round(255 * (1 - (value - 0.75) * 4));
            return string.Format("#%1%200", 
                red.ToString(16).PadLeft(2, "0"), 
                green.ToString(16).PadLeft(2, "0"));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Merge heatmap from another instance (e.g., when syncing servers)
    void MergeHeatmap(string type, STS_HeatmapData otherHeatmap)
    {
        if (!m_Config.m_bEnableHeatmaps || !m_Heatmaps.Contains(type) || !otherHeatmap)
            return;
            
        STS_HeatmapData heatmap = m_Heatmaps.Get(type);
        if (!heatmap || heatmap.GetResolution() != otherHeatmap.GetResolution())
            return;
            
        // Merge values
        for (int y = 0; y < heatmap.GetResolution(); y++)
        {
            for (int x = 0; x < heatmap.GetResolution(); x++)
            {
                vector2 pos = Vector2(x, y);
                float existingValue = heatmap.GetValue(pos);
                float otherValue = otherHeatmap.GetValue(pos);
                
                heatmap.SetValue(pos, existingValue + otherValue);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset heatmap data
    void ResetHeatmap(string type)
    {
        if (!m_Config.m_bEnableHeatmaps || !m_Heatmaps.Contains(type))
            return;
            
        // Create new empty heatmap with same resolution
        STS_HeatmapData heatmap = m_Heatmaps.Get(type);
        int resolution = heatmap.GetResolution();
        
        m_Heatmaps.Set(type, new STS_HeatmapData(type, resolution));
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset all heatmaps
    void ResetAllHeatmaps()
    {
        if (!m_Config.m_bEnableHeatmaps)
            return;
            
        // Reset each heatmap
        foreach (string type, STS_HeatmapData heatmap : m_Heatmaps)
        {
            ResetHeatmap(type);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get heatmap data
    STS_HeatmapData GetHeatmap(string type)
    {
        if (!m_Config.m_bEnableHeatmaps || !m_Heatmaps.Contains(type))
            return null;
            
        return m_Heatmaps.Get(type);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current timestamp
    int GetTime()
    {
        return System.GetUnixTime();
    }
}

//------------------------------------------------------------------------------------------------
// Heatmap data class
class STS_HeatmapData
{
    string m_Type;
    int m_Resolution;
    ref array<float> m_Data;
    
    //------------------------------------------------------------------------------------------------
    void STS_HeatmapData(string type, int resolution)
    {
        m_Type = type;
        m_Resolution = resolution;
        
        // Initialize data array (flattened 2D array)
        m_Data = new array<float>();
        m_Data.Resize(resolution * resolution);
        
        // Initialize all values to 0
        for (int i = 0; i < m_Data.Count(); i++)
        {
            m_Data[i] = 0;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    int GetResolution()
    {
        return m_Resolution;
    }
    
    //------------------------------------------------------------------------------------------------
    string GetType()
    {
        return m_Type;
    }
    
    //------------------------------------------------------------------------------------------------
    float GetValue(vector2 position)
    {
        int index = GetIndex(position);
        if (index < 0 || index >= m_Data.Count())
            return 0;
            
        return m_Data[index];
    }
    
    //------------------------------------------------------------------------------------------------
    void SetValue(vector2 position, float value)
    {
        int index = GetIndex(position);
        if (index < 0 || index >= m_Data.Count())
            return;
            
        m_Data[index] = value;
    }
    
    //------------------------------------------------------------------------------------------------
    void IncrementValue(vector2 position, float increment = 1.0)
    {
        int index = GetIndex(position);
        if (index < 0 || index >= m_Data.Count())
            return;
            
        m_Data[index] += increment;
    }
    
    //------------------------------------------------------------------------------------------------
    protected int GetIndex(vector2 position)
    {
        int x = Math.Round(position[0]);
        int y = Math.Round(position[1]);
        
        // Ensure within bounds
        if (x < 0 || x >= m_Resolution || y < 0 || y >= m_Resolution)
            return -1;
            
        return y * m_Resolution + x;
    }
    
    //------------------------------------------------------------------------------------------------
    array<float> GetData()
    {
        return m_Data;
    }
    
    //------------------------------------------------------------------------------------------------
    void SetData(array<float> data)
    {
        if (!data || data.Count() != m_Resolution * m_Resolution)
            return;
            
        m_Data = data;
    }
} 