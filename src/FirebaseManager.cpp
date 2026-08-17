#include "FirebaseManager.h"
#include "PumpControl.h"
#include "config.h"
#include <RTClib.h>

extern RTC_DS3231 rtc;
#include "OTAManager.h"
#include "secrets.h"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void FirebaseManager::begin() {
    Serial.println("Initialisation de Firebase...");
    Serial.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    // Enable WiFi reconnection
    Firebase.reconnectWiFi(true);

    // Initialisation
    Firebase.begin(&config, &auth);

    fbdo.setBSSLBufferSize(4096, 1024);
    fbdo.setResponseSize(4096);
}

void FirebaseManager::update() {
    // Check for incoming OTA command
    if (Firebase.ready()) {
        if (Firebase.getString(fbdo, "/pump/ota_url")) {
            String url = fbdo.stringData();
            if (url != "" && url != "NONE") {
                // Clear the URL in DB so it doesn't loop
                Firebase.setString(fbdo, "/pump/ota_url", "NONE");
                // Trigger OTA
                OTAManager::getInstance().beginUpdate(url);
            }
        }

        String relayStr = (digitalRead(relayPin) == RELAY_ON) ? "ON" : "OFF";
        Firebase.setString(fbdo, "/pump/relay_state", relayStr);

        Firebase.setBool(fbdo, "/pump/level_high", !digitalRead(Capteur_Niveau_Haut));
        Firebase.setBool(fbdo, "/pump/level_low", !digitalRead(Capteur_Niveau_Bas));

        String modeStr = (Mode == 0) ? "AUTO" : ((Mode == 1) ? "MANUAL" : "MAINTENANCE");
        Firebase.setString(fbdo, "/pump/mode", modeStr);

        // Traitement de la commande depuis l'interface web
        if (Firebase.getString(fbdo, "/pump/command_state")) {
            String cmd = fbdo.stringData();
            if (cmd == "ON" && Mode == 1) { // Seulement en MANU
                setRelayState(RELAY_ON);
                Firebase.setString(fbdo, "/pump/command_state", "IDLE"); 
            } else if (cmd == "OFF" && Mode == 1) {
                setRelayState(RELAY_OFF);
                Firebase.setString(fbdo, "/pump/command_state", "IDLE"); 
            }
        }

        // Changement de mode depuis l'interface web
        if (Firebase.getString(fbdo, "/pump/set_mode")) {
            String newMode = fbdo.stringData();
            if (newMode == "AUTO" && Mode != 0) {
                Mode = 0;
                Firebase.setString(fbdo, "/pump/set_mode", "IDLE");
            } else if (newMode == "MANUAL" && Mode != 1) {
                Mode = 1;
                Firebase.setString(fbdo, "/pump/set_mode", "IDLE");
            } else if (newMode == "MAINTENANCE" && Mode != 2) {
                Mode = 2;
                Firebase.setString(fbdo, "/pump/set_mode", "IDLE");
            }
        }
        
        static unsigned long lastSeenTime = 0;
        if (millis() - lastSeenTime > 5000) {
            Firebase.setInt(fbdo, "/pump/last_seen", time(nullptr));
            lastSeenTime = millis();
        }
    }
}

void FirebaseManager::setRelayState(bool state) {
    if (Firebase.ready()) {
        Firebase.setBool(fbdo, "/pump/relay_state", state);
    }
}

void FirebaseManager::setWaterLevels(bool low, bool high) {
    if (Firebase.ready()) {
        Firebase.setBool(fbdo, "/pump/level_low", low);
        Firebase.setBool(fbdo, "/pump/level_high", high);
    }
}

void FirebaseManager::setMode(int mode) {
    if (Firebase.ready()) {
        String modeStr = (mode == 0) ? "AUTO" : ((mode == 1) ? "MANUAL" : "MAINTENANCE");
        Firebase.setString(fbdo, "/pump/mode", modeStr);
    }
}

void FirebaseManager::addLog(String category, String message, String severity, String timestamp) {
    if (Firebase.ready()) {
        FirebaseJson json;
        json.set("category", category);
        json.set("message", message);
        json.set("sev", severity);
        json.set("ts", timestamp);
        Firebase.pushJSON(fbdo, "/pump/logs", json);
    }
}
