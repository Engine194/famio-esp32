#include "FMRadio.h"

// =========================================================
// Các địa chỉ Register của RDA5807M (Tham khảo Datasheet)
// =========================================================
#define REG_CHIP_ID 0x02
#define REG_CTRL_1 0x03   // Control Register 1 (Power Up, Mute, Band, Tune)
#define REG_FREQ_SET 0x0A // Frequency Setting Register
#define REG_STATUS_1 0x0B // Status Register 1 (Read Frequency, RSSI, Stereo/Mono)
#define REG_CTRL_2 0x04

FMRadio::FMRadio(FileManager *fm)
    : fileManager(fm), currentFreq(99.5f), isPowered(false), rssi(0), currentVolume(10), numSavedChannels(0)
{
    // Khởi tạo các giá trị mặc định
}

// =========================================================
// Khởi tạo I2C và Chip
// =========================================================

void FMRadio::begin()
{
    // 1. Load cấu hình từ SD Card
    loadConfig();

    // 2. Khởi tạo giao tiếp I2C
    Wire.begin();
    Serial.println("FMRadio: Khởi tạo I2C hoàn tất.");

    // 3. Bật chip
    powerOn();

    // 4. Thiết lập Control Register 1 (Power, Band)
    uint16_t init_val = (1 << 14) | (0 << 11);
    writeRegister(REG_CTRL_1, init_val);

    // 5. Thiết lập âm lượng (Control Register 2)
    applyVolume();

    // Đợi 0.5 giây để chip khởi động
    delay(500);

    // 6. Thiết lập tần số đã load
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

// API Dò đài tự động (autoSeekNext)
// Thực hiện dò đài lên (seekUp) và trả về tần số mới.
float FMRadio::autoSeekNext()
{
    // Gọi hàm dò đài lên
    seekUp();

    // Tần số hiện tại (currentFreq) đã được cập nhật bên trong seekUp()
    Serial.printf("FMRadio: Đã hoàn tất dò đài tự động. Tần số mới: %.1f MHz\n", currentFreq);

    return currentFreq;
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
    (*doc)["isPowered"] = isPowered;
}

// =========================================================
// Quản lý Nguồn
// =========================================================

void FMRadio::powerOff()
{
    // Ghi Register 0x03, set bit POWER=0 (Bit 14)
    writeRegister(REG_CTRL_1, 0x0000);
    delay(500);
    isPowered = false;
    Serial.println("FMRadio: Tắt nguồn.");
}

void FMRadio::powerOn()
{
    // Ghi Register 0x03, set bit POWER=1 (Bit 14)
    uint16_t init_val = 0x0000 | (1 << 14);
    writeRegister(REG_CTRL_1, init_val);
    delay(500);
    isPowered = true;
    Serial.println("FMRadio: Mở nguồn.");
}

void FMRadio::loadConfig()
{
    JsonDocument doc;

    // Đọc file fm.json từ SD Card
    if (fileManager->loadJsonFile(FM_CONFIG_FILE, &doc))
    {
        // 1. Load Âm lượng
        currentVolume = doc["volume"] | 10; // Mặc định là 10 nếu không tìm thấy
        if (currentVolume > 15)
            currentVolume = 15;

        // 2. Load Tần số hiện tại
        currentFreq = doc["current_freq"] | 99.5f;

        // 3. Load Danh sách Kênh
        JsonArray channels = doc["channels"].as<JsonArray>();
        numSavedChannels = 0;

        for (JsonObject channel : channels)
        {
            if (numSavedChannels < MAX_CHANNELS)
            {
                float freq = channel["freq"] | 0.0f;
                if (freq >= 87.0f && freq <= 108.0f)
                {
                    savedChannels[numSavedChannels] = freq;
                    numSavedChannels++;
                }
            }
        }
        Serial.printf("FMRadio: Load cấu hình SD thành công. Vol: %d, Kênh: %d\n", currentVolume, numSavedChannels);
    }
    else
    {
        // Khởi tạo cấu hình mặc định nếu load thất bại
        Serial.println("FMRadio: Load config fm.json thất bại. Sử dụng giá trị mặc định.");
        currentVolume = 10;
        currentFreq = 99.5f;
        numSavedChannels = 0;
        saveConfig(); // Lưu cấu hình mặc định mới
    }
}

void FMRadio::saveConfig()
{
    JsonDocument doc;

    // 1. Lưu Âm lượng
    doc["volume"] = currentVolume;

    // 2. Lưu Tần số hiện tại
    doc["current_freq"] = currentFreq;

    // 3. Lưu Danh sách Kênh
    JsonArray channels = doc["channels"].as<JsonArray>();
    for (int i = 0; i < numSavedChannels; i++)
    {
        JsonObject channel = channels.add<JsonObject>();
        // Chỉ lưu tần số (vì index được xác định bằng vị trí trong mảng)
        channel["freq"] = savedChannels[i];
    }

    // Ghi file fm.json vào SD Card
    if (fileManager->saveJsonFile(FM_CONFIG_FILE, doc))
    {
        Serial.println("FMRadio: Lưu cấu hình fm.json thành công.");
    }
    else
    {
        Serial.println("FMRadio: Lỗi khi lưu cấu hình fm.json.");
    }
}

void FMRadio::applyVolume()
{
    // ... (Giữ nguyên logic I2C từ phần trước) ...
    uint16_t ctrl2_reg = readRegister(REG_CTRL_2);
    ctrl2_reg &= 0xFFF0;
    ctrl2_reg |= (currentVolume & 0x0F);
    writeRegister(REG_CTRL_2, ctrl2_reg);
}

void FMRadio::setVolume(uint8_t volume)
{
    if (volume > 15)
        volume = 15;
    if (volume == currentVolume)
        return;

    currentVolume = volume;
    applyVolume();
    saveConfig(); // Lưu lại cấu hình sau khi thay đổi âm lượng
    Serial.printf("FMRadio: Đặt âm lượng thành %d\n", currentVolume);
}

// =========================================================
// Quản lý Kênh đã lưu
// =========================================================

// API 3: Lưu kênh hiện tại
void FMRadio::saveChannel(float freq_mhz)
{
    if (numSavedChannels >= MAX_CHANNELS)
    {
        Serial.println("FMRadio: Đã đạt giới hạn số kênh lưu trữ.");
        return;
    }

    // Lưu tần số vào mảng RAM
    savedChannels[numSavedChannels] = freq_mhz;
    numSavedChannels++;

    Serial.printf("FMRadio: Đã lưu kênh %.1f MHz vào vị trí %d\n", freq_mhz, numSavedChannels - 1);
    saveConfig(); // Lưu toàn bộ cấu hình vào SD Card
}

// API 4: Chọn kênh đã lưu
void FMRadio::selectSavedChannel(uint8_t index)
{
    if (index >= numSavedChannels)
    {
        Serial.println("FMRadio: Index kênh không hợp lệ.");
        return;
    }

    float savedFreq = savedChannels[index];

    Serial.printf("FMRadio: Chọn kênh đã lưu tại index %d: %.1f MHz\n", index, savedFreq);
    setFrequency(savedFreq);
    saveConfig(); // Lưu lại tần số hiện tại
}

// API 5: Lấy danh sách kênh đã lưu
void FMRadio::getSavedChannels(JsonDocument *doc)
{
    JsonArray channels = (*doc)["channels"].as<JsonArray>();

    for (int i = 0; i < numSavedChannels; i++)
    {
        JsonObject channel = channels.add<JsonObject>();
        channel["index"] = i;
        channel["freq"] = savedChannels[i];
    }
}

// Hàm bổ sung: Xóa kênh đã lưu
void FMRadio::deleteChannel(uint8_t index)
{
    if (index >= numSavedChannels)
    {
        Serial.println("FMRadio: Index kênh không hợp lệ để xóa.");
        return;
    }

    // Dịch chuyển các phần tử sau index lên trước
    for (int i = index; i < numSavedChannels - 1; i++)
    {
        savedChannels[i] = savedChannels[i + 1];
    }

    numSavedChannels--;
    Serial.printf("FMRadio: Đã xóa kênh tại index %d. Số kênh còn lại: %d\n", index, numSavedChannels);
    saveConfig(); // Lưu lại cấu hình sau khi xóa
}