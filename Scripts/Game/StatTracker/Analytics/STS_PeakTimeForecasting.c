// STS_PeakTimeForecasting.c
// Component that analyzes player activity patterns and forecasts peak playing times

class STS_PeakTimeForecastingConfig
{
    int m_iMinSamplesRequired = 72; // Minimum hours of data needed before forecasting
    int m_iHourlyBuckets = 168; // 7 days x 24 hours
    float m_fSmoothing = 0.2; // Exponential smoothing factor
    bool m_bEnableSeasonalAdjustment = true; // Adjust for day of week patterns
    bool m_bEnableHolidayDetection = true; // Special handling for holidays
    string m_sTimeZone = "UTC"; // Server timezone for accurate predictions
}

class STS_PlayerCountRecord
{
    int m_iTimestamp;
    int m_iPlayerCount;
}

class STS_PeakTimeForecasting
{
    // Singleton instance
    private static ref STS_PeakTimeForecasting s_Instance;
    
    // Configuration
    protected ref STS_PeakTimeForecastingConfig m_Config;
    
    // References to other components
    protected ref STS_LoggingSystem m_Logger;
    protected ref STS_DatabaseManager m_DatabaseManager;
    
    // Data storage
    protected ref array<int> m_aHourlyPlayerCounts = new array<int>(); // Historical hourly player counts
    protected ref array<int> m_aForecastedPlayerCounts = new array<int>(); // Forecasted player counts
    protected ref map<int, float> m_mDayOfWeekCoefficients = new map<int, float>(); // Day of week adjustment factors
    
    // Timestamp of last data point
    protected int m_iLastUpdateTimestamp = 0;
    
    // Statistics for forecast accuracy
    protected float m_fMeanAbsoluteError = 0;
    protected float m_fMeanPercentageError = 0;
    protected int m_iForecasts = 0;
    
    //------------------------------------------------------------------------------------------------
    protected void STS_PeakTimeForecasting()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_Logger.LogInfo("Initializing Peak Time Forecasting System");
        
        // Initialize configuration
        m_Config = new STS_PeakTimeForecastingConfig();
        
        // Initialize the hourly buckets with zeros
        for (int i = 0; i < m_Config.m_iHourlyBuckets; i++)
        {
            m_aHourlyPlayerCounts.Insert(0);
            m_aForecastedPlayerCounts.Insert(0);
        }
        
        // Set default day of week coefficients
        // These values represent typical weekly patterns where weekends have higher player counts
        m_mDayOfWeekCoefficients.Insert(0, 0.8);  // Monday
        m_mDayOfWeekCoefficients.Insert(1, 0.7);  // Tuesday
        m_mDayOfWeekCoefficients.Insert(2, 0.75); // Wednesday
        m_mDayOfWeekCoefficients.Insert(3, 0.9);  // Thursday
        m_mDayOfWeekCoefficients.Insert(4, 1.2);  // Friday
        m_mDayOfWeekCoefficients.Insert(5, 1.4);  // Saturday
        m_mDayOfWeekCoefficients.Insert(6, 1.15); // Sunday
        
        // Connect to the database
        m_DatabaseManager = STS_DatabaseManager.GetInstance();
        if (m_DatabaseManager)
        {
            LoadHistoricalData();
        }
        else
        {
            m_Logger.LogWarning("Database manager not available - historical data won't be loaded");
        }
        
        // Register update function to run every hour
        GetGame().GetCallqueue().CallLater(UpdatePlayerCount, 3600000, true); // 3600000ms = 1 hour
    }
    
    //------------------------------------------------------------------------------------------------
    static STS_PeakTimeForecasting GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_PeakTimeForecasting();
        }
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Load historical player count data from database
    protected void LoadHistoricalData()
    {
        if (!m_DatabaseManager)
            return;
            
        try 
        {
            // Get player count data from database
            array<ref STS_PlayerCountRecord> records = m_DatabaseManager.GetPlayerCountRepository().GetHistoricalPlayerCounts(m_Config.m_iHourlyBuckets);
            
            if (records && records.Count() > 0)
            {
                // Convert records to hourly counts array
                for (int i = 0; i < records.Count(); i++)
                {
                    STS_PlayerCountRecord record = records[i];
                    int index = i % m_Config.m_iHourlyBuckets;
                    m_aHourlyPlayerCounts[index] = record.m_iPlayerCount;
                }
                
                // Record the timestamp of the most recent record
                m_iLastUpdateTimestamp = records[records.Count() - 1].m_iTimestamp;
                
                m_Logger.LogInfo(string.Format("Loaded %1 historical player count records", records.Count()));
                
                // Update the day of week coefficients based on historical data
                UpdateDayOfWeekCoefficients();
                
                // Generate initial forecast
                GenerateForecast();
            }
            else
            {
                m_Logger.LogWarning("No historical player count data found");
            }
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception loading historical player count data: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Update current player count and add to historical data
    protected void UpdatePlayerCount()
    {
        try 
        {
            // Get current player count
            int currentPlayerCount = GetGame().GetPlayerManager().GetPlayers().Count();
            
            // Get current timestamp
            int currentTimestamp = System.GetUnixTime();
            
            // Calculate hours since last update
            int hoursSinceLastUpdate = 1;
            if (m_iLastUpdateTimestamp > 0)
            {
                hoursSinceLastUpdate = Math.Max(1, (currentTimestamp - m_iLastUpdateTimestamp) / 3600);
            }
            
            // Shift data if more than one hour has passed
            if (hoursSinceLastUpdate > 1)
            {
                // Fill gaps with interpolated values
                int lastKnownCount = m_aHourlyPlayerCounts[0];
                float step = (currentPlayerCount - lastKnownCount) / hoursSinceLastUpdate;
                
                for (int i = hoursSinceLastUpdate - 1; i > 0; i--)
                {
                    int interpolatedCount = lastKnownCount + Math.Round(step * (hoursSinceLastUpdate - i));
                    ShiftAndAddValue(interpolatedCount);
                }
            }
            
            // Add current player count to the data
            ShiftAndAddValue(currentPlayerCount);
            
            // Save to database
            if (m_DatabaseManager)
            {
                STS_PlayerCountRecord record = new STS_PlayerCountRecord();
                record.m_iTimestamp = currentTimestamp;
                record.m_iPlayerCount = currentPlayerCount;
                m_DatabaseManager.GetPlayerCountRepository().SavePlayerCountRecord(record);
            }
            
            // Update timestamp
            m_iLastUpdateTimestamp = currentTimestamp;
            
            // Check prediction accuracy if we have a forecast
            if (m_aForecastedPlayerCounts[0] > 0)
            {
                int forecastedValue = m_aForecastedPlayerCounts[0];
                float absoluteError = Math.AbsFloat(currentPlayerCount - forecastedValue);
                float percentageError = currentPlayerCount > 0 ? (absoluteError / currentPlayerCount) : 0;
                
                // Update error statistics with exponential smoothing
                if (m_iForecasts == 0)
                {
                    m_fMeanAbsoluteError = absoluteError;
                    m_fMeanPercentageError = percentageError;
                }
                else
                {
                    m_fMeanAbsoluteError = (m_fMeanAbsoluteError * 0.95) + (absoluteError * 0.05);
                    m_fMeanPercentageError = (m_fMeanPercentageError * 0.95) + (percentageError * 0.05);
                }
                
                m_iForecasts++;
            }
            
            // Generate new forecast
            if (m_aHourlyPlayerCounts.Count() >= m_Config.m_iMinSamplesRequired)
            {
                GenerateForecast();
            }
            
            m_Logger.LogDebug(string.Format("Updated player count: %1 players online", currentPlayerCount));
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception updating player count: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Shift data array and add new value
    protected void ShiftAndAddValue(int value)
    {
        // Shift all values one position
        for (int i = m_aHourlyPlayerCounts.Count() - 1; i > 0; i--)
        {
            m_aHourlyPlayerCounts[i] = m_aHourlyPlayerCounts[i - 1];
        }
        
        // Add new value at the beginning
        m_aHourlyPlayerCounts[0] = value;
    }
    
    //------------------------------------------------------------------------------------------------
    // Update day of week coefficients based on historical data
    protected void UpdateDayOfWeekCoefficients()
    {
        if (m_aHourlyPlayerCounts.Count() < 24 * 7)
            return;
            
        // Calculate average player count
        float averageCount = 0;
        int totalSamples = 0;
        foreach (int count : m_aHourlyPlayerCounts)
        {
            averageCount += count;
            totalSamples++;
        }
        
        if (totalSamples > 0)
            averageCount /= totalSamples;
            
        // Skip if average is too low
        if (averageCount < 1)
            return;
            
        // Calculate day of week averages
        array<float> dayAverages = new array<float>();
        array<int> dayCounts = new array<int>();
        
        for (int i = 0; i < 7; i++)
        {
            dayAverages.Insert(0);
            dayCounts.Insert(0);
        }
        
        int currentTimestamp = System.GetUnixTime();
        int hourInSeconds = 3600;
        
        // Analyze each data point
        for (int i = 0; i < m_aHourlyPlayerCounts.Count(); i++)
        {
            int timestamp = currentTimestamp - (i * hourInSeconds);
            int dayOfWeek = GetDayOfWeek(timestamp);
            
            dayAverages[dayOfWeek] += m_aHourlyPlayerCounts[i];
            dayCounts[dayOfWeek]++;
        }
        
        // Calculate averages and coefficients
        for (int day = 0; day < 7; day++)
        {
            if (dayCounts[day] > 0)
            {
                dayAverages[day] /= dayCounts[day];
                if (averageCount > 0)
                {
                    // Calculate coefficient as ratio to overall average
                    float coefficient = dayAverages[day] / averageCount;
                    
                    // Smooth with existing value
                    if (m_mDayOfWeekCoefficients.Contains(day))
                    {
                        float existingCoef = m_mDayOfWeekCoefficients.Get(day);
                        coefficient = (existingCoef * 0.7) + (coefficient * 0.3);
                    }
                    
                    // Update coefficient
                    m_mDayOfWeekCoefficients.Set(day, coefficient);
                }
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate forecast for the next week
    protected void GenerateForecast()
    {
        try
        {
            // Skip if not enough data
            if (m_aHourlyPlayerCounts.Count() < m_Config.m_iMinSamplesRequired)
            {
                m_Logger.LogDebug("Not enough historical data for forecasting");
                return;
            }
            
            // Get pattern from the previous weeks
            array<float> basePattern = new array<float>();
            for (int i = 0; i < 168; i++) // 168 hours in a week
            {
                basePattern.Insert(0);
            }
            
            // Calculate average pattern for each hour of the week
            for (int weekHour = 0; weekHour < 168; weekHour++)
            {
                float total = 0;
                int count = 0;
                
                // Look at this hour across multiple previous weeks
                for (int week = 0; week < 4; week++) // Look at 4 weeks of history
                {
                    int index = weekHour + (week * 168);
                    if (index < m_aHourlyPlayerCounts.Count())
                    {
                        total += m_aHourlyPlayerCounts[index];
                        count++;
                    }
                }
                
                if (count > 0)
                {
                    basePattern[weekHour] = total / count;
                }
            }
            
            // Apply trend analysis
            float trend = CalculateTrend();
            
            // Generate forecast
            int currentTimestamp = System.GetUnixTime();
            
            for (int i = 0; i < m_Config.m_iHourlyBuckets; i++)
            {
                int futureTimestamp = currentTimestamp + (i * 3600);
                int hourOfWeek = GetHourOfWeek(futureTimestamp);
                int dayOfWeek = GetDayOfWeek(futureTimestamp);
                
                // Base forecast from pattern
                float forecast = basePattern[hourOfWeek];
                
                // Apply day of week coefficient if enabled
                if (m_Config.m_bEnableSeasonalAdjustment && m_mDayOfWeekCoefficients.Contains(dayOfWeek))
                {
                    forecast *= m_mDayOfWeekCoefficients.Get(dayOfWeek);
                }
                
                // Apply trend
                forecast *= (1 + (trend * i / 168)); // Gradual trend influence
                
                // Apply holiday adjustment if enabled
                if (m_Config.m_bEnableHolidayDetection && IsHoliday(futureTimestamp))
                {
                    forecast *= 1.5; // 50% boost for holidays
                }
                
                // Store forecasted value
                m_aForecastedPlayerCounts[i] = Math.Round(forecast);
            }
            
            m_Logger.LogInfo("Generated new player count forecast");
        }
        catch (Exception e)
        {
            m_Logger.LogError(string.Format("Exception generating forecast: %1", e.ToString()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Calculate the trend (growth or decline) in player counts
    protected float CalculateTrend()
    {
        // Use linear regression to calculate trend
        float sumX = 0;
        float sumY = 0;
        float sumXY = 0;
        float sumXX = 0;
        int n = Math.Min(672, m_aHourlyPlayerCounts.Count()); // Use at most 4 weeks of data
        
        for (int i = 0; i < n; i++)
        {
            float x = i;
            float y = m_aHourlyPlayerCounts[i];
            
            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumXX += x * x;
        }
        
        // Calculate slope
        float slope = 0;
        if (n > 1 && (n * sumXX - sumX * sumX) != 0)
        {
            slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
        }
        
        // Convert to percentage change per week
        float averageY = sumY / n;
        if (averageY > 0)
        {
            return (slope * 168) / averageY; // 168 hours in a week
        }
        
        return 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get hour of week (0-167) from timestamp
    protected int GetHourOfWeek(int timestamp)
    {
        // Convert to local time based on timezone setting
        timestamp += GetTimezoneOffset();
        
        // Calculate day of week (0 = Sunday, 6 = Saturday in Unix time)
        int dayOfWeek = GetDayOfWeek(timestamp);
        
        // Calculate hour of day (0-23)
        int hourOfDay = (timestamp / 3600) % 24;
        
        // Calculate hour of week (0-167)
        return (dayOfWeek * 24) + hourOfDay;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get day of week (0-6, 0 = Sunday) from timestamp
    protected int GetDayOfWeek(int timestamp)
    {
        // Convert to local time based on timezone setting
        timestamp += GetTimezoneOffset();
        
        // Calculate day of week (0 = Sunday, 6 = Saturday in Unix time)
        return ((timestamp / 86400) + 4) % 7; // January 1, 1970 was a Thursday (4)
    }
    
    //------------------------------------------------------------------------------------------------
    // Get timezone offset in seconds based on configuration
    protected int GetTimezoneOffset()
    {
        // Simple implementation with hardcoded offsets for common timezones
        if (m_Config.m_sTimeZone == "UTC") return 0;
        if (m_Config.m_sTimeZone == "GMT") return 0;
        if (m_Config.m_sTimeZone == "EST") return -5 * 3600;
        if (m_Config.m_sTimeZone == "CST") return -6 * 3600;
        if (m_Config.m_sTimeZone == "MST") return -7 * 3600;
        if (m_Config.m_sTimeZone == "PST") return -8 * 3600;
        if (m_Config.m_sTimeZone == "CET") return 1 * 3600;
        if (m_Config.m_sTimeZone == "EET") return 2 * 3600;
        
        // Default to UTC
        return 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if a timestamp represents a holiday
    protected bool IsHoliday(int timestamp)
    {
        // Convert to local time
        timestamp += GetTimezoneOffset();
        
        // Extract date components
        int secondsInDay = 86400;
        int days = timestamp / secondsInDay;
        int year = 1970 + (days / 365); // Approximate year
        
        // Day of year (approximate)
        int dayOfYear = days % 365;
        
        // Check for major gaming holidays
        // Christmas Eve and Christmas (Dec 24-25)
        if (dayOfYear >= 357 && dayOfYear <= 358)
            return true;
            
        // New Year's Eve and New Year's Day (Dec 31-Jan 1)
        if (dayOfYear >= 364 || dayOfYear <= 0)
            return true;
            
        // Spring Break (mid-March, approximate)
        if (dayOfYear >= 74 && dayOfYear <= 81)
            return true;
            
        // Summer Break (June-August, approximate)
        if (dayOfYear >= 152 && dayOfYear <= 243)
            return true;
            
        // Thanksgiving weekend (late November, approximate)
        if (dayOfYear >= 329 && dayOfYear <= 331)
            return true;
        
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the forecast for the next week
    array<int> GetForecast()
    {
        return m_aForecastedPlayerCounts;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the forecast accuracy metrics
    void GetForecastAccuracy(out float meanAbsoluteError, out float meanPercentageError, out int forecastCount)
    {
        meanAbsoluteError = m_fMeanAbsoluteError;
        meanPercentageError = m_fMeanPercentageError;
        forecastCount = m_iForecasts;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the peak time prediction for the next 24 hours
    int GetPredictedPeakHour()
    {
        int peakHour = 0;
        int maxPlayers = 0;
        
        // Check the next 24 hours
        for (int i = 0; i < 24; i++)
        {
            if (m_aForecastedPlayerCounts[i] > maxPlayers)
            {
                maxPlayers = m_aForecastedPlayerCounts[i];
                peakHour = i;
            }
        }
        
        // Convert to hour of day
        int currentHour = (System.GetUnixTime() / 3600) % 24;
        return (currentHour + peakHour) % 24;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get recommended server maintenance time (lowest player count in next 72 hours)
    int GetRecommendedMaintenanceTime()
    {
        int maintenanceHour = 0;
        int minPlayers = 999999;
        
        // Check the next 72 hours
        for (int i = 0; i < 72; i++)
        {
            if (i < m_aForecastedPlayerCounts.Count() && m_aForecastedPlayerCounts[i] < minPlayers)
            {
                minPlayers = m_aForecastedPlayerCounts[i];
                maintenanceHour = i;
            }
        }
        
        // Convert to absolute hour
        int currentHour = (System.GetUnixTime() / 3600);
        return currentHour + maintenanceHour;
    }
}
