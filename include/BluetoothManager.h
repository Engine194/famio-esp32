#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <Arduino.h>
#include "BluetoothA2DPSink.h"
#include "FileManager.h"
#include "Constants.h"

struct MusicMetadata
{
    String title = "";
    String artist = "";
    String album = "";

    void reset()
    {
        title = "";
        artist = "";
        album = "";
    }
};

class BluetoothManager
{
public:
    BluetoothManager(FileManager *fileMgr);

    // Khởi tạo cấu hình chân I2S nhưng CHƯA bật Bluetooth
    void begin();

    // Điều khiển Nguồn (Bật/Tắt Stack)
    void setPower(bool enable);
    bool isPowered() const { return _isPowered; }

    // Điều khiển nhạc
    void play();
    void pause();
    void next();
    void previous();

    // Điều chỉnh âm lượng (0-127 theo thư viện A2DP)
    void setVolume(uint8_t volume);
    uint8_t getVolume() const { return _currentVolume; }

    // Lấy trạng thái tổng hợp cho API
    void getStatus(JsonDocument &doc);

    // Callbacks (Cần để nhận metadata từ điện thoại)
    static void metadataCallback(uint8_t id, const uint8_t *text);
    void confirmPinCode(long pinCode);

private:
    BluetoothA2DPSink a2dp_sink;
    FileManager *fileManager;

    bool _isPowered = false;
    uint8_t _currentVolume = 64; // Mặc định 50%
    static MusicMetadata _meta;

    void loadConfig();
    void saveConfig();
};

#endif