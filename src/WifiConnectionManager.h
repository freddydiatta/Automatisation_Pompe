#ifndef WIFI_CONNECTION_MANAGER_H
#define WIFI_CONNECTION_MANAGER_H

#include <WiFiManager.h>
#include <Arduino.h>

class WifiConnectionManager {
public:
    static WifiConnectionManager& getInstance() {
        static WifiConnectionManager instance;
        return instance;
    }

    void begin();

private:
    WifiConnectionManager() {}
    WiFiManager wifiManager;
};

#endif // WIFI_CONNECTION_MANAGER_H
