#pragma once

#include "sensesp/transforms/transform.h"
#include "sensesp_base_app.h"

namespace sensesp {

// Integrates current (A) over time to produce Amp-hours (Ah).
// Runs internal integration at 100 Hz for accuracy; exposes Ah to consumers
// at their own polling rate (e.g., Signal K output).
class AmpHourIntegrator : public FloatTransform {
 public:
  // config_path is unused for now but kept for consistency with other transforms
  // battery_capacity_ah: capacity in Ah, used to clamp Ah between 0 and capacity
  explicit AmpHourIntegrator(const String& config_path = "", float initial_ah = 0.0f, 
                             float battery_capacity_ah = 0.0f);

  void set(const float& new_value) override;

  float get_ah() const { return this->output_; }
  
  // Set the current Ah value (e.g., from Signal K reset command)
  // Clamped between 0 and battery_capacity_ah
  void set_ah(float ah);
  
  // Get/set efficiency for charging (current > 0), range 0-100%
  float get_charge_efficiency() const { return charge_efficiency_; }
  void set_charge_efficiency(float pct) { charge_efficiency_ = constrain(pct, 0.0f, 100.0f); }
  
  // Get/set efficiency for discharging (current < 0), range 0-100%
  float get_discharge_efficiency() const { return discharge_efficiency_; }
  void set_discharge_efficiency(float pct) { discharge_efficiency_ = constrain(pct, 0.0f, 100.0f); }

 private:
  void integrate();  // Called by internal 100 Hz timer
  unsigned long last_update_ms_ = 0;
  float current_a_ = 0.0f;  // Most recent current reading (A)
  float charge_efficiency_ = 100.0f;    // Efficiency % when charging (current > 0)
  float discharge_efficiency_ = 100.0f; // Efficiency % when discharging (current < 0)
  float battery_capacity_ah_ = 0.0f;    // Battery capacity in Ah (0 = no limit)
};

}  // namespace sensesp
