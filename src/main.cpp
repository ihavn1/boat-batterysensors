// Boilerplate #includes:
#include "sensesp_app_builder.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/ui/config_item.h"

// Sensor-specific #includes:
#include "INA226.h"
#include "sensesp_onewire/onewire_temperature.h"

using namespace sensesp;
using namespace sensesp::onewire;

// Globals
INA226 HouseBatteryINA(0x40);
INA226 StarterBatteryINA(0x41);

// put function declarations here:
float read_houseBattery_busVoltage_callback() { return HouseBatteryINA.getBusVoltage(); }
float read_houseBattery_current_callback() { return HouseBatteryINA.getCurrent(); }

float read_starterBattery_busVoltage_callback() { return StarterBatteryINA.getBusVoltage(); }
float read_starterBattery_current_callback() { return StarterBatteryINA.getCurrent(); }

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

    // Read the sensor every 2 seconds
    unsigned int read_interval = 2000;

    // -------------- House Battery Voltage and current -----------------------

    if (!HouseBatteryINA.begin())
    {
        Serial.println("Failed to find HouseBatteryINA Chip!");
        while (1)
        {
            delay(10);
        }
    }

    HouseBatteryINA.configure(0.0075);

    // Create a RepeatSensor with float output that reads the current of HouseBatteryINA
    // using the function defined above.
    auto *battery_houseBattery_busVoltage =
        new RepeatSensor<float>(read_interval, read_houseBattery_busVoltage_callback);

    // Set the Signal K Path for the output
    const char *sk_path_houseBattery_busVoltage = "electrical.batteries.house.voltage";

    // Send the busVolate to the Signal K server as a Float
    battery_houseBattery_busVoltage->connect_to(
        new SKOutputFloat(sk_path_houseBattery_busVoltage, "", new SKMetadata("V", "Voltage")));

    // Create a RepeatSensor with float output that reads the cuttent of INA0
    // using the function defined above.
    auto *battery_houseBattery_current =
        new RepeatSensor<float>(read_interval, read_houseBattery_current_callback);

    // Set the Signal K Path for the output
    const char *sk_path_house_current = "electrical.batteries.house.current";

    // Send the busVolate to the Signal K server as a Float
    battery_houseBattery_current->connect_to(
        new SKOutputFloat(sk_path_house_current, "", new SKMetadata("A", "Amps")));

    // -------------- starter Battery Voltage and current -----------------------

    if (!StarterBatteryINA.begin())
    {
        Serial.println("Failed to find starterBatteryINA Chip!");
        while (1)
        {
            delay(10);
        }
    }

    StarterBatteryINA.configure(0.0075);

    // Create a RepeatSensor with float output that reads the current of starterBatteryINA
    // using the function defined above.
    auto *battery_starterBattery_busVoltage =
        new RepeatSensor<float>(read_interval, read_starterBattery_busVoltage_callback);

    // Set the Signal K Path for the output
    const char *sk_path_starterBattery_busVoltage = "electrical.batteries.starter.voltage";

    // Send the busVolate to the Signal K server as a Float
    battery_starterBattery_busVoltage->connect_to(
        new SKOutputFloat(sk_path_starterBattery_busVoltage, "", new SKMetadata("V", "Voltage")));

    // Create a RepeatSensor with float output that reads the cuttent of INA0
    // using the function defined above.
    auto *battery_starterBattery_current =
        new RepeatSensor<float>(read_interval, read_starterBattery_current_callback);

    // Set the Signal K Path for the output
    const char *sk_path_starter_current = "electrical.batteries.starter.current";

    // Send the busVolate to the Signal K server as a Float
    battery_starterBattery_current->connect_to(
        new SKOutputFloat(sk_path_starter_current, "", new SKMetadata("A", "Amps")));

    // ############ Battery temperature sensors ##########
    uint8_t pin = 25;
    DallasTemperatureSensors *dts = new DallasTemperatureSensors(pin);

    // Define how often SensESP should read the sensor(s) in milliseconds
    uint read_delay = 500;

    // Below are temperatures sampled and sent to Signal K server
    // To find valid Signal K Paths that fits your need you look at this link:
    // https://signalk.org/specification/1.4.0/doc/vesselsBranch.html

    // Measure house battery temperature
    auto houseBattery_temp =
        new OneWireTemperature(dts, read_delay, "/houseBatteryTemperature/oneWire");

    ConfigItem(houseBattery_temp)
        ->set_title("House Battery Temperature")
        ->set_description("Temperature of the House Battery")
        ->set_sort_order(110);

    auto houseBattery_temp_calibration =
        new Linear(1.0, 0.0, "/houseBatteryTemperature/linear");

    ConfigItem(houseBattery_temp_calibration)
        ->set_title("House Battery Temperature Calibration")
        ->set_description("Calibration for the House Battery temperature sensor")
        ->set_sort_order(120);

    auto houseBattery_temp_sk_output = new SKOutputFloat(
        "electrical.batteries.house.temperature", "/houseBatteryTemperature/skPath");

    ConfigItem(houseBattery_temp_sk_output)
        ->set_title("House Battery Temperature Signal K Path")
        ->set_description("Signal K path for House Battery temperature")
        ->set_sort_order(130);

    houseBattery_temp->connect_to(houseBattery_temp_calibration)
        ->connect_to(houseBattery_temp_sk_output);

    // Measure starter battery temperature
    auto starterBattery_temp =
        new OneWireTemperature(dts, read_delay, "/starterBatteryTemperature/oneWire");

    ConfigItem(starterBattery_temp)
        ->set_title("starter Battery Temperature")
        ->set_description("Temperature of the starter Battery")
        ->set_sort_order(210);

    auto starterBattery_temp_calibration =
        new Linear(1.0, 0.0, "/starterBatteryTemperature/linear");

    ConfigItem(starterBattery_temp_calibration)
        ->set_title("starter Battery Temperature Calibration")
        ->set_description("Calibration for the starter Battery temperature sensor")
        ->set_sort_order(220);

    auto starterBattery_temp_sk_output = new SKOutputFloat(
        "electrical.batteries.starter.temperature", "/starterBatteryTemperature/skPath");

    ConfigItem(starterBattery_temp_sk_output)
        ->set_title("starter Battery Temperature Signal K Path")
        ->set_description("Signal K path for starter Battery temperature")
        ->set_sort_order(230);

    starterBattery_temp->connect_to(starterBattery_temp_calibration)
        ->connect_to(starterBattery_temp_sk_output);
}

void loop()
{
    static auto event_loop = sensesp_app->get_event_loop();
    event_loop->tick();
}
