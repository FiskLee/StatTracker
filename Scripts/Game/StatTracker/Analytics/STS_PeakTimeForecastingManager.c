// STS_PeakTimeForecastingManager.c
// Analysis component for predicting peak player activity times

class STS_PeakTimeForecastingManager
{
    // Singleton instance
    private static ref STS_PeakTimeForecastingManager s_Instance;
    
    // Reference to logging system
    protected STS_LoggingSystem m_Logger;
    
    // Player activity data by hour (0-23) and day of week (0-6)
    protected ref array<ref array<int>> m_PlayerActivityByHourAndDay = new array<ref array<int>>();
    
    // Predicted peak times (hour of day)
    protected ref array<int> m_PeakHoursByDay = new array<int>();
    
    // Predicted peak days (day of week)
    protected ref array<int> m_PeakDays = new array<int>();
    
    // File path for persistence
    protected const string ACTIVITY_DATA_PATH = "$profile:StatTracker/Analytics/peak_activity_data.json";
    
    // Number of weeks to keep in the dataset
    protected const int MAX_WEEKS_DATA = 4;
    
    // Last update timestamp
    protected float m_fLastUpdateTime = 0;
    
    // Update interval (1 hour in seconds)
    protected const float UPDATE_INTERVAL = 3600;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_PeakTimeForecastingManager()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        if (!m_Logger)
        {
            Print("[StatTracker] Failed to get logging system for Peak Time Forecasting", LogLevel.ERROR);
            return;
        }
        
        m_Logger.LogInfo("Initializing Peak Time Forecasting System");
        
        // Initialize hour/day activity matrix
        for (int day = 0; day < 7; day++)
        {
            ref array<int> dayData = new array<int>();
            for (int hour = 0; hour < 24; hour++)
            {
                dayData.Insert(0); // Initialize with 0 players
            }
            m_PlayerActivityByHourAndDay.Insert(dayData);
        }
        
        // Initialize peak hour predictions
        for (int day = 0; day < 7; day++)
        {
            m_PeakHoursByDay.Insert(20); // Default peak hour at 8 PM
        }
        
        // Initialize peak days
        m_PeakDays.Insert(5); // Default Saturday
        m_PeakDays.Insert(6); // Default Sunday
        
        // Create directory if it doesn't exist
        string dir = "$profile:StatTracker/Analytics";
        FileIO.MakeDirectory(dir);
        
        // Load existing data if available
        LoadActivityData();
        
        // Start tracking timer
        GetGame().GetCallqueue().CallLater(UpdateActivityTracking, 60000, true); // Check every minute
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_PeakTimeForecastingManager GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_PeakTimeForecastingManager();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Record current player count
    protected void UpdateActivityTracking()
    {
        float currentTime = System.GetTickCount() / 1000.0;
        
        // Only update once per hour
        if (currentTime - m_fLastUpdateTime < UPDATE_INTERVAL)
            return;
            
        m_fLastUpdateTime = currentTime;
        
        // Get current time
        int hour, minute, second;
        int year, month, day, dayOfWeek;
        System.GetHourMinuteSecond(hour, minute, second);
        System.GetYearMonthDay(year, month, day);
        dayOfWeek = System.GetDayOfWeek() - 1; // Convert to 0-6 (Sunday-Saturday)
        if (dayOfWeek < 0) dayOfWeek = 6; // Handle Sunday
        
        // Get current player count
        array<int> playerIDs = new array<int>();
        GetGame().GetPlayerManager().GetPlayers(playerIDs);
        int playerCount = playerIDs.Count();
        
        // Record activity data
        if (dayOfWeek >= 0 && dayOfWeek < 7 && hour >= 0 && hour < 24)
        {
            // Weighted average - 80% new data, 20% historical
            float oldWeight = 0.8;
            float newWeight = 0.2;
            
            int currentValue = m_PlayerActivityByHourAndDay[dayOfWeek][hour];
            int newValue = Math.Round(currentValue * oldWeight + playerCount * newWeight);
            
            m_PlayerActivityByHourAndDay[dayOfWeek][hour] = newValue;
        }
        
        // Periodically save data and recalculate forecasts
        if (hour == 0 && minute < 10) // Around midnight
        {
            AnalyzeActivityPatterns();
            SaveActivityData();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Analyze player activity data to predict peak times
    protected void AnalyzeActivityPatterns()
    {
        m_Logger.LogInfo("Analyzing player activity patterns for peak time forecasting");
        
        // Find peak hour for each day
        for (int day = 0; day < 7; day++)
        {
            int peakHour = 0;
            int maxPlayers = 0;
            
            for (int hour = 0; hour < 24; hour++)
            {
                if (m_PlayerActivityByHourAndDay[day][hour] > maxPlayers)
                {
                    maxPlayers = m_PlayerActivityByHourAndDay[day][hour];
                    peakHour = hour;
                }
            }
            
            m_PeakHoursByDay[day] = peakHour;
        }
        
        // Find peak days
        array<ref Tuple2<int, int>> dayActivity = new array<ref Tuple2<int, int>>();
        
        for (int day = 0; day < 7; day++)
        {
            int totalActivity = 0;
            for (int hour = 0; hour < 24; hour++)
            {
                totalActivity += m_PlayerActivityByHourAndDay[day][hour];
            }
            
            dayActivity.Insert(new Tuple2<int, int>(day, totalActivity));
        }
        
        // Sort days by activity (descending)
        dayActivity.Sort(ActivityComparer);
        
        // Clear and update peak days (top 2)
        m_PeakDays.Clear();
        for (int i = 0; i < Math.Min(2, dayActivity.Count()); i++)
        {
            m_PeakDays.Insert(dayActivity[i].param1);
        }
        
        m_Logger.LogInfo("Peak time analysis complete");
    }
    
    //------------------------------------------------------------------------------------------------
    // Comparer function for sorting activity data
    static int ActivityComparer(Tuple2<int, int> a, Tuple2<int, int> b)
    {
        if (a.param2 > b.param2) return -1;
        if (a.param2 < b.param2) return 1;
        return 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Save activity data to file
    protected void SaveActivityData()
    {
        // Create JSON representation
        string json = "{\"activity\":[";
        
        for (int day = 0; day < 7; day++)
        {
            json += "[";
            for (int hour = 0; hour < 24; hour++)
            {
                json += m_PlayerActivityByHourAndDay[day][hour].ToString();
                if (hour < 23) json += ",";
            }
            json += "]";
            if (day < 6) json += ",";
        }
        
        json += "],\"peakHours\":[";
        
        for (int day = 0; day < 7; day++)
        {
            json += m_PeakHoursByDay[day].ToString();
            if (day < 6) json += ",";
        }
        
        json += "],\"peakDays\":[";
        
        for (int i = 0; i < m_PeakDays.Count(); i++)
        {
            json += m_PeakDays[i].ToString();
            if (i < m_PeakDays.Count() - 1) json += ",";
        }
        
        json += "]}";
        
        // Save to file
        FileHandle file = FileIO.OpenFile(ACTIVITY_DATA_PATH, FileMode.WRITE);
        if (file != 0)
        {
            FileIO.WriteFile(file, json);
            FileIO.CloseFile(file);
            m_Logger.LogInfo("Peak activity data saved successfully");
        }
        else
        {
            m_Logger.LogError("Failed to save peak activity data");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Load activity data from file
    protected void LoadActivityData()
    {
        if (!FileIO.FileExists(ACTIVITY_DATA_PATH))
        {
            m_Logger.LogInfo("No previous peak activity data found");
            return;
        }
        
        string content;
        FileHandle file = FileIO.OpenFile(ACTIVITY_DATA_PATH, FileMode.READ);
        if (file != 0)
        {
            content = FileIO.ReadFile(file);
            FileIO.CloseFile(file);
            
            try
            {
                // Simple JSON parsing
                JsonSerializer js = new JsonSerializer();
                string error;
                
                // Use a map to process the JSON data
                ref map<string, ref array<ref array<int>>> data = new map<string, ref array<ref array<int>>>();
                
                bool success = js.ReadFromString(data, content, error);
                if (success && data)
                {
                    if (data.Contains("activity"))
                    {
                        m_PlayerActivityByHourAndDay = data.Get("activity");
                    }
                    
                    if (data.Contains("peakHours"))
                    {
                        m_PeakHoursByDay = data.Get("peakHours")[0];
                    }
                    
                    if (data.Contains("peakDays"))
                    {
                        m_PeakDays = data.Get("peakDays")[0];
                    }
                    
                    m_Logger.LogInfo("Successfully loaded peak activity data");
                }
                else
                {
                    m_Logger.LogError("Failed to parse peak activity data: " + error);
                }
            }
            catch (Exception e)
            {
                m_Logger.LogError("Exception loading peak activity data: " + e.ToString());
            }
        }
        else
        {
            m_Logger.LogError("Failed to open peak activity data file");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the predicted peak hours
    array<int> GetPeakHoursByDay()
    {
        return m_PeakHoursByDay;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the predicted peak days
    array<int> GetPeakDays()
    {
        return m_PeakDays;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a string representation of peak times for display
    string GetHumanReadablePeakTimes()
    {
        // Map day numbers to day names
        array<string> dayNames = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        
        string result = "Predicted peak times:\n";
        
        // Peak days first
        result += "Busiest days: ";
        for (int i = 0; i < m_PeakDays.Count(); i++)
        {
            int dayIndex = m_PeakDays[i];
            if (dayIndex >= 0 && dayIndex < 7)
            {
                result += dayNames[dayIndex];
                if (i < m_PeakDays.Count() - 1) result += ", ";
            }
        }
        
        result += "\n\nPeak hours by day:\n";
        
        // Peak hours for each day
        for (int day = 0; day < 7; day++)
        {
            int hour = m_PeakHoursByDay[day];
            string amPm = "AM";
            int displayHour = hour;
            
            if (hour >= 12)
            {
                amPm = "PM";
                if (hour > 12) displayHour = hour - 12;
            }
            if (hour == 0) displayHour = 12;
            
            result += dayNames[day] + ": " + displayHour.ToString() + " " + amPm + "\n";
        }
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current activity heatmap for hours of the day (0-23)
    map<int, int> GetHourlyActivityHeatmap()
    {
        map<int, int> hourlyHeatmap = new map<int, int>();
        
        // Get current day of week
        int dayOfWeek = System.GetDayOfWeek() - 1;
        if (dayOfWeek < 0) dayOfWeek = 6;
        
        // Populate the heatmap with activity data for current day
        if (dayOfWeek >= 0 && dayOfWeek < 7)
        {
            for (int hour = 0; hour < 24; hour++)
            {
                hourlyHeatmap.Insert(hour, m_PlayerActivityByHourAndDay[dayOfWeek][hour]);
            }
        }
        
        return hourlyHeatmap;
    }
} 