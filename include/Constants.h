#ifndef CONSTANTS_H
#define CONSTANTS_H

// =========================================================
// 1. Cấu hình SD Card (SPI Pins)
// =========================================================
#define SPI_SCK_PIN 18  // Serial Clock (SCK)
#define SPI_MISO_PIN 19 // Master In Slave Out (MISO)
#define SPI_MOSI_PIN 23 // Master Out Slave In (MOSI)
#define SD_CS_PIN 5     // Chip Select (CS)

// =========================================================
// 2. Quản lý Nguồn (PowerManager)
// =========================================================
// Pin cho Pin Lithium ADC (Đọc điện áp pin 3S)
#define BATTERY_ADC_PIN 34

// =========================================================
// 3. Cấu hình Wi-Fi Mặc định (WebServer)
// =========================================================
#define DEFAULT_WIFI_SSID "ESP32_Famio_AP"
#define DEFAULT_WIFI_PASS "12345678"
#define MDNS_HOSTNAME "famio"

#define STA_SSID_CONFIG_KEY "sta_ssid"
#define STA_PWD_CONFIG_KEY "sta_password"

#define AP_SSID_CONFIG_KEY "ap_ssid"
#define AP_PWD_CONFIG_KEY "ap_password"

#define CONNECTION_TIMEOUT_S 30

// =========================================================
// 4. Cấu hình I2S cho DAC PCM5102A (Đã dời chân để tránh I2C)
// =========================================================
#define I2S_BCK_PIN  26
#define I2S_WS_PIN   25  // Bạn đã xác nhận bỏ chân 25 cũ, nên dùng làm WS (LRCK)
#define I2S_DOUT_PIN 27  // Chuyển từ 22 sang 27 để tránh SCL của I2C

// Thư mục gốc chứa tất cả dữ liệu dự án trên SD Card
#define PROJECT_ROOT_DIR "/famio"
#define CONFIG_FILE_PATH "/config" // Đường dẫn file config Wi-Fi trên SD Card
#define UI_PATH "/ui"
#define WIFI_CONFIG_FILE "/wifi.json"
#define COMMON_CONFIG_FILE "/common.json"
#define BT_CONFIG_FILE "/bluetooth.json"

#endif // CONSTANTS_H