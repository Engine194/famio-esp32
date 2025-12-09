#include "FMRadio.h"

// =========================================================
// Các địa chỉ Register của RDA5807M (Tham khảo Datasheet)
// =========================================================
#define REG_CHIP_ID 0x02
#define REG_CTRL_1 0x03   // Control Register 1 (Power Up, Mute, Band, Tune)
#define REG_FREQ_SET 0x0A // Frequency Setting Register
#define REG_STATUS_1 0x0B // Status Register 1 (Read Frequency, RSSI, Stereo/Mono)

// Constructor
FMRadio::FMRadio() : currentFreq(99.5f), isPowered(false), rssi(0)
{
    // Khởi tạo các giá trị mặc định
}

// =========================================================
// Khởi tạo I2C và Chip
// =========================================================

void FMRadio::begin()
{
    // 1. Khởi tạo giao tiếp I2C
    Wire.begin();
    Serial.println("FMRadio: Khởi tạo I2C hoàn tất.");

    // 2. Bật chip
    powerOn();

    // 3. Thiết lập băng tần FM World (87.0-108.0 MHz) và kích thước bước nhảy (100kHz)
    // Giả định ghi Register 0x03 (Control Register 1)
    // Bit 0: DHIZ=1 (High Impedance mode for Audio)
    // Bit 1: DMUTE=0 (Unmute)
    // Bit 4: BAND=00 (FM World Band 87-108 MHz)
    // Bit 5: TUNE=1 (Bắt đầu điều chỉnh)
    // Bit 6: RCLK_NON_CAL=0
    // Bit 7: RCLK_MODE=0
    // Bit 8: STEREO=1 (Stereo mode)
    // Bit 14: POWER=1 (Power up)

    // Giá trị khởi tạo giả định: Mute=0, Power=1, Band=World
    uint16_t init_val = 0x0000 | (1 << 14) | (0 << 13) | (0 << 12) | (0 << 11);
    writeRegister(REG_CTRL_1, init_val);

    // Đợi 0.5 giây để chip khởi động
    delay(500);

    // 4. Thiết lập tần số mặc định
    setFrequency(currentFreq);

    Serial.println("FMRadio: Chip RDA5807M đã khởi động.");
}

// =========================================================
// Ghi/Đọc Register I2C cấp thấp
// =========================================================

void FMRadio::writeRegister(uint8_t reg, uint16_t value)
{
    Wire.beginTransmission(RDA5807M_ADDR);
    Wire.write(reg);                     // Địa chỉ Register
    Wire.write((uint8_t)(value >> 8));   // Byte cao (MSB)
    Wire.write((uint8_t)(value & 0xFF)); // Byte thấp (LSB)
    Wire.endTransmission();
}

uint16_t FMRadio::readRegister(uint8_t reg)
{
    // Yêu cầu chip gửi 2 bytes (16-bit) từ địa chỉ reg
    Wire.requestFrom(RDA5807M_ADDR, 2);
    if (Wire.available() == 2)
    {
        uint16_t msb = Wire.read();
        uint16_t lsb = Wire.read();
        return (msb << 8) | lsb;
    }
    return 0; // Trả về 0 nếu đọc thất bại
}

// =========================================================
// Điều chỉnh Tần số
// =========================================================

void FMRadio::setFrequency(float freq_mhz)
{
    // Chuyển đổi MHz sang Channel ID (CH)
    // Công thức cho FM World Band (87.0 MHz - 108.0 MHz, bước nhảy 100kHz)
    // CH = (Tần số - 87.0) / 0.1
    // VD: 99.5 MHz -> (99.5 - 87.0) / 0.1 = 125
    int channel = (int)((freq_mhz * 10 - 870) / 10);

    // Lấy giá trị Register hiện tại (REG_CTRL_1)
    uint16_t ctrl_reg = readRegister(REG_CTRL_1);

    // Thiết lập Channel ID vào các bit 9:0 của Register 0x05 (Giả định)
    uint16_t freq_val = (ctrl_reg & 0xFC00) | (channel & 0x03FF);

    // Set TUNE bit (Bit 15) để bắt đầu dò/điều chỉnh
    freq_val |= (1 << 15);

    // Ghi giá trị mới vào Register
    writeRegister(REG_FREQ_SET, freq_val); // Giả định dùng REG_FREQ_SET (0x0A)

    // Đợi cho quá trình dò hoàn tất (TUNE bit sẽ tự động reset)
    delay(50);

    currentFreq = freq_mhz;
    Serial.printf("FMRadio: Đặt tần số thành %.1f MHz (Channel: %d)\n", freq_mhz, channel);
}

// =========================================================
// Dò đài (Seek)
// =========================================================

void FMRadio::seekUp()
{
    // Logic: Ghi Register 0x03, set bit SEEKUP=1, SE=1, đợi quá trình dò hoàn tất
    // Sau khi dò xong, đọc lại tần số từ REG_STATUS_1

    Serial.println("FMRadio: Đang dò đài lên...");
    // ... (Thực hiện lệnh I2C Seek Up) ...
    // Sau khi dò xong, cập nhật currentFreq
    // currentFreq = readFrequencyFromChip(); // Giả định hàm đọc từ chip
}

void FMRadio::seekDown()
{
    Serial.println("FMRadio: Đang dò đài xuống...");
    // ... (Thực hiện lệnh I2C Seek Down) ...
}

// =========================================================
// Trạng thái (Status)
// =========================================================

void FMRadio::getStatus(JsonDocument *doc)
{
    if (!isPowered)
    {
        (*doc)["error"] = "Chip is powered off.";
        return;
    }

    // Đọc trạng thái chip (REG_STATUS_1 - 0x0B)
    // uint16_t status_reg = readRegister(REG_STATUS_1);

    // Cập nhật giá trị RSSI và Stereo (Giả lập)
    rssi = random(10, 80);            // Cường độ tín hiệu
    bool stereo_status = (rssi > 50); // Giả định

    // Điền dữ liệu vào JsonDocument
    (*doc)["freq"] = currentFreq;
    (*doc)["signal"] = rssi;
    (*doc)["stereo"] = stereo_status;
    (*doc)["powered"] = isPowered;
}

// =========================================================
// Quản lý Nguồn
// =========================================================

void FMRadio::powerOff()
{
    // Ghi Register 0x03, set bit POWER=0 (Bit 14)
    writeRegister(REG_CTRL_1, 0x0000);
    isPowered = false;
    Serial.println("FMRadio: Tắt nguồn.");
}

void FMRadio::powerOn()
{
    // Ghi Register 0x03, set bit POWER=1 (Bit 14)
    uint16_t init_val = 0x0000 | (1 << 14);
    writeRegister(REG_CTRL_1, init_val);
    isPowered = true;
    Serial.println("FMRadio: Mở nguồn.");
}