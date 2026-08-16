#include "FirebaseManager.h"
#include "OTAManager.h"

// Provide your Firebase project credentials here
#define API_KEY "AIzaSyA1XJf_CcvcEL3vL_-HmO9MoeUK6qQi9ss"
#define DATABASE_URL "https://automatisationpompe-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void FirebaseManager::begin() {
    Serial.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    Firebase.reconnectWiFi(true);

    // Initialisation sans authentification (Anonyme)
    Firebase.begin(&config, &auth);

    fbdo.setBSSLBufferSize(4096, 1024);
    fbdo.setResponseSize(4096);
}

void FirebaseManager::update() {
    // Check for incoming OTA command
    if (Firebase.ready()) {
        if (Firebase.RTDB.getString(&fbdo, "/pump/ota_url")) {
            String url = fbdo.stringData();
            if (url != "" && url != "NONE") {
                // Clear the URL in DB so it doesn't loop
                Firebase.RTDB.setString(&fbdo, "/pump/ota_url", "NONE");
                
                // Trigger OTA
                OTAManager::getInstance().beginUpdate(url);
            }
        }
    }
}

void FirebaseManager::setRelayState(bool state) {
    if (Firebase.ready()) {
        Firebase.RTDB.setBool(&fbdo, "/pump/relay_state", state);
    }
}

void FirebaseManager::setWaterLevels(bool low, bool high) {
    if (Firebase.ready()) {
        Firebase.RTDB.setBool(&fbdo, "/pump/level_low", low);
        Firebase.RTDB.setBool(&fbdo, "/pump/level_high", high);
    }
}

void FirebaseManager::setMode(int mode) {
    if (Firebase.ready()) {
        Firebase.RTDB.setInt(&fbdo, "/pump/mode", mode);
    }
}
