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
#include "BluetoothManager.h"
#include "ConnectivityManager.h"

// =========================================================
// Khai báo các Đối tượng Toàn cục (Global Managers)
// =========================================================

FileManager fileManager;
BluetoothManager bluetooth(&fileManager);
PowerManager powerManager;
FMRadio fmRadio(&fileManager);
ConnectivityManager connectivityManager(&fileManager);
AppWebServer appWebServer(&fmRadio, &powerManager, &fileManager, &bluetooth, &connectivityManager);

// =========================================================
// Setup() - Khởi tạo Hệ thống
// =========================================================

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n--- Bắt đầu Hệ thống Famio FM Radio ESP32 ---");


    // 1. Kiểm tra sự tồn tại vật lý của PSRAM
    if (psramInit()) {
        Serial.println("PSRAM: Đã tìm thấy chip vật lý và khởi tạo thành công.");
    } else {
        Serial.println("PSRAM: Không tìm thấy chip hoặc khởi tạo thất bại!");
    }

    // 2. Kiểm tra dung lượng PSRAM khả dụng
    size_t psramSize = ESP.getPsramSize();
    size_t freePsram = ESP.getFreePsram();

    if (psramSize > 0) {
        Serial.printf("Tổng dung lượng PSRAM: %d bytes (%.2f MB)\n", psramSize, psramSize / (1024.0 * 1024.0));
        Serial.printf("Dung lượng PSRAM trống: %d bytes\n", freePsram);
    } else {
        Serial.println("CẢNH BÁO: Hệ thống không nhận được dung lượng PSRAM nào.");
    }

    // 3. Kiểm tra Heap nội bộ (RAM mặc định của ESP32) để đối chiếu
    Serial.printf("DRAM trống (Internal RAM): %d bytes\n", ESP.getFreeHeap());
    Serial.println("--------------------------------\n");

    // Khởi tạo PowerManager và SD Card trước
    powerManager.begin();
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
    if (!fileManager.begin())
    {
        Serial.println("Lỗi nghiêm trọng: Không thể khởi tạo SD Card.");
        return;
    }

    // 1. TẢI CẤU HÌNH (Sử dụng JsonDocument, phù hợp với v7)
    JsonDocument commonConfig;

    // Đường dẫn được lấy từ Constants.h (PROJECT_ROOT_DIR)
    if (!fileManager.loadJsonFile(CONFIG_FILE_PATH COMMON_CONFIG_FILE, &commonConfig))
    {
        Serial.println("Không tải được /config/common.json. Sử dụng cấu hình mặc định.");
    }

    int initialVolume = commonConfig["volume"] | 50;
    float initialFreq = commonConfig["freq"] | 99.5f;

    // QUẢN LÝ KẾT NỐI WI-FI
    if (!connectivityManager.begin())
    {
        // Nếu kết nối/cấu hình thất bại, khởi động lại để thử lại
        Serial.println("Hệ thống không thể kết nối");
        while (1)
            ;
    }

    // KHỞI TẠO WEB SERVER
    appWebServer.begin();
    Wire.begin();
    // HOẶC: Wire.begin(SDA_PIN, SCL_PIN); nếu bạn dùng chân tùy chỉnh
    Serial.println("SETUP: Khởi tạo I2C Bus thành công.");
}

// =========================================================
// Loop() - Vòng lặp Chính
// =========================================================

void loop()
{
    appWebServer.handleClient();
    delay(10);
}