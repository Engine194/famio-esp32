#include <Arduino.h>
#include <SPI.h> 
#include <Wire.h> 
#include <WiFi.h> 
#include <ArduinoJson.h> // Thư viện JSON

// Bao gồm các file Header của các lớp đã tạo
#include "Constants.h"
#include "FileManager.h"
#include "PowerManager.h"
#include "FMRadio.h"
#include "AppWebServer.h"
#include "ConnectivityManager.h" 

// =========================================================
// Khai báo các Đối tượng Toàn cục (Global Managers)
// =========================================================

FileManager fileManager; 
PowerManager powerManager;
FMRadio fmRadio;
ConnectivityManager connectivityManager; 
AppWebServer appWebServer(&fmRadio, &powerManager, &fileManager);

// =========================================================
// Setup() - Khởi tạo Hệ thống
// =========================================================

void setup() {
    Serial.begin(115200);
    delay(100); 
    Serial.println("\n--- Bắt đầu Hệ thống Famio FM Radio ESP32 ---");

    // Khởi tạo PowerManager và SD Card trước
    powerManager.begin();
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
    if (!fileManager.begin()) {
        Serial.println("Lỗi nghiêm trọng: Không thể khởi tạo SD Card.");
    }
    
    // 1. TẢI CẤU HÌNH (Sử dụng JsonDocument, phù hợp với v7)
    JsonDocument config; 
    
    // Đường dẫn được lấy từ Constants.h (PROJECT_ROOT_DIR)
    if (!fileManager.loadJsonFile(PROJECT_ROOT_DIR "/config.json", &config)) { 
        Serial.println("Không tải được config.json. Sử dụng cấu hình mặc định.");
    }
    
    // Lấy các giá trị (sử dụng toán tử | để cung cấp giá trị mặc định)
    const char* apSsid = config["wifi"]["ap_ssid"] | "Famio_Radio_Setup_AP";
    // Mật khẩu mặc định: "12345678"
    const char* apPassword = config["wifi"]["ap_password"] | "12345678"; 
    int initialVolume = config["volume"] | 50;
    float initialFreq = config["freq"] | 99.5f;

    // 2. QUẢN LÝ KẾT NỐI WI-FI
    if (!connectivityManager.begin(apSsid, apPassword)) {
        // Nếu kết nối/cấu hình thất bại, khởi động lại để thử lại
        Serial.println("Hệ thống không thể kết nối. Khởi động lại sau 5s.");
        delay(5000);
        ESP.restart(); 
    }
    
    // 3. KHỞI TẠO THIẾT BỊ
    powerManager.setVolume(initialVolume);
    fmRadio.begin(); 
    
    // 4. KHỞI TẠO WEB SERVER
    appWebServer.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());

    Serial.printf("Hệ thống đã sẵn sàng tại http://famio.local (IP: %s)\n", WiFi.localIP().toString().c_str());
}

// =========================================================
// Loop() - Vòng lặp Chính
// =========================================================

void loop() {
    // 1. Xử lý Web Server
    appWebServer.handleClient();
    
    // 2. Kiểm tra biến trở âm lượng
    static unsigned long lastPotentiometerRead = 0;
    const unsigned long POT_READ_INTERVAL = 100; // Đọc 10 lần/giây
    
    if (millis() - lastPotentiometerRead > POT_READ_INTERVAL) {
        int potVolume = powerManager.readPotentiometer();
        int currentVolume = powerManager.getVolume();
        
        // Cập nhật volume nếu biến trở thay đổi đáng kể
        if (abs(potVolume - currentVolume) > 1) {
            powerManager.setVolume(potVolume);
        }
        lastPotentiometerRead = millis();
    }

    // 3. Cập nhật trạng thái Pin (5 giây/lần)
    static unsigned long lastBatteryUpdate = 0;
    const unsigned long BATT_UPDATE_INTERVAL = 5000; 
    
    if (millis() - lastBatteryUpdate > BATT_UPDATE_INTERVAL) {
        powerManager.getBatteryLevel();
        lastBatteryUpdate = millis();
    }
}