// STS_TimedStats.c
// Provides time-based tracking of player statistics with daily, weekly, and monthly aggregation

class STS_TimedStats
{
    // Singleton instance
    private static ref STS_TimedStats s_Instance;
    
    // References
    protected STS_Config m_Config;
    
    // Constants for time periods
    static const int TIME_PERIOD_DAILY = 0;
    static const int TIME_PERIOD_WEEKLY = 1;
    static const int TIME_PERIOD_MONTHLY = 2;
    
    // Current snapshots
    protected ref STS_StatSnapshot m_CurrentDaySnapshot;
    protected ref STS_StatSnapshot m_CurrentWeekSnapshot;
    protected ref STS_StatSnapshot m_CurrentMonthSnapshot;
    
    // Historical snapshots
    protected ref array<ref STS_StatSnapshot> m_DailySnapshots;
    protected ref array<ref STS_StatSnapshot> m_WeeklySnapshots;
    protected ref array<ref STS_StatSnapshot> m_MonthlySnapshots;
    
    // Last reset timestamps
    protected int m_LastDailyResetTimestamp;
    protected int m_LastWeeklyResetTimestamp;
    protected int m_LastMonthlyResetTimestamp;
    
    // Owner player ID
    protected string m_OwnerPlayerId;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_TimedStats(string playerId)
    {
        m_Config = STS_Config.GetInstance();
        
        m_OwnerPlayerId = playerId;
        
        // Initialize arrays
        m_DailySnapshots = new array<ref STS_StatSnapshot>();
        m_WeeklySnapshots = new array<ref STS_StatSnapshot>();
        m_MonthlySnapshots = new array<ref STS_StatSnapshot>();
        
        // Initialize current snapshots
        int currentTime = GetTime();
        m_CurrentDaySnapshot = new STS_StatSnapshot(currentTime, TIME_PERIOD_DAILY);
        m_CurrentWeekSnapshot = new STS_StatSnapshot(currentTime, TIME_PERIOD_WEEKLY);
        m_CurrentMonthSnapshot = new STS_StatSnapshot(currentTime, TIME_PERIOD_MONTHLY);
        
        // Set initial reset timestamps
        m_LastDailyResetTimestamp = currentTime;
        m_LastWeeklyResetTimestamp = currentTime;
        m_LastMonthlyResetTimestamp = currentTime;
    }
    
    //------------------------------------------------------------------------------------------------
    // Called when a stat is updated, should be called through player stats class
    void OnStatUpdated(string statName, float value, float delta)
    {
        // Don't do anything if timed stats are disabled
        if (!m_Config.m_bEnableTimedStats)
            return;
        
        // Check if we need to perform resets based on time
        CheckForResets();
        
        // Update current period snapshots
        m_CurrentDaySnapshot.UpdateStat(statName, value, delta);
        m_CurrentWeekSnapshot.UpdateStat(statName, value, delta);
        m_CurrentMonthSnapshot.UpdateStat(statName, value, delta);
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if we need to reset any of the time periods
    void CheckForResets()
    {
        int currentTime = GetTime();
        
        // Get the current date/time information
        int year, month, day, hour, minute, second;
        GetDateTimeFromTimestamp(currentTime, year, month, day, hour, minute, second);
        
        // Check daily reset
        if (m_Config.m_bResetDailyStats)
        {
            // If the current hour is greater than or equal to the reset hour
            // and our last reset was before today at the reset hour
            int resetTimestamp = GetTimestampForTime(year, month, day, m_Config.m_iDailyResetHour, 0, 0);
            
            if (hour >= m_Config.m_iDailyResetHour && m_LastDailyResetTimestamp < resetTimestamp)
            {
                // It's time to reset daily stats
                ResetDailyStats();
            }
        }
        
        // Check weekly reset (if the current day of week matches the reset day)
        if (m_Config.m_bResetWeeklyStats)
        {
            int dayOfWeek = GetDayOfWeek(year, month, day);
            
            if (dayOfWeek == m_Config.m_iWeeklyResetDay)
            {
                // Get timestamp for this week's reset time
                int resetTimestamp = GetTimestampForTime(year, month, day, m_Config.m_iDailyResetHour, 0, 0);
                
                // If it's after the reset hour and we haven't reset yet this week
                if (hour >= m_Config.m_iDailyResetHour && m_LastWeeklyResetTimestamp < resetTimestamp)
                {
                    // It's time to reset weekly stats
                    ResetWeeklyStats();
                }
            }
        }
        
        // Check monthly reset (if the current day of month matches the reset day)
        if (m_Config.m_bResetMonthlyStats)
        {
            if (day == m_Config.m_iMonthlyResetDay)
            {
                // Get timestamp for this month's reset time
                int resetTimestamp = GetTimestampForTime(year, month, day, m_Config.m_iDailyResetHour, 0, 0);
                
                // If it's after the reset hour and we haven't reset yet this month
                if (hour >= m_Config.m_iDailyResetHour && m_LastMonthlyResetTimestamp < resetTimestamp)
                {
                    // It's time to reset monthly stats
                    ResetMonthlyStats();
                }
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset daily stats
    void ResetDailyStats()
    {
        if (m_Config.m_bDebugMode)
            Print("[StatTracker] Resetting daily stats for player " + m_OwnerPlayerId);
        
        int currentTime = GetTime();
        
        // Save current snapshot to history
        if (m_DailySnapshots.Count() >= m_Config.m_iMaxSnapshotsPerPlayer)
        {
            // Remove oldest snapshot if we've reached the limit
            m_DailySnapshots.RemoveOrdered(0);
        }
        
        // Finalize the current snapshot and add to history
        m_CurrentDaySnapshot.SetEndTime(currentTime);
        m_DailySnapshots.Insert(m_CurrentDaySnapshot);
        
        // Create new current snapshot
        m_CurrentDaySnapshot = new STS_StatSnapshot(currentTime, TIME_PERIOD_DAILY);
        
        // Update last reset timestamp
        m_LastDailyResetTimestamp = currentTime;
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset weekly stats
    void ResetWeeklyStats()
    {
        if (m_Config.m_bDebugMode)
            Print("[StatTracker] Resetting weekly stats for player " + m_OwnerPlayerId);
        
        int currentTime = GetTime();
        
        // Save current snapshot to history
        if (m_WeeklySnapshots.Count() >= m_Config.m_iMaxSnapshotsPerPlayer)
        {
            // Remove oldest snapshot if we've reached the limit
            m_WeeklySnapshots.RemoveOrdered(0);
        }
        
        // Finalize the current snapshot and add to history
        m_CurrentWeekSnapshot.SetEndTime(currentTime);
        m_WeeklySnapshots.Insert(m_CurrentWeekSnapshot);
        
        // Create new current snapshot
        m_CurrentWeekSnapshot = new STS_StatSnapshot(currentTime, TIME_PERIOD_WEEKLY);
        
        // Update last reset timestamp
        m_LastWeeklyResetTimestamp = currentTime;
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset monthly stats
    void ResetMonthlyStats()
    {
        if (m_Config.m_bDebugMode)
            Print("[StatTracker] Resetting monthly stats for player " + m_OwnerPlayerId);
        
        int currentTime = GetTime();
        
        // Save current snapshot to history
        if (m_MonthlySnapshots.Count() >= m_Config.m_iMaxSnapshotsPerPlayer)
        {
            // Remove oldest snapshot if we've reached the limit
            m_MonthlySnapshots.RemoveOrdered(0);
        }
        
        // Finalize the current snapshot and add to history
        m_CurrentMonthSnapshot.SetEndTime(currentTime);
        m_MonthlySnapshots.Insert(m_CurrentMonthSnapshot);
        
        // Create new current snapshot
        m_CurrentMonthSnapshot = new STS_StatSnapshot(currentTime, TIME_PERIOD_MONTHLY);
        
        // Update last reset timestamp
        m_LastMonthlyResetTimestamp = currentTime;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current day snapshot
    STS_StatSnapshot GetCurrentDaySnapshot()
    {
        return m_CurrentDaySnapshot;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current week snapshot
    STS_StatSnapshot GetCurrentWeekSnapshot()
    {
        return m_CurrentWeekSnapshot;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current month snapshot
    STS_StatSnapshot GetCurrentMonthSnapshot()
    {
        return m_CurrentMonthSnapshot;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a specific day's stats (0 = today, 1 = yesterday, etc.)
    STS_StatSnapshot GetDayStats(int daysAgo)
    {
        if (daysAgo == 0)
            return m_CurrentDaySnapshot;
            
        int index = m_DailySnapshots.Count() - daysAgo;
        if (index >= 0 && index < m_DailySnapshots.Count())
            return m_DailySnapshots[index];
            
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a specific week's stats (0 = this week, 1 = last week, etc.)
    STS_StatSnapshot GetWeekStats(int weeksAgo)
    {
        if (weeksAgo == 0)
            return m_CurrentWeekSnapshot;
            
        int index = m_WeeklySnapshots.Count() - weeksAgo;
        if (index >= 0 && index < m_WeeklySnapshots.Count())
            return m_WeeklySnapshots[index];
            
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a specific month's stats (0 = this month, 1 = last month, etc.)
    STS_StatSnapshot GetMonthStats(int monthsAgo)
    {
        if (monthsAgo == 0)
            return m_CurrentMonthSnapshot;
            
        int index = m_MonthlySnapshots.Count() - monthsAgo;
        if (index >= 0 && index < m_MonthlySnapshots.Count())
            return m_MonthlySnapshots[index];
            
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all daily snapshots
    array<ref STS_StatSnapshot> GetAllDailySnapshots()
    {
        array<ref STS_StatSnapshot> result = new array<ref STS_StatSnapshot>();
        
        // Add all historical snapshots
        foreach (STS_StatSnapshot snapshot : m_DailySnapshots)
        {
            result.Insert(snapshot);
        }
        
        // Add current snapshot
        result.Insert(m_CurrentDaySnapshot);
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all weekly snapshots
    array<ref STS_StatSnapshot> GetAllWeeklySnapshots()
    {
        array<ref STS_StatSnapshot> result = new array<ref STS_StatSnapshot>();
        
        // Add all historical snapshots
        foreach (STS_StatSnapshot snapshot : m_WeeklySnapshots)
        {
            result.Insert(snapshot);
        }
        
        // Add current snapshot
        result.Insert(m_CurrentWeekSnapshot);
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all monthly snapshots
    array<ref STS_StatSnapshot> GetAllMonthlySnapshots()
    {
        array<ref STS_StatSnapshot> result = new array<ref STS_StatSnapshot>();
        
        // Add all historical snapshots
        foreach (STS_StatSnapshot snapshot : m_MonthlySnapshots)
        {
            result.Insert(snapshot);
        }
        
        // Add current snapshot
        result.Insert(m_CurrentMonthSnapshot);
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a trend for a specific stat over time
    array<float> GetStatTrend(string statName, int periodType, int count = 7)
    {
        array<float> result = new array<float>();
        
        if (periodType == TIME_PERIOD_DAILY)
        {
            // Daily trend
            for (int i = count - 1; i >= 0; i--)
            {
                STS_StatSnapshot snapshot = GetDayStats(i);
                if (snapshot)
                {
                    float value = snapshot.GetStatValue(statName);
                    result.Insert(value);
                }
                else
                {
                    result.Insert(0); // No data for this day
                }
            }
        }
        else if (periodType == TIME_PERIOD_WEEKLY)
        {
            // Weekly trend
            for (int i = count - 1; i >= 0; i--)
            {
                STS_StatSnapshot snapshot = GetWeekStats(i);
                if (snapshot)
                {
                    float value = snapshot.GetStatValue(statName);
                    result.Insert(value);
                }
                else
                {
                    result.Insert(0); // No data for this week
                }
            }
        }
        else if (periodType == TIME_PERIOD_MONTHLY)
        {
            // Monthly trend
            for (int i = count - 1; i >= 0; i--)
            {
                STS_StatSnapshot snapshot = GetMonthStats(i);
                if (snapshot)
                {
                    float value = snapshot.GetStatValue(statName);
                    result.Insert(value);
                }
                else
                {
                    result.Insert(0); // No data for this month
                }
            }
        }
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get current timestamp
    static int GetTime()
    {
        return System.GetUnixTime();
    }
    
    //------------------------------------------------------------------------------------------------
    // Convert timestamp to date and time components
    static void GetDateTimeFromTimestamp(int timestamp, out int year, out int month, out int day, out int hour, out int minute, out int second)
    {
        System.GetYearMonthDayUTC(timestamp, year, month, day);
        System.GetHourMinuteSecondUTC(timestamp, hour, minute, second);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get timestamp for a specific time
    static int GetTimestampForTime(int year, int month, int day, int hour, int minute, int second)
    {
        return System.GetYearMonthDayHourMinuteSecondUTC(year, month, day, hour, minute, second);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get day of week (1 = Monday, 7 = Sunday)
    static int GetDayOfWeek(int year, int month, int day)
    {
        // Zeller's Congruence algorithm (modified for ISO week, where Monday is 1 and Sunday is 7)
        if (month < 3)
        {
            month += 12;
            year--;
        }
        
        int h = (day + (13 * (month + 1)) / 5 + year + year / 4 - year / 100 + year / 400) % 7;
        
        // Convert from 0-6 (Sat-Fri) to 1-7 (Mon-Sun)
        return ((h + 5) % 7) + 1;
    }
}

//------------------------------------------------------------------------------------------------
// StatSnapshot class - stores a snapshot of stats for a time period
class STS_StatSnapshot
{
    // Time period constants (must match those in STS_TimedStats)
    static const int TIME_PERIOD_DAILY = 0;
    static const int TIME_PERIOD_WEEKLY = 1;
    static const int TIME_PERIOD_MONTHLY = 2;
    
    // Time information
    protected int m_StartTimestamp;
    protected int m_EndTimestamp;
    protected int m_TimePeriodType;
    
    // Stats storage
    protected ref map<string, float> m_Stats;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_StatSnapshot(int startTime, int periodType)
    {
        m_StartTimestamp = startTime;
        m_EndTimestamp = 0; // Will be set when the period ends
        m_TimePeriodType = periodType;
        
        m_Stats = new map<string, float>();
    }
    
    //------------------------------------------------------------------------------------------------
    // Set end time
    void SetEndTime(int endTime)
    {
        m_EndTimestamp = endTime;
    }
    
    //------------------------------------------------------------------------------------------------
    // Update a stat
    void UpdateStat(string statName, float value, float delta)
    {
        // We store the current value of the stat directly
        m_Stats.Set(statName, value);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a stat value
    float GetStatValue(string statName)
    {
        if (!m_Stats.Contains(statName))
            return 0;
            
        return m_Stats.Get(statName);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get start timestamp
    int GetStartTimestamp()
    {
        return m_StartTimestamp;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get end timestamp
    int GetEndTimestamp()
    {
        return m_EndTimestamp;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get time period type
    int GetTimePeriodType()
    {
        return m_TimePeriodType;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get formatted date string for display
    string GetFormattedDate()
    {
        int year, month, day, hour, minute, second;
        STS_TimedStats.GetDateTimeFromTimestamp(m_StartTimestamp, year, month, day, hour, minute, second);
        
        if (m_TimePeriodType == TIME_PERIOD_DAILY)
        {
            return string.Format("%1/%2/%3", day, month, year);
        }
        else if (m_TimePeriodType == TIME_PERIOD_WEEKLY)
        {
            return string.Format("Week %1/%2", GetWeekNumber(year, month, day), year);
        }
        else if (m_TimePeriodType == TIME_PERIOD_MONTHLY)
        {
            return string.Format("%1/%2", month, year);
        }
        
        return "";
    }
    
    //------------------------------------------------------------------------------------------------
    // Get week number
    protected int GetWeekNumber(int year, int month, int day)
    {
        // Calculate day of year
        int dayOfYear = day;
        for (int m = 1; m < month; m++)
        {
            dayOfYear += GetDaysInMonth(year, m);
        }
        
        // Calculate ISO week number
        int dayOfWeek = STS_TimedStats.GetDayOfWeek(year, month, day);
        int weekNum = (dayOfYear - dayOfWeek + 10) / 7;
        
        // Adjust for weeks that span years
        if (weekNum < 1)
        {
            // Last week of previous year
            if (month == 1 && day <= 3 && dayOfWeek >= 5)
                return GetWeeksInYear(year - 1);
                
            return 1;
        }
        
        if (weekNum > 52)
        {
            // First week of next year
            if (month == 12 && day >= 29 && dayOfWeek <= 3)
                return 1;
                
            return 52;
        }
        
        return weekNum;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get days in month
    protected int GetDaysInMonth(int year, int month)
    {
        switch (month)
        {
            case 2:
                // February - check for leap year
                if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
                    return 29;
                else
                    return 28;
            case 4:
            case 6:
            case 9:
            case 11:
                // April, June, September, November
                return 30;
            default:
                // All other months
                return 31;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Get number of weeks in year
    protected int GetWeeksInYear(int year)
    {
        // A year has 53 weeks if:
        // - It starts on a Thursday, or
        // - It's a leap year and starts on a Wednesday
        int jan1DayOfWeek = STS_TimedStats.GetDayOfWeek(year, 1, 1);
        
        if (jan1DayOfWeek == 4) // Thursday
            return 53;
            
        if (jan1DayOfWeek == 3 && IsLeapYear(year)) // Wednesday and leap year
            return 53;
            
        return 52;
    }
    
    //------------------------------------------------------------------------------------------------
    // Check if year is leap year
    protected bool IsLeapYear(int year)
    {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get all statistics as map
    map<string, float> GetAllStats()
    {
        return m_Stats;
    }
} 