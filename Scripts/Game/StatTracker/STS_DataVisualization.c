// STS_DataVisualization.c
// Provides visualization capabilities for player statistics using SVG graphics

class STS_DataVisualization
{
    // Singleton instance
    private static ref STS_DataVisualization s_Instance;
    
    // Config reference
    protected STS_Config m_Config;
    
    // Persistence manager reference
    protected STS_PersistenceManager m_PersistenceManager;
    
    // Chart colors
    protected const string COLOR_PRIMARY = "#3498db";
    protected const string COLOR_SECONDARY = "#2ecc71";
    protected const string COLOR_TERTIARY = "#e74c3c";
    protected const string COLOR_QUATERNARY = "#f39c12";
    protected const string COLOR_TEXT = "#333333";
    protected const string COLOR_GRID = "#dddddd";
    protected const string COLOR_BACKGROUND = "#ffffff";
    
    //------------------------------------------------------------------------------------------------
    // Constructor
    void STS_DataVisualization()
    {
        m_Config = STS_Config.GetInstance();
        m_PersistenceManager = STS_PersistenceManager.GetInstance();
        
        Print("[StatTracker] Data Visualization initialized");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get singleton instance
    static STS_DataVisualization GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new STS_DataVisualization();
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a bar chart in SVG format
    string GenerateBarChart(array<float> values, array<string> labels, string title, int width = 500, int height = 300)
    {
        if (!m_Config.m_bEnableVisualization)
            return "";
            
        if (!values || values.Count() == 0)
            return "";
            
        if (!labels)
            labels = new array<string>();
            
        // Ensure labels array is same size as values
        while (labels.Count() < values.Count())
        {
            labels.Insert("Item " + labels.Count().ToString());
        }
        
        // Calculate chart dimensions
        int margin = 40;
        int chartWidth = width - (margin * 2);
        int chartHeight = height - (margin * 2);
        
        // Find maximum value for scaling
        float maxValue = 0;
        for (int i = 0; i < values.Count(); i++)
        {
            if (values[i] > maxValue)
                maxValue = values[i];
        }
        
        // Round max value up to a nice number
        maxValue = Math.Ceil(maxValue * 1.1); // Add 10% padding
        
        // Calculate bar width and spacing
        int barCount = values.Count();
        float barWidth = chartWidth / (barCount * 1.5);
        float barSpacing = barWidth / 2;
        
        // Generate SVG
        string svg = string.Format("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%1\" height=\"%2\">", width, height);
        
        // Add background
        svg += string.Format("<rect width=\"%1\" height=\"%2\" fill=\"%3\"/>", width, height, COLOR_BACKGROUND);
        
        // Add title
        svg += string.Format("<text x=\"%1\" y=\"20\" font-family=\"Arial\" font-size=\"16\" fill=\"%2\" text-anchor=\"middle\">%3</text>", 
            width / 2, COLOR_TEXT, title);
        
        // Draw grid lines
        int gridLineCount = 5;
        for (int i = 0; i <= gridLineCount; i++)
        {
            float y = margin + chartHeight - (i * (chartHeight / gridLineCount));
            float value = (i * maxValue) / gridLineCount;
            
            // Grid line
            svg += string.Format("<line x1=\"%1\" y1=\"%2\" x2=\"%3\" y2=\"%2\" stroke=\"%4\" stroke-width=\"1\"/>", 
                margin, y, width - margin, y, COLOR_GRID);
                
            // Grid label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\">%4</text>", 
                margin - 5, y + 4, COLOR_TEXT, value.ToString("0"));
        }
        
        // Draw bars
        for (int i = 0; i < values.Count(); i++)
        {
            float value = values[i];
            float barHeight = (value / maxValue) * chartHeight;
            float x = margin + (i * (barWidth + barSpacing));
            float y = margin + chartHeight - barHeight;
            
            // Bar
            svg += string.Format("<rect x=\"%1\" y=\"%2\" width=\"%3\" height=\"%4\" fill=\"%5\"/>", 
                x, y, barWidth, barHeight, COLOR_PRIMARY);
                
            // Value label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"white\" text-anchor=\"middle\">%3</text>", 
                x + (barWidth / 2), y + 15, value.ToString("0"));
                
            // X-axis label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\" text-anchor=\"middle\" transform=\"rotate(45 %1,%2)\">%4</text>", 
                x + (barWidth / 2), margin + chartHeight + 10, COLOR_TEXT, labels[i]);
        }
        
        // Close SVG
        svg += "</svg>";
        
        return svg;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a line chart in SVG format
    string GenerateLineChart(array<float> values, array<string> labels, string title, int width = 500, int height = 300)
    {
        if (!m_Config.m_bEnableVisualization)
            return "";
            
        if (!values || values.Count() < 2)
            return "";
            
        if (!labels)
            labels = new array<string>();
            
        // Ensure labels array is same size as values
        while (labels.Count() < values.Count())
        {
            labels.Insert("Point " + labels.Count().ToString());
        }
        
        // Calculate chart dimensions
        int margin = 40;
        int chartWidth = width - (margin * 2);
        int chartHeight = height - (margin * 2);
        
        // Find maximum value for scaling
        float maxValue = 0;
        for (int i = 0; i < values.Count(); i++)
        {
            if (values[i] > maxValue)
                maxValue = values[i];
        }
        
        // Round max value up to a nice number
        maxValue = Math.Ceil(maxValue * 1.1); // Add 10% padding
        
        // Calculate point spacing
        int pointCount = values.Count();
        float pointSpacing = chartWidth / (pointCount - 1);
        
        // Generate SVG
        string svg = string.Format("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%1\" height=\"%2\">", width, height);
        
        // Add background
        svg += string.Format("<rect width=\"%1\" height=\"%2\" fill=\"%3\"/>", width, height, COLOR_BACKGROUND);
        
        // Add title
        svg += string.Format("<text x=\"%1\" y=\"20\" font-family=\"Arial\" font-size=\"16\" fill=\"%2\" text-anchor=\"middle\">%3</text>", 
            width / 2, COLOR_TEXT, title);
        
        // Draw grid lines
        int gridLineCount = 5;
        for (int i = 0; i <= gridLineCount; i++)
        {
            float y = margin + chartHeight - (i * (chartHeight / gridLineCount));
            float value = (i * maxValue) / gridLineCount;
            
            // Grid line
            svg += string.Format("<line x1=\"%1\" y1=\"%2\" x2=\"%3\" y2=\"%2\" stroke=\"%4\" stroke-width=\"1\"/>", 
                margin, y, width - margin, y, COLOR_GRID);
                
            // Grid label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\">%4</text>", 
                margin - 5, y + 4, COLOR_TEXT, value.ToString("0"));
        }
        
        // Draw polyline for line chart
        string points = "";
        for (int i = 0; i < values.Count(); i++)
        {
            float x = margin + (i * pointSpacing);
            float y = margin + chartHeight - ((values[i] / maxValue) * chartHeight);
            
            if (i > 0)
                points += " ";
                
            points += x.ToString() + "," + y.ToString();
        }
        
        // Add polyline
        svg += string.Format("<polyline points=\"%1\" fill=\"none\" stroke=\"%2\" stroke-width=\"2\"/>", 
            points, COLOR_PRIMARY);
            
        // Draw points and labels
        for (int i = 0; i < values.Count(); i++)
        {
            float x = margin + (i * pointSpacing);
            float y = margin + chartHeight - ((values[i] / maxValue) * chartHeight);
            
            // Point
            svg += string.Format("<circle cx=\"%1\" cy=\"%2\" r=\"4\" fill=\"%3\"/>", 
                x, y, COLOR_PRIMARY);
                
            // Value label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\" text-anchor=\"middle\">%4</text>", 
                x, y - 10, COLOR_TEXT, values[i].ToString("0"));
                
            // X-axis label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\" text-anchor=\"middle\" transform=\"rotate(45 %1,%2)\">%4</text>", 
                x, margin + chartHeight + 10, COLOR_TEXT, labels[i]);
        }
        
        // Close SVG
        svg += "</svg>";
        
        return svg;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a pie chart in SVG format
    string GeneratePieChart(array<float> values, array<string> labels, string title, int width = 500, int height = 300)
    {
        if (!m_Config.m_bEnableVisualization)
            return "";
            
        if (!values || values.Count() == 0)
            return "";
            
        if (!labels)
            labels = new array<string>();
            
        // Ensure labels array is same size as values
        while (labels.Count() < values.Count())
        {
            labels.Insert("Slice " + labels.Count().ToString());
        }
        
        // Calculate total for percentages
        float total = 0;
        for (int i = 0; i < values.Count(); i++)
        {
            total += values[i];
        }
        
        // Calculate chart dimensions
        int centerX = width / 2;
        int centerY = height / 2;
        int radius = Math.Min(centerX, centerY) - 50;
        
        // Generate SVG
        string svg = string.Format("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%1\" height=\"%2\">", width, height);
        
        // Add background
        svg += string.Format("<rect width=\"%1\" height=\"%2\" fill=\"%3\"/>", width, height, COLOR_BACKGROUND);
        
        // Add title
        svg += string.Format("<text x=\"%1\" y=\"20\" font-family=\"Arial\" font-size=\"16\" fill=\"%2\" text-anchor=\"middle\">%3</text>", 
            width / 2, COLOR_TEXT, title);
        
        // Color array for slices
        array<string> colors = {COLOR_PRIMARY, COLOR_SECONDARY, COLOR_TERTIARY, COLOR_QUATERNARY, "#9b59b6", "#34495e", "#1abc9c", "#d35400"};
        
        // Draw pie slices
        float startAngle = 0;
        for (int i = 0; i < values.Count(); i++)
        {
            float value = values[i];
            float percentage = (value / total) * 100;
            float sliceAngle = (value / total) * 360;
            float endAngle = startAngle + sliceAngle;
            
            // Calculate slice path
            float startX = centerX + radius * Math.Cos(DegreesToRadians(startAngle - 90));
            float startY = centerY + radius * Math.Sin(DegreesToRadians(startAngle - 90));
            float endX = centerX + radius * Math.Cos(DegreesToRadians(endAngle - 90));
            float endY = centerY + radius * Math.Sin(DegreesToRadians(endAngle - 90));
            
            // Flag for large arc (> 180 degrees)
            int largeArcFlag = (sliceAngle > 180) ? 1 : 0;
            
            // Create slice path
            string path = string.Format("M %1 %2 A %3 %3 0 %4 1 %5 %6 L %7 %8 Z",
                startX, startY, radius, largeArcFlag, endX, endY, centerX, centerY);
                
            // Add pie slice
            string color = colors[i % colors.Count()];
            svg += string.Format("<path d=\"%1\" fill=\"%2\"/>", path, color);
            
            // Calculate label position at middle of slice
            float labelAngle = startAngle + (sliceAngle / 2);
            float labelRadius = radius * 0.7; // Place label at 70% of radius
            float labelX = centerX + labelRadius * Math.Cos(DegreesToRadians(labelAngle - 90));
            float labelY = centerY + labelRadius * Math.Sin(DegreesToRadians(labelAngle - 90));
            
            // Add label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"white\" text-anchor=\"middle\">%3%</text>", 
                labelX, labelY, percentage.ToString("0.0"));
                
            // Update start angle for next slice
            startAngle = endAngle;
        }
        
        // Add legend
        int legendX = width - 100;
        int legendY = 50;
        int legendSpacing = 20;
        
        for (int i = 0; i < values.Count(); i++)
        {
            string color = colors[i % colors.Count()];
            float percentage = (values[i] / total) * 100;
            
            // Legend color box
            svg += string.Format("<rect x=\"%1\" y=\"%2\" width=\"10\" height=\"10\" fill=\"%3\"/>", 
                legendX, legendY + (i * legendSpacing), color);
                
            // Legend text
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\">%4 (%5%)</text>", 
                legendX + 15, legendY + (i * legendSpacing) + 9, COLOR_TEXT, labels[i], percentage.ToString("0.0"));
        }
        
        // Close SVG
        svg += "</svg>";
        
        return svg;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate an area chart in SVG format
    string GenerateAreaChart(array<float> values, array<string> labels, string title, int width = 500, int height = 300)
    {
        if (!m_Config.m_bEnableVisualization)
            return "";
            
        if (!values || values.Count() < 2)
            return "";
            
        if (!labels)
            labels = new array<string>();
            
        // Ensure labels array is same size as values
        while (labels.Count() < values.Count())
        {
            labels.Insert("Point " + labels.Count().ToString());
        }
        
        // Calculate chart dimensions
        int margin = 40;
        int chartWidth = width - (margin * 2);
        int chartHeight = height - (margin * 2);
        
        // Find maximum value for scaling
        float maxValue = 0;
        for (int i = 0; i < values.Count(); i++)
        {
            if (values[i] > maxValue)
                maxValue = values[i];
        }
        
        // Round max value up to a nice number
        maxValue = Math.Ceil(maxValue * 1.1); // Add 10% padding
        
        // Calculate point spacing
        int pointCount = values.Count();
        float pointSpacing = chartWidth / (pointCount - 1);
        
        // Generate SVG
        string svg = string.Format("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%1\" height=\"%2\">", width, height);
        
        // Add background
        svg += string.Format("<rect width=\"%1\" height=\"%2\" fill=\"%3\"/>", width, height, COLOR_BACKGROUND);
        
        // Add title
        svg += string.Format("<text x=\"%1\" y=\"20\" font-family=\"Arial\" font-size=\"16\" fill=\"%2\" text-anchor=\"middle\">%3</text>", 
            width / 2, COLOR_TEXT, title);
        
        // Draw grid lines
        int gridLineCount = 5;
        for (int i = 0; i <= gridLineCount; i++)
        {
            float y = margin + chartHeight - (i * (chartHeight / gridLineCount));
            float value = (i * maxValue) / gridLineCount;
            
            // Grid line
            svg += string.Format("<line x1=\"%1\" y1=\"%2\" x2=\"%3\" y2=\"%2\" stroke=\"%4\" stroke-width=\"1\"/>", 
                margin, y, width - margin, y, COLOR_GRID);
                
            // Grid label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\">%4</text>", 
                margin - 5, y + 4, COLOR_TEXT, value.ToString("0"));
        }
        
        // Create area path
        string areaPath = string.Format("M %1 %2 ", margin, margin + chartHeight);
        
        for (int i = 0; i < values.Count(); i++)
        {
            float x = margin + (i * pointSpacing);
            float y = margin + chartHeight - ((values[i] / maxValue) * chartHeight);
            
            areaPath += string.Format("L %1 %2 ", x, y);
        }
        
        // Close the area path
        areaPath += string.Format("L %1 %2 Z", margin + chartWidth, margin + chartHeight);
        
        // Add area with gradient fill
        svg += string.Format("<defs><linearGradient id=\"areaGradient\" x1=\"0\" x2=\"0\" y1=\"0\" y2=\"1\"><stop offset=\"0%\" stop-color=\"%1\" stop-opacity=\"0.8\"/><stop offset=\"100%\" stop-color=\"%1\" stop-opacity=\"0.1\"/></linearGradient></defs>", COLOR_PRIMARY);
        svg += string.Format("<path d=\"%1\" fill=\"url(#areaGradient)\" stroke=\"none\"/>", areaPath);
        
        // Draw polyline for line chart on top of area
        string points = "";
        for (int i = 0; i < values.Count(); i++)
        {
            float x = margin + (i * pointSpacing);
            float y = margin + chartHeight - ((values[i] / maxValue) * chartHeight);
            
            if (i > 0)
                points += " ";
                
            points += x.ToString() + "," + y.ToString();
        }
        
        // Add polyline
        svg += string.Format("<polyline points=\"%1\" fill=\"none\" stroke=\"%2\" stroke-width=\"2\"/>", 
            points, COLOR_PRIMARY);
            
        // Draw points and labels
        for (int i = 0; i < values.Count(); i++)
        {
            float x = margin + (i * pointSpacing);
            float y = margin + chartHeight - ((values[i] / maxValue) * chartHeight);
            
            // Point
            svg += string.Format("<circle cx=\"%1\" cy=\"%2\" r=\"4\" fill=\"%3\"/>", 
                x, y, COLOR_PRIMARY);
                
            // Value label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\" text-anchor=\"middle\">%4</text>", 
                x, y - 10, COLOR_TEXT, values[i].ToString("0"));
                
            // X-axis label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\" text-anchor=\"middle\" transform=\"rotate(45 %1,%2)\">%4</text>", 
                x, margin + chartHeight + 10, COLOR_TEXT, labels[i]);
        }
        
        // Close SVG
        svg += "</svg>";
        
        return svg;
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate a multi-line chart in SVG format
    string GenerateMultiLineChart(array<array<float>> valuesSeries, array<string> seriesNames, array<string> labels, string title, int width = 500, int height = 300)
    {
        if (!m_Config.m_bEnableVisualization)
            return "";
            
        if (!valuesSeries || valuesSeries.Count() == 0)
            return "";
            
        if (!seriesNames)
            seriesNames = new array<string>();
            
        // Ensure series names array is same size as values series
        while (seriesNames.Count() < valuesSeries.Count())
        {
            seriesNames.Insert("Series " + seriesNames.Count().ToString());
        }
        
        if (!labels)
            labels = new array<string>();
            
        // Find the longest series to determine the number of points
        int maxPoints = 0;
        foreach (array<float> series : valuesSeries)
        {
            if (series.Count() > maxPoints)
                maxPoints = series.Count();
        }
        
        // Ensure labels array is same size as the longest series
        while (labels.Count() < maxPoints)
        {
            labels.Insert("Point " + labels.Count().ToString());
        }
        
        // Calculate chart dimensions
        int margin = 40;
        int chartWidth = width - (margin * 2);
        int chartHeight = height - (margin * 2);
        
        // Find maximum value for scaling across all series
        float maxValue = 0;
        foreach (array<float> series : valuesSeries)
        {
            foreach (float value : series)
            {
                if (value > maxValue)
                    maxValue = value;
            }
        }
        
        // Round max value up to a nice number
        maxValue = Math.Ceil(maxValue * 1.1); // Add 10% padding
        
        // Calculate point spacing
        float pointSpacing = chartWidth / (maxPoints - 1);
        
        // Color array for series
        array<string> colors = {COLOR_PRIMARY, COLOR_SECONDARY, COLOR_TERTIARY, COLOR_QUATERNARY, "#9b59b6", "#34495e", "#1abc9c", "#d35400"};
        
        // Generate SVG
        string svg = string.Format("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%1\" height=\"%2\">", width, height);
        
        // Add background
        svg += string.Format("<rect width=\"%1\" height=\"%2\" fill=\"%3\"/>", width, height, COLOR_BACKGROUND);
        
        // Add title
        svg += string.Format("<text x=\"%1\" y=\"20\" font-family=\"Arial\" font-size=\"16\" fill=\"%2\" text-anchor=\"middle\">%3</text>", 
            width / 2, COLOR_TEXT, title);
        
        // Draw grid lines
        int gridLineCount = 5;
        for (int i = 0; i <= gridLineCount; i++)
        {
            float y = margin + chartHeight - (i * (chartHeight / gridLineCount));
            float value = (i * maxValue) / gridLineCount;
            
            // Grid line
            svg += string.Format("<line x1=\"%1\" y1=\"%2\" x2=\"%3\" y2=\"%2\" stroke=\"%4\" stroke-width=\"1\"/>", 
                margin, y, width - margin, y, COLOR_GRID);
                
            // Grid label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\">%4</text>", 
                margin - 5, y + 4, COLOR_TEXT, value.ToString("0"));
        }
        
        // Draw each series
        for (int s = 0; s < valuesSeries.Count(); s++)
        {
            array<float> series = valuesSeries[s];
            string color = colors[s % colors.Count()];
            
            // Draw polyline for this series
            string points = "";
            for (int i = 0; i < series.Count(); i++)
            {
                float x = margin + (i * pointSpacing);
                float y = margin + chartHeight - ((series[i] / maxValue) * chartHeight);
                
                if (i > 0)
                    points += " ";
                    
                points += x.ToString() + "," + y.ToString();
            }
            
            // Add polyline
            svg += string.Format("<polyline points=\"%1\" fill=\"none\" stroke=\"%2\" stroke-width=\"2\"/>", 
                points, color);
                
            // Draw points
            for (int i = 0; i < series.Count(); i++)
            {
                float x = margin + (i * pointSpacing);
                float y = margin + chartHeight - ((series[i] / maxValue) * chartHeight);
                
                // Point
                svg += string.Format("<circle cx=\"%1\" cy=\"%2\" r=\"4\" fill=\"%3\"/>", 
                    x, y, color);
            }
        }
        
        // Draw X-axis labels
        for (int i = 0; i < maxPoints; i++)
        {
            float x = margin + (i * pointSpacing);
            
            // X-axis label
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\" text-anchor=\"middle\" transform=\"rotate(45 %1,%2)\">%4</text>", 
                x, margin + chartHeight + 10, COLOR_TEXT, labels[i]);
        }
        
        // Add legend
        int legendX = width - 120;
        int legendY = 50;
        int legendSpacing = 20;
        
        for (int i = 0; i < seriesNames.Count(); i++)
        {
            string color = colors[i % colors.Count()];
            
            // Legend line
            svg += string.Format("<line x1=\"%1\" y1=\"%2\" x2=\"%3\" y2=\"%2\" stroke=\"%4\" stroke-width=\"2\"/>", 
                legendX, legendY + (i * legendSpacing) + 5, legendX + 15, legendY + (i * legendSpacing) + 5, color);
                
            // Legend point
            svg += string.Format("<circle cx=\"%1\" cy=\"%2\" r=\"3\" fill=\"%3\"/>", 
                legendX + 7.5, legendY + (i * legendSpacing) + 5, color);
                
            // Legend text
            svg += string.Format("<text x=\"%1\" y=\"%2\" font-family=\"Arial\" font-size=\"10\" fill=\"%3\">%4</text>", 
                legendX + 20, legendY + (i * legendSpacing) + 9, COLOR_TEXT, seriesNames[i]);
        }
        
        // Close SVG
        svg += "</svg>";
        
        return svg;
    }
    
    //------------------------------------------------------------------------------------------------
    // Helper function to convert degrees to radians
    protected float DegreesToRadians(float degrees)
    {
        return degrees * (Math.PI / 180);
    }
    
    //------------------------------------------------------------------------------------------------
    // Generate SVG chart for player stats over time
    string GeneratePlayerStatsOverTime(string playerId, string statName, int periodType, int count = 7)
    {
        if (!m_Config.m_bEnableVisualization || !m_Config.m_bEnableTimedStats)
            return "";
            
        // Load player stats
        STS_EnhancedPlayerStats stats = m_PersistenceManager.LoadPlayerStats(playerId);
        if (!stats || !stats.m_TimedStats)
            return "";
            
        // Get stat trend
        array<float> values = stats.m_TimedStats.GetStatTrend(statName, periodType, count);
        if (!values || values.Count() == 0)
            return "";
            
        // Create labels based on period type
        array<string> labels = new array<string>();
        
        STS_TimedStats timedStats = stats.m_TimedStats;
        if (periodType == STS_TimedStats.TIME_PERIOD_DAILY)
        {
            // Daily labels
            for (int i = count - 1; i >= 0; i--)
            {
                STS_StatSnapshot snapshot = timedStats.GetDayStats(i);
                if (snapshot)
                    labels.Insert(snapshot.GetFormattedDate());
                else
                    labels.Insert("Day " + i.ToString());
            }
        }
        else if (periodType == STS_TimedStats.TIME_PERIOD_WEEKLY)
        {
            // Weekly labels
            for (int i = count - 1; i >= 0; i--)
            {
                STS_StatSnapshot snapshot = timedStats.GetWeekStats(i);
                if (snapshot)
                    labels.Insert(snapshot.GetFormattedDate());
                else
                    labels.Insert("Week " + i.ToString());
            }
        }
        else if (periodType == STS_TimedStats.TIME_PERIOD_MONTHLY)
        {
            // Monthly labels
            for (int i = count - 1; i >= 0; i--)
            {
                STS_StatSnapshot snapshot = timedStats.GetMonthStats(i);
                if (snapshot)
                    labels.Insert(snapshot.GetFormattedDate());
                else
                    labels.Insert("Month " + i.ToString());
            }
        }
        
        // Format title based on stat name
        string title = FormatStatTitle(statName);
        if (periodType == STS_TimedStats.TIME_PERIOD_DAILY)
            title += " (Daily)";
        else if (periodType == STS_TimedStats.TIME_PERIOD_WEEKLY)
            title += " (Weekly)";
        else if (periodType == STS_TimedStats.TIME_PERIOD_MONTHLY)
            title += " (Monthly)";
            
        // Generate the chart (line chart for time series)
        return GenerateLineChart(values, labels, title);
    }
    
    //------------------------------------------------------------------------------------------------
    // Format a stat name for display
    protected string FormatStatTitle(string statName)
    {
        // Replace underscores with spaces
        string formatted = statName.Replace("_", " ");
        
        // Capitalize words
        array<string> words = new array<string>();
        formatted.Split(" ", words);
        
        string result = "";
        for (int i = 0; i < words.Count(); i++)
        {
            if (i > 0)
                result += " ";
                
            if (words[i].Length() > 0)
                result += words[i][0].ToUpper() + words[i].Substring(1);
        }
        
        return result;
    }
} 