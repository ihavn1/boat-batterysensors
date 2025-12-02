#include "ah_integrator.h"
#include <Arduino.h>
#include <Preferences.h>
#include <cmath>

#include "esp_log.h"


namespace sensesp {
  static const char *tag = "DEBUG_AH_INTEG";

AmpHourIntegrator::AmpHourIntegrator(const String& config_path, float initial_ah, float battery_capacity_ah)
    : FloatTransform(config_path), marked_capacity_ah_(battery_capacity_ah),
      battery_capacity_ah_(battery_capacity_ah), config_path_(config_path) {
  ah_output_ = initial_ah;
  this->output_ = initial_ah;  // Keep FloatTransform output in sync
  last_update_ms_ = millis();

  (void)config_path_; (void)initial_ah; (void)battery_capacity_ah;

  // Load persisted capacities and efficiencies if available
  if (config_path_.length() > 0) {
    String key = config_path_;
    key.replace('/', '_');
    Preferences prefs;
    if (prefs.begin("battcfg", false)) {  // open read-write to be safe
      if (prefs.isKey((key + "_marked").c_str())) {
        float v = prefs.getFloat((key + "_marked").c_str(), marked_capacity_ah_);
        marked_capacity_ah_ = v;
      }
      if (prefs.isKey((key + "_current").c_str())) {
        float v = prefs.getFloat((key + "_current").c_str(), battery_capacity_ah_);
        battery_capacity_ah_ = v;
      }
      if (prefs.isKey((key + "_charge").c_str())) {
        float v = prefs.getFloat((key + "_charge").c_str(), charge_efficiency_);
        charge_efficiency_ = v;
      }
      if (prefs.isKey((key + "_discharge").c_str())) {
        float v = prefs.getFloat((key + "_discharge").c_str(), discharge_efficiency_);
        discharge_efficiency_ = v;
      }
      // Load persisted Ah value if present
      if (prefs.isKey((key + "_ah").c_str())) {
        float v = prefs.getFloat((key + "_ah").c_str(), (float)ah_output_);
        ah_output_ = v;
        this->output_ = v;  // Keep FloatTransform output in sync
      }
      prefs.end();
    } else {
      (void)key;
    }
  }

  // Start a timer for internal integration. Interval defined by AH_INTEGRATION_INTERVAL_MS.
  event_loop()->onRepeat(AH_INTEGRATION_INTERVAL_MS, [this]() { this->integrate(); });
  // Start a timer to check whether we should persist the Ah value. Interval defined
  // by AH_PERSIST_CHECK_INTERVAL_MS (default 0.2 Hz -> every 5 seconds).
  event_loop()->onRepeat(AH_PERSIST_CHECK_INTERVAL_MS, [this]() { this->maybe_persist_ah(); });
}

void AmpHourIntegrator::set(const float& new_value) {
  // Store the current reading; integration happens in the timer
  ESP_LOGD(tag, "\n#### %f ##############\n",new_value);
  current_a_ = new_value;
}

void AmpHourIntegrator::set_ah(double ah) {
  // Clamp Ah between 0 and battery capacity
  if (battery_capacity_ah_ > 0) {
    ah_output_ = constrain(ah, 0.0, (double)battery_capacity_ah_);
  } else {
    ah_output_ = ah;
  }
  this->output_ = ah_output_;  // Keep FloatTransform output in sync
  // Mark Ah dirty for periodic persistence (avoid frequent NVS writes)
  ah_dirty_ = true;

  // Also persist immediately because this value was explicitly set via SK PUT
  if (config_path_.length() > 0) {
    String key = config_path_;
    key.replace('/', '_');
    Preferences prefs;
    if (prefs.begin("battcfg", false)) {
      prefs.putFloat((key + "_ah").c_str(), (float)ah_output_);
      last_persisted_ah_ = ah_output_;
      last_ah_persist_ms_ = millis();
      ah_dirty_ = false; // already persisted
      prefs.end();
    }
  }
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
  if (fabs(ah_output_ - last_persisted_ah_) < ah_persist_delta_) {
    // Not enough change
    ah_dirty_ = false; // clear dirty to avoid repeated checks until next change
    return;
  }

  String key = config_path_;
  key.replace('/', '_');
  Preferences prefs;
    if (prefs.begin("battcfg", false)) {
      prefs.putFloat((key + "_ah").c_str(), (float)ah_output_);
      last_persisted_ah_ = ah_output_;
      last_ah_persist_ms_ = now;
      ah_dirty_ = false;
      prefs.end();
    } else {
      (void)key;
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
      prefs.end();
    }
  }
}

void AmpHourIntegrator::integrate() {
  unsigned long now = millis();
  unsigned long dt_ms = now - last_update_ms_;
  last_update_ms_ = now;

  // Convert milliseconds to hours: ms / 3_600_000 - use double for precision
  double dt_hours = static_cast<double>(dt_ms) / 3600000.0;

  // current_a_ is in amperes. delta Ah = A * hours - use double for small deltas
  double delta_ah = current_a_ * dt_hours;

    ESP_LOGD(tag, "\n#### Delta %.9f ##############\n",delta_ah);
  
  // Apply efficiency factor based on current direction
  // Positive current = charging, Negative current = discharging
  double efficiency_factor = (current_a_ > 0) ? (charge_efficiency_ / 100.0) : (discharge_efficiency_ / 100.0);
  delta_ah *= efficiency_factor;
  
  ah_output_ += delta_ah;

   ESP_LOGD(tag, "\n#### Output %.6f ##############\n",ah_output_);
  
  // Clamp Ah between 0 and battery capacity
  if (battery_capacity_ah_ > 0) {
    ah_output_ = constrain(ah_output_, 0.0, (double)battery_capacity_ah_);
  }

  this->output_ = ah_output_;  // Keep FloatTransform output in sync for SK sampling

  // Persist Ah if it changed more than the threshold since last persisted
  if (fabs(ah_output_ - last_persisted_ah_) >= ah_persist_delta_ && config_path_.length() > 0) {
    String key = config_path_;
    key.replace('/', '_');
    Preferences prefs;
    if (prefs.begin("battcfg", false)) {
      prefs.putFloat((key + "_ah").c_str(), (float)ah_output_);
      last_persisted_ah_ = ah_output_;
      last_ah_persist_ms_ = now;
      ah_dirty_ = false;
      prefs.end();
    }
  }

  // Lightweight debug output (can be disabled in production)
  (void)current_a_; (void)charge_efficiency_; (void)discharge_efficiency_; (void)dt_ms; (void)delta_ah; (void)this->output_;

  // Do NOT emit here; let Signal K output sample Ah at its own rate
  // This decouples integration timer from Signal K update rate
}

}  // namespace sensesp
