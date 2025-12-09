#include "AppWebServer.h"
#include <ArduinoJson.h>

// Constructor: Khởi tạo Web Server ở cổng 80 và lưu trữ con trỏ
AppWebServer::AppWebServer(FMRadio* radio, PowerManager* power, FileManager* fileMgr) 
    : server(80), fmRadio(radio), powerManager(power), fileManager(fileMgr) {
    
    // Kiểm tra tính hợp lệ của con trỏ (tùy chọn)
    if (!fmRadio || !powerManager || !fileManager) {
        Serial.println("Cảnh báo: Một số module Radio/Power/File chưa được cấp phát.");
    }
}

// =========================================================
// Khởi tạo Wi-Fi và Server
// =========================================================

bool AppWebServer::begin(const char* ssid, const char* password) {
    Serial.printf("Đang kết nối Wi-Fi tới SSID: %s\n", ssid);
    
    // Khởi tạo Wi-Fi
    WiFi.begin(ssid, password);

    // Chờ kết nối (có thể giới hạn thời gian chờ)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nLỗi: Kết nối Wi-Fi thất bại.");
        return false;
    }

    Serial.println("\nKết nối Wi-Fi thành công!");
    Serial.print("Địa chỉ IP: ");
    Serial.println(WiFi.localIP());

    // Đăng ký tất cả các API endpoints
    registerAPIs();
    
    // Bắt đầu Web Server
    server.begin();
    return true;
}

// =========================================================
// Hàm Đăng ký API (Sử dụng std::bind)
// =========================================================

void AppWebServer::registerAPIs() {
    // 1. Root ("/") - Trang chính
    server.on("/", HTTP_GET, std::bind(&AppWebServer::handleRoot, this));

    // 2. API Lấy trạng thái FM
    server.on("/api/fm/status", HTTP_GET, std::bind(&AppWebServer::handleFmStatus, this));

    // 3. API Điều chỉnh tần số (POST)
    server.on("/api/fm/tune", HTTP_POST, std::bind(&AppWebServer::handleFmTune, this));

    // 4. API Điều chỉnh âm lượng
    server.on("/api/system/volume", HTTP_POST, std::bind(&AppWebServer::handleSystemVolume, this));

    // 5. File tĩnh (Sử dụng onNotFound để bắt các URL khác)
    server.onNotFound(std::bind(&AppWebServer::handleStaticFile, this));
}

// =========================================================
// Xử lý Request Cụ thể
// =========================================================

void AppWebServer::handleRoot() {
    // Phục vụ file index.html từ thẻ SD
    File file = fileManager->openFile("/index.html");
    if (file) {
        server.streamFile(file, "text/html");
        file.close();
    } else {
        server.send(404, "text/plain", "File /index.html not found on SD Card!");
    }
}

void AppWebServer::handleStaticFile() {
    String path = server.uri(); 
    File file = fileManager->openFile(path.c_str());

    if (file) {
        // Gửi file với MIME type tự động đoán
        server.streamFile(file, path); 
        file.close();
    } else {
        server.send(404, "text/plain", "File Not Found");
    }
}

void AppWebServer::handleFmStatus() {
    // Cấp phát bộ nhớ cho phản hồi JSON
    JsonDocument statusDoc; 
    
    // GỌI HÀM CỦA FMRADIO ĐỂ LẤY TRẠNG THÁI THẬT
    // if (fmRadio) {
    //     fmRadio->getStatus(&statusDoc); 
    // } else { ... }

    // Giả lập dữ liệu (Xóa khi tích hợp FMRadio)
    statusDoc["freq"] = 99.5;
    statusDoc["volume"] = 75;
    statusDoc["stereo"] = true;
    statusDoc["signal"] = 68;

    String response;
    serializeJson(statusDoc, response);

    server.send(200, "application/json", response);
}

void AppWebServer::handleFmTune() {
    // Kiểm tra xem tham số POST 'freq' có được gửi không
    if (server.hasArg("freq")) { 
        float newFreq = server.arg("freq").toFloat();
        
        // fmRadio->setFrequency(newFreq); // Gọi hàm của module FMRadio
        
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Tham số 'freq' bị thiếu.");
    }
}

void AppWebServer::handleSystemVolume() {
    // Kiểm tra xem tham số POST 'level' có được gửi không
    if (server.hasArg("level")) { 
        int newLevel = server.arg("level").toInt();
        
        // powerManager->setVolume(newLevel); // Gọi hàm của module PowerManager
        
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Tham số 'level' bị thiếu.");
    }
}

// =========================================================
// Hàm xử lý chính (BẮT BUỘC TRONG LOOP)
// =========================================================

void AppWebServer::handleClient() {
    // Hàm này phải được gọi liên tục trong main loop() để Web Server hoạt động
    server.handleClient();
}