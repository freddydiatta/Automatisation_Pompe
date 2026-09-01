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

    // Authentification par email/mot de passe (requise par la regle RTDB "auth != null")
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    Firebase.begin(&config, &auth);

    fbdo.setBSSLBufferSize(4096, 1024);
    fbdo.setResponseSize(4096);

    Serial.print("Authentification Firebase en cours");
    unsigned long authStart = millis();
    while (!Firebase.ready() && millis() - authStart < 10000) {
        Serial.print(".");
        delay(200);
    }
    if (Firebase.ready())
        Serial.println("\nFirebase pret. UID: " + String(auth.token.uid.c_str()));
    else
        Serial.println("\nFirebase non pret apres 10s (nouvelle tentative en tache de fond).");
}

void FirebaseManager::update() {
    static unsigned long lastFirebaseUpdate = 0;
    if (millis() - lastFirebaseUpdate < 1000) return; // 1 update per second maximum
    lastFirebaseUpdate = millis();

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

        // Telemetrie groupee en une seule requete (au lieu de 4 appels separes)
        String relayStr = (digitalRead(relayPin) == RELAY_ON) ? "ON" : "OFF";
        String modeStr = (Mode == 0) ? "AUTO" : ((Mode == 1) ? "MANUAL" : "MAINTENANCE");
        FirebaseJson telemetry;
        telemetry.set("relay_state", relayStr);
        telemetry.set("level_high", !digitalRead(Capteur_Niveau_Haut));
        telemetry.set("level_low", !digitalRead(Capteur_Niveau_Bas));
        telemetry.set("mode", modeStr);
        Firebase.updateNode(fbdo, "/pump", telemetry);

        // Traitement de la commande depuis l'interface web
        // (toujours consommee/remise a IDLE, meme si elle ne s'applique pas au mode actuel,
        // pour ne jamais laisser une commande perimee se declencher plus tard sans raison)
        if (Firebase.getString(fbdo, "/pump/command_state")) {
            String cmd = fbdo.stringData();
            if (cmd != "IDLE") {
                if (Mode == 1) { // Seulement en MANU
                    if (cmd == "ON") setRelayState(RELAY_ON);
                    else if (cmd == "OFF") setRelayState(RELAY_OFF);
                }
                Firebase.setString(fbdo, "/pump/command_state", "IDLE");
            }
        }

        // Changement de mode depuis l'interface web (meme logique : toujours consomme)
        if (Firebase.getString(fbdo, "/pump/set_mode")) {
            String newMode = fbdo.stringData();
            if (newMode != "IDLE") {
                if (newMode == "AUTO") Mode = 0;
                else if (newMode == "MANUAL") Mode = 1;
                else if (newMode == "MAINTENANCE") Mode = 2;
                Firebase.setString(fbdo, "/pump/set_mode", "IDLE");
            }
        }

        // Horodatage de presence : utilise l'heure du serveur Firebase (et non l'horloge
        // locale de l'ESP32, qui n'est fiable que si la synchronisation NTP a reussi au demarrage)
        static unsigned long lastSeenTime = 0;
        if (millis() - lastSeenTime > 5000) {
            Firebase.setTimestamp(fbdo, "/pump/last_seen");
            lastSeenTime = millis();
        }
    }
}

void FirebaseManager::setFault(String message) {
    if (Firebase.ready()) {
        Firebase.setString(fbdo, "/pump/fault", message);
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
