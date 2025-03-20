// STS_PerformanceMonitor.c
// Monitors and tracks performance of StatTracker components

class STS_PerformanceMonitor
{
    // Singleton instance
    private static ref STS_PerformanceMonitor s_Instance;
    
    // Configuration
    protected int m_iPerformanceLogInterval = 300;     // How often to log performance stats (in seconds)
    protected bool m_bLogToFile = true;                // Whether to log performance stats to file
    protected bool m_bEnabled = true;                  // Whether performance monitoring is enabled
    
    // Performance metrics
    protected ref map<string, ref STS_PerformanceMetrics> m_mComponentMetrics = new map<string, ref STS_PerformanceMetrics>();
    
    // Runtime statistics
    protected float m_fStartTime = 0;                  // When monitoring started
    protected int m_iTotalOperations = 0;              // Total operations monitored
    protected float m_fTotalTimeSpent = 0;             // Total time spent in monitored operations (ms)
    
    // Logger
    protected STS_LoggingSystem m_Logger;
    
    // Last report time
    protected float m_fLastReportTime = 0;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_PerformanceMonitor()
    {
        m_Logger = STS_LoggingSystem.GetInstance();
        m_fStartTime = GetGame().GetTime();
        
        // Set up scheduled reporting
        GetGame().GetCallqueue().CallLater(LogPerformanceReport, m_iPerformanceLogInterval * 1000, true);
        
        if (m_Logger)
            m_Logger.LogInfo("Performance Monitor initialized", "STS_PerformanceMonitor", "Constructor");
        else
            Print("[StatTracker] Performance Monitor initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_PerformanceMonitor GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_PerformanceMonitor();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Enable or disable performance monitoring
    void SetEnabled(bool enabled)
    {
        m_bEnabled = enabled;
        
        if (m_Logger)
            m_Logger.LogInfo(string.Format("Performance monitoring %1", enabled ? "enabled" : "disabled"), "STS_PerformanceMonitor", "SetEnabled");
        else
            Print(string.Format("[StatTracker] Performance monitoring %1", enabled ? "enabled" : "disabled"));
    }
    
    //------------------------------------------------------------------------------------------------
    // Start measuring an operation
    float StartMeasurement(string componentName, string operationName)
    {
        if (!m_bEnabled)
            return 0;
            
        // Get current time
        float startTime = GetPerformanceTime();
        
        // Get or create metrics for this component
        STS_PerformanceMetrics metrics;
        if (!m_mComponentMetrics.Find(componentName, metrics))
        {
            metrics = new STS_PerformanceMetrics(componentName);
            m_mComponentMetrics.Insert(componentName, metrics);
        }
        
        // Start measurement
        metrics.StartOperation(operationName, startTime);
        
        return startTime;
    }
    
    //------------------------------------------------------------------------------------------------
    // End measuring an operation
    void EndMeasurement(string componentName, string operationName, float startTime)
    {
        if (!m_bEnabled || startTime == 0)
            return;
            
        // Get current time
        float endTime = GetPerformanceTime();
        float elapsed = endTime - startTime;
        
        // Get metrics for this component
        STS_PerformanceMetrics metrics;
        if (!m_mComponentMetrics.Find(componentName, metrics))
        {
            // Should not happen if StartMeasurement was called first
            if (m_Logger)
                m_Logger.LogWarning(string.Format("No metrics found for component %1", componentName), "STS_PerformanceMonitor", "EndMeasurement");
            return;
        }
        
        // End measurement
        metrics.EndOperation(operationName, elapsed);
        
        // Update total statistics
        m_iTotalOperations++;
        m_fTotalTimeSpent += elapsed;
    }
    
    //------------------------------------------------------------------------------------------------
    // Measure a complete operation
    void MeasureOperation(string componentName, string operationName, float elapsed)
    {
        if (!m_bEnabled)
            return;
            
        // Get or create metrics for this component
        STS_PerformanceMetrics metrics;
        if (!m_mComponentMetrics.Find(componentName, metrics))
        {
            metrics = new STS_PerformanceMetrics(componentName);
            m_mComponentMetrics.Insert(componentName, metrics);
        }
        
        // Add measurement
        metrics.RecordOperation(operationName, elapsed);
        
        // Update total statistics
        m_iTotalOperations++;
        m_fTotalTimeSpent += elapsed;
    }
    
    //------------------------------------------------------------------------------------------------
    // Log a performance report
    void LogPerformanceReport()
    {
        if (!m_bEnabled || !GetGame().IsMissionHost())
            return;
            
        // Update last report time
        m_fLastReportTime = GetGame().GetTime();
        
        // Generate report
        string report = GeneratePerformanceReport();
        
        // Log report
        if (m_Logger)
        {
            m_Logger.LogInfo("Performance Report:\n" + report, "STS_PerformanceMonitor", "LogPerformanceReport");
        }
        else
        {
            Print("[StatTracker] Performance Report:");
            Print(report);
        }
        
        // Log to file if enabled
        if (m_bLogToFile)
        {
            LogReportToFile(report);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Reset all metrics
    void ResetMetrics()
    {
        m_mComponentMetrics.Clear();
        m_iTotalOperations = 0;
        m_fTotalTimeSpent = 0;
        m_fStartTime = GetGame().GetTime();
        
        if (m_Logger)
            m_Logger.LogInfo("Performance metrics reset", "STS_PerformanceMonitor", "ResetMetrics");
        else
            Print("[StatTracker] Performance metrics reset");
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a performance report
    string GeneratePerformanceReport()
    {
        string report = "===== StatTracker Performance Report =====\n";
        
        // Add timestamp
        report += string.Format("Time: %1\n", GetFormattedDateTime());
        
        // Add uptime
        float uptime = (GetGame().GetTime() - m_fStartTime) / 1000; // seconds
        report += string.Format("Uptime: %1\n", FormatTimespan(uptime));
        
        // Add total statistics
        report += string.Format("Total Operations: %1\n", m_iTotalOperations);
        report += string.Format("Total Time Spent: %.2f ms\n", m_fTotalTimeSpent);
        report += string.Format("Average Operation Time: %.4f ms\n", m_iTotalOperations > 0 ? m_fTotalTimeSpent / m_iTotalOperations : 0);
        
        // Add component metrics
        report += "\nComponent Performance:\n";
        
        // Sort components by total time spent
        array<string> componentNames = new array<string>();
        m_mComponentMetrics.GetKeyArray(componentNames);
        componentNames.Sort(); // Sort alphabetically initially
        
        // Then sort by time spent (simple bubble sort)
        for (int i = 0; i < componentNames.Count(); i++)
        {
            for (int j = i + 1; j < componentNames.Count(); j++)
            {
                STS_PerformanceMetrics metricsI = m_mComponentMetrics.Get(componentNames[i]);
                STS_PerformanceMetrics metricsJ = m_mComponentMetrics.Get(componentNames[j]);
                
                if (metricsI.GetTotalTimeSpent() < metricsJ.GetTotalTimeSpent())
                {
                    string temp = componentNames[i];
                    componentNames[i] = componentNames[j];
                    componentNames[j] = temp;
                }
            }
        }
        
        // Add each component's metrics to the report
        foreach (string componentName : componentNames)
        {
            STS_PerformanceMetrics metrics = m_mComponentMetrics.Get(componentName);
            report += metrics.GetSummary();
            report += "\n";
        }
        
        // Add server load information if available
        float serverLoad = GetServerLoad();
        if (serverLoad >= 0)
        {
            report += string.Format("\nServer Load: %.1f%%\n", serverLoad * 100);
        }
        
        report += "========================================\n";
        
        return report;
    }
    
    //------------------------------------------------------------------------------------------------
    // Log report to file
    protected void LogReportToFile(string report)
    {
        string dirPath = "$profile:StatTracker/Logs/Performance";
        if (!FileExist(dirPath))
        {
            MakeDirectory(dirPath);
        }
        
        // Create a filename with date
        string dateStr = GetFormattedDate();
        string filePath = dirPath + "/performance_" + dateStr + ".log";
        
        // Open or create file
        FileHandle file = OpenFile(filePath, FileMode.APPEND);
        if (!file)
        {
            if (m_Logger)
                m_Logger.LogError(string.Format("Failed to open performance log file: %1", filePath), "STS_PerformanceMonitor", "LogReportToFile");
            else
                Print(string.Format("[StatTracker] Failed to open performance log file: %1", filePath));
            return;
        }
        
        // Write report
        FPrint(file, report);
        FPrint(file, "\n");
        
        // Close file
        CloseFile(file);
    }
    
    //------------------------------------------------------------------------------------------------
    // Get a high-precision time value (milliseconds)
    protected float GetPerformanceTime()
    {
        return GetGame().GetHighPrecisionTime();
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the server load (CPU usage) if available
    protected float GetServerLoad()
    {
        // In Enfusion, you would use the system API to get server load
        // This is a placeholder for future implementation
        return -1; // Indicates not available
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Format a timespan in seconds to a human-readable string
    protected string FormatTimespan(float seconds)
    {
        int totalSeconds = seconds;
        int days = totalSeconds / 86400;
        int hours = (totalSeconds % 86400) / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int secs = totalSeconds % 60;
        
        if (days > 0)
            return string.Format("%1d %2h %3m %4s", days, hours, minutes, secs);
        else if (hours > 0)
            return string.Format("%1h %2m %3s", hours, minutes, secs);
        else if (minutes > 0)
            return string.Format("%1m %2s", minutes, secs);
        else
            return string.Format("%1s", secs);
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Get formatted date string (YYYY-MM-DD)
    protected string GetFormattedDate()
    {
        TimeAndDate date = System.GetLocalTime();
        return string.Format("%1-%2-%3", date.Year, date.Month.ToFixed(2, "0"), date.Day.ToFixed(2, "0"));
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper: Get formatted date and time string
    protected string GetFormattedDateTime()
    {
        TimeAndDate date = System.GetLocalTime();
        return string.Format("%1-%2-%3 %4:%5:%6", 
            date.Year, 
            date.Month.ToFixed(2, "0"), 
            date.Day.ToFixed(2, "0"), 
            date.Hour.ToFixed(2, "0"), 
            date.Minute.ToFixed(2, "0"), 
            date.Second.ToFixed(2, "0"));
    }
}

//------------------------------------------------------------------------------------------------
// Class to track performance metrics for a single component
class STS_PerformanceMetrics
{
    // Component name
    string m_sComponentName;
    
    // Performance data per operation
    protected ref map<string, ref STS_OperationMetrics> m_mOperationMetrics = new map<string, ref STS_OperationMetrics>();
    
    // Current operations in progress
    protected ref map<string, float> m_mOperationsInProgress = new map<string, float>();
    
    // Total operations
    protected int m_iTotalOperations = 0;
    protected float m_fTotalTimeSpent = 0;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_PerformanceMetrics(string componentName)
    {
        m_sComponentName = componentName;
    }
    
    //------------------------------------------------------------------------------------------------
    // Start timing an operation
    void StartOperation(string operationName, float startTime)
    {
        m_mOperationsInProgress.Set(operationName, startTime);
    }
    
    //------------------------------------------------------------------------------------------------
    // End timing an operation
    void EndOperation(string operationName, float elapsed)
    {
        // Remove from in-progress operations
        m_mOperationsInProgress.Remove(operationName);
        
        // Record the operation
        RecordOperation(operationName, elapsed);
    }
    
    //------------------------------------------------------------------------------------------------
    // Record a complete operation
    void RecordOperation(string operationName, float elapsed)
    {
        // Get or create metrics for this operation
        STS_OperationMetrics metrics;
        if (!m_mOperationMetrics.Find(operationName, metrics))
        {
            metrics = new STS_OperationMetrics(operationName);
            m_mOperationMetrics.Insert(operationName, metrics);
        }
        
        // Add measurement
        metrics.AddMeasurement(elapsed);
        
        // Update total statistics
        m_iTotalOperations++;
        m_fTotalTimeSpent += elapsed;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get total time spent
    float GetTotalTimeSpent()
    {
        return m_fTotalTimeSpent;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get summary of performance metrics
    string GetSummary()
    {
        string summary = string.Format("Component: %1\n", m_sComponentName);
        summary += string.Format("  Total Operations: %1\n", m_iTotalOperations);
        summary += string.Format("  Total Time: %.2f ms\n", m_fTotalTimeSpent);
        summary += string.Format("  Average Time: %.4f ms\n", m_iTotalOperations > 0 ? m_fTotalTimeSpent / m_iTotalOperations : 0);
        
        // Sort operations by total time spent
        array<string> operationNames = new array<string>();
        m_mOperationMetrics.GetKeyArray(operationNames);
        
        // Simple bubble sort
        for (int i = 0; i < operationNames.Count(); i++)
        {
            for (int j = i + 1; j < operationNames.Count(); j++)
            {
                STS_OperationMetrics metricsI = m_mOperationMetrics.Get(operationNames[i]);
                STS_OperationMetrics metricsJ = m_mOperationMetrics.Get(operationNames[j]);
                
                if (metricsI.GetTotalTime() < metricsJ.GetTotalTime())
                {
                    string temp = operationNames[i];
                    operationNames[i] = operationNames[j];
                    operationNames[j] = temp;
                }
            }
        }
        
        // Add each operation's metrics
        summary += "  Operations:\n";
        foreach (string operationName : operationNames)
        {
            STS_OperationMetrics metrics = m_mOperationMetrics.Get(operationName);
            summary += string.Format("    %1: %2 calls, %.2f ms total, %.4f ms avg, %.4f ms min, %.4f ms max\n",
                operationName,
                metrics.GetCount(),
                metrics.GetTotalTime(),
                metrics.GetAverageTime(),
                metrics.GetMinTime(),
                metrics.GetMaxTime());
        }
        
        return summary;
    }
}

//------------------------------------------------------------------------------------------------
// Class to track performance metrics for a single operation
class STS_OperationMetrics
{
    // Operation name
    string m_sOperationName;
    
    // Metrics
    protected int m_iCount = 0;
    protected float m_fTotalTime = 0;
    protected float m_fMinTime = 999999;  // Initialize to a high value
    protected float m_fMaxTime = 0;
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_OperationMetrics(string operationName)
    {
        m_sOperationName = operationName;
    }
    
    //------------------------------------------------------------------------------------------------
    // Add a measurement
    void AddMeasurement(float elapsed)
    {
        m_iCount++;
        m_fTotalTime += elapsed;
        
        if (elapsed < m_fMinTime)
            m_fMinTime = elapsed;
            
        if (elapsed > m_fMaxTime)
            m_fMaxTime = elapsed;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get count of measurements
    int GetCount()
    {
        return m_iCount;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get total time
    float GetTotalTime()
    {
        return m_fTotalTime;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get average time
    float GetAverageTime()
    {
        if (m_iCount == 0)
            return 0;
            
        return m_fTotalTime / m_iCount;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get minimum time
    float GetMinTime()
    {
        if (m_iCount == 0)
            return 0;
            
        return m_fMinTime;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get maximum time
    float GetMaxTime()
    {
        return m_fMaxTime;
    }
} 