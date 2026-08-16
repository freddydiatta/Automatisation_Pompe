#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

class OTAManager {
public:
    static OTAManager& getInstance() {
        static OTAManager instance;
        return instance;
    }

    // Call this when an OTA update is requested
    // It will safely shut down the pump relay before beginning the update
    void beginUpdate(const String& firmwareUrl);

private:
    OTAManager() {}
    void safeShutdown();
};

#endif // OTA_MANAGER_H
