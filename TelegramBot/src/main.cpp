#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <DoorBotManager.h>
#include "time.h"

const int64_t ledPin = 38;

const char ssid[] = "Galaxy A52 5G di Michele";
const char password[] = "michele03";
WiFiClientSecure client;

const char bot_token[] = "8620740437:AAHVbki-ZPTGqgVfXTVypP7QdyOv1XQ6s78";
AsyncTelegram2 myBot(client);
DoorBotManager botManager(myBot, Serial1);

void setup() {
    // Serial + RGB LED setup
    Serial.begin(115200);
    delay(3000);
    pinMode(ledPin, OUTPUT);
    rgbLedWrite(ledPin, 0, 0, 0);

    // 1. Wi-Fi Connection
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nWi-Fi Connected! IP: " + WiFi.localIP().toString());

    // 2. NTP Time Synchronization
    Serial.print("Sinchronizing clock via NTP...\n");
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    // Wait until the time is synchronized 
    while (now < 100000) { 
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("\nReal time clock synchronized!");

    // 3. Set Telegram certificate for secure connection
    client.setCACert(telegram_cert);

    // 4. Start the Telegram bot
    myBot.setUpdateTime(TELEGRAM_UPDATE_TIME);
    myBot.setTelegramToken(bot_token);

    Serial.println("Connecting to Telegram... ");
    if (myBot.begin()) {
        // If the connection is successful, the LED RGB becomes GREEN and we initialize the bot manager
        rgbLedWrite(ledPin, 0, 10, 0);
        botManager.begin(); // Initialize the door bot manager
        Serial.println("Telegram connected!");
    } else {
        // Otherwise, if the connection fails, the LED RGB becomes RED and we print an error message on the serial monitor
        Serial.println("Telegram error. Turn on red LED!");
        rgbLedWrite(ledPin, 10, 0, 0);
    }
}

void loop() {
    botManager.handleTelegramUpdates(); // Handle Telegram updates and user interactions
    botManager.listenToMSP(); // Listen for responses from MSP
}