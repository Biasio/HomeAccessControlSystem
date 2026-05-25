#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <DoorBotManager.h>
#include "time.h"

#if __has_include("credential.h")
    #include "credential.h"
#else
    #error "FATAL ERROR: 'credential.h' missing! Please copy 'credential-template.h' to 'credential.h' and fill in your data."
#endif

const int64_t ledPin = 38;
WiFiClientSecure client;

AsyncTelegram2 myBot(client);
DoorBotManager botManager(myBot, Serial1);

void setup() {
    Serial.begin(115200);
    delay(3000);
    pinMode(ledPin, OUTPUT);
    rgbLedWrite(ledPin, 0, 0, 0);

    Serial.println("\n========================================");
    Serial.println("    SYSTEM INITIALIZATION STARTING      ");
    Serial.println("========================================\n");

    // 1. Wi-Fi Connection with timeout
    Serial.print("Connecting to Wi-Fi SSID: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    uint32_t wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - wifiStart > 15000) {
            Serial.println("\n[FATAL] Wi-Fi connection timeout! Restarting...");
            ESP.restart();
        }
        Serial.print(".");
        delay(500);
    }
    
    Serial.println("\n[OK] Wi-Fi Connected!");
    Serial.print("     Assigned IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("----------------------------------------\n");

    // 2. NTP Time Synchronization with timeout
    Serial.println("Synchronizing system clock via NTP:");
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
    
    time_t now = time(nullptr);
    uint32_t ntpStart = millis();
    
    while (now < 100000) { 
        if (millis() - ntpStart > 20000) {
            Serial.println("\n[FATAL] NTP synchronization timeout! Restarting...");
            ESP.restart();
        }
        Serial.print(".");
        delay(500);
        now = time(nullptr);
    }
    
    Serial.println("\n[OK] Real-time clock synchronized successfully!");
    Serial.println("----------------------------------------\n");

    // 3. SSL Configuration
    client.setInsecure();

    // 4. Start the Telegram bot
    myBot.setUpdateTime(TELEGRAM_UPDATE_TIME); 
    myBot.setTelegramToken(BOT_TOKEN);

    Serial.println("Testing secure connection to Telegram servers...");
    if (!client.connect("api.telegram.org", 443)) {
        Serial.println("[ERROR] Network Fail: Cannot reach api.telegram.org");
        char err_buf[100];
        client.lastError(err_buf, 100);
        Serial.print("        SSL Error Details: ");
        Serial.println(err_buf);
        Serial.println("----------------------------------------\n");
    } else {
        Serial.println("[OK] SSL Handshake test successful.");
        client.stop(); 
        Serial.println("----------------------------------------\n");
    }

    Serial.println("Launching Telegram Bot Instance...");
    if (myBot.begin()) {
        rgbLedWrite(ledPin, 0, 10, 0);
        botManager.begin();
        Serial.println("[SUCCESS] Telegram bot is online and polling updates.");
        Serial.println("========================================\n");
    } else {
        Serial.println("[FATAL] Telegram initialization (myBot.begin()) failed.");
        rgbLedWrite(ledPin, 10, 0, 0);
        Serial.println("========================================\n");
    }
}

void loop() {
    botManager.handleTelegramUpdates(); 
    botManager.listenToMSP(); 
}