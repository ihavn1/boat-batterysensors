#include <Arduino.h>
#include "battery_helper.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/sensors/sensor.h"

using namespace sensesp;

void setupBatteryINA(INA226& ina, unsigned int read_interval, float shunt_resistance, float current_LSB_mA, const char* voltage_path,
                     const char* current_path, const char* power_path, const char* chip_name) {
    if (!ina.begin()) {
        Serial.println(String("Failed to find ") + chip_name + " Chip!");
        while (1) {
            delay(10);
        }
    }


    ina.configure(shunt_resistance, current_LSB_mA);
    ina.setAverage(INA226_16_SAMPLES);

    auto* voltage_sensor = new RepeatSensor<float>(read_interval, [&ina]() { return ina.getBusVoltage(); });
    voltage_sensor->connect_to(
        new SKOutputFloat(voltage_path, "", new SKMetadata("V", "Voltage")));

    auto* current_sensor = new RepeatSensor<float>(read_interval, [&ina]() { return ina.getCurrent(); });
    current_sensor->connect_to(
        new SKOutputFloat(current_path, "", new SKMetadata("A", "Amps")));

    auto* power_sensor = new RepeatSensor<float>(read_interval, [&ina]() { return ina.getPower(); });
    power_sensor->connect_to(
        new SKOutputFloat(power_path, "", new SKMetadata("W", "Power")));        
}
