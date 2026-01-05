#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <Arduino.h>
#include "Constants.h" 

// Định nghĩa các ngưỡng Pin Lithium 3S (3 pin mắc nối tiếp)
#define VOLTAGE_MAX 12.6f   // Điện áp sạc đầy (3 x 4.2V)
#define VOLTAGE_MIN 9.0f    // Điện áp tối thiểu an toàn (3 x 3.0V)

class PowerManager {
public:
    PowerManager(); 

    void begin();

    // 1. Quản lý Pin
    float getBatteryVoltage();
    int getBatteryLevel(); // Trả về phần trăm pin (0-100)

    // 4. Quản lý Nguồn
    void shutdown();

private:
    float mapAdcToVoltage(int raw_adc);
};

#endif // POWERMANAGER_H