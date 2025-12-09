#include "ConnectivityManager.h"

ConnectivityManager::ConnectivityManager() {
    // Khởi tạo các biến nếu cần
}

// Sử dụng tham số ssid_ap để đặt tên AP và xử lý logic kết nối
bool ConnectivityManager::begin(const char* ssid_ap) {
    Serial.println("ConnectivityManager: Bắt đầu quản lý kết nối Wi-Fi...");

    WiFiManager wm;
    
    // Đặt thời gian chờ cho Portal (ví dụ: 180 giây)
    wm.setTimeout(180); 
    
    // Thử kết nối với thông tin đã lưu. Nếu thất bại, tự động mở AP với tên được truyền vào.
    if (!wm.autoConnect(ssid_ap)) { 
        // --- XỬ LÝ KHÔNG CẤU HÌNH ĐƯỢC ---
        Serial.println("Lỗi: Không thể kết nối hoặc cấu hình. Hệ thống sẽ khởi động lại.");
        return false;
    } 

    // --- KẾT NỐI STA THÀNH CÔNG ---
    Serial.println("Kết nối Wi-Fi STA thành công!");
    Serial.print("Địa chỉ IP: ");
    Serial.println(WiFi.localIP());

    // Khởi tạo mDNS (famio.local)
    if (MDNS.begin(MDNS_HOSTNAME)) {
        Serial.printf("mDNS: Truy cập bằng http://%s.local\n", MDNS_HOSTNAME);
    } else {
        Serial.println("mDNS: Khởi tạo thất bại.");
    }
    
    return true; // Kết nối STA thành công
}
