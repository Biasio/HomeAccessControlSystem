#ifndef DOORBOTMANAGER_H
#define DOORBOTMANAGER_H

#include <stdint.h>
#include <Preferences.h>
#include <AsyncTelegram2.h>
using namespace std;

#define MAX_USERS 11                // Max. number of user managed by the bot (1 Admin + 10 Users with temporary pins)
#define TELEGRAM_UPDATE_TIME 2000   // Wait 2 seconds before checking for new messages
#define BLOCKED_UPDATE_TIME 2000    // Wait 2 seconds before checking if a blocked user can be unblocked
#define MAX_RETRY 3                 // Max. retry for password entry before blocking the user
#define BLOCK_DURATION 300          // Block duration in seconds (5 minutes)

// User states in the access flow
enum UserState {
    STATE_FIRST_ACCESS,             // First time user accesses the bot
    STATE_WAITING_PW,               // Waiting for admin pin input
    STATE_WAITING_TEMP_PIN,         // Ask Admin to get a temporay pin
    STATE_ADMIN_PANEL,              // In admin panel
    STATE_USER_PANEL,               // In user panel
    STATE_WAITING_PIN_DURATION,     // Waiting for pin duration input
    STATE_REM_USER,                 // Waiting for the admin to select the user to remove
    STATE_WAITING_REVOKE,           // Waiting the confirmation from the MSP that the pin has been revoked after a revoke command has been sent
    STATE_ADMIN_BLOCKED             // Admin is blocked when he enters the wrong pin MAX_RETRY times
};

// User profile structure to store user information and state in the access flow
struct UserProfile {
    int64_t chatId = 0;                     // Telegram chat ID for the user
    char firstName[32];                     // User's first name (max 31 characters + null terminator)
    char lastName[32];                      // User's last name (max 31 characters + null terminator)
    UserState state = STATE_FIRST_ACCESS;   // Current state of the user in the access flow
    int retryCount = 0;                     // Count of failed attempts for password entry (used for blocking after max retries)
    bool isAdmin = false;                   // Flag to indicate if the user has admin privileges
    uint32_t blockedUntil = 0;              // Timestamp until which the user is blocked (used when the user enters the wrong pin too many times)
    bool hasActivePin = false;              // Flag to indicate if the user currently has an active temporary pin
    uint32_t pinExpiration = 0;             // Timestamp when the user's temporary pin expires (used to automatically revoke the pin after its duration has passed)
};

// Enum for different types of events that can be received from the MSP
enum MSPEventType {
    EVENT_ADMIN_PIN_OK,             // Event received when the admin pin entered is correct
    EVENT_ADMIN_PIN_WRONG,          // Event received when the admin pin entered is wrong
    EVENT_TEMP_PIN,                 // Event received when a temporary user pin has been generated and received from the MSP    
    EVENT_REVOKE_OK,                // Event received when the MSP confirms that a pin has been revoked successfully after a revoke command has been sent
    EVENT_REQ_TIME,                 // Event received when the MSP requests the current time for displaying it on the LCD
    EVENT_UNKNOWN                   // Default event type when the received event does not match any known event (used for error handling)
};  

// Structure to hold the response from the MSP
struct MSPResponse {
    MSPEventType event; // The type of event received from the MSP
    int64_t chatId;     // The chat ID of the user related to the event
    String payload;     // Additional data related to the event, e.g., the generated temporary pin 
};  

// Class to manage the Telegram bot interactions, user profiles, and communication with the MSP for door control
class DoorBotManager {
private:
    AsyncTelegram2& bot;                    // Reference to the AsyncTelegram2 instance for handling Telegram interactions
    HardwareSerial& mspSerial;              // Reference to the MSP Serial Port for door control
    Preferences flashMemory;                // Preferences instance for storing user profiles in flash memory
    int64_t currentAdminId = 0;             // Variable to track the current admin's chat ID
    String pendingAdminCmd = "";            // Variable to track the pending admin command that requires a response from the MSP
    uint32_t pinDuration = 240;             // Minutes of validity for the temporary user pin, default is 4 hours (240 minutes)
    UserProfile activeUsers[MAX_USERS];     // Array to store active user profiles

    // --- Handle input functions ---
    void handleFirstAccess(TBMessage& msg, UserProfile* profile);   // Handle first time access
    void handleAdminLogin(TBMessage& msg, UserProfile* profile);    // Handle admin login
    void handleBlockedUser(TBMessage& msg, UserProfile* profile);   // Handle blocked user trying to access the bot by informing him about the remaining block time
    void handleAdminPanel(TBMessage& msg, UserProfile* profile);    // Handle admin panel interactions and buttons
    void handleUserPanel(TBMessage& msg, UserProfile* profile);     // Handle user panel interactions and buttons
    void handlePinDuration(TBMessage& msg, UserProfile* profile);   // Handle the admin's input for changing the temporary pin duration and save the new duration in flash memory
    void handleRemoveUser(TBMessage& msg, UserProfile* profile);    // Handle the admin's input for removing a user from the system
    void handleSendRealTime();                                      // Handle the MSP's request for the current time by sending the real-time to the MSP so that it can be displayed on the LCD

    // --- Show menu functions ---
    void showAdminMenu(TBMessage& msg, UserProfile* profile);       // Show admin menu with options to manage users and view logs
    void showUserMenu(TBMessage& msg, UserProfile* profile);        // Show user menu with options to request door access and view pin status
    
    // --- Utility functions ---
    UserProfile* getOrCreateUser(const TBMessage& msg);             // Get existing user or create new UserProfile in activeUsers array
    UserProfile* getUser(int64_t chatId);                           // Get user profile by chat ID
    bool removeUser(int64_t chatId);                                // Remove user profile from array and flash memory 
    bool saveUsersToFlash();                                        // Save user profiles to flash memory
    bool savePinDuration();                                         // Save the pin duration in flash memory
    void revokeUserPin(UserProfile* profile);                       // Send command to MSP to revoke the user's temporary pin
    uint32_t getPinRemainingTime(UserProfile* profile);             // Get the remaining time for the user's temporary pin in minutes
    void rejectPendingUser(const String& userMessage);              // Send a message to the user whose temporary pin request is pending approval to inform them that their request has been rejected by the admin

public:
    // --- Constructor and initialization ---
    DoorBotManager(AsyncTelegram2& telegramBot, HardwareSerial& serialMSP)  // Constructor to initialize the DoorBotManager with references to the Telegram bot and MSP serial port 
    : bot(telegramBot), mspSerial(serialMSP) {}
    bool begin();                                       // Initialize DoorBotManager instance by loading users from flash memory OR starting with an empty user list if there is no flash data (absolute first start)
    
    // --- Handle Telegram Bot functions ---
    void handleTelegramUpdates();   // FSM to handle Telegram messages and user interactions
    void backgroundTasks();         // Check if any blocked user can be unblocked after BLOCK_DURATION has passed AND Check if any user's pin has expired 
    
    // --- Admin buttons ---
    void onPinApprove(TBMessage& msg, UserProfile* profile, const String& cmd);   // Handle the admin's approval of a temporary pin request and send the corresponding command to the MSP
    void onPinReject(TBMessage& msg, UserProfile* profile, const String& cmd);    // Handle the admin's rejection of a temporary pin request and send a message to inform the user about the rejection
    void onPinDuration(TBMessage& msg, UserProfile* profile);   // Allow the admin to change the duration of the temporary pins for users 
    void onRemoveUser(TBMessage& msg, UserProfile* profile);    // Show the admin a list of users with active pins and allow him to select one to remove (revoke the pin and remove the user from the activeUsers array and flash memory)
    void onPinRevokeAll(TBMessage& msg, UserProfile* profile);  // Allow the admin to revoke all active pins for all users at once by sending a command to the MSP for each user with an active pin

    // --- Users buttons ---
    void onPinRequest(TBMessage& msg, UserProfile* profile);    // Send a pin request to the admin
    void onPinStatus(TBMessage& msg, UserProfile* profile);     // Show the remaining time for the temporary pin 
    void onPinRevoke(TBMessage& msg, UserProfile* profile);     // Allow the user to revoke his temporary pin

    // --- Common buttons ---
    void onLogout(TBMessage& msg, UserProfile* profile);    // Handle user logout by resetting their profile and asking them to use /start to access the system again

    // --- MSP communication functions ---
    bool sendCommandToMSP(const char* command);         // Send command to MSP for door control
    MSPResponse parseMSPEvent(const String& event);     // Parse incoming MSP events and return event type
    void listenToMSP();                                 // Listen for responses from MSP
    
    // --- Process MSP events functions ---
    void processAdminPinOk(TBMessage& msg, UserProfile* profile);   // Process the event when the admin pin entered is correct
    void processAdminPinWrong(TBMessage& msg, UserProfile* profile);    // Process the event when the admin pin entered is wrong
    void processTempUserPin(TBMessage& msg, UserProfile* profile, const String& generatedPin);  // Process the event when a temporary user pin has been generated and received from the MSP
    void processRevokePin(TBMessage& msg, UserProfile* profile);    // Process the event when the MSP confirms that a pin has been revoked successfully after a revoke command has been sent

    // --- Test functions ---
    void injectAdminForTesting(int64_t fakeAdminId);
};

#endif // DOORBOTMANAGER_H