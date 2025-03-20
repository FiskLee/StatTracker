// STS_RPC.c
// RPC constants for the StatTracker system

class STS_RPC
{
    // Admin messaging
    static const int ADMIN_MESSAGE = 700;            // Regular admin message
    static const int ADMIN_ANNOUNCEMENT = 701;       // Admin popup announcement
    
    // Player stats
    static const int PLAYER_STATS_UPDATE = 710;      // Update player stats on client
    static const int LEADERBOARD_UPDATE = 711;       // Update leaderboard on client
    
    // Admin dashboard
    static const int MONITOR_UPDATE = 720;           // Send monitoring data to admin
    static const int ADMIN_COMMAND_RESPONSE = 721;   // Response to admin command
    
    // Admin menu
    static const int ADMIN_MENU_DATA = 730;          // Data for admin menu
    static const int ADMIN_MENU_COMMAND = 731;       // Command sent from admin menu
} 