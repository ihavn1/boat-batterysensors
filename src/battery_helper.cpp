#include <Arduino.h>
#include "battery_helper.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/sensors/sensor.h"

using namespace sensesp;

void setupBatteryINA(INA226& ina, unsigned int read_interval, const char* voltage_path,
                     const char* current_path, const char* chip_name) {
    if (!ina.begin()) {
        Serial.println(String("Failed to find ") + chip_name + " Chip!");
        while (1) {
            delay(10);
        }
    }

    ina.configure(0.0075);

    auto* voltage_sensor = new RepeatSensor<float>(read_interval, [&ina]() { return ina.getBusVoltage(); });
    voltage_sensor->connect_to(
        new SKOutputFloat(voltage_path, "", new SKMetadata("V", "Voltage")));

    auto* current_sensor = new RepeatSensor<float>(read_interval, [&ina]() { return ina.getCurrent(); });
    current_sensor->connect_to(
        new SKOutputFloat(current_path, "", new SKMetadata("A", "Amps")));
}
