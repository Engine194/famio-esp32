#ifndef APPWEBSERVER_H
#define APPWEBSERVER_H

#include <WiFi.h>
#include <WebServer.h>      // Thư viện WebServer chuẩn
#include "FMRadio.h"        // Cần để điều khiển FM
#include "PowerManager.h"   // Cần để điều khiển nguồn
#include "FileManager.h"    // Cần để phục vụ file tĩnh và lưu config
#include "Constants.h"      // Nơi chứa các hằng số

class AppWebServer {
public:
    // Constructor nhận con trỏ của các module khác
    AppWebServer(FMRadio* radio, PowerManager* power, FileManager* fileMgr);

    // Khởi tạo và kết nối Wi-Fi
    bool begin(const char* ssid, const char* password); 

    // QUAN TRỌNG: Hàm này PHẢI được gọi liên tục trong loop()
    void handleClient(); 

private:
    // Khai báo đối tượng WebServer
    WebServer server; 
    
    // Con trỏ tới các module khác
    FMRadio* fmRadio;
    PowerManager* powerManager;
    FileManager* fileManager;

    // Hàm đăng ký tất cả các API endpoints
    void registerAPIs();

    // Các hàm xử lý request cụ thể
    void handleRoot();
    void handleStaticFile();
    void handleFmStatus();
    void handleFmTune();
    void handleSystemVolume();
    // ... Thêm các hàm xử lý API khác
};

#endif // APPWEBSERVER_H