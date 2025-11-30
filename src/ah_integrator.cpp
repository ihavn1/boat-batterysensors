#include "ah_integrator.h"
#include <Arduino.h>
#include <Preferences.h>
#include <cmath>

namespace sensesp {

AmpHourIntegrator::AmpHourIntegrator(const String& config_path, float initial_ah, float battery_capacity_ah)
    : FloatTransform(config_path), marked_capacity_ah_(battery_capacity_ah),
      battery_capacity_ah_(battery_capacity_ah), config_path_(config_path) {
  this->output_ = initial_ah;
  last_update_ms_ = millis();

  Serial.printf("[AmpHourIntegrator] Constructor called with config_path='%s', initial_ah=%.2f, capacity=%.2f\n",
                config_path_.c_str(), initial_ah, battery_capacity_ah);

  // Load persisted capacities and efficiencies if available
  if (config_path_.length() > 0) {
    String key = config_path_;
    key.replace('/', '_');
    Preferences prefs;
    if (prefs.begin("battcfg", false)) {  // open read-write to be safe
      if (prefs.isKey((key + "_marked").c_str())) {
        float v = prefs.getFloat((key + "_marked").c_str(), marked_capacity_ah_);
        Serial.printf("[AmpHourIntegrator] Loaded %s_marked = %.2f\n", key.c_str(), v);
        marked_capacity_ah_ = v;
      } else {
        Serial.printf("[AmpHourIntegrator] No persisted %s_marked, using %.2f\n", key.c_str(), marked_capacity_ah_);
      }
      if (prefs.isKey((key + "_current").c_str())) {
        float v = prefs.getFloat((key + "_current").c_str(), battery_capacity_ah_);
        Serial.printf("[AmpHourIntegrator] Loaded %s_current = %.2f\n", key.c_str(), v);
        battery_capacity_ah_ = v;
      } else {
        Serial.printf("[AmpHourIntegrator] No persisted %s_current, using %.2f\n", key.c_str(), battery_capacity_ah_);
      }
      if (prefs.isKey((key + "_charge").c_str())) {
        float v = prefs.getFloat((key + "_charge").c_str(), charge_efficiency_);
        Serial.printf("[AmpHourIntegrator] Loaded %s_charge = %.2f\n", key.c_str(), v);
        charge_efficiency_ = v;
      } else {
        Serial.printf("[AmpHourIntegrator] No persisted %s_charge, using %.2f\n", key.c_str(), charge_efficiency_);
      }
      if (prefs.isKey((key + "_discharge").c_str())) {
        float v = prefs.getFloat((key + "_discharge").c_str(), discharge_efficiency_);
        Serial.printf("[AmpHourIntegrator] Loaded %s_discharge = %.2f\n", key.c_str(), v);
        discharge_efficiency_ = v;
      } else {
        Serial.printf("[AmpHourIntegrator] No persisted %s_discharge, using %.2f\n", key.c_str(), discharge_efficiency_);
      }
      // Load persisted Ah value if present
      if (prefs.isKey((key + "_ah").c_str())) {
        float v = prefs.getFloat((key + "_ah").c_str(), this->output_);
        Serial.printf("[AmpHourIntegrator] Loaded %s_ah = %.6f\n", key.c_str(), v);
        this->output_ = v;
      } else {
        Serial.printf("[AmpHourIntegrator] No persisted %s_ah, using %.6f\n", key.c_str(), this->output_);
      }
      prefs.end();
    } else {
      Serial.printf("[AmpHourIntegrator] Failed to open Preferences namespace battcfg for %s\n", key.c_str());
    }
  }

  // Start a 100 Hz timer for internal integration
  // This ensures high-resolution time sampling even if Signal K updates slower
  event_loop()->onRepeat(10, [this]() { this->integrate(); });
  // Start a 1 Hz timer to check whether we should persist the Ah value
  event_loop()->onRepeat(1000, [this]() { this->maybe_persist_ah(); });
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
  // Mark Ah dirty for periodic persistence (avoid frequent NVS writes)
  ah_dirty_ = true;
}

void AmpHourIntegrator::maybe_persist_ah() {
  if (!ah_dirty_ || config_path_.length() == 0) {
    return;
  }
  unsigned long now = millis();
  if (now - last_ah_persist_ms_ < ah_persist_interval_ms_) {
    // Not yet time to persist
    return;
  }
  // Persist only if delta since last persisted exceeds threshold
  if (fabs(this->output_ - last_persisted_ah_) < ah_persist_delta_) {
    // Not enough change
    ah_dirty_ = false; // clear dirty to avoid repeated checks until next change
    return;
  }

  String key = config_path_;
  key.replace('/', '_');
  Preferences prefs;
  if (prefs.begin("battcfg", false)) {
    bool ok = prefs.putFloat((key + "_ah").c_str(), this->output_);
    if (ok) {
      Serial.printf("[AmpHourIntegrator] Persisted %s_ah = %.6f\n", key.c_str(), this->output_);
      last_persisted_ah_ = this->output_;
      last_ah_persist_ms_ = now;
      ah_dirty_ = false;
    } else {
      Serial.printf("[AmpHourIntegrator] Failed to persist %s_ah (putFloat returned false)\n", key.c_str());
    }
    prefs.end();
  } else {
    Serial.printf("[AmpHourIntegrator] Failed to open Preferences namespace battcfg to persist %s_ah\n", key.c_str());
  }
}

void AmpHourIntegrator::set_marked_capacity_ah(float capacity_ah) {
  marked_capacity_ah_ = constrain(capacity_ah, 0.1f, 10000.0f);
  // Persist
  if (config_path_.length() > 0) {
    String key = config_path_;
    key.replace('/', '_');
    Preferences prefs;
    if (prefs.begin("battcfg", false)) {
      prefs.putFloat((key + "_marked").c_str(), marked_capacity_ah_);
      Serial.printf("[AmpHourIntegrator] Persisted %s_marked = %.2f\n", key.c_str(), marked_capacity_ah_);
      prefs.end();
    }
  }
}

void AmpHourIntegrator::set_current_capacity_ah(float capacity_ah) {
  battery_capacity_ah_ = constrain(capacity_ah, 0.1f, 10000.0f);
  // Persist
  if (config_path_.length() > 0) {
    String key = config_path_;
    key.replace('/', '_');
    Preferences prefs;
    if (prefs.begin("battcfg", false)) {
      prefs.putFloat((key + "_current").c_str(), battery_capacity_ah_);
      Serial.printf("[AmpHourIntegrator] Persisted %s_current = %.2f\n", key.c_str(), battery_capacity_ah_);
      prefs.end();
    }
  }
}

void AmpHourIntegrator::set_charge_efficiency(float pct) {
  charge_efficiency_ = constrain(pct, 0.0f, 100.0f);
  if (config_path_.length() > 0) {
    String key = config_path_;
    key.replace('/', '_');
    Preferences prefs;
    if (prefs.begin("battcfg", false)) {
      prefs.putFloat((key + "_charge").c_str(), charge_efficiency_);
      Serial.printf("[AmpHourIntegrator] Persisted %s_charge = %.2f\n", key.c_str(), charge_efficiency_);
      prefs.end();
    }
  }
}

void AmpHourIntegrator::set_discharge_efficiency(float pct) {
  discharge_efficiency_ = constrain(pct, 0.0f, 100.0f);
  if (config_path_.length() > 0) {
    String key = config_path_;
    key.replace('/', '_');
    Preferences prefs;
    if (prefs.begin("battcfg", false)) {
      prefs.putFloat((key + "_discharge").c_str(), discharge_efficiency_);
      Serial.printf("[AmpHourIntegrator] Persisted %s_discharge = %.2f\n", key.c_str(), discharge_efficiency_);
      prefs.end();
    }
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
