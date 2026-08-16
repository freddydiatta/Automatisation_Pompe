#include "FirebaseManager.h"
#include "OTAManager.h"

#define API_KEY "AIzaSyA1XJf_CcvcEL3vL_-HmO9MoeUK6qQi9ss"
#define DATABASE_URL "https://automatisationpompe-default-rtdb.firebaseio.com/"

using namespace firebase;

DefaultNetwork network;
NoAuth noAuth;
FirebaseApp app;
RealtimeDatabase Database;
AsyncClient aClient(network);

void FirebaseManager::begin() {
    Serial.println("Initialisation de Firebase...");
    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    // Initialisation sans authentification
    app.getApp<NoAuth>(noAuth);
    Database.begin(DATABASE_URL);
}

void FirebaseManager::update() {
    // Check for incoming OTA command
    if (aClient.networkReady()) {
        String url = Database.get<String>(aClient, "/pump/ota_url");
        if (aClient.lastError().code() == 0 && url != "" && url != "NONE") {
            // Clear the URL in DB so it doesn't loop
            Database.set<String>(aClient, "/pump/ota_url", "NONE");
            // Trigger OTA
            OTAManager::getInstance().beginUpdate(url);
        }
    }
}

void FirebaseManager::setRelayState(bool state) {
    if (aClient.networkReady()) {
        Database.set<bool>(aClient, "/pump/relay_state", state);
    }
}

void FirebaseManager::setWaterLevels(bool low, bool high) {
    if (aClient.networkReady()) {
        Database.set<bool>(aClient, "/pump/level_low", low);
        Database.set<bool>(aClient, "/pump/level_high", high);
    }
}

void FirebaseManager::setMode(int mode) {
    if (aClient.networkReady()) {
        Database.set<int>(aClient, "/pump/mode", mode);
    }
}
