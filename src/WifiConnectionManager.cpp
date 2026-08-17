#include "WifiConnectionManager.h"

void WifiConnectionManager::begin() {
    // Set custom IP for portal if needed, otherwise it defaults to 192.168.4.1
    // wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    // Disable debug output for production
    wifiManager.setDebugOutput(false);

    Serial.println("Initialisation du WiFiManager...");
    // AutoConnect tries to connect to the saved Wi-Fi, if it fails, it opens an AP
    // AP name will be "POMPE_CONFIG"
    if (!wifiManager.autoConnect("POMPE_CONFIG", "AdminPompe123")) {
        Serial.println("Erreur de connexion. Redemarrage de l'ESP32.");
        delay(3000);
        ESP.restart();
    }

    Serial.println("WiFi connecte avec succes !");
    Serial.print("Adresse IP : ");
    Serial.println(WiFi.localIP());
}

bool WifiConnectionManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WifiConnectionManager::update() {
    // If connection is lost, WiFiManager handles auto-reconnect internally,
    // but we can add specific logic here if needed.
}
