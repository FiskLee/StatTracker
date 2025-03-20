// STS_LocalizationManager.c
// Manages localization and translations for the StatTracker system

class STS_LocalizationManager
{
    // Singleton instance
    private static ref STS_LocalizationManager s_Instance;
    
    // Current language code
    protected string m_sCurrentLanguage = "en";
    
    // Available languages
    protected ref array<string> m_aAvailableLanguages = new array<string>();
    
    // Translation tables for each language
    protected ref map<string, ref map<string, string>> m_mTranslations = new map<string, ref map<string, string>>();
    
    // Config reference
    protected STS_Config m_Config;
    
    // Logger
    protected STS_LoggingSystem m_Logger;
    
    // Path to localization files
    protected const string LOCALIZATION_PATH = "$profile:StatTracker/Localization/";
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_LocalizationManager()
    {
        m_Config = STS_Config.GetInstance();
        m_Logger = STS_LoggingSystem.GetInstance();
        
        // Initialize available languages
        m_aAvailableLanguages.Insert("en"); // English is always available as fallback
        m_aAvailableLanguages.Insert("fr"); // French
        m_aAvailableLanguages.Insert("de"); // German
        m_aAvailableLanguages.Insert("es"); // Spanish
        m_aAvailableLanguages.Insert("ru"); // Russian
        
        // Set initial language from config if available
        if (m_Config && m_Config.m_sLanguage)
            m_sCurrentLanguage = m_Config.m_sLanguage;
            
        // Load translations
        LoadTranslations();
        
        // Register for config changes
        if (m_Config)
            m_Config.RegisterConfigChangeCallback(OnConfigChanged);
            
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Localization Manager initialized with language: %1", m_sCurrentLanguage), "STS_LocalizationManager", "Constructor");
        else
            Print(string.Format("[StatTracker] Localization Manager initialized with language: %1", m_sCurrentLanguage));
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_LocalizationManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_LocalizationManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load translations for all languages
    void LoadTranslations()
    {
        foreach (string language : m_aAvailableLanguages)
        {
            LoadLanguage(language);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load translations for a specific language
    void LoadLanguage(string languageCode)
    {
        // Create directory if it doesn't exist
        string dirPath = LOCALIZATION_PATH;
        if (!FileExist(dirPath))
        {
            MakeDirectory(dirPath);
        }
        
        string filePath = LOCALIZATION_PATH + languageCode + ".json";
        
        // Check if language file exists, if not create a default one
        if (!FileExist(filePath))
        {
            if (languageCode == "en")
            {
                // For English, create default translations
                CreateDefaultTranslations();
                SaveLanguage(languageCode);
            }
            else if (m_Logger)
            {
                m_Logger.LogWarning(string.Format("Language file not found for %1", languageCode), "STS_LocalizationManager", "LoadLanguage");
            }
            return;
        }
        
        // Open the file
        FileHandle file = OpenFile(filePath, FileMode.READ);
        if (!file)
        {
            if (m_Logger)
                m_Logger.LogError(string.Format("Error opening language file: %1", filePath), "STS_LocalizationManager", "LoadLanguage");
            else
                Print(string.Format("[StatTracker] Error opening language file: %1", filePath));
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
        
        // Parse JSON
        if (jsonStr.Length() == 0)
        {
            if (m_Logger)
                m_Logger.LogWarning(string.Format("Empty language file: %1", filePath), "STS_LocalizationManager", "LoadLanguage");
            else
                Print(string.Format("[StatTracker] Empty language file: %1", filePath));
            return;
        }
        
        // Parse translations from JSON
        map<string, string> translations = ParseTranslations(jsonStr);
        
        // Store in translations map
        if (translations.Count() > 0)
        {
            m_mTranslations.Set(languageCode, translations);
            
            if (m_Logger)
                m_Logger.LogInfo(string.Format("Loaded %1 translations for language: %2", translations.Count(), languageCode), "STS_LocalizationManager", "LoadLanguage");
            else
                Print(string.Format("[StatTracker] Loaded %1 translations for language: %2", translations.Count(), languageCode));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Create default (English) translations
    protected void CreateDefaultTranslations()
    {
        map<string, string> defaultTranslations = new map<string, string>();
        
        // General UI
        defaultTranslations.Insert("STS_TEXT_SCOREBOARD_TITLE", "Player Statistics");
        defaultTranslations.Insert("STS_TEXT_CLOSE", "Close");
        defaultTranslations.Insert("STS_TEXT_SAVE", "Save");
        defaultTranslations.Insert("STS_TEXT_CANCEL", "Cancel");
        defaultTranslations.Insert("STS_TEXT_CONFIRM", "Confirm");
        defaultTranslations.Insert("STS_TEXT_OK", "OK");
        defaultTranslations.Insert("STS_TEXT_BACK", "Back");
        defaultTranslations.Insert("STS_TEXT_NEXT", "Next");
        defaultTranslations.Insert("STS_TEXT_PREVIOUS", "Previous");
        
        // Player Stats
        defaultTranslations.Insert("STS_TEXT_PLAYER_NAME", "Player Name");
        defaultTranslations.Insert("STS_TEXT_KILLS", "Kills");
        defaultTranslations.Insert("STS_TEXT_DEATHS", "Deaths");
        defaultTranslations.Insert("STS_TEXT_RANK", "Rank");
        defaultTranslations.Insert("STS_TEXT_SCORE", "Score");
        defaultTranslations.Insert("STS_TEXT_KD_RATIO", "K/D Ratio");
        defaultTranslations.Insert("STS_TEXT_HEADSHOTS", "Headshots");
        defaultTranslations.Insert("STS_TEXT_PLAYTIME", "Playtime");
        defaultTranslations.Insert("STS_TEXT_OBJECTIVES", "Objectives");
        defaultTranslations.Insert("STS_TEXT_LONGEST_KILL", "Longest Kill");
        defaultTranslations.Insert("STS_TEXT_BEST_KILLSTREAK", "Best Killstreak");
        defaultTranslations.Insert("STS_TEXT_LAST_SEEN", "Last Seen");
        defaultTranslations.Insert("STS_TEXT_DISTANCE_TRAVELED", "Distance Traveled");
        
        // Weapon Categories
        defaultTranslations.Insert("STS_TEXT_WEAPON_RIFLE", "Rifle");
        defaultTranslations.Insert("STS_TEXT_WEAPON_PISTOL", "Pistol");
        defaultTranslations.Insert("STS_TEXT_WEAPON_SMG", "SMG");
        defaultTranslations.Insert("STS_TEXT_WEAPON_SHOTGUN", "Shotgun");
        defaultTranslations.Insert("STS_TEXT_WEAPON_SNIPER", "Sniper Rifle");
        defaultTranslations.Insert("STS_TEXT_WEAPON_MACHINEGUN", "Machine Gun");
        defaultTranslations.Insert("STS_TEXT_WEAPON_EXPLOSIVE", "Explosive");
        defaultTranslations.Insert("STS_TEXT_WEAPON_MELEE", "Melee");
        defaultTranslations.Insert("STS_TEXT_WEAPON_VEHICLE", "Vehicle");
        
        // Team Kill System
        defaultTranslations.Insert("STS_TEXT_TEAMKILL_WARNING", "WARNING: Team killing is not allowed! You have received a warning.");
        defaultTranslations.Insert("STS_TEXT_TEAMKILL_KICK", "You have been kicked for excessive team killing.");
        defaultTranslations.Insert("STS_TEXT_TEAMKILL_BAN", "You have been banned for excessive team killing.");
        defaultTranslations.Insert("STS_TEXT_TEAMKILL_NOTIFICATION", "{0} team killed {1} with {2}");
        
        // Admin Commands
        defaultTranslations.Insert("STS_TEXT_ADMIN_STATS_RESET", "Player statistics have been reset.");
        defaultTranslations.Insert("STS_TEXT_ADMIN_PLAYER_NOTFOUND", "Player not found.");
        defaultTranslations.Insert("STS_TEXT_ADMIN_COMMAND_SUCCESS", "Command executed successfully.");
        defaultTranslations.Insert("STS_TEXT_ADMIN_COMMAND_FAILED", "Command failed to execute.");
        
        // Error Messages
        defaultTranslations.Insert("STS_TEXT_ERROR_DATABASE", "Database error occurred: {0}");
        defaultTranslations.Insert("STS_TEXT_ERROR_SAVE_FAILED", "Failed to save player statistics.");
        defaultTranslations.Insert("STS_TEXT_ERROR_LOAD_FAILED", "Failed to load player statistics.");
        defaultTranslations.Insert("STS_TEXT_ERROR_INVALID_PARAMETER", "Invalid parameter: {0}");
        
        // Set as default English translations
        m_mTranslations.Set("en", defaultTranslations);
    }
    
    //------------------------------------------------------------------------------------------------
    // Save translations for a specific language
    void SaveLanguage(string languageCode)
    {
        // Create directory if it doesn't exist
        string dirPath = LOCALIZATION_PATH;
        if (!FileExist(dirPath))
        {
            MakeDirectory(dirPath);
        }
        
        string filePath = LOCALIZATION_PATH + languageCode + ".json";
        
        // Get translations for this language
        map<string, string> translations;
        if (!m_mTranslations.Find(languageCode, translations) || translations.Count() == 0)
        {
            if (m_Logger)
                m_Logger.LogWarning(string.Format("No translations found for language: %1", languageCode), "STS_LocalizationManager", "SaveLanguage");
            else
                Print(string.Format("[StatTracker] No translations found for language: %1", languageCode));
            return;
        }
        
        // Convert translations to JSON
        string jsonStr = SerializeTranslations(translations);
        
        // Open the file
        FileHandle file = OpenFile(filePath, FileMode.WRITE);
        if (!file)
        {
            if (m_Logger)
                m_Logger.LogError(string.Format("Error opening language file for writing: %1", filePath), "STS_LocalizationManager", "SaveLanguage");
            else
                Print(string.Format("[StatTracker] Error opening language file for writing: %1", filePath));
            return;
        }
        
        // Write to file
        FPrint(file, jsonStr);
        CloseFile(file);
        
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Saved %1 translations for language: %2", translations.Count(), languageCode), "STS_LocalizationManager", "SaveLanguage");
        else
            Print(string.Format("[StatTracker] Saved %1 translations for language: %2", translations.Count(), languageCode));
    }
    
    //------------------------------------------------------------------------------------------------
    // Set the current language
    bool SetLanguage(string languageCode)
    {
        // Check if language is available
        if (m_aAvailableLanguages.Find(languageCode) == -1)
        {
            if (m_Logger)
                m_Logger.LogWarning(string.Format("Language not available: %1", languageCode), "STS_LocalizationManager", "SetLanguage");
            else
                Print(string.Format("[StatTracker] Language not available: %1", languageCode));
            return false;
        }
        
        // Update current language
        m_sCurrentLanguage = languageCode;
        
        // Update config
        if (m_Config)
        {
            m_Config.m_sLanguage = languageCode;
            m_Config.SaveConfig();
        }
        
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Set current language to: %1", languageCode), "STS_LocalizationManager", "SetLanguage");
        else
            Print(string.Format("[StatTracker] Set current language to: %1", languageCode));
            
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the current language
    string GetCurrentLanguage()
    {
        return m_sCurrentLanguage;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get available languages
    array<string> GetAvailableLanguages()
    {
        return m_aAvailableLanguages;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a localized string
    string GetLocalizedString(string key, array<string> params = null)
    {
        // Try to get translation in current language
        map<string, string> currentTranslations;
        if (m_mTranslations.Find(m_sCurrentLanguage, currentTranslations))
        {
            string translation;
            if (currentTranslations.Find(key, translation))
            {
                // If we have parameters, replace placeholders
                if (params && params.Count() > 0)
                {
                    return FormatString(translation, params);
                }
                
                return translation;
            }
        }
        
        // Fallback to English if not found
        if (m_sCurrentLanguage != "en")
        {
            map<string, string> englishTranslations;
            if (m_mTranslations.Find("en", englishTranslations))
            {
                string translation;
                if (englishTranslations.Find(key, translation))
                {
                    // If we have parameters, replace placeholders
                    if (params && params.Count() > 0)
                    {
                        return FormatString(translation, params);
                    }
                    
                    return translation;
                }
            }
        }
        
        // Return key as fallback if translation not found
        if (m_Logger)
            m_Logger.LogWarning(string.Format("Translation not found for key: %1", key), "STS_LocalizationManager", "GetLocalizedString");
            
        return key;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a localized string (shorthand version)
    static string Loc(string key, array<string> params = null)
    {
        return GetInstance().GetLocalizedString(key, params);
    }
    
    //------------------------------------------------------------------------------------------------
    // Parse translations from JSON string
    protected map<string, string> ParseTranslations(string jsonStr)
    {
        map<string, string> result = new map<string, string>();
        
        // Try to use JsonSerializer first for better performance
        JsonSerializer js = new JsonSerializer();
        if (js && js.ReadFromString(result, jsonStr, ""))
        {
            return result;
        }
        
        // Fallback to simple parsing if JsonSerializer fails
        // This is a very simplified JSON parser for the format we use
        // Expected format: {"key1":"value1","key2":"value2",...}
        
        if (jsonStr.Length() < 2 || jsonStr[0] != '{' || jsonStr[jsonStr.Length() - 1] != '}')
            return result;
            
        // Remove the outer braces
        string content = jsonStr.Substring(1, jsonStr.Length() - 2);
        
        // Process one key-value pair at a time
        int pos = 0;
        while (pos < content.Length())
        {
            // Find the key (should start with a quote)
            int keyStart = content.IndexOfFrom("\"", pos);
            if (keyStart == -1)
                break;
                
            int keyEnd = content.IndexOfFrom("\"", keyStart + 1);
            if (keyEnd == -1)
                break;
                
            // Extract the key
            string key = content.Substring(keyStart + 1, keyEnd - keyStart - 1);
            
            // Find the value (should start after a colon and quote)
            int colonPos = content.IndexOfFrom(":", keyEnd);
            if (colonPos == -1)
                break;
                
            int valueStart = content.IndexOfFrom("\"", colonPos);
            if (valueStart == -1)
                break;
                
            int valueEnd = -1;
            bool escaped = false;
            
            // Find the end of the value, handling escaped quotes
            for (int i = valueStart + 1; i < content.Length(); i++)
            {
                if (content[i] == '\\' && !escaped)
                {
                    escaped = true;
                    continue;
                }
                
                if (content[i] == '"' && !escaped)
                {
                    valueEnd = i;
                    break;
                }
                
                escaped = false;
            }
            
            if (valueEnd == -1)
                break;
                
            // Extract the value
            string value = content.Substring(valueStart + 1, valueEnd - valueStart - 1);
            
            // Unescape the value
            value = value.Replace("\\\"", "\"");
            value = value.Replace("\\\\", "\\");
            value = value.Replace("\\n", "\n");
            value = value.Replace("\\r", "\r");
            value = value.Replace("\\t", "\t");
            
            // Add to the map
            result.Insert(key, value);
            
            // Move to the next pair
            pos = valueEnd + 1;
        }
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Serialize translations to JSON string
    protected string SerializeTranslations(map<string, string> translations)
    {
        // Try to use JsonSerializer first for better performance
        JsonSerializer js = new JsonSerializer();
        string jsonStr;
        
        if (js && js.WriteToString(translations, true, jsonStr))
        {
            return jsonStr;
        }
        
        // Fallback to simple serialization if JsonSerializer fails
        jsonStr = "{";
        
        array<string> keys = new array<string>();
        translations.GetKeyArray(keys);
        keys.Sort(); // Sort keys for consistent output
        
        for (int i = 0; i < keys.Count(); i++)
        {
            string key = keys[i];
            string value = translations.Get(key);
            
            // Escape special characters
            value = value.Replace("\\", "\\\\");
            value = value.Replace("\"", "\\\"");
            value = value.Replace("\n", "\\n");
            value = value.Replace("\r", "\\r");
            value = value.Replace("\t", "\\t");
            
            jsonStr += "\"" + key + "\":\"" + value + "\"";
            
            if (i < keys.Count() - 1)
                jsonStr += ",";
        }
        
        jsonStr += "}";
        
        return jsonStr;
    }
    
    //------------------------------------------------------------------------------------------------
    // Format a string with parameters (simple implementation)
    protected string FormatString(string format, array<string> params)
    {
        string result = format;
        
        for (int i = 0; i < params.Count(); i++)
        {
            string placeholder = "{" + i.ToString() + "}";
            result = result.Replace(placeholder, params[i]);
        }
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Handle config changes
    protected void OnConfigChanged(map<string, string> changedValues)
    {
        if (changedValues.Contains("Language"))
        {
            string newLanguage = changedValues.Get("Language");
            if (newLanguage != m_sCurrentLanguage)
            {
                SetLanguage(newLanguage);
            }
        }
    }
} 