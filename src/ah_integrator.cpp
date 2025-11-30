#include "ah_integrator.h"
#include <Arduino.h>

namespace sensesp {

AmpHourIntegrator::AmpHourIntegrator(const String& config_path, float initial_ah, float battery_capacity_ah)
    : FloatTransform(config_path), battery_capacity_ah_(battery_capacity_ah) {
  this->output_ = initial_ah;
  last_update_ms_ = millis();
  // Start a 100 Hz timer for internal integration
  // This ensures high-resolution time sampling even if Signal K updates slower
  event_loop()->onRepeat(10, [this]() { this->integrate(); });
}

void AmpHourIntegrator::set(const float& new_value) {
  // Store the current reading; integration happens in the 100 Hz timer
  current_a_ = new_value;
}

void AmpHourIntegrator::set_ah(float ah) {
  // Clamp Ah between 0 and battery capacity
  if (battery_capacity_ah_ > 0) {
    this->output_ = constrain(ah, 0.0f, battery_capacity_ah_);
  } else {
    this->output_ = ah;
  }
}

void AmpHourIntegrator::integrate() {
  unsigned long now = millis();
  unsigned long dt_ms = now - last_update_ms_;
  last_update_ms_ = now;

  // Convert milliseconds to hours: ms / 3_600_000
  float dt_hours = static_cast<float>(dt_ms) / 3600000.0f;

  // current_a_ is in amperes. delta Ah = A * hours
  float delta_ah = current_a_ * dt_hours;
  
  // Apply efficiency factor based on current direction
  // Positive current = charging, Negative current = discharging
  float efficiency_factor = (current_a_ > 0) ? (charge_efficiency_ / 100.0f) : (discharge_efficiency_ / 100.0f);
  delta_ah *= efficiency_factor;
  
  this->output_ += delta_ah;
  
  // Clamp Ah between 0 and battery capacity
  if (battery_capacity_ah_ > 0) {
    this->output_ = constrain(this->output_, 0.0f, battery_capacity_ah_);
  }

  // Lightweight debug output (can be disabled in production)
  Serial.printf("[AmpHourIntegrator] cur=%.3f A eff=%.1f%% dt=%lu ms dAh=%.6f Ah total=%.6f Ah\n",
                current_a_, (current_a_ > 0) ? charge_efficiency_ : discharge_efficiency_, dt_ms, delta_ah, this->output_);

  // Do NOT emit here; let Signal K output sample Ah at 1 Hz instead
  // This decouples high-precision integration (100 Hz) from Signal K updates (1 Hz)
}

}  // namespace sensesp
