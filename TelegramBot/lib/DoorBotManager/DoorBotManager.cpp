#include "DoorBotManager.h"
#include <time.h>
using namespace std;

bool DoorBotManager::begin() {
    // 1. Initialize Serial1 for MSP communication
    mspSerial.begin(115200, SERIAL_8N1, 16, 17); // RX, TX pins of the ESP32 to communicate with the MSP

    // 2. Load users from flash memory, if there are no users stored, we start with an empty array and wait for the first user to configure the bot as admin
    Serial.println("Loading users from flash memory...");
    // Try to open flash memory in read mode
    if(!flashMemory.begin("activeUsers", true, "nvs")) {
        Serial.println("Error opening flash memory for reading active users.");
        Serial.println("User profile of this session will be lost...");
        memset(activeUsers, 0, sizeof(activeUsers));    // Initialize the activeUsers array
        return false;
    } else {
        currentAdminId = flashMemory.getLong64("admin_id",0);   // Load admin id from flash, if it doesn't exist, it will be 0
        pinDuration = flashMemory.getUInt("pin_dur",240);
        uint32_t sizeUsers = flashMemory.getBytesLength("users"); // Get the size of the users stored in flash, if it doesn't exist, it will be 0
        // If the size of the users stored in flash is correct, we load them in the activeUsers array
        if(sizeUsers == sizeof(activeUsers)) {
            flashMemory.getBytes("users", activeUsers, sizeof(activeUsers));    // Load users from flash if the size is correct
            Serial.println("Users loaded from flash memory.");
        }
        // Otherwise we consider that there are no users stored and we start with an empty array anyway
        else {
            memset(activeUsers, 0, sizeof(activeUsers));    // Start with an empty array anyway
            Serial.println("Flash dimension is not correct, resetting Users...");
        }
        flashMemory.end();
        return true;
    }
}

bool DoorBotManager::saveUsersToFlash() {
    Serial.println("Saving User profiles in flash...");
    // Try to open flash memory in write mode
    if(!flashMemory.begin("activeUsers",false, "nvs")) {
        Serial.println("Error opening flash memory for writing active users.");
        return false;   // Cannot open flash memory for writing
    }
    bool success = true;
    // Save the current admin id in flash memory
    if (flashMemory.putLong64("admin_id", currentAdminId) != sizeof(currentAdminId)) {
        Serial.println("Error: failed to write Admin ID in flash memory.");
        success = false;
    }
    // Save the activeUsers array in flash memory
    if (flashMemory.putBytes("users", activeUsers, sizeof(activeUsers)) != sizeof(activeUsers)) {
        Serial.println("Error: failed to write the full activeUsers array to flash memory.");
        success = false;
    }
    flashMemory.end();
    if(success) {
        Serial.println("User profiles saved successfully in flash memory.");
    }
    return success;    // Return true if both admin id and activeUsers array were saved successfully, false otherwise
}

UserProfile* DoorBotManager::getOrCreateUser(const TBMessage& msg) {
    int32_t firstEmptySlot = -1;    // Track the first empty slot in the activeUsers array 
    for(uint32_t i = 0; i < MAX_USERS; ++i) {
        // If the user is already in the activeUsers array, return a pointer to his UserProfile
        if (activeUsers[i].chatId == msg.chatId) {
            Serial.println("User found!");
            return &activeUsers[i];
        }
        // If we find an empty slot, we save its index to create a new user profile if the user is not already in the array
        if (activeUsers[i].chatId == 0 && firstEmptySlot == -1) {
            firstEmptySlot = i;
        }
    }
    // If there is an empty slot, we create a new UserProfile for him and return a pointer to it
    if (firstEmptySlot != -1) {
        Serial.println("Creating new profile...");
        activeUsers[firstEmptySlot].chatId = msg.chatId;
        String name = msg.sender.firstName;
        String surname = msg.sender.lastName;
        if (name.length() == 0 && surname.length() == 0) {
            name = msg.sender.username;
        }
        if (surname.startsWith("null") || surname.length() == 0) {
            char idBuffer[24];
            snprintf(idBuffer, sizeof(idBuffer), "(%lld)", msg.chatId);
            surname = String(idBuffer);
        }
        name.trim();
        surname.trim();
        size_t maxLen = sizeof(activeUsers[firstEmptySlot].firstName) - 1;
        // We use strncpy to safely copy the first name and last name into the UserProfile struct, ensuring we do not exceed the maximum length defined in the struct
        strncpy(activeUsers[firstEmptySlot].firstName, name.c_str(), maxLen);
        strncpy(activeUsers[firstEmptySlot].lastName, surname.c_str(), maxLen);
        // For safety, we ensure that the strings are null-terminated in case they exceed the maximum length allowed in the UserProfile struct
        activeUsers[firstEmptySlot].firstName[maxLen] = '\0';   
        activeUsers[firstEmptySlot].lastName[maxLen] = '\0';
        saveUsersToFlash();
        return &activeUsers[firstEmptySlot];
    }
    return nullptr; // No space to create a new user profile
}

UserProfile* DoorBotManager::getUser(int64_t chatId) {
    if (chatId == 0) return nullptr; // Invalid chat ID, return nullptr immediately
    for(uint32_t i = 0; i < MAX_USERS; ++i) {
        if (activeUsers[i].chatId == chatId) {
            Serial.println("User found!");
            return &activeUsers[i]; // User found
        }
    }
    return nullptr; // User not found
}

bool DoorBotManager::removeUser(int64_t chatId) {
    Serial.println("Trying to remove User profile in flash...");
    // Find the user profile to remove
    UserProfile* userToremove = getUser(chatId);
    // If the user is not found, we cannot remove him
    if(userToremove == nullptr) {
        Serial.println("Deletion failed, this User is not in our system.");
        return false;
    }
    *userToremove = UserProfile();    // Reset the user profile to default values
    if (saveUsersToFlash()) {
        Serial.println("User removed successfully from flash memory.");
        return true;
    } else {
        Serial.println("Error saving changes to flash memory while removing user.");
        return false;
    }
}

void DoorBotManager::handleTelegramUpdates() {
    TBMessage msg;
    // If I recive a new message...
    if (bot.getNewMessage(msg)) {
        // 1. Find who is writing to me OR save it in memory if there is space
        UserProfile* actualUser = getOrCreateUser(msg);
        // If memory is full, send a message to the user and exit from the function
        if (actualUser == nullptr) {
            bot.sendMessage(msg, "Sorry, the system is currently full.\nPlease try again later.");
            return; 
        }
        // 2.FSM for actualUser served by DoorBotManager instance
        switch (actualUser->state) {
            case STATE_FIRST_ACCESS:
                handleFirstAccess(msg, actualUser);
                break;
            case STATE_WAITING_PW:
                handleAdminLogin(msg, actualUser);
                break;
            case STATE_ADMIN_PANEL:
                handleAdminPanel(msg, actualUser);
                break;
            case STATE_USER_PANEL:
                handleUserPanel(msg, actualUser);
                break;
            case STATE_ADMIN_BLOCKED:
                handleBlockedUser(msg, actualUser);
                break;
            case STATE_WAITING_PIN_DURATION:               
                handlePinDuration(msg, actualUser);
                break;
            case STATE_REM_USER:
                handleRemoveUser(msg, actualUser);
                break;
            case STATE_WAITING_REVOKE:
                bot.sendMessage(msg, "Please wait for the system to process the revocation of your pin...");
                break;
        }
    }
    backgroundTasks();
}

void DoorBotManager::backgroundTasks() {
    bool usersChanged = false;
    uint32_t currentTime = time(nullptr);
    for (uint32_t i = 0; i < MAX_USERS; i++) {
        if (activeUsers[i].chatId == 0) continue; // Skip empty slots in the user array
        // 1. Check if any blocked user can be unblocked after BLOCK_DURATION has passed
        if (activeUsers[i].state == STATE_ADMIN_BLOCKED) {
            if(activeUsers[i].blockedUntil <= currentTime) {
                activeUsers[i].state = STATE_FIRST_ACCESS;
                activeUsers[i].retryCount = 0;
                activeUsers[i].blockedUntil = 0;
                usersChanged = true;
                TBMessage msg;
                msg.chatId = activeUsers[i].chatId;
                msg.messageType = MessageType::MessageText;
                bot.sendMessage(msg, "You have been unblocked. Please use /start to access the menu again.");
                Serial.printf("User %lld unblocked.\n", activeUsers[i].chatId);
            } 
        }
        // 2. Check if any user's pin has expired and revoke it if necessary
        if (activeUsers[i].hasActivePin) {
            // FIX: If waiting for confirmation, resend the command ONLY if the 60-second backoff timer has elapsed
            if (activeUsers[i].state == STATE_WAITING_REVOKE) {
                if (currentTime >= activeUsers[i].blockedUntil) {
                    String cmdToMSP = "REVOKE_PIN:" + String(activeUsers[i].chatId);  
                    if (sendCommandToMSP(cmdToMSP.c_str())) {
                        activeUsers[i].blockedUntil = currentTime + 60; // Reschedule the next retry in 60 seconds
                        usersChanged = true;
                        Serial.printf("Retrying revocation for user %lld...\n", activeUsers[i].chatId);
                    }
                }
            }
            // Normal expiration check for active pins
            else if (getPinRemainingTime(&activeUsers[i]) == 0) {
                revokeUserPin(&activeUsers[i]);
                usersChanged = true;
                TBMessage msg;
                msg.chatId = activeUsers[i].chatId;
                msg.messageType = MessageType::MessageText;
                bot.sendMessage(msg, "Your temporary pin has expired. Wait for the system to process it...");
                Serial.printf("User %lld's pin has expired and has been revoked.\n", activeUsers[i].chatId);
            }
        }
    }
    // If any user's state has changed, we save the updated user profiles to flash memory
    if (usersChanged) {
        saveUsersToFlash();
        Serial.println("Background Tasks: User profiles updated and saved to flash memory.");
    }
}

void DoorBotManager::handleFirstAccess(TBMessage& msg, UserProfile* profile) {
    if(currentAdminId == 0) {
        // No one has configured the door yet
        if(msg.messageType == MessageType::MessageText) {
            bot.sendMessage(msg, "Welcome to the Door Lock System!");
            bot.sendMessage(msg, "It seems that you are the first user to access this bot.");
            // FIX: Added cancel keyboard to prevent the FSM from getting stuck if the first user doesn't input a PIN
            InlineKeyboard cancelKeyboard;
            cancelKeyboard.addButton("Cancel", "cmd_cancel", KeyboardButtonQuery);
            bot.sendMessage(msg, "Please Insert Admin Pin to configure the bot:", cancelKeyboard);
            profile->state = STATE_WAITING_PW;
            saveUsersToFlash();
        }
        return; // If the messagge is not text (diff. from /start), we ingnore it
    }
    InlineKeyboard firstKeyboard;
    firstKeyboard.addButton("Admin", "cmd_admin", KeyboardButtonQuery);
    firstKeyboard.addButton("User", "cmd_user", KeyboardButtonQuery);
    firstKeyboard.addRow();
    firstKeyboard.addButton("Cancel", "cmd_cancel", KeyboardButtonQuery);
    //The system already has an Admin, is just a new User OR Admin from a different device
    switch(msg.messageType) {
        // Ask the user if he is an Admin or a normal User
        case MessageType::MessageText: {
            if (msg.text == "/start") {
                bot.sendMessage(msg, "Welcome to the Door Lock System!");
                bot.sendMessage(msg, "Please identify yourself:", firstKeyboard);
            } else {
                bot.sendMessage(msg, "Please press the Log In button or use /start to log in.");
            }
            break;
        }
        // When a button is pressed...
        case MessageType::MessageQuery: {
            bot.endQuery(msg, "");
            String cmd = msg.callbackQueryData;
            // If the user selects the Admin role, we ask for the PIN to verify their identity
            if(cmd == "cmd_admin") {
                bot.editMessage(msg.chatId, msg.messageID, "You selected: Admin", "");
                InlineKeyboard cancelKeyboard;
                cancelKeyboard.addButton("Cancel", "cmd_cancel", KeyboardButtonQuery);
                bot.sendMessage(msg, "Please insert Admin PIN:", cancelKeyboard);
                profile->state = STATE_WAITING_PW;
            }
            // If the user selects the normal User role, we log them in as User 
            else if(cmd == "cmd_user") {
                bot.editMessage(msg.chatId, msg.messageID, "You selected: User", "");
                bot.sendMessage(msg, "Ok, you've logged in as a User.");
                profile->state = STATE_USER_PANEL;
                showUserMenu(msg, profile);
            }
            // If the Admin/User wants to cancel the login, we reset his state to the initial state
            else if (cmd == "cmd_cancel") {
                String text = "You have been logged out. Use the button below or /start to log in again.";
                InlineKeyboard loginKeyboard;
                loginKeyboard.addButton("Log In", "cmd_login", KeyboardButtonQuery);
                bot.editMessage(msg, text, loginKeyboard);
            }
            // If the user wants to log in again and we ask him to identify himself as Admin or User again
            else if (cmd == "cmd_login") {
                // FIX: Removed the check on currentAdminId to allow anyone to choose their role upon re-login
                bot.editMessage(msg, "Welcome back to the Door Lock System!\nPlease identify yourself:", firstKeyboard);
            }
            // If the admin recive a request for a new temporary pin while he is not logged in...
            else if (cmd.startsWith("cmd_app:") || cmd.startsWith("cmd_rej:")) {
                // FIX: Allow multiple requests by concatenating them with a '|' delimiter.
                if (msg.chatId == currentAdminId) {
                    // Prevent duplicate commands if the admin double-clicks the same button.
                    if (pendingAdminCmd.indexOf(cmd) == -1) {
                        // If there is already a pending command, we concatenate the new command with a '|' delimiter
                        if (pendingAdminCmd != "") {
                            pendingAdminCmd += "|";
                        }
                        pendingAdminCmd += cmd;
                    }
                    bot.editMessage(msg, "Please insert Admin PIN to confirm this action(s):", "");
                    profile->state = STATE_WAITING_PW;
                    saveUsersToFlash();
                } else {
                    bot.sendMessage(msg, "Sorry, you don't have permission to do this.");
                }
            }
            else {
                // If the command is unknown, we inform the user
                bot.sendMessage(msg, "Unknown command received.");
                return; 
            }
            saveUsersToFlash();
            break;
        }
        // Ignore photos, videos, audios, ...
        default:
            bot.sendMessage(msg, "Please send only text messages or use the buttons to interact with the bot.");
            break;
    }
}

void DoorBotManager::handleAdminLogin(TBMessage& msg, UserProfile* profile) {
    // If the admin wants to cancel the login, we reset his state to the initial state and we clear any pending command that requires admin approval
    bool isCancel = false;
    if (msg.messageType == MessageType::MessageQuery && msg.callbackQueryData == "cmd_cancel") {
        bot.endQuery(msg, "");  
        bot.editMessage(msg, "Admin login cancelled. Use /start to access the menu again.", "");
        isCancel = true;
    } else if (msg.messageType == MessageType::MessageText && msg.text == "/cancel") {
        bot.deleteMessage(msg.chatId, msg.messageID);   // Delete the /cancel command message 
        bot.sendMessage(msg, "Admin login cancelled. Use /start to access the menu again.");
        isCancel = true;
    }
    if (isCancel) {
        profile->state = STATE_FIRST_ACCESS;
        // FIX: Clear the pending command on explicit cancel to prevent ghost execution during future logins
        if (pendingAdminCmd != "") {
            pendingAdminCmd = "";
        }
        saveUsersToFlash();
        return;
    }
    // If the message is not text, we ignore it and ask the user to send only text messages
    if(msg.messageType != MessageType::MessageText) {
        bot.sendMessage(msg, "Please send only a text message!");
        return;
    }
    // Check if the PIN is of 4 digits
    if (msg.text.length() != 4) {
        bot.sendMessage(msg, "The PIN must be of 4 digits. Please, retry.");
        return;
    }
    // Check if the PIN contains only digits
    for (int i = 0; i < 4; i++) {
        if (!isdigit(msg.text[i])) {
            bot.sendMessage(msg, "The PIN must contain ONLY numbers (0-9). Please, retry.");
            return;
        }
    }
    bot.deleteMessage(msg.chatId, msg.messageID); // Delete the message containing the PIN for security reasons
    String command = "VERIFY_PIN:" + msg.text + ":" + String(profile->chatId);  // Compose the command to send to the MSP
    if(sendCommandToMSP(command.c_str())) {
        bot.sendMessage(msg, "Verifying the PIN sent...");
    } else {
        // FIX: Provide feedback to the Admin if the hardware Serial port transmission fails
        bot.sendMessage(msg, "Hardware serial error: unable to reach the lock controller. Please try again.");
        Serial.println("I cannot send the PIN to the MSP board");
    }
}

void DoorBotManager::handleBlockedUser(TBMessage& msg, UserProfile* profile) {
    // Check if the block duration has passed and unblock the user if necessary
    if (time(nullptr) >= profile->blockedUntil) {
        profile->state = STATE_FIRST_ACCESS;
        profile->retryCount = 0;
        profile->blockedUntil = 0;
        saveUsersToFlash();
    } 
    // Otherwise, we inform the user that he is still blocked and how much time is left before he can try to log in again
    else {
        uint32_t last = profile->blockedUntil - time(nullptr);
        if (last > 60) {
            uint32_t minutes = last / 60;
            uint32_t seconds = last % 60;
            bot.sendMessage(msg, "You are still blocked for " + String(minutes) + " minute(s) and " + String(seconds) + " second(s).");
        } else {
            bot.sendMessage(msg, "You are still blocked for " + String(last) + " seconds.");
        }
    }
}

void DoorBotManager::revokeUserPin(UserProfile* profile) {
    if (!profile->hasActivePin || profile->state == STATE_WAITING_REVOKE) return; // check for security
    String cmdToMSP = "REVOKE_PIN:" + String(profile->chatId);  // Compose the command to send to the MSP
    if (sendCommandToMSP(cmdToMSP.c_str())) {
        profile->state = STATE_WAITING_REVOKE;
        // FIX: use the 'blockedUntil' variable to schedule the next retry attempt in 60 seconds
        profile->blockedUntil = time(nullptr) + 60;
        saveUsersToFlash();
        Serial.println("Revoke pin command sent to MSP successfully.");
    } else {
        Serial.println("Failed to send revoke pin command to MSP.");
    }   
}

void DoorBotManager::handleAdminPanel(TBMessage& msg, UserProfile* profile) {
    // If the message is a text message, we check if it's the /menu command to show the admin menu
    if (msg.messageType == MessageType::MessageText) {
        if (msg.text == "/menu") {
            bot.deleteMessage(msg.chatId, msg.messageID);
            showAdminMenu(msg, profile);
        }
        // Otherwise we ask the admin to use /menu to interact with the bot
        else {
            bot.sendMessage(msg, "Please use the /menu command to access the Admin Control Panel.");
        }
    }
    // If the message is a query, we check which button has been pressed and we call the corresponding function to handle the request
    else if (msg.messageType == MessageType::MessageQuery) {
        bot.endQuery(msg, "");  // End the query to remove the loading animation on the button
        String cmd = msg.callbackQueryData;
        if (cmd.startsWith("cmd_app:")) {
            onPinApprove(msg, profile, cmd);
        }
        else if (cmd.startsWith("cmd_rej:")) {
            onPinReject(msg, profile, cmd);
        }
        else if (cmd.startsWith("cmd_pin_dur")) {
            onPinDuration(msg, profile);
        }
        else if (cmd.startsWith("cmd_rem_user")) {
            onRemoveUser(msg, profile);
        }
        else if (cmd == "cmd_rev_all") {
            onPinRevokeAll(msg, profile);
        }
        else if (cmd == "cmd_logout") {
            onLogout(msg, profile);
        }
    }
}

void DoorBotManager::showAdminMenu(TBMessage& msg, UserProfile* profile) {
    InlineKeyboard adminMenu;
    adminMenu.addButton("PIN duration", "cmd_pin_dur", KeyboardButtonQuery);
    adminMenu.addRow();
    adminMenu.addButton("Remove User", "cmd_rem_user", KeyboardButtonQuery);
    adminMenu.addRow();
    adminMenu.addButton("Revoke all PINS ", "cmd_rev_all", KeyboardButtonQuery);
    adminMenu.addRow();
    adminMenu.addButton("Logout", "cmd_logout", KeyboardButtonQuery);
    if (bot.sendMessage(msg,"Admin's Control Panel",adminMenu)) {
        Serial.println("Error, Json of the admin menu is too big to be sent.");
    }
}

void DoorBotManager::onPinDuration(TBMessage& msg, UserProfile* profile) {
    profile->state = STATE_WAITING_PIN_DURATION;
    saveUsersToFlash();
    // Inform the admin about the current PIN duration and ask him to insert a new one in minutes, or to cancel the operation
    String text = "The actual PIN duration is: ";
    if (pinDuration >= 60) {
        uint32_t hours = pinDuration / 60;
        uint32_t minutes = pinDuration % 60;
        if (hours > 0) {
            text += String(hours) + " hour(s) and ";
        }
        text += String(minutes) + " minute(s).";
    } else {
        text += String(pinDuration) + " minute(s).";
    }
    text += "\nPlease insert below the new PIN duration in minutes OR insert /cancel to exit.";
    InlineKeyboard cancelKb;
    cancelKb.addButton("Cancel", "cmd_cancel", KeyboardButtonQuery);
    bot.editMessage(msg, text, cancelKb);
}

bool DoorBotManager::savePinDuration() {
    if (!flashMemory.begin("activeUsers", false, "nvs")) {
        Serial.println("Error opening flash memory for writing PIN duration.");
        return false;   // Cannot open flash memory for writing
    }
    bool success = true;
    if (flashMemory.putUInt("pin_dur", pinDuration) == 0) {
        Serial.println("Error: failed to write PIN duration in flash memory.");
        success = false;
    }
    flashMemory.end();
    if (success) {
        Serial.println("PIN duration saved successfully.");
    }
    return success;
}

void DoorBotManager::handlePinDuration(TBMessage& msg, UserProfile* profile) {
    // If the admin wants to cancel the operation...
    if (msg.messageType == MessageType::MessageQuery && msg.callbackQueryData == "cmd_cancel") {
        bot.endQuery(msg, "");
        profile->state = STATE_ADMIN_PANEL;
        saveUsersToFlash();
        bot.editMessage(msg, "PIN duration update cancelled.", "");
        showAdminMenu(msg, profile);
        return;
    }
    // If the message is not text, we ignore it
    if (msg.messageType != MessageType::MessageText) {
        bot.sendMessage(msg, "Please send only a text message with the new PIN duration in minutes, or /cancel to exit.");
        return;
    }
    // The admin can cancel the operation by sending the /cancel command as a text message as well
    if (msg.text.startsWith("/cancel")) {
        bot.deleteMessage(msg.chatId, msg.messageID);
        profile->state = STATE_ADMIN_PANEL;
        saveUsersToFlash();
        bot.sendMessage(msg, "PIN duration update cancelled.");
        showAdminMenu(msg, profile);
        return;
    }
    int32_t newDuration = msg.text.toInt(); // Convert the text message to an integer representing the new duration in minutes
    // If the conversion fails or the duration is not positive, we ask the admin to retry
    if (newDuration > 0) {
        pinDuration = newDuration;
        savePinDuration();
        bot.sendMessage(msg, "PIN duration updated successfully.");
        profile->state = STATE_ADMIN_PANEL;
        saveUsersToFlash(); 
        showAdminMenu(msg, profile);
    } else {
        bot.sendMessage(msg, "Invalid PIN duration. Please enter a positive number.");
    }
}

void DoorBotManager::onRemoveUser(TBMessage& msg, UserProfile* profile) {
    InlineKeyboard usersWithPin;
    String text, cmd;
    for (int32_t i = 0; i < MAX_USERS; ++i) {
        if (activeUsers[i].chatId != 0 && activeUsers[i].chatId != profile->chatId) {
            text = String(activeUsers[i].firstName) + " " + String(activeUsers[i].lastName);
            cmd = "cmd_rem_user:" + String(activeUsers[i].chatId);
            usersWithPin.addButton(text.c_str(), cmd.c_str(), KeyboardButtonQuery);
            usersWithPin.addRow();
        }
    }
    usersWithPin.addButton("Cancel", "cmd_rem_user:cancel", KeyboardButtonQuery);
    Serial.println("Showing list of active Users to admin...");
    profile->state = STATE_REM_USER;
    saveUsersToFlash();
    bot.editMessage(msg, "Choose a User to remove:", usersWithPin);
}

void DoorBotManager::handleRemoveUser(TBMessage& msg, UserProfile* profile) {
    if (profile->state != STATE_REM_USER) {
        bot.sendMessage(msg, "Please use the /menu command to access the Admin Control Panel and select the option to remove a user.");
        return; // For safety, we check that the admin is in the correct state to remove a user, otherwise we ignore the message
    }
    if (msg.messageType == MessageType::MessageQuery) {
        String cmd = msg.callbackQueryData;
        // If the admin wants to cancel the operation...
        if (cmd == "cmd_rem_user:cancel") {
            profile->state = STATE_ADMIN_PANEL;
            saveUsersToFlash();
            bot.editMessage(msg, "User removal cancelled.", "");
            showAdminMenu(msg, profile);
            return;
        }
        // Otherwise, we try to remove the selected user
        size_t delimeter = cmd.indexOf(':');
        if (delimeter != -1) {
            int64_t targetId = atoll(cmd.substring(delimeter+1).c_str());  // Extract the user ID from the callback data
            // For security, check that the chatId extracted from cmd is not the same as the current admin's chat ID
            if (targetId == currentAdminId) {
                bot.editMessage(msg, "Sorry, you cannot remove yourself from the system.", "");
            } else {
                UserProfile* userToRem = getUser(targetId); // Get the UserProfile of the user to remove, if it exists
                // If the user exists, we check if he has an active pin. 
                if (userToRem != nullptr) {
                    // FIX: To prevent "Ghost PINs", NEVER delete a user silently from the ESP32.
                    // Even if ESP32 thinks there is no active pin (due to UART drops), we FORCE a revocation 
                    // to ensure the MSP432 safely cleans its flash memory before the profile is destroyed.
                    userToRem->hasActivePin = true; 
                    if (userToRem->state != STATE_WAITING_REVOKE) {
                        revokeUserPin(userToRem);
                    }
                    bot.editMessage(msg, "Revocation request sent to the system. The user will be removed upon confirmation.", "");
                }
                // Otherwise, if the user is not found in the system, we inform the admin that there was an error and we show the admin menu again
                else {
                    bot.editMessage(msg, "Error: User not found in the system", "");
                    showAdminMenu(msg, profile);
                }
            }
        }
        profile->state = STATE_ADMIN_PANEL;
        showAdminMenu(msg, profile);
    } else {
        bot.sendMessage(msg, "Please use the buttons to select a user to remove or cancel the operation.");
    }
}

void DoorBotManager::onPinRevokeAll(TBMessage& msg, UserProfile* profile) {
    int requestedCount = 0;   // Counter to keep track of how many pins have been requested for revocation
    for (uint32_t i = 0; i < MAX_USERS; i++) {
        // For security, we check that the user has an active pin and that it's not the admin
        if (activeUsers[i].chatId != 0 && activeUsers[i].chatId != currentAdminId && activeUsers[i].hasActivePin) {
            revokeUserPin(&activeUsers[i]);   // Request the revocation of the user's pin
            requestedCount++; // Increment the counter of requested revocations
        }
    }
    // 
    if (requestedCount > 0) {
        bot.editMessage(msg, "Sent revocation requests to the system for " + String(requestedCount) + " user(s). Waiting for confirmation...", "");
    } else {
        bot.editMessage(msg, "There are no active PINs to revoke at the moment.", "");
    }
    //
    profile->state = STATE_ADMIN_PANEL;
    showAdminMenu(msg, profile);
}

void DoorBotManager::handleUserPanel(TBMessage& msg, UserProfile* profile) {
    // If the message is a text message, we check if it's the /menu command to show the user menu
    if (msg.messageType == MessageType::MessageText) {
        if (msg.text == "/menu") {
            bot.deleteMessage(msg.chatId, msg.messageID);
            showUserMenu(msg, profile);
        } 
        // Otherwise we ask the user to use the menu to interact with the bot
        else {
            bot.sendMessage(msg, "Please use the /menu command to access the User Control Panel.");
        }
    }
    // If the message is a query, we check which button has been pressed and we call the corresponding function to handle the request
    else if (msg.messageType == MessageType::MessageQuery) {
        String cmd = msg.callbackQueryData;
        if (cmd == "cmd_pin_request") {
            onPinRequest(msg, profile);
        }
        if (cmd == "cmd_pin_status") {
            onPinStatus(msg, profile);
        }
        if (cmd == "cmd_pin_revoke") {
            onPinRevoke(msg, profile);
        }
        if (cmd == "cmd_logout") {
            onLogout(msg, profile);
        }
        bot.endQuery(msg, "");  // End the query to remove the loading animation on the button
    }
}

void DoorBotManager::showUserMenu(TBMessage& msg, UserProfile* profile) {
    InlineKeyboard userMenu;
    if(!profile->hasActivePin) {
        userMenu.addButton("Request Temporary Pin", "cmd_pin_request", KeyboardButtonQuery);
    } else {
        userMenu.addButton("Temporary Pin Duration", "cmd_pin_status", KeyboardButtonQuery);
        userMenu.addRow();
        userMenu.addButton("Revoke Temporary Pin", "cmd_pin_revoke", KeyboardButtonQuery);
    }
    userMenu.addRow();
    userMenu.addButton("Logout", "cmd_logout", KeyboardButtonQuery);
    if (!bot.sendMessage(msg,"User's Control Panel", userMenu)) {
        Serial.println("Error, Json of the user menu is too big to be sent.");
    }
}

void DoorBotManager::onPinRequest(TBMessage& msg, UserProfile* profile) {
    InlineKeyboard approvalMenu;
    String text = "User " + msg.sender.firstName + " " + msg.sender.lastName + " is requesting temporary access to the door. Do you approve?";
    String appCommand = "cmd_app:" + String(profile->chatId);
    String rejCommand = "cmd_rej:" + String(profile->chatId);
    approvalMenu.addButton("Approve request", appCommand.c_str(), KeyboardButtonQuery);
    approvalMenu.addButton("Reject request", rejCommand.c_str(), KeyboardButtonQuery);
    Serial.println("Sending request for temporary access to the Admin...");
    bot.editMessage(msg, "Request sent to the Administrator. Please wait for approval.", "");   // Inform the user that the request has been sent to the admin
    // Send a new message to the admin to request the approval for the temporary pin
    TBMessage adminMsg;
    adminMsg.chatId = currentAdminId; 
    adminMsg.messageType = MessageType::MessageText;
    bot.sendMessage(adminMsg, text.c_str(), approvalMenu);
}

void DoorBotManager::onPinApprove(TBMessage& msg, UserProfile* profile, const String& cmd) {
    String targetId = cmd.substring(8); // Extract the user ID from the callback data
    int64_t targetUserId = atoll(targetId.c_str()); // Convert the extracted string to an integer representing the user ID
    // Find the user profile with targetUserId in the activeUsers array
    UserProfile* targetUser = getUser(targetUserId);
    String userName = targetId; // Fallback to using the user ID if the user profile is not found
    if (targetUser != nullptr) {
        userName = String(targetUser->firstName) + " " + String(targetUser->lastName);
    }
    String mspCmd = "GEN_PIN:" + targetId;  // Create the command to send to MSP for generating a temporary pin for the user
    if (sendCommandToMSP(mspCmd.c_str())) {
        bot.sendMessage(msg, "Requesting temporary pin for user " + userName + "...");
        Serial.println("Command sent to MSP to generate temporary pin for user " + userName);
    } else {
        bot.sendMessage(msg, "Failed to send command to MSP. Retry later.");
        Serial.println("Failed to send command to MSP for generating temporary pin for user " + userName);
    }
}

void DoorBotManager::onPinReject(TBMessage& msg, UserProfile* profile, const String& cmd) {
    // Extract the User chatId from the callback query data
    String targetId = cmd.substring(8);
    int64_t targetUserId = atoll(targetId.c_str());
    if (targetUserId == 0) {
        bot.sendMessage(msg, "Error, this User ID is not valid");
    }
    UserProfile* userToReject = getUser(targetUserId);
    if (userToReject == nullptr) {
        bot.sendMessage(msg, "Sorry, this User is not in the system anymore.");
        Serial.println("Error: the user to reject is not in the system anymore.");
        return;
    }
    String text = "Rejected the request for temporary access from user " + String(userToReject->firstName) + " " + String(userToReject->lastName);
    Serial.println(text.c_str());    
    bot.sendMessage(msg, text);  // Inform the admin that he has rejected the request
    // Inform the user that the admin has rejected his request and that he has been logged out
    TBMessage msgUser;
    msgUser.chatId = targetUserId;
    msgUser.messageType = MessageType::MessageText;
    bot.sendMessage(msgUser, "Sorry, the Administrator has rejected your request for temporary access.");   // Inform the user that the admin has rejected the request
    InlineKeyboard loginKeyboard;
    loginKeyboard.addButton("Log In", "cmd_login", KeyboardButtonQuery);
    bot.sendMessage(msgUser, "You have been logged out. Use the button below or /start to log in again.", loginKeyboard); // Inform the user that he has been logged out
    removeUser(targetUserId);   // Now remove the user from the system
}

void DoorBotManager::rejectPendingUser(const String& userMessage) {
    if (pendingAdminCmd == "") {
        return; // No pending command to reject
    }
    int startIdx = 0;
    // FIX: Iterate through all commands concatenated with the '|' delimiter to reject everyone in queue
    while (startIdx < pendingAdminCmd.length()) {
        int pipeIdx = pendingAdminCmd.indexOf('|', startIdx);
        if (pipeIdx == -1) pipeIdx = pendingAdminCmd.length();
        String singleCmd = pendingAdminCmd.substring(startIdx, pipeIdx);
        // Extract the User chatId from the single pending command
        String targetId = singleCmd.substring(8);
        int64_t targetUserId = atoll(targetId.c_str());
        // Inform the user that the admin has rejected his request and that he has been logged out
        TBMessage userMsg;
        userMsg.chatId = targetUserId;
        userMsg.messageType = MessageType::MessageText;
        bot.sendMessage(userMsg, userMessage);
        removeUser(targetUserId); 
        startIdx = pipeIdx + 1;
    }
    pendingAdminCmd = "";   // Clear the pending command after rejecting the user
    Serial.println("Pending user request rejected and cleared.");
}

uint32_t DoorBotManager::getPinRemainingTime(UserProfile* profile) {
    if (!profile->hasActivePin) {
        return 0; // No active pin, so remaining time is 0
    }
    uint32_t currentTime = time(nullptr);
    if (currentTime >= profile->pinExpiration) {
        return 0; // Pin already expired
    }
    return profile->pinExpiration - currentTime; 
}

void DoorBotManager::onPinStatus(TBMessage& msg, UserProfile* profile) {
    String text;
    uint32_t secondsLeft = getPinRemainingTime(profile);
    if (secondsLeft > 0) {
        uint32_t hours = secondsLeft / 3600;
        uint32_t minutes = (secondsLeft % 3600) / 60;
        String timeString;
        if (hours > 0) {
            timeString += String(hours) + " hour(s) and ";
        }
        timeString += String(minutes) + " minute(s).";
        text = "Your temporary pin is still valid for " + timeString + ".";
    } else {
        text = "You have not an active pin.";
    }
    bot.editMessage(msg, text, "");
}

void DoorBotManager::onPinRevoke(TBMessage& msg, UserProfile* profile) {
    // Check if the user has an active pin to revoke
    if (!profile->hasActivePin) {
        bot.editMessage(msg, "You don't have an active PIN to revoke.", "");
        return;
    }
    // Send the command to the MSP to revoke the pin for this user
    revokeUserPin(profile);
    bot.editMessage(msg, "Requesting PIN revocation. Please wait...", "");
}

void DoorBotManager::onLogout(TBMessage& msg, UserProfile* profile) {
    String text = "You have been logged out. Use the button below or /start to log in again.";
    InlineKeyboard loginKeyboard;
    loginKeyboard.addButton("Log In", "cmd_login", KeyboardButtonQuery);
    bot.editMessage(msg, text, loginKeyboard);
    profile->state = STATE_FIRST_ACCESS;
    saveUsersToFlash();
}

bool DoorBotManager::sendCommandToMSP(const char* command) {
    uint32_t cmdLength = strlen(command) + 2;   // Add 2 bytes for '/r' and '/n'
    if(mspSerial.availableForWrite() > cmdLength) {
        mspSerial.println(command);
        Serial.println("Command sent to MSP: " + String(command));
        return true;    // Command sent successfully
    }
    return false;   // Not enough space in the serial buffer to send the command
}

MSPResponse DoorBotManager::parseMSPEvent(const String& event) {
    MSPResponse result = {EVENT_UNKNOWN, -1};
    int32_t firstDelimiter = event.indexOf(':');    // The first delimiter is used to separate the command part from the chat ID part
    if(firstDelimiter == -1) {
        Serial.println("Error UART: delimiter ':' not found");
        return result;
    }
    String commandPart = event.substring(0,firstDelimiter); // Get the command part of the event, which is the part of the string before the first delimiter
    int32_t secondDelimiter = event.indexOf(':', firstDelimiter + 1);   // The second delimiter is used to separate the chat ID part from the payload part (if it exists)
    if (secondDelimiter != -1) {
        result.chatId = atoll(event.substring(firstDelimiter + 1, secondDelimiter).c_str());
        result.payload = event.substring(secondDelimiter + 1);  // The payload is the part of the event after the second delimiter, if it exists
    } else {
        result.chatId = atoll(event.substring(firstDelimiter + 1).c_str()); // If there is no second delimiter, the id part is the rest of the string after the first delimiter
    }
    // Using a lookup table to parse the incoming event from MSP and return the corresponding MSPEventType
    struct Command {
        const char* text;
        MSPEventType event;
    };
    // Events that the MSP can send via UART
    const Command lookup[] {
        {"PIN_OK",      EVENT_ADMIN_PIN_OK},
        {"PIN_WRONG",   EVENT_ADMIN_PIN_WRONG},
        {"TEMP_PIN",    EVENT_TEMP_PIN},
        {"REVOKE_OK",   EVENT_REVOKE_OK},
        {"REQ_TIME",    EVENT_REQ_TIME}
    };
    uint32_t numCommands = sizeof(lookup) / sizeof(lookup[0]);  // Calculate the number of commands in the lookup table
    for(uint32_t i = 0; i < numCommands; ++i) {
        if (commandPart.equals(lookup[i].text)) {
            result.event = lookup[i].event;
            return result; // Event found in the lookup table, return the corresponding MSPEventType
        }
    }
    return result;   // If the event is not found, it returns EVENT_UNKNOWN
}

void DoorBotManager::listenToMSP() {
    TBMessage msg;
    msg.messageType = MessageType::MessageText;
    // FIX: Implemented an asynchronous non-blocking internal buffer to accumulate bytes token-by-token
    static char command[64];
    static uint8_t bufferIndex = 0;
    // If there is data available from MSP, read it and parse the event
    while (mspSerial.available() > 0) { // FIX: Use a while loop to read all available bytes from the serial buffer in one go, instead of relying on multiple calls to listenToMSP which might cause delays in processing the incoming data
        char c = mspSerial.read();
        // Cond. A: If the token is a newline character, we consider the command complete and we parse it. We also handle the case of carriage return before newline for better compatibility with different serial implementations.
        if (c == '\n') {
            // Null-terminate the string safely
            command[bufferIndex] = '\0';
            if (bufferIndex > 0 && command[bufferIndex - 1] == '\r') {
                command[bufferIndex - 1] = '\0';
            }
            Serial.println("Received command from MSP: " + String(command));
            MSPResponse mspResponse = parseMSPEvent(command);    // Parse the incoming command and get the corresponding MSPEventType
            msg.chatId = mspResponse.chatId;    // Set the chat ID of the message to the chat ID extracted from the MSP event
            UserProfile* profile = getUser(msg.chatId); // Get the UserProfile corresponding to the chat ID extracted from the MSP event
            switch(mspResponse.event) {
                case EVENT_ADMIN_PIN_OK:
                    processAdminPinOk(msg, profile);
                    break;
                case EVENT_ADMIN_PIN_WRONG:
                    processAdminPinWrong(msg, profile);
                    break;
                case EVENT_TEMP_PIN:
                    processTempUserPin(msg, profile, mspResponse.payload);                     
                    break;
                case EVENT_REVOKE_OK:
                    processRevokePin(msg, profile);
                    break;
                case EVENT_REQ_TIME:
                    handleSendRealTime();
                    break;
                case EVENT_UNKNOWN:
                    Serial.println("MSP command unknown or corrupted.");
                    break;
            }
            bufferIndex = 0; // Reset the buffer index for the next command
        }
        // Cond. B: If the token is not a newline character, we add it to the buffer if there is still space
        else if (bufferIndex < 63) {
            command[bufferIndex++] = c;
        } 
        // Cond. C: Otherwise we reset the buffer to prevent overflow and potential corruption
        else {
            bufferIndex = 0; // Overflow handling: packet too long or corrupted, discarding buffer
        }
    }
}

void DoorBotManager::processAdminPinOk(TBMessage& msg, UserProfile* profile) {
    // FIX: Check if it's the absolute first system configuration
    bool wasFirstConfig = (currentAdminId == 0);
    // If the old admin is still in the system, we revoke his admin privileges 
    if (currentAdminId != 0 && currentAdminId != profile->chatId) {
        UserProfile* oldAdmin = getUser(currentAdminId);
        // For security, we check if the old admin profile still exists in the system before trying to revoke his privileges, in order to avoid potential issues caused by a malicious command with a fake chat ID
        if (oldAdmin != nullptr) {
            oldAdmin->isAdmin = false;
            oldAdmin->state = STATE_FIRST_ACCESS;
            // Inform the old admin that his privileges have been revoked and that he has been logged out
            TBMessage oldAdminMsg;
            oldAdminMsg.chatId = currentAdminId;
            oldAdminMsg.messageType = MessageType::MessageText;
            bot.sendMessage(oldAdminMsg, "Another device has successfully logged in as Administrator. Your admin privileges have been revoked and you have been logged out.");
            removeUser(currentAdminId);   // Now we can remove the old admin from the system to free up space in the activeUsers array
            Serial.printf("Admin privileges revoked for old device: %lld\n", currentAdminId);
        }
    }
    // Now we can set the new admin privileges for the user who has just logged in successfully
    profile->isAdmin = true;
    currentAdminId = profile->chatId; // Set the current admin ID to the chat ID
    profile->retryCount = 0; // Reset retry count on successful login
    // FIX: If the Admin role is being assigned for the first time, dynamically reset any other slot stuck in setup phase
    if (wasFirstConfig) {
        for (uint32_t i = 0; i < MAX_USERS; i++) {
            if (activeUsers[i].chatId != 0 && activeUsers[i].chatId != profile->chatId && activeUsers[i].state == STATE_WAITING_PW) {
                activeUsers[i].state = STATE_FIRST_ACCESS;
                // Inform the user of the reset due to the new admin configuration
                TBMessage resetMsg;
                resetMsg.chatId = activeUsers[i].chatId;
                resetMsg.messageType = MessageType::MessageText;
                bot.sendMessage(resetMsg, "The system has just been configured by another Admin device. Please send /start to register as a regular User or change the Admin device.");
            }
        }
    }
    // If there are pending admin commands that require approval, we process them now that the admin has successfully logged in
    if (pendingAdminCmd != "") {
        int startIdx = 0;
        // FIX: Iterate through all commands concatenated with the '|' delimiter
        while (startIdx < pendingAdminCmd.length()) {
            int pipeIdx = pendingAdminCmd.indexOf('|', startIdx);
            if (pipeIdx == -1) pipeIdx = pendingAdminCmd.length();
            String singleCmd = pendingAdminCmd.substring(startIdx, pipeIdx);
            if (singleCmd.startsWith("cmd_app:")) {
                onPinApprove(msg, profile, singleCmd);
            } 
            else if (singleCmd.startsWith("cmd_rej:")) {
                onPinReject(msg, profile, singleCmd);
            }
            startIdx = pipeIdx + 1; // Move to the next command in the pendingAdminCmd string
        }
        pendingAdminCmd = "";   // Clear the pending commands after processing them
        profile->state = STATE_FIRST_ACCESS;
    }
    // Otherwise we show him the admin menu
    else {
        profile->state = STATE_ADMIN_PANEL;
        showAdminMenu(msg, profile);
    }
    if(saveUsersToFlash()) {
        Serial.println("Admin profile updated successfully in flash memory.");
    } else {
        Serial.println("Error saving admin profile to flash memory.");
    }
}

void DoorBotManager::processAdminPinWrong(TBMessage& msg, UserProfile* profile) {
    profile->retryCount += 1;
    // If the user has reached the maximum number of retries, block him for BLOCK_DURATION seconds
    if(profile->retryCount == MAX_RETRY) {
        profile->state = STATE_ADMIN_BLOCKED;
        profile->blockedUntil = time(nullptr) + BLOCK_DURATION;
        rejectPendingUser("The authorization process failed due to security reasons. Please try requesting a pin again later. ");
        bot.sendMessage(msg, "You have entered the wrong PIN too many times. You are now blocked for 5 minutes.");
    } 
    // Otherwise, we inform him of the wrong PIN and how many attempts he has left before being blocked
    else {
        uint32_t attemptsLeft = MAX_RETRY - profile->retryCount;
        bot.sendMessage(msg, "Wrong PIN! Please try again.\nAttempts remaining: " + String(attemptsLeft));
    }
    saveUsersToFlash();
}

void DoorBotManager::processTempUserPin(TBMessage& msg, UserProfile* profile, const String& generatedPin) {
    profile->hasActivePin = true;
    profile->pinExpiration = time(nullptr) + pinDuration * 60; // Set the pin expiration time to the current time plus the pinDuration in seconds
    saveUsersToFlash();
    // Inform the user that his temporary pin has been generated successfully and show him the pin and its duration
    String text = "Your temporary pin has been generated successfully!\n\n";
    text += "Generated Pin: ** " + generatedPin + " **\n\n";
    text += "It will be valid for ";
    if (pinDuration >= 60) {
        uint32_t hours = pinDuration / 60;
        uint32_t minutes = pinDuration % 60;
        if (hours > 0) {
            text += String(hours) + " hour(s) and ";
        }
        text += String(minutes) + " minute(s).";
    } else {
        text += String(pinDuration) + " minute(s).";
    }
    bot.sendMessage(msg, text);
    // Inform the admin that the temporary pin has been generated for the user
    if (currentAdminId != 0) {
        TBMessage adminMsg;
        adminMsg.chatId = currentAdminId;
        adminMsg.messageType = MessageType::MessageText;
        String adminText = "Success, a temporary pin has been generated for user " + String(profile->firstName) + " " + String(profile->lastName) + ".";
        bot.sendMessage(adminMsg, adminText);
    }
}

void DoorBotManager::processRevokePin(TBMessage& msg, UserProfile* profile) {
    // Inform the user that his temporary pin has been revoked successfully and that he has been logged out
    TBMessage userMsg;
    userMsg.chatId = profile->chatId;
    userMsg.messageType = MessageType::MessageText;
    bot.sendMessage(userMsg, "Your temporary PIN has been successfully revoked by the system. Use /start to access the system again and request a new temporary pin.");
    // Inform the admin that the temporary pin has been revoked for the user
    if (currentAdminId != 0 && currentAdminId != profile->chatId) {
        TBMessage adminMsg;
        adminMsg.chatId = currentAdminId;
        adminMsg.messageType = MessageType::MessageText;
        bot.sendMessage(adminMsg, "Confirmed: PIN revoked and user " + String(profile->firstName) + " " + String(profile->lastName) + " has been removed successfully.");
    }
    removeUser(profile->chatId);    // Now we can remove the user from the system to free up space in the activeUsers array
}

void DoorBotManager::injectAdminForTesting(int64_t fakeAdminId) {
    currentAdminId = fakeAdminId;
    TBMessage msg;
    msg.messageType = MessageType::MessageText;
    msg.chatId = fakeAdminId;
    UserProfile* p = getOrCreateUser(msg);
    if (p != nullptr) {
        p->isAdmin = true;
        p->state = STATE_ADMIN_PANEL;
        saveUsersToFlash();
        Serial.printf("TEST MODE: Fake Admin %lld injected successfully!\n", fakeAdminId);
    }
}

void DoorBotManager::handleSendRealTime() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char response[64];
    
    // Format the time as TIME_SYNC:HH:MM:SS:DD:MM:YYYY:DOW (DOW = Day Of Week, where Sunday=0, Monday=1, ..., Saturday=6)
    snprintf(response, sizeof(response), "TIME_SYNC:%02d:%02d:%02d:%02d:%02d:%04d:%d", 
             timeinfo->tm_hour, 
             timeinfo->tm_min,
             timeinfo->tm_sec, 
             timeinfo->tm_mday, 
             timeinfo->tm_mon + 1, 
             timeinfo->tm_year + 1900, 
             timeinfo->tm_wday);
    sendCommandToMSP(response);
}