#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <FirebaseESP32.h>
#include <WiFi.h>

class FirebaseManager {
public:
    static FirebaseManager& getInstance() {
        static FirebaseManager instance;
        return instance;
    }

    void begin();
    void update();
    void setRelayState(bool state);
    void setWaterLevels(bool low, bool high);
    void setMode(int mode);

private:
    FirebaseManager() {}
};

#endif // FIREBASE_MANAGER_H
