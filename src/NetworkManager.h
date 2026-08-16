#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFiManager.h>
#include <Arduino.h>

class NetworkManager {
public:
    static NetworkManager& getInstance() {
        static NetworkManager instance;
        return instance;
    }

    void begin();
    bool isConnected();
    void update();

private:
    NetworkManager() {}
    WiFiManager wifiManager;
};

#endif // NETWORK_MANAGER_H
