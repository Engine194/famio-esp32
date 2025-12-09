#ifndef FMRADIO_H
#define FMRADIO_H

#include <Arduino.h>
#include <Wire.h>          // Thư viện I2C (Wire)
#include <ArduinoJson.h>   // Cần để trả về trạng thái JSON

// Địa chỉ I2C của RDA5807M
#define RDA5807M_ADDR 0x11 

// Kích thước bước nhảy tần số (ví dụ: 0.1 MHz)
#define FREQ_STEP 10        // 10 = 100 kHz (0.1 MHz)

enum FMRadioBand {
    BAND_FM_WORLD,          // 87.0 MHz - 108.0 MHz
    BAND_FM_JP_WIDE,        // 76.0 MHz - 108.0 MHz 
    // Thêm các băng tần khác nếu cần
};

class FMRadio {
public:
    // Constructor
    FMRadio();

    // Khởi tạo I2C và chip RDA5807M
    void begin();
    
    // Điều chỉnh tần số (tính bằng MHz, ví dụ: 99.5)
    void setFrequency(float freq_mhz);

    // Tự động dò đài (Search/Seek)
    void seekUp();
    void seekDown();

    // Bật/Tắt chế độ tiếng (Stereo/Mono)
    void setStereo(bool enable);

    // Tắt/Mở chip (Power Management)
    void powerOff();
    void powerOn();

    // Trả về trạng thái hiện tại (dùng cho WebServer)
    void getStatus(JsonDocument* doc);

    // Lấy tần số hiện tại
    float getCurrentFrequency() const { return currentFreq; }

private:
    float currentFreq;      // Tần số hiện tại (ví dụ: 99.5)
    bool isPowered;         // Trạng thái nguồn
    int rssi;               // Cường độ tín hiệu

    // Các hàm I2C cấp thấp
    void writeRegister(uint8_t reg, uint16_t value);
    uint16_t readRegister(uint8_t reg);
    void updateRegisters(); // Hàm gửi lệnh chung tới chip
};

#endif // FMRADIO_H