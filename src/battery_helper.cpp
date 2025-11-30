#include <Arduino.h>
#include "battery_helper.h"
#include "ah_integrator.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/signalk/signalk_put_request_listener.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/transforms/linear.h"

using namespace sensesp;

void setupBatteryINA(INA226& ina, unsigned int read_interval, float shunt_resistance, float current_LSB_mA, const char* voltage_path,
                     const char* current_path, const char* power_path, const char* ah_path, 
                     const char* soc_path, float battery_capacity_ah, float initial_ah, const char* chip_name) {
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

    // Amp-hour integrator: integrate current over time at 100 Hz to produce Ah
    // Pass battery capacity so Ah is clamped between 0 and capacity
    // Initial Ah is set to initial_ah (typically full capacity at startup)
    auto* ah_integ = new AmpHourIntegrator("", initial_ah, battery_capacity_ah);
    current_sensor->connect_to(ah_integ);
    
    // Sample Ah from integrator at 1 Hz for Signal K output (decoupled from 100 Hz integration)
    auto* ah_sk_sampler = new RepeatSensor<float>(1000, [ah_integ]() { return ah_integ->get_ah(); });
    ah_sk_sampler->connect_to(new SKOutputFloat(ah_path, "", new SKMetadata("Ah", "Ampere hours")));
    
    // Convert Ah to State of Charge percentage (0-100%)
    // SOC% = (Ah / Capacity) * 100, clamped to 0-100
    auto* soc_percent = new Linear(100.0f / battery_capacity_ah, 0.0f);
    ah_sk_sampler->connect_to(soc_percent);
    soc_percent->connect_to(new SKOutputFloat(soc_path, "", new SKMetadata("ratio", "State of Charge")));
    
    // Signal K input to allow remote reset/calibration of Ah value
    auto* ah_sk_input = new SKPutRequestListener<float>(ah_path);
    ah_sk_input->connect_to(ah_integ);  // Connect to integrator's set() method
    
    // Signal K inputs for charge/discharge efficiency configuration
    String charge_eff_path = String(ah_path) + "/chargeEfficiency";
    String discharge_eff_path = String(ah_path) + "/dischargeEfficiency";
    
    // Create simple consumers that call the efficiency setters
    class ChargeEfficiencyConsumer : public ValueConsumer<float> {
     public:
      ChargeEfficiencyConsumer(AmpHourIntegrator* integ) : integ_(integ) {}
      void set(const float& new_value) override { integ_->set_charge_efficiency(new_value); }
     private:
      AmpHourIntegrator* integ_;
    };
    
    class DischargeEfficiencyConsumer : public ValueConsumer<float> {
     public:
      DischargeEfficiencyConsumer(AmpHourIntegrator* integ) : integ_(integ) {}
      void set(const float& new_value) override { integ_->set_discharge_efficiency(new_value); }
     private:
      AmpHourIntegrator* integ_;
    };
    
    auto* charge_eff_input = new SKPutRequestListener<float>(charge_eff_path);
    charge_eff_input->connect_to(new ChargeEfficiencyConsumer(ah_integ));
    
    auto* discharge_eff_input = new SKPutRequestListener<float>(discharge_eff_path);
    discharge_eff_input->connect_to(new DischargeEfficiencyConsumer(ah_integ));

    auto* power_sensor = new RepeatSensor<float>(read_interval, [&ina]() { return ina.getPower(); });
    power_sensor->connect_to(
        new SKOutputFloat(power_path, "", new SKMetadata("W", "Power")));        
}
