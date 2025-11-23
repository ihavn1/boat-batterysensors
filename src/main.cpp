#include <memory>
#include "onewire_helper.h"
// Boilerplate #includes:
#include "sensesp_app_builder.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/ui/config_item.h"

// Sensor-specific #includes:
#include "INA226.h"
#include "sensesp_onewire/onewire_temperature.h"

using namespace sensesp;

// Board & project constants
// - Use GPIO numbers (ESP32): refer to pins as `ONEWIRE_PIN`, `RPM_PIN`.
// - Use named constants to avoid magic numbers scattered through the code.
static constexpr uint8_t ONEWIRE_PIN = 25;
static constexpr unsigned int TEMPERATURE_READ_DELAY_MS = 2000;
static constexpr unsigned int BATTERY_READ_INTERVAL_MS = 1000;

// Globals
static INA226 HouseBatteryINA(0x40);
static INA226 StarterBatteryINA(0x41);

// Helper moved to separate files
#include "battery_helper.h"

void setup()
{
    // put your setup code here, to run once:
    SetupLogging();

    // Create the global SensESPApp() object
    SensESPAppBuilder builder;
    sensesp_app = builder
                      .set_hostname("battery-sensors")
                      ->get_app();

    Wire.begin();

    // Read the sensor every BATTERY_READ_INTERVAL_MS
    const unsigned int read_interval = BATTERY_READ_INTERVAL_MS;

    // -------------- House Battery Voltage and current -----------------------
    setupBatteryINA(HouseBatteryINA, read_interval, 0.0075F, 0.250F, "electrical.batteries.house.voltage",
                    "electrical.batteries.house.current", "electrical.batteries.house.power", "HouseBatteryINA");

    // -------------- Starter Battery Voltage and current -----------------------
    setupBatteryINA(StarterBatteryINA, read_interval, 0.0075F, 0.250F, "electrical.batteries.starter.voltage",
                    "electrical.batteries.starter.current", "electrical.batteries.starter.power", "StarterBatteryINA");

    // ############ Battery temperature sensors ##########
    constexpr uint8_t pin = ONEWIRE_PIN;
    sensesp::onewire::DallasTemperatureSensors *dts = new sensesp::onewire::DallasTemperatureSensors(pin);

    // Read the sensor every TEMPERATURE_READ_DELAY_MS
    const unsigned int temperature_read_delay = TEMPERATURE_READ_DELAY_MS;

    // Below are temperatures sampled and sent to Signal K server
    // To find valid Signal K Paths that fits your need you look at this link:
    // https://signalk.org/specification/1.4.0/doc/vesselsBranch.html

    // Measure house battery temperature
    add_onewire_temp(dts, temperature_read_delay, "houseBatteryTemperature",
                     "electrical.batteries.house.temperature",
                     "House Battery Temperature", 110, 120, 130);

    // Measure starter battery temperature
    add_onewire_temp(dts, temperature_read_delay, "starterBatteryTemperature",
                     "electrical.batteries.starter.temperature",
                     "Starter Battery Temperature", 210, 220, 230);
}

void loop()
{
    static auto event_loop = sensesp_app->get_event_loop();
    event_loop->tick();
}
