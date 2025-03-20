// STS_PeakTimeForecast.c
// Analyzes player activity patterns to predict peak server times

class STS_PeakTimeForecast
{
    // Singleton instance
    private static ref STS_PeakTimeForecast s_Instance;
    
    // Configuration
    protected int m_iAnalysisPeriodDays = 14; // Analyze last 14 days
    protected int m_iTimeSlotMinutes = 30; // 30-minute time slots
    protected int m_iForecastHorizonDays = 7; // Predict 7 days ahead
    
    // Daily time slots (48 slots of 30 minutes each for a full day)
    protected const int SLOTS_PER_DAY = 48;
    
    // Day of week constants
    protected const int SUNDAY = 0;
    protected const int MONDAY = 1;
    protected const int TUESDAY = 2;
    protected const int WEDNESDAY = 3;
    protected const int THURSDAY = 4;
    protected const int FRIDAY = 5;
    protected const int SATURDAY = 6;
    
    // Historical data: [day_of_week][time_slot] = player_count
    protected ref array<ref array<ref array<int>>> m_aHistoricalData = new array<ref array<ref array<int>>>(); // [day_of_week][time_slot][day_index]
    protected ref array<ref array<float>> m_aAveragePlayerCount = new array<ref array<float>>(); // [day_of_week][time_slot]
    protected ref array<ref array<float>> m_aForecastedPlayerCount = new array<ref array<float>>(); // [future_day_index][time_slot]
    
    // Metadata about data collection
    protected array<string> m_aCollectionDates = new array<string>();
    protected float m_fLastUpdateTime = 0;
    protected float m_fLastForecastTime = 0;
    protected float m_fLastDataCollectionTime = 0;
    protected const float DATA_COLLECTION_INTERVAL = 900.0; // 15 minutes
    protected const float FORECAST_UPDATE_INTERVAL = 3600.0; // 1 hour
    
    // References
    protected ref STS_LoggingSystem m_Logger;
    protected ref STS_DatabaseManager m_DatabaseManager;
    
    //------------------------------------------------------------------------------------------------
    protected void STS_PeakTimeForecast()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_DatabaseManager = STS_DatabaseManager.GetInstance();
        
        InitializeDataStructures();
        LoadHistoricalData();
        
        // Schedule periodic data collection and forecasting
        GetGame().GetCallqueue().CallLater(CollectCurrentPlayerData, DATA_COLLECTION_INTERVAL * 1000, true);
        GetGame().GetCallqueue().CallLater(UpdateForecast, FORECAST_UPDATE_INTERVAL * 1000, true);
        
        m_Logger.LogInfo("Peak Time Forecast system initialized", "STS_PeakTimeForecast", "Constructor");
    }
    
    //------------------------------------------------------------------------------------------------
    // Initialize the data structures
    protected void InitializeDataStructures()
    {
        // Initialize historical data arrays
        for (int day = 0; day < 7; day++)
        {
            m_aHistoricalData.Insert(new array<ref array<int>>());
            m_aAveragePlayerCount.Insert(new array<float>());
            
            for (int slot = 0; slot < SLOTS_PER_DAY; slot++)
            {
                m_aHistoricalData[day].Insert(new array<int>());
                m_aAveragePlayerCount[day].Insert(0);
            }
        }
        
        // Initialize forecast data
        for (int day = 0; day < m_iForecastHorizonDays; day++)
        {
            m_aForecastedPlayerCount.Insert(new array<float>());
            
            for (int slot = 0; slot < SLOTS_PER_DAY; slot++)
            {
                m_aForecastedPlayerCount[day].Insert(0);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_PeakTimeForecast GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_PeakTimeForecast();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load historical player count data from the database
    protected void LoadHistoricalData()
    {
        if (!m_DatabaseManager)
        {
            m_Logger.LogError("Cannot load historical data - database manager not available", 
                "STS_PeakTimeForecast", "LoadHistoricalData");
            return;
        }
        
        try
        {
            // This would call a repository method to get historical player counts
            // For now, we'll simulate with random data for testing
            
            // In a real implementation, you would retrieve this from a database:
            // m_DatabaseManager.GetPlayerCountRepository().GetHistoricalPlayerCounts(m_iAnalysisPeriodDays);
            
            // For testing purposes, populate with randomized data
            for (int day = 0; day < 7; day++)
            {
                for (int slot = 0; slot < SLOTS_PER_DAY; slot++)
                {
                    for (int dataPoint = 0; dataPoint < m_iAnalysisPeriodDays / 7; dataPoint++)
                    {
                        // Simulate realistic patterns:
                        // - Higher counts in evenings and weekends
                        // - Lower counts in early morning
                        int baseCount = Math.RandomInt(5, 10);
                        
                        // Time of day factor (evenings have more players)
                        int hourOfDay = slot / 2; // Convert slot to hour
                        float timeOfDayFactor = 1.0;
                        
                        if (hourOfDay >= 18 && hourOfDay <= 23) // Evening: 6pm - 11pm
                            timeOfDayFactor = 2.5;
                        else if (hourOfDay >= 12 && hourOfDay < 18) // Afternoon: 12pm - 6pm
                            timeOfDayFactor = 1.5;
                        else if (hourOfDay >= 6 && hourOfDay < 12) // Morning: 6am - 12pm
                            timeOfDayFactor = 1.0;
                        else // Late night/early morning: 12am - 6am
                            timeOfDayFactor = 0.5;
                        
                        // Weekend factor
                        float weekendFactor = (day == FRIDAY || day == SATURDAY || day == SUNDAY) ? 1.5 : 1.0;
                        
                        // Calculate final count with some randomness
                        int count = Math.Round(baseCount * timeOfDayFactor * weekendFactor * Math.RandomFloat(0.8, 1.2));
                        
                        m_aHistoricalData[day][slot].Insert(count);
                    }
                }
            }
            
            m_Logger.LogInfo("Historical player count data loaded", "STS_PeakTimeForecast", "LoadHistoricalData");
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception loading historical data: " + e.ToString(), 
                "STS_PeakTimeForecast", "LoadHistoricalData");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Collect current player count data
    protected void CollectCurrentPlayerData()
    {
        if (!m_DatabaseManager)
            return;
            
        try
        {
            // Get current server time
            int currentDayOfWeek = GetCurrentDayOfWeek();
            int currentTimeSlot = GetCurrentTimeSlot();
            
            // Get current player count
            int playerCount = GetGame().GetPlayerCount();
            
            // Add to historical data
            m_aHistoricalData[currentDayOfWeek][currentTimeSlot].Insert(playerCount);
            
            // Trim old data to keep only the analysis period
            int maxDataPoints = m_iAnalysisPeriodDays / 7;
            while (m_aHistoricalData[currentDayOfWeek][currentTimeSlot].Count() > maxDataPoints)
                m_aHistoricalData[currentDayOfWeek][currentTimeSlot].Remove(0);
            
            // Add timestamp to collection dates
            m_aCollectionDates.Insert(GetCurrentTimestamp());
            
            // Trim old collection dates
            int maxDates = m_iAnalysisPeriodDays * SLOTS_PER_DAY;
            while (m_aCollectionDates.Count() > maxDates)
                m_aCollectionDates.Remove(0);
            
            m_fLastDataCollectionTime = GetGame().GetTickTime();
            
            // Save this data point to the database
            // In a full implementation, you would save to a database:
            // m_DatabaseManager.GetPlayerCountRepository().SavePlayerCount(currentDayOfWeek, currentTimeSlot, playerCount);
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception collecting player data: " + e.ToString(), 
                "STS_PeakTimeForecast", "CollectCurrentPlayerData");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update the player count forecast
    protected void UpdateForecast()
    {
        try
        {
            // First calculate averages
            for (int day = 0; day < 7; day++)
            {
                for (int slot = 0; slot < SLOTS_PER_DAY; slot++)
                {
                    float sum = 0;
                    int count = m_aHistoricalData[day][slot].Count();
                    
                    if (count > 0)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            sum += m_aHistoricalData[day][slot][i];
                        }
                        
                        m_aAveragePlayerCount[day][slot] = sum / count;
                    }
                    else
                    {
                        m_aAveragePlayerCount[day][slot] = 0;
                    }
                }
            }
            
            // Now generate forecast for the upcoming days
            int currentDayOfWeek = GetCurrentDayOfWeek();
            
            for (int forecastDay = 0; forecastDay < m_iForecastHorizonDays; forecastDay++)
            {
                int targetDayOfWeek = (currentDayOfWeek + forecastDay) % 7;
                
                for (int slot = 0; slot < SLOTS_PER_DAY; slot++)
                {
                    // For now, simply use the average for this day/time
                    // A more sophisticated implementation would apply time series analysis, trend detection, etc.
                    m_aForecastedPlayerCount[forecastDay][slot] = m_aAveragePlayerCount[targetDayOfWeek][slot];
                    
                    // Add some randomness based on the standard deviation of historical values
                    float stdDev = CalculateStdDev(m_aHistoricalData[targetDayOfWeek][slot], m_aAveragePlayerCount[targetDayOfWeek][slot]);
                    m_aForecastedPlayerCount[forecastDay][slot] += (Math.RandomFloat() * 2 - 1) * stdDev * 0.1;
                    
                    // Ensure non-negative
                    if (m_aForecastedPlayerCount[forecastDay][slot] < 0)
                        m_aForecastedPlayerCount[forecastDay][slot] = 0;
                }
            }
            
            m_fLastForecastTime = GetGame().GetTickTime();
            m_Logger.LogInfo("Player count forecast updated", "STS_PeakTimeForecast", "UpdateForecast");
        }
        catch (Exception e)
        {
            m_Logger.LogError("Exception updating forecast: " + e.ToString(), 
                "STS_PeakTimeForecast", "UpdateForecast");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Calculate standard deviation of a data set
    protected float CalculateStdDev(array<int> data, float mean)
    {
        if (data.Count() <= 1)
            return 0;
            
        float sumSquareDiff = 0;
        
        foreach (int value : data)
        {
            float diff = value - mean;
            sumSquareDiff += diff * diff;
        }
        
        return Math.Sqrt(sumSquareDiff / (data.Count() - 1));
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the current day of week (0-6, where 0 is Sunday)
    protected int GetCurrentDayOfWeek()
    {
        TimeAndDate timeDate = new TimeAndDate();
        System.GetTimeAndDate(timeDate.m_Year, timeDate.m_Month, timeDate.m_Day, timeDate.m_Hour, timeDate.m_Minute, timeDate.m_Second);
        
        // Calculate the day of week (Zeller's Congruence)
        int m = timeDate.m_Month;
        int y = timeDate.m_Year;
        
        if (m < 3)
        {
            m += 12;
            y -= 1;
        }
        
        int dayOfWeek = (timeDate.m_Day + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
        
        // Adjust to have Sunday as 0
        return (dayOfWeek + 6) % 7;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the current time slot (0-47, each representing 30 minutes)
    protected int GetCurrentTimeSlot()
    {
        TimeAndDate timeDate = new TimeAndDate();
        System.GetTimeAndDate(timeDate.m_Year, timeDate.m_Month, timeDate.m_Day, timeDate.m_Hour, timeDate.m_Minute, timeDate.m_Second);
        
        return timeDate.m_Hour * 2 + (timeDate.m_Minute >= 30 ? 1 : 0);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current timestamp in YYYY-MM-DD HH:MM format
    protected string GetCurrentTimestamp()
    {
        TimeAndDate timeDate = new TimeAndDate();
        System.GetTimeAndDate(timeDate.m_Year, timeDate.m_Month, timeDate.m_Day, timeDate.m_Hour, timeDate.m_Minute, timeDate.m_Second);
        
        return string.Format("%1-%2-%3 %4:%5", 
            timeDate.m_Year.ToString(), 
            timeDate.m_Month.ToString().PadLeft(2, "0"), 
            timeDate.m_Day.ToString().PadLeft(2, "0"), 
            timeDate.m_Hour.ToString().PadLeft(2, "0"), 
            timeDate.m_Minute.ToString().PadLeft(2, "0"));
    }
    
    //------------------------------------------------------------------------------------------------
    // Get peak hours for a specific day of week
    array<int> GetPeakHours(int dayOfWeek)
    {
        array<int> peakHours = new array<int>();
        array<float> hourlyAverages = new array<float>();
        
        // Calculate hourly averages
        for (int hour = 0; hour < 24; hour++)
        {
            float average = (m_aAveragePlayerCount[dayOfWeek][hour * 2] + m_aAveragePlayerCount[dayOfWeek][hour * 2 + 1]) / 2;
            hourlyAverages.Insert(average);
        }
        
        // Calculate overall average
        float overallAverage = 0;
        for (int i = 0; i < hourlyAverages.Count(); i++)
        {
            overallAverage += hourlyAverages[i];
        }
        overallAverage /= hourlyAverages.Count();
        
        // Find hours that are significantly above average
        for (int hour = 0; hour < 24; hour++)
        {
            if (hourlyAverages[hour] > overallAverage * 1.5)
            {
                peakHours.Insert(hour);
            }
        }
        
        return peakHours;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a JSON representation of the forecast for the next week
    string GetForecastAsJSON()
    {
        string json = "{";
        
        // Get day names and current day
        array<string> dayNames = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        int currentDayOfWeek = GetCurrentDayOfWeek();
        
        json += "\"generated_on\":\"" + GetCurrentTimestamp() + "\",";
        json += "\"forecast_days\":[";
        
        for (int forecastDay = 0; forecastDay < m_iForecastHorizonDays; forecastDay++)
        {
            int targetDayOfWeek = (currentDayOfWeek + forecastDay) % 7;
            
            if (forecastDay > 0) json += ",";
            
            json += "{";
            json += "\"day_index\":" + forecastDay.ToString() + ",";
            json += "\"day_name\":\"" + dayNames[targetDayOfWeek] + "\",";
            json += "\"hours\":[";
            
            for (int hour = 0; hour < 24; hour++)
            {
                if (hour > 0) json += ",";
                
                float avgPlayerCount = (m_aForecastedPlayerCount[forecastDay][hour * 2] + 
                                        m_aForecastedPlayerCount[forecastDay][hour * 2 + 1]) / 2;
                
                json += "{";
                json += "\"hour\":" + hour.ToString() + ",";
                json += "\"player_count\":" + avgPlayerCount.ToString() + ",";
                
                // Categorize the hour
                string category = "low";
                if (avgPlayerCount > 20) category = "veryhigh";
                else if (avgPlayerCount > 15) category = "high";
                else if (avgPlayerCount > 10) category = "medium";
                
                json += "\"category\":\"" + category + "\"";
                json += "}";
            }
            
            json += "]}";
        }
        
        json += "]}";
        
        return json;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get peak times for the next 7 days as a formatted string (for admin display)
    string GetPeakTimesFormatted()
    {
        array<string> dayNames = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        int currentDayOfWeek = GetCurrentDayOfWeek();
        string result = "Forecasted Peak Times:\n";
        
        for (int forecastDay = 0; forecastDay < m_iForecastHorizonDays; forecastDay++)
        {
            int targetDayOfWeek = (currentDayOfWeek + forecastDay) % 7;
            
            // Get top 3 hours for this day
            array<ref Pair<int, float>> hourlyRanking = new array<ref Pair<int, float>>();
            
            for (int hour = 0; hour < 24; hour++)
            {
                float avgCount = (m_aForecastedPlayerCount[forecastDay][hour * 2] + 
                                 m_aForecastedPlayerCount[forecastDay][hour * 2 + 1]) / 2;
                                 
                hourlyRanking.Insert(new Pair<int, float>(hour, avgCount));
            }
            
            // Sort by player count (descending)
            hourlyRanking.Sort(SortHoursByPlayerCount);
            
            // Add day heading
            if (forecastDay == 0)
                result += "TODAY (" + dayNames[targetDayOfWeek] + "):\n";
            else if (forecastDay == 1)
                result += "TOMORROW (" + dayNames[targetDayOfWeek] + "):\n";
            else
                result += dayNames[targetDayOfWeek] + ":\n";
            
            // Add top 3 hours
            int peakCount = Math.Min(3, hourlyRanking.Count());
            for (int i = 0; i < peakCount; i++)
            {
                Pair<int, float> peak = hourlyRanking[i];
                
                string hourStr = peak.m_Key.ToString().PadLeft(2, "0") + ":00-" + 
                                peak.m_Key.ToString().PadLeft(2, "0") + ":59";
                
                result += "  " + hourStr + ": ~" + Math.Round(peak.m_Value).ToString() + " players\n";
            }
            
            result += "\n";
        }
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Sort helper for hours by player count
    static bool SortHoursByPlayerCount(Pair<int, float> a, Pair<int, float> b)
    {
        // Descending order
        return a.m_Value > b.m_Value;
    }
}

// Utility class for date/time operations
class TimeAndDate
{
    int m_Year;
    int m_Month;
    int m_Day;
    int m_Hour;
    int m_Minute;
    int m_Second;
} 