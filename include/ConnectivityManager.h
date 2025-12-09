#ifndef CONNECTIVITYMANAGER_H
#define CONNECTIVITYMANAGER_H

#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFi.h>

class ConnectivityManager {
public:
    // Constructor
    ConnectivityManager();

    // Khởi tạo Wi-Fi, chạy Portal nếu cần, và khởi động mDNS.
    // Nhận tên AP_SSID cho chế độ Portal từ config.json
    bool begin(const char* ssid_ap);

private:
    // Tên miền mDNS cố định (ví dụ: http://famio.local)
    const char* MDNS_HOSTNAME = "famio"; 
};

#endif // CONNECTIVITYMANAGER_H