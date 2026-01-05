#include "BluetoothManager.h"
#include <esp_gap_bt_api.h>
#include "esp_bt.h"

// Khởi tạo static member
MusicMetadata BluetoothManager::_meta;

BluetoothManager::BluetoothManager(FileManager *fileMgr) : fileManager(fileMgr) {}

void BluetoothManager::begin()
{
    i2s_pin_config_t my_pin_config = {
        .bck_io_num = I2S_BCK_PIN,    // 26
        .ws_io_num = I2S_WS_PIN,      // 25
        .data_out_num = I2S_DOUT_PIN, // 27 (An toàn cho I2C)
        .data_in_num = I2S_PIN_NO_CHANGE};
    a2dp_sink.set_pin_config(my_pin_config);
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(esp_bt_io_cap_t));
    a2dp_sink.set_avrc_metadata_callback(metadataCallback);
    
    loadConfig();
    a2dp_sink.activate_pin_code(false);
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    a2dp_sink.start("ESP32_Famio_Audio");
    a2dp_sink.set_volume(_currentVolume);
}

void BluetoothManager::setPower(bool enable)
{
    if (enable && !_isPowered)
    {
        begin();
        _isPowered = true;
    }
    else if (!enable && _isPowered)
    {
        a2dp_sink.end();
        _isPowered = false;
        _meta.reset();
    }
}

void BluetoothManager::setVolume(uint8_t volume)
{
    _currentVolume = volume;
    if (_isPowered)
    {
        a2dp_sink.set_volume(_currentVolume);
    }
    saveConfig();
}

void BluetoothManager::confirmPinCode(long pinCode)
{
    Serial.println(a2dp_sink.pin_code());
    if (_isPowered & a2dp_sink.pin_code() != 0)
    {
        a2dp_sink.confirm_pin_code(pinCode);
    }
}

void BluetoothManager::play()
{
    if (_isPowered)
        a2dp_sink.play();
}
void BluetoothManager::pause()
{
    if (_isPowered)
        a2dp_sink.pause();
}
void BluetoothManager::next()
{
    if (_isPowered)
        a2dp_sink.next();
}
void BluetoothManager::previous()
{
    if (_isPowered)
        a2dp_sink.previous();
}

void BluetoothManager::metadataCallback(uint8_t id, const uint8_t *text)
{
    String rawText = (char *)text;

    // Giới hạn độ dài chuỗi để bảo vệ Heap
    if (rawText.length() > 64)
    {
        rawText = rawText.substring(0, 61) + "...";
    }

    if (id == 0x1)
        _meta.title = rawText;
    if (id == 0x2)
        _meta.artist = rawText;
    if (id == 0x4)
        _meta.album = rawText;
}

void BluetoothManager::getStatus(JsonDocument &doc)
{
    doc["enabled"] = _isPowered;
    doc["connected"] = a2dp_sink.is_connected();
    doc["volume"] = _currentVolume;
    // Serial.printf("Heap: %s\n", ESP.getFreeHeap());

    // Nếu đang kết nối thì mới gửi tên bài hát, không thì gửi "Chưa kết nối"
    if (a2dp_sink.is_connected())
    {
        doc["title"] = _meta.title.length() > 0 ? _meta.title : "Unknown Title";
        doc["artist"] = _meta.artist.length() > 0 ? _meta.artist : "Unknown Artist";
        doc["album"] = _meta.album.length() > 0 ? _meta.album : "";
    }
    else
    {
        doc["title"] = "Ready to Pair";
        doc["artist"] = "Waiting for connection...";
    }
}

void BluetoothManager::loadConfig()
{
    JsonDocument doc;
    if (fileManager->loadJsonFile(CONFIG_FILE_PATH BT_CONFIG_FILE, &doc))
    {
        _currentVolume = doc["volume"] | 64;
    }
}

void BluetoothManager::saveConfig()
{
    JsonDocument doc;
    doc["volume"] = _currentVolume;
    fileManager->saveJsonFile(CONFIG_FILE_PATH BT_CONFIG_FILE, doc);
}