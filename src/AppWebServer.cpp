#include "AppWebServer.h"
#include <ArduinoJson.h>
#include <ConnectivityManager.h>

// Constructor: Khởi tạo Web Server ở cổng 80 và lưu trữ con trỏ
AppWebServer::AppWebServer(FMRadio* radio, PowerManager* power, FileManager* fileMgr, ConnectivityManager* connectivity) 
    : server(80), fmRadio(radio), powerManager(power), fileManager(fileMgr), connectivity(connectivity)  {
    
    // Kiểm tra tính hợp lệ của con trỏ (tùy chọn)
    if (!fmRadio || !powerManager || !fileManager) {
        Serial.println("Cảnh báo: Một số module Radio/Power/File chưa được cấp phát.");
    }
}

// =========================================================
// Khởi tạo Wi-Fi và Server
// =========================================================

bool AppWebServer::begin() {
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
    
    // API Lấy trạng thái FM
    server.on("/api/fm/status", HTTP_GET, std::bind(&AppWebServer::handleFmStatus, this));
    
    // API Điều chỉnh tần số (POST)
    server.on("/api/fm/tune", HTTP_POST, std::bind(&AppWebServer::handleFmTune, this));
    
    // API Điều chỉnh âm lượng
    server.on("/api/system/volume", HTTP_POST, std::bind(&AppWebServer::handleSystemVolume, this));
    
    // API Cấu hình Wi-Fi
    server.on("/api/wifi/status", HTTP_GET, std::bind(&AppWebServer::handleGetWifiStatus, this));
    server.on("/api/wifi/scan", HTTP_GET, std::bind(&AppWebServer::handleScanNetworks, this));
    server.on("/api/wifi/config", HTTP_POST, std::bind(&AppWebServer::handleSubmitWifiConfig, this));
    server.on("/api/wifi/reset", HTTP_POST, std::bind(&AppWebServer::handleResetWifiConfig, this));

    // 1. Root ("/") - Trang chính
    server.on("/", HTTP_GET, std::bind(&AppWebServer::handleRoot, this));
    AppWebServer::handleStaticFile();
}

// =========================================================
// Xử lý Request Cụ thể
// =========================================================

void AppWebServer::handleRoot() {
    // Phục vụ file index.html từ thẻ SD
    File file = fileManager->openFile(UI_PATH "/index.html");
    if (file) {
        server.streamFile(file, "text/html");
        file.close();
    } else {
        server.send(404, "text/plain", "File /index.html not found on SD Card!");
    }
}

void AppWebServer:: handleStaticFile () {
    server.serveStatic("/", SD, PROJECT_ROOT_DIR "/ui/");
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

// --- XỬ LÝ API WIFI ---

void AppWebServer::handleGetWifiStatus() {
    JsonDocument doc;
    doc["isOperational"] = connectivity->isOperational();
    doc["ip"] = connectivity->isOperational() ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    server.send(200, "application/json", jsonResponse);
}

void AppWebServer::handleScanNetworks() {
    int state = connectivity->startScanNetworks();
    JsonDocument doc; // Kích thước lớn hơn để chứa danh sách mạng
    
    if (state == -1) {
        // Đang quét
        doc["status"] = "scanning";
    } else if (state == -2) {
        // Chưa quét/Quét bị xóa, gọi lại scan để kích hoạt
        doc["status"] = "ready_to_scan";
        connectivity->startScanNetworks(); // Gọi lại để bắt đầu quét
    } else {
        // Quét hoàn tất (state >= 0)
        doc["status"] = "complete";
        doc["count"] = state;
        JsonArray networks = doc["networks"].to<JsonArray>();
        connectivity->getScanResults(networks);
    }
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    server.send(200, "application/json", jsonResponse);
}

void AppWebServer::handleSubmitWifiConfig() {
    // Giả định dữ liệu gửi lên là JSON: {"ssid": "...", "pass": "..."}
    if (server.method() != HTTP_POST || !server.hasArg("plain")) {
        server.send(400, "text/plain", "Bad Request");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    String ssid = doc["ssid"].as<String>();
    String pass = doc["pass"].as<String>();
    
    if (connectivity->checkAndSaveCredentials(ssid, pass)) {
        // Kết nối thành công, đã lưu config. Phản hồi và reset.
        server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"Config saved. Restarting device.\"}" );
        delay(100);
        ESP.restart(); // Thực hiện reset
    } else {
        // Kết nối thất bại/timeout
        server.send(400, "application/json", "{\"status\":\"failed\", \"message\":\"Connection failed or timeout after 30s. Check credentials.\"}" );
    }
}

void AppWebServer::handleResetWifiConfig() {
    // API đưa về Provisioning Mode
    server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"Resetting to Provisioning Mode. Device will restart.\"}" );
    delay(100);
    connectivity->resetToProvisioning(); // Thực hiện reset
}