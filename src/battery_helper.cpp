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
      while (1) {
        delay(10);
      }
    }


    ina.configure(shunt_resistance, current_LSB_mA);
    ina.setAverage(INA226_256_SAMPLES);

    auto* voltage_sensor = new RepeatSensor<float>(read_interval, [&ina]() { return ina.getBusVoltage(); });
    voltage_sensor->connect_to(
        new SKOutputFloat(voltage_path, "", new SKMetadata("V", "Voltage")));

    auto* current_sensor = new RepeatSensor<float>(read_interval, [&ina]() { return ina.getCurrent(); });
    current_sensor->connect_to(
        new SKOutputFloat(current_path, "", new SKMetadata("A", "Amps")));

    // Amp-hour integrator: integrate current over time at 100 Hz to produce Ah
    // Pass battery capacity so Ah is clamped between 0 and capacity
    // Initial Ah is set to initial_ah (typically full capacity at startup)
    // Use a short config key (chip_name) for NVS persistence so keys stay within NVS limits
    auto* ah_integ = new AmpHourIntegrator(String(chip_name), initial_ah, battery_capacity_ah);
    current_sensor->connect_to(ah_integ);
    
    // Sample Ah from integrator at 1 Hz for Signal K output (decoupled from 100 Hz integration)
    auto* ah_sk_sampler = new RepeatSensor<float>(1000, [ah_integ]() { return ah_integ->get_ah(); });
    ah_sk_sampler->connect_to(new SKOutputFloat(ah_path, "", new SKMetadata("Ah", "Ampere hours")));
    
    // Convert Ah to State of Charge percentage (0-100%)
    // SOC% = (Ah / Current Capacity) * 100
    // Uses a custom consumer that recalculates based on dynamic capacity
    class SocPercentConsumer : public ValueConsumer<float> {
     public:
      SocPercentConsumer(AmpHourIntegrator* integ, const char* soc_path) : integ_(integ) {
        output_ = new SKOutputFloat(soc_path, "", new SKMetadata("ratio", "State of Charge"));
      }
      void set(const float& ah) override {
        float capacity = integ_->get_current_capacity_ah();
        float soc = (capacity > 0.0f) ? (ah / capacity) * 100.0f : 0.0f;
        // Clamp to 0-100%
        soc = constrain(soc, 0.0f, 100.0f);
        output_->set_input(soc);
      }
     private:
      AmpHourIntegrator* integ_;
      SKOutputFloat* output_;
    };
    
    auto* soc_consumer = new SocPercentConsumer(ah_integ, soc_path);
    ah_sk_sampler->connect_to(soc_consumer);
    
    // Signal K input to allow remote reset/calibration of Ah value
    auto* ah_sk_input = new SKPutRequestListener<float>(ah_path);
    ah_sk_input->connect_to(ah_integ);  // Connect to integrator's set_ah() method
    
    // Signal K inputs for charge/discharge efficiency configuration
    String charge_eff_path = String(ah_path) + "/chargeEfficiency";
    String discharge_eff_path = String(ah_path) + "/dischargeEfficiency";
    String capacity_path = String(ah_path) + "/capacity";  // Current capacity (degrades)
    String marked_capacity_path = String(ah_path) + "/markedCapacity";  // Nameplate capacity
    
    // Create simple consumers that call the efficiency/ah/capacity setters
    class AhConsumer : public ValueConsumer<float> {
     public:
      AhConsumer(AmpHourIntegrator* integ) : integ_(integ) {}
      void set(const float& new_value) override { integ_->set_ah(new_value); }
     private:
      AmpHourIntegrator* integ_;
    };
    
    class CurrentCapacityConsumer : public ValueConsumer<float> {
     public:
      CurrentCapacityConsumer(AmpHourIntegrator* integ) : integ_(integ) {}
      void set(const float& new_value) override { integ_->set_current_capacity_ah(new_value); }
     private:
      AmpHourIntegrator* integ_;
    };
    
    class MarkedCapacityConsumer : public ValueConsumer<float> {
     public:
      MarkedCapacityConsumer(AmpHourIntegrator* integ) : integ_(integ) {}
      void set(const float& new_value) override { integ_->set_marked_capacity_ah(new_value); }
     private:
      AmpHourIntegrator* integ_;
    };
    
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
    
    // Re-wire Ah input to use the AhConsumer for proper clamping
    ah_sk_input->connect_to(new AhConsumer(ah_integ));
    
    auto* charge_eff_input = new SKPutRequestListener<float>(charge_eff_path);
    charge_eff_input->connect_to(new ChargeEfficiencyConsumer(ah_integ));
    
    auto* discharge_eff_input = new SKPutRequestListener<float>(discharge_eff_path);
    discharge_eff_input->connect_to(new DischargeEfficiencyConsumer(ah_integ));
    
    auto* current_capacity_input = new SKPutRequestListener<float>(capacity_path);
    current_capacity_input->connect_to(new CurrentCapacityConsumer(ah_integ));
    
    auto* marked_capacity_input = new SKPutRequestListener<float>(marked_capacity_path);
    marked_capacity_input->connect_to(new MarkedCapacityConsumer(ah_integ));

    auto* power_sensor = new RepeatSensor<float>(read_interval, [&ina]() { return ina.getPower(); });
    power_sensor->connect_to(
        new SKOutputFloat(power_path, "", new SKMetadata("W", "Power")));
    
    // Expose charge/discharge efficiencies as Signal K outputs so the server
    // publishes metadata and allows PUT requests to those paths.
    auto* charge_eff_sampler = new RepeatSensor<float>(1000, [ah_integ]() { return ah_integ->get_charge_efficiency(); });
    charge_eff_sampler->connect_to(new SKOutputFloat(charge_eff_path.c_str(), "", new SKMetadata("%", "Charge Efficiency")));

    auto* discharge_eff_sampler = new RepeatSensor<float>(1000, [ah_integ]() { return ah_integ->get_discharge_efficiency(); });
    discharge_eff_sampler->connect_to(new SKOutputFloat(discharge_eff_path.c_str(), "", new SKMetadata("%", "Discharge Efficiency")));
    
    // Expose battery capacities as readable Signal K outputs
    auto* current_capacity_sampler = new RepeatSensor<float>(1000, [ah_integ]() { return ah_integ->get_current_capacity_ah(); });
    current_capacity_sampler->connect_to(new SKOutputFloat(capacity_path, "", new SKMetadata("Ah", "Current Capacity")));
    
    auto* marked_capacity_sampler = new RepeatSensor<float>(1000, [ah_integ]() { return ah_integ->get_marked_capacity_ah(); });
    marked_capacity_sampler->connect_to(new SKOutputFloat(marked_capacity_path, "", new SKMetadata("Ah", "Marked Capacity")));
}
