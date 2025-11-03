// Boilerplate #includes:
#include "sensesp_app_builder.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/transforms/linear.h"
//#include "sensesp/ui/config_item.h"

// Sensor-specific #includes:
#include "INA226.h"

using namespace sensesp;

// Globals
INA226 INA0(0x40);

float foo1() { return 1.0;}
float foo2() { return 2.0;}
// put function declarations here:
float read_busVoltage_callback() {return (foo1()); }
float read_current_callback() { return (foo2()); }

void setup()
{
  // put your setup code here, to run once:
  SetupLogging();  

  // Create the global SensESPApp() object
  SensESPAppBuilder builder;
  sensesp_app = builder
  .set_hostname("battery-hub")
  ->get_app();

  Wire.begin();

  // Read the sensor every 2 seconds
  unsigned int read_interval = 2000;

  /*
  if (!INA0.begin())
  {
    Serial.println("Failed to find INA226_0 Chip!");
    while (1)
    {
      delay(10);
    }
  }
 
  INA0.setMaxCurrentShunt(10.0, 0.1);
  INA0.configure();
     */

  // Create a RepeatSensor with float output that reads the current of INA0
  // using the function defined above.
  auto *battery_start_busVoltage =
      new RepeatSensor<float>(read_interval, read_busVoltage_callback);

  // Set the Signal K Path for the output
  const char *sk_path = "electrical.batteries.houseBattery.voltage";

  // Send the busVolate to the Signal K server as a Float
  battery_start_busVoltage->connect_to(
      new SKOutputFloat(sk_path, "", new SKMetadata("V", "Voltage")));
}

void loop()
{
  event_loop()->tick();
}

// put function definitions here:
