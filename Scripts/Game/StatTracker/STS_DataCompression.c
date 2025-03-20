// STS_DataCompression.c
// Provides utilities for compressing and decompressing data to reduce storage space

class STS_DataCompression
{
    // Singleton instance
    private static ref STS_DataCompression s_Instance;
    
    // Config reference
    protected STS_Config m_Config;
    
    // Compression dictionary for common strings
    protected ref map<string, int> m_CompressionDictionary;
    protected ref array<string> m_DecompressionDictionary;
    
    // Dictionary constants
    protected const int DICTIONARY_START = 1000; // Start codes at 1000 to avoid conflicts
    protected const int DICTIONARY_VERSION = 1;  // Version of the dictionary format
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_DataCompression()
    {
        m_Config = STS_Config.GetInstance();
        
        InitializeDictionaries();
        
        Print("[StatTracker] Data Compression initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_DataCompression GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_DataCompression();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize the compression dictionaries
    void InitializeDictionaries()
    {
        m_CompressionDictionary = new map<string, int>();
        m_DecompressionDictionary = new array<string>();
        
        // Add common keys to the dictionary
        AddToDictionary("kills");
        AddToDictionary("deaths");
        AddToDictionary("killstreak");
        AddToDictionary("longestkillstreak");
        AddToDictionary("headshots");
        AddToDictionary("damage_dealt");
        AddToDictionary("damage_taken");
        AddToDictionary("playtime");
        AddToDictionary("distance_traveled");
        AddToDictionary("distance_traveled_vehicle");
        AddToDictionary("distance_traveled_foot");
        AddToDictionary("money_earned");
        AddToDictionary("money_spent");
        AddToDictionary("items_purchased");
        AddToDictionary("items_sold");
        AddToDictionary("achievements");
        AddToDictionary("first_login");
        AddToDictionary("last_login");
        AddToDictionary("sessions");
        AddToDictionary("locations_visited");
        AddToDictionary("weapons_used");
        AddToDictionary("longest_kill_distance");
        AddToDictionary("longest_headshot_distance");
        AddToDictionary("highest_kill_altitude");
        AddToDictionary("player_id");
        AddToDictionary("steam_id");
        AddToDictionary("player_name");
        AddToDictionary("leaderboard_positions");
        AddToDictionary("daily");
        AddToDictionary("weekly");
        AddToDictionary("monthly");
        AddToDictionary("timestamp");
        AddToDictionary("value");
        AddToDictionary("stats");
        AddToDictionary("position");
        AddToDictionary("start_time");
        AddToDictionary("end_time");
        AddToDictionary("period_type");
        AddToDictionary("kills_by_weapon");
        AddToDictionary("kills_by_vehicle");
        AddToDictionary("deaths_by_weapon");
        AddToDictionary("deaths_by_player");
        
        // Add weapon names, vehicles, locations, etc.
        // This would be expanded based on the specific game's requirements
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a string to the dictionary
    protected void AddToDictionary(string value)
    {
        if (!m_CompressionDictionary.Contains(value))
        {
            int code = DICTIONARY_START + m_DecompressionDictionary.Count();
            m_CompressionDictionary.Insert(value, code);
            m_DecompressionDictionary.Insert(value);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Compress a JSON string using dictionary substitution
    string CompressJsonString(string jsonString)
    {
        if (!m_Config.m_bCompressData)
            return jsonString;
            
        if (jsonString.Length() == 0)
            return "";
            
        // Replace dictionary keys with shorter codes
        string compressedJson = jsonString;
        
        // Iterate through dictionary and replace strings
        foreach (string key, int code : m_CompressionDictionary)
        {
            // The format is "key" -> "~code~"
            string pattern = "\"" + key + "\"";
            string replacement = "\"~" + code.ToString() + "~\"";
            
            compressedJson = compressedJson.Replace(pattern, replacement);
        }
        
        // Add dictionary version marker at the start
        compressedJson = "{\"~v~\":" + DICTIONARY_VERSION.ToString() + "," + compressedJson.Substring(1);
        
        return compressedJson;
    }
    
    //------------------------------------------------------------------------------------------------
    // Decompress a JSON string that was compressed using dictionary substitution
    string DecompressJsonString(string compressedJson)
    {
        if (compressedJson.Length() == 0)
            return "";
            
        // Check if this is a compressed string by looking for version marker
        if (compressedJson.IndexOf("\"~v~\":") != 1)
            return compressedJson; // Not compressed or wrong format
            
        // Extract version
        int versionStart = compressedJson.IndexOf("\"~v~\":") + 6;
        int versionEnd = compressedJson.IndexOf(",", versionStart);
        
        if (versionEnd == -1)
            return compressedJson; // Invalid format
            
        string versionStr = compressedJson.Substring(versionStart, versionEnd - versionStart);
        int version = versionStr.ToInt();
        
        if (version != DICTIONARY_VERSION)
        {
            Print(string.Format("[StatTracker] Warning: Compressed data version mismatch. Expected %1, got %2", 
                DICTIONARY_VERSION, version));
        }
        
        // Remove version marker
        compressedJson = "{" + compressedJson.Substring(versionEnd + 1);
        
        // Replace codes with original strings
        string decompressedJson = compressedJson;
        
        // Use regex to find all code patterns "~CODE~"
        for (int code = DICTIONARY_START; code < DICTIONARY_START + m_DecompressionDictionary.Count(); code++)
        {
            int index = code - DICTIONARY_START;
            
            if (index < 0 || index >= m_DecompressionDictionary.Count())
                continue;
                
            string pattern = "\"~" + code.ToString() + "~\"";
            string replacement = "\"" + m_DecompressionDictionary[index] + "\"";
            
            decompressedJson = decompressedJson.Replace(pattern, replacement);
        }
        
        return decompressedJson;
    }
    
    //------------------------------------------------------------------------------------------------
    // Compress an object to JSON string
    string CompressObject(Class obj)
    {
        if (!obj)
            return "";
            
        // Serialize object to JSON
        JsonSerializer serializer = new JsonSerializer();
        string jsonString;
        
        if (!serializer.WriteToString(obj, false, jsonString))
        {
            Print("[StatTracker] Error serializing object to JSON for compression");
            return "";
        }
        
        // Compress the JSON string
        return CompressJsonString(jsonString);
    }
    
    //------------------------------------------------------------------------------------------------
    // Decompress JSON string to object
    bool DecompressToObject(string compressedJson, out Class obj)
    {
        if (compressedJson.Length() == 0)
            return false;
            
        // Decompress JSON string
        string jsonString = DecompressJsonString(compressedJson);
        
        // Deserialize JSON to object
        JsonSerializer serializer = new JsonSerializer();
        string errorMsg;
        
        if (!serializer.ReadFromString(obj, jsonString, errorMsg))
        {
            Print("[StatTracker] Error deserializing JSON to object: " + errorMsg);
            return false;
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Run-length encode a binary array
    array<int> RunLengthEncode(array<int> data)
    {
        if (!data || data.Count() == 0)
            return new array<int>();
            
        array<int> encoded = new array<int>();
        
        int currentValue = data[0];
        int runLength = 1;
        
        for (int i = 1; i < data.Count(); i++)
        {
            if (data[i] == currentValue && runLength < 255)
            {
                // Continue the run
                runLength++;
            }
            else
            {
                // End of run, add to encoded data
                encoded.Insert(runLength);
                encoded.Insert(currentValue);
                
                // Start new run
                currentValue = data[i];
                runLength = 1;
            }
        }
        
        // Add the final run
        encoded.Insert(runLength);
        encoded.Insert(currentValue);
        
        return encoded;
    }
    
    //------------------------------------------------------------------------------------------------
    // Run-length decode a binary array
    array<int> RunLengthDecode(array<int> encoded)
    {
        if (!encoded || encoded.Count() == 0 || encoded.Count() % 2 != 0)
            return new array<int>();
            
        array<int> decoded = new array<int>();
        
        for (int i = 0; i < encoded.Count(); i += 2)
        {
            int runLength = encoded[i];
            int value = encoded[i + 1];
            
            for (int j = 0; j < runLength; j++)
            {
                decoded.Insert(value);
            }
        }
        
        return decoded;
    }
    
    //------------------------------------------------------------------------------------------------
    // Simple string compression for heatmap data or other large datasets
    // Uses run-length encoding and dictionary compression
    string CompressString(string data)
    {
        if (!m_Config.m_bCompressData || data.Length() == 0)
            return data;
            
        // Convert string to array of character codes
        array<int> charCodes = new array<int>();
        for (int i = 0; i < data.Length(); i++)
        {
            charCodes.Insert(data.CharAt(i));
        }
        
        // Run-length encode the character codes
        array<int> encoded = RunLengthEncode(charCodes);
        
        // Convert encoded data back to string
        string compressed = "RLE1:"; // Marker and version
        
        for (int i = 0; i < encoded.Count(); i++)
        {
            // Use a character we're unlikely to see in normal data as separator
            if (i > 0)
                compressed += "|";
                
            compressed += encoded[i].ToString();
        }
        
        return compressed;
    }
    
    //------------------------------------------------------------------------------------------------
    // Decompress a string that was compressed using CompressString
    string DecompressString(string compressed)
    {
        if (compressed.Length() == 0)
            return "";
            
        // Check if this is a compressed string
        if (compressed.IndexOf("RLE1:") != 0)
            return compressed; // Not compressed or wrong format
            
        // Remove marker and version
        compressed = compressed.Substring(5);
        
        // Split into array of integers
        array<string> parts = new array<string>();
        compressed.Split("|", parts);
        
        array<int> encoded = new array<int>();
        foreach (string part : parts)
        {
            encoded.Insert(part.ToInt());
        }
        
        // Run-length decode
        array<int> charCodes = RunLengthDecode(encoded);
        
        // Convert character codes back to string
        string decompressed = "";
        foreach (int code : charCodes)
        {
            decompressed += String.FromAscii(code);
        }
        
        return decompressed;
    }
} 