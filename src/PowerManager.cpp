#include "PowerManager.h"

// Constructor
PowerManager::PowerManager()
{
}

// =========================================================
// Khởi tạo (Setup Pins)
// =========================================================

void PowerManager::begin()
{
    // Thiết lập Pin ADC để đọc điện áp Pin
    // pinMode(BATTERY_ADC_PIN, INPUT);

    Serial.println("PowerManager: Khởi tạo hoàn tất cho pin 3S.");
}

// =========================================================
// 1. Quản lý Pin (Battery Management)
// =========================================================

float PowerManager::mapAdcToVoltage(int raw_adc)
{
    const float VREF = 3.3f;
    const float ADC_MAX = 4095.0f;

    // Giả định tỷ lệ mạch chia áp 4:1 cho khối pin 12.6V
    const float VOLTAGE_MULTIPLIER = 4.0f;

    return (float)raw_adc / ADC_MAX * VREF * VOLTAGE_MULTIPLIER;
}

float PowerManager::getBatteryVoltage()
{
    // int raw_adc = analogRead(BATTERY_ADC_PIN);
    // GIẢ LẬP: Giá trị ADC tương ứng với dải 9V - 12.6V
    int raw_adc = random(2792, 3915);
    return mapAdcToVoltage(raw_adc);
}

int PowerManager::getBatteryLevel()
{
    float voltage = getBatteryVoltage();

    if (voltage >= VOLTAGE_MAX)
        return 100;
    if (voltage <= VOLTAGE_MIN)
        return 0;

    int percentage = (int)((voltage - VOLTAGE_MIN) / (VOLTAGE_MAX - VOLTAGE_MIN) * 100.0f);

    return percentage;
}

// =========================================================
// 4. Quản lý Nguồn (Power Management)
// =========================================================

void PowerManager::shutdown()
{
    Serial.println("PowerManager: Đang chuyển sang chế độ Deep Sleep/Tắt nguồn...");
    // esp_deep_sleep_start();
    delay(100);
    Serial.println("Hệ thống đã ngừng.");
}