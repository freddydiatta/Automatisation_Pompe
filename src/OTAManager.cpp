#include "OTAManager.h"
#include "config.h"
#include <HTTPClient.h>
#include <Update.h>

void OTAManager::safeShutdown() {
    Serial.println("!!! OTA UPDATE DEMARRE !!!");
    Serial.println("COUPURE DE SECURITE DE LA POMPE.");
    // Force the relay to OFF. Note that RELAY_OFF is defined in config.h (typically HIGH for active-low relays)
    digitalWrite(relayPin, RELAY_OFF);
}

void OTAManager::beginUpdate(const String& firmwareUrl) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi non connecte, impossible de lancer l'OTA.");
        return;
    }

    safeShutdown();

    Serial.println("Telechargement du firmware depuis : " + firmwareUrl);

    HTTPClient http;
    http.begin(firmwareUrl);
    
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Erreur HTTP lors du telechargement : %d\n", httpCode);
        http.end();
        return;
    }

    int contentLength = http.getSize();
    bool canBegin = Update.begin(contentLength);
    
    if (canBegin) {
        Serial.println("Debut de la mise a jour OTA. Veuillez patienter...");
        size_t written = Update.writeStream(http.getStream());

        if (written == contentLength) {
            Serial.println("Ecriture reussie !");
        } else {
            Serial.printf("Ecriture partielle : %d / %d\n", written, contentLength);
        }

        if (Update.end()) {
            Serial.println("Mise a jour OTA terminee avec succes ! Redemarrage...");
            delay(1000);
            ESP.restart();
        } else {
            Serial.printf("Erreur Update.end(): %s\n", Update.errorString());
        }
    } else {
        Serial.printf("Espace insuffisant pour la mise a jour (Taille %d)\n", contentLength);
    }
    http.end();
}
